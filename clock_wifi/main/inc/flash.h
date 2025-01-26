#ifndef FLASH_H
#define FLASH_H

#define SPI_FLASH_HOST SPI3_HOST

#define SD_SPI_MOSI 23
#define SD_SPI_CS 21
#define SD_SPI_MISO 19
#define SD_SPI_CLK 18

int initialize_sd(sdspi_dev_handle_t *new_dev_handle);

#endif
