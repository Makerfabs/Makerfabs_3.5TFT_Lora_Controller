SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen3.c
    screens/ui_Screen4.c
    screens/ui_Screen6.c
    screens/ui_Screen7.c
    screens/ui_Screen8.c
    screens/ui_Screen2.c
    screens/ui_Screen9.c
    screens/ui_Screen10.c
    screens/ui_Screen5.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_bg1_png.c
    images/ui_img_1_png.c
    images/ui_img_back_png.c
    images/ui_img_bg_png.c
    images/ui_img_but2_png.c
    images/ui_img_bg2_png.c
    images/ui_img_but4_png.c
    images/ui_img_but3_png.c
    images/ui_img_wifi_png.c
    fonts/ui_font_Font1.c
    fonts/ui_font_Font2.c)

add_library(ui ${SOURCES})
