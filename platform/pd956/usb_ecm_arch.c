/*
 * usb_ecm_arch.c
 *
 *  Created on: 13/01/2020
 *      Author: pm
 */
#include "contiki.h"
#include "compiler.h"
#include "net/ip/uip.h"
#include "ip64-driver.h"
#include "ip64-eth.h"

extern void usb_ecm_xmit_packet(uint8_t *packet, uint16_t len);
extern char cdc_imac_addr[14+1];
//PROCESS(slip_arch_process, "USB CDC-ECM");
extern struct ip64_eth_addr MAC_addr;
/*---------------------------------------------------------------------------*/
static void init(void)
{
	ip64_eth_addr_set(&MAC_addr);
}

/*---------------------------------------------------------------------------*/
// send out Ethernet frame
static int
output(uint8_t *packet, uint16_t len)
{
	usb_ecm_xmit_packet(packet,len);
    return 0;
}

/*---------------------------------------------------------------------------*/
const struct ip64_driver ECM_interface = {
  init, output
};

/*---------------------------------------------------------------------------*/

