#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#define DIGIT_BUFFER_SIZE 6000

static char *s_days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static char *s_months[12] = {"January", 
          "February", 
          "March", 
          "April", 
          "May", 
          "June", 
          "July", 
          "August", 
          "September", // 9 + 9
          "October", 
          "November", 
          "December"};

void superimpose_text_on_display(uint16_t x_start, uint16_t y_start, char *text_to_display, char *file_format_string, uint16_t x_padding);

/** update_display_time
 * Updates the time on the display using a datetime_t object
 */ 
void update_display_time(datetime_t new_time, bool full_update);

void update_display_date(datetime_t current_time);

#endif
