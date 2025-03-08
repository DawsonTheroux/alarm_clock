#ifndef CHIPCOMMS_I2C_DEVICE_H 
#define CHIPCOMMS_I2C_DEVICE_H 

#define I2C0_SCL 25
#define I2C0_SDA 26

// Max of uint8 + 1.
#define CC_I2C_DEVICE_RX_BUFFER_SIZE 10

/* !! Transactions should always be 1 byte */
typedef struct cc_i2c_rx_transaction_t{
  uint8_t* rx_buffer;
  uint8_t rx_len;
}cc_i2c_rx_transaction_t;

void cc_i2c_rx_task(void* args);

#endif
