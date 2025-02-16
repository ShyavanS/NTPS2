#include <stdarg.h>
#include <stdio.h>
#include <dmaKit.h>
#include <tamtypes.h>
#include <libpad.h>
#include "graphics.h"
#include "ntps2_logo.h"
#include "ntps2_font.h"

u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);
u64 Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
u64 TexCol = GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00);

s8 widescreen = -1;
u8 analog = 1;
short int mode;
const u8 grid_x = 35;
const u8 grid_y = 50;
u8 chars_per_row;
float stretch_factor_x;
float offset_x;
float stretch_factor_y;
float offset_y;
float pos_x = 0.0f;
float pos_y = 20.0f;

GSTEXTURE bg_texture;
GSTEXTURE font_texture;
GSGLOBAL *gsGlobal;

void load_embedded_texture(GSTEXTURE *texture, int width, int height, int bpp, void * mem) {
    texture->Width = width;
    texture->Height = height;
    texture->PSM = (bpp == 32) ? GS_PSM_CT32 : GS_PSM_CT16;
    texture->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(width, height, texture->PSM), GSKIT_ALLOC_USERBUFFER);
    texture->Filter = GS_FILTER_LINEAR;
    texture->Mem = mem; // Point to embedded image data
}

void set_analog_mode(void) {
    gsGlobal->Mode = mode;
    gsGlobal->Interlace = GS_INTERLACED;
    gsGlobal->Field = GS_FIELD;
    gsGlobal->Width = 640;
    gsGlobal->Height = (mode == GS_MODE_NTSC) ? 480 : 576;

    offset_y = (mode == GS_MODE_NTSC) ? 0.0f : 14.933333f;
    stretch_factor_y = (mode == GS_MODE_NTSC) ? 0.0f : 96.0f - offset_y;
    offset_x = -22.857143f;
    stretch_factor_x = 22.857143f;
}

void set_digital_mode(void) {
    gsGlobal->Mode = (mode == GS_MODE_NTSC) ? GS_MODE_DTV_480P : GS_MODE_DTV_576P;
    gsGlobal->Interlace = GS_NONINTERLACED;
    gsGlobal->Field = GS_FRAME;
    gsGlobal->Width = 720;
    gsGlobal->Height = (mode == GS_MODE_NTSC) ? 480 : 576;

    offset_y = (mode == GS_MODE_NTSC) ? -16.0f : -19.2f;
    stretch_factor_y = -offset_y;
    offset_x = (mode == GS_MODE_NTSC) ? 17.142857f : -22.857143f;
    stretch_factor_x = (mode == GS_MODE_NTSC) ? 62.857143f : 22.857143f;
}

void toggle_mode(void) {
    if (analog) {
        set_digital_mode();
        analog = 0;
    } else {
        set_analog_mode();
        analog = 1;
    }
}

void toggle_widescreen(void) {
    if (!analog && mode == GS_MODE_NTSC) {
        stretch_factor_x += widescreen * 56.25f;
        offset_x += -widescreen * 56.25f;
    } else {
        stretch_factor_x += widescreen * 80.0f;
        offset_x += -widescreen * 80.0f;
    }
    
    widescreen *= -1;
}

void init_screen(u32 pad_reading) {
    mode = gsKit_check_rom();
    gsGlobal = gsKit_init_global();

    // Auto-detect and apply correct video mode
    gsGlobal->PSM = GS_PSM_CT32;
    gsGlobal->PSMZ = GS_PSMZ_32;
    gsGlobal->PrimAAEnable = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	gsGlobal->ZBuffering = GS_SETTING_OFF;
    
    set_analog_mode();

    if (pad_reading & PAD_SQUARE) {
        toggle_mode();
    }
    if (pad_reading & PAD_TRIANGLE) {
        toggle_widescreen();
    }
    
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
    dmaKit_chan_init(DMA_CHANNEL_TOSPR);

    gsKit_init_screen(gsGlobal);
    load_embedded_texture(&bg_texture, 640, 480, 32, bg_raw);
    load_embedded_texture(&font_texture, 665, 250, 32, font_raw);
    gsKit_texture_upload(gsGlobal, &bg_texture);
    gsKit_texture_upload(gsGlobal, &font_texture);
    gsKit_mode_switch(gsGlobal, GS_PERSISTENT);

    chars_per_row = font_texture.Width / grid_x;
}

void draw_bg(void) {
    // Load the background texture
    gsKit_clear(gsGlobal, Black);
    gsKit_prim_sprite_texture(gsGlobal, &bg_texture, offset_x, offset_y, 0.0f, 0.0f, (float)bg_texture.Width + stretch_factor_x, (float)bg_texture.Height + stretch_factor_y, (float)bg_texture.Width, (float)bg_texture.Height, 2, TexCol);
}

void draw_char(char c, float pos_x, float pos_y, float scale) {
    u8 char_index = c - ' ';

    // Find coordinates of char
    int col = char_index % chars_per_row;
    int row = char_index / chars_per_row;

    // Compute texture coordinates
    float u0 = col * grid_x;
    float v0 = row * grid_y;
    float u1 = u0 + grid_x;
    float v1 = v0 + grid_y;

    // gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 1);
    gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 1);
	// gsKit_set_test(gsGlobal, GS_ATEST_ON);
    gsKit_prim_sprite_texture(gsGlobal, &font_texture, pos_x, pos_y, u0, v0, pos_x + grid_x * scale, pos_y + grid_y * scale, u1, v1, 3, TexCol);
	// gsKit_set_test(gsGlobal, GS_ATEST_ON);
	gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 1);
}

void screen_printf(float scale, const char *fmt, ...) {
    char buffer[256];  // Buffer to hold formatted text
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\n') {  
            pos_y += grid_y * scale;  // Move to the next line
            pos_x = 0.0f;  // Reset X position
        } else if (buffer[i] == '\t') {
            pos_x += 4 * grid_x * scale;  // Move X to make tab character
        } else if (buffer[i] == ' ') {
            pos_x += grid_x * scale;
        } else {
            draw_char(buffer[i], pos_x, pos_y, scale);
            pos_x += grid_x * scale;  // Move X for next character
        }
    }
}

void pos_reset(float x, float y) {
    pos_x = x;
    pos_y = y;
}

void send_frame(void) {
    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
}

void mode_switch(void) {
    gsKit_sync_flip(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);
    gsKit_clear(gsGlobal, Black);
    gsKit_texture_upload(gsGlobal, &bg_texture);
}
