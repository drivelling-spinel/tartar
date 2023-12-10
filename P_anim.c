//=========================================================================
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
// p_anim.c
//
// Revision 1.1, haleyjd 11/14/00
//
// Description:  Handles new frame-linked animation processes
//               Currently performs lightning in sky sectors
//               Primarily Hexen code, credit goes to Raven
//
//               11/14/00: updates scroll offsets for hexen-style
//                         skies
//
// This code is covered by no warrantees whatsoever.
//
//=========================================================================

#include "z_zone.h"
#include "doomstat.h"
#include "r_data.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_sky.h"
#include "m_random.h"
#include "p_setup.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_info.h"

boolean LevelHasLightning;
int NextLightningFlash;
int LightningFlash;
int *LightningLightLevels;
int LevelSky;
int LevelTempSky;

static void P_LightningFlash(void);

//
// P_AnimateSurfaces
//
// Called every tic in P_Ticker
//
void P_AnimateSurfaces(void)
{
   // update sky scroll offsets
   //   haleyjd: stored as regular ints in the mapinfo so we need 
   //   to transform these to fixed point values :)

   Sky1ColumnOffset += ((long)info_skydelta)<<8;
   Sky2ColumnOffset += ((long)info_sky2delta)<<8;
   
   if(LevelHasLightning)
   {
      if(!NextLightningFlash || LightningFlash)
      {
         P_LightningFlash();
      }
      else
      { 
         NextLightningFlash--;
      }
   }
}

static void P_LightningFlash(void)
{
   int i;
   sector_t *tempSec;
   int *tempLight;
   boolean foundSec;
   int flashLight;

   if(LightningFlash)
   {
      LightningFlash--;
      if(LightningFlash)
      {
         tempLight = LightningLightLevels;
         tempSec = sectors;
         for(i=0; i<numsectors; i++, tempSec++)
         {
            if(tempSec->ceilingpic == skyflatnum ||
	       tempSec->ceilingpic == sky2flatnum)
            {
               if(*tempLight < tempSec->lightlevel-4)
               {
                  tempSec->lightlevel -= 4;
               }
               tempLight++;
            }
         }
      }
      else
      {
         tempLight = LightningLightLevels;
         tempSec = sectors;
         for(i=0; i<numsectors; i++, tempSec++)
         {
            if(tempSec->ceilingpic == skyflatnum ||
	       tempSec->ceilingpic == sky2flatnum)
            {
               tempSec->lightlevel = *tempLight;
               tempLight++;
            }
         }
         if(LevelSky != -1 && LevelTempSky != -1)
         {
            skytexture = LevelSky;
         }
      }
      return;
   }
   LightningFlash = (P_Random(pr_lightning)&7)+8;
   flashLight = 200+(P_Random(pr_lightning)&31);
   tempSec = sectors;
   tempLight = LightningLightLevels;
   foundSec = false;
   for(i=0; i<numsectors; i++, tempSec++)
   {
      if(tempSec->ceilingpic == skyflatnum ||
	 tempSec->ceilingpic == sky2flatnum)
      {
         *tempLight = tempSec->lightlevel;
         tempSec->lightlevel = flashLight;
         if(tempSec->lightlevel < *tempLight)
         {
            tempSec->lightlevel = *tempLight;
         }
         tempLight++;
         foundSec = true;
      }
   }
   if(foundSec)
   {
      if(LevelSky != -1 && LevelTempSky != -1)
      {
         skytexture = LevelTempSky;
      }
      S_StartSound(NULL, sfx_thundr);
   }
   if(!NextLightningFlash)
   {
      if(P_Random(pr_lightning) < 50)
      {
         NextLightningFlash = (P_Random(pr_lightning)&15)+16;
      }
      else
      {
         if(P_Random(pr_lightning) < 128 && !(leveltime&32))
         {
            NextLightningFlash = ((P_Random(pr_lightning)&7)+2)*35;
         }
         else
         {
            NextLightningFlash = ((P_Random(pr_lightning)&15)+5)*35;
         }
      }
   }
}

void P_ForceLightning(void)
{
   NextLightningFlash = 0;
}

//
// P_InitLightning(void)
//
// Called from P_SetupLevel
//
void P_InitLightning(void)
{
   int i;
   int secCount;
   char *texturename;

   if(!strcmp(info_lightning, "false"))
   {
      LevelHasLightning = false;
      LightningFlash = 0;
      return;
   }

   LevelSky = skytexture;

   if(*info_altskyname)
   {
     texturename = info_altskyname;
     LevelTempSky = R_TextureNumForName(texturename);
   }
   else
     LevelTempSky = -1;

   LevelHasLightning = true;
   LightningFlash = 0;
   secCount = 0;
   for(i=0; i<numsectors; i++)
   {
      if(sectors[i].ceilingpic == skyflatnum ||
	 sectors[i].ceilingpic == sky2flatnum)
      {
         secCount++;
      }
      LightningLightLevels = (int *)Z_Malloc(secCount*sizeof(int), PU_LEVEL, NULL);
      NextLightningFlash = ((P_Random(pr_nextflash)&15)+5)*35;
   }
}

// EOF