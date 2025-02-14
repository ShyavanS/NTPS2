/*
TODO:
- Add background image
- Documentation
*/

// #include <gsKit.h>
// #include <gsToolkit.h>
// #include <dmaKit.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <unistd.h>
#include <debug.h>
#include <ps2ip.h>
#include <netman.h>
#include <time.h>
#include "time_math.h"
#include "net.h"
#include "ntp.h"
#include "controller.h"
// #include "ntps2_logo.h"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

// GSTEXTURE bg_texture;

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

// void load_embedded_texture(GSTEXTURE *texture, int width, int height, int bpp) {
//     texture->Width = width;
//     texture->Height = height;
//     texture->PSM = (bpp == 32) ? GS_PSM_CT32 : GS_PSM_CT16;
//     texture->Vram = 0;
//     texture->Filter = GS_FILTER_LINEAR;
//     texture->Mem = (void *)bg_raw; // Point to embedded image data
// }

int main(int argc, char *argv[])
{
    clock_t tic;
    clock_t toc;
    time_t ps2_epoch;
    time_t adjusted_epoch;
    u32 epoch;
    u32 pad_reading;
    int y_pos;

    int gmt_offset = configGetTimezone();
    int daylight_savings = configIsDaylightSavingEnabled();

    static char padBuf[256] __attribute__((aligned(64)));

    // u64 Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
    // u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);

    // GSGLOBAL *gsGlobal = gsKit_init_global();
    
    // Auto-detect and apply correct video mode
    // gsGlobal->Mode = gsKit_detect_signal();
    // gsGlobal->PrimAAEnable = GS_SETTING_ON;
    
    // GSFONT *gsFont = gsKit_init_font(GSKIT_FTYPE_BMP_DAT, "host:emotionengine.png");
    
    // dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    // dmaKit_chan_init(DMA_CHANNEL_GIF);
    // dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
    // dmaKit_chan_init(DMA_CHANNEL_TOSPR);

    SifInitRpc(0);
    while(!SifIopReset("", 0)){};
    while(!SifIopSync()){};

    SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb();

    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
	SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);

    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    
    NetManInit();

    init_scr();

    scr_printf("Loading...");

    load_ipconfig();

    // gsKit_init_screen(gsGlobal);
    // gsKit_font_upload(gsGlobal, gsFont);
    // gsKit_mode_switch(gsGlobal, GS_ONESHOT);
    // gsFont->Texture->Filter = GS_FILTER_LINEAR;

    // Load the background texture
    // load_embedded_texture(&bg_texture, 640, 448, 32);
    // gsKit_texture_upload(gsGlobal, &bg_texture);

    padInit(0);
    padPortOpen(0, 0, padBuf);

    scr_setXY(0, 0);
    scr_clear();
    scr_printf("NTPS2: A Bare-Bones NTP Client for the PS2\n");
    scr_printf("******************************************\n");
    scr_printf("X: Save, O: Exit\n");
    scr_printf("******************************************\n\n");
    
    // get epoch time
    epoch = get_ntp_time();
    // epoch = 3333333333;
    tic = clock();
    y_pos = scr_getY();
    ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);

    while (1)
    {
        // gsKit_font_print_scaled(gsGlobal, gsFont, 100, 20, 1, 2.0f, White, "TEST");
        // gsKit_texture_upload(gsGlobal, &bg_texture);
        // gsKit_sync_flip(gsGlobal);
        // gsKit_queue_exec(gsGlobal);

        getSystemTime(gmt_offset, daylight_savings);

        scr_setXY(0, y_pos);
        scr_printf("Current System Time:\n");
        scr_printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                   bcd_to_decimal(rtcTime.year) + 2000, bcd_to_decimal(rtcTime.month), bcd_to_decimal(rtcTime.day),
                   bcd_to_decimal(rtcTime.hour), bcd_to_decimal(rtcTime.minute), bcd_to_decimal(rtcTime.second));

        toc = clock();
        adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;

        time_t_to_sceCdCLOCK(adjusted_epoch, &ntpTime);
        
        scr_printf("Proposed New System Time:\n");
        scr_printf("%04d-%02d-%02d %02d:%02d:%02d\n\n",
                   bcd_to_decimal(ntpTime.year) + 2000, bcd_to_decimal(ntpTime.month), bcd_to_decimal(ntpTime.day),
                   bcd_to_decimal(ntpTime.hour), bcd_to_decimal(ntpTime.minute), bcd_to_decimal(ntpTime.second));

        while (padGetState(0, 0) != 6) {}

        pad_reading = read_pad();

        if (pad_reading & PAD_CROSS)
        {
            scr_printf("Saving...\n");

            ps2_epoch = time_NTP_to_time_t(epoch, 540, 0);
            toc = clock();
            adjusted_epoch = ps2_epoch + (toc - tic) / CLOCKS_PER_SEC;
            setSystemTime(adjusted_epoch);
            scr_printf("Saved!\n");
            sleep(1);
            scr_clear();
            scr_setXY(0, 0);
            scr_printf("NTPS2: A Bare-Bones NTP Client for the PS2\n");
            scr_printf("******************************************\n");
            scr_printf("X: Save, O: Exit\n");
            scr_printf("******************************************\n\n");
            // get epoch time
            epoch = get_ntp_time();
            // epoch = 3333333333;
            tic = clock();
            ps2_epoch = time_NTP_to_time_t(epoch, gmt_offset, daylight_savings);
        }
        else if (pad_reading & PAD_CIRCLE)
        {
            scr_printf("Exiting...\n");
            ps2ipDeinit();
            sleep(1);
            break;
        }
    }

    padPortClose(0, 0);
    padEnd();

    return 0;
}
