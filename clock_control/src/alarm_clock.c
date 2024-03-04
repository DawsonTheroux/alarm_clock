#include <stdio.h>
#include <stdlib.h>

#include "string.h"
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/regs/sio.h"
#include "hardware/structs/sio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "main.h"
#include "alarm_clock.h"


void clock_control_task(void* args){
  for(;;){
    gpio_put(LED_PIN, 1);
    vTaskDelay(500);
    gpio_put(LED_PIN, 0);
    vTaskDelay(500);
  }

}
