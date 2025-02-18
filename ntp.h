// ntp.h
/*
The header file for ntp.c, exposes the necessary functionality to other
parts of the program for getting the time from the NTP server.
This (and the premise of the entire app essentially), was inspired by
joel16/NTPSP which in turn gives credit to lettier/ntpclient.
*/

// Include Statements
#include <tamtypes.h>
#include <libcdvd.h>

// Struct for NTP time packet
struct ntp_packet
{
    u8 li_vn_mode;       // Li-VN-Mode
    u8 stratum;          // Stratum
    u8 poll;             // Poll
    u8 precision;        // Precision
    u32 root_delay;      // Root Delay
    u32 root_dispersion; // Root Dispersion
    u32 ref_id;          // Reference ID
    u32 ref_tm_s;        // Reference Timestamp Seconds
    u32 ref_tm_f;        // Reference Timestamp Fractional
    u32 org_tm_s;        // Origin Timestamp Seconds
    u32 org_tm_f;        // Origin Timestamp Fractional
    u32 rx_tm_s;         // Receive Timestamp Seconds
    u32 rx_tm_f;         // Receive Timestamp Fractional
    u32 tx_tm_s;         // Transmit Timestamp Seconds
    u32 tx_tm_f;         // Transmit Timestamp Fractional
};
typedef struct ntp_packet NTP_PACKET;

extern sceCdCLOCK rtc_time; // Struct for storing RTC time
extern sceCdCLOCK ntp_time; // Struct for storing NTP time

/*
Description: Subroutine to get time from NTP server in a format usable for the PS2.
Inputs:      (sceCdCLOCK)rtc_time, (struct ntp_packet)time_packet
Outputs:     (struct ntp_packet)time_packet
Parameters:  void
Returns:     (u32)timestamp
*/
u32 get_ntp_time(void);
