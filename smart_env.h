#ifndef SMART_ENV_H
#define SMART_ENV_H

#define DEBUG

#define PIN_SDA          (4)
#define PIN_SCL          (5)
#define PIN_DS           (2) // pin 2 is hw pulled up

#define I2C_BME280_ADDRESS 0x76
#define I2C_CCS811_ADDRESS 0x5A // Default I2C Address

#define INTERVAL   100
#define HIST_MAX   300

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
#define EEPROM_ID_ADDR              (128)
#define EEPROM_ENABLE_ADDR          (129)
// +0 : temperature
// +1 : humidity
// +2 : pressure
// +3 : co2
#define EEPROM_LAST_ADDR           (133)


#endif
