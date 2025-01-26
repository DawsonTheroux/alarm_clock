#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "flash.h"

int initialize_sd(sdspi_dev_handle_t *new_dev_handle)
{

  sdmmc_card_t out_card;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI_FLASH_HOST;
  host.max_freq_khz = 5000;

  sdspi_device_config_t dev_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  dev_config.host_id = SPI_FLASH_HOST;
  dev_config.gpio_cs = 5;
  dev_config.gpio_cd = SDSPI_SLOT_NO_CD;
  dev_config.gpio_wp = SDSPI_SLOT_NO_WP;
  dev_config.gpio_int = SDSPI_SLOT_NO_INT;

  // Init the host device.
  spi_bus_config_t buscfg = {
      .miso_io_num = SD_SPI_MISO,
      .mosi_io_num = SD_SPI_MOSI,
      .sclk_io_num = SD_SPI_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 5000,
  };


  if(sdspi_host_init()){
    printf("flash.c - Failed to init flash host\n");
    return -1;
  }

  // Initialize the SPI bus
  if(spi_bus_initialize(host.slot, &buscfg, SPI_DMA_CH1)){
    printf("flash.c - Failed to init SPI host\n");
    return -1;
  }

  if(sdspi_host_init_device(&dev_config, new_dev_handle)){
    printf("flash.c - Failed to init device\n");
    return -1;
  }

  if(sdmmc_card_init(&host, &out_card)){
    printf("flash.c - sdmmc_card_init returned non-zero value.\n");
    return -1;
  }

  printf("Reading a block from index 0:\n");
  uint8_t buffer[512];
  if(sdmmc_io_read_bytes(&out_card, 0, 0, buffer, 512)){
    printf("sdmmc_io_read_bytes returned non-zero value\n");
  }
  for(int i=0; i<512; i++){
    printf("%02X", buffer[i]);
  }
  printf("\n");
  return 0;
}

