#include "non_win32.h"
#ifdef SPIDEY_STANDALONE
#include "platform/plat.h"
#endif

// #define BOOT_GAME
#define MODEL_PREVIEW

// dev-only: skip the CD-ROM disc check so the game runs under Wine without
// a real mixed-mode disc. Off by default. Never uncomment this on a branch
// meant for an upstream PR.
// #define SPIDEY_NO_CD_CHECK

// the standalone build always runs the real game path
#ifdef SPIDEY_STANDALONE
#undef MODEL_PREVIEW
#undef BOOT_GAME
#endif

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
#include "rfront.h"


#include "my_patch.h"

extern int FAIL_VALIDATION;

const i32 POLYBUFFERSIZE = 0x17000;

// 0x4000 dwords, not 0x1000: SpideyMain's rep stosd at 0x455CC5 clears
// 0x4000 of them. Real address 0x5FCE04.
#ifndef SPIDEY_STANDALONE
EXPORT i32 gMainStuff[0x4000];
#else
extern i32 gMainStuff[0x4000];
#endif

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
// 0 EnviroList, 1 G_ENVIRONMENTAL_OBJECT_LIST, 2 the player, 3 the player's extra
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

// forced start level, -1 = none. gRenderTest bit 2 and this together decide
// whether SpideyMain shows the shell at all. Not in idb_globals.txt; the next
// named address is 0x568FB4 WindowName, so this is a standalone dword.
static i32 * const gStartLevelIndex = reinterpret_cast<i32*>(0x00568FB0);

// checked once, right after the shell returns: nonzero means quit the game.
static u8 * const gQuitAfterShell = reinterpret_cast<u8*>(0x0060CF88);

// the debug level cycle: gDebugLevelIndex is a dword but only its low byte is
// ever touched, so it is typed u8 here. gDebugLevelNames is the name pointer
// of record 0 of a table of
// 192 byte records. MSVC folded the field offset into the base address, so
// 0x60CF84 is "record 0's name", not necessarily the start of the table
// (gLevelStatus at 0x60CFA4 sits inside what would be record 0).
static u8 * const gDebugLevelIndex = reinterpret_cast<u8*>(0x0060CFA0);
static char ** const gDebugLevelNames = reinterpret_cast<char**>(0x0060CF84);

// zeroed once per pass of the level loop, right after PlayAway returns. Sits
// immediately after Pad_IdleTime (0x66129C, pcdcPad.cpp) and before gPadInited
// (0x6612AC), so it reads like a second idle counter. Tentative name.
static i32 * const gLevelIdleTime = reinterpret_cast<i32*>(0x006612A0);

// idb_globals.txt: gBitServer. Only used here, and only to delete it through
// its vtable at shutdown, so all we know about the type is that it has a
// virtual destructor. Modelled as CClass, which is what every deletable game
// object in this repo derives from.
static CClass ** const gBitServer = reinterpret_cast<CClass**>(0x0056EB50);

// nullsub_1 (0x4015B0) is two folded empty bodies, print_if_false and trigLog.
// One argument at the call site means trigLog. It is not declared in trig.h,
// so declare it here the way other files declare MechList.
extern void trigLog(const char *fmt, ...);


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
#ifdef SPIDEY_STANDALONE
	// Translated from the disassembly at 0x460FC0 (416 bytes) for the
	// standalone build. The suspend branch is CBody::Suspend inlined
	// (DeleteStuff, mppOriginalList, DeleteFrom, AttachTo, flag), the two
	// asserts are the same strings CBody::Suspend uses. a2 is unused there.
	(void)a2;
	CBody* pItem = *ppList;
	while (pItem)
	{
		CBody* pNext = static_cast<CBody*>(pItem->mNextItem);

		print_if_false((pItem->mCBodyFlags & CBODY_SUSPENDED) == 0, "Suspended flag illegally set");

		if (pItem->mCBodyFlags & 0x40)
		{
			// marked for deletion: one frame of grace, then delete
			if (pItem->mCBodyFlags & 0x80)
				delete pItem;
			else
				pItem->mCBodyFlags |= 0x80;
		}
		else
		{
			pItem->mPlayerDist = Utils_CrapDist(G_MECHLIST->mPos, pItem->mPos);

			if ((pItem->mCBodyFlags & 2) && pItem->mPlayerDist > SuspendedDistance)
			{
				pItem->Suspend(ppList);
			}
			else
			{
				if (pItem->mFlags & 2)
				{
					pItem->EveryFrame();
					static_cast<CSuper*>(pItem)->UpdateFrame();
					pItem->AI();
				}
				else
				{
					pItem->EveryFrame();
					pItem->AI();
				}

				if ((pItem->mCBodyFlags & 8) || pItem->mpShadow)
					pItem->UpdateShadow();
			}
		}

		pItem = pNext;
	}
#else
	typedef void (*func_ptr)(CBody**, i32);
	func_ptr func = (func_ptr)0x00460FC0;
	func(ppList, a2);
#endif
}

// Ob_MaybeUnSuspendOrCull: the other direction, walks the suspended list and
// puts anything that came back inside SuspendedDistance on its home list again.
// @Bogus
static void Ob_MaybeUnSuspendOrCull(void)
{
#ifdef SPIDEY_STANDALONE
	// From the disassembly at 0x461160 (158 bytes): CBody::UnSuspend inlined.
	CBody* pItem = G_SUSPENEDED_LIST;
	while (pItem)
	{
		CBody* pNext = static_cast<CBody*>(pItem->mNextItem);

		if ((pItem->mCBodyFlags & 2)
				&& Utils_CrapDist(G_MECHLIST->mPos, pItem->mPos) <= SuspendedDistance)
		{
			pItem->UnSuspend();
		}

		pItem = pNext;
	}
#else
	typedef void (*func_ptr)(void);
	func_ptr func = (func_ptr)0x00461160;
	func();
#endif
}

// The periodic CD recheck, 0x515D80, in SpideyDX.cpp's address range. It runs
// the disc test at 0x5163E0 behind an anti-tamper junk loop and, if the disc is
// gone, posts WM_CLOSE and exits the process. Deliberately not decompiled: the
// repo has a SPIDEY_NO_CD_CHECK dev toggle that patches 0x5163E0, and writing
// the check out here would fight it. Forwarded so SpideyMain's call site is
// honest.
// @Bogus
static void gsub_515D80(void)
{
#ifndef SPIDEY_STANDALONE
	typedef void (*func_ptr)(void);
	func_ptr func = (func_ptr)0x00515D80;
	func();
#endif
}


// @Ok
// @Matching
void CalcPolyBufferEnd(void)
{
	G_POLY_BUFFER_END = reinterpret_cast<u8*>(
			(reinterpret_cast<u32>(G_PDOUBLE_BUFFER->Polys) + POLYBUFFERSIZE - 0x100) & 0x7FFFFFFF);
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
		Ob_AI(reinterpret_cast<CBody**>(&G_MECHLIST_PLAYER), 0);
		Trig_ResetCPExecutedFlags();

#ifdef SPIDEY_STANDALONE
		// SPIDEY_TRACE_PLAYER=1: one line per frame with the player's state,
		// for comparing against the original game (gdb breakpoints distort the
		// wall-clock frame step, this does not).
		{
			static i32 tracePlayer = -1;
			if (tracePlayer < 0)
				tracePlayer = getenv("SPIDEY_TRACE_PLAYER") ? 1 : 0;
			if (tracePlayer && G_MECHLIST)
			{
				CPlayer *pP = reinterpret_cast<CPlayer*>(G_MECHLIST);
				CCamera *pC = G_CAMERA_LIST;
				fprintf(stderr, "PLAYER t=%u status=%d f80=%d pos=(%d,%d,%d) vel=(%d,%d,%d) ang=(%d,%d,%d) coll=%#x mode=%#x anim=%d frame=%d crawl=%d E2D=%d E2E=%d EBC=%d | cam pos=(%d,%d,%d) mode=%d yaw=%d\n",
					Plat_Ticks(), gLevelStatus, pP->field_80, pP->mPos.vx, pP->mPos.vy, pP->mPos.vz,
					pP->mVel.vx, pP->mVel.vy, pP->mVel.vz,
					pP->mAngles.vx, pP->mAngles.vy, pP->mAngles.vz,
					pP->mCollision, pP->field_E1C, pP->mAnim, pP->mFrame, pP->field_AD4,
					pP->field_E2D, pP->field_E2E, pP->field_EBC,
					pC ? pC->mPos.vx >> 12 : 0, pC ? pC->mPos.vy >> 12 : 0, pC ? pC->mPos.vz >> 12 : 0,
					pC ? (i32)pC->mCameraMode : -1, pC ? (i32)pC->field_23A : 0);
			}
		}
#endif

		Ob_AI(&PowerUpList, 0);
		Ob_AI(&BulletList, 0);
		Ob_AI(&MiscList, 0);
		Ob_AI(&G_ENVIRONMENTAL_OBJECT_LIST, 0);
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
		G_MECHLIST_PLAYER->CutSceneSkipCleanup();

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
		G_CAMERA_LIST->SetMode(CAMERAMODE_ITSYLOOKDOWN);
	}
	else if (G_CAMERA_LIST && G_CAMERA_LIST->mCameraMode == CAMERAMODE_ITSYLOOKDOWN)
	{
		G_CAMERA_LIST->SetMode(CAMERAMODE_DEMO);
	}
}

// @Ok
// 0x004555A0, 1072 bytes. The render half of the frame, called by PlayAway
// right after Logic. The Mac build keeps CountPrimitives(void) as its own
// function in main.cpp (tools/prototypes.json lists it at 184 bytes); the PC
// build inlined it, so it is the gFrontDrawPolyFlag block near the bottom here
// and has no separate body.
void Display(void)
{
	if (*gDebugBanner)
	{
		Mess_SetRGB(255, 0, 0, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(0);
		Mess_DrawText(256, 60, *gDebugBanner, 0, 0x1000);
	}

	if (!(gRenderTest & 0x400) || (gRenderTest & 0x200))
		Ob_AI(reinterpret_cast<CBody**>(&G_CAMERA_LIST), 0);

	// no null check on CameraList, same as the original
	G_VIEWPORT.Zoom = static_cast<u16>(G_CAMERA_LIST->GetZoom());

	Screen_UpdateFades();
	Panel_Display();

	M3d_RenderSetup(G_MIKE_CAMERA, &G_VIEWPORT, G_PDOUBLE_BUFFER->OrderingTable);

	if (gRenderListFlags[9])
		M3d_RenderBackground(BackgroundList);

	if (g3DExplosions)
		M3d_PreprocessWibblyTextures(*gGrenadeExplosionRegion);

	if (*gFireRingObject)
	{
		if ((*gFireRingObject)->field_10C.pWhatever)
		{
			M3d_PreprocessWibblyTextures(*gFireRingRegion);
			M3d_PreprocessPulsingColours(*gFireRingRegion);
		}
		else
		{
			M3d_PreprocessWibblyTextures(*gPsxRingIndex);
		}
	}

	if (*gSymBurnCount)
	{
		M3d_PreprocessWibblyTextures(*gSymBurnRegion);
		M3d_PreprocessPulsingColours(*gSymBurnRegion);
	}

	if (gFireDomes)
	{
		M3d_PreprocessWibblyTextures(*gFireDomeRegion);
		M3d_PreprocessPulsingColours(*gFireDomeRegion);
	}

	Screen_DrawTarget();
	Screen_DrawArrow();
	Mess_Display();

	if (*gTrainingActive)
		PShell_EndTrainingDisplay();

	if (gRenderListFlags[0])
		M3d_Render(EnviroList);

	if (gRenderListFlags[1])
		M3d_Render(G_ENVIRONMENTAL_OBJECT_LIST);

	Music_MusicUpdate();

	if (gRenderListFlags[2])
		M3d_Render(G_MECHLIST_PLAYER);

	if (*gM3dSuperScaleEnabled)
		*gDCUseFixedScale = 1;

	if (gRenderListFlags[3])
		M3d_Render(SpideyAdditionalBodyPartsList);

	*gDCUseFixedScale = 0;

	G_MECHLIST_PLAYER->RenderLookaroundReticle();

	if (gRenderListFlags[4])
		M3d_Render(MiscellaneousRenderingList);

	if (gRenderListFlags[6])
		M3d_Render(BaddyList);

	// The two loops below go through CBody vtable slot 5 (offset 0x14) and
	// CBaddy vtable slot 17 (offset 0x44) in the original. Slot 5 is modelled
	// in the repo (CSearchlight::SpecialRenderer, CSniperTarget and
	// CChopperMissile::DrawTargetRecticle are all virtual there), slot 17 is
	// not: the repo's CBaddy declares 12 virtuals and the original has at
	// least 16. Both slots are dispatched on mType here instead, which reaches
	// the same three or four classes because the type ids are unique.
	for (CBody *pControl = ControlBaddyList; pControl != 0;
			pControl = reinterpret_cast<CBody*>(pControl->mNextItem))
	{
		u16 type = pControl->mType;

		if (type == 322)
			static_cast<CSearchlight*>(pControl)->SpecialRenderer();
		else if (type == 323)
			static_cast<CSniperTarget*>(pControl)->DrawTargetRecticle();
	}

	for (CBody *pBaddy = reinterpret_cast<CBody*>(BaddyList); pBaddy != 0;
			pBaddy = reinterpret_cast<CBody*>(pBaddy->mNextItem))
	{
		u16 type = pBaddy->mType;

		if (type == 321)
		{
			static_cast<CChopperMissile*>(pBaddy)->DrawTargetRecticle();
			continue;
		}

		if (type == 310)
		{
			// scorpion fight housekeeping, folded into the render loop by the
			// original: forget every object the camera is holding on to except
			// the one the player is actually carrying.
			CCamera *pCamera = G_CAMERA_LIST;

			if (pCamera)
			{
				if (pCamera->mCameraMode == CAMERAMODE_USER)
				{
					pCamera->field_F9 = 1;
				}
				else
				{
					pCamera->field_F9 = 0;

					CPlayer *pPlayer = G_MECHLIST_PLAYER;

					for (u32 i = 0; i < 8; i++)
					{
						CItem *pHeld = pCamera->field_184[i];

						if (pHeld == 0)
							break;

						if (pPlayer == 0
								|| reinterpret_cast<CItem*>(pPlayer->mHeldObject) != pHeld)
						{
							pHeld->mFlags &= ~0x800;
							pCamera->field_184[i] = 0;
							pPlayer = G_MECHLIST_PLAYER;
						}
					}
				}
			}
		}
		else if (type != 308 && type != 309)
		{
			continue;
		}

		if (type == 308)
			static_cast<CDocOc*>(pBaddy)->RenderClaws();
		else if (type == 309)
			static_cast<CSuperDocOck*>(pBaddy)->RenderClaws();
		else
			static_cast<CScorpion*>(pBaddy)->TailRenderer();
	}

	if (gRenderListFlags[8])
		M3d_Render(PowerUpList);

	if (gRenderListFlags[5])
		M3d_Render(MiscList);

	if (gRenderListFlags[7])
		M3d_Render(BulletList);

	Post_PostProcessEffects();
	M3d_RenderCleanup();
	Music_MusicUpdate();

	// the Mac build's CountPrimitives: walk the ordering table from the last
	// entry and count how many links carried a primitive this frame.
	if (*gFrontDrawPolyFlag)
	{
		*gOtPrimitiveCount = 0;

		u32 *pEntry = reinterpret_cast<u32*>(G_PDOUBLE_BUFFER->OrderingTable[4095]);

		while (pEntry != reinterpret_cast<u32*>(0xFFFFFF))
		{
			if (*pEntry & 0xFF000000)
				(*gOtPrimitiveCount)++;

			pEntry = reinterpret_cast<u32*>(*pEntry & 0xFFFFFF);

			// always true, the pointer was just masked. Dead assert, kept.
			print_if_false((reinterpret_cast<u32>(pEntry) & 0xFF000000) == 0, "eh?");
		}
	}

	Bit_Display();

	if (G_POST_WATER_EFFECT)
		Post_DoPauseDisplayListProcessing();

	Front_Display();
	Flash_Display();

	*gPolyBufferUsed = static_cast<i32>(
			(reinterpret_cast<u32>(G_PPOLY) & 0x7FFFFFFF)
			- (reinterpret_cast<u32>(G_PDOUBLE_BUFFER->Polys) & 0x7FFFFFFF));
}

// @Ok
// 0x004559D0, 704 bytes. One level, start to finish: it sets the frame loop
// up, runs Logic and Display once per frame until something writes an end code
// into gLevelStatus, then tears the audio down. SpideyMain's inner loop is
// really "call this, then act on gLevelStatus".
void PlayAway(void)
{
	FontManager::ResetCharMaps();

	*gFrontUseAltTriggerMask = 1;

	Bruce_Sync();
	PCGfx_BeginScene(3, -1);

	G_VBLANKS = 0;
	TTime = 0;

	Pad_ClearAll();

	gWaterEffect = 0;
	*gWaterEffectTwo = 0;
	gAttackRelated = 0;
	*gPlayAwayCounter = 0;

	if (*gShellMenuAbort)
	{
		// storyboard and comic viewers: no frame loop at all, just repaint the
		// sky and report end code 11, which sends SpideyMain back to the shell.
		M3d_FadeColour = 0xFFFFFF;
		M3dInit_SetFoggingParams(0, 6000, 2048);

		G_DB_SKY_COLOR = 0;
		G_BFOGGING_RELATED = 1;
		Db_UpdateSky();

		gLevelStatus = 11;
		return;
	}

	gLevelStatus = 0;

	if (*gDoShellForceLevelExit)
		Utils_InitialRand(0x12A6CC58);

	Pause(1);
	Screen_StartCircularFadeIn(32, 8);

	while (gLevelStatus == 0)
	{
		u32 frameStart = G_VBLANKS;

		Db_FlipClear();
		CalcPolyBufferEnd();

		Logic();

		if (gLevelStatus)
			break;

		Music_MusicUpdate();
		Display();
		gsub_430880();

		if (*gShowFrameRate)
		{
			char fps[8];
			strcpy(fps, "XX FPS");

			i32 rate = 60 / G_MECHLIST_PLAYER->field_80;

			if (rate <= 9)
			{
				fps[0] = '0';
				fps[1] = static_cast<char>(rate + '0');
			}
			else
			{
				fps[0] = static_cast<char>(rate / 10 + '0');
				fps[1] = static_cast<char>(rate % 10 + '0');
			}

			Mess_SetScale(192);
			Mess_SetTextJustify(1);
			Mess_DrawText(90, 104, fps, 0, 0x1000);
		}

		// if no vblank happened while the frame was being built, burn one so
		// the game never runs faster than 60Hz.
		if (G_VBLANKS == frameStart)
			Pause(1);

		DoVblankProcessing = 0;

		DrawSync();

		print_if_false(
				(reinterpret_cast<u32>(G_PPOLY) & 0x7FFFFFFF)
					<= (reinterpret_cast<u32>(G_POLY_BUFFER_END) & 0x7FFFFFFF),
				"Undetected Poly Buffer Overflow");

		gsub_430680();

		if (!DoVblankProcessing)
		{
			Utils_VblankProcessing();
			DoVblankProcessing = 1;
		}

		PCGfx_EndScene(1);

		if (G_MECHLIST_PLAYER->IsDead())
		{
			gLevelStatus = 2;
			break;
		}
	}

	G_DB_SKY_COLOR = 0;
	Db_UpdateSky();

	if (G_SCENE_RELATED)
		PCGfx_EndScene(1);

	Spool_Sync();

	*gOtPushback = 1;

	if (gLevelStatus == 3)
		Front_SaveGameState();

	SFX_StopAll();
	Redbook_XAStop();
}

// @Ok
// 0x00455C90, 4555 bytes, 434 instructions. WinMain is its only caller. The
// top level state machine: boot, then a shell/level loop driven by the end
// code PlayAway leaves in gLevelStatus, then shutdown. It normally never
// returns while the game is running.
//
// Three loop heads, which is why this uses gotos: the outer one (the shell and
// the display mode), the level entry one (the CD recheck and Front_LoadGame)
// and the inner one (PlayAway plus the end code switch). The original is built
// the same way, with the same three jump targets.
//
// Two things in here are still reached by raw byte offset instead of by name:
// gSaveGame's fields at 0x48, 0x4C, 0x50 and 0x79 are inside PADDING runs in
// shell.h (trig.cpp already does the same thing for other fields of it), and
// the debug level cycle table at 0x60CF84 has no struct. Both need a shell.h /
// trig side change to fix properly.
//
// The stub this replaces only covered the boot slice and the model preview
// branch. The MODEL_PREVIEW toggle is kept exactly as it was.
void SpideyMain(void)
{
	trigLog("xxx main\n");

	for (i32 i = 0; i < 0x4000; i++)
	{
		gMainStuff[i] = 0x4B415453;
	}

	gMainStuff[0] = 0x544C4148;

	// the two flags the shell reads back through its parameter block. Case 3
	// (finished the last area) sets the second, case 10 sets the first, and
	// both are cleared as they are handed over.
	u32 shellParamOne = 0;
	u32 shellParamTwo = 0;

	u32 shellParams[2];
	u8 *pSaveBytes;
	i32 levelIndex;
	SLevel *pLevel;
	CPlayer *pPlayer;
	char *pNextLevelName;
	i32 endCode;

	Init_AtStart(1);
	PCTex_LoadPcIcons();
	GameFMV_PlayMovie(0, 1, 1, 2.5f);
	GameFMV_PlayMovie(1, 1, 1, 1.0f);
	GameFMV_PlayMovie(2, 1, 1, 1.0f);
	GameFMV_PlayMovie(3, 1, 1, 1.0f);

	Init_Cleanup(0);
	gRunCinemaRelated = 0;

	while (G_GAME_FADE)
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
		goto shutdown;
	}

	Mess_LoadFont("font_big.fnt", -1, -1, -1);
	Mess_LoadFont("sp_fnt02.fnt", -1, -1, -1);
	Mess_LoadFont("sp_fnt03.fnt", -1, -1, -1);

	PShell_NormalFont();

outerLoop:
	M3d_FadeColour = 0xFFFFFF;
	M3dInit_SetFoggingParams(0, 6000, 2048);

	if (!(gRenderTest & 4) && *gStartLevelIndex == -1)
	{
		// normal path: hand control to the shell module and take the level
		// name it leaves in gSaveGame
		((void(*)(i32))gsub_430880)(0);

		Reloc_Load("shell", 1);

		shellParams[0] = shellParamOne;
		shellParamOne = 0;
		shellParams[1] = shellParamTwo;
		shellParamTwo = 0;

		Reloc_CallUserFunction("shell", 0, shellParams, 0);
		Reloc_Unload("shell");

		// the "what if" version of level 1 area 2
		if (Utils_CompareStrings("l1a2_t", G_SAVE_GAME.field_4) && gWhatIf)
			Utils_CopyString("l1a2a_t", G_SAVE_GAME.field_4, 9);

		if (*gQuitAfterShell)
		{
			Init_Cleanup(3);
			goto shutdown;
		}
	}
	else if (*gStartLevelIndex >= 0)
	{
		// a level was forced from the command line: take its name straight out
		// of the level table and throw the restart point away
		Utils_CopyString(Levels[*gStartLevelIndex].mName, G_SAVE_GAME.field_4, 9);

		pSaveBytes = reinterpret_cast<u8*>(&G_SAVE_GAME);

		G_SAVE_GAME.mRestartPointName[0] = 0;
		*reinterpret_cast<i32*>(pSaveBytes + 0x50) = 0;
		pSaveBytes[0x79] = 0;
		*reinterpret_cast<i32*>(pSaveBytes + 0x48) = 0;
		*reinterpret_cast<i32*>(pSaveBytes + 0x4C) = 0;
	}

	DXINIT_SetDisplayOptions(gPendingResolutionX, gPendingResolutionY,
			gPendingColorDepth, gLowGraphics, gBrightnessRelated);

	PCINPUT_SetMouseBounds(0, 0, gPendingResolutionX - 32, gPendingResolutionY - 32);
	PCINPUT_SetMousePosition((gPendingResolutionX - 32) >> 1,
			(gPendingResolutionY - 32) >> 1);

levelEntry:
	gsub_515D80();
	((void(*)(i32))gsub_430880)(Trig_GetLevelId());
	Front_LoadGame(&G_SAVE_GAME, 0, false);

innerLoop:
	PlayAway();
	PCGfx_EndScene(1);

	endCode = gLevelStatus;
	*gLevelIdleTime = 0;

	if (endCode != 2 && endCode != 9)
		gRunCinemaRelated = 0;

	if (endCode != 0)
		*gFrontUseAltTriggerMask = 0;

	switch (endCode)
	{
	case 1:
		// restart the same level from the top
		Init_Cleanup(0);
		gSpideyMainRelated = 5;
		goto levelEntry;

	case 2:
	case 9:
		// the player died or asked to leave: offer retry or quit
		gSpideyMainRelated = 20;
		Init_Cleanup(0);
		Screen_SepiaFade();

		G_GAME_FADE = 10;

		while (G_GAME_FADE)
			;

		if (!Front_ContinueExit())
			goto quitTail;

		((void(*)(i32))gsub_430880)(Trig_GetLevelId());
		Front_LoadGame(&G_SAVE_GAME, 1, false);
		goto innerLoop;

	case 3:
		// the level was finished
		Init_Cleanup(0);
		Screen_SepiaFade();

		if (Utils_CompareStrings(G_SAVE_GAME.field_4, "l8a7_t"))
		{
			// the last area of level 8 counts as l8a6_t. No -1 check on the
			// index here, only the assert, same as the original.
			levelIndex = Front_GetLevelIndex("l8a6_t");

			print_if_false(levelIndex != -1, "Could not find l8a6_t ???");

			if (G_SAVE_GAME.field_56[levelIndex] < 0xFF)
				G_SAVE_GAME.field_56[levelIndex]++;

			PShell_MaybeUnlockStuff();

			shellParamTwo = 1;
			goto outerLoop;
		}

		levelIndex = Front_GetLevelIndex(G_SAVE_GAME.field_4);

		if (levelIndex != -1)
		{
			levelIndex--;

			print_if_false(levelIndex >= 0 && levelIndex < 34, "Bad LevelIndex");

			if (G_SAVE_GAME.field_56[levelIndex] < 0xFF)
				G_SAVE_GAME.field_56[levelIndex]++;
		}

		PShell_MaybeUnlockStuff();

		while (G_GAME_FADE)
			;

		if (!(gRenderTest & 0x80))
		{
			// autosave on the easy difficulties, or when the level itself asks
			// for it through SLevel::field_8 bit 1
			if (G_DIFFICULTY_LEVEL == 0)
			{
				PShell_MaybeSaveGame();
			}
			else
			{
				pLevel = Front_FindLevel(G_SAVE_GAME.field_4);

				if (pLevel && (pLevel->field_8 & 2))
					PShell_MaybeSaveGame();
			}
		}

		((void(*)(i32))gsub_430880)(Trig_GetLevelId());
		Front_LoadGame(&G_SAVE_GAME, 0, true);
		goto innerLoop;

	case 4:
	case 5:
		// debug level cycle: step one record on and wrap when the next record
		// has an empty name
		Init_Cleanup(0);

		(*gDebugLevelIndex)++;

		pNextLevelName = *reinterpret_cast<char**>(
				reinterpret_cast<u8*>(gDebugLevelNames) + *gDebugLevelIndex * 192);

		if (*pNextLevelName == 0)
			*gDebugLevelIndex = 0;

		goto levelEntry;

	case 6:
		// enter the level for real
		Init_Cleanup(2);
		Trig_ParseTRGFile();

		pPlayer = new CPlayer();

		Trig_ExecuteRestart();

		// the camera attaches itself to CameraList, nothing keeps this pointer
		new CCamera(pPlayer);

		goto innerLoop;

	case 7:
		gSpideyMainRelated = 20;
		Init_Cleanup(0);
		Screen_SepiaFade();

quitTail:
		// leaving for good if the shell was skipped, otherwise back to it
		if (gRenderTest & 4)
			goto shutdown;

		if (*gStartLevelIndex >= 0)
			goto shutdown;

		goto outerLoop;

	case 8:
		// restart from the beginning of the level, dropping the restart point
		Front_ClearScreen();
		Init_Cleanup(0);

		G_SAVE_GAME.mRestartPointName[0] = 0;

		((void(*)(i32))gsub_430880)(Trig_GetLevelId());
		Front_LoadGame(&G_SAVE_GAME, 0, false);
		goto innerLoop;

	case 10:
		shellParamOne = 1;
		gSpideyMainRelated = 20;
		Init_Cleanup(0);
		Screen_SepiaFade();
		goto outerLoop;

	case 11:
		Init_Cleanup(0);
		goto outerLoop;

	default:
		print_if_false(0, "Unknown EndCode");
		goto shutdown;
	}

shutdown:
	while (G_GAME_FADE)
		Pause(1);

	delete pYesNoMenu;
	delete *gBitServer;

	Mess_UnloadAllFonts();
	PCSHELL_Shutdown();
	PCTex_FreePcIcons();
	Db_DeleteOTsAndPolyBuffers();
	Init_AtEnd();
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
	validate_CKnottedWebSplat();
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

	patch_camera();
	patch_exp();
	patch_chopper();
	patch_carnage();
	patch_effects();
	patch_manipob();
	patch_vector();
	patch_bit();
	patch_bit2();
	patch_spidey();
	patch_baddy();
	patch_thug();
	patch_cop();
	patch_simby();
	patch_rhino();
	patch_scorpion();
	patch_web();
	patch_venom();
	patch_panel();
	patch_screen();
	patch_ai();
	patch_weapons();
	patch_docock();
	patch_superock();
	patch_lizman();
	patch_blackcat();
	patch_jonah();
	patch_hostage();
	patch_spclone();
	patch_switch();
	patch_torch();
	patch_powerup();
	patch_turret();
	patch_wire();
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

// the standalone build has its own main in platform/main_standalone.cpp
#ifndef SPIDEY_STANDALONE
int main()
{
	compile_time_assertions();
	return run_assertions();
}
#endif


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
			PCGfx_CopyGameResolution();
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
