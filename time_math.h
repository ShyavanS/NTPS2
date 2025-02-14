#include <osd_config.h>

extern const u32 NTP_TO_Y2K_EPOCH;

int bcd_to_decimal(u8 bcd);
time_t sceCdCLOCK_to_time_t(const sceCdCLOCK *clock);
void time_t_to_sceCdCLOCK(time_t t, sceCdCLOCK *clock);
time_t time_NTP_to_time_t(u32 time_NTP, int offset, int dst);
