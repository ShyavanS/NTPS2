#include "controller.h"

struct padButtonStatus buttons;

u32 read_pad(void)
{
    u32 pad_data;
    u32 new_pad;
    static u32 old_pad = 0;

    padRead(0, 0, &buttons);

    pad_data = 0xffff ^ buttons.btns;
    new_pad = pad_data & ~old_pad;
    old_pad = pad_data;

    return new_pad;
}