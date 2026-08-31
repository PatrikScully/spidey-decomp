#include "weapons.h"
#include "validate.h"
#include "mem.h"
#include "ps2funcs.h"
#include "camera.h"
#include "spool.h"
#include "utils.h"
#include "db.h"
#include "panel.h"
#include "ps2m3d.h"

CItem* CWeapons;
extern SCamera gMikeCamera[2];

// @MEDIUMTODO
void CGouraudRibbon::Display(void)
{
    printf("CGouraudRibbon::Display(void)");
}

// @Ok
// @Matching
void CGouraudRibbon::SetRGB(u8 a2,u8 a3,u8 a4)
{
	for (i32 i = 0; i < this->mNumPoints; i++)
	{
		this->mpPoints[i].r = a2;
		this->mpPoints[i].g = a3;
		this->mpPoints[i].b = a4;
	}
}

// @Ok
// @Matching
void CGouraudRibbon::SetWidth(u16 Width)
{
	for (i32 i = 0; i < this->mNumPoints; i++)
	{
		this->mpPoints[i].Width = Width;
	}
}

// @Ok
CGouraudRibbon::~CGouraudRibbon(void)
{
	Mem_Delete(this->mpPoints);
}

// @Ok
// @Test
CSmokeRing::CSmokeRing(i32 NumSectors, u32 a3)
{
	this->field_48.vx = 0;
	this->field_48.vy = 0;
	this->field_48.vz = 0;

	print_if_false(NumSectors != 0, "Zero sectors sent to smoke ring");
	this->mpSectors = static_cast<SSmokeRingRelated *>(DCMem_New(sizeof(SSmokeRingRelated) * NumSectors, 0, 1, 0, 1));
	this->mNumSectors = NumSectors;
	this->field_3C = Spool_FindTextureEntry(a3);
	print_if_false(this->field_3C != 0, "Could not find smoke ring texture");

	for (i32 i = 0; i < this->mNumSectors; i++)
	{
		setPolyGT4();

		this->mpSectors[i].field_3B |= 2;
		this->mpSectors[i].field_42 = this->field_3C->clut;
		this->mpSectors[i].field_4E = this->field_3C->tpage;

		setPolyGT4();

		this->mpSectors[i].field_7 |= 2;
		this->mpSectors[i].field_E = this->field_3C->clut;
		this->mpSectors[i].field_1A = this->field_3C->tpage;
	}

	this->SetRGB(128, 128, 128);
	this->SetUV(0, 0, 2);
	this->field_60 = -1;
}

struct SSmokeRingScreenPoint
{
	i32 xyA;
	i16 visibleA;

	PADDING(2);

	i32 xyB;
	i16 visibleB;

	PADDING(2);

	i32 xyC;
	i16 visibleC;
	i16 minDepth;
};

static SSmokeRingScreenPoint* const gSmokeRingScreenPoints = reinterpret_cast<SSmokeRingScreenPoint*>(0x614CD4);

struct SSmokeRingGT4
{
	u32 tag;

	u8 r0,g0,b0,code;
	i32 xy0;
	u8 u0,v0;
	u16 clut;

	u8 r1,g1,b1,pad1;
	i32 xy1;
	u8 u1,v1;
	u16 tpage;

	u8 r2,g2,b2,pad2;
	i32 xy2;
	u8 u2,v2;
	u16 pad3;

	u8 r3,g3,b3,pad4;
	i32 xy3;
	u8 u3,v3;
	u16 pad5;
};

// @Bogus
// internal helper, not a standalone function in the original (the field
// copies are inlined directly in CSmokeRing::Display); factored out here
// only to keep Display's source readable, no independent ground truth
static void CopySmokeRingTemplate(SSmokeRingGT4* pDst, const SSmokeRingGT4* pSrc)
{
	pDst->tag = pSrc->tag;

	pDst->r0 = pSrc->r0;
	pDst->g0 = pSrc->g0;
	pDst->b0 = pSrc->b0;
	pDst->code = pSrc->code;

	pDst->u0 = pSrc->u0;
	pDst->v0 = pSrc->v0;
	pDst->clut = pSrc->clut;

	pDst->r1 = pSrc->r1;
	pDst->g1 = pSrc->g1;
	pDst->b1 = pSrc->b1;
	pDst->pad1 = pSrc->pad1;

	pDst->u1 = pSrc->u1;
	pDst->v1 = pSrc->v1;
	pDst->tpage = pSrc->tpage;

	pDst->r2 = pSrc->r2;
	pDst->g2 = pSrc->g2;
	pDst->b2 = pSrc->b2;
	pDst->pad2 = pSrc->pad2;

	pDst->u2 = pSrc->u2;
	pDst->v2 = pSrc->v2;
	pDst->pad3 = pSrc->pad3;

	pDst->r3 = pSrc->r3;
	pDst->g3 = pSrc->g3;
	pDst->b3 = pSrc->b3;
	pDst->pad4 = pSrc->pad4;

	pDst->u3 = pSrc->u3;
	pDst->v3 = pSrc->v3;
	pDst->pad5 = pSrc->pad5;
}

// @NotOk
void CSmokeRing::Display(void)
{
	for (i32 i = 0; i < this->mNumSectors; i++)
	{
		SSmokeRingScreenPoint* pPoint = &gSmokeRingScreenPoints[i];
		i32 depth;

		depth = Transform(&this->mpSectors[i].field_80, &pPoint->xyA);
		if (pPoint->xyA == 0x3FF03FF || pPoint->xyA == 0x3FFFC00 ||
			pPoint->xyA == (i32)0xFC0003FF || pPoint->xyA == (i32)0xFC00FC00 ||
			depth < -20000 || depth > 20000)
		{
			pPoint->visibleA = 0;
		}
		else
		{
			pPoint->minDepth = (i16)depth;
			pPoint->visibleA = 1;
		}

		depth = Transform(&this->mpSectors[i].field_74, &pPoint->xyB);
		if (pPoint->xyB == 0x3FF03FF || pPoint->xyB == 0x3FFFC00 ||
			pPoint->xyB == (i32)0xFC0003FF || pPoint->xyB == (i32)0xFC00FC00 ||
			depth < -20000 || depth > 20000)
		{
			pPoint->visibleB = 0;
		}
		else
		{
			pPoint->visibleB = 1;
			if (depth < pPoint->minDepth)
				pPoint->minDepth = (i16)depth;
		}

		depth = Transform(&this->mpSectors[i].field_68, &pPoint->xyC);
		if (pPoint->xyC == 0x3FF03FF || pPoint->xyC == 0x3FFFC00 ||
			pPoint->xyC == (i32)0xFC0003FF || pPoint->xyC == (i32)0xFC00FC00 ||
			depth < -20000 || depth > 20000)
		{
			pPoint->visibleC = 0;
		}
		else
		{
			pPoint->visibleC = 1;
			if (depth < pPoint->minDepth)
				pPoint->minDepth = (i16)depth;
		}
	}

	SSmokeRingGT4* pTemplate1 = reinterpret_cast<SSmokeRingGT4*>(&this->mpSectors[0]);
	SSmokeRingGT4* pTemplate2 = reinterpret_cast<SSmokeRingGT4*>(reinterpret_cast<u8*>(&this->mpSectors[0]) + 0x34);

	SSmokeRingScreenPoint prev = gSmokeRingScreenPoints[0];

	for (i32 j = 1; j <= this->mNumSectors; j++)
	{
		SSmokeRingScreenPoint* pCur = (j == this->mNumSectors) ? &gSmokeRingScreenPoints[0] : &gSmokeRingScreenPoints[j];

		i16 depth = pCur->minDepth;
		if (depth < prev.minDepth)
			depth = prev.minDepth;

		SSmokeRingGT4* pBuiltPoly1 = 0;
		SSmokeRingGT4* pBuiltPoly2 = 0;

		if (!this->field_66 || depth != 0)
		{
			if (this->field_60 & (1 << j))
			{
				if (prev.visibleA && prev.visibleB && pCur->visibleA && pCur->visibleB)
				{
					if ((u8*)pPoly + sizeof(SSmokeRingGT4) > PolyBufferEnd)
						return;

					SSmokeRingGT4* pNewPoly = (SSmokeRingGT4*)pPoly;
					pPoly = (u32*)((u8*)pPoly + sizeof(SSmokeRingGT4));

					gsub_46CB90((void*)"stubbed out: setTexWindow");
					gsub_46CB90((void*)"stubbed out: setTexWindow");
					CopySmokeRingTemplate(pNewPoly, pTemplate1);

					pNewPoly->xy0 = prev.xyA;
					pNewPoly->xy1 = pCur->xyA;
					pNewPoly->xy2 = prev.xyB;
					pNewPoly->xy3 = pCur->xyB;

					pBuiltPoly1 = pNewPoly;
				}

				if (prev.visibleC && prev.visibleB && pCur->visibleC && pCur->visibleB)
				{
					if ((u8*)pPoly + sizeof(SSmokeRingGT4) > PolyBufferEnd)
						return;

					SSmokeRingGT4* pNewPoly = (SSmokeRingGT4*)pPoly;
					pPoly = (u32*)((u8*)pPoly + sizeof(SSmokeRingGT4));

					CopySmokeRingTemplate(pNewPoly, pTemplate2);

					pNewPoly->xy0 = prev.xyB;
					pNewPoly->xy1 = pCur->xyB;
					pNewPoly->xy2 = prev.xyC;
					pNewPoly->xy3 = pCur->xyC;

					pBuiltPoly2 = pNewPoly;
				}
			}
		}

		if (pBuiltPoly1)
			gsub_46CB90((void*)gRenderBuf);

		if (pBuiltPoly2)
			gsub_46CB90((void*)gRenderBuf);

		gsub_46CB90((void*)gRenderBuf);
		gsub_46CB90((void*)gRenderBuf);

		prev = *pCur;
	}
}

// @Ok
// @Validate
void CSmokeRing::SetParams(
		const CVector* a2,
		i32 a3,
		i32 a4)
{
	this->mPos = *a2;
	this->field_54 = a4;
	this->field_50 = a3;

	CSVector v15 = this->field_48;

	i32 v23 = 4096 / this->mNumSectors;

	SSmokeRingRelated* pSector = this->mpSectors;
	for (i32 i = 0; i < this->mNumSectors; i++)
	{
		CVector v16;
		v16.vx = 0;
		v16.vy = 0;
		v16.vz = 0;

		Utils_GetVecFromMagDir(&v16, 1, &v15);

		pSector[i].field_68 = (this->mPos + (v16 * this->field_50));
		CVector v14 = (v16 * this->field_54);
		pSector[i].field_74 = (pSector[i].field_68 + v14);
		pSector[i].field_80 = (pSector[i].field_74 + v14);

		v15.vy += v23;
	}
}

// @Ok
// @NonMatching
// @Test
void CSmokeRing::SetRGB(i32 a2, i32 a3, i32 a4)
{
	if (a2 < 0)
		a2 = 0;

	if (a3 < 0)
		a3 = 0;

	if (a4 < 0)
		a4 = 0;

	for (i32 i = 0; i < this->mNumSectors; i++)
	{
			this->mpSectors[i].field_4 = 0;
			this->mpSectors[i].field_5 = 0;
			this->mpSectors[i].field_6 = 0;

			this->mpSectors[i].field_10 = 0;
			this->mpSectors[i].field_11 = 0;
			this->mpSectors[i].field_12 = 0;
			this->mpSectors[i].field_1C = a2;
			this->mpSectors[i].field_1D = a3;
			this->mpSectors[i].field_1E = a4;

			this->mpSectors[i].field_28 = a2;
			this->mpSectors[i].field_29 = a3;
			this->mpSectors[i].field_2A = a4;

			this->mpSectors[i].field_38 = a2;
			this->mpSectors[i].field_39 = a3;
			this->mpSectors[i].field_3A = a4;

			this->mpSectors[i].field_44 = a2;
			this->mpSectors[i].field_45 = a3;
			this->mpSectors[i].field_46 = a4;

			this->mpSectors[i].field_50 = 0;
			this->mpSectors[i].field_51 = 0;
			this->mpSectors[i].field_52 = 0;

			this->mpSectors[i].field_5C = 0;
			this->mpSectors[i].field_5D = 0;
			this->mpSectors[i].field_5E = 0;
	}
}

// @Ok
// checked against 0x4F4C80: field offsets (field_C/D, field_18/19, field_24/25,
// field_30/31, field_40/41, field_4C/4D, field_58/59, field_64/65), the u0/v0 read
// from field_3C, the step computation and the wrap with & 0x3F all match.
void CSmokeRing::SetUV(i32 a2,i32 a3,i32 a4)
{
	this->field_58 = a2;
	this->field_5C = a3;
	u8 v6 = this->field_3C->u0 + (a2 & 0x3F);

	a3 = (a3 & 0x3F) + this->field_3C->v0;
	i32 v9 = (a4 << 6) / this->mNumSectors;

	u8 a2a = a3 + 32;
	u8 a4a = a3 + 64;
	for (i32 i = 0; i < this->mNumSectors; i++)
	{
		u8 v12 = v6 + v9;
		this->mpSectors[i].field_C = v6;
		this->mpSectors[i].field_D = a3;

		this->mpSectors[i].field_18 = v12;
		this->mpSectors[i].field_19 = a3;

		this->mpSectors[i].field_24 = v6;
		this->mpSectors[i].field_25 = a2a;

		this->mpSectors[i].field_30 = v12;
		this->mpSectors[i].field_31 = a2a;

		this->mpSectors[i].field_40 = v6;
		this->mpSectors[i].field_41 = a2a;

		this->mpSectors[i].field_4C = v12;
		this->mpSectors[i].field_4D = a2a;

		this->mpSectors[i].field_58 = v6;
		this->mpSectors[i].field_59 = a4a;

		this->mpSectors[i].field_64 = v12;
		this->mpSectors[i].field_65 = a4a;

		v6 = (v9 + v6) & 0x3F;
	}
}

// @Ok
CSmokeRing::~CSmokeRing(void)
{
	Mem_Delete(this->mpSectors);
}

// @Ok
// @Matching
CTexturedRibbon::CTexturedRibbon(i32 NumPoints,i32 LeaveTrail)
{
	print_if_false(NumPoints > 1, "NumPoints must be at least 2");
	print_if_false((u32)NumPoints <= 0x20, "NumPoints too big for buffer.");
	this->mNumPoints = NumPoints;

	this->mpPoints = static_cast<SRibbonPoint *>(DCMem_New(sizeof(SRibbonPoint) * NumPoints, 0, 1, 0, 1));

	for (i32 i = 0; i < this->mNumPoints; i++)
	{
		this->mpPoints[i].WidthB = 0;
		this->mpPoints[i].Width = 0;
	}

	print_if_false(!LeaveTrail || LeaveTrail == 1, "LeaveTrail must be 0 or 1");
	this->mTrail = LeaveTrail;
	this->field_50 = 8;

	this->field_60 = static_cast<int *>(DCMem_New(8 * NumPoints + 4, 0, 1, 0, 1));
	this->field_60[0] = 0;
}

// @Ok
// matching
void CTexturedRibbon::SetCoreRGBi(
		i32 a2,
		u8 a3,
		u8 a4,
		u8 a5)
{
	this->field_60[a2 + 1 + this->mNumPoints] = (((a5 << 8) | a4) << 8) | a3;
}

// @Ok
void CTexturedRibbon::SetOuterRGBi(i32 index, u8 a3, u8 a4,u8 a5)
{
	this->field_60[index+1] = (a3 | (((a5 << 8) | a4) << 8));
}

// @Ok
// @Matching
void CTexturedRibbon::SetTexture(Texture* pTex)
{
	print_if_false(pTex != 0, "no texture for ribbon");
	this->field_40 = pTex;
}

// @Ok
CTexturedRibbon::~CTexturedRibbon(void)
{
	Mem_Delete(this->mpPoints);
	Mem_Delete(this->field_60);
}

// @Ok
// @Validate
void CalcScreenNormal(
		SCalcBuffer* pBuffer,
		i32 * a2,
		i32 * a3,
		i32 a4)
{
	if (!pBuffer->field_18 && !pBuffer->field_38)
	{
		i32 v4 = pBuffer->field_20;
		i32 v5 = ((pBuffer->field_20 << 16) >> 16) - ((pBuffer->field_0 << 16) >> 16);
		i32 v6 = (pBuffer->field_20 >> 16) - ((pBuffer->field_0 << 16) >> 16);
		*a2 = v6;
		i32 v7 = 320 * v5 / 512;
		i32 v8 = 320 * v5 / -512;
		*a3 = v8;
		if ( v7 < 0 )
			v7 = v8;
		if ( v6 < 0 )
			v6 = -v6;

		i32 v9;
		if ( v7 <= v6 )
			v9 = v6 + v7 / 2;
		else
			v9 = v7 + v6 / 2;
		if ( v9 >= a4 )
		{
			*a2 = (*a2 << 6) / v9;
			*a3 = (*a3 << 6) / v9;
			*a2 = (*a2 << 9) / 320;
		}
		else
		{
			*a3 = 0;
			*a2 = 0;
		}
	}
	else
	{
		*a3 = 0;
		*a2 = 0;
	}
}

// @Ok
// checked against 0x4F4DF0 (CSmokeRing::Display), the only caller: field order,
// camera subtraction (gMikeCamera[0].Position split across qword_56F1B4 low/high
// and dword_56F1BC) and the gte_ call sequence all match.
INLINE i32 Transform(CVector *a1, i32* a2)
{
	CVector v8;
	v8.vx = 0;
	v8.vy = 0;
	v8.vz = 0;


	v8.vx = (a1->vx >> 12) - gMikeCamera[0].Position.vx;
	v8.vy = (a1->vy >> 12) - gMikeCamera[0].Position.vy;
	v8.vz = (a1->vz >> 12) - gMikeCamera[0].Position.vz;

	gte_ldlv0(reinterpret_cast<VECTOR*>(&v8));

	gte_rtps();
	gte_stsxy(a2);

	i32 v7;
	gte_stlvnl2(&v7);

	return v7;
}

// @Ok
CGouraudRibbon::CGouraudRibbon(i32 NumPoints, i32 LeaveTrail)
{
	print_if_false(NumPoints > 1, "NumPoints must be at least 2");
	print_if_false((u32)NumPoints <= 0x20, "NumPoints too big for buffer.");

	this->mNumPoints = NumPoints;

	this->mpPoints = static_cast<SRibbonPoint *>(
			DCMem_New(
				sizeof(SRibbonPoint) * NumPoints,
				0,
				1,
				0,
				1));

	print_if_false(LeaveTrail == 0 || LeaveTrail == 1, "LeaveTrail must be 0 or 1");

	this->mTrail = LeaveTrail;
}

void validate_CGouraudRibbon(void)
{
	VALIDATE_SIZE(CGouraudRibbon, 0x48);

	VALIDATE(CGouraudRibbon, mTrail, 0x3C);
	VALIDATE(CGouraudRibbon, mNumPoints, 0x40);
	VALIDATE(CGouraudRibbon, mpPoints, 0x44);
}

void validate_CSmokeRing(void)
{
	VALIDATE_SIZE(CSmokeRing, 0x6C);

	VALIDATE(CSmokeRing, field_3C, 0x3C);
	VALIDATE(CSmokeRing, mNumSectors, 0x40);
	VALIDATE(CSmokeRing, mpSectors, 0x44);

	VALIDATE(CSmokeRing, field_48, 0x48);

	VALIDATE(CSmokeRing, field_50, 0x50);
	VALIDATE(CSmokeRing, field_54, 0x54);

	VALIDATE(CSmokeRing, field_58, 0x58);
	VALIDATE(CSmokeRing, field_5C, 0x5C);
	VALIDATE(CSmokeRing, field_60, 0x60);

	VALIDATE(CSmokeRing, field_66, 0x66);

	VALIDATE(CSmokeRing, field_68, 0x68);
	VALIDATE(CSmokeRing, field_6A, 0x6A);
}

void validate_CTexturedRibbon(void)
{
	VALIDATE(CTexturedRibbon, mTrail, 0x3C);

	VALIDATE(CTexturedRibbon, field_40, 0x40);
	VALIDATE(CTexturedRibbon, field_50, 0x50);

	VALIDATE(CTexturedRibbon, mNumPoints, 0x58);
	VALIDATE(CTexturedRibbon, mpPoints, 0x5C);

	VALIDATE(CTexturedRibbon, field_60, 0x60);
}

void validate_SSmokeRingRelated(void)
{
	VALIDATE_SIZE(SSmokeRingRelated, 0x8C);

	VALIDATE(SSmokeRingRelated, field_4, 0x4);
	VALIDATE(SSmokeRingRelated, field_5, 0x5);
	VALIDATE(SSmokeRingRelated, field_6, 0x6);

	VALIDATE(SSmokeRingRelated, field_7, 0x7);

	VALIDATE(SSmokeRingRelated, field_C, 0xC);
	VALIDATE(SSmokeRingRelated, field_D, 0xD);

	VALIDATE(SSmokeRingRelated, field_E, 0xE);

	VALIDATE(SSmokeRingRelated, field_18, 0x18);
	VALIDATE(SSmokeRingRelated, field_19, 0x19);

	VALIDATE(SSmokeRingRelated, field_1A, 0x1A);

	VALIDATE(SSmokeRingRelated, field_1C, 0x1C);
	VALIDATE(SSmokeRingRelated, field_1D, 0x1D);
	VALIDATE(SSmokeRingRelated, field_1E, 0x1E);

	VALIDATE(SSmokeRingRelated, field_24, 0x24);
	VALIDATE(SSmokeRingRelated, field_25, 0x25);

	VALIDATE(SSmokeRingRelated, field_28, 0x28);
	VALIDATE(SSmokeRingRelated, field_29, 0x29);
	VALIDATE(SSmokeRingRelated, field_2A, 0x2A);

	VALIDATE(SSmokeRingRelated, field_30, 0x30);
	VALIDATE(SSmokeRingRelated, field_31, 0x31);

	VALIDATE(SSmokeRingRelated, field_38, 0x38);
	VALIDATE(SSmokeRingRelated, field_39, 0x39);
	VALIDATE(SSmokeRingRelated, field_3A, 0x3A);

	VALIDATE(SSmokeRingRelated, field_3B, 0x3B);

	VALIDATE(SSmokeRingRelated, field_40, 0x40);
	VALIDATE(SSmokeRingRelated, field_41, 0x41);
	VALIDATE(SSmokeRingRelated, field_42, 0x42);

	VALIDATE(SSmokeRingRelated, field_44, 0x44);
	VALIDATE(SSmokeRingRelated, field_45, 0x45);
	VALIDATE(SSmokeRingRelated, field_46, 0x46);

	VALIDATE(SSmokeRingRelated, field_4C, 0x4C);
	VALIDATE(SSmokeRingRelated, field_4D, 0x4D);
	VALIDATE(SSmokeRingRelated, field_4E, 0x4E);

	VALIDATE(SSmokeRingRelated, field_50, 0x50);
	VALIDATE(SSmokeRingRelated, field_51, 0x51);
	VALIDATE(SSmokeRingRelated, field_52, 0x52);

	VALIDATE(SSmokeRingRelated, field_58, 0x58);
	VALIDATE(SSmokeRingRelated, field_59, 0x59);

	VALIDATE(SSmokeRingRelated, field_5C, 0x5C);
	VALIDATE(SSmokeRingRelated, field_5D, 0x5D);
	VALIDATE(SSmokeRingRelated, field_5E, 0x5E);

	VALIDATE(SSmokeRingRelated, field_64, 0x64);
	VALIDATE(SSmokeRingRelated, field_65, 0x65);

	VALIDATE(SSmokeRingRelated, field_68, 0x68);
	VALIDATE(SSmokeRingRelated, field_74, 0x74);
	VALIDATE(SSmokeRingRelated, field_80, 0x80);
}

void validate_SCalcBuffer(void)
{
	VALIDATE_SIZE(SCalcBuffer, 0x3C);

	VALIDATE(SCalcBuffer, field_0, 0x0);
	VALIDATE(SCalcBuffer, field_4, 0x4);

	VALIDATE(SCalcBuffer, field_18, 0x18);
	VALIDATE(SCalcBuffer, field_20, 0x20);

	VALIDATE(SCalcBuffer, field_38, 0x38);
}
