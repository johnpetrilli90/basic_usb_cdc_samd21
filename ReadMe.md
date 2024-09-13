# Basic USB CDC Communication with SAMD21 Xplained Pro
Takes the Atmel Start USB CDC echo example and makes a few changes to the files `usb_start.c` and `usb_start.h` to allow for bi-directional communication.

This program will allow the user to send the command `rr` and get a valid response from the microcontroller. Any other command sent will result in a reponse saying invalid. 

The terminating character is `\n`. 

## Changes to usb_start.h
1. Added a few defines to hold rx and tx size as well as an example command
```
#define RX_BUFFER_SIZE	16
#define TX_BUFFER_SIZE  32
#define READ_REG_CMD	"rr"
#define READ_REG_SIZE	2
```
2. Added struct to hold an rx buffer and the buffer index
```
struct _config_usb_buffer{
	uint8_t rx[RX_BUFFER_SIZE];
	uint8_t rx_idx;	
};

typedef struct _config_usb_buffer usb_buffer_t;
```

3. Added a function declaration called `void process_command(const char* rx);`

## Changes to usb_start.c
1. Added `#include <string.h>`
2. Added usb buffer data structure to the file: `static usb_buffer_t usb_buffer`
3. Modified the read and write callbacks. They were 'crossed over' and they have been modifed to be 'straight thru'
```
cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_out);
cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_in);
```
4. Updated the usb receive callback to do a little parsing
```
static bool usb_device_cb_bulk_in(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	uint8_t temp_buf[RX_BUFFER_SIZE];
	
	memcpy((void*)temp_buf, (void*)usbd_cdc_buffer, RX_BUFFER_SIZE);
	
	for(uint8_t i=0; i<count; i++){
		if(usb_buffer.rx_idx >= RX_BUFFER_SIZE)
			usb_buffer.rx_idx = 0;
			
		usb_buffer.rx[usb_buffer.rx_idx] = temp_buf[i];
		if(usb_buffer.rx[usb_buffer.rx_idx] == '\n'){
			process_command((const char*)usb_buffer.rx);
			usb_buffer.rx_idx = 0;
		}
		else{
			usb_buffer.rx_idx++;
		}
	}
	
	cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer));		//re-arm the rx irq

	/* No error. */
	return false;
}
```
5. Wrote a very basic process command fucntion as an example
```
void process_command(const char* command_buf){
	static char tx_msg[TX_BUFFER_SIZE];
	uint8_t len;
	
	if(!strncmp((const char*)command_buf, (const char*)READ_REG_CMD, READ_REG_SIZE)){
		len = sprintf(tx_msg, "rx cmd: %s\r\n", command_buf);
	}
	else{
		len = sprintf(tx_msg, "%s is an invalid cmd\r\n", command_buf);
	}
	
	cdcdf_acm_write((uint8_t*)tx_msg, len);
}
```