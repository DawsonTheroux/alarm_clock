#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "task.h"

#include "fs.h"
#include "flash.h"
#include "uart_flasher.h"

/* ENOUGH TO STORE PROG_START */
#define SMALL_BUF_SIZE 10

/* ENOUGH SPAGE FOR PAGE + 5 start bytes of (Page index, payload size) */
#define PROG_BUF_SIZE (PROG_HEADER_BYTES + FS_PAGE_SIZE)
#define PROG_HEADER_PAYLOAD_SIZE 3
#define PROG_HEADER_INDEX_SIZE 2
#define PROG_HEADER_BYTES PROG_HEADER_PAYLOAD_SIZE + PROG_HEADER_INDEX_SIZE

uint8_t *uart_buf;
static 
uint32_t uart_buf_ind = 0;
uint8_t *start_prog_string = "PROG_START";
uint32_t current_buf_size;

#define UART_ID uart0

static int suspend_all_tasks(TaskHandle_t *task_handles, uint32_t num_tasks) {
		// printf("All tasks suspended\r\n");
		for(uint32_t i; i<num_tasks; i++){
				vTaskSuspend(task_handles[i]);
		}
		return 0;
}

/* UART interrupt */
void uart_rx_interrupt() {
		while(uart_is_readable(UART_ID)) {
				uart_buf[uart_buf_ind] = uart_getc(UART_ID);
				uart_buf_ind = (uart_buf_ind + 1) % current_buf_size;
		};
}

static int handle_program_request(uart_flasher_args_t *uart_flasher_args)
{
		uint16_t target_index; // Target index in flash in fs pages (2048).
		uint32_t payload_size; // Payload size to write to SPI flash (Only 24 bits).
		uint64_t target_address;

		/* Disable the character IRQ */
		irq_remove_handler(UART0_IRQ, uart_rx_interrupt);
		irq_set_enabled(UART0_IRQ, false);
		uart_set_irq_enables(UART_ID, false, false);
		/* Acknowledge the program state */
		printf("PROG_ACK");
		/* Suspend all the tasks */
		suspend_all_tasks(uart_flasher_args->task_handles, 
											uart_flasher_args->num_tasks);

		/* Allocate the big buffer */
		free(uart_buf);
		uart_buf = (uint8_t *)malloc(sizeof(uint8_t) * PROG_BUF_SIZE);
		current_buf_size = PROG_BUF_SIZE;
		if(uart_buf == NULL) {
				printf("ERROR: Failed to allocate programming buffer\r\n");
				return -1;
		}
		memset(uart_buf, 0x00, PROG_BUF_SIZE);

		/* Clear the SPI flash */
		if(spi_flash_chip_erase()) {
				printf("ERROR: SPI Flash Chip Erase failed\r\n");
				return -1;
		}

		/* Notify the programmer data is ready to be received */
		printf("PROG_READY");

		/* Handle the new data */
		for(;;) {
				/* Wait for the header and derive the target index and payload size. */
				uart_read_blocking(UART_ID, uart_buf, 5);
				vTaskDelay(2 / portTICK_PERIOD_MS);
				target_index = (((uint16_t) uart_buf[0]) << 8) | 
												((uint16_t) uart_buf[1]);

				payload_size = (((uint32_t) uart_buf[2]) << 16) | 
										   (((uint32_t) uart_buf[3]) << 8) | 
												((uint32_t) uart_buf[4]);
				if (payload_size > 2048) {
						printf("ERROR: Payload size is bigger than the program buffer (payload size: %u, current_buf_size: %u\r\n", payload_size, current_buf_size);
						return -1;
				}

				/* Wait for the full payload */
				target_address = ((uint32_t) target_index) * ((uint32_t)FS_PAGE_SIZE);
				/* Send ack and wait for data */
				printf("PROG_ACK");
				printf("%0*d", 4, target_index); 
				printf("%0*d", 4, payload_size);
				uart_read_blocking(UART_ID, uart_buf, payload_size);
				vTaskDelay(2 / portTICK_PERIOD_MS);
				if (spi_flash_page_program(target_address, uart_buf, payload_size)) {
						printf("Failed to program page: %d with buffer of size: %d\n", target_index, payload_size);
				}

				/* If the target index is 0 and the payload size is 9, check if it is PROG_DONE, and finish if it is. */
				// if(target_index == 0 && payload_size == 9 && strncmp(uart_buf, "PROG_DONE", 9) == 0) {
				// if(target_index == 0 && payload_size == 9 && false) { 
				// 		printf("Received PROG_DONE\n");
				// 		break;
				// }
				/* Send an ACK saying that things are good */
				vTaskDelay(2 / portTICK_PERIOD_MS);
				printf("PROG_DONE");
				// printf("buf:0x%X", uart_buf);
				printf("%0*d", 4, target_index); 
				printf("%0*d", 4, payload_size);
				// counter++;
		}

		printf("EXITED\n");
		return 0;
}



void uart_flasher_task (void* args)
{
		bool program_complete = false;
		
        uart_buf = malloc(sizeof(uint8_t) * SMALL_BUF_SIZE);
		// uart_buf = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * SMALL_BUF_SIZE);
		if (uart_buf == NULL) {
				printf("ERROR: Failed to allocate uart buffer\r\n");
				for(;;){}
		}
		memset(uart_buf, 0x00, SMALL_BUF_SIZE);
		current_buf_size = SMALL_BUF_SIZE;
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
						uint8_t start_point = (i + uart_buf_ind) % SMALL_BUF_SIZE;
						bool found_match = true;
						// Check to see if the buffer starts at this point, does
						// the start string exit.
						for(int j=0; j<SMALL_BUF_SIZE; j++) {
								if(uart_buf[(start_point + j) % SMALL_BUF_SIZE] != start_prog_string[j]) {
										found_match = false;
										break;
								}
						}
						if(found_match) {
								/* try and program the falsh, if successfuly set the flag */
								if(!handle_program_request(uart_flasher_args)) {
										program_complete = true;
								}
								printf("handl_program_request returned???\r\n");
								break;
						}
				}
				// If the SPI flash has been programmed, starve the WD
				if(program_complete) {
						printf("STARVING THE WD\r\n");
						watchdog_enable(10, false);
						for(;;){
						}
				}
		}
}
