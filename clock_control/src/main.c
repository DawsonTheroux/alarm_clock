#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "common_i2c_configs.h"
#include "main.h"

#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"
#include "display.h"
#include "time_keeper.h"
#include "flash.h"

void setup_gpio()
{
  // Set the LED GPIO.
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // Initialize the SPI line for Display and flash.
  // The SPI line is initialized at a slow speed until the SPI flash
  // is enabled. Once that is complete, the speed is increased to
  // SPI_FREQ_HZ
  spi_init(spi1, 150 * 1000);
  gpio_set_function(SPI_CLK, GPIO_FUNC_SPI);
  gpio_set_function(SPI_DOUT, GPIO_FUNC_SPI);
  gpio_set_function(SPI_DIN, GPIO_FUNC_SPI);
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  // Initialize the GPIO for all other components.
  init_display_gpio();
  init_flash_gpio();
  // init_flash();

  // Allocate the display buffer
  display_buffer = malloc(display_buffer_size);
  if(display_buffer == NULL){
    printf("Failed to allocate display_buffer\r\n");
    return;
  }
  memset(display_buffer, 0xFF, display_buffer_size);

} 

int main() 
{
  /* I don't want to dynamically allocate all the queues,     */
  /* so instead I am putting them all in the idle task stack. */
  stdio_init_all();
  
  setup_gpio();
  printf("After setup_gpio\r\n");
  // setup_tasks();
  cc_i2c_args_t cc_i2c_args;
  cc_spi_args_t cc_spi_args;
  time_keeper_args_t time_keeper_args;

  /* Setup all the queues */
  QueueHandle_t cc_i2c_tx_queue = xQueueCreate(16, sizeof(cc_i2c_transaction_t)); 
  QueueHandle_t time_keeper_queue = xQueueCreate(16, sizeof(cc_spi_transaction_t));
  cc_i2c_args.tx_queue = &cc_i2c_tx_queue;
  cc_spi_args.time_keeper_queue = &time_keeper_queue;
  time_keeper_args.i2c_tx_queue = &cc_i2c_tx_queue;
  time_keeper_args.spi_rx_queue = &time_keeper_queue;

  /* Setup I2C task */
  TaskHandle_t cc_i2c_tx_handle;
        xTaskCreate(cc_i2c_tx_task,
             "chipcomms i2c tx task",
             configMINIMAL_STACK_SIZE, 
             (void*)&cc_i2c_args, 
             CC_I2C_PRIORITY, 
             &cc_i2c_tx_handle);

  /* Setup SPI task */
  TaskHandle_t cc_spi_rx_handle;
        int returned = xTaskCreate(cc_spi_rx_task, 
             "chipcomms spi rx task", 
             configMINIMAL_STACK_SIZE, 
             (void*)&cc_spi_args, 
             CC_SPI_PRIORITY, 
             &cc_spi_rx_handle);

  /* Setup time keeper task */
  TaskHandle_t time_keeper_handle;
  int returned2 = xTaskCreate(time_keeper_task,
              "time keeper task",
              5000 * 4,
              (void*)&time_keeper_args,
              TIME_KEEPER_PRIORITY,
              &time_keeper_handle);

  // Start the scheduler.
  vTaskStartScheduler();

  for( ;; )
  {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
