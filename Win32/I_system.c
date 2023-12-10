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
//   General-purpose system-specific routines, including timer
//   installation, keyboard, mouse, and joystick code.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_system.c,v 1.14 1998/05/03 22:33:13 killough Exp $";

#include <stdio.h>

#include <stdarg.h>

#include <sdl.h>

#include "../c_runcmd.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../i_video.h"
#include "../doomstat.h"
#include "../m_misc.h"
#include "../g_game.h"
#include "../w_wad.h"
#include "../v_video.h"
#include "../m_argv.h"

#include "../g_bind.h"

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}


// SoM 3/13/2002: SDL time. 1000 ticks per second.
void I_WaitVBL(int count)
{
  SDL_Delay((count*1000)/TICRATE);
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//
static Uint32 basetime=0;

int  I_GetTime_RealTime (void)
{
    Uint32        ticks;

    // milliseconds since SDL initialization
    ticks = SDL_GetTicks();

    return (ticks - basetime)*TICRATE/1000;
}


void I_SetTime(int newtime)
{
  // SoM 3/14/2002: Uh, this function is never called. ??
}

        //sf: made a #define, changed to 16
#define CLOCK_BITS 16

// killough 4/13/98: Make clock rate adjustable by scale factor
int realtic_clock_rate = 100;
static Long64 I_GetTime_Scale = 1<<CLOCK_BITS;
int I_GetTime_Scaled(void)
{
  return (int)(((Long64)I_GetTime_RealTime() * I_GetTime_Scale) >> CLOCK_BITS);
}


static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}


static int I_GetTime_Error()
{
  I_Error("Error: GetTime() used before initialization");
  return 0;
}

int (*I_GetTime)() = I_GetTime_Error;                           // killough

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

int keyboard_installed = 0;
static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
extern int autorun;          // Autorun state

void I_Shutdown(void)
{
  
}


void I_InitKeyboard()
{
  keyboard_installed = 1;
}

void I_Init(void)
{
  int clock_rate = realtic_clock_rate, p;

  if ((p = M_CheckParm("-speed")) && p < myargc-1 &&
      (p = atoi(myargv[p+1])) >= 10 && p <= 1000)
    clock_rate = p;

  basetime = SDL_GetTicks();
    
  // killough 4/14/98: Adjustable speedup based on realtic_clock_rate
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (clock_rate != 100)
      {
        I_GetTime_Scale = ((__LONG64_TYPE__) clock_rate << CLOCK_BITS) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

  // killough 3/6/98: end of keyboard / autorun state changes


  atexit(I_Shutdown);

  { // killough 2/21/98: avoid sound initialization if no sound & no music
    extern boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }
}

//
// I_Quit
//

static char errmsg[2048];    // buffer of error message -- killough

static int has_exited;

void I_Quit (void)
{
  has_exited = 1;   /* Prevent infinitely recursive exits -- killough */

  if (demorecording)
    G_CheckDemoStatus();

        // sf : rearrange this so the errmsg doesn't get messed up
  if (*errmsg)
    puts(errmsg);   // killough 8/8/98
  else
    I_EndDoom();

  M_SaveDefaults();
  G_SaveDefaults(); // haleyjd
}

//
// I_Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
  if (!*errmsg)   // ignore all but the first message -- killough
    {
      va_list argptr;
      va_start(argptr,error);
      vsprintf(errmsg,error,argptr);
      va_end(argptr);
    }

  if (!has_exited)    // If it hasn't exited yet, exit now -- killough
    {
      has_exited=1;   // Prevent infinitely recursive exits -- killough
      exit(-1);
    }
}

// killough 2/22/98: Add support for ENDBOOM, which is PC-specific
// killough 8/1/98: change back to ENDOOM

void I_EndDoom(void)
{
  /*int lump;

  if (lumpinfo && (lump = W_CheckNumForName("ENDOOM")) != -1) // killough 10/98
    {  // killough 8/19/98: simplify
      memcpy(0xb8000 + (byte *) __djgpp_conventional_base,
	     W_CacheLumpNum(lump, PU_CACHE), 0xf00);
      gotoxy(1,24);
    }*/
}

        // check for ESC button pressed, regardless of keyboard handler
int I_CheckAbort()
{
  // check normal keyboard handler
  return false;
}

/*************************
        CONSOLE COMMANDS
 *************************/
int leds_always_off;

VARIABLE_BOOLEAN(leds_always_off, NULL,     yesno);
VARIABLE_INT(realtic_clock_rate, NULL,  0, 500, NULL);

CONSOLE_VARIABLE(i_gamespeed, realtic_clock_rate, 0)
{
  if (realtic_clock_rate != 100)
    {
      I_GetTime_Scale = ((__LONG64_TYPE__) realtic_clock_rate << CLOCK_BITS) / 100;
      I_GetTime = I_GetTime_Scaled;
    }
  else
    I_GetTime = I_GetTime_RealTime;
  
  ResetNet();         // reset the timers and stuff
}

CONSOLE_VARIABLE(i_ledsoff, leds_always_off, 0)
{
}

extern void I_Sound_AddCommands();
extern void I_Video_AddCommands();
extern void I_Input_AddCommands();
extern void Ser_AddCommands();

        // add system specific commands
void I_AddCommands()
{
  C_AddCommand(i_ledsoff);
  C_AddCommand(i_gamespeed);
  
  I_Video_AddCommands();
  I_Sound_AddCommands();
  Ser_AddCommands();
}

//----------------------------------------------------------------------------
//
// $Log: i_system.c,v $
//
//----------------------------------------------------------------------------
