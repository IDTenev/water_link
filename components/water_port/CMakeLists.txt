idf_component_register(
    SRCS
        "gpio/gpio.c"
        "can/can.c"
        "i2c/i2c.c"
        "rs232/rs232.c"
        "rs485/rs485.c"
        "spi/spi.c"

    INCLUDE_DIRS
        "."
        "gpio"
        "can"
        "i2c"
        "rs232"
        "rs485"
        "spi"

    REQUIRES
        water_common
        driver
)
