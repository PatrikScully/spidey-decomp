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
#include "powerup.h"
#include "pshell.h"
#include "spidey.h"
#include "ps2m3d.h"
#include "init.h"
#include "ps2redbook.h"
#include "m3dutils.h"
#include "db.h"
#include "ps2funcs.h"

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

// @MEDIUMTODO
void Shell_ChooseEnemy(i32,u8,signed char)
{
    printf("Shell_ChooseEnemy(i32,u8,signed char)");
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

// @MEDIUMTODO
void Shell_ChooseSurvivalArena(i32)
{
    printf("Shell_ChooseSurvivalArena(i32)");
}

// @MEDIUMTODO
void Shell_ChooseTime(i32,i32)
{
    printf("Shell_ChooseTime(i32,i32)");
}

// @MEDIUMTODO
void Shell_ChooseTrainingControlType(void)
{
    printf("Shell_ChooseTrainingControlType(void)");
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

// @MEDIUMTODO
void Shell_Difficulty(i32)
{
    printf("Shell_Difficulty(i32)");
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

// @MEDIUMTODO
void Shell_Gallery(EShellResult)
{
    printf("Shell_Gallery(EShellResult)");
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

// @MEDIUMTODO
void Shell_LevelSelect(void)
{
    printf("Shell_LevelSelect(void)");
}

// @MEDIUMTODO
void Shell_LoadGame(void)
{
    printf("Shell_LoadGame(void)");
}

// @MEDIUMTODO
void Shell_MainMenu(EShellResult)
{
    printf("Shell_MainMenu(EShellResult)");
}

// @MEDIUMTODO
void Shell_MemoryCard(EShellResult)
{
    printf("Shell_MemoryCard(EShellResult)");
}

// @MEDIUMTODO
void Shell_MovieViewer(void)
{
    printf("Shell_MovieViewer(void)");
}

// @MEDIUMTODO
void Shell_Options(EShellResult)
{
    printf("Shell_Options(EShellResult)");
}

// @MEDIUMTODO
void Shell_RollCredits(void)
{
    printf("Shell_RollCredits(void)");
}

// @MEDIUMTODO
void Shell_SFXMusic(void)
{
    printf("Shell_SFXMusic(void)");
}

// @MEDIUMTODO
void Shell_SaveGame(const u32 *,u32 *)
{
    printf("Shell_SaveGame(u32 const *,u32 *)");
}

// Shell_ScreenAdjust and Shell_ShowRecord call these as real out-of-line functions
// in the original, keep the MSVC inliner away (same trick as PCShell.cpp's
// gsub_430680/gsub_430880/gsub_515850, needed because these stubs live in the same
// TU as their callers).
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// unnamed helper called once per screen adjust / show record frame, address 0x498240.
// same file range as Shell_ScreenAdjust, not yet decompiled on its own.
// @SMALLTODO
EXPORT void gsub_498240(i32, i32)
{
	printf("gsub_498240(i32, i32)");
}

// unnamed helper, address 0x48EA90, name from names.json. Called once per frame by
// several Shell_ menu loops (ScreenAdjust, ShowRecord, ChooseSurvivalArena, ...).
// not yet decompiled on its own.
// @SMALLTODO
EXPORT void CheckForPadUnplugged(void)
{
	printf("CheckForPadUnplugged(void)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// shared per-frame ease value for the title bar shake on some Shell_ menu screens
// (ScreenAdjust, ShowRecord both use it). tentative name, no idb match (0x5512EC).
// distinct from PCShell.cpp's PCSHELL_DoDisplayOptions/DoControllerConfig, which use
// a stack local for the same easing idiom.
EXPORT i32 gShellMenuEase;

// tentative name, no idb match (0x54D38C, checked right after Pad_Update() in several
// Shell_ menu loops; guessed to gate an early abort, e.g. game shutting down. nearest
// idb_globals.txt neighbour is SymBurnRegion at 0x54D388).
static u8 * const gShellMenuAbort = (u8*)0x54D38C;

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

// Widget class from the "pshell" Mac module (spiderman_names.txt:
// __ct__10CRecordBoxFiiP16STrainingMission at 0x47B1E0, Display__10CRecordBoxFv
// at 0x47B240, Update__10CRecordBoxFv at 0x47B5A0, NameEntryOn__10CRecordBoxFUc
// at 0x47B830, __dt__10CRecordBoxFv at 0x47AF00). Declared here, not in
// pshell.h/.cpp, because Shell_ShowRecord (shell.cpp) is the only caller found
// this session. Derives from CClass: the constructor never calls a base ctor
// (CClass has none) and never sets up more than one vtable slot, matching
// Shell_ShowRecord's cleanup call (vtable[0](1), the scalar deleting
// destructor CClass::operator new/delete already cover the alloc/free side).
// Field layout is read off the constructor's writes only (0x47B1E0, 85 bytes,
// decoded whole below); the gaps it never touches (field_28, field_30..3B,
// field_40) stay unlabelled padding until Display/Update get decompiled.
class CRecordBox : public CClass
{
	public:
		EXPORT CRecordBox(i32, i32, STrainingMission*);
		EXPORT virtual ~CRecordBox(void);
		EXPORT void Display(void);
		EXPORT void Update(void);
		EXPORT void NameEntryOn(u8);

		i32 field_4;
		i32 field_8;
		i32 field_C;
		i32 field_10;
		i32 field_14;
		i32 field_18;
		i32 field_1C;
		i32 field_20;
		i32 field_24;
		i32 field_28;
		i32 field_2C;
		u8 field_30[0xC];
		STrainingMission* field_3C;
		i32 field_40;
};

// CRecordBox's methods live in the same TU as their only caller
// (Shell_ShowRecord), so keep the inliner off them (same trick as
// gsub_498240/CheckForPadUnplugged above): the original calls all of these
// out-of-line.
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// @Ok
// @Matching
CRecordBox::CRecordBox(i32 width, i32 height, STrainingMission* pMission)
{
	field_1C = width;
	field_4 = 0xA;
	field_8 = 0xA;
	field_20 = height;
	field_C = 0x116;
	field_10 = 0x60;
	field_14 = 0x30;
	field_18 = 0xC;
	field_24 = 0;
	field_2C = 0x1C;
	field_3C = pMission;
}

// @SMALLTODO
CRecordBox::~CRecordBox(void)
{
	printf("CRecordBox::~CRecordBox(void)");
}

// @BIGTODO
void CRecordBox::Display(void)
{
	printf("CRecordBox::Display(void)");
}

// @MEDIUMTODO
void CRecordBox::Update(void)
{
	printf("CRecordBox::Update(void)");
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

// @NotOk
// Residue: the whole SEH/exception-unwind prologue+epilogue in the original
// (push -1; push offset handler; mov eax,fs:0; push eax; mov fs:0,esp ... down
// to mov fs:0,ecx at the end) is missing from our build, which cascades into
// ~167 mnemonic diffs (register allocation follows from the extra saved
// registers/EH-state slot the frame provides). Everything else about the
// function (all callees, globals, loop shape, CRecordBox construction/
// destruction, string/global addresses) is verified correct against the
// disassembly; see shell.attempts.md for 7 distinct hypotheses tried against
// this ONE root cause (all builds, all rebuilt+cmpsum'd): plain heap
// new+delete (own local pointer, immediate); heap new stored to a global with
// no delete in the function; heap new+delete with an intervening loop and an
// early return that skips the delete (matches the original's own control
// flow exactly); explicit __try/__except (produces a bigger, ebp-based,
// two-value frame, does not match); explicit __try/__finally (same, does not
// match); constructor definition moved after the call site to test whether
// visibility-order suppresses same-TU "can this throw" analysis (no
// difference, trivial ctor still got inlined regardless); a genuine
// stack-allocated (non-heap) local object of a class with a virtual
// destructor (THIS reproduces the exact original frame shape byte-for-byte:
// single push, esp-based, no ebp) but contradicts the original's confirmed
// heap allocation of CRecordBox (push 0x44; call CClass::operator new,
// present in the real disassembly, not something I can explain away). No
// combination tried gets a real heap allocation to carry the frame. Full
// instruction-byte accounting of the 686-byte original (iced_x86 decodes
// exactly 686 bytes with no gaps) rules out a second, still-unidentified
// stack object hiding in the body. Left for a future session with deeper
// tooling (IDA) to confirm what MSVC6 construct produces an exception frame
// around a plain heap allocation.
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

// @MEDIUMTODO
void Shell_Special(EShellResult)
{
    printf("Shell_Special(EShellResult)");
}

// @MEDIUMTODO
void Shell_StoryBoards(void)
{
    printf("Shell_StoryBoards(void)");
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

void validate_SRecordRelated(void)
{
	VALIDATE_SIZE(SRecordRelated, 0x10);

	VALIDATE(SRecordRelated, pName, 0x0);
	VALIDATE(SRecordRelated, field_6, 0x6);

	VALIDATE(SRecordRelated, field_8, 0x8);
	VALIDATE(SRecordRelated, field_9, 0x9);

	VALIDATE(SRecordRelated, field_C, 0xC);
}
