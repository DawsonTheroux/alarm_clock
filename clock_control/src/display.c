#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "images.h"
#include "fs.h"

#include "display.h"


uint8_t* display_buffer;

#define DIGIT_BUFFER_SIZE 4650

/*
 * Clear a section of the display.
 * If the start and end position are the same, then just clear the whole thing.
 */
void clear_display(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y)
{
  uint32_t display_width = ((DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8): (DISPLAY_WIDTH / 8 + 1)) * DISPLAY_HEIGHT * 2; 
  uint32_t full_width = (DISPLAY_WIDTH % 4 == 0) ? (DISPLAY_WIDTH / 4) : (DISPLAY_WIDTH / 4 + 1);
  if(start_x == end_x && start_y == end_y){
    memset(display_buffer, 0xFF, display_width);
    return;
  }
  start_x = (start_x / 4);
  end_x = (end_x / 4);
  for(int i=0; i<end_y - start_y; i++){
    memset(display_buffer + (((start_y + i) * full_width) + start_x), 0xFF, (end_x - start_x));
  }
}


/*
void display_basic_draw()
{
  uint32_t image_size = ((DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8): (DISPLAY_WIDTH / 8 + 1)) * DISPLAY_HEIGHT * 2; 
  uint8_t* display_buffer = malloc(image_size);
  for(int i=0; i< image_size; i++){
    display_buffer[i] = 0x00;
  }
  display_init_full();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  int x_index = 0;
  superimpose_image(x_index, 0, digit_zero, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_one, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_two, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_three, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_four, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_five, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_six, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_seven, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_eight, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, digit_nine, 28, 48);
  x_index += 30;
  superimpose_image(x_index, 0, smile, 16, 16);
  draw_full_display();
  free(display_buffer);
  display_sleep();
}
*/

void init_display_gpio()
{
  /* Init display power pin */
  gpio_init(DISPLAY_PWR);
  gpio_set_dir(DISPLAY_PWR, GPIO_OUT);
  gpio_put(DISPLAY_PWR, 0);
  /* Init display reset pin */
  gpio_init(DISPLAY_RST);
  gpio_set_dir(DISPLAY_RST, GPIO_OUT);
  gpio_put(DISPLAY_RST, 1);
  /* Init display DC (Data/Command) pin */
  gpio_init(DISPLAY_DC);
  gpio_set_dir(DISPLAY_DC, GPIO_OUT);
  gpio_put(DISPLAY_DC, 0);
  /* Init display Chip select pin */
  gpio_init(DISPLAY_CS);
  gpio_set_dir(DISPLAY_CS, GPIO_OUT);
  gpio_put(DISPLAY_CS , 1);
  /* Init display clock pin */
  // TODO: Move this out of the display init gpio command.
  // spi_init(spi1, DISPLAY_SPI_FREQ_HZ);
  // gpio_set_function(SPI_CLK, GPIO_FUNC_SPI);
  // gpio_set_function(SPI_DOUT, GPIO_FUNC_SPI);
  // gpio_set_function(SPI_DIN, GPIO_FUNC_SPI);
  // spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  /* Init display busy detect pin */
  gpio_init(DISPLAY_BUSY);
  gpio_set_dir(DISPLAY_BUSY, GPIO_IN);

}

void display_init_full()
{
  gpio_put(DISPLAY_PWR, 1);            // Turn on the display
  vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 10ms
  display_reset();                     // Hardware reset
  display_wait_busy();
  display_send_command(0x12);          // Issue software reset
  vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 10ms
  display_wait_busy();
  // here I could also be setting a Gate setting. I don't know what that is...

  display_send_command(0x0C);
  display_send_data(0x8B);
  display_send_data(0x9C);
  display_send_data(0xA6);
  display_send_data(0x0F);

  display_send_command(0x3C);
  display_send_data(0x81);
  
  display_send_command(0x11);
  display_send_data(0x01);

  /* Set RAM X address */
  display_send_command(0x44);
  display_send_data(0x00);
  display_send_data(0x31);
  display_wait_busy();                  // Wait for the busy line to go low.
  /* Set RAM Y address */
  display_send_command(0x45);
  display_send_data(0x0f);
  display_send_data(0x01);
  display_send_data(0x00);
  display_send_data(0x00);
  display_wait_busy();                  // Wait for the busy line to go low.
  /* TODO: Find out what this is doing. */
  display_send_command(0x4E);
  display_send_data(0x00);
  display_send_command(0x4F);
  display_send_data(0x0f);
  display_send_data(0x01);

  vTaskDelay(100 / portTICK_PERIOD_MS);
  display_wait_busy();                  // Wait for the busy line to go low.
  
  /* TODO: Find out what this is doing. */
  display_send_command(0x91);
  display_send_data(0x00);

  display_send_command(0xC4);
  display_send_data(0x31);
  display_send_data(0x00);

  display_send_command(0xC5);
  display_send_data(0x0F);
  display_send_data(0x01);
  display_send_data(0x00);
  display_send_data(0x00);

  display_send_command(0xCE);
  display_send_data(0x31);
  display_send_command(0xCF);
  display_send_data(0x0f);
  display_send_data(0x01);

  // Setup 4Gray lookup tables?
  display_send_command(0x32);
  for(int i=0; i < 227; i++){
    display_send_data(LUT_DATA_4Gray[i]);
  }

  display_send_command(0x3f);
  display_send_data(LUT_DATA_4Gray[227]);

  display_send_command(0x03);
  display_send_data(LUT_DATA_4Gray[228]);

  display_send_command(0x04);
  display_send_data(LUT_DATA_4Gray[229]);
  display_send_data(LUT_DATA_4Gray[230]);
  display_send_data(LUT_DATA_4Gray[231]);

  display_send_command(0x2C);
  display_send_data(LUT_DATA_4Gray[232]);

}

/** display_init_partial
 *    Initialize the display in paritial update mode.
 *    Parameters: void
 *    Returns:    void
 */
void display_init_partial()
{
  display_reset();
  display_wait_busy();
  display_send_command(0x3C);
  display_send_data(0x80);
}


void draw_partial_display(uint32_t x_start, uint32_t y_start, uint32_t x_end, uint32_t y_end)
{
  /* NOTE: x axis must be a multiple of 4. */
  uint32_t update_width, width1, width2, update_height;
  uint8_t display_byte, value, temp;
  x_start = x_start - (x_start % 4);
  x_end = x_end + (x_end % 4);
  uint32_t half_width = (DISPLAY_WIDTH % 16 == 0) ? (DISPLAY_WIDTH / 16) : (DISPLAY_WIDTH / 16 + 1);
  uint32_t full_width = (DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8) : (DISPLAY_WIDTH / 8 + 1);
  update_width=((x_end - x_start) % 8 == 0)?((x_end - x_start) / 8 ):((x_end - x_start) / 8 + 1);
  update_height= y_end - y_start;

  display_send_command(0x22); 
  display_send_data(0xc0);   
  display_send_command(0x20); 
  display_wait_busy(); 

  display_send_command(0x11);
  display_send_data(0x01);

  display_send_command(0x91);
  display_send_data(0x00);

  // If the start is only in the right half.
  if(x_start>395) {
    width1 = 0;
    width2 = update_width;
    uint32_t y_partial_start = DISPLAY_HEIGHT - y_start;
    uint32_t y_partial_end = DISPLAY_HEIGHT - y_end;

    display_send_command(0xC4);							 // Set Ram X- address Start / End position
    display_send_data((792-x_start)/8);     						 // XStart, POR = 00h
    display_send_data((792-x_end)/8); //400/8-1
    display_send_command(0xC5);	 									// Set Ram Y- address  Start / End position 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	
    display_send_data(y_partial_end % 256);     									
    display_send_data(y_partial_end / 256);											

    display_send_command(0xCE);	 						 
    display_send_data((792-x_start)/8);	
    display_send_command(0xCF);	 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	

    display_send_command(0xA4);
    for(uint32_t i = 0; i < update_height; i++){
      for(uint32_t j = 0; j < update_width + 1; j++){
        display_byte = 0x00;
        for(uint8_t k = 0; k < 2; k++){
          value = display_buffer[(((i + y_start) * full_width) + (x_start / 8) + j - 1) * 2 + k];
          for(uint8_t l = 0; l < 4; l++){
            temp = value & 0x80;
            if(temp == 0x80){
              display_byte |= 0x01;
            }

            if(k != 1 || l != 3){
              display_byte <<= 1;
            }
            value <<= 2;
          }
        }
        display_send_data(display_byte);
      }
    }
  // If the partial update is only in the left half.
  } else if(x_end<396) {
    width1 = update_width;
    width2 = 0;
    uint32_t y_partial_start = DISPLAY_HEIGHT - y_start;
    uint32_t y_partial_end = DISPLAY_HEIGHT - y_end;

    display_send_command(0x44);	 						 // Set Ram X- address Start / End position
    display_send_data(x_start/8);     						
    display_send_data((x_start/8+width1)); 
    display_send_command(0x45);	 									// Set Ram Y- address  Start / End position 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	
    display_send_data(y_partial_end % 256);     									
    display_send_data(y_partial_end / 256);											

    display_send_command(0x4e);	 						 
    display_send_data(x_start / 8);	
    display_send_command(0x4f);	 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	

    display_send_command(0x24);
    for(uint32_t i = 0; i < update_height; i++){
      for(uint32_t j = 0; j < update_width + 1; j++){
        display_byte = 0x00;
        for(uint8_t k = 0; k < 2; k++){
          value = display_buffer[(((i + y_start) * full_width) + (x_start / 8) + j) * 2 + k];
          for(uint8_t l = 0; l < 4; l++){
            // For some reason this needs to be a temp variable and I don't know why....
            temp = value & 0x80;
            if(temp == 0x80){
              display_byte |= 0x01;
            }
            if(k != 1 || l != 3){
              display_byte <<= 1;
            }
            value <<= 2;
          }
        }
        display_send_data(display_byte);
      }
    }
  } else {

    // The width in the first screen
    width1 = ((396 - x_start) % 8 == 0)?((396 - x_start) / 8 ):((396 - x_start) / 8 + 1);
    // The width in the second screen
    width2 = ((x_end - 395) % 8 == 0)?((x_end - 395) / 8 ):((x_end - 395) / 8 + 1);
    uint32_t y_partial_start = DISPLAY_HEIGHT - y_start;
    uint32_t y_partial_end = DISPLAY_HEIGHT - y_end;

    display_send_command(0x44);	 						 // Set Ram X- address Start / End position
    display_send_data(x_start/8);     						
    display_send_data((x_start/8) + width1); 
    display_send_command(0x45);	 									// Set Ram Y- address  Start / End position 
    display_send_data((y_partial_start - 1)% 256);  
    display_send_data((y_partial_start - 1)/ 256);  
    display_send_data(y_partial_end % 256);     								
    display_send_data(y_partial_end / 256);										

    display_send_command(0x4e);	 						 
    display_send_data(x_start/8);     						
    display_send_command(0x4f);	 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	

    display_send_command(0x24);
    for(uint32_t i = 0; i < update_height; i++){
      for(uint32_t j = 0; j < width1; j++){
        display_byte = 0x00;
        for(uint8_t k = 0; k < 2; k++){
          value = display_buffer[(((i + y_start) * full_width) + (x_start / 8) + j) * 2 + k];
          // value = display_buffer[(((i + y_start) * full_width) + x_start + j) * 2 + k];
          for(uint8_t l = 0; l < 4; l++){
            // For some reason this needs to be a temp variable and I don't know why....
            temp = value & 0x80;
            if(temp == 0x80){
              display_byte |= 0x01;
            }

            if(k != 1 || l != 3){
              display_byte <<= 1;
            }
            value <<= 2;
          }
        }
        display_send_data(display_byte);
      }
    }
    display_send_command(0xC4);							 // Set Ram X- address Start / End position
    display_send_data(half_width - 1);     						 // XStart, POR = 00h
    display_send_data(half_width - width2); //400/8-1
    display_send_command(0xC5);	 									// Set Ram Y- address  Start / End position 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1) / 256);  
    display_send_data(y_partial_end % 256);     									
    display_send_data(y_partial_end / 256);											

    display_send_command(0xCE);	 						 
    display_send_data(half_width - 1);     						 // XStart, POR = 00h
    // display_send_data(792/8);     						 // XStart, POR = 00h
    display_send_command(0xCf);	 
    display_send_data((y_partial_start - 1) % 256);  
    display_send_data((y_partial_start - 1)/ 256); 	

    display_send_command(0xA4);
    for(uint32_t i = 0; i < update_height; i++){
      for(uint32_t j = 0; j < width2; j++){
        display_byte = 0x00;
        for(uint8_t k = 0; k < 2; k++){
          value = display_buffer[(((i + y_start) * full_width) + j + half_width - 1) * 2 + k];
          for(uint8_t l = 0; l < 4; l++){
            temp = value & 0x80;
            if(temp == 0x80){
              display_byte |= 0x01;
            }

            if(k != 1 || l != 3){
              display_byte <<= 1;
            }
            value <<= 2;
          }
        }
        display_send_data(display_byte);
      }
    }

    /*
    display_send_command(0xA6);
    for(uint32_t i = 0; i < update_height; i++){
      for(uint32_t j = 0; j < width2; j++){
        display_byte = 0x00;
        for(uint8_t k = 0; k < 2; k++){
          value = display_buffer[(((i + y_start) * full_width) + x_start + j + half_width - 1) * 2 + k];
          for(uint8_t l = 0; l < 4; l++){
            temp = value & 0x40;
            if(temp == 0x40){
              display_byte |= 0x01;
            }
            if(k != 1 || l != 3){
              display_byte <<= 1;
            }
            value <<= 2;
          }
        }
        display_send_data(display_byte);
      }
    }
  */
  }

  display_turn_on_partial();
}



void draw_full_display()
{
  uint8_t display_byte, value, temp;
  uint32_t half_width = (DISPLAY_WIDTH % 16 == 0) ? (DISPLAY_WIDTH / 16) : (DISPLAY_WIDTH / 16 + 1);
  uint32_t full_width = (DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8) : (DISPLAY_WIDTH / 8 + 1);
  uint32_t height = DISPLAY_HEIGHT;
 
  display_send_command(0x24);
  for(uint32_t i = 0; i < height; i++){
    for(uint32_t j = 0; j < half_width; j++){
      display_byte = 0x00;
      for(uint8_t k = 0; k < 2; k++){
        value = display_buffer[((i * full_width) +j) * 2 + k];
        for(uint8_t l = 0; l < 4; l++){
          // For some reason this needs to be a temp variable and I don't know why....
          temp = value & 0x80;
          if(temp == 0x80){
            display_byte |= 0x01;
          }

          if(k != 1 || l != 3){
            display_byte <<= 1;
          }
          value <<= 2;
        }
      }
      display_send_data(display_byte);
    }
  }

  display_send_command(0x26);
  for(uint32_t i = 0; i < height; i++){
    for(uint32_t j = 0; j < half_width; j++){
      display_byte = 0x00;
      for(uint8_t k = 0; k < 2; k++){
        value = display_buffer[((i * full_width) +j) * 2 + k];
        for(uint8_t l = 0; l < 4; l++){
          temp = value & 0x40;
          if(temp == 0x40){
            display_byte |= 0x01;
          }

          if(k != 1 || l != 3){
          // if(k != 1){
            display_byte <<= 1;
          }
          value <<= 2;
        }
      }
      display_send_data(display_byte);
      // display_send_data(display_buffer[(i * full_width) + j]);
    }
  }

  display_send_command(0xA4);
  for(uint32_t i = 0; i < height; i++){
    for(uint32_t j = 0; j < half_width; j++){
      display_byte = 0x00;
      for(uint8_t k = 0; k < 2; k++){
        value = display_buffer[((i * full_width) + j + half_width - 1) * 2 + k];
        for(uint8_t l = 0; l < 4; l++){
          temp = value & 0x80;
          if(temp == 0x80){
            display_byte |= 0x01;
          }

          if(k != 1 || l != 3){
          // if(k != 1){ 
            display_byte <<= 1;
          }
          value <<= 2;
        }
      }
      display_send_data(display_byte);
      // display_send_data(display_buffer[(i * full_width) + j + half_width - 1]);
    }
  }

  display_send_command(0xA6);
  for(uint32_t i = 0; i < height; i++){
    for(uint32_t j = 0; j < half_width; j++){
      display_byte = 0x00;
      for(uint8_t k = 0; k < 2; k++){
        value = display_buffer[((i * full_width) +j + half_width - 1) * 2 + k];
        for(uint8_t l = 0; l < 4; l++){
          temp = value & 0x40;
          if(temp == 0x40){
            display_byte |= 0x01;
          }
          if(k != 1 || l != 3){
          // if(k != 1){
            display_byte <<= 1;
          }
          value <<= 2;
        }
      }
      display_send_data(display_byte);
      // uint8_t display_value = display_buffer[(i * full_width) + j + half_width - 1];
      // display_send_data(~(display_value & ~display_value));
    }
  }

  display_turn_on_full();

}
void display_clear()
{
  uint32_t i;
  uint32_t max_data = 13600;
  display_send_command(0x24);
  for(i=0; i<max_data; i++) 
  {
      display_send_data(0xff);
  }

  display_send_command(0x26);
  for(i=0; i<max_data; i++)	
  {
      display_send_data(0x00);
  }

  // S part 396*272
  display_send_command(0xA4);
  for(i=0; i<max_data; i++) 
  {
      display_send_data(0xff);
  }

  display_send_command(0xA6);
  for(i=0; i<max_data; i++)	
  {
      display_send_data(0x00);
  }

  display_turn_on_full();
}

void display_reset()
/* Toggle the display reset pin */
{
  gpio_put(DISPLAY_RST, 1);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  gpio_put(DISPLAY_RST, 0);
  vTaskDelay(1 / portTICK_PERIOD_MS);
  gpio_put(DISPLAY_RST, 1);
  vTaskDelay(50 / portTICK_PERIOD_MS);
}

void display_turn_on_full()
{
  display_send_command(0x22);
  // display_send_data(0xCF);
  display_send_data(0xCF);
  display_send_command(0x20);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  display_wait_busy();
}

void display_turn_on_partial()
{
  display_send_command(0x22);
  display_send_data(0xFF);
  display_send_command(0x20);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  display_wait_busy();
}

void display_wait_busy()
{
  while(true){
    if(gpio_get(DISPLAY_BUSY) == 0){
      break;
    }
    vTaskDelay(10);
  }
}

void display_send_data(uint8_t data)
{
  gpio_put(DISPLAY_CS, 0);
  gpio_put(DISPLAY_DC, 1);
  spi_write_blocking(spi1, &data, 1);
  gpio_put(DISPLAY_CS, 1);
}

void display_send_command(uint8_t command)
{
  gpio_put(DISPLAY_DC, 0);
  gpio_put(DISPLAY_CS, 0);
  spi_write_blocking(spi1, &command, 1);
  gpio_put(DISPLAY_CS, 1);
}

void display_sleep()
{
  display_send_command(0x10);
  display_send_data(0x01);
}

void superimpose_image(uint16_t x_pos, uint16_t y_pos, uint8_t* image, uint16_t image_width, uint16_t image_height){
  /* Superimposes an image in the display_buffer.                        */
  /* NOTE: The origin of the display buffer is the top left of the scree */
  /* Parameters:                                                         */
  /*  - display_buffer: The display buffer to add the iamge              */
  /*  - x_pos: The x position to insert the image int the buffer         */
  /*  - y_pos: The y position to insert the image int the buffer         */
  /*  - image: The array of image data to superimpose                    */
  /*  - image_width: width of the image to superimpose                   */
  /*  - image_height: height of the image to superimpose                 */

  /* TODO: Make this function return enough information for paritial screen update */

  uint32_t buffer_index_with_pixel; /* The index in display_buffer containing the current pixel.               */
  uint8_t buffer_mask_offset;       /* The offset to apply to the buffer bitmask.                              */
  uint8_t buffer_pixel_mask;        /* The buffer bitmask to target pixel data.                                */

  uint32_t image_index_with_pixel;  /* The index in image containing the current pixel                         */
  uint8_t image_mask_offset;        /* The offset to apply to the image bitmask to isolate current pixel data. */
  uint8_t image_pixel_mask;         /* The image bitmask to target pixel data.                                 */

  int8_t mask_offset_difference;    /* The difference between the image mask offset and buffer mask offset used to shift the image data into place */

  uint8_t* pixel_to_modify = NULL;  /* Pointer to the display_buffer byte to modify. Used to avoid re-indexing the array many times...             */

  for(int i=0; i < image_height; i++){
    for(int j=0; j < image_width; j++){
      // Compute the offset of the target pixel data (2 bits) in the array byte.
      buffer_mask_offset = 2 * ((x_pos + j) % 4);
      image_mask_offset = 2 * (j % 4);
      mask_offset_difference = buffer_mask_offset - image_mask_offset;
      // Create the bitmasks used to isolate the target pixel data.
      buffer_pixel_mask = ~(0xC0 >> buffer_mask_offset);
      image_pixel_mask = 0xC0 >> image_mask_offset;
      // Isolate the bytes containing the pixel data in the display buffer and image.
      buffer_index_with_pixel = ((y_pos + i) * ((uint32_t)(DISPLAY_WIDTH / 4))) + ((uint32_t)((x_pos + j) / 4));
      image_index_with_pixel = (i * (uint32_t)(image_width / 4)) + (uint32_t)(j / 4);
      pixel_to_modify = &(display_buffer[buffer_index_with_pixel]);
      // Shift the masked image pixel data to fit the buffer bitmask.
      if(mask_offset_difference >= 0){
        *pixel_to_modify = ((image[image_index_with_pixel] & image_pixel_mask) >> mask_offset_difference) | (*pixel_to_modify & (buffer_pixel_mask));
      }else{
        *pixel_to_modify = ((image[image_index_with_pixel] & image_pixel_mask) << (mask_offset_difference * -1)) | (*pixel_to_modify & (buffer_pixel_mask));
      }
    }
  }
}
