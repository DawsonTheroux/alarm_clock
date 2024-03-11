#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "board_led.h"

static uint8_t s_led_state = 0;
static const char *TAG = "led";

void blink_led(void)
{
    gpio_set_level(BOARD_LED_R, s_led_state);
    gpio_set_level(BOARD_LED_G, s_led_state);
    gpio_set_level(BOARD_LED_B, s_led_state);
}

void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BOARD_LED_R);
    gpio_reset_pin(BOARD_LED_G);
    gpio_reset_pin(BOARD_LED_B);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BOARD_LED_R, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOARD_LED_G, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOARD_LED_B, GPIO_MODE_OUTPUT);
    set_led_status(-2);
}

void set_led_status(int status_number)
{
  /* 
   * Set the LED based on status 
   * Status:
   *  1 = Success
   *  0 = Pending
   *  -1 = Failure
  */
  switch(status_number)
  {
    case 1: // Success (green)
      gpio_set_level(BOARD_LED_R, 0);
      gpio_set_level(BOARD_LED_G, 1);
      gpio_set_level(BOARD_LED_B, 0);
      break;
    case -1: // Failure (red)
      gpio_set_level(BOARD_LED_R, 1);
      gpio_set_level(BOARD_LED_G, 0);
      gpio_set_level(BOARD_LED_B, 0);
      break;
    case 0: // Pending (yellow)
      gpio_set_level(BOARD_LED_R, 1);
      gpio_set_level(BOARD_LED_G, 1);
      gpio_set_level(BOARD_LED_B, 0);
      break;
    default: // default (off)
      gpio_set_level(BOARD_LED_R, 0);
      gpio_set_level(BOARD_LED_G, 0);
      gpio_set_level(BOARD_LED_B, 0);
      break;
  }
}
