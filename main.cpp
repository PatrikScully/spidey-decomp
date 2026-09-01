#include "non_win32.h"

// #define BOOT_GAME
#define MODEL_PREVIEW

// dev-only: skip the CD-ROM disc check so the game runs under Wine without
// a real mixed-mode disc. Off by default. Never uncomment this on a branch
// meant for an upstream PR.
#define SPIDEY_NO_CD_CHECK

#include <stdlib.h>

// #define LOCK_VALIDATION

#include "main.h"
#include "my_assert.h"
#include "ob.h"
#include "vector.h"
#include "friction.h"
#include "bit.h"
#include "front.h"
#include "pshell.h"
#include "baddy.h"
#include "mj.h"
#include "submarin.h"
#include "venom.h"
#include "ps2funcs.h"
#include "blackcat.h"
#include "torch.h"
#include "hostage.h"
#include "cop.h"
#include "carnage.h"
#include "chopper.h"
#include "docock.h"
#include "jonah.h"
#include "lizard.h"
#include "lizman.h"
#include "mysterio.h"
#include "platform.h"
#include "rhino.h"
#include "scorpion.h"
#include "simby.h"
#include "spclone.h"
#include "superock.h"
#include "thug.h"
#include "turret.h"
#include "shell.h"
#include "web.h"
#include "bit2.h"
#include "camera.h"
#include "quat.h"
#include "mem.h"
#include "exp.h"
#include "m3dcolij.h"
#include "m3dinit.h"
#include "spidey.h"
#include "message.h"
#include "bullet.h"
#include "trig.h"
#include "effects.h"
#include "FontTools.h"
#include "wire.h"
#include "powerup.h"
#include "switch.h"
#include "chain.h"
#include "Image.h"
#include "ps2pad.h"
#include "bitmap256.h"
#include "PCTex.h"
#include "smoke.h"
#include "panel.h"
#include "manipob.h"
#include "mess.h"
#include "ai.h"
#include <cstring>
#include "spool.h"
#include "l1a3bomb.h"
#include "chunk.h"
#include "weapons.h"
#include "backgrnd.h"
#include "dcshellutils.h"
#include "pkr.h"
#include "pcdcFile.h"
#include "ps2lowsfx.h"
#include "PCInput.h"
#include "PCShell.h"
#include "stubs.h"
#include "SpideyDX.h"
#include "DXsound.h"
#include "DXinit.h"
#include "pack.h"
#include "pal.h"
#include "db.h"
#include "ps2m3d.h"
#include "PCGfx.h"
#include "ps2gamefmv.h"
#include "init.h"
#include "utils.h"
#include "reloc.h"
#include "my_bink.h"
#include "pcdcMem.h"
#include "dcmemcard.h"
#include "ps2card.h"
#include "pcdcBkup.h"
#include "pcdcPad.h"
#include "vram.h"
#include "m3dzone.h"
#include "PRE.h"
#include "dcfileio.h"
#include "PCMovie.h"
#include "flash.h"
#include "screen.h"
#include "post.h"
#include "PCTimer.h"
#include "music.h"
#include "shatter.h"
#include "tweak.h"
#include "ps2redbook.h"


#include "my_patch.h"

extern int FAIL_VALIDATION;

const i32 POLYBUFFERSIZE = 0x17000;

EXPORT i32 gMainStuff[0x1000];

// ---------------------------------------------------------------------------
// Globals the game loop (Logic / Display / PlayAway / SpideyMain) needs and
// that have no repo variable yet. File-local fixed-address pointers with
// tentative descriptive names. Every address was checked against
// ~/Documents/spidey-work/idbs/idb_globals.txt and against the known-size repo
// objects first, so none of them names a slot inside a bigger global. Where
// another file already invented a name for the same address the same name is
// reused and that file is named in the comment.
// ---------------------------------------------------------------------------

// pshell.cpp uses this name for the same address. Logic skips the whole world
// update while it is set, so it reads as "the level is frozen behind the
// end-of-training screen".
static i32 * const gEndTrainingFlag = reinterpret_cast<i32*>(0x0060CFB0);

// pshell.cpp uses this name for the same address. Gates the two end of
// training screen calls, PShell_EndTrainingUpdate and PShell_EndTrainingDisplay.
static i32 * const gTrainingActive = reinterpret_cast<i32*>(0x00682950);

// baddy.cpp uses this name for the same address.
static u8 * const gSubmarinerDieRelated = reinterpret_cast<u8*>(0x0060CFC4);

// only ever tested against zero, and when it is clear the code calls the empty
// debug hook at 0x430880. A release build leftover.
static i32 * const gLogicDebugHookFlag = reinterpret_cast<i32*>(0x0060CFE4);

// Display draws this string in red at 256,60 when it is not null, so it is a
// debug banner. Nothing in the repo writes it.
static char ** const gDebugBanner = reinterpret_cast<char**>(0x0060CF9C);

// ten dwords in a row, each one a "draw this list" switch that Display reads
// once, right before the matching M3d_Render call. Kept as one indexed block
// rather than ten names because they are contiguous and used identically:
// 0 EnviroList, 1 EnvironmentalObjectList, 2 the player, 3 the player's extra
// body parts, 4 MiscellaneousRenderingList, 5 MiscList, 6 BaddyList,
// 7 BulletList, 8 PowerUpList, 9 BackgroundList.
static i32 * const gRenderListFlags = reinterpret_cast<i32*>(0x0054D350);

// idb_globals.txt: GrenadeExplosionRegion. A wibbly texture region id, only
// preprocessed while g3DExplosions is set.
static i32 * const gGrenadeExplosionRegion = reinterpret_cast<i32*>(0x0054A37C);

// idb_globals.txt: gPsxRingIndex / FireRingRegion. Display picks between the
// two for the fire ring depending on gFireRingObject's field_10C.
static i32 * const gPsxRingIndex = reinterpret_cast<i32*>(0x0055AA34);
static i32 * const gFireRingRegion = reinterpret_cast<i32*>(0x0055AA3C);

// idb_globals.txt: FireDomeRegion, the matching region for gFireDomes.
static i32 * const gFireDomeRegion = reinterpret_cast<i32*>(0x0055AA38);

// idb_globals.txt: SymBurnRegion. simby.cpp already calls the counter at
// 0x60CF94 gSymBurnCount; this is the region it turns on.
static i32 * const gSymBurnCount = reinterpret_cast<i32*>(0x0060CF94);
static i32 * const gSymBurnRegion = reinterpret_cast<i32*>(0x0054D388);

// the live fire ring object. Tentative name: it sits two slots before
// gFireDomes/gNumDomes (web.cpp) and Display only reads its field_10C to
// choose which of the two ring regions to preprocess.
static CSuper ** const gFireRingObject = reinterpret_cast<CSuper**>(0x006B559C);

// ps2m3d.cpp calls these gM3dSuperScaleEnabled and gDCUseFixedScale. Display
// turns the fixed vertex scale on around the player's extra body parts.
static i32 * const gM3dSuperScaleEnabled = reinterpret_cast<i32*>(0x005500A4);
static i32 * const gDCUseFixedScale = reinterpret_cast<i32*>(0x0060CF90);

// front.cpp calls this gFrontDrawPolyFlag. It gates the ordering table walk at
// the end of Display, the Mac build's CountPrimitives (inlined here on PC).
static i32 * const gFrontDrawPolyFlag = reinterpret_cast<i32*>(0x0060CFE0);

// where that walk leaves its result: how many ordering table entries carried a
// primitive this frame.
static i32 * const gOtPrimitiveCount = reinterpret_cast<i32*>(0x005FCD68);

// how many bytes of the poly buffer the frame used, written at the very end of
// Display.
static i32 * const gPolyBufferUsed = reinterpret_cast<i32*>(0x0060D00C);

// pshell.cpp calls this gFrontUseAltTriggerMask. PlayAway sets it on entry and
// SpideyMain clears it again once the level ends.
static u8 * const gFrontUseAltTriggerMask = reinterpret_cast<u8*>(0x005FAE9D);

// pshell.cpp calls this gDoShellForceLevelExit, front.cpp calls it
// gFrontShowTrainingTip. PlayAway reseeds the random generator from it.
static i32 * const gDoShellForceLevelExit = reinterpret_cast<i32*>(0x0068293C);

// pshell.cpp calls this gShellMenuAbort. When it is set PlayAway never starts
// the frame loop at all: it just repaints the sky and reports end code 11,
// which sends SpideyMain straight back to the shell. That is how the
// storyboard and comic viewers get out of a "level".
static i32 * const gShellMenuAbort = reinterpret_cast<i32*>(0x0054D38C);

// cleared next to gWaterEffect at the top of PlayAway; purpose unknown, no
// other reader found. Named after its neighbour.
static i32 * const gWaterEffectTwo = reinterpret_cast<i32*>(0x0060FAA0);

// cleared next to gAttackRelated at the top of PlayAway; purpose unknown.
static i32 * const gPlayAwayCounter = reinterpret_cast<i32*>(0x005FCD18);

// nonzero draws the frame rate readout in the top left corner.
static i32 * const gShowFrameRate = reinterpret_cast<i32*>(0x0060CFFC);

// idb_globals.txt: OTPushback. PlayAway forces it back to 1 when the level
// ends. ps2m3d.cpp reads the same address as gM3dOtPushback[0..2].
static i16 * const gOtPushback = reinterpret_cast<i16*>(0x00660F78);

// post.cpp defines this one, it just has no header entry yet.
EXPORT extern i32 gWaterEffect;


// ---------------------------------------------------------------------------
// Two callees of Logic that belong to ob.cpp, not here. The Mac build puts
// both in its ob.cpp TU (tools/prototypes.json, "ob": "Ob_AI(CBody **,int)"
// and "Ob_MaybeUnSuspendOrCull(void)"), and on PC they sit at 0x460FC0 and
// 0x461160, right next to the other Ob_ functions. The repo has neither, and
// ob.cpp/ob.h are owned by somebody else right now, so they are forwarded to
// the original code from here to keep the call sites honest. Move them into
// ob.cpp (and drop these) as soon as that file is free.
// ---------------------------------------------------------------------------

// Ob_AI: walks one object list, ages each item out to the suspended list when
// it is further from the player than SuspendedDistance, and otherwise runs its
// EveryFrame/AI pair.
// @Bogus
static void Ob_AI(CBody **ppList, i32 a2)
{
	typedef void (*func_ptr)(CBody**, i32);
	func_ptr func = (func_ptr)0x00460FC0;
	func(ppList, a2);
}

// Ob_MaybeUnSuspendOrCull: the other direction, walks the suspended list and
// puts anything that came back inside SuspendedDistance on its home list again.
// @Bogus
static void Ob_MaybeUnSuspendOrCull(void)
{
	typedef void (*func_ptr)(void);
	func_ptr func = (func_ptr)0x00461160;
	func();
}


// @Ok
// @Matching
void CalcPolyBufferEnd(void)
{
	PolyBufferEnd = reinterpret_cast<u8*>(
			(reinterpret_cast<u32>(pDoubleBuffer->Polys) + POLYBUFFERSIZE - 0x100) & 0x7FFFFFFF);
}

// @Ok
// 0x00455400, 416 bytes. One half of the per frame work (Display is the other
// half); PlayAway calls both once a frame. gRenderTest bit 0x400 or 0x200 cuts
// it down to nothing but a pad read, and bit 0x100 turns it into a single step
// debugger that stops on key 57 (space) every frame.
void Logic(void)
{
	if (gRenderTest & 0x600)
	{
		Pad_Update();
		return;
	}

	if (gRenderTest & 0x100)
	{
		PCTIMER_Pause();

		// wait for the key to come up, then for the next press. Both loops
		// also give up if the single step bit is cleared while they spin.
		do
		{
			PCINPUT_PollKeyboard();
		}
		while (PCINPUT_IsKeyPressed(57, 0) && (gRenderTest & 0x100));

		do
		{
			PCINPUT_PollKeyboard();
		}
		while (!PCINPUT_IsKeyPressed(57, 0) && (gRenderTest & 0x100));

		PCTIMER_Resume();
	}

	gAttackRelated++;
	TTime++;

	Pad_Update();

	if (!G_POST_WATER_EFFECT && !*gEndTrainingFlag)
	{
		Flash_Update();

		Trig_ResetCPCollisionFlags();
		Ob_AI(reinterpret_cast<CBody**>(&MechList), 0);
		Trig_ResetCPExecutedFlags();

		Ob_AI(&PowerUpList, 0);
		Ob_AI(&BulletList, 0);
		Ob_AI(&MiscList, 0);
		Ob_AI(&EnvironmentalObjectList, 0);
		Ob_AI(reinterpret_cast<CBody**>(&BackgroundList), 0);
		Ob_AI(reinterpret_cast<CBody**>(&BaddyList), 0);
		Ob_AI(&ControlBaddyList, 0);

		Ob_MaybeUnSuspendOrCull();

		Bit_Move();
		Bit_RemoveDeadBits();
	}

	Shatter_MaybeMakeGlassShatterSound();
	Trig_DoPendingCommandLists();

	if (*gSubmarinerDieRelated)
		MechList->CutSceneSkipCleanup();

	gsub_430880();

	Front_Update();

	if (*gTrainingActive)
		PShell_EndTrainingUpdate();

	Mess_Update();

	if (!*gLogicDebugHookFlag)
		gsub_430880();

	// no null check on CameraList in this branch, the original does not have
	// one either.
	if (G_GAMESTATE[8])
	{
		G_GAMESTATE[8] = 1;
		CameraList->SetMode(CAMERAMODE_ITSYLOOKDOWN);
	}
	else if (CameraList && CameraList->mCameraMode == CAMERAMODE_ITSYLOOKDOWN)
	{
		CameraList->SetMode(CAMERAMODE_DEMO);
	}
}

// @BIGTODO
// Re-checked twice with idalib against the real exe. Address 0x00455C90, 4555
// bytes of the poly HUD aside this is the top level game state machine: 434
// instructions, 77 basic blocks, WinMain is its only caller. tools/names.json
// does have it (SpideyMain), and so does the maintainer's IDB.
//
// STATUS: still not implemented, on purpose, see "What actually blocks this"
// at the bottom. Everything else below is now verified, so whoever picks this
// up should not need IDA again except for the two missing callees.
//
// The 2026-08-31 pass listed "six small never-named leaf helpers with no repo
// stub at all" as the main blocker. That was WRONG and is corrected here: five
// of those six are named in tools/names.json AND already written in this repo.
// The corrected map is:
//   0x50A6B0 = PCINPUT_SetMouseBounds (PCInput.h, done)
//   0x458C20 = Mess_UnloadAllFonts    (mess.h, done)
//   0x50C160 = PCSHELL_Shutdown       (PCShell.h, done)
//   0x4305C0 = Db_DeleteOTsAndPolyBuffers (db.h, done)
//   0x47D3A0 = Reloc_Unload(char*)    (reloc.h, done)
//   0x430880 = nullsub_3, a single 0xC3 (ret) in the shipped binary, takes one
//              int, always the value Trig_GetLevelId just returned. A debug or
//              telemetry hook the release build compiled away. No repo home:
//              it sits between Db_DeleteOTsAndPolyBuffers (db.cpp) and
//              FileIO_Init (dcfileio.cpp). Same class as panel.cpp's
//              gsub_4015B0, so it can be a local empty stub.
//
// CALL GRAPH, all of it, with the repo name and header of each callee:
//   Init_AtStart(init.h)             Init_Cleanup(init.h)  Init_AtEnd(init.h)
//   PCTex_LoadPcIcons/FreePcIcons(PCTex.h)  GameFMV_PlayMovie(ps2gamefmv.h)
//   Spool_ClearAllPSXs(spool.h)      PCGfx_DoModelPreview/EndScene(PCGfx.h)
//   Mess_LoadFont/Mess_UnloadAllFonts(mess.h)  PShell_NormalFont(pshell.h)
//   M3dInit_SetFoggingParams(m3dinit.h)        Utils_CopyString/
//   Utils_CompareStrings(utils.h)    Reloc_Load/Reloc_CallUserFunction/
//   Reloc_Unload(reloc.h)            DXINIT_SetDisplayOptions(DXinit.h)
//   PCINPUT_SetMouseBounds/SetMousePosition(PCInput.h)
//   Trig_GetLevelId/Trig_ParseTRGFile/Trig_ExecuteRestart(trig.h)
//   Front_LoadGame/Front_GetLevelIndex/Front_FindLevel/Front_ClearScreen
//   (front.h)                        Screen_SepiaFade(screen.h)
//   PShell_MaybeUnlockStuff/PShell_MaybeSaveGame(pshell.h)  Pause(utils.h)
//   PCSHELL_Shutdown(PCShell.h)      Db_DeleteOTsAndPolyBuffers(db.h)
//   CClass::operator new(main.h)     CPlayer::CPlayer(spidey.h, still a stub)
//   CCamera::CCamera(camera.h)       print_if_false(export.h)
//   sub_515D80 (the periodic CD recheck, keep away from it, see
//   SPIDEY_NO_CD_CHECK above)        PlayAway 0x4559D0 and Front_ContinueExit
//   0x47D830 (the two that do not exist in the repo yet, see below).
//
// GLOBALS, all resolved. Already in the repo: gMainStuff(0x5FCE04),
// gRenderTest(0x2E0988C), gRunCinemaRelated(0x6B470C), gVlanksRelated
// (0x6B4C9C, the IDB calls it GameFade), gLevelStatus(0x60CFA4),
// gWhatIf(0x60CFC5), DifficultyLevel(0x54D474), pYesNoMenu(0x5FAEAC),
// M3d_FadeColour(0x652F38), gLowGraphics(0x6B78F8),
// gBrightnessRelated(0x562D60), gSaveGame(0x682858).
// Named in idb_globals.txt but not in the repo yet: gSpideyMainRelated
// (0x54A510, set to 5 or 20 on the way out of a level), gBitServer(0x56EB50,
// deleted through its vtable at shutdown), Levels(0x54A518, 20 byte entries,
// the code string is field +4, which is why the disasm shows
// off_54A51C[5*index]).
// Still unnamed anywhere, would need tentative names:
//   0x568FB0  start level index, -1 = none. When >= 0 the boot path copies
//             Levels[it].code into gSaveGame and clears the restart point.
//   0x60CF84  base of the debug level cycle table. Indexed as
//             *(char**)(0x60CF84 + 192*index), first field is the level name;
//             MSVC folded the field offset into the base, so 0x60CF84 is
//             "name of record 0", not necessarily the array start (gLevelStatus
//             at 0x60CFA4 sits inside what would otherwise be record 0).
//   0x60CFA0  the index into that table, only its low byte is used.
//   0x60CF88  byte, checked once at boot; nonzero means skip the whole game
//             loop, Init_Cleanup(3) and shut down. Reads like a quit flag.
//   0x6612A0  dword zeroed at the top of every inner loop pass (near
//             Pad_IdleTime 0x66129C).
//   0x5FAE9D  byte cleared whenever gLevelStatus != 0 (near gPostWaterEffect).
//   0x2E096F8 / 0x2E0970C / 0x2E098E4  the width, height and bpp handed to
//             DXINIT_SetDisplayOptions (confirmed against that function's own
//             parameter names in DXinit.cpp).
//
// gSaveGame FIELDS THIS FUNCTION NEEDS. All the 0x6828xx addresses in the
// disasm are gSaveGame (0x682858) plus an offset, so per the address audit rule
// they must NOT get standalone names:
//   0x68285C = +0x04  field_4      (the level code string, already in SSaveGame)
//   0x682865 = +0x0D  mRestartPointName[0]                     (already there)
//   0x6828AE = +0x56  field_56[index]  per area completion count, incremented
//                     on level finish, clamped at 0xFF          (already there)
//   0x6828A0 = +0x48  NOT A FIELD YET, inside PADDING(0x54-0x3F-1)
//   0x6828A4 = +0x4C  NOT A FIELD YET, same padding block
//   0x6828A8 = +0x50  NOT A FIELD YET, same padding block
//   0x6828D1 = +0x79  NOT A FIELD YET, inside PADDING(0x7B-0x79)
// The three at 0x48/0x4C/0x50 are written as dwords and the one at 0x79 as a
// byte, all cleared to 0 together with mRestartPointName when a start level is
// forced. Splitting those two PADDING runs in shell.h costs nothing (the struct
// size does not move) but shell.h is shared, so it wants its own commit and its
// own VALIDATE entries.
//
// BEHAVIOUR, exactly as the disassembly has it:
//  1. Boot. print_if_false("xxx main"), fill gMainStuff with 'STAK' and put
//     'HALT' in slot 0, Init_AtStart(1), PCTex_LoadPcIcons, the four boot
//     movies GameFMV_PlayMovie(0..3, 1, 1, 2.5/1.0/1.0/1.0), Init_Cleanup(0),
//     gRunCinemaRelated = 0, busy wait until gVlanksRelated hits 0. If
//     gRenderTest & 8: Spool_ClearAllPSXs, PCGfx_DoModelPreview,
//     Init_Cleanup(0), jump straight to shutdown. That is the slice the stub
//     below already covers, and it is all of it.
//  2. Otherwise load font_big.fnt, sp_fnt02.fnt and sp_fnt03.fnt through
//     Mess_LoadFont(name,-1,-1,-1), then PShell_NormalFont.
//  3. Outer loop head (the target of "go back to the shell"). Set
//     M3d_FadeColour = 0xFFFFFF and M3dInit_SetFoggingParams(0, 6000, 2048).
//     Then either
//       a. gRenderTest & 4, or a start level index >= 0: if the index is >= 0,
//          copy Levels[index] code into gSaveGame.field_4 with
//          Utils_CopyString(...,9) and clear the four save fields listed above
//          plus mRestartPointName[0]; or
//       b. neither: run the shell. Reloc_Load("shell", 0),
//          Reloc_CallUserFunction("shell", 0, params, 0) where params is a two
//          entry u32 array holding two local flags (set by case 3 and case 10
//          below, both cleared as they are copied in), then
//          Reloc_Unload("shell"). If the level is "l1a2_t" and gWhatIf is set,
//          rewrite it to "l1a2a_t". If the 0x60CF88 quit byte is set, do
//          Init_Cleanup(3) and go to shutdown.
//  4. DXINIT_SetDisplayOptions(width, height, bpp, gLowGraphics,
//     gBrightnessRelated), PCINPUT_SetMouseBounds(0, 0, width-32, height-32),
//     PCINPUT_SetMousePosition((width-32)>>1, (height-32)>>1) (unsigned shifts).
//  5. Level entry point. sub_515D80 (CD recheck), the nullsub_3 hook with
//     Trig_GetLevelId, then Front_LoadGame(&gSaveGame, 0, false).
//  6. Inner loop, once per pass: PlayAway(), PCGfx_EndScene(1),
//     0x6612A0 = 0, and if gLevelStatus is neither 2 nor 9 also
//     gRunCinemaRelated = 0, and if gLevelStatus != 0 also 0x5FAE9D = 0. Then
//     switch (gLevelStatus), 11 cases:
//       1     Init_Cleanup(0), gSpideyMainRelated = 5, back to step 5.
//       2, 9  gSpideyMainRelated = 20, Init_Cleanup(0), Screen_SepiaFade,
//             gVlanksRelated = 10 and busy wait. Then Front_ContinueExit(): if
//             it returns 0 fall into the case 7 tail (quit), else the level id
//             hook again and Front_LoadGame(&gSaveGame, 1, false), stay in the
//             inner loop.
//       3     level finished. Init_Cleanup(0), Screen_SepiaFade. If the level
//             is "l8a7_t": bump gSaveGame.field_56[Front_GetLevelIndex("l8a6_t")]
//             (clamped at 0xFF), PShell_MaybeUnlockStuff, set the first shell
//             flag and go back to step 3. Otherwise bump
//             field_56[Front_GetLevelIndex(current) - 1] the same way (with a
//             0 <= i < 34 assert), PShell_MaybeUnlockStuff, wait for
//             gVlanksRelated, and unless gRenderTest & 0x80, call
//             PShell_MaybeSaveGame when DifficultyLevel is 0 or when
//             Front_FindLevel(current)->field_8 & 2. Then the level id hook and
//             Front_LoadGame(&gSaveGame, 0, true).
//       4, 5  Init_Cleanup(0), advance the debug level index by one byte and
//             wrap to 0 when the next record's name string is empty, back to
//             step 5.
//       6     enter the level. Init_Cleanup(2), Trig_ParseTRGFile, then
//             new CPlayer (CClass::operator new(3836) plus CPlayer::CPlayer,
//             inside an EH cleanup frame), Trig_ExecuteRestart, then
//             new CCamera(thatPlayer) (operator new(756) plus
//             CCamera::CCamera, second EH frame). Stay in the inner loop.
//       7     gSpideyMainRelated = 20, Init_Cleanup(0), Screen_SepiaFade, then
//             the shared quit tail: if gRenderTest & 4 or a start level index
//             is set, shut down, else go back to step 3.
//       8     Front_ClearScreen, Init_Cleanup(0), clear
//             gSaveGame.mRestartPointName[0], level id hook,
//             Front_LoadGame(&gSaveGame, 0, false), stay in the inner loop.
//       10    set the second shell flag, gSpideyMainRelated = 20,
//             Init_Cleanup(0), Screen_SepiaFade, back to step 3.
//       11    Init_Cleanup(0), back to step 3.
//       other print_if_false(0, "Unknown EndCode") and shut down.
//  7. Shutdown. Busy wait on gVlanksRelated calling Pause(1), delete pYesNoMenu
//     and gBitServer through vtable slot 0 with arg 1 (the scalar deleting
//     destructor), Mess_UnloadAllFonts, PCSHELL_Shutdown, PCTex_FreePcIcons,
//     Db_DeleteOTsAndPolyBuffers, Init_AtEnd, return.
// Note Screen_SepiaFade: the original pushes one argument at every call site,
// but 0x48A820 never reads it and screen.cpp already declares it void. Same for
// the extra unused argument DCDrawGouraudPoly gets in panel.cpp.
//
// WHAT ACTUALLY BLOCKS THIS (the only three things left):
//   1. PlayAway, 0x4559D0. Called once per inner loop pass, so it IS the per
//      frame driver: about 170 instructions and roughly 28 further callees,
//      almost none of them decompiled. It sits squarely in main.cpp's own
//      address range (CClass::operator new 0x455390, CItem::operator delete
//      0x4553D0, CalcPolyBufferEnd 0x4553E0, Logic 0x455400, Display 0x4555A0,
//      PlayAway 0x4559D0, SpideyMain 0x455C90), so it, Logic and Display all
//      belong in main.cpp and none of the three exists here yet. The repo's own
//      leaf first rule says do the same TU callees first, and MSVC would inline
//      a printf stub of PlayAway straight into SpideyMain, so writing SpideyMain
//      before PlayAway is the wrong order.
//   2. Front_ContinueExit, 0x47D830, 234 instructions. Decompiled far enough to
//      say what it is: it builds a CMenu (front.cpp's 0x43F9B0 constructor),
//      runs its own input loop, and returns 1 when the player picks continue.
//      Its address is past reloc.cpp's functions (Reloc_Load 0x47CEE0 ...
//      Reloc_CallUserFunction 0x47D470) and its body is front end code, so its
//      real home is probably front.cpp or pshell.cpp, not the file names.json's
//      "Front_" prefix would suggest by address. Not declared anywhere yet.
//   3. CPlayer::CPlayer is still a printf stub (@MEDIUMTODO in spidey.cpp), and
//      case 6 is the only way into a level.
// Order to do this in: PlayAway (with Logic and Display, they are its
// neighbours and probably its callees), then Front_ContinueExit, then this.
// The stub below only reproduces step 1 and the model preview branch; it is not
// a faithful SpideyMain. The real one never returns during normal play.
void SpideyMain(void)
{
	DXERR_printf("xxx main\n");
	for (i32 i = 0; i < 0x1000; i++)
	{
		gMainStuff[i] = 0x4B415453;
	}

	gMainStuff[0] = 0x544C4148;

	Init_AtStart(1);
	PCTex_LoadPcIcons();
	GameFMV_PlayMovie(0, 1, 1, 2.5f);
	GameFMV_PlayMovie(1, 1, 1, 1.0f);
	GameFMV_PlayMovie(2, 1, 1, 1.0f);
	GameFMV_PlayMovie(3, 1, 1, 1.0f);

	Init_Cleanup(0);
	gRunCinemaRelated = 0;

	while (gVlanksRelated)
		;

#ifndef MODEL_PREVIEW
	if (gRenderTest & 8)
#else
	if(1)
#endif
	{
		Spool_ClearAllPSXs();
		PCGfx_DoModelPreview();
		Init_Cleanup(0);
	}
	else
	{
	}
}

// @Ok
// @Matching
// @Leak
void* CClass::operator new(size_t size)
{
	void *pnew = Mem_New(size);

	// Ensure size is a multiple of 4.
	size = ( size + 3 ) & ~0x03;

	// Zero all the newly allocated memory
	u32 *p=(u32 *)pnew;
	for (i32 i=0; i<size/4; ++i) *p++=0;

	return pnew;
}

// @Ok
void CClass::operator delete(void *ptr)
{
	Mem_Delete(ptr);
}

// @Ok
CClass::~CClass()
{
}

template<bool b>

struct StaticAssert{};



template<>

struct StaticAssert<true>

{

	static void assert() {}
};

// @Bogus
void compile_time_assertions(){
	StaticAssert<sizeof(CVector)==12>::assert();
	StaticAssert<sizeof(CFriction)==3>::assert();
	//StaticAssert<sizeof(CBit) == 0x38>::assert();

	//StaticAssert<sizeof(CMenu)==0x53C>::assert();

	//StaticAssert<sizeof(CExpandingBox)==52>::assert();

	StaticAssert<sizeof(CSVector)==6>::assert();

	StaticAssert<sizeof(SVector)==6>::assert();

	StaticAssert<sizeof(CQuadBit)==0x84>::assert();

	//StaticAssert<sizeof(CMJ)==0x324>::assert();

	StaticAssert<sizeof(MATRIX)==0x20>::assert();

	StaticAssert<sizeof(u32)==4>::assert();
	StaticAssert<sizeof(u16)==2>::assert();
	StaticAssert<sizeof(u8)==1>::assert();

	StaticAssert<sizeof(i32)==4>::assert();
	StaticAssert<sizeof(i16)==2>::assert();
	StaticAssert<sizeof(i8)==1>::assert();
}

// @Bogus
extern "C" EXPORT int run_assertions(void)
{
	puts("[*] Starting validation");



	validate_CItem();
	validate_CVector();
	validate_CSVector();
	validate_CBit();
	validate_CQuadBit();
	validate_CFT4Bit();
	validate_CFlatBit();
	validate_CBody();
	validate_SVector();
	validate_CSuper();
	validate_CBaddy();
	validate_CMJ();
	validate_CSubmariner();
	validate_CVenom();
	validate_CBlackCat();
	validate_CTorch();
	validate_CHostage();
	validate_CScriptOnlyBaddy();
	validate_CCop();
	validate_CCarnage();
	validate_CChopper();
	validate_CDocOc();
	validate_CJonah();
	validate_CLizard();
	validate_CLizMan();
	validate_CMystFoot();
	validate_CMysterio();
	validate_CSoftSpot();
	validate_CPlatform();
	validate_CRhino();
	validate_CScorpion();
	validate_CPunchOb();
	validate_CSimbyDrop();
	validate_CSimby();
	validate_CSimbyBase();
	validate_CSpClone();
	validate_CSuperDocOck();
	validate_CThug();
	validate_CTurret();
	validate_MATRIX();
	validate_SMatrix();
	validate_SJoint();
	validate_CRudeWordHitterSpidey();
	validate_CBulletFrag();
	validate_CImpactWeb();
	validate_CDomePiece();
	validate_CDome();
	validate_CDomeRing();
	validate_CWeb();
	validate_CSwinger();
	validate_CTurretBase();
	validate_CDummy();
	validate_CDropDownController();
	validate_CSniperSplat();
	validate_SStateFlags();
	validate_CGPolyLine();
	validate_CKnottedWeb();
	validate_SKnottedWebSeg();
	validate_CCamera();
	validate_CQuat();
	validate_SBlockHeader();
	validate_SHandle();
	validate_CItemFrag();
	validate_SLineInfo();
	validate_STexWibItemInfo();
	validate_CPlayer();
	validate_CSmokeTrail();
	validate_CMessage();
	validate_CTrapWebEffect();
	validate_CWebFrag();
	validate_CMenu();
	validate_SEntry();
	validate_CBullet();
	validate_SLinkInfo();
	validate_CElectrify();
	validate_CSimbySlimeBase();
	validate_CMysterioLaser();
	validate_Font();
	validate_CTurretLaser();
	validate_CLaserFence();
	validate_CGoldFish();
	validate_CPowerUp();
	validate_CSwitch();
	validate_CChain();
	validate_CGLine();
	validate_SlicedImage2();
	validate_Image();
	validate_SControl();
	validate_Bitmap256();
	validate_SPCTexture();
	validate_CPolyLine();
	validate_CSonicBubble();
	validate_CGlow();
	validate_CLinked2EndedBit();
	validate_CRibbonBit();
	validate_CSniperTarget();
	validate_CVenomWrap();
	validate_CSmokeJet();
	validate_CTexturedRibbon();
	validate_CDomeShockWave();
	validate_CMysterioHeadCircle();
	validate_SAnimFrame();
	validate_CFadePalettes();
	validate_CSimpleTexturedRibbon();
	validate_CManipOb();
	validate_SimpleMessage();
	validate_CShellMysterioHeadGlow();
	validate_CWobblyGlow();
	validate_CSimpleAnim();
	validate_CCopPing();
	validate_SHook();
	validate_Spidey_CIcon();
	validate_CShellPreviewIcon();
	validate_CEmber();
	validate_CThugPing();
	validate_CAIProc();
	validate_CAIProc_LookAt();
	validate_Texture();
	validate_CRhinoNasalSteam();
	validate_CAIProc_RotY();
	validate_CAIProc_Fall();
	validate_CAIProc_StateSwitchSendMessage();
	validate_CAIProc_MonitorAttack();
	validate_CAIProc_AccZ();
	validate_SMoveToInfo();
	validate_CAIProc_MoveTo();
	validate_CNonRenderedBit();
	validate_SPSXRegion();
	validate_CSimbyShot();
	validate_CVenomElectrified();
	validate_CCarnageElectrified();
	validate_CConstantLaser();
	validate_CShellSymBurn();
	validate_CExpandingBox();
	validate_CL1A3Bomb();
	validate_CMotionBlur();
	validate_SHitInfo();
	validate_SCommandPoint();
	validate_PendingListEntry();
	validate_CSpecialDisplay();
	validate_CSkidMark();
	validate_TextureEntry();
	validate_CShellVenomElectrified();
	validate_CSkinGoo();
	validate_SSkinGooSource();
	validate_SSkinGooSource2();
	validate_SSkinGooParams();
	validate_CShellCarnageElectrified();
	validate_CShellSuperDocOckElectrified();
	validate_CShellRhinoNasalSteam();
	validate_CShellEmber();
	validate_CShellSimbyMeltSplat();
	validate_CShellSimbyFireDeath();
	validate_CShellGoldFish();
	validate_CShellMysterioHeadCircle();
	validate_SpideyIconRelated();
	validate_CGlowFlash();
	validate_SChainData();
	validate_CSearchlight();
	validate_SFlatBitVelocity();
	validate_CMachineGunBullet();
	validate_CChopperMissile();
	validate_CChunkControl();
	validate_SChunkEntry();
	validate_CGouraudRibbon();
	validate_CCopBulletTracer();
	validate_CCombatImpactRing();
	validate_SCamera();
	validate_SRibbonPoint();
	validate_CRhinoWallImpact();
	validate_CFootprint();
	validate_CChunkSmoke();
	validate_CBouncingRock();
	validate_CFlameExplosion();
	validate_CFrag();
	validate_CPixel();
	validate_CFireySpark();
	validate_CSimbyDroplet();
	validate_CSymBurn();
	validate_CBackground();
	validate_CAngrySpark();
	validate_CBitServer();
	validate_CCarnageHitSpark();
	validate_CChunkBit();
	validate_CShatterBit();
	validate_CTextBox();
	validate_CCopLaserPing();
	validate_CDamagedSoftSpotEffect();
	validate_CElectro();
	validate_CElectroLine();
	validate_CVertexWobble();
	validate_CFireyExplosion();
	validate_CFlamingImpactWeb();
	validate_CTripWire();
	validate_CSmokeRing();
	validate_CTexturedRibbon();
	validate_SLineSeg();
	validate_CWibbly();
	validate_SSmokeRingRelated();
	validate_Sprite2();
	validate_SBitServerEntry();
	validate_PKR_FILEINFO();
	validate_PKR_FOOTER();
	validate_PKR_DIRINFO();
	validate_LIBPKR_HANDLE();
	validate_NODE_DIRINFO();
	validate_PVRHeader();
	validate_ClutPC();
	validate_PKR_HEADER();
	validate_SGDOpenFile();
	validate_NODE_FILEINFO();
	validate_SSFXBank();
	validate_SMapping();
	validate_SActionMap();
	validate_SSaveGame();
	validate_MEMORY_ALLOC();
	validate_SMessageProg();
	validate_SLevel();
	validate_SMessage();
	validate_DXsound();
	validate_DXContext();
	validate_DXContextEntry();
	validate_SVideoMode();
	validate_DXVideoModeContext();
	validate_DxZBufferContext();
	validate_DXPOLY();
	validate_SFontEntry();
	validate_SDataGlyph();
	validate_POLY_FT4();
	validate_POLY_GT4();
	validate_SPack();
	validate_tag_S_Pal();
	validate_SViewport();
	validate_SDoubleBuffer();
	validate_SDXPolyField();
	validate_SPCTexPixelFormat();
	validate_SPCTexContainer();
	validate_SAccess();
	validate_AnimPacket();
	validate_SCalcBuffer();
	validate_SCheat();
	validate_SButton();
	validate_DDPIXELFORMAT();
	validate_ConvertPSXPaletteToPC();
	validate_BmpHeader();
	validate_Load8BitBMP2();
	validate_CWibbling3DExplosion();
	validate_C3DExplosion();
	validate_CGrenadeWave();
	validate_CGrenadeExplosion();
	validate_CRipple();
	validate_SSection();
	validate_SFringeQuad();
	validate_SModel();
	validate_SMessageData();
	validate_SSfxEntry();
	validate_reloc_mod();
	validate_SReloc();
	validate_SRelocEntry();
	validate_SMovieDetails();
	validate_BINKSUMMARY();
	validate_BINK();
	validate_matrix4x4();
	validate_vector3d();
	validate_vector4d();
	validate_SScore();
	validate_SRecords();
	validate_STrainingMission();
	validate_SRecordRelated();
	validate_CRecordBox();
	validate_SDCCardTime();
	validate_SCardHead();
	validate_SBackupFile();
	validate_SSaveFile();
	validate_SDCCardFullTime();
	validate_SPdPadBig();
	validate_SPdPadSmall();
	validate_tagSVRAMRect();
	validate_SZone();
	validate_DCSkaterModel();
	validate_DCMaterial();
	validate_DCObject();
	validate_DCStrip();
	validate_DCObjectList();
	validate_DCKeyFrame();
	validate_DCModelData();
	validate_PREManager();
	validate_CSonicRipple();
	validate_Vector();
	validate_SRhinoData();
	validate_SLight();
	validate_CManipObChunk();
	validate_DB_RECT();
	validate_DR_ENV();
	validate_DRAWENV();
	validate_DISPENV();
	validate_SSfxRelated();
	validate_SSfxAsset();
	validate_CRibbon();
	validate_CGlassBit();
	validate_CSmokeGenerator();
	validate_SDXSoundHolder();
	validate_SDxSomething();
	validate_DSBUFFERDESC();
	validate_TwiddleStuff();
	validate_CSmokePuff();
	validate_SRibbonTexture();
	validate_SSimpleRibbonParams();
	validate_CSpark();
	validate_SIndicator();
	validate_POLY_F3();
	validate_CVenomHitSpark();
	validate_SPushOffset();
	validate_SLink();

	puts("[*] Validation done!");

    return FAIL_VALIDATION;
}

// @Bogus
void runtime_assertions()
{
	int result = run_assertions();

	while(result)
		;
}

// @Bogus
void *my_malloc(size_t s)
{
	void* res = malloc(s);

	return res;
}

// @Bogus
void my_free(void* block)
{
	free(block);
}

// @Bogus
int my_atexit(
   void (MY_CDECL *func )( void )
)
{
	return atexit(func);
}

// @Bogus
void *my_realloc(void *m, size_t s)
{
	void* res = realloc(m, s);

	return res;
}

#ifdef _WIN32
// @Bogus
_onexit_t my_onexit(
   _onexit_t function
)
{
	return _onexit(function);
}
#endif

// @Bogus
void *my_new(size_t s)
{
	void* res = ::operator new(s);
	return res;
}

// @Bogus
void *my_calloc(size_t a, size_t b)
{
	return calloc(a, b);
}

// @Bogus
void patch_alloc(void)
{
	PATCH_PUSH_RET(0x0052A227, my_malloc);
	PATCH_PUSH_RET(0x0052A3C0, my_free);
	PATCH_PUSH_RET(0x00529C39, my_atexit);

	PATCH_PUSH_RET(0x0052F250, my_realloc);
#ifdef _WIN32
	PATCH_PUSH_RET(0x00529BBB, my_onexit);
#endif
	PATCH_PUSH_RET(0x00529BA2, my_new);

	PATCH_PUSH_RET(0x0052C044, my_calloc);
}

// @Bogus
static int my_video_player(const char*, i32)
{
	return 1;
}

// @Bogus
void game_patches(void)
{
	//PATCH_CALL(0x004707BE, my_video_player);

#ifdef SPIDEY_NO_CD_CHECK
	// dev-only: skip the CD-ROM disc check (WinMain check and the periodic
	// recheck both call this helper) so the game runs without a real
	// mixed-mode disc under Wine. Never merge this into an upstream PR,
	// stays on our fork's main branch only, off unless SPIDEY_NO_CD_CHECK
	// is defined at build time.
	*(unsigned char*)0x005163E0 = 0x32; // xor al,al
	*(unsigned char*)0x005163E1 = 0xC0;
	*(unsigned char*)0x005163E2 = 0xC3; // ret
#endif

	patch_alloc();

	patch_mem();
	patch_utils();
	patch_ps2funcs();

	patch_pkr();
	patch_pcdcMem();
	patch_pack();
	patch_vram();

	patch_CItem();
	patch_CBody();

	patch_spool();
	patch_trig();
	patch_pctex();
	patch_dcfileio();
	patch_PCMovie();

	patch_flash();
	patch_pshell();
	patch_FontTools();
	patch_mess();
	patch_m3dcolij();
	patch_CSuper();
	patch_ps2m3d();
	patch_m3dutils();
	patch_CBit();
	patch_CFT4Bit();
}

// @Bogus
void runtime_patches(void)
{
#ifdef _WIN32
	LPVOID text_start = (void*)0x00401000;

	SIZE_T text_size = 0x0053B000 - (int)text_start;

	DWORD text_protect;
	VirtualProtect(text_start, text_size, PAGE_EXECUTE_READWRITE, &text_protect);

	game_patches();

	DWORD t;
	VirtualProtect(text_start, text_size, text_protect, &t);
#endif
}

#include "runtime_version.h"

#ifndef RUNTIME_VERSION
#define RUNTIME_VERSION "LOCAL"

#endif

#ifndef _WIN32

int main()
{
	compile_time_assertions();
	return run_assertions();
}


#else

HMODULE bink_dll;

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved ) 
{
	compile_time_assertions();
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:

			if(GetModuleHandle("tobey_validator.exe") != NULL)
			{
				puts("In validator");
				break;
			}

			AllocConsole();
			SetConsoleTitle("spidey-decomp - " RUNTIME_VERSION);
			freopen("CONOUT$", "w", stdout);

			bink_dll = GetModuleHandleA("binkw32.dll");


			puts("spidey-decomp starting " RUNTIME_VERSION);

			runtime_assertions();
			runtime_patches();

            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
#endif

// @Bogus
void DoAssert(u8 cond, const char* str, ...)
{
	if (!cond)
		puts(str);
}
