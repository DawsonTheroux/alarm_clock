#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "board_led.h"

static const char *TAG = "led";

/*
 * Sets the GPIO pin direction for the on board
 * LED based on the values in the header file.
 */
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

/*
 * Sets the state of the LED on the dev board based
 * led status.
 *
 * SUCCESS: green
 * PENDING: yellow
 * FAIL:    red
 * defualt: off
 */
void set_led_status(led_state new_status)
{
  switch (new_status)
  {
    case SUCCESS: // Success (green)
      gpio_set_level(BOARD_LED_R, 1);
      gpio_set_level(BOARD_LED_G, 0);
      gpio_set_level(BOARD_LED_B, 1);
      break;
    case FAIL:    // Failure (red)
      gpio_set_level(BOARD_LED_R, 0);
      gpio_set_level(BOARD_LED_G, 1);
      gpio_set_level(BOARD_LED_B, 1);
      break;
    case PENDING: // Pending (yellow)
      gpio_set_level(BOARD_LED_R, 0);
      gpio_set_level(BOARD_LED_G, 0);
      gpio_set_level(BOARD_LED_B, 1);
      break;
    default:      // default (off)
      gpio_set_level(BOARD_LED_R, 1);
      gpio_set_level(BOARD_LED_G, 1);
      gpio_set_level(BOARD_LED_B, 1);
      break;
  }
}
