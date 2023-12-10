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
- Demos in Eviternity play outright weirdly.
- Demos in Phobos Anomaly: Reborn go out of sync.
- Demos in Phobos Anomaly: Reborn go out of sync.
- ChexQuest demos go out of sync.
- With FILTERS directory filled with InstaDoom WAD-s Tartar exhausts DOS
  file handles very quickly, suggested CONFIG.SYS setting being FILES=70
- Weird texturing effects have been noticed in some maps e.g. diamonds 
  at the start of Jumpwad MAP03 or huge chains in Eviternity MAP21 do not
  render right
- Math seems to go crazy on some of Eviternity maps (MAP15, MAP29, MAP30 
  and MAP32) breaking rendering completely.
- Top and bottom textures with tall patches sometimes render with glitches; 
  this seems to be relevant for when very tall objects are in proximity, 
  see for example skyscrapers in Jumpwad MAP02 if IDCLIP-ed to close enough.
- No special handling of tall patches in some cases - e.g.  UI graphics;
  in fact only things and textures rendering code is  aware that tall patches exist.
- Tartar crashes when loading Avactor MAP09.
- Eternity MAP11 has sky rendering artifacts 
  _although I think I meant Eviternity when taking notes_
- Switching weapons while shooting crashes game
- Original 1st encounter Wolfendoom sounds are played at incorrect sampling rate
- Weapon in SSG slot has glitchy animation in Rheingold and (original) Arctic Wolf 
