// Emacs style mode select   -*- C++ -*-
//--- --------------------------------------------------------------------------
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
//      All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------
//
// 4/25/98, 5/2/98 killough: reformatted, beautified

static const char
rcsid[] = "$Id: r_segs.c,v 1.16 1998/05/03 23:02:01 killough Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_things.h"
#include "r_draw.h"
#include "w_wad.h"

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

// True if any of the segs textures might be visible.
static boolean  segtextured;
static boolean  markfloor;      // False if the back side is the same plane.
static boolean  markceiling;
static boolean  markfloor2;
static boolean  maskedtexture;
static int      toptexture;
static int      bottomtexture;
static int      midtexture;

angle_t         rw_normalangle; // angle to line origin
int             rw_angle1;
fixed_t         rw_distance;
lighttable_t    **walllights;

//
// regular wall
//
static int      rw_x;
static int      rw_stopx;
static angle_t  rw_centerangle;
static fixed_t  rw_offset;
static fixed_t  rw_scale;
static fixed_t  rw_scalestep;
static fixed_t  rw_midtexturemid;
static fixed_t  rw_toptexturemid;
static fixed_t  rw_bottomtexturemid;
static int      worldtop;
static int      worldbottom;
static int      worldhigh;
static int      worldlow;
static fixed_t  pixhigh;
static fixed_t  pixlow;
static fixed_t  pixhighstep;
static fixed_t  pixlowstep;
static fixed_t  topfrac;
static fixed_t  topstep;
static fixed_t  bottomfrac;
static fixed_t  bottomstep;

#ifdef TRANWATER
static fixed_t  bottomfrac2;    //sf
static fixed_t  bottomstep2;
#endif

static short    *maskedtexturecol;

//
// R_FixWiggle()
// Dynamic wall/texture rescaler, AKA "WiggleHack II"
//  by Kurt "kb1" Baumgardner ("kb") and Andrey "Entryway" Budko ("e6y")
//
//  [kb] When the rendered view is positioned, such that the viewer is
//   looking almost parallel down a wall, the result of the scale
//   calculation in R_ScaleFromGlobalAngle becomes very large. And, the
//   taller the wall, the larger that value becomes. If these large
//   values were used as-is, subsequent calculations would overflow,
//   causing full-screen HOM, and possible program crashes.
//
//  Therefore, vanilla Doom clamps this scale calculation, preventing it
//   from becoming larger than 0x400000 (64*FRACUNIT). This number was
//   chosen carefully, to allow reasonably-tight angles, with reasonably
//   tall sectors to be rendered, within the limits of the fixed-point
//   math system being used. When the scale gets clamped, Doom cannot
//   properly render the wall, causing an undesirable wall-bending
//   effect that I call "floor wiggle". Not a crash, but still ugly.
//
//  Modern source ports offer higher video resolutions, which worsens
//   the issue. And, Doom is simply not adjusted for the taller walls
//   found in many PWADs.
//
//  This code attempts to correct these issues, by dynamically
//   adjusting the fixed-point math, and the maximum scale clamp,
//   on a wall-by-wall basis. This has 2 effects:
//
//  1. Floor wiggle is greatly reduced and/or eliminated.
//  2. Overflow is no longer possible, even in levels with maximum
//     height sectors (65535 is the theoretical height, though Doom
//     cannot handle sectors > 32767 units in height.
//
//  The code is not perfect across all situations. Some floor wiggle can
//   still be seen, and some texture strips may be slightly misaligned in
//   extreme cases. These effects cannot be corrected further, without
//   increasing the precision of various renderer variables, and, 
//   possibly, creating a noticable performance penalty.
//   

static int			max_rwscale = 64 * FRACUNIT;
static int			heightbits = 12;
static int			heightunit = (1 << 12);
static int			invhgtbits = 4;

void R_ResetHeightMath(void)
{
        max_rwscale = 64 * FRACUNIT;
        heightbits = 12;
        heightunit = (1 << 12);
        invhgtbits = 4;
}

static const struct
{
	int clamp;
	int heightbits;
}	
	scale_values[8] = {
		{2048 * FRACUNIT, 12}, {1024 * FRACUNIT, 12},
		{1024 * FRACUNIT, 11}, { 512 * FRACUNIT, 11},
		{ 512 * FRACUNIT, 10}, { 256 * FRACUNIT, 10},
		{ 256 * FRACUNIT,  9}, { 128 * FRACUNIT,  9}
	
};

void R_FixWiggle (sector_t *sector)
{
	static int	lastheight = 0;
	int		height = (sector->ceilingheight - sector->floorheight) >> FRACBITS;

	// disallow negative heights. using 1 forces cache initialization
	if (height < 1)
		height = 1;

	// early out?
	if (height != lastheight)
	{
		lastheight = height;

		// initialize, or handle moving sector
		if (height != sector->cachedheight)
		{
			sector->cachedheight = height;
			sector->scaleindex = 0;
			height >>= 7;

			// calculate adjustment
			while (height >>= 1)
				sector->scaleindex++;
		}

		// fine-tune renderer for this wall
		max_rwscale = scale_values[sector->scaleindex].clamp;
		heightbits = scale_values[sector->scaleindex].heightbits;
		heightunit = (1 << heightbits);
		invhgtbits = FRACBITS - heightbits;
	}
}

//
// R_RenderMaskedSegRange
//

void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2)
{
  column_t *col;
  int      lightnum;
  int      texnum;
  sector_t tempsec;      // killough 4/13/98

  // Calculate light table.
  // Use different light tables
  //   for horizontal / vertical / diagonal. Diagonal?

  curline = ds->curline;  // OPTIMIZE: get rid of LIGHTSEGSHIFT globally

  // killough 4/11/98: draw translucent 2s normal textures

  colfunc = hires == 2 ? R_DrawColumn2 : R_DrawColumn;
  if (curline->linedef->tranlump >= 0 && general_translucency)
    {
#ifdef FAUXTRAN
      if(faux_translucency) colfunc = hires == 2 ? R_DrawCheckers2 : R_DrawCheckers;
      else
#endif
           colfunc = hires == 2 ? R_DrawTLColumn2 : R_DrawTLColumn;
      tranmap = main_tranmap;
      if (curline->linedef->tranlump > 0)
        tranmap = W_CacheLumpNum(curline->linedef->tranlump-1, PU_STATIC);
    }
  // killough 4/11/98: end translucent 2s normal code

  frontsector = curline->frontsector;
  backsector = curline->backsector;

  texnum = texturetranslation[curline->sidedef->midtexture];

  // killough 4/13/98: get correct lightlevel for 2s normal textures
  lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)
              ->lightlevel >> LIGHTSEGSHIFT)+extralight;

  if(wolflooks)
    lightnum = LIGHTLEVELS - 5;

  // haleyjd 08/11/00: optionally skip this to evenly apply colormap
  //                   to all walls -- never do it with custom colormaps
  if(MapUseFullBright && comp[comp_evenlight])
  {  
    if (curline->v1->y == curline->v2->y)
    {
      if(wolflooks) lightnum++;
      else lightnum--;
    }
    else if (curline->v1->x == curline->v2->x)
    {
      if(wolflooks) lightnum--;
      else lightnum++;
    }
  }

  walllights = ds->colormap[ lightnum >= LIGHTLEVELS ? LIGHTLEVELS-1 :
     lightnum <  0 ? 0 : lightnum ] ;

  maskedtexturecol = ds->maskedtexturecol;

  rw_scalestep = ds->scalestep;
  spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
  mfloorclip = ds->sprbottomclip;
  mceilingclip = ds->sprtopclip;

  // find positioning
  if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
      dc_texturemid = frontsector->floorheight > backsector->floorheight
        ? frontsector->floorheight : backsector->floorheight;
      dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
  else
    {
      dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
        ? frontsector->ceilingheight : backsector->ceilingheight;
      dc_texturemid = dc_texturemid - viewz;
    }

  dc_texturemid += curline->sidedef->rowoffset;

  if (fixedcolormap)
    dc_colormap = fixedcolormap;

  // draw the columns
  for (dc_x = x1 ; dc_x <= x2 ; dc_x++, spryscale += rw_scalestep)
    if (maskedtexturecol[dc_x] != D_MAXSHORT)
      {
        if (!fixedcolormap)      // calculate lighting
          {                                    // killough 11/98:
            unsigned index = spryscale>>(LIGHTSCALESHIFT+hires);

            if (index >=  MAXLIGHTSCALE)
              index = MAXLIGHTSCALE-1;

            if (wolflooks)
              index = MAXLIGHTSCALE-32;

            dc_colormap = walllights[index];
          }


        // killough 3/2/98:
        //
        // This calculation used to overflow and cause crashes in Doom:
        //
        // sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
        //
        // This code fixes it, by using double-precision intermediate
        // arithmetic and by skipping the drawing of 2s normals whose
        // mapping to screen coordinates is totally out of range:

        {
          Long64 t = ((Long64) centeryfrac << FRACBITS) -
                     (Long64) dc_texturemid * spryscale;
          if (t + (Long64) textureheight[texnum] * spryscale < 0 ||
              t > (Long64) MAX_SCREENHEIGHT << FRACBITS*2)
            continue;        // skip if the texture is out of screen's range
          sprtopscreen = (long)(t >> FRACBITS); 
        }

        dc_iscale = 0xffffffffu / (unsigned) spryscale;

        // killough 1/25/98: here's where Medusa came in, because
        // it implicitly assumed that the column was all one patch.
        // Originally, Doom did not construct complete columns for
        // multipatched textures, so there were no header or trailer
        // bytes in the column referred to below, which explains
        // the Medusa effect. The fix is to construct true columns
        // when forming multipatched textures (see r_data.c).

        // draw the texture
        col = (column_t *)((byte *)
                           R_GetColumn(texnum, maskedtexturecol[dc_x]) - 3);
#ifdef NORENDER
        if(!norender8)
#endif
          R_DrawMaskedColumn (col);
        maskedtexturecol[dc_x] = D_MAXSHORT;
      }

  // Except for main_tranmap, mark others purgable at this point
  if (curline->linedef->tranlump > 0 && general_translucency)
    Z_ChangeTag(tranmap, PU_CACHE); // killough 4/11/98
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling textures.
// CALLED: CORE LOOPING ROUTINE.
//

#define HEIGHTBITS 12
#define HEIGHTUNIT (1<<HEIGHTBITS)

#define THRESH (0x80 << HEIGHTBITS)

#define DRAW_MASKED_ANYTHING() \
{ \
  int local_floor = floorclip[rw_x], local_ceiling = ceilingclip[rw_x]; \
  dc_x = rw_x; \
  spryscale = rw_scale; \
  { \
    Long64 t = ((Long64) centeryfrac << FRACBITS) - (Long64) dc_texturemid * spryscale; \
    sprtopscreen = (long)(t >> FRACBITS); \
  } \
  mfloorclip = floorclip; \
  mceilingclip = ceilingclip; \
  floorclip[dc_x] = dc_yh + 1; \
  ceilingclip[dc_x] = dc_yl - 1; \
  R_DrawMaskedColumn2((column_t *)((byte *)dc_source - 3)); \
  floorclip[rw_x] = local_floor; \
  ceilingclip[rw_x] = local_ceiling; \
} \

static void R_RenderSegLoop (void)
{
  fixed_t  texturecolumn = 0;   // shut up compiler warning

  for ( ; rw_x < rw_stopx ; rw_x ++)
    {
      // mark floor / ceiling areas
      int yh = bottomfrac>>heightbits, yl = (topfrac+heightunit-1)>>heightbits;
      // no space above wall?
      int bottom = floorclip[rw_x]-1, top = ceilingclip[rw_x]+1;

#ifdef NORENDER
      if (norender2 >= 0 && norender2 != rw_x)
        {
          rw_scale += rw_scalestep;
          topfrac += topstep;
          bottomfrac += bottomstep;
          continue;
        }

      if (debugcolumn && norenderparm)
        {
          static char msgbuf[300];
          DEBUGMSG(">>>\n");
          sprintf(msgbuf, "rw_x=%d, yh=%d, yl=%d, topfrac=%d.%d, topstep=%d.%d, bottomfrac=%d.%d, bottomstep=%d.%d\n",
            rw_x, yh, yl, topfrac>>heightbits, (topfrac&0xfff) << invhgtbits,
            topstep>>heightbits, (topstep&0xfff) << invhgtbits,
            bottomfrac>>heightbits, (bottomfrac&0xfff) << invhgtbits,
            bottomstep>>heightbits, (bottomstep&0xfff) << invhgtbits);
          DEBUGMSG(msgbuf);
          sprintf(msgbuf, "floorclip[rw_x]=%d, floorplane->top[rw_x]=%d, floorplane->bottom[rw_x]=%d\n",
            floorclip[rw_x], floorplane?floorplane->top[rw_x]:-1, floorplane?floorplane->bottom[rw_x]:-1);
          DEBUGMSG(msgbuf);
          sprintf(msgbuf, "ceilingclip[rw_x]=%d, ceilingplane->top[rw_x]=%d, ceilingplane->bottom[rw_x]=%d\n",
            ceilingclip[rw_x],ceilingplane?ceilingplane->top[rw_x]:-1, ceilingplane?ceilingplane->bottom[rw_x]:-1);
          DEBUGMSG(msgbuf);
        }
#endif

      if(yh < 0)
        yh = 0;
        
      if(yl >= viewheight)
        yl = viewheight - 1;

      if (yl < top)
        yl = top;

      if (markceiling)
        {
          bottom = yl-1;

          if (bottom >= floorclip[rw_x])
            bottom = floorclip[rw_x]-1;

          if (top <= bottom)
            {
              ceilingplane->top[rw_x] = top;
              ceilingplane->bottom[rw_x] = bottom;
            }
        }

      bottom = floorclip[rw_x]-1;
      if (yh > bottom)
        yh = bottom;

      if (markfloor)
        {
          top  = yh < ceilingclip[rw_x] ? ceilingclip[rw_x] : yh;
          if (++top <= bottom)
            {
              floorplane->top[rw_x] = top;
              floorplane->bottom[rw_x] = bottom;
            }
        }

#ifdef TRANWATER
      if (markfloor2)
	{
          int yw = bottomfrac2>>heightbits;
	  
	  if(yw < 0) yw = 0;
	  if(yw >= viewheight) yw = viewheight-1;
	  
          if(yw > floorplane2->top[rw_x])
	    floorplane2->top[rw_x] = yw;
	  if(yw < floorplane2->bottom[rw_x])
	    floorplane2->bottom[rw_x] = yw;
	  
	  if(viewz<floorplane2->height) // below plane
	    {
	      top = floorclip2[rw_x]+1;
	      bottom = yw;
	      if(bottom >= floorclip[rw_x]) bottom = floorclip[rw_x]-1;
	    }
	  else                 // above
	    {
	      top = yw;
	      bottom = floorclip2[rw_x]-1;
	      if(top <= ceilingclip[rw_x]) top = ceilingclip[rw_x]+1;
           }
	  
	  if(top <= bottom)
	    {
	      floorplane2->top[rw_x] = top;
	      floorplane2->bottom[rw_x] = bottom;
	    }
	  
      }
#endif

      // texturecolumn and lighting are independent of wall tiers
      if (segtextured)
        {
          unsigned index;

          // calculate texture offset
          angle_t angle =(rw_centerangle+xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
          texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
          texturecolumn >>= FRACBITS;

          // calculate lighting
          index = rw_scale>>(LIGHTSCALESHIFT+hires);  // killough 11/98

          if (index >=  MAXLIGHTSCALE)
            index = MAXLIGHTSCALE-1;

          if (wolflooks)
            index = MAXLIGHTSCALE-32;
          dc_colormap = walllights[index];
          dc_x = rw_x;
          dc_iscale = 0xffffffffu / (unsigned)rw_scale;
        }

      // draw the wall tiers
      if (midtexture)
        {
          dc_yl = yl;     // single sided line
          dc_yh = yh;
          dc_texturemid = rw_midtexturemid;
          dc_source = R_GetColumn(midtexture, texturecolumn);
          dc_texheight = textureheight[midtexture]>>FRACBITS; // killough          
#ifdef NORENDER
          if(!norender5)
#endif
            {
              if(R_HasMultipatchColumns(midtexture))
                DRAW_MASKED_ANYTHING()
              else
                colfunc ();
            }
          ceilingclip[rw_x] = -1;
          floorclip[rw_x] = viewheight;
        }
      else
        {
          // two sided line
          if (toptexture)
            {
              // top wall
              int mid = pixhigh>>heightbits;
              pixhigh += pixhighstep;

              if (mid >= floorclip[rw_x])
                mid = floorclip[rw_x]-1;

              if (mid >= yl)
                {
                  dc_yl = yl;
                  dc_yh = mid;
                  dc_texturemid = rw_toptexturemid;
                  dc_source = R_GetColumn(toptexture, texturecolumn);
                  dc_texheight = textureheight[toptexture]>>FRACBITS;//killough
#ifdef NORENDER
                  if(!norender6)       
#endif
                    {
                      if(R_HasMultipatchColumns(toptexture))
                        DRAW_MASKED_ANYTHING()
                      else                    
                        colfunc ();
                    }
                  ceilingclip[rw_x] = mid;
                }
              else 
                ceilingclip[rw_x] = yl - 1;
            }
          else 
          {  // no top wall
            if (markceiling)
              ceilingclip[rw_x] = yl - 1;
            if (markfloor2)
              floorclip2[rw_x] = yl - 1;
          }

          if (bottomtexture)          // bottom wall
            {
              int mid = (pixlow+heightunit-1)>>heightbits;
              pixlow += pixlowstep;

              // no space above wall?
              if (mid <= ceilingclip[rw_x])
                mid = ceilingclip[rw_x] + 1;

              if (mid <= yh)
                {
                  dc_yl = mid;
                  dc_yh = yh;
                  dc_texturemid = rw_bottomtexturemid;
                  dc_source = R_GetColumn(bottomtexture,
                                          texturecolumn);
                  dc_texheight = textureheight[bottomtexture]>>FRACBITS; // killough
#ifdef NORENDER
                  if(!norender7)
#endif
                    {
                      if(R_HasMultipatchColumns(bottomtexture))
                        DRAW_MASKED_ANYTHING()
                      else   
                        colfunc ();
                    }
                  floorclip[rw_x] = mid;
                  if (floorplane2 && floorplane2->height<worldlow)
                    {
                        floorclip2[rw_x] = mid;
                    }
                }
              else 
                {
                  floorclip[rw_x] = yh + 1;
                }
            }
          else 
          {        // no bottom wall
            if (markfloor)
            {
              // nasty place
              floorclip[rw_x] = yh + 1;
                if (markfloor2)         
                    floorclip2[rw_x] = yh + 1;
            }
          }

          // save texturecol for backdrawing of masked mid texture
          if (maskedtexture)
            maskedtexturecol[rw_x] = texturecolumn;
        }

      rw_scale += rw_scalestep;
      topfrac += topstep;
      bottomfrac += bottomstep;
#ifdef TRANWATER
      bottomfrac2 += bottomstep2;
#endif

#ifdef NORENDER
      if (debugcolumn && norenderparm)
        {
          static char msgbuf[300];
          DEBUGMSG("<<<\n");
          sprintf(msgbuf, "rw_x=%d, yh=%d, yl=%d, topfrac=%d.%d, topstep=%d.%d, bottomfrac=%d.%d, bottomstep=%d.%d\n",
            rw_x, yh, yl, topfrac>>heightbits, (topfrac&0xfff) << invhgtbits,
            topstep>>heightbits, (topstep&0xfff) << invhgtbits,
            bottomfrac>>heightbits, (bottomfrac&0xfff) << invhgtbits,
            bottomstep>>heightbits, (bottomstep&0xfff) << invhgtbits);
          DEBUGMSG(msgbuf);
          sprintf(msgbuf, "floorclip[rw_x]=%d, floorplane->top[rw_x]=%d, floorplane->bottom[rw_x]=%d\n",
            floorclip[rw_x], floorplane?floorplane->top[rw_x]:-1, floorplane?floorplane->bottom[rw_x]:-1);
          DEBUGMSG(msgbuf);
          sprintf(msgbuf, "ceilingclip[rw_x]=%d, ceilingplane->top[rw_x]=%d, ceilingplane->bottom[rw_x]=%d\n",
            ceilingclip[rw_x],ceilingplane?ceilingplane->top[rw_x]:-1, ceilingplane?ceilingplane->bottom[rw_x]:-1);
          DEBUGMSG(msgbuf);
        }
#endif
    }
}

// killough 5/2/98: move from r_main.c, made static, simplified

static fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
  fixed_t dx = abs(x - viewx);
  fixed_t dy = abs(y - viewy);

  if (dy > dx)
    {
      fixed_t t = dx;
      dx = dy;
      dy = t;
    }
  return dx ? FixedDiv(dx, finesine[(tantoangle[FixedDiv(dy,dx) >> DBITS]
				     + ANG90) >> ANGLETOFINESHIFT]) : 0;
}

fixed_t R_PointToDist2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
  fixed_t dx = abs(x2 - x1);
  fixed_t dy = abs(y2 - y1);
  if (dy > dx)
    {
      fixed_t t = dx;
      dx = dy;
      dy = t;
    }
  return dx ? FixedDiv(dx, finesine[(tantoangle[FixedDiv(dy,dx) >> DBITS]
				     + ANG90) >> ANGLETOFINESHIFT]) : 0;
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// killough 5/2/98: reformatted, cleaned up
// Dynamic WiggleFix Implementation - 2014/09/25

fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
	int			anglea = ANG90 + (visangle - viewangle);
	int			angleb = ANG90 + (visangle - rw_normalangle);
	int			den = FixedMul(rw_distance, finesine[anglea >> ANGLETOFINESHIFT]);
        fixed_t                 num = FixedMul(projection, finesine[angleb >> ANGLETOFINESHIFT]);
	fixed_t 		scale;
	
	if (den > (num >> 16))
	{
		scale = FixedDiv(num, den);

		// [kb] When this evaluates True, the scale is clamped,
		//  and there will be some wiggling.
		if (scale > max_rwscale)
			scale = max_rwscale;
		else if (scale < 256)
			scale = 256;
	}
	else
		scale = max_rwscale;

	return scale;
}


//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(const int start, const int stop)
{
  fixed_t hyp;
  fixed_t sineval;
  angle_t distangle, offsetangle;
  int skipmid;

  if (ds_p == drawsegs+maxdrawsegs)   // killough 1/98 -- fix 2s line HOM
    {
      unsigned newmax = maxdrawsegs ? maxdrawsegs*2 : 128; // killough
      drawsegs = realloc(drawsegs,newmax*sizeof(*drawsegs));
      ds_p = drawsegs+maxdrawsegs;
      maxdrawsegs = newmax;
    }

#ifdef RANGECHECK
  if (start >=viewwidth || start > stop)
    I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif

  sidedef = curline->sidedef;
  linedef = curline->linedef;

  // mark the segment as visible for auto map
  linedef->flags |= ML_MAPPED;

  // calculate rw_distance for scale calculation
  rw_normalangle = curline->angle + ANG90;
  offsetangle = abs(rw_normalangle-rw_angle1);

  if (offsetangle > ANG90)
    offsetangle = ANG90;

  distangle = ANG90 - offsetangle;
  hyp = R_PointToDist (curline->v1->x, curline->v1->y);  
  sineval = finesine[distangle>>ANGLETOFINESHIFT];
  rw_distance = FixedMul (hyp, sineval);

  distangle += ANGLE_1;
  skipmid = distangle > 0 && distangle < (ANGLE_1 << 1);


  ds_p->x1 = rw_x = start;
  ds_p->x2 = stop;
  ds_p->curline = curline;
  ds_p->colormap = scalelight;
  rw_stopx = stop+1;

  if(wigglefix)
    R_FixWiggle(frontsector);

  // killough 1/6/98, 2/1/98: remove limit on openings
  // killough 8/1/98: Replaced code with a static limit 
  // guaranteed to be big enough

  // calculate scale at both ends and step
  ds_p->scale1 = rw_scale =
    R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);

  if (stop > start)
    {
      ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
      ds_p->scalestep = rw_scalestep = (ds_p->scale2-rw_scale) / (stop-start);
    }
  else
    ds_p->scale2 = ds_p->scale1;

  // calculate texture boundaries
  //  and decide if floor / ceiling marks are needed
  worldtop = frontsector->ceilingheight - viewz;
  worldbottom = frontsector->floorheight - viewz;

  midtexture = toptexture = bottomtexture = maskedtexture = 0;
  ds_p->maskedtexturecol = NULL;

  if (!backsector)
    {
      // single sided line
      midtexture = texturetranslation[sidedef->midtexture];

      // a single sided line is terminal, so it must mark ends
      markfloor = markceiling = true;

#ifdef TRANWATER
      markfloor2 = !!floorplane2;
#endif
      
      if (linedef->flags & ML_DONTPEGBOTTOM)
        {         // bottom of texture at bottom
          fixed_t vtop = frontsector->floorheight +
            textureheight[sidedef->midtexture];
          rw_midtexturemid = vtop - viewz;
        }
      else        // top of texture at top
        rw_midtexturemid = worldtop;

      rw_midtexturemid += sidedef->rowoffset;

      {      // killough 3/27/98: reduce offset
        fixed_t h = textureheight[sidedef->midtexture];
        if (h & (h-FRACUNIT))
          rw_midtexturemid %= h;
      }

      ds_p->silhouette = SIL_BOTH;
      ds_p->sprtopclip = screenheightarray;
      ds_p->sprbottomclip = negonearray;
      ds_p->bsilheight = D_MAXINT;
      ds_p->tsilheight = D_MININT;

    }
  else      // two sided line
    {
      ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
      ds_p->silhouette = 0;

      if (frontsector->floorheight > backsector->floorheight)
        {
          ds_p->silhouette = SIL_BOTTOM;
          ds_p->bsilheight = frontsector->floorheight;
        }
      else
        if (backsector->floorheight > viewz)
          {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = D_MAXINT;
          }

      if (frontsector->ceilingheight < backsector->ceilingheight)
        {
          ds_p->silhouette |= SIL_TOP;
          ds_p->tsilheight = frontsector->ceilingheight;
        }
      else
        if (backsector->ceilingheight < viewz)
          {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = D_MININT;
          }

      // killough 1/17/98: this test is required if the fix
      // for the automap bug (r_bsp.c) is used, or else some
      // sprites will be displayed behind closed doors. That
      // fix prevents lines behind closed doors with dropoffs
      // from being displayed on the automap.
      //
      // killough 4/7/98: make doorclosed external variable

      {
        extern int doorclosed;    // killough 1/17/98, 2/8/98, 4/7/98
        if (doorclosed || backsector->ceilingheight<=frontsector->floorheight)
          {
            ds_p->sprbottomclip = negonearray;
            ds_p->bsilheight = D_MAXINT;
            ds_p->silhouette |= SIL_BOTTOM;
          }
        if (doorclosed || backsector->floorheight>=frontsector->ceilingheight)
          {                   // killough 1/17/98, 2/8/98
            ds_p->sprtopclip = screenheightarray;
            ds_p->tsilheight = D_MININT;
            ds_p->silhouette |= SIL_TOP;
          }
      }

      worldhigh = backsector->ceilingheight - viewz;
      worldlow = backsector->floorheight - viewz;

      // hack to allow height changes in outdoor areas
      if ((frontsector->ceilingpic == skyflatnum ||
           frontsector->ceilingpic == sky2flatnum) && 
          (backsector->ceilingpic == skyflatnum ||
           backsector->ceilingpic == sky2flatnum))
        worldtop = worldhigh;

      markfloor = worldlow != worldbottom
        || backsector->floorpic != frontsector->floorpic
        || backsector->lightlevel != frontsector->lightlevel

        // killough 3/7/98: Add checks for (x,y) offsets
        || backsector->floor_xoffs != frontsector->floor_xoffs
        || backsector->floor_yoffs != frontsector->floor_yoffs

        // killough 4/15/98: prevent 2s normals
        // from bleeding through deep water
        || frontsector->heightsec != -1

                // sf: for coloured lighting
        || (backsector->heightsec != frontsector->heightsec && !comp[comp_clighting])

        // killough 4/17/98: draw floors if different light levels
        || backsector->floorlightsec != frontsector->floorlightsec
	 ;

      markceiling = worldhigh != worldtop
        || backsector->ceilingpic != frontsector->ceilingpic
        || backsector->lightlevel != frontsector->lightlevel

        // killough 3/7/98: Add checks for (x,y) offsets
        || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
        || backsector->ceiling_yoffs != frontsector->ceiling_yoffs

        // killough 4/15/98: prevent 2s normals
        // from bleeding through fake ceilings
        || (frontsector->heightsec != -1 &&
            (frontsector->ceilingpic!=skyflatnum &&
             frontsector->ceilingpic!=sky2flatnum))

        // killough 4/17/98: draw ceilings if different light levels
        || backsector->ceilinglightsec != frontsector->ceilinglightsec
                // sf: for coloured lighting
        || (backsector->heightsec != frontsector->heightsec && !comp[comp_clighting])
        ;

      if (backsector->ceilingheight <= frontsector->floorheight
          || backsector->floorheight >= frontsector->ceilingheight)
        markceiling = markfloor = true;   // closed door

        markfloor2 = floorplane2 &&
                (worldhigh!=worldtop || worldlow!=worldbottom);

      if (worldhigh < worldtop)   // top texture
        {
          toptexture = texturetranslation[sidedef->toptexture];
          rw_toptexturemid = linedef->flags & ML_DONTPEGTOP ? worldtop :
            backsector->ceilingheight+textureheight[sidedef->toptexture]-viewz;
        }

      if (worldlow > worldbottom) // bottom texture
        {
          bottomtexture = texturetranslation[sidedef->bottomtexture];
          rw_bottomtexturemid = linedef->flags & ML_DONTPEGBOTTOM ? worldtop :
            worldlow;
        }
      rw_toptexturemid += sidedef->rowoffset;

      // killough 3/27/98: reduce offset
      {
        fixed_t h = textureheight[sidedef->toptexture];
        if (h & (h-FRACUNIT))
          rw_toptexturemid %= h;
      }

      rw_bottomtexturemid += sidedef->rowoffset;

      // killough 3/27/98: reduce offset
      {
        fixed_t h;
        h = textureheight[sidedef->bottomtexture];
        if (h & (h-FRACUNIT))
          rw_bottomtexturemid %= h;
      }

      // allocate space for masked texture tables
      if (sidedef->midtexture && !skipmid)    // masked midtexture
        {
          maskedtexture = true;
          ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
          lastopening += rw_stopx - rw_x;
        }
    }

  // calculate rw_offset (only needed for textured lines)
  segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

  if (segtextured)
    {

      offsetangle = rw_normalangle-rw_angle1;

      if (offsetangle > ANG180)
	offsetangle = -offsetangle;

      if (offsetangle > ANG90)
        offsetangle = ANG90;

      sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
      rw_offset = FixedMul (hyp, sineval); 

      if (rw_normalangle-rw_angle1 < ANG180)
        rw_offset = -rw_offset;
      rw_offset += sidedef->textureoffset;
      rw_offset += curline->offset;

      rw_centerangle = ANG90 + viewangle - rw_normalangle; 

      // calculate light table
      //  use different light tables
      //  for horizontal / vertical / diagonal
      // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
      if (!fixedcolormap)
        {
          int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

          if(wolflooks)
            lightnum = LIGHTLEVELS - 5;

          // haleyjd 08/11/00: optionally skip this to evenly apply colormap
          //                   to all walls -- never do it with custom 
          //                   colormaps
          if(MapUseFullBright && comp[comp_evenlight])
          {  
            if (curline->v1->y == curline->v2->y)
            {
              if(wolflooks)
              {
                if((linedef && !P_IsDoor(linedef)) ||
                 (backsector && (P_IsSecret(backsector) || P_WasSecret(backsector)))) lightnum++;
              }
              else lightnum--;
            }
            else if (curline->v1->x == curline->v2->x)
            {
              if(wolflooks)
              {
                if(!P_IsDoor(linedef) || P_IsSecret(backsector) || P_WasSecret(backsector)) lightnum--;
              }
              else lightnum++;
            } 
          }

          if (lightnum < 0)
            walllights = scalelight[0];
          else if (lightnum >= LIGHTLEVELS)
            walllights = scalelight[LIGHTLEVELS-1];
          else
            walllights = scalelight[lightnum];
        }
    }

  // if a floor / ceiling plane is on the wrong side of the view
  // plane, it is definitely invisible and doesn't need to be marked.

  // killough 3/7/98: add deep water check
  if (frontsector->heightsec == -1)
    {
      if (frontsector->floorheight >= viewz)       // above view plane
        markfloor = false;
      if (frontsector->ceilingheight <= viewz &&
          frontsector->ceilingpic != skyflatnum &&
	  frontsector->ceilingpic != sky2flatnum)   // below view plane
        markceiling = false;
    }

  // calculate incremental stepping values for texture edges
  worldtop >>= invhgtbits;
  worldbottom >>= invhgtbits;

  topstep = -FixedMul (rw_scalestep, worldtop);
  topfrac = (centeryfrac >> invhgtbits) - FixedMul (worldtop, rw_scale);
  
  if(topstep < -THRESH || topstep > THRESH)
    {
      topstep = 0;
      topfrac = 0;
    } 

  bottomstep = -FixedMul (rw_scalestep,worldbottom);
  bottomfrac = (centeryfrac >> invhgtbits) - FixedMul (worldbottom, rw_scale);
  if(bottomstep < -THRESH || bottomstep > THRESH)
    {
      bottomstep = 0;
      bottomfrac = viewheight << heightbits;
    } 

#ifdef TRANWATER
  if (floorplane2)
    {
      int worldplane = (floorplane2->height - viewz)>>invhgtbits;
      bottomstep2 = -FixedMul (rw_scalestep,worldplane);
      bottomfrac2 = (centeryfrac >> invhgtbits) - FixedMul (worldplane, rw_scale);
    }
#endif

  if (backsector)
    {
      worldhigh >>= invhgtbits;
      worldlow >>= invhgtbits;

      if (worldhigh < worldtop)
        {
          pixhigh = (centeryfrac>>invhgtbits) - FixedMul (worldhigh, rw_scale);
          pixhighstep = -FixedMul (rw_scalestep,worldhigh);
        }
      if (worldlow > worldbottom)
        {
          pixlow = (centeryfrac>>invhgtbits) - FixedMul (worldlow, rw_scale);
          pixlowstep = -FixedMul (rw_scalestep,worldlow);
        }
    }

  // render it
  if (markceiling)
    if (ceilingplane)   // killough 4/11/98: add NULL ptr checks
      ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
    else
      markceiling = 0;

  if (markfloor)
    if (floorplane)     // killough 4/11/98: add NULL ptr checks
      if(!comp[comp_vpdup] && markceiling && ceilingplane == floorplane)
        floorplane = R_DupPlane (floorplane, rw_x, rw_stopx - 1);
      else
        floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);
    else
      markfloor = 0;

#ifdef TRANWATER
  if (markfloor2)
    if (floorplane2)
      floorplane2 = R_CheckPlane (floorplane2, rw_x, rw_stopx-1);
    else
      markfloor2 = 0;
#endif

  R_RenderSegLoop();

  // save sprite clipping info
  if ((ds_p->silhouette & SIL_TOP || maskedtexture) && !ds_p->sprtopclip)
    {
      memcpy (lastopening, ceilingclip+start, 2*(rw_stopx-start));
      ds_p->sprtopclip = lastopening - start;
      lastopening += rw_stopx - start;
    }
  if ((ds_p->silhouette & SIL_BOTTOM || maskedtexture) && !ds_p->sprbottomclip)
    {
      memcpy (lastopening, floorclip+start, 2*(rw_stopx-start));
      ds_p->sprbottomclip = lastopening - start;
      lastopening += rw_stopx - start;
    }
  if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
    {
      ds_p->silhouette |= SIL_TOP;
      ds_p->tsilheight = D_MININT;
    }
  if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
    {
      ds_p->silhouette |= SIL_BOTTOM;
      ds_p->bsilheight = D_MAXINT;
    }
  ds_p++;
}

//----------------------------------------------------------------------------
//
// $Log: r_segs.c,v $
// Revision 1.16  1998/05/03  23:02:01  killough
// Move R_PointToDist from r_main.c, fix #includes
//
// Revision 1.15  1998/04/27  01:48:37  killough
// Program beautification
//
// Revision 1.14  1998/04/17  10:40:31  killough
// Fix 213, 261 (floor/ceiling lighting)
//
// Revision 1.13  1998/04/16  06:24:20  killough
// Prevent 2s sectors from bleeding across deep water or fake floors
//
// Revision 1.12  1998/04/14  08:17:16  killough
// Fix light levels on 2s textures
//
// Revision 1.11  1998/04/12  02:01:41  killough
// Add translucent walls, add insurance against SIGSEGV
//
// Revision 1.10  1998/04/07  06:43:05  killough
// Optimize: use external doorclosed variable
//
// Revision 1.9  1998/03/28  18:04:31  killough
// Reduce texture offsets vertically
//
// Revision 1.8  1998/03/16  12:41:09  killough
// Fix underwater / dual ceiling support
//
// Revision 1.7  1998/03/09  07:30:25  killough
// Add primitive underwater support, fix scrolling flats
//
// Revision 1.6  1998/03/02  11:52:58  killough
// Fix texturemapping overflow, add scrolling walls
//
// Revision 1.5  1998/02/09  03:17:13  killough
// Make closed door clipping more consistent
//
// Revision 1.4  1998/02/02  13:27:02  killough
// fix openings bug
//
// Revision 1.3  1998/01/26  19:24:47  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  06:10:42  killough
// Discard old Medusa hack -- fixed in r_data.c now
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
