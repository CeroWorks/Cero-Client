import shutil
import subprocess
import os
from logger import step, ok, info, warn_, fail_

def run():
    step("Checking dependencies...")
    
    for cmd in ["python3", "g++", "pkg-config"]:
        if not shutil.which(cmd):
            fail_(f"{cmd} is required.")
            
    if subprocess.run(["pkg-config", "--exists", "webkit2gtk-4.1"]).returncode != 0:
        fail_("webkit2gtk-4.1 is missing. Install with: sudo apt install libwebkit2gtk-4.1-dev")
    
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
            fail_("Failed to download webview library. Please check your internet connection.")
        ok("WebView downloaded successfully.")
    else:
        ok("WebView library found.")
        
    ok("All dependencies satisfied.")