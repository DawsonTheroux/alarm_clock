#include <stdio.h>

#include "string.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
// #include "FreeRTOSConfig.h"
#include "task.h"
#include "main.h"
#include "alarm_clock.h"


void clock_control_task(void* args)
{
  for(;;){
    gpio_put(LED_PIN, 1);
    vTaskDelay(1000);
    gpio_put(LED_PIN, 0);
    vTaskDelay(1000);
  }
}
