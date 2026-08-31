#pragma once

#ifndef WEB_H
#define WEB_H

#include "bit.h"
#include "bit2.h"
#include "export.h"
#include "spidey.h"
#include "m3dutils.h"

EXPORT extern i32 gFireDomes;
EXPORT extern i32 gNumDomes;
EXPORT extern CBody* WebList;

class CImpactWeb : public CFlatBit
{
	public:
		PADDING(0x24);
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
};

class CDomeRing : public CBody {
	public:

		PADDING(4);

		i32 field_F8;
		i32 field_FC;
		i32 field_100;
		i32 field_104;
		i32 field_108;

		PADDING(1);
};


class CWeb : public CBody
{
	public:

		PADDING(4);

		i32 field_F8;

		PADDING(4);

		i32 field_100;
		i32 field_104;
		CVector field_108;

		CVector field_114;

		i32 field_120;
		i32 field_124;
		i32 field_128;
		u8 *field_12C;

		i32 field_130;

		i32 field_134;
		i32 field_138;
};

class CSwinger : public CBody 
{
	public:
		EXPORT i32 IsOneTimeToDie(void);
		EXPORT void SetSpideyAnimFrame(i32);

		PADDING(0x180-0xF4);

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
