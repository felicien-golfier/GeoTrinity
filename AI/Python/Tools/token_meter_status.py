"""Status line: caches Claude Code's status JSON (it carries the plan rate-limit %) for token_meter.py.

Prints a compact "five_hour x% (resets Mon 14:30) · seven_day y% (resets Wed 09:00)" line. Raw JSON goes to AI/Output/token_meter/status.json; % changes are logged to pct_samples.jsonl.
"""
import json, os, sys, time
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

def log_samples(pcts, d):
    """Append (time, window, %) to pct_samples.jsonl whenever a window's % changes: the series to fit tokens -> % against (all transcripts, by timestamp)."""
    path = os.path.join(OUT, "pct_samples.jsonl")
    try:
        last = json.loads(open(path, encoding="utf8").readlines()[-1])
    except (OSError, IndexError, ValueError):
        last = {}
    row = {"t": round(time.time(), 1), "pct": {k.split(".")[-1]: v for k, v in pcts.items()},
           "win": {k: v.get("resets_at") for k, v in d.get("rate_limits", {}).items() if isinstance(v, dict)}}
    if (row["pct"], row["win"]) != (last.get("pct"), last.get("win")):
        with open(path, "a", encoding="utf8") as f:
            f.write(json.dumps(row) + "\n")

def main():
    try:
        d = json.load(sys.stdin)
    except ValueError:
        return
    os.makedirs(OUT, exist_ok=True)
    pcts = dict(find_pct(d.get("rate_limits", {})))
    with open(os.path.join(OUT, "status.json"), "w", encoding="utf8") as f:
        json.dump({"rate_pcts": pcts, "resets": {k: v.get("resets_at") for k, v in d.get("rate_limits", {}).items() if isinstance(v, dict)},
                   "raw": d}, f)
    log_samples(pcts, d)
    try:
        mode = open(os.path.join(OUT, "..", "mode.txt"), encoding="utf8").read().strip() or "normal"
    except OSError:
        mode = "normal"
    print(" · ".join([mode] + [f"{k.split('.')[-1]} {v:.0f}%{reset_label(d, k)}" for k, v in pcts.items()]))

if __name__ == "__main__":
    main()
