// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
// Level info.
//
// Under smmu, level info is stored in the level marker: ie. "mapxx"
// or "exmx" lump. This contains new info such as: the level name, music
// lump to be played, par time etc.
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "doomdef.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "w_wad.h"
#include "p_setup.h"
#include "p_info.h"
#include "p_mobj.h"
#include "sounds.h"
#include "t_script.h"
#include "z_zone.h"

// haleyjd: TODO
// Shouldn't all of this really be in a struct?

char *info_interpic;
char *info_levelname;
int  info_partime;
char *info_music;
char *info_skyname;
char *info_creator = "unknown";
char *info_levelpic;
char *info_nextlevel;
char *info_intertext;
char *info_backdrop;
char *info_weapons;
int  info_scripts;       // has the current level got scripts?

char *info_altskyname; // haleyjd : new mapinfo stuff
char *info_colormap;
char *info_lightning;
char *info_sky2name;
int  info_skydelta;
int  info_sky2delta;
char *info_nextsecret;
char *info_killfinale;
char *info_endofgame;

int  info_nobloodcolor;
int  info_wolf3d;
int  info_wolfcolor;
int  info_ghostskull;

char * info_intermusic;
char * info_endpic;

char *info_sound_swtchn; // haleyjd: environment sounds
char *info_sound_swtchx;
char *info_sound_stnmov;
char *info_sound_pstop;
char *info_sound_bdcls;
char *info_sound_bdopn;
char *info_sound_doropn;
char *info_sound_dorcls;
char *info_sound_pstart;

#ifdef BOSSACTION
char *info_bossaction_clear;
int info_bossaction_thingtype;
int info_bossaction_tag;
int info_bossaction_linespecial;
#endif

#ifdef EPISINFO
char ** info_epis_name = 0;
int * info_epis_num = 0;
char ** info_epis_pic = 0;
char ** info_epis_start = 0;
int info_epis_count = 0;

static char * curr_epis_name = 0;
static int curr_epis_num = 0;
static char * curr_epis_pic = 0;
static char * curr_epis_start = 0;
#endif


int info_enterpictime;

void P_LowerCase(char *line);
void P_StripSpaces(char *line);
static void P_RemoveEqualses(char *line);
static void P_RemoveComments(char *line);

void P_ParseInfoCmd(char *line);
void P_ParseLevelVar(char *cmd, boolean epis);
void P_ParseInfoCmd(char *line);
void P_ParseScriptLine(char *line);
void P_ClearLevelVars();
// haleyjd 12/13/01
//void P_ParseInterText(char *line);
void P_InitWeapons();

enum
{
  RT_LEVELINFO,
  RT_SCRIPT,
#ifdef EPISINFO
  RT_EPISINFO,
#endif
  RT_OTHER,
// haleyjd 12/13/01
//  RT_INTERTEXT
} readtype;

extern char finallevel[9];
extern int detect_finallevel;

void P_LoadLevelInfo(int lumpnum)
{
  char *lump;
  char *rover;
  char *startofline;

  readtype = RT_OTHER;
  P_ClearLevelVars();

  lump = W_CacheLumpNum(lumpnum, PU_STATIC);
  
  rover = startofline = lump;
  
  while(rover < lump+lumpinfo[lumpnum]->size)
    {
      if(*rover == '\n') // end of line
	{
	  *rover = 0;               // make it an end of string (0)
	  P_ParseInfoCmd(startofline);
	  startofline = rover+1; // next line
	  *rover = '\n';            // back to end of line
	}
      rover++;
    }
  Z_Free(lump);
  
  P_InitWeapons();
}

void P_ParseInfoCmd(char *line)
{
   P_CleanLine(line);
  
   if(readtype != RT_SCRIPT)       // not for scripts
   {
      P_StripSpaces(line);
      P_LowerCase(line);
      
      while(*line == ' ') 
	 line++;
      
      if(!*line) 
	 return;
      
      if((line[0] == '/' && line[1] == '/') ||     // comment
	 line[0] == '#' || line[0] == ';') return;
   }
  
   if(*line == '[')                // a new section seperator
   {
      line++;
#ifdef EPISINFO
      if(!strncmp(line, "episode info", 12))
         readtype = RT_EPISINFO;
#endif
      if(!strncmp(line, "level info", 10))
	 readtype = RT_LEVELINFO;
      
      if(!strncmp(line, "scripts", 7))
      {
	 readtype = RT_SCRIPT;
	 info_scripts = true;    // has scripts
      }
// haleyjd 12/13/01
//    if(!strncmp(line, "intertext", 9))
//	 readtype = RT_INTERTEXT;
      return;
   }
  
   switch(readtype)
   {
   case RT_LEVELINFO:
      P_ParseLevelVar(line, false);
      break;

#ifdef EPISINFO
   case RT_EPISINFO:
      P_ParseLevelVar(line, true);
      break;
#endif
   case RT_SCRIPT:
      P_ParseScriptLine(line);
      break;
// haleyjd 12/13/01      
//   case RT_INTERTEXT:
//      P_ParseInterText(line);
//      break;
      
   case RT_OTHER:
      break;
   }
}

//
//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

enum
{
  IVT_STRING,
  IVT_INT,
  IVT_END
};

typedef struct
{
  int type;
  char *name;
  void *variable;
} levelvar_t;

levelvar_t levelvars[]=
{
  {IVT_STRING,    "levelpic",     &info_levelpic},
  {IVT_STRING,    "levelname",    &info_levelname},
  {IVT_INT,       "partime",      &info_partime},
  {IVT_STRING,    "music",        &info_music},
  {IVT_STRING,    "skyname",      &info_skyname},
  {IVT_STRING,    "creator",      &info_creator},
  {IVT_STRING,    "interpic",     &info_interpic},
  {IVT_STRING,    "nextlevel",    &info_nextlevel},
  {IVT_INT,       "gravity",      &gravity},
  {IVT_STRING,    "inter-backdrop",&info_backdrop},
  {IVT_STRING,    "defaultweapons",&info_weapons},
  {IVT_STRING,    "altskyname",   &info_altskyname},
  {IVT_STRING,    "colormap",     &info_colormap},   // haleyjd
  {IVT_STRING,    "lightning",    &info_lightning},
  {IVT_STRING,    "sky2name",     &info_sky2name},
  {IVT_INT,       "skydelta",     &info_skydelta},
  {IVT_INT,       "sky2delta",    &info_sky2delta},
  {IVT_STRING,    "sound-swtchn", &info_sound_swtchn},
  {IVT_STRING,    "sound-swtchx", &info_sound_swtchx},
  {IVT_STRING,    "sound-stnmov", &info_sound_stnmov},
  {IVT_STRING,    "sound-pstop",  &info_sound_pstop},
  {IVT_STRING,    "sound-bdcls",  &info_sound_bdcls},
  {IVT_STRING,    "sound-bdopn",  &info_sound_bdopn},
  {IVT_STRING,    "sound-dorcls", &info_sound_dorcls},
  {IVT_STRING,    "sound-doropn", &info_sound_doropn},
  {IVT_STRING,    "sound-pstart", &info_sound_pstart},
  {IVT_STRING,    "nextsecret",   &info_nextsecret},
  {IVT_STRING,    "killfinale",   &info_killfinale},
  {IVT_STRING,    "endofgame",    &info_endofgame},
  {IVT_STRING,    "intertext",    &info_intertext}, // haleyjd 12/13/01
  {IVT_INT,       "nobloodcolor", &info_nobloodcolor},
  {IVT_STRING,    "intermusic",   &info_intermusic}, 
  {IVT_STRING,    "endpic",       &info_endpic}, 
#ifdef BOSSACTION
  {IVT_INT,       "bossaction-thingtype",       &info_bossaction_thingtype},
  {IVT_INT,       "bossaction-tag",             &info_bossaction_tag},
  {IVT_INT,       "bossaction-linespecial",     &info_bossaction_linespecial},
  {IVT_STRING,    "bossaction-clear",           &info_bossaction_clear}, 
#endif
  {IVT_INT,       "wolf3d",       &info_wolf3d},
  {IVT_INT,       "wolfcolor",    &info_wolfcolor},
  {IVT_INT,       "ghostskull",   &info_ghostskull},
  {IVT_INT,       "enterpictime", &info_enterpictime},
#ifdef EPISINFO
  {IVT_STRING,    "episfirstmap", &curr_epis_start},
  {IVT_INT,       "episnum",      &curr_epis_num},
  {IVT_STRING,    "epispic",      &curr_epis_pic},
  {IVT_STRING,    "episname",     &curr_epis_name},
#endif
  {IVT_END,       0,              0}


};

void P_ParseLevelVar(char *cmd, boolean epis)
{
  char varname[50];
  char *equals;
  levelvar_t* current;
  
  if(!*cmd) return;
  
  P_RemoveEqualses(cmd);
  
  // right, first find the variable name
  
  sscanf(cmd, "%s", varname);
  
  // find what it equals
  equals = cmd+strlen(varname);
  while(*equals == ' ') equals++; // cut off the leading spaces
  
  current = levelvars;
  
  while(current->type != IVT_END)
    {
      if(!strcmp(current->name, varname))
	{
          // seemingly valid levelinfo found, so reset detected finallevel
          if(!epis)
            finallevel[0] = 0;
          switch(current->type)
	    {
	    case IVT_STRING:
	      *(char**)current->variable         // +5 for safety
		= Z_Malloc(strlen(equals)+5, PU_LEVEL, NULL);
	      strcpy(*(char**)current->variable, equals);
	      break;
	      
	    case IVT_INT:
	      *(int*)current->variable = atoi(equals);
                     break;
	    }
	}
      current++;
    }
}

static void LoadDefaultSoundNames(void)
{
   info_sound_swtchn = S_sfx[sfx_swtchn].name;
   info_sound_swtchx = S_sfx[sfx_swtchx].name;
   info_sound_stnmov = S_sfx[sfx_stnmov].name;
   info_sound_pstop  = S_sfx[sfx_pstop].name;
   info_sound_bdcls  = S_sfx[sfx_bdcls].name;
   info_sound_bdopn  = S_sfx[sfx_bdopn].name;
   info_sound_doropn = S_sfx[sfx_doropn].name;
   info_sound_dorcls = S_sfx[sfx_dorcls].name;
   info_sound_pstart = S_sfx[sfx_pstart].name;
}

// clear all the level variables so that none are left over from a
// previous level

void P_ClearLevelVars()
{
  info_levelname = info_skyname = info_levelpic = "";
  info_music = "";
  info_intermusic = "";
  info_endpic = "";
  info_creator = "unknown";
  info_interpic = "INTERPIC";
  info_partime = -1;
  info_altskyname = "";         // haleyjd
  info_colormap   = "COLORMAP";
  info_lightning = info_killfinale = info_endofgame = "false";
  info_sky2name   = "-";
  info_skydelta   = 0;
  info_sky2delta  = 0;
  info_nobloodcolor = 0;
  info_wolf3d = 0;
  info_wolfcolor = woco_none;
  info_ghostskull = 0;
  info_enterpictime = -1;
#ifdef BOSSACTION
  info_bossaction_clear = "false";
  info_bossaction_thingtype = info_bossaction_tag = info_bossaction_linespecial = 0;
#endif  
  
  LoadDefaultSoundNames(); // haleyjd

  if(gamemode == commercial && isExMy(levelmapname))
  {
     static char nextlevel[10];
     info_nextlevel = nextlevel;
     
     // set the next episode
     strcpy(nextlevel, levelmapname);
     nextlevel[3] ++;
     if(nextlevel[3] > '9')  // next episode
     {
	nextlevel[3] = '1';
	nextlevel[1] ++;
     }
     info_music = levelmapname;
  }
  else
  {
     info_nextlevel = "";
     if(gamemode == commercial)
     {
        if(detect_finallevel && *finallevel)
           {
              info_endofgame = stricmp(levelmapname, finallevel) ? "false" : "true";
           }
        else
           {
              if(gamemap == 30) info_endofgame = "true";
           }
     }
  }
  // haleyjd: always empty unless set by user
  info_nextsecret = "";
  
  info_weapons = "";
  gravity = FRACUNIT;     // default gravity
  info_intertext = info_backdrop = NULL;
  
  T_ClearScripts();
  info_scripts = false;
}

//
// P_ParseScriptLine
//

// Add a line to the levelscript

void P_ParseScriptLine(char *line)
{
  int allocsize;

             // +10 for comfort
  allocsize = strlen(line) + strlen(levelscript.data) + 10;
  
  // realloc the script bigger
  levelscript.data =
    Z_Realloc(levelscript.data, allocsize, PU_LEVEL, 0);
  
  // add the new line to the current data using sprintf (ugh)
  sprintf(levelscript.data, "%s%s\n", levelscript.data, line);
}

void P_CleanLine(char *line)
{
  char *temp;
  
  for(temp=line; *temp; temp++)
    *temp = *temp<32 ? ' ' : *temp;
}

void P_LowerCase(char *line)
{
  char *temp;
  
  for(temp=line; *temp; temp++)
    *temp = tolower(*temp);
}

void P_StripSpaces(char *line)
{
  char *temp;
  
  temp = line+strlen(line)-1;
  
  while(*temp == ' ')
    {
      *temp = 0;
      temp--;
    }
}

static void P_RemoveComments(char *line)
{
  char *temp = line;
  
  while(*temp)
    {
      if(*temp=='/' && *(temp+1)=='/')
	{
	  *temp = 0; return;
	}
      temp++;
    }
}

static void P_RemoveEqualses(char *line)
{
  char *temp;
  
  temp = line;
  
  while(*temp)
    {
      if(*temp == '=')
	{
	  *temp = ' ';
	}
      temp++;
    }
}

/*
  haleyjd 12/13/01: removed in favor of better implementation
        
	  // dumbass fixed-length intertext size
#define INTERTEXTSIZE 1024

void P_ParseInterText(char *line)
{
  while(*line==' ') line++;
  if(!*line) return;
  
  if(!info_intertext)
    {
      info_intertext = Z_Malloc(INTERTEXTSIZE, PU_LEVEL, 0);
      *info_intertext = 0; // first char as the end of the string
    }
  sprintf(info_intertext, "%s%s\n", info_intertext, line);
}
*/

boolean default_weaponowned[NUMWEAPONS];

void P_InitWeapons()
{
  char *s;
  
  memset(default_weaponowned, 0, sizeof(default_weaponowned));
  
  s = info_weapons;
  
  while(*s)
    {
      switch(*s)
	{
	case '3': default_weaponowned[wp_shotgun] = true; break;
	case '4': default_weaponowned[wp_chaingun] = true; break;
	case '5': default_weaponowned[wp_missile] = true; break;
	case '6': default_weaponowned[wp_plasma] = true; break;
	case '7': default_weaponowned[wp_bfg] = true; break;
	case '8': default_weaponowned[wp_supershotgun] = true; break;
	// haleyjd: new weapons
	case '9': 
           if(EternityMode)
             default_weaponowned[wp_grenade] = true;
	   break;
	default: break;
	}
      s++;
    }
}

#ifdef EPISINFO
static int epis_num_allocated = 0;

static int P_EnsureEpisBuffer()
{
   while(info_epis_count >= epis_num_allocated)
   {
      int sz = epis_num_allocated * 2;      
      if(sz == 0)
        sz = 1;

      info_epis_name = realloc(info_epis_name, sizeof(*info_epis_name) * sz);
      info_epis_num = realloc(info_epis_num, sizeof(*info_epis_num) * sz);
      info_epis_pic = realloc(info_epis_pic, sizeof(*info_epis_pic) * sz);
      info_epis_start = realloc(info_epis_start, sizeof(*info_epis_start) * sz);

      epis_num_allocated = sz;
   }

   return epis_num_allocated;
}

void P_LoadEpisodeInfo(int lumpnum)
{
  char *lump;
  char *rover;
  char *startofline;

  readtype = RT_OTHER;

  lump = W_CacheLumpNum(lumpnum, PU_STATIC);

  curr_epis_name = "";
  curr_epis_num = 0;
  curr_epis_pic = "";
  curr_epis_start = "";
  
  rover = startofline = lump;

  while(rover < lump+lumpinfo[lumpnum]->size)
    {
      if(*rover == '\n') // end of line
	{
	  *rover = 0;               // make it an end of string (0)
	  P_ParseInfoCmd(startofline);
	  startofline = rover+1; // next line
	  *rover = '\n';            // back to end of line
	}
      rover++;
    }
  Z_Free(lump);


  if(curr_epis_num > 0 && *curr_epis_name && *curr_epis_pic && *curr_epis_start)
    {
      P_EnsureEpisBuffer();
      info_epis_num[info_epis_count] = curr_epis_num;
      info_epis_pic[info_epis_count] = curr_epis_pic;
      info_epis_start[info_epis_count] = curr_epis_start;
      info_epis_name[info_epis_count] = curr_epis_name;
      info_epis_count += 1;

      usermsg("Episode %s(%d) definition found", curr_epis_name, curr_epis_num);
    }

}


#endif

// EOF
