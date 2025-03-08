#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
//#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"
#include "driver/sdspi_host.h"

#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "board_led.h"
#include "wifi_setup.h"
#include "chipcomms_i2c_device.h"
#include "chipcomms_spi_host.h"
#include "flash.h"

#include "driver/ledc.h"

#include "main.h"

static const char *TAG = "main";

void configure_nvs()
{
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
}

void control_buzzer(void *args)
{
  /*
  ledc_timer_config_t led_timer_config;
  led_timer_config.speed_mode = LEDC_LOW_SPEED_MODE;
  led_timer_config.duty_resolution = LEDC_TIMER_8_BIT;
  led_timer_config.timer_num = LEDC_TIMER_MAX - 1;
  led_timer_config.freq_hz = 2000;
  led_timer_config.clk_cfg = LEDC_REF_TICK;
  ledc_timer_config(&led_timer_config);

  ledc_channel_config_t led_channel_config;
  led_channel_config.gpio_num = 22;
  led_channel_config.speed_mode = LEDC_LOW_SPEED_MODE;
  led_channel_config.channel = 0;
  led_channel_config.timer_sel = LEDC_TIMER_MAX - 1;
  led_channel_config.duty = 0x7F;
  led_channel_config.hpoint = 0x7F;
  ledc_channel_config(&led_channel_config);
  */
  
  gpio_reset_pin(22);
  gpio_set_direction(22, GPIO_MODE_OUTPUT);

  int value = 1;
  for(;;){
    value = (value == 0) ? 1 : 0;
    gpio_set_level(22, value);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void app_main(void)
{
  configure_nvs();
  /* I don't want to dynamically allocate all the queues,     */
  /* so instead I am putting them all in the idle task stack. */
  cc_spi_args_t cc_spi_args;
  wifi_args_t wifi_args;

  QueueHandle_t spi_tx_queue = xQueueCreate(16, sizeof(spi_transaction_t*));
  cc_spi_args.tx_queue = &spi_tx_queue;
  wifi_args.spi_tx_queue = &spi_tx_queue;

  // sdspi_dev_handle_t sd_handle;
  // if(initialize_sd(&sd_handle)){
  //   printf("Failed to initialize the SD card\n");
  // }

  TaskHandle_t spi_task_handle;
  //xTaskCreatePinnedToCore(cc_spi_tx_task, "SPI tx task", 4096, &cc_spi_args, 3, &(spi_task_handle), 0);
  xTaskCreate(cc_spi_tx_task, "SPI tx task", 4096, &cc_spi_args, 3, &spi_task_handle);

  TaskHandle_t i2c_task_handle;
  // xTaskCreatePinnedToCore(cc_i2c_rx_task, "I2C rx task", 4096, NULL, 3, &(i2c_task_handle), 0);
  xTaskCreate(cc_i2c_rx_task, "I2C rx task", 4096, NULL, 3, &i2c_task_handle);

  // Wifi task to setup and connect to wifi. 
  TaskHandle_t wifi_task_handle;
  // xTaskCreatePinnedToCore(wifi_init_sta, "wifi setup + rtc", 5096, &wifi_args, 9, &(wifi_task_handle), 1);
  xTaskCreate(wifi_init_sta, "wifi setup + rtc", 5096, &wifi_args, 9, &wifi_task_handle);

  TaskHandle_t buzzer_task;
  // xTaskCreatePinnedToCore(wifi_init_sta, "wifi setup + rtc", 5096, &wifi_args, 9, &(wifi_task_handle), 1);
  // xTaskCreate(control_buzzer, "buzzer", 1024, NULL, 5, &buzzer_task);
  
  for(;;){
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}


