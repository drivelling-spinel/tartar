Recent brilliant Knee Deep in Knee Deep in ZDoom has inspired me to blow off the dust from Tartar code and prepare a maintenance release of Stage 3 that mostly addresses issues with the new mod (all that while Stage 4 has been put on hiatus somewhat and none of Stage 4 commits have made it through into the trunk yet).

The release in the works now will contain:

###### New Tartat build that:

1. No longer alters level geometry upon loading in an attempt to fix slime trail,s as this used to cause rendering problems even with vanilla Final Doom maps. remove_slime_trails configuration file property and p_rmslime CVAR are added to allow players to revert to the old behaviour, but it is not recommended.
2. No longer detects Freedoom Phase II as being TNT Evilution, outputs Feedoom Phase I or Freedoom Phase II as the name of the game at the start and treats Freedoom as a game mission pack internally
3. Inverts the logic for SMMU coloured lightning and tall textures support compatibility flags to bring them in line with how compatibility flags work, all this for better demo compatibility (flag on means feature off)
4. Does not corrupt recoloring tables after blood recoloring is switched off - at least KDiKDiZD was sensitive to this resulting in sprite artifacts
5. Supports new option in SMMU-style map information to signal that blood recoloring is not recommended in a map. Loading such map will suppress blood recoloring for the duration of the map, but not change any of the game options, allowing the player to switch between, say KDiKDiZD and some less palette-hacks-intense other mod, without the hassle of going into game options each time. 
The way it is achieved is by treating the option in mapinfo as a "soft" option in the sense that after the level is started and blood recoloring is suppressed, player may switch it back on any time via Options / Enemies menu and that choice will persist throughout the playthrough, including respawning and subseqent next level. Starting new game resets the engine to again respect recommendations from the maps.
6. Makes dehacked patches imported from JUMPWAD.WAD and INSTADOOM.WAD work with PWAD-s that themselves have complex PWAD patches, thus enabling selfies and Archie-infused jumping with them. 
7. Fixes buffer overflow errors when running commands from CSC script files including from KEYS.CSC, and improves controls over buffer length handling here (sound samples) and there (game sprite list).d

###### New compatibility WAD for KDiKDiZD that:

1. Shows longer names for the maps and tall skies (as modern Eternity Engine does) and marks maps to not have monster blood recoloring on them
2. Changes selfie sprites from INSTADOOM to be displayed with correct colors 
3. Addresses menu items and message drawing issue with Tartar that results in some of the colors being wrong for them

Loook for the WAD named KDiKDi_A.WAD in GOODIES\TAPE and put it into TAPE directory to be loaded by Tartar automatically with KDiKDiZD.

###### New kind of distribution package for Tartar

For the maintenance release I am introducing an experimental package that includes Tartar preconfigured and bundled with DosBOX for Windows and all the goodies and extras, all as a single ZIP file. Freedoom Phase II is included as IWAD and Community Chest 2 as a sample PWAD - drag it onto DOOM2.CMD to start playing immediately. This package includes works by other authors that are provided unmodified with readme and licence files available in COPYRGHT directory (provided these were part of the said works). Look for Tartar-portable.zip in the binary distribution directory to try it out.

Note that:
I. None of the commerically available IWAD-s or PWAD-s are included.
II. An alternative build of Tartar is included in the portable package that does not support 640x400 resolution and switches to 640x480 resolution instead.


**Finally**, as a small bonus an alternative take on MAP01 of KDiKDiZD is included which allows the player to select difficulty level and jump to the ZM1 via a teleporter in the room. Load it after the mod itself this way:

   TARTAR -file KDiKDi_A KDiKDi_B KDiKDiM1 -warp 1

Be mindful of playing too much with that UAC terminal found in the map - _you have been warned_.
