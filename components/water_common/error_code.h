typedef enum {
    NO_ERROR = 0,
    GPIO_ERROR,
} error_code_gpio;

typedef enum {
    SPI_OK = 0,
    SPI_ERROR,
    SPI_ERROR_ARGUMENT,
    SPI_ERROR_DRIVER,
} error_code_spi;