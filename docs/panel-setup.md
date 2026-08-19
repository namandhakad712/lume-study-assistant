# Lume Study Assistant — Panel Setup Guide

The panel is a **Ray mini-app** (source in `source/miniapp/`) plus the visual panel
designed in the **TuyaOpen IDE → Panel** editor. This documents both so you can rebuild
or replicate the panel.

## Product / DP reference

Device product PID: `okqfzw6tkrabylcs` (bound to the "Lume" product on Tuya platform).
DPs used by the firmware and panel:

| DP ID | Code | Type | Mode | Meaning |
|-------|------|------|------|---------|
| 2 | `status` | enum | ro | `standby/charging/charge_done/playing/goto_charge/sleep/following/moving/on/off` |
| 6 | `volume_set` | value | rw | 0–100% |
| 9 | `conversational_mode` | enum | rw | `hold/key/weakup/free` |
| 101 | `mute` | bool | rw | true = muted |
| 102 | `led_switch` | bool | rw | true = LED on |
| 103 | `study_mode` | enum | rw | `general/physics/chemistry/biology/solve_jee_pyq/solve_neet_pyq/ncert/book_reading/module_question/test_mode/exam_mode` |
| 104 | `focus_timer` | value | rw | 0–180 minutes, step 5 |

Mirror: `source/miniapp/src/devices/schema.ts` (generated) and
`source/embedded/include/tuya_dp_profile.h` (generated). Regenerate with
`tuyaopen dp generate --target embedded,miniapp`.

## Visual panel (IDE Panel editor)

**Theme:** Light, English (multi-language configured). Panel version 1.0.0.

### Page layout
- **Title header**: "Lume" / "Rankify Assist" branding
- **Volume Level** — value slider bound to DP 6 (`volume_set`), default 50%
- **Conversation Mode** — enum selector bound to DP 9 (`conversational_mode`), options
  `hold / key / weakup / free`
- **Focus Timer** — value control bound to DP 104 (`focus_timer`), presets in minutes
  (e.g. 25 / 45 / 90), step 5, max 180
- **Mute** — toggle component bound to DP 101 (`mute`)

### Mute component config (toggle)
- **Interaction → Click**: Trigger function → **Mute** (DP 101), Target value = **Not**
  (toggle — sends the opposite of the current state). Tap once = mute, tap again = unmute.
- **Style Linkage** (component id `Mute`):
  - `Mute_colour_on+icon` — when Mute = on (true): colored icon + background
  - `Mute_colour_off+icon` — when Mute = off (false): gray icon + background `#ccc9c9` 100%
- Icon background color: `#ccc9c9`

> Target-value rules: **Yes** = always send `true` (can never unmute), **No** = always
> send `false`, **Not** = toggle (recommended for mute).

### LED, Study Mode, Focus Timer
- **Indicator LED** — toggle component bound to DP 102 (`led_switch`), default off.
- **Study Mode** — selector bound to DP 103 (`study_mode`), same enum options as DP list.
- **Focus Timer** — setter bound to DP 104; on change the device starts the countdown,
  plays a start chime, and reports `{"104":0}` when finished.

## Mini-app (Ray) panel source

- `source/miniapp/` — Ray panel project (TypeScript + React).
- DPs are accessed through `@ray-js/panel-sdk` (`device` model in `src/devices/`).
- Build: `npm install && npm run build` (outputs to `dist/tuya` for upload).
- Upload/publish via **TuyaOpen IDE → Panel**, or `npm run dev` for local preview.

## Publishing checklist
1. Design the page + components in the IDE Panel editor.
2. Verify each component's DP binding and style linkage in **Preview** (toggle Mute → icon
   grays out; LED on/off; Focus Timer counts down).
3. Save → **Release** a new panel version.
4. If the app shows the old panel, sync the device / re-open the device page in the app.