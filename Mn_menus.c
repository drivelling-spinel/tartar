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
//--------------------------------------------------------------------------
//
// Menus
//
// the actual menus: structs and handler functions (if any)
// console commands to activate each menu
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_main.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "m_random.h"
#include "mn_engin.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"
#include "d_dialog.h" // haleyjd

// menus: all in this file (not really extern)
extern menu_t menu_newgame;
extern menu_t menu_main;
extern menu_t menu_episode;
extern menu_t menu_startmap;

// Blocky mode, has default, 0 = high, 1 = normal
//int     detailLevel;    obsolete -- killough
int screenSize;      // screen size

char *mn_phonenum;           // phone number to dial
char *mn_demoname;           // demo to play
char *mn_wadname;            // wad to load

// haleyjd: moved these up here to fix Z_Free error

// haleyjd: was 7
#define SAVESLOTS 8

char empty_slot[] = "empty slot";
char *savegamenames[SAVESLOTS];

void MN_InitMenus()
{
  int i; // haleyjd

  mn_phonenum = Z_Strdup("555-1212", PU_STATIC, 0);
  mn_demoname = Z_Strdup("demo1", PU_STATIC, 0);
  mn_wadname = Z_Strdup("", PU_STATIC, 0);

  // haleyjd: initialize via zone memory
  for(i=0; i<SAVESLOTS; i++)
  {
     savegamenames[i] = Z_Strdup("", PU_STATIC, 0);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// THE MENUS
//
//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
//
// Main Menu
//

void MN_MainMenuDrawer();

menu_t menu_main =
{
  {
    // 'doom' title drawn by the drawer
    
    {it_runcmd, "new game",             "mn_newgame",            "M_NGAME"},
    {it_runcmd, "options",              "mn_options",            "M_OPTION"},
    {it_runcmd, "load game",            "mn_loadgame",           "M_LOADG"},
    {it_runcmd, "save game",            "mn_savegame",           "M_SAVEG"},
    {it_runcmd, "quit",                 "mn_quit",               "M_QUITG"},
    {it_end},
  },
  100, 65,                // x, y offsets
  0,                     // start with 'new game' selected
  mf_skullmenu,          // a skull menu
  MN_MainMenuDrawer
};

void MN_MainMenuDrawer()
{
  // hack for m_doom compatibility
   V_DrawPatch(94, 2, 0, W_CacheLumpName("M_DOOM", PU_CACHE));
}

// mn_newgame called from main menu:
// goes to start map OR
// starts menu
// according to use_startmap, gametype and modifiedgame


CONSOLE_COMMAND(mn_newgame, 0)
{
  if(netgame && !demoplayback)
    {
      MN_Alert(s_NEWGAME);
      return;
    }

  if(gamemode == commercial || gamemission == chex)
    {
       
      // dont use new game menu if not needed
      if(!modifiedgame && gamemode == commercial
	 && W_CheckNumForName("START") >= 0
	 && use_startmap)
        {
	  if(use_startmap == -1)              // not asked yet
	    MN_StartMenu(&menu_startmap);
	  else
            {        // use start map 
	      G_DeferedInitNew(defaultskill, "START");
	      MN_ClearMenus ();
            }
        }
      else
	MN_StartMenu(&menu_newgame);
    }
  else
    {
      // hack -- cut off thy flesh consumed if not retail
      if(gamemode != retail)
        menu_episode.menuitems[5].type = it_end;
      MN_StartMenu(&menu_episode);
    }
}

// menu item to quit doom:
// pop up a quit message as in the original

CONSOLE_COMMAND(mn_quit, 0)
{
  int quitmsgnum;
  char quitmsg[128];

  if(cmdtype != c_menu && menuactive) return;

  quitmsgnum = M_Random() % 14;

  // sf: use s_QUITMSG if it has been replaced in a dehacked file
  // haleyjd 1/17/00: fixed this to work with NULL pointer derefernce
  //  fix in d_deh.c
  sprintf(quitmsg, "%s\n\n%s",
          strcmp(s_QUITMSG, "") ? s_QUITMSG : endmsg[quitmsgnum],
	  s_DOSY);
  
  MN_Question(quitmsg, "quit");

}

/////////////////////////////////////////////////////////
//
// Episode Selection
//

int start_episode = 1;

menu_t menu_episode =
{
  {
    {it_title, "which episode?",             NULL,           "M_EPISOD"},
    {it_gap},
    {it_runcmd, "auto",     "mn_episode 0",  "M_WAD"},
    {it_gap},
    {it_runcmd, "knee deep in the dead",     "mn_episode 1",  "M_EPI1"},
    {it_runcmd, "the shores of hell",        "mn_episode 2",  "M_EPI2"},
    {it_runcmd, "inferno!",                  "mn_episode 3",  "M_EPI3"},
    {it_runcmd, "thy flesh consumed",        "mn_episode 4",  "M_EPI4"},
    {it_end},
  },
  40, 30,              // x, y offsets
  2,                   // select episode 1
  mf_skullmenu,        // skull menu
};

// console command to select episode

CONSOLE_COMMAND(mn_episode, cf_notnet)
{
  if(!c_argc)
    {
      C_Printf("usage: episode <epinum>\n");
      return;
    }

  start_episode = atoi(c_argv[0]);

  if(!modifiedgame && !start_episode)
    {
      start_episode = 1;
    }

  if(gamemode == shareware && start_episode > 1)
    {
      MN_Alert(s_SWSTRING);
      return;
    }

  MN_StartMenu(&menu_newgame);
}

//////////////////////////////////////////////////////////
//
// New Game Menu: Skill Level Selection
//

menu_t menu_newgame =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "choose skill level",   NULL,                   "M_SKILL"},
    {it_gap},
    {it_runcmd, "i'm too young to die", "newgame 0",            "M_JKILL"},
    {it_runcmd, "hey, not too rough",   "newgame 1",            "M_ROUGH"},
    {it_runcmd, "hurt me plenty.",      "newgame 2",            "M_HURT"},
    {it_runcmd, "ultra-violence",       "newgame 3",            "M_ULTRA"},
    {it_runcmd, "nightmare!",           "newgame 4",            "M_NMARE"},
    {it_end},
  },
  40, 15,               // x,y offsets
  6,                    // starting item: hurt me plenty
  mf_skullmenu,         // is a skull menu
};

CONSOLE_COMMAND(newgame, cf_notnet)
{
  int skill = gameskill;
  
  // skill level is argv 0
  
  if(c_argc) skill = atoi(c_argv[0]);

  if(gamemode == commercial || (!start_episode && modifiedgame))
    {
      // start on newest level from wad
      G_DeferedInitNew(skill, firstlevel);
    }
  else
    {
      // start on first level of selected episode
      G_DeferedInitNewNum(skill, start_episode, 1);
    }

  MN_ClearMenus();
}

//////////////////////////////////////////////////
//
// First-time Query menu to use start map
//

menu_t menu_startmap =
{
  {
    {it_title,  "new game",             NULL,                   "M_NEWG"},
    {it_gap},
    {it_info,   "Eternity includes a 'start map' to let"},
    {it_info,   "you start new games from in a level."},
    {it_gap},
    {it_info,   FC_GOLD "in the future would you rather:"},
    {it_gap},
    {it_runcmd, "use the start map",            "use_startmap 1; mn_newgame"},
    {it_runcmd, "use the menu",                 "use_startmap 0; mn_newgame"},
    {it_end},
  },
  40, 15,               // x,y offsets
  7,                    // starting item: start map
  mf_leftaligned | mf_background, 
};

char *str_startmap[] = {"ask", "no", "yes"};
VARIABLE_INT(use_startmap, NULL, -1, 1, str_startmap);
CONSOLE_VARIABLE(use_startmap, use_startmap, 0) {}

/////////////////////////////////////////////////////
//
// Features Menu
//
// Access to new SMMU features
//

menu_t menu_features =
{
  {
    {it_title,  FC_GOLD "features",     NULL,                   "M_FEAT"},
    {it_runcmd, "multiplayer",          "mn_multi" },
    {it_runcmd, "load wad",             "mn_loadwad" },
    {it_runcmd, "demos",                "mn_demos" },
    {it_gap},
    {it_runcmd, "about",                "credits" },
    {it_end},
  },
  100, 1,                               // x,y
  1,                                    // start item
  mf_background | mf_leftaligned        // skull menu
};

CONSOLE_COMMAND(mn_features, 0)
{
  MN_StartMenu(&menu_features);
}

////////////////////////////////////////////////
//
// Demos Menu
//
// Allow Demo playback and (recording),
// also access to camera angles
//

menu_t menu_demos = 
{
  {
    {it_title,      FC_GOLD "demos",          NULL,             "m_demos"},
    {it_info,       FC_GOLD "play demo"},
    {it_variable,   "demo name",              "mn_demoname"},
    {it_gap},
    {it_runcmd,     "play demo",              "mn_clearmenus; playdemo %mn_demoname"},
    {it_runcmd,     "time demo",              "mn_clearmenus; timedemo %mn_demoname 1"},
    {it_runcmd,     "stop playing demo",      "mn_clearmenus; stopdemo"},
    {it_gap},
    {it_info,       FC_GOLD "cameras"},
    {it_toggle,     "viewpoint changes",      "cooldemo"},
    {it_toggle,     "chasecam",               "chasecam"},
    {it_toggle,     "walkcam",                "walkcam"},
    {it_gap},
    {it_end},
  },
  150, 1,           // x,y
  2,                // start item
  mf_background,    // full screen
};

VARIABLE_STRING(mn_demoname,     NULL,           12);
CONSOLE_VARIABLE(mn_demoname,    mn_demoname,     0) {}

CONSOLE_COMMAND(mn_demos, cf_notnet)
{
  MN_StartMenu(&menu_demos);
}

//////////////////////////////////////////////////////////////////
//
// Load new pwad menu
//
// Using SMMU dynamic wad loading
//

menu_t menu_loadwad =
{
  {
    {it_title,     FC_GOLD "load wad",    NULL,                   "M_WAD"},
    {it_info,      FC_GOLD "load wad"},
    {it_variable,  "wad name",          "mn_wadname"},
    {it_gap},
    {it_runcmd,    "load wad",          "addfile %mn_wadname; starttitle"},
    {it_end},
  },
  150, 1,                     // x,y offsets
  2,                          // starting item
  mf_background               // full screen 
};

VARIABLE_STRING(mn_wadname,     NULL,           15);
CONSOLE_VARIABLE(mn_wadname,    mn_wadname,     0) {}

CONSOLE_COMMAND(mn_loadwad, cf_notnet)
{
  if(gamemode == shareware)
    {
      MN_Alert("You must purchase the full version\n"
	       "of doom to load external .wad\n"
	       "files.\n"
	       "\n"
	       "%s", s_PRESSKEY);
      return;
    }

  MN_StartMenu(&menu_loadwad);
}

//////////////////////////////////////////////////////////////
//
// Multiplayer Menu
//
// Access to the new Multiplayer features of SMMU
//

menu_t menu_multiplayer =
{
  {
    {it_title,  FC_GOLD "multiplayer",  NULL,                   "M_MULTI"},
    {it_info,   FC_GOLD "connect:"},
    {it_runcmd, "serial/modem",         "mn_serial"},
    {it_runcmd, "tcp/ip",               "mn_tcpip"},
    {it_gap},
    {it_runcmd, "disconnect",           "disconnect"},
    {it_gap},
    {it_info,   FC_GOLD "setup"},
    {it_runcmd, "chat macros",          "mn_chatmacros"},
    {it_runcmd, "player setup",         "mn_player"},
    {it_runcmd, "game settings",        "mn_multigame"},
    {it_end},
  },
  100, 1,                                       // x,y offsets
  2,                                            // starting item
  mf_background|mf_leftaligned,                 // fullscreen
};

CONSOLE_COMMAND(mn_multi, 0)
{
  MN_StartMenu(&menu_multiplayer);
}
  
/////////////////////////////////////////////////////////////////
//
// Multiplayer Game settings
//

enum
{
  cn_menu_setupgame,
  cn_serial_answer,
  cn_serial_connect,
  // no dial: answer is server so it sets the settings
  // dial is started immediately
  cn_udp_server,
} connect_type;

menu_t menu_multigame =
{
  {
    {it_title,    FC_GOLD "multiplayer",        NULL,             "M_MULTI"},
    {it_runcmd,   "done",                       "mn_startgame"},
    {it_gap},
    {it_info,     FC_GOLD "game settings"},
    {it_toggle,   "game type",                  "deathmatch"},
    {it_variable, "starting level",             "startlevel"},
    {it_toggle,   "skill level",                "skill"},
    {it_toggle,   "no monsters",                "nomonsters"},
    {it_info,     FC_GOLD "auto-exit"},
    {it_variable, "time limit",                 "timelimit"},
    {it_variable, "frag limit",                 "fraglimit"},
    {it_gap},
    {it_runcmd,   "advanced..",                 "mn_advanced"},
    {it_end},
  },
  130, 1,
  4,                            // start
  mf_background,                // full screen
};

        // level to start on
VARIABLE_STRING(startlevel,    NULL,   8);
CONSOLE_VARIABLE(startlevel, startlevel, cf_handlerset)
{
  char *newvalue = c_argv[0];

  // check for a valid level
  if(W_CheckNumForName(newvalue) == -1)
    MN_ErrorMsg("level not found!");
  else
    {
      if(startlevel) Z_Free(startlevel);
      startlevel = Z_Strdup(newvalue, PU_STATIC, 0);
    }
}

CONSOLE_COMMAND(mn_multigame, 0)            // just setting options from menu
{
  connect_type = cn_menu_setupgame;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_ser_answer, 0)           // serial wait-for-call
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_serial_answer;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_ser_connect, 0)          // serial nullmodem
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_serial_connect;
  MN_StartMenu(&menu_multigame);
}

CONSOLE_COMMAND(mn_udpserv, 0)              // udp start server
{
  C_SetConsole();               // dont want demos interfering
  connect_type = cn_udp_server;
  MN_StartMenu(&menu_multigame);
}
        // start game
CONSOLE_COMMAND(mn_startgame, 0)
{
  char *console_cmds[] =
  {
    "mn_prevmenu",          // menu game setup
    "answer",               // cn_serial_answer
    "nullmodem",            // cn_serial_connect
    "connect",              // udp connect
  };
  
  cmdtype = c_menu;
  C_RunTextCmd(console_cmds[connect_type]);
}

/////////////////////////////////////////////////////////////////
//
// Multiplayer Game settings
// Advanced menu
//

menu_t menu_advanced =
{
  {
    {it_title,    FC_GOLD "advanced",           NULL,             "M_MULTI"},
    {it_runcmd,   "done",                       "mn_prevmenu"},
    {it_gap},
    {it_toggle,   "fast monsters",              "fast"},
    {it_toggle,   "respawning monsters",        "respawn"},
    {it_toggle,   "allow mlook",                "allowmlook"},
    {it_toggle,   "allow mlook with bfg",       "bfglook"},
    {it_toggle,   "allow autoaim",              "autoaim"},
    {it_variable, "weapon change time",         "weapspeed"},
    {it_toggle,   "variable friction",          "varfriction"},
    {it_toggle,   "boom pusher objects",        "pushers"},
    {it_toggle,   "hurting floors(slime)",      "nukage"},
    {it_end},
  },
  170, 1,
  3,                            // start
  mf_background,                // full screen
};

CONSOLE_COMMAND(mn_advanced, cf_server)
{
  MN_StartMenu(&menu_advanced);
}

/////////////////////////////////////////////////////////////////
//
// TCP/IP Menu
//
// When its done!
//

menu_t menu_tcpip =
{
  {
    {it_title,  FC_GOLD "TCP/IP",            NULL,           "M_TCPIP"},
    {it_gap},
    {it_info,   "not implemented yet. :)"},
    {it_runcmd, "",                          "mn_prevmenu"},
    {it_end},
  },
  180,1,                        // x,y offset
  3,
  mf_background,                // full-screen
};

CONSOLE_COMMAND(mn_tcpip, 0)
{
  MN_StartMenu(&menu_tcpip);
}

/////////////////////////////////////////////////////////////////
//
// Serial/Modem Game
//

menu_t menu_serial =
{
  {
    {it_title,  FC_GOLD "Serial/modem",          NULL,           "M_SERIAL"},
    {it_info,           FC_GOLD "settings"},
    {it_toggle,         "com port to use",      "com"},
    {it_variable,       "phone number",         "mn_phonenum"},
    {it_info,           FC_GOLD "connect:"},
    {it_runcmd,         "null modem link",      "mn_ser_connect"},
    {it_runcmd,         "dial",                 "dial %mn_phonenum"},
    {it_runcmd,         "wait for call",        "mn_ser_answer"},
    {it_end},
  },
  180,1,                        // x,y offset
  2,
  mf_background,                // fullscreen
};

CONSOLE_COMMAND(mn_serial, 0)
{
  MN_StartMenu(&menu_serial);
}

VARIABLE_STRING(mn_phonenum,     NULL,           126);
CONSOLE_VARIABLE(mn_phonenum,    mn_phonenum,     0) {}

/////////////////////////////////////////////////////////////////
//
// Chat Macros
//

menu_t menu_chatmacros =
{
  {
    {it_title,  FC_GOLD "chat macros",           NULL,           "M_CHATM"},
    {it_variable,       "0",            "chatmacro0"},
    {it_variable,       "1",            "chatmacro1"},
    {it_variable,       "2",            "chatmacro2"},
    {it_variable,       "3",            "chatmacro3"},
    {it_variable,       "4",            "chatmacro4"},
    {it_variable,       "5",            "chatmacro5"},
    {it_variable,       "6",            "chatmacro6"},
    {it_variable,       "7",            "chatmacro7"},
    {it_variable,       "8",            "chatmacro8"},
    {it_variable,       "9",            "chatmacro9"},
    {it_end}
  },
  20,1,                                 // x, y offset
  1,                                    // chatmacro0 at start
  mf_background,                        // full-screen
};

CONSOLE_COMMAND(mn_chatmacros, 0)
{
  MN_StartMenu(&menu_chatmacros);
}

/////////////////////////////////////////////////////////////////
//
// Player Setup
//

void MN_PlayerDrawer();

menu_t menu_player =
{
  {
    {it_title,  FC_GOLD "player setup",           NULL,           "M_PLAYER"},
    {it_variable,       "player name",          "name"},
    {it_toggle,         "player colour",        "colour"},
    {it_toggle,         "player skin",          "skin"},
    {it_toggle,         "handedness",           "lefthanded"},
    {it_end}
  },
  150,1,                                // x, y offset
  1,                                    // chatmacro0 at start
  mf_background,                        // full-screen
  MN_PlayerDrawer
};

#define SPRITEBOX_X 200
#define SPRITEBOX_Y 80

void MN_PlayerDrawer()
{
  int lump;
  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  patch_t *patch;
  
  V_DrawBox(SPRITEBOX_X, SPRITEBOX_Y, 80, 80);
  
  sprdef = &sprites[players[consoleplayer].skin->sprite];
  
  sprframe = &sprdef->spriteframes[0];
  lump = sprframe->lump[1];
  
  patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
  
  V_DrawPatchTranslated
    (
     SPRITEBOX_X + 40,
     SPRITEBOX_Y + 70,
     0,
     patch,
     players[displayplayer].colormap ?
           (char*)translationtables + 256*(players[displayplayer].colormap-1) :
           cr_red,
     -1
     );
}

CONSOLE_COMMAND(mn_player, 0)
{
  MN_StartMenu(&menu_player);
}


/////////////////////////////////////////////////////////////////
//
// Load Game
//

// haleyjd: numerous fixes here from 8-17 version of SMMU

#define SAVESTRINGSIZE  24

// load/save box patches
patch_t *patch_left = NULL;
patch_t *patch_mid;
patch_t *patch_right;

void MN_SaveGame()
{
  int save_slot = (char **)c_command->variable->variable - savegamenames;

  if(gamestate != GS_LEVEL) 
     return; // only save in level

  if(save_slot < 0 || save_slot >= SAVESLOTS)
     return; // sanity check

  G_SaveGame(save_slot, savegamenames[save_slot]);
  MN_ClearMenus();

  // haleyjd 02/23/02: restored from MBF
  if(quickSaveSlot == -2)
     quickSaveSlot = save_slot;
}

// create the savegame console commands
void MN_CreateSaveCmds()
{
  // haleyjd: something about the way these commands are being created
  //          is causing the console code to free a ptr with no zone id...
  //          08/08/00: fixed -- was trying to Z_Free a string on the C
  //                    heap - see initializers above in MN_InitMenus

  int i;

  for(i=0; i<SAVESLOTS; i++)  // haleyjd
    {
      command_t *save_command;
      variable_t *save_variable;
      char tempstr[10];
      
      // create the variable first
      save_variable = Z_Malloc(sizeof(*save_variable), PU_STATIC, 0); // haleyjd
      save_variable->variable = &savegamenames[i];
      save_variable->v_default = NULL;
      save_variable->type = vt_string;      // string value
      save_variable->min = 0;
      save_variable->max = SAVESTRINGSIZE;
      save_variable->defines = NULL;

      // now the command
      save_command = Z_Malloc(sizeof(*save_command), PU_STATIC, 0); // haleyjd

      sprintf(tempstr, "savegame_%i", i);
      save_command->name = strdup(tempstr);
      save_command->type = ct_variable;
      save_command->flags = 0;
      save_command->variable = save_variable;
      save_command->handler = MN_SaveGame;
      save_command->netcmd = 0;

      (C_AddCommand)(save_command); // hook into cmdlist
    }
}


//
// MN_ReadSaveStrings
//  read the strings from the savegame files
// based on the mbf sources
//
void MN_ReadSaveStrings(void)
{
  int i;

  for (i=0; i<SAVESLOTS; i++)
    {
      char name[PATH_MAX+1];    // killough 3/22/98
      char description[SAVESTRINGSIZE]; // sf
      FILE *fp;  // killough 11/98: change to use stdio

      G_SaveGameName(name, i);

      // haleyjd: fraggle got rid of this - perhaps cause of the crash?
      //          I've re-implemented it below to try to resolve the
      //          zoneid check error -- bingo, along with new init code.
      // if(savegamenames[i])
      //  Z_Free(savegamenames[i]);

      fp = fopen(name,"rb");
      if (!fp)
	{   // Ty 03/27/98 - externalized:
          // haleyjd
          if(savegamenames[i]) Z_Free(savegamenames[i]);
          savegamenames[i] = Z_Strdup("empty slot", PU_STATIC, 0);
	  continue;
	}

      fread(description, SAVESTRINGSIZE, 1, fp);
      if(savegamenames[i]) Z_Free(savegamenames[i]);
      savegamenames[i] = Z_Strdup(description, PU_STATIC, 0);  // haleyjd
      fclose(fp);
    }
}

void MN_DrawLoadBox(int x, int y)
{
  int i;

  if(!patch_left)        // initial load
    {
      patch_left = W_CacheLumpName("M_LSLEFT", PU_STATIC);
      patch_mid = W_CacheLumpName("M_LSCNTR", PU_STATIC);
      patch_right = W_CacheLumpName("M_LSRGHT", PU_STATIC);
    }

  V_DrawPatch(x, y, 0, patch_left);
  x += patch_left->width;

  for(i=0; i<24; i++)
    {
      V_DrawPatch(x, y, 0, patch_mid);
      x += patch_mid->width;
    }

  V_DrawPatch(x, y, 0, patch_right);
}

void MN_LoadGameDrawer();

// haleyjd: all saveslot names changed to be consistent

menu_t menu_loadgame =
{
  {
    {it_title,  FC_GOLD "load game",           NULL,              "M_LGTTL"},
    {it_gap},
    {it_runcmd, "save slot 0",                 "mn_load 0"},
    {it_gap},
    {it_runcmd, "save slot 1",                 "mn_load 1"},
    {it_gap},
    {it_runcmd, "save slot 2",                 "mn_load 2"},
    {it_gap},
    {it_runcmd, "save slot 3",                 "mn_load 3"},
    {it_gap},
    {it_runcmd, "save slot 4",                 "mn_load 4"},
    {it_gap},
    {it_runcmd, "save slot 5",                 "mn_load 5"},
    {it_gap},
    {it_runcmd, "save slot 6",                 "mn_load 6"},
    {it_gap},
    // haleyjd: the previously missing slot!
    {it_runcmd, "save slot 7",                 "mn_load 7"},
    {it_end},
  },
  50, 15,                           // x, y
  2,                                // starting slot
  mf_skullmenu | mf_leftaligned,    // skull menu
  MN_LoadGameDrawer,
};


void MN_LoadGameDrawer()
{
  int i, y;

  for(i=0, y=48; i<SAVESLOTS; i++, y+=2 * M_LINE) // haleyjd
    {
      MN_DrawLoadBox(45, y);
    }
  
  // this is lame
  for(i=0, y=2; i<SAVESLOTS; i++, y+=2)  // haleyjd
    {
      menu_loadgame.menuitems[y].description =
	savegamenames[i] ? savegamenames[i] : empty_slot;
    }
}

CONSOLE_COMMAND(mn_loadgame, 0)
{
   if(netgame && !demoplayback)
   {
      MN_Alert(s_LOADNET);
      return;
   }
   
   // haleyjd 02/23/02: restored from MBF
   if(demorecording) // killough 5/26/98: exclude during demo recordings
   {
      MN_Alert("you can't load a game\n"
	       "while recording a demo!\n\n"PRESSKEY);
      return;
   }
   
   MN_ReadSaveStrings();  // get savegame descriptions
   MN_StartMenu(&menu_loadgame);
}

CONSOLE_COMMAND(mn_load, 0)
{
   char name[PATH_MAX+1];     // killough 3/22/98
   int slot;
   
   if(c_argc < 1)
      return;
   
   slot = atoi(c_argv[0]);
   
   if(savegamenames[slot] == empty_slot)
      return;     // empty slot
   
   G_SaveGameName(name, slot);
   G_LoadGame(name, slot, false);
   
   MN_ClearMenus();
}

// haleyjd 02/23/02: Quick Load -- restored from MBF and converted
// to use console commands
CONSOLE_COMMAND(quickload, 0)
{
   char tempstring[80];

   if(netgame && !demoplayback)
   {
      MN_Alert(s_QLOADNET);
      return;
   }

   if(demorecording)
   {
      MN_Alert("you can't quickload\n"
	       "while recording a demo!\n\n"PRESSKEY);
      return;
   }

   if(quickSaveSlot < 0)
   {
      MN_Alert(s_QSAVESPOT);
      return;
   }
   
   sprintf(tempstring, s_QLPROMPT, savegamenames[quickSaveSlot]);
   MN_Question(tempstring, "qload");
}

CONSOLE_COMMAND(qload, cf_hidden)
{
   char name[PATH_MAX+1];     // killough 3/22/98
   
   G_SaveGameName(name, quickSaveSlot);
   G_LoadGame(name, quickSaveSlot, false);
}

/////////////////////////////////////////////////////////////////
//
// Save Game
//

// haleyjd: fixes continue here from 8-17 build

void MN_SaveGameDrawer()
{
   int i, y;
   
   for(i = 0, y = 48; i < SAVESLOTS; i++, y += 2*M_LINE) // haleyjd
   {
      MN_DrawLoadBox(45, y);
   }
}

menu_t menu_savegame = 
{
  {
    {it_title,  FC_GOLD "save game",           NULL,              "M_SGTTL"},
    {it_gap},
    {it_variable, "",                          "savegame_0"},
    {it_gap},
    {it_variable, "",                          "savegame_1"},
    {it_gap},
    {it_variable, "",                          "savegame_2"},
    {it_gap},
    {it_variable, "",                          "savegame_3"},
    {it_gap},
    {it_variable, "",                          "savegame_4"},
    {it_gap},
    {it_variable, "",                          "savegame_5"},
    {it_gap},
    {it_variable, "",                          "savegame_6"},
    // haleyjd: missing slot
    {it_gap},
    {it_variable, "",                          "savegame_7"},
    {it_end},
  },
  50, 15,                           // x, y
  2,                                // starting slot
  mf_skullmenu | mf_leftaligned,    // skull menu
  MN_SaveGameDrawer,
};

CONSOLE_COMMAND(mn_savegame, 0)
{
   // haleyjd 02/23/02: restored from MBF
   // killough 10/6/98: allow savegames during single-player demo 
   // playback
   
   if(!usergame && (!demoplayback || netgame))
   {
      MN_Alert(s_SAVEDEAD); // Ty 03/27/98 - externalized
      return;
   }
   
   if(gamestate != GS_LEVEL || cinema_pause || currentdialog)
      return;    // only save in levels -- haleyjd: never in cinemas
   
   MN_ReadSaveStrings();
   MN_StartMenu(&menu_savegame);
}

// haleyjd 02/23/02: Quick Save -- restored from MBF, converted to
// use console commands
CONSOLE_COMMAND(quicksave, 0)
{
   char tempstring[80];

   if(!usergame && (!demoplayback || netgame))  // killough 10/98
   {
      S_StartSound(NULL, sfx_oof);
      return;
   }
   
   if(gamestate != GS_LEVEL || cinema_pause || currentdialog)
      return;
  
   if(quickSaveSlot < 0)
   {
      quickSaveSlot = -2; // means to pick a slot now
      MN_ReadSaveStrings();
      MN_StartMenu(&menu_savegame);
      return;
   }
   
   sprintf(tempstring, s_QSPROMPT, savegamenames[quickSaveSlot]);
   MN_Question(tempstring, "qsave");
}

CONSOLE_COMMAND(qsave, cf_hidden)
{
   G_SaveGame(quickSaveSlot, savegamenames[quickSaveSlot]);
}

/////////////////////////////////////////////////////////////////
//
// Options Menu
//
// Massively re-organised from the original version
//

menu_t menu_options =
{
  {
    {it_title,  FC_GOLD "options",              NULL,             "M_OPTTTL"},
    {it_info,   FC_GOLD "input"},
    {it_runcmd, "key bindings",			"mn_keybindings"},
    {it_runcmd, "mouse options",                "mn_mouse"},
    {it_info,   FC_GOLD "output"},
    {it_runcmd, "video options",                "mn_video"},
    {it_runcmd, "sound options",                "mn_sound"},
    {it_info,   FC_GOLD "game options"},
    {it_runcmd, "features",                     "mn_features"},
    {it_runcmd, "compatibility",                "mn_compat"},
    {it_runcmd, "eternity options",		"mn_etccompat"}, // haleyjd
    {it_runcmd, "enemies",                      "mn_enemies"},
    {it_runcmd, "weapons",                      "mn_weapons"},
    {it_runcmd, "end game",                     "mn_endgame"},
    {it_info,   FC_GOLD "game widgets"},
    {it_runcmd, "hud settings",                 "mn_hud"},
    {it_runcmd, "status bar",                   "mn_status"},
    {it_runcmd, "automap",                      "mn_automap"},
    {it_end},
  },
  100, 1,                               // x,y offsets
  2,                                    // starting item: first selectable
  mf_background|mf_leftaligned,         // draw background: not a skull menu
};

CONSOLE_COMMAND(mn_options, 0)
{
  MN_StartMenu(&menu_options);
}

CONSOLE_COMMAND(mn_endgame, 0)
{
  if(gamestate == GS_DEMOSCREEN) return;
  if(cmdtype != c_menu && menuactive) return;
  
  MN_Question(s_ENDGAME, "starttitle");
}

/////////////////////////////////////////////////////////////////
//
// Video Options
//

void MN_VideoModeDrawer();

menu_t menu_video =
{
  {
    {it_title,        FC_GOLD "video",                NULL, "m_video"},
    {it_info,         FC_GOLD "mode"},
    {it_toggle,       "renderer resolution",          "v_hires"},
    {it_toggle,       "scale to higher resolution",   "v_scale_hi"},
    {it_toggle,       "aspect ratio to scale to",     "v_scale_aspect"},
    {it_toggle,       "use page-flipping",            "v_page_flip"},
    {it_toggle,       "wait for retrace",             "v_retrace"},
    {it_toggle,       "show fps",                     "v_show_fps"},
    {it_slider,       "gamma correction",             "gamma"},
    
    {it_info,         FC_GOLD "rendering"},
    {it_runcmd,       "particle options",             "mn_particle"},
    {it_slider,       "screen size",                  "screensize"},
    {it_toggle,       "hom detector flashes",         "r_homflash"},
    {it_toggle,       "translucency",                 "r_trans"},
    {it_variable,     "translucency percentage",      "r_tranpct"},
    
    {it_info,         FC_GOLD "misc."},
    {it_toggle,       "screenshot format",            "shot_type"},
    {it_toggle,       "text mode startup",            "textmode_startup"},
    
    {it_end},
  },
  200, 1,               // x,y offset
  2,                    // start on first selectable
  mf_background,        // full-screen menu
  MN_VideoModeDrawer
};
                 
void MN_VideoModeDrawer()
{
  int lump;
  patch_t *patch;
  spritedef_t *sprdef;
  spriteframe_t *sprframe;

  // draw an imp fireball
  
  sprdef = &sprites[states[S_TBALL1].sprite];
  sprframe = &sprdef->spriteframes[0];
  lump = sprframe->lump[0];

  patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
  
  V_DrawBox(269, 132, 20, 20);
  V_DrawPatchTL(281, 144, 0, patch, NULL);
}

CONSOLE_COMMAND(mn_video, 0)
{
  MN_StartMenu(&menu_video);
}

menu_t menu_particles =
{
   {
      {it_title,        FC_GOLD "video",                NULL, "m_video"},
      {it_toggle,       "render particle effects",     "draw_particles"},
      {it_toggle,       "particle blood splats",       "bloodsplattype"},
      {it_toggle,       "particle bullet puffs",       "bulletpufftype"},
      {it_toggle,       "draw rocket trails",          "rocket_trails"},
      {it_toggle,       "draw grenade trails",         "grenade_trails"},
      {it_end},
   },
   200, 1,               // x,y offset
   1,                    // start on first selectable
   mf_background,        // full-screen menu
};


CONSOLE_COMMAND(mn_particle, 0)
{
   MN_StartMenu(&menu_particles);
}


/////////////////////////////////////////////////////////////////
//
// Sound Options
//

menu_t menu_sound =
{
  {
    {it_title,      FC_GOLD "sound",                NULL, "m_sound"},
    {it_info,       FC_GOLD "use setup.exe to select" },
    {it_info,       FC_GOLD "digi and midi cards"},
    {it_gap},
    {it_info,       FC_GOLD "volume"},
    {it_slider,     "sfx volume",                   "sfx_volume"},
    {it_slider,     "music volume",                 "music_volume"},
    {it_info,       FC_GOLD "setup"},
    {it_toggle,     "sound channels",               "snd_channels"},
    {it_toggle,     "force reverse stereo",         "s_flippan"},
    {it_info,       FC_GOLD "misc"},
    {it_toggle,     "pitched sounds",               "s_pitched"},
    {it_end},
  },
  180, 1,                      // x, y offset
  5,                           // first selectable
  mf_background,               // full-screen menu
};

CONSOLE_COMMAND(mn_sound, 0)
{
  MN_StartMenu(&menu_sound);
}

/////////////////////////////////////////////////////////////////
//
// Mouse Options
//

menu_t menu_mouse =
{
  {
    {it_title,      FC_GOLD "mouse",                NULL,   "m_mouse"},
      {it_toggle,     "enable mouse",                 "use_mouse"},
      {it_info,       FC_GOLD "sensitivity"},
      {it_slider,     "horizontal",                   "sens_horiz"},
      {it_slider,     "vertical",                     "sens_vert"},
      {it_info,       FC_GOLD "misc."},
      {it_toggle,     "invert mouse",                 "invertmouse"},
      {it_toggle,     "smooth turning",               "smooth_turning"},
      {it_toggle,     "enable joystick",              "use_joystick"},
      {it_info,       FC_GOLD"mouselook"},
      {it_toggle,     "always mouselook",             "alwaysmlook"},
      {it_toggle,     "stretch sky",                  "r_stretchsky"},
      {it_end},
  },
  200, 1,                       // x, y offset
  1,                            // first selectable
  mf_background,                // full-screen menu
};

CONSOLE_COMMAND(mn_mouse, 0)
{
  MN_StartMenu(&menu_mouse);
}


/////////////////////////////////////////////////////////////////
//
// HUD Settings
//

menu_t menu_hud =
{
  {
    {it_title,      FC_GOLD "hud settings",         NULL,      "m_hud"},
    {it_info,       FC_GOLD "hud messages"},
    {it_toggle,     "messages",                     "messages"},
    {it_toggle,     "message colour",               "mess_colour"},
    {it_toggle,     "messages scroll",              "mess_scrollup"},
    {it_toggle,     "message lines",                "mess_lines"},
    {it_variable,   "message time (ms)",            "mess_timer"},
    {it_toggle,     "obituaries",                   "obituaries"},
    {it_toggle,     "obituary colour",              "obcolour"},
    {it_info,       FC_GOLD "full screen display"},
    {it_toggle,     "display type",                 "hu_overlay"},
    {it_toggle,     "hide secrets",                 "hu_hidesecrets"},
    {it_info,       FC_GOLD "misc."},
    {it_toggle,     "crosshair type",               "crosshair"},
    {it_toggle,     "show frags in DM",             "show_scores"},
    {it_toggle,     "automap coords follow pointer",   "map_coords"},
    {it_end},
  },
  200, 1,                              // x,y offset
  2,
  mf_background,
};

CONSOLE_COMMAND(mn_hud, 0)
{
  MN_StartMenu(&menu_hud);
}


/////////////////////////////////////////////////////////////////
//
// Status Bar Settings
//

menu_t menu_statusbar =
{
  {
    {it_title,      FC_GOLD "status bar",           NULL,           "m_stat"},
    {it_toggle,     "numbers always red",           "st_rednum"},
    {it_toggle,     "percent sign grey",            "st_graypct"},
    {it_toggle,     "single key display",           "st_singlekey"},
    {it_info,       FC_GOLD "status bar colours"},
    {it_variable,   "ammo ok percentage",           "ammo_yellow"},
    {it_variable,   "ammo low percentage",          "ammo_red"},
    {it_variable,   "armour high percentage",       "armor_green"},
    {it_variable,   "armour ok percentage",         "armor_yellow"},
    {it_variable,   "armour low percentage",        "armor_red"},
    {it_variable,   "health high percentage",       "health_green"},
    {it_variable,   "health ok percentage",         "health_yellow"},
    {it_variable,   "health low percentage",        "health_red"},
    {it_end},
  },
  200, 1,
  1,
  mf_background,
};

CONSOLE_COMMAND(mn_status, 0)
{
  MN_StartMenu(&menu_statusbar);
}


/////////////////////////////////////////////////////////////////
//
// Automap colours
//

menu_t menu_automap = 
{
  {
    {it_title,    FC_GOLD,                        NULL,         "m_auto"},
    {it_automap,  "background colour",            "mapcolor_back"},
    {it_automap,  "walls",                        "mapcolor_wall"},
    {it_automap,  "closed door",                  "mapcolor_clsd"},
    {it_automap,  "change in floor height",       "mapcolor_fchg"},
    {it_automap,  "red door",                     "mapcolor_rdor"},
    {it_automap,  "yellow door",                  "mapcolor_ydor"},
    {it_automap,  "blue door",                    "mapcolor_bdor"},
    {it_automap,  "teleport line",                "mapcolor_tele"},
    {it_automap,  "secret",                       "mapcolor_secr"},
    {it_automap,  "exit",                         "mapcolor_exit"},
    {it_automap,  "unseen line",                  "mapcolor_unsn"},

    {it_automap,  "sprite",                       "mapcolor_sprt"},
    {it_automap,  "crosshair",                    "mapcolor_hair"},
    {it_automap,  "single player arrow",          "mapcolor_sngl"},
    {it_automap,  "friend",                       "mapcolor_frnd"},
    {it_automap,  "red key",                      "mapcolor_rkey"},
    {it_automap,  "yellow key",                   "mapcolor_ykey"},
    {it_automap,  "blue key",                     "mapcolor_bkey"},

    {it_end},
  },
  200, 1,               // x,y
  1,                    // starting item
  mf_background,        // fullscreen
};

CONSOLE_COMMAND(mn_automap, 0)
{
  MN_StartMenu(&menu_automap);
}


/////////////////////////////////////////////////////////////////
//
// Weapon Options
//

menu_t menu_weapons =
{
  {
    {it_title,      FC_GOLD "weapons",              NULL,        "m_weap"},
    {it_info,       FC_GOLD "weapon options"},
    {it_toggle,     "bfg type",                       "bfgtype"},
    {it_toggle,     "bobbing",                        "bobbing"},
    {it_toggle,     "recoil",                         "recoil"},
    {it_info,       FC_BRICK "fist/chainsaw switch"},
    {it_gap},
    {it_info,       FC_GOLD "weapon prefs."},
    {it_variable,   "1st choice",                     "weappref_1"},
    {it_variable,   "2nd choice",                     "weappref_2"},
    {it_variable,   "3rd choice",                     "weappref_3"},
    {it_variable,   "4th choice",                     "weappref_4"},
    {it_variable,   "5th choice",                     "weappref_5"},
    {it_variable,   "6th choice",                     "weappref_6"},
    {it_variable,   "7th choice",                     "weappref_7"},
    {it_variable,   "8th choice",                     "weappref_8"},
    {it_variable,   "9th choice",                     "weappref_9"},
    {it_end},
  },
  150, 1,                              // x,y offset
  2,                                   // starting item
  mf_background,                       // full screen
};

CONSOLE_COMMAND(mn_weapons, 0)
{
  MN_StartMenu(&menu_weapons);
}

/////////////////////////////////////////////////////////////////
//
// Compatibility vectors
//

menu_t menu_compat =
{
  {
    {it_title,      FC_GOLD "compatibility",        NULL,        "m_compat"},
    
    {it_toggle,   "some objects don't hang over cliffs",    "comp_dropoff"},
    {it_toggle,   "torque simulation disabled",             "comp_falloff"},
    
    {it_toggle,   "god mode isn't absolute",                "comp_god"},
    {it_toggle,   "power-up cheats have limited duration",  "comp_infcheat"},
    
    {it_toggle,   "sky unaffected by invulnerability",      "comp_skymap"},
    
    {it_toggle,   "blazing doors, double closing sound",    "comp_blazing"},
    {it_toggle,   "tagged door lighting effect off",        "comp_doorlight"},
    
    {it_toggle,   "pain elemental 20 lost soul limit",      "comp_pain"},
    {it_toggle,   "lost souls get stuck behind walls",      "comp_skull"},
    {it_toggle,   "monsters walk off lifts",                "comp_staylift"},
    {it_toggle,   "monsters get stuck to doortracks",       "comp_doorstuck"},
    {it_toggle,   "monsters don't give up pursuit",         "comp_pursuit"},
    {it_toggle,   "any monster can telefrag on map30",      "comp_telefrag"},
    {it_toggle,   "arch-vile resurrects invincible ghosts", "comp_vile"},
    
    {it_gap},
    {it_runcmd, "more..",                     "mn_compat2"},
    {it_end},
  },
  270, 1,                      // x,y
  1,                           // starting item
  mf_background,               // full screen
};

CONSOLE_COMMAND(mn_compat, 0)
{
  MN_StartMenu(&menu_compat);
}

menu_t menu_compat2 =
{
  {
    {it_title,      FC_GOLD "compatibility",        NULL,        "m_compat"},
    {it_toggle,   "zombie players can exit levels",         "comp_zombie"},
    {it_toggle,   "use doom's stairbuilding method",        "comp_stairs"},
    {it_toggle,   "use doom's floor motion behaviour",      "comp_floors"},
    {it_toggle,   "use doom's linedef trigger model",       "comp_model"},
    {it_toggle,   "linedef effects with sector tag = 0",    "comp_zerotags"},
   
    {it_end},
  },
  270, 1,                      // x,y
  1,                           // starting item
  mf_background,               // full screen
};

CONSOLE_COMMAND(mn_compat2, 0)
{
  MN_StartMenu(&menu_compat2);
}



// haleyjd: New compatibility/functionality options for Eternity
menu_t menu_etccompat =
{
  {
    {it_title, FC_GOLD "eternity options", NULL, "M_ETCOPT"},
    {it_toggle,   "use start map",                          "use_startmap"},
    {it_toggle, "terrain effects inactive", 		    "comp_terrain"},
    {it_toggle, "creatures may respawn outside map",        "comp_respawnfix"},
    {it_toggle, "falling damage inactive",                  "comp_fallingdmg"},
    {it_toggle, "colormaps applied unevenly to some walls", "comp_evenlight"},
    {it_toggle, "272 line type is WR Start Script",	    "comp_smmuline"},
    {it_toggle,   "MBF mushroom explosion frame in DEH",    "comp_mushroom"},   
    {it_end},
  },
  270, 1,               // x, y
  1,                    // starting item
  mf_background,	// full screen
};

CONSOLE_COMMAND(mn_etccompat, 0)
{
   MN_StartMenu(&menu_etccompat);
}

/////////////////////////////////////////////////////////////////
//
// Enemies
//

menu_t menu_enemies =
{
  {
    {it_title,      FC_GOLD "enemies",              NULL,      "m_enem"},
    {it_info,       FC_GOLD "monster options"},
    {it_toggle,     "monsters remember target",     "mon_remember"},
    {it_toggle,     "monster infighting",           "mon_infight"},
    {it_toggle,     "monsters back out",            "mon_backing"},
    {it_toggle,     "monsters avoid hazards",       "mon_avoid"},
    {it_toggle,     "affected by friction",         "mon_friction"},
    {it_toggle,     "climb tall stairs",            "mon_climb"},
    {it_toggle,     "no height check melee attacks",   "comp_scratch"}, // joel
    {it_toggle,     "color of blood",               "mon_bloodcolor"},
    {it_info,       FC_GOLD "mbf friend options"},
    {it_variable,   "friend distance",              "mon_distfriend"},
    {it_toggle,     "rescue dying friends",         "mon_helpfriends"},
    {it_end},
  },
  200,1,                              // x,y offset
  2,                                  // starting item
  mf_background                       // full screen
};

CONSOLE_COMMAND(mn_enemies, 0)
{
  MN_StartMenu(&menu_enemies);
}


/////////////////////////////////////////////////////////////////
//
// Framerate test
//
// Option on "video options" menu allows you to timedemo your
// computer on demo2. When you finish you are presented with
// this menu

// test framerates
int framerates[] = {2462, 1870, 2460, 698, 489};
int this_framerate;
void MN_FrameRateDrawer();

menu_t menu_framerate = 
{
  {
    {it_title,    FC_GOLD "framerate"},
    {it_gap},
    {it_info,     "this graph shows your framerate against that"},
    {it_info,     "of a fast modern computer (using the same"},
    {it_info,     "vidmode)"},
    {it_gap},
    {it_runcmd,   "ok",          "mn_prevmenu"},
    {it_gap},
    {it_end},
  },
  15, 15,                                // x, y
  6,                                     // starting item
  mf_background | mf_leftaligned,        // align left
  MN_FrameRateDrawer,
};

#define BARCOLOR 208

void MN_FrameRateDrawer()
{
  int x, y;
  int scrwidth = SCREENWIDTH << hires;
  int linelength;
  char tempstr[50];
  
  // fast computers framerate is always 3/4 of screen

  sprintf(tempstr, "your computer: %i.%i fps",
	  this_framerate/10, this_framerate%10);
  MN_WriteText(tempstr, 50, 80);
  
  y = 93 << hires;
  linelength = (3 * scrwidth * this_framerate) / (4 * framerates[0]);
  if(linelength > scrwidth) linelength = scrwidth-2;
 
  // draw your computers framerate
  for(x=0; x<linelength; x++)
    {
      *(screens[0] + y*scrwidth + x) = BARCOLOR;
      if(hires)
	*(screens[0] + (y+1)*scrwidth + x) = BARCOLOR;
    }

  sprintf(tempstr, "fast computer (k6-2 450): %i.%i fps",
          framerates[0]/10, framerates[0]%10);
  MN_WriteText(tempstr, 50, 110);

  y = 103 << hires;

  // draw my computers framerate
  for(x=0; x<(scrwidth*3)/4; x++)
    {
      *(screens[0] + y*scrwidth + x) = BARCOLOR;
      if(hires)
	*(screens[0] + (y+1)*scrwidth + x) = BARCOLOR;
    }
}

void MN_ShowFrameRate(int framerate)
{
  this_framerate = framerate;
  MN_StartMenu(&menu_framerate);
  D_StartTitle();      // user does not need to see the console
}

//------------------------------------------------------------------------
//
// Key Bindings
//

menu_t menu_keybindings =
  {
    {
        {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
	{it_runcmd,       "weapon keys",           "mn_weaponkeys"},
	{it_runcmd,       "environment",           "mn_envkeys"},
        {it_runcmd,       "extra keys",            "mn_extrakeys"},
	{it_info, FC_GOLD "basic movement"},
	{it_binding,      "move forward",          "forward"},
	{it_binding,      "move backward",         "backward"},
	{it_binding,      "run",                   "speed"},
	{it_binding,      "turn left",             "left"},
	{it_binding,      "turn right",            "right"},
	{it_binding,      "strafe on",             "strafe"},
	{it_binding,      "strafe left",           "moveleft"},
	{it_binding,      "strafe right",          "moveright"},
	{it_binding,      "180 degree turn",       "flip"},
	{it_binding,      "mlook on",              "mlook"},
	{it_binding,      "look up",               "lookup"},
	{it_binding,      "look down",             "lookdown"},
	{it_binding,      "center view",           "center"},
	{it_binding,      "use",                   "use"},
	{it_end},
    },
    150, 1,                       // x,y offsets
    1,
    mf_background, // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_keybindings, 0)
{
  MN_StartMenu(&menu_keybindings);
}

//------------------------------------------------------------------------
//
// Key Bindings: weapon keys
//

menu_t menu_weaponbindings =
  {
    {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
	{it_info, FC_GOLD "weapon keys"},
	{it_binding,      "weapon 1",              "weapon1"},
	{it_binding,      "weapon 2",              "weapon2"},
	{it_binding,      "weapon 3",              "weapon3"},
	{it_binding,      "weapon 4",              "weapon4"},
	{it_binding,      "weapon 5",              "weapon5"},
	{it_binding,      "weapon 6",              "weapon6"},
	{it_binding,      "weapon 7",              "weapon7"},
	{it_binding,      "weapon 8",              "weapon8"},
	{it_binding,      "weapon 9",              "weapon9"},
	{it_binding,      "weapon 10",             "weapon10"},
	{it_binding,      "next weapon",           "nextweapon"},
	{it_binding,      "attack/fire",           "attack"},
	{it_end},
    },
    150, 1,                        // x,y offsets
    2,
    mf_background,  // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_weaponkeys, 0)
{
  MN_StartMenu(&menu_weaponbindings);
}

//------------------------------------------------------------------------
//
// Key Bindings: Environment
//

menu_t menu_envbindings =
  {
    {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
	{it_info,         FC_GOLD "environment"},
	{it_binding,      "screen size up",        "screensize +"},
	{it_binding,      "screen size down",      "screensize -"},
	{it_binding,      "load game",             "mn_loadgame"},
	{it_binding,      "save game",             "mn_savegame"},
	{it_binding,      "volume",                "mn_sound"},
	{it_binding,      "toggle hud",            "hu_overlay /"},
	{it_binding,      "quicksave",             "quicksave"},
	{it_binding,      "end game",              "mn_endgame"},
	{it_binding,      "toggle messages",       "messages /"},
	{it_binding,      "quickload",             "quickload"},
	{it_binding,      "quit",                  "mn_quit"},
	{it_binding,      "gamma correction",      "gamma /"},
	{it_end},
    },
    150, 1,                        // x,y offsets
    2,
    mf_background,  // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_envkeys, 0)
{
  MN_StartMenu(&menu_envbindings);
}


//------------------------------------------------------------------------
//
// Key Bindings: Environment
//

menu_t menu_extrabindings =
  {
    {
      {it_title,  FC_GOLD "key bindings",          NULL,        "M_KEYBND"},
        {it_info,         FC_GOLD "extras"},
        {it_binding,      "next palette",          "pal_next"},
        {it_binding,      "prev palette",          "pal_prev"},
        {it_binding,      "selfie stick",          "selfie"},
        {it_binding,      "screenshot",            "screenshot"},
	{it_end},
    },
    150, 1,                        // x,y offsets
    1,
    mf_background,  // draw background: not a skull menu
  };

CONSOLE_COMMAND(mn_extrakeys, 0)
{
  MN_StartMenu(&menu_extrabindings);
}

void MN_AddMenus()
{
  C_AddCommand(mn_newgame);
  C_AddCommand(mn_episode);
  C_AddCommand(startlevel);
  C_AddCommand(use_startmap);

  C_AddCommand(mn_loadgame);
  C_AddCommand(mn_load);
  C_AddCommand(mn_savegame);
  
  C_AddCommand(mn_features);
  C_AddCommand(mn_loadwad);
  C_AddCommand(mn_wadname);
  C_AddCommand(mn_demos);
  C_AddCommand(mn_demoname);
  
  C_AddCommand(mn_multi);
  C_AddCommand(mn_serial);
  C_AddCommand(mn_phonenum);
  C_AddCommand(mn_tcpip);
  C_AddCommand(mn_chatmacros);
  C_AddCommand(mn_player);
  C_AddCommand(mn_advanced);
  
  // different connect types
  C_AddCommand(mn_ser_answer);
  C_AddCommand(mn_ser_connect);
  C_AddCommand(mn_udpserv);
  C_AddCommand(mn_startgame);
  C_AddCommand(mn_multigame);
  
  C_AddCommand(mn_options);
  C_AddCommand(mn_mouse);
  C_AddCommand(mn_video);
  C_AddCommand(mn_particle);  // haleyjd: particle options menu
  C_AddCommand(mn_sound);
  C_AddCommand(mn_weapons);
  C_AddCommand(mn_compat);
  C_AddCommand(mn_compat2);
  C_AddCommand(mn_etccompat); // haleyjd: new eternity options menu
  C_AddCommand(mn_enemies);
  C_AddCommand(mn_hud);
  C_AddCommand(mn_status);
  C_AddCommand(mn_automap);

  C_AddCommand(mn_keybindings);
  C_AddCommand(mn_weaponkeys);
  C_AddCommand(mn_envkeys);
  C_AddCommand(mn_extrakeys);

  C_AddCommand(newgame);

  // prompt messages
  C_AddCommand(mn_quit);
  C_AddCommand(mn_endgame);

  // haleyjd: quicksave, quickload
  C_AddCommand(quicksave);
  C_AddCommand(quickload);
  C_AddCommand(qsave);
  C_AddCommand(qload);

  MN_CreateSaveCmds();
}

// EOF
