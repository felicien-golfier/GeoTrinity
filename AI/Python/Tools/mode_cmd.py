"""UserPromptSubmit hook: `/mode [saving|normal|alone]` sets AI/Output/mode.txt locally and blocks the prompt, so the model is never called (0 tokens).

`/mode` alone shows the current mode. Any other prompt passes through untouched.
"""
import json, os, re, sys

MODES = ("saving", "normal", "alone")
FILE = os.path.join(os.path.dirname(__file__), "..", "..", "Output", "mode.txt")

def main():
    try:
        prompt = json.load(sys.stdin).get("prompt", "").strip()
    except ValueError:
        return
    m = re.fullmatch(r"/mode(?:\s+(\w+))?", prompt)
    if not m:
        return
    arg = (m.group(1) or "").lower()
    if arg in MODES:
        os.makedirs(os.path.dirname(FILE), exist_ok=True)
        with open(FILE, "w", encoding="utf8") as f:
            f.write(arg)
        msg = f"Mode set to {arg}."
    else:
        try:
            cur = open(FILE, encoding="utf8").read().strip() or "normal"
        except OSError:
            cur = "normal"
        msg = f"Mode: {cur}. Usage: /mode {'|'.join(MODES)}"
    print(json.dumps({"decision": "block", "reason": msg}))

if __name__ == "__main__":
    main()
