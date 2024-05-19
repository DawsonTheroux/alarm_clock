#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/util/datetime.h" // Only needed for datetime_to_str();
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/rtc.h"
#include "common_inc/common_spi_configs.h"
#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"
#include "display.h"
#include "time_keeper.h"

void time_keeper_task(void* args)
{
  time_keeper_args_t* time_keeper_args = (time_keeper_args_t*)args;
  setup_rtc();
  printf("Hello time keeper task\r\n");
  // Cast the args to the params
  datetime_t current_time;
  char datetime_buf[256];
  char* datetime_str = &datetime_buf[0];
  cc_spi_transaction_t* cc_spi_transaction;

  for(;;){
    /* Delay for 1s */
    if(xQueueReceive(*(time_keeper_args->spi_rx_queue), &cc_spi_transaction, 100) == pdTRUE){
      printf("Received from queue!!\r\n");
      switch(cc_spi_transaction->data[0]){
        case SPI_CMD_TIMESYNC:
          printf("Updating RTC from SPI\r\n");
          update_time_from_spi(cc_spi_transaction->data + 1);
          free(cc_spi_transaction->data);
          free(cc_spi_transaction);
          printf("!!TIME UPDATED!!\r\n");
          break;
        default:
          printf("time_keeper received unknown command %d\r\n", cc_spi_transaction->data[0]);
          free(cc_spi_transaction->data);
          free(cc_spi_transaction);
          break;
      }
    }
    /* Print the current time. This will probably be able to be changed later to update */
    /* the display                                                                      */
    rtc_get_datetime(&current_time);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &current_time);
    printf("Current time is: %s\r\n", datetime_str);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup_rtc()
{
  datetime_t current_time = {
    .year = 1970,
    .month = 01,
    .day = 01,
    .dotw = 4, // Thursday
    .hour = 00,
    .min = 00,
    .sec = 00,
  };

  rtc_init();
  rtc_set_datetime(&current_time);
}

void update_time_from_spi(uint8_t* spi_buffer_data){
  uint8_t count = 0;
  datetime_t new_time = {
    .year = (spi_buffer_data[count++] << 8) | (spi_buffer_data[count++]),
    .month = spi_buffer_data[count++],
    .day = spi_buffer_data[count++],
    // .dotw = spi_buffer_data[count++], 
    .hour = spi_buffer_data[count++],
    .min = spi_buffer_data[count++],
    .sec = spi_buffer_data[count++],
  };
  rtc_set_datetime(&new_time);
}

