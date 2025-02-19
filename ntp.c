// ntp.c
/*
This file contains necessary functionality to get time from an NTP server.
Using the network functionality of the PS2, it establishes a socket connection
with the server, requests the time and performs some operations to get it into
a format usable for the PS2. This (and the premise of the entire app essentially),
was inspired by joel16/NTPSP which in turn gives credit to lettier/ntpclient.
*/

// Include Statements
#include <string.h>
#include <ps2ip.h>
#include <netdb.h>
#include "graphics.h"
#include "time_math.h"
#include "ntp.h"

struct ntp_packet time_packet; // Struct for NTP time packet

sceCdCLOCK rtc_time; // Struct for storing RTC time
sceCdCLOCK ntp_time; // Struct for storing NTP time

/*
Description: Subroutine to get time from NTP server in a format usable for the PS2.
Inputs:      (sceCdCLOCK)rtc_time, (struct ntp_packet)time_packet
Outputs:     (struct ntp_packet)time_packet
Parameters:  void
Returns:     (u32)timestamp
*/
u32 get_ntp_time(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    u32 timestamp;
    time_t t;

    struct timeval timeout = {5, 0}; // 5-second timeout

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Socket connection

    if (sockfd < 0)
    {
        screen_printf(text_scale, "Socket creation failed");
        ps2ipDeinit();
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&time_packet, 0, sizeof(time_packet));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(123);                        // NTP uses port 123
    struct hostent *server = gethostbyname("0.pool.ntp.org"); // NTP server hostname

    if (!server)
    {
        screen_printf(text_scale, "Failed to resolve NTP server");
        close(sockfd);
        return -1;
    }

    memcpy(&server_addr.sin_addr, server->h_addr, server->h_length);

    // Get time from RTC and convert to GMT
    sceCdReadClock(&rtc_time);
    configConvertToGmtTime(&rtc_time);

    // Convert time to epoch time and then from Y2K to NTP epoch
    t = sceCdCLOCK_to_time_t(&rtc_time);
    u32 t_tx_ntp = t + NTP_TO_Y2K_EPOCH;

    // Set up transmit NTP packet
    time_packet.li_vn_mode = (0 << 6) | (4 << 3) | 3; // No leap second indication, version number 4, client mode
    time_packet.tx_tm_s = htonl(t_tx_ntp);

    // Send NTP packet to server
    if (sendto(sockfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        screen_printf(text_scale, "Failed to send NTP request");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }

    // Recieve NTP packet from server
    if (recvfrom(sockfd, &time_packet, sizeof(time_packet), 0, NULL, NULL) < 0)
    {
        screen_printf(text_scale, "Failed to receive NTP response");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }

    close(sockfd); // Close socket

    time_packet.tx_tm_s = ntohl(time_packet.tx_tm_s); // Convert from big endian to little endian
    timestamp = time_packet.tx_tm_s;

    return timestamp;
}
