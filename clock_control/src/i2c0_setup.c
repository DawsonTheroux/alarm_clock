#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#include "common_inc/common_configs.h"
#include "i2c0_setup.h"

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

void dummy_i2c_transmit_task(void* args)
{
  for(;;){
    // uart_puts(uart0, " Hello, UART!\n\r");
    printf("Sending I2C data\n\r");
    int i2c_result;
    char data_to_send[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    printf("Data to send:");
    for(int i=0; i<4; i++){
      printf("0x%02X", data_to_send[i]);
    }
    printf("\r\n");
    i2c_result = i2c_write_timeout_us(i2c_default, 
                                      ESP32_I2C_ADDR, 
                                      data_to_send, 
                                      4,
                                      false, 
                                      1000);

    if(i2c_result == PICO_ERROR_GENERIC){
      printf("No ACK on data\n\r");
    }else{
      printf("The number of bytes writted: %d\n\r", i2c_result);
    }
    vTaskDelay(1500);
  }
}
