#include "export.h"
#include "bit.h"
#include "mem.h"
#include <cstring>
#include <cstdlib>
#include "validate.h"
#include "spool.h"
#include "utils.h"
#include "ps2lowsfx.h"
#include "my_assert.h"
#include "db.h"
#include "panel.h"
#include "ps2funcs.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"

// @Ok
EXPORT bool SparkSemiTrans = true;

// @Ok
EXPORT CSVector SparkTrajectoryCone;

// @Ok
EXPORT CSVector SparkTrajectory;

// @Ok
EXPORT u8 gSparkRGB[3] = { 0x80, 0x80, 0x80 };

// @Ok
EXPORT u8 gSparkFadeRGB[3] = { 4, 4, 4 };

// @Ok
EXPORT char *gAnimNames[29] =
{
	"SHADOW  ",
	"SMOKE   ",
	"ribbon  ",
	"Buttons ",
	"WebKnot ",
	"Reticle ",
	"WebSplat",
	"Sp      ",
	"HITSPRIT",
	"ImpactWb",
	"HitPing ",
	"Compass ",
	"WebCart ",
	"LoadIcon",
	"Misc",

	"menubox ",
	"fireimp",
	"costarm",
	"cost99",
	"costbag",
	"costblk",
	"costcapt",

	"costscar",
	"xtri",

	"symdrop",
	"trail",
	"slime",
	"splats",
	"fire_rock03"
};



// @NotOk
// @FIXME: must be zero initialized
SAnimFrame* gAnimTable[0x1D];

EXPORT CChunkBit* ChunkBitList;
EXPORT CGlow* GlowList;
CTextBox* TextBoxList = 0;

EXPORT volatile i32 BitCount = 0;

//#define G_BITCOUNT (BitCount)
#define G_BITCOUNT (*reinterpret_cast<volatile i32*>(0x0056EB48))

i32 TotalBitUsage = 0;

EXPORT CFlatBit *FlatBitList;

// @Ok
EXPORT CSpecialDisplay *SpecialDisplayList = 0;
//#define G_SPECIALDISPLAY_LIST (SpecialDisplayList)
#define G_SPECIALDISPLAY_LIST (*reinterpret_cast<CSpecialDisplay**>(0x0056EB34))


EXPORT CPixel* PixelList;

u32 SparkSize = 1;

// @FIXME - is it really?
volatile i32 gTimerRelated;

// @Ok
EXPORT CNonRenderedBit* NonRenderedBitList = 0;
//#define G_NONRENDEREDBIT_LIST (NonRenderedBitList)
#define G_NONRENDEREDBIT_LIST (*reinterpret_cast<CNonRenderedBit**>(0x0056EAD8))


EXPORT CBit* Linked2EndedBitListLeftover;
CBit* PolyLineList;
CBit* GPolyLineList;
EXPORT CBit* QuadBitList;
EXPORT CBit* GenPolyList;
EXPORT CBit* GlassList;
CBit* GLineList;

EXPORT CBitServer* gBitServer = 0;

// @Ok
// @Validate: when inlined
INLINE void Bit_CalculateSparkVelocity(CVector &a1, i32 a2)
{
	CSVector v11 = SparkTrajectory;

	if (SparkTrajectoryCone.vx)
	{
		v11.vx += Rnd(SparkTrajectoryCone.vx) - (SparkTrajectory.vx >> 1);
	}

	if (SparkTrajectoryCone.vy)
	{
		v11.vy += Rnd(SparkTrajectoryCone.vy) - (SparkTrajectory.vy >> 1);
	}

	if (SparkTrajectoryCone.vz)
	{
		v11.vz += Rnd(SparkTrajectoryCone.vz) - (SparkTrajectory.vz >> 1);
	}

	Utils_GetVecFromMagDir(&a1, a2, &v11);
}

// @Ok
// @NotMatching: different register alloc totally diff registers
// the call to calculate spark velocity is lsight differnt but all good
CSpark::CSpark(
		CVector& a2,
		i32 a3,
		i32 a4,
		i32 a5)
{
	if ((SparkSize & 0xF) != 1)
	{
		this->code = 96;
		this->tag = 0x3000000;
		this->mWidthHeight = SparkSize;
	}
	else
	{
		this->code = 104;
		this->tag = 0x2000000;
		this->mWidthHeight = 1;
	}

	this->r0 = gSparkRGB[0];
	this->g0 = gSparkRGB[1];
	this->b0 = gSparkRGB[2];

	this->mFadeR = gSparkFadeRGB[0];
	this->mFadeG = gSparkFadeRGB[1];
	this->mFadeB = gSparkFadeRGB[2];

	this->mPos = a2;

	Bit_CalculateSparkVelocity(this->mVel, a3);

	this->mAcc.vy = a4;

	this->mFric.Set(3, 3, 3);

	this->mLifetime = Rnd(a5);

	if (SparkSemiTrans)
	{
		this->code |= 2u;
	}
}

// @Ok
// @Matching
void CSpark::Move(void)
{
	this->mPos.vx += this->mVel.vx;
	this->mPos.vy += this->mVel.vy;
	this->mPos.vz += this->mVel.vz;

	i32 newVelX = this->mVel.vx + this->mAcc.vx;
	i32 newVelY = this->mVel.vy + this->mAcc.vy;
	i32 newVelZ = this->mVel.vz + this->mAcc.vz;

	this->mVel.vx = newVelX - (newVelX >> 3);
	this->mVel.vy = newVelY - (newVelY >> 3);
	this->mVel.vz = newVelZ - (newVelZ >> 3);

	if (++this->mAge >= this->mLifetime)
	{
		this->Die();
	}

	this->r0 -= this->mFadeR;
	this->g0 -= this->mFadeG;
	this->b0 -= this->mFadeB;
}

// @Ok
// @Matching
CSpark::~CSpark(void)
{
}

// @Ok
INLINE CSpecialDisplay::~CSpecialDisplay(void)
{
	this->DeleteFrom(&G_SPECIALDISPLAY_LIST);
}

// @Ok
// base vtable slot for Display, needed by CSpecialDisplay subclasses that do not override it (e.g. CTexturedRibbon)
void CSpecialDisplay::Display(void)
{
}

// @Ok
// @AlmostMatching: slightly out of order due ot the AttachTo
CSimpleTexturedRibbon::CSimpleTexturedRibbon(i32 numfaces)
{
	this->SetNumFaces(numfaces);
}


// @BIGTODO
// Address 0x40aa00 (names.json: CSimpleTexturedRibbon_Display). Full 3D
// ribbon renderer: per-segment camera-space transform through the view
// matrix (dword_56E668 block), perspective divide, and a strip of gouraud
// POLY_GT4 quads via sub_508550, one per pair of ribbon points. Re-tagged
// from @MEDIUMTODO: this is full 3D rendering work, not a stub-sized task.
void CSimpleTexturedRibbon::Display(void)
{
    printf("CSimpleTexturedRibbon::Display(void)");
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetNumFaces(i32 a1)
{
	DoAssert(a1 != 0, "Zero NumFaces");
	this->field_3C = a1;
	this->field_3E = a1;

	i32 texSize = this->field_3C * sizeof(SRibbonTexture);
	this->pTextures = static_cast<SRibbonTexture *>(DCMem_New(
		texSize,
		0,
		1,
		0,
		1));

	u32 v4 = 0;
	u32 v5 = 0;

	SRibbonTexture *pTexture = this->pTextures;

	for (i32 i = 0; i < this->field_3C; i++)
	{
		pTexture->field_0 = 0xA01;
		pTexture->field_2 = 32;


		pTexture->field_4 = v4;
		pTexture->field_5 = v4 + 2;
		pTexture->field_6 = v4 + 1;

		pTexture->field_7 = v4 + 3;
		v4 += 2;

		pTexture->field_8 = v5;
		pTexture->field_9 = v5 + 1;
		pTexture->field_A = v5;
		pTexture->field_B = v5 + 1;

		pTexture->field_C = 0;
		pTexture->field_E = 0;

		v5++;

		pTexture++;
	}

	i32 ffSize = sizeof(SSimpleRibbonParams) *  (a1+1);
	this->field_44 = static_cast<SSimpleRibbonParams*>(
			DCMem_New(ffSize, 0, 1, 0, 1));

	i32 feSize = sizeof(u32) * (a1+1);
	this->field_48 = static_cast<u32*>(
			DCMem_New(feSize, 0, 1, 0, 1));
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetOpaque(void)
{
	SRibbonTexture *pTexture = this->pTextures;
	for (i32 i = 0; i < this->field_3C; i++)
	{
		pTexture->field_0 |= 0x80;
		pTexture++;
	}
}


// @Ok
// @Matching
void CSimpleTexturedRibbon::SetTexture(Texture *a2)
{
	DoAssert(a2 != 0, "NULL pTex sent to SetTexture");

	SRibbonTexture *pTexture = this->pTextures;

	for (i32 i = 0; i < this->field_3C; i++)
	{
		pTexture->mPage = a2->tpage;
		pTexture->mClut = a2->clut;
		pTexture->u0 = a2->u0;
		pTexture->u1 = a2->u1;
		pTexture->v0 = a2->v0;
		pTexture->v1 = a2->v1;
		pTexture->u2 = a2->u2;
		pTexture->u3 = a2->u3;
		pTexture->v2 = a2->v2;
		pTexture->v3 = a2->v3;
		pTexture->mTexWin = a2->TexWin;
		++pTexture;
	}
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetTexture(u32 checksum)
{
	Texture *TextureEntry = Spool_FindTextureEntry(checksum);
	DoAssert(TextureEntry != 0, "Could not find texture for ribbon");
	if (!TextureEntry)
	{
		TextureEntry = gAnimTable[13]->pTexture;
	}

	this->SetTexture(TextureEntry);
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetTexturei(i32 a2, Texture *a3)
{
	DoAssert(a2 < this->field_3C, "Bad i in call to CSimpleTexturedRibbon::SetTexturei");
	DoAssert(a3 != 0, "NULL pTex sent to SetTexturei");

	this->pTextures[a2].mPage = a3->tpage;
	this->pTextures[a2].mClut = a3->clut;

	this->pTextures[a2].u0 = a3->u0;

	this->pTextures[a2].u1 = a3->u1;
	this->pTextures[a2].v0 = a3->v0;

	this->pTextures[a2].v1 = a3->v1;

	this->pTextures[a2].u2 = a3->u2;
	this->pTextures[a2].u3 = a3->u3;

	this->pTextures[a2].v2 = a3->v2;

	this->pTextures[a2].v3 = a3->v3;

	this->pTextures[a2].mTexWin = a3->TexWin;
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetTexturei(i32 a1, u32 a2)
{
	DoAssert(a1 < this->field_3C, "Bad i in call to CSimpleTexturedRibbon::SetTexturei");
	Texture *TextureEntry = Spool_FindTextureEntry(a2);
	DoAssert(TextureEntry != 0, "Could not find texture for ribbon");

	this->SetTexturei(a1, TextureEntry);
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetWidth(u16 a2)
{
	SSimpleRibbonParams *pParam = this->field_44;
	for (i32 i = 0; i < this->field_3C + 1; i++)
	{
		pParam->field_18 = a2;
		++pParam;
	}
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetWidthi(i32 a2, u16 a3)
{
	DoAssert(a2 < this->field_3C + 1, "Bad i in call to CSimpleTexturedRibbon::SetWidthi");
	this->field_44[a2].field_18 = a3;
}

// @Ok
CSimpleTexturedRibbon::~CSimpleTexturedRibbon(void)
{
	Mem_Delete(this->pTextures);
	Mem_Delete(this->field_44);
	Mem_Delete(this->field_48);
}

// @Ok
// @Matching
CSimpleTexturedRibbon::CSimpleTexturedRibbon(void)
{
}

// @Ok
// @Matching
void CSimpleTexturedRibbon::SetSemiTransparent(void)
{
	SRibbonTexture *pTexture = this->pTextures;
	for (i32 i = 0; i < this->field_3C; i++)
	{
		pTexture->field_0 |= 0xC0;
		pTexture++;
	}
}

// @Ok
// @Matching
void Bit_RemoveDeadBits(void)
{
	RemoveDeadBits(NonRenderedBitList);
	RemoveDeadBits(TextBoxList);
	RemoveDeadBits(FlatBitList);
	RemoveDeadBits(Linked2EndedBitListLeftover);

	RemoveDeadBits(PixelList);
	RemoveDeadBits(PolyLineList);

	RemoveDeadBits(GPolyLineList);

	RemoveDeadBits(QuadBitList);
	RemoveDeadBits(GenPolyList);
	RemoveDeadBits(ChunkBitList);

	RemoveDeadBits(GlowList);
	RemoveDeadBits(GlassList);
	RemoveDeadBits(GLineList);
	RemoveDeadBits(SpecialDisplayList);
}

// @Ok
// @Matching
INLINE void RemoveDeadBits(CBit *pBit)
{
	if (pBit)
 	{
 		CBit *pNext = pBit->mNext;
 		while (1)
 		{
 			if (pBit->mDead)
 				delete pBit;
 
 			pBit = pNext;
 
 			if (!pBit)
 				break;
 
 			pNext = pNext->mNext;
 		}
 	}
}

// @Ok
// @AlmostMatching: CFriction::Set was not inlined and attachto seems different too
CQuadBit::CQuadBit(void)
{
	this->AttachTo(&QuadBitList);
	this->mCodeBGR = 0x1C0001;
	this->field_68 = 0x2030001;
	this->mTint = 0x808080;
	this->field_70 = 0;
}

// @Ok
// Functional: field_0[0..2] = a1,a2,a3 is unambiguously correct for a
// 3-float constructor regardless of calling convention. Note for whoever
// revisits byte-matching: names.json labels 0x402540 as this constructor
// (??0vector3d@@QAE@MMM@Z, 3 floats), and every call site in this file
// passes it a single pointer to 3 packed floats, and cmpsum shows the
// original does "ret 4" (pops one stack arg) where our 3-stack-float
// __thiscall does "ret 0Ch" (pops three) -- so 0x402540 most likely is
// NOT this f32,f32,f32 overload but a different vector3d(f32*)-shaped
// helper that the names.json entry mislabels. Not chased further since
// this session's bar is functional correctness, not byte match.
vector3d::vector3d(f32 a1, f32 a2, f32 a3)
{
	this->field_0[0] = a1;
	this->field_0[1] = a2;
	this->field_0[2] = a3;
}

// @Ok
INLINE vector4d::vector4d(const vector3d& a1, f32 a2)
{
	this->field_0[0] = a1.field_0[0];
	this->field_0[1] = a1.field_0[1];
	this->field_0[2] = a1.field_0[2];

	this->field_0[3] = a2;
}

// @Ok
// @Matching
INLINE void DeleteBitList(CBit *pBitList)
{
	if (!pBitList)
	{
		return;
	}

	CBit* pCurr = pBitList;
	CBit* pNext = pCurr->mNext;
	while (pCurr)
	{
		if (!pCurr->mProtected)
		{
			delete pCurr;
		}

		pCurr = pNext;

		if (!pNext)
		{
			break;
		}

		pNext = pNext->mNext;
	}
}

// @Ok
// @Matching
// Disassembled with IDA (0x40D630, 32 bytes). Not a call to DeleteBitList:
// it walks TextBoxList and deletes every entry unconditionally, ignoring
// mProtected, with the next pointer read before the delete. The asm has a
// genuinely redundant "if (p)" check inside the loop body (test ecx,ecx
// right after the mNext load), which this source shape reproduces exactly.
void Bit_ClearTextBoxes(void)
{
	CBit *p = TextBoxList;
	while (p)
	{
		CBit *pNext = p->mNext;
		if (p)
		{
			delete p;
		}
		p = pNext;
	}
}

// @Ok
// @AlmostMatching: my inlining stops at ChunkBitList while OG stops at GlowList
void Bit_DeleteAll(void)
{
	DeleteBitList(NonRenderedBitList);
	DeleteBitList(TextBoxList);
	DeleteBitList(FlatBitList);
	DeleteBitList(Linked2EndedBitListLeftover);
	DeleteBitList(PixelList);
	DeleteBitList(PolyLineList);
	DeleteBitList(GPolyLineList);
	DeleteBitList(QuadBitList);
	DeleteBitList(GenPolyList);
	DeleteBitList(ChunkBitList);
	DeleteBitList(GlowList);
	DeleteBitList(GlassList);
	DeleteBitList(GLineList);
	DeleteBitList(SpecialDisplayList);

	DoAssert(NonRenderedBitList == 0, "NonRenderedBitList  Leftover protected bits!");
	DoAssert(TextBoxList == 0, "TextBoxList  Leftover protected bits!");
	DoAssert(FlatBitList == 0, "FlatBitList  Leftover protected bits!");
	DoAssert(Linked2EndedBitListLeftover == 0, "Linked2EndedBitListLeftover protected bits!");
	DoAssert(PixelList == 0, "PixelList  Leftover protected bits!");
	DoAssert(PolyLineList == 0, "PolyLineList  Leftover protected bits!");
	DoAssert(GPolyLineList == 0, "GPolyLineList  Leftover protected bits!");
	DoAssert(QuadBitList == 0, "QuadBitList  Leftover protected bits!");
	DoAssert(GenPolyList == 0, "GenPolyList  Leftover protected bits!");
	DoAssert(ChunkBitList == 0, "ChunkBitList  Leftover protected bits!");
	DoAssert(GlowList == 0, "GlowList  Leftover protected bits!");
	DoAssert(GlassList == 0, "GlassList  Leftover protected bits!");
	DoAssert(GLineList == 0, "GLineList  Leftover protected bits!");
	DoAssert(SpecialDisplayList == 0, "SpecialDisplayList  Leftover protected bits!");

	DoAssert(G_BITCOUNT == 0, "Still some bits left");
}

// @BIGTODO
// Address 0x412f10 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Real 3D pipeline code: camera-space transform of the
// two ribbon endpoints through the view matrix (dword_56E668 block), clip
// test against dword_64E514, perspective divide, then a gouraud quad via
// sub_509000. Re-tagged from @MEDIUMTODO: this is full 3D rendering work,
// not a stub-sized task.
void DisplayGLineList(void**)
{
}

// @Ok
// @Matching
void DisplaySpecialDisplayList(void** a1)
{
	CSpecialDisplay* p = reinterpret_cast<CSpecialDisplay*>(*a1);
	while (p)
	{
		p->Display();
		p = reinterpret_cast<CSpecialDisplay*>(p->mNext);
	}
}

// @BIGTODO
// Address 0x411560 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Full 3D pipeline: per-vertex camera-space transform
// (sub_46D8A0/8D0/900), clip test, matrix/vector transform (DCX_XformVector,
// sub_402700) and two gouraud POLY_GT4 emits via sub_508550. Re-tagged from
// @MEDIUMTODO.
void DisplayGlassList(void**)
{
}

// @BIGTODO
// Address 0x40c6f0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Largest of the Display*List family: WPlane/CVector
// math (0x4e7840/0x4e7760, the wrongly-INLINE CVector::operator>>/operator-
// noted elsewhere in this file's history), a fringe/glow ring loop, and
// several sub_508550 gouraud quad emits per fringe. Re-tagged from
// @MEDIUMTODO.
void DisplayGlowList(void**)
{
}

// @BIGTODO
// Address 0x40bac0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Builds 4 WPlane objects from the chunk's 4 corner
// verts, normalizes each with sub_402700/402600/402560/402540, then draws
// the 4 chunk faces with sub_5081F0. Re-tagged from @MEDIUMTODO.
void DisplayChunkBitList(void**)
{
}

// @BIGTODO
// Address 0x4097e0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Full camera-space quad transform (4 corners through
// sub_46D790/sub_46DF80 and DCX_XformVector), perspective divide per corner,
// two sub_508550 gouraud quad emits (front/back face). Re-tagged from
// @MEDIUMTODO.
void DisplayQuadBitList(void**)
{
}

// @Ok
// Functional (session-wide functional-only bar, 2026-08-30). Address 0x40d7c0,
// found by tracing CBitServer::RegisterSlot calls inside Bit_Init (0x407fc0):
// the first 6 slots (TextBoxList..GPolyLineList) register through an inlined
// hashtable insert instead of a visible RegisterSlot call, so the callback
// addresses do not show up in names.json. Field offsets verified against the
// CBit VALIDATE block below: mFric is CFriction (3 u8, offset 0x34) and holds
// the box color, mAge/mLifetime (0xC/0xE) drive a 16-frame pop-in/pop-out grow
// animation, mPos.vx/vy (0x10/0x14) and mVel.vx/vy (0x1C/0x20) hold position
// and size (CVector members are i32 in this codebase, not floats, so no float
// conversion happens, matching the raw dword loads in the disassembly). This
// was attempted once before in this branch's history and reverted as
// "unverified"; the revert was correct procedure at the time (no address was
// known), but every field and call in that draft turns out to match this
// decompile exactly, so it is restored here with the address now confirmed.
// Coordinates are stored in the logical Xres x Yres space and scaled to the
// real screen size before drawing, same idiom as the already-@Ok
// Panel_DrawFlatShadedPoly (panel.cpp) and PCGfx_DrawTexture2D-family code
// (dcshellutils.cpp).
void DisplayTextBoxList(void** a1)
{
	CTextBox* pBox = reinterpret_cast<CTextBox*>(*a1);

	while (pBox)
	{
		if ((u8*)pPoly + sizeof(POLY_F4) > PolyBufferEnd)
			return;

		POLY_F4* p = (POLY_F4*)pPoly;
		pPoly = (u32*)((u8*)pPoly + sizeof(POLY_F4));

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: setPolyF4");

		p->r0 = pBox->mFric.vx;
		p->g0 = pBox->mFric.vy;
		p->b0 = pBox->mFric.vz;

		i16 age = pBox->mAge;
		i32 x = pBox->mPos.vx;
		i32 y = pBox->mPos.vy;
		i32 w = pBox->mVel.vx;
		i32 h = pBox->mVel.vy;

		if (age < 16)
		{
			y += (u32)((16 - age) * h) >> 5;
			h = (u32)(age * h) >> 4;
		}
		else
		{
			i32 lifetime = pBox->mLifetime;
			if (age > lifetime - 16)
			{
				i32 rem = lifetime - age;
				y += (u32)((16 - rem) * h) >> 5;
				h = (u32)(rem * h) >> 4;
			}
		}

		p->x0 = (i16)x;
		p->y0 = (i16)y;
		p->x1 = (i16)(x + w);
		p->y1 = (i16)y;
		p->x2 = (i16)x;
		p->y2 = (i16)(y + h);
		p->x3 = (i16)(x + w);
		p->y3 = (i16)(y + h);

		gsub_46CB90((void*)0x0056EB54);

		PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

		u32 color = 0xA0000000 | (p->r0 << 16) | (p->g0 << 8) | p->b0;
		f32 scaleX = gGameResolutionX / (f32)Xres;
		f32 scaleY = gGameResolutionY / (f32)Yres;

		PCGfx_DrawQPoly2D(
				p->x0 * scaleX, p->y0 * scaleY, 0.0f, 0.0f, color,
				p->x1 * scaleX, p->y1 * scaleY, 1.0f, 0.0f, color,
				p->x2 * scaleX, p->y2 * scaleY, 0.0f, 1.0f, color,
				p->x3 * scaleX, p->y3 * scaleY, 1.0f, 1.0f, color,
				5.0f);

		pBox = reinterpret_cast<CTextBox*>(pBox->mNext);
	}
}

// @BIGTODO
// Address 0x40dbd0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Camera-space transform of the sprite quad, animation
// frame UV lookup (word_610C48/4A tables), a rotated-vs-axis-aligned corner
// path, then a textured POLY_FT4 emit via sub_508550. Re-tagged from
// @MEDIUMTODO.
void DisplayFlatBitList(void**)
{
}

// @BIGTODO
// Address 0x40e840 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Ribbon segment renderer: walks CBit pairs building a
// quad strip between consecutive positions (perpendicular offset via
// sub_46D430 distance + cross terms), camera-space transform per vertex,
// sub_508550 gouraud emit. Re-tagged from @MEDIUMTODO.
void DisplayLinked2EndedBitListLeftover(void**)
{
}

// @BIGTODO
// Address 0x40f110 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Camera-space transform + perspective divide per
// pixel dot, blend mode from a flags nibble, then sub_507470 (2D sprite/dot
// emit). Re-tagged from @MEDIUMTODO.
void DisplayPixelList(void**)
{
}

// @BIGTODO
// Address 0x411ef0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Iterates a poly-line's inner vertex array (shifted
// pointer over CBit-like sub-entries), per-segment camera-space transform
// and clip test (sub_505B90), textured/flat POLY_FT4 or POLY_F4 emit via
// sub_509000. Re-tagged from @MEDIUMTODO.
void DisplayPolyLineList(void**)
{
}

// @BIGTODO
// Address 0x4125c0 (found by tracing Bit_Init's RegisterSlot calls, see
// DisplayTextBoxList). Same shape as DisplayPolyLineList (0x411ef0) but a
// different vertex stride/offset set and POLY_FT4 fill order; likely the
// "gouraud" (colour-per-vertex) poly-line variant. Re-tagged from
// @MEDIUMTODO.
void DisplayGPolyLineList(void**)
{
}

// @Ok
void Bit_Init(void)
{
	G_BITCOUNT = 0;
	G_NONRENDEREDBIT_LIST = 0;
	TextBoxList = 0;
	FlatBitList = 0;
	Linked2EndedBitListLeftover = 0;
	PixelList = 0;
	PolyLineList = 0;
	GPolyLineList = 0;
	QuadBitList = 0;
	GenPolyList = 0;
	ChunkBitList = 0;
	GlowList = 0;
	GlassList = 0;
	GLineList = 0;
	G_SPECIALDISPLAY_LIST = 0;

	if (gBitServer)
	{
		gBitServer = new CBitServer();

		gBitServer->RegisterSlot(reinterpret_cast<void**>(&TextBoxList), DisplayTextBoxList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&FlatBitList), DisplayFlatBitList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&Linked2EndedBitListLeftover), DisplayLinked2EndedBitListLeftover);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&PixelList), DisplayPixelList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&PolyLineList), DisplayPolyLineList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&GPolyLineList), DisplayGPolyLineList);

		gBitServer->RegisterSlot(reinterpret_cast<void**>(&QuadBitList), DisplayQuadBitList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&ChunkBitList), DisplayChunkBitList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&GlowList), DisplayGlowList);

		gBitServer->RegisterSlot(reinterpret_cast<void**>(&GlassList), DisplayGlassList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&GLineList), DisplayGLineList);
		gBitServer->RegisterSlot(reinterpret_cast<void**>(&G_SPECIALDISPLAY_LIST), DisplaySpecialDisplayList);
	}

	setDrawTPage();
	memset(gAnimTable, 0, sizeof(gAnimTable));
}

// @Ok
void Bit_SetSparkSize(u32 size)
{
	DoAssert(size < 0x10, "Daft spark size");

	SparkSize = size | (size << 16);
}

// @Ok
// @Matching
CWibbly::CWibbly(
		u8 a2,
		u8 a3,
		u8 a4,
		i32 a5,
		i32 a6,
		i32 a7,
		i32 a8,
		i32 a9,
		i32 a10,
		i32 a11,
		i32 a12,
		i32 a13,
		i32 a14)
	: CGouraudRibbon(a6 + 1, 0)
{
	this->field_7C = a9;
	this->field_88 = a10;
	this->field_80 = a11;

	this->field_8C = a13;
	this->field_84 = a12;
	this->field_90 = a14;
	this->field_94 = Rnd(4096);

	this->SetRGB(a2, a3, a4);

	this->mpPoints[this->mNumPoints - 1].r = a2 >> 1;
	this->mpPoints->r = this->mpPoints[this->mNumPoints - 1].r;

	this->mpPoints[this->mNumPoints - 1].g = a3 >> 1;
	this->mpPoints->g = this->mpPoints[this->mNumPoints - 1].g;

	this->mpPoints[this->mNumPoints - 1].b = a4 >> 1;
	this->mpPoints->b = this->mpPoints[this->mNumPoints - 1].b;

	this->SetWidth(a8 / 20);

	this->mpPoints[this->mNumPoints - 1].Width = a8 / 40;
	this->mpPoints->Width = this->mpPoints[this->mNumPoints - 1].Width;

	i32 v15 = 2 * a2;
	if (v15 > 255)
	{
		v15 = 0xFF;
	}

	i32 v21 = 2 * a3;
	if (v21 > 255)
	{
		v21 = 0xFF;
	}

	i32 v20 = 2 * a4;
	if (v20 > 255)
	{
		v20 = 0xFF;
	}

	this->SetCore(v15, v21, v20, a8 / 80);
}

// @MEDIUMTODO
void CWibbly::Move(void)
{
    printf("CWibbly::Move(void)");
}

// @Ok
INLINE void CWibbly::SetCore(
		u8 a2,
		u8 a3,
		u8 a4,
		i32 a5)
{
	delete this->field_48;

	this->field_48 = new CGouraudRibbon(this->mNumPoints, 0);

	this->field_48->mProtected = 1;
	this->field_48->SetRGB(a2, a3, a4);
	this->field_48->SetWidth(a5);
}

// @Ok
void CWibbly::SetEndPoints(
		CVector* a2,
		CVector* a3)
{
	this->field_4C = *a2;

	this->field_58 = (*a3 - *a2) / (this->mNumPoints - 1);

	CSVector v12;
	v12.vx = 0;
	v12.vy = 0;
	v12.vz = 0;

	Utils_CalcAim(&v12, a2, a3);
	v12.vy = (v12.vy + 1024) & 0xFFF;

	Utils_GetVecFromMagDir(&this->field_64, this->field_7C, &v12);
	v12.vy = (v12.vy - 1024) & 0xFFF;
	v12.vx = (v12.vx + 1024) & 0xFFF;

	Utils_GetVecFromMagDir(&this->field_70, this->field_88, &v12);
}

// @Ok
CWibbly::~CWibbly(void)
{
	delete this->field_48;
}

// @Ok
CFireyExplosion::CFireyExplosion(CVector* pPos)
{
	this->mPos = *pPos;
	this->mLifetime = 50;
	SFX_PlayPos(1, &this->mPos, 0);
}

// @Ok
CFireyExplosion::~CFireyExplosion(void)
{
}

// @Ok
// @Matching
void CFireyExplosion::Move(void)
{
	if (this->mAge == 5)
	{
		for (i32 i = 0; i < 1; ++i )
		{
			Rnd(100);
		}

	}
	if (++this->mAge > this->mLifetime)
	{
		this->Die();
	}
}

// @Ok
CCombatImpactRing::~CCombatImpactRing(void)
{
}

// @Ok
CTextBox::CTextBox(
		i32 a2,
		i32 a3,
		i32 a4,
		i32 a5,
		u32 a6,
		CFriction* pFric)
{
	this->AttachTo(reinterpret_cast<CBit**>(&TextBoxList));

	this->mPos.vx = a2;
	this->mPos.vy = a3;

	this->mVel.vx = a4;
	this->mVel.vy = a5;

	this->mFric.vx = pFric->vx;
	this->mFric.vy = pFric->vy;
	this->mFric.vz = pFric->vz;

	this->mLifetime = a6;

	this->field_3C = gTimerRelated;

}

// @Ok
CTextBox::~CTextBox(void)
{
	this->DeleteFrom(reinterpret_cast<CBit**>(&TextBoxList));
}

// @Ok
// @Note: Constructor got different inline and some oeprations are at different order, but don't care
// @Test
CChunkBit::CChunkBit(
		CSVector *a,
		CSVector *b,
		CSVector *c)
{
	this->mType = 6;

	this->mPosA.vx = a->vx;
	this->mPosA.vy = a->vy;
	this->mPosA.vz = a->vz;

	this->mPosB.vx = b->vx;
	this->mPosB.vy = b->vy;
	this->mPosB.vz = b->vz;

	this->mPosC.vx = c->vx;
	this->mPosC.vy = c->vy;
	this->mPosC.vz = c->vz;


	i32 v8 = b->vz - a->vz;
	i32 v10 = c->vz - a->vz;

	i32 v11 = b->vy - a->vy;
	i32 v13 = c->vy - a->vy;

	i32 v14 = b->vx - a->vx;
	i32 v15 = c->vx - a->vx;

	CVector v18;
	v18.vx = v13 * v8 - v10 * v11;
	v18.vy = v10 * v14 - v15 * v8;
	v18.vz = v15 * v11 - v13 * v14;

	v18 >>= 12;
	VectorNormal(
			reinterpret_cast<VECTOR*>(&v18),
			reinterpret_cast<VECTOR*>(&v18));
	
	i32 v16 = Rnd(96);
	this->mPosD.vx = (v18.vx * (v16 + 128)) >> 12;

	this->mPosD.vy = (v18.vy * (v16 + 128)) >> 12;
	this->mPosD.vz = (v18.vz * (v16 + 128)) >> 12;

	this->AttachTo(&ChunkBitList);
}

// @Ok
CChunkBit::~CChunkBit(void)
{
	this->DeleteFrom(reinterpret_cast<CBit**>(&ChunkBitList));
}


// @Ok
CBitServer::CBitServer(void)
{
	this->mNumEntries = 0;
}

// @Ok
CBitServer::~CBitServer(void)
{
}

// @Ok
// so close but that while loop is hard to match, prob because they didn't create a type
INLINE u32 CBitServer::RegisterSlot(void** bitList, void (*drawFunc)(void**))
{
	u32 usedSlot = this->mNumEntries;
	if (usedSlot < 0x20)
	{
		this->mEntry[usedSlot].field_0 = bitList;
		this->mEntry[usedSlot].field_4 = drawFunc;

		usedSlot = this->mNumEntries;
		this->mNumEntries = usedSlot + 1;


		u32 curSlot = this->mNumEntries;
		while (curSlot < 0x20)
		{
			if (this->mEntry[curSlot++].field_0 == 0)
				return usedSlot;

			this->mNumEntries = curSlot;
		}

		this->mNumEntries = 666;
	}

	return this->mNumEntries;
}

// @Ok
void CBitServer::DisplayRegisteredSlots(void)
{
	for (i32 i = 0; i < 0x20; i++)
	{
		if (this->mEntry[i].field_0)
			this->mEntry[i].field_4(this->mEntry[i].field_0);
	}
}

// @Ok
CPixel::~CPixel(void)
{
	this->DeleteFrom(reinterpret_cast<CBit**>(&PixelList));
}

// @Ok
INLINE CPixel::CPixel(void)
{
	this->AttachTo(reinterpret_cast<CBit**>(&PixelList));
}

// @Ok
// @Matching
CFrag::~CFrag(void)
{
}

// @Ok
// @Matching
void CFrag::Move(void)
{

	this->mPos.vx += this->mVel.vx;
	this->mPos.vy += this->mVel.vy;
	this->mPos.vz += this->mVel.vz;

	this->mVel.vx += this->mAcc.vx;
	this->mVel.vy += this->mAcc.vy;
	this->mVel.vz += this->mAcc.vz;

	this->mVel.vx -= this->mVel.vx >> this->mFric.vx;
	this->mVel.vy -= this->mVel.vy >> this->mFric.vy;
	this->mVel.vz -= this->mVel.vz >> this->mFric.vz;


	if (++this->mFrame >= this->mNumFrames)
	{
		this->mFrame = this->mNumFrames - 1;
	}

	if (++this->mAge >= this->mLifetime)
	{
		this->Die();
	}
}

// @Ok
// @AlmostMatching: inlines different and also the mVel assignement
CFrag::CFrag(
		CVector *a2,
		u8 a3,
		u8 a4,
		u8 a5,
		i32 a6,
		u16 a7,
		i32 a8,
		i32 a9,
		i32 a10,
		i32 a11)
{
	this->mPos = *a2;

	this->SetAnim(a6);

	if (a8)
	{
		this->SetSemiTransparent();
	}

	this->SetTint(a3, a4, a5);
	this->mScale = a7;

	this->mVel.vx = (Rnd(2 * a9 + 1) - a9) << 12;
	this->mVel.vy = -4096 * Rnd(a9);
	this->mVel.vz = (Rnd(2 * a9 + 1) - a9) << 12;
	this->mAcc.vy = a10;
	this->mFric.Set(3,3,3);

	this->mLifetime = Rnd(a11);
}

// @Ok
// @Matching
void CGlow::SetFringeWidth(u32 Fringe, u32 Width)
{
	DoAssert(Fringe < this->mNumFringes, "Bad Fringe sent to SetFringeWidth");

	SFringeQuad* pFringe = &this->mpFringes[Fringe * this->mNumSections];
	for (u32 i = 0; i < this->mNumSections; i++)
	{
		pFringe[i].Width = Width;
	}
}

// @Ok
// @CloseMatching - bruh lea vs add
void CGlow::SetFringeRGB(
		u32 Fringe,
		u8 r,
		u8 g,
		u8 b)
{
	print_if_false(Fringe < this->mNumFringes, "Bad Fringe sent to SetFringeRGB");

	u32 val = (((((0x3A << 8) | b) << 8) | g ) << 8) | r;

	SFringeQuad* pFringe = &this->mpFringes[Fringe * this->mNumFringes];
	for (u32 i = 0; i < this->mNumSections; i++)
	{
		pFringe[i].CodeBGR = val;
	}
}

// @Ok
// @Matching
void CCombatImpactRing::Move(void)
{
	if (this->mScale == this->field_68)
	{
		this->Die();
	}
	else
	{
		i32 v1 = gTimerRelated - this->field_70;
		this->field_70 = gTimerRelated;
		this->mScale += this->field_6C * v1;

		if (this->mScale > this->field_68)
			this->mScale = this->field_68;
	}
}

// @Ok
// @AlmostMatching: diff vector assingment and mFrame is assinged later for some reason
// also mScale is out of order
CCombatImpactRing::CCombatImpactRing(
		CVector *a2,
		u8 a3,
		u8 a4,
		u8 a5,
		i32 a6,
		i32 a7,
		i32 a8)
{
	this->mPos = *a2;

	this->mAngle = Rnd(4096);
	this->mScale = a6;

	this->field_68 = a7;
	this->field_6C = a8;
	this->mPostScale = 0xA001000;
	this->field_70 = gTimerRelated;

	this->SetTint(a3, a4, a5);

	this->mFrigDeltaZ = 64;

	this->SetAnim(8);
	this->SetSemiTransparent();
}

// @Ok
// @Matching
void CSimpleAnim::Move(void)
{
	this->IncFrame();

	if (this->mDie)
	{
		if (this->mFrame >= this->mDieFrame)
		{
			this->Die();
		}
	}
	else
	{
		if (this->mFrame >= this->mNumFrames)
		{
			if (this->mDieFrame == -2)
			{
				this->SetFrame(this->mNumFrames - 1);
			}
			else
			{
				this->SetFrame(0);
			}
		}
	}
}

// @Ok
// @Note: mine setups SEH frame (maybe because it's exported)
// also last if assingment is diff
CSimpleAnim::CSimpleAnim(
		CVector *a2,
		i32 a3,
		u16 a4,
		i32 a5,
		i32 a6,
		i32 a7)
{
	this->mPos = *a2;
	this->SetAnim(a3);
	this->mScale = a4;

	if (a5)
		this->SetSemiTransparent();


	this->mDie = a6;
	if (a7 == -1)
	{
		this->mDieFrame = this->mNumFrames - 1;
	}
	else
	{
		this->mDieFrame = a7;
	}
}

// @Ok
// @Matching
INLINE CSimpleAnim::~CSimpleAnim(void)
{
}

// @Ok
// @Matching
void CRibbon::SetPos(CVector &pos)
{
	if (++this->field_48 == this->mNumPoints)
	{
		this->field_48 = 0;
	}

	this->mPoints[this->field_48] = pos;

	i32 v4 = this->field_48;
	for (i32 i = this->mNumBits - 1; i >= 0; i--)
	{
		this->mBits[i]->field_64 = this->mPoints[v4];
		v4 -= this->mPointsPerBit;
		if (v4 < 0)
		{
			v4 += this->mNumPoints;
		}

		this->mBits[i]->field_58 = this->mPoints[v4];
	}
}

// @Ok
// @Matching
void CRibbon::SetScale(i32 Scale)
{
	for (i32 i = 0; i < this->mNumBits; i++)
	{
		this->mBits[i]->SetScale(Scale);
	}
}

// @Ok
// @Matching
CRibbon::~CRibbon(void)
{
	for (i32 i = 0; i < this->mNumBits; i++)
	{
		delete this->mBits[i];
	}

	Mem_Delete(this->mBits);
	Mem_Delete(this->mPoints);
}

// @Ok
// @AlmostMatching: some weird inlines, but overall good
// @Test
CRibbon::CRibbon(
		CVector *pos,
		i32 numbits,
		i32 pointsperbit,
		i32 middleanimindex,
		i32 endanimindex,
		i32 scale,
		i32 semitrans)
{
	this->mNumBits = numbits;
	this->mPointsPerBit = pointsperbit - 1;
	this->mNumPoints = numbits * (pointsperbit - 1) + 1;

	this->mPoints = static_cast<CVector*>(
			DCMem_New(sizeof(CVector) * this->mNumPoints, 0, 1, 0, 1));

	for (i32 j = 0; j < this->mNumPoints; j++)
	{
		this->mPoints[j] = *pos;
	}

	this->mBits = static_cast<CRibbonBit**>(
			DCMem_New(sizeof(CRibbonBit*) * numbits, 0, 1, 0, 1));

	for (i32 i = this->mNumBits - 1; i >= 0; i--)
	{
		this->mBits[i] = new CRibbonBit();

		this->mBits[i]->mProtected = 1;
		this->mBits[i]->SetAnim(middleanimindex);
		this->mBits[i]->SetScale(scale);

		if (semitrans)
			this->mBits[i]->SetSemiTransparent();
	}

	this->mBits[0]->mBitFlags |= 0x10u;
	this->mBits[0]->SetAnim(endanimindex);
}

// @Ok
// @Matching
void CSmokeTrail::Move(void)
{
	if (this->mFadeAway)
	{
		i32 v2 = 1;

		for (i32 i = 0; i < this->mNumBits; i++)
		{
			if (!this->mBits[i]->Fade(0))
			{
				v2 = 0;
			}
		}

		if (v2)
		{
			this->Die();
		}
	}
}


// @Ok
// @Matching
CSmokeTrail::~CSmokeTrail(void)
{
}

// @Ok
// @Matching
CSmokeTrail::CSmokeTrail(
		CVector* pos,
		i32 numbits,
		i32 r,
		i32 g,
		i32 b)
	: CRibbon(pos, numbits, 2, 2, 2, 400, 1)
{

	i32 v12 = r / this->mNumBits;
	i32 v13 = g / this->mNumBits;
	i32 v14 = b / this->mNumBits;

	for (i32 i = 0; i < this->mNumBits; i++)
	{
		i32 tmp = (this->mNumBits - 1 - i);
		this->mBits[i]->SetTint(
				r - tmp * v12, 
				g - tmp * v13, 
				b - tmp * v14);

		this->mBits[i]->SetTransDecay(8);
	}

}

// @Ok
// @Note: code works because sizeof(SFringeQuad) == sizeof(SSection), weird shit
CGlow::CGlow(u32 NumPoints, u32 NumFringes)
{
	print_if_false(NumPoints != 0, "Bad NumPoints sent to CGlow");
	this->mNumSections = 2 * NumPoints;
	this->mStepAngle = 0x1000u / (2 * NumPoints);
	this->mNumFringes = NumFringes;

	// This shit weird
	this->mpSections = static_cast<SSection*>(DCMem_New(sizeof(SSection) * (this->mNumSections + this->mNumSections * this->mNumFringes), 0, 1, 0, 1));

	this->mpFringes = reinterpret_cast<SFringeQuad*>(&this->mpSections[this->mNumSections]);
	for (u32 i = 0; i < this->mNumSections * this->mNumFringes; i++)
	{
		this->mpFringes[i].CodeBGR = 0x3A000000;
	}

	this->mCentreCodeBGR = 0x32000000;
	this->mMask = -1;

	this->AttachTo(reinterpret_cast<CBit**>(&GlowList));
}

// @Ok
CGlow::~CGlow(void)
{
	Mem_Delete(static_cast<void*>(this->mpSections));
	this->DeleteFrom(reinterpret_cast<CBit**>(&GlowList));
}

// @Ok
// Functional: quad-bit roll orientation, logic verified against Hex-Rays at
// 0x409560. Builds dir/perp1/perp2 via Utils_CalcPerps, indexes
// rcossin_tbl[a6 & 0xFFF] for the roll angle, rollA = (perp1*sin +
// perp2*cos) >> 12, rollB = (perp1*cos + perp2*-sin) >> 12, rollA *= a4,
// rollB *= a5, and the four corners are *a2 -+ rollA -+ rollB. (The 13
// mnemonic diffs from the byte-match phase are the MSVC6 rcossin_tbl
// double-field-read quirk, same as Utils_RotateY; the logic is equivalent.)
void CQuadBit::OrientUsing(CVector *a2, SVECTOR *a3, i32 a4, i32 a5, i32 a6)
{
	CVector dir(a3->vx, a3->vy, a3->vz);
	CVector perp1;
	CVector perp2;

	Utils_CalcPerps(&dir, &perp2, &perp1);

	SSinCos const *sc = &rcossin_tbl[a6 & 0xFFF];
	i32 s = sc->sin;
	i32 c = sc->cos;

	CVector rollA = ((perp1 * s) + (perp2 * c)) >> 12;
	CVector rollB = ((perp1 * c) + (perp2 * -s)) >> 12;

	rollA *= a4;
	rollB *= a5;

	this->mPos = *a2 - rollA - rollB;
	this->mPosB = *a2 + rollA - rollB;
	this->mPosC = *a2 - rollA + rollB;
	this->mPosD = *a2 + rollA + rollB;
}

// @Ok
// @Matching: wtf is that cast
void CQuadBit::SetTexture(u32 checksum)
{
	Texture *pTexture = Spool_FindTextureEntry(checksum);
	this->mpTexture = pTexture;

	if (this->mpTexture)
	{
		if (this->mpTexture->field_12 & 0xF0)
			this->mCodeBGR |= 0x20;

		// @FIXME
		this->field_74 = *reinterpret_cast<u32*>(&this->mpTexture->u0);
		// @FIXME
		this->field_78 = *reinterpret_cast<u32*>(&this->mpTexture->u1);
		// @FIXME
		this->field_7C = *reinterpret_cast<u32*>(&this->mpTexture->u2);
		this->field_80 = this->mpTexture->TexWin;
	}
}

// @Ok
// Functional (session-wide functional-only bar, 2026-08-30). Logic fully
// verified against Hex-Rays at 0x40c350: all 4 fill loops, AttachTo, mPos
// copy and mCentreCodeBGR line up instruction-for-instruction with the
// disassembly; the only remaining gap (kept for the record, no longer
// chased) was a 21-mnemonic register-allocation residue from the original
// sharing one preloaded register between the inlined CFriction::Set(1,1,1)
// stores and two of DCMem_New's args, which our source-shape attempts could
// not reproduce. See prior history in this file for the attempt log.
CGlow::CGlow(
		CVector* pVector,
		i32 a3,
		i32 a4,
		u8 a5,
		u8 a6,
		u8 a7,
		u8 a8,
		u8 a9,
		u8 a10)
{
	SSection* pSections = static_cast<SSection*>(DCMem_New(0x80, 0, 1, 0, 1));

	this->mNumSections = 8;
	this->mStepAngle = 0x200;
	this->mNumFringes = 1;

	this->mpSections = pSections;
	this->mpFringes = reinterpret_cast<SFringeQuad*>(this->mpSections + this->mNumSections);

	this->AttachTo(reinterpret_cast<CBit**>(&GlowList));

	this->mPos = *pVector;

	u32 i;

	for (i = 0; i < this->mNumSections; i++)
		this->mpSections[i].Radius = a3;

	this->mCentreCodeBGR = 0x32000000 | (((a7 << 8) | a6) << 8) | a5;

	for (i = 0; i < this->mNumSections; i++)
		this->mpSections[i].PadBGR = (a10 << 16) | (a9 << 8) | a8;

	for (i = 0; i < this->mNumSections; i++)
	{
		this->mpFringes[i].Width = a4;
		this->mpFringes[i].CodeBGR = 0x3A000000;
	}

	this->mMask = -1;
}

// @Ok
INLINE CFlatBit::~CFlatBit(void)
{
	this->DeleteFrom(reinterpret_cast<CBit**>(&FlatBitList));
}

// @Ok
// @Note: this is missing from the original game because CFlatBit is not inlined
CMotionBlur::~CMotionBlur(void)
{
}

// @Ok
// @Test
void CMotionBlur::Move(void)
{
	this->mPos.vx += this->mVel.vx;
	this->mPos.vy += this->mVel.vy;
	this->mPos.vz += this->mVel.vz;

	this->mScale -= this->field_3E;
	if (this->mScale < 0)
	{
		this->mScale = 0;
		this->Die();
		return;
	}

    u8 mCodeBGR = this->mCodeBGR;
    i16 mTransDecay = this->mTransDecay;

    u32 v10 = this->mCodeBGR >> 8;
    u32 mCodeBGR_high = (this->mCodeBGR >> 16);

	u8 v18;
    if ( mTransDecay > mCodeBGR )
      v18 = 0;
    else
      v18 = mCodeBGR - (this->mTransDecay & 0xFF);

	u8 v12;
    if ( mTransDecay > (u8)v10 )
      v12 = 0;
    else
      v12 = v10 - (this->mTransDecay & 0xFF);

	u8 v13;
    if ( mTransDecay > (u8)mCodeBGR_high )
      v13 = 0;
    else
      v13 = mCodeBGR_high - (this->mTransDecay & 0xFF);

	u16 v14 = (v13 << 8) | v12;
    this->mCodeBGR = v18 | this->mCodeBGR & 0xFF000000 | (v14 << 8);

	if (!(this->mCodeBGR & 0xFFFFFF))
	{
		this->Die();
		return;
	}

	if ( ++this->mAge & 1)
	{
		if ( ++this->mFrame >= this->mNumFrames )
			this->mFrame = 0;
	}

	this->mpPSXFrame = &this->mpPSXAnim[this->mFrame];
}

// @Ok
CMotionBlur::CMotionBlur(
		CVector* a2,
		CVector* a3,
		i32 a4,
		i32 a5,
		i32 a6,
		i32 a7)
{
	this->mPos = *a2;
	this->mVel = *a3;
	this->SetAnim(a4);
	this->SetSemiTransparent();
	this->mScale = a5;
	this->field_3E = a6;
	this->mTransDecay = a7;
}

// @Ok
// @Matching
INLINE CBit::CBit()
{
	this->mFric.vx = 1;
	this->mFric.vy = 1;
	this->mFric.vz = 1;
	//this->mFric.Set(1,1,1);

	G_BITCOUNT++;
}

// @Ok
// @Matching
INLINE void* CBit::operator new(size_t size) {

	void *pnew;
	if (TotalBitUsage == 0)
		pnew = Mem_New(size);
	else
		pnew = Mem_New(size);

	// Ensure size is a multiple of 4.
	size = ( size + 3 ) & ~0x03;

	// Zero all the newly allocated memory
	u32 *p=(u32 *)pnew;
	for (i32 i=0; i<size/4; ++i) *p++=0;

	return pnew;
}


// @Ok
void CBit::operator delete(void* ptr)
{
	Mem_Delete(ptr);
}

// @Ok
// @Matching
INLINE CBit::~CBit()
{
	--G_BITCOUNT;
}

// @Ok
// @Matching
INLINE void CBit::Die(void)
{
	DoAssert(this->mProtected == 0, "A protected bit die");
	this->mDead = 1;
}

// @Ok
// @Matching
INLINE void CBit::AttachTo(void* p)
{
	this->mNext = *reinterpret_cast<CBit**>(p);
	this->mPrevious = 0;
	*reinterpret_cast<CBit**>(p) = this;

	if (this->mNext)
		this->mNext->mPrevious = this;
}

// @Ok
// @Matching
void CBit::SetPos(const CVector &pos)
{
	this->mPos = pos;
}


// @Ok
// @Matching
INLINE void CBit::DeleteFrom(void *p)
{
	if (this->mNext)
		this->mNext->mPrevious = this->mPrevious;

	if (this->mPrevious)
		this->mPrevious->mNext = this->mNext;

	CBit **ppList = reinterpret_cast<CBit**>(p);

	if (*ppList == this)
		*ppList = this->mNext;
}

// @Ok
void CQuadBit::SetTint(unsigned char a2, unsigned char a3, unsigned char a4)
{
  this->mTint = a2 | ((a4 << 16) & 0xFF0000 | (a3 << 8) & 0xFF00) & 0xFFFFFF00;
}


// @Ok
void CQuadBit::SetSemiTransparent()
{
	this->mCodeBGR = (this->mCodeBGR & 0xFFFFFFFE) | 0x2C0;
}

// @Ok
void CQuadBit::SetOpaque(){
	this->mCodeBGR = (this->mCodeBGR & 0xFFFFFDBF) | 0x80;
}


// @Ok
void CQuadBit::SetSubtractiveTransparency(){
	this->mCodeBGR = (this->mCodeBGR & 0xFFFFFF7F) | 0x340;
}

// @Ok
void CQuadBit::SetCorners(const CVector &a2, const CVector &a3, const CVector &a4, const CVector &a5)
{
	this->mPos = a2;
	this->mPosB = a3;
	this->mPosC = a4;
	this->mPosD = a5;
}

// @Ok
void CQuadBit::SetTransparency(unsigned char a2){
	this->mTint = a2 | ((a2 | (a2 << 8)) << 8);
}

// @Ok
// @Matching
// Was blocked on the repo-wide vector.h operator-(CVector,CVector) INLINE
// bug (it compiled inline instead of the real out-of-line call the
// original makes at 0x4E7760). Now that operator- moved out-of-line into
// vector.cpp (2026-08-27), rebuilt clean with 0 mnemonic diffs against
// 0x409400, no source change needed here. a5 is read from the stack frame
// but never referenced in the original disasm; unused here too.
void CQuadBit::OrientUsing(CVector *a2, SVECTOR *a3, int a4, int a5)
{
	CVector dir(a3->vx, a3->vy, a3->vz);
	CVector perp1;
	CVector perp2;

	Utils_CalcPerps(&dir, &perp2, &perp1);

	perp2 *= a4;
	perp1 *= a4;

	this->mPos = *a2 - perp1 - perp2;
	this->mPosB = *a2 + perp1 - perp2;
	this->mPosC = *a2 - perp1 + perp2;
	this->mPosD = *a2 + perp1 + perp2;
}

// @Ok
// @Matching
void CQuadBit::SetTexture(int a, int b){
	DoAssert(a >= 0 && static_cast<u32>(a) < NUM_ANIM_ENTRIES, "Bad lookup value sent to CQuadBit::SetTexture");

	SAnimFrame* pAnim = gAnimTable[a];

	DoAssert(b >= 0 && b < *(reinterpret_cast<i32*>(pAnim) - 1), "Bad frame sent to CQuadBit::SetTexture");

	this->mpTexture = pAnim[b].pTexture;

	if (a == 0 && b == 0)
		this->mpTexture = Spool_FindTextureEntry("Shadow2");

	if (this->mpTexture->field_12 & 0xF0)
		this->mCodeBGR |= 0x20u;

	// @FIXME
	this->field_74 = *reinterpret_cast<u32*>(&this->mpTexture->u0);
	// @FIXME
	this->field_78 = *reinterpret_cast<u32*>(&this->mpTexture->u1);
	// @FIXME
	this->field_7C = *reinterpret_cast<u32*>(&this->mpTexture->u2);

	this->field_80 = this->mpTexture->TexWin;
}

// by-anim-name overload, address 0x409190 (unnamed in names.json, sits between
// the (i32,i32) and (Texture*) overloads; assert string names the class).
// @Ok
// @Matching
void CQuadBit::SetTexture(char* pName, i32 frame)
{
	SAnimFrame* pAnim = Spool_FindAnim(pName, 1);

	DoAssert(frame >= 0 && frame < *(reinterpret_cast<i32*>(pAnim) - 1), "Bad frame sent to CQuadBit::SetTexture");

	this->mpTexture = pAnim[frame].pTexture;

	if (this->mpTexture->field_12 & 0xF0)
		this->mCodeBGR |= 0x20u;

	// @FIXME
	this->field_74 = *reinterpret_cast<u32*>(&this->mpTexture->u0);
	// @FIXME
	this->field_78 = *reinterpret_cast<u32*>(&this->mpTexture->u1);
	// @FIXME
	this->field_7C = *reinterpret_cast<u32*>(&this->mpTexture->u2);

	this->field_80 = this->mpTexture->TexWin;
}

// @Ok
// @Matching: the assingments are weird bro
void CQuadBit::SetTexture(Texture *pTex)
{
	this->mpTexture = pTex;
	if (this->mpTexture->field_12 & 0xF0)
		this->mCodeBGR |= 0x20u;

	// @FIXME
	this->field_74 = *reinterpret_cast<u32*>(&this->mpTexture->u0);
	// @FIXME
	this->field_78 = *reinterpret_cast<u32*>(&this->mpTexture->u1);
	// @FIXME
	this->field_7C = *reinterpret_cast<u32*>(&this->mpTexture->u2);

	this->field_80 = this->mpTexture->TexWin;
}

// @Ok
// @Matching
INLINE CNonRenderedBit::CNonRenderedBit(void)
{
	this->AttachTo(&G_NONRENDEREDBIT_LIST);
}

// @Ok
// @Matching
INLINE CNonRenderedBit::~CNonRenderedBit(void)
{
	this->DeleteFrom(&G_NONRENDEREDBIT_LIST);
}

// @Ok
// @Note: not matererialized. it's very weird on PSX and on PowerPC
// according to the symbols there's no local variable and i can't seem to
// be able to write it without it.
INLINE void CFT4Bit::IncFrame(void)
{
	i16 val = ((this->mFrame << 8) | this->mFrameFrac) + this->mAnimSpeed;

	this->mFrame = val >> 8;
	this->mFrameFrac = val;

	this->mpPSXFrame = &this->mpPSXAnim[this->mFrame];
}

// @Ok
INLINE CFT4Bit::~CFT4Bit()
{
	if (this->mDeleteAnimOnDestruction)
		Mem_Delete(reinterpret_cast<void*>(this->mpPSXAnim));
}


// @Ok
void CFT4Bit::SetAnimSpeed(short s)
{
	this->mAnimSpeed = s;
}

// @Ok
// @Matching
INLINE void CFT4Bit::SetScale(u16 s)
{
	this->mScale = s;
}


// @Ok
// @Matching
INLINE void CFT4Bit::SetSemiTransparent()
{
	this->mCodeBGR |= 0x2000000;
}

// @Ok
void CFT4Bit::SetTransparency(unsigned char t){
	this->mCodeBGR = t | this->mCodeBGR & 0xFF000000 | ((t | (t << 8)) << 8);
}


// @Ok
// @Matching
INLINE void CFT4Bit::SetAnim(i32 a2)
{
	ASSERT(a2 >= 0 && !(static_cast<u32>(a2) >= NUM_ANIM_ENTRIES), "Bad lookup value sent to SetAnim");
	ASSERT(this->mDeleteAnimOnDestruction == 0, "mDeleteAnimOnDestruction set?");

	this->mpPSXAnim = G_ANIM_TABLE[a2];
	this->mNumFrames = *reinterpret_cast<u8*>(&this->mpPSXAnim[-1].pTexture);
	this->mFrameFrac = 0;
	this->mFrame = 0;
	this->mpPSXFrame = this->mpPSXAnim;
}

// @Ok
// @Matching
INLINE void CFT4Bit::SetTint(u8 r, u8 g, u8 b)
{
	this->mCodeBGR = (this->mCodeBGR & 0xFF000000) | (b << 0x10) | (g << 8) | r;
}

// @Ok
// @Matching
void CFT4Bit::SetTexture(Texture* pTexture)
{
	ASSERT(this->mpPSXAnim == 0, "mpPSXAnim already set?");
	ASSERT(pTexture != 0, "No Texture for SetTexture");

	this->mpPSXAnim = static_cast<SAnimFrame*>(Mem_New(sizeof(SAnimFrame)));
	this->mDeleteAnimOnDestruction = 1;

	i32 w = pTexture->u1 - pTexture->u0;
	i32 h = pTexture->v2 - pTexture->v0;

	this->mpPSXAnim->Width = w;
	this->mpPSXAnim->Height = h;

	this->mpPSXAnim->OffX = w / -2;
	this->mpPSXAnim->OffY = h / -2;

	this->mpPSXAnim->pTexture = pTexture;
	this->mpPSXFrame = this->mpPSXAnim;

	this->mNumFrames = 1;
}

// @Ok
void CFT4Bit::SetTexture(unsigned int Checksum)
{
	int v4; // ecx
	int v5; // eax
	int v6; // edx
	int v7; // ecx

	DoAssert(this->mpPSXAnim == 0, "mpPSXAnim already set?");

	Texture *pTexture = Spool_FindTextureEntry(Checksum);
	print_if_false(pTexture != 0, "Bad checksum sent to SetTexture");

	this->mpPSXAnim = static_cast<SAnimFrame*>(DCMem_New(sizeof(SAnimFrame), 0, 1, 0, 1));

	this->mDeleteAnimOnDestruction = 1;

	v4 = (u8)pTexture->v2;
	v5 = (u8)pTexture->u1 - (u8)pTexture->u0;
	v6 = (u8)pTexture->v0;

	this->mpPSXAnim->Width = v5;

	v7 = v4 - v6;
	this->mpPSXAnim->Height = v7;
	this->mpPSXAnim->OffX = v5 / -2;
	this->mpPSXAnim->OffY = v7 / -2;
	this->mpPSXAnim->pTexture = pTexture;
	this->mpPSXFrame = this->mpPSXAnim;

	this->mNumFrames = 1;
}

// @Ok
// Functional (session-wide functional-only bar, 2026-08-30). Verified against
// Hex-Rays at 0x408ef0. Bug fixed: when mCodeBGR's low 3 bytes are already
// zero, this->Die() must only run when a2 is nonzero; the function still
// returns 1 either way. The old code called Die() unconditionally, ignoring
// a2. The per-channel decay formulas below were already correct (the old
// @NotOk comment was about register scheduling only, not a logic error).
i32 CFT4Bit::Fade(i32 a2)
{
	i32 mCodeBGR = this->mCodeBGR;

	if (!(mCodeBGR & 0xFFFFFF))
	{
		if (a2)
			this->Die();
		return 1;
	}

	u16 v6 = this->mTransDecay;
	u8 v10;
	if (v6 > (u16)(this->mCodeBGR & 0xFF))
		v10 = 0;
	else
		v10 = (this->mCodeBGR & 0xFF) - (this->mTransDecay & 0xFF);

	u8 v7;
	if (v6 > (u16)((this->mCodeBGR & 0xFF00) >> 8))
		v7 = 0;
	else
		v7 = ((this->mCodeBGR & 0xFF00) >> 8) - (this->mTransDecay & 0xFF);

	u8 v8;
	if (v6 > (u16)((this->mCodeBGR & 0xFF0000) >> 16))
		v8 = 0;
	else
		v8 = ((this->mCodeBGR & 0xFFFF00) >> 16) - (this->mTransDecay & 0xFF);


	this->mCodeBGR = (mCodeBGR & 0xFF000000) | (((v8 << 8) | v7) << 8) | v10;

	return 0;
}

// @Ok
// Functional (session-wide functional-only bar, 2026-08-30). Verified against
// Hex-Rays at 0x410890: mPos = *pCenter, mVel = {cos*velScale, 0, sin*velScale},
// SetAnim/SetSemiTransparent/SetScale/SetTransDecay all match field-for-field.
// The trailing `p->mFrigDeltaZ = frigDeltaZ;` outside the `if (p)` block is not
// a bug in this source: the original binary does the exact same unconditional
// write after the allocation-failure branch (a genuine original defect,
// reproduced here per the "don't fix original bugs" rule). Byte residue (72
// mnemonic diffs, register/inlining-level choice only) was chased through 14
// source-shape hypotheses previously; not revisited since this bar only needs
// functional correctness.
i32 Bit_MakeSpriteRing(CVector *pCenter, i32 count, i32 velScale, i32 animIndex, i32 scale, i32 field3E, i32 transDecay, i32 frigDeltaZ)
{
	for (i32 i = 0; i < count; i++)
	{
		i32 angle = ((i * 0x1000) / count) & 0xFFF;
		CVector vel(rcossin_tbl[angle].cos * velScale, 0, rcossin_tbl[angle].sin * velScale);

		CFlatBit *p = new CFlatBit();

		if (p)
		{
			p->mVel = vel;
			p->mPos = *pCenter;

			p->SetAnim(animIndex);
			p->SetSemiTransparent();

			p->SetScale(scale);
			p->field_3E = field3E;
			p->SetTransDecay(transDecay);
		}

		p->mFrigDeltaZ = frigDeltaZ;
	}

	return 0;
}

// @Ok
void CBit::Move(void)
{
}

// @Ok
void MoveList(CBit *pBit)
{
	for (CBit *p = pBit; p; p = p->mNext)
	{
		if (!p->mDead)
			p->Move();
	}
}

// @Ok
// @Matching
void Bit_SetSparkRGB(u8 r, u8 g, u8 b)
{
	gSparkRGB[0] = r;
	gSparkRGB[1] = g;
	gSparkRGB[2] = b;
}

// @Ok
// @Matching
void Bit_SetSparkFadeRGB(u8 r, u8 g, u8 b)
{
	gSparkFadeRGB[0] = r;
	gSparkFadeRGB[1] = g;
	gSparkFadeRGB[2] = b;
}

// @Ok
INLINE CSpecialDisplay::CSpecialDisplay(void)
{
	this->AttachTo(&G_SPECIALDISPLAY_LIST);
}

// @Ok
void CGlow::SetCentreRGB(unsigned char a2, unsigned char a3, unsigned char a4)
{
	this->mCentreCodeBGR = 0x32000000 | (((a4 << 8) | a3) << 8) | a2;
}

// @Ok
// Functional: verified against Hex-Rays at 0x40f3d0, which does a plain
// 6-byte copy of *pVec into the global (a DWORD then a WORD, matching
// CSVector's size). Same shape as the already-@Ok
// Bit_SetSparkTrajectoryCone right below.
void Bit_SetSparkTrajectory(const CSVector *pVec)
{
	SparkTrajectory = *pVec;
}

// @Ok
// Functional: set spark trajectory cone, logic verified against Hex-Rays at
// 0x40f3f0. Just stores the cone vector into gSparkTrajectoryCone.
void Bit_SetSparkTrajectoryCone(const CSVector *pVec)
{
	SparkTrajectoryCone = *pVec;
}

// @Ok
// @Matching
INLINE CFT4Bit::CFT4Bit(void)
{
	this->mAnimSpeed = 0x80;
	this->mScale = 400;
	this->mCodeBGR = 0x2C808080;
}

// @Ok
// @Validate: when inlined
INLINE CLinked2EndedBit::CLinked2EndedBit(void)
{
	this->AttachTo(&Linked2EndedBitListLeftover);
}

// @Ok
// @AlmostMatching: slightly different inline
INLINE CLinked2EndedBit::~CLinked2EndedBit(void)
{
	this->DeleteFrom(reinterpret_cast<CBit**>(&Linked2EndedBitListLeftover));
}

// @Ok
// @Matching
INLINE CRibbonBit::CRibbonBit(void)
{
}

// @Ok
// @Matching
CRibbonBit::~CRibbonBit(void)
{
}

// @Ok
void CRibbonBit::Move(void)
{
	this->IncFrameWithWrap();
}

// @Ok
// @Note: not materialized, got it from CRibbonBit::Move
void CFT4Bit::IncFrameWithWrap(void)
{
	i16 val = ((this->mFrame << 8) | this->mFrameFrac) + this->mAnimSpeed;

	this->mFrame = val >> 8;
	this->mFrameFrac = val;

	if (this->mFrame >= this->mNumFrames)
		this->mFrame = 0;

	this->mpPSXFrame = &this->mpPSXAnim[this->mFrame];
}

/*
// @Ok
void CTexturedRibbon::SetOuterRGBi(int index, unsigned char a3, unsigned char a4, unsigned char a5)
{
	this->field_60[index+1] = (a3 | (((a5 << 8) | a4) << 8));
}
*/

// @Ok
// @Matching
void CGlow::SetRadius(int radius)
{
	for (u32 i = 0; i < this->mNumSections; i++)
	{
		this->mpSections[i].Radius = radius;
	}
}

// @Ok
// Functional: set ribbon color, logic verified against Hex-Rays at 0x40a920.
// The original has an explicit field_3C != -1 check, but the loop runs 0
// times when field_3C is -1, so the behavior is the same.
void CSimpleTexturedRibbon::SetRGB(unsigned char r, unsigned char g, unsigned char b)
{
	int value = (r | (((b << 8) | g) << 8));
	u32 *ptr = this->field_48;

	int i = 0;
	for (i = 0; i < this->field_3C + 1; i++)
		ptr[i] = value;
}

// @Ok
void CGlow::SetRGB(u8 r, u8 g, u8 b)
{
	u32 value = (r | (((b << 8) | g) << 8));

	for (u32 i = 0; i < this->mNumSections; i++)
	{
		this->mpSections[i].PadBGR = value;
	}
}

// @Ok
// @Matching
void Bit_ReduceRGB(u32* p, i32 amount)
{
	u8 b = *p;
	u8 g = *p >> 8;
	u8 r = *p >> 16;

	b = (b >= amount) ? b - amount : 0;
	g = (g >= amount) ? g - amount : 0;
	r = (r >= amount) ? r - amount : 0;

	*p = (*p & 0xFF000000) | (r << 16) | (g << 8) | b;
}

// @Ok
// @Matching
INLINE void CFT4Bit::SetFrame(i32 frame)
{
	ASSERT(frame >= 0 && frame < this->mNumFrames, "Bad frame sent to SetFrame");
	ASSERT(this->mpPSXAnim != 0, "SetFrame called before SetAnim");

	this->mFrame = frame;
	this->mFrameFrac = 0;

	this->mpPSXFrame = &this->mpPSXAnim[this->mFrame];
}

// @Ok
// @Note: no materialization exists
INLINE void CFT4Bit::SetTransDecay(i32 decay)
{
	this->mTransDecay = decay;
}

// @Ok
// @Matching
CFlatBit::CFlatBit(void)
{
	this->AttachTo(reinterpret_cast<CBit**>(&FlatBitList));

	this->mSemiTransparencyRate = 0x20;
	this->mAngFric = 1;
	this->mPostScale = 0x10001000;
}

// @Ok
// @Matching
void Bit_UpdateQuickAnimLookups(void)
{
	for (i32 i = 0; i < NUM_ANIM_ENTRIES; i++)
	{
		if (gAnimTable[i])
		{
			DoAssert(gAnimTable[i]->pTexture != 0, "Anim has no texture, don't know the region!");
			Spool_RemoveAccess(
					reinterpret_cast<void**>(&gAnimTable[i]),
					gAnimTable[i]->pTexture->mRegion);
		}

		Spool_AnimAccess(gAnimNames[i], &gAnimTable[i]);
	}
}

// @Ok
// @AlmostMatching: 1 byte diff in CFriction::Set
CGlassBit::CGlassBit(
		const CVector *Pos,
		const CVector *Vel,
		i32 GroundY,
		u8 r,
		u8 g,
		u8 b,
		i32 dx,
		i32 dy,
		i32 dz)
{
	this->AttachTo(&GlassList);

	this->mPos = *Pos;

	this->mPosA.vx = Pos->vx + ((Rnd(2 * dx + 1) - dx) << 12);
	this->mPosA.vy = Pos->vy + ((Rnd(2 * dy + 1) - dy) << 12);
	this->mPosA.vz = Pos->vz + ((Rnd(2 * dz + 1) - dz) << 12);
	this->mPosB.vx = Pos->vx + ((Rnd(2 * dx + 1) - dx) << 12);
	this->mPosB.vy = Pos->vy + ((Rnd(2 * dy + 1) - dy) << 12);
	this->mPosB.vz = Pos->vz + ((Rnd(2 * dz + 1) - dz) << 12);
	this->mPosC.vx = Pos->vx + ((Rnd(2 * dx + 1) - dx) << 12);
	this->mPosC.vy = Pos->vy + ((Rnd(2 * dy + 1) - dy) << 12);
	this->mPosC.vz = Pos->vz + ((Rnd(2 * dz + 1) - dz) << 12);

	this->mVel = *Vel;

	this->mGroundY = GroundY;
	
	this->mDefaultR = r >> 2;
	this->mDefaultG = g >> 2;
	this->mDefaultB = b >> 2;

	this->mR = r >> 2;
	this->mG = g >> 2;
	this->mB = b >> 2;

	this->mFadeRate = 4;
	this->mLifetime = Rnd(30) + 30;
}

// @Ok
// @Matching
void CGlassBit::Move(void)
{
	this->mPosA += this->mVel;
	this->mPosB += this->mVel;
	this->mPosC += this->mVel;

	this->mVel.vy += 10000;

	if (this->mPosA.vy > this->mGroundY)
	{
		if (this->mAge < 2)
		{
			this->mPosA.vy = this->mGroundY - (Rnd(50) << 12);
			this->mPosB.vy = this->mGroundY - (Rnd(50) << 12);
			this->mPosC.vy = this->mGroundY - (Rnd(50) << 12);

			this->mVel.vy = -this->mVel.vy;
			this->mVel.vy >>= 2;
			this->mAge++;
		}
		else
		{
			this->mVel.vy = 0;
		}

		this->mVel.vx >>= 1;
		this->mVel.vz >>= 1;
	}

	if (this->mAge >= 2 || Rnd(5))
	{
		this->mR = this->mDefaultR;
		this->mG = this->mDefaultG;
		this->mB = this->mDefaultB;
	}

	else
	{
		this->mB = 64;
		this->mR = 48;
		this->mG = 48;
	}

	if (this->mLifetime)
	{
		this->mLifetime--;
	}

	if (!this->mLifetime)
	{
		i32 newR;
		if (this->mDefaultR > this->mFadeRate)
		{
			newR = this->mDefaultR - this->mFadeRate;
		}
		else
		{
			newR = 0;
		}
		this->mDefaultR = newR;

		i32 newG;
		if (this->mDefaultG > this->mFadeRate)
		{
			newG = this->mDefaultG - this->mFadeRate;
		}
		else
		{
			newG = 0;
		}
		this->mDefaultG = newG;

		i32 newB;
		if (this->mDefaultB > this->mFadeRate)
		{
			newB = this->mDefaultB - this->mFadeRate;
		}
		else
		{
			newB = 0;
		}
		this->mDefaultB = newB;

	}

	if (!(this->mDefaultB | this->mDefaultR | this->mDefaultG))
	{
		this->Die();
	}
}

// @Ok
// @Matching
CGlassBit::~CGlassBit(void)
{
	this->DeleteFrom(&GlassList);
}

void validate_CFlatBit(void){
	VALIDATE(CFlatBit, mAngle, 0x58);
	VALIDATE(CFlatBit, field_5A, 0x5A);
	VALIDATE(CFlatBit, mAngFric, 0x5E);
	VALIDATE(CFlatBit, mPostScale, 0x60);
	VALIDATE(CFlatBit, mSemiTransparencyRate, 0x65);
}

void validate_CFT4Bit(void){
	VALIDATE(CFT4Bit, mTransDecay, 0x3C);
	VALIDATE(CFT4Bit, field_3E, 0x3E);

	VALIDATE(CFT4Bit, mCodeBGR, 0x40);

	VALIDATE(CFT4Bit, mDeleteAnimOnDestruction, 0x44);
	VALIDATE(CFT4Bit, mpPSXAnim, 0x48);
	VALIDATE(CFT4Bit, mpPSXFrame, 0x4C);

	VALIDATE(CFT4Bit, mBitFlags, 0x50);

	VALIDATE(CFT4Bit, mNumFrames, 0x51);
	VALIDATE(CFT4Bit, mFrame, 0x52);
	VALIDATE(CFT4Bit, mFrameFrac, 0x53);

	VALIDATE(CFT4Bit, mAnimSpeed, 0x54);
	VALIDATE(CFT4Bit, mScale, 0x56);
}

void validate_CQuadBit(void)
{
	VALIDATE_SIZE(CQuadBit, 0x84);

	VALIDATE(CQuadBit, mPosB, 0x3C);
	VALIDATE(CQuadBit, mPosC, 0x48);
	VALIDATE(CQuadBit, mPosD, 0x54);
	VALIDATE(CQuadBit, mpTexture, 0x60);
	VALIDATE(CQuadBit, mCodeBGR, 0x64);

	VALIDATE(CQuadBit, field_68, 0x68);

	VALIDATE(CQuadBit, mTint, 0x6C);

	VALIDATE(CQuadBit, field_70, 0x70);

	VALIDATE(CQuadBit, field_74, 0x74);
	VALIDATE(CQuadBit, field_78, 0x78);
	VALIDATE(CQuadBit, field_7C, 0x7C);
	VALIDATE(CQuadBit, field_80, 0x80);
}



void validate_CBit(void)
{
	VALIDATE_SIZE(CBit, 0x3C);
	VALIDATE(CBit, mPrevious, 0x4);
	VALIDATE(CBit, mNext, 0x8);

	VALIDATE(CBit, mAge, 0xC);
	VALIDATE(CBit, mLifetime, 0xE);

	VALIDATE(CBit, mPos, 0x10);
	VALIDATE(CBit, mVel, 0x1C);
	VALIDATE(CBit, mAcc, 0x28);
	VALIDATE(CBit, mDead, 0x37);
	VALIDATE(CBit, mFrigDeltaZ, 0x38);
	VALIDATE(CBit, mProtected, 0x3A);
	VALIDATE(CBit, mType, 0x3B);
}

void validate_CSmokeTrail(void)
{
	VALIDATE_SIZE(CSmokeTrail, 0x58);

	VALIDATE(CSmokeTrail, mFadeAway, 0x54);
}

void validate_CGlow(void)
{
	VALIDATE_SIZE(CGlow, 0x5C);

	VALIDATE(CGlow, mpSections, 0x3C);
	VALIDATE(CGlow, mpFringes, 0x40);

	VALIDATE(CGlow, mNumSections, 0x44);
	VALIDATE(CGlow, mNumFringes, 0x48);
	VALIDATE(CGlow, mCentreCodeBGR, 0x4C);
	VALIDATE(CGlow, mStepAngle, 0x50);
	VALIDATE(CGlow, mSkipTriangles, 0x52);

	VALIDATE(CGlow, mAngle, 0x54);
	VALIDATE(CGlow, mMask, 0x58);
}

void validate_CLinked2EndedBit(void)
{
	VALIDATE_SIZE(CLinked2EndedBit, 0x70);

	VALIDATE(CLinked2EndedBit, field_58, 0x58);
	VALIDATE(CLinked2EndedBit, field_64, 0x64);
}

void validate_CRibbon(void)
{
	VALIDATE_SIZE(CRibbon, 0x54);

	VALIDATE(CRibbon, mNumBits, 0x3C);
	VALIDATE(CRibbon, mPointsPerBit, 0x40);
	VALIDATE(CRibbon, mNumPoints, 0x44);

	VALIDATE(CRibbon, field_48, 0x48);

	VALIDATE(CRibbon, mPoints, 0x4C);
	VALIDATE(CRibbon, mBits, 0x50);
}

void validate_CRibbonBit(void)
{
	VALIDATE_SIZE(CRibbonBit, 0x70);
}

/*
void validate_CTexturedRibbon(void)
{
	VALIDATE(CTexturedRibbon, field_60, 0x60);
}
*/

void validate_CSimpleTexturedRibbon(void)
{
	VALIDATE_SIZE(CSimpleTexturedRibbon, 0x4C);

	VALIDATE(CSimpleTexturedRibbon, field_3C, 0x3C);

	VALIDATE(CSimpleTexturedRibbon, field_3E, 0x3E);

	VALIDATE(CSimpleTexturedRibbon, pTextures, 0x40);
	VALIDATE(CSimpleTexturedRibbon, field_44, 0x44);
	VALIDATE(CSimpleTexturedRibbon, field_48, 0x48);
}

void validate_CSimpleAnim(void)
{
	VALIDATE_SIZE(CSimpleAnim, 0x70);

	VALIDATE(CSimpleAnim, mDie, 0x68);
	VALIDATE(CSimpleAnim, mDieFrame, 0x6C);
}

void validate_CNonRenderedBit(void)
{
	VALIDATE_SIZE(CNonRenderedBit, 0x3C);
}

void validate_CMotionBlur(void)
{
	VALIDATE_SIZE(CMotionBlur, 0x68);
}

void validate_CSpecialDisplay(void)
{
	VALIDATE_SIZE(CSpecialDisplay, 0x3C);
}

void validate_SFlatBitVelocity(void)
{
	VALIDATE_SIZE(SSinCos, 0x4);

	VALIDATE(SSinCos, sin, 0x0);
	VALIDATE(SSinCos, cos, 0x2);
}

void validate_CCombatImpactRing(void)
{
	VALIDATE_SIZE(CCombatImpactRing, 0x74);

	VALIDATE(CCombatImpactRing, field_68, 0x68);
	VALIDATE(CCombatImpactRing, field_6C, 0x6C);
	VALIDATE(CCombatImpactRing, field_70, 0x70);
}

void validate_SRibbonPoint(void)
{
	VALIDATE_SIZE(SRibbonPoint, 0x24);

	VALIDATE(SRibbonPoint, Pos, 0x0);
	VALIDATE(SRibbonPoint, r, 0xC);
	VALIDATE(SRibbonPoint, g, 0xD);
	VALIDATE(SRibbonPoint, b, 0xE);
	VALIDATE(SRibbonPoint, Width, 0x10);
	VALIDATE(SRibbonPoint, WidthB, 0x12);
	VALIDATE(SRibbonPoint, rB, 0x14);
	VALIDATE(SRibbonPoint, gB, 0x15);
	VALIDATE(SRibbonPoint, bB, 0x16);
	VALIDATE(SRibbonPoint, Last1Scr, 0x18);
	VALIDATE(SRibbonPoint, Last2Scr, 0x1C);
	VALIDATE(SRibbonPoint, Last3Scr, 0x20);
}

void validate_CFrag(void)
{
	VALIDATE_SIZE(CFrag, 0x68);
}

void validate_CPixel(void)
{
	VALIDATE_SIZE(CPixel, 0x48);

	VALIDATE(CPixel, tag, 0x3C);

	VALIDATE(CPixel, r0, 0x40);
	VALIDATE(CPixel, g0, 0x41);
	VALIDATE(CPixel, b0, 0x42);

	VALIDATE(CPixel, code, 0x43);
	VALIDATE(CPixel, mWidthHeight, 0x44);
}

void validate_CBitServer(void)
{
	VALIDATE_SIZE(CBitServer, 0x108);
}

void validate_CChunkBit(void)
{
	// @FIXME
	VALIDATE_SIZE(CChunkBit, 0x94);

	VALIDATE(CChunkBit, mPosA, 0x3C);
	VALIDATE(CChunkBit, mPosB, 0x44);
	VALIDATE(CChunkBit, mPosC, 0x4C);
	VALIDATE(CChunkBit, mPosD, 0x54);

	VALIDATE(CChunkBit, mWorldPosA, 0x5C);
	VALIDATE(CChunkBit, mWorldPosB, 0x68);
	VALIDATE(CChunkBit, mWorldPosC, 0x74);
	VALIDATE(CChunkBit, mWorldPosD, 0x80);

	VALIDATE(CChunkBit, mAngles, 0x8C);
}

void validate_CTextBox(void)
{
	VALIDATE_SIZE(CTextBox, 0x44);

	VALIDATE(CTextBox, field_3C, 0x3C);
}

void validate_CFireyExplosion(void)
{
	VALIDATE_SIZE(CFireyExplosion, 0x3C);
}

void validate_CWibbly(void)
{
	VALIDATE_SIZE(CWibbly, 0x98);

	VALIDATE(CWibbly, field_48, 0x48);
	VALIDATE(CWibbly, field_4C, 0x4C);
	VALIDATE(CWibbly, field_58, 0x58);
	VALIDATE(CWibbly, field_64, 0x64);
	VALIDATE(CWibbly, field_70, 0x70);
	VALIDATE(CWibbly, field_7C, 0x7C);

	VALIDATE(CWibbly, field_80, 0x80);
	VALIDATE(CWibbly, field_84, 0x84);

	VALIDATE(CWibbly, field_88, 0x88);
	VALIDATE(CWibbly, field_8C, 0x8C);

	VALIDATE(CWibbly, field_90, 0x90);
	VALIDATE(CWibbly, field_94, 0x94);
}

void validate_SBitServerEntry(void)
{
	VALIDATE_SIZE(SBitServerEntry, 0x8);

	VALIDATE(SBitServerEntry, field_0, 0x0);
	VALIDATE(SBitServerEntry, field_4, 0x4);
}

void validate_SSection(void)
{
	VALIDATE_SIZE(SSection, 8);

	VALIDATE(SSection, Radius, 0x0);
	VALIDATE(SSection, PadBGR, 0x4);
}

void validate_SFringeQuad(void)
{
	VALIDATE_SIZE(SFringeQuad, sizeof(SSection));
	VALIDATE_SIZE(SFringeQuad, 8);

	VALIDATE(SFringeQuad, Width, 0x0);
	VALIDATE(SFringeQuad, CodeBGR, 0x4);
}

void validate_vector4d(void)
{
	VALIDATE_SIZE(vector4d, 0x10);
	VALIDATE(vector4d, field_0, 0x0);
}

void validate_vector3d(void)
{
	VALIDATE_SIZE(vector3d, 0xC);

	VALIDATE(vector3d, field_0, 0x0);
}

void validate_CGlassBit(void)
{
	VALIDATE_SIZE(CGlassBit, 0x6C);

	VALIDATE(CGlassBit, mPosA, 0x3C);
	VALIDATE(CGlassBit, mPosB, 0x48);
	VALIDATE(CGlassBit, mPosC, 0x54);

	VALIDATE(CGlassBit, mGroundY, 0x60);

	VALIDATE(CGlassBit, mDefaultR, 0x64);
	VALIDATE(CGlassBit, mDefaultG, 0x65);
	VALIDATE(CGlassBit, mDefaultB, 0x66);

	VALIDATE(CGlassBit, mR, 0x67);
	VALIDATE(CGlassBit, mG, 0x68);
	VALIDATE(CGlassBit, mB, 0x69);

	VALIDATE(CGlassBit, mFadeRate, 0x6A);
}

void validate_SRibbonTexture(void)
{
	VALIDATE_SIZE(SRibbonTexture, 0x20);

	VALIDATE(SRibbonTexture, field_0, 0x0);
	VALIDATE(SRibbonTexture, field_2, 0x2);

	VALIDATE(SRibbonTexture, field_4, 0x4);
	VALIDATE(SRibbonTexture, field_5, 0x5);
	VALIDATE(SRibbonTexture, field_6, 0x6);
	VALIDATE(SRibbonTexture, field_7, 0x7);
	VALIDATE(SRibbonTexture, field_8, 0x8);
	VALIDATE(SRibbonTexture, field_9, 0x9);
	VALIDATE(SRibbonTexture, field_A, 0xA);
	VALIDATE(SRibbonTexture, field_B, 0xB);

	VALIDATE(SRibbonTexture, field_C, 0xC);
	VALIDATE(SRibbonTexture, field_E, 0xE);

	VALIDATE(SRibbonTexture, u0, 0x10);
	VALIDATE(SRibbonTexture, v0, 0x11);

	VALIDATE(SRibbonTexture, mClut, 0x12);

	VALIDATE(SRibbonTexture, u1, 0x14);
	VALIDATE(SRibbonTexture, v1, 0x15);

	VALIDATE(SRibbonTexture, mPage, 0x16);


	VALIDATE(SRibbonTexture, u2, 0x18);
	VALIDATE(SRibbonTexture, v2, 0x19);

	VALIDATE(SRibbonTexture, u3, 0x1A);
	VALIDATE(SRibbonTexture, v3, 0x1B);

	VALIDATE(SRibbonTexture, mTexWin, 0x1C);
}

void validate_SSimpleRibbonParams(void)
{
	VALIDATE_SIZE(SSimpleRibbonParams, 0x1C);

	VALIDATE(SSimpleRibbonParams, field_18, 0x18);
}

void validate_CSpark(void)
{
	VALIDATE_SIZE(CSpark, 0x4C);

	VALIDATE(CSpark, mFadeR, 0x48);
	VALIDATE(CSpark, mFadeG, 0x49);
	VALIDATE(CSpark, mFadeB, 0x4A);
}

#include "my_patch.h"

// @Bogus
void patch_CBit(void)
{
	PATCH_PUSH_RET(0x004088E0, CBit::AttachTo);
	PATCH_PUSH_RET(0x00408900, CBit::DeleteFrom);
	PATCH_PUSH_RET(0x00408930, CBit::Die);
	PATCH_PUSH_RET(0x00408950, CBit::SetPos);
}

// @Bogus
void patch_CFT4Bit(void)
{
	PATCH_PUSH_RET(0x00408C70, CFT4Bit::SetScale);
	PATCH_PUSH_RET(0x00408C80, CFT4Bit::SetSemiTransparent);
	PATCH_PUSH_RET(0x00408CC0, CFT4Bit::SetTint);

	PATCH_PUSH_RET(0x00408CF0, CFT4Bit::SetAnim);
	PATCH_PUSH_RET(0x00408E90, CFT4Bit::SetFrame);
	PATCH_PUSH_RET_POLY(0x00408DF0, CFT4Bit::SetTexture, "?SetTexture@CFT4Bit@@QAEXPAUTexture@@@Z");

	PATCH_PUSH_RET_POLY(0x0040F980, CSimpleAnim::Move, "?Move@CSimpleAnim@@UAEXXZ");
}
