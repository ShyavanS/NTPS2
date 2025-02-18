// main.c
/*
This file contains the main program for NTPS2. It uses functions from the other
files to get the time from the RTC and from an NTP server and update the time
in addition to displaying everything on the screen and getting user input via
the controller.
*/

// Include Statements
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <ps2ip.h>
#include <netman.h>
#include <time.h>
#include "time_math.h"
#include "net.h"
#include "ntp.h"
#include "controller.h"
#include "graphics.h"

// Importing data from necessary modules
extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

/*
Description: Subroutine to get the system time from the RTC and convert to the local timezone.
Inputs:      void
Outputs:     (sceCdCLOCK)rtc_time
Parameters:  (int)offset, (int)dst
Returns:     void
*/
void getSystemTime(int offset, int dst)
{
    sceCdReadClock(&rtc_time);         // Read RTC
    configConvertToGmtTime(&rtc_time); // Convert from JST to GMT

    // Apply DST to offset if needed
    if (dst)
    {
        offset += 60;
    }

    time_t t = sceCdCLOCK_to_time_t(&rtc_time); // Convert to Y2K epoch time
    t += offset * 60;                           // Apply offset in seconds
    time_t_to_sceCdCLOCK(t, &rtc_time);         // Convert back to BCD and store in local timezone
}

/*
Description: Subroutine to set the system time and write to the RTC and convert to JST.
Inputs:      void
Outputs:     (sceCdCLOCK)ntp_time
Parameters:  (time_t)ntp_time_t
Returns:     void
*/
void setSystemTime(time_t ntp_time_t)
{
    time_t_to_sceCdCLOCK(ntp_time_t, &ntp_time);

    sceCdWriteClock(&ntp_time);
}

/*
Description: Subroutine for main program, will repeatedly loop to do all tasks the program requires.
Inputs:      void
Outputs:     void
Parameters:  (int)argc, (char[] *)argv
Returns:     int
*/
int main(int argc, char *argv[])
{
    // Keeping track of elapsed time between getting and setting NTP time
    clock_t tic;
    clock_t toc;

    // Storing epoch times
    time_t ps2_epoch;
    time_t adjusted_epoch;
    u32 epoch;

    // Storing controller state
    u32 pad_reading;

    u8 sleep_time = 2;       // Time the system pauses for to display certain information on screen
    float text_scale = 0.5f; // Scale factor to print out time info on screen

    // Get local timezone info
    int gmt_offset = configGetTimezone();
    int daylight_savings = configIsDaylightSavingEnabled();

    // Buffer to store controller information
    static char pad_buf[256] __attribute__((aligned(64)));

    // Initialize SIF RPC
    SifInitRpc(0);
    while (!SifIopReset("", 0))
    {
    };
    while (!SifIopSync())
    {
    };

    // Initialize associated modules and patches
    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();
    sbv_patch_enable_lmb();

    // Load networking modules
    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);

    // Load IO modules
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    // Initialize player 1 controller
    padInit(0);
    padPortOpen(0, 0, pad_buf);

    // Wait for controller to be ready
    while (padGetState(0, 0) != 6)
    {
    };

    pad_reading = read_pad(); // Read controller state

    init_screen(pad_reading); // Initialize screen and video output
    draw_bg();                // Draw background image
    send_frame();             // Render image
    NetManInit();             // Initialize NetMan module
    load_ipconfig();          // Get IP configuration
    send_frame();             // Render output of IP configuration
    pos_reset();              // Reset print cursor position
    sleep(sleep_time);        // Wait for user to view info before clearing screen
    mode_switch();            // Switch from persistent to oneshot draw queue for looping updates

    epoch = get_ntp_time();                                              // Get NTP epoch time
    tic = clock();                                                       // Start counting time elapsed
    ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings); // Convert to PS2 epoch in local timezone

    while (1)
    {
        sleep_time = 0; // Dont wait to update display unless necessary

        draw_bg(); // Draw background image

        // Print out user info, A means triangle & # means square
        screen_printf(text_scale, "NTPS2: A Bare-Bones NTP Client for the PS2\n");
        screen_printf(text_scale, "------------------------------------------\n");
        screen_printf(text_scale, "X: Save O: Exit\nA: Toggle Widescreen\n#: Toggle A->D (on startup)\n");
        screen_printf(text_scale, "------------------------------------------\n\n");

        getSystemTime(gmt_offset, daylight_savings); // Get RTC time

        // Print out current RTC time
        screen_printf(text_scale, "Current System Time:\n");
        screen_printf(text_scale, "%04d-%02d-%02d %02d:%02d:%02d\n\n", bcd_to_decimal(rtc_time.year) + 2000, bcd_to_decimal(rtc_time.month), bcd_to_decimal(rtc_time.day), bcd_to_decimal(rtc_time.hour), bcd_to_decimal(rtc_time.minute), bcd_to_decimal(rtc_time.second));

        toc = clock();                                             // Stop counting time elapsed
        adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC; // Add time elapsed to NTP time

        time_t_to_sceCdCLOCK(adjusted_epoch, &ntp_time); // Convert NTP time to PS2 format including elapsed time

        // Print out time retrieved from NTP server
        screen_printf(text_scale, "Proposed New System Time:\n");
        screen_printf(text_scale, "%04d-%02d-%02d %02d:%02d:%02d\n\n", bcd_to_decimal(ntp_time.year) + 2000, bcd_to_decimal(ntp_time.month), bcd_to_decimal(ntp_time.day), bcd_to_decimal(ntp_time.hour), bcd_to_decimal(ntp_time.minute), bcd_to_decimal(ntp_time.second));

        // Wait for controler to be ready
        while (padGetState(0, 0) != 6)
        {
        };

        pad_reading = read_pad(); // Read controller state

        // Toggle widescreen on triangle press
        if (pad_reading & PAD_TRIANGLE)
        {
            toggle_widescreen();
        }

        // Save time on cross press or exit program on circle press
        if (pad_reading & PAD_CROSS)
        {
            sleep_time = 1; // Wait to update display with "save" message

            ps2_epoch = time_NTP_to_time_t(epoch, 540, 0);             // Convert to PS2 epoch in JST
            toc = clock();                                             // Update time elapsed
            adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC; // Recalculate epoch
            setSystemTime(adjusted_epoch);                             // Set RTC with new time

            screen_printf(text_scale, "Saved!");

            epoch = get_ntp_time();                                              // Get time from NTP server if user wants to set time again
            tic = clock();                                                       // Re-start elapsed time counter
            ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings); // Convert to PS2 epoch in local timezone
        }
        else if (pad_reading & PAD_CIRCLE)
        {
            sleep_time = 1; // Wait to update display with "exit" message

            screen_printf(text_scale, "Exiting...");

            send_frame();
            sleep(sleep_time);
            ps2ipDeinit();

            break;
        }

        send_frame();
        pos_reset(); // Reset print cursor position on each loop to start from top

        // Sleep if needed to wait for user to view info on screen
        if (sleep_time)
        {
            sleep(sleep_time);
        }
    }

    padPortClose(0, 0);
    padEnd();

    return 0;
}
