#include "spid_ai0.h"

#include "bit.h"
#include "baddy.h"
#include "camera.h"
#include "db.h"
#include "effects.h"
#include "flash.h"
#include "m3dcolij.h"
#include "m3dutils.h"
#include "m3dzone.h"
#include "powerup.h"
#include "reloc.h"
#include "switch.h"
#include "ps2m3d.h"
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

// The pose CPlayer::ApplyPose runs while the 4 flag is set on mFlags. Not
// named in the IDB; the address is only referenced from here.
static i16 * const gSpideyFixedPose = reinterpret_cast<i16*>(0x005564E4);

// The two Reloc_CallUserFunction effect names the fists spawn (both are
// plain strings in .rdata, read straight from the exe).
static char * const gFistEffectNameLeft = reinterpret_cast<char*>(0x005559C4);
static char * const gFistEffectNameRight = reinterpret_cast<char*>(0x005559CC);

// The camera/boss anchor object SpideyAI0 aims the lookaround camera at;
// real IDB name.
static i32 * const pgBossRelated = reinterpret_cast<i32*>(0x0056E998);
#define gBossRelated (*pgBossRelated)

static u16 * const gAnimFollowOnData = reinterpret_cast<u16*>(0x00555C6C);


// 0x00454040 M3dUtils_GetPartAngles: real name from tools/names.json, but the
// function is not decompiled and m3dutils.h has no declaration for it, so it
// is forwarded to the original here. cdecl, four arguments (the two call
// sites below are followed by an `add esp` that accounts for 4 dwords).
// @Bogus
static void gM3dUtils_GetPartAngles(CSuper *pSuper, i32 part, CSVector *pAngles, i32 a4)
{
	typedef void (*func_ptr)(CSuper*, i32, CSVector*, i32);
	func_ptr func = (func_ptr)0x00454040;
	func(pSuper, part, pAngles, a4);
}

// CPowerUp::TakeEffect (0x0046B860, real name from tools/names.json) is a
// __thiscall member the repo has not decompiled and powerup.h does not
// declare. Same member-function-pointer adapter baddy.cpp uses for
// gsub_4C9180, because this build's compiler rejects the __thiscall keyword.
struct SPowerUpTakeEffectAdapter
{
	u8 TakeEffect(CPlayer *pPlayer);
};

// CSwinger::SetRenderEnd (0x004F74B0) and CWeb::SetFirePos (0x004F6170) are
// real names from tools/names.json, but web.h declares neither, so both are
// reached through the same adapter trick. They are __thiscall with one
// CVector reference argument.
struct SSwingerRenderEndAdapter
{
	void SetRenderEnd(CVector &pos);
};

struct SWebFirePosAdapter
{
	void SetFirePos(CVector &pos);
};

// @Bogus
static void gCSwinger_SetRenderEnd(void *pSwinger, CVector &pos)
{
	typedef void (SSwingerRenderEndAdapter::*memfn)(CVector&);
	union { memfn m; void *p; } u;
	u.p = (void*)0x004F74B0;
	(reinterpret_cast<SSwingerRenderEndAdapter*>(pSwinger)->*u.m)(pos);
}

// @Bogus
static void gCWeb_SetFirePos(CWeb *pWeb, CVector &pos)
{
	typedef void (SWebFirePosAdapter::*memfn)(CVector&);
	union { memfn m; void *p; } u;
	u.p = (void*)0x004F6170;
	(reinterpret_cast<SWebFirePosAdapter*>(pWeb)->*u.m)(pos);
}

// spidey.h declares CPlayer::UpdateAndTrackCombo as returning void, but the
// original (0x004C7120) returns the combo state that state 0x800 switches on
// (0 = the combo ended, 2/3/6 = which follow-up move to start). The repo's
// copy is still a printf stub, so this forwards to the original rather than
// invent a return value. spidey.h's owner should change the declaration to
// `i32 UpdateAndTrackCombo(void)`.
struct SUpdateComboAdapter
{
	i32 UpdateAndTrackCombo(void);
};

// @Bogus
static i32 gCPlayer_UpdateAndTrackCombo(CPlayer *pPlayer)
{
	typedef i32 (SUpdateComboAdapter::*memfn)(void);
	union { memfn m; void *p; } u;
	u.p = (void*)0x004C7120;
	return (reinterpret_cast<SUpdateComboAdapter*>(pPlayer)->*u.m)();
}

// CSwinger's constructor (0x004F6F00, real name from tools/names.json) is not
// declared in web.h, so the allocation is spelled out and the constructor
// reached through an adapter, the same way the two web helpers above are.
struct SSwingerCtorAdapter
{
	void Construct(CVector *pAnchor, i32 length, CSVector *pAngles, CVector *pEnd);
};

// @Bogus
static void gCSwinger_ctor(void *pSwinger, CVector *pAnchor, i32 length,
	CSVector *pAngles, CVector *pEnd)
{
	typedef void (SSwingerCtorAdapter::*memfn)(CVector*, i32, CSVector*, CVector*);
	union { memfn m; void *p; } u;
	u.p = (void*)0x004F6F00;
	(reinterpret_cast<SSwingerCtorAdapter*>(pSwinger)->*u.m)(pAnchor, length, pAngles, pEnd);
}

// @Bogus
static u8 gCPowerUp_TakeEffect(CBody *pPowerUp, CPlayer *pPlayer)
{
	typedef u8 (SPowerUpTakeEffectAdapter::*memfn)(CPlayer*);
	union { memfn m; void *p; } u;
	u.p = (void*)0x0046B860;
	return (reinterpret_cast<SPowerUpTakeEffectAdapter*>(pPowerUp)->*u.m)(pPlayer);
}

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
	switch ((u32)pPlayer->field_E1C)
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

		// 0x4B50F1. The end of a scripted "stand up here" move: teleport to
		// hook 2 and face the stored normal.
		case 0x80000:
		{
			CVector hook;
			CVector up;

			pPlayer->field_EA6 = 0;
			if (pPlayer->mAnimFinished == 0) break;

			pPlayer->field_AD4 = 1;

			hook.vx = 0;
			hook.vy = 0;
			hook.vz = 0;
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hook), pPlayer, 2);
			pPlayer->mPos = hook;

			pPlayer->field_AC8.vx = 0;
			pPlayer->field_AC8.vy = 0x1000;
			pPlayer->field_AC8.vz = 0;

			// 0xB84: the CSVector angles the scripted move ends on.
			pPlayer->field_A8.vx = PLR_I16(pPlayer, 0xB84);
			pPlayer->field_A8.vy = PLR_I16(pPlayer, 0xB86);
			pPlayer->field_A8.vz = PLR_I16(pPlayer, 0xB88);

			up.vx = 0;
			up.vy = 0x1000;
			up.vz = 0;
			pPlayer->OrientToNormal(true, &up);
			pPlayer->LockTargetTorsoAngle();
			pPlayer->SwitchToStandMode();
			break;
		}

		// 0x4B5CFE. Reaching down for a pickup.
		case 0x100000:
		{
			u16 anim;
			CManipOb *pObject;

			pPlayer->field_EA6 = 0;

			if (pPlayer->mAnimFinished != 0)
			{
				pPlayer->SwitchToStandMode();
				break;
			}

			if (pPlayer->mHeldObject != 0) break;

			anim = pPlayer->mAnim;
			if (anim == 0)
			{
				// 0x4B6597
				pPlayer->SwitchToStandMode();
				break;
			}

			if (!((anim == 0xC4 && pPlayer->mFrame >= 0xA)
				|| (anim == 0xBE && pPlayer->mFrame >= 0xA)))
			{
				break;
			}

			// 0xE4C: SHandle of the object the reach was started for.
			pObject = reinterpret_cast<CManipOb*>(Mem_RecoverPointer(
				reinterpret_cast<SHandle*>(reinterpret_cast<char*>(pPlayer) + 0xE4C)));
			if (pObject == 0) break;

			if (Utils_CrapDist(pPlayer->mPos, pObject->mPos) >= 0x300) break;

			pPlayer->mHeldObject = pObject;
			pObject->Pickup();
			SFX_Play(7, 0x2000, 0);
			break;
		}

		// 0x4B5F4D. Rolling: the roll angle is driven straight off the frame
		// number, so the player spins exactly once over the animation.
		case 0x400000:
		{
			i32 frame;

			if (pPlayer->CheckGroundGone() != 0) break;

			pPlayer->mAnimSpeed = 0x20000;

			if ((u16)animJustFinished != 0xFFFF)
			{
				u8 *pAnimEntry;
				i32 numFrames;

				frame = pPlayer->mFrame;
				if (pPlayer->CheckJump() == 0) break;

				// Animations[region * 17] (export.h) is really
				// gCostumeRegionEntries[region * 17 + 2]; +0x100 is the
				// animation's frame count.
				pAnimEntry = reinterpret_cast<u8*>(Animations[(i32)pPlayer->mRegion * 17]);
				numFrames = *reinterpret_cast<i32*>(pAnimEntry + 0x100);

				pPlayer->field_548 = (i32)((u32)(frame << 11) / (u32)(numFrames - 1));
				pPlayer->OrientToNormal(false, &ZeroVector);
				pPlayer->field_548 = 0;
				break;
			}

			pPlayer->field_548 = 0x800;
			pPlayer->OrientToNormal(false, &ZeroVector);
			pPlayer->field_548 = 0;

			if (pPlayer->CheckJump() != 0) break;

			if ((u8)(pPlayer->field_E2E | pPlayer->field_E2D) == 0)
			{
				// 0x4B682F
				pPlayer->SwitchToStandMode();
				break;
			}

			// 0x4B6010
			pPlayer->field_E1C = 0x10;
			break;
		}

		// 0x4B5DDB. Knocked down / getting back up.
		case 0x800000:
		{
			u16 lastAnim = (u16)animJustFinished;
			u16 anim;

			pPlayer->field_EA6 = 0;

			// 0x54E: the flail-drop chain is running.
			if (PLR_U8(pPlayer, 0x54E) != 0)
			{
				if (lastAnim == 0xAF)
				{
					pPlayer->PlaySingleAnim(0xB1, 0, -1);
				}
				else if (lastAnim == 0xB1)
				{
					pPlayer->field_E1C = 4;
					PLR_U8(pPlayer, 0x54E) = 0;
					break;
				}
			}

			if (PLR_U16(pPlayer, 0xE8E) > 0x1E0)
			{
				gLevelStatus = 8;
			}

			if (pPlayer->mAnim == 0x11D
				&& pPlayer->mFrame >= 7
				&& (u32)(gTimerRelated - pPlayer->field_5B4) > 0x3C)
			{
				pPlayer->field_5AC = 5;
				pPlayer->field_5B0 = 0;
				pPlayer->field_5B4 = (i32)gTimerRelated;
				SFX_PlayPos(0x16, &pPlayer->mPos, 0);
			}

			if (lastAnim == 0xB4 || lastAnim == 0xAE)
			{
				pPlayer->field_36C = 0x1E;
				pPlayer->field_AE5 = 0;
				pPlayer->SwitchToStandMode();
				break;
			}

			if (lastAnim == 0xAA || lastAnim == 0xBB || lastAnim == 0xB5
				|| lastAnim == 0x11D)
			{
				// 0x4B6597
				pPlayer->SwitchToStandMode();
				break;
			}

			anim = pPlayer->mAnim;
			if (anim != 0xAF && anim != 0xB0) break;
			if ((pPlayer->mCollision & 2) == 0) break;

			pPlayer->PlaySingleAnim(0xB2, 0, -1);
			if (pPlayer->mHealth > 0) break;

			pPlayer->SwitchToDeathMode(false);
			break;
		}

		// 0x4B6DD8. Holding a web dome: pop it when the player struggles.
		case 0x20000000:
		{
			u8 *pPad;

			pPlayer->mRMinor = 0x96;

			if ((u16)animJustFinished == 0x11C)
			{
				pPlayer->SwitchToStandMode();
				pPlayer->mRMinor = 0x64;
				break;
			}

			if (pPlayer->mAnim == 0x11C) break;

			pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
			if ((u32)(gTimerRelated - pPlayer->field_374) <= 0x96
				&& pPad[0x120] == 0 && pPad[0x130] == 0 && pPad[0x100] == 0)
			{
				break;
			}

			pPlayer->PlaySingleAnim(0x11C, 0, -1);
			SFX_PlayPos(0x23, &pPlayer->mPos, 0);

			{
				CDome *pDome = reinterpret_cast<CDome*>(
					Mem_RecoverPointer(&pPlayer->field_AB8));

				if (pDome != 0)
				{
					pDome->Burst();
					pPlayer->field_AB8 = Mem_MakeHandle(0);
				}
			}
			break;
		}

		// 0x4B6DCA
		case 0x40000000:
			pPlayer->field_EA6 = 0;
			break;

		// 0x4B6D81. Pulling a switch.
		case 0x80000000u:
		{
			CSwitch *pSwitch;

			if (pPlayer->field_E20 != 0)
			{
				// 0x4B509C
				if ((u16)animJustFinished != 0xFFFF)
				{
					pPlayer->SwitchToStandMode();
				}
				break;
			}

			if (pPlayer->mFrame < 0xC) break;

			// 0xE54: SHandle of the switch being pulled.
			pSwitch = reinterpret_cast<CSwitch*>(Mem_RecoverPointer(
				reinterpret_cast<SHandle*>(reinterpret_cast<char*>(pPlayer) + 0xE54)));
			if (pSwitch != 0)
			{
				pSwitch->Flick();
			}

			pPlayer->field_E20++;
			break;
		}

		// 0x4B3C33. The frame a swing web is released on: spawn the CSwinger
		// that actually carries the player.
		case 0x200:
		{
			pPlayer->field_EA6 = 0;

			if (pPlayer->field_E20 == 0)
			{
				CameraList->field_12C = 2;
				pPlayer->PutCameraBehind(0x10);
				pPlayer->field_E20++;
			}

			if ((pPlayer->field_504 & 0x200) != 0
				&& (u32)(gTimerRelated - (i32)pPlayer->field_500) < 6)
			{
				if (pPlayer->ShouldPlayerDropFlail() != 0)
				{
					pPlayer->PlaySingleAnim(0xAF, 0, -1);
					pPlayer->mVel.vz = 0;
					pPlayer->mVel.vx = 0;
					pPlayer->field_E1C = 0x800000;
					PLR_U8(pPlayer, 0x54E) = 0;
				}
				else
				{
					pPlayer->PlaySingleAnim(0xD8, 0, -1);
					pPlayer->field_E1C = 4;
				}
				pPlayer->field_54D = 0;
			}

			if (pPlayer->CheckStickToCeiling() != 0) break;

			if (pPlayer->mVel.vy < 0)
			{
				pPlayer->field_E2D = (char)0x81;
				pPlayer->field_E2E = 0;
				pPlayer->field_EBC = 0x10;
				break;
			}

			pPlayer->field_E1C = 0x400;
			pPlayer->PlaySingleAnim(0x113, 0, -1);
			SFX_PlayPos(0x19, &pPlayer->mPos, 0);
			pPlayer->DecreaseWebbing(0x80);

			PLR_I32(pPlayer, 0xD7C) = (i32)Utils_Dist(pPlayer->mPos, pPlayer->field_D64);
			pPlayer->CalculateSwingWebParameters(&pPlayer->field_D64);

			print_if_false(pPlayer->field_E64 == 0, "already swinging");

			{
				void *pMem = CItem::operator new(0x190);

				if (pMem != 0)
				{
					gCSwinger_ctor(pMem, &pPlayer->field_D64,
						PLR_I32(pPlayer, 0xD7C), &pPlayer->field_D80,
						&pPlayer->field_DA0);
				}

				pPlayer->field_E64 = reinterpret_cast<i32*>(pMem);
				// the original writes this even when the allocation failed
				*reinterpret_cast<i32*>(reinterpret_cast<char*>(pMem) + 0xF8) =
					(u8)pPlayer->field_5E8;
			}

			SFX_Play(Rnd(3) + 0x15, 0x2000, 0);
			break;
		}

		// 0x4B30AE. Punching: hand the tick to the combo tracker and start
		// whatever follow-up move it picks.
		case 0x800:
		{
			u16 attackKind;
			i32 comboState;
			CBody *pAimAt;

			pPlayer->field_EA6 = 0;
			if (pPlayer->CheckGroundGone() != 0) break;
			if (pPlayer->CheckJump() != 0) break;

			attackKind = pPlayer->field_8FC;
			if (attackKind == 0 || attackKind == 1)
			{
				u8 *pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);

				if (pPad[0x110] != 0
					&& (pPad[0x120] != 0 || pPad[0x130] != 0)
					&& (u32)(gTimerRelated - pPlayer->field_898) <= 4)
				{
					CBody *pTarget = pPlayer->SelectTargetBaddy(0xBE, -0x1000, 0x1000, 0);

					pPlayer->field_DD8 = Mem_MakeHandle(pTarget);
					pPlayer->field_DE0 = (i32)gTimerRelated;
					pPlayer->field_E1C = 0x2000000;
					pPlayer->PlaySingleAnim(0x78, 0, -1);
					break;
				}
			}

			comboState = gCPlayer_UpdateAndTrackCombo(pPlayer);
			if (comboState == 0)
			{
				pPlayer->field_548 = 0;
				pPlayer->SwitchToStandMode();
				break;
			}

			if (comboState == 2)
			{
				pAimAt = pPlayer->field_DCC;
				pPlayer->field_8F8 = 1;
				pPlayer->field_552 = 0;
				pPlayer->field_E1C = 0x4000;
				pPlayer->field_8ED = 0;
			}
			else if (comboState == 3)
			{
				pAimAt = pPlayer->field_DCC;
				pPlayer->field_8F8 = 2;
				pPlayer->field_552 = 0;
				pPlayer->field_E1C = 0x8000;

				if (pPlayer->field_E2E > 0)
				{
					pPlayer->field_544 = 2;
				}
				else if (pPlayer->field_E2E < 0)
				{
					pPlayer->field_544 = 1;
				}
				else
				{
					pPlayer->field_544 = 0;
				}
				pPlayer->field_8ED = 0;
			}
			else if (comboState == 6)
			{
				pAimAt = pPlayer->field_DCC;
				pPlayer->field_8F8 = 2;
				pPlayer->field_552 = 0;
				pPlayer->field_E1C = 0x10000;
				pPlayer->field_8ED = 0;
			}
			else
			{
				break;
			}

			if (pAimAt == 0)
			{
				pPlayer->LockTargetTorsoAngle();
			}
			else
			{
				pPlayer->SetTargetTorsoAngleToThisPoint(&pAimAt->mPos);
			}
			PLR_U8(pPlayer, 0xAE) &= 0xFE;
			break;
		}

		// 0x4B3DF9. Landing out of a surface transition: snap to hook 2 and
		// then slide the stored distance along field_C84.
		case 0x1000:
		{
			u16 lastAnim = (u16)animJustFinished;
			i32 slide;
			i16 nx;
			i16 ny;
			i16 nz;
			CVector hook;
			CVector normal;

			pPlayer->field_EA6 = 0;

			if (lastAnim == 0xFFFF)
			{
				// 0x4B3FB8: eat the queued slide 8 units at a time
				i32 left = PLR_I32(pPlayer, 0xC58);

				if (left == 0) break;

				if (left >= 8)
				{
					pPlayer->mPos += (pPlayer->field_C6C * 8);
					PLR_I32(pPlayer, 0xC58) = left - 8;
				}
				else
				{
					pPlayer->mPos += (pPlayer->field_C6C * left);
					PLR_I32(pPlayer, 0xC58) = 0;
				}
				break;
			}

			if (lastAnim == 0x57)
			{
				slide = 0xC;
			}
			else if (lastAnim == 0x58)
			{
				slide = 0x10;
			}
			else
			{
				slide = (lastAnim == 0x3C) ? 0x42 : 0x37;
			}

			nx = PLR_I16(pPlayer, 0xB84);
			ny = PLR_I16(pPlayer, 0xB86);
			nz = PLR_I16(pPlayer, 0xB88);

			PLR_U8(pPlayer, 0xAD6) = 1;
			pPlayer->field_A8.vx = nx;
			pPlayer->field_A8.vy = ny;
			pPlayer->field_A8.vz = nz;

			if (ny < (i16)0xF5D8)
			{
				pPlayer->field_8E9 = 0;
				pPlayer->field_8E8 = 0;
				pPlayer->field_AD4 = 0;
				pPlayer->SwitchToStandMode();

				if (lastAnim != 0x57 && lastAnim != 0x58)
				{
					pPlayer->PlaySingleAnim(0x14, 0, -1);
					pPlayer->field_E1C = 1;
				}
			}
			else
			{
				pPlayer->field_E1C = 0x10;

				if (ny > 0xD48)
				{
					if (pPlayer->field_ADA == 0)
					{
						pPlayer->field_AD8 = 1;
					}
					pPlayer->field_8E8 = 0;
					pPlayer->field_8E9 = 1;
				}
				else
				{
					if (pPlayer->field_8E8 == 0 && pPlayer->field_ADB != 0)
					{
						pPlayer->field_AD9 = 1;
					}
					pPlayer->field_8E8 = 1;
					pPlayer->field_8E9 = 0;
				}
			}

			hook.vx = 0;
			hook.vy = 0;
			hook.vz = 0;
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hook), pPlayer, 2);
			pPlayer->mPos = hook;

			normal.vx = -pPlayer->field_C84.vx;
			pPlayer->field_AC8.vx = normal.vx;
			normal.vy = -pPlayer->field_C84.vy;
			pPlayer->field_AC8.vy = normal.vy;
			normal.vz = -pPlayer->field_C84.vz;
			pPlayer->field_AC8.vz = normal.vz;

			pPlayer->OrientToNormal(true, &normal);
			pPlayer->LockTargetTorsoAngle();
			pPlayer->mPos += (pPlayer->field_C84 * slide);
			break;
		}

		default:
			if ((u32)pPlayer->field_E1C > 0x10000)
			{
				// 0x4B50AE, remaining sub-states. Not ported yet.
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
			else if ((u32)pPlayer->field_E1C > 0x100)
			{
				// 0x4B307C, sub-states 0x200..0x8000. Not ported yet.
			}
			break;
	}


	// -----------------------------------------------------------------------
	// def_4B215D, 0x4B6E8C..0x4B86B1. The shared tail: every state's `break`
	// above lands here, and so does the state-machine default.
	// -----------------------------------------------------------------------
	{
		u8 *pPad = reinterpret_cast<u8*>(pPlayer->field_E0C);
		i32 camYaw;
		i32 state;
		u8 anyMove;
		i32 torsoSpeed;
		i16 heading;
		i32 headingBefore;
		i32 turnLeft;
		CVector accel;
		i32 bAccelSet;
		CBody *pPlatform;
		CVector prevPos;
		i32 riseSpeed;
		SLineInfo lineInfo;
		CVector hookA;
		CVector hookB;

		// --- camera facing, 0x4B6E8C ---
		// 0xAD6: "the camera may be auto-steered this tick".
		if (PLR_U8(pPlayer, 0xAD6) != 0
			&& PLR_U8(pPlayer, 0x8EC) == 0
			&& pPlayer->field_54C == 0)
		{
			if (pPlayer->field_8E8 != 0)
			{
				pPlayer->field_551 = 0;
				if ((pPlayer->field_E1C & 0x3000) == 0)
				{
					pPlayer->PutCameraBehind(0x1E);
				}
			}
			// 0xE70: SHandle of the object the camera locks onto.
			else if (pPad[0x50] != 0 && PLR_I32(pPlayer, 0xE70) != 0)
			{
				CSVector aim;
				void *pLockTarget = Mem_RecoverPointer(
					reinterpret_cast<SHandle*>(reinterpret_cast<char*>(pPlayer) + 0xE70));

				Utils_CalcAim(&aim, &pPlayer->mPos,
					reinterpret_cast<CVector*>(reinterpret_cast<char*>(pLockTarget) + 8));
				CameraList->SetCamAngle(aim.vy, 0);
				pPlayer->field_551 = 1;
			}
			else if (pPlayer->field_EA6 == 0)
			{
				pPlayer->field_551 = 0;

				// the original calls this and throws the result away
				pPlayer->GetEffectiveHeading();

				if (pPad[0x41] != 0)
				{
					pPad[0x41] = 0;
					pPlayer->field_201 = 0;
					pPlayer->PutCameraBehind(0);
					pPlayer->field_56C = 0;
				}
				else if (pPad[0x111] != 0
					&& pPlayer->field_8EA == 0
					&& (pPlayer->field_E1C & 0x2001C001) != 0)
				{
					pPad[0x111] = 0;
					// 0x2D1: cleared with the camera reset, like field_201.
					PLR_U8(pPlayer, 0x2D1) = 0;
					pPlayer->PutCameraBehind(0);
				}
				else if ((u8)(pPlayer->field_E2E | pPlayer->field_E2D) != 0
					&& pPlayer->field_E1C == 0x10)
				{
					i16 camAngle = CameraList->field_23A;
					i16 want = (i16)(pPlayer->field_E32 + camAngle);
					i32 delta = ((i32)camAngle - (i32)want) & 0xFFF;

					if (delta < 0x4B0 || delta > 0xB50)
					{
						CameraList->SetCamAngle(want, 0x3C);
					}
					else if (delta > 0x7F0 && delta < 0x810)
					{
						CameraList->SetCamAngle(CameraList->field_236, 0);
					}
					else
					{
						// maps 0x4B0..0xB50 onto a full 0..0x800 sweep
						i32 t = ((delta - 0x4B0) << 11) / 0x6A0;
						i32 pitch = ((i32)Sine(t & 0xFFF) * 128 >> 12) + 0x3C;

						CameraList->SetCamAngle(want, (u16)pitch);
					}
				}
				else
				{
					CameraList->SetCamAngle(CameraList->field_236, 0);
				}
			}
			else
			{
				if (pPad[0x41] != 0)
				{
					pPad[0x41] = 0;
					pPlayer->field_201 = 0;
					pPlayer->PutCameraBehind(0);
					pPlayer->field_56C = 0;
					pPlayer->field_551 = 1;
				}

				if (pPlayer->field_551 == 0)
				{
					pPlayer->PutCameraBehind(0x14);
					pPlayer->field_551 = 1;
				}
			}
		}

		// --- torso aim, 0x4B70D5 ---
		anyMove = (u8)(pPlayer->field_E2E | pPlayer->field_E2D);
		torsoSpeed = 0;
		if (anyMove != 0)
		{
			i16 aim = pPlayer->field_E32;

			if (aim > 0x100 && aim < 0xF00)
			{
				torsoSpeed = ((i32)aim - 0x100) / 512 + 2;
			}
			else
			{
				torsoSpeed = 1;
			}
		}

		camYaw = (CameraList != 0) ? (i32)CameraList->field_23A : 0;
		state = pPlayer->field_E1C;

		if ((state & 0x10) != 0)
		{
			if (pPlayer->field_8E8 != 0)
			{
				pPlayer->SetTargetTorsoAngle(pPlayer->field_E32, false);
			}
			else
			{
				pPlayer->SetTargetTorsoAngle((i16)(pPlayer->field_E32 + camYaw),
					pPlayer->field_1AC != 0);
			}
		}
		else if (state == 1)
		{
			if (anyMove == 0)
			{
				pPlayer->LockTargetTorsoAngle();
			}
		}
		else if ((state & 0x4E) != 0)
		{
			if (pPlayer->field_AE6 != 0 || torsoSpeed == 0)
			{
				pPlayer->LockTargetTorsoAngle();
			}
			else
			{
				pPlayer->SetTargetTorsoAngle((i16)(pPlayer->field_E32 + camYaw), false);
			}
		}

		// --- heading and the scripted turn, 0x4B717F ---
		heading = pPlayer->GetEffectiveHeading();
		turnLeft = pPlayer->field_DF8;
		pPlayer->mAngles.vy = heading;
		headingBefore = (i32)heading;
		pPlayer->field_548 = headingBefore;

		if (turnLeft != 0)
		{
			i32 dt = pPlayer->field_80;

			if (turnLeft > dt)
			{
				i16 step = (i16)(pPlayer->field_DF4 * dt);

				pPlayer->mAngles.vy = (i16)(((i32)step + headingBefore) & 0xFFF);
				pPlayer->field_DF8 = turnLeft - dt;
			}
			else
			{
				// 0xDF0: the angle the scripted turn ends on.
				pPlayer->field_DF8 = 0;
				pPlayer->mAngles.vy = PLR_I16(pPlayer, 0xDF0);
			}

			if (pPlayer->field_A8.vy > 0xD48)
			{
				i32 idx = (headingBefore - (i32)pPlayer->mAngles.vy) & 0xFFF;
				CVector dir(Sine(idx), 0, Cosine(idx));

				gte_SetRotMatrix(&pPlayer->mTransform);
				gte_ldlvl(reinterpret_cast<VECTOR*>(&dir));
				gte_rtir();
				gte_stlvnl(reinterpret_cast<VECTOR*>(&pPlayer->field_AC8));
			}
		}

		pPlayer->field_548 = (i32)pPlayer->mAngles.vy - pPlayer->field_548;

		pPlayer->mAcc.vz = 0;
		pPlayer->mAcc.vx = 0;
		if (pPlayer->field_E1C == 0x1000000)
		{
			// dead store, the next test overwrites it either way
			pPlayer->mAcc.vy = 0;
		}
		if (pPlayer->field_AD4 != 0 || pPlayer->mAnim == 0x115)
		{
			pPlayer->mAcc.vy = 0;
		}
		else
		{
			pPlayer->mAcc.vy = 0xA000;
		}

		// --- move input into an acceleration, 0x4B72AD ---
		bAccelSet = 0;
		{
			MATRIX identityA;
			MATRIX identityB;

			if (pPlayer->field_8E8 != 0)
			{
				gte_SetRotMatrix(reinterpret_cast<MATRIX*>(
					reinterpret_cast<char*>(pPlayer) + 0x89C));
			}
			else
			{
				print_if_false(CameraList != 0, "no camera");
				M3dMaths_SetIdentityRotation(&identityA);
				M3dMaths_SetIdentityRotation(&identityB);
				gte_SetRotMatrix(&identityB);
			}

			accel = (pPlayer->mVel >> 6);
			gte_ldlvl(reinterpret_cast<VECTOR*>(&accel));
			gte_rtir();
			gte_stlvnl(reinterpret_cast<VECTOR*>(&accel));
			accel <<= 6;

			{
				// 0xEF4/0xEF8: the "perpendicularising" run around a boss.
				u8 wasPerpendicularising = pPlayer->field_EF4;
				i32 moveMag = 0;
				i32 moveScale = 0;
				i32 yaw = 0;

				pPlayer->field_EF4 = 0;

				if (pPlayer->field_8EA == 0
					&& pPlayer->field_AE4 != 0
					&& (pPlayer->field_E1C & 0xBFF7F8C1) == 0)
				{
					if ((u8)(pPlayer->field_E2E | pPlayer->field_E2D) != 0)
					{
						i32 sideways = ((i32)pPlayer->field_E2E << 12) / 64;
						i32 forwards = ((i32)pPlayer->field_E2D << 12) / 64;

						if (sideways < 0) sideways = -sideways;
						if (forwards < 0) forwards = -forwards;

						moveMag = sideways + forwards;
						if (moveMag > 0x1000)
						{
							moveMag = 0x1000;
						}

						if (pPlayer->mAnim == 0x32 || pPlayer->mAnim == 0x33
							|| pPlayer->mAnim == 0x34)
						{
							i32 speed = ((moveMag >> 6) << 16) >> 6;

							pPlayer->mAnimSpeed = speed;
							if (speed < 0x8000)
							{
								pPlayer->mAnimSpeed = 0x8000;
							}
						}

						if (pPlayer->field_AD4 != 0)
						{
							moveScale = 0x1A;
						}
						else if (pPlayer->mHeldObject != 0)
						{
							moveScale = ((pPlayer->mHeldObject->field_10C >> 3) & 1) ? 0x0A : 0x18;
						}
						else
						{
							moveScale = 0x28;
							moveMag = 0x1000;
						}

						if (pPlayer->field_8E8 != 0)
						{
							i32 idx;
							i32 c;

							heading = pPlayer->GetEffectiveHeading();
							idx = ((i32)heading - (i32)(u16)pPlayer->field_E32) & 0xFFF;
							c = Cosine(idx);
							if (c < 0) c = -c;
							c = (c * moveMag) >> 12;
							c = c * moveScale;
							c = c * pPlayer->field_EBC;
							accel.vz = -(c / 16);
							bAccelSet = 1;
						}
						else
						{
							heading = pPlayer->GetEffectiveHeading();
							yaw = (i32)heading;

							if (CameraList->mCameraMode == CAMERAMODE_LOOKAROUND)
							{
								CVector *pAnchor = reinterpret_cast<CVector*>(
									reinterpret_cast<char*>(gBossRelated) + 8);
								i32 dist = Utils_XZDist(&pPlayer->mPos, pAnchor);

								if (dist > 0x300)
								{
									CSVector aim;

									Utils_CalcAim(&aim, &pPlayer->mPos, pAnchor);
									yaw = ((i32)(u16)pPlayer->field_E32 + (i32)aim.vy) & 0xFFF;

									if (pPlayer->field_E32 == 0x400
										|| pPlayer->field_E32 == (i16)0xC00)
									{
										i32 radius = pPlayer->GetPerpendicularisationRadius();
										u8 keepDistance;

										if (dist > radius)
										{
											dist -= pPlayer->field_80;
											keepDistance = 0;
										}
										else
										{
											keepDistance = wasPerpendicularising;
										}

										pPlayer->field_EF4 = 1;
										if (keepDistance == 0)
										{
											pPlayer->field_EF8 = dist;
										}
									}
								}
							}
							else if ((pPlayer->field_E1C & 6) != 0 && pPlayer->field_E8C == 0)
							{
								yaw = ((i32)(u16)pPlayer->field_E32 + camYaw) & 0xFFF;
							}

							if (pPlayer->field_AE5 == 0)
							{
								i32 idx = yaw & 0xFFF;
								i32 ebc = pPlayer->field_EBC;
								i32 c = (((i32)Cosine(idx) * moveMag) >> 12) * ebc * moveScale;
								i32 sn = (((i32)Sine(idx) * moveMag) >> 12) * ebc * moveScale;

								accel.vz = -(c / 16);
								accel.vx = -(sn / 16);
								bAccelSet = 1;
							}
						}
					}
					else if ((pPlayer->field_E1C & 0x40040004) == 0)
					{
						accel.vx = 0;
						accel.vz = 0;
						bAccelSet = 1;
					}
				}
			}

			// --- apply it, 0x4B763C ---
			if (pPlayer->field_8E8 != 0)
			{
				gte_SetRotMatrix(&pPlayer->mTransform);
			}
			else
			{
				gte_SetRotMatrix(&identityA);
			}

			if (bAccelSet != 0)
			{
				pPlayer->mVel = (accel >> 6);
				gte_ldlvl(reinterpret_cast<VECTOR*>(&pPlayer->mVel));
				gte_rtir();
				gte_stlvnl(reinterpret_cast<VECTOR*>(&pPlayer->mVel));
				pPlayer->mVel <<= 6;
			}
		}

		// --- ride the platform under us, 0x4B76C0 ---
		pPlatform = pPlayer->field_DBC;
		if (pPlatform != 0)
		{
			i32 dz = pPlatform->mVel.vz;
			bool moved;

			prevPos = pPlayer->mPos;

			if (PLR_U16(pPlayer, 0xE8E) != 0)
			{
				i32 dx = pPlatform->mVel.vx;

				moved = ((dx | dz) != 0);
				if (moved)
				{
					pPlayer->mPos.vx += dx;
				}
			}
			else
			{
				i32 dy = pPlatform->mVel.vy;
				i32 dx = pPlatform->mVel.vx;

				moved = ((dy | dx | dz) != 0);
				if (moved)
				{
					pPlayer->mPos.vx += dx;
					pPlayer->mPos.vy += dy;
				}
			}

			if (moved)
			{
				pPlayer->mPos.vz += dz;

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
				lineInfo.StartCoords = prevPos;
				lineInfo.EndCoords = pPlayer->mPos;

				M3dColij_InitLineInfo(&lineInfo);
				M3dZone_LineToItem(&lineInfo, 1);

				if (lineInfo.pItem != 0)
				{
					pPlayer->mPos = prevPos;
				}
			}
		}

		// --- vertical impulse, 0x4B77B8 ---
		if ((pPlayer->mCollision & 0x100) != 0)
		{
			pPlayer->field_E88 = 0;
		}

		riseSpeed = pPlayer->field_E84;
		if (riseSpeed > 0 || pPlayer->field_E88 > 0)
		{
			if (riseSpeed < 0x10000 && pPlayer->field_E88 == 0)
			{
				pPlayer->mVel.vy = (((pPlayer->field_E80 >> 12) * riseSpeed) >> 4) & (i32)0xFFFFF000;
			}
			else
			{
				pPlayer->mVel.vy = pPlayer->field_E80;
			}
		}

		if ((i32)(riseSpeed & (i32)0xFFFF0000) > 0 && pPlayer->field_E1C == 0x200)
		{
			pPlayer->mVel.vy = pPlayer->field_E80;
		}

		pPlayer->ProcessSFXArray();

		if (pPlayer->field_E88 > 0)
		{
			pPlayer->field_E88 -= (pPlayer->field_80 << 16) / 2;
			if (pPlayer->field_E88 < 0)
			{
				pPlayer->field_E88 = 0;
			}
		}

		if (pPlayer->field_E84 > 0)
		{
			pPlayer->field_E84 -= (pPlayer->field_80 << 16) / 2;
			if (pPlayer->field_E84 < 0)
			{
				pPlayer->field_E84 = 0;
			}
		}

		if (pPlayer->field_AD4 != 0 || (pPlayer->field_E1C & 0x3E3FF780) != 0)
		{
			pPlayer->field_5AC = 0;
		}

		// --- ran into a baddy, 0x4B78A8 ---
		if (pPlayer->field_E8 != pPlayer->mPos)
		{
			CVector hitPos;

			if ((pPlayer->field_E1C & 0x1000000) != 0 && pPlayer->field_8D8 == 0)
			{
				CVector lineStart;
				CVector lineEnd;
				CBody *pIgnore = 0;
				CBody *pHit;

				lineStart.vx = pPlayer->field_E8.vx;
				lineStart.vy = pPlayer->mPos.vy;
				lineStart.vz = pPlayer->field_E8.vz;
				lineEnd = pPlayer->mPos + (pPlayer->mPos - pPlayer->field_E8);

				for (;;)
				{
					pHit = M3dColij_LineToSphere(&lineStart, &lineEnd, &hitPos,
						reinterpret_cast<CBody*>(BaddyList), pIgnore, 0x1000);
					if (pHit == 0)
					{
						break;
					}

					{
						i32 kind = (i32)pHit->mType - 304;

						if ((u32)kind > 0x14)
						{
							break;
						}

						// jpt_4B7987: the 21 mTypes 304..324 pick one of three
						// paths through byte_4B8778.
						if (kind == 1 || kind == 11)
						{
							// types 305 and 315: pass straight through, carry
							// on from the hit point ignoring this one
							lineStart = hitPos;
							pIgnore = pHit;
							continue;
						}

						if (kind == 4 || kind == 5 || kind == 7 || kind == 12
							|| kind == 14 || kind == 15 || kind == 17
							|| kind == 18 || kind == 19)
						{
							// types 308, 309, 311, 316, 318, 319, 321..323
							break;
						}
					}

					// everything else: smash into it
					{
						SHitInfo hit;

						hit.field_C.vx = 0;
						hit.field_C.vy = 0;
						hit.field_C.vz = 0;
						hit.field_0 = 0x1E;
						hit.field_8 = (u16)pPlayer->GetDamageInflictedFromDifficulty(0x14);
						hit.field_4 = 2;
						hit.field_C.vx = -pPlayer->field_C6C.vx;
						hit.field_C.vy = 0;
						hit.field_C.vz = -pPlayer->field_C6C.vz;
						hit.field_18 = 0x200;
						hit.field_1A = 0xF;
						VectorNormal(reinterpret_cast<VECTOR*>(&hit.field_C),
							reinterpret_cast<VECTOR*>(&hit.field_C));

						pHit->Hit(&hit);
						SFX_PlayPos(0x10, &pPlayer->mPos, 0);

						// 0x4B8BE0, inlined: the knockback timer pair.
						pPlayer->field_534 = 0x168;
						pPlayer->field_52C = (pPlayer->field_528 + 0xB) << 10;

						pPlayer->mVel.vz = 0;
						pPlayer->mVel.vx = 0;
						pPlayer->field_8D8 = 1;
					}
					break;
				}
			}
			else if ((pPlayer->field_E1C & 0x14) != 0)
			{
				CBody *pHit = M3dColij_LineToSphere(&pPlayer->field_E8, &pPlayer->mPos,
					&hitPos, reinterpret_cast<CBody*>(BaddyList), 0, 0x1000);

				if (pHit != 0)
				{
					pPlayer->CollideWithObject(pHit);
				}
			}
		}

		// --- pick up power-ups, 0x4B7A74 ---
		{
			CBody *pPowerUp = reinterpret_cast<CBody*>(PowerUpList);

			while (pPowerUp != 0)
			{
				if ((pPowerUp->mCBodyFlags & 0x40) == 0
					&& Utils_CrapDist(pPlayer->mPos, pPowerUp->mPos) < (i32)pPowerUp->mRMinor)
				{
					if (pPowerUp->mType == 0xB)
					{
						if ((pPlayer->field_E1C & 0x11) != 0)
						{
							gCPowerUp_TakeEffect(pPowerUp, pPlayer);
						}
					}
					else if (gCPowerUp_TakeEffect(pPowerUp, pPlayer) == 0)
					{
						// 0x4B8C00, inlined
						PLR_U8(pPowerUp, 0x124) = 1;

						// 0x370: "the pickup jingle is free to play again".
						if (PLR_U8(pPlayer, 0x370) != 0)
						{
							u16 kind = pPowerUp->mType;

							if (kind == 8 || (kind > 0xD && kind <= 0x10))
							{
								SFX_Play(0x28, 0x2000, 0);
								PLR_U8(pPlayer, 0x370) = 0;
							}
						}
					}
					break;
				}

				pPowerUp = reinterpret_cast<CBody*>(pPowerUp->mNextItem);
			}

			if (pPowerUp == 0)
			{
				PLR_U8(pPlayer, 0x370) = 1;
			}
		}

		// --- push away from baddies while standing, 0x4B7B1E ---
		if ((pPlayer->field_E1C & 1) != 0 && pPlayer->field_E6C == 0)
		{
			CBody *pBaddy = reinterpret_cast<CBody*>(BaddyList);

			while (pBaddy != 0)
			{
				if (pBaddy->mRMinor != 0)
				{
					CVector away = pPlayer->mPos - pBaddy->mPos;
					i32 dist = away.Length();

					if (dist < (((i32)pBaddy->mRMinor * 3) << 10 >> 12))
					{
						i32 strength = 10;

						away.vy = 0;
						away.vx = away.vx / dist;
						away.vz = away.vz / dist;
						pPlayer->mVel += (away * strength);
					}
				}

				pBaddy = reinterpret_cast<CBody*>(pBaddy->mNextItem);
			}
		}

		// --- trigger-zone sweep along the movement, 0x4B7BF0 ---
		{
			// function-local static in the original (guard byte 0x6A7F24,
			// object 0x6A7EF8); the registered atexit handler is a bare retn.
			static CVector sLastMoveDirection;

			CVector moveDir = (pPlayer->mPos - pPlayer->field_E8) >> 12;

			if (pPlayer->field_E1C != 0x80)
			{
				CVector normal;
				CVector back;

				if ((moveDir.vz | moveDir.vy | moveDir.vx) != 0)
				{
					sLastMoveDirection = moveDir;
				}

				VectorNormal(reinterpret_cast<VECTOR*>(&sLastMoveDirection),
					reinterpret_cast<VECTOR*>(&normal));

				back = pPlayer->field_E8 - (normal * 0x10);

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
				lineInfo.StartCoords = back;
				lineInfo.EndCoords = pPlayer->mPos;

				M3dColij_InitLineInfo(&lineInfo);

				TriggerCollisionCheck = 0;
				lineInfo.RecordTriggerZoneHits = 1;
				M3dZone_LineToItem(&lineInfo, 1);
				TriggerCollisionCheck = 0;
			}
		}

		// --- build this frame's pose, 0x4B7D5A ---
		if ((pPlayer->mFlags & 4) != 0)
		{
			pPlayer->ApplyPose(gSpideyFixedPose);
		}
		else
		{
			M3d_BuildTransform(pPlayer);
		}

		pPlayer->DoShadowCheck();

		if (pPlayer->field_94D != 0)
		{
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pPlayer->field_91C), pPlayer, 6);
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pPlayer->field_928), pPlayer, 5);
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pPlayer->field_934), pPlayer, 1);
			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pPlayer->field_940), pPlayer, 0);
		}

		pPlayer->UpdateTrails();

		if (pPlayer->field_584 != 0)
		{
			CVector hookPos;

			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hookPos), pPlayer, 5);
			pPlayer->field_584->SetPos(hookPos);
		}

		if (pPlayer->field_588 != 0)
		{
			CVector hookPos;

			M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hookPos), pPlayer, 6);
			pPlayer->field_588->SetPos(hookPos);
		}

		// --- head look-at, 0x4B7E32 ---
		// 0xA80: "the head tracks something this tick".
		if (PLR_U8(pPlayer, 0xA80) != 0)
		{
			CVector target;
			bool bHaveTarget = true;

			if (pPlayer->field_E00 != 0)
			{
				Trig_GetPosition(&target, pPlayer->field_E00);
			}
			else if (pPlayer->field_8E8 != 0
				|| pPlayer->field_8E9 != 0
				|| pPlayer->field_DCC == 0
				|| pPlayer->field_8EA != 0)
			{
				bHaveTarget = false;
			}
			else
			{
				target = pPlayer->field_DCC->mPos;
			}

			if (bHaveTarget)
			{
				CVector headHook;
				CVector look;

				M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&headHook), pPlayer, 8);

				look.vx = (target.vx - headHook.vx) >> 12;
				look.vy = (target.vy - headHook.vy) >> 12;
				look.vz = (target.vz - headHook.vz) >> 12;

				gte_SetRotMatrix(reinterpret_cast<MATRIX*>(
					reinterpret_cast<char*>(pPlayer) + 0x89C));
				gte_ldlvl(reinterpret_cast<VECTOR*>(&look));
				gte_rtir();
				gte_stlvnl(reinterpret_cast<VECTOR*>(&look));
				look <<= 12;

				Utils_CalcAim(reinterpret_cast<CSVector*>(
						reinterpret_cast<char*>(pPlayer) + 0xE04),
					&ZeroVector, &look);

				// clamp the head yaw out of the 0x200..0xE00 dead zone
				{
					i16 *pHeadYaw = reinterpret_cast<i16*>(
						reinterpret_cast<char*>(pPlayer) + 0xE04);
					i16 *pHeadPitch = reinterpret_cast<i16*>(
						reinterpret_cast<char*>(pPlayer) + 0xE06);
					i32 yawVal = *pHeadYaw;
					i32 pitchVal = *pHeadPitch;

					if (yawVal > 0x800)
					{
						if (yawVal < 0xE00) *pHeadYaw = 0xE00;
					}
					else if (yawVal > 0x200)
					{
						*pHeadYaw = 0x200;
					}

					if (pitchVal > 0x800)
					{
						if (pitchVal < 0xD00) *pHeadPitch = 0xD00;
					}
					else if (pitchVal > 0x300)
					{
						*pHeadPitch = 0x300;
					}
				}
			}
		}
		else
		{
			PLR_I16(pPlayer, 0xE04) = 0;
			PLR_I16(pPlayer, 0xE06) = 0;
		}

		// --- ease the head joint towards the target angles, 0x4B7FB9 ---
		if (pPlayer->mpJoints != 0)
		{
			i16 *pJointAngles = reinterpret_cast<i16*>(
				reinterpret_cast<char*>(pPlayer->mpJoints) + 0x24);
			i32 axis;

			for (axis = 0; axis < 2; axis++)
			{
				i16 cur = pJointAngles[axis];
				i32 diff = (i32)PLR_I16(pPlayer, 0xE04 + axis * 2) - (i32)cur;

				if (diff > 0x800)
				{
					diff -= 0x1000;
				}
				else if (diff < -0x800)
				{
					diff += 0x1000;
				}

				if (diff != 0)
				{
					if (diff > 0x40)
					{
						diff = 0x40;
					}
					else if (diff < -0x40)
					{
						diff = -0x40;
					}

					pJointAngles[axis] = (i16)(diff + cur);
				}
			}
		}

		if (pPlayer->mHealth < 0)
		{
			pPlayer->mHealth = 0;
		}

		// --- the spidey-sense buzz and the two fists, 0x4B806B ---
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hookA), pPlayer, 1);
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&hookB), pPlayer, 0);

		{
			CBody *pBuzz = reinterpret_cast<CBody*>(Mem_RecoverPointer(&pPlayer->field_ED4));

			if (pBuzz != 0)
			{
				i32 pulse;

				M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pBuzz->mPos), pPlayer, 8);
				gM3dUtils_GetPartAngles(pPlayer, 7, &pBuzz->mAngles, 0);

				pulse = Sine(((i32)gTimerRelated << 6) & 0xFFF);
				if (pulse < 0) pulse = -pulse;
				pulse = pulse / 2 + 0x800;

				pBuzz->mScale.vx = (i16)pulse;
				pBuzz->mScale.vy = (i16)pulse;
				pBuzz->mScale.vz = (i16)pulse;

				if (G_PULSATING_HEAD_FLAG != 0)
				{
					pBuzz->mScale.vx = (i16)(pBuzz->mScale.vx << 1);
					pBuzz->mScale.vy = (i16)(pBuzz->mScale.vy << 1);
					pBuzz->mScale.vz = (i16)(pBuzz->mScale.vz << 1);
				}

				SpideyAI0_ApplyCheatScale(pBuzz, 0);
			}
		}

		{
			i32 fist;

			for (fist = 0; fist < 2; fist++)
			{
				CBody *pFist = reinterpret_cast<CBody*>(
					Mem_RecoverPointer(&pPlayer->field_5B8[fist]));

				if (pFist != 0)
				{
					i32 dist;

					M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pFist->mPos),
						pPlayer, (fist != 0) ? 3 : 4);
					gM3dUtils_GetPartAngles(pPlayer, (fist != 0) ? 5 : 10,
						&pFist->mAngles, 0);

					dist = Utils_CrapDist(pFist->mPos, CameraList->mPos);
					if (dist < 0x100)
					{
						pFist->mFlags |= 0xC00;
						if (dist < 0xAA)
						{
							pFist->mRGB = 0;
						}
						else
						{
							u32 level = (u32)(dist - 0xAA);

							pFist->mRGB = (((level << 8) | level) << 8) | level;
						}
					}
					else
					{
						pFist->mFlags &= 0xF3FF;
						pFist->mRGB = 0x808080;
					}

					if (pPlayer->field_5E8 != 0)
					{
						u32 blue = pFist->mRGB & 0xFF;

						pFist->mFlags |= 0x400;
						pFist->mRGB = (u32)((Rnd((i32)blue) / 2) << 8) | blue;
					}
				}
			}
		}

		// --- the flaming-fists effect, 0x4B826A ---
		if (pPlayer->field_5E8 != 0 && pPlayer->field_5AC != 0)
		{
			u16 anim = pPlayer->mAnim;
			u32 args[2];
			i32 count;
			i32 i;

			if (anim == 0x66 || anim == 0x68 || anim == 0x6A || anim == 0xD5)
			{
				count = 3;
			}
			else
			{
				count = (Rnd(4) != 0) ? 0 : 1;
			}

			args[0] = (u32)&hookA;
			args[1] = 0x100;
			for (i = 0; i < count; i++)
			{
				Reloc_CallUserFunction(gFistEffectNameLeft, 5, args, 0);
			}

			if (anim == 0x64 || anim == 0x6A || anim == 0xD5)
			{
				count = 3;
			}
			else
			{
				count = (Rnd(4) != 0) ? 0 : 1;
			}

			args[0] = (u32)&hookB;
			for (i = 0; i < count; i++)
			{
				Reloc_CallUserFunction(gFistEffectNameRight, 5, args, 0);
			}
		}

		// --- swing line, web and held object render ends, 0x4B8346 ---
		if (pPlayer->field_E64 != 0)
		{
			if (pPlayer->mAnim == 0x118)
			{
				hookA -= (pPlayer->field_C6C * 8);
				gCSwinger_SetRenderEnd(pPlayer->field_E64, hookA);
			}
			else
			{
				hookB -= (pPlayer->field_C6C * 8);
				gCSwinger_SetRenderEnd(pPlayer->field_E64, hookB);
			}
		}

		if (pPlayer->field_E6C != 0)
		{
			CWeb *pWeb = reinterpret_cast<CWeb*>(pPlayer->field_E6C);
			CVector firePos;

			if (pWeb->field_102 == 0)
			{
				firePos = hookB;
			}
			else if (pWeb->field_102 == 1)
			{
				firePos = hookA;
			}
			else
			{
				firePos = hookA + ((hookB - hookA) / 2);
			}

			gCWeb_SetFirePos(pWeb, firePos);
		}

		if (pPlayer->mHeldObject != 0)
		{
			CManipOb *pHeld = pPlayer->mHeldObject;
			CVector mid = hookA + ((hookB - hookA) / 2);
			CVector dir = (mid - pPlayer->mPos) >> 6;
			i32 reach;

			VectorNormal(reinterpret_cast<VECTOR*>(&dir),
				reinterpret_cast<VECTOR*>(&dir));

			reach = (i32)pHeld->field_108;
			pHeld->mPos = (mid + (dir * reach)) + (pPlayer->field_C6C * 0x20);
			pHeld->mAngles.vy = (i16)(pHeld->mAngles.vy + (i16)pPlayer->field_548);
		}

		// --- keep the fists model in step with the animation, 0x4B8638 ---
		// 0xAB4: the animation id the current fists model was built for.
		if (pPlayer->mAnim != (u16)PLR_U8(pPlayer, 0xAB4))
		{
			// gFistsData[anim] >> 14 is the fists variant, see
			// CPlayer::SortFistsData in spidey.cpp.
			u16 *pFistsData = reinterpret_cast<u16*>(0x00555A14);

			pPlayer->CreateFists((u8)(pFistsData[pPlayer->mAnim] >> 14));
			PLR_U8(pPlayer, 0xAB4) = (u8)pPlayer->mAnim;
		}

		pPlayer->SetupLookaroundCamera();
		pPlayer->DrawOffscreenSpideySenseIndicatorList();

		// clear the pad's per-frame edge flags, skipping entries 14 and 15
		{
			i32 i;

			for (i = 0; i < 20; i++)
			{
				if (i != 14 && i != 15)
				{
					pPad[i * 16 + 1] = 0;
					PLR_U8(pPlayer, 0x1C1 + i * 16) = 0;
				}
			}
		}
	}
}
