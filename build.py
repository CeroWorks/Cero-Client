#!/usr/bin/env python3
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "tools", "build"))

from logger import step, ok, info, warn_, fail_, C_BOLD, C_GREEN, C_RESET
import check_deps
import prepare_dirs
import build_agent
import build_bootstrapper
import package_assets
import build_launcher
import finalize

def main():
    start_time = time.time()
    
    step("CeroClient Build Pipeline")
    
    try:
        check_deps.run()
        prepare_dirs.run()
        build_agent.run()
        build_bootstrapper.run()
        package_assets.run()
        build_launcher.run()
        finalize.run()
        
        duration = time.time() - start_time
        print(f"\n{C_BOLD}{C_GREEN}Build finished successfully in {duration:.2f}s !{C_RESET}\n")
        
    except SystemExit as e:
        sys.exit(e.code)
    except Exception as e:
        fail_(f"Unexpected error: {e}")

if __name__ == "__main__":
    main()