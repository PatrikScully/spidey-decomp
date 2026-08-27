#include "l5a7lsc.h"

#include "spidey.h"
#include "trig.h"
#include "spool.h"
#include "baddy.h"

// current water surface height (world units, shifted by 12), used by all
// the water level scripts (l5a6lsc, l5a7lsc, l6a1lsc, l6a2lsc, l6a3lsc) and
// read by M3d_Render / M3d_RenderSetup / CBaddy_ExecuteCommand. Move this to
// a shared header once one of those other callers is decompiled.
static i32 * const gWaterLevel = (i32*)0x005FCDA8;

// frame counter for how long Spidey has been below the pool's water level;
// only referenced from this function.
static i32 * const gL5A7DrowningTimer = (i32*)0x005FB754;

// @Ok
// @AlmostMatching: MSVC6 emits "add eax,[ecx+80h]" for
// "*gL5A7DrowningTimer += MechList->field_80;" in the original, our build
// emits "mov ecx,[eax+80h]" then "lea eax,[ecx+edx]" (one extra 3 byte
// instruction, MechList in eax not ecx). Everything before and after this
// single spot matches exactly (all remaining diffs are this one
// instruction shifting every later offset by 3 bytes). Tried 15 source
// shapes for this one line without changing the codegen: plain compound
// assign, split into a named "timer" local, operand order swapped (field
// first vs timer first), declaring a "mech" local for MechList first,
// declaring "timer" local first, embedding the assignment inside the if
// condition, gL5A7DrowningTimer[] indexing instead of *deref, a volatile
// timer pointer, reinterpret_cast to CBody*, a separate "delta" local, a
// pointer-to-field local, and swapping the early return for a wrapping
// if-block. Residue: 54 mnemonic diffs on cmpsum, all downstream of this
// one instruction.
void L5A7LSC_MonitorSpideyinWater(const u32 *,u32 *)
{
	MechList->mFlags &= ~8;

	i32 x = MechList->mPos.vx >> 12;
	i32 z = MechList->mPos.vz >> 12;
	i32 y = MechList->mPos.vy >> 12;

	if (x <= -3600 || x >= -900 || z <= 3300 || z >= 5000 || y <= 2700 || y >= 4700)
		goto resetTimer;

	MechList->mFlags |= 8;

	{
		u32 Model = Spool_GetModel(0xC50DC421, gObjFileRegion);

		CBody *pool = EnvironmentalObjectList;
		while (pool)
		{
			if (pool->mType == 0x192 && pool->mRegion == gObjFileRegion && pool->mModel == Model)
				break;

			pool = reinterpret_cast<CBody*>(pool->mNextItem);
		}

		i32 waterLevel;
		if (pool)
		{
			waterLevel = pool->mPos.vy >> 12;
		}
		else
		{
			print_if_false(0, "Pool object not found");
			waterLevel = 0;
		}

		*gWaterLevel = waterLevel << 12;

		if (y <= waterLevel)
			goto resetTimer;
	}

	*gL5A7DrowningTimer += MechList->field_80;

	if (*gL5A7DrowningTimer > 20)
	{
		u8 found = 0;

		for (i32 i = 1; i < NumNodes; i++)
		{
			if (*G_OFFSETLIST[i] == 1)
			{
				CVector v;
				Trig_GetPosition(&v, i);

				if (v.vx == 0xFEAB8000 && v.vy == 0x532000 && v.vz == 0xFE330000)
				{
					found = 1;
					*gL5A7DrowningTimer = 0;
					Trig_SendPulseToNode(i);
					MechList->mPos = v;
					break;
				}
			}
		}

		print_if_false(found, "No TRG_Drowning node");
	}
	return;

resetTimer:
	*gL5A7DrowningTimer = 0;
}

// @Ok
// @Matching
void L5A7LSC_RelocatableModuleClear(void)
{
}

// @Ok
// @Matching
void L5A7LSC_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = L5A7LSC_RelocatableModuleClear;
	pMod->field_C[0] = L5A7LSC_MonitorSpideyinWater;
	Spidey_SetUserFunction("l5a7lsc", 1u);
}

// not a real PC function. There is no separate address for it in
// names.json; the compiler fully inlined its body into
// L5A7LSC_MonitorSpideyinWater (the Pool object search loop there). Mac has
// it as its own function (obtainWaterLevelInPool, 176 bytes), PC does not.
// Leaving this stub as is, same as downloadTexture in PCTex.cpp.
// @SMALLTODO
EXPORT void obtainWaterLevelInPoolA7(i32)
{
    printf("obtainWaterLevelInPool(i32)");
}
