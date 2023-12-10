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
- ChexQuest demos go out of sync.
- With FILTERS directory filled with InstaDoom WAD-s Tartar exhausts DOS
  file handles very quickly, suggested CONFIG.SYS setting being FILES=70
- Weird texturing effects have been noticed in some maps e.g. diamonds 
  at the start of Jumpwad MAP03 or huge chains in Eviternity MAP21 do not
  render right
- Extremely big maps like Eviternity MAP32 would not render correctly.
- Loading game with -load command line argument crashes the game with 
  Eviternity
- Top and bottom textures with tall patches sometimes render with glitches;
  this seems to be relevant for when very tall objects are in proximity,
  see for example skyscrapers in Jumpwad MAP02 if IDCLIP-ed to close enough
- Tall patches are not supported by Tartar for UI graphics and such
  
