# OmniScan

Public command surface for **root** — the ESP32-S3 wireless recon agent (Wi‑Fi · BLE · Sub‑GHz · Wi‑Fi LR).

**Firmware / flash:** [rockstars4nny-hub/root](https://github.com/rockstars4nny-hub/root)  
**Command reference:** [COMMANDS.md](COMMANDS.md)  
**ARIA:** Kit → **Omni** tab proxies `./omni` to the kit.

Authorized assessments on gear you own or have written permission to test. Passiveive listen + SoftAP dashboard — no BLE write, jam, or inject.

## Quick start

1. Flash [root](https://github.com/rockstars4nny-hub/root) (`pio run -e esp32-s3-n16r8 -t upload`).
2. Join SoftAP `root` / `root-radar` → `http://192.168.4.1`.
3. Serial @ 115200 or HTTP:

```bash
curl -s -X POST http://192.168.4.1/api/omni \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"./omni status"}'
```

```text
./omni start
./omni ble list
./omni subghz raw
./omni lr status
./omni system help
```

## Source in this repo

| File | Role |
|------|------|
| [COMMANDS.md](COMMANDS.md) | Full `./omni` command list |
| [omni_cmd.h](omni_cmd.h) / [omni_cmd.cpp](omni_cmd.cpp) | Parser + help text (built into root firmware) |

## Related

- [root](https://github.com/rockstars4nny-hub/root) — ESP32 firmware + dashboard
- [ARIA](https://github.com/rockstars4nny-hub/ARIA) — operator console (Omni tab)
