#include <stdlib.h>
#include <string.h>
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "voltage_graph.h"
#include <lvgl.h>

#define TAG "ESP_RECEIVER"
#define APP_TAG "MAIN_APP"

typedef struct
{
    unsigned int state;
    char err[20];
    float batt1_voltage;
    float batt2_voltage;
    float total_voltage;
    float load_current;
    bool isBalancing;
    bool isCharging;
    bool isOutputLoad;
    bool hasLoad;
} esp_now_data_t;

uint8_t master_mac[ESP_NOW_ETH_ALEN] = {0x78, 0x21, 0x84, 0x8d, 0x09, 0x48};

float batt1 = 0;
float batt2 = 0;
float total = 0;
unsigned int bms_status = 0;
bool isBalancing;
bool isCharging;
bool isOutputLoad;
bool hasLoad;
float load_current;

char data_text[100];

static lv_style_t icon_style;

static void esp_now_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    if (data_len != sizeof(esp_now_data_t))
    {
        ESP_LOGE(TAG, "Received invalid data length");
        return;
    }

    esp_now_data_t *received_data = (esp_now_data_t *)data;

    ESP_LOGI(TAG, "Received Data:");
    ESP_LOGI(TAG, "Battery 1 Voltage: %f", received_data->batt1_voltage);
    ESP_LOGI(TAG, "Battery 2 Voltage: %f", received_data->batt2_voltage);
    ESP_LOGI(TAG, "Total Voltage: %f", received_data->total_voltage);

    batt1 = received_data->batt1_voltage;
    batt2 = received_data->batt2_voltage;
    total = received_data->total_voltage;
    bms_status = received_data->state;
    isBalancing = received_data->isBalancing;
    isCharging = received_data->isCharging;
    isOutputLoad = received_data->isOutputLoad;
    hasLoad = received_data->hasLoad;
    load_current = received_data->load_current;
}

void wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void initialize_esp_now(void)
{
    esp_err_t err = esp_now_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW: %s", esp_err_to_name(err));
        return;
    }

    err = esp_now_register_recv_cb(esp_now_recv_callback);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register receive callback: %s", esp_err_to_name(err));
        return;
    }

    // Add peer (master device)
    esp_now_peer_info_t peer_info = {
        .peer_addr = {0},
        .channel = 0,
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false};
    memcpy(peer_info.peer_addr, master_mac, ESP_NOW_ETH_ALEN);

    err = esp_now_add_peer(&peer_info);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "ESP-NOW Initialized Successfully");
}

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

    lv_obj_t *batt1_label = lv_label_create(screen);
    lv_obj_set_pos(batt1_label, 50, 0);

    lv_obj_t *batt2_label = lv_label_create(screen);
    lv_obj_set_pos(batt2_label, 50, 20);

    lv_obj_t *total_label = lv_label_create(screen);
    lv_obj_set_pos(total_label, 50, 40);

    lv_obj_t *state_label = lv_label_create(screen);
    lv_obj_set_pos(state_label, 50, 60);

    lv_obj_t *load_status_label = lv_label_create(screen);
    lv_obj_set_pos(load_status_label, 50, 80);

    lv_obj_t *balance_status_label = lv_label_create(screen);
    lv_obj_set_pos(balance_status_label, 50, 100);

    lv_obj_t *charge_status_label = lv_label_create(screen);
    lv_obj_set_pos(charge_status_label, 50, 120);

    while (1)
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

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
