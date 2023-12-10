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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//   
//   System-specific graphics code, along with some ill-placed
//   keyboard, mouse, and joystick code that needs to be moved to
//   i_system.c (TODO)
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: i_video.c,v 1.12 1998/05/03 22:40:35 killough Exp $";

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>

#include <sdl.h>

#include "../doomstat.h"

#include "../am_map.h"
#include "../c_io.h"
#include "../c_runcmd.h"
#include "../d_main.h"
#include "../i_video.h"
#include "../m_argv.h"
#include "../m_bbox.h"
#include "../mn_engin.h"
#include "../r_draw.h"
#include "../st_stuff.h"
#include "../v_video.h"
#include "../w_wad.h"
#include "../wi_stuff.h"
#include "../z_zone.h"

SDL_Surface *sdlscreen;

///////////////////////////////////////////////////////////////////////////
//
// Input Code
//
//////////////////////////////////////////////////////////////////////////

extern void I_InitKeyboard();      // i_system.c

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//
static int I_TranslateKey(int sym)
{
  int rc = 0;
  switch (sym) 
  {  
  case SDLK_LEFT:     rc = KEYD_LEFTARROW;	break;
  case SDLK_RIGHT:    rc = KEYD_RIGHTARROW;	break;
  case SDLK_DOWN:     rc = KEYD_DOWNARROW;	break;
  case SDLK_UP:       rc = KEYD_UPARROW;	break;
  case SDLK_ESCAPE:   rc = KEYD_ESCAPE;	break;
  case SDLK_RETURN:   rc = KEYD_ENTER;	break;
  case SDLK_TAB:      rc = KEYD_TAB;		break;
  case SDLK_F1:       rc = KEYD_F1;		break;
  case SDLK_F2:       rc = KEYD_F2;		break;  
  case SDLK_F3:       rc = KEYD_F3;		break;
  case SDLK_F4:       rc = KEYD_F4;		break;  
  case SDLK_F5:       rc = KEYD_F5;		break;
  case SDLK_F6:       rc = KEYD_F6;		break;  
  case SDLK_F7:       rc = KEYD_F7;		break;
  case SDLK_F8:       rc = KEYD_F8;		break;  
  case SDLK_F9:       rc = KEYD_F9;		break;
  case SDLK_F10:      rc = KEYD_F10;		break;  
  case SDLK_F11:      rc = KEYD_F11;		break;
  case SDLK_F12:      rc = KEYD_F12;		break;  
  case SDLK_BACKSPACE:
  case SDLK_DELETE:   rc = KEYD_BACKSPACE;	break;
  case SDLK_PAUSE:    rc = KEYD_PAUSE;	break;
  case SDLK_EQUALS:   rc = KEYD_EQUALS;	break;
  case SDLK_MINUS:    rc = KEYD_MINUS;	break;

//  case SDLK_KP0:      rc = KEYD_KEYPAD0;	break;
//  case SDLK_KP1:      rc = KEYD_KEYPAD1;	break;
//  case SDLK_KP2:      rc = KEYD_KEYPAD2;	break;
//  case SDLK_KP3:      rc = KEYD_KEYPAD3;	break;
//  case SDLK_KP4:      rc = KEYD_KEYPAD4;	break;
//  case SDLK_KP5:      rc = KEYD_KEYPAD0;	break;
//  case SDLK_KP6:      rc = KEYD_KEYPAD6;	break;
//  case SDLK_KP7:      rc = KEYD_KEYPAD7;	break;
//  case SDLK_KP8:      rc = KEYD_KEYPAD8;	break;
//  case SDLK_KP9:      rc = KEYD_KEYPAD9;	break;
//  case SDLK_KP_PLUS:  rc = KEYD_KEYPADPLUS;	break;
//  case SDLK_KP_MINUS: rc = KEYD_KEYPADMINUS;	break;
//  case SDLK_KP_DIVIDE:	rc = KEYD_KEYPADDIVIDE;	break;
//  case SDLK_KP_MULTIPLY: rc = KEYD_KEYPADMULTIPLY; break;
//  case SDLK_KP_ENTER:	rc = KEYD_KEYPADENTER;	break;
//  case SDLK_KP_PERIOD:	rc = KEYD_KEYPADPERIOD;	break;

  case SDLK_NUMLOCK:    rc = KEYD_NUMLOCK;	break;
  case SDLK_SCROLLOCK:  rc = KEYD_SCROLLLOCK;	break;
  case SDLK_CAPSLOCK:   rc = KEYD_CAPSLOCK;	break;  
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:     rc = KEYD_RSHIFT;	break;  
  case SDLK_LCTRL:
  case SDLK_RCTRL:      rc = KEYD_RCTRL;	break;  

  case SDLK_LALT:  case SDLK_LMETA:
  case SDLK_RALT:  case SDLK_RMETA:	rc = KEYD_RALT;		break;
  case SDLK_PAGEUP: rc = KEYD_PAGEUP; break;
  case SDLK_PAGEDOWN: rc = KEYD_PAGEDOWN; break;
  case SDLK_HOME:  rc = KEYD_HOME; break;
  case SDLK_END:  rc = KEYD_END; break;
  case SDLK_INSERT:  rc = KEYD_INSERT; break;
  default:		
    rc = sym;		
    break;
  }
  return rc;
}

int I_ScanCode2DoomCode (int a)
{
  return a;
}

int I_DoomCode2ScanCode (int a)
{
  return a;
}


extern int usemouse;   // killough 10/98

/////////////////////////////////////////////////////////////////////////////
//
// JOYSTICK                                                  // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

extern int usejoystick;
extern int joystickpresent;

/* Julian: 6/6/2001 : Allegro compatibility
   Removed declarations that are of no use whatever version of allegro is used
   since allegro.h is included, but were causing conflicts on earlier allegro

   old code here :

        // Seems like the ints have been replaced by #defines
        extern int joy_x,joy_y;
        extern int joy_b1,joy_b2,joy_b3,joy_b4;

        // prototype has changed (returns int in earlier version)
        void poll_joystick(void);
*/

// I_JoystickEvents() gathers joystick data and creates an event_t for
// later processing by G_Responder().

static void I_JoystickEvents()
{
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
  I_JoystickEvents(); // Obtain joystick data                 phares 4/3/98
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////


// SoM 3/14/2002: Rewrote event function for use with SDL
static void I_GetEvent()
{
  SDL_Event event;
  event_t   d_event;
  event_t   mouseevent;

  static int buttons = 0;

  int       sendmouseevent = 0;

  mouseevent.type = ev_mouse;
  mouseevent.data1 = 0;
  mouseevent.data2 = mouseevent.data3 = 0;

  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_KEYDOWN:
        d_event.type = ev_keydown;
        d_event.data1 = I_TranslateKey(event.key.keysym.sym);
        if(d_event.data1)
          D_PostEvent(&d_event);
      break;
      case SDL_KEYUP:
        d_event.type = ev_keyup;
        d_event.data1 = I_TranslateKey(event.key.keysym.sym);
        if(d_event.data1)
          D_PostEvent(&d_event);
      break;
      case SDL_MOUSEMOTION:       
        if(!usemouse)
          continue;

        mouseevent.data3 += -event.motion.yrel;
        mouseevent.data2 += event.motion.xrel;
        sendmouseevent = 1;
      break;
      case SDL_MOUSEBUTTONUP:
        if(!usemouse)
          continue;
        buttons &= ~event.button.button;
        sendmouseevent = 1;
        d_event.type = ev_keyup;
        if(event.button.button == SDL_BUTTON_LEFT)
          d_event.data1 = KEYD_MOUSE1;
        else if(event.button.button == SDL_BUTTON_MIDDLE)
          d_event.data1 = KEYD_MOUSE2;
        else
          d_event.data1 = KEYD_MOUSE3;
        D_PostEvent(&d_event);
      break;
      case SDL_MOUSEBUTTONDOWN:
        if(!usemouse)
          continue;
        buttons |= event.button.button;
        sendmouseevent = 1;
        d_event.type = ev_keydown;
        if(event.button.button == SDL_BUTTON_LEFT)
          d_event.data1 = KEYD_MOUSE1;
        else if(event.button.button == SDL_BUTTON_MIDDLE)
          d_event.data1 = KEYD_MOUSE2;
        else
          d_event.data1 = KEYD_MOUSE3;
        D_PostEvent(&d_event);
      break;
    }
  }

  if(sendmouseevent)
    D_PostEvent(&mouseevent);

}

//
// I_StartTic
//

void I_StartTic()
{
   I_GetEvent();
}

////////////////////////////////////////////////////////////////////////////
//
// Graphics Code
//
////////////////////////////////////////////////////////////////////////////

//
// I_UpdateNoBlit
//

void I_UpdateNoBlit (void)
{
}


int use_vsync;     // killough 2/8/98: controls whether vsync is called
int page_flip;     // killough 8/15/98: enables page flipping
int hires;
int vesamode;
boolean noblit;

static boolean in_graphics_mode;
static boolean in_page_flip, in_hires, linear;
static int scroll_offset;
static unsigned destscreen;

void I_FinishUpdate(void)
{
  if (noblit || !in_graphics_mode || in_page_flip)
    return;

  // SoM 3/14/2002: Only one way to go about this without asm code
  //memcpy(sdlscreen->pixels, screens[0], (SCREENWIDTH << hires) * (SCREENHEIGHT << hires));
  SDL_UpdateRect(sdlscreen, 0, 0, (SCREENWIDTH << hires), (SCREENHEIGHT << hires));
}

//
// I_ReadScreen
//

void I_ReadScreen(byte *scr)
{
  int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;

  memcpy(scr,*screens,size);
}

//
// killough 10/98: init disk icon
//

int disk_icon;

static SDL_Surface *diskflash, *old_data;

static void I_InitDiskFlash(void)
{
/*  byte temp[32*32];

  if (diskflash)
    {
      SDL_FreeSurface(diskflash);
      SDL_FreeSurface(old_data);
    }

  //sf : disk is actually 16x15
  diskflash = SDL_CreateRGBSurface(SDL_SWSURFACE, 16<<hires, 15<<hires, 8, 0, 0, 0, 0);
  old_data = SDL_CreateRGBSurface(SDL_SWSURFACE, 16<<hires, 15<<hires, 8, 0, 0, 0, 0);

  V_GetBlock(0, 0, 0, 16, 15, temp);
  V_DrawPatchDirect(0, -1, 0, W_CacheLumpName(M_CheckParm("-cdrom") ?
					     "STCDROM" : "STDISK", PU_CACHE));
  V_GetBlock(0, 0, 0, 16, 15, (char *)diskflash->pixels);
  V_DrawBlock(0, 0, 0, 16, 15, temp);*/
}

//
// killough 10/98: draw disk icon
//

void I_BeginRead(void)
{
  if (!disk_icon || !in_graphics_mode)
    return;

  /*blit(screen, old_data,
       (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires),
       0, 0, 16 << hires, 15 << hires);

  blit(diskflash, screen, 0, 0, (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires), 16 << hires, 15 << hires);*/
}

//
// killough 10/98: erase disk icon
//

void I_EndRead(void)
{
  if (!disk_icon || !in_graphics_mode)
    return;

/*  blit(old_data, screen, 0, 0, (SCREENWIDTH-16) << hires,
       scroll_offset + ((SCREENHEIGHT-15)<<hires), 16 << hires, 15 << hires);*/
}

void I_SetPalette(byte *palette)
{
  int i;
  SDL_Color colors[256];

  if (!in_graphics_mode)             // killough 8/11/98
    return;

  for(i = 0; i < 256; i++)
  {
    colors[i].r = gammatable[usegamma][*palette++];
    colors[i].g = gammatable[usegamma][*palette++];
    colors[i].b = gammatable[usegamma][*palette++];
  }

  SDL_SetPalette(sdlscreen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
}

void I_ShutdownGraphics(void)
{
  if (in_graphics_mode)  // killough 10/98
    {
      in_graphics_mode = false;
      in_textmode = true;
      sdlscreen = NULL;
    }
}

#define BADVID "video mode not supported"

extern boolean setsizeneeded;

//
// killough 11/98: New routine, for setting hires and page flipping
//

// sf: now returns true if an error occurred
static boolean I_InitGraphicsMode(void)
{
  int flags = SDL_SWSURFACE;
  int v_w = 320 << hires;
  int v_h = 200 << hires;

  scroll_offset = 0;

  V_Init();  

  switch(v_mode)
  {
    case 2:
    case 4:
      v_h = 240 << hires;
    case 1:
     flags |= SDL_FULLSCREEN;
    break;
  }
  
  // sf: reorganised to use v_mode
  
  if(SDL_VideoModeOK(v_w, v_h, 8, flags))
  {
    sdlscreen = SDL_SetVideoMode(v_w, v_h, 8, flags);
  }
  else
  {
    I_Error("couldn't set video mode %ix%i\n", v_w, v_h);
  }

  screens[0] = (char *)sdlscreen->pixels;
  
  MN_ErrorMsg("");       // clear any error messages
  
  in_graphics_mode = true;
  in_textmode = false;
  in_page_flip = false; //page_flip;
  in_hires = hires;

  setsizeneeded = true;

  I_InitDiskFlash();        // Initialize disk icon
  I_SetPalette(W_CacheLumpName("PLAYPAL",PU_CACHE));

  return false;
}

void I_ResetScreen(void)
{
  // Switch out of old graphics mode
  
  I_ShutdownGraphics();
  
  // Switch to new graphics mode
  // check for errors -- we may be setting to a different mode instead
  
  if(I_InitGraphicsMode())
    return;

  // reset other modules
  
  if (automapactive)
    AM_Start();             // Reset automap dimensions

  ST_Start();               // Reset palette

  if (gamestate == GS_INTERMISSION)
    {
      WI_DrawBackground();
      V_CopyRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0);
    }

  Z_CheckHeap();
}

void I_InitGraphics(void)
{
  static int firsttime = true;

  if (!firsttime)
    return;

  firsttime = false;

  if (nodrawers) // killough 3/2/98: possibly avoid gfx mode
    return;

  // init keyboard
  I_InitKeyboard();

  //
  // enter graphics mode
  //

  atexit(I_ShutdownGraphics);

  //signal(SIGINT, SIG_IGN);  // ignore CTRL-C in graphics mode

  in_page_flip = page_flip;

  V_ResetMode();

  Z_CheckHeap();
}

// the list of video modes is stored here in i_video.c
// the console commands to change them are in v_misc.c,
// so that all the platform-specific stuff is in here.
// v_misc.c does not care about the format of the videomode_t,
// all it asks is that it contains a text value 'description'
// which describes the mode
        
videomode_t videomodes[]=
{
  // hires, pageflip, vesa, description
  {0, 0, 0, "320x200 Windowed"},
  {0, 1, 0, "320x200 Fullscreen"},
  {0, 0, 1, "320x200 Fullscreen(2)"},
  {1, 0, 0, "640x400 Windowed"},
  {1, 1, 0, "640x400 Fullscreen"},
  {0, 0, 0,  NULL}  // last one has NULL description
};

void I_SetMode(int i)
{
  static int firsttime = true;    // the first time to set mode

  v_mode = i;
  
  hires = videomodes[i].hires;
  page_flip = videomodes[i].pageflip;
  vesamode = videomodes[i].vesa;
  
  if(firsttime)
    I_InitGraphicsMode();
  else
    I_ResetScreen();

  firsttime = false;
}        

/************************
        CONSOLE COMMANDS
 ************************/

VARIABLE_BOOLEAN(use_vsync, NULL,  yesno);
VARIABLE_BOOLEAN(disk_icon, NULL,  onoff);

CONSOLE_VARIABLE(v_diskicon, disk_icon, 0) {}
CONSOLE_VARIABLE(v_retrace, use_vsync, 0)
{
  V_ResetMode();
}

VARIABLE_INT(usemouse, NULL,            0, 1, yesno);
VARIABLE_INT(usejoystick, NULL,         0, 1, yesno);

CONSOLE_VARIABLE(use_mouse, usemouse, 0) {}
CONSOLE_VARIABLE(use_joystick, usejoystick, 0) {}

void I_Video_AddCommands()
{
  C_AddCommand(use_mouse);
  C_AddCommand(use_joystick);

  C_AddCommand(v_diskicon);
  C_AddCommand(v_retrace);
}

//----------------------------------------------------------------------------
//
// $Log: i_video.c,v $
// Revision 1.12  1998/05/03  22:40:35  killough
// beautification
//
// Revision 1.11  1998/04/05  00:50:53  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.10  1998/03/23  03:16:10  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.9  1998/03/09  07:13:35  killough
// Allow CTRL-BRK during game init
//
// Revision 1.8  1998/03/02  11:32:22  killough
// Add pentium blit case, make -nodraw work totally
//
// Revision 1.7  1998/02/23  04:29:09  killough
// BLIT tuning
//
// Revision 1.6  1998/02/09  03:01:20  killough
// Add vsync for flicker-free blits
//
// Revision 1.5  1998/02/03  01:33:01  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.4  1998/02/02  13:33:30  killough
// Add support for -noblit
//
// Revision 1.3  1998/01/26  19:23:31  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:59:14  killough
// New PPro blit routine
//
// Revision 1.1.1.1  1998/01/19  14:02:50  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
