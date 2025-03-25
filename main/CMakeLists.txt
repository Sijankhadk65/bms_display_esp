idf_component_register(SRCS "voltage_graph.c" "esp_now_receiver.c" "hello_world_main.c" "lcd.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS ".")
