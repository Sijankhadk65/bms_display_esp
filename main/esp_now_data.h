#ifndef ESP_NOW_DATA_H
#define ESP_NOW_DATA_H

#include <stdbool.h>

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

#endif // ESP_NOW_DATA_H