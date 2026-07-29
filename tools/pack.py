#!/usr/bin/env python3

import os, sys, shutil, hashlib, zipfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "packaging"))

from logger import step, ok, info, warn_, fail_
import config
from utils import safe_join, check_extension, inject_version
from html_parser import parse_and_include, collect_refs
from lint import lint_js_definitions
from minifier import obfuscate_js, minify_css, minify_html

def main():
    if len(sys.argv) > 1:
        config.ASSETS_ROOT = sys.argv[1]
        config.ASSETS_ROOT_ABS = os.path.abspath(config.ASSETS_ROOT)
    if len(sys.argv) > 2:
        config.OUT_DAT = sys.argv[2]
        config.OUT_CHECKSUM = config.OUT_DAT + ".checksum"

    if not os.path.isdir(config.ASSETS_ROOT):
        fail_(f"Directory {config.ASSETS_ROOT} not found")

    info(f"Launcher version: {config.LAUNCHER_VERSION}")

    step("Analyzing JS definitions...")
    lint_js_definitions()

    step("Compiling HTML...")
    compiled_html = {}
    used_assets = set()
    for root, dirs, files in os.walk(config.ASSETS_ROOT):
        dirs.sort()
        for file in sorted(files):
            if not file.endswith(".html"):
                continue
            path = os.path.join(root, file)
            arcname = os.path.relpath(path, config.ASSETS_ROOT).replace("\\", "/")
            compiled = parse_and_include(path)
            compiled_html[arcname] = compiled.encode("utf-8")
            used_assets.add(arcname)
            used_assets |= collect_refs(compiled, os.path.dirname(os.path.abspath(path)))
            info(f"Compiled {arcname}")

    to_scan = list(used_assets)
    scanned = set()
    while to_scan:
        rel = to_scan.pop()
        if rel in scanned:
            continue
        scanned.add(rel)
        full = safe_join(config.ASSETS_ROOT_ABS, rel)
        if not full or not os.path.isfile(full):
            continue
        if not full.lower().endswith((".css", ".js", ".html")):
            continue
        if os.path.getsize(full) > config.MAX_FILE_SIZE:
            continue
        try:
            with open(full, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
        except OSError:
            continue
        for r in collect_refs(content, os.path.dirname(os.path.abspath(full))):
            if r not in used_assets:
                used_assets.add(r)
                to_scan.append(r)

    if os.path.isdir(config.DIST_ROOT):
        shutil.rmtree(config.DIST_ROOT)
    os.makedirs(config.DIST_ROOT)

    step(f"Materializing into {config.DIST_ROOT}/")
    dist_root_abs = os.path.abspath(config.DIST_ROOT)
    all_assets = set()
    for root, _, files in os.walk(config.ASSETS_ROOT):
        for f in files:
            rel = os.path.relpath(os.path.join(root, f), config.ASSETS_ROOT).replace("\\", "/")
            all_assets.add(rel)

    skipped, written = [], 0
    for arcname in sorted(all_assets):
        src_path = safe_join(config.ASSETS_ROOT_ABS, arcname)
        dst_path = safe_join(dist_root_abs, arcname)
        if not src_path or not dst_path:
            warn_(f"Path rejected: {arcname}")
            continue
        if not check_extension(arcname) and arcname not in compiled_html:
            warn_(f"Unauthorized extension: {arcname}")
            skipped.append(arcname)
            continue
        os.makedirs(os.path.dirname(dst_path) or ".", exist_ok=True)
        is_forced = arcname.startswith(config.FORCE_INCLUDE_PREFIXES)

        if arcname in compiled_html:
            with open(dst_path, "wb") as f:
                f.write(compiled_html[arcname])
            written += 1
        elif arcname in used_assets or is_forced:
            ext = os.path.splitext(arcname)[1].lower()
            if ext in (".css", ".js", ".json", ".svg"):
                try:
                    with open(src_path, "r", encoding="utf-8") as f:
                        content = f.read()
                    with open(dst_path, "w", encoding="utf-8") as f:
                        f.write(inject_version(content) if config.VERSION_TAG_REGEX.search(content) else content)
                except (OSError, UnicodeDecodeError):
                    shutil.copy2(src_path, dst_path)
            else:
                shutil.copy2(src_path, dst_path)
            written += 1
        else:
            skipped.append(arcname)

    ok(f"{written} files included")
    if skipped:
        info(f"{len(skipped)} files ignored:")
        for s in skipped:
            print(f"     - {s}")

    step("Obfuscation and minification...")
    obfuscate_js(config.DIST_ROOT)
    minify_css(config.DIST_ROOT)
    minify_html(config.DIST_ROOT)

    final_assets = []
    for root, _, files in os.walk(config.DIST_ROOT):
        for f in sorted(files):
            full = os.path.join(root, f)
            arcname = os.path.relpath(full, config.DIST_ROOT).replace("\\", "/")
            if os.path.islink(full):
                warn_(f"Symlink rejected: {arcname}")
                continue
            if arcname.startswith("/") or ".." in arcname.split("/"):
                warn_(f"Suspicious path: {arcname}")
                continue
            if not check_extension(arcname):
                warn_(f"Extension rejected: {arcname}")
                continue
            sz = os.path.getsize(full)
            if sz > config.MAX_FILE_SIZE:
                warn_(f"Too large: {arcname}")
                continue
            final_assets.append((arcname, full))

    if not final_assets:
        fail_("No assets to package")

    step(f"Building ZIP ({len(final_assets)} files)...")

    out_dir = os.path.dirname(config.OUT_DAT)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with zipfile.ZipFile(config.OUT_DAT, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for arcname, full in final_assets:
            with open(full, "rb") as f:
                data = f.read()
            zf.writestr(arcname, data)
            info(f"+ {arcname:50s} {len(data):>8} bytes")

    zip_size = os.path.getsize(config.OUT_DAT)
    if zip_size > config.MAX_BUNDLE_SIZE:
        fail_(f"ZIP too large: {zip_size}")
    ok(f"Total ZIP: {zip_size} bytes")

    h = hashlib.sha256()
    with open(config.OUT_DAT, "rb") as f:
        while chunk := f.read(1 << 20):
            h.update(chunk)
    digest = h.hexdigest()
    with open(config.OUT_CHECKSUM, "w", encoding="ascii") as f:
        f.write(digest + "\n")

    print()
    ok(f"{config.OUT_DAT:<30}  {zip_size:>10} bytes  ({len(final_assets)} ZIP entries)")
    ok(f"sha256 : {digest}")
    ok(f"checksum -> {config.OUT_CHECKSUM}")

if __name__ == "__main__":
    main()