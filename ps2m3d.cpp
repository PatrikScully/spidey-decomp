#include "ps2m3d.h"
#include "ps2funcs.h"
#include "db.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "spool.h"
#include <math.h>
#include <string.h>

#include "validate.h"

i32 gWideScreen;

EXPORT u32* pColourTable;

// XblanksNow / XblanksThen from the PS2 source (m3d.mik): vblank-based frame
// timer, updated elsewhere (not in this file). Only referenced (read) here.
// Not in idb_globals.txt, only ever read in ps2m3d.cpp functions.
//#define G_XBLANKS_NOW (gXblanksNow)
#define G_XBLANKS_NOW (*reinterpret_cast<u32*>(0x0065CFA4))
//#define G_XBLANKS_THEN (gXblanksThen)
#define G_XBLANKS_THEN (*reinterpret_cast<u32*>(0x00660F88))


// @Ok
// @Matching
vector4d& vector4d::operator=(const vector4d& other)
{
	this->field_0[0] = other.field_0[0];
	this->field_0[1] = other.field_0[1];
	this->field_0[2] = other.field_0[2];
	this->field_0[3] = other.field_0[3];

	return *this;
}

// @Ok
INLINE vector4d& matrix4x4::operator[](i32 index)
{
	return this->field_0[index];
}

// @Ok
// @Matching
matrix4x4::matrix4x4(
			f32 a1,
			f32 a2,
			f32 a3,
			f32 a4,
			f32 a5,
			f32 a6,
			f32 a7,
			f32 a8,
			f32 a9,
			f32 a10,
			f32 a11,
			f32 a12,
			f32 a13,
			f32 a14,
			f32 a15,
			f32 a16)
{
	this->field_0[0].field_0[0] = a1;
	this->field_0[0].field_0[1] = a2;
	this->field_0[0].field_0[2] = a3;
	this->field_0[0].field_0[3] = a4;
	
	this->field_0[1].field_0[0] = a5;
	this->field_0[1].field_0[1] = a6;
	this->field_0[1].field_0[2] = a7;
	this->field_0[1].field_0[3] = a8;
	
	this->field_0[2].field_0[0] = a9;
	this->field_0[2].field_0[1] = a10;
	this->field_0[2].field_0[2] = a11;
	this->field_0[2].field_0[3] = a12;
	
	this->field_0[3].field_0[0] = a13;
	this->field_0[3].field_0[1] = a14;
	this->field_0[3].field_0[2] = a15;
	this->field_0[3].field_0[3] = a16;
	
}

// @Ok
// @AlmostMatching: 48 positional mnemonic diffs out of 254 instructions,
// one instruction short of the original (253 vs 254). Frame size (0x80),
// pushes (ebx,esi,edi), all 16 dot products, the 16 temp slots, the
// inlined result stores and the whole row-copy loop body are mnemonic
// identical. The residue sits in the copy-loop setup only: the original
// walks the result rows in ecx and the dest rows in edx, and forms the
// diff as dest minus result, which needs an extra "mov esi,eax" to keep
// dest in eax for the return. Our build picks the mirrored roles (dest in
// ecx, result in edx, diff formed as result minus dest), which needs one
// instruction less, so the loop counter setup gets scheduled earlier into
// the FPU stream and the lines in between shift by one slot. 19 hypotheses
// tried in total (see the ps2m3d attempts log): 16-arg ctor with the
// expressions as args, ctor with named locals (goes out of line), direct
// field stores with void return, matrix4x4* return (fixed the prologue,
// frame size and push set), every combination of operator[] and .field_0
// indexing on both sides of the row copy, reference binding on either
// side, explicit walker pointers for source and dest, *dest = result,
// do/while loop shape, and declaration order swaps. The walker role choice
// never flipped.
//
// Not one of the file's original 7 stubs. Added because M3d_RenderSetup,
// M3d_Render and RenderSuperItem all call it (leaf-first dependency) to
// concatenate transforms; found via the maintainer's IDB (spideypc_names.txt
// calls it matrix4x4_ml), named gsub_476A00 here since tools/names.json only
// has it as sub_476A00.
// dest is written through a local first because dest may alias a or b (e.g.
// gsub_476A00(&m, &m, &n) to do "m = m * n" in place); writing straight into
// *dest while still reading it back for later cells would corrupt the
// result. Returns dest, like the original (eax holds dest at ret).
EXPORT matrix4x4* gsub_476A00(matrix4x4* dest, matrix4x4 const* a, matrix4x4 const* b)
{
	f32 m33 = a->field_0[3].field_0[2] * b->field_0[2].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[3] + a->field_0[3].field_0[0] * b->field_0[0].field_0[3] + b->field_0[3].field_0[3] * a->field_0[3].field_0[3];
	f32 m32 = b->field_0[3].field_0[2] * a->field_0[3].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[2] + a->field_0[3].field_0[2] * b->field_0[2].field_0[2] + a->field_0[3].field_0[0] * b->field_0[0].field_0[2];
	f32 m31 = a->field_0[3].field_0[2] * b->field_0[2].field_0[1] + a->field_0[3].field_0[0] * b->field_0[0].field_0[1] + b->field_0[3].field_0[1] * a->field_0[3].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[1];
	f32 m30 = a->field_0[3].field_0[1] * b->field_0[1].field_0[0] + b->field_0[3].field_0[0] * a->field_0[3].field_0[3] + a->field_0[3].field_0[2] * b->field_0[2].field_0[0] + a->field_0[3].field_0[0] * b->field_0[0].field_0[0];
	f32 m23 = a->field_0[2].field_0[2] * b->field_0[2].field_0[3] + a->field_0[2].field_0[3] * b->field_0[3].field_0[3] + a->field_0[2].field_0[0] * b->field_0[0].field_0[3] + a->field_0[2].field_0[1] * b->field_0[1].field_0[3];
	f32 m22 = a->field_0[2].field_0[0] * b->field_0[0].field_0[2] + a->field_0[2].field_0[3] * b->field_0[3].field_0[2] + a->field_0[2].field_0[2] * b->field_0[2].field_0[2] + a->field_0[2].field_0[1] * b->field_0[1].field_0[2];
	f32 m21 = a->field_0[2].field_0[3] * b->field_0[3].field_0[1] + a->field_0[2].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[2].field_0[1] + a->field_0[2].field_0[0] * b->field_0[0].field_0[1];
	f32 m20 = b->field_0[1].field_0[0] * a->field_0[2].field_0[1] + a->field_0[2].field_0[0] * b->field_0[0].field_0[0] + a->field_0[2].field_0[3] * b->field_0[3].field_0[0] + a->field_0[2].field_0[2] * b->field_0[2].field_0[0];
	f32 m13 = a->field_0[1].field_0[2] * b->field_0[2].field_0[3] + a->field_0[1].field_0[3] * b->field_0[3].field_0[3] + a->field_0[1].field_0[0] * b->field_0[0].field_0[3] + a->field_0[1].field_0[1] * b->field_0[1].field_0[3];
	f32 m12 = a->field_0[1].field_0[0] * b->field_0[0].field_0[2] + b->field_0[3].field_0[2] * a->field_0[1].field_0[3] + a->field_0[1].field_0[2] * b->field_0[2].field_0[2] + a->field_0[1].field_0[1] * b->field_0[1].field_0[2];
	f32 m11 = a->field_0[1].field_0[3] * b->field_0[3].field_0[1] + a->field_0[1].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[1].field_0[1] + a->field_0[1].field_0[0] * b->field_0[0].field_0[1];
	f32 m10 = b->field_0[1].field_0[0] * a->field_0[1].field_0[1] + a->field_0[1].field_0[0] * b->field_0[0].field_0[0] + a->field_0[1].field_0[3] * b->field_0[3].field_0[0] + a->field_0[1].field_0[2] * b->field_0[2].field_0[0];
	f32 m03 = a->field_0[0].field_0[2] * b->field_0[2].field_0[3] + a->field_0[0].field_0[3] * b->field_0[3].field_0[3] + a->field_0[0].field_0[0] * b->field_0[0].field_0[3] + b->field_0[1].field_0[3] * a->field_0[0].field_0[1];
	f32 m02 = b->field_0[0].field_0[2] * a->field_0[0].field_0[0] + b->field_0[3].field_0[2] * a->field_0[0].field_0[3] + b->field_0[2].field_0[2] * a->field_0[0].field_0[2] + b->field_0[1].field_0[2] * a->field_0[0].field_0[1];
	f32 m01 = a->field_0[0].field_0[3] * b->field_0[3].field_0[1] + a->field_0[0].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[0].field_0[1] + a->field_0[0].field_0[0] * b->field_0[0].field_0[1];
	f32 m00 = b->field_0[1].field_0[0] * a->field_0[0].field_0[1] + a->field_0[0].field_0[0] * b->field_0[0].field_0[0] + a->field_0[0].field_0[3] * b->field_0[3].field_0[0] + a->field_0[0].field_0[2] * b->field_0[2].field_0[0];

	matrix4x4 result;
	result.field_0[0].field_0[0] = m00;
	result.field_0[0].field_0[1] = m01;
	result.field_0[0].field_0[2] = m02;
	result.field_0[0].field_0[3] = m03;
	result.field_0[1].field_0[0] = m10;
	result.field_0[1].field_0[1] = m11;
	result.field_0[1].field_0[2] = m12;
	result.field_0[1].field_0[3] = m13;
	result.field_0[2].field_0[0] = m20;
	result.field_0[2].field_0[1] = m21;
	result.field_0[2].field_0[2] = m22;
	result.field_0[2].field_0[3] = m23;
	result.field_0[3].field_0[0] = m30;
	result.field_0[3].field_0[1] = m31;
	result.field_0[3].field_0[2] = m32;
	result.field_0[3].field_0[3] = m33;

	for (i32 i = 0; i < 4; i++)
	{
		(*dest)[i] = result[i];
	}

	return dest;
}

/*
EXPORT void __ml(matrix4x4 const *,matrix4x4 const *);

EXPORT void matrix4x4::__vc(const(i32);
EXPORT matrix4x4::matrix4x4(f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32,f32);

EXPORT void uWibble(STexWibVertInfo *);
EXPORT void vWibble(STexWibVertInfo *);

EXPORT void vector4d::__vc(const(i32);
*/


// @FIXME
char gRenderBuf[4] = { 0, 0, 0, 0 };

// @Ok
// @Matching
void M3d_BuildTransform(CSuper* pSuper)
{
	if ((pSuper->mExtraFlags & 1) == 0 )
	{
		M3dMaths_RotMatrixYXZ(
				reinterpret_cast<SVECTOR *>(&pSuper->mAngles),
				&pSuper->mTransform);
	}
	if (pSuper->mFlags & 0x200)
	{
		M3dMaths_ScaleMatrix(pSuper, &pSuper->mTransform);
	}

	pSuper->mTransform.t[0] = pSuper->mPos.vx >> 12;
	pSuper->mTransform.t[1] = pSuper->mPos.vy >> 12;
	pSuper->mTransform.t[2] = pSuper->mPos.vz >> 12;
}

// @BIGTODO
void M3d_Render(void*)
{
	printf("void M3d_Render(void*)");
}

// @MEDIUMTODO
void DCModel_RenderModel(SModel const *,DCModelData *,matrix4x4 const *)
{
    printf("DCModel_RenderModel(SModel const *,DCModelData *,matrix4x4 const *)");
}

// @MEDIUMTODO
void DC_PSXModel_RenderModel(SModel const *,matrix4x4 const *,void const *,DCModelData *)
{
    printf("DC_PSXModel_RenderModel(SModel const *,matrix4x4 const *,void const *,DCModelData *)");
}

struct SRGBI
{
	u8 r;
	u8 g;
	u8 b;
	u8 Interval;
};

struct SColourPulseInfo
{
	u8 VertexColourIndex;
	u8 ListLen;
	u8 ListPos;
	u8 t;
	SRGBI RGBs[1];
};

// @Ok
// (0x00476790). Advances the per-record colour-pulse phase by the frame
// delta (from the xblank counters) and wraps the list position. Logic
// verified against the IDB. The build is 11 bytes longer than the original
// because MSVC6 saves ebp in the prologue while the original defers it to
// the loop entry (a register save/restore timing difference only, no logic
// difference); left as-is in the functional phase.


void M3d_PreprocessPulsingColours(i32 Region)
{
	if (Region == -1)
	{
		return;
	}

	u32 *pData = PSXRegion[Region].pColourPulseData;
	if (!pData)
	{
		return;
	}

	print_if_false(pData[-2] == 7, "Pointer doesn't point to a colour pulsing packet");

	u32 *pEnd = reinterpret_cast<u32*>(reinterpret_cast<u8*>(pData) + pData[-1]);

	G_COLOUR_TABLE = PSXRegion[Region].pColourTable;
	print_if_false(G_COLOUR_TABLE != 0, "Pulsing a non-existent colour table");

	i32 dt;
	if (G_XBLANKS_NOW < G_XBLANKS_THEN)
	{
		dt = 1;
	}
	else
	{
		dt = G_XBLANKS_NOW - G_XBLANKS_THEN;
	}

	while (pData < pEnd)
	{
		SColourPulseInfo *pColourPulseInfo = reinterpret_cast<SColourPulseInfo*>(pData);

		print_if_false(pColourPulseInfo->ListLen != 0, "Zero list length");

		SRGBI *RGBs = pColourPulseInfo->RGBs;
		i32 ListLen = pColourPulseInfo->ListLen;
		i32 ListPos = pColourPulseInfo->ListPos;
		i32 t = pColourPulseInfo->t;

		t += dt;
		while (t >= RGBs[ListPos].Interval)
		{
			t -= RGBs[ListPos].Interval;
			ListPos++;
			if (ListPos == ListLen)
			{
				ListPos = 0;
			}
		}

		pColourPulseInfo->ListPos = ListPos;
		pColourPulseInfo->t = t;

		i32 rgb0[3];
		rgb0[0] = RGBs[ListPos].r;
		rgb0[1] = RGBs[ListPos].g;
		rgb0[2] = RGBs[ListPos].b;

		i32 OldListPos = ListPos;
		ListPos++;
		if (ListPos == ListLen)
		{
			ListPos = 0;
		}

		i32 drgb[3];
		drgb[0] = RGBs[ListPos].r;
		drgb[1] = RGBs[ListPos].g;
		drgb[2] = RGBs[ListPos].b;

		print_if_false(RGBs[ListPos].Interval != 0, "Zero interval");

		i32 interval = RGBs[OldListPos].Interval;

		i32 blue = rgb0[2] + t * (drgb[2] - rgb0[2]) / interval;
		i32 green = rgb0[1] + t * (drgb[1] - rgb0[1]) / interval;
		i32 red = rgb0[0] + t * (drgb[0] - rgb0[0]) / interval;

		u32 colour = gConvertedColors[red & 0xFF] | (gConvertedColors[green & 0xFF] << 8) | (gConvertedColors[blue & 0xFF] << 16);

		G_COLOUR_TABLE[pColourPulseInfo->VertexColourIndex] = colour;

		pData = reinterpret_cast<u32*>(pColourPulseInfo + 1) + ListLen - 1;
	}
}

static i32 * const gWibbleTables = (i32*)0x00660748;
static volatile i32 * const gM3dWibbleFrame = (i32*)0x0065CFA4;
static volatile i32 * const gM3dWibbleScroll = (i32*)0x0065F724;
static volatile u32 * const * const gM3dWibbleModelData = (u32* const*)0x005F6764;

// @Ok
// (0x00475FB0, 1848 bytes). Preprocesses the wibble (texture animation)
// packets for a PSX region. For each wibble packet it reads the per-face
// wibble indices, looks them up in gWibbleTables (offset by the frame),
// adds the scroll offset, and writes the resulting texture coordinates
// (u,v) into the model's face data, scaled by the texture's 1/width,1/height.
void M3d_PreprocessWibblyTextures(i32 region)
{
	if (region == -1)
		return;

	u32 *pTexWibData = PSXRegion[region].pTexWibData;
	if (pTexWibData == 0)
		return;

	print_if_false(*((i32*)(pTexWibData - 8)) == 6, "Pointer doesn't point to a texture-wibble packet");

	f32 invWidth = 1.0f;
	f32 invHeight = 1.0f;
	f32 texX = 1.0f;
	f32 texY = 1.0f;

	u32 *packet = pTexWibData;
	while (*packet != 0)
	{
		print_if_false(((u32)packet - 12) % 0x24 == 0, "PreProcessWibblyTextures(): itemIndex not computed correctly.");

		u32 *pSuperBase = (u32*)((u8*)&PSXRegion[0].pSuper + 68 * region);
		u32 *pItem = (u32*)(*pSuperBase + 64 * ((*packet - 12) / 0x24));
		u32 *pNext = packet + 4;
		u16 numFaces = *(u16*)(packet + 12);

		if (pItem[1] >= 0)  // field at +5 (char)
		{
			u16 modelIndex = *(u16*)((u8*)pItem + 0x1A);
			u8 itemRegion = *(u8*)((u8*)pItem + 0x1F);
			SModel *pModel = PSXRegion[itemRegion].ppModels[modelIndex];
			i32 modelDataBase = (int)gM3dWibbleModelData[itemRegion];
			i32 faceBase = *(i32*)(modelDataBase + 4 * (9 * modelIndex) + 4);
			i32 faceList = modelDataBase + 4 * (9 * modelIndex);

			i32 frame = (*gM3dWibbleFrame & 0x3FF) + 1;
			i32 scrollY = (frame * *(i16*)(packet + 6)) >> 4;
			i32 scrollX = (frame * *(i16*)(packet + 4)) >> 4;
			gM3dWibbleScroll[0] = (frame * *(i32*)(packet + 8)) >> 10;

			if (numFaces > 0)
			{
				f32 *pFaceData = (f32*)(faceBase + 36);
				u8 *pWibData = (u8*)(packet + 23);
				pNext += 16 * numFaces;
				i32 facePtr = (int)pModel + 4 * *(u16*)((u8*)pModel + 4) + 14 * 4 + 4 * *(u16*)((u8*)pModel + 8);

				for (i32 f = numFaces; f != 0; f--)
				{
					print_if_false(*(u8*)facePtr & 1, "Wibbling a non-textured face");
					if ((scrollX != 0 || scrollY != 0) && (*((u8*)facePtr) & 0x20) == 0)
						print_if_false(0, "Scrolling a non-tiled texture");

					i32 tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
					if ((*(i32*)(faceList + 12) & 0x400) != 0)
					{
						tx0 = *(u8*)((u8*)facePtr + 20) << 24;
						ty0 = *(u8*)((u8*)facePtr + 21) << 24;
						tx1 = *(u8*)((u8*)facePtr + 22) << 24;
						ty1 = *(u8*)((u8*)facePtr + 23) << 24;
						tx2 = *(u8*)((u8*)facePtr + 24) << 24;
						ty2 = *(u8*)((u8*)facePtr + 25) << 24;
						tx3 = *(u8*)((u8*)facePtr + 26) << 24;
						ty3 = *(u8*)((u8*)facePtr + 27) << 24;
					}
					else
					{
						tx0 = *(u16*)((u8*)facePtr + 20) << 8;
						ty0 = *(u16*)((u8*)facePtr + 28) << 8;
						tx1 = *(u16*)((u8*)facePtr + 22) << 8;
						ty1 = *(u16*)((u8*)facePtr + 30) << 8;
						tx2 = *(u16*)((u8*)facePtr + 24) << 8;
						ty2 = *(u16*)((u8*)facePtr + 32) << 8;
						tx3 = *(u16*)((u8*)facePtr + 26) << 8;
						ty3 = *(u16*)((u8*)facePtr + 34) << 8;
					}
					if (scrollX != 0)
					{
						tx0 += 2 * scrollX;
						tx1 += 2 * scrollX;
						tx2 += 2 * scrollX;
						tx3 += 2 * scrollX;
					}
					if (scrollY != 0)
					{
						ty0 += 2 * scrollY;
						ty1 += 2 * scrollY;
						ty2 += 2 * scrollY;
						ty3 += 2 * scrollY;
					}

					i32 w0 = *(pWibData - 5) >> 4;
					i32 w1 = *(pWibData - 4) >> 4;
					i32 w2 = *(pWibData - 1) >> 4;
					i32 w3 = *pWibData >> 4;
					i32 w4 = pWibData[3] >> 4;
					i32 w5 = pWibData[4] >> 4;
					i32 w6 = pWibData[7] >> 4;
					i32 w7 = pWibData[8] >> 4;

					i32 d0 = (w0 != 0) ? gWibbleTables[64 * w0 + (((u8)gM3dWibbleScroll[0] + 4 * (*(pWibData - 5) & 0xF)) & 0x3F)] : 0;
					i32 d1 = (w1 != 0) ? gWibbleTables[64 * w1 + (((u8)gM3dWibbleScroll[0] + 4 * (*(pWibData - 4) & 0xF)) & 0x3F)] : 0;
					i32 d2 = (w2 != 0) ? gWibbleTables[64 * w2 + (((u8)gM3dWibbleScroll[0] + 4 * (*(pWibData - 1) & 0xF)) & 0x3F)] : 0;
					i32 d3 = (w3 != 0) ? gWibbleTables[64 * w3 + (((u8)gM3dWibbleScroll[0] + 4 * (*pWibData & 0xF)) & 0x3F)] : 0;
					i32 d4 = (w4 != 0) ? gWibbleTables[64 * w4 + (((u8)gM3dWibbleScroll[0] + 4 * (pWibData[3] & 0xF)) & 0x3F)] : 0;
					i32 d5 = (w5 != 0) ? gWibbleTables[64 * w5 + (((u8)gM3dWibbleScroll[0] + 4 * (pWibData[4] & 0xF)) & 0x3F)] : 0;
					i32 d6 = (w6 != 0) ? gWibbleTables[64 * w6 + (((u8)gM3dWibbleScroll[0] + 4 * (pWibData[7] & 0xF)) & 0x3F)] : 0;
					i32 d7 = (w7 != 0) ? gWibbleTables[64 * w7 + (((u8)gM3dWibbleScroll[0] + 4 * (pWibData[8] & 0xF)) & 0x3F)] : 0;

					i32 cu0 = d0 + tx0;
					i32 cv0 = d1 + ty0;
					i32 cu1 = d2 + tx1;
					i32 cv1 = d3 + ty1;
					i32 cu2 = d4 + tx2;
					i32 cv2 = d5 + ty2;
					i32 cu3 = d6 + tx3;
					i32 cv3 = d7 + ty3;

					i32 texData = *(i32*)((u8*)facePtr + 16);
					print_if_false(texData != 0, "No Texture data");
					if (texData != 0)
					{
						u8 *pVram = *(u8**)(texData + 28);
						print_if_false(pVram != 0, "Texture has no pVRAMRect info");
						print_if_false(*(i32*)(pVram + 4) != 0, "pVRAMRect info has no pack info");
						u8 vramType = *pVram;
						if ((vramType & 8) != 0)
						{
							i32 pack = *(i32*)(pVram + 4);
							texX = (f32)(2 * (*(u8*)pack & 0x7F));
							texY = (f32)*(u8*)(pack + 2);
							invWidth = (f32)(2 * *(u16*)(pack + 4));
							invHeight = (f32)*(u16*)(pack + 6);
						}
						else
						{
							u8 *vramData = *(u8**)(pVram + 4);
							if ((vramType & 0x10) != 0)
							{
								texX = (f32)*vramData;
								texY = (f32)vramData[2];
								invWidth = (f32)*(u16*)(vramData + 4);
							}
							else
							{
								print_if_false((vramType & 4) != 0, "Unexpected Texture bit depth");
								texX = (f32)(4 * (*vramData & 0x3F));
								texY = (f32)vramData[2];
								invWidth = (f32)(4 * *(u16*)(vramData + 4));
							}
							invHeight = (f32)*(u16*)(vramData + 6);
						}
						if (invWidth == 0.0f)
							print_if_false(0, "Zero Tex Width");
						if (invHeight == 0.0f)
							print_if_false(0, "Zero Tex Height");
						invWidth = 1.0f / invWidth;
						invHeight = 1.0f / invHeight;
					}

					f32 *out = pFaceData;
					out[-4] = ((f32)(cu0 >> 8) - texX) * invWidth;
					out[0] = ((f32)(cv0 >> 8) - texY) * invHeight;
					out[-3] = ((f32)(cu1 >> 8) - texX) * invWidth;
					out[1] = ((f32)(cv1 >> 8) - texY) * invHeight;
					f32 *out2 = pFaceData + 14;
					out2[-16] = ((f32)(cu2 >> 8) - texX) * invWidth;
					out2[-12] = ((f32)(cv2 >> 8) - texY) * invHeight;
					out2[-15] = ((f32)(cu3 >> 8) - texX) * invWidth;
					out2[-11] = ((f32)(cv3 >> 8) - texY) * invHeight;

					facePtr += 4 * (*(i32*)facePtr >> 18);
					pWibData += 16;
					scrollX = scrollX;  // keep
				}
			}
			packet = pNext;
		}
		else
		{
			packet += 4 + 4 * numFaces;
		}
	}
}

static volatile u8 * const gM3dBackgroundFlag = (u8*)0x00550024;
static volatile i32 * const gM3dBackgroundDword = (i32*)0x00660F90;
static u32 ** const gM3dBackgroundClut = (u32**)0x0064F5D0;
static volatile u32 * const * const gM3dBackgroundModelData = (u32* const*)0x005F6764;
static volatile i32 * const gM3dBackgroundSave = (i32*)0x0054D384;
static volatile f32 * const gM3dBackgroundScale = (f32*)0x00550090;
static volatile u8 * const gM3dBackgroundFlagTwo = (u8*)0x00652F3C;
static volatile u8 * const gM3dBackgroundFlagThree = (u8*)0x00660FE2;
static i32 * const gM3dIdentityOne = (i32*)0x0064E518;
static i32 * const gM3dIdentityTwo = (i32*)0x0064E51C;
static i32 * const gM3dIdentityThree = (i32*)0x0064E520;

// @Ok
// (0x004747C0, 1089 bytes). Renders the background models. Walks a linked
// list of background entries (next at +0x20) from the end, and for each
// entry that is usable and in a usable PSX region, builds a rotation matrix
// from the entry's angles, scales it, and renders the model.
void M3d_RenderBackground(void *pList)
{
	if (pList == 0)
		return;

	PCGfx_SetRenderParameter(DCGfx_RenderParameter_4, (DCGfx_RenderSetting)(DCGfx_RenderSetting_e | DCGfx_RenderSetting_1));
	PCGfx_SetRenderParameter(DCGfx_RenderParameter_1, DCGfx_RenderSetting_9);
	PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

	u8 savedFlag = *gM3dBackgroundFlag;
	*gM3dBackgroundFlag = 1;
	*gM3dBackgroundFlagTwo = 1;

	// count the nodes in the linked list
	i32 count = 0;
	void *node = pList;
	while (node != 0)
	{
		node = *(void**)((u8*)node + 0x20);
		count++;
	}

	for (i32 i = count; i > 0; i--)
	{
		if (i >= 11 || *gM3dBackgroundFlagThree == 0)
		{
			// find the i-th node from the start
			void *v4 = pList;
			i32 v5 = i - 1;
			while (v5 != 0)
			{
				v4 = *(void**)((u8*)v4 + 0x20);
				v5--;
			}

			i8 entryFlag = *(i8*)((u8*)v4 + 5);
			u8 region = *(u8*)((u8*)v4 + 0x1F);
			if (entryFlag >= 0 && PSXRegion[region].Usable != 0)
			{
				*gM3dBackgroundDword = -65536;
				*gM3dBackgroundClut = PSXRegion[region].pColourTable;
				u16 modelIndex = *(u16*)((u8*)v4 + 0x1A);
				SModel *pModel = PSXRegion[region].ppModels[modelIndex];
				DCModelData *pModelData = (DCModelData*)(gM3dBackgroundModelData[region] + 36 * modelIndex);
				i16 angleX = *(i16*)((u8*)v4 + 0x14);
				i16 angleY = *(i16*)((u8*)v4 + 0x16);
				i16 angleZ = *(i16*)((u8*)v4 + 0x18);

				matrix4x4 v48;
				if (angleX != 0 || angleY != 0 || angleZ != 0)
				{
					f32 scale = 3.1415927f / 2048.0f;
					f32 z = (f32)angleZ * scale;
					f32 y = (f32)angleY * scale;
					f32 x = (f32)angleX * scale;
					f32 sinx = (f32)sin(x), cosx = (f32)cos(x);
					f32 siny = (f32)sin(y), cosy = (f32)cos(y);
					f32 sinz = (f32)sin(z), cosz = (f32)cos(z);
					f32 c00 = cosz * cosy;
					f32 c01 = sinz * siny;
					f32 c02 = cosz * siny;
					f32 c03 = sinz * cosy;
					f32 c10 = cosy * sinx;
					f32 c11 = -sinx;
					f32 c12 = siny * sinx;
					f32 c20 = c00 * sinx + c01;
					f32 c21 = cosx * sinz;
					f32 c22 = c02 * sinx - c03;
					f32 c23 = c03 * sinx - c02;
					f32 c30 = sinz * sinx;
					f32 c31 = c01 * sinx + c00;
					v48 = matrix4x4(c31, c30, c23, 0, c22, c21, c20, 0, c12, c11, c10, 0, 0, 0, 0, 1.0f);
				}
				else
				{
					// identity matrix from the global tables
					v48 = matrix4x4(
						gM3dIdentityOne[0], gM3dIdentityOne[1], gM3dIdentityOne[2], gM3dIdentityOne[3],
						gM3dIdentityOne[4], gM3dIdentityOne[5], gM3dIdentityOne[6], gM3dIdentityOne[7],
						gM3dIdentityOne[8], gM3dIdentityOne[9], gM3dIdentityOne[10], gM3dIdentityOne[11],
						gM3dIdentityOne[12], gM3dIdentityOne[13], gM3dIdentityOne[14], gM3dIdentityOne[15]);
				}

				f32 s = *gM3dBackgroundScale;
				matrix4x4 v49 = matrix4x4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1.0f);

				matrix4x4 v50;
				gsub_476A00(&v50, &v48, &v49);
				memcpy(&v48, &v50, sizeof(matrix4x4));

				i32 saved = *gM3dBackgroundSave;
				*gM3dBackgroundSave = 0;
				i32 modelFlags = *(i32*)((u8*)pModelData + 0xC);
				if ((modelFlags & 0x100) == 0)
				{
					if ((modelFlags & 0x4000) != 0)
						DC_PSXModel_RenderModel(pModel, &v48, 0, pModelData);
					else
						DCModel_RenderModel(pModel, pModelData, &v48);
				}
				*gM3dBackgroundSave = saved;
			}
		}
	}

	*gM3dBackgroundFlagTwo = 0;
	*gM3dBackgroundFlag = savedFlag;
	PCGfx_SetRenderParameter(DCGfx_RenderParameter_1, DCGfx_RenderSetting_8);
	PCGfx_SetRenderParameter(DCGfx_RenderParameter_4, DCGfx_RenderSetting_e);
	PCGfx_UseTexture(1, DCGfx_BlendingMode_0);
	*gM3dBackgroundFlagThree = 0;
}

// @Ok
// @Test
// can't get it to match but that's fine, looks good tho
void M3d_RenderCleanup(void)
{
	SetDrawArea();
	pPoly += 3;

	stubbed_printf(gRenderBuf);

	if (gWideScreen)
	{
		PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

		f32 v2 = (f32)gGameResolutionY;
		f32 v5 = (f32)(unsigned int)Yres;
		f32 v1 = v2 / v5;
		f32 v6 = (f32)gWideScreen;
		f32 v12 = v1 * v6;
		f32 v7 = (f32)gGameResolutionX;
		f32 v3 = (f32)(unsigned int)Xres;
		f32 v8 = v7 / v3 * 512.0f;
		PCGfx_DrawQuad2D(
				0,
				0,
				v8,
				v12,
				0,
				0,
				1.0,
				1.0,
				0xFF000000,
				0.0,
				false);

		f32 v13 = (f32)gGameResolutionY;
		f32 v9 = (f32)(unsigned int)Yres;
		f32 v4 = v13 / v9;
		f32 v14 = (f32)gWideScreen;
		f32 v18 = v14 * v4;
		f32 v15 = (f32)gGameResolutionX;
		f32 v10 = (f32)(unsigned int)Xres;
		f32 v11 = v15 / v10 * 512.0f;
		f32 v16 = (f32)(240 - gWideScreen);
		f32 v17 = v16 * v4;
		PCGfx_DrawQuad2D(
				0,
				v17,
				v11,
				v18,
				0,
				0,
				1.0,
				1.0,
				0xFF000000,
				0.0,
				false);
	}
}

typedef void (*ConvertMATRIXTomatrix4x4_fn)(MATRIX*, matrix4x4*);
typedef void (*gsub_470610_fn)(u16);
typedef void (*gsub_46D5D0_fn)(i16*, i16*);
typedef void (*gsub_4021D0_fn)(matrix4x4*);

// @Bogus
// @FIXME forward to original: ConvertMATRIXTomatrix4x4_0 (0x402400, 147B)
// converts a GTE MATRIX to a matrix4x4.
static void ConvertMATRIXTomatrix4x4_0(MATRIX *m, matrix4x4 *out) { ConvertMATRIXTomatrix4x4_fn f = (ConvertMATRIXTomatrix4x4_fn)0x00402400; f(m, out); }

// @Bogus
// @FIXME forward to original: sub_470610 (0x470610, 20B), sets a GTE register.
static void gsub_470610(u16 v) { gsub_470610_fn f = (gsub_470610_fn)0x00470610; f(v); }

// @Bogus
// @FIXME forward to original: sub_46D5D0 (0x46D5D0, 69B), sets the GTE
// geometry offset from the camera transform.
static void gsub_46D5D0(i16 *m, i16 *t) { gsub_46D5D0_fn f = (gsub_46D5D0_fn)0x0046D5D0; f(m, t); }

// @Bogus
// @FIXME forward to original: sub_4021D0 (0x4021D0, 545B), normalizes a
// matrix4x4 (called on the camera transform matrix).
static void gsub_4021D0(matrix4x4 *m) { gsub_4021D0_fn f = (gsub_4021D0_fn)0x004021D0; f(m); }

static volatile i32 * const gM3dFadeTimer = (i32*)0x0065CFA4;
static volatile i32 * const gM3dFadeTimerPrev = (i32*)0x00660F88;
static volatile i32 * const gM3dTimerRelated = (i32*)0x006B4CA8;
static volatile i32 * const gM3dFadeFrames = (i32*)0x00660F84;
static volatile i32 * const gM3dFadeCount = (i32*)0x0064E558;
static volatile i32 * const gM3dFadeDist = (i32*)0x0064E568;
static volatile i32 * const gM3dFadeStep = (i32*)0x006191D8;
static volatile i32 * const gM3dFadeNear = (i32*)0x0064E560;
static volatile i32 * const gM3dFadeNearStep = (i32*)0x00628600;
static volatile u32 * const gM3dFadeColour = (u32*)0x00652F38;
static volatile i32 * const gM3dFogFlag = (i32*)0x0054D384;
static volatile i32 * const gM3dCameraPtr = (i32*)0x00628640;
static volatile i32 * const gM3dViewportPtr = (i32*)0x0064E514;
static volatile i32 * const gM3dRenderArg = (i32*)0x00660F68;
static volatile u16 * const gM3dPixelAspectX = (u16*)0x00654F58;
static volatile u16 * const gM3dPixelAspectY = (u16*)0x00654F5C;
static volatile i16 * const gM3dProjMatrix = (i16*)0x0065CEB8;
static volatile u8 * const gM3dRenderFlag = (u8*)0x00660FE8;
static volatile i32 * const gM3dCamOffsetX = (i32*)0x00660730;
static volatile i32 * const gM3dCamOffsetY = (i32*)0x00660734;
static volatile i32 * const gM3dCamOffsetZ = (i32*)0x00660738;
static volatile f32 * const gM3dProjNear = (f32*)0x00550078;
static volatile f32 * const gM3dProjFar = (f32*)0x0055007C;
static volatile f32 * const gM3dProjScale = (f32*)0x00550080;
static volatile f32 * const gM3dProjConst = (f32*)0x00550064;
static volatile u8 * const gM3dObjFileRegion = (u8*)0x006B3824;
static volatile u8 * const gM3dRegionTwo = (u8*)0x006B4678;
static volatile i32 * const gM3dCamFocusX = (i32*)0x005FCDA8;

// @Ok
// (0x00472DC0, 2605 bytes). Sets up the 3D render for a frame: advances the
// fade timer, sets fog params, computes the viewport projection matrix from
// the SViewport (xL/xR/yT/yB/Zoom/Hither/Yon), sets the GTE rotation and
// geometry offset from the camera, preprocesses pulsing colours and wibble
// textures, builds the final projection matrix, and calls PCGfx_RenderInit.
void M3d_RenderSetup(SCamera *pCam, SViewport *pView, u32 *a3)
{
	*gM3dFadeTimerPrev = *gM3dFadeTimer;
	bool timerChanged = (*gM3dFadeTimer == *gM3dTimerRelated);
	*gM3dFadeTimer = *gM3dTimerRelated;
	if (!timerChanged)
		(*gM3dFadeFrames)++;

	if (*gM3dFadeCount > 0)
	{
		(*gM3dFadeDist) += *gM3dFadeStep;
		(*gM3dFadeCount)--;
		(*gM3dFadeNear) += *gM3dFadeNearStep;
		u32 c = *gM3dFadeColour;
		u32 v54 = ((u8)(c >> 16) << 16) + ((c >> 8) & 0xFF) + (c & 0xFF00FF00);
		if ((v54 & 0xFFFFFF) == 0xFFFFFF)
		{
			*gM3dFogFlag = 1;
			f32 nearF = (f32)(*gM3dFadeNear) * 100.0f;
			f32 farF = (f32)(*gM3dFadeDist) * 100.0f;
			PCGfx_SetFogParams(farF, nearF, v54);
		}
		else
		{
			*gM3dFogFlag = 0;
			f32 nearF = (f32)(*gM3dFadeNear) * 0.98f;
			f32 farF = (f32)(*gM3dFadeDist) * 0.98f;
			PCGfx_SetFogParams(farF, nearF, v54);
		}
	}

	*gM3dCameraPtr = (i32)pCam;
	*gM3dViewportPtr = (i32)pView;
	*(u16*)((char*)pView + 0x0A) = (u16)(*gM3dFadeNear);
	*gM3dRenderArg = (i32)a3;

	u32 *v5 = pPoly;
	if (gPrintStubbed == 0)
		stubbed_printf((char*)"stubbed out: SetDrawArea");
	pPoly = v5 + 12;
	if (gPrintStubbed == 0)
		stubbed_printf((char*)gRenderBuf);

	u16 xL = *(u16*)((char*)pView + 0x00);
	u16 yB = *(u16*)((char*)pView + 0x02);
	u16 xR = *(u16*)((char*)pView + 0x04);
	u16 yT = *(u16*)((char*)pView + 0x06);
	u16 vpHither = *(u16*)((char*)pView + 0x08);
	u16 vpYon = *(u16*)((char*)pView + 0x0A);
	u16 zoom = *(u16*)((char*)pView + 0x0C);
	i32 v7 = (((u32)xR - xL) << 11) & 0xFFFFF000;
	i32 v8 = xL + xR;
	v7 = (v7 & 0xFFFF0000) | ((((v7 / zoom) << 12) / *gM3dPixelAspectY) & 0xFFFF);
	u16 fieldE = (u16)v7;
	*(u16*)((char*)pView + 0x10) = (u16)(v8 >> 1);
	*(u16*)((char*)pView + 0x0E) = fieldE;
	*(u16*)((char*)pView + 0x12) = (u16)((yB + yT) >> 1);

	volatile i16 *pm = gM3dProjMatrix;
	pm[0] = 0;
	pm[1] = 0;
	pm[2] = -4096;
	pm[4] = 0;
	pm[3] = vpYon;
	pm[6] = 0;
	pm[5] = 4096;
	pm[7] = -vpHither;

	i32 v11 = *gM3dPixelAspectX * fieldE;
	i32 v12 = ((u32)xR + 0x1FFFFF * xL) << 11;
	i32 v13 = M3dMaths_SquareRoot0((v11 >> 12) * (v11 >> 12) + (v12 >> 12) * (v12 >> 12));
	pm[8] = 0;
	pm[10] = 0;
	pm[12] = 0;
	pm[9] = v11 / v13;
	pm[11] = -pm[9];
	pm[13] = v12 / v13;
	pm[14] = v12 / v13;
	pm[15] = 0;

	i32 v14 = *gM3dPixelAspectY * fieldE;
	i32 v15 = ((u32)xR + 0x1FFFFF * xL) << 11;
	i32 v16 = M3dMaths_SquareRoot0((v14 >> 12) * (v14 >> 12) + (v15 >> 12) * (v15 >> 12));
	*(i32*)(pm + 16) = v14 / v16;
	*(i32*)(pm + 18) = v15 / v16;
	*(i32*)(pm + 22) = v15 / v16;
	*(i32*)(pm + 20) = -(i16)(v14 / v16);

	v14 = (v14 & 0xFFFF0000) | (fieldE & 0xFFFF);
	i32 v17 = (yB - yT) >> 1;
	i32 v18 = M3dMaths_SquareRoot0(v17 * v17 + v14 * v14);
	pm[24] = 0;
	pm[28] = 0;
	pm[30] = 0;
	pm[34] = 0;
	pm[25] = (v14 << 12) / v18;
	pm[27] = -pm[25];
	pm[29] = (v17 << 12) / v18;
	pm[32] = (v17 << 12) / v18;

	i32 v19 = (xR - xL) >> 1;
	v14 = (v14 & 0xFFFF0000) | (fieldE & 0xFFFF);
	i32 v20 = M3dMaths_SquareRoot0(v19 * v19 + v14 * v14);
	*(i32*)(pm + 36) = (v14 << 12) / v20;
	pm[39] = 0;
	pm[42] = 0;
	pm[44] = 0;
	pm[37] = -(i16)((v14 << 12) / v20);
	pm[41] = (v19 << 12) / v20;
	pm[46] = (v19 << 12) / v20;

	gte_SetRotMatrix(&pCam->Transform);
	SVECTOR *v21 = (SVECTOR*)(pm + 24);
	i32 v70 = 0;
	while ((int)v21 < (int)(pm + 32))
	{
		gte_ldv0(v21 - 3);
		gte_rtv0();
		gte_stsv((SVECTOR*)((char*)0x00628648 + v70));
		gte_ldv0(v21);
		gte_rtv0();
		gte_stsv((SVECTOR*)((char*)0x00628620 + v70));
		v21++;
		v70 += 6;
	}
	gsub_470610(fieldE);
	if (gPrintStubbed == 0)
		stubbed_printf((char*)"stubbed out: SetGeomOffset");
	gsub_46D5D0((i16*)pCam->Transform.m, (i16*)((char*)pCam + sizeof(SCamera)));

	i16 *p_pad = (i16*)((char*)pCam + sizeof(SCamera) + 0x20);
	for (i32 i = 3; i != 0; --i)
	{
		*p_pad = (*gM3dPixelAspectY * *(i16*)((char*)p_pad - 32)) >> 12;
		*(i16*)((char*)p_pad + 6) = (*gM3dPixelAspectX * *(i16*)((char*)p_pad - 26)) >> 12;
		*(i16*)((char*)p_pad + 12) = *(i16*)((char*)p_pad - 20);
		p_pad = (i16*)((char*)p_pad + 2);
	}

	M3d_PreprocessPulsingColours(EnvRegions[0]);
	M3d_PreprocessPulsingColours(EnvRegions[1]);
	M3d_PreprocessPulsingColours(*gM3dObjFileRegion);
	M3d_PreprocessPulsingColours(*gM3dRegionTwo);
	M3d_PreprocessWibblyTextures(*gM3dObjFileRegion);

	static volatile i16 * const gM3dGeomOffX = (i16*)0x00628608;
	static volatile i16 * const gM3dGeomOffY = (i16*)0x0062860A;
	static volatile i16 * const gM3dGeomOffZ = (i16*)0x0062860C;
	static volatile i16 * const gM3dGeomOffW = (i16*)0x0062860E;
	*gM3dGeomOffX = -*(i16*)((char*)*gM3dCameraPtr + 58);
	*gM3dGeomOffY = -*(i16*)((char*)*gM3dCameraPtr + 60);
	*gM3dGeomOffZ = -*(i16*)((char*)*gM3dCameraPtr + 62);
	*gM3dGeomOffW = (*gM3dCamFocusX >> 12) - *(i16*)((char*)*gM3dCameraPtr + 8);

	PCGfx_UseTexture(-1, DCGfx_BlendingMode_0);
	PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

	if (*gM3dRenderFlag != 0)
	{
		pCam->Transform.t[0] = pCam->Position.vx;
		pCam->Transform.t[1] = pCam->Position.vy;
		pCam->Transform.t[2] = pCam->Position.vz;
	}
	pCam->Transform.t[0] += *gM3dCamOffsetX;
	pCam->Transform.t[1] = *gM3dCamOffsetY + pCam->Transform.t[1];
	pCam->Transform.t[2] = *gM3dCamOffsetZ + pCam->Transform.t[2];

	matrix4x4 camMatrix;
	ConvertMATRIXTomatrix4x4_0(&pCam->Transform, &camMatrix);
	matrix4x4 stru_56E778;
	memcpy(&stru_56E778, (void*)0x0056E778, sizeof(matrix4x4));
	gsub_4021D0(&stru_56E778);

	f32 a1a = (f32)gGameResolutionX / (f32)Xres;
	f32 a1b = (f32)gGameResolutionY / (f32)Yres;
	f32 left = (f32)xL * a1a;
	f32 right = (f32)(xR - xL) * a1a;
	f32 top = (f32)yT * a1b;
	f32 bottom = (f32)(yB - yT) * a1b;
	f32 yon = (f32)vpYon;
	f32 hither = (f32)vpHither;
	f32 invRange = yon / (yon - hither);
	f32 zoomF = (f32)zoom;
	f32 v91 = *gM3dProjConst * 4096.0f / zoomF;
	f32 v94 = -(invRange * hither);
	f32 v83 = right * 0.5f;
	f32 v72 = bottom * 0.5f;
	f32 v76 = left + v83;
	f32 v92 = top + v72;
	f32 v84 = v83 * v91;

	matrix4x4 v95 = matrix4x4(v84, 0, 0, 0, 0, v84, 0, 0, v76, v92, invRange, 1, 0, 0, v94, 0);
	vector4d v93[4];
	for (i32 j = 0; j < 4; j++)
		v93[j] = v95.field_0[j];

	matrix4x4 stru_56E570;
	memcpy(&stru_56E570, (void*)0x0056E570, sizeof(matrix4x4));
	f32 *src = (f32*)v93;
	f32 *dst = &stru_56E570.field_0[0].field_0[0];
	// copy the 16 floats from v93 into stru_56E570, with the first row replaced
	for (i32 r = 0; r < 4; r++)
	{
		for (i32 c = 0; c < 4; c++)
		{
			if (r == 0)
				dst[r*4+c] = (c == 0) ? v92 : src[r*4+c];
			else
				dst[r*4+c] = src[r*4+c];
		}
	}

	PCGfx_RenderInit(hither, yon, (f32)fieldE);

	static matrix4x4 * const gM3dFinalProjMatrix = (matrix4x4*)0x0056E6F8;
	gsub_476A00(gM3dFinalProjMatrix, &v95, &stru_56E778);

	i32 result = *gM3dFogFlag;
	if (*gM3dFogFlag != 0)
	{
		i32 v45 = *gM3dFadeDist;
		if (v45 < 10)
		{
			v45 = 10;
			*gM3dFadeDist = 10;
		}
		result = *gM3dFadeNear;
		if (*gM3dFadeNear < 15)
		{
			result = 15;
			*gM3dFadeNear = 15;
		}
		if (result <= v45)
			*gM3dFadeNear = v45 + 5;
		*gM3dProjNear = 1.0f / (f32)(*gM3dFadeNear);
		*gM3dProjFar = 1.0f / (f32)(*gM3dFadeDist);
		*gM3dProjScale = 255.0f / (*gM3dProjFar - *gM3dProjNear);
	}
	(void)result;
}

// @MEDIUMTODO
void RenderSuperItem(CItem *,bool)
{
    printf("RenderSuperItem(CItem *,bool)");
}

void validate_matrix4x4(void)
{
	VALIDATE_SIZE(matrix4x4, 64);

	VALIDATE(matrix4x4, field_0, 0x0);
}

#include "my_patch.h"

// @Bogus
void patch_ps2m3d(void)
{
	PATCH_PUSH_RET(0x00475F50, M3d_BuildTransform);
}
