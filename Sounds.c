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
//      Created by a sound utility.
//      Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: sounds.c,v 1.3 1998/05/03 22:44:25 killough Exp $";

// killough 5/3/98: reformatted

#include "doomtype.h"
#include "sounds.h"
#include "p_skin.h"

//
// Information about all the music
//

musicinfo_t S_music[] = {
  { 0 },
  { "e1m1", 0 },
  { "e1m2", 0 },
  { "e1m3", 0 },
  { "e1m4", 0 },
  { "e1m5", 0 },
  { "e1m6", 0 },
  { "e1m7", 0 },
  { "e1m8", 0 },
  { "e1m9", 0 },
  { "e2m1", 0 },
  { "e2m2", 0 },
  { "e2m3", 0 },
  { "e2m4", 0 },
  { "e2m5", 0 },
  { "e2m6", 0 },
  { "e2m7", 0 },
  { "e2m8", 0 },
  { "e2m9", 0 },
  { "e3m1", 0 },
  { "e3m2", 0 },
  { "e3m3", 0 },
  { "e3m4", 0 },
  { "e3m5", 0 },
  { "e3m6", 0 },
  { "e3m7", 0 },
  { "e3m8", 0 },
  { "e3m9", 0 },
  { "inter", 0 },
  { "intro", 0 },
  { "bunny", 0 },
  { "victor", 0 },
  { "introa", 0 },
  { "runnin", 0 },
  { "stalks", 0 },
  { "countd", 0 },
  { "betwee", 0 },
  { "doom", 0 },
  { "the_da", 0 },
  { "shawn", 0 },
  { "ddtblu", 0 },
  { "in_cit", 0 },
  { "dead", 0 },
  { "stlks2", 0 },
  { "theda2", 0 },
  { "doom2", 0 },
  { "ddtbl2", 0 },
  { "runni2", 0 },
  { "dead2", 0 },
  { "stlks3", 0 },
  { "romero", 0 },
  { "shawn2", 0 },
  { "messag", 0 },
  { "count2", 0 },
  { "ddtbl3", 0 },
  { "ampie", 0 },
  { "theda3", 0 },
  { "adrian", 0 },
  { "messg2", 0 },
  { "romer2", 0 },
  { "tense", 0 },
  { "shawn3", 0 },
  { "openin", 0 },
  { "evil", 0 },
  { "ultima", 0 },
  { "read_m", 0 },
  { "dm2ttl", 0 },
  { "dm2int", 0 },
  { "newmus", 0 },
};

//
// Information about all the sfx
//
// killough 12/98: 
// Reimplemented 'singularity' flag, adjusting many sounds below

sfxinfo_t S_sfx[] = {
  { 0 },  // S_sfx[0] needs to be a dummy for odd reasons.

  { "pistol", sg_none,   64, 0, -1, -1, 0 },
  { "shotgn", sg_none,   64, 0, -1, -1, 0 },
  { "sgcock", sg_none,   64, 0, -1, -1, 0 },
  { "dshtgn", sg_none,   64, 0, -1, -1, 0 },
  { "dbopn",  sg_none,   64, 0, -1, -1, 0 },
  { "dbcls",  sg_none,   64, 0, -1, -1, 0 },
  { "dbload", sg_none,   64, 0, -1, -1, 0 },
  { "plasma", sg_none,   64, 0, -1, -1, 0 },
  { "bfg",    sg_none,   64, 0, -1, -1, 0 },
  { "sawup",  sg_none,   64, 0, -1, -1, 0 },
  { "sawidl", sg_none,  118, 0, -1, -1, 0 },
  { "sawful", sg_none,   64, 0, -1, -1, 0 },
  { "sawhit", sg_none,   64, 0, -1, -1, 0 },
  { "rlaunc", sg_none,   64, 0, -1, -1, 0 },
  { "rxplod", sg_none,   70, 0, -1, -1, 0 },
  { "firsht", sg_none,   70, 0, -1, -1, 0 },
  { "firxpl", sg_none,   70, 0, -1, -1, 0 },
  { "pstart", sg_none,  100, 0, -1, -1, 0 },
  { "pstop",  sg_none,  100, 0, -1, -1, 0 },
  { "doropn", sg_none,  100, 0, -1, -1, 0 },
  { "dorcls", sg_none,  100, 0, -1, -1, 0 },
  { "stnmov", sg_none,  119, 0, -1, -1, 0 },
  { "swtchn", sg_none,   78, 0, -1, -1, 0 },
  { "swtchx", sg_none,   78, 0, -1, -1, 0 },
  { "plpain", sg_none,   96, 0, -1, -1, 0, sk_plpain+1 },
  { "dmpain", sg_none,   96, 0, -1, -1, 0 },
  { "popain", sg_none,   96, 0, -1, -1, 0 },
  { "vipain", sg_none,   96, 0, -1, -1, 0 },
  { "mnpain", sg_none,   96, 0, -1, -1, 0 },
  { "pepain", sg_none,   96, 0, -1, -1, 0 },
  { "slop",   sg_none,   78, 0, -1, -1, 0, sk_slop+1 },
  { "itemup", sg_itemup, 78, 0, -1, -1, 0 },
  { "wpnup",  sg_wpnup,  78, 0, -1, -1, 0 },
  { "oof",    sg_oof,    96, 0, -1, -1, 0, sk_oof+1 },
  { "telept", sg_none,   32, 0, -1, -1, 0 },
  { "posit1", sg_none,   98, 0, -1, -1, 0 },
  { "posit2", sg_none,   98, 0, -1, -1, 0 },
  { "posit3", sg_none,   98, 0, -1, -1, 0 },
  { "bgsit1", sg_none,   98, 0, -1, -1, 0 },
  { "bgsit2", sg_none,   98, 0, -1, -1, 0 },
  { "sgtsit", sg_none,   98, 0, -1, -1, 0 },
  { "cacsit", sg_none,   98, 0, -1, -1, 0 },
  { "brssit", sg_none,   94, 0, -1, -1, 0 },
  { "cybsit", sg_none,   92, 0, -1, -1, 0 },
  { "spisit", sg_none,   90, 0, -1, -1, 0 },
  { "bspsit", sg_none,   90, 0, -1, -1, 0 },
  { "kntsit", sg_none,   90, 0, -1, -1, 0 },
  { "vilsit", sg_none,   90, 0, -1, -1, 0 },
  { "mansit", sg_none,   90, 0, -1, -1, 0 },
  { "pesit",  sg_none,   90, 0, -1, -1, 0 },
  { "sklatk", sg_none,   70, 0, -1, -1, 0 },
  { "sgtatk", sg_none,   70, 0, -1, -1, 0 },
  { "skepch", sg_none,   70, 0, -1, -1, 0 },
  { "vilatk", sg_none,   70, 0, -1, -1, 0 },
  { "claw",   sg_none,   70, 0, -1, -1, 0 },
  { "skeswg", sg_none,   70, 0, -1, -1, 0 },
  { "pldeth", sg_none,   32, 0, -1, -1, 0, sk_pldeth+1 },
  { "pdiehi", sg_none,   32, 0, -1, -1, 0, sk_pdiehi+1 },
  { "podth1", sg_none,   70, 0, -1, -1, 0 },
  { "podth2", sg_none,   70, 0, -1, -1, 0 },
  { "podth3", sg_none,   70, 0, -1, -1, 0 },
  { "bgdth1", sg_none,   70, 0, -1, -1, 0 },
  { "bgdth2", sg_none,   70, 0, -1, -1, 0 },
  { "sgtdth", sg_none,   70, 0, -1, -1, 0 },
  { "cacdth", sg_none,   70, 0, -1, -1, 0 },
  { "skldth", sg_none,   70, 0, -1, -1, 0 },
  { "brsdth", sg_none,   32, 0, -1, -1, 0 },
  { "cybdth", sg_none,   32, 0, -1, -1, 0 },
  { "spidth", sg_none,   32, 0, -1, -1, 0 },
  { "bspdth", sg_none,   32, 0, -1, -1, 0 },
  { "vildth", sg_none,   32, 0, -1, -1, 0 },
  { "kntdth", sg_none,   32, 0, -1, -1, 0 },
  { "pedth",  sg_none,   32, 0, -1, -1, 0 },
  { "skedth", sg_none,   32, 0, -1, -1, 0 },
  { "posact", sg_none,  120, 0, -1, -1, 0 },
  { "bgact",  sg_none,  120, 0, -1, -1, 0 },
  { "dmact",  sg_none,  120, 0, -1, -1, 0 },
  { "bspact", sg_none,  100, 0, -1, -1, 0 },
  { "bspwlk", sg_none,  100, 0, -1, -1, 0 },
  { "vilact", sg_none,  100, 0, -1, -1, 0 },
  { "noway",  sg_oof,    78, 0, -1, -1, 0, sk_oof+1  },
  { "barexp", sg_none,   60, 0, -1, -1, 0 },
  { "punch",  sg_none,   64, 0, -1, -1, 0, sk_punch+1 },
  { "hoof",   sg_none,   70, 0, -1, -1, 0 },
  { "metal",  sg_none,   70, 0, -1, -1, 0 },
  { "chgun",  sg_none,   64, &S_sfx[sfx_pistol], 150, 0, 0 },
  { "tink",   sg_none,   60, 0, -1, -1, 0 },
  { "bdopn",  sg_none,  100, 0, -1, -1, 0 },
  { "bdcls",  sg_none,  100, 0, -1, -1, 0 },
  { "itmbk",  sg_none,  100, 0, -1, -1, 0 },
  { "flame",  sg_none,   32, 0, -1, -1, 0 },
  { "flamst", sg_none,   32, 0, -1, -1, 0 },
  { "getpow", sg_getpow, 60, 0, -1, -1, 0 },
  { "bospit", sg_none,   70, 0, -1, -1, 0 },
  { "boscub", sg_none,   70, 0, -1, -1, 0 },
  { "bossit", sg_none,   70, 0, -1, -1, 0 },
  { "bospn",  sg_none,   70, 0, -1, -1, 0 },
  { "bosdth", sg_none,   70, 0, -1, -1, 0 },
  { "manatk", sg_none,   70, 0, -1, -1, 0 },
  { "mandth", sg_none,   70, 0, -1, -1, 0 },
  { "sssit",  sg_none,   70, 0, -1, -1, 0 },
  { "ssdth",  sg_none,   70, 0, -1, -1, 0 },
  { "keenpn", sg_none,   70, 0, -1, -1, 0 },
  { "keendt", sg_none,   70, 0, -1, -1, 0 },
  { "skeact", sg_none,   70, 0, -1, -1, 0 },
  { "skesit", sg_none,   70, 0, -1, -1, 0 },
  { "skeatk", sg_none,   70, 0, -1, -1, 0 },
  { "radio",  sg_none,   60, 0, -1, -1, 0 },

#ifdef DOGS
  // killough 11/98: dog sounds
  { "dgsit",  sg_none,   98, 0, -1, -1, 0 },
  { "dgatk",  sg_none,   70, 0, -1, -1, 0 },
  { "dgact",  sg_none,  120, 0, -1, -1, 0 },
  { "dgdth",  sg_none,   70, 0, -1, -1, 0 },
  { "dgpain", sg_none,   96, 0, -1, -1, 0 },
#endif

  // Start Eternity TC New Sounds
  { "minsit", sg_none,   92, 0, -1, -1, 0 },
  { "minat1", sg_none,   98, 0, -1, -1, 0 },
  { "minat2", sg_none,   98, 0, -1, -1, 0 },
  { "minat3", sg_none,   98, 0, -1, -1, 0 },
  { "minpai", sg_none,   96, 0, -1, -1, 0 },
  { "mindth", sg_none,   32, 0, -1, -1, 0 },
  { "minact", sg_none,   94, 0, -1, -1, 0 },
  { "stfpow", sg_none,   92, 0, -1, -1, 0 },
  { "phohit", sg_none,   60, 0, -1, -1, 0 },
  { "brook1", sg_none,   32, 0, -1, -1, 0 },
//  { "winde",   sg_none,   32, 0, -1, -1, 0 }, // joel - prevent eternity conflict
  { "wind3",  sg_none,   32, 0, -1, -1, 0 },
  { "water1", sg_none,   32, 0, -1, -1, 0 },
  { "amb3",   sg_none,   32, 0, -1, -1, 0 },
  { "amb7",   sg_none,   32, 0, -1, -1, 0 },
  { "amb9",   sg_none,   32, 0, -1, -1, 0 },
  { "amb10",  sg_none,   32, 0, -1, -1, 0 },
  { "bird",   sg_none,   32, 0, -1, -1, 0 },
  { "bugs1",  sg_none,   32, 0, -1, -1, 0 },
  { "bugs2",  sg_none,   32, 0, -1, -1, 0 },
  { "bugs3",  sg_none,   32, 0, -1, -1, 0 },
  { "ktydid", sg_none,   32, 0, -1, -1, 0 },
  { "frogs",  sg_none,   32, 0, -1, -1, 0 },
  { "owl",    sg_none,   32, 0, -1, -1, 0 },
  { "trebrk", sg_none,   92, 0, -1, -1, 0 },
  { "clratk", sg_none,  120, 0, -1, -1, 0 },
  { "clrpn",  sg_none,   96, 0, -1, -1, 0 },
  { "clrdth", sg_none,   32, 0, -1, -1, 0 },
  { "cblsht", sg_none,   70, 0, -1, -1, 0 },
  { "cblexp", sg_none,   70, 0, -1, -1, 0 },
  { "gloop",  sg_none,   32, 0, -1, -1, 0 },
  { "thundr", sg_none,   96, 0, -1, -1, 0 },
  { "muck",   sg_none,   32, 0, -1, -1, 0 },
  { "bubble", sg_none,   32, 0, -1, -1, 0 },
  { "shlurp", sg_none,   32, 0, -1, -1, 0 },
  { "rocks",  sg_none,   32, 0, -1, -1, 0 },
  { "clrdef", sg_none,   60, 0, -1, -1, 0 }, 
  { "dfdth",  sg_none,    70, 0, -1, -1, 0 },
  { "dwrfpn", sg_none,    96, 0, -1, -1, 0 },
  { "dfsit1", sg_none,    98, 0, -1, -1, 0 },
  { "dfsit2", sg_none,    98, 0, -1, -1, 0 },
  { "fdsit",  sg_none,    98, 0, -1, -1, 0 },
  { "fdpn",   sg_none,    96, 0, -1, -1, 0 },
  { "fddth",  sg_none,    70, 0, -1, -1, 0 },
  { "heal",   sg_none,    60, 0, -1, -1, 0 },
  { "harp",   sg_none,    60, 0, -1, -1, 0 },
  { "wofp",   sg_none,    60, 0, -1, -1, 0 },
  { "nspdth", sg_none,    90, 0, -1, -1, 0 },
  { "chains", sg_none,    32, 0, -1, -1, 0 },
  { "glass5", sg_none,    32, 0, -1, -1, 0 },
  { "gong",   sg_none,    32, 0, -1, -1, 0 },
  { "lava2",  sg_none,    32, 0, -1, -1, 0 },
  { "locked", sg_none,    32, 0, -1, -1, 0 },
  { "burn",   sg_none,    32, 0, -1, -1, 0 },
  { "mumhed", sg_none,    96, 0, -1, -1, 0 },
  { "mumsit", sg_none,    98, 0, -1, -1, 0 },
  { "mumpai", sg_none,    96, 0, -1, -1, 0 },
  { "mumdth", sg_none,    70, 0, -1, -1, 0 },
  { "amb1",   sg_none,    32, 0, -1, -1, 0 },
  { "cone3",  sg_none,    60, 0, -1, -1, 0 },
  { "icedth", sg_none,    96, 0, -1, -1, 0 },
  { "plfeet", sg_oof,     96, 0, -1, -1, 0, sk_plfeet+1 },
  { "plfall", sg_none,    96, 0, -1, -1, 0, sk_plfall+1 },
  { "fallht", sg_none,    96, 0, -1, -1, 0, sk_fallht+1 },
  { "clsit1", sg_none,	  92, 0, -1, -1, 0 },
  { "clsit2", sg_none,	  92, 0, -1, -1, 0 },
  { "clsit3", sg_none,	  92, 0, -1, -1, 0 },
  { "clsit4", sg_none,	  92, 0, -1, -1, 0 },
  { "keys2a", sg_itemup,  96, 0, -1, -1, 0 },
  // End Eternity TC New Sounds
  { "buzz", sg_none, 96, 0, -1, -1, 0 }, // joel: ambient sounds
  { "comp", sg_none, 96, 0, -1, -1, 0 },
  { "drip", sg_none, 96, 0, -1, -1, 0 },
  { "fire", sg_none, 96, 0, -1, -1, 0 },
  { "lava", sg_none, 96, 0, -1, -1, 0 },
  { "river", sg_none, 96, 0, -1, -1, 0 },
  { "wfall", sg_none, 96, 0, -1, -1, 0 },
  { "wind", sg_none, 96, 0, -1, -1, 0 },
  { "wind1", sg_none, 96, 0, -1, -1, 0 },
  { "cross",  sg_none,   64, 0, -1, -1, 0 }, // joel: cross sound
  // Somehow more Eternity TC sounds
  { "mpstrt", sg_none,  100, 0, -1, -1, 0 },
  { "mpstop", sg_none,  100, 0, -1, -1, 0 },
  { "mstnmv", sg_none,  119, 0, -1, -1, 0 },
  { "mdropn", sg_none,  100, 0, -1, -1, 0 },
  { "mdrcls", sg_none,  100, 0, -1, -1, 0 },
  { "mbdopn",  sg_none,  100, 0, -1, -1, 0 },
  { "mbdcls",  sg_none,  100, 0, -1, -1, 0 },
};
sfxinfo_t chgun=
  { "chgun",  sg_none,   64, 0, -1, -1, 0 };

//----------------------------------------------------------------------------
//
// $Log: sounds.c,v $
// Revision 1.3  1998/05/03  22:44:25  killough
// beautification
//
// Revision 1.2  1998/01/26  19:24:54  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:03  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------