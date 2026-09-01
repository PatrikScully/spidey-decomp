#include "spid_ai0.h"

// @BIGTODO
// Investigated 2026-08-31, re-checked 2026-09-01 with idalib against
// the real SpideyPC.exe (address 0x4B13F0, named "SpideyAI0" in
// tools/names.json). Confirmed size: 0x72DA bytes (29402, matches
// tools/functions/4920304.bin exactly), 7655 instructions, 1482
// basic blocks, a 0x4C4 byte stack frame with dozens of CVector /
// CSVector locals. This is Spidey's whole player-control state
// machine (movement, combat, swinging, wall crawl, etc).
//
// Callee check (2026-09-01): 155 unique callees. 120 already have a
// real name in tools/names.json, but the direct dispatch still hits
// several callees that are themselves unfinished stubs, so a partial
// translation would just call into printf placeholders mid-frame:
// CPlayer::CheckKick (@MEDIUMTODO), CPlayer::UpdateAndTrackCombo
// (@MEDIUMTODO), CPlayer::FireWeb(bool,i32,CVector*,bool,CSVector*)
// (@MEDIUMTODO), CPlayer::DrawOffscreenSpideySenseIndicatorList
// (@MEDIUMTODO), CPlayer::SetupLookaroundCamera (0x4C38A0, 3674
// bytes, still @NotOk, its own known hard blocker per PLAN.md). The
// other 34 callees (cluster around 0x4B8B70-0x4B8C70, plus scattered
// ones in the 0x4C0000-0x4C9000 range) have no name at all yet, not
// even in IDA's own analysis, so their behavior is unknown.
//
// Given a wrong translation here runs every frame and drives real
// player control, and given several direct dependencies are not
// implemented yet, this is not tractable as a single pass. Not a
// leaf function per the repo's leaf-first rule. Leaving as @BIGTODO
// with this callee map for whoever picks it up next: implement the
// still-unnamed cluster and the five listed stubs first, then this
// function becomes a state-machine dispatch over already-real code.
void SpideyAI0(CPlayer *)
{
    printf("SpideyAI0(CPlayer *)");
}

