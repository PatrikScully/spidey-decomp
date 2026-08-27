#include "l5a6lsc.h"
#include "spidey.h"
#include "ob.h"
#include "spool.h"
#include "baddy.h"

// @MEDIUMTODO
void L5A6LSC_MonitorSpideyinWater(const u32 *,u32 *)
{
    printf("L5A6LSC_MonitorSpideyinWater(u32 const *,u32 *)");
}

// @Ok
// @Matching
void L5A6LSC_RelocatableModuleClear(void)
{
}

// @Ok
// @Matching
void L5A6LSC_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = L5A6LSC_RelocatableModuleClear;
	pMod->field_C[0] = L5A6LSC_MonitorSpideyinWater;
	Spidey_SetUserFunction("l5a6lsc", 1u);
}

// @Ok
// @Matching
EXPORT i32 obtainWaterLevelInPool(i32 poolIndex)
{
	u32 model;

	if (poolIndex == 0)
		model = Spool_GetModel(0x8D1EE7F8, gObjFileRegion);
	else if (poolIndex == 1)
		model = Spool_GetModel(0x6A6967FF, gObjFileRegion);
	else if (poolIndex == 2)
		model = Spool_GetModel(0xD7B2D512, gObjFileRegion);
	else
		model = Spool_GetModel(0xD7B2D512, gObjFileRegion);

	CBody *node = EnvironmentalObjectList;

	while (node)
	{
		u8 region = gObjFileRegion;

		if (node->mType == 0x192 && region == node->mRegion)
		{
			switch (poolIndex)
			{
				case 0:
				case 1:
					if (node->mModel == model)
						goto found;
					break;

				case 2:
					if (node->mModel == model)
					{
						i32 x = node->mPos.vx >> 12;
						i32 z = node->mPos.vz >> 12;

						if (x > -360 && x < 1600 && z > 6800 && z < 10900)
							goto found;
					}
					break;

				case 3:
					if (node->mModel == model)
					{
						i32 x = node->mPos.vx >> 12;
						i32 z = node->mPos.vz >> 12;

						if (x > -360 && x < 1600 && z > -2100 && z < 2000)
							goto found;
					}
					break;
			}
		}

		node = reinterpret_cast<CBody*>(node->mNextItem);
	}

	print_if_false(0, "Pool object not found");
	return 0;

found:
	return node->mPos.vy >> 12;
}
