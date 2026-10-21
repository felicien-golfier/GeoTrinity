# HUD/Menu/Gems

Gems menu over `UGeoGemSubsystem` (`GeoTrinity/Public/Gem/CLAUDE.md`). Opened from the character sheet.

| File | Role |
|---|---|
| `GeoGemsWidget.h` | Menu: `PageSwitcher` (`LoadoutPage`, `ForgePage`), page and class tabs, shard count |
| `GeoGemPageWidget.h` | Base of both pages; the one way they edit the profile: `CommitChanges` (save + server + menu refresh) |
| `GeoGemLoadoutWidget.h` | Socket board: clusters of tier rings around a centre |
| `GeoGemForgeWidget.h` | Forge: tiers (select, break down free copies → shards), gem types with owned / free counts, shards → gems |
| `GeoGemSocketButton.h` | Bare button drawn by its `UGeoGemGlyph`; says which socket it is on click |

- Shard count (header, `ShardsFrame` key cap): `ShardIcon` = `DA_Icon_Gems_Shard`, the three-facet pink crystal of the design, 24 px, then the number and `SHARDS`.
- Glyph drawing and colours: `Style/Glyph/`; gem polygons and highlight from theme `GemStyle`.

## Unequip and tier filter (Loadout page)
- No unequip in the gem detail, none per socket: a socket changes by replacement.
- `UnequipTierButton`, at the bottom of the stacks column (`UnequipTierRow` in `InventoryColumn`), follows the tier filter: tab CUT → `UNEQUIP ALL CUTS` (`UnequipTierFormat`, {0} the plural tier name) → `UGeoGemProfileSave::UnequipTier` for that tier, echo included; tab ALL → `UNEQUIP ALL` (`UnequipAllText`), every tier. Disabled when the build holds nothing of it. Through `CommitChanges` like every edit; clears the pick.
- Tier filter tabs read the tier name only (ALL, CHIP, CUT, PRISM): the owned count is not shown (`FilterFormat` gone).

## Echo socket (Loadout page)
Rules: `GeoTrinity/Public/Gem/CLAUDE.md` "Echo socket". The page only shows and calls `UGeoGemProfileSave`.
- One `UGeoGemSocketButton` between the clusters (`EchoPosition` = centre, `EchoSize` tunables), index `GetEchoIndex()` = one past `GeoGem::GetSockets()`, so picks / hovers use the same socket index; drawn as the echoed gem, or empty / locked as the echo glyph (`Style/Glyph/`: the Chip, Cut and Prism outlines nested, never a Core look; locked adds the unlock level).
- Picking it opens its detail: gem, the value it gives, `ECHO LEVEL n · xp / cost XP` (text, no bar; at the max level just `ECHO LEVEL n`), no button. No upgrade button: levels come from XP.
- Endless look: the echo level is never shown with a max (`EchoMaxLevel` stays hidden, nor the XP kept past it), no pips, no "MAX" state; players read it as infinite.
- Class level box: once the shared echo level is 1 or more (any class shown, even under 20), an `ECHO LEVEL` block under the class level shows it as a bare number in violet, almost as big as the class level; the character sheet's XP meter shows echo XP only for a class at level 20.
- Echo socket look (design: Gems Loadout (Echo) artboard): the board's focal point, far larger than any socket. A `UGeoEchoHalo` (`Style/Glyph/`) built with the board sits behind the echo button (`EchoHaloSize` tunable, about 3x `EchoSize`; `EchoHaloStyle` = the frame style it wears, `EchoHaloOpenColor`): dark disc, ring, a rosace of outline squares turned a step apart, a second smaller one turning the other way, small squares running the ring like frame runners. Colour: the echoed gem's, violet while open and empty, grey while locked.
- Halo states: idle = slow turn, calm glow; a stack that can echo is picked, or the socket is picked / hovered = faster turn, brighter glow, runners faster (the frame style's active blend); locked = still, dim, no runners. Opening the socket (shared echo level 0 → 1) is a one-off flare (active blend held for a moment, `PlayFlare`), not a loop; shown once per session.
- The halo never takes input (HitTestInvisible); the echo button on top stays the click target. The other sockets and the cluster rings are untouched.
- No echo button: the click on the echo socket is the action (stack picked, then socket; or the reverse). The gem detail has two buttons only, EQUIP (the first empty socket the picked stack fits) and ALL (every socket it fits, replaces what is slotted); both always shown, hovering a stack or a socket included, and act on the picked stack (off with none). ALL wears the theme's normal button text role, the room the echo button took goes to it.
- One click echoes, even a type the build slots: those sockets empty at once (no confirmation step). Off for a Core / locked / no copy / already echoed (the note says why).
- Slotting the echoed type into a socket is allowed: the echo empties, the socket fills. A Core already in another socket moves: the previous socket empties. Neither has a refusal note any more (`NoteCoreOnce` gone).
- Stack tile of the echoed type: "ECHOED" mark, not "n FREE"; its sockets glow like any pickable socket.
- Totals table: a row for the echoed stat after the slotted ones, its count cell reads `ECHO` (`ECH` when the column is too narrow for it), where a slotted stat shows `×n`; the level is in the detail.
- A stack picked, then the echo socket (or the reverse), echoes it, like a socket.

## Messages
Every refusal and every side effect the player did not ask for shows in the message strip (`Frame/CLAUDE.md`, "Message log"); texts are `FText` tunables of the widget (`GeoGem|Messages`), formats with the gem / tier / count. Clicks only, never hover. Disabled buttons post nothing (their note says why).
- Refusal (`Damage`), from `EGeoGemRefusal` on a socket or stack click: already in this socket, wrong tier, socket locked (level), no free copy; echo: not unlocked (level 20), a Core, already the echo, no copy. Clicking the locked echo socket with nothing picked says when it opens.
- Side effect (`Neutral`): a Core moved from its previous socket; echoing emptied n sockets of that gem; slotting the echoed type cleared the echo.
- Success (`Heal`): ALL filled n sockets; unequip emptied n; build created / deleted; build limit and last-build refusals (`Damage`).
- Forge: broken / crafted lines (`Heal`).
- Key bindings page: a rejected key (wrong device, or a conflict) says so. Server browser: a failed search says so.
