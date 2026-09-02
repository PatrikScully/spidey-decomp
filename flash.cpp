#include "flash.h"
#include "ps2m3d.h"
#include "db.h"
#include "panel.h"
#include "ps2funcs.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"

// These eleven globals stay repo local. I scanned the whole exe for each
// address and every single reference sits inside the five functions in this
// file, all of which patch_flash hooks (Flash_Display has exactly one caller,
// Display+0x401 at 0x004559A1, and that is the call PATCH_CALL redirects), so
// the exe never touches its own copies once we are hooked. Addresses read out
// of the original: FadeCountdown 0x005FAA64, FlashCountdown 0x005FAA5C,
// Fading 0x005FAA60, CurrentImportance 0x005FAA54, FlashSort 0x005FAA58,
// CurrentR 0x005FAA14, CurrentG 0x005FAA00, CurrentB 0x005FAA04,
// dR 0x005FA9FC, dG 0x005FA9F8, dB 0x005FA9F4 (Flash_Reset 0x0043D800,
// Flash_Screen 0x0043D830 and Flash_Update 0x0043D8C0 store them in that
// order). dB and dG match idb_globals.txt.

// @Ok
EXPORT i32 FadeCountdown;
// @Ok
EXPORT i32 FlashCountdown;
// @Ok
EXPORT i32 Fading;
// @Ok
EXPORT u8 CurrentImportance;
// @Ok
EXPORT i32 FlashSort;

// @Ok
EXPORT u32 CurrentR;
// @Ok
EXPORT u32 CurrentG;
// @Ok
EXPORT u32 CurrentB;

// @Ok
EXPORT u32 dR;
// @Ok
EXPORT u32 dG;
// @Ok
EXPORT u32 dB;

// @Ok
// @Matching
i32 Flash_FadeFinished(void)
{
	return FadeCountdown == 0;
}

// The bump-allocated scratch record buffer here is db.cpp's pPoly/PolyBufferEnd
// (0x56FB04/0x5FCD1C), reached through G_PPOLY/G_POLY_BUFFER_END in db.h. A
// small record is written into it, then immediately used to build a draw call.

// @Ok
// Verified against 0x43D980 (Flash_Display's own original address; the
// call site at 0x4559A1 is the only thing PATCH_CALL redirects, so the
// previous forward-to-original stub was calling real, working original
// code, not itself). Full logic: bail out if there is nothing to flash or
// fade; otherwise bump-allocate a 0x20 byte record from the shared scratch
// buffer (same buffer as front.cpp's menu highlight records), fill it with
// a full screen quad in the game's fixed 512x240 virtual resolution
// (0,0)-(512,0)-(0,240)-(512,240), pack CurrentR/G/B's high byte (bits
// 16-23, since Flash_Screen stores StartR<<16 etc, so that byte is the
// displayable 0-255 component) into a 0x20RRGGBB color (fixed top byte,
// looks like a blend-mode tag rather than a real alpha), and submit it as
// an untextured quad via PCGfx_UseTexture(1, ...)/PCGfx_DrawQPoly2D,
// scaled from the 512x240 reference space to the real screen resolution
// the same way every other 2D draw call in the repo does
// (gGameResolutionX/Y over Xres/Yres, e.g. PCPanel_DrawTexturedPoly,
// panel.cpp, screen.cpp). The record's tag dwords at +0x18/+0x1C are
// written after the draw call (0x1000000 always, then 0xE1000640 while
// Fading else 0xE1000620); nothing in the repo reads this buffer back yet
// (front.cpp notes the same for its own writer), so those two tags are
// reproduced byte for byte but their real meaning is unknown.
void Flash_Display(void)
{
	if (FlashCountdown == 0 && Fading == 0)
		return;

	u8 *rec = reinterpret_cast<u8*>(G_PPOLY);
	u8 *newPos = rec + 0x20;

	if (newPos > G_POLY_BUFFER_END)
		return;

	G_PPOLY = reinterpret_cast<u32*>(newPos);

	if (!gPrintStubbed)
		stubbed_printf("stubbed out: setPolyF4");

	u8 r = static_cast<u8>(CurrentR >> 16);
	u8 g = static_cast<u8>(CurrentG >> 16);
	u8 b = static_cast<u8>(CurrentB >> 16);

	*reinterpret_cast<u8*>(rec + 0x7) |= 2;
	*reinterpret_cast<u8*>(rec + 0x4) = r;
	*reinterpret_cast<u8*>(rec + 0x5) = g;
	*reinterpret_cast<u8*>(rec + 0x6) = b;

	*reinterpret_cast<i16*>(rec + 0x8) = 0;
	*reinterpret_cast<i16*>(rec + 0xA) = 0;
	*reinterpret_cast<i16*>(rec + 0xC) = 512;
	*reinterpret_cast<i16*>(rec + 0xE) = 0;
	*reinterpret_cast<i16*>(rec + 0x10) = 0;
	*reinterpret_cast<i16*>(rec + 0x12) = 240;
	*reinterpret_cast<i16*>(rec + 0x14) = 512;
	*reinterpret_cast<i16*>(rec + 0x16) = 240;

	gsub_46CB90(G_RENDER_BUF);

	PCGfx_UseTexture(1, DCGfx_BlendingMode_1);

	f32 scaleY = static_cast<f32>(G_GAME_RESOLUTION_Y) / static_cast<f32>(G_YRES);
	f32 scaleX = static_cast<f32>(G_GAME_RESOLUTION_X) / static_cast<f32>(G_XRES);

	u32 color = 0x20000000u | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;

	f32 x0 = 0.0f * scaleX;
	f32 y0 = 0.0f * scaleY;
	f32 x1 = 512.0f * scaleX;
	f32 y1 = 0.0f * scaleY;
	f32 x2 = 0.0f * scaleX;
	f32 y2 = 240.0f * scaleY;
	f32 x3 = 512.0f * scaleX;
	f32 y3 = 240.0f * scaleY;

	PCGfx_DrawQPoly2D(
		x0, y0, 0.0f, 0.0f, color,
		x1, y1, 1.0f, 0.0f, color,
		x2, y2, 0.0f, 1.0f, color,
		x3, y3, 1.0f, 1.0f, color,
		7.0f);

	*reinterpret_cast<u32*>(rec + 0x18) = 0x1000000;

	if (Fading != 0)
		*reinterpret_cast<u32*>(rec + 0x1C) = 0xE1000640;
	else
		*reinterpret_cast<u32*>(rec + 0x1C) = 0xE1000620;

	gsub_46CB90(G_RENDER_BUF);
}

// @Ok
// @Matching
void Flash_Reset(void)
{
	FlashCountdown = 0;
	FadeCountdown = 0;
	Fading = 0;
	CurrentImportance = 0;
}

// @Ok
// @Matching
void Flash_Screen(
		u8 StartR,
		u8 StartG,
		u8 StartB,
		i32 Frames,
		u8 Importance,
		i32 Sort)
{
	if (Importance >= CurrentImportance)
	{
		if (Frames)
		{
			CurrentR = StartR << 16;
			CurrentG = StartG << 16;
			CurrentB = StartB << 16;

			dR = (CurrentR) / Frames;
			dG = (CurrentG) / Frames;
			dB = (CurrentB) / Frames;

			FlashCountdown = Frames;
			CurrentImportance = Importance;
			FlashSort = Sort;
		}
	}

}

// @Ok
// @Matching
void Flash_Update(void)
{
	if (Fading)
	{
		if (FadeCountdown)
		{
			if (--FadeCountdown == 0)
			{
				CurrentB = 0xFF0000;
				CurrentG = 0xFF0000;
				CurrentR = 0xFF0000;
			}
			else
			{
				CurrentR += dR;
				CurrentG += dR;
				CurrentB += dR;
			}
		}
	}
	else if (FlashCountdown)
	{
		if (--FlashCountdown == 0)
		{
			CurrentImportance = 0;
		}
		else
		{
			CurrentR -= dR;
			CurrentG -= dG;
			CurrentB -= dB;
		}
	}
}

#include "my_patch.h"

// @Bogus
void patch_flash(void)
{
	PATCH_PUSH_RET(0x0043D820, Flash_FadeFinished);
	PATCH_PUSH_RET(0x0043D800, Flash_Reset);
	PATCH_PUSH_RET(0x0043D830, Flash_Screen);
	PATCH_PUSH_RET(0x0043D8C0, Flash_Update);

	PATCH_CALL(0x004559A1, Flash_Display);
}
