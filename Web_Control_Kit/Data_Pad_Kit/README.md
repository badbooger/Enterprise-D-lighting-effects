# Data_Pad_Kit — 7" DataPad for the Web Control Kit

> **⚠️ Work in progress.** A version of the [`Data_Pad`](../../Data_Pad) sketch made to pair with the [`Bridge_ESP_Webpage`](../Bridge_ESP_Webpage) / [`Engine_Room_ESP_Webpage`](../Engine_Room_ESP_Webpage) firmware. The top-level `Data_Pad` is the original build and is unchanged.

Runs on the Waveshare ESP32-S3-Touch-LCD-7 (800×480) with a DY1703A sound player on the SD card.

## Differences from the top-level Data_Pad

- **Pairing announce:** for the first 60s after every boot the DataPad broadcasts a pairing announce. Press **Pair New DataPad** on the Bridge control page, then reset the DataPad within that minute — Bridge and EngRoom save its MAC. The DataPad itself still needs the Bridge/EngRoom MACs compiled in (below).
- **Torpedoes:** separate **AFT PHOTON** button (for EngRoom boards with the aft photon LED) and **FIRE ALL WEAPONS** (forward + aft + weapons sound), matching the control page.
- **Warp:** the warp slider is replaced by an **ENGAGE WARP** toggle (warp speed set by `WARP_BTN_SPEED`, default 1, same as the control page).
- **Red / yellow alert** also play an alert tone on the saucer (Bridge file 06 for red, 05 for yellow — the DataPad's voiced alert, file 28, only exists on its own SD card). Skipped when the sound destination is set to PAD.
- **Easter-egg sounds** stop at file 42 (no files 43–44 needed on the SD card).

## Arduino IDE setup

| Setting | Value |
|---------|-------|
| Board | Waveshare ESP32-S3-Touch-LCD-7 (or ESP32S3 Dev Module) |
| Board package | esp32 by Espressif Systems v3.3.8 (3.3.11 also tested) |
| Flash size | 16MB |
| Partition scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload speed | 921600 |

Libraries are the same as the top-level [`Data_Pad`](../../Data_Pad) (LVGL **8.3.11** — not 9.x, ESP32_Display_Panel 1.0.0, ESP32_IO_Expander, esp-lib-utils).

Flash over the board's **UART** USB-C port — the other port gives no serial connection for uploading. Wrong partition or PSRAM settings = the board won't boot.

`arduino-cli` equivalent:

```
arduino-cli compile --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=921600" Data_Pad_Kit
```

## MAC addresses to set

Near the top of `Data_Pad_Kit.ino`:

```cpp
uint8_t broadcastAddress1[]        = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // EngRoom
uint8_t broadcastAddress2[]        = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Bridge
uint8_t broadcastAddressWarpCore[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // WarpCore
```

Get your Bridge and EngRoom MACs from the Bridge control page (**Show MAC Addresses**), or from each board's Serial output at boot. Leave WarpCore at zeros if you don't have one.

## SD card

8GB FAT32, files `00001.mp3`–`00042.mp3` in the card root, **copied in number order** (the player plays in write order). Files 01–25 are the same as the Bridge sound module; 26–42 are DataPad-only — see [`SOUND_MAP.md`](../../SOUND_MAP.md).
