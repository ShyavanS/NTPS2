/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/

// net.c
/*
This file contains necessary functionality for networking. It gets ip
configuration through DHCP and sets up networking so the PS2 can establish a
socket connection to the NTP server. A significant portion of this was
replicated from ps2ConnectToSocket/ps2ConnectToSocket.
*/

// Include Statements
#include <tamtypes.h>
#include <kernel.h>
#include <ps2ip.h>
#include <netman.h>
#include <netdb.h>
#include "graphics.h"

/*
Description: Subroutine to apply network inteface configuration.
Inputs:      void
Outputs:     void
Parameters:  (int)mode
Returns:     (int)result
*/
static int eth_apply_net_IF_config(int mode)
{
	int result;

	// By default, auto-negotiation is used
	static int CurrentMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

	// Change the setting, only if different
	if (CurrentMode != mode)
	{
		if ((result = NetManSetLinkMode(mode)) == 0)
		{
			CurrentMode = mode;
		}
	}
	else
	{
		result = 0;
	}

	return result;
}

/*
Description: Subroutine to apply IP configuration.
Inputs:      void
Outputs:     void
Parameters:  (int)use_dhcp, (const struct ip4_addr *)ip, (const struct ip4_addr *)netmask,
			 (const struct ip4_addr *)gateway, (const struct ip4_addr *)dns
Returns:     (int)result
*/
static int eth_apply_IP_config(int use_dhcp, const struct ip4_addr *ip, const struct ip4_addr *netmask, const struct ip4_addr *gateway, const struct ip4_addr *dns)
{
	t_ip_info ip_info;
	const ip_addr_t *dns_curr;
	int result;

	// SMAP is registered as the "sm0" device to the TCP/IP stack
	if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
	{
		// Obtain the current DNS server settings
		dns_curr = dns_getserver(0);

		// Check if it's the same. Otherwise, apply the new configuration
		if ((use_dhcp != ip_info.dhcp_enabled) || (!use_dhcp &&
												   (!ip_addr_cmp(ip, (struct ip4_addr *)&ip_info.ipaddr) ||
													!ip_addr_cmp(netmask, (struct ip4_addr *)&ip_info.netmask) ||
													!ip_addr_cmp(gateway, (struct ip4_addr *)&ip_info.gw) ||
													!ip_addr_cmp(dns, dns_curr))))
		{
			if (use_dhcp)
			{
				ip_info.dhcp_enabled = 1;
			}
			else
			{
				// Copy over new settings if DHCP is not used
				ip_addr_set((struct ip4_addr *)&ip_info.ipaddr, ip);
				ip_addr_set((struct ip4_addr *)&ip_info.netmask, netmask);
				ip_addr_set((struct ip4_addr *)&ip_info.gw, gateway);

				ip_info.dhcp_enabled = 0;
			}

			// Update settings
			result = ps2ip_setconfig(&ip_info);

			if (!use_dhcp)
			{
				dns_setserver(0, (ip_addr_t *)dns);
			}
		}
		else
		{
			result = 0;
		}
	}

	return result;
}

/*
Description: Subroutine to check ethernet status.
Inputs:      void
Outputs:     void
Parameters:  (s32)alarm_id, (u16)time, (void *)common
Returns:     void
*/
static void eth_status_check_cb(s32 alarm_id, u16 time, void *common)
{
	iWakeupThread(*(int *)common);
}

/*
Description: Subroutine to wait until network status is valid.
Inputs:      void
Outputs:     void
Parameters:  (int *)checking_function
Returns:     void
*/
static int wait_valid_net_state(int (*checking_function)(void))
{
	int thread_ID, retry_cycles;

	// Wait for a valid network status
	thread_ID = GetThreadId();

	for (retry_cycles = 0; checking_function() == 0; retry_cycles++)
	{
		// Sleep for 1000ms
		SetAlarm(1000 * 16, &eth_status_check_cb, &thread_ID);
		SleepThread();

		// 10s = 10*1000ms
		if (retry_cycles >= 10)
		{
			return -1;
		}
	}

	return 0;
}

/*
Description: Subroutine to get network inteface link status.
Inputs:      void
Outputs:     void
Parameters:  void
Returns:     int
*/
static int eth_get_net_IF_link_status(void)
{
	return (NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP);
}

/*
Description: Subroutine to wait until network inteface link state is valid.
Inputs:      void
Outputs:     void
Parameters:  void
Returns:     int
*/
static int eth_wait_valid_net_IF_link_status(void)
{
	return wait_valid_net_state(&eth_get_net_IF_link_status);
}

/*
Description: Subroutine to print out ethernet link status.
Inputs:      void
Outputs:     void
Parameters:  void
Returns:     void
*/
static void eth_print_link_status(void)
{
	int mode, baseMode;

	// SMAP is registered as the "sm0" device to the TCP/IP stack
	screen_printf(text_scale, "Link:\t");

	if (NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP)
	{
		screen_printf(text_scale, "Up\n");
	}
	else
	{
		screen_printf(text_scale, "Down\n");
	}

	screen_printf(text_scale, "Mode:\t");

	mode = NetManIoctl(NETMAN_NETIF_IOCTL_ETH_GET_LINK_MODE, NULL, 0, NULL, 0);

	// NETMAN_NETIF_ETH_LINK_MODE_PAUSE is a flag, so file it off first
	baseMode = mode & (~NETMAN_NETIF_ETH_LINK_DISABLE_PAUSE);

	switch (baseMode)
	{
	case NETMAN_NETIF_ETH_LINK_MODE_10M_HDX:
		screen_printf(text_scale, "10M HDX");
		break;
	case NETMAN_NETIF_ETH_LINK_MODE_10M_FDX:
		screen_printf(text_scale, "10M FDX");
		break;
	case NETMAN_NETIF_ETH_LINK_MODE_100M_HDX:
		screen_printf(text_scale, "100M HDX");
		break;
	case NETMAN_NETIF_ETH_LINK_MODE_100M_FDX:
		screen_printf(text_scale, "100M FDX");
		break;
	default:
		screen_printf(text_scale, "Unknown");
	}

	if (!(mode & NETMAN_NETIF_ETH_LINK_DISABLE_PAUSE))
	{
		screen_printf(text_scale, " with ");
	}
	else
	{
		screen_printf(text_scale, " without ");
	}

	screen_printf(text_scale, "Flow Control\n");
}

/*
Description: Subroutine to print out IP configuration.
Inputs:      void
Outputs:     void
Parameters:  void
Returns:     void
*/
static void eth_print_IP_config(void)
{
	t_ip_info ip_info;
	const ip_addr_t *dns_curr;
	u8 ip_address[4], netmask[4], gateway[4], dns[4];

	// SMAP is registered as the "sm0" device to the TCP/IP stack
	if (ps2ip_getconfig("sm0", &ip_info) >= 0)
	{
		// Obtain the current DNS server settings
		dns_curr = dns_getserver(0);

		ip_address[0] = ip4_addr1((struct ip4_addr *)&ip_info.ipaddr);
		ip_address[1] = ip4_addr2((struct ip4_addr *)&ip_info.ipaddr);
		ip_address[2] = ip4_addr3((struct ip4_addr *)&ip_info.ipaddr);
		ip_address[3] = ip4_addr4((struct ip4_addr *)&ip_info.ipaddr);

		netmask[0] = ip4_addr1((struct ip4_addr *)&ip_info.netmask);
		netmask[1] = ip4_addr2((struct ip4_addr *)&ip_info.netmask);
		netmask[2] = ip4_addr3((struct ip4_addr *)&ip_info.netmask);
		netmask[3] = ip4_addr4((struct ip4_addr *)&ip_info.netmask);

		gateway[0] = ip4_addr1((struct ip4_addr *)&ip_info.gw);
		gateway[1] = ip4_addr2((struct ip4_addr *)&ip_info.gw);
		gateway[2] = ip4_addr3((struct ip4_addr *)&ip_info.gw);
		gateway[3] = ip4_addr4((struct ip4_addr *)&ip_info.gw);

		dns[0] = ip4_addr1(dns_curr);
		dns[1] = ip4_addr2(dns_curr);
		dns[2] = ip4_addr3(dns_curr);
		dns[3] = ip4_addr4(dns_curr);

		screen_printf(text_scale, "IP:\t%d.%d.%d.%d\n"
								  "NM:\t%d.%d.%d.%d\n"
								  "GW:\t%d.%d.%d.%d\n"
								  "DNS:\t%d.%d.%d.%d\n",
					  ip_address[0], ip_address[1], ip_address[2], ip_address[3],
					  netmask[0], netmask[1], netmask[2], netmask[3],
					  gateway[0], gateway[1], gateway[2], gateway[3],
					  dns[0], dns[1], dns[2], dns[3]);
	}
	else
	{
		screen_printf(text_scale, "Unable to read IP address.\n");
	}
}

/*
Description: Subroutine to load IP configuration and set up networking.
Inputs:      void
Outputs:     void
Parameters:  void
Returns:     void
*/
void load_ipconfig(void)
{
	struct ip4_addr IP, NM, GW, DNS;
	int EthernetLinkMode;

	// Set network interface link mode
	EthernetLinkMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

	// Attempt to apply the new link setting
	if (eth_apply_net_IF_config(EthernetLinkMode) != 0)
	{
		screen_printf(text_scale, "Error: failed to set link mode.\n");
		return;
	}

	// Initialize IP addresses
	IP4_ADDR(&IP, 169, 254, 0, 1);
	IP4_ADDR(&NM, 255, 255, 255, 0);
	IP4_ADDR(&GW, 192, 168, 1, 1);
	IP4_ADDR(&DNS, 192, 168, 1, 1);

	ps2ipInit(&IP, &NM, &GW); // Initialize the TCP/IP protocol stack
	dns_setserver(0, &DNS);	  // Set DNS server

	// Change IP address
	eth_apply_IP_config(1, &IP, &NM, &GW, &DNS);

	// Wait for the link to become ready
	if (eth_wait_valid_net_IF_link_status() != 0)
	{
		screen_printf(text_scale, "Error: failed to get valid link status.\n");
		return;
	}

	// Print out link info
	screen_printf(text_scale, "Initialized:\n");
	eth_print_link_status();
	eth_print_IP_config();
}
