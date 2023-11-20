#include <stdint.h>

int init_usb();
void wait_for_usb();
int usb_rx_buffer_len();
int usb_read_bytes(uint8_t *data, uint32_t size);
int usb_tx_buffer_len();
int usb_write_bytes(const uint8_t *data, uint32_t size);
int printu(char *format, ...);
int printuln(char *format, ...);
