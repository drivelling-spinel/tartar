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
//  DOOM main program (D_DoomMain) and game loop, plus functions to
//  determine game mode (shareware, registered), parse command line
//  parameters, configure game parameters (turbo), and call the startup
//  functions.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: d_main.c,v 1.47 1998/05/16 09:16:51 killough Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>


#include "d_io.h"  // SoM 3/12/2002: moved unistd stuff into d_io.h
#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"
#include "sounds.h"
#include "z_zone.h"
#include "c_runcmd.h"
#include "c_io.h"
#include "c_net.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "mn_engin.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "p_chase.h"
#include "r_draw.h"
#include "r_main.h"
#include "d_main.h"
#include "d_deh.h"  // Ty 04/08/98 - Externalizations
#include "t_script.h"    // for FraggleScript init
#include "g_bind.h" // haleyjd
#include "d_dialog.h"
#include "ex_stuff.h"
#include "p_info.h"
#include "dstrings.h"

// DEHacked support - Ty 03/09/97
// killough 10/98:
// Add lump number as third argument, for use when filename==NULL
void ProcessDehFile(char *filename, char *outfilename, int lump);
void ProcessExtraDehFile(extra_file_t extra, char *filename, char *outfilename, int lump);

// killough 10/98: support -dehout filename
static char *D_dehout(void)
{
  static char *s;      // cache results over multiple calls
  if (!s)
    {
      int p = M_CheckParm("-dehout");
      if (!p)
	p = M_CheckParm("-bexout");
      s = p && ++p < myargc ? myargv[p] : "";
    }
  return s;
}

char **wadfiles;

// killough 10/98: preloaded files
#define MAXLOADFILES 2
char *wad_files[MAXLOADFILES], *deh_files[MAXLOADFILES];
// haleyjd: allow two auto-loaded console scripts
char *csc_files[MAXLOADFILES];

int textmode_startup = 0;  // sf: textmode_startup for old-fashioned people
int use_startmap = -1;     // default to -1 for asking in menu
int use_continue = 0;
boolean devparm;           // started game with -devparm
boolean nolfbparm;      // working -nolfb  GB 2014
boolean nopmparm;       // working -nopm   GB 2014
#ifdef CALT
boolean noasmxparm;     // working -noasmx GB 2014
#endif
boolean asmp6parm;      // working -asmp6  GB 2014
boolean safeparm;       // working -safe   GB 2014
boolean v12_compat;     // GB 2014, for v1.2 WAD stock demos
boolean v11_compat;
boolean v11detected;

#ifdef NORENDER
boolean norenderparm;
#endif

boolean nodemo;

// jff 1/24/98 add new versions of these variables to remember command line
boolean clnomonsters;   // checkparm of -nomonsters
boolean clrespawnparm;  // checkparm of -respawn
boolean clfastparm;     // checkparm of -fast
// jff 1/24/98 end definition of command line version of play mode switches

int r_blockmap = false;       // -blockmap command line

boolean nomonsters;     // working -nomonsters
boolean respawnparm;    // working -respawn
boolean fastparm;       // working -fast

boolean singletics = false; // debug flag to cancel adaptiveness

//jff 1/22/98 parms for disabling music and sound
boolean nosfxparm;
boolean nomusicparm;

//jff 4/18/98
extern boolean inhelpscreens;

skill_t startskill;
int     startepisode;
int     startmap;
char    *startlevel;
boolean autostart;
FILE    *debugfile;

boolean advancedemo;

extern boolean timingdemo, singledemo, demoplayback, fastdemo; // killough

char    wadfile[PATH_MAX+1];       // primary wad file
char    mapdir[PATH_MAX+1];        // directory of development maps
char    basedefault[PATH_MAX+1];   // default file
char    baseiwad[PATH_MAX+1];      // jff 3/23/98: iwad directory
char    basesavegame[PATH_MAX+1];  // killough 2/16/98: savegame directory

// set from iwad: level to start new games from
char firstlevel[9] = "";

// detecting
char finallevel[9] = "";
int detect_finallevel = 1;


int estimated_maps_no;


//jff 4/19/98 list of standard IWAD names
const char *const standard_iwads[]=
{
  "/hacx.wad",
  "/chex.wad",
  "/doom2f.wad",
  "/doom2.wad",
  "/plutonia.wad",
  "/tnt.wad",
  "/doom.wad",
  "/doomu.wad",
  "/doom1.wad",
  "/freedo~2.wad",
  "/freedo~1.wad",
  "/tpd190~1.wad",
  "/freedm.wad"
};
static const int nstandard_iwads = sizeof standard_iwads/sizeof*standard_iwads;

void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//

event_t events[MAXEVENTS];
int eventhead, eventtail;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(event_t *ev)
{
  events[eventhead++] = *ev;
  eventhead &= MAXEVENTS-1;
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//

void D_ProcessEvents (void)
{
  int i;
  // IF STORE DEMO, DO NOT ACCEPT INPUT
  // sf: I don't think SMMU is going to be played in any store any
  //     time soon =)
//  if (gamemode != commercial || W_CheckNumForName("map01") >= 0)
    for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
    {
      if((events+eventtail)->type == ev_keydown && (events+eventtail)->data1 == KEYD_ENTER)
        i = 0;
      if (automapactive && !menuactive)
        {
          if (consoleactive ? !C_Responder(events+eventtail) : !ST_Responder(events+eventtail))
            if (!G_Responder(events+eventtail))
              if (!MN_Responder(events+eventtail))
                 C_Responder(events+eventtail);
        }
      else if(consoleactive)
        {
          if (!C_Responder(events+eventtail))
            if (!ST_Responder(events+eventtail))
             if (!MN_Responder(events+eventtail))
               G_Responder(events+eventtail);
        }
      else 
        {
          if (!ST_Responder(events+eventtail))
            if (!MN_Responder(events+eventtail))
              if (gamestate == GS_DEMOSCREEN ? !C_Responder(events+eventtail) : !G_Responder(events+eventtail))
                 gamestate == GS_DEMOSCREEN ? G_Responder(events+eventtail) : C_Responder(events+eventtail);
        }
    }  
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw

gamestate_t    oldgamestate = -1;  // sf: globaled
gamestate_t    wipegamestate = GS_DEMOSCREEN;
void           R_ExecuteSetViewSize(void);
camera_t       *camera;
extern boolean setsizeneeded;
boolean        redrawsbar;      // sf: globaled
boolean        redrawborder;    // sf: cleaned up border redraw
boolean        redrawdlgdone;   // haleyjd: needed to clean up dialogue

void D_Display (void)
{
  if (nodrawers)                    // for comparative timing / profiling
    return;

  R_ReInitIfNeeded();

#ifdef NORENDER
  if(norenderparm)
    {
      V_FillScreen(64, FG);
      if(norender2 >= 0 && viewwidth)
        norender2 %= viewwidth;
    }
#endif
  abort_render = false;
  if (setsizeneeded)                // change the view size if needed
    {
      R_ExecuteSetViewSize();
      R_FillBackScreen();       // redraw backscreen
    }

        // save the current screen if about to wipe
        // no melting consoles
  if (gamestate != wipegamestate && wipegamestate != GS_CONSOLE)
    {
      Wipe_StartScreen();
    }

  if (inwipe || c_moving || menuactive || redrawdlgdone)
    redrawsbar = redrawborder = true;   // redraw status bar and border

  switch (gamestate)                // do buffered drawing
    {
    case GS_LEVEL:
          // see if the border needs to be initially drawn
      if(oldgamestate != GS_LEVEL)
          R_FillBackScreen ();    // draw the pattern into the back screen
      HU_Erase();

      if (automapactive)
          AM_Drawer();
      else
      {
          // see if the border needs to be updated to the screen
          if(redrawborder) R_DrawViewBorder ();    // redraw border
          R_RenderPlayerView (&players[displayplayer], camera);
      }

      ST_Drawer(ST_FS_STYLE, redrawsbar);    // killough 11/98
      HU_Drawer();
      if(currentdialog)
	 DLG_Drawer();
      break;
    case GS_INTERMISSION:
      WI_Drawer();      
      break;
    case GS_FINALE:
      F_Drawer();
      break;
    case GS_DEMOSCREEN:
      D_PageDrawer();
      HU_Drawer();
      break;
    case GS_CONSOLE:
      break;
    }

  redrawsbar = false; // reset this now
  redrawborder = false;
  redrawdlgdone = false; // haleyjd

  // clean up border stuff
  if (gamestate != oldgamestate && gamestate != GS_LEVEL)
    ST_Stop();

  oldgamestate = wipegamestate = gamestate;

  // draw pause pic
  if (paused && !walkcam_active)        // sf: not if walkcam active for
    {                                   // frads taking screenshots
      int y = 4;
      if (!automapactive)
	y += viewwindowy;
      V_DrawPatch(viewwindowx+(scaledviewwidth-68)/2,
			y,0,W_CacheLumpName ("M_PAUSE", PU_CACHE));
    }

  if (inwipe)
      Wipe_Drawer();

  // menus go directly to the screen
  MN_Drawer();         // menu is drawn even on top of everything
  C_Drawer();          // but not over console
  NetUpdate();         // send out any new accumulation
  
    //sf : now system independent
  if(v_ticker)
    V_FPSDrawer();

        // sf: wipe changed: runs alongside the rest of the game rather
        //     than in its own loop

  I_FinishUpdate ();              // page flip or blit buffer
}

//
//  DEMO LOOP
//

static int demosequence;         // killough 5/2/98: made static
static int pagetic;
static char *pagename;

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
  // killough 12/98: don't advance internal demos if a single one is 
  // being played. The only time this matters is when using -loadgame with
  // -fastdemo, -playdemo, or -timedemo, and a consistency error occurs.

  if (/*!singledemo &&*/ --pagetic < 0)
    D_AdvanceDemo();
    

  ST_TryStop();
}

//
// D_PageDrawer
//
// killough 11/98: add credits screen
//

void D_640PageDrawer(char *key);

        // titlepic checksums
#define DOOM1TITLEPIC 382248766
#define DOOM2TITLEPIC 176650962

void D_PageDrawer(void)
{
  V_FillScreen(BG_COLOR, FG);
  if (pagename)
    {
      int l = W_CheckNumForName(pagename);
      if(l == -1)
        {
	  MN_DrawCredits();        // just draw the credits instead
	  return;
        }
      
      if(hires == 1)               // check for original title screen
        {
	  long checksum = W_LumpCheckSum(l);
	  if(checksum == DOOM1TITLEPIC)
	    {
	      D_640PageDrawer("UDTTL");
	      return;
	    }
	  if(checksum == DOOM2TITLEPIC)
	    {
	      D_640PageDrawer("D2TTL");
	      return;
	    }
        }
      // otherwise draw simple 320x200 pic
      // sf: removed useless crap (purpose ???)
      {
	byte *t = W_CacheLumpNum(l, PU_CACHE);
	V_DrawPatch(0, 0, 0, (patch_t *) t);
        }
    }
  else
    MN_DrawCredits();  
}

        // sf: 640x400 title screens at satori's request
void D_640PageDrawer(char *key)
{
  char tempstr[10];
  
  // draw the patches
  
  sprintf(tempstr, "%s00", key);
  V_DrawPatchUnscaled(0, 0, 0, W_CacheLumpName(tempstr, PU_CACHE));
  
  sprintf(tempstr, "%s01", key);
  V_DrawPatchUnscaled(0, SCREENHEIGHT, 0,
		      W_CacheLumpName(tempstr, PU_CACHE));
  
  sprintf(tempstr, "%s10", key);
  V_DrawPatchUnscaled(SCREENWIDTH, 0, 0,
		      W_CacheLumpName(tempstr, PU_CACHE));
  
  sprintf(tempstr, "%s11", key);
  V_DrawPatchUnscaled(SCREENWIDTH, SCREENHEIGHT, 0,
		      W_CacheLumpName(tempstr, PU_CACHE));
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//

void D_AdvanceDemo (void)
{
  advancedemo = true;
  S_InsertSomeRandomness();
}

// killough 11/98: functions to perform demo sequences

static void D_SetPageName(char *name)
{
  pagename = name;
}

static void D_DrawTitle1(char *name)
{
  S_StartTitleMusic(mus_intro);
  pagetic = (TICRATE*170)/35;
  D_SetPageName(name);
}

static void D_DrawTitle2(char *name)
{
  S_StartTitleMusic(mus_dm2ttl);
  D_SetPageName(name);
}

// killough 11/98: tabulate demo sequences

static struct 
{
  void (*func)(char *);
  char *name;
} const demostates[][4] =
  {
    {
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle2, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
    },

    {
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
    },

    {
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
    },

    {
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "CREDIT"},
      {D_DrawTitle1,  "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {D_SetPageName, "CREDIT"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {G_DeferedPlayDemo, "demo4"},
    },

    {
      {NULL},
      {NULL},
      {NULL},
      {NULL},
    }
  };

//
// This cycles through the demo sequences.
//
// killough 11/98: made table-driven

void D_DoAdvanceDemo(void)
{
  players[consoleplayer].playerstate = PST_LIVE;  // not reborn
  advancedemo = usergame = paused = false;
  gameaction = ga_nothing;

  pagetic = TICRATE * 11;         // killough 11/98: default behavior
  gamestate = GS_DEMOSCREEN;

  if (!demostates[++demosequence][gamemode].func)
    demosequence = 0;
  demostates[demosequence][gamemode].func
    (demostates[demosequence][gamemode].name);

}

//
// D_StartTitle
//
void D_StartTitle (void)
{
  S_ResetTitleMusic();
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo();
}

void D_Reborn (void)
{
  gameaction = ga_reborn;
}


#ifdef GAMEBAR
// print title for every printed line
static char title[128];
#endif

//
// D_AddFile
//
// Rewritten by Lee Killough
//
// killough 11/98: remove limit on number of files
//

static int numwadfiles, numwadfiles_alloc;

void D_AddFile(char *file)
{
        // sf: allocate for +2 for safety
  if (numwadfiles+2 >= numwadfiles_alloc)
    wadfiles = realloc(wadfiles, (numwadfiles_alloc = numwadfiles_alloc ?
                                  numwadfiles_alloc * 2 : 8)*sizeof(char*));
  wadfiles[numwadfiles] = strdup(file); //sf: always NULL at end
  wadfiles[numwadfiles+1] = NULL;
  numwadfiles++;
}

void D_InsertFile(char *file, int index)
{
  char ** newfiles;
  int newalloc;
  newfiles = malloc((newalloc = numwadfiles_alloc ?
                        numwadfiles + 2 >= numwadfiles_alloc ?
                           numwadfiles_alloc * 2 : numwadfiles_alloc
                        : 8)*sizeof(char*));
  if(numwadfiles_alloc)
  {  
     memcpy(newfiles, wadfiles, 
        index * sizeof(char*));
     newfiles[index] = strdup(file);
     memcpy(newfiles + index + 1, wadfiles + index, 
        (numwadfiles_alloc - index - 1) * sizeof(char*));
     free(wadfiles);
  }
  else newfiles[0] = strdup(file); //sf: always NULL at end
  numwadfiles++;
  newfiles[numwadfiles] = NULL;
  wadfiles = newfiles;
  numwadfiles_alloc = newalloc;
}


boolean D_HasFileInFilelist(const char * filenae, const char *EXT)
{
  int s = strlen(filenae), i, el, nl;
  char * ext = strcpy(malloc(s + 5), filenae), * noext = strcpy(malloc(s + 1), filenae);
  boolean found = false;
  
  AddDefaultExtension(ext, EXT);
  if(s > 4 && !stricmp(filenae + s - 4, EXT)) noext[s-4] = 0;
  el = strlen(ext);
  nl = strlen(noext);
  
  for (i = 0; i<numwadfiles && !found; i++)
     {
        int l = strlen(wadfiles[i]);
        found = found || (l>=el && !strnicmp(wadfiles[i]+l-el, ext, el)
              && (l==el || wadfiles[i][l-el-1] == '/' || wadfiles[i][l-el-1] == '\\'));
        found = found || (l>=nl && !strnicmp(wadfiles[i]+l-nl, noext, nl)
              && (l==nl || wadfiles[i][l-nl-1] == '/' || wadfiles[i][l-nl-1] == '\\'));
     }

   free(ext);
   free(noext);
   return found;
}


boolean D_HasWadInWadlist(const char * wadname)
{
  return D_HasFileInFilelist(wadname, ".WAD");
}

#ifdef EPISINFO
static void D_ListEpisodes()
{
  int i = 0;

  for(i = 0; i < info_epis_count ; i += 1)
    {
      if(i == 0)
        {
          usermsg(FC_GRAY "Additional episodes found:");
        }
      usermsg(FC_GRAY " %d " FC_RED "%s", info_epis_num[i], info_epis_name[i]);
    }
}
#endif
        //sf: console command to list loaded files
void D_ListWads()
{
  int i;
  C_Printf(FC_GRAY "Loaded WADs:\n" FC_RED);
  
  for(i=0; i<numwadfiles; i++)
    C_Printf("%s\n",wadfiles[i]);

  Ex_ListTapeWad();    
}

char basedir[257];

// Return the path where the executable lies -- Lee Killough
char *D_DoomExeDir(void)
{
  static char *base;
  if (!base)        // cache multiple requests
    {
      size_t len = strlen(*myargv);
      char *p
#if 1
      ;
      basedir[0] = 0;
      if ( len>256 ) len = 256;
      p = (base = basedir) + len;
      strncpy(basedir, *myargv, len);
#else
       = (base = malloc(len+1)) + len;
      strcpy(base,*myargv);
#endif
      while (p > base && *p!='/' && *p!='\\')
	*p--=0;
    }
  return base;
}

// killough 10/98: return the name of the program the exe was invoked as
char *D_DoomExeName(void)
{
  static char *name;    // cache multiple requests
  if (!name)
    {
      char *p = *myargv + strlen(*myargv);
      int i = 0;
      while (p > *myargv && p[-1] != '/' && p[-1] != '\\' && p[-1] != ':')
	p--;
      while (p[i] && p[i] != '.')
	i++;
      strncpy(name = malloc(i+1), p, i)[i] = 0;
    }
  return name;
}

//
// CheckIWAD
//
// Verify a file is indeed tagged as an IWAD
// Scan its lumps for levelnames and return gamemode as indicated
// Detect missing wolf levels in DOOM II
//
// The filename to check is passed in iwadname, the gamemode detected is
// returned in gmode, hassec returns the presence of secret levels
//
// jff 4/19/98 Add routine to test IWAD for validity and determine
// the gamemode from it. Also note if DOOM II, whether secret levels exist
//
// killough 11/98:
// Rewritten to considerably simplify
// Added Final Doom support (thanks to Joel Murdoch)
//

static void CheckIWAD(const char *iwadname,
		      GameMode_t *gmode,
		      GameMission_t *gmission,  // joel 10/17/98 Final DOOM fix
		      boolean *hassec)
{
  FILE *fp = fopen(iwadname, "rb");
  int ud, rg, sw, cm, sc, tnt, plut, hacx, freed, v11 = 0;
  filelump_t lump;
  wadinfo_t header;
  const char *n = lump.name;


  if (!fp)
    I_Error("Can't open IWAD: %s\n",iwadname);

  // read IWAD header
  if (fread(&header, 1, sizeof header, fp) != sizeof header ||
      /*header.identification[0] != 'I' ||*/ header.identification[1] != 'W' ||
      header.identification[2] != 'A' || header.identification[3] != 'D')
    I_Error("IWAD tag not present: %s\n",iwadname);

  fseek(fp, LONG(header.infotableofs), SEEK_SET);

  // Determine game mode from levels present
  // Must be a full set for whichever mode is present
  // Lack of wolf-3d levels also detected here
	// sf: this is almost impossible to understand

  for (hacx=freed=ud=rg=sw=cm=sc=tnt=plut=0, header.numlumps = LONG(header.numlumps);
       header.numlumps && fread(&lump, sizeof lump, 1, fp); header.numlumps--)
    *n=='S' && n[1]=='T' && n[3]=='B' && n[4]=='A' && n[5]=='R' && n[6] && !n[7] ? ++v11 :
    *n=='E' && n[2]=='M' && !n[4] ?
      n[1]=='4' ? ++ud : n[1]!='1' ? rg += n[1]=='3' || n[1]=='2' : ++sw :
    *n=='M' && n[1]=='A' && n[2]=='P' && !n[5] ?
      ++cm, sc += n[3]=='3' && (n[4]=='1' || n[4]=='2') :
    *n=='C' && n[1]=='A' && n[2]=='V' && !n[7] ? ++tnt :
    *n=='M' && n[1]=='C' && !n[3] ? ++plut :
    *n=='H' && n[1]=='A' && n[2]=='C' && n[3]=='X' && n[4]=='-' ? ++hacx :
    *n=='F' && n[1]=='R' && n[2]=='E' && n[3]=='E' && n[4]=='D' && n[5]=='O' && n[6]=='O' && n[7] == 'M' && freed++;

  fclose(fp);

  *gmission = doom;
  *hassec = false;
  *gmode =
    cm >= 30 ? (*gmission = freed ? freedoom : tnt >= 4 ? pack_tnt :
		plut >= 8 ? pack_plut : doom2,
		*hassec = sc >= 2, commercial) :
    hacx ? (*gmission = cm <= 5 ? none : hacx_reg , *hassec = sc > 0 , commercial) :
    ud >= 9 ? (*gmission = freed ? freedoom : doom, retail) :
    rg >= 18 ? (v11detected = v11 > 2 , registered) :
    sw >= 9 ? (v11detected = v11 > 2, shareware) :
    indetermined;

}

// jff 4/19/98 Add routine to check a pathname for existence as
// a file or directory. If neither append .wad and check if it
// exists as a file then. Else return non-existent.

boolean WadFileStatus(char *filename,boolean *isdir)
{
  struct stat sbuf;
  int i;

  *isdir = false;                 //default is directory to false
  if (!filename || !*filename)    //if path NULL or empty, doesn't exist
    return false;

  if (!stat(filename,&sbuf))      //check for existence
    {
      *isdir=S_ISDIR(sbuf.st_mode); //if it does, set whether a dir or not
      return true;                  //return does exist
    }

  i = strlen(filename);           //get length of path
  if (i>=4)
    if(!strnicmp(filename+i-4,".wad",4))
      return false;               //if already ends in .wad, not found

  strcat(filename,".wad");        //try it with .wad added
  if (!stat(filename,&sbuf))      //if it exists then
    {
      if (S_ISDIR(sbuf.st_mode))  //but is a dir, then say we didn't find it
	return false;
      return true;                //otherwise return file found, w/ .wad added
    }
  filename[i]=0;                  //remove .wad
  return false;                   //and report doesn't exist
}

//
// FindIWADFile
//
// Search in all the usual places until an IWAD is found.
//
// The global baseiwad contains either a full IWAD file specification
// or a directory to look for an IWAD in, or the name of the IWAD desired.
//
// The global standard_iwads lists the standard IWAD names
//
// The result of search is returned in baseiwad, or set blank if none found
//
// IWAD search algorithm:
//
// Set customiwad blank
// If -iwad present set baseiwad to normalized path from -iwad parameter
//  If baseiwad is an existing file, thats it
//  If baseiwad is an existing dir, try appending all standard iwads
//  If haven't found it, and no : or / is in baseiwad,
//   append .wad if missing and set customiwad to baseiwad
//
// Look in . for customiwad if set, else all standard iwads
//
// Look in DoomExeDir. for customiwad if set, else all standard iwads
//
// If $DOOMWADDIR is an existing file
//  If customiwad is not set, thats it
//  else replace filename with customiwad, if exists thats it
// If $DOOMWADDIR is existing dir, try customiwad if set, else standard iwads
//
// If $HOME is an existing file
//  If customiwad is not set, thats it
//  else replace filename with customiwad, if exists thats it
// If $HOME is an existing dir, try customiwad if set, else standard iwads
//
// IWAD not found
//
// jff 4/19/98 Add routine to search for a standard or custom IWAD in one
// of the standard places. Returns a blank string if not found.
//
// killough 11/98: simplified, removed error-prone cut-n-pasted code
//

char *FindIWADFile(void)
{
  static const char *envvars[] = {"DOOMWADDIR", "HOME"};
  static char iwad[PATH_MAX+1], customiwad[PATH_MAX+1];
  boolean isdir=false;
  int i,j;
  char *p;

  *iwad = 0;       // default return filename to empty
  *customiwad = 0; // customiwad is blank

  //jff 3/24/98 get -iwad parm if specified else use .
  if ((i = M_CheckParm("-iwad")) && i < myargc-1)
    {
      NormalizeSlashes(strcpy(baseiwad,myargv[i+1]));
      if (WadFileStatus(strcpy(iwad,baseiwad),&isdir))
	if (!isdir)
	  return iwad;
	else
	  for (i=0;i<nstandard_iwads;i++)
	    {
	      int n = strlen(iwad);
	      strcat(iwad,standard_iwads[i]);
	      if (WadFileStatus(iwad,&isdir) && !isdir)
		return iwad;
	      iwad[n] = 0; // reset iwad length to former
	    }
      else
	if (!strchr(iwad,':') && !strchr(iwad,'/'))
	  AddDefaultExtension(strcat(strcpy(customiwad, "/"), iwad), ".wad");
    }

  for (j=0; j<2; j++)
    {
      strcpy(iwad, j ? D_DoomExeDir() : ".");
      NormalizeSlashes(iwad);

      if(devparm)       // sf: only show 'looking in' for devparm
              printf("Looking in %s\n",iwad);   // killough 8/8/98
      if (*customiwad)
	{
	  strcat(iwad,customiwad);
	  if (WadFileStatus(iwad,&isdir) && !isdir)
	    return iwad;
	}
      else
	for (i=0;i<nstandard_iwads;i++)
	  {
	    int n = strlen(iwad);
	    strcat(iwad,standard_iwads[i]);
	    if (WadFileStatus(iwad,&isdir) && !isdir)
	      return iwad;
	    iwad[n] = 0; // reset iwad length to former
	  }
    }

  for (i=0; i<sizeof envvars/sizeof *envvars;i++)
    if ((p = getenv(envvars[i])))
      {
	NormalizeSlashes(strcpy(iwad,p));
	if (WadFileStatus(iwad,&isdir))
	  if (!isdir)
	    {
	      if (!*customiwad)
                return usermsg("Looking for %s",iwad), iwad; // killough 8/8/98
	      else
		if ((p = strrchr(iwad,'/')))
		  {
		    *p=0;
		    strcat(iwad,customiwad);
                    usermsg("Looking for %s",iwad);  // killough 8/8/98
		    if (WadFileStatus(iwad,&isdir) && !isdir)
		      return iwad;
		  }
	    }
	  else
	    {
              if(devparm)       // sf: devparm only
                      printf("Looking in %s\n",iwad);  // killough 8/8/98
	      if (*customiwad)
		{
		  if (WadFileStatus(strcat(iwad,customiwad),&isdir) && !isdir)
		    return iwad;
		}
	      else
		for (i=0;i<nstandard_iwads;i++)
		  {
		    int n = strlen(iwad);
		    strcat(iwad,standard_iwads[i]);
		    if (WadFileStatus(iwad,&isdir) && !isdir)
		      return iwad;
		    iwad[n] = 0; // reset iwad length to former
		  }
	    }
      }

  *iwad = 0;
  return iwad;
}

//
// IdentifyVersion
//
// Set the location of the defaults file and the savegame root
// Locate and validate an IWAD file
// Determine gamemode from the IWAD
//
// supports IWADs with custom names. Also allows the -iwad parameter to
// specify which iwad is being searched for if several exist in one dir.
// The -iwad parm may specify:
//
// 1) a specific pathname, which must exist (.wad optional)
// 2) or a directory, which must contain a standard IWAD,
// 3) or a filename, which must be found in one of the standard places:
//   a) current dir,
//   b) exe dir
//   c) $DOOMWADDIR
//   d) or $HOME
//
// jff 4/19/98 rewritten to use a more advanced search algorithm

char *game_name = "unknown game";        // description of iwad

void IdentifyVersion (void)
{
  int         i;    //jff 3/24/98 index of args on commandline
  struct stat sbuf; //jff 3/24/98 used to test save path for existence
  char *iwad;

  // get config file from same directory as executable
  // killough 10/98
  sprintf(basedefault,"%s/%s.cfg", D_DoomExeDir(), D_DoomExeName());

  // set save path to -save parm or current dir

  strcpy(basesavegame,".");       //jff 3/27/98 default to current dir
  if ((i=M_CheckParm("-save")) && i<myargc-1) //jff 3/24/98 if -save present
    {
      if (!stat(myargv[i+1],&sbuf) && S_ISDIR(sbuf.st_mode)) // and is a dir
	strcpy(basesavegame,myargv[i+1]);  //jff 3/24/98 use that for savegame
      else
	puts("Error: -save path does not exist, using current dir");  // killough 8/8/98
    }

  // locate the IWAD and determine game mode from it

  iwad = FindIWADFile();

  if (iwad && *iwad)
    {
      usermsg("IWAD found: %s",iwad); //jff 4/20/98 print only if found

      CheckIWAD(iwad,
		&gamemode,
		&gamemission,   // joel 10/16/98 gamemission added
		&haswolflevels);

      switch(gamemode)
	{
	case retail:
          i = strlen(iwad);
          if (i>=8 && !strnicmp(iwad+i-8,"chex.wad",8))
            {
              game_name = "Chex Quest";
              gamemission = chex;
              break;
            }
           
          if(gamemission == freedoom) 
            {
              game_name = "Freedoom Phase I";
              break;
            } 
	  game_name = "Ultimate DOOM version";  // killough 8/8/98
	  break;

	case registered:
	  game_name = "DOOM Registered version";
	  break;

	case shareware:
	  game_name = "DOOM Shareware version";
	  break;

	case commercial:

	  // joel 10/16/98 Final DOOM fix
	  switch (gamemission)
	    {
	    case pack_tnt:
	      game_name = "Final DOOM: TNT - Evilution version";
	      MARK_EXTRA_LOADED(EXTRA_WIMAPS, true);
	      break;

      case freedoom:
        game_name = "Freedoom Phase II";
        break;

	    case pack_plut:
	      game_name = "Final DOOM: The Plutonia Experiment version";
	      MARK_EXTRA_LOADED(EXTRA_WIMAPS, true);
	      break;

            case hacx_reg:
              game_name = "HACX Registered version";
              break;

	    case doom2:
	    default:
	      MARK_EXTRA_LOADED(EXTRA_WIMAPS, true);

	      i = strlen(iwad);
	      if (i>=10 && !strnicmp(iwad+i-10,"doom2f.wad",10))
		{
		  language=french;
		  game_name = "DOOM II version, French language";
		}
	      else
		game_name = haswolflevels ? "DOOM II version" :
		     "DOOM II version, german edition, no wolf levels";

	      break;
	    }
	  // joel 10/16/88 end Final DOOM fix

	default:
	  break;
	}

      usermsg(game_name);

      if (gamemode == indetermined)
        usermsg(game_name = "Unknown Game Version, may not work");  // killough 8/8/98

      D_AddFile(iwad);
    }
  else
    I_Error("IWAD not found\n");
}

// killough 5/3/98: old code removed

//
// Find a Response File
//

#define MAXARGVS 100

void FindResponseFile (void)
{
  int i;

  for (i = 1;i < myargc;i++)
    if (myargv[i][0] == '@')
      {
	FILE *handle;
	int  size;
	int  k;
	int  index;
	int  indexinfile;
	char *infile;
	char *file;
	char *moreargs[MAXARGVS];
	char *firstargv;

	// READ THE RESPONSE FILE INTO MEMORY

	// killough 10/98: add default .rsp extension
	char *filename = malloc(strlen(myargv[i])+5);
	AddDefaultExtension(strcpy(filename,&myargv[i][1]),".rsp");

	handle = fopen(filename,"rb");
	if (!handle)
	  I_Error("No such response file!");          // killough 10/98

        usermsg("Found response file %s!",filename);
	free(filename);

	fseek(handle,0,SEEK_END);
	size = ftell(handle);
	fseek(handle,0,SEEK_SET);
	file = malloc (size);
	fread(file,size,1,handle);
	fclose(handle);

	// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
	for (index = 0,k = i+1; k < myargc; k++)
	  moreargs[index++] = myargv[k];

	firstargv = myargv[0];
	myargv = calloc(sizeof(char *),MAXARGVS);
	myargv[0] = firstargv;

	infile = file;
	indexinfile = k = 0;
	indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
	do
	  {
	    myargv[indexinfile++] = infile+k;
	    while(k < size &&
		  ((*(infile+k)>= ' '+1) && (*(infile+k)<='z')))
	      k++;
	    *(infile+k) = 0;
	    while(k < size &&
		  ((*(infile+k)<= ' ') || (*(infile+k)>'z')))
	      k++;
	  }
	while(k < size);

	for (k = 0;k < index;k++)
	  myargv[indexinfile++] = moreargs[k];
	myargc = indexinfile;

	// DISPLAY ARGS
        usermsg("%d command-line args:",myargc-1); // killough 10/98
	for (k=1;k<myargc;k++)
          usermsg("%s",myargv[k]);
	break;
      }
}

// killough 10/98: moved code to separate function

static void D_ProcessDehCommandLine(void)
{
  // ty 03/09/98 do dehacked stuff
  // Note: do this before any other since it is expected by
  // the deh patch author that this is actually part of the EXE itself
  // Using -deh in BOOM, others use -dehacked.
  // Ty 03/18/98 also allow .bex extension.  .bex overrides if both exist.
  // killough 11/98: also allow -bex

  int p = M_CheckParm ("-deh");
  if (p || (p = M_CheckParm("-bex")))
    {
      // the parms after p are deh/bex file names,
      // until end of parms or another - preceded parm
      // Ty 04/11/98 - Allow multiple -deh files in a row
      // killough 11/98: allow multiple -deh parameters

      boolean deh = true;
      while (++p < myargc)
	if (*myargv[p] == '-')
	  deh = !strcasecmp(myargv[p],"-deh") || !strcasecmp(myargv[p],"-bex");
	else
	  if (deh)
	    {
	      char file[PATH_MAX+1];      // killough
	      AddDefaultExtension(strcpy(file, myargv[p]), ".bex");
	      if (access(file, F_OK))  // nope
		{
		  AddDefaultExtension(strcpy(file, myargv[p]), ".deh");
		  if (access(file, F_OK))  // still nope
		    I_Error("Cannot find .deh or .bex file named %s",
			    myargv[p]);
		}
	      // during the beta we have debug output to dehout.txt
	      // (apparently, this was never removed after Boom beta-killough)
	      ProcessDehFile(file, D_dehout(), 0);  // killough 10/98
	    }
    }
  // ty 03/09/98 end of do dehacked stuff
}

void D_DehackedFile(char * file)
{
  if (access(file, F_OK)) I_Error("Cannot find .deh or .bex file named %s", file);
  ProcessDehFile(file, D_dehout(), 0);  
}

void D_ExtraDehackedFile(char * file, extra_file_t extra)
{
  if (access(file, F_OK)) I_Error("Cannot find .deh or .bex file named %s", file);
  ProcessExtraDehFile(extra, file, D_dehout(), 0);  
}

// killough 10/98: support preloaded wads

static void D_ProcessWadPreincludes(void)
{
  if (!M_CheckParm ("-noload"))
    {
      int i;
      char *s;
      for (i=0; i<MAXLOADFILES; i++)
	if ((s=wad_files[i]))
	  {
	    while (isspace(*s))
	      s++;
	    if (*s)
	      {
		char file[PATH_MAX+1];
		AddDefaultExtension(strcpy(file, s), ".wad");
		if (!access(file, R_OK))
		  {
		    D_AddFile(file);
		    modifiedgame = true;
		  }
		else
                  usermsg("Warning: could not open %s", file);
	      }
	  }
    }
}

// killough 10/98: support preloaded deh/bex files

static void D_ProcessDehPreincludes(void)
{
  if (!M_CheckParm ("-noload"))
    {
      int i;
      char *s;
      for (i=0; i<MAXLOADFILES; i++)
	if ((s=deh_files[i]))
	  {
	    while (isspace(*s))
	      s++;
	    if (*s)
	      {
		char file[PATH_MAX+1];
		AddDefaultExtension(strcpy(file, s), ".bex");
		if (!access(file, R_OK))
		  ProcessDehFile(file, D_dehout(), 0);
		else
		  {
		    AddDefaultExtension(strcpy(file, s), ".deh");
		    if (!access(file, R_OK))
		      ProcessDehFile(file, D_dehout(), 0);
		    else
                      usermsg("Warning: could not open %s .deh or .bex", s);
		  }
	      }
	  }
    }
}

// haleyjd: auto-executed console scripts

static void D_AutoExecScripts(void)
{
   if(!M_CheckParm("-nocscload")) // separate param from above
   {
      int i;
      char *s;
      for(i=0; i<MAXLOADFILES; i++)
	 if((s=csc_files[i]))
	 {
	    while(isspace(*s))
	       s++;
	    if(*s)
	    {
	       char file[PATH_MAX+1];
	       AddDefaultExtension(strcpy(file, s), ".csc");
	       if (!access(file, R_OK))
		  C_RunScriptFromFile(file);
	       else
                  usermsg("Warning: could not open console script %s", s);
	    }
	 }
   }
}

// killough 10/98: support .deh from wads
//
// A lump named DEHACKED is treated as plaintext of a .deh file embedded in
// a wad (more portable than reading/writing info.c data directly in a wad).
//
// If there are multiple instances of "DEHACKED", we process each, in first
// to last order (we must reverse the order since they will be stored in
// last to first order in the chain). Passing NULL as first argument to
// ProcessDehFile() indicates that the data comes from the lump number
// indicated by the third argument, instead of from a file.

// sf: reorganised to use d_newwadlumps

static void D_ProcessDehInWad(int i, extra_file_t extra)
{
  if (i >= 0)
    {
      if (!strncasecmp(lumpinfo[i]->name, "dehacked", 8) && lumpinfo[i]->namespace == ns_global
        && W_LumpLength(i) > 0)
 	      ProcessExtraDehFile(extra, NULL, D_dehout(), i);
    }
}
        //sf:
void startupmsg(char *func, char *desc)
{
  DEBUGMSG(func); DEBUGMSG(": "); DEBUGMSG(desc); DEBUGMSG("\n");
  if(in_textmode)
  {
     printf("%s: %s\n", func, desc);
  }
  // add colours in console mode
  C_Printf(FC_GRAY "%s: " FC_RED "%s\n", func, desc);
  if(!in_textmode)
  {
     C_Update();
  }
}

// sf: this is really part of D_DoomMain but I made it into
// a seperate function
// this stuff needs to be kept together

void D_SetGraphicsMode()
{
  DEBUGMSG("** set graphics mode\n");

  // set graphics mode
  I_InitGraphics();
  DEBUGMSG("done\n");

  // set up the console to display startup messages
  gamestate = GS_CONSOLE; 
  current_height = SCREENHEIGHT;
  c_showprompt = false;
#ifdef 0                // LP: these just clutter console now
  C_Puts(game_name);    // display description of gamemode
  D_ListWads();         // list wads to the console
  C_Printf("\n");       // leave a gap
#endif
  Ex_ListTapeWad();
#ifdef EPISINFO
  D_ListEpisodes();
#endif
}

//
// D_CheckRelatdWads
//

static int D_CheckRelatedWads()
{
  int i = 1;
  int n = 0;
  while(i < numwadfiles)
    {
      int j = Ex_InsertRelatedWads(wadfiles[i], i);
      n += j;
      i += j + 1;
    }
  return n;
}

//
// D_DoomMain
//

void D_DoomMain(void)
{
  int p, slot;
  char file[PATH_MAX+1];      // killough 3/22/98

  byte codfound, codlevfound;
  int i;

  setbuf(stdout, NULL);

  FindResponseFile();         // Append response file arguments to command-line

  devparm = !!M_CheckParm ("-devparm");         //sf: move up here

  // killough 10/98: set default savename based on executable's name
  sprintf(savegamename = malloc(16), "%.4ssav", D_DoomExeName());

  nodemo                      = M_CheckParm ("-nodemo");
#ifdef NORENDER
  norenderparm                = M_CheckParm ("-norender");
#endif
  safeparm                    = M_CheckParm ("-safe");   // GB 2014  
  // GB 2014, safeparm: skip nearptr_enable function, retain memory protection. 0,1 FPS less if I put it in i_main
  if (_get_dos_version(1)==0x532) {printf("Windows NT based OS detected: Safe mode enabled.\n"); safeparm=1;} // Windows NT, 2000, XP
  if (!safeparm) 
   // 2/2/98 Stan, Must call this here.  It's required by both netgames and i_video.c.
  if (!__djgpp_nearptr_enable()) {printf ("Failed trying to allocate DOS near pointers, try -safe parameter.\n"); return;}
  nolfbparm                   = M_CheckParm ("-nolfb");  // GB 2014
  nopmparm                    = M_CheckParm ("-nopm");   // GB 2014
#ifdef CALT
  noasmxparm                  = M_CheckParm ("-noasmx"); // GB 2014
#endif
  asmp6parm                   = M_CheckParm ("-asmp6");  // GB 2014  

  Ex_ResetExtraStatus();
  modifiedgame = false;

  IdentifyVersion();

  D_BuildBEXTables(); // haleyjd

  // killough 10/98: process all command-line DEH's first
  D_ProcessDehCommandLine();

  // sf: removed beta

  // jff 1/24/98 set both working and command line value of play parms
  // sf: make boolean for console
  nomonsters = clnomonsters = !!M_CheckParm ("-nomonsters");
  respawnparm = clrespawnparm = !!M_CheckParm ("-respawn");
  fastparm = clfastparm = !!M_CheckParm ("-fast");
  // jff 1/24/98 end of set to both working and command line value

  if (M_CheckParm ("-deathmatch"))
    deathmatch = 1;
  if (M_CheckParm ("-altdeath"))
    deathmatch = 2;
  if (M_CheckParm ("-trideath"))  // deathmatch 3.0!
    deathmatch = 3;     

#ifdef GAMEBAR

  switch ( gamemode )
    {
      case retail:
	sprintf (title,"The Ultimate DOOM Startup");
	break;
      case shareware:
	sprintf (title,"DOOM Shareware Startup");
	break;
	
      case registered:
	sprintf (title,"DOOM Registered Startup");
	break;
	
      case commercial:
	switch (gamemission)      // joel 10/16/98 Final DOOM fix
	  {
	    case pack_plut:
	      sprintf (title,"DOOM 2: Plutonia Experiment");
	      break;
	      
	    case pack_tnt:
	      sprintf (title, "DOOM 2: TNT - Evilution");
	      break;
	      
	    case doom2:
	    default:
	      sprintf (title,"DOOM 2: Hell on Earth");
	      break;
	  }
	break;
	// joel 10/16/98 end Final DOOM fix
	
      default:
        // peridot: FIXME - anything not in gamemission enum is Public DOOM?        
	sprintf (title, "Public DOOM");
	break;
    }
  printf("%s\n", title);
  printf("%s\nBuilt on %s at %s\n", title, version_date, 
         version_time);    // killough 2/1/98
#else
  // haleyjd: always provide version date/time
  printf("Built on %s at %s\n", version_date, version_time);
#endif /* GAMEBAR */
    
  if (devparm)
    {
      printf(D_DEVSTR);
      v_ticker = true;  // turn on the fps ticker
    }
  
  if (M_CheckParm("-cdrom"))
    {
      usermsg(D_CDROM);
#ifdef _MSC_VER
      mkdir("c:/doomdata");
#else
      mkdir("c:/doomdata",0);
#endif
      
      // killough 10/98:
      sprintf(basedefault, "c:/doomdata/%s.cfg", D_DoomExeName());
    }

  // turbo option
  if ((p=M_CheckParm ("-turbo")))
    {
      extern int turbo_scale;
      extern int forwardmove[2];
      extern int sidemove[2];

      if (p<myargc-1)
        turbo_scale = atoi(myargv[p+1]);
      if (turbo_scale < 10)
        turbo_scale = 10;
      if (turbo_scale > 400)
        turbo_scale = 400;
      usermsg ("turbo scale: %i%%",turbo_scale);
      forwardmove[0] = forwardmove[0]*turbo_scale/100;
      forwardmove[1] = forwardmove[1]*turbo_scale/100;
      sidemove[0] = sidemove[0]*turbo_scale/100;
      sidemove[1] = sidemove[1]*turbo_scale/100;
    }
  
  modifiedgame = false;         // reset, ignoring smmu.wad etc.
  
  // add any files specified on the command line with -file wadfile
  // to the wad list

  // killough 1/31/98, 5/2/98: reload hack removed, -wart same as -warp now.

  if ((p = M_CheckParm ("-file")))
    {
      // the parms after p are wadfile/lump names,
      // until end of parms or another - preceded parm
      // killough 11/98: allow multiple -file parameters

      boolean file = modifiedgame = true;            // homebrew levels
      while (++p < myargc)
	if (*myargv[p] == '-')
	  file = !strcasecmp(myargv[p],"-file");
	else
	  if (file)
	    D_AddFile(myargv[p]);
    }

  codfound = D_HasWadInWadlist("cod");
  codlevfound = D_HasWadInWadlist("codlev");

  if(!codfound)
     {
        char filestr[256];
        // get smmu.wad from the same directory as smmu.exe
        // 25/10/99: use same name as exe

        // haleyjd: merged smmu.wad and eternity.wad

        sprintf(filestr, "%seternity.wad", D_DoomExeDir());
        D_InsertFile(filestr, 1); 
     }
  else if(codlevfound)
    {
      s_GOTBLUESKUL = GOTGREENCARD;
      s_GOTYELWSKUL = GOTBLACKCARD;
      s_GOTREDSKULL = GOTPURPLCARD;
      s_PD_BLUES    = PD_GREENS; 
      s_PD_REDS     = PD_PURPLS; 
      s_PD_YELLOWS  = PD_BLACKS; 
    }
     
  if (!(p = M_CheckParm("-playdemo")) || p >= myargc-1)    // killough
    if ((p = M_CheckParm ("-fastdemo")) && p < myargc-1)   // killough
      fastdemo = true;             // run at fastest speed possible
    else
      p = M_CheckParm ("-timedemo");

  if (p && p < myargc-1)
    {
      strcpy(file,myargv[p+1]);
      AddDefaultExtension(file,".lmp");     // killough
      D_AddFile(file);
      usermsg("Playing demo %s",file);
    }

  // get skill / episode / map from parms

  startskill = sk_none; // jff 3/24/98 was sk_medium, just note not picked
  startepisode = 1;
  startmap = 1;
  autostart = false;

  if ((p = M_CheckParm ("-skill")) && p < myargc-1)
    {
      startskill = myargv[p+1][0]-'1';
      autostart = true;
    }

  if ((p = M_CheckParm ("-episode")) && p < myargc-1)
    {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
    }

        // sf: moved back timer code
  if ((p = M_CheckParm ("-timer")) && p < myargc-1 && deathmatch)
    {
      int time = atoi(myargv[p+1]);
      extern int levelTimeLimit;

      usermsg("Levels will end after %d minute%s.", time, time>1 ? "s" : "");
      levelTimeLimit = time;
    }

  // sf: moved from p_spec.c
  
  // See if -frags has been used

  p = M_CheckParm("-frags");
  if (p && deathmatch)
    {
      int frags;
      extern int levelFragLimit;

      frags = atoi(myargv[p+1]);
      if (frags <= 0) frags = 10;  // default 10 if no count provided
      levelFragLimit = frags;
    }

  if ((p = M_CheckParm ("-avg")) && p < myargc-1 && deathmatch)
  {
    extern int levelTimeLimit;

    levelTimeLimit = 20 * 60 * TICRATE;
    puts("Austin Virtual Gaming: Levels will end after 20 minutes");
  }

  if (((p = M_CheckParm ("-warp")) ||      // killough 5/2/98
       (p = M_CheckParm ("-wart"))) && p < myargc-1)
    if (gamemode == commercial)
      {
	startmap = atoi(myargv[p+1]);
	autostart = true;
      }
    else    // 1/25/98 killough: fix -warp xxx from crashing Doom 1 / UD
      if (p < myargc-2)
	{
	  startepisode = atoi(myargv[++p]);
	  startmap = atoi(myargv[p+1]);
	  autostart = true;
	}

  //jff 1/22/98 add command line parms to disable sound and music
  {
    int nosound = !!M_CheckParm("-nosound");
    nomusicparm = nosound || M_CheckParm("-nomusic");
    nosfxparm   = nosound || M_CheckParm("-nosfx");
  }
  //jff end of sound/music command line parms

  // killough 3/2/98: allow -nodraw -noblit generally
  nodrawers = !!M_CheckParm ("-nodraw");
  noblit = !!M_CheckParm ("-noblit");

  if(M_CheckParm("-debugfile")) // sf: debugfile check earlier
  {
     char filename[20];
     sprintf(filename, "debug%i.txt", consoleplayer);
     usermsg("debug output to: %s", filename);
     debugfile = fopen(filename, "w");
  }

  if(M_CheckParm("-debugstd"))
  {
     usermsg("debug output to stdout");
     debugfile = stdout;
  }
  
  // haleyjd: need to do this before M_LoadDefaults
  C_InitPlayerName();
 
  startupmsg("M_LoadDefaults", "Load system defaults.");
  M_LoadDefaults();              // load before initing other systems

  bodyquesize = default_bodyquesize; // killough 10/98

  G_ReloadDefaults();    // killough 3/4/98: set defaults just loaded.
  // jff 3/24/98 this sets startskill if it was -1

  // 1/18/98 killough: Z_Init call moved to i_main.c

  // init subsystems
  startupmsg("V_Init", "allocate screens.");    // killough 11/98: moved down to here
  //V_Init(); - leaving the line above for "flavor"

  D_ProcessWadPreincludes(); // killough 10/98: add preincluded wads at the end

  if(!M_CheckParm("-noload")) D_CheckRelatedWads(); 

//  D_AddFile(NULL);           // killough 11/98

   MARK_EXTRA_LOADED(EXTRA_WIMAPS, !M_CheckParm("-noload") &&
    ((IS_EXTRA_LOADED(EXTRA_WIMAPS) && !modifiedgame) || M_CheckParm("-wimaps")));

  startupmsg("W_Init", "Init WADfiles.");
  
  Ex_DetectAndLoadTapeWads(wadfiles, !M_CheckParm("-noload"));

  Ex_InsertFixes(wadfiles[0], !M_CheckParm("-noload"));
  if (M_CheckParm ("-skins"))
    {
      startupmsg("Extras","Detecting skin WADs to autoload.");
      Ex_DetectAndAddSkins();
    }
  W_InitMultipleFiles(wadfiles);

  usermsg("");  // gap

  D_ProcessDehPreincludes(); // killough 10/98: process preincluded .deh files

  // Check for -file in shareware
  if (modifiedgame)
    {
      // These are the lumps that will be checked in IWAD,
      // if any one is not present, execution will be aborted.
      static const char name[23][8]= {
	"e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
	"e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
	"dphoof","bfgga0","heada1","cybra1","spida1d1" };
      int i;

      if (gamemode == shareware)
	I_Error("\nYou cannot -file with the shareware version. Register!");

      // Check for fake IWAD with right name,
      // but w/o all the lumps of the registered version.
      if (gamemode == registered)
	for (i = 0;i < 23; i++)
	  if (W_CheckNumForName(name[i])<0 &&
	      (W_CheckNumForName)(name[i],ns_sprites)<0) // killough 4/18/98
	    I_Error("\nThis is not the registered version.");
    }

  if (codfound && codlevfound)
    gamemission = cod;

  V_InitColorTranslation(); //jff 4/24/98 load color translation lumps

  // killough 2/22/98: copyright / "modified game" / SPA banners removed

  // Ty 04/08/98 - Add 5 lines of misc. data, only if nonblank
  // The expectation is that these will be set in a .bex file

  // haleyjd: in order for these to play the appropriate role, they
  //  should appear in the console if not in text mode startup

  if(textmode_startup)
  {
     if (*startup1) puts(startup1);
     if (*startup2) puts(startup2);
     if (*startup3) puts(startup3);
     if (*startup4) puts(startup4);
     if (*startup5) puts(startup5);
  }
  // End new startup strings

  startupmsg("C_Init","Init console.");
  C_Init();

  startupmsg("V_InitMisc","Init miscellaneous video patches");
  V_InitMisc();

  startupmsg("I_Init","Setting up machine state.");
  I_Init();

  /////////////////////////////////////////////////////////////////////////
  
  // devparm override of early set graphics mode

  if(!textmode_startup && !devparm)
    {
      startupmsg("D_SetGraphicsMode", "Set graphics mode");
      D_SetGraphicsMode();
      V_FillScreen(BG_COLOR, FG);
    }
  
  startupmsg("R_Init","Init DOOM refresh daemon");
  R_Init();

  startupmsg("P_Init","Init Playloop state.");
  P_Init();

  startupmsg("HU_Init","Setting up heads up display.");
  HU_Init();

  startupmsg("ST_Init","Init status bar.");
  ST_Init();

  startupmsg("MN_Init","Init menu.");
  MN_Init();

  startupmsg("T_Init", "Init FraggleScript.");
  T_Init();

  startupmsg("D_CheckNetGame","Check netgame status.");
  D_CheckNetGame();

  // haleyjd: this SHOULD be late enough...
  startupmsg("G_LoadDefaults", "Init keybindings.");
  G_LoadDefaults();

  // haleyjd: AFTER keybindings for overrides
  startupmsg("D_AutoExecScripts", "Executing console scripts.");
  D_AutoExecScripts();

  // load all dymic extras before starting the game
  if (!M_CheckParm ("-noload"))
    {
      startupmsg("Extras","Detecting extra WADs to autoload.");
      Ex_DetectAndLoadExtras();
    }

  //////////////////////////////////////////////////////////////////////
  //
  // Must be in Graphics mode by now!
  //
    
  // check
  if(in_textmode)
    {
      D_SetGraphicsMode();
      V_FillScreen(BG_COLOR, FG);
    }

  // haleyjd: updated for eternity
  C_Printf("\n");
  C_Seperator();
  C_Printf("\n"
	   FC_GRAY "THE ETERNITY ENGINE" FC_RED " by James Haley\n"
	   "Based on SMMU by Simon 'Fraggle' Howard\n"
	   "http://doomworld.com/eternity/ \n"
           "version %i.%02i '%s' \n",
	   VERSION/100, VERSION%100, version_name);
  usermsg("Built on %s at %s\n", version_date, version_time);

  // haleyjd: if we didn't do textmode startup, these didn't show up
  //  earlier, so now is a cool time to show them :)
  // haleyjd: altered to prevent printf attacks
  if(!textmode_startup)
  {
        if (*startup1) C_Printf("%s", startup1);
        if (*startup2) C_Printf("%s", startup2);
        if (*startup3) C_Printf("%s", startup3);
        if (*startup4) C_Printf("%s", startup4);
        if (*startup5) C_Printf("%s", startup5);
  }
  
  if(!textmode_startup && !devparm)
    C_Update();

  // Are we in Caverns of Darkness mode?
  if (gamemission == cod)
     {
        usermsg(FC_GOLD "Caverns of Darkness compatibility activated");
     }
  if (codfound)
     {
        C_Printf("Not loading bundled eternity.wad\n");
        C_Printf("as " FC_GOLD "cod.wad" FC_RED " was provided with -file\n");
     }

  idmusnum = -1; //jff 3/17/98 insure idmus number is blank

  // check for a driver that wants intermission stats
  if ((p = M_CheckParm ("-statcopy")) && p<myargc-1)
    {
      // for statistics driver
      extern  void* statcopy;

      // killough 5/2/98: this takes a memory
      // address as an integer on the command line!

      statcopy = (void*) atoi(myargv[p+1]);
      usermsg("External statistics registered.");
    }

  // sf: -blockmap option as a variable now
  if(M_CheckParm("-blockmap")) r_blockmap = true;

  startupmsg("S_Init","Setting up sound.");
  S_Init(snd_SfxVolume, snd_MusicVolume);

  // start the appropriate game based on parms

  // killough 12/98: 
  // Support -loadgame with -record and reimplement -recordfrom.

  if ((slot = M_CheckParm("-recordfrom")) && (p = slot+2) < myargc)
    G_RecordDemo(myargv[p]);
  else
    {
      slot = M_CheckParm("-loadgame");
      if ((p = M_CheckParm("-record")) && ++p < myargc)
	{
	  autostart = true;
	  G_RecordDemo(myargv[p]);
	}
    }

  if ((p = M_CheckParm ("-fastdemo")) && ++p < myargc)
    {                                 // killough
      fastdemo = true;                // run at fastest speed possible
      timingdemo = true;              // show stats after quit
      G_DeferedPlayDemo(myargv[p]);
      singledemo = true;              // quit after one demo
    }
  else
    if ((p = M_CheckParm("-timedemo")) && ++p < myargc)
      {
	  G_TimeDemo(myargv[p], false);
      }
    else
      if ((p = M_CheckParm("-playdemo")) && ++p < myargc)
	{
	  G_DeferedPlayDemo(myargv[p]);
	  singledemo = true;          // quit after one demo
	}

  DEBUGMSG("start gamestate: title screen or whatever\n");

  HU_Start();

  startlevel = Z_Strdup(G_GetNameForMap(startepisode, startmap), PU_STATIC, 0);

  // killough 12/98: inlined D_DoomLoop

  if (slot && ++slot < myargc)
    {
      slot = atoi(myargv[slot]);        // killough 3/16/98: add slot info
      G_SaveGameName(file, slot);       // killough 3/22/98
      G_LoadGame(file, slot, true);     // killough 5/15/98: add command flag
    }
  else
    if (!singledemo)                    // killough 12/98
    {
      if(netgame)
	{
          C_SendNetData();

	  if (demorecording)
	     G_BeginRecording();
	}
      else if(autostart)
	{
	  G_InitNew(startskill, startlevel);

	  if (demorecording)
	     G_BeginRecording();
	}
      else
	{
          D_StartTitle();                 // start up intro loop
	}
    }

  // this fixes a strange bug, don't know why but it works
  if(!textmode_startup && !devparm)
    for(p=0; p<10; p++)
          C_Update();

  C_InstaPopup();

  DEBUGMSG("start main loop\n");

  oldgamestate = wipegamestate = gamestate;

  redrawborder = true;

  while(1)
    {
      // frame syncronous IO operations
      I_StartFrame ();

      DEBUGMSG("tics\n");
      TryRunTics (); // will run at least one tic

      DEBUGMSG("sound\n")

      // killough 3/16/98: change consoleplayer to displayplayer
      S_UpdateSounds(players[displayplayer].mo);// move positional sounds

      DEBUGMSG("display\n")

      // Update display, next frame, with current state.
      D_Display();

      DEBUGMSG("more sound\n")

      // Sound mixing for the buffer is synchronous.
      I_UpdateSound();

      // Synchronous sound output is explicitly called.
      // Update sound output.
      I_SubmitSound();
    }
}

// re init everything after loading a wad

void D_ReInitWadfiles()
{
  R_Free();
  R_Init();
  P_Free();
  P_Init();
  I_RescanSounds();
}

void D_NewWadLumps(int handle, extra_file_t extra)
{
  int i;
  char wad_firstlevel[9] = "";
  int reset_finallevel = 0;
  char wad_finallevel[9] = "";
  strncpy(wad_finallevel, finallevel, 8);
  
  for(i=0; i<numlumps; i++)
    {
      if(lumpinfo[i]->handle != handle) continue;
      
      if(!strncmp(lumpinfo[i]->name, "THINGS", 8))    // a level
	{
	  char *name = lumpinfo[i-1]->name; // previous lump
	  
	  // 'ExMy'
          // TODO: for DOOM1 we don't actually detect final level of a pwad
          if(isExMy(name))
            {
              reset_finallevel = 1;
              if(name[3] != '9' && !W_IsLumpFromIWAD(i)) estimated_maps_no += 1;
            }
          else if(isMAPxy(name))
            {
              if(!W_IsLumpFromIWAD(i)) estimated_maps_no += 1;
            }

	  if(isExMy(name) && isExMy(wad_firstlevel))
	    {
	      if(name[1] < wad_firstlevel[1] ||       // earlier episode?
		 // earlier level in the same episode?
                 (name[1] == wad_firstlevel[1] && name[3] < wad_firstlevel[3]))
		strncpy(wad_firstlevel, name, 8);
	    }
	  if(isMAPxy(name) && isMAPxy(wad_firstlevel))
	    {
	      if(name[3] < wad_firstlevel[3] || // earlier 10 levels
		 // earlier in the same 10 levels?
                 (name[3] == wad_firstlevel[3] && name[4] < wad_firstlevel[4]))
		strncpy(wad_firstlevel, name, 8);
              if(name[3] == '3') // 31, 32
                {
                  if(name[4] == '1' || name[4] == '2') continue;
                }
              if(!*wad_finallevel ||
                wad_finallevel[3] < name[3] ||
                wad_finallevel[4] < name[4])
                {
                  strncpy(wad_finallevel, name, 8);
                }
	    }

	  // none set yet
	  // ignore ones called 'start' as these are checked
	  // elsewhere (m_menu.c)
	  if(!*wad_firstlevel && strcmp(name, "START") )
	    strncpy(wad_firstlevel, name, 8);
	}
      // new sound
      if(!strncmp(lumpinfo[i]->name, "DSCHGUN",8)) // chaingun sound
	S_Chgun();
      if(!strncmp(lumpinfo[i]->name, "DS", 2))
	{
          S_UpdateSound(i);
	}
      
      // new music
      if(!strncmp(lumpinfo[i]->name, "D_", 2))
	{
	  S_UpdateMusic(i);
	}

      // skins
      if(!strncmp(lumpinfo[i]->name, "S_SKIN", 6))
	{
	  P_ParseSkin(i);
	}

      // mbf dehacked lump
      if(!strncmp(lumpinfo[i]->name, "DEHACKED", 8))
	{
	  D_ProcessDehInWad(i, extra);
	}

#ifdef EPISINFO
      if(!strncmp(lumpinfo[i]->name, "EPISINFO", 8))
	{
          P_LoadEpisodeInfo(i);
	}
#endif
    } 
  
  if(*wad_firstlevel) // a new first level?
    strncpy(firstlevel, wad_firstlevel, 8);

  if(*wad_finallevel)
    strncpy(finallevel, wad_finallevel, 8);

  if(reset_finallevel)
    finallevel[0] = 0;

}

void usermsg(char *s, ...)
{
  static char msg[1024];
  va_list v;
  va_start(v,s);
  vsprintf(msg,s,v);                  // print message in buffer
  va_end(v);

  if(in_textmode)
  {
     printf("%s\n", msg);
  }
  C_Printf("%s\n", msg);
  if(!in_textmode)
  {
     C_Update();
  }
}

// add a new .wad file
// returns true if successfully loaded

boolean D_AddNewFile(char *s)
{
  c_showprompt = false;
  if(W_AddNewFile(c_argv[0])) return false;
  modifiedgame = true;
  D_AddFile(c_argv[0]);   // add to the list of wads
  C_SetConsole();
  D_ReInitWadfiles();
  return true;
}

//----------------------------------------------------------------------------
//
// $Log: d_main.c,v $
// Revision 1.47  1998/05/16  09:16:51  killough
// Make loadgame checksum friendlier
//
// Revision 1.46  1998/05/12  10:32:42  jim
// remove LEESFIXES from d_main
//
// Revision 1.45  1998/05/06  15:15:46  jim
// Documented IWAD routines
//
// Revision 1.44  1998/05/03  22:26:31  killough
// beautification, declarations, headers
//
// Revision 1.43  1998/04/24  08:08:13  jim
// Make text translate tables lumps
//
// Revision 1.42  1998/04/21  23:46:01  jim
// Predefined lump dumper option
//
// Revision 1.39  1998/04/20  11:06:42  jim
// Fixed print of IWAD found
//
// Revision 1.37  1998/04/19  01:12:19  killough
// Fix registered check to work with new lump namespaces
//
// Revision 1.36  1998/04/16  18:12:50  jim
// Fixed leak
//
// Revision 1.35  1998/04/14  08:14:18  killough
// Remove obsolete adaptive_gametics code
//
// Revision 1.34  1998/04/12  22:54:41  phares
// Remaining 3 Setup screens
//
// Revision 1.33  1998/04/11  14:49:15  thldrmn
// Allow multiple deh/bex files
//
// Revision 1.32  1998/04/10  06:31:50  killough
// Add adaptive gametic timer
//
// Revision 1.31  1998/04/09  09:18:17  thldrmn
// Added generic startup strings for BEX use
//
// Revision 1.30  1998/04/06  04:52:29  killough
// Allow demo_insurance=2, fix fps regression wrt redrawsbar
//
// Revision 1.29  1998/03/31  01:08:11  phares
// Initial Setup screens and Extended HELP screens
//
// Revision 1.28  1998/03/28  15:49:37  jim
// Fixed merge glitches in d_main.c and g_game.c
//
// Revision 1.27  1998/03/27  21:26:16  jim
// Default save dir offically . now
//
// Revision 1.26  1998/03/25  18:14:21  jim
// Fixed duplicate IWAD search in .
//
// Revision 1.25  1998/03/24  16:16:00  jim
// Fixed looking for wads message
//
// Revision 1.23  1998/03/24  03:16:51  jim
// added -iwad and -save parms to command line
//
// Revision 1.22  1998/03/23  03:07:44  killough
// Use G_SaveGameName, fix some remaining default.cfg's
//
// Revision 1.21  1998/03/18  23:13:54  jim
// Deh text additions
//
// Revision 1.19  1998/03/16  12:27:44  killough
// Remember savegame slot when loading
//
// Revision 1.18  1998/03/10  07:14:58  jim
// Initial DEH support added, minus text
//
// Revision 1.17  1998/03/09  07:07:45  killough
// print newline after wad files
//
// Revision 1.16  1998/03/04  08:12:05  killough
// Correctly set defaults before recording demos
//
// Revision 1.15  1998/03/02  11:24:25  killough
// make -nodraw -noblit work generally, fix ENDOOM
//
// Revision 1.14  1998/02/23  04:13:55  killough
// My own fix for m_misc.c warning, plus lots more (Rand's can wait)
//
// Revision 1.11  1998/02/20  21:56:41  phares
// Preliminarey sprite translucency
//
// Revision 1.10  1998/02/20  00:09:00  killough
// change iwad search path order
//
// Revision 1.9  1998/02/17  06:09:35  killough
// Cache D_DoomExeDir and support basesavegame
//
// Revision 1.8  1998/02/02  13:20:03  killough
// Ultimate Doom, -fastdemo -nodraw -noblit support, default_compatibility
//
// Revision 1.7  1998/01/30  18:48:15  phares
// Changed textspeed and textwait to functions
//
// Revision 1.6  1998/01/30  16:08:59  phares
// Faster end-mission text display
//
// Revision 1.5  1998/01/26  19:23:04  phares
// First rev with no ^Ms
//
// Revision 1.4  1998/01/26  05:40:12  killough
// Fix Doom 1 crashes on -warp with too few args
//
// Revision 1.3  1998/01/24  21:03:04  jim
// Fixed disappearence of nomonsters, respawn, or fast mode after demo play or IDCLEV
//
// Revision 1.1.1.1  1998/01/19  14:02:53  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
