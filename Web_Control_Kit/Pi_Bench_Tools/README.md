# Pi_Bench_Tools

Raspberry Pi side of the batch setup bench. Tested on a Pi 4B with Raspberry Pi OS Lite (Debian 13 trixie, NetworkManager). Plug the Pi into your home network by **Ethernet** — its WiFi is used to host the bench network, so you reach the Pi and its dashboard over Ethernet.

Everything lives under `~/bench/` on the Pi:

```
~/bench/OTA_Server/        bin_server.py, bin-server.service, bins/
~/bench/Pi_Bench_Tools/    bootstrap-ap/
~/bench/Sound_Loader/      contents of sound-loader/, plus sounds/
```

## 1. Bench WiFi network — [`bootstrap-ap/`](bootstrap-ap)

An isolated WiFi network (no internet, no gateway) that `Batch_Bootstrap` boards join. Edit `bootstrap-ap.env` first, then `sudo ./setup-bootstrap-ap.sh`. Full details in that folder's README.

## 2. Firmware server + dashboard — [`../OTA_Server/`](../OTA_Server)

`bin_server.py` (Python standard library only) does three things on port 8000:

- Serves `bins/Bridge_ESP.bin` and `bins/Engine_Room_ESP.bin` to bootstrap boards. Export these from the Arduino IDE (**Sketch → Export Compiled Binary**) for `Bridge_ESP_Webpage` and `Engine_Room_ESP_Webpage`, and rename them to those two names. Every download is logged to `mac_log.csv` with the board's MAC.
- Receives each board's progress reports and keeps them in `events.csv`.
- Shows the dashboard at **`http://<pi-ethernet-address>:8000/status`** — one card per Bridge + EngRoom pair with each step's time, a green PASS once both boards have flashed and each knows the other's MAC, or red FAIL with the reason. **Start new batch** archives the list with a timestamp.

Run it as a service:

```bash
sudo cp ~/bench/OTA_Server/bin-server.service /etc/systemd/system/
sudo systemctl enable --now bin-server
```

## 3. Sound module loader — [`sound-loader/`](sound-loader)

Plug a Bridge's DY-SV17F sound module into the Pi's USB and it's loaded automatically:

1. A udev rule recognises the module by its own USB identity (`uart mp3 player`, whole-disk FAT) and starts `sound-loader@<device>.service`.
2. `load_sounds.sh` refuses anything larger than 8MB (so a USB stick or other drive is never touched), mounts the module, **deletes everything on it**, copies the 25 files in order, checks every file against the source, then unmounts.
3. The dashboard shows *Copying sounds 12/25* … then **DONE — safe to remove**, or FAILED with the reason. About 2 minutes per module — the module's flash is slow.

The module is 4MB and the 25 files fill almost all of it, and the player plays files in the order they were written — that's why it's wiped and loaded fresh every time.

**Sound files are not included in this repo.** Put your own 25 files, named `00001.mp3` … `00025.mp3` per [`SOUND_MAP.md`](../../SOUND_MAP.md), in `~/bench/Sound_Loader/sounds/`.

Install once:

```bash
cd ~/bench/Sound_Loader && sudo ./install.sh
```
