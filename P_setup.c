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
//  Do all the WAD I/O, get map description,
//  set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_setup.c,v 1.16 1998/05/07 00:56:49 killough Exp $";

#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "doomstat.h"
#include "hu_frags.h"
#include "m_bbox.h"
#include "m_argv.h"
#include "g_game.h"
#include "w_wad.h"
#include "p_hubs.h"
#include "r_main.h"
#include "r_things.h"
#include "r_sky.h"
#include "p_chase.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_skin.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_enemy.h"
#include "p_info.h"
#include "r_defs.h"
#include "s_sound.h"
#include "t_script.h"
#include "p_anim.h"  // haleyjd: lightning
#include "hu_fspic.h" // haleyjd
#include "p_partcl.h"
#include "d_dialog.h"
#include "d_io.h" // SoM 3/14/2002: strncasecmp
#include "t_vari.h"
#include "ex_stuff.h"

void T_BuildGameArrays(void); // in t_array.c

extern int level_error;

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

int      newlevel = false;
int      doom1level = false;    // doom 1 level running under doom 2
char     levelmapname[10];

int      numvertexes;
vertex_t *vertexes;

int      numsegs;
seg_t    *segs;

int      numsectors;
sector_t *sectors;

int      numsubsectors;
subsector_t *subsectors;

int      numnodes;
node_t   *nodes;
#ifdef NO_RECURSION_BSP
int      *nodepath;
#endif

int      numlines;
line_t   *lines;

int      numsides;
side_t   *sides;

int      numthings;
mobj_t   **spawnedthings;               // array of spawned things

boolean EternityMode;  // haleyjd 09/18/99
boolean MapUseFullBright;
boolean DoubleSky; // enables hexen-style parallax skies

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.

int       bmapwidth, bmapheight;  // size in mapblocks

// killough 3/1/98: remove blockmap limit internally:
long      *blockmap;              // was short -- killough

// offsets in blockmap are from here
long      *blockmaplump;          // was short -- killough

fixed_t   bmaporgx, bmaporgy;     // origin of block map

mobj_t    **blocklinks;           // for thing chains

//
// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without the special effect, this could
// be used as a PVS lookup as well.
//

byte *rejectmatrix;

// Maintain single and multi player starting spots.

// 1/11/98 killough: Remove limit on deathmatch starts
mapthing_t *deathmatchstarts;      // killough
size_t     num_deathmatchstarts;   // killough

mapthing_t *deathmatch_p;
mapthing_t playerstarts[MAXPLAYERS];


static char msgbuf[256];

int randomize_music = 0;
int random_mus_num = -1;


static boolean P_ShouldRandomizeMusic(int mapLumpNum)
{
  lumpinfo_t * mapLump = lumpinfo[mapLumpNum], * musLump = NULL;
  int musLumpNum = S_NowPlayingLumpNum();

  if(musLumpNum < 0 || !randomize_music)
    return FALSE;

  if(randomize_music == randm_always)
    return TRUE;

  musLump = lumpinfo[musLumpNum];

  if(randomize_music == randm_no_runnin)
    {
      if(W_IsLumpFromIWAD(musLumpNum))
        if(!strnicmp("D_RUNNIN", musLump->name, 8)
         ||!strnicmp("D_E1M1", musLump->name, 8))
           return TRUE;
    }

  if(*info_music)
    return FALSE;

  if(!W_IsLumpFromIWAD(musLumpNum))
      return FALSE;

  if(estimated_maps_no > 3)
    return FALSE;

  return TRUE;
}


//
// P_LoadVertexes
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadVertexes (int lump)
{
  byte *data;
  int i;

  // Determine number of lumps:
  //  total lump length / vertex record length.
  numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

  // Allocate zone memory for buffer.
  vertexes = Z_Malloc(numvertexes*sizeof(vertex_t),PU_LEVEL,0);

  // Load data into cache.
  data = W_CacheLumpNum(lump, PU_STATIC);

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i=0; i<numvertexes; i++)
    {
      vertexes[i].x = SHORT(((mapvertex_t *) data)[i].x)<<FRACBITS;
      vertexes[i].y = SHORT(((mapvertex_t *) data)[i].y)<<FRACBITS;
    }

  // Free buffer memory.
  Z_Free (data);
}

//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadSegs (int lump)
{
  int i;
  byte *data;
  size_t sz;

  sz = W_LumpLength(lump);
  
  if(sz % sizeof(mapseg_t)) return;
  numsegs = sz / sizeof(mapseg_t);
  segs = Z_Malloc(numsegs*sizeof(seg_t),PU_LEVEL,0);
  memset(segs, 0, numsegs*sizeof(seg_t));
  data = W_CacheLumpNum(lump,PU_STATIC);

  for (i=0; i<numsegs; i++)
    {
      seg_t *li = segs+i;
      mapseg_t *ml = (mapseg_t *) data + i;

      int side, linedef;
      line_t *ldef;

      li->v1 = &vertexes[(unsigned short)SHORT(ml->v1)];
      li->v2 = &vertexes[(unsigned short)SHORT(ml->v2)];

      li->angle = (SHORT(ml->angle))<<16;
      li->offset = (SHORT(ml->offset))<<16;
      linedef = (unsigned short)SHORT(ml->linedef);
      ldef = &lines[linedef];
      li->linedef = ldef;
      side = SHORT(ml->side);
      li->sidedef = &sides[(unsigned short)(ldef->sidenum[side])];
      li->frontsector = sides[(unsigned short)(ldef->sidenum[side])].sector;

      // killough 5/3/98: ignore 2s flag if second sidedef missing:
      if (ldef->flags & ML_TWOSIDED && (unsigned short)(ldef->sidenum[side^1])!=0xffff)
        li->backsector = sides[(unsigned short)(ldef->sidenum[side^1])].sector;
      else
        li->backsector = 0;
    }

  Z_Free (data);
}


// for those weird segs with 32 bit vertex references...

void P_LoadSegs32 (int lump)
{
  int i;
  byte *data;
  size_t sz;

  sz = W_LumpLength(lump);
  if(sz % sizeof(mapsegext_t)) return;
  numsegs = W_LumpLength(lump) / sizeof(mapsegext_t);
  segs = Z_Malloc(numsegs*sizeof(seg_t),PU_LEVEL,0);
  memset(segs, 0, numsegs*sizeof(seg_t));
  data = W_CacheLumpNum(lump,PU_STATIC);

  for (i=0; i<numsegs; i++)
    {
      seg_t *li = segs+i;
      mapsegext_t *ml = (mapsegext_t *) data + i;

      int side, linedef;
      line_t *ldef;

      li->v1 = &vertexes[LONG(ml->v1)];
      li->v2 = &vertexes[LONG(ml->v2)];

      li->angle = (SHORT(ml->angle))<<16;
      li->offset = (SHORT(ml->offset))<<16;
      linedef = (unsigned short)SHORT(ml->linedef);
      ldef = &lines[linedef];
      li->linedef = ldef;
      side = SHORT(ml->side);
      li->sidedef = &sides[(unsigned short)(ldef->sidenum[side])];
      li->frontsector = sides[(unsigned short)(ldef->sidenum[side])].sector;
      // killough 5/3/98: ignore 2s flag if second sidedef missing:
      if (ldef->flags & ML_TWOSIDED && (unsigned short)(ldef->sidenum[side^1])!=0xffff)
        li->backsector = sides[(unsigned short)(ldef->sidenum[side^1])].sector;
      else
        li->backsector = 0;
    }

  Z_Free (data);
}

static void P_ValidateSegs()
{
  int i;
  seg_t *seg = segs;
  if(!numsegs) return;
  
  for(i = 0; i < numsegs ; i +=1, seg++)
  {
    if(seg->v1 - vertexes < 0) break;
    if(seg->v1 - vertexes >= numvertexes) break;     
    if(seg->v2 - vertexes < 0) break;
    if(seg->v2 - vertexes >= numvertexes) break;
    if(seg->linedef - lines < 0) break;
    if(seg->linedef - lines >= numlines) break;
    if(seg->sidedef - sides < 0) break;
    if(seg->sidedef - sides >= numsides) break;
  }
  
  if(i < numsegs)
  {
    sprintf(msgbuf, "seg=%d, v1=%ld, v2=%ld, linedef=%ld, sidedef=%ld\n", i, seg->v1 - vertexes,
      seg->v2 - vertexes, seg->linedef - lines, seg->sidedef - sides);
    DEBUGMSG(msgbuf);  
  
    numsegs = 0;
    Z_Free(segs);
    segs = 0;
  }
}

//
// P_LoadSubsectors
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadSubsectors (int lump)
{
  byte *data;
  int i;
  size_t sz;
  
  sz = W_LumpLength(lump);
  if(sz % sizeof(mapsubsector_t)) return;
  numsubsectors = sz / sizeof(mapsubsector_t);
  subsectors = Z_Malloc(numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
  data = W_CacheLumpNum(lump, PU_STATIC);

  memset(subsectors, 0, numsubsectors*sizeof(subsector_t));

  for (i=0; i<numsubsectors; i++)
    {
      subsectors[i].numlines  = SHORT(((mapsubsector_t *) data)[i].numsegs );
      subsectors[i].firstline = (unsigned short)SHORT(((mapsubsector_t *) data)[i].firstseg);
    }

  Z_Free (data);
}

void P_LoadSubsectors32 (int lump)
{
  byte *data;
  int i;
  size_t sz;

  sz = W_LumpLength(lump);
  if(sz % sizeof(mapsubsectorext_t)) return;
  numsubsectors = sz / sizeof(mapsubsectorext_t);
  subsectors = Z_Malloc(numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
  data = W_CacheLumpNum(lump, PU_STATIC);

  memset(subsectors, 0, numsubsectors*sizeof(subsector_t));

  for (i=0; i<numsubsectors; i++)
    {
      subsectors[i].numlines  = SHORT(((mapsubsectorext_t *) data)[i].numsegs );
      subsectors[i].firstline = LONG(((mapsubsectorext_t *) data)[i].firstseg);
    }

  Z_Free (data);
}

static void P_ValidateSubsectors()
{
  int i;
  subsector_t *ss = subsectors;
  
  if(!numsubsectors) return;
  
  for(i = 0; i < numsubsectors ; i += 1, ss += 1)
  {
    if(ss->firstline >= numsegs) break;
    if(ss->firstline + (unsigned short) ss->numlines > numsegs) break;
  }
  
  if(i < numsubsectors)
  {
    sprintf(msgbuf, "ss=%d, ss->firstline=%d, ss->numlines=%d\n", i, ss->firstline, (unsigned) ss->numlines);
    DEBUGMSG(msgbuf);
    
    numsubsectors = 0;
    Z_Free(subsectors);
    subsectors = 0;
  }
  
}

//
// P_LoadSectors
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadSectors (int lump)
{
  byte *data;
  int  i;

  numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
  sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
  memset (sectors, 0, numsectors*sizeof(sector_t));
  data = W_CacheLumpNum (lump,PU_STATIC);

  for (i=0; i<numsectors; i++)
    {
      sector_t *ss = sectors + i;
      const mapsector_t *ms = (mapsector_t *) data + i;

      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
      ss->floorpic = R_FlatNumForName(ms->floorpic);
      ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
      ss->lightlevel = SHORT(ms->lightlevel);
      ss->special = SHORT(ms->special);
      ss->oldspecial = SHORT(ms->special);
      ss->tag = SHORT(ms->tag);
      ss->thinglist = NULL;
      ss->touching_thinglist = NULL;            // phares 3/14/98

      ss->nextsec = -1; //jff 2/26/98 add fields to support locking out
      ss->prevsec = -1; // stair retriggering until build completes

      // killough 3/7/98:
      ss->floor_xoffs = 0;
      ss->floor_yoffs = 0;      // floor and ceiling flats offsets
      ss->ceiling_xoffs = 0;
      ss->ceiling_yoffs = 0;
      ss->heightsec = -1;       // sector used to get floor and ceiling height
      ss->floorlightsec = -1;   // sector used to get floor lighting
      // killough 3/7/98: end changes

      // killough 4/11/98 sector used to get ceiling lighting:
      ss->ceilinglightsec = -1;

      // killough 4/4/98: colormaps:
      ss->bottommap = ss->midmap = ss->topmap = 0;

      // killough 10/98: sky textures coming from sidedefs:
      ss->sky = 0;

      // [kb] for R_FixWiggle()
      ss->cachedheight = -1;
    }

  Z_Free (data);
}


//
// P_LoadNodes
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadNodes32 (int lump);

void P_LoadNodes (int lump)
{
  byte *data;
  int  i;
  size_t sz;
  
  sz = W_LumpLength(lump);
  if(sz % sizeof(mapnode_t)) return;
  
  data = W_CacheLumpNum (lump, PU_STATIC);
  if(sz > 8 && !strncmp("xNd4", data, 4) && 0 == ((long *)data)[1])
  {
    Z_Free(data);
    P_LoadNodes32(lump);
    return; 
  }
  
  numnodes = sz / sizeof(mapnode_t);
  nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
#ifdef NO_RECURSION_BSP
  nodepath = Z_Calloc (numnodes, sizeof(int), PU_LEVEL, 0);
#endif

  for (i=0; i<numnodes; i++)
    {
      node_t *no = nodes + i;
      mapnode_t *mn = (mapnode_t *) data + i;
      int j;

      no->x = SHORT(mn->x)<<FRACBITS;
      no->y = SHORT(mn->y)<<FRACBITS;
      no->dx = SHORT(mn->dx)<<FRACBITS;
      no->dy = SHORT(mn->dy)<<FRACBITS;

      for (j=0 ; j<2 ; j++)
        {
          int k;
          short ch = SHORT(mn->children[j]);
          if(ch == -1)  
            no->children[j] = -1;
          else if(ch & NF_SUBSECTOR)  
            {
              no->children[j] = (((unsigned short)ch) & ~NF_SUBSECTOR) | NFX_SUBSECTOR;
            }
          else 
            no->children[j] = (unsigned short) ch;
          for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }

  Z_Free (data);
}

void P_LoadNodes32 (int lump)
{
  byte *data;
  int  i;
  size_t sz;
  
  sz = W_LumpLength(lump);
  if((sz - 8) % sizeof(mapnodeext_t)) return;
  
  numnodes = (sz - 8) / sizeof(mapnodeext_t);
  nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump, PU_STATIC) + 8;
#ifdef NO_RECURSION_BSP
  nodepath = Z_Calloc (numnodes, 1, PU_LEVEL, 0);
#endif

  for (i=0; i<numnodes; i++)
    {
      node_t *no = nodes + i;
      mapnodeext_t *mn = (mapnodeext_t *) data + i;
      int j;

      no->x = SHORT(mn->x)<<FRACBITS;
      no->y = SHORT(mn->y)<<FRACBITS;
      no->dx = SHORT(mn->dx)<<FRACBITS;
      no->dy = SHORT(mn->dy)<<FRACBITS;

      for (j=0 ; j<2 ; j++)
        {
          int k;
          no->children[j] = LONG(mn->children[j]);
          for (k=0 ; k<4 ; k++)
            no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }

  Z_Free (data - 8);
}

//
// P_LoadThings
//
// killough 5/3/98: reformatted, cleaned up
//
// sf: added spawnedthings for scripting
//
// haleyjd: added missing player start detection
//

void P_LoadThings(int lump)
{
   int  i;
   byte *data = W_CacheLumpNum(lump, PU_STATIC);
   
   numthings = W_LumpLength(lump) / sizeof(mapthing_t); //sf: use global
   spawnedthings = Z_Malloc(numthings * sizeof(mobj_t*), PU_LEVEL, 0);

   // haleyjd: explicitly nullify old player object pointers
   if(!deathmatch)
   {
      for(i = 0; i < MAXPLAYERS; i++)
      {
	 if(playeringame[i])
	    players[i].mo = NULL;
      }
   }
   
   for(i = 0; i < numthings; i++)
   {
      mapthing_t *mt = (mapthing_t *) data + i;
      boolean dont_spawn = false;

      // Do not spawn cool, new monsters if !commercial
      if(gamemode != commercial)
      {
	 switch(mt->type)
	 {
	 case 68:  // Arachnotron
	 case 64:  // Archvile
	 case 88:  // Boss Brain
	 case 89:  // Boss Shooter
	 case 69:  // Hell Knight
	 case 67:  // Mancubus
	 case 71:  // Pain Elemental
	 case 65:  // Former Human Commando
	 case 66:  // Revenant
	 case 84:  // Wolf SS
            dont_spawn = true;
	 }
      }
      
      // non SMMU stuff from Eternity or COD will
      // only be loaded when appropriate
      if(mt->type < 5001 || mt->type > 5004)
      {
         dont_spawn = dont_spawn || !(
           mt->type < 4988 ||
           (gamemission == cod && mt->type >= 4988) ||
           (EternityMode && mt->type >= 6001) 
         );
      }

      if(dont_spawn)
      {
         spawnedthings[i] = NULL; // haleyjd
         continue;
      }
      
      // Do spawn all other stuff.
      mt->x = SHORT(mt->x);
      mt->y = SHORT(mt->y);
      mt->angle = SHORT(mt->angle);
      mt->type  = SHORT(mt->type);
      mt->options = SHORT(mt->options);
      
      spawnedthings[i] = P_SpawnMapThing(mt);
#ifdef BOSSACTION
      if(info_bossaction_thingtype == mt->type && spawnedthings[i])
      {
         spawnedthings[i]->flags2 |= MF2_MAPINFOBOSS;
      }
#endif      
   }
   
   // haleyjd: all player things for players in this game
   //          should now be valid in SP or co-op
   if(!deathmatch)
   {
      for(i = 0; i < MAXPLAYERS; i++)
      {
	 if(playeringame[i] && !players[i].mo)
	 {
	    I_Error("P_LoadThings: Missing required player start %i", 
	            i+1);
	 }
      }
   }

   Z_Free(data);
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//        ^^^
// ??? killough ???
// Does this mean secrets used to be linedef-based, rather than sector-based?
//
// killough 4/4/98: split into two functions, to allow sidedef overloading
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadLineDefs (int lump)
{
  byte *data;
  int  i;

  numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
  lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);
  memset (lines, 0, numlines*sizeof(line_t));
  data = W_CacheLumpNum (lump,PU_STATIC);

  for (i=0; i<numlines; i++)
    {
      maplinedef_t *mld = (maplinedef_t *) data + i;
      line_t *ld = lines+i;
      vertex_t *v1, *v2;

      ld->flags = SHORT(mld->flags);
      ld->special = SHORT(mld->special);
      ld->tag = SHORT(mld->tag);
      v1 = ld->v1 = &vertexes[(unsigned short)SHORT(mld->v1)];
      v2 = ld->v2 = &vertexes[(unsigned short)SHORT(mld->v2)];
      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;

      ld->tranlump = -1;   // killough 4/11/98: no translucency by default

      ld->slopetype = !ld->dx ? ST_VERTICAL : !ld->dy ? ST_HORIZONTAL :
	FixedDiv(ld->dy, ld->dx) > 0 ? ST_POSITIVE : ST_NEGATIVE;

      if (v1->x < v2->x)
	{
	  ld->bbox[BOXLEFT] = v1->x;
	  ld->bbox[BOXRIGHT] = v2->x;
	}
      else
	{
	  ld->bbox[BOXLEFT] = v2->x;
	  ld->bbox[BOXRIGHT] = v1->x;
	}

      if (v1->y < v2->y)
	{
	  ld->bbox[BOXBOTTOM] = v1->y;
	  ld->bbox[BOXTOP] = v2->y;
	}
      else
	{
	  ld->bbox[BOXBOTTOM] = v2->y;
	  ld->bbox[BOXTOP] = v1->y;
	}

      ld->sidenum[0] = SHORT(mld->sidenum[0]);
      ld->sidenum[1] = SHORT(mld->sidenum[1]);

      // killough 4/4/98: support special sidedef interpretation below
      if ((unsigned short)(ld->sidenum[0]) != 0xffff && ld->special)
        sides[(unsigned short)(*ld->sidenum)].special = ld->special;
    }
  Z_Free (data);
}

// killough 4/4/98: delay using sidedefs until they are loaded
// killough 5/3/98: reformatted, cleaned up

void P_LoadLineDefs2(int lump)
{
  int i = numlines;
  register line_t *ld = lines;
  for (;i--;ld++)
    {
      // killough 11/98: fix common wad errors (missing sidedefs):

      if ((unsigned short)(ld->sidenum[0]) == 0xffff)
	ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side

      if ((unsigned short)(ld->sidenum[1]) == 0xffff)
	ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side

      ld->frontsector = ((unsigned short)(ld->sidenum[0])!=0xffff) ? sides[(unsigned short)(ld->sidenum[0])].sector : 0;
      ld->backsector  = ((unsigned short)(ld->sidenum[1])!=0xffff) ? sides[(unsigned short)(ld->sidenum[1])].sector : 0;
      switch (ld->special)
	{                       // killough 4/11/98: handle special types
	  int lump, j;

	case 260:               // killough 4/11/98: translucent 2s textures
            lump = sides[(unsigned short)(*ld->sidenum)].special; // translucency from sidedef
	    if (!ld->tag)                       // if tag==0,
	      ld->tranlump = lump;              // affect this linedef only
	    else
	      for (j=0;j<numlines;j++)          // if tag!=0,
		if (lines[j].tag == ld->tag)    // affect all matching linedefs
		  lines[j].tranlump = lump;
	    break;
	}
    }
}

//
// P_LoadSideDefs
//
// killough 4/4/98: split into two functions

void P_LoadSideDefs (int lump)
{
  numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
  sides = Z_Malloc(numsides*sizeof(side_t),PU_LEVEL,0);
  memset(sides, 0, numsides*sizeof(side_t));
}

// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

void P_LoadSideDefs2(int lump)
{
  byte *data = W_CacheLumpNum(lump,PU_STATIC);
  int  i;

  for (i=0; i<numsides; i++)
    {
      register mapsidedef_t *msd = (mapsidedef_t *) data + i;
      register side_t *sd = sides + i;
      register sector_t *sec;

      sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

      // killough 4/4/98: allow sidedef texture names to be overloaded
      // killough 4/11/98: refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.

      sd->sector = sec = &sectors[(unsigned short)SHORT(msd->sector)];
      switch (sd->special)
	{
	case 242:                       // variable colormap via 242 linedef
	  sd->bottomtexture =
	    (sec->bottommap =   R_ColormapNumForName(msd->bottomtexture)) < 0 ?
	    sec->bottommap = 0, R_TextureNumForName(msd->bottomtexture): 0 ;
	  sd->midtexture =
	    (sec->midmap =   R_ColormapNumForName(msd->midtexture)) < 0 ?
	    sec->midmap = 0, R_TextureNumForName(msd->midtexture)  : 0 ;
	  sd->toptexture =
	    (sec->topmap =   R_ColormapNumForName(msd->toptexture)) < 0 ?
	    sec->topmap = 0, R_TextureNumForName(msd->toptexture)  : 0 ;
	  break;

	case 260: // killough 4/11/98: apply translucency to 2s normal texture
	  sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
	    (sd->special = W_CheckNumForName(msd->midtexture)) < 0 ||
	    W_LumpLength(sd->special) != 65536 ?
	    sd->special=0, R_TextureNumForName(msd->midtexture) :
	      (sd->special++, 0) : (sd->special=0);
	  sd->toptexture = R_TextureNumForName(msd->toptexture);
	  sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
	  break;

	default:                        // normal cases
	  sd->midtexture = R_TextureNumForName(msd->midtexture);
	  sd->toptexture = R_TextureNumForName(msd->toptexture);
	  sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
	  break;
	}
    }
  Z_Free (data);
}

byte * P_LoadVerticesExtended(byte * data)
{
  int OrgVerts, NewVerts;
  int i;
  vertex_t * old;
  OrgVerts = LONG(*(long *)data);
  data += sizeof(long);
  NewVerts = LONG(*(long *)data);
  data += sizeof(long);

  sprintf(msgbuf, "OrgVerts=%d, NewVerts=%d, numvers=%d\n", OrgVerts, NewVerts, numvertexes);
  DEBUGMSG(msgbuf);

  assert(OrgVerts <= numvertexes);
  numvertexes = OrgVerts + NewVerts;
  old = vertexes;
  vertexes = Z_Realloc(vertexes, (OrgVerts + NewVerts) * sizeof(vertex_t), PU_LEVEL, 0);
  for(i = 0 ; i < numlines ; i += 1)
    {
       lines[i].v1 = &vertexes[lines[i].v1 - old];
       lines[i].v2 = &vertexes[lines[i].v2 - old];
    }
  
  for(i = OrgVerts; i < numvertexes ; i += 1)
    {
      vertexes[i].x = LONG(*(long *)data);
      data += sizeof(long);
      vertexes[i].y = LONG(*(long *)data);
      data += sizeof(long);
    }
  return data;
}


byte * P_LoadSubsectorsExtended(byte * data)
{
  int first = 0, i;
  numsubsectors = LONG(*(long *)data);
  data += sizeof(long);
  subsectors = Z_Malloc(numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
  memset(subsectors, 0, numsubsectors*sizeof(subsector_t));

  sprintf(msgbuf, "numsubsectors=%d\n", numsubsectors);
  DEBUGMSG(msgbuf);

  for (i=0; i<numsubsectors; i++)
    {
      subsectors[i].numlines = LONG(*(long *)data);
      data += sizeof(long);
      subsectors[i].firstline = first;
      first += subsectors[i].numlines;
    }

  return data;
}

byte * P_LoadSegsExtended(byte * data)
{
  int i;
  fixed_t dx, dy;
  numsegs = LONG(*(long *)data);
  data += sizeof(long);
  segs = Z_Malloc(numsegs*sizeof(seg_t),PU_LEVEL,0);
  memset(segs, 0, numsegs*sizeof(seg_t));

  sprintf(msgbuf, "numsegs=%d\n", numsegs);
  DEBUGMSG(msgbuf);

  for (i=0; i<numsegs; i++)
    {
      seg_t *li = segs+i;
      int side, linedef;
      line_t *ldef;

      li->v1 = &vertexes[LONG(*(long *)data)];
      data += sizeof(long);
      li->v2 = &vertexes[LONG(*(long *)data)];
      data += sizeof(long);
      
      linedef = (unsigned short)SHORT(*(short *)data);
      data += sizeof(short);
      ldef = &lines[linedef];
      li->linedef = ldef;
      
      dx = li->v2->x - li->v1->x;
      dy = li->v2->y - li->v1->y;
      
      // these are fairly arbitrary at the moment...
      if((li->linedef->dx > 0 && dx > 0) || (li->linedef->dx < 0 && dx < 0))
        li->offset = R_PointToDist2(li->linedef->v1->x, li->linedef->v1->y, li->v1->x, li->v1->y);
      else if(li->linedef->dx < 0)
        li->offset = R_PointToDist2(li->linedef->v2->x, li->linedef->v2->y, li->v1->x, li->v1->y);
      else if(dx < 0)
        li->offset = R_PointToDist2(li->linedef->v2->x, li->linedef->v2->y, li->v1->x, li->v1->y);
      else if((li->linedef->dy > 0 && dy > 0) || (li->linedef->dy < 0 && dy < 0))
        li->offset = R_PointToDist2(li->linedef->v1->x, li->linedef->v1->y, li->v1->x, li->v1->y);
      else if(li->linedef->dy < 0)
        li->offset = R_PointToDist2(li->linedef->v2->x, li->linedef->v2->y, li->v1->x, li->v1->y);
      else
        li->offset = R_PointToDist2(li->linedef->v2->x, li->linedef->v2->y, li->v1->x, li->v1->y);
      li->angle = R_PointToAngle2(li->v1->x, li->v1->y, li->v2->x, li->v2->y);

      side = *(byte *)data;
      data += sizeof(byte);
      li->sidedef = &sides[(unsigned short)(ldef->sidenum[side])];
      li->frontsector = sides[(unsigned short)(ldef->sidenum[side])].sector;

      // killough 5/3/98: ignore 2s flag if second sidedef missing:
      if (ldef->flags & ML_TWOSIDED && (unsigned short)(ldef->sidenum[side^1])!=0xffff)
        li->backsector = sides[(unsigned short)(ldef->sidenum[side^1])].sector;
      else
        li->backsector = 0;
    }
    
    return data;
}

byte * P_LoadNodesExtended(byte * data)
{
  int  i;

  numnodes = LONG(*(long *)data);
  data += sizeof(long);
  nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);

  sprintf(msgbuf, "numnodes=%d\n", numnodes);
  DEBUGMSG(msgbuf);

  for (i=0; i<numnodes; i++)
    {
      int j;
      node_t *no = nodes + i;

      no->x = SHORT(*(short *)data)<<FRACBITS;
      data += sizeof(short);
      no->y = SHORT(*(short *)data)<<FRACBITS;
      data += sizeof(short);
      no->dx = SHORT(*(short *)data)<<FRACBITS;
      data += sizeof(short);
      no->dy = SHORT(*(short *)data)<<FRACBITS;
      data += sizeof(short);

      for (j=0 ; j<2 ; j++)
        {
          int k;
          for (k=0 ; k<4 ; k++)
            {
              no->bbox[j][k] = SHORT(*(short *)data)<<FRACBITS;
              data += sizeof(short);
            }
        }
      
      for (j=0 ; j<2 ; j++)
        {
           no->children[j] = LONG(*(long *)data);
           data += sizeof(long);
        }
    }

  return data;
}


void P_LoadExtended(int lumpnum)
{
  byte *data = W_CacheLumpNum(lumpnum, PU_STATIC);
  int len = W_LumpLength(lumpnum);
  byte * runner = data;
  
  if(len < 4 || strncmp("XNOD", runner, 4)) 
    {
      Z_Free(data);
      return;
    }

  runner += 4;
  DEBUGMSG("P_LoadVerticesExtended\n");
  runner = P_LoadVerticesExtended(runner);
  DEBUGMSG("P_LoadSubsectorsExtended\n");
  runner = P_LoadSubsectorsExtended(runner);
  DEBUGMSG("P_LoadSegsExtended\n");
  runner = P_LoadSegsExtended(runner);
  DEBUGMSG("P_LoadNodesExtended\n");
  runner = P_LoadNodesExtended(runner);
  DEBUGMSG("P_LoadExtended done\n");
  
  Z_Free(data);
}

//
// killough 10/98:
//
// Rewritten to use faster algorithm.
//
// New procedure uses Bresenham-like algorithm on the linedefs, adding the
// linedef to each block visited from the beginning to the end of the linedef.
//
// The algorithm's complexity is on the order of nlines*total_linedef_length.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.

static void P_CreateBlockMap(void)
{
  register int i;
  fixed_t minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;

  // First find limits of map

  for (i=0; i<numvertexes; i++)
    {
      if (vertexes[i].x >> FRACBITS < minx)
	minx = vertexes[i].x >> FRACBITS;
      else
	if (vertexes[i].x >> FRACBITS > maxx)
	  maxx = vertexes[i].x >> FRACBITS;
      if (vertexes[i].y >> FRACBITS < miny)
	miny = vertexes[i].y >> FRACBITS;
      else
	if (vertexes[i].y >> FRACBITS > maxy)
	  maxy = vertexes[i].y >> FRACBITS;
    }

  // Save blockmap parameters

  bmaporgx = minx << FRACBITS;
  bmaporgy = miny << FRACBITS;
  bmapwidth  = ((maxx-minx) >> MAPBTOFRAC) + 1;
  bmapheight = ((maxy-miny) >> MAPBTOFRAC) + 1;

  // Compute blockmap, which is stored as a 2d array of variable-sized lists.
  //
  // Pseudocode:
  //
  // For each linedef:
  //
  //   Map the starting and ending vertices to blocks.
  //
  //   Starting in the starting vertex's block, do:
  //
  //     Add linedef to current block's list, dynamically resizing it.
  //
  //     If current block is the same as the ending vertex's block, exit loop.
  //
  //     Move to an adjacent block by moving towards the ending block in 
  //     either the x or y direction, to the block which contains the linedef.

  {
    typedef struct { int n, nalloc, *list; } bmap_t;  // blocklist structure
    unsigned tot = bmapwidth * bmapheight;            // size of blockmap
    bmap_t *bmap = calloc(sizeof *bmap, tot);         // array of blocklists

    for (i=0; i < numlines; i++)
      {
	// starting coordinates
	int x = (lines[i].v1->x >> FRACBITS) - minx;
	int y = (lines[i].v1->y >> FRACBITS) - miny;
	
	// x-y deltas
	int adx = lines[i].dx >> FRACBITS, dx = adx < 0 ? -1 : 1;
	int ady = lines[i].dy >> FRACBITS, dy = ady < 0 ? -1 : 1; 

	// difference in preferring to move across y (>0) instead of x (<0)
	int diff = !adx ? 1 : !ady ? -1 :
	  (((x >> MAPBTOFRAC) << MAPBTOFRAC) + 
	   (dx > 0 ? MAPBLOCKUNITS-1 : 0) - x) * (ady = abs(ady)) * dx -
	  (((y >> MAPBTOFRAC) << MAPBTOFRAC) + 
	   (dy > 0 ? MAPBLOCKUNITS-1 : 0) - y) * (adx = abs(adx)) * dy;

	// starting block, and pointer to its blocklist structure
	int b = (y >> MAPBTOFRAC)*bmapwidth + (x >> MAPBTOFRAC);

	// ending block
	int bend = (((lines[i].v2->y >> FRACBITS) - miny) >> MAPBTOFRAC) *
	  bmapwidth + (((lines[i].v2->x >> FRACBITS) - minx) >> MAPBTOFRAC);

	// delta for pointer when moving across y
	dy *= bmapwidth;

	// deltas for diff inside the loop
	adx <<= MAPBTOFRAC;
	ady <<= MAPBTOFRAC;

	// Now we simply iterate block-by-block until we reach the end block.
	while ((unsigned) b < tot)    // failsafe -- should ALWAYS be true
	  {
	    // Increase size of allocated list if necessary
	    if (bmap[b].n >= bmap[b].nalloc)
	      bmap[b].list = realloc(bmap[b].list, 
				     (bmap[b].nalloc = bmap[b].nalloc ? 
				      bmap[b].nalloc*2 : 8)*sizeof*bmap->list);

	    // Add linedef to end of list
	    bmap[b].list[bmap[b].n++] = i;

	    // If we have reached the last block, exit
	    if (b == bend)
	      break;

	    // Move in either the x or y direction to the next block
	    if (diff < 0) 
	      diff += ady, b += dx;
	    else
	      diff -= adx, b += dy;
	  }
      }

    // Compute the total size of the blockmap.
    //
    // Compression of empty blocks is performed by reserving two offset words
    // at tot and tot+1.
    //
    // 4 words, unused if this routine is called, are reserved at the start.

    {
      int count = tot+6;  // we need at least 1 word per block, plus reserved's

      for (i = 0; i < tot; i++)
	if (bmap[i].n)
	  count += bmap[i].n + 2; // 1 header word + 1 trailer word + blocklist

      // Allocate blockmap lump with computed count
      blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);
    }                                                                    

    // Now compress the blockmap.
    {
      int ndx = tot += 4;         // Advance index to start of linedef lists
      bmap_t *bp = bmap;          // Start of uncompressed blockmap

      blockmaplump[ndx++] = 0;    // Store an empty blockmap list at start
      blockmaplump[ndx++] = -1;   // (Used for compression)

      for (i = 4; i < tot; i++, bp++)
	if (bp->n)                                      // Non-empty blocklist
	  {
	    blockmaplump[blockmaplump[i] = ndx++] = 0;  // Store index & header
	    do
	      blockmaplump[ndx++] = bp->list[--bp->n];  // Copy linedef list
	    while (bp->n);
	    blockmaplump[ndx++] = -1;                   // Store trailer
	    free(bp->list);                             // Free linedef list
	  }
	else            // Empty blocklist: point to reserved empty blocklist
	  blockmaplump[i] = tot;

      free(bmap);    // Free uncompressed blockmap
    }
  }
}

//
// P_LoadBlockMap
//
// killough 3/1/98: substantially modified to work
// towards removing blockmap limit (a wad limitation)
//
// killough 3/30/98: Rewritten to remove blockmap limit
//

void P_LoadBlockMap (int lump)
{
  long count;

	// sf: -blockmap checkparm made into variable
	// also checking for levels without blockmaps (0 length)
  if (r_blockmap || W_LumpLength(lump)==0 ||
	(count = W_LumpLength(lump)/2) >= 0x10000)
    P_CreateBlockMap();
  else
    {
      long i;
      short *wadblockmaplump = W_CacheLumpNum (lump, PU_LEVEL);
      blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

      // killough 3/1/98: Expand wad blockmap into larger internal one,
      // by treating all offsets except -1 as unsigned and zero-extending
      // them. This potentially doubles the size of blockmaps allowed,
      // because Doom originally considered the offsets as always signed.

      blockmaplump[0] = (unsigned short)SHORT(wadblockmaplump[0]);
      blockmaplump[1] = (unsigned short)SHORT(wadblockmaplump[1]);
      blockmaplump[2] = (long)(SHORT(wadblockmaplump[2])) & 0xffff;
      blockmaplump[3] = (long)(SHORT(wadblockmaplump[3])) & 0xffff;

      for (i=4 ; i<count ; i++)
	{
          short t = SHORT(wadblockmaplump[i]);          // killough 3/1/98
	  blockmaplump[i] = t == -1 ? -1l : (long) t & 0xffff;
	}

      Z_Free(wadblockmaplump);

      bmaporgx = blockmaplump[0]<<FRACBITS;
      bmaporgy = blockmaplump[1]<<FRACBITS;
      bmapwidth = blockmaplump[2];
      bmapheight = blockmaplump[3];
    }

  // clear out mobj chains
  count = sizeof(*blocklinks)* bmapwidth*bmapheight;
  blocklinks = Z_Malloc (count,PU_LEVEL, 0);
  memset (blocklinks, 0, count);
  blockmap = blockmaplump+4;
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
// killough 5/3/98: reformatted, cleaned up
// killough 8/24/98: rewrote to use faster algorithm

static void AddLineToSector(sector_t *s, line_t *l)
{
  M_AddToBox(s->blockbox, l->v1->x, l->v1->y);
  M_AddToBox(s->blockbox, l->v2->x, l->v2->y);
  *s->lines++ = l;
}

void P_GroupLines (void)
{
  int i, total;
  line_t **linebuffer;

  // look up sector number for each subsector
  for (i=0; i<numsubsectors; i++)
    subsectors[i].sector = segs[subsectors[i].firstline].sidedef->sector;

  // count number of lines in each sector
  for (i=0; i<numlines; i++)
    {
      lines[i].frontsector->linecount++;
      if (lines[i].backsector && lines[i].backsector != lines[i].frontsector)
	lines[i].backsector->linecount++;
    }

  // compute total number of lines and clear bounding boxes
  for (total=0, i=0; i<numsectors; i++)
    {
      total += sectors[i].linecount;
      M_ClearBox(sectors[i].blockbox);
    }

  // build line tables for each sector
  linebuffer = Z_Malloc(total * sizeof(*linebuffer), PU_LEVEL, 0);

  for (i=0; i<numsectors; i++)
    {
      sectors[i].lines = linebuffer;
      linebuffer += sectors[i].linecount;
    }
  
  for (i=0; i<numlines; i++)
    {
      AddLineToSector(lines[i].frontsector, &lines[i]);
      if (lines[i].backsector && lines[i].backsector != lines[i].frontsector)
	AddLineToSector(lines[i].backsector, &lines[i]);
    }

  for (i=0; i<numsectors; i++)
    {
      sector_t *sector = sectors+i;
      int block;

      // adjust pointers to point back to the beginning of each list
      sector->lines -= sector->linecount;

      // set the degenmobj_t to the middle of the bounding box
      sector->soundorg.x = (sector->blockbox[BOXRIGHT] + 
			    sector->blockbox[BOXLEFT])/2;
      sector->soundorg.y = (sector->blockbox[BOXTOP] + 
			    sector->blockbox[BOXBOTTOM])/2;

      // adjust bounding box to map blocks
      block = (sector->blockbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapheight ? bmapheight-1 : block;
      sector->blockbox[BOXTOP]=block;

      block = (sector->blockbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM]=block;

      block = (sector->blockbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapwidth ? bmapwidth-1 : block;
      sector->blockbox[BOXRIGHT]=block;

      block = (sector->blockbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXLEFT]=block;
    }
}

//
// killough 10/98
//
// Remove slime trails.
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coordinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//      2        2                         2        2
//    dx  x0 + dy  x1 + dx dy (y0 - y1)  dy  y0 + dx  y1 + dx dy (x0 - x1)
//   {---------------------------------, ---------------------------------}
//                  2     2                            2     2
//                dx  + dy                           dx  + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

void P_RemoveSlimeTrails(void)                // killough 10/98
{
  byte *hit = calloc(1, numvertexes);         // Hitlist for vertices
  int i;
  for (i=0; i<numsegs; i++)                   // Go through each seg
    {
      const line_t *l = segs[i].linedef;      // The parent linedef
      if (l->dx && l->dy)                     // We can ignore orthogonal lines
	{
	  vertex_t *v = segs[i].v1;
	  do
	    if (!hit[v - vertexes])           // If we haven't processed vertex
	      {
		hit[v - vertexes] = 1;        // Mark this vertex as processed
		if (v != l->v1 && v != l->v2) // Exclude endpoints of linedefs
		  { // Project the vertex back onto the parent linedef
		    Long64 dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
		    Long64 dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
		    Long64 dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
		    Long64 s = dx2 + dy2;
		    int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;
		    v->x = (int)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
		    v->y = (int)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
		  }
	      }  // Obfuscated C contest entry:   :)
	  while ((v != segs[i].v2) && (v = segs[i].v2));
	}
    }
  free(hit);
}

//
// P_CheckLevel
//
// sf 11/9/99
// we need to do this now because we no longer have to
// conform to the MAPxy or ExMy standard previously
// imposed
//

char *levellumps[] =
{
  "label",        // ML_LABEL,    A separator, name, ExMx or MAPxx
  "THINGS",       // ML_THINGS,   Monsters, items..
  "LINEDEFS",     // ML_LINEDEFS, LineDefs, from editing
  "SIDEDEFS",     // ML_SIDEDEFS, SideDefs, from editing
  "VERTEXES",     // ML_VERTEXES, Vertices, edited and BSP splits generated
  "SEGS",         // ML_SEGS,     LineSegs, from LineDefs split by BSP
  "SSECTORS",     // ML_SSECTORS, SubSectors, list of LineSegs
  "NODES",        // ML_NODES,    BSP nodes
  "SECTORS",      // ML_SECTORS,  Sectors, from editing
  "REJECT",       // ML_REJECT,   LUT, sector-sector visibility
  "BLOCKMAP"      // ML_BLOCKMAP  LUT, motion clipping, walls/grid element
};

boolean P_CheckLevel(int lumpnum)
{
        int i, ln;

        for(i=ML_THINGS; i<=ML_BLOCKMAP; i++)
        {
                ln = lumpnum+i;
                if(ln > numlumps ||     // past the last lump
                   strncmp(lumpinfo[ln]->name, levellumps[i], 8) )
                        return false;
        }
        return true;    // all right
}


//
// P_SetupLevel
//
// killough 5/3/98: reformatted, cleaned up

void T_InitSaveList(void); // haleyjd
void P_LoadOlo(void);

//
// P_SetupLevel
//
// killough 5/3/98: reformatted, cleaned up

void P_SetupLevel(char *mapname, int playermask, skill_t skill)
{
  int   i;
  int   lumpnum;

  totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
  wminfo.partime = 180;
  c_showprompt = false;         // kill console prompt as nothing can
                                // be typed at the moment
  for (i=0; i<MAXPLAYERS; i++)
    players[i].killcount = players[i].secretcount = players[i].itemcount = 0;

  // Initial height of PointOfView will be set by player think.
  players[consoleplayer].viewz = 1;

  // haleyjd: ensure that dialogue is stopped by now for sure --
  // all methods of changing levels call this function for sure, and
  // its the last place because of the Z_FreeTags call below -- if
  // a dialogue ran past this point, it could cause memory corruption
  if(currentdialog)
     DLG_Stop();

  if(debugfile)
    {
      fprintf(debugfile, "P_SetupLevel: got here\n mapname: %s\n",mapname);
      fflush(debugfile);
    }

      // get the map name lump number
  if((lumpnum = W_CheckNumForName(mapname)) == -1)
  {
    C_Printf("Map not found: '%s'\n", mapname);
    C_SetConsole();
    return;
  }

  if(!P_CheckLevel(lumpnum))     // not a level
    {
      C_Printf("Not a level: '%s'\n", mapname);
      C_SetConsole();
      return;
    }

  strncpy(levelmapname, mapname, 8);
  leveltime = 0;

  DEBUGMSG("stop sounds\n");

  // Make sure all sounds are stopped before Z_FreeTags. - sf: why?
  S_StopSounds();       // sf: s_start split into s_start, s_stopsounds
			// because of this requirement
		
		// free the old level
  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);

  P_FreeSecNodeList();  // sf: free the psecnode_t linked list in p_map.c
  P_InitThinkers();

#ifdef ARCTIC_STUFF
  Ex_EnsureCorrectArcticPart(gamemap);
#endif

  P_LoadOlo();                          // level names etc
  P_LoadLevelInfo (lumpnum);    // load level lump info(level name etc)

  if(!strcmp(info_colormap, "COLORMAP"))  // haleyjd
      MapUseFullBright = true;
  else
      MapUseFullBright = false;

  if(strcmp(info_sky2name, "-")) // haleyjd 11/14/00
     DoubleSky = true;
  else
     DoubleSky = false;

  WI_StopCamera();      // reset the intermissions camera

// haleyjd: changed from if(0) to #if 0
#if 0
  // when loading a hub level, display a 'loading' box
  if(hub_changelevel)
    V_SetLoading(4, "loading");
#endif

  DEBUGMSG("hu_newlevel\n");
  newlevel = !W_IsLumpFromIWAD(lumpnum);
  doom1level = false;
  HU_NewLevel();
  HU_Start();

  // must be after p_loadlevelinfo as the music lump name is got there
  S_Start();

  DEBUGMSG("P_SetupLevel: loaded level info\n");
  
	// load the sky
  R_StartSky();

  DEBUGMSG("P_SetupLevel: sky done\n");

  // note: most of this ordering is important

  // killough 3/1/98: P_LoadBlockMap call moved down to below
  // killough 4/4/98: split load of sidedefs into two parts,
  // to allow texture names to be used in special linedefs

  level_error = false;  // reset

  P_LoadVertexes  (lumpnum+ML_VERTEXES);
  P_LoadSectors   (lumpnum+ML_SECTORS);
  P_LoadSideDefs  (lumpnum+ML_SIDEDEFS);             // killough 4/4/98
  P_LoadLineDefs  (lumpnum+ML_LINEDEFS);             //       |

#if 0
  V_LoadingIncrease();  // update
#endif

  P_LoadSideDefs2 (lumpnum+ML_SIDEDEFS);             //       |
  P_LoadLineDefs2 (lumpnum+ML_LINEDEFS);             // killough 4/4/98

  if(level_error)       // drop to the console
  {             
    C_SetConsole();
    C_Puts(FC_GOLD "Unrecoverable error while loading map\n" FC_RED);
    return;
  }

  numnodes = numsegs = numsubsectors = 0;
  P_LoadExtended  (lumpnum+ML_NODES);  
  P_LoadBlockMap  (lumpnum+ML_BLOCKMAP);             // killough 3/1/98
  if (!numsegs)
  {
    P_LoadSegs(lumpnum+ML_SEGS);
    P_ValidateSegs();
  }
  if (!numsegs)
  {
    // should these go hand by hand?
    P_LoadSegs32(lumpnum+ML_SEGS);
    P_ValidateSegs();
    P_LoadSubsectors32(lumpnum+ML_SSECTORS);
    P_ValidateSubsectors();
  }
  if (!numsubsectors) 
  {
    P_LoadSubsectors(lumpnum+ML_SSECTORS);
    P_ValidateSubsectors();
  }
  if (!numsubsectors) 
  {
    P_LoadSubsectors32(lumpnum+ML_SSECTORS);
    P_ValidateSubsectors();
  }

  if (!numnodes) P_LoadNodes(lumpnum+ML_NODES);
  if (!numnodes) P_LoadNodes32(lumpnum+ML_NODES);

  level_error = !numsubsectors || !numsegs || !numnodes;
  
  if(level_error)       // drop to the console
  {             
    C_SetConsole();
    C_Puts(FC_GOLD "Unrecoverable error while loading map" FC_RED);
    return;
  }

  DEBUGMSG("loaded level\n");

#if 0
  V_LoadingIncrease();    // update
#endif

  rejectmatrix = W_CacheLumpNum(lumpnum+ML_REJECT,PU_LEVEL);
  P_GroupLines();

  if(remove_slime_trails) {
    DEBUGMSG("remove slime trails from wad\n");
    P_RemoveSlimeTrails();    // killough 10/98: remove slime trails from wad
  }

  // Note: you don't need to clear player queue slots --
  // a much simpler fix is in g_game.c -- killough 10/98

  bodyqueslot = 0;
  deathmatch_p = deathmatchstarts;

  P_LoadThings(lumpnum+ML_THINGS);

  DEBUGMSG("ok, things loaded, spawn players\n");

  // if deathmatch, randomly spawn the active players
  if (deathmatch)
    for (i=0; i<MAXPLAYERS; i++)
      if (playeringame[i])
	{
	  players[i].mo = NULL;
	  G_DeathMatchSpawnPlayer(i);
	}

  DEBUGMSG("done\n");

  // killough 3/26/98: Spawn icon landings:
  if (gamemode==commercial)
    P_SpawnBrainTargets();

  // haleyjd: init miscellaneous spots
  P_InitMiscMapSpots();
  T_BuildGameArrays(); // build corresponding arrays for FS
  T_InitSaveList();    // init save list for arrays

  // clear special respawning que
  iquehead = iquetail = 0;

  // set up world state
  P_SpawnSpecials();

#if 0
  V_LoadingIncrease();      // update
#endif

  // haleyjd
  P_InitLightning();
  
  // preload graphics
  if (precache)
    R_PrecacheLevel();

  // psprites
  HU_FragsUpdate();     // reset frag counter
  HU_ReInitFSPicList(); // haleyjd: reset FSPic list globals

  R_SetViewSize (screenSize+3); //sf

  T_PreprocessScripts();        // preprocess FraggleScript scripts

#if 0
  V_LoadingIncrease();
#endif

  DEBUGMSG("P_SetupLevel: finished\n");
  if(doom1level && gamemode == commercial)
          C_Printf("doom 1 level\n");

  camera = NULL;        // camera off

  

  if(P_ShouldRandomizeMusic(lumpnum) && random_mus_num >= 0)          
    {
      const char * mus = S_ChangeToPreselectedMusic(random_mus_num);
      if(mus)
        player_printf(&players[consoleplayer], "%cNow playing %c%s",
           128+mess_colour, 128 + CR_GOLD, mus);      
    }
  random_mus_num = -1;
}

void P_InitEternityVars(void);

//
// P_Init
//
void P_Init (void)
{
   P_InitParticleEffects(); // haleyjd 09/30/01
   P_InitSwitchList();
   P_InitPicAnims();
   R_InitSprites(spritelist);
   P_InitHubs();
   P_LoadTerrainTypeDefs(); // haleyjd 11/20/00
   P_InitTerrainTypes(); // haleyjd 07/03/99
   P_InitEternityVars(); // haleyjd 09/18/99
}

void P_Free(void)
{
   R_FreeSprites();
}

//
// OLO Support. - sf
//
// OLOs were something I came up with a while ago when making my 'onslaunch'
// launcher. They are lumps which hold information about the lump: which
// deathmatch type they are best played on etc. which was read by onslaunch
// which adjusted its launch settings appropriately. More importantly,
// they hold the level names which I use here..
//

olo_t olo;
int olo_loaded = false;

void P_LoadOlo()
{
  int lumpnum;
  char *lump;

  if((lumpnum = W_CheckNumForName("OLO")) == -1)
    return;

  lump = W_CacheLumpNum(lumpnum, PU_CACHE);
  
  if(strncmp(lump, "OLO", 3))
    return;
  
  memcpy(&olo, lump, sizeof(olo_t));
  
  olo_loaded = true;
}

// test thingy

void C_DumpThings()
{
  int i;
  
  for(i=0; i<numthings; i++)
    {
      C_Printf("%i\n", spawnedthings[i]);
    }
  
  C_Printf(FC_GRAY"(%i)\n", numthings);
}

//===============================================
//
// P_InitEternityVars
//
// This function tests for the ETERNITY lump
// which acts as a switch for turning on new
// weapons, interludes, finales, etc.
// State of presence is written to EternityMode
//
//===============================================
void P_InitEternityVars(void)
{
   if(W_CheckNumForName("ETERNITY") != -1)
     {
       EternityMode = true;
       usermsg("Eternity TC Mode activated");
     }
   else
     EternityMode = false;
}


//===============================================
//
// P_ApplyPersistentOptions
//
// For options that get saved in savegames as variables 
// only override them if they have not been saved yet
//
//===============================================

void P_ApplyPersistentOptions(void) 
{
    int bloodvar, wolfvar, contvar, bjvar, musloopvar;

    hint_bloodcolor = "";
    bloodvar = T_GetGlobalIntVar("_private_bloodcolor", -1);
    if(bloodvar < 0 && info_nobloodcolor)
      {
        R_RefreshTranslationTables(0);
        prtclblood = 0;
        hint_bloodcolor = "Using Original from map defaults.\nToggle to override.";
      }
    else
      {
        R_RefreshTranslationTables(bloodcolor);
        prtclblood = bloodcolor;
      }
    if(bloodvar >= 0)
      {
        T_EnsureGlobalIntVar("_private_bloodcolor", bloodvar);
      }


    hint_wolfendoom = "";
    wolfvar = T_GetGlobalIntVar("_private_wolfendoom", -1);
    if(wolfvar < 0 && info_wolf3d)
      {
        wolf3dmode = 1;
        hint_wolfendoom = "Using On from map defaults.\nToggle to override.";
      }
    else 
      {
        wolf3dmode = wolfendoom;
      }
    if(wolfvar >= 0)
      {
        T_EnsureGlobalIntVar("_private_wolfendoom", wolfvar);
      }
  
    hint_bjskin = "";
    bjvar = T_GetGlobalIntVar("_private_bjskin", -1);
    if(bjvar < 0 && wolf3dmode)
      {
        if(Ex_SetWolfendoomSkin())
          hint_bjskin = "Using BJ sprite from map defaults.\nToggle to override.";
      }
    if(bjvar >= 0)
      {
        T_EnsureGlobalIntVar("_private_bjskin", bjvar);
      }


    contvar = T_GetGlobalIntVar("_private_continue", -1);
    if(contvar >= 0)
      {
        T_EnsureGlobalIntVar("_private_continue", contvar);
      }


    musloopvar = T_GetGlobalIntVar("_private_continuous_music", -1);
    if(musloopvar >= 0)
      {
        T_EnsureGlobalIntVar("_private_continuous_music", musloopvar);
      }
    if(musloopvar > 0)
      {
        loop_music = 0;
      }

}

//----------------------------------------------------------------------------
//
// $Log: p_setup.c,v $
// Revision 1.16  1998/05/07  00:56:49  killough
// Ignore translucency lumps that are not exactly 64K long
//
// Revision 1.15  1998/05/03  23:04:01  killough
// beautification
//
// Revision 1.14  1998/04/12  02:06:46  killough
// Improve 242 colomap handling, add translucent walls
//
// Revision 1.13  1998/04/06  04:47:05  killough
// Add support for overloading sidedefs for special uses
//
// Revision 1.12  1998/03/31  10:40:42  killough
// Remove blockmap limit
//
// Revision 1.11  1998/03/28  18:02:51  killough
// Fix boss spawner savegame crash bug
//
// Revision 1.10  1998/03/20  00:30:17  phares
// Changed friction to linedef control
//
// Revision 1.9  1998/03/16  12:35:36  killough
// Default floor light level is sector's
//
// Revision 1.8  1998/03/09  07:21:48  killough
// Remove use of FP for point/line queries and add new sector fields
//
// Revision 1.7  1998/03/02  11:46:10  killough
// Double blockmap limit, prepare for when it's unlimited
//
// Revision 1.6  1998/02/27  11:51:05  jim
// Fixes for stairs
//
// Revision 1.5  1998/02/17  22:58:35  jim
// Fixed bug of vanishinb secret sectors in automap
//
// Revision 1.4  1998/02/02  13:38:48  killough
// Comment out obsolete reload hack
//
// Revision 1.3  1998/01/26  19:24:22  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:02:21  killough
// Generalize and simplify level name generation
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
