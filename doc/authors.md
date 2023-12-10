# Acknowledgements

Tartar is based on previously released work by other authors.
Please refer to COPYRGHT directory for the collection of licences
covering their work listed below.

1. Doom is a rather scary hellish 3-D game, source code for which has 
   been [released](COPYRGHT/DOOMLIC.TXT) by ID Software.

2. MBF (Marine's Best Friend) is a port of Doom source code to DOS
   based on BOOM (another DOS source port by the famous modding group
   Team TNT). MBF up to version 2.0.3 was developed by Lee Killough.

3. MBF 2.0.4 is a contemporary [maintenance release](COPYRGHT/MBFUP204.TXT)
   developed by G. "Gerwin" Broers that improves MBF stability and 
   compatibility with video and audio hardware. 

4. SMMU (Smack My Marine Up) was a source port by Simon "Fraggle"
   Howard that built on the original MBF. SMMU version 3.21 served
   as a basis for [Eternity Engine](COPYRGHT/ee331b7-dos.txt) - 
   game engine for Eternity TC project.

5. Eternity Enigine also contained portions of code from
   [ZDoom by Randy Heit](COPYRGHT/zdoomcode-license.txt). 

6. Starting with version [3.29 developer beta 5](eternity/ee329db5.txt),
   Eternity Engine contributors lead by James "Quasar" Haley started
   removing from the code the features related to the TC. These were 
   expected to come back eventually as EDF externalized versions, but 
   this has never materialized and TC was eventually cancelled.

7. Version 3.29 developer beta 5 was used as a basis for 
   custom game engine of Caverns of Darkness TC by the Chaos Crew.
   Engine was developed by Joel Murdoch and source code for it 
   has [been released](COPYRGHT/cod10src.txt) in 2020.    

8. All of the source ports above depend on Allegro library as a wrapper 
   for DOS platform-specific system, audio and video calls. They all use
   build tools and standard libraries from [DJGPP](COPYRGHT/COPYING.DJ)
   by DJ Delorie. 

Tartar combines source code of Caverns of Darkness TC engine by Joel Murdoch
with MBF 2.0.4 platform specific code and Allegro library updated by Gerwin
to bring extra stability and hardware compatibility to the early beta 
version of Eternity Enigne, thus making it and historic content that was built
to be run with it, more accessible to players with modern hardware, 
should they choose to run DOS or Windows 9X on that hardware for retro gaming.
It is built with DJGPP following the process explained by Gerwin for MBF 2.0.4. 

As Eternity Engine constitutes the majority of Tartar's code, a collection
of [documents](eternity) from various publicly available sources that describe 
the state of Eternity circa 3.29db5-joel2 is included with Tartar's source.  
