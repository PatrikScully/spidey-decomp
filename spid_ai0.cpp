#include "spid_ai0.h"

#include "bit.h"
#include "camera.h"
#include "db.h"
#include "effects.h"
#include "flash.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "mem.h"
#include "ob.h"
#include "manipob.h"
#include "PCInput.h"
#include "ps2lowsfx.h"
#include "ps2pad.h"
#include "ps2redbook.h"
#include "tweak.h"
#include "ps2funcs.h"
#include "trig.h"
#include "utils.h"
#include "vector.h"
#include "web.h"

// ---------------------------------------------------------------------------
// CPlayer fields this file needs that spidey.h does not name yet. They all sit
// inside existing PADDING runs; spidey.h belongs to another worktree, so they
// are reached by byte offset here and reported for it to adopt. Evidence for
// each one is on the line where it is used.
// ---------------------------------------------------------------------------
#define PLR_U8(p, off)  (*reinterpret_cast<u8*>(reinterpret_cast<char*>(p) + (off)))
#define PLR_I8(p, off)  (*reinterpret_cast<i8*>(reinterpret_cast<char*>(p) + (off)))
#define PLR_I16(p, off) (*reinterpret_cast<i16*>(reinterpret_cast<char*>(p) + (off)))
#define PLR_U16(p, off) (*reinterpret_cast<u16*>(reinterpret_cast<char*>(p) + (off)))
#define PLR_I32(p, off) (*reinterpret_cast<i32*>(reinterpret_cast<char*>(p) + (off)))

// gWaterEffect is the repo global from post.cpp (spidey.cpp reaches it the
// same way). Note trig.cpp instead points a file-local pointer at the game
// address 0x0060FA9C for the same thing; the two readings disagree and should
// be unified once post.h grows a declaration.
extern i32 gWaterEffect;

// init.cpp's global, no header declares it yet.
extern i32 gInitRelatedTwo;

// baddy.cpp's global (real IDB name, 0x0060D9E0), no header declares it yet.
extern CSVector gTrajectoryVector;

// Death/respawn countdown, tentative name. The only cross reference in the
// whole binary is SpideyAI0's state 128 case (a scan of every
// tools/functions/*.bin for the little endian address finds this function and
// nothing else): it accumulates field_80 once the death animation has
// finished, and at 0x78 or more resets the flash and sets gLevelStatus to 2.
static i32 * const gSpideyDeathTimer = reinterpret_cast<i32*>(0x006A7F98);

// The dword right after Db_SkyColor (db.h, 0x0056FC74). SpideyAI0 copies it
// into Db_SkyColor when the pending water effect fires, which is the same pair
// trig.cpp's SetSkyColor command writes. trig.cpp already has an identical
// file-local pointer under this name; it belongs in db.h next to Db_SkyColor.
static u32 * const gDbSkyColorTarget = reinterpret_cast<u32*>(0x0056FC78);

// gSaveGame + 0x7B, the "vibration on" option byte. Same pointer spidey.cpp
// uses under this name in two places.
static u8 * const gSaveGameVibration = reinterpret_cast<u8*>(0x006828D3);

// Cheat flags. Same addresses (and names) pshell.cpp defines; they scale the
// extra body parts SpideyAI0 spawns.
#define G_PULSATING_HEAD_FLAG (*reinterpret_cast<i32*>(0x0060CFF0))
#define G_TOON_SPIDEY_FLAG (*reinterpret_cast<i32*>(0x02E09BF0))
#define G_STICKMAN_FLAG (*reinterpret_cast<i32*>(0x02E09BF4))

// CurrentSuit (0x005559DC, real IDB name); spool.cpp already has the same
// macro under this name, baddy.cpp the same address as a pointer.
#define G_CURRENTSUIT (*reinterpret_cast<i32*>(0x005559DC))

// Lookaround camera accumulators, same file-local pointers (and names)
// spidey.cpp uses for CPlayer::EnterLookaroundMode / SetupLookaroundCamera.
static i32 * const gLookaroundYawOffset = reinterpret_cast<i32*>(0x006A7FFC);
static i32 * const gLookaroundActiveCamAngle = reinterpret_cast<i32*>(0x006A818C);
static i32 * const gLookaroundPitchSmoothed = reinterpret_cast<i32*>(0x006A82B4);
static i32 * const gLookaroundYawSmoothed = reinterpret_cast<i32*>(0x006A8D54);

// gSaveGame practice-difficulty byte, same pointer/name pshell.cpp and
// spidey.cpp already use.
static u8 * const gPracticeDifficultyFlag = reinterpret_cast<u8*>(0x0060CFC7);

// {u16 anim, u16 followOnAnim} x 200, sorted into animation order by
// CPlayer::SortAnimationFollowOnData (spidey.cpp, whose comment already points
// at this read site, 0x4B204A).
// gSaveGame + 0x?? style byte that spidey.cpp already calls
// gRedbookXaPlayingMaybe at this address; kept the same name.
static char * const gRedbookXaPlayingMaybe = reinterpret_cast<char*>(0x00682770);

// real IDB names
static i32 * const pgRedbookXaRelatedOne = reinterpret_cast<i32*>(0x00681D1C);
static i32 * const pgRedbookXaRelatedTwo = reinterpret_cast<i32*>(0x006612C0);
#define gRedbookXaRelatedOne (*pgRedbookXaRelatedOne)
#define gRedbookXaRelatedTwo (*pgRedbookXaRelatedTwo)

static u16 * const gAnimFollowOnData = reinterpret_cast<u16*>(0x00555C6C);

// Scales the two extra body parts (spidey sense buzz, fists) the way the
// cheats ask for. Reproduces the original's three separate inline copies:
// they only differ in what the stickman cheat does, a halve for the fists
// (0x4B1862, 0x4B18CF) and a *3/4 for the buzz (0x4B16DE).
// @Bogus
static void SpideyAI0_ApplyCheatScale(CBody *pBody, i32 stickmanHalves)
{
	if (G_TOON_SPIDEY_FLAG != 0)
	{
		pBody->mScale.vx = (i16)(pBody->mScale.vx * 3 >> 1);
		pBody->mScale.vy = (i16)(pBody->mScale.vy * 3 >> 1);
		pBody->mScale.vz = (i16)(pBody->mScale.vz * 3 >> 1);
	}
	else if (G_STICKMAN_FLAG != 0)
	{
		if (stickmanHalves != 0)
		{
			pBody->mScale.vx = (i16)(pBody->mScale.vx >> 1);
			pBody->mScale.vy = (i16)(pBody->mScale.vy >> 1);
			pBody->mScale.vz = (i16)(pBody->mScale.vz >> 1);
		}
		else
		{
			pBody->mScale.vx = (i16)(pBody->mScale.vx * 3 >> 2);
			pBody->mScale.vy = (i16)(pBody->mScale.vy * 3 >> 2);
			pBody->mScale.vz = (i16)(pBody->mScale.vz * 3 >> 2);
		}
	}
}

// @BIGTODO
// Original at 0x4B13F0, 0x72DA bytes / 7655 instructions / 1482 basic blocks.
// Identity confirmed: the maintainer's IDB (idbs/spideypc_names.txt line 1652)
// and the Mac build (.SpideyAI0__FP7CPlayer, 0x109820) agree with
// tools/names.json, and on the Mac SpideyAI0 is the ONLY function in
// spid_ai0.cpp (the next symbol is __sinit_spid_ai0_cpp).
//
// PARTIALLY PORTED. What is real code below:
//   * the per-frame prologue, 0x4B13F0..0x4B211F
//   * the state dispatch itself, 0x4B211F..0x4B215D
// Everything else is still the original's job; each unported state is marked
// with the address range it covers so the next pass can take them one at a
// time. Nothing here is hooked, so the partial body cannot affect the running
// game.
//
// STRUCTURE. This is a one-hot state machine on CPlayer::field_E1C, which
// holds a single set bit, not a small ordinal:
//        > 0x10000  -> 0x4B50AE  (further dispatch, 1936 instructions)
//       == 0x10000  -> 0x4B4E9B  (132 instructions)
//        > 0x100    -> 0x4B307C  (further dispatch, 1871 instructions)
//       == 0x100    -> 0x4B2F80  (58 instructions)
//   otherwise switch(field_E1C - 1) over jpt_4B86CC with the byte index table
//   at 0x4B86EC. Read out of the exe, the eight jump-table slots are
//       0 -> 0x4B2164   1 -> 0x4B274D   2 -> 0x4B28D9   3 -> 0x4B2BF3
//       4 -> 0x4B2D43   5 -> 0x4B269B   6 -> 0x4B2892   7 -> 0x4B6E8C
//   and the index table selects slot 7 (the default) for every value except
//       1 -> 0x4B2164 (346 instr)    2 -> 0x4B274D  (80 instr)
//       4 -> 0x4B28D9 (215 instr)    8 -> 0x4B2BF3  (77 instr)
//      16 -> 0x4B2D43 (157 instr)   64 -> 0x4B269B  (39 instr)
//     128 -> 0x4B2892 (15 instr)
//   0x4B6E8C (def_4B215D) is both the default case and the shared `break`
//   target every state jumps back to, which is why 130+ `jmp 0x4B6E8C` show up
//   in the disassembly. It runs to the epilogue at 0x4B86B1 (1725 instructions)
//   and contains a second switch at 0x4B7971 over CItem::mType 304..324
//   (21 cases; 308, 309, 311, 316, 318, 319 and 321..323 fall to the default).
//
// ADDRESS FINDINGS made while mapping the prologue (see the report):
//   * 0x466CE0 is CPlayer::DoPhysics. PLAN.md records its PC address as
//     unlocated. The Mac build orders Physics_SetGravity (0xA7270),
//     DoPhysics (0xA7340), DoSwingingPhysics (0xA82A0), DoCrawlingPhysics
//     (0xA8640); the PC has Physics_SetGravity (0x466C70), sub_466CE0,
//     sub_467D20, CPlayer_DoCrawlingPhysics (0x467FD0) in the same order, and
//     sub_466CE0's body tail-calls both sub_467D20 and 0x467FD0 exactly the
//     way DoPhysics dispatches to the swinging and crawling variants.
//   * 0x467D20 is CPlayer::DoSwingingPhysics, by the same ordering.
//   * 0x4BD510 is CPlayer::ReadAnalogueInput (already @Ok in spidey.cpp with
//     no address recorded). Its first instructions copy field_E2D/field_E2E
//     into field_E2F/field_E30, clear both, and bail on field_E18 with
//     field_1AC, which is exactly what the implemented body does.
void SpideyAI0(CPlayer *pPlayer)
{
	u32 keyMappingA;
	u32 keyMappingB;

	// var_49C: mVel.vy as it was before DoPhysics ran, compared against the
	// post-physics value by state 4.
	i32 velYBeforePhysics = 0;

	// var_4A0: the animation whose follow-on was just resolved, 0xFFFF when
	// no animation finished this tick. Only ever read 16 bits wide.
	i32 animJustFinished = 0xFFFF;

	pPlayer->field_AE4 = 1;

	PCINPUT_GetKeyboardMappingForAction(0x40, &keyMappingA);
	PCINPUT_GetKeyboardMappingForAction(0x100, &keyMappingB);

	// PC-only: with the "suit 5" costume the two mapped keys together toggle
	// field_57C, which drives the 0x800 CItem flag right below.
	if (G_CURRENTSUIT == 5
		&& PCINPUT_IsKeyPressed((u8)keyMappingA, 1) != 0
		&& PCINPUT_IsKeyPressed((u8)keyMappingB, 1) != 0
		&& pPlayer->field_1AC == 0
		&& pPlayer->field_E18 == 0)
	{
		i8 wasSet = pPlayer->field_57C;

		reinterpret_cast<u8*>(pPlayer->field_E0C)[0x51] = 0;
		pPlayer->field_57C = (i8)(wasSet == 0);
		pPlayer->field_570 = 0;
		SFX_PlayPos(0x1F, &pPlayer->mPos, 0);
	}

	if (pPlayer->field_57C != 0)
	{
		pPlayer->mFlags |= 0x800;
	}
	else
	{
		pPlayer->mFlags &= 0xF7FF;
	}

	if (gWaterEffect == 1)
	{
		Db_SkyColor = *gDbSkyColorTarget;
		Db_UpdateSky();
		gWaterEffect = 0;
	}

	if (pPlayer->field_36C != 0)
	{
		pPlayer->field_36C -= pPlayer->field_80;
		if (pPlayer->field_36C < 0)
		{
			pPlayer->field_36C = 0;
		}
	}

	if (pPlayer->field_534 != 0)
	{
		pPlayer->field_534 -= pPlayer->field_80;
		if (pPlayer->field_534 < 0)
		{
			pPlayer->field_534 = 0;
		}
	}

	// electrocution timer: keeps the pad buzzing while it runs down.
	if (pPlayer->field_50C != 0)
	{
		if (*gSaveGameVibration != 0)
		{
			Pad_ActuatorOn(0, (u16)pPlayer->field_50C, 1, 0xDB);
		}

		pPlayer->field_50C -= pPlayer->field_80;
		if (pPlayer->field_50C <= 0)
		{
			pPlayer->field_50C = 0;
			Effects_UnElectrify(pPlayer);
		}
	}

	// spidey-sense buzz timer (field_ECC), its SFX handle (field_ED0) and the
	// extra body part it shows (field_ED4).
	if (pPlayer->field_ECC != 0)
	{
		i32 buzzLeft = pPlayer->field_ECC - pPlayer->field_80;

		pPlayer->field_ECC = buzzLeft;
		if (buzzLeft <= 0)
		{
			pPlayer->field_ECC = 0;
		}
		else
		{
			CBody *pBuzz;

			// 0xEC8: the buzz kind. 1 also triggers the water effect.
			if (PLR_I32(pPlayer, 0xEC8) == 1)
			{
				gWaterEffect = 1;
			}

			if (pPlayer->field_ED0 == 0)
			{
				pPlayer->field_ED0 = SFX_Play(0x1A, 0x2000, 0);
			}

			if ((i32)(Pad_GetActuatorTime(0, 1) & 0xFFFF) < buzzLeft
				&& *gSaveGameVibration != 0)
			{
				Pad_ActuatorOn(0, (u16)pPlayer->field_ECC, 1, 0x3C);
			}

			if (Mem_RecoverPointer(&pPlayer->field_ED4) == 0)
			{
				pBuzz = new CBody();
				pPlayer->field_ED4 = Mem_MakeHandle(pBuzz);
				pBuzz->InitItem("items");
				pBuzz->mFlags |= 0x200;
				pBuzz->mModel = 4;
				pBuzz->mType = 0x1F8;
				pBuzz->AttachTo(reinterpret_cast<CBody**>(&SpideyAdditionalBodyPartsList));

				if (G_PULSATING_HEAD_FLAG != 0)
				{
					pBuzz->mScale.vx = (i16)(pBuzz->mScale.vx << 1);
					pBuzz->mScale.vy = (i16)(pBuzz->mScale.vy << 1);
					pBuzz->mScale.vz = (i16)(pBuzz->mScale.vz << 1);
				}

				SpideyAI0_ApplyCheatScale(pBuzz, 0);
			}
		}
	}

	if (pPlayer->field_ECC == 0)
	{
		CBody *pBuzz;

		if (pPlayer->field_ED0 != 0)
		{
			SFX_Stop(pPlayer->field_ED0);
			pPlayer->field_ED0 = 0;
		}

		pBuzz = reinterpret_cast<CBody*>(Mem_RecoverPointer(&pPlayer->field_ED4));
		if (pBuzz != 0)
		{
			pBuzz->DeleteFrom(reinterpret_cast<CBody**>(&SpideyAdditionalBodyPartsList));
			delete pBuzz;
			pPlayer->field_ED4 = Mem_MakeHandle(0);
		}
	}

	// the two fists (field_5B8), sized from the combo bonus timer field_5B0.
	if (pPlayer->field_5AC != 0)
	{
		i32 fist;

		if (pPlayer->field_5B0 < 0x2000)
		{
			pPlayer->field_5B0 += pPlayer->field_80 * 384;
		}
		else
		{
			pPlayer->field_5B0 = 0x2000;
		}

		for (fist = 0; fist < 2; fist++)
		{
			CBody *pFist = reinterpret_cast<CBody*>(Mem_RecoverPointer(&pPlayer->field_5B8[fist]));

			if (pFist == 0)
			{
				pFist = new CBody();
				pPlayer->field_5B8[fist] = Mem_MakeHandle(pFist);
				pFist->InitItem("items");
				pFist->mModel = 8;
				pFist->mType = 0x1F9;
				pFist->AttachTo(reinterpret_cast<CBody**>(&SpideyAdditionalBodyPartsList));
				SpideyAI0_ApplyCheatScale(pFist, 1);
				pFist->mFlags |= 0x200;
			}

			if (pPlayer->field_5B0 < 0x2000)
			{
				i16 scale = (i16)(pPlayer->field_5B0 / 2);

				pFist->mFlags |= 0x200;
				pFist->mScale.vx = scale;
				pFist->mScale.vy = scale;
				pFist->mScale.vz = scale;
				SpideyAI0_ApplyCheatScale(pFist, 1);
			}
		}
	}
	else if (pPlayer->field_5B0 > 0)
	{
		i32 half;
		i32 fist;

		pPlayer->field_5B0 -= pPlayer->field_80 * 512;
		if (pPlayer->field_5B0 < 0)
		{
			pPlayer->field_5B0 = 0;
		}

		half = pPlayer->field_5B0 / 2;

		for (fist = 0; fist < 2; fist++)
		{
			CBody *pFist = reinterpret_cast<CBody*>(Mem_RecoverPointer(&pPlayer->field_5B8[fist]));

			if (pFist != 0)
			{
				if (half != 0)
				{
					pFist->mFlags |= 0x200;
					pFist->mScale.vx = (i16)half;
					pFist->mScale.vy = (i16)half;
					pFist->mScale.vz = (i16)half;
				}
				else
				{
					pFist->DeleteFrom(reinterpret_cast<CBody**>(&SpideyAdditionalBodyPartsList));
					delete pFist;
					// the original only clears the pointer half of the handle
					pPlayer->field_5B8[fist].pWhatever = 0;
				}
			}
		}
	}

	// 0x553: one-shot "first tick" flag; drops the player onto the floor.
	if (PLR_U8(pPlayer, 0x553) == 0)
	{
		i32 groundY;

		PLR_U8(pPlayer, 0x553) = 1;
		groundY = Utils_GetGroundHeight(&pPlayer->mPos, 0, 0x800, 0);
		if (groundY != -1)
		{
			pPlayer->mPos.vy = groundY - (pPlayer->field_EA8 << 12);
			pPlayer->field_E1C = 1;
			pPlayer->field_A8.vx = 0;
			pPlayer->field_A8.vy = (i16)0xF000;
			pPlayer->field_A8.vz = 0;
			pPlayer->OrientToNormal(false, &ZeroVector);
			pPlayer->SetFloorCamera(0);
			pPlayer->PutCameraBehind(0);
		}
	}

	if (pPlayer->field_5D8 == 0 && pPlayer->field_5E8 == 0)
	{
		if (pPlayer->mWebbing < 0x200)
		{
			pPlayer->IncreaseWebbing(pPlayer->field_80 * 4);
		}
		else if (pPlayer->mWebbing < 0x555)
		{
			pPlayer->IncreaseWebbing(pPlayer->field_80);
		}
	}

	if (pPlayer->field_E64 != 0)
	{
		if ((pPlayer->field_E1C & 0x400) != 0)
		{
			reinterpret_cast<CBody*>(pPlayer->field_E64)->AI();
		}
		else
		{
			delete reinterpret_cast<CBody*>(pPlayer->field_E64);
			pPlayer->field_E64 = 0;
		}
	}

	if (pPlayer->field_DBC != 0 && (pPlayer->field_DBC->mCBodyFlags & 0x40) != 0)
	{
		pPlayer->field_DBC = 0;
	}

	BaddyCollisionCheck = 0;

	// 0xE28: a plain countdown, nothing in the prologue reads it.
	if (PLR_I32(pPlayer, 0xE28) != 0)
	{
		PLR_I32(pPlayer, 0xE28) -= pPlayer->field_80;
		if (PLR_I32(pPlayer, 0xE28) < 0)
		{
			PLR_I32(pPlayer, 0xE28) = 0;
		}
	}

	pPlayer->mAnimSpeed = 0x10000;

	// 0xE24: "AI suspended" gate; the whole tick is skipped.
	if (PLR_U8(pPlayer, 0xE24) != 0)
	{
		return;
	}

	print_if_false(G_MECHLIST->mNextItem == 0, "2+ elements in MechList");

	pPlayer->mFlags |= 4;
	pPlayer->field_E8 = pPlayer->mPos;

	velYBeforePhysics = pPlayer->mVel.vy;

	if ((pPlayer->field_E1C & 0x83000) == 0)
	{
		// 0x466CE0, identified as CPlayer::DoPhysics (see the note above).
		pPlayer->DoPhysics();
	}

	if (pPlayer->field_A8.vy > 0xD48)
	{
		pPlayer->OrientToNormal(true, &pPlayer->field_AC8);
		pPlayer->field_8E8 = 0;
		pPlayer->field_8E9 = 1;
	}
	else
	{
		pPlayer->field_8E9 = 0;
		pPlayer->OrientToNormal(false, &ZeroVector);
		pPlayer->field_8E8 = (u8)(pPlayer->field_A8.vy >= (i16)0xF5D8);
	}

	{
		i32 state = pPlayer->field_E1C;
		// 0xE3C: gTimerRelated stamp of the last tick spent in state 4.
		i32 lastAirborneTime = PLR_I32(pPlayer, 0xE3C);
		i32 camKind;

		if (state != 4)
		{
			PLR_I32(pPlayer, 0xE3C) = (i32)gTimerRelated;
		}

		camKind = pPlayer->field_540;

		if ((state & 0x3000) == 0)
		{
			if (state == 4)
			{
				if (camKind != 5
					&& pPlayer->mVel.vy > 0
					&& (u32)(gTimerRelated - lastAirborneTime) > 0x3C)
				{
					pPlayer->SetFallingCamera(15);
				}
			}
			else if (camKind != 4 && pPlayer->field_54C != 0)
			{
				pPlayer->SetSwingCamera(15);
			}
			else if (camKind != 2 && pPlayer->field_8E9 != 0)
			{
				pPlayer->SetCeilingCamera(16);
			}
			else if (camKind != 1 && pPlayer->field_8E8 != 0 && pPlayer->field_54C == 0)
			{
				pPlayer->SetWallCamera(16);
			}
			else if (camKind != 0
				&& pPlayer->field_8E8 == 0
				&& pPlayer->field_8E9 == 0
				&& pPlayer->field_54C == 0)
			{
				pPlayer->SetFloorCamera(16);
			}
		}
	}

	if (pPlayer->field_AD4 != 0)
	{
		pPlayer->mFric.vx = 1;
		pPlayer->mFric.vy = 1;
		pPlayer->mFric.vz = 1;
	}
	else if ((pPlayer->field_E1C & 0x1000000) != 0)
	{
		pPlayer->mFric.vx = 0x1F;
		pPlayer->mFric.vy = 0x1F;
		pPlayer->mFric.vz = 0x1F;
	}
	else if ((pPlayer->field_E1C & 6) != 0)
	{
		pPlayer->mFric.vx = 4;
		pPlayer->mFric.vy = 4;
		pPlayer->mFric.vz = 4;
	}
	else
	{
		pPlayer->mFric.vx = 1;
		pPlayer->mFric.vy = ((pPlayer->field_E1C & 0x40000) != 0) ? 1 : 4;
		pPlayer->mFric.vz = 1;
	}

	pPlayer->SelectAutoAimTarget();
	pPlayer->UpdateOffscreenSpideySenseIndicatorList();
	pPlayer->BuildOffscreenSpideySenseIndicatorList();

	pPlayer->mRMinor = 0x64;

	// 0xE8E: how long the player has been in a wall-crawl/swing state without
	// a collision, as a 16 bit accumulator of field_80.
	if ((pPlayer->field_E1C & 0x800004) != 0 && (pPlayer->mCollision & 2) == 0)
	{
		PLR_U16(pPlayer, 0xE8E) = (u16)(PLR_U16(pPlayer, 0xE8E) + (u16)pPlayer->field_80);
	}
	else
	{
		PLR_U16(pPlayer, 0xE8E) = 0;
	}

	pPlayer->ReadAnalogueInput();

	{
		u8 *pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
		u8 padFire;
		i32 usedDirs;

		if (pPad[0x100] == 0)
		{
			pPlayer->field_E8D = 1;
		}

		usedDirs = pPlayer->field_8E4;
		if (usedDirs != 0)
		{
			if ((usedDirs & 1) != 0 && pPlayer->field_E2D <= 0)
			{
				pPlayer->field_8E4 = usedDirs & ~1;
			}

			usedDirs = pPlayer->field_8E4;
			if ((usedDirs & 2) != 0 && pPlayer->field_E2D >= 0)
			{
				pPlayer->field_8E4 = usedDirs & ~2;
			}

			usedDirs = pPlayer->field_8E4;
			if ((usedDirs & 4) != 0 && pPlayer->field_E2E <= 0)
			{
				pPlayer->field_8E4 = usedDirs & ~4;
			}

			usedDirs = pPlayer->field_8E4;
			if ((usedDirs & 8) != 0 && pPlayer->field_E2E >= 0)
			{
				pPlayer->field_8E4 = usedDirs & ~8;
			}
		}

		if (pPlayer->field_550 != 0 && pPad[0x70] == 0)
		{
			pPlayer->field_550 = 0;
		}

		// only these states let the lookaround button be held
		if ((pPlayer->field_E1C & 0x3EBE3FCE) != 0)
		{
			pPad[0x40] = 0;
		}
		else if ((pPlayer->field_E1C & 0x1C000) != 0)
		{
			if (pPlayer->field_8EA == 0)
			{
				pPad[0x40] = 0;
			}
		}
		else if (PLR_I32(pPlayer, 0xE48) != 0 || pPlayer->field_EA4 != 4)
		{
			pPad[0x40] = 0;
		}

		padFire = pPad[0x40];

		// 0x8EB: lookaround allowed this tick.
		if (padFire != 0 && PLR_U8(pPlayer, 0x8EB) != 0)
		{
			pPlayer->field_56C += pPlayer->field_80;
		}
		else
		{
			pPlayer->field_56C = 0;
		}

		if ((u32)pPlayer->field_56C > 0xF)
		{
			if (pPlayer->field_8EA == 0)
			{
				ECameraMode mode = CameraList->mCameraMode;

				if (mode != CAMERAMODE_START
					&& mode != CAMERAMODE_FAR
					&& mode != CAMERAMODE_OVERHEAD
					&& mode != CAMERAMODE_NOTHING)
				{
					pPlayer->EnterLookaroundMode();
				}
			}
			else if (padFire == 0)
			{
				pPlayer->ExitLookaroundMode();
			}
		}
		else if (pPlayer->field_8EA != 0 && padFire == 0)
		{
			pPlayer->ExitLookaroundMode();
		}

		if (pPlayer->field_8EA != 0 && (pPlayer->field_CE4 | pPlayer->field_CB4) == 0)
		{
			u8 pressed;

			if (*gPracticeDifficultyFlag != 0)
			{
				pressed = pPad[0x101];
			}
			else
			{
				pressed = pPad[0x71];
			}

			if (pressed != 0 && pPlayer->field_E64 == 0)
			{
				pPad[0x101] = 0;
				pPlayer->field_54F = 1;
				pPad[0x71] = 0;
				pPlayer->field_2C1 = 0;
				// 0x231: cleared together with field_2C1 here.
				PLR_U8(pPlayer, 0x231) = 0;
			}

			if (pPlayer->field_E2D != 0 && pPlayer->field_8E0 == 0)
			{
				i32 delta = ((pPlayer->field_E2D >> 3) * pPlayer->field_8F0 * pPlayer->field_80) >> 8;
				i32 was = *gLookaroundActiveCamAngle;
				i32 now;

				// G_GAMESTATE[14] flips the pitch direction (look inversion).
				if (G_GAMESTATE[14] == 0)
				{
					now = was + delta;
				}
				else
				{
					now = was - delta;
				}

				*gLookaroundActiveCamAngle = now;
				if ((now < 0 ? -now : now) > 0x400)
				{
					*gLookaroundActiveCamAngle = was;
				}
			}

			if (pPlayer->field_E2E != 0 && pPlayer->field_8E0 == 0)
			{
				i32 delta = ((pPlayer->field_E2E >> 3) * pPlayer->field_8F0 * pPlayer->field_80) >> 8;
				i32 now = *gLookaroundYawOffset + delta;

				*gLookaroundYawOffset = now;
				if ((now < 0 ? -now : now) > 0x400)
				{
					i32 clamped = (now > 0x400) ? (now - 0x400) : (now + 0x400);
					i16 heading = pPlayer->GetEffectiveHeading();

					if (pPlayer->field_8E9 != 0)
					{
						pPlayer->SetTargetTorsoAngle((i16)((heading - clamped) & 0xFFF), false);
					}
					else
					{
						pPlayer->SetTargetTorsoAngle((i16)((clamped + heading) & 0xFFF), false);
					}
				}
			}
		}
		else
		{
			*gLookaroundYawOffset = 0;
			*gLookaroundPitchSmoothed = 0;
			*gLookaroundYawSmoothed = 0;
		}
	}

	// field_EBC ramps up to 16 while there is any move input and back down to
	// 0 when there is none.
	if ((pPlayer->field_E2E | pPlayer->field_E2D) != 0)
	{
		if (pPlayer->field_EBC < 0x10)
		{
			pPlayer->field_EBC += pPlayer->field_80;
			if (pPlayer->field_EBC > 0x10)
			{
				pPlayer->field_EBC = 0x10;
			}
		}
	}
	else
	{
		if (pPlayer->field_EBC != 0)
		{
			pPlayer->field_EBC -= pPlayer->field_80;
		}
		if (pPlayer->field_EBC < 0)
		{
			pPlayer->field_EBC = 0;
		}
	}

	// animation follow-on: when the anim that just finished has an entry in
	// the sorted follow-on table, chain straight into it.
	if (pPlayer->mAnimFinished != 0)
	{
		u16 anim = pPlayer->mAnim;

		if (gAnimFollowOnData[anim * 2] != 0)
		{
			animJustFinished = anim;
			pPlayer->PlaySingleAnim(gAnimFollowOnData[anim * 2 + 1], 0, -1);
		}
		else if (anim == 0)
		{
			animJustFinished = 0;
			pPlayer->PlaySingleAnim(0, 0, -1);
		}
		else if (anim == 0x32)
		{
			animJustFinished = 0x32;
			pPlayer->PlaySingleAnim(Rnd(2) != 0 ? 0x33 : 0x55, 0, -1);
		}
		else if (anim == 0x33 || anim == 0x55)
		{
			animJustFinished = anim;
			pPlayer->PlaySingleAnim(0x32, 0, -1);
		}
		else if (anim == 0x39)
		{
			animJustFinished = 0x39;
			pPlayer->PlaySingleAnim(pPlayer->mAnimDir == 1 ? 0x32 : 0x13, 0, -1);
		}
		else if (anim == 0x3A)
		{
			animJustFinished = 0x3A;
			pPlayer->PlaySingleAnim(pPlayer->mAnimDir == 1 ? 0x34 : 0x13, 0, -1);
		}
	}

	// -----------------------------------------------------------------------
	// state dispatch, 0x4B211F..0x4B215D. Every case's `break` is the
	// original's `jmp def_4B215D`, the shared tail at 0x4B6E8C, which is not
	// ported yet.
	// -----------------------------------------------------------------------
	switch (pPlayer->field_E1C)
	{
		// 0x4B2164. Standing still: the idle/lean animations, the idle
		// Redbook track, and the idle web-shot at the ceiling.
		case 1:
		{
			u8 *pPad;
			u8 anyInput;
			u8 xaPlaying;
			u16 anim;
			i32 wantAnim;
			i32 roll;
			i32 webChance;
			i32 midChance;
			CWeb *pWeb;

			if (pPlayer->CheckGroundGone() != 0) goto L_ReleaseWeb;

			anim = pPlayer->mAnim;
			if (anim == 0x124) break;

			if ((anim == 0x122 && pPlayer->field_E6C != 0) || anim == 0x123)
			{
				if (pPlayer->mFrame > 0x1C && pPlayer->field_E6C != 0)
				{
					reinterpret_cast<CWeb*>(pPlayer->field_E6C)->field_102 = 2;
				}

				pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
				anyInput = (u8)(pPad[0x70] | pPad[0x60] | pPad[0x50] | pPad[0x40]
					| pPad[0x30] | pPad[0x20] | pPad[0x10] | pPad[0]
					| (u8)pPlayer->field_E2D | (u8)pPlayer->field_E2E);
				xaPlaying = *gRedbookXaPlayingMaybe;

				if (anyInput != 0)
				{
					if (xaPlaying != 0)
					{
						if (gRedbookXaRelatedOne == 0x20)
						{
							if (gRedbookXaRelatedTwo != 9)
							{
								pPlayer->PlaySingleAnim(0x124, 0, -1);
								goto L_ReleaseWeb;
							}
							Redbook_XAStop();
						}
						else if (gRedbookXaRelatedOne == 0x21 && gRedbookXaRelatedTwo == 0)
						{
							Redbook_XAStop();
						}
					}

					pPlayer->PlaySingleAnim(0x124, 0, -1);
					goto L_ReleaseWeb;
				}

				if (xaPlaying != 0) break;
				if (Rnd(0x40) != 0) break;

				if (Rnd(2) != 0)
				{
					Redbook_XAPlay(0x21, 0, 0);
				}
				else
				{
					Redbook_XAPlay(0x20, 9, 0);
				}
				break;
			}

			if (pPlayer->CheckWebShot() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckJumpingR1ZipWeb() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckKick() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckCeilingJumpingSmashPunch() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckJumpingSwingWeb() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckJumpingR2ZipWeb() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckJump() != 0) goto L_ReleaseWeb;
			if (pPlayer->CheckForwards(true) != 0) goto L_ReleaseWeb;

			if (pPlayer->field_8EA == 0)
			{
				pPlayer->field_EA6 = (i16)(pPlayer->field_EA6 + (u16)pPlayer->field_80);
			}

			anim = pPlayer->mAnim;
			if (pPlayer->field_DF8 == 0)
			{
				if (anim == 0x1B || anim == 0x1C || anim == 0x1D || anim == 0x1E)
				{
					pPlayer->PlaySingleAnim(pPlayer->field_AD4 != 0 ? 0x13 : 0, 0, -1);
				}
			}
			else
			{
				pPlayer->field_EA6 = 0;

				if (anim != 0xC8 && anim != 0xC2)
				{
					if (pPlayer->field_AD4 != 0)
					{
						if (pPlayer->field_DF4 > 0)
						{
							if (anim != 0x1D) pPlayer->PlaySingleAnim(0x1D, 0, -1);
						}
						else
						{
							if (anim != 0x1E) pPlayer->PlaySingleAnim(0x1E, 0, -1);
						}
					}
					else
					{
						if (pPlayer->field_DF4 > 0)
						{
							if (anim != 0x1B) pPlayer->PlaySingleAnim(0x1B, 0, -1);
						}
						else
						{
							if (anim != 0x1C) pPlayer->PlaySingleAnim(0x1C, 0, -1);
						}
					}
				}
			}

			// the idle "shoot a web at the ceiling" animation reaching its
			// release frame: spawn the web and fire it at the spot the
			// lookaround raycast below stored in 0x514/0x520.
			if (pPlayer->mAnim == 0x122
				&& pPlayer->mFrame >= 4
				&& pPlayer->field_E6C == 0)
			{
				pWeb = new CWeb();
				pWeb->field_F8 = (u8)pPlayer->field_5E8;
				pPlayer->field_E6C = reinterpret_cast<i32*>(pWeb);
				pWeb->field_102 = 0;
				// 0x510: "the idle web is out" flag.
				PLR_U8(pPlayer, 0x510) = 1;
				pWeb->Fire(pPlayer->mPos,
					*reinterpret_cast<CVector*>(reinterpret_cast<char*>(pPlayer) + 0x514),
					0,
					true,
					*reinterpret_cast<CSVector*>(reinterpret_cast<char*>(pPlayer) + 0x520));

				reinterpret_cast<CWeb*>(pPlayer->field_E6C)->field_12C[0x6C] = 0;
				SFX_PlayPos(0x15, &pPlayer->mPos, 0);
			}

			if (pPlayer->field_8EA != 0)
			{
				wantAnim = (pPlayer->field_AD4 != 0) ? 0x13 : 0;
				if (pPlayer->mAnim != (u16)wantAnim)
				{
					pPlayer->PlaySingleAnim(wantAnim, 0, -1);
				}
				goto L_ReleaseWeb;
			}

			if ((u16)animJustFinished != 0) break;
			if ((u16)pPlayer->field_EA6 <= 0xF0) break;

			roll = Rnd(0x64);
			webChance = 0;

			if (pPlayer->field_AD4 == 0 && (u16)pPlayer->field_EA6 > 0xE10)
			{
				SLineInfo lineInfo;

				lineInfo.StartCoords = pPlayer->mPos;
				lineInfo.EndCoords.vx = pPlayer->mPos.vx;
				lineInfo.EndCoords.vy = pPlayer->mPos.vy - 0x800000;
				lineInfo.EndCoords.vz = pPlayer->mPos.vz;
				lineInfo.MinCoords.vx = 0;
				lineInfo.MinCoords.vy = 0;
				lineInfo.MinCoords.vz = 0;
				lineInfo.MaxCoords.vx = 0;
				lineInfo.MaxCoords.vy = 0;
				lineInfo.MaxCoords.vz = 0;
				lineInfo.Position.vx = 0;
				lineInfo.Position.vy = 0;
				lineInfo.Position.vz = 0;
				lineInfo.Normal.vx = 0;
				lineInfo.Normal.vy = 0;
				lineInfo.Normal.vz = 0;

				M3dColij_InitLineInfo(&lineInfo);
				M3dZone_LineToItem(&lineInfo, 1);

				if (lineInfo.pItem != 0 && (lineInfo.pFace[3] & 0x40000) == 0)
				{
					CVector *pHitPos = reinterpret_cast<CVector*>(reinterpret_cast<char*>(pPlayer) + 0x514);
					CSVector *pHitNormal = reinterpret_cast<CSVector*>(reinterpret_cast<char*>(pPlayer) + 0x520);

					pHitPos->vx = lineInfo.Position.vx;
					pHitPos->vy = lineInfo.Position.vy;
					pHitPos->vz = lineInfo.Position.vz;
					pHitNormal->vx = lineInfo.Normal.vx;
					pHitNormal->vy = lineInfo.Normal.vy;
					pHitNormal->vz = lineInfo.Normal.vz;
					webChance = 0x32;
				}
			}

			if (roll < webChance)
			{
				pPlayer->PlaySingleAnim(0x122, 0, -1);
				break;
			}

			midChance = webChance + ((0x64 - webChance) / 2);
			if (roll < midChance)
			{
				pPlayer->PlaySingleAnim(0x125, 0, -1);
				break;
			}

			pPlayer->PlaySingleAnim(0x126, 0, -1);
			break;

		L_ReleaseWeb:
			// 0x4B24D5, shared with several other states
			if (pPlayer->field_E6C != 0)
			{
				reinterpret_cast<CWeb*>(pPlayer->field_E6C)->SwitchToBlob();
				pPlayer->field_E6C = 0;
			}
			break;
		}

		// 0x4B274D. Airborne after a jump: the whole "can I grab something"
		// chain, then the landing timeout.
		case 2:
		{
			u8 *pPad;

			pPlayer->field_EA6 = 0;

			if (pPlayer->CheckStickToCeiling() != 0) break;
			if (pPlayer->CheckJumpingSmashKick() != 0) break;
			if (pPlayer->CheckJumpingSwingWeb() != 0) break;
			if (pPlayer->CheckJumpingR1ZipWeb() != 0) break;
			if (pPlayer->CheckJumpingR2ZipWeb() != 0) break;

			if (pPlayer->mAnimFinished != 0)
			{
				if (pPlayer->mAnim == 0xE7)
				{
					pPlayer->PlaySingleAnim(0xE8, 0, -1);
				}
				else if (pPlayer->mAnim == 0xD7)
				{
					pPlayer->PlaySingleAnim(0xD8, 0, -1);
				}
			}

			pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
			if (pPad[0x100] == 0
				&& (DifficultyLevel != 0 || Trig_GetLevelId() != 0x705))
			{
				pPlayer->field_E88 = 0;
			}

			if ((pPlayer->field_504 & 2) != 0
				&& (u32)(gTimerRelated - (i32)pPlayer->field_500) < 6)
			{
				if (pPlayer->ShouldPlayerDropFlail() == 0) break;

				pPlayer->PlaySingleAnim(0xAF, 0, -1);
				pPlayer->mVel.vz = 0;
				pPlayer->mVel.vx = 0;
				pPlayer->field_E1C = 0x800000;
				// 0x54E: cleared with every flail drop.
				PLR_U8(pPlayer, 0x54E) = 0;
				break;
			}

			if (pPlayer->field_E84 > 0) break;
			if (pPlayer->field_E88 > 0) break;

			pPlayer->field_E1C = 4;
			break;
		}

		// 0x4B28D9. Falling.
		case 4:
		{
			i32 velY;
			u16 anim;
			bool retarget;

			pPlayer->field_EA6 = 0;

			if (pPlayer->mHealth <= 0)
			{
				if (pPlayer->CheckLanded() != 0)
				{
					pPlayer->SwitchToDeathMode(false);
				}
				break;
			}

			if (pPlayer->CheckLanded() != 0) break;
			if (pPlayer->CheckStickToWall() != 0) break;
			if (pPlayer->CheckStickToCeiling() != 0) break;
			if (pPlayer->CheckJumpingSmashKick() != 0) break;
			if (pPlayer->CheckJumpingSwingWeb() != 0) break;
			if (pPlayer->CheckJumpingR1ZipWeb() != 0) break;
			if (pPlayer->CheckJumpingR2ZipWeb() != 0) break;

			// fallen out of the world for long enough
			if (PLR_U16(pPlayer, 0xE8E) > 0x1E0)
			{
				gLevelStatus = 8;
			}

			if (pPlayer->mAnimFinished != 0 && pPlayer->mAnim == 0xD7)
			{
				pPlayer->PlaySingleAnim(0xD8, 0, -1);
			}
			if (pPlayer->mAnimFinished != 0 && pPlayer->mAnim == 0x7F)
			{
				pPlayer->PlaySingleAnim(0xD8, 0, -1);
			}

			if ((pPlayer->field_504 & 4) != 0
				&& (u32)(gTimerRelated - (i32)pPlayer->field_500) < 6
				&& pPlayer->ShouldPlayerDropFlail() != 0)
			{
				pPlayer->PlaySingleAnim(0xAF, 0, -1);
				pPlayer->field_E1C = 0x800000;
				PLR_U8(pPlayer, 0x54E) = 0;
			}

			velY = pPlayer->mVel.vy;
			anim = pPlayer->mAnim;
			retarget = true;

			if (((velY ^ velYBeforePhysics) & 0x80000000) == 0
				&& (anim == 0xD3 || anim == 0xD4 || anim == 0xE0 || anim == 0xE1
					|| anim == 0xE7 || anim == 0xE8 || anim == 0xE2 || anim == 0xE4
					|| anim == 0xE9 || anim == 0xEB || anim == 0xAF || anim == 0xB0
					|| anim == 0x7F || anim == 0xEE || anim == 0xF1 || anim == 0xF0
					|| anim == 0xEF || anim == 0xD7 || anim == 0xD8 || anim == 0xDD
					|| anim == 0xDA || anim == 0xDE || anim == 0xDB || anim == 0x116
					|| anim == 0x114))
			{
				retarget = (pPlayer->field_54D != 0 && velY > 0);
			}

			if (retarget)
			{
				pPlayer->field_E38 = pPlayer->mPos.vy;
				// 0xE3C: gTimerRelated stamp of the last fall retarget.
				PLR_I32(pPlayer, 0xE3C) = (i32)gTimerRelated;

				anim = pPlayer->mAnim;
				if (anim != 0xAF && anim != 0xB0)
				{
					if (pPlayer->field_54D != 0)
					{
						pPlayer->PlaySingleAnim(anim == 0x118 ? 0x116 : 0x114, 0, -1);
						pPlayer->field_54D = 0;
					}
					else if (anim == 0xE7)
					{
						pPlayer->PlaySingleAnim(0xE8, 0, -1);
					}
					else if (anim == 0xE0)
					{
						pPlayer->PlaySingleAnim(0xE1, 0, -1);
					}
					else if (anim == 0xDA)
					{
						pPlayer->PlaySingleAnim(0xDB, 0, -1);
					}
					else if (anim == 0xDD)
					{
						pPlayer->PlaySingleAnim(0xDE, 0, -1);
					}
					else if (anim == 0xD7)
					{
						pPlayer->PlaySingleAnim(0xD8, 0, -1);
					}
					else if (anim != 0xE8 && anim != 0xD8)
					{
						pPlayer->PlaySingleAnim(0xD4, 0, -1);
					}
				}
			}

			if (pPlayer->field_AE5 != 0
				&& (u32)(gTimerRelated - PLR_I32(pPlayer, 0xE3C)) > 0x1E)
			{
				pPlayer->field_AE5 = 0;
				pPlayer->field_AE6 = 0;
			}
			break;
		}

		// 0x4B2BF3. On a wall.
		case 8:
		{
			u8 senseFlag;
			u8 *pPad;
			CBody *pTarget;
			i32 e84;

			pPlayer->field_EA6 = 0;

			if (pPlayer->CheckGroundGone() != 0) break;

			if (pPlayer->mAnim != 0xB2 && pPlayer->mAnim != 0xB4)
			{
				if (pPlayer->CheckForwards(true) != 0) break;
				if (pPlayer->CheckJump() != 0) break;
				if (pPlayer->CheckKick() != 0) break;
				if (pPlayer->CheckJumpingSwingWeb() != 0) break;
				if (pPlayer->CheckJumpingR1ZipWeb() != 0) break;
				if (pPlayer->CheckJumpingR2ZipWeb() != 0) break;
			}

			senseFlag = pPlayer->field_E8C;
			pPlayer->field_AE4 = (u8)(senseFlag != 0);

			if ((u16)animJustFinished == 0xFFFF) break;
			if ((u16)animJustFinished == 0xB2) break;

			if (senseFlag == 0)
			{
				// 0x4B682F
				pPlayer->SwitchToStandMode();
				break;
			}

			pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
			if (pPad[0x100] == 0)
			{
				// 0x4B6010
				pPlayer->field_E1C = 0x10;
				break;
			}

			pTarget = pPlayer->field_DBC;
			pPlayer->field_E80 = (i32)0xFFFC4000;
			if (pTarget != 0)
			{
				i32 targetVelY = pTarget->mVel.vy;

				if (targetVelY >= (i32)0xFFFC0000)
				{
					pPlayer->field_E80 = targetVelY + (i32)0xFFFC4000;
				}
				else
				{
					pPlayer->field_E80 = targetVelY * 2 - 0x3C000;
				}
			}

			pPlayer->PlaySingleAnim(0xDF, 0, -1);

			e84 = pPlayer->field_E84;
			pPlayer->field_E8D = 0;
			pPlayer->field_E1C = 0x40;
			pPlayer->field_E88 = (e84 >> 1) + e84;

			if ((pPlayer->mCollision & 1) != 0)
			{
				pPlayer->field_EBC = 0;
			}
			break;
		}

		// 0x4B2D43. Standing / running on the ground.
		case 16:
		{
			u16 followOn;
			i32 aimDelta;
			i16 heading;
			u16 anim;
			i32 newAnim;

			pPlayer->field_EA6 = 0;

			if (pPlayer->mHeldObject != 0 && (pPlayer->mCollision & 0x40) != 0)
			{
				CManipOb *pHeld;

				pPlayer->PlaySingleAnim(0x15, 0, -1);
				pHeld = pPlayer->mHeldObject;
				pPlayer->mHeldObject = 0;
				pHeld->Smash();
			}

			if (pPlayer->CheckGroundGone() != 0) break;
			if (pPlayer->CheckWebShot() != 0) break;
			if (pPlayer->CheckJump() != 0) break;
			if (pPlayer->CheckCeilingJumpingSmashPunch() != 0) break;
			if (pPlayer->CheckKick() != 0) break;
			if (pPlayer->CheckWebShot() != 0) break;
			if (pPlayer->CheckRunIntoWall() != 0) break;

			if (pPlayer->field_AD4 == 0)
			{
				if (pPlayer->CheckJumpingSwingWeb() != 0) break;
				if (pPlayer->CheckJumpingR1ZipWeb() != 0) break;
				if (pPlayer->CheckJumpingR2ZipWeb() != 0) break;
			}

			if (pPlayer->CheckInteriorSurfaceTransition() != 0
				|| pPlayer->CheckExteriorSurfaceTransition() != 0
				|| pPlayer->CheckFenceSurfaceTransition() != 0)
			{
				// the original calls the out of line CVector zero helper at
				// 0x4B8B70 on mVel here
				pPlayer->mVel.vx = 0;
				pPlayer->mVel.vy = 0;
				pPlayer->mVel.vz = 0;
				break;
			}

			if (pPlayer->field_8EA != 0)
			{
				// 0x4B682F
				pPlayer->SwitchToStandMode();
				break;
			}

			if ((u8)(pPlayer->field_E2E | pPlayer->field_E2D) == 0)
			{
				pPlayer->SwitchToStandMode();
				break;
			}

			followOn = (u16)animJustFinished;
			if (followOn == 0x34)
			{
				if (pPlayer->field_AD5 == 0)
				{
					pPlayer->PlaySingleAnim(0x32, 0, -1);
				}
			}
			else if (followOn == 0x33 || followOn == 0x55)
			{
				if (pPlayer->field_AD5 != 0)
				{
					pPlayer->PlaySingleAnim(0x34, 0, -1);
				}
			}

			if (pPlayer->field_8E8 != 0)
			{
				heading = pPlayer->GetEffectiveHeading();
				aimDelta = ((u16)pPlayer->field_E32 - (i32)(u16)heading) & 0xFFF;
			}
			else
			{
				i32 turned = ((u16)pPlayer->field_E32 + (i32)CameraList->field_23A) & 0xFFF;

				heading = pPlayer->GetEffectiveHeading();
				aimDelta = (turned - (i32)(u16)heading) & 0xFFF;
			}

			if (aimDelta <= 0x600) break;
			if (aimDelta >= 0xA00) break;

			anim = pPlayer->mAnim;
			if (anim == 0x15)
			{
				newAnim = 0x19;
			}
			else if (anim == 0x34)
			{
				newAnim = 0x5B;
			}
			else if (anim == 0x32 || anim == 0x33)
			{
				newAnim = 0x59;
			}
			else
			{
				break;
			}

			pPlayer->PlaySingleAnim(newAnim, 0, -1);
			pPlayer->mVel.vx = 0;
			pPlayer->mVel.vy = 0;
			pPlayer->mVel.vz = 0;
			pPlayer->LockTargetTorsoAngle();
			pPlayer->field_E1C = 0x400000;
			break;
		}

		// 0x4B269B. Wall jump wind up.
		case 64:
		{
			u16 anim = pPlayer->mAnim;

			pPlayer->field_EA6 = 0;

			if (anim == 0xD2)
			{
				if (pPlayer->mAnimFinished == 0) break;
				pPlayer->PlaySingleAnim(0xD3, 0, -1);
			}
			else if (anim == 0xDF)
			{
				if (pPlayer->mAnimFinished == 0) break;
				pPlayer->PlaySingleAnim(Rnd(2) != 0 ? 0xE7 : 0xE0, 0, -1);
			}
			else
			{
				break;
			}

			pPlayer->field_E84 = 0x70000;
			pPlayer->field_E80 = (i32)0xFFFC4000;
			if (pPlayer->field_DBC != 0)
			{
				pPlayer->field_E80 = pPlayer->field_DBC->mVel.vy - 0x3C000;
			}
			pPlayer->field_E1C = 2;
			pPlayer->field_E88 = 0xA8000;
			break;
		}

		// 0x4B2892. Dead, waiting out the respawn countdown.
		case 128:
		{
			i32 elapsed = 0;

			pPlayer->field_EA6 = 0;

			if (pPlayer->mAnimFinished != 0)
			{
				if (*gSpideyDeathTimer >= 0x78)
				{
					gInitRelatedTwo = 0;
					gLevelStatus = 2;
					Flash_Reset();
				}
				elapsed = *gSpideyDeathTimer + pPlayer->field_80;
			}

			*gSpideyDeathTimer = elapsed;
			break;
		}

		// 0x4B2F80. Starting a swing.
		case 0x100:
		{
			pPlayer->field_EA6 = 0;
			pPlayer->field_54C = 1;

			// ebx still holds the 13 loaded at 0x4B211A here
			if (pPlayer->mFrame < 13) break;

			pPlayer->field_E80 = (i32)0xFFFC4000;
			if (pPlayer->field_DBC != 0)
			{
				pPlayer->field_E80 = pPlayer->field_DBC->mVel.vy - 0x3C000;
			}

			pPlayer->field_E1C = 0x200;
			pPlayer->field_E20 = 0;
			SFX_PlayPos(9, &pPlayer->mPos, 0);

			pPlayer->field_E84 = 0x70000;
			pPlayer->field_E88 = 0xA8000;
			pPlayer->field_A8.vx = 0;
			pPlayer->field_A8.vy = (i16)0xF000;
			pPlayer->field_A8.vz = 0;

			if ((pPlayer->field_DAC.vx | pPlayer->field_DAC.vy | pPlayer->field_DAC.vz) != 0)
			{
				CVector normal;

				normal.vx = (pPlayer->mPos.vx - pPlayer->field_DAC.vx) >> 12;
				normal.vy = 0;
				normal.vz = (pPlayer->mPos.vz - pPlayer->field_DAC.vz) >> 12;
				VectorNormal(reinterpret_cast<VECTOR*>(&normal), reinterpret_cast<VECTOR*>(&normal));
				pPlayer->OrientToNormal(true, &normal);
				pPlayer->field_8E8 = 0;
			}
			else
			{
				pPlayer->OrientToNormal(false, &ZeroVector);
			}
			break;
		}

		default:
			if (pPlayer->field_E1C > 0x10000)
			{
				// 0x4B50AE, 1936 instructions. Not ported yet.
			}
			else if (pPlayer->field_E1C == 0x10000)
			{
				// 0x4B4E9B. Web-swing/attach wind up: three animations that
				// each fire a web on their release frame and drip webbing.
				u16 anim = pPlayer->mAnim;

				pPlayer->field_EA6 = 0;

				if (anim == 0x8B)
				{
					if (pPlayer->mAnimFinished != 0)
					{
						// 0x4B6597
						pPlayer->SwitchToStandMode();
						break;
					}

					// ebp still holds the 3 loaded at 0x4B2124
					if (pPlayer->mFrame < 3) break;

					if (pPlayer->field_552 == 0)
					{
						pPlayer->field_552 = 1;
						if (pPlayer->field_8ED != 0)
						{
							pPlayer->FireWeb(false, 0x384, &pPlayer->field_DC0, false, &gTrajectoryVector);
						}
						else
						{
							pPlayer->FireWeb(true, 0x384, &ZeroVector, false, &gTrajectoryVector);
						}
					}
					else if (pPlayer->CheckJump() != 0)
					{
						break;
					}

					if (pPlayer->mFrame > 5) break;
					pPlayer->CreateWebDrips(true, false);
					break;
				}

				if (anim == 0xFF || anim == 0x109)
				{
					if (pPlayer->mFrame < 5) break;

					if (pPlayer->field_552 == 0)
					{
						pPlayer->field_552 = 1;
						if (pPlayer->field_8ED != 0)
						{
							pPlayer->FireWeb(false, 0x384, &pPlayer->field_DC0, false, &gTrajectoryVector);
						}
						else
						{
							pPlayer->FireWeb(true, 0x384, &ZeroVector, false, &gTrajectoryVector);
						}
					}

					if (pPlayer->mFrame <= 0xB)
					{
						pPlayer->CreateWebDrips(true, true);
					}

					if (pPlayer->mAnimFinished == 0) break;

					pPlayer->PlaySingleAnim(anim == 0xFF ? 0x100 : 0x10A, 0, -1);
					break;
				}

				if (anim == 0x100 || anim == 0x10A)
				{
					if (pPlayer->CheckJump() != 0) break;
					if (pPlayer->mAnimFinished == 0) break;
					pPlayer->SwitchToStandMode();
				}
			}
			else if (pPlayer->field_E1C > 0x100)
			{
				// 0x4B307C, 1871 instructions. Not ported yet.
			}
			break;
	}

	// def_4B215D, 0x4B6E8C..0x4B86B1, 1725 instructions. Not ported yet.
}
