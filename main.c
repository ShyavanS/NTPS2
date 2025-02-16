/*
TODO:
- Transparency
- Scaling text
- Documentation
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <debug.h>
#include <ps2ip.h>
#include <netman.h>
#include <time.h>
#include "time_math.h"
#include "net.h"
#include "ntp.h"
#include "controller.h"
#include "graphics.h"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

void getSystemTime(int offset, int dst)
{
    sceCdReadClock(&rtcTime);
    configConvertToGmtTime(&rtcTime);

    if (dst)
    {
        offset += 60;
    }

    time_t t = sceCdCLOCK_to_time_t(&rtcTime);
    t += offset * 60; // Apply offset in seconds
    time_t_to_sceCdCLOCK(t, &rtcTime);
}

void setSystemTime(time_t ntp_time)
{
    time_t_to_sceCdCLOCK(ntp_time, &ntpTime);

    sceCdWriteClock(&ntpTime);
}

int main(int argc, char *argv[])
{
    clock_t tic;
    clock_t toc;
    time_t ps2_epoch;
    time_t adjusted_epoch;
    u32 epoch;
    u32 pad_reading;

    u8 sleep_time = 2;
    float scale = 0.4f;

    int gmt_offset = configGetTimezone();
    int daylight_savings = configIsDaylightSavingEnabled();

    static char padBuf[256] __attribute__((aligned(64)));

    SifInitRpc(0);
    while (!SifIopReset("", 0)) {};
    while (!SifIopSync()) {};

    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();
    sbv_patch_enable_lmb();

    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);

    padInit(0);
    padPortOpen(0, 0, padBuf);

    while (padGetState(0, 0) != 6) {};

    pad_reading = read_pad();

    init_screen(pad_reading);
    draw_bg();
    send_frame();
    NetManInit();
    load_ipconfig();
    send_frame();
    sleep(sleep_time);
    mode_switch();

    // get epoch time
    epoch = get_ntp_time();
    tic = clock();
    ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);

    while (1)
    {
        sleep_time = 0;

        draw_bg();
        screen_printf(scale, "NTPS2: A Bare-Bones NTP Client for the PS2\n");
        screen_printf(scale, "------------------------------------------\n");
        screen_printf(scale, "X: Save O: Exit\nA: Toggle Widescreen\n#: Toggle A->D (on startup)\n");
        screen_printf(scale, "------------------------------------------\n\n");
        
        getSystemTime(gmt_offset, daylight_savings);

        screen_printf(scale, "Current System Time:\n");
        screen_printf(scale, "%04d-%02d-%02d %02d:%02d:%02d\n\n", bcd_to_decimal(rtcTime.year) + 2000, bcd_to_decimal(rtcTime.month), bcd_to_decimal(rtcTime.day), bcd_to_decimal(rtcTime.hour), bcd_to_decimal(rtcTime.minute), bcd_to_decimal(rtcTime.second));

        toc = clock();
        adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;

        time_t_to_sceCdCLOCK(adjusted_epoch, &ntpTime);

        screen_printf(scale, "Proposed New System Time:\n");
        screen_printf(scale, "%04d-%02d-%02d %02d:%02d:%02d\n\n", bcd_to_decimal(ntpTime.year) + 2000, bcd_to_decimal(ntpTime.month), bcd_to_decimal(ntpTime.day), bcd_to_decimal(ntpTime.hour), bcd_to_decimal(ntpTime.minute), bcd_to_decimal(ntpTime.second));

        while (padGetState(0, 0) != 6) {};

        pad_reading = read_pad();

        if (pad_reading & PAD_TRIANGLE) {
            toggle_widescreen();
        }

        if (pad_reading & PAD_CROSS)
        {
            sleep_time = 1;
            ps2_epoch = time_NTP_to_time_t(epoch, 540, 0);
            toc = clock();
            adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;
            setSystemTime(adjusted_epoch);

            screen_printf(scale, "Saved!");

            // get epoch time
            epoch = get_ntp_time();
            tic = clock();
            ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);
        }
        else if (pad_reading & PAD_CIRCLE)
        {
            sleep_time = 1;
            screen_printf(scale, "Exiting...");
            send_frame();
            sleep(sleep_time);
            ps2ipDeinit();

            break;
        }

        send_frame();
        pos_reset(0.0f, 20.0f);

        if (sleep_time) {
            sleep(sleep_time);
        }
    }

    padPortClose(0, 0);
    padEnd();

    return 0;
}
