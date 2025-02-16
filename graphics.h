#include <gsKit.h>

extern const u8 grid_y;
extern GSGLOBAL *gsGlobal;
extern float stretch_factor_x;
extern float stretch_factor_y;
extern float offset_y;

void toggle_widescreen(void);
void init_screen(u32 pad_reading);
void draw_bg(void);
void draw_char(char c, float pos_x, float pos_y, float scale);
void screen_printf(float scale, const char *fmt, ...);
void pos_reset(float x, float y);
void send_frame(void);
void mode_switch(void);
