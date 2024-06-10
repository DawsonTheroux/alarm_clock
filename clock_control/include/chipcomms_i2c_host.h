#ifndef CHIPCOMMS_I2C_HOST_H
#define CHIPCOMMS_I2C_HOST_H
// LED Pin on pico dev board.
#include "FreeRTOS.h"
#include "queue.h"

#define I2C0_SDA PICO_DEFAULT_I2C_SDA_PIN
#define I2C0_SCL PICO_DEFAULT_I2C_SCL_PIN

typedef struct cc_i2c_transaction_t{
  uint8_t* tx_buffer;
  size_t buffer_len;
}cc_i2c_transaction_t;

typedef struct cc_i2c_args_t{
  QueueHandle_t* tx_queue;
} cc_i2c_args_t;

/** setup_i2c0_bus
 * Initializes I2C0 for Chip communications.
 * RP2040 acts as main
 */
void setup_i2c0_bus();

/** cc_i2c_tx_task
 * Task that handles all I2C communication outgoing from
 * the RP2040 into the ESP32.
 *
 * args is expected to be a cc_i2c_args_t object.
 */
void cc_i2c_tx_task(void* args);

#endif
