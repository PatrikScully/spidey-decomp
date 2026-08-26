#include "m3dinit.h"
#include "bit.h"
#include "validate.h"
#include "dcmodel.h"
#include "pcdcMem.h"
#include "mem.h"
#include "PCGfx.h"

u32 M3d_FadeColour;

EXPORT i16 WibbleTables[1024];
u32 Xres;
u32 Yres;

EXPORT i32 PixelAspectX;
EXPORT i32 PixelAspectY;

// One entry per DC region: a heap block holding an array of these items.
// Item size 0x24 confirmed by the pointer stride in DCClearRegion's original code.
// Field meanings beyond pData/field_C are unknown.
struct SDCRegionItem
{
	void *pData;

	PADDING(8);

	u8 field_C;

	PADDING(0x17);
};

// Unknown globals, tentative names. Only used from this file, no header entry.
// Nearest named neighbours (idb_globals.txt): gPushOffsetOne at 0x5F6718 (before),
// gGlobalSkaterModel at 0x5F6808 (dcmodel.cpp) which sits right after this array
// ends, so this table is probably around 41 pointers long.
// Declared i32 (not a pointer type): MSVC6 will not fold a "pointer to pointer"
// tentative global to a plain immediate address, an i32 does.
// gDCRegionItemCounts is volatile: the original re-reads it on every access
// (loop bottom check) instead of caching the value in a register.
static i32 * const gDCRegionItems = (i32 *)0x5F6764;
static volatile i32 * const gDCRegionItemCounts = (volatile i32 *)0x5F6860;
static i32 * const gDCRegionItemTotal = (i32 *)0x5F7298;

// @NotOk
// Residue: register allocation / prologue scheduling only, 40 mnemonic diffs,
// same instruction COUNT and semantics, all downstream of one root cause.
// The original computes the addresses of gDCRegionItems[a1] and
// gDCRegionItemCounts[a1] once each (lea into ebp/edi) and pushes ebp/esi in
// the prologue before the null check even though the early-return path does
// not need them; our build always recomputes both via SIB addressing off a1
// (kept in edi/edx) and never lifts an address into a dedicated register
// until partway into the item-freeing loop, so it only pushes ebp there.
// Both versions end up using the same 4 non-volatile registers overall
// (ebx, esi, edi, ebp), just assigned/scheduled differently.
// Tried and rejected (11 attempts, each rebuilt and diffed against the
// original): 1) gDCRegionItems as void** (MSVC6 never folds a
// pointer-to-pointer literal address to an immediate, always materializes a
// real pointer variable and loads it, confirmed with an isolated cl.exe
// test); 2) gDCRegionItems as i32* with casts at each use (fixes #1, folds
// to a plain immediate, this is the version kept); 3) explicit
// "i32 *pSlot = &gDCRegionItems[a1];" local, dereferenced everywhere (CSE'd
// away completely, identical to plain indexing); 4) gDCRegionItemCounts
// declared volatile (reproduces the per-access reload the original does,
// and does make ebp appear caching a value, closest result, kept); 5) same
// but gDCRegionItems also volatile (no change from #4); 6) explicit
// volatile-qualified slot pointers for both arrays (identical to #4, still
// CSE'd to the same shape); 7) count check folded into the for-loop
// condition instead of a separate "i32 count" local (regressed, lost the
// ebp caching entirely); 8) volatile slot pointers plus a separate "count"
// local (identical to #4); 9) gDCRegionItems non-volatile with #4's counts
// handling (identical output to #4, dropped the redundant volatile); 10)
// composed "if (a && b)" flag check rewritten as nested ifs per tips.txt
// (identical machine code, only label numbers changed); 11) wrapped the
// whole body in "if (pRegion) {...}" instead of an early return (identical
// prologue, no change). None of these change which register the compiler
// picks to hold a live address versus recomputing it from a1, which is an
// internal MSVC6 scheduling choice, not something these source shapes
// control (same class of issue as the Utils_VblankProcessing CSE case in
// CLAUDE.md's Matching tricks section).
void DCClearRegion(i32 a1)
{
	SDCRegionItem *pRegion = (SDCRegionItem *)gDCRegionItems[a1];

	if (pRegion == NULL)
		return;

	if ((pRegion->field_C & 0x10) && gDCRegionItemCounts[a1])
		DCClearSkater();

	i32 count = gDCRegionItemCounts[a1];
	SDCRegionItem *pBase = (SDCRegionItem *)gDCRegionItems[a1];

	if (count > 0)
	{
		SDCRegionItem *pItem = pBase;
		i32 i = 0;

		do
		{
			if (pItem->pData)
				Mem_Delete2(pItem->pData);

			pItem->pData = NULL;

			i++;
			pItem++;
		} while (i < gDCRegionItemCounts[a1]);
	}

	gDCRegionItemTotal[0] -= gDCRegionItemCounts[a1];
	syFree(pBase);
	gDCRegionItemCounts[a1] = 0;
	gDCRegionItems[a1] = (i32)NULL;
}

// @Ok
void M3dInit_InitAtStart(void)
{
	M3dInit_SetResolution(0x200, 0xF0);
	M3dInit_SetFoggingParams(0, 6000, 0x800);

	i16 *pWibble = &WibbleTables[0];
	for (i32 i = 0; i < 16; i++)
	{
		for (i32 j = 0; j < 4096; j+=64)
		{
			*pWibble = (i * rcossin_tbl[j].sin) >> 4;
			pWibble++;
		}
	}

	M3d_FadeColour = 0x80000000;
}

// @MEDIUMTODO
void M3dInit_ParsePSX(i32)
{
    printf("M3dInit_ParsePSX(i32)");
}

// Fog transition state. Tentative names, no idb_globals.txt entries for these
// addresses. gFogFar/gFogNear are the current (persistent, cross-call) fog
// distances; gFogFarRate/gFogNearRate are per-step deltas used to animate a
// transition to a new target over gFogTransitionSteps calls (elsewhere, not
// in this file). gFogRangeShift/gFogRangeShiftFinal hold a power-of-two shift
// count so the range normalises to around 0x1000. gFogColorIsWhite flags
// whether M3d_FadeColour's RGB is fully white (0xFFFFFF after channel swap).
static i32 * const gFogTransitionSteps = (i32 *)0x64E558;
static i32 * const gFogNear = (i32 *)0x64E560;
static i32 * const gFogFar = (i32 *)0x64E568;
static i32 * const gFogFarRate = (i32 *)0x6191D8;
static i32 * const gFogNearRate = (i32 *)0x628600;
static i32 * const gFogRangeShift = (i32 *)0x61B5DC;
static i32 * const gFogRangeShiftFinal = (i32 *)0x5FC1E4;
static i32 * const gFogNearCopy = (i32 *)0x5FC1E0;
static i32 * const gFogFarCopy = (i32 *)0x5FC1DC;
static i32 * const gFogColorIsWhite = (i32 *)0x54D384;

// @NotOk
// Residue: 81 mnemonic diffs, same operations and same semantics (verified by
// hand against the original disassembly instruction by instruction), pure
// register allocation / prologue scheduling. The original pushes ebx, esi,
// edi ALL early (before the power-of-two check even runs) and never needs a
// 4th callee-saved register; our build always needs one more (ebp), which it
// uses first to hold Min across both branches of the transition-vs-snap if,
// then recycles for the two literal-0 stores (gFogTransitionSteps=0 and
// gFogRangeShift=0) later in the function. 9 hypotheses tried, each rebuilt
// and diffed: 1) direct translation matching the disassembly's apparent
// shape; 2) moved the gFogTransitionSteps store to the top of the transition
// branch (matches the original's instruction order there); 3) added a named
// "oldFar" local for the pre-existing gFogFar value (matches ebx being
// loaded before the divide); 4) swapped the print_if_false condition to
// "Range == (Range & -Range)" instead of the reverse (matches the original's
// "cmp esi,eax" operand order, confirmed with an isolated cl.exe test); 5)
// introduced curFar/curNear as function-scope locals shared by both branches
// so the tail gFogNearCopy/gFogFarCopy stores reuse the same register the
// branches wrote instead of re-reading the globals (this is what pulled in
// the extra ebp, since Min is now referenced on both branches); 6) changed
// the color channel-swap formula from "|" to "+" between the mask and the
// shifted/byte parts, since the original uses "add" not "or" for combining
// non-overlapping bit fields; 7) changed the first gFogRangeShift store from
// "= shift" to a literal "= 0" to try to stop the compiler sharing one
// zero register between it and the transition-steps store (no effect); 8)
// split the packed-color computation into named "color"/"byte2" locals
// matching the original's single M3d_FadeColour read reused for both parts;
// 9) tried bundling "shift" into the same top-of-function declaration as
// curFar/curNear (isolated cl.exe test only, regressed to needing an even
// earlier push edi with no other benefit, reverted). Confirmed via an
// isolated cl.exe test that print_if_false is NOT the inlining problem
// documented elsewhere in CLAUDE.md for this call site (it compiles to a
// real out-of-line call here, not inlined). The parameter order for the
// PCGfx_SetFogParams call (far*scale, near*scale, packedColor) was derived
// by hand-tracing the stack slot reuse in the original (the function reuses
// its own now-dead Min argument slot as scratch for the two float
// conversions instead of doing "sub esp,N"), and matches once cross-checked
// against PCGfx_SetFogParams's own (already @Ok @Matching) definition. Same
// class of hard-to-reproduce register-allocation/prologue-timing residue as
// DCClearRegion above and the documented Utils_VblankProcessing CSE case.
void M3dInit_SetFoggingParams(long Dummy, long Min, u32 Range)
{
	print_if_false(Range == (Range & -Range), "Fogging range must be a power of two");

	i32 curFar, curNear;

	if (Dummy > 0)
	{
		curFar = *gFogFar;

		*gFogTransitionSteps = Dummy;
		*gFogFarRate = (Min - curFar) / Dummy;

		curNear = *gFogNear;
		*gFogNearRate = (Min + (i32)Range - curNear) / Dummy;
	}
	else
	{
		curFar = Min;
		curNear = Min + Range;

		*gFogTransitionSteps = 0;
		*gFogFar = curFar;
		*gFogNear = curNear;
	}

	i32 shift = 0;
	*gFogRangeShift = 0;

	if (Range < 0x1000)
	{
		do
		{
			Range <<= 1;
			shift++;
		} while (Range < 0x1000);

		*gFogRangeShift = shift;
	}

	if (Range > 0x1000)
	{
		do
		{
			Range >>= 1;
			shift--;
		} while (Range > 0x1000);

		*gFogRangeShift = shift;
	}

	*gFogRangeShiftFinal = shift;

	u32 color = M3d_FadeColour;
	u32 byte2 = ((u8 *)&M3d_FadeColour)[2];
	u32 packed = (color & 0xFF00FF00) + ((byte2 + ((color & 0xFF) << 16)));

	*gFogNearCopy = curNear;
	*gFogFarCopy = curFar;

	if ((packed & 0xFFFFFF) == 0xFFFFFF)
	{
		*gFogColorIsWhite = 1;
		PCGfx_SetFogParams((f32)*gFogFar * 100.0f, (f32)*gFogNear * 100.0f, packed);
	}
	else
	{
		*gFogColorIsWhite = 0;
		PCGfx_SetFogParams((f32)*gFogFar * 0.98f, (f32)*gFogNear * 0.98f, packed);
	}
}

// @Ok
INLINE void M3dInit_SetResolution(u32 X,u32 Y)
{
	if (Y << 2 < X * 3)
	{
		Xres = X;
		Yres = Y;
		PixelAspectX = 0x1000;
		PixelAspectY = (X * 0x3000) / (Y << 2);
		return;
	}

	Xres = X;
	Yres = Y;
	PixelAspectX = (Y << 0xE) / (X * 3);
	PixelAspectX = 0x1000;
}

// @SMALLTODO
void alloc_dc_models(i32,i32)
{
    printf("alloc_dc_models(i32,i32)");
}

// @SMALLTODO
void setup_pulsing_colors(i32)
{
    printf("setup_pulsing_colors(i32)");
}

// @Ok
// Hated this piece of shit, memoery accesses are fucked up on the original for some reason
// there might be a hidden struct or smth, it's not the first time I see something like this
// where pointers to the middle of the struct are used
void M3dInit_FlagZeroWibbles(STexWibItemInfo *pTexWibItemInfo)
{
	print_if_false(pTexWibItemInfo != NULL, "NULL pTexWibItemInfo");

	STexWibItemInfo *v1 = pTexWibItemInfo;
	STexWibItemInfo *v3;
	unsigned int v2;

	while (v1->ItemOffset.Full)
	{
		v2 = 0;

		v1->ZeroUAmplitudes = 1;
		v2 = v1->field_C.Full;
		v1->ZeroVAmplitudes = 1;

		v3 = v1 + 1;


#define AmplitudeCheck(x, arg) {\
	int tmp = 0;\
	tmp = (x);\
	if ( ((int)((tmp) & 0xFFFFFFF0)) > ((int)0x00000050) ) {\
		v1->arg = 0; }\
}

#define UAmplitudeCheck(x) AmplitudeCheck(x, ZeroUAmplitudes)
#define VAmplitudeCheck(x) AmplitudeCheck(x, ZeroVAmplitudes)

		while (v2)
		{
			UAmplitudeCheck(v3->ItemOffset.Byte[2]);
			VAmplitudeCheck(v3->ItemOffset.Byte[3]);

			UAmplitudeCheck(v3->field_6);
			VAmplitudeCheck(v3->field_7);

			UAmplitudeCheck(v3->field_8);
			VAmplitudeCheck(v3->field_9);


			UAmplitudeCheck(v3->ZeroUAmplitudes);
			VAmplitudeCheck(v3->ZeroVAmplitudes);
				
			v3++;
			v2--;
		}

		v1 = v3;

	}
}

void validate_STexWibItemInfo()
{
	VALIDATE_SIZE(STexWibItemInfo, 0x10);

	VALIDATE(STexWibItemInfo, ItemOffset, 0x0);

	VALIDATE(STexWibItemInfo, field_6, 0x6);
	VALIDATE(STexWibItemInfo, field_7, 0x7);
	VALIDATE(STexWibItemInfo, field_8, 0x8);
	VALIDATE(STexWibItemInfo, field_9, 0x9);

	VALIDATE(STexWibItemInfo, field_C, 0xC);

	VALIDATE(STexWibItemInfo, ZeroUAmplitudes, 0xE);
	VALIDATE(STexWibItemInfo, ZeroVAmplitudes, 0xF);
}
