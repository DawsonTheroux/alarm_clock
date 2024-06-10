#ifndef DISPLAY_H
#define DISPLAY_H


#define DISPLAY_PWR  15
#define DISPLAY_BUSY 14
#define DISPLAY_CS   13
// #define DISPLAY_DC   12
#define DISPLAY_DC 8
// #define DISPLAY_DOUT 11
// #define DISPLAY_CLK  10
#define SPI_CLK 10
#define SPI_DOUT 11
#define SPI_DIN 12
#define DISPLAY_RST  9

#define TEST_PIN 2

#define DISPLAY_WIDTH 792
#define DISPLAY_HEIGHT 272


/* The full length of the display_buffer array. */
static uint32_t display_buffer_size = ((DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8): (DISPLAY_WIDTH / 8 + 1)) * DISPLAY_HEIGHT * 2; 

/* The display buffer used by functions like draw_full_display() and draw_partial_display(). */
extern uint8_t* display_buffer;

/** display_basic_draw
 * Draws a test image on the display
 */
void display_basic_draw();

/** init_display_gpio
 * Initializes the GPIO used by the display.c file.
 *
 * NOTE: SPI bus used is initialied by the main.c file.
 */
void init_display_gpio();

/** display_init_full
 * Initializes the display to redraw the whole pannel.
 */
void display_init_full();

/** display_init_partial
 * Initializes the display to redraw the only a portion
 * of the pannel.
 */
void display_init_partial();

/** draw_full_display
 * Sends the full display_buffer to the pannel
 * for a full update.
 */
void draw_full_display();

/** draw_partial_display
 * Updates only a portion of the display.
 *
 * NOTE: X offsets must be multiples of four. When
 *       providing values without mod four, images 
 *       become unaligned by a byte. RAM X offsets
 *       must be byte aligned. Some logic could be
 *       added to fix this issue, but I am drained
 *       from the tedium of fixing the issues with
 *       display alignment. I like text alignment.
 */
void draw_partial_display(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end);

/** display_reset
 * Sends the hardware reset signal to the display using
 * the DISPLAY_RST pin.
 */
void display_reset();

/** display_send_data
 * Sends a byte of data to the display using SPI.
 *
 * Data works by setting DISPLAY_DC high prior to
 * sending the command
 */
void display_send_data(uint8_t data);

/** display_send_command
 * Sends a byte command to the display using SPI.
 *
 * Commands works by setting DISPLAY_DC low prior
 * to sending the command.
 */
void display_send_command(uint8_t command);

/** display_wait_busy
 * Waits for the display busy line to go low
 * indicating display is no longer busy.
 */
void display_wait_busy();

/** display_clear
 * Full update of white to the screen.
 */
void display_clear();

/** display_turn_on_full
 * Turn on the display for a full update.
 */
void display_turn_on_full();

/** display_turn_on_partial
 * Turns on the display for a partial update.
 */
void display_turn_on_partial();

/** display_sleep
 * Puts the display to sleep and powers it down
 */
void display_sleep();

/** superimpose_image
 * Super imposes image date from 'image' onto the display buffer
 * given its width, height, x, and y position.
 */
void superimpose_image(uint16_t x_pos, uint16_t y_pos, uint8_t* image, uint16_t image_width, uint16_t image_height);

/** update_display_time
 * Updates the time on the display using a datetime_t object
 */ 
void update_display_time(datetime_t new_time, bool full_update);


/* The lut data to send to the registers */
static unsigned char LUT_DATA_4Gray[] = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 
0x01, 0x4A, 0x00, 0x00, 0x00, 0x01, 0x00,
0x01, 0x82, 0x42, 0x00, 0x00, 0x10, 0x00,
0x01, 0x8A, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 
0x01, 0x41, 0x00, 0x00, 0x00, 0x01, 0x00,
0x01, 0x82, 0x42, 0x00, 0x00, 0x10, 0x00,
0x01, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 
0x01, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00,
0x01, 0x82, 0x42, 0x00, 0x00, 0x10, 0x00,
0x01, 0x41, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x01, 0x8A, 0x00, 0x00, 0x00, 0x01, 0x00,
0x01, 0x82, 0x42, 0x00, 0x00, 0x10, 0x00,
0x01, 0x4A, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

0x02, 0x00, 0x00,    //FR, XON
0x22, 0x17, 0x41, 0xA8, 0x32, 0x40, 
//EOPT  VGH   VSH1  VSH2  VSL   VCOM

};
#endif

