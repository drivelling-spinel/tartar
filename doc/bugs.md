# Known issues

- Dynamically adding sound effects from pwads with new names (e.g. from SMMU-style skins) 
  and addressing sound affects by string names does not work after move to MBF 2.0.4. sound routines
- Key bindings are somewhat wobbly and code is a mess after rearraning to allow bindable
  console commands in more game states
- Performance in NUTS.WAD is unacceptably poor straight from the start
- In MAP03 of DV.WAD standing across from the huge mural in the "cathedral" causes a drop
  in performance 
- Playing demo of MAP04 shipped with Deus Vult causes a crash or system hanging after 10-15 minutes
- TARTAR.EXE is a memory hog, especially in "real" MSDOS with CWSDPMI.EXE. 
  Loading Eviternity MAP19 or MAP24 in "real" MSDOS with CWSDPMI.EXE causes out of memory error.
  It works in Windows 98 and DOSBox with the same CWSDPMI.EXE 63Mb of memory.
- Demos in Eviternity play outright weirdly
- Demos in Phobos Anomaly: Reborn play go out of sync.
- ChexQuest demos go out of sync.
- With FILTERS directory filled with InstaDoom WAD-s Tartar exhausts DOS file handles very quickly,
  suggested CONFIG.SYS setting being FILES=70 
  
