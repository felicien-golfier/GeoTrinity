"""Token/usage meter. `start` (UserPromptSubmit) snapshots plan usage %; `stop` (Stop) prints the turn line.

Stop emits {"systemMessage": ...}: shown to the user only, never in the model's context. `start` prints nothing
(UserPromptSubmit stdout would enter the context). Plan % comes from token_meter_status.py's cache.
Per-session state: AI/Output/token_meter/<session_id>.json. Subagent (sidechain) tokens are not counted.
"""
import json, os, sys, time

DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Output", "token_meter")

def fmt(n):
    return f"{n/1000:.0f}k" if n >= 1000 else str(n)

def plan_pct():
    """Five-hour plan usage %, or None when the status line has not cached one."""
    try:
        p = json.load(open(os.path.join(DIR, "status.json"), encoding="utf8"))["rate_pcts"]
    except (OSError, ValueError, KeyError):
        return None
    return p.get("five_hour", next(iter(p.values()), None))

def load(sid):
    try:
        return json.load(open(os.path.join(DIR, f"{sid}.json"), encoding="utf8"))
    except (OSError, ValueError):
        return {"deltas": [], "tok": []}

def save(sid, st):
    os.makedirs(DIR, exist_ok=True)
    json.dump(st, open(os.path.join(DIR, f"{sid}.json"), "w", encoding="utf8"))

def turn_tokens(path):
    """(new_in, cached_in, out, calls) of the last prompt's turn, and the same summed over the session; dedupes streamed messages."""
    calls, idx = {}, -1
    with open(path, encoding="utf8") as f:
        for line in f:
            try:
                d = json.loads(line)
            except ValueError:
                continue
            if d.get("isSidechain"):
                continue
            m = d.get("message", {})
            if d.get("type") == "user" and not d.get("isMeta"):
                c = m.get("content")
                if isinstance(c, str) or (isinstance(c, list) and not any(
                        isinstance(b, dict) and b.get("type") == "tool_result" for b in c)):
                    idx += 1
            elif d.get("type") == "assistant" and m.get("usage") and m.get("id"):
                calls[m["id"]] = (m["usage"], idx)
    def tot(items):
        return (sum(u.get("input_tokens", 0) + u.get("cache_creation_input_tokens", 0) for u, _ in items),
                sum(u.get("cache_read_input_tokens", 0) for u, _ in items),
                sum(u.get("output_tokens", 0) for u, _ in items), len(items))
    allc = list(calls.values())
    return tot([c for c in allc if c[1] == idx]), tot(allc)

def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "stop"
    hook = json.load(sys.stdin)
    sid = hook.get("session_id", "unknown")
    st = load(sid)
    if mode == "start":
        st["start_pct"], st["start_time"] = plan_pct(), time.time()
        save(sid, st)
        return
    (tin, tc, tout, tn), (sin, sc, sout, sn) = turn_tokens(hook.get("transcript_path", ""))
    now, parts = plan_pct(), []
    parts.append(f"{time.time() - st.get('start_time', time.time()):.0f}s")
    if now is not None and st.get("start_pct") is not None:
        d = now - st["start_pct"]
        if d < 0:                      # window reset mid-turn: only what was spent since the reset is known
            d = now
        st["deltas"].append(d)
        parts.append(f"{d:+.1f}%, session {sum(st['deltas']):.1f}%")
        st["start_pct"] = now
        save(sid, st)
    parts.append(f"tokens turn {fmt(tin)}+{fmt(tc)}c/{fmt(tout)} {tn} calls, session {fmt(sin)}+{fmt(sc)}c/{fmt(sout)} {sn} calls")
    print(json.dumps({"systemMessage": " · ".join(parts)}))

if __name__ == "__main__":
    main()
