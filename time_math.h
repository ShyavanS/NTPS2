// time_math.h
/*
The header file for time_math.c, exposes the necessary functionality to other
parts of the program for converting time between various formats.
*/

// Include Statements
#include <osd_config.h>

// Number of seconds to convert between NTP (1900) and Y2K epochs, this will
// have to be reset in 2036 when NTP era 1 begins.
extern const u32 NTP_TO_Y2K_EPOCH;

/*
Description: Subroutine to convert from BCD to decimal as PS2 stores time in BCD.
Inputs:      void
Outputs:     void
Parameters:  (u8)bcd
Returns:     int
*/
int bcd_to_decimal(u8 bcd);

/*
Description: Subroutine to convert sceCdCLOCK to time_t (seconds since 2000-01-01 00:00:00 UTC).
Inputs:      void
Outputs:     void
Parameters:  (const sceCdCLOCK *)clock
Returns:     time_t
*/
time_t sceCdCLOCK_to_time_t(const sceCdCLOCK *clock);

/*
Description: Subroutine to convert time_t to sceCdCLOCK (storing in BCD format).
Inputs:      void
Outputs:     (sceCdCLOCK *)clock
Parameters:  (time_t)t, (sceCdCLOCK *)clock
Returns:     void
*/
void time_t_to_sceCdCLOCK(time_t t, sceCdCLOCK *clock);

/*
Description: Subroutine to convert NTP time to time_t (1900 epoch to Y2K epoch).
Inputs:      void
Outputs:     void
Parameters:  (u32)time_NTP, (int)offset, (int)dst
Returns:     (time_t)ps2_time
*/
time_t time_NTP_to_time_t(u32 time_NTP, int offset, int dst);
