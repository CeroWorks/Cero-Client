import shutil
import subprocess
from logger import step, ok, fail_

def run():
    step("Checking dependencies...")
    
    for cmd in ["python3", "g++", "pkg-config"]:
        if not shutil.which(cmd):
            fail_(f"{cmd} is required.")
            
    if subprocess.run(["pkg-config", "--exists", "webkit2gtk-4.1"]).returncode != 0:
        fail_("webkit2gtk-4.1 is missing. Install with: sudo apt install libwebkit2gtk-4.1-dev")
        
    ok("All dependencies satisfied.")