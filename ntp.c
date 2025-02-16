#include <string.h>
#include <debug.h>
#include <ps2ip.h>
#include <netdb.h>
#include "time_math.h"
#include "ntp.h"

struct ntp_packet time_packet;

sceCdCLOCK rtcTime;
sceCdCLOCK ntpTime;

u32 get_ntp_time(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    u32 timestamp;
    time_t t;

    struct timeval timeout = {5, 0}; // 5-second timeout

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sockfd < 0)
    {
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

    if (!server)
    {
        scr_printf("Failed to resolve NTP server");
        close(sockfd);
        return -1; // Return an error code
    }

    memcpy(&server_addr.sin_addr, server->h_addr, server->h_length);

    sceCdReadClock(&rtcTime);
    configConvertToGmtTime(&rtcTime);
    t = sceCdCLOCK_to_time_t(&rtcTime);
    u32 t_tx_ntp = t + NTP_TO_Y2K_EPOCH;

    time_packet.li_vn_mode = (0 << 6) | (4 << 3) | 3; // No leap second indication, version number 4, client mode
    time_packet.tx_tm_s = htonl(t_tx_ntp);

    if (sendto(sockfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        scr_printf("Failed to send NTP request");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }
    if (recvfrom(sockfd, &time_packet, sizeof(time_packet), 0, NULL, NULL) < 0)
    {
        scr_printf("Failed to receive NTP response");
        close(sockfd);
        ps2ipDeinit();
        return -1;
    }

    close(sockfd);

    time_packet.tx_tm_s = ntohl(time_packet.tx_tm_s);
    timestamp = time_packet.tx_tm_s;

    return timestamp;
}