# HUD/Menu/Gems

Gems menu over `UGeoGemSubsystem` (`GeoTrinity/Public/Gem/CLAUDE.md`). Opened from the character sheet.

| File | Role |
|---|---|
| `GeoGemsWidget.h` | Menu: `PageSwitcher` (`LoadoutPage`, `ForgePage`), page and class tabs, shard count |
| `GeoGemPageWidget.h` | Base of both pages; the one way they edit the profile: `CommitChanges` (save + server + menu refresh) |
| `GeoGemLoadoutWidget.h` | Socket board: clusters of tier rings around a centre |
| `GeoGemForgeWidget.h` | Forge: tiers (select, break down free copies → shards), gem types with owned / free counts, shards → gems |
| `GeoGemSocketButton.h` | Bare button drawn by its `UGeoGemGlyph`; says which socket it is on click |

- Glyph drawing and colours: `Style/Glyph/`; gem polygons and highlight from theme `GemStyle`.

## Echo socket (Loadout page)
Rules: `GeoTrinity/Public/Gem/CLAUDE.md` "Echo socket". The page only shows and calls `UGeoGemProfileSave`.
- One `UGeoGemSocketButton` between the clusters (`EchoPosition` = centre, `EchoSize` tunables), index `GetEchoIndex()` = one past `GeoGem::GetSockets()`, so picks / hovers use the same socket index; drawn as a Core socket (empty / locked shows the unlock level) or the echoed gem.
- Picking it opens its detail: gem, the value it gives, `ECHO LEVEL n · xp / cost XP` (text, no bar; at the max level just `ECHO LEVEL n`), UNEQUIP. No upgrade button: levels come from XP.
- Endless look: the echo level is never shown with a max (`EchoMaxLevel` stays hidden, nor the XP kept past it), no pips, no "MAX" state; players read it as infinite.
- Class level box: once the shared echo level is 1 or more (any class shown, even under 20), an `ECHO LEVEL` block under the class level shows it as a bare number in violet, almost as big as the class level; the character sheet's XP meter shows echo XP only for a class at level 20.
- Echo socket look (design: Gems Loadout (Echo) artboard): the board's focal point, far larger than any socket. A `UGeoEchoHalo` (`Style/Glyph/`) built with the board sits behind the echo button (`EchoHaloSize` tunable, about 3x `EchoSize`; `EchoHaloStyle` = the frame style it wears, `EchoHaloOpenColor`): dark disc, ring, a rosace of outline squares turned a step apart, a second smaller one turning the other way, small squares running the ring like frame runners. Colour: the echoed gem's, violet while open and empty, grey while locked.
- Halo states: idle = slow turn, calm glow; a stack that can echo is picked, or the socket is picked / hovered = faster turn, brighter glow, runners faster (the frame style's active blend); locked = still, dim, no runners. Opening the socket (shared echo level 0 → 1) is a one-off flare (active blend held for a moment, `PlayFlare`), not a loop; shown once per session.
- The halo never takes input (HitTestInvisible); the echo button on top stays the click target. The other sockets and the cluster rings are untouched.
- Gem detail gains `EchoButton` after UNEQUIP (built by `AI/Python/Gem/gems_page.py`; off for a Core / locked / no copy / already echoed). A type the build slots needs a second press (label ECHO IT, note says it empties sockets); any other pick or click away cancels (`bConfirmingEcho`).
- Stack tile of the echoed type: "ECHOED" mark, not "n FREE"; its sockets do not glow.
- Totals table: a row for the echoed stat after the slotted ones, its count cell `LV n` of the echo level.
- A stack picked, then the echo socket (or the reverse), echoes it, like a socket.
