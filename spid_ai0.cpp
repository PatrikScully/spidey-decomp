#include "spid_ai0.h"

// @BIGTODO
// Investigated 2026-08-31 (address 0x4B13F0, per idb_globals /
// spideypc_names.txt from the maintainer's newer IDB, not in
// tools/names.json). This is far bigger than MEDIUMTODO: 7655
// instructions, a 0x4C4 byte stack frame with dozens of CVector /
// CSVector locals. This is Spidey's whole player-control state
// machine (movement, combat, swinging, wall crawl, etc), calling
// dozens of other functions, most not yet decompiled in the repo.
// Not a leaf function and not tractable as a single pass. Leaving
// as TODO, retagged to reflect real size.
void SpideyAI0(CPlayer *)
{
    printf("SpideyAI0(CPlayer *)");
}

