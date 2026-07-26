#ifndef __I_INPUT__
#define __I_INPUT__

#include "doomtype.h"

#define MAX_MOUSE_BUTTONS 3

typedef boolean (*grabmouse_callback_t)(void);

void I_InitInput(void);
void I_SetGrabMouseCallback(grabmouse_callback_t func);

extern float mouse_acceleration;
extern int mouse_threshold;
extern int vanilla_keyboard_mapping;

#endif /* #ifndef __I_INPUT__ */
