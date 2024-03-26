// Pico includes.
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/irq.h"

// FreeRTOS includes.
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Project includes.
#include "common_inc/common_spi_configs.h"
#include "chipcomms_spi_device.h"
#include "main.h"

/* NOTE: This could probably be improved
*        by modifying the ISR to only read what is currently
*        available and then let the scheduler continue.
*
*        For now though, I am expecting small amounts of transfers
*        so I will leave as is.
*
*        This may be a good option in case there is a problem with the line.
*/

QueueHandle_t spi_rx_queue;
uint8_t* spi_buffer;

#define MAX_SPI_LEN 512

typedef struct spi_rx_data_t{
  uint16_t data_length;
  uint8_t* data;
  uint8_t command;
} spi_rx_data_t;

void setup_spi()
{
    spi_init(spi_default, CHIPCOMMS_SPI_FREQ_HZ);
    spi_set_slave(spi_default, true);
    gpio_set_function(SPI0_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_CSN_PIN, GPIO_FUNC_SPI);
    spi_set_format(spi_default,8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
    printf("Set Baud rate: %d - RX pin: %d - TX pin: %d - SCK pin: %d - CS pin: %d\r\n", spi_get_baudrate(spi_default), SPI0_RX_PIN, SPI0_TX_PIN, SPI0_SCK_PIN, SPI0_CSN_PIN);

}

void spi_irq_handler(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint8_t data_parts[2];
  uint8_t* rx_data;
  spi_rx_data_t* new_queue_data = (spi_rx_data_t*) malloc(sizeof(spi_rx_data_t));
  // Read the first two bytes of data to get the transmit size.
  spi_read_blocking(spi_default, 0, data_parts, 2);
  new_queue_data->data_length = data_parts[0] << 8 | data_parts[1];
  // Allocate the rx data buffer to the data_length.
  rx_data = (uint8_t*)malloc(new_queue_data->data_length);
  // Read data_length bytes from the SPI line.
  spi_read_blocking(spi_default, 0, rx_data, new_queue_data->data_length);
  new_queue_data->data = rx_data;
  // Clear the interupt.
  spi_get_hw(spi_default)->icr |= SPI_SSPICR_RTIC_BITS;
  // Send the data to the queue, and yield.
  xQueueSendFromISR(spi_rx_queue, &new_queue_data, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void spi_read_task(void* args)
{
  setup_spi();
  spi_rx_queue = xQueueCreate(16, sizeof(spi_rx_data_t*));
  spi_get_hw(spi_default)->imsc |= SPI_SSPIMSC_RXIM_BITS;
  spi_buffer = (uint8_t*)malloc(256);

  irq_set_exclusive_handler(SPI0_IRQ, spi_irq_handler);
  irq_set_priority(SPI0_IRQ,0);
  irq_set_enabled(SPI0_IRQ,true);
  bool failed = false;
  for(;;){
    spi_rx_data_t* spi_rx_data;
    if(pdTRUE == xQueueReceive(spi_rx_queue, &spi_rx_data, portMAX_DELAY)){
      printf("Received length: %d - Message: ", spi_rx_data->data_length);
      //for(int i=0;i<spi_rx_data.data_length; i++)
      for(int i=0;i<spi_rx_data->data_length; i++)
      {
        printf("%c", spi_rx_data->data[i]);
      }
      printf("\r\n");
      free(spi_rx_data->data);
      free(spi_rx_data);
    }
  }
}
