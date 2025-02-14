#include <tamtypes.h>
#include <libcdvd.h>

extern struct ntp_packet time_packet;

extern sceCdCLOCK rtcTime;
extern sceCdCLOCK ntpTime;

u32 get_ntp_time(void);