import os
from config import (
    ASSETS_ROOT_ABS, IMPORT_REGEX, REF_REGEX, ASSET_HINT_REGEX, 
    MAX_IMPORT_DEPTH, MAX_TOTAL_IMPORTS, MAX_FILE_SIZE
)
from utils import safe_join, inject_version

_import_counter = {"n": 0}

def parse_and_include(file_path: str, stack=None, depth: int = 0) -> str:
    stack = stack or set()
    if depth > MAX_IMPORT_DEPTH:
        return "<!-- max depth -->"
    if _import_counter["n"] > MAX_TOTAL_IMPORTS:
        return "<!-- import budget exhausted -->"
    _import_counter["n"] += 1

    abs_path = os.path.abspath(file_path)
    if not abs_path.startswith(ASSETS_ROOT_ABS + os.sep) and abs_path != ASSETS_ROOT_ABS:
        return "<!-- out of root -->"
    if abs_path in stack:
        return "<!-- circular -->"
    if not os.path.isfile(abs_path):
        return "<!-- missing -->"
    if os.path.getsize(abs_path) > MAX_FILE_SIZE:
        return "<!-- too large -->"

    with open(abs_path, "r", encoding="utf-8") as f:
        content = f.read()
    content = inject_version(content)
    current_dir = os.path.dirname(abs_path)
    new_stack = stack | {abs_path}

    def replace_match(m):
        rel = m.group(1)
        target = safe_join(ASSETS_ROOT_ABS, os.path.relpath(
            os.path.normpath(os.path.join(current_dir, rel)), ASSETS_ROOT_ABS))
        if not target:
            return "<!-- bad import path -->"
        return parse_and_include(target, new_stack, depth + 1)

    return IMPORT_REGEX.sub(replace_match, content)

def normalize_ref(ref: str, base_dir: str):
    if not ref or ref.startswith(("http://","https://","//","data:","mailto:","#","javascript:")):
        return None
    ref = ref.split("?")[0].split("#")[0]
    if not ref:
        return None
    target = (os.path.normpath(os.path.join(ASSETS_ROOT_ABS, ref.lstrip("/")))
              if ref.startswith("/")
              else os.path.normpath(os.path.join(base_dir, ref)))
    try:
        if os.path.commonpath([ASSETS_ROOT_ABS, os.path.abspath(target)]) != ASSETS_ROOT_ABS:
            return None
    except ValueError:
        return None
    return os.path.relpath(target, ASSETS_ROOT_ABS).replace("\\", "/")

def collect_refs(content: str, base_dir: str) -> set:
    found = set()
    for m in REF_REGEX.finditer(content):
        raw = m.group(1) or m.group(2) or m.group(3)
        if not raw:
            continue
        candidates = ([c.strip().split()[0] for c in raw.split(",")]
                      if "," in raw else [raw.strip()])
        for ref in candidates:
            n = normalize_ref(ref, base_dir)
            if n:
                found.add(n)
    for m in ASSET_HINT_REGEX.finditer(content):
        raw = m.group(1).strip().strip('"\'')
        n = normalize_ref(raw, ASSETS_ROOT_ABS)
        if n:
            found.add(n)
    return found