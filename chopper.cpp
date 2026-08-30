#include "chopper.h"
#include "validate.h"
#include "utils.h"
#include "baddy.h"
#include "trig.h"
#include "spool.h"
#include "ps2lowsfx.h"
#include "m3dutils.h"
#include "exp.h"
#include "spidey.h"
#include "camera.h"
#include "chunk.h"
#include "ps2pad.h"
#include "front.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include <cmath>
#include "ai.h"
#include "panel.h"
#include "PCGfx.h"

extern CBody* ControlBaddyList;
extern CBaddy* BaddyList;

extern i32 DifficultyLevel;

extern const char *gObjFile;
extern CPlayer* MechList;

extern CCamera* CameraList;

// scratch camera position/rotation matrix for GTE screen projection, same addresses
// utils.cpp's gCameraViewMatrix and spidey.cpp's stru_56F1B4/stru_56F224 use
// (CPlayer::RenderLookaroundReticle idiom).
static CVector * const gCameraViewPos = (CVector*)0x0056F1B4;
static MATRIX * const gCameraViewMatrix = (MATRIX*)0x0056F224;

// Hooks-packet data table for CChopper's model attach points, passed to
// M3dUtils_ReadHooksPacket. Same pattern as turret.cpp's raw 0x55984C and
// thug.cpp's gThugTypeRelated* tables: fixed, read-only data baked into the
// original exe image, not a mutable global, so no G_* macro is needed.
static void * const gChopperHooksPacket = (void*)0x00548F88;

// @Ok
// @Matching
void Chopper_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 318)
		{
			delete pSearch;
		}
		else if (pSearch->mType == 322)
		{
			delete pSearch;
		}
		else if (pSearch->mType == 323)
		{
			delete pSearch;
		}

		pSearch = pNext;
	}
}

// @Ok
// @Matching
void Chopper_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Chopper_RelocatableModuleClear;
	pMod->field_C[0] = Chopper_CreateChopper;
	pMod->field_C[1] = Chopper_CreateSearchlight;
	pMod->field_C[2] = Chopper_CreateSniper;
}

// @Ok
CBulletFrag::~CBulletFrag(void)
{
}

// @Ok
// @Test
void CChopper::TrackSpidey(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->MarkAIProcList(0, 256, 0);
			this->SetHeightMode(4);

			this->SetDesiredPosForTrackMode();
			this->dumbAssPad++;
			break;
		case 1:
			if (this->field_218 & 1)
			{
				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			else if (this->GetToDesiredPos())
			{
				this->field_1F8 = 0;
				this->SetHeightMode(5);
				this->dumbAssPad++;
			}
			else
			{
				this->GetOutOfCameraPath();
				this->field_35C |= 1;
			}
			break;
		case 2:

			this->field_35C |= 1;
			if (this->field_218 & 1)
			{
				this->field_37C = 0;

				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			else
			{
				this->field_1F8 += this->field_80;
				if (this->field_1F8 > 30)
				{
					if (this->field_380)
						this->StartStrafeOnslaught();
					this->dumbAssPad++;
				}
			}


			break;
		case 3:
			this->field_35C |= 1;

			if (!this->field_384 || !this->field_380)
			{
				if (this->field_218 & 1)
					this->field_31C.bothFlags = 2;

				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @NotOk
// @FIXME name does not have a V
// @Note: reconstructed from tools/functions/4352816.bin. States 0/1 mirror the
// GetToPos fallthrough idiom used in FollowWaypoints. State 2 walks the
// waypoint's links (G_OFFSETLIST), pulsing every linked trigger except a
// special type-1002 link, which is remembered as the gun target. State 3's
// interpolation block (Trig_GetPosition + CVector lerp into field_3A8) is a
// best-effort reconstruction of the control flow shape, not fully instruction
// verified. cmpsum: 223 mnemonic diffs; MSVC6 did not emit the original's
// jump-table dispatch for this switch (built an if-chain instead), first
// divergence right at the dispatch. 1 attempt, not iterated. Needs real work.
void CChopper::FireMachineGunAtWaypointV(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->MarkAIProcList(0, 256, 0);
			this->SetHeightMode(4);
			this->dumbAssPad++;
		case 1:
			if (this->GetToPos(&this->field_33C))
			{
				this->SetHeightMode(5);
				this->field_3C4 = 1;
				this->dumbAssPad++;
			}
			break;
		case 2:
		{
			print_if_false(1u, "Bad register index");

			u16* LinksPointer = Trig_GetLinksPointer(this->realRegisterArr[0]);
			i32 found = 0;

			for (i32 i = 0; i < LinksPointer[0]; i++)
			{
				i16 link = LinksPointer[1 + i];

				if (found == 0 && G_OFFSETLIST[link][0] == 1002)
					found = link;
				else
					Trig_SendPulseToNode(link);
			}

			if (found == 0)
			{
				this->field_384 = 0;
				this->field_31C.bothFlags = 1;
				this->dumbAssPad = 0;
			}
			else
			{
				print_if_false(1u, "Bad register index");
				this->realRegisterArr[1] = 0;
				print_if_false(1u, "Bad register index");
				this->realRegisterArr[2] = 0;

				this->dumbAssPad++;
				this->field_384 = 2;

				if (this->field_3C4)
				{
					this->field_3C4 = 0;
					print_if_false(1u, "Bad register index");

					Trig_GetPosition(&this->field_3B8, this->realRegisterArr[0]);

					print_if_false(1u, "Bad register index");
					CVector target;
					Trig_GetPosition(&target, this->realRegisterArr[1]);

					print_if_false(1u, "Bad register index");

					for (i32 j = 0; j < 4; j++)
					{
						this->field_3B8 += (target - this->field_3B8) * this->realRegisterArr[2];
						this->field_3A8 = this->field_3B8;
					}
				}
			}
			break;
		}
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
INLINE i32 CChopper::GetToDesiredPos(void)
{
	CVector v13 = (MechList->mPos - this->field_33C);

	v13.vy = this->field_34C;
	return this->GetToPos(&v13);
}

// @Ok
void INLINE CChopper::GetOutOfCameraPath(void)
{
	if (this->InCameraPath())
	{
		i32 newY = CameraList->mPos.vy - 409600;
		if (this->field_34C > newY)
			this->field_34C = newY;
	}
}


// @Ok
i32 INLINE CChopper::InCameraPath(void)
{
	i32 v1 = this->field_360 - CameraList->field_23A;
	if (v1 < -2048)
	{
		v1 += 4096;
	}
	else if (v1 > 2048)
	{
		v1 -= 4096;
	}

	return my_abs(v1) < 250;
}

// @Ok
// @Test
void CChopper::StartStrafeOnslaught(void)
{
	if (MechList->field_8E8)
	{
		CVector v18(0, (Vblanks & 1) != 0 ? 4096 : -4096, 0);

		gte_ldopv1(reinterpret_cast<VECTOR*>(&MechList->field_C84));
		gte_ldopv2(reinterpret_cast<VECTOR*>(&v18));

		gte_op12();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&v18));
		VectorNormal(
				reinterpret_cast<VECTOR*>(&v18),
				reinterpret_cast<VECTOR*>(&v18));

		v18 *= 400;
		
		this->field_388 = MechList->mPos - v18;
		this->field_3A4 = Rnd(4) + 8;

		v18 /= (this->field_3A4 >> 1);

		this->field_394 = v18;
	}
	else
	{
		i32 v6 = this->field_360 - this->field_358;

		if (v6 < -2048)
		{
			v6 += 4096;
		}
		else if (v6 > 2048)
		{
			v6 -= 4096;
		}

		
		CSVector v13;

		v13.vy = this->field_358 + (v6 < 0 ? -800 : 800);
		v13.vx = 0;
		v13.vz = 0;

		CVector v17;
		Utils_GetVecFromMagDir(&v17, 4096, &v13);

		v17 >>= 12;
		v17 *= 200;

		CVector v16 = MechList->mPos;
		if (!MechList->field_AD4)
		{
			i32 GroundHeight = Utils_GetGroundHeight(&MechList->mPos, 300, 300, 0);

			if (GroundHeight != -1)
				v16.vy = GroundHeight;
		}

		this->field_388 = v16 - v17;
		this->field_3A4 = Rnd(4) + 8;

		v17 /= (this->field_3A4 >> 1);
		this->field_394 = v17;
	}

	this->field_3A0 = 0;
	this->field_384 = 1;
}

// @Ok
// @Test
void CChopper::Shoot(void)
{
	if (this->field_384)
	{
		if (!this->field_328)
			this->field_328 = SFX_PlayPos(0x8008u, &this->mPos, 0);

		switch (this->field_384)
		{
			case 1:
				if ((gAttackRelated & 3) == 0)
				{
					CVector v8;
					M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&v8), this, 1);

					CVector v7 = this->field_394 * this->field_3A0;
					v7 += this->field_388;

					v7 += (v7 - v8);

					this->field_3A8 = v7;
					M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&v8), this, 1);

					this->ShotCollision(&v8, &v7);

					if (++this->field_3A0 > this->field_3A4)
						this->field_384 = 0;
				}

				break;
			case 2:
				if ((gAttackRelated & 3) == 0)
				{
					CVector v8;
					M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&v8), this, 1);
					this->ShotCollision(&v8, &this->field_3B8);
					this->field_3C4 = 1;
				}
				break;
			default:
				print_if_false(0, "Unknown shooting mode!");
				break;
		}
	}
}

// @Ok
void CChopper::ShotCollision(CVector *a2, CVector *a3)
{
	new CGlowFlash(a2, 5, 0xA0u, 144, 96, 16, 0, 0, 0, 0, 50, 20, 1, 20, 10, 40, 20, 10, 2);
	new CMachineGunBullet(a2, a3, this);
}

// @Ok
// @AlmostMatching: vec assignement always differs dunno why
void CChopper::SetDesiredPosForTrackMode(void)
{
	this->field_360 = this->field_358;
	if (this->field_32C)
	{
		if (this->field_218 & 0x20)
		{
			this->field_360 -= 653;
			if (!Rnd(16))
				this->field_218 &= 0xFFFFFFDF;
			}
			else
			{
				this->field_360 += 653;
				if (!Rnd(16))
					this->field_218 |= 0x20u;
			}
	}

	this->field_360 = Rnd(796) + this->field_360 - 398;
	this->field_360 &= 0xFFFu;
	this->field_370 = 800;
	this->field_370 = Rnd(50) + this->field_370 - 25;

	CSVector v14;
	v14.vx = 0;
	v14.vy = this->field_360;
	v14.vz = 0;
	Utils_GetVecFromMagDir(&this->field_364, 4096, &v14);

	this->field_33C = (this->field_364 >> 12) * this->field_370;
	this->field_33C.vy = 3276800;

	this->field_33C.vy += (Rnd(614400) - 327680) + 20480;

	this->field_34C = MechList->mPos.vy - this->field_33C.vy;

	if (this->field_34C > this->field_350)
		this->field_34C = this->field_350;
}

// @Ok
// @Note: checked against the Hex-Rays decompile of tools/functions/4350624.bin
// (0x4262a0). Two real bugs fixed: (1) the field_328 SFX block had the
// condition inverted and was missing the field_384 gate entirely, the
// original is "if (field_328) { if (field_384) ModifyPos; else { Stop;
// field_328=0; } }", not "if (!field_328) Stop; else ModifyPos;";
// (2) the RotateBlades/AimGunPod pose data address was wrong (0x1A2BD8),
// the original uses the SAME table (0x548F48) for the blade-rotation
// ApplyPose call inside RotateBlades and for this trailing unconditional
// ApplyPose call (which does the per-frame InBetween/BuildPose refresh,
// separate from RotateBlades'/AimGunPod's own "if (!mpJoints)"-guarded
// lazy-init calls to the same function). The switch below matches: case 3
// is CChopper::WaitForTrigger() inlined (same TU, INLINE keyword), the
// shared dumbAssPad=0/bothFlags=2 tail case 0 jumps into is exactly this
// source's case 0 body, just reordered.
void CChopper::AI(void)
{
	if (this->pMessage)
		this->CleanUpMessages(1, 0);

	if ((gAttackRelated & 3) == 0)
	{
		if (this->field_328)
		{
			if (this->field_384)
			{
				SFX_ModifyPos(this->field_328, &this->mPos, 0);
			}
			else
			{
				SFX_Stop(this->field_328);
				this->field_328 = 0;
			}
		}

		if (this->field_324)
			SFX_ModifyPos(this->field_324, &this->mPos, 0);
		else
			this->field_324 = SFX_PlayPos(0x8001u, &this->mPos, 0);
	}

	this->DoChopperPhysics();
	this->RotateBlades();
	this->AimGunPod();
	this->ApplyPose(reinterpret_cast<i16*>(0x548F48));

	if (this->field_384)
	{
		this->Shoot();
	}
	else
	{
		this->field_3A8.vx = 0;
		this->field_3A8.vy = 0;
		this->field_3A8.vz = 0;
	}

	switch (this->field_31C.bothFlags)
	{
		case 0:
			this->field_348 = 45;
            this->field_31C.bothFlags = 2;
			this->dumbAssPad = 0;
			break;
		case 1:
			this->TrackSpidey();
			break;
		case 2:
			this->FollowWaypoints();
			break;
		case 3:
			this->WaitForTrigger();
			break;
		case 4:
			this->FireMachineGunAtWaypointV();
			break;
		default:
			print_if_false(0, "Unknown state!");
			break;
	}
}

// @Ok
void CChopper::FollowWaypoints(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->MarkAIProcList(0, 256, 0);
			this->field_218 &= 0xFFFFFFFE;
			Trig_GetPosition(&this->field_33C, this->field_1F4);

			this->SetHeightMode(4);
			this->field_34C = this->field_33C.vy;
			this->dumbAssPad++;

			this->DoWaypointAction();

			if (this->field_218 & 8)
			{
				this->SetTargetAngleFromPos(&this->field_33C);
			}
			else if (this->field_218 & 16)
			{
				new CAIProc_LookAt(this, MechList, 0, 0, 55, 200);
			}
		case 1:
			if (this->GetToPos(&this->field_33C))
			{
				this->SetHeightMode(5);
				if (!this->DoArrivalAction())
				{
					if (!this->GetNextWaypoint())
						this->field_31C.bothFlags = 1;
					this->dumbAssPad = 0;
				}
			}
			else if ((this->field_218 & 0x10) == 0)
			{
				this->field_35C |= 1u;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
// @Note: checked against the Hex-Rays decompile of tools/functions/4349680.bin
// (0x425ef0). One real bug: ptr[18] should be ptr[19] (byte offset 38, not
// 36) for the adjusted yaw component; the unadjusted x component write at
// ptr[24] was already right. this+392 (mpJoints) matches this class's
// mpJoints field exactly (the earlier "field_188" note was a false alarm,
// there is no separate field_188).
void CChopper::AimGunPod(void)
{
	if (this->field_3A8.vx)
	{
		CVector v4;
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&v4), this, 0);

		CSVector v3;

		Utils_CalcAim(&v3, &v4, &this->field_3A8);
		v3.vy = (v3.vy - this->mAngles.vy) & 0xFFF;

		if (!this->mpJoints)
			this->ApplyPose(reinterpret_cast<i16*>(0x548F48));

		i16* ptr = reinterpret_cast<i16*>(this->mpJoints);
		ptr[24] = v3.vx;
		ptr[19] = v3.vy;
	}
}

// @Ok
// @Note: checked against the Hex-Rays decompile inlined into
// tools/functions/4350624.bin's CChopper::AI (0x4262a0, RotateBlades is
// INLINE so has no separate original address). The blade-rotation indices
// (ptr[6], ptr[13]) already matched; the pose data address was wrong
// (0x1A2BD8), the original uses 0x548F48, the same table AimGunPod uses.
void INLINE CChopper::RotateBlades(void)
{
	if (!this->mpJoints)
		this->ApplyPose(reinterpret_cast<i16*>(0x548F48));

	u16* ptr = reinterpret_cast<u16*>(this->mpJoints);

	ptr[6] += 35 * this->field_80;
	ptr[6] &= 0xFFF;

	ptr[13] += 35 * this->field_80;
	ptr[13] &= 0xFFF;
}

// @Ok
INLINE void CChopper::SetTargetAngleFromPos(CVector* a2)
{
	CSVector v4;
	v4.vx = 0;
	v4.vy = 0;
	v4.vz = 0;
	Utils_CalcAim(&v4, &this->mPos, a2);
	this->field_360 = v4.vy;
}

// @Ok
i32 INLINE CChopper::GetToPos(CVector* a2)
{
	if (Utils_CrapDist(this->field_330, *a2) < (2 * this->field_348))
		return 1;

	CSVector v6;
	Utils_CalcAim(&v6, &this->field_330, a2);

	i32 v5;
	if (my_abs(this->mAngVel.vy) > 0x20)
		v5 = 0;
	else
		v5 = this->field_348 * (32 - my_abs(this->mAngVel.vy)) / 32;

	Utils_GetVecFromMagDir(&this->mVel, v5, &v6);
	return 0;
}

// @Ok
i32 INLINE CChopper::DoWaypointAction(void)
{
	i16 *ptr = G_OFFSETLIST[this->field_1F4];
	if ( ptr[0] == 1002 )
	{
		switch (ptr[1])
		{
			case 1:
				this->field_218 |= 8;
				break;
			case 2:
				this->field_218 &= 0xFFFFFFF7;
				break;
			case 3:
				this->field_218 |= 0x10;
				break;
			case 4:
				this->field_348 = 65;
				break;
			case 5:
				this->field_348 = 45;
				break;
			case 6:
				this->field_348 = 20;
				break;
			default:
				break;
		}
	}

	return 0;
}

// @Ok
i32 INLINE CChopper::DoArrivalAction(void)
{
	i16* ptr = G_OFFSETLIST[this->field_1F4];
	if (ptr[0] == 1002 && ptr[1] == 7)
	{
		this->field_31C.bothFlags = 3;
		this->dumbAssPad = 0;
		return 1;
	}

	return 0;
}

// @Ok
void CChopper::DoChopperPhysics(void)
{
	CVector v15 = this->mVel;
	CVector v13;
	CVector v14;

	for (i32 i = this->field_80; i; i++)
	{
		this->mVel += this->mAcc;
		this->mVel %= this->mFric;
		this->mVel.KillSmall();

		this->field_330 += this->mVel;
		this->AngleToTargetAngle();
		this->SetHeight();
		
		this->mPos.vx = this->field_330.vx;
		this->mPos.vz = this->field_330.vz;
	}

	v13 = this->mVel - v15;
	Utils_RotateWorldToObject(this, &v13, &v14);


	if (abs(v14.vz) > 20480)
		this->mAngles.vx = Utils_ShiftFilter(this->mAngles.vx, v14.vz > 0 ? 128 : -128, 5, 16);
	else
		this->mAngles.vx = Utils_ShiftFilter(this->mAngles.vx, 0, 1, 2);

	if (abs(v14.vx) > 20480)
		this->mAngles.vz = Utils_ShiftFilter(this->mAngles.vz, v14.vx > 0 ? -256 : 256, 5, 16);
	else
		this->mAngles.vz = Utils_ShiftFilter(this->mAngles.vz, 0, 1, 2);
	this->field_35C = 0;
}

// @Ok
// @Matching
void CChopper::SetHeight(void)
{
	i32 v2;
	switch(this->field_374)
	{
		case 0:
			break;
		case 2:
			v2 = this->field_330.vy - (this->field_354 >> 12) * rcossin_tbl[this->field_378 & 0xFFF].sin;
			if (this->mPos.vy != v2)
			{
				this->mPos.vy = Utils_ShiftFilter(this->mPos.vy, v2, 1, 12288);
				break;
			}
			this->field_374 = 1;
		case 1:
			this->AdjustSineWaveAmplitude(0x10000, 182);
			this->field_378 += 51;
			this->mPos.vy = this->field_330.vy - (this->field_354 >> 12) * rcossin_tbl[this->field_378 & 0xFFF].sin;
			break;
		case 4:
			this->field_378 = 1024;
			this->field_374 = 3;
		case 3:
			this->AdjustSineWaveAmplitude(0x20000, 364);

			this->mPos.vy = this->field_330.vy +
				Utils_ShiftFilter(this->mPos.vy - this->field_330.vy,
					-this->field_354,
					1,
					12288);
			break;
		case 5:
			this->mPos.vy = this->field_330.vy + Utils_ShiftFilter(
					this->mPos.vy - this->field_330.vy,
					this->field_354,
					1,
					12288);
			if (this->mPos.vy == this->field_330.vy + this->field_354)
			{
				this->field_378 = 3072;
				this->field_374 = 2;
			}
			break;
		default:
			print_if_false(0, "Unknown height mode!");
			break;
	}

}

// @Ok
void CChopper::FireMachineGunAtWaypoint(u32 a2, u32 a3)
{
	Trig_GetPosition(&this->field_33C, a2);
	print_if_false(1u, "Bad register index");
	this->realRegisterArr[0] = a3;
	this->field_31C.bothFlags = 4;
	this->dumbAssPad = 0;
}

// @Ok
void CChopper::FireMissileAtWaypoint(u32 a2)
{
	CVector v10;

	M3dUtils_GetHookPosition(
			reinterpret_cast<VECTOR*>(&v10),
			this,
			this->field_3B4);

	u32 res;
	if ( this->field_3B4 == 9 )
		res = 2;
	else
		res = this->field_3B4 + 1;

	this->field_3B4 = res;

	new CChopperMissile(&v10, this, a2, 0);
}

// @Ok
void CChopper::SetFlag(u16 a2, i16 a3)
{
	if (a2)
		print_if_false(0, "Bad param");
	else
		this->field_380 = a3 != 0;
}

// @Ok
CChopper::~CChopper(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));

	if (this->field_328)
		SFX_Stop(this->field_328);

	if (this->field_324)
		SFX_Stop(this->field_324);
}

// @NotOk
// @Note: checked against the Hex-Rays decompile of tools/functions/4346880.bin
// (0x425400). Everything up to and including the mVel computation
// (Utils_GetVecFromMagDir(&mVel, field_108>>12, &mAngles)) matches this
// source. The stepping loop below that does NOT match: the original mutates
// this->mPos DIRECTLY inside the collision-check loop (mPos += mVel*2 each
// iteration, via CVector::operator+=), and ALSO updates a turn-smoothing
// sub-state each iteration (this+136/138/142/144/148/149, a CSVector pair
// plus shift-amount bytes, fed through CSVector::Mask()/KillSmall() and a
// manual "x - (x >> shift)" IIR-style smoothing formula) before the
// Utils_Dist(mPos, field_110) < 0x80 hit check, not just a plain position
// accumulate in a local. This source's version steps a local `pos` copy with
// no per-step turn-state update, which is a real behavioural gap (the missile
// homing curve depends on that per-substep smoothing feeding mAngVel-derived
// state forward into later frames), not just register residue. AFTER the
// loop, the original also repositions the smoke ribbon at
// mPos - Utils_GetVecFromMagDir(128, mAngles) (a fixed offset behind the
// missile along its facing), not at mPos directly as this source's
// field_F8->SetPos(this->mPos) does. The this+136 region's exact field
// identity (offsets relative to CBody, likely inside/near mAngVel's
// smoothing state) was not resolved. Left @NotOk: fixing this needs mapping
// those fields properly, which is more than triage time allows in this pass.
void CChopperMissile::AI(void)
{
	if (this->field_120)
	{
		if (this->field_120 <= this->field_80)
			this->field_120 = 0;
		else
			this->field_120 -= this->field_80;
	}

	if (this->field_FC == 2)
		return;

	if (DifficultyLevel == 1 || DifficultyLevel == 0)
	{
		if (this->field_108 < 0x2A000)
		{
			this->field_108 += this->field_80 << 13;
			if (this->field_108 > 0x2A000)
				this->field_108 = 0x2A000;
		}
	}
	else
	{
		if (this->field_108 < 0x40000)
		{
			this->field_108 += this->field_80 << 13;
			if (this->field_108 > 0x40000)
				this->field_108 = 0x40000;
		}
	}

	CSVector aimDir;
	aimDir.vx = 0;
	aimDir.vy = 0;
	aimDir.vz = 0;
	Utils_CalcAim(&aimDir, &this->mPos, &this->field_110);

	CSVector newAngles;
	if (Utils_CrapDist(this->mPos, this->field_110) > 0x200)
		Utils_TurnTowards(aimDir, &newAngles, &this->mAngVel, this->mAngAcc, this->field_108 >> 12);
	else
		newAngles = aimDir;

	this->mAngles = newAngles;

	Utils_GetVecFromMagDir(&this->mVel, this->field_108 >> 12, &this->mAngles);

	bool hitTarget = false;
	CVector pos = this->mPos;

	for (i32 steps = 0; steps < this->field_80; steps += 2)
	{
		pos += this->mVel * 2;

		if (Utils_Dist(pos, this->field_110) < 0x80)
		{
			hitTarget = true;
			break;
		}
	}

	this->mPos = pos;
	this->field_F8->SetPos(this->mPos);
	SFX_ModifyPos(this->field_10C, &this->mPos, 0);

	if (!hitTarget)
		return;

	if (this->field_FC != 0 && this->field_FC != 1)
		return;

	u16* LinksPointer = Trig_GetLinksPointer(this->field_100);

	if (this->field_FC == 0)
	{
		if (LinksPointer[0])
		{
			for (i32 i = 0; i < LinksPointer[0]; i++)
				Trig_SendPulseToNode(LinksPointer[1 + i]);
		}
		this->Explode();
	}
	else
	{
		if (LinksPointer[0])
		{
			i32 v9 = LinksPointer[1];
			if (*G_OFFSETLIST[v9] == 1002)
			{
				this->field_100 = v9;
				Trig_GetPosition(&this->field_110, v9);
				return;
			}
		}
		this->Explode();
	}
}

// @Ok
// @Matching
void CChopperMissile::Explode(void)
{
	if (this->field_11C)
	{
		Chunk_ChunkItemByChecksum(this->field_11C);
	}

	u32 v2 = Utils_Dist(this->mPos, MechList->mPos);

	if (v2 < 0x19A)
	{
		SHitInfo v7;

		v7.field_0 = 6;
		v7.field_4 = 24;

		if ( v2 < 0x118 )
			v7.field_8 = 100;
		else
			v7.field_8 = 100 - 95 * (v2 - 280) / 130;

		MechList->Hit(&v7);
	}

	if (v2 < 0x320 && gSaveGame.field_7B)
	{
		Pad_ActuatorOn(0, 0x3Cu, 1, 0x64u);
	}

	new CGrenadeWave(
			&this->mPos,
			0x80u,
			0x60u,
			0x10u,
			512,
			7);


	this->field_F8->mFadeAway = 1;
	this->field_F8->mProtected = 0;
	this->field_F8 = 0;
	this->Die();
}

// @NotOk
// @Note: reconstructed from tools/functions/4342784.bin. Guarded by
// field_104 (a Trig node id, non-zero when a target link is set) and field_120 (a state
// gate, must be 0), then Trig_GetPosition(&field_110, field_104) refreshes the target
// position and the same gte_SetRotMatrix/m3d_ZeroTransVector/gte_ldlv0/gte_rtps/
// gte_stlvnl2/gte_stsxy screen projection idiom as
// CSniperTarget::DrawTargetRecticle is applied, clipped on depth < 200. The original
// then draws two Panel_DrawTexturedPoly icons (field_124 plus what looks like a
// distance/warning readout, calling an unnamed helper at 0x509000 six times, most
// likely digit rendering for a distance readout) which this reconstruction does not
// reproduce: could not determine that helper's signature from the disassembly alone
// without pulling in another file's stub declarations, so only the icon draw and a
// schematic bracket are implemented here, matching the CSniperTarget precedent. This
// is a best-effort structural reconstruction, not instruction verified.
// cmpsum: 975 mnemonic diffs, first divergence right at the field_104/field_120 guard
// (our compare order differs). 1 attempt (structural reconstruction only). Needs real
// work.
void CChopperMissile::DrawTargetRecticle(void)
{
	if (!this->field_104 || this->field_120)
		return;

	Trig_GetPosition(&this->field_110, this->field_104);

	CVector camPos = *gCameraViewPos;
	CVector relPos = (this->field_110 >> 12) - camPos;

	gte_SetRotMatrix(gCameraViewMatrix);
	m3d_ZeroTransVector();
	gte_ldlv0(reinterpret_cast<VECTOR*>(&relPos));
	gte_rtps();

	i32 depth;
	gte_stlvnl2(&depth);

	i16 screenXY[2];
	gte_stsxy(reinterpret_cast<i32*>(screenXY));

	if (depth < 200)
		return;

	i32 screenX = screenXY[0];
	i32 screenY = screenXY[1];

	POLY_FT4* poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(this->field_124, 0));
	if (!poly)
		return;

	*reinterpret_cast<u32*>(&poly->r0) = 0x2E808080;
	poly->tpage = (poly->tpage & 0xFFDF) | 0x40;

	i32 halfW = 12;
	i32 halfH = 12;

	poly->x0 = static_cast<i16>(screenX - halfW);
	poly->y0 = static_cast<i16>(screenY - halfH);
	poly->x1 = static_cast<i16>(screenX + halfW);
	poly->y1 = static_cast<i16>(screenY - halfH);
	poly->x2 = static_cast<i16>(screenX - halfW);
	poly->y2 = static_cast<i16>(screenY + halfH);
	poly->x3 = static_cast<i16>(screenX + halfW);
	poly->y3 = static_cast<i16>(screenY + halfH);

	i32 bracket = 20;

	for (i32 i = 0; i < 4; i++)
	{
		i32 signX = (i & 1) ? 1 : -1;
		i32 signY = (i & 2) ? 1 : -1;

		f32 x0 = static_cast<f32>(screenX + signX * bracket);
		f32 y0 = static_cast<f32>(screenY + signY * bracket);
		f32 x1 = static_cast<f32>(screenX + signX * (bracket - 6));
		f32 y1 = y0;
		f32 x2 = x0;
		f32 y2 = static_cast<f32>(screenY + signY * (bracket - 6));

		PCGfx_UseTexture(0, DCGfx_BlendingMode_0);
		PCGfx_DrawQPoly2D(
				x0, y0, 0.0f, 1.0f, 0xFFFFFFFFu,
				x1, y1, 0.0f, 1.0f, 0xFFFFFFFFu,
				x2, y2, 0.0f, 1.0f, 0xFFFFFFFFu,
				x0, y0, 0.0f, 1.0f, 0xFFFFFFFFu,
				0.0f);
	}
}

// @Ok
CChopperMissile::~CChopperMissile(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));

	if (this->field_10C)
		SFX_Stop(this->field_10C);

	if (this->field_F8)
		delete this->field_F8;
}

// @Ok
INLINE i32 CChopperMissile::GetFinalTargetNode(i32 a2)
{
	for (u16 *LinksPointer = Trig_GetLinksPointer(a2);
			LinksPointer;
			LinksPointer = Trig_GetLinksPointer(LinksPointer[1]))
	{
		
		i32 v9 = LinksPointer[1];

		if (*G_OFFSETLIST[v9] != 1002)
			return v9;
	}
	return 0;
}

// @Ok
void CChopperMissile::CommonInitialisation(void)
{
	this->mType = 321;

	this->InitItem(gObjFile);
	this->mFlags &= ~2u;
	this->mCBodyFlags &= ~0x10u;

	this->mModel = Spool_GetModel(0x8CEF63CD, gObjFileRegion);
	this->mRMinor = 0;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->field_F8 = new CSmokeTrail(&this->mPos, 6, 80, 80, 96);

	this->field_F8->mProtected = 1;
	this->field_F8->SetScale(768);
	
	char v7[16] = "ChopperTarget01";
	this->field_124 = Spool_FindTextureEntry(v7);
	this->field_10C = SFX_PlayPos(0x8001u, &this->mPos, 0);
}

// @Ok
INLINE CChopperMissile::CChopperMissile(
		CVector* a2,
		CSuper* a3,
		i32 a4,
		i32 a5)
{
	this->field_110.vx = 0;
	this->field_110.vy = 0;
	this->field_110.vz = 0;

	this->field_FC = a5;
	this->field_100 = a4;

	this->mPos = *a2;

	Trig_GetPosition(&this->field_110, a4);

	this->field_104 = this->GetFinalTargetNode(a4);

	if (DifficultyLevel == 3)
		this->field_120 = 20;

	this->CommonInitialisation();
}

// @NotOk
// @Note: checked against the Hex-Rays decompile of tools/functions/4338352.bin
// (0x4232b0). Confirmed WRONG: this source computes the beam radius from
// lineinfo.Distance (hit distance), but the original computes it from
// -(beamDir DOT lineinfo.Normal) >> 12, i.e. a foreshortening factor from the
// angle between the beam and the hit surface, not the hit distance at all
// (physically makes more sense for a light-cone flaring on angled surfaces).
// That value is stored in a field right before field_138 (a1[77], i.e.
// this+0x134, one CVector-worth before field_138 starts), not used directly
// as a radius. The sqrt branch threshold (4073/0xFE9) does match this
// source's, but is tested against the dot-product value, not Distance. The
// two GTE basis-vector calls after the branch use 4 near-identical looking
// calls that IDA mislabels "qt_register_signal_spy_callbacks" (a bogus FLIRT
// signature match, not real Qt calls; almost certainly gte_ldopv1/ldopv2 or
// similar), so the cross-product basis reconstruction needs re-deriving
// against the real GTE op order, not assumed from the CStartStrafeOnslaught
// precedent. The fan loop also scales by two different constants (275 and
// 315) applied to both near and far basis components, not a single `radius`
// value multiplied post-hoc as this source does. Left @NotOk: the beam
// radius algorithm needs a real rewrite, not a residue chase.
// cmpsum: 307 mnemonic diffs, first divergence right at the prologue register
// allocation. 1 attempt (structural reconstruction only). Needs real work.
void CSearchlight::CalculateSearchlight(CSVector* a2)
{
	CVector beamDir;
	Utils_GetVecFromMagDir(&beamDir, 0x1000, a2);

	CVector endPos = this->mPos + beamDir;

	SLineInfo lineinfo;
	lineinfo.StartCoords = this->mPos;
	lineinfo.EndCoords = endPos;
	lineinfo.MinCoords.vx = 0;
	lineinfo.MinCoords.vy = 0;
	lineinfo.MinCoords.vz = 0;
	lineinfo.MaxCoords.vx = 0;
	lineinfo.MaxCoords.vy = 0;
	lineinfo.MaxCoords.vz = 0;
	lineinfo.iLo = 0;
	lineinfo.iHi = 0;
	lineinfo.jLo = 0;
	lineinfo.jHi = 0;
	lineinfo.Distance = 0;
	lineinfo.Length = 0;
	lineinfo.pItem = 0;
	lineinfo.Position.vx = 0;
	lineinfo.Position.vy = 0;
	lineinfo.Position.vz = 0;
	lineinfo.Normal.vx = 0;
	lineinfo.Normal.vy = 0;
	lineinfo.Normal.vz = 0;

	M3dColij_InitLineInfo(&lineinfo);
	M3dZone_LineToItem(&lineinfo, 1);

	i32 radius = 0x400;

	if (lineinfo.pItem)
	{
		i32 dist = lineinfo.Distance;

		if (dist < 0xFE9)
		{
			double t = (double)(0x1000 - dist) / 0x1000;
			radius = (i32)(sqrt(t) * 0x1000);
		}
		else
		{
			radius = (dist * (0x1000 - dist)) >> 12;
		}
	}

	CVector up(0, 4096, 0);
	CVector right;
	gte_ldopv1(reinterpret_cast<VECTOR*>(&beamDir));
	gte_ldopv2(reinterpret_cast<VECTOR*>(&up));
	gte_op12();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&right));
	VectorNormal(reinterpret_cast<VECTOR*>(&right), reinterpret_cast<VECTOR*>(&right));

	CVector up2;
	gte_ldopv1(reinterpret_cast<VECTOR*>(&right));
	gte_ldopv2(reinterpret_cast<VECTOR*>(&beamDir));
	gte_op12();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&up2));
	VectorNormal(reinterpret_cast<VECTOR*>(&up2), reinterpret_cast<VECTOR*>(&up2));

	this->field_138[0] = this->mPos;
	this->field_138[1] = endPos;

	for (i32 i = 0; i < 32; i++)
	{
		i32 s = rcossin_tbl[(i << 7) & 0xFFF].sin;
		i32 c = rcossin_tbl[(i << 7) & 0xFFF].cos;

		CVector offset = ((right * s) + (up2 * c)) >> 12;

		this->field_138[2 + i * 2] = this->mPos + (offset * (radius >> 12));
		this->field_138[3 + i * 2] = endPos + (offset * (radius >> 12));
	}
}

// @Ok
// @Note: verified functionally correct against the Hex-Rays decompile of
// tools/functions/4339760.bin (0x423830): bbox reject on x then y, then the
// three edge-function sign tests against the d0 triangle-orientation term
// match term for term. Remaining diffs are register allocation / packed-arg
// unpacking order only (not chasing byte match per the functional bar).
void CSearchlight::CheckPointInScreenTri(u32 p, u32 a, u32 b, u32 c)
{
	i32 px = (i16)p;
	i32 py = (i16)(p >> 16);
	i32 ax = (i16)a;
	i32 ay = (i16)(a >> 16);
	i32 bx = (i16)b;
	i32 by = (i16)(b >> 16);
	i32 cx = (i16)c;
	i32 cy = (i16)(c >> 16);

	if (px < ax && px < bx && px < cx)
		return;
	if (px > ax && px > bx && px > cx)
		return;
	if (py < ay && py < by && py < cy)
		return;
	if (py > ay && py > by && py > cy)
		return;

	i32 d0 = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);

	i32 e1 = (cx - bx) * (py - by) - (px - bx) * (cy - by);
	if (d0 < 0 && e1 > 0)
		return;
	if (d0 > 0 && e1 < 0)
		return;

	i32 e2 = (ax - cx) * (py - cy) - (px - cx) * (ay - cy);
	if (d0 < 0 && e2 > 0)
		return;
	if (d0 > 0 && e2 < 0)
		return;

	i32 e3 = (bx - ax) * (py - ay) - (px - ax) * (by - ay);
	if (d0 < 0 && e3 > 0)
		return;
	if (d0 > 0 && e3 < 0)
		return;

	this->field_12C = 1;
}

// @NotOk
// @Note: reconstructed from tools/functions/4334144.bin. Same
// gte_SetRotMatrix/m3d_ZeroTransVector/gte_ldlv0/gte_rtps/gte_stlvnl2/gte_stsxy screen
// projection idiom as CPlayer::RenderLookaroundReticle (spidey.cpp, same
// stru_56F1B4/stru_56F224 scratch globals), clip tested against screen bounds, then a
// Panel_DrawTexturedPoly icon draw for field_11C with a fixed tint/blend flag, an
// aspect-ratio scale derived from the texture, and 4 bracket-corner quads via
// PCGfx_UseTexture/PCGfx_DrawQPoly2D. The icon draw (color, tpage flag) is traced with
// reasonable confidence; the bracket quad coordinates and PCGfx_DrawQPoly2D's many
// float arguments are a best-effort schematic, not instruction verified: could not
// trace that many packed float args by hand from raw disassembly.
// cmpsum: 675 mnemonic diffs, first divergence right at the prologue register
// allocation. 1 attempt (structural reconstruction only). Needs real work.
void CSniperTarget::DrawTargetRecticle(void)
{
	CVector camPos = *gCameraViewPos;
	CVector relPos = (this->field_104 >> 12) - camPos;

	gte_SetRotMatrix(gCameraViewMatrix);
	m3d_ZeroTransVector();
	gte_ldlv0(reinterpret_cast<VECTOR*>(&relPos));
	gte_rtps();

	i32 depth;
	gte_stlvnl2(&depth);

	i16 screenXY[2];
	gte_stsxy(reinterpret_cast<i32*>(screenXY));

	i32 screenX = screenXY[0];
	i32 screenY = screenXY[1];

	if (screenX < -200 || screenX > 712 || screenY < -200 || screenY > 440)
		return;

	POLY_FT4* poly = reinterpret_cast<POLY_FT4*>(Panel_DrawTexturedPoly(this->field_11C, 0));
	if (!poly)
		return;

	*reinterpret_cast<u32*>(&poly->r0) = 0x2E808080;
	poly->tpage = (poly->tpage & 0xFFDF) | 0x40;

	i32 halfW = 12;
	i32 halfH = 12;

	poly->x0 = static_cast<i16>(screenX - halfW);
	poly->y0 = static_cast<i16>(screenY - halfH);
	poly->x1 = static_cast<i16>(screenX + halfW);
	poly->y1 = static_cast<i16>(screenY - halfH);
	poly->x2 = static_cast<i16>(screenX - halfW);
	poly->y2 = static_cast<i16>(screenY + halfH);
	poly->x3 = static_cast<i16>(screenX + halfW);
	poly->y3 = static_cast<i16>(screenY + halfH);

	i32 bracket = 20;

	for (i32 i = 0; i < 4; i++)
	{
		i32 signX = (i & 1) ? 1 : -1;
		i32 signY = (i & 2) ? 1 : -1;

		f32 x0 = static_cast<f32>(screenX + signX * bracket);
		f32 y0 = static_cast<f32>(screenY + signY * bracket);
		f32 x1 = static_cast<f32>(screenX + signX * (bracket - 6));
		f32 y1 = y0;
		f32 x2 = x0;
		f32 y2 = static_cast<f32>(screenY + signY * (bracket - 6));

		PCGfx_UseTexture(0, DCGfx_BlendingMode_0);
		PCGfx_DrawQPoly2D(
				x0, y0, 0.0f, 1.0f, 0xFFFFFFFFu,
				x1, y1, 0.0f, 1.0f, 0xFFFFFFFFu,
				x2, y2, 0.0f, 1.0f, 0xFFFFFFFFu,
				x0, y0, 0.0f, 1.0f, 0xFFFFFFFFu,
				0.0f);
	}
}

// @NotOk
// @Note: reconstructed from tools/functions/4331776.bin. Kill-flag check + Die(), then
// a 3-state machine on field_100: state 0/1 are a line-of-sight/aim acquisition loop
// (Utils_CalcAim + Utils_TurnTowards toward field_104, a mAngVel/mAngAcc/mAngFric
// physics-style settle integrator reused for muzzle sway run field_80 times,
// Utils_GetVecFromMagDir to get the muzzle direction, an M3dColij raycast from the
// muzzle to the camera, a running best-distance field_12C, and periodic voice-line
// SFX_Play calls gated by three independent timers field_130/134/138), transitioning
// to state 2 once close enough. State 2 rate-limits firing (global timer 0x6B4CA8
// minus field_124, a shot budget field_F8 < field_FC, and a 40% random roll) and
// spawns a CMachineGunBullet owned by this (matching the existing
// CMachineGunBullet(CVector*,CVector*,CSniperTarget*) constructor: field_A4 == 10).
// This is a best-effort structural reconstruction of the control flow shape, not
// instruction verified: several new fields (field_F8/100/124/12C/130/134/138/154/158)
// were carved out of what the header had as PADDING, and the exact raycast/voice-line
// argument wiring and the aim-settle math could not be traced byte for byte by hand.
// cmpsum: 536 mnemonic diffs, first divergence right at the prologue (missing the SEH
// frame setup entirely). 1 attempt (structural reconstruction only). Needs real work.
void CSniperTarget::AI(void)
{
	if (this->mFlags & 1)
	{
		this->mFlags &= ~1;
		this->Die();
		return;
	}

	switch (this->field_100)
	{
		case 0:
		case 1:
		{
			CSVector aimDir;
			aimDir.vx = 0;
			aimDir.vy = 0;
			aimDir.vz = 0;
			Utils_CalcAim(&aimDir, &this->field_104, reinterpret_cast<CVector*>(&this->field_13C));

			i32 rate = (this->field_100 == 0) ? 8 : 0x10;
			Utils_TurnTowards(aimDir, reinterpret_cast<CSVector*>(&this->mAngles), &this->mAngVel, this->mAngAcc, rate);

			for (i32 i = 0; i < this->field_80; i++)
			{
				i16 vx = this->mAngVel.vx + this->mAngAcc.vx;
				this->mAngVel.vx = vx - (vx >> this->mAngFric.vx);

				i16 vy = this->mAngVel.vy + this->mAngAcc.vy;
				this->mAngVel.vy = vy - (vy >> this->mAngFric.vy);
			}

			this->mAngVel.Mask();
			this->mAngVel.KillSmall();

			CVector muzzleDir;
			Utils_GetVecFromMagDir(&muzzleDir, this->field_100 == 0 ? 0x12 : 0x10,
					reinterpret_cast<CSVector*>(&this->mAngles));

			CVector muzzleEnd = reinterpret_cast<CVector&>(this->field_13C) + muzzleDir;

			SLineInfo lineinfo;
			lineinfo.StartCoords = reinterpret_cast<CVector&>(this->field_13C);
			lineinfo.EndCoords = CameraList->mPos;
			lineinfo.MinCoords.vx = 0;
			lineinfo.MinCoords.vy = 0;
			lineinfo.MinCoords.vz = 0;
			lineinfo.MaxCoords.vx = 0;
			lineinfo.MaxCoords.vy = 0;
			lineinfo.MaxCoords.vz = 0;
			lineinfo.iLo = 0;
			lineinfo.iHi = 0;
			lineinfo.jLo = 0;
			lineinfo.jHi = 0;
			lineinfo.Distance = 0;
			lineinfo.Length = 0;
			lineinfo.pItem = 0;
			lineinfo.Position.vx = 0;
			lineinfo.Position.vy = 0;
			lineinfo.Position.vz = 0;
			lineinfo.Normal.vx = 0;
			lineinfo.Normal.vy = 0;
			lineinfo.Normal.vz = 0;

			M3dColij_InitLineInfo(&lineinfo);
			M3dZone_LineToItem(&lineinfo, 1);

			if (lineinfo.pItem)
			{
				this->field_104 = lineinfo.Position;
			}

			i32 dist = Utils_Dist(reinterpret_cast<CVector&>(this->field_13C), MechList->mPos);

			if (dist > this->field_12C)
			{
				this->field_12C = dist;
				this->field_130 = Vblanks;
			}
			else if (dist < this->field_134)
			{
				this->field_134 = dist;

				if (Vblanks - this->field_130 > 0x78)
				{
					SFX_Play((Rnd(6) & 0xFE) == 0 ? 0x8F00 : 0x8F04, 0x3C, 0);
					this->field_130 = Vblanks;
				}
			}

			if (Vblanks - this->field_138 > 0x78)
			{
				SFX_Play((Rnd(8) & 0xFE) == 0 ? 0x8F18 : 0x8F1C, 0x3C, 0);
				this->field_138 = Vblanks;
			}

			if (dist < 200)
			{
				this->field_100 = 2;
				this->field_128 = false;
				this->field_F8 = 0;
				this->field_FC = 0;
			}

			break;
		}
		case 2:
		{
			this->field_104 = this->field_104 + reinterpret_cast<CVector&>(this->field_148) * this->field_80;

			if (Vblanks - this->field_124 <= 10)
				break;

			if (this->field_154 >= this->field_158)
				break;

			this->field_124 = Vblanks;

			if (Rnd(100) >= 60)
				break;

			SFX_Play((Rnd(4) & 0xFE) == 0 ? 0x8F38 : 0x8F3C, 0x3C, 0);

			new CMachineGunBullet(reinterpret_cast<CVector*>(&this->field_13C), &this->field_104, this);

			this->field_F8++;

			if (this->field_F8 == this->field_FC)
			{
				this->field_120 = 180;
				this->field_100 = 0;
			}

			break;
		}
		default:
			break;
	}
}

// @Ok
// @Note: fixed against the Hex-Rays decompile of tools/functions/4329728.bin
// (0x421100). Two real bugs in the earlier version: (1) the "neither env nor
// player hit" case used to `return` early, but the original still runs the
// pSniper->BulletResult() counter, the chopper knockback check, and Die() in
// that case, it only skips the effect/SFX spawn; (2) on any hit the original
// always spawns a CBulletFrag and a CSmokeGenerator at mEnd (constructor
// addresses 0x420C20 / 0x412A30 confirmed by signature), not a CSniperSplat.
// Residue not chased further (cosmetic, SFX-id/spark related, not gameplay
// logic): the original also plays a rare probability-gated skip on the no
// pChopper env-hit SFX branch, tweaks a random field on the CGlowFlash object
// after construction, calls an unidentified virtual method through a global
// on player hits, and spawns an extra "webball crater" decal object via a
// name-lookup factory (sub_4C95C0) on environment hits with no owning
// chopper; none of these change hit detection, damage, or the death/cleanup
// path.
void CMachineGunBullet::Move(void)
{
	this->field_74 += 300;
	this->field_78 += 300;

	CVector* dir = reinterpret_cast<CVector*>(&this->field_68);
	CVector* base = reinterpret_cast<CVector*>(&this->field_5C);

	if (this->field_74 < 0)
	{
		this->mStart = *base;
	}
	else if (this->field_74 <= this->field_7C)
	{
		this->mStart = *base + (*dir * this->field_74);
	}
	else
	{
		this->mStart = *base + (*dir * this->field_7C);
	}

	if (this->field_78 < 0)
	{
		this->mEnd = *base;
	}
	else if (this->field_78 <= this->field_7C)
	{
		this->mEnd = *base + (*dir * this->field_78);
	}
	else
	{
		this->mEnd = *base + (*dir * this->field_7C);
	}

	bool hitPlayer = false;
	bool hitEnv = false;

	if (this->field_74 <= this->field_7C)
	{
		CVector zero(0, 0, 0);
		if (!M3dColij_LineToSphere(&this->mStart, &this->mEnd, &zero, reinterpret_cast<CBody*>(MechList), 0, 0x1000))
			return;

		hitPlayer = true;
	}
	else if (this->field_88)
	{
		hitEnv = true;
	}

	CSniperTarget* pSniper = static_cast<CSniperTarget*>(Mem_RecoverPointer(&this->field_8C));
	CChopper* pChopper = static_cast<CChopper*>(Mem_RecoverPointer(&this->field_94));

	print_if_false(!(pSniper && pChopper), "Both sniper and chopper owner");

	if (hitEnv || hitPlayer)
	{
		new CGlowFlash(&this->mEnd, 5, 0xFFu, 0xFFu, 0xFFu, 0, 0xFFu, 0x40u, 0u, 0, 9, 0, 1, 0xC, 0x28, 6, 0x14, 1, 1);
		new CBulletFrag(&this->mEnd);
		new CSmokeGenerator(&this->mEnd, 5, 2, 80, 70, 64, 5, 20, 1000, 700);

		u32 sfxId;
		if (hitPlayer)
			sfxId = (Rnd(2) ? 37 : 38) | 0x8000;
		else if (pChopper)
			sfxId = (Rnd(2) ? 39 : 40) | 0x8000;
		else
			sfxId = (Rnd(2) ? 0x75 : 0x76) | 0x8000;

		SFX_PlayPos(sfxId, &this->mEnd, 0);
	}

	if (pSniper)
		pSniper->BulletResult(hitPlayer);

	if (pChopper && hitPlayer && MechList->field_8E8)
	{
		CVector dirVec = this->mEnd - this->mStart;
		i32 len = dirVec.Length();
		if (len > 0xE74)
			pChopper->field_37C = 0x12C;
	}

	this->Die();
}

// @Ok
// @Note: rewritten from the Hex-Rays decompile of tools/functions/4328896.bin
// (0x420dc0), fixing several bugs the earlier reconstruction had:
// - field_68/6C/70 (the ray direction, this+104..112) is dir/field_7C
//   (a NORMALIZED direction), not dir*field_7C. Move() already scales this
//   vector by a distance, so the earlier multiply double-scaled it.
// - lineinfo.MinCoords was never zeroed (decompile memsets Min+Max together).
// - field_80/82/84 store the raycast hit NORMAL (lineinfo.Normal, i16), not
//   the hit position; confirmed by the dot-product use of these fields in
//   the crater-spawn block of Move().
// - field_78 was never initialised; original sets field_78 = -Rnd(200) and
//   field_74 = field_78 - 250 (both effectively always negative).
// - field_5C/60/64 (the "base" CVector Move() sweeps mStart/mEnd from) was
//   never written; original sets it to *a2 right before the sweep clamp.
// - the final mStart/mEnd clamp uses the same 3-way (< 0 / <= field_7C / >
//   field_7C) shape as Move(), applied once at construction time.
void CMachineGunBullet::Common(CVector* a2, CVector* a3)
{
	this->field_9C = 4;
	this->SetRGB0(0, 0, 0);
	this->SetRGB1(255, 255, 255);

	this->mCodeBGR0 |= 0x2000000;

	CVector dir = *a3 - *a2;
	this->field_7C = dir.Length();
	print_if_false(this->field_7C != 0, "Zero length in CMachineGunBullet::Common");

	this->field_68 = dir.vx / this->field_7C;
	this->field_6C = dir.vy / this->field_7C;
	this->field_70 = dir.vz / this->field_7C;

	if (this->field_7C < 5000)
		this->field_7C = 5000;

	SLineInfo lineinfo;
	lineinfo.StartCoords = *a2;
	lineinfo.EndCoords.vx = this->field_68 * this->field_7C + a2->vx;
	lineinfo.EndCoords.vy = this->field_6C * this->field_7C + a2->vy;
	lineinfo.EndCoords.vz = this->field_70 * this->field_7C + a2->vz;

	lineinfo.MinCoords.vx = 0;
	lineinfo.MinCoords.vy = 0;
	lineinfo.MinCoords.vz = 0;

	lineinfo.MaxCoords.vx = 0;
	lineinfo.MaxCoords.vy = 0;
	lineinfo.MaxCoords.vz = 0;

	lineinfo.Position.vx = 0;
	lineinfo.Position.vy = 0;
	lineinfo.Position.vz = 0;

	lineinfo.Normal.vx = 0;
	lineinfo.Normal.vy = 0;
	lineinfo.Normal.vz = 0;

	M3dColij_InitLineInfo(&lineinfo);
	M3dZone_LineToItem(&lineinfo, 1);

	if (lineinfo.pItem)
	{
		this->field_88 = 1;
		this->field_80 = lineinfo.Normal.vx;
		this->field_82 = lineinfo.Normal.vy;
		this->field_84 = lineinfo.Normal.vz;

		CVector delta;
		delta.vx = lineinfo.Position.vx - a2->vx;
		delta.vy = lineinfo.Position.vy - a2->vy;
		delta.vz = lineinfo.Position.vz - a2->vz;
		this->field_7C = delta.Length();
	}

	this->field_78 = -Rnd(200);
	this->field_74 = this->field_78 - 250;

	CVector* base = reinterpret_cast<CVector*>(&this->field_5C);
	*base = *a2;

	CVector* dirVec = reinterpret_cast<CVector*>(&this->field_68);

	if (this->field_74 < 0)
	{
		this->mStart = *a2;
	}
	else if (this->field_74 <= this->field_7C)
	{
		this->mStart = *base + (*dirVec * this->field_74);
	}
	else
	{
		this->mStart = *base + (*dirVec * this->field_7C);
	}

	if (this->field_78 < 0)
	{
		this->mEnd = *base;
	}
	else if (this->field_78 <= this->field_7C)
	{
		this->mEnd = *base + (*dirVec * this->field_78);
	}
	else
	{
		this->mEnd = *base + (*dirVec * this->field_7C);
	}
}

// @Ok
INLINE CMachineGunBullet::CMachineGunBullet(CVector* a2, CVector* a3)
{
	this->field_5C = 0;
	this->field_60 = 0;
	this->field_64 = 0;

	this->field_68 = 0;
	this->field_6C = 0;
	this->field_70 = 0;

	this->field_80 = 0;
	this->field_82 = 0;
	this->field_84 = 0;

	this->field_A8 = 0;
	this->field_AC = 0;
	this->field_B0 = 0;

	this->Common(a2, a3);

	this->field_A4 = 15;
}

// @Ok
INLINE CMachineGunBullet::CMachineGunBullet(CVector* a2, CVector* a3, CChopper* pChopper)
{
	this->field_5C = 0;
	this->field_60 = 0;
	this->field_64 = 0;

	this->field_68 = 0;
	this->field_6C = 0;
	this->field_70 = 0;

	this->field_80 = 0;
	this->field_82 = 0;
	this->field_84 = 0;

	this->field_A8 = 0;
	this->field_AC = 0;
	this->field_B0 = 0;

	this->Common(a2, a3);

	this->field_94 = Mem_MakeHandle(static_cast<void*>(pChopper));
	this->field_A4 = 5;
}

// @Ok
INLINE CMachineGunBullet::CMachineGunBullet(CVector* a2, CVector* a3, CSniperTarget* pSniper)
{
	this->field_5C = 0;
	this->field_60 = 0;
	this->field_64 = 0;

	this->field_68 = 0;
	this->field_6C = 0;
	this->field_70 = 0;

	this->field_80 = 0;
	this->field_82 = 0;
	this->field_84 = 0;

	this->field_A8 = 0;
	this->field_AC = 0;
	this->field_B0 = 0;

	this->Common(a2, a3);

	this->field_8C = Mem_MakeHandle(static_cast<void*>(pSniper));
	this->field_A4 = 10;
}

// @Ok
CBulletFrag::CBulletFrag(CVector* a2)
{
	this->mPos = *a2;
	this->SetTexture(0xF5A14AFF);
	this->mScale = Rnd(200) + 350;

	i32 v3 = Rnd(4096);
	i32 v4 = Rnd(10) + 10;

	this->mVel.vx = v4 * rcossin_tbl[v3 & FLATBIT_VELOCITIES_MAX_INDEX].sin;
	this->mVel.vz = v4 * rcossin_tbl[v3 & FLATBIT_VELOCITIES_MAX_INDEX].cos;

	this->mVel.vy = -81920 - (Rnd(30) << 12);
	this->field_5A = 500;

	if (Rnd(2))
		this->field_5A *= -1;

	this->mPostScale = 0xC001000;
	this->mLifetime = Rnd(10) + 10;
}

// @Ok
void CSniperSplat::Move(void)
{
	switch (this->field_84)
	{
		case 0:
			this->field_84 = 1;
			break;
		case 1:
			if (++this->mAge > 30)
				this->field_84 = 2;
			break;
		case 2:
			Bit_ReduceRGB(&this->mTint, 3);
			if (!(0xFFFFFF & this->mTint))
				this->Die();
			break;
		default:
			print_if_false(0, "Bad CSplat mode");
			break;
	}
}

// @Ok
CSniperSplat::CSniperSplat(CVector* a2, SVECTOR* a3)
{
	this->SetTexture(Spool_FindTextureChecksum("WebBall_Crater_01"));
	this->SetTint(64, 64, 64);
	this->SetSubtractiveTransparency();

	i32 first = Rnd(30) + 30;
	i32 second = Rnd(30) + 30;
	i32 third = Rnd(4096);

	this->OrientUsing(a2, a3, first, second, third);

	this->mType = 33;
}

// @Ok
CSniperTarget::~CSniperTarget(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&ControlBaddyList));
}

// @Ok
CSniperTarget::CSniperTarget(i32 a2)
{
	this->field_104.vx = 0;
	this->field_104.vy = 0;
	this->field_104.vz = 0;
	this->field_110 = 0;
	this->field_114 = 0;
	this->field_118 = 0;
	this->field_13C = 0;
	this->field_140 = 0;
	this->field_144 = 0;
	this->field_148 = 0;
	this->field_14C = 0;
	this->field_150 = 0;

	this->mType = 323;
	Trig_GetPosition(&this->mPos, a2);

	u16* LinksPointer = Trig_GetLinksPointer(a2);
	print_if_false(*LinksPointer != 0, "No link for snipertarget");

	Trig_GetPosition(&this->field_104, LinksPointer[1]);

	this->field_11C = Spool_FindTextureEntry("snipertarget02");
	this->field_120 = 180;

	this->AttachTo(reinterpret_cast<CBody**>(&ControlBaddyList));
}

// @Ok
// @Note: verified field by field against the Hex-Rays decompile of
// tools/functions/4347712.bin (0x425740): every zeroed field, mFlags/mType/
// mFric/mCBodyFlags/mRMinor/mNode/field_1F4/field_31C/field_380/field_3B4
// assignment, the mPos copy into field_33C then field_330, and the final
// AttachTo/hooks-packet call all match. The only gap was the unnamed global
// at 0x548F88, now gChopperHooksPacket above.
CChopper::CChopper(i16* a2, i32 a3)
{
	this->field_330.vx = 0;
	this->field_330.vy = 0;
	this->field_330.vz = 0;

	this->field_33C.vx = 0;
	this->field_33C.vy = 0;
	this->field_33C.vz = 0;

	this->field_364.vx = 0;
	this->field_364.vy = 0;
	this->field_364.vz = 0;

	this->field_388.vx = 0;
	this->field_388.vy = 0;
	this->field_388.vz = 0;

	this->field_394.vx = 0;
	this->field_394.vy = 0;
	this->field_394.vz = 0;

	this->field_3A8.vx = 0;
	this->field_3A8.vy = 0;
	this->field_3A8.vz = 0;

	this->field_3B8.vx = 0;
	this->field_3B8.vy = 0;
	this->field_3B8.vz = 0;

	this->field_3C8 = 0;
	this->field_3CC = 0;
	this->field_3D0 = 0;


	this->InitItem("chopper");
	this->mFlags |= 4u;

	this->SquirtAngles(reinterpret_cast<i16*>(this->SquirtPos(a2)));

	this->mType = 318;

	this->mFric.vx = 3;
	this->mFric.vy = 3;
	this->mFric.vz = 3;

	this->mCBodyFlags &= ~0x10u;
	this->mRMinor = 0;
	CBody::AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->field_1F4 = a3;
	this->mNode = a3;
	this->field_31C.bothFlags = 0;

	this->field_380 = 1;
	this->field_3B4 = 2;

	this->field_33C = this->mPos;
	this->field_330 = this->field_33C;

	this->field_360 = 2048;
	this->field_358 = 2048;
	this->mAngles.vy = 2048;
	M3dUtils_ReadHooksPacket(this, gChopperHooksPacket);
}

// @Ok
void Chopper_CreateChopper(const u32* a1, u32* a2)
{
	i16* v3 = reinterpret_cast<i16*>(a1[0]);
	i32 v4 = a1[1];

	*a2 = reinterpret_cast<u32>(new CChopper(v3, v4));
}


// @Ok
void Chopper_CreateSniper(const u32* a1, u32* a2)
{
	i32 v3 = *a1;

	*a2 = reinterpret_cast<u32>(new CSniperTarget(v3));
}

// @Ok
void Chopper_CreateSearchlight(const u32* a1, u32* a2)
{
	i32 v3 = *a1;

	*a2 = reinterpret_cast<u32>(new CSearchlight(v3));
}

// @NotOk
// @Note: reconstructed from tools/functions/4340240.bin. Same
// gte_SetRotMatrix/m3d_ZeroTransVector/gte_ldlv0/gte_rtps/gte_stlvnl2/gte_stsxy screen
// projection idiom as CSniperTarget::DrawTargetRecticle, applied per vertex of
// field_138[] (the CVector[66] light-cone mesh CalculateSearchlight fills). If the beam
// source (field_138[0]) is too close to the camera (depth < 200) the whole draw is
// skipped. Otherwise it resets field_12C (the CheckPointInScreenTri hit flag, re-armed
// every render) and draws the beam as a flat-shaded (PCGfx_UseTexture with no texture)
// triangle strip walking the near/far vertex pairs. This is a best-effort schematic:
// the per-vertex draw call parameters were not traced byte for byte, only the GTE
// transform and the overall loop/early-out shape are grounded in the disassembly.
// cmpsum: 498 mnemonic diffs, first divergence right at the prologue register
// allocation. 1 attempt (structural reconstruction only). Needs real work.
void CSearchlight::SpecialRenderer(void)
{
	gte_SetRotMatrix(gCameraViewMatrix);
	m3d_ZeroTransVector();

	CVector camPos = *gCameraViewPos;
	CVector relPos = (this->field_138[0] >> 12) - camPos;

	gte_ldlv0(reinterpret_cast<VECTOR*>(&relPos));
	gte_rtps();

	i32 depth;
	gte_stlvnl2(&depth);

	i16 screenXY[2];
	gte_stsxy(reinterpret_cast<i32*>(screenXY));

	if (depth < 200)
		return;

	this->field_12C = 0;

	PCGfx_UseTexture(1, DCGfx_BlendingMode_1);

	f32 prevX = static_cast<f32>(screenXY[0]);
	f32 prevY = static_cast<f32>(screenXY[1]);

	for (i32 i = 1; i < 66; i++)
	{
		CVector rel = (this->field_138[i] >> 12) - camPos;

		gte_ldlv0(reinterpret_cast<VECTOR*>(&rel));
		gte_rtps();
		gte_stlvnl2(&depth);
		gte_stsxy(reinterpret_cast<i32*>(screenXY));

		f32 x = static_cast<f32>(screenXY[0]);
		f32 y = static_cast<f32>(screenXY[1]);

		PCGfx_DrawQPoly2D(
				prevX, prevY, 0.0f, 1.0f, 0x40FFFFFFu,
				x, y, 0.0f, 1.0f, 0x40FFFFFFu,
				x, y, 0.0f, 1.0f, 0x40FFFFFFu,
				prevX, prevY, 0.0f, 1.0f, 0x40FFFFFFu,
				0.0f);

		prevX = x;
		prevY = y;
	}
}

// @Ok
CSearchlight::~CSearchlight(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&ControlBaddyList));
}

// @Ok
// @Note: checked field by field against the Hex-Rays decompile of
// tools/functions/4337392.bin (0x422ef0). The kill-flag check, waypoint timer
// (field_100/0xF0, field_104/field_110 swap, field_11C step), CalcAim, and
// CalculateSearchlight call, plus the field_12C/field_130/field_128 gating,
// all matched exactly. Fixed three real bugs in the spark-spawn block:
// (1) the jitter was applied to the wrong end: the original spawns the spark
// FROM camPos (untouched) TO MechList->mPos + jitter, this source had it
// backwards (jittered start, plain end); (2) perpA/perpB were swapped
// between the sin and cos terms (original: perpB*sin, perpA*cos); (3) the
// rcossin_tbl index had a spurious "<< 2" (the original indexes the table
// directly by the angle, word_610C48/610C4A already account for the 4-byte
// SSinCos stride), which could read out of bounds. Also folded the jitter
// magnitude/angle into one Rnd(0x100)/Rnd(0x1000) pair shared by both jitter
// components, matching the original (it does not re-roll per component).
void CSearchlight::AI(void)
{
	if (this->mFlags & 1)
	{
		this->mFlags &= ~1;
		this->Die();
		return;
	}

	this->field_100 += this->field_80;
	if (this->field_100 >= 0xF0)
	{
		u16* LinksPointer = Trig_GetLinksPointer(this->field_FC);
		this->field_100 -= 0xF0;

		print_if_false(LinksPointer[0] != 0, "No path for searchlight");

		this->field_F8 = this->field_FC;
		this->field_104 = this->field_110;
		this->field_FC = LinksPointer[1];
		Trig_GetPosition(&this->field_110, this->field_FC);

		this->field_11C = (this->field_110 - this->field_104) / 0xF0;
	}

	CVector target = this->field_104 + this->field_11C * this->field_100;

	CSVector aimDir;
	aimDir.vx = 0;
	aimDir.vy = 0;
	aimDir.vz = 0;
	Utils_CalcAim(&aimDir, &this->mPos, &target);

	this->CalculateSearchlight(&aimDir);

	if (!this->field_12C)
	{
		this->field_130 = 0;
		return;
	}

	this->field_130 += this->field_80;

	if (this->field_130 <= 0xA)
	{
		return;
	}
	else if (this->field_130 <= 0x1E)
	{
		this->field_128 += this->field_80;
		if (this->field_128 <= 8)
			return;

		this->field_128 -= 8;

		CVector camPos = CameraList->mPos;
		camPos.vy += 0x200000;

		CVector perpA, perpB;
		Utils_CalcPerps(&MechList->field_C84, &perpB, &perpA);

		i32 jitterMag = Rnd(0x100);
		i32 jitterAngle = Rnd(0x1000) & 0xFFF;

		CVector jitterA = perpB * ((jitterMag * rcossin_tbl[jitterAngle].sin) >> 0xC);
		CVector jitterB = perpA * ((jitterMag * rcossin_tbl[jitterAngle].cos) >> 0xC);

		CVector sparkStart = camPos;
		CVector sparkEnd = MechList->mPos + jitterA + jitterB;

		SFX_Play(0x8074, 0x2000, 0);
		new CMachineGunBullet(&sparkStart, &sparkEnd);
	}
	else if (this->field_130 > 0x41)
	{
		this->field_130 = 0xA;
	}
}

// @Ok
CSearchlight::CSearchlight(i32 a2)
{
	this->field_104.vx = 0;
	this->field_104.vy = 0;
	this->field_104.vz = 0;

	this->field_110.vx = 0;
	this->field_110.vy = 0;
	this->field_110.vz = 0;

	this->field_11C.vx = 0;
	this->field_11C.vy = 0;
	this->field_11C.vz = 0;

	for (i32 i = 0; i < 66; i++)
	{
		this->field_138[i].vx = 0;
		this->field_138[i].vy = 0;
		this->field_138[i].vz = 0;
	}

	this->mType = 322;
	this->AttachTo(reinterpret_cast<CBody**>(&ControlBaddyList));

	Trig_GetPosition(&this->mPos, a2);
	u16 *LinksPointer = Trig_GetLinksPointer(a2);
	print_if_false(*LinksPointer == 0, "No path for searchlight");

	this->field_F8 = LinksPointer[1];
	Trig_GetPosition(&this->field_104, this->field_F8);

	u16 *OtherLinks = Trig_GetLinksPointer(this->field_F8);
	print_if_false(*OtherLinks == 0, "No path for searchlight");

	this->field_FC = OtherLinks[1];
	Trig_GetPosition(&this->field_110, this->field_FC);

	this->field_11C = (this->field_110 - this->field_104) / 240;
	this->field_100 = 0;
}

// @Ok
INLINE void CChopper::WaitForTrigger(void)
{
	switch (this->dumbAssPad)
	{
		case 0:

			if (this->field_218 & 4)
			{
				this->field_218 &= 0xFB;
				if (this->GetNextWaypoint())
				{
					this->dumbAssPad = 0;
					this->field_31C.bothFlags = 2;
				}
				else
				{
					this->dumbAssPad = 0;
					this->field_31C.bothFlags = 1;
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
	}
}

// @Ok
void CBulletFrag::Move()
{
  this->mPos.vx += this->mVel.vx;
  this->mPos.vy += this->mVel.vy;
  this->mPos.vz += this->mVel.vz;
  this->mVel.vy += 0x7390;

  this->mAngle += this->field_5A;

  if (++this->mAge > (i32)(u16)this->mLifetime)
	  this->Die();
}

// @Ok
INLINE void CChopper::SetHeightMode(int mode)
{
	this->field_374 = mode;
}

// @Ok
INLINE void CSniperTarget::BulletResult(bool result)
{
	this->field_FC++;
	if (result)
		this->field_128 = true;
}

// @Ok
void INLINE CChopper::AdjustSineWaveAmplitude(int a2, int a3)
{
	int tmp = this->field_354;
	if (tmp != a2)
	{
		this->field_354 = Utils_LinearFilter(tmp, a2, a3);
	}
}

// @Ok
void INLINE CChopper::AngleToTargetAngle(void)
{
	i32 v1 = (this->field_360 & 0xFFF) - (this->mAngles.vy & 0xFFF);

	if (v1 > 2048)
	{
		v1 -= 4096;
	}
	else if (v1 < -2048)
	{
		v1 += 4096;
	}


	this->mAngVel.vy = v1 >> 5;
	this->mAngles.vy += this->mAngVel.vy;
}

void validate_CChopper(void){
	VALIDATE_SIZE(CChopper, 0x3D8);

	VALIDATE(CChopper, field_324, 0x324);
	VALIDATE(CChopper, field_328, 0x328);

	VALIDATE(CChopper, field_32C, 0x32C);

	VALIDATE(CChopper, field_330, 0x330);

	VALIDATE(CChopper, field_33C, 0x33C);

	VALIDATE(CChopper, field_348, 0x348);
	VALIDATE(CChopper, field_34C, 0x34C);

	VALIDATE(CChopper, field_350, 0x350);
	VALIDATE(CChopper, field_354, 0x354);

	VALIDATE(CChopper, field_358, 0x358);
	VALIDATE(CChopper, field_35C, 0x35C);

	VALIDATE(CChopper, field_360, 0x360);
	VALIDATE(CChopper, field_364, 0x364);

	VALIDATE(CChopper, field_370, 0x370);

	VALIDATE(CChopper, field_374, 0x374);
	VALIDATE(CChopper, field_378, 0x378);
	VALIDATE(CChopper, field_37C, 0x37C);

	VALIDATE(CChopper, field_380, 0x380);
	VALIDATE(CChopper, field_384, 0x384);

	VALIDATE(CChopper, field_388, 0x388);

	VALIDATE(CChopper, field_394, 0x394);

	VALIDATE(CChopper, field_3A0, 0x3A0);
	VALIDATE(CChopper, field_3A4, 0x3A4);

	VALIDATE(CChopper, field_3A8, 0x3A8);

	VALIDATE(CChopper, field_3B4, 0x3B4);
	VALIDATE(CChopper, field_3B8, 0x3B8);

	VALIDATE(CChopper, field_3C4, 0x3C4);

	VALIDATE(CChopper, field_3C8, 0x3C8);
	VALIDATE(CChopper, field_3CC, 0x3CC);
	VALIDATE(CChopper, field_3D0, 0x3D0);

	VALIDATE_VTABLE(CChopper, FireMissileAtWaypoint, 17);
	VALIDATE_VTABLE(CChopper, FireMachineGunAtWaypoint, 18);
	VALIDATE_VTABLE(CChopper, SetFlag, 19);
}

void validate_CBulletFrag(void){
	VALIDATE_SIZE(CBulletFrag, 0x68);
}

void validate_CSniperSplat(void){
	VALIDATE_SIZE(CSniperSplat, 0x88);
}

void validate_CSniperTarget(void)
{
	VALIDATE_SIZE(CSniperTarget, 0x15C);

	VALIDATE(CSniperTarget, field_F8, 0xF8);
	VALIDATE(CSniperTarget, field_FC, 0xFC);
	VALIDATE(CSniperTarget, field_100, 0x100);

	VALIDATE(CSniperTarget, field_104, 0x104);
	VALIDATE(CSniperTarget, field_110, 0x110);
	VALIDATE(CSniperTarget, field_114, 0x114);
	VALIDATE(CSniperTarget, field_118, 0x118);
	VALIDATE(CSniperTarget, field_11C, 0x11C);
	VALIDATE(CSniperTarget, field_120, 0x120);
	VALIDATE(CSniperTarget, field_124, 0x124);

	VALIDATE(CSniperTarget, field_128, 0x128);

	VALIDATE(CSniperTarget, field_12C, 0x12C);
	VALIDATE(CSniperTarget, field_130, 0x130);
	VALIDATE(CSniperTarget, field_134, 0x134);
	VALIDATE(CSniperTarget, field_138, 0x138);

	VALIDATE(CSniperTarget, field_13C, 0x13C);
	VALIDATE(CSniperTarget, field_140, 0x140);
	VALIDATE(CSniperTarget, field_144, 0x144);
	VALIDATE(CSniperTarget, field_148, 0x148);
	VALIDATE(CSniperTarget, field_14C, 0x14C);
	VALIDATE(CSniperTarget, field_150, 0x150);

	VALIDATE(CSniperTarget, field_154, 0x154);
	VALIDATE(CSniperTarget, field_158, 0x158);

	VALIDATE_VTABLE(CSniperTarget, DrawTargetRecticle, 5);
}

void validate_CSearchlight(void)
{
	VALIDATE_SIZE(CSearchlight, 0x450);

	VALIDATE(CSearchlight, field_F8, 0xF8);
	VALIDATE(CSearchlight, field_FC, 0xFC);
	VALIDATE(CSearchlight, field_100, 0x100);
	VALIDATE(CSearchlight, field_104, 0x104);
	VALIDATE(CSearchlight, field_110, 0x110);
	VALIDATE(CSearchlight, field_11C, 0x11C);
	VALIDATE(CSearchlight, field_128, 0x128);
	VALIDATE(CSearchlight, field_12C, 0x12C);
	VALIDATE(CSearchlight, field_130, 0x130);
	VALIDATE(CSearchlight, field_134, 0x134);
	VALIDATE(CSearchlight, field_138, 0x138);

	VALIDATE_VTABLE(CSearchlight, SpecialRenderer, 5);
}

void validate_CMachineGunBullet(void)
{
	VALIDATE_SIZE(CMachineGunBullet, 0xB8);

	VALIDATE(CMachineGunBullet, field_5C, 0x5C);
	VALIDATE(CMachineGunBullet, field_60, 0x60);
	VALIDATE(CMachineGunBullet, field_64, 0x64);

	VALIDATE(CMachineGunBullet, field_68, 0x68);
	VALIDATE(CMachineGunBullet, field_6C, 0x6C);
	VALIDATE(CMachineGunBullet, field_70, 0x70);

	VALIDATE(CMachineGunBullet, field_74, 0x74);
	VALIDATE(CMachineGunBullet, field_78, 0x78);
	VALIDATE(CMachineGunBullet, field_7C, 0x7C);

	VALIDATE(CMachineGunBullet, field_80, 0x80);
	VALIDATE(CMachineGunBullet, field_82, 0x82);
	VALIDATE(CMachineGunBullet, field_84, 0x84);

	VALIDATE(CMachineGunBullet, field_88, 0x88);

	VALIDATE(CMachineGunBullet, field_8C, 0x8C);
	VALIDATE(CMachineGunBullet, field_94, 0x94);
	VALIDATE(CMachineGunBullet, field_9C, 0x9C);

	VALIDATE(CMachineGunBullet, field_A4, 0xA4);

	VALIDATE(CMachineGunBullet, field_A8, 0xA8);
	VALIDATE(CMachineGunBullet, field_AC, 0xAC);
	VALIDATE(CMachineGunBullet, field_B0, 0xB0);
}

void validate_CChopperMissile(void)
{
	VALIDATE_SIZE(CChopperMissile, 0x128);

	VALIDATE(CChopperMissile, field_F8, 0xF8);

	VALIDATE(CChopperMissile, field_FC, 0xFC);
	VALIDATE(CChopperMissile, field_100, 0x100);
	VALIDATE(CChopperMissile, field_104, 0x104);
	VALIDATE(CChopperMissile, field_108, 0x108);

	VALIDATE(CChopperMissile, field_10C, 0x10C);

	VALIDATE(CChopperMissile, field_110, 0x110);
	VALIDATE(CChopperMissile, field_11C, 0x11C);

	VALIDATE(CChopperMissile, field_120, 0x120);
	VALIDATE(CChopperMissile, field_124, 0x124);

	VALIDATE_VTABLE(CChopperMissile, DrawTargetRecticle, 5);
}
