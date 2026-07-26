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
//	Refresh (R_*) module, global header.
//	All the rendering/drawing stuff is here.
//

#ifndef __R_LOCAL__
#define __R_LOCAL__

// Binary Angles, sine/cosine/atan lookups.
#include "tables.h" // IWYU pragma: export

// Screen size related parameters.
#include "doomdef.h" // IWYU pragma: export

// Include the refresh/render data structs.
#include "r_data.h" // IWYU pragma: export



//
// Separate header file for each module.
//
#include "r_main.h" // IWYU pragma: export
#include "r_bsp.h" // IWYU pragma: export
#include "r_segs.h" // IWYU pragma: export
#include "r_plane.h" // IWYU pragma: export
#include "r_data.h" // IWYU pragma: export
#include "r_things.h" // IWYU pragma: export
#include "r_draw.h" // IWYU pragma: export

#endif		// __R_LOCAL__
