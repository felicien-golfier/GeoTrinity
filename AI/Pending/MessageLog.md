# MessageLog — pending

Goal: a small stack of short, timed messages in the menu space beside BACK, showing what happens in the UI (every rule); this step is the message system only.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Frame/CLAUDE.md` ("Message log"), `AI/Python/UI/CLAUDE.md`.

## Code (to implement)
- [ ] `Source/GeoTrinityUI/Public/HUD/Menu/Frame/GeoMessageSubsystem.h` + `Private/.../.cpp` — `UGameInstanceSubsystem`; `FGeoMessage { FText Text; EGeoColor Color; double PostTime; }`; `PostMessage(FText, EGeoColor)`; `GetMessages()`; `OnMessagePosted` delegate; drops the oldest early past `MaxLines`; `Clear()` called from `UGeoMenuRootWidget::NativeDestruct` / `CloseMenu`; prunes messages past `Lifetime + FadeTime`.
- [ ] `.../GeoMessageLog.h` + `.cpp` — `UGeoMessageLog : UWidget`, Slate paint (no UUserWidget tick); per-message slot / alpha from age; `SlideTime` easing; active timer that sleeps when empty; tunables per CLAUDE.md.
- [ ] `GeoMenuRootWidget.cpp` — clear the log when the menu closes.
- [ ] Pure layout function (messages + now → slot, alpha per line) kept free of Slate so it is testable.

## Scripts
- [ ] `AI/Python/UI/message_log.py` — adds `MessageLog` to `WBP_MenuFrame` bottom row (Overlay, over the padding), left inset = BACK's width, fill horizontally, bottom-aligned; top level of main and pause menus wear that frame already; sets tunables via `asset_guard.write`. Add its line to `AI/Python/UI/CLAUDE.md` order (after `menu_pages.py`).

## Editor / Blueprint (by hand or script)
- [ ] `WBP_MenuFrame` — check BACK's size (`WBP_MenuPageFrame`), set `LeftInset` to its width and `LineHeight` so `MaxLines × LineHeight` ≈ BACK height.
- [ ] Pick the small text role for `TextRole` (a theme role, not a new one).

## Tests
- [ ] `Source/GeoTrinityUI/Private/Tests/GeoMessageLogTests.cpp` (low-level, `AI/Testing.md`; needs a UI test module or the `GeoTrinityTests` module once it exists) — layout function: one message sits on the bottom line; a second appears above; when the first expires the second targets the bottom line; the 6th drops the oldest; alpha fades to 0 at `Lifetime + FadeTime`.
- [ ] Run: `Tools\Run_Tests.bat MessageLog`.

## Verify
- [ ] Build Editor target (new module class + subsystem); headless test run; layout audit `AI/Python/Runtime/pie_layout_audit.py` on a page (no mouse).
- [ ] Not wired to any rule yet: nothing posts until a later step (list of rules to message is the next feature).
