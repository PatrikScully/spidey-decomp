#include "front.h"
#include "validate.h"
#include "utils.h"
#include "mess.h"
#include "ps2funcs.h"
#include "trig.h"
#include "ps2pad.h"
#include "powerup.h"
#include "dcmemcard.h"
#include "PCShell.h"
#include "ps2lowsfx.h"
#include "panel.h"
#include "spidey.h"
#include "camera.h"
#include "screen.h"
#include "ps2redbook.h"

CMenu* pYesNoMenu;

// idb_globals.txt: 0x005FAED0 gPausedMenu.
CMenu* gPausedMenu;

EXPORT i32 gFrontGauge;

SSaveGame gSaveGame;

// @FIXME add content
SLevel Levels[FRONT_NUM_LEVELS];

// Tentative names, no idb_globals.txt entries. CMenu::Display spawns a
// small effect record (looks like a highlight/arrow indicator, ~0x28 bytes,
// many i16 position fields) out of a bump-allocated buffer when the
// selection moves to a new entry, guarded by these two globals: current
// write position and one-past-the-end of the buffer. Field layout of the
// record itself is not understood (no consumer of this buffer has been
// decompiled yet), so CMenu::Display below pokes it by raw byte offset
// instead of a named struct.
#define gMenuHighlightBufPos (*reinterpret_cast<u8**>(0x0056FB04))
#define gMenuHighlightBufEnd (*reinterpret_cast<u8**>(0x005FCD1C))

// idb_globals.txt: 0x0054D341 gPrintStubbed. Guards debug stubbed_printf
// calls in both CMenu::Display and Front_LoadGame below (not menu-specific,
// despite the guess made before the idb name turned up).
#define gPrintStubbed (*reinterpret_cast<u8*>(0x0054D341))

// @Ok
// @Matching
INLINE void CMenu::KillBox(void)
{
	delete this->ptr_to;
	this->ptr_to = 0;
}

// @Ok
// Functional decompile, verified logic field-by-field against the IDA
// decompile and disassembly at 0x4401b0 (2026-08-31): the entry loop bounds
// (mCursorLine/mNumLines/field_1B), the SEntry stride (0x20, matches
// gouraud color fields), the fixed-point blend (weight from Sine(),
// (a+b)>>1 + ((weight*(a-b))>>13), *350/256 clamp to 255) and the
// highlight-record bump allocator (gMenuHighlightBufPos/End, 0x28-byte
// record) all match the original. cmpsum still shows 288 mnemonic diffs,
// but built length matches the original exactly (1091 bytes, 358 decoded
// instructions), so this is register allocation / statement scheduling
// residue, not a logic gap. Per this session's functional-decomp bar, not
// pursuing further; byte-match attempt log in front.attempts.md.
// The "just selected this entry" effect writes ~15 fields into a
// bump-allocated ~0x28-byte record (gMenuHighlightBufPos/End) whose struct
// is completely undocumented - no consumer of that buffer has been
// decompiled yet, so the fields below are raw offset pokes with guessed
// meanings (position pairs for what is probably a highlight arrow/box),
// not a named struct member list.
void CMenu::Display(void)
{
	if (this->ptr_to && this->ptr_to->field_30 == 0)
		return;

	Mess_SetTextJustify(this->mJustification);

	i32 y = this->mY;

	for (i32 i = this->mCursorLine;
			i < this->mNumLines && i < (this->mCursorLine + this->field_1B);
			i++)
	{
		y += this->mEntry[i].unk_a;

		if (!this->mEntry[i].unk_b)
			continue;

		if (this->mEntry[i].val_a <= 0)
		{
			y += this->mLineSep;
			continue;
		}

		if (i == this->mLine)
		{
			if (this->field_1E & 0xFF)
			{
				i32 weight = Sine(this->field_20);
				this->field_20 += 200;

				i32 r = ((this->mEntry[i].unk_c + this->mEntry[i].field_11) >> 1)
					+ (((this->mEntry[i].unk_c - this->mEntry[i].field_11) * weight) >> 13);
				i32 g = ((this->mEntry[i].unk_d + this->mEntry[i].field_12) >> 1)
					+ (((this->mEntry[i].unk_d - this->mEntry[i].field_12) * weight) >> 13);
				i32 b = ((this->mEntry[i].unk_e + this->mEntry[i].field_13) >> 1)
					+ (((this->mEntry[i].unk_e - this->mEntry[i].field_13) * weight) >> 13);

				r = r * 350 / 256;
				g = g * 350 / 256;
				b = b * 350 / 256;
				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				Mess_SetRGB(static_cast<u8>(r), static_cast<u8>(g), static_cast<u8>(b), 0);

				i32 sr = ((this->mEntry[i].field_14 + this->mEntry[i].field_17) >> 1)
					+ (((this->mEntry[i].field_14 - this->mEntry[i].field_17) * weight) >> 13);
				i32 sg = ((this->mEntry[i].field_15 + this->mEntry[i].field_18) >> 1)
					+ (((this->mEntry[i].field_15 - this->mEntry[i].field_18) * weight) >> 13);
				i32 sb = ((this->mEntry[i].field_16 + this->mEntry[i].field_19) >> 1)
					+ (((this->mEntry[i].field_16 - this->mEntry[i].field_19) * weight) >> 13);

				sr = sr * 350 / 256;
				sg = sg * 350 / 256;
				sb = sb * 350 / 256;
				if (sr > 255) sr = 255;
				if (sg > 255) sg = 255;
				if (sb > 255) sb = 255;

				Mess_SetRGBBottom(static_cast<u8>(sr), static_cast<u8>(sg), static_cast<u8>(sb));
			}
			else
			{
				Mess_SetRGB(this->mEntry[i].unk_c, this->mEntry[i].unk_d, this->mEntry[i].unk_e, 0);
				Mess_SetRGBBottom(this->mEntry[i].field_14, this->mEntry[i].field_15, this->mEntry[i].field_16);
			}
		}
		else
		{
			Mess_SetRGB(this->mEntry[i].field_11, this->mEntry[i].field_12, this->mEntry[i].field_13, 0);
			Mess_SetRGBBottom(this->mEntry[i].field_17, this->mEntry[i].field_18, this->mEntry[i].field_19);
		}

		Mess_TextWidth(this->mEntry[i].name);
		i32 drawResult = Mess_DrawText(this->mX, y, this->mEntry[i].name, 0, 0x1000);

		if (this->field_16 && i == this->field_17 && this->mJustification == 0)
		{
			i32 highlightOffset = (this->mEntry[i].val_a * drawResult) / 512
				+ (this->mEntry[i].val_a * 14) / 256;

			u8* rec = gMenuHighlightBufPos;
			u8* next = rec + 0x28;

			if (next <= gMenuHighlightBufEnd)
			{
				gMenuHighlightBufPos = next;

				if (!gPrintStubbed)
					stubbed_printf(reinterpret_cast<char*>(0x0054ABF0));
				if (!gPrintStubbed)
					stubbed_printf(reinterpret_cast<char*>(0x0054ABF0));

				i16 yMinus5 = static_cast<i16>(y - 5);
				i16 xBase = static_cast<i16>(this->mX - highlightOffset);
				i16 xBaseMinus20 = static_cast<i16>(xBase - 20);
				i16 yMinus11 = static_cast<i16>(yMinus5 - 6);
				i16 yPlus1 = static_cast<i16>(yMinus5 + 6);
				i16 highlightOffset2 = static_cast<i16>(highlightOffset + this->mX);
				i16 highlightOffset2Plus20 = static_cast<i16>(highlightOffset2 + 20);

				*reinterpret_cast<u8*>(rec + 4) = 0x96;
				*reinterpret_cast<u8*>(rec + 5) = 0;
				*reinterpret_cast<u8*>(rec + 6) = 0;
				*reinterpret_cast<i16*>(rec + 0xA) = yMinus5;
				*reinterpret_cast<i16*>(rec + 8) = xBase;
				*reinterpret_cast<i16*>(rec + 0xC) = xBaseMinus20;
				*reinterpret_cast<i16*>(rec + 0x10) = xBaseMinus20;
				*reinterpret_cast<i16*>(rec + 0xE) = yMinus11;
				*reinterpret_cast<i16*>(rec + 0x12) = yPlus1;
				*reinterpret_cast<u8*>(rec + 0x18) = 0x96;
				*reinterpret_cast<u8*>(rec + 0x19) = 0;
				*reinterpret_cast<u8*>(rec + 0x1A) = 0;
				*reinterpret_cast<i16*>(rec + 0x1E) = yMinus5;
				*reinterpret_cast<i16*>(rec + 0x1C) = highlightOffset2;
				*reinterpret_cast<i16*>(rec + 0x22) = yMinus11;
				*reinterpret_cast<i16*>(rec + 0x20) = highlightOffset2Plus20;
				*reinterpret_cast<i16*>(rec + 0x24) = highlightOffset2Plus20;
				*reinterpret_cast<i16*>(rec + 0x26) = yPlus1;

				stubbed_printf(reinterpret_cast<char*>(0x0056EB54));
				stubbed_printf(reinterpret_cast<char*>(0x0056EB54));
			}
		}

		y += this->mLineSep;
	}

	if (this->ptr_to)
	{
		this->ptr_to->Display();

		if (this->mZoomBoxType == 2)
			this->ptr_to->field_24 = 1;
	}
}

// @Ok
// @Matching
void CMenu::Zoom(i32 zoomboxType)
{
	this->mZoomBoxType = zoomboxType;
	this->KillBox();

	switch (zoomboxType)
	{
		case 0:
			this->ptr_to = new CExpandingBox(
				0,
				this->mY - 18,
				512,
				this->GetMenuHeight() + 27,
				0,
				0,
				1000,
				15,
				0);
			break;
		case 1:
		case 2:
			i32 v16;
			i32 v17;
			if (Utils_CompareStrings(Mess_GetCurrentFont(), "sp_fnt03.fnt"))
			{
				v17 = 10;
				v16 = 14;
			}
			else
			{
				v17 = 12;
				v16 = 17;
			}

			this->ptr_to = new CExpandingBox(
				this->mX - 5,
				this->mY - v17,
				this->menu_width + 12,
				v16 + this->GetMenuHeight(),
				0,
				0,
				30,
				15,
				0);

			break;
		default:
			print_if_false(0, "Bad zoombox type");
			break;
	}
}

// @Ok
// @Matching
void CMenu::AddEntry(const char* pString)
{
	print_if_false(this->mNumLines < 40, "Too many entries added to menu");
	this->mEntry[this->mNumLines].name = pString;
	this->mEntry[this->mNumLines].unk_b = 1;
	this->mEntry[this->mNumLines].unk_a = 0;

	i32 width = Mess_TextWidth(this->mEntry[this->mNumLines].name);
	if (width > this->menu_width)
		this->menu_width = width + this->width_val_a;

	this->mNumLines++;
	this->field_32++;
}

// @Ok
// @Matching
void Front_ClearScreen(void)
{
	ClearImage();
	ClearImage();
}

// Tentative names, no name in idb_globals.txt for these three. Shared with
// CheckForPadUnplugged and Front_Display, which both load pointers from the
// same fixed addresses to draw the same blinking text. Not yet implemented
// anywhere else in the repo, so kept file-local for now.
#define gFrontPadTextOne (*reinterpret_cast<char**>(0x0054B764))
#define gFrontPadTextTwo (*reinterpret_cast<char**>(0x0054B768))
#define gFrontPadTextThree (*reinterpret_cast<char**>(0x0054BBC8))

// Tentative names/globals for Front_Display, no idb_globals.txt entries
// unless noted. gFrontDrawPolyFlag/gFrontShowTrainingTip/gFrontMysteryFlag
// are booleans gating one-off draw blocks; gFrontHintY is a persistent
// animated y-coordinate (this file's only writer); gFrontScreenState is the
// front-end screen-type switch selector; gFrontControllerTwoMenu is a
// second CMenu* right after the already-named gPausedMenu (0x5FAED0, from
// idb_globals.txt); the seven 0x54Bxxx/0x54BBC8 entries are char* text
// pointers, same pattern as gFrontYesText/gFrontPadTextOne above.
#define gFrontDrawPolyFlag (*reinterpret_cast<i32*>(0x0060CFE0))
#define gFrontShowTrainingTip (*reinterpret_cast<i32*>(0x0068293C))
#define gFrontHintY (*reinterpret_cast<i16*>(0x0054A7E0))
#define gFrontScreenState (*reinterpret_cast<i32*>(0x005FAECC))
#define gFrontControllerTwoMenu (*reinterpret_cast<CMenu**>(0x005FAED4))
#define gFrontTrainingTextOne (*reinterpret_cast<char**>(0x0054B76C))
#define gFrontTrainingTextTwo (*reinterpret_cast<char**>(0x0054B890))
#define gFrontPausedText (*reinterpret_cast<char**>(0x0054B74C))
#define gFrontHintText (*reinterpret_cast<char**>(0x0054B748))
#define gFrontYesNoPromptText (*reinterpret_cast<char**>(0x0054B784))
// Same address as gsub_430880 (nullsub_3), see PCShell.cpp/shell.cpp for
// precedent on calling it with a dummy arg via a function-pointer cast.
#define gFrontMysteryValueOne (*reinterpret_cast<i32*>(0x005512EC))
// idb_globals.txt: 0x005FB1B0 gInitRelatedTwo (already a plain, non-fixed
// repo global in init.cpp; only read here, so a file-local fixed-address
// macro is enough, no need to touch init.cpp/init.h for a single caller).
#define G_INIT_RELATED_TWO (*reinterpret_cast<i32*>(0x005FB1B0))

// Forward-declared instead of #include "spidey.h": Front_Display below
// only uses the pointer VALUE (cast to u8* for a raw offset read), so the
// full CPlayer definition is not needed and pulling in spidey.h's huge
// dependency graph is not worth it for one field read.
class CPlayer;
extern CPlayer* MechList;

// @Ok
// Functional decompile, cmpsum residue down to 2 mnemonic diffs (from 145
// on the first honest pass; see front.attempts.md). Remaining diff is a
// tail-merge: the original keeps two separate `ret` instructions at the
// end of the two TTime-blink paths (different `add esp` cleanup, 0x28 vs
// 0x14), while this build's optimizer merges them into one shared `ret`
// regardless of source shape - a compiler-side scheduling decision, not a
// logic gap (instruction counts otherwise match, 220 vs 219). Per this
// session's functional-decomp bar, accepted as-is. The switch's jump table
// bytes are not in tools/functions/<addr>.bin (extraction stops at the
// last real instruction), so the case body order (screen states 1,2,3,4 in
// address order 0x440C40/0x440CB6/0x440CC9/0x440D51) is inferred from
// normal compiler layout, not read directly. `MechList->field_E2` is
// accessed by raw pointer offset instead of adding a new field to CPlayer
// in spidey.h - that struct is huge and shared with many other already-
// `@Ok` files, not safe to touch from a front.cpp-only session (same
// caution as the SLevel/init.cpp note on Front_LoadGame below).
void Front_Display(void)
{
	// same address as gsub_430880 (nullsub_3), declared and defined in
	// PCShell.cpp; see shell.cpp for the same local-extern precedent.
	extern void gsub_430880(void);

	if (gFrontDrawPolyFlag)
		Panel_DrawFlatShadedPoly(0x13, 0xBA, 0xC2, 0x2A, 0, 0, 0, 0, 0);

	if (G_POST_WATER_EFFECT)
		gsub_430880();

	Mess_SetTextJustify(0);

	if (gFrontShowTrainingTip && (Vblanks & 0x20))
	{
		Mess_SetRGB(0x80, 0x80, 0x80, 0);
		Mess_SetScale(0x100);
		Mess_DrawText(0x100, 0x32, gFrontTrainingTextOne, 0, 0x1000);
		Mess_DrawText(0x100, 0xC8, gFrontTrainingTextTwo, 0, 0x1000);
	}

	if (MechList
			&& *reinterpret_cast<i16*>(reinterpret_cast<u8*>(MechList) + 0xE2) <= 0
			&& G_INIT_RELATED_TWO == 1)
	{
		Mess_SetRGB(0x80, 0x80, 0x80, 0);
		Mess_SetScale(0x180);
		Mess_DrawText(0x100, gFrontHintY, gFrontHintText, 0, 0x1000);

		gFrontHintY += 8;
		if (gFrontHintY > 0x78)
			gFrontHintY = 0x78;
	}

	Mess_SetScale(0x100);

	i32 screenState = gFrontScreenState;

	switch (screenState)
	{
		case 1:
			Mess_SetScale(0x100);
			Mess_SetRGB(0x7F, 0x19, 0x21, 0);
			Mess_DrawText(0x100, 0x32, gFrontPausedText, 0, 0x1000);
			((void(*)(i32,i32))gsub_430880)(gFrontMysteryValueOne, 0xAD);
			print_if_false(gPausedMenu != 0, reinterpret_cast<char*>(0x0054AC34));
			gPausedMenu->mX = 0x100;
			gPausedMenu->Display();
			PCSHELL_DrawMouseCursor();
			return;

		case 2:
			if (!gFrontControllerTwoMenu)
				return;
			gFrontControllerTwoMenu->Display();
			return;

		case 3:
			Mess_SetScale(0x100);
			Mess_SetRGB(0x7F, 0x19, 0x21, 0);
			Mess_DrawText(0x100, 0x32, gFrontPausedText, 0, 0x1000);
			((void(*)(i32,i32))gsub_430880)(gFrontMysteryValueOne, 0xAD);
			Mess_SetRGB(0x4D, 0x53, 0x69, 0);
			Mess_SetScale(0x100);
			Mess_DrawText(0x100, 0x4B, gFrontYesNoPromptText, 0, 0x1000);
			pYesNoMenu->Display();
			PCSHELL_DrawMouseCursor();
			return;

		case 4:
			Mess_SetScale(0x100);
			Mess_SetRGB(0x7F, 0x19, 0x21, 0);
			Mess_DrawText(0x100, 0x32, gFrontPausedText, 0, 0x1000);
			Front_RGBRed();
			break;

		default:
			return;
	}

	if (G_SCONTROL[0].Type == 0)
	{
		if ((TTime / 10) & 1)
		{
			Mess_DrawText(0x100, 0xB0, gFrontPadTextOne, 0, 0x1000);
			Mess_DrawText(0x100, 0xC0, gFrontPadTextTwo, 0, 0x1000);
		}

		return;
	}

	if ((TTime / 10) & 1)
		Mess_DrawText(0x100, 0xB8, gFrontPadTextThree, 0, 0x1000);
}

// @Ok
// it seems the game was using older obj file
// code looks debug
INLINE SLevel* Front_FindLevel(char* pTRGName)
{
	for (i32 i = 0;
			i < FRONT_NUM_LEVELS;
			i++)
	{
		char *pName = Levels[i].mName;
		if (!pName[0])
			break;

		if (pName[1] == pTRGName[1] && pName[3] == pTRGName[3])
			return &Levels[i];
	}

	return 0;
}

// @SMALLTODO
void Front_GetButtons(i32 *,i32 *,i32 *,i32 *)
{
    printf("Front_GetButtons(i32 *,i32 *,i32 *,i32 *)");
}

// @Ok
i32 Front_GetLevelIndex(char* pTRGName)
{
	SLevel* pLevel = Front_FindLevel(pTRGName);
	if (!pLevel)
		return -1;

	i32 index =
		(reinterpret_cast<i32>(pLevel) - reinterpret_cast<i32>(&Levels[0]))/sizeof(SLevel);

	print_if_false(index >= 0 && index < 34, "Bad LevelIndex found by Front_GetLevelIndex");
	return index;
}

// Tentative names, no idb_globals.txt entries. gFrontCardExists holds the
// DCCard_Exists(0) result. gDefaultSaveGame is a template SSaveGame the game
// copies into gSaveGame at boot (size matches SSaveGame, 0xBC bytes).
// gFrontYesText/gFrontNoText are the two CMenu entries added to pYesNoMenu,
// guessed from the variable name (a yes/no confirmation menu).
static u8* const gFrontCardExists = (u8*)0x005FAD98;
#define G_DEFAULT_SAVE_GAME (*reinterpret_cast<SSaveGame*>(0x00550E10))
#define gFrontYesText (*reinterpret_cast<char**>(0x0054B780))
#define gFrontNoText (*reinterpret_cast<char**>(0x0054B77C))

// Tentative name, no idb_globals.txt entry (sits in the gap between named
// gPostWaterEffect 0x5FAE98 and pYesNoMenu 0x5FAEAC). Used by CMenu::Update
// as a held-direction repeat counter: reset to 0 when the mouse/trigger
// state resets, incremented once per frame the cursor does not move,
// checked against CMenu::scrollbar_one for the auto-repeat rate.
#define gMenuNavRepeatTimer (*reinterpret_cast<i32*>(0x005FAEA0))

// @Bogus
// Plain non-throwing placement new. The original builds pYesNoMenu with a
// raw CClass::operator new call plus a manual constructor call (no SEH
// frame in the original bytes), not a plain "new CMenu(...)" expression
// (which pulls in exception unwind scaffolding for the allocation, seen
// elsewhere in this file, e.g. CMenu::Zoom's "new CExpandingBox(...)").
inline void* operator new(size_t, void* location)
{
	return location;
}

// @Ok
// @Matching
void Front_Init(void)
{
	*gFrontCardExists = DCCard_Exists(0);
	gSaveGame = G_DEFAULT_SAVE_GAME;

	PShell_ApplyGameState();

	print_if_false(pYesNoMenu == 0, "pYesNoMenu already created");

	void* pMenuMem = CMenu::operator new(sizeof(CMenu));
	if (pMenuMem)
		pYesNoMenu = ::new (pMenuMem) CMenu(0x100, 0, 0, 0x100, 0x100, 0x10);
	else
		pYesNoMenu = 0;

	pYesNoMenu->scrollbar_zero = 1;
	// Original writes only the low byte of field_1E (mov [ecx+1Eh],bl in the
	// original bytes), not a full i16 store. Reproduce with a byte poke
	// instead of guessing field_1E is really two u8s, since nothing else in
	// the repo touches this field yet.
	*reinterpret_cast<u8*>(&pYesNoMenu->field_1E) = 1;

	pYesNoMenu->AddEntry(gFrontYesText);
	pYesNoMenu->AddEntry(gFrontNoText);

	pYesNoMenu->mY = 0x74;
}

// Tentative names/globals for Front_LoadGame, no idb_globals.txt entries
// unless noted. gFrontSlotCounter/gFrontSlotShuffleTable implement a
// non-repeating random digit picker (Utils_Jumble reshuffles the table of 9
// once every 9 draws); gFrontCameraModeFlagOne/Two are two byte flags set
// from the pre-call RestartNode value, meaning unclear without a consumer.
#define gFrontSlotCounter (*reinterpret_cast<i32*>(0x00682944))
static i32 gFrontSlotShuffleTable[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
#define gFrontCameraModeFlagOne (*reinterpret_cast<u8*>(0x0056FB78))
#define gFrontCameraModeFlagTwo (*reinterpret_cast<u8*>(0x0056FBF4))

// @NotOk
// residue: 156 mnemonic diffs (cmpsum), instruction counts close (206
// original vs 208 built). One honest attempt, not iterated further (see
// front.attempts.md). This function allocates and constructs a CPlayer and
// a CCamera with plain `new` expressions, which is exactly why the
// original has an SEH frame (exception-safe cleanup if a constructor
// throws, same reason CMenu::Zoom's `new CExpandingBox(...)` has one, see
// that comment). Matching MSVC6's SEH scope-table generation by hand is not
// realistic in one session. The residue is not pure register-naming noise
// either: a saved-RestartNode local ends up zeroing a register (ebx) early
// for a different reason than the original, and that zero register then
// gets reused as the "compare against 0" operand for several later
// `!gPrintStubbed`-style checks, turning the original's `test al,al` into
// `cmp al,bl` in a few places - a real, if minor, mnemonic-level diff, not
// just an operand/address difference.
// SLevel's field_C/field_10 are read here as one raw 32-bit value at
// offset +0xC (not through named struct fields) instead of widening
// SLevel::field_C from u16 to i32: `Levels[35].field_C = 0xFFEC;` in
// init.cpp sits inside an already-`@Ok` function in a file this session
// does not own, and widening the field would change that store's
// instruction encoding (2-byte immediate becomes 4-byte) - not safe to
// risk from here. The raw 32-bit read still gets the right VALUE, because
// `Levels[]` is a zero-initialized BSS global and the upper 16 bits are
// currently unnamed padding right after field_C.
void Front_LoadGame(SSaveGame *pSave, i32 a2, bool /* a3, unused */)
{
	// same address as gsub_430880 (nullsub_3), declared and defined in
	// PCShell.cpp; see shell.cpp for the same local-extern precedent.
	extern void gsub_430880(void);

	i32 savedRestartNode = RestartNode;

	if (gSaveGame.field_4[1] == 'f' && gSaveGame.field_4[3] == '1')
	{
		print_if_false(gFrontSlotCounter < 9, reinterpret_cast<char*>(0x0054ACCC));

		gSaveGame.field_4[4] = static_cast<char>(gFrontSlotShuffleTable[gFrontSlotCounter] + '0');
		gSaveGame.field_4[5] = '_';
		gSaveGame.field_4[6] = 't';
		gSaveGame.field_4[7] = 0;
		gSaveGame.mRestartPointName[0] = 0;

		gFrontSlotCounter++;
		if (gFrontSlotCounter >= 9)
		{
			gFrontSlotCounter = 0;
			Utils_Jumble(gFrontSlotShuffleTable, 9);
		}
	}

	SLevel* pLevel = Front_FindLevel(gSaveGame.field_4);

	if (pLevel)
	{
		i32 field_10 = *reinterpret_cast<i32*>(reinterpret_cast<u8*>(pLevel) + 0x10);
		i32 field_C = *reinterpret_cast<i32*>(reinterpret_cast<u8*>(pLevel) + 0xC);

		gSaveGame.field_84 |= field_10;
		gSaveGame.field_90 |= field_C;

		if (*reinterpret_cast<u8*>(0x0060CFC5) && Utils_CompareStrings(gSaveGame.field_4, reinterpret_cast<char*>(0x0054A808)))
			gSaveGame.field_84 |= 0x2000000;
	}

	SFX_SpoolOutLevelSFX();

	char levelId[5];
	levelId[0] = gSaveGame.field_4[0];
	levelId[1] = gSaveGame.field_4[1];
	levelId[2] = gSaveGame.field_4[2];
	levelId[3] = gSaveGame.field_4[3];
	levelId[4] = 0;
	SFX_SpoolInLevelSFX(levelId);

	Spidey_SetUserFunction(0, 0);
	((void(*)(i32))gsub_430880)(2000);
	Trig_LoadTRG(gSaveGame.field_4);

	gFrontGauge = 0;
	((void(*)(i32))gsub_430880)(4);

	CPlayer* player = new CPlayer();

	if (a2)
	{
		RestartNode = savedRestartNode;
	}
	else if (pSave->mRestartPointName[0])
	{
		Trig_SetRestart(pSave->mRestartPointName);
	}

	new CCamera(player);

	if (!gPrintStubbed)
		stubbed_printf(reinterpret_cast<char*>(0x00549620));
	gFrontCameraModeFlagOne = 0;

	if (!gPrintStubbed)
		stubbed_printf(reinterpret_cast<char*>(0x00549620));
	gFrontCameraModeFlagTwo = 0;

	G_REDBOOK_XA_CURRENT_PRIORITY = -1;
	Trig_ExecuteRestart();

	CVector v;
	v.vx = 0;
	v.vy = 0;
	v.vz = 0;

	char *pName = &reinterpret_cast<char*>(Trig_GetPosition(&v, RestartNode))[6];
	char *pDestBuf = &gSaveGame.mRestartPointName[0];
	i32 i = 0;

	while (*pName)
	{
		i++;
		*pDestBuf++ = *pName++;
		print_if_false((u8)(i < 50), reinterpret_cast<char*>(0x0054ACA4));
	}

	*pDestBuf = 0;

	CameraList->SetMode(CAMERAMODE_DEMO);
	Screen_StartCircularFadeIn(0x20, 8);
}

// @Ok
// @Matching
void Front_MiniUpdate(void)
{
	i32 type = G_SCONTROL[0].Type;

	if (type != -1)
	{
		if (type != 0)
			return;

		Mess_SetRGB(0xFF, 0, 0, 0);

		if ((TTime / 10) & 1)
		{
			Mess_DrawText(0x100, 0xB0, gFrontPadTextOne, 0, 0x1000);
			Mess_DrawText(0x100, 0xC0, gFrontPadTextTwo, 0, 0x1000);
		}

		return;
	}

	if ((TTime / 10) & 1)
		Mess_DrawText(0x100, 0xB8, gFrontPadTextThree, 0, 0x1000);
}

// @Ok
// @Matching
void Front_RGBRed(void)
{
	Mess_SetRGB(0xFF, 0, 0, 0);
}

// @Ok
void Front_SaveGameState(void)
{
	CVector v4;
	v4.vx = 0;
	v4.vy = 0;
	v4.vz = 0;

	char *pName = &reinterpret_cast<char*>(Trig_GetPosition(&v4, RestartNode))[6];
	char *pDestBuf = &gSaveGame.mRestartPointName[0];
	i32 i = 0;

	while (*pName)
	{
		i++;
		*pDestBuf++ = *pName++;
		print_if_false((u8)(i < 50), "Too many chars in restart point name");
	}

	*pDestBuf = 0;
}

// @MEDIUMTODO
void Front_Update(void)
{
    printf("Front_Update(void)");
}

// @Ok
// @Matching
INLINE void PrintPaused(void)
{
	Mess_SetScale(256);
	Mess_SetRGB(0x7F, 0x19, 0x21, 0);
	Mess_DrawText(256, 50, "Paused", 0, 0x1000);
}

// @Ok
// Functional: turn off gauge, logic verified against Hex-Rays at 0x440ae0.
void Front_GaugeOff(void)
{
	gFrontGauge = 0;
}

// @Ok
CMenu::CMenu(int x,int y,unsigned char Justification,int HiScale,int LowScale, int LineSep)
{
	this->mX = x;
	this->mY = y;
	this->mJustification = Justification;
	this->mLineSep = LineSep;

	for (int i = 0; i<40; i++)
	{
		this->mEntry[i].unk_c = 0x80;
		this->mEntry[i].unk_d = 0x80;
		this->mEntry[i].unk_e = 0x80;
		this->mEntry[i].field_14 = 69;
		this->mEntry[i].field_15 = 60;
		this->mEntry[i].field_16 = 107;
		this->mEntry[i].field_11 = 69;
		this->mEntry[i].field_12 = 60;
		this->mEntry[i].field_13 = 107;
		this->mEntry[i].field_17 = 40;
		this->mEntry[i].field_18 = 35;
		this->mEntry[i].field_19 = 62;
		this->mEntry[i].field_8 = HiScale;
		this->mEntry[i].field_A = LowScale;
	}

	/*
	this->scrollbar_one = 2;

	this->field_1B = -1;
	this->scrollbar_zero = 1;
	this->field_32 = 0;
	this->field_30 = 0;
	this->field_34 = 0;
	this->field_38 = 0;
	*/

	this->scrollbar_one = 2;
	this->field_32 = 0;
	this->field_30 = 0;
	this->field_34 = 0;
	this->field_38 = 0;
	this->mLine = 0;
	this->field_1B = -1;
	this->scrollbar_zero = 1;

	this->Reset();
}

// @Ok
void CMenu::GetEntryXY(const char* entry, int* x, int* y)
{
	*x = this->mX;
	*y = this->mY;

	for ( int i = this->mCursorLine;
			i < this->mNumLines
				&& i < (this->mCursorLine + this->field_1B);
			i++ )
	{
		*y += this->mEntry[i].unk_a;
		if ( this->mEntry[i].unk_b )
		{
			if ( Utils_CompareStrings(entry, this->mEntry[i].name) )
				return;
			*y += this->mLineSep;
		}
	}
}

// @Ok
INLINE i32 CMenu::GetMenuHeight(void)
{
  int v1 = 0;
  int v2 = 0;

  for ( int i = this->mCursorLine;
        i < this->mNumLines && i < (this->mCursorLine + this->field_1B);
        i++ )
  {
    if ( this->mEntry[i].unk_b )
    {
      v1 += this->mEntry[i].unk_a;
      v2++;
    }
  }

  return v1 + (v2 - 1) * this->mLineSep;
}

// @Ok
void CMenu::CentreY(void)
{
	this->mY = (240 - this->GetMenuHeight()) / 2;
}

// @Ok
void CMenu::NonGouraud(void)
{
	for(int i = 0; i < 40; i++)
	{
		this->mEntry[i].field_14 = this->mEntry[i].unk_c;
		this->mEntry[i].field_15 = this->mEntry[i].unk_d;
		this->mEntry[i].field_16 = this->mEntry[i].unk_e;
		this->mEntry[i].field_17 = this->mEntry[i].field_11;
		this->mEntry[i].field_18 = this->mEntry[i].field_12;
		this->mEntry[i].field_19 = this->mEntry[i].field_13;
	}
}

// @Ok
void CMenu::SetNormalColor(unsigned int a2, int a3, int a4, int a5)
{
	this->mEntry[a2].field_11 = a3;
	this->mEntry[a2].field_12 = a4;
	this->mEntry[a2].field_13 = a5;

	this->mEntry[a2].field_17 = 150 * a3 / 256;
	this->mEntry[a2].field_18 = 150 * a4 / 256;
	this->mEntry[a2].field_19 = 150 * a5 / 256;
}


// @Ok
void CMenu::SetSelColor(unsigned int a2, int a3, int a4, int a5)
{
	this->mEntry[a2].unk_c = a3;
	this->mEntry[a2].unk_d = a4;
	this->mEntry[a2].unk_e = a5;

	this->mEntry[a2].field_14 = 150 * a3 / 256;
	this->mEntry[a2].field_15 = 150 * a4 / 256;
	this->mEntry[a2].field_16 = 150 * a5 / 256;
}

// @Ok
int INLINE CMenu::FindEntry(const char* a2)
{
	for(int i = 0; i<this->mNumLines; i++)
	{
		if (Utils_CompareStrings(a2, this->mEntry[i].name))
		{
			return i;
		};
	}
	
	print_if_false(0, "Entry not found");
	return 0;
}

// @Ok
void CMenu::EntryOn(const char* a2)
{
	int res = this->FindEntry(a2);
	if (!this->mEntry[res].unk_b)
	{
		this->mEntry[res].unk_b = 1;
		this->field_32--;
	}
}

// @Ok
// On the assignement it's weird has it properly addresses the offset
// while for the if it does the weird jump
void CMenu::EntryOff(const char* a2)
{
	int res = this->FindEntry(a2);
	if (this->mEntry[res].unk_b)
	{
		this->mEntry[res].unk_b = 0;
		this->field_32--;
	}
}

// @Ok
// has the sub eax, 0 for some reason
void CMenu::CentreX(void)
{
	int just = this->mJustification;

	if (just)
	{
		switch(just)
		{
			case 1:
				this->mX = (512 - this->menu_width) / 2;
				return;
			case 2:
				this->mX = this->menu_width + (512 - this->menu_width) / 2;
				return;
		}
	}
	else
	{
		this->mX = 256;
	}
}

// @Ok
int CMenu::FinishedZooming(void)
{
	int *ptr_to = reinterpret_cast<int*>(this->ptr_to);
	if (!ptr_to)
	{
		return 1;
	}

	return ptr_to[12] != 0;
}

// @Ok
void CMenu::AdjustWidth(int width)
{
	this->width_val_a = width;
}

// @Ok
int CMenu::ChoiceIs(const char* pString)
{
	if (this->mLine < 0x28)
	{
		return Utils_CompareStrings(pString, this->mEntry[this->mLine].name);
	}

	return 0;
}

// @Ok
// slightly different registers but the same
void CMenu::SetRedText(unsigned char Line)
{
	print_if_false(Line < this->mNumLines, "Bad line sent to SetRedText");

	this->mEntry[Line].unk_c = 255;
	this->mEntry[Line].unk_d = 0;
	this->mEntry[Line].unk_d = -1;

	this->mEntry[Line].field_15 = 0;
	this->mEntry[Line].field_16 = 0;

	this->mEntry[Line].field_11 = 96;
	this->mEntry[Line].field_12 = 0;
	this->mEntry[Line].field_13 = 0;

	
	this->mEntry[Line].field_17 = 96;
	this->mEntry[Line].field_18 = 0;
	this->mEntry[Line].field_19 = 0;
}

// @Ok
void CMenu::Reset(void)
{
	this->SetLine(0);
}

// @Ok
// loop offset starts in field_8, instead of unk_b, but that's fine
void CMenu::SetLine(char Line)
{
	this->mLine = Line;

	SEntry *entries = this->mEntry;
	for(i32 i = 0; i < 40; i++)
	{
		i16 v5;

		if (i != this->mLine)
		{
			v5 = entries[i].field_A;
		}
		else
		{
			v5 = entries[i].field_8;
		}

		entries[i].val_b = v5;
		entries[i].val_a = v5;
	}
}

// CMenu::ProcessMouse (0x50C8A0) is defined in PCShell.cpp, not here: its
// address sits next to the PCSHELL_* functions, and the original CMenu::Update
// calls it with a plain direct call, same TU boundary as CMenu::EntryEnable
// (see the comment on that function in PCShell.cpp).

// Mac symbol Update__5CMenuFv, address 0x440600
// @NotOk
// residue: 182 mnemonic diffs (cmpsum), all one cascade from a single
// register choice in the repeat-timer check (original keeps the timer
// counter in edi across the whole "if (timer<=20) skip" / idiv-by-
// scrollbar_one block, reusing edi right after for the 0x400 constant;
// our build loads it into eax directly and never touches edi there).
// Everything before this point (leading push order, the disabled-entry
// skip loop, the ProcessMouse call site, the direction of the timer<=20
// check, the up/down navigation do-whiles) matches. 7 hypotheses tried,
// logged in front.attempts.md; below the 15-hypothesis minimum for a
// medium function, so left @NotOk rather than @AlmostMatching.
void CMenu::Update(void)
{
	if ((u8)this->mLine > 40)
		return;

	while (!(this->mEntry[this->mLine].unk_b && this->mEntry[this->mLine].what == 0))
	{
		this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_A;
		this->mEntry[this->mLine].val_a = this->mEntry[this->mLine].field_A;

		this->mLine++;
		if (this->mLine >= this->mNumLines)
			this->mLine = 0;

		this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_8;
		this->mEntry[this->mLine].val_a = this->mEntry[this->mLine].field_A;
	}

	i32 startLine = this->mLine;
	i32 mouseRes = this->ProcessMouse();

	if (mouseRes != 2 && PCSHELL_CheckTriggers(0x3003, 0, 0))
	{
		i32 timer = gMenuNavRepeatTimer;

		if (timer != 0)
		{
			if (timer <= 20)
				goto skipMove;
			if (timer % this->scrollbar_one != 0)
				goto skipMove;
		}

		{
			if (PCSHELL_CheckTriggers(0x1001, 0, 0))
			{
				G_SCONTROL[0].Up.Triggered = 0;
				this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_A;

				do
				{
					if (this->mLine != 0)
					{
						this->mLine--;
						this->field_20 = 0x400;
					}
					else if (this->scrollbar_zero)
					{
						this->mLine = this->mNumLines - 1;
						this->field_20 = 0x400;
					}
				}
				while (!(this->mEntry[this->mLine].unk_b && this->mEntry[this->mLine].what == 0));

				this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_8;
			}

			if (PCSHELL_CheckTriggers(0x2002, 0, 0))
			{
				G_SCONTROL[0].Down.Triggered = 0;
				this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_A;

				do
				{
					this->mLine++;
					if (this->mLine < this->mNumLines)
					{
						this->field_20 = 0x400;
					}
					else if (this->scrollbar_zero)
					{
						this->mLine = 0;
						this->field_20 = 0x400;
					}
					else
					{
						this->mLine--;
					}
				}
				while (!(this->mEntry[this->mLine].unk_b && this->mEntry[this->mLine].what == 0));

				this->mEntry[this->mLine].val_b = this->mEntry[this->mLine].field_8;
			}
		}

skipMove:
		gMenuNavRepeatTimer++;
	}
	else
	{
		gMenuNavRepeatTimer = 0;
	}

	if (this->mLine != startLine)
		SFX_Play(0x29, 0x3FFF, 0);

	for (i32 i = 0; i < this->mNumLines; i++)
	{
		if (this->mEntry[i].val_a >= this->mEntry[i].val_b)
		{
			this->mEntry[i].val_a = this->mEntry[i].val_a + 40;
			if (this->mEntry[i].val_a > this->mEntry[i].val_b)
				this->mEntry[i].val_a = this->mEntry[i].val_b;
		}

		if (this->mEntry[i].val_a <= this->mEntry[i].val_b)
		{
			this->mEntry[i].val_a = this->mEntry[i].val_a - 40;
			if (this->mEntry[i].val_a < this->mEntry[i].val_b)
				this->mEntry[i].val_a = this->mEntry[i].val_b;
		}
	}

	if (mouseRes == 0 && this->mLine != startLine)
	{
		if (this->mLine < this->mCursorLine)
		{
			this->mCursorLine = this->mLine;
		}
		else
		{
			i32 activeCount = 0;

			if (this->mCursorLine <= this->mLine)
			{
				for (i32 k = this->mCursorLine; k <= this->mLine; k++)
					if (this->mEntry[k].unk_b)
						activeCount++;
			}

			if (activeCount > this->field_1B)
			{
				if (this->mLine == 0)
				{
					this->mCursorLine = 0;
				}
				else
				{
					i32 visible = 0;

					do
					{
						if (this->mEntry[this->mCursorLine].unk_b)
						{
							visible++;
							if (visible == this->field_1B)
								break;
						}

						this->mCursorLine--;
					}
					while (this->mCursorLine != 0);
				}
			}
		}
	}

	if (!this->ptr_to)
		return;

	if ((u16)this->field_32 <= this->field_1B)
	{
		this->ptr_to->field_28 = 0;
	}
	else
	{
		this->ptr_to->field_28 = (this->mCursorLine * 256) / (this->field_32 - this->field_1B);
	}
}

// @Ok
CMenu::~CMenu()
{
	this->KillBox();
}

void validate_CMenu(void)
{
	VALIDATE_SIZE(CMenu, 0x53C);

	VALIDATE(CMenu, ptr_to, 0x4);
	VALIDATE(CMenu, menu_width, 0x8);
	VALIDATE(CMenu, text_val_b, 0xA);
	VALIDATE(CMenu, width_val_a, 0xC);

	VALIDATE(CMenu, scrollbar_one, 0x10);
	VALIDATE(CMenu, scrollbar_zero, 0x11);
	VALIDATE(CMenu, mJustification, 0x12);
	VALIDATE(CMenu, mLine, 0x14);

	VALIDATE(CMenu, mCursorLine, 0x15);
	VALIDATE(CMenu, field_16, 0x16);
	VALIDATE(CMenu, field_17, 0x17);
	VALIDATE(CMenu, mNumLines,  0x1A);
	VALIDATE(CMenu, field_1B,  0x1B);
	VALIDATE(CMenu, mZoomBoxType,  0x1C);
	VALIDATE(CMenu, field_1E, 0x1E);
	VALIDATE(CMenu, field_20, 0x20);
	VALIDATE(CMenu, mX, 0x24);
	VALIDATE(CMenu, mY, 0x28);
	VALIDATE(CMenu, mLineSep, 0x2C);

	VALIDATE(CMenu, field_30, 0x30);
	VALIDATE(CMenu, field_32, 0x32);
	VALIDATE(CMenu, field_34, 0x34);
	VALIDATE(CMenu, field_38, 0x38);

	VALIDATE(CMenu, mEntry, 0x3C);
}

void validate_SEntry(void)
{
	VALIDATE_SIZE(SEntry, 0x20);

	VALIDATE(SEntry, name, 0x0);
	VALIDATE(SEntry, val_a, 0x4);
	VALIDATE(SEntry, val_b, 0x6);
	VALIDATE(SEntry, field_8, 0x8);
	VALIDATE(SEntry, field_A, 0xA);
	VALIDATE(SEntry, unk_a, 0xC);
	VALIDATE(SEntry, unk_b, 0xD);
	VALIDATE(SEntry, unk_c, 0xE);
	VALIDATE(SEntry, unk_d, 0xF);
	VALIDATE(SEntry, unk_e, 0x10);
	VALIDATE(SEntry, field_11, 0x11);
	VALIDATE(SEntry, field_12, 0x12);
	VALIDATE(SEntry, field_13, 0x13);
	VALIDATE(SEntry, field_14, 0x14);
	VALIDATE(SEntry, field_15, 0x15);
	VALIDATE(SEntry, field_16, 0x16);
	VALIDATE(SEntry, field_17, 0x17);
	VALIDATE(SEntry, field_18, 0x18);
	VALIDATE(SEntry, field_19, 0x19);
	VALIDATE(SEntry, field_1A, 0x1A);
	VALIDATE(SEntry, field_1B, 0x1B);
	VALIDATE(SEntry, what, 0x1C);
}

void validate_SLevel(void)
{
	VALIDATE_SIZE(SLevel, 0x14);

	VALIDATE(SLevel, mDisplayName, 0x0);
	VALIDATE(SLevel, mName, 0x4);

	VALIDATE(SLevel, field_C, 0xC);
}
