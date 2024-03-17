#ifndef I2C_0_SETUP_H
#define I2C_0_SETUP_H
// LED Pin on pico dev board.
#define LED_PIN 25

#define I2C0_SDA PICO_DEFAULT_I2C_SDA_PIN
#define I2C0_SCL PICO_DEFAULT_I2C_SCL_PIN

void setup_i2c0_bus();
void dummy_i2c_transmit_task(void* args);

#endif
