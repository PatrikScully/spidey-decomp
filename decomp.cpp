#include "decomp.h"
#include "ob.h"
#include "ps2funcs.h"

// @BIGTODO
// forward to original. This resolves the whole per-frame animation
// hierarchy for pSuper: LOD model selection, building the part
// calculation order from the hierarchy data, decompressing the pose
// matrices for every part, and applying the root part's rotation. Over
// 1200 bytes in the original, uses a calculation-order array
// (pSuper->mpCalculationOrder) not yet built anywhere in the repo, and
// calls several still-unnamed GTE helpers (sub_433A60/DecompressStream,
// sub_4581A0, sub_46E730, sub_46E0F0, sub_46DA10, sub_46E270/MTC2). Left
// as a forward until that subsystem is decompiled; used by
// M3dUtils_GetHookPosition/M3dUtils_GetDynamicHookPosition/M3dUtils_BuildPose.
SMatrix* Decomp_GetAnimTransform(CSuper* pSuper)
{
	typedef SMatrix* (*func_ptr)(CSuper*);
	func_ptr func = (func_ptr)0x00433D60;
	return func(pSuper);
}
