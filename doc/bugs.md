# Known issues

- Dynamically adding sound effects from pwads with new names (e.g. from SMMU-style skins) and addressing sound affects by string names does not work after move to MBF 2.0.4. sound routines
- Key bindings are somewhat wobbly and code is a mess after rearraning to allow bindable console commands in more game states
- Performance in NUTS.WAD is unacceptably poor straight from the start
- In MAP03 of DV.WAD standing across from the huge mural in the "cathedral" causes a drop in performance 
- Playing demo of MAP04 of DV.WAD shipped with the PWAD causes a crash or system hanging after 10-15 minutes
- TARTAR.EXE is a memory hog, especially in "real" MSDOS with CWSDPMI.EXE. _Probably_ because it uses calloc() to allocate the screen buffers as a linear area in memory, switching resolutions while in game causes a crash from time to time. This does not happen in Windows 98.
- Loading Eviternity maps in "real" MSDOS with CWSDPMI.EXE causes crashes even for MAP01. It works in DosBox on Windows 10 or in Windows 98 even with MAP19 or MAP24 and MAP31.
- Demos in Eviternity play outright weirdly, in some other PWAD-s play with desyncs.
- ChexQuest demos play with desyncs.
