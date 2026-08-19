# Architecture

## Platform

| | |
|---|---|
| Platform | `t5ai` |
| Board | Tuya T5AI-Core (`kconfigId`: `TUYA_T5AI_CORE`) |
| SDK | TuyaOpen SDK (`main`) |
| Framework | `base` (audio / AI components) |
| RTOS | tuya-tal (`tuya_app_main` task, 1024×4 stack, prio 1) |
| Entry point | `tuya_app_main()` — `source/embedded/src/tuya_app_main.c` |
| License | factory pre-burned on module (UUID/AuthKey in `tuya_config.h` are placeholders) |

Board config (`app_default.config`):

```
CONFIG_PROJECT_NAME="Tuya-Open-Preview"
CONFIG_TUYA_PRODUCT_ID="okqfzw6tkrabylcs"
CONFIG_BOARD_CHOICE_T5AI=y
CONFIG_BOARD_CHOICE_TUYA_T5AI_CORE=y
CONFIG_ENABLE_MBEDTLS_SSL_MAX_CONTENT_LEN=4096
CONFIG_BUTTON_NAME="ai_chat_button"
CONFIG_ENABLE_AI_COMPONENTS=y
CONFIG_ENABLE_LIBLVGL=y
CONFIG_ENABLE_COMP_AI_DISPLAY=n
```

## Firmware modules (`source/embedded/src/`)

| File | Responsibility |
|------|----------------|
| `tuya_app_main.c` | entry point; cloud init, WiFi/netmgr, OTA notify, DP dispatch, hardware registration |
| `app_chat_bot.c` | AI chat lifecycle; feeds every PCM mic slice to the knock detector (`AI_USER_EVT_MIC_DATA`) |
| `app_dp_ctrl.c` | all DP logic: volume, conversational mode, mute, LED override, study mode, focus timer, agent text injection |
| `app_knock.c` | desk double-knock wake: energy-burst detection in the mic stream, gap 100–800 ms, cooldown 1.5 s |
| `reset_netcfg.c` | network-config reset handling (re-pair) |

### Audio & AI flow

```
mic ──> ai_audio_input ──> app_chat_bot (AI_USER_EVT_MIC_DATA)
                              │
                              ├──> app_knock_feed_mic()   (double-knock wake)
                              │
                              └──> AI chat (Tuya AI agent, cloud)
speaker <── ai_audio_player (TTS answers, alerts, timer chimes)
```

<div align="center" style="border:2px dashed #999; border-radius:12px; padding:24px; margin:24px 0; background:#fafafa;">
  📸 <strong>Diagram placeholder</strong> — a clean audio/AI flow diagram (mic → knock detect →
  AI agent → TTS → speaker)<br/>
  <small>Save as <code>docs/images/audio-ai-flow.png</code> and add: <code>&lt;img src="images/audio-ai-flow.png"&gt;</code></small>
</div>

- Wake: physical button (`ai_chat_button`) or double knock → `ai_mode_handle_key(TDL_BUTTON_PRESS_SINGLE_CLICK, …)`.
- Chat replies are played through `ai_audio_player` (short, voice-first answers).
- Agent text injection uses `tuya_ai_text_input` (see [AI Agent](agent.md)).

## Data points

All 7 active DPs are handled in `app_dp_ctrl.c`; IDs/bounds come from the generated
`source/embedded/include/tuya_dp_profile.h`. Full reference: [DP Schema](dp-schema.md).

## Focus timer (DP 104)

- Set from app (0–180 min, step 5).
- Firmware runs the countdown on-device (timing via tal timer).
- Start: plays `AI_AUDIO_ALERT_POWER_ON` chime, injects `"The user started the
  study focus timer for <N> minutes. Remember this."` into the agent.
- End: reports `{"104":0}`, plays wakeup alert, injects
  `"The focus timer has ended. Ask the user whether they completed their study goal."`
- Cancel by re-setting to 0.

## LED (DP 102)

- User override (on/off) via `tdl_led_*` — forced states beat AI-driven LED states.
- On init, the user's saved override is re-asserted after the mode layer drives the LED.

## Known architectural notes

- **No auto-reset circuit** on T5AI-Core — flashing requires manual RST (see [Build & Flash](build-flash.md)).
- **Cloud timers land on DP 207**, which is not in the product schema — agent is
  forbidden from creating them (see [AI Agent](agent.md)).
- Device boots in `hold` conversation mode until the app sets `free` (KV-persisted).