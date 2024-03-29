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
// DESCRIPTION:  Platform-independent sound code
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: s_sound.c,v 1.11 1998/05/03 22:57:06 killough Exp $";

// killough 3/7/98: modified to allow arbitrary listeners in spy mode
// killough 5/2/98: reindented, removed useless code, beautified

#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "i_sound.h"
#include "r_main.h"
#include "m_random.h"
#include "w_wad.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "p_info.h"
#include "d_io.h" // SoM 3/14/2002: strncasecmp

#include "time.h"

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1200<<FRACBITS)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
//
// killough 12/98: restore original
// #define S_CLOSE_DIST (160<<FRACBITS)

#define S_CLOSE_DIST (200<<FRACBITS)

#define S_ATTENUATOR ((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128
#define S_STEREO_SWING (96<<FRACBITS)

// sf: sound/music hashing
#define SOUND_HASHSLOTS 17
// use sound_hash for music hash too
#define sound_hash(s)                             \
         ( ( tolower((s)[0]) + (s)[0] ?           \
             tolower((s)[1]) + (s)[1] ?           \
             tolower((s)[2]) + (s)[2] ?           \
             tolower((s)[3]) + (s)[3] ?           \
             tolower((s)[4]) : 0 : 0 : 0 : 0 ) % SOUND_HASHSLOTS )

static void S_CreateSoundHashTable();


//jff 1/22/98 make sound enabling variables readable here
extern int snd_card, mus_card;
extern boolean nosfxparm, nomusicparm;
//jff end sound enabling variables readable here

int loop_music = 1;

typedef struct
{
  sfxinfo_t *sfxinfo;  // sound information (if null, channel avail.)
  const mobj_t *origin;// origin of sound
  int handle;          // handle of the sound being played
  int pitch;
} channel_t;

// the set of channels available
static channel_t *channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int snd_SfxVolume = 15;

// Maximum volume of music. Useless so far.
int snd_MusicVolume = 15;

// precache sounds ?
int s_precache = 1;

// whether songs are mus_paused
static boolean mus_paused;

// music currently being played
static musicinfo_t *mus_playing;

// following is set
//  by the defaults code in M_misc:
// number of channels available
int numChannels;
int default_numChannels;  // killough 9/98

//jff 3/17/98 to keep track of last IDMUS specified music num
int idmusnum;

        // sf:
sfxinfo_t *sfxinfos[SOUND_HASHSLOTS];
musicinfo_t *musicinfos[SOUND_HASHSLOTS];

static boolean title_mus_tampered = false;
static boolean finale_mus_tampered = false;

//
// Internals.
//

static void S_StopChannel(int cnum)
{
  if (channels[cnum].sfxinfo)
    {
      if (I_SoundIsPlaying(channels[cnum].handle))
	I_StopSound(channels[cnum].handle);      // stop the sound playing
      channels[cnum].sfxinfo = 0;
    }
}

        //sf: listener now a camera_t for external cameras
static int S_AdjustSoundParams(camera_t *listener, const mobj_t *source,
				int *vol, int *sep, int *pitch)
{
  fixed_t adx, ady, dist;
  angle_t angle;

  // calculate the distance to sound origin
  //  and clip it if necessary
  //
  // killough 11/98: scale coordinates down before calculations start
  // killough 12/98: use exact distance formula instead of approximation

  adx = abs((listener->x >> FRACBITS) - (source->x >> FRACBITS));
  ady = abs((listener->y >> FRACBITS) - (source->y >> FRACBITS));

  if (ady > adx)
    dist = adx, adx = ady, ady = dist;

  dist = adx ? FixedDiv(adx, finesine[(tantoangle[FixedDiv(ady,adx) >> DBITS]
				       + ANG90) >> ANGLETOFINESHIFT]) : 0;

  if (source)
    {
      // source in a killed-sound sector?
      if (R_PointInSubsector(source->x, source->y)->sector->special
	  & SF_KILLSOUND)
	return 0;
    }
  else
    // are _we_ in a killed-sound sector?
    if(gamestate == GS_LEVEL &&
       R_PointInSubsector(listener->x, listener->y)
       ->sector->special & SF_KILLSOUND)
      return 0;

  if (!dist)  // killough 11/98: handle zero-distance as special case
    {
      *sep = NORM_SEP;
      *vol = snd_SfxVolume;
      return *vol > 0;
    }

  if (dist > S_CLIPPING_DIST >> FRACBITS)
    return 0;

  // angle of source to listener
  // sf: use listenx, listeny

  // volume calculation
  *vol = dist < S_CLOSE_DIST >> FRACBITS ? snd_SfxVolume :
    snd_SfxVolume * ((S_CLIPPING_DIST>>FRACBITS)-dist) /
    S_ATTENUATOR;

  angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

  angle -= viewangle;
  if(angle > ANG90 && angle < ANG270)
    {
       *vol -= *vol >> 1;
    }

  angle >>= 24;
  *sep = angle*2-128;
  if(*sep < 64)
    {
       *sep = -*sep;
    }
  if(*sep > 192) 
    {
      *sep = 512-*sep;
    }

  return *vol > 0;
}

//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//

static int S_getChannel(const void *origin, sfxinfo_t *sfxinfo)
{
  // channel number to use
  int cnum;
  channel_t *c;

  // Find an open channel
  // killough 12/98: replace is_pickup hack with singularity flag
  for (cnum=0; cnum<numChannels && channels[cnum].sfxinfo; cnum++)
    if (origin && channels[cnum].origin == origin &&
        channels[cnum].sfxinfo->singularity == sfxinfo->singularity)
      {
        S_StopChannel(cnum);
        break;
      }

    // None available
  if (cnum == numChannels)
    {      // Look for lower priority
      for (cnum=0 ; cnum<numChannels ; cnum++)
        if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
          break;
      if (cnum == numChannels)
        return -1;                  // No lower priority.  Sorry, Charlie.
      else
        S_StopChannel(cnum);        // Otherwise, kick out lower priority.
    }

  c = &channels[cnum];              // channel is decided to be cnum.
  c->sfxinfo = sfxinfo;
  c->origin = origin;
  return cnum;
}

void S_StartSfxInfo(const mobj_t *origin, sfxinfo_t *sfx)
{
  int sep, pitch, priority, cnum;
  int volume = snd_SfxVolume;
  int sfx_id;

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  // haleyjd:  we must weed out degenmobj_t's before trying to dereference
  // these fields -- a thinker check perhaps?

  // haleyjd: part deux:  only while playing a level!  crashes ending

  if (sfx->skinsound && gamestate == GS_LEVEL) // check for skin sounds
    {
      if(origin && 
         (origin->thinker.function == P_MobjThinker) &&  // haleyjd
         origin->skin && origin->skin->sounds[sfx->skinsound-1])
        sfx = S_SfxInfoForName(origin->skin->sounds[sfx->skinsound-1]);
    }

  sfx_id = sfx - S_sfx;

#ifdef RANGECHECK
  // haleyjd 06/11/01: can't do this any more due to hashing
  // check for bogus sound #
  //if (sfx_id < 1 || sfx_id > NUMSFX)
  //  I_Error("Bad sfx #: %d", sfx_id);
#endif

  // Initialize sound parameters
  if (sfx->link)
    {
      pitch = sfx->pitch;
      priority = sfx->priority;
      volume += sfx->volume;

      if (volume < 1)
        return;

      if (volume >= snd_SfxVolume)
        volume = snd_SfxVolume;
    }
  else
    {
      pitch = NORM_PITCH;
      priority = NORM_PRIORITY;
    }

  // Check to see if it is audible, modify the params
  // killough 3/7/98, 4/25/98: code rearranged slightly

  if (pitched_sounds)
    {
      // hacks to vary the sfx pitches
      if ((sfx_id) >= sfx_sawup && (sfx_id) <= sfx_sawhit)
        pitch += 8 - (M_Random()&15);
      else
        if ((sfx_id) != sfx_itemup && (sfx_id) != sfx_tink)
          pitch += 16 - (M_Random()&31);

      if (pitch<0)
        pitch = 0;

      if (pitch>255)
        pitch = 255;
    }

  if (!origin || origin == players[displayplayer].mo)
    sep = NORM_SEP;
  else
    {
      // sf: change to adjustsoundparams means we need to build a quick
      // camera_t. horrible i know
      camera_t player; mobj_t *mo = players[displayplayer].mo;
      player.x = mo->x; player.y = mo->y; player.z = mo->z;
      player.angle = mo->angle;
      
      // use an external cam?
      if (!S_AdjustSoundParams
	  (
	   camera ? camera : &player,
	   origin,
	   &volume,
	   &sep,
	   &pitch
	   )
	  )
	return;
      else
	if (origin->x == players[displayplayer].mo->x &&
	    origin->y == players[displayplayer].mo->y)
	  sep = NORM_SEP;
    }


  // kill old sound
  // killough 12/98: replace is_pickup hack with singularity flag
  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo &&
        channels[cnum].sfxinfo->singularity == sfx->singularity &&
	channels[cnum].origin == origin)
      {
        S_StopChannel(cnum);
        break;
      }

  // try to find a channel
  cnum = S_getChannel(origin, sfx);

  if (cnum<0)
    return;

  while(sfx->link) sfx = sfx->link;     // sf: skip thru link(s)

  // Assigns the handle to one of the channels in the mix/output buffer.
  channels[cnum].handle = I_StartSound(sfx, volume, sep, pitch, priority);
  channels[cnum].pitch = pitch;
}

void S_StartSound(const mobj_t *origin, int sfx_id)
{
  S_StartSfxInfo(origin, &S_sfx[sfx_id]);
}

void S_StartSoundName(const mobj_t *origin, char *name)
{
  sfxinfo_t *sfx;
  
  if(!(sfx = S_SfxInfoForName(name)))
    doom_printf("sound not found: %s\n", name);
  else
    S_StartSfxInfo(origin, sfx);
}

void S_StopSound(const mobj_t *origin)
{
  int cnum;
  
  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
      {
        S_StopChannel(cnum);
        break;
      }
}

static int S_CheckMusicLump(int muisc);
static void S_DoChangeMusic(musicinfo_t *music, int looping);

//
// Stop and resume music, during game PAUSE.
//

void S_PauseSound(void)
{
  if (mus_playing && !mus_paused)
    {
      I_PauseSong(mus_playing->handle);
      mus_paused = true;
    }
}

void S_ResumeSound(void)
{
  if (mus_playing && mus_paused)
    {
      I_ResumeSong(mus_playing->handle);
      mus_paused = false;
    }
}

void S_RestartMusic(void)
{
  S_UpdateMusicLooping(true);
}

static void S_UpdateMusicLooping(boolean force)
{
  if(!title_mus_tampered && 
    (mus_playing == &S_music[mus_dm2ttl] ||
       mus_playing == &S_music[mus_intro] ||
          mus_playing == &S_music[mus_introa]))
    {
      return;
    }

  if(!finale_mus_tampered &&
    mus_playing == &S_music[mus_bunny])
    {
      return;
    }

  if(!force && !mus_playing)
    {
      return;
    }

  if(force)              
    S_ResumeSound();

  if ((!mus_paused && !I_QrySongPlaying(0)) || force)
    {      
      if((!force && !loop_music) || (force && !mus_playing))
        {
          if(randomize_music == randm_always)
            S_ChangeToRandomMusic();
          else
            S_ChangeToNextMusic(true);

          if(mus_playing)
            C_Printf("Now playing %c%.6s\n", 128 + CR_GOLD, mus_playing->name);      
        }
      else
        {
          S_DoChangeMusic(mus_playing, loop_music);
        }
    }
}

//
// Updates music & sounds
//

static int mustics = 0;

void S_UpdateSounds(const mobj_t *listener)
{
  int cnum;
  camera_t player;      // sf: a camera_t holding the information about
                        // the player

  if (mus_card && !nomusicparm)
    {
      if(mustics++ > 35)
        {
          mustics = 0;
          S_UpdateMusicLooping(false);
        }
    }

  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

  if(listener)
    {            // fill in player first
      player.x = listener->x;  player.y = listener->y;
      player.z = listener->z;
      player.angle = listener->angle;
    }

                // now update each individual channel
  for (cnum=0 ; cnum<numChannels ; cnum++)
    {
      channel_t *c = &channels[cnum];
      sfxinfo_t *sfx = c->sfxinfo;

      if (sfx)
        {
          if (I_SoundIsPlaying(c->handle))
            {
              // initialize parameters
              int volume = snd_SfxVolume;
              int pitch = NORM_PITCH;
              int sep = NORM_SEP;

	      /*
              if (sfx->link)
                {
                  pitch = sfx->pitch;
                  volume += sfx->volume;
                  if (volume < 1)
                    {
                      S_StopChannel(cnum);
                      continue;
                    }
                  else
                    if (volume > snd_SfxVolume)
                      volume = snd_SfxVolume;
                }
		*/

              // check non-local sounds for distance clipping
              // or modify their params

	      // sf: removed check for if
	      // listener is source, for silencing sectors
	      // sf again: use external camera if there is one
	      // fix afterglows bug: segv because of NULL listener

              if (c->origin) // killough 3/20/98
                if (!S_AdjustSoundParams
		    (
		     camera ? camera : listener ? &player : NULL,
		     c->origin,
		     &volume,
		     &sep,
		     &pitch
		     )
		    )
                  S_StopChannel(cnum);
                else
                  I_UpdateSoundParams(c->handle, volume, sep, c->pitch);
            }
          else   // if channel is allocated but sound has stopped, free it
            S_StopChannel(cnum);
        }
    }
}

void S_SetMusicVolume(int volume)
{
  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

#ifdef RANGECHECK
  if (volume < 0 || volume > 127)
    I_Error("Attempt to set music volume at %d", volume);
#endif

  I_SetMusicVolume(127);
  I_SetMusicVolume(volume);
  snd_MusicVolume = volume;
}

void S_SetSfxVolume(int volume)
{
  //jff 1/22/98 return if sound is not enabled
  if (!snd_card || nosfxparm)
    return;

#ifdef RANGECHECK
  if (volume < 0 || volume > 127)
    I_Error("Attempt to set sfx volume at %d", volume);
#endif

  snd_SfxVolume = volume;
}

// sf: created changemusicnum, not limited to original musics
// change by music number
// removed mus_new

void S_ChangeMusicNum(int musnum, int looping)
{
  musicinfo_t *music;
  
  if (musnum <= mus_None || musnum >= NUMMUSIC)
    {
      doom_printf("Bad music number %d\n", musnum);
      return;
    }

  music = &S_music[musnum];
    
  S_ChangeMusic(music, looping);
}

int S_NowPlayingLumpNum()
{
  char namebuf[9];
  int lumpnum;

  if(mus_playing)
    {
      sprintf(namebuf, "d_%s", mus_playing->name);
      return W_CheckNumForName(namebuf);
    }
  return NULL;
}

static int seed = -1;

void S_InsertSomeRandomness()
{
  seed = -1;
}

int S_PreselectRandomMusic(boolean no_runnin)
{
  int i;
  static int last = 0;
  int min = mus_None + 1;
  int max = mus_inter - 1;
  if(gamemode == commercial)
    {
      min = mus_runnin;
      max = mus_read_m - 1;
    }
  else if(gamemode == shareware)
    {
      max = mus_e1m9;
    }
  if(no_runnin) min += 1;

  if(seed == -1)
    {
      i = seed = time(NULL) % 16;
      while(i --> 0) P_Random(pr_mustrack);
    }

  i = (P_Random(pr_mustrack) % (max - min + 1)) + min;
  if(mus_playing == &S_music[i] || i == last + min)
    {
      i = (P_Random(pr_mustrack) % (max - min + 1)) + min;
    }
  last = i - min;
  return i;
}

char * S_ChangeToPreselectedMusic(int i)
{
  musicinfo_t * music = NULL;

  music = &S_music[i];
  S_ChangeMusic(music, loop_music);

  if(mus_playing)
    {
      static char name[7];
      name[0] = 0;
      strncpy(name, mus_playing->name, 6);
      idmusnum = i;
      return name;
    }

  return NULL;
}

char * S_ChangeToRandomMusic()
{
  int i = S_PreselectRandomMusic(false);
  musicinfo_t * music = NULL;

  music = &S_music[i];
  S_ChangeMusic(music, loop_music);

  if(mus_playing)
    {
      static char name[7];
      name[0] = 0;
      strncpy(name, mus_playing->name, 6);
      idmusnum = i;
      title_mus_tampered = true;
      finale_mus_tampered = true;
      return name;
    }

  return NULL;
}

char * S_ChangeToNextMusic(boolean next)
{
  int i;
  musicinfo_t * music = NULL;

  for(i = mus_None + 1 ; i < NUMMUSIC ; i += 1)
    {
      if(mus_playing == &S_music[i])
        {
          int min = mus_None + 1;
          int max = mus_runnin - 1;
          int m = i + (next ? 1 : -1);

          if(gamemode == commercial)
            {
              min = mus_runnin;
              max = NUMMUSIC - 1;
            }
          else if(gamemode == shareware)
            {
              max = mus_e1m9;
            }

          if(m > max) m = min;
          else if(m < min) m = max;

          music = &S_music[m];
          i = m;
          break;
        }
    }

  if(!music)
    {
      music = &S_music[(gamemode == commercial) ? mus_runnin : mus_e1m1];
    }

  if(music) S_ChangeMusic(music, loop_music);

  if(mus_playing)
    {
      static char name[7];
      name[0] = 0;
      strncpy(name, mus_playing->name, 6);
      title_mus_tampered = true;
      finale_mus_tampered = true;
      idmusnum = i;
      return name;
    }

  return NULL;
}

// change by name
void S_ChangeMusicName(char *name, int looping)
{
  musicinfo_t *music;

  // haleyjd: special handling for music name "-" to enable explicit
  //          music stopping via FS changemusic function - follows 
  //          precedence of using "-" to indicate no texture on a line
  
  if(!strcmp(name, "-"))
  {
  	S_StopMusic();
  	return;
  }
  
  music = S_MusicForName(name);

  if(music)
    S_ChangeMusic(music, looping);
  else
    {
      doom_printf("music not found: %s\n", name);    
      S_StopMusic(); // stop music anyway
    }
}

static int S_CheckMusicLump(int musnum)
{
  musicinfo_t * music = NULL;
  char namebuf[9];

  if (musnum <= mus_None || musnum >= NUMMUSIC)
    {
      return 0;
    }
  music = &S_music[musnum];

  sprintf(namebuf, "d_%s", music->name);
  return W_CheckNumForName(namebuf) >= 0;
}


static void S_DoChangeMusic(musicinfo_t *music, int looping)
{
  int lumpnum;
  char namebuf[9];

  //LP: failed last time, don't try again
  if(music == mus_playing && mus_playing->handle < 0)
    return;

  sprintf(namebuf, "d_%s", music->name);

  lumpnum = W_CheckNumForName(namebuf);
  if(lumpnum == -1)
    {
      doom_printf("bad music name '%s'\n", music->name);
      return;
    }

  // load & register it
  music->data = W_CacheLumpNum(lumpnum, PU_MUSIC);
  music->handle = I_RegisterSong(music->data);

  // play it
  I_PlaySong(music->handle, looping);

  mus_playing = music;
}

void S_ChangeMusic(musicinfo_t *music, int looping)
{
    //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  // same as the one playing ?

  if(mus_playing == music )
    return;  

  // shutdown old music
  S_StopMusic();

  S_DoChangeMusic(music, looping);
}

void S_StartTitleMusic(int m_id)
{
  if(!title_mus_tampered)
    {
      S_StopMusic();
      if(m_id == mus_intro && I_IsMusicCardOPL())
        {
          if(S_CheckMusicLump(mus_introa))
            m_id = mus_introa;
        }
      S_StartMusic(m_id);
    }
}

void S_ResetTitleMusic()
{
  title_mus_tampered = false;
}

void S_StartFinaleMusic(int mus)
{
  finale_mus_tampered = false;
  S_StartMusic(mus);
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
  S_ChangeMusicNum(m_id, false);
}

void S_StopMusic(void)
{
  if (!mus_playing)
    return;

  if (mus_paused)
    I_ResumeSong(mus_playing->handle);

  I_StopSong(mus_playing->handle);
  I_UnRegisterSong(mus_playing->handle);
  Z_ChangeTag(mus_playing->data, PU_CACHE);

  mus_playing->data = 0;
  mus_playing = 0;
}

void S_StopSounds()
{
  int cnum;
  // kill all playing sounds at start of level
  //  (trust me - a good idea)

  //jff 1/22/98 skip sound init if sound not enabled
  if (snd_card && !nosfxparm)
    for (cnum=0 ; cnum<numChannels ; cnum++)
      if (channels[cnum].sfxinfo)
        S_StopChannel(cnum);
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
  int mnum;

  S_StopSounds();

  //jff 1/22/98 return if music is not enabled
  if (!mus_card || nomusicparm)
    return;

  // start new music for the level
  mus_paused = 0;

  if(!*info_music && gamemap==0)
    {
      // dont know what music to play
      // we need a default
      info_music = gamemode == commercial ? "runnin" : "e1m1";
    }
  
  // sf: replacement music
  if(*info_music)
    S_ChangeMusicName(info_music, loop_music);
  else
    {
      if (idmusnum!=-1)
	mnum = idmusnum; //jff 3/17/98 reload IDMUS music if not -1
      else
	if (gamemode == commercial)
          mnum = mus_runnin + (gamemap - 1) % (mus_ultima - mus_runnin + 1);
	else
	  {
	    static const int spmus[] =     // Song - Who? - Where?
	      {
		mus_e3m4,     // American     e4m1
		mus_e3m2,     // Romero       e4m2
		mus_e3m3,     // Shawn        e4m3
		mus_e1m5,     // American     e4m4
		mus_e2m7,     // Tim  e4m5
		mus_e2m4,     // Romero       e4m6
		mus_e2m6,     // J.Anderson   e4m7 CHIRON.WAD
		mus_e2m5,     // Shawn        e4m8
		mus_e1m9      // Tim          e4m9
	      };
	    
	    // sf: simplified
            mnum = gameepisode < 4 ?
              (mus_e1m1 + (gameepisode - 1)*9 + gamemap-1) :
              gameepisode > 4 ?
              (mus_e1m1 + ((gameepisode - 2)%3)*9 + gamemap-1) :
	      spmus[gamemap-1];
	  }
     
      // start music
      S_ChangeMusicNum(mnum, true);
    }
}

void S_PreCacheAllSounds()
{
 // Nope
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//

void S_Init(int sfxVolume, int musicVolume)
{
  S_CreateSoundHashTable();

  //jff 1/22/98 skip sound init if sound not enabled
  if (snd_card && !nosfxparm)
    {
      usermsg("\tdefault sfx volume %d", sfxVolume);  // killough 8/8/98

      S_SetSfxVolume(sfxVolume);

      // Allocating the internal channels for mixing
      // (the maximum numer of sounds rendered
      // simultaneously) within zone memory.

      // killough 10/98:
      channels = calloc(numChannels = default_numChannels, sizeof(channel_t));
    }

  S_SetMusicVolume(musicVolume);

  if(s_precache)        // sf: option to precache sounds
    {
      S_PreCacheAllSounds();
      usermsg("\tprecached all sounds.");
    }
  else
    usermsg("\tsounds to be cached dynamically.");

  if(W_CheckNumForName("GENMIDI") >= 0)
    {
      I_LoadSoundBank(W_CacheLumpName("GENMIDI", PU_CACHE));
      usermsg("\tloading sound banks from GENMIDI lump");
    }

  // no sounds are playing, and they are not mus_paused
  mus_paused = 0;

}

/////////////////////////////////////////////////////////////////////////
//
// Sound Hashing
//

int soundhash_created = false;  // set to 1 when created

        // store sfxinfo_t in hashchain
void S_StoreSfxInfo(sfxinfo_t *sfxinfo)
{
  int hashnum = sound_hash(sfxinfo->name);
  
  if(!soundhash_created)
    {
      S_CreateSoundHashTable();
      hashnum = sound_hash(sfxinfo->name);
    }
  
  // hook it in

  sfxinfo->next = sfxinfos[hashnum];
  sfxinfos[hashnum] = sfxinfo;
}

static void S_CreateSoundHashTable()
{
  int i;
  
  if(soundhash_created) return;   // already done
  soundhash_created = true;    // set here to prevent recursive calls
  
  // clear hash slots first

  for(i=0; i<SOUND_HASHSLOTS; i++)
    sfxinfos[i] = 0;

  for(i=1; i<NUMSFX; i++)
    {
      S_StoreSfxInfo(&S_sfx[i]);
    }
}

sfxinfo_t *S_SfxInfoForName(char *name)
{
   sfxinfo_t *si;

   // haleyjd: reorganized according to SMMU v3.30
   //   sound hashtable being referenced too early?

   if(!soundhash_created)
     S_CreateSoundHashTable();

   si = sfxinfos[sound_hash(name)];

   while(si)
     {
       if(!strcasecmp(name, si->name)) return si;
       si = si->next;
     }
   
   return NULL;
}

void S_Chgun()
{
  memcpy(&S_sfx[sfx_chgun], &chgun, sizeof(sfxinfo_t));
  S_sfx[sfx_chgun].data = NULL;
}

// free sound and reload
// also check to see if a new sound name has been found
// (ie. not one in the original game). If so, we create
// a new sfxinfo_t and hook it into the hashtable for use
// by scripting and skins

// NOTE: LUMPNUM NOT SOUNDNUM
void S_UpdateSound(int lumpnum)
{
  sfxinfo_t *sfx;
  char name[8];
  
  strncpy(name,lumpinfo[lumpnum]->name+2,6);
  name[6] = 0;
  
  sfx = S_SfxInfoForName(name);
  
  if(!sfx)
    {
      // create a new one and hook into hashchain
      
      sfx = Z_Malloc(sizeof(sfxinfo_t), PU_STATIC, 0);
      
      sfx->name = strdup(name);
      sfx->singularity = sg_none;
      sfx->priority = 64;
      sfx->link = NULL;
      sfx->pitch = sfx->volume = -1;
      sfx->skinsound = 0;
      sfx->data = NULL;
      S_StoreSfxInfo(sfx);
    }
  
  if(sfx->data)
    {
      // free it if cached
      Z_Free(sfx->data);      // free
      sfx->data = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Music Hashing
//

static boolean mushash_created = false;

static void S_HookMusic(musicinfo_t *music)
{
  int hashslot;

  if(!music || !music->name) return;
  
  hashslot = sound_hash(music->name);

  music->next = musicinfos[hashslot];
  musicinfos[hashslot] = music;
}

static void S_CreateMusicHashTable()
{
  int i;

  // only build once
  
  if(mushash_created)
    return;
  else
    mushash_created = true;

  for(i=0; i<SOUND_HASHSLOTS; i++)
    musicinfos[i] = NULL;
  
  // hook in all musics
  for(i=0; i<NUMMUSIC; i++)
    {
      S_HookMusic(&S_music[i]);
    }
}

musicinfo_t *S_MusicForName(char *name)
{
  int hashnum = sound_hash(name);
  musicinfo_t *mus;

  if(!mushash_created)
    S_CreateMusicHashTable();

  for(mus=musicinfos[hashnum]; mus; mus = mus->next)
    {
      if(!strcasecmp(name, mus->name))
	return mus;
    }
    
  return NULL;
}

void S_UpdateMusic(int lumpnum)
{
  musicinfo_t *music;
  char sndname[8];

  strncpy(sndname, lumpinfo[lumpnum]->name + 2, 6);

  // check if one already in the table first

  music = S_MusicForName(sndname);

  if(!music)         // not found in list
    {
      // build a new musicinfo_t
      music = Z_Malloc(sizeof(*music), PU_STATIC, 0);
      music->name = Z_Strdup(sndname, PU_STATIC, 0);

      // hook into hash list
      S_HookMusic(music);
    }
}

// haleyjd: rudimentary sound checking function

boolean S_CheckSoundPlaying(mobj_t *mo, char *name)
{
	int cnum;
	
	for(cnum=0; cnum<numChannels && channels[cnum].sfxinfo; cnum++)
	{
		if(mo && channels[cnum].origin == mo &&
		   !strcmp(channels[cnum].sfxinfo->name, name))
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

VARIABLE_BOOLEAN(s_precache,      NULL, onoff);
VARIABLE_BOOLEAN(pitched_sounds,  NULL, onoff);
VARIABLE_INT(default_numChannels, NULL, 1, 128, NULL);
VARIABLE_INT(snd_SfxVolume,       NULL, 0, 15,  NULL);
VARIABLE_INT(snd_MusicVolume,     NULL, 0, 15,  NULL);
VARIABLE_BOOLEAN(forceFlipPan,    NULL, onoff);

void S_ResetVolume()
{
  S_SetMusicVolume(snd_MusicVolume);
  S_SetSfxVolume(snd_SfxVolume);
}

CONSOLE_VARIABLE(s_precache, s_precache, 0) {}
CONSOLE_VARIABLE(s_pitched, pitched_sounds, 0) {}
CONSOLE_VARIABLE(snd_channels, default_numChannels, 0) {}
CONSOLE_VARIABLE(sfx_volume, snd_SfxVolume, 0)
{
  S_ResetVolume();
}
CONSOLE_VARIABLE(music_volume, snd_MusicVolume, 0)
{
  S_ResetVolume();
}

// haleyjd 12/08/01: allow user to force reversal of audio channels
CONSOLE_VARIABLE(s_flippan, forceFlipPan, 0) {}

void S_AddCommands()
{
  C_AddCommand(s_pitched);
  C_AddCommand(s_precache);
  C_AddCommand(snd_channels);
  C_AddCommand(sfx_volume);
  C_AddCommand(music_volume);
  C_AddCommand(s_flippan);
}


//----------------------------------------------------------------------------
//
// $Log: s_sound.c,v $
// Revision 1.11  1998/05/03  22:57:06  killough
// beautification, #include fix
//
// Revision 1.10  1998/04/27  01:47:28  killough
// Fix pickups silencing player weapons
//
// Revision 1.9  1998/03/23  03:39:12  killough
// Fix spy-mode sound effects
//
// Revision 1.8  1998/03/17  20:44:25  jim
// fixed idmus non-restore, space bug
//
// Revision 1.7  1998/03/09  07:32:57  killough
// ATTEMPT to support hearing with displayplayer's hears
//
// Revision 1.6  1998/03/04  07:46:10  killough
// Remove full-volume sound hack from MAP08
//
// Revision 1.5  1998/03/02  11:45:02  killough
// Make missing sounds non-fatal
//
// Revision 1.4  1998/02/02  13:18:48  killough
// stop incorrect looping of music (e.g. bunny scroll)
//
// Revision 1.3  1998/01/26  19:24:52  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/23  01:50:49  jim
// Added music/sound options, and enables
//
// Revision 1.1.1.1  1998/01/19  14:03:04  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
