# Sound Map — Bridge DY-SV17F & DataPad DY1703A

## Overview

Two separate sound players, two separate SD cards. File numbers 1–25 are identical on both
cards (same sounds, same slots) so BRIDGE/PAD/BOTH destination mode needs no translation.
Files 26+ exist on the DataPad card only.

| Player | Unit | UART | Baud | Storage | Files |
|--------|------|------|------|---------|-------|
| DY-SV17F | Bridge ESP32-S3 | UART2 TX=GPIO17 RX=GPIO18 | 9600 | 4MB flash | 00001–00025 |
| DY1703A | DataPad ESP32-S3 | UART1 TX=GPIO16 RX=GPIO15 | 9600 | SD card | 00001–00042 |

Default volume: 15 / 30 on both players.

---

## Sound Sources

**Star Trek sounds (files 01–42):** All TNG audio clips sourced from [TrekCore Audio Archive](https://www.trekcore.com/audio/). The site has a much larger selection than what's used here — the files listed below are just what worked for this build. You can substitute any clips you prefer, just keep in mind which slots are auto-triggered by firmware effects (startup, shutdown, damage) versus which are manual-only buttons on the DataPad. Those distinctions are noted in the trigger map and button table below. As long as the slot numbers match, the content is up to you.

**Touch sounds (files 26–27):** Generic UI click sounds — any two short click/beep sounds will work.

**Star Wars easter egg sounds (files 43–44):** Not included in this repository. Source from your own soundtrack or digital purchase. Files 43 and 44 can be any two audio files — the button code expects them at those slot numbers regardless of content.

---

## Bridge File Map (DY-SV17F) — files 01–25

Ready-to-copy files: `sounds/DY_Player/` — copy all 25 files to the root of the Bridge player flash.
Total size: ~3.9MB (within 4MB flash limit).

> If the player rejects files, remove `00003.mp3` (tng_poweringdown, 402KB) first —
> it is the least essential file and freeing it drops total size to ~3.5MB.

### Power — files 01–03
Auto-trigger: **startup always plays file 01**; **shutdown plays random 02–03**

| File | Original filename | Description |
|------|------------------|-------------|
| 00001.mp3 | tng_poweringup.mp3 | Power-up / systems online — startup trigger |
| 00002.mp3 | ship_shutdown.mp3 | Ship shutdown sound |
| 00003.mp3 | tng_poweringdown.mp3 | Power-down sequence |

### Alerts — files 04–06
Auto-trigger: damage sequence; manual ALERTS button on DataPad

| File | Original filename | Description |
|------|------------------|-------------|
| 00004.mp3 | alert08.mp3 | Alert beep — very short |
| 00005.mp3 | alert01.mp3 | Alert tone 1 — short |
| 00006.mp3 | alertklaxon_clean2.mp3 | Red alert klaxon — long, cut off at 2.5s |

> Files 04 and 06 are used as the damage alert (randomly chosen). File 05 is manual only.

### Damage — files 07–12
Auto-trigger: **random from 07–12** after the alert klaxon

| File | Original filename | Description |
|------|------------------|-------------|
| 00007.mp3 | largeexplosion1.mp3 | Large explosion hit |
| 00008.mp3 | largeexplosion3.mp3 | Large explosion hit 2 |
| 00009.mp3 | shield_sizzle.mp3 | Shield impact / sizzle |
| 00010.mp3 | smallexplosion2.mp3 | Small explosion |
| 00011.mp3 | starboardnacellenotfunctional_ep.mp3 | Voiced: "Starboard nacelle not functional" |
| 00012.mp3 | tng_powerloss.mp3 | Power loss sound |

### Warp — files 13–18
Manual only — WARP button on DataPad cycles through these

| File | Original filename | Description |
|------|------------------|-------------|
| 00013.mp3 | tng_flyby2.mp3 | Ship flyby whoosh |
| 00014.mp3 | tng_warp4_clean.mp3 | Warp engage — warp 4 |
| 00015.mp3 | tng_warp7.mp3 | Warp engage — warp 7 |
| 00016.mp3 | tng_warp_exit.mp3 | Warp exit / drop out |
| 00017.mp3 | tng_warp_flash.mp3 | Warp flash effect |
| 00018.mp3 | tng_warp_out2.mp3 | Warp out |

### Weapons — files 19–25
Manual only — WEAPONS button on DataPad cycles through these

| File | Original filename | Description |
|------|------------------|-------------|
| 00019.mp3 | tng_fireallweapons_ep.mp3 | Voiced: "Fire all weapons" |
| 00020.mp3 | tng_phaser_clean.mp3 | Phaser fire |
| 00021.mp3 | tng_phaser_strike.mp3 | Phaser strike impact |
| 00022.mp3 | tng_torpedo2_clean.mp3 | Photon torpedo 2 |
| 00023.mp3 | tng_torpedo_clean.mp3 | Photon torpedo |
| 00024.mp3 | tng_weapons2_clean.mp3 | Weapons fire 2 |
| 00025.mp3 | tng_weapons_clean.mp3 | Weapons fire |

---

## DataPad-Only File Map (DY1703A) — files 26–42

Files 01–25 on the DataPad SD card are identical copies of the Bridge files above.
Files 26–42 are DataPad-only — never sent to the Bridge via ESP-NOW.

Source files are in the `sounds/` subfolders listed below.
Name them `00026.mp3`, `00027.mp3` etc. when copying to the DataPad SD card.

### Touch feedback — files 26–27
Played locally on button press. DataPad only.

| File | Source file | Description |
|------|-------------|-------------|
| 00026.mp3 | `sounds/Data Pad sounds/padd_1.mp3` | UI touch sound 1 |
| 00027.mp3 | `sounds/Data Pad sounds/padd_2.mp3` | UI touch sound 2 |

### Voiced alerts — files 28–31
Richer alert lines with voice. DataPad only.

| File | Source file | Description |
|------|-------------|-------------|
| 00028.mp3 | `sounds/alerts with voices/tng_red_alert1.mp3` | Red alert with voiced klaxon |
| 00029.mp3 | `sounds/alerts with voices/lifesupportfailurealldecksabandonship_ep.mp3` | "Life support failure, all decks abandon ship" |
| 00030.mp3 | `sounds/alerts with voices/warningprimaryshieldsfailing_ep.mp3` | "Warning: primary shields failing" |
| 00031.mp3 | `sounds/alerts with voices/warningstructuralintegrity_ep.mp3` | "Warning: structural integrity..." |

### Ambient — files 32–38
Background loop sounds. DataPad only.

| File | Source file | Description |
|------|-------------|-------------|
| 00032.mp3 | `sounds/ambient sounds/ambient_bridge_1.mp3` | Bridge ambience 1 |
| 00033.mp3 | `sounds/ambient sounds/tng_bridge_1.mp3` | Bridge ambience 2 |
| 00034.mp3 | `sounds/ambient sounds/tng_bridge_2.mp3` | Bridge ambience 3 |
| 00035.mp3 | `sounds/ambient sounds/tng_bridge_3.mp3` | Bridge ambience 4 |
| 00036.mp3 | `sounds/ambient sounds/engineering_clean.mp3` | Engineering ambience |
| 00037.mp3 | `sounds/ambient sounds/tng_engine_1.mp3` | Engine room 1 |
| 00038.mp3 | `sounds/ambient sounds/tng_engine_2.mp3` | Engine room 2 |

### Easter eggs — files 39–42
DataPad only.

| File | Source file | Description |
|------|-------------|-------------|
| 00039.mp3 | `sounds/Funny add in joke to play/selfdestructsequenceinitiatedwarpcorebreach_ep.mp3` | "Self destruct sequence initiated, warp core breach" |
| 00040.mp3 | `sounds/Funny add in joke to play/selfdestructsequenceterminated_ep.mp3` | "Self destruct sequence terminated" |
| 00041.mp3 | `sounds/Funny add in joke to play/unabletocomply.mp3` | "Unable to comply" |
| 00042.mp3 | `sounds/Funny add in joke to play/youarenotauthorisedtoaccessthisfacility_clean.mp3` | "You are not authorised to access this facility" |
| 00043.mp3 | *(user-supplied — not included in repo)* | Star Wars main theme |
| 00044.mp3 | *(user-supplied — not included in repo)* | Imperial March (Darth Vader's Theme) |

---

## Bridge Trigger Map

| Event | Behaviour | Code location |
|-------|-----------|---------------|
| Startup sequence begins | Always plays file 01 (tng_poweringup) | `Bridge_ESP.ino` — `LED_STARTUP` case |
| Shutdown sequence begins | Random file 02 or 03 | `Bridge_ESP.ino` — `LED_SHUTDOWN` case |
| Damage effect starts | Random file 04 or 06 (alert) for 2.5s, then random file 07–12 | `Bridge_ESP.ino` — `initElecShort()` + `updateElecShort()` |
| Damage cancelled/expired | Sound stops | `Bridge_ESP.ino` — `stopElecShort()` |

---

## DataPad Sound Screen Buttons

Each button cycles through its category. Label updates to show `CATEGORY\nN / total` on each press.
Destination (BRIDGE / PAD / BOTH) set via toggle on Screen2.

| Button | Category | File range | Destination |
|--------|----------|-----------|-------------|
| POWER | Power sounds | 01–03 | BRIDGE / PAD / BOTH |
| ALERTS | Alert tones | 04–06 | BRIDGE / PAD / BOTH |
| DAMAGE | Damage effects | 07–12 | BRIDGE / PAD / BOTH |
| WARP | Warp sounds | 13–18 | BRIDGE / PAD / BOTH |
| WEAPONS | Weapon effects | 19–25 | BRIDGE / PAD / BOTH |
| STOP | — | — | BRIDGE / PAD / BOTH |
| Volume slider | — | 0–30 | Bridge only (DataPad volume set separately on Screen2) |
| REPEAT | — | off / one / all | Bridge only |

---

## Code Constants

### Bridge (Bridge_ESP.ino)

```cpp
#define SND_POWERUP_FILE      1
#define SND_SHUTDOWN_FIRST    2
#define SND_SHUTDOWN_LAST     3
#define SND_DMG_ALERT_1       4
#define SND_DMG_ALERT_2       6
#define DMG_ALERT_MS       2500
#define SND_ALERT_FIRST       4
#define SND_ALERT_LAST        6
#define SND_DAMAGE_FIRST      7
#define SND_DAMAGE_LAST      12
#define SND_WARP_FIRST       13
#define SND_WARP_LAST        18
#define SND_WEAPONS_FIRST    19
#define SND_WEAPONS_LAST     25
```

### DataPad (Data_Pad.ino) — shared range

```cpp
// Files 01–25 match Bridge exactly
#define SND_POWER_FIRST       1
#define SND_POWER_LAST        3
#define SND_ALERT_FIRST       4
#define SND_ALERT_LAST        6
#define SND_DAMAGE_FIRST      7
#define SND_DAMAGE_LAST      12
#define SND_WARP_FIRST       13
#define SND_WARP_LAST        18
#define SND_WEAPONS_FIRST    19
#define SND_WEAPONS_LAST     25
```

### DataPad (Data_Pad.ino) — DataPad-only range

```cpp
#define SND_TOUCH_FIRST      26
#define SND_TOUCH_LAST       27
#define SND_VOICED_FIRST     28
#define SND_VOICED_LAST      31
#define SND_AMBIENT_FIRST    32
#define SND_AMBIENT_LAST     38
#define SND_EASTER_FIRST     39
#define SND_EASTER_LAST      42
```

---

## How to Load the Bridge DY-SV17F

1. Connect the player via USB (appears as USB mass storage) or use UART flash tool
2. Copy all 25 files from `sounds/DY_Player/` to the root of the player flash
3. Files must be named exactly `00001.mp3`–`00025.mp3` — no folders, no other names
4. Eject and power cycle — player is ready

## How to Prepare the DataPad SD Card

1. Copy all 25 files from `sounds/DY_Player/` to the root of the SD card (same names)
2. Rename and copy each DataPad-only file according to the table above (26–42) — source Star Trek clips from trekcore.com/audio/
3. Optionally add files 00043.mp3 and 00044.mp3 for the Star Wars easter egg buttons — source from your own copy; any two audio files will work at those slots
4. Files must be in the root — no folders
5. Insert SD card into the DY1703A module
