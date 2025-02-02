#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/util/datetime.h" // Only needed for datetime_to_str();
#include "hardware/rtc.h"
#include "hardware/spi.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common_spi_configs.h"
#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"
#include "display.h"
#include "display_manager.h"
#include "time_keeper.h"
#include "fs.h"
#include "flash.h"


/*
void temp_func()
{
  uint8_t read_buffer[4096];
  uint32_t read_length;
  memset(read_buffer, 0, 514);

  while(init_sd_spi_mode()){
      printf("SD card SPI mode init FAILED...Trying again.\r\n");
      vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  printf("SD CARD INITIALIZED\r\n");

  // if(read_file("/lrgfnt/0.CHR", read_buffer, 2048, &read_length)){
  if(read_file("/lrgfnt/;.CHR", read_buffer, 4096, &read_length)){
    printf("read_file failed.. read_length: %u\r\n", read_length);
  }
  printf("0 = %u, 1 = %u, 9 = %u\r\n", '0', '1', '9');

  uint8_t image_height = read_buffer[0];
  uint8_t image_width = read_buffer[1];
  uint8_t *image_data = read_buffer + 2;

  superimpose_image(0, 0, image_data, image_width, image_height);
  display_init_full();
  draw_full_display();



loop:
  for(;;){
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}
*/

void time_keeper_task(void* args)
{
  time_keeper_args_t* time_keeper_args = (time_keeper_args_t*)args;
  setup_rtc();
  printf("Hello time keeper task\r\n");
  // Cast the args to 
  uint8_t previous_minute = -1;
  uint8_t previous_day = -1;
  datetime_t current_time;
  char datetime_buf[256];
  char* datetime_str = &datetime_buf[0];
  cc_spi_transaction_t* cc_spi_transaction;
  bool refresh_display = true;
  bool full_update = true;

  while(init_sd_spi_mode()){
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  // Speed up SPI1 to full speed.
  spi_set_baudrate(spi1, SPI_FREQ_HZ);

  // temp_func();
  for(;;){
    if(xQueueReceive(*(time_keeper_args->spi_rx_queue), &cc_spi_transaction, 100) == pdTRUE){
      switch(cc_spi_transaction->data[0]){
        case SPI_CMD_TIMESYNC:
          refresh_display = true;
          full_update = true;
          update_time_from_spi(cc_spi_transaction->data + 1);
          free(cc_spi_transaction->data);
          free(cc_spi_transaction);
          // printf("!!TIME UPDATED!!\r\n");
          break;
        default:
          printf("ERROR: time_keeper received unknown command %d\r\n", cc_spi_transaction->data[0]);
          free(cc_spi_transaction->data);
          free(cc_spi_transaction);
          break;
      }
    }
    /* Print the current time. This will probably be able to be changed later to update */
    /* the display                                                                      */
    rtc_get_datetime(&current_time);
    if(previous_minute != current_time.min || refresh_display){
      if(current_time.min % 5 == 0 || current_time.min % 10 == 0){
        full_update = true;
      }
      if(refresh_display || previous_day != current_time.day){
        full_update = true;
        update_display_date(current_time);
      }
      update_display_time(current_time, !full_update);
      if(full_update){
        display_init_full();
        draw_full_display();
        display_sleep();
        full_update = false;
      }
      refresh_display = false;
      previous_minute = current_time.min;
      previous_day = current_time.day;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
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

/*
* Apparently calculates the day of the week.
* Source: https://rosettacode.org/wiki/Day_of_the_week#C
*/
static uint8_t calculate_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
  int adjustment, mm, yy;
  adjustment = (14 - month) / 12;
  mm = month + 12 * adjustment - 2;
  yy = year - adjustment;
  return (day+(13 * mm - 1) / 5 + yy + yy / 4 - yy / 100 + yy / 400) % 7;
}

void update_time_from_spi(uint8_t* spi_buffer_data){
  uint8_t count = 0;
  datetime_t new_time = {
    .year = (spi_buffer_data[count++] << 8) | (spi_buffer_data[count++]),
    .month = spi_buffer_data[count++],
    .day = spi_buffer_data[count++],
    .dotw = 0,
    .hour = spi_buffer_data[count++],
    .min = spi_buffer_data[count++],
    .sec = spi_buffer_data[count++],
  };
  new_time.dotw = calculate_day_of_week(new_time.year, new_time.month, new_time.day);
  rtc_set_datetime(&new_time);
}

