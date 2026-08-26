#include "shatter.h"

i32 gGlassShatterSound;

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
void Shatter_MaybeMakeGlassShatterSound(void)
{
	gGlassShatterSound = 0;
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
void Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)
{
    printf("Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32)");
}

// @MEDIUMTODO
void Shatter_Glass(i32,CVector const *,CVector const *,CVector const *,CVector const *,u8,u8,u8)
{
    printf("Shatter_Glass(i32,CVector const *,CVector const *,CVector const *,CVector const *,u8,u8,u8)");
}

// @SMALLTODO
i32 Shatter_Item(CItem *,i32,i32)
{
    printf("Shatter_Item(CItem *,i32,i32)");
	return 0x12082024;
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
