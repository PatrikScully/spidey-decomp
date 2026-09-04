Second tagged build of the standalone game.

Level 1 now plays from the intro cutscene into gameplay, and you can move
Spider-Man with the keyboard. Sound, music and the movies are still stubs.
Expect bugs.

Fixed since v0.0.1 (every fix checked against the original game running
under Wine):

- The arrow keys move Spider-Man. The player code cleared all pad buttons
  every frame, so no key press ever reached him.
- The mouse is not inverted any more. The two axes were swapped.
- Black Cat shows up in her scene. Her aim was mirrored, so she walked away
  from Spider-Man, and the shadow code overwrote her body matrix, so she was
  drawn lying flat on the roof.
- The difficulty menu has its box and the little Spider-Man again.
- The window opens at 2x (1280x960). Set SPIDEY_SCALE=1 for the old size.
- The HUD is not sheared any more (health bar, compass needle and arrow).
- Menu and viewer models do not explode any more (anim decoder with a block
  size of 1).
- Spider-Man does not get stuck on a wall seam and fall to his death in the
  intro climb (collision test for the second triangle of a quad).
- Most buildings of level 1 were missing, they are drawn now.
- The sky sits on the camera instead of the world origin, and the colours are
  not washed out (the brightness table was signed).
- The title and legal screens had red and blue swapped.
- The game ran twice as fast at 60 fps, it is held to 30 now.

You need your own copy of Spider-Man (2000) for PC. Copy data.pkr,
media.pkr, texture.dat and SpideyPC.exe into one directory and pass that
directory to the game. See the README for each system.

Files:

- spidey-vX-linux-x86-sdl3.tar.gz: the standalone game for Linux (32 bit x86,
  SDL3 and OpenGL), with the libSDL3 it needs.
- spidey-vX-windows-binkw32.zip: the Phase 1 DLL. Rename the game's
  binkw32.dll to binkw32_.dll and put this one in its place.
- spidey-vX-source.tar.gz and .zip: the sources at this tag.
