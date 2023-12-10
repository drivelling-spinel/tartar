// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_main.c,v 1.2 2000-08-12 21:29:28 fraggle Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: i_main.c,v 1.2 2000-08-12 21:29:28 fraggle Exp $";

#include <signal.h>
#include <allegro.h>
#include <dpmi.h>      // Required!

#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"    
#include "../i_system.h" 

// cleanup handling -- killough:
static void handler(int s)
{
  char buf[2048];

  signal(s,SIG_IGN);  // Ignore future instances of this signal.

  strcpy(buf,
         s==SIGSEGV ? "Segmentation Violation" :
         s==SIGINT  ? "Interrupted by User" :
         s==SIGILL  ? "Illegal Instruction" :
         s==SIGFPE  ? "Floating Point Exception" :
         s==SIGTERM ? "Killed" : "Terminated by signal");

  // If corrupted memory could cause crash, dump memory
  // allocation history, which points out probable causes

  if (s==SIGSEGV || s==SIGILL || s==SIGFPE) Z_DumpHistory(buf);

  //cph - changed to prevent format string attacks, GB 2014, from PrBoom:
  //I_Error(buf);
  I_Error("%s",buf);
}

void I_Quit(void);

void set_config_rel(const char * config)
{
  char filestr[256];
  sprintf(filestr, "%s%s", D_DoomExeDir(), config);
  set_config_file(filestr);
}

int main(int argc, char **argv)
{
  myargc = argc;
  myargv = argv;

  /*
     killough 1/98:

     This fixes some problems with exit handling
     during abnormal situations.

     The old code called I_Quit() to end program,
     while now I_Quit() is installed as an exit
     handler and exit() is called to exit, either
     normally or abnormally. Seg faults are caught
     and the error handler is used, to prevent
     being left in graphics mode or having very
     loud SFX noise because the sound card is
     left in an unstable state.
  */
  set_config_rel("SETUP.CFG");
  allegro_init();
  Z_Init();                  // 1/18/98 killough: start up memory stuff first
  atexit(I_Quit);
  signal(SIGSEGV, handler);
  signal(SIGTERM, handler);
  signal(SIGILL,  handler);
  signal(SIGFPE,  handler);
  signal(SIGILL,  handler);
  signal(SIGINT,  handler);  // killough 3/6/98: allow CTRL-BRK during init
  signal(SIGABRT, handler);
  D_DoomMain (); 
  return 0;
}


//----------------------------------------------------------------------------
//
// $Log: i_main.c,v $
// Revision 1.2  2000-08-12 21:29:28  fraggle
// change license header
//
// Revision 1.1.1.1  2000/07/29 13:20:39  fraggle
// imported sources
//
// Revision 1.8  1998/05/15  00:34:03  killough
// Remove unnecessary crash hack
//
// Revision 1.7  1998/05/13  22:58:04  killough
// Restore Doom bug compatibility for demos
//
// Revision 1.6  1998/05/03  22:38:36  killough
// beautification
//
// Revision 1.5  1998/04/27  02:03:11  killough
// Improve signal handling, to use Z_DumpHistory()
//
// Revision 1.4  1998/03/09  07:10:47  killough
// Allow CTRL-BRK during game init
//
// Revision 1.3  1998/02/03  01:32:58  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.2  1998/01/26  19:23:24  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
