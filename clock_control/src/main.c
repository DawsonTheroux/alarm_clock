#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "common_inc/common_configs.h"
#include "main.h"

#include "alarm_clock.h"
#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"


void setup_gpio()
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
}

int setup_peripherals()
{
  // NOTE: We get UART for free.
  setup_gpio();
  setup_i2c0_bus();
  /* Functions to setup
  setup_spi0_host();
  setup_spi1_device();
  setup_pwm();
  */
}

int main() 
{
    stdio_init_all();
    // Initialize the built in LED GPIO.
    setup_peripherals();

    /* I2C Dummy task*/
    
   //  TaskHandle_t i2c_dummy_task;
	  // xTaskCreate(dummy_i2c_transmit_task, "i2c0 task", configMINIMAL_STACK_SIZE, NULL, 10, &(i2c_dummy_task));

    /* I2C Dummy task*/
    TaskHandle_t spi_dummy_task;
	  xTaskCreate(spi_read_task, "spi task", configMINIMAL_STACK_SIZE, NULL, 10, &(spi_dummy_task));

    /* Alarm clock control code */
    //  TaskHandle_t clock_control_handle;
	  // xTaskCreate(clock_control_task, "clock_task", configMINIMAL_STACK_SIZE, NULL, 10, &(clock_control_handle));

	  vTaskStartScheduler();

    for( ;; )
    {

    }
}
