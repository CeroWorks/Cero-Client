import os
import sys
import subprocess
from pathlib import Path
from logger import step, ok, info, fail_

def run():
    step(f"Building local launcher ({'windows' if sys.platform == "win32" else 'linux/bsd'})")

    env = os.environ.copy()

    src_dir = Path("src")
    sources = sorted([str(p) for p in src_dir.rglob("*") if p.suffix in [".c", ".cpp"]])
    
    if sys.platform == "win32":
        sources = [s.replace("\\", "/") for s in sources]
    
    if not sources:
        fail_("No C/C++ sources found in src/")
    ok(f"{len(sources)} sources detected")

    objs = []
    for s in sources:
        obj = s.replace("src/", "obj/").replace(".cpp", ".o").replace(".c", ".o")
        objs.append(obj)
    objs_str = " ".join(objs)

    if sys.platform == "win32":
        win_defs = "-D_WIN32 -DWIN32_LEAN_AND_MEAN -D_WINSOCKAPI_"
        
        webview2_inc = os.environ.get("WEBVIEW2_INCLUDE", "")
        webview2_lib = os.environ.get("WEBVIEW2_LIB", "")
        
        inc_flags = "-Iinclude -Ithird_party/webview/core/include"
        if webview2_inc:
            inc_flags += f" -I{webview2_inc}"
            
        lib_flags = ""
        if webview2_lib:
            lib_flags += f" -L{webview2_lib}"
        
        if os.path.exists("/usr/lib/libngtcp2_crypto_libressl.a") or os.path.exists("C:/msys64/mingw64/lib/libngtcp2_crypto_libressl.a"):
            curl_static_deps = "-Wl,--start-group -l:libcurl.a -l:libssh2.a -l:libnghttp2.a -l:libnghttp3.a -l:libngtcp2.a -l:libngtcp2_crypto_libressl.a -l:libssl.a -l:libcrypto.a -l:libz.a -l:libzstd.a -l:libbrotlidec.a -l:libbrotlicommon.a -l:libpsl.a -Wl,--end-group"
        else:
            curl_static_deps = "-lcurl -lssl -lcrypto -lssh2 -lnghttp2 -lnghttp3 -lz -lzstd -lbrotlidec -lbrotlicommon -lpsl -lws2_32 -lwldap32 -lcrypt32 -lnormaliz -lsecur32 -liphlpapi"
            
        win_libs = f"{lib_flags} -lws2_32 -lwldap32 -lcrypt32 -lnormaliz -lsecur32 -liphlpapi -l:WebView2Loader.dll.lib -lole32 -lshlwapi -lversion -ladvapi32 -luser32 -lshell32 -lgdi32 -static-libgcc -static-libstdc++ -ldwmapi -lwininet -lbcrypt -Wl,--defsym=fstat64=_fstat64 -s -Wl,-subsystem,windows"
        
        TAB = "\t"
        makefile_content = f"""CC       = gcc
CXX      = g++
CFLAGS   = -O2 -std=c11 {inc_flags} {win_defs} -Wno-unused-function
CXXFLAGS = -O2 -std=c++17 {inc_flags} {win_defs} -Wno-unused-function
LDFLAGS  = {curl_static_deps} {win_libs}

TARGET   = CeroClient.exe
OBJS     = {objs_str}

 $(TARGET): $(OBJS)
{TAB}$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
{TAB}@mkdir -p $(dir $@)
{TAB}$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
{TAB}@mkdir -p $(dir $@)
{TAB}$(CXX) $(CXXFLAGS) -c $< -o $@
"""
    elif sys.platform == "darwin":
        # --- CONFIGURATION macOS (Apple Silicon & Intel) ---
        # macOS utilise son WebKit natif via Cocoa, pas besoin de pkg-config pour ça.
        # On utilise brew pour curl.
        brew_prefix = "/opt/homebrew" if os.path.exists("/opt/homebrew") else "/usr/local"
        
        inc_flags = f"-Iinclude -Ithird_party/webview/core/include -I{brew_prefix}/include"
        lib_flags = f"-L{brew_prefix}/lib -lcurl"
        
        # Frameworks macOS nécessaires pour WebView et l'interface
        frameworks = "-framework WebKit -framework Cocoa -framework AppKit -framework Foundation"
        
        TAB = "\t"
        makefile_content = f"""CC       = clang
CXX      = clang++
CFLAGS   = -O2 -std=c11 {inc_flags} -Wno-unused-function
CXXFLAGS = -O2 -std=c++17 {inc_flags} -Wno-unused-function
LDFLAGS  = {lib_flags} {frameworks}

TARGET   = CeroClient
OBJS     = {objs_str}

 $(TARGET): $(OBJS)
{TAB}$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
{TAB}@mkdir -p $(dir $@)
{TAB}$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
{TAB}@mkdir -p $(dir $@)
{TAB}$(CXX) $(CXXFLAGS) -c $< -o $@
"""
    else:
        pkg_config_path_export = ""
        if "freebsd" in sys.platform:
            bsd_paths = "/usr/local/lib/pkgconfig:/usr/local/libdata/pkgconfig"
            env["PKG_CONFIG_PATH"] = bsd_paths + ":" + env.get("PKG_CONFIG_PATH", "")
            pkg_config_path_export = f"export PKG_CONFIG_PATH={bsd_paths} && "

        webkit_pkg = "webkit2gtk-4.1"
        if subprocess.run(["pkg-config", "--exists", webkit_pkg], env=env).returncode != 0:
            webkit_pkg = "webkit2gtk-4.0"
            info("Using webkit2gtk-4.0")
        else:
            info("Using webkit2gtk-4.1")

        tray_cflags = ""
        tray_ldflags = ""
        tray_define = ""

        if subprocess.run(["pkg-config", "--exists", "ayatana-appindicator3-0.1"], env=env).returncode == 0:
            tray_cflags = subprocess.check_output(["pkg-config", "--cflags", "ayatana-appindicator3-0.1"], env=env).decode().strip()
            tray_ldflags = subprocess.check_output(["pkg-config", "--libs", "ayatana-appindicator3-0.1"], env=env).decode().strip()
            tray_define = "-DHAVE_AYATANA"
            info("Tray: Ayatana AppIndicator detected")
        elif subprocess.run(["pkg-config", "--exists", "appindicator3-0.1"], env=env).returncode == 0:
            tray_cflags = subprocess.check_output(["pkg-config", "--cflags", "appindicator3-0.1"], env=env).decode().strip()
            tray_ldflags = subprocess.check_output(["pkg-config", "--libs", "appindicator3-0.1"], env=env).decode().strip()
            tray_define = "-DHAVE_APPINDICATOR"
            info("Tray: AppIndicator detected")
        else:
            info("Tray: NOT detected - disabled")

        TAB = "\t"
        makefile_content = f"""CC       = cc
CXX      = c++
CFLAGS   = -O2 -Iinclude -Ithird_party/webview/core/include {tray_define} {tray_cflags} $(shell {pkg_config_path_export}pkg-config --cflags libcurl)
CXXFLAGS = -O2 -std=c++17 -Iinclude -Ithird_party/webview/core/include $(shell {pkg_config_path_export}pkg-config --cflags gtk+-3.0 {webkit_pkg} libcurl)
LDFLAGS  = $(shell {pkg_config_path_export}pkg-config --libs gtk+-3.0 {webkit_pkg} libcurl) {tray_ldflags}

TARGET   = CeroClient
OBJS     = {objs_str}

 $(TARGET): $(OBJS)
{TAB}$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
{TAB}@mkdir -p $(dir $@)
{TAB}$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
{TAB}@mkdir -p $(dir $@)
{TAB}$(CXX) $(CXXFLAGS) -c $< -o $@
"""

    with open("Makefile", "w") as f:
        f.write(makefile_content.strip() + "\n")
    ok("Makefile generated")

    info("Compiling launcher...")
    if sys.platform == "win32":
        make_cmd = "mingw32-make"
    elif "freebsd" in sys.platform:
        make_cmd = "gmake"
    else:
        make_cmd = "make"
        
    result = subprocess.run([make_cmd], env=env)
    
    if result.returncode != 0:
        fail_("Build failed")
        
    if os.path.exists("Makefile"):
        os.remove("Makefile")