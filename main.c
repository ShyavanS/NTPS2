/*
TODO:
- Add background image
- Documentation
*/

#include <kernel.h>
// #include <gsKit.h>
// #include <gsToolkit.h>
// #include <dmaKit.h>
#include <string.h>
#include <tamtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <osd_config.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <debug.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <unistd.h>
#include <libpad.h>
#include <netman.h>
#include <ps2ip.h>
#include <netdb.h>
#include <sys/socket.h>
// #include "ntps2_logo.h"

#define IPCONFIG_PATH_1 "mc0:/SYS-CONF/IPCONFIG.DAT"
#define IPCONFIG_PATH_2 "mc1:/SYS-CONF/IPCONFIG.DAT"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

struct ntp_packet {
    u8 li_vn_mode; // Li-VN-Mode
    u8 stratum; // Stratum
    u8 poll; // Poll
    u8 precision; // Precision
    u32 root_delay; // Root Delay
    u32 root_dispersion; // Root Dispersion
    u32 ref_id; // Reference ID
    u32 ref_tm_s; // Reference Timestamp Seconds
    u32 ref_tm_f; // Reference Timestamp Fractional
    u32 org_tm_s; // Origin Timestamp Seconds
    u32 org_tm_f; // Origin Timestamp Fractional
    u32 rx_tm_s; // Receive Timestamp Seconds
    u32 rx_tm_f; // Receive Timestamp Fractional
    u32 tx_tm_s; // Transmit Timestamp Seconds
    u32 tx_tm_f; // Transmit Timestamp Fractional
} time_packet;

struct padButtonStatus buttons;

sceCdCLOCK rtcTime;
sceCdCLOCK ntpTime;

// GSTEXTURE bg_texture;

const u32 NTP_TO_Y2K_EPOCH = 3155673600;

// Convert BCD to Decimal
int bcd_to_decimal(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Convert Decimal to BCD
u8 decimal_to_bcd(int dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

// Helper function to check if a year is a leap year
int is_leap_year(int year)
{
    year += 2000; // Convert from two-digit format
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Days in months for normal and leap years
const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Non-leap year
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Leap year
};

// Convert sceCdCLOCK to time_t (seconds since 2000-01-01 00:00:00 UTC)
time_t sceCdCLOCK_to_time_t(const sceCdCLOCK *clock)
{
    int year = bcd_to_decimal(clock->year);
    int month = bcd_to_decimal(clock->month) - 1; // Adjust for zero-based index
    int day = bcd_to_decimal(clock->day) - 1;     // Days are one-based
    int hour = bcd_to_decimal(clock->hour);
    int minute = bcd_to_decimal(clock->minute);
    int second = bcd_to_decimal(clock->second);

    // Calculate total days since 2000-01-01
    time_t days = 0;
    for (int y = 0; y < year; y++)
    {
        days += is_leap_year(y) ? 366 : 365;
    }

    for (int m = 0; m < month; m++)
    {
        days += days_in_month[is_leap_year(year)][m];
    }

    days += day;

    // Convert days to seconds and add time of day
    return (days * 86400) + (hour * 3600) + (minute * 60) + second;
}

// Convert time_t back to sceCdCLOCK (storing in BCD format)
void time_t_to_sceCdCLOCK(time_t t, sceCdCLOCK *clock)
{
    int year = 0;
    while (t >= (is_leap_year(year) ? 366 : 365) * 86400)
    {
        t -= (is_leap_year(year) ? 366 : 365) * 86400;
        year++;
    }

    clock->year = decimal_to_bcd(year);

    int month = 0;
    while (t >= days_in_month[is_leap_year(year)][month] * 86400)
    {
        t -= days_in_month[is_leap_year(year)][month] * 86400;
        month++;
    }

    clock->month = decimal_to_bcd(month + 1);
    clock->day = decimal_to_bcd(t / 86400 + 1);
    t %= 86400;
    clock->hour = decimal_to_bcd(t / 3600);
    t %= 3600;
    clock->minute = decimal_to_bcd(t / 60);
    clock->second = decimal_to_bcd(t % 60);
}

time_t time_NTP_to_time_t(u32 time_NTP, int offset, int dst)
{
    time_t unix_time = time_NTP - NTP_TO_Y2K_EPOCH;

    if (dst)
    {
        offset += 60;
    }

    unix_time += offset * 60;

    return unix_time;
}

void getSystemTime(int offset, int dst)
{
    sceCdReadClock(&rtcTime);
    configConvertToGmtTime(&rtcTime);

    if (dst)
    {
        offset += 60;
    }

    time_t t = sceCdCLOCK_to_time_t(&rtcTime);
    t += offset * 60; // Apply offset in seconds
    time_t_to_sceCdCLOCK(t, &rtcTime);
}

void setSystemTime(time_t ntp_time)
{
    time_t_to_sceCdCLOCK(ntp_time, &ntpTime);

    sceCdWriteClock(&ntpTime);
}

u32 get_ntp_time(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    u32 timestamp;
    int32_t time_offset;
    time_t t;

    struct timeval timeout = {5, 0}; // 5-second timeout

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sockfd < 0) {
    scr_printf("Socket creation failed");
    ps2ipDeinit();
    return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&time_packet, 0, sizeof(time_packet));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(123); // NTP uses port 123
    struct hostent *server = gethostbyname("0.pool.ntp.org");

    if (!server) {
        scr_printf("Failed to resolve NTP server");
        close(sockfd);
        return -1; // Return an error code
    }

    memcpy(&server_addr.sin_addr, server->h_addr, server->h_length);

    sceCdReadClock(&rtcTime);
    configConvertToGmtTime(&rtcTime);
    t = sceCdCLOCK_to_time_t(&rtcTime);
    u32 t_tx_ntp = t + NTP_TO_Y2K_EPOCH;

    time_packet.li_vn_mode = (0 << 6) | (4 << 3) | 3;  // No leap second indication, version number 4, client mode
    time_packet.tx_tm_s = htonl(t_tx_ntp);

    if (sendto(sockfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        scr_printf("Failed to send NTP request");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }
    if (recvfrom(sockfd, &time_packet, sizeof(time_packet), 0, NULL, NULL) < 0) {
        scr_printf("Failed to receive NTP response");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }
    
    close(sockfd);

    sceCdReadClock(&rtcTime);
    configConvertToGmtTime(&rtcTime);
    t = sceCdCLOCK_to_time_t(&rtcTime);
    u32 t_rx_ntp = t + NTP_TO_Y2K_EPOCH;

    time_packet.tx_tm_s = ntohl(time_packet.tx_tm_s);
    time_packet.rx_tm_s = ntohl(time_packet.rx_tm_s);

    scr_printf("%d\n", time_packet.tx_tm_s);
    scr_printf("%d\n", time_packet.rx_tm_s);
    scr_printf("%d\n", t_tx_ntp);
    scr_printf("%d\n", t_rx_ntp);
    time_offset = ((int32_t)(time_packet.rx_tm_s - t_tx_ntp) + (int32_t)(time_packet.tx_tm_s - t_rx_ntp)) / 2;
    timestamp = time_packet.tx_tm_s + time_offset;
    scr_printf("%d\n", time_offset);
    scr_printf("%d\n", timestamp);
    
    return timestamp;
}

static int ethApplyNetIFConfig(int mode)
{
	int result;
	//By default, auto-negotiation is used.
	static int CurrentMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

	if(CurrentMode != mode)
	{	//Change the setting, only if different.
		if((result = NetManSetLinkMode(mode)) == 0)
			CurrentMode = mode;
	}else
		result = 0;

	return result;
}

static int ethApplyIPConfig(int use_dhcp, const struct ip4_addr *ip, const struct ip4_addr *netmask, const struct ip4_addr *gateway, const struct ip4_addr *dns)
{
	t_ip_info ip_info;
	const ip_addr_t *dns_curr;
	int result;

	//SMAP is registered as the "sm0" device to the TCP/IP stack.
	if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
	{
		//Obtain the current DNS server settings.
		dns_curr = dns_getserver(0);

		//Check if it's the same. Otherwise, apply the new configuration.
		if ((use_dhcp != ip_info.dhcp_enabled)
		    ||	(!use_dhcp &&
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
			{	//Copy over new settings if DHCP is not used.
				ip_addr_set((struct ip4_addr *)&ip_info.ipaddr, ip);
				ip_addr_set((struct ip4_addr *)&ip_info.netmask, netmask);
				ip_addr_set((struct ip4_addr *)&ip_info.gw, gateway);

				ip_info.dhcp_enabled = 0;
			}

			//Update settings.
			result = ps2ip_setconfig(&ip_info);
			if (!use_dhcp)
				dns_setserver(0, dns);
		}
		else
			result = 0;
	}

	return result;
}

static void EthStatusCheckCb(s32 alarm_id, u16 time, void *common)
{
	iWakeupThread(*(int*)common);
}

static int WaitValidNetState(int (*checkingFunction)(void))
{
	int ThreadID, retry_cycles;

	// Wait for a valid network status;
	ThreadID = GetThreadId();
	for(retry_cycles = 0; checkingFunction() == 0; retry_cycles++)
	{	//Sleep for 1000ms.
		SetAlarm(1000 * 16, &EthStatusCheckCb, &ThreadID);
		SleepThread();

		if(retry_cycles >= 10)	//10s = 10*1000ms
			return -1;
	}

	return 0;
}

static int ethGetNetIFLinkStatus(void)
{
	return(NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP);
}

static int ethWaitValidNetIFLinkState(void)
{
	return WaitValidNetState(&ethGetNetIFLinkStatus);
}

static void ethPrintLinkStatus(void)
{
	int mode, baseMode;

	//SMAP is registered as the "sm0" device to the TCP/IP stack.
	scr_printf("Link:\t");
	if (NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP)
		scr_printf("Up\n");
	else
		scr_printf("Down\n");

	scr_printf("Mode:\t");
	mode = NetManIoctl(NETMAN_NETIF_IOCTL_ETH_GET_LINK_MODE, NULL, 0, NULL, 0);

	//NETMAN_NETIF_ETH_LINK_MODE_PAUSE is a flag, so file it off first.
	baseMode = mode & (~NETMAN_NETIF_ETH_LINK_DISABLE_PAUSE);
	switch(baseMode)
	{
		case NETMAN_NETIF_ETH_LINK_MODE_10M_HDX:
			scr_printf("10M HDX");
			break;
		case NETMAN_NETIF_ETH_LINK_MODE_10M_FDX:
			scr_printf("10M FDX");
			break;
		case NETMAN_NETIF_ETH_LINK_MODE_100M_HDX:
			scr_printf("100M HDX");
			break;
		case NETMAN_NETIF_ETH_LINK_MODE_100M_FDX:
			scr_printf("100M FDX");
			break;
		default:
			scr_printf("Unknown");
	}
	if(!(mode & NETMAN_NETIF_ETH_LINK_DISABLE_PAUSE))
		scr_printf(" with ");
	else
		scr_printf(" without ");
	scr_printf("Flow Control\n");
}

static void ethPrintIPConfig(void)
{
	t_ip_info ip_info;
	const ip_addr_t *dns_curr;
	u8 ip_address[4], netmask[4], gateway[4], dns[4];

	//SMAP is registered as the "sm0" device to the TCP/IP stack.
	if (ps2ip_getconfig("sm0", &ip_info) >= 0)
	{
		//Obtain the current DNS server settings.
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

		scr_printf(	"IP:\t%d.%d.%d.%d\n"
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
		scr_printf("Unable to read IP address.\n");
	}
}

void load_ipconfig() {
    struct ip4_addr IP, NM, GW, DNS;
    t_ip_info ipconfig;
	int EthernetLinkMode;

    //The network interface link mode/duplex can be set.
	EthernetLinkMode = NETMAN_NETIF_ETH_LINK_MODE_AUTO;

    //Attempt to apply the new link setting.
	if(ethApplyNetIFConfig(EthernetLinkMode) != 0) {
		scr_printf("Error: failed to set link mode.\n");
	}

    //Initialize IP address.
    IP4_ADDR(&IP, 169, 254, 0, 1);
    IP4_ADDR(&NM, 255, 255, 255, 0);
    IP4_ADDR(&GW, 192, 168, 1, 1);
    IP4_ADDR(&DNS, 192, 168, 1, 1);
	
	//Initialize the TCP/IP protocol stack.
	ps2ipInit(&IP, &NM, &GW);
	dns_setserver(0, &DNS);	//Set DNS server

	//Change IP address
	ethApplyIPConfig(1, &IP, &NM, &GW, &DNS);

	//Wait for the link to become ready.
	// scr_printf("Waiting for connection...\n");
	if(ethWaitValidNetIFLinkState() != 0) {
		scr_printf("Error: failed to get valid link status.\n");
	}

    // scr_printf("Initialized:\n");
	// ethPrintLinkStatus();
	// ethPrintIPConfig();
}

u32 read_pad(void)
{
    u32 pad_data;
    u32 new_pad;
    static u32 old_pad = 0;

    padRead(0, 0, &buttons);

    pad_data = 0xffff ^ buttons.btns;
    new_pad = pad_data & ~old_pad;
    old_pad = pad_data;

    return new_pad;
}

// void load_embedded_texture(GSTEXTURE *texture, int width, int height, int bpp) {
//     texture->Width = width;
//     texture->Height = height;
//     texture->PSM = (bpp == 32) ? GS_PSM_CT32 : GS_PSM_CT16;
//     texture->Vram = 0;
//     texture->Filter = GS_FILTER_LINEAR;
//     texture->Mem = (void *)bg_raw; // Point to embedded image data
// }

int main(int argc, char *argv[])
{
    clock_t tic;
    clock_t toc;
    time_t ps2_epoch;
    time_t adjusted_epoch;
    u32 epoch;
    u32 pad_reading;
    int y_pos;

    int gmt_offset = configGetTimezone();
    int daylight_savings = configIsDaylightSavingEnabled();

    static char padBuf[256] __attribute__((aligned(64)));

    // u64 Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
    // u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);

    // GSGLOBAL *gsGlobal = gsKit_init_global();
    
    // Auto-detect and apply correct video mode
    // gsGlobal->Mode = gsKit_detect_signal();
    // gsGlobal->PrimAAEnable = GS_SETTING_ON;
    
    // GSFONT *gsFont = gsKit_init_font(GSKIT_FTYPE_BMP_DAT, "host:emotionengine.png");
    
    // dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    // dmaKit_chan_init(DMA_CHANNEL_GIF);
    // dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
    // dmaKit_chan_init(DMA_CHANNEL_TOSPR);

    SifInitRpc(0);
    while(!SifIopReset("", 0)){};
    while(!SifIopSync()){};

    SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb();

    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
	SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    
    NetManInit();

    init_scr();

    load_ipconfig();

    // gsKit_init_screen(gsGlobal);
    // gsKit_font_upload(gsGlobal, gsFont);
    // gsKit_mode_switch(gsGlobal, GS_ONESHOT);
    // gsFont->Texture->Filter = GS_FILTER_LINEAR;

    // Load the background texture
    // load_embedded_texture(&bg_texture, 640, 448, 32);
    // gsKit_texture_upload(gsGlobal, &bg_texture);

    padInit(0);
    padPortOpen(0, 0, padBuf);

    scr_printf("NTPS2: A Bare-Bones NTP Client for the PS2\n");
    scr_printf("******************************************\n");
    scr_printf("X: Save, O: Exit\n");
    scr_printf("******************************************\n\n");
    
    // get epoch time
    epoch = get_ntp_time();
    // epoch = 3333333333;
    tic = clock();
    y_pos = scr_getY();
    ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);

    while (1)
    {
        // gsKit_font_print_scaled(gsGlobal, gsFont, 100, 20, 1, 2.0f, White, "TEST");
        // gsKit_texture_upload(gsGlobal, &bg_texture);
        // gsKit_sync_flip(gsGlobal);
        // gsKit_queue_exec(gsGlobal);

        getSystemTime(gmt_offset, daylight_savings);

        scr_setXY(0, y_pos);
        scr_printf("Current System Time:\n");
        scr_printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                   bcd_to_decimal(rtcTime.year) + 2000, bcd_to_decimal(rtcTime.month), bcd_to_decimal(rtcTime.day),
                   bcd_to_decimal(rtcTime.hour), bcd_to_decimal(rtcTime.minute), bcd_to_decimal(rtcTime.second));

        toc = clock();
        adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;

        time_t_to_sceCdCLOCK(adjusted_epoch, &ntpTime);
        
        scr_printf("Proposed New System Time:\n");
        scr_printf("%04d-%02d-%02d %02d:%02d:%02d\n\n",
                   bcd_to_decimal(ntpTime.year) + 2000, bcd_to_decimal(ntpTime.month), bcd_to_decimal(ntpTime.day),
                   bcd_to_decimal(ntpTime.hour), bcd_to_decimal(ntpTime.minute), bcd_to_decimal(ntpTime.second));

        while (padGetState(0, 0) != 6) {}

        pad_reading = read_pad();

        if (pad_reading & PAD_CROSS)
        {
            scr_printf("Saving...\n");

            ps2_epoch = time_NTP_to_time_t(epoch, 540, 0);
            toc = clock();
            adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;
            setSystemTime(adjusted_epoch);
            scr_printf("Saved!\n");
            sleep(1);
            scr_clear();
            scr_setXY(0, 0);
            scr_printf("NTPS2: A Bare-Bones NTP Client for the PS2\n");
            scr_printf("******************************************\n");
            scr_printf("X: Save, O: Exit\n");
            scr_printf("******************************************\n\n");
            // get epoch time
            epoch = get_ntp_time();
            // epoch = 3333333333;
            tic = clock();
            ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);
        }
        else if (pad_reading & PAD_CIRCLE)
        {
            scr_printf("Exiting...\n");
            ps2ipDeinit();
            sleep(1);
            break;
        }
    }

    padPortClose(0, 0);
    padEnd();

    return 0;
}
