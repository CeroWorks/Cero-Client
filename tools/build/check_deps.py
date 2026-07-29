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
            
    if not is_windows:
        pkg_config_path = os.environ.get("PKG_CONFIG_PATH", "")
        freebsd_paths = "/usr/local/lib/pkgconfig:/usr/local/libdata/pkgconfig"
        if not pkg_config_path:
            os.environ["PKG_CONFIG_PATH"] = freebsd_paths
        elif freebsd_paths not in pkg_config_path:
            os.environ["PKG_CONFIG_PATH"] = pkg_config_path + ":" + freebsd_paths

        webkit_found = False
        if subprocess.run(["pkg-config", "--exists", "webkit2gtk-4.1"]).returncode == 0:
            webkit_found = True
        elif subprocess.run(["pkg-config", "--exists", "webkit2gtk-4.0"]).returncode == 0:
            webkit_found = True
            
        if not webkit_found:
            fail_("webkit2gtk is missing. Install webkit2gtk-4.1 (Linux) or webkit2-gtk_41 (FreeBSD)")
            
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