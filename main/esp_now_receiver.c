#include "esp_now_receiver.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define TAG "ESP_NOW_RECEIVER"

uint8_t master_mac[ESP_NOW_ETH_ALEN] = {0x78, 0x21, 0x84, 0x8d, 0x09, 0x48};

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

    // batt1 = received_data->batt1_voltage;
    // batt2 = received_data->batt2_voltage;
    // total = received_data->total_voltage;
    // bms_status = received_data->state;
    // isBalancing = received_data->isBalancing;
    // isCharging = received_data->isCharging;
    // isOutputLoad = received_data->isOutputLoad;
    // hasLoad = received_data->hasLoad;
    // load_current = received_data->load_current;
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
