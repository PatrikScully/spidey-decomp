#include "l5a5lsc.h"

#include "spidey.h"
#include "l1a3bomb.h"
#include "panel.h"
#include "trig.h"
#include "spool.h"
#include "ps2m3d.h"
#include "baddy.h"
#include "powerup.h"
#include "utils.h"

EXPORT u32 gL5A5RelatedTwo;

#define LEN_L5A5_TEXS 4
EXPORT Texture *gL5A5Texs[LEN_L5A5_TEXS];

// tentative name/guess: fixed game address 0x60F76C, the 4 bytes right after
// gAnimWebcart's SAnimFrame* (0x60F760, panel.cpp, @Ok @Matching Panel_Init) and
// before gBombDieRelatedOne (0x60F771, l1a3bomb.h). Read-only here, used as a
// vertical scroll offset for the subway progress bar. Not in idb_globals.txt.
static i32 * const gAnimWebcartYOffset = (i32*)0x0060F76C;

struct SL5A5BarRect
{
	i32 x, y, w, h;
};

// read from the original .rdata at 0x54B43C..0x54B47B (SpideyPC.exe), 4 rects of
// {x, y, w, h}, one per bar piece drawn below (Car, Track, LeftEnd, RightEnd).
static const SL5A5BarRect gL5A5BarRects[4] =
{
	{ -2, 0, 26, 10 },
	{  6, 0, 19, 10 },
	{  7, 2, 16, 10 },
	{  6, 2, 16, 10 },
};

// @Ok
// @Matching
void L5A5LSC_DisplayProgressBar(u32 const *,u32 *)
{
	Texture *pTex;

	pTex = gL5A5Texs[0];
	if (!pTex)
	{
		pTex = gL5A5Texs[0] = Spool_FindTextureEntry("SubwayPanel_Track");
	}
	print_if_false(pTex != 0, "Could not find texture");

	pTex = gL5A5Texs[1];
	if (!pTex)
	{
		pTex = gL5A5Texs[1] = Spool_FindTextureEntry("SubwayPanel_RightEnd");
	}
	print_if_false(pTex != 0, "Could not find texture");

	pTex = gL5A5Texs[2];
	if (!pTex)
	{
		pTex = gL5A5Texs[2] = Spool_FindTextureEntry("SubwayPanel_LeftEnd");
	}
	print_if_false(pTex != 0, "Could not find texture");

	pTex = gL5A5Texs[3];
	if (!pTex)
	{
		pTex = gL5A5Texs[3] = Spool_FindTextureEntry("SubwayPanel_Car");
	}
	print_if_false(pTex != 0, "Could not find texture");

	u8 wasArmed = gBombDieRelatedTwo;
	gBombRelated = 4096;

	if (!wasArmed)
	{
		gBombDieRelatedTwo = 1;
		gBombAIRelated = 9000;
		gBombDieTimerRelated = gTimerRelated;

		if (gWideScreen > 0)
		{
			return;
		}
	}
	else if (!gBombAIRelated)
	{
		Trig_SendPulseToNode(EndLevelNode);
		return;
	}

	POLY_FT4 *poly;
	i32 carOffset = ((9000u - gBombAIRelated) * 176 >> 12) + 0x26;

	poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(gL5A5Texs[3], 0));
	Panel_SetStretchedScreenCoords(
		gL5A5BarRects[0].x + carOffset,
		gL5A5BarRects[0].y - *gAnimWebcartYOffset + 0xD3,
		poly, gL5A5Texs[3], gL5A5BarRects[0].w, gL5A5BarRects[0].h);
	DCPanel_DrawTexturedPoly(1.0f, poly, gL5A5Texs[3], 0);

	for (i32 i = 0; i < 0x1A4; i += 0x1E)
	{
		poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(gL5A5Texs[0], 0));
		Panel_SetStretchedScreenCoords(
			i + gL5A5BarRects[1].x + 0x26,
			gL5A5BarRects[1].y - *gAnimWebcartYOffset + 0xD9,
			poly, gL5A5Texs[0], gL5A5BarRects[1].w, gL5A5BarRects[1].h);
		DCPanel_DrawTexturedPoly(4.0f, poly, gL5A5Texs[0], 0);
	}

	poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(gL5A5Texs[2], 0));
	Panel_SetStretchedScreenCoords(
		gL5A5BarRects[2].x + 0xB,
		gL5A5BarRects[2].y - *gAnimWebcartYOffset + 0xD7,
		poly, gL5A5Texs[2], gL5A5BarRects[2].w, gL5A5BarRects[2].h);
	DCPanel_DrawTexturedPoly(3.0f, poly, gL5A5Texs[2], 0);

	poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(gL5A5Texs[1], 0));
	Panel_SetStretchedScreenCoords(
		gL5A5BarRects[3].x + 0x1CA,
		gL5A5BarRects[3].y - *gAnimWebcartYOffset + 0xD7,
		poly, gL5A5Texs[1], gL5A5BarRects[3].w, gL5A5BarRects[3].h);
	DCPanel_DrawTexturedPoly(3.0f, poly, gL5A5Texs[1], 0);
}

// @Ok
// @AlmostMatching: reorder assignements
void L5A5LSC_RelocatableModuleClear(void)
{
	gBombDieRelatedTwo = 0;
	gBombDieTimerRelated = gTimerRelated;

	for (i32 i = 0; i < LEN_L5A5_TEXS; i++)
	{
		gL5A5Texs[i] = 0;
	}
}

// @Ok
// @AlmostMatching: assignemtn to dietimerelated is slightly off
void L5A5LSC_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = L5A5LSC_RelocatableModuleClear;
	pMod->field_C[0] = L5A5LSC_WiggleSubwayCars;
	pMod->field_C[1] = L5A5LSC_DisplayProgressBar;
	Spidey_SetUserFunction("l5a5lsc", 1u);

	gBombDieRelatedTwo = 0;
	gBombDieTimerRelated = gTimerRelated;
	gL5A5RelatedTwo = 0;
}

// @MEDIUMTODO
void L5A5LSC_WiggleSubwayCars(u32 const *,u32 *)
{
    printf("L5A5LSC_WiggleSubwayCars(u32 const *,u32 *)");
}
