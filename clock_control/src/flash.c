#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#include "FreeRTOS.h"

#include "main.h"
#include "task.h"
#include "flash.h"

// TODO:
// Read the status register from the device
// #define WAIT_MS 10000

uint8_t dummy_value = 0xFF;

static void spi_flash_write(uint8_t *write_buf, uint8_t msg_len)
{
	gpio_put(FLASH_CS, 0);
    spi_write_blocking(spi1, write_buf, msg_len);
	gpio_put(FLASH_CS, 1);
}

static void spi_flash_read_reg(uint8_t reg, uint8_t *response)
{
	gpio_put(FLASH_CS, 0);
    spi_write_blocking(spi1, &reg, 1);
	spi_read_blocking(spi1, dummy_value, response, 1);
	gpio_put(FLASH_CS, 1);
}

/* Returns -1 if the WIP never finished */
static int spi_flash_wait_wip()
{
		uint8_t reg_value;
		// for(uint32_t i=0;i<WAIT_MS;i++) {
        for(;;) {
		    spi_flash_read_reg(FLASH_RDSR, &reg_value);
			if(!(reg_value & FLASH_RDSR_WIP_BIT)) {
				return 0;
			}
    		vTaskDelay(1 / portTICK_PERIOD_MS);
		}
        printf("spi_flash_wait_wip timeout\n");
		return -1;
}

static int spi_flash_wait_wren()
{
	uint8_t command = FLASH_WREN;
	uint8_t reg_value;

	// for(uint32_t i=0;i<WAIT_MS;i++) {
    for(;;) {
		/* Write the WREN command */
		spi_flash_write(&command, 1);
		/* Check RDSR for write enable */
		reg_value = 0;
		spi_flash_read_reg(FLASH_RDSR, &reg_value);
		if(reg_value & FLASH_RDSR_WEL_BIT){
		    return 0;
		}
    	vTaskDelay(1 / portTICK_PERIOD_MS);
	}
	// Currently this is never reached. It might be good to make a max attempts value.
    printf("spi_flash_wait_wren timeout\n");
	return -1;
}

static int spi_flash_en4b()
{
		uint8_t command = FLASH_EN4B;
		uint8_t rdcr_value;

		/* Write enable */
		if(spi_flash_wait_wren()) {
					return -2;
		}
		/* Set EN4B */
		spi_flash_write(&command, 1);
		/* Read the configuration register */
		rdcr_value = 0;
		spi_flash_read_reg(FLASH_RDCR, &rdcr_value);

		/* Check if 4-byte mode is enabled */
		if(rdcr_value & FLASH_RDCR_4BYTE_BIT) {
				return 0;
		}
		return -1;
}

void init_flash_gpio()
{
  gpio_init(FLASH_CS);
  gpio_set_dir(FLASH_CS, GPIO_OUT);
  gpio_put(FLASH_CS, 1); // CS is active low.
}

int init_sd_spi_mode()
{
	/* set the spi flash into 4-Byte addressing mode */
	if(spi_flash_en4b()) {
		printf("FLASH ERROR: Failed to init flash in 4-byte address mode\n");
		return -1;
	}
    uint8_t buf[512];
    uint64_t addr = 0x2810;
    printf("==DEBUG, FIRST 512 (%X) bytes==\n", addr);
    spi_flash_read_page(0x2800, buf, 512);
    for(int i=0; i<512; i++) {
        printf("%02X", buf[i]);
    }
    // for(;;){}
    printf("\n==");

    return 0;

	/* TEST STUFF BELOW */
	/* Enable 4-byte addressing */
	// int buf_size = 512;
	// uint8_t write_buf[buf_size];
	// uint8_t read_buf[buf_size];
	// uint8_t reg_value;
	// /* init the test buffer */
	// bool count_up = false;
	// for(int i=0; i<buf_size; i++) {
	// 				if((uint8_t)i == 0) {
	// 								count_up = !count_up;
	// 				}

	// 				if(count_up) {
	// 						write_buf[i] = (uint8_t)i;
	// 				} else {
	// 						write_buf[i] = 255 - (uint8_t)i;
	// 				}
	// }
	// /* Sector erase (4k) */
	// if(spi_flash_sector_erase(0x0000)) {
	// 				printf("Flash never finished WIP\n");
	// 				return -1;
	// }

	// /* READ THE ERASED PAGE */
	// /* read the value of the page back */
	// printf("==Erased page==\n");
	// memset(read_buf, 0x00, buf_size);
	// spi_flash_read_page(0x0000, read_buf, buf_size);
	// for(int i=0; i<buf_size; i++){
	// 				printf("%02X ", read_buf[i]);
	// }
	// printf("\n\n");

	// /* PROGRAM THE PAGE WITH THE WRITE BUF */
	// if(spi_flash_page_program(0x0000, write_buf, buf_size)) {
	// 				printf("spi flash page program failed\n");
	// 				return -1;
	// }

	// /* READ THE FIRST PAGE FROM SPI FLASH */
	// spi_flash_read_page(0x0000, read_buf, buf_size);
	// printf("==Read value 1(After write): ==");
	// for(int i=0; i<buf_size; i++){
	// 		if(read_buf[i] != write_buf[i]) {
	// 				printf(" !!");
	// 		}
	// 		printf("%02X ", read_buf[i]);
	// }
	// printf("\n\n");

	// /* DEBUG, this command is called again if it returns -1 */
	// return -1;
	// // return 0;
}

int spi_flash_read_page(uint32_t address, uint8_t *res_buf, uint32_t buf_len)
{
	uint8_t command = FLASH_READ;
    uint8_t address_arr[4] = {0,0,0,0};
	address_arr[0] = (uint8_t)((address & 0xFF000000) >> 24);
	address_arr[1] = (uint8_t)((address & 0x00FF0000) >> 16); 
	address_arr[2] = (uint8_t)((address & 0x0000FF00) >> 8); 
	address_arr[3] = (uint8_t)(address & 0x000000FF);
	gpio_put(FLASH_CS, 0);
	spi_write_blocking(spi1, &command, 1);
  	spi_write_blocking(spi1, address_arr, 4);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	spi_read_blocking(spi1, dummy_value, res_buf, buf_len);
    gpio_put(FLASH_CS, 1);
	return 0;
}

int spi_flash_page_program(uint32_t address, uint8_t *buf, uint32_t buf_len)
{
	// Only 256 bytes can be transfered at a time.
	// Make this function break up the writes into 256 byte chunks.
	uint8_t command = FLASH_PP;
	uint8_t response = 0;
	uint8_t reg_value = 0;
	// uint8_t read_buf[256];
	uint8_t address_arr[4] = {0,0,0,0};
	uint32_t remaining_bytes = buf_len;
	uint32_t bytes_to_write = 0;

	/* Break the writes into 256 byte chunks */
	while(remaining_bytes > 0) {
			/* The number of bytes to write is either 256, or the number of remaining bytes */
			if(spi_flash_wait_wren()) {
					printf("FLASH ERROR: Failed to enable Write EN Bit\n");
					return -1;
			}
			spi_flash_read_reg(FLASH_RDSR, &reg_value);
			bytes_to_write = (remaining_bytes >= 256) ? 256: remaining_bytes;
			// bytes_to_write = (remaining_bytes >= 64) ? 64: remaining_bytes;

			// DEBUG, NO CHIP ENABLE FOR TESTING
			gpio_put(FLASH_CS, 0);

			command = FLASH_PP;
  		    spi_write_blocking(spi1, &command, 1);

			/* Convert the address into a 4 byte array */
			/* I was doing this like: (uint8_t *)&address,         */
			/* which doesn't work because system endianness. (DUH) */
			if (address_arr == NULL) {
					printf("address_arr is null???");
					return -1;
			}
			address_arr[0] = (uint8_t)((address & 0xFF000000) >> 24);
			address_arr[1] = (uint8_t)((address & 0x00FF0000) >> 16); 
			address_arr[2] = (uint8_t)((address & 0x0000FF00) >> 8); 
			address_arr[3] = (uint8_t)(address & 0x000000FF);

			/* Give the idle task some time to run before writting the full block */
			vTaskDelay(1 / portTICK_PERIOD_MS);
  		    spi_write_blocking(spi1, address_arr, 4);
			vTaskDelay(1 / portTICK_PERIOD_MS);
  		    spi_write_blocking(spi1, buf, bytes_to_write);

			// DEBUG NO CHIP ENABLE FOR TESTING
			gpio_put(FLASH_CS, 1);

			/* Wait for WIP flag to clean */
			if(spi_flash_wait_wip()) {
					printf("FLASH ERROR: Failed to wait for WIP in page program\n");
					return -2;
			}
			/* read the security register */
			spi_flash_read_reg(FLASH_RDSCUR, &reg_value);
			if(reg_value & FLASH_RDSCUR_P_FAIL) {
							printf("FLASH ERROR: SPI flash RDSCR reported P_FAIL\n");
							return -3;
			}

			if ((remaining_bytes - bytes_to_write) <= 0) {
					break;
			}
			
			/* Increment the buffers and counters */
			address += bytes_to_write;         // Address is incremented to the end of the written bytes.
			buf += bytes_to_write;			       // Buffer pointer is incremented to end of written bytes.
			remaining_bytes -= bytes_to_write; // Bytes written is removed from remaining bytes.

			vTaskDelay(1 / portTICK_PERIOD_MS);
	}

	return spi_flash_wait_wip();
}

int spi_flash_sector_erase(uint32_t address)
{
	uint8_t command = FLASH_SE;
	uint8_t response;
	uint8_t reg_value;

	/* Write enable */
	if(spi_flash_wait_wren()) {
			printf("FLASH ERROR: Failed to enable Write in RDSR\n");
			return -1;
	}

	/* Send the sector erase command */
	gpio_put(FLASH_CS, 0);
    spi_write_blocking(spi1, &command, 1);
    spi_write_blocking(spi1, (uint8_t*)&address, 4);
	gpio_put(FLASH_CS, 1);

	return spi_flash_wait_wip();
}


int spi_flash_chip_erase()
{
	uint8_t command = FLASH_CE;
	uint8_t response;
	uint8_t reg_value;

	/* Write enable */
	if(spi_flash_wait_wren()) {
			printf("FLASH ERROR: Failed to enable Write in RDSR\n");
			return -1;
	}

	/* Send the sector erase command */
	gpio_put(FLASH_CS, 0);
    spi_write_blocking(spi1, &command, 1);
	gpio_put(FLASH_CS, 1);

	return spi_flash_wait_wip();
}
