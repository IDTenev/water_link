#include <stdio.h>

#include "gpio.h"
#include "rs232.h"
#include "rs485.h"
#include "i2c.h"
#include "spi.h"
#include "can.h"

#include "w5500.h"

void app_main(void)
{
    printf("Boot Water v1.1\n");

    printf("Initialing System\n");
    //port init
    gpio_init();
    rs232_init();
    rs485_init();
    i2c_init();
    spi_init();
    can_init();

    //driver init
    w5500_init();

    printf("Start Water-Link v1.0\n");
}
