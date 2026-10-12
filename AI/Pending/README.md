# AI/Pending

One **temporary** file per feature: everything only this feature's implementation needs — one-off steps, sources, assets to fetch, values to copy, Blueprint / editor work. Lasting design rules go in the owning folder's `CLAUDE.md` instead. Created in workflow step 3 (root `CLAUDE.md`), consumed in step 4, **deleted when empty**.

## Template: `AI/Pending/<Feature>.md`
```
# <Feature> — pending

Goal: one line — the Why.
Docs written: list of CLAUDE.md files that describe it.

## Code (to implement)
- [ ] path/File.h/.cpp — what it must do (names from the CLAUDE.md)

## Scripts
- [ ] AI/Python/<Topic>/<script>.py — what it builds; how to run

## Editor / Blueprint (by hand or script)
- [ ] Asset `/Game/...` — property = value, component, node to add

## Verify
- [ ] How to see it work (test, bridge script, PIE only if asked)
```

- Concrete asset paths, property names, values; no prose.
- A step an editor script can do becomes a script row; the rest stays here for the user.
