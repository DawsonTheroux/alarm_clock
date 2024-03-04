#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "main.h"

#include "alarm_clock.h"



int main() 
{
    stdio_init_all();


    TaskHandle_t clock_control_task_handle = NULL;

    // Initialize the built in LED GPIO.
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uint32_t status_clock_control_task = xTaskCreate(
                    clock_control_task,
                    "Matrix Task",
                    4096,
                    NULL,
                    10,
                    &clock_control_task_handle);

    vTaskStartScheduler();

    for( ;; )
    {

    }
}
