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
#include "p_skin.h"


#define EXTRA_STATES_INDEX(extra) ( (extra) == EXTRA_JUMP ? 2 : (extra) == EXTRA_SELFIE ? 1 : 0 )
#define INIT_EXTRA_STATES(extra) { int idx = EXTRA_STATES_INDEX(extra); \
  if(idx && states3[idx] == states) \
    { \
      memcpy(states3[idx] = malloc(sizeof(states)), &states, sizeof(states)); \
      memcpy(weaponinfo3[idx] = malloc(sizeof(weaponinfo)), &weaponinfo, sizeof(weaponinfo)); \
    } \
}

#ifdef ARCTIC_STUFF
#define INTIT_PRISTINE() {\
  if(!pristine_st) memcpy(pristine_st = malloc(sizeof(states)), &states[0], sizeof(states)); \
  if(!pristine_mt) memcpy(pristine_mt = malloc(sizeof(mobjinfo_t) * MT_EXTRAS), &mobjinfo[0], sizeof(mobjinfo_t) * MT_EXTRAS); \
}

#define RESTORE_PRISTINE() { \
  assert(pristine_st); \
  memcpy(&states[0], pristine_st, sizeof(states)); \
  assert(pristine_mt); \
  memcpy(&mobjinfo[0], pristine_mt, sizeof(mobjinfo_t) * MT_EXTRAS); \
}



extern state_t * pristine_st;
extern mobjinfo_t * pristine_mt;
#endif

#define plyr (&players[consoleplayer])     /* the console player */

static char *Ex_tape(void)
{
  static char *tps;      // cache results over multiple calls
  if (!tps)
    {
      int p = M_CheckParm("-shim");
      tps = p && ++p < myargc ? myargv[p] : "";
      if(!*tps)
        {
          p = M_CheckParm("-tape");
          tps = p && ++p < myargc ? myargv[p] : "";
        }
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

#ifdef ARCTIC_STUFF
static char *arctic_part1_wad = 0, *arctic_part1_deh = 0;
static char *arctic_part2_wad = 0, *arctic_part2_deh = 0;
#endif

static int Ex_DetectAndLoadSigilShims(char * filename)
{
    struct stat sbuf;
    const char * tapename = "SIGIL_II";

    ExtractFileBase(filename, filestr, sizeof(filestr) - 1);
    if(strnicmp(filestr, "SIGIL", 5))
      return 0;

    assert(strlen(D_DoomExeDir()) + strlen(tapename) + 10 <= sizeof(tapestr));
    sprintf(tapestr, "%sshims/%s", D_DoomExeDir(), tapename);
    AddDefaultExtension(tapestr, ".wad");
    if (!stat(tapestr, &sbuf)) 
      if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;
    sprintf(tapestr, "%stape/%s", D_DoomExeDir(), tapename);
    AddDefaultExtension(tapestr, ".wad");
    if (!stat(tapestr, &sbuf)) 
      if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;

    return 0;
}

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
      sprintf(tapestr, "%sshims/%s", D_DoomExeDir(), filestr);
      AddDefaultExtension(tapestr, ".wad");
      if (!stat(tapestr, &sbuf)) 
        if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;
      sprintf(tapestr, "%stape/%s", D_DoomExeDir(), filestr);
      AddDefaultExtension(tapestr, ".wad");
      if (!stat(tapestr, &sbuf)) 
        if(!W_AddExtraFile(tapestr, EXTRA_TAPE)) return 1;

      if(Ex_DetectAndLoadSigilShims(*filenames))
        return 1;
      filenames++;
    }
    
  *tapestr = 0;
  return 0;
}

void Ex_ListTapeWad()
{
  if(*tapestr)
    C_Printf(FC_GRAY "Shim loaded: " FC_RED "Lumps from shim %s will override those from all other loaded WADs\n", tapestr);
}

int Ex_FindAllFilesByExtension(char * dir, char * ext, char *** fnames)
{
  DIR *dirp;
  struct dirent * ent;
  char ** nms;
  int num = 0, i, l = strlen(dir) + 2;

  assert(strlen(ext) == 4);
 
  dirp = opendir(dir);
  while(dirp && (ent = readdir(dirp)))
    {
      if(ent->d_namlen >= 4 && !strnicmp(ext, ent->d_name + ent->d_namlen - 4, 4))
        num += 1;
    }

  if(num)
    {
      rewinddir(dirp);
      nms = malloc(sizeof(char *) * num);
      *fnames = nms;
      i = num;

      while(i && (ent = readdir(dirp)))
        if(ent->d_namlen >= 4 && !strnicmp(ext, ent->d_name + ent->d_namlen - 4, 4))
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


int Ex_FindAllWads(char * dir, char *** fnames)
{
  return Ex_FindAllFilesByExtension(dir, ".wad", fnames);
}

int Ex_FindAllDeh(char * dir, char *** fnames)
{
  return Ex_FindAllFilesByExtension(dir, ".deh", fnames);
}

int Ex_FindAllBex(char * dir, char *** fnames)
{
  return Ex_FindAllFilesByExtension(dir, ".bex", fnames);
}

typedef int (find_all_func_t)(char *, char ***);
find_all_func_t *find_all_funcs[] = { Ex_FindAllWads, Ex_FindAllDeh, Ex_FindAllBex };

static int Ex_FindSubdirWads(const char * subdir, char *** fnames)
{
  struct stat sbuf;
  *filestr = 0;
  sprintf(filestr, "%s%s", D_DoomExeDir(), subdir);

  if (!stat(filestr, &sbuf))      
    {
      if(!S_ISDIR(sbuf.st_mode)) return 0;
      return Ex_FindAllWads(filestr, fnames);     
    }

  return 0;  
}


static int Ex_DetectAndLoadSubdirWads(const char * kind, const char * subdir, extra_file_t extra)
{
  char **fnames;
  int numwads, loaded = 0;
  int i;

  numwads = Ex_FindSubdirWads(subdir, &fnames);
  *filestr = 0;

  for(i = 0 ; i < numwads ; i += 1)
    {
      if(!W_AddExtraFile(fnames[i], extra))
        {
          if(!loaded) ExtractFileBase(fnames[i], filestr, sizeof(filestr) -1);
          loaded += 1;
        }
    }

  if(loaded == 1) usermsg(FC_GRAY "%s %s loaded", filestr, kind);
  if(loaded > 1) usermsg(FC_GRAY "%s and %d other %s%s loaded",
    filestr, loaded, kind, loaded == 2 ? "" : "s");

  if(numwads)
    {
      for(i = 0 ; i < numwads ; i += 1) free(fnames[i]);
      free(fnames);
    }

  return loaded;
}

int Ex_DetectAndLoadFilters()
{
  return Ex_DetectAndLoadSubdirWads("filter", "filters", EXTRA_FILTERS);
}


int Ex_DetectAndAddSkins()
{
  char **fnames;
  int numwads, loaded = 0;
  int i;

  numwads = Ex_FindSubdirWads("skins", &fnames);
  *filestr = 0;

  for(i = 0 ; i < numwads ; i += 1)
    {
      D_AddFile(fnames[i]);
      ExtractFileBase(fnames[i], filestr, sizeof(filestr) -1);
      loaded += 1;
    }

  if(loaded == 1) usermsg(FC_GRAY "%s skin added", filestr);
  if(loaded > 1) usermsg(FC_GRAY "%s and %d other skin%s added",
    filestr, loaded, loaded == 2 ? "" : "s");

  if(numwads)
    {
      for(i = 0 ; i < numwads ; i += 1) free(fnames[i]);
      free(fnames);
    }

  return loaded;
}


int Ex_InsertAllFixesInDir(char * dirname)
{
  struct stat sbuf;
  int numfixes = 0, i, total = 0, e;
  char **fnames;

  if (!stat(dirname, &sbuf))      
    {
    
      if(!S_ISDIR(sbuf.st_mode)) return 0;
      for(e = 0 ; e < sizeof(find_all_funcs) / sizeof(*find_all_funcs) ; e += 1)
        {
          numfixes = find_all_funcs[e](dirname, &fnames);
          if(numfixes)
            {
              for(i = numfixes - 1 ; i >= 0 ; i -= 1)
              {
                D_InsertFile(fnames[i], 1); 
                free(fnames[i]);
              }
              free(fnames);
              total += numfixes;
            }
         }
    }

  return total;  
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
  C_Printf(FC_GRAY "%s\n",s_GOTSELFIE);
  
  return 1;
}

int Ex_DetectAndLoadJumpwad()
{
  struct stat sbuf;
  int i = 0;
  
  *filestr = 0;
  sprintf(filestr, "%sjumpwad.wad", D_DoomExeDir());
  if(stat(filestr, &sbuf)) return 0;
  
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
      C_Printf(FC_GRAY "Intermission maps loaded from %s\n", fname);
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

  return loaded;
}

int Ex_DetectAndLoadNerve()
{
  int loaded = 0;

  loaded += Ex_LoadWiMapsWad("intmapnr.wad");
  
  return loaded;
}


void Ex_DetectAndLoadExtras(void)
{
  int loaded = 0, total = 0;
  MARK_EXTRA_LOADED(EXTRA_FILTERS, (total += loaded = Ex_DetectAndLoadFilters(), loaded));
  MARK_EXTRA_LOADED(EXTRA_JUMP, (total += loaded = Ex_DetectAndLoadJumpwad(), loaded));
  MARK_EXTRA_LOADED(EXTRA_SELFIE, (total += loaded = Ex_DetectAndLoadSelfie(), loaded));
  // WIMAPS is special in that D_Main can signal not to load them via extra status
  if(IS_EXTRA_LOADED(EXTRA_WIMAPS))
    MARK_EXTRA_LOADED(EXTRA_WIMAPS, (total += loaded = Ex_DetectAndLoadWiMaps(), loaded));
  MARK_EXTRA_LOADED(EXTRA_NERVE, (total += loaded = Ex_DetectAndLoadNerve(), loaded));
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

int reset_palette_needed = 0;

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
  
  //initializing state tables here to avoid setting up a separate method for this
  INIT_EXTRA_STATES(EXTRA_JUMP);
  INIT_EXTRA_STATES(EXTRA_SELFIE);
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
      static char * names[] = { "PISF", "PISG", "PLSS", "SS_START", "S_END", "S_START", "DEHACKED" };
      int i = 0;

      for(i = 0 ; i < 7 ; i += 1)
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
#ifdef ARCTIC_STUFF
  // a good place to initialize from pristine values
  INTIT_PRISTINE();
#endif
}

////////////////////////////////////////////////////////////////////////////////////
// WolfenDoom stuff
////////////////////////////////////////////////////////////////////////////////////

int Ex_SetWolfendoomSkin()
{
  skin_t * skin = P_SkinForName("BJ");
  if(skin)
    P_SetSkinEx(skin, consoleplayer, true);
  return !!skin;
}

#ifdef ARCTIC_STUFF
int Ex_LoadArcticPart1()
{
  if(arctic_part1_wad && arctic_part1_deh)
    {
      if(!W_AddExtraFile(arctic_part1_wad, EXTRA_ARCTIC))
        {
          RESTORE_PRISTINE();
          D_ExtraDehackedFile(arctic_part1_deh, EXTRA_NONE);
                              // here hack to tread deh as an ordinary one
          D_ReInitWadfiles();
          MARK_EXTRA_LOADED(EXTRA_ARCTIC, false);
          return 1;
        }
    }
  return 0;
}

int Ex_LoadArcticPart2()
{
  if(arctic_part2_wad && arctic_part2_deh)
    {
      if(!W_AddExtraFile(arctic_part2_wad, EXTRA_ARCTIC))
        {
          RESTORE_PRISTINE();
          D_ExtraDehackedFile(arctic_part2_deh, EXTRA_NONE);
                              // here hack to tread deh as an ordinary one
          D_ReInitWadfiles();
          MARK_EXTRA_LOADED(EXTRA_ARCTIC, true);
          return 1;
        }
    }
  return 0;
}


void Ex_EnsureCorrectArcticPart(int lev)
{
  if(!arctic_part1_wad || !arctic_part1_deh ||
    !arctic_part2_wad || !arctic_part2_deh)
    return;

  if(lev < 20 && IS_EXTRA_LOADED(EXTRA_ARCTIC))
    {
      if(Ex_LoadArcticPart1())
        C_Printf(FC_GRAY "Loaded Arctic part 1 wads\n%s\n%s",
          arctic_part1_wad, arctic_part1_deh);
      else
        C_Printf(FC_GRAY "Failed to load Arctic part 1 wads\n%s\n%s",
          arctic_part1_wad, arctic_part1_deh);
    }
  else if(lev >= 20 && !IS_EXTRA_LOADED(EXTRA_ARCTIC))
    {
      if(Ex_LoadArcticPart2())
        C_Printf(FC_GRAY "Loaded Arctic part 2 wads\n%s\n%s",
          arctic_part2_wad, arctic_part2_deh);
      else
        C_Printf(FC_GRAY "Failed to load Arctic part 2 wads\n%s\n%s",
          arctic_part2_wad, arctic_part2_deh);
    }
}
#endif

void Ex_RevertDSStates()
{
  state_t *local_states;
  int i;
  
  for(i = 0 ; i < sizeof(states3) / sizeof(*states3) ; i += 1)
    {
      local_states = states3[i];
      local_states[S_DSGUN2].sprite = SPR_TNT1;
      local_states[S_DSGUN2].tics = 8;
  }
   
}

int Ex_InsertResWadIfMissing(const char * wadname, int index, const char * reswadname)
{
  int i = strlen(wadname); 
  struct stat sbuf;
  if(D_HasWadInWadlist(reswadname)) return 0;
  for(i = strlen(wadname) - 1 ; i >= 0 && wadname[i] != '/' && wadname[i] != '\\' ; i -= 1);
  assert(++i + strlen(reswadname) < sizeof(filestr));
  strncpy(filestr, wadname, i);
  filestr[i] = 0;
  strcat(filestr, reswadname);
  if(stat(filestr, &sbuf)) return 0;
  D_InsertFile(filestr, index);  
  return 1;
}


int Ex_CheckWadsGeneralized(const char * wadname, const int index, char * pwads[], int pwads_count, char * reswads[], int reswads_count) 
{
  int i;
  ExtractFileBase(wadname, filestr, sizeof(filestr) - 1);
  assert(strlen(filestr) + 5 <= sizeof(filestr));
  AddDefaultExtension(filestr, ".wad");
  for( i = 0 ; i < pwads_count ; i += 1)
    {
      if(!stricmp(filestr, pwads[i]))
        {
          int c = 0, j;
          for( j = 0 ; j < reswads_count; j += 1)
            c += Ex_InsertResWadIfMissing(wadname, index + c, reswads[j]);
          return c;
        }
    }

  return 0;
}

char * Ex_CheckFilePathIfExists(const char * wadname, const char * tocheck)
{
  int i = strlen(wadname); 
  struct stat sbuf;
  for(i = strlen(wadname) - 1 ; i >= 0 && wadname[i] != '/' && wadname[i] != '\\' ; i -= 1);
  assert(++i + strlen(tocheck) < sizeof(filestr));
  strncpy(filestr, wadname, i);
  filestr[i] = 0;
  strcat(filestr, tocheck);
  if(stat(filestr, &sbuf)) return 0;
  return strdup(filestr);
}

static char * WOLFDOOM_PWADS[] = {"1ST_ENC.WAD", "AFTERMTH.WAD"};
static char * WOLFDOOM_RES_WADS[] = { "WLFGFX.WAD", "WLFSND.WAD", "WLFST.WAD", "WLFTXT.WAD" };

int Ex_Check1stEncWads(const char * wadname, const int index) 
{
  return Ex_CheckWadsGeneralized(wadname, index, WOLFDOOM_PWADS, sizeof(WOLFDOOM_PWADS) / sizeof(*WOLFDOOM_PWADS),
    WOLFDOOM_RES_WADS, sizeof(WOLFDOOM_RES_WADS) / sizeof(*WOLFDOOM_RES_WADS));
}

static char * SPEAR_PWADS[] = {"SOD.WAD"};
static char * SPEAR_RES_WADS[] = { "BJ.WAD", "DISKSKIN.WAD" };

int Ex_CheckSpearWads(const char * wadname, const int index) 
{
  return Ex_CheckWadsGeneralized(wadname, index, SPEAR_PWADS, sizeof(SPEAR_PWADS) / sizeof(*SPEAR_PWADS),
    SPEAR_RES_WADS, sizeof(SPEAR_RES_WADS) / sizeof(*SPEAR_RES_WADS));
}


static char * NOCT_PWADS[] = {"CONFRONT.WAD", "TRAIL.WAD", "SECRET.WAD"};
static char * NOCT_RES_WADS[] = { "NOCTFX.WAD", "BJ.WAD", "DISKSKIN.WAD" };

int Ex_CheckNoctWads(const char * wadname, const int index) 
{
  return Ex_CheckWadsGeneralized(wadname, index, NOCT_PWADS, sizeof(NOCT_PWADS) / sizeof(*NOCT_PWADS),
    NOCT_RES_WADS, sizeof(NOCT_RES_WADS) / sizeof(*NOCT_RES_WADS));
}

static char * ORG_PWADS[] = {"FUHRER.WAD", "FAUST.WAD", "ESCAPE.WAD"};
static char * ORG_RES_WADS[] = { "ORGFX.WAD", "BJ.WAD", "DISKSKIN.WAD" };

int Ex_CheckOriginalWads(const char * wadname, const int index) 
{
  return Ex_CheckWadsGeneralized(wadname, index, ORG_PWADS, sizeof(ORG_PWADS) / sizeof(*ORG_PWADS),
    ORG_RES_WADS, sizeof(ORG_RES_WADS) / sizeof(*ORG_RES_WADS));
}

static char * ARCTIC_PWADS[] = { "GFX1.WAD", "GFX2.WAD"};
static char * ARCTIC_RES_WADS[] = { "ARCTIC.WAD", "ARC_FIX.WAD" };
static char * ARCTIC_RES_WADS2[] = { "PISTOL.WAD", "ARCTICSN.WAD" };

static char * ARCTIC_PWADS3[] = { "ARCTIC.WAD" };
static char * ARCTIC_RES_WADS3[] = { "ARC_FIX.WAD", "PISTOL.WAD", "GFX1.WAD", "ARCTICSN.WAD" };

#ifdef ARCTIC_STUFF
void Ex_CheckArcticPartWads(const char * wadname,
                            const char * wad1, const char *deh1,
                            const char * wad2, const char *deh2)
{
  char * w1, * d1, * w2, * d2;
  w1 = Ex_CheckFilePathIfExists(wadname, wad1);
  d1 = Ex_CheckFilePathIfExists(wadname, deh1);
  w2 = Ex_CheckFilePathIfExists(wadname, wad2);
  d2 = Ex_CheckFilePathIfExists(wadname, deh2);
  if(w1 && d1 && w2 && d2)
    {
      arctic_part1_wad = w1;
      arctic_part1_deh = d1;
      arctic_part2_wad = w2;
      arctic_part2_deh = d2;
    }
  else
    {
      free(w1);
      free(w2);
      free(d1);
      free(d2);
    }
}
#endif

int Ex_CheckArcticWads(const char * wadname, const int index) 
{ 

  // option 1 - GFX1 or GFX2
  int c = Ex_CheckWadsGeneralized(wadname, index, ARCTIC_PWADS, sizeof(ARCTIC_PWADS) / sizeof(*ARCTIC_PWADS),
    ARCTIC_RES_WADS, sizeof(ARCTIC_RES_WADS) / sizeof(*ARCTIC_RES_WADS));
  c += Ex_CheckWadsGeneralized(wadname, index + c + 1, ARCTIC_PWADS, sizeof(ARCTIC_PWADS) / sizeof(*ARCTIC_PWADS),
    ARCTIC_RES_WADS2, sizeof(ARCTIC_RES_WADS2) / sizeof(*ARCTIC_RES_WADS2));
  if(c != 0)
    {
#ifdef ARCTIC_STUFF
      Ex_CheckArcticPartWads(wadname, "GFX1.WAD", "ARCTIC1.DEH",
                             "GFX2.WAD", "ARCTIC2.DEH");
#endif
      return c;
    }
    
  c = Ex_CheckWadsGeneralized(wadname, index + 1, ARCTIC_PWADS3, sizeof(ARCTIC_PWADS3) / sizeof(*ARCTIC_PWADS3),
    ARCTIC_RES_WADS3, sizeof(ARCTIC_RES_WADS3) / sizeof(*ARCTIC_RES_WADS3));
#ifdef ARCTIC_STUFF
  if(c != 0) Ex_CheckArcticPartWads(wadname, "GFX1.WAD", "ARCTIC1.DEH",
                                             "GFX2.WAD", "ARCTIC2.DEH");
#endif
  return c;
}

static char * ARCTICSE_PWADS[] = { "ARCTGFX1.WAD", "ARCTGFX2.WAD"};
static char * ARCTICSE_RES_WADS[] = { "ARCTIC.WAD", "ARCTLEV.WAD" };


static char * ARCTICSE_PWADS3[] = { "ARCTIC.WAD"};
static char * ARCTICSE_RES_WADS3[] = { "ARCTLEV.WAD", "ARCTGFX1.WAD", "ARCTIC1.DEH" };

int Ex_CheckArcticSeWads(const char * wadname, const int index) 
{
  int c = Ex_CheckWadsGeneralized(wadname, index, ARCTICSE_PWADS, sizeof(ARCTICSE_PWADS) / sizeof(*ARCTICSE_PWADS),
    ARCTICSE_RES_WADS, sizeof(ARCTICSE_RES_WADS) / sizeof(*ARCTICSE_RES_WADS));
  if(c != 0)
    {
#ifdef ARCTIC_STUFF
      Ex_CheckArcticPartWads(wadname, "ARCTGFX1.WAD", "ARCTIC1.DEH",
                                      "ARCTGFX2.WAD", "ARCTIC2.DEH");
#endif
      return c;
    }

  c = Ex_CheckWadsGeneralized(wadname, index + 1, ARCTICSE_PWADS3, sizeof(ARCTICSE_PWADS3) / sizeof(*ARCTICSE_PWADS3),
    ARCTICSE_RES_WADS3, sizeof(ARCTICSE_RES_WADS3) / sizeof(*ARCTICSE_RES_WADS3));
#ifdef ARCTIC_STUFF
  if(c != 0) Ex_CheckArcticPartWads(wadname, "ARCTGFX1.WAD", "ARCTIC1.DEH",
                                             "ARCTGFX2.WAD", "ARCTIC2.DEH");
#endif
  return c;
}
 

int Ex_CheckBTSXWads(const char * wadname, const int index) 
{
  int c;
  char * cpy;

  ExtractFileBase(wadname, filestr, sizeof(filestr) - 1);
  assert(strlen(filestr) + 5 <= sizeof(filestr));
  AddDefaultExtension(filestr, ".wad");
  if(strnicmp(filestr, "BTSX_E", 6))
    return 0;
  cpy=strdup(filestr);
  cpy[strlen(cpy)-5]--;
  c = Ex_InsertResWadIfMissing(wadname, index + 1, cpy);
  if(!c)
    {
      cpy[strlen(cpy)-5] += 2;
      c = Ex_InsertResWadIfMissing(wadname, index + 1, cpy);
    }
  free(cpy);
  return c;
}

int Ex_CheckKDiKDiZDWads(const char * wadname, const int index) 
{
  int c;
  char * cpy;

  ExtractFileBase(wadname, filestr, sizeof(filestr) - 1);
  assert(strlen(filestr) + 5 <= sizeof(filestr));
  AddDefaultExtension(filestr, ".wad");
  if(strnicmp(filestr, "KDIKDI_", 7))
    return 0;
  cpy=strdup(filestr);
  cpy[strlen(cpy)-5]--;
  c = Ex_InsertResWadIfMissing(wadname, index + 1, cpy);
  if(!c)
    {
      cpy[strlen(cpy)-5] += 2;
      c = Ex_InsertResWadIfMissing(wadname, index + 1, cpy);
    }
  free(cpy);
  return c;                         
}

int Ex_IsSigilGarbageEpisode(const char * name, const char * patch)
{
  if(!stricmp(name, "SIGIL") || !stricmp(name, "SIGIL II"))
    return W_CheckNumForName(patch) < 0;
  return 0;
}

typedef int (related_wad_func_t)(const char *, const int);
related_wad_func_t *related_wad_funcs[] = { Ex_Check1stEncWads,
  Ex_CheckArcticWads, Ex_CheckArcticSeWads,
  Ex_CheckBTSXWads, Ex_CheckKDiKDiZDWads,
  Ex_CheckOriginalWads, Ex_CheckNoctWads,
  Ex_CheckSpearWads };

int Ex_InsertRelatedWads(const char * wadname, const int index)
{
  int i;
  for(i = 0 ; i < sizeof(related_wad_funcs) / sizeof(*related_wad_funcs) ; i += 1)
    {
      int j = related_wad_funcs[i](wadname, index);
      if(j) return j;
    }
  return 0;  
}
