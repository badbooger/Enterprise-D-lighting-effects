# DataPad — Waveshare 7" ESP32-S3

LCARS touchscreen controller for the Enterprise-D prop. Runs a full LVGL-based LCARS interface on an 800×480 7" display. Controls all lighting, sound, WiFi/OTA, and settings across Bridge and EngRoom via ESP-NOW. Has its own DY1703A sound player for UI sounds and ambient audio.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | Waveshare ESP32-S3-Touch-LCD-7 (or ESP32S3 Dev Module) |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Flash size | 16MB |
| Partition scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload speed | 921600 |

**Required libraries** (in `libraries/` — set sketchbook path to the project root):

| Library | Version | Notes |
|---------|---------|-------|
| lvgl | 8.3.11 | Must be 8.x — LVGL 9 has a different API and will not compile |
| ESP32_Display_Panel | 1.0.0 | Display + touch driver |
| ESP32_IO_Expander | — | Dependency of ESP32_Display_Panel |
| esp-lib-utils | — | Dependency of ESP32_Display_Panel |

---

## First flash

Connect via USB-C to the Waveshare board. Select the correct COM port and upload. The board may need the flash mode button held during initial connection depending on the specific Waveshare revision.

After the first USB flash, subsequent updates can be done OTA — see the root README for the OTA procedure.

**OTA password:** `admin` — entered in Arduino IDE at upload time.

---

## MAC addresses to update

Open `Data_Pad.ino` and find the peer MAC arrays near the top of the file:

```cpp
uint8_t broadcastAddress1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // EngRoom
uint8_t broadcastAddress2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Bridge
uint8_t broadcastAddressWarpCore[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // WarpCore
```

Replace with the actual MACs from your hardware (run `MAC_address_retriver/` on each board).

---

## Screens

| Screen | File | Purpose |
|--------|------|---------|
| Screen 1 | `ui_Screen1.c` | Main ops — Power, Nav, Engines, Damage, Warp |
| Screen 2 | `ui_Screen2.c` | Settings — timing, sync mode, sensor, sound toggles |
| Screen 3 | `ui_Screen3.c` | WiFi credentials and OTA |
| BridgeSound | `ui_BridgeSound.c/.h` | DY-SV17F sound player controls |
| RedAlert | `ui_RedAlert.c/.h` | Full-screen red alert overlay |

---

## UI editing — important

The UI was built in SquareLine Studio (LVGL 8.3.x project) and then heavily hand-edited. Follow these rules before touching any UI file:

- **Never replace `ui_BridgeSound.c/.h` with a SquareLine Studio export** — it is hand-written from scratch and an SLS export will overwrite it.
- **Never replace `ui_Screen1.c` or `ui_Screen2.c` with an SLS export** — both have hand-written layout and event code that SLS doesn't know about.
- **`ui_events.c` stubs get re-commented on every SLS export** — after any SLS re-export, re-comment the stubs in `ui_events.c` manually.
- `ui.c` and `ui.h` (the screen registry) can be replaced with SLS export output safely.

All UI changes are made by editing the `.c` files directly.

---

## Sound (DY1703A)

UART1, TX=GPIO16, RX=GPIO15, 9600 baud. SD card in root — files `00001.mp3`–`00044.mp3`, no folders. Files 1–25 match the Bridge DY-SV17F file numbers exactly (same sound, same file number, different player). Files 26–44 are DataPad-only (touch feedback, voiced alerts, ambient loops, easter eggs).

See `SOUND_MAP.md` for the full file list.

---

## NVS settings

Namespace: `"datapad"`.

| Key | Default | Purpose |
|-----|---------|---------|
| `sync_mode` | 1 | Assembled (1) / Separated (0) — startup routes to both sections or each independently |
| `touch_snd` | false | UI touch click sounds |
| `ambient_on` | false | Ambient audio auto-play on power-up |
| `radar_on` | true | LD2410C presence sensor trigger enabled |
| `alert_radar` | true | Sensor fires red alert when model is already on |
| `nav_timing` | 1 | Nav light timing: 1=1.0s, 2=1.5s, 3=always on |
| `wifi_auto` | false | Auto-connect to saved WiFi on boot |
