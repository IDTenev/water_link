//-------------------------------------------
//PINOUT
//-------------------------------------------
//GPIO
#define PIN_IO38    38
#define PIN_IO39    39
#define PIN_IO40    40
#define PIN_IO41    41
#define PIN_IO42    42
#define PIN_IO02    02
#define PIN_IO01    01

//SW
#define PIN_IO06    06
#define PIN_IO14    14
#define PIN_IO37    37

//RS485
#define PIN_RS485_RX    15
#define PIN_RS485_TX    16
#define PIN_RS485_DE    07

//RS232
#define PIN_RS232_RX    17
#define PIN_RS232_TX    18

//CAN
#define PIN_CAN_RX    05
#define PIN_CAN_TX    04

//I2C
#define PIN_I2C_SDA   08
#define PIN_I2C_SCL   09

//SPI
#define PIN_SPI_MOSI   11
#define PIN_SPI_MISO   13
#define PIN_SPI_SCK    12
#define PIN_SPI_CS     10
#define PIN_SPI_INT    47

//W5500
#define PIN_W5500_RST     21
#define PIN_W5500_PMODE0  36
#define PIN_W5500_PMODE1  35
#define PIN_W5500_PMODE2  45

//Water
#define PIN_WATER_LED   48
#define PIN_WATER_SW    46
//-------------------------------------------

//-------------------------------------------
//data
//-------------------------------------------
//water ws2812
#define WS2812_WATER_NUM    1

//SW
#define SW_SHORT    0
#define SW_OPEN     1

//RS232
#define RS232_UART_PORT      UART_NUM_1
#define RS232_UART_BAUD      115200

//W5500
#ifndef WATER_SPI_HOST
#define WATER_SPI_HOST SPI2_HOST
#endif

#ifndef WATER_SPI_DMA_CHAN
#define WATER_SPI_DMA_CHAN SPI_DMA_CH_AUTO
#endif

#define W5500_VERSIONR   0x0039

// Common Register Block
#define W5500_BSB_COMMON          (0x00)

// Socket n blocks (n:0..7)
#define W5500_BSB_SOCK_REG(n)     (uint8_t)(0x01 + ((n) * 4))
#define W5500_BSB_SOCK_TX(n)      (uint8_t)(0x02 + ((n) * 4))
#define W5500_BSB_SOCK_RX(n)      (uint8_t)(0x03 + ((n) * 4))

// -------------------------
// Socket Register Offsets (W5500)
// -------------------------
#define W5500_Sn_MR          0x0000
#define W5500_Sn_CR          0x0001
#define W5500_Sn_IR          0x0002
#define W5500_Sn_SR          0x0003
#define W5500_Sn_PORT        0x0004
#define W5500_Sn_DHAR        0x0006
#define W5500_Sn_DIPR        0x000C
#define W5500_Sn_DPORT       0x0010
#define W5500_Sn_MSSR        0x0012
#define W5500_Sn_TOS         0x0015
#define W5500_Sn_TTL         0x0016

#define W5500_Sn_RXBUF_SIZE  0x001E
#define W5500_Sn_TXBUF_SIZE  0x001F

#define W5500_Sn_TX_FSR      0x0020
#define W5500_Sn_TX_RD       0x0022
#define W5500_Sn_TX_WR       0x0024

#define W5500_Sn_RX_RSR      0x0026
#define W5500_Sn_RX_RD       0x0028
#define W5500_Sn_RX_WR       0x002A

// -------------------------
// Socket Commands (Sn_CR)
// -------------------------
#define W5500_CR_OPEN        0x01
#define W5500_CR_LISTEN      0x02
#define W5500_CR_CONNECT     0x04
#define W5500_CR_DISCON      0x08
#define W5500_CR_CLOSE       0x10
#define W5500_CR_SEND        0x20
#define W5500_CR_SEND_MAC    0x21
#define W5500_CR_SEND_KEEP   0x22
#define W5500_CR_RECV        0x40

// -------------------------
// Socket Modes (Sn_MR)
// -------------------------
#define W5500_MR_CLOSE       0x00
#define W5500_MR_TCP         0x01
#define W5500_MR_UDP         0x02
#define W5500_MR_MACRAW      0x04

// -------------------------
// Common Register Offsets (W5500)
// -------------------------
#define W5500_MR        0x0000
#define W5500_GAR       0x0001  // 4B
#define W5500_SUBR      0x0005  // 4B
#define W5500_SHAR      0x0009  // 6B
#define W5500_SIPR      0x000F  // 4B

#define W5500_INTLEVEL  0x0013  // 2B
#define W5500_IR        0x0015  // 1B
#define W5500_IMR       0x0016  // 1B
#define W5500_SIR       0x0017  // 1B
#define W5500_SIMR      0x0018  // 1B

#define W5500_RTR       0x0019  // 2B (retry time-value, unit=100us)
#define W5500_RCR       0x001B  // 1B

#define W5500_PHYCFGR   0x002E  // 1B

//-------------------------------------------