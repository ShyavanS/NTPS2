// time_math.c
/*
This file contains necessary functionality to get convert time between various
formats and do the translation work between the NTP server and the PS2 as they
store time in different formats. It has functions to compute all the different
operations that will need to be done on the time.
*/

// Include Statements
#include <time.h>
#include "time_math.h"

// Number of seconds to convert between NTP (1900) and Y2K epochs, this will
// have to be reset in 2036 when NTP era 1 begins.
const u32 NTP_TO_Y2K_EPOCH = 3155673600;

// Days in months for normal and leap years
const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Non-leap year
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Leap year
};

/*
Description: Subroutine to convert from BCD to decimal as PS2 stores time in BCD.
Inputs:      void
Outputs:     void
Parameters:  (u8)bcd
Returns:     int
*/
int bcd_to_decimal(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/*
Description: Subroutine to convert from decimal to BCD as PS2 stores time in BCD.
Inputs:      void
Outputs:     void
Parameters:  (int)dec
Returns:     u8
*/
u8 decimal_to_bcd(int dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

/*
Description: Subroutine to check if it is a leap year.
Inputs:      void
Outputs:     void
Parameters:  (int)year
Returns:     int
*/
int is_leap_year(int year)
{
    year += 2000; // Convert from two-digit format

    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/*
Description: Subroutine to convert sceCdCLOCK to time_t (seconds since 2000-01-01 00:00:00 UTC).
Inputs:      void
Outputs:     void
Parameters:  (const sceCdCLOCK *)clock
Returns:     time_t
*/
time_t sceCdCLOCK_to_time_t(const sceCdCLOCK *clock)
{
    // Get time in decimal format
    int year = bcd_to_decimal(clock->year);
    int month = bcd_to_decimal(clock->month) - 1; // Adjust for zero-based index
    int day = bcd_to_decimal(clock->day) - 1;     // Days are one-based
    int hour = bcd_to_decimal(clock->hour);
    int minute = bcd_to_decimal(clock->minute);
    int second = bcd_to_decimal(clock->second);

    time_t days = 0;

    // Calculate total days since 2000-01-01
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

/*
Description: Subroutine to convert time_t to sceCdCLOCK (storing in BCD format).
Inputs:      void
Outputs:     (sceCdCLOCK *)clock
Parameters:  (time_t)t, (sceCdCLOCK *)clock
Returns:     void
*/
void time_t_to_sceCdCLOCK(time_t t, sceCdCLOCK *clock)
{
    int year = 0;

    // Get number of years since 2000
    while (t >= (is_leap_year(year) ? 366 : 365) * 86400)
    {
        t -= (is_leap_year(year) ? 366 : 365) * 86400;
        year++;
    }

    clock->year = decimal_to_bcd(year); // Convert decimal to BCD

    int month = 0;

    // Get month in current year
    while (t >= days_in_month[is_leap_year(year)][month] * 86400)
    {
        t -= days_in_month[is_leap_year(year)][month] * 86400;
        month++;
    }

    // Convert decimal to BCD
    clock->month = decimal_to_bcd(month + 1);
    clock->day = decimal_to_bcd(t / 86400 + 1);

    t %= 86400;

    clock->hour = decimal_to_bcd(t / 3600);

    t %= 3600;

    clock->minute = decimal_to_bcd(t / 60);
    clock->second = decimal_to_bcd(t % 60);
}

/*
Description: Subroutine to convert NTP time to time_t (1900 epoch to Y2K epoch).
Inputs:      void
Outputs:     void
Parameters:  (u32)time_NTP, (int)offset, (int)dst
Returns:     (time_t)ps2_time
*/
time_t time_NTP_to_time_t(u32 time_NTP, int offset, int dst)
{
    time_t ps2_time = time_NTP - NTP_TO_Y2K_EPOCH; // Convert between epochs

    // Add DST if needed to GMT offset
    if (dst)
    {
        offset += 60;
    }

    ps2_time += offset * 60; // Convert GMT offset to seconds and add to time

    return ps2_time;
}
