// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Game console commands
//
// console commands controlling the game functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"
#include "f_wipe.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "mn_engin.h"
#include "m_random.h"
#include "p_inter.h"
#include "w_wad.h"

#include "s_sound.h"  // haleyjd: restored exit sounds
#include "sounds.h"   // haleyjd: restored exit sounds

extern void I_WaitVBL(int); // haleyjd: restored exit sounds

extern int automlook;
extern int invert_mouse;
extern int screenshot_pcx;
extern boolean sendpause;
extern int forwardmove[2];
extern int sidemove[2];

////////////////////////////////////////////////////////////////////////
//
// Game Commands
//

CONSOLE_COMMAND(i_error, 0)
{
  I_Error(c_args);
}

CONSOLE_COMMAND(starttitle, cf_notnet)
{
  MN_ClearMenus();         // put menu away
  D_StartTitle();
}

CONSOLE_COMMAND(endgame, cf_notnet)
{
  C_SetConsole();
}

CONSOLE_COMMAND(pause, cf_server)
{
  sendpause = true;
}

// haleyjd: Restoration of original exit behavior

int quitsounds[8] =
{
  sfx_pldeth,
  sfx_dmpain,
  sfx_popain,
  sfx_slop,
  sfx_telept,
  sfx_posit1,
  sfx_posit3,
  sfx_sgtatk
};

int quitsounds2[8] =
{
  sfx_vilact,
  sfx_getpow,
  sfx_boscub,
  sfx_slop,
  sfx_skeswg,
  sfx_kntdth,
  sfx_bspact,
  sfx_sgtatk
};

CONSOLE_COMMAND(quit, 0)
{
  // haleyjd: re-added code for playing random sound before exit
  //          fixed for portability
#ifdef DJGPP
  extern int snd_card;
  
  if((!netgame || demoplayback) && !nosfxparm && snd_card)
  {
  	if(gamemode == commercial) // doom2 sounds
  	  S_StartSound(NULL, quitsounds2[(gametic>>2)&7]);
  	else 			   // anything else
  	  S_StartSound(NULL, quitsounds[(gametic>>2)&7]);
  	I_WaitVBL(105);
  }
#endif
  
  exit(0);
}

CONSOLE_COMMAND(animshot, 0)
{
  if(!c_argc)
    {
      C_Printf(
	       "animated screenshot.\n"
	       "usage: animshot <frames>\n");
      return;
    }
  animscreenshot = atoi(c_argv[0]);
  current_height = current_target = 0;    // turn off console
}

// particle stuff
VARIABLE_BOOLEAN(drawparticles, NULL, onoff);
CONSOLE_VARIABLE(draw_particles, drawparticles, 0) {}

VARIABLE_BOOLEAN(bloodsplat_particle, NULL, onoff);
CONSOLE_VARIABLE(bloodsplattype, bloodsplat_particle, 0) {}

VARIABLE_BOOLEAN(bulletpuff_particle, NULL, onoff);
CONSOLE_VARIABLE(bulletpufftype, bulletpuff_particle, 0) {}

VARIABLE_BOOLEAN(drawrockettrails, NULL, onoff);
CONSOLE_VARIABLE(rocket_trails, drawrockettrails, 0) {}

VARIABLE_BOOLEAN(drawgrenadetrails, NULL, onoff);
CONSOLE_VARIABLE(grenade_trails, drawgrenadetrails, 0) {}

// always mlook

VARIABLE_BOOLEAN(automlook, NULL,           onoff);
CONSOLE_VARIABLE(alwaysmlook, automlook, 0) {}

// invert mouse

VARIABLE_BOOLEAN(invert_mouse, NULL,        onoff);
CONSOLE_VARIABLE(invertmouse, invert_mouse, 0) {}

// horizontal mouse sensitivity

VARIABLE_INT(mouseSensitivity_horiz, NULL, 0, 64, NULL);
CONSOLE_VARIABLE(sens_horiz, mouseSensitivity_horiz, 0) {}

// vertical mouse sensitivity

VARIABLE_INT(mouseSensitivity_vert, NULL, 0, 48, NULL);
CONSOLE_VARIABLE(sens_vert, mouseSensitivity_vert, 0) {}

// player bobbing -- haleyjd: altered to read default, use netcmd

VARIABLE_BOOLEAN(player_bobbing, &default_player_bobbing, onoff);
CONSOLE_NETVAR(bobbing, player_bobbing, cf_server, netcmd_bobbing) {}

// turbo scale

int turbo_scale = 100;
VARIABLE_INT(turbo_scale, NULL,         10, 400, NULL);
CONSOLE_VARIABLE(turbo, turbo_scale, 0)
{
  C_Printf ("turbo scale: %i%%\n",turbo_scale);
  forwardmove[0] = (0x19*turbo_scale)/100;
  forwardmove[1] = (0x32*turbo_scale)/100;
  sidemove[0] = (0x18*turbo_scale)/100;
  sidemove[1] = (0x28*turbo_scale)/100;
}

CONSOLE_NETCMD(exitlevel, cf_server|cf_level, netcmd_exitlevel)
{
  G_ExitLevel();
}

//////////////////////////////////////
//
// Demo Stuff
//

CONSOLE_COMMAND(playdemo, cf_notnet)
{
  if(W_CheckNumForName(c_argv[0]) == -1)
    {
      C_Printf("%s not found\n",c_argv[0]);
      return;
    }

  G_DeferedPlayDemo(c_argv[0]);
  singledemo = true;            // quit after one demo
}


CONSOLE_COMMAND(stopdemo, cf_notnet)
{
  G_StopDemo();
}

CONSOLE_COMMAND(timedemo, cf_notnet)
{
   if(c_argc != 2)
   {
      C_Printf("usage: timedemo demoname showmenu\n");
      return;
   }
   G_TimeDemo(c_argv[0], !!atoi(c_argv[1]));
}

// 'cool' demo

VARIABLE_BOOLEAN(cooldemo, NULL,            onoff);
CONSOLE_VARIABLE(cooldemo, cooldemo, 0) {}

///////////////////////////////////////////////////
//
// Wads
//

// load new wad
// buffered command: r_init during load

CONSOLE_COMMAND(addfile, cf_notnet|cf_buffered)
{
  D_AddNewFile(c_argv[0]);
}

// list loaded wads

CONSOLE_COMMAND(listwads, 0)
{
  D_ListWads();
}
                                     
// random seed

CONST_INT(rngseed);
CONSOLE_CONST(rngseed, rngseed);

// suicide

CONSOLE_NETCMD(kill, cf_level, netcmd_kill)
{
  mobj_t *mobj;
  int playernum;
  
  playernum = cmdsrc;
  
  mobj = players[playernum].mo;
  P_DamageMobj(mobj, NULL, mobj,
	       2*(players[playernum].health+players[playernum].armorpoints) );
  mobj->momx = mobj->momy = mobj->momz = 0;
}

// change level

CONSOLE_NETCMD(map, cf_server, netcmd_map)
{
  if(!c_argc)
    {
      C_Printf("usage: map <mapname>\n"
               "   or map <wadfile.wad>\n");
      return;
    }
  
  G_StopDemo();
  
  // check for .wad files
  // i'm not particularly a fan of this myself, but..
  
  if(strlen(c_argv[0]) > 4)
    {
      char *extension;
      extension = c_argv[0] + strlen(c_argv[0]) - 4;
      if(!strcmp(extension, ".wad"))
	{
	  if(D_AddNewFile(c_argv[0]))
	    {
	      G_InitNew(gameskill, firstlevel);
	    }
	  return;
	}
    }
  
  G_InitNew(gameskill, c_argv[0]);
}

        // player name
VARIABLE_STRING(default_name, NULL,             18);
CONSOLE_NETVAR(name, default_name, cf_handlerset, netcmd_name)
{
  int playernum;
  
  playernum = cmdsrc;
  
  strncpy(players[playernum].name, c_argv[0], 18);
  if(playernum == consoleplayer)
    {
      free(default_name);
      default_name = strdup(c_argv[0]);
    }
}

// screenshot type

char *str_pcx[] = {"bmp", "pcx"};
VARIABLE_BOOLEAN(screenshot_pcx, NULL,     str_pcx);
CONSOLE_VARIABLE(shot_type,     screenshot_pcx, 0) {}

// textmode startup

extern int textmode_startup;            // d_main.c
VARIABLE_BOOLEAN(textmode_startup, NULL,        onoff);
CONSOLE_VARIABLE(textmode_startup, textmode_startup, 0) {}

// demo insurance

extern int demo_insurance;
char *insure_str[]={"off", "on", "when recording"};
VARIABLE_INT(demo_insurance, &default_demo_insurance, 0, 2, insure_str);
CONSOLE_VARIABLE(demo_insurance, demo_insurance, cf_notnet) {}

extern int smooth_turning;
VARIABLE_BOOLEAN(smooth_turning, NULL,          onoff);
CONSOLE_VARIABLE(smooth_turning, smooth_turning, 0) {}

// haleyjd: new stuff

extern int map_point_coordinates;
VARIABLE_BOOLEAN(map_point_coordinates, NULL, onoff);
CONSOLE_VARIABLE(map_coords, map_point_coordinates, 0) {}

#ifdef DOGS
VARIABLE_INT(dogs, &default_dogs, 0, 3, NULL);
CONSOLE_VARIABLE(numhelpers, dogs, cf_notnet)
{
   int count;

   if(!c_argc)
   {
      C_Printf("usage: numhelpers <num>\n num: 0-3\n");
      return;
   }

   count = atoi(c_argv[0]);

   if(count < 0) count = 0;
   if(count > 3) count = 3;

   dogs = count; // set internal variables appropriately
}
#endif

////////////////////////////////////////////////////////////////
//
// Chat Macros
//

void G_AddChatMacros()
{
  int i;

  for(i=0; i<10; i++)
    {
      variable_t *variable;
      command_t *command;
      char tempstr[10];
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &chat_macros[i];
      variable->v_default = NULL;
      variable->type = vt_string;      // string value
      variable->min = 0;
      variable->max = 128;              // 40 chars is enough
      variable->defines = NULL;

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "chatmacro%i", i);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = 0;
      command->variable = variable;
      command->handler = NULL;
      command->netcmd = 0;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

///////////////////////////////////////////////////////////////
//
// Weapon Prefs
//

extern int weapon_preferences[2][NUMWEAPONS+1];                   

void G_SetWeapPref(int prefnum, int newvalue)
{
  int i;
  
  // find the pref which has the new value
  
  // haleyjd: ignore new weapons when appropriate
  if(prefnum >= wp_grenade && !EternityMode)
  {
	  C_Printf("Preference change ignored\n");
	  return;
  }
  
  for(i=0; i<NUMWEAPONS; i++)
    if(weapon_preferences[0][i] == newvalue) break;

  weapon_preferences[0][i] = weapon_preferences[0][prefnum];
  weapon_preferences[0][prefnum] = newvalue;
}

char *weapon_str[NUMWEAPONS] =
{"fist", "pistol", "shotgun", "chaingun", "rocket launcher", "plasma gun",
 "bfg", "chainsaw", "double shotgun", "grenade launcher"}; // haleyjd

void G_WeapPrefHandler()
{
  int prefnum = (int *)c_command->variable->variable - weapon_preferences[0];
  G_SetWeapPref(prefnum, atoi(c_argv[0]));
}

void G_AddWeapPrefs()
{
  int i;

  for(i=0; i<NUMWEAPONS; i++)   // haleyjd
    {
      variable_t *variable;
      command_t *command;
      char tempstr[13]; // haleyjd: increased size -- bug fix!
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &weapon_preferences[0][i];
      variable->v_default = NULL;
      variable->type = vt_int;
      variable->min = 1;
      variable->max = NUMWEAPONS; // haleyjd
      variable->defines = weapon_str;  // use weapon string defines

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "weappref_%i", i+1);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = cf_handlerset;
      command->variable = variable;
      command->handler = G_WeapPrefHandler;
      command->netcmd = 0;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

///////////////////////////////////////////////////////////////
//
// Compatibility vectors
//

// names given to cmds
const char *comp_strings[] =
{
  "telefrag",
  "dropoff",
  "vile",
  "pain",
  "skull",
  "blazing",
  "doorlight",
  "model",
  "god",
  "falloff",
  "floors",
  "skymap",
  "pursuit",
  "doorstuck",
  "staylift",
  "zombie",
  "stairs",
  "infcheat",
  "zerotags",
  "terrain",    // haleyjd: TerrainTypes
  "respawnfix", //          Nightmare respawn location fix
  "fallingdmg", //	    Players take falling damage
  "evenlight",  //          Even application of colormaps
  "smmuline",   //          SMMU 272 linedef bug
  "scratch",    //          02/15/02: melee z check
};

void G_AddCompat()
{
  int i;

  for(i=0; i<=comp_scratch; i++)   // haleyjd: update this regularly
    {
      variable_t *variable;
      command_t *command;
      char tempstr[32];
      
      // create the variable first
      variable = malloc(sizeof(*variable));
      variable->variable = &comp[i];
      variable->v_default = &default_comp[i];
      variable->type = vt_int;      // string value
      variable->min = 0;
      variable->max = 1;
      variable->defines = yesno;

      // now the command
      command = malloc(sizeof(*command));

      sprintf(tempstr, "comp_%s", comp_strings[i]);
      command->name = strdup(tempstr);
      command->type = ct_variable;
      command->flags = cf_server | cf_netvar;
      command->variable = variable;
      command->handler = NULL;
      command->netcmd = netcmd_comp_0 + i;

      (C_AddCommand)(command); // hook into cmdlist
    }
}

void G_AddCommands()
{
  C_AddCommand(i_error);
  C_AddCommand(starttitle);
  C_AddCommand(endgame);
  C_AddCommand(pause);
  C_AddCommand(quit);
  C_AddCommand(animshot);
  C_AddCommand(shot_type);
  C_AddCommand(alwaysmlook);
  C_AddCommand(bobbing);
  C_AddCommand(sens_vert);
  C_AddCommand(sens_horiz);
  C_AddCommand(invertmouse);
  C_AddCommand(turbo);
  C_AddCommand(playdemo);
  C_AddCommand(timedemo);
  C_AddCommand(cooldemo);
  C_AddCommand(stopdemo);
  C_AddCommand(exitlevel);
  C_AddCommand(addfile);
  C_AddCommand(listwads);
  C_AddCommand(rngseed);
  C_AddCommand(kill);
  C_AddCommand(map);
  C_AddCommand(name);
  C_AddCommand(textmode_startup);
  C_AddCommand(demo_insurance);
  C_AddCommand(smooth_turning);

  // haleyjd: new stuff
  C_AddCommand(map_coords);
#ifdef DOGS
  C_AddCommand(numhelpers);
#endif

  C_AddCommand(draw_particles);
  C_AddCommand(bloodsplattype);
  C_AddCommand(bulletpufftype);
  C_AddCommand(rocket_trails);
  C_AddCommand(grenade_trails);
  
  G_AddChatMacros();
  G_AddWeapPrefs();
  G_AddCompat();
}

// EOF