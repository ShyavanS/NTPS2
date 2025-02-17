// graphics.c
/*
This file contains necessary functionality to display things on the screen.
Mostly re-creating the debug module's scr_printf function in order to also add
a background image for the app.
*/

// Include Statements
#include <stdarg.h>
#include <stdio.h>
#include <dmaKit.h>
#include <tamtypes.h>
#include <libpad.h>
#include "graphics.h"
#include "ntps2_logo.h"
#include "ntps2_font.h"

// Colours set up as unsigned 64-bit integers
u64 black = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00);
u64 tex_col = GS_SETREG_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x00);

u8 chars_per_row;       // Number of characters per row in bitmap font grid
short int mode;         // Video mode of PS2
float stretch_factor_x; // X-scaling factor for background
float offset_x;         // X-offset factor for background
float stretch_factor_y; // Y-scaling factor for background
float offset_y;         // Y-offset factor for background
float char_scale_x;     // X-scaling factor for characters
float char_scale_y;     // Y-scaling factor for characters
float reset_y;          // Reset Y-position for printing to screen

const u8 GRID_X = 28; // Character grid width for bitmap font
const u8 GRID_Y = 40; // Character grid height for bitmap font
s8 widescreen = -1;   // Widescreen flag
u8 analog = 1;        // Analog video flag
float pos_x = 0.0f;   // Starting X-position for printing to screen
float pos_y = 0.0f;   // Starting Y-position for printing to screen

GSTEXTURE bg_texture;   // Background texture
GSTEXTURE font_texture; // Font texture
GSGLOBAL *gs_global;    // Global screen

/*
Description: Subroutine to load embedded textures from c header files.
Inputs:      void
Outputs:     (GSTEXTURE *)texture
Parameters:  (GSTEXTURE *)texture, (int)width, (int)height, (int)bpp, (void *)mem
Returns:     void
*/
void load_embedded_texture(GSTEXTURE *texture, int width, int height, int bpp, void *mem)
{
    texture->Width = width;
    texture->Height = height;
    texture->PSM = (bpp == 32) ? GS_PSM_CT32 : GS_PSM_CT16;
    texture->Vram = gsKit_vram_alloc(gs_global, gsKit_texture_size(width, height, texture->PSM), GSKIT_ALLOC_USERBUFFER);
    texture->Filter = GS_FILTER_LINEAR;
    texture->Mem = mem; // Point to embedded image data
}

/*
Description: Subroutine to set video output to analog NTSC or PAL modes.
Inputs:      (short int)mode
Outputs:     (GSGLOBAL *)gs_global, (float)offset_y, (float)stretch_factor_y,
             (float)offset_x, (float)stretch_factor_x, (float)char_scale_x,
             (float)char_scale_y, (float)reset_y, (float)pos_y
Parameters:  void
Returns:     void
*/
void set_analog_mode(void)
{
    // Set gs_global parameters based on video mode
    gs_global->Mode = mode;
    gs_global->Interlace = GS_INTERLACED;
    gs_global->Field = GS_FIELD;
    gs_global->Width = 640;
    gs_global->Height = (mode == GS_MODE_NTSC) ? 480 : 576;

    // Set texture scaling and offset parameters based on video mode
    offset_y = (mode == GS_MODE_NTSC) ? 0.0f : 14.933333f;
    stretch_factor_y = (mode == GS_MODE_NTSC) ? 0.0f : 96.0f - offset_y;
    offset_x = -22.857143f;
    stretch_factor_x = 22.857143f;
    char_scale_x = 1.071429f;
    char_scale_y = (mode == GS_MODE_NTSC) ? 1.071429f : 1.2f;
    reset_y = pos_y = (mode == GS_MODE_NTSC) ? 18.0f : 35.0f;
}

/*
Description: Subroutine to set video output to digital 480p or 576p modes.
Inputs:      (short int)mode
Outputs:     (GSGLOBAL *)gs_global, (float)offset_y, (float)stretch_factor_y,
             (float)offset_x, (float)stretch_factor_x, (float)char_scale_x,
             (float)char_scale_y, (float)reset_y, (float)pos_y
Parameters:  void
Returns:     void
*/
void set_digital_mode(void)
{
    // Set gs_global parameters based on video mode
    gs_global->Mode = (mode == GS_MODE_NTSC) ? GS_MODE_DTV_480P : GS_MODE_DTV_576P;
    gs_global->Interlace = GS_NONINTERLACED;
    gs_global->Field = GS_FRAME;
    gs_global->Width = 720;
    gs_global->Height = (mode == GS_MODE_NTSC) ? 480 : 576;

    // Set texture scaling and offset parameters based on video mode
    offset_y = (mode == GS_MODE_NTSC) ? -16.0f : -19.2f;
    stretch_factor_y = -offset_y;
    offset_x = (mode == GS_MODE_NTSC) ? 17.142857f : -22.857143f;
    stretch_factor_x = (mode == GS_MODE_NTSC) ? 62.857143f : 22.857143f;
    char_scale_x = (mode == GS_MODE_NTSC) ? 1.0717f : 1.071429f;
    char_scale_y = 1.14f;
    reset_y = pos_y = 3.0f;
}

/*
Description: Subroutine to toggle video mode from analog to digital on startup.
             (initiated by square button)
Inputs:      (u8)analog
Outputs:     (u8)analog
Parameters:  void
Returns:     void
*/
void toggle_mode(void)
{
    if (analog)
    {
        set_digital_mode();
        analog = 0;
    }
    else
    {
        set_analog_mode();
        analog = 1;
    }
}

/*
Description: Subroutine to toggle display from standard to widescreen.
             (initiated by triangle button)
Inputs:      (u8)analog, (short)mode, (s8)widescreen
Outputs:     (u8)widescreen, (float)offset_x, (float)stretch_factor_x,
             (float)char_scale_x
Parameters:  void
Returns:     void
*/
void toggle_widescreen(void)
{
    if (!analog && mode == GS_MODE_NTSC)
    {
        stretch_factor_x += widescreen * 56.25f;
        offset_x += -widescreen * 56.25f;
    }
    else
    {
        stretch_factor_x += widescreen * 80.0f;
        offset_x += -widescreen * 80.0f;
    }

    if (char_scale_x == 1.0717f && widescreen == -1)
    {
        char_scale_x = 0.94375f;
    }
    else if (char_scale_x == 1.071429f && widescreen == -1)
    {
        char_scale_x = 0.84375f;
    }
    else if (char_scale_x == 0.94375f && widescreen == 1)
    {
        char_scale_x = 1.0717f;
    }
    else if (char_scale_x == 0.84375f && widescreen == 1)
    {
        char_scale_x = 1.071429f;
    }

    widescreen *= -1;
}

/*
Description: Subroutine to initialize display and load textures.
Inputs:      (unsigned char[])bg_raw, (unsigned char[])font_raw,
             GSTEXTURE(font_texture), (const u8)GRID_X
Outputs:     (GSGLOBAL *)gs_global, (short)mode, GSTEXTURE(bg_texture),
             GSTEXTURE(font_texture), (u8)chars_per_row
Parameters:  (u32)pad_reading
Returns:     void
*/
void init_screen(u32 pad_reading)
{

    mode = gsKit_check_rom();        // Detect video mode based on ROM version
    gs_global = gsKit_init_global(); // Initialize gs_global

    // Apply necessary parameters for screen to ensure proper display
    gs_global->PSM = GS_PSM_CT32;
    gs_global->PSMZ = GS_PSMZ_32;
    gs_global->PrimAAEnable = GS_SETTING_ON;
    gs_global->DoubleBuffering = GS_SETTING_OFF;
    gs_global->ZBuffering = GS_SETTING_OFF;
    gs_global->PrimAlphaEnable = GS_SETTING_ON;

    // Initialize to analog display mode
    set_analog_mode();

    // Toggle mode and widescreen on user input
    if (pad_reading & PAD_SQUARE)
    {
        toggle_mode();
    }
    if (pad_reading & PAD_TRIANGLE)
    {
        toggle_widescreen();
    }

    // Setup dmaKit
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
    dmaKit_chan_init(DMA_CHANNEL_TOSPR);

    gsKit_init_screen(gs_global);                                 // Initialize screen
    load_embedded_texture(&bg_texture, 640, 480, 32, bg_raw);     // Load background
    load_embedded_texture(&font_texture, 532, 200, 32, font_raw); // Load font
    gsKit_texture_upload(gs_global, &bg_texture);                 // Upload background VRAM
    gsKit_texture_upload(gs_global, &font_texture);               // Upload font to VRAM
    gsKit_mode_switch(gs_global, GS_PERSISTENT);                  // Set draw mode to persistent

    chars_per_row = font_texture.Width / GRID_X; // Calculate number of characters per row in bitmap font
}

/*
Description: Subroutine to draw background.
Inputs:      (u64)black, (GSTEXTURE)bg_texture, (float)offset_x, (float)offset_y,
             (float)stretch_factor_x, (float)stretch_factor_y, (u64)tex_col
Outputs:     (GSGLOBAL *)gs_global
Parameters:  void
Returns:     void
*/
void draw_bg(void)
{
    // Clear screen
    gsKit_clear(gs_global, black);

    // Set alpha blending parameters
    gsKit_set_primalpha(gs_global, GS_BLEND_FRONT2BACK, 1);
    gsKit_set_test(gs_global, GS_ATEST_OFF);

    // Draw texture
    gsKit_prim_sprite_texture(gs_global, &bg_texture, offset_x, offset_y, 0.0f, 0.0f, (float)bg_texture.Width + stretch_factor_x, (float)bg_texture.Height + stretch_factor_y, (float)bg_texture.Width, (float)bg_texture.Height, 2, tex_col);
}

/*
Description: Subroutine to draw characters.
Inputs:      (GSTEXTURE)font_texture, (u8)chars_per_row, (const u8)GRID_X,
             (const u8)GRID_Y, (u64)tex_col
Outputs:     (GSGLOBAL *)gs_global
Parameters:  (char)c, (float)pos_x, (float)pos_y, float(scale_x), float(scale_y)
Returns:     void
*/
void draw_char(char c, float pos_x, float pos_y, float scale_x, float scale_y)
{
    // Convert ASCII index of char to index on bitmap font grid
    u8 char_index = c - ' ';

    // Find coordinates of char on grid
    int col = char_index % chars_per_row;
    int row = char_index / chars_per_row;

    // Compute texture coordinates
    float u0 = col * GRID_X;
    float v0 = row * GRID_Y;
    float u1 = u0 + GRID_X;
    float v1 = v0 + GRID_Y;

    // Set alpha blending parameters and draw texture
    gsKit_set_primalpha(gs_global, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs_global, GS_ATEST_OFF);
    gsKit_prim_sprite_texture(gs_global, &font_texture, pos_x, pos_y, u0, v0, pos_x + GRID_X * scale_x, pos_y + GRID_Y * scale_y, u1, v1, 3, tex_col);
    gsKit_set_test(gs_global, GS_ATEST_ON);
    gsKit_set_primalpha(gs_global, GS_BLEND_BACK2FRONT, 0);
}

/*
Description: Subroutine to print formatted strings to the screen.
Inputs:      GSTEXTURE(font_texture), (float)char_scale_x, (float)char_scale_y,
             (const u8)GRID_X, (const u8)GRID_Y, (float)pos_x, (float)pos_y
Outputs:     (float)pos_x, (float)pos_y, (char [])buffer
Parameters:  (float)scale, (const char *)fmt, ...
Returns:     void
*/
void screen_printf(float scale, const char *fmt, ...)
{
    char buffer[256]; // Buffer to hold formatted text
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args); // Format the string and output to buffer
    va_end(args);

    // Adjust scaling factors
    float scale_x = scale * char_scale_x;
    float scale_y = scale * char_scale_y;

    // Iterate over char array and print each char
    for (int i = 0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] == '\n')
        {
            pos_y += GRID_Y * scale_y; // Move to the next line
            pos_x = 0.0f;              // Reset X position
        }
        else if (buffer[i] == '\t')
        {
            pos_x += 4 * GRID_X * scale_x; // Move X to make tab character
        }
        else if (buffer[i] == ' ')
        {
            pos_x += GRID_X * scale_x; // Move X to make space character
        }
        else
        {
            draw_char(buffer[i], pos_x, pos_y, scale_x, scale_y); // Draw character
            pos_x += GRID_X * scale_x;                            // Move X for next character
        }
    }
}

/*
Description: Subroutine to reset printing cursor position.
Inputs:      (float)reset_y
Outputs:     (float)pos_x, (float)pos_y
Parameters:  void
Returns:     void
*/
void pos_reset(void)
{
    pos_x = 0.0f;
    pos_y = reset_y;
}

/*
Description: Subroutine to send a frame over to the PS2 display.
Inputs:      (GSGLOBAL *)gs_global
Outputs:     void
Parameters:  void
Returns:     void
*/
void send_frame(void)
{
    gsKit_queue_exec(gs_global); // Execute draw queue
    gsKit_sync_flip(gs_global);  // Flip framebuffer on VSync
}

/*
Description: Subroutine to switch drawing modes.
Inputs:      (GSTEXTURE)bg_texture
Outputs:     (GSGLOBAL *)gs_global
Parameters:  void
Returns:     void
*/
void mode_switch(void)
{
    gsKit_sync_flip(gs_global);               // Flip framebuffer on VSync
    gsKit_mode_switch(gs_global, GS_ONESHOT); // Set draw mode to oneshot
    gsKit_clear(gs_global, black);            // Clear screen
}
