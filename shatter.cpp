#include "shatter.h"
#include "utils.h"

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

// @NotOk
// residue: control flow / register allocation still differs a lot from the original, see
// shatter.attempts.md. Logic matches (verified by hand tracing the original disassembly:
// count==0 stores color's low 3 bytes directly, otherwise averages table[] lookups of
// color's bytes 0/1/2, plus byte 3 too when mode==4, dividing by 3 or 4).
// Address confirmed 2026-08-27: 0x48CEB0 (tools/functions/4771504.bin, 314 bytes). It has no
// entry in tools/names.json (sits unnamed between Split 0x48C730 and Shatter_Item 0x48CFF0);
// found by hand-disassembling that gap and matching the writes to gShatterColor's fields
// (0x6A7684/5/6) plus the count==0/mode==4/else branch shapes against this function.
// cmpsum.sh 0x48CEB0 "?CalcRGB@@YAXHIHPAI@Z" . reports 85 mnemonic diffs from the top
// (register/push-order allocation only, same class of residue as before; not re-attempted
// this session since CalcRGB is not one of this session's assigned stubs).
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

// @MEDIUMTODO
// Investigated 2026-08-27, not decompiled: address 0x48D0F0, 1216 bytes
// (tools/functions/4772080.bin). Disassembled in full. Builds a local SLineInfo via
// M3dColij_InitLineInfo/M3dZone_LineToItem, computes a shard center via CVector operator+
// (0x4E7720, fine) and operator/= (0x4E7680, fine) and normal via VectorNormal, then
// allocates and constructs a CGlassBit (bit.cpp, already @Ok @AlmostMatching) inside an SEH
// frame (mov [fs:0],esp prologue -> the local CGlassBit has a nontrivial destructor). Not
// attempted as real source for two reasons: (1) it calls CVector operator>> (0x4E7840, at
// least 4 times) and operator- (0x4E7760, at least 3 times), both confirmed wrongly INLINE
// in vector.h (see CLAUDE.md, discovered in this file already) so our build can never emit
// the matching out-of-line `call` instructions; (2) bit.cpp's Bit_MakeSpriteRing shows the
// same "new CXxxBit()" SEH-construction shape is already a known-hard, still-open case (14
// source-shape hypotheses tried, still @NotOk on a much smaller function) - a fresh attempt
// here is very unlikely to land inside the effort available this session. Left as a stub
// rather than push a guaranteed-mismatching implementation.
void Shatter_Glass(i32,CVector const *,CVector const *,CVector const *,CVector const *,u8,u8,u8)
{
    printf("Shatter_Glass(i32,CVector const *,CVector const *,CVector const *,CVector const *,u8,u8,u8)");
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
