# Known issues

- Dynamically adding sound effects from pwads with new names (e.g.
  from SMMU-style skins) and addressing sound affects by string names
  does not work after move to MBF 2.0.4. sound routines
- Loading a game saved on E1-type exit special sector crahses Tartar
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
- After option for random music is switched off while in game, restarting 
  current level is not sufficient to go back to the original track. 
  In case of the first level, starting new game is not sufficuent either. 
  To go back to the original track warping to another level and warping back 
  or restarting the game executable is nceessary
