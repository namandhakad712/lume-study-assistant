# Study AI Assistant — Full Development Plan (study-assist-PLAN.md)

> **Project**: Tuya-Open-Preview — "Study AI Assistant"
> **Hardware**: Tuya T5AI-Core (T5AI chip, TUYA_T5AI_CORE board)
> **Goal**: Top-20 TuyaOpen Preview Tester submission — unique voice-AI device with **desk double-tap (knock) wake**, button wake, LED activity feedback, seamless non-interrupting AI talk, and memory.
> **Status**: v1 (chat bot port) + v2 (knock-to-wake) built & flashed. v3 (own product + license) in progress.

---

## 1. Product Vision & Feature Set

| # | Feature | Requirement | Status |
|---|---------|-------------|--------|
| 1 | AI voice chat (turn-based) | Wake → ask → AI answers → back to idle. No interrupted talk. | ✅ Implemented (AI_CHAT_MODE_ONE_SHOT) |
| 2 | **Double fist-tap (knock) wake** | Two knocks on the desk trigger wake — no button, no touch. | ✅ Implemented (`app_knock.c`) — needs live tuning |
| 3 | Button wake (fallback) | Physical button P29 single-click = wake | ✅ Implemented |
| 4 | LED activity feedback | LED blinks in LISTEN/THINK, solid in SPEAK, off in idle | ✅ Handled by ONESHOT mode (`tdl_led`) |
| 5 | Memory (multi-turn context) | AI remembers conversation context | ✅ Cloud agent handles context (project brain) |
| 6 | Own product + license | Clean product in **AI Toy** category, own free license, Rankify untouched | 🚧 In progress (v3) |
| 7 | Volume control DP | Volume set/report over cloud | ✅ Firmware (DPID_VOLUME=6) — needs matching product DP |
| 8 | Miniapp panel (optional) | Ray panel showing chat/status | ⏳ Post-v3 optional |

---

## 2. Current Firmware State (v1 + v2)

### What's already done
- **Ported the SDK chat-bot app** (`your_chat_bot`) into `source/embedded/`:
  - `include/app_chat_bot.h`, `src/app_chat_bot.c` — AI chat glue, `AI_CHAT_MODE_ONE_SHOT`, default volume 70, MIC_DATA event hook
  - `include/reset_netcfg.h`, `src/reset_netcfg.c` — 5x reset-count network re-config
  - `src/tuya_app_main.c` — full IoT app: `tuya_iot_init`, netmgr, board registration, volume DP, OTA handler
  - `Kconfig` (APP_CONFIG → ENABLE_AI_COMPONENTS, TUYA_PRODUCT_ID) and `app_default.config` (board T5AI-Core, PID, MBEDTLS 4096, button, AI components, **ENABLE_COMP_AI_DISPLAY=n** ← critical fix, see §6)
- **Knock-to-wake detector** (`include/app_knock.h`, `src/app_knock.c`):
  - Feed: `AI_USER_EVT_MIC_DATA` slices (80 ms PCM16LE) inside the app's event callback
  - Energy per slice → adaptive noise floor (1 s rolling minimum) → burst detection → double-knock (100–800 ms gap) → worker thread fires `ai_mode_handle_key(TDL_BUTTON_PRESS_SINGLE_CLICK)` — same path as the physical button
  - Gated on `ai_mode_get_state() == AI_MODE_STATE_IDLE` → never interrupts listening/thinking/speaking
  - No extra hardware — pure software on the existing codec stream
- **Build**: `tos.py build` → `dist/Tuya-Open-Preview_0.1.0/Tuya-Open-Preview_QIO_0.1.0.bin` (4.1 MiB)
- **Flashed** on the module via tyutool v3.2.8 (COM3, 921600 baud, ~80 s)
- **Boot verified**: app banner, license read (`Authorization read succeeds`), DP schema sync, Wi-Fi connect, MQTT path reached

### Known firmware issues (documented in TESTING-REPORT.md)
- `reset_netcfg.c` returns uninitialized `rt` (SDK reference bug, kept faithful)
- Crash on first boot was **not** our app code — it was `ENABLE_COMP_AI_DISPLAY=y` default with no display on the board (LVGL null-deref). Fixed via `ENABLE_COMP_AI_DISPLAY=n`
- `DPID_VOLUME` was 3 (wrong for Rankify product) → changed to **6** to match `volume_set`

---

## 3. v3 — Clean Product Setup (AI Toy category)

> ⚠️ **Rankify (`okqfzw6tkrabylcs`) is OBSOLETE. Do NOT touch it.** The module's pre-burned license is bound to Rankify, so we must erase + write the new product's license.

### 3.1 Product creation (manual, user) — ✅ DONE
- **Product**: "Study assistant" · **PID `7jgm24mceuhwcrnw`** · **Category AI Toy** · Wi-Fi + BLE
- **Standard DPs**: 101 `percent_state`, 102 `control`, 103 `mode`, 104 `current_storage`
- **volume_set custom DP**: pending (user adds in IDE → expect id **105**; confirm via dp-schema)
- **License**: pending — collect the 2 free licenses; provide UUID + AuthKey for module write

### 3.2 Agent project (brain) association — 🚧 IN PROGRESS
- ✅ Brain project **"Study Assist Brain" `aipt_fvjuqr11yk8w`** created via CLI (agentType home)
- ⛔ Binding to PID blocked: agent must be **published + shelved** first; `model-save`/`model-publish` **do not exist in the CLI** (TESTING-REPORT D6) — publish must be done in the **IDE: AI Agent → project → publish model**
- After publish: CLI shelf (`project update-status --shelf-status 1`) → bind:
  ```bash
  # Step 1 (设备端): dry-run → confirm
  node .tuyaopen/ide/bin/devplat-cli-launcher.js product-agent save --pid 7jgm24mceuhwcrnw --end-type 3 --project-codes "aipt_fvjuqr11yk8w"
  # Step 2 (面板端):
  node .tuyaopen/ide/bin/devplat-cli-launcher.js product-agent save --pid 7jgm24mceuhwcrnw --end-type 2 --project-codes "aipt_fvjuqr11yk8w"
  # Step 3 (面板部署):
  node .tuyaopen/ide/bin/devplat-cli-launcher.js product-agent ui-save --pid 7jgm24mceuhwcrnw --project-codes "aipt_fvjuqr11yk8w" --float-project-code "aipt_fvjuqr11yk8w"
  ```

### 3.3 Firmware changes for v3

| File | Change |
|------|--------|
| `source/embedded/Kconfig` | `TUYA_PRODUCT_ID` default → `7jgm24mceuhwcrnw` |
| `source/embedded/app_default.config` | `CONFIG_TUYA_PRODUCT_ID="7jgm24mceuhwcrnw"` |
| `source/embedded/include/tuya_config.h` | Placeholders only — license comes from module flash (do NOT hardcode) |
| `src/tuya_app_main.c` | `DPID_VOLUME` → actual `volume_set` id on new product (expect **105**, confirm via dp-schema) |

Rebuild: `tos.py build` → new QIO image.

### 3.4 Flash sequence (clean slate)

```powershell
# 1. Close tyutool GUI (holds COM3)
# 2. Full erase (RF flag preserved):
& tyutool_cli.exe --plain erase -d t5 -p COM3 -s 0x0 -l 0x1EE000
# 3. Write the NEW product's license:
& tyutool_cli.exe --plain authorize -p COM3 --uuid <UUID> --authkey <AUTHKEY>
# 4. Verify license:
& tyutool_cli.exe --plain authorize -p COM3
# 5. Flash our firmware:
& tyutool_cli.exe --plain write -d t5 -f dist\...\Tuya-Open-Preview_QIO_0.1.0.bin -p COM3
```

> ⚠️ **Never full-erase without having a valid license ready to write** — it wipes the pre-burned license and the device becomes cloud-dead.

---

## 4. On-Device Verification Plan

| Step | Check | Pass criteria |
|------|-------|---------------|
| 1 | Boot | Banner shows `Tuya-Open-Preview`, no MemFault, no crash loop |
| 2 | License | `Authorization read succeeds` |
| 3 | Cloud | `dp_schema create Success`, MQTT connected, agent ready |
| 4 | Button wake | Single-click → wake alert → LISTEN → ask question → AI reply plays |
| 5 | Knock wake | 2 desk taps → same wake path (tune §5) |
| 6 | No-interrupt | During AI speech, knock/button do NOT cut the reply |
| 7 | LED | Blink on listen, on while speaking, off when idle |
| 8 | Volume DP | App sets volume 0–100, cloud reports correct value |
| 9 | Memory | Ask follow-up question referencing earlier context — AI remembers |
| 10 | OTA | OTA request notification path doesn't break device |

---

## 5. Knock Tuning Procedure (on-desk)

`app_knock.c` tuning knobs (macros):

| Macro | Default | Effect |
|-------|---------|--------|
| `KNOCK_THRESH_GAIN` | 8 | hit threshold = floor × gain |
| `KNOCK_THRESH_MIN` | 300 | hard minimum energy |
| `KNOCK_MAX_BURST_MS` | 200 | reject long loud stretches (speech) |
| `KNOCK_MIN_GAP_MS` | 100 | min gap between knocks |
| `KNOCK_MAX_GAP_MS` | 800 | max window for 2nd knock |
| `KNOCK_ARM_COOLDOWN_MS` | 1500 | ignore window after trigger |

Tuning loop (device on desk, monitor running):
1. Log quiet-room energy (add temporary `PR_DEBUG` of slice energy if needed)
2. Knock naturally → if no trigger: lower `KNOCK_THRESH_MIN`, then `KNOCK_THRESH_GAIN`
3. If false triggers on speech/ambient: raise gain/min, tighten burst window
4. If double-knock mis-detected as 2 singles: widen `KNOCK_MAX_GAP_MS`
5. Rebuild → flash → repeat until ~95%+ success on natural knocks

---

## 6. Critical Lessons (crash post-mortem)

**Symptom**: crash loop every boot, `MemFault`, MMFAR=0x4.
**Symbolication** (addr2line on `debug/bk7258_ap/app.elf`):
```
lv_obj_get_display ← CRASH        lv_obj_tree.c:304
lv_obj_invalidate_area            lv_obj_pos.c:811
lv_obj_remove_flag                lv_obj.c:160
__ui_init                         ai_ui_chat_wechat.c:162
ai_ui_init                        ai_ui_manage.c:540
__page_chat_open                  ai_ui_manage.c:143
ai_chat_init                      ai_chat_main.c:380
app_chat_bot_init                 app_chat_bot.c:73
user_main                         tuya_app_main.c:325
```
**Root cause**: `ENABLE_COMP_AI_DISPLAY` Kconfig **defaults to `y`**; the T5AI-Core board has **no display**; `ai_chat_init` unconditionally initializes LVGL UI → `lv_scr_act()` NULL → crash.
**Fix**: `CONFIG_ENABLE_COMP_AI_DISPLAY=n` in `app_default.config`.
**Takeaway for report**: the SDK should either auto-detect display availability or gate UI init on a display registration check.

---

## 7. Remaining Work / Roadmap

- [ ] User: **add `volume_set` custom DP (0–100)** to product `7jgm24mceuhwcrnw` in IDE → share resulting DP id (expect 105)
- [ ] User: **publish a model version** for "Study Assist Brain" (`aipt_fvjuqr11yk8w`) in IDE (AI Agent page) — CLI cannot (D6)
- [ ] Agent: shelf the agent + bind to PID (device-side end-type 3, panel-side end-type 2, ui-save)
- [ ] User: **collect 2 free licenses** for the product → share UUID + AuthKey
- [ ] Agent: set new PID in Kconfig + app_default.config, update `DPID_VOLUME`, rebuild
- [ ] User+agent: erase module → write new license → verify → flash QIO
- [ ] On-device verification (§4) with live monitor
- [ ] Knock sensitivity tuning (§5)
- [ ] Optionally: Ray miniapp panel (status/chat UI) via `tuyaopen dp generate`
- [ ] Compile TESTING-REPORT.md additions from the v3 session
- [ ] Final tester submission package (firmware + report + video if required)

---

## 8. Open Questions (for user)

1. ~~**AI Toy product created?** Share PID when done.~~ ✅ `7jgm24mceuhwcrnw`
2. **License**: share UUID + AuthKey (platform page) after collecting the free licenses.
3. ~~**Agent brain**: reuse Rankify Brain or new project?~~ ✅ New project "Study Assist Brain" `aipt_fvjuqr11yk8w` created; needs IDE model publish.
4. **Product DPs**: add `volume_set` custom DP — confirm id after creation.
5. **Miniapp panel**: wanted for the submission, or firmware-only is fine?
