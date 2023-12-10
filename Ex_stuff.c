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


#define plyr (&players[consoleplayer])     /* the console player */


boolean selfieMode = false;
boolean commercialWiMaps;


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

  if(states2[0] == states2[1])
    {
      memcpy(states2[1] = malloc(sizeof(states)), &states, sizeof(states));
      memcpy(weaponinfo2[1] = malloc(sizeof(weaponinfo)), &weaponinfo, sizeof(weaponinfo));
    }
  if(W_AddExtraFile(filestr, EXTRA_SELFIE)) return 0;
  // another hack - we just happen to know that selfie overrides BFG sprites
  for(i = 0 ; i < NUMSTATES ; i += 1)
    {
      if(states2[1][i].sprite == SPR_BFGG) states2[1][i].sprite = SPR_SELF;
      if(states2[1][i].action == A_FirePlasma) states2[1][i].action = A_TakeSelfie;
      if(states2[1][i].action == A_BFGsound) states2[1][i].action = A_SelfieSound;
    }
  weaponinfo2[1][wp_bfg].ammo = am_noammo;
  selfieMode = true;
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
  
  if(states2[0] == states2[1])    
    {
      memcpy(states2[1] = malloc(sizeof(states)), &states, sizeof(states));
      memcpy(weaponinfo2[1] = malloc(sizeof(weaponinfo)), &weaponinfo, sizeof(weaponinfo));
    }
  if(W_AddExtraFile(filestr, EXTRA_JUMP)) return 0;
  for(i = 0 ; i < NUMSTATES ; i += 1)
    {
      if(states2[1][i].sprite == SPR_PISF) states2[1][i].sprite = SPR_JMPF;
      if(states2[1][i].sprite == SPR_PISG) states2[1][i].sprite = SPR_JMPG;
      if(states2[1][i].sprite == SPR_PLSS) states2[1][i].sprite = SPR_JMPS;
      if(states2[1][i].action == A_FirePlasma) states2[1][i].action = A_PlaceJumppad;
      if(states2[1][i].action == A_VileAttack) states2[1][i].action = A_TossUp;
    }
  weaponinfo2[1][wp_pistol].ammo = am_noammo;
  mobjinfo[MT_JUMPPAD].deathsound = sfx_jump;
  mobjinfo[MT_JUMPPAD].seesound = sfx_None;
  selfieMode = true;

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

  if(!loaded)
    commercialWiMaps = false;


  loaded += Ex_LoadWiMapsWad("intmapnr.wad");
  return loaded;
}

void Ex_DetectAndLoadExtras(void)
{
  //HACK: loading selfie.wad _after_ jumpwad.wad for a purpose
  if(Ex_DetectAndLoadFilters() + Ex_DetectAndLoadJumpwad() + Ex_DetectAndLoadSelfie() + Ex_DetectAndLoadWiMaps())
    D_ReInitWadfiles();
}

/***************************
           CONSOLE COMMANDS
 ***************************/

/******** command list *********/

CONSOLE_COMMAND(selfie, 0)
{
  if(!plyr || !selfieMode || gamestate != GS_LEVEL) return;
  
  if(!(plyr->cheats & CF_SELFIE)) {
    plyr->pendingweapon = wp_selfie;
  }
}

CONSOLE_COMMAND(pogo, 0)
{
  if(!plyr || !selfieMode || gamestate != GS_LEVEL) return;
  
  if(!(plyr->cheats & CF_SELFIE)) {
    plyr->pendingweapon = wp_pogo;
  }
}


void Ex_AddCommands()
{
   C_AddCommand(selfie);   
   C_AddCommand(pogo);   
}
