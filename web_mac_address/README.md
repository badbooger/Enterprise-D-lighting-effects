# Web MAC Address Retriever

> **Safety Warning:** These PCBs are experimental, hand-designed hobby boards — not certified consumer products. If you are building with LiPo or Li-ion batteries, take proper precautions: never leave charging batteries unattended, use appropriate cell protection, and verify polarity before connecting power. Incorrect wiring or battery handling can cause fire, burns, or damage to components. Proceed at your own risk.

---

## What This Does

This sketch comes preloaded on your ESP32 boards. When you power a board on for the first time, it starts a local WiFi access point — no internet connection or router needed. Connect to it with your phone or laptop and the board's MAC address is displayed on a webpage that opens automatically.

The boards in this project communicate with each other using ESP-NOW, a direct wireless protocol that requires each board to know the MAC addresses of its peers. Before you can flash the main sketches, you need to collect the MAC address from every board and enter them into the code.

**WiFi access point details:**
- SSID: `enterprise`
- Password: `ncc-1701-d`
- IP: `192.168.4.1`

This is a standalone access point — your phone/laptop connects directly to the ESP32. It does not connect to your home network or the internet.

**Built-in OTA (Over-The-Air) updates:** After the initial USB flash, this sketch can be updated wirelessly. If you connect your computer to the board's WiFi access point, the Arduino IDE will show `web_mac_address` as a network port under **Tools > Port**. You can upload updated sketches over WiFi without plugging in a USB cable. This also applies to the main project sketches once they are flashed — they all support OTA updates.

The MAC address is also printed to the serial monitor at 115200 baud if you prefer to read it that way.

## How to Use

1. **Open** `web_mac_address.ino` in the Arduino IDE.
2. **Select your board** in the IDE (e.g. ESP32-S3 for the Bridge, XIAO ESP32-C3 for EngRoom, etc.).
3. **Upload** via USB.
4. **Connect** your phone or laptop to the WiFi network:
   - SSID: `enterprise`
   - Password: `ncc-1701-d`
5. A captive portal page should **open automatically** showing the board's MAC address. If it doesn't, open a browser and go to `http://192.168.4.1`.
6. **Write down the MAC address** — you'll need it when setting up the main sketches.
7. **Repeat** for each board in the project.

## OTA Updates

After the first USB flash, you can update this sketch over WiFi:

1. Connect your computer to the `enterprise` WiFi network.
2. In Arduino IDE, go to **Tools > Port** and select `web_mac_address` from the network ports list.
3. Upload as normal.

## Next Steps

Once you have all your MAC addresses, head to the main repository for everything else — sketch documentation, GPIO pin assignments, board settings, wiring references, and assembly notes:

**https://github.com/badbooger/Enterprise-D-lighting-effects**

Each sketch folder in the repo has its own README with the specific setup instructions for that unit.
