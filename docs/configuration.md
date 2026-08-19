# Configuration

This page documents every configuration surface of the project: project metadata,
platform/board config, device authorization, AI-agent skill files, and the
generated DP profile.

## Project identity

| File | Role |
|------|------|
| `.tuyaopen/project.json` | canonical descriptor: name `Tuya-Open-Preview`, version `0.1.0`, type `tuyaopen-app`, framework `base`, license `Apache-2.0` |
| `.tuyaopen/status.json` | lifecycle (`scaffolded → configured → built → flashed`), build/flash results |
| `.tuyaopen/architecture.json` | surfaces (embedded/miniapp), modules, dependencies |
| `tuyaopen.project.ini` | human-readable mirror: project, platform (`t5ai`), board (`TUYA_T5AI_CORE`), build output |

> `.tuyaopen/` and `tuyaopen.project.ini` are **machine-local** (credentials,
> SDK paths) and gitignored — they are not part of the public repo.

## Board & platform (`source/embedded/app_default.config`)

```ini
CONFIG_PROJECT_NAME="Tuya-Open-Preview"
CONFIG_PROJECT_VERSION="0.1.0"
CONFIG_TUYA_PRODUCT_ID="okqfzw6tkrabylcs"
CONFIG_BOARD_CHOICE_T5AI=y
CONFIG_BOARD_CHOICE_TUYA_T5AI_CORE=y
CONFIG_TUYA_T5AI_BOARD_EX_MODULE_NONE=y
CONFIG_ENABLE_MBEDTLS_SSL_MAX_CONTENT_LEN=4096
CONFIG_BUTTON_NAME="ai_chat_button"
CONFIG_BUTTION_STACK_SIZE=4096
CONFIG_ENABLE_AI_COMPONENTS=y
CONFIG_ENABLE_LIBLVGL=y
CONFIG_ENABLE_COMP_AI_DISPLAY=n
```

Notable switches:

| Symbol | Value | Effect |
|--------|-------|--------|
| `CONFIG_BOARD_CHOICE_T5AI` | y | T5AI platform |
| `CONFIG_BOARD_CHOICE_TUYA_T5AI_CORE` | y | T5AI-Core board BSP |
| `CONFIG_ENABLE_AI_COMPONENTS` | y | AI audio components (mic capture, player, chat) |
| `CONFIG_ENABLE_LIBLVGL` | y | LVGL linked (display components **not** enabled) |
| `CONFIG_ENABLE_COMP_AI_DISPLAY` | n | no AI display — pure voice device |
| `CONFIG_BUTTON_NAME` | `ai_chat_button` | button registered by BSP |

Change via `tos.py config` / `tos.py menuconfig` — do not hand-edit unless you
know the exact Kconfig symbols.

## Device authorization (`source/embedded/include/tuya_config.h`)

- Ships with **placeholder** `UUID` / `AuthKey` — never real credentials.
- The module's license was **pre-burned at the factory**; the device authorizes
  itself with the burned credentials, so the placeholders never need filling.
- After a full reflash, the device may come up unbound — re-authorize via the
  TuyaOpen IDE or `tyutool` UART authorize, then re-pair in the app.
- The bundled flash dump (`tyutool_read_*.bin`) is gitignored on purpose — it
  contains the license region and must never be published.

## DP profile (generated)

- `source/embedded/include/tuya_dp_profile.h` — `DPID_*` defines + value bounds,
  stamped with `// DP-HASH:`.
- `source/miniapp/src/devices/schema.ts` — panel-side mirror.
- Regenerate with: `tuyaopen dp generate --target embedded,miniapp`
- Never edit these by hand; update the product in the IDE/CLI first.

See [DP Schema](dp-schema.md) for the full reference.

## AI agent skill files (not app code)

The repo used to carry AI-agent skill packs under `.agents/` and `.claude/`
(hardware-vibe-coding, smart-panel-dev, tuya-iot-platform, etc.). These are
**developer-tooling only** and are gitignored in the public repo:

- `.agents/` — opencode/agent skill catalogue
- `.claude/` — Claude Code skills
- `AGENTS.md`, `CLAUDE.md` — agent instructions (root)

They are not required to build, flash, or use the device.

## Environment (Windows build)

The SDK toolchain (Python 3.12 + GNU make) hits a **codepage bug** when packaging:
```
'charmap' codec can't encode character
```
Workaround — always set UTF-8 before invoking `tos.py`:

```powershell
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
```

Full build/flash recipe: [Build & Flash](build-flash.md).