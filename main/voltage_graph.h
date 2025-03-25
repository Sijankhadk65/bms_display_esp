#ifndef VOLTAGE_GRAPH_H
#define VOLTAGE_GRAPH_H

#include <lvgl.h>

#define MAX_DATA_POINTS 50

// Structure to hold graph data
typedef struct
{
    float voltage_data[MAX_DATA_POINTS];
    int current_index;
} VoltageGraphData;

// Function prototypes
void add_voltage_data_point(float voltage);
lv_obj_t *create_voltage_chart(lv_obj_t *parent);

#endif // VOLTAGE_GRAPH_H