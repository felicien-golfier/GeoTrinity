# MessageLog — pending

Goal: a small stack of short, timed messages in the menu space beside BACK, showing what happens in the UI (every rule); this step is the message system only.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Frame/CLAUDE.md` ("Message log"), `AI/Python/UI/CLAUDE.md`.
Done: C++ (`GeoMessageSubsystem`, `GeoMessageLog`, root clears on `CloseMenu` / `NativeDestruct`), `GeoMessageLogTests.cpp` (built, `GeoTrinity.MessageLog` passes), `AI/Python/UI/message_log.py` written.

## Scripts
- [ ] Run `AI/Python/UI/message_log.py` (headless, after `menu_pages.py`; report `AI/Output/message_log.txt`). It adds `MessageLog` to `WBP_MenuFrame` with `LineHeight` 11 and `LeftInset` BACK width + 16.

## Editor / Blueprint (by hand or script)
- [ ] `WBP_MenuFrame` — check BACK's real height (`WBP_MenuPageFrame`), tune `LineHeight` so `MaxLines × LineHeight` ≈ BACK height, and `LeftInset` to BACK's width.
- [ ] Confirm the `Label` text role reads well at that line height (a theme role, not a new one).

## Verify
- [ ] Layout audit `AI/Python/Runtime/pie_layout_audit.py` on a page (no mouse).
- [ ] Not wired to any rule yet: nothing posts until a later step (list of rules to message is the next feature). Delete this file once the script has run and the two editor items are checked.
