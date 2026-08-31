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

// @NotOk
// Forward to original (0x4F7550); not yet decompiled. Called by
// CPlayer::CheckJumpingSmashKick to release the held web-swinging object.
void CSwinger_SwingBack(CSwinger *a1)
{
	typedef void (*func_ptr)(CSwinger*);
	func_ptr func = (func_ptr)0x004F7550;
	func(a1);
}
