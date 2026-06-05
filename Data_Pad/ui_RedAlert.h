#ifndef UI_REDALERT_H
#define UI_REDALERT_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_RedAlert_screen_init(void);
extern void ui_RedAlert_screen_destroy(void);
extern lv_obj_t * ui_RedAlert;
extern void ui_event_RedAlertDismiss(lv_event_t * e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
