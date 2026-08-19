# Lume Study Assistant

Voice-first AI study companion for NEET UG prep, built on the **TuyaOpen** embedded
platform (T5AI chip, T5AI-Core board). Talk to it hands-free, start a study focus
timer from the app, and get honest, voice-friendly help — no screen required.

> **Docs site**: https://namandhakad712.github.io/lume-study-assistant/
> (full documentation — architecture, DP schema, agent config, build & flash guide)

---

## Features

- **Voice AI chat** — press the button or **double-knock the desk** to wake the AI;
  ask anything, get short voice answers (Hinglish / Hindi / English).
- **Study focus timer** — set 5–180 minutes from the panel app; the device plays a
  chime on start/end and asks whether you hit your study goal.
- **Conversation modes** — `hold` / `key` / `weakup` / `free` (DP 9), persisted.
- **Mute, LED, volume, study mode** — all app-controlled via Tuya DPs.
- **NEET-UG brain** — the Tuya AI agent ("Lume") keeps answers NCERT-first and
  short, remembers the student's name/goal, and strictly refuses cloud timers.
- **No screen, no wires** — single speaker device; everything is voice + app.

## Hardware

| | |
|---|---|
| Chip | T5AI (Beken BK7258-class, Wi-Fi + audio AI) |
| Board | Tuya T5AI-Core (`TUYA_T5AI_CORE`) |
| Audio | On-board mic + speaker (AI audio components) |
| Input | Physical button (`ai_chat_button`) + desk double-knock detection |
| LED | On-board indicator (`led`), app-driven + AI-state driven |
| Network | Wi-Fi, provisioned via the Tuya app |

## Repository layout

```
├── .tuyaopen/                  # IDE + AI metadata (gitignored — machine-local)
├── docs/                       # MkDocs site (GitHub Pages)
│   ├── index.md                # overview
│   ├── architecture.md         # chip, board, firmware modules
│   ├── configuration.md        # project config, Kconfig, agent files
│   ├── dp-schema.md            # full DP reference
│   ├── agent.md                # AI agent ("Brain") prompt + injection
│   ├── build-flash.md          # build & flash guide (with gotchas)
│   └── panel-setup.md          # panel app setup (visual + Ray)
├── source/
│   ├── embedded/               # firmware (C) — build with tos.py
│   │   ├── include/            # app headers, tuya_config.h, tuya_dp_profile.h
│   │   └── src/                # tuya_app_main.c, app_chat_bot.c, app_dp_ctrl.c,
│   │                           # app_knock.c, reset_netcfg.c
│   └── miniapp/                # Ray mini-app panel (TypeScript)
├── dist/                       # published firmware bins (QIO / UA / UG)
├── LICENSE                     # MIT
├── README.md
└── TESTING-REPORT.md           # device test session log (findings A–E)
```

## Quick start

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

`tuya_config.h` ships with placeholder UUID/AuthKey — the module's real license
was pre-burned at the factory. Authorize via tyutool UART or the IDE.

## Product & DPs

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

Full reference: [docs/dp-schema.md](docs/dp-schema.md). Generated profile:
`source/embedded/include/tuya_dp_profile.h`.

## Testing

Device-on-bench findings are logged in [TESTING-REPORT.md](TESTING-REPORT.md)
(issue classes A–E: agent, board, cloud, chat, environment). Highlights:

- Cloud timers created by the agent fire correctly (BIC) but land on DP 207,
  which is not in the schema — the agent is therefore forbidden from creating
  timers (see [docs/agent.md](docs/agent.md)).
- Re-flashing a licensed module can silently un-bind it from the account —
  re-authorize in the app after flashing.
- The board boots in `hold` conversation mode until the app sets `free`.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 namandhakad712.