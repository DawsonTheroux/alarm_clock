#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/util/datetime.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "images.h"
#include "fs.h"

#include "display.h"
#include "display_manager.h"

static int create_date_string(datetime_t date, char *date_string)
{
  /* Date string for Day of week, month, day, year */
  char *date_template = "%s, %s %d, %d";
  sprintf(date_string, date_template, s_days[date.dotw], s_months[date.month - 1], date.day, date.year);

  /* Date string for day of week - YYYY/MM/DD*/
  /*
  char *date_template = "%s - %02d-%02d-%02d";
  sprintf(date_string, date_template, s_days[date.dotw], date.year, date.month, date.day);
  */
  return 0;

}

/*
 * update display date
 */
void update_display_date(datetime_t current_time)
{
    printf("Update display date\n");

    char date_string[32];
    char char_path[64];
    uint8_t char_buffer[4650];
    uint32_t read_length = 0;

    uint16_t date_x_start;
    uint16_t date_y_start;
    uint16_t date_x_end;
    uint16_t date_y_end;

    uint16_t x_offset = DISPLAY_WIDTH;
    uint16_t y_offset = DISPLAY_HEIGHT - 5;

    // Create the string to print.
    create_date_string(current_time, date_string);
    for(int i=strlen(date_string) - 1; i>=0; i--){
        if(date_string[i] == ' '){
            x_offset -= char_buffer[1] + 10;
            continue;
        }
        sprintf(char_path, "/smlfnt/%c.CHR", date_string[i]);
        if (read_file(char_path, char_buffer, 4650, &read_length)) {
            printf("Failed to read char from path: %s\r\n", char_path);
            return;
        }
        x_offset -= char_buffer[1];

        // Clear the display on the first character read.
        if(i == strlen(date_string) - 1){
            clear_display((int)(DISPLAY_WIDTH / 3) - 5, 
                          y_offset - char_buffer[0], DISPLAY_WIDTH, 
                          DISPLAY_HEIGHT);
        }
        superimpose_image(x_offset, y_offset - char_buffer[2], char_buffer + 4, char_buffer[1], char_buffer[0]);
        x_offset -= char_buffer[3];
    }
}

/*
 * update the time in the display buffer with the one contained in the `new_time` parameter.
 * TODO: Change the function to only do a partial update if partial_update parameter is provided.
 */
void update_display_time(datetime_t new_time, bool partial_update)
{
    printf("Hello display time\n");
    uint8_t digits[4];
    uint32_t x_offset = DISPLAY_WIDTH;
    uint32_t y_offset = 0;
    uint32_t digit_index = 0;
    uint8_t digit_buffer[DIGIT_BUFFER_SIZE];
    uint32_t read_length = 0;
    uint16_t meridiem_width = 0;

    // Offsets that represent the window that the date fills..
    uint16_t date_x_start = 0;
    uint16_t date_y_start = 0;
    uint16_t date_x_end = 0;
    uint16_t date_y_end = 0;

    char char_path[14];
    // char *digit_format = "/lrgfnt/%c.CHR";
    char *digit_format = "/timfnt/%c.CHR";
    char *meridiem_format = "/smlfnt/%c.CHR";
    bool is_am = true;

    // Lear the right 2/3 of the display with the height of a 0 char.
    sprintf(char_path, digit_format, 48);
    if(read_file(char_path, digit_buffer, DIGIT_BUFFER_SIZE, &read_length)){
        printf("Failed to read char from path: %s\r\n", char_path);
        return;
    }
    y_offset = (DISPLAY_HEIGHT / 2) + (digit_buffer[0] / 2); // Bottom Justified.
    date_x_start = (int)(DISPLAY_WIDTH / 3) - 5;
    date_y_start = y_offset - digit_buffer[0] - 5;
    date_x_end = DISPLAY_WIDTH;
    date_y_end = y_offset +5;
    //clear_display((int)(DISPLAY_WIDTH / 3) - 5, y_offset - 5, DISPLAY_WIDTH, y_offset + digit_buffer[0] + 5);
    clear_display(date_x_start, date_y_start, date_x_end, date_y_end);

    // Determine if it is AM or PM and convert time to 12-hour time.
    is_am = new_time.hour < 12;
    digits[0] = (is_am || new_time.hour == 12) ? ((new_time.hour == 0) ? 1 : new_time.hour / 10) : ((new_time.hour % 12) / 10);
    digits[1] = (is_am || new_time.hour == 12) ? ((new_time.hour == 0) ? 2 : new_time.hour % 10) : ((new_time.hour % 12) % 10);
    digits[2] = new_time.min / 10;
    digits[3] = new_time.min % 10;

    // Super impose the meridiam.
    sprintf(char_path, meridiem_format, 'M');
    if(read_file(char_path, digit_buffer, DIGIT_BUFFER_SIZE, &read_length)){
        printf("Failed to read char from path %s\r\n", char_path);
        return;
    }
    x_offset  -= digit_buffer[1] + 2;
    superimpose_image(x_offset, date_y_start + 5, digit_buffer + 4, digit_buffer[1], digit_buffer[0]);
    sprintf(char_path, meridiem_format, (is_am) ? 'A' : 'P');
    if(read_file(char_path, digit_buffer, DIGIT_BUFFER_SIZE, &read_length)){
        printf("Failed to read char from path %s\r\n", char_path);
        return;
    }
    x_offset -= digit_buffer[1] + 2;
    meridiem_width = x_offset;
    superimpose_image(x_offset, date_y_start + 5, digit_buffer + 4, digit_buffer[1], digit_buffer[0]);


    // Loop through the hour and minute digits to apply them to the buffer.
    for(int i=0; i<4; i++){
        // Read the digit from the SD card using the digit_format path.
        sprintf(char_path, digit_format, 48 + digits[3 - i]);
        if(read_file(char_path, digit_buffer, DIGIT_BUFFER_SIZE, &read_length)){
            printf("Failed to read char from path: %s\r\n", char_path);
            return;
        }
        x_offset -= digit_buffer[1] + 10;
        // If the hour starts with a zero, don't write it.
        if(i == 3 && digits[3 - i] == 0){
            memset(digit_buffer+4, 0xFF, DIGIT_BUFFER_SIZE-4);
        }
        superimpose_image(x_offset, y_offset - digit_buffer[2], digit_buffer + 4, digit_buffer[1], digit_buffer[0]);
        // super impose the ':' semerator.
        if(i == 1){
            sprintf(char_path, digit_format, ':');
            if(read_file(char_path, digit_buffer, DIGIT_BUFFER_SIZE, &read_length)){
                printf("Failed to read char from path: %s\r\n", char_path);
                return;
            }
            x_offset -= digit_buffer[1] + 10;
            superimpose_image(x_offset, y_offset - digit_buffer[2], digit_buffer + 4, digit_buffer[1], digit_buffer[0]);
        }
    }
    x_offset -= digit_buffer[1] + 10;
    // TODO: CHange this to only do anything if partial_update is true.
    // For non-partial updates, another function will be called.
    if(partial_update){
        display_init_partial();
        draw_partial_display(date_x_start, date_y_start, date_x_end, date_y_end);
        display_sleep();
    }
}
