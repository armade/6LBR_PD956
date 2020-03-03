#ifndef IP64_CONF_H
#define IP64_CONF_H
#include "ip64-driver.h"

extern const struct ip64_driver ECM_interface;


#define IP64_CONF_UIP_FALLBACK_INTERFACE ip64_eth_interface
#define IP64_CONF_INPUT                  ip64_eth_interface_input
//#define IP64_CONF_DHCP                   1
#define IP64_CONF_ETH_DRIVER             ECM_interface
/*
#undef UIP_FALLBACK_INTERFACE
#define UIP_FALLBACK_INTERFACE ip64_uip_fallback_interface

#include "ip64-slip-interface.h"
#include "ip64-null-driver.h"

#define IP64_CONF_UIP_FALLBACK_INTERFACE_SLIP 		1
#define IP64_CONF_UIP_FALLBACK_INTERFACE 			ip64_slip_interface
#define IP64_CONF_INPUT                  			ip64_slip_interface_input
#define IP64_CONF_ETH_DRIVER             			ip64_null_driver*/

#endif /* IP64_CONF_H */
