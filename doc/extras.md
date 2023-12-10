# Extras and goodies 

Description of additional stuff included with Tartar distribution
or auto-loaded by Tartar if installed by the players 

## Extras

Extras are mods by other authors that Tartar provides first-class support for,
and would load in case they are found side by side with TARTAR.EXE, 
but that are not shipped with Tartar. Players do not need any special command 
line arguments to load these WAD-s, once they have been installed.

### InstaDoom filters

If FILTERS directory is found side by side with TARTAR.EXE it will be scanned 
for WAD-s, and all WAD-s found will be loaded for the purpose of registering 
the palettes and color maps defined in them. Players can use PAL_NEXT and 
PAL_PREV console commands to cycle through those palettes while playing the 
game with arbitrary PWAD-s loaded. The commands can be bound to key in 
Extra Keys submenu of Key Bindings. By default \[ and \] are assigned to the
commands. 

To try palette cycling in action:

1. Install Tartar.

2. Download InstaDoom mod, e.g. from from ID Games archive.

3. From InstaDoom distribution zip, extract FILTERS directory and place that 
   directory (with all its contents) in the same directory as TARTAR.EXE.

4. If in "real" DOS set FILES in CONFIG.SYS to a considerably big value, e.g. 
   FILES=70  
   
5. Run TARTAR.EXE as usual (no need for an additional -file command line 
   argument to load the filters) and be on the lookout for a message in the 
   console mentioning the number of loaded filters.

6. While in game (starting immediately from the title screen) 
   use \[ and \] keys to switch palettes.

### InstaDoom selfie stick

In addition to the FILTERS from InstaDoom Tartar has support for SELFIE.WAD 
from the same mod. When SELFIE.WAD and SELFIE.DEH both are found in the same 
directory as TARTAR.EXE, they are loaded and players are given a selfie stick. 
Unlike the original SELFIE.WAD stick, the one in Tartar does not replace any 
of the players' weapons, nor does it require any ammo. Behaviour of plasma gun 
and BFG are unchanged when SELFIE.WAD is loaded as an extra. 

To activate the stick use SELFIE console command, by default bound to key \\.
To deactivate the stick select any weapon. With the stick out press Fire to 
take a selfie; this produces 3 screenshots, filenames for which are output in 
the console.

To try Tartar with selfie extra:

1. Install Tartar.

2. Download InstaDoom mod, e.g. from ID Games archive.

3. From InstaDoom distribution zip, extract SELFIE.WAD and SELFIE.DEH into 
   the same directory as TARTAR.EXE.

4. Run TARTAR.EXE as usual (no need for additional -file or -deh command line 
   argument to load SELFIE.WAD or SELFIE.DEH) and be on the lookout for a 
   message in the console mentioning the selfie stick.

5. Play the game, take out the stick by pressing L whenever you want, remove it
   by switching to any weapon. Use Fire to take selfies.

### Custom Intermission Maps

Tartar supports custom Intermission Maps by doomworld.com user @olyacim. 
If one of DOOM2, TNT or PLUTONIA IWADs is loaded without any PWAD-s and 
Intermission Maps WAD is detected by Tartar, it will use @olyacim's images
for backgrounds in MAP01-MAP30 intemission screens, decorating them with blood 
spats and "you-are-here" arrows.

To use Intermission Maps with Tartar:

1. Install Tartar.

2. Obtain INTMAPD2.WAD and place it in the same directory as TARTAR.EXE.

3. _Optional_ obtain and place INTMAPPL.WAD (for PLUTONIA.WAD), 
   INTMAPEV.WAD (for TNT.WAD), INTMAPNR.WAD (for NERVE.WAD) in the same
   directory as TARTAR.EXE.

4. Run TARTAR.EXE with DOOM2 IWAD. When running without PWAD-s no additional 
   arguments are required. When running with PWADS, enable the intermission 
   maps by adding -wimaps command line argument.

5. For custom intermission maps when running with TNT or PLUTONIA IWAD-s 
   see step 3 above. Also check the section on helper WAD-s for details on
   No Rest for The Living intermission backgrounds.
   
### Helper WAD-s

Helper WAD-s found in GOODIES\TAPE directory are PWAD-specific extras that add
to players experience with particular WAD-s or fix bugs that would make 
playing those WAD-s with Tartar impossible or less enjoyable.

Among other goodies the following helper WAD-s are included in the distribution:

 - GOODIES\TAPE\LONGTREK.WAD - adds custom intermission backgrounds; will crash
                               unless INTMAPD2.WAD is also loaded. See Custom
                               Intermission Maps section of this file for 
                               instructions
 - GOODIES\TAPE\3DODOOM.WAD  - fixes an issue with garbage output as options menu 
                               background 
 - GOODIES\TAPE\NERVE.WAD    - adds custom title pic, level names, assigns 
                               correct music tracks and ensures intended level 
                               progression for No Rest for the Living. Also 
                               enables custom intermission backgrounds provided 
                               INTMAPNR.WAD is also loaded.
                           
To play No Rest for the Living with Tartar enjoying the benefits of the
helper WAD provided:

1. Install Tartar and check that Doom 2 runs with it.

2. _Optional_ obtain INTMAPNR.WAD and place it in the same directory as 
   TARTAR.EXE.

3. Put _helper_ NERVE.WAD found in GOODIES\TAPE directory of the distribution
   into TAPE directory sitting side by side with TARTAR.EXE. E.g.
   
   C:\GAMES\DOOM2\TARTAR.EXE  
   C:\GAMES\DOOM2\INTMAPNR.WAD    - optional
   ...  
   C:\GAMES\DOOM2\TAPE  
   C:\GAMES\DOOM2\TAPE\NERVE.WAD  

4. Obtain actual No Rest for the Living NERVE.WAD - e.g. from Doom 3 BFG
   edition installation and put it into the same directory as TARTAR.EXE.

5. Run Tartar with a command similar the below one:

   TARTAR.EXE -file NERVE.WAD -nodemo
                           
6. If the abvove steps have been all carried out, both helper NERVE.WAD 
   and actual PWAD will be loaded and custom title pic will be displayed.

### Jumping with JUMPWAD.WAD

If JUMPWAD.WAD is found side by side with TARTAR.EXE it will be loaded but
out of all lumps only those that are required for the unique JUMPWAD 
jump mechanics will be loaded. With JUMPWAD.WAD loaded as an extra,
players can use POGO console command to activate jumping mode. In this mode
their current weapon is brought down, and firing will result in jumping
instead. The command can be bound to a key in Extra Keys submenu of Key 
Bindings. By default no key is assigned to the command. To leave jumping
mode switch to any weapon.

Step by step instructions for activating the mod:

1. Install Tartar.

2. Download JUMPWAD.WAD, e.g. from ID Games archive and put it into 
   the same directory as TARTAR.EXE

3. Run TARTAR.EXE as usual (no need for an additional -file command line 
   argument to load the JUMPWAD.WAD as an extra).

4. While in game use POGO console command to activate jumping mode.

5. Optionally go to Options and navigate to Bindings / Extra Keys 
   submenu to assign a permanent key to POGO command.

Note that Tartar also has experimental support for actually playing JUMPWAD.
For that players have to load JUMPWAD.WAD in "the conventional manner" 
with -file command line argument when starting TARTAR.EXE .

## Tartar demonstrations

### Chex Quest

Included in DEMOS\CHEX is a bundle of Tartar binaries and Chex Quest 1 and 
Chex Quest 2 games. The demo allows anyone to appreciate Tartar in action
without even loading any of Doom assets (i.e. one does not need to own Doom),
and without a risk of displaying content not suitable for minors, like gore
on the walls or demon heads being blown into smithereens, or other
"nasty" things found in ID games. The configuration is adjusted so that 
included Dehacked patch by Simon Howard is loaded.

To install and play:

1. Download the directory contents (MEGA has an option of downloading directory
   as ZIP file) including subdirectories

2. Run SETUP.EXE included in TARTAR subdirectory and follow on-screen 
   instructions to configure sound and music card

3. Run CHEX.BAT for Chex Quest 1 or CHEX2.BAT for Chex Quest 2

### Cancelled Eternity TC content

DEMOS\ETERNITY contains a package of maps and assets from the cancelled
Eternity TC that Eternity Engine was originally built to run. Most files 
included have been published by James Haley, with the exception of
map "Fun in Dreadmere" that was "created" in 2021 by Tartar's author  
using geometry from original "Dreadmere" map by James Haley to test
Tartar's capability of loading Eternity TC resources. The "new" map
file is TRIBUTE\SWMP2021.WAD. 

To play the maps:

1. Download the directory contents (MEGA has an option of downloading directory
   as ZIP file) including subdirectories

2. Run SETUP.EXE included in TARTAR subdirectory and follow on-screen 
   instructions to configure sound and music card

3. To run the maps DOOM2.WAD is required, except for map ETC21.WAD
   DOOM.WAD is required instead. For fast and easy start copy either DOOM2.WAD
   or both DOOM2.WAD and DOOM.WAD into the directory containing ETC.WAD 

4. In MS-DOS or Windows 9X change into the directory with ETC.WAD and
   run LAUNCH.BAT. This will list the available maps and provide further
   instructions

5. In DOSBox, FreeDOS or elsewhere, or if using an alternative command shell,
   use a command similar to the below to run a map:  

   TARTAR\TARTAR.EXE -file ETC.WAD LEVLES\ETC01.WAD  
   or  
   TARTAR\TARTAR.EXE -iwad DOOM.WAD -file ETCGFX1.WAD LEVELS\ETC21.WAD  

## Goodies

### COD10

GOODIES\COD10 contains Caverns of Darkness engine executable (COD.EXE) 
built from the sources found in COD10SRC.ZIP that Joel Murdoch 
has shared in 2020 without any modifications, the only change being 
that allegro.h was replaced with the version coming from MBF 2.0.4
sources. The binary was built with the same version of DJGPP as MBF 2.0.4 
binaries and linked with Allegro library produced from MBF 2.0.4 sources.
That alone has improved stability of COD.EXE on the author's 
target system.

To use, download the original COD distribution (the file I got was 
named CC-COD.ZIP), unzip it and then copy over the contents of GOODIES\COD10.
It is not necessary to overrite KEYS.CSC with the version provided,
if it's already present in your COD directory. Simply run COD.EXE to play.

### Eternity dialog tools

A Python-based tool to compile dialog scripts into binary format that 
Eternity Engine originally used for dialogs is included in GOODIES\DLGTOOLS. 
Sample scripts in plain text format and resulting binary lumps are provided
as well. To run the tool first make sure Python 3 is installed, then run 
the following command, replacing the names of script file and dialog lump 
as appropriate:

python dlgtools.py compile <script0.txt >dialog0.lmp  

The sample dialogs are in fact from the "Fun in Dreadmere" map included in the 
Eternity TC assets demo pack.

### Edited Eternal Doom shell 

Eternal Doom by Team TNT has a powerfull shell executable that unfortunatelly 
requires that player's Doom source port file is named DOOM2.EXE. Provided in 
GOODIES\ETERNAL is a hex-edited copy of the shell that calls ETERNITY.EXE 
instead, so that players wishing to play Eternal Doom via Eternity Engine 
can do so from the shell, enjoying all of the extras: simple level 
(and extra level) selection, WAD patching and the like.

### MAPINFO lumps for No Rest for the Living

By version 3.29db5-joel2 Eternity Engine had developed support for advanced 
features via MAPINFO files, allowing for example exiting to secret levels or 
ending the game from arbitrary maps. Unlike other ports, SMMU and older 
versions of Eternity did not keep all MAPINFO details in a single lump, and 
instead looked for these details in the index entries of the each map itself 
(e.g. MAPINFO for MAP01 would be in the lump MAP01, MAPINFO for MAP02 - 
in the lump MAP02 of the same wad file, etc.). 

In GOODIES\NERVE\LUMPS\NOINTMAP one can find a set of such lumps that, 
if incorporated into NERVE.WAD allows for No Rest for the Living to be played 
via Tartar with intended map names, music tracks and end game text.

Alternatively in GOODIES\NERVE\LUMPS\INTMAPS a similar set of lumps is 
provided that uses intermission screen backgrounds from [No Rest for the Living
 Intermission Maps](https://www.doomworld.com/forum/topic/129415) by
@oliacym. These are also included in the NERVE.WAD helper WAD shipped with
Tartar distribution, so check the secsion on Helper WADs in Extras section 
of this file for more information. 

