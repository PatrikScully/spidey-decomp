#include "ps2m3d.h"
#include "ps2funcs.h"
#include "db.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "spool.h"

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

// @NotOk
// Not one of the file's original 7 stubs. Added because M3d_RenderSetup,
// M3d_Render and RenderSuperItem all call it (leaf-first dependency) to
// concatenate transforms; found via the maintainer's IDB (spideypc_names.txt
// calls it matrix4x4_ml), named gsub_476A00 here since tools/names.json only
// has it as sub_476A00.
// dest is written through a local first because dest may alias a or b (e.g.
// gsub_476A00(&m, &m, &n) to do "m = m * n" in place); writing straight into
// *dest while still reading it back for later cells would corrupt the
// result.
// Residue: every fld/fmul/faddp in the whole function matches the original
// exactly in type, operand and order (all 16 dot products, including the
// odd per-cell term ordering and the swapped-operand cases). The only
// mismatch is register allocation at the very top: the original pushes 3
// callee-saved regs (ebx,esi,edi) and uses a 0x80-byte frame; this build
// pushes 4 (ebx,ebp,esi,edi) and uses a 0x40-byte frame. That one insertion
// shifts every stack offset after it, which is why cmpsum reports 251
// mnemonic diffs despite the logic being right. Hypotheses tried (3, well
// under the 15-hypothesis medium-function bar, so this stays @NotOk, not
// @AlmostMatching):
// 1. matrix4x4 operator*(a,b) returning by value via "return matrix4x4(16
//    args)", relying on the existing 16-arg ctor. Produces real out-of-line
//    calls to the ctor (3x) instead of inlined stores; original has no
//    calls at all. Rejected.
// 2. Plain function, explicit dest pointer, named "matrix4x4 result;" local
//    (needs the new empty default ctor) filled in field-by-field, then
//    "*dest = result;". Matches the arithmetic core exactly; wrong register
//    count/frame size (this version, currently in tree).
// 3. Same as 2 but returning matrix4x4 by value (hidden return slot, a/b by
//    pointer, no explicit dest param) instead of an explicit out-pointer.
//    Went the other way on register count (2 instead of 3) and increased
//    the diff count to 267. Rejected, reverted to hypothesis 2.
EXPORT void gsub_476A00(matrix4x4* dest, matrix4x4 const* a, matrix4x4 const* b)
{
	matrix4x4 result;

	result.field_0[3].field_0[3] = a->field_0[3].field_0[2] * b->field_0[2].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[3] + a->field_0[3].field_0[0] * b->field_0[0].field_0[3] + b->field_0[3].field_0[3] * a->field_0[3].field_0[3];
	result.field_0[3].field_0[2] = b->field_0[3].field_0[2] * a->field_0[3].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[2] + a->field_0[3].field_0[2] * b->field_0[2].field_0[2] + a->field_0[3].field_0[0] * b->field_0[0].field_0[2];
	result.field_0[3].field_0[1] = a->field_0[3].field_0[2] * b->field_0[2].field_0[1] + a->field_0[3].field_0[0] * b->field_0[0].field_0[1] + b->field_0[3].field_0[1] * a->field_0[3].field_0[3] + a->field_0[3].field_0[1] * b->field_0[1].field_0[1];
	result.field_0[3].field_0[0] = a->field_0[3].field_0[1] * b->field_0[1].field_0[0] + b->field_0[3].field_0[0] * a->field_0[3].field_0[3] + a->field_0[3].field_0[2] * b->field_0[2].field_0[0] + a->field_0[3].field_0[0] * b->field_0[0].field_0[0];
	result.field_0[2].field_0[3] = a->field_0[2].field_0[2] * b->field_0[2].field_0[3] + a->field_0[2].field_0[3] * b->field_0[3].field_0[3] + a->field_0[2].field_0[0] * b->field_0[0].field_0[3] + a->field_0[2].field_0[1] * b->field_0[1].field_0[3];
	result.field_0[2].field_0[2] = a->field_0[2].field_0[0] * b->field_0[0].field_0[2] + a->field_0[2].field_0[3] * b->field_0[3].field_0[2] + a->field_0[2].field_0[2] * b->field_0[2].field_0[2] + a->field_0[2].field_0[1] * b->field_0[1].field_0[2];
	result.field_0[2].field_0[1] = a->field_0[2].field_0[3] * b->field_0[3].field_0[1] + a->field_0[2].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[2].field_0[1] + a->field_0[2].field_0[0] * b->field_0[0].field_0[1];
	result.field_0[2].field_0[0] = b->field_0[1].field_0[0] * a->field_0[2].field_0[1] + a->field_0[2].field_0[0] * b->field_0[0].field_0[0] + a->field_0[2].field_0[3] * b->field_0[3].field_0[0] + a->field_0[2].field_0[2] * b->field_0[2].field_0[0];
	result.field_0[1].field_0[3] = a->field_0[1].field_0[2] * b->field_0[2].field_0[3] + a->field_0[1].field_0[3] * b->field_0[3].field_0[3] + a->field_0[1].field_0[0] * b->field_0[0].field_0[3] + a->field_0[1].field_0[1] * b->field_0[1].field_0[3];
	result.field_0[1].field_0[2] = a->field_0[1].field_0[0] * b->field_0[0].field_0[2] + b->field_0[3].field_0[2] * a->field_0[1].field_0[3] + a->field_0[1].field_0[2] * b->field_0[2].field_0[2] + a->field_0[1].field_0[1] * b->field_0[1].field_0[2];
	result.field_0[1].field_0[1] = a->field_0[1].field_0[3] * b->field_0[3].field_0[1] + a->field_0[1].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[1].field_0[1] + a->field_0[1].field_0[0] * b->field_0[0].field_0[1];
	result.field_0[1].field_0[0] = b->field_0[1].field_0[0] * a->field_0[1].field_0[1] + a->field_0[1].field_0[0] * b->field_0[0].field_0[0] + a->field_0[1].field_0[3] * b->field_0[3].field_0[0] + a->field_0[1].field_0[2] * b->field_0[2].field_0[0];
	result.field_0[0].field_0[3] = a->field_0[0].field_0[2] * b->field_0[2].field_0[3] + a->field_0[0].field_0[3] * b->field_0[3].field_0[3] + a->field_0[0].field_0[0] * b->field_0[0].field_0[3] + b->field_0[1].field_0[3] * a->field_0[0].field_0[1];
	result.field_0[0].field_0[2] = b->field_0[0].field_0[2] * a->field_0[0].field_0[0] + b->field_0[3].field_0[2] * a->field_0[0].field_0[3] + b->field_0[2].field_0[2] * a->field_0[0].field_0[2] + b->field_0[1].field_0[2] * a->field_0[0].field_0[1];
	result.field_0[0].field_0[1] = a->field_0[0].field_0[3] * b->field_0[3].field_0[1] + a->field_0[0].field_0[2] * b->field_0[2].field_0[1] + b->field_0[1].field_0[1] * a->field_0[0].field_0[1] + a->field_0[0].field_0[0] * b->field_0[0].field_0[1];
	result.field_0[0].field_0[0] = b->field_0[1].field_0[0] * a->field_0[0].field_0[1] + a->field_0[0].field_0[0] * b->field_0[0].field_0[0] + a->field_0[0].field_0[3] * b->field_0[3].field_0[0] + a->field_0[0].field_0[2] * b->field_0[2].field_0[0];

	*dest = result;
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
void DCModel_RenderModel(SModel const *,DCModelData *,matrix4x4 const *,void const *)
{
    printf("DCModel_RenderModel(SModel const *,DCModelData *,matrix4x4 const *,void const *)");
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

// @NotOk
// NOT AlmostMatching: verified the built function is 11 bytes longer than the
// original (469 vs 458, 147 vs 151 decoded instructions) per CLAUDE.md's
// "verify byte length" rule, so this is a real code-shape gap, not pure
// scheduling residue, even though the cause is well understood. 118
// mnemonic diffs / 21 hypotheses tried (13 on the real build, 8 in an
// isolated MSVC6 sandbox). Every instruction after the loop entry point is
// mnemonic-identical to the original, just shifted by exactly one slot: the
// original pushes ebx/esi/edi in the prologue and defers "push ebp" (and
// the matching pop) to the point where the per-record loop is actually
// entered, so the two early-return paths (Region==-1, !pData) never touch
// ebp at all. This build always pushes all 4 callee-saved registers in the
// prologue, so every early return carries one extra unneeded pop ebp.
// Every attempt to convince MSVC6 to defer the ebp save (do/while loop
// shape, array vs scalar rgb temps, caching vs re-reading ListLen, if/else
// vs ternary for dt, explicit SPSXRegion* pointer caching) left the diff
// count and shift unchanged. Needs a source shape where ebp is genuinely
// unused on both early-return paths.
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

// @MEDIUMTODO
void M3d_PreprocessWibblyTextures(i32)
{
    printf("M3d_PreprocessWibblyTextures(i32)");
}

// @MEDIUMTODO
void M3d_RenderBackground(void *)
{
    printf("M3d_RenderBackground(void *)");
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

// @MEDIUMTODO
void M3d_RenderSetup(SCamera *,SViewport *,u32 *)
{
    printf("M3d_RenderSetup(SCamera *,SViewport *,u32 *)");
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
