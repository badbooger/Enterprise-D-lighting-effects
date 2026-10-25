# Web Control Kit

> **⚠️ Work in progress.** A separate version of the Bridge and EngRoom firmware, plus the bench tooling used to set up boards in batches. The main sketches at the top of this repo ([`Bridge_ESP`](../Bridge_ESP), [`Engine_Room_ESP`](../Engine_Room_ESP), [`Data_Pad`](../Data_Pad)) are the original DataPad-driven build and are unaffected by anything in this folder.

The idea: run the model with **no DataPad and no home network**. Bridge hosts its own WiFi access point with a built-in LCARS control page, EngRoom joins it automatically, and everything is controlled from a phone or laptop browser.

## What's in here

| Folder | What it is |
|--------|-----------|
| [`Bridge_ESP_Webpage/`](Bridge_ESP_Webpage) | Bridge (ESP32-S3) firmware with its own WiFi AP and control page, demo mode, browser firmware update, 3-power-cycle assembly toggle |
| [`Engine_Room_ESP_Webpage/`](Engine_Room_ESP_Webpage) | EngRoom (Xiao ESP32-C3) firmware to pair with it — joins Bridge's AP, aft photon torpedo support, fallback recovery AP |
| [`Data_Pad_Kit/`](Data_Pad_Kit) | Optional 7" DataPad (Waveshare ESP32-S3) firmware to pair with the two `_Webpage` sketches — Pair New DataPad support, aft photon / fire all buttons, warp button |
| [`Batch_Bootstrap/`](Batch_Bootstrap) | Bench-only firmware for setting up boards in batches: pairs a Bridge + EngRoom by itself, then pulls the real firmware over WiFi |
| [`OTA_Server/`](OTA_Server) | Raspberry Pi server that hands out the firmware `.bin` files and shows a live dashboard of each pair's progress |
| [`Pi_Bench_Tools/`](Pi_Bench_Tools) | Pi setup: the isolated bench WiFi network, and a USB auto-loader for the Bridge's DY-SV17F sound module |

**Just want to run one model?** You only need the two `_Webpage` sketches — flash them by USB like the main sketches (see their READMEs). Add `Data_Pad_Kit` if you want a touchscreen too. Everything else here is for setting up several boards at once.

## Batch setup in short

1. A Raspberry Pi hosts a private bench WiFi network (`Pi_Bench_Tools/bootstrap-ap`) and runs `OTA_Server/bin_server.py`, serving the compiled `_Webpage` firmware.
2. Each new board gets `Batch_Bootstrap` by USB once — S3 boards as Bridge, C3 boards as EngRoom.
3. Power a Bridge and an EngRoom up together: they join the bench network, find each other over ESP-NOW, save each other's MAC, then download and install their real firmware.
4. Watch it happen on the Pi's dashboard (`http://<pi-address>:8000/status`): Joined → Paired → Flashed for each board, then PASS for the pair.
5. Plug each DY-SV17F sound module into the Pi's USB — it's cleared, loaded with the 25 sound files and verified automatically, with progress on the same dashboard.
6. Pack each kit against the checklist at `http://<pi-address>:8000/pack` (works well on a tablet). Tick items as they go in the box; "Kit packed" logs the kit and resets the list. The items come from `OTA_Server/packing_list.json` — edit it to match your own kit.
