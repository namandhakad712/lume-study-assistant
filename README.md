# Tuya-Open-Preview

Embedded firmware scaffolded from `TuyaOpenSDK/tools/app_template/base` into `source/embedded/`. Build with `cd source/embedded && tos.py build`.

## Layout

```
Tuya-Open-Preview/
├── .tuyaopen/              # IDE + AI metadata (read by AI agents)
│   ├── project.json        # canonical project descriptor
│   ├── status.json         # lifecycle / build / flash state
│   └── architecture.json   # per-surface architecture
├── source/
│   ├── embedded/           # firmware (`tos.py build` runs here)
│   └── miniapp/            # mini-app / panel (Panel SDK scaffolds here)
├── tuyaopen.project.ini    # legacy human-readable mirror
└── README.md
```

## Surfaces

- **Embedded firmware** — `source/embedded/`. Build & flash via `tos.py` once the TuyaOpen SDK is installed.
- **Mini app (panel)** — `source/miniapp/`. Scaffold the panel project there with the Tuya Panel App SDK; see `source/miniapp/README.md`.
