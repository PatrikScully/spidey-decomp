First tagged build of the standalone game.

What works: the game boots to the menus, loads level 1, renders the city and
runs the player physics the same way the original game does (checked frame by
frame against the original running under Wine). Sound, music and the movies
are still stubs. Expect bugs.

You need your own copy of Spider-Man (2000) for PC. Copy data.pkr,
media.pkr, texture.dat and SpideyPC.exe into one directory and pass that
directory to the game. See the README for each system.

Files:

- spidey-vX-linux-x86-sdl3.tar.gz: the standalone game for Linux (32 bit x86,
  SDL3 and OpenGL), with the libSDL3 it needs.
- spidey-vX-windows-binkw32.zip: the Phase 1 DLL. Rename the game's
  binkw32.dll to binkw32_.dll and put this one in its place.
- spidey-vX-source.tar.gz and .zip: the sources at this tag.
