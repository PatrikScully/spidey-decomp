#include "bit2.h"
#include "ob.h"
#include "mem.h"
#include "my_assert.h"
#include "utils.h"

#include "validate.h"

// @Ok
// @Matching
CGLineParticle::CGLineParticle(
		CVector &a2,
		CVector &a3,
		u16 a4,
		i32 a5)
{
	this->mEnd = a2;
	this->mStart = this->mEnd;

	this->mVel = a3;
	this->mLifetime = a4;
	this->field_5C = G_TIMER_RELATED;

	this->mAcc.vy = a5 << 12;
}

// @Ok
// @Matching
void CGLineParticle::Move(void)
{
	u32 timeDiff = G_TIMER_RELATED - this->field_5C;
	if (this->mLifetime < timeDiff)
	{
		this->Die();
	}
	else
	{
		this->mLifetime -= timeDiff;
		this->field_5C = G_TIMER_RELATED;

		this->mEnd.vx = this->mStart.vx;
		this->mEnd.vy = this->mStart.vy;
		this->mEnd.vz = this->mStart.vz;

		this->mVel.vy += this->mAcc.vy;

		this->mStart.vx += this->mVel.vx * timeDiff;
		this->mStart.vy += this->mVel.vy * timeDiff;
		this->mStart.vz += this->mVel.vz * timeDiff;
	}
}

// @Ok
// @Matching
CGLineParticle::~CGLineParticle(void)
{
}


// @Ok
// @Test
void CGPolyLine::SetStartAndEnd(
		CVector *pStart,
		CVector *pEnd)
{

	CVector segment = (*pEnd - *pStart) / this->mNumSegs;
	CVector current = *pStart;

	this->mStart = *pStart;

	for (i32 i = 0; i < this->mNumSegs; i++)
	{
		current += segment;
		this->mSegs[i].End = current;
	}
}

// @Ok
CGPolyLine::~CGPolyLine()
{
	Mem_Delete(this->mSegs);
	this->DeleteFrom(&G_GPOLYLINE_LIST);
}

// @Ok
// @Test
void CGPolyLine::SetSemiTransparent(void)
{
	for (i32 i = 0; i < this->mNumSegs; i++)
	{
		this->mSegs[i].code |= 2;
	}

	this->mCode |= 2;
}

// @Ok
// @Note: not matching because there's a single write for the r,g,b,code
// it can match with that commented part but i don't want to do it that way
CGPolyLine::CGPolyLine(i32 numsegs)
{
	this->mStart.vx = 0;
	this->mStart.vy = 0;
	this->mStart.vz = 0;

	print_if_false((numsegs > 0 && numsegs < 100), "Bad numsegs sent to gpolyline constructor");

	this->mSegs = static_cast<SLineSeg*>(DCMem_New(16 * numsegs, 0, 1, 0, 1));

	for (i32 i = 0; i < numsegs; i++)
	{
		/*
		i32* dst = reinterpret_cast<i32*>(&this->mSegs[i].r);
		*dst = 0x50FFFFFF;
		*/

		this->mSegs[i].r = 0xFF;
		this->mSegs[i].g = 0xFF;
		this->mSegs[i].b = 0xFF;
		this->mSegs[i].code = 0x50;

		this->mSegs[i].End = ZeroVector;


	}

	this->mNumSegs = numsegs;
	this->AttachTo(&G_GPOLYLINE_LIST);
	this->mStartR = -1;

	this->mStartG = -1;
	this->mStartB = -1;
	this->mCode = 80;
}

// Segment count for CKnottedWeb: distance between the two endpoints, one segment per 80
// units, clamped to [1, 40]. Split out of the constructor because the original computes it
// before calling the CGPolyLine base constructor (this needs to happen in a member
// initializer, which can't hold the clamping if/else directly). Not a separate function in
// the original (inlined into 0x4F8E20, no address of its own), but real decompiled logic.
// @Ok
static i32 KnottedWeb_NumSegs(const CVector &start, const CVector &end)
{
	i32 numSegs = (end - start).Length() / 80;

	if (numSegs < 1)
		numSegs = 1;
	else if (numSegs > 40)
		numSegs = 40;

	return numSegs;
}

// @Ok
// residue: field_C/field_D/field_10/field_11 in SKnottedWebSeg are placeholder names (see
// bit2.h). The values and their Rnd() ranges are taken straight from the disasm.
CKnottedWeb::CKnottedWeb(const CVector &start, const CVector &end)
	: CGPolyLine(KnottedWeb_NumSegs(start, end))
{
	this->field_58 = 0;
	this->field_5C = 0;
	this->field_60 = 0;

	this->mStartR = 0xA2;
	this->mStartG = 0xA2;
	this->mStartB = 0xA2;

	for (i32 i = 0; i < this->mNumSegs; i++)
	{
		this->mSegs[i].r = 0xA2;
		this->mSegs[i].g = 0xA2;
		this->mSegs[i].b = 0xA2;
	}

	this->mpExtraSegs = static_cast<SKnottedWebSeg*>(DCMem_New(28 * this->mNumSegs, 0, 1, 0, 1));

	for (i32 j = 0; j < this->mNumSegs; j++)
	{
		SKnottedWebSeg *pSeg = &this->mpExtraSegs[j];

		pSeg->field_C = 4;
		pSeg->field_D = static_cast<u8>(Rnd(100) + 125);
		pSeg->field_E = static_cast<i16>(Rnd(4096));
		pSeg->field_11 = static_cast<u8>(Rnd(192) + 64);
		pSeg->field_10 = static_cast<u8>(Rnd(19) - 9);

		CFlatBit *pBit = new CFlatBit();
		pBit->SetSemiTransparent();
		pBit->SetAnim(4);
		pBit->SetScale(176);
		pBit->mProtected = 1;

		pSeg->mpBit = pBit;
	}

	this->mpInnerLine = new CGPolyLine(2 * this->mNumSegs);
	this->mpInnerLine->mStartR = 0xA2;
	this->mpInnerLine->mStartG = 0xA2;
	this->mpInnerLine->mStartB = 0xA2;

	for (i32 k = 0; k < 2 * this->mNumSegs; k++)
	{
		this->mpInnerLine->mSegs[k].r = 0xA2;
		this->mpInnerLine->mSegs[k].g = 0xA2;
		this->mpInnerLine->mSegs[k].b = 0xA2;
	}

	this->field_70 = 1;
}

// @Ok
// @AlmostMatching: diff reg allocation for the reg for the loop
CPolyLine::CPolyLine(i32 numsegs)
{
	DoAssert(numsegs > 0 && numsegs < 100, "Bad numsegs sent to polyline constructor");

	this->mSegs = static_cast<SLineSeg *>(
			DCMem_New(sizeof(SLineSeg) * numsegs, 0, 1, 0, 1));

	for (i32 i = 0; i < numsegs; i++)
	{
		SLineSeg *pSeg = &this->mSegs[i];
		// @FIXME - affects portability, but it's how the devs did it
		*reinterpret_cast<u32*>(&pSeg->r) = 0x40FFFFFF;

		pSeg->End = ZeroVector;
	}


	this->mNumSegs = numsegs;
	this->AttachTo(&G_POLYLINE_LIST);
}

// @Ok
// @Matching
INLINE CGLine::CGLine(void)
{
	this->mCodeBGR0 = 0x50808080;
	this->mPadBGR1 = 0x55808080;
	this->AttachTo(&G_GLINE_LIST);
}

// @Ok
// @Matching
CGLine::~CGLine(void)
{
	this->DeleteFrom(&G_GLINE_LIST);
}

// @Ok
// @Matching
void CGLine::SetRGB0(u8 a2, u8 a3, u8 a4)
{
	this->mCodeBGR0 = (this->mCodeBGR0 & 0xFF000000) | (a4 << 16) | (a3 << 8) | a2;
}

// @Ok
// @Matching
void CGLine::SetRGB1(u8 a2, u8 a3, u8 a4)
{
	this->mPadBGR1 = a2 | (a4 << 16) | (a3 << 8);
}

// @Ok
// @Matching: man, the pointer thingy is needed
void CPolyLine::SetSemiTransparent(void)
{
	for (i32 i = 0; i < this->mNumSegs; i++)
	{
		SLineSeg *pSeg = &this->mSegs[i];
		pSeg->code |= 2;
	}
}

// @Ok
// @Matching
CPolyLine::~CPolyLine(void)
{
	Mem_Delete(this->mSegs);
	this->DeleteFrom(&G_POLYLINE_LIST);
}

// @Ok
// @Note: missing the SEH handler on the OG and mPos assingment is different
CSmokeGenerator::CSmokeGenerator(
		const CVector *a2,
		i32 a3,
		i32 a4,
		u8 a5,
		u8 a6,
		u8 a7,
		i32 a8,
		i32 a9,
		i32 a10,
		i32 a11)
{
	DoAssert(a3 != 0, "Smoke duration must be non zero");

	this->mPos = *a2;
	this->mLifetime = a3;

	this->mPuffs = a4;

	this->mR = a5;
	this->mG = a6;
	this->mB = a7;

	this->mVBase = a8;
	this->mVRandom = a9;

	this->mScaleBase = a10;
	this->mScaleRandom = a11;

	this->mFrigDeltaZ = 100;
}

// @Ok
// @Matching
void CSmokeGenerator::Move(void)
{
	CVector v8;

	for (i32 i = 0; i < this->mPuffs; i++)
	{
		v8.vy = -4096 * (this->mVBase + Rnd(this->mVRandom));

		CMotionBlur *pBlur = new CMotionBlur(
				&this->mPos,
				&v8,
				1,
				this->mScaleBase + Rnd(this->mScaleRandom),
				0,
				10);

		pBlur->mFrigDeltaZ = this->mFrigDeltaZ;
		pBlur->mAngle = Rnd(4096);
		
		pBlur->SetTint(this->mR, this->mG, this->mB);
	}

	if (++this->mAge == this->mLifetime && this->mLifetime != 0xFFFF)
	{
		this->Die();
	}
}

// @Ok
// @Matching
CSmokeGenerator::~CSmokeGenerator(void)
{
}

void validate_CGPolyLine(void){
	VALIDATE_SIZE(CGPolyLine, 0x58);

	VALIDATE(CGPolyLine, mNumSegs, 0x3C);

	VALIDATE(CGPolyLine, mSegs, 0x40);
	VALIDATE(CGPolyLine, mStart, 0x44);

	VALIDATE(CGPolyLine, mStartR, 0x50);
	VALIDATE(CGPolyLine, mStartG, 0x51);
	VALIDATE(CGPolyLine, mStartB, 0x52);
	VALIDATE(CGPolyLine, mCode, 0x53);

	VALIDATE(CGPolyLine, field_57, 0x57);
}

void validate_CKnottedWeb(void)
{
	VALIDATE_SIZE(CKnottedWeb, 0x78);

	VALIDATE(CKnottedWeb, field_58, 0x58);
	VALIDATE(CKnottedWeb, field_5C, 0x5C);
	VALIDATE(CKnottedWeb, field_60, 0x60);

	VALIDATE(CKnottedWeb, mpExtraSegs, 0x64);
	VALIDATE(CKnottedWeb, mpInnerLine, 0x68);

	VALIDATE(CKnottedWeb, field_6D, 0x6D);
	VALIDATE(CKnottedWeb, field_6E, 0x6E);
	VALIDATE(CKnottedWeb, field_70, 0x70);
	VALIDATE(CKnottedWeb, field_74, 0x74);
}

void validate_SKnottedWebSeg(void)
{
	VALIDATE_SIZE(SKnottedWebSeg, 0x1C);

	VALIDATE(SKnottedWebSeg, mPos, 0x0);
	VALIDATE(SKnottedWebSeg, field_C, 0xC);
	VALIDATE(SKnottedWebSeg, field_D, 0xD);
	VALIDATE(SKnottedWebSeg, field_E, 0xE);
	VALIDATE(SKnottedWebSeg, field_10, 0x10);
	VALIDATE(SKnottedWebSeg, field_11, 0x11);
	VALIDATE(SKnottedWebSeg, mpBit, 0x14);
}

void validate_CGLine(void)
{
	VALIDATE_SIZE(CGPolyLine, 0x58);

	VALIDATE(CGLine, mCodeBGR0, 0x3C);
	VALIDATE(CGLine, mPadBGR1, 0x40);

	VALIDATE(CGLine, mStart, 0x44);

	VALIDATE(CGLine, mEnd, 0x50);
}

void validate_CPolyLine(void)
{
	VALIDATE_SIZE(CPolyLine, 0x54);

	VALIDATE(CPolyLine, mNumSegs, 0x40);
	VALIDATE(CPolyLine, mSegs, 0x44);
	VALIDATE(CPolyLine, mStart, 0x48);
}

void validate_SLineSeg(void)
{
	VALIDATE_SIZE(SLineSeg, 0x10);

	VALIDATE(SLineSeg, End, 0x0);

	VALIDATE(SLineSeg, r, 0xC);
	VALIDATE(SLineSeg, g, 0xD);
	VALIDATE(SLineSeg, b, 0xE);
	VALIDATE(SLineSeg, code, 0xF);
}

void validate_CSmokeGenerator(void)
{
	VALIDATE_SIZE(CSmokeGenerator, 0x54);

	VALIDATE(CSmokeGenerator, mPuffs, 0x3C);

	VALIDATE(CSmokeGenerator, mR, 0x40);
	VALIDATE(CSmokeGenerator, mG, 0x41);
	VALIDATE(CSmokeGenerator, mB, 0x42);

	VALIDATE(CSmokeGenerator, mVBase, 0x44);
	VALIDATE(CSmokeGenerator, mVRandom, 0x48);


	VALIDATE(CSmokeGenerator, mScaleBase, 0x4C);
	VALIDATE(CSmokeGenerator, mScaleRandom, 0x50);
}

void validate_CGLineParticle(void)
{
	VALIDATE_SIZE(CGLineParticle, 0x60);

	VALIDATE(CGLineParticle, field_5C, 0x5C);
}

#include "my_patch.h"

// Same two rules as patch_bit in bit.cpp: virtuals and constructors/destructors
// go through PATCH_PUSH_RET_POLY, and nothing that draws is hooked (the three
// line renderers for these classes live in bit.cpp and all go through
// PCGfx_DrawLine).
//
// CKnottedWeb is skipped. Its constructor is at 0x4F8E20 and its vtable has
// CKnottedWeb::~CKnottedWeb (0x4F9060) and CKnottedWeb::Move (0x4F9150), and
// this repo declares neither, so a hooked constructor would lose both slots.

// @Bogus
void patch_bit2(void)
{
	PATCH_PUSH_RET_POLY(0x00411D50, CPolyLine::CPolyLine, "??0CPolyLine@@QAE@H@Z");
	PATCH_PUSH_RET_POLY(0x00411E50, CPolyLine::~CPolyLine, "??1CPolyLine@@UAE@XZ");
	PATCH_PUSH_RET(0x00411EC0, CPolyLine::SetSemiTransparent);

	PATCH_PUSH_RET_POLY(0x00412350, CGPolyLine::CGPolyLine, "??0CGPolyLine@@QAE@H@Z");
	PATCH_PUSH_RET_POLY(0x00412450, CGPolyLine::~CGPolyLine, "??1CGPolyLine@@UAE@XZ");
	PATCH_PUSH_RET(0x004124C0, CGPolyLine::SetSemiTransparent);
	PATCH_PUSH_RET(0x00412500, CGPolyLine::SetStartAndEnd);

	PATCH_PUSH_RET_POLY(0x00412A30, CSmokeGenerator::CSmokeGenerator, "??0CSmokeGenerator@@QAE@PBVCVector@@HHEEEHHHH@Z");
	PATCH_PUSH_RET_POLY(0x00412AE0, CSmokeGenerator::~CSmokeGenerator, "??1CSmokeGenerator@@UAE@XZ");
	PATCH_PUSH_RET_POLY(0x00412AF0, CSmokeGenerator::Move, "?Move@CSmokeGenerator@@UAEXXZ");

	PATCH_PUSH_RET_POLY(0x00412C00, CGLine::CGLine, "??0CGLine@@QAE@XZ");
	PATCH_PUSH_RET_POLY(0x00412C90, CGLine::~CGLine, "??1CGLine@@UAE@XZ");
	PATCH_PUSH_RET(0x00412CF0, CGLine::SetRGB0);
	PATCH_PUSH_RET(0x00412D20, CGLine::SetRGB1);

	PATCH_PUSH_RET_POLY(0x00412D40, CGLineParticle::CGLineParticle, "??0CGLineParticle@@QAE@AAVCVector@@0GH@Z");
	PATCH_PUSH_RET_POLY(0x00412E30, CGLineParticle::~CGLineParticle, "??1CGLineParticle@@UAE@XZ");
	PATCH_PUSH_RET_POLY(0x00412E90, CGLineParticle::Move, "?Move@CGLineParticle@@UAEXXZ");
}
