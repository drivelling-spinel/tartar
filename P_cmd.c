// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
// Game console variables 
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>

#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "doomstat.h"

#include "d_main.h"
#include "f_wipe.h"
#include "g_game.h"
#include "m_random.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_inter.h"
#include "p_spec.h"
#include "r_draw.h"
#include "t_vari.h"
#include "m_misc.h"

/***************************************************************************
                'defines': string values for variables
***************************************************************************/

char *yesno[]={"no","yes"};
char *onoff[]={"off","on"};

char *colournames[]= {"green","indigo","brown","red","tomato","dirt","blue",
                      "gold","sea","black","purple","vomit", "pink", "cream","white"};

char *textcolours[]=
{
  FC_BRICK  "brick" FC_RED,
  FC_TAN    "tan"   FC_RED,
  FC_GRAY   "gray"  FC_RED,
  FC_GREEN  "green" FC_RED,
  FC_BROWN  "brown" FC_RED,
  FC_GOLD   "gold"  FC_RED,
  FC_RED    "red"   FC_RED,
  FC_BLUE   "blue"  FC_RED,
  FC_ORANGE "orange"FC_RED,
  FC_YELLOW "yellow"FC_RED
};

char *skills[]=
{"im too young to die", "hey not too rough", "hurt me plenty",
 "ultra violence", "nightmare"};
char *bfgtypestr[3]= {"bfg9000", "classic", "bfg11k"};
char *dmstr[] = {"co-op", "deathmatch", "altdeath", "trideath"};
char *bloodcolors[] = {"original", "blue", "green", "yellow", "auto" };
char *randomize_modes[] = {"never", "always", "auto", "no runnin" };


/*************************************************************************
        Constants
 *************************************************************************/

CONST_STRING(info_creator);
CONSOLE_CONST(creator, info_creator);


CONST_STRING(hint_bloodcolor);
CONSOLE_CONST(hint_bloodcolor, hint_bloodcolor);

CONST_STRING(hint_wolfendoom);
CONSOLE_CONST(hint_wolfendoom, hint_wolfendoom);

CONST_STRING(hint_bjskin);
CONSOLE_CONST(hint_bjskin, hint_bjskin);


CONST_INT(estimated_maps_no);
CONSOLE_CONST(estmaps, estimated_maps_no);


/*************************************************************************
                Game variables
 *************************************************************************/

// player colour

VARIABLE_INT(default_colour, NULL, 0, TRANSLATIONCOLOURS-1, colournames);
CONSOLE_NETVAR(colour, default_colour, cf_handlerset, netcmd_colour)
{
  int playernum, colour;
  
  playernum = cmdsrc;
  colour = atoi(c_argv[0]) % TRANSLATIONCOLOURS;
  
  players[playernum].colormap = colour;
  if(gamestate == GS_LEVEL)
    players[playernum].mo->colour = colour;
  
  if(playernum == consoleplayer) default_colour = colour; // typed
}

// deathmatch

VARIABLE_INT(deathmatch, NULL,                  0, 3, dmstr);
CONSOLE_NETVAR(deathmatch, deathmatch, cf_server, netcmd_deathmatch) {}

// skill level

VARIABLE_INT(gameskill, &defaultskill,          0, 4, skills);
CONSOLE_NETVAR(skill, gameskill, cf_server, netcmd_skill)
{
  startskill = gameskill = atoi(c_argv[0]);
  if(cmdsrc == consoleplayer)
    defaultskill = gameskill + 1;
}

// allow mlook

VARIABLE_BOOLEAN(allowmlook,NULL,                   onoff);
CONSOLE_NETVAR(allowmlook, allowmlook, cf_server, netcmd_allowmlook) {}

// bfg type

VARIABLE_INT(bfgtype, &default_bfgtype,         0, 2, bfgtypestr);
CONSOLE_NETVAR(bfgtype, bfgtype, cf_server, netcmd_bfgtype) {}

// autoaiming 

VARIABLE_BOOLEAN(autoaim, &default_autoaim,         onoff);
CONSOLE_NETVAR(autoaim, autoaim, cf_server, netcmd_autoaim) {}

// weapons recoil 

VARIABLE_BOOLEAN(weapon_recoil, &default_weapon_recoil, onoff);
CONSOLE_NETVAR(recoil, weapon_recoil, cf_server, netcmd_recoil) {}

// allow pushers

VARIABLE_BOOLEAN(allow_pushers, &default_allow_pushers, onoff);
CONSOLE_NETVAR(pushers, allow_pushers, cf_server, netcmd_pushers) {}

// varying friction

VARIABLE_BOOLEAN(variable_friction, &default_variable_friction, onoff);
CONSOLE_NETVAR(varfriction, variable_friction, cf_server, netcmd_varfriction){}

// enable nukage

extern int enable_nuke;         // p_spec.c
VARIABLE_BOOLEAN(enable_nuke, NULL, onoff);
CONSOLE_NETVAR(nukage, enable_nuke, cf_server, netcmd_nukage) {}

// weapon changing speed

VARIABLE_INT(weapon_speed, &default_weapon_speed, 1, 200, NULL);
CONSOLE_NETVAR(weapspeed, weapon_speed, cf_server, netcmd_weapspeed) {}

// allow mlook with bfg

char *str_bfglook[] = {"off", "on", "fixedgun"};
VARIABLE_INT(bfglook,   NULL,                   0, 2, str_bfglook);
CONSOLE_NETVAR(bfglook, bfglook, cf_server, netcmd_bfglook) {}

// 'auto exit' variables

VARIABLE_INT(levelTimeLimit,    NULL,           0, 100,         NULL);
CONSOLE_NETVAR(timelimit, levelTimeLimit, cf_server, netcmd_timelimit) {}

VARIABLE_INT(levelFragLimit,    NULL,           0, 100,         NULL);
CONSOLE_NETVAR(fraglimit, levelFragLimit, cf_server, netcmd_fraglimit) {}


  /************** monster variables ***********/

// fast monsters

VARIABLE_BOOLEAN(fastparm, NULL,                    onoff);
CONSOLE_NETVAR(fast, fastparm, cf_server, netcmd_fast)
{
  G_SetFastParms(fastparm); // killough 4/10/98: set -fast parameter correctly
}

// no monsters

VARIABLE_BOOLEAN(nomonsters, NULL,                  onoff);
CONSOLE_NETVAR(nomonsters, nomonsters, cf_server, netcmd_nomonsters) { }

// respawning monsters

VARIABLE_BOOLEAN(respawnparm, NULL,                 onoff);
CONSOLE_NETVAR(respawn, respawnparm, cf_server, netcmd_respawn) {}

// monsters remember

VARIABLE_BOOLEAN(monsters_remember, &default_monsters_remember, onoff);
CONSOLE_NETVAR(mon_remember, monsters_remember, cf_server, netcmd_monremember) {}

// infighting among monsters

VARIABLE_BOOLEAN(monster_infighting, &default_monster_infighting, onoff);
CONSOLE_NETVAR(mon_infight, monster_infighting, cf_server, netcmd_moninfight) {}

// monsters backing out

VARIABLE_BOOLEAN(monster_backing, &default_monster_backing, onoff);
CONSOLE_NETVAR(mon_backing, monster_backing, cf_server, netcmd_monbacking) {}

// monsters avoid hazards

VARIABLE_BOOLEAN(monster_avoid_hazards, &default_monster_avoid_hazards, onoff);
CONSOLE_NETVAR(mon_avoid, monster_avoid_hazards, cf_server, netcmd_monavoid) {}

// monsters affected by friction

VARIABLE_BOOLEAN(monster_friction, &default_monster_friction, onoff);
CONSOLE_NETVAR(mon_friction, monster_friction, cf_server, netcmd_monfriction) {}

// monsters climb tall steps

VARIABLE_BOOLEAN(monkeys, &default_monkeys,         onoff);
CONSOLE_NETVAR(mon_climb, monkeys, cf_server, netcmd_monclimb) {}

// help dying friends

VARIABLE_BOOLEAN(help_friends, &default_help_friends, onoff);
CONSOLE_NETVAR(mon_helpfriends, help_friends, cf_server, netcmd_monhelpfriends) {}

// distance friends keep from player

VARIABLE_INT(distfriend, &default_distfriend,   0, 1024, NULL);
CONSOLE_NETVAR(mon_distfriend, distfriend, cf_server, netcmd_mondistfriend) {}


// blood recolor
VARIABLE_INT(bloodcolor, NULL, 0, sizeof(bloodcolors)/sizeof(*bloodcolors) - 1, bloodcolors);
CONSOLE_VARIABLE(mon_bloodcolor, bloodcolor, 0) {
    prtclblood = bloodcolor;
    if(defaults_loaded) T_EnsureGlobalIntVar("_private_bloodcolor", bloodcolor);
    R_RefreshTranslationTables(bloodcolor);
    hint_bloodcolor = "";
}

// wolf3d mode
VARIABLE_BOOLEAN(wolfendoom, NULL, onoff);
CONSOLE_VARIABLE(wolfendoom, wolfendoom, 0) {
    wolf3dmode = wolfendoom;
    if(defaults_loaded) T_EnsureGlobalIntVar("_private_wolfendoom", wolfendoom);
    hint_wolfendoom = "";
}

VARIABLE_BOOLEAN(wolflooks, NULL, onoff);
CONSOLE_VARIABLE(wolflooks, wolflooks, 0) {}

VARIABLE_INT(randomize_music, NULL, 0, sizeof(randomize_modes)/sizeof(*randomize_modes) - 1, randomize_modes);
CONSOLE_VARIABLE(randmus, randomize_music, 0) {}


void P_Chase_AddCommands();
void P_Skin_AddCommands();

void P_AddCommands()
{
  C_AddCommand(creator);

  C_AddCommand(hint_bloodcolor);
  C_AddCommand(hint_wolfendoom);
  C_AddCommand(hint_bjskin);
  
  C_AddCommand(colour);
  C_AddCommand(deathmatch);
  C_AddCommand(skill);
  C_AddCommand(allowmlook);
  C_AddCommand(bfgtype);
  C_AddCommand(autoaim);
  C_AddCommand(recoil);
  C_AddCommand(pushers);
  C_AddCommand(varfriction);
  C_AddCommand(nukage);
  C_AddCommand(weapspeed);
  C_AddCommand(bfglook);
  
  C_AddCommand(fast);
  C_AddCommand(nomonsters);
  C_AddCommand(respawn);
  C_AddCommand(mon_remember);
  C_AddCommand(mon_infight);
  C_AddCommand(mon_backing);
  C_AddCommand(mon_avoid);
  C_AddCommand(mon_friction);
  C_AddCommand(mon_climb);
  C_AddCommand(mon_helpfriends);
  C_AddCommand(mon_distfriend);
  C_AddCommand(mon_bloodcolor);

  C_AddCommand(wolfendoom);
  C_AddCommand(wolflooks);

  C_AddCommand(randmus);
  C_AddCommand(estmaps);

  C_AddCommand(timelimit);
  C_AddCommand(fraglimit);
  
  P_Chase_AddCommands();
  P_Skin_AddCommands();
}

// EOF
