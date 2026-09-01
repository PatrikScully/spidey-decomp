#include "post.h"
#include "bit.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "db.h"

EXPORT i32 gWaterEffect;
EXPORT i32 gPostTimerRelated;
EXPORT i32 gPostWaterEffect;
EXPORT u16 gPostWordRelated;

EXPORT u8 gPaletteProcessingPaused;

EXPORT i32 gPostSpideyLogoRelated;
EXPORT u32 gPostPauseRelated;


// @Ok
// @Matching
void Post_DoPauseDisplayListProcessing(void)
{
	if (gPaletteProcessingPaused)
		Post_SpideyLogo();
}

// @Ok
// @Matching
void Post_DoPausePaletteProcessing(void)
{
	gPaletteProcessingPaused = 1;
	gPostSpideyLogoRelated = 0;
	gPostPauseRelated = Db_SkyColor;
}

// @Ok
// @Matching
void Post_PostProcessEffects(void)
{
	if (gWaterEffect)
	{
		gPostTimerRelated = 48 * gTimerRelated;
		Post_WaterEffect();
	}
	if (gPostWordRelated)
	{
		--gPostWordRelated;
	}
}

// Investigated 2026-08-31, left as a stub (not tractable in the time
// available, documenting findings per repo policy rather than guessing).
// Real PC address found via IDA xrefs on gPaletteProcessingPaused/
// gPostSpideyLogoRelated/gPostPauseRelated: 0x46A3E0, size 0x8db (2267
// bytes, matches the Mac prototypes.json size of 2372 for
// "Post_SpideyLogo(void)" closely enough to be the same function). Not in
// names.json and no tools/functions/*.bin entry, so there is no ground
// truth byte blob checked into the repo for it either; this was found
// straight from the exe.
// The call site (Post_DoPauseDisplayListProcessing, unnamed in names.json
// as sub_46ACC0/0x46ACC0, tagged @Ok in this file even though its body is
// just "if (gPaletteProcessingPaused) Post_SpideyLogo();") already matches
// byte-for-byte with this printf stub in place, because compare.py masks
// call targets: "call <stub>" and "call <real function>" look identical at
// the mnemonic/operand level it checks. That is why the gap was invisible
// until traced by hand.
// Why it is hard: the function draws the "SPIDER-MAN" logo lettering as a
// sequence of individually shaped textured quads, reading per-letter
// mesh/UV records out of a raw data table (off_54ED9C in the disassembly)
// with 3 record shapes selected by a leading tag byte (3, 1, 0), plus two
// small lookup tables (word_54ECBC/word_54ECBE, 512 entries each,
// apparently a sin/cos-style angle-to-offset table) and a sprintf-style
// helper (sub_46CB90) that formats each letter's glyph index into a
// per-quad UV rect. None of this data table format exists anywhere else
// in the repo (it is not SLineInfo, not POLY_FT4, not an SAnimFrame
// table); decompiling this function correctly needs the actual byte
// layout of that table, which is not something IDA's decompiler recovers
// on its own and is not written down anywhere in this codebase. Guessing
// a layout risks producing code that "looks plausible" but draws garbage
// or crashes on the real letter data. Leaving as a stub rather than
// guessing, per the "don't guess, document and move on" rule.
// Re-verified 2026-09-01 with a fresh idalib decompile of 0x46A3E0: confirms every
// finding above field-for-field. The record walk reads a tag byte (3 or 1) off
// off_54ED9C to pick between a 24-byte record (4 UV pairs, via sub_507910 called
// twice per record) and a 20-byte record (3 UV pairs), both indexing word_54ECBC/
// word_54ECBE (angle/offset tables) by a per-vertex byte read out of the same
// unknown table, and both formatting a per-letter glyph string through sub_46CB90
// (format string byte_56EB54) before submitting geometry via sub_507910 (a 21-argument
// draw-primitive call, itself not decompiled anywhere in this repo). None of
// off_54ED9C's record layout, byte_56EB54's format string content, or sub_507910's
// role is confirmed anywhere else in the codebase; implementing this would mean
// guessing a whole undocumented per-letter mesh table format, which the acceptance
// bar for this session explicitly says not to do. Left as a stub.
// @MEDIUMTODO
INLINE void Post_SpideyLogo(void)
{
    printf("Post_SpideyLogo(void)");
}

// @Ok
// @Matching
void Post_UndoPausePaletteProcessing(void)
{
	if (gPaletteProcessingPaused)
		gPaletteProcessingPaused = 0;
}

// @Ok
// @Matching
// @Note: getting the v1 and v2 assignment was insane I had wrong code thank god I checked the size
INLINE void Post_WaterEffect(void)
{
	if (!gPostWaterEffect)
	{
		i32 v1 = rcossin_tbl[(gPostTimerRelated / 2) & 0xFFF].sin >> 6;
		i32 v2 = rcossin_tbl[(gPostTimerRelated / 2) & 0xFFF].cos >> 6;

		PCGfx_UseTexture(1, DCGfx_BlendingMode_1);
		f32 v3 = gGameResolutionY;
		f32 a3 = Yres;
		f32 v8 = v3 / a3 * 240.0f;
		f32 a3a = gGameResolutionX;

		f32 v4 = Xres;
		f32 a3b = a3a / v4 * 512.0f;

		PCGfx_DrawQuad2D(
			0.0,
			0.0,
			a3b,
			v8,
			0.0,
			0.0,
			1.0,
			1.0,
			(my_abs(v1) + 64) << 8 | (my_abs(v2) + 64) << 16 | 0x30000032,
			6.0,
			0);
	}
}
