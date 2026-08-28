#pragma once

#ifndef SHELL_H
#define SHELL_H

#include "ob.h"
#include "export.h"
#include "reloc.h"
#include "main.h"

EXPORT extern CBody *MiscList;

struct SRecordRelated
{
	char* pName;

	PADDING(2);

	u8 field_6;

	PADDING(1);

	i8 field_8;
	i8 field_9;

	PADDING(0xC-0x9-1);

	i8 field_C;

	PADDING(0x10-0xC-1);
};

struct SScore
{
	u8 field_0;
	u8 field_1;
	u8 field_2;
	u8 field_3;
	u8 field_4;
};

#define NUM_CHALLS 23
#define NUM_RECORDS_PER_CHALL 5
struct SRecords
{
	PADDING(3);

	SScore mScores[NUM_CHALLS * NUM_RECORDS_PER_CHALL];
};

struct SSaveGame
{
	u32 mChecksum;

	// Sized from Front_LoadGame (front.cpp): the highest byte written/read
	// there is field_4[7] (offsets 0x68285D through 0x682863 relative to
	// gSaveGame at 0x682858), still inside the old padding block before
	// mRestartPointName at 0xD, so this does not move any other field.
	char field_4[9];

	char mRestartPointName[50];

	// @FIXME: figure proper size
	char field_3F[1];

	PADDING(0x54-0x3F-1);

	i8 mDifficulty;

	// ActivateCheat CHEAT_ROBRTSON ("Storyboard Viewer") toggle
	u8 mCheatStoryboardFlag;

	// PShell_MaybeUnlockStuff loops over 0x22 (34) of these checking completion
	u8 field_56[0x22];

	// ActivateCheat CHEAT_LEANEST ("everything") sets this to 1
	u8 field_78;

	PADDING(0x7B-0x79);

	u8 field_7B;

	// PShell_MaybeUnlockStuff stores its "which unlock popup" result here
	u8 field_7C;
	PADDING(0x80-0x7C-1);

	// ActivateCheat costume unlock bitmask (SECRTWAR/MIGUELOH/TRISNTNL/
	// SYNOPTIC/XILRTRNS/KICKME/MRWATSON/SMLVIII/CLUBNOIR cheats)
	i32 field_80;

	// ActivateCheat CHEAT_RGSGLLRY ("Character Viewer") = -1
	i32 field_84;

	// movie viewer unlock bitmask; also used directly as gSaveGame.field_88
	// in ps2gamefmv.cpp ("|= 1 << a1"). ActivateCheat CHEAT_CINEMA sets -1
	i32 field_88;

	// ActivateCheat CHEAT_FANBOY / CHEAT_LEANEST; same address as
	// powerup.cpp's gCheatUnlockFlags standalone global
	i32 field_8C;

	// ActivateCheat CHEAT_KIRBYFAN ("Game Comic Covers") = 0x3F
	i32 field_90;

	// PShell_ApplyGameState reads these and writes them into gGameState[11..13]
	i32 field_94;
	i32 field_98;
	i32 field_9C;

	// PShell_ApplyGameState writes gBootRomSoundMode here
	u8 field_A0;
	PADDING(0xA4-0xA0-1);

	// PShell_ApplyGameState copies this into DoubleBuffer[0/1].Disp.screen.x
	i16 field_A4;
	PADDING(0xA8-0xA4-2);

	// PShell_ApplyGameState copies this into DoubleBuffer[0/1].Disp.screen.y
	i16 field_A8;
	PADDING(0xAC-0xA8-2);

	// applied via Pad_SetDigitalMapping through gGameState[0..3]
	i16 mDigitalMapping[4];

	// applied via Pad_SetAnalogueMapping through gGameState[4..7]
	i16 mAnalogueMapping[4];
};

class CWobblyGlow : public CGlow
{
	public:
		EXPORT CWobblyGlow(CVector*, i32, i32, i32, u8, u8, u8, u8, u8, u8);
		EXPORT virtual void Move(void);

		i32 mInc[8];
		i32 mT[8];

		i32 mAmplitude;
		i32 mInnerRadius;
};

class CShellMysterioHeadGlow : public CWobblyGlow
{
	public:
		EXPORT CShellMysterioHeadGlow(void);
		EXPORT virtual void Move(void);

		u16 field_A4;

		PADDING(0xA8-0xA4-2);
};

class CRudeWordHitterSpidey : public CSuper
{
public:
	EXPORT CRudeWordHitterSpidey(void);
	EXPORT void AI(void);

	i32 field_1A4;
	i32 field_1A8;

};

class CDummy : public CSuper {
public:
	EXPORT void FadeBack(void);
	EXPORT void FadeAway(void);
	EXPORT void SelectNewTrack(i32);
	EXPORT void SelectNewAnim(void);

	u16* field_1A4;
	u16* field_1A8;
	u16* field_1AC;

	PADDING(0x1B8-0x1AC-4);

	u16* field_1B8;
	u16* field_1BC;
	i32 field_1C0;

	PADDING(0x1F8-0x1C0-4);


	i32 field_1F8;
	i32 field_1FC;

	PADDING(0x240-0x1FC-4);

	CItem field_240;

	PADDING(0x288 - 0x240 - sizeof(CItem));

	CItem field_288;

	PADDING(0x2d4 - 0x288 - sizeof(CItem));


	CVector field_2D4[4];
	CVector field_304[23];
	CVector field_418[128];
};

class Spidey_CIcon : public CSuper
{
	public:
		EXPORT Spidey_CIcon(i32, i32, i32);
		EXPORT Spidey_CIcon(i32);
		EXPORT void AI(void);
		EXPORT void SetIcon(i32);
};

class CShellSymBurn : public CSuper
{
	public:
		EXPORT CShellSymBurn(CVector*);
		EXPORT void AI(void);

		i32 field_1A4;
};

class CShellVenomElectrified : public CNonRenderedBit
{
	public:
		EXPORT CShellVenomElectrified(CSuper*);
		EXPORT virtual void Move(void);

		SHandle field_3C;
		i32 field_44;
};

class CShellCarnageElectrified : public CNonRenderedBit
{
	public:
		EXPORT CShellCarnageElectrified(CSuper*);
		EXPORT virtual void Move(void);

		SHandle field_3C;
		i32 field_44;
};

class CShellSuperDocOckElectrified : public CNonRenderedBit
{
	public:
		EXPORT CShellSuperDocOckElectrified(CSuper*);
		EXPORT virtual void Move(void);

		SHandle field_3C;
		i32 field_44;
};

class CShellRhinoNasalSteam : public CFlatBit
{
	public:
		EXPORT CShellRhinoNasalSteam(CVector*, CVector*);
		EXPORT virtual void Move(void);
};

class CShellEmber : public CFlatBit
{
	public:
		EXPORT CShellEmber(CVector*, i32);
		EXPORT virtual void Move(void);

		i32 field_68;
		i32 field_6C;
		i32 field_70;
		i32 field_74;
		i32 field_78;
		i32 field_7C;
		i32 field_80;
		i32 field_84;
		i32 field_88;
		i32 field_8C;
};

class CShellSimbyMeltSplat : public CQuadBit
{
	public:
		EXPORT CShellSimbyMeltSplat(CVector*);
		EXPORT virtual void Move(void);


		i32 field_84;
		i32 field_88;
		i32 field_8C;
		CVector field_90;
		CVector field_9C;
		CVector field_A8;
};

class CShellSimbyFireDeath : public CNonRenderedBit
{
	public:
		EXPORT CShellSimbyFireDeath(CDummy*);
		u8 fullPad[0x15];
};

class CShellGoldFish : public CBody
{
	public:
		EXPORT CShellGoldFish(CDummy*);
		EXPORT virtual ~CShellGoldFish(void);
		EXPORT virtual void AI(void);

		PADDING(4);

		SHandle field_F8;
		i32 field_100;
		i32 field_104;
		i32 field_108;
		i32 field_10C;
		i32 field_110;
		i32 field_114;
};

class CShellMysterioHeadCircle : public CQuadBit
{
	public:
		EXPORT CShellMysterioHeadCircle(CDummy*);
		EXPORT virtual ~CShellMysterioHeadCircle(void);
		EXPORT virtual void Move(void);

		SHandle field_84;

		PADDING(4);

		i32 field_90;
};

struct SpideyIconRelated
{
	char *Name;
	i32 IconModel;
	i16 field_8;

	PADDING(2);

	i16 field_C;

	PADDING(2);

	i32 field_10;
	i32 field_14;
	i32 field_18;

	PADDING(0x28-0x18-4);
};

// Size 0x10 confirmed by PShell_EndTrainingDisplay (pshell.cpp): the game's
// gChallenges table (0x00551118, named in idb_globals.txt) is 23 entries of
// this struct back to back, ending exactly at the next named global
// (gTrainingSeconds, 0x00551288: 0x551118 + 23*0x10 == 0x551288).
struct STrainingMission
{
	// tentative: read as a char* by Shell_ShowRecord (Mess_DrawText title arg)
	// and by PShell_EndTrainingDisplay through CRecordBox::field_3C (the
	// challenge/mission name).
	char* field_0;

	// Added 2026-08-27, found in PShell_EndTrainingInit (0x47B720, pshell.cpp):
	// the entry-search loop there reads this word for every gChallenges
	// entry (cmp word ptr [&gChallenges[i]+4], ax) and compares it against
	// the low 16 bits of Trig_GetLevelID()'s return value, still resident in
	// ax at that point. Only entries whose mLevelId matches get checked
	// further (against mAreaId below). Our own guess at the field's
	// purpose; not confirmed against the maintainer's IDB.
	i16 mLevelId;

	PADDING(0x7 - 0x6);

	// Added 2026-08-27, found in PShell_EndTrainingInit (0x47B720, pshell.cpp):
	// the entry-search loop there reads this byte for every gChallenges
	// entry (mov dl,[ecx] with ecx = &gChallenges[i] + 7) and compares it
	// against gTrainingSeconds (0x00551288, the value read once at the top
	// of that function) or the sentinel 0xFF (matches any). This picks
	// which gChallenges entry is "the one for the level that just ended".
	// Our own guess at the field's purpose (which training area/level this
	// challenge belongs to); not confirmed against the maintainer's IDB, no
	// per-field struct names are available from the 9.2 IDB export.
	i8 mAreaId;

	PADDING(0xB - 0x7 - 1);

	// renamed from field_B (2026-08-27): print_if_false in CRecordBox::Display
	// (0x47B240) reads this same byte through CRecordBox::field_3C and asserts
	// it with the literal string "Bad ScoreUnits" (confirmed by reading the
	// .data section of the original SpideyPC.exe at 0x551A80, the string the
	// assert pushes). Sign-extended, passed as DisplayScore's last argument;
	// used as a 4-way switch in Display (0=Time, 1=Kills, 2=Items, 3=Points
	// column header, strings at 0x54B8D0/D4/D8/DC in the original .data).
	i8 mScoreUnits;

	// Added 2026-08-27, found in PShell_EndTrainingInit (0x47B720, pshell.cpp):
	// the score-insertion loop there reads this byte (gChallenges[i]+0xC) once
	// per challenge and uses it to pick the comparison direction against the
	// new score (flag==0: insert where the new score is greater, "jg";
	// flag!=0: insert where the new score is less, "jl"). Our own guess:
	// whether a lower value ranks better for this challenge (true for
	// mScoreUnits==0, Time, false otherwise). Not confirmed against the
	// maintainer's IDB.
	i8 mLowerIsBetter;

	PADDING(0x10 - 0xC - 1);
};

// Widget class from the "pshell" Mac module. Moved here from shell.cpp (was
// file-local, comment said "only caller found this session") because
// pshell.cpp's PShell_EndTraining* functions use a second CRecordBox instance
// (own global pointer, own STrainingMission source) for the end-of-training
// score display, so the class is shared between the two TUs now. Derives from
// CClass: the constructor never calls a base ctor (CClass has none) and never
// sets up more than one vtable slot, matching Shell_ShowRecord's cleanup call
// (vtable[0](1), the scalar deleting destructor; CClass::operator new/delete
// already cover the alloc/free side). Field layout is read off the
// constructor's writes (sub_47B1E0, the 96 bytes right before Display in the
// original) plus Display (0x47B240) and Update (0x47B560), both confirmed by
// address in tools/names.json.
// 2026-08-27 correction: an earlier session's comment here attributed
// __ct__10CRecordBoxFiiP16STrainingMission/__dt__10CRecordBoxFv/
// NameEntryOn__10CRecordBoxFUc to specific addresses "from spiderman_names.txt"
// (the Mac IDB). That IDB actually has these at 0xc7b60/0xc7c10/0xc8320 (checked
// against idbs/spiderman_names.txt directly), nothing like the PC-looking hex
// quoted before. The quoted addresses were real PC addresses (0x47B1E0 is the
// ctor, confirmed working) but 0x47AF00 and 0x47B830 are NOT the destructor or
// NameEntryOn: 0x47AF00 is a tiny unrelated sub inside the neighbouring
// CExpandingBox functions, and 0x47B830 falls inside PShell_EndTrainingInit's
// own byte range. Left ~CRecordBox and NameEntryOn as todo stubs rather than
// decompiling against the wrong bytes; their real PC addresses are still
// unconfirmed as of this note.
// field_30..3B layout (2026-08-27, read off Update/Display): field_30 is read
// as a plain 4-byte int (both functions test it non-zero before doing
// anything, most likely a CExpandingBox-style "active/visible" flag rather
// than something CRecordBox owns outright -- see the CExpandingBox comment on
// the constructor). field_38 is the one CRecordBox::Update's print_if_false
// names "Bad mLetterIndex" via the string at 0x551A90 in the original .data,
// confirmed the same way as mScoreUnits above: the current typed letter
// position (0..2) into the 3-letter high score name. field_39 is the currently
// selected score row (0..NUM_RECORDS_PER_CHALL-1), no string evidence for a
// name yet. field_34/35/36 drive Display's row-highlight blink and have no
// string evidence either. field_37/3A/3B are never read or written by
// Update/Display; kept as padding.
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
		i32 field_30;
		u8 field_34;
		u8 field_35;
		u8 field_36;
		PADDING(1);
		u8 mLetterIndex;
		u8 field_39;
		PADDING(2);
		STrainingMission* field_3C;
		i32 field_40;
};

enum EShellResult
{
};

class CMenu;

static const i32 INPUT_MAX_SIZE = 9;

EXPORT i32 Shell_DeRudify(char[INPUT_MAX_SIZE]);
EXPORT i32 Shell_ContainsSubString(const char*, const char*);
EXPORT void CallAI(CBody *);
EXPORT i32 CalcIndexOfContinueLevel(void);

EXPORT void Shell_AddGameSlots(CMenu *);
EXPORT u32 Shell_CalculateGameChecksum(SSaveGame *);
EXPORT void Shell_CharacterViewer(void);
EXPORT void Shell_Cheats(void);
EXPORT i32 Shell_ChooseEnemy(i32,u8,signed char);
EXPORT void Shell_ChooseItemCollection(i32);
EXPORT void Shell_ChooseSpeedTraining(i32);
EXPORT void Shell_ChooseSurvivalArena(i32);
EXPORT void Shell_ChooseTime(i32,i32);
EXPORT i32 Shell_ChooseTrainingControlType(void);
EXPORT void Shell_ChooseTrainingMission(i32);
EXPORT void Shell_ComicCollection(void);
EXPORT void Shell_CostumeViewer(void);
EXPORT i32 Shell_Difficulty(i32);
EXPORT void Shell_DisplayGameInfo(i32,i32,SSaveGame *);
EXPORT void Shell_DoShell(const u32 *,u32 *);
EXPORT void Shell_DrawBackground(void);
EXPORT void Shell_DrawTitleBar(i32,i32,const char *,i32,i32,i32,i32,i32);
EXPORT void Shell_Gallery(EShellResult);
EXPORT void Shell_GameCovers(void);
EXPORT i32 Shell_InputName(char *,i32,i32, const char *);
EXPORT void Shell_LegalScreen(void);
EXPORT i32 Shell_LevelSelect(void);
EXPORT void Shell_LoadGame(void);
EXPORT void Shell_MainMenu(EShellResult);
EXPORT i32 Shell_MemoryCard(EShellResult);
EXPORT void Shell_MovieViewer(void);
EXPORT void Shell_Options(EShellResult);
EXPORT void Shell_RollCredits(void);
EXPORT void Shell_SFXMusic(void);
// helpers used by Shell_SFXMusic (forwarded to original)
EXPORT void DrawSlider(i32, i32, i32, i32);
EXPORT i32 SliderDrag(i32, i32, i32);
EXPORT void sub_515850(void);
EXPORT void Shell_SaveGame(const u32 *,u32 *);
EXPORT void Shell_ScreenAdjust(void);
EXPORT void Shell_ShowRecord(char const *,char const *,STrainingMission *);
EXPORT void Shell_Special(EShellResult);
EXPORT void Shell_StoryBoards(void);
EXPORT void Shell_TitleScreen(void);
EXPORT void Shell_VerySmallFont(void);
EXPORT void PShell_Cleanup(void);
EXPORT void PShell_Initialise(void);
EXPORT void PShell_LowText(void);
EXPORT i32 RecordsExist(u8,i8, i8);
EXPORT i32 SameScore(const SScore *,const SScore *);
EXPORT void Merge(SRecords *, const SRecords *);
EXPORT void Merge(SScore *, const SScore *,i32);
EXPORT i32 IsBetter(i32, i32, i32);

EXPORT void Shell_RelocatableModuleInit(reloc_mod *);

EXPORT extern SAnimFrame* gBackgroundAnimFrame;

void validate_CRudeWordHitterSpidey(void);
void validate_CDummy(void);
void validate_CWobblyGlow(void);
void validate_CShellMysterioHeadGlow(void);
void validate_Spidey_CIcon(void);
void validate_CShellSymBurn(void);
void validate_CShellVenomElectrified(void);
void validate_CShellCarnageElectrified(void);
void validate_CShellSuperDocOckElectrified(void);
void validate_CShellRhinoNasalSteam(void);
void validate_CShellEmber(void);
void validate_CShellSimbyMeltSplat(void);
void validate_CShellSimbyFireDeath(void);
void validate_CShellGoldFish(void);
void validate_CShellMysterioHeadCircle(void);
void validate_SpideyIconRelated(void);
void validate_SSaveGame(void);
void validate_SScore(void);
void validate_SRecords(void);
void validate_STrainingMission(void);
void validate_SRecordRelated(void);
void validate_CRecordBox(void);

#endif
