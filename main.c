#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <netman.h>
#include <ps2ip.h>
#include <time.h>
#include <osd_config.h>
#include <libpad.h>
#include <sifrpc.h>

#define SERVER "pool.ntp.org"
#define PORT 123

struct NTPPacket {
    u8 li_vn_mode; // Leap Indicator, Version Number, Mode
    u8 stratum;
    u8 poll;
    u8 precision;
    u32 root_delay;
    u32 root_dispersion;
    u32 ref_id;
    u32 ref_timestamp_secs;
    u32 ref_timestamp_frac;
    u32 orig_timestamp_secs;
    u32 orig_timestamp_frac;
    u32 recv_timestamp_secs;
    u32 recv_timestamp_frac;
    u32 trans_timestamp_secs;
    u32 trans_timestamp_frac;
};

void setSystemTime(u32 seconds) {
    struct tm *tm_time = gmtime((time_t *)&seconds);  // Convert Unix time to struct tm
    sceCdCLOCK rtcTime;

    rtcTime.stat = 0;  // Always set to 0 before calling sceCdSetClock()
    rtcTime.year = tm_time->tm_year + 1900 - 2000;  // Convert from Unix time (1970-based) to PS2 format (2000-based)
    rtcTime.month = tm_time->tm_mon + 1;  // tm_mon is 0-based (0=Jan, 11=Dec), so add 1
    rtcTime.day = tm_time->tm_mday;
    rtcTime.hour = tm_time->tm_hour;
    rtcTime.minute = tm_time->tm_min;
    rtcTime.second = tm_time->tm_sec;

    sceCdWriteClock(&rtcTime);
}

u32 read_pad(struct padButtonStatus *buttons) {
    u32 pad_data;
    u32 new_pad;
    static u32 old_pad = 0;

    padRead(0, 0, buttons);

    pad_data = 0xffff ^ buttons->btns;
    new_pad = pad_data & ~old_pad;
    old_pad = pad_data;

    return new_pad;
}

int main(int argc, char *argv[]) {
    SifInitRpc(0);
    
    // Initialize network
    // ps2ipInit(2851995649, 4294967040, 3232235777);
    // ps2ip_setconfig();
    // if (ps2ipInit(2851995649, 4294967040, 3232235777) < 0) {
    //     scr_printf("Network initialization failed!\n");
    //     return -1;
    // }

    printf("Network initialized. Attempting to contact NTP server.\n");

    struct sockaddr_in server_addr;
    int sockfd;
    struct NTPPacket packet = {0};

    // Initialize NTP request
    // packet.li_vn_mode = (0 << 6) | (4 << 3) | 3; // NTP mode client, version 4

    // Set up socket
    // sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (sockfd < 0) {
    //     scr_printf("Failed to create socket.\n");
    //     return -1;
    // }

    // memset(&server_addr, 0, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(PORT);
    // server_addr.sin_addr.s_addr = inet_addr(SERVER); // You might need a DNS resolver

    // Send NTP request
    // sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Receive NTP response
    // socklen_t addr_len = sizeof(server_addr);
    // recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, &addr_len);

    time_t tic = time(NULL);

    // Convert NTP time to Unix time
    u32 ntp_time = ntohl(packet.trans_timestamp_secs) - 2208988800U; // NTP epoch starts in 1900
    static char padBuf[256] __attribute__((aligned(64)));
    
    padInit(0);
    padPortOpen(0, 0, padBuf);
    float timezone_offset = 0;
    sceCdCLOCK rtcTime;
    sceCdReadClock(&rtcTime);
    
    printf("Current System Time:\n");
    printf("PS2 clock currently set to: %04d-%02d-%02d %02d:%02d:%02d\n",
           rtcTime.year + 2000, rtcTime.month, rtcTime.day,
           rtcTime.hour, rtcTime.minute, rtcTime.second);

    printf("Choose with UP & DOWN, Confirm with X.\n");

    struct padButtonStatus buttons;

    while (padGetState(0, 0) != 6) {}

    while (~(read_pad(&buttons) & PAD_CROSS)) {
        printf("Choose your timezone offset from UTC: %.1f\n", timezone_offset);

        if (read_pad(&buttons) & PAD_UP) {
            timezone_offset += 0.5;
        } else if (read_pad(&buttons) & PAD_DOWN) {
            timezone_offset -= 0.5;
        }
    }

    u32 timezone_offset_sec = timezone_offset * 36000;

    time_t toc = time(NULL);

    // Adjust for the local timezone
    // ntp_time += timezone_offset_sec + toc - tic;
    ntp_time = 1738700000 + timezone_offset_sec + toc - tic;
    // Set system time
    setSystemTime(ntp_time);
    sceCdReadClock(&rtcTime);

    printf("System time set!\n");
    printf("PS2 clock set to: %04d-%02d-%02d %02d:%02d:%02d\n",
           rtcTime.year + 2000, rtcTime.month, rtcTime.day,
           rtcTime.hour, rtcTime.minute, rtcTime.second);

    return 0;
}
