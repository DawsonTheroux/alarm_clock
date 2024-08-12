#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_task_wdt.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "driver/spi_master.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "board_led.h"
#include "wifi_setup.h"
#include "chipcomms_spi_host.h"
#include "common_inc/common_spi_configs.h"
#include "wifi_secrets.h"


static const char *TAG = "clock_wifi";
static int s_wifi_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
QueueHandle_t *wifi_to_spi_queue;

/* event_handler
 *
 * Wifi event handler funciton that will attempt reconnect 5 times.
 * This function will set LED status based on based on the result
 * of the wifi event
 */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    set_led_status(PENDING);
    ESP_LOGI(TAG, "FIRST esp_wifi_connect()");
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_wifi_retry_num < WIFI_MAXIMUM_RETY) {
      ESP_LOGI(TAG, "Second esp_wifi_connect()");
      esp_wifi_connect();
      s_wifi_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      set_led_status(FAIL);
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG,"connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_wifi_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void sntp_sync_callback(struct timeval *synced_time)
{
  // Convert the sync time to local time.
  printf("received SNTP sync callback...sending command to SPI (RP2040)\n");
  time_t now;
  struct tm local_time;
  // Allocate the SPI transaction.
  spi_transaction_t* spi_time_tx = (spi_transaction_t*)malloc(sizeof(spi_transaction_t));
  uint8_t* spi_tx_buffer = (uint8_t*)malloc(sizeof(uint8_t) * SPI_TIMESYNC_LEN + 2);
  // Get the latest time.
  time(&now);
  localtime_r(&now, &local_time);
  // Get the sync time in local time.
  uint16_t current_year = local_time.tm_year + 1900;
  uint8_t buff_iter = 0;
  // Set the data in the SPI transaction.
  spi_tx_buffer[buff_iter++] = (SPI_TIMESYNC_LEN & 0xFF00) >> 8;
  spi_tx_buffer[buff_iter++] = (SPI_TIMESYNC_LEN & 0xFF);
  printf("PSEUDO MESSAGE LEN: (1: %d), (2: %d), (total:%d)\n", spi_tx_buffer[0], spi_tx_buffer[1], (spi_tx_buffer[0] <<8) | spi_tx_buffer[1]);
  spi_tx_buffer[buff_iter++] = SPI_CMD_TIMESYNC;
  // Set the year.
  printf("Found current year: %d\n", current_year);
  spi_tx_buffer[buff_iter++] = (current_year & 0xFF00) >> 8;
  spi_tx_buffer[buff_iter++] = (current_year & 0xFF);
  // Month is indexed at 1 in RP2040, but zero here.
  spi_tx_buffer[buff_iter++] = local_time.tm_mon + 1;
  spi_tx_buffer[buff_iter++] = local_time.tm_mday;
  // spi_tx_buffer[buff_iter++] = local_time.tm_wday;
  spi_tx_buffer[buff_iter++] = local_time.tm_hour;
  spi_tx_buffer[buff_iter++] = local_time.tm_min;
  spi_tx_buffer[buff_iter++] = local_time.tm_sec;

  spi_time_tx->tx_buffer = spi_tx_buffer;
  spi_time_tx->length = (SPI_TIMESYNC_LEN + 2) * 8;
  spi_time_tx->rx_buffer = NULL;
  spi_time_tx->rxlength = 0;
  spi_time_tx->flags = 0;

  xQueueSend(*wifi_to_spi_queue, &spi_time_tx, pdMS_TO_TICKS(100));
}

/* Initialize Wi-Fi as sta and set scan method */
/* Returns 0 if the AP is found, returns -1 if it is not found */
int8_t wifi_scan(void)
{
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_scan_start(NULL, true);
  ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
  for (int i = 0; i < number; i++) {
    if(strcmp((char*)ap_info[i].ssid, WIFI_SSID) == 0){
      ESP_LOGI(TAG, "REFOUND WIFI %s", WIFI_SSID);
      return 0;
    }
  }
  ESP_LOGI(TAG, "Wifi with %s not found...", WIFI_SSID);
  return -1;
}

// TODO: Add logic for on wifi disconnect to restart SNTP
void wifi_init_sta(void *args)
{
  wifi_args_t* wifi_args = (wifi_args_t*)args;
  wifi_to_spi_queue = wifi_args->spi_tx_queue;

  s_wifi_event_group = xEventGroupCreate();
  setup_wifi();

  setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();
  bool sntp_started = false;
  // Set RTC timezone
  for(;;){
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if(!sntp_started && bits & WIFI_CONNECTED_BIT){
      start_sntp();
      if(esp_sntp_enabled()){
        set_led_status(SUCCESS);
        printf("!!!SNTP snabled!!!\n");
        sntp_started = true;
      }
    }
    /* uncomment this if the WIFI connect should happen indefinitely. */
    // if(bits & WIFI_FAIL_BIT && wifi_scan() == 0)
    // {
    //   ESP_LOGI(TAG, "Found AP...attempting connect");
    //   xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT | WIFI_CONNECTED_BIT);
    //   esp_wifi_connect();
    // }else if (!sntp_started && bits & WIFI_CONNECTED_BIT){
    //   start_sntp();
    //   if(esp_sntp_enabled()){
    //     printf("!!!SNTP snabled!!!\n");
    //     sntp_started = true;
    //   }
    // }

    // time_t now;
    // struct tm timeinfo;
    // char strtime_buf [64];
    // time(&now);
    // localtime_r(&now, &timeinfo);
    // strftime(strtime_buf, sizeof(strtime_buf), "%c", &timeinfo);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void start_sntp()
{
  printf("SNTP: Starting SNTP because connected\n");
  esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.google.com");

  esp_netif_sntp_init(&sntp_config);
  sntp_set_time_sync_notification_cb(sntp_sync_callback);
}

void setup_wifi()
{
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
      .threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD,
      .sae_pwe_h2e = WIFI_SAE_MODE,
      .sae_h2e_identifier = WIFI_H2E_IDENTIFIER,
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  // printf("ABOUT TO WAIT FOR BITS IN SETUP\n");
  // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
  //         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
  //         pdFALSE,
  //         pdFALSE,
  //         portMAX_DELAY);
  //
  // /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
  //  * happened. */
  // if (bits & WIFI_CONNECTED_BIT) {
  //     ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
  //              WIFI_SSID, WIFI_PASSWORD);
  // } else if (bits & WIFI_FAIL_BIT) {
  //     ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
  //              WIFI_SSID, WIFI_PASSWORD);
  // } else {
  //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
  // }
}


static void http_get_request(char* request, char* web_server, char* port)
{
  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res;
  struct in_addr *addr;
  int s, r;
  char recv_buf[64];

  int err = getaddrinfo(web_server, port, &hints, &res);

  if(err != 0 || res == NULL) {
    ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return;
  }
  /* Code to print the resolved IP.
     Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
  addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
  ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

  s = socket(res->ai_family, res->ai_socktype, 0);
  if(s < 0) {
    ESP_LOGE(TAG, "... Failed to allocate socket.");
    freeaddrinfo(res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return;
  }
  ESP_LOGI(TAG, "... allocated socket");

  if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
    close(s);
    freeaddrinfo(res);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return;
  }

  ESP_LOGI(TAG, "... connected");
  freeaddrinfo(res);

  if (write(s, request, strlen(request)) < 0) {
    ESP_LOGE(TAG, "... socket send failed");
    close(s);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return;
  }
  ESP_LOGI(TAG, "... socket send success");

  struct timeval receiving_timeout;
  receiving_timeout.tv_sec = 5;
  receiving_timeout.tv_usec = 0;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, sizeof(receiving_timeout)) < 0){
    ESP_LOGE(TAG, "... failed to set socket receiving timeout");
    close(s);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return;
  }
  ESP_LOGI(TAG, "... set socket receiving timeout success");

  /* Read HTTP response */
  do {
    bzero(recv_buf, sizeof(recv_buf));
    r = read(s, recv_buf, sizeof(recv_buf)-1);
    for(int i = 0; i < r; i++) {
      putchar(recv_buf[i]);
    }
  } while(r > 0);

  ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
  close(s);
}


void get_current_time(void *pvParameters)
{
  static char* request = "GET /api/ip HTTP/1.1\r\nHost: worldtimeapi.org\r\nUser-Agent: esp-idf/1.0 esp32\r\n\r\n";
  // static const char HOWSMYSSL_REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n" "Host: "WEB_SERVER"\r\n" "User-Agent: esp-idf/1.0 esp32\r\n" "\r\n";
  char* web_server = "worldtimeapi.org";
  char* port = "80";
  
  http_get_request(request, web_server, port);
}

