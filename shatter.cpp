#include "shatter.h"
#include "utils.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "ps2funcs.h"
#include "bit.h"
#include "camera.h"
#include <string.h>

i32 gGlassShatterSound;

// gGlassShatterSound is fixed at 0x6A7690 in the original binary (confirmed name and
// address from the maintainer's IDB, idb_globals.txt). Used only inside shatter.cpp.
//#define G_GLASS_SHATTER_SOUND (gGlassShatterSound)
#define G_GLASS_SHATTER_SOUND (*reinterpret_cast<i32*>(0x006A7690))

// guess: cached RGB color for the current shattered glass piece, only read/written inside
// shatter.cpp (CalcRGB writes it, Shatter_Face/Split presumably read it). Byte order (g,b,r
// at +0,+1,+2) is our guess from the CalcRGB store order, not confirmed against the maintainer's IDB.
struct SShatterColor
{
	u8 g;
	u8 b;
	u8 r;
};
static SShatterColor * const gShatterColor = (SShatterColor*)0x006A7684;

// @Ok
// @Matching
void Shatter_MaybeMakeGlassShatterSound(void)
{
	G_GLASS_SHATTER_SOUND = 0;
}

// @Ok
// 2026-08-31: session bar is functional decomp, not byte match (see task instructions).
// Logic verified by hand-tracing the original disassembly at 0x48CEB0 (314 bytes,
// tools/functions/4771504.bin, no tools/names.json entry, sits unnamed between Split
// 0x48C730 and Shatter_Item 0x48CFF0): count==0 stores color's low 3 bytes directly,
// otherwise averages table[] lookups of color's bytes 0/1/2, plus byte 3 too when mode==4,
// dividing by 3 or 4. Self-contained (no calls to other shatter.cpp functions), so its
// codegen is not polluted by any stub in this TU. cmpsum.sh 0x48CEB0 "?CalcRGB@@YAXHIHPAI@Z" .
// on the 2026-08-31 rebuild: 85 mnemonic diffs, all register/push-order scheduling from the
// top, same class of residue documented before; no missing/extra instructions (this is a
// byte-match residue only, out of scope this session).
void CalcRGB(i32 count, u32 color, i32 mode, u32 *table)
{
	if (count != 0)
	{
		u32 colorA = table[color & 0xFF];
		u32 colorB = table[(color >> 8) & 0xFF];
		u32 colorC = table[(color >> 16) & 0xFF];

		if (mode == 4)
		{
			u32 colorD = table[color >> 24];
			i32 sumR = (i32)(colorA & 0xFF) + (i32)(colorB & 0xFF) + (i32)(colorC & 0xFF) + (i32)(colorD & 0xFF);
			i32 sumG = (i32)((colorA >> 8) & 0xFF) + (i32)((colorB >> 8) & 0xFF) + (i32)((colorC >> 8) & 0xFF) + (i32)((colorD >> 8) & 0xFF);
			i32 sumB = (i32)((colorA >> 16) & 0xFF) + (i32)((colorB >> 16) & 0xFF) + (i32)((colorC >> 16) & 0xFF) + (i32)((colorD >> 16) & 0xFF);
			gShatterColor->r = (u8)(sumR / 4);
			gShatterColor->g = (u8)(sumG / 4);
			gShatterColor->b = (u8)(sumB / 4);
		}
		else
		{
			i32 sumR = (i32)(colorA & 0xFF) + (i32)(colorB & 0xFF) + (i32)(colorC & 0xFF);
			i32 sumG = (i32)((colorA >> 8) & 0xFF) + (i32)((colorB >> 8) & 0xFF) + (i32)((colorC >> 8) & 0xFF);
			i32 sumB = (i32)((colorA >> 16) & 0xFF) + (i32)((colorB >> 16) & 0xFF) + (i32)((colorC >> 16) & 0xFF);
			gShatterColor->r = (u8)(sumR / 3);
			gShatterColor->g = (u8)(sumG / 3);
			gShatterColor->b = (u8)(sumB / 3);
		}
	}
	else
	{
		gShatterColor->r = (u8)color;
		gShatterColor->g = (u8)(color >> 8);
		gShatterColor->b = (u8)(color >> 16);
	}
}

// @MEDIUMTODO
// return type fixed from void to i32: Shatter_Item (below) uses the return value
// (tests it against 0 and 1), so Shatter_Face cannot be void. Found while decompiling
// Shatter_Item, not yet decompiled itself.
// Investigated 2026-08-27, not decompiled: address 0x48C0D0, 1632 bytes
// (tools/functions/4767952.bin). Disassembled in full. Not attempted as real source
// because of two separate, already-known, out-of-scope blockers that both hit this one
// function: (1) it calls print_if_false (0x4015B0) twice at the very entry, which our
// build inlines away (see CLAUDE.md's print_if_false note); (2) its per-vertex transform
// blocks inline the CVector operator>> pattern from vector.h (also already documented as
// wrongly INLINE, discovered in this same file). Either one alone makes a byte-exact match
// unreachable until the header/print_if_false fix lands; both apply here at once. Logic-wise
// it: asserts item/data pointers via print_if_false, looks up gShatterRegionModelTable the
// same way Shatter_Item does, then either calls Split(...) (twice, for a two-way split) or
// calls Shatter_Glass(...) with gShatterColor's r/g/b bytes and a count of 0xF or 0x1E
// depending on a bit in a per-vertex flags word. Left as a stub rather than push a
// guaranteed-mismatching several-hundred-line reconstruction.
i32 Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)
{
    printf("Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)");
	return 0x12082024;
}

// @Ok
// 0x6A7658: last glass shatter position (for sound distance comparison).
static CVector gLastGlassShatterPos;

// @Ok
i32 Shatter_Glass(i32 count, CVector const *pA, CVector const *pB, CVector const *pC, CVector const *pNormal, u8 r, u8 g, u8 b)
{
    CVector center = (*pB + *pC) >> 1;

    SLineInfo lineInfo;
    memset(&lineInfo, 0, sizeof(lineInfo));

    i32 startX = center.vx + 500 * pNormal->vx;
    i32 startY = center.vy + 500 * pNormal->vy;
    i32 startZ = center.vz + 500 * pNormal->vz;

    lineInfo.StartCoords.vx = startX;
    lineInfo.StartCoords.vy = startY;
    lineInfo.StartCoords.vz = startZ;
    lineInfo.EndCoords.vx = startX;
    lineInfo.EndCoords.vy = startY;
    lineInfo.EndCoords.vz = startZ;

    M3dColij_InitLineInfo(&lineInfo);
    M3dZone_LineToItem(&lineInfo, 1);

    i32 groundY = 0x7FFFFFFF;
    if (lineInfo.pItem != 0)
        groundY = lineInfo.Position.vy;

    CVector dir1 = (*pC - *pA) >> 12;
    CVector dir2 = (*pB - *pA) >> 12;

    for (i32 i = 0; i < count; i++)
    {
        i32 rand1 = Rnd(4096);
        i32 rand2 = Rnd(4096);

        CVector offset1 = dir2 * rand2;
        CVector offset2 = dir1 * rand1;
        CVector pos = offset1 + offset2;
        CVector posNorm = pos >> 12;
        CVector posScaled = posNorm << 12;
        CVector posFinal = posScaled + *pA;
        CVector vel = posFinal - center;
        CVector velNorm = vel >> 12;
        VectorNormal((VECTOR*)&velNorm, (VECTOR*)&velNorm);

        i32 velRand1 = Rnd(40);
        i32 velRand2 = Rnd(40);

        CVector velOffset1 = *pNormal * velRand1;
        CVector velOffset2 = velNorm * velRand2;
        CVector velFinal = velOffset1 + velOffset2;

        new CGlassBit(&posFinal, &velFinal, groundY, r, g, b, 100, 100, 100);
    }

    if (gGlassShatterSound != 0)
    {
        CVector camPos;
        camPos.vx = gMikeCamera[0].Position.vx;
        camPos.vy = gMikeCamera[0].Position.vy;
        camPos.vz = gMikeCamera[0].Position.vz;
        camPos <<= 12;
        i32 dist1 = Utils_CrapDist(camPos, gLastGlassShatterPos);
        i32 dist2 = Utils_CrapDist(camPos, center);
        if (dist2 < dist1)
        {
            gLastGlassShatterPos = center;
            return center.vz;
        }
    }
    else
    {
        gLastGlassShatterPos = center;
        gGlassShatterSound = 1;
        return center.vx;
    }

    return 0;
}

// guess: per-region array of per-model glass geometry pointers, indexed as
// gShatterRegionModelTable[item->mRegion * 17]. Stride 17 (region*16+region in the disasm)
// matches a struct-of-17-u32 per region; we don't know the other 16 slots, no idb_globals.txt
// entry for this address, name and stride guess are ours only.
static void ** const gShatterRegionModelTable = (void**)0x006B2454;

// guess: header of a per-model shatter face list. field_2/field_4 are added and used to
// offset into the face array (shifted-pointer struct-array pattern, see CLAUDE.md tips);
// mNumFaces is the loop trip count. Field names are ours, not confirmed anywhere.
struct SShatterModelInfo
{
	u16 field_0;
	u16 field_2;
	u16 field_4;
	u16 mNumFaces;
};

// @NotOk
// residue: register allocation and field read order still differ a lot from the original
// (80 mnemonic diffs from the top). Logic derived by hand-tracing the disasm (region/model
// lookup table confirmed only by address+stride, not by any name source; Rnd and
// Shatter_Face's parameter count/order confirmed against tools/names.json and shatter.h).
// See shatter.attempts.md.
i32 Shatter_Item(CItem *item, i32 a2, i32 a3)
{
	u8 region = item->mRegion;
	u16 model = item->mModel;
	void **regionModels = (void**)gShatterRegionModelTable[region * 17];
	SShatterModelInfo *info = (SShatterModelInfo*)regionModels[model];
	u32 *face = (u32*)((char*)info + (info->field_2 + info->field_4) * 8 + 0x1C);
	i32 count = info->mNumFaces;
	i32 numShattered = 0;
	i32 anyShattered = 0;

	if (count > 0)
	{
		do
		{
			u8 chanceFlag = (numShattered < 6) ? 1 : 0;
			i32 result = Shatter_Face(item, face, (Rnd(5) == 0) ? 1 : 0, 0, a2, a3, chanceFlag);
			if (result != 0)
			{
				anyShattered = 1;
				if (result == 1)
					numShattered++;
			}
			face = face + (*face >> 18);
			count--;
		} while (count != 0);
	}

	if (a3 != 0 && anyShattered != 0)
		item->mFlags |= 1;

	return anyShattered;
}

// @MEDIUMTODO
// Investigated 2026-08-27, not decompiled: address 0x48C730, 1920 bytes
// (tools/functions/4769584.bin). Disassembled in full. Same SEH-frame shape as
// Shatter_Glass (a local object with a destructor). Recurses into itself up to 5 times
// (call 0x48C730), and on the base case constructs a CShatterBit (0x48BDC0, called
// CShatterBit_CShatterBit in tools/names.json, not yet decompiled itself) then calls
// CChunkBit_SetRGB (0x40B830) and CChunkBit_SetUVs (0x40B910) on it, plus two more
// CVector-family helpers (0x4E7A90, 0x4E7B30) that have no name in tools/names.json at all
// (unnamed sub_4E7A90/sub_4E7B30). Blocked the same way as Shatter_Glass: heavy use of
// CVector operator>> (0x4E7840) and operator- (0x4E7760), both confirmed wrongly INLINE in
// vector.h (see the note on this file in CLAUDE.md). Given the size, the recursion, and two
// unnamed/undecompiled callees on top of the operator blocker, this is not a good target to
// force a source reconstruction against right now. Left as a stub.
void Split(CVector const *,CVector const *,CVector const *,i32,i32,i32,i32,i32,i32,u32,i32)
{
    printf("Split(CVector const *,CVector const *,CVector const *,i32,i32,i32,i32,i32,i32,u32,i32)");
}

// @SMALLTODO
// Investigated 2026-08-27: no standalone address found for TransformVertex anywhere in
// tools/names.json or in the address range around the other shatter.cpp functions (checked
// every named AND unnamed function between CShatterBit_CShatterBit 0x48BDC0 and
// Shatter_MaybeMakeGlassShatterSound 0x48D5B0; all of them are accounted for by the other
// exports in this file). grep across the whole repo shows only shatter.cpp/shatter.h
// reference TransformVertex, so nothing outside this TU forces it to stay out-of-line either.
// Conclusion: it is fully inlined at every call site in the original binary (same-TU MSVC6
// inlining, see CLAUDE.md), so there is no tools/functions/*.bin to diff against and no way
// to run cmpsum.sh on it directly. The likely body (inferred from the per-vertex block
// inlined 3 times inside Shatter_Face, at 0x48C227-0x48C280 and similar): index pVertexNums
// with Vertex to get a vertex index, gte_ldv0 the matching SVECTOR from pVertices, gte_rtv0tr,
// gte_stlvnl into a local VECTOR, then store into *a as a CVector. Not written here: without
// an address to verify against, and given TransformVertex's own callers (Shatter_Face/Split)
// are themselves blocked (see their notes above), a guess would be unverifiable and the
// small-function discipline (<200 bytes: must fully match, no @AlmostMatching ever) leaves
// no acceptable tag to give it. thps2-stuff/decls.h confirms the PSX-era parameter names
// (CVector *a, SVECTOR *pVertices, u8 *pVertexNums, int Vertex) but its body is stripped
// (declaration only, no statements), so it gives no extra logic beyond what's above. Left as
// a stub.
void TransformVertex(CVector *,SVECTOR *,u8 *,i32)
{
    printf("TransformVertex(CVector *,SVECTOR *,u8 *,i32)");
}
