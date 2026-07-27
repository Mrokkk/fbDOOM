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
//       Key definitions
//

#ifndef __RL_DOOMKEYS__
#define __RL_DOOMKEYS__

// FIXME: hack for Raylib and Doom using same names for keys

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define D_KEY_RIGHTARROW    0xae
#define D_KEY_LEFTARROW     0xac
#define D_KEY_UPARROW       0xad
#define D_KEY_DOWNARROW     0xaf
#define D_KEY_STRAFE_L      0xa0
#define D_KEY_STRAFE_R      0xa1
#define D_KEY_USE           0xa2
#define D_KEY_FIRE          0xa3
#define D_KEY_ESCAPE        27
#define D_KEY_ENTER         13
#define D_KEY_TAB           9
#define D_KEY_F1            (0x80+0x3b)
#define D_KEY_F2            (0x80+0x3c)
#define D_KEY_F3            (0x80+0x3d)
#define D_KEY_F4            (0x80+0x3e)
#define D_KEY_F5            (0x80+0x3f)
#define D_KEY_F6            (0x80+0x40)
#define D_KEY_F7            (0x80+0x41)
#define D_KEY_F8            (0x80+0x42)
#define D_KEY_F9            (0x80+0x43)
#define D_KEY_F10           (0x80+0x44)
#define D_KEY_F11           (0x80+0x57)
#define D_KEY_F12           (0x80+0x58)

#define D_KEY_BACKSPACE     0x7f
#define D_KEY_PAUSE         0xff

#define D_KEY_EQUALS        0x3d
#define D_KEY_MINUS         0x2d

#define D_KEY_RSHIFT        (0x80+0x36)
#define D_KEY_RCTRL         (0x80+0x1d)
#define D_KEY_RALT          (0x80+0x38)

#define D_KEY_LALT          D_KEY_RALT

// new keys:

#define D_KEY_CAPSLOCK      (0x80+0x3a)
#define D_KEY_NUMLOCK       (0x80+0x45)
#define D_KEY_SCRLCK        (0x80+0x46)
#define D_KEY_PRTSCR        (0x80+0x59)

#define D_KEY_HOME          (0x80+0x47)
#define D_KEY_END           (0x80+0x4f)
#define D_KEY_PGUP          (0x80+0x49)
#define D_KEY_PGDN          (0x80+0x51)
#define D_KEY_INS           (0x80+0x52)
#define D_KEY_DEL           (0x80+0x53)

#define D_KEYP_0            0
#define D_KEYP_1            D_KEY_END
#define D_KEYP_2            D_KEY_DOWNARROW
#define D_KEYP_3            D_KEY_PGDN
#define D_KEYP_4            D_KEY_LEFTARROW
#define D_KEYP_5            '5'
#define D_KEYP_6            D_KEY_RIGHTARROW
#define D_KEYP_7            D_KEY_HOME
#define D_KEYP_8            D_KEY_UPARROW
#define D_KEYP_9            D_KEY_PGUP

#define D_KEYP_DIVIDE       '/'
#define D_KEYP_PLUS         '+'
#define D_KEYP_MINUS        '-'
#define D_KEYP_MULTIPLY     '*'
#define D_KEYP_PERIOD       0
#define D_KEYP_EQUALS       D_KEY_EQUALS
#define D_KEYP_ENTER        D_KEY_ENTER

#endif          // __RL_DOOMKEYS__
