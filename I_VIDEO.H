// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_video.h,v 1.2 2000-08-12 21:29:28 fraggle Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __I_VIDEO__
#define __I_VIDEO__

#include <allegro.h>

#include "doomtype.h"
#include "m_misc.h" // GB 2014: for M_DrawText

#define RESULTING_SCALE ((hires + scale_to_hires) > 2 ? 2 : (hires + scale_to_hires))
#define SCALING_TO_HIRES (scale_to_hires && (hires + scale_to_hires <= 2))
#define CORRECTING_ASPECT (scale_aspect && screen_h != 200 && screen_h != 400)
#define CORRECT_ASPECT(x) ( CORRECTING_ASPECT ? (x * 6 / 5) : (x) )

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode

void I_InitGraphics (void);
void I_ShutdownGraphics(void);

// Takes full 8 bit values.
void I_SetPalette (byte* palette);

// Reloads palette lump before applying
// Have not found better place to put this,
// however using macro as the function is not in fact platform specific
#define I_ResetPalette() { byte * pal = W_CacheDynamicLumpName(DYNA_PLAYPAL, PU_CACHE); I_SetPalette(pal + 768*(st_palette > 0 ? st_palette : 0)); }
// and as sad as it is, modules calling this need the below
extern int st_palette;


void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen (byte* scr);

int I_DoomCode2ScanCode(int);   // killough
int I_ScanCode2DoomCode(int);   // killough

void I_ResetScreen(void);   // killough 10/98

extern int show_fps;   // GB 2014
extern int use_vsync;  // killough 2/8/98: controls whether vsync is called
extern int page_flip; 
extern int hires;  
extern int lcdres;  
extern int scale_to_hires;
extern int disk_icon;  // killough 10/98
extern int in_graphics_mode; // GB 2014: share
extern int scale_aspect;
extern int screen_h;
extern boolean nolfbparm; // checkparm of -noLFB GB 2014
extern int modeswitched;
extern int fps;
extern char mode_string[];
extern int blackband;

#endif

//----------------------------------------------------------------------------
//
// $Log: i_video.h,v $
// Revision 1.2  2000-08-12 21:29:28  fraggle
// change license header
//
// Revision 1.1.1.1  2000/07/29 13:20:41  fraggle
// imported sources
//
// Revision 1.4  1998/05/03  22:40:58  killough
// beautification
//
// Revision 1.3  1998/02/09  03:01:51  killough
// Add vsync for flicker-free blits
//
// Revision 1.2  1998/01/26  19:27:01  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
