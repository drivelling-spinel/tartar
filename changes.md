# Comprehensive list of changes

************************************************************************
This file contains the description of what has been changed in Tartar 
as compared to the sources of Caverns of Darkness that Joel Murdoch has 
made available as COD10SRC.ZIP. 

## Menu 

### Features menu item has been moved from main menu into Options
  
  This was done because vanilla TCs and Chex do not provide a patch
  for Features, and it looks very out of place as a result

### Currently selected item in options menu is now flashing
  
  Again, vanilla TCs and Chex use colored images for font glyphs 
  that dont's translate into one of the CR_... colors
  which makes selected menu item in Eternity options indistinguishable
  from other items

### Bugfix: entry of special characters in menus is no longer possible

  This was especially apparent if one used Shift when entering savegame
  name

### New entry "Load WAD" added to Episode selection menu

  By default Eternity would start new game not from E1M1 or MAP01
  but from the first changed map it detects if pwads with maps are 
  loaded. This means that loading Tech Gone Bad would result new
  game to start from E1M8, and if Sigil Compat is loaded, new games
  will start from E2M1 in Doom 1. Unfortunatelly, this is less useful
  for pwads that modify several episodes. For example Alients TC would 
  start from E1M1 and not E2M1 as one would expect. In Tartar, when 
  starting new Doom 1 game player can always select the desired episode. 
  There is also an additional entry in the menu Load WAD (named so because
  author didn't want to add new patches into the resource wad) that 
  will trigger default Eternity behaviour. With Doom 2, Final Doom and 
  Chex new games always start with the first changed map when a pwad
  with maps is loaded.

## Sound 

### Updated Alegro library and sound routines 

  [allegro.h](/allegro.h) in Tartar comes from the version of Allegro found in 
  [MBF 2.0.4](https://www.vogons.org/viewtopic.php?f=24&t=40857), and Tartar
  binaries are linked with liballegro produced from the sources that 
  the author of MBF 2.0.4 has shared. System dependent sound code has been
  replaced with that from MBF 2.0.4. This single change was the main driver 
  of the effort by the author, as he found previously available build of 
  Cavens of Darkness to be somewhat unstable when it came to sound and music 
  on a modern "Frankenstein" system in native DOS. 
  
  For those curious, target system for Tartar development was eqipped with
  a Yamaha YMF744B-R based PCI sound card and an X3MB MIDI synthesizer
  connector to the MPU-401 port of the said card. It was intended to be used
  in Windows 98 with VXD drivers by Yamaha and in DOS with DSDMA utility.

  Please refer to MBF 2.0.4 changelog included for the details on 
  Allegro changes. Tartar source code does not include modified Allegro 
  sources.

### Sound and MIDI card options have been removed from Sound options

  Following the "change in policy" from MBF 2.0.4 inclusion it is no
  longer possible to select sound card in Tartar options menu. Instead
  player should use a separate SETUP.EXE configuration utility. This is
  taken as is from MBF 2.0.4 in Tartar distribution and produces SETUP.CFG
  file (similar to ALLEGRO.CFG file that original ASETUP.EXE from Allegro
  library produced).

- Sound caching is no longer optional and option is gone from Sound options
  With MBF 2.0.4 lowlevel sound routines inclusion, sound effects are always
  cached at the start of the game. Option to control this is now gone.

## Video

### VESA-based lowlevel routines replace Allegro ones for video

  Low-level system video routines have been replaced by those by author  
  of [MBF 2.0.4](https://www.vogons.org/viewtopic.php?f=24&t=40857).
  The new routines use VESA API calls instead of Allegro and for improved
  video cards compatibility. 
  
  For the list of cards tested with MBF 2.0.4
  please consult the readmes for that port, however it should be noted that
  author was able to get high resolution modes work with the embedded
  video card by Intel that he had on the target system, which did not work
  with the DOS source ports he had previously tried. 

### Video mode selection options are gone

  It is no longer possible to directly select video mode in the options menu.
  Instead following MBF 2.0.4 example, video mode selection is done based
  on the set of options that player sets in options menu. These options are
  described in this section, and there is also a separate section of this 
  file with the list option combinations suitable for some specific cases.

### C Video, screenshots, renderer and graphics routines have been generalized

  All the routines that supported high resolution modes (i.e. 640x400 resolution
  in addition to 320x200) have been updated to support any resolution that
  is a power of 2 multiple of 320x200. In practical terms this means 1280x800
  is now supported, but in theory C functions could work with e.g. 2560x1600
  as well. 

  This is **not true** for Assembler functions, which have only been provided
  for the newly introduced 1280x800 renderer resolution. So adding new hi-res
  modes is sadly not just a matter of extending available configuration options. 

### Renderer resolution option has been added

  Player can choose to render to 320x200, 640x400 and 1280x800

  - 320x200   rendered image will be output in 320x200 resolution
  - 640x400   rendered image will be output into 640x400 resolution if supported
              otherwise will be output into 640x480 resolution 
              with black bars on top and bottom
  - 1280x800  rendered image will be output into 1280x1024 resolution
              with black bars on top and bottom

  The following CVAR values are supported for selecting resolution v_hires: 0, 1, 2.
  The same can be setup with hires configuration file option.

### Option for scaling to higher resolution has been added
  
  If activated the following scaling will be performed:

  - 320x200   rendered image will be scaled to 640x400 
              and output into 640x400 resolution if supported
              otherwise will be output into 640x480 resolution
              with black bars on top and bottom
  - 640x400   rendered image will be scaled to 1280x800 and output
              into 1280x1024 resolution with black bars on top and bottom
  - 1280x800  rendered image will not be scaled and still be output
              into 1280x1024 resolution with black bars on top and bottom

  Game resolution will be selected accordingly. No interpolation or filtering 
  are performed while scaling; each pixel is simply drawn as 2x2 block of the
  same color. 

  Scaling of screen buffer is done in RAM, after which scaled screen buffer
  contents are sent into video card's memory. This means that
  while less CPU intensive, scaling will still require at least one
  larger size screen buffer, and thus more memory than with scaling off
  at the same renderer resolution.

  CVAR to enable/disable scaling is v_scale_hi or with scale_to_hires
  option in the configuration file.

### Option for scaling to a 4:3 resolition has been added
  
  If activated without scaling to higher resolution being on:
  - 320x200   rendered image will not be changed
  - 640x400   rendered image will not be changed if video card supports 640x400 
              resolution, otherwise it will be scaled and output into 640x480 
              without black bars on top and bottom
  - 1280x800  rendered image will be scaled to 1280x960, and output 
              into 1280x1024 with black bars on top and bottom
              more narrow than when compared to 1280x800 image 
              
  No interpolation or filtering is performed while scaling.
  In fact scaling is performed by simply outputting some of the screen
  buffer lines twice. I.e. without scaling to higher resolution
  on some of the lines pixels will be drawn as 1x2 blocks of the same
  color. With scaling to higher resolution, on the same lines pixels
  will be drawn as 2x3 blocks of the same colors, rather then 2x2 
  blocks as on other lines.

  CVAR controlling this is v_scale_aspect, and this can be also set
  with scale_aspect option of the configuration file.

### Screenshot functions respect video scaling options
  
  Saving a screenshot will result in the same image as being output to the screen,
  as screenshot functions will perform the same scaling as screen output functions
  do.

### Video page flipping can be explicitly set in Video options
  
  As in MBF 2.0.4, the option to enable page flipping via VESA API call in 
  low-level vieo routines (taken from MBF 2.0.4) has been added. CVAR v_page_flip and 
  configuration file option page_flip can be used to control it. 

### Waiting for retrace now follows MBF 2.0.4 logic 
  
  Waiting for retrace (vsync) will be attempted in page flipped modes only,
  as this is the behaviour of the incorporated MBF 2.0.4 low level video routines.

### Option to show flashing disk icon when loading has been removed
  
  Unlike Allegro counterparts, MBF 2.0.4 low level vide routines do not have 
  generic purpose blitting functions, so rendring sprites at arbitrary moment in game 
  life cycle is no longer possible. One such case was the flashing disk/CD-ROM icon,
  and as it's no longer supported, option is gone for it from the menu.

### Option to show FPS counter has been added to Video options
  
  Low-level video routines from MBF 2.0.4 include code for FPS measurement.
  Unike MBF 2.0.4, Tartar uses SMMU HUD widget to render FPS values,
  so even if option is active, FPS counter will only be visible in game 
  and during demos, but not in title screen or menus. One can use 
  v_show_fps CVAR and show_fps configuration file option to switch FPS 
  output on or off.  

  If FPS counter is on, Tartar will also display current video settings
  (including actual mode that was configured) after they have been changed, 
  or for a few seconds after new level is started or saved game is loaded.
  These will also be shown in game only.

### Timed demo benchmarks have been removed from Video options menu 
  
  On one hand author has never been able to get them working, on the other
  these were referring to the benchmark scores tied to a certain set of 
  video modes which no longer made sense with the changes described above. 
  This change has only removed timed demos from Options menu. Running with 
  command line arguments should still be possible. 

### Command line options from MBF 2.0.4 have been added
  
  - -nolfb   avoid using LFB modes with VESA routines 
  - -nopm    avoid using PM VESA functions
  - -safe    try the most compatible settings for video card
  - -asmp6   choose P6-specific assembler optimized memory copy code 
             regardless of the detected CPU family

### Option to stretch skies is now persisted in configuration file
  
  Configuration file option stretchsky has been added to store it.

### Experimental "checkered translucency" has been added
  
  New translucency mode has been added that does not use tranmap file.
  Instead it renders either odd or even pixel of sprites and textures,
  depending on the number of line being drawn. This results in 50%
  translucency effect when used with intended equipment, e.g. a CRT
  VGA display at high enough resolution (1280x1024 would do).

  ![checkers.png](checkers.png)
  _Photo demonstrating the resulting effect_
  
  Column and span rendering functions have been provided as well 
  as functions for rendering sprites with translated colors and
  for particles. 
  
  One limitation is that translucency is not "additive", which means 
  that translucent objects act as opaque for other translucent objects
  and will block them. Also, in addition to applying this to all 
  objects in game for which general transparency applies, code applies 
  this to fuzz-ed objects, like Specter monster or player with invisibility 
  powerup. 
  
  ![comptran.png](comptran.png) 
  _comparison of no transparency, tranmap-based general transparency 
  and "checkered" transparency_  
  
  To use this mode first enable general translucency in the option
  menus, and then either set r_fauxtrans CVAR or change faux_translucency
  option in the configuration file.

### Experimental "water transluency" code has been compiled in

  The TRANWATER preprocessor define is set to On for compiling Tartar,
  which causes experimental ("no feature") code for rendering Boom-style  
  (i.e. line type 242) deep water surface as translucent to be called.
  Because this code results in nothing resembling translucent water surface, 
  it needs to be enabled explicitly with r_watertrans CVAR or 
  water_translucency option in the configuration file, both of which are 
  off by default. This will also only work if general translucency is  
  enabled in options.  

## Recommended video options for common use cases

### 1. Classic 320x200

Player has authentic retro or otherwise performance sensitive hardware,
or interest in classic vanilla/BOOM/MBF experience.

- Renderer resolution - 320x200
- Scale to higher resolution - No
- Aspect ratio to scale to - Original

320x200 resolution will be configured by Tartar.

![usecase1.png](usecase1.png)

### 2. Classic MBF hires

Player has access to slightly more powerful hardware including and a 
CRT display and videocard that supports 640x400

- Renderer resoltion - 640x400
- Scale to higher resolution - No
- Aspect ratio to scale to - Original

640x400 resolution will be configured by Tartar.

### 3. Classic MBF hires on a less compatilble computer

Player has access to slightly more powerful hardware including and a 
CRT display but their videocard only supports 640x480 and not 640x400

- Renderer resoltion - 640x400
- Scale to higher resolution - No
- Aspect ratio to scale to - 4:3

640x480 resolution will be configured by Tartar.

![usecase3.png](usecase3.png)

### 4. Gaming system with a "classic" LCD monitor attached

Player has a "classic" 1280x1024 display with 5:4 aspect ratio and
hadrware that is fast enough and has enough memory to handle this 
resolution.

- Renderer resoltion - 1280x800
- Scale to higher resolution - No
- Aspect ratio to scale to - 4:3

1280x1024 resolution will be configured by Tartar.

![usecase4.png](usecase4.png)


### 5. Square pixel appreciation on a more modern system

Player has a "classic" 1280x1024 display with 5:4 aspect ratio,
hadrware that is fast enought and has enough memory to handle this 
resolution, and an appreciation for sprites looking the same they would
look in a graphical editor had they loaded them.

- Renderer resoltion - 1280x800
- Scale to higher resolution - No
- Aspect ratio to scale to - Original

1280x1024 resolution will be configured by Tartar.

![usecase5.png](usecase5.png)

## Mouse changes

### Option to enable turning smoothing with mouse is now persisted
  
  Configuration file option for it is smooth_turning, same name as for CVAR.

### _Experimental_ smoother mouse turning
  
  Mouse turning smoothing takes 4 samples into account for _extra_
  smooth turning when smooth turning is activated in options menu.
  Previously 2 samples were used.

### _Experimental_ filtering of mouse movement
  
  Historically Eternity had been suffering from "jerky" mouse turning, as sharp 
  turns by the player could easily result in 360 (or more) degree turns in game. 
  Rather than fixing this at the source as SoM has done 
  in [late 2008](https://web.archive.org/web/20090110014229/http://som.mancubus.net/),   
  Tartar author has introduced filtering of exessive mouse readings
  to prevent unexpected turning behavior. This change is specific to Tartar and
  makes mouse turns in Tartar "feel" different from other ports. There is no
  configuration option or CVAR to disable it. 

## System

### Tartar always loads _eternity.wad_ as resource wad

  

## WADS compatibility

## Gameplay changes

### Option for blood re-coloring including "intelligent" mode has bee added
  
  Enemies menu in Tartar has option to enable re-coloring of blood splats into 
  other colors from the default red. Color can be set to blue, yellow or green.
  This will affect all blood splats and particle blood, but not gore, corpses,
  decorations or wall textures. Screen fade into red when player is in pain is 
  also not affected by this option. 
  
  Additionally intelligent mode is available (Auto in options) that:
  - changes all blood color to green in Chex 
  - re-colors Doom Barons and Knights blood color to green
  - re-colors Doom Cacodemons blood color to blue
  - retains default color for all other splats (which is red, unless replaced ny pwads)

  CVAR mon_bloodcolor controls blood re-coloring, values are 0-4, 0 corresponds
  to no re-coloring, 4 - to auto (intelligent mode). The configuration file option
  is bloodcolor.

## New CVARS

This section lists all the new console variables introduced in Tartar.
