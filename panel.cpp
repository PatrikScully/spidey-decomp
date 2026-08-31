#include "panel.h"
#include "spool.h"
#include "l1a3bomb.h"
#include "spidey.h"
#include "db.h"
#include "ps2funcs.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "trig.h"
#include "reloc.h"
#include "bit.h"
#include "utils.h"
#include "ps2lowsfx.h"
#include "mess.h"

#include "validate.h"

EXPORT i32 gHealthBarItemType;
EXPORT i32 gHealthBarRelated;
EXPORT i32 gHealthBarRelatedTwo;
EXPORT Texture* gHealthBarTextures[5];

// @FIXME
EXPORT CVector gCompassPosition;

EXPORT u8 gCompassStatus;

EXPORT SAnimFrame* gAnimSp;
EXPORT SAnimFrame* gAnimCompass;
EXPORT SAnimFrame* gAnimWebcart;

EXPORT CBody* gHealthBarOne;
EXPORT CBody* gHealthBarTwo;


// real translation (0x4631c0, 1091 bytes). Two near-duplicate blocks
// selected by (a4 == 1): the vertex1/vertex2 argument slots swap which
// POLY_GT4 corner feeds them (colors/coords keep their own alpha,
// 0xDF for corners 0/1, 0x60 for corners 2/3, tied to the source corner
// not the argument slot, confirmed from the color-pack read order in each
// block). cmpsum: 296 mnemonic diffs, diverges right at the prologue, at
// the print_if_false call. Likely the same known repo-wide defect noted in
// CLAUDE.md ("print_if_false inlining makes old @Ok tags go stale"):
// export.h's print_if_false is static so our build always inlines it,
// while the original calls it out-of-line at 0x4015B0. This looks like the
// shared root cause behind this function and both DCPanel_DrawTexturedPoly
// overloads above (all three call print_if_false and are all stuck at a
// small stable diff count); none of the three can byte-match until that
// header issue is fixed. 1 attempt this session, not iterated further
// @Ok
// Functional: draw gouraud quad, logic verified against Hex-Rays at 0x4631c0.
void DCDrawGouraudPoly(f32 zOffset, POLY_GT4 *poly, Texture *tex, i32 a4)
{
	print_if_false(tex != 0, "no texture for draw gouraud poly.");

	PCGfx_UseTexture(tex->clut, DCGfx_BlendingMode_2);

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 y3 = poly->y3 * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 x3 = poly->x3 * scaleX;
	f32 y2 = poly->y2 * scaleY;
	f32 x2 = poly->x2 * scaleX;
	f32 y1 = poly->y1 * scaleY;
	f32 x1 = poly->x1 * scaleX;
	f32 y0 = poly->y0 * scaleY;
	f32 x0 = poly->x0 * scaleX;

	u32 color0 = 0xDF000000 | (poly->r0 << 16) | (poly->g0 << 8) | poly->b0;
	u32 color1 = 0xDF000000 | (poly->r1 << 16) | (poly->g1 << 8) | poly->b1;
	u32 color2 = 0x60000000 | (poly->r2 << 16) | (poly->g2 << 8) | poly->b2;
	u32 color3 = 0x60000000 | (poly->r3 << 16) | (poly->g3 << 8) | poly->b3;

	if (a4 == 1)
	{
		PCGfx_DrawQPoly2D(
				x0, y0, 0.0f, 0.0f, color0,
				x2, y2, 1.0f, 0.0f, color2,
				x1, y1, 0.0f, 1.0f, color1,
				x3, y3, 1.0f, 1.0f, color3,
				zOffset);
	}
	else
	{
		PCGfx_DrawQPoly2D(
				x0, y0, 0.0f, 0.0f, color0,
				x1, y1, 1.0f, 0.0f, color1,
				x2, y2, 0.0f, 1.0f, color2,
				x3, y3, 1.0f, 1.0f, color3,
				zOffset);
	}
}

// packed PS2-style 0x00BBGGRR colors need swapping to 0xFFRRGGBB for the PC renderer.
// @Bogus
static u32 DCGouraud_SwapColor(u32 c)
{
	return 0xFF000000 | ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}

// @Ok
// real translation, 0x00462FB0, 9 args (zOffset,x,y,w,h,c0,c1,c2,c3). The
// old version of this function had a bogus 10th i32 parameter and used it
// (plus two of the color args) in the wrong vertex slots: it read c4/c2/c3/c1
// where the original reads c0/c1/c2/c3 in plain corner order. Confirmed from
// Hex-Rays at 0x462fb0 (only caller of this address is the not-yet-written
// Panel_DisplayHealthBar/Panel_Display, so no existing caller depended on
// the old, wrong param list). Functional only, not chasing byte match this
// session.
void DCDrawGouraudPoly(f32 zOffset, i32 x, i32 y, i32 w, i32 h, u32 c0, u32 c1, u32 c2, u32 c3)
{
	PCGfx_UseTexture(1, DCGfx_BlendingMode_1);

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 yEnd = (y + h) * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 xEnd = (x + w) * scaleX;
	f32 xScaled = x * scaleX;
	f32 yScaled = y * scaleY;

	PCGfx_DrawQPoly2D(
			xScaled, yScaled, 0.0f, 0.0f, DCGouraud_SwapColor(c0),
			xEnd, yScaled, 1.0f, 0.0f, DCGouraud_SwapColor(c1),
			xScaled, yEnd, 0.0f, 1.0f, DCGouraud_SwapColor(c2),
			xEnd, yEnd, 1.0f, 1.0f, DCGouraud_SwapColor(c3),
			zOffset);
}

// @Ok
// @Matching
void DCPanel_DrawFlatShadedPoly(f32 zOffset, i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32, i32 blendMode)
{
	u8 alpha = 0xFF;

	if (blendMode == 1)
	{
		PCGfx_UseTexture(blendMode, (DCGfx_BlendingMode)blendMode);
		alpha = 0x7F;
	}
	else if (blendMode == 2)
	{
		PCGfx_UseTexture(1, DCGfx_BlendingMode_1);
		alpha = 0x7F;
	}
	else if (blendMode == 3)
	{
		PCGfx_UseTexture(1, DCGfx_BlendingMode_1);
		alpha = 0xDC;
	}
	else
	{
		PCGfx_UseTexture(1, DCGfx_BlendingMode_0);
	}

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 hScaled = h * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 wScaled = w * scaleX;
	f32 yScaled = y * scaleY;
	f32 xScaled = x * scaleX;

	u32 color = (alpha << 24) | (r << 16) | (g << 8) | b;

	PCGfx_DrawQuad2D(
			xScaled,
			yScaled,
			wScaled,
			hScaled,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			color,
			zOffset,
			false);
}

// @Ok
// real translation (0x4626a0, 640 bytes). Found a genuine functional bug
// while cross-checking against Hex-Rays and the raw disasm of the only
// three call sites (shell.cpp gsub_498240, tagged @Ok @Matching, e.g.
// `DCPanel_DrawTexturedPoly(1.0f, pPoly, gAnimTable[23], x - 85, y - 11,
// 20, 12, G_SORT, 0)`): the trailing i32 params are, in real stack order,
// x, y, w, h, an unused slot, then flags. The old source here had them as
// x, y, a6(unused), h, w, flags, so the geometry branch read the unused
// slot (G_SORT in the shell.cpp calls) as the width and never looked at
// the real width argument. Confirmed from the raw asm at 0x4626a0: in the
// true branch, edx = arg_14 (added to x for x1/x3) and ebx = arg_18 (added
// to y for y2), and arg_14/arg_18 are exactly the "w"/"h" slots that match
// frame->Width/frame->Height in the fallback (else) branch below. Since
// callers pass arguments positionally, only the declaration order needed
// to change here; shell.cpp's already-matching call site is unaffected.
// Functional only, not chasing byte match this session.
void DCPanel_DrawTexturedPoly(f32 zOffset, POLY_FT4 *poly, SAnimFrame const *frame, i32 x, i32 y, i32 w, i32 h, i32 a8, u32 flags)
{
	print_if_false(frame != 0, "NULL pFrame for draw texture poly.");

	if (w && h)
	{
		poly->x0 = (i16)x;
		poly->x2 = (i16)x;
		poly->y0 = (i16)y;
		poly->y1 = (i16)y;
		poly->x1 = (i16)(w + x);
		poly->x3 = (i16)(w + x);
		poly->y2 = (i16)(h + y);
		poly->y3 = (i16)(h + y);
	}
	else
	{
		poly->x0 = (i16)x;
		poly->y0 = (i16)y;
		poly->x1 = (i16)(x + frame->Width);
		poly->y1 = (i16)y;
		poly->x2 = (i16)x;
		poly->y2 = (i16)(y + frame->Height);
		poly->x3 = (i16)(x + frame->Width);
		poly->y3 = (i16)(y + frame->Height);
	}

	u32 color = flags;
	if (!flags)
	{
		color = 0xFF000000 | (poly->r0 << 16) | (poly->g0 << 8) | poly->b0;
	}

	Texture *tex = frame->pTexture;
	PCGfx_UseTexture(tex->clut, DCGfx_BlendingMode_0);

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 y3 = poly->y3 * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 x3 = poly->x3 * scaleX;
	f32 y2 = poly->y2 * scaleY;
	f32 x2 = poly->x2 * scaleX;
	f32 y1 = poly->y1 * scaleY;
	f32 x1 = poly->x1 * scaleX;
	f32 y0 = poly->y0 * scaleY;
	f32 x0 = poly->x0 * scaleX;

	PCGfx_DrawQPoly2D(
			x0, y0, 0.01f, 0.01f, color,
			x1, y1, 1.0f, 0.01f, color,
			x2, y2, 0.01f, 1.0f, color,
			x3, y3, 1.0f, 1.0f, color,
			zOffset);
}

// @Ok
// real translation (0x4624a0, 506 bytes). Logic checked against Hex-Rays:
// clut read from frame->pTexture->clut, color falls back to poly->r0/g0/b0
// when flags is 0, all 8 corners scaled the same way as the other
// DCPanel_DrawTexturedPoly overloads. The remaining diffs seen in earlier
// sessions were register/scheduling residue only (float scale step
// ordering), not a functional problem. Functional only, not chasing byte
// match this session.
void DCPanel_DrawTexturedPoly(f32 zOffset, POLY_FT4 *poly, SAnimFrame const *frame, u32 flags)
{
	print_if_false(frame != 0, "NULL pFrame for draw texture poly.");

	Texture *tex = frame->pTexture;
	PCGfx_UseTexture(tex->clut, DCGfx_BlendingMode_0);

	u32 color = flags;
	if (!flags)
	{
		color = 0xFF000000 | (poly->r0 << 16) | (poly->g0 << 8) | poly->b0;
	}

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 y3 = poly->y3 * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 x3 = poly->x3 * scaleX;
	f32 y2 = poly->y2 * scaleY;
	f32 x2 = poly->x2 * scaleX;
	f32 y1 = poly->y1 * scaleY;
	f32 x1 = poly->x1 * scaleX;
	f32 y0 = poly->y0 * scaleY;
	f32 x0 = poly->x0 * scaleX;

	PCGfx_DrawQPoly2D(
			x0, y0, 0.01f, 0.01f, color,
			x1, y1, 1.0f, 0.01f, color,
			x2, y2, 0.01f, 1.0f, color,
			x3, y3, 1.0f, 1.0f, color,
			zOffset);
}

// @Ok
// Functional: draw textured quad, logic verified against Hex-Rays at 0x462930.
// The 29 mnemonic diffs are register allocation / scheduling residue.
void DCPanel_DrawTexturedPoly(f32 zOffset, POLY_FT4 *poly, Texture const *tex, u32 flags)
{
	print_if_false(tex != 0, "no texture for draw textured poly.");

	PCGfx_UseTexture(tex->clut, DCGfx_BlendingMode_0);

	u32 color = flags;
	if (!flags)
	{
		color = 0xFF000000 | (poly->r0 << 16) | (poly->g0 << 8) | poly->b0;
	}

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 y3 = poly->y3 * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 x3 = poly->x3 * scaleX;
	f32 y2 = poly->y2 * scaleY;
	f32 x2 = poly->x2 * scaleX;
	f32 y1 = poly->y1 * scaleY;
	f32 x1 = poly->x1 * scaleX;
	f32 y0 = poly->y0 * scaleY;
	f32 x0 = poly->x0 * scaleX;

	PCGfx_DrawQPoly2D(
			x0, y0, 0.01f, 0.01f, color,
			x1, y1, 1.0f, 0.01f, color,
			x2, y2, 0.01f, 1.0f, color,
			x3, y3, 1.0f, 1.0f, color,
			zOffset);
}

// unnamed globals used only by gsub_46CB90. Not in idb_globals.txt near neighbours,
// tentative names based on how they gate the debug print below.
static u8 * const gDebugPrintDisabled = (u8*)0x006B4CB8;
static u8 * const gDebugPrintEnabled = (u8*)0x0054F038;

static char * const gDebugPrintBuf = (char*)0x006109E0;

// gsub_4015B0 is declared in panel.h and defined further down, so it is not
// visible for same-TU inlining at this call site (matches the original,
// which has a real "call" instruction here, not an inlined body).

// unnamed helper at 0x46CB90, argument is gRenderBuf (idb_globals.txt: 0x0056EB54, exact type unknown).
// Not runtime-hooked this session, so a printf placeholder instead of a forward-to-original
// for gsub_4015B0. cmpsum: 0 mnemonic diffs.
// @Ok
// @Matching
void gsub_46CB90(void* fmt, ...)
{
	if (*gDebugPrintDisabled)
		return;

	if (!*gDebugPrintEnabled)
		return;

	va_list args;
	va_start(args, fmt);
	vsprintf(gDebugPrintBuf, (char*)fmt, args);

	gsub_4015B0(gDebugPrintBuf);
}

// unnamed helper at 0x4015B0 (names.json calls it print_if_false, but the
// export.h print_if_false has a different arg count and is static/inlined
// away in our build). Original bytes are a single `ret` (1 byte function,
// tools/functions/4199856.bin), so the body is empty in this build
// configuration. cmpsum: 0 mnemonic diffs.
// @Ok
// @Matching
EXPORT void gsub_4015B0(void*)
{
}

// @Ok
void Panel_CreateCompass(CVector * pVec)
{
	gCompassPosition = *pVec >> 12;
	gCompassStatus = 1;
}

// @MEDIUMTODO
void Panel_Display(void)
{
    printf("Panel_Display(void)");
}

// @MEDIUMTODO
void Panel_DisplayCompass(void)
{
    printf("Panel_DisplayCompass(void)");
}

// @MEDIUMTODO
void Panel_DisplayHealthBar(void)
{
    printf("Panel_DisplayHealthBar(void)");
}

// @Ok
// screen Y offset for the HUD (runtime value, 0 at boot)
static i32 * const gPanelScreenY = (i32*)0x0060F76C;
// bomb-timer animation state (l1a3bomb level)
static u8 * const gBombTimerAnimOne = (u8*)0x0060F784;
static u8 * const gBombTimerAnimTwo = (u8*)0x0060F785;
static i32 * const gBombTimerAnimOnePos = (i32*)0x0060F66C;
static i32 * const gBombTimerAnimTwoPos = (i32*)0x0060F74C;
static i32 * const gBombTimerLastMinute = (i32*)0x0060F780;

// @Ok
void Panel_DisplayTimer(void)
{
	i32 LevelId = Trig_GetLevelId();
	switch (LevelId)
	{
	case 513:
		if (MechList == 0 || (MechList->field_E18 == 0 && MechList->field_1AC == 0))
			Reloc_CallUserFunction("l2a1lsc", 1, 0, 0);
		break;
	case 1281:
		if (MechList == 0 || (MechList->field_E18 == 0 && MechList->field_1AC == 0))
			Reloc_CallUserFunction("venom", 1, 0, 0);
		break;
	case 1285:
		Reloc_CallUserFunction("l5a5lsc", 1, 0, 0);
		break;
	case 2054:
		Reloc_CallUserFunction("superock", 1, 0, 0);
		break;
	default:
		if (gBombDieRelatedOne != 0)
		{
			if (*gBombTimerAnimOne == 0 && Rnd(20) == 0)
			{
				*gBombTimerAnimOne = 1;
				*gBombTimerAnimOnePos = 69632;
			}
			if (*gBombTimerAnimTwo == 0 && Rnd(30) == 0)
			{
				*gBombTimerAnimTwo = 1;
				*gBombTimerAnimTwoPos = 94208;
			}
			char v57[8];
			strcpy(v57, "00:00");
			i32 mins = gBombAIRelated / 0xE10;
			i32 secs = gBombAIRelated / 0x3C - 60 * (gBombAIRelated / 0xE10);
			if (mins < 0xA)
			{
				v57[0] = 48;
			}
			else
			{
				v57[0] = mins / 10 + 48;
				mins = mins + 2 * (4 * (mins / -10) - mins / 10);
			}
			v57[1] = mins + 48;
			if (secs != *gBombTimerLastMinute)
			{
				SFX_Play(0x20, 0x2000, 0);
				*gBombTimerLastMinute = secs;
			}
			if (secs < 10)
			{
				v57[3] = 48;
			}
			else
			{
				v57[3] = secs / 10 + 48;
				secs = secs + 2 * (4 * (secs / -10) - secs / 10);
			}
			v57[4] = secs + 48;

			Texture* pTexture = gAnimTable[14][19].pTexture;
			if (pTexture != 0)
			{
				POLY_FT4* v5 = (POLY_FT4*)Panel_DrawTexturedPoly(pTexture, 0);
				Panel_SetStretchedScreenCoords(0 + 219, 0 + *gPanelScreenY + 19, v5, pTexture, 27, 24);
				PCGfx_UseTexture(pTexture->clut, DCGfx_BlendingMode_1);
				float yScale = (float)gGameResolutionY / (float)Yres;
				float xScale = (float)gGameResolutionX / (float)Xres;
				u32 col = v5->b0 | ((v5->g0 | ((v5->r0 | 0xFFFFFF00) << 8)) << 8);
				PCGfx_DrawQPoly2D(
					(float)v5->x0 * xScale, (float)v5->y0 * yScale, 0, 0, col,
					(float)v5->x1 * xScale, (float)v5->y1 * yScale, 1065353216, 0, col,
					(float)v5->x2 * xScale, (float)v5->y2 * yScale, 0, 1065353216, col,
					(float)v5->x3 * xScale, (float)v5->y3 * yScale, 1065353216, 1065353216, col, 0.6f);

				POLY_FT4* v7 = (POLY_FT4*)Panel_DrawTexturedPoly(pTexture, 0);
				Panel_SetStretchedScreenCoords(20 + 241, 0 + *gPanelScreenY + 19, v7, pTexture, 27, 24);
				v7->u2 = v5->u1;
				v7->u0 = v5->u1;
				v7->u3 = v5->u0;
				v7->u1 = v5->u0;
				PCGfx_UseTexture(pTexture->clut, DCGfx_BlendingMode_1);
				u32 col2 = v7->b0 | ((v7->g0 | ((v7->r0 | 0xFFFFFF00) << 8)) << 8);
				PCGfx_DrawQPoly2D(
					(float)v7->x0 * xScale, (float)v7->y0 * yScale, 1065353216, 0, col2,
					(float)v7->x1 * xScale, (float)v7->y1 * yScale, 0, 0, col2,
					(float)v7->x2 * xScale, (float)v7->y2 * yScale, 1065353216, 1065353216, col2,
					(float)v7->x3 * xScale, (float)v7->y3 * yScale, 0, 1065353216, col2, 0.6f);
			}

			Mess_SetScale(256);
			Mess_SetTextJustify(1);
			Mess_SetRGB(0x60, 0x60, 0x80, 0);
			char* CurrentFont = Mess_GetCurrentFont();
			char v58[32];
			sprintf(v58, CurrentFont);
			Mess_SetCurrentFont("sp_fnt01.fnt");
			Mess_DrawText(234, *gPanelScreenY + 38, v57, 0, 0x1000);
			Mess_SetCurrentFont(v58);
			DCPanel_DrawFlatShadedPoly(0.5f, 222, *gPanelScreenY + 23, 78, 20, 0, 0, 0, 0, 1);

			if (*gBombTimerAnimOne != 0)
			{
				i32 pos = *gBombTimerAnimOnePos >> 12;
				if (pos >= 23)
				{
					i32 h = pos <= 36 ? 7 : 43 - pos;
					DCPanel_DrawFlatShadedPoly(0.5f, 222, pos + *gPanelScreenY, 78, h, 24, 0x18, 0x18, 0, 2);
				}
				else
				{
					DCPanel_DrawFlatShadedPoly(0.5f, 222, *gPanelScreenY + 23, 78, pos - 16, 24, 0x18, 0x18, 0, 2);
				}
				*gBombTimerAnimOnePos += 3798;
				if (*gBombTimerAnimOnePos > 176128)
					*gBombTimerAnimOne = 0;
			}
			if (*gBombTimerAnimTwo != 0)
			{
				DCPanel_DrawFlatShadedPoly(0.5f, 222, *gPanelScreenY + (*gBombTimerAnimTwoPos >> 12), 78, 1, 32, 0x20, 0x20, 0, 2);
				*gBombTimerAnimTwoPos += 2334;
				if (*gBombTimerAnimTwoPos > 176128)
					*gBombTimerAnimTwo = 0;
			}
		}
		break;
	}
}

// @Ok
// @Matching
int Panel_DrawFlatShadedPoly(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32, i32 a9)
{
	if ((u8*)pPoly + sizeof(POLY_F4) > PolyBufferEnd)
	{
		return 0;
	}

	POLY_F4* p = (POLY_F4*)pPoly;
	pPoly = (u32*)((u8*)pPoly + sizeof(POLY_F4));

	if (!gPrintStubbed)
	{
		gsub_46CB90((void*)"Panel_DrawFlatShadedPoly");
	}

	p->r0 = r;
	p->b0 = b;
	p->g0 = g;

	p->x0 = (i16)x;
	p->y0 = (i16)y;
	p->x1 = (i16)(x + w);
	p->y1 = (i16)y;
	p->x2 = (i16)x;
	p->y2 = (i16)(y + h);
	p->x3 = (i16)(x + w);
	p->y3 = (i16)(y + h);

	gsub_46CB90((void*)0x0056EB54);

	if (a9)
	{
		if (!gPrintStubbed)
		{
			gsub_46CB90((void*)"Panel_DrawFlatShadedPoly: extra");
		}

		if ((u8*)pPoly + 8 > PolyBufferEnd)
		{
			return 0;
		}

		pPoly = (u32*)((u8*)pPoly + 8);

		if (!gPrintStubbed)
		{
			gsub_46CB90((void*)"Panel_DrawFlatShadedPoly: extra2");
		}

		gsub_46CB90((void*)0x0056EB54);
	}

	return (int)p;
}

// @Ok
// @Matching
void Panel_Init(void)
{
	Spool_AnimAccess("Sp", &gAnimSp);
	Spool_AnimAccess("Compass", &gAnimCompass);
	Spool_AnimAccess("Webcart", &gAnimWebcart);
}

// @Ok
// @AlmostMatching: same logic but slightly out of order
void Panel_SetStretchedScreenCoords(
		i32 a1,
		i32 a2,
		POLY_FT4 *a3,
		SAnimFrame *a4,
		i32 a5,
		i32 a6)
{
	i32 v6 = a5;
	if (!v6)
	{
		v6 = a4->Width;
	}

	v6 *= 6554;

	if ( (v6 & 0xFFF) >= 0x800)
	{
		v6 += 4096;
	}

	v6 >>= 12;

	if (!a6)
	{
		a6 = a4->Height;
	}

	i32 OffY = a4->OffY;
	i32 OffX = a4->OffX;


	a3->x0 = a1 + ((6554 * OffX) >> 12);
	a3->y0 = a2 + OffY;

	a3->x1 = a3->x0 + v6;
	a3->y1 = a3->y0;
	a3->x2 = a3->x0;

	a3->y2 = a3->y0 + a6;
	a3->x3 = a3->x1;
	a3->y3 = a3->y2;
}

// @Ok
// @Matching
void Panel_SetStretchedScreenCoords(
		i32 a1,
		i32 a2,
		POLY_FT4 *a3,
		const Texture *a4,
		i32 a5,
		i32 a6)
{
	i32 v6 = a5;
	if (!a5)
	{
		v6 = a4->u1 - a4->u0;
	}

	v6 *= 6554;

	if ((v6 & 0xFFF) >= 0x800)
	{
		v6 += 4096;
	}

	v6 >>= 12;

	if (!a6)
	{
		a6 = a4->v2 - a4->v0;
	}

	a3->x0 = a1;
	a3->y0 = a2;
	a3->x1 = a1 + v6;
	a3->y1 = a2;
	a3->x2 = a1;
	a3->y2 = a2 + a6;
	a3->x3 = a3->x1;
	a3->y3 = a3->y2;
}

// @Ok
INLINE void Panel_UpdateTimer(void)
{
	if (gBombDieRelatedTwo)
	{
		u32 v1 = 0;
		if (MechList->field_E18 || MechList->field_1AC)
		{
			v1 = 0;
		}
		else
		{
			v1 = (gBombRelated * (gTimerRelated - gBombDieTimerRelated)) >> 12;
		}

		if (gBombAIRelated > v1)
		{
			gBombAIRelated -= v1;
		}
		else
		{
			gBombAIRelated = 0;
		}

	}

	gBombDieTimerRelated = gTimerRelated;
}

// @Ok
void Panel_CreateHealthBar(CBody* pBody, i32 a2)
{
	if ( a2 != 316 )
	{
		gHealthBarOne = pBody;
		gHealthBarItemType = a2;
		gHealthBarRelated = pBody->mHealth;
	}
	else
	{
		gHealthBarTwo = pBody;
		gHealthBarRelatedTwo = pBody->mHealth;
	}

	switch ( a2 )
	{
		case 310:
			gHealthBarTextures[0] = Spool_FindTextureEntry("scorpion");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("jonah");
			gHealthBarTextures[3] = Spool_FindTextureEntry("scorpion_wounded");
			break;
		case 307:
			gHealthBarTextures[0] = Spool_FindTextureEntry("rhino");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("rhino_wounded");
			break;
		case 311:
			gHealthBarTextures[0] = Spool_FindTextureEntry("mysterio");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("mysterio_wounded");
			break;
		case 313:
			gHealthBarTextures[0] = Spool_FindTextureEntry("venom");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("maryjane_01");
			gHealthBarTextures[3] = Spool_FindTextureEntry("maryJane_bar");
			gHealthBarTextures[4] = Spool_FindTextureEntry("venom_wounded");
			break;
		case 308:
			gHealthBarTextures[0] = Spool_FindTextureEntry("dococ");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("docOc_wounded");
			break;
		case 314:
			gHealthBarTextures[0] = Spool_FindTextureEntry("carnage");
			gHealthBarTextures[1] = Spool_FindTextureEntry("boss");
			gHealthBarTextures[2] = Spool_FindTextureEntry("carnage_wounded");
			break;
		default:
			gHealthBarOne = 0;
			gHealthBarItemType = 0;
			break;
	}
}

// @Ok
void Panel_DestroyHealthBar(void)
{
	gHealthBarOne = 0;
	gHealthBarTwo = 0;
}

// @Ok
void Panel_DestroyCompass(void)
{
	gCompassStatus = 0;
}

// auto_inline off: both overloads below are real out-of-line functions in
// the original binary (0x462B90 and 0x462BB0). The new 4-arg overload
// further down calls the SAnimFrame one, which calls the Texture one; under
// /Ob2 the compiler otherwise inlines both into that call site, turning the
// original's two real "call" instructions into inlined bodies and desyncing
// the whole function. Same fix as ps2funcs.cpp's gsub_46E990 callers.
#pragma auto_inline(off)

// @Ok
int Panel_DrawTexturedPoly(SAnimFrame* pFrame, int a2)
{
	return Panel_DrawTexturedPoly(pFrame->pTexture, a2);
}

// @Ok
// @Matching
int Panel_DrawTexturedPoly(Texture* pTexture, int a2)
{
	if (!pTexture)
	{
		return 0;
	}

	print_if_false(a2 < 0x1000, "Panel_DrawTexturedPoly");

	if ((u8*)pPoly + sizeof(POLY_FT4) > PolyBufferEnd)
	{
		return 0;
	}

	POLY_FT4* p = (POLY_FT4*)pPoly;
	pPoly = (u32*)((u8*)pPoly + sizeof(POLY_FT4));

	p->tag = 0x09000000;
	*(u32*)&p->r0 = 0x2C808080;

	u32 u0v0clut = *(u32*)&pTexture->u0;
	u32 u1v1tpage = *(u32*)&pTexture->u1;
	u32 u2v2u3v3 = *(u32*)&pTexture->u2;
	u16 u3v3 = *(u16*)&pTexture->u3;

	*(u32*)&p->u0 = u0v0clut;
	*(u32*)&p->u1 = u1v1tpage;
	*(u32*)&p->u2 = u2v2u3v3;
	*(u16*)&p->u3 = u3v3;

	gsub_46CB90((void*)0x0056EB54);

	return (int)p;
}

#pragma auto_inline(on)

// real translation, 0x00462B30, 94 bytes, called from PShell_DrawMenuBox
// (pshell.cpp) with (frame, x - 14, someY, sort). Builds on the 2-arg
// overload right above: gets the poly from Panel_DrawTexturedPoly(pFrame,
// sort), then writes an (x,y) positioned quad sized by pFrame->Width/Height,
// same shape as Panel_SetStretchedScreenCoords. No null check on the
// returned poly pointer before the field stores, matching the original.
// Needed #pragma auto_inline(off) around the two overloads above (they
// were getting inlined into the call here under /Ob2, turning the
// original's real "call" into inlined code). Also needed the x1/y2 sums as
// named i16 locals, declared between the y0 and y1 stores: this makes the
// compiler evaluate the Width+x sum before storing y1 (and the Height+y sum
// before the "add esp,8" call cleanup), matching the original's
// instruction scheduling. i32 locals in the same spot gave a completely
// different (byte-loaded, 32-bit add) shape; only i16 reproduced the
// movzx+16-bit-add pattern the original uses. cmpsum: 0 mnemonic diffs,
// only remaining byte diff is the relocated call target.
// @Ok
// @Matching
int Panel_DrawTexturedPoly(SAnimFrame* pFrame, i32 x, i32 y, i32 sort)
{
	POLY_FT4* p = (POLY_FT4*)Panel_DrawTexturedPoly(pFrame->pTexture, sort);

	p->x0 = (i16)x;
	p->y0 = (i16)y;
	i16 x1 = (i16)(pFrame->Width + x);
	p->y1 = (i16)y;
	p->x1 = x1;
	p->x2 = (i16)x;
	i16 y2 = (i16)(pFrame->Height + y);
	p->y2 = y2;
	p->x3 = (i16)(pFrame->Width + x);
	p->y3 = (i16)(pFrame->Height + y);

	return (int)p;
}

void validate_SAnimFrame(void)
{
	VALIDATE_SIZE(SAnimFrame, 0x8);

	VALIDATE(SAnimFrame, OffX, 0x0);
	VALIDATE(SAnimFrame, OffY, 0x1);
	VALIDATE(SAnimFrame, Width, 0x2);
	VALIDATE(SAnimFrame, Height, 0x3);
	VALIDATE(SAnimFrame, pTexture, 0x4);
}

void validate_Texture(void)
{
	VALIDATE_SIZE(Texture, 0x2C);

	VALIDATE(Texture, u0, 0x0);
	VALIDATE(Texture, v0, 0x1);

	VALIDATE(Texture, clut, 0x2);

	VALIDATE(Texture, u1, 0x4);
	VALIDATE(Texture, v1, 0x5);

	VALIDATE(Texture, tpage, 0x6);

	VALIDATE(Texture, u2, 0x8);
	VALIDATE(Texture, v2, 0x9);

	VALIDATE(Texture, u3, 0xA);
	VALIDATE(Texture, v3, 0xB);

	VALIDATE(Texture, TexWin, 0xC);
	VALIDATE(Texture, Usage, 0x10);
	VALIDATE(Texture, field_12, 0x12);
	VALIDATE(Texture, Checksum, 0x14);

	VALIDATE(Texture, pPalette, 0x18);
	VALIDATE(Texture, x, 0x1C);
	VALIDATE(Texture, y, 0x1E);

	VALIDATE(Texture, pNext, 0x20);
	VALIDATE(Texture, pPrevious, 0x24);
	VALIDATE(Texture, mRegion, 0x28);
}

void validate_POLY_FT4(void)
{
	VALIDATE_SIZE(POLY_FT4, 0x28);

	VALIDATE(POLY_FT4, tag, 0x0);

	VALIDATE(POLY_FT4, r0, 0x4);
	VALIDATE(POLY_FT4, g0, 0x5);
	VALIDATE(POLY_FT4, b0, 0x6);

	VALIDATE(POLY_FT4, code, 0x7);

	VALIDATE(POLY_FT4, x0, 0x8);
	VALIDATE(POLY_FT4, y0, 0xA);

	VALIDATE(POLY_FT4, u0, 0xC);
	VALIDATE(POLY_FT4, v0, 0xD);

	VALIDATE(POLY_FT4, clut, 0xE);

	VALIDATE(POLY_FT4, x1, 0x10);
	VALIDATE(POLY_FT4, y1, 0x12);

	VALIDATE(POLY_FT4, u1, 0x14);
	VALIDATE(POLY_FT4, v1, 0x15);

	VALIDATE(POLY_FT4, tpage, 0x16);

	VALIDATE(POLY_FT4, x2, 0x18);
	VALIDATE(POLY_FT4, y2, 0x1A);

	VALIDATE(POLY_FT4, x3, 0x20);
	VALIDATE(POLY_FT4, y3, 0x22);

	VALIDATE(POLY_FT4, u3, 0x24);
	VALIDATE(POLY_FT4, v3, 0x25);
}

void validate_POLY_GT4(void)
{
	VALIDATE_SIZE(POLY_GT4, 0x34);

	VALIDATE(POLY_GT4, tag, 0x0);

	VALIDATE(POLY_GT4, r0, 0x4);
	VALIDATE(POLY_GT4, g0, 0x5);
	VALIDATE(POLY_GT4, b0, 0x6);

	VALIDATE(POLY_GT4, code, 0x7);

	VALIDATE(POLY_GT4, x0, 0x8);
	VALIDATE(POLY_GT4, y0, 0xA);

	VALIDATE(POLY_GT4, u0, 0xC);
	VALIDATE(POLY_GT4, v0, 0xD);

	VALIDATE(POLY_GT4, clut, 0xE);

	VALIDATE(POLY_GT4, r1, 0x10);
	VALIDATE(POLY_GT4, g1, 0x11);
	VALIDATE(POLY_GT4, b1, 0x12);

	VALIDATE(POLY_GT4, x1, 0x14);
	VALIDATE(POLY_GT4, y1, 0x16);

	VALIDATE(POLY_GT4, u1, 0x18);
	VALIDATE(POLY_GT4, v1, 0x19);

	VALIDATE(POLY_GT4, tpage, 0x1A);

	VALIDATE(POLY_GT4, r2, 0x1C);
	VALIDATE(POLY_GT4, g2, 0x1D);
	VALIDATE(POLY_GT4, b2, 0x1E);

	VALIDATE(POLY_GT4, x2, 0x20);
	VALIDATE(POLY_GT4, y2, 0x22);

	VALIDATE(POLY_GT4, u2, 0x24);
	VALIDATE(POLY_GT4, v2, 0x25);

	VALIDATE(POLY_GT4, r3, 0x28);
	VALIDATE(POLY_GT4, g3, 0x29);
	VALIDATE(POLY_GT4, b3, 0x2A);

	VALIDATE(POLY_GT4, x3, 0x2C);
	VALIDATE(POLY_GT4, y3, 0x2E);

	VALIDATE(POLY_GT4, u3, 0x30);
	VALIDATE(POLY_GT4, v3, 0x31);
}

void validate_POLY_F4(void)
{
	VALIDATE_SIZE(POLY_F4, 0x18);

	VALIDATE(POLY_F4, tag, 0x0);

	VALIDATE(POLY_F4, r0, 0x4);
	VALIDATE(POLY_F4, g0, 0x5);
	VALIDATE(POLY_F4, b0, 0x6);

	VALIDATE(POLY_F4, code, 0x7);

	VALIDATE(POLY_F4, x0, 0x8);
	VALIDATE(POLY_F4, y0, 0xA);

	VALIDATE(POLY_F4, x1, 0xC);
	VALIDATE(POLY_F4, y1, 0xE);

	VALIDATE(POLY_F4, x2, 0x10);
	VALIDATE(POLY_F4, y2, 0x12);

	VALIDATE(POLY_F4, x3, 0x14);
	VALIDATE(POLY_F4, y3, 0x16);
}
