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
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "board_led.h"
#include "wifi_setup.h"
#include "chipcomms_i2c_device.h"
#include "chipcomms_spi_host.h"
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

  TaskHandle_t spi_task_handle;
  //xTaskCreatePinnedToCore(cc_spi_tx_task, "SPI tx task", 4096, &cc_spi_args, 3, &(spi_task_handle), 0);
  xTaskCreate(cc_spi_tx_task, "SPI tx task", 4096, &cc_spi_args, 3, &(spi_task_handle));

  TaskHandle_t i2c_task_handle;
  // xTaskCreatePinnedToCore(cc_i2c_rx_task, "I2C rx task", 4096, NULL, 3, &(i2c_task_handle), 0);
  xTaskCreate(cc_i2c_rx_task, "I2C rx task", 4096, NULL, 3, &(i2c_task_handle));

  // Wifi task to setup and connect to wifi. 
  TaskHandle_t wifi_task_handle;
  // xTaskCreatePinnedToCore(wifi_init_sta, "wifi setup + rtc", 5096, &wifi_args, 9, &(wifi_task_handle), 1);
  xTaskCreate(wifi_init_sta, "wifi setup + rtc", 5096, &wifi_args, 9, &(wifi_task_handle));
  
  for(;;){
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}


