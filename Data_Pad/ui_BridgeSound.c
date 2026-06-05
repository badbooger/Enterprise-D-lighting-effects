// DataPad — BridgeSound screen (hand-written, do not replace with SLS export)
// LCARS frame matches Screen1/Screen2: same decorative block positions and colors.
// Center-relative positioning throughout (LV_ALIGN_CENTER), matching Screen1/2 style.
// Screen: 800x480 landscape.

#include "ui.h"

lv_obj_t * ui_BridgeSound      = NULL;
lv_obj_t * ui_SndBackBtn       = NULL;
lv_obj_t * ui_SndBackLabel     = NULL;
lv_obj_t * ui_SndTitle         = NULL;
lv_obj_t * ui_SndPlay1Btn      = NULL;   // POWER
lv_obj_t * ui_SndPlay1Label    = NULL;
lv_obj_t * ui_SndPlay2Btn      = NULL;   // ALERTS
lv_obj_t * ui_SndPlay2Label    = NULL;
lv_obj_t * ui_SndPlay3Btn      = NULL;   // DAMAGE
lv_obj_t * ui_SndPlay3Label    = NULL;
lv_obj_t * ui_SndPlay4Btn      = NULL;   // WARP
lv_obj_t * ui_SndPlay4Label    = NULL;
lv_obj_t * ui_SndPlay5Btn      = NULL;   // WEAPONS
lv_obj_t * ui_SndPlay5Label    = NULL;
lv_obj_t * ui_SndPlay6Btn      = NULL;   // VOICED (DataPad only)
lv_obj_t * ui_SndPlay6Label    = NULL;
lv_obj_t * ui_SndPlay7Btn      = NULL;   // AMBIENT (DataPad only)
lv_obj_t * ui_SndPlay7Label    = NULL;
lv_obj_t * ui_SndPlay8Btn      = NULL;   // EXTRAS (DataPad only)
lv_obj_t * ui_SndPlay8Label    = NULL;
lv_obj_t * ui_SndStopBtn       = NULL;
lv_obj_t * ui_SndStopLabel     = NULL;
lv_obj_t * ui_SndPlayLastBtn   = NULL;
lv_obj_t * ui_SndPlayLastLabel = NULL;
lv_obj_t * ui_SndVolLabel      = NULL;
lv_obj_t * ui_SndVolSlider     = NULL;
lv_obj_t * ui_SndRepeatBtn     = NULL;
lv_obj_t * ui_SndRepeatLabel   = NULL;

// ── Event callbacks ───────────────────────────────────────────────────────────

void ui_event_SndBackBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
}

void ui_event_SndPlay1Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundPower(e);    }
void ui_event_SndPlay2Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundAlerts(e);   }
void ui_event_SndPlay3Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundDamage(e);   }
void ui_event_SndPlay4Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundWarp(e);     }
void ui_event_SndPlay5Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundWeapons(e);  }
void ui_event_SndPlay6Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundVoiced(e);   }
void ui_event_SndPlay7Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundAmbient(e);  }
void ui_event_SndPlay8Btn(lv_event_t * e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundExtras(e);   }
void ui_event_SndStopBtn(lv_event_t * e)     { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundStop(e);     }
void ui_event_SndPlayLastBtn(lv_event_t * e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundPlayLast(e); }

void ui_event_SndVolSlider(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) SoundVolume(e);
}

void ui_event_SndRepeatBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) SoundRepeat(e);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// Non-clickable LCARS frame block (radius=0 for sidebar/header, or 700 for pills).
static lv_obj_t * make_dec(lv_obj_t * parent, int cx, int cy, int w, int h,
                            uint32_t bg_hex, int radius)
{
    lv_obj_t * obj = lv_btn_create(parent);
    lv_obj_set_x(obj, cx);
    lv_obj_set_y(obj, cy);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_align(obj, LV_ALIGN_CENTER);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                      LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

// Non-functional labeled pill — visually identical to a functional button but has no action.
static void make_dec_lbl(lv_obj_t * parent, int cx, int cy, int w, int h,
                          uint32_t bg_hex, const char * text)
{
    lv_obj_t * btn = make_dec(parent, cx, cy, w, h, bg_hex, 700);
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
}

// Interactive pill button. cx/cy are center-relative offsets (LV_ALIGN_CENTER).
static lv_obj_t * make_snd_btn(lv_obj_t * parent, int cx, int cy, int w, int h,
                                uint32_t bg_hex, lv_obj_t ** label_out,
                                const char * label_text, const lv_font_t * font)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_x(btn, cx);
    lv_obj_set_y(btn, cy);
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
    lv_label_set_text(lbl, label_text);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(lbl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (label_out) *label_out = lbl;
    return btn;
}

// ── Screen init ───────────────────────────────────────────────────────────────

void ui_BridgeSound_screen_init(void)
{
    ui_BridgeSound = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_BridgeSound, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_BridgeSound, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BridgeSound, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── LCARS decorative frame — same block positions as Screen1 ─────────────

    make_dec(ui_BridgeSound, -340, -218,  87,  32, 0xCC99FF, 0);
    make_dec(ui_BridgeSound, -317, -124, 132, 147, 0xCC99FF, 0);
    make_dec(ui_BridgeSound, -250, -197,  91,  84, 0x000000, 700);
    make_dec(ui_BridgeSound, -340,   -4,  87,  82, 0xCC99FF, 0);
    make_dec(ui_BridgeSound, -251,  -56,  91, 100, 0x000000, 700);
    make_dec(ui_BridgeSound, -340,   60,  87,  36, 0xFF9966, 0);
    make_dec(ui_BridgeSound, -340,  102,  87,  36, 0xFF9966, 0);
    make_dec(ui_BridgeSound, -340,  137,  87,  22, 0xFF9966, 0);
    make_dec(ui_BridgeSound, -340,  195,  87,  80, 0xFF9966, 0);

    lv_obj_t * lcars_hdr = make_dec(ui_BridgeSound, -116, -131, 261, 49, 0xCC99FF, 0);
    make_dec(ui_BridgeSound,   26, -131,  13, 49, 0xFFBBAA, 0);
    make_dec(ui_BridgeSound,   76, -131,  77, 49, 0xFFBBAA, 0);
    make_dec(ui_BridgeSound,  126, -131,  13, 49, 0xFFBBAA, 0);
    make_dec(ui_BridgeSound,  151, -131,  27, 49, 0x8899FF, 0);
    make_dec(ui_BridgeSound,  280, -131, 220, 49, 0x8899FF, 0);

    ui_SndTitle = lv_label_create(lcars_hdr);
    lv_obj_set_width(ui_SndTitle, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SndTitle, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SndTitle, 0);
    lv_obj_set_y(ui_SndTitle, 13);
    lv_obj_set_align(ui_SndTitle, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SndTitle, "BRIDGE SOUND");
    lv_obj_set_style_text_color(ui_SndTitle, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SndTitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SndTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Vertical volume slider ────────────────────────────────────────────────
    // Centered horizontally in the gap between the LCARS sidebar (right edge
    // abs x=104) and the button grid (left edge abs x=230): midpoint = 167.
    // abs (150,185), size (35,200) — wider, shorter, and centred in that gap.
    ui_SndVolSlider = lv_slider_create(ui_BridgeSound);
    lv_obj_set_pos(ui_SndVolSlider, 150, 185);
    lv_obj_set_size(ui_SndVolSlider, 35, 200);
    lv_obj_set_align(ui_SndVolSlider, LV_ALIGN_TOP_LEFT);
    lv_slider_set_range(ui_SndVolSlider, 0, 30);
    lv_slider_set_value(ui_SndVolSlider, 15, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_SndVolSlider, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SndVolSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SndVolSlider, lv_color_hex(0xFF9C00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SndVolSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SndVolSlider, lv_color_hex(0xFF9C00), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SndVolSlider, 255, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_SndVolLabel = lv_label_create(ui_BridgeSound);
    lv_obj_set_pos(ui_SndVolLabel, 148, 389);
    lv_obj_set_align(ui_SndVolLabel, LV_ALIGN_TOP_LEFT);
    lv_label_set_text(ui_SndVolLabel, "VOL");
    lv_obj_set_style_text_color(ui_SndVolLabel, lv_color_hex(0xFF9C00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SndVolLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SndVolLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── 5×5 button grid — 100×45 pills, functional + decorative mixed ─────────
    // Slider 20px, gap 10 → buttons start abs x=230.
    // Col spacing: 100+15px gap. Col centers (rel): -120, -5, 110, 225, 340.
    // Row spacing: 45+22px gap. Row centers (rel): -73, -6, 61, 128, 195.
    // Col gap (15px) now close to row gap (22px) — much more even than before.
    // Functional and decorative buttons are mixed throughout the grid.
    //
    //  POWER    SENSORS   ALERTS    TACTICAL  DAMAGE
    //  SHIELDS  WEAPONS   COMMS     WARP      MEDICAL
    //  VOICED   SECURITY  AMBIENT   NAV SYS   EXTRAS
    //  ENGINES  STOP      PLAY LAST REPEAT    SCIENCE
    //  BACK     SYSTEMS   STATUS    GRID      OPS

    // Row 0
    ui_SndPlay1Btn = make_snd_btn(ui_BridgeSound, -120, -73, 100, 45, 0xFFFFAA,
                                  &ui_SndPlay1Label, "POWER",    &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,    -5, -73, 100, 45, 0xFF9966,  "SENSORS");
    ui_SndPlay2Btn = make_snd_btn(ui_BridgeSound,  110, -73, 100, 45, 0xFF9C00,
                                  &ui_SndPlay2Label, "ALERTS",   &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,   225, -73, 100, 45, 0xCC99FF,  "TACTICAL");
    ui_SndPlay3Btn = make_snd_btn(ui_BridgeSound,  340, -73, 100, 45, 0xFFBBAA,
                                  &ui_SndPlay3Label, "DAMAGE",   &lv_font_montserrat_10);

    // Row 1
    make_dec_lbl(ui_BridgeSound,  -120,  -6, 100, 45, 0xFF9966,  "SHIELDS");
    ui_SndPlay5Btn = make_snd_btn(ui_BridgeSound,   -5,  -6, 100, 45, 0xFF9C00,
                                  &ui_SndPlay5Label, "WEAPONS",  &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,   110,  -6, 100, 45, 0xFFCC99,  "COMMS");
    ui_SndPlay4Btn = make_snd_btn(ui_BridgeSound,   225,  -6, 100, 45, 0x8899FF,
                                  &ui_SndPlay4Label, "WARP",     &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,   340,  -6, 100, 45, 0xCC99CC,  "MEDICAL");

    // Row 2
    ui_SndPlay6Btn = make_snd_btn(ui_BridgeSound, -120,  61, 100, 45, 0xCC99CC,
                                  &ui_SndPlay6Label, "VOICED",   &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,    -5,  61, 100, 45, 0xFF9966,  "SECURITY");
    ui_SndPlay7Btn = make_snd_btn(ui_BridgeSound,  110,  61, 100, 45, 0xCC99FF,
                                  &ui_SndPlay7Label, "AMBIENT",  &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,   225,  61, 100, 45, 0xFFBBAA,  "NAV SYS");
    ui_SndPlay8Btn = make_snd_btn(ui_BridgeSound,  340,  61, 100, 45, 0xFFCC99,
                                  &ui_SndPlay8Label, "EXTRAS",   &lv_font_montserrat_10);

    // Row 3
    make_dec_lbl(ui_BridgeSound,  -120, 128, 100, 45, 0xFF9966,  "ENGINES");
    ui_SndStopBtn = make_snd_btn(ui_BridgeSound,    -5, 128, 100, 45, 0xCC2222,
                                 &ui_SndStopLabel,  "STOP",      &lv_font_montserrat_10);
    ui_SndPlayLastBtn = make_snd_btn(ui_BridgeSound, 110, 128, 100, 45, 0xFFFFAA,
                                     &ui_SndPlayLastLabel, "PLAY\nLAST", &lv_font_montserrat_10);
    ui_SndRepeatBtn = make_snd_btn(ui_BridgeSound,  225, 128, 100, 45, 0x6666CC,
                                   &ui_SndRepeatLabel, "REPEAT\nOFF", &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,   340, 128, 100, 45, 0xCC99FF,  "SCIENCE");

    // Row 4
    ui_SndBackBtn = make_snd_btn(ui_BridgeSound,  -120, 195, 100, 45, 0xFF9C00,
                                 &ui_SndBackLabel, "< BACK",     &lv_font_montserrat_10);
    make_dec_lbl(ui_BridgeSound,    -5, 195, 100, 45, 0xFFBBAA,  "SYSTEMS");
    make_dec_lbl(ui_BridgeSound,   110, 195, 100, 45, 0xFF9966,  "STATUS");
    make_dec_lbl(ui_BridgeSound,   225, 195, 100, 45, 0xCC99CC,  "GRID");
    make_dec_lbl(ui_BridgeSound,   340, 195, 100, 45, 0xFFCC99,  "OPS");

    // ── Register events ───────────────────────────────────────────────────────
    lv_obj_add_event_cb(ui_SndPlay1Btn,    ui_event_SndPlay1Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay2Btn,    ui_event_SndPlay2Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay3Btn,    ui_event_SndPlay3Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay4Btn,    ui_event_SndPlay4Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay5Btn,    ui_event_SndPlay5Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay6Btn,    ui_event_SndPlay6Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay7Btn,    ui_event_SndPlay7Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlay8Btn,    ui_event_SndPlay8Btn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndStopBtn,     ui_event_SndStopBtn,     LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndPlayLastBtn, ui_event_SndPlayLastBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndVolSlider,   ui_event_SndVolSlider,   LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndRepeatBtn,   ui_event_SndRepeatBtn,   LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SndBackBtn,     ui_event_SndBackBtn,     LV_EVENT_ALL, NULL);
}

void ui_BridgeSound_screen_destroy(void)
{
    if (ui_BridgeSound) lv_obj_del(ui_BridgeSound);
    ui_BridgeSound      = NULL;
    ui_SndBackBtn       = NULL;  ui_SndBackLabel     = NULL;
    ui_SndTitle         = NULL;
    ui_SndPlay1Btn      = NULL;  ui_SndPlay1Label    = NULL;
    ui_SndPlay2Btn      = NULL;  ui_SndPlay2Label    = NULL;
    ui_SndPlay3Btn      = NULL;  ui_SndPlay3Label    = NULL;
    ui_SndPlay4Btn      = NULL;  ui_SndPlay4Label    = NULL;
    ui_SndPlay5Btn      = NULL;  ui_SndPlay5Label    = NULL;
    ui_SndPlay6Btn      = NULL;  ui_SndPlay6Label    = NULL;
    ui_SndPlay7Btn      = NULL;  ui_SndPlay7Label    = NULL;
    ui_SndPlay8Btn      = NULL;  ui_SndPlay8Label    = NULL;
    ui_SndStopBtn       = NULL;  ui_SndStopLabel     = NULL;
    ui_SndPlayLastBtn   = NULL;  ui_SndPlayLastLabel = NULL;
    ui_SndVolLabel      = NULL;  ui_SndVolSlider     = NULL;
    ui_SndRepeatBtn     = NULL;  ui_SndRepeatLabel   = NULL;
}
