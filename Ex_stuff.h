// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022 Ludicrous_peridot
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
//      Extra stuff.
//
//-----------------------------------------------------------------------------

#ifndef __EX_STUFF__
#define __EX_STUFF__

void Ex_DetectAndLoadExtras(void);
int Ex_DetectAndLoadTapeWads(char *const *filenames, int autoload);
int Ex_InsertFixes(char * iwadfile, int autoload);
void Ex_ListTapeWad();

// load a specialy treated wad file that if game detects it 

typedef enum {
  DYNA_PLAYPAL,
  DYNA_TRANMAP,
  DYNA_COLORMAP,
  DYNA_TOTAL
} dyna_lumpname_t;


int Ex_AddExtraFile(char *filename, extra_file_t extra);
void * Ex_CacheDynamicLumpName(dyna_lumpname_t name, int tag);
int Ex_DynamicNumForName(dyna_lumpname_t name);


// Reloads palette lump before applying
#define I_ResetPalette() { byte * pal = Ex_CacheDynamicLumpName(DYNA_PLAYPAL, PU_CACHE); I_SetPalette(pal + 768*(st_palette > 0 ? st_palette : 0)); }
// and as sad as it is, modules calling this need the below
extern int st_palette;
extern int playpal_wad;              // idx of the wad that has selected PLAYPAL
extern int playpal_wads_count;       // number of loaded wads with PLAYPAL lump
extern int default_playpal_wad;      // idx of that last loaded wad with PLAYPAL lump
                                     // preceding those autoloaded at the end
extern char ** dyna_playpal_wads;    // names of the wad containing a PLAYPAL lump


byte extra_status[NUM_EXTRAS];

#define DOOMED_SELFIE (-1000)

void Ex_ResetExtraStatus();

#define IS_EXTRA_LOADED(extra) (extra_status[extra])
#define MARK_EXTRA_LOADED(extra, trueornot) { extra_status[extra] = trueornot; }

#define EXTRA_STATES_TABLE(extra) ((extra == EXTRA_SELFIE || extra == EXTRA_JUMP) ? states2[1] : states2[0])
#define EXTRA_WEAPONS_TABLE(extra) ((extra == EXTRA_SELFIE || extra == EXTRA_JUMP) ? weaponinfo2[1] : weaponinfo2[0])
#define EXTRA_ACTOR_STATES(actor) ((actor->intflags & MIF_STATE2) ? states2[1] : states2[0])
#define EXTRA_PLAYER_STATES(player) ((player->cheats & CF_SELFIE) ? states2[1] : states2[0])
#define EXTRA_PLAYER_WEAPONS(player) ((player->cheats & CF_SELFIE) ? weaponinfo2[1] : weaponinfo2[0])
#define EXTRA_INFO_STATES(info) ((info->doomednum == DOOMED_SELFIE) ? states2[1] : states2[0]) 
#define EXTRA_ACTOR_FLAG(info) ((info->doomednum == DOOMED_SELFIE) ? MIF_STATE2 : 0)

#endif
