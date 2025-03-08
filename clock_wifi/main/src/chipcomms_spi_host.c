#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "chipcomms_spi_host.h"
#include "common_inc/common_spi_configs.h"

// TODO: Move the setup out of the spi task.
void cc_spi_tx_task(void* args)
{
  printf("Hello SPI task\n");
  cc_spi_args_t* cc_spi_args = (cc_spi_args_t*)args;
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
  ret = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH2);
  ESP_ERROR_CHECK(ret);

  spi_device_interface_config_t hspi_cfg = {
    .clock_speed_hz = CHIPCOMMS_SPI_FREQ_HZ,
    .mode = CHIPCOMMS_SPI_MODE,
    .spics_io_num = 15,  // Currently no chip select
    .queue_size = 512,
    .clock_source = SPI_CLK_SRC_DEFAULT,
  };

  ret = spi_bus_add_device(HSPI_HOST, &hspi_cfg, &chipcomm_spi_handle);
  if(ret != ESP_OK){
    printf("Failed to add RP2040 SPI device to HSPI\n");
    spi_bus_remove_device(chipcomm_spi_handle);
  }

  spi_transaction_t* spi_transaction = NULL;
  for(;;){
    if(pdTRUE == xQueueReceive(*(cc_spi_args->tx_queue), &spi_transaction, pdMS_TO_TICKS(100))){
      printf("SPI: Sending message of len: %d with code: 0x%02X\n", spi_transaction->length, ((uint8_t*)spi_transaction->tx_buffer)[2]);
      ret = spi_device_polling_transmit(chipcomm_spi_handle, spi_transaction);
      ESP_ERROR_CHECK(ret);
      free(spi_transaction->tx_buffer);
      free(spi_transaction);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
