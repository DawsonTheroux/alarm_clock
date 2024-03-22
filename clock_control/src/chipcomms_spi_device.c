// Pico includes.
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// FreeRTOS includes.
#include "FreeRTOS.h"
#include "task.h"

// Project includes.
#include "common_inc/common_spi_configs.h"
#include "chipcomms_spi_device.h"

/*TODO:
 * Setup an IRQ to handle the reception of SPI data
 * In reality, what would be nice would be a two message approach
 * where the first message is the length of the message that is about to be sent,
 * then the next is the full message.
 *
 * THis may actually be necessary since the read blocking just blocks until a read is possible
*/

void spi_read_task(void* args)
{
    // Enable SPI 0 at 1 MHz and connect to GPIOs
    int buf_len = 16;
    spi_init(spi_default, CHIPCOMMS_SPI_FREQ_HZ);
    spi_set_slave(spi_default, true);
    gpio_set_function(SPI0_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_CSN_PIN, GPIO_FUNC_SPI);
    spi_set_format(spi_default,8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
    printf("RX pin: %d - TX pin: %d - SCK pin: %d - CS pin: %d\r\n", SPI0_RX_PIN, SPI0_TX_PIN, SPI0_SCK_PIN, SPI0_CSN_PIN);

  uint8_t in_buf[buf_len];
  uint8_t out_buf[buf_len];
  for(int i=0; i<buf_len;i++){
    out_buf[1]=0;
  }

  int counter = 0;
  for(;;){
    printf("Hello SPI %d\r\n", counter++);
    // spi_read_blocking(spi_default, 0, in_buf, buf_len);
    spi_write_read_blocking(spi_default, out_buf, in_buf, buf_len);
    printf("Reading from the SPI buffer: ");
    for(int i=0; i<buf_len; i++){
      printf("0x%02X - ", in_buf[i]);
    }
    printf("\r\n");
    printf("Now in chars: ");
    for(int i=0; i<buf_len; i++){
      printf("%c-", in_buf[i]);
    }
    printf("\r\n");
  }
}
