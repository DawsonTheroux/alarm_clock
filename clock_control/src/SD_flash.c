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

// TODO: Make this wayyy faster with DMA.
//       This way it could be a fast PICO library.

uint8_t dummy_value = 0xFF;
void init_flash_gpio()
{
  gpio_init(FLASH_CS);
  gpio_set_dir(FLASH_CS, GPIO_OUT);
  gpio_put(FLASH_CS, 1); // CS is active low.
}

void init_flash()
{
  /* To initialize the SD card, we must send 80 clock cycles to it.*/
  /* So lets send 10 bytes */
  spi_write_blocking(spi1, &dummy_value, 1);
  spi_write_blocking(spi1, &dummy_value, 10);
}

int init_sd_spi_mode()
{
  uint8_t res[5];
  uint8_t command_count = 0;
  uint16_t try_count = 0;
  memset(res, 0xFF, 5);

  // Attempt to start the SD card with CMD0.
  while(res[0] != SD_IDLE_STATUS){
    if(++try_count >= 100){
      printf("!!SD ERROR: 0!!\r\n");
      return -1;
    }
    if(send_recv_sd_command(sd_cmd_0, res)){
      printf("!!SD ERROR: 1!!\r\n");
      return -1;
    }
  }
  vTaskDelay(1 / portTICK_PERIOD_MS);

  // Send CMD8
  if(send_recv_sd_command(sd_cmd_8, res)){
    printf("!!SD ERROR: 2!!\r\n");
    return -1;
  }
  
  // Check SD_IDLE_STATUS and echo command.
  if(res[0] != SD_IDLE_STATUS || res[4] != 0xAA){
    printf("!!SD ERROR: 3!!\r\n");
    return -1;
  }

  // Wait until 0x41 responds with res[0] = 0x0;
  while(res[0] != SD_READY_STATUS){
    if(command_count >= 100){
      printf("!!SD ERROR: 4!!\r\n");
      return -1;
    }

    // SEND COMMAND 55
    if(send_recv_sd_command(sd_cmd_55, res)){
      printf("!!SD ERROR: 5!!\r\n");
      return -1;
    }
    // If command 55 failed, don't send command 41.
    if(res[0] < 2){
      if(send_recv_sd_command(sd_cmd_41, res)){
        printf("!!SD ERROR: 6!!\r\n");
        return -1;
      }
    }
    command_count++;
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  if(send_recv_sd_command(sd_cmd_58, res)){
    printf("!!SD ERROR: 7!!\r\n");
    return -1;
  }
  // Check SD_IDLE_STATUS and echo command.
  // SEND COMMAND 58
  res[1] = 0x00;
  if(send_recv_sd_command(sd_cmd_58, res)){
    printf("!!SD ERROR: 8!!\r\n");
    return -1;
  }
  if(!(res[1] & 0x80)){
    printf("!!SD ERROR: 9!!\r\n");
    return -1;
  }
  return 0;
}

int flash_read_block(uint32_t addr, uint8_t *buffer, uint8_t *token)
{
  uint8_t res, read;
  uint16_t read_attempts = 0;
  int status = 0;
  // Enable FLASH CS.
  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 0);
  spi_write_blocking(spi1, &dummy_value, 1);

  // Issue read command.
  transmit_sd_command(sd_cmd_17.cmd, addr, sd_cmd_17.crc);
  if(sd_cmd_17.response_func(&res)){
    printf("No response from CMD17 (Read block)\r\n");
    status = -1;
    *token = res;
    goto read_exit;
  }
  if(res == 0xFF){
    printf("CMD17 returned 0xFF\r\n");
    status = -1;
    *token = res;
    goto read_exit;
  }
  // Read the line until the data start byte is present.
  while(++read_attempts != FLASH_MAX_READ_ATTEMPTS){
    spi_read_blocking(spi1, dummy_value, &res, 1);
    if(res != 0xFF){
      break;
    }
  }
  if(read_attempts == FLASH_MAX_READ_ATTEMPTS){
    printf("Reached max read attempts...Failing SD read\r\n");
    *token = res;
    status = -1;
    goto read_exit;
  }
  // SD card signals start of read data.
  if(res == SD_START_TOKEN){
    // Read block (512 bytes + 2 bytes for the CRC)
    spi_read_blocking(spi1, dummy_value, buffer, 512 + 2);
  }else{
    printf("SD Read error: %02X\r\n", res);
  }
  *token = res;
read_exit:
  // Disable FLASH CS
  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 1);
  spi_write_blocking(spi1, &dummy_value, 1);
  return status;
}

int flash_write_block(uint32_t addr, uint8_t *buffer, uint8_t *token)
{
  uint8_t res;
  uint16_t read_attempts = 0;
  int status = 0;
  uint8_t data_start[1] = {SD_START_TOKEN};
  *token = 0xFF;

  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 0);
  spi_write_blocking(spi1, &dummy_value, 1);

  // Issue write command.
  transmit_sd_command(sd_cmd_24.cmd, addr, sd_cmd_24.crc);

  if(sd_cmd_24.response_func(&res)){
    printf("No response from CMD24 (Write block)\r\n");
    status = -1;
    *token = res;
    goto write_exit;
  }

  if(res == SD_READY_STATUS){
    spi_write_blocking(spi1, data_start, 1);
    spi_write_blocking(spi1, buffer, 512);

    while(++read_attempts <= FLASH_MAX_WRITE_ATTEMPS){
      spi_read_blocking(spi1, dummy_value, &res, 1);
      if(res != 0xFF){
        break;
      }
    }
    if((res & 0x1F) == 0x05){
      *token = 0x05;
      while(res == 0x00){
        if(++read_attempts > 100){
          break;
        }
        spi_read_blocking(spi1, dummy_value, &res, 1);
      }
    }else{
      *token = res;
    }
  }
write_exit:
  // Disable FLASH CS
  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 1);
  spi_write_blocking(spi1, &dummy_value, 1);
  return status;
}


int send_recv_sd_command(sd_command_t sd_cmd, uint8_t *res)
{
  int status = 0;
  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 0);
  spi_write_blocking(spi1, &dummy_value, 1);

  // Send the CMD8 command.
  transmit_sd_command(sd_cmd.cmd, sd_cmd.arg, sd_cmd.crc);

  // If reading the response fails.
  if(sd_cmd.response_func(res)){
    status = -1;
  }
  spi_write_blocking(spi1, &dummy_value, 1);
  gpio_put(FLASH_CS, 1);
  spi_write_blocking(spi1, &dummy_value, 1);
  return status;
}

void transmit_sd_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
  // Package the input into a buffer.
  uint8_t cmd_buffer[6] = {(cmd|0x40), 
                           (uint8_t)(arg >> 24),
                           (uint8_t)(arg >> 16),
                           (uint8_t)(arg >> 8),
                           (uint8_t)(arg),
                           (crc|0x01)};
  // TODO: I would like to calculate the CRC instead of having to provide it.
  // Transmit the generated buffer.
  spi_write_blocking(spi1, cmd_buffer, 6);
}

int sd_read_res1(uint8_t* res)
{
  int status = -1;
  int i = 0;
  // Wait 8 bytes for a response.
  for(i=0; i<8;i++){
    spi_read_blocking(spi1, dummy_value, res, 1);
    if(res[0] != 0xFF){
      status = 0;
      break;
    }
  }
  return status;
}


int sd_read_res3_7(uint8_t* res)
{
  if(sd_read_res1(res)){
    return -1;
  }
  spi_read_blocking(spi1, dummy_value, res+1, 4);
  return 0;
}
