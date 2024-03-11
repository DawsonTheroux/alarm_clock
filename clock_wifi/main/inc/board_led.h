#ifndef BOARD_LED_H
#define BOARD_LED_H
// LED config
#define BOARD_LED_R 0
#define BOARD_LED_G 1
#define BOARD_LED_B 8

#define BLINK_DELAY_MS 100

void blink_led(void);
void configure_led(void);
void set_led_status(int status_number);

#endif
