// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Sky rendering.
//
//-----------------------------------------------------------------------------

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

// SKY, store the number for name.
#define SKYFLATNAME  "F_SKY1"
// haleyjd: hexen-style skies
#define SKY2FLATNAME "F_SKY2"

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

#define SKY_HEIGHT (128)
#define SKY_HEIGHT_FRAC (SKY_HEIGHT << FRACBITS)
#define SKY_MID_FRAC (100 << FRACBITS)

extern int skytexture;
extern int sky2texture; // haleyjd
extern int skytexturemid;
extern int stretchsky;

// Called whenever the view size changes.
void R_InitSkyMap(void);

// init sky at start of level
void R_StartSky();

#endif

//----------------------------------------------------------------------------
//
// $Log: r_sky.h,v $
// Revision 1.4  1998/05/03  22:56:25  killough
// Add m_fixed.h #include
//
// Revision 1.3  1998/05/01  14:15:29  killough
// beautification
//
// Revision 1.2  1998/01/26  19:27:46  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
