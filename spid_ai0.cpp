#include "spid_ai0.h"

// @BIGTODO
// Original at 0x4B13F0. Investigated 2026-08-31, re-checked 2026-09-01,
// and mapped structurally 2026-09-01 (this note) with idalib against the
// real SpideyPC.exe.
//
// Identity is confirmed, names.json is right here: the maintainer's IDB
// (idbs/spideypc_names.txt line 1652) also calls 0x4B13F0 "SpideyAI0",
// and the Mac build has the matching symbol .SpideyAI0__FP7CPlayer
// (idbs/spiderman_names.txt). tools/prototypes.json gives the Mac size as
// 32080 bytes against 29402 (0x72DA) here, so it is the same function,
// slightly tighter on PC.
//
// Size: 0x72DA bytes (matches tools/functions/4920304.bin exactly), 7655
// instructions, 1482 basic blocks, a 0x4A4 byte stack frame holding
// roughly fifty CVector / CSVector temporaries, and an SEH frame
// (SEH_4B13F0).
//
// STRUCTURE (new this session, the earlier note did not have it). This is
// NOT one flat blob, it is a one-hot state machine:
//
//   * The main dispatch is at 0x4B211F..0x4B215D on CPlayer field 0xE1C,
//     which holds a single set bit (a state id, not a small ordinal):
//         value  > 0x10000  -> 0x4B50AE
//         value == 0x10000  -> 0x4B4E9B
//         value  > 0x100    -> 0x4B307C
//         value == 0x100    -> 0x4B2F80
//         otherwise switch(value-1), a 128 entry jump table at
//         jpt_4B215D with the byte index table at byte_4B86EC. Only the
//         powers of two are real cases: 1, 2, 4, 8, 16, 64 and 128.
//         Everything else falls into the shared default def_4B215D,
//         which is also where every state's "break" jumps back to (that
//         is why 130+ `jmp def_4B215D` show up in the disassembly).
//     So a port can be built one state at a time: pick a bit, decompile
//     that one case, leave the rest to the original.
//
//   * A second, smaller switch sits at 0x4B7971..0x4B7987, after the
//     state machine: it reads the 16 bit field at +0x38 of an object
//     pointer (CItem::mType, per ob.cpp's VALIDATE list) and dispatches
//     21 cases over types 304..324, with 308/309/311/316/318/319/321-323
//     falling to the default. This is the "what am I holding / standing
//     on" tail, not part of the state machine.
//
//   * Before the state machine, the head (0x4B13F0..0x4B211F) is a flat
//     per-frame prologue: it sets CPlayer+0xAE4 = 1, reads the keyboard
//     mappings for actions 0x40 and 0x100 through
//     PCINPUT_GetKeyboardMappingForAction, has a CurrentSuit (0x5559DC)
//     == 5 special case, applies the pending water effect (gWaterEffect
//     0x60FA9C -> Db_SkyColor + Db_UpdateSky, the same globals
//     trig.cpp's WaterEffectOn / SetSkyColor commands write), and counts
//     down several CPlayer timers.
//
// BLOCKERS, still true after this pass. 155 unique callees; 120 have a
// real name in tools/names.json, but several the dispatch reaches
// directly are still unfinished in this repo, so a partial translation
// would call printf placeholders every frame while the player is being
// driven: CPlayer::CheckKick (@MEDIUMTODO),
// CPlayer::UpdateAndTrackCombo (@MEDIUMTODO),
// CPlayer::FireWeb(bool,i32,CVector*,bool,CSVector*) (@MEDIUMTODO),
// CPlayer::DrawOffscreenSpideySenseIndicatorList (@MEDIUMTODO),
// CPlayer::SetupLookaroundCamera (0x4C38A0, 3674 bytes, @NotOk, its own
// known hard blocker per PLAN.md). Another 34 callees (a cluster around
// 0x4B8B70-0x4B8C70 plus scattered ones in 0x4C0000-0x4C9000) have no
// name at all yet, not even in the maintainer's IDB.
//
// Recommended order for whoever picks this up: finish the five named
// stubs above, name the 0x4B8B70-0x4B8C70 cluster, then port state bits
// one at a time starting with the smallest case, keeping this stub for
// the states that are not done. Do not attempt it in one pass: a wrong
// translation here runs every frame and drives all of Spider-Man's
// movement and combat.
void SpideyAI0(CPlayer *)
{
    printf("SpideyAI0(CPlayer *)");
}

