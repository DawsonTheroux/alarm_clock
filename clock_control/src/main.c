#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "common_inc/common_i2c_configs.h"
#include "main.h"

#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"
#include "display.h"
#include "time_keeper.h"

void setup_gpio()
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  init_display_gpio();
  display_buffer = malloc(image_size);
  for(int i=0; i<image_size; i++){
    display_buffer[i] = 0xFF;
  }
  // display_basic_draw();
} 

int main() 
{
  /* I don't want to dynamically allocate all the queues,     */
  /* so instead I am putting them all in the idle task stack. */
  stdio_init_all();
  printf("In main, getting ready to draw\r\n");

  /*
  TaskHandle_t display_handle;
	xTaskCreate(display_task,
             "display task",
             configMINIMAL_STACK_SIZE, 
             NULL,
             8, 
             &display_handle);
             */
  
  setup_gpio();
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
	xTaskCreate(cc_spi_rx_task, 
             "chipcomms spi rx task", 
             configMINIMAL_STACK_SIZE, 
             (void*)&cc_spi_args, 
             CC_SPI_PRIORITY, 
             &cc_spi_rx_handle);

  /* Setup time keeper task */
  TaskHandle_t time_keeper_handle;
  xTaskCreate(time_keeper_task,
              "time keeper task",
              4096 * 4,
              (void*)&time_keeper_args,
              TIME_KEEPER_PRIORITY,
              &time_keeper_handle);

    
  // Start the scheduler.
  vTaskStartScheduler();

  for( ;; )
  {

  }
}
