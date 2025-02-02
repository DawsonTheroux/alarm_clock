#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#include "common_i2c_configs.h"
#include "chipcomms_i2c_host.h"

void setup_i2c0_bus()
{
  i2c_init(i2c_default, 400 * 1000); // 100kHz
  gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);

#if defined(USE_INTERNAL_PULLUP_I2C0)
  printf("Using internal pullup for I2C0\n\r");
  gpio_pull_up(I2C0_SDA);
  gpio_pull_up(I2C0_SCL);
#endif
}

void cc_i2c_tx_task(void* args)
{
  /* !!I2C uses 1 byte for transmission length!! */

  cc_i2c_args_t* cc_i2c_args = (cc_i2c_args_t*)args;
  cc_i2c_transaction_t* cc_i2c_transaction;
  setup_i2c0_bus();

  /* Wait until there is a message to transmit in the queue, */
  /* then transmit it.                                       */
  for(;;){
    if(pdTRUE == xQueueReceive(*(cc_i2c_args->tx_queue), &cc_i2c_transaction, 100)){
      printf("I2C: Sending message of len: %d\r\n", cc_i2c_transaction->buffer_len);
      int i2c_result = i2c_write_timeout_us(i2c_default,
                                            ESP32_I2C_ADDR,
                                            cc_i2c_transaction->tx_buffer,
                                            cc_i2c_transaction->buffer_len,
                                            false, // Do not retain the line after transmit.
                                            1000);

      // If the transmission fails, try and add it back to the queue.
      // If adding it to the queue failes, just throw it away.
      if(i2c_result == PICO_ERROR_GENERIC){
        printf("NO ACK ON I2C send of data with command: 0x%02X\r\n"
               , cc_i2c_transaction->tx_buffer[0]);
        if(xQueueSendToFront(*(cc_i2c_args->tx_queue), &cc_i2c_transaction, 10) == pdPASS){
          // Skip the free because the i2c transaction is still in the queue.
          continue;
        }
      }
      // Free both the buffer and the transaction.
      free(cc_i2c_transaction->tx_buffer);
      free(cc_i2c_transaction);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
