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
//      The actual span/column drawing functions.
//      Here find the main potential for optimization,
//       e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: r_draw.c,v 1.16 1998/05/03 22:41:46 killough Exp $";

#include "doomstat.h"
#include "w_wad.h"
#include "r_draw.h"
#include "r_main.h"
#include "v_video.h"
#include "mn_engin.h"

#define MAXWIDTH  MAX_SCREENWIDTH          /* kilough 2/8/98 */
#define MAXHEIGHT MAX_SCREENHEIGHT

#define SBARHEIGHT 32             /* status bar height at bottom of screen */

#ifdef DJGPP
#define USEASM /* sf: changed #ifdef DJGPP to #ifdef USEASM */
#endif

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

byte *viewimage; 
int  viewwidth;
int  scaledviewwidth;
int  scaledviewheight;        // killough 11/98
int  viewheight;
int  viewwindowx;
int  viewwindowy; 
byte *ylookup[MAXHEIGHT]; 
int  columnofs[MAXWIDTH]; 
int  linesize = SCREENWIDTH;  // killough 11/98

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//

// byte translations[3][256]; // REDUNDANT
 
byte *tranmap;          // translucency filter maps 256x256   // phares 
byte *main_tranmap;     // killough 4/11/98

//
// R_DrawColumn
// Source is the top of the column to scale.
//

lighttable_t *dc_colormap; 
int     dc_x; 
int     dc_yl; 
int     dc_yh; 
fixed_t dc_iscale;
fixed_t dc_texturemid;
int     dc_texheight;    // killough
byte    *dc_source;      // first pixel in a column (possibly virtual) 
int     dc_faux;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
// 

#ifndef USEASM     // killough 2/15/98

void R_DrawColumn (void) 
{ 
  int              count; 
  register byte    *dest;            // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;     

  count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  dest = ylookup[dc_yl] + columnofs[dc_x];  

  // Determine scaling, which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98: more performance tuning

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
        heightmask++;
        heightmask <<= FRACBITS;
          
        if (frac < 0)
          while ((frac += heightmask) <  0);
        else
          while (frac >= heightmask)
            frac -= heightmask;
          
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
            
            *dest = colormap[source[frac>>FRACBITS]];
            dest += linesize;                     // killough 11/98
            if ((frac += fracstep) >= heightmask)
              frac -= heightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
            *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
          }
        if (count & 1)
          *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
  }
} 

#endif

// Here is the version of R_DrawColumn that deals with translucent  // phares
// textures and sprites. It's identical to R_DrawColumn except      //    |
// for the spot where the color index is stuffed into *dest. At     //    V
// that point, the existing color index and the new color index
// are mapped through the TRANMAP lump filters to get a new color
// index whose RGB values are the average of the existing and new
// colors.
//
// Since we're concerned about performance, the 'translucent or
// opaque' decision is made outside this routine, not down where the
// actual code differences are.

#ifndef USEASM                       // killough 2/21/98: converted to x86 asm

void R_DrawTLColumn (void)                                           
{ 
  int              count; 
  register byte    *dest;           // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;

  count = dc_yh - dc_yl + 1; 

  // Zero length, column does not exceed a pixel.
  if (count <= 0)
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  dest = ylookup[dc_yl] + columnofs[dc_x];  
  
  // Determine scaling,
  //  which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98, 2/21/98: more performance tuning
  
  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
        heightmask++;
        heightmask <<= FRACBITS;
          
        if (frac < 0)
          while ((frac += heightmask) <  0);
        else
          while (frac >= heightmask)
            frac -= heightmask;
        
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough
              
            *dest = tranmap[(*dest<<8)+colormap[source[frac>>FRACBITS]]]; // phares
            dest += linesize;          // killough 11/98
            if ((frac += fracstep) >= heightmask)
              frac -= heightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
            dest += linesize;   // killough 11/98
            frac += fracstep;
            *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
            dest += linesize;   // killough 11/98
            frac += fracstep;
          }
        if (count & 1)
          *dest = tranmap[(*dest<<8)+colormap[source[(frac>>FRACBITS) & heightmask]]]; // phares
      }
  }
} 

#endif  // killough 2/21/98: converted to x86 asm

//
// Spectre/Invisibility.
//


#define FUZZTABLE 50 
#define FUZZOFF (SCREENWIDTH)

static const int fuzzoffset[FUZZTABLE] = {
  1,0,1,0,1,1,0,
  1,1,0,1,1,1,0,
  1,1,1,0,0,0,0,
  1,0,0,1,1,1,1,0,
  1,0,1,1,0,0,1,
  1,0,0,0,0,1,1,
  1,1,0,1,1,0,1 
}; 

#define HI_FUZZTABLE 151

static const int hi_fuzzoffset[] = {
  1,0,1,0,1,1,0,
  1,1,0,1,1,1,0,
  1,1,1,0,0,0,0,
  1,0,0,1,1,1,1,0,
  1,0,1,1,0,0,1,
  1,0,0,0,0,1,1,
  1,1,0,1,1,0,1, 
  1,0,0,0,0,1,1,
  1,0,1,1,0,0,1,
  1,0,0,1,1,1,1,0,
  1,1,1,0,0,0,0,
  1,1,0,1,1,1,0,
  1,1,1,0,0,0,0,
  1,0,0,0,0,1,1,
  1,0,1,1,0,0,1,
  1,0,0,1,1,1,1,0,
  1,1,0,1,1,1,0,
  1,1,1,0,0,0,0,
  1,1,0,1,1,0,1,
  1,0,1,1,0,0,1,
  1,1,1,0,0,0,0,1
}; 

static int fuzzpos = 0; 

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//

// sf: restored original fuzz effect (changed in mbf)
// sf: changed to use vis->colormap not fullcolormap
//     for coloured lighting and SHADOW now done with
//     flags not NULL colormap

void R_DrawFuzzColumn(void) 
{ 
  if (hires)
    {
      byte     *dest, *src; 
      fixed_t  frac;
      fixed_t  fracstep;
      const int b = hires, w = SCREENWIDTH << hires, bw = w * hires;
      int      ly = dc_yl;
      int      hirescount = b - (ly % b); 
      int      fw = (ly < viewheight - b - 1) ? bw  : -w, bc = ly < b ? 0 : -w;

      // Zero length.
      if (dc_yh - dc_yl < 0) 
        return; 
        
#ifdef RANGECHECK 
      // haleyjd: these should apparently be adjusted for hires

      if ((unsigned) dc_x >= SCREENWIDTH << hires
          || dc_yl < 0 
          || dc_yh >= SCREENHEIGHT << hires)
        I_Error ("R_DrawFuzzColumn: %i to %i at %i",
                 dc_yl, dc_yh, dc_x);
#endif

      // Keep till detailshift bug in blocky mode fixed,
      //  or blocky mode removed.

      // Does not work with blocky mode.
      dest = ylookup[dc_yl] + columnofs[dc_x];
      src = ylookup[dc_yl - (dc_yl % b)] + columnofs[dc_x - (dc_x % b)];
      
      // Looks familiar.
      fracstep = dc_iscale; 
      frac = dc_texturemid + (dc_yl-centery)*fracstep; 

      if(dc_x % b) do 
        {
          *dest = dc_colormap[6*256+dest[-1]];
          dest += SCREENWIDTH << hires;
          frac += fracstep; 
        } while (++ly <= dc_yh); 

      // Looks like an attempt at dithering,
      // using the colormap #6 (of 0-31, a bit brighter than average).
      
      else do  
        {
          lighttable_t * localmap = !comp[comp_clighting] ? dc_colormap : fullcolormap;

          // Lookup framebuffer, and retrieve
          //  a pixel that is either one column
          //  left or right of the current one.
          // Add index from colormap to index.
          // killough 3/20/98: use fullcolormap instead of colormaps
          // sf: use dc_colormap for coloured lighting

                    //sf : hires
          *dest = localmap[6*256+src[hi_fuzzoffset[fuzzpos] ? fw : bc]];

          if(!--hirescount)
            {
              hirescount = b;
               // Clamp table lookup index.
              if (++fuzzpos == HI_FUZZTABLE) 
                fuzzpos = 0;
              src += bw;
              fw = (ly < viewheight - b - 1) ? bw : -w;
              bc = -w;
            }
            
          dest += SCREENWIDTH << hires;
          frac += fracstep; 
        } while (++ly <= dc_yh); 
    }
  else
    {
      int      count; 
      byte     *dest; 
      fixed_t  frac;
      fixed_t  fracstep;     

      fuzzpos %= FUZZTABLE;

      // Adjust borders. Low...
      if (!dc_yl) 
        dc_yl = 1;

      // .. and high.
      if (dc_yh == viewheight-1) 
        dc_yh = viewheight - 2; 

      count = dc_yh - dc_yl;

      // Zero length.
      if (count < 0) 
        return; 
        
#ifdef RANGECHECK 
      // haleyjd: these should apparently be adjusted for hires

      if ((unsigned) dc_x >= SCREENWIDTH 
          || dc_yl < 0 
          || dc_yh >= SCREENHEIGHT)
        I_Error ("R_DrawFuzzColumn: %i to %i at %i",
                 dc_yl, dc_yh, dc_x);
#endif

      // Keep till detailshift bug in blocky mode fixed,
      //  or blocky mode removed.

      // Does not work with blocky mode.
      dest = ylookup[dc_yl] + columnofs[dc_x];
      
      // Looks familiar.
      fracstep = dc_iscale; 
      frac = dc_texturemid + (dc_yl-centery)*fracstep; 

      // Looks like an attempt at dithering,
      // using the colormap #6 (of 0-31, a bit brighter than average).

      do 
        {
          lighttable_t * localmap = !comp[comp_clighting] ? dc_colormap : fullcolormap;
          // Lookup framebuffer, and retrieve
          //  a pixel that is either one column
          //  left or right of the current one.
          // Add index from colormap to index.
          // killough 3/20/98: use fullcolormap instead of colormaps

          // sf: use dc_colormap for coloured lighting
            //sf : hires
          *dest = localmap[6*256+dest[
          fuzzoffset[fuzzpos] ? SCREENWIDTH : -(SCREENWIDTH)]];


          // Clamp table lookup index.
          if (++fuzzpos == FUZZTABLE) 
            fuzzpos = 0;
            
          dest += SCREENWIDTH;

          frac += fracstep; 
        } while (count--); 
    }
}

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//

byte *dc_translation, *translationtables;

void R_DrawTranslatedColumn (void) 
{ 
  int      count; 
  byte     *dest; 
  fixed_t  frac;
  fixed_t  fracstep;     
 
  count = dc_yh - dc_yl; 
  if (count < 0) 
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT)
    I_Error ( "R_DrawColumn: %i to %i at %i",
              dc_yl, dc_yh, dc_x);
#endif 

  // FIXME. As above.
  dest = ylookup[dc_yl] + columnofs[dc_x]; 

  // Looks familiar.
  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  count++;        // killough 1/99: minor tuning

  // Here we do an additional index re-mapping.
  do 
    {
      // Translation tables are used
      //  to map certain colorramps to other ones,
      //  used with PLAY sprites.
      // Thus the "green" ramp of the player 0 sprite
      //  is mapped to gray, red, black/indigo. 
      
      *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
      dest += (SCREENWIDTH << hires);      // killough 11/98
        
      frac += fracstep; 
    }
  while (--count); 
} 

#ifdef FAUXTRAN
void R_DrawTranslatedCheckers (void) 
{ 
  int      count; 
  byte     *dest; 
  fixed_t  frac;
  fixed_t  fracstep;     
 
  count = dc_yh - dc_yl; 
  if (count < 0) 
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT)
    I_Error ( "R_DrawColumn: %i to %i at %i",
              dc_yl, dc_yh, dc_x);
#endif 

  // FIXME. As above.
  dest = ylookup[dc_yl] + columnofs[dc_x];
  dc_faux = dc_yl ^ dc_x;

  // Looks familiar.
  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  count++;        // killough 1/99: minor tuning

  // Here we do an additional index re-mapping.
  do 
    {
      // Translation tables are used
      //  to map certain colorramps to other ones,
      //  used with PLAY sprites.
      // Thus the "green" ramp of the player 0 sprite
      //  is mapped to gray, red, black/indigo. 

      if (dc_faux & 1)
         *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
      dest += (SCREENWIDTH << hires);      // killough 11/98
        
      frac += fracstep;
      dc_faux ++;
    }
  while (--count); 
} 
#endif

//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//

typedef struct
{
  int start;      // start of the sequence of colours
  int number;     // number of colours
} translat_t;

translat_t translations[] =
{
    {96,  16},     // indigo
    {64,  16},     // brown
    {32,  16},     // red
  
  /////////////////////////
  // New colours
  
    {176, 16},     // tomato
    {128, 16},     // dirt
    {200, 8},      // blue
    {160, 8},      // gold
    {152, 8},      // felt?
    {0,   1},      // bleeacckk!!
    {250, 5},      // purple
    {168, 8}, // bright pink, kinda
    {216, 8},      // vomit yellow
    {16,  16},     // pink                       
    {56,  8},      // cream
    {88,  8},      // white


    {200, 8},      // blue
    {112, 16},     // good old green again
    {160, 8},      // gold
    {250, 5},      // purple
};

// sf : rewritten

void R_InitTranslationTables (void)
{
  int i, c;
  
  translationtables = Z_Malloc(256 * (TRANSLATIONCOLOURS + 4), PU_STATIC, 0);
  
  for(i=0; i<TRANSLATIONCOLOURS + 4; i++)
    {
      for(c=0; c<256; c++)
	translationtables[i*256 + c] =
          (c >= 0x70 && c <= 0x7f) || (i >= TRANSLATIONCOLOURS && c >= 0xb0 && c <= 0xbf && bloodcolor) 
             ? translations[i].start +
                ((c & 0xf) * (translations[i].number-1))/15
             : c;
    }
}


void R_RefreshTranslationTables (int color)
{
  int i, c;
  
  for(i=TRANSLATIONCOLOURS ; i < TRANSLATIONCOLOURS + 4 ; i++)
    {
      for(c=0xb0; c<=0xbf; c++)
	translationtables[i*256 + c] =
          color  
             ? translations[i].start +
                ((c & 0xf) * (translations[i].number-1))/15
             : c;
    }
}

//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//

int  ds_y; 
int  ds_x1; 
int  ds_x2;

lighttable_t *ds_colormap; 

fixed_t ds_xfrac; 
fixed_t ds_yfrac; 
fixed_t ds_xstep; 
fixed_t ds_ystep;

// start of a 64*64 tile image 
byte *ds_source;        

#ifndef USEASM      // killough 2/15/98

void R_DrawSpan (void) 
{ 
  register unsigned position;
  unsigned step;

  byte *source;
  byte *colormap;
  byte *dest;
    
  unsigned count;
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;
                
  position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
  step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
                
  source = ds_source;
  colormap = ds_colormap;
  dest = ylookup[ds_y] + columnofs[ds_x1];       
  count = ds_x2 - ds_x1 + 1; 
        
  while (count >= 4)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[0] = colormap[source[spot]]; 

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[1] = colormap[source[spot]];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[2] = colormap[source[spot]];
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[3] = colormap[source[spot]]; 
                
      dest += 4;
      count -= 4;
    } 

  while (count)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      *dest++ = colormap[source[spot]]; 
      count--;
    } 
} 

#endif


#ifdef FAUXTRAN
void drawCheckeredSpan (void)
{ 
  register unsigned position;
  unsigned step;

  byte *source;
  byte *colormap;
  byte *dest;
    
  unsigned count;
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;
                
  position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
  step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
                
  source = ds_source;
  colormap = ds_colormap;
  dest = ylookup[ds_y] + columnofs[ds_x1];       
  count = ds_x2 - ds_x1 + 1;

  dc_faux = ds_x1 ^ ds_y;

  if(dc_faux) 
    while (count >= 4)
      { 
        ytemp = position>>4;
        ytemp = ytemp & 4032;
        xtemp = position>>26;
        position += step;
        position += step;
        if((xtemp ^ (ytemp >> 6)) & 1)
          {
            spot = xtemp | ytemp;
            dest[0] = colormap[source[spot]]; 
          }

        ytemp = position>>4;
        ytemp = ytemp & 4032;
        xtemp = position>>26;
        position += step;
        position += step;
        if((xtemp ^ (ytemp >> 6)) & 1)
          {
            spot = xtemp | ytemp;
            dest[2] = colormap[source[spot]]; 
          }
                
        dest += 4;
        count -= 4;
      }
  else
    while (count >= 4)
      { 
        position += step;
        ytemp = position>>4;
        ytemp = ytemp & 4032;
        xtemp = position>>26;
        position += step;
        if((xtemp ^ (ytemp >> 6)) & 1)
          {
            spot = xtemp | ytemp;
            dest[1] = colormap[source[spot]]; 
          }

        
        position += step;
        ytemp = position>>4;
        ytemp = ytemp & 4032;
        xtemp = position>>26;
        position += step;
        if((xtemp ^ (ytemp >> 6)) & 1)
          {
            spot = xtemp | ytemp;
            dest[3] = colormap[source[spot]]; 
          }
                
        dest += 4;
        count -= 4;
      }

  while (count)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      position += step;
      if(dc_faux & 1)
        {
          spot = xtemp | ytemp;
          *dest++ = colormap[source[spot]];
        }
      dc_faux++;
      count--;
    } 
}
#endif

        // sf: 
void R_DrawTLSpan (void)
{ 
  register unsigned position;
  unsigned step;

  byte *source;
  byte *colormap;
  byte *dest;
    
  unsigned count;
  unsigned spot; 
  unsigned xtemp;
  unsigned ytemp;

#ifdef FAUXTRAN
  if(faux_translucency)
    {
      drawCheckeredSpan();
      return;
    }
#endif
  
  position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
  step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);
                
  source = ds_source;
  colormap = ds_colormap;
  dest = ylookup[ds_y] + columnofs[ds_x1];       
  count = ds_x2 - ds_x1 + 1; 
        
  while (count >= 4)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[0] = tranmap[(dest[0]<<8)+colormap[source[spot]] ]; 

      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[1] = tranmap[(dest[1]<<8)+colormap[source[spot]] ]; 
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[2] = tranmap[(dest[2]<<8)+colormap[source[spot]] ]; 
        
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      dest[3] = tranmap[(dest[3]<<8)+colormap[source[spot]] ]; 
                
      dest += 4;
      count -= 4;
    } 

  while (count)
    { 
      ytemp = position>>4;
      ytemp = ytemp & 4032;
      xtemp = position>>26;
      spot = xtemp | ytemp;
      position += step;
      *dest++ = tranmap[(*dest<<8)+colormap[source[spot]] ];
      count--;
    } 
} 


//
// R_InitBuffer 
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//

void R_InitBuffer(int width, int height)
{ 
  int i; 
  int sc = hires - 1;

  linesize = (SCREENWIDTH << hires);    // killough 11/98

  // Handle resize,
  //  e.g. smaller view windows
  //  with border and/or status bar.

  viewwindowx = SCREENWIDTH-width;
  if( sc > 0 ) viewwindowx <<= sc;
  else if( sc < 0 ) viewwindowx >>= 1;
   

  // Column offset. For windows.

  for (i = width << hires ; i--; )   // killough 11/98
    columnofs[i] = viewwindowx + i;
    
  // Same with base row offset.

  viewwindowy = width==SCREENWIDTH ? 0 : SCREENHEIGHT-SBARHEIGHT-height;
  if( sc > 0 ) viewwindowy <<= sc;
  else if( sc < 0 ) viewwindowy >>= 1;

  // Precalculate all row offsets.

  for (i = height << hires; i--; )
    ylookup[i] = screens[0] + (i+viewwindowy)*linesize; // killough 11/98
} 

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//

void R_FillBackScreen (void) 
{ 
  // killough 11/98: trick to shadow variables
  int x = viewwindowx, y = viewwindowy; 
  int viewwindowx = x >> hires, viewwindowy = y >> hires;  // killough 11/98
  patch_t *patch;

  if (scaledviewwidth == 320)
    return;

  // killough 11/98: use the function in m_menu.c
  V_DrawBackground(gamemode==commercial ? "GRNROCK" : "FLOOR7_2", screens[1]);
        
  patch = W_CacheLumpName("brdr_t", PU_CACHE);

  for (x=0; x<scaledviewwidth; x+=8)
    V_DrawPatch(viewwindowx+x,viewwindowy-8,1,patch);

  patch = W_CacheLumpName("brdr_b",PU_CACHE);

  for (x=0; x<scaledviewwidth; x+=8)   // killough 11/98:
    V_DrawPatch (viewwindowx+x,viewwindowy+scaledviewheight,1,patch);

  patch = W_CacheLumpName("brdr_l",PU_CACHE);

  for (y=0; y<scaledviewheight; y+=8)             // killough 11/98
    V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
  patch = W_CacheLumpName("brdr_r",PU_CACHE);

  for (y=0; y<scaledviewheight; y+=8)             // killough 11/98
    V_DrawPatch(viewwindowx+scaledviewwidth,viewwindowy+y,1,patch);

  // Draw beveled edge. 
  V_DrawPatch(viewwindowx-8,
              viewwindowy-8,
              1,
              W_CacheLumpName("brdr_tl",PU_CACHE));
    
  V_DrawPatch(viewwindowx+scaledviewwidth,
              viewwindowy-8,
              1,
              W_CacheLumpName("brdr_tr",PU_CACHE));
    
  V_DrawPatch(viewwindowx-8,
              viewwindowy+scaledviewheight,             // killough 11/98
              1,
              W_CacheLumpName("brdr_bl",PU_CACHE));
    
  V_DrawPatch(viewwindowx+scaledviewwidth,
              viewwindowy+scaledviewheight,             // killough 11/98
              1,
              W_CacheLumpName("brdr_br",PU_CACHE));
} 

//
// Copy a screen buffer.
//

void R_VideoErase(unsigned ofs, int count)
{ 
  if (hires)     // killough 11/98: hires support
    {
      int x = ofs % SCREENWIDTH;
      int y = ofs - x;
      int lines = 1 << hires, cols = count << hires;
      ofs = (y << hires << hires) + (x << hires);

      while (--lines>=0)
        {
          memcpy(screens[0]+ofs, screens[1]+ofs, cols);   // LFB copy.
          ofs += (SCREENWIDTH << hires);
        }
    }
  else
    {
      memcpy(screens[0]+ofs, screens[1]+ofs, count);   // LFB copy.
    }
} 

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
// killough 11/98: 
// Rewritten to avoid relying on screen wraparound, so that it
// can scale to hires automatically in R_VideoErase().
//

void R_DrawViewBorder(void) 
{ 
  int side, ofs, i;
 
  if (scaledviewwidth == SCREENWIDTH) 
    return;

  // copy top
  for (ofs = 0, i = viewwindowy >> hires; i--; ofs += SCREENWIDTH)
    R_VideoErase(ofs, SCREENWIDTH); 

  // copy sides
  for (side = viewwindowx >> hires, i = scaledviewheight; i--;)
    { 
      R_VideoErase(ofs, side); 
      ofs += SCREENWIDTH;
      R_VideoErase(ofs - side, side); 
    } 

  // copy bottom 
  for (i = viewwindowy >> hires; i--; ofs += SCREENWIDTH)
    R_VideoErase(ofs, SCREENWIDTH); 
 
  V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT); 
} 

// haleyjd: experimental column drawer for masked sky textures
void R_DrawNewSkyColumn(void) 
{ 
  int              count; 
  register byte    *dest;            // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;     

  count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawNewSkyColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  dest = ylookup[dc_yl] + columnofs[dc_x];  

  // Determine scaling, which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  // Inner loop that does the actual texture mapping,
  //  e.g. a DDA-lile scaling.
  // This is as fast as it gets.       (Yeah, right!!! -- killough)
  //
  // killough 2/1/98: more performance tuning

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register int heightmask = dc_texheight-1;
    if (dc_texheight & heightmask)   // not a power of 2 -- killough
      {
        heightmask++;
        heightmask <<= FRACBITS;
          
        if (frac < 0)
          while ((frac += heightmask) <  0);
        else
          while (frac >= heightmask)
            frac -= heightmask;
          
        do
          {
            // Re-map color indices from wall texture column
            //  using a lighting/special effects LUT.
            
            // heightmask is the Tutti-Frutti fix -- killough

            // haleyjd
            if(source[frac>>FRACBITS])
              *dest = colormap[source[frac>>FRACBITS]];
            dest += linesize;                     // killough 11/98
            if ((frac += fracstep) >= heightmask)
              frac -= heightmask;
          } 
        while (--count);
      }
    else
      {
        while ((count-=2)>=0)   // texture height is a power of 2 -- killough
          {
            if(source[(frac>>FRACBITS) & heightmask])
              *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
            if(source[(frac>>FRACBITS) & heightmask])
              *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
            dest += linesize;   // killough 11/98
            frac += fracstep;
          }
        if ((count & 1) && source[(frac>>FRACBITS) & heightmask])
          *dest = colormap[source[(frac>>FRACBITS) & heightmask]];
      }
  }
} 


void R_DrawTallSkyColumn(void) 
{ 
  int              count; 
  register byte    *dest;            // killough
  register fixed_t frac;            // killough
  fixed_t          fracstep;     
  column_t * thisPost, * nextPost;
  int postLen;

  count = dc_yh - dc_yl + 1; 

  if (count <= 0)    // Zero length, column does not exceed a pixel.
    return; 
                                 
#ifdef RANGECHECK 
  if ((unsigned)dc_x >= MAX_SCREENWIDTH
      || dc_yl < 0
      || dc_yh >= MAX_SCREENHEIGHT) 
    I_Error ("R_DrawTallSkyColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

  // Framebuffer destination address.
  // Use ylookup LUT to avoid multiply with ScreenWidth.
  // Use columnofs LUT for subwindows? 

  dest = ylookup[dc_yl] + columnofs[dc_x];  

  // Determine scaling, which is the only mapping to be done.

  fracstep = dc_iscale; 
  frac = dc_texturemid + (dc_yl-centery)*fracstep; 

  thisPost = (column_t *)(dc_source - 3);
  // panic if first post is offset (i.e. masked sky column)
  if(thisPost->topdelta != 0)
    return;
  postLen = thisPost->length;
  nextPost = (column_t *)(((byte *)thisPost) + postLen + 4);
  // skip the stub post for tall textures
  if(nextPost->topdelta == 0xfe && nextPost->length == 0)
    nextPost = (column_t *)(((byte *)nextPost) + 4);
  if(nextPost->topdelta == 1 && nextPost->length == 0)
    nextPost = (column_t *)(((byte *)nextPost) + 4);
  // panic if second post does not start immediately after first finishes
  if(nextPost->topdelta > 0)
    return;

  {
    register const byte *source = dc_source;            
    register const lighttable_t *colormap = dc_colormap; 
    register int heightmask = dc_texheight-1;
    register const byte *tallSource = ((byte*)nextPost) + 3;
    register const int thresh = postLen;
    heightmask++;
    heightmask <<= FRACBITS;
      
    if (frac < 0)
      while ((frac += heightmask) <  0);
    else
      while (frac >= heightmask)
        frac -= heightmask;
      
    do
      {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        
        // haleyjd
        register int offs = frac >> FRACBITS;
        if(offs < thresh)
          {
            *dest = colormap[source[offs]];
          }
        else
          {
            *dest = colormap[tallSource[offs - thresh]];
          }         
        dest += linesize;                     // killough 11/98
        if ((frac += fracstep) >= heightmask)
          frac -= heightmask;
      } 
    while (--count);
  }
} 

//----------------------------------------------------------------------------
//
// $Log: r_draw.c,v $
// Revision 1.16  1998/05/03  22:41:46  killough
// beautification
//
// Revision 1.15  1998/04/19  01:16:48  killough
// Tidy up last fix's code
//
// Revision 1.14  1998/04/17  15:26:55  killough
// fix showstopper
//
// Revision 1.13  1998/04/12  01:57:51  killough
// Add main_tranmap
//
// Revision 1.12  1998/03/23  03:36:28  killough
// Use new 'fullcolormap' for fuzzy columns
//
// Revision 1.11  1998/02/23  04:54:59  killough
// #ifdef out translucency code since its in asm
//
// Revision 1.10  1998/02/20  21:57:04  phares
// Preliminarey sprite translucency
//
// Revision 1.9  1998/02/17  06:23:40  killough
// #ifdef out code duplicated in asm for djgpp targets
//
// Revision 1.8  1998/02/09  03:18:02  killough
// Change MAXWIDTH, MAXHEIGHT defintions
//
// Revision 1.7  1998/02/02  13:17:55  killough
// performance tuning
//
// Revision 1.6  1998/01/27  16:33:59  phares
// more testing
//
// Revision 1.5  1998/01/27  16:32:24  phares
// testing
//
// Revision 1.4  1998/01/27  15:56:58  phares
// Comment about invisibility
//
// Revision 1.3  1998/01/26  19:24:40  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:05:55  killough
// Use unrolled version of R_DrawSpan
//
// Revision 1.1.1.1  1998/01/19  14:03:02  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
