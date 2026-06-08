# USS Enterprise-D Prop Controller

Custom lighting and sound controller for the DeAgostini USS Enterprise-D partwork model kit (31 issues). Replaces the original lighting PCBs with custom ESP32-based boards controlled from a 3D-printed LCARS PADD touchscreen.

![USS Enterprise-D prop model](<Hero Shot.jpg>)

## What it does

- **LCARS DataPad** — 7" touchscreen prop running a full LVGL LCARS interface. Controls all lighting, sound, and settings on both model sections wirelessly.
- **ESP-NOW mesh** — Bridge (saucer) and EngRoom (stardrive) communicate without any router or hub. Normal operation requires no WiFi.
- **Coordinated effects** — startup, shutdown, damage, warp, and navigation sequences across 18 LED groups on the saucer and 10 groups on the stardrive, all triggered from the PADD.
- **Sound** — DY-SV17F player on the Bridge drives 25 TNG sound files. DataPad has its own second player for UI sounds and ambient audio.
- **OTA updates** — all three units flash over WiFi via Arduino IDE. WiFi only starts when you need it; ESP-NOW resumes automatically when it disconnects.

All LEDs are white. All effects are brightness, PWM duty cycle, and timing patterns only — no colour control.

---

## Units

| Unit | Board | Sketch | Role |
|------|-------|--------|------|
| Bridge | ESP32-S3 | `Bridge_ESP/` | Saucer — 18 LED groups, sound |
| EngRoom | Xiao ESP32-C3 | `Engine_Room_ESP/` | Stardrive — 10 LED groups |
| DataPad 7" | Waveshare ESP32-S3 7" | `Data_Pad/` | LCARS touchscreen controller |
| DataPad 2.8" | Waveshare ESP32-S3 2.8" | `Data_Pad_240x320/` | Smaller PADD variant |
| WarpCore | Arduino Nano ESP32 | `WarpCore_ESP32/` | Standalone warp core display (see note below) |

---

## Hardware

Six custom PCB designs (seven boards total), all designed in EasyEDA. Gerbers, BOMs, and pick-and-place files are in `enterprise documentation/PCB Design/`.

See `HARDWARE.md` for pinouts, resistor values, power budget, battery configuration, and connector maps.

Parts list with Amazon links: `amazon parts.txt`.

---

## Getting started

### Prerequisites

- Arduino IDE 2.x
- esp32 by Espressif Systems **v3.3.8** (Board Manager)
- Set your Arduino sketchbook path to this folder — all libraries are in `libraries/`

### First flash (USB)

Flash in this order so ESP-NOW communication works from the start:

1. **Bridge** — see [Bridge_ESP/README.md](Bridge_ESP/README.md)
2. **EngRoom** — see [Engine_Room_ESP/README.md](Engine_Room_ESP/README.md)
3. **DataPad** — see [Data_Pad/README.md](Data_Pad/README.md)

Before flashing: run `MAC_address_retriver/` on each board to get its MAC address, then update the peer MAC arrays in each sketch to match your hardware.

### OTA updates (after initial flash)

Normal operation: WiFi off, ESP-NOW on channel 1.

1. Press **START WIFI** on the DataPad — pushes your saved WiFi credentials to all units, all connect.
2. Arduino IDE → Tools → Port → select the network port for the unit to flash.
3. Upload. OTA password: **admin** (enter in Arduino IDE at upload time — no `setPassword()` call in the sketch).
4. Press **SAVE WIFI** again on the DataPad → all units disconnect, revert to channel 1.

> Use a **phone hotspot**, not a home mesh/multi-AP network. Units on different channels cannot reach each other via ESP-NOW and silently fail. See `DEVNOTES.md` for details.

---

## Repository contents

| Path | Contents |
|------|----------|
| `Bridge_ESP/` | Saucer ESP32-S3 sketch |
| `Engine_Room_ESP/` | Stardrive Xiao ESP32-C3 sketch |
| `Data_Pad/` | 7" DataPad sketch and LVGL UI files |
| `Data_Pad_240x320/` | 2.8" DataPad variant |
| `WarpCore_ESP32/` | WarpCore ESP32 sketch |
| `Bridge_NVS_Clear/` | Utility — wipes Bridge WiFi credentials from NVS |
| `EngRoom_NVS_Clear/` | Utility — wipes EngRoom WiFi credentials from NVS |
| `MAC_address_retriver/` | Utility — reads and prints a board's MAC address |
| `enterprise documentation/PCB Design/` | Gerbers, BOMs, pick-and-place for all 6 board designs |
| `enterprise documentation/Data Pad stl/` | DataPad PADD case STL files |
| `enterprise documentation/Warp_Core/` | Original Thingiverse WarpCore design archive (see below) |
| `enterprise documentation/pcbs test prints/` | PCB test-fit prints — print before ordering boards |
| `HARDWARE.md` | Full hardware reference |
| `DEVNOTES.md` | Session logs, build history, fix queue, pending work |
| `SOUND_MAP.md` | Sound file list and trigger map |
| `amazon parts.txt` | Parts list with Amazon links |

**Not included:**
- `sounds/` — source TNG audio clips from [trekcore.com/audio](https://trekcore.com/audio/). See `SOUND_MAP.md` for the file list and naming format.
- `libraries/` — install per `PROJECT_REFERENCE.md`.

---

## WarpCore

The WarpCore is a standalone unit with its own repository — [Enterprise-D-WarpCore](https://github.com/badbooger/Enterprise-D-WarpCore) — `WarpCore_ESP32/` is included here for reference. See the WarpCore repo for the full README, pin assignments, and build notes.

The 3D model is based on **ST:TNG Warp Core by ElmoC** (Thingiverse thing:1656741). The original listing is no longer available online. A complete archive including all STL files, the original PCB files, and the original Arduino sketch is preserved in `enterprise documentation/Warp_Core/`. Licensed under Creative Commons Attribution — Non-Commercial — No Derivatives 3.0.

Other warp core designs exist on Thingiverse in different sizes, including versions designed around ARGB LED strips which give a colour-animated effect. Worth exploring if you are starting from scratch.

---

## License

Code and PCB designs: [MIT](LICENSE)  
WarpCore 3D model: CC BY-NC-ND 3.0 (ElmoC — see `enterprise documentation/Warp_Core/LICENSE.txt`)  
Sound files: not included — source separately from trekcore.com/audio/
