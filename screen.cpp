#include "screen.h"
#include "db.h"
#include "panel.h"
#include "camera.h"
#include "ps2funcs.h"
#include "psx_types.h"
#include "mem.h"
#include "utils.h"
#include "PCShell.h"
#include "SpideyDX.h"
#include "m3dinit.h"
#include "PCGfx.h"

// stru_56F224: a fixed MATRIX the original loads into the GTE rotation
// register before projecting the target position (0x56F224).
static MATRIX * const gTargetRotMatrix = (MATRIX*)0x56F224;


EXPORT bool gScreenTarget;


EXPORT CVector gTargetRelated;
EXPORT u16 gTargetOne;
EXPORT u16 gTargetTwo;

EXPORT i32 gCircularFadeRelated;
EXPORT i32 gCircularFadeRelatedOne;
EXPORT i32 gCircularFadeRelatedTwo;

EXPORT u8 gCircularFadeRelatedThree;
EXPORT u8 gCircularFadeRelatedFour;


// guess: pointer to some "current view/clip" record, only known from the disasm (loaded
// as a pointer from this fixed address, then two u16 fields at +8/+0xA read as a Y-range
// clip test). No idb_globals.txt entry for this address, name and layout are our guess.
#define G_VIEW_CLIP_INFO (*reinterpret_cast<u8**>(0x0064E514))

// @Ok
void Screen_DrawArrow(void)
{
	if (!gScreenTarget)
		return;

	u32 *next = pPoly + 15;
	if ((u8*)next >= PolyBufferEnd)
		return;
	POLY_F3 *tri = (POLY_F3*)pPoly;
	pPoly = next;

	VECTOR relPos;
	relPos.vx = (gTargetRelated.vx >> 12) - gMikeCamera[0].Position.vx;
	relPos.vy = (gTargetRelated.vy >> 12) - gMikeCamera[0].Position.vy;
	relPos.vz = (gTargetRelated.vz >> 12) - gMikeCamera[0].Position.vz;

	gte_ldlv0(&relPos);
	gte_rtps();
	i32 stlv[3];
	gte_stlvnl2(stlv);

	u8 *clip = G_VIEW_CLIP_INFO;
	u16 clipMin = *(u16*)(clip + 8);
	u16 clipMax = *(u16*)(clip + 0xA);
	if ((u32)relPos.vy < clipMin || (u32)relPos.vy > clipMax)
		return;

	i32 sxy;
	gte_stsxy(&sxy);
	i32 screenX = (i16)sxy;
	i32 screenY = (i16)(sxy >> 16);

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: setPolyF3");

	tri->code = 0xA0;
	tri->x0 = (i16)screenX;
	tri->y0 = (i16)screenY;
	tri->x1 = (i16)(screenX - 12);
	tri->y1 = (i16)(screenY - 12);
	tri->x2 = (i16)(screenX + 12);
	tri->y2 = (i16)(screenY - 12);

	gsub_46CB90((void*)0x0056EB54);

	POLY_F4 *quad = (POLY_F4*)((u8*)tri + 0x14);
	quad->code = 0xA0;

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: setPolyF4");

	quad->x0 = (i16)(screenX - 6);
	quad->x1 = (i16)(screenX - 6);
	quad->y0 = (i16)(screenY - 12);
	quad->y2 = (i16)(screenY - 12);
	quad->x2 = (i16)(screenX + 6);
	quad->x3 = (i16)(screenX + 6);
	quad->y1 = (i16)(screenY - 24);
	quad->y3 = (i16)(screenY - 24);

	gsub_46CB90((void*)0x0056EB54);
}

// @Ok
// (0x0048AA90, 925 bytes). Draws the 4-triangle target reticle. The IDB
// decompiler mangled the FPU scaling; the logic is: project the target with
// the GTE, then for each of 4 triangles build 3 corners from rcossin_tbl
// offsets around the screen x/y and scale by gGameResolution/Yres,Xres.
void Screen_DrawTarget(void)
{
	if (!gScreenTarget)
		return;

	gte_SetRotMatrix(gTargetRotMatrix);
	m3d_ZeroTransVector();

	VECTOR relPos;
	relPos.vx = (gTargetRelated.vx >> 12) - gMikeCamera[0].Position.vx;
	relPos.vy = (gTargetRelated.vy >> 12) - gMikeCamera[0].Position.vy;
	relPos.vz = (gTargetRelated.vz >> 12) - gMikeCamera[0].Position.vz;
	gte_ldlv0(&relPos);
	gte_rtps();

	PCGfx_UseTexture(1, DCGfx_BlendingMode_0);

	u8 *pPolyByte = (u8*)pPoly;
	if (pPolyByte + 80 >= PolyBufferEnd)
		return;

	u8 *pQuad = pPolyByte + 4;  // quad data starts 4 bytes in (header)
	pPoly = (u32*)(pPolyByte + 80);

	i32 screenXY;
	gte_stsxy(&screenXY);
	i16 screenX = (i16)screenXY;
	i16 screenY = (i16)(screenXY >> 16);

	f32 yScale = (f32)gGameResolutionY / (f32)Yres;
	f32 xScale = (f32)gGameResolutionX / (f32)Xres;

	i32 angleA = gTargetTwo;
	i32 angleB = gTargetTwo + 128;
	for (i32 i = 0; i < 4; i++)
	{
		u8 *v = pQuad + i * 20;
		*(u32*)v = 41120;  // 0xA0A0 gray

		i16 c0x = screenX + (((gTargetOne - 12) * rcossin_tbl[angleA & 0xFFF].sin) >> 12);
		i16 c0y = screenY + 320 * (((gTargetOne - 12) * rcossin_tbl[angleA & 0xFFF].cos) >> 12) / 512;
		i16 c1x = screenX + ((gTargetOne * rcossin_tbl[(angleB - 256) & 0xFFF].sin) >> 12);
		i16 c1y = screenY + 320 * ((gTargetOne * rcossin_tbl[(angleB - 256) & 0xFFF].cos) >> 12) / 512;
		i16 c2x = screenX + ((gTargetOne * rcossin_tbl[angleB & 0xFFF].sin) >> 12);
		i16 c2y = screenY + 320 * ((gTargetOne * rcossin_tbl[angleB & 0xFFF].cos) >> 12) / 512;

		*(i16*)(v + 4) = c0x;
		*(i16*)(v + 6) = c0y;
		*(i16*)(v + 8) = c1x;
		*(i16*)(v + 10) = c1y;
		*(i16*)(v + 12) = c2x;
		*(i16*)(v + 14) = c2y;

		f32 p0x = (f32)c0x * xScale;
		f32 p0y = (f32)c0y * yScale;
		f32 p1x = (f32)c1x * xScale;
		f32 p1y = (f32)c1y * yScale;
		f32 p2x = (f32)c2x * xScale;
		f32 p2y = (f32)c2y * yScale;

		PCGfx_DrawQPoly2D(p0x, p0y, 1.0f, 0.0f, 41120, p1x, p1y, 0.0f, 0.0f, 41120,
				p2x, p2y, 0.0f, 1.0f, 41120, p2x, p2y, 1.0f, 1.0f, 41120, 5.0f);

		angleA += 1024;
		angleB += 1024;
	}
}

// Same two addresses as gFrontCameraModeFlagOne/Two in front.cpp (0x56FB78 /
// 0x56FBF4, "meaning unclear without a consumer" per that file's comment).
// Not G_* macros (no shared/hooked state), so a second tentative name in
// this file follows the older "duplicate static address, own name" rule.
// Here they are cleared right before, and set right after, three
// Db_FlipClear()/optimized_unused_garbage() calls bracketing the whole
// palette rebuild, so a "double buffer busy" style flag pair is the best
// guess from this call site.
#define gDbBusyFlagOne (*reinterpret_cast<u8*>(0x0056FB78))
#define gDbBusyFlagTwo (*reinterpret_cast<u8*>(0x0056FBF4))

// @NotOk
// residue: 84 mnemonic diffs (cmpsum), down from 110 on the first attempt.
// 8 hypotheses logged in screen.attempts.md (below the medium-function 15
// minimum, so not @AlmostMatching). Two real bugs found and fixed along the
// way: a missing CSE on the channel-sum expression, and a signed/unsigned
// shift (sar vs shr) from using i32 instead of u32 for the sum locals.
// Main open residue: a dead counter (read from pDoubleBuffer vs &DoubleBuffer[0],
// never used again) that the original keeps alive in a register for 240
// loop iterations without ever touching memory, which a plain local, a
// volatile local, and a real global each reproduce differently but not
// exactly; and register-spill diffs inside the per-pixel loop suggesting
// higher live-range pressure than the original.
void Screen_SepiaFade(void)
{
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: MoveImage");
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");

	Pause(1);
	gDbBusyFlagTwo = 0;
	gDbBusyFlagOne = 0;
	Db_FlipClear();
	gsub_430680();

	void *p = DCMem_New(0x1000, 0, 1, 0, true);
	i32 x = (i32)pDoubleBuffer - (i32)&DoubleBuffer[0];
	void *chunk[4];
	chunk[0] = p;
	p = (u8*)p + 0x400;
	chunk[1] = p;
	p = (u8*)p + 0x400;
	chunk[2] = p;
	p = (u8*)p + 0x400;
	chunk[3] = p;

	i32 i = 0;
	volatile u16 bx = (x != 0) ? 0x100 : 0;

	for (; i < 0xF0; i++, bx++)
	{
		u32 *p = (u32*)chunk[i & 3];

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: StoreImage");

		for (i32 j = 0; j < 0x100; j++)
		{
			u32 d = *p;

			u32 s = ((d >> 10) & 0x1F) + ((d >> 5) & 0x1F) + (d & 0x1F);
			u8 grey1 = (s * 1365) >> 12;
			u8 grey2 = (s * 682) >> 12;
			u32 lo = (d & 0x8000) | (grey1 << 10) | (grey2 << 5) | grey2;

			u32 s2 = ((d >> 26) & 0x1F) + ((d >> 21) & 0x1F) + ((d >> 16) & 0x1F);
			u8 grey1b = (s2 * 1365) >> 12;
			u8 grey2b = (s2 * 682) >> 12;
			u32 hi = (grey1b << 10) | (grey2b << 5) | grey2b;

			*p = (d & 0x80000000) | lo | (hi << 16);
			p++;
		}
	}

	Mem_Delete(chunk[0]);
	Db_FlipClear();
	gsub_430680();
	Db_FlipClear();
	gDbBusyFlagTwo = 1;
	gDbBusyFlagOne = 1;

	G_GAME_FADE = 0x1E005A;
}

// @Ok
void Screen_SetTarget(
		CVector *a1,
		u16 a2,
		i16 a3)
{
	gTargetRelated = *a1;
	gTargetOne = a2;
	gTargetTwo = a3;
}

// @Ok
void Screen_StartCircularFadeIn(i32,i32 a2)
{
	gCircularFadeRelated = 32;
	gCircularFadeRelatedOne = 0;
	gCircularFadeRelatedTwo = a2 << 12;

	gCircularFadeRelatedThree = 1;
	gCircularFadeRelatedFour = 0;
}

// @Ok
// @Matching
void Screen_UpdateFades(void)
{
	if (gCircularFadeRelatedThree)
	{
		gCircularFadeRelatedOne += gCircularFadeRelatedTwo >> 12;
		gCircularFadeRelatedTwo += 3072;
		if ( gCircularFadeRelatedOne >= 640 )
		{
			gCircularFadeRelatedThree = 0;
			return;
		}
	}
	else
	{
		if (!gCircularFadeRelatedFour)
			return;
		gCircularFadeRelatedOne -= gCircularFadeRelatedTwo >> 12;
		if (gCircularFadeRelatedOne <= 0)
		{
			gCircularFadeRelatedFour = 0;
			return;
		}
	}

	screen_DrawCircularFade();
}

// @MEDIUMTODO
INLINE void screen_DrawCircularFade(void)
{
    printf("screen_DrawCircularFade(void)");
}

// @Ok
// @Matching
void Screen_TargetOn(bool value)
{
	gScreenTarget = value;
}
