# Engine Room ESP (Webpage variant) — Xiao ESP32-C3

> **⚠️ Work in progress.** This is a newer variant of the EngRoom firmware, meant to pair with [`Bridge_ESP_Webpage`](../Bridge_ESP_Webpage). It hasn't been through the same amount of real-world testing as the plain [`Engine_Room_ESP`](../Engine_Room_ESP) sketch yet. Expect rough edges. If you just want the proven, minimal firmware, use `Engine_Room_ESP` instead.

Stardrive section controller — same core effects as `Engine_Room_ESP` (nacelles, deflector, impulse engines, nav lights, neck/case windows, ESP-NOW to Bridge/DataPad), plus:

- **Independent aft photon torpedo control**, for the optional second-LED "Photon 2x" torpedo variant (adds a connector on GPIO9, active-low, no transistor needed). Fires and animates separately from the forward tube — not just a mirror of it. If you don't have the second connector installed, this is simply unused.
- **Automatically joins Bridge's WiFi AP** on boot — no manual WiFi setup needed as long as `Bridge_ESP_Webpage` is running nearby with its default AP settings.
- **Fallback access point** if Bridge's AP can't be reached within 3 minutes (Bridge off, unflashed, or out of range) — EngRoom hosts its own small AP with a bare-bones recovery/test page, so you're never locked out of OTA just because Bridge is unreachable.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | XIAO_ESP32C3 |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Upload speed | 921600 |

No external libraries needed — `WebServer.h` is part of the ESP32 Arduino core, same as everything else this sketch uses.

---

## First flash

Same as `Engine_Room_ESP` — see that folder's README for module assembly notes, the download-mode button sequence, and the board-settings-reset warning. OTA password is `admin`.

---

## MAC addresses to update

Open `Engine_Room_ESP_Webpage.ino` and find the peer MAC arrays near the top of the file:

```cpp
uint8_t broadcastAddress[]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // DataPad
uint8_t bridgeAddress[]     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Bridge
uint8_t dataPad2Address[]   = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // DataPad 2.8" (optional)
```

Two ways to set these:
- **Edit and recompile** — replace with the actual MACs from your hardware (run `MAC_address_retriver/` on each board), same as the plain sketch.
- **Set them live over Serial** — connect at 115200 baud and send `SETMAC bridge AA:BB:CC:DD:EE:FF` (or `SETMAC datapad ...` / `SETMAC datapad2 ...`). Saved to NVS, applied immediately, survives reboots.

---

## Fallback AP and recovery page

If EngRoom can't join Bridge's AP within 3 minutes, it hosts its own:

| Setting | Value |
|---------|-------|
| SSID | `EngRoom Fallback` |
| Password | `ncc1701-d` |

Connect to it and browse to `http://192.168.4.1/` for a plain status/test page: connection status, All LEDs On/Off (also pings Bridge over ESP-NOW as a connectivity check), an LED test walk, a plain reboot, and a reset-to-default that clears settings but keeps your paired peer MACs. It keeps retrying Bridge's AP in the background and tears itself down automatically once Bridge is reachable again — no reboot needed.

---

## Everything else

GPIO assignments (including GPIO9 for the optional aft photon connector), key effects, and NVS settings are the same as the plain `Engine_Room_ESP` sketch — see that folder's README for the full reference tables. The NVS clear utility (`EngRoom_NVS_Clear/`) works the same way here too.
