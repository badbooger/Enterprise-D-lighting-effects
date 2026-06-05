# EngRoom_NVS_Clear

Utility sketch for the EngRoom Xiao ESP32-C3. Clears the `wifi_auto` flag and saved WiFi credentials from NVS, then connects to WiFi and starts ArduinoOTA so the main `Engine_Room_ESP` sketch can be flashed back without USB access.

Use this when EngRoom is stuck trying to auto-connect to WiFi on every boot (symptom: long delay before boot indicator blinks, DataPad shows EngRoom as offline).

---

## When to use

- EngRoom takes a long time to become responsive after powering on
- EngRoom is not appearing on the DataPad after boot
- A previous OTA session was not cleanly closed (SaveWifi was not pressed a second time to revert)

---

## How to use

**1. Set credentials**

Open `EngRoom_NVS_Clear.ino` and set your WiFi network at the top:

```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";
```

**2. Flash this sketch**

Flash via the normal OTA procedure (DataPad → SaveWifi → upload to `EngRoom.local` in Arduino IDE). If EngRoom is completely unreachable, USB is the fallback — the Xiao ESP32-C3 has its own onboard USB-C connector. On first USB flash the Xiao may need download mode: hold BOOT, tap RESET, release BOOT.

**3. Wait for WiFi**

Watch Serial Monitor at 115200 baud. You will see:

```
EngRoom NVS cleared.
Connecting to WiFi......
Connected. IP: 192.168.x.x
mDNS: EngRoom.local
Ready for OTA — upload Engine_Room_ESP now.
```

**4. Flash Engine_Room_ESP back**

In Arduino IDE select `Tools > Port > EngRoom.local` and upload `Engine_Room_ESP`.

---

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | XIAO_ESP32C3 |
| Board package | esp32 by Espressif Systems v3.3.8 |
| Upload speed | 921600 |

OTA password: `admin` — entered in Arduino IDE upload config, no `setPassword()` call in the sketch.

---

## What gets cleared

| NVS key | Action |
|---------|--------|
| `wifi_auto` | Set to `false` |
| `wifi_ssid` | Removed |
| `wifi_pass` | Removed |

All other NVS keys (timing settings, sync mode, assembly mode, etc.) are left untouched.
