# Known issues

- Dynamically adding sound effects from pwads with new names (e.g.
  from SMMU-style skins) and addressing sound affects by string names
  does not work after move to MBF 2.0.4. sound routines
- Multipost single patch textures that are less than 255 pixel high are 
  rendered with hoizontal line artifacts when not used as mid-textures
- Loading a game saved on E1-type exit special sector crahses Tartar
- When loading a game where player skin was changed from marine, 
  player mobj sprite is retained, but player skin "is set to" marine 
- Map30 in Eviternity has some visual glitches (HOM) at the beginning 
- Jumping in Jumpwad loaded as PWAD results in red screen fade sometimes 
- Moving from map to map with MAP CCMD sometimes does not cause screen wipe,
  which leaves garbage on screen borders when view size is not covering all screen
- Mouse sensitivity is not adjusted by the screen buffer scaling option
  so users have to adjust it manually if they change the scale
- STAGES\STAGE5 is mislabeled in the distro
- Credits screen says the version is Stage 5 Testing, when it's not
- Tartar is not compatible with original COD.EXE savegames. While loading
  in principle works, game crashes eventually. It is also not compatible with
  COD10MBF.EXE build published in fall 2021, funny artifacts occurring game
  is loaded
- Sigil is auto-loaded after Sigil II. Should be the other way around. Also
  auto-loading capability is not effectiv for non-LFN systems.
- Berserk and pain states use red colormaps in Chex Quest
- After option for random music is switched off while in game, restarting 
  current level is not sufficient to go back to the original track. 
  In case of the first level, starting new game is not sufficuent either. 
  To go back to the original track warping to another level and warping back 
  or restarting the game executable is nceessary
