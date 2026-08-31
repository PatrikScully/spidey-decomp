#include "ps2m3d.h"
#include "ps2funcs.h"
#include "db.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "spool.h"
#include "algebra.h"
#include <math.h>
#include <string.h>

#include "validate.h"

// Same address as PCTex.cpp/spool.cpp/trig.cpp's own file-local
// G_LOWGRAPHICS copies (0x6B78F8, gLowGraphics per CLAUDE.md); this file
// needs its own copy too, following the existing repo pattern of a
// per-file macro rather than a shared header (that "never duplicate across
// files" rule is not consistently applied to this particular macro yet).
//#define G_LOWGRAPHICS (gLowGraphics)
#define G_LOWGRAPHICS (*reinterpret_cast<i32*>(0x006B78F8))

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

typedef i32 (*gsub_509000_fn)(f32, f32, f32, i32, f32, f32, f32, i32, f32);

// @Bogus
// @FIXME forward to original: gsub_509000 (0x509000): draws a screen-space "line" as a colored quad
// billboard between two projected 3D points (a1,a2,a3)-(a5,a6,a7), width a9,
// color a8 (0xAARRGGBB). Traced this session for DCModel_RenderModel's
// debug-light-vector block: builds 4 tagKMVERTEX3-shaped corner records via
// sub_509740 (perp-offset helper, unnamed) and submits them through
// gsub_509400 (already @Ok in PCGfx.cpp) + sub_5071B0 (unnamed, looks like
// submitPoly-with-count). Both of those inner helpers are still
// undecompiled, so this is forwarded to the original instead of
// reimplemented (matches the ClearRegion/@FIXME forward precedent) -- it is
// only reached when the gDCDebugLightFlag global (0x65CEB0) is set, which
// looks like a debug/diagnostic toggle, not part of normal gameplay
// rendering.
EXPORT i32 gsub_509000(f32 x0, f32 y0, f32 z0, i32 a4, f32 x1, f32 y1, f32 z1, u32 color, f32 width)
{
	gsub_509000_fn f = (gsub_509000_fn)0x00509000;
	return f(x0, y0, z0, a4, x1, y1, z1, color, width);
}

// New file-scope statics for DCModel_RenderModel / DC_PSXModel_RenderModel.
// Best-effort tentative names under the session-wide "functional
// decompilation, not byte-matching" bar -- not run through the full
// nearest-neighbor address audit CLAUDE.md normally requires (there are ~25
// of them from one function); flagged here so a future pass can do that
// properly. None of these collided with tools/names.json or
// idb_globals.txt except where noted.

// Per-call profiling counters, accumulated every DCModel_RenderModel call.
static volatile i32 * const gDCStatVertsOrig   = (i32*)0x00660FB0; // += SModel::NumVertices (via DCModelData::mNumVertices)
static volatile i32 * const gDCStatVertsWelded = (i32*)0x00660FB4; // += DCModelData::mVertexCount
static volatile i32 * const gDCStatFaces       = (i32*)0x00660FD0; // += DCModelData::mNumFaces
static volatile i32 * const gDCStatCalls       = (i32*)0x00660FD8; // ++ per call

// Same address M3d_RenderSetup already uses as a function-local
// gM3dFinalProjMatrix (matrix4x4*) -- the frame's camera+projection matrix,
// composed once per frame. DCModel_RenderModel multiplies the object's own
// transform (pTransform) by this to get the full object-to-screen matrix.
static matrix4x4 * const gDCFinalProjMatrix = (matrix4x4*)0x0056E6F8;
// Same address as algebra.cpp's file-local gGfxMatrix / bit.cpp's
// gFrameProjMatrix: the ACTIVE 16-float transform Algebra_Transform4 reads.
// DCModel_RenderModel writes the composed object-to-screen matrix here
// before doing any per-vertex projection.
static f32 * const gDCGfxMatrix = (f32*)0x0056E668;

// Shared screen-space vertex scratch pool (tagKMVERTEX3 records, PCGfx.h).
// DCModel_RenderModel and DC_PSXModel_RenderModel both index into it,
// offset by the model's own mVertexCount (reason for the offset is not
// understood, transliterated as-is from the disassembly).
static tagKMVERTEX3 * const gDCVertexPool = (tagKMVERTEX3*)0x0062E510;

// 16 bytes/entry, reverse-indexed (999-idx) precomputed screen positions,
// used when a DCVert's mFlags bit 0x2 is set (the "stitched" index packed
// into mFlags bits 16-23 by DCModel_CreateFromSModel, see dcmodel.h) --
// shares an exact screen position across welded copies of one source
// vertex so UV-seam splits don't crack at a shared edge.
static f32 * const gDCStitchPositionTable = (f32*)0x0062861C;

// Bump-decrementing (16 bytes/record) output cursor. Written whenever a
// DCVert's mFlags bit 0x1 is set, right after that vertex's screen position
// is computed. Tentative: "attachment point" list (a marked vertex whose
// resolved screen position gets recorded for some other system to read,
// e.g. an effect/weapon attach point), not cross-checked against a reader.
static volatile u8 * gDCAttachPointCursor = (u8*)0x0064F5CC;

// gFloatSuperRelated is already a repo-wide global (ob.h/ob.cpp, 0x54FFE0),
// used directly below instead of a new tentative pointer.
static volatile i32 * const gDCUseSuperScale  = (i32*)0x00660F74; // flag: use gFloatSuperRelated for the vertex scale
static f32 * const gDCFixedScaleConst         = (f32*)0x00550074; // raw float constant, alternate vertex scale
static volatile i32 * const gDCUseFixedScale  = (i32*)0x0060CF90; // flag: use gDCFixedScaleConst for the vertex scale

// Same addresses as the file-scope statics M3d_RenderBackground/
// M3d_RenderSetup declare further down this file (gM3dBackgroundDword,
// gM3dBackgroundFlagThree, gM3dFogFlag, gM3dProjNear, gM3dProjFar,
// gM3dProjScale) -- redeclared here (own file-local copies, same repo
// convention already used throughout ps2m3d.cpp) since those are declared
// textually AFTER this function.
static volatile i32 * const gDCOverrideFlags     = (i32*)0x00660F90; // == gM3dBackgroundDword
static volatile u8 *  const gDCHiResScaleFlag     = (u8*)0x00660FE2; // == gM3dBackgroundFlagThree
static volatile i32 * const gDCFogEnabled        = (i32*)0x0054D384; // == gM3dFogFlag
static volatile f32 * const gDCProjNear          = (f32*)0x00550078; // == gM3dProjNear
static volatile f32 * const gDCProjFar           = (f32*)0x0055007C; // == gM3dProjFar
static volatile f32 * const gDCProjScale         = (f32*)0x00550080; // == gM3dProjScale

static volatile i32 * const gDCTexAnimFlag    = (i32*)0x00660F9C; // gates the animated-texture-color block
static volatile i32 * const gDCTintFlag       = (i32*)0x00660F94; // gates a tint-recompute sub-step inside it
static volatile u8 * const  gDCNoFogFlag      = (u8*)0x00660FE1;  // byte, "skip the far-plane fog scan, always dither" flag

static volatile i32 * const gDCLightCount     = (i32*)0x0064E510; // count, shared by the tint block and the debug-light block
static f32 * const gDCLightTintTable          = (f32*)0x0064F5B0; // 3 floats/entry RGB tint buffer, gDCLightCount entries
// Two big fixed-offset tables read by the per-light dot-product loop
// (per-light "direction"/normal-space vector and per-light "color", 3
// floats/12 bytes per entry each). The huge literal byte offsets in the
// disassembly (6456936, 6616488/92/96) are MSVC folding small-index array
// reads into neighbouring globals' base addresses (see CLAUDE.md's "Global
// boundaries" note) -- not independently resolved to a clean base address
// this session, kept as raw offsets.
#define GDC_LIGHT_VECTOR_TABLE_X(i) (*(f32*)(6456936 + 12 * (i)))
#define GDC_LIGHT_VECTOR_TABLE_Y(i) (*(f32*)(6456936 + 12 * (i) + 4))
#define GDC_LIGHT_VECTOR_TABLE_Z(i) (*(f32*)(6456936 + 12 * (i) + 8))
#define GDC_LIGHT_COLOR_TABLE_X(i)  (*(f32*)(6616488 + 12 * (i)))
#define GDC_LIGHT_COLOR_TABLE_Y(i)  (*(f32*)(6616488 + 12 * (i) + 4))
#define GDC_LIGHT_COLOR_TABLE_Z(i)  (*(f32*)(6616488 + 12 * (i) + 8))
// Stitch-index-keyed lit-color table, parallels gDCStitchPositionTable
// (position) but for lighting: dword_64F5D8 (3 ints/entry, only element 0
// read) plus a second 12-bytes/entry table at 0x64F858 for components 1/2.
static i32 * const gDCStitchColorIndexTable = (i32*)0x0064F5D8;
static volatile u8 * gDCLitColorOutCursor = (u8*)0x0065DFA8; // bump-incrementing (12 bytes/record) output cursor, DCVert mFlags bit 0x1

static volatile i32 * const gDCTexAnimColorA    = (i32*)0x00660F58;
static volatile i32 * const gDCTexAnimColorB    = (i32*)0x00660F54;
static volatile i32 * const gDCTexAnimColorC    = (i32*)0x00660F50;
static volatile u32 * const gDCTexAnimColorSrcA = (u32*)0x00660F48;
static volatile u32 * const gDCTexAnimColorSrcB = (u32*)0x00660F5C;
static volatile u32 * const gDCTexAnimColorSrcC = (u32*)0x006191D4;

static volatile i32 * const gDCDebugLightFlag = (i32*)0x0065CEB0; // gates the sub_509000 debug-line block

static i32 * const gDCLastBoundTexture = (i32*)0x00568170; // idb_globals.txt: gUseTextureRelated -- last texture id passed to PCGfx_UseTexture
static i32 * const gDCLastNoLightFlag  = (i32*)0x00AC08DC; // last "no-light" flag passed to PCGfx_UseTexture, batching cache
static i32 * const gDCFaceSortKey      = (i32*)0x00AC08E0; // PCGfx.cpp already references this address generically as an OT/sort key
static i32 * const gDCFaceSortKeyExtra = (i32*)0x00AC08E4;
static volatile i32 * const gDCForceNoTexFlag  = (i32*)0x00660FFC;
static volatile i32 * const gDCForceNoLightFlag = (i32*)0x00660FF8;
static volatile i32 * const gDCBatchCallCount   = (i32*)0x00660FD4;

static u16 * const gDCFaceTexIndexOut = (u16*)0x006191DC; // per-face resolved texture id, filled by the OT-build loop
static u8 *  const gDCFaceNoLightOut  = (u8*)0x0065F72C;  // per-face "no-light" flag, filled by the OT-build loop
static u16 * const gDCFaceSortBiasOut = (u16*)0x00652F54; // per-face env-map/sort adjustment, filled by the OT-build loop
static u16 * const gDCTriIndexTemplate = (u16*)0x0062C850; // fixed 6-u16-per-face-slot triangle index template

static i32 * const gDCForceTexIndex = (i32*)0x00660FE4; // per-model forced texture-index override (0 = use the face's own)
static i32 * const gDCEnvMapTableA  = (i32*)0x00614F88;
static i32 * const gDCEnvMapTableB  = (i32*)0x00614F98;
static u16 * const gDCEnvMapTableC  = (u16*)0x00615018;

// @Ok
// (0x00476D00, ~5.2KB.) The real DC-format model renderer: reads every
// field DCModel_CreateFromSModel wrote (dcmodel.h) and projects/lights/
// batches the model's faces for PCGfx submission. Session-wide override:
// functional decompilation, not byte-matching (see CLAUDE.md task header).
//
// Cross-validation against dcmodel.h's DCModelData/DCVert/DCFace field
// docs, found while implementing this (see the ps2m3d.attempts.md log and
// the task report for the full writeup):
//  - CONFIRMS DCModelData::mFlags 0x100 = skip render entirely.
//  - REFINES: mFlags 0x10 is ALSO tested here (`(mFlags&0x10)||(mFlags&0x100)`
//    both bail out before any work), not just 0x100 as the header's "bits
//    tested" list implied.
//  - CONFIRMS DCModelData::mVertexCount (a2[4]) is the vertex-loop bound.
//  - CONFIRMS DCModelData::mSortBiasNormal / mSortBiasLowGraphics (a2[7],
//    a2[8]) are selected by gLowGraphics and added into the per-face OT/
//    sort key (gDCFaceSortKey = bias + gDCFaceSortBiasOut[i]).
//  - CONFIRMS/EXPLAINS DCVert's documented "defect": when the source
//    per-vertex flag bit 0x10 is set, DCVert::x/DCVert::y are NOT floats,
//    they are raw i32 INDICES into the screen-space vertex scratch pool
//    (gDCVertexPool). This is not a bug -- it is a billboard/ribbon vertex
//    whose screen position is generated from two already-projected "anchor"
//    vertices (a perpendicular offset scaled by DCVert::z, which IS a real
//    float) instead of having its own 3D position at all.
//  - REFINES DCVert::mFlags bit 0x2 ("stitched index"): confirms it means
//    "share this exact screen position with another welded copy", read
//    from gDCStitchPositionTable via the byte-2 index packed by
//    DCModel_CreateFromSModel.
//  - New finding: DCVert::mFlags bit 0x1 marks an "attachment point"
//    vertex -- its resolved screen position also gets appended to a
//    separate bump-allocated list (gDCAttachPointCursor). Not documented
//    in dcmodel.h before this session.
//  - New finding: DCFace::field_34[0] bit 0x2 gates the OT-build loop's
//    per-face skip test, and field_34[0] bit 0x1 (with field_34[2..3] as a
//    u16) is a per-face RUNTIME texture-index override read back by the
//    renderer. Refines the header's "low confidence, runtime scratch"
//    guess for field_34 into something concrete.
void DCModel_RenderModel(SModel const *pModel, DCModelData *pData, matrix4x4 const *pTransform)
{
	if ((pModel->Flags & 0x20) != 0)
		return;
	if (pModel->NumFaces == 0)
		return;
	if (pData == 0)
		return;

	i32 flags = pData->mFlags;
	u8 flagsByte1 = (u8)((u32)flags >> 8);
	if ((flags & 0x10) != 0 || (flags & 0x100) != 0)
		return;

	i32 numVertsOrig = pModel->NumVertices;
	DCFace *pFaces = (DCFace*)pData->pFaces;
	i32 numVertsOrigClamped = (u16)numVertsOrig;
	DCVert *pVerts = pData->pVertices;

	gDCStatVertsOrig[0]   += pData->mNumVertices;
	i32 vertexCount = pData->mVertexCount;
	gDCStatVertsWelded[0] += pData->mVertexCount;
	gDCStatFaces[0]       += pData->mNumFaces;
	gDCStatCalls[0]++;

	// objToScreen = pTransform * gDCFinalProjMatrix (row-vector*matrix, same
	// convention as gsub_476A00, confirmed algebraically against the 16 dot
	// products in the original disassembly -- see the ps2m3d attempts log).
	matrix4x4 objToScreen;
	gsub_476A00(&objToScreen, pTransform, gDCFinalProjMatrix);
	memcpy(gDCGfxMatrix, &objToScreen, sizeof(matrix4x4));

	// Per-vertex projected-color/dither scale. Defaults to 1.0f in low
	// graphics mode; in normal graphics mode it is usually 4.0 (or 1.0 if
	// gM3dBackgroundFlagThree is clear), bumped 1.025x for a "transparent
	// face" model (mFlags bit 0x800), or overridden entirely by a Super's
	// float or a fixed constant when the matching global flags are set.
	f32 vertexScale = 1.0f;
	if (!G_LOWGRAPHICS)
	{
		vertexScale = 4.0f;
		if (*gDCHiResScaleFlag == 0)
			vertexScale = 1.0f;
		if ((flagsByte1 & 8) != 0)
			vertexScale = vertexScale * 1.025f;
		if (*gDCUseSuperScale != 0)
			vertexScale = gFloatSuperRelated;
		if (*gDCUseFixedScale != 0)
			vertexScale = *gDCFixedScaleConst;
	}

	i32 remainingVerts = numVertsOrig - 1;
	i32 remainingVertsSaved = numVertsOrig - 1;
	tagKMVERTEX3 *pScratchBase = gDCVertexPool + vertexCount;

	if (numVertsOrig - 1 >= 0)
	{
		f32 *pWrite = &pScratchBase->field_8;
		bool sawBillboardAnchor = false;

		for (i32 i = 0; i < numVertsOrig; i++)
		{
			DCVert *pv = &pVerts[i];
			i32 vf = pv->mFlags;

			if ((vf & 2) != 0)
			{
				i32 stitchIdx = 999 - (u8)((u32)vf >> 16);
				f32 *src = (f32*)((u8*)gDCStitchPositionTable + 16 * stitchIdx);
				pWrite[-1] = src[0];
				pWrite[0]  = src[1];
				pWrite[1]  = src[2];
			}
			else if (sawBillboardAnchor)
			{
				// nullsub_1 in the original is a confirmed empty no-op
				// (0x4015B0), so its call here is omitted.
			}
			else if ((((u8)vf) & 0x10) == 0)
			{
				f32 in4[4] = { pv->x, pv->y, pv->z, 1.0f };
				f32 out4[4];
				Algebra_Transform4(out4, in4);

				f32 w = out4[3];
				f32 invW = (fabsf(w) > 0.0000000099999999f) ? (1.0f / w) : -1.0e12f;
				f32 sx = out4[0] * invW;
				f32 sy = out4[1] * invW;
				f32 sz = invW * vertexScale;

				pWrite[-1] = sx;
				pWrite[0]  = sy;
				pWrite[1]  = sz;

				if ((pv->mFlags & 1) != 0)
				{
					*(f32*)(gDCAttachPointCursor + 0) = sx;
					*(f32*)(gDCAttachPointCursor + 4) = sy;
					*(f32*)(gDCAttachPointCursor + 8) = sz;
					gDCAttachPointCursor -= 16;
				}
			}
			else
			{
				// Billboard/ribbon vertex: x/y hold raw i32 indices into
				// the scratch pool (the documented DCVert "defect"); z is
				// a real float half-width. See the function-level comment.
				sawBillboardAnchor = true;

				i32 idx1 = *(i32*)&pv->x;
				i32 idx2 = *(i32*)&pv->y;
				f32 halfWidth = pv->z;

				tagKMVERTEX3 *anchor1 = pScratchBase + idx1;
				tagKMVERTEX3 *anchor2 = pScratchBase + idx2;

				f32 ax = anchor1->field_4, ay = anchor1->field_8, az = anchor1->field_C;
				f32 bx = anchor2->field_4, by = anchor2->field_8;

				f32 dx = bx - ax;
				f32 dy = ay - by;
				f32 lenSq = dy * dy + dx * dx;
				if (lenSq != 0.0f)
				{
					f32 invLen = 1.0f / sqrtf(lenSq);
					dx = invLen * dx;
					dy = invLen * dy;
				}

				f32 py = dy * halfWidth;
				f32 px = dx * halfWidth;
				f32 depthScale = az * 133.33333f;
				f32 offX = depthScale * py * 2.556f;
				f32 offY = depthScale * px;

				pWrite[-1] = offX + ax;
				pWrite[0]  = offY + ay;
				pWrite[1]  = az * vertexScale;
				// nullsub_1 (both branches) omitted, confirmed empty no-op.
			}

			pWrite += 8;
		}

		remainingVerts = remainingVertsSaved;
	}

	// Far-plane fog-dither scan: is at least one welded vertex within
	// (0, gM3dProjFar)? Skipped (treated as "yes") when gDCNoFogFlag is set.
	bool inFogRange = false;
	if (*gDCNoFogFlag == 0)
	{
		if (*gDCFogEnabled != 0 && !G_LOWGRAPHICS && remainingVerts >= 0)
		{
			f32 *pDepth = &pScratchBase->field_C;
			while (*pDepth >= *gDCProjFar || *pDepth <= 0.0f)
			{
				pDepth += 8;
				if (--remainingVerts < 0)
					goto fogScanDone;
			}
			inFogRange = true;
		}
	}
	else
	{
		inFogRange = true;
	}
fogScanDone:

	// Per-vertex dynamic-light tint block (only when gDCTexAnimFlag is set).
	// NEW FINDING, high confidence: this is the first confirmed READ of
	// DCModelData::pNormals by DCModel_RenderModel (dcmodel.h flagged that
	// field as unconfirmed at render time). It is walked in lock-step with
	// pVertices (same loop index, same iteration count), i.e. pNormals has
	// (at least) mVertexCount entries, one welded-vertex normal each --
	// not a separately-indexed per-source-normal array. Output is a
	// dedicated lit-color scratch buffer at 0x65DFB8 (gDCLitColorScratch,
	// 3 floats/vertex, walked in the SAME lock-step, NOT the tagKMVERTEX3
	// pool -- an earlier draft of this function wrongly conflated the two).
	if (*gDCTexAnimFlag != 0)
	{
		if ((u16)(pModel->NumVertices + pModel->NumFaces) <= pModel->NumNormals)
		{
			i32 lightCount = *gDCLightCount;
			DCNormal *pNormals = pData->pNormals;

			if (*gDCTintFlag != 0)
			{
				f32 scaleA = (f32)(u32)*gDCTexAnimColorSrcA / 255.0f;
				f32 scaleB = (f32)(u32)*gDCTexAnimColorSrcB / 255.0f;
				f32 scaleC = (f32)(u32)*gDCTexAnimColorSrcC / 255.0f;

				*(f32*)gDCTexAnimColorC = scaleA * *(f32*)gDCTexAnimColorC;
				*(f32*)gDCTexAnimColorB = scaleB * *(f32*)gDCTexAnimColorB;
				*(f32*)gDCTexAnimColorA = scaleC * *(f32*)gDCTexAnimColorA;

				if (lightCount > 0)
				{
					f32 *pLightTint = gDCLightTintTable;
					for (i32 li = 0; li < lightCount; li++)
					{
						f32 r = scaleA * pLightTint[0];
						f32 g = scaleB * pLightTint[1];
						f32 b = scaleC * pLightTint[2];
						pLightTint[0] = r;
						pLightTint[1] = g;
						pLightTint[2] = b;
						pLightTint += 3;
					}
				}
			}

			if (remainingVertsSaved >= 0)
			{
				f32 *pLitColor = (f32*)0x0065DFB8; // gDCLitColorScratch, 3 floats/vertex
				i32 vertsLeft = remainingVertsSaved + 1;

				for (i32 i = 0; i < vertsLeft; i++)
				{
					DCVert *pv = &pVerts[i];
					DCNormal *pn = &pNormals[i];

					if ((pv->mFlags & 2) != 0)
					{
						// Stitched/welded vertex: share the lit color from
						// a table keyed the same way as the position-share
						// table above (byte-2 of mFlags), confirming the
						// "share across welded copies" idea also applies
						// to lighting, not just screen position.
						i32 stitchIdx = 999 - (u8)((u32)pv->mFlags >> 16);
						((i32*)pLitColor)[-2] = gDCStitchColorIndexTable[3 * stitchIdx];
						pLitColor[-1] = *(f32*)(0x0064F858 + 12 * stitchIdx + 4);
						pLitColor[0]  = *(f32*)(0x0064F858 + 12 * stitchIdx + 8);
					}
					else
					{
						pLitColor[-2] = *(f32*)gDCTexAnimColorA;
						pLitColor[-1] = *(f32*)gDCTexAnimColorB;
						pLitColor[0]  = *(f32*)gDCTexAnimColorC;

						for (i32 li = 0; li < lightCount; li++)
						{
							f32 dot = GDC_LIGHT_VECTOR_TABLE_X(li) * pn->x
									+ GDC_LIGHT_VECTOR_TABLE_Z(li) * pn->z
									+ GDC_LIGHT_VECTOR_TABLE_Y(li) * pn->y;
							if (dot >= 0.0f)
							{
								pLitColor[-2] += dot * GDC_LIGHT_COLOR_TABLE_X(li);
								pLitColor[-1] += dot * GDC_LIGHT_COLOR_TABLE_Y(li);
								pLitColor[0]  += dot * GDC_LIGHT_COLOR_TABLE_Z(li);
							}
						}

						if ((pv->mFlags & 1) != 0)
						{
							// Same "attachment point" bit as the position
							// loop above: also records this vertex's lit
							// color into a companion bump-allocated list.
							*(f32*)gDCLitColorOutCursor = pLitColor[-2];
							*(f32*)(gDCLitColorOutCursor + 4) = pLitColor[-1];
							*(f32*)(gDCLitColorOutCursor + 8) = pLitColor[0];
							gDCLitColorOutCursor += 12;
						}
					}
					pLitColor += 3;
				}
			}
		}
		else
		{
			*gDCTexAnimFlag = 0;
		}
	}

	// Main per-face OT-build/texture-batching loop.
	// mSortBiasNormal (index 7) / mSortBiasLowGraphics (index 8), selected
	// with the ORIGINAL's raw (not boolified) `a2[dword_6B78F8 + 7]`
	// indexing -- reproduced as-is per CLAUDE.md (don't "fix" a source
	// idiom that happens to rely on the flag being exactly 0 or 1).
	i32 sortBias = ((i32*)pData)[7 + G_LOWGRAPHICS];
	i32 batchCount = 0;

	if (numVertsOrig > 0) // numFacesOrig (SModel::NumFaces) guards the whole block below
	{
		i32 facesLeft = pModel->NumFaces;
		u8 *pFaceCursor = (u8*)&pFaces->mVertSlot[0]; // "v46": pFaces + 12
		i32 overrideMask = *gDCOverrideFlags;

		i32 packedFaceFlags = 0;
		u32 alphaMask = 0xFF000000u;
		i32 alphaMode = 0; // 0/1/2, from mFlags bits 0x40/0x80

		for (;;)
		{
			u16 faceFlags = *(u16*)(pFaceCursor - 12); // DCFace::mFlags
			packedFaceFlags = (u16)overrideMask | (faceFlags & ((u16)(overrideMask >> 16)));
			alphaMask = 0xFF000000u;
			alphaMode = 0;

			u8 lowCombined = (u8)overrideMask | (u8)(faceFlags & (u8)(overrideMask >> 8));
			u8 fieldByte0 = *(pFaceCursor + 40); // DCFace::field_34[0]
			if ((lowCombined & 0xC0) != 0 && (fieldByte0 & 2) == 0)
				break;

			pFaceCursor += 56;
			facesLeft--;
			if (facesLeft == 0)
				goto afterFaceLoop;
		}

		if ((packedFaceFlags & 0x40) != 0)
		{
			alphaMode = ((packedFaceFlags & 0x80) != 0) ? 2 : 1;
			alphaMask = ((packedFaceFlags & 0x80) != 0) ? 0x80000000u : 0xFF000000u;
		}
		if ((overrideMask & 0x40) != 0)
		{
			alphaMode = 2;
			alphaMask = 0xFF000000u;
		}

		for (;;)
		{
			// Resolve this face's 4 corner slots (DCFace::mVertSlot), lazily
			// filling any not-yet-touched welded slot (index >= NumVertices)
			// from the corresponding corner's already-resolved neighbour.
			u16 cornerSlots[4];
			u16 *pSrcSlots = (u16*)pFaceCursor; // DCFace::mVertSlot[0..3]

			for (i32 c = 0; c < 4; c++)
			{
				i16 rawSlot = (i16)pSrcSlots[c];
				cornerSlots[c] = pSrcSlots[c];
				if (rawSlot < 0)
				{
					cornerSlots[c] = (u16)(pSrcSlots[c] & 0x7FFF);
				}
				else
				{
					i32 slot = cornerSlots[c];
					if (slot >= numVertsOrigClamped)
					{
						tagKMVERTEX3 *pDst = pScratchBase + slot;
						tagKMVERTEX3 *pSrc = pScratchBase + (u8)pFaceCursor[c - 8]; // DCFace::mVertIndex[c]
						pDst->field_4 = pSrc->field_4;
						pDst->field_8 = pSrc->field_8;
						pDst->field_C = pSrc->field_C;
					}
				}
			}

			// NOTE: the original also writes this loop's per-corner values
			// into a second scratch buffer at 0x62C512 (growing 12
			// bytes/face) with a different corner reorder ([2]=old[0]
			// etc). Confirmed via xref search this session that NOTHING,
			// in this function or any other, ever reads 0x62C512 back --
			// it is dead write-only scratch with no observable effect, so
			// it is not reproduced here (cornerSlots below is the only
			// copy of the resolved corner data that is actually consumed,
			// by the texture/color-resolve code right after).

			// Resolve this face's texture index.
			u16 texIndex;
			if ((packedFaceFlags & 1) != 0)
			{
				u16 override;
				if (*gDCForceTexIndex != 0)
					override = (u16)*gDCForceTexIndex;
				else if ((*(pFaceCursor + 40) & 1) != 0) // DCFace::field_34[0] bit 0
					override = *(u16*)(pFaceCursor + 42); // DCFace::field_34[2..3]
				else
					override = *(u16*)(pFaceCursor - 10); // DCFace::mTexIndex

				// print_if_false("Non ARB Texture", ...) omitted (debug-only assert).

				for (i32 c = 0; c < 4; c++)
				{
					u16 slot = (u16)(cornerSlots[c] & 0x7FFF);
					pScratchBase[slot].field_10 = *(f32*)(pFaceCursor + 24 + 4 * c - 16); // DCFace::mU[c]
					pScratchBase[slot].field_14 = *(f32*)(pFaceCursor + 24 + 4 * c);      // DCFace::mV[c]
				}
				texIndex = override;
			}
			else
			{
				texIndex = 1;
			}

			gDCFaceTexIndexOut[batchCount] = texIndex;
			gDCFaceNoLightOut[batchCount] = (u8)alphaMode;
			batchCount++;

			// Per-face sort-bias output: 0 by default, overridden by an
			// environment-map-mode lookup only in low-graphics mode
			// (matches gDCEnvMapTableA/B/C's case values against the
			// original switch exactly -- 0x2000->A, 0x4000->B, 0x6000->C).
			u16 sortBiasOut = 0;
			if (G_LOWGRAPHICS)
			{
				switch (packedFaceFlags & 0x6000)
				{
					case 0x2000: sortBiasOut = (u16)*gDCEnvMapTableA; break;
					case 0x4000: sortBiasOut = (u16)*gDCEnvMapTableB; break;
					case 0x6000: sortBiasOut = *gDCEnvMapTableC; break;
					default: break;
				}
			}
			gDCFaceSortBiasOut[batchCount - 1] = sortBiasOut;

			// Vertex-color resolve for this face's 4 corners. Always runs
			// (NOT gated by G_LOWGRAPHICS -- only the sort-bias lookup
			// above is). Three paths: (1) the tex-anim block's per-vertex
			// lit output (gDCLitColorScratch, indexed by the ORIGINAL
			// mVertIndex, not the welded slot); (2) mFlags bit 0x800 --
			// NEW FINDING, refines dcmodel.h's "medium confidence:
			// transparent/blended face" guess into something concrete:
			// this path reads DCFace::mColor[c] as a PALETTE INDEX into
			// G_COLOUR_TABLE (pColourTable), not a direct RGB value, i.e.
			// 0x800 means "this face's color is a colour-table index", not
			// a blend flag; (3) the face's own baked mColor/mColorExtra
			// (already gamma-corrected by DCModel_CreateFromSModel per
			// dcmodel.h).
			if (*gDCTexAnimFlag != 0)
			{
				f32 *pLitColor = (f32*)0x0065DFB8; // gDCLitColorScratch, same buffer as the tint block above
				for (i32 c = 0; c < 4; c++)
				{
					u16 slot = (u16)(cornerSlots[c] & 0x7FFF);
					u8 origVertIdx = pFaceCursor[c - 8]; // DCFace::mVertIndex[c]
					i32 ri = (i32)(pLitColor[origVertIdx * 3 + 0] * 255.0f);
					i32 gi = (i32)(pLitColor[origVertIdx * 3 + 1] * 255.0f);
					i32 bi = (i32)(pLitColor[origVertIdx * 3 + 2] * 255.0f);
					i32 orAll = ri | gi | bi;
					if ((orAll & (i32)0xFFFFFF00) != 0)
					{
						if (orAll < 0)
						{
							if (ri < 0) ri = 0;
							if (gi < 0) gi = 0;
							if (bi < 0) bi = 0;
						}
						if (((ri | gi | bi) & 0x7FFFFF00) != 0)
						{
							if (ri > 255) ri = 255;
							if (gi > 255) gi = 255;
							if (bi > 255) bi = 255;
						}
					}
					pScratchBase[slot].field_18 = alphaMask | (u32)bi | (((u32)gi | ((u32)ri << 8)) << 8);
					*(i32*)((u8*)&pScratchBase[slot] + 28) = 0; // untyped tail padding, offset 0x1C (no named field)
				}
			}
			else if ((packedFaceFlags & 0x800) != 0)
			{
				for (i32 c = 0; c < 4; c++)
				{
					u16 slot = (u16)(cornerSlots[c] & 0x7FFF);
					u8 colorIdx = pFaceCursor[c - 4]; // DCFace::mColor[0..2] / mColorExtra, raw 4-byte walk
					u32 entry = alphaMask | ((u32*)G_COLOUR_TABLE)[colorIdx];
					u32 packed = (entry & 0xFF00FF00u) | ((u8)entry << 16) | (u8)(entry >> 16);
					pScratchBase[slot].field_18 = packed;
					*(i32*)((u8*)&pScratchBase[slot] + 28) = 0; // untyped tail padding, offset 0x1C (no named field)
				}
			}
			else
			{
				u32 baseColor = alphaMask | *(u32*)(pFaceCursor - 4); // DCFace::mColor[0..2]+mColorExtra
				u32 bc = (baseColor & 0xFF00FF00u) | ((u8)baseColor << 16) | (u8)(baseColor >> 16);
				for (i32 c = 0; c < 4; c++)
				{
					u16 slot = (u16)(cornerSlots[c] & 0x7FFF);
					pScratchBase[slot].field_18 = bc;
					*(i32*)((u8*)&pScratchBase[slot] + 28) = 0; // untyped tail padding, offset 0x1C (no named field)
				}
			}

			pFaceCursor += 56;
			facesLeft--;
			if (facesLeft == 0)
				goto afterFaceLoop;

			// find the next qualifying face (same gate as the search above)
			for (;;)
			{
				u16 nextFlags = *(u16*)(pFaceCursor - 12);
				packedFaceFlags = (u16)overrideMask | (nextFlags & (u16)(overrideMask >> 16));
				u8 nextLowCombined = (u8)overrideMask | (u8)(nextFlags & (u8)(overrideMask >> 8));
				if ((nextLowCombined & 0xC0) != 0 && (*(pFaceCursor + 40) & 2) == 0)
					break;
				pFaceCursor += 56;
				facesLeft--;
				if (facesLeft == 0)
					goto afterFaceLoop;
			}

			if ((packedFaceFlags & 0x40) != 0)
			{
				alphaMode = ((packedFaceFlags & 0x80) != 0) ? 2 : 1;
				alphaMask = ((packedFaceFlags & 0x80) != 0) ? 0x80000000u : 0xFF000000u;
			}
			if ((overrideMask & 0x40) != 0)
			{
				alphaMode = 2;
				alphaMask = 0xFF000000u;
			}
		}
	}
afterFaceLoop:

	// Debug light-vector visualization (only when gDCDebugLightFlag is set).
	if (*gDCDebugLightFlag != 0)
	{
		PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

		f32 origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		f32 originOut[4];
		Algebra_Transform4(originOut, origin);
		f32 originInvW = (fabsf(originOut[3]) > 0.0000000099999999f) ? (1.0f / originOut[3]) : -1.0e12f;
		f32 originX = originOut[0] * originInvW;
		f32 originY = originOut[1] * originInvW;

		i32 lightCount = *gDCLightCount;
		if (lightCount > 0)
		{
			f32 *pLightTint = gDCLightTintTable;
			for (i32 li = 0; li < lightCount; li++)
			{
				f32 lx = *(f32*)0x00550068 * pLightTint[2];
				f32 ly = *(f32*)0x00550068 * pLightTint[1];
				f32 lz = *(f32*)0x00550068 * pLightTint[0];

				f32 in4[4] = { lz, ly, lx, 1.0f };
				f32 out4[4];
				Algebra_Transform4(out4, in4);
				f32 invW = (fabsf(out4[3]) > 0.0000000099999999f) ? (1.0f / out4[3]) : -1.0e12f;
				f32 px = out4[0] * invW;
				f32 py = out4[1] * invW;

				u32 color = 0xFFFFFF00u
					| (u8)(pLightTint[2] * 255.0)
					| (((u8)(pLightTint[1] * 255.0) | (((u8)(pLightTint[0] * 255.0)) << 8)) << 8);

				gsub_509000(originX, originY, invW, -1, px, py, invW, color, 2.0f);
				pLightTint += 3;
			}
		}
	}

	// Fog-distance / no-fog color remap pass over the whole scratch pool
	// range this call touched (pScratchBase[0..pData->mVertexCount)).
	if (*gDCLastNoLightFlag == 0)
	{
		if (*gDCNoFogFlag != 0)
		{
			for (i32 i = 0; i < vertexCount; i++)
			{
				u32 c = pScratchBase[i].field_18;
				u32 blended = (*(u32*)0x00550060 * (c >> 8));
				pScratchBase[i].field_18 = blended ^ (0xFFFFFFu & (c ^ blended));
			}
		}
		else if (inFogRange)
		{
			for (i32 i = 0; i < vertexCount; i++)
			{
				f32 depth = pScratchBase[i].field_C;
				if (depth < *gDCProjFar && depth > 0.0f)
				{
					i32 remap = 0;
					if (depth > *gDCProjNear)
						remap = (i32)((depth - *gDCProjNear) * *gDCProjScale);
					u32 c = pScratchBase[i].field_18;
					u32 blended = (u32)remap * (c >> 8);
					pScratchBase[i].field_18 = blended ^ (0xFFFFFFu & (c ^ blended));
				}
			}
		}

		// Texture-batch submission: group consecutive OT-build faces that
		// share (texture id, no-light flag, sort-bias), call PCGfx_UseTexture
		// once per batch (skipping the call when nothing changed), then
		// submit the batch's triangle indices from the fixed template.
		i32 idx = 0;
		if (batchCount > 0)
		{
			do
			{
				i32 noLight = inFogRange ? 1 : gDCFaceNoLightOut[idx];
				i32 lastTex = *gDCLastBoundTexture;
				u16 tex = gDCFaceTexIndexOut[idx];

				if (*gDCLastBoundTexture != tex || *gDCLastNoLightFlag != noLight)
				{
					if (*gDCForceNoTexFlag != 0)
					{
						PCGfx_UseTexture(1, DCGfx_BlendingMode_0);
					}
					else
					{
						i32 noLightArg = (*gDCForceNoLightFlag != 0) ? 0 : noLight;
						PCGfx_UseTexture(tex, (DCGfx_BlendingMode)noLightArg);
					}
					lastTex = *gDCLastBoundTexture;
					(*gDCBatchCallCount)++;
				}

				u16 bias = gDCFaceSortBiasOut[idx];
				i32 j = idx;
				*gDCFaceSortKey = sortBias + bias;
				do
				{
					j++;
				} while (j < batchCount
					&& lastTex == (i32)(u16)gDCFaceTexIndexOut[j]
					&& *gDCLastNoLightFlag == (i32)(u8)gDCFaceNoLightOut[j]
					&& bias == gDCFaceSortBiasOut[j]);

				PCGfx_ClipSendIndexedVertList(
					pScratchBase,
					4 * batchCount,
					gDCTriIndexTemplate + 6 * idx,
					6 * (j - idx));

				idx = j;
			} while (idx < batchCount);
		}

		*gDCFaceSortKey = 0;
		*gDCFaceSortKeyExtra = 0;
	}
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
