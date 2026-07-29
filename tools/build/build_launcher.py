import os
import subprocess
from pathlib import Path
from logger import step, ok, info, fail_

def run():
    step("Building local launcher (linux)")

    # Détection des sources
    src_dir = Path("src")
    sources = sorted([str(p) for p in src_dir.rglob("*") if p.suffix in [".c", ".cpp"]])
    
    if not sources:
        fail_("No C/C++ sources found in src/")
    ok(f"{len(sources)} sources detected")

    # Préparation des objets pour le Makefile
    objs = []
    for s in sources:
        obj = s.replace("src/", "obj/").replace(".cpp", ".o").replace(".c", ".o")
        objs.append(obj)
    objs_str = " ".join(objs)

    # Détection Tray
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

    # Génération du Makefile
    info("Generating minimal Makefile")
    
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

clean:
>rm -rf obj $(TARGET)
"""
    with open("Makefile", "w") as f:
        f.write(makefile_content.strip() + "\n")
    ok("Makefile generated")

    # Compilation
    info("Compiling launcher...")
    result = subprocess.run(["make"])
    
    if result.returncode != 0:
        fail_("Build failed")
        
    # Nettoyage du Makefile
    if os.path.exists("Makefile"):
        os.remove("Makefile")