# Batch_Bootstrap

Bench-only firmware for setting up Bridge + EngRoom boards in batches. Flash it once by USB onto each new board; after that the board pairs itself with its partner and installs its real firmware ([`Bridge_ESP_Webpage`](../Bridge_ESP_Webpage) / [`Engine_Room_ESP_Webpage`](../Engine_Room_ESP_Webpage)) over WiFi. Never shipped on a finished board — it replaces itself.

## Before flashing — edit the config block at the top

```cpp
#define EXPECTED_CHIP   "ESP32-S3"     // "ESP32-S3" for Bridge boards, "ESP32-C3" for EngRoom boards
#define BOARD_ROLE      ROLE_BRIDGE    // ROLE_BRIDGE or ROLE_ENGROOM
#define BOOTSTRAP_WIFI_SSID   "EDIT_BEFORE_FLASHING_SSID"
#define BOOTSTRAP_WIFI_PASS   "EDIT_BEFORE_FLASHING_PASS"
#define OTA_SERVER_HOST       "http://192.168.50.1:8000"
#define STATUS_LED_PIN         48      // 48 = S3 dev board NeoPixel; -1 for the C3
```

- The WiFi SSID/password must exactly match `Pi_Bench_Tools/bootstrap-ap/bootstrap-ap.env`.
- `OTA_SERVER_HOST` is the Pi's address on that bench network (the default matches the bench AP's default gateway).
- Build once per role: one build for all the S3s (Bridge), one for all the C3s (EngRoom).

## What it does

1. **Checks the chip** — if an S3 build lands on a C3 (or vice versa) it halts with a repeating Serial message and 5 LED blinks, instead of pairing the wrong board. An invalid `BOARD_ROLE` halts with 3 blinks.
2. **Joins the bench WiFi**, retrying forever until it's up.
3. **Pairs** — broadcasts its role over ESP-NOW every 2s and adopts the first board of the opposite role it hears. The partner's MAC is written to NVS (read back to verify) under the same keys the real firmware reads at boot, so no MAC editing is ever needed.
4. **Downloads the real firmware** from the Pi 5s after pairing (`Bridge_ESP.bin` or `Engine_Room_ESP.bin`), one retry if it fails, then reboots into it. Two failures → 4 LED blinks, and the board keeps running the bootstrap so you can retry.
5. **Reports each step to the Pi's dashboard** (joined, paired, flashed / failed with the reason). These reports are best-effort — if the dashboard isn't running, pairing and flashing still work.

The last thing the Pi can see is "flashed" (image downloaded, verified and written). The real firmware never joins the bench network, so check the finished pair by joining Bridge's own `Enterprise D` AP.

## Status page and Serial

- Each board serves a plain status page at its bench-network IP (shown on Serial and on the dashboard) with a **Retry OTA Pull** button.
- Serial at 115200 prints a heartbeat every 10s. Send `CLEARMAC` to forget the stored partner and pair again.
