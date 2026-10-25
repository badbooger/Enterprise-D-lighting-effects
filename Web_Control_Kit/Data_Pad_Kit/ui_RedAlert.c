#include "ui.h"
#include "red_alert_img.h"

lv_obj_t * ui_RedAlert = NULL;

static const lv_img_dsc_t red_alert_dsc = {
  .header = {
    .cf          = LV_IMG_CF_TRUE_COLOR,
    .always_zero = 0,
    .reserved    = 0,
    .w           = RED_ALERT_IMG_W,
    .h           = RED_ALERT_IMG_H,
  },
  .data_size = sizeof(red_alert_img_data),
  .data      = red_alert_img_data,
};

void ui_event_RedAlertDismiss(lv_event_t * e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) RedAlertDismiss(e);
}

void ui_RedAlert_screen_init(void)
{
  ui_RedAlert = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_RedAlert, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_RedAlert, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_RedAlert, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_RedAlert, ui_event_RedAlertDismiss, LV_EVENT_ALL, NULL);

  lv_obj_t * img = lv_img_create(ui_RedAlert);
  lv_img_set_src(img, &red_alert_dsc);
  lv_img_set_zoom(img, 512);
  lv_img_set_pivot(img, RED_ALERT_IMG_W / 2, RED_ALERT_IMG_H / 2);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
}

void ui_RedAlert_screen_destroy(void)
{
  if (ui_RedAlert) lv_obj_del(ui_RedAlert);
  ui_RedAlert = NULL;
}
