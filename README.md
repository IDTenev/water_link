# Water-Link v1.0
```
이 모드는 Esp32s3 MCU가 장착된 Water v1.1 보드를 이용합니다.
esp-idf : v5.5.2
ide : vscode
chip : esp32s3 N16R8
```
##### [Esp-IDF 설치는 이쪽을 참고하세요](https://dl.espressif.com/dl/eim/)

## Pinheader 설명
```
3.3v : 0.5A 미만 사용, 과부하에 주의하세요
5v : 2A 미만 사용, 과부하에 주의하세요
5VPWR : USB-C의 전원만 연결 되어 있음

IO38 : GPIO, PWM
IO39 : GPIO, PWM
IO40 : GPIO, PWM
IO41 : GPIO, PWM
IO42 : GPIO, PWM
IO02 : GPIO, PWM, Touch2, ADC1_1, RTC
IO01 : GPIO, PWM, Touch1, ADC1_0, RTC
```

## 외장 포트 설명
![io_port](https://github.com/user-attachments/assets/760f20dc-366c-47ea-8ee6-969aac6ddaff)

### 1. 485
#### Pinout
```
* HW UART와 연결 되어 있음
IO15/RXD1 : 485_RX 
IO16/TXD1 : 485_TX
IO07      : 485_REDE
```

### 2. 232
#### Pinout
```
* SW UART 사용 필요
* HW UART 필요 시 Water의 USB-C를 이용
IO17 : 232_RX 
IO18 : 232_TX
```

### 3. CAN
#### Pinout
```
* CAN2.0 (CANA, CANB) 지원, FDCAN 사용 불가능
* TWAI 이용
IO04 : CAN_TX 
IO05 : CAN_RX
```
#### SideSW
```
종단저항 120옴 설정 가능
CAN_RS : SN65HVD230DR의 8번핀 (RS)를 다음 두 핀중 채결 가능
         1. 3.3v
         2. 4.7k옴 - GND
```

### 4. I2C
#### Pinout
```
IO08 : SDA 
IO09 : SCL
```
#### SideSW
```
I2C BUS PULL UP 설정 가능
     1. 4.7k옴 - 3.3v
     2. X
```

### 5. Ehternet
#### Pinout
```
W5500, SPI 사용
IO10 : CS 
IO11 : MOSI
IO12 : SCK
IO13 : MISO
IO47 : INTn

IO36 : PMODE0 (PULLUP)
IO35 : PMODE1 (PULLUP)
IO45 : PMODE2 (PULLUP)
IO21 : W5500 RST
```

### 6. On Board SW
#### Pinout
```
IO06 : PULLUP
IO14 : PULLUP
IO37 : PULLUP
RST : GND로 연결
```
