#include "ui.h"
#include "yellow_alert_img.h"

lv_obj_t * ui_YellowAlert = NULL;

static const lv_img_dsc_t yellow_alert_dsc = {
  .header = {
    .cf          = LV_IMG_CF_TRUE_COLOR,
    .always_zero = 0,
    .reserved    = 0,
    .w           = YELLOW_ALERT_IMG_W,
    .h           = YELLOW_ALERT_IMG_H,
  },
  .data_size = sizeof(yellow_alert_img_data),
  .data      = yellow_alert_img_data,
};

static void ui_event_YellowAlertDismiss(lv_event_t * e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) YellowAlertDismiss(e);
}

void ui_YellowAlert_screen_init(void)
{
  ui_YellowAlert = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_YellowAlert, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_YellowAlert, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(ui_YellowAlert, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_add_event_cb(ui_YellowAlert, ui_event_YellowAlertDismiss, LV_EVENT_ALL, NULL);

  lv_obj_t * img = lv_img_create(ui_YellowAlert);
  lv_img_set_src(img, &yellow_alert_dsc);
  lv_img_set_zoom(img, 512);
  lv_img_set_pivot(img, YELLOW_ALERT_IMG_W / 2, YELLOW_ALERT_IMG_H / 2);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
}

void ui_YellowAlert_screen_destroy(void)
{
  if (ui_YellowAlert) lv_obj_del(ui_YellowAlert);
  ui_YellowAlert = NULL;
}
