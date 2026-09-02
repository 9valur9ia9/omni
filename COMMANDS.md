# OmniScan (`./omni`)

**root** wireless recon agent for **ESP32-S3** — Serial UART · HTTP `POST /api/omni` · ESP-NOW (Wi‑Fi LR).

Authorized / owned-kit use only. Passiveive listen + SoftAP dashboard. No BLE write, no jam, no inject.

Firmware lives in **[rockstars4nny-hub/root](https://github.com/rockstars4nny-hub/root)**. ARIA Kit → **Omni** tab proxies `POST /api/root/omni`.

---

## Transport

| Path | How |
|------|-----|
| Serial | USB UART @ 115200 — type `./omni …` |
| HTTP | `POST http://192.168.4.1/api/omni` with `{"cmd":"./omni status"}` |
| HTTP GET | `GET /api/omni?cmd=./omni%20status` |
| ARIA | Kit → Omni console → `/api/root/omni` |
| ESP-NOW | Peer path after `./omni lr peer <MAC>` (Wi‑Fi LR) |

---

## Core

| Command | Description |
|---------|-------------|
| `./omni start` | Start full-spectrum scan (Wi‑Fi + BLE + Sub‑GHz + LoRa) |
| `./omni stop` | Stop scanning / save session snapshot |
| `./omni status` | All subsystem status |
| `./omni system help` | Full in-firmware help |
| `./omni system info` | Heap / PSRAM / uptime |
| `./omni system reset` | Soft reboot |

---

## Wi‑Fi

| Command | Description |
|---------|-------------|
| `./omni wifi channel <1-11>` | Fix listen channel |
| `./omni wifi channel hop` | Hop channels 1→11 |
| `./omni wifi channel fixed` | Stop hopping |
| `./omni wifi handshake` | Passive EAPoL observation stats |
| `./omni wifi deauth` | Observed deauth/disassoc frames |

---

## BLE (onboard ESP32-S3)

| Command | Description |
|---------|-------------|
| `./omni ble scan on` | Enable advertise scan |
| `./omni ble scan off` | Disable BLE scan |
| `./omni ble list` | List discovered advertisers |
| `./omni ble filter <name>` | Name substring filter |
| `./omni ble filter clear` | Clear filter |

Devices appear in `/api/devices` with `"band":"ble"`.

---

## Sub‑GHz (CC1101)

| Command | Description |
|---------|-------------|
| `./omni subghz scan on` / `off` | Enable / disable CC1101 RX |
| `./omni subghz freq <315\|433\|868\|915>` | Fixed band |
| `./omni subghz hop` | Hop 315→433→868→915 |
| `./omni subghz list` | Recent hits |
| `./omni subghz raw` | Last 10 raw packets (HEX / ASCII / GPS) |
| `./omni subghz raw <N>` | Last N packets (1–50) |
| `./omni subghz raw last` | Newest packet only |
| `./omni subghz raw filter <mhz>` | Filter by frequency (e.g. `433.92`) |
| `./omni subghz raw clear` | Clear PSRAM ring |
| `./omni subghz raw save` | Export `SUBGHZ` `.bin` to `psram://…` (no SD on kit) |
| `./omni subghz raw decode <hex>` | ASCII / binary / base64 / 8·16-bit views |
| `./omni subghz raw analyze` | Patterns, band %, RSSI & byte hist |

Raw ring default: **2048 × 128 bytes** in PSRAM (override with `-DROOT_SUBGHZ_RAW_CAP=…`).

---

## LoRa (E22 UART)

| Command | Description |
|---------|-------------|
| `./omni lora scan on` / `off` | Listen transparent RX |
| `./omni lora freq <868.1\|915.0>` | Note frequency (module configured externally) |
| `./omni lora list` | Recent LoRa hits |

---

## Wi‑Fi Long Range / ESP-NOW

Espressif **802.11 LR** on SoftAP (`b/g/n + LR`) + ESP-NOW peer messaging.

| Command | Description |
|---------|-------------|
| `./omni lr status` | Protocol / peer / sent·ACK |
| `./omni lr peer <AA:BB:CC:DD:EE:FF>` | Set peer MAC |
| `./omni lr send <message>` | Send payload |
| `./omni lr test` | Ping / PONG RTT |

---

## GPS · AP · Log

| Command | Description |
|---------|-------------|
| `./omni gps status` | Fix / inject status |
| `./omni gps reset` | Clear / reacquire |
| `./omni ap status` | SoftAP SSID / clients / IP |
| `./omni ap on` / `off` | SoftAP (kit dashboard) |
| `./omni ap ssid <name>` | Change SSID |
| `./omni log status` | Session log |
| `./omni log dump` | Dump snapshot |
| `./omni log save` | Force save path |

Laptop GPS inject: `POST /api/gps` `{"lat":…,"lon":…}`.

---

## Example session

```text
./omni start
./omni status
./omni ble list
./omni subghz raw
./omni subghz raw 5
./omni subghz raw filter 433.92
./omni subghz raw decode 48656C6C6F
./omni subghz raw analyze
./omni lr status
./omni system help
```

```bash
curl -s -X POST http://192.168.4.1/api/omni \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"./omni subghz raw last"}'
```

---

## Ceiling (honest)

| On stock kit | Status |
|--------------|--------|
| Wi‑Fi promiscuous listen | Yes |
| SoftAP dashboard | Yes |
| Onboard BLE advertise scan | Yes |
| Sub‑GHz raw HEX | Yes |
| Wi‑Fi LR + ESP-NOW | Yes |
| BLE write / MITM | No |
| Deauth TX / jam | No |
| MicroSD capture | No (PSRAM / `psram://` only) |
