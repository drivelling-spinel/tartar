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
//      Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_saveg.c,v 1.17 1998/05/03 23:10:22 killough Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_saveg.h"
#include "m_random.h"
#include "am_map.h"
#include "p_enemy.h"
#include "p_hubs.h"
#include "p_skin.h"
#include "t_vari.h"
#include "t_script.h"
#include "t_array.h" // haleyjd
#include "hu_fspic.h"
#include "ex_stuff.h"

byte *save_p;

// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
// #define PADSAVEP()    do { save_p += (4 - ((int) save_p & 3)) & 3; } while (0)

// sf: uncomment above for sgi and gecko if you want then
// this makes for smaller savegames
#define PADSAVEP()      {}

int num_thinkers;       // number of thinkers in level being archived

// sf: globalised for script mobj pointers

mobj_t **mobj_p;    // killough 2/14/98: Translation table

        // sf: seperate function
void P_FreeObjTable()
{
  free(mobj_p);    // free translation table
}
// sf: made these into seperate functions
//     for FraggleScript saving object ptrs too

void P_NumberObjects()
{
  thinker_t *th;

  num_thinkers = 0; //init to 0

  // killough 2/14/98:
  // count the number of thinkers, and mark each one with its index, using
  // the prev field as a placeholder, since it can be restored later.

  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    if (th->function == P_MobjThinker)
      th->prev = (thinker_t *) ++num_thinkers;
}

void P_DeNumberObjects()
{
  thinker_t *prev = &thinkercap;
  thinker_t *th;
  
  for (th = thinkercap.next ; th != &thinkercap ; prev=th, th=th->next)
    th->prev = prev;
}

        // get the mobj number from the mobj
int P_MobjNum(mobj_t *mo)
{
  long l = mo ? (long)mo->thinker.prev : -1;   // -1 = NULL
 
  // extra check for invalid thingnum (prob. still ptr)
  if(l<0 || l>num_thinkers) l = -1;
  return l;
}

mobj_t *P_MobjForNum(int n)
{
  return (n == -1) ? (NULL) : (mobj_p[n]);
}

//
// P_ArchivePlayers
//
void P_ArchivePlayers (void)
{
  int i;

  CheckSaveGame(sizeof(player_t) * MAXPLAYERS); // killough

  for (i=0 ; i<MAXPLAYERS ; i++)
    if (playeringame[i])
      {
        int      j;
        state_t * local_states = EXTRA_PLAYER_STATES(&players[i]);
        player_t *dest;

        PADSAVEP();
        dest = (player_t *) save_p;
        memcpy(dest, &players[i], sizeof(player_t));
        save_p += sizeof(player_t);
        for (j=0; j<NUMPSPRITES; j++)
          if (dest->psprites[j].state)
            dest->psprites[j].state =
              (state_t *)(dest->psprites[j].state-local_states);
      }
}

//
// P_UnArchivePlayers
//
void P_UnArchivePlayers (void)
{
  int i;

  for (i=0 ; i<MAXPLAYERS ; i++)
    if (playeringame[i])
      {
        int j;
        state_t * local_states;

        PADSAVEP();

	      // sf: when loading a hub level using save games,
	      //     do not change the player data when crossing
	      //     levels: ie. retain the same weapons etc.
  
    	  if(!hub_changelevel)
	        {
	          memcpy(&players[i], save_p, sizeof(player_t));
	          local_states = EXTRA_PLAYER_STATES(&players[i]);
    	      for (j=0 ; j<NUMPSPRITES ; j++)
	            if (players[i].psprites[j].state)
	              {
		              players[i].psprites[j].state = &local_states[ (int)players[i].psprites[j].state ];
		            } 
	        }
	
        save_p += sizeof(player_t);

        // will be set when unarc thinker
        players[i].mo = NULL;
        players[i].attacker = NULL;
    	  players[i].skin = &marine;  // reset skin
    	  players[i].attackdown = players[i].usedown = false;  // sf
    	  players[i].cmd.buttons = 0;    // sf
     }
}


//
// P_ArchiveWorld
//
void P_ArchiveWorld (void)
{
  int            i;
  const sector_t *sec;
  const line_t   *li;
  const side_t   *si;
  short          *put;

  // killough 3/22/98: fix bug caused by hoisting save_p too early
  // killough 10/98: adjust size for changes below
  
  // haleyjd  09/00: we need to save friction & movefactor now too,
  //  since Halif can freeze water, and a setfriction function for
  //  FraggleScript would be in line with this addition
  
  size_t size = 
    (sizeof(short)*5 + sizeof sec->floorheight + sizeof sec->ceilingheight
     + sizeof sec->friction + sizeof sec->movefactor) // haleyjd
    * numsectors + sizeof(short)*3*numlines + 4;

  for (i=0; i<numlines; i++)
    {
      if ((unsigned short)(lines[i].sidenum[0]) != 0xffff)
        size +=
	  sizeof(short)*3 + sizeof si->textureoffset + sizeof si->rowoffset;
      if ((unsigned short)(lines[i].sidenum[1]) != 0xffff)
	size +=
	  sizeof(short)*3 + sizeof si->textureoffset + sizeof si->rowoffset;
    }

  CheckSaveGame(size); // killough

  PADSAVEP();                // killough 3/22/98

  put = (short *)save_p;

  // do sectors
  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
    {
      // killough 10/98: save full floor & ceiling heights, including fraction
      memcpy(put, &sec->floorheight, sizeof sec->floorheight);
      put = (void *)((char *) put + sizeof sec->floorheight);
      memcpy(put, &sec->ceilingheight, sizeof sec->ceilingheight);
      put = (void *)((char *) put + sizeof sec->ceilingheight);

      // haleyjd: save the friction information too
      memcpy(put, &sec->friction, sizeof sec->friction);
      put = (void *)((char *) put + sizeof sec->friction);
      memcpy(put, &sec->movefactor, sizeof sec->movefactor);
      put = (void *)((char *) put + sizeof sec->movefactor);

      *put++ = sec->floorpic;
      *put++ = sec->ceilingpic;
      *put++ = sec->lightlevel;
      *put++ = sec->special;            // needed?   yes -- transfer types
      *put++ = sec->tag;                // needed?   need them -- killough
    }

  // do lines
  for (i=0, li = lines ; i<numlines ; i++,li++)
    {
      int j;

      *put++ = li->flags;
      *put++ = li->special;
      *put++ = li->tag;

      for (j=0; j<2; j++)
        if ((unsigned short)(li->sidenum[j]) != 0xffff)
          {
            si = &sides[(unsigned short)(li->sidenum[j])];

	    // killough 10/98: save full sidedef offsets,
	    // preserving fractional scroll offsets

	    memcpy(put, &si->textureoffset, sizeof si->textureoffset);
	    put = (void *)((char *) put + sizeof si->textureoffset);
	    memcpy(put, &si->rowoffset, sizeof si->rowoffset);
	    put = (void *)((char *) put + sizeof si->rowoffset);

            *put++ = si->toptexture;
            *put++ = si->bottomtexture;
            *put++ = si->midtexture;
          }
    }
  save_p = (byte *) put;
}



//
// P_UnArchiveWorld
//
void P_UnArchiveWorld (void)
{
  int          i;
  sector_t     *sec;
  line_t       *li;
  const short  *get;

  PADSAVEP();                // killough 3/22/98

  get = (short *) save_p;

  // do sectors
  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
    {
      // killough 10/98: load full floor & ceiling heights, including fractions

      memcpy(&sec->floorheight, get, sizeof sec->floorheight);
      get = (void *)((char *) get + sizeof sec->floorheight);
      memcpy(&sec->ceilingheight, get, sizeof sec->ceilingheight);
      get = (void *)((char *) get + sizeof sec->ceilingheight);

      // haleyjd: retrieve the friction information we now save
      memcpy(&sec->friction, get, sizeof sec->friction);
      get = (void *)((char *) get + sizeof sec->friction);
      memcpy(&sec->movefactor, get, sizeof sec->movefactor);
      get = (void *)((char *) get + sizeof sec->movefactor);

      sec->floorpic = *get++;
      sec->ceilingpic = *get++;
      sec->lightlevel = *get++;
      sec->special = *get++;
      sec->tag = *get++;
      sec->ceilingdata = 0; //jff 2/22/98 now three thinker fields, not two
      sec->floordata = 0;
      sec->lightingdata = 0;
      sec->soundtarget = 0;
    }

  // do lines
  for (i=0, li = lines ; i<numlines ; i++,li++)
    {
      int j;

      li->flags = *get++;
      li->special = *get++;
      li->tag = *get++;
      for (j=0 ; j<2 ; j++)
        if ((unsigned short)(li->sidenum[j]) != 0xffff)
          {
            side_t *si = &sides[(unsigned short)(li->sidenum[j])];

	    // killough 10/98: load full sidedef offsets, including fractions

	    memcpy(&si->textureoffset, get, sizeof si->textureoffset);
	    get = (void *)((char *) get + sizeof si->textureoffset);
	    memcpy(&si->rowoffset, get, sizeof si->rowoffset);
	    get = (void *)((char *) get + sizeof si->rowoffset);

            si->toptexture = *get++;
            si->bottomtexture = *get++;
            si->midtexture = *get++;
          }
    }
  save_p = (byte *) get;
}

//
// Thinkers
//

typedef enum {
  tc_end,
  tc_mobj
} thinkerclass_t;

//
// P_ArchiveThinkers
//
// 2/14/98 killough: substantially modified to fix savegame bugs

void P_ArchiveThinkers (void)
{
  thinker_t *th;

  CheckSaveGame(sizeof brain);      // killough 3/26/98: Save boss brain state
  memcpy(save_p, &brain, sizeof brain);
  save_p += sizeof brain;

  // check that enough room is available in savegame buffer
  CheckSaveGame(num_thinkers*(sizeof(mobj_t)+4));       // killough 2/14/98

  // save off the current thinkers
  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    if (th->function == P_MobjThinker)
      {
        mobj_t *mobj;

        *save_p++ = tc_mobj;
        PADSAVEP();
        mobj = (mobj_t *)save_p;
        memcpy (mobj, th, sizeof(*mobj));
        save_p += sizeof(*mobj);
        mobj->state = (state_t *)(mobj->state - states);

        // killough 2/14/98: convert pointers into indices.
        // Fixes many savegame problems, by properly saving
        // target and tracer fields. Note: we store NULL if
        // the thinker pointed to by these fields is not a
        // mobj thinker.

        if (mobj->target)
          mobj->target = mobj->target->thinker.function ==
            P_MobjThinker ?
            (mobj_t *) mobj->target->thinker.prev : NULL;

        if (mobj->tracer)
          mobj->tracer = mobj->tracer->thinker.function ==
            P_MobjThinker ?
            (mobj_t *) mobj->tracer->thinker.prev : NULL;

        // killough 2/14/98: new field: save last known enemy. Prevents
        // monsters from going to sleep after killing monsters and not
        // seeing player anymore.

        if (mobj->lastenemy)
          mobj->lastenemy = mobj->lastenemy->thinker.function ==
            P_MobjThinker ?
            (mobj_t *) mobj->lastenemy->thinker.prev : NULL;

        // killough 2/14/98: end changes

        if (mobj->above_thing)                                      // phares
          mobj->above_thing = mobj->above_thing->thinker.function ==
            P_MobjThinker ?
            (mobj_t *) mobj->above_thing->thinker.prev : NULL;

        if (mobj->below_thing)
          mobj->below_thing = mobj->below_thing->thinker.function ==
            P_MobjThinker ?
            (mobj_t *) mobj->below_thing->thinker.prev : NULL;      // phares

        if (mobj->player)
          mobj->player = (player_t *)((mobj->player-players) + 1);
      }

  // add a terminating marker
  *save_p++ = tc_end;

  // killough 9/14/98: save soundtargets
  {
    int i;
    CheckSaveGame(numsectors * sizeof(mobj_t *));       // killough 9/14/98
    for (i = 0; i < numsectors; i++)
      {
	mobj_t *target = sectors[i].soundtarget;
	if (target)
	  target = (mobj_t *) target->thinker.prev;
	memcpy(save_p, &target, sizeof target);
	save_p += sizeof target;
      }
  }
  
  // killough 2/14/98: restore prev pointers
        // sf: still needed for saving script mobj pointers
  // killough 2/14/98: end changes
}

//
// killough 11/98
//
// Same as P_SetTarget() in p_tick.c, except that the target is nullified
// first, so that no old target's reference count is decreased (when loading
// savegames, old targets are indices, not really pointers to targets).
//

static void P_SetNewTarget(mobj_t **mop, mobj_t *targ)
{
  *mop = NULL;
  P_SetTarget(mop, targ);
}

void T_BuildGameArrays(void);

//
// P_UnArchiveThinkers
//
// 2/14/98 killough: substantially modified to fix savegame bugs
//

void P_UnArchiveThinkers (void)
{
  thinker_t *th;
  size_t    size;        // killough 2/14/98: size of or index into table

  // killough 3/26/98: Load boss brain state
  memcpy(&brain, save_p, sizeof brain);
  save_p += sizeof brain;

  // remove all the current thinkers
  for (th = thinkercap.next; th != &thinkercap; )
    {
      thinker_t *next = th->next;
      if (th->function == P_MobjThinker)
        P_RemoveMobj ((mobj_t *) th);
      else
        Z_Free (th);
      th = next;
    }
  P_InitThinkers ();

  // killough 2/14/98: count number of thinkers by skipping through them
  {
    byte *sp = save_p;     // save pointer and skip header
    for (size = 1; *save_p++ == tc_mobj; size++)  // killough 2/14/98
      {                     // skip all entries, adding up count
        PADSAVEP();
        save_p += sizeof(mobj_t);
      }

    if (*--save_p != tc_end)
      I_Error ("Unknown tclass %i in savegame", *save_p);

    // first table entry special: 0 maps to NULL
    *(mobj_p = malloc(size * sizeof *mobj_p)) = 0;   // table of pointers
    save_p = sp;           // restore save pointer
  }

  // read in saved thinkers
  for (size = 1; *save_p++ == tc_mobj; size++)    // killough 2/14/98
    {
      mobj_t *mobj = Z_Malloc(sizeof(mobj_t), PU_LEVEL, NULL);

      // killough 2/14/98 -- insert pointers to thinkers into table, in order:
      mobj_p[size] = mobj;

      PADSAVEP();
      memcpy (mobj, save_p, sizeof(mobj_t));
      save_p += sizeof(mobj_t);
      mobj->state = states + (int) mobj->state;

      if (mobj->player)
        (mobj->player = &players[(int) mobj->player - 1]) -> mo = mobj;

      P_SetThingPosition (mobj);

      // haleyjd: here we can handle the Cyberdemon Guard
      //          so that his axe never comes back
      if(mobj->type == MT_CYBERGUARD2 &&
         mobj->state != &states[S_CYBERGUARD2LOOK])
      {
	      mobj->info = &mobjinfo[MT_CYBORG];
      }
      else
        mobj->info = &mobjinfo[mobj->type];

      // killough 2/28/98:
      // Fix for falling down into a wall after savegame loaded:
      //      mobj->floorz = mobj->subsector->sector->floorheight;
      //      mobj->ceilingz = mobj->subsector->sector->ceilingheight;

      mobj->thinker.function = P_MobjThinker;
      P_AddThinker (&mobj->thinker);
    }

  // killough 2/14/98: adjust target and tracer fields, plus
  // lastenemy field, to correctly point to mobj thinkers.
  // NULL entries automatically handled by first table entry.
  //
  // killough 11/98: use P_SetNewTarget() to set fields

  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      P_SetNewTarget(&((mobj_t *) th)->target,
        mobj_p[(size_t)((mobj_t *)th)->target]);

      P_SetNewTarget(&((mobj_t *) th)->tracer,
        mobj_p[(size_t)((mobj_t *)th)->tracer]);

      P_SetNewTarget(&((mobj_t *) th)->lastenemy,
        mobj_p[(size_t)((mobj_t *)th)->lastenemy]);

      // phares: added two new fields for Sprite Height problem

      P_SetNewTarget(&((mobj_t *) th)->above_thing,
        mobj_p[(size_t)((mobj_t *)th)->above_thing]);

      P_SetNewTarget(&((mobj_t *) th)->below_thing,
        mobj_p[(size_t)((mobj_t *)th)->below_thing]);
    }

  {  // killough 9/14/98: restore soundtargets
    int i;
    for (i = 0; i < numsectors; i++)
      {
	mobj_t *target;
	memcpy(&target, save_p, sizeof target);
	save_p += sizeof target;
	P_SetNewTarget(&sectors[i].soundtarget, mobj_p[(size_t) target]);
      }
  }

  // killough 3/26/98: Spawn icon landings:
  if (gamemode == commercial)
    P_SpawnBrainTargets();

  // haleyjd: initialize miscellaneous spots (gamemode-independent)
  P_InitMiscMapSpots();
  T_BuildGameArrays();
}

//
// P_ArchiveSpecials
//
enum {
  tc_ceiling,
  tc_door,
  tc_floor,
  tc_plat,
  tc_flash,
  tc_strobe,
  tc_glow,
  tc_elevator,    //jff 2/22/98 new elevator type thinker
  tc_scroll,      // killough 3/7/98: new scroll effect thinker
  tc_pusher,      // phares 3/22/98:  new push/pull effect thinker
  tc_flicker,     // killough 10/4/98
  tc_endspecials
} specials_e;

//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
// T_MoveElevator, (plat_t: sector_t *), - active list      // jff 2/22/98
// T_Scroll                                                 // killough 3/7/98
// T_Pusher                                                 // phares 3/22/98
// T_FireFlicker                                            // killough 10/4/98
//

void P_ArchiveSpecials (void)
{
  thinker_t *th;
  size_t    size = 0;          // killough

  // save off the current thinkers (memory size calculation -- killough)

  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    if (!th->function)
      {
        platlist_t *pl;
        ceilinglist_t *cl;     //jff 2/22/98 need this for ceilings too now
        for (pl=activeplats; pl; pl=pl->next)
          if (pl->plat == (plat_t *) th)   // killough 2/14/98
            {
              size += 4+sizeof(plat_t);
              goto end;
            }
        for (cl=activeceilings; cl; cl=cl->next) // search for activeceiling
          if (cl->ceiling == (ceiling_t *) th)   //jff 2/22/98
            {
              size += 4+sizeof(ceiling_t);
              goto end;
            }
      end:;
      }
    else
      size +=
        th->function==T_MoveCeiling  ? 4+sizeof(ceiling_t) :
        th->function==T_VerticalDoor ? 4+sizeof(vldoor_t)  :
        th->function==T_MoveFloor    ? 4+sizeof(floormove_t):
        th->function==T_PlatRaise    ? 4+sizeof(plat_t)    :
        th->function==T_LightFlash   ? 4+sizeof(lightflash_t):
        th->function==T_StrobeFlash  ? 4+sizeof(strobe_t)  :
        th->function==T_Glow         ? 4+sizeof(glow_t)    :
        th->function==T_MoveElevator ? 4+sizeof(elevator_t):
        th->function==T_Scroll       ? 4+sizeof(scroll_t)  :
        th->function==T_Pusher       ? 4+sizeof(pusher_t)  :
        th->function==T_FireFlicker  ? 4+sizeof(fireflicker_t) :
      0;

  CheckSaveGame(size);          // killough

  // save off the current thinkers
  for (th=thinkercap.next; th!=&thinkercap; th=th->next)
    {
      if (!th->function)
        {
          platlist_t *pl;
          ceilinglist_t *cl;    //jff 2/22/98 add iter variable for ceilings

          // killough 2/8/98: fix plat original height bug.
          // Since acv==NULL, this could be a plat in stasis.
          // so check the active plats list, and save this
          // plat (jff: or ceiling) even if it is in stasis.

          for (pl=activeplats; pl; pl=pl->next)
            if (pl->plat == (plat_t *) th)      // killough 2/14/98
              goto plat;

          for (cl=activeceilings; cl; cl=cl->next)
            if (cl->ceiling == (ceiling_t *) th)      //jff 2/22/98
              goto ceiling;

          continue;
        }

      if (th->function == T_MoveCeiling)
        {
          ceiling_t *ceiling;
        ceiling:                               // killough 2/14/98
          *save_p++ = tc_ceiling;
          PADSAVEP();
          ceiling = (ceiling_t *)save_p;
          memcpy (ceiling, th, sizeof(*ceiling));
          save_p += sizeof(*ceiling);
          ceiling->sector = (sector_t *)(ceiling->sector - sectors);
          continue;
        }

      if (th->function == T_VerticalDoor)
        {
          vldoor_t *door;
          *save_p++ = tc_door;
          PADSAVEP();
          door = (vldoor_t *) save_p;
          memcpy (door, th, sizeof *door);
          save_p += sizeof(*door);
          door->sector = (sector_t *)(door->sector - sectors);
          //jff 1/31/98 archive line remembered by door as well
          door->line = (line_t *) (door->line ? door->line-lines : -1);
          continue;
        }

      if (th->function == T_MoveFloor)
        {
          floormove_t *floor;
          *save_p++ = tc_floor;
          PADSAVEP();
          floor = (floormove_t *)save_p;
          memcpy (floor, th, sizeof(*floor));
          save_p += sizeof(*floor);
          floor->sector = (sector_t *)(floor->sector - sectors);
          continue;
        }

      if (th->function == T_PlatRaise)
        {
          plat_t *plat;
        plat:   // killough 2/14/98: added fix for original plat height above
          *save_p++ = tc_plat;
          PADSAVEP();
          plat = (plat_t *)save_p;
          memcpy (plat, th, sizeof(*plat));
          save_p += sizeof(*plat);
          plat->sector = (sector_t *)(plat->sector - sectors);
          continue;
        }

      if (th->function == T_LightFlash)
        {
          lightflash_t *flash;
          *save_p++ = tc_flash;
          PADSAVEP();
          flash = (lightflash_t *)save_p;
          memcpy (flash, th, sizeof(*flash));
          save_p += sizeof(*flash);
          flash->sector = (sector_t *)(flash->sector - sectors);
          continue;
        }

      if (th->function == T_StrobeFlash)
        {
          strobe_t *strobe;
          *save_p++ = tc_strobe;
          PADSAVEP();
          strobe = (strobe_t *)save_p;
          memcpy (strobe, th, sizeof(*strobe));
          save_p += sizeof(*strobe);
          strobe->sector = (sector_t *)(strobe->sector - sectors);
          continue;
        }

      if (th->function == T_Glow)
        {
          glow_t *glow;
          *save_p++ = tc_glow;
          PADSAVEP();
          glow = (glow_t *)save_p;
          memcpy (glow, th, sizeof(*glow));
          save_p += sizeof(*glow);
          glow->sector = (sector_t *)(glow->sector - sectors);
          continue;
        }

      // killough 10/4/98: save flickers
      if (th->function == T_FireFlicker)
        {
          fireflicker_t *flicker;
          *save_p++ = tc_flicker;
          PADSAVEP();
          flicker = (fireflicker_t *)save_p;
          memcpy (flicker, th, sizeof(*flicker));
          save_p += sizeof(*flicker);
          flicker->sector = (sector_t *)(flicker->sector - sectors);
          continue;
        }

      //jff 2/22/98 new case for elevators
      if (th->function == T_MoveElevator)
        {
          elevator_t *elevator;         //jff 2/22/98
          *save_p++ = tc_elevator;
          PADSAVEP();
          elevator = (elevator_t *)save_p;
          memcpy (elevator, th, sizeof(*elevator));
          save_p += sizeof(*elevator);
          elevator->sector = (sector_t *)(elevator->sector - sectors);
          continue;
        }

      // killough 3/7/98: Scroll effect thinkers
      if (th->function == T_Scroll)
        {
          *save_p++ = tc_scroll;
          memcpy (save_p, th, sizeof(scroll_t));
          save_p += sizeof(scroll_t);
          continue;
        }

      // phares 3/22/98: Push/Pull effect thinkers

      if (th->function == T_Pusher)
        {
          *save_p++ = tc_pusher;
          memcpy (save_p, th, sizeof(pusher_t));
          save_p += sizeof(pusher_t);
          continue;
        }
    }

  // add a terminating marker
  *save_p++ = tc_endspecials;
}


//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials (void)
{
  byte tclass;

  // read in saved thinkers
  while ((tclass = *save_p++) != tc_endspecials)  // killough 2/14/98
    switch (tclass)
      {
      case tc_ceiling:
        PADSAVEP();
        {
          ceiling_t *ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVEL, NULL);
          memcpy (ceiling, save_p, sizeof(*ceiling));
          save_p += sizeof(*ceiling);
          ceiling->sector = &sectors[(int)ceiling->sector];
          ceiling->sector->ceilingdata = ceiling; //jff 2/22/98

          if (ceiling->thinker.function)
            ceiling->thinker.function = T_MoveCeiling;

          P_AddThinker (&ceiling->thinker);
          P_AddActiveCeiling(ceiling);
          break;
        }

      case tc_door:
        PADSAVEP();
        {
          vldoor_t *door = Z_Malloc (sizeof(*door), PU_LEVEL, NULL);
          memcpy (door, save_p, sizeof(*door));
          save_p += sizeof(*door);
          door->sector = &sectors[(int)door->sector];

          //jff 1/31/98 unarchive line remembered by door as well
          door->line = (int)door->line!=-1? &lines[(int)door->line] : NULL;

          door->sector->ceilingdata = door;       //jff 2/22/98
          door->thinker.function = T_VerticalDoor;
          P_AddThinker (&door->thinker);
          break;
        }

      case tc_floor:
        PADSAVEP();
        {
          floormove_t *floor = Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
          memcpy (floor, save_p, sizeof(*floor));
          save_p += sizeof(*floor);
          floor->sector = &sectors[(int)floor->sector];
          floor->sector->floordata = floor; //jff 2/22/98
          floor->thinker.function = T_MoveFloor;
          P_AddThinker (&floor->thinker);
          break;
        }

      case tc_plat:
        PADSAVEP();
        {
          plat_t *plat = Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
          memcpy (plat, save_p, sizeof(*plat));
          save_p += sizeof(*plat);
          plat->sector = &sectors[(int)plat->sector];
          plat->sector->floordata = plat; //jff 2/22/98

          if (plat->thinker.function)
            plat->thinker.function = T_PlatRaise;

          P_AddThinker (&plat->thinker);
          P_AddActivePlat(plat);
          break;
        }

      case tc_flash:
        PADSAVEP();
        {
          lightflash_t *flash = Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
          memcpy (flash, save_p, sizeof(*flash));
          save_p += sizeof(*flash);
          flash->sector = &sectors[(int)flash->sector];
          flash->thinker.function = T_LightFlash;
          P_AddThinker (&flash->thinker);
          break;
        }

      case tc_strobe:
        PADSAVEP();
        {
          strobe_t *strobe = Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
          memcpy (strobe, save_p, sizeof(*strobe));
          save_p += sizeof(*strobe);
          strobe->sector = &sectors[(int)strobe->sector];
          strobe->thinker.function = T_StrobeFlash;
          P_AddThinker (&strobe->thinker);
          break;
        }

      case tc_glow:
        PADSAVEP();
        {
          glow_t *glow = Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
          memcpy (glow, save_p, sizeof(*glow));
          save_p += sizeof(*glow);
          glow->sector = &sectors[(int)glow->sector];
          glow->thinker.function = T_Glow;
          P_AddThinker (&glow->thinker);
          break;
        }

      case tc_flicker:           // killough 10/4/98
        PADSAVEP();
        {
          fireflicker_t *flicker = Z_Malloc (sizeof(*flicker), PU_LEVEL, NULL);
          memcpy (flicker, save_p, sizeof(*flicker));
          save_p += sizeof(*flicker);
          flicker->sector = &sectors[(int)flicker->sector];
          flicker->thinker.function = T_FireFlicker;
          P_AddThinker (&flicker->thinker);
          break;
        }

        //jff 2/22/98 new case for elevators
      case tc_elevator:
        PADSAVEP();
        {
          elevator_t *elevator = Z_Malloc (sizeof(*elevator), PU_LEVEL, NULL);
          memcpy (elevator, save_p, sizeof(*elevator));
          save_p += sizeof(*elevator);
          elevator->sector = &sectors[(int)elevator->sector];
          elevator->sector->floordata = elevator; //jff 2/22/98
          elevator->sector->ceilingdata = elevator; //jff 2/22/98
          elevator->thinker.function = T_MoveElevator;
          P_AddThinker (&elevator->thinker);
          break;
        }

      case tc_scroll:       // killough 3/7/98: scroll effect thinkers
        {
          scroll_t *scroll = Z_Malloc (sizeof(scroll_t), PU_LEVEL, NULL);
          memcpy (scroll, save_p, sizeof(scroll_t));
          save_p += sizeof(scroll_t);
          scroll->thinker.function = T_Scroll;
          P_AddThinker(&scroll->thinker);
          break;
        }

      case tc_pusher:   // phares 3/22/98: new Push/Pull effect thinkers
        {
          pusher_t *pusher = Z_Malloc (sizeof(pusher_t), PU_LEVEL, NULL);
          memcpy (pusher, save_p, sizeof(pusher_t));
          save_p += sizeof(pusher_t);
          pusher->thinker.function = T_Pusher;
          pusher->source = P_GetPushThing(pusher->affectee);
          P_AddThinker(&pusher->thinker);
          break;
        }

      default:
        I_Error ("P_UnarchiveSpecials:Unknown tclass %i "
                 "in savegame",tclass);
      }
}

// killough 2/16/98: save/restore random number generator state information

void P_ArchiveRNG(void)
{
  CheckSaveGame(sizeof rng);
  memcpy(save_p, &rng, sizeof rng);
  save_p += sizeof rng;
}

void P_UnArchiveRNG(void)
{
  memcpy(&rng, save_p, sizeof rng);
  save_p += sizeof rng;
}

// killough 2/22/98: Save/restore automap state
void P_ArchiveMap(void)
{
  CheckSaveGame(sizeof followplayer + sizeof markpointnum +
                markpointnum * sizeof *markpoints +
                sizeof automapactive);

  memcpy(save_p, &automapactive, sizeof automapactive);
  save_p += sizeof automapactive;
  memcpy(save_p, &followplayer, sizeof followplayer);
  save_p += sizeof followplayer;
  memcpy(save_p, &automap_grid, sizeof automap_grid);
  save_p += sizeof automap_grid;
  memcpy(save_p, &markpointnum, sizeof markpointnum);
  save_p += sizeof markpointnum;

  if (markpointnum)
    {
      memcpy(save_p, markpoints, sizeof *markpoints * markpointnum);
      save_p += markpointnum * sizeof *markpoints;
    }
}

void P_UnArchiveMap(void)
{
  if(!hub_changelevel) memcpy(&automapactive, save_p, sizeof automapactive);
  save_p += sizeof automapactive;
  if(!hub_changelevel) memcpy(&followplayer, save_p, sizeof followplayer);
  save_p += sizeof followplayer;
  if(!hub_changelevel) memcpy(&automap_grid, save_p, sizeof automap_grid);
  save_p += sizeof automap_grid;

  if (automapactive)
    AM_Start();

  memcpy(&markpointnum, save_p, sizeof markpointnum);
  save_p += sizeof markpointnum;

  if (markpointnum)
    {
      while (markpointnum >= markpointnum_max)
        markpoints = realloc(markpoints, sizeof *markpoints *
         (markpointnum_max = markpointnum_max ? markpointnum_max*2 : 16));
      memcpy(markpoints, save_p, markpointnum * sizeof *markpoints);
      save_p += markpointnum * sizeof *markpoints;
    }
}

/*******************************
                SCRIPT SAVING
 *******************************/

// save all the neccesary FraggleScript data.
// we need to save the levelscript(all global
// variables), the runningscripts (scripts
// currently suspended) and the spawnedthings
// array (array of ptrs. to things spawned at
// level start)

// haleyjd 11/23/00: Here I sit at 4:07 am on Thanksgiving of the
// year 2000 attempting to finish up a rewrite of the FraggleScript
// higher architecture. ::sighs::
//
// haleyjd: note - as of 01/07/01 I have reverted FraggleScript
// back to the two-area system of v3.21 -- the levelscript and
// a hub script for semi-permanent variables. While the loss of
// the global script areas is regrettable, its the last resort.
//

/***************** save the levelscript *************/

// make sure we remember all the global
// variables.

void P_ArchiveLevelScript()
{
  int num_variables;
  int i;
  int *int_p;
  short *short_p;
  
  // all we really need to do is save the variables
  // count the variables first
  
  // count number of variables
  num_variables = 0;
  for(i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript.variables[i];
      while(sv && sv->type != svt_label)
	{
	  num_variables++;
	  sv = sv->next;
	}
    }
  
  CheckSaveGame(sizeof(short));
  
  short_p = (short *)save_p;
  *short_p++ = num_variables;    // write num_variables
  save_p = (unsigned char *)short_p;      // restore save_p
  
  // go thru hash chains, store each variable
  for(i=0; i<VARIABLESLOTS; i++)
    {
      // go thru this hashchain
      svariable_t *sv = levelscript.variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while(sv && sv->type != svt_label)
	{
	  
	  CheckSaveGame(strlen(sv->name) + 1 + sizeof(int));
	  
	  // write svariable: name
	  
	  strcpy(save_p, sv->name);
	  save_p += strlen(sv->name) + 1; // 1 extra for ending NULL
                
	  // type
	  int_p = (int *)save_p;
	  *int_p++ = sv->type;   // store type;
	  save_p = (unsigned char *)int_p;
	  
	  switch(sv->type)        // store depending on type
	    {
	    case svt_string:
	      {
		CheckSaveGame(strlen(sv->value.s)+5); // 5 for safety
		strcpy(save_p, sv->value.s);
		save_p += strlen(sv->value.s) + 1;
		break;
	      }
	    case svt_int:
	      {
		long *long_p;
		
		CheckSaveGame(sizeof(long)); 
		long_p = (long *) save_p;
		*long_p++ = sv->value.i;
		save_p = (unsigned char *)long_p;
		break;
	      }
	    case svt_mobj:
	      {
		long *long_p;
		
		CheckSaveGame(sizeof(long)); 
		long_p = (long *) save_p;
		*long_p++ = P_MobjNum(sv->value.mobj);
		save_p = (unsigned char *)long_p;
		break;
	      }
	    case svt_fixed: // haleyjd: forgotten!
	       {
		  fixed_t *fixed_p;

		  CheckSaveGame(sizeof(fixed_t));
		  fixed_p = (fixed_t *)save_p;
		  *fixed_p++ = sv->value.fixed;
		  save_p = (unsigned char *)fixed_p;
		  break;
	       }
	    case svt_array: // haleyjd: arrays
	       {
		  sfarray_t *cur;
		  int *int_p;

		  cur = sfsavelist.next;
		  while(cur && sv->value.a != cur)
		     cur = cur->next;

		  CheckSaveGame(sizeof(int));
		  int_p = (int *)save_p;

		  // zero is unused, so use it for NULL
		  *int_p++ = cur ? cur->saveindex : 0;

		  save_p = (unsigned char *)int_p;
		  break;
	       }
	    }
	  sv = sv->next;
	}
    }
}

void P_UnArchiveLevelScript()
{
  short *short_p;
  int i;
  int *int_p;
  int num_variables;
  
  // free all the variables in the current levelscript first
  
  for(i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript.variables[i];
      
      while(sv && sv->type != svt_label)
	{
	  svariable_t *next = sv->next;
	  Z_Free(sv);
	  sv = next;
	}
      levelscript.variables[i] = sv;       // null or label
    }

  // now read the number of variables from the savegame file
  
  short_p = (short *)save_p;
  num_variables = *short_p++;
  save_p = (unsigned char *)short_p;
  
  for(i=0; i<num_variables; i++)
    {
      svariable_t *sv = Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
      int hashkey;
      
      // name
      sv->name = Z_Strdup(save_p, PU_LEVEL, 0);
      save_p += strlen(sv->name) + 1;
      
      int_p = (int *)save_p;
      sv->type = *int_p++;
      save_p = (unsigned char *)int_p;
      
      switch(sv->type)        // read depending on type
	{
	case svt_string:
	  {
	    sv->value.s = Z_Strdup(save_p, PU_LEVEL, 0);
	    save_p += strlen(sv->value.s) + 1;
	    break;
	  }
	case svt_int:
	  {
	    long *long_p = (long *) save_p;
	    sv->value.i = *long_p++;
	    save_p = (unsigned char *)long_p;
	    break;
	  }
	case svt_mobj:
	  {
	    long *long_p = (long *) save_p;
	    sv->value.mobj = P_MobjForNum(*long_p++);
	    save_p = (unsigned char *)long_p;
	    break;
	  }
        case svt_fixed:  // haleyjd: forgotten!
	   {
	      fixed_t *fixed_p = (fixed_t *)save_p;
	      sv->value.fixed = *fixed_p++;
	      save_p = (unsigned char *)fixed_p;
	      break;
	   }
	case svt_array: // haleyjd: arrays
	   {
	      int ordinal;
	      int *int_p; 
	      
	      int_p = (int *)save_p;	      
	      ordinal = *int_p++;
	      save_p = (unsigned char *)int_p;

	      if(!ordinal)
	      {
		 sv->value.a = NULL;
	      }
	      else
	      {
		 sfarray_t *cur = sfsavelist.next;

		 while(cur)
		 {
		    if(ordinal == cur->saveindex)
		       break;

		    cur = cur->next;
		 }
		 
		 // set even if cur is NULL somehow (not a problem)
		 sv->value.a = cur;
	      }
	      break;
	   }
     default:
     break;
	}
      
      // link in the new variable
      hashkey = variable_hash(sv->name);
      sv->next = levelscript.variables[hashkey];
      levelscript.variables[hashkey] = sv;
    }
}

// haleyjd:
// If the hub system is to become capable of lasting over game
// sessions, the hub script information MUST be saved. If the save
// is permanent (ie a real save or load action by the player) then
// we need this information. If its a hub save or load, then we'll
// still load it, but immediately overwrite it afterward in the hub
// code with the current values.

void P_ArchiveHubScript(void)
{
   int num_variables;
   int i;
   int *int_p;
   short *short_p;

   // all we really need to do is save the variables
   // count the variables first
  
   // count number of variables
   num_variables = 0;
   for(i=0; i<VARIABLESLOTS; i++)
   {
      svariable_t *sv = hub_script.variables[i];
      while(sv && sv->type != svt_label)
      {
	  num_variables++;
	  sv = sv->next;
      }
   }
   
   CheckSaveGame(sizeof(short));
  
   short_p = (short *) save_p;
   *short_p++ = num_variables;    // write num_variables
   save_p = (unsigned char *) short_p;      // restore save_p
  
   // go thru hash chains, store each variable
   for(i=0; i<VARIABLESLOTS; i++)
   {
      // go thru this hashchain
      svariable_t *sv = hub_script.variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while(sv && sv->type != svt_label)
      {
	 CheckSaveGame(strlen(sv->name) + 1 + sizeof(int));
	 
	 // write svariable: name
	 strcpy(save_p, sv->name);
	 save_p += (strlen(sv->name) + 1); // 1 extra for ending NULL
                
	 // type -- haleyjd: rewritten to be type safe
	 int_p = (int *)save_p;
	 *int_p++ = sv->type;
	 save_p = (unsigned char *)int_p;
	  
	 switch(sv->type)        // store depending on type
	 {
	 case svt_string:
	    {
	       CheckSaveGame(strlen(sv->value.s)+5); // 5 for safety
	       strcpy(save_p, sv->value.s);
	       save_p += strlen(sv->value.s) + 1;
	       break;
	    }
	 case svt_int:
	    {
	       long *long_p;
	       CheckSaveGame(sizeof(long)); 
	       long_p = (long *) save_p;
	       *long_p++ = sv->value.i;
	       save_p = (unsigned char *)long_p;
	       break;
	    }
	 case svt_mobj:
	    {
	       // haleyjd 06/06/01: 
	       // not allowed, so ignore if they got in somehow
	       break;
	    }
	 case svt_fixed: // haleyjd: forgotten!
	    {
	       fixed_t *fixed_p;
	       CheckSaveGame(sizeof(fixed_t));
	       fixed_p = (fixed_t *)save_p;
	       *fixed_p++ = sv->value.fixed;
	       save_p = (unsigned char *)fixed_p;
	       break;
	    }
	 case svt_array: // haleyjd: arrays are not allowed
	    break;
	 }
	 sv = sv->next;
      }
   }
}

void P_UnArchiveHubScript(void)
{
  short *short_p;
  int i;
  int *int_p;
  int num_variables;
  
  // free all the variables in the current levelscript first
  
  for(i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = hub_script.variables[i];
      
      while(sv && sv->type != svt_label)
	{
	  svariable_t *next = sv->next;
	  Z_Free(sv);
	  sv = next;
	}
      hub_script.variables[i] = sv;       // null or label
    }

  // now read the number of variables from the savegame file
  
  short_p = (short *)save_p;
  num_variables = *short_p++;
  save_p = (unsigned char *)short_p;
  
  for(i=0; i<num_variables; i++)
    {
      svariable_t *sv = Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
      int hashkey;
      
      // name
      sv->name = Z_Strdup(save_p, PU_LEVEL, 0);
      save_p += strlen(sv->name) + 1;
      
      int_p = (int *)save_p;
      sv->type = *int_p++;
      save_p = (unsigned char *)int_p;
      
      switch(sv->type)        // read depending on type
	{
	case svt_string:
	  {
	    sv->value.s = Z_Strdup(save_p, PU_LEVEL, 0);
	    save_p += strlen(sv->value.s) + 1;
	    break;
	  }
	case svt_int:
	  {
	    long *long_p = (long *) save_p;
	    sv->value.i = *long_p++;
	    save_p = (unsigned char *)long_p;
	    break;
	  }
	case svt_mobj:
	  {
	     // haleyjd 06/06/01: 
	     // not allowed, so ignore if they got in somehow
	     break;
	  }
        case svt_fixed:  // haleyjd: forgotten!
	   {
	      fixed_t *fixed_p = (fixed_t *)save_p;
	      sv->value.fixed = *fixed_p++;
	      save_p = (unsigned char *)fixed_p;
	      break;
	   }
	case svt_array: // haleyjd: not allowed
	   break;
	default:
     break;
	}
      
      // link in the new variable
      hashkey = variable_hash(sv->name);
      sv->next = hub_script.variables[hashkey];
      hub_script.variables[hashkey] = sv;
    }
}

/**************** save the runningscripts ***************/

extern runningscript_t runningscripts;        // t_script.c
runningscript_t *new_runningscript();         // t_script.c
void clear_runningscripts();		      // t_script.c

// save a given runningscript

void P_ArchiveRunningScript(runningscript_t *rs)
{
  short *short_p;
  int i;
  int *int_p;
  int num_variables;
  
  CheckSaveGame(sizeof(short) * 8); // room for 8 shorts
  
  short_p = (short *) save_p;

  *short_p++ = rs->script->scriptnum;      // save scriptnum
  *short_p++ = rs->savepoint - rs->script->data; // offset
  *short_p++ = rs->wait_type;
  *short_p++ = rs->wait_data;
  
  // save pointer to trigger using prev
  *short_p++ = P_MobjNum(rs->trigger);
  
  // count number of variables
  num_variables = 0;
  for(i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = rs->variables[i];
      while(sv && sv->type != svt_label)
	{
	  num_variables++;
	  sv = sv->next;
	}
    }
  *short_p++ = num_variables;

  save_p = (unsigned char *)short_p;
  
  // save num_variables
  
  // store variables
  // go thru hash chains, store each variable
  for(i=0; i<VARIABLESLOTS; i++)
    {
      // go thru this hashchain
      svariable_t *sv = rs->variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while(sv && sv->type != svt_label)
	{
	  
	  CheckSaveGame(strlen(sv->name) + 1 + sizeof(int));
	  
	  // write svariable: name
	  
	  strcpy(save_p, sv->name);
	  save_p += (strlen(sv->name) + 1); // 1 extra for ending NULL
                
	  // type
	  int_p = (int *)save_p;
	  *int_p++ = sv->type;   // store type;
	  save_p = (unsigned char *)int_p;
	  
	  switch(sv->type)        // store depending on type
	    {
	    case svt_string:
	      {
		CheckSaveGame(strlen(sv->value.s)+5); // 5 for safety
		strcpy(save_p, sv->value.s);
		save_p += strlen(sv->value.s) + 1;
		break;
	      }
	    case svt_int:
	      {
		long *long_p;
		
		CheckSaveGame(sizeof(long)+4); 
		long_p = (long *) save_p;
		*long_p++ = sv->value.i;
		save_p = (unsigned char *)long_p;
		break;
	      }
	    case svt_mobj:
	      {
		long *long_p;
		
		CheckSaveGame(sizeof(long)+4); 
		long_p = (long *) save_p;
		*long_p++ = P_MobjNum(sv->value.mobj);
		save_p = (unsigned char *)long_p;
		break;
	      }
	    case svt_fixed: // haleyjd: forgotten!
	       {
		  fixed_t *fixed_p;

		  CheckSaveGame(sizeof(fixed_t)+4);
		  fixed_p = (fixed_t *)save_p;
		  *fixed_p++ = sv->value.fixed;
		  save_p = (unsigned char *)fixed_p;
		  break;
	       }
	    case svt_array:
	       {
  		  sfarray_t *cur;
		  int *int_p;

		  cur = sfsavelist.next;
		  while(cur && sv->value.a != cur)
		     cur = cur->next;

		  CheckSaveGame(sizeof(int));
		  int_p = (int *)save_p;

		  // zero is unused, so use it for NULL
		  *int_p++ = cur ? cur->saveindex : 0;

		  save_p = (unsigned char *)int_p;
		  break;
	       }
	    // others do not appear in user scripts
	    
	    default:
	       break;
	    }
	  
	  sv = sv->next;
	}
    }
}

// get the next runningscript

runningscript_t *P_UnArchiveRunningScript()
{
  int i;
  int scriptnum;
  int num_variables;
  runningscript_t *rs;

  // create a new runningscript
  rs = new_runningscript();
  
  {
    short *short_p = (short*) save_p;

    scriptnum = *short_p++;        // get scriptnum
    
    // levelscript?
    rs->script = 
     (scriptnum == -1) ? &levelscript : levelscript.children[scriptnum];

    // read out offset from save
    rs->savepoint = rs->script->data + (*short_p++);
    rs->wait_type = *short_p++;
    rs->wait_data = *short_p++;
    // read out trigger thing
    rs->trigger = P_MobjForNum(*short_p++);
    
    // get number of variables
    num_variables = *short_p++;

    save_p = (unsigned char*) short_p;      // restore save_p
  }
  
  // read out the variables now (fun!)
  
  // start with basic script slots/labels
  
  for(i=0; i<VARIABLESLOTS; i++)
    rs->variables[i] = rs->script->variables[i];
  
  for(i=0; i<num_variables; i++)
    {
      svariable_t *sv = Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
      int hashkey;
      
      // name
      sv->name = Z_Strdup(save_p, PU_LEVEL, 0);
      save_p += strlen(sv->name) + 1;
      
      sv->type = *save_p++;
      
      switch(sv->type)        // read depending on type
	{
	case svt_string:
	  {
	    sv->value.s = Z_Strdup(save_p, PU_LEVEL, 0);
	    save_p += strlen(sv->value.s) + 1;
	    break;
	  }
	case svt_int:
	  {
	    long *long_p = (long *) save_p;
	    sv->value.i = *long_p++;
	    save_p = (char *)long_p;
	    break;
	  }
	case svt_mobj:
	  {
	    long *long_p = (long *) save_p;
	    sv->value.mobj = P_MobjForNum(*long_p++);
	    save_p = (char *)long_p;
	    break;
	  }
        case svt_fixed:  // haleyjd: forgotten!
	  {
	     fixed_t *fixed_p = (fixed_t *)save_p;
	     sv->value.fixed = *fixed_p++;
	     save_p = (char *)fixed_p;
	     break;
	  }
	case svt_array:
	   {
	      int ordinal;
	      int *int_p;

	      int_p = (int *)save_p;
	      ordinal = *int_p++;
	      save_p = (unsigned char *)int_p;

	      if(!ordinal)
	      {
		 sv->value.a = NULL;
	      }
	      else
	      {
		 sfarray_t *cur = sfsavelist.next;

		 while(cur)
		 {
		    if(ordinal == cur->saveindex)
		       break;

		    cur = cur->next;
		 }

		 sv->value.a = cur;
	      }	      
	      break;
	   }
	default:
	   break;
	}
      
      // link in the new variable
      hashkey = variable_hash(sv->name);
      sv->next = rs->variables[hashkey];
      rs->variables[hashkey] = sv;
    }
  
  return rs;
}

        // archive all runningscripts in chain
void P_ArchiveRunningScripts()
{
  long *long_p;
  runningscript_t *rs;
  int num_runningscripts = 0;
  
  // count runningscripts
  for(rs = runningscripts.next; rs; rs = rs->next)
    num_runningscripts++;
  
  CheckSaveGame(sizeof(long));
  
  // store num_runningscripts
  long_p = (long *) save_p;
  *long_p++ = num_runningscripts;
  save_p = (unsigned char *)long_p;        
  
  // now archive them
  rs = runningscripts.next;
  while(rs)
    {
      P_ArchiveRunningScript(rs);
      rs = rs->next;
    }
  
  // haleyjd: what's this for?
  //long_p = (long *) save_p;
}

        // restore all runningscripts from save_p
void P_UnArchiveRunningScripts()
{
  runningscript_t *rs;
  long *long_p;
  int num_runningscripts;
  int i;
  
  // remove all runningscripts first : may have been started
  // by levelscript on level load
  
  clear_runningscripts();
  
  // get num_runningscripts
  long_p = (long *) save_p;
  num_runningscripts = *long_p++;
  save_p = (unsigned char *) long_p;        
  
  for(i=0; i<num_runningscripts; i++)
    {
      // get next runningscript
      rs = P_UnArchiveRunningScript();
      
      // hook into chain
      rs->next = runningscripts.next;
      rs->prev = &runningscripts;
      rs->prev->next = rs;
      if(rs->next)
	rs->next->prev = rs;
    }
}

/******************** save spawnedthings *****************/

void P_ArchiveSpawnedThings()
{
  int i;
  long *long_p;
  
  CheckSaveGame(sizeof(long) * numthings); // killough
  
  long_p = (long *) save_p;
  
  for(i=0; i<numthings; i++)
    {
      *long_p++ = P_MobjNum(spawnedthings[i]);       // store it
    }
  
  save_p = (unsigned char *) long_p;       // restore save_p
}

void P_UnArchiveSpawnedThings()
{
  long *long_p;
  int i;
  
  // restore spawnedthings
  long_p = (long *) save_p;
  
  for(i=0; i<numthings; i++)
    {
      spawnedthings[i] = P_MobjForNum(*long_p++);
    }
  
  save_p = (unsigned char *) long_p;       // restore save_p
}

        /*************** main script saving functions ************/

void P_ArchiveFSArrays(void);
void P_UnArchiveFSArrays(void);

void P_ArchiveScripts()
{
  // save thing list
  P_ArchiveSpawnedThings();

  // haleyjd: save FS arrays
  P_ArchiveFSArrays();

  // save levelscript
  P_ArchiveLevelScript();

  // haleyjd: save hub script variables too
  P_ArchiveHubScript();
  
  // save runningscripts
  P_ArchiveRunningScripts();
}

void P_UnArchiveScripts()
{
  // get thing list
  P_UnArchiveSpawnedThings();

  // haleyjd: restore FS arrays
  P_UnArchiveFSArrays();

  // restore levelscript
  P_UnArchiveLevelScript();

  // haleyjd: save hub script variables too
  P_UnArchiveHubScript();

  // restore runningscripts
  P_UnArchiveRunningScripts();
}

//
// FS HU Pics save support -- haleyjd 03/21/01: joy!
//

void P_ArchiveFSPics(void)
{
   fspic_t *rover, *pic_p;
   int *int_p;
   size_t picsize, *size_p;
   
   int count = 0;

   // count number of fspics
   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      count++;
   }

   CheckSaveGame(sizeof(int)*2 + sizeof(size_t) + sizeof(fspic_t)*count);

   int_p = (int *)save_p;
   *int_p++ = count;
   *int_p = lasthandle;
   save_p += sizeof(int) * 2;

   picsize = sizeof(fspic_t) * count;
   size_p = (size_t *)save_p;
   *size_p = picsize;
   save_p += sizeof(size_t);

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      pic_p = (fspic_t *)save_p;
      memcpy(pic_p, rover, sizeof(fspic_t));
      save_p += sizeof(fspic_t);
      
      pic_p->next = NULL; // restored by P_UnArchiveFSPics
      pic_p->data = NULL; // restored by HU_DrawFSPics
   }
}

void P_UnArchiveFSPics(void)
{
   int *int_p;
   int count, i;
   size_t *size_p;
   fspic_t *newfsp, *pic_p, *last;

   int_p = (int *)save_p;
   // read count
   count = *int_p++;
   // read lasthandle
   lasthandle = *int_p;
   save_p += sizeof(int) * 2;

   size_p = (size_t *)save_p;
   if(*size_p != sizeof(fspic_t) * count)
      I_Error("P_UnArchiveFSPics: save game inconsistency!\n");
   save_p += sizeof(size_t);
   
   last = &fspiclist;
   
   for(i=0; i<count; i++)
   {
      newfsp = Z_Malloc(sizeof(fspic_t), PU_LEVEL, NULL);
      pic_p = (fspic_t *)save_p;
      memcpy(newfsp, pic_p, sizeof(fspic_t));
      save_p += sizeof(fspic_t);

      last->next = newfsp;
      last = newfsp;
   }
}

//
// FS Array Saving
//
// Array variables are saved by the code above for the level and
// running scripts, but first this stuff needs to be done -- enumerate
// and archive the arrays themselves.
//

unsigned int num_fsarrays;

static void P_NumberFSArrays(void)
{
   sfarray_t *cur;

   num_fsarrays = 0;

   cur = sfsavelist.next; // start at first array

   while(cur)
   {
      cur->saveindex = ++num_fsarrays;
      cur = cur->next;
   }
}

static size_t P_CalcFSArraySize(void)
{
   size_t total = 0;
   sfarray_t *cur = sfsavelist.next;

   while(cur)
   {
      total += (sizeof(svalue_t) * cur->length);
      cur = cur->next;
   }

   return total;
}

// must be called before running/level script archiving
void P_ArchiveFSArrays(void)
{
   size_t sizeToSave;
   sfarray_t *cur;
   unsigned int *uint_p;
   svalue_t *sval_p;

   P_NumberFSArrays(); // number all the arrays

   // unsigned ints: size of each array and number of arrays total
   // P_CalcFSArraySize: length of all arrays * sizeof svalue_t
   sizeToSave = sizeof(unsigned int) * (num_fsarrays+1) + 
                P_CalcFSArraySize();

   CheckSaveGame(sizeToSave);

   // write number of FS arrays
   uint_p = (unsigned int *)save_p;
   *uint_p++ = num_fsarrays;
   save_p = (unsigned char *)uint_p;
      
   // start at first array
   cur = sfsavelist.next;

   while(cur)
   {
      int i;

      // write the length of this array
      uint_p = (unsigned int *)save_p;
      *uint_p++ = cur->length;
      save_p = (unsigned char *)uint_p;

      // write the contents of this array
      for(i=0; i<cur->length; i++)
      {
	 mobj_t *temp = NULL;
	 
	 // must weed out mobj references and use savegame index
	 if(cur->values[i].type == svt_mobj)
	 {
	    // save pointer value
	    temp = cur->values[i].value.mobj;
	    // write ordinal into pointer
	    cur->values[i].value.mobj = (mobj_t *)P_MobjNum(temp);
	 }
	 
	 sval_p = (svalue_t *)save_p;
	 memcpy(sval_p, &(cur->values[i]), sizeof(svalue_t));
	 save_p += sizeof(svalue_t);

	 // restore to pointer value
	 if(cur->values[i].type == svt_mobj)
	    cur->values[i].value.mobj = temp;
      }

      cur = cur->next;
   }
}

// must be called before unarchiving running/level scripts
void P_UnArchiveFSArrays(void)
{
   unsigned int *uint_p;
   int i;
   sfarray_t *newArray, *last;
   svalue_t *sval_p;

   T_InitSaveList(); // reinitialize the save list

   // read number of FS arrays
   uint_p = (unsigned int *)save_p;
   num_fsarrays = *uint_p++;
   save_p = (unsigned char *)uint_p;

   last = &sfsavelist;
   // read all the arrays
   for(i=0; i<num_fsarrays; i++)
   {
      int j;
      
      newArray = Z_Malloc(sizeof(sfarray_t), PU_LEVEL, NULL);
      memset(newArray, 0, sizeof(sfarray_t));

      // read length of this array
      uint_p = (unsigned int *)save_p;
      newArray->length = *uint_p++;
      save_p = (unsigned char *)uint_p;
      
      newArray->values = Z_Malloc(newArray->length * sizeof(svalue_t),
	                          PU_LEVEL, NULL);
      
      // read all archived values
      for(j=0; j<newArray->length; j++)
      {
	 sval_p = (svalue_t *)save_p;
	 memcpy(&(newArray->values[j]), sval_p, sizeof(svalue_t));
	 save_p += sizeof(svalue_t);

	 // set mobj references back to appropriate pointer
	 if(newArray->values[j].type == svt_mobj)
	 {
	    newArray->values[j].value.mobj = 
	       P_MobjForNum((int)(newArray->values[j].value.mobj));
	 }
      }

      // link in the new array -- must reconstruct list in same
      // order as read (T_AddArray will not work for this)
      last->next = newArray;
      last = newArray;

   }

   // now number all the arrays
   P_NumberFSArrays();
}

//----------------------------------------------------------------------------
//
// $Log: p_saveg.c,v $
// Revision 1.17  1998/05/03  23:10:22  killough
// beautification
//
// Revision 1.16  1998/04/19  01:16:06  killough
// Fix boss brain spawn crashes after loadgames
//
// Revision 1.15  1998/03/28  18:02:17  killough
// Fix boss spawner savegame crash bug
//
// Revision 1.14  1998/03/23  15:24:36  phares
// Changed pushers to linedef control
//
// Revision 1.13  1998/03/23  03:29:54  killough
// Fix savegame crash caused in P_ArchiveWorld
//
// Revision 1.12  1998/03/20  00:30:12  phares
// Changed friction to linedef control
//
// Revision 1.11  1998/03/09  07:20:23  killough
// Add generalized scrollers
//
// Revision 1.10  1998/03/02  12:07:18  killough
// fix stuck-in wall loadgame bug, automap status
//
// Revision 1.9  1998/02/24  08:46:31  phares
// Pushers, recoil, new friction, and over/under work
//
// Revision 1.8  1998/02/23  04:49:42  killough
// Add automap marks and properties to saved state
//
// Revision 1.7  1998/02/23  01:02:13  jim
// fixed elevator size, comments
//
// Revision 1.4  1998/02/17  05:43:33  killough
// Fix savegame crashes and monster sleepiness
// Save new RNG info
// Fix original plats height bug
//
// Revision 1.3  1998/02/02  22:17:55  jim
// Extended linedef types
//
// Revision 1.2  1998/01/26  19:24:21  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
