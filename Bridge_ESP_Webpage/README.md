# Bridge ESP (Webpage variant) — ESP32-S3

> **⚠️ Work in progress.** This is a newer variant of the Bridge firmware that hosts its own WiFi control page — it hasn't been through the same amount of real-world testing as the plain [`Bridge_ESP`](../Bridge_ESP) sketch yet. Expect rough edges. If you just want the proven, minimal firmware (DataPad-driven, no built-in webpage), use `Bridge_ESP` instead.

Saucer section controller — same core effects as `Bridge_ESP` (window groups, nav lights, impulse engines, sound player, startup/shutdown sequences, ESP-NOW to EngRoom/DataPad), plus:

- **Hosts its own WiFi access point and control webpage** (`WebServer.h`) — no home network or DataPad required to run the model. Connect a phone or laptop directly to Bridge's AP and control everything from a browser.
- **Autonomous demo mode** — an optional showpiece loop that cycles power on/off and fires random effects on its own, useful for unattended display.
- **DataPad auto-pairing window** — a button on the control page opens a short listen-and-adopt window so a new DataPad can be paired without recompiling anything.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Partition scheme | Default 4MB with spiffs (or Huge APP if flash space is needed) |
| Upload speed | 921600 |

No external libraries needed — `WebServer.h` is part of the ESP32 Arduino core, same as everything else this sketch uses.

This folder needs both files together: `Bridge_ESP_Webpage.ino` and `control_html.h` (the control page's HTML/JS, served straight from flash).

---

## First flash

Same as `Bridge_ESP` — see that folder's README for PCB/module assembly notes and the board-settings-reset warning. OTA password is `admin`.

---

## MAC addresses to update

Open `Bridge_ESP_Webpage.ino` and find the peer MAC arrays near the top of the file:

```cpp
uint8_t broadcastAddress1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // EngRoom
uint8_t broadcastAddress2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // DataPad
```

Two ways to set these:
- **Edit and recompile** — replace with the actual MACs from your hardware (run `MAC_address_retriver/` on each board), same as the plain sketch.
- **Set them live over the control page** — once flashed and connected to Bridge's AP, browse to `http://<bridge-ip>/setmac?peer=engroom&mac=AA:BB:CC:DD:EE:FF` (or `peer=datapad`). Saved to NVS, applied immediately, survives reboots. No recompiling needed if you swap a board later.

---

## Connecting to Bridge's control page

Bridge hosts its own access point:

| Setting | Value |
|---------|-------|
| SSID | `Enterprise D` |
| Password | `ncc1701-d` |

Connect a phone or laptop to it, then browse to `http://192.168.4.1/`. Change the SSID/password constants near the top of the sketch if you'd rather use your own.

---

## Everything else

GPIO assignments, key effects, sound setup, and NVS settings are identical to the plain `Bridge_ESP` sketch — see that folder's README for the full reference tables. The NVS clear utility (`Bridge_NVS_Clear/`) works the same way here too.
