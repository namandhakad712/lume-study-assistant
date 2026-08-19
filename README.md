# Lume Study Assistant

**Voice-first AI study companion for exam prep** — built on the **TuyaOpen**
embedded platform (T5AI chip, T5AI-Core board). Talk to it hands-free, start a
study focus timer from the app, and get honest, voice-friendly help — no screen
required.

<div align="center" style="border:2px dashed #999; border-radius:12px; margin:24px 0; background:#fafafa;">
  <img src="docs/images/lume.jpg">
</div>

---
> **Docs site**: https://namandhakad712.github.io/lume-study-assistant/
> (full documentation — architecture, DP schema, agent config, build & flash guide)

## ℹ️ Built with (attributes)

| Component | Property / Statement | Reference |
|-----------|---------------------|-----------|
| **TuyaOpen SDK** | Open-source Tuya IoT development framework (TAL/TKL abstraction, `tos.py` toolchain) | [github.com/tuya/tuyaopen](https://github.com/tuya/tuyaopen) |
| **TuyaOpen IDE** | AI-agent-assisted IDE for embedded + panel development; scaffolded this project (`tuyaopen-app`, framework `base`) | [developer.tuya.com](https://developer.tuya.com) |
| **T5AI chip** | Wi-Fi + AI-audio SoC (Beken BK7258-class) with on-chip AI audio components (mic capture, player, KWS) | [developer.tuya.com/docs](https://developer.tuya.com/en/docs/iot) |
| **T5AI-Core board** | Tuya development board (`TUYA_T5AI_CORE` Kconfig) — mic, speaker, button, LED, Wi-Fi | [developer.tuya.com](https://developer.tuya.com) |
| **Tuya Developer Platform** | Cloud: product `okqfzw6tkrabylcs`, DP schema, AI Agent ("Brain") `aipt_fvjuqr11yk8w`, BIC cloud functions | [developer.tuya.com](https://developer.tuya.com) |
| **Ray mini-app (panel)** | Tuya Panel SDK / Ray framework — `source/miniapp/` (TypeScript + React) | [developer.tuya.com/en/docs/iot/ray](https://developer.tuya.com/en/docs/iot/ray) |
| **tyutool** | Tuya serial flash/debug tool (GUI + CLI) shipped with the SDK | `tools/tyutool/` in the SDK |

> This is an **independent demo project** — not affiliated with, endorsed by, or
> maintained by Tuya. Built during the TuyaOpen IDE Preview Program using public
> SDK/docs and Tuya's cloud platform under their respective licenses/terms.

## ✨ Features

- 🎙️ **Voice AI chat** — press the button or **double-knock the desk** to wake the
  AI; ask anything, get short voice answers (Hinglish / Hindi / English).
- ⏱️ **Study focus timer** — set 5–180 minutes from the panel app; the device plays
  a chime on start/end and asks whether you hit your study goal.
- 💬 **Conversation modes** — `hold` / `key` / `weakup` / `free` (DP 9), persisted.
- 🔇 **Mute, LED, volume, study mode** — all app-controlled via Tuya DPs.
- 🧠 **Exam-prep brain** — the Tuya AI agent ("Lume") keeps answers syllabus-first
  and short, remembers the student's name/goal, and strictly refuses cloud timers.
- 🚫 **No screen, no wires** — single speaker device; everything is voice + app.

## 🖥️ Hardware

| | |
|---|---|
| Chip | T5AI (Beken BK7258-class, Wi-Fi + audio AI) |
| Board | Tuya T5AI-Core (`TUYA_T5AI_CORE`) |
| Audio | On-board mic + speaker (AI audio components) |
| Input | Physical button (`ai_chat_button`) + desk double-knock detection |
| LED | On-board indicator (`led`), app-driven + AI-state driven |
| Network | Wi-Fi, provisioned via the Tuya app |

## 📁 Repository layout

```
├── .tuyaopen/                  # IDE + AI metadata (gitignored — machine-local)
├── docs/                       # docs site (GitHub Pages — markdown only)
│   ├── index.md                # this overview
│   ├── architecture.md         # chip, board, firmware modules
│   ├── configuration.md        # project config, Kconfig, agent files
│   ├── dp-schema.md            # full DP reference + platform screenshots
│   ├── agent.md                # AI agent ("Brain") prompt + injection
│   ├── agent-prompt.md         # paste-ready prompt text
│   ├── build-flash.md          # build & flash guide (with gotchas)
│   ├── panel-setup.md          # panel app setup (visual + Ray)
├── source/
│   ├── embedded/               # firmware (C) — build with tos.py
│   └── miniapp/                # Ray mini-app panel (TypeScript)
├── dist/                       # published firmware bins (QIO / UA / UG)
├── LICENSE                     # MIT
└── README.md
```

## 🚀 Quick start

### 1. Build the firmware

Requires the TuyaOpen SDK (IDE-installed). Windows note: the toolchain hits a
Python encoding bug — set UTF-8 first.

```powershell
cd source/embedded
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
# prepend SDK .venv/.tools to PATH, then:
tos.py build
```

Output lands in `source/embedded/dist/Tuya-Open-Preview_0.1.0/`:
`Tuya-Open-Preview_QIO_0.1.0.bin` (flash image), plus `_UA_`/`_UG_` variants.

### 2. Flash the device

T5AI-Core has **no auto-reset circuit** — press the board **RST** during the
handshake window (or power-cycle). Working recipe: tyutool GUI, **COM4 @ 460800**,
~1m30s. See [docs/build-flash.md](docs/build-flash.md) for CLI usage and gotchas.

### 3. Set up the panel & agent

- **Panel** — build/design in TuyaOpen IDE → Panel (Ray mini-app). Mute is a
  toggle (Target = "Not"). See [docs/panel-setup.md](docs/panel-setup.md).
- **Agent** — paste the system prompt in IDE → AI Agent → Brain → Study Assist
  Brain (`aipt_fvjuqr11yk8w`), save & publish. See [docs/agent.md](docs/agent.md).

### 4. Device authorization

`tuya_config.h` ships with placeholders — the module's real device credentials
were pre-burned at the factory. If the device comes up unbound after a reflash,
re-authorize via tyutool UART or the IDE, then re-pair in the app.

## 📊 Product & DPs

Product PID `okqfzw6tkrabylcs` ("Rankify Assist"). Active DPs:

| ID | Code | Type | Mode | Meaning |
|----|------|------|------|---------|
| 2 | `status` | enum | ro | standby / charging / playing / on / off … |
| 6 | `volume_set` | value | rw | 0–100 % |
| 9 | `conversational_mode` | enum | rw | hold / key / weakup / free |
| 101 | `mute` | bool | rw | true = muted |
| 102 | `led_switch` | bool | rw | true = LED on |
| 103 | `study_mode` | enum | rw | general / physics / chemistry / biology / … |
| 104 | `focus_timer` | value | rw | 0–180 min, step 5 |

Full reference with screenshots: [docs/dp-schema.md](docs/dp-schema.md).
Generated profile: `source/embedded/include/tuya_dp_profile.h`.

## 📜 License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 namandhakad712.

> ⚠️ **ARCHIVED** — no further changes or maintenance. This project was built as
> part of the **TuyaOpen IDE Preview Program** (evaluation/demo purposes).