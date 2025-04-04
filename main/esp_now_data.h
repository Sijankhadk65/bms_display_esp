#ifndef ESP_NOW_DATA_H
#define ESP_NOW_DATA_H

#include <stdbool.h>

// Structure to store received data
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

#endif // ESP_NOW_DATA_H