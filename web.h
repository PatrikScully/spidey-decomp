#pragma once

#ifndef WEB_H
#define WEB_H

#include "bit.h"
#include "bit2.h"
#include "export.h"
#include "spidey.h"
#include "m3dutils.h"

struct SModel;

EXPORT extern i32 gFireDomes;
EXPORT extern i32 gNumDomes;
EXPORT extern CBody* WebList;

// Address 0x4F7680. Axis-aligned-box-vs-line-segment clip test; see its own comment in
// web.cpp. pMin/pMax are a box's two opposite corners; on a hit, *pEnd is overwritten with
// the crossing point.
EXPORT i32 BoundingBoxCollisionCheck(SVECTOR const *pMin, SVECTOR const *pMax, SVECTOR const *pStart, SVECTOR *pEnd);

class CImpactWeb : public CFlatBit
{
	public:
		// Original 0x4F9940. Mac mangled name
		// __ct__10CImpactWebFRC7CVectorRC8CSVectoriii gives the parameter
		// list. The web splat left on a wall when a web shot misses.
		EXPORT CImpactWeb(const CVector &Pos, const CSVector &Normal, i32 Speed, i32 Damage, i32 Lifetime);

		// Fields recovered from the constructor's own stores (0x4F9940),
		// which is the only decompiled user of this class so far.

		// the Damage argument, run through
		// CPlayer::GetDamageInflictedFromDifficulty when a player exists
		i32 mDamage;

		// gTimerRelated at spawn time
		i32 mStartTime;

		// the item the forward ray hit, and the face on it. Both left
		// uninitialised when the ray misses (original defect: only the six
		// fields below are pre-zeroed).
		CItem *mpHitItem;
		u32 *mpHitFace;

		CVector mHitPos;
		CSVector mHitNormal;

		PADDING(2);
};


class CDomePiece : public CBody 
{
	public:
		EXPORT CDomePiece(CVector*, i32, i32, i32);
		EXPORT virtual ~CDomePiece(void) OVERRIDE;

		i32 field_F4;
		i32 field_F8;
		i32 field_FC;
};

class CDome : public CBody
{
	public:
		EXPORT CDome(CPlayer*, i32);
		EXPORT virtual ~CDome(void) OVERRIDE;

		PADDING(4);

		SHandle hPlayer;

		i32 field_100;
		i32 field_104;

		CClass *field_108;
		CClass *field_10C;
		CClass *field_110;
		CClass *field_114;
		CClass *field_118;

		// Original 0x4FAD50. Pops the dome: see the stub in web.cpp.
		EXPORT void Burst(void);
};

class CDomeRing : public CBody {
	public:

		// 0x4F5510 / 0x4F5720. Only one CDomeRing exists at a time; the
		// constructor deletes the previous one (gpCurrentDomeRing, web.cpp).
		EXPORT CDomeRing(const CVector *pPos, i32 bFire);
		EXPORT virtual ~CDomeRing(void) OVERRIDE;

		PADDING(4);

		// saved copy of the ring model's vertices, 3 i16 per vertex, so the
		// destructor can put the shared SModel geometry back
		i16 *field_F8;

		// per-vertex outward direction, also 3 i16 per vertex. Only [0] and
		// [2] are ever written; [1] is left uninitialised by the original
		// (original defect, kept).
		i16 *field_FC;

		// the ring's SModel (region model 0), whose vertices get animated
		// outwards in place
		SModel *field_100;

		i32 field_104;
		i32 field_108;

		// nonzero for the fire dome variant ("firering" instead of "ring")
		i32 field_10C;
};


// 0x4F5BC0. The blob of webbing left where a web shot lands. Only CWeb::Fire
// makes one. Base is CQuadBit (0x84), confirmed by the constructor's first
// call being CQuadBit::CQuadBit and by its SetTexture / SetSemiTransparent /
// OrientUsing calls; own fields start at 0x8C and the class ends at 0xB0,
// which is the size the original passes to CBit::operator new.
class CKnottedWebSplat : public CQuadBit
{
	public:
		EXPORT CKnottedWebSplat(const CVector *pPos, const CVector *pNormal);

		PADDING(0x8C - 0x84);

		// splat centre: pPos pushed 10 units out along pNormal
		CVector field_8C;

		// the two corner offsets the constructor derives from the quad's
		// own mPosB / mPosC after OrientUsing
		CVector field_98;
		CVector field_A4;
};

class CWeb : public CBody
{
	public:

		EXPORT CWeb(void);

		PADDING(4);

		i32 field_F8;

		PADDING(4);

		u16 field_100;

		// Written as a u16 (0) by CPlayer::CheckJumpingR1ZipWeb and
		// CheckJumpingR2ZipWeb right after the web is allocated, and read
		// back as a u16 by CPlayer::FireWeb, which passes it to
		// M3dUtils_GetHookPosition as the hook index the web leaves from.
		// field_100 was an i32 covering 0x100..0x104 before that was found;
		// nothing in the repo used it, so it is split into two u16 here.
		u16 field_102;

		i32 field_104;
		CVector field_108;

		CVector field_114;

		i32 field_120;
		i32 field_124;
		i32 field_128;
		// the drawn strand, made by CWeb::Fire
		CKnottedWeb *field_12C;

		i32 field_130;

		// SHandle (pWhatever/Id) of whatever this web is attached to; read
		// back with Mem_RecoverPointer by CWeb::SwitchToBlob and by
		// CPlayer::CalculateTugWebPathPoints (spidey.cpp).
		i32 field_134;
		i32 field_138;

		EXPORT void SwitchToBlob(void);

		// Original 0x4F5ED0. Mac mangled name
		// Fire__4CWebFR7CVectorR7CVectorP5CBodybR8CSVector gives the
		// parameter list. Anchors the web between two points and spawns the
		// drawn strand; pTarget is whatever the web attached to.
		EXPORT void Fire(CVector &Hook, CVector &Target, CBody *pTarget, bool bSplat, CSVector &Normal);
};

// Original 0x4F8D10. Mac mangled name Web_Trap__FP6CSuperi. Wraps a baddy
// in a trap web.
EXPORT void Web_Trap(CSuper *pSuper, i32 a2);

// Fields at 0xF8/0xFC/0x17C reverse engineered 2026-08-31 while decompiling
// CSwinger_SwingBack (0x4F7550, web.cpp): that function reads all three
// through `this`, confirmed instruction-by-instruction against the disasm
// (field_F8 and field_FC feed a CVector subtraction and a copied-out i32;
// mpLine is asserted non-null ("No line?") and its CGPolyLine fields
// mNumSegs/mSegs/mStart are read through it at the offsets CGPolyLine
// already validates in bit2.cpp). field_F8's exact purpose is not
// confirmed (a plain i32 copied verbatim into a spawned effect object's
// own field_74, see bit2.h's CKnottedWeb); field_FC is a CVector, likely
// some kind of "last known hook/anchor offset" given how it is subtracted
// from the swing line's last segment endpoint, but that reading is our own
// guess, not confirmed.
class CSwinger : public CBody
{
	public:
		EXPORT i32 IsOneTimeToDie(void);
		EXPORT void SetSpideyAnimFrame(i32);

		PADDING(0xF8-0xF4);

		i32 field_F8;

		CVector field_FC;

		PADDING(0x17C-0x108);

		CGPolyLine *mpLine;

		i32 field_180;

		PADDING(0x190-0x180-4);
};

// Called by CPlayer::CheckJumpingSmashKick to release the held web-swinging
// object before the smash kick animation starts.
EXPORT void CSwinger_SwingBack(CSwinger *a1);

class CSplat : public CQuadBit
{
	public:
};

// Reverse engineered 2026-08-31 (session working on CTrapWebEffect::Burst,
// 0x4F8600) via CWebFrag's constructor (0x4FA080, called from Burst with
// CBit::operator new(0x8C)). Mac mangled name from
// idbs/spiderman_names.txt confirms the parameter list:
// CWebFrag(int, const CVector&, const CVector&, const CVector&, const
// CVector&, int, int). Base is CGLine (bit2.h): the constructor's first
// call is CGLine::CGLine(), confirmed by name via
// idbs/spideypc_names.txt ("00412c00: ??0CGLine@@QAE@XZ") matching
// names.json's own "CGLine_CGLine" for 0x412C00, and CGLine's own size
// (0x5C, VALIDATE_SIZE in bit2.cpp) exactly matches where CWebFrag's own
// fields start (0x5C) and CGLine's mCodeBGR0 (0x3C)/mStart(0x44)/mEnd(0x50)
// are the fields the ctor writes through inherited offsets.
class CWebFrag : public CGLine
{
	public:
		EXPORT CWebFrag(
				i32 GroundY,
				const CVector &PrevPos,
				const CVector &HookA,
				const CVector &HookB,
				const CVector &SuperPos,
				i32 Speed,
				i32 Jitter);

		// Three copies of the same vector: (HookA - SuperPos) scaled to
		// length Speed (left at zero if HookA == SuperPos), with a random
		// (-10..10) fixed-point jitter added to the vy component of each
		// copy when Jitter == 0. Only the stores are confirmed from the
		// disasm; which reader (if any, e.g. a not-yet-decompiled Move())
		// uses which copy is not known, so these keep raw names.
		CVector field_5C;
		CVector field_68;
		CVector field_74;

		i32 mGroundY;

		// Second CGLine, heap-allocated and constructed by CWebFrag's own
		// ctor (same 0x2000000 mCodeBGR0 flag, CBit::mProtected = 1), whose
		// mStart/mEnd run HookA -> HookB. Together with this object's own
		// mStart/mEnd (PrevPos -> HookA), the pair draws a connected
		// two-segment "web strand" through the three points.
		CGLine *field_84;

		// Random small variant/index (1..3, or 6..8 with lower odds); exact
		// use (sprite frame? which of two shapes?) unknown.
		u8 field_88;
		PADDING(3);
};

class CTrapWebEffect : public CNonRenderedBit
{
	public:
		// 0x4F7F40. Type goes into the inherited CBit::mType and selects
		// which CSuper handle slot points back at this effect (field_104
		// for 0, field_10C for 1), the same split CTrapWebEffect::Burst
		// already reads back.
		EXPORT CTrapWebEffect(CSuper *pSuper, i32 Type);

		// 0x4F83C0. Adds one strand of webbing to the effect.
		EXPORT void AddAnotherStrand(void);

		EXPORT void Burst(void);

		SHandle field_3C;
		i32 *field_44;

		// Inline array of hook definitions Burst() feeds one at a time to
		// M3dUtils_GetDynamicHookPosition. Size derived from the exact
		// padding gap to field_418 (0x3D0 bytes); 0x3D0 / sizeof(SHook)
		// (8) == 122 exactly, strong evidence this is really an SHook[]
		// and not opaque padding.
		SHook field_48[122];

		u8 field_418;
		PADDING(0x430-0x418-1);
};

class CDomeShockWave : public CNonRenderedBit
{
	public:
		EXPORT CDomeShockWave(i32);
		EXPORT virtual ~CDomeShockWave(void);

		EXPORT void ResetHitFlags(CBody*);

		PADDING(8);
		CVector field_44;
		i32 field_50[16];
		i32 field_90;

		PADDING(4);
};

EXPORT int Web_GetGroundY(const CVector*);
EXPORT i32 Web_CollideWithSuper(CSuper *,CVector const *,CVector const *,SHook *,i32);


void validate_CImpactWeb(void);
void validate_CKnottedWebSplat(void);
void validate_CDomePiece(void);
void validate_CDome(void);
void validate_CDomeRing(void);
void validate_CWeb(void);
void validate_CSwinger(void);
void validate_CSplat(void);
void validate_CTrapWebEffect(void);
void validate_CDomeShockWave(void);
void validate_CWebFrag(void);

#endif
