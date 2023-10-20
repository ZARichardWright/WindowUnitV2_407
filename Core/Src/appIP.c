/*
 * appIP.c
 *
 *  Created on: Aug 10, 2021
 *      Author: Richard
 */

#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_adc.h>
#include <stm32f4xx_hal_gpio.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "queue.h"
#include "appOutdoorComs.h"
#include "memory.h"
#include "appSystem.h"
#include "minmea.h"
#include "lwip.h"
#include "api.h"

#include "Modbus.h"
#if ENABLE_TCP == 1
#include "api.h"
#include "ip4_addr.h"
#include "netif.h"
#include "dns.h"
#include "ip4.h"
#endif

#ifndef SRC_APPIP_C_
#define SRC_APPIP_C_

extern struct netif gnetif;
extern modbusHandler_t ModbusH_3;
extern APP_SYSTEM_DATA *SystemState;


#define GETTOPBYTE(A) ((A>>8)&0x00FF)
#define GETBOTBYTE(A) ((A)&0x00FF)

mb_errot_t ulNotificationValue = ERR_OK;
struct ip4_addr resolved;

void smtp_serverFound(const char *name, const struct ip4_addr *ipaddr, void *arg) {
	char msg[32];

	if ((ipaddr) && (ipaddr->addr)) {
		ipaddr_ntoa_r(ipaddr, msg, 32);
		printf("%s:%s\r\n", name, msg);
		SystemState->ServerIP.addr = ipaddr->addr;
		//IP_ADDR4(&SystemState->ServerIP,192,168,0,78);
	}

}
void ethernetif_notify_conn_changed(struct netif *netif)
{
	if(netif_is_link_up(netif))
	{
		netif_set_up(&gnetif);
		while(!netif_is_up(netif))
			osDelay(1000);

		if((SystemState->IP12==0x00)&&(SystemState->IP34 ==0x00))
			dhcp_start(netif);
		else
		{
			IP4_ADDR(&gnetif.ip_addr, GETTOPBYTE(SystemState->IP12), GETBOTBYTE(SystemState->IP12), GETTOPBYTE(SystemState->IP34), GETBOTBYTE(SystemState->IP34));
			IP4_ADDR(&gnetif.netmask, GETTOPBYTE(SystemState->NM12), GETBOTBYTE(SystemState->NM12), GETTOPBYTE(SystemState->NM34), GETBOTBYTE(SystemState->NM34));
			IP4_ADDR(&gnetif.gw, GETTOPBYTE(SystemState->GW12), GETBOTBYTE(SystemState->GW12), GETTOPBYTE(SystemState->GW34), GETBOTBYTE(SystemState->GW34));
		}
	}
	else
	{
		netif_set_down(&gnetif);
		SystemState->ServerIP.addr =0;
	}
	printf("Link :%d:%d:%d:%d\r\n", ip4_addr1(&gnetif.ip_addr), ip4_addr2(&gnetif.ip_addr), ip4_addr3(&gnetif.ip_addr), ip4_addr4(&gnetif.ip_addr));
}
void StartIP(void *argument) {
	struct ip4_addr resolved;

	osDelay(1000);

	printf("Start IP Task\r\n");
	osDelay(500);
	MX_LWIP_Init();
	if((SystemState->IP12!=0x00)&&(SystemState->IP34 !=0x00))
	{
		printf("Static\r\n");
		dhcp_stop(&gnetif);
	}
	else
	{
		printf("With DHCP\r\n");
	}


	while(!SystemState->IPSet)
		osDelay(5000);

	if((SystemState->IP12!=0x00)||(SystemState->IP34 !=0x00))
	{
		IP4_ADDR(&gnetif.ip_addr, GETTOPBYTE(SystemState->IP12), GETBOTBYTE(SystemState->IP12), GETTOPBYTE(SystemState->IP34), GETBOTBYTE(SystemState->IP34));
		IP4_ADDR(&gnetif.netmask, GETTOPBYTE(SystemState->NM12), GETBOTBYTE(SystemState->NM12), GETTOPBYTE(SystemState->NM34), GETBOTBYTE(SystemState->NM34));
		IP4_ADDR(&gnetif.gw, GETTOPBYTE(SystemState->GW12), GETBOTBYTE(SystemState->GW12), GETTOPBYTE(SystemState->GW34), GETBOTBYTE(SystemState->GW34));
	}
	/* Infinite loop */
	osDelay(1000);
	for (;;) {

		if (!ip4_addr_isany_val(gnetif.ip_addr)) {
			break;
		} else
			osDelay(1000);
	}
	printf("Task IP %d.%d.%d.%d\r\n",GETTOPBYTE(SystemState->IP12),GETBOTBYTE(SystemState->IP12),GETTOPBYTE(SystemState->IP34),GETBOTBYTE(SystemState->IP34) );

	printf("Task 2 :%d:%d:%d:%d\r\n", ip4_addr1(&gnetif.ip_addr), ip4_addr2(&gnetif.ip_addr), ip4_addr3(&gnetif.ip_addr), ip4_addr4(&gnetif.ip_addr));
	printf("Port %d\r\n",SystemState->ServerPort);



	for (;;) {

		if (!netif_is_link_up(&gnetif)) {
			SystemState->ServerIP.addr = 0;
		} else

		{
			if (SystemState->ServerPort != 0) {

				if ((ModbusH_3.newconn->recvmbox == 0x00) || (ModbusH_3.newconn->type == NETCONN_INVALID)) {
					printf("DNS Loop up started\r\n");
					switch (dns_gethostbyname("aircon.ddns.net", &SystemState->ServerIP, smtp_serverFound, NULL)) {
					//switch (dns_gethostbyname("aircon.ddns.net", &resolved, smtp_serverFound, NULL)) {
					case ERR_OK:
						// numeric or cached, returned in resolved
						printf("DNS Cached\r\n");
						break;
					case ERR_INPROGRESS:
						// need to ask, will return data via callback
						printf("Waiting\r\n");
						break;
					default:
						printf("DNS Still busy\r\n");
						// bad arguments in function call
						break;

					}
				}
			}
		}
		osDelay(30000);
	}

}

#endif /* SRC_APPIP_C_ */
