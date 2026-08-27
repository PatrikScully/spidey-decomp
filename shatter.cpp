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
i32 Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)
{
    printf("Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)");
	return 0x12082024;
}

// @MEDIUMTODO
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
void Split(CVector const *,CVector const *,CVector const *,i32,i32,i32,i32,i32,i32,u32,i32)
{
    printf("Split(CVector const *,CVector const *,CVector const *,i32,i32,i32,i32,i32,i32,u32,i32)");
}

// @SMALLTODO
void TransformVertex(CVector *,SVECTOR *,u8 *,i32)
{
    printf("TransformVertex(CVector *,SVECTOR *,u8 *,i32)");
}
