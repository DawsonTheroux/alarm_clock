#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "chipcomms_spi_host.h"
#include "common_inc/common_spi_configs.h"

void spi_data_transfer_task(void* args)
{
  printf("Hello SPI task\n");
  esp_err_t ret;
  spi_device_handle_t chipcomm_spi_handle;
  spi_bus_config_t buscfg = {
      .miso_io_num = HSPI_PIN_MISO,
      .mosi_io_num = HSPI_PIN_MOSI,
      .sclk_io_num = HSPI_PIN_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 8,
  };
  printf("MISO: %d, MOSI: %d, sclk: %d", HSPI_PIN_MISO, HSPI_PIN_MOSI, HSPI_PIN_CLK);
  //Initialize the SPI bus
  ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);

  /*
  rp2040_config_t rp2040_config = {
    .cs_io = -1,
    .host = HSPI_HOST,
    .miso_io = HSPI_PIN_MISO,
    .mosi_io = HSPI_PIN_MOSI,
  }
    */
  spi_device_interface_config_t hspi_cfg = {
     // .command_bits = 8,
    .clock_speed_hz = CHIPCOMMS_SPI_FREQ_HZ,
    .mode = CHIPCOMMS_SPI_MODE,
    // .mode = 1,
    .spics_io_num = 15,  // Currently no chip select
    .queue_size = 512,
    // .input_delay_ns = 10,
    .clock_source = SPI_CLK_SRC_DEFAULT,
    
    // There is probably more to configure here.
  };

  ret = spi_bus_add_device(HSPI_HOST, &hspi_cfg, &chipcomm_spi_handle);
  if(ret != ESP_OK){
    printf("Failed to add RP2040 SPI device to HSPI\n");
    spi_bus_remove_device(chipcomm_spi_handle);
  }

  // uint8_t tx_message[1] = {0};
  for(;;){
    char* tx_message = "Hello SPI, RP2040";
    // printf("Sending message: %s -HEX=", tx_message);
    // for(int i=0; i<strlen(tx_message); i++){
      // printf("0x%02X ", tx_message[i]);
    // }
    // printf("\n");
    // printf("The length of the command is: %d", strlen(tx_message)*8);
    spi_transaction_t hello_spi_transaction = {
      // .cmd = SPI_CMD_HELLO,
      .tx_buffer = tx_message,
      // Note: Length is in bits
      .length = strlen(tx_message) * 8,
      // .length = 8,
    };
    ret = spi_device_polling_transmit(chipcomm_spi_handle, &hello_spi_transaction );
    ESP_ERROR_CHECK(ret);
    // I may need to wait done after transmit.
    vTaskDelay(100);

  }
}
