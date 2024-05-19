#ifndef CHIPCOMMS_SPIS_DEVICE_H
#define CHIPCOMMS_SPIS_DEVICE_H
#include "hardware/dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SPI0_RX_PIN PICO_DEFAULT_SPI_RX_PIN
#define SPI0_TX_PIN PICO_DEFAULT_SPI_TX_PIN
#define SPI0_SCK_PIN PICO_DEFAULT_SPI_SCK_PIN
#define SPI0_CSN_PIN PICO_DEFAULT_SPI_CSN_PIN
#define SPI_DMA_CHAN 0;

typedef struct spi_irq_data_t{
  QueueHandle_t spi_rx_queue;
  uint8_t* spi_temp_rx_buffer;
  uint16_t current_transfer_len;
  uint dma_rx_chan;
}spi_irq_data_t;

typedef struct cc_spi_transaction_t{
  uint8_t* data;
  uint16_t data_length;
} cc_spi_transaction_t;

typedef struct cc_spi_args_t{
  QueueHandle_t* time_keeper_queue;
} cc_spi_args_t;

void cc_spi_rx_task(void* args);
void spi_dma_irq_handler(void);
void spi_rx_irq_handler(void);

#endif
