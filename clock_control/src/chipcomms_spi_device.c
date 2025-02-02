// Pico includes.
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/irq.h"
#include "hardware/dma.h"

// FreeRTOS includes.
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Project includes.
#include "common_spi_configs.h"
#include "chipcomms_spi_device.h"
#include "main.h"

static spi_irq_data_t spi_irq_data;

void setup_spi()
{
  dma_channel_config spi_rx_dma_config;
  // Setup the SPI channel.
  spi_init(spi_default, CHIPCOMMS_SPI_FREQ_HZ);
  spi_set_slave(spi_default, true);
  gpio_set_function(SPI0_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI0_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI0_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI0_CSN_PIN, GPIO_FUNC_SPI);
  spi_set_format(spi_default,8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

  // Iniaialize spi_irq_data
  spi_irq_data.spi_rx_queue = xQueueCreate(16, sizeof(cc_spi_transaction_t*));
  spi_irq_data.spi_temp_rx_buffer = NULL;
  spi_irq_data.current_transfer_len = 0;
  spi_irq_data.dma_rx_chan = dma_claim_unused_channel(true);

  // Configure SPI RX DMA.
  spi_rx_dma_config = dma_channel_get_default_config(spi_irq_data.dma_rx_chan);
  channel_config_set_transfer_data_size(&spi_rx_dma_config, DMA_SIZE_8);
  channel_config_set_dreq(&spi_rx_dma_config, spi_get_dreq(spi_default, false));
  channel_config_set_read_increment(&spi_rx_dma_config, false);
  channel_config_set_write_increment(&spi_rx_dma_config, true);
  dma_channel_configure(spi_irq_data.dma_rx_chan,
                        &spi_rx_dma_config,
                        NULL,                         // set write address later.
                        &spi_get_hw(spi_default)->dr, // Read address.
                        0,                            // Set transfer size later.
                        false);                       // Start transfer NOW.

  // Setup the SPI IRQ.
  spi_get_hw(spi_default)->imsc |= SPI_SSPIMSC_RXIM_BITS; // Enable interupt on RX buffer !empty
  irq_set_exclusive_handler(SPI0_IRQ, spi_rx_irq_handler);
  irq_set_priority(SPI0_IRQ,1);
  irq_set_enabled(SPI0_IRQ,true);

  // Setup SPI RX DMA IRQ.
  dma_channel_set_irq0_enabled(spi_irq_data.dma_rx_chan, true);
  irq_set_exclusive_handler(DMA_IRQ_0, spi_dma_irq_handler);
  irq_set_priority(DMA_IRQ_0, 0);
  irq_set_enabled(DMA_IRQ_0, true);

  // printf("Set Baud rate: %d - RX pin: %d - TX pin: %d - SCK pin: %d - CS pin: %d\r\n", spi_get_baudrate(spi_default), SPI0_RX_PIN, SPI0_TX_PIN, SPI0_SCK_PIN, SPI0_CSN_PIN);
}

void spi_dma_irq_handler(void)
/* Handle the DMA DONE IRQ
 * This should reset the static rpi_irq_data
 * data instance by resetting the current transfer length
 * and moving the sip_temp_rx_buffer to NULL
 */
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  cc_spi_transaction_t* new_queue_data = (cc_spi_transaction_t*) malloc(sizeof(cc_spi_transaction_t));
  new_queue_data->data_length = spi_irq_data.current_transfer_len;
  new_queue_data->data = spi_irq_data.spi_temp_rx_buffer;
  // Reset the values in the spi_irq_data
  spi_irq_data.spi_temp_rx_buffer = NULL;
  spi_irq_data.current_transfer_len = 0;
  // Send the data to the queue, and yield.
  xQueueSendFromISR(spi_irq_data.spi_rx_queue, &new_queue_data, &xHigherPriorityTaskWoken);
  // Clear the SPI RX IRQ and DMA IRQ
  spi_get_hw(spi_default)->icr |= SPI_SSPICR_RTIC_BITS;
  dma_hw->ints0 = 1u << spi_irq_data.dma_rx_chan;
  // Reenable the SPI RX interrupt.
  irq_set_enabled(SPI0_IRQ,true);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void spi_rx_irq_handler(void)
/* Handle the RX IRQ for SPI by opening
 * a DMA channel and starting to listen
 * for DMA finished.
 */
{
  irq_set_enabled(SPI0_IRQ,false);
  // Disable the interupt while the transfer is occuring.
  spi_get_hw(spi_default)->imsc |= SPI_SSPIMSC_RXIM_BITS; // Enable interupt on RX buffer !empty
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint8_t data_parts[2];
  uint8_t* rx_data;
  // Read the first two bytes for the data length and malloc.
  spi_read_blocking(spi_default, 0, data_parts, 2);
  spi_irq_data.current_transfer_len = (data_parts[0] << 8) | data_parts[1];
  spi_irq_data.spi_temp_rx_buffer = (uint8_t*)malloc(spi_irq_data.current_transfer_len);
  // Set the DMA transfer count.
  dma_channel_set_trans_count(spi_irq_data.dma_rx_chan, spi_irq_data.current_transfer_len, false);
  // Set the RX buffer and start the counter.
  dma_channel_set_write_addr(spi_irq_data.dma_rx_chan, spi_irq_data.spi_temp_rx_buffer, true);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void cc_spi_rx_task(void* args)
{
  // printf("Hello SPI task\r\n");
  cc_spi_args_t* cc_spi_args = (cc_spi_args_t*)args;
  setup_spi();
  for(;;){
    cc_spi_transaction_t* cc_spi_transaction;
    if(pdTRUE == xQueueReceive(spi_irq_data.spi_rx_queue, &cc_spi_transaction, 100)){
      // control the flow based on the command byte.
      switch(cc_spi_transaction->data[0]){
        case SPI_CMD_TIMESYNC:
          // printf("Sending to SPI\r\n");
          // It is up to the time keeper to free the spi data now.
          xQueueSend(*(cc_spi_args->time_keeper_queue), &cc_spi_transaction, 100);
          break;
        default:
          printf("SPI received unknown command\r\n");
          free(cc_spi_transaction->data);
          free(cc_spi_transaction);
          break;
      }
      // free(cc_spi_transaction->data);
      // free(cc_spi_transaction);
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}
