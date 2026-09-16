import os
import shutil
import subprocess
import sys
from logger import step, ok, info, warn_, fail_

ASSETS_DIR = "assets"
CONFIG_PATH = "tailwind.config.js"
INPUT_CSS = os.path.join("style", "tailwind.src.css")
OUTPUT_CSS = os.path.join("style", "tailwind.css")

DEFAULT_INPUT_CSS = "@tailwind base;\n@tailwind components;\n@tailwind utilities;\n"


def _npx_cmd():
    return "npx.cmd" if sys.platform == "win32" else "npx"


def run():
    step("Building Tailwind CSS...")

    config_full_path = os.path.join(ASSETS_DIR, CONFIG_PATH)
    if not os.path.exists(config_full_path):
        fail_(f"{config_full_path} introuvable")

    output_full_path = os.path.join(ASSETS_DIR, OUTPUT_CSS)

    if not shutil.which(_npx_cmd()):
        if os.path.exists(output_full_path):
            warn_("npx introuvable - Node.js n'est pas installé, on garde "
                  f"le {OUTPUT_CSS} existant (potentiellement obsolète)")
        else:
            warn_("npx introuvable - Node.js n'est pas installé, impossible de "
                  "générer le CSS Tailwind (l'UI risque d'être mal stylée)")
        return

    if not os.path.isdir("node_modules"):
        info("node_modules absent, installation des dépendances npm...")
        result = subprocess.run(["npm", "install"], shell=(sys.platform == "win32"))
        if result.returncode != 0:
            fail_("npm install a échoué")

    input_full_path = os.path.join(ASSETS_DIR, INPUT_CSS)
    if not os.path.exists(input_full_path):
        info(f"{input_full_path} absent, création avec les directives Tailwind par défaut")
        with open(input_full_path, "w") as f:
            f.write(DEFAULT_INPUT_CSS)

    cmd = [
        _npx_cmd(), "tailwindcss",
        "-c", CONFIG_PATH,
        "-i", INPUT_CSS,
        "-o", OUTPUT_CSS,
        "--minify",
    ]

    result = subprocess.run(cmd, cwd=ASSETS_DIR, shell=(sys.platform == "win32"))
    if result.returncode != 0:
        fail_("Tailwind build failed")

    if not os.path.exists(output_full_path):
        fail_(f"{output_full_path} n'a pas été généré")

    size = os.path.getsize(output_full_path)
    ok(f"Tailwind CSS généré ({size} octets) -> {output_full_path}")