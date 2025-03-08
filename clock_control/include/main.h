#ifndef MAIN_H
#define MAIN_H

#define LED_PIN 25

#define CC_SPI_PRIORITY 5
#define CC_I2C_PRIORITY 5
#define TIME_KEEPER_PRIORITY 8
/* High priority, but does literally nothing 99.99% of the time. */
#define UART_FLASHER_PRIORITY 9

// #define SPI_FREQ_HZ (1 * 1000 * 1000)
// #define SPI_FREQ_HZ (1 * 1000 * 1000)

#define SPI_FREQ_HZ (60 * 1000 * 1000)
/* Flash works at 100MHz, but I need to test with display as well. */
// #define SPI_FREQ_HZ (100 * 1000 * 1000)

#endif

