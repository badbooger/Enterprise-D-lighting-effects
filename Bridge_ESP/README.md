# Bridge ESP — ESP32-S3

Saucer section controller. Drives 15 window groups and 3 dedicated outputs (nav lights, two impulse engines) through NPN transistors. Runs the startup/shutdown sequence, engine pulse, navigation blink, and electrical-short damage effect. Hosts a sound player (DY-SV17F). Communicates with EngRoom and the DataPad via ESP-NOW.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Partition scheme | Default 4MB with spiffs (or Huge APP if flash space is needed) |
| Upload speed | 921600 |

No external libraries needed — all dependencies (ArduinoOTA, ESPmDNS, Preferences, esp_now) are part of the ESP32 Arduino core.

---

## PCB and module assembly

The saucer PCBs are designed around an ESP32-S3 module (WROOM or equivalent). JLCPCB can place and solder the module as part of the PCB assembly order if they have the part in stock — check availability when ordering. The assembled PCB has its own USB-C connector for flashing, so no separate dev board is needed once the PCB is built.

If JLCPCB doesn't have the module available, or if you want to test the sketch before the PCBs arrive: flash the sketch onto a standalone ESP32-S3 dev board first, confirm it works, then transfer the module to the PCB. Once installed, use the onboard USB-C on the PCB or OTA for all subsequent flashes.

---

## First flash

Connect via the USB-C port on the PCB (or dev board). Select the correct COM port in Arduino IDE and upload.

After the first USB flash, subsequent updates can be done OTA — see the root README for the OTA procedure.

**OTA password:** `admin` — entered in Arduino IDE at upload time. There is no `ArduinoOTA.setPassword()` call in the sketch; the library accepts whatever the IDE sends.

---

## MAC addresses to update

Open `Bridge_ESP.ino` and find the peer MAC arrays near the top of the file. Replace with the actual MACs from your hardware (run `MAC_address_retriver/` on each board):

```cpp
uint8_t broadcastAddress1[] = { 0x3c, 0xdc, 0x75, 0xae, 0xcd, 0xb8 };  // EngRoom
uint8_t broadcastAddress2[] = { 0x20, 0x6e, 0xf1, 0xa9, 0xa1, 0x14 };  // DataPad
```

---

## GPIO assignments

| Label | GPIO | Function |
|-------|------|----------|
| G01–G15 | 6, 7, 35, 5, 15, 21, 16, 14, 13, 12, 11, 10, 8, 3, 9 | 15 window groups (indices 0–14) |
| NAV | 38 | Navigation lights — LEDC blink |
| IM LEFT | 36 | Impulse engine port — LEDC sine pulse |
| IM RIGHT | 37 | Impulse engine starboard — LEDC sine pulse |
| Sound TX | 17 | DY-SV17F UART TX |
| Sound RX | 18 | DY-SV17F UART RX |

All LED groups are switched through MMBT2222A NPN transistors. GPIO drives the transistor base; LEDs are on the collector side. Do not connect LEDs directly to GPIO pins.

**Startup sequence order:**
```
G15 → G14 → G02 → G01+G03 → G09 → G08+G10 → G07+G11 → G06+G12 → G05+G13 → G04 → NAV → Engines
```
Shutdown runs the same sequence in reverse.

---

## Key effects

**Engine pulse** — smooth sine-wave brightness cycle (80–255, 5-second period) on both impulse engines. Always on when the model is running.

**Navigation blink** — toggles at `blinkIntervalMs` (default 1000ms, configurable from DataPad). If DataPad contact is lost, switches to a fault pattern: 3 rapid flashes then a 1.8s pause.

**Electrical short (damage mode)** — 2–3 random window groups flicker via PWM; remaining windows snap to random on/off states; engines go to independent random flicker. Auto-cancels after 2 minutes. Restores pre-damage state on cancel.

**Auto-start** — if no ESP-NOW contact is received within `auto_ms` (default 30s) of boot, Bridge runs the startup sequence independently. Useful when running the saucer without the DataPad.

---

## Sound (DY-SV17F)

UART2, TX=GPIO17, RX=GPIO18, 9600 baud. 25 sound files on 4MB onboard flash. Files named `00001.mp3`–`00025.mp3` — copy them to the player in order (sorted by name). Ready-to-copy files are in `sounds/DY_Player/`. See `SOUND_MAP.md` for the full file list.

Auto-triggered sounds: startup plays file 1, shutdown plays file 2 or 3 (random), damage plays an alert klaxon then a random damage sound.

---

## NVS settings

Namespace: `"bridge"`. All timing values are configurable from the DataPad settings screen and persist across reboots.

| Key | Default | Purpose |
|-----|---------|---------|
| `blink_ms` | 1000 | Nav light blink interval (ms) |
| `win_ms` | 300 | Window startup step timing (ms) |
| `spc_ms` | 800 | Nav/engine startup step timing (ms) |
| `auto_ms` | 30000 | Auto-start timeout — 0 to disable |
| `conn_ms` | 300000 | Connection-lost timeout (ms) |
| `sync_mode` | true | Assembled (true) / Separated (false) mode |
| `asm_mode` | false | Assembly blink mode |

---

## NVS clear utility

If Bridge gets stuck trying to connect to WiFi on every boot: flash `Bridge_NVS_Clear/Bridge_NVS_Clear.ino`, confirm "Done" in Serial Monitor (115200), then reflash the main sketch.
