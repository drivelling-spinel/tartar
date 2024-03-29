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
//      Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------

#ifndef __R_THINGS__
#define __R_THINGS__

#include "r_defs.h"

// Constant arrays used for psprite clipping and initializing clipping.

extern short negonearray[MAX_SCREENWIDTH];         // killough 2/8/98:
extern short screenheightarray[MAX_SCREENWIDTH];   // change to MAX_*

// Vars for R_DrawMaskedColumn

extern short   *mfloorclip;
extern short   *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t pspritescale;
extern fixed_t pspriteiscale;

// haleyjd: particle variables and structures

typedef struct particle_s
{
   fixed_t x, y, z;
   fixed_t velx, vely, velz;
   fixed_t accx, accy, accz;
   byte	ttl;
   byte	trans;
   byte	size;
   byte	fade;
   int  color;
   int  next;
} particle_t;

extern int numParticles;
extern int activeParticles;
extern int inactiveParticles;
extern particle_t *Particles;

void R_DrawMaskedColumn(column_t *column);
void R_SortVisSprites(void);
void R_AddSprites(sector_t *sec,int); // killough 9/18/98
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(char **namelist);
void R_FreeSprites(void);
void R_ClearSprites(void);
void R_DrawMasked(void);

void R_ClipVisSprite(vissprite_t *vis, int xl, int xh);

void R_DrawParticle(vissprite_t *vis);
#ifdef FAUXTRAN
void R_DrawCheckeredParticle(vissprite_t *vis);
#endif
void R_ProjectParticle(particle_t *particle);
void R_ClearParticles(void);
void R_InitParticles(void);
void R_FreeParticles(void);
particle_t *newParticle(void);

#endif

//----------------------------------------------------------------------------
//
// $Log: r_things.h,v $
// Revision 1.4  1998/05/03  22:46:19  killough
// beautification
//
// Revision 1.3  1998/02/09  03:23:27  killough
// Change array decl to use MAX screen width/height
//
// Revision 1.2  1998/01/26  19:27:49  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
