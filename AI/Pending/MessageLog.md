# MessageLog — pending

Goal: a small stack of short, timed messages in the menu space beside BACK, showing what happens in the UI (every rule): the system, then the Gems / Forge / key bindings / server browser rules wired to it.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Frame/CLAUDE.md` ("Message log"), `AI/Python/UI/CLAUDE.md`.
Done: C++ (`GeoMessageSubsystem`, `GeoMessageLog`, root clears on `CloseMenu` / `NativeDestruct`), `GeoMessageLogTests.cpp` (built, `GeoTrinity.MessageLog` passes), `AI/Python/UI/message_log.py` written.

## Scripts
- [x] Run `AI/Python/UI/message_log.py` (headless, after `menu_pages.py`; report `AI/Output/message_log.txt`: OK). It adds `MessageLog` to `WBP_MenuFrame` with `LineHeight` 15 and `LeftInset` BACK width + 16.

## Editor / Blueprint (by hand or script)
- [ ] `WBP_MenuFrame` — check BACK's real height (`WBP_MenuPageFrame`), tune `LineHeight` so `MaxLines × LineHeight` ≈ BACK height, and `LeftInset` to BACK's width.
- [ ] Confirm the `Label` text role reads well at that line height (a theme role, not a new one).

## Verify
- [ ] Layout audit `AI/Python/Runtime/pie_layout_audit.py` on a page (no mouse).
- [x] Rules wired in C++ (`Gems/CLAUDE.md` "Messages"), Forge `MessageText` removed (code, `gems_page.py`, `WBP_GemForge`); built, `GeoTrinity.MessageLog` and `GeoTrinity.Gems` pass.
- [ ] Not wired (gameplay-side, the strip is menu-only): class level-up, loot, echo unlock. Delete this file once the two editor items above are checked.
