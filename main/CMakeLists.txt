idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        water_port
        water_driver
        water_service
        esp_timer
        freertos
)
