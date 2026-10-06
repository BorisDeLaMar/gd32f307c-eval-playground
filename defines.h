#define TX_RX_BUF_SIZE 256

#define EEPROM_ADDRESS 0xA0

#define MAIN_FLASH_START_ADDR 0x08000000
#define MAIN_FLASH_END_ADDR 0x082FFFFF

#define I2C_EVENT_SBSEND  BIT(0) // start bit sent
#define I2C_EVENT_ADDSEND BIT(1) // address sent
#define I2C_EVENT_TBE     BIT(2) // is set when data register of I2C_Data is empty
#define I2C_EVENT_RBNE    BIT(3)
#define I2C_EVENT_BTC     BIT(4) // byte transmission complete