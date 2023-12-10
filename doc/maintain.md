## Stage 3 Maintenance Release 1

Recent brilliant Knee Deep in Knee Deep in ZDoom has inspired me to blow off the dust from Tartar code and prepare a maintenance release of Stage 3 that mostly addresses issues with the new mod (all that while Stage 4 has been put on hiatus somewhat and none of Stage 4 commits have made it through into the trunk yet).

The release includes:

###### New Tartar build that:

1. No longer alters level geometry upon loading in an attempt to fix slime trails as this used to cause rendering problems even with vanilla Final Doom maps. remove_slime_trails configuration file property and p_rmslime CVAR are added to allow players to revert to the old behaviour, but it is not recommended.
2. No longer detects Freedoom Phase II as being TNT Evilution, outputs Feedoom Phase I or Freedoom Phase II as the name of the game at the start and treats Freedoom as a game mission pack internally
3. Inverts the logic for SMMU coloured lightning and tall textures support compatibility flags to bring them in line with how compatibility flags work, all this for better demo compatibility (flag on means feature off)
4. Does not corrupt recoloring tables after blood recoloring is switched off - at least KDiKDiZD was sensitive to this resulting in sprite artifacts
5. Supports new option in SMMU-style map information to signal that blood recoloring is not recommended in a map. Loading such map will suppress blood recoloring for the duration of the map, but not change any of the game options, allowing the player to switch between, say KDiKDiZD and some less palette-hacks-intense other mod, without the hassle of going into game options each time. 
The way it is achieved is by treating the option in mapinfo as a "soft" option in the sense that after the level is started and blood recoloring is suppressed, player may switch it back on any time via Options / Enemies menu and that choice will persist throughout the playthrough, including respawning and subsequent next levels. Starting new game resets the engine to again respect recommendations from the maps.
6. Makes dehacked patches imported from JUMPWAD.WAD and INSTADOOM.WAD work with PWAD-s that themselves have complex PWAD patches, thus enabling selfies and Archie-infused jumping with them. 
7. Fixes buffer overflow errors when running commands from CSC script files including from KEYS.CSC, and improves controls over buffer length handling here (sound samples) and there (game sprite list).d

###### New compatibility WAD for KDiKDiZD that:

1. Shows longer names for the maps and tall skies (as modern Eternity Engine does) and marks maps to not have monster blood recoloring on them
2. Changes selfie sprites from INSTADOOM to be displayed with correct colors 
3. Addresses menu items and message drawing issue with Tartar that results in some of the colors being wrong for them

Loook for the WAD named KDiKDi_A.WAD in GOODIES\TAPE and put it into TAPE directory to be loaded by Tartar automatically with KDiKDiZD.

###### New kind of distribution package for Tartar

For the maintenance release I am introducing an experimental package that includes Tartar preconfigured and bundled with DosBOX for Windows and all the goodies and extras, all as a single ZIP file. Freedoom Phase II is included as IWAD and Community Chest 2 as a sample PWAD - drag it onto DOOM2.CMD to start playing immediately. This package includes works by other authors that are provided unmodified with readme and license files available in COPYRGHT directory (provided these were part of the said works). Look for Tartar-portable.zip in the binary distribution directory to try it out.

Note that:
I. None of the commercially available IWAD-s or PWAD-s are included.
II. An alternative build of Tartar is included in the portable package that does not support 640x400 resolution and switches to 640x480 resolution instead.


**Finally**, as a small bonus an alternative take on MAP01 of KDiKDiZD is included which allows the player to select difficulty level and jump to the ZM1 via a teleporter in the room. Load it after the mod itself this way:

   TARTAR -file KDiKDi_A KDiKDi_B KDiKDiM1 -warp 1

Be mindful of playing too much with that UAC terminal found in the map - _you have been warned_.



## Stage 3 Maintenance Release 2

### Sound

- Stereo image is more pronounced with stereo separation computation "copied" from Hexen source, 
  replacing original MBF code, and slight volume adjusment applied to sound sources behind the player
- WAV format sound effect lumps with "metadata" are now supported
- Native Doom format sound effect lumps with big-endian headers are now supported
  (these are found in some PWADs originating from Mac computers, in particular old versions of Wolfendoom)

### Gameplay changes and bugfixes

- Game no longer crashes sporadically after loading a WAD that references a non-existent flat lump 
- Intermission text is no longer cut off completely if a line which is too long to fit on the screen is encountered
- "Tall" sky textures (e.g. 256 pixel and higher) that have two "posts" per "column" are rendered without a "tutti-frutti" horizontal line
- "Tall" sky textures are no longer vertically misplaced 
- Dehacked patches for PWADs originating from Mac computers can now be read, e.g. those for Wolfendoom   
  (CR-terminated lines are supported and sporadic '\0' characters are ignored)
- Leaving map via a "death" (or "suicide") exit starts next level at "pistol start", does not leavel player in a "zombie" state
- New configuration propertry `hide_weapon_on_flash` has been added, designed to be used exclusively from OPTIONS lumps inside PWADs.   
  The value is a per-weapont-type bit mask (as per weapontype_t enum) that has set bit in case when weapon main sprite
  has to be suppressed when weapon flash is displayed.   
  The intended use is with compatibility WAD-s for Wolfendoom, where this addresses weapon sprite issues the PWADs have.
- Intermission text display has been shifted a few pixels to the left to accommodate for longer strings in some PWADS 
- New `intermusic` property has been added to MAPINFO to select particular song to play during intermission (outtro) screen
- New set of "boss action" properties has been added to MAPINFO following UMAPINFO standard as an example.   
  Code for these properties needs preprocessor definition `BOSSACTION` to be compiled in, which is set by default. 
- `bossaction_clear` when present and set to anything but `false` cancels any "boss action" on the map;   
  takes precedence over all other "boss action" properties
- `bossaction_thingtype` decimal [thing type](https://doomwiki.org/wiki/Thing_types) of the monster that acts as the boss of the map;   
  only a single type of boss per map is supported
- `bossaction_linespecial` [special line type](https://doomwiki.org/wiki/Linedef_type#Table_of_all_types)    
  corresponding to the effect to trigger when all the level bosses are dead
- `bossaction_tag` sector tag to apply the effect defined by `bossaction_linespecial` to;   
  only a single special sector per level is supported; vaue of **666** is used if omitted or in case of **0**

Here's an exmple of MAPINFO lump with these new properties:

    [level info]
    levelname=The Dam
    levelpic=CWILV05
    nextlevel=MAP07
    skyname=SKYOR
    partime=240
    music=THE_DA
    intermusic=DM2TTL
    inter-backdrop=WWATER
    endofgame=true
    intertext=M6TEXT
    bossaction-thingtype=16
    bossaction-linespecial=23
    bossaction-tag=69



### Extras

- `Hydrosphere.wad` "helper" WAD is included with the release. Look for it under GOODIES\TAPE directory.
- `KDiKDi_A.WAD` "helper" WAD has been updated to reflect better "tall" sky textures supported by the source port. 
- "Helper" WADs have been included for the following PWADs form [Wolfendoom PWAD Collection](http://lazrojas.com/wolfendoom),   
  that adress weapon sprite issues:
  + Fist Encounter (aka Wolfendoom)        - `wlfgfx.wad`   
    _also fixes Aftermath by Bruce Ryder_
  + Second Encounter                       - `2nd_enc.wad`
  + Treasure Hunt                          - `hunt.wad`
  + Die Fuehrer, Die                       - `die.wad`
  + Astrostein ep1                         - `astro.wad`
  + Astrostein ep2                         - `astro2.wad`
  + Astrostein ep3                         - `astro3.wad`
  + Totenhaus                              - `toten.wad`
  + Operation Eisenman                     - `eisen.wad`
  + Atctic Wolf (original)                 - `gfx1.wad` and `gfx2.wad`
  + Rheingold ep1                          - `rhein1.wad`
  + Rheingold ep2                          - `rhein2.wad`
  + Halten Sie                             - `halten.wad`

## Stage 3 Maintenance Release 3
_Note that this is documentation for a work in progress version of Tartar_

### Menus

- Loading game from empty slot is no longer possible
- When entering the name for a saved game, 
  player is presented for editing with the name 
  currently saved into the slot instead of a
  blank name. There's no longer a need
  to type in the name of the game from scratch
  every time the game is saved with the same name.
  Entering the name of a game being saved into
  an empty sleep slot starts with a blank name as 
  before.
- Empty savegame slots are shown as numbered by default
- Load and Save Game screens layout yas been adjusted,
  so that if a patch of non-standard height is used for 
  their title, names of saved games are not offset from
  the corresponding save game slot graphics. This in particular
  improves the looks in Mac versions of the Spear of Destiny,
  Original Missions and Nocturnal Missions from Wolfendoom
  series by Laz Rojas.

### Bugfixes 

- Loading of DeHackED patches no longer stops when 
  a line starting with '\\0' character is encountered.
  This allows loading Mac version of Portal PWAD from
  the Wolfendoom series by Laz Rojas
- Bugs have been fixed in loading of tall patches,
  building in-memory tall patch structures for mid textures,
  rendering of tall patches in skies, map objects and mid textures.
  Most apparently this results in giant chains now being drawn
  correctly in Eviternity MAP21.
- Bugs have been fixed in the tall patches workaround  function for 
  top, bottom and 1s mid textures. The scyscrapers in MAP02 of
  Jumpwad render without glitches now.
- Workaround for tall textures is no longer applied to textures 
  of less than 256 pixels. This condition existed previously,
  but was coded wrongly, resulting in rendering artifacts on some
  of the normal textures
- Bug has been fixed in offset computation for segments loaded
  from extended ZDoom format data. The most apparent change
  is that animated diamonds at the start of MAP03 of Jumpwad are 
  rendered correctly.
- Bug in handling of huge node tables has been fixed by 
  extending segment number reference from 16 to 32 bits in the
  subsector structure. As a result maps on the scale of MAP32 of 
  Eviternity have their geometry loaded and displayed correctly now.

### System

- DeepSea 32 bit format for NODES, SEGS and SUBSECTORS is now supprted, so all maps of Avactor (including MAP09 and MAP10) 
can be loaded
  
  