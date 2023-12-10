// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2001 James Haley
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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   Code that ties particle effects to map objects, adapted
//   from zdoom. Thanks to Randy Heit.
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_main.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_random.h"
#include "p_partcl.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"
#include "p_mobj.h"
#include "ex_stuff.h"

// static integers to hold particle color values
static int grey1, grey2, grey3, grey4, red, green, blue, yellow, black,
           red1, green1, blue1, yellow1, purple, purple1, white,
	   rblue1, rblue2, rblue3, rblue4, orange, yorange, dred, grey5,
	   maroon1, maroon2;

static struct particleColorList {
	int *color, r, g, b;
} particleColors[] = {
	{&grey1,	85,  85,  85 },
	{&grey2,	171, 171, 171},
	{&grey3,	50,  50,  50 },
	{&grey4,	210, 210, 210},
	{&grey5,	128, 128, 128},
	{&red,		255, 0,   0  },  
	{&green,	0,   200, 0  },  
	{&blue,		0,   0,   255},
	{&yellow,	255, 255, 0  },  
	{&black,	0,   0,   0  },  
	{&red1,		255, 127, 127},
	{&green1,	127, 255, 127},
	{&blue1,	127, 127, 255},
	{&yellow1,	255, 255, 180},
	{&purple,	120, 0,   160},
	{&purple1,	200, 30,  255},
	{&white, 	255, 255, 255},
	{&rblue1,	81,  81,  255},
	{&rblue2,	0,   0,   227},
	{&rblue3,	0,   0,   130},
	{&rblue4,	0,   0,   80 },
	{&orange,	255, 120, 0  },
	{&yorange,	255, 170, 0  },
	{&dred,		80,  0,   0  },
	{&maroon1,	154, 49,  49 },
	{&maroon2,	125, 24,  24 },
	{NULL}
};

void P_InitParticleEffects(void)
{
   byte *palette;
   struct particleColorList *pc = particleColors;

   // LP: for those WAD-s that don't provide a PLAYPAL lump
   if(Ex_DynamicNumForName(DYNA_PLAYPAL) < 0) return;
   
   palette = Ex_CacheDynamicLumpName(DYNA_PLAYPAL, PU_STATIC);

   // match particle colors to best fit and write back to
   // static variables
   while(pc->color)
   {
      *(pc->color) = V_FindBestColor(palette, pc->r, pc->g, pc->b);
      pc++;
   }

   Z_ChangeTag(palette, PU_CACHE);
}

void P_ParticleThinker(void)
{
   int i;
   particle_t *particle, *prev;
   
   i = activeParticles;
   prev = NULL;
   while(i != -1) 
   {
      //byte oldtrans;
      
      particle = Particles + i;
      i = particle->next;

      // haleyjd: unfortunately due to BOOM limitations on
      // translucency, particles can't currently fade, so they'll
      // always be translucent -- I may rework this later
      
      //oldtrans = particle->trans;
      //particle->trans -= particle->fade;
      if(/*oldtrans < particle->trans ||*/ --particle->ttl == 0)
      {
	 memset(particle, 0, sizeof(particle_t));
	 if(prev)
	    prev->next = i;
	 else
	    activeParticles = i;
	 particle->next = inactiveParticles;
	 inactiveParticles = particle - Particles;
	 continue;
      }
      particle->x += particle->velx;
      particle->y += particle->vely;
      particle->z += particle->velz;
      particle->velx += particle->accx;
      particle->vely += particle->accy;
      particle->velz += particle->accz;
      prev = particle;
   }
}

void P_RunEffects(void)
{
   int snum = 0;
   thinker_t *currentthinker = &thinkercap;

   if(camera)
   {
      subsector_t *ss = R_PointInSubsector(camera->x, camera->y);
      snum = (ss->sector - sectors) * numsectors;
   }
   else
   {
      subsector_t *ss = players[consoleplayer].mo->subsector;
      snum = (ss->sector - sectors) * numsectors;
   }

   while((currentthinker = currentthinker->next) != &thinkercap)
   {
      if(currentthinker->function == P_MobjThinker)
      {
	 int rnum;
	 mobj_t *mobj = (mobj_t *)currentthinker;
	 rnum = snum + (mobj->subsector->sector - sectors);
	 if(mobj->effects)
	 {
	    // run only if possibly visible
            if(rejectmatrix)
              if(!(rejectmatrix[rnum>>3] & (1<<(rnum&7))))
                 P_RunEffect(mobj, mobj->effects);
	 }
      }
   }
}

#define FADEFROMTTL(a) (255/(a))

particle_t *JitterParticle(int ttl)
{
   particle_t *particle = newParticle();
   
   if (particle) 
   {
      int i;
      fixed_t *val = &particle->velx;
            
      // Set initial velocities
      for(i = 3; i; i--, val++)
	 *val = (FRACUNIT/4096) * (M_Random() - 128);
      
      // Set initial accelerations
      for(i = 3; i; i--, val++)
	 *val = (FRACUNIT/16384) * (M_Random() - 128);
      
      particle->trans = 255;	// fully opaque
      particle->ttl = ttl;
      particle->fade = FADEFROMTTL(ttl);
   }
   return particle;
}

static void MakeFountain(mobj_t *actor, int color1, int color2)
{
   particle_t *particle;
   
   if(!(leveltime & 1))
      return;
   
   particle = JitterParticle(51);
   
   if (particle)
   {
      angle_t an  = M_Random()<<(24-ANGLETOFINESHIFT);
      fixed_t out = FixedMul(actor->radius, M_Random()<<8);
      
      particle->x = actor->x + FixedMul(out, finecosine[an]);
      particle->y = actor->y + FixedMul(out, finesine[an]);
      particle->z = actor->z + actor->height + FRACUNIT;
      
      if(out < actor->radius/8)
	 particle->velz += FRACUNIT*10/3;
      else
	 particle->velz += FRACUNIT*3;

      particle->accz -= FRACUNIT/11;
      if(M_Random() < 30)
      {
	 particle->size = 4;
	 particle->color = color2;
      } 
      else 
      {
	 particle->size = 6;
	 particle->color = color1;
      }
   }
}

void P_RunEffect(mobj_t *actor, int effects)
{
   angle_t moveangle = R_PointToAngle2(0,0,actor->momx,actor->momy);
   particle_t *particle;
   
   if((effects & FX_ROCKET) && drawrockettrails)
   {
      int i, speed;

      // Rocket trail
      fixed_t backx = 
	 actor->x - FixedMul(finecosine[(moveangle)>>ANGLETOFINESHIFT], 
	                     actor->radius*2);
      fixed_t backy = 
	 actor->y - FixedMul(finesine[(moveangle)>>ANGLETOFINESHIFT], 
	                     actor->radius*2);
      fixed_t backz = 
	 actor->z - (actor->height>>3) * (actor->momz>>16) + 
	 (2*actor->height)/3;
      
      angle_t an = (moveangle + ANG90) >> ANGLETOFINESHIFT;

      particle = JitterParticle(3 + (M_Random() & 31));
      if(particle)
      {
	 fixed_t pathdist = M_Random()<<8;
	 particle->x = backx - FixedMul(actor->momx, pathdist);
	 particle->y = backy - FixedMul(actor->momy, pathdist);
	 particle->z = backz - FixedMul(actor->momz, pathdist);
	 speed = (M_Random () - 128) * (FRACUNIT/200);
	 particle->velx += FixedMul(speed, finecosine[an]);
	 particle->vely += FixedMul(speed, finesine[an]);
	 particle->velz -= FRACUNIT/36;
	 particle->accz -= FRACUNIT/20;
	 particle->color = yellow;
	 particle->size = 2;
      }
      
      for(i = 6; i; i--)
      {
	 particle_t *particle = JitterParticle (3 + (M_Random() & 31));
	 if (particle)
	 {
	    fixed_t pathdist = M_Random()<<8;
	    particle->x = backx - FixedMul(actor->momx, pathdist);
	    particle->y = backy - FixedMul(actor->momy, pathdist);
	    particle->z = backz - FixedMul(actor->momz, pathdist) + 
	                  (M_Random() << 10);
	    speed = (M_Random () - 128) * (FRACUNIT/200);
	    particle->velx += FixedMul(speed, finecosine[an]);
	    particle->vely += FixedMul(speed, finesine[an]);
	    particle->velz += FRACUNIT/80;
	    particle->accz += FRACUNIT/40;
	    if(M_Random() & 7)
	       particle->color = grey2;
	    else
	       particle->color = grey1;

	    particle->size = 3;
	 } 
	 else
	 {
	    break;
	 }
      }
   }
   
   if((effects & FX_GRENADE) && drawgrenadetrails)
   {
      // Grenade trail
      
      P_DrawSplash2(6,
	 actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
	 actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
	 actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3,
	 moveangle + ANG180, 2, 2);
   }

   if(effects & FX_FOUNTAINMASK)
   {
      // Particle fountain
      
      static const int *fountainColors[16] = 
      { &black,	  &black,
        &red,	  &red1,
        &green,	  &green1,
        &blue,	  &blue1,
        &yellow,  &yellow1,
        &purple,  &purple1,
        &black,	  &grey3,
        &grey4,	  &white
      };
      int color = (effects & FX_FOUNTAINMASK) >> 15;
      MakeFountain(actor, *fountainColors[color], 
	           *fountainColors[color+1]);
   }
}

void P_DrawSplash(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int kind)
{
   int color1, color2;
   
   switch(kind) 
   {
   case 1: // Spark
      color1 = orange;
      color2 = yorange;
      break;
   default:
      return;
   }
   
   for(; count; count--)
   {
      angle_t an;
      particle_t *p = JitterParticle(10);
            
      if(!p)
	 break;
      
      p->size = 2;
      p->color = M_Random() & 0x80 ? color1 : color2;
      p->velz -= M_Random() * 512;
      p->accz -= FRACUNIT/8;
      p->accx += (M_Random() - 128) * 8;
      p->accy += (M_Random() - 128) * 8;
      p->z = z - M_Random() * 1024;
      an = (angle + (M_Random() << 21)) >> ANGLETOFINESHIFT;
      p->x = x + (M_Random() & 15)*finecosine[an];
      p->y = y + (M_Random() & 15)*finesine[an];
   }
}

void P_DrawSplash2(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown, int kind)
{
   int color1, color2, zvel, zspread, zadd;
   
   switch(kind)
   {
   case 0:		// Blood
      color1 = red;
      color2 = dred;
      break;
   case 1:		// Gunshot
      color1 = grey3;
      color2 = grey5;
      break;
   case 2:		// Smoke
      color1 = grey3;
      color2 = grey1;
      break;
   case 0x10:           // Blue blood
      color1 = blue;
      color2 = rblue1;
      break;
   case 0x20:           // Green blood
      color1 = green;
      color2 = green1;
      break;
   case 0x30:           // Yellow blood
      color1 = yellow;
      color2 = yellow1;
      break;
   default:
      return;
   }
   
   zvel = -128;
   zspread = updown ? -6000 : 6000;
   zadd = (updown == 2) ? -128 : 0;
   
   for(; count; count--)
   {
      particle_t *p = newParticle();
      angle_t an;
      
      if(!p)
	 break;
      
      p->ttl = 12;
      p->fade = FADEFROMTTL(12);
      p->trans = 255;
      p->size = 4;
      p->color = M_Random() & 0x80 ? color1 : color2;
      p->velz = M_Random() * zvel;
      p->accz = -FRACUNIT/22;
      if(kind)
      {
	 an = (angle + ((M_Random() - 128) << 23)) >> ANGLETOFINESHIFT;
	 p->velx = (M_Random() * finecosine[an]) >> 11;
	 p->vely = (M_Random() * finesine[an]) >> 11;
	 p->accx = p->velx >> 4;
	 p->accy = p->vely >> 4;
      }
      p->z = z + (M_Random() + zadd) * zspread;
      an = (angle + ((M_Random() - 128) << 22)) >> ANGLETOFINESHIFT;
      p->x = x + (M_Random() & 31)*finecosine[an];
      p->y = y + (M_Random() & 31)*finesine[an];
   }
}

void P_DisconnectEffect(mobj_t *actor)
{
   int i;
   
   for(i = 64; i; i--)
   {
      particle_t *p = JitterParticle (TICRATE*2);
      
      if(!p)
	 break;
      
      p->x = actor->x + 
	     ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
      p->y = actor->y + 
	     ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
      p->z = actor->z + (M_Random()<<8) * (actor->height>>FRACBITS);
      p->accz -= FRACUNIT/4096;
      p->color = M_Random() < 128 ? maroon1 : maroon2;
      p->size = 4;
   }
}

// EOF
