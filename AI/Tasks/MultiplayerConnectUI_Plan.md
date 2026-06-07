# Multiplayer Connection UI — Direct IP (Phase 7, step 1)

> **Status:** APPROVED, not yet implemented. Self-contained execution doc for a future session.
> Read this top-to-bottom before starting. Mandatory pre-reads are listed in §0.

---

## Context / Why

The user wants to play GeoTrinity with friends. The netcode already works (replicated GAS,
server-authoritative projectiles — see `AI/Architecture.md` and session notes dated
2026-02-23 in `AI/Tasks/IMPLEMENTATION_PLAN.md`), but there is **no way to connect from
inside the game**: launching drops you straight into `DraftMap` as a standalone session.

Phase 7 of `AI/Tasks/IMPLEMENTATION_PLAN.md` (lines ~245-263) planned an **EOS join-by-code**
menu, but every task there is unchecked and EOS needs external account setup on
dev.epicgames.com. This doc is the **smaller first step**: a main-menu screen with **Host**
and **Join** buttons using **direct-IP** connection, so the user can play *now*.

### Decisions already locked (do NOT re-litigate)
- **Transport:** direct IP now. Keep the connect logic behind a subsystem seam so an EOS
  backend can replace it later **without rewriting the UI**.
- **Menu:** a new dedicated `MainMenuMap` shown on launch.
- **Game map:** Host travels into the **existing `DraftMap`** as a listen server. No new level.

### Key concept the user asked about — "do I need a Create Server button?"
No separate server process. Unreal's **listen server** = the host's client *is* the server.
So **Host = "create a server"**; it's one button that runs
`ServerTravel("/Game/Maps/DraftMap?listen")`. Friends `ClientTravel` to the host IP. A
dedicated/headless server is a separate, later concern and explicitly out of scope.

### Networking caveat (tell the user, set expectations)
Direct IP works instantly on the **same LAN** (host's local IP via `ipconfig`). Over the
**internet**, the host must port-forward **UDP 7777**, or everyone uses a LAN-emulator VPN
(Tailscale / Hamachi / Radmin). Removing this friction is exactly what the later EOS step
buys — which is why the subsystem seam matters.

---

## 0. Mandatory pre-reads (per repo rules in root `CLAUDE.md`)

- `AI/CodingStyle.md` — **required before writing any C++**. Key rules that bite here:
  simplicity first; `const` by default; forward-declare in headers; no silent fallbacks
  (use `ensureMsgf` for missing-but-required, plain `if` + `UE_LOG` for legit runtime misses);
  name unused params in `.h`, suppress with `/*name*/` in `.cpp`.
- `AI/Commands.md` — build commands + copyright header (`// Copyright 2024 GeoTrinity. All Rights Reserved.` on every new file).
- `Source/GeoTrinity/Public/System/CLAUDE.md` — subsystem conventions; update it when done.
- `Source/GeoTrinity/Public/HUD/CLAUDE.md` — widget C++base+BP pattern; update when done.
- `Source/GeoTrinity/Public/GameClasses/CLAUDE.md` — game-framework classes; update when done.
- `AI/MCP/CLAUDE.md` + `AI/MCP/MCP_UI.md` + `AI/MCP/MCP_Blueprint.md` + `AI/MCP/MCP_Settings.md`
  — **required before any MCP/Python editor automation** (widget tree, BP, map, ini).
- `AI/MCP/MCP_DocStyle.md` — **required before editing any doc in `AI/MCP/`** (only if you touch those).
- Reference example for widget building: `AI/Python/charge_beam_gauge.py` and
  `Source/GeoTrinity/Private/Tool/GeoWidgetBuilderUtil.cpp`.

### Verified current state (from exploration, 2026-06-07)
- `GeoTrinity.Build.cs`: OnlineSubsystem is commented out (line ~53). No `Sockets` dep yet.
  Editor-only deps already include `UnrealEd`, `Blutility`, `UMGEditor`, `PropertyBindingUtils`.
- `Config/DefaultEngine.ini` `[/Script/EngineSettings.GameMapsSettings]`:
  `GameDefaultMap = /Game/Maps/DraftMap.DraftMap`, `EditorStartupMap = /Game/Maps/DraftMap.DraftMap`,
  `GameInstanceClass = /Script/GeoTrinity.GeoGameInstance`. No `GlobalDefaultGameMode`, no EOS sections.
- Maps present: `Content/Maps/DraftMap.umap`, `Content/Maps/DevMap.umap`. **No MainMenuMap.**
- `AGeoGameMode` (`Public/GameClasses/GeoGameMode.h`): `HandleStartingNewPlayer` → `RestartPlayer`
  if not spectator; `Logout` → `GeoGameState->NotifyPlayerDiedInFight` if match in progress;
  `ReadyToStartMatch` returns false (code-driven start). No PostLogin/ChoosePlayerStart. The
  gameplay GameMode BP is `Content/Game/BP_GeoGameMode.uasset`. **No menu GameMode exists.**
- `UGeoGameInstance` (`Public/GameClasses/GeoGameInstance.h`): `Init()` sets team attitude
  solver. Persists across travel — correct host for a `UGameInstanceSubsystem`.
- Subsystem pattern to mirror: `Public/System/GeoActorPoolingSubsystem.h`,
  `Public/System/GeoCombatStatsSubsystem.h` (these are *World* subsystems; ours is a
  *GameInstance* subsystem because it must survive level travel).
- Widget pattern: C++ base + BP subclass. Base `UGeoUserWidget`
  (`Public/HUD/GeoUserWidget.h`) exposes `InitFromHUD` / `BindCallbacksFromHUD`. Widgets are
  created via `CreateWidget<T>()` then `AddToViewport()` (see `GeoHUD::InitOverlay`).
- Widget-tree builder shim: `UGeoWidgetBuilderUtil` (`Public/Tool/GeoWidgetBuilderUtil.h`,
  `#if WITH_EDITOR`, `UEditorUtilityObject`). Has `BuildChargeBeamGaugeWidget`, `SetImageRoot`,
  `SetImageRootFromMaterial`, `InspectWidgetBlueprint`, private `BeginBuild`/`FinishBuild`/`LogWidget`.
- MCP fact: Python **cannot** mutate the widget designer tree — buttons/textbox require a C++
  shim `UFUNCTION`. Adding a new `UFUNCTION` needs a **full build + editor restart** before
  Python can call it.

---

## 1. C++ — `UGeoSessionSubsystem` (the connection seam)

**Files:** `Source/GeoTrinity/Public/System/GeoSessionSubsystem.h`,
`Source/GeoTrinity/Private/System/GeoSessionSubsystem.cpp`

Base class `UGameInstanceSubsystem` (include `Subsystems/GameInstanceSubsystem.h`). This is
the seam an EOS backend will later re-implement. All methods `BlueprintCallable`,
`Category = "GeoTrinity|Session"`.

### API

```cpp
/** Loads DraftMap as a listen server (host plays AND serves). Server-side travel. */
UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
void HostListenServer();

/** Travels the local player to a host at Address (IP, optional :port). */
UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Session")
void JoinByAddress(const FString& Address);

/** Best-effort local IPv4 to read out to friends; "127.0.0.1" on failure. */
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GeoTrinity|Session")
FString GetLocalIPv4() const;
```

### Implementation notes
- `HostListenServer()`:
  ```cpp
  UWorld* World = GetWorld();
  if (!ensureMsgf(World, TEXT("HostListenServer: no World"))) { return; }
  World->ServerTravel(TEXT("/Game/Maps/DraftMap?listen"));
  ```
  The `?listen` URL option is what promotes the host into a listen server. Use the map's
  **package path** (`/Game/Maps/DraftMap`), not the editor display name.
- `JoinByAddress(Address)`:
  ```cpp
  FString Trimmed = Address.TrimStartAndEnd();
  if (Trimmed.IsEmpty()) { UE_LOG(LogTemp, Warning, TEXT("JoinByAddress: empty address")); return; }
  APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
  if (!ensureMsgf(PC, TEXT("JoinByAddress: no local PlayerController"))) { return; }
  PC->ClientTravel(Trimmed, ETravelType::TRAVEL_Absolute);
  ```
  Empty address is a legit runtime miss (user clicked Join with blank field) → `if` + log, not
  an assert. Missing PC is a structural bug → `ensureMsgf`. If the user omits a port the engine
  default 7777 applies automatically — no special handling.
- `GetLocalIPv4()` const — use the socket subsystem:
  ```cpp
  ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
  if (!Sockets) { return TEXT("127.0.0.1"); }
  bool bCanBind = false;
  TSharedPtr<FInternetAddr> Addr = Sockets->GetLocalHostAddr(*GLog, bCanBind);
  return (Addr.IsValid() && Addr->IsValid()) ? Addr->ToString(false /*bAppendPort*/) : TEXT("127.0.0.1");
  ```
  Includes: `Sockets/Public/SocketSubsystem.h` (or `SocketSubsystem.h`) and
  `Sockets/Public/IPAddress.h`. **Before using these, open the engine headers to confirm exact
  signatures for the installed UE 5.7** (`GetLocalHostAddr`, `ToString(bool)`) per CodingStyle
  rule "check Epic source before using any UE named constant or method". Returning a fallback
  string here is acceptable (it's a display convenience, not required state) — that's the one
  legitimate place a fallback is fine; log nothing or a `UE_LOG` Verbose.

### Build.cs change
`Source/GeoTrinity/GeoTrinity.Build.cs`: add `"Sockets"` to `PublicDependencyModuleNames`
(needed only for `GetLocalIPv4`). **Leave OnlineSubsystem commented out** — that line is
reserved for the future EOS step. `ServerTravel`/`ClientTravel` are in `Engine` (already a dep).

---

## 2. C++ — `AGeoMainMenuGameMode` (lightweight menu mode)

**Files:** `Source/GeoTrinity/Public/GameClasses/GeoMainMenuGameMode.h`,
`Source/GeoTrinity/Private/GameClasses/GeoMainMenuGameMode.cpp`

`AGameModeBase` subclass (NOT `AGameMode` — we don't want match-state machinery). Purpose:
keep `AGeoGameMode`'s boss/match logic off the menu map.

```cpp
UCLASS()
class GEOTRINITY_API AGeoMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AGeoMainMenuGameMode(const FObjectInitializer& ObjectInitializer);
};
```
Constructor sets `DefaultPawnClass = nullptr;` (menu needs no pawn). Nothing else in C++ —
the BP subclass (§5) handles showing the widget on BeginPlay, keeping UI wiring in BP where the
project keeps it. (If you prefer pure C++, you *could* create+AddToViewport the widget in
`BeginPlay` here with a `TSubclassOf<UUserWidget> MenuWidgetClass` UPROPERTY — but the
established pattern is BP wiring, so default to the BP approach.)

---

## 3. C++ — `UGeoMenuWidget` (BP-callable Host/Join helpers)

**Files:** `Source/GeoTrinity/Public/HUD/GeoMenuWidget.h`,
`Source/GeoTrinity/Private/HUD/GeoMenuWidget.cpp`

Thin base so the BP graph stays trivial (button → call node). Matches the project's
C++base+BP convention.

```cpp
UCLASS()
class GEOTRINITY_API UGeoMenuWidget : public UGeoUserWidget
{
    GENERATED_BODY()
public:
    /** Host a listen server (Host button OnClicked). */
    UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Menu")
    void Host();

    /** Join a host at Address (Join button OnClicked, pass the IP textbox text). */
    UFUNCTION(BlueprintCallable, Category = "GeoTrinity|Menu")
    void Join(const FString& Address);

    /** Local IPv4 to display for the host to read out. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GeoTrinity|Menu")
    FString GetLocalIP() const;
};
```

Implementations fetch the subsystem and forward:
```cpp
UGeoSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UGeoSessionSubsystem>();
if (!ensureMsgf(Session, TEXT("UGeoMenuWidget: GeoSessionSubsystem missing"))) { return; }
Session->HostListenServer();   // or JoinByAddress / GetLocalIPv4
```
Get the GameInstance via `UUserWidget::GetGameInstance()` (exists on UUserWidget). Note these
are *not* trivial forwarders banned by CodingStyle — they bridge BP→subsystem with a null
guard and resolve the subsystem, so they earn their keep. (If review disagrees, the alternative
is for BP to call the subsystem directly via `GetGameInstanceSubsystem` node and drop this
class — but C++ base keeps it consistent.)

`UGeoMenuWidget` inherits `InitFromHUD`/`BindCallbacksFromHUD` from `UGeoUserWidget` but the
menu has no HUD; those go unused here (acceptable — it's the shared base).

---

## 4. C++ — `BuildMainMenuWidget` shim in `UGeoWidgetBuilderUtil`

**Files:** `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h` (+ `.cpp`), all inside the
existing `#if WITH_EDITOR`.

Add one editor-only `UFUNCTION` that builds the `WBP_MainMenu` widget tree (Python can't).
Mirror the structure of `BuildChargeBeamGaugeWidget` — call `BeginBuild`, construct widgets via
`WidgetTree->ConstructWidget<T>(T::StaticClass(), TEXT("Name"))`, slot them, call `FinishBuild`.

```cpp
/**
 * Builds the WBP_MainMenu tree:
 *   Root: CanvasPanel (or VerticalBox centered)
 *     - TitleText      : TextBlock  "GeoTrinity"
 *     - HostButton      : Button   (label child TextBlock "Host")
 *     - IPInput         : EditableTextBox  (hint "Host IP")
 *     - JoinButton      : Button   (label child TextBlock "Join")
 *     - LocalIPText     : TextBlock  (host reads this out; bound at runtime to GetLocalIP)
 * Saves the asset after building.
 */
UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
static void BuildMainMenuWidget(UWidgetBlueprint* WidgetBlueprint);
```

**Critical naming rule (from `MCP_UI.md`):** the names passed to `ConstructWidget` become the
`BindWidget` variable names the BP exposes. Use exactly `HostButton`, `JoinButton`, `IPInput`,
`LocalIPText` so the BP graph (§5) can reference them. `UButton`'s label is a child `UTextBlock`
set as its content (`Button->SetContent(...)` / add as child). Includes needed (already used in
the file or add): `"Components/Button.h"`, `"Components/TextBlock.h"`,
`"Components/EditableTextBox.h"`, `"Components/VerticalBox.h"` or `"Components/CanvasPanel.h"`,
plus the existing `"WidgetBlueprint.h"`, `"Blueprint/WidgetTree.h"`, `"Kismet2/KismetEditorUtilities.h"`.
A centered `VerticalBox` root is simplest; if using CanvasPanel follow the fixed-position slot
note in `MCP_UI.md`. **Confirm UButton/UEditableTextBox API against the UE 5.7 headers** before coding.

---

## 5. Blueprint / map / ini assets (MCP Python + editor)

Editor must be open. Follow `AI/MCP/CLAUDE.md`: Python `execute_script` for asset creation &
CDO; the C++ shim (§4) for the widget tree. Put the reusable script at
**`AI/Python/main_menu_widget.py`** and reference it by path (never paste full scripts into docs).

1. **Create `WBP_MainMenu`** under `/Game/HUD/Menu/`, parent class `UGeoMenuWidget`
   (use `WidgetBlueprintFactory` with `parent_class`, then `AssetTools` — see
   `AI/Python/charge_beam_gauge.py`; do NOT set `use_inherited_viewport_size`).
2. **Call `BuildMainMenuWidget`** on the shim CDO from Python to build the tree (only works
   after the §1-4 build + editor restart).
3. **BP graph wiring** (in editor or via the BP graph — minimal):
   - `HostButton.OnClicked` → `self.Host()`.
   - `JoinButton.OnClicked` → `self.Join(IPInput.GetText() → ToString)`.
   - `Event Construct` → set `LocalIPText` to `"Your IP: " + GetLocalIP()` and
     `SetInputModeUIOnly` + show mouse cursor (or do cursor in the menu GameMode/PC).
4. **Create `MainMenuMap`** under `/Game/Maps/` — near-empty level (just a camera/light is fine).
   Set World Settings **GameMode Override** = `BP_MainMenuGameMode`.
5. **Create `BP_MainMenuGameMode`** under `/Game/Game/`, parent `AGeoMainMenuGameMode`. On
   `BeginPlay`: `CreateWidget(WBP_MainMenu)` → `AddToViewport`, set input mode UI-only, show
   cursor. (Alternatively expose a `MenuWidgetClass` UPROPERTY in §2 and do this in C++.)
6. **ini change** — `Config/DefaultEngine.ini` `[/Script/EngineSettings.GameMapsSettings]`:
   - `GameDefaultMap = /Game/Maps/MainMenuMap.MainMenuMap` (launch into the menu).
   - **Keep** `EditorStartupMap = /Game/Maps/DraftMap.DraftMap` (editor work still opens the arena).
   - `DraftMap` keeps its `BP_GeoGameMode` override untouched — host's `ServerTravel` into it loads
     the real gameplay mode unchanged. Use `MCP_Settings.md` for the persist-config pattern, or
     edit the ini directly (it's a plain text edit, no asset).

---

## 6. Build & order of operations

1. Write §1-4 C++ (Build.cs + 3 new class pairs + shim function).
2. **Full build** via `AI/Commands.md` Bash build:
   `"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" GeoTrinity Win64 Development "C:\GeoTrinity\GeoTrinity.uproject"`
   (Live Coding is NOT enough — new `UFUNCTION`s/classes need a full build.) Editor must be closed for the build.
3. **Restart the editor** so the new shim `UFUNCTION` is callable from Python/MCP.
4. Do §5 asset work (Python + editor).
5. Update docs: `System/CLAUDE.md` (add `GeoSessionSubsystem`),
   `HUD/CLAUDE.md` (add `GeoMenuWidget`), `GameClasses/CLAUDE.md` (add `GeoMainMenuGameMode`),
   `Tool/CLAUDE.md` (add `BuildMainMenuWidget` to the `GeoWidgetBuilderUtil` list). Check the
   relevant Phase 7 boxes in `AI/Tasks/IMPLEMENTATION_PLAN.md` (subsystem, menu mode, ini,
   MainMenuMap, BP menu mode, BP menu widget) — note these are the **direct-IP** equivalents;
   leave the EOS-specific rows unchecked.

---

## 7. Files summary

| File | Change |
|---|---|
| `Source/GeoTrinity/Public/System/GeoSessionSubsystem.h` / `Private/System/GeoSessionSubsystem.cpp` | **New** — Host/Join/GetLocalIPv4 seam |
| `Source/GeoTrinity/Public/GameClasses/GeoMainMenuGameMode.h` / `Private/GameClasses/GeoMainMenuGameMode.cpp` | **New** — lightweight menu mode |
| `Source/GeoTrinity/Public/HUD/GeoMenuWidget.h` / `Private/HUD/GeoMenuWidget.cpp` | **New** — BP-callable Host/Join helpers |
| `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h` / `Private/Tool/GeoWidgetBuilderUtil.cpp` | Add `BuildMainMenuWidget` shim (editor-only) |
| `Source/GeoTrinity/GeoTrinity.Build.cs` | Add `"Sockets"` dep |
| `Config/DefaultEngine.ini` | `GameDefaultMap` → MainMenuMap |
| `Content/Maps/MainMenuMap.umap` | **New** (MCP/editor) |
| `Content/HUD/Menu/WBP_MainMenu.uasset` | **New** (MCP/editor) |
| `Content/Game/BP_MainMenuGameMode.uasset` | **New** (MCP/editor) |
| `AI/Python/main_menu_widget.py` | **New** build script |
| `System/CLAUDE.md`, `HUD/CLAUDE.md`, `GameClasses/CLAUDE.md`, `Tool/CLAUDE.md` | Doc updates |
| `AI/Tasks/IMPLEMENTATION_PLAN.md` | Tick direct-IP Phase 7 rows |

---

## 8. Verification (end-to-end)

1. Build succeeds (`Build.bat ... Development`).
2. Launch editor → PIE with **Net Mode = Play Standalone**, **1 player**: `WBP_MainMenu` shows
   with title, Host button, IP field, Join button, and "Your IP: <addr>" text populated.
3. **Two-client LAN test** (PIE "Play As Listen Server" w/ 2 players, or two packaged/standalone
   instances):
   - Instance A clicks **Host** → travels into `DraftMap`, player spawns, boss arena loads.
   - Instance B types A's LAN IP → **Join** → travels in, spawns as a 2nd player;
     `AGeoGameMode::HandleStartingNewPlayer` assigns its class by join order.
4. Both pawns present & replicated (movement + abilities fire) — exercises the existing
   server-authoritative netcode through the new connect path.
5. Disconnect B mid-fight → confirm `AGeoGameMode::Logout` → `NotifyPlayerDiedInFight` still fires.

---

## 9. Out of scope (explicitly deferred)
- EOS join-by-code, OnlineSubsystem modules, lobby codes (the later Phase 7 step — the
  subsystem seam in §1 is where that backend slots in).
- Dedicated / headless server.
- Staging-zone GameMap and late-join spawn rules.
- Lobby / player-ready screen; class-selection UI (join-order assignment stays as-is).
```
