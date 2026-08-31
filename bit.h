#pragma once

#ifndef BIT_H
#define BIT_H

#include "main.h"
#include "vector.h"
#include "texture.h"
#include "ps2funcs.h"

// @Note guessed the name
struct SSimpleRibbonParams
{
	PADDING(0x18);
	u32 field_18;
};

// @Note: guessed name
struct SRibbonTexture
{
	u16 field_0;
	u16 field_2;

	u8 field_4;
	u8 field_5;
	u8 field_6;
	u8 field_7;
	u8 field_8;
	u8 field_9;
	u8 field_A;
	u8 field_B;

	u16 field_C;
	u16 field_E;

	u8 u0;
	u8 v0;

	u16 mClut;

	u8 u1;
	u8 v1;

	u16 mPage;

	u8 u2;
	u8 v2;

	u8 u3;
	u8 v3;

	u32 mTexWin;
};

class vector3d
{
	public:
		f32 field_0[3];

		EXPORT vector3d(f32, f32, f32);
};

class vector4d
{
	public:
		f32 field_0[4];

		// @Ok
		EXPORT vector4d(void)
		{
		}

		EXPORT vector4d(const vector3d&, f32);
		EXPORT vector4d& operator=(const vector4d&);
};

struct SFringeQuad
{
	u32 Width;
	u32 CodeBGR;
};

struct SSection
{
	u32 Radius;
	u32 PadBGR;
};

struct SAnimFrame
{
	char OffX;
	char OffY;
	u8 Width;
	u8 Height;
	Texture *pTexture;
};


EXPORT extern u32 SparkSize;
// @FIXME - is it really volatile?
EXPORT extern volatile i32 gTimerRelated;

struct SRibbonPoint {
	// offset: 0000 (12 bytes)
	CVector Pos;
	// offset: 000C
	u8 r;
	// offset: 000D
	u8 g;
	// offset: 000E
	u8 b;
	// offset: 0010
	u16 Width;
	// offset: 0012
	u16 WidthB;
	// offset: 0014
	u8 rB;
	// offset: 0015
	u8 gB;
	// offset: 0016
	u8 bB;
	// offset: 0018
	i32 Last1Scr;
	// offset: 001C
	i32 Last2Scr;
	// offset: 0020
	i32 Last3Scr;
};

class CBit
{
	public:
		CBit* mPrevious;
		CBit* mNext;

		i16 mAge;
		u16 mLifetime;

		CVector mPos;
		CVector mVel;
		CVector mAcc;
		CFriction mFric;
		u8 mDead;

		u16 mFrigDeltaZ;
		u8 mProtected;
		u8 mType;

		EXPORT CBit();
		EXPORT virtual ~CBit();
		EXPORT virtual void Move(void);
		EXPORT void* operator new(size_t size);
		EXPORT void operator delete(void*);
		EXPORT void Die();
		EXPORT void AttachTo(void*);
		EXPORT void SetPos(const CVector &pos);
		EXPORT void DeleteFrom(void*);
};

class CQuadBit : public CBit {

public:
	CVector mPosB;
	CVector mPosC;
	CVector mPosD;
	Texture* mpTexture;
	u32 mCodeBGR;

	u32 field_68;

	u32 mTint;

	u32 field_70;

	u32 field_74;
	u32 field_78;
	u32 field_7C;
	u32 field_80;

	EXPORT CQuadBit(void);

	EXPORT void SetTint(u8 a2, u8 a3, u8 a4);
	EXPORT void SetSemiTransparent();
	EXPORT void SetOpaque();
	EXPORT void SetSubtractiveTransparency();
	EXPORT void SetCorners(const CVector &a2, const CVector &a3, const CVector &a4, const CVector &a5);
	EXPORT void SetTransparency(u8 a2);
	EXPORT void OrientUsing(CVector *, SVECTOR *, i32, i32);
	EXPORT void OrientUsing(CVector *, SVECTOR *, i32, i32, i32);
	EXPORT void SetTexture(i32, i32);
	EXPORT void SetTexture(u32);
	EXPORT void SetTexture(Texture*);
	EXPORT void SetTexture(char*, i32);
	EXPORT void SetTexture(char*);
};

class CFT4Bit : public CBit
{
	public:
		EXPORT CFT4Bit(void);
		EXPORT virtual ~CFT4Bit();
		EXPORT void SetAnimSpeed(short);
		EXPORT void SetScale(u16);
		EXPORT void SetSemiTransparent();
		EXPORT void SetTransparency(u8 t);
		EXPORT void SetAnim(i32);
		EXPORT void IncFrameWithWrap(void);
		EXPORT void SetFrame(i32);
		EXPORT void SetTint(u8, u8, u8);
		EXPORT void SetTexture(Texture*);
		EXPORT void SetTexture(u32);
		EXPORT int Fade(i32);
		EXPORT void SetTransDecay(i32);
		EXPORT void IncFrame(void);

		i16 mTransDecay;
		u16 field_3E;
		u32 mCodeBGR;

		u8 mDeleteAnimOnDestruction;
		PADDING(0x3);

		SAnimFrame *mpPSXAnim;
		SAnimFrame *mpPSXFrame;
		u8 mBitFlags;

		u8 mNumFrames;
		i8 mFrame;
		u8 mFrameFrac;

		i16 mAnimSpeed;
		i16 mScale;
};

class CFlatBit : public CFT4Bit {

public:

	EXPORT CFlatBit(void);
	EXPORT virtual ~CFlatBit(void) OVERRIDE;
	i16 mAngle;
	i16 field_5A;

	PADDING(0x2);

	u16 mAngFric;
	u32 mPostScale;

	PADDING(1);
	u8 mSemiTransparencyRate;
};

class CNonRenderedBit : public CBit {
	public:
		EXPORT CNonRenderedBit(void);
		EXPORT virtual ~CNonRenderedBit(void) OVERRIDE;
};

class CSpecialDisplay : public CBit
{
	public:
		EXPORT CSpecialDisplay(void);
		EXPORT ~CSpecialDisplay(void) OVERRIDE;
		EXPORT virtual void Display(void);
};

class CSimpleTexturedRibbon : public CSpecialDisplay
{
	public:
		EXPORT void SetRGB(u8, u8, u8);
		EXPORT void SetSemiTransparent(void);

		EXPORT CSimpleTexturedRibbon(void);
		EXPORT CSimpleTexturedRibbon(i32);
		EXPORT virtual void Display(void) OVERRIDE;
		EXPORT void SetNumFaces(i32);
		EXPORT void SetOpaque(void);
		EXPORT void SetTexture(Texture *);
		EXPORT void SetTexture(u32);
		EXPORT void SetTexturei(i32,Texture *);
		EXPORT void SetTexturei(i32,u32);
		EXPORT void SetWidth(u16);
		EXPORT void SetWidthi(i32,u16);
		EXPORT virtual ~CSimpleTexturedRibbon(void) OVERRIDE;

		u16 field_3C;

		u16 field_3E;

		SRibbonTexture *pTextures;

		SSimpleRibbonParams *field_44;

		u32 *field_48;
};

class CGlow : public CBit
{
	public:
		EXPORT CGlow(u32, u32);
		EXPORT CGlow(CVector*, i32, i32, u8, u8, u8, u8, u8, u8);

		EXPORT virtual ~CGlow(void);

		EXPORT void SetCentreRGB(u8, u8, u8);
		EXPORT void SetRadius(i32);
		EXPORT void SetRGB(u8, u8, u8);

		EXPORT void SetFringeWidth(u32, u32);
		EXPORT void SetFringeRGB(u32, u8, u8, u8);

		SSection* mpSections;
		SFringeQuad* mpFringes;

		u32 mNumSections;
		u32 mNumFringes;
		u32 mCentreCodeBGR;
		i16 mStepAngle;
		u8 mSkipTriangles;

		u16 mAngle;
		u32 mMask;
};

class CLinked2EndedBit : public CFT4Bit
{
	public:
		EXPORT CLinked2EndedBit(void);
		EXPORT virtual ~CLinked2EndedBit(void) OVERRIDE;

		CVector field_58;
		CVector field_64;
};

class CRibbonBit : public CLinked2EndedBit
{
	public:
		EXPORT CRibbonBit(void);
		EXPORT virtual ~CRibbonBit(void) OVERRIDE;
		EXPORT virtual void Move(void);
};

class CRibbon : public CNonRenderedBit
{
	public:
		EXPORT CRibbon(CVector*, i32, i32, i32, i32, i32, i32);
		EXPORT virtual ~CRibbon(void) OVERRIDE;
		EXPORT void SetScale(i32);

		EXPORT void SetPos(CVector&);

		i32 mNumBits;
		i32 mPointsPerBit;
		i32 mNumPoints;

		i32 field_48;

		CVector *mPoints;
		CRibbonBit **mBits;
};

class CSmokeTrail : public CRibbon
{
	public:

		EXPORT CSmokeTrail(CVector*, i32, i32, i32, i32);
		EXPORT virtual void Move(void) OVERRIDE;
		EXPORT virtual ~CSmokeTrail(void) OVERRIDE;

		i32 mFadeAway;
};


/*
class CTexturedRibbon : public CSpecialDisplay
{
	public:
		EXPORT void SetOuterRGBi(int, u8, u8, u8);
		u8 topPad[0x60-0x3C];
		i32* field_60;
};
*/

class CSimpleAnim : public CFlatBit
{
	public:
		EXPORT CSimpleAnim(CVector*, i32, u16, i32, i32, i32);
		EXPORT virtual ~CSimpleAnim(void) OVERRIDE;
		EXPORT virtual void Move(void) OVERRIDE;

		i32 mDie;
		i32 mDieFrame;
};

class CMotionBlur : public CFlatBit
{
	public:
		EXPORT CMotionBlur(CVector*, CVector*, i32,i32,i32,i32);
		EXPORT virtual ~CMotionBlur(void) OVERRIDE;

		EXPORT virtual void Move(void);
};

class CCombatImpactRing : public CFlatBit
{
	public:
		EXPORT CCombatImpactRing(CVector*, u8, u8, u8, i32, i32, i32);
		EXPORT virtual ~CCombatImpactRing(void) OVERRIDE;
		EXPORT virtual void Move(void) OVERRIDE;

		i32 field_68;
		i32 field_6C;
		i32 field_70;
};

class CFrag : public CFlatBit
{
	public:
		EXPORT CFrag(CVector*, u8, u8, u8, i32, u16, i32, i32, i32, i32);
		EXPORT virtual ~CFrag(void) OVERRIDE;
		EXPORT virtual void Move(void) OVERRIDE;
};

class CPixel : public CBit
{
	public:
		EXPORT CPixel(void);
		EXPORT virtual ~CPixel(void);

		u32 tag;

		u8 r0;
		u8 g0;
		u8 b0;

		u8 code;

		u32 mWidthHeight;
};

struct SBitServerEntry
{
	void** field_0;
	void (*field_4)(void**);
};

class CBitServer : public CClass
{
	public:
		EXPORT CBitServer(void);
		EXPORT virtual ~CBitServer(void);
		EXPORT u32 RegisterSlot(void**, void (*)(void**));
		EXPORT void DisplayRegisteredSlots(void);

		u32 mNumEntries;
		SBitServerEntry mEntry[0x20];
};

class CChunkBit : public CBit
{
	public:
		EXPORT CChunkBit(CSVector*, CSVector*, CSVector*);
		EXPORT virtual ~CChunkBit(void);

		// Declared here (Mac prototypes confirm both names/signatures, tools/prototypes.json
		// group "bit") because CShatterBit's construction (Split, shatter.cpp) calls them
		// directly and non-virtually. Not implemented yet: both write into a ~52 byte field
		// range (0x94-0xC8) this class does not declare, see the @FIXME on
		// validate_CChunkBit's VALIDATE_SIZE below and the CShatterBit class comment further
		// down. Stubbed in bit.cpp, out of scope for this CShatterBit-focused session.
		EXPORT void SetRGB(u8, u8, u8);
		EXPORT void SetUVs(u16, u16, u8, u8, u8, u8, u8, u8);

		SVECTOR mPosA;
		SVECTOR mPosB;
		SVECTOR mPosC;
		SVECTOR mPosD;

		CVector mWorldPosA;
		CVector mWorldPosB;
		CVector mWorldPosC;
		CVector mWorldPosD;

		CSVector mAngles;
};

// CShatterBit : public CChunkBit. Found while decompiling Shatter_Face/Split (shatter.cpp),
// the "shattered glass triangle fragment" object created by Split()'s base case (recursion
// depth 0) via operator new(216) + CShatterBit::CShatterBit (0x48BDC0). Not present anywhere
// in the repo before this session; grep confirmed zero prior references.
//
// Evidence (2026-08-31, IDA Hex-Rays decompile + raw disasm of 0x48BDC0 ctor, 0x48BEC0 dtor,
// 0x48BF20 SetPos, 0x48C060 Move, and their call site in Split at 0x48C730):
// - sizeof == 216 (0xD8), confirmed by the operator new(216) call size in Split.
// - The ctor forwards its first 3 args straight into CChunkBit::CChunkBit(CSVector*,CSVector*,
//   CSVector*) (0x40B570, already @Ok in this repo) via sub_40B570, then overwrites the vtable
//   pointer set by that base call with CShatterBit's own vtable (off_53BE88) -- exactly the
//   codegen a real `CShatterBit : public CChunkBit` with a ctor that base-inits CChunkBit(a,b,c)
//   would produce. Confirms the inheritance.
// - CChunkBit::SetRGB (0x40B830) and CChunkBit::SetUVs (0x40B910), called directly (not
//   virtually) from Split right after construction, write into CChunkBit's OWN fields at
//   0x94-0xC8 (six UV floats, a u16, four RGB-ish dwords). Those 0x94-0xC8 bytes are legitimate
//   CChunkBit territory that the current repo's CChunkBit struct does not yet declare (see the
//   `// @FIXME` on validate_CChunkBit's VALIDATE_SIZE(CChunkBit, 0x94) in bit.cpp -- confirmed
//   here from the callee side, independent evidence). Fixing CChunkBit itself is out of scope
//   for this CShatterBit-focused session (another agent works bit.cpp's Display*List functions
//   in parallel); represented below as an explicit PADDING gap so CShatterBit's own new fields
//   land at their real offsets without touching CChunkBit's declaration.
// - Mac build prototypes (tools/prototypes.json, group "shatter") independently confirm the
//   class name, the 5-arg ctor signature (CSVector const&,CSVector const&,CSVector const&,
//   CVector const&,int), and that Move/SetPos/dtor are all real (non-inlined) member functions
//   on Mac too.
class CShatterBit : public CChunkBit
{
	public:
		EXPORT CShatterBit(CSVector const&, CSVector const&, CSVector const&, CVector const&, i32);
		EXPORT virtual ~CShatterBit(void) OVERRIDE;
		EXPORT virtual void Move(void) OVERRIDE;
		EXPORT void SetPos(const CVector& pos);

		// 0x94-0xC7 (52 bytes): CChunkBit's own real-but-undeclared UV/color fields (written by
		// CChunkBit::SetRGB/SetUVs, called on every CShatterBit from Split). Not CShatterBit's;
		// left as padding here on purpose, see class comment above.
		PADDING(0x34);

		// 0xC8: confirmed by Split (0x48C730, `mov [esi+0C8h], ecx` right after the SetUVs call,
		// ecx loaded from Split's own arg_24 = its 10th parameter, the u32 one). NOT written by
		// the constructor itself (checked: the ctor's disasm never touches 0xC8). Every call site
		// found (Split's single base-case construction) passes through a caller-supplied u32
		// untouched, so this is very likely a packed face/glass color forwarded from
		// Shatter_Face's CalcRGB output; name is our guess.
		u32 mShardColor;

		// 0xCC: CSVector, set once in the ctor to a random per-axis "spin rate" (one axis gets
		// Rnd(160)+80, i.e. 80..239, the other two are 0, chosen by a coin flip between the first
		// two axes; the third axis is always 0), then added onto the inherited CChunkBit::mAngles
		// (0x8C) every Move() tick via CSVector::operator+= (confirmed: Move's disasm calls
		// sub_4E7900, our known CSVector::operator+=, with this+0x8C as the implicit `this` and
		// this+0xCC as the explicit argument). Name is our guess (never seen in idb_globals.txt,
		// this is a member field not a global).
		CSVector mSpinRate;

		// 0xD4: confirmed CRibbon* by name resolution alone -- Move and SetPos both gate a call
		// to sub_410EB0 on "this+0xD4 != 0", and sub_410EB0 resolves in tools/names.json to
		// `?SetPos@CRibbon@@QAEXAAVCVector@@@Z` (CRibbon::SetPos(CVector&), already @Ok/declared
		// in this file). The destructor (0x48BEC0) also checks this field and, if non-null, calls
		// through its vtable slot 0 with arg 1 (the standard MSVC "scalar deleting destructor"
		// pattern, i.e. `delete mTrailRibbon;`). Never written anywhere we found in this session's
		// tracing (not by the ctor, not by Split, not by SetPos/Move) -- so nothing in the traced
		// call graph ever actually attaches a ribbon; left as a real field for fidelity, always
		// nullptr along every path we decompiled. Name is our guess.
		CRibbon* mTrailRibbon;
};

class CTextBox : public CBit
{
	public:
		EXPORT CTextBox(i32, i32, i32, i32, u32, CFriction*);
		EXPORT virtual ~CTextBox(void);

		i32 field_3C;
		PADDING(4);
};

class CFireyExplosion : public CNonRenderedBit
{
	public:
		EXPORT CFireyExplosion(CVector*);
		EXPORT virtual ~CFireyExplosion(void);

		EXPORT virtual void Move(void) OVERRIDE;
};

class CGouraudRibbon : public CSpecialDisplay
{
	public:
		EXPORT CGouraudRibbon(i32, i32);
		EXPORT void Display(void);
		EXPORT void SetRGB(u8,u8,u8);
		EXPORT void SetWidth(u16);
		EXPORT ~CGouraudRibbon(void);

		i32 mTrail;
		i32 mNumPoints;
		SRibbonPoint* mpPoints;
};

class CWibbly : public CGouraudRibbon
{
	public:
		EXPORT CWibbly(u8,u8,u8,i32,i32,i32,i32,i32,i32,i32,i32,i32,i32);
		EXPORT virtual void Move(void) OVERRIDE;
		EXPORT void SetCore(u8,u8,u8,i32);
		EXPORT void SetEndPoints(CVector *,CVector *);
		EXPORT virtual ~CWibbly(void) OVERRIDE;

		CGouraudRibbon* field_48;
		CVector field_4C;
		CVector field_58;
		CVector field_64;
		CVector field_70;
		i32 field_7C;

		i32 field_80;
		i32 field_84;

		i32 field_88;
		i32 field_8C;
		i32 field_90;

		i32 field_94;
};

class CGlassBit : public CBit
{
	public:
		EXPORT CGlassBit(CVector const *,CVector const *,i32,u8,u8,u8,i32,i32,i32);
		EXPORT virtual void Move(void) OVERRIDE;
		EXPORT virtual ~CGlassBit(void) OVERRIDE;

		CVector mPosA;
		CVector mPosB;
		CVector mPosC;

		i32 mGroundY;

		u8 mDefaultR;
		u8 mDefaultG;
		u8 mDefaultB;

		u8 mR;
		u8 mG;
		u8 mB;

		u8 mFadeRate;

		PADDING(1);
};

class CSpark : public CPixel
{
	public:
		EXPORT CSpark(CVector&,i32,i32,i32);
		EXPORT virtual void Move(void) OVERRIDE;
		EXPORT ~CSpark(void) OVERRIDE;

		u8 mFadeR;
		u8 mFadeG;
		u8 mFadeB;

		PADDING(1);
};

EXPORT extern CBit* GPolyLineList;

EXPORT i32 Bit_MakeSpriteRing(CVector*, i32, i32, i32, i32, i32, i32, i32);
EXPORT void MoveList(CBit *);
EXPORT void Bit_SetSparkRGB(u8, u8, u8);
EXPORT void Bit_SetSparkFadeRGB(u8, u8, u8);
EXPORT void Bit_SetSparkTrajectory(const CSVector *);
EXPORT void Bit_CalculateSparkVelocity(CVector&,i32);

EXPORT void Bit_SetSparkTrajectoryCone(const CSVector *);

EXPORT void Bit_ReduceRGB(u32*, i32);
EXPORT void Bit_SetSparkSize(u32);

EXPORT void DisplaySpecialDisplayList(void**);
EXPORT void DisplayTextBoxList(void**);

EXPORT void Bit_Init(void);
EXPORT void Bit_DeleteAll(void);
EXPORT void Bit_ClearTextBoxes(void);
EXPORT void Bit_UpdateQuickAnimLookups(void);
EXPORT void RemoveDeadBits(CBit *);
EXPORT void Bit_RemoveDeadBits(void);
EXPORT void MoveBits(CBit *);
EXPORT void Bit_Move(void);
EXPORT void Bit_Display(void);

EXPORT void DeleteBitList(CBit*);

EXPORT extern CTextBox* TextBoxList;

#define NUM_ANIM_ENTRIES 0x1D
EXPORT extern SAnimFrame* gAnimTable[NUM_ANIM_ENTRIES];

//#define G_ANIM_TABLE (gAnimTable)
#define G_ANIM_TABLE (reinterpret_cast<SAnimFrame**>(0x0056EA64))

EXPORT extern i32 TotalBitUsage;

void validate_CFlatBit(void);
void validate_CFT4Bit(void);
void validate_CQuadBit(void);
void validate_CBit(void);
void validate_CNonRenderedBit(void);
void validate_CSmokeTrail(void);
void validate_CGlow(void);
void validate_CLinked2EndedBit(void);
void validate_CRibbonBit(void);
//void validate_CTexturedRibbon(void);
void validate_CSimpleTexturedRibbon(void);
void validate_CSimpleAnim(void);
void validate_CMotionBlur(void);
void validate_CSpecialDisplay(void);
void validate_SFlatBitVelocity(void);
void validate_CRibbon(void);
void validate_CCombatImpactRing(void);
void validate_SRibbonPoint(void);
void validate_CFrag(void);
void validate_CPixel(void);
void validate_CBitServer(void);
void validate_CChunkBit(void);
void validate_CShatterBit(void);
void validate_CTextBox(void);
void validate_CFireyExplosion(void);
void validate_CWibbly(void);
void validate_SBitServerEntry(void);
void validate_SSection(void);
void validate_SFringeQuad(void);
void validate_vector3d(void);
void validate_vector4d(void);
void validate_CGlassBit(void);
void validate_SRibbonTexture(void);
void validate_SSimpleRibbonParams(void);
void validate_CSpark(void);


EXPORT extern CBit* GLineList;
EXPORT extern CBit* PolyLineList;

void patch_CBit(void);
void patch_CFT4Bit(void);

#endif
