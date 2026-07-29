import os
import sys
import subprocess
from pathlib import Path
from logger import step, ok, info, fail_

def run():
    is_windows = (sys.platform == "win32")
    step(f"Building local launcher ({'windows' if is_windows else 'linux'})")

    src_dir = Path("src")
    sources = sorted([str(p) for p in src_dir.rglob("*") if p.suffix in [".c", ".cpp"]])
    
    if is_windows:
        sources = [s.replace("\\", "/") for s in sources]
    
    if not sources:
        fail_("No C/C++ sources found in src/")
    ok(f"{len(sources)} sources detected")

    objs = []
    for s in sources:
        obj = s.replace("src/", "obj/").replace(".cpp", ".o").replace(".c", ".o")
        objs.append(obj)
    objs_str = " ".join(objs)

    if is_windows:
        win_defs = "-D_WIN32 -DWIN32_LEAN_AND_MEAN -D_WINSOCKAPI_"
        curl_static_deps = "-Wl,--start-group -l:libcurl.a -l:libssh2.a -l:libnghttp2.a -l:libnghttp3.a -l:libngtcp2.a -l:libngtcp2_crypto_libressl.a -l:libssl.a -l:libcrypto.a -l:libz.a -l:libzstd.a -l:libbrotlidec.a -l:libbrotlicommon.a -l:libpsl.a -Wl,--end-group"
        win_libs = "-lws2_32 -lwldap32 -lcrypt32 -lnormaliz -lsecur32 -liphlpapi -lWebView2Loader -lole32 -lshlwapi -lversion -ladvapi32 -luser32 -lshell32 -lgdi32 -static-libgcc -static-libstdc++ -ldwmapi -lwininet -lbcrypt -Wl,--defsym=fstat64=_fstat64 -s -Wl,-subsystem,windows"
        
        makefile_content = f"""
.RECIPEPREFIX = >
CC       = gcc
CXX      = g++
CFLAGS   = -O2 -std=c11 -Iinclude -Ithird_party/webview/core/include {win_defs} -Wno-unused-function
CXXFLAGS = -O2 -std=c++17 -Iinclude -Ithird_party/webview/core/include {win_defs} -Wno-unused-function
LDFLAGS  = {curl_static_deps} {win_libs}

TARGET   = CeroClient.exe
OBJS     = {objs_str}

 $(TARGET): $(OBJS)
>$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
>@mkdir -p $(dir $@)
>$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
>@mkdir -p $(dir $@)
>$(CXX) $(CXXFLAGS) -c $< -o $@
"""
    else:
        tray_cflags = ""
        tray_ldflags = ""
        tray_define = ""

        if subprocess.run(["pkg-config", "--exists", "ayatana-appindicator3-0.1"]).returncode == 0:
            tray_cflags = subprocess.check_output(["pkg-config", "--cflags", "ayatana-appindicator3-0.1"]).decode().strip()
            tray_ldflags = subprocess.check_output(["pkg-config", "--libs", "ayatana-appindicator3-0.1"]).decode().strip()
            tray_define = "-DHAVE_AYATANA"
            info("Tray: Ayatana AppIndicator detected")
        elif subprocess.run(["pkg-config", "--exists", "appindicator3-0.1"]).returncode == 0:
            tray_cflags = subprocess.check_output(["pkg-config", "--cflags", "appindicator3-0.1"]).decode().strip()
            tray_ldflags = subprocess.check_output(["pkg-config", "--libs", "appindicator3-0.1"]).decode().strip()
            tray_define = "-DHAVE_APPINDICATOR"
            info("Tray: AppIndicator detected")
        else:
            info("Tray: NOT detected (neither Ayatana nor AppIndicator) - disabled")

        makefile_content = f"""
.RECIPEPREFIX = >
CC       = cc
CXX      = c++
CFLAGS   = -O2 -Iinclude -Ithird_party/webview/core/include {tray_define} {tray_cflags} $(shell pkg-config --cflags libcurl)
CXXFLAGS = -O2 -std=c++17 -Iinclude -Ithird_party/webview/core/include $(shell pkg-config --cflags gtk+-3.0 webkit2gtk-4.1 libcurl)
LDFLAGS  = $(shell pkg-config --libs gtk+-3.0 webkit2gtk-4.1 libcurl) {tray_ldflags}

TARGET   = CeroClient
OBJS     = {objs_str}

 $(TARGET): $(OBJS)
>$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

obj/%.o: src/%.c
>@mkdir -p $(dir $@)
>$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
>@mkdir -p $(dir $@)
>$(CXX) $(CXXFLAGS) -c $< -o $@
"""

    with open("Makefile", "w") as f:
        f.write(makefile_content.strip() + "\n")
    ok("Makefile generated")

    info("Compiling launcher...")
    make_cmd = "mingw32-make" if is_windows else "make"
    result = subprocess.run([make_cmd])
    
    if result.returncode != 0:
        fail_("Build failed")
        
    if os.path.exists("Makefile"):
        os.remove("Makefile")