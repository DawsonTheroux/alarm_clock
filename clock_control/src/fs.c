#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "flash.h"
#include "fs.h"

// RULES:
//  DIRECTORIES SHOULD NOT EXCEED ONE PAGE (128 entries)
//  PATHS SHOULD START WITH A '/'

/*
* This is in implementation of a custom file system.
*
*
* Modify read_flash and write_flash with the device read funcitons.
*/

/*
 * Flash read function
 * In this case, does 4 SD card reads to read the full 2048 block size for the file system.
 *
 * Input address is in FS pages
*/
static uint8_t read_flash(uint32_t address, uint32_t length, uint8_t *read_buffer)
{
  uint8_t read_token;
  uint32_t sd_block_address = address / SD_BLOCK_LEN;
  uint32_t number_of_blocks = (length > SD_BLOCK_LEN) ? (length / SD_BLOCK_LEN) : 0;

  // Use a temporary buffer to read an SD block, then copy the memory to the read_buffer.
  uint8_t tmp_buffer[SD_BLOCK_LEN];
  for(int i=0; i<number_of_blocks + 1; i++){
    if(flash_read_block(sd_block_address + i, tmp_buffer, &read_token)){
      return -1;
    }
    memcpy(read_buffer + (i * SD_BLOCK_LEN), tmp_buffer, ((SD_BLOCK_LEN * (i + 1)) < length) ? SD_BLOCK_LEN : length);
    length -= SD_BLOCK_LEN;
  }
  return 0;
}

/*
* Flash write function
*/
static uint8_t write_flash(uint32_t address, uint8_t *write_buffer)
{
  uint8_t write_token;
  return flash_read_block(address, write_buffer, &write_token);
}


/*
 * Gets the length of the root component in the file path.
 */
static uint8_t root_component_length(char* filepath, uint8_t *component_length)
{
  *component_length = 0;
  for(int i=0; i<strlen(filepath); i++){
    if(filepath[i] == '/'){
      break;
    }
    (*component_length)++;
  }
}

/*
 * Reads the directory from the page index. 
 * The length of the directory and the data are returned through `directory_length` and `directory_data`
 */
static uint8_t read_directory(uint32_t directory_index, uint8_t *directory_data, uint32_t *directory_length)
{
  if(read_flash(directory_index, FS_PAGE_SIZE, directory_data)){
    printf("Failed to read flash during read_directory with: (page index: %u)\r\n", directory_index);
    return -1;
  }
  *directory_length = *(uint32_t*)(directory_data+2);
  return 0;
}

/*
 * Finds the entry with `filename` in the provided directory_buffer. 
 * The pointer to the entry is returned via `directory_entry_pointer`.
 */
static uint8_t find_file_in_directory(uint8_t *directory_buffer, char *filename, uint8_t name_length, uint32_t *entry_address, uint32_t *entry_length, char* name, char* extension)
{
  // Seperate the name into the name and the extension.
  uint8_t extension_index = 0;
  bool found_index = false;

  // Reset return parameters.
  *entry_length = 0;
  memset(name, 0, DIR_NAME_LEN + 1);
  memset(extension, 0, DIR_EXTENSION_LEN + 1);

  // Look for the index with a '.'
  for(uint8_t i=0; i<name_length; i++){
    if(filename[i] == '.'){
      name[i] = '\0';
      found_index = true;
      continue;
    }
    if(found_index == false){
      name[i] = filename[i];
    }else{
      extension[extension_index++] = filename[i];
    }
  }
  // If no extension was found, set the extension to the directory extension.
  if(extension_index == 0){
    memset(extension, 0xFF, 3);
    name[name_length] = '\0';
  }
  extension[3] = '\0';
  // Loop through all the entries (length of buffer / DIR_ENTRY_LEN)
  // Bytes 2-6 is the length of the directory entry.
  // Start at 16 to skip the first entry
  for(int i=16; i<(*(uint32_t*)(directory_buffer+2)); i += 16){
    if(strncmp(name, directory_buffer+i+6, DIR_NAME_LEN) == 0){
      if(strncmp(extension, directory_buffer+i+6 + DIR_NAME_LEN, DIR_EXTENSION_LEN) == 0){
        *entry_address = ((uint32_t)(*((uint16_t*)(directory_buffer+i)))) * FS_PAGE_SIZE;
        // For some reason, casting to uint32_t and dereferencing doesn't work.
        // *entry_length = *((uint32_t*)(directory_buffer+i+2));
        for(int j=0; j<4; j++){
          (*entry_length) |= (((uint32_t)(directory_buffer[i+2+j])) << (j * 8));
        }
        return 0;
      }
    }
  }
  printf("Failed to find file:%s.%s inside directory\r\n", name, extension);
  return 1;
}

/*
* Read a file from the file system
*/
uint8_t read_file(char *filepath, uint8_t *read_buffer, uint32_t max_size, uint32_t *bytes_read)
{
  uint8_t directory_buffer[FS_PAGE_SIZE];
  uint32_t directory_size = 0;
  uint8_t name_length= 0;
  uint8_t *directory_entry_pointer;
  uint32_t entry_address = 0;
  uint32_t entry_length = 0; 
  char entry_name[8];
  char entry_extension[4];

  // Read the root directory data.
  if(read_directory(ROOT_DIR_OFFSET, directory_buffer, &directory_size)){
    printf("fs.c:read_file() - Failed to read root directory in \r\n");
    return -1;
  }

  while(directory_buffer != NULL){
    filepath++; // remove leading '/'
    // Get the root component and gets its information.
    root_component_length(filepath, &name_length);
    find_file_in_directory(directory_buffer, filepath, name_length, &entry_address, &entry_length, entry_name, entry_extension);
    filepath += name_length; // remove the root component from the path.
    // If the entry is a directory, then get the next entry for directory buffer.
    if(entry_extension[0] == 0xFF && entry_extension[1] == 0xFF && entry_extension[2] == 0xFF){
      if(read_directory(entry_address, directory_buffer, &directory_size)){
        printf("fs.c:read_file() - Failed to read directory with name: %s at offset %u\r\n", entry_name, entry_address);
        return -1;
      }
    }else{
      // read the file because its a file.
      if(entry_length > max_size){
        printf("fs.c:read_file() - Failed to read file...Entry length(%u) is larger than max_size(%u)\r\n", entry_length, max_size);
        return -1;
      }
      if(read_flash(entry_address, entry_length, read_buffer)){
        printf("fs.c:read_file() - Failed to read flash address: %u, length: %u for file with name: %s.%s\r\n", entry_address, entry_length, entry_name, entry_extension);
        return -1;
      }
      return 0;
    }
  }
  return 0;
}

/*
* write a file to the file system.
*/
uint8_t write_file(char* filepath, uint8_t* file_buffer, uint32_t file_size)
{
  printf("write_file not implemented yet\r\n");
  return false;
}

/*
* Get the size of a file
*/
uint8_t file_size(char* filepath, uint32_t* file_size)
{
  printf("file_size not implemented yet\r\n");
  return false;
}
