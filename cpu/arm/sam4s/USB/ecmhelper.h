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

#ifndef __ECM_HELPER_H
#define __ECM_HELPER_H

#include <stdint.h>

#define USB_FS_MAX_PACKET_SIZE   64

#define ECM_MAX_USB_DATA_PACKET_SIZE   USB_FS_MAX_PACKET_SIZE
#define ECM_MAX_USB_NOTIFY_PACKET_SIZE USB_FS_MAX_PACKET_SIZE
#define ECM_MAX_SEGMENT_SIZE           1514

struct ecm_interface
{
	usb_iface_desc_t             ctl_interface;
	usb_cdc_hdr_desc_t cdc_ecm_header;
	usb_cdc_union_desc_t  cdc_ecm_union;
	cdc_enet_functional_desc_t   cdc_ecm_functional;
	usb_ep_desc_t              ctl_ep;
	usb_iface_desc_t             dat_interface;
	usb_ep_desc_t              ep_in;
	usb_ep_desc_t              ep_out;
};

/* macro to help generate CDC ECM USB descriptors */

#define ECM_DESCRIPTOR(NOTIFY_ITF, DATA_ITF, NOTIFY_EP, DATAOUT_EP, DATAIN_EP, IMAC_STRING) \
    { \
      .ctl_interface = { \
        /*Interface Descriptor */ \
        .bLength            = sizeof(usb_iface_desc_t),             /* Interface Descriptor size */ \
        .bDescriptorType    = USB_DT_INTERFACE,                        /* Interface */ \
        .bInterfaceNumber   = NOTIFY_ITF,                                      /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x01,                                            /* One endpoints used */ \
        .bInterfaceClass    = CDC_CLASS_COMM,                                  /* Communication Interface Class */ \
        .bInterfaceSubClass = CDC_SUBCLASS_ETH,                                /* Ethernet Control Model */ \
        .bInterfaceProtocol = 0x00,                                            /* No specific protocol */ \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .cdc_ecm_header = { \
        /*Header Functional Descriptor*/ \
        .bFunctionLength    = sizeof(usb_cdc_hdr_desc_t), /* bLength: Endpoint Descriptor size */ \
        .bDescriptorType    = CDC_CS_INTERFACE,                                /* bDescriptorType: CS_INTERFACE */ \
        .bDescriptorSubtype = 0x00,                                            /* bDescriptorSubtype: Header Func Desc */ \
        .bcdCDC             = LE16(0x0120),                              /* bcdCDC: spec release number */ \
      }, \
 \
      .cdc_ecm_union = { \
        /*Union Functional Descriptor*/ \
        .bFunctionLength    = sizeof(usb_cdc_union_desc_t),  \
        .bDescriptorType    = CDC_CS_INTERFACE,                                /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x06,                                            /* Union func desc */ \
        .bMasterInterface   = NOTIFY_ITF,                                      /* Notify interface */ \
        .bSlaveInterface0   = DATA_ITF,                                        /* Data Interface */ \
      }, \
 \
      .cdc_ecm_functional = { \
        /*Ethernet Networking Functional Descriptor*/ \
        .bFunctionLength    = sizeof(cdc_enet_functional_desc_t),   \
        .bDescriptorType    = CDC_CS_INTERFACE,                                /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x0F,                                            /* Ethernet Networking */ \
        .iMACAddress        = IMAC_STRING,                                     /* mandatory string providing MAC address */ \
        .bmEthernetStatistics = {0,0,0,0},                                     \
        .wMaxSegmentSize    = LE16(ECM_MAX_SEGMENT_SIZE),                /* in bytes, usually 1514 */ \
        .wMCFilters         = LE16(0),                                   /* no filters */ \
        .bNumberPowerFilters = 0,                                              /* no filters */ \
      }, \
 \
      .ctl_ep = { \
        /* Notify Endpoint Descriptor*/ \
        .bLength            = sizeof(usb_ep_desc_t),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_DT_ENDPOINT,                         /* Endpoint */ \
        .bEndpointAddress   = NOTIFY_EP,                                       \
        .bmAttributes       = 0x03,                                            /* Interrupt */ \
        .wMaxPacketSize     = LE16(ECM_MAX_USB_NOTIFY_PACKET_SIZE),      \
        .bInterval          = 0xFF,                                            \
      }, \
 \
      .dat_interface = { \
        /*Data class interface descriptor*/ \
        .bLength            = sizeof(usb_iface_desc_t),             /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_DT_INTERFACE,                        \
        .bInterfaceNumber   = DATA_ITF,                                        /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x02,                                            /* Two endpoints used */ \
        .bInterfaceClass    = CDC_CLASS_DATA,                                  /* CDC */ \
        .bInterfaceSubClass = 0x00,                                            \
        .bInterfaceProtocol = 0x00,                                            \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .ep_in = { \
        /* Data Endpoint OUT Descriptor */ \
        .bLength            = sizeof(usb_ep_desc_t),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_DT_ENDPOINT,                         /* Endpoint */ \
        .bEndpointAddress   = DATAOUT_EP,                                      \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = LE16(ECM_MAX_USB_DATA_PACKET_SIZE),        \
        .bInterval          = 0x00,                                            /* ignore for Bulk transfer */ \
      }, \
 \
      .ep_out = { \
        /* Data Endpoint IN Descriptor*/ \
        .bLength            = sizeof(usb_ep_desc_t),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_DT_ENDPOINT,                         /* Endpoint */ \
        .bEndpointAddress   = DATAIN_EP,                                       \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = LE16(ECM_MAX_USB_DATA_PACKET_SIZE),        \
        .bInterval          = 0x00                                             /* ignore for Bulk transfer */ \
      }, \
    },

#endif /* __ECM_HELPER_H */
