#include <time.h>
#include "time_math.h"

const u32 NTP_TO_Y2K_EPOCH = 3155673600;

// Convert BCD to Decimal
int bcd_to_decimal(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Convert Decimal to BCD
u8 decimal_to_bcd(int dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

// Helper function to check if a year is a leap year
int is_leap_year(int year)
{
    year += 2000; // Convert from two-digit format
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Days in months for normal and leap years
const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Non-leap year
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Leap year
};

// Convert sceCdCLOCK to time_t (seconds since 2000-01-01 00:00:00 UTC)
time_t sceCdCLOCK_to_time_t(const sceCdCLOCK *clock)
{
    int year = bcd_to_decimal(clock->year);
    int month = bcd_to_decimal(clock->month) - 1; // Adjust for zero-based index
    int day = bcd_to_decimal(clock->day) - 1;     // Days are one-based
    int hour = bcd_to_decimal(clock->hour);
    int minute = bcd_to_decimal(clock->minute);
    int second = bcd_to_decimal(clock->second);

    // Calculate total days since 2000-01-01
    time_t days = 0;
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

// Convert time_t back to sceCdCLOCK (storing in BCD format)
void time_t_to_sceCdCLOCK(time_t t, sceCdCLOCK *clock)
{
    int year = 0;
    while (t >= (is_leap_year(year) ? 366 : 365) * 86400)
    {
        t -= (is_leap_year(year) ? 366 : 365) * 86400;
        year++;
    }

    clock->year = decimal_to_bcd(year);

    int month = 0;
    while (t >= days_in_month[is_leap_year(year)][month] * 86400)
    {
        t -= days_in_month[is_leap_year(year)][month] * 86400;
        month++;
    }

    clock->month = decimal_to_bcd(month + 1);
    clock->day = decimal_to_bcd(t / 86400 + 1);
    t %= 86400;
    clock->hour = decimal_to_bcd(t / 3600);
    t %= 3600;
    clock->minute = decimal_to_bcd(t / 60);
    clock->second = decimal_to_bcd(t % 60);
}

time_t time_NTP_to_time_t(u32 time_NTP, int offset, int dst)
{
    time_t unix_time = time_NTP - NTP_TO_Y2K_EPOCH;

    if (dst)
    {
        offset += 60;
    }

    unix_time += offset * 60;

    return unix_time;
}