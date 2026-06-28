# Prop Controller — Developer Notes

All three sketches live in this folder. Read this before making changes.

---

## Next Session — Action Required

**Session 46 (2026-06-28). board_utility mDNS + serial WiFi credentials.**

**Changes this session:**

- **`board_utility/board_utility.ino` — mDNS added:** `ESPmDNS` added; board now advertises as `board-utility.local` (HTTP service registered). Accessible via `http://board-utility.local/` on iOS, Mac, Windows. Android does not support `.local` mDNS natively — use IP from serial instead.
- **`board_utility/board_utility.ino` — serial WiFi credentials:** Serial now prints AP SSID, AP password, full IP URL, and mDNS URL on boot — makes connecting easier when captive portal is missed (common on Android).
- **Repo updated:** `board_utility.ino` copied to `Repo output/` and pushed.

**Pending (carried forward):**
- Bridge board swap still pending — old boards in model, new boards on bench.
- Website live at enterprise-d-refit.com (Cloudflare Pages, 2026-06-25). No further deploy action needed.
- Website rewrite for general audience — less spec sheet, more build story (feedback from non-technical visitor, session 45).
- `Data_Pad_28` sketch cleanup — remove all leftover UI and code from when it was developed as the main DataPad controller. Now a pocket remote only; strip back to what's actually used.
- `Data_Pad_28` future — second screen for sound control. Random-by-category is probably the best fit. Also: ESP-NOW button to trigger easter eggs on the 7" DataPad remotely.

---

**Session 45 (2026-06-25). Website live, Save WiFi hint fix, board settings warning, 2.8" audio fix.**

**Website — next session feedback (from non-technical visitor):**
- Landing page doesn't grab attention or explain what the project is for a general audience — rewrite to be fun and accessible, not technical
- Add a "story" page: funny/human write-up of the build process with photos that anyone can follow, not just makers
- General direction: less spec sheet, more "look what this guy built and here's the ridiculous journey to get there"

**Changes this session:**

- **Website deployed:** Live at `enterprise-d-refit.com` and `www.enterprise-d-refit.com`. Hosted on Cloudflare Pages (static upload). DNS moved from GoDaddy to Cloudflare (nameservers changed — domain stays at GoDaddy). `hero.jpg` resized from 8160×3768/4.2MB to 1920×887/242KB. Google Fonts import replaced with local `antonio-regular.woff2` in `lcars.css`. To update site: Cloudflare dashboard → Workers & Pages → long-dawn-d294 → Deployments → Upload assets → drag `site/` folder.
- **Save WiFi hint fix — `Data_Pad` and `Data_Pad_Hero`:** `ui_LabelSaveHint` was declared in `ui_Screen3.c` but missing from `ui_Screen3.h` — added `extern lv_obj_t * ui_LabelSaveHint` to both headers. Font changed from `lv_font_montserrat_10` to `lv_font_montserrat_14` for readability. `Data_Pad.ino` was copied from `Data_Pad_Hero.ino` and MACs/mDNS updated — this resolved a persistent behaviour difference between the two sketches that couldn't be traced to any code difference.
- **Board settings warning:** Added warning to all six sketch READMEs (`Data_Pad`, `Data_Pad_Hero`, `Bridge_ESP`, `Bridge_ESP_Model`, `Engine_Room_ESP`, `WarpCore_ESP32`) — Arduino IDE resets board settings to defaults when a sketch is opened or the `.ino` is replaced, which causes a bad flash requiring USB recovery (and disassembly for installed boards).
- **DEVNOTES updated:** GPIO 9 bodge wire removed from pending — fixed in PCB files. Website deploy item resolved.
- **Battery alert repeated-trigger fix — `Data_Pad` and `Data_Pad_Hero`:** `batWarnFired`/`batCritFired` were shared between Bridge and EngRoom. EngRoom's good battery reading was resetting the shared flag, allowing Bridge's low/miscalibrated reading to re-trigger the alarm every 30 seconds. Fixed by splitting into per-unit flags: `bridgeBatWarnFired`, `bridgeBatCritFired`, `engRoomBatWarnFired`, `engRoomBatCritFired`. Each unit's flag only resets when that unit's own reading improves. Also added `> 0` guard to EngRoom battery handling (was already on Bridge). Confirmed working on bench DataPad — Hero DataPad battery died before testing, flash when charged.
- **2.8" audio fix — `Data_Pad_28/Audio_PCM5101.cpp`:** Local PCM5101 audio never worked — first time tested this session. Root cause: debug scaffolding left in `Play_Music()` that called `Music_pause()` / `Music_resume()` / `Music_pause()` immediately after `connecttoFS`, leaving audio paused. Also `Audio_Loop()` was replaying a test file (`A.mp3`) whenever audio stopped. Both removed. Audio now working on both sound buttons. Note: `File_Search()` always prints "file not found" in serial monitor due to ESP32 core returning full path from `file.name()` — harmless, no effect on playback.
- **Repo updated:** `Data_Pad.ino`, `ui_Screen3.c`, `ui_Screen3.h`, `README.md` (Data_Pad), all sketch READMEs, `DEVNOTES.md` copied to `Repo output/`. MACs zeroed in repo `Data_Pad.ino`.

---

**Session 44 (2026-06-24). Website rewrite, Save WiFi hint, portable web server.**

**Changes this session:**

- **LCARS website restructured (9 pages):** Assembly page renamed "Lessons Learned" — rewritten as entertaining build stories (phantom short, GPIO 9 boot freeze, near-miss NVS clear, boot indicator payoff). Programming content moved to new Firmware page (startup coordination, offline routing, connection-lost handler, WiFi auto-revert, refinements, battery monitor). Warp Core page condensed to project overview — Power Notes moved to Hardware page. New Resources page (repos, community credits, model, boards, tools). Gallery populated with 26 photos in 7 sections. All `<code>` tags stripped site-wide. Session references removed. 1:900 scale corrected throughout.
- **Save WiFi hint label:** Added `ui_LabelSaveHint` below Save WiFi button on DataPad Screen 3 — shows "Press again to disconnect" when saved, clears on manual or auto revert. Updated in `Data_Pad/` and `Data_Pad_Hero/` (both `ui_Screen3.c` and `.ino`). `Data_Pad_28/` got the label in `ui_Screen2.c` but SaveWifi is still a stub there.
- **WebServer_ESP sketch (not in main repo):** Standalone portable web server on Hosyond 4.0" ESP32-E board. Serves the LCARS website from SD card over open WiFi AP ("Enterprise-D") with captive portal. LCARS-themed display with WiFi and GitHub QR codes. Uses ESPAsyncWebServer + SD (HSPI) + TFT_eSPI (ST7796S). `copy_to_sd.bat` script copies site files and patches CSS for local Antonio font (no internet on captive portal). Antonio woff2 downloaded to `site/fonts/`.
- **Repo pushed:** Save WiFi hint label + DEVNOTES/HARDWARE doc updates.

**Previous session (43, 2026-06-23):**

- **`board_utility/board_utility.ino` — ESP-NOW PCB test commands added:** ESP-NOW send capability added to the board_utility web UI. New "PCB Test" card with 4 buttons (Asm ON, Asm OFF, All On, All Off) and an optional MAC target field. Broadcast by default (`FF:FF:FF:FF:FF:FF`), targeted if a MAC is entered — temporary peer registered/removed per send. Shared `struct_message` and 4 LED command constants copied in. Send-only, no receive callback. No changes to other sketches needed.
- **`board_utility/README.md` updated:** "Testing a Newly Assembled PCB" section rewritten to lead with the new web UI method. Manual source-edit method kept as fallback.
- **LCARS design story website built (`site/`):** 7-page static HTML/CSS/JS site converting the full `DESIGN_STORY.md` into an LCARS-themed website. Initial build — subsequently rewritten in session 44.
- **Hardware corrections found during website review:** FFC over-engineering noted, DY-SV17F speaker corrected to mono, battle bridge pull-down rediagnosed as GPIO 9 strapping pin, Xiao footprint sourcing corrected.

**Previous session (42, 2026-06-23):**

- **New saucer PCB assembled for testing.** MAC: `80:f1:b2:cc:46:30`. Used forced `asmMode = true` in a copy of Bridge sketch (`Bridge_ESP_asm_mode_on/`) to verify solder joints — no DataPad needed.
- **`board_utility/README.md` updated:** added "Testing a Newly Assembled PCB" section — documents how to force assembly mode on boot for both Bridge and EngRoom to verify solder connections without needing a DataPad.

**Planned — board_utility assembly mode (build next session):**

Add ESP-NOW to the board_utility sketch so it can send assembly mode and basic LED commands from the web UI. Two send modes:

- **Broadcast (default):** sends to `FF:FF:FF:FF:FF:FF` — hits every ESP-NOW device on ch1. No MAC addresses needed. Good for fresh boards.
- **Targeted:** optional MAC text field in the web UI. If populated, registers that MAC as an ESP-NOW peer and sends directly.

Commands to support: `LED_ASSEMBLY_MODE` on/off, `LED_ON` (all groups), `LED_ALL_OFF`. Uses the shared `struct_message` (same layout so receiving sketches accept it). Send-only — no receive callback needed. Already running `WIFI_AP_STA` so ESP-NOW works on the STA interface with no changes to AP/captive-portal behavior.

Web UI adds a "PCB Test" card below the existing NVS section:
```
Target: [__MAC field__] (empty = broadcast)
[Asm ON] [Asm OFF] [All On] [All Off]
```

**Planned — standalone controller (future):**

A separate sketch (not crammed into board_utility) that replaces the DataPad for users who don't want to build the touchscreen hardware. Same ESP-NOW control, served as a responsive web UI from any ESP32 dev board over its own AP. Could incorporate the LD2410C mmWave sensor on a UART pin. USB powered, no custom PCB needed. Would support the full command set: startup/shutdown sequences, sync mode, timing controls, sound. LCARS CSS theming.

**Planned — NVS-based MAC provisioning (future, requires all sketch changes):**

Instead of hardcoding peer MAC arrays in each sketch, store them in NVS. The board_utility (or standalone controller) pushes MAC assignments to boards over ESP-NOW broadcast — no need to know the target's MAC to configure it.

How it works:
- New command constant (e.g. `SET_PEER_MAC`, next available = 32). `ledGroup` = which peer slot (1=Bridge, 2=EngRoom, 3=DataPad, 4=WarpCore). 6-byte MAC packed into `ssid1[]`.
- Broadcast with target `boardInd` in the message — every board receives it, only the addressed board processes it.
- Each sketch's `setup()` reads peer MACs from NVS keys (`peer_brg`, `peer_eng`, `peer_dp`, `peer_wc`), falls back to current hardcoded defaults if not set.
- After storing, sketch re-registers the ESP-NOW peer with the new MAC.
- Utility web UI: fields for each board's MAC, "Push Config" button. Optional "Read Current" to ask boards to report stored peers.
- Eliminates the "reflash everything when a board is swapped" problem. Hardcoded defaults mean existing behavior is unchanged until actively configured.

Requires changes to all four sketches (Bridge, EngRoom, DataPad, WarpCore) + the utility. Struct does not change — reuses existing fields.

**Pending (carried forward):**
- Reflash: `Data_Pad` + `Data_Pad_Hero` (Save WiFi hint label), `Bridge_ESP_Model` (new EngRoom MAC), `Engine_Room_ESP` (GPIO 3 nacelle pin — fixed in PCB files, no bodge wire needed).
- Bridge board swap still pending — old boards in model, new boards on bench.
- Website live at enterprise-d-refit.com (Cloudflare Pages, 2026-06-25). No further deploy action needed.

---

**Session 40 complete (2026-06-18). Repo cleanup — MAC addresses scrubbed, board_utility sketch created, NVS clear sketches consolidated.**

**Changes this session:**

- **Repo MAC address scrub:** All real MAC addresses removed from `Repo output/` for public GitHub repo. Sketch peer MAC arrays zeroed to `{0x00, ...}` with "run MAC_address_retriver on your board" comments (`Bridge_ESP.ino`, `Data_Pad.ino`, `Engine_Room_ESP.ino`). MAC references in `DEVNOTES.md` (~12 instances), `HARDWARE.md`, and `Engine_Room_ESP/README.md` replaced with `<redacted>`. `WarpCore_ESP32.ino` was already clean.
- **Data_Pad.ino repo sync:** Repo copy was stale — missing the `ledValue > 0` guard on Bridge battery alerts (session 39 fix). Synced to repo.
- **`board_utility/` sketch created:** Standalone ESP32 utility preloaded on boards. Starts WiFi AP (`enterprise` / `ncc-1701-d`), runs captive portal that auto-opens a webpage showing the board's STA MAC address. Includes NVS clear buttons (Bridge, EngRoom, DataPad, WarpCore, Clear All) and ArduinoOTA. No serial monitor or WiFi credential editing needed — connect phone/laptop to AP, get MAC, clear NVS if needed.
- **`Bridge_NVS_Clear/` and `EngRoom_NVS_Clear/` removed from repo:** NVS clear functionality consolidated into `board_utility`. Old sketches required hardcoding WiFi credentials; new utility uses its own AP instead.
- **Repo pushed:** Three commits — MAC redaction, board_utility addition with NVS clear, rename from `web_mac_address` to `board_utility`.

**Pending:**
- Reflash model units: `Bridge_ESP_Model` (old Bridge board, new EngRoom MAC), `Data_Pad_Hero` (all session 39 changes), `Engine_Room_ESP` (GPIO 3 nacelle pin — after bodge wire done).
- GPIO 9 bodge wire on EngRoom PCB: mask GPIO 9 trace, jump to GPIO 3 for right nacelle.
- Bridge board swap still pending — old boards in model, new boards on bench.

---

**Session 39 complete (2026-06-18). New EngRoom board installed in model. PCB fixes, ESP swap, GPIO 9 strapping pin fix, broadcast WiFi push.**

**Changes this session:**

- **EngRoom PCB nacelle polarity fix:** Left and right nacelle connector footprints had positive and negative pads reversed. Discovered during assembly — nacelle LEDs would not light with correct wiring polarity. PCB updated in EasyEDA and new Gerbers exported (`EngRoomPCB.zip`). No firmware change needed.
- **EngRoom ESP replaced — new MAC `80:F1:B2:60:6F:54`:** Xiao ESP32-C3 swapped out. Previous MAC `3c:dc:75:ae:da:fc`. Updated in `Bridge_ESP.ino`, `Data_Pad.ino`, `Data_Pad_Hero.ino`, `Bridge_ESP_Model.ino`, `PROJECT_REFERENCE.md`, `GPIO outputs EngRoom.txt`, `CLAUDE.md`.
- **GPIO 9 strapping pin fix (`Engine_Room_ESP.ino`):** Right nacelle on GPIO 9 caused boot freeze — GPIO 9 is the BOOT strapping pin on ESP32-C3, and the 1.2kΩ base resistor into the NPN transistor pulled it LOW at power-on, putting the chip into download mode. `PIN_NAC_R` changed from 9 to 3. Hardware workaround: solder mask GPIO 9 trace, jump wire to GPIO 3. PCB updated in EasyEDA. Old board was unaffected — nacelles were on GPIO 2/3, not strapping pins.
- **`Engine_Room_ESP_Model/` deleted:** Old EngRoom board no longer in model — new board installed. Only `Engine_Room_ESP.ino` remains.
- **Model sketch MAC updates:** `Bridge_ESP_Model.ino` — EngRoom peer updated from old board `cd:b8` to new `80:F1:B2:60:6F:54`. `Data_Pad_Hero.ino` — same. `Engine_Room_ESP.ino` — DataPad peer updated from bench `a6:f3:4c` to model `a9:a1:14` (Bridge peer was already correct).
- **Broadcast WiFi push (`Data_Pad.ino`, `Data_Pad_Hero.ino`):** SaveWifi now sends credentials via ESP-NOW broadcast (`FF:FF:FF:FF:FF:FF`) first, then unicast to all registered peers as fallback. 500ms delay between broadcast and unicast, and another 500ms before DataPad starts its own WiFi connect — prevents channel shift from killing the broadcast frame before it transmits. Broadcast peer registered in setup on both DataPad sketches.
- **EngRoom MAC in WiFi status response (`Engine_Room_ESP.ino`):** WiFi connect success response now includes `WiFi.macAddress()` in the `ssid1` field. DataPad displays IP + MAC on Label18 (Screen 3) for diagnostic confirmation.
- **Bridge battery alert guard (`Data_Pad.ino`, `Data_Pad_Hero.ino`):** Battery alert logic now skips when `ledValue == 0` (no ADC hardware). Old Bridge board has no voltage divider — was sending 0mV every 30s, triggering red alert repeatedly. Display still updates (shows 0%/`--%%`), alerts resume when real readings arrive from new boards.
- **Hero DataPad mDNS renamed (`Data_Pad_Hero.ino`):** `mdnsName` changed from `"DataPad"` to `"HeroDataPad"`. Both DataPads plugged in simultaneously now show as distinct OTA targets in Arduino IDE (`HeroDataPad.local` vs `DataPad.local`).
- **Stardrive power consumption measured:** EngRoom section on bench supply draws 275–325mA at 7.4V with everything on, settling near 275mA (~2W).

**Pending:**
- Reflash model units: `Bridge_ESP_Model` (old Bridge board, new EngRoom MAC), `Data_Pad_Hero` (all session 39 changes), `Engine_Room_ESP` (GPIO 3 nacelle pin — after bodge wire done).
- GPIO 9 bodge wire on EngRoom PCB: mask GPIO 9 trace, jump to GPIO 3 for right nacelle.
- Bridge board swap still pending — old boards in model, new boards on bench.

---

**Session 38 complete (2026-06-13). Pocket remote startup-mode bug fixed; Red Alert root cause found (edits had landed on the wrong DataPad sketch); old EngRoom board sketch created.**

**Changes this session:**

- **Pocket remote `PowerUpAll()` rewritten (`Data_Pad_28.ino`):** now mirrors `Data_Pad.ino`'s leader-dispatch pattern using `bridgeOnline`/`engRoomOnline` — sends `LED_STARTUP` to Bridge only when both units are online (Bridge relays to EngRoom at step 10), avoiding the previous `sendBoth()` relay/restart loop that made the remote's power-on always look like "separated mode". `assembled` is hardcoded `true` (remote has no live sync-mode tracking) — flagged to user as fine for now since the remote is mainly used with the model assembled.
- **Red Alert root cause found:** last session's Red Alert trigger handling (`boardInd==3 && status==5` → `RedAlertStart()`) and the pocket-remote power/damage UI-mirror block were added to `Data_Pad/Data_Pad.ino` — the **bench/new-board** 7" sketch — instead of `Data_Pad_Hero/Data_Pad_Hero.ino`, the sketch actually flashed to the 7" unit in the model. Ported both blocks into `Data_Pad_Hero.ino`'s `handleReceivedData()`. `Data_Pad.ino` keeps its copy too — user wants it left in place since it'll become the new-board sketch once that board is installed.
- **`Engine_Room_ESP_Model/Engine_Room_ESP_Model.ino` created:** new sketch for the old EngRoom board in the model (`3c:dc:75:ae:cd:b8`). Copied from `Engine_Room_ESP.ino` (which already had pocket-remote integration from last session), then: fixed the DataPad peer MAC (was the bench/test-board MAC `...a6:f3:4c`, now the model MAC `20:6e:f1:a9:a1:14`); GPIO/IDX/`STARTUP_SEQ` rewritten for the old board's 11-pin, 5-window layout (GPIO 2/3 = right/left nacelle, GPIO 8/9 = right/left battle bridge, no battery ADC); removed `readBatteryVolts()`/`PIN_BAT_ADC`/ADC setup and the `LED_BAT_LEVEL` status-ping block (old board has no ADC divider, GPIO 2 is reused as a nacelle pin). **Not yet flashed — first-time flash to the old EngRoom board.**
- **`sounds/copy_to_remote_player.bat` created:** same 44-file sound set as the DataPad's loader script, copies into a `/sounds/` subfolder on the pocket remote's SD card (remote firmware reads `/sounds/00001.mp3` etc., DataPad reads from card root).
- **Doc-accuracy pass (CLAUDE.md, PROJECT_REFERENCE.md, HARDWARE.md)** — root cause of the Red Alert bug was `Data_Pad_Hero.ino` being completely missing from the Units tables, so last session's edits landed on the wrong sketch. Fixed:
  - Added `Data_Pad_Hero/Data_Pad_Hero.ino` as the model 7" DataPad sketch in both CLAUDE.md and PROJECT_REFERENCE.md Units tables; relabeled `Data_Pad/Data_Pad.ino` as bench/new-board. Added a naming-exception note for "Hero" (named for the hero-shot photo build, not `Data_Pad_Model`).
  - Same fix for Bridge: `Bridge_ESP_Model.ino` (model) vs `Bridge_ESP.ino` (bench/new board) — CLAUDE.md and PROJECT_REFERENCE.md both previously listed `Bridge_ESP.ino` as "installed in model", which was wrong.
  - CLAUDE.md's EngRoom GPIO Pinout table split into "New board (`Engine_Room_ESP.ino`, bench, not yet installed)" and "Old board (`Engine_Room_ESP_Model.ino`, currently installed in model)" — the old-board layout reconstructed for the new sketch above is now documented centrally.
  - PROJECT_REFERENCE.md's EngRoom MAC row was listing only `da:fc` (the not-installed board) with no label — split into model (`cd:b8`) / bench-not-installed (`da:fc`) rows to match CLAUDE.md.
  - HARDWARE.md's EngRoom PCB revision note (line ~148) reworded — dropped "(new boards fitted 2026-05-28)... confirmed working" phrasing that implied the new GPIO layout was already in the model; now describes it as a bench-confirmed PCB design fact.

**Pending:**
- **Reflash for this session's changes:** `Data_Pad_Hero/Data_Pad_Hero.ino` (7" DataPad, model — Red Alert fix), `Data_Pad_240x320/Data_Pad_28/Data_Pad_28.ino` (pocket remote — PowerUpAll fix), `Engine_Room_ESP_Model/Engine_Room_ESP_Model.ino` (old EngRoom board, model — first flash, new sketch).
- After reflashing: verify Red Alert works from the pocket remote; verify power-on brings up Bridge+EngRoom together without the restart/relay-loop glitch; verify each EngRoom LED group lights correctly on the old-board GPIO remap (reconstructed from notes, not yet hardware-tested).
- Board swap: install the session 36 new boards (Bridge, EngRoom, nacelle) and flash `Bridge_ESP_Model` via USB.
- WarpCore sensor housing/calibration — fit the new (already printed) base, wire LD2410C UART to a spare Nano, calibrate via HLKRadarTool.
- Pocket remote → repo: once 2.8" remote tested, add `Data_Pad_240x320/` to public repo.

---

**Session 37 complete (2026-06-13). Old model Bridge board recovered; EngRoom startup relay bug fixed and verified.**

**Changes this session:**

- **Old model Bridge board recovered:** previously thought locked up/OTA-unreachable. Reflashed and powered up — board itself was not dead. Root cause uncertain: likely a bad/interrupted OTA upload (possibly tied to the long pre-OTA boot delay) and/or a leftover dummy cell in the power pack shorting the pack — removed.
- **EngRoom startup relay bug fixed:** `Bridge_ESP_Model.ino`'s EngRoom peer (`broadcastAddress1`) was `3c:dc:75:ae:da:fc` (the bench/new, not-yet-installed EngRoom board) — wrong. The EngRoom board actually in the model is the **old board, `3c:dc:75:ae:cd:b8`**. Fixed, reflashed, and verified — startup relay to EngRoom now works.
- **CLAUDE.md corrections:** EngRoom MAC table split into "model, old board" (`cd:b8`) and "bench / new board, not yet installed" (`da:fc`) rows, mirroring Bridge's model/bench split. Added a naming-convention note: `_Model` suffix = targets old/model hardware; no suffix = targets new bench-tested boards. `Data_Pad.ino` / `Engine_Room_ESP.ino` have no `_Model` variant and correctly target the new boards — their MACs should not be "fixed" to match old-board MACs.
- **History correction:** session 28-30's note that a new EngRoom board (`da:fc`) was "fully assembled in model" was wrong — that board was bench-tested only, never installed. **None of the session 36 new boards (Bridge, EngRoom, nacelle) have been installed** — all bench-tested, functioning as expected, awaiting install.

**Pending:**
- Board swap: install the session 36 new boards (Bridge, EngRoom, nacelle) and flash `Bridge_ESP_Model` via USB.
- WarpCore sensor housing/calibration — fit the new (already printed) base, wire LD2410C UART to a spare Nano, calibrate via HLKRadarTool.
- Pocket remote → repo: once 2.8" remote tested, add `Data_Pad_240x320/` to public repo.

---

**Session 36 complete (2026-06-09). Board assembly complete — all boards ready for installation.**

**Changes this session:**

- **Nacelle LED resistors confirmed:** Parallel-pair groups (3 pairs of 2 LEDs, each pair sharing one resistor) use **240Ω**. Two single LEDs (both through-hole components) each use **470Ω** to match pair brightness (~8.75mA each at 7.4V nominal). Nav lights resistor left unchanged at **1.2kΩ**.
- **Voltage divider confirmed fitted:** Both Bridge and EngRoom new boards have the correct **270kΩ + 100kΩ** ADC resistors populated. ADC calibrated against bench supply across 6.0V–8.4V — multiplier updated from 3.70 → **3.83** in both sketches. Accurate to within 1% across the full range.
- **All boards assembled** — Bridge, EngRoom, and nacelle PCBs built and ready for installation in model.
- **`Nacelle_Test/Nacelle_Test.ino` created:** USB test sketch for Xiao ESP32-C3 — sets GPIO 8 (left nacelle) and GPIO 9 (right nacelle) HIGH at full brightness to verify LED fit before closing up.

---

**Session 35 complete (2026-06-08). 2.8" pocket remote sketch built.**

**Changes this session:**

- **New concept:** 2.8" Waveshare ESP32-S3 board repurposed as a simple pocket handheld remote. Single-screen LCARS-styled UI. 7" DataPad continues to work normally.
- **`Data_Pad_240x320/Data_Pad_28/ui_Screen1.c` rewritten:** 6-button LCARS layout replaces the old 5-button clone layout. Buttons: POWER, RED ALERT, DAMAGE CTRL, SOUND 1, SOUND 2, STOP SOUND. Black background, rectangular amber/red/blue/purple buttons, "USS ENTERPRISE NCC-1701-D" on the orange top bar. Battery voltage shown in status area.
- **`Data_Pad_240x320/Data_Pad_28/ui_Screen1.h` updated:** New externs for ui_Button52 (Red Alert), ui_Button53/54/55 (sound), ui_LabelRedAlert, ui_LabelBat.
- **`Data_Pad_240x320/Data_Pad_28/ui_events.h` updated:** Added RedAlertTrigger, SoundPlay1, SoundPlay2, SoundStop declarations.
- **`Data_Pad_240x320/Data_Pad_28/Data_Pad_28.ino` updated:** `USE_LOCAL_AUDIO` enabled. 7" DataPad MAC (`20:6e:f1:a9:a1:14`) registered as 3rd ESP-NOW peer (`broadcastAddress3`). Added `sendDataPad()` helper. Added `RedAlertTrigger()` — sends `boardInd=3, status=5` to the 7" DataPad. Added `BAT_Init()` in setup and `updateBatDisplay()` called every 10s. mDNS changed to `DataPad2.local` to avoid collision with 7" DataPad. Screen2/BridgeSound navigation removed; stub handlers kept compiled. No shared struct or ledCmd constants changed.
- **`Data_Pad_240x320/Data_Pad_28/BAT_Driver.h` / `BAT_Driver.cpp` added:** Copied from `Display examples/ESP32-S3-Touch-LCD-2.8-Demo/`. GPIO 8, 12-bit ADC, 3× divider, offset 0.990476, returns float volts.
- **`Data_Pad/Data_Pad.ino` (7" DataPad) updated:** 4 lines added to `handleReceivedData()` — checks `boardInd==3 && status==5` and calls `RedAlertStart(NULL)`. This lets the pocket remote trigger the DataPad's existing red alert screen (shows ui_RedAlert, plays file 28 voiced klaxon, auto-dismisses after 60s). No new ledCmd constants.

- **Touch double-init bug fixed (`Display_ST7789.cpp`, `Data_Pad_28.ino`):** `LCD_Init()` was calling `Touch_Init()` internally at the end of the function. If setup() also called `Touch_Init()` explicitly (as Waveshare's own demo does), `Wire1.begin()` and `attachInterrupt()` were called twice — can hang the I2C bus and corrupt serial output. Fix: removed `Touch_Init()` from inside `LCD_Init()`. Added explicit `Touch_Init()` call in setup() between `LCD_Init()` and `Lvgl_Init()`, where it's visible and can only be called once.
- **Web server removed from `Data_Pad_28.ino`:** `AsyncTCP` and `ESPAsyncWebServer` includes, `AsyncWebServer server(80)`, the full `setupWebServer()` function, and the `server.begin()` / `MDNS.addService("http","tcp",80)` calls in `startWifiServices()` are all gone. WiFi credentials are now set via `#define WIFI_SSID` / `#define WIFI_PASS` at the top of the sketch — fill in, flash once, and the credentials auto-save to NVS on first successful connect. Future OTA flashes can leave the defines blank; saved NVS values are used automatically. OTA and mDNS still work as before.

**Pending (pre-flash):**
- Flash **2.8" pocket remote** (new sketch).
- Flash **7" DataPad** (picks up 4-line receive handler change).
- Bridge and EngRoom: no reflash needed.
- **Sound slots:** SND_FILE_1=1 and SND_FILE_2=2 are placeholders. User to decide which tracks to put on the 2.8"'s SD card and update constants accordingly. SD card files go in `/sounds/` as `00001.mp3`, `00002.mp3`, etc.

**Pending (ongoing):**
- ~~Old model boards locked up — OTA not reachable. Must open model to reflash. Plan: open model, swap to new boards (correct 270kΩ ADC resistors), flash `Bridge_ESP_Model` via USB.~~ **DONE session 36** — new boards assembled with correct 270kΩ + 100kΩ ADC resistors; ready for installation.
- WarpCore sensor housing/calibration — fit the new (already printed) base, wire LD2410C UART to a spare Nano, calibrate via HLKRadarTool.
- ~~Nacelle warp-flash resistor bench testing — user to try standard values ≥220Ω and report back which looks best.~~ **DONE session 36** — 240Ω for parallel pairs, 470Ω for single LED, 1.2kΩ for nav lights.
- **Pocket remote → repo:** Once the 2.8" remote is tested and confirmed working, add `Data_Pad_240x320/` to the public repo (currently excluded by .gitignore). MAC-zero the peer arrays before publishing.

---

**Session 34 complete (2026-06-08). Main repo made public. WarpCore repo published — GitHub publish is done.**

**Changes this session:**

- **Repo set to public:** https://github.com/badbooger/Enterprise-D-lighting-effects is now live and public (was private since Session 33).
- **Considered renaming Bridge → Saucer and EngRoom → Star_Drive** for Trek theming. Decided against it — would touch mDNS hostnames, NVS namespaces, variable names, and `boardInd` labels across all four sketches, forcing a reflash of every unit. Not worth the churn for a cosmetic change. Names stay as-is.
- **WarpCore repo created and published:** https://github.com/badbooger/Enterprise-D-WarpCore — tagged v1.0. New `WarpCore Repo output/` folder built (same clean-copy pattern as the main repo): `WarpCore_ESP32.ino` (MACs already zeroed), `README.md`, and a new MIT `LICENSE` (code only — no 3D-model/sound notes, those live in the main repo).
- **Cross-links filled in both READMEs:** root `README.md` WarpCore section now links to the WarpCore repo; `WarpCore_ESP32/README.md` (both the standalone repo's copy and the embedded reference copy in the main repo) now links back to the main repo, replacing the `<!-- add main repo link here -->` placeholder. Pushed to main repo as commit `00bfe85`.
- **GitHub publish effort is now complete** — both repos live, public, tagged v1.0, cross-linked. `PUBLISH_CHECKLIST.md` fully checked off.
- **GitHub topics chosen for the main repo:** `esp32`, `esp32-s3`, `arduino`, `star-trek`, `startrek-tng`, `lcars`, `prop-replica`, `esp-now`, `lvgl` — set via the repo page's "About" gear icon (not stored in any repo file).
- **Repo name typo fixed:** Main repo renamed on GitHub from `Enterprise-D-lightning-effects` → `Enterprise-D-lighting-effects` ("lighting", not "lightning"). Updated local git remote, both READMEs' cross-links, `PUBLISH_CHECKLIST.md`, and historical log entries above to match. GitHub redirects the old URL automatically.
- **Real MAC addresses found leaking in README example blocks — fixed:** Session 33's MAC-zeroing pass only touched the `.ino` peer-address arrays, missing the matching example code blocks in `Bridge_ESP/README.md`, `Engine_Room_ESP/README.md`, `Data_Pad/README.md`, and both `WarpCore_ESP32/README.md` copies — all showed the real on-model device MACs (DataPad, Bridge, old EngRoom, WarpCore). Replaced with `{0x00, ...}` placeholders to match the sketches. **Lesson: when scrubbing identifying info from sketches before publishing, also grep the READMEs/docs for the same values — they often duplicate examples.** Pushed to both repos.
- **Reviewed WarpCore repo's "3D model" section:** considered duplicating the `enterprise documentation/Warp_Core/` archive (STLs, PCB files, attribution) into the WarpCore repo too, since Thingiverse no longer hosts the original. Decided against it — both repos are under the same GitHub account, so a second copy there adds no real preservation redundancy, just sync upkeep. Files stay where they are, single source of truth in the main repo (already attributed and CC-licensed). No changes made.
- **Added `ASSEMBLY.md` build guide:** new top-level file in the main repo with 9 photos from `Enterprise Pictures/assambly photos/` (copied into new `assembly_photos/` folder), covering FFC routing through the stardrive frame and neck connection, nacelle routing, dual speaker mounting (zip-tied to lower framing), and DataPad internal wiring. Linked from the root README's "Repository contents" table. Committed as `a5cebab` and pushed.
- **Fixed dead `DESIGN_STORY.md` link in published `ASSEMBLY.md`:** `DESIGN_STORY.md` is intentionally kept out of the public repo (its content is reserved for a future standalone LCARS-themed write-up site, not the GitHub repo) — but `ASSEMBLY.md` referenced it anyway, a dead link for anyone reading the published guide. Pointed the reference at `HARDWARE.md` instead (the FFC reasoning is already explained inline in `ASSEMBLY.md`). Committed `e9b4f2e`, pushed.
- **`DESIGN_STORY.md` updated (working copy only — not published):** added a new "Remaining to Complete the Project" section listing the two physical items left: the old-model board swap, and the WarpCore sensor housing/calibration. Confirmed status of the **new WarpCore base — designed and printed, not yet fitted**; sensor not yet calibrated. Calibration plan: spare Arduino Nano as USB-to-serial passthrough to HLKRadarTool (LD2410C UART pins not yet wired — only OUT connected). Considered and rejected a WiFi-based bridge through the WarpCore's own Nano ESP32 — would need new bridge firmware plus virtual-COM-port software on the PC for a one-off tuning task; the spare-Nano passthrough needs no new code and the hardware is already on hand.
- **Nacelle warp-flash brightness — resistor swap planned (no board change):** user wants more punch from the nacelle "warp flash" (6× blue 1206 LEDs, 20mA/Vf 2.6–3.1V, wired as 3 parallel pairs each sharing one current-limiting resistor, all on one transistor — documented in `HARDWARE.md` Nacelle PCB Notes). Walked through the resistor math: pushed back on the parallel-pair-per-resistor topology in general (uneven current split between paired LEDs from Vf mismatch), but since the nacelle's diffusers already mask any visible pair imbalance, the only real constraint is keeping the *hotter* LED of each pair under 20mA in a worst-case split. Calculated floor of **220Ω** (worst case ~70/30 split keeps the hot LED at ≤~18.5mA at 8.4V full charge). User has a resistor kit and will bench test multiple standard values, won't go below 220Ω.

**GitHub publish project is now fully wrapped up — repo is public, cross-linked, documented, and handed over to the community for further contributions.**

**Pending:**
- Old model boards locked up — OTA not reachable. Must open model to reflash. Plan: open model, swap to new boards (correct 270kΩ ADC resistors), flash `Bridge_ESP_Model` via USB.
- WarpCore sensor housing/calibration — fit the new (already printed) base, wire LD2410C UART to a spare Nano, calibrate via HLKRadarTool.
- Nacelle warp-flash resistor bench testing — user to try standard values ≥220Ω and report back which looks best.

---

**Session 33 complete (2026-06-05). Repo output folder created and pushed to GitHub.**

**Changes this session:**

- **`Repo output/` created:** Clean copy of all repo files — sketches, docs, PCB files, 3D files. Kept separate from working `enterprise/` folder. No changes to originals.
- **MAC addresses zeroed in repo copies:** `Bridge_ESP.ino`, `Engine_Room_ESP.ino`, `Data_Pad.ino`, `WarpCore_ESP32.ino` — all peer MAC arrays set to `{0x00, ...}` with comment pointing to `MAC_address_retriver`. NVS clear sketches already had placeholder creds.
- **Dev/test folders excluded from repo:** `Bridge_Sound_Test/`, `DataPad_DFPlayer_Test/`, `Display examples/`, `WarpCore_PinTest/`, `WarpCore/` (old sketch), `Data_Pad_Hero/`, `notes/`, `Bridge_ESP_Model/`, `PUBLISH_CHECKLIST.md`, `backup_docs.sh`, `ISSUES_LIST.txt`, `Enterprise Pictures/`.
- **EasyEDA libraries excluded:** `enterprise documentation/PCB Design/libraries/` removed from repo copy — Gerber zips and BOM/CPL xlsx files are sufficient for JLCPCB.
- **`Data_Pad/filelist.txt` removed** from repo copy (dev artifact).
- **git init + initial commit:** 118 files, commit `822777e`. Remote set to `https://github.com/badbooger/Enterprise-D-lighting-effects.git`.

**Pending:**
- WarpCore repo — create on GitHub, link from main README and WarpCore README.
- Old model boards locked up — OTA not reachable. Must open model to reflash. Plan: open model, swap to new boards (correct 270kΩ ADC resistors), flash `Bridge_ESP_Model` via USB.

**Done:**
- Hero photo shoot — complete. Photos taken before old boards locked up.
- Repo live: https://github.com/badbooger/Enterprise-D-lighting-effects — tagged v1.0.

---

**Session 32 complete (2026-05-29). Bridge web server removed. Boot indicator timer fix. Auto-start default shortened.**

**Changes this session:**

- **Web server removed (`Bridge_ESP.ino`, `Bridge_ESP_Model.ino`):** `AsyncTCP` and `ESPAsyncWebServer` includes removed. `AsyncWebServer server(80)` object removed. Entire HTML page (PROGMEM) removed. Both `server.on()` route handlers removed from `setup()`. `server.begin()` and `MDNS.addService("_http", "_tcp", 80)` removed from `StartWiFi()`. mDNS itself retained — `Bridge.local` still resolves for OTA. DataPad is the sole control interface.
- **Boot indicator timer fix (`Bridge_ESP.ino`, `Bridge_ESP_Model.ino`):** `bootIndicatorBlinkMs = millis()` added at end of `setup()` (just before `bootMs`). Root cause: boot indicator starts before `soundInit()` (500ms delay) and WiFi/ESP-NOW init (~200ms), so by the time `loop()` first runs ~700ms have elapsed — past the 500ms interval — causing an immediate toggle to OFF on the first iteration. If DataPad then sends `LED_STARTUP` quickly, the indicator was cancelled in the OFF state. User saw one flash then nothing.
- **Auto-start default shortened (`Bridge_ESP.ino`, `Bridge_ESP_Model.ino`):** `autoStartDelayMs` default changed from 30,000ms to 10,000ms. Root cause of old board 30-second delay: NVS clear wiped the previously-saved `auto_ms` value, reverting to the 30s default. In standalone mode (no DataPad) this was the only startup trigger. 10s is enough for DataPad to connect and suppress auto-start if present, but not painful standalone.
- **Old board freeze diagnosis:** "LEDs won't start until WiFi times out" was `wifi_auto = true` left in NVS from a previous OTA session, causing `StartWiFi()` to block for up to 20 seconds in `loop()`. Fixed by session 30's `wifi_auto`-on-fail NVS clear — old boards need reflash or NVS clear sketch to pick up that fix. GPIO 2 floating (no ADC voltage divider on old boards) is not a factor — `analogReadMilliVolts()` on a floating pin is instantaneous.

**Pending:**
- Old model boards locked up — OTA not reachable. Must open model to reflash. Plan: open model, swap to new boards (correct 270kΩ ADC resistors), flash `Bridge_ESP_Model` via USB.

**Done:**
- Hero photo shoot — complete. Photos taken before old boards locked up.

---

**Session 31 complete (2026-05-29). Bench DataPad SD card setup. Bridge model-install sketch created.**

**Changes this session:**

- **Bench DataPad SD card:** Swapped to a pre-formatted card (previous Linux card couldn't be reformatted cleanly). Ran `copy_to_datapad_player.bat` to E: — all 44 sound files copied. Confirmed working.
- **Bench DataPad setup complete:** Model DataPad (`20:6e:f1:a9:a1:14`) stays with model. Bench DataPad (`20:6e:f1:a6:f3:4c`) used for development going forward.
- **`Bridge_ESP_Model/` created:** Copy of `Bridge_ESP.ino` with model DataPad MAC (`20:6e:f1:a9:a1:14`) as `broadcastAddress2`. Use this sketch to flash the model Bridge. Main `Bridge_ESP/Bridge_ESP.ino` retains bench DataPad MAC (`20:6e:f1:a6:f3:4c`) for bench work.

**Pending:**
- Flash `Bridge_ESP_Model` to model Bridge via OTA (`Bridge.local`) — batteries were low this session, may resolve on their own after charging.
- Hero photo shoot — flash `Data_Pad_Hero`, shoot, done.
- Install new boards after photos — swap to correct 270kΩ ADC resistors, install, reflash main sketches.

---

**Session 30 complete (2026-05-28). Bridge boot indicator fix, wifi_auto NVS fix, NVS clear sketches updated with OTA.**

**Changes this session:**

- **Bridge boot indicator moved early (`Bridge_ESP.ino`):** Boot indicator (NAV + G14 + G15 blink) now starts immediately after `ledcAttach` calls, before `soundInit()` and `WiFi.mode()`. Previously set at end of `setup()` — `WiFi.mode(WIFI_STA)` + `esp_now_init()` + `soundInit()` 500ms delay caused a multi-second pause before any visible indication.
- **wifi_auto cleared on WiFi fail — Bridge and EngRoom:** `StartWiFi()` failure path now writes `wifi_auto=false` to NVS before returning. Previously left `wifi_auto=true` in NVS after a failed attempt, causing Bridge/EngRoom to retry WiFi on every subsequent boot. Root cause of Bridge taking over a minute to become responsive after an OTA session that wasn't cleanly closed.
- **NVS clear sketches updated with WiFi + OTA (`Bridge_NVS_Clear/`, `EngRoom_NVS_Clear/`):** Both sketches now clear NVS then connect to WiFi (hardcoded `WIFI_SSID`/`WIFI_PASS` at top of sketch) and start ArduinoOTA. Boards are installed in model — USB connectors not accessible. Flash NVS clear via OTA, wait for "Ready for OTA" in Serial Monitor, then upload main sketch back via OTA to `Bridge.local` / `EngRoom.local`. README files added to both sketch folders.

**Pending:**
- Hero photo shoot with model boards — flash `Data_Pad_Hero`, shoot, then swap to new boards and reflash `Data_Pad`.
- Bridge needs reflashing for boot indicator fix and wifi_auto NVS fix.

---

**Session 29 complete (2026-05-28). Doc updates and hero photo DataPad copy.**

**Changes this session:**

- **Doc updates:** `Engine_Room_ESP/README.md` — GPIO table rewritten for new board layout, description updated (battle bridge removed, 9 groups), nav lights corrected to GPIO 5. `HARDWARE.md` — LED groups table updated (9 LED pins, 3 window groups), nav GPIO corrected to 5 in two places, EngRoom PCB revision note converted from pending to confirmed-done with full GPIO detail, "LED effects not yet coded" line removed.
- **`Data_Pad_Hero/` created:** Full copy of `Data_Pad/` for hero photo shoot with old installed boards. MACs set to: EngRoom `3c:dc:75:ae:cd:b8` (old board), Bridge `e0:72:a1:d7:37:14` (model install). Main `Data_Pad/` sketch retains new MACs. Flash `Data_Pad_Hero` to DataPad for photos, then switch back to `Data_Pad` when new boards go in.

**Pending:**

---

**Session 28 complete (2026-05-28). EngRoom new board GPIO reassignment, battery monitor, and DataPad alert fix. All changes confirmed working on hardware.**

**Changes this session:**

- **EngRoom MAC updated:** `3c:dc:75:ae:cd:b8` → `3c:dc:75:ae:da:fc` (new installed board). Updated in `Engine_Room_ESP.ino`, `Bridge_ESP.ino` (peer address), `Data_Pad.ino` (peer address), `PROJECT_REFERENCE.md`, `GPIO outputs EngRoom.txt`, `CLAUDE.md`.
- **EngRoom GPIO reassignment (`Engine_Room_ESP.ino`):** New board hardware has nacelles on GPIO 8/9 and neck windows consolidated. `PIN_NAC_R` 2→9, `PIN_NAC_L` 3→8. `WIN_PINS` collapsed from 5 to 3 entries: `{20, 7, 21}` (neck all, eng top, eng bot) — GPIO 20 now drives back-of-neck + both battle bridges wired together. `IDX_BB_RIGHT`/`IDX_BB_LEFT` removed; `IDX_NECK=0`, `IDX_ENG_TOP=1`, `IDX_ENG_BOT=2`. `STARTUP_SEQ` step 0 updated from 3-entry neck walk to single `IDX_NECK`. `ALL_PINS` reduced from 11 to 9 entries (GPIO 2 and 3 removed). Externally-used IDX values (5–10, 20) unchanged — no DataPad changes required.
- **EngRoom battery monitor (`Engine_Room_ESP.ino`):** `PIN_BAT_ADC = 2`, `LED_BAT_LEVEL = 31` added. `readBatteryVolts()` using `analogReadMilliVolts() × 3.70 / 1000` (same 270kΩ+100kΩ divider as Bridge). `analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db)` in setup. Battery mV sent to DataPad every 30s alongside status ping.
- **DataPad alert fix — EngRoom battery now triggers alerts (`Data_Pad.ino`):** `LED_BAT_LEVEL` from EngRoom (`boardInd=2`) previously updated display only. Same yellow/red threshold logic (BAT_WARN_MV=7400, BAT_CRIT_MV=7000) added to the EngRoom receive path. `batWarnFired`/`batCritFired` flags shared — either board going low fires the alert, no double-trigger.
- **DataPad Bridge display fix (`Data_Pad.ino`):** Bridge battery label now shows `--%%` (grey) when `bridgeBatVolts == 0.0f` (offline / no reading received yet), matching the existing EngRoom behaviour. Previously showed `0%` which was misleading.

**Pending:**

---

**Session 27 complete (2026-05-28). Battery monitor firmware implemented — Bridge ADC read, DataPad display and alerts.**

**Changes this session:**

- **`LED_BAT_LEVEL = 31` added (Bridge + DataPad):** Bridge sends battery voltage as mV integer every 30s. DataPad receives it, updates Screen2 header labels, and triggers alerts at thresholds. EngRoom uses the same constant (differentiated by boardInd=2) — display-only when EngRoom firmware is ready.
- **Bridge ADC firmware (`Bridge_ESP.ino`):** `PIN_BAT_ADC = 2`, 11dB attenuation, `readBatteryVolts()` using `analogReadMilliVolts() × 3.70 / 1000`. Periodic send every 30s via `LED_BAT_LEVEL` to DataPad. Written for final 270kΩ+100kΩ divider values — bench 470kΩ+200kΩ reads ~10% high (confirmed: 69% shown at 7.2V supply, expected ~58%).
- **DataPad battery display (`Data_Pad.ino`, `ui_Screen2.c`):** Two labels in Screen2 header zone — "BRIDGE xx%" and "ENGROOM --%". Percentage formula: (V − 6.0) / 2.4 × 100, clamped 0–100%. Colour: green ≥50%, amber 20–49%, red <20%. EngRoom label stays at "--%" until EngRoom sends LED_BAT_LEVEL.
- **Yellow alert (`ui_YellowAlert.c/h`, `yellow_alert_img.h`):** Fires at ≤7.4V (7400mV). Full-screen image overlay (`yellow alert.png` → 400×249 RGB565 header, displayed at 2× zoom). Plays sound file 28, repeats every 16s. Auto-dismisses after 60s; tap anywhere to dismiss early. One-shot per session (batWarnFired flag, resets on reboot).
- **Red alert threshold:** Fires at ≤7.0V (7000mV) via existing `RedAlertStart()`. batCritFired flag prevents re-trigger.
- **Bridge bench MAC:** `80:f1:b2:cc:46:5c` — new bench ESP32-S3 module. DataPad `broadcastAddress2` swapped to bench MAC for testing. Swap back to `e0:72:a1:d7:37:14` (model install) before final deployment.
- **Battery alert thresholds confirmed on bench (2026-05-28):** Power supply used to test trigger voltages with bench 470kΩ+200kΩ divider. Yellow alert (BAT_WARN_MV=7400) fired at 6.72–6.75V supply — matches calculated 6.70V within resistor and ADC tolerance. Red alert (BAT_CRIT_MV=7000) also confirmed. Up to 30s lag before alert fires (Bridge sends reading every 30s). Flag reset logic confirmed working — voltage raised back above threshold resets flags, alerts re-trigger correctly on next drop below threshold.

**Pending:**

---

**Session 26 complete (2026-05-28). Component substitutions confirmed for hand assembly. No firmware changes.**

**Changes this session:**

- **SS14 confirmed as B0530W substitute for hand assembly:** SS14 (SMA, 1A 40V) is electrically a better part than the B0530W (SOD-123, 0.5A 30V) — higher current and voltage rating, similar Vf. SMA body overhangs SOD-123 pads but both ends make solid contact when hand-soldered. Acceptable for bench testing. EasyEDA BOM retains B0530W for any JLCPCB assembly run — SS14 is hand-assembly only.
- **Voltage divider bench substitute confirmed:** 470kΩ + 200kΩ works as a hand-assembly substitute for the PCB's 270kΩ + 100kΩ. Output range 1.79V–2.51V across 6.0V–8.4V — comfortably within 11dB attenuation range. Firmware should be written for the final 270kΩ+100kΩ values (multiplier 3.70); bench parts give ~10% different readings but that's acceptable for hardware bring-up. Swap to correct resistors before writing calibration constants.
- **JLCPCB ordering costs noted:** Boards ordered bare (no assembly) — boards $29, shipping $65. SMD parts ordered through JLCPCB's separate component supplier — parts $20, shipping another $65. Suppliers cannot be combined. For anyone replicating: use JLCPCB's full SMT assembly service to get boards + parts in one shipment, or batch board revisions to amortise shipping cost.

**Pending:**

---

**Session 25 complete (2026-05-28). HARDWARE.md connector table updated. No firmware changes.**

**Changes this session:**

- **105I connector table updated (HARDWARE.md):** 105I #3 added — 4-pin connector, not split, all pins belong to Group 3 (G03). Existing #1 and #2 entries annotated as split. Assembly warning note added: three physically similar 4-pin connectors all labelled 105I; #3 is unsplit (Group 3 only), #1 and #2 are split across Groups 3, 13, and 14.

---

**Session 24 complete (2026-05-28). GitHub repo prep: `amazon parts.txt` added with Amazon links for all project components. No firmware changes.**

**Changes this session:**

- **`amazon parts.txt` created:** Full parts list with Amazon links for all components used in the build. URLs stripped of tracking parameters (clean `dp/ASIN` links only). Notes added to the 2×3×4 square LEDs (red and white) and 1.25mm JST connectors (2-pin and 4-pin) flagging them as good spares — LED leads and connectors can break during assembly. Sound board entries clarified: saucer DY-SV17F is board-mounted with 4MB onboard flash; DataPad DY-SV17F is a different form factor using SD card storage (same controller chip).
- **README files written:** Root `README.md` created (project showcase — what it does, unit table, getting started, OTA procedure, repo contents, attribution). `Bridge_ESP/README.md`, `Engine_Room_ESP/README.md`, and `Data_Pad/README.md` all rewritten — previous versions were outdated (wrong struct, wrong EngRoom MAC, EngRoom incorrectly stated LED effects not built). Each sketch README covers: what it does, Arduino IDE board settings, first flash + OTA, MACs to update, GPIO assignments, key effects, NVS settings. Assembly notes added: Bridge PCB has ESP32-S3 module placeable by JLCPCB if in stock, PCB has its own USB-C for flashing, dev board route documented as fallback. EngRoom Xiao ESP32-C3 is not a JLCPCB-placeable part — must be hand-soldered after PCB arrives, uses its own onboard USB-C for flashing.
- **EngRoom MAC corrected:** `90:70:69:07:0B:88` (old benchtop test ESP) replaced with `3c:dc:75:ae:cd:b8` (installed unit) in `PROJECT_REFERENCE.md`. All READMEs were written with the correct MAC from the start.
- **WarpCore README written:** `WarpCore_ESP32/README.md` created for the separate WarpCore repo. Covers: 3D model attribution (ElmoC CC BY-NC-ND 3.0, original listing gone), hardware list, pin assignments (with the D2–D6 vs raw GPIO warning), Arduino IDE setup, first flash, MACs to update, standalone vs ESP-NOW integrated modes, chase effect, presence sensor, NVS settings. Main repo README updated to reference the WarpCore repo.
- **LICENSE added:** MIT license, copyright Daniel 2026. Covers all code and PCB designs. Notes at bottom clarify WarpCore 3D model (CC BY-NC-ND 3.0, ElmoC) and excluded sound files. CLAUDE.md added to .gitignore — dev tool briefing, not user-facing. README license block updated to link to LICENSE file.
- **Split connector reference rewritten (HARDWARE.md):** Old section-centric table replaced with connector-centric table — 13 split connectors, one row each, showing both sections, wire colours for each pin group (RB/RBlu/YB/YG/YW), and assembly notes. 20E corrected (section 10 = pins 3,4, was wrong) and flagged as verify-on-assembly candidate for backwards LED connection. Hand notes folder added to .gitignore. Repo hold decision: publishing after new boards are fitted and battery monitor firmware is written and tested — repo goes up complete rather than with known pending changes.
- **Auto MAC discovery considered and ruled out:** ESP-NOW requires peer MACs to be known before sending — discovery would need broadcast phase + NVS persistence across all four sketches. Not worth the complexity for a prop with fixed hardware. MACs only change on board replacement, which requires a reflash anyway.
- **Hand notes photographed:** Photos saved locally in `enterprise documentation/led lay out/Hand notes/` — excluded from repo via .gitignore, kept as personal reference. Checklist item ticked off.
- **3D print files added to `enterprise documentation/`:**
  - `Data Pad stl/` — DataPad PADD case (top shell, bottom shell, body mount).
  - `pcbs test prints/` — Three test-fit STLs (PCB1, PCB 2&3, nacelle) used to verify board dimensions before ordering. Print before placing JLCPCB orders.
  - `Warp_Core/` — Complete archive of the original Thingiverse WarpCore design (thing:1656741, by ElmoC, CC BY-NC-ND 3.0). Original listing no longer available online — full download preserved here including all STLs, original Eagle PCB files (unmodified), original Arduino Nano sketch (unmodified, AVR only), build instructions PDF, attribution cards, and photos. HARDWARE.md updated with full file inventory and note on ARGB alternative designs.

**Pending:**
- LD2410C UART config (deferred — sensor housing being made, wire UART when housing built)
- New boards assembly in progress. Awaiting 270kΩ resistors for ADC voltage divider; using 470kΩ+200kΩ as hand-assembly bench substitute. Swap to correct resistors before calibrating. Split connector verification to be done during assembly.

---

**Session 23 complete (2026-05-26). PCB BOM/CPL audit and EasyEDA revision. No firmware changes.**

**Changes this session:**

- **BOM audit — all boards:** All five BOMs checked (BridgePCB1/2/3, EngRoom, NeckPCB). Found two issues: (1) EngRoom and NeckPCB base resistors labeled "1.2k" but had part number `0603WAF1002T5E` (10kΩ) — EasyEDA had kept old part when value was changed. (2) 100k/270k ADC voltage divider resistors on BridgePCB2 and EngRoom had wrong/same placeholder part number — left as-is, JLCPCB prompts for manual selection at order time.
- **EasyEDA revision — all Bridge PCBs:** Pull-down resistors removed from BridgePCB1, PCB2, PCB3. All base resistors changed from 10kΩ to 1.2kΩ — matches LED current-limiting value, reduces BOM to one resistor value across all boards. EN and reset pin resistors on BridgePCB2 (R210, R211) kept at 10kΩ — these are signal/bias resistors with a required value, not base resistors.
- **EasyEDA revision — EngRoom and NeckPCB:** Base resistor schematic parts corrected to `0603WAF1201T5E` (C22765) — previously only the value field had been updated, not the actual part.
- **Pick-and-place files exported:** CPL files added for all boards alongside Gerbers and BOMs.
- **NacellePCB added:** New board export — BOM, pick-and-place, and Gerber zip. Used ×2 (port and starboard). All components transplanted from original nacelle boards except power connector. Four through-hole footprints are hand-assembled (square LED, VCC injection point, forward red 3mm LED on wire leads, nav lights pigtail with two 3mm LEDs). See HARDWARE.md for full connector/LED map.
- **HARDWARE.md updated:** Nacelle PCB Notes section added; PCB Gerber Files table updated to include all six boards with BOM and CPL columns.

**Pending:**
- LD2410C UART config (deferred — sensor housing being made, wire UART when housing built)

**Session 22 complete (2026-05-25). All changes confirmed working. DataPad reflashed.**

**Changes this session:**

- **Ambient sound fix — touch sound race** (`Data_Pad.ino`): Ambient was not starting on manual POWER UP because `play_touch_sound()` fires before `PowerUpAll()` and two back-to-back UART play commands to the DY1703A caused the second (ambient) to be dropped. Fixed by removing the immediate `padSndPlay()` call from `PowerUpAll()` — replaced with `ambientNextPlayMs = millis() + 3000` so the ambient loop handles the first clip 3 seconds after power-up. Works identically for both manual button and sensor-triggered startup.
- **Ambient override fix — warp sounds** (`Data_Pad.ino`): Regular ambient timer (30s) was firing during warp and overriding the warp engine ambient (file 37, 17s). Fixed in two places: (1) ambient loop now checks `!warpAmbientMs` before playing — defers by 30s instead when warp is active; (2) `stopWarpSounds()` schedules `ambientNextPlayMs = millis() + 3000` on warp deactivation so regular ambient resumes 3s after warp ends rather than being stopped with no resume.
- **Warp slider moved down, label moved below** (`ui_Screen1.c`): Slider was overlapping the LCARS header strip (Button30, y=-131, h=49, bottom at abs y=158) and the label was being covered by the slider knob at warp=0. Final position after two-pass tuning: slider height=220 (was 250), y=55 (track top at abs y=185, 27px clear of header strip, knob at max clears by ~17px). WARP label at y=190 (abs y=430, ~15px below knob at warp=0). Label and slider creation order swapped — slider first, label after.
- **Nav light timing saved to NVS** (`Data_Pad.ino`, `CLAUDE.md`): New `nvNavTiming` global (int, 1–3) persists the last selected timing across reboots. Saved to NVS key `"nav_timing"` (namespace `"datapad"`, default 1) whenever `NavLightsSet()` lands on states 1/2/3 — state 0 (off) never saved, so coming back from OFF always restores last timing. Loaded in setup. `syncButtonsOn()` uses `nvNavTiming` for navState and button label on power-up. Timing applied to hardware after startup: `handleReceivedData()` hooks Bridge's `status=2` (startup complete) message to send the appropriate `LED_BLINK 1000/1500` or `LED_ON` to both Bridge and EngRoom nav — `ledsAreOn` guard ensures shutdown-complete (also `status=2`) is skipped.

**Pending:**
- LD2410C UART config (deferred — sensor housing being made, wire UART when housing built)

**Previous session (21, 2026-05-25):**

**Changes — GitHub prep pass:**

- **Bridge serial debug removed** (good-to-have #3): All 38 `Serial.*` calls removed from `Bridge_ESP.ino`. `OnDataSent` callback removed (was Serial-only). `last_ota_time` global removed. OTA `.onStart`/`.onEnd`/`.onProgress`/`.onError` callbacks removed (all Serial-only) — only `setHostname("Bridge")` kept. Bridge is sealed in model with no USB access; serial output went nowhere.
- **`soundInit()` simplified** (`Bridge_ESP.ino`): Probe query, response loop, and unused `ok` variable removed. Now just `delay(500)` (module settle) + `soundSetVolume()`.
- **DataPad dead variable removed** (`Data_Pad.ino`): `bool effSimpleOn = false;` deleted — declared but never read; `LED_EFFECTS_SIMPLE` is sent directly to EngRoom from a UI button with no local state tracking needed.
- **DataPad `// FIX:` comment cleanup** (`Data_Pad.ino`): Six stale development-era `// FIX:` prefixes resolved — two deleted entirely (referenced old line numbers, described already-fixed code), four had prefix stripped (useful WHY content kept).
- **Sound attribution** (`SOUND_MAP.md`): Added "Sound Sources" section — trekcore.com/audio/ credited for all Star Trek clips. Note added that any clips can be substituted as long as slot numbers match. Star Wars files (43–44) documented as user-supplied / not included in repo.
- **`.gitignore` created**: Excludes `sounds/`, `libraries/`, `_BACKUP/`, `.claude/`, `Data_Pad_240x320/`, `COST_TRACKER.csv`, `*.lnk`, `Thumbs.db`, `.DS_Store`.
- **Deleted obsolete files**: `Data_Pad/red_alert_png.h` (unused — only `red_alert_img.h` is included), `UI/` folder (SquareLine Studio reference export, not needed), `esp32_unit1/` (early prototype headers), `enterprise documentation/WarpCore/WarpCore.ino` (old version — live version is `WarpCore_ESP32/WarpCore_ESP32.ino`).
- **Bridge compile size noted**: 82% flash (1,074,880 / 1,310,720 bytes), 15% RAM. Sufficient headroom for a minimal REST API web server using the built-in `WebServer` library if wanted later. Heavier options (ESPAsyncWebServer, served HTML UI) would be tight in the remaining 230KB.

**Previous session (20, 2026-05-24):**

- **Easter egg sound buttons (Screen 1):** Two existing decorative LCARS sidebar buttons on Screen 1 made secretly functional — no labels, no visual change. `ui_Button38` (top purple strip, 87×32, y=-218) plays a random funny clip (files 39–42: self-destruct, unable to comply, not authorised) via `EasterEggFunny()`. `ui_Button35` (bottom orange block, 87×80, y=195) plays a random Star Wars track (files 43–44: main theme, Imperial March) via `EasterEggStarWars()`. Both use Arduino `random()` — no cycle index, every tap is random. No `play_touch_sound()` call (would reveal the hidden button). Flag pattern changed from decorative (strips CLICKABLE+PRESS_LOCK+etc.) to functional (strips SCROLLABLE only). Changes in: `ui_Screen1.c` (flags + event stubs + event_cb registrations), `ui_Screen1.h` (extern declarations), `Data_Pad.ino` (handler functions after `SoundExtras()`), `ui_events.h` (forward declarations).
- **Screen 2 button color cleanup:** Removed harsh red and orange colors. All Off: `0xCC2222` → `0xCC99CC`. Orange (`0xFF9C00`) buttons replaced with alternating softer colors: Bridge ASM, WarpCore, Alert Sensor, Windows → `0xFF9966`; EngRoom ASM, Sensors, Main Menu → `0xCC99CC`. No position or label changes. `ui_Screen2.c` only.

**Previous session (19, 2026-05-24):**

- **Screen 2 full layout redesign (reference-based):** User created a SquareLine Studio reference file (`UI/ui_Screen2.c`) with exact button/slider positions. `Data_Pad/ui_Screen2.c` completely rewritten to match.
  - **Sidebar/header** restored to original Screen2 LCARS style: orange/purple blocks at x=-354, 12 thin header strips, "SYSTEM SETUP" title at montserrat_40. Replaced the BridgeSound-style sidebar that was used in the previous attempt.
  - **Sliders:** Two native LVGL vertical sliders (w=25, h=150 — height > width triggers native vertical mode). `ui_BridgeVolSlider` at x=-238, `ui_PadVolSlider` at x=-288, both at y=68 (centered in lower half of screen), labels at y=-30. Orange indicator and knob. Min=0 at bottom, max=30 at top. Replaced the rotated `transform_angle` approach (invisible on hardware) and the previous black-cutout horizontal sliders.
  - **Button grid:** 5×5 grid (cols x=-148/-28/93/217/332, rows y=-41/20/62/117/184), 105×45 pills, 9 empty slots. PAD AUDIO at Col3 Row2 (x=93, y=20); Sync Mode at Col2 Row3 (x=-28, y=62) — swapped from initial reference placement.
- **Screen 1 button position updates (reference-based):** Applied 8 position-only patches to `Data_Pad/ui_Screen1.c`: Power, Photons, Settings, Nav Lights, Impulse Eng, Damage Control, Deflector, Red Alert all moved to reference coordinates. Impulse Eng color changed `0x982205` → `0xCC99CC`; Deflector label changed `"DEFLECT"` → `"DEFLECTOR"`.

**Previous session (18, 2026-05-24):**

- **Bug fix — WindowsToggle wrong target:** `WindowsToggle()` was sending `GRP_ALL_WINDOWS` to `broadcastAddress1` (EngRoom). EngRoom has no windows group and silently ignored the command — WINDOWS OFF had no effect. Fixed: changed to `sendLedCmdTo(broadcastAddress2, GRP_ALL_WINDOWS, ...)` (Bridge).
- **Bug fix — Warp ambient plays weapon sounds:** The warp ambient timer (fires 8s after activation) was sending `SND_CMD_REPEAT=1` + `SND_CMD_PLAY=37` to Bridge. Bridge has files 1–25 only — file 37 is constrained to 25 (`tng_weapons_clean`), which then looped. Fixed: removed Bridge-side sends from the ambient block entirely. Warp ambient is now DataPad only — calls `padSndPlay(37)` and re-arms every 17s.
- **AllOn() instant-on:** Changed from `LED_STARTUP` animated sequence to immediate all-on. Bridge: `LED_ON GRP_ALL_WINDOWS` (windows), `LED_ON GRP_BOTH_ENGINES` (impulse engines), `LED_BLINK GRP_ALL 0` (nav at saved interval). EngRoom: `LED_ON GRP_ALL` (windows), `LED_BLINK GRP_ALL 0` (nav), `LED_ON IDX_ENGROOM_DEFLECTOR`, `LED_ON IDX_ENGROOM_IMPULSE`, `LED_ON GRP_BOTH_NAC` (nacelle ramp). WarpCore: `LED_ON`. No startup sounds — this is a bypass switch, not a startup sequence.
- **External power supply — no-mod path identified:** The neck battery box area has a matching cutout directly above it in the saucer section. Ribbon cables (same style as neck/EngRoom FFC runs) can be routed through that gap to carry external power into the saucer without any structural modifications. A connector on one of the boards is all that would be needed. No PCB layout changes — connector addition only. Noted for future display-mode power work.

**Pending from session 18 (now complete):**
- ~~DataPad reflash~~ — **DONE session 19.**
- ~~Button layout pass (deflector/impulse on Screen 1 need to move down to clear LCARS border)~~ — **DONE session 19.**

**Previous session (17, 2026-05-24):**

- **Red alert screen:** Full-screen black background with 247×204 image at 2× zoom, centred on 800×480. Tap anywhere to dismiss. Auto-dismisses after 60 seconds. `tng_red_alert1` (file 28) plays on trigger and replays every 16 seconds. `ui_RedAlert.c/.h` new files; `red_alert_img.h` contains raw RGB565 pixel data (100KB, converted from PNG via Python zlib/struct). `LV_USE_PNG` stays at 0 — `LV_IMG_CF_TRUE_COLOR` with pre-converted pixel data used instead.
- **RED ALERT button (Screen 1):** `ui_RedAlertBtn` at x=230, y=165 (same row as Settings). 105×45, radius=700, red (`0xCC0000`), white text "RED\nALERT", font montserrat_10. Calls `RedAlertStart()`.
- **Alert sensor toggle (Screen 2):** `ui_RadarAlertBtn` at x=340, y=-150. Controls whether the LD2410C presence trigger fires red alert when model is already on. Separate from `ui_RadarBtn` (master sensor on/off). State saved to NVS `"datapad"/"alert_radar"`. Label: "ALERT\nSENSOR ON" / "ALERT\nSENSOR OFF". `RadarAlertToggle()` handler in `Data_Pad.ino`.
- **Windows toggle button (Screen 2):** `ui_WinBtn` at x=340, y=-70. Sends `LED_ON`/`LED_OFF` with `ledGroup=18` (`GRP_ALL_WINDOWS`) to Bridge independently of the startup/shutdown sequence. `WindowsToggle()` handler. Label: "WINDOWS\nON" / "WINDOWS\nOFF". `syncButtonsOff()`/`syncButtonsOn()` updated to sync `windowsOn` state and button label.
- **Radar trigger updated:** When model is already on and `redAlertSensorEnabled` is true, a new sensor presence event fires `RedAlertStart()` instead of being ignored. The `redAlertActive` guard prevents re-triggering while alert is running.
- **State vars added:** `redAlertActive`, `redAlertSensorEnabled`, `redAlertAutoOffMs`, `redAlertNextSndMs`, `windowsOn`. Loaded from NVS in setup where applicable.

**Previous session (16, 2026-05-24):**

- **Struct cleanup (all 4 sketches):** Removed legacy `a` and `r` fields from `struct_message`. Fields were unused ARGB debug remnants from early development. Bridge, EngRoom, DataPad, and WarpCore all updated and reflashed. CLAUDE.md struct updated; heading corrected from "ALL THREE" to "ALL FOUR SKETCHES" now that WarpCore is a full mesh member.
- **DESIGN_STORY.md updates:** Clarified WarpCore is a community 3D print sourced from Thingiverse — not part of the original subscription kit. Sensor housing plan updated: will be integrated into the WarpCore's existing 3D printed base as a CAD modification, not a separate external housing. Future Additions entry corrected to reflect completed integration and current architecture.

**Previous session (15, 2026-05-24):**

- **LD2410C radar trigger architecture change (WarpCore + DataPad):** WarpCore no longer sends `LED_STARTUP` directly to Bridge and EngRoom on presence detection. Now sends `LED_RADAR_TRIG=30` to DataPad only. DataPad checks `ledsAreOn` and calls `PowerUpAll(NULL)` if model is off. Prevents trigger loops (someone standing in front of sensor), keeps all on/off logic in DataPad which has authoritative state. 3s lockout on WarpCore side still applies. New constants: `LED_RADAR_EN=29` (DataPad→WarpCore: enable/disable sensor), `LED_RADAR_TRIG=30` (WarpCore→DataPad: presence detected). Both defined in DataPad and WarpCore only — Bridge and EngRoom untouched.
- **Sensor toggle button (DataPad Screen 2):** `ui_RadarBtn` pill button at x=340, y=10 (right column alongside EngRoomStatusBtn row, above WarpCoreBtn). Labels: SENSORS ON / SENSORS OFF. `RadarToggle()` handler saves to NVS `"datapad"/"radar_on"` and sends `LED_RADAR_EN` to WarpCore. WarpCore saves to NVS `"warpcore"/"radar_on"` on receive. Both units persist state independently across reboots. Default: enabled.
- **AllOff/AllOn/PowerUpAll button state sync (DataPad):** `AllOff()` previously only sent `LED_ALL_OFF` to Bridge and EngRoom — `ledsAreOn` was never cleared, causing a lockout where POWER UP ALL tried to shut down instead of start. Fixed with two static helpers `syncButtonsOff()` and `syncButtonsOn()` that update all subsystem state vars and button labels in one call. `syncButtonsOff()` also calls `stopWarpSounds()` and resets the warp slider. `AllOff()` now also sends `LED_SHUTDOWN` to WarpCore (was missing). `AllOn()` now also sends `LED_STARTUP` to WarpCore (was missing) and calls `syncButtonsOn()`. `PowerUpAll()` startup/shutdown both call the appropriate helper — button labels for NAC/NAV/ENG/DEFLECT/WARP CORE all update on power up and down.
- **WarpSpeed auto-on WarpCore:** If the warp slider is moved from 0 while `warpCoreOn=false`, `LED_ON` is sent to WarpCore and `warpCoreOn` + button label update immediately. Does not trigger full `PowerUpAll`.
- **LD2410C sensor notes:** Detection range currently short — sensor config requires UART wired (only OUT/D7 is wired now). Future plan: wire UART to old Arduino Nano as USB serial passthrough, use HLKRadarTool PC app to configure detection gates and sensitivity. When sensor housing is made, run UART wires back to WarpCore for firmware-based config.

**Previous session (13, 2026-05-22):**
- `LED_WARP=28` added to all 3 sketches + WarpCore. EngRoom: nacelle warp mode with dim-flash-ramp startup sequence (3s dim to 25%, 1s ramp to 100%, then steady hold). Brightness scales with speed (80% at slider 1, 100% at slider 10). Flash only on first activation; speed changes while warp is active adjust brightness without flash. Normal nacelle pulse: period 5000ms, minDuty=80, maxDuty=115 (45% ceiling). Deflector: 60% steady during normal, 100% during warp. DataPad: `WarpSpeed()` handler, vertical orange warp slider on right side of Screen 1. NAC button overrides warp. Warp sounds: tng_warp4_clean (file 14) on activation via `playToDest()`, tng_engine_1 (file 37) loops after 8s (sound finishes). Engine ambient repeats every 17s on DataPad (track length), Bridge uses native `SND_CMD_REPEAT`. `stopWarpSounds()` cleans up on deactivation. WarpCore: ESP-NOW receive skeleton — MAC printed on boot, pot fallback when not connected.

**Previous session (12, 2026-05-22):**

**Changes this session:**
- WiFi Screen 3 text area pre-population: `populateWifiFields()` reads saved SSID from NVS on Screen 3 load — shows saved SSID and `"*****"` for password. Defaults ("enterprise"/"ncc-1701-d") stay when nothing saved. `SaveWifi()` password guard reads real password from NVS if field shows `"*****"`. DataPad only.
- WiFi status heartbeat fix: Bridge and EngRoom now include `wifiStatus` and `ipAddress` in their 30s heartbeat when `wifiActive && WiFi.status() == WL_CONNECTED`. DataPad no longer misses the one-shot status message — refreshes every 30s. Confirmed working.
- SoftAP concept documented in DEVNOTES #12 and DESIGN_STORY.

**Previous session (11, 2026-05-22):**
- Pending #13: Power-up status indicator implemented. Bridge: nav + G14 + G15 blink at 500ms. EngRoom: nav + both nacelles blink at 500ms. Duration: 20 seconds or until `LED_STARTUP` received from DataPad.

**Previous session (10, 2026-05-22):**
- Good-to-have #4: WiFi auto-revert implemented and confirmed working. All 3 units auto-revert to ch=1 after 90s of dropped WiFi. DataPad shows "WiFi lost — reverted" on Screen 3.
- Bug fix: status buttons force-OFFLINE on DataPad auto-revert; units come back ONLINE within 30s via heartbeat.
- Battery runtime documented in HARDWARE.md: ~2.5 hrs active use, Bridge (one cell set only) to 3.33V/cell, EngRoom to 3.66V/cell.

**Previous session (9, 2026-05-22):**
- Fix: EngRoom auto-starting after being powered off — `intentionallyOff` flag added.

**Previous session (8, 2026-05-22):**

**Changes this session:**
- Assembled/Separated sync mode button shows `PART ONLINE` (amber) when mode is assembled but one unit is offline. `updateSyncModeBtn()` helper called from watchdog, first-contact, and toggle.
- Fix #2: Photon timing tuned — `PHOTON_CHARGE_MS 750`, `PHOTON_FIRE_MS 200`. EngRoom only, confirmed on hardware.
- Deflector toggle button added to Screen 1 (good-to-have #1 done).
- ALL OFF / ALL ON buttons added to Screen 2 (good-to-have #2 done).
- Touch sounds on all Screen 1 and Screen 2 button presses — `play_touch_sound()`, ON/OFF toggle + NVS on Screen 2 (good-to-have #6 partial done).
- Ambient auto-play: starts on POWER UP, software polling every 30s, files 33–37 only, ON/OFF toggle + NVS on Screen 2. Touch sound interruption handled with 2s resume timer (good-to-have #6 full done).
- Button layout tweaks deferred — deflector and impulse need to move down to clear LCARS border.

**Previous session (7, 2026-05-22):**
- Assembled/Separated sync mode implemented (`LED_SYNC_MODE=25`). DataPad `PowerUpAll()` routes startup/shutdown based on which units are online and current mode. Bridge and EngRoom store mode in NVS (`sync_mode` bool). DataPad stores preference in NVS (`sync_mode` int, namespace `"datapad"`).
- Bridge and EngRoom status indicator buttons added to Screen 2 — show ONLINE (orange) / OFFLINE (dark blue-grey). Non-interactive, colour updated live from watchdog.
- ASSEMBLED/SEPARATED toggle button added to Screen 2. Broadcasts `LED_SYNC_MODE` to both units on press, persists to NVS.
- Sound fallback: when Bridge is offline, DataPad plays startup/shutdown sound locally via `padSndPlay()`.
- Fix #4: Nacelle override bug fixed in EngRoom — `LED_ON` handler and `updateStartup()` nacelle step now check `nacelleActive`/`nacelleRamping` before resetting the ramp. Eliminates brightness dip when nacelles already running.

**Immediate hardware tasks:**
- [ ] Neck PCB + EngRoom PCB revision in progress (2026-05-21) — order and fit new boards when revision complete. **Current boards unchanged — no firmware changes until new boards fitted.**
  - **Neck PCB:** separate nav/photon resistors (both 1.2kΩ), added neck nav transistor, removed pull-downs, base resistors changed 10kΩ→1.2kΩ, connector footprint reversed, board layout and mounting hole locations updated to match original model.
  - **EngRoom PCB:** FFC connector updated to carry GPIO 4 control signal to neck nav transistor (was post-transistor collector output). Voltage regulator moved away from centre standoff (caps moved with it). Battery voltage monitor added on GPIO 2 (ADC1) — voltage divider R1=270kΩ, R2=100kΩ, covers 6.0V–8.4V range into 1.62V–2.27V on ADC. Neck windows (10 groups, 3 transistors) combined onto one GPIO signal — all 3 transistor bases tied to same net, no component changes. All base resistors changed 10kΩ→1.2kΩ. Pull-down resistors removed.
  - Left nacelle connector footprint corrected to match physical wiring layout.
  - **New GPIO assignments (new boards only):** neck windows → GPIO 20, left nacelle → GPIO 8, right nacelle → GPIO 9. Firmware update required when new boards fitted — do not change firmware until then.
- [ ] EngRoom firmware update for new board GPIO assignments — when new boards fitted: neck windows GPIO 20, left nacelle GPIO 8, right nacelle GPIO 9. Also add ADC battery monitor read on GPIO 2 (`ADC_11db` attenuation, voltage divider 270kΩ/100kΩ). All 3 units may need reflashing if struct changes.
- [ ] Bridge PCB revision in progress (2026-05-21) — order and fit new boards when revision complete. **Current boards unchanged — no firmware changes until new boards fitted.**
  - **PCB 2:** B0530W Schottky diode (SOD-123, 0.5A 30V) added on each 3.3V regulator output — fixes USB back-feed through battery regulator when battery is off. USB-C D+ and D- pins corrected (were swapped on original layout, required physical workaround).
  - **PCB 3:** Non-UART mode resistors removed from DY-SV17F mode selection array — only UART configuration resistors retained.
  - **All boards:** Group 9 pin now correctly split between groups 9 and 1 (was only connected to group 9). Battery voltage monitor added on GPIO 2 (ADC1) — voltage divider R1=270kΩ, R2=100kΩ, tapped from local supply rail, close to GPIO 2. Maps 6.0V–8.4V → 1.62V–2.27V on ADC.
  - **Firmware update required when new boards fitted:** add ADC battery monitor read on GPIO 2 (`ADC_11db` attenuation).

**Firmware fix queue:**
- [x] Fix #2: Photon torpedo timing — **FIXED 2026-05-22.** `PHOTON_CHARGE_MS 750`, `PHOTON_FIRE_MS 200`.
- [x] Fix #4: Nacelle control overridden during/after startup — **FIXED 2026-05-22** in `Engine_Room_ESP.ino`

---

## Fix Queue

### Fixes — confirmed broken

| # | Issue | Symptoms | Suspected cause |
|---|-------|----------|-----------------|
| ~~1~~ | ~~Photon torpedo sound not playing~~ | **FIXED 2026-05-20** — confirmed working after reflash. Full peer mesh allowing direct EngRoom→Bridge communication resolved the delivery issue. | |
| ~~2~~ | ~~Photon torpedo effect timing needs adjusting~~ | **FIXED 2026-05-22** — `PHOTON_CHARGE_MS 750`, `PHOTON_FIRE_MS 200`. Confirmed good on hardware. |
| 5 | EngRoom auto-starts after being powered off | After `LED_SHUTDOWN`, EngRoom sits at `EFF_IDLE`. If DataPad goes quiet for `connLostMs` (default 5 min), the connection-lost handler fires and — because `EFF_IDLE` matches its trigger condition — calls `LED_STARTUP`, turning EngRoom back on. **FIXED 2026-05-22** — `intentionallyOff` bool added. Set on shutdown completion and `LED_ALL_OFF`; cleared on `LED_STARTUP`. Connection-lost auto-restart and nav blink both suppressed when flag is set. EngRoom only. |
| ~~3~~ | ~~Nav LED on lower EngRoom board fires when photon fires~~ | **FIXED 2026-05-21** — resolved by neck PCB revision (good-to-have #5). Second inline resistor on neck PCB separates nav and photon current paths, eliminating backfeed. No firmware change needed. |
| ~~4~~ | ~~Nacelle control blocked/overridden during/after startup~~ | **FIXED 2026-05-22** — `updateStartup()` nacelle step and `LED_ON` nacelle handler both now check `nacelleActive`/`nacelleRamping` before resetting the ramp. Brightness dip eliminated. `Engine_Room_ESP.ino` only. |

### Hardware fixes — PCB revision required

| # | Issue | Fix |
|---|-------|-----|
| ~~H1~~ | ~~Bridge PCB 2 EN pin RC circuit reversed + unnecessary caps~~ | **FIXED 2026-05-28.** Schematic had 3.3V → cap → EN → resistor → GND (reversed). EN was held low through the resistor while cap charged — unreliable boot. Also: 100nF cap across reset switch removed (unnecessary). Correct circuit: 3.3V → 10kΩ → EN pin → pushbutton → GND. EasyEDA updated. Confirmed working with new boards. |

### Good-to-haves — enhancements for later

| # | Feature | Notes |
|---|---------|-------|
| ~~1~~ | ~~Deflector standalone DataPad button~~ | **DONE 2026-05-22.** `ui_DeflectorBtn` on Screen 1 at x=230, y=-39 (final position after layout pass session 19). Label "DEFLECTOR" / "DEFLECTOR\nON". Color 0xFF9C00 orange. `DeflectorToggle()` in `Data_Pad.ino`. `IDX_ENGROOM_DEFLECTOR=6` sends `LED_ON`/`LED_OFF` to EngRoom. |
| ~~2~~ | ~~All Off / All On toggle button on Screen 2~~ | **DONE 2026-05-22.** Two pill buttons on Screen 2 (`ui_AllOffBtn`, `ui_AllOnBtn`). `AllOff()` broadcasts `LED_ALL_OFF=99` to both units instantly. `AllOn()` turns everything on instantly without startup animation — sends `LED_ON`/`LED_BLINK` to all subsystems directly (session 18). No sounds — functions as a bypass switch. |
| ~~3~~ | ~~Remove serial debug from Bridge~~ | **DONE 2026-05-25.** All 38 `Serial.*` calls removed. `OnDataSent` callback, OTA progress callbacks, and `last_ota_time` global also removed. `soundInit()` probe loop cleaned up. Bridge and DataPad reflash pending. |
| ~~4~~ | ~~WiFi auto-revert after disconnect timeout~~ | **DONE 2026-05-22.** `WIFI_REVERT_MS=90000`. Bridge/EngRoom: `wifiActive`/`wifiDropMs` globals + watchdog in `loop()`. DataPad: `revertWifiLocal()` helper extracted from `SaveWifi()`, watchdog in `loop()`. Also adds missing `esp_wifi_set_channel(1,...)` to DataPad's manual revert path. DataPad forces Bridge/EngRoom status to OFFLINE on auto-revert; they return via heartbeat within 30s. All 3 units reflashed and confirmed working. |
| ~~5~~ | ~~EngRoom: separate resistors for nav and photon LEDs~~ | **DONE 2026-05-21** during neck PCB revision. Second resistor added inline on neck PCB — 1.2kΩ on white nav LED, 2.2kΩ on red photon LED. Fixes nav dimming and photon-to-nav backfeed on lower EngRoom board. No firmware change needed. |
| ~~6~~ | ~~DataPad touch sounds + ambient sound triggers~~ | **DONE 2026-05-22.** Touch sounds: `play_touch_sound()` called from every `LV_EVENT_CLICKED` wrapper on Screen 1 and Screen 2 (not Screen 3 or BridgeSound). ON/OFF toggle + NVS (`"touch_snd"`) on Screen 2. Ambient: auto-starts on POWER UP, software polling every 30s via `ambientNextPlayMs` in `loop()`. Files 33–37 only (32 and 38 excluded — too short). ON/OFF toggle + NVS (`"ambient_on"`) on Screen 2. Touch sounds interrupt ambient; ambient resumes 2s after click via `ambientResumeMs` timer. DY1703A hardware loop commands (0x0C, 0x11, 0x33) all tested — none worked; software polling used instead. |

---

## ⚠️ CRITICAL WARNING — WiFi + ESP-NOW Channel Conflict

**This is the single most disruptive issue on this project. Read before touching any WiFi code.**

### What happens

ESP-NOW and WiFi share the same radio on the ESP32. When a unit connects to a WiFi AP, the radio moves to that AP's channel (e.g. ch=11). Any other unit that is NOT connected to WiFi (or connected to a different AP) will be on a different channel (e.g. ch=1). ESP-NOW between units on different channels **silently fails** — sends return FAIL with no useful error.

### Multi-AP / mesh networks make this worse

If your router uses multiple access points (mesh system, or a dual-band/tri-band router with the same SSID on multiple radios), each unit may connect to a **different physical AP on a different channel** even though they all use the same SSID and password. All three units report "connected" and the channels look fine individually, but they are not the same channel and ESP-NOW fails.

**Confirmed failing setup:** home mesh network — DataPad lands on ch=11, Bridge and EngRoom land on ch=1.  
**Confirmed working setup:** mobile hotspot (single AP, single channel) — all units land on same channel, ESP-NOW works.

### Safe operating procedure

- **Normal use:** keep WiFi auto-connect **OFF** on all three units. ESP-NOW runs on ch=1, everything works reliably.
- **OTA updates:** use a single-AP hotspot (not a mesh network). Press SaveWifi on DataPad → all three connect → OTA → press SaveWifi again to clear and revert.
- **Never leave `wifi_auto=true` saved on all three units** when deploying — they will auto-connect on every boot and may land on different channels.

### Symptoms of this problem

- ESP-NOW sends show `Send status: FAIL` in Serial Monitor
- Connection watchdog on DataPad shows Bridge/EngRoom offline even though they just booted
- Heartbeat pings time out
- Channel debug output shows mismatched `ch=` values between units

### Recovery

If a unit gets stuck with `wifi_auto=true` saved and is boot-looping on WiFi connect:
- Flash `Bridge_NVS_Clear/Bridge_NVS_Clear.ino` (Bridge) or `EngRoom_NVS_Clear/EngRoom_NVS_Clear.ino` (EngRoom)
- Wait for "Done" in Serial Monitor (115200)
- Reflash the main sketch
- For DataPad: press SaveWifi button a second time (the clear/revert path)

---

## Fix History

### Session 13 — Warp Effect, Sounds, Nacelle/Deflector Tuning (2026-05-22)

**EngRoom + DataPad reflashed and verified. WarpCore sketch updated (not yet connected).**

#### Warp effect — EngRoom

New `LED_WARP=28` command. `nacelleWarpMode`, `nacelleWarpSpeed` (1–10), `nacelleWarpPhase` (0/1/2), `nacelleWarpPhaseMs` added.

Three-phase startup sequence on first activation:
- Phase 0: snap to 25% (64/255), hold 150ms
- Phase 1: linear ramp 25%→100% over 400ms
- Phase 2: steady hold at mapped brightness (80%–100% based on speed)

Speed changes while warp is already active skip the flash and go straight to phase 2. Flash only on first activation (off→warp). Clearing warp (`LED_WARP val=0`) returns nacelles to normal sine mode.

#### Normal nacelle pulse

Period 5000ms (slowed from 2500ms). minDuty=80, maxDuty=115 (45% ceiling) — subtle slow breathing at low brightness, leaving headroom for warp to be visibly brighter.

#### Deflector tied to warp

`updateDeflectorGlow()` simplified: 60% steady (153) during normal operation, 100% (255) during warp (`nacelleWarpMode`). Previous 6-second sine glow removed.

#### DataPad — warp slider

`WarpSpeed()` handler, `warpSpeedVal` global. Vertical orange LVGL slider on right side of Screen 1 (x=320, y=10, w=20, h=250). LCARS orange (`0xFF9C00`) on indicator and knob, dark track. Label "WARP" above.

NAC button coordinates with warp: both ON and OFF presses reset the slider to 0 and send `LED_WARP=0`. Slider moving > 0 sets `nacelleOn = true`; slider at 0 sets `nacelleOn = false`.

#### WarpCore sketch

ESP-NOW receive skeleton added to `WarpCore_ESP32/WarpCore_ESP32.ino`. Shared `struct_message` struct added. `OnDataRecv()` checks for `LED_WARP` command and sets `remoteSpeed`. `getDelay()` uses remote speed when `remoteOverride = true`, otherwise pot as before. MAC address printed to Serial on boot at 115200. Power-cycle to revert to pot control.

#### Warp sounds

`tng_warp4_clean` (file 14) plays via `playToDest()` on warp activation — respects `padAudioDest`. Flash sequence retimed to match sound: 3s dim phase, 1s ramp phase. After 8s (sound finishes), `tng_engine_1` (file 37) starts as looping ambient. Bridge: `SND_CMD_REPEAT, 1` + play (native loop). DataPad: software poll every 17s (track length, same DY1703A no-native-repeat workaround). `stopWarpSounds()` helper clears both on deactivation — stops playback, clears Bridge repeat, zeroes `warpAmbientMs`. `warpAmbientMs` global tracks next ambient trigger. Timing noted as close; minor tweaking deferred.

**Files changed:** `Engine_Room_ESP/Engine_Room_ESP.ino`, `Data_Pad/Data_Pad.ino`, `Data_Pad/ui_Screen1.c`, `Data_Pad/ui_Screen1.h`, `Data_Pad/ui_events.h`, `Bridge_ESP/Bridge_ESP.ino`, `WarpCore_ESP32/WarpCore_ESP32.ino`, `CLAUDE.md`, `DEVNOTES.md`

---

### Session 12 — WiFi Screen Improvements (2026-05-22)

**All 3 units reflashed and verified.**

#### WiFi text area pre-population (DataPad)

`populateWifiFields()` added to `Data_Pad.ino` and declared in `ui_events.h`. Called at end of `ui_Screen3_screen_init()`. Reads `wifi_ssid` from NVS (`"datapad"` namespace) — if saved, sets TextArea1 to the SSID and TextArea2 to `"*****"`. If nothing saved, hardcoded defaults ("enterprise"/"ncc-1701-d") remain.

`SaveWifi()` password guard: if `lv_textarea_get_text(ui_TextArea2)` returns `"*****"`, reads real password from NVS instead of saving the masked string.

#### WiFi status heartbeat (Bridge + EngRoom)

`StartWiFi()` sent `wifiStatus` + `ipAddress` only once on connect. If that message was lost during the channel transition, DataPad never showed the connection. Fixed by adding WiFi status to the 30s heartbeat: when `wifiActive && WiFi.status() == WL_CONNECTED`, the ping includes `wifiStatus = 5` (Bridge) or `wifiStatus = 3` (EngRoom) and the current IP. DataPad `handleReceivedData()` already handled these fields — no DataPad change needed.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `Data_Pad/Data_Pad.ino`, `Data_Pad/ui_Screen3.c`, `Data_Pad/ui_events.h`

---

### Session 11 — Power-Up Status Indicator (2026-05-22)

**Bridge and EngRoom only. DataPad unchanged.**

Non-blocking boot indicator runs in `loop()` for 20 seconds after `setup()` completes, or until `LED_STARTUP` is received. Blinks at 500ms intervals so the flashing is clearly distinct from a frozen startup sequence.

**Bridge:** nav light + window groups G14 (pinArray idx 13, GPIO 3) and G15 (pinArray idx 14, GPIO 9). Controlled via `ledcWrite(PIN_NAV, ...)` and `winWrite(13/14, ...)`.

**EngRoom:** nav light + both nacelles (`PIN_NAC_R` GPIO 2, `PIN_NAC_L` GPIO 3). All three blink together via `ledcWrite`.

State: `bootIndicatorActive` (bool), `bootIndicatorStartMs` (uint32_t), `bootIndicatorBlinkMs` (uint32_t), `bootIndicatorPhase` (bool). All set at end of `setup()`. The `LED_STARTUP` handler clears `bootIndicatorActive` — `allOff()` that follows handles the LED cleanup, so no duplicate off-writes needed.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `DEVNOTES.md`

---

### Session 10 — WiFi Auto-Revert (2026-05-22)

**All 3 units reflashed and verified.**

#### WiFi auto-revert

Added a 90-second watchdog on all three units that monitors `WiFi.status()` after a successful connection. If the AP drops and doesn't come back within 90 seconds, each unit independently calls its revert function — disconnects WiFi, restores ch=1, and ESP-NOW resumes.

On Bridge and EngRoom: `wifiActive` bool + `wifiDropMs` timestamp added. `StartWiFi()` sets `wifiActive = true` on success; `RevertWiFi()` clears it. Watchdog runs in `loop()` after the `startWifi1` dispatch.

On DataPad: revert logic extracted from `SaveWifi()`'s second-press branch into `revertWifiLocal()`. The extracted helper also adds the missing `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)` call that the original inline path lacked. Both manual second-press and auto-revert call the same helper. Watchdog added after the WiFi state machine block in `loop()`.

#### Status button bug fix

After auto-revert, DataPad's Bridge/EngRoom status buttons were staying ONLINE. Root cause: `UNIT_OFFLINE_MS` (90s) and `WIFI_REVERT_MS` (90s) are the same, so units reverted and resumed pinging before the offline watchdog could flip them dark. Fix: at the moment of auto-revert, DataPad explicitly zeros `lastBridgeMs`/`lastEngRoomMs` and sets `bridgeOnline`/`engRoomOnline = false`, immediately updating the status buttons and `ui_Label8` to OFFLINE. Units return to ONLINE within 30 seconds via normal heartbeats once ESP-NOW resumes on ch=1.

Bridge and EngRoom don't send an explicit "reverted" message — DataPad picks them back up automatically through the existing 30s heartbeat mechanism.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `Data_Pad/Data_Pad.ino`, `HARDWARE.md`, `DEVNOTES.md`, `DESIGN_STORY.md`

---

### Session 9 — EngRoom Auto-Start Bug Fix (2026-05-22)

**EngRoom only. No Bridge or DataPad changes.**

#### Root cause

After `LED_SHUTDOWN`, EngRoom's `currentEffect` is `EFF_IDLE` with all LEDs off. DataPad sends keepalive heartbeats every 30 seconds, so `lastContactMs` stays fresh while DataPad is running. But if DataPad goes quiet — battery dies, powered off, or channel drift — the connection-lost handler fires after `connLostMs` (default 5 minutes). The handler checks `currentEffect == EFF_IDLE && !nacelleActive && !blinkActive`, all of which are true after a clean shutdown, so it calls `LED_STARTUP` and turns EngRoom back on.

Pressing ALL OFF temporarily masked the problem by sending an ESP-NOW message to EngRoom, refreshing `lastContactMs` and resetting the 5-minute timer — it wasn't actually preventing the auto-start, just delaying it.

#### Fix

Added `bool intentionallyOff = false;` to track whether EngRoom was explicitly told to shut down.

- Set to `true`: in `updateShutdown()` when `seqStep < 0` (sequence complete), and in the `LED_ALL_OFF` handler.
- Cleared to `false`: in the `LED_STARTUP` handler (any startup command means active again).
- In the connection-lost handler: the auto-restart and nav blink are both wrapped in `if (!intentionallyOff)`. When the flag is set, the unit stays completely dark regardless of how long DataPad is quiet.

**Files changed:** `Engine_Room_ESP/Engine_Room_ESP.ino`, `DEVNOTES.md`

---

### Session 8 — UI Additions, Touch Sounds, Ambient, Fix #2 (2026-05-22)

**DataPad only. No Bridge or EngRoom reflash needed.**

#### PART ONLINE indicator on sync mode button

`updateSyncModeBtn()` helper function added. When `nvsSyncMode == 1` (assembled) but only one unit is reachable, button shows `"PART\nONLINE"` in amber (`0xBB8800`) instead of the normal LCARS orange. Updates from three call sites: first-contact in `handleReceivedData()`, watchdog state-change block, and `SyncModeToggle()`.

#### Fix #2: Photon torpedo timing

`PHOTON_CHARGE_MS` changed from 1200 → 750. `PHOTON_FIRE_MS` changed from 250 → 200. EngRoom only, confirmed good on hardware.

#### Deflector toggle button (Screen 1)

`ui_DeflectorBtn` at x=125, y=-85 (above Impulse Engine). `DeflectorToggle()` sends `LED_ON`/`LED_OFF` to EngRoom `IDX_ENGROOM_DEFLECTOR=6`. Follows exact `NacelleToggle()` pattern. Label toggles `"DEFLECT"` / `"DEFLECT\nON"`. Position subject to layout pass — currently overlaps LCARS border slightly.

#### ALL OFF / ALL ON buttons (Screen 2)

`ui_AllOffBtn` and `ui_AllOnBtn` on Screen 2 (x=225 and x=340, y=145). `AllOff()` broadcasts `LED_ALL_OFF=99` to both Bridge and EngRoom (instant `allOff()`, no animation). `AllOn()` sends `LED_STARTUP val=1` to both simultaneously (each unit runs its own full solo sequence). `AllOn()` also updates `ledsAreOn` and the POWER DOWN button label.

#### Touch sounds (Screen 1 + Screen 2)

`play_touch_sound()` called from every `LV_EVENT_CLICKED` wrapper in `ui_Screen1.c` and `ui_Screen2.c`. Not added to Screen 3 (WiFi) or BridgeSound screen. ON/OFF toggle button `ui_TouchSndBtn` on Screen 2 (x=225, y=205). Setting persisted to NVS (`"datapad"/"touch_snd"`, default false).

#### Ambient auto-play (DataPad DY1703A)

Auto-starts on POWER UP ALL (if `ambientEnabled`), stops on POWER DOWN. Also starts/stops immediately when toggled on Screen 2. Software polling: `ambientNextPlayMs` timer in `loop()` re-triggers `padSndPlay()` every 30s (`AMBIENT_POLL_MS`). Files 33–37 only (32 and 38 excluded — too short for 30s polling). Touch sounds interrupt ambient; resumes 2s after click via `ambientNextPlayMs = millis() + 2000` override in `play_touch_sound()`. ON/OFF toggle `ui_AmbientBtn` on Screen 2 (x=340, y=205). Setting persisted to NVS (`"datapad"/"ambient_on"`, default false).

**DY1703A loop command investigation:** Hardware repeat commands `0x33`, `0x11`, and `0x0C` all tested and failed (either no loop or stopped playback entirely). Software polling is the confirmed-working solution for this player. `padSndSetRepeat()` left in code but unused.

**Files changed:** `Data_Pad/Data_Pad.ino`, `Data_Pad/ui_Screen1.c`, `Data_Pad/ui_Screen1.h`, `Data_Pad/ui_Screen2.c`, `Data_Pad/ui_Screen2.h`, `Data_Pad/ui_events.h`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `DEVNOTES.md`, `CLAUDE.md`

---

### Session 7 — Assembled/Separated Sync Mode, Startup Routing Fix, Fix #4 (2026-05-22)

#### Bug fixed: startup/shutdown silently failed when a unit was offline

`PowerUpAll()` previously sent `LED_STARTUP` to Bridge blindly regardless of online status. If Bridge was off, EngRoom never started (it depends on Bridge relaying at seqStep 10). Same problem in reverse for shutdown.

**Root cause:** hard-coded single-target sends with no online check and no fallback routing.

#### Assembled/Separated sync mode — `LED_SYNC_MODE = 25`

Implemented the planned sync mode feature as the proper fix for offline routing.

**DataPad (`Data_Pad.ino`):**
- `nvsSyncMode` global (NVS namespace `"datapad"`, key `sync_mode`, default 1=assembled).
- `PowerUpAll()` rewritten with full routing table. At press time, checks `bridgeOnline`/`engRoomOnline` and `nvsSyncMode` to decide what to send and where. Assembled mode with both online: Bridge gets `LED_STARTUP val=0` and orchestrates the relay to EngRoom. Any unit offline or separated mode: each online unit gets its own direct command.
- `LED_STARTUP ledValue` convention: `0` = assembled relay mode (Bridge relays to EngRoom, skips own nav/impulse); `1` = solo mode (Bridge runs full independent sequence, no relay).
- Shutdown routing mirrors startup: assembled+both online → EngRoom orchestrates → relays `LED_SHUTDOWN` to Bridge at completion. Any offline or separated → each unit gets direct command.
- Sound fallback: if Bridge is offline when POWER UP/DOWN is pressed, DataPad plays startup (`SND_POWER_FIRST`) or shutdown (random `SND_POWER_FIRST+1`–`SND_POWER_LAST`) on its own player via `padSndPlay()`.
- `SyncModeToggle()` handler: toggles `nvsSyncMode`, saves to NVS, broadcasts `LED_SYNC_MODE` to both units.

**Bridge (`Bridge_ESP.ino`):**
- `assembledMode` bool (NVS `"bridge"/"sync_mode"`, default `true`).
- `startupSolo` bool (RAM only, set by `LED_STARTUP` handler from `ledValue`).
- `updateStartup()`: nav and impulse steps run when `!espNowReceived || startupSolo`. EngRoom relay fires only when `!startupSolo`.
- `LED_SHUTDOWN` sound plays when `!espNowReceived || !assembledMode` — ensures Bridge plays its own shutdown sound in separated mode (EngRoom won't send it).
- `LED_SYNC_MODE` case: updates `assembledMode`, saves to NVS.

**EngRoom (`Engine_Room_ESP.ino`):**
- `assembledMode` bool (NVS `"engroom"/"sync_mode"`, default `true`).
- `updateStartup()`: Bridge nav/impulse sync sends (`LED_BLINK`, `LED_ENGINE`) wrapped in `if (assembledMode)`. Nav/impulse still run locally regardless.
- `updateShutdown()`: `LED_SHUTDOWN` relay to Bridge wrapped in `if (assembledMode)`.
- `LED_SYNC_MODE` case: updates `assembledMode`, saves to NVS.

#### Screen 2 new UI elements (`ui_Screen2.c/.h`, `ui_events.h`)

- `ui_BridgeStatusBtn` and `ui_EngRoomStatusBtn` — display-only pill buttons (same style as ASM buttons). Start dark/OFFLINE. Turn LCARS orange when first ping received; revert to dark if watchdog times out.
- `ui_SyncModeBtn` — clickable pill button, wired to `SyncModeToggle`. Label initialised from `nvsSyncMode` at screen creation.
- Status buttons updated in two places in `Data_Pad.ino`: `handleReceivedData()` on first contact, and the watchdog block on any online-state change.

#### Fix #4: Nacelle override bug (`Engine_Room_ESP.ino` only)

Two targeted guards added:
- `LED_ON` nacelle handler: `nacelleRampingDown = false; if (!nacelleActive && !nacelleRamping) { ... start ramp ... }` — skips ramp reset if nacelles already running.
- `updateStartup()` nacelle step: same guard; `nacDone = true` stays unconditional.

#### Files changed

`Bridge_ESP/Bridge_ESP.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `Data_Pad/Data_Pad.ino`, `Data_Pad/ui_Screen2.c`, `Data_Pad/ui_Screen2.h`, `Data_Pad/ui_events.h`, `CLAUDE.md`, `DEVNOTES.md`

**All 3 units need reflashing.**

---

### Session 6 — EngRoom Assembly Complete, Startup Sequence Coordination (2026-05-20)

**Nav/photon LED swap discovered during assembly mode test:**
While swapping boards in the lower EngRoom section, used assembly mode to walk through LEDs. Nav lights were not responding correctly. Traced the problem back to the software pin swap applied last session (2026-05-19) to compensate for the neck board connector being wired backwards — that fix corrected the neck side but reversed the expected layout on the lower EngRoom board, causing nav and photon to appear in each other's positions.

**Current state:** Software swap still in place. Nav and photon LEDs are functional but firmware labels (`PIN_NAV`/`PIN_PHOTON`) do not match physical GPIO assignments.

**Fix plan documented:**
1. Interim: physically re-pin the neck board LED connector so wiring matches firmware, then revert software swap in `Engine_Room_ESP.ino`.
2. Permanent: PCB revision to reverse the connector footprint on the neck board so it is correct by design.

**Assembly completed this session:** EngRoom stardrive section installed in model. All effects confirmed working. Two open hardware issues remain (nav backfeed and nav dimming — both need second resistor on neck PCB).

**Full peer mesh — EngRoom now talks to Bridge directly:**
- Added `bridgeAddress[]` (model MAC) to `Engine_Room_ESP.ino`.
- Bridge added as ESP-NOW peer in EngRoom init and in `StartWiFi()` peer refresh.
- EngRoom shutdown completion now sends `LED_SHUTDOWN` directly to Bridge (no DataPad middleman).

**Startup sequence coordination (final):**
- `Data_Pad.ino`: `PowerUpAll` sends `LED_STARTUP` to Bridge only.
- `Bridge_ESP.ino`: relays `LED_STARTUP` to EngRoom at step 9→10 transition.
- `Bridge_ESP.ino`: steps 10 (nav) and 11 (impulse) skipped when `espNowReceived == true` (connected mode). In standalone mode they run normally so saucer works independently.
- `Engine_Room_ESP.ino`: during startup, at nav step sends `LED_BLINK` to Bridge (resets Bridge nav timer to same moment → both blink in sync). At impulse step sends `LED_ENGINE` to Bridge (both impulse start together).

**Shutdown sequence coordination:**
- `Data_Pad.ino`: `PowerUpAll` sends `LED_SHUTDOWN` to EngRoom only.
- EngRoom runs shutdown in reverse (nacelles → impulse → deflector → nav → eng sections → neck).
- `Engine_Room_ESP.ino`: when shutting off nav, also sends `LED_OFF (grp=15)` to Bridge. When shutting off impulse, also sends `LED_OFF (grp=19)` to Bridge.
- EngRoom shutdown complete → sends `LED_SHUTDOWN` directly to Bridge → Bridge shuts down windows in reverse.

**DataPad OTA UI fix:**
- `Data_Pad.ino`: OTA `onStart` blanks backlight and acquires LVGL lock (pauses rendering task). `onEnd` and `onError` release lock and restore backlight. Eliminates screen flickering and improves upload speed.

**Sound loop fix:**
- `Data_Pad.ino`: `PowerUpAll` sends `SND_CMD_REPEAT, 0` to Bridge before triggering startup. Ensures DY-SV17F is never in loop mode on power-up regardless of last session's REPEAT button state.

**Shutdown sound transferred to EngRoom (connected mode):**
- `Engine_Room_ESP.ino`: `LED_SHUTDOWN` handler sends `SND_CMD_PLAY` (file 2 or 3, random) to Bridge via ESP-NOW immediately when shutdown starts.
- `Bridge_ESP.ino`: `LED_SHUTDOWN` handler only plays shutdown sound when `!espNowReceived` (standalone mode). In connected mode EngRoom triggers it.

**Nav/impulse sync confirmed working.** All three units need reflashing.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`, `Data_Pad/Data_Pad.ino`, `GPIO outputs EngRoom.txt`, `DEVNOTES.md`, `DESIGN_STORY.md`, `HARDWARE.md`.

---

### Session 5 — BridgeSound Screen Full Redesign

#### BridgeSound screen: complete retheme and layout rebuild

`ui_BridgeSound.c/.h` hand-rewritten from scratch. Screen now matches Screen1/Screen2 visual style throughout.

**LCARS frame:** identical decorative block layout to Screen1 — same sidebar (purple sections with black circle cutouts, orange bottom blocks) and same header bar (purple title block, peach dividers, blue right fill). Title reads "BRIDGE SOUND".

**Three new DataPad-only sound categories added** (`Data_Pad.ino`):
- `SoundVoiced()` — cycles files 28–31 (voiced alert lines). Calls `padSndPlay()` directly, never sent to Bridge.
- `SoundAmbient()` — cycles files 32–38 (ambient loop sounds). DataPad only.
- `SoundExtras()` — cycles files 39–44 (easter eggs inc. Star Wars). DataPad only.

Each has its own index variable (`sndVoicedIdx`, `sndAmbientIdx`, `sndEasterIdx`) and updates its button label with `NAME\nN / total` on each press. Orange = bridge-sendable categories; purple/lavender = DataPad-only.

**PLAY LAST button added** (`SoundPlayLast()` in `Data_Pad.ino`):
- Replays whatever file was most recently played.
- `lastSndFile` / `lastSndPadOnly` statics track the last play event.
- `playToDest()` sets these on every bridge-sendable play; DataPad-only handlers set them explicitly.
- Replays bridge-sendable sounds via `playToDest()` (respects dest mode); DataPad-only sounds via `padSndPlay()` directly. Does nothing if nothing has been played yet.

**Layout:** 5×5 grid of 100×45 pill buttons (25 total: 12 functional + 13 decorative). Column gap 15px, row gap 22px — much more even than previous 6px/22px. Vertical volume slider centered in the gap between LCARS sidebar and button grid (abs x=150, w=35, h=200), shorter than full grid height. VOL label below slider at montserrat_14. Functional and decorative buttons mixed throughout the grid (not grouped by row). Decorative buttons use `make_dec_lbl()` helper — same pill shape/styling as functional buttons, with TNG LCARS-style labels (SENSORS, TACTICAL, SHIELDS, COMMS, MEDICAL, SECURITY, NAV SYS, ENGINES, SCIENCE, SYSTEMS, STATUS, GRID, OPS).

**Files changed:** `Data_Pad/ui_BridgeSound.c`, `Data_Pad/ui_BridgeSound.h`, `Data_Pad/Data_Pad.ino`

**Flashed and confirmed working — all sound categories, PLAY LAST, and volume slider tested and functional.**

---

### Session 4 — Sound Volume Routing, Bridge Volume Slider, DataPad Physical Build Complete

#### DataPad: sound screen volume slider now routes per audio destination

`SoundVolume()` previously always sent `SND_CMD_VOL` to the Bridge regardless of the `padAudioDest` setting. Changed to route per destination:

- `padAudioDest = BRIDGE (0)` → sends `SND_CMD_VOL` to Bridge via ESP-NOW only
- `padAudioDest = PAD (1)` → calls `padSndSetVol()` on local DY1703A only
- `padAudioDest = BOTH (2)` → does both simultaneously

Also updates `bridgeSndVol` / `padSndVol` globals on change, and syncs the Screen2 sliders live if Screen2 happens to be open at the time.

#### DataPad: new Bridge volume slider on Screen2

Added `ui_BridgeVolSlider` to Screen2, placed side-by-side with the existing PAD VOL slider. Calls `BridgeVolume()` which always sends `SND_CMD_VOL` to Bridge directly, independent of `padAudioDest`. Both Screen2 sliders now init from their respective globals (`padSndVol`, `bridgeSndVol`) on screen load instead of hardcoded 15.

New global: `uint8_t bridgeSndVol = 15` in `Data_Pad.ino`.

#### DataPad: audio destination defaults to BOTH on boot

`padAudioDest` default changed from 0 (BRIDGE) to 2 (BOTH) — both players active on startup without manual toggle. `ui_PadDestBtn` label init in `ui_Screen2.c` updated to match ("BOTH").

**Files changed:** `Data_Pad/Data_Pad.ino`, `Data_Pad/ui_events.h`, `Data_Pad/ui_Screen2.c`, `Data_Pad/ui_Screen2.h`

**Tested and confirmed working.**

#### DataPad physical build — ready for final print

All last parts arrived. Electronics fully wired and test-fitted in the prototype case. Ready for final print.

Final assembly method decided:
- Screen mounting screws retained — these also hold the two case halves together
- All other internal components secured by melting the printed plastic down with a soldering iron to form rivet-style retention points — no screws elsewhere
- Chosen because this is the intended permanent install; the riveted method is cleaner and more secure than screws for components that will never need to come out again

---

### Session 3 — Sound System, Damage Effect, DataPad Screen Timeout, EngRoom PCB Test

#### Bridge: damage effect reworked

- Flicker effect: reduced from all windows to 2–3 randomly selected groups. Active groups do PWM flicker (one) and slow on/off fault cycling (rest). Non-active windows snap to random on/off state and hold.
- Flicker intervals slowed for less chaotic look. Tuning constants in `updateElecShort()`.
- Damage sound sequence: alert klaxon/beep plays immediately on damage start, cuts to random damage sound after `DMG_ALERT_MS` (2500ms). Handled by `dmgSndPhase` state in `initElecShort()` / `updateElecShort()`.
- Startup/shutdown sounds: startup always plays file 1 (tng_poweringup). Shutdown picks random from files 2–3.
- `stopElecShort()` consolidated: now handles all restore (windows, engines, nav, `currentEffect`, sound phase reset) in one place. All callers simplified.
- `allWindowsOff()`, `releaseZoneDim()`, `stopElecShort()`: added `pinMode(pin, OUTPUT)` after every `ledcDetach()` to fix LED restore failures in ESP32 Arduino 3.x.
- `updateShutdown()`: added `allOff()` at sequence end as hard safety guarantee.
- `LED_OFF grp=-1` path: no longer calls `allOff()` when damage isn't active (prevents all-LEDs-off when DataPad state is out of sync with Bridge).

#### Bridge: sound file range expanded

- DY-SV17F file map expanded from 4 files to 25. Files in `sounds/DY_Player/` — copy with `copy_to_player.bat` (or Windows Explorer sorted by name after formatting) to guarantee FAT write order.
- `SND_CMD_PLAY` constrain in `handleLedCommand` was clamped to `(val, 1, 4)` — changed to `(val, 1, 25)`. This was the root cause of all sound screen buttons playing file 4.
- Default volume changed from 20 to 15.

#### DataPad 7": screen timeout

- `lv_disp_get_inactive_time(NULL)` used to track idle time. After `SCREEN_TIMEOUT_MS` (5 min) of no touch, backlight off. Any touch wakes it. Defined in `Data_Pad.ino`.

#### DataPad 7": sound screen rebuilt

- BridgeSound screen expanded from 4 fixed-file buttons to 5 category buttons (POWER, ALERTS, DAMAGE, WARP, WEAPONS).
- Each button cycles through its file range on each press, label updates to show `CATEGORY\nN / total`.
- `ui_BridgeSound.c/.h` hand-rewritten with `make_snd_btn()` helper and 3+2 button layout.
- Sound constants added to `Data_Pad.ino` matching Bridge ranges.

#### EngRoom: PCB test blink sketch

- All 11 Xiao ESP32-C3 GPIO pins (D0–D10) blink together for first 10s, then walk one-at-a-time.
- Serial Monitor prints active pin during walk phase for pinout mapping.
- WiFi/OTA/ESP-NOW infrastructure unchanged and still running alongside test.

---

### Session 2 — Damage Control, Engine Effect, ESP-NOW

#### Bridge: Damage control window state not restored on cancel

**Problem:** When the electrical short (damage) effect ended — either via the 2-minute auto-cancel or the DataPad cancel button — the window LEDs were left in whatever random flickery state they were in during the effect instead of returning to their pre-damage state.

**Root causes (two separate bugs):**

1. `stopElecShort()` detached the LEDC channels from the two PWM-flicker windows but never wrote a final state to any window pin. Windows were left at whatever voltage the last flicker cycle left them.

2. The DataPad cancel button sends `LED_OFF grp=-1` to Bridge. The `LED_OFF` handler was calling `allOff()` after `stopElecShort()`, which turned everything dark instead of restoring the pre-damage running state. The auto-cancel path in `loop()` correctly restored state; the manual cancel path did not.

**Fixes:**
- Added `preDmgZoneActive` and `preDmgWindowOn[NUM_WINDOWS]` globals to save each window's `digitalRead` state before `initElecShort()` runs.
- `stopElecShort()` now restores all windows to their saved pre-damage state after detaching LEDC channels.
- `LED_OFF grp=-1` handler: when damage is active, performs the same restore-from-damage as auto-cancel (engines, blink, windows all returned to pre-damage state) instead of calling `allOff()`.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`

---

#### Bridge: NAV "constant on" kept flashing

**Problem:** DataPad navState=3 ("NAV ON") sends `LED_ON` cmd with `grp=IDX_NAV`. The Bridge `LED_ON` handler for `IDX_NAV` was setting `blinkActive = true`, which drove the blink timer and caused the NAV light to flash instead of staying on solid.

**Fix:** Changed `LED_ON` for `IDX_NAV` to `blinkActive = false; ledcWrite(PIN_NAV, 255)`.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`

---

#### Bridge: Engine effect changed from flicker to sine-wave pulse

**Change:** Replaced the random-interval `updateEngineFlicker()` function with `updateEnginePulse()`, which drives a smooth sine-wave brightness cycle between `minDuty=130` and `maxDuty=255` on a 2-second period. The engines are always visibly on; they never cut to zero.

**Tuning constants** (in `updateEnginePulse()`):
- `period` — ms per full cycle (2000 = 2 second breath)
- `minDuty` — minimum brightness 0–255 (130 = ~51%)
- `maxDuty` — maximum brightness 0–255 (255 = full)

Removed the now-unused `nextEngAMs` and `nextEngBMs` timer variables.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`

---

#### All units: ESP-NOW fails after WiFi connects

**Problem:** Calling `WiFi.begin()` — even on a failed attempt — scans channels 1–13 and leaves the radio on an arbitrary channel. DataPad and EngRoom default to ch=1 when not connected to WiFi. Bridge on a different channel after a failed connect → ESP-NOW FAIL on every subsequent send.

Additionally, `WiFi.begin()` resets the power-save mode back to the default (light sleep). In light-sleep mode the radio duty-cycles between beacon intervals and ESP-NOW ACKs from the remote device arrive during a sleep window → perpetual FAIL on the sender.

Peers registered at boot with `channel=0` can also fail to pick up the new channel in some IDF 5.x builds after the channel changes.

**Fixes applied to all three units:**
- `#include <esp_wifi.h>` added.
- `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)` called at boot right after `WiFi.mode(WIFI_STA)` — ensures ch=1 before the first send.
- `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)` called after any failed connect and in `RevertWiFi()` — restores ch=1 so ESP-NOW works again immediately.
- After **successful** WiFi connect: `esp_wifi_set_ps(WIFI_PS_NONE)` (disable power save) + delete and re-add all ESP-NOW peers — forces peers onto the new channel.
- `WiFi.mode(WIFI_STA)` removed from the failed-connect path (calling it when already in STA mode can reinitialise the driver and wipe the channel setting).
- WiFi connect retries: two attempts of 10 seconds each before declaring failure (all three units).

**DataPad auto-connect additional fix:** Before `WiFi.begin()` in the boot auto-connect block, DataPad now sends `startWifi=1` with credentials to Bridge and EngRoom via ESP-NOW (while still on ch=1). This triggers all three to connect simultaneously and land on the same AP channel.

**Files changed:** `Bridge_ESP/Bridge_ESP.ino`, `Data_Pad/Data_Pad/Data_Pad.ino`, `Engine_Room_ESP/Engine_Room_ESP.ino`

---

#### All units: Channel debug logging

Added `wifiChannel` global (cached in main task, safe to read from callbacks) and `Serial.printf` at every channel-change point. Serial Monitor will now show:

```
ESP-NOW ready  ch=1          ← boot, ch=1 confirmed
WiFi attempt 1/2...          ← connect in progress
Connected. IP: x.x.x.x  ch=6
ESP-NOW peers refreshed  ch=6
Send status: OK  ch=6        ← send succeeded on ch=6
Send status: FAIL  ch=1      ← mismatch: this unit on ch=1, peer on different ch
WiFi: did not connect after 2 attempts. ESP-NOW ch=1  ← restored to ch=1
WiFi reverted. ESP-NOW ch=1  ← revert successful
```

`ch=0` in old output was a known ESP-IDF limitation — `esp_wifi_get_channel()` returns 0 when called from inside the ESP-NOW send callback (WiFi task context). Fixed by caching the value in a global from the main task instead.

**Files changed:** All three sketches.

---

#### NVS clear utility sketches

Two standalone sketches that clear only the WiFi credentials from NVS while leaving timing settings intact:
- `Bridge_NVS_Clear/Bridge_NVS_Clear.ino`
- `EngRoom_NVS_Clear/EngRoom_NVS_Clear.ino`

Flash, confirm in Serial Monitor, reflash main sketch. Takes under 5 seconds.

---

## Assembly Notes

### Bridge PCB pin label assignments are mirrored and flipped

The reference drawing used during Bridge PCB design was sourced from a website with mirrored and flipped pin labels. The label assignments on the board came from that drawing and do not match the original kit documentation. **The physical board layout is correct** — mounting holes, connectors, and board fit are all fine.

Current boards work as-is: plug each LED connector to its matching label on the board and everything works. Two items flagged for the next board revision: (1) mounting holes need to shift to the opposite side — discovered during neck assembly, board had to move to clear the power switch and holes no longer line up; (2) pin label assignments need correcting if exact kit documentation compatibility is wanted. See HARDWARE.md for details.

**Confirmed during neck assembly (2026-05-19):** pin order issue encountered as expected. Resolved by swapping LEDs/connectors into the physically correct positions rather than following kit documentation pin order. Neck assembly completed.

---

### LED panels lighting in wrong groups

During reassembly for testing, some LED panels lit up in the wrong group — the LED group assignments were reversed on the original PCB designs. This has been corrected on the PCB files.

**Tip: test LEDs frequently during reassembly, not just at the end.** Power up after each section is connected so a wiring mistake is caught early rather than after everything is buttoned up.

**If a group lights up in the wrong panel:** the easy fix is to swap the LEDs into the physically correct panel rather than trying to re-pin connectors or follow the original kit instructions for which LED goes in which socket. The original instructions assume the original PCBs; our PCBs have their own grouping logic.

### EngRoom nav / photon LED swap — resolved

**Unit:** EngRoom — neck board connector

**Problem:** The nav and photon LED wires on the neck board connector were soldered in reverse order. Caused GPIO 4 (PIN_NAV) to light the photon torpedo LED and GPIO 5 (PIN_PHOTON) to light the nav light LED. Discovered via assembly mode walk (2026-05-20).

**Resolved 2026-05-20:** Neck board connector re-pinned — wires now in correct order. Firmware was already correct (`PIN_NAV=4`, `PIN_PHOTON=5`), no sketch change needed. Reflash EngRoom to confirm.

**Final fix (PCB revision):** Reverse the connector footprint on the neck board PCB so it is correct by design. No re-pinning needed after that.

---

### Battle Bridge PCB — Pull-down resistors removed from all boards

**Unit:** EngRoom (GPIO 8 = Right battle bridge, GPIO 9 = Left battle bridge)

**Issue:** Left battle bridge — fitting the base pull-down resistor caused all LEDs on that board to stay permanently on. Swapping all SMD components made no difference. Multimeter showed a direct short to GND on the pull-down pad where the schematic showed no GND connection — confirmed EasyEDA copper pour clearance fault; the pour was connecting the pull-down pad to the wrong net on the physical board.

**Resolution:** Pull-down resistors removed from all battle bridge boards. All boards operate correctly without them. Decision made to omit base pull-down resistors from all future board designs — ESP32 OUTPUT mode actively holds pins LOW, making external pull-downs redundant. See HARDWARE.md Battle Bridge PCB Notes.

---

## Upcoming — GitHub Release & Project Website

- **GitHub repo prep** — IN PROGRESS (session 24). Done: `.gitignore`, sound attribution in `SOUND_MAP.md`, Bridge serial debug removed, sketch dead-code cleanup, obsolete files deleted, `amazon parts.txt` (full parts list with clean links). Remaining: write root `README.md` and sketch-level READMEs (`Bridge_ESP/`, `Engine_Room_ESP/`, `Data_Pad/`). WarpCore excluded from this repo — will link to a separate repo.
- **Project website** — design story, build photos, and videos. Content already written in `DESIGN_STORY.md`. Consider a simple static site (GitHub Pages or similar) — LCARS aesthetic would suit the theme. Photos and videos taken throughout the build ready to use.

---

## Pending Work (in order)

1. ~~**EngRoom in-model test**~~ — **DONE 2026-05-20.** Stardrive section installed, full assembly complete, all effects working. Open hardware issues: fix queue #3 (nav backfeed) and #5 (nav dimming) — both need second resistor on neck PCB.
2. ~~**EngRoom nav/photon connector re-pin**~~ — **DONE 2026-05-20.** Neck board connector re-pinned. Firmware was already correct (`PIN_NAV=4`, `PIN_PHOTON=5`) — no sketch change needed. Reflash pending.
3. ~~**EngRoom nav+photon resistor fix**~~ — **DONE** (PCB revision 2026-05-21). Separate resistors on neck PCB: 1.2kΩ on white nav, 2.2kΩ on red photon. Eliminates backfeed and nav dimming.
4. ~~**BridgeSound screen extra buttons**~~ — **DONE session 5.** VOICED, AMBIENT, EXTRAS added. PLAY LAST added. Full LCARS retheme with 5×5 mixed grid.
5. ~~**Screen 2 brightness/timing sliders**~~ — **CANCELLED.** Too complex for the benefit; Windows toggle added instead. Next free `ledCmd` constant remains **31**.
6. **DataPad battery monitor** — SLS export done (`ui_Slider1`, `ui_Label25`–27 on Screen 2). Code not yet added to `Data_Pad.ino`. Board revision required for voltage divider on ADC1 (GPIO 1/2/4).
7. ~~**LCARS UI theme**~~ — **DONE.** Screen1, Screen2, and BridgeSound all themed as of session 5.
8. ~~**Struct cleanup**~~ — **DONE session 16 (2026-05-24).** Removed legacy `a` and `r` fields from `struct_message`. All 4 sketches updated and reflashed.
9. ~~**Startup sequence coordination**~~ — **DONE session 6.** Full coordinated startup/shutdown. EngRoom is master for nav/impulse sync — sends `LED_BLINK`/`LED_ENGINE` to Bridge at its nav/impulse steps. Bridge skips those steps in connected mode (`espNowReceived`). Shutdown runs EngRoom-first; EngRoom sends LED_OFF to Bridge nav/impulse then relays `LED_SHUTDOWN` when complete. Full peer mesh: EngRoom now has Bridge as peer. **Reflash all 3 units.**
10. ~~**Assembled/Separated sync mode**~~ — **DONE 2026-05-22.** `LED_SYNC_MODE=25` implemented. DataPad `PowerUpAll()` routes based on online status and mode. Screen 2 ASSEMBLED/SEPARATED toggle + Bridge/EngRoom status indicator buttons. Sound fallback on Bridge offline. All 3 units reflashed and verified.
11. **LCARS web UI** *(after LED effects complete)* — replace basic HTML on Bridge with LittleFS-served LCARS page. `/led` endpoint stays as-is; HTML/CSS/JS in `data/index.html`.
12. **WiFi / OTA via DataPad SoftAP** *(replaces old multi-AP workaround)* — DataPad creates its own WiFi AP (`WiFi.mode(WIFI_AP_STA)`, `WiFi.softAP("enterprise", password, 1)` — channel 1). Bridge and EngRoom receive credentials via ESP-NOW and connect as stations. All three stay on ch=1, ESP-NOW continues to work alongside WiFi. No external hotspot needed, no channel drift possible. Save WiFi button flow changes: first press starts the SoftAP and pushes credentials to Bridge/EngRoom; second press stops AP and reverts all to ch=1 ESP-NOW. Auto-revert (90s) also stops the AP. Screen 3 instruction label updated to show "Connect to 'enterprise' WiFi then open http://Bridge.local / http://EngRoom.local". SSID and password taken from the Screen 3 text fields (defaulting to "enterprise"/"ncc-1701-d"). Significant change to WiFi flow — plan carefully before implementing.
13. ~~**Power-up status indicator**~~ — **DONE 2026-05-22.** Bridge: nav + G14 + G15 blink 500ms for 20s. EngRoom: nav + nacelles blink 500ms for 20s. Cancelled immediately by `LED_STARTUP`. Flashing chosen over solid so boot state is distinguishable from a frozen startup.
14. ~~**Full LED count + power budget**~~ — **DONE 2026-05-21.** EngRoom: 50 LEDs (~133mA total at nominal). Bridge: 170 LEDs (~353mA total at nominal). Runtime: 350mAh cells → EngRoom ~2.6hrs, Bridge ~2.0hrs. 1000mAh cells → EngRoom ~7.5hrs, Bridge ~5.7hrs. See HARDWARE.md for full breakdown.
15. **Bridge PCB revision** — shift mounting holes to opposite side (clear power switch); optionally correct mirrored pin label assignments.
17. **Home Assistant / Alexa integration** *(long-term goal, low priority)* — remote voice/automation control for basic on/off and possibly effect triggers. Current WiFi setup (OTA via Save WiFi, ESP-NOW for normal operation) stays unchanged. Integration would require Bridge to connect to the home network when available — keeping existing WiFi flow for OTA but adding a persistent connection mode for smart home. Bridge already has an HTTP `/led` endpoint which is a natural integration point. Alexa could use a custom skill or Emulated Hue; Home Assistant via MQTT or HTTP. SoftAP mode (#12) is not compatible with this since it isolates from the home network — would need a separate "smart home mode" where Bridge connects to home WiFi and stays connected. Consider this when planning any further WiFi architecture changes.
~~16.~~ ~~**Red alert screen**~~ — **DONE 2026-05-24.** `ui_RedAlert.c/.h` — full-screen black background, 247×204 image converted to RGB565 C array (`red_alert_img.h`, 100KB), displayed at 2× zoom centred on 800×480. `tng_red_alert1` (file 28) plays on trigger and repeats every 16s. Auto-dismisses after 60s or on any tap. Triggered by RED ALERT button on Screen 1 or by radar sensor when model is already on (`redAlertSensorEnabled`). `LV_USE_PNG` left at 0 — `LV_IMG_CF_TRUE_COLOR` with pre-converted pixel data used instead.
18. ~~**WarpCore MAC registration**~~ — **DONE 2026-05-24.** MAC: `20:6e:f1:31:da:54`. Registered in DataPad as `broadcastAddressWarpCore[]`, peer added in setup.
19. **WarpCore LD2410C wiring** — IN PROGRESS 2026-05-24. OUT pin wired to D7 on Arduino Nano ESP32. Power: 3.3V boost to 5V (same boost converter module as DataPad DY1703A). Firmware handles rising-edge detection with 3s debounce — sends `LED_RADAR_TRIG` to DataPad when presence detected. DataPad powers up model if off; fires red alert if model already on and `redAlertSensorEnabled`. UART not yet wired — sensor housing build will run UART wires for gate/sensitivity config via HLKRadarTool.
    - **Calibration wiring plan (2026-06-09):** A dedicated calibration cable with JST connector will be made for the spare 5V Arduino Nano passthrough. LD2410C UART pins will be brought out to a JST connector on the WarpCore — disconnect for normal use, plug in the Nano cable for calibration. The LD2410C is 3.3V logic: Nano TX → LD2410C RX needs a voltage divider (1kΩ + 2kΩ to GND) built permanently into the calibration cable. LD2410C TX → Nano RX is straight through (3.3V into 5V Nano is fine). Use HLKRadarTool at 256000 baud on the Nano's COM port to configure detection gates and sensitivity.
~~20.~~ ~~**Button layout pass**~~ — **DONE session 19 (2026-05-24).** All Screen 1 functional buttons repositioned to reference SLS coordinates. Deflector and Impulse moved. Impulse color changed from dark red to nav lights purple. Deflector label corrected to "DEFLECTOR".

---

## OTA Password

**Password: `admin`** — entered in Arduino IDE at upload time. No `ArduinoOTA.setPassword()` call in any sketch; the library accepts whatever the IDE sends.

---

## ESP-NOW / WiFi Quick Reference

| Scenario | Expected ch= | ESP-NOW |
|---|---|---|
| All units, no WiFi | ch=1 all | ✓ Works |
| All units, same single AP | ch=N all (same) | ✓ Works |
| Mixed: some WiFi, some not | Different ch= | ✗ Fails |
| Multi-AP mesh, same SSID | Different ch= | ✗ Fails |

**Rule:** If any unit is on a different channel than the others, ESP-NOW between them fails. Always confirm all units show the same `ch=` value in Serial Monitor when troubleshooting.
