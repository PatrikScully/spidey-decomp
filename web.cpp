#include "web.h"
#include "m3dinit.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "spidey.h"
#include "baddy.h"
#include "utils.h"
#include "camera.h"
#include "spool.h"

#include "validate.h"

i32 gFireDomes;
i32 gNumDomes;
CBody* WebList;

EXPORT i32 gGetGroundDefaultValue;

extern CBody* MiscList;

// @MEDIUMTODO
// Investigated 2026-08-31, left as a stub, not attempted. Findings for
// whoever picks this up next (address 0x4F7AE0, ~1126 bytes):
// - Most of the function decompiles cleanly. It transforms the line
//   segment (a2 start, a3 end) into pSuper's local space, then walks a
//   per-hook mesh table looking for the closest hook whose bounding box
//   the line crosses:
//   - World-to-local transform: M3dMaths_TransposeMatrix1(&pSuper->mTransform,
//     &localMatrix) then gte_SetRotMatrix(&localMatrix) (or the
//     memcpy(gRotMatrix, ...) form Utils_RotateWorldToObject in utils.cpp
//     already uses), followed by the same MTC2(GT_ZERO)/MTC2(GT_ONE)/
//     gte_mvmva(1,0,0,3,0)/gte_stsv idiom Utils_RotateWorldToObject already
//     has, run once for (start - pSuper->mPos) and once for (end - pSuper->mPos).
//   - Region lookup: pSuper->mRegion (CItem, offset 0x1F) indexes
//     CItemRelatedList[mRegion*17] (ob.h, 0x6B2454) for a per-hook mesh
//     pointer table, and a second, currently unnamed twin table at
//     0x6B2458 (dword_6B2458[mRegion*17]+8) for the hook count.
//   - The blocker: both tables index into the same undocumented "SModel"
//     packed struct that a prior session already investigated and declined
//     to guess at (see m3dinit.cpp's M3dInit_ParsePSX @MEDIUMTODO comment,
//     2026-08-31: "every one of those five arrays is an opaque,
//     undocumented struct-of-pointers table... dcmodel.h only forward-
//     declares struct SModel, zero fields known"). Here the per-hook entry
//     at CItemRelatedList[region*17][i]+24, stride 24 bytes, is read
//     directly as a MATRIX* (passed straight into
//     M3dMaths_TransposeMatrix1), and *v18+12 (v18 = the twin table's
//     per-hook pointer) is read as a flat i16 mesh-vertex array. Getting
//     these offsets right needs the same struct reverse-engineering pass
//     the m3dinit.cpp note flags as out of scope for a single-file task.
//   - Also missing: BoundingBoxCollisionCheck (tools/names.json calls it
//     that, address 0x4F7680, tentative name only, no repo declaration or
//     stub anywhere). Called as
//     BoundingBoxCollisionCheck(&meshPoint0, &meshPoint1, &localStart, &localEnd)
//     (cdecl, 4 args, all pointers to packed i16 triples), returns a bool
//     gating a running-minimum distance search that fills the output SHook
//     (a4) with the winning hook's scaled position and sets a4->Offset (a4[3])
//     to a bit index (1,2,4,8,...). Its own body is a separate, undocumented
//     ~unknown-size routine; forwarding to the original address would work
//     for runtime correctness but the 4 packed-point arguments only make
//     sense once the SModel mesh layout above is known, so a forward stub
//     here would be guesswork about which fields feed it.
// Not attempted further: the real blocker is the shared SModel struct, not
// this function's own control flow, which is otherwise a plain nested loop.
i32 Web_CollideWithSuper(CSuper *,CVector const *,CVector const *,SHook *,i32)
{
    printf("Web_CollideWithSuper(CSuper *,CVector const *,CVector const *,SHook *,i32)");
	return 0x23022025;
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

// @BIGTODO
// Investigated 2026-08-31, left as a stub, not attempted. Findings for
// whoever picks this up next (address 0x4F8600, ~570 bytes):
// - The shape: recover a CSuper* from this->field_3C (SHandle), print_if_false
//   if null, then if this->field_44's target has a nonzero hook count at
//   +0x3C: M3d_BuildTransform(pSuper), then for each hook
//   (this->field_44 + 0x44, stride 0x10, count at +0x3C) call
//   M3dUtils_GetDynamicHookPosition(VECTOR*, CSuper*, SHook*) to fill
//   this->field_48 (offset 0x48, a VECTOR). It also does an inlined
//   ground-height probe identical to Web_GetGroundY's gLineInfo pattern
//   (StartCoords = pSuper->mPos, EndCoords = pSuper->mPos + (0, 0x1388000, 0),
//   M3dColij_InitLineInfo, M3dZone_LineToItem(&gLineInfo, 1), then
//   groundY = gLineInfo.pItem ? gLineInfo.Position.vy : gLineInfo.EndCoords.vy).
//   Then a loop over hook pairs (i, i+1) allocates a new CWebFrag(0x8C bytes,
//   via CBit::operator new) per pair and calls its constructor with the
//   ground position, the two hook world positions (from field_44+0x44,
//   stride 0x20 this time, not 0x10), and constants (25, 0) -- constructor
//   signature confirmed from the Mac mangled name already in names.json:
//   CWebFrag(int, const CVector&, const CVector&, const CVector&, const CVector&, int, int).
//   After the loop: clear this->field_418's related flag bit on the
//   recovered CSuper if this->field_418 is set, then a 3-way switch on
//   this->field_3B (0/1/other) picks which pair of Mem_MakeHandle results
//   to store (field_104/108 vs field_10C/110), printing "Bad CTrapWebEffect
//   type" for any other value. Finally calls CBit::Die() (real, already @Ok).
// - Blockers, both out of this file's scope:
//   1. M3dUtils_GetDynamicHookPosition (m3dutils.cpp, currently a total
//      printf stub, @BIGTODO there) is a hard functional dependency: Burst's
//      hook positions come from it. Decompiling Burst without it gives wrong
//      numbers, not just wrong bytes.
//   2. CWebFrag does not exist anywhere in the repo (no header, no .cpp,
//      no forward declaration). Its constructor is only known by mangled
//      name and its size (0x8C, from the CBit::operator new(0x8C) call) is
//      the only layout fact available. Creating it from scratch means
//      guessing at every field, which the "don't guess a missing struct"
//      rule for this session rules out.
//   3. this->field_44's target object (accessed at +0x3C count, +0x40 array
//      pointer, +0x44 inline hook array with 0x10 and 0x20 byte strides in
//      different parts of the function) is not CSuper itself (CSuper/CBody/
//      CItem's known layout does not reach a hook table at those offsets);
//      it is a separate, currently unnamed and unmapped structure.
// Not attempted further: needs a same-session fix to m3dutils.cpp plus a
// brand new CWebFrag class with a struct layout nobody has reverse
// engineered yet, both outside the web.cpp assignment.
void CTrapWebEffect::Burst(void)
{
	printf("void CTrapWebEffect::Burst(void)");
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

	VALIDATE(CSwinger, field_180, 0x180);
}

void validate_CTrapWebEffect(void)
{
	VALIDATE_SIZE(CTrapWebEffect, 0x430);

	VALIDATE(CTrapWebEffect, field_3C, 0x3C);
	VALIDATE(CTrapWebEffect, field_44, 0x44);
	VALIDATE(CTrapWebEffect, field_418, 0x418);
}

void validate_CDomeShockWave(void)
{
	VALIDATE_SIZE(CDomeShockWave, 0x98);

	VALIDATE(CDomeShockWave, field_44, 0x44);
	VALIDATE(CDomeShockWave, field_50, 0x50);
	VALIDATE(CDomeShockWave, field_90, 0x90);
}
