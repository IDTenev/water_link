idf_component_register(
    SRCS
        "w5500/w5500.c"

    INCLUDE_DIRS
        "."
        "w5500"

    REQUIRES
        water_common
        water_port
        driver
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
)
