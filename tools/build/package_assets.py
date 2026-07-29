import subprocess
import sys
import os
from logger import step, ok, fail_

def run():
    step("Packaging assets...")
    script_path = os.path.join("tools", "pack.py")
    
    result = subprocess.run([
        sys.executable, script_path, "assets", "bin/assets/assets.dat"
    ])
    
    if result.returncode != 0:
        fail_("Asset packaging failed")
    ok("Assets packaged successfully")