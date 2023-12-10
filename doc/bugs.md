# Known issues

- Dynamically adding sound effects from pwads with new names (e.g.
  from SMMU-style skins) and addressing sound affects by string names
  does not work after move to MBF 2.0.4. sound routines
- Performance in NUTS.WAD is unacceptably poor straight from the start
- In MAP03 of DV.WAD standing across from the huge painting (or tapestry)
  in the "cathedral" causes a drop in performance 
- TARTAR.EXE is a memory hog, especially in higher resolutions.
  Running in Windows 98 is recommended with 65535 DPMI memory in shortcut
  properties. For DOSBox enabling full 63Mb of memory is recommended.
- With FILTERS directory filled with InstaDoom WAD-s Tartar exhausts DOS
  file handles very quickly, suggested CONFIG.SYS setting being FILES=70
- Loading a game saved on E1-type exit special sector crahses Tartar
- When loading a game where player skin was changed from marine, 
  player mobj sprite is retained, but player skin is set to marine 
- Map30 in Eviternity has some visual glitches (HOM) at the beginning 
- Jumping in Jumpwad loaded as PWAD results in red screen fade sometimes 
- Switching between palettes on the fly causes garbage colors on the screen 
  bor a brief moment if status bar logic causes palete to be changed
- Moving from map to map with MAP CCMD sometimes does not cause screen wipe,
  which leaves garbage on screen borders when view size is not covering all screen
- Switching to the "extra" screen size makes non-ingured mugshot to show for a 
  short period of time in status bar