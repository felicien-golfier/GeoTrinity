# Video Editing by Prompt

Turning a request — "make three shorts from this stream", "cut this to ten minutes", "a montage on this track" —
into a finished video. Read `CLAUDE.md` in this folder first for the setup and the tools.

## The loop

1. Pin the target: format, length, tone, sources. Ask only what the prompt leaves open and a default cannot fill.
2. Scan every source (`video_scan.py`); read `timeline.txt`, then the sheets it points to.
3. Shortlist moments and show them as a cut list — timecode, length, one line on why — before any long render.
4. Write the edit list and render it with `--draft`.
5. Scan the draft with a small `--every` and read its sheets: framing, text, captions, cut points, the ending.
6. Fix, then render the final without `--draft`; report its path, length and size.

- One edit list per output, named for it (`short_01.json`, `recap.json`); a new version is a new output name.
- A user comment changes the edit list, never a render by hand.

## Output targets

| Target | Size | Length | Traits |
|---|---|---|---|
| Short / Reel / TikTok | 1080 × 1920 | 15–60 s | Hook in the first 2 s, captions always, fill or blur framing |
| Square post | 1080 × 1080 | 15–60 s | Captions, centred action |
| Long video | Source size, 16:9 | Any | Chronological, dead air cut, sections with a fade |
| Montage | Target's size | Length of the music | Cuts on the beat, music as the main sound |

- Loudness `-14` LUFS for every web target; gameplay keeps `fps` 60, everything else 30.
- Platform length limits change; check the current limit when a piece nears one.

## Finding moments in a long video

| Signal | Where | Means |
|---|---|---|
| Peak far over the median | `timeline.txt` loudest windows | Reaction, shout, laughter, explosion, crowd |
| Scene cuts clustering | `cuts` column | Action, fast camera, menus flashing |
| Words | `said` column, `words.json` | Exclamations, jokes, questions answered, named events |
| Long gap with no words and low loudness | `timeline.txt` | Dead air to cut |
| The frames | sheets | What happens: a kill, a boss phase, a fail, a win screen |

- Shortlist two to three times the moments needed, from loudness and words together, then confirm each on the
  sheets; drop any that needs earlier context to make sense.
- Span-scan each keeper to place its cuts: in on the first word's start or 1–3 s before the action, out just after
  the payoff or the reaction, never mid-word — word timings give both.
- A moment that stands alone beats a louder one that does not.

## Shorts

- Open on the payoff or the most striking frame; a 1–2 s flash of the peak before the setup is a hook.
- `fill` with `focus` on the action, read off the sheets; `blur` when the whole wide frame matters.
- Captions on every clip with speech, 2–3 words per line, size 80–100, bottom or centre.
- One top text of at most five words states the hook; it stays on for 2–3 s.
- Tighten speech by splitting a clip at every pause over about 0.4 s in `words.json`, hard cuts between the parts.
- End on the reaction or a clean beat, never on a fade to nothing.

## Long videos

- Keep the source's order; cut dead air, repeats and off-topic stretches; hard cuts inside a section, a 0.3–0.5 s
  fade between sections.
- Section starts come from the transcript's topic changes; they double as chapter times in the description.
- Every clip opens its own decoder: past a few dozen clips, render each section as its own output, then join the
  sections as clips of a final edit with hard cuts.
- Captions are optional on long videos; a burned-in title card per section helps skimming.

## Gameplay footage

- Voice chat shares the game's audio track: a showcase keeps the game sound by cutting only inside voice-free spans —
  gaps of several seconds between words in `words.json`.
- Debug panels and HUD text leak player names and dev numbers: crop them off (`crop`), in stills too.
- The recording player's own health bar (`video_bar.py`) gives their every death and respawn to the frame; the camera
  and the wipe snap show only the team's end.
- For a vertical cut, a `follow` window zooms onto the player with no bands: keys every 0.5–1 s read off gridded
  frames (a white grid every 240 × 216 source pixels), the window kept below any debug panel with names.
- Colour does not track a player: auras and hit flashes share its colour; the keys are placed by reading frames.
- Gameplay stays at 60 fps.

## Compilations of one repeated event

A run of deaths, wipes, kills or near-misses, cut faster and faster, ending on a punchline card.

- Find a visual signature the event always ends on — a snap back to the lobby, a death screen, a flash — on the
  overview sheets; one small frame every 3 s shows every occurrence at once.
- Place each occurrence exactly: scene scores around the candidate, keeping the peak, then a sheet of the frame just
  before and just after each one to confirm.
- Each clip shows what causes the event and ends just after it; lengths shrink by halves (4, 2, 1 s, then about
  0.5 s for the rest), all hard cuts (`cut`), uncaptioned (`captions: false`), with a counter in the top text.
- Past the voiced opening, clips are muted (`volume` 0) and the game's own event sound (`SourceArt/Audio/`) is placed
  on each event (`sounds`), each cut where the next one starts, the last ringing into the card.
- Past about 0.5 s the run turns into a flipbook: flips of 0.3, 0.2, then 0.1 s, each a different occurrence
  (recycled freely) picking up the event's own clock where the last flip stopped, from just before the event to
  a second or more after it. Every flip's window is centred on that occurrence's burst, so the debris stays in one
  place on screen while the footage changes under it; only occurrences with a visible burst away from the frame
  edges are used. The one event sound lands where the clock crosses the event.
- The punchline card holds 3–4 s, its lines staggered in.

## Montages

- The music is the spine: no ducking, clip volume low or zero unless a clip's own sound is the point.
- Cuts land on beats: the Audacity server's beat finder labels them (`AI/MCP/Audacity/CLAUDE.md`); clip lengths
  are whole beat counts.
- Clips of 1–3 s, the strongest last; a slowed clip (`speed` 0.5) on the biggest hit.
- Transitions sparingly — a hard cut on a beat reads stronger than a fade.

## Making content from scratch

| Need | Source |
|---|---|
| Title, section or end card | A colour clip with text in the edit list |
| Picture, logo, thumbnail, designed card | GIMP (`AI/MCP/Gimp/`), used as an image clip |
| Sound effect, sting, whoosh | The sound pipeline (`AI/SoundCreation.md`) |
| Music bed | A file the user provides, or a loop built in Audacity |
| Narration | Recorded by the user, then scanned for its captions |
| Game footage | Recorded by the user; the scan finds its moments like any other |

- A thumbnail is a frame from the sheets, re-extracted at full size, finished in GIMP.

## Reading feedback

| The user says | Change |
|---|---|
| Too slow, drags | Shorter clips, cut pauses, drop the weakest moment |
| Hook is weak | Move the best moment or its peak frame first; shorter top text |
| Can't read it | Bigger captions or text, fewer words per line, darker frame behind |
| Wrong part of the frame | `focus`, or `blur` instead of `fill` |
| Music too loud or quiet | Music `volume`; ducking on for speech |
| Feels abrupt | A short fade on that join only, or a later out point |
