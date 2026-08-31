#include "m3dinit.h"
#include "m3dcolij.h"
#include "validate.h"
#include "ob.h"

// @Ok
u16	Inquiry=0xFFFF;

// @Ok
SLineInfo gLineInfo;

i32 LineOfSightCheck;

u32 M3dColij_OneMask;
u32 M3dColij_ZeroMask;

i32 BaddyCollisionCheck;
i32 CameraCollisionCheck;
i32 TriggerCollisionCheck;

// @NotOk
// @FIXME - check ppc version to address this
i16 gUnkPose[1];

// guess: scratch rotation matrix, sits right before gLineInfo in the binary (0x5FBE18,
// gLineInfo is 0x5FBE38, exactly sizeof(MATRIX)=0x20 apart) but is not part of the
// SLineInfo struct itself (SLineInfo::WorldCst already validated at its own offset 0x48).
// No idb_globals.txt entry for this address, name and boundary are our guess.
static MATRIX * const gLineColijRotMatrix = (MATRIX*)0x005FBE18;

// guess: pointer to the current "eye" vector used for the line-of-sight GTE view setup.
// No idb_globals.txt entry, name is our guess.
#define G_CURRENT_COLIJ_VECTOR (*reinterpret_cast<SVECTOR**>(0x005FBD20))

// @Ok
// @Matching
// Return type fixed from void to i32 (the original returns 0 if pInfo->pItem is null,
// 1 otherwise).
i32 M3dColij_GetLineInfo(SLineInfo *pInfo)
{
	if (pInfo->pItem)
	{
		M3dMaths_RotMatrixYXZ((SVECTOR*)&pInfo->pItem->mAngles, gLineColijRotMatrix);
		gte_SetRotMatrix(gLineColijRotMatrix);
		gte_ldv0(G_CURRENT_COLIJ_VECTOR);
		gte_rtv0();
		gte_stsv((SVECTOR*)&pInfo->Normal);
		return 1;
	}

	return 0;
}

// @Ok
// @Matching
void M3dColij_LineInfoFixup(SLineInfo *pInfo)
{
	i32 v2 = M3dMaths_MulDiv64(pInfo->Distance, 0x4000, pInfo->Length);
	i32 v3 = pInfo->EndCoords.vx - pInfo->StartCoords.vx;
	i32 v5 = pInfo->EndCoords.vy - pInfo->StartCoords.vy;
	i32 v4 = pInfo->EndCoords.vz - pInfo->StartCoords.vz;

	i32 v8 = (v3 < 0 ? -1 : 1) * M3dMaths_MulDiv64(my_abs(v3), v2, 0x4000);
	i32 v6 = (v5 < 0 ? -1 : 1) * M3dMaths_MulDiv64(my_abs(v5), v2, 0x4000);
	i32 v7 = (v4 < 0 ? -1 : 1) * M3dMaths_MulDiv64(my_abs(v4), v2, 0x4000);

	pInfo->Position.vx = v8 + pInfo->StartCoords.vx;
	pInfo->Position.vy = v6 + pInfo->StartCoords.vy;
	pInfo->Position.vz = v7 + pInfo->StartCoords.vz;
}

// @Ok
// @Leak
// @Matching
void M3dColij_LineToItemZoned(CItem **ppItem,SLineInfo *pInfo)
{
	if	(!ppItem) return;

	if (pInfo->Length==0)
		return;

	gte_SetRotMatrix(&pInfo->WorldCst);

	M3dAsm_LineColijPreprocessItemsZoned(ppItem,0,pInfo,pInfo->Inquiry);

	for (	; *ppItem; ppItem++)
		if	((*ppItem)->mInquiry != pInfo->Inquiry)
		{
			(*ppItem)->mInquiry =	pInfo->Inquiry;


			M3dColij_LineToThisItem(*ppItem, pInfo);
		}
}

// @Ok
// @Leak
INLINE void NextInquiry(void)
{
	// increment inquiry
	// if it's 0, set to 1 and set all objects' inquiry fields to 0.
	// If this weren't done, a rare bug may occur where the previous collision test with a particular object was
	// performed 65536 tests ago, and the current test would automatically fail.
	if	(!++Inquiry)
	{
		CItem	*pItem;
		Inquiry=1;
		for (pItem=EnviroList; pItem; pItem=pItem->mNextItem)
			pItem->mInquiry=0;
		for (pItem=EnvironmentalObjectList; pItem; pItem=pItem->mNextItem)
			pItem->mInquiry=0;
	}
}


// @Ok
// Sets up a fresh SLineInfo for a line-vs-world collision query: resets the
// nearest-hit trackers, clears the hit result fields, builds the line's
// bounding box, builds the combined rotation matrix M3dAsm_LineColijPreprocessItems(Zoned)
// uses to transform vertices into line-aligned space, and hands out a fresh
// Inquiry token.
//
// The original builds the sin/cos-like terms via a leading-zero-count
// normalize (gte_ldlzc/gte_stlzc), a table-precision M3dMaths_SquareRoot0
// call on the normalized value, then a shift-back (PS2 hardware sqrt needs a
// normalized operand for full precision). Our M3dMaths_SquareRoot0 already
// does a real double sqrt with full precision (sqrt(x << 2k) == sqrt(x) << k
// for the even shift counts used there, so the normalize/shift-back cancels
// out algebraically), so we can call it directly on the plain squared
// distance and get the same result.
void M3dColij_InitLineInfo(SLineInfo *pInfo)
{
	pInfo->Distance = 0x7FFFFFFF;
	pInfo->tNear = 0x7FFFFFFF;

	pInfo->pItem = NULL;
	pInfo->pFace = NULL;
	pInfo->Model = -1;

	pInfo->RecordTriggerZoneHits = 0;
	pInfo->DropDown = 0;

	i32 dx = pInfo->EndCoords.vx - pInfo->StartCoords.vx;
	i32 dy = pInfo->EndCoords.vy - pInfo->StartCoords.vy;
	i32 dz = pInfo->EndCoords.vz - pInfo->StartCoords.vz;

	i16 sdx = (i16)(dx >> 12);
	i16 sdz = (i16)(dz >> 12);
	i16 sdy = (i16)(dy >> 12);

	i32 horiz2 = sdz * sdz + sdx * sdx;

	if (horiz2 != 0)
	{
		i32 horizMag = M3dMaths_SquareRoot0(horiz2);

		i16 sinYaw = (i16)((i32)sdx * 4096 / horizMag);
		i16 cosYaw = (i16)((i32)sdz * 4096 / horizMag);

		gLineColijRotMatrix->m[0][0] = cosYaw;
		gLineColijRotMatrix->m[0][1] = 0;
		gLineColijRotMatrix->m[0][2] = -sinYaw;
		gLineColijRotMatrix->m[1][0] = 0;
		gLineColijRotMatrix->m[1][1] = 4096;
		gLineColijRotMatrix->m[1][2] = 0;
		gLineColijRotMatrix->m[2][0] = sinYaw;
		gLineColijRotMatrix->m[2][1] = 0;
		gLineColijRotMatrix->m[2][2] = cosYaw;

		i32 full2 = horiz2 + sdy * sdy;
		i32 fullMag = M3dMaths_SquareRoot0(full2);

		pInfo->Length = fullMag;

		i16 sinPitch = (i16)((i32)sdy * 4096 / fullMag);
		i16 cosPitch = (i16)(horizMag * 4096 / fullMag);

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = cosPitch;
		pInfo->WorldCst.m[1][2] = -sinPitch;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = sinPitch;
		pInfo->WorldCst.m[2][2] = cosPitch;

		MulMatrix(&pInfo->WorldCst, gLineColijRotMatrix);
	}
	else
	{
		i16 sign;

		if (sdy >= 0)
		{
			sign = 4096;
			pInfo->Length = sdy;
			pInfo->DropDown = 1;
		}
		else
		{
			sign = -4096;
			pInfo->Length = -sdy;
		}

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = 0;
		pInfo->WorldCst.m[1][2] = -sign;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = sign;
		pInfo->WorldCst.m[2][2] = 0;
	}

	gte_SetRotMatrix(&pInfo->WorldCst);

	pInfo->MinCoords.vx = pInfo->StartCoords.vx < pInfo->EndCoords.vx ? pInfo->StartCoords.vx : pInfo->EndCoords.vx;
	pInfo->MaxCoords.vx = pInfo->StartCoords.vx < pInfo->EndCoords.vx ? pInfo->EndCoords.vx : pInfo->StartCoords.vx;

	pInfo->MinCoords.vy = pInfo->StartCoords.vy < pInfo->EndCoords.vy ? pInfo->StartCoords.vy : pInfo->EndCoords.vy;
	pInfo->MaxCoords.vy = pInfo->StartCoords.vy < pInfo->EndCoords.vy ? pInfo->EndCoords.vy : pInfo->StartCoords.vy;

	pInfo->MinCoords.vz = pInfo->StartCoords.vz < pInfo->EndCoords.vz ? pInfo->StartCoords.vz : pInfo->EndCoords.vz;
	pInfo->MaxCoords.vz = pInfo->StartCoords.vz < pInfo->EndCoords.vz ? pInfo->EndCoords.vz : pInfo->StartCoords.vz;

	NextInquiry();
	pInfo->Inquiry = Inquiry;
}

// @BIGTODO
// Original at 0x452C30, 1047 bytes. Checked in IDA (2026-08-31): needs real
// decompiles of leaf helpers that do not exist in the repo yet, before this
// function can be written for real (leaf-first rule):
//   - CVector::operator-(const CVector&, const CVector&) at 0x4E7760 and
//     CVector::operator>>(const CVector&, int) at 0x4E7840, both called
//     out-of-line here. These are the exact addresses already flagged in
//     CLAUDE.md as wrongly-INLINE in vector.h (repo-wide issue, not fixed
//     yet). Also calls sub_4E7870, sub_4E7800, sub_4E7680, sub_4E7590 in the
//     same 0x4E7xxx range, almost certainly more CVector operators.
//   - A cluster of unnamed GTE-area helpers around 0x46D6xx-0x46E0xx
//     (sub_46DD00, sub_46D790, sub_46D930, sub_46D700, sub_46DEB0,
//     sub_46DF60, sub_46D990, sub_46E010), none named or decompiled in the
//     repo. sub_46D430 is M3dMaths_SquareRoot0 (already @Ok).
// This is a real line-vs-sphere/body test (walks a body's item chain doing a
// bounding-box reject then a closest-point-on-segment distance check against
// each item's collision radius, offsets +8/+12/+16 = position, +220 =
// radius-ish u16, +70 bit 0x40 = some skip flag, +32 = next-item link).
// Leaving as @BIGTODO rather than guessing the helper bodies; not touched.
i32 M3dColij_LineToSphere(CVector*, CVector*, CVector*, CBody*, CBody*, i32)
{
	printf("i32 M3dColij_LineToSphere(CVector*, CVector*, CVector*, CBody*, CBody*, i32)");
	return 0x03072024;
}

// @BIGTODO
// Original at 0x4529C0, 617 bytes. Checked in IDA (2026-08-31): this does the
// real per-item line-vs-model collision test (picks a coarse/fine face table
// by radius, sets up the GTE transform for the item, calls into a
// spool/trigger-zone lookup and a Trig_TriggerCommandPoint dispatch on hit).
// Needs real decompiles of several unnamed leaf helpers before it can be
// written for real (leaf-first rule), the biggest being sub_46F1F0 (1176
// bytes) and sub_46F6B0 (359 bytes); also sub_46D810, sub_46D620, sub_46E4D0,
// sub_46DD40, none named or decompiled in the repo yet. Left as a
// forward-to-original stub (functionally correct at runtime, already used by
// the @Ok M3dColij_LineToItem/LineToItemZoned) rather than guessing the
// bodies of ~1600 bytes of undecompiled leaf code; not touched further.
void M3dColij_LineToThisItem(CItem* pItem, SLineInfo* pInfo)
{
	typedef void (*func_ptr)(CItem*, SLineInfo*);
	func_ptr func = (func_ptr)0x004529C0;

	func(pItem, pInfo);
}

// @Ok
// @Leak
// @Matching
void M3dColij_LineToItem(
		CItem* pItem,
		SLineInfo* pInfo)
{

	if	(!pItem)	return;

	if (pInfo->Length==0)
		return;

	gte_SetRotMatrix(&pInfo->WorldCst);
	M3dAsm_LineColijPreprocessItems(pItem, 0, pInfo, pInfo->Inquiry);

	for (	; pItem;	pItem=pItem->mNextItem)
		if	(pItem->mInquiry != pInfo->Inquiry)
		{
			pItem->mInquiry	= pInfo->Inquiry;
			M3dColij_LineToThisItem(pItem, pInfo);
		}
}

void validate_Vector(void)
{
	VALIDATE_SIZE(Vector, 0xC);

	VALIDATE(Vector, vx, 0x0);
	VALIDATE(Vector, vy, 0x4);
	VALIDATE(Vector, vz, 0x8);
}

void validate_SLineInfo(void)
{
	VALIDATE_SIZE(SLineInfo, 0xA4);

	VALIDATE(SLineInfo, StartCoords, 0x0);
	VALIDATE(SLineInfo, EndCoords, 0xC);


	VALIDATE(SLineInfo, MinCoords, 0x18);

	VALIDATE(SLineInfo, MaxCoords, 0x24);

	VALIDATE(SLineInfo, iLo, 0x30);
	VALIDATE(SLineInfo, iHi, 0x34);
	VALIDATE(SLineInfo, jLo, 0x38);
	VALIDATE(SLineInfo, jHi, 0x3C);

	VALIDATE(SLineInfo, Distance, 0x40);
	VALIDATE(SLineInfo, Length, 0x44);

	VALIDATE(SLineInfo, WorldCst, 0x48);

	VALIDATE(SLineInfo, pItem, 0x68);

	VALIDATE(SLineInfo, Position, 0x6C);

	VALIDATE(SLineInfo, Normal, 0x78);

	VALIDATE(SLineInfo, pFace, 0x80);
	VALIDATE(SLineInfo, Model, 0x84);

	VALIDATE(SLineInfo, RecordTriggerZoneHits, 0x88);
	VALIDATE(SLineInfo, DropDown, 0x89);

	VALIDATE(SLineInfo, Inquiry, 0x8A);
	VALIDATE(SLineInfo, tNear, 0x8C);

	VALIDATE(SLineInfo, tNumtrLo, 0x90);
	VALIDATE(SLineInfo, tNumtrHi, 0x94);
	VALIDATE(SLineInfo, tDenomLo, 0x98);
	VALIDATE(SLineInfo, tDenomHi, 0x9C);
	VALIDATE(SLineInfo, NormalOffset, 0xA0);
}

#include "my_patch.h"

// @Bogus
void patch_m3dcolij(void)
{
	PATCH_PUSH_RET(0x004527C0, M3dColij_LineToItem);
	PATCH_PUSH_RET(0x00452820, M3dColij_LineToItemZoned);
	PATCH_PUSH_RET(0x004528E0, M3dColij_LineInfoFixup);
}
