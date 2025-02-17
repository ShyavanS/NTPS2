// graphics.h
/*
The header file for graphics.c, exposes the necessary functionality to other
parts of the program for drawing to the screen.
*/

// Include Statements
#include <gsKit.h>

/*
Description: Subroutine to toggle display from standard to widescreen.
             (initiated by triangle button)
Inputs:      (u8)analog, (short)mode, (s8)widescreen
Outputs:     (u8)widescreen, (float)offset_x, (float)stretch_factor_x,
             (float)char_scale_x
Parameters:  void
Returns:     void
*/
void toggle_widescreen(void);

/*
Description: Subroutine to initialize display and load textures.
Inputs:      (unsigned char[])bg_raw, (unsigned char[])font_raw,
             GSTEXTURE(font_texture), (const u8)grid_x
Outputs:     (GSGLOBAL *)gs_global, (short)mode, GSTEXTURE(bg_texture),
             GSTEXTURE(font_texture), (u8)chars_per_row
Parameters:  (u32)pad_reading
Returns:     void
*/
void init_screen(u32 pad_reading);

/*
Description: Subroutine to draw background.
Inputs:      (u64)black, (GSTEXTURE)bg_texture, (float)offset_x, (float)offset_y,
             (float)stretch_factor_x, (float)stretch_factor_y, (u64)tex_col
Outputs:     (GSGLOBAL *)gs_global
Parameters:  void
Returns:     void
*/
void draw_bg(void);

/*
Description: Subroutine to draw characters.
Inputs:      (GSTEXTURE)font_texture, (u8)chars_per_row, (const u8)grid_x,
             (const u8)grid_y, (u64)tex_col
Outputs:     (GSGLOBAL *)gs_global
Parameters:  (char)c, (float)pos_x, (float)pos_y, float(scale_x), float(scale_y)
Returns:     void
*/
void draw_char(char c, float pos_x, float pos_y, float scale_x, float scale_y);

/*
Description: Subroutine to print formatted strings to the screen.
Inputs:      GSTEXTURE(font_texture), (float)char_scale_x, (float)char_scale_y,
             (const u8)grid_x, (const u8)grid_y, (float)pos_x, (float)pos_y
Outputs:     (float)pos_x, (float)pos_y, (char [])buffer
Parameters:  (float)scale, (const char *)fmt, ...
Returns:     void
*/
void screen_printf(float scale, const char *fmt, ...);

/*
Description: Subroutine to reset printing cursor position.
Inputs:      (float)reset_y
Outputs:     (float)pos_x, (float)pos_y
Parameters:  void
Returns:     void
*/
void pos_reset(void);

/*
Description: Subroutine to send a frame over to the PS2 display.
Inputs:      (GSGLOBAL *)gs_global
Outputs:     void
Parameters:  void
Returns:     void
*/
void send_frame(void);

/*
Description: Subroutine to switch drawing modes.
Inputs:      (GSTEXTURE)bg_texture
Outputs:     (GSGLOBAL *)gs_global
Parameters:  void
Returns:     void
*/
void mode_switch(void);
