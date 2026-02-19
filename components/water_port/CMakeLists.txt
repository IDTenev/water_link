idf_component_register(
    SRCS
        "p_gpio/p_gpio.c"
        "p_can/p_can.c"
        "p_i2c/p_i2c.c"
        "p_rs232/p_rs232.c"
        "p_rs485/p_rs485.c"
        "p_spi/p_spi.c"

    INCLUDE_DIRS
        "."
        "p_gpio"
        "p_can"
        "p_i2c"
        "p_rs232"
        "p_rs485"
        "p_spi"

    REQUIRES
        water_common
        driver
)
