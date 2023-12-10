# Tartar

Tartar is a Doom source port for DOS, author aiming for it to be useful
to players running pure (as in non-emulated) DOS or Windows 9X on modern 
machines for retro gaming. 

## Tartar highlights

- Has better audio compatibility through updated Allegro library 
  from [MBF 2.0.4 Maintenance Release](doc/COPYRGHT/MBFUP204.TXT)
- Supports additional screen resolutions of 640x480 and 1280x1024
- Can load
    - classic Caverns of Darkness TC for DOS
    - what remains from the cancelled Eternity TC
    - Chex Quest without dependency on any Doom assets
- Includes numerous smaller changes that are seen by the author as 
  quality of life improvements 

## Documentation

1. [CHANGES](doc/changes.md) - comprehensive list of changes in Tartar 
                               compared to ee-2.39db5-joel2 it is based on
2. [AUTHORS](doc/authors.md) - acknowledgements for authors of the ports Tartar 
                               is based on and their work
3. [README](doc/TARTAR.TXT)  - templated readme one would expect to accompany 
                               any Doom related release
4. [EXTRAS](doc/extras.md)   - notes on extras and goodies included with the
                               binary distribution 

## Obtaining and installing Tartar

Tartar distribution is available following the link below:

<https://mega.nz/folder/V50HiaSJ#yZa2LjO_kJB3Vs4tvlR3cQ>

There, TARTAR directory contains the the port TARTAR.EXE and additional 
assets and files required to run it. EXTRAS document above describes the
contents of DEMOS and EXTRAS directories.

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
