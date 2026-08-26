#include "screen.h"
#include "db.h"
#include "panel.h"
#include "camera.h"
#include "ps2funcs.h"
#include "psx_types.h"


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

// @NotOk
// residue: 54 mnemonic diffs (from 66 before the G_VIEW_CLIP_INFO fix), all a register
// scheduling mismatch in the relative-position computation (the original loads all 3
// target components plus camera.vx up front, ours loads them in declaration order).
// See screen.attempts.md. Semantics otherwise look right (target reticle: transform the
// target position relative to the camera through the GTE, clip test against a Y range,
// then fill a POLY_F3 (arrow head) + POLY_F4 (arrow shaft) pair from the allocated poly
// buffer; both primitive setup calls are guarded by gPrintStubbed like the rest of this
// stubbed PC port, the other two gsub_46CB90("") calls are not guarded, matching the
// original bytes).
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

// @MEDIUMTODO
void Screen_DrawTarget(void)
{
    printf("Screen_DrawTarget(void)");
}

// @MEDIUMTODO
void Screen_SepiaFade(void)
{
    printf("Screen_SepiaFade(void)");
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
