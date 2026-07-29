import os
from logger import step, ok

def run():
    step("Preparing directories...")
    os.makedirs("obj", exist_ok=True)
    os.makedirs("bin/linux/client", exist_ok=True)
    os.makedirs("bin/assets", exist_ok=True)
    ok("Directories ready.")