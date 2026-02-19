idf_component_register(
    SRCS
        "d_w5500/d_w5500.c"
        "d_input/d_switch.c"

    INCLUDE_DIRS
        "."
        "d_w5500"
        "d_input"

    REQUIRES
        water_common
        water_port
        driver
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
)
