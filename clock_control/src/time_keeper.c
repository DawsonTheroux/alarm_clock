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
  uint8_t previous_minute = -1;
  datetime_t current_time;
  char datetime_buf[256];
  char* datetime_str = &datetime_buf[0];
  cc_spi_transaction_t* cc_spi_transaction;
  bool refresh_display = true;
  bool full_update = true;

  for(;;){
    /* Delay for 1s */
    if(xQueueReceive(*(time_keeper_args->spi_rx_queue), &cc_spi_transaction, 100) == pdTRUE){
      switch(cc_spi_transaction->data[0]){
        case SPI_CMD_TIMESYNC:
          refresh_display = true;
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
    vTaskDelay(50 / portTICK_PERIOD_MS);
    rtc_get_datetime(&current_time);
    if(previous_minute != current_time.sec || refresh_display){
      previous_minute = current_time.sec;
      datetime_to_str(datetime_str, sizeof(datetime_buf), &current_time);
      printf("Updating display time to %s\r\n", datetime_str);
      update_display_time(current_time, full_update);
      if(full_update){
        full_update = false;
      }
      refresh_display = false;
    }
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

