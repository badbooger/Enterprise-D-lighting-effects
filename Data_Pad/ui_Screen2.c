// Screen 2 — SYSTEM SETUP
// Hand-edited. Do not replace with SquareLine Studio export output.

#include "ui.h"

// ── Globals ───────────────────────────────────────────────────────────────────

lv_obj_t * ui_Screen2          = NULL;
lv_obj_t * ui_Button18         = NULL;   // Main Menu
lv_obj_t * ui_Button1          = NULL;   // WIFI
lv_obj_t * ui_Button52         = NULL;   // Bridge ASM
lv_obj_t * ui_Button53         = NULL;   // EngRoom ASM
lv_obj_t * ui_BridgeStatusBtn  = NULL;
lv_obj_t * ui_EngRoomStatusBtn = NULL;
lv_obj_t * ui_SyncModeBtn      = NULL;
lv_obj_t * ui_AllOffBtn        = NULL;
lv_obj_t * ui_AllOnBtn         = NULL;
lv_obj_t * ui_TouchSndBtn      = NULL;
lv_obj_t * ui_AmbientBtn       = NULL;
lv_obj_t * ui_PadDestBtn       = NULL;
lv_obj_t * ui_PadVolSlider     = NULL;
lv_obj_t * ui_BridgeVolSlider  = NULL;
lv_obj_t * ui_WarpCoreBtn      = NULL;
lv_obj_t * ui_RadarBtn         = NULL;
lv_obj_t * ui_WinBtn           = NULL;
lv_obj_t * ui_RadarAlertBtn    = NULL;
lv_obj_t * ui_BridgeBatLabel   = NULL;
lv_obj_t * ui_EngRoomBatLabel  = NULL;

// ── Event stubs ───────────────────────────────────────────────────────────────

void ui_event_PadVolSlider(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) PadVolume(e);
}

void ui_event_BridgeVolSlider(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) BridgeVolume(e);
}

void ui_event_Button52(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); BridgeAssemblyMode(e); }
}

void ui_event_Button53(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); EngRoomAssemblyMode(e); }
}

void ui_event_SyncModeBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); SyncModeToggle(e); }
}

void ui_event_TouchSndBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) TouchSoundToggle(e);
}

void ui_event_AmbientBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) AmbientToggle(e);
}

void ui_event_WarpCoreBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); WarpCoreToggle(e); }
}

void ui_event_RadarBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); RadarToggle(e); }
}

void ui_event_WinBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); WindowsToggle(e); }
}

void ui_event_RadarAlertBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); RadarAlertToggle(e); }
}

void ui_event_AllOffBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); AllOff(e); }
}

void ui_event_AllOnBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); AllOn(e); }
}

void ui_event_Button18(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        play_touch_sound();
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
    }
}

void ui_event_Button1(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        play_touch_sound();
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen3_screen_init);
    }
}

void ui_event_PadDestBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { play_touch_sound(); PadAudioDest(e); }
}

// ── Local helpers ─────────────────────────────────────────────────────────────

static lv_obj_t * make_dec(lv_obj_t * parent, int cx, int cy, int w, int h,
                            uint32_t bg_hex, int radius)
{
    lv_obj_t * obj = lv_btn_create(parent);
    lv_obj_set_x(obj, cx); lv_obj_set_y(obj, cy);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_align(obj, LV_ALIGN_CENTER);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK |
                      LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                      LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

static lv_obj_t * make_s2_btn(lv_obj_t * parent, int cx, int cy, int w, int h,
                               uint32_t bg_hex, lv_obj_t ** out, const char * text)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_x(btn, cx); lv_obj_set_y(btn, cy);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 700, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(lbl, LV_ALIGN_CENTER);
    lv_label_set_text(lbl, text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(lbl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (out) *out = btn;
    return lbl;
}

static lv_obj_t * make_status_btn(lv_obj_t * parent, int cx, int cy, int w, int h,
                                   lv_obj_t ** out, const char * text)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_x(btn, cx); lv_obj_set_y(btn, cy);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK |
                      LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(btn, 700, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333355), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_obj_set_align(lbl, LV_ALIGN_CENTER);
    lv_label_set_text(lbl, text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(lbl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (out) *out = btn;
    return lbl;
}

// ── Screen init ───────────────────────────────────────────────────────────────

void ui_Screen2_screen_init(void)
{
    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── LCARS sidebar blocks ──────────────────────────────────────────────────
    make_dec(ui_Screen2, -354, -203,  59,  61, 0xFF9C00,  0);
    make_dec(ui_Screen2, -354, -154,  59,  31, 0xFFCC99,  0);
    make_dec(ui_Screen2, -354, -135,  59,  31, 0xFFCC99, 70);
    make_dec(ui_Screen2, -354, -100,  59,  31, 0xFFCC99, 60);
    make_dec(ui_Screen2, -354,  -81,  59,  31, 0xFFCC99,  0);
    make_dec(ui_Screen2, -354,  -40,  59,  42, 0xCC99CC,  0);
    make_dec(ui_Screen2, -354,   60,  59, 151, 0xFF9C00,  0);
    make_dec(ui_Screen2, -354,  183,  59,  86, 0xFF9C00,  0);

    // ── LCARS header thin strips ──────────────────────────────────────────────
    make_dec(ui_Screen2, -319, -125,  76, 11, 0xFFCC99, 0);
    make_dec(ui_Screen2, -310, -110,  57, 11, 0xFFCC99, 0);
    make_dec(ui_Screen2, -244, -125,  70, 11, 0xCC99CC, 0);
    make_dec(ui_Screen2, -244, -110,  70, 11, 0xCC99CC, 0);
    make_dec(ui_Screen2,  -83, -125, 242, 11, 0xFFCC99, 0);
    make_dec(ui_Screen2,  -83, -110, 242, 11, 0xFFCC99, 0);
    make_dec(ui_Screen2,   79, -125,  79, 11, 0xCC99CC, 0);
    make_dec(ui_Screen2,   79, -110,  79, 11, 0xCC99CC, 0);
    make_dec(ui_Screen2,  199, -125, 157, 11, 0xFF9C00, 0);
    make_dec(ui_Screen2,  199, -110, 157, 11, 0xFF9C00, 0);
    make_dec(ui_Screen2,  338, -125, 112, 11, 0xCC99CC, 0);
    make_dec(ui_Screen2,  338, -110, 112, 11, 0xCC99CC, 0);

    // ── Title label ───────────────────────────────────────────────────────────
    lv_obj_t * title = lv_label_create(ui_Screen2);
    lv_obj_set_x(title, 202); lv_obj_set_y(title, -183);
    lv_obj_set_align(title, LV_ALIGN_CENTER);
    lv_label_set_text(title, "SYSTEM SETUP");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Battery percentage labels — header zone ───────────────────────────────
    ui_BridgeBatLabel = lv_label_create(ui_Screen2);
    lv_obj_set_x(ui_BridgeBatLabel, -148); lv_obj_set_y(ui_BridgeBatLabel, -183);
    lv_obj_set_align(ui_BridgeBatLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_BridgeBatLabel, "BRIDGE --%");
    lv_obj_set_style_text_color(ui_BridgeBatLabel, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_BridgeBatLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BridgeBatLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_EngRoomBatLabel = lv_label_create(ui_Screen2);
    lv_obj_set_x(ui_EngRoomBatLabel, -28); lv_obj_set_y(ui_EngRoomBatLabel, -183);
    lv_obj_set_align(ui_EngRoomBatLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_EngRoomBatLabel, "ENGROOM --%");
    lv_obj_set_style_text_color(ui_EngRoomBatLabel, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_EngRoomBatLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EngRoomBatLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Volume sliders — vertical (rotated 90° CW), orange, left zone ────────
    // 90° CW rotation: min (0) at bottom, max (30) at top — natural fader direction.

    lv_obj_t * bridgeVolLabel = lv_label_create(ui_Screen2);
    lv_obj_set_x(bridgeVolLabel, -238); lv_obj_set_y(bridgeVolLabel, -30);
    lv_obj_set_align(bridgeVolLabel, LV_ALIGN_CENTER);
    lv_label_set_text(bridgeVolLabel, "BRIDGE\nVOL");
    lv_obj_set_style_text_color(bridgeVolLabel, lv_color_hex(0xFF9C00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bridgeVolLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bridgeVolLabel, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bridgeVolLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BridgeVolSlider = lv_slider_create(ui_Screen2);
    lv_slider_set_range(ui_BridgeVolSlider, 0, 30);
    lv_slider_set_value(ui_BridgeVolSlider, bridgeSndVol, LV_ANIM_OFF);
    lv_obj_set_width(ui_BridgeVolSlider, 25);
    lv_obj_set_height(ui_BridgeVolSlider, 150);
    lv_obj_set_x(ui_BridgeVolSlider, -238); lv_obj_set_y(ui_BridgeVolSlider, 68);
    lv_obj_set_align(ui_BridgeVolSlider, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_BridgeVolSlider, lv_color_hex(0x333333), LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BridgeVolSlider,  255,                      LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BridgeVolSlider, lv_color_hex(0xFF9C00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BridgeVolSlider,  255,                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BridgeVolSlider, lv_color_hex(0xFF9C00), LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BridgeVolSlider,  255,                      LV_PART_KNOB      | LV_STATE_DEFAULT);

    lv_obj_t * padVolLabel = lv_label_create(ui_Screen2);
    lv_obj_set_x(padVolLabel, -288); lv_obj_set_y(padVolLabel, -30);
    lv_obj_set_align(padVolLabel, LV_ALIGN_CENTER);
    lv_label_set_text(padVolLabel, "PAD\nVOL");
    lv_obj_set_style_text_color(padVolLabel, lv_color_hex(0xFF9C00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(padVolLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(padVolLabel, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(padVolLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PadVolSlider = lv_slider_create(ui_Screen2);
    lv_slider_set_range(ui_PadVolSlider, 0, 30);
    lv_slider_set_value(ui_PadVolSlider, padSndVol, LV_ANIM_OFF);
    lv_obj_set_width(ui_PadVolSlider, 25);
    lv_obj_set_height(ui_PadVolSlider, 150);
    lv_obj_set_x(ui_PadVolSlider, -288); lv_obj_set_y(ui_PadVolSlider, 68);
    lv_obj_set_align(ui_PadVolSlider, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_PadVolSlider, lv_color_hex(0x333333), LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PadVolSlider,  255,                      LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PadVolSlider, lv_color_hex(0xFF9C00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PadVolSlider,  255,                      LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PadVolSlider, lv_color_hex(0xFF9C00), LV_PART_KNOB      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PadVolSlider,  255,                      LV_PART_KNOB      | LV_STATE_DEFAULT);

    // ── Button grid — 105×45 pills ────────────────────────────────────────────
    // Cols (center-rel x): -148, -28,  93, 217, 332
    // Rows (center-rel y):  -41,  20,  62, 117, 184
    // Empty slots: Col1 Rows2,3; Col2 Row2; Col3 Rows1,3,4; Col4 Row3; Col5 Rows2,4

    // Row1 y=-41
    make_s2_btn(ui_Screen2, -148, -41, 105, 45, 0xFF9966, &ui_Button52,
                "BRIDGE\nASM");
    make_s2_btn(ui_Screen2,  -28, -41, 105, 45, 0xCC99CC, &ui_Button53,
                "ENGROOM\nASM");
    // Col3 Row1 — empty
    make_s2_btn(ui_Screen2,  217, -41, 105, 45, 0xCC99CC, &ui_AllOffBtn,
                "ALL OFF");
    make_s2_btn(ui_Screen2,  332, -41, 105, 45, 0xFF9966, &ui_WarpCoreBtn,
                warpCoreOn ? "WARP\nCORE ON" : "WARP\nCORE");

    // Row2 y=20
    // Col1 Row2 — empty
    // Col2 Row2 — empty
    make_s2_btn(ui_Screen2,   93,  20, 105, 45, 0xFFCC99, &ui_PadDestBtn,
                "PAD\nAUDIO");
    make_s2_btn(ui_Screen2,  217,  20, 105, 45, 0xFFFFAA, &ui_AllOnBtn,
                "ALL ON");
    // Col5 Row2 — empty

    // Row3 y=62
    // Col1 Row3 — empty
    make_s2_btn(ui_Screen2,  -28,  62, 105, 45, 0xFFFFAA, &ui_SyncModeBtn,
                nvsSyncMode ? "ASSEMBLED" : "SEPARATED");
    // Col3 Row3 — empty
    // Col4 Row3 — empty
    make_s2_btn(ui_Screen2,  332,  62, 105, 45, 0xCC99CC, &ui_RadarBtn,
                radarEnabled ? "SENSORS\nON" : "SENSORS\nOFF");

    // Row4 y=117
    make_s2_btn(ui_Screen2, -148, 117, 105, 45, 0xCC99CC, &ui_Button1,
                "WIFI");
    make_status_btn(ui_Screen2, -28, 117, 105, 45, &ui_BridgeStatusBtn,
                    "BRIDGE\nOFFLINE");
    // Col3 Row4 — empty
    make_s2_btn(ui_Screen2,  217, 117, 105, 45, 0xFFCC99, &ui_TouchSndBtn,
                touchSoundEnabled ? "TOUCH SND\nON" : "TOUCH SND");
    make_s2_btn(ui_Screen2,  332, 117, 105, 45, 0xFF9966, &ui_RadarAlertBtn,
                redAlertSensorEnabled ? "ALERT\nSEN ON" : "ALERT\nSENSOR");

    // Row5 y=184
    make_s2_btn(ui_Screen2, -148, 184, 105, 45, 0xCC99CC, &ui_Button18,
                "MAIN\nMENU");
    make_status_btn(ui_Screen2, -28, 184, 105, 45, &ui_EngRoomStatusBtn,
                    "ENGROOM\nOFFLINE");
    make_s2_btn(ui_Screen2,   93, 184, 105, 45, 0xFF9966, &ui_WinBtn,
                windowsOn ? "WINDOWS\nON" : "WINDOWS\nOFF");
    make_s2_btn(ui_Screen2,  217, 184, 105, 45, 0xCC99CC, &ui_AmbientBtn,
                ambientEnabled ? "AMBIENT\nON" : "AMBIENT");
    // Col5 Row5 — empty

    // ── Register events ───────────────────────────────────────────────────────
    lv_obj_add_event_cb(ui_Button18,        ui_event_Button18,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Button1,         ui_event_Button1,         LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Button52,        ui_event_Button52,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Button53,        ui_event_Button53,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SyncModeBtn,     ui_event_SyncModeBtn,     LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PadDestBtn,      ui_event_PadDestBtn,      LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WinBtn,          ui_event_WinBtn,          LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_RadarAlertBtn,   ui_event_RadarAlertBtn,   LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_RadarBtn,        ui_event_RadarBtn,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WarpCoreBtn,     ui_event_WarpCoreBtn,     LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AllOffBtn,       ui_event_AllOffBtn,       LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AllOnBtn,        ui_event_AllOnBtn,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_TouchSndBtn,     ui_event_TouchSndBtn,     LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AmbientBtn,      ui_event_AmbientBtn,      LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_BridgeVolSlider, ui_event_BridgeVolSlider, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PadVolSlider,    ui_event_PadVolSlider,    LV_EVENT_ALL, NULL);
}

void ui_Screen2_screen_destroy(void)
{
    if (ui_Screen2) lv_obj_del(ui_Screen2);
    ui_Screen2          = NULL;
    ui_Button18         = NULL;
    ui_Button1          = NULL;
    ui_Button52         = NULL;
    ui_Button53         = NULL;
    ui_BridgeStatusBtn  = NULL;
    ui_EngRoomStatusBtn = NULL;
    ui_SyncModeBtn      = NULL;
    ui_AllOffBtn        = NULL;
    ui_AllOnBtn         = NULL;
    ui_TouchSndBtn      = NULL;
    ui_AmbientBtn       = NULL;
    ui_PadDestBtn       = NULL;
    ui_PadVolSlider     = NULL;
    ui_BridgeVolSlider  = NULL;
    ui_WarpCoreBtn      = NULL;
    ui_RadarBtn         = NULL;
    ui_WinBtn           = NULL;
    ui_RadarAlertBtn    = NULL;
    ui_BridgeBatLabel   = NULL;
    ui_EngRoomBatLabel  = NULL;
}
