#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "driver/i2c_slave.h"
#include "common_inc/common_i2c_configs.h"
#include "chipcomms_i2c_device.h"


uint8_t cc_i2c_rx_buffer[CC_I2C_DEVICE_RX_BUFFER_SIZE];

static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
  BaseType_t high_task_wakeup = pdFALSE;
  QueueHandle_t* receive_queue = (QueueHandle_t*) user_data;
  cc_i2c_rx_transaction_t* rx_transaction = (cc_i2c_rx_transaction_t*)malloc(sizeof(cc_i2c_rx_transaction_t));
  rx_transaction->rx_len = edata->buffer[0];
  rx_transaction->rx_buffer = (uint8_t*)malloc(rx_transaction->rx_len);
  memcpy(rx_transaction->rx_buffer, edata->buffer + 1, rx_transaction->rx_len);

  xQueueSendFromISR(*receive_queue, &rx_transaction, &high_task_wakeup);
  return high_task_wakeup == pdTRUE;
}

void setup_cc_i2c_device(i2c_slave_dev_handle_t* cc_i2c_rx_handle, QueueHandle_t* s_receive_queue)
{
  // Configure the I2C device
  i2c_slave_config_t i2c_cc_config = {
      .addr_bit_len = I2C_ADDR_BIT_LEN_7,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .send_buf_depth = 256,
      .scl_io_num = I2C0_SCL ,
      .sda_io_num = I2C0_SDA,
      .slave_addr = ESP32_I2C_ADDR,
  };
  // Create a new slave device.
  ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_cc_config, cc_i2c_rx_handle));
  // Create a queue to receive the data.
  // QueueHandle_t s_receive_queue = xQueueCreate(16, sizeof(cc_i2c_rx_transaction_t*));
  i2c_slave_event_callbacks_t cbs = {
      .on_recv_done = i2c_slave_rx_done_callback,
  };
  ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(*cc_i2c_rx_handle, &cbs, s_receive_queue));
  // Do the first call to the I2C receive.
  ESP_ERROR_CHECK(i2c_slave_receive(*cc_i2c_rx_handle, cc_i2c_rx_buffer, CC_I2C_DEVICE_RX_BUFFER_SIZE));
}

void cc_i2c_rx_task(void* args)
{
  i2c_slave_dev_handle_t cc_i2c_rx_handle;
  QueueHandle_t s_receive_queue = xQueueCreate(1, sizeof(cc_i2c_rx_transaction_t));
  setup_cc_i2c_device(&cc_i2c_rx_handle, &s_receive_queue);
  cc_i2c_rx_transaction_t* cc_i2c_rx_transaction;
  for(;;){
    // Block indefinitely until an I2C Message is received
    if(pdTRUE == xQueueReceive(s_receive_queue, &cc_i2c_rx_transaction, pdMS_TO_TICKS(10))){
      // printf("I2C: Received trans with len: %d\n", cc_i2c_rx_transaction->rx_len);
      free(cc_i2c_rx_transaction->rx_buffer);
      free(cc_i2c_rx_transaction);
      ESP_ERROR_CHECK(i2c_slave_receive(cc_i2c_rx_handle, cc_i2c_rx_buffer, CC_I2C_DEVICE_RX_BUFFER_SIZE));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
