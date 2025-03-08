#ifndef FS_H
#define FS_H

#define FS_PAGE_SIZE 2048
#define DIR_ENTRY_LEN 16
#define DIR_NAME_LEN 7  
#define DIR_EXTENSION_LEN 3
#define ROOT_DIR_OFFSET 0x0

int read_file(char *filepath, uint8_t *read_buffer, uint32_t max_size, uint32_t *bytes_read);
int write_file(char* filepath, uint8_t* file_buffer, uint32_t file_size);
int file_size(char* filepath, uint32_t* file_size);

#endif
