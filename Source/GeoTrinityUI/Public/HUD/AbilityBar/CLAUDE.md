# HUD/AbilityBar

C++ data layer on `AGeoHUD`; layout is Blueprint (`WBP_AbilityBar`, `WBP_AbilitySlot`, `WBP_StatusBar`).

| File | Role |
|---|---|
| `GeoAbilityBarWidget.h` | Bottom-centre bar; `BuildBar` creates one slot per `GetAbilityBarEntries()` entry and one passive tile per `GetPassiveBarEntries()` entry (`PassiveBox`, left of `SlotBox`); refreshes badges on the deploy-count ping; rebuilds the passive tiles when the granted passive set changes |
| `GeoAbilitySlotWidget.h` | One slot: icon, cooldown sweep, countdown, value gauge + its number, deploy badge, live key label; pulls everything from the HUD each tick. Also the passive tile (`WBP_PassiveSlot`: no key / count text, `bIsPassive` entry) |
| `GeoStatusBarWidget.h` | Active-effect icon row, local player; pure C++ tree polling `GetActiveEffectIcons()` |

- `GetAbilityBarEntries()`: granted activatable abilities, passives skipped, matched to the class `UAbilityInfo` entry.
- `GetPassiveBarEntries()`: granted passives (class passives and Core passives) matched to their `UAbilityInfo` entry, icon required; granted order.
- Passive column: left of the ability slots, `UWrapBox` vertical, wrapping every 2 tiles → two tiles stacked, a new column for the 3rd, 4th… (6 passives = 3 columns). Status tiles keep the whole right side.
- Height invariant: 2 passive tiles + gap = one ability icon + its key label (tile 50, gap 6 in `AI/Python/UI/passive_bar.py`; passive tile is square). `WBP_AbilityBar`: `Row` (HorizontalBox) = `PassiveBox`, `SlotBox`; the bar's centre shifts right by half the passive width.
- Passive tile = ability slot look: icon, cooldown sweep + countdown, frame. Passives are always "active", so the in-use pin (sweep full while active) is skipped for `bIsPassive`.
- Frame lit = sweep empty AND `AGeoHUD::IsAbilityReady(Tag)` → the ability's `IsReady(ASC)` (`GeoTrinity/.../Abilities/Base/CLAUDE.md`, "HUD readiness"). Same call for slots and passive tiles; the slot never branches on ability class.
- Cooldown read through `GetAbilityCooldown` like any ability: a Core passive's cooldown is a replicated GE (`Abilities/Gem/CLAUDE.md`), never a server-only timestamp.
- A slot holds every ability sharing its InputTag; shows last active/activatable, else first (Square channel ↔ detonate).
- Sweep pinned full while active, then depletes on the cooldown; clears at once without a cooldown.
- Value gauge = the class gauge on the slot: `AGeoHUD::GetAbilityGauge(Tag, Fill, bReady, Label)` → the displayed ability's `GetClassGauge` + `GetGaugeLabel` (`Abilities/Base/CLAUDE.md`, "HUD readiness"). Same fill as the player card's ring. No answer → `GaugeMeter` and `GaugeText` collapsed.
- Two kinds of fill on one slot: time = `CooldownMeter` (radial `Sweep`, dark, depletes); value = `GaugeMeter` (`Rise`, bottom → top, class colour, `DA_Meter_SlotGauge`). Tree order: `Icon`, `GaugeMeter`, `CooldownMeter`, texts.
- Centre text: a running countdown wins; otherwise `GaugeText` = the label; no label → empty. The number is the ability's, never derived from the fill in the slot.
- Value gauge never lights the frame: frame rule unchanged (sweep empty AND `IsAbilityReady`).
- Deploy badge = `GetDeployCountForAbility` (charges, not live deployables); slot greys only at 0.
- `OnPlayerDeployCountChanged` is a tagless ping; each slot re-queries its own tag.
- Status tiles: one per active GE with an `Icon`, plus the synthetic sweet-spot gauge entry (reveals bottom-to-top, shines when full).
