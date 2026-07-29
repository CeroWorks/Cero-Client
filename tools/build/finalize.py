import os
import sys
import shutil
from logger import step, ok, fail_

def run():
    step("Finalizing build...")
    
    if sys.platform == "win32":
        binary_name = "CeroClient.exe"
        dest_dir = "bin/windows/client"
    else:
        binary_name = "CeroClient"
        dest_dir = "bin/linux/client"
        
    if not os.path.exists(binary_name):
        fail_(f"{binary_name} binary not found after build!")
        
    dest_path = os.path.join(dest_dir, binary_name)
    
    os.makedirs(dest_dir, exist_ok=True)
    if os.path.exists(dest_path):
        os.remove(dest_path)
        
    shutil.move(binary_name, dest_path)
    ok(f"Launcher installed in {dest_path}")