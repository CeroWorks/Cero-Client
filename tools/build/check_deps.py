import shutil
import subprocess
import sys
import os
from logger import step, ok, info, warn_, fail_

def run():
    step("Checking dependencies...")
    
    is_windows = (sys.platform == "win32")
    
    if is_windows:
        cmds = ["python", "gcc", "g++"]
    else:
        cmds = ["python3", "cc", "c++", "pkg-config"]
    
    for cmd in cmds:
        if not shutil.which(cmd):
            fail_(f"{cmd} is required.")
            
    webview_path = os.path.join("third_party", "webview")
    if not os.path.isdir(webview_path):
        info("WebView library not found. Downloading...")
        os.makedirs("third_party", exist_ok=True)
        
        result = subprocess.run([
            "git", "clone", "--depth", "1", 
            "https://github.com/webview/webview.git", 
            webview_path
        ])
        
        if result.returncode != 0:
            fail_("Failed to download webview library.")
        ok("WebView downloaded successfully.")
    else:
        ok("WebView library found.")
        
    ok("All dependencies satisfied.")