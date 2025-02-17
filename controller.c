// controller.c
/*
This file contains necessary functionality to read buttons for user input.
*/

// Include Statements
#include "controller.h"

// Struct to hold controller status
struct padButtonStatus buttons;

/*
Description: Subroutine to read button status from the player 1 controller.
Inputs:      (struct padButtonStatus)buttons
Outputs:     void
Parameters:  void
Returns:     (u32)new_pad
*/
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
