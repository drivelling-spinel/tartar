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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Refresh of things represented by sprites --
//  i.e. map objects and particles.
//
//  Particle code largely from zdoom, thanks to Randy Heit.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_things.c,v 1.22 1998/05/03 22:46:41 killough Exp $";

#include "c_io.h"
#include "doomstat.h"
#include "w_wad.h"
#include "g_game.h"
#include "d_main.h"
#include "p_skin.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "r_draw.h"
#include "r_things.h"
#include "m_argv.h"
#include "p_info.h"

#define MINZ        (FRACUNIT*4)
#define BASEYCENTER 100

extern int columnofs[];

typedef struct {
  int x1;
  int x2;
  int column;
  int topclip;
  int bottomclip;
} maskdraw_t;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
extern int updownangle;

extern int global_cmap_index; // haleyjd: NGCS

short pscreenheightarray[MAX_SCREENWIDTH]; // for psprites

fixed_t pspritescale;
fixed_t pspriteiscale;

static lighttable_t **spritelights;        // killough 1/25/98 made static

// constant arrays
//  used for psprite clipping and initializing clipping

short negonearray[MAX_SCREENWIDTH];        // killough 2/8/98:
short screenheightarray[MAX_SCREENWIDTH];  // change to MAX_*
int lefthanded=0;

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches

spritedef_t *sprites;
int numsprites;

#define MAX_SPRITE_FRAMES 29          /* Macroized -- killough 1/25/98 */

static spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
static int maxframe;

// haleyjd: global particle system state

int        numParticles;
int        activeParticles;
int        inactiveParticles;
particle_t *Particles;

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//

static void R_InstallSpriteLump(int lump, unsigned frame,
                                unsigned rotation, boolean flipped)
{
  if (frame >= MAX_SPRITE_FRAMES || rotation > 8)
    I_Error("R_InstallSpriteLump: Bad frame characters in lump %i", lump);

  if ((int) frame > maxframe)
    maxframe = frame;

  if (rotation == 0)
    {    // the lump should be used for all rotations
      int r;
      for (r=0 ; r<8 ; r++)
        if (sprtemp[frame].lump[r]==-1)
          {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (byte) flipped;
            sprtemp[frame].rotate = false; //jff 4/24/98 if any subbed, rotless
          }
      return;
    }

  // the lump is only used for one rotation

  if (sprtemp[frame].lump[--rotation] == -1)
    {
      sprtemp[frame].lump[rotation] = lump - firstspritelump;
      sprtemp[frame].flip[rotation] = (byte) flipped;
      sprtemp[frame].rotate = true; //jff 4/24/98 only change if rot used
    }
}

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
// (4 chars exactly) to be used.
//
// Builds the sprite rotation matrixes to account
// for horizontally flipped sprites.
//
// Will report an error if the lumps are inconsistent.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
//
// A sprite that is flippable will have an additional
//  letter/number appended.
//
// The rotation character can be 0 to signify no rotations.
//
// 1/25/98, 1/31/98 killough : Rewritten for performance
//
// Empirically verified to have excellent hash
// properties across standard Doom sprites:

#define R_SpriteNameHash(s) ((unsigned)((s)[0]-((s)[1]*3-(s)[3]*2-(s)[2])*2))

void R_InitSpriteDefs(char **namelist)
{
  size_t numentries = lastspritelump-firstspritelump+1;
  struct { int index, next; } *hash;
  int i;

  if (!numentries || !*namelist)
    return;

  // count the number of sprite names
  for (i=0; namelist[i]; i++) ;

  numsprites = i;

  sprites = Z_Calloc(numsprites, sizeof(*sprites), PU_STATIC, NULL);

  // Create hash table based on just the first four letters of each sprite
  // killough 1/31/98

  hash = malloc(sizeof(*hash)*numentries); // allocate hash table

  for (i=0; i<numentries; i++)             // initialize hash table as empty
    hash[i].index = -1;

  for (i=0; i<numentries; i++)             // Prepend each sprite to hash chain
    {                                      // prepend so that later ones win
      int j = R_SpriteNameHash(lumpinfo[i+firstspritelump]->name) % numentries;
      hash[i].next = hash[j].index;
      hash[j].index = i;
    }

  // scan all the lump names for each of the names,
  //  noting the highest frame letter.

  for (i=0 ; i<numsprites ; i++)
    {
      const char *spritename = namelist[i];
      int j = hash[R_SpriteNameHash(spritename) % numentries].index;

      if (j >= 0)
        {
          memset(sprtemp, -1, sizeof(sprtemp));
          maxframe = -1;
          do
            {
              register lumpinfo_t *lump = lumpinfo[j + firstspritelump];

              // Fast portable comparison -- killough
              // (using int pointer cast is nonportable):

              if (!((lump->name[0] ^ spritename[0]) |
                    (lump->name[1] ^ spritename[1]) |
                    (lump->name[2] ^ spritename[2]) |
                    (lump->name[3] ^ spritename[3])))
                {
                  R_InstallSpriteLump(j+firstspritelump,
                                      lump->name[4] - 'A',
                                      lump->name[5] - '0',
                                      false);
                  if (lump->name[6])
                    R_InstallSpriteLump(j+firstspritelump,
                                        lump->name[6] - 'A',
                                        lump->name[7] - '0',
                                        true);
                }
            }
          while ((j = hash[j].next) >= 0);

          // check the frames that were found for completeness
          if ((sprites[i].numframes = ++maxframe))  // killough 1/31/98
            {
              int frame;
              for (frame = 0; frame < maxframe; frame++)
                switch ((int) sprtemp[frame].rotate)
                  {
                  case -1:
                    // no rotations were found for that frame at all
                    usermsg ("R_InitSprites: No patches found "
                             "for %.8s frame %c", namelist[i], frame+'A');
                    break;

                  case 0:
                    // only the first rotation is needed
                    break;

                  case 1:
                    // must have all 8 frames
                    {
                      int rotation;
                      for (rotation=0 ; rotation<8 ; rotation++)
                        if (sprtemp[frame].lump[rotation] == -1)
                          I_Error ("R_InitSprites: Sprite %.8s frame %c "
                                   "is missing rotations",
                                   namelist[i], frame+'A');
                      break;
                    }
                  }
              // allocate space for the frames present and copy sprtemp to it
              sprites[i].spriteframes =
                Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
              memcpy (sprites[i].spriteframes, sprtemp,
                      maxframe*sizeof(spriteframe_t));
            }
        }
    }
  free(hash);             // free hash table
}

void R_FreeSprites()
{
  int i = 0;
  for(i = 0 ; i < numsprites ; i += 1)
    if(sprites[i].numframes) 
      free(sprites[i].spriteframes);
  free(sprites);
  sprites = 0;
}

//
// GAME FUNCTIONS
//

static vissprite_t *vissprites, **vissprite_ptrs;  // killough
static size_t num_vissprite, num_vissprite_alloc, num_vissprite_ptrs;

//
// R_InitSprites
// Called at program start.
//

void R_InitSprites(char **namelist)
{
  int i;
  for (i=0; i<MAX_SCREENWIDTH; i++)    // killough 2/8/98
    negonearray[i] = -1;
  R_InitSpriteDefs(namelist);
}

//
// R_ClearSprites
// Called at frame start.
//

void R_ClearSprites (void)
{
  num_vissprite = 0;            // killough
}

//
// R_NewVisSprite
//

vissprite_t *R_NewVisSprite(void)
{
  if (num_vissprite >= num_vissprite_alloc)             // killough
    {
      num_vissprite_alloc = num_vissprite_alloc ? num_vissprite_alloc*2 : 128;
      vissprites = realloc(vissprites,num_vissprite_alloc*sizeof(*vissprites));
    }
 return vissprites + num_vissprite++;
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//

short   *mfloorclip;
short   *mceilingclip;
fixed_t spryscale;
fixed_t sprtopscreen;

void R_DrawMaskedColumn(column_t *column)
{
  int topscreen, bottomscreen, tall = 0;
  fixed_t basetexturemid = dc_texturemid;
  
  dc_texheight = 0; // killough 11/98

  while (column->topdelta != 0xff)
    {
      if(column->topdelta <= tall)
        tall += column->topdelta;
      else
        tall = column->topdelta;
      // calculate unclipped screen coordinates for post
      topscreen = sprtopscreen + spryscale*tall;
      bottomscreen = topscreen + spryscale*column->length;

      // Here's where "sparkles" come in -- killough:
      dc_yl = (topscreen + FRACUNIT - 1)>>FRACBITS;
      dc_yh = (bottomscreen-1)>>FRACBITS;

      if (dc_yh >= mfloorclip[dc_x])
        dc_yh = mfloorclip[dc_x]-1;

      if (dc_yl <= mceilingclip[dc_x])
        dc_yl = mceilingclip[dc_x]+1;

      // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
      if (dc_yl <= dc_yh && dc_yh < viewheight )
        {
          dc_source = (byte *) column + 3;
          dc_texturemid = basetexturemid - (tall<<FRACBITS);
          // Drawn by either R_DrawColumn
          //  or (SHADOW) R_DrawFuzzColumn.
          colfunc();
        }
        
      column = (column_t *)((byte *) column + column->length + 4);
    }
  dc_texturemid = basetexturemid;
}

void R_DrawMaskedColumn2(column_t *column)
{
  column_t *wrap = column;
  int topscreen, bottomscreen, tall = 0;
  fixed_t basetexturemid = dc_texturemid;
  fixed_t th = (mceilingclip[dc_x] + 1) << FRACBITS;

  if(column->topdelta == 0xff)
    return;

    
  while(sprtopscreen + spryscale * dc_texheight < th)
    {
      sprtopscreen += spryscale * dc_texheight;
    }
    
  while(sprtopscreen > th)
    {
      sprtopscreen -= spryscale * dc_texheight;
    }

  dc_texheight = 0; // killough 11/98

  do
    {
      int len = column->length;
      
      if(column->topdelta <= tall)
        tall += column->topdelta;
      else
        tall = column->topdelta;
      // calculate unclipped screen coordinates for post
      
      if(len > 0)
        {
          topscreen = sprtopscreen + spryscale*tall;
          bottomscreen = topscreen + spryscale*len;

          // Here's where "sparkles" come in -- killough:
          dc_yl = (topscreen + FRACUNIT - 1)>>FRACBITS;
          dc_yh = (bottomscreen-1)>>FRACBITS;

          if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x]-1;

          if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x]+1;
          // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
          if (dc_yl <= dc_yh && dc_yh < viewheight )
            {
              dc_source = (byte *) column + 3;
              dc_texturemid = FixedMul(centeryfrac - topscreen, dc_iscale);
              // Drawn by either R_DrawColumn
              //  or (SHADOW) R_DrawFuzzColumn.
              colfunc();
            }
        }
      column = (column_t *)((byte *) column + len + 4);
      if(column->topdelta == 0xff) 
        {
          column = wrap;
          sprtopscreen = bottomscreen;
          tall = 0;
        }
    } while(dc_yh < mfloorclip[dc_x] - 1);
    dc_texturemid = basetexturemid;
}
//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//

void R_DrawVisSprite(vissprite_t *vis, int x1, int x2)
{
  column_t *column;
  int      texturecolumn;
  fixed_t  frac;
  patch_t  *patch;
  
  if(vis->patch == -1)
  {
     // this vissprite belongs to a particle
#ifdef FAUXTRAN
     if(general_translucency && faux_translucency) R_DrawCheckeredParticle(vis);
     else
#endif
       R_DrawParticle(vis);
     return;
  }
  
  patch = W_CacheLumpNum (vis->patch+firstspritelump, PU_CACHE);

  dc_colormap = vis->colormap;

  // killough 4/11/98: rearrange and handle translucent sprites
  // mixed with translucent/non-translucent 2s normals

  // sf: shadow draw now done by mobj flags, not a null colormap

  if(vis->mobjflags & MF_SHADOW)   // shadow draw
  {
     colfunc = R_DrawFuzzColumn;    // killough 3/14/98
  }
  else if(vis->colour)
  {
     colfunc = R_DrawTranslatedColumn;
     dc_translation = translationtables + vis->colour*256 - 256;
#ifdef FAUXTRAN
     if(vis->mobjflags & MF_TRANSLUCENT && general_translucency && faux_translucency)
       colfunc = R_DrawTranslatedCheckers;
#endif
  }
  else if(vis->mobjflags & MF_TRANSLUCENT && general_translucency) // phares
  {
#ifdef FAUXTRAN
     if(faux_translucency) colfunc = hires == 2 ? R_DrawCheckers2 : R_DrawCheckers;
     else
#endif
       colfunc = hires == 2 ? R_DrawTLColumn2 : R_DrawTLColumn;
     tranmap = main_tranmap;       // killough 4/11/98
  }
  else
     colfunc = hires == 2 ? R_DrawColumn2 : R_DrawColumn;         // killough 3/14/98, 4/11/98

  dc_iscale = abs(vis->xiscale);
  dc_texturemid = vis->texturemid;
  frac = vis->startfrac;
  spryscale = vis->scale;
  sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);

  for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
    {
      texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
      if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
        I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif

      column = (column_t *)((byte *) patch +
                            LONG(patch->columnofs[texturecolumn]));
#ifdef NORENDER
      if(!norender3)
#endif
        R_DrawMaskedColumn (column);
    }
  colfunc = hires == 2 ? R_DrawColumn2 : R_DrawColumn; // killough 3/14/98
}

//
// R_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//

void R_ProjectSprite (mobj_t* thing)
{
  fixed_t   gzt;               // killough 3/27/98
  fixed_t   tx;
  fixed_t   xscale;
  int       x1;
  int       x2;
  spritedef_t   *sprdef;
  spriteframe_t *sprframe;
  int       lump;
  boolean   flip;
  vissprite_t *vis;
  fixed_t   iscale;
  int heightsec;      // killough 3/27/98

  // transform the origin point
  fixed_t tr_x = thing->x - viewx;
  fixed_t tr_y = thing->y - viewy;

  fixed_t gxt = FixedMul(tr_x,viewcos);
  fixed_t gyt = -FixedMul(tr_y,viewsin);

  fixed_t tz = gxt-gyt;

  // thing is behind view plane?
  if (tz < MINZ)
    return;

  if(thing->flags2 & MF2_DONTDRAW) // haleyjd 04/18/99: MF2_DONTDRAW
    return; // don't generate vissprite

  xscale = FixedDiv(projection, tz);

  gxt = -FixedMul(tr_x,viewsin);
  gyt = FixedMul(tr_y,viewcos);
  tx = -(gyt+gxt);

  // too far off the side?
  if (abs(tx)>(tz<<2))
    return;

    // decide which patch to use for sprite relative to player
  if ((unsigned) thing->sprite >= numsprites)
  {
    C_Printf ("R_ProjectSprite: invalid sprite number %i\n", thing->sprite);
    C_SetConsole();
    return;
  }

  sprdef = &sprites[thing->sprite];

  if ((thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
    I_Error ("R_ProjectSprite:invalid frame %i for sprite %s",
             thing->frame & FF_FRAMEMASK, spritelist[thing->sprite]);

  sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
    {
      // choose a different rotation based on player view
      angle_t ang = R_PointToAngle(thing->x, thing->y);
      unsigned rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
      lump = sprframe->lump[rot];
      flip = (boolean) sprframe->flip[rot];
    }
  else
    {
      // use single rotation for all views
      lump = sprframe->lump[0];
      flip = (boolean) sprframe->flip[0];
    }

  // calculate edges of the shape
  tx -= spriteoffset[lump];
  x1 = (centerxfrac + FixedMul(tx,xscale)) >>FRACBITS;

    // off the right side?
  if (x1 > viewwidth)
    return;

  tx +=  spritewidth[lump];
  x2 = ((centerxfrac + FixedMul(tx,xscale)) >> FRACBITS) - 1;

    // off the left side
  if (x2 < 0)
    return;

  gzt = thing->z + spritetopoffset[lump];

  // killough 4/9/98: clip things which are out of view due to height
  // sf : fix for look up/down
//        centeryfrac=(viewheight<<(FRACBITS-1));
  if (thing->z > viewz + FixedDiv((viewheight<<(FRACBITS)), xscale) ||
      gzt      < viewz - FixedDiv((viewheight<<(FRACBITS))-viewheight, xscale))
    return;

  // killough 3/27/98: exclude things totally separated
  // from the viewer, by either water or fake ceilings
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings

  heightsec = thing->subsector->sector->heightsec;

  if (heightsec != -1)   // only clip things which are in special sectors
    {
      // haleyjd: and yet ANOTHER assumption!
      int phs = viewcamera ? viewcamera->heightsec :
                  viewplayer->mo->subsector->sector->heightsec;
      if (phs != -1 && viewz < sectors[phs].floorheight ?
          thing->z >= sectors[heightsec].floorheight :
          gzt < sectors[heightsec].floorheight)
        return;
      if (phs != -1 && viewz > sectors[phs].ceilingheight ?
          gzt < sectors[heightsec].ceilingheight &&
          viewz >= sectors[heightsec].ceilingheight :
          thing->z >= sectors[heightsec].ceilingheight)
        return;
    }

  // store information in a vissprite
  vis = R_NewVisSprite ();

  // killough 3/27/98: save sector for special clipping later
  vis->heightsec = heightsec;

  vis->mobjflags = thing->flags;
  vis->colour = thing->colour;
  if (thing->intflags & MIF_BLOODBLUE)
    {
      if(info_wolfcolor & woco_any) vis->colour = 19;
      else vis->colour = 16;
    }
  else if (thing->intflags & MIF_BLOODGREEN) vis->colour = 17;
  else if (thing->intflags & MIF_BLOODYELLOW) vis->colour = 18;

  vis->scale = xscale;
  vis->gx = thing->x;
  vis->gy = thing->y;
  vis->gz = thing->z;
  vis->gzt = gzt;                          // killough 3/27/98
  vis->texturemid = gzt - viewz;
  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  iscale = FixedDiv(FRACUNIT, xscale);

  if (flip)
    {
      vis->startfrac = spritewidth[lump]-1;
      vis->xiscale = -iscale;
    }
  else
    {
      vis->startfrac = 0;
      vis->xiscale = iscale;
    }

  if (vis->x1 > x1)
    vis->startfrac += vis->xiscale*(vis->x1-x1);
  vis->patch = lump;

  // get light level
  if (thing->flags & MF_SHADOW)     // sf
    vis->colormap = colormaps[global_cmap_index]; // haleyjd: NGCS -- was 0
  else if (fixedcolormap)
    vis->colormap = fixedcolormap;      // fixed map
  else if (MapUseFullBright && (thing->frame & FF_FULLBRIGHT)) // haleyjd
    vis->colormap = fullcolormap;       // full bright  // killough 3/20/98
  else
    {      // diminished light
      int index = xscale>>(LIGHTSCALESHIFT+hires);  // killough 11/98
      if (index >= MAXLIGHTSCALE)
        index = MAXLIGHTSCALE-1;
      vis->colormap = spritelights[index];
    }

}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//

// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting
void R_AddSprites(sector_t* sec, int lightlevel)
{
  mobj_t *thing;
  int    lightnum;

  // BSP is traversed by subsector.
  // A sector might have been split into several
  //  subsectors during BSP building.
  // Thus we check whether its already added.

  if (sec->validcount == validcount)
    return;

  // Well, now it will be done.
  sec->validcount = validcount;

  lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS-1];
  else
    spritelights = scalelight[lightnum];

  // Handle all things in sector.

  for (thing = sec->thinglist; thing; thing = thing->snext)
    R_ProjectSprite(thing);
}

//
// R_DrawPSprite
//

void R_DrawPSprite (pspdef_t *psp)
{
  fixed_t       tx;
  int           x1, x2;
  spritedef_t   *sprdef;
  spriteframe_t *sprframe;
  int           lump;
  boolean       flip;
  vissprite_t   *vis;
  vissprite_t   avis;

  // haleyjd: total invis. psprite disable

  if(viewplayer->mo->flags2 & MF2_DONTDRAW)
    return;

  // decide which patch to use

#ifdef RANGECHECK
  if ((unsigned) psp->state->sprite >= numsprites)
    I_Error ("R_DrawPSprite: invalid sprite number %i", psp->state->sprite);
#endif

  sprdef = &sprites[psp->state->sprite];

#ifdef RANGECHECK
  if ((psp->state->frame&FF_FRAMEMASK) >= sprdef->numframes)
    I_Error ("R_DrawPSprite: invalid frame %i for sprite %s",
             (int)(psp->state->frame & FF_FRAMEMASK),
             spritelist[psp->state->sprite]);
#endif

  sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

  lump = sprframe->lump[0];
  flip = ( (boolean) sprframe->flip[0] ) ^ lefthanded;

  // calculate edges of the shape
  tx = psp->sx-160*FRACUNIT;
  tx -= spriteoffset[lump];

  x1 = (centerxfrac + FixedMul (tx,pspritescale))>>FRACBITS;

  // off the right side
  if (x1 > viewwidth)
    return;

  tx +=  spritewidth[lump];
  x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

  // off the left side
  if (x2 < 0)
    return;
 
  if(lefthanded)
  {
        int tmpx=x1;
        x1=viewwidth-x2;
        x2=viewwidth-tmpx;    // viewwidth-x1
  }

  // store information in a vissprite
  vis = &avis;
  vis->mobjflags = 0;

  // killough 12/98: fix psprite positioning problem
  vis->texturemid = (BASEYCENTER<<FRACBITS) /* + FRACUNIT/2 */ -
                    (psp->sy-spritetopoffset[lump]);

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->scale = pspritescale;
  vis->colour = 0;      // sf: default colourmap

  if (flip)
    {
      vis->xiscale = -pspriteiscale;
      vis->startfrac = spritewidth[lump]-1;
    }
  else
    {
      vis->xiscale = pspriteiscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1)
    vis->startfrac += vis->xiscale*(vis->x1-x1);

  vis->patch = lump;

  if (viewplayer->powers[pw_invisibility] > 4*32
   || viewplayer->powers[pw_invisibility] & 8)
  {
           // sf: shadow draw now detected by flags
#ifdef FAUXTRAN
     if(general_translucency && faux_translucency) vis->mobjflags |= MF_TRANSLUCENT;
     else
#endif
       vis->mobjflags |= MF_SHADOW;                    // shadow draw
     vis->colormap = colormaps[global_cmap_index]; // haleyjd: NGCS -- was 0
  }
  else if (fixedcolormap)
    vis->colormap = fixedcolormap;           // fixed color
  else if (psp->state->frame & FF_FULLBRIGHT)
    vis->colormap = fullcolormap;            // full bright // killough 3/20/98
  else
    vis->colormap = spritelights[MAXLIGHTSCALE-1];  // local light

  if(psp->trans) // translucent gunflash
    vis->mobjflags |= MF_TRANSLUCENT;

  if(viewplayer->readyweapon == wp_bfg && bfglook==2)
  {
    R_DrawVisSprite (vis, vis->x1, vis->x2);
  }
  else
  {
    centery = viewheight/2 ;
    centeryfrac = centery << FRACBITS;
    R_DrawVisSprite (vis, vis->x1, vis->x2);
    centery = (viewheight/2) + updownangle;
    centeryfrac = centery<<FRACBITS;
  }
}

//
// R_DrawPlayerSprites
//

int R_Pspriteclip();

void R_DrawPlayerSprites(void)
{
  int i, lightnum;
  pspdef_t *psp;
  sector_t tmpsec;
  int floorlightlevel, ceilinglightlevel;
  int skip = (1 << viewplayer->readyweapon) & hide_weapon_on_flash;
  
        // sf: psprite switch
  if(!showpsprites || viewcamera) return;
    
  R_SectorColormap(viewplayer->mo->subsector->sector);

  // get light level
  // killough 9/18/98: compute lightlevel from floor and ceiling lightlevels
  // (see r_bsp.c for similar calculations for non-player sprites)

  R_FakeFlat(viewplayer->mo->subsector->sector, &tmpsec,
             &floorlightlevel, &ceilinglightlevel, 0);
  lightnum = ((floorlightlevel+ceilinglightlevel) >> (LIGHTSEGSHIFT+1))
    + extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS-1];
  else
    spritelights = scalelight[lightnum];

  for(i=0;i<viewwidth;i++)
  {
    pscreenheightarray[i] = viewheight;
  }

  // clip to screen bounds
  mfloorclip = pscreenheightarray;
  mceilingclip = negonearray;

  // add all active psprites
  for (i=0, psp=viewplayer->psprites; i<NUMPSPRITES; i++,psp++)
  {
    if(skip)
    {
      if(skip && i == ps_weapon) 
      {
        continue;
      }
      if(skip && i == ps_flash) 
      {
        if (psp->state)
        {
          R_DrawPSprite (psp);
        }
        else 
        {
          i = -1;
          psp = viewplayer->psprites-1;
        }
        skip = 0;
        continue;
      }
    }
    if (psp->state)
      R_DrawPSprite (psp);
  }
}

//
// R_SortVisSprites
//
// Rewritten by Lee Killough to avoid using unnecessary
// linked lists, and to use faster sorting algorithm.
//

// killough 9/22/98: inlined memcpy of pointer arrays

/* Julian 6/7/2001

        1) cleansed macro layout
        2) remove esi,edi,ecx from cloberred regs since used in constraints
           (useless on old gcc, error maker on modern versions)
*/

#ifdef DJGPP

#define bcopyp(d, s, n) \
asm(\
" cld\n"\
"rep\n" \
"movsl" :\
: "D"(d), "S"(s), "c"(n) : "%cc")

#else

#define bcopyp(d, s, n) memcpy(d, s, (n) * sizeof(void *))

#endif

// killough 9/2/98: merge sort

static void msort(vissprite_t **s, vissprite_t **t, int n)
{
  if (n >= 16)
    {
      int n1 = n/2, n2 = n - n1;
      vissprite_t **s1 = s, **s2 = s + n1, **d = t;

      msort(s1, t, n1);
      msort(s2, t, n2);

      while ((*s1)->scale > (*s2)->scale ?
             (*d++ = *s1++, --n1) : (*d++ = *s2++, --n2));

      if (n2)
        bcopyp(d, s2, n2);
      else
        bcopyp(d, s1, n1);

      bcopyp(s, t, n);
    }
  else
    {
      int i;
      for (i = 1; i < n; i++)
        {
          vissprite_t *temp = s[i];
          if (s[i-1]->scale < temp->scale)
            {
              int j = i;
              while ((s[j] = s[j-1])->scale < temp->scale && --j);
              s[j] = temp;
            }
        }
    }
}

void R_SortVisSprites (void)
{
  if (num_vissprite)
    {
      int i = num_vissprite;

      // If we need to allocate more pointers for the vissprites,
      // allocate as many as were allocated for sprites -- killough
      // killough 9/22/98: allocate twice as many

      if (num_vissprite_ptrs < num_vissprite*2)
        {
          free(vissprite_ptrs);  // better than realloc -- no preserving needed
          vissprite_ptrs = malloc((num_vissprite_ptrs = num_vissprite_alloc*2)
                                  * sizeof *vissprite_ptrs);
        }

      while (--i>=0)
        vissprite_ptrs[i] = vissprites+i;

      // killough 9/22/98: replace qsort with merge sort, since the keys
      // are roughly in order to begin with, due to BSP rendering.

      msort(vissprite_ptrs, vissprite_ptrs + num_vissprite, num_vissprite);
    }
}

// clip the psprite against the floor

int R_Pspriteclip()
{
        return 0;
}

//
// R_DrawSprite
//

void R_DrawSprite (vissprite_t* spr)
{
  drawseg_t *ds;
  short   clipbot[MAX_SCREENWIDTH];       // killough 2/8/98:
  short   cliptop[MAX_SCREENWIDTH];       // change to MAX_*
  int     x;
  int     r1;
  int     r2;
  fixed_t scale;
  fixed_t lowscale;

  for (x = spr->x1 ; x<=spr->x2 ; x++)
    clipbot[x] = cliptop[x] = -2;

  // Scan drawsegs from end to start for obscuring segs.
  // The first drawseg that has a greater scale is the clip seg.

  // Modified by Lee Killough:
  // (pointer check was originally nonportable
  // and buggy, by going past LEFT end of array):

  //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
    {      // determine if the drawseg obscures the sprite
      if (ds->x1 > spr->x2 || ds->x2 < spr->x1 ||
          (!ds->silhouette && !ds->maskedtexturecol))
        continue;      // does not cover sprite

      r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

      if (ds->scale1 > ds->scale2)
        {
          lowscale = ds->scale2;
          scale = ds->scale1;
        }
      else
        {
          lowscale = ds->scale1;
          scale = ds->scale2;
        }

      if (scale < spr->scale || (lowscale < spr->scale &&
                    !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
        {
          if (ds->maskedtexturecol)       // masked mid texture?
            R_RenderMaskedSegRange(ds, r1, r2);
          continue;               // seg is behind sprite
        }

      // clip this piece of the sprite
      // killough 3/27/98: optimized and made much shorter

      if (ds->silhouette&SIL_BOTTOM && spr->gz < ds->bsilheight) //bottom sil
        for (x=r1 ; x<=r2 ; x++)
          if (clipbot[x] == -2)
            clipbot[x] = ds->sprbottomclip[x];

      if (ds->silhouette&SIL_TOP && spr->gzt > ds->tsilheight)   // top sil
        for (x=r1 ; x<=r2 ; x++)
          if (cliptop[x] == -2)
            cliptop[x] = ds->sprtopclip[x];
    }

  // killough 3/27/98:
  // Clip the sprite against deep water and/or fake ceilings.
  // killough 4/9/98: optimize by adding mh
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
  // killough 11/98: fix disappearing sprites

  if (spr->heightsec != -1)  // only things in specially marked sectors
    {
      fixed_t h,mh;

      // haleyjd: yet another instance of assumption that only players
      // can be involved in determining the z partition...
      int phs = viewcamera ? viewcamera->heightsec :
	          viewplayer->mo->subsector->sector->heightsec;

      if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
          (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 &&
          (h >>= FRACBITS) < viewheight)
        if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
          {                          // clip bottom
            for (x=spr->x1 ; x<=spr->x2 ; x++)
              if (clipbot[x] == -2 || h < clipbot[x])
                clipbot[x] = h;
          }
        else                        // clip top
          if (phs != -1 && viewz <= sectors[phs].floorheight) // killough 11/98
            for (x=spr->x1 ; x<=spr->x2 ; x++)
              if (cliptop[x] == -2 || h > cliptop[x])
                cliptop[x] = h;

      if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
          (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 &&
          (h >>= FRACBITS) < viewheight)
        if (phs != -1 && viewz >= sectors[phs].ceilingheight)
          {                         // clip bottom
            for (x=spr->x1 ; x<=spr->x2 ; x++)
              if (clipbot[x] == -2 || h < clipbot[x])
                clipbot[x] = h;
          }
        else                       // clip top
          for (x=spr->x1 ; x<=spr->x2 ; x++)
            if (cliptop[x] == -2 || h > cliptop[x])
              cliptop[x] = h;
    }
  // killough 3/27/98: end special clipping for deep water / fake ceilings

  // all clipping has been performed, so draw the sprite
  // check for unclipped columns

  for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
      if (clipbot[x] == -2)
        clipbot[x] = viewheight;

      if (cliptop[x] == -2)
        cliptop[x] = -1;
    }

  mfloorclip = clipbot;
  mceilingclip = cliptop;
  R_DrawVisSprite (spr, spr->x1, spr->x2);
}

//
// R_DrawMasked
//

void R_DrawMasked(void)
{
  int i;
  drawseg_t *ds;

  if(drawparticles)
  {
     int i = activeParticles;
     while(i != -1)
     {
	R_ProjectParticle(Particles + i);
	i = Particles[i].next;
     }
  }

  R_SortVisSprites();

  // draw all vissprites back to front

  for(i = num_vissprite; --i >= 0; )
    R_DrawSprite(vissprite_ptrs[i]);         // killough

  // render any remaining masked mid textures

  // Modified by Lee Killough:
  // (pointer check was originally nonportable
  // and buggy, by going past LEFT end of array):

  //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

  for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
    if (ds->maskedtexturecol)
      R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

  // draw the psprites on top of everything
  //  but does not draw on side views
  if (!viewangleoffset)
    R_DrawPlayerSprites();
}

//=====================================================================
//
// haleyjd 09/30/01
//
// Particle Rendering
// This incorporates itself mostly seamlessly within the
// vissprite system, incurring only minor changes to the functions
// above.

//
// newParticle
//
// Tries to find an inactive particle in the Particles list
// Returns NULL on failure
//
particle_t *newParticle(void)
{
   particle_t *result = NULL;
   if(inactiveParticles != -1)
   {
      result = Particles + inactiveParticles;
      inactiveParticles = result->next;
      result->next = activeParticles;
      activeParticles = result - Particles;
   }
   return result;
}

void R_FreeParticles()
{
  free(Particles);
  Particles = 0;
}

//
// R_InitParticles
//
// Allocate the particle list and initialize it
//
void R_InitParticles(void)
{
   int i;

   numParticles = 0;

   if((i = M_CheckParm("-numparticles")) && i < myargc - 1)
      numParticles = atoi(myargv[i+1]);
   
   if(numParticles == 0) // assume default
   {
      numParticles = 4000;
   }
   else if(numParticles < 100)
   {
      numParticles = 100;
   }
   
   Particles = Z_Malloc(numParticles*sizeof(particle_t), PU_STATIC, NULL);
   R_ClearParticles();
}

//
// R_ClearParticles
//
// set up the particle list
//
void R_ClearParticles(void)
{
   int i;
   
   memset(Particles, 0, numParticles*sizeof(particle_t));
   activeParticles = -1;
   inactiveParticles = 0;
   for(i = 0; i < numParticles - 1; i++)
      Particles[i].next = i + 1;
   Particles[i].next = -1;
}

void R_ProjectParticle(particle_t *particle)
{
   fixed_t tr_x, tr_y;
   fixed_t gxt, gyt, gzt;
   fixed_t tx, tz;
   fixed_t xscale;
   int x1, x2;
   vissprite_t*	vis;
   sector_t* sector = NULL;
   fixed_t iscale;
   int heightsec = -1;
   
   // transform the origin point
   tr_x = particle->x - viewx;
   tr_y = particle->y - viewy;
   
   gxt = FixedMul(tr_x, viewcos); 
   gyt = -FixedMul(tr_y, viewsin);
   
   tz = gxt - gyt; 
   
   // particle is behind view plane?
   if(tz < MINZ)
      return;
   
   xscale = FixedDiv(projection, tz);
   //yscale = FixedDiv(projectiony, tz);
   
   gxt = -FixedMul(tr_x, viewsin); 
   gyt = FixedMul(tr_y, viewcos); 
   tx = -(gyt+gxt); 
   
   // too far off the side?
   if(abs(tx) > (tz<<2))
      return;
   
   // calculate edges of the shape
   x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
   
   // off the right side?
   if(x1 >= viewwidth)
      return;
   
   x2 = ((centerxfrac + 
          FixedMul(tx+particle->size*(FRACUNIT/4), xscale)) >> FRACBITS);
   
   // off the left side
   if(x2 < 0)
      return;
   
   gzt = particle->z+1;
   
   // killough 3/27/98: exclude things totally separated
   // from the viewer, by either water or fake ceilings
   // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
   
   {
      subsector_t *subsector = 
	 R_PointInSubsector(particle->x, particle->y);
      sector = subsector->sector;
      heightsec = sector->heightsec;
      if(particle->z < sector->floorheight || 
	 particle->z > sector->ceilingheight)
	 return;
   }
   
   // only clip particles which are in special sectors
   if(heightsec != -1)
   {
      int phs = viewcamera ? viewcamera->heightsec :
                viewplayer->mo->subsector->sector->heightsec;
      
      if(phs != -1 && 
	 viewz < sectors[phs].floorheight ?
	         particle->z >= sectors[heightsec].floorheight :
                 gzt < sectors[heightsec].floorheight)
	 return;

      if(phs != -1 && 
	 viewz > sectors[phs].ceilingheight ?
	         gzt < sectors[heightsec].ceilingheight &&
	           viewz >= sectors[heightsec].ceilingheight :
                 particle->z >= sectors[heightsec].ceilingheight)
	 return;
   }
   
   // store information in a vissprite
   vis = R_NewVisSprite();
   vis->heightsec = heightsec;
   vis->scale = xscale;
   vis->gx = particle->x;
   vis->gy = particle->y;
   vis->gz = particle->z;
   vis->gzt = gzt;
   vis->texturemid = vis->gzt - viewz;
   vis->x1 = x1 < 0 ? 0 : x1;
   vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
   //vis->translation = NULL;
   iscale = FixedDiv(FRACUNIT, xscale);
   vis->startfrac = particle->color;
   vis->xiscale = iscale;
   vis->patch = -1;
   vis->mobjflags = particle->trans;
   
   if(fixedcolormap)
   {
      vis->colormap = fixedcolormap;
   } 
   else
   {
      // haleyjd 01/12/02: wow is this code wrong! :)
      //int index = xscale>>(LIGHTSCALESHIFT + hires);
      //if(index >= MAXLIGHTSCALE) 
      //   index = MAXLIGHTSCALE-1;      
      //vis->colormap = spritelights[index];

      lighttable_t **ltable;
      sector_t tmpsec;
      int floorlightlevel, ceilinglightlevel, lightnum, index;

      R_FakeFlat(sector, &tmpsec, &floorlightlevel, &ceilinglightlevel,
	         false);

      lightnum = (floorlightlevel + ceilinglightlevel) / 2;
      lightnum = (lightnum >> LIGHTSEGSHIFT) + extralight;

      if(lightnum < 0)
	 ltable = scalelight[0];
      else if(lightnum >= LIGHTLEVELS)
	 ltable = scalelight[LIGHTLEVELS - 1];
      else
	 ltable = scalelight[lightnum];

      index = xscale >> (LIGHTSCALESHIFT + hires);
      if(index >= MAXLIGHTSCALE)
	 index = MAXLIGHTSCALE - 1;

      vis->colormap = ltable[index];
   }
}


#ifdef FAUXTRAN
//
// R_DrawCheckeredParticle
//
void R_DrawCheckeredParticle(vissprite_t *vis)
{
   int x1, x2;
   int yl, yh;
   byte color;

   x1 = vis->x1;
   x2 = vis->x2;
   if(x1 < 0)
      x1 = 0;
   if(x2 < x1)
      x2 = x1;
   if(x2 >= viewwidth)
      x2 = viewwidth - 1;

   yl = (centeryfrac - FixedMul(vis->texturemid, vis->scale) + 
         FRACUNIT - 1) >> FRACBITS;
   yh = yl + (x2 - x1);

   // due to square shape, it is unnecessary to clip the entire
   // particle
   if(yh >= mfloorclip[x1])
      yh = mfloorclip[x1]-1;
   if(yl <= mceilingclip[x1])
      yl = mceilingclip[x1]+1;
   if(yh >= mfloorclip[x2])
      yh = mfloorclip[x2]-1;
   if(yl <= mceilingclip[x2])
      yl = mceilingclip[x2]+1;

   color = vis->colormap[vis->startfrac];

   {
      int xcount, ycount, spacing;
      byte *dest;

      xcount = x2 - x1 + 1;
      ycount = yh - yl + 1;
      
      if(ycount <= 0)
	 return;

      spacing = (SCREENWIDTH << hires) - xcount;
      dest = ylookup[yl] + columnofs[x1];
      
      do // step in y
      {
	 int count = xcount;
         dc_faux = (yh - ycount + 1) ^ (x2 - count + 1);
         
	 do // step in x
	 {
            if((dc_faux & 1) || (!vis->mobjflags)) *dest = color;
	    dest += 1;	   // go to next pixel
            dc_faux ++;
	 } while(--count);
	 dest += spacing;  // go to next row
      } while(--ycount);
   }
}
#endif


//
// R_DrawParticle
//
// haleyjd: this function had to be mostly rewritten
//
void R_DrawParticle(vissprite_t *vis)
{
   int x1, x2;
   int yl, yh;
   byte color;

   x1 = vis->x1;
   x2 = vis->x2;
   if(x1 < 0)
      x1 = 0;
   if(x2 < x1)
      x2 = x1;
   if(x2 >= viewwidth)
      x2 = viewwidth - 1;

   yl = (centeryfrac - FixedMul(vis->texturemid, vis->scale) + 
         FRACUNIT - 1) >> FRACBITS;
   yh = yl + (x2 - x1);

   // due to square shape, it is unnecessary to clip the entire
   // particle
   if(yh >= mfloorclip[x1])
      yh = mfloorclip[x1]-1;
   if(yl <= mceilingclip[x1])
      yl = mceilingclip[x1]+1;
   if(yh >= mfloorclip[x2])
      yh = mfloorclip[x2]-1;
   if(yl <= mceilingclip[x2])
      yl = mceilingclip[x2]+1;

   color = vis->colormap[vis->startfrac];

   {
      int xcount, ycount, spacing;
      byte *dest;

      xcount = x2 - x1 + 1;
      ycount = yh - yl + 1;
      
      if(ycount <= 0)
	 return;

      spacing = (SCREENWIDTH << hires) - xcount;
      dest = ylookup[yl] + columnofs[x1];
      
      do // step in y
      {
	 int count = xcount;
	 
	 do // step in x
	 {
	    if(general_translucency && vis->mobjflags)
	       *dest = main_tranmap[(*dest << 8) + color];
	    else
	       *dest = color;
	    dest += 1;	   // go to next pixel
	 } while(--count);
	 dest += spacing;  // go to next row
      } while(--ycount);
   }
}

//----------------------------------------------------------------------------
//
// $Log: r_things.c,v $
// Revision 1.22  1998/05/03  22:46:41  killough
// beautification
//
// Revision 1.21  1998/05/01  15:26:50  killough
// beautification
//
// Revision 1.20  1998/04/27  02:04:43  killough
// Fix incorrect I_Error format string
//
// Revision 1.19  1998/04/24  11:03:26  jim
// Fixed bug in sprites in PWAD
//
// Revision 1.18  1998/04/13  09:45:30  killough
// Fix sprite clipping under fake ceilings
//
// Revision 1.17  1998/04/12  02:02:19  killough
// Fix underwater sprite clipping, add wall translucency
//
// Revision 1.16  1998/04/09  13:18:48  killough
// minor optimization, plus fix ghost sprites due to huge z-height diffs
//
// Revision 1.15  1998/03/31  19:15:27  killough
// Fix underwater sprite clipping bug
//
// Revision 1.14  1998/03/28  18:15:29  killough
// Add true deep water / fake ceiling sprite clipping
//
// Revision 1.13  1998/03/23  03:41:43  killough
// Use 'fullcolormap' for fully-bright colormap
//
// Revision 1.12  1998/03/16  12:42:37  killough
// Optimize away some function pointers
//
// Revision 1.11  1998/03/09  07:28:16  killough
// Add primitive underwater support
//
// Revision 1.10  1998/03/02  11:48:59  killough
// Add failsafe against texture mapping overflow crashes
//
// Revision 1.9  1998/02/23  04:55:52  killough
// Remove some comments
//
// Revision 1.8  1998/02/20  22:53:22  phares
// Moved TRANMAP initialization to w_wad.c
//
// Revision 1.7  1998/02/20  21:56:37  phares
// Preliminarey sprite translucency
//
// Revision 1.6  1998/02/09  03:23:01  killough
// Change array decl to use MAX screen width/height
//
// Revision 1.5  1998/02/02  13:32:49  killough
// Performance tuning, program beautification
//
// Revision 1.4  1998/01/26  19:24:50  phares
// First rev with no ^Ms
//
// Revision 1.3  1998/01/26  06:13:58  killough
// Performance tuning
//
// Revision 1.2  1998/01/23  20:28:14  jim
// Basic sprite/flat functionality in PWAD added
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
