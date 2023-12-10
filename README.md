# Tartar

Tartar is a Doom source port for DOS, author aiming for it to be useful
to players running pure (as in non-emulated) DOS or Windows 9X on modern 
machines for retro gaming. 

## Tartar highlights

- Has better audio compatibility through updated Allegro library 
  from [MBF 2.0.4 Maintenance Release](doc/COPYRGHT/MBFUP204.TXT)
  and plays 16 bit sound effects (useful if you have SB16 or later)
- Supports additional screen resolutions 640x480 and 1280x1024
  with and without upscaling from smaller core rendering resolution
  and optional aspect ratio correction for non-CRT displays
- Includes numerous quality of life improvements including 
  auto-loading of WAD, DEH and BEX files to include
  skins, fixes and WAD compatibility packs for specific PWADs
- Offers extended game view size for higher resolutions, 
  new optional "grille" translucency for sprites and textures and 
  new optional "simplified" and more accessible level shading mode  
- Contains many bugfixes and PWAD compatibility improvements
- Implements the compulsory blood spat recoloring for "hissies", 
  "bruisers", flemoids, ubermutants and roaches

## What to play with Tartar

- Original Caverns of Darkness TC for DOS
- Curios from what remains of the cancelled Eternity TC
- Vanilla Doom 2, TNT Evilution and Plutonia Experiment   
  (with animated intermission screens a-la Doom 1 courtesy of 
  Doomworld user \@oliacym)
- No Rest for the Living (aka NERVE.WAD) with proper map names,
  music tracks assignment, levels and secrets progression and 
  intermission backdrops courtesy of \@oliacym 
  with helper WAD included in the distribution
- Chex Quest without Doom IWAD - and it will end after level 5!   
  (also green slime splashes can be activated by switching on particles)
- [Wolfendoom collection by Laz Rojas]
  (https://www.moddb.com/mods/tartar/news/tartar-source-port-almost-good-enough-for-wolfendoom-now)
  (MAC versions strongly preferred!)
  with helper WAD included in the distribution
- Strain TC for DOS - via a compatibility option to change [MBF collisions code]
  (https://www.doomworld.com/forum/post/2507168)
- Sunlust and maps using _uncompressed_ extended nodes table format
- Eviternity and maps using sprites and textures that are
  more than 255 pixels tall
- Ancient Aliens and maps with non 9px high fonts, elaborate MUS format music tracks
  and non power-of-2 wide textures 
- Avactor and maps with DeepSea extended node format
- Hydrosphere and maps with higher resolution sky textures,
  in this case also with proper progression and ending
  with helper WAD included in the distribution
- The Long Trek Back Home with intermission backdrops by \@oliacym 
  with helper WAD included in the distribution
- 2022 A Doom Odyssey with non-episodic linear progression through all maps,
  proper secrets progression and intermission screens 
  with helper WAD included in the distribution
- ??? and maps with non-Unix line endings in DeHackEd patches, 
  or other DeHackEd patches sheninegans like references to non-existant code pointers
  (e.g. Rush)
- HACX commercial release without Doom II IWAD  
  (but make sure Tartar is given a PLAYPAL, e.g. by putting one of InstaDoom
  filter WAD-s into FILTERS directory AND that text mode startup is on!)
- Also - only if you are desperate enough - The People's Doom as IWAD

## Also...
... for some traits "less common among source ports":
- Arcade play mode where player is resurrected on the spot without
  level restarting 
- Next/prev music track bindable hotkeys
- Screenshots from virtually anywhere in the game with bindable hotkey
- Cycle through InstaDoom filters (or any other PLAYPAL/COLORMAP providing
WAD-s for that matter) without restarting the level
- Voltron assembly features:
  -- drop in SELFIE.WAD and SELFIE.DEH to make sefies in any PWAD
     and still shoot plasma rifle happily 
  -- drop in JUMPWAD.WAD to switch to "pogo stick" mode in any PWAD
     and jump around by pressing fire
     (by the way, you can also load Jumpwad as a PWAD with Tartar)
- Send regards to the loved ones with a bindable hotkey, 
  (e.g. while recording a video)

## What Tartar CANNOT do
While these traits may be more common among source ports these days,
this is not what Tartar supports:
- Cannot display PNG sprites and textures
- Cannot load _compressed_ extended format node tables or Hexen format maps
- Cannot play OGG, MP3 or FLAC sounds or music
- Cannot process UMAPINFO or singe lump stand alone EMAPINFO lumps
- Cannot cope with MBF21 and DEHEXTRA features and code pointers in DeHackEd patches
- Cannot run Heretic, Hexen or Strife 
- Cannot apply EDF or DECORATE extensions from the mods to the game

## Documentation

1. [CHANGES](doc/changes.md)     - comprehensive list of changes in Tartar 
                                   milestone release compared to the 
                                   Eternity Engine version it is based on
2. [MAINTAIN](doc/maintain.md)   - changes in latest maintenance release 
                                   not covered by the above doc
3. [AUTHORS](doc/authors.md)     - acknowledgements for authors of the ports 
                                   Tartar is based on and their work
4. [STAGE1](doc/STAGE1.TXT)      - templated (and outdated ) readme one 
                                   would expect to accompany any Doom mod
5. [BUGS](doc/bugs.md)           - list of known issues in Tartar 
6. [EXTRAS](doc/extras.md)       - notes on extras and goodies included with 
                                   the distribution 

## Obtaining and installing Tartar

Tartar distribution is available following the link below:

<https://mega.nz/folder/V50HiaSJ#yZa2LjO_kJB3Vs4tvlR3cQ>

There, TARTAR directory contains latest version of the port 
including TARTAR.EXE and additional assets and files required to run it. 
EXTRAS document above describes the contents of DEMOS and GOODIES directories. 


### Playing Doom with TARTAR

1. Download TARTAR directory contents from the above link (MEGA has means 
   to download directory as a ZIP archive)

2. For a quick and simple start either 

   - put one of the supported WADs (e.g. DOOM.WAD, DOOM1.WAD, DOOM2.WAD, 
     PLUTONIA.WAD or TNT.WAD) into the directory with TARTAR.EXE 
   
   - or put TARTAR.EXE and other files from the directory into your Doom 
     game installation  
  
3. Run SETUP.EXE from directory with TARTAR.EXE and follow on-screen 
   instructions to configure sound and music card.

4. Run TARTAR.EXE to play 

5. Also be sure to check [EXTRAS](doc/extras.md) for explanation on best 
   way of playing No Rest for the Living Doom II PWAD and for guides on
   installing extra mods with Tartar.

### Playing Caverns of Darkness with Tartar

1. Download Caverns of Darkness TC distribution and unzip the contents 
   into a separate directory

2. Download TARTAR directory contents from the above link (MEGA has means 
   to download directory as a ZIP archive) and copy everything there 
   to the same directory as COD **except do not** copy KEYS.CSC 
   and retain the one shipped with the TC.

3. Copy DOOM2.WAD into the same directory as COD

4. Run SETUP.EXE from the COD directory and follow
   on screen instructions to configure sound and music card.

5. Run TARTAR.EXE -file COD.WAD CODLEV.WAD to play

### Installing popular fix packs with Tartar

Tartar's WAD autoloading makes it easy to add fix packs
to the installation. Suggested WAD-s to try out include:
  - Doom Sound Bulb by \@SeanTheBermanator
  - Hi Res Doom Sound Effects Pack by \@perkristian
  - Doom 2 Minor Sprite Fixing Project by \@revenant100

Follow the below steps to have them installed with Tartar

1. Install Tartar

2. Drop the desired fix pack WAD into FIXES directory

3. Or if Tartar will be used with multiple IWADs drop the WAD into
   a subdirectory of FIXES that has name matching the name of the IWAD
   the fix pack is to be loaded with. For example:
      
  C:\GAMES\TARTAR\   
  C:\GAMES\TARTAR\TARTAR.EXE   
  ...   
  C:\GAMES\TARTAR\FIXES\   
  C:\GAMES\TARTAR\FIXES\Doom_Sound_Bulb.wad   
  C:\GAMES\TARTAR\FIXES\DOOM\   
  C:\GAMES\TARTAR\FIXES\DOOM\D1SPFX20.WAD   
  C:\GAMES\TARTAR\FIXES\DOOM2\   
  C:\GAMES\TARTAR\FIXES\DOOM2\D2SPFX20.WAD   

4. Start Tartar as you would normally do and enjoy improved Doom experience

5. Should you want to play a certain PWAD without any of the fix packs loaded
   start Tartar with a command similar to the below one:
      
   TARTAR.EXE -noload -file NUTS.WAD

## On the name of the port

I've been asked why Tartar refers to itself as Eternity Engine when
being run. As Tartar's author I personally have no issue with this small
identity crisis and would not hurry resolving it. Even though Tartar comes
from 2021 it is in a way a thing of the past, and in author's opinion,
retaining such smaller bits helps retain the "retro" feel that the port
has to it.

Of course, if this confuses players, author will make the necessary changes,
(after all they are not hard to make).

The name itself... could it be that Tartar is from Greek Tartarus 
(Τάρταρος) that is the deepest abyss in Ancient Greek mythology where,
no doubt, demons and horrible beasts of chaos roam. A suitable place
for a Doom game protagonist to visit...  

Well, in fact not. Tartar continues a glorious tradition of giving Doom 
source ports gastronomic names and bears the name of the white sauce, 
traditionally served with fried fish (and chips). 
Indeed, cod is even more delicious when served with tartar.
