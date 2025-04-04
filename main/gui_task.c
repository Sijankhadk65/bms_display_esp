#include "gui_task.h"
#include <lvgl.h>
#include <stdio.h>
#include "esp_log.h"

#include "lcd.h"

#define TAG "GUI"

static lv_style_t icon_style;
char data_text[100];

lv_obj_t *screen;
lv_obj_t *batt1_label;
lv_obj_t *batt2_label;
lv_obj_t *total_label;
lv_obj_t *state_label;
lv_obj_t *load_status_label;
lv_obj_t *balance_status_label;
lv_obj_t *charge_status_label;

float batt1 = 0;
float batt2 = 0;
float total = 0;
unsigned int bms_status = 0;
bool isBalancing = 0;
bool isCharging = 0;
bool isOutputLoad = 0;
bool hasLoad = 0;
float load_current = 0;

void set_battery_indicator_GUI(float voltage)
{
    if (voltage > 2.25 && voltage < 4.5)
    {
        snprintf(data_text, sizeof(data_text), "%s : %.2f ", LV_SYMBOL_BATTERY_3, voltage);
        lv_style_set_text_color(&icon_style, lv_color_make(12, 229, 12));
    }
    else if (voltage < 2.25 && voltage > 1.0)
    {
        snprintf(data_text, sizeof(data_text), "%s : %.2f ", LV_SYMBOL_BATTERY_2, voltage);
        lv_style_set_text_color(&icon_style, lv_color_make(12, 117, 229));
    }
    else
    {
        snprintf(data_text, sizeof(data_text), "%s : %.2f ", LV_SYMBOL_BATTERY_1, voltage);
        lv_style_set_text_color(&icon_style, lv_color_make(12, 12, 229)); // BGR
    }
}

static void create_gui(void)
{
    // Create a screen
    screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    batt1_label = lv_label_create(screen);
    lv_obj_set_pos(batt1_label, 50, 0);

    batt2_label = lv_label_create(screen);
    lv_obj_set_pos(batt2_label, 50, 20);

    total_label = lv_label_create(screen);
    lv_obj_set_pos(total_label, 50, 40);

    state_label = lv_label_create(screen);
    lv_obj_set_pos(state_label, 50, 60);

    load_status_label = lv_label_create(screen);
    lv_obj_set_pos(load_status_label, 50, 80);

    balance_status_label = lv_label_create(screen);
    lv_obj_set_pos(balance_status_label, 50, 100);

    charge_status_label = lv_label_create(screen);
    lv_obj_set_pos(charge_status_label, 50, 120);
}

static void update_gui()
{
    set_battery_indicator_GUI(batt1);
    lv_obj_add_style(batt1_label, &icon_style, 0);
    lv_label_set_text(batt1_label, data_text);

    set_battery_indicator_GUI(batt2);
    lv_obj_add_style(batt2_label, &icon_style, 0);
    lv_label_set_text(batt2_label, data_text);

    set_battery_indicator_GUI(total);
    lv_obj_add_style(total_label, &icon_style, 0);
    lv_label_set_text(total_label, data_text);

    if (bms_status == 0)
    {
        snprintf(data_text, sizeof(data_text), "%s IDLE", LV_SYMBOL_PAUSE);
    }
    else if (bms_status == 1)
    {
        snprintf(data_text, sizeof(data_text), "%s DISCONNECTED", LV_SYMBOL_WARNING);
    }
    else if (bms_status == 2)
    {
        snprintf(data_text, sizeof(data_text), "%s CHARGING", LV_SYMBOL_CHARGE);
    }
    else if (bms_status == 3)
    {
        snprintf(data_text, sizeof(data_text), "%s DISCHARGING", LV_SYMBOL_POWER);
    }
    else if (bms_status == 4)
    {
        snprintf(data_text, sizeof(data_text), "%s BALANCING", LV_SYMBOL_REFRESH);
    }
    lv_label_set_text(state_label, data_text);

    if (hasLoad)
    {
        snprintf(data_text, sizeof(data_text), "Load %s", LV_SYMBOL_OK);
    }
    else
    {
        snprintf(data_text, sizeof(data_text), "Load %s", LV_SYMBOL_CLOSE);
    }
    lv_label_set_text(load_status_label, data_text);

    if (isBalancing)
    {
        snprintf(data_text, sizeof(data_text), "Balancing %s", LV_SYMBOL_OK);
    }
    else
    {
        snprintf(data_text, sizeof(data_text), "Balancing %s", LV_SYMBOL_CLOSE);
    }
    lv_label_set_text(balance_status_label, data_text);

    if (isCharging)
    {
        snprintf(data_text, sizeof(data_text), "Charging %s", LV_SYMBOL_OK);
    }
    else
    {
        snprintf(data_text, sizeof(data_text), "Charging %s", LV_SYMBOL_CLOSE);
    }
    lv_label_set_text(charge_status_label, data_text);
}