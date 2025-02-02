#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include "FreeRTOS.h"
#include "task.h"

#include "uart_flasher.h"

/* ENOUGH TO STORE PROG_START */
#define SMALL_BUF_SIZE 10
uint8_t small_buf[SMALL_BUF_SIZE];
uint8_t small_buf_ind = 0;
uint8_t *start_prog_string = "PROG_START";

#define UART_ID uart0

/* UART interrupt */
void uart_rx_interrupt() {
		while(uart_is_readable(UART_ID)) {
				small_buf[small_buf_ind] = uart_getc(UART_ID);
				small_buf_ind = (small_buf_ind + 1) % SMALL_BUF_SIZE;
		};
}

void uart_flasher_task (void* args)
{
		printf("Hello uart task\r\n");
		uart_flasher_args_t *uart_flasher_args = (uart_flasher_args_t *)args;

		/* Setup the interrupt to monitor the UART */
		irq_set_exclusive_handler(UART0_IRQ, uart_rx_interrupt);
		irq_set_enabled(UART0_IRQ, true);
		uart_set_irq_enables(UART_ID, true, false);

		for(;;) {
				vTaskDelay(500 / portTICK_PERIOD_MS);

				// Loop through the buffer for each char as the starting point.
				for(int i=0; i<SMALL_BUF_SIZE; i++) {
						uint8_t start_point = (i + small_buf_ind) % SMALL_BUF_SIZE;
						bool found_match = true;
						// Check to see if the buffer starts at this point, does
						// the start string exist.
						for(int j=0; j<SMALL_BUF_SIZE; j++) {
								if(small_buf[(start_point + j) % SMALL_BUF_SIZE] != start_prog_string[j]) {
										found_match = false;
										break;
								}
						}
						if(found_match) {
								printf("FOUND MATCH\r\n");
								printf("PROG_ACK");
								break;
						}
				}
				// Wait for a queue, the queue will have a UART message.
				// If the message is PROG_START
				// Respond with PROG_ACK
				// Clear the SPI FLASH
				// Stop all tasks
				// Notify PROG_READY
				// A buffer is allocated that is fs page size + 5 bytes
				// When a transaction is received, the first two bytes is the page offset
				//		the second three bytes is the number of bytes in the transaction. (2048 MAX)
				//		and the remaining is the data to be written.
				// The board should be reset after the data is programed. maybe through software or some
				//   other means.
		}

}
