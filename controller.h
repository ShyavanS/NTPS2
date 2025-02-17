// controller.h
/*
The header file for controller.c, exposes the necessary functionality to other
parts of the program for reading user input.
*/

// Include Statements
#include <tamtypes.h>
#include <libpad.h>

// Struct to hold controller status
extern struct padButtonStatus buttons;

/*
Description: Subroutine to read button status from the player 1 controller.
Inputs:      (struct padButtonStatus)buttons
Outputs:     void
Parameters:  void
Returns:     (u32)new_pad
*/
u32 read_pad(void);
