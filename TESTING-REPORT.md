# TuyaOpen Preview — Tester Report (TESTING-REPORT.md)

> **Product under test**: TuyaOpen IDE + `tuya-devplat-cli` v0.0.3 + `tuyaopen` project CLI + `tos.py` + `tyutool_cli` v3.2.8 + TuyaOpen SDK (commit `306715753b462ce71e122f7ec1c70f1695e605d6`, T5AI platform)
> **Environment**: Windows 10/11, PowerShell 5.1, T5AI-Core board (COM3 flash / COM4 log, CH342 dual-serial), Wi-Fi `FTTH`, region AY (India)
> **Scope**: full project lifecycle — scaffold → configure → build → flash → license → cloud → AI chat → crash triage
> **Doc format**: one entry per problem / notice / suggestion / addition. ID prefix per category.

---

## Legend

| Tag | Meaning |
|-----|---------|
| [BUG] | Definite defect (crash, wrong behavior, broken API) |
| [UX] | Confusing or misleading behavior / documentation |
| [SUG] | Feature request / improvement suggestion |
| [NOT] | Observation worth noting, not a defect |
| [ADD] | Positive finding / works-well finding |
| Sev | Severity: 🔴 Critical / 🟠 Major / 🟡 Minor / 🔵 Info |

---

## A. Critical Bugs

### A1 [BUG] 🔴 SDK AI-chat UI init crashes with NULL display — `ENABLE_COMP_AI_DISPLAY` defaults ON
- **Tool/Area**: TuyaOpen SDK `src/ai_components`, board bring-up
- **Symptom**: Deterministic crash-loop on every boot. `MemFault`, MMFAR=0x4 (null deref), faulting thread `tuya_app_main`. Device reboots ~10 s after boot, forever.
- **Symbolicated stack** (`arm-none-eabi-addr2line -e app.elf`):
  ```
  lv_obj_get_display      lv_obj_tree.c:304   ← CRASH
  lv_obj_invalidate_area  lv_obj_pos.c:811
  lv_obj_remove_flag      lv_obj.c:160
  __ui_init               ai_ui_chat_wechat.c:162
  ai_ui_init              ai_ui_manage.c:540
  __page_chat_open        ai_ui_manage.c:143
  ai_chat_init            ai_chat_main.c:380
  app_chat_bot_init       app_chat_bot.c:73
  ```
- **Root cause**: `ai_components/ai_ui/Kconfig` declares `menuconfig ENABLE_COMP_AI_DISPLAY` with `default y`. Any app enabling `ENABLE_AI_COMPONENTS` gets LVGL chat UI compiled in by default. On boards with **no display** (T5AI-Core has audio/LED/button only), `lv_scr_act()` returns NULL and `lv_obj_remove_flag(NULL,...)` derefs NULL+4.
- **Repro**: enable AI components on any display-less board, build, flash, observe crash loop.
- **Impact**: 🔴 AI apps cannot run on display-less boards out of the box.
- **Workaround**: `CONFIG_ENABLE_COMP_AI_DISPLAY=n` in `app_default.config`.
- **Fix suggestion**: (1) default `n`; (2) runtime guard — check a display is registered before UI init; (3) Kconfig `depends on` a display component. The audio-only chat path works perfectly with UI off.

### A2 [BUG] 🟠 `bk_build_package.py` crashes on Windows with `'charmap' codec can't encode`
- **Tool/Area**: `tos.py build` packaging step (Windows)
- **Symptom**: build fails at the final packaging stage with a UnicodeEncodeError from a `charmap` codec — the tool writes log text to a Windows console pipe using the ANSI codepage.
- **Repro**: run `tos.py build` from PowerShell on a machine with a non-UTF8 console codepage (e.g. en-IN / CP1252).
- **Impact**: 🟠 Windows users cannot complete a build without knowing the workaround; error message does not hint at it.
- **Workaround**:
  ```powershell
  $env:PYTHONUTF8='1'; $env:PYTHONIOENCODING='utf-8'; tos.py build
  ```
- **Fix suggestion**: set UTF-8 mode programmatically in `bk_build_package.py` (`sys.stdout.reconfigure(encoding='utf-8', errors='replace')` or launch python with `-X utf8`), and print a hint on encode errors.

### A3 [BUG] 🟠 Toolchain auto-download fails silently (truncated + DNS error) on first platform prep
- **Tool/Area**: `tos.py` / `platform_prepare` toolchain bootstrap (`gcc-arm-none-eabi-10.3-2021.10`)
- **Symptom**: First build downloads the ARM toolchain; download truncated mid-way and/or `NameResolutionError` to the armkeil CDN. No retry, no resume, and the error does not explain how to install the toolchain manually.
- **Impact**: 🟠 New Windows/India users hit a hard stop with cryptic output.
- **Workaround**: download the exact zip manually to `platform/tools/gcc-arm-none-eabi-10.3-2021.10-win32.zip` (sha256 verified: `d287439b3090843f3f4e29c7c41f81d958a5323aecefcf705c203bfd8ae3f2e7`), then rerun build.
- **Fix suggestion**: retry/resume logic, checksum validation with a clear message, and a documented manual-download escape hatch printed in the error.

### A4 [BUG] 🟠 `tuya-devplat-cli` wrapper is a POSIX shell script — unusable from PowerShell
- **Tool/Area**: `.tuyaopen/ide/bin/tuya-devplat-cli`
- **Symptom**: Running the documented full path from PowerShell fails: `Cannot run a document in the middle of a pipeline` / can't execute a `#!/bin/sh` script directly.
- **Impact**: 🟠 AGENTS.md and IDE docs say "use the full path" — that path does not execute on Windows PowerShell, which is the documented development shell of this IDE.
- **Workaround**: `node .tuyaopen/ide/bin/devplat-cli-launcher.js <args>`.
- **Fix suggestion**: ship `tuya-devplat-cli.cmd` / `.ps1` wrappers alongside the sh script (or make the launcher a `.js` invoked via a `.cmd` shim).

### A5 [BUG] 🟡 `product detail` and `product dp-schema` use different flag names for the same input
- **Tool/Area**: `tuya-devplat-cli`
- **Symptom**: `product detail` accepts `--id` (alias `--product-id`) but **rejects** `--pid` ("Unknown flag: --pid. Did you mean --id?"). `product dp-schema` accepts `--pid` (alias `--product-id`) but rejects `--id`. AGENTS.md documents `detail --pid` — which fails.
- **Impact**: 🟡 Copy-paste from docs or between subcommands fails; each command needs its own flag memorized.
- **Fix suggestion**: accept both `--id` and `--pid` everywhere; add a top-level `--product` alias.

### A6 [BUG] 🟡 `product category-tree` cannot enumerate leaf categories — `--type` and `--keyword` both broken
- **Tool/Area**: `tuya-devplat-cli product category-tree`
- **Symptom**:
  - `--type solution` → `Category group "行业解决方案" not found in API response` (region AY)
  - `--type standard` → `Category group "标准类目" not found in API response` — same bug; the CLI hardcodes CN-market group names that the AY API doesn't return
  - Without `--type` the tree returns only top-level groups (`djd`, `xjd`, `dgzm`, `jyyl`, `qt`, …) with empty `categories: []` — leaf categories (e.g. "AI Toy" 智能玩具) are unreachable
  - `--keyword "玩具"` sends `searchKey` in the request body but the CLI performs **no client-side filtering** and returns the identical unfiltered tree
- **Impact**: 🟡 It is impossible to find the category code needed by `product create-common` from the CLI alone → product creation is manual/IDE-only for new categories.
- **Fix suggestion**: (1) resolve group names per region instead of hardcoding; (2) expand/flatten child categories and filter them client-side when `--keyword` is passed; (3) add `product create-common` auto-suggestion of category code by keyword.

---

### A8 [BUG] 🟠 Cloud timer actions are rejected — DP 207 ("timer") missing from device schema
- **Tool/Area**: TuyaOpen SDK `tuya_device_timer.c` + product DP schema
- **Symptom**: voice-set timer ("5 minute timer please") → agent creates a **cloud timer** via the BIC "timer" function → at the scheduled time the SDK executes it and pushes `{"207":"{...}"}` to the app; firmware logs `DP ID 207 Invalid` (dp_schema.c:352/387) and **does nothing**. From the user's side: "timer set" was confirmed by voice, then it "never ends".
- **Evidence** (device log, 13:55:21, exactly 5 min after the voice request at 13:50:52 — cloud timer rounded to the minute):
  ```
  tuya_device_timer.c:600 #29907337  [ONCE: 2026-08-19 13:55]  >> Action: dp207="{"date":"20260819","loops":"0000000","time":"13:55"}"
  dp_schema.c:352 DP ID 207 Invalid
  ```
- **Root cause**: DP 207 is the **standard hidden Tuya "timer" DP** used by the cloud-timer service (product's `uiConfig.bic` has `"code":"timer","selected":true`). The device schema (fetched from cloud activation) contains only the product's declared DPs (2,6,9,101–104) — no 207 — so `tuya_iot_dp_parse(DP_CMD_TIMER)` rejects the action before the app can react.
- **Impact**: 🟠 Any timer created via voice (or by the panel's scheduled-task UI) silently no-ops on the device.
- **Workaround**: set the focus timer from the app (DP 104 path) — verified working (chime + goal question at 13:44:19).
- **Fix suggestion**: (1) register DP 207 in the device schema as a hidden string DP so the timer action is accepted, then map a received 207 action to the focus-timer-end flow (chime + agent question); or (2) have the SDK forward unregistered-DP timer actions to a generic callback instead of dropping them with a log-only error.

### A9 [BUG] 🟠 Reflashing the bound module silently un-binds the device in the app
- **Tool/Area**: tuya-devplat / cloud pairing after reflash
- **Symptom**: after a successful reflash, the device received a cloud-initiated reset (`Reset id:d75abd…`, `unactive`, `GATEWAY_NOT_EXISTS`) and dropped offline; the app showed it unbound. No tool surfaces "your reflash invalidated the binding" — only log forensics reveal it.
- **Impact**: 🟠 silent service disruption; user must re-pair.
- **Fix suggestion**: post-flash status line in IDE/CLI ("device was previously bound; re-pair required"), and a clearer cloud-side reason in the app.

### A7 [BUG] 🟠 Real-Name Verification redirect loop blocks license collection (individual developer)
- **Tool/Area**: Tuya Developer Platform web (product → free license collection)
- **Symptom**: individual developer account is asked to pass real-name verification before placing orders/collecting licenses; clicking "Go Now" redirects back to the profile page in a **loop** — verification can never be completed, and the free-license flow stays permanently blocked.
- **Impact**: 🟠 The entire "new product + free license" path (our v3 plan, and D1's CLI gap) is unreachable for this account class; workaround = reuse the module's existing product binding.
- **Workaround**: skip license collection entirely — reflash under the module's already-bound product (`okqfzw6tkrabylcs`), which needs **no erase, no license write**.
- **Fix suggestion**: (platform) break the redirect loop and surface the actual verification step; (CLI/IDE) make the prerequisite explicit in the license dialog so users know verification is required before starting the flow.

## B. UX / Documentation Issues

### B1 [UX] 🟠 AGENTS.md documents `product detail --pid` which does not exist (see A5)
- **Tool/Area**: project AGENTS.md vs CLI reality
- **Detail**: AGENTS.md "Developer Platform CLI" section uses `product detail --pid <PID> --format json` — the CLI rejects `--pid` for `detail`. Agent runs from docs fail first try.
- **Fix suggestion**: regenerate docs from `schema get --group product --command detail`.

### B2 [UX] 🟠 Skill path mismatch: `.agents/skills/tuyaopen/debug-helper` documented, `.agents/skills/tuyaopen-debug-helper` real
- **Tool/Area**: bundled agent skills layout
- **Detail**: AGENTS.md (and the skill header) reference `tuyaopen/debug-helper`, `tuyaopen/dev-loop`, `tuyaopen/build` etc. under `.agents/skills/`, but several skills actually live flat: `.agents/skills/tuyaopen-debug-helper`, `tuyaopen-build`, `tuyaopen-dev-loop`. Script paths from docs fail.
- **Fix suggestion**: normalize skill directory naming (single convention: `owner/name`), and make AGENTS.md paths auto-generated.

### B3 [UX] 🟡 `monitor_helper.py` silently spawns a dead monitor when `OPEN_SDK_ROOT` is unset
- **Tool/Area**: debug-helper `monitor_helper.py`
- **Detail**: `_sdk_root()` falls back to `os.getcwd()` when `OPEN_SDK_ROOT` is missing; it then spawns `CWD\tos.py monitor ...` which doesn't exist → the process dies instantly with **no log file, no error**, and `status` reports `running:false`. An agent tails and gets "log file not found" — misleading.
- **Impact**: 🟡 Several minutes of confusion for agents not running inside the IDE terminal.
- **Fix suggestion**: fail fast with a clear message ("OPEN_SDK_ROOT not set and tos.py not found in <cwd>"), and capture the child's stderr into the session state on spawn failure.

### B4 [UX] 🟡 `tos.py monitor -l -p <port>` arg order trap
- **Tool/Area**: `tos.py monitor`
- **Detail**: `-l` takes a FILE argument; `tos.py monitor -l -p COM4` parses `-p` as the log file and then errors `Got unexpected extra argument (COM4)`. Flag-order dependence with no validation.
- **Fix suggestion**: validate that the value of `-l` doesn't start with `-`; document order (`-p` before `-l`); or make `-l` auto-name the file.

### B5 [UX] 🟡 tyutool GUI/CLI port contention gives generic "Access is denied"
- **Tool/Area**: tyutool
- **Detail**: with the tyutool GUI open, CLI `write` fails `plugin error: serial I/O: Access is denied` with no hint that another tool holds the port.
- **Fix suggestion**: on Windows, detect the busy COM port and print "held by another process (close tyutool GUI / other monitor)".

### B6 [UX] 🟡 tyutool `authorize` (read-only) ends with `Flash OK 2.8s`
- **Tool/Area**: tyutool authorize
- **Detail**: A read-only auth dump prints `Flash OK  2.8s` as the last line — a write-style status on a read operation. Confusing for script parsing.
- **Fix suggestion**: print `Auth read OK` / `Auth write OK` depending on the operation.

### B7 [UX] 🟡 Stored crash record is replayed at every boot before the new banner
- **Tool/Area**: T5AI bootloader / crash handler
- **Detail**: After any crash, every subsequent boot prints the full stored MemFault dump ("AP crash happend, rr: 0x12 ... MemFault ... CPU1 Current regs ..."), THEN boots normally. A healthy boot looks like a crash. Hard to distinguish a replayed record from a live crash without reading timestamps.
- **Impact**: 🟡 Cost us a full cycle of misdiagnosis.
- **Fix suggestion**: tag replayed records ("previous crash, stored record, booting anyway"), and/or add a "live crash" marker only on real-time faults.

### B8 [UX] 🟡 "Full chip erase (RF flag preserved)" preset erases only 0x0–0x1EDFFF
- **Tool/Area**: tyutool GUI erase presets (T5AI)
- **Detail**: The preset labeled "Full chip erase" targets `0x00000000 – 0x001EDFFF` (~1.9 MB) — a fraction of the 8 MB flash. Labels and ranges disagree; CLI `erase` default length is `0x200000` (2 MB) too. A user believing the chip is erased may leave stale app data in higher regions.
- **Fix suggestion**: rename presets to actual ranges ("Bootloader+app region erase"), or implement true full-chip erase.

### B9 [UX] 🟠 Flashing hangs with `plugin error: timeout after 10 attempts` — no auto-reset circuit on T5AI-Core
- **Tool/Area**: tyutool CLI + GUI, T5AI-Core board
- **Detail**: the board has **no DTR/RTS auto-reset circuit**; the flashed app boots ~1 s after power-on and grabs the UART. `tyutool_cli write` repeatedly fails `plugin error: timeout after 10 attempts` (GUI shows the same) with **no hint** that a manual **RST button press during the handshake window** (or a power-cycle) is required. We wasted a full session on this before discovering the procedure.
- **Workaround**: hold RST or power-cycle while the tool is in the handshake/retry phase (the GUI's ~10 s window works with an RST press).
- **Fix suggestion**: board bring-up docs must state "no auto-reset circuit — press RST during handshake"; tool should print that hint on handshake timeout; and the IDE flash command should detect the missing reset line.

### B10 [UX] 🟡 Focus-timer start plays a Chinese voice greeting instead of a chime
- **Tool/Area**: app firmware `app_dp_ctrl.c:238`
- **Detail**: starting the focus timer (DP 104) plays `AI_AUDIO_ALERT_POWER_ON` — a voice greeting ("我在这里，我们一起玩吧" / "Hello, I'm here, let's play") — which the user perceived as a confusing "weird Chinese phrase" on every timer start. The alert enum names don't map to what a user expects (POWER_ON is a spoken greeting, not a beep), and nothing in the docs warns that alert files are voice prompts.
- **Impact**: 🟡 confusing UX; the "start" sound cannot be told apart from a wake-up greeting.
- **Fix suggestion**: use a dedicated short chime/alert for timer start (or the WAKEUP-type alert used at timer end), and document the bundled alert files' actual audio content per enum index. Verified enum order from `ai_audio_player.h`: POWER_ON=0, NOT_ACTIVE=1, NETWORK_CFG=2, NETWORK_CONNECTED=3, NETWORK_FAIL=4, NETWORK_DISCONNECT=5, BATTERY_LOW=6, PLEASE_AGAIN=7, LONG_KEY_TALK=8, KEY_TALK=9, WAKEUP_TALK=10, RANDOM_TALK=11, WAKEUP=12.

### B11 [UX] 🟡 AI agent hallucinates device state — no device-context injection
- **Tool/Area**: AI agent (Brain) + app firmware
- **Detail**: the cloud agent has **zero visibility into device state**; asked "timer kitna bacha?" it asserted "Ravi ka time chal raha hai. Timer running hai." and later "Focus timer khatam ho chuka hai" while the device had **no** timer active (no DP 104 traffic in the log). Because the agent truthfully confirmed a timer it created via the cloud BIC timer (A8), the user believed a real countdown existed.
- **Impact**: 🟡 wrong answers erode trust; hard to detect without log forensics.
- **Fix suggestion**: (1) inject device context into the agent via `tuya_ai_text_input` when a timer starts/ends (we implemented this — `__app_dp_agent_text` in `app_dp_ctrl.c`); (2) prompt rule forbidding the agent from claiming it can see/set device state.

### B12 [UX] 🟡 Device boots in HOLD mode although app default is FREE
- **Tool/Area**: app firmware KV config
- **Detail**: on boot the KV read returns `{"volume":86,"chat_mode":0}` — `chat_mode:0` = `AI_CHAT_MODE_HOLD` overrides the code default `AI_CHAT_MODE_FREE`. A user who previously switched to Free (DP 9 = free) must re-switch after every reboot until the KV is rewritten with the new mode. The boot mode depends on a stale KV, not on the code default.
- **Fix suggestion**: persist mode on change (write KV) and/or treat KV `chat_mode` as authoritative only after first user change; surface current boot mode in the boot banner.

### B13 [UX] 🟡 Boot log replays "previous crash" record — looks like a live crash (follow-up to B7)
- **Tool/Area**: T5AI bootloader / crash handler
- **Detail**: after any crash, every boot prints the stored MemFault dump **before** the app banner, so a healthy boot of a previously-crashed device reads as an ongoing crash loop unless you check timestamps. Cost us a misdiagnosis cycle.
- **Fix suggestion**: label replayed records ("stored crash from previous boot") and print a "live crash" marker only for real-time faults.

---

## C. Notices / Observations

### C1 [NOT] 🔵 PID ↔ license product mismatch is silent
- **Detail**: We flashed firmware built with PID `u9bu0onwwwj6eyov` while the module's pre-burned license belongs to product `okqfzw6tkrabylcs`. The device still activated, synced the **license product's** DP schema (ids 1,2,6,9 — exactly Rankify's `switch_charge/status/volume_set/conversational_mode`), and cloud chat worked; only DP id expectations mismatched (see C2). No tool (devplat CLI, tyutool, IDE) surfaces "firmware PID ≠ license product PID".
- **Suggestion**: add a post-flash / IDE check comparing firmware PID vs auth product binding.

### C2 [NOT] 🟠 App-level DP ids are hardcoded and drift silently from the platform schema
- **Detail**: reference app hardcodes `DPID_VOLUME 3`; the bound product's actual volume DP is id 6 (`volume_set`). Volume set/report silently no-ops (device reports DP 3 which the product doesn't define; cloud ignores). Only discovered by diffing boot-log schema vs `dp-schema` CLI output.
- **Fix**: we changed `DPID_VOLUME 6` to match `volume_set`. General suggestion: generate DP id constants from the product snapshot (the `tuyaopen dp generate` profile exists — wire it into the embedded app) and validate at build time.

### C3 [NOT] 🔵 Country-code detection calls external service during build
- **Detail**: build prints `country code error: HTTPConnectionPool(host='www.ip-api.com'...)` repeatedly mid-build (offline/DNS-blocked networks). Non-fatal noise; also a privacy/network-dependency concern for an offline-friendly SDK.
- **Suggestion**: cache country, allow env override (`TUYA_COUNTRY`), and downgrade to a single warning.

### C4 [NOT] 🔵 `bk_bluetooth_init bluetooth already initialised` + `rosc` warnings at boot
- **Detail**: benign CP-side noise from the dual-core boot (BT stack already up, 32 kHz RC calibration warning). Not defects, but adds log clutter that looks alarming.

### C5 [NOT] 🔵 Build versioning has two inconsistent sources
- **Detail**: `CONFIG_PROJECT_VERSION` (0.1.0) drives image naming; C macro `PROJECT_VERSION` falls back to `1.0.0` in `tuya_main.c` via `#ifndef`. Banner prints the right one only if the SDK wires the macro. Easy for app code to show a stale version.
- **Suggestion**: single source of truth; fail the build if macro and config mismatch.

### C6 [NOT] 🟠 SDK reference app ships an uninitialized-variable bug: `reset_netcfg.c` returns uninitialized `rt`
- **Detail**: `reset_netconfig_check()` declares `OPERATE_RET rt;` and only assigns it in some branches — when `rst_cnt < RESET_NETCNT_MAX` it returns garbage (kept faithful in our port, flagged for the report). UB in SDK sample code.
- **Suggestion**: initialize `rt = OPRT_OK`; add `-Werror=maybe-uninitialized` in sample builds.

### C7 [NOT] 🟡 Dual-UART port roles are not stable across tools/enumeration on T5AI-Core
- **Detail**: `tyutool list-ports` reports COM3 = USB-Enhanced-SERIAL-B (CH342) and COM4 = USB-Enhanced-SERIAL-A. Flash succeeded via **GUI on COM4** @460800 (13:20:18, verification passed) while the CLI on COM3 kept failing with handshake timeouts (see B9); serial-monitor capture worked on COM3 @460800. The "flash port = lower enumeration" rule of thumb did **not** hold reliably here.
- **Suggestion**: the flash tool should probe both UARTs and/or report which one is the bootloader-capable port instead of assuming an ordering.

### C8 [NOT] 🟡 MQTT decryption hiccup after reconnect: `Cmd Parse Fail:-25344`
- **Detail**: after an MQTT re-connect, two `Cmd Parse Fail:-25344` (MQTT decrypt) errors appeared; the device recovered on its own (subsequent PUBACKs fine). Also one `event or session id was null` from the AI agent event path. No user-visible impact in this session.
- **Suggestion**: log the plaintext reason alongside the opaque code so reconnects can be diagnosed without SDK source diving.

### C9 [NOT] 🔵 Cloud-timer BIC works end-to-end (timing verified)
- **Detail**: despite A8, the **cloud** side of the timer was correct: the agent's "timer" BIC function created the task and it fired at the exact scheduled minute (13:55:21 for a 5-min timer requested at 13:50:52, rounding to the next minute). Only the device-side DP-207 acceptance was missing.

### C10 [NOT] 🔵 App-controlled focus timer (DP 104) verified end-to-end
- **Detail**: setting the timer from the app works: `app_dp: focus timer set -> 5 min (300 s)`, `focus timer finished` at the deadline, WAKEUP alert chime played, goal-question text injected to the agent, and `{"104":0}` reported back to the cloud. Knock detector healthy throughout (energy 150–232 thr band, heap stable ~136K, PSRAM ~8.5M, watchdog fed).

---

## D. Feature Requests / Suggestions

### D1 [SUG] Free-license collection is IDE-only — no CLI path
- **Detail**: "each product has 2 free licenses, account max 6" is only reachable through the IDE dialog. Agents driving the devplat CLI cannot collect or enumerate licenses, and the CLI has no license group at all (`auth`, `product`, `device`… none expose license state).
- **Suggestion**: add `license list / license collect --pid` to the CLI (matching the IDE dialog semantics + the "cannot be revoked" confirmation), and expose available/used counts.

### D2 [SUG] No CLI verification that a product has an agent (brain) associated device-side
- **Detail**: `project list` shows `endTypePidList` and `product-agent info` exists, but there is no guard before flashing an AI app — the whole AI-chat feature depends on the product↔agent association (end-type 3). We only confirmed by reading `project list` output manually.
- **Suggestion**: `product-agent info --pid <PID> --end-type 3` should be part of a pre-flash checklist command (e.g. `device ready-check`).

### D3 [SUG] `tuyaopen` project CLI: no command to map/validate DP ids vs the platform snapshot
- **Detail**: `tuyaopen dp generate` regenerates `tuya_dp_profile.h`/`schema.ts`, but nothing validates the app's hardcoded DP ids (like `DPID_VOLUME 3` vs actual `volume_set`=6) against the bound product snapshot.
- **Suggestion**: `tuyaopen dp validate --app-id 3 --expected volume_set` or a diff in `dp generate`.

### D4 [SUG] Flash tool should expose "which product is this license bound to"
- **Detail**: `tyutool authorize -p COMx` returns UUID/AuthKey only. The product binding was discovered indirectly from the boot log's dp_schema.
- **Suggestion**: `authorize --product-info` (query the platform by UUID, or decode the license region) so a flashed module's product can be verified offline-first.

### D5 [SUG] Windows build experience: one command, one venv, no console-encoding traps
- **Detail**: combining A2 (encoding), A3 (toolchain), C3 (country lookup), the first Windows build has ≥3 failure modes that are environment-specific and poorly surfaced.
- **Suggestion**: a `tos.py doctor` that checks console encoding, toolchain presence/checksum, network reachability, and prints a checklist.

### D6 [SUG] New agent projects cannot be published via CLI — dead end after `project create`
- **Tool/Area**: `tuya-devplat-cli project` / `product-agent save`
- **Detail**: Binding an agent to a product requires the agent to be **published + shelved** first. `product-agent save` fails with `PREREQUISITE_NOT_MET` and instructs: `project model-save → project model-publish`, then `project update-status --shelf-status 1`. But:
  - `schema list` proves `model-save` / `model-publish` exist in **no group** of this CLI — the error suggests commands that don't exist
  - `project update-status` (shelf) IS authorized, but fails with `AI_PLATFORM_PROJECT_EXIST_UNPUBLISHED_REGION` until the model is published
  - `ai-image generate` (needed for `--icon` on create) fails `api key permission denied` — no icon can be generated via CLI either (we reused an existing CDN icon)
- **Impact**: 🟠 A brand-new agent project is a CLI dead end: create works, bind does not. Full AI-hardware onboarding is impossible without leaving the CLI (publish must happen in the IDE AI-Agent page).
- **Workaround**: publish a model version in the IDE (AI Agent → project → model management → publish), then CLI can shelf + bind.
- **Fix suggestion**: add `project model-save` / `model-publish` (or a combined `project publish`) to the CLI; make `product-agent save`'s next-steps text only reference commands that actually exist; document the IDE-only publish step.

### D7 [SUG] No CLI visibility into AI usage/cost (credits, ASR/TTS/LLM consumption)
- **Tool/Area**: `tuya-devplat-cli` (all groups)
- **Detail**: the platform bills AI usage in **Credits** (formula: Model + ASR/TTS + Extended − Waiver; daily billing, invoice in 2 days). `schema search credit|cost|usage` returns **zero** billing/consumption commands — there is no way to check how many credits a device burned, whether the daily "AI Agent Integration" waiver quota is exhausted, or what a conversation round actually cost, without opening the platform web console.
- **Impact**: 🟡 For an AI-hardware product (pay-as-you-go model), cost telemetry is essential; CLI-driven development/testing has no cost guardrails or verification.
- **Fix suggestion**: add a `billing` group — `billing usage --pid <PID> --since <date>` (daily credit consumption per device), `billing waiver-status --pid <PID>` (free-quota remaining), and surface per-conversation credit deduction in `debug` or agent runtime-logs output. Reference rates are public: Aliyun paraformer-realtime-v2 ¥0.13/ASR-hour, cosyvoice-v3-flash ¥1.00/10k TTS chars, Qwen-flash ¥0.15/1M in + ¥1.50/1M out.

---

## E. Additions — What Worked Well

### E1 [ADD] 🔵 Symbolication out of the box
- `dist/.../debug/bk7258_ap/app.elf` + toolchain addr2line resolved the crash to exact lines (`lv_obj_tree.c:304`) — the debug artifacts in the dist folder are excellent. Keep them mandatory.

### E2 [ADD] 🔵 Pre-burned license + `tuya_authorize_read` flow is frictionless
- Reading the module license via tyutool (`authorize -p COM3`), and the firmware printing `Authorization read succeeds` with zero `tuya_config.h` edits, is the smoothest auth path of any vendor we tested.

### E3 [ADD] 🔵 `AI_USER_EVT_MIC_DATA` fires on every slice regardless of wake state
- The mic stream is always delivered to the app (`ai_audio_input.c`), which enabled our **software-only knock detector** — a unique product feature (desk double-tap wake) with zero extra hardware. Great architecture.

### E4 [ADD] 🔵 Background build + incremental rebuild works reliably
- `tos.py build` reuses the Ninja/CMake cache; only changed TUs recompile (`[1/21]` style), and the whole flow works under `Start-Process` with a redirected log.

### E5 [ADD] 🔵 `tuyaopen hardware set-used` is a nice agent-native contract
- Recording confirmed peripherals (`audio,led,button`) via one JSON-envelope command with `{"ok":true,"written":3,"source":"vibe"}` — simple, parseable, replaces fragile file editing.

### E6 [ADD] 🔵 tyutool `--plain` + tab-separated `list-ports` is agent-friendly
- `path  vid:pid  usb_interface  port_role  display_name` parsed cleanly for COM3/COM4 (CH342 dual-serial). Flash-vs-log port roles worked first try.

### E7 [ADD] 🔵 GUI flash tool verified: `Verification passed. Flash succeeded.` in 1m31s
- After the B9 RST-during-handshake procedure was discovered, the tyutool **GUI** flashed and **verified** the firmware (`Verification passed. Flash succeeded.` — 1m31s), which was more robust than the CLI's handshake window. The GUI also exposed `Authorize` read verification cleanly on the same port.

### E8 [ADD] 🔵 AI agent genuinely creates cloud timers when asked by voice
- The Brain's "timer" BIC function is real and precise: voice "5 minute timer please" → cloud timer created → fired at the scheduled minute (13:55:21, 5 min later, one-shot, loops `0000000`). The agent did **not** fabricate the "Done – timer set" confirmation — the defect was only the device-side DP-207 handling (A8).

---

## F. Session Log (chronological, this project)

| # | When | What | Outcome |
|---|------|------|---------|
| 1 | Setup | `tos.py build` first run | Toolchain download failed (A3) → manual zip install, sha256 verified |
| 2 | Setup | build again | Encoding crash in packaging (A2) → `PYTHONUTF8=1` fix |
| 3 | Build | v1 chat-bot port | ✅ `Tuya-Open-Preview_QIO_0.1.0.bin` (4.3 MB) |
| 4 | Feature | v2 knock detector (`app_knock.c`) | ✅ builds; wake via mic energy + state gate |
| 5 | Flash | tyutool `write -d t5 -p COM3` | ✅ Flash OK 80.5 s; board boots our banner |
| 6 | Boot | crash loop | 🔴 A1: UI init w/o display → `ENABLE_COMP_AI_DISPLAY=n` fixed; reflash ✅ |
| 7 | Cloud | boot log | dp_schema sync shows license product = `okqfzw6tkrabylcs` (C1/C2) |
| 8 | License | `authorize -p COM3` | ✅ pre-burned UUID/AuthKey read; product binding only via boot log (D4) |
| 9 | CLI | devplat calls from PowerShell | wrapper unusable (A4), flag mismatch (A5), category-tree fails (A6) |
| 10 | CLI | `project list` | found agent "Rankify Brain" `aipt_f6hcebhd23nk` associated end-type 2+3 with `okqfzw6tkrabylcs` (D2) |
| 11 | Plan | product strategy | new "AI Toy" product + free licenses (D1), Rankify untouched |
| 12 | v3 | product created in IDE | "Study assistant" PID `7jgm24mceuhwcrnw`, AI Toy, Wi-Fi+BLE, std DPs 101–104 |
| 13 | v3 | `project create` | "Study Assist Brain" `aipt_fvjuqr11yk8w` ✅ created via CLI (icon: reused existing CDN URL because `ai-image generate` → api key permission denied, D6) |
| 14 | v3 | `product-agent save` (device-side) | ❌ `PREREQUISITE_NOT_MET` — agent not published/shelved; suggested `project model-save/publish` **don't exist in CLI** (D6); `update-status` fails `AI_PLATFORM_PROJECT_EXIST_UNPUBLISHED_REGION` → publish must be IDE-only |
| 15 | v3 | license collection | ❌ Real-Name Verification redirect loop (A7) blocks free-license collection → v3 new-product path abandoned |
| 16 | Flash | reflash attempt 1 (CLI COM3, GUI COM4) | ❌ `plugin error: timeout after 10 attempts` repeatedly — **no auto-reset circuit on T5AI-Core** (B9); app boots and grabs UART |
| 17 | Flash | reflash with **RST press during handshake** (GUI, COM4 @460800) | ✅ `Verification passed. Flash succeeded.` 1m31s — new firmware live (knock tuning logs, DP schema 2/6/9/101–104) |
| 18 | Cloud | post-flash device state | device received cloud-initiated reset (`Reset id:…`, `unactive`, `GATEWAY_NOT_EXISTS`) — silently unbound (A9); re-paired in app |
| 19 | Feature | full feature pass (serial-debug 13:36–13:44) | ✅ mute (DP101), LED (DP102), mode→free (DP9=3), **app-set focus timer end** (13:44:19: chime + goal question + `{"104":0}`), knock detector stable |
| 20 | UX | focus timer start | plays `AI_AUDIO_ALERT_POWER_ON` — a Chinese **voice greeting**, not a chime (B10); enum order documented |
| 21 | AI | agent asked about timer | ❌ hallucinated "timer running"/"timer khatam ho chuka" — device had no active timer (B11) → fix: device-context injection + prompt rule |
| 22 | AI | voice "5 minute timer please" | agent created a **cloud timer** (E8) which fired at 13:55:21 → `DP ID 207 Invalid` → device did nothing (A8). "Timer never ended" mystery solved |
| 23 | Cloud | reconnect stability | `Cmd Parse Fail:-25344` ×2 + `event or session id was null` — self-recovered (C8) |
| 24 | Build | timer-context injection (`__app_dp_agent_text` in `app_dp_ctrl.c`) | ✅ code added: agent told when app-timer starts (with real minutes) and when it ends; **build+flash pending** |

---

## G. Open Report Items

- [x] Verify knock threshold behavior on live desk (tuning pass) — ✅ thresholds 150–232 tracked room noise correctly (C10)
- [ ] Validate volume DP id on the new product after creation (C2)
- [ ] Re-test A1 fix across a display-less and a display-equipped board
- [ ] Confirm D1 license CLI gap with a feature request ticket
- [x] Flash UX on T5AI-Core (B9) — procedure documented (RST during handshake); GUI more robust than CLI
- [x] Voice-set timer investigation (A8) — root cause: DP 207 missing from device schema; cloud side correct (E8); fix suggested, not yet applied
- [ ] Rebuild + flash the timer-context injection (row 24) and verify agent answers about app-set timers truthfully
- [ ] Test cloud-timer fix (A8) once DP-207 registration is applied
