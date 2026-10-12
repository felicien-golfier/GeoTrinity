"""Status line: caches Claude Code's status JSON (it carries the plan rate-limit %) for token_meter.py.

Prints a compact "five_hour x% (resets Mon 14:30) · seven_day y% (resets Wed 09:00)" line. Raw JSON goes to AI/Output/token_meter/status.json.
"""
import json, os, sys
from datetime import datetime

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Output", "token_meter")

def find_pct(d, path=""):
    """Yield (path, value) for every *used_percentage* key, whatever the schema nests."""
    if isinstance(d, dict):
        for k, v in d.items():
            p = f"{path}.{k}" if path else k
            if "used_percentage" in k and isinstance(v, (int, float)):
                yield path or k, v
            else:
                yield from find_pct(v, p)

def reset_label(d, window):
    """" (resets Wed 09:00)" for a rate-limit window, empty when the status JSON has no reset time."""
    resets_at = d.get("rate_limits", {}).get(window, {}).get("resets_at")
    if resets_at is None:
        return ""
    return " (resets " + datetime.fromtimestamp(resets_at).strftime("%a %H:%M") + ")"

def main():
    try:
        d = json.load(sys.stdin)
    except ValueError:
        return
    os.makedirs(OUT, exist_ok=True)
    pcts = dict(find_pct(d.get("rate_limits", {})))
    with open(os.path.join(OUT, "status.json"), "w", encoding="utf8") as f:
        json.dump({"rate_pcts": pcts, "raw": d}, f)
    print(" · ".join(f"{k.split('.')[-1]} {v:.0f}%{reset_label(d, k)}" for k, v in pcts.items()) or "usage n/a")

if __name__ == "__main__":
    main()
