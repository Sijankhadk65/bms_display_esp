#include <stdlib.h>
#include <string.h>
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include "esp_now_receiver.h"
#include "voltage_graph.h"
#include <lvgl.h>

#define APP_TAG "MAIN_APP"

#define DATA_UPDATE_INTERVAL_MS 50

void app_main(void)
{
    // Initialize WiFi
    wifi_init();

    // Initialize ESP-NOW
    initialize_esp_now();

    // LCD IO and Panel Handles
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;

    // Initialize LCD hardware
    esp_err_t ret = app_lcd_init(&lcd_io, &lcd_panel);
    if (ret != ESP_OK)
    {
        ESP_LOGE(APP_TAG, "LCD initialization failed");
        return;
    }

    // Initialize LCD Backlight
    lcd_display_brightness_init();
    lcd_display_brightness_set(50); // Set to 50% brightness

    // Initialize LVGL with the LCD
    lv_display_t *disp = app_lvgl_init(lcd_io, lcd_panel);
    if (disp == NULL)
    {
        ESP_LOGE(APP_TAG, "LVGL initialization failed");
        return;
    }

    // Create a screen
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    // Title label
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Voltage Graph");
    lv_obj_set_style_text_color(title_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create voltage chart
    lv_obj_t *voltage_chart = create_voltage_chart(screen);
    lv_obj_center(voltage_chart);

    // Get the voltage series
    lv_chart_series_t *voltage_series = lv_chart_get_series_next(voltage_chart, NULL);

    // Periodically update the display with simulated voltage data
    int counter = 0;
    while (1)
    {
        // Simulate voltage data (sine wave between 0-9V)
        float voltage = 4.5 + 4.5 * sin(counter * 0.1);

        // Add data point to chart
        lv_chart_set_next_value(voltage_chart, voltage_series, (int16_t)(voltage * 10));

        // Increment counter
        counter++;

        // Delay to simulate update interval
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
