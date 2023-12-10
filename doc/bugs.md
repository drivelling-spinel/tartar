# Known issues

- Dynamically adding sound effects from pwads with new names (e.g.
  from SMMU-style skins) and addressing sound affects by string names
  does not work after move to MBF 2.0.4. sound routines
- Performance in NUTS.WAD is unacceptably poor straight from the start
- In MAP03 of DV.WAD standing across from the huge painting (or tapestry)
  in the "cathedral" causes a drop in performance 
- TARTAR.EXE is a memory hog, especially in higher resolutions.
  Running in Windows 98 is recommended with 65535 DPMI memory in shortcut
  properties. For DOSBox enablimg full 63Mb of memory is recommeded.
- Demos in Eviternity play outright weirdly.
- Demos in Phobos Anomaly: Reborn go out of sync.
- ChexQuest demos go out of sync.
- With FILTERS directory filled with InstaDoom WAD-s Tartar exhausts DOS
  file handles very quickly, suggested CONFIG.SYS setting being FILES=70
- Weird texturing effects have been noticed in some maps with extended
  nodes tables - e.g. diamonds at the start of Jumpwad MAP03 do not
  render right
- Tall patches rendering is not supported. Both sprites and textures
  that use patches with vertical extents of 255 pixels and more will
  render incorrectly. See Eviternity MAP27 for an example of both.
- Extremely big maps like Eviternity MAP32 would not render correctly.
- Trying to detect the keyboard in SETUP.EXE gives ... 
  "error reading keyboard.dat".
  
