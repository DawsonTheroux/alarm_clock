#ifndef CHIPCOMMS_SPI_HOST_H
#define CHIPCOMMS_SPI_HOST_H

#define HSPI_PIN_MOSI 13
#define HSPI_PIN_MISO 12
#define HSPI_PIN_CLK  14
#define HSPI_PIN_CS  15
#define HSPI_HOST SPI2_HOST // I think this is HSPI host.

void spi_data_transfer_task(void* args);

#endif
