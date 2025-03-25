#ifndef ESP_NOW_RECEIVER_H
#define ESP_NOW_RECEIVER_H

#include <esp_now.h>
#include "esp_now_data.h"

extern uint8_t master_mac[ESP_NOW_ETH_ALEN];

void wifi_init(void);
void initialize_esp_now(void);
void esp_now_recv_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

#endif // ESP_NOW_RECEIVER_H