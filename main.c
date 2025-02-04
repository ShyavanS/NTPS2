#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <netman.h>
#include <ps2ip.h>
#include <time.h>
#include <input.h>
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
    struct clock_data {
        u8 hour;
        u8 minute;
        u8 second;
        u8 day;
        u8 month;
        u8 year;
    } clock;

    struct tm *timeinfo = localtime((time_t *)&seconds);

    clock.hour = timeinfo->tm_hour;
    clock.minute = timeinfo->tm_min;
    clock.second = timeinfo->tm_sec;
    clock.day = timeinfo->tm_mday;
    clock.month = timeinfo->tm_mon + 1;
    clock.year = timeinfo->tm_year % 100;

    SifSetIopTime(&clock);
}

int main(int argc, char *argv[]) {
    SifInitRpc(0);
    init_scr();
    scr_setXY(20, 20);

    // Initialize network
    // ps2ipInit(2851995649, 4294967040, 3232235777);
    // ps2ip_setconfig();
    // if (ps2ipInit(2851995649, 4294967040, 3232235777) < 0) {
    //     scr_printf("Network initialization failed!\n");
    //     return -1;
    // }

    scr_printf("Network initialized. Attempting to contact NTP server.\n");

    struct sockaddr_in server_addr;
    int sockfd;
    struct NTPPacket packet = {0};

    // Initialize NTP request
    // packet.li_vn_mode = (0 << 6) | (4 << 3) | 3; // NTP mode client, version 4

    scr_setXY(20, 70);

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

    pad_t * controller = pad_open(0, 0, 0, 0);
    float timezone_offset = 0;
    
    scr_printf("Choose with UP & DOWN, Confirm with X.");
    scr_setXY(20, 120);

    while (~PAD_CROSS && pad_get_state(controller)) {
        scr_sprintf("Choose your timezone offset from UTC: %.1f", timezone_offset);

        if (PAD_UP && pad_get_state(controller)) {
            timezone_offset += 0.5;
        } else if (PAD_DOWN && pad_get_state(controller)) {
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

    scr_setXY(20, 170);
    printf("System time set!\n");

    return 0;
}
