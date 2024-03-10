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
}
