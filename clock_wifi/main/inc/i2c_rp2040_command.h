#ifndef I2C_RP2040_COMMAND
#define I2C_RP2040_COMMAND

#define I2C0_SCL 25
#define I2C0_SDA 26
#define DATA_LENGTH 10

void i2c_task(void* args);

#endif
