// main.c
/*
This file contains the main program for NTPS2. It uses functions from the other
files to get the time from the RTC and from an NTP server and update the time
in addition to displaying everything on the screen and getting user input via
the controller.
*/

// For File IO
#define NEWLIB_PORT_AWARE

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
#include <kernel.h>
#include <fileio.h>
#include <libmc.h>
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

extern unsigned char usbd_irx[];
extern unsigned int size_usbd_irx;

extern unsigned char usbmass_bd_irx[];
extern unsigned int size_usbmass_bd_irx;

extern unsigned char usbhdfsd_irx[];
extern unsigned int size_usbhdfsd_irx;

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
    time_t_to_sceCdCLOCK(ntp_time_t, &ntp_time); // Convert Y2K epoch time to BCD in local timezone
    sceCdWriteClock(&ntp_time);                  // Write RTC with converted time
}

/*
Description: Subroutine parse the specified path of the ELF to be launched using the provided config file.
Inputs:      void
Outputs:     (char *)out_path
Parameters:  (char *)config_file, (char *)out_path
Returns:     int
*/
int parseConfig(char *config_file, char *out_path, int *auto_set)
{

    // Zero out things by default & set a buffer
    char conf_buff[260];
    memset(out_path, 0, 256);
    *auto_set = 0;

    int conf = open(config_file, O_RDONLY); // Open config file in read mode

    // Config file doesn't exist or is inaccessible
    if (conf < 0)
    {
        return -1;
    }

    int bytes = read(conf, conf_buff, sizeof(conf_buff) - 1); // Read config file

    close(conf);

    // Config file is empty or corrupted
    if (bytes <= 0)
    {
        return -2;
    }

    conf_buff[bytes] = '\0'; // Null termination

    size_t len = strlen(conf_buff);

    // Replace any newline terminators with null terminator
    while (len > 0 && (conf_buff[len - 1] == '\n' || conf_buff[len - 1] == '\r' || conf_buff[len - 1] == ' '))
    {
        conf_buff[--len] = '\0';
    }

    char *sep = strchr(conf_buff, ','); // use comma seperator

    // Config broken if there's no seperator
    if (!sep)
    {
        return -3;
    }

    *sep = '\0'; // Split into two strings at seperator with null termiantor

    // Set input flag according to first character
    if (conf_buff[0] == '1')
    {
        *auto_set = 1;
    }
    else
    {
        *auto_set = 0;
    }

    strncpy(out_path, sep + 1, 255); // Copy path into output from buffer

    // Null termination and define start
    out_path[255] = '\0';
    char *path_start = out_path;

    // Remove leading whitespace
    while (*path_start == ' ')
    {
        path_start++;
    }

    // Adjust start of string if needed
    if (path_start != out_path)
    {
        memmove(out_path, path_start, strlen(path_start) + 1);
    }

    return 0;
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

    u8 sleep_time = 2; // Time the system pauses for to display certain information on screen

    // Get local timezone info
    int gmt_offset = configGetTimezone();
    int daylight_savings = configIsDaylightSavingEnabled();

    // Buffer to store controller information
    static char pad_buf[256] __attribute__((aligned(64)));

    // Config file variables for parsing, input flag, and exit ELF
    char config_path[256];
    char launch_file[256];
    int parse = -1;
    int skip_input = 0;

    // Initialize SIF RPC & IOP reset
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
    fioInit();
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    // Load IRX modules for networking & USB storage
    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);
    SifExecModuleBuffer(usbd_irx, size_usbd_irx, 0, NULL, NULL);
    SifExecModuleBuffer(usbmass_bd_irx, size_usbmass_bd_irx, 0, NULL, NULL);
    SifExecModuleBuffer(usbhdfsd_irx, size_usbhdfsd_irx, 0, NULL, NULL);

    // Load built-in modules for MC storage, serial IO, & gamepad
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:MCMAN", 0, NULL);
    SifLoadModule("rom0:MCSERV", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    mcInit(MC_TYPE_MC); // Initialize memory card

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

    // Check for config file in launch path of ELF (working directory)
    if (argc > 0 && argv[0])
    {
        // Get current path and trim to last '/' to exclude ELF file
        strncpy(config_path, argv[0], 256);
        char *trim_point = strrchr(config_path, '/');

        // In case file is at root of storage device, look for ':' instead of '/'
        if (!trim_point)
        {
            trim_point = strrchr(config_path, ':');
        }

        if (trim_point)
        {
            // Null terminate & add config file name to path
            *(trim_point + 1) = '\0';
            strcat(trim_point, "NTPS2.txt");

            parse = parseConfig(config_path, launch_file, &skip_input); // Check if config file is available & parse
        }
    }

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

        // Save time on cross press or exit program on circle press or perform tasks with no input with flag set
        if ((pad_reading & PAD_CROSS) || skip_input)
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

            // Exit on flag set
            if (skip_input)
            {
                screen_printf(text_scale, "\n");
                break;
            }
        }
        else if (pad_reading & PAD_CIRCLE)
        {
            sleep_time = 1; // Adjust refresh rate of display for showing exit message to user
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

    // Close gamepad connection
    padPortClose(0, 0);
    padEnd();

    // Check if config file was parsed earlier
    if (!parse)
    {
        int fd = open(launch_file, O_RDONLY); // Check if ELF specified is accessible

        if (fd >= 0)
        {
            // Exit message & close ELF
            screen_printf(text_scale, "Found config. Exiting to specified ELF...");
            send_frame();
            sleep(sleep_time);
            close(fd);

            // Static variables to store ELF info for launcher
            static t_ExecData elfdata;
            static char *args[1];
            static char launch_copy[256];

            // Copy path to ELF
            memcpy(launch_copy, launch_file, 256);

            // Load ELF into memory
            args[0] = launch_copy;
            int ret = SifLoadElf(launch_file, &elfdata);

            // If load was successful, de-init network modules, exit RPC, & flush cache before launching ELF
            if (ret == 0)
            {
                NetManDeinit();
                ps2ipDeinit();
                SifExitRpc();
                FlushCache(0);
                FlushCache(2);
                ExecPS2((void *)elfdata.epc, (void *)elfdata.gp, 1, args);
            }
        }
    }

    // Exit message for no/incorrect config file
    screen_printf(text_scale, "No config. Exiting to browser...");
    send_frame();
    sleep(sleep_time);

    // De-init network modelues, exit RPC, & flush cache before exiting to OSDSYS browser
    NetManDeinit();
    ps2ipDeinit();
    SifExitRpc();
    FlushCache(0);
    FlushCache(2);
    return 0;
}
