#include "voltage_graph.h"
#include <math.h>

// Global graph data
VoltageGraphData graph_data = {0};

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