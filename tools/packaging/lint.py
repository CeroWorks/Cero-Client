import os
import re
from config import (
    ASSETS_ROOT, JS_FUNC_DEF_RE, JS_CALL_RE, HTML_HANDLER_RE, JS_BUILTINS
)
from logger import warn_, ok

def extract_js_from_html(html: str) -> str:
    chunks = []
    for m in re.finditer(r'<script\b[^>]*>(.*?)</script>', html, re.DOTALL | re.IGNORECASE):
        tag = html[m.start():m.start() + m.group(0).index('>') + 1]
        if re.search(r'\bsrc\s*=', tag, re.IGNORECASE):
            continue
        chunks.append(m.group(1))
    for m in HTML_HANDLER_RE.finditer(html):
        chunks.append(m.group(1))
    return "\n".join(chunks)

def lint_js_definitions():
    defined, called, sources = set(), {}, []
    for root, _, files in os.walk(ASSETS_ROOT):
        for f in sorted(files):
            full = os.path.join(root, f)
            ext = f.lower().rsplit('.', 1)[-1]
            try:
                if ext == 'js':
                    with open(full, 'r', encoding='utf-8', errors='ignore') as fh:
                        sources.append((full, fh.read()))
                elif ext in ('html', 'htm'):
                    with open(full, 'r', encoding='utf-8', errors='ignore') as fh:
                        js = extract_js_from_html(fh.read())
                    if js.strip():
                        sources.append((full, js))
            except OSError:
                pass
    for path, code in sources:
        for m in JS_FUNC_DEF_RE.finditer(code):
            name = next((g for g in m.groups() if g), None)
            if name:
                defined.add(name)
        for m in JS_CALL_RE.finditer(code):
            name = m.group(1)
            if name in JS_BUILTINS:
                continue
            line = code[:m.start()].count('\n') + 1
            called.setdefault(name, []).append((path, line))
    missing = [(n, l) for n, l in called.items() if n not in defined]
    if missing:
        warn_("Called but never defined functions:")
        for name, locs in sorted(missing):
            print(f"     - {name}()")
            for path, line in locs[:3]:
                print(f"       at {os.path.relpath(path, ASSETS_ROOT)}:{line}")
            if len(locs) > 3:
                print(f"       ... +{len(locs)-3} others")
    else:
        ok("All called functions are defined")