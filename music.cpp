#include "music.h"

#include <stdio.h>

// @MEDIUMTODO
// 0x00459650, 1696 bytes. This translation unit holds exactly one function.
// The Mac build proves it: its symbol table has a music.cpp TU with
// Music_MusicUpdate(void) (1800 bytes) as its only entry, and
// tools/prototypes.json lists it under "music".
//
// What the original does, read from the disassembly:
//  - returns straight away while gPostWaterEffect or the training flag
//    (0x60CFB0) is set, and remembers the vblank count it last ran on so it
//    can work out how many vblanks passed since the previous call;
//  - switches on Trig_GetLevelId() and, per level, picks a pair of tables out
//    of the block at 0x54D868..0x54DE74: a list of SFX ids and a list of
//    vblank durations. It walks that list with the vblank delta and starts
//    each track with SFX_Play(id | 0xC000, 0x2000, 0);
//  - runs the fight music on top of that through the CPlayer fields at
//    0x528/0x52C/0x530/0x538 and 0xC5C/0xC60/0xC64/0xC68/0xC69, fading the
//    voice up and down with SFX_SetVoiceVolume.
//
// Left as a stub on purpose: the whole thing is table driven and the roughly
// fifteen id/duration tables have to be pulled out of the exe and checked
// before it can be written. Guessing them would play the wrong music with no
// crash and no compile error, which the plan says not to do. Logic, Display
// and PlayAway (main.cpp) already call it in the right places.
void Music_MusicUpdate(void)
{
	printf("Music_MusicUpdate(void)");
}
