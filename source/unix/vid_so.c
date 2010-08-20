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
// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#if defined HAVE_DLFCN_H
#include <dlfcn.h> // ELF dl loader
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "client/client.h"

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_xpos;			// X coordinate of window position
cvar_t		*vid_ypos;			// Y coordinate of window position
cvar_t		*vid_fullscreen;
cvar_t		*vid_width;
cvar_t		*vid_height;
extern cvar_t	*vid_ref;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules

qboolean vid_restart = false;
qboolean vid_active = false;

//==========================================================================

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	vid_restart = true;
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
	{ "Mode 6: 1360x768",  1360, 768, 6 },
    { "Mode 7: 1600x1200", 1600, 1200, 7 },
	{ "Mode 8: 1680x1050", 1680, 1050, 8 },
	{ "Mode 9: 2048x1536", 2048, 1536, 9 }
};

static int s_numVidModes = ( sizeof(vid_modes) / sizeof(vid_modes[0]));
qboolean VID_GetModeInfo ( int *width, int *height, int mode)
{
	vidmode_t *vm;

	if ( mode < -1 )
	{
		return false;
	}

	if ( mode >= s_numVidModes )
	{
		return false;
	}

	if ( mode == -1 )
	{
		*width = vid_width->value;
		*height = vid_height->value;
		return true;
	}

	vm = &vid_modes[mode];

	*width = vm->width;
	*height = vm->height;

	return true;
}

void VID_ModeList_f(void)
{
	int i;

	for ( i = 0; i < s_numVidModes; i++)
	{
		Com_Printf ( "%s\n", vid_modes[i].description );
	}
	Com_Printf("For custom resolutions, set 'gl_mode' to -1 and use 'vid_width' / 'vid_height'\n");
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	if ( vid_restart )
	{
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		VID_Shutdown();

		Com_Printf( "--------- [Loading Renderer] ---------\n" );

		Swap_Init ();

		if ( R_Init( 0, 0 ) == -1 )
		{
			R_Shutdown();
			vid_active = false;
			Com_Error (ERR_FATAL, "Couldn't initialize renderer!");
		}

		Com_Printf( "------------------------------------\n");

		vid_restart = false;
		vid_active = true;
		cls.disable_screen = false;
	}

}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_ref = Cvar_Get ("vid_ref", "glx", CVAR_ARCHIVE);
	vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get( "vid_gamma", "1", CVAR_ARCHIVE );
	vid_width = Cvar_Get ( "vid_width", "640", CVAR_ARCHIVE );
	vid_height = Cvar_Get ( "vid_height", "480", CVAR_ARCHIVE );

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_modelist", VID_ModeList_f);

#if 0
	// 2010-08 Probably very obsolete.
	/* Disable the 3Dfx splash screen */
	putenv("FX_GLIDE_NO_SPLASH=0");
#endif	

	/* Start the graphics mode and load refresh DLL */
	vid_restart = true;
	vid_active = false;
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( vid_active )
	{
		R_Shutdown ();
		vid_active = false;
	}
}

