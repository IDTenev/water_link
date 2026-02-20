idf_component_register(
    SRCS
        "d_w5500/d_w5500.c"
        "d_w5500/d_w5500_reg.c"
        "d_input/d_switch.c"
        "d_display/d_ws2812.c"

    INCLUDE_DIRS
        "."
        "d_w5500"
        "d_input"
        "d_display"

    REQUIRES
        water_common
        water_port
        driver
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
)
