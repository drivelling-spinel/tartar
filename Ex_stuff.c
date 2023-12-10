// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022 Ludicros_peridot
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

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "doomstat.h"
#include "c_runcmd.h"
#include "c_io.h"
#include "c_net.h"
#include "ex_stuff.h"
#include "dstrings.h"
#include "d_deh.h"  // Ty 03/27/98 - externalized strings
#include "sounds.h"
#include "d_main.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_argv.h"

#define EXTRA_STATES_INDEX(extra) ( extra == EXTRA_JUMP ? 2 : extra == EXTRA_SELFIE ? 1 : 0 )
#define INIT_EXTRA_STATES(extra) { int idx = EXTRA_STATES_INDEX(extra); \
  if(idx && states3[idx] == states) \
    { \
      memcpy(states3[idx] = malloc(sizeof(states)), &states, sizeof(states)); \
      memcpy(weaponinfo3[idx] = malloc(sizeof(weaponinfo)), &weaponinfo, sizeof(weaponinfo)); \
    } \
}

#define plyr (&players[consoleplayer])     /* the console player */

static char *Ex_tape(void)
{
  static char *tps;      // cache results over multiple calls
  if (!tps)
    {
      int p = M_CheckParm("-tape");
      tps = p && ++p < myargc ? myargv[p] : "";
    }
  return tps;
}

static char *Ex_fixes(void)
{
  static char *fps;      // cache results over multiple calls
  if (!fps)
    {
      int p = M_CheckParm("-fixes");
      fps = p && ++p < myargc ? myargv[p] : "";
    }
  return fps;
}



static char filestr[PATH_MAX+1];
static char tapestr[PATH_MAX+1];

static int        tapehandle = 0;
extern int        hashnumlumps;

int Ex_DetectAndLoadTapeWads(char *const *filenames, int autoload)
{
  if(*Ex_tape()) 
    {
      assert(strlen(Ex_tape()) < sizeof(tapestr));
      strcat(tapestr, Ex_tape());
      AddDefaultExtension(tapestr, ".wad");
      if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;
    }
  
  while(*filenames && autoload)
    {
      struct stat sbuf;
      ExtractFileBase(*filenames, filestr, sizeof(filestr) - 1);
      assert(strlen(D_DoomExeDir()) + strlen(filestr) + 10 <= sizeof(tapestr));
      sprintf(tapestr, "%stape/%s", D_DoomExeDir(), filestr);
      AddDefaultExtension(tapestr, ".wad");
      if (!stat(tapestr, &sbuf)) 
        if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;
      filenames++;
    }
    
  *tapestr = 0;
  return 0;
}

void Ex_ListTapeWad()
{
  if(*tapestr)
    C_Printf(FC_GOLD "%s " FC_RED "is taped onto the WADs\n", tapestr);
}

int Ex_FindAllWads(char * dir, char *** fnames)
{
  DIR *dirp;
  struct dirent * ent;
  char ** nms;
  int num = 0, i, l = strlen(dir) + 2;

  dirp = opendir(dir);
  while(dirp && (ent = readdir(dirp)))
    {
      if(ent->d_namlen >= 4 && !strnicmp(".wad", ent->d_name + ent->d_namlen - 4, 4))
        num += 1;
    }

  if(num)
    {
      rewinddir(dirp);
      nms = malloc(sizeof(char *) * num);
      *fnames = nms;
      i = num;

      while(i && (ent = readdir(dirp)))
        if(ent->d_namlen >= 4 && !strnicmp(".wad", ent->d_name + ent->d_namlen - 4, 4))
          {
            nms[num - i] = malloc(ent->d_namlen + l);
            memset(nms[num - i], 0, ent->d_namlen + l);
            strcpy(nms[num - i], dir);
            nms[num-i][l - 2] = '/';
            strncpy(nms[num - i] + l - 1, ent->d_name, ent->d_namlen);
            i -= 1;
          }
    }

  if(dirp) closedir(dirp);
  return num;
}


int Ex_FindFilterWads(char *** fnames)
{
  struct stat sbuf;
  *filestr = 0;
  sprintf(filestr, "%sfilters", D_DoomExeDir());

  if (!stat(filestr, &sbuf))      
    {
      if(!S_ISDIR(sbuf.st_mode)) return 0;
      return Ex_FindAllWads(filestr, fnames);     
    }

  return 0;  
}


int Ex_DetectAndLoadFilters()
{
  char **fnames;
  int numfilters, loaded = 0;
  int i;

  numfilters = Ex_FindFilterWads(&fnames);
  *filestr = 0;

  for(i = 0 ; i < numfilters ; i += 1)
    {
      if(!W_AddExtraFile(fnames[i], EXTRA_FILTERS))
        {
          if(!loaded) ExtractFileBase(fnames[i], filestr, sizeof(filestr) -1);
          loaded += 1;
        }
    }

  if(loaded == 1) usermsg("%s filter loaded", filestr);
  if(loaded > 1) usermsg("%s and %d other filter%s loaded",
    filestr, loaded, loaded == 2 ? "" : "s");

  if(numfilters)
    {
      for(i = 0 ; i < numfilters ; i += 1) free(fnames[i]);
      free(fnames);
    }

  return loaded;
}

int Ex_InsertAllFixesInDir(char * dirname)
{
  struct stat sbuf;
  int numfixes = 0, i;
  char **fnames;

  if (!stat(dirname, &sbuf))      
    {
      if(!S_ISDIR(sbuf.st_mode)) return 0;
      numfixes = Ex_FindAllWads(dirname, &fnames);
      if(numfixes)
        {
          for(i = numfixes - 1 ; i >= 0 ; i -= 1)
          {
            D_InsertFile(fnames[i]); 
            free(fnames[i]);
          }
          free(fnames);
        }
    }

  return numfixes;  
}

int Ex_InsertFixes(char * iwadfile, int autoload)
{
  int numfixes = 0;
  int l = 0;
  *filestr = 0;

  if (*Ex_fixes())
    {
      assert(strlen(Ex_fixes()) < sizeof(filestr));
      strcpy(filestr, Ex_fixes());
    }
  else if(autoload)
    {
      assert(strlen(D_DoomExeDir()) + strlen("fixes") < sizeof(filestr));
      sprintf(filestr, "%s%s", D_DoomExeDir(), "fixes");
    }

  if(!*filestr) return 0;

  l = strlen(filestr);
  numfixes += Ex_InsertAllFixesInDir(filestr);
  assert(l < sizeof(filestr) - 2);
  strcat(filestr, "/");
  ExtractFileBase(iwadfile, filestr + l + 1, sizeof(filestr) - l - 1);
  numfixes += Ex_InsertAllFixesInDir(filestr);

  return numfixes;
}


void A_TakeSelfie();
void A_SelfieSound();
void A_FirePlasma();
void A_BFGsound();
void A_PlaceJumppad();
void A_VileAttack();
void A_TossUp();

int Ex_DetectAndLoadSelfie()
{
  struct stat sbuf;
  int i = 0;

  *filestr = 0;
  sprintf(filestr, "%sselfie.wad", D_DoomExeDir());
  if(stat(filestr, &sbuf)) return 0;

  INIT_EXTRA_STATES(EXTRA_SELFIE);
  if(W_AddExtraFile(filestr, EXTRA_SELFIE)) return 0;
  // another hack - we just happen to know that selfie overrides BFG sprites
  for(i = 0 ; i < NUMSTATES ; i += 1)
    {
      if(states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].sprite == SPR_BFGG) states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].sprite = SPR_SELF;
      if(states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].action == A_FirePlasma) states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].action = A_TakeSelfie;
      if(states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].action == A_BFGsound) states3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][i].action = A_SelfieSound;
    }
  weaponinfo3[EXTRA_STATES_INDEX(EXTRA_SELFIE)][wp_bfg].ammo = am_noammo;
  MARK_EXTRA_LOADED(EXTRA_SELFIE, true);
  C_Printf("%s\n",s_GOTSELFIE);
  
  return 1;
}

int Ex_DetectAndLoadJumpwad()
{
  struct stat sbuf;
  int i = 0;
  
  *filestr = 0;
  sprintf(filestr, "%sjumpwad.wad", D_DoomExeDir());
  if(stat(filestr, &sbuf)) return 0;
  
  INIT_EXTRA_STATES(EXTRA_JUMP);
  if(W_AddExtraFile(filestr, EXTRA_JUMP)) return 0;
  for(i = 0 ; i < NUMSTATES ; i += 1)
    {
      if(states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite == SPR_PISF) states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite = SPR_JMPF;
      if(states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite == SPR_PISG) states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite = SPR_JMPG;
      if(states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite == SPR_PLSS) states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].sprite = SPR_JMPS;
      if(states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].action == A_FirePlasma) states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].action = A_PlaceJumppad;
      if(states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].action == A_VileAttack) states3[EXTRA_STATES_INDEX(EXTRA_JUMP)][i].action = A_TossUp;
    }
  weaponinfo3[EXTRA_STATES_INDEX(EXTRA_JUMP)][wp_pistol].ammo = am_noammo;
  mobjinfo[MT_JUMPPAD].deathsound = sfx_jump;
  mobjinfo[MT_JUMPPAD].seesound = sfx_None;
  MARK_EXTRA_LOADED(EXTRA_JUMP, true);

  return 1;
}


int Ex_LoadWiMapsWad(const char *fname)
{
  struct stat sbuf;

  *filestr = 0;
  sprintf(filestr, "%s%s", D_DoomExeDir(), fname);
  if(!stat(filestr, &sbuf) && !W_AddExtraFile(filestr, EXTRA_WIMAPS)) 
    {
      C_Printf("Intermission maps loaded from %s\n", fname);
      return 1;
    }

  return 0;
}


int Ex_DetectAndLoadWiMaps()
{
  int loaded = 0;

  switch(gamemission)
  {
    case doom2:
      loaded += Ex_LoadWiMapsWad("intmapd2.wad");
      break;

    case pack_tnt:
      loaded += Ex_LoadWiMapsWad("intmapev.wad");
      break;

    case pack_plut:
      loaded += Ex_LoadWiMapsWad("intmappl.wad");
      break;

    default:
  }

  MARK_EXTRA_LOADED(EXTRA_WIMAPS, loaded);
  
  loaded += Ex_LoadWiMapsWad("intmapnr.wad");
  return loaded;
}

void Ex_DetectAndLoadExtras(void)
{
  int loaded = 0, total = 0;
  MARK_EXTRA_LOADED(EXTRA_FILTERS, total += loaded = Ex_DetectAndLoadFilters());
  MARK_EXTRA_LOADED(EXTRA_JUMP, total += loaded = Ex_DetectAndLoadJumpwad());
  MARK_EXTRA_LOADED(EXTRA_SELFIE, total += loaded = Ex_DetectAndLoadSelfie());
  MARK_EXTRA_LOADED(EXTRA_WIMAPS, total += loaded = Ex_DetectAndLoadWiMaps());
  if(total)
    D_ReInitWadfiles();
}

/***************************
           CONSOLE COMMANDS
 ***************************/

/******** command list *********/

CONSOLE_COMMAND(selfie, 0)
{
  if(!plyr || !IS_EXTRA_LOADED(EXTRA_SELFIE) || gamestate != GS_LEVEL) return;
  
  if(!(plyr->cheats & CF_SELFIE)) {
    plyr->pendingweapon = wp_selfie;
  }
}

CONSOLE_COMMAND(pogo, 0)
{
  if(!plyr || !IS_EXTRA_LOADED(EXTRA_JUMP) || gamestate != GS_LEVEL) return;
  
  if(!(plyr->cheats & CF_JUMP)) {
    plyr->pendingweapon = wp_pogo;
  }
}


void Ex_AddCommands()
{
   C_AddCommand(selfie);   
   C_AddCommand(pogo);   
}


int Ex_CheckNumForNameOnTape(register const char *name)
{
  if(!tapehandle) return -1;
  else 
    {
      register int i = lumpinfo[W_LumpNameHash(name) % (unsigned) hashnumlumps]->index;
      while (i >= 0 && (strncasecmp(lumpinfo[i]->name, name, 8) ||
                        lumpinfo[i]->handle != tapehandle))
        i = lumpinfo[i]->next;
      return i;
    }
}

// W_CacheDynamicLumpName
// fetches a lump from an arbitrary loaded wad,
// rather then using the latest one
// uses a simple lookup table for actual lumpnum

int playpal_wad;         
int playpal_wads_count;  
int default_playpal_wad; 
int * dyna_playpal_nums;
int * dyna_colormap_nums;
int * dyna_tranmap_nums;
char ** dyna_playpal_wads;

int * dyna_lump_nums[DYNA_TOTAL];

void Ex_InitDynamicLumpNames()
{
  default_playpal_wad = playpal_wad = -1;
  playpal_wads_count = 0;
  dyna_colormap_nums = dyna_tranmap_nums = dyna_playpal_nums = 0;
  memset(dyna_lump_nums, 0xff, sizeof(int *) * DYNA_TOTAL);
}


int Ex_SetDefaultDynamicLumpNames()
{
  int was = playpal_wad;
  playpal_wad = default_playpal_wad;
  if(playpal_wad < 0 && playpal_wads_count > 0) playpal_wad = 0;
  was = was != playpal_wad;

  return was;
}

int Ex_DynamicNumForName(dyna_lumpname_t name)
{
  assert(name >= 0);
  assert(name < DYNA_TOTAL);

  if(playpal_wads_count <= 0 || playpal_wad < 0) return -1;
  return dyna_lump_nums[name][playpal_wad];
}


void * Ex_CacheDynamicLumpName(dyna_lumpname_t name, int tag)
{
  static char * names[] = { "PLAYPAL", "TRANMAP", "COLORMAP" };
  int num = Ex_DynamicNumForName(name);

  // if no wads detected with PLAYPAL just crash
  if(num < 0)
    {
      W_GetNumForName(names[name]);
    }

  return W_CacheLumpNum(num, tag);
}

int Ex_ShouldKeepLump(lumpinfo_t * lump, int lumpnum, char * wadname, extra_file_t extra)
{
  if(extra == EXTRA_FILTERS)
    {
      static char * names[] = { "PLAYPAL", "TRANMAP", "COLORMAP" };
      int i = 0;

      for(i = 0 ; i < 3 ; i += 1)
        if(!strnicmp(names[i], lump->name, 8)) return 1;

      return 0;
    }
    
  if(extra == EXTRA_SELFIE)
    {
      static char * names[] = { "SELF", "DSBFG", "SS_START", "S_END", "S_START", "DEHACKED" };
      int i = 0;

      for(i = 0 ; i < 6 ; i += 1)
        {
          if(!strnicmp(names[i], lump->name, strlen(names[i]))) return 1;
        }
      return 0;
    }
  
  if(extra == EXTRA_WIMAPS)
    {
      static char * names[] = { "WI2MAP", "NRLMAP", "EVIMAP", "PLUMAP", "WISP", "WIUR" };
      int i = 0;

      for(i = 0 ; i < (gamemode == commercial ? 6 : 4); i += 1)
        {
          if(!strnicmp(names[i], lump->name, strlen(names[i]))) return 1;
        }
      return 0;
    }

  if(extra == EXTRA_JUMP)
    {
      static char * names[] = { "PISF", "PISG", "PLSS", "SS_START", "S_END", "S_START", "DEHACKED", "DSFIRXPL" };
      int i = 0;

      for(i = 0 ; i < 8 ; i += 1)
        {
          if(!strnicmp(names[i], lump->name, strlen(names[i]))) return 1;
        }
      return 0;
    }

  return 1;
}

int Ex_DynamicLumpFilterProc(lumpinfo_t * lump, int lumpnum, char * wadname, const extra_file_t extra)
{
  char * tmpname;

  if(!Ex_ShouldKeepLump(lump, lumpnum, wadname, extra)) return 0;
  
  if(extra != EXTRA_TAPE)
    {      
      int sticky = Ex_CheckNumForNameOnTape(lump->name);
      if(sticky >= 0)
        {
          lump->handle = lumpinfo[sticky]->handle;
          lump->size = lumpinfo[sticky]->size;
          lump->position = lumpinfo[sticky]->position;
          lump->data = lumpinfo[sticky]->data;
          lump->cache = lumpinfo[sticky]->cache;
        }
    }

  if(extra == EXTRA_SELFIE && !strnicmp(lump->name, "DSBFG", 8))
    {
      char *c = lump->name;
      c[2] = 'S'; c[3] = 'L'; c[4] = 'F';
      return 1;  
    }
    
  if(extra == EXTRA_JUMP)
    {
      if(!strnicmp(lump->name, "PISG", 4) || !strnicmp(lump->name, "PISF", 4) || !strnicmp(lump->name, "PLSS", 4))
        {
          char *c = lump->name;
          c[0] = 'J'; c[1] = 'M'; c[2] = 'P';
          return 1;
        }
      if(!strnicmp(lump->name, "DSFIRXPL", 8))
        {
          char *c = lump->name;
          c[2] = 'J'; c[3] = 'M'; c[4] = 'P';
          return 1;
        }
    }
  

  if(extra == EXTRA_TAPE)
    {
      tapehandle = lump->handle;
      return 1;
    }

  if(strnicmp(lump->name, "PLAYPAL", 8)) return 1;

  tmpname = strcpy(malloc(strlen(wadname) + 1), wadname);
  dyna_lump_nums[DYNA_PLAYPAL] = dyna_playpal_nums =
    realloc(dyna_playpal_nums, sizeof(int) * (playpal_wads_count + 1));
  dyna_lump_nums[DYNA_COLORMAP] = dyna_colormap_nums =
    realloc(dyna_colormap_nums, sizeof(int) * (playpal_wads_count + 1));
  dyna_lump_nums[DYNA_TRANMAP] = dyna_tranmap_nums =
    realloc(dyna_tranmap_nums, sizeof(int) * (playpal_wads_count + 1));
  dyna_playpal_wads = realloc(dyna_playpal_wads, sizeof(char *) * (playpal_wads_count + 1));
  if(extra != EXTRA_NONE)
    {
      dyna_colormap_nums[playpal_wads_count] = 0;
      dyna_tranmap_nums[playpal_wads_count] = 0;
      dyna_playpal_wads[playpal_wads_count] = tmpname;
      dyna_playpal_nums[playpal_wads_count++] = lumpnum;
    }
  else
    {
      int i;
      for(i = playpal_wads_count ; i > default_playpal_wad + 1 ; i -= 1)
        {
          dyna_playpal_nums[i] = dyna_playpal_nums[i - 1];
          dyna_playpal_wads[i] = dyna_playpal_wads[i - 1];
        }
      dyna_playpal_nums[++default_playpal_wad] = lumpnum;
      dyna_playpal_wads[default_playpal_wad] = tmpname;
      dyna_colormap_nums[default_playpal_wad] = -1;
      dyna_tranmap_nums[default_playpal_wad] = -1;
      playpal_wads_count += 1;
    }

  return 1;
}

void Ex_DynamicLumpCoalesceProc(lumpinfo_t * lump, int oldnum, int newnum)
{
  int i = 0;

  for(i = 0 ; i < playpal_wads_count ; i += 1)
    {
      int j = 0;
      for(j = 0 ; j < DYNA_TOTAL ; j += 1)
        {
          if(dyna_lump_nums[j][i] == oldnum)
            {
              dyna_lump_nums[j][i] = newnum;
              return;
            }
         }
    }
}


void Ex_DynamicLumpsInWad(int handle, int start, int count, extra_file_t extra)
{
  int i;
  int playpal = -1, colormap = -1, tranmap = -1;

  for(i = 0 ; i < count ; i += 1)
    {
      if(!strnicmp(lumpinfo[i + start]->name, "PLAYPAL", 8)) playpal = i + start;
      else if(!strnicmp(lumpinfo[i + start]->name, "COLORMAP", 8)) colormap = i + start;
      else if(!strnicmp(lumpinfo[i + start]->name, "TRANMAP", 8)) tranmap = i + start;
    }

  if(playpal >= 0)
    {
      for(i = 0 ; i < playpal_wads_count && dyna_playpal_nums[i] != playpal ; i += 1);
      if(i < playpal_wads_count)
        {
          if(colormap >= 0) dyna_colormap_nums[i] = colormap;
          else dyna_colormap_nums[i] = dyna_colormap_nums[0];

          if(tranmap >= 0) dyna_tranmap_nums[i] = tranmap;
          else dyna_tranmap_nums[i] = dyna_tranmap_nums[0];
        }
    }

  D_NewWadLumps(handle, extra);
}


////////////////////////////////////////////////////////////////////////////////////


void Ex_ResetExtraStatus()
{
  memset(extra_status, 0, sizeof(extra_status));
}
