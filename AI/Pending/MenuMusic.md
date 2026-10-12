# MenuMusic — pending

Goal: play "hang inspiration.mp3" (Downloads) in a loop in the main menu.
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/CLAUDE.md` ("Menu music").

## Code (to implement)
- [x] `Source/GeoTrinityUI/Public/HUD/Menu/GeoMainMenuWidget.h` — `UPROPERTY(EditDefaultsOnly, Category = "Music") TObjectPtr<USoundBase> MusicLoop;` + `TObjectPtr<UAudioComponent> MusicComponent;` (JavaDoc, update class comment)
- [x] `Private/HUD/Menu/GeoMainMenuWidget.cpp` — `NativeConstruct`: if `MusicLoop`, `UGameplayStatics::CreateSound2D(this, MusicLoop, 1.f, 1.f, 0.f, nullptr, false, false)` then `FadeIn(1.f, 1.f)`; `NativeDestruct`: `FadeOut(0.5f, 0.f)` and null it

## Scripts
- [x] `AI/Python/Audio/menu_music_loop.py` (done: 65.6 s, 113 BPM, 31 bars, 1.8–67.4 s; seam picture `AI/Output/menu_music_loop_seam.png`) (`uv run`, numpy; 71.6 s, 44.1 kHz stereo source): decode the mp3 with ffmpeg, cut a seamless loop, write `SourceArt/Audio/MenuMusic/SFX_MenuInspiration_Loop.wav` (16-bit PCM, peak −1 dBFS per `AI/SoundCreation.md`). No Audacity.
  - Trim leading / trailing silence and the mp3 encoder delay (~1100 samples).
  - Pick start `S` and end `E` on zero crossings, on the same beat phase: search `E` near the end and `S` near the start maximising normalised cross-correlation of the 100 ms before `E` with the 100 ms before `S` (plus tempo from autocorrelation to land on whole bars).
  - Wrap-around click removal: equal-power crossfade of the tail `[E-F, E]` into the head `[S-F, S]` (F ≈ 1–2 s, baked in so the file loops with no fade at its edges, per the `_Loop` rule).
  - Check: the jump `E → S` has a sample step below the signal's own RMS step; render a 2-pass concatenation and show the seam picture in `AI/Output/`.
  - If no good pair exists, report the best seam and its score; the user decides.
- [x] `AI/Python/Asset/import_sound_waves.py` FOLDERS row added
- [ ] Run `import_sound_waves.py` via mcp-unreal (editor closed rules: `AI/MCP/CLAUDE.md`). `_Loop` suffix marks it looping.

## Editor / Blueprint (by hand or script)
- [ ] `/Game/Art/SFX/Music/SW_MenuInspiration_Loop` — Sound Class = `/Game/Art/SFX/Mix/SC_Music`
- [ ] Main menu widget BP (the `UGeoMainMenuWidget` subclass) — `MusicLoop` = `SW_MenuInspiration_Loop`

## Verify
- [ ] Build (`GEO_NO_PAUSE=1`, editor closed); user listens in the lobby: loops without a gap, Music slider changes its volume
