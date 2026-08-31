#include "zrhinog.h"
#include "export.h"

// @MEDIUMTODO
// Investigated 2026-08-31 (address 0x43A400, 106 instructions). This
// is bigger than SMALLTODO looked: it needs a new particle class we
// do not have yet. The original calls M3dUtils_GetDynamicHookPosition
// and Web_GetGroundY (both already in the repo), then a helper at
// 0x43A300 that spawns 20 dust particles. That helper allocates a
// 96 byte object (0x43A300 -> 0x4088A0(96)) and constructs it at
// 0x439E20. The constructor sets the vtable to off_53B7B0 (not
// CGLine's own vtable, and not CGLineParticle's, which is a
// different, already-implemented class at 0x413080 with a 4 arg
// ctor and 0x60 byte size). 0x439E20 calls CGLine::CGLine() first,
// then CGLine::SetRGB0/SetRGB1 (bit2.cpp), so it looks like a
// CGLine-derived class, tentatively "CPingLine" (names.json's own
// guess for this address), that does not exist in the repo. Its
// exact field layout (beyond CGLine) and its Move()/destructor are
// unknown. Also uses an unnamed data table at word_610C48 (footstep
// offset table, size/shape unknown). Not implementing without
// guessing the new class, per repo instructions. Leaving as TODO.
void Effects_RhinoStomp(CSuper *)
{
	printf("Effects_RhinoStomp");
}