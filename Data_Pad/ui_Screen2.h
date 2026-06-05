// Hand-edited. Do not replace with SquareLine Studio export output.

#ifndef UI_SCREEN2_H
#define UI_SCREEN2_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_Screen2
extern void ui_Screen2_screen_init(void);
extern void ui_Screen2_screen_destroy(void);
extern lv_obj_t * ui_Screen2;

// Navigation
extern void ui_event_Button18(lv_event_t * e);
extern lv_obj_t * ui_Button18;
extern void ui_event_Button1(lv_event_t * e);
extern lv_obj_t * ui_Button1;

// Assembly mode
extern void ui_event_Button52(lv_event_t * e);
extern lv_obj_t * ui_Button52;
extern void ui_event_Button53(lv_event_t * e);
extern lv_obj_t * ui_Button53;

// Status indicators (display-only)
extern lv_obj_t * ui_BridgeStatusBtn;
extern lv_obj_t * ui_EngRoomStatusBtn;

// Battery percentage labels (header zone)
extern lv_obj_t * ui_BridgeBatLabel;
extern lv_obj_t * ui_EngRoomBatLabel;

// Toggles
extern void ui_event_SyncModeBtn(lv_event_t * e);
extern lv_obj_t * ui_SyncModeBtn;
extern void ui_event_AllOffBtn(lv_event_t * e);
extern lv_obj_t * ui_AllOffBtn;
extern void ui_event_AllOnBtn(lv_event_t * e);
extern lv_obj_t * ui_AllOnBtn;
extern void ui_event_TouchSndBtn(lv_event_t * e);
extern lv_obj_t * ui_TouchSndBtn;
extern void ui_event_AmbientBtn(lv_event_t * e);
extern lv_obj_t * ui_AmbientBtn;
extern void ui_event_WarpCoreBtn(lv_event_t * e);
extern lv_obj_t * ui_WarpCoreBtn;
extern void ui_event_RadarBtn(lv_event_t * e);
extern lv_obj_t * ui_RadarBtn;
extern void ui_event_WinBtn(lv_event_t * e);
extern lv_obj_t * ui_WinBtn;
extern void ui_event_RadarAlertBtn(lv_event_t * e);
extern lv_obj_t * ui_RadarAlertBtn;

// Audio
extern lv_obj_t * ui_PadDestBtn;
extern lv_obj_t * ui_PadVolSlider;
extern lv_obj_t * ui_BridgeVolSlider;
extern void ui_event_BridgeVolSlider(lv_event_t * e);
extern void ui_event_PadDestBtn(lv_event_t * e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
