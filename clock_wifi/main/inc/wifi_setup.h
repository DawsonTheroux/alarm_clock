#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#define WIFI_MAXIMUM_RETY   10
#define WIFI_H2E_IDENTIFIER ""
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_SAE_MODE       WPA3_SAE_PWE_BOTH

/* The number APs to add to the list */
#define DEFAULT_SCAN_LIST_SIZE 32

#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define DEFAULT_SNTP_REFRESH_TIME_MS 12 * 60 * 60 * 1000 // 12 hours.

#include "freertos/queue.h"

typedef struct wifi_args_t{
  QueueHandle_t* spi_tx_queue;
}wifi_args_t;

void wifi_init_sta(void* args);
void get_current_time(void *pvParameters);
void setup_wifi();
void start_sntp();
// void http_get_request(char* request);
#endif 
