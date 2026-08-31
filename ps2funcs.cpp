#include "ps2funcs.h"
#include "validate.h"
#include <cmath>
#include <cstring>
#include "ob.h"
#include "m3dcolij.h"
#include "Sbinit.h"
#include "stubs.h"
#include "PCGfx.h"
#include "pcdcMem.h"
#include "ps2gamefmv.h"
#include "ps2redbook.h"
#include "ps2lowsfx.h"
#include "ps2pad.h"
#include "PCTex.h"

#include <cstdlib>

#include "my_assert.h"

i32 gClutRelated;

i32 DoVblankProcessing = 1;
i32 gPortRelatedOne;
i32 gSomeSize = 0x6E0000;

SSinCos rcossin_tbl[FLATBIT_VELOCITIES_SIZE];

i32 Pal16X;
i32 Pal16Y;

EXPORT i16 gRotMatrix[3][3];

EXPORT int vertexRegister[4];

// guesses: not in idb_globals.txt. Positioned right after vertexRegister ("V0"),
// same shape, used by MTC2 the same way for register indices 2/3 ("V1") and 4/5 ("V2").
EXPORT int gVertexRegister1[4];
EXPORT int gVertexRegister2[4];

EXPORT VECTOR translationVector;
EXPORT VECTOR gGeneralLongVector;

EXPORT int gRtpsRelatedNoClue;
EXPORT int gRtpsRelatedNoClue2;
EXPORT int gRtpsRelatedNoClue3;

EXPORT VECTOR gFtwOp12;
EXPORT VECTOR gWtfOP12;
EXPORT VECTOR gOp12Result;

static unsigned char stubGte = 1;

u8 gPrintStubbed = 1;
u8 gClearImagePrint = 1;

// @Ok
// @Matching
void MTC2(i32 a1, GTREGType a2)
{
	print_if_false(a2 >= 0 && a2 < 0x16, "Invalid GTE register specified to MTC2.");

	switch (a2)
	{
	case GT_ZERO:
		vertexRegister[0] = (i16)a1;
		vertexRegister[1] = a1 >> 16;
		break;
	case GT_ONE:
		vertexRegister[2] = (i16)a1;
		break;
	case GT_TWO:
		gVertexRegister1[0] = (i16)a1;
		gVertexRegister1[1] = a1 >> 16;
		break;
	case GT_THREE:
		gVertexRegister1[2] = (i16)a1;
		break;
	case GT_FOUR:
		gVertexRegister2[0] = a1 & 0xFFFF;
		gVertexRegister2[1] = a1 >> 16;
		break;
	case GT_FIVE:
		gVertexRegister2[2] = (i16)a1;
		break;
	case GT_SIX:
		print_if_false(0, "MTC2 tried to write to IR0.");
		break;
	case GT_SEVEN:
		gOp12Result.vx = a1;
		break;
	case GT_EIGHT:
		gOp12Result.vy = a1;
		break;
	case GT_NINE:
		gOp12Result.vz = a1;
		break;
	case GT_TEN:
		print_if_false(0, "MTC2 tried to write to MAC0.");
		break;
	case GT_ELEVEN:
		gGeneralLongVector.vx = a1;
		break;
	case GT_TWELVE:
		gGeneralLongVector.vy = a1;
		break;
	case GT_THIRTEEN:
		gGeneralLongVector.vz = a1;
		break;
	case GT_FOURTEEN:
		print_if_false(0, "MTC2 tried to write to RotMat.");
		break;
	case GT_FIFTEEN:
		gRotMatrix[2][2] = (i16)a1;
		break;
	case GT_SIXTEEN:
		translationVector.vx = a1;
		break;
	case GT_SEVENTEEN:
		translationVector.vy = a1;
		break;
	case GT_EIGHTEEN:
		translationVector.vz = a1;
		break;
	case GT_NINETEEN:
	case GT_TWENTY:
	case GT_TWENTYONE:
	default:
		print_if_false(0, "Unhandled case.");
		break;
	}
}

// @Bogus
void setPolyGT4(void)
{
	if (!gPrintStubbed)
		stubbed_printf("stubbed out: setPolyGT4");
}

// @Ok
// @Matching
void TransMatrix(MATRIX* a1, VECTOR* a2)
{
	a1->t[0] = a2->vx;
	a1->t[1] = a2->vy;
	a1->t[2] = a2->vz;
}

// @Ok
// @Matching
void gte_op0(void)
{
	gGeneralLongVector.vx = gWtfOP12.vy * gFtwOp12.vz - gWtfOP12.vz * gFtwOp12.vy;
	gGeneralLongVector.vy = gWtfOP12.vz * gFtwOp12.vx - gFtwOp12.vz * gWtfOP12.vx;
	gGeneralLongVector.vz = gFtwOp12.vy * gWtfOP12.vx - gWtfOP12.vy * gFtwOp12.vx;
}

// @Ok
// @Matching
void gte_SetRotMatrix(MATRIX* a1)
{
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 3; j++){
			gRotMatrix[i][j] = a1->m[i][j];
		}
	}
}

// auto_inline off: these are real out-of-line functions in the original binary (each has
// exactly one caller in this TU). With /Ob2 the compiler would otherwise inline them into
// that caller, turning the caller's real "call ADDR" into inlined code and desyncing the
// caller's disassembly. This does not change behavior, only codegen shape.
#pragma auto_inline(off)

// unnamed helper, address 0x0046E990. Called once by M3dAsm_LineColijPreprocessItemsZoned
// with the fixed-point start/end coordinate arrays and the address of a local the caller
// DOES read back afterward, as a per-axis swap bitmask (confirmed against Hex-Rays: the
// caller casts it to a byte and passes it straight into gsub_46EA20 as the flags
// parameter). Sorts a1[i]/a2[i] per axis (so a1 <= a2), toggling bit i of *a3 whenever a
// swap happens, then stores the per-axis a2-a1 deltas into gRotMatrix (idb_globals.txt:
// 0x00610B20 = gRotMatrix; here it is reused as scratch storage for the sorted box's
// extents, read back by gsub_46EB30 below).
// @Ok
// @Matching
EXPORT void gsub_46E990(i32 *a1, i32 *a2, i32 *a3)
{
	if (a1[0] > a2[0])
	{
		*a3 ^= 1;
		i32 tmp = a1[0];
		a1[0] = a2[0];
		a2[0] = tmp;
	}

	if (a1[1] > a2[1])
	{
		*a3 ^= 2;
		i32 tmp = a1[1];
		a1[1] = a2[1];
		a2[1] = tmp;
	}

	if (a1[2] > a2[2])
	{
		*a3 ^= 4;
		i32 tmp = a1[2];
		a1[2] = a2[2];
		a2[2] = tmp;
	}

	*reinterpret_cast<i32*>(&gRotMatrix[0][0]) = a2[0] - a1[0];
	*reinterpret_cast<i32*>(&gRotMatrix[1][1]) = a2[1] - a1[1];
	*reinterpret_cast<i32*>(&gRotMatrix[2][2]) = a2[2] - a1[2];
}

// unnamed helper, address 0x0046EA20. Called once per item by
// M3dAsm_LineColijPreprocessItemsZoned (and by M3dAsm_LineColijPreprocessItems) with the
// per-model pointer looked up from CItemRelatedList[region*17][model], the line's sorted
// start/end coords (flat, not a Vector by value), the axis-swap bitmask gsub_46E990 wrote
// through its 3rd argument, and the item's fixed-point position. pModel[3]/[4]/[5] each
// pack two i16 model-space extents per axis (low 16 bits, high 16 bits). Builds the
// item's world-space bounding box from those extents and writes it into the two output
// Vector-shaped int[3] pointers, mirroring the box about (start+end) per axis whenever
// gsub_46E990 had to swap that axis, so both corners line up with the line's (now always
// start <= end) coordinates. Return value discarded by every caller, so it is void here.
// @Ok
EXPORT void gsub_46EA20(const i32 *pModel, i32 startX, i32 startY, i32 startZ,
                         i32 endX, i32 endY, i32 endZ, u8 flags,
                         i32 posX, i32 posY, i32 posZ, i32 *pOut1, i32 *pOut2)
{
	i32 loX = (i16)pModel[3] + posX + 2;
	i32 hiX = (pModel[3] >> 16) + posX - 2;

	if (flags & 1)
	{
		pOut1[0] = startX + endX - loX;
		pOut2[0] = startX + endX - hiX;
	}
	else
	{
		pOut1[0] = hiX;
		pOut2[0] = loX;
	}

	i32 loY = (i16)pModel[4] + posY + 2;
	i32 hiY = (pModel[4] >> 16) + posY - 2;

	if (flags & 2)
	{
		pOut1[1] = startY + endY - loY;
		pOut2[1] = startY + endY - hiY;
	}
	else
	{
		pOut1[1] = hiY;
		pOut2[1] = loY;
	}

	i32 loZ = (i16)pModel[5] + posZ + 2;
	i32 hiZ = (pModel[5] >> 16) + posZ - 2;

	if (flags & 4)
	{
		pOut1[2] = startZ + endZ - loZ;
		pOut2[2] = startZ + endZ - hiZ;
	}
	else
	{
		pOut1[2] = hiZ;
		pOut2[2] = loZ;
	}
}

// unnamed helper, address 0x0046EB30. Called once per item by
// M3dAsm_LineColijPreprocessItemsZoned (and by M3dAsm_LineColijPreprocessItems) with the
// line's sorted start (a1..a3) and end (a4..a6) coords and the item's box corners as
// written by gsub_46EA20 (a7..a9 = pOut1, a10..a12 = pOut2). Returns nonzero when the
// line segment intersects the box (a slab/cross-product test). Reads the per-axis
// end-start deltas back out of gRotMatrix's diagonal, where gsub_46E990 stashed them as
// scratch (see its comment above).
// @Ok
EXPORT i32 gsub_46EB30(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6,
                        i32 a7, i32 a8, i32 a9, i32 a10, i32 a11, i32 a12)
{
	if (a7 > a4 || a8 > a5 || a9 > a6 || a10 < a1 || a11 < a2 || a12 < a3)
		return 0;

	i32 dx = *reinterpret_cast<i32*>(&gRotMatrix[0][0]);
	i32 dy = *reinterpret_cast<i32*>(&gRotMatrix[1][1]);
	i32 dz = *reinterpret_cast<i32*>(&gRotMatrix[2][2]);

	i32 ey = a11 - a2;
	i32 ez = a12 - a3;
	i32 ex = a10 - a1;

	i32 dxez = dx * ez;
	i32 dyez = dy * ez;

	if (dyez - dz * ey >= 0)
	{
		if (dx * ey - dy * ex < 0)
			return (dy * (a9 - a3) - dz * ey <= 0) && (dx * ey - dy * (a7 - a1) >= 0);

		return (dz * ex - dx * (a9 - a3) >= 0) && (dx * (a8 - a2) - dy * ex <= 0);
	}

	if (dz * ex - dxez < 0)
		return (dz * ex - dx * (a9 - a3) >= 0) && (dx * (a8 - a2) - dy * ex <= 0);

	if (dyez - dz * (a8 - a2) >= 0 && dz * (a7 - a1) - dxez <= 0)
		return 1;

	return 0;
}

#pragma auto_inline(on)

// @Ok
// Fixed against Hex-Rays decompile of 0x0046E7B0 (2026-08-31): the old version had two
// real bugs, not just register-allocation residue. (1) The "unused" bitmask local IS read
// back: gsub_46E990 toggles bit i of it whenever it swaps axis i, and the caller then
// casts it to a byte and feeds it into gsub_46EA20 as a flags argument. (2) gsub_46EA20
// and gsub_46EB30 were being called with the wrong shape entirely (gsub_46EA20 took only
// the model pointer; gsub_46EB30 took Vector-by-value start/itemPos/end). Their real
// signatures (confirmed against the disassembly of both) are flat-int and very different:
// gsub_46EA20 builds the item's world-space bounding box from the model's packed extents
// and the swap flags, gsub_46EB30 does a line-vs-box intersection test against that box.
// See both functions' updated comments above for the full call shape.
void M3dAsm_LineColijPreprocessItemsZoned(CItem **ppItem, i32 ModelTable, SLineInfo *pInfo, u16 Inquiry)
{
	Vector start = {0, 0, 0};
	Vector end = {0, 0, 0};

	CItem *pItem = *ppItem;

	if (pItem)
	{
		start.vx = pInfo->StartCoords.vx >> 12;
		start.vy = pInfo->StartCoords.vy >> 12;
		start.vz = pInfo->StartCoords.vz >> 12;

		end.vx = pInfo->EndCoords.vx >> 12;
		end.vy = pInfo->EndCoords.vy >> 12;
		end.vz = pInfo->EndCoords.vz >> 12;

		i32 swapFlags = 0;
		gsub_46E990(reinterpret_cast<i32*>(&start), reinterpret_cast<i32*>(&end), &swapFlags);

		do
		{
			i32 **pRegionEntry = CItemRelatedList[pItem->mRegion * 17];

			if (pItem->mFlags & 0x21)
			{
				pItem->mInquiry = Inquiry;
			}
			else if (pItem->mInquiry != Inquiry)
			{
				const i32 *pModel = reinterpret_cast<const i32*>(pRegionEntry[pItem->mModel]);

				i32 posX = pItem->mPos.vx >> 12;
				i32 posY = pItem->mPos.vy >> 12;
				i32 posZ = pItem->mPos.vz >> 12;

				i32 boxA[3];
				i32 boxB[3];

				gsub_46EA20(pModel, start.vx, start.vy, start.vz, end.vx, end.vy, end.vz,
				            static_cast<u8>(swapFlags), posX, posY, posZ, boxA, boxB);

				if (!gsub_46EB30(start.vx, start.vy, start.vz, end.vx, end.vy, end.vz,
				                 boxA[0], boxA[1], boxA[2], boxB[0], boxB[1], boxB[2]))
				{
					pItem->mInquiry = Inquiry;
				}
			}

			ppItem++;
			pItem = *ppItem;
		} while (pItem);
	}
}

// @Ok
void gte_ldv0(const SVECTOR* a1){
	vertexRegister[0] = a1->vx;
	vertexRegister[1] = a1->vy;
	vertexRegister[2] = a1->vz;
}

// @Ok
EXPORT void INLINE FixedXForm(i16 matrix[3][3], const VECTOR* a, VECTOR *r){

	int x = a->vx;
	int y = a->vy;
	int z = a->vz;

	r->vx = (x * matrix[0][0] + y * matrix[0][1] + z * matrix[0][2]) >> 12;
	r->vy = (x * matrix[1][0] + y * matrix[1][1] + z * matrix[1][2]) >> 12;
	r->vz = (x * matrix[2][0] + y * matrix[2][1] + z * matrix[2][2]) >> 12;
	r->pad = (long)r;
}

// @Ok
void gte_rtv0tr(void)
{
	FixedXForm(gRotMatrix, (VECTOR*)&vertexRegister[0], &gGeneralLongVector);

	gGeneralLongVector.vx += translationVector.vx >> 12;
	gGeneralLongVector.vy += translationVector.vy >> 12;
	gGeneralLongVector.vz += translationVector.vz >> 12;
}

// @Ok
void gte_stlvnl(VECTOR *a1)
{
  a1->vx = gGeneralLongVector.vx;
  a1->vy = gGeneralLongVector.vy;
  a1->vz = gGeneralLongVector.vz;
}

// @Ok
void gte_rtps(void){

	FixedXForm(gRotMatrix, (VECTOR*)&vertexRegister[0], &gGeneralLongVector);
	gGeneralLongVector.vz = translationVector.vz + gGeneralLongVector.vy;
	

	if (gGeneralLongVector.vz == 0){
		gGeneralLongVector.vx = 0x8000;
		gGeneralLongVector.vy = 0x8000;
	}
	else{
		gGeneralLongVector.vx = gRtpsRelatedNoClue2 / 2
                          + (gGeneralLongVector.vx + translationVector.vx) * gRtpsRelatedNoClue / gGeneralLongVector.vz;
		gGeneralLongVector.vy = gRtpsRelatedNoClue3 / 2
							  + (translationVector.vy
							   + ((vertexRegister[0] * gRotMatrix[1][0]
								 + vertexRegister[1] * gRotMatrix[1][1]
								 + vertexRegister[2] * gRotMatrix[1][2]) >> 12))
							  * gRtpsRelatedNoClue
							  / gGeneralLongVector.vz;
	}

}


// @ok
void gte_rtpt(void){
	if ( !stubGte )
		stubbed_printf("stubbed out: gte_rtpt()");
}

// @Ok
void gte_op12(void)
{
  gGeneralLongVector.vz = (gFtwOp12.vy * gWtfOP12.vx - gWtfOP12.vy * gFtwOp12.vx) >> 12;
  gGeneralLongVector.vx = (gWtfOP12.vy * gFtwOp12.vz - gWtfOP12.vz * gFtwOp12.vy) >> 12;
  gGeneralLongVector.vy = (gWtfOP12.vz * gFtwOp12.vx - gFtwOp12.vz * gWtfOP12.vx) >> 12;
  gOp12Result = gGeneralLongVector;
}


// @Ok
void gte_ldlvl(VECTOR *a1)
{
  gOp12Result = *a1;
}

// @Ok
void gte_sqr0(void)
{
  gGeneralLongVector.vx = gOp12Result.vx * gOp12Result.vx;
  gGeneralLongVector.vy = gOp12Result.vy * gOp12Result.vy;
  gGeneralLongVector.vz = gOp12Result.vz * gOp12Result.vz;
}



// @Ok
void gte_rtv0(void)
{
	FixedXForm(gRotMatrix, (VECTOR*)&vertexRegister[0], &gGeneralLongVector);

	gOp12Result = gGeneralLongVector;
}

// @Ok
void gte_stlvnl0(int *a1)
{
  *a1 = gGeneralLongVector.vx;
}

// @Ok
void gte_stlvnl2(int *a1)
{
  *a1 = gGeneralLongVector.vz;
}

EXPORT int gScalar;
// @Ok
void gte_gpf0()
{
  gGeneralLongVector.vx = gOp12Result.vx * gScalar;
  gGeneralLongVector.vy = gScalar * gOp12Result.vy;
  gGeneralLongVector.vz = gScalar * gOp12Result.vz;
}

EXPORT int lzc;

// @Ok
// @Matching
void gte_stlzc(int *a1)
{
  int v1; // esi
  int v2; // eax
  int v3; // eax

  v1 = lzc;
  print_if_false(lzc != 0, "lzc not zero");
  if ( v1 < 0 )
  {
    v2 = 0;
    do
    {
      v1 <<= 1;
      ++v2;
    }
    while ( v1 < 0 );
    *a1 = v2;
  }
  else
  {
	v3 = 0;
    do
    {
      v1 <<= 1;
      ++v3;
    }
    while ( v1 >= 0 );
    *a1 = v3;
  }
}


// @Ok
void gte_stsv(SVECTOR *a1)
{
  a1->vx = (short)gOp12Result.vx;
  a1->vy = (short)gOp12Result.vy;
  a1->vz = (short)gOp12Result.vz;
}


// @Ok
// Verified 2026-08-31 against Hex-Rays decompile of 0x0046E0F0: the row*vector dot product
// here (gRotMatrix[i][0]*v7->vx + gRotMatrix[i][1]*v7->vy + gRotMatrix[i][2]*v7->vz per row)
// matches exactly once the original's 32-bit-aliased reads of gRotMatrix (dword_610B20,
// dword_610B24, dword_610B28, dword_610B2C, dword_610B30, each packing two adjacent i16
// matrix cells) are unpacked back into individual gRotMatrix[i][j] cells; same for the
// v7 selection (dword_610BB0 vs dword_610BE0 based on a3, matching vertexRegister vs
// gOp12Result here) and the final gOp12Result/gGeneralLongVector writes including the pad
// field. Only register-home/scheduling residue remains, which is fine under the
// functional-only bar for this session.
void gte_mvmva(int _sf, int mx, int a3, int cv, int lm)
{
  VECTOR *v7; // eax

  print_if_false(!(_sf!=0 && _sf!=1), "sf!=0 && sf!=1");
  print_if_false(mx == 0, "MX!=0");
  print_if_false(!a3 || a3 == 3, "bad v");
  print_if_false(cv == 3, "cv!=3");
  print_if_false(lm == 0, "lm!=0");
  v7 = (VECTOR *)vertexRegister;

  if ( a3 )
    v7 = &gOp12Result;

  gGeneralLongVector.vx = gRotMatrix[0][0] * v7->vx + gRotMatrix[0][1] * v7->vy + gRotMatrix[0][2] * v7->vz;
  gGeneralLongVector.vy = gRotMatrix[1][0] * v7->vx + gRotMatrix[1][1] * v7->vy + gRotMatrix[1][2] * v7->vz;
  gGeneralLongVector.vz = gRotMatrix[2][0] * v7->vx + gRotMatrix[2][1] * v7->vy + gRotMatrix[2][2] * v7->vz;

  if ( _sf == 1 )
  {
    gGeneralLongVector.vx = gGeneralLongVector.vx >> 12;
    gGeneralLongVector.vy = gGeneralLongVector.vy >> 12;
    gGeneralLongVector.vz = gGeneralLongVector.vz >> 12;
  }

  gOp12Result.vz = gGeneralLongVector.vz;
  gOp12Result.vx = gGeneralLongVector.vx;
  gOp12Result.vy = gGeneralLongVector.vy;
  gOp12Result.pad = gGeneralLongVector.pad;
}


// @Ok
void gte_stsxy(int *a1)
{
  *a1 = (gGeneralLongVector.vx & 0xFFFF) | (gGeneralLongVector.vy << 16);
}

// @Ok
void gte_lddp(int a1)
{
  gScalar = a1;
}


// @Ok
void gte_ldsvrtrow0(const SVECTOR *a1)
{
  gRotMatrix[0][0] = a1->vx;
  gRotMatrix[0][1] = a1->vy;
  gRotMatrix[0][2] = a1->vz;
}

// @Ok
void gte_ldopv1(VECTOR *a1)
{
  gWtfOP12 = *a1;
}

// @Ok
void gte_ldopv2(VECTOR *a1)
{
  gFtwOp12 = *a1;
}


// @Ok
// @Matching
void gte_ldlv0(const VECTOR *a1)
{
  *(VECTOR *)vertexRegister = *a1;
}


// @Ok
// @Matching
void gte_stsxy3(int *a1, int *a2, int *a3)
{
  *a1 = (gOp12Result.vx & 0xFFFF) | (gOp12Result.vy << 16);
  *a2 = (gOp12Result.vx & 0xFFFF) | (gOp12Result.vy << 16);
  *a3 = (gOp12Result.vx & 0xFFFF) | (gOp12Result.vy << 16);
  if ( !stubGte )
    stubbed_printf("stubbed out:  gte_stsxy3");
}


// @Ok
void gte_rtir(void){
	FixedXForm(gRotMatrix, &gOp12Result, &gGeneralLongVector);
}

// @Ok
// @Matching
void gsub_46D9B0(VECTOR *a1)
{
	a1->vx = gOp12Result.vx;
	a1->vy = gOp12Result.vy;
	a1->vz = gOp12Result.vz;
}

// @Ok
void M3dMaths_SetIdentityRotation(MATRIX *a1)
{
  a1->m[2][2] = 4096;
  a1->m[1][1] = 4096;
  a1->m[0][0] = 4096;

  a1->m[2][1] = 0;
  a1->m[2][0] = 0;
  a1->m[1][2] = 0;

  a1->m[1][0] = 0;
  a1->m[0][2] = 0;
  a1->m[0][1] = 0;
}

// @NotOk
// Residue: the maths (row*col dot product of a1*a2 into a3, each >>12) was already right; walking
// pointers over a1/a2/a3 (instead of a1->m[i][j] direct indexing) got 3 mnemonic diffs down to 87
// (from 109), matching the original's incremental-pointer read/store shape. The remaining diffs
// are in the register allocation of the 9 dot-product statements (which of eax/ebx/ecx/edx/edi/
// ebp/esi holds which of v3..v20 at each point) and the exact read/store instruction-scheduling
// interleave for the a1/a3 walks (the original peeks 2 elements ahead before advancing the
// pointer at a few points, not a uniform 1-at-a-time walk). 4 attempts tried (direct indexing;
// walking pointer for a1/a2 reads only; + walking pointer for a3 store; declaration-order swap of
// p3), see attempts log. Below the 15-hypothesis medium-size bar, revisit.
void MulMatrix0(MATRIX *a1, MATRIX *a2, MATRIX *a3)
{
  int v3; // [sp+0h] [-78h]
  int v4; // [sp+4h] [-74h]
  int v5; // [sp+8h] [-70h]
  int v6; // [sp+Ch] [-6Ch]
  int v7; // [sp+10h] [-68h]
  int v8; // [sp+14h] [-64h]
  int v9; // [sp+18h] [-60h]
  int v10; // [sp+1Ch] [-5Ch]
  int v11; // [sp+20h] [-58h]
  int v12; // [sp+24h] [-54h]
  int v13; // [sp+28h] [-50h]
  int v14; // [sp+2Ch] [-4Ch]
  int v15; // [sp+30h] [-48h]
  int v16; // [sp+34h] [-44h]
  int v17; // [sp+38h] [-40h]
  int v18; // [sp+3Ch] [-3Ch]
  int v19; // [sp+40h] [-38h]
  int v20; // [sp+44h] [-34h]

  i16* p1 = &a1->m[0][0];
  v12 = *p1; p1++;
  v13 = *p1; p1++;
  v14 = *p1; p1++;
  v15 = *p1; p1++;
  v16 = *p1; p1++;
  v17 = *p1; p1++;
  v18 = *p1; p1++;
  v19 = *p1; p1++;
  v20 = *p1;

  i16* p2 = &a2->m[0][0];
  v3 = *p2; p2++;
  v4 = *p2; p2++;
  v5 = *p2; p2++;
  v6 = *p2; p2++;
  v7 = *p2; p2++;
  v8 = *p2; p2++;
  v9 = *p2; p2++;
  v10 = *p2; p2++;
  v11 = *p2;

  i16* p3 = &a3->m[0][0];
  *p3 = (v12 * v3 + v13 * v6 + v14 * v9) >> 12; p3++;
  *p3 = (v12 * v4 + v13 * v7 + v14 * v10) >> 12; p3++;
  *p3 = (v12 * v5 + v13 * v8 + v14 * v11) >> 12; p3++;
  *p3 = (v15 * v3 + v16 * v6 + v17 * v9) >> 12; p3++;
  *p3 = (v15 * v4 + v16 * v7 + v17 * v10) >> 12; p3++;
  *p3 = (v15 * v5 + v16 * v8 + v17 * v11) >> 12; p3++;
  *p3 = (v18 * v3 + v19 * v6 + v20 * v9) >> 12; p3++;
  *p3 = (v18 * v4 + v19 * v7 + v20 * v10) >> 12; p3++;
  *p3 = (v18 * v5 + v19 * v8 + v20 * v11) >> 12;
}


// @Ok
void MulMatrix(MATRIX *a1, MATRIX *a2)
{
  //MATRIX v2 = *a1;

	MATRIX v2;
	for (int i = 0; i<3; i++){
		for (int j = 0; j<3; j++){
			v2.m[i][j] = a2->m[i][j];
		}
	}
  MulMatrix0(&v2, a2, a1);
}

// @Ok
void m3d_ZeroTransVector(void)
{
  translationVector.vx = 0;
  translationVector.vy = 0;
  translationVector.vz = 0;
}

// @Ok
// @Matching
void VectorNormal(VECTOR* a1, VECTOR* a2)
{
	float fx = (float)a1->vx;
	float fy = (float)a1->vy;
	float fz = (float)a1->vz;

	float lenSq = fx * fx + fy * fy + fz * fz;

	if (lenSq == 0.0f)
	{
		a2->vx = 0;
		a2->vy = 0x1000;
		a2->vz = 0;
		return;
	}

	float len = (float)sqrt((double)lenSq);

	a2->vx = (i32)((float)(a1->vx << 12) / len);
	a2->vy = (i32)((float)(a1->vy << 12) / len);
	a2->vz = (i32)((float)(a1->vz << 12) / len);
}

// @Ok
// @Matching
// @Note: kudos to valps, adding extra parentheses fix it
i32 M3dMaths_SquareRoot0(i32 i){

    if (i <= -32768) {
        return 32768;
    }

    if (i < 0) {
        return 0;
    }
    
	return sqrt(((double)i));
}



// @Ok
// @Matching
i32 M3dMaths_MulDiv64(i32 a1, i32 a2, i32 a3)
{
	if (!a3)
	{
		return -1;
	}

	f64 hope = (f64)a1 * (f64)a2;
	hope /= (f64)a3;

	ASSERT(hope <= 2147483647.0, "hope<=INT_MAX");
	ASSERT(hope >= -2147483648.0, "hope>=INT_MIN");

	return hope;
}

// @Ok
void M3dMaths_TransposeMatrix1(MATRIX *a1, MATRIX *a2)
{
	a2->m[0][0] = a1->m[0][0];
	a2->m[0][1] = a1->m[1][0];
	a2->m[0][2] = a1->m[2][0];
	a2->m[1][0] = a1->m[0][1];
	a2->m[1][1] = a1->m[1][1];
	a2->m[1][2] = a1->m[2][1];
	a2->m[2][0] = a1->m[0][2];
	a2->m[2][1] = a1->m[1][2];
	a2->m[2][2] = a1->m[2][2];
}



// @Ok
// @Matching
void M3dMaths_ScaleMatrix(CItem *a1, MATRIX *a2)
{
	MATRIX v7;
	MATRIX v8;
	memset((void*)&v8, 0, sizeof(v8));

	v8.m[0][0] = a1->mScale.vx;
	v8.m[1][1] = a1->mScale.vy;
	v8.m[2][2] = a1->mScale.vz;

	MulMatrix0(a2, &v8, &v7);

	for (i32 i = 0; i < 3; i++)
	{
		for(i32 j = 0; j < 3; j++)
		{
			a2->m[i][j] = v7.m[i][j];
		}

	}
}

// @Ok
// @Matching
void M3dMaths_CopyMat(MATRIX* a1, MATRIX* a2)
{
	memcpy(reinterpret_cast<void*>(a2), reinterpret_cast<void*>(a1), 3*3*2);
}


unsigned char byte_54D347 = 1;
// @Ok
void M3dAsm_ProcessPolys(unsigned int*, SVECTOR*, int)
{
	if ( !byte_54D347 )
		stubbed_printf("stubbed out: void M3dAsm_ProcessPolys(Uint32 *pFace, SVECTOR *Normals, int NumFaces)");
}


// @Ok
void M3dAsm_SetTransVector(VECTOR* a1)
{
	translationVector.vx = a1->vx;
	translationVector.vy = a1->vy;
	translationVector.vz = a1->vz;
}


// @Ok
// @Matching
MATRIX* RotMatrixYXZ(SVECTOR *a1, MATRIX *a2)
{
	float rx = (float)a1->vx * 0.0015360969118773937f;
	float sx = (float)sin(rx);
	float cx = (float)cos(rx);

	float ry = (float)a1->vy * 0.0015360969118773937f;
	float sy = (float)sin(ry);
	float cy = (float)cos(ry);

	float rz = (float)a1->vz * 0.0015360969118773937f;
	float sz = (float)sin(rz);
	float cz = (float)cos(rz);

	float t1 = sz * sy;
	float t2 = cz * cy;
	a2->m[0][0] = (t1 * sx + t2) * 4096.0f;

	float t3 = cz * sy;
	float t4 = sz * cy;
	a2->m[0][1] = (t3 * sx - t4) * 4096.0f;

	a2->m[0][2] = (sy * cx) * 4096.0f;
	a2->m[1][0] = (sz * cx) * 4096.0f;
	a2->m[1][1] = (cz * cx) * 4096.0f;
	a2->m[1][2] = (-sx) * 4096.0f;
	a2->m[2][0] = (t4 * sx - t3) * 4096.0f;
	a2->m[2][1] = (t2 * sx + t1) * 4096.0f;
	a2->m[2][2] = (cy * cx) * 4096.0f;

	return a2;
}

// @Ok
MATRIX* M3dMaths_RotMatrixYXZ(SVECTOR *a1, MATRIX *a2)
{
	return RotMatrixYXZ(a1, a2);
}


// @Ok
int ratan2(int x, int y)
{
	if (!y)
	{
		if (x < 0)
		{
			return -1024;
		}
		else if (x > 0)
		{
			return 1024;
		}
		else
		{
			print_if_false(0, "x and y are both zero (ratan2)");
			return 0;
		}
	}

	return atan2((f64)x, (f64)y) * 651.0006103515625;

}


// @Ok
// @Matching
i32 GetClut(int, int a2)
{
	return a2 - gClutRelated;
}

// Researched 2026-08-31 via Hex-Rays decompile of 0x0046ECB0: this is a non-zoned sibling
// of M3dAsm_LineColijPreprocessItemsZoned. Same mFlags&0x21/mInquiry shortcut and CItem
// linked-list walk (mNextItem at offset 0x20, not the ppItem array Zoned uses), and it
// calls gsub_46EA20/gsub_46EB30 the same way Zoned now does. It also builds and uses the
// item's rotation: if mAngles (offset 0x14, u16/u16 pair read as one dword) is nonzero, it
// calls two more not-yet-decompiled helpers, sub_46D1E0 (0x0046D1E0, builds a 3x3 matrix
// from the angles) and sub_46CFC0 (0x0046CFC0, matrix*vector transform, called twice on
// the box corners), then re-sorts the transformed corners with gsub_46E990 before the
// gsub_46EA20/gsub_46EB30 calls. Leaving this as a forward stub: it needs those two new
// leaf helpers decompiled first (leaf-first rule), and the whole function is large (the
// Hex-Rays pseudocode alone is much bigger than Zoned). Good next candidate once
// sub_46D1E0/sub_46CFC0 are done.
// @BIGTODO
void M3dAsm_LineColijPreprocessItems(CItem* pItem, i32 ModelTable, SLineInfo* pInfo, u16 Inquiry)
{
	typedef void (*func_ptr)(CItem*, i32, SLineInfo*, u16);

	func_ptr func = (func_ptr)0x0046ECB0;

	func(pItem, ModelTable, pInfo, Inquiry);
}

// idb_globals.txt: DCFatalError @ 0x6150E4
i32 DCFatalError;

// @Ok
// @Matching
void DCSetFatalError(i32 a1)
{
	DCFatalError = a1;

	if (PCGfx_IsInScene())
	{
		PCGfx_EndScene(1);

		// same address as gsub_430880 (nullsub_3), declared and defined in
		// PCShell.cpp; extern here, cast to accept the (unused) dummy arg this
		// call site passes, so it is a real cross-TU direct call.
		extern void gsub_430880(void);
		((void(*)(i32))gsub_430880)(4);
	}

	GameFMV_StopFMV();
	Redbook_XAExit();
	SFX_ShutDown();
	DCPad_ShutDownVibrations();
	PCTex_ReleaseAllTextures();

	if (DCFatalError == 2)
	{
		// same address as gsub_430880 (nullsub_3), declared and defined in
		// PCShell.cpp; extern here so this call is a real cross-TU direct
		// call instead of an indirect register call.
		extern void gsub_430880(void);
		gsub_430880();
	}
	else
	{
		sbExitSystem();

		// same address as buIsReady (pcdcBkup.h) but called here with no argument
		// pushed, so it is probably a different function whose body got folded
		// into buIsReady's at link time. Cast to a real cross-TU direct call.
		extern i32 buIsReady(i32);
		((void(*)(void))buIsReady)();
	}

	exit(0);
}

// @Ok
INLINE void DCInitSinCosTable(void)
{
	for (i32 i = 0; i < FLATBIT_VELOCITIES_SIZE; i++)
	{
		f64 v9 = (f64)i * 0.001536096911877394;
		rcossin_tbl[i].sin = sin(v9) * 4096.0;
		rcossin_tbl[i].cos = cos(v9) * 4096.0;
	}

}

EXPORT i32 gBroadcastMode = 0x38;
EXPORT i32 gDisplayModeRelated;
EXPORT i32 gDisplayModeRelatedTwo = 2;
EXPORT u8 gEuropeVersion;

// @Ok
INLINE u8 IsForEurope(void)
{
	return gEuropeVersion;
}

// @Ok
void Port_InitAtStart(void)
{
	gBroadcastMode = 0x38;
	gDisplayModeRelated = 0;
	gDisplayModeRelatedTwo = 1;

	i32 v1 = syCblCheck();
	i32 v2 = 0;

	switch (syCblCheck())
	{
		case 0:
		case 2:
			v2 = syCblCheckBroadcast();
			switch (v2)
			{
				case 0:
				case 2:
					print_if_false(IsForEurope() == 0, "NTSC TV on European version.");
					gBroadcastMode = 0x38;
					break;
				case 1:
				case 3:
					print_if_false(IsForEurope(), "Pal TV on non-European version.");
					gBroadcastMode = 0x7A;
					break;
				default:
					print_if_false(0, "invalid broadcast enumeration value.");
					break;
			}
			break;
		case 1:
			gBroadcastMode = 0x31;
			break;
		default:
			print_if_false(0, "invalid cable enumeration value.");
			break;
	}

	sbInitSystem(gBroadcastMode, gDisplayModeRelated, gDisplayModeRelatedTwo);
	if (!v1 && (v2 == 1 || v2 == 3))
	{
		kmSetPALEXTCallback(reinterpret_cast<void*>(0x46CD70), 0);
		kmSetDisplayMode(122, 0, 1, 0);
	}
	gPortRelatedOne = reinterpret_cast<i32>(syMalloc(gSomeSize));
	print_if_false(gPortRelatedOne != 0, "Out of system memory.");
	DCInitSinCosTable();
}

// @Ok
void Port_Exit(void)
{
	sbExitSystem();
}


void validate_MATRIX(void){
	VALIDATE_SIZE(MATRIX, 0x20);
	VALIDATE(MATRIX, m, 0x0);
	VALIDATE(MATRIX, t, 0x14);
}

void validate_SMatrix(void)
{
	VALIDATE_SIZE(SMatrix, 0x18);
	VALIDATE(SMatrix, m, 0x0);
	VALIDATE(SMatrix, t, 0x12);
}

void validate_SJoint(void)
{
	VALIDATE_SIZE(SJoint, 0xC);

	VALIDATE(SJoint, Angles, 0x0);
	VALIDATE(SJoint, Displacement, 0x6);
}

void validate_SLink(void)
{
	VALIDATE_SIZE(SLink, 0xC);

	VALIDATE(SLink, Part, 0x0);
	VALIDATE(SLink, ParentPart, 0x2);
	VALIDATE(SLink, Pivot, 0x4);
	VALIDATE(SLink, ParentLink, 0xA);
}

#include "my_patch.h"

// @Bogus
void patch_ps2funcs(void)
{
	PATCH_PUSH_RET(0x0046D430, M3dMaths_SquareRoot0);
	PATCH_PUSH_RET(0x0046D500, M3dMaths_MulDiv64);
	PATCH_PUSH_RET(0x0046E730, M3dMaths_RotMatrixYXZ);
}
