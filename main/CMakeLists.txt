idf_component_register(SRCS "hello_world_main.c" "lcd.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS ".")
