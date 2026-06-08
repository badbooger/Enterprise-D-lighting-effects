# USS Enterprise-D Warp Core Controller

ESP32-based lighting controller for a 3D-printed TNG warp core prop. Runs a 10-LED chase effect with a smooth ambient glow, speed-controlled by a potentiometer. Optionally integrates with the [Enterprise-D Prop Controller](https://github.com/badbooger/Enterprise-D-lightning-effects) via ESP-NOW — the DataPad can control speed and on/off remotely, and the LD2410C presence sensor triggers the main model to power up when someone approaches.

Runs fully standalone with no other hardware required.

---

## 3D model

Based on **ST:TNG Warp Core by ElmoC**. Licensed under Creative Commons Attribution — Non-Commercial — No Derivatives 3.0.

The original Thingiverse listing (thing:1656741) is no longer available. A complete archive of all STL files, the original PCB files, and the original Arduino sketch is preserved in the main Enterprise-D repo under `enterprise documentation/Warp_Core/`.

A modified base (`WarpCoreBase_mmWaveSensor.3mf`) is included in that archive — it integrates the LD2410C mmWave sensor housing directly into the warp core base as a single unit, replacing the original base and a separate sensor mount.

Other warp core designs exist on Thingiverse in different sizes, including versions built around ARGB LED strips which give a full colour-animated effect. Worth exploring if you are starting from scratch — ARGB gives considerably more visual range than the single-colour PWM approach used here.

---

## Hardware

| Component | Notes |
|-----------|-------|
| Arduino Nano ESP32 | Drop-in replacement for the original Arduino Nano in ElmoC's design |
| 10× NPN transistors (MMBT2222A or similar) | One per chase LED group |
| 10× chase LEDs | Through NPN transistors — do not connect directly to GPIO |
| 1× ambient LED | Driven via `analogWrite` on A5 |
| 10kΩ potentiometer | Speed control — wiper to A7, ends to 3.3V and GND |
| LD2410C mmWave presence sensor | Optional — OUT pin to D7, powered from 5V boost converter |

---

## Pin assignments

| Pin | Function | Notes |
|-----|----------|-------|
| A4 | Chase LED 1 | |
| A3 | Chase LED 2 | |
| A2 | Chase LED 3 | |
| A1 | Chase LED 4 | |
| A0 | Chase LED 5 | |
| D2 | Chase LED 6 | Use `D2` constant — raw `2` is a different GPIO on the Nano ESP32 |
| D3 | Chase LED 7 | Use `D3` constant |
| D4 | Chase LED 8 | Use `D4` constant |
| D5 | Chase LED 9 | Use `D5` constant |
| D6 | Chase LED 10 | Use `D6` constant |
| A5 | Ambient LED | `analogWrite` PWM — dim during chase, full on pause |
| A7 | Speed pot | ADC — wiper input |
| D7 | LD2410C OUT | Push-pull, HIGH = presence detected. No pull-up needed. |

> **Important:** On the Arduino Nano ESP32, `D2`–`D6` Arduino constants do **not** map to raw GPIO 2–6. Always use the `D2`/`D3` etc. constants in the sketch — using raw integers will drive the wrong pins.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | Arduino Nano ESP32 |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Upload speed | 921600 |

No external libraries needed — all dependencies (ArduinoOTA, ESPmDNS, Preferences, esp_now) are part of the ESP32 Arduino core.

---

## First flash

Connect via USB-C. Select the correct COM port in Arduino IDE and upload.

After the first USB flash, subsequent updates can be done OTA once WiFi is connected (see OTA section in the main Enterprise-D repo README).

**OTA password:** `admin` — entered in Arduino IDE at upload time.

---

## MAC addresses to update

If integrating with the Enterprise-D system, open `WarpCore_ESP32.ino` and update the peer MAC arrays to match your hardware:

```cpp
uint8_t dataPadAddress[] = {0x20, 0x6e, 0xf1, 0xa9, 0xa1, 0x14};  // DataPad
uint8_t bridgeAddress[]  = {0xe0, 0x72, 0xa1, 0xd7, 0x37, 0x14};  // Bridge
uint8_t engRoomAddress[] = {0x3c, 0xdc, 0x75, 0xae, 0xcd, 0xb8};  // EngRoom
```

Run `MAC_address_retriver/` (from the main repo) on each board to read its MAC.

---

## How it works

### Standalone mode

On boot, if `on_default` is true in NVS (default), the chase effect starts immediately. Speed is read from the potentiometer on A7 — turn it to adjust the chase rate. No DataPad or other hardware needed.

### Chase effect

10 LEDs chase in sequence. When the last LED turns off, the ambient LED (A5) pulses to full brightness, holds briefly, dims back down, then the chase restarts. Speed maps pot/remote value 0–10 to a 200ms–20ms step delay.

### ESP-NOW integration (optional)

When connected to the Enterprise-D system the DataPad can:
- Turn the warp core on/off (`LED_ON` / `LED_OFF` / `LED_STARTUP` / `LED_SHUTDOWN`)
- Set warp speed 1–10 (`LED_WARP`) — overrides the pot while active, returns to pot on `LED_WARP val=0`
- Enable/disable the presence sensor (`LED_RADAR_EN`)

### Presence sensor (LD2410C)

When presence is detected on D7 and `radarEnabled` is true, WarpCore sends `LED_RADAR_TRIG` to the DataPad. DataPad powers up the full Enterprise model if it is off, or triggers red alert if it is already on.

Detection range is configured via the HLKRadarTool app over Bluetooth or UART. The current build has only the OUT pin wired (D7) — UART is not yet connected, so sensitivity must be configured via the Bluetooth app.

---

## NVS settings

Namespace: `"warpcore"`.

| Key | Default | Purpose |
|-----|---------|---------|
| `on_default` | true | Run chase effect on boot without waiting for DataPad |
| `radar_on` | true | LD2410C presence trigger enabled |
| `wifi_auto` | false | Auto-connect to saved WiFi on boot |
