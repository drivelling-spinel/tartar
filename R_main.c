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
//      Rendering main loop and setup functions,
//       utility functions (BSP, geometry, trigonometry).
//      See tables.c, too.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: r_main.c,v 1.13 1998/05/07 00:47:52 killough Exp $";

#include "doomstat.h"
#include "i_video.h"
#include "c_runcmd.h"
#include "g_game.h"
#include "mn_engin.h" 
#include "r_main.h"
#include "r_things.h"
#include "r_plane.h"
#include "r_ripple.h"
#include "r_bsp.h"
#include "r_draw.h"
#include "m_bbox.h"
#include "r_sky.h"
#include "s_sound.h"
#include "v_video.h"
#include "ex_stuff.h"
#include "c_io.h"
#include "hu_stuff.h"

// Fineangles in the SCREENWIDTH wide window.
int fov=2048;   //sf: made an int from a #define

// killough: viewangleoffset is a legacy from the pre-v1.2 days, when Doom
// had Left/Mid/Right viewing. +/-ANG90 offsets were placed here on each
// node, by d_net.c, to set up a L/M/R session.

int viewdir;    // 0 = forward, 1 = left, 2 = right
int viewangleoffset;
int validcount = 1;         // increment every time a check is made
lighttable_t *fixedcolormap;
int      centerx, centery;
fixed_t  centerxfrac, centeryfrac;
fixed_t  projection;
fixed_t  viewx, viewy, viewz;
angle_t  viewangle;
fixed_t  viewcos, viewsin;
player_t *viewplayer;
extern lighttable_t **walllights;
boolean  showpsprites=1; //sf
camera_t *viewcamera;
int zoom = 1;   // sf: fov/zooming

extern int screenSize;

extern int global_cmap_index; // haleyjd: NGCS

void R_HOMdrawer();

#ifdef NORENDER
int norender1;
int norender2 = -1;
int norender3;
int norender4;
int norender5;
int norender6;
int norender7;
int norender8;
int norender9;
int norender0;
int debugcolumn;
#endif


boolean wigglefix;


//
// precalculated math tables
//

angle_t clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X. 

int viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.

angle_t xtoviewangle[MAX_SCREENWIDTH+1];   // killough 2/8/98

// killough 3/20/98: Support dynamic colormaps, e.g. deep water
// killough 4/4/98: support dynamic number of them as well

int numcolormaps;
lighttable_t *(*c_scalelight)[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];
lighttable_t *(*scalelight)[MAXLIGHTSCALE];
lighttable_t *(*zlight)[MAXLIGHTZ];
lighttable_t *fullcolormap;
lighttable_t **colormaps;

// killough 3/20/98, 4/4/98: end dynamic colormaps

int extralight;                           // bumped light from gun blasts

void (*colfunc)(void) = R_DrawColumn;     // current column draw function

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//

int R_PointOnSide(fixed_t x, fixed_t y, node_t *node)
{
  if (!node->dx)
    return x <= node->x ? node->dy > 0 : node->dy < 0;

  if (!node->dy)
    return y <= node->y ? node->dx < 0 : node->dx > 0;
        
  x -= node->x;
  y -= node->y;
  
  // Try to quickly decide by looking at sign bits.
  if ((node->dy ^ node->dx ^ x ^ y) < 0)
    return (node->dy ^ x) < 0;  // (left is negative)
  return FixedMul(y, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, x);
}

// killough 5/2/98: reformatted

int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
  fixed_t lx = line->v1->x;
  fixed_t ly = line->v1->y;
  fixed_t ldx = line->v2->x - lx;
  fixed_t ldy = line->v2->y - ly;

  if (!ldx)
    return x <= lx ? ldy > 0 : ldy < 0;

  if (!ldy)
    return y <= ly ? ldx < 0 : ldx > 0;
  
  x -= lx;
  y -= ly;
        
  // Try to quickly decide by looking at sign bits.
  if ((ldy ^ ldx ^ x ^ y) < 0)
    return (ldy ^ x) < 0;          // (left is negative)
  return FixedMul(y, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, x);
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{       
  return (y -= viewy, (x -= viewx) || y) ?
    x >= 0 ?
      y >= 0 ? 
        (x > y) ? tantoangle[SlopeDiv2(y,x)] :                      // octant 0 
                ANG90-1-tantoangle[SlopeDiv2(x,y)] :                // octant 1
        x > (y = -y) ? -tantoangle[SlopeDiv2(y,x)] :                // octant 8
                       ANG270+tantoangle[SlopeDiv2(x,y)] :          // octant 7
      y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv2(y,x)] : // octant 3
                            ANG90 + tantoangle[SlopeDiv2(x,y)] :    // octant 2
        (x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv2(y,x)] :  // octant 4
                              ANG270-1-tantoangle[SlopeDiv2(x,y)] : // octant 5
    0;
}

angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{       
  return (y -= viewy, (x -= viewx) || y) ?
    x >= 0 ?
      y >= 0 ? 
        (x > y) ? tantoangle[SlopeDiv2(y,x)] :                      // octant 0 
                ANG90-1-tantoangle[SlopeDiv2(x,y)] :                // octant 1
        x > (y = -y) ? -tantoangle[SlopeDiv2(y,x)] :                // octant 8
                       ANG270+tantoangle[SlopeDiv2(x,y)] :          // octant 7
      y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv2(y,x)] : // octant 3
                            ANG90 + tantoangle[SlopeDiv2(x,y)] :    // octant 2
        (x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv2(y,x)] :  // octant 4
                              ANG270-1-tantoangle[SlopeDiv2(x,y)] : // octant 5
    0;
}


//
// R_InitTextureMapping
//
// killough 5/2/98: reformatted

static void R_InitTextureMapping (void)
{
  register int i,x;
  fixed_t focallength;
    
  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  //
  // Calc focallength
  //  so fov angles covers SCREENWIDTH.

                        // sf: zooming
  focallength = FixedDiv(centerxfrac*zoom, finetangent[FINEANGLES/4+fov/2]);
        
  for (i=0 ; i<FINEANGLES/2 ; i++)
    {
      int t;
      if (finetangent[i] > FRACUNIT*2)
        t = -1;
      else
        if (finetangent[i] < -FRACUNIT*2)
          t = viewwidth+1;
      else
        {
          t = FixedMul(finetangent[i], focallength);
          t = (centerxfrac - t + FRACUNIT-1) >> FRACBITS;
          if (t < -1)
            t = -1;
          else
            if (t > viewwidth+1)
              t = viewwidth+1;
        }
      viewangletox[i] = t;
    }
    
  // Scan viewangletox[] to generate xtoviewangle[]:
  //  xtoviewangle will give the smallest view angle
  //  that maps to x.

  for (x=0; x<=viewwidth; x++)
    {
      for (i=0; viewangletox[i] > x; i++)
        ;
      xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }
    
  // Take out the fencepost cases from viewangletox.
  for (i=0; i<FINEANGLES/2; i++)
    if (viewangletox[i] == -1)
      viewangletox[i] = 0;
    else 
      if (viewangletox[i] == viewwidth+1)
        viewangletox[i] = viewwidth;
        
  clipangle = xtoviewangle[0];
}

//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//

#define DISTMAP 2

void R_InitLightTables (void)
{
  int i;

  // killough 4/4/98: dynamic colormaps

  c_zlight = malloc(sizeof(*c_zlight) * numcolormaps);
  c_scalelight = malloc(sizeof(*c_scalelight) * numcolormaps);

  // Calculate the light levels to use
  //  for each level / distance combination.
  for (i=0; i< LIGHTLEVELS; i++)
    {
      int j, startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0; j<MAXLIGHTZ; j++)
        {
          int scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
          int t, level = startmap - (scale >>= LIGHTSCALESHIFT)/DISTMAP;

          if (level < 0)
            level = 0;
          else
            if (level >= NUMCOLORMAPS)
              level = NUMCOLORMAPS-1;

          // killough 3/20/98: Initialize multiple colormaps
          level *= 256;
          for (t=0; t<numcolormaps; t++)         // killough 4/4/98
            c_zlight[t][i][j] = colormaps[t] + level;
        }
    }
}

void R_ReInitLightTables (void)
{
  int i;

  // Calculate the light levels to use
  //  for each level / distance combination.
  for (i=0; i< LIGHTLEVELS; i++)
    {
      int j, startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0; j<MAXLIGHTZ; j++)
        {
          int scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
          int t, level = startmap - (scale >>= LIGHTSCALESHIFT)/DISTMAP;

          if (level < 0)
            level = 0;
          else
            if (level >= NUMCOLORMAPS)
              level = NUMCOLORMAPS-1;

          // killough 3/20/98: Initialize multiple colormaps
          level *= 256;
          c_zlight[0][i][j] = colormaps[0] + level;
        }
    }
}


//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//

boolean setsizeneeded;
int     setblocks;

void R_SetViewSize(int blocks)
{
  setsizeneeded = true;
  setblocks = blocks;
}

//
// R_ExecuteSetViewSize
//

void R_ExecuteSetViewSize (void)
{
  int i;

  setsizeneeded = false;

  if (setblocks == 11)
    {
      scaledviewwidth = SCREENWIDTH;
      scaledviewheight = SCREENHEIGHT;                   // killough 11/98
    }
  else if (setblocks == 12)
    {
      scaledviewwidth = SCREENWIDTH;
      scaledviewheight = EFFECTIVE_HEIGHT - ST_HEIGHT;   // killough 11/98
    }
  else
    {
      scaledviewwidth = setblocks*32;
      scaledviewheight = (setblocks*168/10) & ~7;        // killough 11/98
    }

  viewwidth = scaledviewwidth << hires;                  // killough 11/98
  viewheight = scaledviewheight << hires;                // killough 11/98

  centerx = viewwidth/2;
  centery = viewheight/2;
  centerxfrac = centerx<<FRACBITS;
  centeryfrac = centery<<FRACBITS;
  projection = centerxfrac * zoom;      // sf: zooming

  R_InitBuffer(scaledviewwidth, scaledviewheight);       // killough 11/98
        
  R_InitTextureMapping();
    
  // psprite scales
                                // sf: zooming added
  pspritescale = FixedDiv(zoom*viewwidth, SCREENWIDTH);       // killough 11/98
  pspriteiscale = FixedDiv(SCREENWIDTH, zoom*viewwidth);       // killough 11/98
    
  // thing clipping
  for (i=0 ; i<viewwidth ; i++)
  {
    screenheightarray[i] = viewheight;
  }

  // planes
  for (i=0 ; i<viewheight*2 ; i++)
    {
      fixed_t dy = abs(((i-viewheight)<<FRACBITS)+FRACUNIT/2);
                // sf: zooming
      origyslope[i] = FixedDiv(viewwidth*zoom*(FRACUNIT/2), dy);
    }
   yslope = origyslope + (viewheight/2);
        
  for (i=0 ; i<viewwidth ; i++)
    {
      fixed_t cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
      distscale[i] = FixedDiv(FRACUNIT,cosadj);
    }
    
  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0; i<LIGHTLEVELS; i++)
    {
      int j, startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {                                       // killough 11/98:
          int t, level = startmap - j*SCREENWIDTH/scaledviewwidth/DISTMAP;
            
          if (level < 0)
            level = 0;

          if (level >= NUMCOLORMAPS)
            level = NUMCOLORMAPS-1;

          // killough 3/20/98: initialize multiple colormaps
          level *= 256;

          for (t=0; t<numcolormaps; t++)     // killough 4/4/98
            c_scalelight[t][i][j] = colormaps[t] + level;
        }
    }
}


void R_Free(void)
{
  int i;

  R_FreeData();

  Z_Free(translationtables);
  translationtables = 0;

  Z_Free(c_scalelight);
  c_scalelight = 0;
  Z_Free(c_zlight);
  c_zlight = 0;
  scalelight = 0;
  zlight = 0;
  fullcolormap = 0;
  
  R_FreeParticles();
}

//
// R_Init
//

void R_Init (void)
{
  R_InitData();
  R_SetViewSize(screenSize+3);
  R_InitPlanes();
  R_InitLightTables();
  R_InitSkyMap();
  R_InitTranslationTables();
  R_InitParticles(); // haleyjd
}

//
// R_PointInSubsector
//
// killough 5/2/98: reformatted, cleaned up

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
  int nodenum = numnodes-1;
  while (!(nodenum & NFX_SUBSECTOR))
    nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];
  return &subsectors[nodenum & ~NFX_SUBSECTOR];
}

int autodetect_hom = 0;       // killough 2/7/98: HOM autodetection flag
long updownangle = 0;

//
// R_SetupFrame
//

void R_SetupFrame (player_t *player, camera_t *camera)
{               
  mobj_t *mobj;
  static int oldzoom;

  // check for change to zoom
  if(zoom != oldzoom)
  {
        R_ExecuteSetViewSize(); // reset view
        oldzoom = zoom;
  }

  viewplayer = player;
  mobj = player->mo;

  viewcamera = camera;
  if(!camera)
  {
          viewx = mobj->x;
          viewy = mobj->y;
          viewz = player->viewz;
          viewangle = mobj->angle;// + viewangleoffset;
                 // y shearing
          updownangle = player->updownangle;
  }
  else
  {
          viewx = camera->x;
          viewy = camera->y;
          viewz = camera->z;
          viewangle = camera->angle;
          updownangle = camera->updownangle;
  }

  extralight = player->extralight;
  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

        // y shearing
  updownangle *= zoom;          // sf: zooming
                // check for limits
  updownangle = updownangle >  50 ?  50 :
                updownangle < -50 ? -50 : updownangle;
        // scale to screen size
  updownangle = (updownangle * viewheight) / 100;

  centery = (viewheight/2) + updownangle;
  centeryfrac = (centery<<FRACBITS);
  yslope = origyslope + (viewheight>>1) - updownangle;

        // use drawcolumn
  colfunc = hires == 2 ? R_DrawColumn2 : R_DrawColumn; //sf

  validcount++;
}

//

typedef enum
{
        area_normal,
        area_below,
        area_above
} area_t;

void R_SectorColormap(sector_t *s)
{
  int cm;
  // killough 3/20/98, 4/4/98: select colormap based on player status

  // haleyjd: NGCS
  //if(s->heightsec == -1) cm = 0;
  if(s->heightsec == -1) cm = global_cmap_index;
  else
  {
     sector_t *viewsector;
     area_t viewarea;
     viewsector = R_PointInSubsector(viewx,viewy)->sector;

             // find which area the viewpoint (player) is in
     viewarea =
        viewsector->heightsec == -1 ? area_normal :
        viewz < sectors[viewsector->heightsec].floorheight ? area_below :
        viewz > sectors[viewsector->heightsec].ceilingheight ? area_above :
        area_normal;

     s = s->heightsec + sectors;

     cm = viewarea==area_normal ? s->midmap :
           viewarea==area_above ? s->topmap : s->bottommap;

     // haleyjd: NGCS -- sector colormaps may be 0
     if(!cm) cm = global_cmap_index;
  }

  fullcolormap = colormaps[cm];
  zlight = c_zlight[cm];
  scalelight = c_scalelight[cm];

  if (viewplayer->fixedcolormap)
    {
      int i;
      // killough 3/20/98: localize scalelightfixed (readability/optimization)
      static lighttable_t *scalelightfixed[MAXLIGHTSCALE];

      fixedcolormap = fullcolormap   // killough 3/20/98: use fullcolormap
        + viewplayer->fixedcolormap*256*sizeof(lighttable_t);
        
      walllights = scalelightfixed;

      for (i=0 ; i<MAXLIGHTSCALE ; i++)
        scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

}

angle_t R_WadToAngle(int wadangle)
{
  if(demo_version < 302)            // maintain compatibility
    return (wadangle / 45) * ANG45;

  return wadangle * (ANG45 / 45);     // allows wads to specify angles to
                                      // the nearest degree, not nearest 45
}

static int render_ticker = 0;
int flatskip = 0;
boolean abort_render = false;

//
// R_RenderView
//
void R_RenderPlayerView (player_t* player, camera_t *camerapoint)
{       
  R_SetupFrame (player, camerapoint);

  // Clear buffers.
  R_ClearClipSegs ();
  R_ClearDrawSegs ();
  R_ClearPlanes ();
  R_ClearSprites ();

  R_ResetHeightMath();
    
  if (autodetect_hom)
        R_HOMdrawer();

  // check for new console commands.
  NetUpdate ();
  if(abort_render)
    {
      abort_render = false;
      return;
    }

  // The head node is the last node output.
  R_RenderBSPNode (numnodes-1);
    
  // Check for new console commands.
  NetUpdate ();
  if(abort_render)
    {
      abort_render = false;
      return;
    }
     
  if(!flatskip || render_ticker % flatskip) R_DrawPlanes ();
    
  // Check for new console commands.
  NetUpdate ();
  if(abort_render)
    {
      abort_render = false;
      return;
    }
    
  R_DrawMasked ();

  // Check for new console commands.
  NetUpdate ();

  render_ticker++;
#ifdef NORENDER
  debugcolumn = false;
#endif
}

        // sf: rewritten

void R_HOMdrawer()
{
  int y, colour;
  char *dest;

  colour = !flashing_hom || (gametic % 20) < 9 ? 0xb0 : 0;
  dest = screens[0] + viewwindowy*(SCREENWIDTH<<hires) + viewwindowx;

  for(y=viewwindowy; y<viewwindowy+viewheight; y++)
  {
     memset(dest, colour, viewwidth);
     dest += SCREENWIDTH<<hires;
  }

//      if (gametic-lastshottic < TICRATE*2 && gametic-lastshottic > TICRATE/8);
}


void R_ResetTrans()
{
  if (general_translucency
#ifdef FAUXTRAN
  && !faux_translucency
#endif
  )
    R_ReInitTranMap(0);
}

int reinitneeded;

void R_ReInitIfNeeded(void)
{
  int was_needed = reinitneeded;
  reinitneeded = false;

  if(was_needed)
    {
      R_ReInitColormaps2();
      R_ReInitLightTables();
      R_SetViewSize(screenSize+3);
      R_ResetTrans();
      P_InitParticleEffects();
      I_ResetPalette();
    }
}

//
//  Console Commands
//

char *handedstr[]       = {"right", "left"};

VARIABLE_BOOLEAN(lefthanded, NULL,                  handedstr);
VARIABLE_BOOLEAN(r_blockmap, NULL,                  onoff);
VARIABLE_INT(flatskip, NULL,                    0, 100, NULL);
VARIABLE_BOOLEAN(flashing_hom, NULL,                onoff);
VARIABLE_BOOLEAN(visplane_view, NULL,               onoff);
VARIABLE_BOOLEAN(r_precache, NULL,                  onoff);
VARIABLE_BOOLEAN(showpsprites, NULL,                yesno);
VARIABLE_BOOLEAN(stretchsky, NULL,                  onoff);
VARIABLE_BOOLEAN(r_swirl, NULL,                     onoff);
VARIABLE_BOOLEAN(general_translucency, NULL,        onoff);
VARIABLE_BOOLEAN(faux_translucency, NULL,           yesno);
VARIABLE_BOOLEAN(water_translucency, NULL,           onoff);
VARIABLE_INT(tran_filter_pct, NULL,             0, 100, NULL);
VARIABLE_BOOLEAN(autodetect_hom, NULL,              yesno);
VARIABLE_INT(screenSize, NULL,                  0, 9, NULL);
VARIABLE_INT(zoom, NULL,                        0, 8192, NULL);
VARIABLE_INT(usegamma, NULL,                    0, 4, NULL);

VARIABLE_INT(playpal_wad, NULL, 0, MAXINT, NULL);

CONSOLE_COMMAND(pal_next, 0)
{
    int next = playpal_wad + 1;
    char * msg = 0;
    if (playpal_wads_count <= 1) return;

    if (next < 0 || next >= playpal_wads_count)
      {
        next = 0;    
      }

    playpal_wad = next;
    reinitneeded = true;
    msg = calloc(1, 20 + strlen(dyna_playpal_wads[playpal_wad]));
    sprintf(msg, "%s Palette", dyna_playpal_wads[playpal_wad]);
    HU_PlayerMsg(msg);
    free(msg);
}

CONSOLE_COMMAND(pal_prev, 0)
{
    int next = playpal_wad - 1;
    char *msg = 0;
    if (playpal_wads_count <= 1) return;

    if (next < 0 || next >= playpal_wads_count)
      {
        next = playpal_wads_count - 1;    
      }

    playpal_wad = next;
    reinitneeded = true;
    msg = calloc(1, 20 + strlen(dyna_playpal_wads[playpal_wad]));
    sprintf(msg, "%s Palette", dyna_playpal_wads[playpal_wad]);
    HU_PlayerMsg(msg);
    free(msg);
}

CONSOLE_VARIABLE(pal_curr, playpal_wad, 0)
{
    int next = playpal_wad;
    char * msg = 0;

    if (playpal_wads_count <= 1)
      {
        playpal_wad = 0;
        return;
      }

    if(next < 0 || next >= playpal_wads_count) {
        next = default_playpal_wad;
    }
    if(next < 0) next = 0;

    playpal_wad = next;
    reinitneeded = true;
    msg = calloc(1, 20 + strlen(dyna_playpal_wads[playpal_wad]));
    sprintf(msg, "%s Palette", dyna_playpal_wads[playpal_wad]);
    HU_PlayerMsg(msg);
    free(msg);
}

extern int * dyna_lump_nums[];


CONSOLE_COMMAND(pal_list, 0)
{
    int i;

    C_Printf("PLAYPALs loaded:\n");
    for(i = 0 ; i < playpal_wads_count ; i += 1)
      {
        C_Printf("%s %d:%s", i == playpal_wad ? "*" :
          i == default_playpal_wad ? "@" : "  ", i, dyna_playpal_wads[i]);
        if(c_argc)
          {
            int j;
            for(j = 0 ; j < DYNA_TOTAL ; j += 1)
              C_Printf(" %d", dyna_lump_nums[j][i]);
          }
        C_Printf("\n");
      }
}


CONSOLE_VARIABLE(gamma, usegamma, 0)
{
    if(gammastyle)
      if(usegamma == 3) usegamma = 0;
      else if(usegamma) usegamma = 4;
        // change to new gamma val
    I_ResetPalette ();
}
CONSOLE_VARIABLE(lefthanded, lefthanded, 0) {}
CONSOLE_VARIABLE(r_blockmap, r_blockmap, 0) {}
// CONSOLE_VARIABLE(r_flatskip, flatskip, 0) {}
CONSOLE_VARIABLE(r_homflash, flashing_hom, 0) {}
CONSOLE_VARIABLE(r_planeview, visplane_view, 0) {}
CONSOLE_VARIABLE(r_zoom, zoom, 0) {}
CONSOLE_VARIABLE(r_precache, r_precache, 0) {}
CONSOLE_VARIABLE(r_showgun, showpsprites, 0) {}
CONSOLE_VARIABLE(r_showhom, autodetect_hom, 0)
{
        doom_printf("hom detection %s", autodetect_hom ? "on" : "off");
}
CONSOLE_VARIABLE(r_stretchsky, stretchsky, 0) {}
CONSOLE_VARIABLE(r_swirl, r_swirl, 0) {}
CONSOLE_VARIABLE(r_fauxtrans, faux_translucency, 0)
{
  R_ResetTrans();
}
CONSOLE_VARIABLE(r_watertrans, water_translucency, 0)
{
  R_ResetTrans();
}
CONSOLE_VARIABLE(r_trans, general_translucency, 0)
{
  R_ResetTrans();
}
CONSOLE_VARIABLE(r_tranpct, tran_filter_pct, 0)
{
  R_ResetTrans();
}

void HU_ToggleHUD(void);

CONSOLE_VARIABLE(screensize, screenSize, cf_buffered)
{
  S_StartSound(NULL,sfx_stnmov);

  if(screenSize >= 8)
  {
     clearscreen = 1;
     ST_Reposition();
     AM_LevelInit();
  }

  if(gamestate == GS_LEVEL) // not in intercam
  {
     hide_menu = 20;             // hide the menu for a few tics
     R_SetViewSize (screenSize + 3);
  }
}

CONSOLE_COMMAND(p_dumphubs, 0)
{
  extern void P_DumpHubs();
  P_DumpHubs();
}


VARIABLE_BOOLEAN(remove_slime_trails, NULL, onoff);

CONSOLE_VARIABLE(p_rmslime, remove_slime_trails, 0)
{
  C_Printf("p_rmslime change will take effect on next level start");
}

#ifdef NORENDER
VARIABLE_BOOLEAN(norender1, NULL, onoff);
VARIABLE_INT(norender2, NULL, -1, 1280, NULL);
VARIABLE_BOOLEAN(norender3, NULL, onoff);
VARIABLE_BOOLEAN(norender4, NULL, onoff);
VARIABLE_BOOLEAN(norender5, NULL, onoff);
VARIABLE_BOOLEAN(norender6, NULL, onoff);
VARIABLE_BOOLEAN(norender7, NULL, onoff);
VARIABLE_BOOLEAN(norender8, NULL, onoff);
VARIABLE_BOOLEAN(norender9, NULL, onoff);
VARIABLE_BOOLEAN(norender0, NULL, onoff);

VARIABLE_BOOLEAN(debugcolumn, NULL, onoff);
CONSOLE_VARIABLE(debugcolumn, debugcolumn, 0) {}


CONSOLE_VARIABLE(r_norender1, norender1, 0) {
  norender1 = norenderparm && norender1;
  if(norenderparm)
    HU_PlayerMsg(norender1 ? "Sky rendering off" : "Sky rendering on");
}
CONSOLE_VARIABLE(r_norender2, norender2, 0) {
  char msg[32];
  norender2 = norender2 % 1280;
  if (!norenderparm)
    {
      norender2 = -1;
      return;
    }
  sprintf(msg, "Rendering column %d", norender2);
  HU_PlayerMsg(msg);
}
CONSOLE_VARIABLE(r_norender3, norender3, 0) {
  norender3 = norenderparm && norender3;
  if(norenderparm)
    HU_PlayerMsg(norender3 ? "Sprites rendering off" : "Sprites rendering on");
}
CONSOLE_VARIABLE(r_norender4, norender4, 0) {
  norender4 = norenderparm && norender4;
  if(norenderparm)
    HU_PlayerMsg(norender4 ? "Noop" : "Noop");
}
CONSOLE_VARIABLE(r_norender5, norender5, 0) {
  norender5 = norenderparm && norender5;
  if(norenderparm)
    HU_PlayerMsg(norender5 ? "Top rendering off" : "Top rendering on");
}
CONSOLE_VARIABLE(r_norender6, norender6, 0) {
  norender6 = norenderparm && norender6;
  if(norenderparm)
    HU_PlayerMsg(norender6 ? "Mid rendering off" : "Mid rendering on");
}
CONSOLE_VARIABLE(r_norender7, norender7, 0) {
  norender7 = norenderparm && norender7;
  if(norenderparm)
    HU_PlayerMsg(norender7 ? "Bottom rendering off" : "Bottom rendering on");
}
CONSOLE_VARIABLE(r_norender8, norender8, 0) {
  norender8 = norenderparm && norender8;
  if(norenderparm)
    HU_PlayerMsg(norender8 ? "Masked rendering off" : "Masked rendering on");
}
CONSOLE_VARIABLE(r_norender9, norender9, 0) {
  norender9 = norenderparm && norender9;
  if(norenderparm)
    HU_PlayerMsg(norender9 ? "Flats rendering off" : "Flats rendering on");
}

CONSOLE_VARIABLE(r_norender0, norender0, 0) {
  norender0 = norender0 && norenderparm;
  norender1 = norender3 = norender4 = norender5 =
  norender6 = norender7 = norender8 = norender9 =
  norender0;

  if(!norender0)
    norender2 = -1;

  if(norenderparm)
    HU_PlayerMsg(norender0 ? "All rendering off" : "All rendering on");
}
#endif

VARIABLE_BOOLEAN(wigglefix, NULL, onoff);
CONSOLE_VARIABLE(r_wigglefix, wigglefix, 0) {}

void R_AddCommands()
{
   C_AddCommand(lefthanded);
   C_AddCommand(r_blockmap);
   C_AddCommand(r_homflash);
   C_AddCommand(r_planeview);
   C_AddCommand(r_zoom);
   C_AddCommand(r_precache);
   C_AddCommand(r_showgun);
   C_AddCommand(r_showhom);
   C_AddCommand(r_stretchsky);
   C_AddCommand(r_swirl);
   C_AddCommand(r_trans);
   C_AddCommand(r_fauxtrans);
   C_AddCommand(r_watertrans);
   C_AddCommand(r_tranpct);
   C_AddCommand(screensize);
   C_AddCommand(gamma);

   C_AddCommand(p_dumphubs);
   
   C_AddCommand(p_rmslime);

   C_AddCommand(pal_curr);
   C_AddCommand(pal_next);
   C_AddCommand(pal_prev);
   C_AddCommand(pal_list);

#ifdef NORENDER
   C_AddCommand(r_norender1);
   C_AddCommand(r_norender2);
   C_AddCommand(r_norender3);
   C_AddCommand(r_norender4);
   C_AddCommand(r_norender5);
   C_AddCommand(r_norender6);
   C_AddCommand(r_norender7);
   C_AddCommand(r_norender8);
   C_AddCommand(r_norender9);
   C_AddCommand(r_norender0);
   C_AddCommand(debugcolumn);
#endif

   C_AddCommand(r_wigglefix);
}

//----------------------------------------------------------------------------
//
// $Log: r_main.c,v $
// Revision 1.13  1998/05/07  00:47:52  killough
// beautification
//
// Revision 1.12  1998/05/03  23:00:14  killough
// beautification, fix #includes and declarations
//
// Revision 1.11  1998/04/07  15:24:15  killough
// Remove obsolete HOM detector
//
// Revision 1.10  1998/04/06  04:47:46  killough
// Support dynamic colormaps
//
// Revision 1.9  1998/03/23  03:37:14  killough
// Add support for arbitrary number of colormaps
//
// Revision 1.8  1998/03/16  12:44:12  killough
// Optimize away some function pointers
//
// Revision 1.7  1998/03/09  07:27:19  killough
// Avoid using FP for point/line queries
//
// Revision 1.6  1998/02/17  06:22:45  killough
// Comment out audible HOM alarm for now
//
// Revision 1.5  1998/02/10  06:48:17  killough
// Add flashing red HOM indicator for TNTHOM cheat
//
// Revision 1.4  1998/02/09  03:22:17  killough
// Make TNTHOM control HOM detector, change array decl to MAX_*
//
// Revision 1.3  1998/02/02  13:29:41  killough
// comment out dead code, add HOM detector
//
// Revision 1.2  1998/01/26  19:24:42  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:02  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
