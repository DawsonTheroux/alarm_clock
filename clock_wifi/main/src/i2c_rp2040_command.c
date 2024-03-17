#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "driver/i2c_slave.h"
#include "common_inc/common_configs.h"
#include "i2c_rp2040_command.h"


/* TODO:
*     - Clean up this file so it is more readable.
*     - Commands should be a set length, or change length depending
*       on the command.
*         ex. Get weather can be 0x10
*             but set wifi would be 0x11 + data for wifi.
*     - Once the SPI is setup, start configuring commands.
*     - Once the commands are configured, setup a queues between
*       this task and the command handler task to handle commands.
*     - THe first command that can be setup would be get current time.
*     - Commands will need to be in a common config file in 
*       the common folder.
*/

static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    if(sizeof(edata->buffer) > 0){
      xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    }
    return high_task_wakeup == pdTRUE;
}

void i2c_task(void* args)
{
  // Allocated the data read buffer.
  uint8_t *data_rd = (uint8_t *) malloc(DATA_LENGTH);
  for(int i=0; i<DATA_LENGTH; i++){
    data_rd[i] = 0;
  }
  uint32_t size_rd = 0;
  
  // I2C Slave config.
  i2c_slave_config_t i2c_slv_config = {
      .addr_bit_len = I2C_ADDR_BIT_LEN_7,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .send_buf_depth = 256,
      .scl_io_num = I2C0_SCL ,
      .sda_io_num = I2C0_SDA,
      .slave_addr = ESP32_I2C_ADDR,
  };
  
  // Create a new slave device.
  i2c_slave_dev_handle_t slave_handle;
  ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &slave_handle));
  
  // Create a queue to receive the data.
  QueueHandle_t s_receive_queue = xQueueCreate(1, sizeof(i2c_slave_rx_done_event_data_t));
  i2c_slave_event_callbacks_t cbs = {
      .on_recv_done = i2c_slave_rx_done_callback,
  };
  ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(slave_handle, &cbs, s_receive_queue));
  
  i2c_slave_rx_done_event_data_t rx_data;
  ESP_ERROR_CHECK(i2c_slave_receive(slave_handle, data_rd, DATA_LENGTH));
  for(;;){
    // Block indefinitely until an I2C Message is received
    if(pdTRUE == xQueueReceive(s_receive_queue, &rx_data, pdMS_TO_TICKS(10))){
      printf("Buffer length: %d", sizeof(rx_data.buffer));
      printf("rx_data.buffer:");
      for(int i=0; i<DATA_LENGTH; i++){
        printf(" 0x%02X", rx_data.buffer[i]);
        // printf(" %b", data_rd[i]);
      }

      printf("\n");
      printf("data_rd:");
      for(int i=0; i<DATA_LENGTH; i++){
        printf(" 0x%02X", data_rd[i]);
        // printf(" %b", data_rd[i]);
      }
      printf("\n");
      ESP_ERROR_CHECK(i2c_slave_receive(slave_handle, data_rd, DATA_LENGTH));
    }else{
      printf("didn't get data Printf\n");
    }
    vTaskDelay(50);
  }
}
//---------------------------------------------------------
