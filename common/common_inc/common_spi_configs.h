#ifndef COMMON_SPI_CONFIGS_H
#define COMMON_SPI_CONFIGS_H

#define CHIPCOMMS_SPI_FREQ_HZ (1 * 1000 * 1000) // 1MHz (Can be up to 80)
// #define CHIPCOMMS_SPI_FREQ_HZ (500 * 1000) // 1MHz (Can be up to 80)

// There is a bug in SPI mode 0 for RP2040, so it must be SPI mode 1
#define CHIPCOMMS_SPI_MODE 1

// SPI COMMADNDS
#define SPI_CMD_HELLO 0x01

void spi_data_transfer_task(void* args);

#endif
