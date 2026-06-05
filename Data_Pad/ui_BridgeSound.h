// DataPad — BridgeSound screen header (hand-written, do not replace with SLS export)

#ifndef UI_BRIDGESOUND_H
#define UI_BRIDGESOUND_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_BridgeSound_screen_init(void);
extern void ui_BridgeSound_screen_destroy(void);
extern lv_obj_t * ui_BridgeSound;
extern lv_obj_t * ui_SndBackBtn;
extern lv_obj_t * ui_SndBackLabel;
extern lv_obj_t * ui_SndTitle;
extern void ui_event_SndBackBtn(lv_event_t * e);
extern void ui_event_SndPlay1Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay1Btn;
extern lv_obj_t * ui_SndPlay1Label;
extern void ui_event_SndPlay2Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay2Btn;
extern lv_obj_t * ui_SndPlay2Label;
extern void ui_event_SndPlay3Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay3Btn;
extern lv_obj_t * ui_SndPlay3Label;
extern void ui_event_SndPlay4Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay4Btn;
extern lv_obj_t * ui_SndPlay4Label;
extern void ui_event_SndPlay5Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay5Btn;
extern lv_obj_t * ui_SndPlay5Label;
extern void ui_event_SndPlay6Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay6Btn;
extern lv_obj_t * ui_SndPlay6Label;
extern void ui_event_SndPlay7Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay7Btn;
extern lv_obj_t * ui_SndPlay7Label;
extern void ui_event_SndPlay8Btn(lv_event_t * e);
extern lv_obj_t * ui_SndPlay8Btn;
extern lv_obj_t * ui_SndPlay8Label;
extern void ui_event_SndStopBtn(lv_event_t * e);
extern lv_obj_t * ui_SndStopBtn;
extern lv_obj_t * ui_SndStopLabel;
extern lv_obj_t * ui_SndVolLabel;
extern void ui_event_SndVolSlider(lv_event_t * e);
extern lv_obj_t * ui_SndVolSlider;
extern void ui_event_SndRepeatBtn(lv_event_t * e);
extern lv_obj_t * ui_SndRepeatBtn;
extern lv_obj_t * ui_SndRepeatLabel;
extern void ui_event_SndPlayLastBtn(lv_event_t * e);
extern lv_obj_t * ui_SndPlayLastBtn;
extern lv_obj_t * ui_SndPlayLastLabel;

// Sound handlers defined in Data_Pad.ino
void SoundPower(lv_event_t * e);
void SoundAlerts(lv_event_t * e);
void SoundDamage(lv_event_t * e);
void SoundWarp(lv_event_t * e);
void SoundWeapons(lv_event_t * e);
void SoundVoiced(lv_event_t * e);
void SoundAmbient(lv_event_t * e);
void SoundExtras(lv_event_t * e);
void SoundPlayLast(lv_event_t * e);
void SoundStop(lv_event_t * e);
void SoundVolume(lv_event_t * e);
void SoundRepeat(lv_event_t * e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
