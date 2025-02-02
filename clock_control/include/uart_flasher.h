#ifndef UART_FLASHER_H
#define UART_FLASHER_H

#include "FreeRTOS.h"
#include "queue.h"

/* The UART flasher task is used to program the SPI flash on the pico.  */
/* The majority of the time it will do LITTERALLY NOTHING.              */
/* All it is going to do is wait on a semaphore from a UART interrupt.  */
/* When it gets the correct message starting the programming sequence   */
/* From UART, it will suspend all other tasks, and then program the SPI */
/* flash.                                                               */

typedef struct uart_flasher_args_t {
		TaskHandle_t *task_handles;
		uint32_t num_tasks;
} uart_flasher_args_t;


void uart_flasher_task (void* args);

#endif
