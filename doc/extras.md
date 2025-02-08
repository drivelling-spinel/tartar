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
Extra Keys submenu of Key Bindings. 

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
   use assigned keys, e.g. \[ and \], to switch palettes.

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

Helper WAD-s found in SHIMS directory are PWAD-specific extras that add
to players experience with particular WAD-s or fix bugs that would make 
playing those WAD-s with Tartar impossible or less enjoyable.

Some example of helper WAD-s included in the distribution:

 - SHIMS\LONGTREK.WAD        - adds custom intermission backgrounds; will crash
                               unless INTMAPD2.WAD is also loaded. See Custom
                               Intermission Maps section of this file for 
                               instructions
 - SHIMS\3DODOOM.WAD         - fixes an issue with garbage output as options menu 
                               background 
 - SHIMS\NERVE.WAD           - adds custom title pic, level names, assigns 
                               correct music tracks and ensures intended level 
                               progression for No Rest for the Living. Also 
                               enables custom intermission backgrounds provided 
                               INTMAPNR.WAD is also loaded.
                           
To play No Rest for the Living with Tartar enjoying the benefits of the
helper WAD provided:

1. Install Tartar and check that Doom 2 runs with it.

2. _Optional_ obtain INTMAPNR.WAD and place it in the same directory as 
   TARTAR.EXE.

3. Put _helper_ NERVE.WAD found in SHIMS directory of the distribution
   into SHIMS directory sitting side by side with TARTAR.EXE. E.g.
   
   C:\GAMES\DOOM2\TARTAR.EXE  
   C:\GAMES\DOOM2\INTMAPNR.WAD    - optional
   ...  
   C:\GAMES\DOOM2\SHIMS  
   C:\GAMES\DOOM2\SHIMS\NERVE.WAD  

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

