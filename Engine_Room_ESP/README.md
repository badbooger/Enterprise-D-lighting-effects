# Engine Room ESP — Xiao ESP32-C3

Stardrive section controller. Drives 9 LED groups through NPN transistors: nacelles (sine-wave pulse with warp mode), deflector, photon torpedoes, impulse engines, nav lights, engineering case sections, and neck (back of neck + both battle bridges on one pin). Communicates with Bridge and the DataPad via ESP-NOW. Supports OTA firmware updates and mDNS (`EngRoom.local`) when WiFi is connected.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | XIAO_ESP32C3 |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Upload speed | 921600 |

No external libraries needed — all dependencies are part of the ESP32 Arduino core.

---

## PCB and module assembly

The Seeed Studio Xiao ESP32-C3 must be soldered to the EngRoom PCB yourself — JLCPCB does not carry it as a placeable part, so it cannot be installed during PCB assembly. Order the Xiao separately and hand-solder it to the board after the PCB arrives.

Once soldered, the Xiao's own onboard USB-C connector is used for flashing — no other USB port is needed.

---

## First flash

Connect via the Xiao's USB-C port. The Xiao ESP32-C3 may need to be put into download mode on first flash: hold BOOT, tap RESET, release BOOT. Select the correct COM port in Arduino IDE and upload.

After the first USB flash, subsequent updates can be done OTA — see the root README for the OTA procedure.

**OTA password:** `admin` — entered in Arduino IDE at upload time.

---

## MAC addresses to update

Open `Engine_Room_ESP.ino` and find the peer MAC arrays near the top of the file:

```cpp
uint8_t broadcastAddress1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // DataPad
uint8_t broadcastAddress2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Bridge
```

Replace with the actual MACs from your hardware (run `MAC_address_retriver/` on each board).

---

## GPIO assignments

**Lower engineering section:**

| GPIO | Function |
|------|----------|
| 2 | Battery ADC (270kΩ+100kΩ divider, ADC_11db) |
| 4 | Photon torpedoes |
| 5 | Nav lights |
| 6 | Deflector |
| 7 | Top engineering case |
| 8 | Left nacelle |
| 9 | Right nacelle |
| 10 | Impulse engine |
| 21 | Bottom engineering case |

**Neck section** (routed via FFC from lower engineering PCB):

| GPIO | Function |
|------|----------|
| 20 | Neck windows — back of neck + both battle bridges wired together, one pin |

All LED groups are switched through MMBT2222A NPN transistors. Do not connect LEDs directly to GPIO pins.

> **Note on nav lights:** GPIO 5 drives 4 transistor bases in parallel (EngRoom PCB, neck PCB, port nacelle, starboard nacelle) — all nav LEDs across the stardrive section from one pin.

---

## Key effects

**Nacelle pulse (normal)** — slow sine-wave breathing (5000ms period, 80–115 duty range) when model is running.

**Nacelle warp mode** — triggered by `LED_WARP` command with a speed value 1–10. Three-phase startup: snap to 25% → ramp to 100% over ~1.5s → hold at speed-mapped brightness (80–100%). Speed changes while warp is active skip the ramp. Returns to normal sine on `LED_WARP val=0`.

> **Brighter warp flash (optional hardware change):** The nacelle PCB uses 6× blue 1206 SMD LEDs wired as 3 parallel pairs, each pair sharing one current-limiting resistor. Swapping those resistors to a lower value increases brightness. At 7.4V nominal with Vf ≈ 2.6–3.1V, **240Ω** gives ~8.75mA per LED — noticeably brighter than a typical stock value and well within the 20mA LED rating. In practice, brightness between paired LEDs is indistinguishable. Do not go below 220Ω. Any single LEDs on the board (non-paired, through-hole) need a separate higher value — **470Ω** — to match the same per-LED current.

**Deflector** — 60% steady brightness during normal operation, 100% during warp.

**Photon torpedoes** — charge/fire flash sequence on `LED_ELEC_SHORT` (repurposed command). Charge for 750ms, fire flash at 200ms.

**Startup sequence** — lighting groups come on in steps coordinated with the Bridge startup. EngRoom sends `LED_BLINK` to Bridge at the nav step and `LED_ENGINE` at the impulse step so both sections sync their timing.

**Assembly mode** — all groups blink together for board testing and LED positioning.

---

## NVS settings

Namespace: `"engroom"`. All configurable from the DataPad settings screen.

| Key | Default | Purpose |
|-----|---------|---------|
| `blink_ms` | 1000 | Nav light blink interval (ms) |
| `sync_mode` | true | Assembled / Separated mode |
| `asm_mode` | false | Assembly blink mode |
| `eff_simple` | false | Simple effects mode (constant ON instead of sine/pulse) |

---

## NVS clear utility

If EngRoom gets stuck trying to connect to WiFi on every boot: flash `EngRoom_NVS_Clear/EngRoom_NVS_Clear.ino`, confirm "Done" in Serial Monitor (115200), then reflash the main sketch.
