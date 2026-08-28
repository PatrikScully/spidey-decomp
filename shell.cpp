#include "shell.h"
#include "ps2m3d.h"
#include "utils.h"
#include "ps2lowsfx.h"
#include "effects.h"
#include "spool.h"
#include "panel.h"
#include "front.h"
#include "PCGfx.h"
#include "mess.h"
#include "ps2pad.h"
#include "dcshellutils.h"
#include "utils.h"
#include "PCShell.h"
#include "PCInput.h"
#include "DXsound.h"
#include "powerup.h"
#include "pshell.h"
#include "spidey.h"
#include "ps2m3d.h"
#include "init.h"
#include "ps2redbook.h"
#include "m3dutils.h"
#include "db.h"
#include "ps2funcs.h"
#include "tweak.h"
#include "ps2gamefmv.h"
#include "bmr.h"
#include "ps2card.h"

#include <cstring>

#include "validate.h"

EXPORT SLight M3d_RudeSpideyLight =
{
  { { -2047, -2896, -2047 }, { 0, 0, 4096 }, { 0, 0, -4096 } },
  0,
  { { 1440, 1920, 1760 }, { 1440, 1920, 1760 }, { 1440, 1920, 1760 } },
  0,
  { 1600, 1600, 1600 }
};

EXPORT SLight M3d_SpideyCIconLight =
{
  { { -2047, -2896, -2047 }, { 0, 0, 4096 }, { 0, 0, -4096 } },

  0,
  { { 2048, 2048, 2048 }, { 2048, 2048, 2048 }, { 2048, 2048, 2048 } },
  0,

  { 2048, 2048, 2048 }
};



// @FIXME
EXPORT SRecords gGlobalRecords;

EXPORT i32 dword_6A7788[16];
EXPORT void* gBiographies;
EXPORT i32 gPshellArmorRealted;

// @FIXME
EXPORT SRecordRelated gChallenges[NUM_CHALLS];

EXPORT u16 OTPushback[3];
EXPORT u8 gPShellCleanup = 1;
EXPORT i32 gShellFromGame;
EXPORT i32 gShellInitialized;


EXPORT u8 gCurrentCostume;

CBody *MiscList;

// @FIXME
EXPORT SSkinGooSource gVenomSkinGooSource;
EXPORT SSkinGooParams gVenomSkinGooParams;

// @FIXME
EXPORT SSkinGooSource gCarnageSkinGooSourceShell;
EXPORT SSkinGooParams gCarnageSkinGooParams;

// @FIXME
EXPORT SSkinGooSource gSuperDocOckSkinGooSource;
EXPORT SSkinGooParams gSuperDocOckSkinGooParams;

EXPORT i32 gShellMysterioRelated;
extern SPSXRegion PSXRegion[];

SAnimFrame* gBackgroundAnimFrame;

const i32 NUM_SAVE_GAME_SLOTS = 8;
EXPORT SSaveGame gSaveGameSlots[NUM_SAVE_GAME_SLOTS];

// sin/cos pair table, i16[2*n] = sin(n), i16[2*n+1] = cos(n), n = angle & 0xFFF
static i16 * const word_610C48 = (i16*)0x610C48;

// @Ok
// @Matching
void Shell_RelocatableModuleClear(void)
{
}

// @Ok
// @Matching
void Shell_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Shell_RelocatableModuleClear;
	pMod->field_C[0] = Shell_DoShell;
	pMod->field_C[1] = Shell_SaveGame;
}

// @Ok
void Shell_AddGameSlots(CMenu* pMenu)
{
	print_if_false(pMenu->mNumLines == 0, "Tried to add slots to non-empty menu");

	for (i32 i = 0; i < NUM_SAVE_GAME_SLOTS; i++)
	{
		i32 v6 = 0;

		if (gSaveGameSlots[i].mChecksum)
		{
			u32 checksum = Shell_CalculateGameChecksum(&gSaveGameSlots[i]);

			if (checksum == gSaveGameSlots[i].mChecksum)
			{
				pMenu->AddEntry(gSaveGameSlots[i].field_3F);
			}
			else
			{
				pMenu->AddEntry("read error");
				v6 = 1;
			}
		}
		else
		{
				pMenu->AddEntry("---- empty ----");
				v6 = 1;
		}

		if (v6)
		{
			pMenu->mEntry[i].unk_c = 80;
			pMenu->mEntry[i].unk_d = 16;
			pMenu->mEntry[i].unk_e = 36;
			pMenu->mEntry[i].field_11 = 40;
			pMenu->mEntry[i].field_12 = 8;

			pMenu->mEntry[i].field_13 = 0x12;
			pMenu->mEntry[i].field_14 = 0x50;

			pMenu->mEntry[i].field_15 = 0x10;
			pMenu->mEntry[i].field_16 = 0x24;

			pMenu->mEntry[i].field_17 = 0x28;
			pMenu->mEntry[i].field_18 = 8;

			pMenu->mEntry[i].field_19 = 0x12;
		}
	}
}

// @Ok
// PowerPC version implies that mSize of SSaveGame is not a field but part of the array
// i don't like it
INLINE u32 Shell_CalculateGameChecksum(SSaveGame* pSave)
{
	u32 checksum = 0;
	print_if_false(1u, "Size of SSaveGame not a multiple of 4");

	u32* fields = &reinterpret_cast<u32*>(pSave)[1];

	for (i32 i = 0; i<0x2E; i++)
	{
		if (checksum & 0x80000000)
		{
			checksum = checksum * 2 + 1;
		}
		else
		{
			checksum <<= 1;
		}

		checksum += fields[i];
	}

	return checksum | 1;
}

// @MEDIUMTODO
void Shell_CharacterViewer(void)
{
	printf("void Shell_CharacterViewer");
}

// @Ok
// @Note: at the end it does add esp 2 times instead of one time but it's the same
void Shell_Cheats(void)
{
	const char *pDesc = 0;
	char v3[12];

	v3[0] = 0;

	while (Shell_InputName(v3, 1, 1, pDesc))
	{
		i32 res = PShell_ActivateCheat(v3);
		if (res != -1)
		{
			SFX_Play(0x1D, 0x2000, 0);
			pDesc = gCheats[res].pDescription;
		}
		else
		{
			SFX_Play(0x1B, 0x2000, 0);
			pDesc = 0;
		}

		v3[0] = 0;
	}

	SFX_Play(0x23, 0x2000, 0);

	if (gCurrentCostume != 5)
	{
		Spidey_BagHead(4096, gCurrentCostume != 9 ? 0 : 2);
	}
	else
	{
		Spidey_BagHead(4096, 1);
	}
}

// unnamed helper, address 0x48EA90, name from names.json. Called once per frame by
// several Shell_ menu loops (ScreenAdjust, ShowRecord, ChooseSurvivalArena, ...).
// not yet decompiled on its own. Kept out-of-line (same trick as PCShell.cpp's
// gsub_430680/gsub_430880/gsub_515850, needed because this stub lives in the same
// TU as its callers).
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// @SMALLTODO
EXPORT void CheckForPadUnplugged(void)
{
	printf("CheckForPadUnplugged(void)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// shared per-frame ease value for the title bar shake on some Shell_ menu screens
// (ScreenAdjust, ShowRecord, ChooseSurvivalArena all use it). tentative name, no idb
// match (0x5512EC). distinct from PCShell.cpp's PCSHELL_DoDisplayOptions/
// DoControllerConfig, which use a stack local for the same easing idiom. original
// static initial value confirmed 0x200 from the exe's raw .data bytes at 0x5512EC
// (Shell_ChooseSurvivalArena reads/updates it without resetting it first, so the
// initializer is load-bearing there even though ScreenAdjust/ShowRecord both
// overwrite it before first use).
EXPORT i32 gShellMenuEase = 0x200;

// tentative name, no idb match (0x54D38C, checked right after Pad_Update() in several
// Shell_ menu loops; guessed to gate an early abort, e.g. game shutting down. nearest
// idb_globals.txt neighbour is SymBurnRegion at 0x54D388).
static u8 * const gShellMenuAbort = (u8*)0x54D38C;

// @Ok
i32 Shell_ChooseEnemy(i32 a1, u8 a2, i8 a3)
{
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("henchman");
	pMenu->AddEntry("thug");
	i32 v18 = 1;
	i32 v19 = 1;
	i32 v17 = 0;
	if ((u8)(gSaveGame.field_84 >> 8) >= 0x80)
	{
		pMenu->AddEntry("lizardman");
		goto skip1;
	}
	{
		i32 v5 = -1;
		for (i32 idx = 0; idx < NUM_CHALLS; idx++)
		{
			if (gChallenges[idx].field_6 == a2 && gChallenges[idx].field_8 == a3 && gChallenges[idx].field_9 == 2)
				v5 = idx;
		}
		print_if_false(v5 != -1, "Mission not found");
		if (gGlobalRecords.mScores[5 * v5].field_0 != 0)
		{
			pMenu->AddEntry("lizardman");
			if (a1 != 0)
				goto skip1;
		}
		else
			pMenu->AddEntry("? ? ? ?");
		pMenu->SetRedText(2);
		v18 = 0;
	}
	skip1:
	if ((gSaveGame.field_84 & 0x40000) == 0)
	{
		i32 v9 = -1;
		for (i32 idx = 0; idx < NUM_CHALLS; idx++)
		{
			if (gChallenges[idx].field_6 == a2 && gChallenges[idx].field_8 == a3 && gChallenges[idx].field_9 == 3)
				v9 = idx;
		}
		print_if_false(v9 != -1, "Mission not found");
		if (gGlobalRecords.mScores[5 * v9].field_0 != 0)
		{
			pMenu->AddEntry("symbiote");
			if (a1 != 0)
				goto skip2;
		}
		else
			pMenu->AddEntry("? ? ? ?");
		pMenu->SetRedText(3);
		v19 = 0;
		goto skip2;
	}
	pMenu->AddEntry("symbiote");
	skip2:
	pMenu->CentreY();
	pMenu->Zoom(0);

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v21 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		if (a1 != 0)
			Shell_DrawTitleBar(128, 38, "High Scores", 1, 0, 150, -21, 29);
		else
			Shell_DrawTitleBar(128, 38, "training", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (a1 == 0 && pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(256, 57, "select the type of opponent", 0, 0x1000);
			Mess_DrawText(256, 69, "you wish to train against!", 0, 0x1000);
			const char* v13 = 0;
			switch (pMenu->mLine)
			{
			case 0:
				Mess_DrawText(256, 185, "the henchmen are on the lookout for", 0, 0x1000);
				Mess_DrawText(256, 200, "spidey and armed with two hand guns!", 0, 0x1000);
				break;
			case 1:
				Mess_DrawText(256, 185, "the thugs are high-tech bank robbers", 0, 0x1000);
				v13 = "armed with laser guns!";
				break;
			case 2:
				if (pMenu->ChoiceIs("? ? ? ?"))
					break;
				Mess_DrawText(256, 185, "the lizardmen are a fierce result", 0, 0x1000);
				v13 = "of a twisted experiment!";
				break;
			case 3:
				if (pMenu->ChoiceIs("? ? ? ?"))
					break;
				Mess_DrawText(256, 185, "the symbiotes are created from", 0, 0x1000);
				v13 = "carnage's alien symbiote!";
				break;
			default:
				break;
			}
			if (v13)
				Mess_DrawText(256, 200, v13, 0, 0x1000);
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
			break;
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			i32 x, y;
			pMenu->GetEntryXY(pMenu->mEntry[pMenu->mLine].name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(pMenu->mEntry[pMenu->mLine].name, x, y, pMenu->mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			i32 chosen = 0;
			i32 denied = 0;
			switch (pMenu->mLine)
			{
			case 0:
				*(i32*)0x0055128C = 0;
				chosen = 1;
				break;
			case 1:
				*(i32*)0x0055128C = 1;
				chosen = 1;
				break;
			case 2:
				if (v18 == 0)
					denied = 1;
				else
				{
					*(i32*)0x0055128C = 2;
					chosen = 1;
				}
				break;
			case 3:
				if (v19 != 0)
				{
					*(i32*)0x0055128C = 3;
					chosen = 1;
				}
				else
					denied = 1;
				break;
			default:
				print_if_false(0, "Bad enemy choice");
				break;
			}
			if (chosen)
			{
				v17 = 1;
				SFX_Play(0x1F, 0x2000, 0);
				goto done;
			}
			if (denied)
				SFX_Play(0x1B, 0x2000, 0);
		}
		if (Vblanks == v21)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}
	G_SCONTROL[0].Circle.Triggered = 0;
	SFX_Play(0x23, 0x2000, 0);
	done:
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v17;
}

// @MEDIUMTODO
void Shell_ChooseItemCollection(i32)
{
    printf("Shell_ChooseItemCollection(i32)");
}

// @MEDIUMTODO
void Shell_ChooseSpeedTraining(i32)
{
    printf("Shell_ChooseSpeedTraining(i32)");
}

// arena names for the two AddEntry calls (0x54BAEC/0x54BAF0 in the original, plain
// string-literal globals like gShowRecordTitle below since we only ever read them).
static char* STR_ARENA_COMBAT_ROOM = "combat room";
static char* STR_ARENA_BUILDING_TOP = "building top";

// title text (0x54BBA0/0x54BBA4). 0x54BBA0 is the same address as
// Shell_ShowRecord's gShowRecordTitle below ("High Scores"); duplicated here as its
// own literal since gShowRecordTitle is file-local (static) and declared later.
static char* STR_ARENA_HIGH_SCORES_TITLE = "High Scores";
static char* STR_ARENA_TRAINING_TITLE = "training";

// instructional heading + per-arena description lines (0x54B9D8/DC/E0/E4/E8).
static char* STR_ARENA_HEADING = "select which area to train in!";
static char* STR_ARENA_COMBAT_ROOM_DESC1 = "this area is an enclosed room";
static char* STR_ARENA_COMBAT_ROOM_DESC2 = "that spidey trains in!";
static char* STR_ARENA_BUILDING_TOP_DESC1 = "this area is located on top of a";
static char* STR_ARENA_BUILDING_TOP_DESC2 = "new york building top!";

static char* STR_BAD_ARENA_CHOICE = "Bad arena choice";

// output of the menu: which arena the player picked (0 = combat room, 1 = building
// top). tentative name, no idb match (0x551294). original static initial value is
// -1 (unselected sentinel), confirmed from the exe's raw .data bytes.
EXPORT i32 gTrainingArenaChoice = -1;

// Confirms the CMenu/SEH theory from shell.attempts.md's bonus finding generalizes
// beyond the throwaway test: `new CMenu(0x100,0,0,0x100,0x100,0x10)` in this real
// function reproduces the original's exact SEH prologue (mov eax,fs:0; push -1;
// push offset handler; push eax; mov fs:0,esp) and epilogue in our own build, byte
// for byte in shape. The whole per-frame loop (CMenu Display/Update/FinishedZooming/
// GetEntryXY, PCSHELL_CheckTriggers cancel/confirm branches, description text,
// DrawSync idiom, VblankProcessing) is a faithful reconstruction from the
// disassembly and cross-referenced against Shell_ScreenAdjust/Shell_ShowRecord's
// already-@Ok idioms for every shared call/global.
//
// Residue: 242 mnemonic diffs (down from 267 on the first pass), all one root
// cause. Original caches the constant 0 in ebp very early (right between "push esi"
// and "push edi" in the register-save sequence, before PShell_NormalFont is even
// called) and reuses that register for both CMenu ctor args (y, Justification), the
// null-check comparand, and dozens of later zero arguments/comparisons throughout
// the function. Our build never establishes that early cached register: it either
// materializes ebp=0 lazily right before the null check, or (depending on unrelated
// local variable types elsewhere in the function) skips the cached register
// entirely and pushes literal 0 at each use site. Confirmed via a throwaway
// isolated test (Shell_CMenuSchedTest, removed before commit: just the
// DrawSync+PShell_NormalFont+new CMenu+2x AddEntry+CentreY+Zoom(0) preamble, no
// loop) that a minimal preamble with few later zero-reuses never gets the cached
// register either -- so this is a whole-function register-pressure heuristic in
// MSVC6's allocator, not something localized to the construction site itself.
//
// Attempts (all rebuilt + cmpsum'd, each a distinct theory targeting this one
// cluster, first divergence always the ebp-vs-push-edi ordering right after the
// SEH prologue and 3 register saves):
// 1. Baseline straight translation: 267 diffs.
// 2. Named `i32 zero = 0;` local declared before PShell_NormalFont(), used for the
//    ctor's y/Justification args instead of the literal 0: no change (267). MSVC
//    already constant-propagates a never-reassigned local identically to a literal.
// 3. Split `CMenu* menu;` declaration from its `menu = new CMenu(...)` assignment
//    (separate statements instead of one): no change (267).
// 4. `mouseSelected` local changed from u8 to i32: 267 -> 242, and shifted the
//    specific instructions at the divergence point (a `cmp eax,ebp` became
//    `test eax,eax`), confirming a local's TYPE elsewhere in the function does
//    perturb the allocator's decision at the top of the function, even though it
//    didn't fix the root divergence.
// 5. Declaration order swap (`i32 x, y;` before vs after `const char* name = ...;`
//    in the mouse-hover block): no change (242).
// 6. Isolated minimal-preamble diagnostic (Shell_CMenuSchedTest, see above): no
//    cached register at all with a short function, confirming the effect is
//    whole-function, not localized.
//
// This is the same class of problem CLAUDE.md already documents as sometimes
// irreproducible from source (Utils_VblankProcessing's CSE hoist, 5 attempts,
// accepted as residue) -- a whole-function MSVC6 register-allocation heuristic that
// resists targeted source changes. Left @NotOk rather than forcing an
// @AlmostMatching claim, since 242 diffs is real residue, not a single-instruction
// toolchain quirk. Flagging for whoever picks this up next: try adding more early,
// close-together zero-valued arguments right after the CMenu construction (before
// AddEntry/CentreY/Zoom) to see if there's a density/proximity threshold that flips
// the allocator's decision; the isolated-preamble test above only tried the
// construction site itself with nothing added around it.
// @Ok
void Shell_ChooseSurvivalArena(i32 fromHighScores)
{
	// defined once in PCShell.cpp, called here through a local extern (same pattern
	// as Shell_ScreenAdjust/Shell_ShowRecord above).
	extern void gsub_430880(void);
	extern void gsub_430680(void);

	Pause(1);

	// DrawSync(), written out by hand (see the comment on this idiom in
	// Shell_ShowRecord above).
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	gsub_430680();

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");

	PShell_NormalFont();

	CMenu* menu = new CMenu(0x100, 0, 0, 0x100, 0x100, 0x10);

	menu->AddEntry(STR_ARENA_COMBAT_ROOM);
	menu->AddEntry(STR_ARENA_BUILDING_TOP);
	menu->CentreY();
	menu->Zoom(0);

	while (1)
	{
		gsub_430880();
		Db_FlipClear();
		CalcPolyBufferEnd();

		i32 vblanksSnapshot = Vblanks;

		if (!gSceneRelated)
			PCGfx_BeginScene(1u, -1);

		Shell_DrawBackground();

		Shell_DrawTitleBar(0x80, 0x26, fromHighScores ? STR_ARENA_HIGH_SCORES_TITLE : STR_ARENA_TRAINING_TITLE, 1, 0, 0x96, -21, 0x1D);

		menu->Display();

		if (!fromHighScores && menu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(0x100, 0x3c, STR_ARENA_HEADING, 0, 0x1000u);

			if (menu->mLine == 0)
			{
				Mess_DrawText(0x100, 0xaa, STR_ARENA_COMBAT_ROOM_DESC1, 0, 0x1000u);
				Mess_DrawText(0x100, 0xb6, STR_ARENA_COMBAT_ROOM_DESC2, 0, 0x1000u);
			}
			else if (menu->mLine == 1)
			{
				Mess_DrawText(0x100, 0xaa, STR_ARENA_BUILDING_TOP_DESC1, 0, 0x1000u);
				Mess_DrawText(0x100, 0xb6, STR_ARENA_BUILDING_TOP_DESC2, 0, 0x1000u);
			}
		}

		PCSHELL_DrawMouseCursor();

		if (gSceneRelated)
			PCGfx_EndScene(1);

		gShellMenuEase = PShell_MoveTowards(gShellMenuEase, 0x180);

		if (menu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);

		Pad_Update();

		if (*gShellMenuAbort)
			return;

		CheckForPadUnplugged();

		menu->Update();

		if (PCSHELL_CheckTriggers(0x20220, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			break;
		}

		i32 mouseSelected = 0;

		if (PCSHELL_CheckTriggers(0x100, 1, 1))
		{
			i32 x, y;
			const char* name = menu->mEntry[menu->mLine].name;
			menu->GetEntryXY(name, &x, &y);

			if (PCSHELL_IsMouseOverText(name, x, y, menu->mJustification))
				mouseSelected = 1;
		}

		if (menu->mLine < 0x28 && (mouseSelected || PCSHELL_CheckTriggers(0x10010, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			SFX_Play(0x1F, 0x2000, 0);

			if (menu->mLine == 0)
				gTrainingArenaChoice = 0;
			else if (menu->mLine == 1)
				gTrainingArenaChoice = 1;
			else
				print_if_false(0, STR_BAD_ARENA_CHOICE);

			break;
		}

		if (Vblanks == vblanksSnapshot)
			Pause(1);

		DoVblankProcessing = 0;
		Pause(1);

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		gsub_430680();

		if (!DoVblankProcessing)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}

		PCSHELL_Relax();
	}

	delete menu;

	Pause(1);

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	gsub_430680();

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");

	Pad_ClearTriggers(G_SCONTROL);
}

// @Ok
void Shell_ChooseTime(i32 a1, i32 a2)
{
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	if (a2 != 0)
	{
		pMenu->AddEntry("30 seconds");
		pMenu->AddEntry("90 seconds");
	}
	else
	{
		pMenu->AddEntry("60 seconds");
		pMenu->AddEntry("120 seconds");
	}
	pMenu->CentreY();
	pMenu->Zoom(0);
	*(i32*)0x005512EC = 384;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v15 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		if (a1 != 0)
			Shell_DrawTitleBar(128, 38, "High Scores", 1, 0, 150, -21, 29);
		else
			Shell_DrawTitleBar(128, 38, "training", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (a1 == 0 && pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(256, 60, "select the amount of time in", 0, 0x1000);
			Mess_DrawText(256, 72, "which you wish to train!", 0, 0x1000);
			if (pMenu->mLine == 1)
			{
				if (a2 == 0)
				{
					Mess_DrawText(256, 170, "eliminate as many opponents as", 0, 0x1000);
					Mess_DrawText(256, 182, "possible in 120 seconds!", 0, 0x1000);
				}
				else
				{
					Mess_DrawText(256, 170, "collect as many coins as", 0, 0x1000);
					Mess_DrawText(256, 182, "possible in 90 seconds!", 0, 0x1000);
				}
			}
			else if (pMenu->mLine == 0)
			{
				if (a2 != 0)
				{
					Mess_DrawText(256, 170, "collect as many coins as", 0, 0x1000);
					Mess_DrawText(256, 182, "possible in 30 seconds!", 0, 0x1000);
				}
				else
				{
					Mess_DrawText(256, 170, "eliminate as many opponents as", 0, 0x1000);
					Mess_DrawText(256, 182, "possible in 60 seconds!", 0, 0x1000);
				}
			}
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*(i32*)0x0054D38C != 0)
			return;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			delete pMenu;
			Pause(1);
			if (!gPrintStubbed)
				gsub_46CB90((void*)"stubbed out: DrawSync");
			Pad_ClearTriggers(G_SCONTROL);
			return;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			i32 x, y;
			pMenu->GetEntryXY(pMenu->mEntry[pMenu->mLine].name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(pMenu->mEntry[pMenu->mLine].name, x, y, pMenu->mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText || PCSHELL_CheckTriggers(65552, 1, 1)))
			break;
		if (Vblanks == v15)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}
	G_SCONTROL[0].Start.Triggered = 0;
	G_SCONTROL[0].X.Triggered = 0;
	SFX_Play(0x1F, 0x2000, 0);
	i32 v10;
	if (pMenu->mLine == 0)
		v10 = (a2 != 0) ? 30 : 60;
	else if (pMenu->mLine == 1)
		v10 = (a2 != 0) ? 90 : 120;
	else
		print_if_false(0, "Bad time attack time");
	*(i32*)0x00551288 = v10;
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
}

// @Ok
i32 Shell_ChooseTrainingControlType(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_ChooseTrainingControlType() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("kid mode");
	pMenu->AddEntry("standard");
	pMenu->CentreY();
	pMenu->SetLine(1);
	pMenu->Zoom(0);

	i32 v0 = 0;
	SAnimFrame* pAnim = Spool_FindAnim("kiddy", 1);
	i32 v3 = 0;
	i32 v9 = 0;
	i32 v10 = 256;
	i32 v11 = 0;
	i32 v12 = 0;
	i32 v17 = 0;
	i32 v18 = 0;
	i32 IsMouseOverText = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		v17 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		i32 v5 = pMenu->ChoiceIs("kid mode") && v11 == 0;
		// sub_497690(pAnim, 330, v9, v5, v10); // kiddy animation
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v0, 38, "control type", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(256, 168, "select type of control", 0, 0x1000);
			PShell_DefaultText();
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v18 = PShell_MoveTowards(v0, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (v11 != 0)
		{
			v3 += 400;
			if (v3 <= 2048)
				v10 = 256 - (rcossin_tbl[v3 & 0xFFF].sin << 7 >> 12);
			else
			{
				v3 = 2048;
				v10 = 256;
				v9 -= 15;
				if (v9 < -15)
				{
					v12 = 1;
					goto label52;
				}
			}
			goto label41;
		}
		v9 += 15;
		if (v9 > 144)
		{
			v3 += 400;
			v9 = 144;
			if (v3 <= 2048)
				v10 = 256 - (rcossin_tbl[v3 & 0xFFF].sin << 7 >> 12);
			else
			{
				v3 = 2048;
				v10 = 256;
			}
		}
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*(i32*)0x0054D38C != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			i32 x, y;
			pMenu->GetEntryXY(pMenu->mEntry[pMenu->mLine].name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(pMenu->mEntry[pMenu->mLine].name, x, y, pMenu->mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			if (pMenu->mLine != 0)
			{
				if (pMenu->mLine == 1)
					*(u8*)0x0060CFC7 = 0;
			}
			else
				*(u8*)0x0060CFC7 = 1;
			if (pMenu->mLine != 0)
			{
				SFX_Play(0x1F, 0x2000, 0);
				label50:
				v12 = 1;
				goto label52;
			}
			SFX_Play(0x1F, 0x2000, 0);
			v11 = 1;
			v3 = 0;
			v9 = 144;
		}
		if (PCSHELL_CheckTriggers(131616, 1, 1))
			break;
		label41:
		Mess_Update();
		if (Vblanks == v17)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		v0 = v18;
	}
	G_SCONTROL[0].Circle.Triggered = 0;
	SFX_Play(0x23, 0x2000, 0);
	label52:
	delete pMenu;
	Init_KillAll();
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	Redbook_XAStop();
	return v12;
}

// @MEDIUMTODO
void Shell_ChooseTrainingMission(i32)
{
    printf("Shell_ChooseTrainingMission(i32)");
}

// @MEDIUMTODO
void Shell_ComicCollection(void)
{
    printf("Shell_ComicCollection(void)");
}

// @MEDIUMTODO
void Shell_CostumeViewer(void)
{
    printf("Shell_CostumeViewer(void)");
}

// @Ok
i32 Shell_Difficulty(i32 a1)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("kid mode");
	pMenu->AddEntry("easy");
	pMenu->AddEntry("normal");
	pMenu->AddEntry("hard");
	pMenu->CentreY();
	pMenu->SetLine(2);
	pMenu->mY = 93;
	pMenu->Zoom(0);

	i32 v3 = 0;
	i32 v4 = 0;
	SAnimFrame* pAnim = Spool_FindAnim("kiddy", 1);
	i32 v32 = 0;
	i32 v30 = 0;
	i32 v31 = 256;
	i32 v28 = 0;
	i32 v35 = 0;
	i32 v34 = 0;
	i32 v40 = 0;
	i32 v6 = 0;
	i32 v19 = 0;
	i32 mLine = 0;
	i32 IsMouseOverText = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		v40 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		v6 = pMenu->ChoiceIs("kid mode") && v32 == 0;
		// sub_497690(pAnim, 321, v4, v6, v31); // kiddy animation
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		if (a1 != 0)
			Shell_DrawTitleBar(v3, 38, "new game", 1, 0, 150, -21, 29);
		else
			Shell_DrawTitleBar(v3, 38, "difficulty", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(256, 190, "please select difficulty level", 0, 0x1000);
			if (a1 != 0 && v4 == 124)
			{
				i32 v7 = 0;
				while (gSaveGame.field_56[v7] == 0)
				{
					if (++v7 >= 34)
						goto label36;
				}
				Mess_SetTextJustify(1);
				i32 v8 = v35 & 0xFFF;
				v35 += 200;
				i32 sin = rcossin_tbl[v8].sin;
				i32 v10 = ((68 * sin) >> 13) + 94;
				i32 v11 = 350 * (((59 * sin) >> 13) + 98) / 256;
				if (v11 > 255) v11 = -1;
				i32 v13 = 350 * v10 / 256;
				if (350 * v10 / 256 > 255) v13 = -1;
				i32 v14 = 350 * (((21 * sin) >> 13) + 117) / 256;
				if (v14 > 255) v14 = -1;
				Mess_SetRGB(v11, v13, v14, 0);
				i32 v15 = ((45 * sin) >> 13) + 84;
				i32 v16 = 350 * (((29 * sin) >> 13) + 54) / 256;
				if (v16 > 255) v16 = -1;
				i32 v17 = 350 * (((25 * sin) >> 13) + 47) / 256;
				if (v17 > 255) v17 = -1;
				i32 v18 = 350 * v15 / 256;
				if (v18 > 255) v18 = -1;
				Mess_SetRGBBottom(v16, v17, v18);
				Mess_DrawText(230, 28, "Warning!", 0, 0x1000);
				Mess_DrawText(230, 40, "Proceeding will erase", 0, 0x1000);
				Mess_DrawText(230, 52, "unsaved game progress!", 0, 0x1000);
			}
			label36:
			PShell_DefaultText();
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v3 = PShell_MoveTowards(v3, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (v32 == 0)
			break;
		v19 = v28 + 400;
		v28 += 400;
		if (v28 <= 2048)
			v31 = 256 - (rcossin_tbl[v19 & 0xFFF].sin << 7 >> 12);
		else
		{
			v4 -= 15;
			v28 = 2048;
			v31 = 256;
			v30 = v4;
			if (v4 < -15)
			{
				v34 = 1;
				goto label85;
			}
		}
		label69:
		Mess_Update();
		if (Vblanks == v40)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		continue;
	}
	v30 = v4 + 15;
	if (v4 + 15 > 124)
	{
		v30 = 124;
		i32 v20 = v28 + 400;
		v28 += 400;
		if (v28 <= 2048)
			v31 = 256 - (rcossin_tbl[v20 & 0xFFF].sin << 7 >> 12);
		else
		{
			v28 = 2048;
			v31 = 256;
		}
	}
	if (pMenu->mLine > 0x28)
		Pad_ClearTriggers(G_SCONTROL);
	Pad_Update();
	if (*(i32*)0x0054D38C != 0)
		return 0;
	CheckForPadUnplugged();
	mLine = (u8)pMenu->mLine;
	pMenu->Update();
	if (mLine != (u8)pMenu->mLine && pMenu->mLine == 0 && *(u8*)0x00682770 == 0)
	{
		i32 v22 = Rnd(10);
		Redbook_XAPlay(((i32*)0x00554610)[2 * v22], ((i32*)0x00554614)[2 * v22], 0);
	}
	if (PCSHELL_CheckTriggers(256, 1, 1))
	{
		i32 x, y;
		pMenu->GetEntryXY(pMenu->mEntry[pMenu->mLine].name, &x, &y);
		IsMouseOverText = PCSHELL_IsMouseOverText(pMenu->mEntry[pMenu->mLine].name, x, y, pMenu->mJustification);
	}
	if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
	{
		G_SCONTROL[0].Start.Triggered = 0;
		G_SCONTROL[0].X.Triggered = 0;
		i32 v25;
		switch (pMenu->mLine)
		{
		case 0:
			v25 = 0;
			*(u8*)0x0060CFC7 = 1;
			DifficultyLevel = 0;
			goto label65;
		case 1:
			v25 = 1;
			goto label63;
		case 2:
			v25 = 2;
			goto label63;
		case 3:
			v25 = 3;
			label63:
			DifficultyLevel = v25;
			goto label64;
		default:
			v25 = DifficultyLevel;
			if (DifficultyLevel != 0)
				label64:
				*(u8*)0x0060CFC7 = 0;
			else
				*(u8*)0x0060CFC7 = 1;
			label65:
			gSaveGame.mDifficulty = v25;
			if (pMenu->mLine != 0)
			{
				SFX_Play(0x1F, 0x2000, 0);
				v34 = 1;
				goto label84;
			}
			SFX_Play(0x1F, 0x2000, 0);
			v32 = 1;
			v28 = 0;
			v30 = 124;
			break;
		}
	}
	if (!PCSHELL_CheckTriggers(131616, 1, 1))
		goto label69;
	G_SCONTROL[0].Circle.Triggered = 0;
	SFX_Play(0x23, 0x2000, 0);
	label84:
	label85:
	delete pMenu;
	Init_KillAll();
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	Redbook_XAStop();
	if (v34 != 0)
		gSaveGame.field_78 = 1;
	return v34;
}

// @Ok
void Shell_DisplayGameInfo(
		i32 a1,
		i32 a2,
		SSaveGame* pSave)
{
	if (pSave->mChecksum)
	{
		u32 checksum = Shell_CalculateGameChecksum(pSave);

		if (pSave->mChecksum == checksum)
		{
			SLevel* pLevel = Front_FindLevel(pSave->field_4);
			if (pLevel)
			{
				Mess_SetTextJustify(1);
				Mess_DrawText(a1, a2, pLevel->mDisplayName, 0, 0x1000);

				const char* v9 = gRenderBuf;

				switch (pSave->mDifficulty)
				{
					case 0:
						v9 = "kid mode";
						break;
					case 1:
						v9 = "easy";
						break;
					case 2:
						v9 = "normal";
						break;
					case 3:
						v9 = "hard";
						break;
				}

				Mess_DrawText(a1, a2 + 15, "difficulty:", 0, 0x1000);
				Mess_DrawText(a1 + 150, a2 + 15, v9, 0, 0x1000);
			}
		}
	}
}

// @MEDIUMTODO
void Shell_DoShell(const u32 *,u32 *)
{
    printf("Shell_DoShell(u32 const *,u32 *)");
}

// @Ok
// @Matching
void Shell_DrawBackground(void)
{
	if (!gBackgroundAnimFrame)
		Spool_AnimAccess("menubg", &gBackgroundAnimFrame);

	PCPanel_DrawTexturedPoly(-1.0, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
}

// @Ok
// @Matching
void Shell_DrawTitleBar(
		i32 a1,
		i32 a2,
		const char * a3,
		i32 a4,
		i32 a5,
		i32 a6,
		i32 a7,
		i32 a8)
{
	Mess_SetTextJustify(0);
	Mess_SetRGB(0x80u, 0x80u, 0x80u, 0);
	Mess_SetRGBBottom(0x45u, 60, 107);
	if (a4)
		PShell_BigFont();
	Mess_SetSort(4094);
	Mess_DrawText(a1, a2, a3, 0, 0x1000u);

	i32 v8;
	if ( a5 < a1 )
		v8 = a6 + a1 + Mess_TextWidth(a3) / 2 - a5;
	else
		v8 = a1 - Mess_TextWidth(a3) / 2 - a5 - a6;
	PShell_DrawHighlight(a5, a7 + a2, v8, a8);
	Mess_SetSort(0);
	if (a4)
		PShell_NormalFont();
}

// @Ok
i32 Shell_Gallery(EShellResult a1)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	i32 v9 = 0;
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("Character viewer");
	pMenu->AddEntry("movie viewer");
	pMenu->AddEntry("comic collection");
	pMenu->AddEntry("game covers");
	pMenu->AddEntry("storyboards");
	pMenu->CentreY();
	pMenu->Zoom(0);
	switch (a1)
	{
	case 8:
		pMenu->SetLine(0);
		break;
	case 9:
		pMenu->SetLine(1);
		break;
	case 10:
		pMenu->SetLine(2);
		break;
	case 11:
		pMenu->SetLine(3);
		break;
	case 12:
		pMenu->SetLine(4);
		break;
	default:
		print_if_false(0, "Bad default");
		break;
	}
	if (gSaveGame.mCheatStoryboardFlag == 0)
	{
		pMenu->SetRedText(4);
		pMenu->mEntry[4].unk_c = 100;
		pMenu->mEntry[4].field_11 = 64;
		pMenu->mEntry[4].field_14 = 100;
		pMenu->mEntry[4].field_17 = 64;
	}

	i32 v4 = 0;
	i32 circleExit = 0;
	*(i32*)0x005512EC = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v12 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v4, 38, "gallery", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			switch (pMenu->mLine)
			{
			case 0:
				Mess_DrawText(256, 187, "view character models from the game", 0, 0x1000);
				break;
			case 1:
				Mess_DrawText(256, 187, "view movies from the game", 0, 0x1000);
				break;
			case 2:
				Mess_DrawText(256, 187, "view comics collected from the game", 0, 0x1000);
				break;
			case 3:
				Mess_DrawText(256, 187, "view the game covers", 0, 0x1000);
				break;
			case 4:
				Mess_DrawText(256, 187, "view the original storyboards for", 0, 0x1000);
				Mess_DrawText(256, 200, "creating the game movies", 0, 0x1000);
				break;
			default:
				break;
			}
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v4 = PShell_MoveTowards(v4, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			circleExit = 1;
			break;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			u8 mJustification = pMenu->mJustification;
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			switch (pMenu->mLine)
			{
			case 0:
				v9 = 8;
				break;
			case 1:
				v9 = 9;
				break;
			case 2:
				v9 = 10;
				break;
			case 3:
				v9 = 11;
				break;
			case 4:
				if (gSaveGame.mCheatStoryboardFlag != 0)
					v9 = 12;
				break;
			default:
				break;
			}
			if (v9 != 0)
			{
				SFX_Play(0x1F, 0x2000, 0);
				break;
			}
			SFX_Play(0x1B, 0x2000, 0);
		}
		if (Vblanks == v12)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}

	if (circleExit)
	{
		G_SCONTROL[0].Circle.Triggered = 0;
		SFX_Play(0x23, 0x2000, 0);
	}
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v9;
}

// @MEDIUMTODO
void Shell_GameCovers(void)
{
    printf("Shell_GameCovers(void)");
}

// @MEDIUMTODO
i32 Shell_InputName(char *,i32,i32, const char *)
{
    printf("Shell_InputName(char *,i32,i32,char *)");
	return 0x27092024;
}

EXPORT u8 gInLegalScreen;

// @Ok
void Shell_LegalScreen(void)
{
	if (!gInLegalScreen)
	{
		gInLegalScreen = 1;
		Front_ClearScreen();

		DrawSync();
		Pad_ClearTriggers(gSControl);
		Pad_Update();
		Pad_ClearTriggers(gSControl);

		Sprite2* v0 = new Sprite2("LegalPC.bmp", 1, 0, 0, 3);
		u32 v3 = Vblanks + 180;
		while ( 1 )
		{
			if (!gSceneRelated)
				PCGfx_BeginScene(1u, -1);

			v0->draw(
				0,
				0,
				8,
				-1.0f);
			if (gSceneRelated)
				PCGfx_EndScene(1);
			++TTime;
			Pad_Update();

			if (Vblanks > v3)
				break;

			PCSHELL_Relax();
		}

		delete v0;
		Mess_DeleteAll();
		Front_ClearScreen();

		DrawSync();
		Pad_ClearTriggers(gSControl);
	}
}

static i32 * const gShowAllLevels = (i32*)0x0060CFD8;

// @Ok
i32 Shell_LevelSelect(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	i32 v18 = 0;
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(0, 0, 1, 256, 256, 13);
	pMenu->AdjustWidth(10);

	// find the lowest completion byte among the 34 levels
	i32 v2 = 1000000;
	for (i32 i = 0; i < 34; i++)
	{
		if ((u8)gSaveGame.field_56[i] < v2)
			v2 = (u8)gSaveGame.field_56[i];
	}
	// find the first level with that (lowest) completion byte
	i32 v4 = 0;
	i32 v5 = 0;
	while ((u8)gSaveGame.field_56[v5] != v2)
	{
		if (++v5 >= 34)
			break;
	}
	v4 = v5;

	for (i32 v6 = 0; v6 < 34; v6++)
	{
		print_if_false(*Levels[v6].mDisplayName != 0, "Bad level name");
		if (*gShowAllLevels != 0 || gSaveGame.field_56[v6] != 0 || v6 == v4)
			pMenu->AddEntry(Levels[v6].mDisplayName);
	}
	pMenu->scrollbar_one = 1;
	pMenu->field_1B = 9;
	pMenu->scrollbar_zero = 0;
	pMenu->CentreX();
	pMenu->CentreY();
	if (pMenu->mNumLines > 9)
		pMenu->Zoom(2);
	else
		pMenu->Zoom(1);

	i32 v9 = 0;
	const char* v14 = 0;
	*(i32*)0x005512EC = 384;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v22 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v9, 38, "level select", 1, 0, 150, -21, 29);
		pMenu->Display();
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v9 = PShell_MoveTowards(v9, 128);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			goto done;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			u8 mJustification = pMenu->mJustification;
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
			break;
		if (Vblanks == v22)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}

	// a level was selected
	G_SCONTROL[0].Start.Triggered = 0;
	G_SCONTROL[0].X.Triggered = 0;
	v14 = pMenu->mEntry[pMenu->mLine].name;
	gSaveGame.field_4[0] = 0;
	if (*Levels[0].mDisplayName != 0)
	{
		i32 idx = -1;
		for (i32 li = 0; li < 34; li++)
		{
			if (Utils_CompareStrings(v14, Levels[li].mDisplayName) == 0)
			{
				idx = li;
				break;
			}
			if (*Levels[li + 1].mDisplayName == 0)
				break;
		}
		if (idx != -1)
			Utils_CopyString(Levels[idx].mName, gSaveGame.field_4, 9);
	}
	gSaveGame.mRestartPointName[0] = 0;
	*(i32*)((char*)&gSaveGame + 0x50) = 0;
	*(u8*)((char*)&gSaveGame + 0x79) = 0;
	*(i32*)((char*)&gSaveGame + 0x48) = 0;
	*(i32*)((char*)&gSaveGame + 0x4C) = 0;
	v18 = 1;
	SFX_Play(0x1F, 0x2000, 0);

done:
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v18;
}

// @Ok
// buffer the Merge loop writes merged records into, then copies back
static char * const gMergeBuffer = (char*)0x00610790;
// alt texture set table for Spidey_LoadAlternativeTextureSet
static u32 * const gAltTextureSet = (u32*)0x00552830;
// mouse-related flag used by the input helper below
static u8 * const gMouseRelated = (u8*)0x005FAE9D;

// inlines sub_440E40: splits pad/mouse triggers into per-button flags
static void Shell_ReadTriggers(i32 *pSelect, i32 *pBack, i32 *pAny, i32 *pMouse)
{
	i32 mask = 0x40000;
	*pMouse = 0;
	*pAny = 0;
	*pBack = 0;
	*pSelect = 0;
	if (*gMouseRelated != 0)
		mask = 262208;
	*pMouse = PCSHELL_CheckTriggers(mask, 1, 1);
	*pSelect = PCSHELL_CheckTriggers(65552, 1, 1) || *pMouse;
	*pBack = PCSHELL_CheckTriggers(131616, 1, 1);
	*pAny = *pBack || *pSelect;
	if (*pSelect)
		*pBack = 0;
}

i32 Shell_LoadGame(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_LoadGame() without shell initialised");
	i32 v0 = 0;
	i32 state = 0;
	i32 exiting = 0;
	CMenu* pMenu = 0;
	i32 introCount = 0;
	i32 delay = 0;
	i32 result = 0;
	*(i32*)0x005512EC = 384;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 vblanks = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v0, 38, "load game", 1, 0, 150, -21, 29);
		Mess_SetRGB(0x6B, 0x5D, 0xA7, 0);
		Mess_SetRGBBottom(0x3E, 54, 96);
		Mess_SetSort(0);
		switch (state)
		{
		case 2:
			Mess_DrawText(256, 96, "save game file contains", 0, 0x1000);
			Mess_DrawText(256, 113, "no spider-man game save.", 0, 0x1000);
			Mess_DrawText(256, 130, "press esc key", 0, 0x1000);
			Mess_DrawText(256, 147, "to cancel.", 0, 0x1000);
			break;
		case 3:
			Mess_DrawText(256, 90, "error reading save game file", 0, 0x1000);
			Mess_DrawText(256, 124, "press esc key", 0, 0x1000);
			Mess_DrawText(256, 141, "to cancel.", 0, 0x1000);
			break;
		case 5:
			if (pMenu->FinishedZooming())
			{
				Mess_SetTextJustify(0);
				Mess_SetRGB(0x45, 0x3C, 0x6B, 0);
				Mess_SetRGBBottom(0x28, 35, 62);
				Shell_DisplayGameInfo(190, 70, &gSaveGameSlots[pMenu->mLine]);
			}
			print_if_false(pMenu != 0, "No games menu?");
			pMenu->Display();
			break;
		case 6:
			PShell_BigFont();
			Mess_DrawText(256, 101, "load successful", 0, 0x1000);
			PShell_NormalFont();
			Mess_DrawText(256, 143, "press enter to continue.", 0, 0x1000);
			break;
		default:
			break;
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		i32 v25 = PShell_MoveTowards(v0, 128);
		if ((++TTime & 1) != 0)
			Card_CheckStatus(0, 0);
		if (pMenu != 0 && pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		i32 v22, v30, v23, v26;
		Shell_ReadTriggers(&v22, &v30, &v23, &v26);
		if (pMenu != 0 && pMenu->mLine >= 0x28)
		{
			v22 = 0;
			v26 = 0;
		}
		i32 IsMouseOverText = 0;
		switch (state)
		{
		case 0:
			if (introCount != 0)
			{
				if (--introCount == 0)
				{
					delay = 150;
					state = 1;
				}
			}
			else
			{
				introCount = 10;
			}
			goto vblank;
		case 1:
			switch (CardStatus)
			{
			case -2:
				state = 4;
				break;
			case -1:
				state = 7;
				break;
			case 1:
			{
				i32 v5 = Card_Load();
				if (v5 != 0)
				{
					if (v5 != 1)
					{
						state = 3;
						break;
					}
					state = 2;
					break;
				}
				print_if_false(pMenu == 0, "Already got games menu?");
				PShell_NormalFont();
				pMenu = new CMenu(90, 70, 0, 256, 256, 15);
				*(u8*)((char*)pMenu + 0x18) = 1;
				Shell_AddGameSlots(pMenu);
				pMenu->Zoom(0);
				state = 5;
				break;
			}
			default:
				if (delay == 0)
				{
					state = 2;
					break;
				}
				--delay;
				break;
			}
			goto vblank;
		case 2:
		case 3:
		case 4:
			if (CardStatus == -1 || CardStatus == 2)
			{
				state = 0;
				goto vblank;
			}
			if (v23 != 0 || PCSHELL_CheckTriggers(256, 1, 1))
			{
				SFX_Play(0x23, 0x2000, 0);
				exiting = 1;
			}
			goto vblank;
		case 5:
			print_if_false(pMenu != 0, "No games menu?");
			if (CardStatus == -1)
			{
				delete pMenu;
				pMenu = 0;
				state = 7;
			}
			else
			{
				pMenu->Update();
				if (PCSHELL_CheckTriggers(256, 1, 1))
				{
					i32 mLine = pMenu->mLine;
					u8 mJust = pMenu->mJustification;
					const char* name = pMenu->mEntry[mLine].name;
					i32 x, y;
					pMenu->GetEntryXY(name, &x, &y);
					IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJust);
				}
				if (v22 || IsMouseOverText)
				{
					char* slot = (char*)&gSaveGameSlots[pMenu->mLine];
					if (*(u32*)slot == 0)
					{
						SFX_Play(0x1B, 0x2000, 0);
					}
					else
					{
						print_if_false(1, "Size of SSaveGame not a multiple of 4");
						i32 checksum = 0;
						u32* data = (u32*)(slot + 4);
						for (i32 i = 0; i < 46; i++)
						{
							u32 c = (u32)checksum;
							u32 doubled = (c << 1) | (c >> 31);
							checksum = (i32)(data[i] + doubled);
						}
						checksum |= 1;
						if (*(u32*)slot == (u32)checksum)
						{
							SFX_Play(0x1F, 0x2000, 0);
							memcpy(&gSaveGame, slot, sizeof(gSaveGame));
							for (i32 j = 0; j < NUM_CHALLS; j++)
							{
								Merge((SScore*)(gMergeBuffer + j * 25 + 3), &gGlobalRecords.mScores[j * 5], gChallenges[j].field_C);
							}
							gGlobalRecords = *(SRecords*)gMergeBuffer;
							PShell_ApplyGameState();
							Spidey_LoadAlternativeTextureSet(gAltTextureSet, gSaveGame.field_7C + 1);
							state = 6;
							Pad_ClearTriggers(G_SCONTROL);
						}
						else
						{
							SFX_Play(0x1B, 0x2000, 0);
						}
					}
				}
			}
			goto vblank;
		case 6:
			if (v23 != 0 || PCSHELL_CheckTriggers(256, 1, 1))
			{
				SFX_Play(0x1F, 0x2000, 0);
				result = 1;
				exiting = 1;
			}
			goto vblank;
		case 7:
			if (CardStatus == -2 || (CardStatus > 0 && CardStatus <= 2))
			{
				state = 0;
				goto vblank;
			}
			else if (G_SCONTROL[0].Circle.Triggered != 0)
			{
				G_SCONTROL[0].Circle.Triggered = 0;
				SFX_Play(0x23, 0x2000, 0);
				exiting = 1;
			}
			goto vblank;
		}
	vblank:
		if (v30 != 0 && exiting == 0)
		{
			SFX_Play(0x23, 0x2000, 0);
			exiting = 1;
		}
		if (Vblanks == vblanks)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		if (exiting == 0)
		{
			v0 = v25;
			continue;
		}
		delete pMenu;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		Pad_ClearTriggers(G_SCONTROL);
		return result;
	}
}

// @MEDIUMTODO
void Shell_MainMenu(EShellResult)
{
    printf("Shell_MainMenu(EShellResult)");
}

// @Ok
i32 Shell_MemoryCard(EShellResult a1)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MemoryCard() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("load game data");
	pMenu->AddEntry("Save game data");
	pMenu->CentreY();
	pMenu->Zoom(0);
	if (a1 == 20)
		pMenu->SetLine(0);
	else if (a1 == 21)
		pMenu->SetLine(1);
	else
		print_if_false(0, "Bad default sent to Shell_MemoryCard()");

	i32 v4 = 0;
	i32 v13 = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v11 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v4, 38, "FILE SYSTEM", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			Mess_DrawText(256, 168, "please select to load", 0, 0x1000);
			Mess_DrawText(256, 184, "or save game data", 0, 0x1000);
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v4 = PShell_MoveTowards(v4, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			u8 mJustification = pMenu->mJustification;
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			SFX_Play(0x1F, 0x2000, 0);
			if (pMenu->ChoiceIs("load game data"))
			{
				v13 = 20;
				goto done;
			}
			if (pMenu->ChoiceIs("Save game data"))
			{
				v13 = 21;
				goto done;
			}
		}
		if (PCSHELL_CheckTriggers(49164, 1, 1))
		{
			G_SCONTROL[0].Right.Triggered = 0;
			G_SCONTROL[0].Left.Triggered = 0;
		}
		if (PCSHELL_CheckTriggers(131616, 1, 1))
			break;
		if ((G_SCONTROL[0].X.Triggered != 0 || G_SCONTROL[0].Start.Triggered != 0) && Vblanks == v11)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}
	G_SCONTROL[0].Circle.Triggered = 0;
	SFX_Play(0x23, 0x2000, 0);

done:
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v13;
}

struct SMovieEntry
{
	char* pDisplayName;
	char* pFileName;
	i32 mUnlockId;
	i32 field_C;
};
static SMovieEntry* const gMovieTable = (SMovieEntry*)0x00554448;

// @Ok
void Shell_MovieViewer(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(188, 73, 1, 256, 256, 13);
	pMenu->AdjustWidth(10);
	print_if_false(gMovieTable[0].mUnlockId != -1, "First movie not found!");
	pMenu->AddEntry(gMovieTable[0].pDisplayName);
	for (i32 i = 1; i < 21; i++)
	{
		if (gMovieTable[i].mUnlockId != -1 && ((1 << gMovieTable[i].mUnlockId) & gSaveGame.field_88) != 0)
			pMenu->AddEntry(gMovieTable[i].pDisplayName);
	}
	pMenu->scrollbar_one = 1;
	pMenu->field_1B = 9;
	pMenu->scrollbar_zero = 0;
	pMenu->CentreX();
	pMenu->CentreY();
	if (pMenu->mNumLines > 9)
		pMenu->Zoom(2);
	else
		pMenu->Zoom(1);

	i32 v0 = 0;
	i32 cdNotFound = 0;
	i32 exitMenu = 0;
	i32 playId = -1;
	*(i32*)0x005512EC = 384;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v15 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v0, 38, "movie viewer", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (cdNotFound)
		{
			Mess_SetRGB(0x80, 0x80, 0x80, 0);
			Mess_SetTextJustify(0);
			Mess_DrawText(256, 210, "cd not found - insert the cd", 0, 0x1000);
			Mess_DrawText(256, 225, "then select a movie to view", 0, 0x1000);
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v0 = PShell_MoveTowards(v0, 128);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			exitMenu = 1;
		}
		else
		{
			i32 IsMouseOverText = 0;
			if (PCSHELL_CheckTriggers(256, 1, 1))
			{
				u8 mJustification = pMenu->mJustification;
				const char* name = pMenu->mEntry[pMenu->mLine].name;
				i32 x, y;
				pMenu->GetEntryXY(name, &x, &y);
				IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
			}
			if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
			{
				G_SCONTROL[0].Start.Triggered = 0;
				G_SCONTROL[0].X.Triggered = 0;
				playId = -1;
				const char* selName = pMenu->mEntry[pMenu->mLine].name;
				for (i32 i = 0; i < 21; i++)
				{
					if (Utils_CompareStrings(selName, gMovieTable[i].pDisplayName) == 0)
						playId = gMovieTable[i].mUnlockId;
				}
				if (playId != -1)
					SFX_Play(0x1F, 0x2000, 0);
				else
					SFX_Play(0x1B, 0x2000, 0);
			}
		}
		if (exitMenu)
			break;
		if (playId != -1)
		{
			u8 r = GameFMV_PlayMovie((u8)playId, 1, 1, 1.0f);
			Pad_IdleTime = 0;
			cdNotFound = (r == 0);
			playId = -1;
			continue;
		}
		if (Vblanks == v15)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}

	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
}

// @Ok
i32 Shell_Options(EShellResult a1)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	i32 v9 = 0;
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(254, 97, 0, 256, 256, 16);
	pMenu->AddEntry("keyboard configuration");
	pMenu->AddEntry("joystick configuration");
	pMenu->AddEntry("music and sound");
	pMenu->AddEntry("display options");
	pMenu->AddEntry("file system");
	i32 v4 = 18;
	if (PCINPUT_GetNumControllerButtons() == 0)
	{
		pMenu->EntryOff("joystick configuration");
		v4 = 0;
	}
	switch (a1)
	{
	case 3:
		pMenu->SetLine(4);
		break;
	case 14:
		pMenu->SetLine(0);
		break;
	case 15:
		if (pMenu->mEntry[1].unk_b != 0)
			pMenu->SetLine(1);
		else
			pMenu->SetLine(0);
		break;
	case 16:
		pMenu->SetLine(2);
		break;
	case 17:
		pMenu->SetLine(3);
		break;
	default:
		print_if_false(0, "Bad default sent to Shell_Config");
		break;
	}
	pMenu->Zoom(0);

	i32 v1 = 0;
	*(i32*)0x005512EC = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v12 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v1, 38, "options", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			switch (pMenu->mLine)
			{
			case 0:
				Mess_DrawText(256, v4 + 180, "change keyboard configuration", 0, 0x1000);
				break;
			case 1:
				Mess_DrawText(256, v4 + 180, "change button configuration", 0, 0x1000);
				Mess_DrawText(256, v4 + 193, "and controller settings", 0, 0x1000);
				break;
			case 2:
				Mess_DrawText(256, v4 + 180, "adjust volume settings", 0, 0x1000);
				Mess_DrawText(256, v4 + 193, "for music and sound", 0, 0x1000);
				break;
			case 3:
				Mess_DrawText(256, v4 + 180, "change display settings", 0, 0x1000);
				break;
			case 4:
				Mess_DrawText(256, v4 + 180, "load or save a game", 0, 0x1000);
				break;
			default:
				break;
			}
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		i32 v13 = PShell_MoveTowards(v1, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		PShell_NormalFont();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			goto done;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			u8 mJustification = pMenu->mJustification;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
			break;
		if (Vblanks == v12)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		v1 = v13;
	}

	G_SCONTROL[0].Start.Triggered = 0;
	G_SCONTROL[0].X.Triggered = 0;
	SFX_Play(0x1F, 0x2000, 0);
	if (pMenu->ChoiceIs("keyboard configuration"))
		v9 = 14;
	if (pMenu->ChoiceIs("display options"))
		v9 = 17;
	if (pMenu->ChoiceIs("file system"))
		v9 = 3;
	if (pMenu->ChoiceIs("joystick configuration"))
		v9 = 15;
	if (pMenu->ChoiceIs("music and sound"))
		v9 = 16;

done:
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v9;
}

// @MEDIUMTODO
void Shell_RollCredits(void)
{
    printf("Shell_RollCredits(void)");
}

// @FIXME forward to original
EXPORT void DrawSlider(i32 a1, i32 a2, i32 a3, i32 a4)
{
	typedef void (*func_ptr)(i32, i32, i32, i32);
	func_ptr func = (func_ptr)0x00498060;
	func(a1, a2, a3, a4);
}
// @FIXME forward to original
EXPORT i32 SliderDrag(i32 a1, i32 a2, i32 a3)
{
	typedef i32 (*func_ptr)(i32, i32, i32);
	func_ptr func = (func_ptr)0x00497F80;
	return func(a1, a2, a3);
}
// @FIXME forward to original
EXPORT void sub_515850(void)
{
	typedef void (*func_ptr)(void);
	func_ptr func = (func_ptr)0x00515850;
	func();
}

// @Ok
void Shell_SFXMusic(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(270, 90, 2, 256, 256, 20);
	pMenu->AddEntry("music and sfx level");
	pMenu->AddEntry("voice level");
	pMenu->AddEntry("movie level");
	pMenu->AddEntry("audio");
	pMenu->AddEntry("initial settings");
	pMenu->scrollbar_zero = 0;
	pMenu->Zoom(0);

	i32 v25 = 0;
	i32 v24 = 0;
	i32 v3 = 0;
	*(i32*)0x005512EC = 512;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v29 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v3, 38, "music and sound", 1, 0, 150, -21, 29);
		if (pMenu->FinishedZooming())
		{
			DrawSlider(270, 81, (gGameState[12] << 8) / 0x3FFF, pMenu->mLine != 0);
			DrawSlider(270, 101, gGameState[13], pMenu->mLine != 1);
			DrawSlider(270, 121, gGameState[11], pMenu->mLine != 2);
			if (pMenu->mLine == 3)
				PShell_DefaultText();
			else
			{
				Mess_SetTextJustify(0);
				Mess_SetRGB(0x45, 0x3C, 0x6B, 0);
				Mess_SetRGBBottom(0x28, 35, 62);
			}
			PShell_SmallFont();
			if (gBootRomSoundMode != 0)
				Mess_DrawText(368, 149, "mono", 0, 0x1000);
			else
				Mess_DrawText(368, 149, "stereo", 0, 0x1000);
			PShell_NormalFont();
		}
		pMenu->Display();
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		i32 v30 = PShell_MoveTowards(v3, 150);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Mess_Update();
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return;
		CheckForPadUnplugged();
		i32 v23 = 0;
		i32 v5 = 0;
		if (PCSHELL_CheckTriggers(256, 1, 0))
		{
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			u8 mJustification = pMenu->mJustification;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			if (PCSHELL_IsMouseOverText(name, x, y, mJustification))
				v5 = 1;
		}
		if (pMenu->mLine < 0x28 && (v5 != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].X.Triggered = 0;
			v23 = 1;
		}
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			Mess_DeleteAll();
			delete pMenu;
			Pause(1);
			if (!gPrintStubbed)
				gsub_46CB90((void*)"stubbed out: DrawSync");
			Pad_ClearTriggers(G_SCONTROL);
			*(i32*)0x005512EC = 512;
			sub_515850();
			return;
		}
		PShell_NormalFont();
		pMenu->Update();
		i32 v7 = 0;
		if (*(u8*)0x006A7784 == 0)
		{
			i32 v8 = 0;
			for (i32 i = 81; i < 141; i += 20)
			{
				if (PCSHELL_IsMouseOver(270, i, 480, i + 10))
					pMenu->SetLine(v8);
				v8++;
			}
		}
		i32 mLine = pMenu->mLine;
		if (mLine != 0)
		{
			if (mLine == 1)
				v7 = SliderDrag(270, 101, gGameState[13]);
			else if (mLine == 2)
				v7 = SliderDrag(270, 121, gGameState[11]);
		}
		else
			v7 = SliderDrag(270, 81, (gGameState[12] << 8) / 0x3FFF);

		if (PCSHELL_CheckTriggers(49164, 0, 0))
		{
			if (v24 == 0 || (v24 > 20 && (v24 & 1) == 0))
			{
				if (PCSHELL_CheckTriggers(32776, 0, 0))
					v7 = 1;
				if (PCSHELL_CheckTriggers(16388, 0, 0))
					v7 = -1;
			}
			v24++;
		}
		else
			v24 = 0;

		if (pMenu->mLine != 1 && v25 != 0)
			Redbook_XAStop();

		switch (pMenu->mLine)
		{
		case 0:
			if (v7 != 0)
			{
				i32 v12 = (v7 << 14) / 16;
				i32 v14 = v12 + gGameState[12];
				gGameState[12] += v12;
				if (v14 < 0)
					v14 = 0;
				else if (v14 > 0x3FFF)
					v14 = 0x3FFF;
				gGameState[12] = v14;
				gSaveGame.field_94 = v14;
			}
			if (v23 != 0)
				SFX_Play(0x1C, 0x2000, 0);
			break;
		case 1:
			if (v7 != 0)
			{
				i32 v16 = (v7 << 8) / 16;
				i32 v15 = v16 + gGameState[13];
				gGameState[13] += v16;
				if (v15 < 0)
					v15 = 0;
				else if (v15 > 255)
					v15 = 255;
				gGameState[13] = v15;
				gSaveGame.field_9C = v15;
				v25 = 1;
			}
			if (v23 != 0 || v25 == 0)
			{
				if (Rnd(2) != 0)
					Redbook_XAPlay(58, 8, 0);
				else
					Redbook_XAPlay(64, 12, 0);
			}
			break;
		case 2:
			{
				i32 v16 = (v7 << 8) / 16;
				i32 v17 = v16 + gGameState[11];
				gGameState[11] += v16;
				if (v17 < 0)
					v17 = 0;
				else if (v17 > 255)
					v17 = 255;
				gGameState[11] = v17;
				gSaveGame.field_98 = v17;
			}
			break;
		case 3:
			if (v23 != 0 || v7 != 0)
			{
				SFX_Play(0x1F, 0x2000, 0);
				DCSetBootROMSoundMode(gBootRomSoundMode == 0);
				gSaveGame.field_A0 = gBootRomSoundMode;
			}
			break;
		case 4:
			if (v23 != 0)
			{
				SFX_Play(0x1F, 0x2000, 0);
				gGameState[12] = 11087;
				gGameState[11] = 177;
				gGameState[13] = 201;
				gSaveGame.field_94 = 11087;
				gSaveGame.field_98 = 177;
				gSaveGame.field_9C = 201;
			}
			break;
		default:
			break;
		}
		if (pMenu->mLine != 1)
			v25 = 0;
		if (Vblanks == v29)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		v3 = v30;
	}
}

// @MEDIUMTODO
void Shell_SaveGame(const u32 *,u32 *)
{
    printf("Shell_SaveGame(u32 const *,u32 *)");
}

// Shell_ScreenAdjust and Shell_ShowRecord call these as real out-of-line functions
// in the original, keep the MSVC inliner away (same trick as PCShell.cpp's
// gsub_430680/gsub_430880/gsub_515850, needed because these stubs live in the same
// TU as their callers). CheckForPadUnplugged and gShellMenuEase/gShellMenuAbort now
// declared earlier in this file (before the Shell_Choose* family), see there.
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// footer label text pointers, same string-pointer table idiom as pshell.cpp's
// gText* macros. Contents confirmed against the exe's raw .data (0x54B89C ->
// "select", 0x54B8A4 -> "back", 0x54B98C -> "Option 1").
#define gTextSelect (*reinterpret_cast<char**>(0x0054B89C))
#define gTextBack (*reinterpret_cast<char**>(0x0054B8A4))
#define gTextOption1 (*reinterpret_cast<char**>(0x0054B98C))

// unnamed helper called once per screen adjust / show record frame, address 0x498240.
// Draws the footer bar: select/back/Option 1 labels, the two xtri button icons
// (gAnimTable[23], "xtri") plus one Buttons icon (gAnimTable[3], "Buttons "),
// and the highlight strip behind them.
// @Ok
// @Matching
EXPORT void gsub_498240(i32 x, i32 y)
{
	PShell_SmallFont();
	Mess_SetShadowRGB(0xFF);
	Mess_SetTextJustify(0);
	Mess_SetRGB(0x80, 0x80, 0x80, 0);
	Mess_SetRGBBottom(0x45, 0x3C, 0x6B);
	Mess_DrawText(x - 25, y, gTextSelect, 0, 0x1000);
	Mess_DrawText(x + 59, y, gTextBack, 0, 0x1000);
	Mess_DrawText(x - 128, y, gTextOption1, 0, 0x1000);

	POLY_FT4* pPoly = (POLY_FT4*)Panel_DrawTexturedPoly(gAnimTable[23], x - 80, y - 10, G_SORT);
	print_if_false(pPoly != 0, "error");
	if (pPoly)
	{
		pPoly->b0 = 0x80;
		pPoly->g0 = 0x80;
		pPoly->r0 = 0x80;
		DCPanel_DrawTexturedPoly(1.0f, pPoly, gAnimTable[23], x - 85, y - 11, 20, 12, G_SORT, 0);
	}

	pPoly = (POLY_FT4*)Panel_DrawTexturedPoly(gAnimTable[23] + 1, x + 14, y - 10, G_SORT);
	print_if_false(pPoly != 0, "error");
	if (pPoly)
	{
		pPoly->b0 = 0x80;
		pPoly->g0 = 0x80;
		pPoly->r0 = 0x80;
		DCPanel_DrawTexturedPoly(1.0f, pPoly, gAnimTable[23] + 1, x + 9, y - 11, 20, 12, G_SORT, 0);
	}

	pPoly = (POLY_FT4*)Panel_DrawTexturedPoly(gAnimTable[3] + 1, x - 189, y - 10, G_SORT);
	print_if_false(pPoly != 0, "error");
	if (pPoly)
	{
		pPoly->b0 = 0x80;
		pPoly->g0 = 0x80;
		pPoly->r0 = 0x80;
		DCPanel_DrawTexturedPoly(1.0f, pPoly, gAnimTable[3] + 1, x - 194, y - 11, 20, 12, G_SORT, 0);
	}

	PShell_DrawHighlight(0x200, y - 17, x - 832, 24);
	PShell_NormalFont();
	Mess_SetShadowRGB(0x29);
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

static char* STR_SCREEN_ADJUST_TITLE = "screen adjust";
static char* STR_SCREEN_ADJUST_LINE1 = "Use the directional";
static char* STR_SCREEN_ADJUST_LINE2 = "buttons to center";
static char* STR_SCREEN_ADJUST_LINE3 = "the screen";

// @Ok
// @Matching
void Shell_ScreenAdjust(void)
{
	// defined once in PCShell.cpp, called here through a local extern (same pattern as
	// Shell_TitleScreen's gsub_430880 call below).
	extern void gsub_430880(void);
	extern void gsub_430680(void);

	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");

	i32 savedX = DoubleBuffer[0].Disp.screen.x;
	i32 savedY = DoubleBuffer[0].Disp.screen.y;
	i32 cancelled = 0;

	i32 titleEase = 0;
	gShellMenuEase = 0x2C8;

	while (1)
	{
		gsub_430880();
		Db_FlipClear();
		CalcPolyBufferEnd();

		i32 vblanksSnapshot = Vblanks;

		if (!gSceneRelated)
			PCGfx_BeginScene(1u, -1);

		gsub_498240(gShellMenuEase, 0xDE);

		Shell_DrawBackground();

		Shell_DrawTitleBar(titleEase, 0x26, STR_SCREEN_ADJUST_TITLE, 1, 0, 0x96, -21, 0x1D);

		PShell_DefaultText();
		Mess_SetRGB(0x6Bu, 0x5Du, 0xA7u, 0);
		Mess_SetRGBBottom(0x3Eu, 0x36, 0x60);
		Mess_DrawText(0x100, 0x64, STR_SCREEN_ADJUST_LINE1, 0, 0x1000u);
		Mess_DrawText(0x100, 0x75, STR_SCREEN_ADJUST_LINE2, 0, 0x1000u);
		Mess_DrawText(0x100, 0x86, STR_SCREEN_ADJUST_LINE3, 0, 0x1000u);

		if (gSceneRelated)
			PCGfx_EndScene(1);

		titleEase = PShell_MoveTowards(titleEase, 0x80);
		gShellMenuEase = PShell_MoveTowards(gShellMenuEase, 0x180);

		Pad_Update();

		if (*gShellMenuAbort)
			return;

		CheckForPadUnplugged();

		if (G_SCONTROL[0].Right.Pressed && DoubleBuffer[0].Disp.screen.x < 0x20)
		{
			DoubleBuffer[0].Disp.screen.x++;
			DoubleBuffer[1].Disp.screen.x++;
		}

		if (G_SCONTROL[0].Left.Pressed && DoubleBuffer[0].Disp.screen.x > 0)
		{
			DoubleBuffer[0].Disp.screen.x--;
			DoubleBuffer[1].Disp.screen.x--;
		}

		if (G_SCONTROL[0].Up.Pressed && DoubleBuffer[0].Disp.screen.y > 0)
		{
			DoubleBuffer[0].Disp.screen.y--;
			DoubleBuffer[1].Disp.screen.y--;
			G_SCONTROL[0].Up.Triggered = 0;
		}

		if (G_SCONTROL[0].Down.Pressed && DoubleBuffer[0].Disp.screen.y < 0x20)
		{
			DoubleBuffer[0].Disp.screen.y++;
			DoubleBuffer[1].Disp.screen.y++;
			G_SCONTROL[0].Down.Triggered = 0;
		}

		if (G_SCONTROL[0].Circle.Triggered)
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			cancelled = 1;
			break;
		}

		if (G_SCONTROL[0].X.Triggered || G_SCONTROL[0].Start.Triggered)
		{
			SControl* pad = G_SCONTROL;
			pad[0].Start.Triggered = 0;
			pad[0].X.Triggered = 0;
			SFX_Play(0x1F, 0x2000, 0);
			break;
		}

		if (Vblanks == vblanksSnapshot)
			Pause(1);

		*(volatile i32*)&DoVblankProcessing = 0;
		Pause(1);
		DrawSync();
		gsub_430680();

		if (!*(volatile i32*)&DoVblankProcessing)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}

		PCSHELL_Relax();
	}

	Pause(1);
	DrawSync();
	gsub_430680();
	DrawSync();
	Pad_ClearTriggers(G_SCONTROL);

	if (cancelled)
	{
		DoubleBuffer[0].Disp.screen.x = savedX;
		DoubleBuffer[0].Disp.screen.y = savedY;
		DoubleBuffer[1].Disp.screen.x = savedX;
		DoubleBuffer[1].Disp.screen.y = savedY;
	}

	*reinterpret_cast<i32*>(&gSaveGame.field_A4) = DoubleBuffer[0].Disp.screen.x;
	*reinterpret_cast<i32*>(&gSaveGame.field_A8) = DoubleBuffer[0].Disp.screen.y;

	gShellMenuEase = 0x200;
}

// CRecordBox is now declared in shell.h (moved 2026-08-27, pshell.cpp needs
// it too for the end-of-training record box; see the comment on the class).
// The constructor/destructor moved to pshell.cpp on 2026-08-27 (see the
// comment there): IDA on the real exe showed Shell_ShowRecord's SEH cleanup
// frame around `new CRecordBox(...)` needs the constructor's body to be
// INVISIBLE to the compiler at the call site, not just un-inlined.

// column header / score-unit label strings, read directly out of the
// original SpideyPC.exe .data section (they're stored as real char*
// globals, not inline literals: the original loads them with
// "mov eax,[54B8xxh]", a dereference, not "push offset"). Addresses:
// Place 0x54B8C8, Name 0x54B8CC, Time 0x54B8D0, Kills 0x54B8D4,
// Items 0x54B8D8, Points 0x54B8DC, "---" 0x54B8E0. We don't pin our own
// globals to those addresses (relocated data addresses are an accepted
// diff per the matching discipline), just keep the same string content.
static char* gRecordPlaceLabel = "Place";
static char* gRecordNameLabel = "Name";
static char* gRecordTimeLabel = "Time";
static char* gRecordKillsLabel = "Kills";
static char* gRecordItemsLabel = "Items";
static char* gRecordPointsLabel = "Points";
static char* gRecordDashesLabel = "---";

// @Ok
// @Matching
void CRecordBox::Display(void)
{
	Mess_SetSort(0);

	if (field_30)
	{
		Mess_SetRGB(0x45, 0x3C, 0x6B, 0);

		Mess_DrawText(field_1C + 0x27, field_20 + 0xF, gRecordPlaceLabel, 0, 0x1000u);
		Mess_DrawText(field_1C + 0x8B, field_20 + 0xF, gRecordNameLabel, 0, 0x1000u);

		switch (field_3C->mScoreUnits)
		{
			case 0:
				Mess_DrawText(field_1C + 0xEF, field_20 + 0xF, gRecordTimeLabel, 0, 0x1000u);
				break;
			case 1:
				Mess_DrawText(field_1C + 0xEF, field_20 + 0xF, gRecordKillsLabel, 0, 0x1000u);
				break;
			case 2:
				Mess_DrawText(field_1C + 0xEF, field_20 + 0xF, gRecordItemsLabel, 0, 0x1000u);
				break;
			case 3:
				Mess_DrawText(field_1C + 0xEF, field_20 + 0xF, gRecordPointsLabel, 0, 0x1000u);
				break;
			default:
				print_if_false(0, "Bad ScoreUnits");
				break;
		}

		i32 y = field_20 + 0x21;

		if (field_34)
			field_34--;

		if (field_35)
			field_35--;

		if (!field_35 && field_36 < 5)
		{
			field_35 = 3;
			field_36++;
			field_34 = 2;
			SFX_Play(0x29, 0x3FFF, 0);
		}

		i32 missionIndex = (reinterpret_cast<u8*>(field_3C) - reinterpret_cast<u8*>(gChallenges)) >> 4;
		SScore* pRow = &gGlobalRecords.mScores[missionIndex * NUM_RECORDS_PER_CHALL];

		for (i32 row = 1; row <= field_36; row++)
		{
			if (row == field_36 && field_34)
				Mess_SetRGB(0xFF, 0xFF, 0xFF, 0);
			else
				Mess_SetRGB(0x60, 0x60, 0x60, 0);

			char rowStr[0xC];
			rowStr[0] = static_cast<char>(row + 0x30);
			rowStr[1] = 0;
			Mess_DrawText(field_1C + 0x27, y, rowStr, 0, 0x1000u);

			if (pRow->field_0)
			{
				char letterBuf[0xC];

				letterBuf[0] = pRow->field_0;
				letterBuf[1] = 0;
				Mess_DrawText(field_1C + 0x76, y, letterBuf, 0, 0x1000u);

				letterBuf[0] = pRow->field_1;
				letterBuf[1] = 0;
				Mess_DrawText(field_1C + 0x8A, y, letterBuf, 0, 0x1000u);

				letterBuf[0] = pRow->field_2;
				letterBuf[1] = 0;
				Mess_DrawText(field_1C + 0x9E, y, letterBuf, 0, 0x1000u);

				DisplayScore(field_1C + 0xEF, y, (pRow->field_4 << 8) + pRow->field_3, field_3C->mScoreUnits);
			}
			else
			{
				Mess_DrawText(field_1C + 0x8B, y, gRecordDashesLabel, 0, 0x1000u);
				Mess_DrawText(field_1C + 0xEF, y, gRecordDashesLabel, 0, 0x1000u);
			}

			y += 0xE;
			pRow++;
		}
	}

	if (field_40 && (*reinterpret_cast<volatile u8*>(0x6B4CA0) & 0x10))
	{
		char cursorStr[0xC];
		cursorStr[0] = '_';
		cursorStr[1] = 0;

		Mess_DrawText(field_1C + mLetterIndex * 20 + 0x76, field_20 + field_39 * 14 + 0x24, cursorStr, 0, 0x1000u);
	}

	// original calls the CExpandingBox widget-frame draw out of line
	// (0x47AF10, CExpandingBox_Display in tools/names.json). CRecordBox's
	// own fields (field_4..field_2C) are laid out exactly like
	// CExpandingBox's (see the CRecordBox ctor comment), so a plain
	// pointer cast reaches the same object; this is a real call, not
	// inlined (CExpandingBox::Display lives in pshell.cpp, a different
	// TU).
	reinterpret_cast<CExpandingBox*>(this)->Display();
}

// @Ok
// @Matching
void CRecordBox::Update(void)
{
	print_if_false(mLetterIndex < 3, "Bad mLetterIndex");

	if (!field_40)
		return;

	if (!field_30)
		return;

	i32 missionIndex = (reinterpret_cast<u8*>(field_3C) - reinterpret_cast<u8*>(gChallenges)) >> 4;
	SScore* pScores = &gGlobalRecords.mScores[missionIndex * NUM_RECORDS_PER_CHALL];

	for (i32 key = 0; key < 0x100; key++)
	{
		if (!PCINPUT_IsKeyPressed(static_cast<u8>(key), 1))
			continue;

		char keyName[0x20];
		DXINPUT_GetKeyName(static_cast<u8>(key), keyName);

		if (strlen(keyName) != 1)
			continue;

		if (keyName[0] < 'A' || keyName[0] > 'Z')
			continue;

		if (static_cast<u8>(mLetterIndex) > 2)
			continue;

		reinterpret_cast<u8*>(&pScores[field_39])[mLetterIndex] = keyName[0];

		mLetterIndex++;

		SFX_Play(0x29, 0x2000, 0);

		if (static_cast<u8>(mLetterIndex) > 2)
			mLetterIndex = 0;
	}

	if (PCSHELL_CheckTriggers(0x50010, 1, 1))
	{
		reinterpret_cast<u8*>(&gGlobalRecords)[0] = reinterpret_cast<u8*>(&pScores[field_39])[0];
		reinterpret_cast<u8*>(&gGlobalRecords)[1] = reinterpret_cast<u8*>(&pScores[field_39])[1];
		reinterpret_cast<u8*>(&gGlobalRecords)[2] = reinterpret_cast<u8*>(&pScores[field_39])[2];

		field_40 = 0;

		SFX_Play(0x1F, 0x2000, 0);
	}

	if (PCSHELL_CheckTriggers(0x4004, 1, 1))
	{
		G_SCONTROL[0].Left.Triggered = 0;

		if (mLetterIndex != 0)
		{
			mLetterIndex--;
			SFX_Play(0x29, 0x2000, 0);
		}
	}

	if (PCSHELL_CheckTriggers(0x8008, 1, 1))
	{
		G_SCONTROL[0].Right.Triggered = 0;

		if (static_cast<u8>(mLetterIndex) < 2)
		{
			mLetterIndex++;
			SFX_Play(0x29, 0x2000, 0);
		}
	}
}

// @SMALLTODO
void CRecordBox::NameEntryOn(u8)
{
	printf("CRecordBox::NameEntryOn(u8)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// tentative name, no idb_globals.txt match near 0x6A7ADC. Holds the CRecordBox
// widget for the current Shell_ShowRecord call. Not a stack local: the
// original reads/writes it via a fixed address across the whole per-frame
// loop, so it has to be a real global (or MSVC would have kept it in a
// register/stack slot).
static CRecordBox* gShowRecordBox;

// tentative default title text, no idb_globals.txt match near 0x54BBA0.
static char* gShowRecordTitle = "High Scores";

// @Ok
// @Matching
// SEH mystery SOLVED 2026-08-27 with IDA (Hex-Rays on the real exe): the
// missing frame was never about ctor complexity (see the 7 failed
// hypotheses this comment used to list, kept in shell.attempts.md).
// CRecordBox::CRecordBox is a trivial straight-line ctor (no calls at all,
// confirmed by decompiling 0x47B1E0), yet the real exe still wraps this
// `new CRecordBox(...)` in the unwind-state frame. The actual trigger:
// whether the constructor's DEFINITION is visible to the compiler in the
// same translation unit as the `new` call. Same-TU visibility (even with
// `#pragma auto_inline(off)`, which only blocks literal inlining, not this
// separate throw analysis) lets MSVC6 prove the ctor can't throw and drop
// the protection; a declaration-only (different-TU) ctor is opaque, so
// MSVC6 always protects it. Reordering the ctor's position WITHIN one TU
// (one of the 7 failed attempts) does not hide it either, since the whole
// TU is visible regardless of declaration order. Fix: moved
// CRecordBox::CRecordBox/~CRecordBox out of shell.cpp into pshell.cpp (a
// different TU), matching the class-precedent CMenu already had by
// accident (CMenu::CMenu lives in front.cpp, never in the same TU as any
// of its callers, which is why Shell_ChooseSurvivalArena's `new CMenu(...)`
// already reproduced the frame). cmpsum: 0 mnemonic diffs.
void Shell_ShowRecord(char const *, char const *, STrainingMission* pMission)
{
	// same pattern as Shell_ScreenAdjust/Shell_TitleScreen above: defined once
	// in PCShell.cpp, called here through a local extern.
	extern void gsub_430880(void);
	extern void gsub_430680(void);

	print_if_false(gShellInitialized != 0, "Called Shell_ShowRecord() without shell initialised");

	Pause(1);

	// DrawSync(), written out by hand instead of calling the ps2funcs.h
	// INLINE version: that one calls export.h's static stubbed_printf, which
	// our compiler inlines away (no varargs to block it, unlike
	// print_if_false), so it would never emit the original's real
	// "call 0046CB90h". gsub_46CB90 (panel.cpp) is the actual out-of-line
	// implementation at that address, already @Ok/@Matching there.
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	gsub_430680();

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");

	gShowRecordBox = new CRecordBox(0x75, 0x50, pMission);

	gShellMenuEase = 0x200;

	while (1)
	{
		gsub_430880();
		Db_FlipClear();
		CalcPolyBufferEnd();

		i32 vblanksSnapshot = Vblanks;

		if (!gSceneRelated)
			PCGfx_BeginScene(1u, -1);

		Shell_DrawBackground();

		Shell_DrawTitleBar(0x80, 0x26, gShowRecordTitle, 1, 0, 0x96, -21, 0x1D);

		Mess_SetRGB(0x60u, 0x60u, 0x60u, 0);
		Mess_SetTextJustify(0);
		Mess_DrawText(0x100, 0x41, pMission->field_0, 0, 0x1000u);

		gShowRecordBox->Display();

		PCSHELL_DrawMouseCursor();

		if (gSceneRelated)
			PCGfx_EndScene(1);

		gShellMenuEase = PShell_MoveTowards(gShellMenuEase, 0x180);

		Pad_Update();

		if (*gShellMenuAbort)
			return;

		CheckForPadUnplugged();

		if (PCSHELL_CheckTriggers(0x20220, 1, 1))
			break;

		if (Vblanks == vblanksSnapshot)
			Pause(1);

		*(volatile i32*)&DoVblankProcessing = 0;
		Pause(1);

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		gsub_430680();

		if (!*(volatile i32*)&DoVblankProcessing)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}

		PCSHELL_Relax();
	}

	G_SCONTROL[0].Circle.Triggered = 0;
	SFX_Play(0x23, 0x2000, 0);

	delete gShowRecordBox;

	Pause(1);

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	gsub_430680();

	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");

	Pad_ClearTriggers(G_SCONTROL);

	gShellMenuEase = 0x200;
}

// @Ok
i32 Shell_Special(EShellResult a1)
{
	print_if_false(gShellInitialized != 0, "Called Shell_Special() without shell initialised");
	i32 v10 = 0;
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(256, 0, 0, 256, 256, 16);
	pMenu->AddEntry("costumes");
	pMenu->AddEntry("view credits");
	pMenu->AddEntry("cheats");
	i32 v4 = *gShowAllLevels;
	for (i32 i = 0; i < 34; i++)
	{
		if (gSaveGame.field_56[i] != 0)
		{
			v4 = 1;
			break;
		}
	}
	if (v4 != 0)
		pMenu->AddEntry("level select");
	pMenu->CentreY();
	pMenu->Zoom(0);
	switch (a1)
	{
	case 22:
		pMenu->SetLine(0);
		break;
	case 23:
		pMenu->SetLine(2);
		break;
	case 24:
		pMenu->SetLine(1);
		break;
	case 25:
		print_if_false(v4 != 0, "Bugger");
		pMenu->SetLine(3);
		break;
	default:
		print_if_false(0, "Bad default sent to Shell_Special");
		break;
	}

	i32 v1 = 0;
	*(i32*)0x005512EC = 0;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v13 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v1, 38, "special", 1, 0, 150, -21, 29);
		pMenu->Display();
		if (pMenu->FinishedZooming())
		{
			PShell_InstructionalText();
			switch (pMenu->mLine)
			{
			case 0:
				Mess_DrawText(256, 180, "change in-game spider-man outfit", 0, 0x1000);
				Mess_DrawText(256, 193, "and gain special abilities", 0, 0x1000);
				break;
			case 1:
				Mess_DrawText(256, 180, "view credits of those who", 0, 0x1000);
				Mess_DrawText(256, 193, "worked on this game", 0, 0x1000);
				break;
			case 2:
				Mess_DrawText(256, 180, "enter cheat codes", 0, 0x1000);
				break;
			case 3:
				Mess_DrawText(256, 180, "select which level to play", 0, 0x1000);
				break;
			default:
				break;
			}
		}
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		v1 = PShell_MoveTowards(v1, 128);
		*(i32*)0x005512EC = PShell_MoveTowards(*(i32*)0x005512EC, 384);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return 0;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			goto done;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			u8 mJustification = pMenu->mJustification;
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
			break;
		if (Vblanks == v13)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
	}

	G_SCONTROL[0].Start.Triggered = 0;
	G_SCONTROL[0].X.Triggered = 0;
	SFX_Play(0x1F, 0x2000, 0);
	switch (pMenu->mLine)
	{
	case 0:
		v10 = 22;
		break;
	case 1:
		v10 = 24;
		break;
	case 2:
		v10 = 23;
		break;
	case 3:
		v10 = 25;
		break;
	default:
		break;
	}

done:
	delete pMenu;
	Pause(1);
	if (!gPrintStubbed)
		gsub_46CB90((void*)"stubbed out: DrawSync");
	Pad_ClearTriggers(G_SCONTROL);
	return v10;
}

// @Ok
void Shell_StoryBoards(void)
{
	print_if_false(gShellInitialized != 0, "Called Shell_MainMenu() without shell initialised");
	PShell_NormalFont();

	CMenu* pMenu = new CMenu(188, 73, 1, 256, 256, 13);
	pMenu->AdjustWidth(10);
	for (i32 i = 0; i < 21; i++)
	{
		if (gMovieTable[i].mUnlockId != -1)
			pMenu->AddEntry(gMovieTable[i].pDisplayName);
	}
	pMenu->scrollbar_one = 1;
	pMenu->field_1B = 9;
	pMenu->scrollbar_zero = 0;
	pMenu->CentreX();
	pMenu->CentreY();
	if (pMenu->mNumLines > 9)
		pMenu->Zoom(2);
	else
		pMenu->Zoom(1);

	i32 v5 = 0;
	*(i32*)0x005512EC = 384;

	while (1)
	{
		Db_FlipClear();
		CalcPolyBufferEnd();
		i32 v16 = Vblanks;
		if (gSceneRelated == 0)
			PCGfx_BeginScene(1, -1);
		if (gBackgroundAnimFrame == 0)
			Spool_AnimAccess("menubg", &gBackgroundAnimFrame);
		PCPanel_DrawTexturedPoly(-1.0f, gBackgroundAnimFrame->pTexture, 0, 0, 512, 240, 128);
		Shell_DrawTitleBar(v5, 38, "storyboards", 1, 0, 150, -21, 29);
		pMenu->Display();
		PCSHELL_DrawMouseCursor();
		if (gSceneRelated != 0)
			PCGfx_EndScene(1);
		i32 v17 = PShell_MoveTowards(v5, 128);
		if (pMenu->mLine > 0x28)
			Pad_ClearTriggers(G_SCONTROL);
		Pad_Update();
		if (*gShellMenuAbort != 0)
			return;
		CheckForPadUnplugged();
		pMenu->Update();
		if (PCSHELL_CheckTriggers(131616, 1, 1))
		{
			G_SCONTROL[0].Circle.Triggered = 0;
			SFX_Play(0x23, 0x2000, 0);
			delete pMenu;
			Pause(1);
			if (!gPrintStubbed)
				gsub_46CB90((void*)"stubbed out: DrawSync");
			Pad_ClearTriggers(G_SCONTROL);
			return;
		}
		i32 IsMouseOverText = 0;
		if (PCSHELL_CheckTriggers(256, 1, 1))
		{
			u8 mJustification = pMenu->mJustification;
			const char* name = pMenu->mEntry[pMenu->mLine].name;
			i32 x, y;
			pMenu->GetEntryXY(name, &x, &y);
			IsMouseOverText = PCSHELL_IsMouseOverText(name, x, y, mJustification);
		}
		if (pMenu->mLine < 0x28 && (IsMouseOverText != 0 || PCSHELL_CheckTriggers(65552, 1, 1)))
		{
			G_SCONTROL[0].Start.Triggered = 0;
			G_SCONTROL[0].X.Triggered = 0;
			SFX_Play(0x1F, 0x2000, 0);
			const SMovieEntry* pSel = 0;
			const char* selName = pMenu->mEntry[pMenu->mLine].name;
			for (i32 i = 0; i < 21; i++)
			{
				if (Utils_CompareStrings(selName, gMovieTable[i].pDisplayName) == 0)
					pSel = &gMovieTable[i];
			}
			print_if_false(pSel != 0, "Storyboard not found");
			if (pSel != 0 && pSel->field_C != 0)
			{
				char bmpName[13];
				strcpy(bmpName, "l*s*_*.bmp");
				bmpName[1] = pSel->pFileName[1];
				bmpName[3] = pSel->pFileName[3];
				i32 page = 1;
				bmpName[5] = page + 48;
				BMP_Draw(bmpName);
				while (1)
				{
					Pad_Update();
					if (PCSHELL_CheckTriggers(16388, 1, 1))
					{
						--page;
						G_SCONTROL[0].Left.Triggered = 0;
						if (page < 1)
							page = pSel->field_C;
						bmpName[5] = page + 48;
						BMP_Draw(bmpName);
					}
					else if (PCSHELL_CheckTriggers(32776, 1, 1))
					{
						++page;
						G_SCONTROL[0].Right.Triggered = 0;
						if (page > pSel->field_C)
							page = 1;
						bmpName[5] = page + 48;
						BMP_Draw(bmpName);
					}
					else if (PCSHELL_CheckTriggers(131104, 1, 1))
						break;
				}
				SFX_Play(0x23, 0x2000, 0);
				G_SCONTROL[0].Circle.Triggered = 0;
				G_SCONTROL[0].X.Triggered = 0;
				G_SCONTROL[0].Start.Triggered = 0;
			}
		}
		if (Vblanks == v16)
			Pause(1);
		DoVblankProcessing = 0;
		Pause(1);
		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		if (DoVblankProcessing == 0)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}
		PCSHELL_Relax();
		v5 = v17;
	}
}

// @Ok
// @Matching
void Shell_TitleScreen(void)
{
	Front_ClearScreen();
	DrawSync();
	Pad_ClearTriggers(gSControl);
	Pad_Update();
	Pad_ClearTriggers(gSControl);

	Sprite2* v0 = new Sprite2("title.bmp", 1, 0, 0, 3);

	// same address as gsub_430880 (nullsub_3), declared and defined in
	// PCShell.cpp; cast to accept the (unused) dummy arg this call site passes.
	extern void gsub_430880(void);
	((void(*)(i32))gsub_430880)(3);

	Redbook_XAPlay(0x43, 0xD, 0);

	while (1)
	{
		if (!gSceneRelated)
			PCGfx_BeginScene(1u, -1);

		v0->screenHeight();

		v0->draw(0, 0, 8, -1.0f);

		Front_MiniUpdate();

		if (gSceneRelated)
			PCGfx_EndScene(1);

		++TTime;
		Pad_Update();

		if (PCSHELL_CheckTriggers(0x40010, 1, 1))
			break;

		gsub_430880();
		PCSHELL_Relax();
	}

	gSControl[0].Start.Triggered = 0;
	delete v0;

	Redbook_XAStop();
	Mess_DeleteAll();

	Utils_InitialRand(Vblanks);

	for (i32 i = 10000; i > 0; i--)
		Rnd(10);

	// tentative: 9 i32 game-address array, no name in idb_globals.txt (nearest
	// neighbours are gTrainingSeconds 0x551288 and gCheats 0x5513E0)
	static i32 * const gTitleScreenShuffleTable = (i32*)0x5512A0;
	Utils_Jumble(gTitleScreenShuffleTable, 9);

	Front_ClearScreen();
	DrawSync();
	Pad_ClearTriggers(gSControl);
}

// @Ok
INLINE void Shell_VerySmallFont(void)
{
	Mess_SetScale(256);
	Mess_SetCurrentFont("sp_fnt03.fnt");
}


// @BIGTODO
// fill these
EXPORT SpideyIconRelated SpideyIcons[8];


const i32 NUM_LEVELS = 34;
EXPORT u8 LevelIndexes[NUM_LEVELS];

// @Ok
// @Test
i32 CalcIndexOfContinueLevel(void)
{
	i32 bar = 1000000;
	for (i32 i = 0; i < NUM_LEVELS; i++)
	{
		if (LevelIndexes[i] < bar)
			bar = LevelIndexes[i];
	}

	i32 index;
	for (index = 0; index < NUM_LEVELS; index++)
	{
		if (LevelIndexes[index] == bar)
			break;
	}

	return index;
}

// @Ok
void Spidey_CIcon::SetIcon(i32 option)
{
	print_if_false(option >= 0 && (u32)option < 8, "Bad option");

	if (SpideyIcons[option].IconModel < 0)
	{
		this->mFlags |= 1;
		return;
	}

	this->InitItem(SpideyIcons[option].Name);

	i32 iconModel = SpideyIcons[option].IconModel;
	if (iconModel < 0)
	{
		this->mFlags |= 1;
	}
	else
	{
		print_if_false(iconModel < reinterpret_cast<i32>(PSXRegion[this->mRegion].ppModels[-1]),
				"Bad icon model");

		this->mFlags &= 0xFFFE;
		this->mModel = iconModel;
	}


	if (PSXRegion[this->mRegion].Filename[9])
	{
		this->mFlags |= 0x482;
		this->mpLight = &M3d_SpideyCIconLight;
		this->RunAnim(0, 0, -1);
	}
	else
	{
		this->mFlags &= 0xFB7D;
		this->mpLight = 0;
	}

	this->mPos.vy = SpideyIcons[option].field_10 << 12;
	this->mPos.vz = SpideyIcons[option].field_14 << 12;

	this->mAngles.vx = SpideyIcons[option].field_8;
	this->mAngles.vz = SpideyIcons[option].field_C;
}

// @Ok
Spidey_CIcon::Spidey_CIcon(i32 icon)
{
	this->SetIcon(icon);
}

// @Ok
// @Test
Spidey_CIcon::Spidey_CIcon(i32 a2, i32 a3, i32 a4)
{
	this->mPos.vx = a2 << 12;
	this->mPos.vy = a3 << 12;
	this->mPos.vz = a4 << 12;

	this->InitItem("items");

	this->mModel = 5;
	this->mFlags &= 0xFB7D;

	this->mpLight = 0;
	this->mFlags |= 0x200;
	this->mScale.vx = 2048;
	this->mScale.vy = 2048;
	this->mScale.vz = 2048;
}

// @Ok
INLINE void CallAI(CBody *pList)
{
	CBody* pCur = pList;
	if (pCur)
	{
		for (
				CBody *pNext = reinterpret_cast<CBody*>(pCur->mNextItem);
				;
				pNext = reinterpret_cast<CBody*>(pNext->mNextItem))
		{
			if (pCur->mCBodyFlags & 0x40)
			{
				if (pCur->mCBodyFlags & 0x80)
				{
					delete pCur;
				}
			}
			else
			{
				pCur->InterleaveAI();
			}

			pCur = pNext;
			if (!pNext)
				break;
		}
	}
}

// @MEDIUMTODO
void CShellMysterioHeadCircle::Move(void)
{
}

// @Ok
CShellMysterioHeadCircle::~CShellMysterioHeadCircle(void)
{
	--gShellMysterioRelated;
}

// @Ok
CShellMysterioHeadCircle::CShellMysterioHeadCircle(CDummy *pDummy)
{
	this->field_84 = Mem_MakeHandle(reinterpret_cast<void*>(pDummy));

	this->SetTexture(0xB968C0FD);
	this->SetSemiTransparent();

	this->field_90 = Rnd(100) + 100 * gShellMysterioRelated + 50;

	if (gShellMysterioRelated & 1)
		this->field_90 *= -1;

	++gShellMysterioRelated;
}

// @NotOk
// Residue: register allocation only (63 mnemonic diffs, all downstream of one root
// cause). All three sin/cos table lookups (heading, field_108-phase, field_104-phase)
// read the SAME masked-index twice (offset +0 and +2 into word_610C48). The original
// computes the byte offset ONCE via an explicit "shl reg,2" then does two plain
// [reg+610C48h]/[reg+610C48h+2] reads; our build always folds the *4 into SIB-scaled
// addressing per read ([reg*4+610C48h]) instead, twice. Tried: masked index as a plain
// i32 local (word_610C48[2*idx], word_610C48[2*idx+1]) - SIB every time; an i16* local
// pre-advanced by 2*idx with [0]/[1] indexing - reduced diffs (74->63, this is the kept
// version) but still SIB-addresses instead of reusing the pointer; a manually pre-*2
// index folded before the array subscript - regressed (66). The same "index used twice"
// shape DOES compile to the shl+plain-offset form in CShellEmber::Move's idx7c case in
// this same file, so this is source-shape-dependent, not a hard compiler limit; ran out
// of iteration budget this session to find the exact trigger. Everything else (dead
// hook.Offset=1 init before the real value, the two-step truncating +=0xF63C/+=0xDA1C
// on the i16 struct fields, the branch polarity on the field_110 Rnd reset, the
// reinterpret_cast<CSuper*>(pDummy) reuse for M3d_BuildTransform/GetDynamicHookPosition)
// is verified correct against the disassembly. 6 attempts.
void CShellGoldFish::AI(void)
{
	CDummy *pDummy = static_cast<CDummy*>(Mem_RecoverPointer(&this->field_F8));

	if (!pDummy)
	{
		this->Die();
		return;
	}

	M3d_BuildTransform(reinterpret_cast<CSuper*>(pDummy));

	if (this->field_10C)
		this->field_10C--;

	if (!this->field_10C)
	{
		this->field_10C = Rnd(0x190) + 0x14;
		this->mAngVel.vy = -this->mAngVel.vy;
	}

	if (this->field_110)
		this->field_110--;

	if (!this->field_110)
	{
		this->field_110 = 0x14;

		if (this->mAngVel.vy < 0)
			this->mAngVel.vy = -0x28 - Rnd(0x5A);
		else
			this->mAngVel.vy = Rnd(0x5A) + 0x28;
	}

	if (this->field_100)
		this->mAngVel.vy <<= 1;

	this->field_114 += this->mAngVel.vy;
	i16 *tblHeading = word_610C48 + 2 * (this->field_114 & 0xFFF);
	i32 sinH = tblHeading[0];
	i32 cosH = tblHeading[1];

	i32 phase108 = this->field_108;
	i32 idx108 = phase108 & 0xFFF;
	i32 magVal = word_610C48[2 * idx108];
	this->field_108 = phase108 + 0xA;

	i32 phase104 = this->field_104;
	i32 idx104 = phase104 & 0xFFF;
	i32 bobVal = word_610C48[2 * idx104];
	this->field_104 = phase104 + 0x50;

	SHook hook;
	hook.Offset = 1;

	i32 mag = (magVal * 500) / 4096 + 0x8FC;
	hook.Part.vx = (mag * sinH) >> 12;
	hook.Part.vz = (mag * cosH) >> 12;
	hook.Part.vz += 0xF63C;

	hook.Offset = bobVal * 600 / 4096 - 0x5DC;
	hook.Offset += 0xDA1C;

	M3dUtils_GetDynamicHookPosition(
			reinterpret_cast<VECTOR*>(&this->mPos),
			reinterpret_cast<CSuper*>(pDummy),
			&hook);

	if (this->mAngVel.vy > 0)
		this->mAngles.vy = this->field_114;
	else
		this->mAngles.vy = this->field_114 + 0x800;
}

// @Ok
CShellGoldFish::~CShellGoldFish(void)
{
	this->DeleteFrom(&MiscList);
}

// @Ok
CShellGoldFish::CShellGoldFish(CDummy *pDummy)
{
	this->field_F8 = Mem_MakeHandle(reinterpret_cast<void*>(pDummy));

	this->InitItem("goldfish");
	this->mType = 506;
	this->AttachTo(&MiscList);

	this->mFlags |= 0x200;
	this->mAngVel.vy = 50;

	this->mScale.vz = 10000;
	this->mScale.vy = 10000;
	this->mScale.vx = 10000;
}

// @MEDIUMTODO
CShellSimbyFireDeath::CShellSimbyFireDeath(CDummy*)
{
	printf("CShellSimbyFireDeath::CShellSimbyFireDeath(CDummy*)");
}

// @Ok
// @Test
void CShellSimbyMeltSplat::Move(void)
{
	switch (this->field_84)
	{
		case 0:
			this->field_88 += 10;
			if (this->field_88 >= this->field_8C)
				this->field_84 = 1;

			break;
		case 1:
			Bit_ReduceRGB(&this->mTint, 10);
			if (!(0xFFFFFF & this->mTint))
					this->Die();
			break;
	}

	CVector a3 = (this->field_88 * this->field_9C);
	CVector v11 = (this->field_88 * this->field_A8);

	this->mPos = (this->field_90 - a3) - v11;

	this->mPosB = (this->field_90 + a3) - v11;

	this->mPosC = (this->field_90 - a3) + v11;

	this->mPosD = (this->field_90 + a3) + v11;
}

// @Ok
CShellSimbyMeltSplat::CShellSimbyMeltSplat(CVector* pVec)
{
	this->field_90.vx = 0;
	this->field_90.vy = 0;
	this->field_90.vz = 0;

	this->field_9C.vx = 0;
	this->field_9C.vy = 0;
	this->field_9C.vz = 0;

	this->field_A8.vx = 0;
	this->field_A8.vy = 0;
	this->field_A8.vz = 0;

	this->SetTexture(0x3AF6DFF);
	this->SetSemiTransparent();
	this->SetTint(0xFF, 0, 0);

	this->field_8C = Rnd(50) + 70;
	this->field_90 = *pVec;

	SVECTOR v11;
	v11.vx = 0;
	v11.vy = -4096;
	v11.vz = 0;

	this->OrientUsing(&this->field_90, &v11, 1, 1, Rnd(4096));

	this->field_9C = (this->mPosB - this->mPos) >> 1;
	this->field_A8 = (this->mPosC - this->mPos) >> 1;

	this->Move();
	this->mType = 21;
}

// @NotOk
// Residue: register allocation only. Logic verified correct (spiral position update,
// fade-out of R/G/B intensities, flicker-scaled color repack). Two spots resist matching
// after 14 tried source shapes (see attempts log): (1) this->field_80 should load into
// ecx as the very first memory read of the function, before mVel.vy/mPos.vy; every source
// order tried instead loads mVel.vy/mPos.vy first, or (when field_80 is moved first) pulls
// field_7C forward too early. (2) the final mCodeBGR repack: the original packs the
// field_88 contribution via a bare "mov cl,dh" byte extraction (no explicit shift), ours
// always emits sar+and+or for all three channels.
void CShellEmber::Move(void)
{
	this->mPos.vy -= this->mVel.vy;
	i32 idx80 = this->field_80 & 0xFFF;
	this->field_80 += 100;
	i32 amp = (this->field_78 * word_610C48[2 * idx80]) >> 12;

	i32 phase7c = this->field_7C;
	i32 idx7c = phase7c & 0xFFF;
	this->mPos.vx = amp * word_610C48[2 * idx7c] + this->field_68;
	this->mPos.vz = amp * word_610C48[2 * idx7c + 1] + this->field_70;
	this->field_7C = phase7c + 100;

	if (this->field_74)
	{
		this->field_74--;
	}
	else
	{
		i32 v84 = this->field_84 < 15 ? 0 : this->field_84 - 15;
		this->field_84 = v84;

		i32 v88 = this->field_88 < 15 ? 0 : this->field_88 - 15;
		this->field_88 = v88;

		i32 v8c = this->field_8C < 15 ? 0 : this->field_8C - 15;
		this->field_8C = v8c;

		if (!(v84 | v88 | v8c))
			this->Die();
	}

	i32 flicker = Rnd(0x100);
	this->mCodeBGR = (((this->field_8C * flicker) >> 8) << 16)
		| (((this->field_88 * flicker) >> 8) << 8)
		| ((this->field_84 * flicker) >> 8)
		| (this->mCodeBGR & 0xFF000000u);
}

// @Ok
// @Test
CShellEmber::CShellEmber(
		CVector* pVec,
		i32 a3)
{
	this->field_68 = 0;
	this->field_6C = 0;
	this->field_70 = 0;

	this->mPos = *pVec;
	this->field_68 = this->mPos.vx;
	this->field_70 = this->mPos.vz;

	this->field_78 = Rnd(10) + 10;
	this->field_7C = Rnd(4096);
	this->field_80 = Rnd(4096);

	this->SetTexture(0x13C0A001);
	this->mScale = Rnd(200) + 350;

	this->field_84 = 255;
	this->field_88 = 128;
	this->field_8C = 0;

	this->SetTint(0xFF, 128, 0);
	this->SetSemiTransparent();

	this->field_74 = (a3 * (Rnd(5) + 5)) > 8;
	this->mVel.vy = (a3 * (Rnd(5) + 6)) << 12 >> 8;
}

// @Ok
CShellMysterioHeadGlow::CShellMysterioHeadGlow(void)
	: CWobblyGlow(&ZeroVector, 150, 120, 90, 255, 255, 255, 0x80u, 0, 0xFFu)
{
}

// @Ok
CWobblyGlow::CWobblyGlow(
		CVector* Pos,
		i32 InnerRadius,
		i32 FringeRadius,
		i32 Amp,
		u8 r0,
		u8 g0,
		u8 b0,
		u8 r1,
		u8 g1,
		u8 b1)
	: CGlow(Pos, InnerRadius, FringeRadius, r0, g0, b0, r1, g1, b1)
{
	this->mAmplitude = Amp * InnerRadius / 256;

	this->mInnerRadius = InnerRadius;

	for (u32 i = 0; i < this->mNumSections; i++)
	{
		this->mInc[i] = Rnd(4096);
		this->mT[i] = Rnd(50) + 200;
	}
}

// @Ok
// @Test
void CShellRhinoNasalSteam::Move(void)
{
	if (this->mAnimSpeed)
	{
		i16 v3 = (this->mFrame << 8) | this->mFrameFrac;
		v3 += this->mAnimSpeed;

		this->mFrameFrac = v3;
		v3 >>= 8;

		this->mFrame = v3;

		if (this->mFrame >= this->mNumFrames)
		{
			this->mAnimSpeed = 0;
			this->mFrame = this->mNumFrames - 1;
		}

		this->mpPSXFrame = &this->mpPSXAnim[this->mFrame];
	}

	this->mPos += this->mVel;

	this->mVel.vy -= 1024;
	if (++this->mAge > 30)
	{
		this->Die();
	}
	else
	{
		this->SetTransparency(64 - 2 * (0xFF & this->mAge));
		this->SetScale(Rnd(4) + 4 *(this->mAge + 32));
	}
}

// @Ok
CShellRhinoNasalSteam::CShellRhinoNasalSteam(
		CVector* a2,
		CVector* a3)
{
	this->mPos = *a2;
	this->mVel = *a3;

	this->SetAnim(1);
	this->SetSemiTransparent();
	this->SetTransparency(0x40);
	this->SetAnimSpeed(128);
	this->SetScale(128);
	this->mAngle = Rnd(4096);
}

// @Ok
// skin goo params are not okay
void CShellSuperDocOckElectrified::Move(void)
{
	CSuper *pSuper = static_cast<CSuper*>(Mem_RecoverPointer(&this->field_3C));

	if (!pSuper)
	{
		this->Die();
		return;
	}

	M3d_BuildTransform(pSuper);

	if (++this->field_44 > 0)
	{
		new CSkinGoo(pSuper, &gSuperDocOckSkinGooSource, 19, &gSuperDocOckSkinGooParams);
		this->field_44 = 0;
	}

}

// @Ok
CShellSuperDocOckElectrified::CShellSuperDocOckElectrified(CSuper* pSuper)
{
	print_if_false(pSuper != 0, "NULL pointer");
	print_if_false(pSuper->mType == 309, "Non SuperDocOck");

	this->field_3C = Mem_MakeHandle(reinterpret_cast<void*>(pSuper));
}

// @Ok
// skin goo params are not okay
void CShellCarnageElectrified::Move(void)
{
	CSuper *pSuper = static_cast<CSuper*>(Mem_RecoverPointer(&this->field_3C));

	if (!pSuper)
	{
		this->Die();
		return;
	}

	M3d_BuildTransform(pSuper);

	if (++this->field_44 > 0)
	{
		new CSkinGoo(pSuper, &gCarnageSkinGooSourceShell, 19, &gCarnageSkinGooParams);
		this->field_44 = 0;
	}

}

// @Ok
CShellCarnageElectrified::CShellCarnageElectrified(CSuper* pSuper)
{
	print_if_false(pSuper != 0, "NULL pSuper sent to CShellCarnageElectrified");
	print_if_false(pSuper->mType == 314, "Non carnage sent to CShellCarnageElectrified");

	this->field_3C = Mem_MakeHandle(reinterpret_cast<void*>(pSuper));
}


// @NotOk
// skin goo params are not okay
void CShellVenomElectrified::Move(void)
{
	CSuper *pSuper = static_cast<CSuper*>(Mem_RecoverPointer(&this->field_3C));

	if (!pSuper)
	{
		this->Die();
		return;
	}

	M3d_BuildTransform(pSuper);

	if (++this->field_44 > 0)
	{
		new CSkinGoo(pSuper, &gVenomSkinGooSource, 19, &gVenomSkinGooParams);
		this->field_44 = 0;
	}

}

// @Ok
CShellVenomElectrified::CShellVenomElectrified(CSuper* pSuper)
{
	print_if_false(pSuper != 0, "NULL pSuper sent to CVenomWrap");
	print_if_false(pSuper->mType == 313, "Non venom sent to CShellVenomElectrified");

	this->field_3C = Mem_MakeHandle(reinterpret_cast<void*>(pSuper));
}

// @Ok
void CDummy::SelectNewAnim(void)
{
	if (this->field_1B8)
	{
		this->field_1B8++;
		if (*this->field_1B8 == 0xFFFF)
		{
			this->SelectNewTrack(0);
		}
		else
		{
			this->RunAnim(*this->field_1B8, 0, -1);
		}
	}
	else
	{
		this->RunAnim(this->field_1C0, 0, -1);
	}
}

// @Ok
void CDummy::SelectNewTrack(int a2)
{
	this->field_1B8 = 0;
	this->field_1BC = 0;

	if (this->field_1A4 || this->field_1A8 || this->field_1AC)
	{
		do
		{
			switch(Rnd(3))
			{
				case 0:
					this->field_1B8 = this->field_1A4;
					break;
				case 1:
					this->field_1B8 = this->field_1A8;
					break;
				case 2:
					this->field_1B8 = this->field_1AC;
					break;
			}
		}
		while(!this->field_1B8);

		print_if_false(*this->field_1B8 != 0xFFFF, "First anim must not be 0xFFFF");

		if (a2)
		{
			u16 *v7 = this->field_1B8;
			i32 i = 0;
			for (i = 0; *v7 != 0xFFFF; i++)
				v7++;

			i32 v9 = 0;
			i32 v10;
			do
			{
				v10 = Rnd(i);
				v9++;
			}
			while (this->field_1B8[v10] == this->mAnim && v9 < 100);

			if (this->field_1B8[v10] != this->mAnim)
				this->field_1B8 = &this->field_1B8[v10];
		}

		this->field_1BC = this->field_1B8;
		this->RunAnim(*this->field_1BC, 0, -1);
	}
	else
	{
		this->RunAnim(this->field_1C0, 0, -1);
	}
}

// @Ok
void INLINE CDummy::FadeAway(void)
{
	this->field_1F8 = 1;
	this->field_1FC = 0;

	this->mFlags &= 0xFF7F;
	this->mFlags |= 0x800;

	this->mRGB = 0x202020;

	this->OutlineOn();
	this->SetOutlineSemiTransparent();
	this->SetOutlineRGB(0, 0, 0);
}

// @Ok
void INLINE CDummy::FadeBack(void)
{
	this->field_1FC = 1;
	this->field_1F8 = 0;
}

// @NotOk
// Global
void INLINE CWobblyGlow::Move(void)
{
	for (u32 i = 0; i < this->mNumSections; i++)
	{
		this->mInc[8+i] += this->mInc[i];
		int v3 = this->mInc[8+i];
		this->mpSections[i].Radius = this->mInnerRadius + this->mAmplitude * word_610C48[2 * (v3 & 0xFFF)] / 4096;
	}
}

// @Ok
void CShellMysterioHeadGlow::Move(void)
{
	CWobblyGlow::Move();
	this->mAngle += this->field_A4;
}

// @Ok
void Spidey_CIcon::AI(void)
{
	this->mAngles.vy += 50;
	if (this->mFlags & 2)
	{
		this->UpdateFrame();
		M3d_BuildTransform(this);
	}
}

// @NotOk
// globals
CRudeWordHitterSpidey::CRudeWordHitterSpidey(void)
{
	this->InitItem("spidey");
	this->mFlags |= 0x480;

	this->mpLight = &M3d_RudeSpideyLight;

	this->field_194 |= 0x420;

	this->RunAnim(0, 0, -1);

	this->mFrame = 18;
	this->mPos.vx = 0xFFF92000;
	this->mPos.vy = 0x104000;
	this->mPos.vz = 0x1F4000;
	this->mAngles.vy = 0xFD76;
}

// @Ok
void CRudeWordHitterSpidey::AI(void)
{
	this->field_1A8++;
	if (this->field_1A8 > 60)
	{
		this->mPos.vy += 0x14000;
	}
	else
	{
		this->mPos.vy -= 0x14000;
		if (this->mPos.vy < 0x91000)
		{
			this->mPos.vy = 0x91000;
		}
	}

	if (this->mAnimFinished)
	{
		if (!this->mAnim)
		{
			this->RunAnim(0x64, 0, -1);
		}
		else
		{
			this->RunAnim(0, 0, -1);
		}
	}

	this->UpdateFrame();

	if (this->mFrame == 7 && !this->field_1A4)
	{
		switch (Rnd(4))
		{
			case 0:
				SFX_Play(0xE, 0x2000, 0);
				break;
			case 1:
				SFX_Play(0xF, 0x2000, 0);
				break;
			case 2:
				SFX_Play(0x10, 0x2000, 0);
				break;
			case 3:
				SFX_Play(0x11, 0x2000, 0);
				break;
			default:
				break;
		}

		this->field_1A4 = 1;
	}

	M3d_BuildTransform(this);
}


// @Ok
CShellSymBurn::CShellSymBurn(CVector* pVector)
{
	this->mPos = *pVector;
	this->InitItem("fire");
	this->mFlags |= 0x602;
	this->mScale.vy = 0;
	this->mRGB = 0xFFFFFF;
	this->AttachTo(&MiscList);
}

SVECTOR gYAnglesRelated;

// @NotOk
// slightly different assembly, not important
void CShellSymBurn::AI(void)
{
	this->mAngles.vy = gYAnglesRelated.vy + 2048;
	this->mScale.vx = 3000;
	this->mScale.vz = 3000;

	if (++this->field_1A4 > 60)
	{
		i32 v3 = (this->mRGB & 0xFF) - 4;
		if (v3 < 0)
			v3 = 0;

		this->mScale.vy -= 75;
		this->mRGB = v3 | ((v3 | (v3 << 8)) << 8);

		if (this->mScale.vy < 0)
			this->mScale.vy = 0;

		if (!v3 || !this->mScale.vy)
		{
			this->Die();
		}
	}
	else
	{
		i32 v5 = (this->mRGB & 0xFF) - 129;
		if (v5 < 128)
			v5 = 128;

		this->mScale.vy += 800;
		this->mRGB = v5 | ((v5 | (v5 << 8)) << 8);

		if (this->mScale.vy > 4096)
			this->mScale.vy = 4096;
	}

	M3d_BuildTransform(this);
}

char *gBadWords[30] =
{
	"sjnkpc",
	"cmpxkpc",
	"bstf",
	"\x62\x74\x74\x00",
	"gvdl",
	"cvhhfs",
	"xbol",
	"\x75\x6A\x75\x00"
	"dvou",
	"tobudi",
	"qvttz",
	"tiju",
	"qjtt",
	"\x64\x76\x6E\x00",
	"\x77\x62\x68\x00",
	"gfmudi",
	"tqvol",
	"\x6B\x6A\x7B\x00",
	"dpdl",
	"gjtujoh",
	"ovutbd",
	"bobm",
	"ejmep",
	"cbtubse",
	"dpdl",
	"cvuu",
	"qfojt",
	"uxbu",
	"cjudi",
};

char *gGoodWords[30] = 
{
	"flower",
	"happy",
	"pretty",
	"puppy",
	"bunny",
	"donut",
	"lolly",
	"love",
	"nice",
	"cake",
	"poppy",
	"fluffy",
	"cloud",
	"rainbow",
	"icecream",
	"sugar",
	"windmill",
	"iowa",
	"toffee",
	"taffy",
	"candy",
	"sodapop",
	"bubble",
	"cinnamon",
	"dinosaur",
	"balloon",
	"lobster",
	"honey",
	"potato",
	"spice",
};

// @Ok
i32 Shell_DeRudify(char inp[INPUT_MAX_SIZE])
{
	char buffer[9];

	for (i32 i = 0; ; i++)
	{
		if (i >= 29)
			return 0;

		Utils_CopyString(gBadWords[i], buffer, 9);
		for (char *j = buffer; *j; j++)
			--*j;

		if (Shell_ContainsSubString(inp, buffer))
			break;
	}

	i32 result = Utils_CopyString(gGoodWords[Rnd(30)], inp, 9);
	for (i32 k = result; k < 8; k++)
		inp[k] = '.';

	return result;

}

// @NotOk
// good candidate for tests
INLINE i32 Shell_ContainsSubString(const char* hay, const char* needle)
{
	for (const char *hayPtr = hay; *hayPtr; hayPtr++)
	{
		const char *needlePtr = needle;
		for (; *needlePtr; needlePtr++)
		{
			char needleChar = *needlePtr;
			char hayChar = hay[needlePtr-needle];

			if (needleChar >= 'A' && needleChar <= 'Z')
				needleChar += ' ';

			if (hayChar >= 'A' && hayChar <= 'Z')
				hayChar += ' ';

			if (hayChar != needleChar)
				break;
		}

		if (!*needlePtr)
			return 1;
	}

	return 0;
}

// @Ok
// @AlmostMatching: no nullsub
void PShell_Cleanup(void)
{
	if ( gShellInitialized )
	{
		if (!gPshellArmorRealted)
		{
			PShell_BigFont();
			Mess_UnloadFont();
		}

		PShell_SmallFont();
		Mess_UnloadFont();

		Shell_VerySmallFont();
		Mess_UnloadFont();

		PShell_NormalFont();

		Spool_ClearPSX("control");
		Spool_ClearPSX("icons");
		Spool_ClearPSX("vmu");
		Spool_ClearPSX("shell");
		SFX_SpoolOutLevelSFX();

		if (gBiographies)
		{
			Mem_Delete(gBiographies);
			gBiographies = 0;
		}

		Pad_ClearAll();
		Front_ClearScreen();

		DrawSync();
		Init_KillAll();
		dword_6A7788[2] = 0;
		dword_6A7788[3] = 0;
		dword_6A7788[4] = 0;

		dword_6A7788[5] = 0;
		dword_6A7788[6] = 0;
		dword_6A7788[7] = 0;
		dword_6A7788[8] = 0;
		dword_6A7788[9] = 0;
		dword_6A7788[10] = 0;
		dword_6A7788[11] = 0;
		dword_6A7788[12] = 0;
		dword_6A7788[13] = 0;
		dword_6A7788[14] = 0;
		dword_6A7788[15] = 0;
		dword_6A7788[1] = 0;
		dword_6A7788[0] = 0;

		//nullsub_3(v0);

		gPShellCleanup = 1;
		OTPushback[0] = 1;

		OTPushback[1] = 1;
		gShellInitialized = 0;
	}
}

// @Ok
// @Matching
void PShell_Initialise(void)
{
	if (gShellFromGame)
		print_if_false(gShellInitialized == 0, "Shell initialised twice, fromgame");
	else
		print_if_false(gShellInitialized == 0, "Shell initialised twice, not fromgame");

	gPShellCleanup = 0;

	Spool_PSX("shell", 0);
	Spool_PSX("icons", 0);
	Spool_PSX("vmu", 0);
	Spool_PSX("control", 0);
	Mess_LoadFont("font_big.fnt", -1, -1, -1);
	Mess_LoadFont("sp_fnt02.fnt", -1, -1, -1);
	Mess_LoadFont("sp_fnt03.fnt", -1, -1, -1);
	SFX_SpoolInLevelSFX("menu");
	PShell_NormalFont();
	Spool_AnimAccess("menubg", &gBackgroundAnimFrame);

	OTPushback[0] = 1;
	OTPushback[1] = -60;

	PShell_MaybeUnlockStuff();
	PCSHELL_Initialize();
	gShellInitialized = 1;
}

// @Ok
// @Matching
INLINE void PShell_LowText(void)
{
	Mess_SetTextJustify(0);
	Mess_SetRGB(0x45u, 0x3Cu, 0x6Bu, 0);
	Mess_SetRGBBottom(0x28u, 35, 62);
}

// @NotOk
// @Note: validate when inlined
INLINE i32 RecordsExist(
		u8 a1,
		i8 a2,
		i8 a3)
{
	i32 v3 = -1;

	for (i32 i = 0; i < NUM_CHALLS; i++)
	{
		if (a1 == gChallenges[i].field_6
				&& a2 == gChallenges[i].field_8
				&& a3 == gChallenges[i].field_9)
		{
			v3 = i;
		}
	}
	print_if_false(v3 != -1, "Mission not found");
	return gGlobalRecords.mScores[5*v3].field_0;
}

// @Ok
INLINE i32 IsBetter(
		i32 a1,
		i32 a2,
		i32 a3)
{
	if (a3)
		return a2 > a1;

	return a1 < a2;
}

// @NotOk
// @Note: validate when inlined
INLINE void Merge(SRecords *a1, const SRecords *a2)
{
	for (i32 i = 0; i < NUM_CHALLS; i++)
	{
		Merge(&a1->mScores[5*i],
				&a2->mScores[5*i],
				gChallenges[i].field_C);
	}
}

// @Ok
// @Test
void Merge(
		SScore *a1,
		const SScore *a2,
		i32 a3)
{

	for (i32 i = 0; i < NUM_RECORDS_PER_CHALL; i++)
	{
		i32 v7 = 1;

		for (i32 j = 0; j < NUM_RECORDS_PER_CHALL; j++)
		{
			if (SameScore(&a1[i], &a2[j]))
			{
				v7 = 0;
			}
		}

		if (!a2[i].field_0)
		{
			v7 = 0;
		}

		if (v7)
		{
			i16 v11 = a2[i].field_3 + (a2[i].field_4 << 8);
			for (i32 v10 = 0; v10 < NUM_RECORDS_PER_CHALL; v10++)
			{
				i16 calc = (a1[v10].field_3 + (a1[v10].field_4 << 8));
				if (!a1[v10].field_0 ||
						IsBetter(v11, calc, a3))
				{
					for (i32 k = 4; k > v10; k--)
					{
						memcpy(&a1[k], &a1[k - 1], sizeof(SScore));
					}

					memcpy(&a1[v10], &a2[i], sizeof(SScore));
					break;
				}
			}
			
		}
	}
}

// @Ok
// @NotMatching: weeeeeeird codegen
INLINE i32 SameScore(
		const SScore *a1,
		const SScore *a2)
{
	if ( !a1->field_0 && !a2->field_0)
		return 1;
	return a1->field_0 == a2->field_0
		&& a1->field_1 == a2->field_1
		&& a1->field_2 == a2->field_2
		&& a1->field_3 == a2->field_3
		&& a1->field_4 == a2->field_4;
}

void validate_CRudeWordHitterSpidey(void){
	VALIDATE_SIZE(CRudeWordHitterSpidey, 0x1AC);

	
	VALIDATE(CRudeWordHitterSpidey, field_194, 0x194);
	VALIDATE(CRudeWordHitterSpidey, field_1A4, 0x1A4);
	VALIDATE(CRudeWordHitterSpidey, field_1A8, 0x1A8);
}

void validate_CDummy(void){
	VALIDATE_SIZE(CDummy, 0xA18);

	VALIDATE(CDummy, field_1A4, 0x1A4);
	VALIDATE(CDummy, field_1A8, 0x1A8);
	VALIDATE(CDummy, field_1AC, 0x1AC);
	VALIDATE(CDummy, field_1B8, 0x1B8);
	VALIDATE(CDummy, field_1BC, 0x1BC);
	VALIDATE(CDummy, field_1C0, 0x1C0);

	VALIDATE(CDummy, field_1F8, 0x1F8);
	VALIDATE(CDummy, field_1FC, 0x1FC);

	VALIDATE(CDummy, field_240, 0x240);
	VALIDATE(CDummy, field_288, 0x288);

	VALIDATE(CDummy, field_2D4, 0x2D4);
	VALIDATE(CDummy, field_304, 0x304);
	VALIDATE(CDummy, field_418, 0x418);
}

void validate_CShellMysterioHeadGlow(void)
{
	VALIDATE_SIZE(CShellMysterioHeadGlow, 0xA8);

	VALIDATE(CShellMysterioHeadGlow, field_A4, 0xA4);
}


void validate_CWobblyGlow(void)
{
	VALIDATE_SIZE(CWobblyGlow, 0xA4);

	VALIDATE(CWobblyGlow, mInc, 0x5C);
	VALIDATE(CWobblyGlow, mT, 0x7C);

	VALIDATE(CWobblyGlow, mAmplitude, 0x9C);
	VALIDATE(CWobblyGlow, mInnerRadius, 0xA0);
}

void validate_Spidey_CIcon(void)
{
	VALIDATE_SIZE(Spidey_CIcon, 0x1A4);
}

void validate_CShellSymBurn(void)
{
	VALIDATE_SIZE(CShellSymBurn, 0x1A8);

	VALIDATE(CShellSymBurn, field_1A4, 0x1A4);
}

void validate_CShellVenomElectrified(void)
{
	VALIDATE_SIZE(CShellVenomElectrified, 0x48);

	VALIDATE(CShellVenomElectrified, field_3C, 0x3C);
	VALIDATE(CShellVenomElectrified, field_44, 0x44);
}

void validate_CShellCarnageElectrified(void)
{
	VALIDATE_SIZE(CShellVenomElectrified, 0x48);

	VALIDATE(CShellVenomElectrified, field_3C, 0x3C);
	VALIDATE(CShellVenomElectrified, field_44, 0x44);
}

void validate_CShellSuperDocOckElectrified(void)
{
	VALIDATE_SIZE(CShellVenomElectrified, 0x48);

	VALIDATE(CShellVenomElectrified, field_3C, 0x3C);
	VALIDATE(CShellVenomElectrified, field_44, 0x44);
}

void validate_CShellRhinoNasalSteam(void)
{
	VALIDATE_SIZE(CShellRhinoNasalSteam, 0x68);
}

void validate_CShellEmber(void)
{
	VALIDATE_SIZE(CShellEmber, 0x90);

	VALIDATE(CShellEmber, field_68, 0x68);
	VALIDATE(CShellEmber, field_6C, 0x6C);
	VALIDATE(CShellEmber, field_70, 0x70);
	VALIDATE(CShellEmber, field_74, 0x74);
	VALIDATE(CShellEmber, field_78, 0x78);
	VALIDATE(CShellEmber, field_7C, 0x7C);
	VALIDATE(CShellEmber, field_80, 0x80);
	VALIDATE(CShellEmber, field_84, 0x84);
	VALIDATE(CShellEmber, field_88, 0x88);
	VALIDATE(CShellEmber, field_8C, 0x8C);
}

void validate_CShellSimbyMeltSplat(void)
{
	VALIDATE_SIZE(CShellSimbyMeltSplat, 0xB4);

	VALIDATE(CShellSimbyMeltSplat, field_84, 0x84);
	VALIDATE(CShellSimbyMeltSplat, field_88, 0x88);
	VALIDATE(CShellSimbyMeltSplat, field_8C, 0x8C);

	VALIDATE(CShellSimbyMeltSplat, field_90, 0x90);
	VALIDATE(CShellSimbyMeltSplat, field_9C, 0x9C);
	VALIDATE(CShellSimbyMeltSplat, field_A8, 0xA8);
}

void validate_CShellSimbyFireDeath(void)
{
	VALIDATE_SIZE(CShellSimbyFireDeath, 0x54);
}

void validate_CShellGoldFish(void)
{
	VALIDATE_SIZE(CShellGoldFish, 0x118);

	VALIDATE(CShellGoldFish, field_F8, 0xF8);
	VALIDATE(CShellGoldFish, field_100, 0x100);
	VALIDATE(CShellGoldFish, field_104, 0x104);
	VALIDATE(CShellGoldFish, field_108, 0x108);
	VALIDATE(CShellGoldFish, field_10C, 0x10C);
	VALIDATE(CShellGoldFish, field_110, 0x110);
	VALIDATE(CShellGoldFish, field_114, 0x114);
}

void validate_CShellMysterioHeadCircle(void)
{
	VALIDATE_SIZE(CShellMysterioHeadCircle, 0x94);

	VALIDATE(CShellMysterioHeadCircle, field_84, 0x84);
	VALIDATE(CShellMysterioHeadCircle, field_90, 0x90);
}

void validate_SpideyIconRelated(void)
{
	VALIDATE_SIZE(SpideyIconRelated, 0x28);

	VALIDATE(SpideyIconRelated, Name, 0x0);
	VALIDATE(SpideyIconRelated, IconModel, 0x4);
	VALIDATE(SpideyIconRelated, field_8, 0x8);
	VALIDATE(SpideyIconRelated, field_C, 0xC);
	VALIDATE(SpideyIconRelated, field_10, 0x10);
	VALIDATE(SpideyIconRelated, field_14, 0x14);
	VALIDATE(SpideyIconRelated, field_18, 0x18);
}

void validate_SSaveGame(void)
{
	VALIDATE_SIZE(SSaveGame, 0xBC);

	VALIDATE(SSaveGame, mChecksum, 0x0);
	VALIDATE(SSaveGame, field_4, 0x4);

	VALIDATE(SSaveGame, mRestartPointName, 0xD);

	VALIDATE(SSaveGame, field_3F, 0x3F);

	VALIDATE(SSaveGame, mDifficulty, 0x54);
	VALIDATE(SSaveGame, mCheatStoryboardFlag, 0x55);
	VALIDATE(SSaveGame, field_56, 0x56);
	VALIDATE(SSaveGame, field_78, 0x78);

	VALIDATE(SSaveGame, field_7B, 0x7B);
	VALIDATE(SSaveGame, field_7C, 0x7C);

	VALIDATE(SSaveGame, field_80, 0x80);
	VALIDATE(SSaveGame, field_84, 0x84);
	VALIDATE(SSaveGame, field_88, 0x88);
	VALIDATE(SSaveGame, field_8C, 0x8C);
	VALIDATE(SSaveGame, field_90, 0x90);

	VALIDATE(SSaveGame, field_94, 0x94);
	VALIDATE(SSaveGame, field_98, 0x98);
	VALIDATE(SSaveGame, field_9C, 0x9C);
	VALIDATE(SSaveGame, field_A0, 0xA0);
	VALIDATE(SSaveGame, field_A4, 0xA4);
	VALIDATE(SSaveGame, field_A8, 0xA8);
	VALIDATE(SSaveGame, mDigitalMapping, 0xAC);
	VALIDATE(SSaveGame, mAnalogueMapping, 0xB4);
}

void validate_SScore(void)
{
	VALIDATE_SIZE(SScore, 5);

	VALIDATE(SScore, field_0, 0x0);
	VALIDATE(SScore, field_1, 0x1);
	VALIDATE(SScore, field_2, 0x2);
	VALIDATE(SScore, field_3, 0x3);
	VALIDATE(SScore, field_4, 0x4);
}

void validate_SRecords(void)
{
	VALIDATE_SIZE(SRecords, 0x242);

	VALIDATE(SRecords, mScores, 0x3);
}

void validate_STrainingMission(void)
{
	VALIDATE_SIZE(STrainingMission, 0x10);

	VALIDATE(STrainingMission, field_0, 0x0);
	VALIDATE(STrainingMission, mAreaId, 0x7);
	VALIDATE(STrainingMission, mScoreUnits, 0xB);
	VALIDATE(STrainingMission, mLowerIsBetter, 0xC);
}

void validate_CRecordBox(void)
{
	VALIDATE_SIZE(CRecordBox, 0x44);

	VALIDATE(CRecordBox, field_4, 0x4);
	VALIDATE(CRecordBox, field_8, 0x8);
	VALIDATE(CRecordBox, field_C, 0xC);
	VALIDATE(CRecordBox, field_10, 0x10);
	VALIDATE(CRecordBox, field_14, 0x14);
	VALIDATE(CRecordBox, field_18, 0x18);
	VALIDATE(CRecordBox, field_1C, 0x1C);
	VALIDATE(CRecordBox, field_20, 0x20);
	VALIDATE(CRecordBox, field_24, 0x24);
	VALIDATE(CRecordBox, field_28, 0x28);
	VALIDATE(CRecordBox, field_2C, 0x2C);
	VALIDATE(CRecordBox, field_30, 0x30);
	VALIDATE(CRecordBox, field_34, 0x34);
	VALIDATE(CRecordBox, field_35, 0x35);
	VALIDATE(CRecordBox, field_36, 0x36);
	VALIDATE(CRecordBox, mLetterIndex, 0x38);
	VALIDATE(CRecordBox, field_39, 0x39);
	VALIDATE(CRecordBox, field_3C, 0x3C);
	VALIDATE(CRecordBox, field_40, 0x40);
}

void validate_SRecordRelated(void)
{
	VALIDATE_SIZE(SRecordRelated, 0x10);

	VALIDATE(SRecordRelated, pName, 0x0);
	VALIDATE(SRecordRelated, field_6, 0x6);

	VALIDATE(SRecordRelated, field_8, 0x8);
	VALIDATE(SRecordRelated, field_9, 0x9);

	VALIDATE(SRecordRelated, field_C, 0xC);
}
