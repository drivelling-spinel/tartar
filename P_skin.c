/******************************** skins ***********************************/
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
//                Copyright(C) 1999 Simon Howard 'Fraggle'
//
// Skins (Doom Legacy)
//
// Skins are a set of sprites which replace the normal player sprites, so
// in multiplayer the players can look like whatever they want.
//
//--------------------------------------------------------------------------

/* includes ************************/

#include "c_runcmd.h"
#include "c_io.h"
#include "c_net.h"
#include "d_player.h"
#include "doomstat.h"
#include "d_main.h"
#include "info.h"
#include "p_info.h"
#include "p_skin.h"
#include "r_things.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_io.h" //SoM 3/13/2002: Get rid of the strncasecmp warnings in VC++
#include "ex_stuff.h"

// SoM: 3/13/2002: VC++ throws an error here.
#ifdef _MSC_VER
skin_t marine={"PLAY", "marine", SPR_PLAY, NULL, "STF", 0};
#else
skin_t marine={"PLAY", "marine", SPR_PLAY, {}, "STF", 0};
#endif

skin_t **skins = NULL;
char **spritelist = NULL;
char *default_skin = NULL;     // name of currently selected skin

boolean isvowel(char c);

void P_AddSkins();
void P_AddSkin(skin_t *newskin);

char *skinsoundnames[NUMSKINSOUNDS]=
{         
   "dsplpain",
   "dspdiehi",
   "dsoof",
   "dsslop",
   "dspunch",
   "dsradio",
   "dspldeth",
   "dsplfall",
   "dsplfeet",
   "dsfallht", 
};

void P_CreateMarine();
void P_CacheFaces(skin_t *skin);

void P_InitSkins()
{
        int i;
        char **currentsprite;
        skin_t **currentskin;

                // allocate spritelist
        if(spritelist) Z_Free(spritelist);
        spritelist = Z_Malloc(sizeof(char*) * (MAXSKINS+NUMSPRITES+1),
                                PU_STATIC, 0);

                // add the normal sprites
        currentsprite = spritelist;
        for(i=0; i<NUMSPRITES+1; i++)
        {
                if(!sprnames[i]) break;
                *currentsprite = sprnames[i];
                currentsprite++;
        }

        if(skins)
        {
                      // add the skins
            for(currentskin = skins; *currentskin; currentskin++)
            {
                 *currentsprite = (*currentskin)->spritename;
                 (*currentskin)->sprite = currentsprite-spritelist;
                 P_CacheFaces(*currentskin);
                 currentsprite++;
            }
        }

        *currentsprite = NULL;     // end in null
        P_CreateMarine();
}

        // create the marine skin

void P_CreateMarine()
{
        int i;
        static int marine_created = false;

        if(marine_created) return;      // dont make twice
        
        for(i=0; i<NUMSKINSOUNDS; i++)
                marine.sounds[i] = NULL;
        marine.faces = default_faces;
        marine.facename = "STF";

        if(default_skin == NULL) default_skin = Z_Strdup("marine", PU_STATIC, 0);

        P_AddSkin(&marine);

        marine_created = true;
}

        // add a new skin to the skins list
void P_AddSkin(skin_t *newskin)
{
    int numskins;

        // add a skin to the skins list

           // is the skins list already alloced?
    if(skins)   // yes, so realloc it a bit bigger
    {           
        for(numskins=0; skins[numskins]; numskins++);
                                // space for 5 more
        skins = Z_Realloc(skins, sizeof(skin_t*)*(numskins+5),
                          PU_STATIC, NULL);
    }
    else        // no, so make it
    {
        numskins = 0;
                           // space for 5 for comfort
        skins = Z_Malloc(sizeof(skin_t*)*5, PU_STATIC, NULL);
    }

    skins[numskins] = newskin;      // add the new skin
    skins[numskins+1] = NULL;           // end the list
}

skin_t *newskin;

void P_AddSpriteLumps(int lumpnum, char *named)
{
        int i, n=strlen(named);

        for(i=lumpnum;i<numlumps;i++)
        {
                if(!strncasecmp(lumpinfo[i]->name, named, n))
                {
                        // mark as sprites so that W_CoalesceMarkedResource
                        // will group them as sprites
                  lumpinfo[i]->namespace = ns_sprites;
                }
        }
}

void P_ParseSkinCmd(char *line)
{
        int i;

        while(*line==' ') line++;
        if(!*line) return;      // maybe nothing left now

        if(!strncasecmp(line, "name", 4))
        {
                char *skinname = line+4;
                while(*skinname == ' ') skinname++;
                newskin->skinname = strdup(skinname);
        }
        if(!strncasecmp(line, "sprite", 6))
        {
                char *spritename = line+6;
                while(*spritename == ' ') spritename++;
                strncpy(newskin->spritename, spritename, 4);
                newskin->spritename[4] = 0;
        }
        if(!strncasecmp(line, "face", 4))
        {
                char *facename = line+4;
                while(*facename == ' ') facename++;
                newskin->facename = strdup(facename);
                newskin->facename[3] = 0;
        }

        // is it a sound?
        return;           // LP - no skin sounds in Tartar yet

        for(i=0; i<NUMSKINSOUNDS; i++)
        {
           if(!strncasecmp(line, skinsoundnames[i], strlen(skinsoundnames[i])))
           {                    // yes!
              char *newsoundname = line + strlen(skinsoundnames[i]);
              while(*newsoundname == ' ') newsoundname++;
              newsoundname += 2;        // ds

              newskin->sounds[i] = strdup(newsoundname);
           }
        }
}

void P_ParseSkin(int lumpnum)
{
        char *lump;
        char *rover;
        static char *inputline;
        int i;
        boolean comment;

        if(!inputline) inputline = Z_Malloc(256, PU_STATIC, 0);
        newskin = Z_Malloc(sizeof(skin_t), PU_STATIC, 0);
        newskin->spritename = Z_Malloc(5, PU_STATIC, 0);
        strncpy(newskin->spritename, lumpinfo[lumpnum+1]->name, 4);
        newskin->spritename[4] = 0;
        newskin->facename = "STF";      // default status bar face
        newskin->faces = 0;
        newskin->from_extras = 0;
        
        for(i=0; i<NUMSKINSOUNDS; i++)
                newskin->sounds[i] = NULL;       // init to NULL

        lump = W_CacheLumpNum(lumpnum, PU_STATIC);  // get the lump

        rover = lump; inputline[0] = 0;
        comment = false;

        while(rover < lump+lumpinfo[lumpnum]->size)
        {
                if( (*rover=='/' && *(rover+1)=='/') ||        // '//'
                    (*rover==';') || (*rover=='#') )            // ';', '#'
                        comment = true;
                if(*rover>31 && !comment)
                {
                   sprintf(inputline, "%s%c", inputline,
                         *rover=='=' ? ' ' : *rover);
                }
                if(*rover=='\n') // end of line
                {
                        P_ParseSkinCmd(inputline);    // parse the line
                        inputline[0] = 0;
                        comment = false;
                }
                rover++;
        }
        P_ParseSkinCmd(inputline);    // parse the last line

        Z_Free(lump);

        P_AddSkin(newskin);
        P_AddSpriteLumps(lumpnum, newskin->spritename);
}

void P_CacheFaces(skin_t *skin)
{
        if(skin->faces) return; // already cached

        if(!strcasecmp(skin->facename,"STF"))
        {
                skin->faces = default_faces;
        }
        else
        {
                skin->faces =
                      Z_Malloc(ST_NUMFACES*sizeof(patch_t*),PU_STATIC,0);
                ST_CacheFaces(skin->faces, skin->facename);
        }
}

        // this could be done with a hash table for speed.
        // i cant be bothered tho, its not something likely to be
        // being done constantly, only now and again
skin_t *P_SkinForName(char *s)
{
        skin_t **skin=skins;

        while(*s==' ') s++;

        if(!skins) return NULL;

        while(*skin)
        {
                if(!strcasecmp(s,(*skin)->skinname) )
                {
                        return *skin;
                }
                skin++;
        }

        return NULL;
}

void P_SetSkinEx(skin_t *skin, int playernum, boolean extras)
{
        if(!playeringame[playernum]) return;

        players[playernum].skin = skin;
        if(gamestate == GS_LEVEL)
        {
                players[playernum].mo->skin = skin;
                players[playernum].mo->sprite = skin->sprite;
        }

        skin->from_extras = extras;
        if(playernum == consoleplayer) default_skin = skin->skinname;
}

void P_SetSkin(skin_t *skin, int playernum)
{
        P_SetSkinEx(skin, playernum, false);
}

        // change to previous skin
skin_t * P_PrevSkin(int player)
{
        int numskins;
        int skinnum;

        for(numskins=0; skins[numskins]; numskins++);

        // find the skin in the list first

        for(skinnum=0; skins[skinnum]; skinnum++)
                if(players[player].skin == skins[skinnum]) break;

        if(skinnum == numskins) return NULL;         // not found (?)

        --skinnum;      // previous skin

        if(skinnum < 0) skinnum = numskins-1;   // loop around

        return skins[skinnum];
}

        // change to next skin
skin_t * P_NextSkin(int player)
{
        int numskins;
        int skinnum;

        for(numskins=0; skins[numskins]; numskins++);

        // find the skin in the list first

        for(skinnum=0; skins[skinnum]; skinnum++)
                if(players[player].skin == skins[skinnum]) break;

        if(skinnum == numskins) return NULL;         // not found (?)

        ++skinnum;      // next skin

        if(skinnum >= numskins) skinnum = 0;    // loop around

        return skins[skinnum];
}


/**** console stuff ******/

CONSOLE_COMMAND(listskins, 0)
{
        skin_t **skin=skins;

        if(!skins) return;

        while(*skin)
        {
                char tempstr[10];
                strncpy(tempstr,(*skin)->spritename,4);
                tempstr[4]=0;

                C_Printf("%s\n",(*skin)->skinname);
                skin++;
        }
}

//      helper macro to ensure grammatical correctness :)

#define isvowel(c)              \
          ( (c)=='a' || (c)=='e' || (c)=='i' || (c)=='o' || (c)=='u' )

VARIABLE_STRING(default_skin, NULL, 50);

        // player skin
CONSOLE_NETVAR(skin, default_skin, cf_handlerset, netcmd_skin)
{
        skin_t *skin;

        if(!c_argc)
        {
            if(consoleplayer == cmdsrc)
               C_Printf("%s is %s %s\n", players[cmdsrc].name,
                  isvowel(players[cmdsrc].skin->skinname[0]) ? "an" : "a",
                         players[cmdsrc].skin->skinname);
            return;
        }

        if(!strcmp(c_argv[0], "+"))
                skin = P_NextSkin(cmdsrc);
        else if(!strcmp(c_argv[0], "-"))
                skin = P_PrevSkin(cmdsrc);
        else if(!(skin = P_SkinForName(c_argv[0])))
        {
           if(consoleplayer == cmdsrc)
                C_Printf("skin not found: '%s'\n", c_argv[0]);
           return;
        }

        P_SetSkin(skin, cmdsrc);
        // wake up status bar for new face
        redrawsbar = true;
        if(defaults_loaded)
          T_EnsureGlobalIntVar("_private_bjskin", 1);
        hint_bjskin = "";
}

void P_Skin_AddCommands()
{
   C_AddCommand(skin);
   C_AddCommand(listskins);
}

// EOF
