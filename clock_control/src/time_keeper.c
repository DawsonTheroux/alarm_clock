#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/util/datetime.h" // Only needed for datetime_to_str();
#include "hardware/rtc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "common_inc/common_spi_configs.h"
#include "chipcomms_i2c_host.h"
#include "chipcomms_spi_device.h"
#include "display.h"
#include "time_keeper.h"
#include "flash.h"


void temp_func()
{
  uint8_t read_buffer[514];
  uint8_t write_buffer[512];
  memset(write_buffer, 0xFA, 512);
  uint8_t read_token;
  memset(read_buffer, 0, 514);

  while(init_sd_spi_mode()){
      printf("SD card SPI mode init FAILED...Trying again.\r\n");
  }

  printf("SD CARD INITIALIZED\r\n");
  if(flash_read_block(0x0, read_buffer, &read_token)){
    printf("SD READ FAILED\r\n");
  }
  if(read_token == 0xFE){
    printf("READ SUCCESSFUL with buffer:\r\n");
    for(int i=0; i<512; i++){
      if(i!=0 && i %16 == 0){
        printf("\r\n");
      }
      printf("%02X ", read_buffer[i]);
    }
    printf("\r\n");
    if(flash_write_block(0x0, write_buffer, &read_token)){
      printf("SD WRITE FAILED\r\n");
      goto loop;
    }
    printf("Got token from write: %02X\r\n", read_token);
    if(flash_read_block(0x0, read_buffer, &read_token)){
      printf("Second SD READ FAILED\r\n");
      goto loop;
    }
    for(int i=0; i<512; i++){
      if(i!=0 && i %16 == 0){
        printf("\r\n");
      }
      printf("%02X ", read_buffer[i]);
    }
    printf("\r\n");
  }else{
    printf("Read token note 0xFE: %02X\r\n", read_token);
  }


loop:
  for(;;){
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void time_keeper_task(void* args)
{
  time_keeper_args_t* time_keeper_args = (time_keeper_args_t*)args;
  setup_rtc();
  printf("Hello time keeper task\r\n");
  // Cast the args to 
  uint8_t previous_minute = -1;
  datetime_t current_time;
  char datetime_buf[256];
  char* datetime_str = &datetime_buf[0];
  cc_spi_transaction_t* cc_spi_transaction;
  bool refresh_display = true;
  bool full_update = true;

  temp_func();



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
      if(current_time.sec == 0){
        full_update = true;
      }
      previous_minute = current_time.sec;
      datetime_to_str(datetime_str, sizeof(datetime_buf), &current_time);
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

