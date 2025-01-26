#ifndef TIME_KEEPER_H
#define TIME_KEEPER_H
#include "FreeRTOS.h"
#include "queue.h"

typedef struct time_keeper_args_t{
  QueueHandle_t* i2c_tx_queue;
  QueueHandle_t* spi_rx_queue;
}time_keeper_args_t;

void time_keeper_task(void* args);
void setup_rtc();
void update_time_from_spi();


#endif
