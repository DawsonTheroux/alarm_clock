#ifndef CHIPCOMMS_I2C_HOST_H
#define CHIPCOMMS_I2C_HOST_H
// LED Pin on pico dev board.

#define I2C0_SDA PICO_DEFAULT_I2C_SDA_PIN
#define I2C0_SCL PICO_DEFAULT_I2C_SCL_PIN

void setup_i2c0_bus();
void dummy_i2c_transmit_task(void* args);

#endif
