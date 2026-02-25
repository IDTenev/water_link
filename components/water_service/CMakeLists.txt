idf_component_register(
    SRCS
        "s_ethernet/s_udp.c"
        "s_ethernet/s_dhcp.c"
        "s_ethernet/s_ping.c"

    INCLUDE_DIRS
        "."
        "s_ethernet"

    REQUIRES
        water_common
        water_port
        water_driver
        driver
        esp_timer
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
)
