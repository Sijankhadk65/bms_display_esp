#include <stdlib.h>
#include <string.h>
#include "math.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include <lvgl.h>

#define TAG "ESP_NOW_RECEIVER"
#define APP_TAG "MAIN_APP"

#define MAX_DATA_POINTS 50
#define DATA_UPDATE_INTERVAL_MS 50

#define MAX_DATA_POINTS 50

// Structure to hold graph data
typedef struct
{
    float voltage_data[MAX_DATA_POINTS];
    int current_index;
} VoltageGraphData;

// Global graph data
VoltageGraphData graph_data = {0};

// Function to add new voltage data point
void add_voltage_data_point(float voltage)
{
    // Shift existing data points
    for (int i = MAX_DATA_POINTS - 1; i > 0; i--)
    {
        graph_data.voltage_data[i] = graph_data.voltage_data[i - 1];
    }

    // Add new data point at the start
    graph_data.voltage_data[0] = voltage;

    // Increment index (capped at MAX_DATA_POINTS)
    if (graph_data.current_index < MAX_DATA_POINTS)
    {
        graph_data.current_index++;
    }
}

// Create a custom chart object for voltage graph
lv_obj_t *create_voltage_chart(lv_obj_t *parent)
{
    // Create chart
    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 240, 120);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);

    // Configure chart
    lv_chart_set_point_count(chart, MAX_DATA_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 9);

    // Create a series for voltage
    lv_chart_series_t *voltage_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    return chart;
}

// Structure to store received data
typedef struct
{
    char timeStamp[32];
    unsigned int status;
    float batt1_voltage;
    float batt2_voltage;
    float total_voltage;
    bool isBalancing;
    bool isOverload;
    bool shouldCharge;
    bool isOutputLoad;
} esp_now_data_t;

// Peer MAC address (replace with your master device's MAC address)
uint8_t master_mac[ESP_NOW_ETH_ALEN] = {0x78, 0x21, 0x84, 0x8d, 0x09, 0x48};

// Callback function when data is received
static void esp_now_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    // Check if the data size matches our expected structure
    if (data_len != sizeof(esp_now_data_t))
    {
        ESP_LOGE(TAG, "Received invalid data length");
        return;
    }

    // Cast received data to our data structure
    esp_now_data_t *received_data = (esp_now_data_t *)data;

    // Log received data
    ESP_LOGI(TAG, "Received Data:");
    ESP_LOGI(TAG, "Battery 1 Voltage: %f", received_data->batt1_voltage);
    ESP_LOGI(TAG, "Battery 2 Voltage: %f", received_data->batt2_voltage);
    ESP_LOGI(TAG, "Total Voltage: %f", received_data->total_voltage);
}

void wifi_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP adapter
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi in station mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void initialize_esp_now(void)
{
    // Initialize ESP-NOW
    esp_err_t err = esp_now_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize ESP-NOW: %s", esp_err_to_name(err));
        return;
    }

    // Register receive callback
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

void app_main(void)
{
    // Initialize WiFi
    wifi_init();

    // Initialize ESP-NOW
    initialize_esp_now();

    ESP_LOGI(TAG, "ESP-NOW Receiver Ready");

    // LCD IO and Panel Handles
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;

    // Initialize LCD hardware
    esp_err_t ret = app_lcd_init(&lcd_io, &lcd_panel);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "LCD initialization failed");
        return;
    }

    // Initialize LCD Backlight
    lcd_display_brightness_init();
    lcd_display_brightness_set(50); // Set to 50% brightness

    // Initialize LVGL with the LCD
    lv_display_t *disp = app_lvgl_init(lcd_io, lcd_panel);
    if (disp == NULL)
    {
        ESP_LOGE(TAG, "LVGL initialization failed");
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
