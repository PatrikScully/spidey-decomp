#include "l6a1lsc.h"

#include "spidey.h"
#include "baddy.h"
#include "spool.h"

// @MEDIUMTODO
void L6A1LSC_MonitorSpideyinWater(u32 const *,u32 *)
{
    printf("L6A1LSC_MonitorSpideyinWater(u32 const *,u32 *)");
}

// @Ok
// @Matching
void L6A1LSC_RelocatableModuleClear(void)
{
}

// @Ok
// @Matching
void L6A1LSC_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = L6A1LSC_RelocatableModuleClear;
	pMod->field_C[0] = L6A1LSC_MonitorSpideyinWater;
	Spidey_SetUserFunction("l6a1lsc", 1u);
}

// @Ok
// @Matching
EXPORT i32 obtainWaterLevelInPoolL6A1(i32 level)
{
	i32 model;

	if (level == 0)
	{
		model = Spool_GetModel(0x6A6967FF, gObjFileRegion);
	}
	else if (level == 1)
	{
		model = Spool_GetModel(0x9E0EE08E, gObjFileRegion);
	}
	else
	{
		// forces a real reload from the parameter's stack home instead of the
		// already-cached register, matches the original exactly (see attempts file)
		model = *(volatile i32*)&level;
	}

	for (CItem *item = EnvironmentalObjectList; item; item = item->mNextItem)
	{
		if (item->mType == 0x192 && item->mRegion == gObjFileRegion)
		{
			switch (level)
			{
				case 0:
					if (item->mModel == model)
						return item->mPos.vy >> 12;
					break;
				case 1:
					if (item->mModel == model)
						return item->mPos.vy >> 12;
					break;
			}
		}
	}

	print_if_false(0, "Pool object not found");
	return 0;
}
