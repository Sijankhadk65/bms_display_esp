#ifndef GUI_TASSK_H
#define GUI_TASK_H

#include <lvgl.h>

// extern static lv_style_t icon_style;
// extern char data_text[100];
void set_battery_indicator_GUI(float voltage);
static void create_gui(void);
static void update_gui();

#endif