/**
 * \file
 *
 * \brief Default descriptors for a USB Device with a single interface CDC
 *
 * Copyright (c) 2009-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "conf_usb.h"
#include "udd.h"
#include "udc_desc.h"
#include "udi_cdc.h"

#include "ecmhelper.h"
/**
 * \defgroup udi_cdc_group_single_desc USB device descriptors for a single interface
 *
 * The following structures provide the USB device descriptors required for
 * USB Device with a single interface CDC.
 *
 * It is ready to use and do not require more definition.
 *
 * @{
 */

/*
    USB descriptor macros for CDC-ECM

    Copyright (C) 2015,2016,2018 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/



#include <stdint.h>

#include "conf_usb.h"



//! Structure for USB Device Configuration Descriptor
COMPILER_PACK_SET(1)
typedef struct {
	usb_conf_desc_t conf;
	struct ecm_interface                             ecm;
} udc_desc_t;
COMPILER_PACK_RESET()

enum
{
	USB_ECM_EP_SEND = 1,
	USB_ECM_EP_RECV = 2,
	USB_ECM_EP_COMM = 3,
};


#define USB_FS_MAX_PACKET_SIZE   64

#define ECM_MAX_USB_DATA_PACKET_SIZE   USB_FS_MAX_PACKET_SIZE
#define ECM_MAX_USB_NOTIFY_PACKET_SIZE USB_FS_MAX_PACKET_SIZE
#define ECM_MAX_SEGMENT_SIZE           1514



#define  USB_DEVICE_NB_INTERFACE       (2*UDI_CDC_PORT_NB)

#ifdef USB_DEVICE_LPM_SUPPORT
# define USB_VERSION   USB_V2_1
#else
# define USB_VERSION   USB_V2_0
#endif

enum
{
	USB_STR_ZERO,
	USB_STR_MANUFACTURER,
	USB_STR_PRODUCT,
	USB_STR_SERIAL_NUMBER,
	USB_STR_IMAC_ADDR,
	USB_STR_COUNT,
};

//! USB Device Descriptor
COMPILER_WORD_ALIGNED
usb_dev_desc_t usb_device_descriptor =
{
	.bLength            = sizeof(usb_dev_desc_t),
	.bDescriptorType    = USB_DT_DEVICE,
	.bcdUSB             = 0x0200,
	.bDeviceClass       = 0,
	.bDeviceSubClass    = 0,
	.bDeviceProtocol    = 0,
	.bMaxPacketSize0    = USB_DEVICE_EP_CTRL_SIZE,
	.idVendor           = LE16(USB_DEVICE_VENDOR_ID),
	.idProduct          = LE16(USB_DEVICE_PRODUCT_ID),
	.bcdDevice          = 0x0100,
	.iManufacturer      = USB_STR_MANUFACTURER,
	.iProduct           = USB_STR_PRODUCT,
	.iSerialNumber      = USB_STR_SERIAL_NUMBER,
	.bNumConfigurations = 1,
};


//! USB Device Configuration Descriptor filled for full and high speed
COMPILER_WORD_ALIGNED
UDC_DESC_STORAGE udc_desc_t udc_desc_fs = {
	.conf =
	{
		.bLength             = sizeof(usb_conf_desc_t),
		.bDescriptorType     = USB_DT_CONFIGURATION,
		.wTotalLength        = LE16(sizeof(udc_desc_t)),
		.bNumInterfaces      = 2,
		.bConfigurationValue = 1,
		.iConfiguration      = 0,
		.bmAttributes        = 0x80,
		.bMaxPower           = 50, // 100 mA
	},

	ECM_DESCRIPTOR(
			/* Notify ITF */ 0x00,
			/* Data ITF */ 0x01,
			/* Notify EP */ USB_EP_DIR_IN | USB_ECM_EP_COMM,
			/* DataOut EP */ USB_EP_DIR_OUT | USB_ECM_EP_RECV,
			/* DataIn EP */ USB_EP_DIR_IN | USB_ECM_EP_SEND,
			/* iMACstring */ USB_STR_IMAC_ADDR)

};



/**
 * \name UDC structures which content all USB Device definitions
 */
//@{

//! Associate an UDI for each USB interface
UDC_DESC_STORAGE udi_api_t *udi_apis[USB_DEVICE_NB_INTERFACE] = {
#  define UDI_CDC_API(index, unused) \
	&udi_api_cdc_comm, \
	&udi_api_cdc_data,
	MREPEAT(UDI_CDC_PORT_NB, UDI_CDC_API, ~)
#  undef UDI_CDC_API
};

//! Add UDI with USB Descriptors FS & HS
UDC_DESC_STORAGE udc_config_speed_t udc_config_fs[1] = { {
	.desc          = (usb_conf_desc_t UDC_DESC_STORAGE*)&udc_desc_fs,
	.udi_apis = udi_apis,
}};


//! Add all information about USB Device in global structure for UDC
UDC_DESC_STORAGE udc_config_t udc_config = {
	.confdev_lsfs = &usb_device_descriptor,
	.conf_lsfs = udc_config_fs,
	.conf_bos = NULL,
};

//@}
//@}
