#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
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
          
    configure_led();
    configure_nvs();
    // wifi_init_sta();

    TaskHandle_t wifi_task_handle;
	  xTaskCreate(wifi_init_sta, "wifi_setup", 4096,NULL, 10, &(wifi_task_handle) );
    ESP_LOGI(TAG, "After WIFI Setup");
    for(;;){
        vTaskDelay(500);
        ESP_LOGI(TAG, "Probing for time");
        get_current_time(NULL);
    }
}


