import shutil
import subprocess
import sys
import os
import zipfile
import urllib.request
import urllib.error
from pathlib import Path
from logger import step, ok, info, warn_, fail_

WEBVIEW2_VERSION = "1.0.2592.51"
WEBVIEW2_NUPKG_URL = (
    "https://www.nuget.org/api/v2/package/"
    f"Microsoft.Web.WebView2/{WEBVIEW2_VERSION}"
)
WEBVIEW2_SDK_DIR = Path("third_party") / "webview2_sdk"


def _arch_tag():
    machine = os.environ.get("PROCESSOR_ARCHITECTURE", "").upper()
    machine_w6432 = os.environ.get("PROCESSOR_ARCHITEW6432", "").upper()
    combined = machine_w6432 or machine
    if "ARM64" in combined:
        return "arm64"
    if combined in ("X86", "I386", "I686"):
        return "x86"
    return "x64"


def _download(url, dest):
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "CeroClient-Build"})
        with urllib.request.urlopen(req, timeout=60) as resp, open(dest, "wb") as out:
            shutil.copyfileobj(resp, out)
    except (urllib.error.URLError, OSError) as e:
        fail_(f"Download failed: {url} ({e})")


def ensure_webview2_sdk():
    """
    Downloads and extracts the WebView2 NuGet package if missing.
    Returns (include_dir, lib_dir, runtime_dll_path).
    """
    arch = _arch_tag()
    include_dir = WEBVIEW2_SDK_DIR / "build" / "native" / "include"
    lib_dir = WEBVIEW2_SDK_DIR / "build" / "native" / arch
    dll_path = (
        WEBVIEW2_SDK_DIR / "runtimes" / f"win-{arch}" / "native" / "WebView2Loader.dll"
    )

    needed = [
        include_dir / "WebView2.h",
        lib_dir / "WebView2Loader.dll.lib",
        dll_path,
    ]

    if all(p.exists() for p in needed):
        ok(f"WebView2 SDK found ({arch}).")
        return include_dir, lib_dir, dll_path

    info(f"WebView2 SDK missing. Downloading {WEBVIEW2_VERSION} ({arch})...")
    WEBVIEW2_SDK_DIR.mkdir(parents=True, exist_ok=True)
    nupkg = WEBVIEW2_SDK_DIR / "webview2.nupkg"

    _download(WEBVIEW2_NUPKG_URL, nupkg)

    try:
        with zipfile.ZipFile(nupkg) as z:
            for member in z.namelist():
                low = member.lower()
                keep = (
                    low.startswith("build/native/include/")
                    or low.startswith(f"build/native/{arch}/")
                    or low.startswith(f"runtimes/win-{arch}/native/")
                )
                if keep and not member.endswith("/"):
                    z.extract(member, WEBVIEW2_SDK_DIR)
    except zipfile.BadZipFile:
        nupkg.unlink(missing_ok=True)
        fail_("Downloaded WebView2 package is corrupted.")
    finally:
        nupkg.unlink(missing_ok=True)

    missing = [p for p in needed if not p.exists()]
    if missing:
        fail_(
            "WebView2 SDK extraction incomplete: "
            + ", ".join(str(p) for p in missing)
        )

    ok(f"WebView2 SDK ready ({arch}).")
    return include_dir, lib_dir, dll_path


def check_webview2_runtime():
    """Non-fatal check: is the Evergreen runtime installed on this machine?"""
    if sys.platform != "win32":
        return
    try:
        import winreg
    except ImportError:
        return

    guid = "{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
    keys = [
        (winreg.HKEY_LOCAL_MACHINE,
         rf"SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{guid}"),
        (winreg.HKEY_LOCAL_MACHINE,
         rf"SOFTWARE\Microsoft\EdgeUpdate\Clients\{guid}"),
        (winreg.HKEY_CURRENT_USER,
         rf"SOFTWARE\Microsoft\EdgeUpdate\Clients\{guid}"),
    ]
    for root, path in keys:
        try:
            with winreg.OpenKey(root, path) as k:
                version, _ = winreg.QueryValueEx(k, "pv")
                if version and version != "0.0.0.0":
                    ok(f"WebView2 Runtime detected (v{version}).")
                    return
        except OSError:
            continue

    warn_("WebView2 Runtime not detected on this machine.")
    warn_("The launcher window will not render. Install the Evergreen Runtime:")
    warn_("  https://developer.microsoft.com/microsoft-edge/webview2/")


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

    if is_windows:
        ensure_webview2_sdk()
        check_webview2_runtime()

    ok("All dependencies satisfied.")