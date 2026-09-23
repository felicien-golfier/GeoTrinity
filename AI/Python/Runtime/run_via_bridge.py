"""Runs an editor script file over the editor bridge's HTTP route, for when the mcp-unreal client is down.

Host-side: run with the engine's bundled Python, not inside the editor.
    python.exe AI/Python/Runtime/run_via_bridge.py <script.py> [port]
The script runs as __main__ in the open editor. The route reports success even when the script raises, so the
source is wrapped in a handler that writes OK or the traceback to Saved/bridge_run_result.txt, printed here after.
"""
import json
import os
import sys
import urllib.request

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
RESULT = os.path.join(PROJECT_DIR, "Saved", "bridge_run_result.txt").replace("\\", "/")


def run(script_path, port=8090, timeout=600):
    """Execute script_path in the editor and return what it reported: OK, or its traceback."""
    # The editor's working directory is not the project, so the path handed over is absolute.
    script_path = os.path.abspath(script_path).replace("\\", "/")
    wrapper = "\n".join([
        "import traceback",
        "try:",
        f"    exec(compile(open(r'{script_path}', encoding='utf-8').read(), r'{script_path}', 'exec'),"
        " {'__name__': '__main__'})",
        f"    open(r'{RESULT}', 'w').write('OK')",
        "except Exception:",
        f"    open(r'{RESULT}', 'w').write(traceback.format_exc())",
    ])
    if os.path.exists(RESULT):
        os.remove(RESULT)

    request = urllib.request.Request(f"http://127.0.0.1:{port}/api/editor/execute_script",
                                     data=json.dumps({"script": wrapper}).encode("utf-8"),
                                     headers={"Content-Type": "application/json"})
    urllib.request.urlopen(request, timeout=timeout).read()
    if not os.path.exists(RESULT):
        return "NO RESULT: the editor did not finish the script (crashed, stopped on a breakpoint, or still running)"

    with open(RESULT) as handle:
        return handle.read()


if __name__ == "__main__":
    print(run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 8090))
