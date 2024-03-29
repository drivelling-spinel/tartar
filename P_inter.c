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
//      Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_inter.c,v 1.10 1998/05/03 23:09:29 killough Exp $";

#include "c_io.h"
#include "doomstat.h"
#include "dstrings.h"
#include "m_random.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_frags.h"
#include "am_map.h"
#include "r_main.h"
#include "r_segs.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_tick.h"
#include "d_deh.h"  // Ty 03/22/98 - externalized strings
#include "d_player.h"
#include "p_inter.h"

#define BONUSADD        6

// Ty 03/07/98 - add deh externals
// Maximums and such were hardcoded values.  Need to externalize those for
// dehacked support (and future flexibility).  Most var names came from the key
// strings used in dehacked.

int initial_health = 100;
int initial_bullets = 50;
int maxhealth = 100; // was MAXHEALTH as a #define, used only in this module
int max_armor = 200;
int green_armor_class = 1;  // these are involved with armortype below
int blue_armor_class = 2;
int max_soul = 200;
int soul_health = 100;
int mega_health = 200;
int god_health = 100;   // these are used in cheats (see st_stuff.c)
int idfa_armor = 200;
int idfa_armor_class = 2;
// not actually used due to pairing of cheat_k and cheat_fa
int idkfa_armor = 200;
int idkfa_armor_class = 2;

int bfgcells = 40;      // used in p_pspr.c
// Ty 03/07/98 - end deh externals

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO]  = {200, 50, 300, 50, 50, 100};  //haleyjd: grenades
int clipammo[NUMAMMO] = { 10,  4,  20,  1, 1, 0}; // joel - for cross

//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

boolean P_GiveAmmo(player_t *player, ammotype_t ammo, int num)
{
  int oldammo;

  if (ammo == am_noammo)
    return false;

  // haleyjd: Part of the campaign to eliminate unnecessary
  //   I_Error bombouts for things that can be ignored safely
  if((unsigned) ammo > NUMAMMO)
  {
     C_Printf("P_GiveAmmo: bad type %i\a\n", ammo);
     return false;
     //I_Error ("P_GiveAmmo: bad type %i", ammo);
  }

  if ( player->ammo[ammo] == player->maxammo[ammo]  )
    return false;

  if(ammo == am_gren && !EternityMode) // haleyjd
    return false;

  if (num)
    num *= clipammo[ammo];
  else
    num = clipammo[ammo]/2;

  // give double ammo in trainer mode, you'll need in nightmare
  if (gameskill == sk_baby || gameskill == sk_nightmare)
    num <<= 1;

  oldammo = player->ammo[ammo];
  player->ammo[ammo] += num;

  if (player->ammo[ammo] > player->maxammo[ammo])
    player->ammo[ammo] = player->maxammo[ammo];

  // If non zero ammo, don't change up weapons, player was lower on purpose.
  if (oldammo)
    return true;

  if (P_ShouldKeepWeapon(player))
    return true;

  // We were down to zero, so select a new weapon.
  // Preferences are not user selectable.
  switch (ammo)
    {
    case am_clip:
      if (player->readyweapon == wp_fist)
        if (player->weaponowned[wp_chaingun])
          player->pendingweapon = wp_chaingun;
        else
          player->pendingweapon = wp_pistol;
      break;

    case am_shell:
      if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
        if (player->weaponowned[wp_shotgun])
          player->pendingweapon = wp_shotgun;
        break;

      case am_cell:
        if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
          if (player->weaponowned[wp_plasma])
            player->pendingweapon = wp_plasma;
        break;

      case am_misl:
        if (player->readyweapon == wp_fist)
          if (player->weaponowned[wp_missile])
            player->pendingweapon = wp_missile;
        break;

      case am_gren:  // haleyjd
        if(player->readyweapon == wp_fist)
          if(player->weaponowned[wp_grenade])
            player->pendingweapon = wp_grenade;
        break;

    default:
      break;
    }
  return true;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//

boolean P_GiveWeapon(player_t *player, weapontype_t weapon, boolean dropped)
{
  boolean gaveammo;

  if(weapon == wp_cross && gamemission != cod)
    return false;

  if(weapon == wp_grenade && !EternityMode) // haleyjd
    return false;

  if (netgame && deathmatch!=2 && !dropped)
    {
      // leave placed weapons forever on net games
      if (player->weaponowned[weapon])
        return false;

      player->bonuscount += BONUSADD;
      player->weaponowned[weapon] = true;

      P_GiveAmmo(player, weaponinfo[weapon].ammo, deathmatch ? 5 : 2);

      // LP: not checking if P_ShouldKeepWeapon in netgames on purpose
      player->pendingweapon = weapon;
      S_StartSound(player->mo, sfx_wpnup); // killough 4/25/98, 12/98
      return false;
    }

  // give one clip with a dropped weapon, two clips with a found weapon
  gaveammo = weaponinfo[weapon].ammo != am_noammo &&
    P_GiveAmmo(player, weaponinfo[weapon].ammo, dropped ? 1 : 2);

  if(!player->weaponowned[weapon])
    {
      player->weaponowned[weapon] = true;
      if(!P_ShouldKeepWeapon(player)) {
        player->pendingweapon = weapon;
      }
      return true;
    }

  return gaveammo;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//

boolean P_GiveBody(player_t *player, int num)
{
  if (player->health >= maxhealth)
    return false; // Ty 03/09/98 externalized MAXHEALTH to maxhealth
  player->health += num;
  if (player->health > maxhealth)
    player->health = maxhealth;
  player->mo->health = player->health;
  return true;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//

boolean P_GiveArmor(player_t *player, int armortype)
{
  int hits = armortype*100;
  if (player->armorpoints >= hits && !wolf3dmode)
    return false;   // don't pick up
  player->armortype = armortype;
  if(wolf3dmode) player->armorpoints += hits;
  else player->armorpoints = hits;
  return true;
}

//
// P_GiveCard
//

void P_GiveCard(player_t *player, card_t card)
{
  if (player->cards[card])
    return;
  player->bonuscount = BONUSADD;
  player->cards[card] = 1;
}

//
// P_GivePower
//
// Rewritten by Lee Killough
//

boolean P_GivePower(player_t *player, int power)
{
  static const int tics[NUMPOWERS] = {
    INVULNTICS, 1 /* strength */, INVISTICS,
    IRONTICS, 1 /* allmap */, INFRATICS,
    INVISTICS, /* haleyjd: totalinvis */
   };

  switch (power)
    {
      case pw_invisibility:
        player->mo->flags |= MF_SHADOW;
        break;
      case pw_allmap:
        if (player->powers[pw_allmap])
          return false;
        break;
      case pw_strength:
        P_GiveBody(player,100);
        break;
      case pw_totalinvis:   // haleyjd: total invisibility
        player->mo->flags2 |= MF2_DONTDRAW;
        break;
    }

  // Unless player has infinite duration cheat, set duration (killough)

  if (player->powers[power] >= 0)
    player->powers[power] = tics[power];
  return true;
}

//
// P_TouchSpecialThing
//

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher)
{
  player_t *player;
  int      i;
  int      sound;
  char*    message = NULL;
  boolean  removeobj = true;
  fixed_t  delta = special->z - toucher->z;

  if (delta > toucher->height || delta < -8*FRACUNIT)
    return;        // out of reach

  sound = sfx_itemup;
  player = toucher->player;

  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (toucher->health <= 0)
    return;

    // Identify by sprite.
  switch (special->sprite)
    {
      // armor
    case SPR_ARM1:
      if (!P_GiveArmor (player, green_armor_class))
        return;
      message = s_GOTARMOR; // Ty 03/22/98 - externalized
      break;

    case SPR_ARM2:
      if (!P_GiveArmor (player, blue_armor_class))
        return;
      message = s_GOTMEGA; // Ty 03/22/98 - externalized
      break;

      // bonus items
    case SPR_BON1:

        // sf: removed beta
      player->health++;               // can go over 100%
      if (player->health > (maxhealth * 2))
        player->health = (maxhealth * 2);
      player->mo->health = player->health;
      message = s_GOTHTHBONUS; // Ty 03/22/98 - externalized
      break;

    case SPR_BON2:

        // sf: remove beta

      player->armorpoints++;          // can go over 100%
      if (player->armorpoints > max_armor && !wolf3dmode && !v12_compat)
        player->armorpoints = max_armor;
      if (!player->armortype)
        player->armortype = green_armor_class;
      message = s_GOTARMBONUS; // Ty 03/22/98 - externalized
      break;

        // sf: removed beta items

    case SPR_SOUL:
      player->health += soul_health;
      if (player->health > max_soul)
        player->health = max_soul;
      player->mo->health = player->health;
      message = s_GOTSUPER; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_MEGA:
      if (gamemode != commercial)
        return;
      player->health = mega_health;
      player->mo->health = player->health;
      P_GiveArmor (player,blue_armor_class);
      message = s_GOTMSPHERE; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

        // cards
        // leave cards for everyone
    case SPR_BKEY:
      if (!player->cards[it_bluecard])
        message = s_GOTBLUECARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_bluecard);
      removeobj = !netgame;
      break;

    case SPR_YKEY:
      if (!player->cards[it_yellowcard])
        message = s_GOTYELWCARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_yellowcard);
      removeobj = !netgame;
      break;

    case SPR_RKEY:
      if (!player->cards[it_redcard])
        message = s_GOTREDCARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_redcard);
      removeobj = !netgame;
      break;

    case SPR_BSKU:
      if (!player->cards[it_blueskull])
        message = s_GOTBLUESKUL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_blueskull);
      removeobj = !netgame;
      break;

    case SPR_YSKU:
      if (!player->cards[it_yellowskull])
        message = s_GOTYELWSKUL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_yellowskull);
      removeobj = !netgame;
      break;

    case SPR_RSKU:
      if (!player->cards[it_redskull])
        message = s_GOTREDSKULL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_redskull);
      removeobj = !netgame;
      break;

      // medikits, heals
    case SPR_STIM:
      if (!P_GiveBody (player, 10))
        return;
      message = s_GOTSTIM; // Ty 03/22/98 - externalized
      break;

    case SPR_MEDI:
      if (!P_GiveBody (player, 25))
        return;
                // sf: fix medineed (check for below 25, but medikit gives
                  // 25, so always > 25)
      message = player->health < 50 ?     // was 25
        s_GOTMEDINEED : s_GOTMEDIKIT; // Ty 03/22/98 - externalized

      break;


      // power ups
    case SPR_PINV:
      if (!P_GivePower (player, pw_invulnerability))
        return;
      message = s_GOTINVUL; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PSTR:
      if (!P_GivePower (player, pw_strength))
        return;
      message = s_GOTBERSERK; // Ty 03/22/98 - externalized
      if (!P_ShouldKeepWeapon(player) && player->readyweapon != wp_fist)
        // sf: removed beta
	  player->pendingweapon = wp_fist;
      sound = sfx_getpow;
      break;

    case SPR_PINS:
      if (!P_GivePower (player, pw_invisibility))
        return;
      message = s_GOTINVIS; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_SUIT:
      if (!P_GivePower (player, pw_ironfeet))
        return;

        // sf:removed beta

      message = s_GOTSUIT; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PMAP:
      if (!P_GivePower (player, pw_allmap))
        return;
      message = s_GOTMAP; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PVIS:

      if (!P_GivePower (player, pw_infrared))
        return;

        // sf:removed beta
      sound = sfx_getpow;
      message = s_GOTVISOR; // Ty 03/22/98 - externalized
      break;

      // ammo
    case SPR_CLIP:
      if (special->flags & MF_DROPPED)
        {
          if (!P_GiveAmmo (player,am_clip,0))
            return;
        }
      else
        {
          if (!P_GiveAmmo (player,am_clip,1))
            return;
        }
      message = s_GOTCLIP; // Ty 03/22/98 - externalized
      break;

    case SPR_AMMO:
      if (!P_GiveAmmo (player, am_clip,5))
        return;
      message = s_GOTCLIPBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_ROCK:
      if (!P_GiveAmmo (player, am_misl,1))
        return;
      message = s_GOTROCKET; // Ty 03/22/98 - externalized
      break;

    case SPR_BROK:
      if (!P_GiveAmmo (player, am_misl,5))
        return;
      message = s_GOTROCKBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_CELL:
      if (!P_GiveAmmo (player, am_cell,1))
        return;
      message = s_GOTCELL; // Ty 03/22/98 - externalized
      break;

    case SPR_CELP:
      if (!P_GiveAmmo (player, am_cell,5))
        return;
      message = s_GOTCELLBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_SHEL:
      if (!P_GiveAmmo (player, am_shell,1))
        return;
      message = s_GOTSHELLS; // Ty 03/22/98 - externalized
      break;

    case SPR_SBOX:
      if (!P_GiveAmmo (player, am_shell,5))
        return;
      message = s_GOTSHELLBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_BPAK:
      if (!player->backpack)
        {
          for (i=0 ; i<NUMAMMO-1; i++) // joel - no extra faith w/pak
            player->maxammo[i] *= 2;
          player->backpack = true;
        }
      if(special->flags & MF_DROPPED)
      {
              int i;
              for (i=0 ; i<NUMAMMO-1 ; i++) // joel - as above
              {
                   player->ammo[i] +=special->extradata.backpack->ammo[i];
                   if(player->ammo[i]>player->maxammo[i])
                      player->ammo[i]=player->maxammo[i];
              }
              P_GiveWeapon(player,special->extradata.backpack->weapon,true);
              Z_Free(special->extradata.backpack);
              message = "got player backpack";
      }
      else
      {
              for (i=0 ; i<NUMAMMO ; i++)
                P_GiveAmmo (player, i, 1);
              message = s_GOTBACKPACK; // Ty 03/22/98 - externalized
      }

      break;

        // weapons
    case SPR_BFUG:
      if (!P_GiveWeapon (player, wp_bfg, false) )
        return;
      message = bfgtype==0 ? s_GOTBFG9000       // sf
                      : bfgtype==1 ? "You got the BFG 2704!"
                      : bfgtype==2 ? "You got the BFG 11K!"
                      : "You got _some_ kind of BFG";
      sound = sfx_wpnup;
      break;

    case SPR_MGUN:
      if (!P_GiveWeapon (player, wp_chaingun, special->flags & MF_DROPPED))
        return;
      message = s_GOTCHAINGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_CSAW:
      if (!P_GiveWeapon(player, wp_chainsaw, false))
        return;
      message = s_GOTCHAINSAW; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_LAUN:
      if (!P_GiveWeapon (player, wp_missile, false) )
        return;
      message = s_GOTLAUNCHER; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_PLAS:
      if (!P_GiveWeapon(player, wp_plasma, false))
        return;
      message = s_GOTPLASMA; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_SHOT:
      if (!P_GiveWeapon(player, wp_shotgun, special->flags & MF_DROPPED))
        return;
      message = s_GOTSHOTGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_SGN2:
      if (!P_GiveWeapon(player, wp_supershotgun, special->flags & MF_DROPPED))
        return;
      message = s_GOTSHOTGUN2; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    // start new Eternity power-ups

    case SPR_GRLN:  // haleyjd
      if(!P_GiveWeapon(player, wp_grenade, false))
        return;
      message = "You got an M15 grenade launcher!"; // haleyjd
      sound = sfx_wpnup;
      break;

    case SPR_GRNP:
      if (!P_GiveAmmo(player, am_gren,1))
        return;
      message = "You picked up an M15 grenade";
      break;
      
    case SPR_PNS2:
      if(!P_GivePower(player, pw_totalinvis))
        return;
      message = "Total Invisibility!";
      sound = sfx_getpow;
      break;

    case SPR_KEY1:
      message = "Steel Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY2:
      message = "Cave Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY3:
      message = "Axe Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY4:
      message = "Fire Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY5:
      message = "Emerald Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY6:
      message = "Dungeon Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY7:
      message = "Silver Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY8:
      message = "Rusted Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEY9:
      message = "Horn Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEYA:
      message = "Swamp Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;
    case SPR_KEYB:
      message = "Castle Key";
      sound = sfx_keys2a;
      removeobj = !netgame;
      break;

// joel: Holy Cross
    case SPR_CRSS:
      if (!P_GiveWeapon (player, wp_cross, false) )
        return;
      player->ammo[am_faith] = 100;
      message = s_GOTCROSS; // joel - change for Eternity compatibility
      sound = sfx_getpow; // joel: different pickup sound
      break;

    default:
      // I_Error("P_SpecialThing: Unknown gettable thing");
      return;      // killough 12/98: suppress error message
    }

  // sf: display message using player_printf
  if(message)
    player_printf(player, "%c%s", 128+mess_colour, message);
  if(removeobj)
    P_RemoveMobj (special);

  if (special->flags & MF_COUNTITEM)
    player->itemcount++;
  player->bonuscount += BONUSADD;

  S_StartSound(player->mo, sound);   // killough 4/25/98, 12/98
}

//
// KillMobj
//
// killough 11/98: make static
// sf 9/99: globaled removed for FraggleScript

void P_KillMobj(mobj_t *source, mobj_t *target)
{
  mobjtype_t item;
  mobj_t     *mo;
  boolean    removeItemFlag = false; // haleyjd

  if(target->flags & MF_CORPSE) return; // already dead

  target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);
  target->flags2 &= ~MF2_INVULNERABLE; // haleyjd 04/09/99

  if (target->type != MT_SKULL)
    target->flags &= ~MF_NOGRAVITY;

  target->flags |= MF_CORPSE|MF_DROPOFF;
  target->height >>= 2;

  // killough 8/29/98: remove from threaded list
  P_UpdateThinker(&target->thinker);

  if (source && source->player)
    {
      // count for intermission
      // killough 7/20/98: don't count friends
      if (!(target->flags & MF_FRIEND))
	if (target->flags & MF_COUNTKILL)
	  source->player->killcount++;
      if (target->player)
      {
        source->player->frags[target->player-players]++;
        HU_FragsUpdate();
      }
    }
    else
      if (!netgame && (target->flags & MF_COUNTKILL) )
        {
          // count all monster deaths,
          // even those caused by other monsters
	  // killough 7/20/98: don't count friends
	  if (!(target->flags & MF_FRIEND))
	    players->killcount++;
        }

  if (target->player)
    {
      // count environment kills against you
      if (!source)
      {
        target->player->frags[target->player-players]++;
        HU_FragsUpdate();
      }

      target->flags &= ~MF_SOLID;
      target->player->playerstate = PST_DEAD;
      P_DropWeapon (target->player);

      if (target->player == &players[consoleplayer] && automapactive)
	if (!demoplayback) // killough 11/98: don't switch out in demos, though
	  AM_Stop();    // don't die in auto map; switch view prior to dying
    }

  if (target->health < -target->info->spawnhealth && target->info->xdeathstate)
    P_SetMobjState (target, target->info->xdeathstate);
  else
    P_SetMobjState (target, target->info->deathstate);

  target->tics -= P_Random(pr_killtics)&3;

  if (target->tics < 1)
    target->tics = 1;

  if (gamemission == chex) return;

  // Drop stuff.
  // This determines the kind of object spawned
  // during the death frame of a thing.

  switch (target->type)
    {
    case MT_PLAYER:
      if(deathmatch != 3) return;       // backpack only dropped in dm3
      item = MT_MISC24; // backpack
      break;

    case MT_WOLFSS:
    case MT_POSSESSED:
      item = MT_CLIP;
      break;

    case MT_SHOTGUY:
      item = MT_SHOTGUN;
      break;

    case MT_CHAINGUY:
      item = MT_CHAINGUN;
      break;
    
      // haleyjd 01/09/02: moved here from A_Fall
    case MT_MINOTAUR:
       item = MT_MEGA;
       removeItemFlag = true;
       break;

    case MT_CPOSSESSED: // joel: New bad guys drop other bad guys
    case MT_CSHOTGUY:
    case MT_CCHAINGUY:
      item = MT_SKULL;
      break;

    case MT_CSPIDER:
      item = MT_BABY;
      break;

    case HardAssZombieman: // joel: tough zombieman drops rocket
      item = MT_MISC18;
      break;

    default:
      return;
    }

  mo = P_SpawnMobj (target->x, target->y, ONFLOORZ, item);
  mo->flags |= MF_DROPPED;    // special versions of items

  if(removeItemFlag)
     mo->flags &= ~MF_COUNTITEM;

  if(mo->type == MT_MISC24)       // put all the players stuff into the
  {                             // backpack
       int a;
       mo->extradata.backpack = Z_Malloc(sizeof(backpack_t), PU_LEVEL, NULL);
       for(a=0; a<NUMAMMO; a++)
             mo->extradata.backpack->ammo[a] = target->player->ammo[a];
       mo->extradata.backpack->weapon = target->player->readyweapon;
            // set the backpack moving slightly faster than the player

                // start it moving in a (fairly) random direction
                // i cant be bothered to create a new random number
                // class right now
       // haleyjd: not demo safe, fix later (TODO)
       /*
       mo->momx = target->momx * (gametic-basetic) % 5;
       mo->momy = target->momy * (gametic-basetic+30) % 5;
       */
  }
}

        //
        // DEATH MESSAGES
        //
        // the %c at the beginning of each string is used to change the
        // colour to the obituary colour
        //
        // haleyjd: need to enforce NUMWEAPONS here

char *deathmess1[NUMWEAPONS+1] = {
        "%c%s was punched to death",           // fist
        "%c%s died from pistol wounds",        // pistol
        "%c%s got shotgun-blasted",            // shotgun
        "%c%s got chaingunned to death",       // chaingun
        "%c%s failed to avoid the rocket",      // rockets
        "%c%s admires the pretty blue stuff..",// plasma
        "%c%s saw the green flash",           // bfg
        "%c%s got chopped up",                 // chainsaw
        "%c%s got two shells in the chest",    // 2x shotgun
        "%c%s tripped on a grenade",           // grenade launcher
        "%c%s died",                           // default
};

char *deathmess2[NUMWEAPONS+1] = {
        "%c%s performed G.B.H. on %s",         // fist
        "%c%s took out %s with a pistol",      // pistol
        "%c%s blasted %s away",                // shotgun
        "%c%s chaingunned down %s",            // chaingun
        "%c%s blew %s away",                   // rockets
        "%c%s burned %s",                      // plasma
        "%c%s bfg'ed %s",                      // bfg
        "%c%s mistook %s for a tree",          // chainsaw
        "%c%s shows %s his double barrels",    // 2x shotgun
        "%c%s caught %s's grenade",            // grenade launcher
        "%c%s killed %s",                      // default
};

// haleyjd: enum for accessing monster death msgs
enum {
  MSG_POSSESSED,
  MSG_SHOTGUY,
  MSG_VILE,
  MSG_UNDEAD,
  MSG_FATSO,
  MSG_CHAINGUY,
  MSG_TROOP,
  MSG_SERGEANT,
  MSG_SHADOWS,
  MSG_HEAD,
  MSG_BRUISER,
  MSG_KNIGHT,
  MSG_SKULL,
  MSG_SPIDER,
  MSG_BABY,
  MSG_CYBORG,
  MSG_WOLFSS,
#ifdef DOGS
  MSG_DOGS,
#endif
  MSG_MINOTAUR,
  MSG_CLERIC,
  MSG_LEADERCLERIC,
  MSG_DWARF,
  MSG_ALTERDWARF,
  MSG_NEWSPIDER,
  MSG_MUMMY,
  MSG_DEFAULT,
  NUM_MONSTEROBITUARIES
};

// haleyjd: monster death messages and default message
char *mndeathmsgs[NUM_MONSTEROBITUARIES] =
{
        "%c%s was capped by a zombie",                  // trooper
        "%c%s was blasted by a former sergeant",        // sergeant
        "%c%s was fried by an archvile",                // archvile
        "%c%s got sucker-punched by a revenant",        // revenant
        "%c%s was flattened by a mancubus",             // mancubus
        "%c%s felt the heat from a chaingunner",        // chaingun guy
        "%c%s was torn apart by an imp",                // imp
        "%c%s was chewed on by a demon",                // demon
        "%c%s didn't see the spectre",                  // spectre
        "%c%s was roasted by a cacodemon",              // cacodemon
        "%c%s was ripped open by a baron",              // baron
        "%c%s bows before a hell knight",               // hellknight
        "%c%s was spooked by a lost soul",              // lost soul
        "%c%s stood in awe of the spiderdemon",         // spiderdemon
        "%c%s couldn't hide from an arachnotron",       // arachnotron
        "%c%s gulped down a cyberdemon's rocket",       // cyberdemon
        "%c%s was no match for the past",         	// wolf ss
#ifdef DOGS
        "%c%s was chased down by a rabid puppy",        // dog
#endif
        "%c%s felt the fury of Minotaurus",             // minotaur
        "%c%s was vanquished by a cleric",              // cleric
        "%c%s was banished by laekmir",                 // leader cleric
        "%c%s got in halif's way",                      // dwarf
        "%c%s couldn't escape the evil dwarf",          // alterdwarf
        "%c%s was gunned down by an arachnophyte",      // arachnophyte
        "%c%s was petrified by a golem",                // golem
        "%c%s died",                                    // default
};

//sf: obituaries
void P_DeathMessage(mobj_t *source, mobj_t *target, mobj_t *inflictor)
{
        int killweapon, messtype;

        if(!target->player) return;     // not a player
        if(!obituaries) return;         // obituaries off

        // haleyjd: compiler warning fix
        killweapon = NUMWEAPONS;

	// haleyjd 01/09/02: moved this up for safety/clarity
	if(!source) // killed by environment
	{
	   doom_printf("%c%s died", 128+obcolour, target->player->name);
	   return;
	}

        if(!source->player) // killed by a monster
        {
	   char *msg = NULL;
	   
	   switch(source->type)
	   {
	   case MT_POSSESSED: msg = mndeathmsgs[MSG_POSSESSED]; break;
	   case MT_SHOTGUY:   msg = mndeathmsgs[MSG_SHOTGUY]; break;
	   case MT_VILE:      msg = mndeathmsgs[MSG_VILE]; break;
	   case MT_UNDEAD:    msg = mndeathmsgs[MSG_UNDEAD]; break;
	   case MT_FATSO:     msg = mndeathmsgs[MSG_FATSO]; break;
	   case MT_CHAINGUY:  msg = mndeathmsgs[MSG_CHAINGUY]; break;
	   case MT_TROOP:     msg = mndeathmsgs[MSG_TROOP]; break;
	   case MT_SERGEANT:  msg = mndeathmsgs[MSG_SERGEANT]; break;
	   case MT_SHADOWS:   msg = mndeathmsgs[MSG_SHADOWS]; break;
	   case MT_HEAD:      msg = mndeathmsgs[MSG_HEAD]; break;
	   case MT_BRUISER:   msg = mndeathmsgs[MSG_BRUISER]; break;
	   case MT_KNIGHT:    msg = mndeathmsgs[MSG_KNIGHT]; break;
	   case MT_SKULL:     msg = mndeathmsgs[MSG_SKULL]; break;
	   case MT_SPIDER:    msg = mndeathmsgs[MSG_SPIDER]; break;
	   case MT_BABY:      msg = mndeathmsgs[MSG_BABY]; break;
	   case MT_CYBORG:
	   case MT_CYBERGUARD2: msg = mndeathmsgs[MSG_CYBORG]; break;
	   case MT_WOLFSS:    msg = mndeathmsgs[MSG_WOLFSS]; break;
#ifdef DOGS
	   case MT_DOGS:      msg = mndeathmsgs[MSG_DOGS]; break;
#endif
	   case MT_MINOTAUR:  msg = mndeathmsgs[MSG_MINOTAUR]; break;
	   case MT_CLERIC:    msg = mndeathmsgs[MSG_CLERIC]; break;
	   case MT_LEADERCLERIC: msg = mndeathmsgs[MSG_LEADERCLERIC]; break;
	   case MT_DWARF:     msg = mndeathmsgs[MSG_DWARF]; break;
	   case MT_ALTERDWARF: msg = mndeathmsgs[MSG_ALTERDWARF]; break;
	   case MT_NEWSPIDER: msg = mndeathmsgs[MSG_NEWSPIDER]; break;
	   case MT_MUMMY:     msg = mndeathmsgs[MSG_MUMMY]; break;
	   default:           msg = mndeathmsgs[MSG_DEFAULT];
	   } // end switch
	   
	   doom_printf(msg, 128+obcolour, target->player->name);
	   return;
        }

        if(source == inflictor)         // killed by shooting etc.
                killweapon = source->player->readyweapon;
        else
                    // find what weapon caused the kill
        {
           if(inflictor) // haleyjd: fix possible page fault!
           {
                switch(inflictor->type)
                {
                        case MT_BFG: killweapon = wp_bfg; break;
                        case MT_ROCKET: killweapon = wp_missile; break;
                        case MT_PLASMA: killweapon = wp_plasma; break;
                        case MT_GRENADE: killweapon = wp_grenade; break;
                        default: killweapon = NUMWEAPONS; break;
                }
           }
        }

        if(source->player == target->player)    // suicide ?
        {
                // inflictor: only use message if an inflictor is used
            if(inflictor)
	    {
                if(killweapon == wp_missile)
                {
                   doom_printf("%c%s should have stood back", 128+obcolour,
                          source->player->name);
                   return;
                }
                else if(killweapon == wp_bfg)
                {
                   doom_printf("%c%s used a bfg close-up",
                        128+obcolour, source->player->name);
                   return;
                }
                else if(killweapon == wp_grenade)
                {
                   doom_printf("%c%s lost track of a grenade",
                        128+obcolour, source->player->name);
                   return;
                }
	    }

            doom_printf("%c%s suicides", 128+obcolour,
                        source->player->name);
            return;
        }

        //  choose a message type, 0 or 1
        messtype = M_Random() % 2;

        if(messtype)
	{
            doom_printf(deathmess1[killweapon], 128+obcolour,
                        target->player->name, source->player->name);
	}
        else
	{
            doom_printf(deathmess2[killweapon], 128+obcolour,
                        source->player->name, target->player->name);
	}
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//

void P_DamageMobj(mobj_t *target,mobj_t *inflictor, mobj_t *source, int damage)
{
  player_t *player;
  boolean justhit;          // killough 11/98

  // killough 8/31/98: allow bouncers to take damage
  if (!(target->flags & (MF_SHOOTABLE | MF_BOUNCES)))
    return; // shouldn't happen...

  if (target->health <= 0)
    return;

  // haleyjd: 
  // Invulnerability -- telestomp can still kill to avoid getting stuck
  // Dormancy -- things are invulnerable until they are awakened
  if((target->flags2 & MF2_INVULNERABLE ||
      target->flags2 & MF2_DORMANT) && damage < 10000)
    return;

  if(target->flags2 & MF2_DORMANT)
  {
     target->flags2 &= ~MF2_DORMANT;
     target->tics = 1;
  }

  if (target->flags & MF_SKULLFLY)
  {
    if(target->type == MT_MINOTAUR)
      return;  // haleyjd: Minotaur invincible during charge
    target->momx = target->momy = target->momz = 0;
  }

  player = target->player;
  if (player && gameskill == sk_baby)
    damage >>= 1;   // take half damage in trainer mode

  if(inflictor)
  {
     // special damage types -- haleyjd 03/27/99
     switch(inflictor->type)
     {
        case MT_MINOTAUR:
          if(inflictor->flags & MF_SKULLFLY) // for charge attack
          {
             angle_t tangle = R_PointToAngle2(source->x, source->y, target->x, target->y);
             fixed_t tthrust = 16*FRACUNIT;
             tangle >>= ANGLETOFINESHIFT;

             target->momx += FixedMul(tthrust, finecosine[tangle]);
             target->momy += FixedMul(tthrust, finesine[tangle]);
             if(target->player)
             {
                target->reactiontime = 14;
             }
          }
	  break;
        default:
           break;
     }
  }

  // Some close combat weapons should not
  // inflict thrust and push the victim out of reach,
  // thus kick away unless using the chainsaw.

  if (inflictor && !(target->flags & MF_NOCLIP) &&
      (!source || !source->player ||
       source->player->readyweapon != wp_chainsaw))
    {
         unsigned ang = R_PointToAngle2 (inflictor->x, inflictor->y,
                                      target->x, target->y);

         fixed_t thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

        // make fall forwards sometimes
         if ( damage < 40 && damage > target->health
           && target->z - inflictor->z > 64*FRACUNIT
           && P_Random(pr_damagemobj) & 1)
         {
            ang += ANG180;
            thrust *= 4;
         }

         ang >>= ANGLETOFINESHIFT;
         target->momx += FixedMul (thrust, finecosine[ang]);
         target->momy += FixedMul (thrust, finesine[ang]);

      // killough 11/98: thrust objects hanging off ledges
      if (target->intflags & MIF_FALLING && target->gear >= MAXGEAR)
	target->gear = 0;
    }

  // player specific
  if (player)
    {
      // end of game hell hack
      if (target->subsector->sector->special == 11 && damage >= target->health)
        damage = target->health - 1;

      // Below certain threshold,
      // ignore damage in GOD mode, or with INVUL power.
      // killough 3/26/98: make god mode 100% god mode in non-compat mode

      if ((damage < 1000 || (!comp[comp_god] && player->cheats&CF_GODMODE)) &&
          (player->cheats&CF_GODMODE || player->powers[pw_invulnerability]))
        return;

      if (player->armortype && !wolf3dmode)
        {
          int saved = player->armortype == 1 ? damage/3 : damage/2;
          if (player->armorpoints <= saved)
            {
              // armor is used up
              saved = player->armorpoints;
              player->armortype = 0;
            }
          player->armorpoints -= saved;
          damage -= saved;
        }

      player->health -= damage;       // mirror mobj health here for Dave
      if (player->health < 0)
        player->health = 0;

      player->attacker = source;
      player->damagecount += damage;  // add damage after armor / invuln

      if (player->damagecount > 100)
        player->damagecount = 100;  // teleport stomp does 10k points...

#if 0
      // killough 11/98: 
      // This is unused -- perhaps it was designed for
      // a hand-connected input device or VR helmet,
      // to pinch the player when they're hurt :)

      {
	int temp = damage < 100 ? damage : 100;
	
	if (player == &players[consoleplayer])
	  I_Tactile (40,10,40+temp*2);
      }
#endif

    }


  // do the damage
  if ((target->health -= damage) <= 0)
    {
      if(target->player)        // death messages for players
              P_DeathMessage(source, target, inflictor);
      P_KillMobj(source, target);
      return;
    }

  // haleyjd: special falling death hack
  //          if we got here, we didn't really die
  if(target->intflags & MIF_DIEDFALLING)
    target->intflags &= ~MIF_DIEDFALLING;

  // killough 9/7/98: keep track of targets so that friends can help friends
  if (demo_version >= 203)
    {
      // If target is a player, set player's target to source,
      // so that a friend can tell who's hurting a player
      if (player)
	P_SetTarget(&target->target, source);
      
      // killough 9/8/98:
      // If target's health is less than 50%, move it to the front of its list.
      // This will slightly increase the chances that enemies will choose to
      // "finish it off", but its main purpose is to alert friends of danger.
      if (target->health*2 < target->info->spawnhealth)
	{
	  thinker_t *cap = &thinkerclasscap[target->flags & MF_FRIEND ? 
					   th_friends : th_enemies];
	  (target->thinker.cprev->cnext = target->thinker.cnext)->cprev =
	    target->thinker.cprev;
	  (target->thinker.cnext = cap->cnext)->cprev = &target->thinker;
	  (target->thinker.cprev = cap)->cnext = &target->thinker;
	}
    }

  if ((justhit = (P_Random (pr_painchance) < target->info->painchance &&
		  !(target->flags & MF_SKULLFLY)))) //killough 11/98: see below
  { // haleyjd 10/06/99: remove pain for zero-damage projectiles

     if(!(demo_version >= 329 && damage <= 0))
       P_SetMobjState(target, target->info->painstate);
  }
  target->reactiontime = 0;           // we're awake now...

  // killough 9/9/98: cleaned up, made more consistent:

  if (source && (source != target || v12_compat) &&
      source->type != MT_VILE &&
      (!target->threshold || target->type == MT_VILE) &&
      ((source->flags ^ target->flags) & MF_FRIEND || 
       (monster_infighting && !wolf3dmode) || demo_version < 203))
    {
      // if not intent on another player, chase after this one
      //
      // killough 2/15/98: remember last enemy, to prevent
      // sleeping early; 2/21/98: Place priority on players
      // killough 9/9/98: cleaned up, made more consistent:

      if (!target->lastenemy || target->lastenemy->health <= 0 ||
	  (demo_version < 203 ? !target->lastenemy->player :
	   !((target->flags ^ target->lastenemy->flags) & MF_FRIEND) &&
	   target->target != source)) // remember last enemy - killough
	P_SetTarget(&target->lastenemy, target->target);

      P_SetTarget(&target->target, source);       // killough 11/98
      target->threshold = BASETHRESHOLD;
      if (target->state == &states[target->info->spawnstate]
          && target->info->seestate != S_NULL)
        P_SetMobjState (target, target->info->seestate);
    }

  // killough 11/98: Don't attack a friend, unless hit by that friend.
  if (justhit && (target->target == source || !target->target ||
		  !(target->flags & target->target->flags & MF_FRIEND)))
    target->flags |= MF_JUSTHIT;    // fight back!
}

//----------------------------------------------------------------------------
//
// $Log: p_inter.c,v $
// Revision 1.10  1998/05/03  23:09:29  killough
// beautification, fix #includes, move some global vars here
//
// Revision 1.9  1998/04/27  01:54:43  killough
// Prevent pickup sounds from silencing player weapons
//
// Revision 1.8  1998/03/28  17:58:27  killough
// Fix spawn telefrag bug
//
// Revision 1.7  1998/03/28  05:32:41  jim
// Text enabling changes for DEH
//
// Revision 1.6  1998/03/23  03:25:44  killough
// Fix weapon pickup sounds in spy mode
//
// Revision 1.5  1998/03/10  07:15:10  jim
// Initial DEH support added, minus text
//
// Revision 1.4  1998/02/23  04:44:33  killough
// Make monsters smarter
//
// Revision 1.3  1998/02/17  06:00:54  killough
// Save last enemy, change RNG calling sequence
//
// Revision 1.2  1998/01/26  19:24:05  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:59  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
