# Build & Flash

## Prerequisites

- TuyaOpen SDK installed (IDE-managed) — `C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK`
  on the dev machine.
- Python 3.12 + GNU make from the SDK `.tools` / `.venv`.
- Verify once: `tos.py check`.

## Build

```powershell
cd source/embedded

# Windows: fix the toolchain codepage bug (A2) — always set these:
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"

# PATH must include the SDK venv + tools (the IDE does this; in a plain shell:)
$env:PATH = "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\.venv\Scripts;" +
            "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\.tools\python\3.12.13\cpython-3.12.13-windows-x86_64-none;" +
            "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\.tools\make\4.4.1;" + $env:PATH

python "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\tos.py" build
```

**Output** → `source/embedded/dist/Tuya-Open-Preview_0.1.0/`:

| File | Purpose |
|------|---------|
| `Tuya-Open-Preview_QIO_0.1.0.bin` | full flash image (use this) |
| `Tuya-Open-Preview_UA_0.1.0.bin` | user app partition |
| `Tuya-Open-Preview_UG_0.1.0.bin` | upgrade image |

The published bins are tracked in `dist/` so anyone can flash without rebuilding.

### Build gotchas

- **A2 – charmap bug**: without `PYTHONUTF8`, packaging fails with
  `'charmap' codec can't encode character …`. The env vars above fix it.
- First build takes a while (SDK components). Incremental builds are fast.
- `tos.py build` must run from `source/embedded` (where `app_default.config` lives).

## Flash

> **Critical**: T5AI-Core has **no auto-reset circuit** (finding B9). You must
> press the board's **RST button during the handshake window**, or power-cycle,
> or the tool hangs waiting for the device.

### GUI (recommended — verified working)

1. Open tyutool **GUI** (from the SDK tools).
2. Port: **COM4 @ 460800** (observed working pair; see ports note below).
3. Select `Tuya-Open-Preview_QIO_0.1.0.bin`.
4. Click flash → **immediately press RST** on the board.
5. Wait ~1m30s for `Verification passed. Flash succeeded.`

<div align="center" style="border:2px dashed #999; border-radius:12px; padding:24px; margin:24px 0; background:#fafafa;">
  📸 <strong>Screenshot placeholder</strong> — tyutool GUI flash dialog (port, baud, bin
  selected, success message)<br/>
  <small>Save as <code>docs/images/flash-gui.png</code> and add: <code>&lt;img src="images/flash-gui.png"&gt;</code></small>
</div>

### CLI

```powershell
& "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\tools\tyutool\tyutool_cli.exe" `
    --plain write -d t5 -p COM3 -f source/embedded/dist/Tuya-Open-Preview_0.1.0/Tuya-Open-Preview_QIO_0.1.0.bin
```

(CLI @921600; again — press **RST during the handshake**.)

### Ports note (finding C7)

The two USB-serial roles are **unstable**: COM3 was the log port
(USB-Enhanced-SERIAL-B) and COM4 the flash port (USB-Enhanced-SERIAL-A), but the
roles have swapped between sessions. If flash times out, swap ports or check
Device Manager.

## After flashing

1. **Re-authorize / re-pair** (finding A9): a full reflash can silently un-bind
   the device from your account (`Reset id:…`, `unactive`, `GATEWAY_NOT_EXISTS`).
   Re-add the device in the Tuya app (license is pre-burned, no UUID/AuthKey entry
   needed).
2. **Set conversation mode to `free`** (finding B12): the device boots in `hold`
   until the app writes DP 9 = `free`.
3. Check the log console (`PR_*` output) for clean boot:
   `app init ok`, cloud `mqtt connected`, no `Cmd Parse Fail`.

## Monitoring device logs

Serial monitor on the **log port** (usually COM3 @ 460800 or as detected):

```powershell
& "C:\Users\naman\TuyaOpenIDE\TuyaOpenSDK\tools\tyutool\tyutool_cli.exe" --plain monitor -p COM3
```

Or use the TuyaOpen IDE's built-in monitor. Logs use `PR_DEBUG/PR_NOTICE/PR_WARN/PR_ERR`.

<div align="center" style="border:2px dashed #999; border-radius:12px; padding:24px; margin:24px 0; background:#fafafa;">
  📸 <strong>Screenshot placeholder</strong> — serial monitor showing a clean boot
  (app init ok, mqtt connected)<br/>
  <small>Save as <code>docs/images/boot-log.png</code> and add: <code>&lt;img src="images/boot-log.png"&gt;</code></small>
</div>