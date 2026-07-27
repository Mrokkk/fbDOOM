//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  DOOM keyboard input
//

#include "doomtype.h"
#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "platform/platform.h"

int vanilla_keyboard_mapping = 1;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

int usemouse = 1;
float mouse_acceleration = 3.0;
int mouse_threshold = 2;

void I_StartTic(void)
{
    I_Platform_ReadEvents();
}

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
    UNUSED(func);
}

void I_InitInput(void)
{
    I_AtExit(&I_Platform_ShutdownInput, true);
    I_Platform_InitInput();
}
