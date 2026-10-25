SET(SOURCES ui_Screen1.c
    ui_Screen2.c
    ui_Screen3.c
    ui_BridgeSound.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c)

add_library(ui ${SOURCES})
