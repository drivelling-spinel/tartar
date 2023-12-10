// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
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
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/* includes ************************/

#include "z_zone.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "doomstat.h"
#include "doomtype.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "info.h"
#include "m_random.h"
#include "m_fixed.h"
#include "p_chase.h"
#include "p_map.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "p_spec.h"
#include "p_hubs.h"
#include "p_inter.h"
#include "r_data.h"
#include "r_main.h"
#include "r_segs.h"
#include "s_sound.h"
#include "w_wad.h"
#include "hu_fspic.h"
#include "d_dialog.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_script.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"
#include "t_array.h" // haleyjd

extern int firstcolormaplump, lastcolormaplump;      // r_data.c

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);

// functions. SF_ means Script Function not, well.. heh, me

        /////////// actually running a function /////////////


/*******************
  FUNCTIONS
 *******************/

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'

void SF_Print(void)
{
  int i;

  if(!t_argc)
  {
     return;
  }

  // haleyjd: 8-17 stringvalue macro cleaned this up a lot!
  for(i=0; i<t_argc; i++)
    {
      C_Printf(stringvalue(t_argv[i]));
    }
}

        // return a random number from 0 to 255
void SF_Rnd(void)
{
  t_return.type = svt_int;
  t_return.value.i = P_Random(pr_script);
}

// looping section. using the rover, find the highest level
// loop we are currently in and return the section_t for it.

section_t *looping_section(void)
{
  section_t *best = NULL;         // highest level loop we're in
                                  // that has been found so far
  int n;
  
  // check thru all the hashchains

  for(n=0; n<SECTIONSLOTS; n++)
    {
      section_t *current = current_script->sections[n];
      
      // check all the sections in this hashchain
      while(current)
	{
	  // a loop?

	  if(current->type == st_loop)
	    // check to see if it's a loop that we're inside
	    if(rover >= current->start && rover <= current->end)
	      {
		// a higher nesting level than the best one so far?
		if(!best || (current->start > best->start))
		  best = current;     // save it
	      }
	  current = current->next;
	}
    }
  
  return best;    // return the best one found
}

        // "continue;" in FraggleScript is a function
void SF_Continue(void)
{
  section_t *section;
  
  if(!(section = looping_section()) )       // no loop found
    {
      script_error("continue() not in loop\n");
      return;
    }

  rover = section->end;      // jump to the closing brace
}

void SF_Break(void)
{
  section_t *section;
  
  if(!(section = looping_section()) )
    {
      script_error("break() not in loop\n");
      return;
    }

  rover = section->end+1;   // jump out of the loop
}

void SF_Goto(void)
{
  if(t_argc < 1)
    {
      script_error("incorrect arguments to goto\n");
      return;
    }
  
  // check argument is a labelptr
  
  if(t_argv[0].type != svt_label)
    {
      script_error("goto argument not a label\n");
      return;
    }
  
  // go there then if everythings fine
  
  rover = t_argv[0].value.labelptr;

}

void SF_Return(void)
{
  killscript = true;      // kill the script
}

void SF_Include(void)
{
  char tempstr[12];

  if(t_argc < 1)
    {
      script_error("incorrect arguments to include()");
      return;
    }

  if(t_argv[0].type == svt_string)
    strncpy(tempstr, t_argv[0].value.s, 8);
  else
    sprintf(tempstr, "%i", (int)t_argv[0].value.i);
  
  parse_include(tempstr);
}

void SF_Input(void)
{
/*        static char inputstr[128];

        gets(inputstr);

        t_return.type = svt_string;
        t_return.value.s = inputstr;
*/
  C_Printf("input() function not available in doom\a\n");
}

void SF_Beep(void)
{
  C_Printf("\a");
}

void SF_Clock(void)
{
  t_return.type = svt_int;
  t_return.value.i = (gametic*100)/35;
}

    /**************** doom stuff ****************/

void SF_ExitLevel(void)
{
  G_ExitLevel();
}

        // centremsg
void SF_Tip(void)
{
  int i;
  char tempstr[256]="";
  
  if(current_script->trigger->player != &players[displayplayer])
    return;

  // haleyjd: made consistent with other tip functions
  for(i=0; i<t_argc; i++)
    sprintf(tempstr,"%s%s", tempstr, stringvalue(t_argv[i]));
    
  HU_CentreMsg(tempstr);
}

        // tip to a particular player
void SF_PlayerTip(void)
{
  int i, plnum;
  char tempstr[256]="";
  
  if(!t_argc)
    { script_error("player not specified\n"); return;}
  
  plnum = intvalue(t_argv[0]);
  
  // haleyjd: 8-17
  for(i=1; i<t_argc; i++)
    sprintf(tempstr,"%s%s", tempstr, stringvalue(t_argv[i]));
  
  // haleyjd: changed to use HU_CentreMsg as appropriate
  // (was using player_printf ??)
  if(plnum == consoleplayer) 
     // only if this console is the specified player
     HU_CentreMsg(tempstr);
}

        // message player
void SF_Message(void)
{
  int i;
  char tempstr[256]="";
  
  if(current_script->trigger->player != &players[displayplayer])
  {
     return;
  }
  
  // haleyjd: 8-17
  for(i=0; i<t_argc; i++)
    sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

  doom_printf(tempstr);
}

        // message to a particular player
void SF_PlayerMsg(void)
{
  int i, plnum;
  char tempstr[256]="";
  
  if(!t_argc)
    { script_error("player not specified\n"); return;}
  
  plnum = intvalue(t_argv[0]);

  // haleyjd: bounds-checking
  if(plnum < 0 || plnum >= MAXPLAYERS)
  {
     script_error("player number out of range: %i\n", plnum);
     return;
  }
  
  // haleyjd: 8-17
  for(i=1; i<t_argc; i++)
    sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));
  
  player_printf(&players[plnum], tempstr);
}

void SF_PlayerInGame(void)
{
   int plnum;

   if(!t_argc)
   { 
      script_error("player not specified\n"); 
      return;
   }

   plnum = intvalue(t_argv[0]);

   if(plnum < 0 || plnum >= MAXPLAYERS)
   {
      script_error("player number out of range: %i\n", plnum);
      return;
   }
   
   t_return.type = svt_int;
   t_return.value.i = playeringame[plnum];
}

void SF_PlayerName(void)
{
  int plnum;
  
  if(!t_argc)
    {
      player_t *pl;
      pl = current_script->trigger->player;
      if(pl) plnum = pl - players;
      else
	{
	  script_error("script not started by player\n");
	  return;
	}
    }
  else
    plnum = intvalue(t_argv[0]);

  if(plnum < 0 || plnum >= MAXPLAYERS)
  {
     script_error("player number out of range: %i\n", plnum);
     return;
  }
  
  t_return.type = svt_string;
  t_return.value.s = players[plnum].name;
}

        // object being controlled by player
void SF_PlayerObj(void)
{
  int plnum;
  
  if(!t_argc)
    {
      player_t *pl;
      pl = current_script->trigger->player;
      if(pl) plnum = pl - players;
      else
	{
	  script_error("script not started by player\n");
	  return;
	}
    }
  else
    plnum = intvalue(t_argv[0]);

  if(plnum < 0 || plnum >= MAXPLAYERS)
  {
     script_error("player number out of range: %i\n", plnum);
     return;
  }
  
  t_return.type = svt_mobj;
  t_return.value.mobj = players[plnum].mo;
}

extern void SF_StartScript();      // in t_script.c
extern void SF_ScriptRunning();
extern void SF_Wait();
extern void SF_TagWait();
extern void SF_ScriptWait();
extern void SF_ScriptWaitPre();    // haleyjd: new wait types

/*********** Mobj code ***************/

void SF_Player(void)
{
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) :
    current_script->trigger;
  
  t_return.type = svt_int;
  
  if(mo && mo->player) // haleyjd: added mo->player
    {
      t_return.value.i = (int)(mo->player - players);
    }
  else
    {
      t_return.value.i = -1;
    }
}

//
// SF_Spawn
// 
// Implements: mobj spawn(int type, int x, int y, [int angle], [int z])
//
void SF_Spawn(void)
{
  int x, y, z, objtype;
  mobjinfo_t *typeinfo;
  angle_t angle = 0;

  
  if(t_argc < 3)
    { script_error("insufficient arguments to function\n"); return; }
  
  objtype = intvalue(t_argv[0]);
  // invalid object to spawn
  if(objtype < 0 || objtype >= NUMMOBJTYPES)
    { script_error("unknown object type: %i\n", objtype); return; }

  // haleyjd: retrieve the mobjinfo_t for this type
  typeinfo = &mobjinfo[objtype];

  x = intvalue(t_argv[1]) * FRACUNIT;
  y = intvalue(t_argv[2]) * FRACUNIT;
  
  // haleyjd: SoM's spawn z-coordinate fix
  if(t_argc >= 5)
     z = intvalue(t_argv[4]) * FRACUNIT;
  else
     z = (typeinfo->flags & MF_SPAWNCEILING) ? ONCEILINGZ : ONFLOORZ;
  
  // haleyjd: SoM's angle fix
  if(t_argc >= 4)
  {
     angle = intvalue(t_argv[3]) * (45 / ANG45);
  }
  
  t_return.type = svt_mobj;
  t_return.value.mobj = P_SpawnMobj(x, y, z, objtype);
  t_return.value.mobj->angle = angle;
}

void SF_RemoveObj(void)
{
  mobj_t *mo;
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  mo = MobjForSvalue(t_argv[0]);
  if(mo)  // nullptr check
    P_RemoveMobj(mo);
}

void SF_KillObj(void)
{
  mobj_t *mo;
  
  if(t_argc)
     mo = MobjForSvalue(t_argv[0]);
  else
     mo = current_script->trigger;  // default to trigger object

  if(mo)  // nullptr check
    P_KillMobj(current_script->trigger, mo);         // kill it
}

        // mobj x, y, z
void SF_ObjX(void)
{
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

  t_return.type = svt_fixed;           // haleyjd: SoM's fixed-point fix
  t_return.value.f = mo ? mo->x : 0;   // null ptr check
}

void SF_ObjY(void)
{
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

  t_return.type = svt_fixed;         // haleyjd
  t_return.value.f = mo ? mo->y : 0; // null ptr check
}

void SF_ObjZ(void)
{
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

  t_return.type = svt_fixed;         // haleyjd
  t_return.value.f = mo ? mo->z : 0; // null ptr check
}

        // mobj angle
void SF_ObjAngle(void)
{
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

  t_return.type = svt_fixed; // haleyjd: fixed-point -- SoM again :)
  t_return.value.f = mo ? AngleToFixed(mo->angle) : 0;   // null ptr check
}


        // teleport: object, sector_tag
void SF_Teleport(void)
{
   line_t line;    // dummy line for teleport function
   mobj_t *mo;
   
   if(!t_argc)   // no arguments
   {
      script_error("insufficient arguments to function\n");
      return;
   }
   else if(t_argc == 1)    // 1 argument: sector tag
   {
      mo = current_script->trigger;   // default to trigger
      line.tag = intvalue(t_argv[0]);
   }
   else    // 2 or more
   {                       // teleport a given object
      mo = MobjForSvalue(t_argv[0]);
      line.tag = intvalue(t_argv[1]);
   }
   
   if(mo)
      EV_Teleport(&line, 0, mo);
}

void SF_SilentTeleport(void)
{
  line_t line;    // dummy line for teleport function
  mobj_t *mo;
  
  if(!t_argc)   // no arguments
    {
      script_error("insufficient arguments to function\n");
      return;
    }
  else if(t_argc == 1)    // 1 argument: sector tag
    {
      mo = current_script->trigger;   // default to trigger
      line.tag = intvalue(t_argv[0]);
    }
  else    // 2 or more
    {                       // teleport a given object
      mo = MobjForSvalue(t_argv[0]);
      line.tag = intvalue(t_argv[1]);
    }

  if(mo)
    EV_SilentTeleport(&line, 0, mo);
}

void SF_DamageObj(void)
{
  mobj_t *mo;
  int damageamount;

  if(!t_argc)   // no arguments
    {
      script_error("insufficient arguments to function\n");
      return;
    }
  else if(t_argc == 1)    // 1 argument: damage trigger by amount
    {
      mo = current_script->trigger;   // default to trigger
      damageamount = intvalue(t_argv[0]);
    }
  else    // 2 or more
    {                       // damage a given object
      mo = MobjForSvalue(t_argv[0]);
      damageamount = intvalue(t_argv[1]);
    }
  
  if(mo)
    P_DamageMobj(mo, NULL, current_script->trigger, damageamount);
}

        // the tag number of the sector the thing is in
void SF_ObjSector(void)
{
  // use trigger object if not specified
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
  
  t_return.type = svt_int;
  t_return.value.i = mo ? mo->subsector->sector->tag : 0; // nullptr check
}

        // the health number of an object
void SF_ObjHealth(void)
{
  // use trigger object if not specified
  mobj_t *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

  t_return.type = svt_int;
  t_return.value.i = mo ? mo->health : 0;
}

void SF_ObjFlag(void)
{
  mobj_t *mo;
  int flagnum;
  
  // haleyjd: major reformatting

  if(!t_argc)   // no arguments
  {
     script_error("no arguments for function\n");
     return;
  }
  else if(t_argc == 1)         // use trigger, 1st is flag
  {
     // use trigger:
     mo = current_script->trigger;
     flagnum = intvalue(t_argv[0]);
  }
  else if(t_argc == 2)
  {
     // specified object
     mo = MobjForSvalue(t_argv[0]);
     flagnum = intvalue(t_argv[1]);
  }
  else                     // >= 3 : SET flags
  {
     mo = MobjForSvalue(t_argv[0]);
     flagnum = intvalue(t_argv[1]);
     
     if(mo)          // nullptr check
     {
	long newflag;
	// remove old bit
	mo->flags = mo->flags & ~(1 << flagnum);
	
	// make the new flag
	newflag = (!!intvalue(t_argv[2])) << flagnum;
	mo->flags |= newflag;   // add new flag to mobj flags
	
	// haleyjd: this was outside the nullptr check!
	P_UpdateThinker(&(mo->thinker));     // update thinker
     }     
  }
  
  t_return.type = svt_int;  
  t_return.value.i = mo ? !!(mo->flags & (1 << flagnum)) : 0;
}

        // apply momentum to a thing
void SF_PushThing(void)
{
  mobj_t *mo;
  angle_t angle;
  fixed_t force;
  
  if(t_argc<3)   // missing arguments
    {
      script_error("insufficient arguments for function\n");
      return;
    }

  mo = MobjForSvalue(t_argv[0]);
  
  if(!mo)
  {
     return;
  }
  
  // haleyjd: more of SoM's fixed-point work
  angle = FixedToAngle(fixedvalue(t_argv[1]));
  force = fixedvalue(t_argv[2]);
  
  mo->momx += FixedMul(finecosine[angle >> ANGLETOFINESHIFT], force);
  mo->momy += FixedMul(finesine[angle >> ANGLETOFINESHIFT], force);
}

// New Functions By SoM
//  SF_ReactionTime -- useful for freezing things
//  SF_MobjTarget   -- sets a thing's target field
//  SF_MobjMomx, MobjMomy, MobjMomz -- momentum functions

void SF_ReactionTime(void)
{
  mobj_t *mo;

  if(t_argc < 1)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  mo = MobjForSvalue(t_argv[0]);

  if(t_argc > 1)
  {
     if(mo) 
	mo->reactiontime = (intvalue(t_argv[1]) * 35) / 100;
  }

  t_return.type = svt_int;
  t_return.value.i = mo ? mo->reactiontime : 0;
}

// Sets a mobj's Target! >:)
void SF_MobjTarget(void)
{
  mobj_t*  mo;
  mobj_t*  target;

  if(t_argc < 1)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  mo = MobjForSvalue(t_argv[0]);
  if(t_argc > 1)
  {
    target = MobjForSvalue(t_argv[1]);
    if(mo && target) // haleyjd: added target check -- no NULL allowed
    {
       P_SetTarget(&mo->target, target);
       P_SetMobjState(mo, mo->info->seestate);
       mo->flags |= MF_JUSTHIT;
    }
  }
  
  t_return.type = svt_mobj;
  t_return.value.mobj = mo ? mo->target : NULL;
}

void SF_MobjMomx(void)
{
  mobj_t*   mo;

  if(t_argc < 1)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  mo = MobjForSvalue(t_argv[0]);
  if(t_argc > 1)
  {
     if(mo) 
	mo->momx = fixedvalue(t_argv[1]);
  }

  t_return.type = svt_fixed;
  t_return.value.f = mo ? mo->momx : 0;
}

void SF_MobjMomy(void)
{
  mobj_t*   mo;

  if(t_argc < 1)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  mo = MobjForSvalue(t_argv[0]);
  if(t_argc > 1)
  {
     if(mo)
	mo->momy = fixedvalue(t_argv[1]);
  }

  t_return.type = svt_fixed;
  t_return.value.f = mo ? mo->momy : 0;
}

void SF_MobjMomz(void)
{
  mobj_t*   mo;

  if(t_argc < 1)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  mo = MobjForSvalue(t_argv[0]);
  if(t_argc > 1)
  {
     if(mo)
	mo->momz = fixedvalue(t_argv[1]);
  }

  t_return.type = svt_fixed;
  t_return.value.f = mo ? mo->momz : 0;
}

/****************** Trig *********************/

void SF_PointToAngle(void)
{
  angle_t angle;
  int x1, y1, x2, y2;

  if(t_argc<4)
    {
      script_error("insufficient arguments to function\n");
      return;
    }

  // haleyjd: SoM's fixed-pt fixes
  x1 = intvalue(t_argv[0]) << FRACBITS;
  y1 = intvalue(t_argv[1]) << FRACBITS;
  x2 = intvalue(t_argv[2]) << FRACBITS;
  y2 = intvalue(t_argv[3]) << FRACBITS;
                            
  angle = R_PointToAngle2(x1, y1, x2, y2);

  t_return.type = svt_fixed;
  t_return.value.f = AngleToFixed(angle);
}

void SF_PointToDist(void)
{
  int dist;
  int x1, x2, y1, y2;
  
  if(t_argc<4)
    {
      script_error("insufficient arguments to function\n");
      return; 
    }

  // haleyjd: SoM's fixed-pt fixes
  x1 = intvalue(t_argv[0]) << FRACBITS;
  y1 = intvalue(t_argv[1]) << FRACBITS;
  x2 = intvalue(t_argv[2]) << FRACBITS;
  y2 = intvalue(t_argv[3]) << FRACBITS;
                            
  dist = R_PointToDist2(x1, y1, x2, y2);
  t_return.type = svt_fixed;
  t_return.value.f = dist;
}

/************* Camera functions ***************/

camera_t script_camera;
camera_t *script_prev_camera = NULL;

// setcamera(obj, [angle], [height], [pitch])
//
// haleyjd: significantly improved with ideas by SoM and myself

void SF_SetCamera(void)
{
  mobj_t *mo;
  angle_t angle;
  int pitch;

  if(t_argc < 1)
    {
      script_error("insufficient arguments to function\n");
      return;
    }

  mo = MobjForSvalue(t_argv[0]);
  if(!mo)
  {
     script_error("invalid location object for camera\n");
     return;         // nullptr check
  }
  
  angle = t_argc < 2 ? mo->angle : FixedToAngle(fixedvalue(t_argv[1]));
  
  script_camera.x = mo->x;
  script_camera.y = mo->y;
  script_camera.z = t_argc < 3 ? (mo->z + (41 << FRACBITS)) : (intvalue(t_argv[2]) << FRACBITS);
  script_camera.heightsec = mo->subsector->sector->heightsec;
  script_camera.angle = angle;
  if(t_argc < 4)
    script_camera.updownangle = 0;
  else
  {
    pitch = intvalue(t_argv[3]);
    if(pitch < -50) pitch = -50;
    if(pitch > 50)  pitch =  50;
    script_camera.updownangle = pitch;
  }
  
  script_prev_camera = camera;
  camera = &script_camera;
}

void SF_ClearCamera(void)
{
   // haleyjd: use script_prev_camera in case the player
   // was using walk or chasecam (not necessary to save in savegames)
  camera = script_prev_camera;          // turn off camera
}

/*********** sounds ******************/

        // start sound from thing
void SF_StartSound(void)
{
  mobj_t *mo;
  
  if(t_argc < 2)
    {
      script_error("insufficient arguments to function\n");
      return; 
    }
  
  if(t_argv[1].type != svt_string)
    {
      script_error("sound lump argument not a string!\n");
      return;
    }

  mo = MobjForSvalue(t_argv[0]);
  
  S_StartSoundName(mo, t_argv[1].value.s);
}

        // start sound from sector
void SF_StartSectorSound(void)
{
  sector_t *sector;
  int tagnum, secnum;

  if(t_argc < 2)
    { script_error("insufficient arguments to function\n"); return; }
  if(t_argv[1].type != svt_string)
    { script_error("sound lump argument not a string!\n"); return;}

  tagnum = intvalue(t_argv[0]);
  // argv is sector tag
  
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];
  
  S_StartSoundName((mobj_t *)&sector->soundorg, t_argv[1].value.s);
}

/************* Sector functions ***************/

        // floor height of sector
void SF_FloorHeight(void)
{
  sector_t *sector;
  int tagnum;
  int secnum;
  fixed_t dest;
  int returnval = 1; // haleyjd: SoM's fixes
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);

  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];

  if(t_argc > 1)          // > 1: set floorheight
  {
      int i;
      boolean crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;

      i = -1;
      dest = fixedvalue(t_argv[1]);
      
      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
      {
	 //sectors[i].floorheight = intvalue(t_argv[1]) * FRACUNIT;
	 if(T_MovePlane(&sectors[i], 
	      abs(dest - sectors[i].floorheight), dest, crush, 0, 
	      (dest > sectors[i].floorheight) ? 1 : -1) 
	    == crushed)
	 {
	    returnval = 0;
	 }	    
      }
  }
  else
     returnval = sectors[secnum].floorheight >> FRACBITS;

  // return floorheight

  t_return.type = svt_int;
  t_return.value.i = returnval;
}

void SF_MoveFloor(void)
{
  int secnum = -1;
  sector_t *sec;
  floormove_t *floor;
  int tagnum, platspeed = 1, destheight;
  
  if(t_argc < 2)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  destheight = intvalue(t_argv[1]) * FRACUNIT;
  platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);
  
  // move all sectors with tag

  while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
      sec = &sectors[secnum];
      
      // Don't start a second thinker on the same floor
      if (P_SectorActive(floor_special,sec))
	continue;
      
      floor = Z_Malloc(sizeof(floormove_t), PU_LEVSPEC, 0);
      P_AddThinker(&floor->thinker);
      sec->floordata = floor;
      floor->thinker.function = T_MoveFloor;
      floor->type = -1;   // not done by line
      floor->crush = false;

      floor->direction =
	destheight < sec->floorheight ? plat_down : plat_up;
      floor->sector = sec;
      floor->speed = platspeed;
      floor->floordestheight = destheight;
    }
}

        // ceiling height of sector
void SF_CeilingHeight(void)
{
  sector_t *sector;
  fixed_t dest;
  int secnum;
  int tagnum;
  int returnval = 1;
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  
  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];
  
  if(t_argc > 1)          // > 1: set ceilheight
  {
      int i;
      boolean crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;

      i = -1;
      dest = fixedvalue(t_argv[1]);

      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
      {
	  //sectors[i].ceilingheight = intvalue(t_argv[1]) * FRACUNIT;
	 if(T_MovePlane(&sectors[i],
	      abs(dest - sectors[i].ceilingheight), dest, crush, 1,
	      (dest > sectors[i].ceilingheight) ? 1 : -1)
	    == crushed)
	 {
	    returnval = 0;
	 }
      }
  }
  else
     returnval = sectors[secnum].ceilingheight >> FRACBITS;
  
  // return ceiling height
  t_return.type = svt_int;
  t_return.value.i = returnval;
}

void SF_MoveCeiling(void)
{
  int secnum = -1;
  sector_t *sec;
  ceiling_t *ceiling;
  int tagnum, platspeed = 1, destheight;

  if(t_argc < 2)
    { script_error("insufficient arguments to function\n"); return; }

  tagnum = intvalue(t_argv[0]);
  destheight = intvalue(t_argv[1]) * FRACUNIT;
  platspeed = FLOORSPEED * (t_argc > 2 ? intvalue(t_argv[2]) : 1);

  // move all sectors with tag
  
  while ((secnum = P_FindSectorFromTag(tagnum, secnum)) >= 0)
    {
      sec = &sectors[secnum];
      
      // Don't start a second thinker on the same floor
      if (P_SectorActive(ceiling_special,sec))
	continue;
      
      ceiling = Z_Malloc(sizeof(*ceiling), PU_LEVSPEC, 0);
      P_AddThinker(&ceiling->thinker);
      sec->ceilingdata = ceiling;
      ceiling->thinker.function = T_MoveCeiling;
      ceiling->type = genCeiling;   // not done by line
      ceiling->crush = false;
      
      ceiling->direction =
	destheight < sec->ceilingheight ? plat_down : plat_up;
      ceiling->sector = sec;
      ceiling->speed = platspeed;
      // just set top and bottomheight the same
      ceiling->topheight = ceiling->bottomheight = destheight;
      
      ceiling->tag = sec->tag;
      P_AddActiveCeiling(ceiling);
    }
}

void SF_LightLevel(void)
{
  sector_t *sector;
  int secnum;
  int tagnum;
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  
  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];
  
  if(t_argc > 1)          // > 1: set light level
    {
      int i = -1;
      
      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
	{
	  sectors[i].lightlevel = intvalue(t_argv[1]);
	}
    }
  
  // return lightlevel
  t_return.type = svt_int;
  t_return.value.i = sector->lightlevel;
}

void SF_FadeLight(void)
{
  int sectag, destlevel, speed = 1;
  
  if(t_argc < 2)
    { script_error("insufficient arguments to function\n"); return; }
  
  sectag = intvalue(t_argv[0]);
  destlevel = intvalue(t_argv[1]);
  speed = t_argc>2 ? intvalue(t_argv[2]) : 1;
  
  P_FadeLight(sectag, destlevel, speed);
}

void SF_FloorTexture(void)
{
  int tagnum, secnum;
  sector_t *sector;
  char floortextname[10];
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  
  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];
  
  if(t_argc > 1)
    {
      int i = -1;
      int picnum = R_FlatNumForName(t_argv[1].value.s);
      
      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
	{
	  sectors[i].floorpic = picnum;
	}
    }
  
  strncpy(floortextname, lumpinfo[sector->floorpic + firstflat]->name, 8);
  
  t_return.type = svt_string;
  t_return.value.s = Z_Strdup(floortextname, PU_LEVEL, 0);
}

void SF_SectorColormap(void)
{
  int tagnum, secnum;
  sector_t *sector;
  char cmapname[10];
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  
  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
  
  sector = &sectors[secnum];
  
  if(t_argc > 1)
    {
      int i = -1;
      int mapnum = R_ColormapNumForName(t_argv[1].value.s);
      
      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
	{
	  if(mapnum == -1)
	    {
	      sectors[i].midmap = 0;
	    }
	  else
	    {
	      sectors[i].midmap = mapnum;
	      sectors[i].heightsec = i;
	    }
	}
    }

  // haleyjd: this was setting the string to the actual lump name,
  // which means the user could actually alter lumpinfo[]. Dangerous.
  // Z_Strdup *must* be used to copy all FS string values.

  strncpy(cmapname, lumpinfo[firstcolormaplump + sector->midmap]->name, 8);
    
  t_return.type = svt_string;
  t_return.value.s = Z_Strdup(cmapname, PU_LEVEL, 0);
}

void SF_CeilingTexture(void)
{
  int tagnum, secnum;
  sector_t *sector;
  char floortextname[10];
  
  if(!t_argc)
    { script_error("insufficient arguments to function\n"); return; }
  
  tagnum = intvalue(t_argv[0]);
  
  // argv is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}

  sector = &sectors[secnum];
  
  if(t_argc > 1)
    {
      int i = -1;
      int picnum = R_FlatNumForName(t_argv[1].value.s);
      
      // set all sectors with tag
      while ((i = P_FindSectorFromTag(tagnum, i)) >= 0)
	{
	  sectors[i].ceilingpic = picnum;
	}
    }
  
  strncpy(floortextname, lumpinfo[sector->ceilingpic + firstflat]->name, 8);

  t_return.type = svt_string;
  t_return.value.s = Z_Strdup(floortextname, PU_LEVEL, 0);
}

// haleyjd: disabled until hub system is rewritten
void SF_ChangeHubLevel(void)
{
#ifdef ETERNITY_BETA_ENABLES
  int tagnum;

  if(!t_argc)
    {
      script_error("hub level to go to not specified!\n");
      return;
    }
  if(t_argv[0].type != svt_string)
    {
      script_error("level argument is not a string!\n");
      return;
    }

  // second argument is tag num for 'seamless' travel
  if(t_argc > 1)
    tagnum = intvalue(t_argv[1]);
  else
    tagnum = -1;

  P_SavePlayerPosition(current_script->trigger->player, tagnum);
  P_HubChangeLevel(t_argv[0].value.s);
#else
  script_error("hub system disabled in eternity v3.29\n");
#endif
}

// for start map: start new game on a particular skill
void SF_StartSkill(void)
{
  int skill;

  if(t_argc < 1) 
    { script_error("need skill level to start on\n"); return;}

  // -1: 1-5 is how we normally see skills
  // 0-4 is how doom sees them

  skill = intvalue(t_argv[0]) - 1;

  G_DeferedInitNew(skill, firstlevel);
}

//////////////////////////////////////////////////////////////////////////
//
// Doors
//

// opendoor(sectag, [delay], [speed])

void SF_OpenDoor(void)
{
  int speed, wait_time;
  int sectag;
  
  if(t_argc < 1)
    {
      script_error("need sector tag for door to open\n");
      return;
    }

  // got sector tag
  sectag = intvalue(t_argv[0]);

  // door wait time
  
  if(t_argc > 1)    // door wait time
    wait_time = (intvalue(t_argv[1]) * 35) / 100;
  else
    wait_time = 0;  // 0= stay open
  
  // door speed

  if(t_argc > 2)
    speed = intvalue(t_argv[2]);
  else
    speed = 1;    // 1= normal speed
  
  EV_OpenDoor(sectag, speed, wait_time);  
}

void SF_CloseDoor(void)
{
  int speed;
  int sectag;
  
  if(t_argc < 1)
    {
      script_error("need sector tag for door to open\n");
      return;
    }

  // got sector tag
  sectag = intvalue(t_argv[0]);

  // door speed

  if(t_argc > 1)
    speed = intvalue(t_argv[1]);
  else
    speed = 1;    // 1= normal speed
  
  EV_CloseDoor(sectag, speed);
}

// run console cmd

void SF_RunCommand(void)
{
  int i;
  char tempstr[1024] = ""; // haleyjd: increased buffer size
  
  // haleyjd: changed to use stringvalue macro for consistency
  for(i=0; i<t_argc; i++)
     sprintf(tempstr,"%s%s", tempstr, stringvalue(t_argv[i]));
    
  cmdtype = c_typed;
  C_RunTextCmd(tempstr);
}

// any linedef type

void SF_LineTrigger()
{
  line_t junk;
  
  if(!t_argc)
    {
      script_error("need line trigger type\n");
      return;
    }
  
  junk.special = intvalue(t_argv[0]);
  junk.tag = t_argc > 1 ? intvalue(t_argv[1]) : 0;
  
  if (!P_UseSpecialLine(t_trigger, &junk, 0))    // Try using it
    P_CrossSpecialLine(&junk, 0, t_trigger);   // Try crossing it
}

void SF_ChangeMusic(void)
{
  if(!t_argc)
    {
      script_error("need new music name\n");
      return;
    }
  if(t_argv[0].type != svt_string)
    { script_error("incorrect argument to function\n"); return;}

  S_ChangeMusicName(t_argv[0].value.s, 1);
}

//==============================
// haleyjd: Eternity Extensions
//
// Note:
//   These are non-standard
//   FraggleScript functions
//==============================

// SF_ObjFlag2
//
//  Same as SF_ObjFlag, but for the flags2 field :)

void SF_ObjFlag2(void)
{
   mobj_t *mo;
   int flagnum;
   
   if(!t_argc)   // no arguments
   {
      script_error("insufficient arguments to function\n"); 
      return; 
   }
   else if(t_argc == 1)         // use trigger, 1st is flag
   {
      // use trigger:
      mo = current_script->trigger;
      flagnum = intvalue(t_argv[0]);
   }
   else if(t_argc == 2)
   {
      // specified object
      mo = MobjForSvalue(t_argv[0]);
      flagnum = intvalue(t_argv[1]);
   }
   else                     // >= 3 : SET flags
   {
      mo = MobjForSvalue(t_argv[0]);
      flagnum = intvalue(t_argv[1]);
      
      if(mo)          // nullptr check
      {
	 long newflag;
	 
	 // remove old bit
	 mo->flags2 = mo->flags2 & ~(1 << flagnum);
	 
	 // make the new flag
	 newflag = (!!intvalue(t_argv[2])) << flagnum;
	 mo->flags2 |= newflag;   // add new flag to mobj flags
	 P_UpdateThinker(&(mo->thinker));
      }
   }
   
   t_return.type = svt_int;
   t_return.value.i = mo ? !!(mo->flags2 & (1 << flagnum)) : 0;
}

/*
   SF_SpawnShot

     Spawns a projectile
     Can be shot by specified thing and/or at specified thing,
     else default are shot by trigger and shot at trigger's target.
*/
void SF_SpawnShot(void)
{
   int objtype;
   mobj_t *source = NULL;
   mobj_t *dest = NULL;

   // t_argv[0] = type to spawn
   // t_argv[1] = source mobj, optional, -1 to default
   // t_argv[2] = destination mobj, optional, -1 to default
   // t_argv[3] = boolean, face target or not
   
   if(t_argc < 4)
   {
      script_error("insufficient arguments to function\n");
      return;
   }
   
   if(t_argv[1].type == svt_int && t_argv[1].value.i < 0)
      source = current_script->trigger;
   else
      source = MobjForSvalue(t_argv[1]);
   
   if(t_argv[2].type == svt_int && t_argv[2].value.i < 0)
      dest = current_script->trigger->target;
   else
      dest = MobjForSvalue(t_argv[2]);
   
   if(!source || !dest)
      return;
   
   if(intvalue(t_argv[3]))
   {
      source->flags &= ~MF_AMBUSH;
      source->angle = R_PointToAngle2(source->x, source->y,
	                              dest->x, dest->y);
      if(dest->flags & MF_SHADOW || dest->flags2 & MF2_DONTDRAW) 
      { 
	 int t = P_Random(pr_facetarget);
	 source->angle += (t - P_Random(pr_facetarget))<<21;
      }
   }
   
   objtype = intvalue(t_argv[0]);
   
   if(objtype < 0 || objtype >= NUMMOBJTYPES)
   {
      script_error("unknown object type: %i\n", objtype);
      return;
   }
   
   t_return.type = svt_mobj;
   t_return.value.mobj = P_SpawnMissile(source, dest, objtype);
}

/*
   SF_CheckLife()
     Due to lack of comparison <= and >= operators, it is impractical to
     check if something is really dead or not, could have health <= 0,
     so this really is necessary :)

     haleyjd: retained as utility function after addition of comparisons
*/
void SF_CheckLife(void)
{
   mobj_t *mo;

   if(!t_argc)
   {
      mo = current_script->trigger;
   }
   else
      mo = MobjForSvalue(t_argv[0]);

   t_return.type = svt_int;
   t_return.value.i = mo ? (mo->health > 0) : 0;
}

/*
   SF_SetLineBlocking()

     Sets a line blocking or unblocking

     setlineblocking(tag, [1|0]);
*/
void SF_SetLineBlocking(void)
{
   line_t *line;
   int blocking;
   int searcher = -1;

   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   blocking = intvalue(t_argv[1]) ? ML_BLOCKING : 0;

   while((line = P_FindLine(intvalue(t_argv[0]), &searcher)) != NULL)
   {
      line->flags = (line->flags & ~ML_BLOCKING) | blocking;
   }
}

// similar, but monster blocking

void SF_SetLineMonsterBlocking(void)
{
   line_t *line;
   int blocking;
   int searcher = -1;

   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   blocking = intvalue(t_argv[1]) ? ML_BLOCKMONSTERS : 0;

   while((line = P_FindLine(intvalue(t_argv[0]), &searcher)) != NULL)
   {
      line->flags = (line->flags & ~ML_BLOCKMONSTERS) | blocking;
   }
}

/*
   SF_SetLineTexture
     
     #2 in a not-so-long line of ACS-inspired functions
     This one is *much* needed, IMO

     setlinetexture(tag, side, position, texture)
*/

#define TEXTURE_BOTTOM 1
#define TEXTURE_MIDDLE 2

void SF_SetLineTexture(void)
{
   line_t *line;
   int tag;
   int side;
   int position;
   char *texture;
   int texturenum;
   int searcher;

   if(t_argc != 4)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   tag = intvalue(t_argv[0]);
   side = intvalue(t_argv[1]);   
   if(side < 0 || side > 1)
   {
      script_error("invalid side number for texture change\n");
      return;
   }
   
   position = intvalue(t_argv[2]);
   if(position < 1 || position > 3)
   {
      script_error("invalid position for texture change\n");
      return;
   }
   
   texture = stringvalue(t_argv[3]);
   texturenum = R_TextureNumForName(texture);
   
   searcher = -1;
   
   while((line = P_FindLine(tag, &searcher)) != NULL)
   {
      // bad sidedef, Hexen just SEGV'd here!
      if((unsigned short)(line->sidenum[side]) == 0xffff)
        continue;

      if(position == TEXTURE_MIDDLE)
      {
         sides[(unsigned short)(line->sidenum[side])].midtexture = texturenum;
      }
      else if(position == TEXTURE_BOTTOM)
      {
         sides[(unsigned short)(line->sidenum[side])].bottomtexture = texturenum;
      }
      else
      { // TEXTURE_TOP
         sides[(unsigned short)(line->sidenum[side])].toptexture = texturenum;
      }
   }
}

/*
   SF_SetFriction

     With the friction savegame fix in place in p_saveg.c, the
     path is paved for this function, which will alter the friction
     of the tagged sectors.

     setfriction(tag, amount)

       - amount is the same as the length of the linedef you'd use
         to normally obtain the same friction value:
	 amount < 100, mud
	 amount > 100, ice
*/
void SF_SetFriction(void)
{
  int tagnum, secnum, fvalue, fr, mvf, i;

  if(t_argc != 2)
  { 
     script_error("insufficient arguments to function\n");
     return; 
  }

  tagnum = intvalue(t_argv[0]);
  fvalue = intvalue(t_argv[1]);
  
  // ugly hard-coded constants for computing friction!
  fr = (0x1EB8*fvalue)/0x80 + 0xD000;   // set friction
  mvf = ((0x10092 - fr)*(0x70))/0x158;  // set movefactor

  // argv[0] is sector tag
  secnum = P_FindSectorFromTag(tagnum, -1);
  
  if(secnum < 0)
  {  
     // no error, just ignore it
     return;
  }

  i = -1;
      
  // set all sectors with tag
  while((i = P_FindSectorFromTag(tagnum, i)) >= 0)
  {
     sectors[i].friction   = fr;
     sectors[i].movefactor = mvf;
     sectors[i].special |= FRICTION_MASK; // give friction mask
  }
}

/*
  SF_GetFriction

    Returns the friction value of the first sector with this tag
    number -- a bit limited, but its the best solution currently
    available. (ExtraData should fix this problem)

    getfriction(tag)
*/
void SF_GetFriction(void)
{
   int tagnum, secnum, fr;
   
   if(!t_argc)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   tagnum = intvalue(t_argv[0]);
   secnum = P_FindSectorFromTag(tagnum, -1);

   if(secnum < 0)
   {
      return;
   }

   fr = ((sectors[secnum].friction - 0xD000)*0x80)/0x1EB8;

   t_return.type = svt_int;
   t_return.value.i = fr;
}

/*

  SF_SpawnShotAngle

  A more general-purpose missile firing function, more like the
  one found in ACS -- this one requires only a source mobj and an
  angle to fire at.

*/
void SF_SpawnShotAngle(void)
{
}

// SoM: Max, Min, Abs math functions.
void SF_Max(void)
{
  fixed_t n1, n2;

  if(t_argc != 2)
  {
    script_error("insufficient arguments to function\n");
    return;
  }

  n1 = fixedvalue(t_argv[0]);
  n2 = fixedvalue(t_argv[1]);

  t_return.type = svt_fixed;
  t_return.value.f = (n1 > n2) ? n1 : n2;
}



void SF_Min(void)
{
  fixed_t   n1, n2;

  if(t_argc != 2)
  {
    script_error("invalid number of arguments\n");
    return;
  }

  n1 = fixedvalue(t_argv[0]);
  n2 = fixedvalue(t_argv[1]);

  t_return.type = svt_fixed;
  t_return.value.f = (n1 < n2) ? n1 : n2;
}


void SF_Abs(void)
{
  fixed_t   n1;

  if(t_argc != 1)
  {
    script_error("invalid number of arguments\n");
    return;
  }

  n1 = fixedvalue(t_argv[0]);

  t_return.type = svt_fixed;
  t_return.value.f = (n1 < 0) ? -n1 : n1;
}

// continue new Eternity functions

/* 
   SF_Gameskill, SF_Gamemode

   Access functions are more elegant for these than variables, 
   especially for the game mode, which doesn't exist as a numeric 
   variable already.
*/
void SF_Gameskill(void)
{
   t_return.type = svt_int;
   t_return.value.i = gameskill + 1;  // +1 for the user skill value
}

void SF_Gamemode(void)
{
   t_return.type = svt_int;   
   if(!netgame)
   {
      t_return.value.i = 0; // single-player
   }
   else if(!deathmatch)
   {
      t_return.value.i = 1; // cooperative
   }
   else
      t_return.value.i = 2; // deathmatch
}

/*
   SF_PlayerSkin()
   
   Get the name of the skin the player is currently using.
*/
void SF_PlayerSkin(void)
{
  int plnum;
  char skinname[256];
  
  if(!t_argc)
  {
      player_t *pl;
      pl = current_script->trigger->player;
      if(pl) 
	 plnum = pl - players;
      else
      {
	 script_error("script not started by player\n");
	 return;
      }
  }
  else
    plnum = intvalue(t_argv[0]);

  if(plnum < 0 || plnum >= MAXPLAYERS)
  {
     script_error("player number out of range: %i", plnum);
     return;
  }

  if(players[plnum].skin)
     strncpy(skinname, players[plnum].skin->skinname, 255);
  else
     strcpy(skinname, "marine");
  
  t_return.type = svt_string;
  t_return.value.s = Z_Strdup(skinname, PU_LEVEL, 0);
}

/*
   SF_IsPlayerObj()
     
       A function suggested by SoM to help the script coder prevent
       exceptions related to calling player functions on non-player
       objects.
*/
void SF_IsPlayerObj(void)
{
   mobj_t *mo;

   if(!t_argc)
   {
      mo = current_script->trigger;
   }
   else
      mo = MobjForSvalue(t_argv[0]);

   t_return.type = svt_int;
   t_return.value.i = (mo && mo->player) ? 1 : 0;
}

void SF_PlayerKeys(void)
{
   int  playernum, keynum, givetake;

   if(t_argc < 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }
   
   playernum = intvalue(t_argv[0]);
   if(playernum < 0 || playernum > 3)
   {
      script_error("player number out of range: %i\n", playernum);
      return;
   }
   if(!playeringame[playernum]) // no error, just return -1
   {
      t_return.type = svt_int;
      t_return.value.i = -1;
      return;
   }
   
   keynum = intvalue(t_argv[1]);
   if(keynum < 0 || keynum >= NUMCARDS)
   {
      script_error("key number out of range: %i\n", keynum);
      return;
   }

   if(t_argc == 2)
   {
      t_return.type = svt_int;
      t_return.value.i = players[playernum].cards[keynum];
      return;
   }
   else
   {
      givetake = intvalue(t_argv[2]);
      if(givetake)
	 players[playernum].cards[keynum] = true;
      else
	 players[playernum].cards[keynum] = false;

      t_return.type = svt_int;
      t_return.value.i = 0;
   }
}

void SF_PlayerAmmo(void)
{
   int playernum, ammonum, amount;

   if(t_argc < 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   playernum = intvalue(t_argv[0]);
   if(playernum < 0 || playernum >= MAXPLAYERS)
   {
      script_error("player number out of range: %i", playernum);
      return;
   }
   if(!playeringame[playernum])
   {
      t_return.type = svt_int;
      t_return.value.i = -1;
      return;
   }

   ammonum = intvalue(t_argv[1]);
   if(ammonum < 0 || ammonum >= NUMAMMO)
   {
      script_error("ammo number out of range: %i", ammonum);
      return;
   }

   if(t_argc == 2)
   {
      t_return.type = svt_int;
      t_return.value.i = players[playernum].ammo[ammonum];
      return;
   }
   else if(t_argc >= 3)
   {
      amount = intvalue(t_argv[2]);
      
      if(amount > players[playernum].maxammo[ammonum])
	 amount = players[playernum].maxammo[ammonum];
      if(amount < 0)
	 amount = 0;

      t_return.type = svt_int;
      t_return.value.i = players[playernum].ammo[ammonum] = amount;
   }
}

// removed SF_PlayerMaxAmmo

// haleyjd: significantly different from the Legacy function
// spawnexplosion(damage, spot, [source])

void SF_SpawnExplosion(void)
{
   int damage;
   mobj_t *spot, *source;
   
   if(t_argc < 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }
   
   damage = intvalue(t_argv[0]);
   if(damage < 0)
      damage = 0;

   // spot is the thing blowing up, required
   spot = MobjForSvalue(t_argv[1]);
   
   // source is the thing that takes credit for damage, optional
   if(t_argc > 2)
      source = MobjForSvalue(t_argv[2]);
   else
      source = current_script->trigger;

   if(!spot || !source) // no null pointer action allowed
      return;

   P_RadiusAttack(spot, source, damage);
}

//
// movecamera(targetobj, targetheight, movespeed, targetangle, anglespeed)
//
// haleyjd: I've had to make misc. modifications to this function,
// for one, the first camera parameter is unnecessary because FS can
// only maintain one active script_camera, and it must be valid (set
// via setcamera()) in order to use this -- largely by SoM
//

void SF_MoveCamera(void)
{
  fixed_t    x, y, z;  
  fixed_t    xdist, ydist, zdist, xydist, movespeed;
  fixed_t    xstep, ystep, zstep, targetheight;
  angle_t    anglespeed, anglestep, angledist, targetangle, 
             mobjangle, bigangle, smallangle;
  
  // I have to use floats for the math where angles are divided 
  // by fixed values.  
  double     fangledist, fanglestep, fmovestep;
  int	     angledir;  
  mobj_t*    target;
  int        moved;
  int        quad1, quad2;

  angledir = moved = 0;

  if(camera != &script_camera)
  {
     script_error("attempt to move camera before setcamera\n");
     return;
  }
  
  if(t_argc < 5)
  {
     script_error("movecamera: insufficient arguments to function\n");
     return;
  }

  target = MobjForSvalue(t_argv[0]);
  if(!target) { script_error("invalid target for camera\n"); return; }

  targetheight = fixedvalue(t_argv[1]);
  movespeed    = fixedvalue(t_argv[2]);
  targetangle  = FixedToAngle(fixedvalue(t_argv[3]));
  anglespeed   = FixedToAngle(fixedvalue(t_argv[4]));

  // figure out how big one step will be
  xdist = target->x - script_camera.x;
  ydist = target->y - script_camera.y;
  zdist = targetheight - script_camera.z;
  
  // Angle checking...  
  //    90  
  //   Q1|Q0  
  //180--+--0  
  //   Q2|Q3  
  //    270
  quad1 = targetangle / ANG90;
  quad2 = script_camera.angle / ANG90;
  bigangle = targetangle > script_camera.angle ? targetangle : script_camera.angle;
  smallangle = targetangle < script_camera.angle ? targetangle : script_camera.angle;
  if((quad1 > quad2 && quad1 - 1 == quad2) || (quad2 > quad1 && quad2 - 1 == quad1) ||
     quad1 == quad2)
  {
     angledist = bigangle - smallangle;
     angledir = targetangle > script_camera.angle ? 1 : -1;
  }
  else
  {
     angle_t diff180 = (bigangle + ANG180) - (smallangle + ANG180);
     
     if(quad2 == 3 && quad1 == 0)
     {
	angledist = diff180;
	angledir = 1;
     }
     else if(quad1 == 3 && quad2 == 0)
     {
	angledist = diff180;
	angledir = -1;
     }
     else
     {
	angledist = bigangle - smallangle;
	if(angledist > ANG180)
	{
	   angledist = diff180;
	   angledir = targetangle > script_camera.angle ? -1 : 1;
	}
	else
	   angledir = targetangle > script_camera.angle ? 1 : -1;
     }
  }
  
  // set step variables based on distance and speed
  mobjangle = R_PointToAngle2(script_camera.x, script_camera.y, target->x, target->y);
  xydist = R_PointToDist2(script_camera.x, script_camera.y, target->x, target->y);

  xstep = FixedMul(finecosine[mobjangle >> ANGLETOFINESHIFT], movespeed);
  ystep = FixedMul(finesine[mobjangle >> ANGLETOFINESHIFT], movespeed);

  if(xydist && movespeed)
     zstep = FixedDiv(zdist, FixedDiv(xydist, movespeed));
  else
     zstep = zdist > 0 ? movespeed : -movespeed;

  if(xydist && movespeed && !anglespeed)
  {
     fangledist = ((double)angledist / ANGLE_1);
     fmovestep = ((double)FixedDiv(xydist, movespeed) / FRACUNIT);
     if(fmovestep)
	fanglestep = fangledist / fmovestep;
     else
	fanglestep = 360;

     anglestep = fanglestep * ANGLE_1;
  }
  else
     anglestep = anglespeed;

  if(abs(xstep) >= (abs(xdist) - 1))
     x = target->x;
  else
  {
     x = script_camera.x + xstep;
     moved = 1;
  }

  if(abs(ystep) >= (abs(ydist) - 1))
     y = target->y;
  else
  {
     y = script_camera.y + ystep;
     moved = 1;
  }

  if(abs(zstep) >= (abs(zdist) - 1))
     z = targetheight;
  else
  {
     z = script_camera.z + zstep;
     moved = 1;
  }

  if(anglestep >= angledist)
     script_camera.angle = targetangle;
  else
  {
     if(angledir == 1)
     {
	script_camera.angle += anglestep;
	moved = 1;
     }
     else if(angledir == -1)
     {
	script_camera.angle -= anglestep;
	moved = 1;
     }
  }

  script_camera.x = x;
  script_camera.y = y;
  script_camera.z = z;

  script_camera.heightsec = R_PointInSubsector(x, y)->sector->heightsec;

  t_return.type = svt_int;
  t_return.value.i = moved;
}

//
// FraggleScript Heads-Up Pics -- By SoM, modified by Quasar
//

//
// SF_CreateFSPic
//
// Implements: int createpic(string lumpname, int x, int y,
//                           int draw, int trans, fixed priority)
//
void SF_CreateFSPic(void)
{
   int handle, lumpnum;

   if(t_argc != 6)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   lumpnum = W_CheckNumForName(stringvalue(t_argv[0]));
   
   handle = HU_CreateFSPic(lumpnum, intvalue(t_argv[1]),
                           intvalue(t_argv[2]), intvalue(t_argv[3]), 
			   intvalue(t_argv[4]), fixedvalue(t_argv[5]));

   t_return.type = svt_int;
   t_return.value.i = handle;
}

//
// SF_ModifyFSPic
//
// Implements: void modifypic(int handle, string lumpname, 
//                            [int x,] [int y])
//
void SF_ModifyFSPic(void)
{
   int handle, lumpnum, x, y;
   
   if(t_argc < 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   handle  = intvalue(t_argv[0]);
   lumpnum = W_CheckNumForName(stringvalue(t_argv[1]));

   if(t_argc >= 3)
      x = intvalue(t_argv[2]);
   else
      x = -1;

   if(t_argc >= 4)
      y = intvalue(t_argv[3]);
   else
      y = -1;

   HU_ModifyFSPic(handle, lumpnum, x, y);
}

//
// SF_FSPicSetVisible
//
// Implements: void setpicvisible(int handle, int visible)
//
void SF_FSPicSetVisible(void)
{
   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   HU_ToggleFSPicVisible(intvalue(t_argv[0]), intvalue(t_argv[1]));
}

//
// SF_FSPicSetTrans
//
// Implements: void setpictrans(int handle, int trans)
//
void SF_FSPicSetTrans(void)
{
   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   HU_ToggleFSPicTL(intvalue(t_argv[0]), intvalue(t_argv[1]));
}

//
// SF_GetFSPicHandle
//
// Implements: int getpichandle(string lumpname, int x, int y)
//
// Note: this function doesn't make guarantees about which fs pic
//       you'll get, if you've made two identical ones, it always
//       chooses the one with lower priority or the one that was
//       made latest if they're equal in priority. This is just in
//       case you lose a handle somehow (???)
//
void SF_GetFSPicHandle(void)
{
   int lumpnum;
   
   if(t_argc != 3)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   lumpnum = W_CheckNumForName(stringvalue(t_argv[0]));

   t_return.type = svt_int;
   t_return.value.i = HU_GetFSPicHandle(lumpnum,
                                        intvalue(t_argv[1]),
					intvalue(t_argv[2]));
}

//
// SF_GetFSPicAttribute
//
// Implements: int getpicattr(int handle, string selector)
//
void SF_GetFSPicAttribute(void)
{
   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   t_return.type = svt_int;
   t_return.value.i =
     HU_GetFSPicAttribute(intvalue(t_argv[0]), stringvalue(t_argv[1]));
}

//
// SF_GetFSPicLowestPriority
//
// Implements: fixed getpiclp(void)
//
void SF_GetFSPicLowestPriority(void)
{
   t_return.type = svt_fixed;
   t_return.value.f = HU_GetLowestPriority();
}

//
// SF_GetFSPicHighestPriority
//
// Implements: fixed getpichp(void)
//
void SF_GetFSPicHighestPriority(void)
{
   t_return.type = svt_fixed;
   t_return.value.f = HU_GetHighestPriority();
}

//
// SF_GetFSPicPriority
//
// Implements: fixed getpicpriority(int handle)
//
void SF_GetFSPicPriority(void)
{
   if(!t_argc)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   t_return.type = svt_fixed;
   t_return.value.f = HU_GetFSPicPriority(intvalue(t_argv[0]));
}

//
// SF_SetFSPicPriority
//
// Implements: void setpicpriority(int handle, fixed priority)
//
void SF_SetFSPicPriority(void)
{
   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   HU_SetFSPicPriority(intvalue(t_argv[0]), fixedvalue(t_argv[1]));
}

// thing dormancy
// this function is needed to awaken dormant things since just
// setting the flag is not sufficient

//
// SF_ObjAwaken
//
// Implements: void objawaken([mobj mo])
//
void SF_ObjAwaken(void)
{
   mobj_t *mo;

   if(!t_argc)
      mo = current_script->trigger;
   else
      mo = MobjForSvalue(t_argv[0]);

   if(mo)
   {
      mo->flags2 &= ~MF2_DORMANT;
      mo->tics = 1;
   }
}

//
// SF_AmbientSound
//
// Implements: void ambientsound(string name)
//
void SF_AmbientSound(void)
{
   if(!t_argc)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   S_StartSoundName(NULL, stringvalue(t_argv[0]));
}

//
// SF_TimedTip
//
// Implements: void timedtip(int clocks, ...)
//
void SF_TimedTip(void)
{
   int i, clocks;
   char tempstr[256] = "";
   
   if(t_argc < 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }
   
   clocks = intvalue(t_argv[0]);

   for(i=1; i<t_argc; i++)
    sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

   HU_CentreMsgTimed(tempstr, clocks);
}

//
// SF_PlayerTimedTip
//
// Implements: void playertimedtip(int playernum, int clocks, ...)
//
void SF_PlayerTimedTip(void)
{
   int i, clocks, plnum;
   char tempstr[256] = "";
   
   if(t_argc < 3)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   plnum = intvalue(t_argv[0]);
   if(plnum < 0 || plnum >= MAXPLAYERS)
   {
      script_error("player number out of range: %i", plnum);
      return;
   }

   clocks = intvalue(t_argv[1]);

   for(i=2; i<t_argc; i++)
    sprintf(tempstr, "%s%s", tempstr, stringvalue(t_argv[i]));

   if(plnum == consoleplayer)
      HU_CentreMsgTimed(tempstr, clocks);
}

// array functions in t_array.c
void SF_NewArray(void);          // impls: array newArray(...)
void SF_ArrayCopyInto(void);     // impls: void copyInto(array, array)
void SF_ArrayElementAt(void);    // impls: 'a elementAt(array, int)
void SF_ArraySetElementAt(void); // impls: void setElementAt(array, int, 'a)
void SF_ArrayLength(void);       // impls: int length(array)

// 
// SF_ExitSecret
//
// Implements: void exitsecret()
//
// This enables a script to cause a secret exit action. If
// the level info variable nextsecret is set, it will be used,
// otherwise default behavior occurs (meaning the same level will
// be reloaded unless the function is called on a map normally
// enabled for secrets). scriptSecret is used to force this 
// functionality even in the German edition, but it can cause 
// bomb-outs if used inappropriately (decl'd in g_game.c).
//
void SF_ExitSecret(void)
{
   scriptSecret = true;
   G_SecretExitLevel();
}

//
// SF_CinemaPause
//
// Implements: void cinemapause()
//
void SF_CinemaPause(void)
{
   cinema_pause = !cinema_pause;
}

//
// SF_StartDialog
//
// Implements: void startdialogue(int plnum, string lump, string name,
//                                string picname)
//
void SF_StartDialogue(void)
{
   int plnum;

   if(t_argc != 4)
   {
      script_error("incorrect arguments to function\n");
      return;
   }
   
   plnum = intvalue(t_argv[0]);
   if(plnum < 0 || plnum >= MAXPLAYERS)
   {
      script_error("player number out of range: %i", plnum);
      return;
   }

   DLG_Init(); // initialize system (may already be initialized)
   DLG_Start(plnum, stringvalue(t_argv[1]), stringvalue(t_argv[2]), 
             stringvalue(t_argv[3]));
}

// Type forcing functions -- useful with arrays et al

void SF_MobjValue(void)
{
   if(t_argc != 1)
   {
      script_error("incorrect arguments to function\n");
      return;
   }
   t_return.type = svt_mobj;
   t_return.value.mobj = MobjForSvalue(t_argv[0]);
}

void SF_StringValue(void)
{  
   if(t_argc != 1)
   {
      script_error("incorrect arguments to function\n");
      return;
   }
   t_return.type = svt_string;
   t_return.value.s = Z_Strdup(stringvalue(t_argv[0]), PU_LEVEL, 0);
}

void SF_IntValue(void)
{
   if(t_argc != 1)
   {
      script_error("incorrect arguments to function\n");
      return;
   }
   t_return.type = svt_int;
   t_return.value.i = intvalue(t_argv[0]);
}

void SF_FixedValue(void)
{
   if(t_argc != 1)
   {
      script_error("incorrect arguments to function\n");
      return;
   }
   t_return.type = svt_fixed;
   t_return.value.f = fixedvalue(t_argv[0]);
}

void SF_InDialogue(void)
{
   t_return.type = svt_int;
   t_return.value.i = !!currentdialog;
}

//////////////////////////////////////////////////////////////////////////
//
// Init Functions
//

void init_functions(void)
{
  // add all the functions
  add_game_int("consoleplayer", &consoleplayer);
  add_game_int("displayplayer", &displayplayer);
  add_game_int("zoom", &zoom);
  add_game_int("fov", &zoom); // haleyjd: temporary alias (?)
  add_game_mobj("trigger", &trigger_obj);
  add_game_array("cameranodes", &sfcnodes); // haleyjd: camera nodes
  
  // important C-emulating stuff
  new_function("break", SF_Break);
  new_function("continue", SF_Continue);
  new_function("return", SF_Return);
  new_function("goto", SF_Goto);
  new_function("include", SF_Include);
  
  // standard FraggleScript functions
  new_function("print", SF_Print);
  new_function("rnd", SF_Rnd);
  new_function("input", SF_Input);
  new_function("beep", SF_Beep);
  new_function("clock", SF_Clock);
  new_function("wait", SF_Wait);
  new_function("tagwait", SF_TagWait);
  new_function("scriptwait", SF_ScriptWait);
  new_function("startscript", SF_StartScript);
  new_function("scriptrunning", SF_ScriptRunning);
  
  // doom stuff
  new_function("startskill", SF_StartSkill);
  new_function("exitlevel", SF_ExitLevel);
  new_function("tip", SF_Tip);
  new_function("message", SF_Message);
  
  // player stuff
  new_function("playermsg", SF_PlayerMsg);
  new_function("playertip", SF_PlayerTip);
  new_function("playeringame", SF_PlayerInGame);
  new_function("playername", SF_PlayerName);
  new_function("playerobj", SF_PlayerObj);

  // mobj stuff
  new_function("spawn", SF_Spawn);
  new_function("kill", SF_KillObj);
  new_function("removeobj", SF_RemoveObj);
  new_function("objx", SF_ObjX);
  new_function("objy", SF_ObjY);
  new_function("objz", SF_ObjZ);
  new_function("teleport", SF_Teleport);
  new_function("silentteleport", SF_SilentTeleport);
  new_function("damageobj", SF_DamageObj);
  new_function("player", SF_Player);
  new_function("objsector", SF_ObjSector);
  new_function("objflag", SF_ObjFlag);
  new_function("pushobj", SF_PushThing);
  new_function("objangle", SF_ObjAngle);
  
  // haleyjd: left out
  new_function("objhealth", SF_ObjHealth);

  // sector stuff
  new_function("floorheight", SF_FloorHeight);
  new_function("floortext", SF_FloorTexture);
  new_function("floortexture", SF_FloorTexture);   // haleyjd: alias
  new_function("movefloor", SF_MoveFloor);
  new_function("ceilheight", SF_CeilingHeight);
  new_function("ceilingheight", SF_CeilingHeight); // haleyjd: alias
  new_function("moveceil", SF_MoveCeiling);
  new_function("moveceiling", SF_MoveCeiling);     // haleyjd: aliases
  new_function("ceilingtexture", SF_CeilingTexture);
  new_function("ceiltext", SF_CeilingTexture);  // haleyjd: wrong
  new_function("lightlevel", SF_LightLevel);    // handler - was
  new_function("fadelight", SF_FadeLight);      // SF_FloorTexture!
  new_function("colormap", SF_SectorColormap);
	
  // cameras!
  new_function("setcamera", SF_SetCamera);
  new_function("clearcamera", SF_ClearCamera);
  
  // trig functions
  new_function("pointtoangle", SF_PointToAngle);
  new_function("pointtodist", SF_PointToDist);

  // sound functions
  new_function("startsound", SF_StartSound);
  new_function("startsectorsound", SF_StartSectorSound);
  new_function("changemusic", SF_ChangeMusic);
  
  // hubs!
  new_function("changehublevel", SF_ChangeHubLevel);

  // doors
  new_function("opendoor", SF_OpenDoor);
  new_function("closedoor", SF_CloseDoor);

  new_function("runcommand", SF_RunCommand);
  new_function("linetrigger", SF_LineTrigger);

  // Eternity extensions
  new_function("objflag2", SF_ObjFlag2);
  new_function("spawnshot", SF_SpawnShot);
  new_function("checklife", SF_CheckLife);
  new_function("setlineblocking", SF_SetLineBlocking);
  new_function("setlinetexture", SF_SetLineTexture);
  new_function("setfriction", SF_SetFriction);
  new_function("getfriction", SF_GetFriction);

  // SoM's new miscellaneous functions
  new_function("reactiontime", SF_ReactionTime);
  new_function("objtarget", SF_MobjTarget);
  new_function("objmomx", SF_MobjMomx);
  new_function("objmomy", SF_MobjMomy);
  new_function("objmomz", SF_MobjMomz);
  new_function("max", SF_Max);
  new_function("min", SF_Min);
  new_function("abs", SF_Abs);

  // more Eternity functions
  new_function("gameskill", SF_Gameskill);
  new_function("gamemode", SF_Gamemode);
  new_function("playerskin", SF_PlayerSkin);
  new_function("isplayerobj", SF_IsPlayerObj);
  new_function("playerkeys", SF_PlayerKeys);
  new_function("playerammo", SF_PlayerAmmo);
  new_function("spawnexplosion", SF_SpawnExplosion);
  new_function("movecamera", SF_MoveCamera);

  // fs hu pics
  new_function("createpic", SF_CreateFSPic);
  new_function("modifypic", SF_ModifyFSPic);
  new_function("setpicvisible", SF_FSPicSetVisible);
  new_function("setpictrans", SF_FSPicSetTrans);
  new_function("getpichandle", SF_GetFSPicHandle);
  new_function("getpicattr", SF_GetFSPicAttribute);
  new_function("getpiclp", SF_GetFSPicLowestPriority);
  new_function("getpichp", SF_GetFSPicHighestPriority);
  new_function("getpicpriority", SF_GetFSPicPriority);
  new_function("setpicpriority", SF_SetFSPicPriority);

  // more misc functions
  new_function("objawaken", SF_ObjAwaken);
  new_function("ambientsound", SF_AmbientSound);
  new_function("timedtip", SF_TimedTip);
  new_function("playertimedtip", SF_PlayerTimedTip);
  new_function("setlinemnblock", SF_SetLineMonsterBlocking);
  new_function("scriptwaitpre", SF_ScriptWaitPre);
  new_function("exitsecret", SF_ExitSecret);
  new_function("startdialogue", SF_StartDialogue);
  new_function("cinemapause", SF_CinemaPause);
  new_function("indialogue", SF_InDialogue);
  
  // forced coercion functions
  new_function("mobjvalue", SF_MobjValue);
  new_function("stringvalue", SF_StringValue);
  new_function("intvalue", SF_IntValue);
  new_function("fixedvalue", SF_FixedValue);
  
  // arrays -- in t_array.c
  new_function("newArray", SF_NewArray);
  new_function("copyInto", SF_ArrayCopyInto);
  new_function("elementAt", SF_ArrayElementAt);
  new_function("setElementAt", SF_ArraySetElementAt);
  new_function("length", SF_ArrayLength);
}

// EOF
