# Known issues

- Dynamically adding sound effects from pwads with new names (e.g.
  from SMMU-style skins) and addressing sound affects by string names
  does not work after move to MBF 2.0.4. sound routines
- When loading a game where player skin was changed from marine, 
  player mobj sprite is retained, but player skin "is set to" marine 
- Jumping in Jumpwad loaded as PWAD results in red screen fade sometimes 
- Moving from map to map with MAP CCMD sometimes does not cause screen wipe,
  which leaves garbage on screen borders when view size is not covering all screen
- Mouse sensitivity is not adjusted by the screen buffer scaling option
  so users have to adjust it manually if they change the scale
- Tartar is not compatible with original COD.EXE savegames. While loading
  in principle works, game crashes eventually. It is also not compatible with
  COD10MBF.EXE build published in fall 2021, funny artifacts occurring game
  is loaded
- Time spent in level is cut off in automap for WAD-s with wider fonts (e.g. 
  Eviternity).
- When PWAD defining custom episodes via EPISINFO is loaded with CHEX.WAD IWAD, 
  episode selection menu shows 4 built-in episodes, instead of 1
- When game mode is shareware, registered or retail, 
  episode is set to end after map 8, regardless of MAPINFO details
- Tartar only uses _standard named_ lumps for level name graphics 
  when displaying map name below "Entering" in the tally screen. 
  This is in contrast to displaying MAPINFO-defined graphic above 
  "Finished".
- Aspect ratio correction malfunctions in VGA Mode X, so this feature is disabled 
  unless one of high resolution options is chosen

