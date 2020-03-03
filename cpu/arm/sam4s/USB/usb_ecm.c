/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Peter Lawrence
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include "net/ip/uip.h"
#include "udi_cdc.h"
#include "usb_ecm.h"

#include "ip64.h"
#include "ip64-eth.h"
#include "ip64-arp.h"
#include <net/ipv4/uip_arp.h>
#include "net/ipv4/uip-neighbor.h"
#include <net/ipv4/uip-fw.h>
#include <net/ipv6/uip-ds6.h>

#include "EMC_drv.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define LOG6LBR_MODULE "USB"
#include "log-6lbr.h"

PROCESS(cdc_ecm_drv_process, "USB CDC-ECM");

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define ECM_MAX_SEGMENT_SIZE           1514

static COMPILER_WORD_ALIGNED uint8_t received[ECM_MAX_SEGMENT_SIZE];
static COMPILER_WORD_ALIGNED uint8_t transmitted[ECM_MAX_SEGMENT_SIZE];

static bool can_xmit;
static int collected;

void usb_ecm_init(void)
{
	process_start(&cdc_ecm_drv_process, NULL);
}

static void usb_ecm_send(uint8_t *data, int size)
{
	udi_cdc_write_buf(data, size);
}

static void usb_ecm_ep_recv_callback(int size)
{
	eth_drv_input(received, size);
}

static void usb_ecm_ep_send_callback(int size)
{
  (void)size;

  can_xmit = true;
}

void rx_poll(void)
{
	process_poll(&cdc_ecm_drv_process);
}

void get_recieved_data(void)
{
	int size;

	size = udi_cdc_get_nb_received_data();

	collected += udi_cdc_read_no_polling(&received[collected], size);

	if((size != 64)){
		usb_ecm_ep_recv_callback(collected);
		collected = 0;
	}
}

PROCESS_THREAD(cdc_ecm_drv_process, ev, data)
{
  PROCESS_BEGIN();

  LOG6LBR_INFO("CDC-ECM recieve process started\n");

  while(1) {
	  PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
	  get_recieved_data();

  }

  PROCESS_END();
}

/*void usb_ecm_recv_renew(void)
{

  udi_cdc_read_no_polling(received, sizeof(received));
}*/

void usb_configuration_callback(int config)
{
  (void)config;

  //usb_ecm_recv_renew();
  can_xmit = true;
}



void tx_empty_poll(void)
{
	usb_ecm_ep_send_callback(0);
}

bool usb_ecm_can_xmit(void)
{
  return can_xmit;
}

void usb_ecm_xmit_packet(uint8_t *packet, uint16_t len)
{
	int packet_size = len;
	memcpy(transmitted, packet, packet_size);
	__DMB();
	can_xmit = false;
	usb_ecm_send(transmitted, packet_size);
}

#define PACKET_TYPE_PROMISCUOUS		(1<<0)
#define PACKET_TYPE_ALL_MULTICAST	(1<<1)
#define PACKET_TYPE_DIRECTED		(1<<2)
#define PACKET_TYPE_BROADCAST		(1<<3)
#define PACKET_TYPE_MULTICAST		(1<<4)

/*//// Device specific request types
 * #define  USB_REQ_CDC_SET_ETHERNET_MULTICAST_FILTERS              0x40
 * #define  USB_REQ_CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERNFILTER 0x41
 * #define  USB_REQ_CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERNFILTER 0x42
 * #define  USB_REQ_CDC_SET_ETHERNET_PACKET_FILTER                  0x43
 * #define  USB_REQ_CDC_GET_ETHERNET_STATISTIC                      0x44
 */
static
void cdc_ecm_set_ethernet_packet_filter(uint16_t usb_ecm_packet_filter)
{

	if(usb_ecm_packet_filter & PACKET_TYPE_PROMISCUOUS) {
		//PRINTF_P(PSTR("PROMISCUOUS "));
		//USB_ETH_HOOK_SET_PROMISCIOUS_MODE(true);
	} else {
		//USB_ETH_HOOK_SET_PROMISCIOUS_MODE(false);
	}

	/*if(usb_ecm_packet_filter & PACKET_TYPE_ALL_MULTICAST)
		PRINTF_P(PSTR("ALL_MULTICAST "));
	if(usb_ecm_packet_filter & PACKET_TYPE_DIRECTED)
		PRINTF_P(PSTR("DIRECTED "));
	if(usb_ecm_packet_filter & PACKET_TYPE_BROADCAST)
		PRINTF_P(PSTR("BROADCAST "));
	if(usb_ecm_packet_filter & PACKET_TYPE_MULTICAST)
		PRINTF_P(PSTR("MULTICAST "));

	PRINTF_P(PSTR("\n"));*/
}


char usb_device_specific_request(void)
{
	//uint16_t wLength = udd_g_ctrlreq.req.wLength;
	uint16_t request = udd_g_ctrlreq.req.bRequest;
	uint16_t value = udd_g_ctrlreq.req.wValue;

	switch(request)
	{
		case USB_REQ_CDC_SET_ETHERNET_PACKET_FILTER:

			cdc_ecm_set_ethernet_packet_filter(value);
			return 1;
			break;

     	default:
			break;

	}

  	return 0;
}
