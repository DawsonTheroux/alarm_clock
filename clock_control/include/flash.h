#ifndef FLASH_H
#define FLASH_H

#define FLASH_CS 7
#define FLASH_MAX_READ_ATTEMPTS (uint16_t) 10000
#define FLASH_MAX_WRITE_ATTEMPS (uint16_t) 10000
#define SD_BLOCK_LEN 512
#define SD_IDLE_STATUS   (uint8_t) 0x01
#define SD_READY_STATUS  (uint8_t) 0x00
#define SD_START_TOKEN   (uint8_t) 0xFE
#define SD_DATA_ACCEPTED (uint8_t) 0x1F

/* FLASH STATUS REGISTER BITS */
#define FLASH_RDSR_WIP_BIT  0x01
#define FLASH_RDSR_WEL_BIT  0x02
#define FLASH_RDSR_BP0_BIT  0x04
#define FLASH_RDSR_BP1_BIT  0x08
#define FLASH_RDSR_BP2_BIT  0x10
#define FLASH_RDSR_BP3_BIT  0x20
#define FLASH_RDSR_QE_BIT   0x40
#define FLASH_RDSR_SRWD_BIT 0x80


/* FLASH CONFIG REGISTER BITS */
#define FLASH_RDCR_ODS0_BIT  0x01
#define FLASH_RDCR_ODS1_BIT  0x02
/* bit 2 is reserved */
#define FLASH_RDCR_TB_BIT    0x08
#define FLASH_RDCR_PBE_BIT   0x10
#define FLASH_RDCR_4BYTE_BIT 0x20
/* bit 6 is reserved */
/* bit 7 is reserved */

/* FLASH SECURITY REGISTER CONFIG */
#define FLASH_RDSCUR_P_FAIL 0x20 // Program failed flag
#define FLASH_RDSCUR_E_FAIL 0x40 // Erase failed flag

/* FLASH COMMANDS */
#define FLASH_PP     0x02 // Page Program
#define FLASH_READ   0x03 // Read
#define FLASH_RDSR   0x05 // Read Status Register
#define FLASH_WREN   0x06 // Write Enable
#define FLASH_RDCR   0x15 // Read config Register
#define FLASH_SE     0x20 // Sector Erase
#define FLASH_CE     0x60 // Chip Erase
#define FLASH_RDSCUR 0x2B // Read security register
#define FLASH_EN4B   0xB7 // Enable 4-byte address

/** init_flash_gpio
 * Initializes the flash GPIOs
 */
void init_flash_gpio();

/** init_sd_spi_mode
 * Sets SD card into SPI mode SPI
 */
int init_sd_spi_mode();


int spi_flash_read_page(uint32_t address, uint8_t *res_buf, uint32_t buf_len);

int spi_flash_page_program(uint32_t address, uint8_t *buf, uint32_t buf_len);

int spi_flash_sector_erase(uint32_t address);

int spi_flash_chip_erase();

#endif
