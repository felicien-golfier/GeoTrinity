# MenuGuide — pending (later)

Goal: in tutorial steps 11-12 the player opens the Gems menu (loadout, then Forge) themselves, led by a guide: the next control on the path is highlighted (Esc menu → Character → Gems → Forge tab) and nothing else can be clicked until the guide is done. A guide the player must follow, reusable for any menu path.
Docs written: none yet (workflow steps 2-3 when picked up). Referenced by `AI/Tutorial/Steps.md` (steps 11, 12) and `AI/Pending/Tutorial.md` (slice 7).

## Open design (user)
- [ ] How: highlight look, how a path is authored (list of widget names / page classes?), input lock (only the highlighted control reacts, Esc included?), what happens if the player closes the menu mid-path
- [ ] Is the ability-page pop-up of steps 3-5 kept as it is (auto open, `GeoUnlockAbilityAction`) or moved to the guide too

## Known effects on existing code
- `GeoOpenGemsPageAction` (opens the menu for the player) is replaced by the guide's action, then removed whole (code, doc row in `Encounter/Action/CLAUDE.md`, `IGeoHUDInterface::OpenGemsPage`, `AGeoHUD`, `UGeoGemsWidget::ShowPage` if unused)
- Its open bug goes with it: opening while the menu is already up re-adds the pause menu to the viewport (warning) and pushes the Gems page twice (`UGeoPauseMenuWidget::OpenOnPage`)
