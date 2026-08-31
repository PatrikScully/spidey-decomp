#include "m3dutils.h"
#include "validate.h"

#include "spool.h"
#include "decomp.h"

#include "my_assert.h"

// @Ok
// @Matching
void M3dUtils_ReadLinksPacket(CSuper* pSuper, void* pPacket)
{
	i32 NumJoints = reinterpret_cast<u16*>(pPacket)[1];
	pSuper->mpLinks = reinterpret_cast<SLink*>(reinterpret_cast<i32>(pPacket) + 4);

	pSuper->mpPoseBuffer = static_cast<SMatrix*>(Mem_New(sizeof(SMatrix) * G_PSXREGION[pSuper->mRegion].NumParts));

	pSuper->mpJoints = static_cast<SJoint*>(Mem_New(sizeof(SJoint) * NumJoints));

	for (i32 i = 0; i < NumJoints; i++)
	{
		pSuper->mpJoints[i].Displacement.vz = 0;
		pSuper->mpJoints[i].Displacement.vy = 0;
		pSuper->mpJoints[i].Displacement.vx = 0;

		pSuper->mpJoints[i].Angles.vz = 0;
		pSuper->mpJoints[i].Angles.vy = 0;
		pSuper->mpJoints[i].Angles.vx = 0;
	}

	for (i32 j = 0; j < NumJoints; j++)
	{
		i32 k;
		for (k = 0; k < NumJoints; k++)
		{
			if (pSuper->mpLinks[j].ParentPart == pSuper->mpLinks[k].Part)
			{
				pSuper->mpLinks[j].ParentLink = k;
				break;
			}
		}

		if (k == NumJoints)
		{
			pSuper->mpLinks[j].ParentLink = 0xFFFF;
		}
	}
}

// @Ok
// @Matching
void M3dUtils_InBetween(CSuper *pSuper)
{
	u32 *pAnimFile = G_PSXREGION[pSuper->mRegion].pAnimFile;
    i32 Interval =  (pAnimFile[2 * pSuper->mAnim + 2] >> 16) + 1;
    
	if (Interval != 1)
	{
		i32 NumParts = G_PSXREGION[pSuper->mRegion].NumParts;
		ASSERT(NumParts <= 30, "Too many parts for TweenBuffer");

		M3dUtils_InterpolateVectors(
				4 * NumParts,
				Interval,
				pAnimFile,
				pSuper,
				0,
				NumParts);
	}
}

// @BIGTODO
void M3dUtils_BuildPose(CSuper* pSuper)
{
	typedef void (*func_ptr)(CSuper*);
	func_ptr func = (func_ptr)0x00454450;
	func(pSuper);
}


// @BIGTODO
void M3dUtils_InterpolateVectors(i32 NumVectors, i32 Interval, u32* pAnimFile, CItem* pItem, i32 Part, i32 NumParts)
{
	typedef void (*func_ptr)(i32, i32, u32*, CItem*, i32,i32);

	func_ptr func = (func_ptr)0x00454270;
	func(NumVectors, Interval, pAnimFile, pItem, Part, NumParts);
}

// Scratch buffer the animation system tweens part-local pose matrices into
// when the current anim frame does not land exactly on a keyframe (see the
// "Too many parts for TweenBuffer" ASSERT in M3dUtils_InBetween and the
// M3dUtils_InterpolateVectors write target). Fixed game address, confirmed
// from the original disassembly of M3dUtils_InterpolateVectors,
// M3dUtils_GetHookPosition and M3dUtils_GetDynamicHookPosition, all of
// which reference 0x5FC250 directly. Sized for up to 30 parts (SMatrix, 24
// bytes each).
static SMatrix* const gTweenBuffer = reinterpret_cast<SMatrix*>(0x5FC250);

// @Ok
void M3dUtils_GetHookPosition(VECTOR* pOut, CSuper* pSuper, i32 hookIndex)
{
	SHook* pHook = &G_PSXREGION[pSuper->mRegion].pHooks[hookIndex];
	u16 PartIndex = pHook->Offset;
	u16 Anim = pSuper->mAnim;

	// mirror the part-0 column of the world transform while this hook is
	// resolved, then flip it back at the end (see the matching block below)
	if (pSuper->mExtraFlags & 2)
	{
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}

	SMatrix* pPoseFrame;

	if (pSuper->mFlags & 4)
	{
		if (pSuper->mpPoseBuffer == NULL)
		{
			pOut->vx = 0;
			pOut->vy = 0;
			pOut->vz = 0;
			return;
		}

		pPoseFrame = pSuper->mpPoseBuffer + PartIndex;
	}
	else
	{
		u32* pAnimFile = G_PSXREGION[pSuper->mRegion].pAnimFile;
		u32 IntervalWord = pAnimFile[2 * Anim + 2];

		if (IntervalWord & 0xFFFF0000)
		{
			M3dUtils_InterpolateVectors(
					4,
					(IntervalWord >> 16) + 1,
					pAnimFile,
					pSuper,
					PartIndex,
					G_PSXREGION[pSuper->mRegion].NumParts);

			pPoseFrame = gTweenBuffer + PartIndex;
		}
		else
		{
			pPoseFrame = Decomp_GetAnimTransform(pSuper) + PartIndex;
		}
	}

	gsub_46F820(pHook, pPoseFrame, &pSuper->mTransform);
	gte_stlvnl(pOut);

	pOut->vx <<= 12;
	pOut->vy <<= 12;
	pOut->vz <<= 12;

	pOut->vx = pSuper->mPos.vx + ((pOut->vx - pSuper->mPos.vx) >> 4);
	pOut->vy = pSuper->mPos.vy + ((pOut->vy - pSuper->mPos.vy) >> 4);
	pOut->vz = pSuper->mPos.vz + ((pOut->vz - pSuper->mPos.vz) >> 4);

	if (pSuper->mExtraFlags & 2)
	{
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}
}

// @BIGTODO
void M3dUtils_GetDynamicHookPosition(VECTOR*, CSuper*, SHook*)
{
	printf("void M3dUtils_GetDynamicHookPosition(VECTOR*, CSuper*, SHook*)");
}

// @Ok
// @Matching
void M3dUtils_ReadHooksPacket(CSuper* pSuper, void* pPacket)
{
	G_PSXREGION[pSuper->mRegion].pHooks = reinterpret_cast<SHook*>(reinterpret_cast<i32>(pPacket) + 4);
}

void validate_SHook(void)
{
	VALIDATE_SIZE(SHook, 0x8);

	VALIDATE(SHook, Part, 0x0);
	VALIDATE(SHook, Offset, 0x6);
}


#include "my_patch.h"

// @Bogus
void patch_m3dutils(void)
{
	PATCH_PUSH_RET(0x00453C30, M3dUtils_ReadHooksPacket);
	PATCH_PUSH_RET(0x00454200, M3dUtils_InBetween);

	PATCH_PUSH_RET(0x00453C50, M3dUtils_ReadLinksPacket);
}
