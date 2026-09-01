#include "web.h"
#include "m3dinit.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "ps2m3d.h"
#include "spidey.h"
#include "baddy.h"
#include "utils.h"
#include "camera.h"
#include "spool.h"
#include "decomp.h"
#include <string.h>

extern i16 gRotMatrix[3][3];

#include "validate.h"

i32 gFireDomes;
i32 gNumDomes;
CBody* WebList;

EXPORT i32 gGetGroundDefaultValue;

extern CBody* MiscList;

// Second table parallel to CItemRelatedList (ob.h, 0x6B2454), indexed the same way
// (region*17). Web_CollideWithSuper (0x4F7AE0) is the only known reader: it reads a hook
// COUNT from this table's per-region entry at offset+8
// (`*(DWORD*)(dword_6B2458[region*17]+8)`). No idb_globals.txt entry; name and every field
// beyond that one offset are our guess.
static i32 *** const gWebHookCountTable = (i32***)0x006B2458;

// @Ok
// 2026-08-31: functional decompile (session bar). Address 0x4F7680. Previously undeclared and
// unimplemented anywhere in the repo (tools/names.json's own tentative name). Self-contained
// axis-aligned-box-vs-line-segment clip test, independent of any mesh/SModel struct knowledge
// (all four parameters are raw SVECTOR triples): a coarse per-axis trivial-reject first, then
// 6 unrolled face-plane clips (classic fully-unrolled PS1-era collision macro expansion,
// confirmed instruction-by-instruction against the 0x4F7680 disasm). Returns 1 and overwrites
// *pEnd with the exact crossing point on the first face the segment actually crosses INSIDE
// the box's other two dimensions; returns 0 if the segment never crosses the box.
i32 BoundingBoxCollisionCheck(SVECTOR const *pMin, SVECTOR const *pMax, SVECTOR const *pStart, SVECTOR *pEnd)
{
	if (pStart->vy > pMin->vy && pEnd->vy > pMin->vy)
		return 0;

	if (pStart->vy < pMax->vy && pEnd->vy < pMax->vy)
		return 0;

	if (pStart->vx > pMin->vx && pEnd->vx > pMin->vx)
		return 0;

	if (pStart->vx < pMax->vx && pEnd->vx < pMax->vx)
		return 0;

	if (pStart->vz > pMin->vz && pEnd->vz > pMin->vz)
		return 0;

	if (pStart->vz < pMax->vz && pEnd->vz < pMax->vz)
		return 0;

	i32 dy = pEnd->vy - pStart->vy;
	i32 dz = pEnd->vz - pStart->vz;
	i32 dx = pEnd->vx - pStart->vx;

	// Face: x == pMin->vx
	if (pStart->vx > pMin->vx && pEnd->vx < pMin->vx)
	{
		i32 y = pStart->vy + dy * (pMin->vx - pStart->vx) / dx;
		i32 z = pStart->vz + dz * (pMin->vx - pStart->vx) / dx;

		if (y < pMin->vy && y > pMax->vy && z < pMin->vz && z > pMax->vz)
		{
			pEnd->vx = pMin->vx;
			pEnd->vy = static_cast<i16>(y);
			pEnd->vz = static_cast<i16>(z);
			return 1;
		}
	}

	// Face: x == pMax->vx
	if (pStart->vx < pMax->vx && pEnd->vx > pMax->vx)
	{
		i32 y = pStart->vy + dy * (pMax->vx - pStart->vx) / dx;
		i32 z = pStart->vz + dz * (pMax->vx - pStart->vx) / dx;

		if (y < pMin->vy && y > pMax->vy && z < pMin->vz && z > pMax->vz)
		{
			pEnd->vx = pMax->vx;
			pEnd->vy = static_cast<i16>(y);
			pEnd->vz = static_cast<i16>(z);
			return 1;
		}
	}

	// Face: y == pMin->vy
	if (pStart->vy > pMin->vy && pEnd->vy < pMin->vy)
	{
		i32 x = pStart->vx + dx * (pMin->vy - pStart->vy) / dy;
		i32 z = pStart->vz + dz * (pMin->vy - pStart->vy) / dy;

		if (x < pMin->vx && x > pMax->vx && z < pMin->vz && z > pMax->vz)
		{
			pEnd->vx = static_cast<i16>(x);
			pEnd->vy = pMin->vy;
			pEnd->vz = static_cast<i16>(z);
			return 1;
		}
	}

	// Face: y == pMax->vy
	if (pStart->vy < pMax->vy && pEnd->vy > pMax->vy)
	{
		i32 x = pStart->vx + dx * (pMax->vy - pStart->vy) / dy;
		i32 z = pStart->vz + dz * (pMax->vy - pStart->vy) / dy;

		if (x < pMin->vx && x > pMax->vx && z < pMin->vz && z > pMax->vz)
		{
			pEnd->vx = static_cast<i16>(x);
			pEnd->vy = pMax->vy;
			pEnd->vz = static_cast<i16>(z);
			return 1;
		}
	}

	// Face: z == pMin->vz
	if (pStart->vz > pMin->vz && pEnd->vz < pMin->vz)
	{
		i32 x = pStart->vx + dx * (pMin->vz - pStart->vz) / dz;
		i32 y = pStart->vy + dy * (pMin->vz - pStart->vz) / dz;

		if (x < pMin->vx && x > pMax->vx && y < pMin->vy && y > pMax->vy)
		{
			pEnd->vx = static_cast<i16>(x);
			pEnd->vy = static_cast<i16>(y);
			pEnd->vz = pMin->vz;
			return 1;
		}
	}

	// Face: z == pMax->vz
	if (pStart->vz >= pMax->vz)
		return 0;

	if (pEnd->vz <= pMax->vz)
		return 0;

	i32 x = pStart->vx + dx * (pMax->vz - pStart->vz) / dz;
	i32 y = pStart->vy + dy * (pMax->vz - pStart->vz) / dz;

	if (x >= pMin->vx || x <= pMax->vx || y >= pMin->vy || y <= pMax->vy)
		return 0;

	pEnd->vx = static_cast<i16>(x);
	pEnd->vy = static_cast<i16>(y);
	pEnd->vz = pMax->vz;
	return 1;
}

// @Ok
// 2026-08-31: functional decompile (session bar), address 0x4F7AE0 (1126 bytes). Supersedes
// the earlier "investigated, left as a stub" note this function used to carry (still visible
// in git history): BoundingBoxCollisionCheck (above) is now implemented, and the GTE
// world-to-local rotate is the same idiom Utils_RotateWorldToObject (utils.cpp) already uses,
// just run four times: once for pStart/pEnd against pSuper's own matrix, then again per hook
// against that hook's own local matrix, read straight out of Decomp_GetAnimTransform's
// SMatrix array (decomp.h/ob.h). SMatrix::m is bit-compatible with MATRIX::m (both i16[3][3],
// the only field M3dMaths_TransposeMatrix1 touches), confirmed by the disasm's own inner-loop
// call to it with a SMatrix* (0x4F7AE0's v16) in place of a MATRIX*.
//
// The one thing still not independently confirmed anywhere else in the repo is the exact
// record CItemRelatedList[region*17] (ob.h) points at per hook: only the one offset actually
// read here is used (+12: a packed, per-axis-interleaved i16[6] bounding box -- min0,max0,
// min1,max1,min2,max2, confirmed both by BoundingBoxCollisionCheck's own two-SVECTOR-corner
// signature and by the disasm's own stack packing of the 6 values into two interleaved
// triples). Reproduced below as raw offset arithmetic rather than invented as a named struct,
// since naming fields never read here would be a guess this repo's bar explicitly forbids.
// gWebHookCountTable (this file, own comment) is the same kind of parallel, only-partially-
// understood table.
//
// Walks pSuper's hook table for its region, skipping hook 0 and any hook whose bit is clear
// in pSuper->field_194 (CSuper, ob.h). For each remaining hook: offsets the pSuper-local
// segment by the hook's own SMatrix::t (>>4), rotates that offset segment again by the hook's
// own SMatrix::m (transposed), scales the hook's bounding box by a5/4096 (or takes it
// verbatim when a5==4096), and tests the doubly-local segment against that box with
// BoundingBoxCollisionCheck. Among every hook whose box the segment actually crosses, keeps
// the one whose hit point is closest to the segment start (Euclidean distance via
// gte_ldlvl+gte_sqr0+gte_stlvnl+M3dMaths_SquareRoot0, the same idiom M3dColij_LineToSphere
// already uses, m3dcolij.cpp) and fills pOut with that hook's hit position (scaled back up by
// <<4) and pOut->Offset with the winning hook's bit index. Returns 1 if any hook was found,
// 0 otherwise (pOut->Offset != -1).
i32 Web_CollideWithSuper(CSuper *pSuper, CVector const *pStart, CVector const *pEnd, SHook *pOut, i32 a5)
{
	MATRIX superMatrix;
	M3dMaths_TransposeMatrix1(&pSuper->mTransform, &superMatrix);
	memcpy(gRotMatrix, &superMatrix, sizeof(gRotMatrix));

	SVECTOR localStart;
	localStart.vx = static_cast<i16>((pStart->vx - pSuper->mPos.vx) >> 12);
	localStart.vy = static_cast<i16>((pStart->vy - pSuper->mPos.vy) >> 12);
	localStart.vz = static_cast<i16>((pStart->vz - pSuper->mPos.vz) >> 12);

	MTC2(*reinterpret_cast<i32*>(&localStart.vx), GT_ZERO);
	MTC2(*reinterpret_cast<i32*>(&localStart.vz), GT_ONE);
	gte_mvmva(1, 0, 0, 3, 0);
	gte_stsv(&localStart);

	SVECTOR localEnd;
	localEnd.vx = static_cast<i16>((pEnd->vx - pSuper->mPos.vx) >> 12);
	localEnd.vy = static_cast<i16>((pEnd->vy - pSuper->mPos.vy) >> 12);
	localEnd.vz = static_cast<i16>((pEnd->vz - pSuper->mPos.vz) >> 12);

	MTC2(*reinterpret_cast<i32*>(&localEnd.vx), GT_ZERO);
	MTC2(*reinterpret_cast<i32*>(&localEnd.vz), GT_ONE);
	gte_mvmva(1, 0, 0, 3, 0);
	gte_stsv(&localEnd);

	pOut->Offset = -1;

	u32 bestDist = 0xFFFFFFFFu;

	SMatrix *pHookMatrix = Decomp_GetAnimTransform(pSuper) + 1;

	i32 regionBase = pSuper->mRegion * 17;
	i32 **pRegionHooks = CItemRelatedList[regionBase] + 1;
	i32 hookCount = *reinterpret_cast<i32*>(reinterpret_cast<char*>(gWebHookCountTable[regionBase]) + 8);

	i32 bit = 2;

	for (i32 hookIndex = 1; hookIndex < hookCount; hookIndex++, bit <<= 1, pHookMatrix++, pRegionHooks++)
	{
		if (pSuper->field_194 & bit)
			continue;

		i16 offX = static_cast<i16>(pHookMatrix->t[0] >> 4);
		i16 offY = static_cast<i16>(pHookMatrix->t[1] >> 4);
		i16 offZ = static_cast<i16>(pHookMatrix->t[2] >> 4);

		SVECTOR hookStart;
		hookStart.vx = static_cast<i16>(localStart.vx - offX);
		hookStart.vy = static_cast<i16>(localStart.vy - offY);
		hookStart.vz = static_cast<i16>(localStart.vz - offZ);

		SVECTOR hookEnd;
		hookEnd.vx = static_cast<i16>(localEnd.vx - offX);
		hookEnd.vy = static_cast<i16>(localEnd.vy - offY);
		hookEnd.vz = static_cast<i16>(localEnd.vz - offZ);

		MATRIX hookMatrixT;
		M3dMaths_TransposeMatrix1(reinterpret_cast<MATRIX*>(pHookMatrix), &hookMatrixT);
		memcpy(gRotMatrix, &hookMatrixT, sizeof(gRotMatrix));

		MTC2(*reinterpret_cast<i32*>(&hookStart.vx), GT_ZERO);
		MTC2(*reinterpret_cast<i32*>(&hookStart.vz), GT_ONE);
		gte_mvmva(1, 0, 0, 3, 0);
		gte_stsv(&hookStart);

		MTC2(*reinterpret_cast<i32*>(&hookEnd.vx), GT_ZERO);
		MTC2(*reinterpret_cast<i32*>(&hookEnd.vz), GT_ONE);
		gte_mvmva(1, 0, 0, 3, 0);
		gte_stsv(&hookEnd);

		i16 *pBox = reinterpret_cast<i16*>(reinterpret_cast<char*>(*pRegionHooks) + 12);

		SVECTOR boxMin;
		SVECTOR boxMax;

		if (a5 == 4096)
		{
			boxMin.vx = pBox[0];
			boxMax.vx = pBox[1];
			boxMin.vy = pBox[2];
			boxMax.vy = pBox[3];
			boxMin.vz = pBox[4];
			boxMax.vz = pBox[5];
		}
		else
		{
			boxMin.vx = static_cast<i16>((a5 * pBox[0]) >> 12);
			boxMax.vx = static_cast<i16>((a5 * pBox[1]) >> 12);
			boxMin.vy = static_cast<i16>((a5 * pBox[2]) >> 12);
			boxMax.vy = static_cast<i16>((a5 * pBox[3]) >> 12);
			boxMin.vz = static_cast<i16>((a5 * pBox[4]) >> 12);
			boxMax.vz = static_cast<i16>((a5 * pBox[5]) >> 12);
		}

		SVECTOR hitPoint = hookEnd;

		if (BoundingBoxCollisionCheck(&boxMin, &boxMax, &hookStart, &hitPoint))
		{
			CVector delta;
			delta.vx = hookStart.vx - hitPoint.vx;
			delta.vy = hookStart.vy - hitPoint.vy;
			delta.vz = hookStart.vz - hitPoint.vz;

			gte_ldlvl(reinterpret_cast<VECTOR*>(&delta));
			gte_sqr0();

			VECTOR deltaSq;
			gte_stlvnl(&deltaSq);

			u32 dist = static_cast<u32>(M3dMaths_SquareRoot0(deltaSq.vx + deltaSq.vy + deltaSq.vz));

			if (dist < bestDist)
			{
				bestDist = dist;
				pOut->Part.vx = static_cast<i16>(hitPoint.vx << 4);
				pOut->Part.vy = static_cast<i16>(hitPoint.vy << 4);
				pOut->Part.vz = static_cast<i16>(hitPoint.vz << 4);
				pOut->Offset = static_cast<i16>(hookIndex);
			}
		}
	}

	return pOut->Offset != -1;
}

// @Ok
CDomeShockWave::CDomeShockWave(i32 a2)
{
	this->mType = 8;
	this->mPos = MechList->mPos;
	this->mPos.vy += 204800;

	this->field_44.vx = 240;
	this->field_44.vy = 240;
	this->field_44.vz = 240;

	this->field_90 = a2;

	this->ResetHitFlags(BaddyList);
	this->ResetHitFlags(EnvironmentalObjectList);

	for (i32 i = 0; i < 16; i++)
	{
		this->field_50[i] = Rnd(4096);
	}

	CameraList->Shake(this->mPos, CAMERASHAKE_MEDIUM);
}

// @Ok
CDomeShockWave::~CDomeShockWave(void)
{
}

// @Ok
CDomePiece::CDomePiece(
		CVector* a2,
		i32 a3,
		i32 a4,
		i32 a5)
{
	this->mPos = *a2;
	this->field_F8 = a4;

	if (a5)
		this->InitItem("firedome");
	else
		this->InitItem("webdome3");

	print_if_false(a3 < reinterpret_cast<u32*>(PSXRegion[this->mRegion].ppModels)[-1], "Bad Model sent to CDomePiece");

	this->mModel = a3;
	this->AttachTo(&MiscList);
	this->mFlags |= 0x400;
	this->mRGB = 0;
	this->field_FC = 4;
}

// @Ok
CDomePiece::~CDomePiece(void)
{
	this->DeleteFrom(&MiscList);
}

// @Ok
CDome::CDome(
		CPlayer* pSpidey,
		i32 a3)
{
	print_if_false(pSpidey != 0, "NULL pSpidey");
	this->hPlayer = Mem_MakeHandle(pSpidey);

	this->mPos = pSpidey->mPos;

	this->mPos.vy += pSpidey->field_EA8 << 12;
	this->field_104 = a3;

	if ( a3 )
	{
		this->InitItem("firedome");
		this->mModel = 1;
		gFireDomes++;
		this->mFlags |= 0x200;
		this->mScale.vy = 0;
	}
	else
	{
		this->InitItem("webdome2");
	}

	this->mFlags |= 1;
	this->AttachTo(&MiscList);
	this->field_100 = 0;
	++gNumDomes;
}

// @Ok
CDome::~CDome(void)
{
	this->DeleteFrom(&MiscList);

	delete this->field_108;
	delete this->field_10C;
	delete this->field_110;
	delete this->field_114;
	delete this->field_118;

	if (this->field_104)
		gFireDomes--;
	gNumDomes--;
}

// The single live CDomeRing. Written by CDomeRing's constructor (which
// deletes the previous ring first) and cleared by its destructor, which also
// asserts it still points at itself. Only web.cpp touches 0x6B559C, apart
// from a level-teardown reset at 0x45566A, so it stays file-local here. No
// idb_globals.txt entry, the name is ours.
static CDomeRing ** const gpCurrentDomeRing = (CDomeRing**)0x006B559C;

// @Ok
// 0x4F5510, 494 bytes. Builds the expanding shockwave ring that a dome leaves
// behind when it pops. Only one may exist, so any previous one is deleted
// first. Saves the ring model's vertices into field_F8 (the destructor puts
// them back, because CDomeRing animates the SHARED region model in place) and
// precomputes a per-vertex outward direction into field_FC.
//
// Original defect kept: field_FC[3*i + 1] (the y component of the direction)
// is never written, so every ring runs off whatever the freshly allocated
// block happened to hold there.
CDomeRing::CDomeRing(const CVector *pPos, i32 bFire)
{
	if (*gpCurrentDomeRing != 0)
		delete *gpCurrentDomeRing;

	*gpCurrentDomeRing = this;

	this->field_10C = bFire;

	if (bFire != 0)
		this->InitItem("firering");
	else
		this->InitItem("ring");

	this->AttachTo(&MiscList);

	if (this->field_10C != 0)
		this->mFlags |= 0x200;
	else
		this->mFlags |= 0x600;

	this->mRGB = 0x808080;

	this->mPos = *pPos;
	this->field_104 = 100;
	this->mPos.vy -= 0x5A000;
	this->field_108 = 3;

	SModel *pModel = G_PSXREGION[this->mRegion].ppModels[0];
	this->field_100 = pModel;

	this->field_F8 = reinterpret_cast<i16*>(Mem_New(6 * pModel->NumVertices));
	this->field_FC = reinterpret_cast<i16*>(Mem_New(6 * pModel->NumVertices));

	i16 *pSaved = this->field_F8;
	i16 *pDir = this->field_FC;
	SVECTOR *pVert = reinterpret_cast<SVECTOR*>(reinterpret_cast<u8*>(pModel) + 28);

	i32 i = 0;

	if (pModel->NumVertices != 0)
	{
		do
		{
			pSaved[0] = pVert->vx;
			pSaved[1] = pVert->vy;
			pSaved[2] = pVert->vz;

			i32 length = M3dMaths_SquareRoot0(pVert->vz * pVert->vz + pVert->vx * pVert->vx);
			print_if_false(length != 0, "Divide by zero in CDomeRing!");

			pDir[0] = (i16)(pVert->vx * this->field_104 / length);
			pDir[2] = (i16)(pVert->vz * this->field_104 / length);

			pSaved += 3;
			pDir += 3;
			pVert++;
			i++;
		}
		while (i < this->field_100->NumVertices);
	}
}

// @Ok
// 0x4F5720, 215 bytes. Puts the shared region model's vertices back the way
// CDomeRing's constructor found them, frees the two scratch buffers and drops
// the singleton.
//
// Note: CDomeRing's own virtual overrides (vtable 0x53C73C slots 2, 5, 6 and
// 7 -- sub_4F5800 animates the model vertices outwards each frame, sub_4F58B0
// / sub_4F5960 / sub_4F9040 are three class-specific virtuals) are still not
// decompiled, so a repo-built CDomeRing gets CBody's vtable and never
// animates. The constructor and destructor above are faithful; the rest of
// the class is open work.
CDomeRing::~CDomeRing(void)
{
	this->DeleteFrom(&MiscList);

	SModel *pModel = this->field_100;
	i16 *pSaved = this->field_F8;
	SVECTOR *pVert = reinterpret_cast<SVECTOR*>(reinterpret_cast<u8*>(pModel) + 28);

	i32 i = 0;

	if (pModel->NumVertices != 0)
	{
		do
		{
			pVert->vx = pSaved[0];
			pVert->vy = pSaved[1];
			pVert->vz = pSaved[2];

			pSaved += 3;
			pVert++;
			i++;
		}
		while (i < this->field_100->NumVertices);
	}

	Mem_Delete(this->field_F8);
	Mem_Delete(this->field_FC);

	print_if_false(*gpCurrentDomeRing == this, "Eh?");

	*gpCurrentDomeRing = 0;
}

// @Ok
// @AlmostMatching: vector assignment is different, this one doesn't use esi either
i32 Web_GetGroundY(const CVector* a1)
{

	gLineInfo.StartCoords = *a1;

	gLineInfo.EndCoords.vx = a1->vx;
	gLineInfo.EndCoords.vy = a1->vy + 0x1388000;
	gLineInfo.EndCoords.vz = a1->vz;

	M3dColij_InitLineInfo(&gLineInfo);
	M3dZone_LineToItem(&gLineInfo, 1);

	if (!gLineInfo.pItem)
		return gLineInfo.EndCoords.vy;

	return gLineInfo.Position.vy;
}

// @MEDIUMTODO
// 0x4F7F40, 429 bytes. Scoped but not written out. What it does: chains
// CNonRenderedBit::CNonRenderedBit, zeroes the first 6 bytes of 81 entries at
// +0x48 (stride 8), stores Mem_MakeHandle(pSuper) in field_3C and Type in the
// inherited mType, then writes a Mem_MakeHandle(this) back into the CSuper's
// own slot (field_104 for Type 0, field_10C for Type 1; any other Type only
// asserts). Then +0x424 = 400, +0x41E = 6, +0x41F = <a per-Type count> / 6,
// and field_44 = new CGPolyLine(80) with its +0x3C cleared.
//
// Blocked on class layout, not on callees: the count written to +0x41F comes
// from a byte at CSuper+0x13E, negated and randomised for Type 0 and divided
// by three for Type 1, and +0x41E / +0x41F / +0x424 are fields this repo does
// not have yet. Writing them would mean inventing three offsets inside the
// existing field_48 padding block, which is exactly the guesswork PLAN.md
// forbids.
//
// Layout finding while scoping this (NOT yet applied, because
// CTrapWebEffect::Burst is already @Ok against the current declaration): the
// zeroing loop runs 81 iterations of stride 8 over +0x48, i.e. 0x48..0x2D0,
// so web.h's `SHook field_48[122]` (0x48..0x418) is too long. 0x2D0 onwards is
// a separate i16 array, with a count at +0x374, a byte triple at +0x378 and a
// pointer array at +0x37C, all read by AddAnotherStrand below.
CTrapWebEffect::CTrapWebEffect(CSuper *, i32)
{
	printf("CTrapWebEffect::CTrapWebEffect(CSuper *,i32)");
}

// @MEDIUMTODO
// 0x4F83C0, 435 bytes. Blocked leaf-first on two callees that do not exist
// yet, CTrapWebEffect::MoveProjector (0x4F82A0) and CTrapWebEffect::CalcHook
// (0x4F8190), and on the same unmapped fields as the constructor above.
// What it does: while field_44's strand count (+0x3C) is under 80, advances
// the projector, adds one hook, and then, once at least two hooks exist and
// with a 1-in-4 chance and under 20 strands, picks the widest gap between the
// newest hook and each earlier one, records the (newest, newest-1, widest)
// index triple, and allocates a CQuadBit for the new strand
// (SetTexture(0x35006853), SetSemiTransparent).
void CTrapWebEffect::AddAnotherStrand(void)
{
	printf("CTrapWebEffect::AddAnotherStrand(void)");
}

// @Ok
// Reverse engineered 2026-08-31 from the original disasm at 0x4F8600
// (~570 bytes, SEH-protected the same way the CLAUDE.md "new T(...) SEH
// frame" family is: this session did not try to reproduce that frame,
// session rule is functional decomp only). Blockers noted by the previous
// session are both resolved this session:
//  - M3dUtils_GetDynamicHookPosition is @Ok (done earlier the same day).
//  - CWebFrag is now declared/implemented (web.h), reverse engineered from
//    its constructor at 0x4FA080 (see the class comment in web.h and
//    CWebFrag::CWebFrag below for the field-by-field evidence).
// Field roles confirmed this session:
//  - this->field_44 points at an unnamed, unmapped structure (never
//    constructed anywhere in this repo, so its own layout beyond these
//    three offsets is out of scope): a hook count at +0x3C, a VECTOR*
//    pointer at +0x40 into an inline VECTOR[] (16-byte stride) starting at
//    +0x44. Confirmed by raw pointer arithmetic in the disasm, not
//    guessed: the loop's byte-offset accumulator advances by 0x10 (one
//    VECTOR) per hook and is added to the pointer read from +0x40.
//  - this->field_48 (SHook[122], see web.h) is fed one entry at a time as
//    the pHook argument; the count comes from the field_44 target.
//  - The "ground height" is exactly Web_GetGroundY's gLineInfo pattern
//    inlined in the original; called Web_GetGroundY directly here since
//    the two are functionally identical (session goal is functional
//    correctness, not matching MSVC6's inlining choice byte-for-byte).
//  - this->mType (inherited CBit::mType, tested at offset 0x3B in the
//    disasm) selects which CSuper attachment slot gets cleared to a
//    Mem_MakeHandle(NULL): field_104 for mType == 0, field_10C for
//    mType == 1 (both already declared on CSuper in ob.h, and both are
//    already used elsewhere in the repo, e.g. carnage.cpp, to recover a
//    CTrapWebEffect* back out via Mem_RecoverPointer). Any other mType
//    prints "Bad CTrapWebEffect type", matching the original's assert.
void CTrapWebEffect::Burst(void)
{
	CSuper *pSuper = reinterpret_cast<CSuper*>(Mem_RecoverPointer(&this->field_3C));
	print_if_false(pSuper != NULL, "pSuper NULL??");

	if (pSuper != NULL)
	{
		u8 *pTarget = reinterpret_cast<u8*>(this->field_44);
		i32 HookCount = *reinterpret_cast<i32*>(pTarget + 0x3C);

		if (HookCount != 0)
		{
			M3d_BuildTransform(pSuper);

			VECTOR *pPositions = *reinterpret_cast<VECTOR**>(pTarget + 0x40);
			i32 i;

			// index 0 is always computed once, unconditionally, before the
			// count-gated loop below (matches the disasm exactly: the
			// pre-loop call uses field_48[0], the loop's first pass then
			// re-writes positions[0] using field_48[1]).
			M3dUtils_GetDynamicHookPosition(&pPositions[0], pSuper, &this->field_48[0]);

			for (i = 0; i < HookCount; i++)
			{
				M3dUtils_GetDynamicHookPosition(&pPositions[i], pSuper, &this->field_48[i + 1]);
			}

			i32 GroundY = Web_GetGroundY(&pSuper->mPos);

			CVector Prev(pPositions[0].vx, pPositions[0].vy, pPositions[0].vz);

			for (i = 0; i < HookCount - 1; i += 2)
			{
				CVector HookA(pPositions[i].vx, pPositions[i].vy, pPositions[i].vz);
				CVector HookB(pPositions[i + 1].vx, pPositions[i + 1].vy, pPositions[i + 1].vz);

				new CWebFrag(GroundY, Prev, HookA, HookB, pSuper->mPos, 25, 0);

				Prev = HookB;
			}
		}

		if (this->field_418)
			pSuper->mFlags &= ~0x400;

		if (this->mType == 0)
		{
			pSuper->field_104 = Mem_MakeHandle(NULL);
		}
		else if (this->mType == 1)
		{
			pSuper->field_10C = Mem_MakeHandle(NULL);
		}
		else
		{
			print_if_false(0, "Bad CTrapWebEffect type");
		}
	}

	this->Die();
}

// @Ok
// See the class comment in web.h for the full field derivation. Order of
// operations here follows the disasm: base CGLine::CGLine() runs first
// (implicit), then the 9 new dwords (field_5C/68/74) are zeroed, then
// mCodeBGR0 gets its flag bit, mGroundY/mStart/mEnd are set, the second
// CGLine (field_84) is allocated and given the same flag bit plus
// mProtected = 1, then the direction/length/scale computation, then the
// three-way copy plus optional jitter, then the random variant byte.
CWebFrag::CWebFrag(
		i32 GroundY,
		const CVector &PrevPos,
		const CVector &HookA,
		const CVector &HookB,
		const CVector &SuperPos,
		i32 Speed,
		i32 Jitter)
{
	this->field_5C.vx = 0;
	this->field_5C.vy = 0;
	this->field_5C.vz = 0;

	this->field_68.vx = 0;
	this->field_68.vy = 0;
	this->field_68.vz = 0;

	this->field_74.vx = 0;
	this->field_74.vy = 0;
	this->field_74.vz = 0;

	this->mCodeBGR0 |= 0x2000000;

	this->mGroundY = GroundY;

	this->mStart = HookA;
	this->mEnd = PrevPos;

	this->field_84 = new CGLine();

	this->field_84->mCodeBGR0 |= 0x2000000;
	this->field_84->mProtected = 1;

	this->field_84->mStart = HookA;
	this->field_84->mEnd = HookB;

	CVector Dir = HookA - SuperPos;
	i32 Length = Dir.Length();

	if (Length != 0)
	{
		this->field_68 = Dir * Speed / Length;
	}

	this->field_5C = this->field_68;
	this->field_74 = this->field_68;

	if (Jitter == 0)
	{
		this->field_5C.vy += (Rnd(21) - 10) << 12;
		this->field_68.vy += (Rnd(21) - 10) << 12;
		this->field_74.vy += (Rnd(21) - 10) << 12;
	}

	if (Rnd(3) != 0)
		this->field_88 = Rnd(3) + 6;
	else
		this->field_88 = Rnd(3) + 1;
}

// @Ok
int CSwinger::IsOneTimeToDie(void)
{
	return this->field_180 >= 4096;
}

// @Ok
// @Matching
void CSwinger::SetSpideyAnimFrame(i32 a2)
{
	MechList->mFrame = (a2 * (this->field_180 - 2048)) >> 11;
}


// @Ok
INLINE void CDomeShockWave::ResetHitFlags(CBody* body)
{
	for(CBody *cur = body; cur; cur = reinterpret_cast<CBody*>(cur->mNextItem))
		cur->mCBodyFlags &= 0xFEFF;
}

void validate_CImpactWeb(void){
	VALIDATE_SIZE(CImpactWeb, 0x8C);
}

void validate_CDomePiece(void){
	VALIDATE_SIZE(CDomePiece, 0x100);

	VALIDATE(CDomePiece, field_F4, 0xF4);
	VALIDATE(CDomePiece, field_F8, 0xF8);
	VALIDATE(CDomePiece, field_FC, 0xFC);
}

void validate_CDome(void){
	VALIDATE_SIZE(CDome, 0x11C);

	VALIDATE(CDome, hPlayer, 0xF8);
	VALIDATE(CDome, field_100, 0x100);

	VALIDATE(CDome, field_104, 0x104);
	VALIDATE(CDome, field_108, 0x108);
	VALIDATE(CDome, field_10C, 0x10C);
	VALIDATE(CDome, field_110, 0x110);
	VALIDATE(CDome, field_114, 0x114);
	VALIDATE(CDome, field_118, 0x118);
}

void validate_CDomeRing(void){
	VALIDATE_SIZE(CDomeRing, 0x110);

	VALIDATE(CDomeRing, field_F8, 0xF8);
	VALIDATE(CDomeRing, field_FC, 0xFC);
	VALIDATE(CDomeRing, field_100, 0x100);
	VALIDATE(CDomeRing, field_104, 0x104);
	VALIDATE(CDomeRing, field_108, 0x108);
}

void validate_CWeb(void){
	VALIDATE_SIZE(CWeb, 0x13C);


	VALIDATE(CWeb, field_F8, 0xF8);

	VALIDATE(CWeb, field_100, 0x100);
	VALIDATE(CWeb, field_102, 0x102);
	VALIDATE(CWeb, field_104, 0x104);

	VALIDATE(CWeb, field_108, 0x108);
	VALIDATE(CWeb, field_114, 0x114);

	VALIDATE(CWeb, field_120, 0x120);
	VALIDATE(CWeb, field_124, 0x124);
	VALIDATE(CWeb, field_128, 0x128);

	VALIDATE(CWeb, field_12C, 0x12C);

	VALIDATE(CWeb, field_130, 0x130);


	VALIDATE(CWeb, field_134, 0x134);
	VALIDATE(CWeb, field_138, 0x138);
}

void validate_CSwinger(void){
	VALIDATE_SIZE(CSwinger, 0x190);

	VALIDATE(CSwinger, field_F8, 0xF8);
	VALIDATE(CSwinger, field_FC, 0xFC);
	VALIDATE(CSwinger, mpLine, 0x17C);
	VALIDATE(CSwinger, field_180, 0x180);
}

void validate_CTrapWebEffect(void)
{
	VALIDATE_SIZE(CTrapWebEffect, 0x430);

	VALIDATE(CTrapWebEffect, field_3C, 0x3C);
	VALIDATE(CTrapWebEffect, field_44, 0x44);
	VALIDATE(CTrapWebEffect, field_48, 0x48);
	VALIDATE(CTrapWebEffect, field_418, 0x418);
}

void validate_CDomeShockWave(void)
{
	VALIDATE_SIZE(CDomeShockWave, 0x98);

	VALIDATE(CDomeShockWave, field_44, 0x44);
	VALIDATE(CDomeShockWave, field_50, 0x50);
	VALIDATE(CDomeShockWave, field_90, 0x90);
}

void validate_CWebFrag(void)
{
	VALIDATE_SIZE(CWebFrag, 0x8C);

	VALIDATE(CWebFrag, field_5C, 0x5C);
	VALIDATE(CWebFrag, field_68, 0x68);
	VALIDATE(CWebFrag, field_74, 0x74);
	VALIDATE(CWebFrag, mGroundY, 0x80);
	VALIDATE(CWebFrag, field_84, 0x84);
	VALIDATE(CWebFrag, field_88, 0x88);
}

// Global flag CSwinger_SwingBack only READS, never writes. Evidence: the exact same
// "test eax,eax; jnz <skip>" guard idiom, reading this SAME address (0x60D224), also appears
// verbatim (checked via IDA xrefs) in several other, apparently unrelated SEH-wrapped
// functions elsewhere in the binary (0x42FFC0, 0x43C150, 0x43C330, 0x43C3E0, 0x43C430,
// 0x457E00, 0x457EE0, 0x458050, 0x458210, 0x4582D0, 0x48C0D0, 0x48C730, 0x4F9BD0, 0x4FAF40),
// none of which write to it in the code inspected either. Whatever writes it to nonzero is
// outside every function traced this session. Named for its observed role in THIS function
// only (a "run the block below once" gate); its true system-wide purpose is not confirmed.
static volatile i32 * const gWebSwingBackOnceFlag = (i32*)0x0060D224;

// Vtable of an unidentified CKnottedWeb-derived class. CSwinger_SwingBack (0x4F7550) builds a
// plain CKnottedWeb in freshly allocated memory, then overwrites its vtable pointer with this
// address right after construction returns -- the same "construct as base, then poke the
// vtable to the real derived type" idiom already used for box->vtable in
// PShell_EndTrainingInit (pshell.cpp). No name found in idb_globals.txt, spideypc_names.txt,
// or spiderman_names.txt (Mac build). The two extra bytes of state this subclass reads/writes
// beyond a plain CKnottedWeb (field_6D, field_74) are declared directly on CKnottedWeb itself
// in bit2.h, since they sit inside CKnottedWeb's own already-validated 0x78-byte footprint,
// in bytes CKnottedWeb's own constructor never initializes.
static void * const gSwingBackWebVtable = (void*)0x0053C750;

// Plain non-throwing placement new, same convention as front.cpp's copy (the original builds
// this object via a raw CBit::operator new call plus a manual constructor call, not a plain
// `new CKnottedWeb(...)` expression, which would allocate sizeof(CKnottedWeb)=0x78 bytes
// instead of the 0x7C the disasm actually pushes -- see CSwinger_SwingBack below).
// @Bogus
inline void* operator new(size_t, void* location)
{
	return location;
}

// @Ok
// 2026-08-31: functional decompile (session bar). Address 0x4F7550; Mac mangled name
// (idbs/spiderman_names.txt) confirms "CSwinger::SwingBack". Despite the name this is not a
// simple detach/reset: gated by gWebSwingBackOnceFlag (see its own comment), it builds one
// CKnottedWeb-shaped "snapping web" visual effect object spanning from the swing line's start
// point to a point derived from the line's last segment end minus a1->field_FC, then
// reclassifies it (manual vtable poke to gSwingBackWebVtable, see its own comment) and copies
// a1->field_F8 into the resulting object's field_74 (CKnottedWeb, bit2.h). Every field access
// and call argument order below was cross-checked instruction-by-instruction against the
// 0x4F7550 disasm (operator- and Utils_CalcUnitFacingCamera's argument orders in particular,
// since both return/write through a hidden pointer).
// Called by CPlayer::CheckJumpingSmashKick to release the held web-swinging object.
void CSwinger_SwingBack(CSwinger *a1)
{
	// Original: nullsub_1(a1->mpLine != 0, "No line?"). nullsub_1 (0x4015B0) is the
	// confirmed-empty print_if_false stub this whole repo already omits at call sites (e.g.
	// ps2m3d.cpp), so only the semantic assertion is preserved here as a comment.

	if (*gWebSwingBackOnceFlag == 0)
	{
		CKnottedWeb *pWeb = (CKnottedWeb*)CBit::operator new(0x7C);

		if (pWeb)
		{
			CGPolyLine *pLine = a1->mpLine;
			SLineSeg *pLastSeg = &pLine->mSegs[pLine->mNumSegs - 1];

			CVector relEnd = pLastSeg->End - a1->field_FC;

			::new (pWeb) CKnottedWeb(pLine->mStart, relEnd);

			*reinterpret_cast<void**>(pWeb) = gSwingBackWebVtable;

			pWeb->SetStartAndEnd(&pLine->mStart, &relEnd);

			Utils_CalcUnitFacingCamera(
					&pLine->mStart,
					&relEnd,
					reinterpret_cast<CVector*>(&pWeb->field_58));

			for (i32 i = 0; i < pWeb->mNumSegs; i++)
			{
				pWeb->mpExtraSegs[i].mPos = pWeb->mSegs[i].End;
			}

			pWeb->field_6D = 1;

			pWeb->SetSemiTransparent();
			pWeb->mpInnerLine->SetSemiTransparent();
		}

		// Original writes this unconditionally, even down the "operator new returned null"
		// path (where it would null-deref) -- a genuine defect in the original if
		// CBit::operator new ever really returns 0 here; reproduced as-is per this repo's
		// "reproduce the source-level bug, don't fix it" convention (CLAUDE.md).
		pWeb->field_74 = a1->field_F8;
	}
}

// Turns a live web into a blob: mode field_104 goes to 3 and, unless the
// attached item is type 401, bit 3 of the attached item's flag word at 0x2A8
// is cleared (the "web attached" flag on the target, going by the fact that
// this is the only thing switching to blob mode does to it). Called by
// CPlayer::Hit (0x4BD890) and CPlayer::SwitchToDeathMode when the player
// loses a web mid-flight. Original address 0x4F6260.
// @Ok
void CWeb::SwitchToBlob(void)
{
	this->field_104 = 3;

	CItem *pTarget = reinterpret_cast<CItem*>(
			Mem_RecoverPointer(reinterpret_cast<SHandle*>(&this->field_134)));

	if (pTarget == 0)
		return;

	if (pTarget->mType == 401)
		return;

	// offset 0x2A8 of the attached item. Which concrete class owns it is not
	// known yet (CWeb only ever sees it through this handle), so it stays a
	// raw offset rather than a guessed field name.
	*reinterpret_cast<i32*>(reinterpret_cast<u8*>(pTarget) + 0x2A8) &= ~8;
}

// @Ok
// verified against the IDA disasm of 0x4F5DA0 (136 bytes). Chains
// CBody::CBody, zeroes the two anchor vectors and the three scalars behind
// them, links the new web into WebList and stamps mType 1. Everything else
// (the mode in field_104, the webbing cost in field_102, the hook and
// target handles) is filled in by the caller, CPlayer::CheckJumpingR1ZipWeb
// or CPlayer::CheckJumpingR2ZipWeb.
CWeb::CWeb(void)
{
	this->field_108.vx = 0;
	this->field_108.vy = 0;
	this->field_108.vz = 0;

	this->field_114.vx = 0;
	this->field_114.vy = 0;
	this->field_114.vz = 0;

	this->field_120 = 0;
	this->field_124 = 0;
	this->field_128 = 0;

	this->AttachTo(&WebList);

	this->mType = 1;
}

// @MEDIUMTODO
// 0x4F5ED0, 662 bytes. Called by CPlayer::FireWeb.
void CWeb::Fire(CVector &, CVector &, CBody *, bool, CSVector &)
{
	printf("CWeb::Fire(CVector &,CVector &,CBody *,bool,CSVector &)");
}

// @Ok
// 0x4F8D10, 229 bytes. Called by CPlayer::FireWeb. Wraps a baddy in webbing:
// reuses the CTrapWebEffect already hanging off the baddy's field_104 if
// there is one, otherwise makes a fresh one (Type 0) and tells the player it
// just landed a hit. Either way eight more strands go on.
//
// The original writes field_418 straight through the pointer `new` returned
// without checking it, so an out-of-memory CBit::operator new dereferences
// null here. That is the original's own defect and is left as written.
//
// The original emits eight straight-line AddAnotherStrand calls rather than a
// loop; the loop below is the same eight calls.
void Web_Trap(CSuper *pSuper, i32 a2)
{
	print_if_false(pSuper != 0, "NULL pBaddy sent to Web_Trap");
	print_if_false((pSuper->mFlags >> 1) & 1, "Non superitem sent to Web_Trap");

	CTrapWebEffect *pEffect = reinterpret_cast<CTrapWebEffect*>(
			Mem_RecoverPointer(&pSuper->field_104));

	if (pEffect == 0)
	{
		pEffect = new CTrapWebEffect(pSuper, 0);
		pEffect->field_418 = (u8)a2;

		if (G_MECHLIST != 0)
			reinterpret_cast<CPlayer*>(G_MECHLIST)->SetFirstContactDetails();
	}

	for (i32 i = 0; i < 8; i++)
		pEffect->AddAnotherStrand();
}

// @Ok
// 0x4F9940, 604 bytes. Called by CPlayer::FireWeb. The web projectile itself:
// a CFlatBit that flies from Pos along Normal at Speed. The constructor casts
// the whole flight path as one line up front (gLineInfo + M3dZone_LineToItem)
// and, if it will hit something, shortens mLifetime to the number of frames
// that takes and remembers the impact point, so the sprite stops exactly on
// the surface instead of being tested every frame.
//
// LineOfSightCheck (m3dcolij.h) is raised around the M3dZone_LineToItem call,
// the same "this is a sight/trace query, not a real collision" flag the rest
// of the collision code uses.
//
// Original defects kept: mpHitItem / mpHitFace are only written on a hit,
// while the six fields after them are pre-zeroed, so a miss leaves those two
// holding whatever the freshly allocated block had; and the "Not in list"
// walk over EnviroList computes a result that only ever feeds print_if_false,
// which is a no-op in this build.
CImpactWeb::CImpactWeb(
		const CVector &Pos,
		const CSVector &Normal,
		i32 Speed,
		i32 Damage,
		i32 Lifetime)
{
	this->mHitPos.vx = 0;
	this->mHitPos.vy = 0;
	this->mHitPos.vz = 0;

	this->mHitNormal.vx = 0;
	this->mHitNormal.vy = 0;
	this->mHitNormal.vz = 0;

	if (G_MECHLIST != 0)
		this->mDamage = reinterpret_cast<CPlayer*>(G_MECHLIST)->GetDamageInflictedFromDifficulty(Damage);
	else
		this->mDamage = Damage;

	this->mPos = Pos;
	this->mStartTime = gTimerRelated;

	print_if_false(Speed != 0, "Zero speed sent to CImpactWeb");

	Utils_GetVecFromMagDir(&this->mVel, Speed, const_cast<CSVector*>(&Normal));

	this->mLifetime = (u16)Lifetime;

	gLineInfo.StartCoords = this->mPos;

	gLineInfo.EndCoords.vx = this->mPos.vx + this->mLifetime * this->mVel.vx;
	gLineInfo.EndCoords.vy = this->mPos.vy + this->mLifetime * this->mVel.vy;
	gLineInfo.EndCoords.vz = this->mPos.vz + this->mLifetime * this->mVel.vz;

	M3dColij_InitLineInfo(&gLineInfo);

	LineOfSightCheck = 1;
	M3dZone_LineToItem(&gLineInfo, 0);
	LineOfSightCheck = 0;

	if (gLineInfo.pItem != 0)
	{
		CItem *pItem = EnviroList;

		while (pItem != 0 && pItem != gLineInfo.pItem)
			pItem = pItem->mNextItem;

		print_if_false(pItem != 0, "Not in list");

		this->mpHitItem = gLineInfo.pItem;
		this->mpHitFace = gLineInfo.pFace;

		this->mHitPos.vx = gLineInfo.Position.vx;
		this->mHitPos.vy = gLineInfo.Position.vy;
		this->mHitPos.vz = gLineInfo.Position.vz;

		this->mHitNormal.vx = gLineInfo.Normal.vx;
		this->mHitNormal.vy = gLineInfo.Normal.vy;
		this->mHitNormal.vz = gLineInfo.Normal.vz;

		this->mLifetime = (u16)(gLineInfo.Distance / Speed);

		print_if_false((this->mpHitItem->mFlags & 0x10) == 0, "Hit env obj!");
	}

	if ((i32)this->mLifetime > Lifetime)
		this->mLifetime = (u16)Lifetime;

	this->SetAnim(9);
	this->SetFrame(0);
	this->SetScale(0);

	this->mPostScale = 0x0A001000;

	this->field_5A = Rnd(2) != 0 ? 768 : -768;
}

// @Ok
// 0x4FAD50, 205 bytes. Called by CPlayer::PriorToVenomDistanceAttack. Pops
// the dome: a web dome (field_104 == 0) shatters the item it trapped, then
// the dome switches to mode 3, spawns the shockwave and the ring, and dies.
//
// field_110 is typed CClass* like the dome's other four sub-objects, but the
// original treats this one as a CItem here (it clears mFlags bit 0x400 at
// +4 and makes a thiscall to CItem::Burst), so it is cast, exactly as
// written. Both allocations are plain `new`: the original's
// CBit::operator new(0x98) / CClass::operator new(0x110) plus null check is
// what MSVC6 emits for `new CDomeShockWave` / `new CDomeRing` (CItem's
// operator new is link-folded onto CClass's).
void CDome::Burst(void)
{
	if (this->field_104 == 0)
	{
		CItem *pTrapped = reinterpret_cast<CItem*>(this->field_110);

		if (pTrapped != 0)
		{
			pTrapped->mFlags &= ~0x400;
			pTrapped->Burst(30, 30);
		}
	}

	this->field_100 = 3;

	new CDomeShockWave(this->field_104);
	new CDomeRing(&this->mPos, this->field_104);

	this->Die();
}
