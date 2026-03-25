idf_component_register(
    SRCS
        "d_w5500/d_w5500.c"
        "d_w5500/d_w5500_reg.c"
        "d_w5500/d_w5500_socket.c"
        "d_w5500/d_w5500_net.c"
        "d_input/d_switch.c"
        "d_display/d_ws2812.c"
        "d_display/d_ht16k33.c"
        "d_pipette/d_pipette_232.c"
        "d_pipette/d_pipette_can.c"

    INCLUDE_DIRS
        "."
        "d_w5500"
        "d_input"
        "d_display"
        "d_pipette"

    REQUIRES
        water_common
        water_port
        driver
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
)
