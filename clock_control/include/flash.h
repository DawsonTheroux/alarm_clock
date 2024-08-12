#ifndef FLASH_H
#define FLASH_H

#define FLASH_CS 2
#define FLASH_MAX_READ_ATTEMPTS (uint16_t) 10000
#define FLASH_MAX_WRITE_ATTEMPS (uint16_t) 10000
#define SD_BLOCK_LEN 512
#define SD_IDLE_STATUS   (uint8_t) 0x01
#define SD_READY_STATUS  (uint8_t) 0x00
#define SD_START_TOKEN   (uint8_t) 0xFE
#define SD_DATA_ACCEPTED (uint8_t) 0x1F


typedef struct sd_command_t {
  uint8_t cmd;    // The command number.
  uint32_t arg;   // The argument for the command. 
  uint8_t crc;    // The CRC for the command.
  int(*response_func)(uint8_t*);  // The funciton pointer for the command.
}sd_command_t;

/** init_flash_gpio
 * Initializes the flash GPIOs
 */
void init_flash_gpio();

/** init_flash
 * Initializes the SD card to SPI mode.
 */
void init_flash();

/** init_sd_spi_mode
 * Sets SD card into SPI mode SPI
 */
int init_sd_spi_mode();

/** flash_read_block
 * Reads a block from the flash.
 *
 * addr:   The flash page to read from.
 * buffer: The buffer to read the data to (Must be 514 bytes)
 * token:  Response value.
 * 
 */
int flash_read_block(uint32_t addr, uint8_t *buffer, uint8_t *token);

/** flash_write_block
 * Writes a block to flash.
 *
 * addr:   The address to write to
 * buffer: The buffer to read the data to (Must be 514 bytes)
 * token:  Response value.
 */
int flash_write_block(uint32_t addr, uint8_t *buffer, uint8_t *token);

/** transmit_sd_command 
 * Handles the transmission of the SD command via the SPI bus.
 *
 * Do not call this directly, instead call send_recv_sd_command.
 */
void transmit_sd_command(uint8_t cmd, uint32_t arg, uint8_t crc);

/** send_recv_sd_command
 * Performs CS, uses transmit_sd_command and waits for the appropriate
 * response of the SD command.
 */
int send_recv_sd_command(sd_command_t sd_cmd, uint8_t* res);

/** sd_read_res1
 * Wait for res1
 *
 * Returns the response of the command.
 *  Value of 0 means nothing was received.
 */
int sd_read_res1(uint8_t* res);

/** sd_read_res7
 * Read a SPI response type 7. (This response if 5 bytes long?)
 */
int sd_read_res3_7(uint8_t* res);

// Command 0 - Set idle state
static sd_command_t sd_cmd_0 = {
  .cmd = 0,
  .arg = 0x00000000,
  .crc = 0x94,
  .response_func = sd_read_res1,
};

// Command 8 - Send interface condition.
static sd_command_t sd_cmd_8 = {
  .cmd = 8,
  .arg = 0x0000001AA,
  .crc = 0x86,
  .response_func = sd_read_res3_7,
};

// Command 17 - SD read block.
// When using this command, you need 
// to manage the data read after res1.
static sd_command_t sd_cmd_17 = {
  .cmd = 17,
  .arg = 0, // Arg is the address of the block
  .crc = 0x00,
  .response_func = sd_read_res1,
};

// Command 24 - SD write block.
// When using this command, the data read
// from the command must be managed. 
// response_func is only used to report the error code.
static sd_command_t sd_cmd_24 = {
  .cmd = 24,
  .arg = 0,  //Arg is the address to write to.
  .crc = 0x00,
  .response_func = sd_read_res1,
};

// Command 41 - ACMD application command
static sd_command_t sd_cmd_41 = {
  .cmd = 41,
  .arg = 0x40000000,
  .crc = 0x00,
  .response_func = sd_read_res1,
};

// Command 55 - Next command is APP_CMD
static sd_command_t sd_cmd_55 = {
  .cmd = 55,
  .arg = 0x00000000,
  .crc = 0x00,
  .response_func = sd_read_res1,
};

static sd_command_t sd_cmd_58 = {
  .cmd = 58,
  .arg = 0x00000000,
  .crc = 0x00,
  .response_func = sd_read_res3_7,
};

#endif
