/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#include <string.h>
#include <stdio.h>

#include "atmel_start.h"
#include "usb_start.h"

static usb_buffer_t usb_buffer;

#if CONF_USBD_HS_SP
static uint8_t single_desc_bytes[] = {
    /* Device descriptors and Configuration descriptors list. */
    CDCD_ACM_HS_DESCES_LS_FS};
static uint8_t single_desc_bytes_hs[] = {
    /* Device descriptors and Configuration descriptors list. */
    CDCD_ACM_HS_DESCES_HS};
#define CDCD_ECHO_BUF_SIZ CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ_HS
#else
static uint8_t single_desc_bytes[] = {
    /* Device descriptors and Configuration descriptors list. */
    CDCD_ACM_DESCES_LS_FS};
#define CDCD_ECHO_BUF_SIZ CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ
#endif

static struct usbd_descriptors single_desc[]
    = {{single_desc_bytes, single_desc_bytes + sizeof(single_desc_bytes)}
#if CONF_USBD_HS_SP
       ,
       {single_desc_bytes_hs, single_desc_bytes_hs + sizeof(single_desc_bytes_hs)}
#endif
};

/** Buffers to receive and echo the communication bytes. */
static uint32_t usbd_cdc_buffer[CDCD_ECHO_BUF_SIZ / 4];

/** Ctrl endpoint buffer */
static uint8_t ctrl_buffer[64];

/**
 * \brief Callback invoked when bulk OUT data received
 */
static bool usb_device_cb_bulk_out(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{

	/* No error. */
	return false;
}

/**
 * \brief Callback invoked when bulk IN data received
 */
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

/**
 * \brief Callback invoked when Line State Change
 */
static bool usb_device_cb_state_c(usb_cdc_control_signal_t state)
{
	if (state.rs232.DTR) {
		/* Callbacks must be registered after endpoint allocation */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_out);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_in);
		/* Start Rx */
		cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer));
	}

	/* No error. */
	return false;
}

/**
 * \brief CDC ACM Init
 */
void cdc_device_acm_init(void)
{
	/* usb stack init */
	usbdc_init(ctrl_buffer);

	/* usbdc_register_funcion inside */
	cdcdf_acm_init();

	usbdc_start(single_desc);
	usbdc_attach();
}

/**
 * Example of using CDC ACM Function.
 * \note
 * In this example, we will use a PC as a USB host:
 * - Connect the DEBUG USB on XPLAINED board to PC for program download.
 * - Connect the TARGET USB on XPLAINED board to PC for running program.
 * The application will behave as a virtual COM.
 * - Open a HyperTerminal or other COM tools in PC side.
 * - Send out a character or string and it will echo the content received.
 */
void cdcd_acm_example(void)
{
	while (!cdcdf_acm_is_enabled()) {
		// wait cdc acm to be installed
	};

	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)usb_device_cb_state_c);

	while (1) {
	}
}

void usb_init(void)
{

	cdc_device_acm_init();
}


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