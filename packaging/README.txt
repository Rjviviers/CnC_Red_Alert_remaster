Command & Conquer: Red Alert - multiplayer port (development build)
=====================================================================

This is an automatic build from the project's CI. It is a work in progress: it may not reach
the main menu yet. Full instructions: docs/playing.md in the source repository.

You need your own Red Alert data files
--------------------------------------
This download contains only the program. The game data (the .MIX files) is not included and
is not allowed to be: copy it from your own copy of Red Alert, for example the
C&C Ultimate Collection or the 2008 freeware release.

Required:  REDALERT.MIX, MAIN.MIX
Optional:  EXPAND.MIX (Counterstrike), EXPAND2.MIX and HIRES1.MIX (Aftermath)

Upper/lower case of the file names does not matter.

Running
-------
Either copy the .MIX files into this folder, or point the game at the folder that holds them:

  Windows:  ra95.exe -data="C:\Games\Red Alert"
  Linux:    ./ra95 -data=/path/to/redalert

or set Path= in the [Data] section of REDALERT.INI (the game creates that file next to itself
on its first run, so this folder must be writable).

If something is missing, the game says which files it looked for and where, and exits.
Messages are also written to the terminal (stderr).

Linux: the program is 32-bit (x86). On 64-bit Debian/Ubuntu install the 32-bit libraries:
  sudo dpkg --add-architecture i386 && sudo apt update
  sudo apt install libc6:i386 libstdc++6:i386 libgcc-s1:i386 libx11-6:i386 libxext6:i386 \
       libxrandr2:i386 libxcursor1:i386 libxi6:i386 libxfixes3:i386 libasound2t64:i386
(on releases before Ubuntu 24.04 / Debian 13 the last one is called libasound2:i386).
Then: chmod +x ra95 (if needed) and run it as above.

Windows: the Microsoft Visual C++ runtime DLLs the program needs are included next to it.

License: GPLv3 with additional terms; see LICENSE.md in the source repository.
