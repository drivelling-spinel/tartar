# Tartar

Tartar is a Doom source port for DOS, author aiming for it to be useful
to players running pure (as in non-emulated) DOS or Windows 9X on modern 
machines for retro gaming. 

## Tartar highlights

- Has better audio compatibility through updated Allegro library 
  from [MBF 2.0.4 Maintenance Release](doc/COPYRGHT/MBFUP204.TXT)
  and plays 16 bit sound effects 
- Supports additional screen resolutions of 640x480 and 1280x1024
- Can load
    - classic Caverns of Darkness TC for DOS
    - what remains from the cancelled Eternity TC
    - Chex Quest without dependency on any Doom assets
- Includes numerous quality of life improvements including 
  auto-loading of WAD, DEH and BEX files, compatibility improvements
  and bugfixes  
- Treats certain popular WADs as first class mods improving
  player's experience with them:
  - InstaDoom by \@linguica
  - Intemission Maps by \@oliacym
  - No Rest for the Living via a "helper WAD"  
  - Jumpwad.wad by \@Grain of Salt and Ribbiks 
- New display (game view) size for higher resolutions and 
  new translucency mode
- Can load more complex (i.e. "more contemporary") maps
  than the Eternity Engine version it is based on. Some but 
  not all maps from Eviternity, Ancient Aliens and Sunlust can
  be played for example

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

There, STAGE3M3 directory contains current version of the port 
including TARTAR.EXE and additional assets and files required to run it. 
EXTRAS document above describes the contents of DEMOS and GOODIES directories. 

If you are looking for the 2021 Christmas binary, which was previously
under TARTAR directory - it's available under STAGES/STAGE1.

### Playing Doom with TARTAR

1. Download STAGE3M3 directory contents from the above link (MEGA has means 
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

2. Download STAGE3M3 directory contents from the above link (MEGA has means 
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
