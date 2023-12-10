# Extras and goodies 

Description of additional stuff included with Tartar distribution

## Demos

### Chex Quest

Included in DEMOS\CHEX is a bundle of Tartar binaries and Chex Quest 1 and 
Chex Quest 2 games. The demo allows anyone to appreciate Tartar in action
without even loading any of Doom assets (i.e. one does not need to own Doom),
and without a risk of displaying content not suitable for minors, like gore
on the walls or demon heads being split blown into smithereens, or other
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

3. To run the maps DOOM2.WAD is be required, except for map ETC21.WAD
   DOOM.WAD is required instead. For fast and easy start copy either DOOM2.WAD
   or both DOOM2.WAD and DOOM.WAD into the directory containing ETC.WAD 

4. Under MS-DOS or Windows 9X CD into the directory with ETC.WAD and
   run LAUNCH.BAT. This will list the available maps and provide further
   instructions

5. In DOSBox, FreeDOS or elsewhere, or if using an alternative command shell,
   please use a command similar to the below to run a map:  

   TARTAR\TARTAR.EXE -file ETC.WAD LEVLES\ETC01.WAD  

   or

   TARTAR\TARTAR.EXE -iwad DOOM.WAD -file ETCGFX1.WAD LEVELS\ETC21.WAD  

## Extras

### COD10

EXTRAS\COD10 contains Caverns of Darkness engine executable (COD.EXE) 
built from the sources found in COD10SRC.ZIP that Joel Murdoch 
has shared in 2020 without any modifications, the only change being 
that allegro.h was replaced with the version coming from MBF 2.0.4
sources. The binary was built with the same version of DJGPP as MBF 2.0.4 
binaries and linked with Allegro library produced from MBF 2.0.4 sources.
That alone has improved stability of COD.EXE on the author's 
target system.

To use, download the original COD distribution (the file I got was 
named CC-COD.ZIP), unzip it and then copy over the contents of EXTRAS\COD10.
It is not necessary to overrite KEYS.CSC with the version provided,
if it's already present in your COD directory. Simply run COD.EXE to play.

### Eternity dialog tools

A Python-based tool to compile dialog scripts into binary format that 
Eternity Engine originally used for dialogs is included in EXTRAS\DLGTOOLS. 
Sample scripts in plain text format and resulting binary lumps are provided
as well. To run the tool first make sure Python 3 is installed, then run 
the following command, replacing the names of script file and dialog lump 
as appropriate:

python dlgtools.py compile <script0.txt >dialog0.lmp  

The sample dialogs are in fact from the "Fun in Dreadmere" map included in the 
Eternity TC assets demo pack.

### Edited Eternal Doom shell 

Eternal Doom by Team TNT has a wonderfull shell executable that unfortunatelly 
requires that player's Doom source port file is named DOOM2.EXE. Provided in 
EXTRAS\ETERNAL is a hex-edited copy of the shell that calls ETERNITY.EXE instead,
so that players wishing to play Eternal Doom via Eternity Engine can do so 
from the shell, enjoying all of the extras, like simple level (and extra level) 
selection, WAD patching and the like.

### MAPINFO lumps for No Rest for the Living

By version 3.29db5-joel2 Eternity Engine had developed support for advanced features 
via MAPINFO files, allowing for example exiting to secret levels or ending the game 
from arbitrary maps. Unlike other ports, SMMU and older versions of Eternity did not
keep all MAPINFO details in a single lump, and instead looked for these details 
in the index entries of the each maps itself (e.g. MAPINFO for MAP01 would be 
in the lump MAP01, MAPINFO for MAP02 - in the lump MAP01 of the same wad file, etc.). 
In EXTRAS\NERVE one can find a set of such lumps that, if incorporated into NERVE.WAD 
(player has to do that on their own) allows for No Rest for the Living 
to be played via Tartar with intended map names, music tracks and end game text.

