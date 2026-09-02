import sys

if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

C_BOLD = "\033[1m"
C_DIM = "\033[2m"
C_GREEN = "\033[32m"
C_RED = "\033[31m"
C_CYAN = "\033[36m"
C_YELLOW = "\033[33m"
C_RESET = "\033[0m"

def step(msg):
    print(f"\n{C_BOLD}{C_CYAN}»{C_RESET} {C_BOLD}{msg}{C_RESET}")

def ok(msg):
    print(f"  {C_GREEN}✓{C_RESET} {msg}")

def info(msg):
    print(f"  {C_DIM}›{C_RESET} {msg}")

def warn_(msg):
    print(f"  {C_YELLOW}!{C_RESET} {msg}")

def fail_(msg):
    print(f"  {C_RED}✗{C_RESET} {msg}")
    sys.exit(1)