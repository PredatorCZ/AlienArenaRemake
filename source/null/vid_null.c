/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// vid_null.c -- null video driver to aid porting efforts
// this assumes that one of the refs is statically linked to the executable

#include "../client/client.h"

viddef_t	viddef;				// global video state

/*
==========================================================================

DIRECT LINK GLUE

==========================================================================
*/

void VID_NewWindow (int width, int height)
{
	viddef.width = width;
	viddef.height = height;
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
    int         mode;
} vidmode_t;

vidmode_t vid_modes[] =
{
   	{ "Mode 0: 640x480",   640, 480,   0 },
    { "Mode 1: 800x600",   800, 600,   1 },
    { "Mode 2: 960x720",   960, 720,   2 },
    { "Mode 3: 1024x768",  1024, 768,  3 },
    { "Mode 4: 1152x864",  1152, 864,  4 },
    { "Mode 5: 1280x960",  1280, 960, 5 },
	{ "Mode 6: 1280x1024", 1280, 1024, 6 },
	{ "Mode 7: 1360x768",  1360, 768, 7 },
	{ "Mode 8: 1366x768",  1366, 768, 8 },
    { "Mode 9: 1600x1200", 1600, 1200, 9 },
	{ "Mode 10: 1680x1050", 1680, 1050, 10 },
	{ "Mode 11: 1920x1080", 1920, 1080, 11 },
	{ "Mode 12: 2048x1536", 2048, 1536, 12 }
};
#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
    if ( mode < 0 || mode >= VID_NUM_MODES )
        return false;

    *width  = vid_modes[mode].width;
    *height = vid_modes[mode].height;

    return true;
}


void	VID_Init (void)
{
    refimport_t	ri;

    viddef.width = 320;
    viddef.height = 240;
   
	// call the init function
    if (R_Init (NULL, NULL) == -1)
		Com_Error (ERR_FATAL, "Couldn't start refresh");
}

void	VID_Shutdown (void)
{
    R_Shutdown ();
}

void	VID_CheckChanges (void)
{
}

void	VID_MenuInit (void)
{
}

void	VID_MenuDraw (void)
{
}

const char *VID_MenuKey( int k)
{
	return NULL;
}
