#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#define WIFI_MAXIMUM_RETY   5
#define WIFI_H2E_IDENTIFIER ""
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_SAE_MODE       WPA3_SAE_PWE_BOTH

#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

void wifi_init_sta(void);
void get_current_time(void *pvParameters);
// void http_get_request(char* request);


#endif 
