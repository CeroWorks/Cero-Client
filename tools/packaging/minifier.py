import os
import re
import json
import tempfile
import subprocess
from config import RESERVED_NAMES
from logger import warn_, ok, info

def _run(cmd_list, env=None):
    try:
        return subprocess.run(cmd_list, capture_output=True, text=True, shell=False, env=env)
    except FileNotFoundError:
        return subprocess.CompletedProcess(cmd_list, returncode=127, stdout="", stderr="Command not found")

def _tool_available(tool):
    return _run([tool, "--version"]).returncode == 0

def obfuscate_js(dist_root):
    try:
        npm_root = _run(["npm", "root", "-g"]).stdout.strip()
    except Exception:
        npm_root = ""
    env = os.environ.copy()
    if npm_root:
        env["NODE_PATH"] = npm_root

    if _run(["node", "-e", "require('terser')"], env=env).returncode != 0:
        warn_(f"terser not found (NODE_PATH={npm_root}) - skipping JS obfuscation")
        return

    js_files = []
    for root, _, files in os.walk(dist_root):
        for f in files:
            if f.endswith(".js"):
                js_files.append(os.path.join(root, f).replace("\\", "/"))
    if not js_files:
        info("No .js files to process")
        return

    script = r"""
const fs = require('fs'), { minify } = require('terser');
const files    = JSON.parse(process.argv[2]);
const reserved = JSON.parse(process.argv[3]);
(async () => {
  let fail = 0;
  for (const file of files) {
    try {
      const code   = fs.readFileSync(file, 'utf8');
      const result = await minify(code, {
        compress: { passes: 3, drop_debugger: true, booleans_as_integers: true },
        mangle:   { reserved, properties: { regex: /^_/, reserved } },
        format:   { ascii_only: true, comments: false },
      });
      if (result.error) throw result.error;
      fs.writeFileSync(file, result.code, 'utf8');
      console.log('OK ' + file);
    } catch(e) {
      console.log('ERR ' + file + ' :: ' + (e.message || e));
      fail++;
    }
  }
  process.exit(fail > 0 ? 1 : 0);
})();
"""
    with tempfile.NamedTemporaryFile("w", suffix=".js", delete=False, encoding="utf-8") as tf:
        tf.write(script)
        script_path = tf.name
    try:
        r = _run(["node", script_path,
                  json.dumps(js_files), json.dumps(RESERVED_NAMES)], env=env)
    finally:
        try: os.remove(script_path)
        except OSError: pass

    ok_count = 0
    for line in (r.stdout or "").splitlines():
        if line.startswith("OK "):
            ok_count += 1
        elif line.startswith("ERR "):
            warn_(line[4:])
    if r.returncode != 0 and not (r.stdout or "").strip():
        warn_(f"Runner failed: {r.stderr.strip()[:300]}")
    ok(f"{ok_count}/{len(js_files)} JS files obfuscated")

def minify_css(dist_root):
    if not _tool_available("cleancss"):
        warn_("cleancss missing — skipping CSS minification")
        return
    count = 0
    for root, _, files in os.walk(dist_root):
        for f in files:
            if not f.endswith(".css"):
                continue
            path = os.path.join(root, f)
            r = _run(["cleancss", "-O2", "-o", path, path])
            if r.returncode != 0:
                warn_(f"CSS minify error: {r.stderr.strip()}")
            else:
                count += 1
    ok(f"{count} CSS files minified")

def minify_html(dist_root):
    if not _tool_available("html-minifier-terser"):
        warn_("html-minifier-terser missing — skipping HTML minification")
        return
    js_opts = json.dumps({
        "compress": True,
        "mangle":   {"reserved": RESERVED_NAMES},
        "format":   {"comments": False},
    })
    count = 0
    for root, _, files in os.walk(dist_root):
        for f in files:
            if not f.endswith(".html"):
                continue
            path = os.path.join(root, f)
            cmd = [
                "html-minifier-terser", path,
                "--collapse-whitespace", "--remove-comments",
                "--remove-redundant-attributes",
                "--remove-script-type-attributes",
                "--remove-style-link-type-attributes",
                "--minify-css", "true",
                "--minify-js", js_opts,
                "-o", path,
            ]
            r = _run(cmd)
            if r.returncode != 0:
                err = (r.stderr or r.stdout).strip().splitlines()
                warn_(f"HTML minify error: {err[-1] if err else 'unknown'}")
            else:
                count += 1
    ok(f"{count} HTML files minified")