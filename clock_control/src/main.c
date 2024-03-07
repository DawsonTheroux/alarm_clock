#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#include "alarm_clock.h"



int main() 
{
    stdio_init_all();


    // TaskHandle_t clock_control_task_handle = NULL;

    // Initialize the built in LED GPIO.
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    TaskHandle_t xled_handle1;
	
	  xTaskCreate(clock_control_task, "LED", configMINIMAL_STACK_SIZE, (void*)&delay_time1, 10, &(xled_handle1) );
    vTaskCoreAffinitySet(xled_handle1, (1 << 0));

	  vTaskStartScheduler();

    for( ;; )
    {

    }
}
