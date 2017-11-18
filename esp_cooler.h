#ifndef ESP_COOLER_H
#define ESP_COOLER_H

//#define DEBUG

#define PIN_SDA          (5)
#define PIN_SCL          (4)
#define PIN_DS           (2) // pin 2 is hw pulled up
#define PIN_FAN         (15) // pin15 is hw pull-down

//#define PIN_LIGHT        (2)
//#define PIN_BUTTON       (0)
//#define PIN_LED         (15)
//#define PIN_TFT_DC       (4)
//#define PIN_TFT_CS      (15)
//#define PIN_SD_CS       (16)
//#define PIN_SPI_CLK   (14)
//#define PIN_SPI_MOSI  (13)
//#define PIN_SPI_MISO  (12)

//#define DRV8830_1_ADDRESS      (0x64)

#define EEPROM_SSID_ADDR             (0)
#define EEPROM_PASS_ADDR            (32)
#define EEPROM_MDNS_ADDR            (96)
#define EEPROM_AUTOFAN_ADDR       (128)
#define EEPROM_LOWLIMIT_ADDR      (129)
#define EEPROM_HIGHLIMIT_ADDR     (131)
#define EEPROM_LAST_ADDR          (133)

#define UDP_LOCAL_PORT      (2390)
#define NTP_PACKET_SIZE       (48)
#define SECONDS_UTC_TO_JST (32400)


#endif
