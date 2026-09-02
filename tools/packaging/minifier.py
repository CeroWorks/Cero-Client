import os
import json
import tempfile
import subprocess

from config import RESERVED_NAMES
from logger import warn_, ok, info

def _find_project_root():
    current = os.path.abspath(os.path.dirname(__file__))

    while True:
        if os.path.isfile(os.path.join(current, "package.json")):
            return current

        parent = os.path.dirname(current)
        if parent == current:
            raise RuntimeError("Project root containing package.json not found")

        current = parent


PROJECT_ROOT = _find_project_root()
NODE_MODULES = os.path.join(PROJECT_ROOT, "node_modules")
NODE_BIN = os.path.join(NODE_MODULES, ".bin")

def _npm_tool(name):
    """Retourne le chemin d'un exécutable npm local."""
    suffix = ".cmd" if os.name == "nt" else ""
    return os.path.join(NODE_BIN, name + suffix)


def _node_env():
    """Environnement permettant à Node de trouver les modules locaux."""
    env = os.environ.copy()

    existing_node_path = env.get("NODE_PATH", "")
    env["NODE_PATH"] = os.pathsep.join(
        path for path in (NODE_MODULES, existing_node_path) if path
    )

    env["PATH"] = os.pathsep.join([
        NODE_BIN,
        env.get("PATH", ""),
    ])

    return env


def _run(cmd_list, env=None):
    try:
        return subprocess.run(
            cmd_list,
            capture_output=True,
            text=True,
            shell=False,
            env=env or _node_env(),
            cwd=PROJECT_ROOT,
        )
    except FileNotFoundError:
        return subprocess.CompletedProcess(
            cmd_list,
            returncode=127,
            stdout="",
            stderr="Command not found",
        )


def _tool_available(tool):
    executable = _npm_tool(tool)

    if not os.path.isfile(executable):
        return False

    return _run([executable, "--version"]).returncode == 0


def obfuscate_js(dist_root):
    env = _node_env()

    check = _run(
        ["node", "-e", "require('terser')"],
        env=env,
    )

    if check.returncode != 0:
        warn_(
            "terser not found in local node_modules — "
            "run npm install"
        )
        if check.stderr:
            warn_(check.stderr.strip().splitlines()[-1])
        return

    js_files = []

    for root, _, files in os.walk(dist_root):
        for filename in files:
            if filename.endswith(".js"):
                path = os.path.abspath(os.path.join(root, filename))
                js_files.append(path.replace("\\", "/"))

    if not js_files:
        info("No .js files to process")
        return

    script = r"""
const fs = require('fs');
const { minify } = require('terser');

const files = JSON.parse(process.argv[2]);
const reserved = JSON.parse(process.argv[3]);

(async () => {
  let fail = 0;

  for (const file of files) {
    try {
      const code = fs.readFileSync(file, 'utf8');

      const result = await minify(code, {
        compress: {
          passes: 3,
          drop_debugger: true,
          booleans_as_integers: true
        },
        mangle: {
          reserved,
          properties: {
            regex: /^_/,
            reserved
          }
        },
        format: {
          ascii_only: true,
          comments: false
        }
      });

      if (result.error) {
        throw result.error;
      }

      fs.writeFileSync(file, result.code, 'utf8');
      console.log('OK ' + file);
    } catch (e) {
      console.log('ERR ' + file + ' :: ' + (e.message || e));
      fail++;
    }
  }

  process.exit(fail > 0 ? 1 : 0);
})();
"""

    script_path = None

    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".js",
            delete=False,
            encoding="utf-8",
            dir=PROJECT_ROOT,
        ) as temporary_file:
            temporary_file.write(script)
            script_path = temporary_file.name

        result = _run([
            "node",
            script_path,
            json.dumps(js_files),
            json.dumps(RESERVED_NAMES),
        ], env=env)

    finally:
        if script_path:
            try:
                os.remove(script_path)
            except OSError:
                pass

    ok_count = 0

    for line in (result.stdout or "").splitlines():
        if line.startswith("OK "):
            ok_count += 1
        elif line.startswith("ERR "):
            warn_(line[4:])

    if result.returncode != 0 and not (result.stdout or "").strip():
        warn_(f"Runner failed: {result.stderr.strip()[:300]}")

    ok(f"{ok_count}/{len(js_files)} JS files obfuscated")


def minify_css(dist_root):
    cleancss = _npm_tool("cleancss")

    if not _tool_available("cleancss"):
        warn_("cleancss missing — run npm install")
        return

    count = 0

    for root, _, files in os.walk(dist_root):
        for filename in files:
            if not filename.endswith(".css"):
                continue

            path = os.path.abspath(os.path.join(root, filename))
            result = _run([cleancss, "-O2", "-o", path, path])

            if result.returncode != 0:
                warn_(f"CSS minify error: {result.stderr.strip()}")
            else:
                count += 1

    ok(f"{count} CSS files minified")


def minify_html(dist_root):
    html_minifier = _npm_tool("html-minifier-terser")

    if not _tool_available("html-minifier-terser"):
        warn_("html-minifier-terser missing — run npm install")
        return

    js_opts = json.dumps({
        "compress": True,
        "mangle": {
            "reserved": RESERVED_NAMES,
        },
        "format": {
            "comments": False,
        },
    })

    count = 0

    for root, _, files in os.walk(dist_root):
        for filename in files:
            if not filename.endswith(".html"):
                continue

            path = os.path.abspath(os.path.join(root, filename))

            command = [
                html_minifier,
                path,
                "--collapse-whitespace",
                "--remove-comments",
                "--remove-redundant-attributes",
                "--remove-script-type-attributes",
                "--remove-style-link-type-attributes",
                "--minify-css", "true",
                "--minify-js", js_opts,
                "-o", path,
            ]

            result = _run(command)

            if result.returncode != 0:
                errors = (result.stderr or result.stdout).strip().splitlines()
                message = errors[-1] if errors else "unknown"
                warn_(f"HTML minify error: {message}")
            else:
                count += 1

    ok(f"{count} HTML files minified")