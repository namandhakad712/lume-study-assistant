# Testing & Known Issues

Bench test sessions are logged in full in [TESTING-REPORT.md](TESTING-REPORT.md)
(25+ sessions, issue classes **A–E**). Summary below.

## Verified working

| # | Item | Result |
|---|------|--------|
| E1 | Boot with fresh binary (flash dump + ELF symbolication) | OK |
| E2 | Button wake → voice AI chat | OK |
| E7 | GUI tyutool flash (COM4 @ 460800, RST during handshake) | OK |
| E8 | Agent genuinely creates cloud timers (observed, see A8) | (unwanted) |
| C9 | Cloud timer BIC fires on schedule (action lands on dp207) | OK, but rejected by device |
| C10 | App-controlled focus timer DP 104 → chime + goal-question injection | OK end-to-end |

## Issue classes

### A — Agent (cloud)

- **A8 · Cloud timer lands on DP 207**: agent-created cloud timer fired exactly on
  time but delivered `dp207` → `DP ID 207 Invalid` → device did nothing.
  **Fix applied**: strict prompt — agent never creates cloud timers.
- **A9 · Reflash silently un-binds device**: after flashing a fresh QIO image the
  device reset (`Reset id:…`, `unactive`, `GATEWAY_NOT_EXISTS`). Re-pair in app.

### B — Board / firmware

- **B9 · No auto-reset circuit**: flashing requires manual RST during handshake.
- **B10 · Timer-start chime is the Chinese greeting**: focus-timer start plays
  `AI_AUDIO_ALERT_POWER_ON` ("我在这里，我们一起玩吧") — confusing; a dedicated
  start chime is pending.
- **B11 · Agent hallucinates device state**: claims about mute/LED/volume are not
  grounded — mitigated by the honesty rules in the prompt.
- **B12 · Boots in `hold` mode**: KV stores `chat_mode:0` → device boots HOLD
  despite code default FREE; persists only after app sets DP 9 = free.
- **B13 · Boot log replays previous crash**: stale log buffer on cold boot.

### C — Cloud / platform

- **C7 · Port roles unstable**: COM3/COM4 swap between flash/log roles.
- **C8 · MQTT `Cmd Parse Fail:-25344`**: transient command parse failure during
  agent chat; non-fatal.
- **C9/C10** — see verified working above.

### D — Chat / audio
(no open D-class items — voice chat, knock wake, TTS verified.)

### E — Environment

- **A2 (E-side) · charmap build bug** — fixed with `PYTHONUTF8=1` (see
  [Build & Flash](build-flash.md)).

## Open items

1. Dedicated focus-timer start chime (replace B10 greeting).
2. Decide whether to register DP 207 (to make cloud timers work) or keep the
   app-only timer policy (current).
3. Quick AI Panel selection in the IDE (currently `Latest_PANEL` active).
4. Confirm advanced-function toggles on the product (CloudTiming recommended off
   or bound properly).
5. Decide cloudless behavior for mute/LED (DP 101/102) if the product goes
   cloud-free.

## How to reproduce / extend

1. Build + flash per [Build & Flash](build-flash.md).
2. Pair in Tuya app → set conversation mode `free`.
3. Test: button/knock wake → ask "set a 5 minute timer" (agent must refuse) →
   set DP 104 from the app → verify chime + goal question at end.
4. Append findings to `TESTING-REPORT.md` with the issue-class lettering scheme.