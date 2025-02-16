#include <tamtypes.h>
#include <libcdvd.h>

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

extern sceCdCLOCK rtcTime;
extern sceCdCLOCK ntpTime;

u32 get_ntp_time(void);