import os
import shutil
from logger import step, ok, fail_

def run():
    step("Finalizing build...")
    
    if not os.path.exists("CeroClient"):
        fail_("CeroClient binary not found after build!")
        
    dest_path = "bin/linux/client/CeroClient"
    
    # Si le fichier existe déjà, on le supprime avant de le remplacer
    if os.path.exists(dest_path):
        os.remove(dest_path)
        
    shutil.move("CeroClient", dest_path)
    ok(f"Launcher installed in {dest_path}")