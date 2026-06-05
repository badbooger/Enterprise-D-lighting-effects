# Sound Screen — SquareLine Studio Integration Guide

## What Was Generated
The following files are ready to use in the project:
- `Data_Pad/Data_Pad/ui_BridgeSound.c` — full sound screen implementation
- `Data_Pad/Data_Pad/ui_BridgeSound.h` — declarations
- `Data_Pad/Data_Pad/ui.h` — already updated to include BridgeSound
- `Data_Pad/Data_Pad/ui.c` — already updated to init/destroy BridgeSound

---

## What You Need to Do in SquareLine Studio

### Step 1 — Add the Sound Screen
1. In SLS, click **Add Screen**
2. Name it exactly: `BridgeSound`
   - SLS will prefix it with `ui_` → the screen object becomes `ui_BridgeSound`
   - This must match exactly or the init calls in `ui.c` won't link
3. Leave the screen **completely blank** — no objects, no events
   - All content is already built in `ui_BridgeSound.c`

### Step 2 — Add a Navigation Button on Screen1
1. On Screen1, add a button wherever you want the "Sound" nav button
2. Name it whatever you like (e.g. `SndNavBtn`) — it won't conflict
3. Set the button's action:
   - **Event:** Clicked
   - **Action:** Change Screen
   - **Screen:** BridgeSound
   - **Animation:** Fade On
   - **Speed:** 500ms, Delay: 0ms
4. SLS will auto-generate the event handler for this button in `ui_Screen1.c`

### Step 3 — Export from SLS
Export / Create Template Project as normal.

### Step 4 — Handle the Exported Files

**Replace these with the SLS-exported versions (SLS owns these):**
- `ui.h` — will now include `ui_BridgeSound.h`
- `ui.c` — will now call `ui_BridgeSound_screen_init/destroy`
- `ui_Screen1.c` — will have the new nav button event handler

**Keep these files — do NOT replace with SLS output:**
- `ui_BridgeSound.c` ← SLS generates an empty stub; keep the hand-written version
- `ui_BridgeSound.h` ← same — keep the hand-written version

### Step 5 — Re-comment ui_events.c Stubs
As usual after any SLS export, re-comment the stubs in `ui_events.c`:
```c
//void PowerUpAll(lv_event_t * e)    { }
//void NavLightsSet(lv_event_t * e)  { }
//void ImpulsEng(lv_event_t * e)     { }
//void DamageControl(lv_event_t * e) { }
//void StartWiFi(lv_event_t * e)     { }
//void SaveWifi(lv_event_t * e)      { }
```
The sound handlers (SoundPlay1-4, SoundStop, SoundVolume, SoundRepeat) are
forward-declared in `ui_BridgeSound.h` — they do NOT appear in `ui_events.h`
and do NOT need stubs in `ui_events.c`.

---

## Screen Layout Reference (ui_BridgeSound)
Screen size: 800 × 480, coordinates relative to center

| Object             | Type    | Position (x, y) | Size (w × h) | Color   | Notes                   |
|--------------------|---------|-----------------|--------------|---------|-------------------------|
| ui_SndBackBtn      | Button  | -315, -195      | 100 × 40     | Green   | Navigates back to Screen1 |
| ui_SndBackLabel    | Label   | child of above  | —            | —       | Text: "< BACK"          |
| ui_SndTitle        | Label   | 45, -195        | content      | White   | Text: "SOUND CONTROLS"  |
| ui_SndPlay1Btn     | Button  | -125, -100      | 230 × 70     | Green   | Plays SND_FILE_1        |
| ui_SndPlay1Label   | Label   | child of above  | —            | —       | Text: "Sound 1" (update when named) |
| ui_SndPlay2Btn     | Button  | 125, -100       | 230 × 70     | Green   | Plays SND_FILE_2        |
| ui_SndPlay2Label   | Label   | child of above  | —            | —       | Text: "Sound 2"         |
| ui_SndPlay3Btn     | Button  | -125, -20       | 230 × 70     | Green   | Plays SND_FILE_3        |
| ui_SndPlay3Label   | Label   | child of above  | —            | —       | Text: "Sound 3"         |
| ui_SndPlay4Btn     | Button  | 125, -20        | 230 × 70     | Green   | Plays SND_FILE_4        |
| ui_SndPlay4Label   | Label   | child of above  | —            | —       | Text: "Sound 4"         |
| ui_SndStopBtn      | Button  | 0, 65           | 380 × 55     | Red     | Stops playback          |
| ui_SndStopLabel    | Label   | child of above  | —            | —       | Text: "STOP"            |
| ui_SndVolLabel     | Label   | -185, 140       | content      | White   | Text: "Volume"          |
| ui_SndVolSlider    | Slider  | 60, 140         | 310 × 20     | —       | Range 0–30, default 20  |
| ui_SndRepeatBtn    | Button  | 0, 193          | 210 × 42     | Blue    | Cycles: OFF → LOOP ONE → LOOP ALL |
| ui_SndRepeatLabel  | Label   | child of above  | —            | —       | Text updates on each press |

---

## Naming Note
All object names use the `ui_Snd` prefix so they are clearly grouped and will
never conflict with default SLS auto-numbered names (`ui_Button51` etc.).
Adding more buttons or labels to any other screen in SLS will not collide.
