#include "spidey.h"
#include "validate.h"
#include "mem.h"
#include "camera.h"
#include "screen.h"
#include "ps2funcs.h"
#include <cmath>
#include <cstring>
#include "ps2lowsfx.h"
#include "ps2redbook.h"
#include "utils.h"
#include "m3dutils.h"
#include "bit.h"
#include "trig.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "ps2lowsfx.h"
#include "spool.h"
#include "DXinit.h"
#include "dcfileio.h"
#include "reloc.h"
#include "baddy.h"
#include "my_assert.h"
#include "texture.h"

// @Ok
EXPORT u16 gSpideyCeilingCameraXOffset;
// @Ok
EXPORT u16 gSpideyCeilingCameraYOffset;
// @Ok
EXPORT u16 gSpideyCeilingCameraZOffset;
// @Ok
EXPORT u16 gSpideyCeilingCameraXZDistance;
// @Ok
EXPORT u16 gSpideyCeilingCameraYDistance;

// @Ok
i32 *gSpideySFXEntry[300];

// @Ok
EXPORT i16 gSpideyFloorCamXOffset;
// @Ok
EXPORT i16 gSpideyFloorCamYOffset;
// @Ok
EXPORT i16 gSpideyFloorCamZOffset;

// @Ok
EXPORT i16 gSpideyFloorCamXZDistance;
// @Ok
EXPORT i16 gSpideyFloorCamYDistance;

// @Ok
EXPORT i16 gSpideySwingCamXOffset;
// @Ok
EXPORT i16 gSpideySwingCamYOffset;
// @Ok
EXPORT i16 gSpideySwingCamZOffset;

// @Ok
EXPORT i16 gSpideySwingCamXZDistance;
// @Ok
EXPORT i16 gSpideySwingCamYDistance;

// @Ok
EXPORT i16 gSpideyWallCamXOffset;
// @Ok
EXPORT i16 gSpideyWallCamYOffset;
// @Ok
EXPORT i16 gSpideyWallCamZOffset;

// @Ok
EXPORT i16 gSpideyWallCamXZDistance;

// @Ok
EXPORT i16 gSpideyWallCamYDistance;


// @Ok
EXPORT u8 gSpideyVramProcessing;

// @Ok
EXPORT SAnimFrame *gSpideyAnim;

// @Ok
EXPORT SAnimFrame *gSpideyAnimTwo;

// @Ok
EXPORT i16 gSpideyFallingCamXOff;
// @Ok
EXPORT i16 gSpideyFallingCamYOff;
// @Ok
EXPORT i16 gSpideyFallingCamZOff;

// @Ok
EXPORT i16 gSpideyFallingCamXZDist;
// @Ok
EXPORT i16 gSpideyFallingCamYDist;


// @Ok
EXPORT SLight M3d_PlayerLight =
{

  { { -2430, -2228, -2430 }, { 2509, -2896, 1447 }, { -648, -3711, -1607 } },

  0,
  { { 3200, 1040, 2048 }, { 2720, 1600, 1920 }, { 2400, 2560, 2048 } },
  0,

  { 1800, 1800, 1440 }
};


CItem* SpideyAdditionalBodyPartsList;
CItem* MiscellaneousRenderingList;

u8 gSpideyPsxIndex;
CPlayer* MechList;
extern i32 CurrentSuit;

EXPORT void *gSpideyHeadModel;

extern CCamera* CameraList;

// @Bogus
void CPlayer::nullsub_one(i32)
{
	printf("void CPlayer::nullsub_one(i32)");
}

// @Ok
void Bruce_Sync(void)
{
	print_if_false(MechList != 0, "NULL pointer");
	MechList->field_D3C = MechList->mPos;
	MechList->field_D4E = MechList->mAngles;
}

// @MEDIUMTODO
void CPlayer::AI(void)
{
    printf("CPlayer::AI(void)");
}

// @Ok
// @NotMatching: the light assingment does not match, didn't care enough
void CPlayer::AdjustBrightness(u16 a2)
{
	// @Ok - according to PPC it's a static variable
	static u32 gPlayerBrightness = -1;
	if (this->field_570 < a2)
	{
		this->field_570 += 8 * this->field_80;

		if (this->field_570 > a2)
		{
			this->field_570 = a2;
		}
	}
	else if (this->field_570 > a2)
	{
		this->field_570 -= 8 * this->field_80;

		if (this->field_570 > a2)
		{
			this->field_570 = a2;
		}
	}

	u32 v5 = this->field_570;
	if (gPlayerBrightness != v5)
	{
		M3d_PlayerLight.ColorMatrix[0][0] = (3200 * v5) >> 8;
		M3d_PlayerLight.ColorMatrix[0][1] = (1040 * v5) >> 8;
		M3d_PlayerLight.ColorMatrix[0][2] = 8 * v5;

		M3d_PlayerLight.ColorMatrix[1][0] = (2720 * v5) >> 8;
		M3d_PlayerLight.ColorMatrix[1][1] = (1600 * v5) >> 8;
		M3d_PlayerLight.ColorMatrix[1][2] = (1920 * v5) >> 8;

		M3d_PlayerLight.ColorMatrix[2][0] = (2400 * v5) >> 8;
		M3d_PlayerLight.ColorMatrix[2][1] = 10 * v5;
		M3d_PlayerLight.ColorMatrix[2][2] = 8 * v5;

		M3d_PlayerLight.BackColor[0] = (1800 * v5) >> 8;
		M3d_PlayerLight.BackColor[1] = (1800 * v5) >> 8;
		M3d_PlayerLight.BackColor[2] = (1440 * v5) >> 8;
		gPlayerBrightness = v5;
	}
}

// @MEDIUMTODO
void CPlayer::BuildOffscreenSpideySenseIndicatorList(void)
{
    printf("CPlayer::BuildOffscreenSpideySenseIndicatorList(void)");
}

// @MEDIUMTODO
CPlayer::CPlayer(void)
{
    printf("CPlayer::CPlayer(void)");
}

// @Ok
// @Test
i32 CPlayer::CalculateIntermediateTrailSteps(CVector *a2,CVector * a3,CVector * a4)
{
	u32 v8 = Utils_Dist(*a3, *a2) >> 5;
	if (v8 > 1)
		v8 = 1;

	i32 len_a3 = a3->Length();
	i32 len_a2 = a2->Length();

	CVector v18;
	switch (v8)
	{
		case 0:
			return 0;
		case 1:

			v18.vx = a3->vx + (a2->vx - a3->vx) / 2;
			v18.vy = a3->vy + (a2->vy - a3->vy) / 2;
			v18.vz = a3->vz + (a2->vz - a3->vz) / 2;

			v8 >>= 8;
			VectorNormal(
					reinterpret_cast<VECTOR*>(&v18),
					reinterpret_cast<VECTOR*>(&v18));

			v18 *= (len_a3 + len_a2) / 2;

			*a4 = (v18 + this->mPos);
			return 1;
		default:
			return 0;
	}
}

// @Ok
// @Test
void CPlayer::CalculateSwingWebParameters(CVector * a2)
{
	VECTOR v3;
	VECTOR v4;
	VECTOR v5;
	v5.vx = (this->mPos.vx - a2->vx) >> 12;
	v5.vy = (this->mPos.vy - a2->vy) >> 12;
	v5.vz = (this->mPos.vz - a2->vz) >> 12;
	VectorNormal(&v5, &v5);
	gte_ldopv1(&v5);
	gte_ldopv2(reinterpret_cast<VECTOR*>(&this->field_DA0));
	gte_op12();
	gte_stlvnl(&v4);
	VectorNormal(&v4, &v4);
	gte_ldopv1(&v5);
	gte_ldopv2(&v4);
	gte_op12();
	gte_stlvnl(&v3);

	this->field_D80.vx = v4.vx;
	this->field_D86.vx = v4.vy;
	this->field_D8C.vx = v4.vz;
	this->field_D80.vy = v5.vx;
	this->field_D86.vy = v5.vy;
	this->field_D8C.vy = v5.vz;
	this->field_D80.vz = v3.vx;
	this->field_D86.vz = v3.vy;
	this->field_D8C.vz = v3.vz;
	this->field_DB8 = abs(v5.vy);
}

// @SMALLTODO
void CPlayer::CalculateTugWebPathPoints(void)
{
    printf("CPlayer::CalculateTugWebPathPoints(void)");
}

// @Ok
// @AlmostMatching: SetTargetTorsoAngleToThisPoint arg pushed one instruction earlier
u8 CPlayer::CheckCeilingJumpingSmashPunch(void)
{
	if (this->field_8EA || !this->field_8E9 && !this->field_8E8)
	{
		return 0;
	}

	if (!this->field_DCC)
	{
		return 0;
	}

	// @FIXME
	u8 *v3 = reinterpret_cast<u8*>(this->field_E0C);
	if (!v3[289] && !v3[305])
	{
		return 0;
	}

	CVector v17 = (this->field_DCC->mPos - this->mPos) >> 12;
	VectorNormal(
			reinterpret_cast<VECTOR*>(&v17),
			reinterpret_cast<VECTOR*>(&v17));

	this->field_AE4 = 0;
	this->field_A8.vx = 0;
	this->field_A8.vy = -4096;
	this->field_A8.vz = 0;

	this->OrientToNormal(0, &ZeroVector);

	this->field_AD4 = 0;
	this->field_8DC = 0;
	this->field_8CC = this->field_DCC->mPos;

	this->SetTargetTorsoAngleToThisPoint(&this->field_8CC);

	this->field_E1C = 0x1000000;
	this->field_8C8 = this->field_8C4;
	this->field_8C4 = gTimerRelated;
	this->field_8D8 = 0;

	// @FIXME
	if (reinterpret_cast<u8*>(this->field_E0C)[289])
		this->PlaySingleAnim(133, 0, -1);
	else
		this->PlaySingleAnim(129, 0, -1);

	return 1;
}

// @MEDIUMTODO
void CPlayer::CheckExteriorSurfaceTransition(void)
{
    printf("CPlayer::CheckExteriorSurfaceTransition(void)");
}

// @SMALLTODO
void CPlayer::CheckFenceSurfaceTransition(void)
{
    printf("CPlayer::CheckFenceSurfaceTransition(void)");
}

// @MEDIUMTODO
void CPlayer::CheckForwards(bool)
{
    printf("CPlayer::CheckForwards(bool)");
}

// @Ok
i32 CPlayer::CheckGroundGone(void)
{
	if (!(this->mCollision & 2))
	{
		if ( this->field_EA4 )
			this->field_EA4--;

		if (this->field_EA4)
			return 0;

		if ( this->mHeldObject )
		{
			CVector v11 = (4 * this->field_C84);
			this->mHeldObject->Drop(&v11);
			this->mHeldObject = 0;
		}

		this->field_E38 = this->mPos.vy;
		this->PlaySingleAnim(212, 0, -1);

		this->field_E8C = 0;
		this->field_AE5 = 0;
		this->field_AE6 = 0;
		if ( this->field_AD4 )
		{
			this->field_AD4 = 0;
			this->field_A8.vx = 0;
			this->field_A8.vy = -4096;
			this->field_A8.vz = 0;

			CVector v11;
			v11.vx = 0;
			v11.vy = 0;
			v11.vz = 4096;
			this->OrientToNormal(true, &v11);
		}

		this->field_E1C = 4;

		return 1;
	}

	return 0;
}

// @MEDIUMTODO
void CPlayer::CheckInteriorSurfaceTransition(void)
{
    printf("CPlayer::CheckInteriorSurfaceTransition(void)");
}

// @MEDIUMTODO
void CPlayer::CheckJump(void)
{
    printf("CPlayer::CheckJump(void)");
}

// @MEDIUMTODO
void CPlayer::CheckJumpingR1ZipWeb(void)
{
    printf("CPlayer::CheckJumpingR1ZipWeb(void)");
}

// @MEDIUMTODO
void CPlayer::CheckJumpingR2ZipWeb(void)
{
    printf("CPlayer::CheckJumpingR2ZipWeb(void)");
}

// @MEDIUMTODO
void CPlayer::CheckJumpingSmashKick(void)
{
    printf("CPlayer::CheckJumpingSmashKick(void)");
}

// @MEDIUMTODO
void CPlayer::CheckJumpingSwingWeb(void)
{
    printf("CPlayer::CheckJumpingSwingWeb(void)");
}

// @MEDIUMTODO
void CPlayer::CheckKick(void)
{
    printf("CPlayer::CheckKick(void)");
}

// @MEDIUMTODO
void CPlayer::CheckLanded(void)
{
    printf("CPlayer::CheckLanded(void)");
}

// @NotOk
// fix type
i32 CPlayer::CheckRunIntoWall(void)
{
	if ( this->mHeldObject )
		return 0;

	u8 v3 = 1;

	if (this->mCollision & 1)
	{
		if ( this->field_B84.vy <= 3400
				&& this->field_B74
				&& this->field_B84.vy >= -2600
				// @FIXME
				&& !(this->field_B8C[3] & 0x40000))
		{

			if (((this->field_C6C.vx - this->field_B84.vx) >> 12) +
					((this->field_C6C.vy - this->field_B84.vy) >> 12) +
					((this->field_C6C.vz - this->field_B84.vz) >> 12) > 3800)
			{
				v3 = 0;
				this->field_AD7 += this->field_80;
			}

			if (this->field_AD7 > 0x14)
			{
				this->field_AD7 = 0;
				this->PlaySingleAnim(14, 0, -1);
				this->field_E1C = 0x80000;
				return 1;
			}
		}

	}

	if (v3)
		this->field_AD7 = 0;
	return 0;
}

// @Ok
i32 CPlayer::CheckStickToCeiling(void)
{
	if ( this->mVel.vy > 0
		|| !(this->mCollision & 0x100)
		|| !this->field_C18
		|| !(reinterpret_cast<u8*>(this->field_E0C)[256])
		|| this->field_C28.vy <= 3400
		|| this->field_C30[3] & 0x40000)
	{
		return 0;
	}

	this->field_AD4 = 1;
	this->field_A8 = this->field_C28;
	this->field_AC8 = this->field_C6C;
	this->OrientToNormal(true, &this->field_AC8);

	this->field_E88 = 0;
	this->field_E84 = 0;

	this->mPos = this->field_C1C;
	this->mPos.vx += this->field_A8.vx * this->field_EA8;
	this->mPos.vy += this->field_A8.vy * this->field_EA8;
	this->mPos.vz += this->field_A8.vz * this->field_EA8;

	if ( this->mAnim == 232 )
		this->PlaySingleAnim(234, 0, -1);
	else
		this->PlaySingleAnim(227, 0, -1);

	if (this->field_E1C & 0x300)
		CameraList->field_12C = -1;

	this->field_E1C = 1;
	SFX_Play(9u, 0x2000, 0);
	this->field_AE5 = 0;
	this->field_54C = 0;

	return 1;
}

// @MEDIUMTODO
void CPlayer::CheckStickToWall(void)
{
    printf("CPlayer::CheckStickToWall(void)");
}

// @MEDIUMTODO
void CPlayer::CheckSwingWebAvailability(SLineInfo *)
{
    printf("CPlayer::CheckSwingWebAvailability(SLineInfo *)");
}

// @SMALLTODO
void CPlayer::CheckSwitchToGrabbedMode(CVector const *,CVector *)
{
    printf("CPlayer::CheckSwitchToGrabbedMode(CVector const *,CVector *)");
}

// @MEDIUMTODO
void CPlayer::CheckWebShot(void)
{
    printf("CPlayer::CheckWebShot(void)");
}

// @NotOk
// residue: header declared this void, real return is u8 (0/1), fixed here.
// prologue, stack layout (sub esp,8), and every field/call address match.
// remaining 66 diffs are pure register-role swaps: the branchless ternary
// for v3 (this->field_E1C != 4 ? 16 : 8) puts the ternary result in eax and
// Distance in ecx in the original, our build swaps them (ecx/eax reversed)
// even though load order (field_E1C then Distance) already matches; same
// swap propagates through the coordinate math below it. tried: explicit
// if/else instead of ternary for v3 (broke the branchless codegen entirely,
// worse: 67 diffs, reverted), single scalar `output` instead of i32[3]
// (fixed the stack size mismatch from 0x14 to the original's 0x8, kept).
u8 CPlayer::CheckZipWebAvailability(SLineInfo *pLineInfo, i32 a2)
{
	i32 v3 = (this->field_E1C != 4) ? 16 : 8;

	if (pLineInfo->Distance <= v3)
		return 0;

	if (pLineInfo->Distance >= a2)
		return 0;

	if (pLineInfo->pFace[3] & 0x40000)
		return 0;

	gte_ldsvrtrow0((const SVECTOR*)&this->field_A8);

	SVECTOR local;
	local.vx = (this->field_C84.vx * this->field_EA8 - this->mPos.vx + pLineInfo->Position.vx) >> 12;
	local.vy = (this->field_C84.vy * this->field_EA8 - this->mPos.vy + pLineInfo->Position.vy) >> 12;
	local.vz = (this->field_C84.vz * this->field_EA8 - this->mPos.vz + pLineInfo->Position.vz) >> 12;

	gte_ldv0(&local);
	gte_rtv0();

	i32 output;
	gte_stlvnl0(&output);

	if (this->field_E1C == 4)
		return 1;

	return output > 0x40;
}

// @Ok
void CPlayer::CollideWithObject(CBody* a2)
{
	CVector v8;

	v8 = (this->mPos - a2->mPos) >> 6;
	VectorNormal(
			reinterpret_cast<VECTOR*>(&v8),
			reinterpret_cast<VECTOR*>(&v8));

	i32 v5 = v8.vz * (this->mVel.vz >> 6) + v8.vx * (this->mVel.vx >> 6);
	if (v5 <= 0)
	{
		v5 >>= 12;
		this->mVel.vx -= (v5 * v8.vx) >> 6;
		this->mVel.vz -= (v5 * v8.vz) >> 6;
	}
}

// @MEDIUMTODO
void CPlayer::CreateCombatImpactEffect(CVector *,i32)
{
    printf("CPlayer::CreateCombatImpactEffect(CVector *,i32)");
}

// @MEDIUMTODO
void CPlayer::CreateWebDrips(bool,bool)
{
    printf("CPlayer::CreateWebDrips(bool,bool)");
}

// @MEDIUMTODO
void CPlayer::DoMGSShadow(void)
{
    printf("CPlayer::DoMGSShadow(void)");
}

// @MEDIUMTODO
void CPlayer::DoShadowCheck(void)
{
    printf("CPlayer::DoShadowCheck(void)");
}

// @MEDIUMTODO
void CPlayer::DrawOffscreenSpideySenseIndicatorList(void)
{
    printf("CPlayer::DrawOffscreenSpideySenseIndicatorList(void)");
}

// @MEDIUMTODO
void CPlayer::DrawReticle(u16,u16,u32)
{
    printf("CPlayer::DrawReticle(u16,u16,u32)");
}

// @MEDIUMTODO
void CPlayer::EnterLookaroundMode(void)
{
    printf("CPlayer::EnterLookaroundMode(void)");
}

// @MEDIUMTODO
void CPlayer::FireWeb(bool,i32,CVector *,bool,CSVector *)
{
    printf("CPlayer::FireWeb(bool,i32,CVector *,bool,CSVector *)");
}

// @SMALLTODO
void CPlayer::GetComboFrameInfoPointer(u16)
{
    printf("CPlayer::GetComboFrameInfoPointer(u16)");
}

// @SMALLTODO
void CPlayer::GetComboPartsInfoPointer(u16)
{
    printf("CPlayer::GetComboPartsInfoPointer(u16)");
}

// @Ok
// @Matching
i32 CPlayer::GetDamageInflictedFromDifficulty(i32 a2)
{
	if (CurrentSuit == 2 || CurrentSuit == 3 || CurrentSuit == 4)
	{
		a2 *= 2;
	}

	if (DifficultyLevel != 2)
	{
		if (!DifficultyLevel)
		{
			return a2 << 13 >> 12;
		}

		i32 dmg = a2 * 3;

		if (DifficultyLevel == 1)
		{
			return dmg << 11 >> 12;
		}

		return dmg << 10 >> 12;
	}

	return a2;
}

// @SMALLTODO
void CPlayer::GetEnterExitFrameInfoPointer(u16)
{
    printf("CPlayer::GetEnterExitFrameInfoPointer(u16)");
}

// @Ok
// @Matching
i32 CPlayer::GetFreeIndicatorListEntry(void)
{
	for (i32 i = 0; i < 6; i++)
	{
		if (!this->field_5F0[i].field_C.pWhatever)
		{
			this->field_5F0[i].mInUse = 0;
			return i;
		}
	}

	return -1;
}

// @Ok
INLINE i32* CPlayer::GetNewCommandBlock(u32 a1)
{
	i32* res = static_cast<i32*>(DCMem_New(4 * a1, 0, 1, 0, 1));
	res[a1 - 1] = 0;

	if (!this->field_1BC)
	{
		this->field_1BC = res;
	}
	else
	{
		i32* it = this->field_1BC;
		while (1)
		{
			if (!it[it[1] - 1])
				break;

			it = reinterpret_cast<i32*>(it[it[1] - 1]);
		}

		it[it[1] - 1] = reinterpret_cast<i32>(res);
	}

	return res;
}

// @SMALLTODO
void CPlayer::GetPerpendicularisationRadius(void)
{
    printf("CPlayer::GetPerpendicularisationRadius(void)");
}

// @MEDIUMTODO
void CPlayer::GrabUpdate(CVector *,i16 *)
{
    printf("CPlayer::GrabUpdate(CVector *,i16 *)");
}

// @SMALLTODO
void CPlayer::HandleControlsForSurfaceTransition(bool)
{
    printf("CPlayer::HandleControlsForSurfaceTransition(bool)");
}

// @MEDIUMTODO
i32 CPlayer::Hit(SHitInfo *)
{
    printf("CPlayer::Hit(SHitInfo *)");
    return 0x04082024;
}

// @Ok
// @Matching
u8 CPlayer::IfPlayerCeilingCheck(i32 a2, i32 a3)
{
	DoAssert(a2 <= a3, "Bad min and max for C_IF_PLAYER_CEILING_CHECK");
	if (!this->field_8EA || this->field_CB4)
	{
		if (this->mPos.vy >= a2 && this->mPos.vy <= a3)
		{
			if (this->field_8E9 || this->field_8E8 && this->field_B84.vy > 3400)
			{
				return 1;
			}
		}
	}

	return 0;
}

// @Ok
// @Matching
i32 CPlayer::IncHealth(i32 a2)
{
	if (this->mHealth < this->mMaxHealth && this->mHealth > 0)
	{
		this->mHealth += a2;

		if (this->mHealth > this->mMaxHealth)
		{
			this->mHealth = this->mMaxHealth;
		}

		this->field_5E0 = gTimerRelated;
		this->field_5D0++;
		return 1;
	}

	return 0;
}

// @NotOk
// residue: original keeps two independent per-iteration registers (an
// ascending bound counter esi, tested against 0x60, and a value eax
// freshly recomputed as 0x60-esi each pass); every source form tried here
// (ascending for, do-while, independent counters, != and unsigned compares,
// index*stride) gets fused by our compiler into one descending counter,
// which changes both the loop compare and the stored value's derivation.
// volatile on the counter stops the fusion but adds a stack spill (extra
// sub esp,8 prologue and [esp] reloads) the original does not have.
// 7 distinct hypotheses tried, none reproduce the original register split.
void CPlayer::InitialiseOffscreenSpideySenseIndicatorList(void)
{
	SIndicator *pIndicator = this->field_5F0;

	for (i32 i = 6; i != 0; i--)
	{
		i32 *pEntry = (i32*)((u8*)pIndicator + 0x18);

		for (i32 j = 0; j < 0x60; j += 0x18)
		{
			pEntry[1] = 0;
			pEntry[0] = 0x60 - j;

			setPolyF3();
			setSemiTrans();

			pEntry = (i32*)((u8*)pEntry + 0x14);
		}

		pIndicator++;
	}
}

// @SMALLTODO
void CPlayer::InitialiseSFXArray(void)
{
    printf("CPlayer::InitialiseSFXArray(void)");
}

// @MEDIUMTODO
void CPlayer::InitiateCombo(u16,i32)
{
    printf("CPlayer::InitiateCombo(u16,i32)");
}

// @Ok
// @Matching
u8 CPlayer::IsInIndicatorList(SHandle &a2)
{
	for (i32 i = 0; i < 6; i++)
	{
		if (this->field_5F0[i].field_C.pWhatever && this->field_5F0[i].field_C.Id == a2.Id)
		{
			return 1;
		}
	}

	return 0;
}

// @Ok
// @Note: PlaySingleAnim is cooked
u8 CPlayer::KnockSpideyFromCrawlPosition(void)
{
	if (!this->field_AD4 || !this->field_8E8 && !this->field_8E9)
	{
		return 0;
	}

	if (this->field_8EA)
	{
		this->ExitLookaroundMode();
	}

	this->field_AD4 = 0;
	this->field_E1C = 0x800000;

	this->field_A8.vx = 0;
	this->field_A8.vy = -4096;
	this->field_A8.vz = 0;

	this->PlaySingleAnim(175, 0, -1);
	this->field_AE5 = 1;

	if (this->field_8E8)
	{
		this->OrientToNormal(1, &this->field_C84);
		this->field_8E8 = 0;
	}
	else if (this->field_8E9)
	{
		this->OrientToNormal(1, &this->field_C6C);
		this->field_8E9 = 0;
	}

	return 1;
}

// @SMALLTODO
void CPlayer::LockTargetTorsoAngle(void)
{
    printf("CPlayer::LockTargetTorsoAngle(void)");
}

// globals for CPlayer::NotifyKill below (no idb_globals.txt entries nearby,
// all tentative names, guessed from usage):
// gKillTaunt* (0x55649C..0x5564E1): six 2-byte-stride {group,variant} pick
// tables, one per (early/late damage window) x (a2 special id) combination.
// gKillTauntHistory1..5 (0x6A7FE8..0x6A7FF8): last 5 played sound ids, used
// to avoid repeats.
// gKillTauntLastVariant (0x6A9070): last picked variant index (write-only
// here).
// gKillNotifyCallCount (0x60CFBC): call counter, incremented on every call
// regardless of outcome.
static u8 * const gKillTauntTableEarlySpecial = (u8*)0x005564D0;
static u8 * const gKillTauntTableEarly144 = (u8*)0x005564D8;
static u8 * const gKillTauntTableEarlyOther = (u8*)0x0055649C;
static u8 * const gKillTauntTableLateSpecial = (u8*)0x005564D4;
static u8 * const gKillTauntTableLate144 = (u8*)0x005564E0;
static u8 * const gKillTauntTableLateOther = (u8*)0x005564BC;
static i32 * const gKillTauntHistory1 = (i32*)0x006A7FE8;
static i32 * const gKillTauntHistory2 = (i32*)0x006A7FEC;
static i32 * const gKillTauntHistory3 = (i32*)0x006A7FF0;
static i32 * const gKillTauntHistory4 = (i32*)0x006A7FF4;
static i32 * const gKillTauntHistory5 = (i32*)0x006A7FF8;
static i32 * const gKillTauntLastVariant = (i32*)0x006A9070;
static i32 * const gKillNotifyCallCount = (i32*)0x0060CFBC;

// @NotOk
// residue: 122 mnemonic diffs. the baddy-list scan, the two damage-window
// conditions, all six table picks, the repeat-check against the history and
// the final shift+play all match structurally (same globals, same call
// targets, same table addresses, same branch conditions), but the original
// spills "elapsed" (gTimerRelated - field_35C) to a stack slot and reloads
// it from there on every retry through the pick loop, while our build keeps
// it live in a register across retries instead. See attempts log for what
// was tried.
void CPlayer::NotifyKill(u16 a2)
{
	if (this->field_354 && Rnd(2))
	{
		CBaddy *b = BaddyList;

		while (b)
		{
			if ((b->mCBodyFlags & 0x200) && b->mHealth > 0 && (b->field_2A8 & 0x20))
				goto done;

			b = (CBaddy*)b->mNextItem;
		}

		{
			i32 elapsed = gTimerRelated - this->field_35C;
			i32 groupIndex;
			i32 variantIndex;
			bool checkRepeat;
			i32 soundId;

retry:
			checkRepeat = true;

			if (elapsed < 0xF0 && (this->field_358 - this->mHealth) < 0xA)
			{
				if (a2 == 0x132 || a2 == 0x140)
				{
					i32 idx = Rnd(4) & 0xFE;
					groupIndex = gKillTauntTableEarlySpecial[idx];
					variantIndex = gKillTauntTableEarlySpecial[idx + 1];
					checkRepeat = false;
				}
				else if (a2 == 0x144)
				{
					i32 idx = Rnd(8) & 0xFE;
					groupIndex = gKillTauntTableEarly144[idx];
					variantIndex = gKillTauntTableEarly144[idx + 1];
					checkRepeat = false;
				}
				else
				{
					i32 idx = Rnd(0x20) & 0xFE;
					groupIndex = gKillTauntTableEarlyOther[idx];
					variantIndex = gKillTauntTableEarlyOther[idx + 1];
				}
			}
			else if (elapsed > 0x4B0 && (this->field_358 - this->mHealth) > 0x32)
			{
				if (a2 == 0x132 || a2 == 0x140)
				{
					i32 idx = Rnd(4) & 0xFE;
					groupIndex = gKillTauntTableLateSpecial[idx];
					variantIndex = gKillTauntTableLateSpecial[idx + 1];
					checkRepeat = false;
				}
				else if (a2 == 0x144)
				{
					i32 idx = Rnd(4) & 0xFE;
					groupIndex = gKillTauntTableLate144[idx];
					variantIndex = gKillTauntTableLate144[idx + 1];
					checkRepeat = false;
				}
				else
				{
					i32 idx = Rnd(0x14) & 0xFE;
					groupIndex = gKillTauntTableLateOther[idx];
					variantIndex = gKillTauntTableLateOther[idx + 1];
				}
			}
			else
			{
				goto done;
			}

			soundId = (groupIndex << 4) + variantIndex;
			*gKillTauntLastVariant = variantIndex;

			if (checkRepeat &&
				(*gKillTauntHistory1 == soundId ||
				 *gKillTauntHistory2 == soundId ||
				 *gKillTauntHistory3 == soundId ||
				 *gKillTauntHistory4 == soundId ||
				 *gKillTauntHistory5 == soundId))
			{
				goto retry;
			}

			{
				i32 h2 = *gKillTauntHistory2;
				i32 h3 = *gKillTauntHistory3;
				i32 h4 = *gKillTauntHistory4;
				i32 h5 = *gKillTauntHistory5;

				*gKillTauntHistory1 = h2;
				*gKillTauntHistory2 = h3;
				*gKillTauntHistory3 = h4;
				*gKillTauntHistory4 = h5;
				*gKillTauntHistory5 = soundId;

				Redbook_XAPlay(groupIndex, variantIndex, 0x14);
			}
		}
	}

done:
	(*gKillNotifyCallCount)++;
}

// @MEDIUMTODO
void CPlayer::ParseFightData(void)
{
    printf("CPlayer::ParseFightData(void)");
}

// @SMALLTODO
void CPlayer::ProcessSFXArray(void)
{
    printf("CPlayer::ProcessSFXArray(void)");
}

// @MEDIUMTODO
void CPlayer::ReadAnalogueInput(void)
{
    printf("CPlayer::ReadAnalogueInput(void)");
}

// @SMALLTODO
void CPlayer::SelectAutoAimTarget(void)
{
    printf("CPlayer::SelectAutoAimTarget(void)");
}

// @SMALLTODO
void CPlayer::SelectTargetBaddy(i32,i32,i32,i32)
{
    printf("CPlayer::SelectTargetBaddy(i32,i32,i32,i32)");
}

// @MEDIUMTODO
void CPlayer::SelectTargetSwitch(i32,i32,SHandle *,i32,i32)
{
    printf("CPlayer::SelectTargetSwitch(i32,i32,SHandle *,i32,i32)");
}

// @Ok
EXPORT u8 gSpideyArmorSet;

// @Ok
// @Matching
u8 CPlayer::SetArmor(bool a2)
{
	gSpideyAnimTwo = 0;
	if (a2)
	{
		gSpideyAnimTwo = Spool_FindAnim("costarm", 1);
		switch (DifficultyLevel)
		{
			case 0:
				this->field_5EC = 600;
				break;
			case 1:
				this->field_5EC = 200;
				break;
			case 2:
				this->field_5EC = 100;
				break;
			case 3:
				this->field_5EC = 80;
				break;
			default:
				break;
		}
	}

	if (a2 && gSpideyArmorSet)
	{
		return 1;
	}

	if (!a2 && !gSpideyArmorSet)
	{
		return 1;
	}

	Spidey_DoArmorVRAMProcessing(a2);
	this->field_5E9 = a2;
	gSpideyArmorSet = a2;

	return 1;
}

// @Ok
// @Matching
void CPlayer::SetCeilingCamera(i32 a3)
{
	CCamera *pCamera = CameraList;
	if (pCamera->mCameraMode == 3)
	{
		pCamera->SetCamXOffset(gSpideyCeilingCameraXOffset, a3);
		pCamera->SetCamYOffset(gSpideyCeilingCameraYOffset, a3);
		pCamera->SetCamZOffset(gSpideyCeilingCameraZOffset, a3);
		pCamera->SetCamXZDistance(gSpideyCeilingCameraXZDistance, a3);
		pCamera->SetCamYDistance(gSpideyCeilingCameraYDistance, a3);
		this->field_540 = 2;
	}
}

// @Ok
// @Matching
void CPlayer::SetFloorCamera(i32 a3)
{
	CCamera *pCamera = CameraList;
	if (pCamera)
	{
		if (pCamera->mCameraMode == 3)
		{
			pCamera->SetCamXOffset(gSpideyFloorCamXOffset, a3);
			pCamera->SetCamYOffset(gSpideyFloorCamYOffset, a3);
			pCamera->SetCamZOffset(gSpideyFloorCamZOffset, a3);
			pCamera->SetCamXZDistance(gSpideyFloorCamXZDistance, a3);
			pCamera->SetCamYDistance(gSpideyFloorCamYDistance, a3);
			this->field_540 = 0;
		}
	}
}

// @Ok
// @Matching
void CPlayer::SetFirstContactDetails(void)
{
	if (!this->field_354)
	{
		this->field_354 = 1;
		this->field_358 = this->mHealth;
		this->field_35C = gTimerRelated;
	}
}

// @Ok
// @Matching
void CPlayer::SetFallingCamera(i32 a3)
{
	CCamera *pCamera = CameraList;
	if (pCamera->mCameraMode == 3)
	{
		pCamera->SetCamXOffset(gSpideyFallingCamXOff, a3);
		pCamera->SetCamYOffset(gSpideyFallingCamYOff, a3);
		pCamera->SetCamZOffset(gSpideyFallingCamZOff, a3);

		pCamera->SetCamXZDistance(gSpideyFallingCamXZDist, a3);
		pCamera->SetCamYDistance(gSpideyFallingCamYDist, a3);

		this->field_540 = 5;
	}
}

// @Ok
// @Matching
void CPlayer::SetFocusLockTarget(const CBody *a2)
{
	this->hLockTarget = Mem_MakeHandle(const_cast<CBody*>(a2));
}

// @MEDIUMTODO
void CPlayer::SetSpideyCamValue(u16,u16,i16,u16,u16)
{
    printf("CPlayer::SetSpideyCamValue(u16,u16,i16,u16,u16)");
}

// @Ok
// @matching
void CPlayer::SetSwingCamera(i32 a3)
{
	CCamera *pCamera = CameraList;
	if (pCamera->mCameraMode == 3)
	{
		pCamera->SetCamXOffset(gSpideySwingCamXOffset, a3);
		pCamera->SetCamYOffset(gSpideySwingCamYOffset, a3);
		pCamera->SetCamZOffset(gSpideySwingCamZOffset, a3);
		pCamera->SetCamXZDistance(gSpideySwingCamXZDistance, a3);
		pCamera->SetCamYDistance(gSpideySwingCamYDistance, a3);
		this->field_540 = 4;
	}
}

// @Ok
// @Matching
void CPlayer::SetWallCamera(i32 a3)
{
	CCamera *pCamera = CameraList;
	if (pCamera->mCameraMode == 3)
	{
		pCamera->SetCamXOffset(gSpideyWallCamXOffset, a3);
		pCamera->SetCamYOffset(gSpideyWallCamYOffset, a3);
		pCamera->SetCamZOffset(gSpideyWallCamZOffset, a3);
		pCamera->SetCamXZDistance(gSpideyWallCamXZDistance, a3);
		pCamera->SetCamYDistance(gSpideyWallCamYDistance, a3);
		this->field_540 = 1;
	}
}

// @MEDIUMTODO
void CPlayer::SetupLookaroundCamera(void)
{
    printf("CPlayer::SetupLookaroundCamera(void)");
}

// @Ok
// @Matching
u8 CPlayer::ShouldPlayerDropFlail(void)
{
	return Utils_GetGroundHeight(&this->mPos, 0, 4096, 0) != -1;
}

// @SMALLTODO
void CPlayer::SortAnimationFollowOnData(void)
{
    printf("CPlayer::SortAnimationFollowOnData(void)");
}

// @SMALLTODO
void CPlayer::SortFistsData(void)
{
    printf("CPlayer::SortFistsData(void)");
}

// helper for CPlayer::SwitchToDeathMode/SwitchToSynthesizedInput below: the
// original does "read vtable[0], call with arg 1" (scalar deleting
// destructor) on untyped pointers. SVTableSlot0Deletable is a throwaway
// class with nothing but a virtual destructor, so `delete` on a pointer
// cast to it reproduces that exact call shape without needing the
// __thiscall keyword (rejected by this build's compiler flags, error
// C4234).
struct SVTableSlot0Deletable
{
	virtual ~SVTableSlot0Deletable() {}
};

// @NotOk
// residue: 88 mnemonic diffs (down from 122 on the first honest pass). the
// entire early-out path (a2==true), the field_54C reset path, the
// KnockSpideyFromCrawlPosition path and the field_E1C in {2,4} case match
// byte for byte. the remaining diffs are all one cascade from a single
// instruction: the third equality check in the field_E1C>0x10 chain
// (state==0x800000, written as two chained `state -= 0x40` then
// `state -= 0x7FFF80`, matching the original's own subtract-chain shape)
// compiles to `add eax,0FF800080h; test eax,eax; jne` instead of the
// original's `sub eax,7FFF80h; je`, an extra `test` the original does not
// have. tried: direct equality compare instead of the subtract (worse, 89),
// compound assignment in the condition (no change), a fresh local instead
// of reusing `state` (no change). left as residue, see attempts log.
void CPlayer::SwitchToDeathMode(bool a2)
{
	if (a2)
	{
		u32 levelGroup = (u32)Trig_GetLevelID() >> 8;

		if (levelGroup >= 9 && levelGroup <= 0x17)
		{
			Reloc_CallUserFunction((char*)0x556A90, 1, 0, 0);
			return;
		}

		gLevelStatus = 2;
		return;
	}

	bool wasDying = this->field_54C != 0;
	this->mHealth = 0;

	if (wasDying)
	{
		i32 *p = gSpideySFXEntry[0xB0];
		this->field_54C = 0;
		this->field_E1C = 0x800000;
		this->field_350 = p;

		if (p)
		{
			while (p[0] != -1)
			{
				p[0] &= 0xFFFF;
				p++;
			}
		}

		this->RunAnim(0xB0, 0, -1);

		delete reinterpret_cast<SVTableSlot0Deletable*>(this->field_E64);
		this->field_E64 = 0;

		*(i32*)((u8*)CameraList + 0x12C) = -1;
		return;
	}

	if (this->KnockSpideyFromCrawlPosition())
	{
		i32 *p = gSpideySFXEntry[0xB0];
		this->field_350 = p;

		if (p)
		{
			while (p[0] != -1)
			{
				p[0] &= 0xFFFF;
				p++;
			}
		}

		this->RunAnim(0xB0, 0, -1);
		return;
	}

	u32 state = this->field_E1C;

	if (state <= 0x10)
	{
		if (state == 0x10)
		{
			goto caseBig;
		}

		state--;

		if ((u32)state > 7)
		{
			goto caseDefault;
		}

		switch (state)
		{
			case 0:
			case 7:
				goto caseBig;

			case 1:
			case 3:
				goto caseSmall;

			default:
				goto caseDefault;
		}
	}
	else
	{
		state -= 0x40;

		if (state == 0)
			goto caseBig;

		state -= 0x40;

		if (state == 0)
			return;

		state -= 0x7FFF80;

		if (state == 0)
			goto caseBig;

		goto caseDefault;
	}

caseSmall:
	{
		if (this->mAnim == 0xB0)
			return;

		i32 *p = gSpideySFXEntry[0xB0];
		this->field_350 = p;

		if (p)
		{
			while (p[0] != -1)
			{
				p[0] &= 0xFFFF;
				p++;
			}
		}

		this->RunAnim(0xB0, 0, -1);
		this->field_E1C = 4;
		return;
	}

caseBig:
	{
		if (this->mAnim != 0xB0 && this->mAnim != 0xB2)
		{
			i32 *p = gSpideySFXEntry[0xAB];
			this->mVel.vx = 0;
			this->mVel.vy = 0;
			this->mVel.vz = 0;
			this->field_E1C = 0x80;
			this->field_350 = p;

			if (p)
			{
				while (p[0] != -1)
				{
					p[0] &= 0xFFFF;
					p++;
				}
			}

			this->RunAnim(0xAB, 0, -1);
			SFX_PlayPos(0x24, (CVector*)((u8*)this + 8), 0);
			return;
		}

		i32 *p = gSpideySFXEntry[0xB6];
		this->mVel.vx = 0;
		this->mVel.vy = 0;
		this->mVel.vz = 0;
		this->field_E1C = 0x80;
		this->field_350 = p;

		if (p)
		{
			while (p[0] != -1)
			{
				p[0] &= 0xFFFF;
				p++;
			}
		}

		this->RunAnim(0xB6, 0, -1);
		SFX_PlayPos(9, (CVector*)((u8*)this + 8), 0);
		SFX_PlayPos(0x24, (CVector*)((u8*)this + 8), 0);
		return;
	}

caseDefault:
	{
		i32 *p = gSpideySFXEntry[0xAB];
		this->mVel.vx = 0;
		this->mVel.vy = 0;
		this->mVel.vz = 0;
		this->field_E1C = 0x80;
		this->field_350 = p;

		if (p)
		{
			while (p[0] != -1)
			{
				p[0] &= 0xFFFF;
				p++;
			}
		}

		this->RunAnim(0xAB, 0, -1);
		SFX_PlayPos(0x24, (CVector*)((u8*)this + 8), 0);
		return;
	}
}

// @NotOk
// residue: 92 mnemonic diffs on one honest pass, not iterated further
// given the function's size (319 bytes, medium tier) and the amount of
// still-undocumented struct territory it touches. instruction counts match
// (106 original, 106 built), so nothing is missing or extra, this is pure
// scheduling/register-allocation residue: notably the compiler hoists the
// pInput/field_1B8 store to the very top of the function (cheapest
// dependency, no other value ready yet) ahead of the mVel zeroing, even
// though both are written in the same order as the original disassembly.
// new header field: field_AB8 (SHandle) carved out of the old
// 0x8ED-0xAC8 padding block (matches the Mem_RecoverPointer/Mem_MakeHandle
// call shapes exactly, see validate_CPlayer). field_1A4, field_1B4,
// field_1B8 (i16*, this is where the pInput parameter gets stored) are
// NOT carved out (still raw offsets into existing padding) since there
// was not enough context from this one function alone to name them with
// confidence; done via explicit pointer casts instead.
// this->mVel is CBody's (ob.h) field at 0x60-0x6B, matches the three
// field_60/64/68 zero stores exactly.
// the two vtable[0] calls (on Mem_RecoverPointer's result and on
// mHeldObject) use SVTableSlot0Deletable above; mHeldObject is declared
// CManipOb* with its own virtual destructor but a plain `delete` on it is
// not safe here, since CBody virtuals earlier in the hierarchy could put
// the destructor at a different vtable slot than the one this disassembly
// reads directly at offset 0.
// gSpideySFXEntry[21] (0x6A830C) and gSpideySFXEntry[0] (0x6A82B8) are
// both inside the already-declared gSpideySFXEntry[300] array (top of this
// file) - both addresses land exactly on an element boundary, so no new
// global was needed for either. RunAnim (CSuper, ob.h) argument order
// confirmed from the push sequence (cdecl reverses declaration order).
void CPlayer::SwitchToSynthesizedInput(i16 *pInput)
{
	this->mVel.vx = 0;
	this->mVel.vy = 0;
	this->mVel.vz = 0;

	this->field_1AC = 1;
	*((u8*)this + 0x1B4) = 1;
	*(i16**)((u8*)this + 0x1B8) = pInput;

	this->field_AE5 = 0;
	*((u8*)this + 0x1A4) = 0;
	this->field_1A8 = 0;

	CSmokeTrail **ppTrail = &this->field_584;

	for (i32 i = 2; i != 0; i--)
	{
		if (*ppTrail)
		{
			(*ppTrail)->mFadeAway = 1;
			*ppTrail = 0;
		}

		ppTrail++;
	}

	void *pRecovered = Mem_RecoverPointer(&this->field_AB8);

	if (pRecovered)
	{
		delete reinterpret_cast<SVTableSlot0Deletable*>(pRecovered);
		this->field_AB8 = Mem_MakeHandle(0);
	}

	if (this->mHeldObject)
	{
		delete reinterpret_cast<SVTableSlot0Deletable*>(this->mHeldObject);
		this->mHeldObject = 0;

		if (this->field_E1C & 0x10)
		{
			i32 *p = gSpideySFXEntry[21];
			this->field_350 = p;

			if (p)
			{
				while (p[0] != -1)
				{
					p[0] &= 0xFFFF;
					p++;
				}
			}

			this->RunAnim(0x15, 0, -1);
		}
		else
		{
			i32 *p = gSpideySFXEntry[0];
			this->field_350 = p;

			if (p)
			{
				while (p[0] != -1)
				{
					p[0] &= 0xFFFF;
					p++;
				}
			}

			this->RunAnim(0, 0, -1);
		}
	}

	u8 *pClear = (u8*)this + 0x1C0;

	for (i32 j = 20; j != 0; j--)
	{
		pClear[0] = 0;
		pClear[1] = 0;
		pClear[2] = 0;

		pClear += 0x10;
	}
}

// @MEDIUMTODO
void CPlayer::SynthesizeAnalogueInput(void)
{
    printf("CPlayer::SynthesizeAnalogueInput(void)");
}

// @MEDIUMTODO
void CPlayer::UpdateAndTrackCombo(void)
{
    printf("CPlayer::UpdateAndTrackCombo(void)");
}

// @SMALLTODO
void CPlayer::UpdateOffscreenSpideySenseIndicatorList(void)
{
    printf("CPlayer::UpdateOffscreenSpideySenseIndicatorList(void)");
}

// @MEDIUMTODO
void CPlayer::UpdateTrails(void)
{
    printf("CPlayer::UpdateTrails(void)");
}

// @MEDIUMTODO
CPlayer::~CPlayer(void)
{
    printf("CPlayer::~CPlayer(void)");
}

// globals for Spidey_BagHead below (no idb_globals.txt entry, tentative
// names from usage):
// gBagHeadModeOne/Two: bool flags for a2==1 / a2==2, stored at 0x60CFF4/F8.
// gCurrentCostumeRegionIndex (0x6B4679): u8 index into CItemRelatedList
// (already named in ob.h, stride 0x44/68, same array Spidey_SwapSuitTextures
// indexes the same way).
// gBagHeadScaleFactor (0x556280): stores a1 verbatim, write-only here.
// gBagHeadOffsetTable1/2 (0x556284 / 0x556368): i16[3]-per-entry lookup
// tables selected by a2==1 / a2==2, walked in lockstep with the vertex loop.
static i32 * const gBagHeadModeOne = (i32*)0x0060CFF4;
static i32 * const gBagHeadModeTwo = (i32*)0x0060CFF8;
static u8 * const gCurrentCostumeRegionIndex = (u8*)0x006B4679;
static i32 * const gBagHeadScaleFactor = (i32*)0x00556280;
static i16 * const gBagHeadOffsetTable1 = (i16*)0x00556284;
static i16 * const gBagHeadOffsetTable2 = (i16*)0x00556368;

// @NotOk
// known blocker: this calls print_if_false, which our compiler always
// inlines (it is static in export.h) while the original calls it out of
// line (see CLAUDE.md "print_if_false inlining" note). that alone rules
// out a full match here, independent of anything else in this function.
// residue beyond print_if_false: the source pointer (into gSpideyHeadModel)
// is computed in the original via a two-step subtract-then-add that
// algebraically cancels down to (gSpideyHeadModel+2); written here as the
// simplified direct form, which is very unlikely to reproduce the exact
// original instruction sequence, but it is functionally identical to it.
void Spidey_BagHead(i32 a1, i32 a2)
{
	print_if_false(gSpideyHeadModel != 0, "Error");

	*gBagHeadModeOne = (a2 == 1);
	*gBagHeadModeTwo = (a2 == 2);

	u8 regionIndex = *gCurrentCostumeRegionIndex;
	*gBagHeadScaleFactor = a1;

	u8 *pRegion = (u8*)CItemRelatedList + regionIndex * 0x44;
	u8 *pSub = *(u8**)(pRegion + 0x1C);
	i16 count = *(i16*)(pSub + 2);
	i16 *pDest = (i16*)(pSub + 0x1C);

	if (count > 0)
	{
		u8 *pSrc = (u8*)gSpideyHeadModel + 2;
		i16 *pTable1 = gBagHeadOffsetTable1;
		i16 *pTable2 = gBagHeadOffsetTable2;

		for (i32 i = count; i != 0; i--)
		{
			if (!(*(pSrc + 4) & 0x12))
			{
				if (a2 == 1)
				{
					pDest[0] = pTable1[0];
					pDest[1] = pTable1[1];
					pDest[2] = pTable1[2];
				}
				else if (a2 == 2)
				{
					pDest[0] = pTable2[0];
					pDest[1] = pTable2[1];
					pDest[2] = pTable2[2];
				}
				else
				{
					pDest[0] = (i32)(*(i16*)(pSrc - 2)) * a1 >> 12;
					pDest[1] = (i32)(*(i16*)(pSrc)) * a1 >> 12;
					pDest[2] = (i32)(*(i16*)(pSrc + 2)) * a1 >> 12;
				}
			}

			pDest = (i16*)((u8*)pDest + 8);
			pSrc += 8;
			pTable1 = (i16*)((u8*)pTable1 + 6);
			pTable2 = (i16*)((u8*)pTable2 + 6);
		}
	}
}

// @Ok
// @Matching
INLINE void Spidey_DoArmorVRAMProcessing(bool a1)
{
	if (gLowGraphics)
	{
		if ((a1 && gSpideyVramProcessing) || (!a1 && !gSpideyVramProcessing))
		{
		   return;
		}

		if (a1)
		{
			Spidey_SwapSuitTextures(CurrentSuit, 0);
		}
		else
		{
			Spidey_SwapSuitTextures(0, CurrentSuit);
		}

		
		gSpideyVramProcessing = !gSpideyVramProcessing;
	}
}

// @Ok
// @Matching
void Spidey_LoadAlternativeHealthIcon(i32 a1)
{
	gSpideyAnimTwo = 0;
	gSpideyAnim = 0;
	switch ( a1 )
	{
		case 2:
			Spool_PSX("cost99", 0);
			gSpideyAnim = Spool_FindAnim("cost99", 1);
			break;
		case 3:
		case 9:
			Spool_PSX("costblk", 0);
			gSpideyAnim = Spool_FindAnim("costblk", 1);
			break;
		case 4:
			Spool_PSX("costcapt", 0);
			gSpideyAnim = Spool_FindAnim("costcapt", 1);
			break;
		case 6:
			Spool_PSX("costbag", 0);
			gSpideyAnim = Spool_FindAnim("costbag", 1);
			break;
		case 7:
			Spool_PSX("costscar", 0);
			gSpideyAnim = Spool_FindAnim("costscar", 1);
			break;
		case 10:
			Spool_PSX("costpete", 0);
			gSpideyAnim = Spool_FindAnim("costpete", 1);
			break;
		default:
			break;
	}
}

// globals for Spidey_LoadAlternativeTextureSet below:
// gRegionReloadRelated (0x55627C): from idb_globals.txt, last spool region
// index reloaded by this function, cleared with ClearRegion before a new
// region loads.
// gAltTexSetNames (0x5512C0): array of string pointers, no idb_globals.txt
// entry nearby, tentative name, guessed from usage (indexed by a2, passed
// to Spool_PSX to load a region for the low graphics path).
// gAltTexSetFileSuffix (0x556694/0x556698): raw 5 bytes (4+1) appended to
// the copied suit name to build a file path checked with FileIO_FileExists.
// written as raw memory (not strcat) because a real strcat call needs an
// extra register (ebx) to keep the buffer alive across the call, which the
// original does not use here; the original builds the suffix inline
// (strlen via scasb, then two raw stores), reproduced the same way below.
// content is a guess since it does not affect code matching (only the data
// address relocates).
static i32 * const gRegionReloadRelated = (i32*)0x0055627C;
#define gAltTexSetNames ((char**)0x005512C0)
static i32 * const gAltTexSetFileSuffixLo = (i32*)0x00556694;
static u8 * const gAltTexSetFileSuffixHi = (u8*)0x00556698;

extern char SuitNames[11][32];

// @NotOk
// known blocker: calls print_if_false, which our compiler always inlines
// (it is static in export.h) while the original calls it out of line (see
// CLAUDE.md "print_if_false inlining" note, also hit by the neighbouring
// Spidey_BagHead/Spidey_SwapSuitTextures in this file). that alone rules
// out a full match on the hardware-renderer branch below.
// residue on the low graphics branch (print_if_false not reached there):
// 45 mnemonic diffs, all one cluster from the Spool_PSX(gAltTexSetNames[a2])
// call onward. cmpsum against a fresh build with gAltTexSetNames written as
// a #define (not a `char** const` global) matches the original's single
// `mov ecx,[esi*4+5512Ch]` fold, but the following call-argument push for
// Spidey_SwapSuitTextures still schedules one instruction earlier than the
// original relative to the two field stores (gRegionReloadRelated,
// PSXRegion[region].Protected); reordering the three statements in source
// made it worse (120 diffs), not better, so left as scheduling residue.
// attempts logged in ~/Documents/spidey-work/wt/spidey.attempts.md.
void Spidey_LoadAlternativeTextureSet(u32 const *, i32 a2)
{
	if (gLowGraphics)
	{
		if (CurrentSuit == a2)
			return;

		if (a2 == 6)
		{
			if (!*gBagHeadModeOne)
				Spidey_BagHead(*gBagHeadScaleFactor, 1);

			goto checkModeTwo;
		}
		else
		{
			if (*gBagHeadModeOne == 1)
				Spidey_BagHead(*gBagHeadScaleFactor, 0);

			if (a2 == 10)
			{
				if (!*gBagHeadModeTwo)
					Spidey_BagHead(*gBagHeadScaleFactor, 2);

				goto afterModeTwo;
			}
		}

checkModeTwo:
		if (*gBagHeadModeTwo == 1)
			Spidey_BagHead(*gBagHeadScaleFactor, 0);

afterModeTwo:
		if (*gRegionReloadRelated >= 0)
		{
			ClearRegion(*gRegionReloadRelated, 1);
		}

		i32 oldSuit = CurrentSuit;
		CurrentSuit = a2;

		if (a2 == 1)
		{
			*gRegionReloadRelated = -1;
			Spidey_SwapSuitTextures(oldSuit, a2);
		}
		else
		{
			i32 region = Spool_PSX(gAltTexSetNames[a2], 0);
			*gRegionReloadRelated = region;
			PSXRegion[region].Protected = 1;
			Spidey_SwapSuitTextures(oldSuit, a2);
		}
	}
	else
	{
		print_if_false(a2 >= 1 && a2 <= 10, "Spidey_LoadAlternativeTextureSet(): suit out of range\r\n");

		char path[0x20];
		Utils_CopyString(SuitNames[a2], path, sizeof(path));

		i32 len = strlen(path);
		*(i32*)(path + len) = *gAltTexSetFileSuffixLo;
		path[len + 4] = *gAltTexSetFileSuffixHi;

		if (!FileIO_FileExists(path))
		{
			a2 = 1;
		}

		if (CurrentSuit != a2)
		{
			ClearRegion(*gCurrentCostumeRegionIndex, 1);
			CurrentSuit = a2;

			i32 region = Spool_PSX(SuitNames[a2], 0);
			*gCurrentCostumeRegionIndex = (u8)region;
			PSXRegion[region].Protected = 1;
		}
	}
}

// globals for Spidey_StoreTextureEntry below (no idb_globals.txt entry,
// tentative names from usage):
// gGlobalTextureEntryCount (0x6A9050): running count into gGlobalTextureEntries.
// gGlobalTextureEntries (0x6A8000): array of {Texture* pTexture; i16 mA2; i16 mA3;
// u32 mChecksum;} (stride 0xC), terminated by a pTexture==0 sentinel entry.
// gSuitChecksumTable (0x53C1A4): i32[16] per suit (stride 0x40), checksum lookup.
// gCostumeTextureIds (0x6A8D74): i32 (zero-extended u16 value) per
// (suit*16+slot) slot, same table Spidey_SwapSuitTextures indexes.
static i32 * const gGlobalTextureEntryCount = (i32*)0x006A9050;

struct SGlobalTextureEntry
{
	const Texture *pTexture;
	i16 mA2;
	i16 mA3;
	u32 mChecksum;
};
static SGlobalTextureEntry * const gGlobalTextureEntries = (SGlobalTextureEntry*)0x006A8000;

static i32 * const gSuitChecksumTable = (i32*)0x0053C1A4;
static i32 * const gCostumeTextureIds = (i32*)0x006A8D74;

// @NotOk
// known blocker: the fail path calls print_if_false, always inlined by our
// build (static in export.h), while the original calls it out of line
// (retail body is a single `ret`, confirmed against tools/functions -
// it is a no-op in the shipped game). Exact argument order at that one
// call site is ambiguous from the disassembly (only 2 stack slots pushed
// for a message with one %X format spec), so this passes the checksum as
// a printf-style value, which is functionally sensible either way since
// the call does nothing in retail.
// preserved bug: the gLowGraphics==0 search loop compares
// gGlobalTextureEntries[count] (the NEXT free slot, loop-invariant) against
// the checksum on every iteration instead of gGlobalTextureEntries[i] -
// the compiled code hoists the loop-invariant load/compare exactly like
// this, so the "search" only ever matches on i==0 or never matches. this
// looks like a genuine off-by-index bug in the original; reproduced
// verbatim per repo convention (tips.txt: preserve source-level bugs).
// residue: 38 mnemonic diffs (down from 70 on the first honest pass, after
// three fixes: keeping the search as a real for-loop with the invariant
// index instead of collapsing it to one check, deferring the checksum
// read into the count>0 branch instead of hoisting it unconditionally,
// and declaring gCostumeTextureIds as i32 (matching the original's 4-byte
// zero-extended store/compare, `mov [x*4+6A8D74h],eax` after `xor eax,eax`)
// instead of i16 (all confirmed against the disassembly). remaining diffs
// are mostly register pressure (original keeps 3 callee-saved regs live
// across the loop: ebx=cached checksum, esi=loop counter, edi=search
// pointer; ours only needs 2, folding the checksum into a different
// register) and the suit*16+slot indexing using scaled-index addressing
// instead of the original's flat ecx-offset form. tried a do-while instead
// of for (to drop a redundant count>0 recheck at loop entry): made it
// worse (52 diffs), reverted.
void Spidey_StoreTextureEntry(Texture const *pTexture, i16 a2, i16 a3)
{
	if (!gLowGraphics)
	{
		i32 count = *gGlobalTextureEntryCount;

		if (count > 0)
		{
			u32 checksum1 = pTexture->Checksum;

			for (i32 j = 0; j < count; j++)
			{
				if (gGlobalTextureEntries[count].mChecksum == checksum1)
				{
					gGlobalTextureEntries[count].pTexture = pTexture;
					return;
				}
			}
		}

		u32 checksum = pTexture->Checksum;
		gGlobalTextureEntries[count].mChecksum = checksum;
		gGlobalTextureEntries[count].pTexture = pTexture;
		*gGlobalTextureEntryCount = count + 1;
		gGlobalTextureEntries[count].mA2 = a2;
		gGlobalTextureEntries[count].mA3 = a3;
		gGlobalTextureEntries[count + 1].pTexture = 0;

		return;
	}

	u32 checksum = pTexture->Checksum;
	i32 *pEntry = gSuitChecksumTable + CurrentSuit * 16;
	i32 i;

	for (i = 0; i < 16; i++)
	{
		if ((u32)pEntry[i] == checksum)
		{
			gCostumeTextureIds[CurrentSuit * 16 + i] = pTexture->clut;
			return;
		}
	}

	for (i = 0; i < 16; i++)
	{
		if ((u32)gSuitChecksumTable[i] == checksum)
		{
			gCostumeTextureIds[i] = pTexture->clut;
			return;
		}
	}

	print_if_false(0, "Spidey_StoreTextureEntry(): Checksum not found: %8.8X\r\n", checksum);
}

// globals for Spidey_SwapSuitTextures below (no idb_globals.txt entry):
// gCostumeMeshPtrs (0x5F6764), pointer array indexed directly by region id
// (not scaled), each entry points at a per-region mesh-piece list, walked
// in lockstep with CItemRelatedList[region] (ob.h) for word_6B2478[region]
// (export.h, already used the same way in m3dutils.cpp, stride 34 u16 =
// 0x44 per region) iterations. field offsets are guesses from the
// disassembly only (no struct declared): the CItemRelatedList entry's
// first field (offset 0) is a pointer to a sub-struct with a count at +6;
// the gCostumeMeshPtrs entry (offset +4 from the stored pointer) is a
// pointer to a list of entries (texture id at +2), stride 0x38 per entry,
// outer stride 0x24 for the mesh-piece list and 4 for the CItemRelatedList
// sub-array. reuses gCostumeTextureIds (spidey.cpp,
// Spidey_StoreTextureEntry) for the actual texture id remap table. also
// declared gCostumeRegionEntries at the same address as CItemRelatedList
// (0x6B2454) so this file can index it with a plain array subscript
// (region*17) instead of casting CItemRelatedList's established i32***
// type to a byte pointer.
static void ** const gCostumeMeshPtrs = (void**)0x005F6764;
static void ** const gCostumeRegionEntries = (void**)0x006B2454;

// @NotOk
// known blocker: calls print_if_false, always inlined by our build (static
// in export.h), while the original calls it out of line (retail body is a
// single `ret`, confirmed via tools/functions - a no-op in the shipped
// game). string confirmed: "SwapSuitTextures() called in hardware mode!"
// (0x556644, printed when gLowGraphics==0, i.e. hardware mode), region
// name "spidey" (0x556670) passed to Spool_FindRegion.
// residue: 50 mnemonic diffs (down from 57), after widening outerCount to
// i32 (original tests the full edx register after the 16-bit load, because
// an earlier xor edx,edx in the same block left the upper half zero; ours
// needs the wider type to reproduce that). one residue not tracked down:
// our build loads gCostumeMeshPtrs/gCostumeRegionEntries as if they were
// real relocatable globals (`mov esi,[reloc]; mov ecx,[esi+eax]`) instead
// of folding the fixed address into the addressing mode
// (`mov ecx,[eax*4+5F6764h]` in the original) even though the same
// `static X* const = (X*)0xADDR` idiom folds fine elsewhere in the repo
// (word_6B2478, export.h) - tried a simpler single-pointer type instead of
// a double pointer, no change, left as residue given the print_if_false
// blocker already rules out a full match on this function regardless.

void Spidey_SwapSuitTextures(i32 a1, i32 a2)
{
	print_if_false(gLowGraphics != 0, "SwapSuitTextures() called in hardware mode!");

	i32 region = Spool_FindRegion("spidey");
	i32 byteOffset = region * 68;

	i32 outerCount = *(i16*)((u8*)word_6B2478 + byteOffset);

	if (outerCount > 0)
	{
		u8 *pRegionEntry = (u8*)gCostumeRegionEntries[region * 17];
		u8 *pMeshList = (u8*)gCostumeMeshPtrs[region] + 4;

		for (i32 i = outerCount; i != 0; i--)
		{
			u8 *pSub = *(u8**)pRegionEntry;
			i16 innerCount = *(i16*)(pSub + 6);
			u8 *pEntry = *(u8**)pMeshList;

			if (innerCount > 0)
			{
				u16 *pTexId = (u16*)(pEntry + 2);

				for (i32 j = innerCount; j != 0; j--)
				{
					i32 texId = *pTexId;
					i32 *pSearch = gCostumeTextureIds + a1 * 16;
					i32 k;

					for (k = 0; k < 16; k++)
					{
						if (texId == pSearch[k])
						{
							i32 newId = gCostumeTextureIds[a2 * 16 + k];

							if (newId != 0)
								*pTexId = (u16)newId;

							break;
						}
					}

					pTexId = (u16*)((u8*)pTexId + 0x38);
				}
			}

			pRegionEntry += 4;
			pMeshList += 0x24;
		}
	}
}

// @SMALLTODO
void spideyLog(char *,...)
{
    printf("spideyLog(char *,...)");
}

// @NotOk
// gCostumeRegionEntries[Region * 17] holds a per-region pointer table (same
// table Spidey_SwapSuitTextures/Spidey_BagHead already use); index 7 is the
// head model entry. entry+2 (u16) is the vertex/part count, entry+0x1C is
// the raw data the copy pulls from. DCMem_New args (size, 1, 1, 0, 1)
// confirmed against the push order in the disassembly.
// residue: 22 mnemonic diffs, all downstream of one thing: the original
// compiles the count*8-byte copy into a bare `rep movsd` with no remainder
// handling (dword count computed via a reused decremented register, then
// masked with 0x3FFFFFFE, which is a no-op for realistic sizes). every
// tried source shape that reaches `rep movsd` at all (memcpy with a runtime
// byte count, `size*8`, `size<<3`, or a byte count precomputed into its own
// variable) always adds the standard MSVC6 remainder tail (`shr ecx,2; rep
// movsd; and ecx,3; rep movsb`), confirmed identical to the already-matched
// Bitmap256::Bitmap256 (0x413670) memcpy call. every tried plain pointer
// loop (dword while-loop, dword for-loop with array indexing, do-while
// copying 2 dwords per iteration, the original sketch's 4-u16-field-store
// loop) compiles to a real load/store loop instead of `rep movsd` at all,
// since this compiler only lowers to `rep movsd` for an actual memcpy call.
// attempts (9): memcpy(dst,src,8*size) -> rep movsd + tail, 31 diffs;
// u16 4-field manual loop (matches an earlier sketch) -> real loop, 31
// diffs; struct-of-2-u32 array assignment loop -> real per-field loop, no
// movsd; dword pointer while(n--) *d++=*s++ -> real loop; dword pointer
// for(i<n) dst[i]=src[i] -> real loop, best result, 22 diffs (kept); if
// (size) { do {2 stores} while(--size); } -> real loop, 27 diffs; memcpy
// with count precomputed then count*4 -> same as plain memcpy, 31 diffs;
// memcpy(dst,src,size<<3) -> same tail, 29 diffs. kept the dword
// for-loop version (22 diffs) as the closest honest translation; the
// remaining diffs are the prologue push order (original pushes
// ebx/esi/edi unconditionally up front, ours defers push until the
// registers are actually needed) which is fallout of never reaching
// `rep movsd`, not a separate issue.
void Spidey_CopyHeadModel(i32 Region)
{
	if (!gSpideyHeadModel)
	{
		void **pEntry = reinterpret_cast<void**>(gCostumeRegionEntries[Region * 17]);
		u16 *ptr = reinterpret_cast<u16*>(pEntry[7]);
		u16 size = ptr[1];

		u16 *result = static_cast<u16*>(DCMem_New(8 * size, 1, 1, 0, 1));
		gSpideyHeadModel = static_cast<void*>(result);

		u32 *dst = reinterpret_cast<u32*>(result);
		u32 *src = reinterpret_cast<u32*>(reinterpret_cast<u8*>(ptr) + 0x1C);
		i32 n = size * 2;

		for (i32 i = 0; i < n; i++)
			dst[i] = src[i];
	}
}

// @Ok
void Spidey_FreeHeadModel(void)
{
	Mem_Delete(static_cast<void*>(gSpideyHeadModel));
	gSpideyHeadModel = 0;
}

// @Ok
u8 CPlayer::IncreaseWebbing(i32 amount)
{
	if (this->mHealth <= 0)
		return 0;

	i32 v3 = 10;
	if (CurrentSuit == 6 || CurrentSuit == 9 || CurrentSuit == 10)
		v3 = 2;

	if ( (this->mWebbing >= 4096 || this->field_5E8) && this->field_5D8 >= v3)
		return 0;

	this->mWebbing += amount;

	if (this->mWebbing > 4096)
	{
		if (this->field_5D8 < v3)
		{
			this->field_5D8++;
			this->mWebbing -= 4096;
			this->field_5DC = gTimerRelated;
			this->field_5D0++;
			return 1;
		}

		this->mWebbing = 4096;
	}

	this->field_5D0++;
	return 1;
}

// @NotOk
// residue: original computes &a1 and pushes both call args first, then
// stores a1.vx/vy/vz through the post-push stack offsets. our build always
// hoists the two zero stores (vx, vz) before the address-of/push, keeping
// only the vy store (which depends on the pVector read) after the push.
// tried: statement order (vy first/last), a2/a1 declaration order swap,
// default-ctor-then-assign-vy (ctor's own zero stores get hoisted earlier
// still), named pointer locals for the call args (optimized away, no
// change), and a real 3-arg SVECTOR constructor (same early-hoist as the
// default ctor). best result so far: 23 mnemonic diffs, all in this one
// instruction-scheduling cluster; every later instruction matches once
// this settles (call targets/relocations aside).
void CPlayer::SetStartOrientation(CSVector* pVector)
{
	MATRIX a2;
	SVECTOR a1;

	a1.vy = pVector->vy;
	a1.vx = 0;
	a1.vz = 0;

	M3dMaths_RotMatrixYXZ(&a1, &a2);
	MulMatrix(&this->mTransform, &a2);
	this->OrientToNormal(0, &ZeroVector);
}

// @NotOk
// validate later
INLINE void CPlayer::CreateFists(u8 a2)
{
	if (a2 & 1)
	{
		this->field_194 &= 0xFFFFFFDF;
		this->field_194 |= 0x40;
	}
	else
	{
		this->field_194 &= 0xFFFFFFBF;
		this->field_194 |= 0x20;
	}

	if (a2 & 2)
	{
		this->field_194 &= 0xFFFFFBFF;
		this->field_194 |= 0x800;
	}
	else
	{
		this->field_194 &= 0xFFFFF7FF;
		this->field_194 |= 0x400;
	}
}

// @Ok
void CPlayer::SetIgnoreInputTimer(int a2)
{
	this->field_E18 = a2;
	if (a2)
	{
		this->field_E12 = this->mAnimSpeed;
		if (this->field_8EA)
		{
			this->ExitLookaroundMode();
		}
	}
}

// @Ok
void CPlayer::SetCamAngleLock(u16 a1)
{
	if (a1)
	{
		this->gCamAngleLock = 0;
	}
	else
	{
		this->gCamAngleLock = 1;
	}
}

static CCamera * const gGlobalThisCamera = (CCamera*)0x69696969;
static int * const dword_660F80 = (int*)0x660F80;
static int * const dword_60F76C = (int*)0x60F76C;

// @NotOk
// globals need to replace
void CPlayer::ExitLookaroundMode(void)
{
	if (this->field_8EA)
	{
		int c90 = this->field_C90;
		this->field_CB4 = 0;
		this->field_CE4 = 0;
		this->field_56C = 0;
		this->field_8EA = 0;

		*dword_660F80 = 0;
		*dword_60F76C = 0;


		if (c90)
		{
			Mem_Delete(reinterpret_cast<void*>(c90));
			this->field_C90 = 0;
		}

		gGlobalThisCamera->PopMode();
		this->PutCameraBehind(0);
		this->field_DE4 = 0;
		Screen_TargetOn(false);

		i16 *v3 = reinterpret_cast<i16*>(this->mpJoints);
		if (v3)
		{
			v3[6] = 0;
			v3[7] = 0;
			v3[18] = 0;
			v3[19] = 0;
		}
	}
}

static int * const dword_6A81FC = (int*)0x6A81FC;
static int * const dword_6A8208 = (int*)0x6A8208;
static int * const dword_6A8260 = (int*)0x6A8260;

// @NotOk
// Remove globals, there's an extra test for some reason
void CPlayer::SetSpideyLookaroundCamValue(u16 a1, u16 a2, i16 a3)
{
	u32 actualA1 = a1;
	if (actualA1)
	{
		actualA1--;
		if (actualA1)
		{
			actualA1--;
			if (!actualA1)
			{
				if (a2)
				{
					print_if_false(0, "Bad spidey cam param type");
				}
				else
				{
					*dword_6A8208 = a3;
				}
			}
		}
		else
		{
			if (a2)
			{
				print_if_false(0, "Bad spidey cam param type");
			}
			else
			{
				*dword_6A81FC = a3;
			}
		}

	}
	else if (a2)
	{
		print_if_false(0, "Bad spidey cam param type");
	}
	else
	{
		*dword_6A8260 = a3;
	}
}

// @Ok
// slightly different register allocation
void CPlayer::SetTargetTorsoAngleToThisPoint(CVector *a2)
{
	gte_SetRotMatrix(&this->field_89C);


	CVector v8;
	v8.vx = (a2->vx - this->mPos.vx) >> 12;
	v8.vy = (a2->vy - this->mPos.vy) >> 12;
	v8.vz = (a2->vz - this->mPos.vz) >> 12;

	gte_ldlvl(reinterpret_cast<VECTOR*>(&v8));
	gte_rtir();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&v8));

	i16 v6 = (1024 - ratan2(-v8.vz, -v8.vx)) & 0xFFF;

	i16 v7;
	if (this->field_8E9)
	{
		v7 = this->GetEffectiveHeading() - v6;
	}
	else
	{
		v7 = this->GetEffectiveHeading() + v6;
	}

	this->SetTargetTorsoAngle(v7 & 0xFFF, 0);
}

// @Ok
i16 CPlayer::GetEffectiveHeading(void)
{ 
	if (!this->field_8E8)
	{
		return (1024 - ratan2(this->field_C6C.vz, this->field_C6C.vx)) & 0xFFF;
	}

	CVector fourth;
	fourth.vx = 0;
	fourth.vy = -4096;
	fourth.vz = 0;

	CVector second;
	second.vx = 0;
	second.vy = 0;
	second.vz = 0;

	gte_ldopv1(reinterpret_cast<VECTOR*>(&fourth));
	gte_ldopv2(reinterpret_cast<VECTOR*>(&this->field_C84));
	gte_op12();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&second));
	VectorNormal(reinterpret_cast<VECTOR*>(&second), reinterpret_cast<VECTOR*>(&second));

	CVector first;
	first.vx = 0;
	first.vy = 0;
	first.vz = 0;

	gte_ldopv1(reinterpret_cast<VECTOR*>(&second));
	gte_ldopv2(reinterpret_cast<VECTOR*>(&this->field_C84));
	gte_op12();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&first));

	first.vx = -first.vx;
	first.vy = -first.vy;
	first.vz = -first.vz;

	gte_SetRotMatrix(&this->field_89C);
	gte_ldlvl(reinterpret_cast<VECTOR*>(&first));
	gte_rtir();

	CVector third;
	third.vx = 0;
	third.vy = 0;
	third.vz = 0;

	gte_stlvnl(reinterpret_cast<VECTOR*>(&third));

	return (ratan2(third.vz, third.vx) + 1024) & 0xFFF;
}

// @MEDIUMTODO
void CPlayer::SetTargetTorsoAngle(i16, int)
{
	printf("void CPlayer::SetTargetTorsoAngle(i16, int)");
}


static i32 * const dword_60CFE8 = (i32*)0x60CFE8;
static i32 * const dword_54D474 = (i32*)0x54D474;
static char * const byte_682770 = (char*)0x682770;
extern int CurrentSuit;

// @NotOk
// Globals
// The part with >> 12 has a jump in the original rather than it's perfect
char CPlayer::DecreaseWebbing(i32 a2)
{
	if (!this->field_1AC &&
			!*dword_60CFE8 &&
			CurrentSuit != 3 &&
			CurrentSuit != 4)
	{
		int v3;
		int v4;

		int tmpDword = *dword_54D474;
		if (!tmpDword)
		{
			v3 = a2 << 7;
			v4 = v3 >> 12;
		}
		else if (tmpDword == 1)
		{
			v3 = a2 << 11;
			v4 = v3 >> 12;
		}

		int v5 = this->mWebbing;
		if (v5 > v4)
		{
			this->mWebbing = v5 - v4;
			return 1;
		}

		int v7 = this->field_5D8;
		if (v7)
		{
			this->mWebbing = v5 - v4 + 4096;
			this->field_5D8 = v7 - 1;
			SFX_PlayPos(0x1E, &this->mPos, 0);
			this->field_5E8 = 0;
			return 1;
		}

		if (!this->field_E10)
		{
			if (!*byte_682770)
			{
				Redbook_XAPlay(33, Rnd(3) + 2, 0);
			}

			this->field_5E8 = 0;
			return 0;
		}

		return 1;
	}

	return 1;
}


static CVector * const stru_56F1B4 = (CVector*)0x56F1B4;
static MATRIX * const stru_56F224 = (MATRIX*)0x56F224;

// @NotOk
// Globals
// Can be optimized (remove tmp)
// gte_ldlv0 is dangerous it reads more memory than needed
void CPlayer::RenderLookaroundReticle(void)
{
	if (this->field_DE4)
	{

		CVector tmp = *stru_56F1B4;
		CVector vec  = (this->field_DC0 >> 12) - tmp;
		
		gte_SetRotMatrix(stru_56F224);
		m3d_ZeroTransVector();
		gte_ldlv0(reinterpret_cast<VECTOR*>(&vec));
		gte_rtps();

		int v5;
		gte_stlvnl2(&v5);

		i16 v6[2];
		gte_stsxy(reinterpret_cast<i32*>(v6));

		i32 v3 = 3072 - v5;
		if (v3 < 768)
		{
			v3 = 768;
		}

		this->DrawRecticle(v6[0], v6[1], v3);
	}
}

// @BIGTODO
void CPlayer::DrawRecticle(u16, u16, u32)
{
	printf("void CPlayer::DrawRecticle(unsigned __int16, unsigned __int16, unsigned int)");
}


// @Ok
// instead of sub 0x1000 we do add 0xFFFFF000, dunno why
// also abs is different but wtv
void CPlayer::SetTargetTorsoAngle(i16 a2, bool a3)
{
	int v4 = (a2 & 0xFFF);
	i16 EffectiveHeading = this->GetEffectiveHeading();

	if ( (i16)v4 == EffectiveHeading)
	{
		this->field_DF8 = 0;
		return;
	}

	i32 v6 = this->field_E1C;
	if (v6 & 6)
		this->field_DF8 = 5 * this->field_DFC;
	else
		this->field_DF8 = 10;


	if (v6 & 0x2000000)
		this->field_DF8 <<= 1;

	this->field_DF0 = v4;

	i32 v7;
	if (v4 > EffectiveHeading)
	{
		v7 = v4 - EffectiveHeading;
		if ( v7 >= 2048 )
			v7 = (i16)(v4 - 0x1000) - EffectiveHeading;
	}
	else
	{
		i32 v8 = EffectiveHeading;
		if ( EffectiveHeading - v4 >= 2048 )
			v8 = (i16)(EffectiveHeading - 0x1000);
		v7 = v4 - v8;
	}

	int v9 = this->field_DF8;
	int v10 = v7 / v9;
	bool v11 = this->field_AD4 == 0;
	this->field_DF4 = v7 / v9;
	int v12 = 384;
	if ( v11 )
		v12 = 512;
	int v13 = v12 / v9;
	if ( a3 )
		v13 <<= 1;
	if ( v10 > v13 )
	{
		this->field_DF4 = v13;
		this->field_DF8 = abs(v7 / v13);
	}
	if ( this->field_DF4 < -v13 )
	{
		this->field_DF4 = -v13;
		this->field_DF8 = abs(v7 / v13);
	}
}

static i16 * const word_6A8C66 = (i16*)0x6A8C66;
static i16 * const word_610C4A = (i16*)0x610C4A;
static i16 * const word_610C48 = (i16*)0x610C48;

// @NotOk
// globals
void CPlayer::PutCameraBehind(i32 a2)
{
	if (!this->gCamAngleLock)
	{
		if (!this->field_8E8)
		{
			gGlobalThisCamera->SetCamAngle(this->GetEffectiveHeading(), a2);
		}
		else
		{
			int v5 = (1024 - ratan2(this->field_C84.vz, this->field_C84.vx)) & 0xFFF;
			gGlobalThisCamera->SetCamAngle(v5, a2);

			if (gGlobalThisCamera->mCameraMode == CAMERAMODE_DEMO)
			{
				if ((this->field_E2E | this->field_E2D) && this->field_E1C == 16)
				{
					i32 v6 = 2 * (this->field_E2D & 0xFFF);
					gGlobalThisCamera->SetCamYDistance(*word_6A8C66 + ((500 * word_6A8C66[v6]) >> 12), a2);
					gGlobalThisCamera->SetCamAngle(v5 + ((700 * word_610C48[v6]) >> 12), a2);
				}
				else
				{
					gGlobalThisCamera->SetCamYDistance(*word_6A8C66, a2);
				}
			}
		}


	}
}


// @NotOk
// not matching become smoke trai lhas no cosntructor so it's inlined af
void CPlayer::CreateJumpingSmashKickTrail(void)
{
	CVector vec;
	vec.vx = 0;
	vec.vy = 0;
	vec.vz = 0;

	if (!this->field_584)
	{
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&vec),
				this,
				5);

		int args = this->field_580;
		CSmokeTrail *smokeTrail = new CSmokeTrail(
				&vec,
				4,
				args,
				*(reinterpret_cast<unsigned char*>(&args) + 2),
				*(reinterpret_cast<unsigned char*>(&args) + 1));

		this->field_584 = smokeTrail;
	}

	if (!this->field_588)
	{
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&vec),
				this,
				6);

		int args = this->field_580;
		CSmokeTrail *smokeTrail = new CSmokeTrail(
				&vec,
				4,
				static_cast<unsigned char>(args),
				*(reinterpret_cast<unsigned char*>(&args) + 2),
				*(reinterpret_cast<unsigned char*>(&args) + 1));

		this->field_588 = smokeTrail;
	}
}

// @Ok
// @Matching
INLINE void CPlayer::ResetSFXArrayEntry(u32 a2)
{
	i32 *v2 = gSpideySFXEntry[a2];
	if (v2)
	{
		while (*v2 != -1)
		{
			*v2 = *v2 & 0xFFFF;
			v2++;
		}
	}
}

// @Ok
// @Matching
INLINE void CPlayer::PlaySingleAnim(i32 a2, i32 a3, i32 a4)
{
	i32 *tmp = gSpideySFXEntry[a2];
	this->field_350 = tmp;

	if (tmp)
	{
		this->ResetSFXArrayEntry(a2);
	}

	CSuper::RunAnim(a2, a3, a4);
}

// @BIGTODO
void CPlayer::OrientToNormal(bool, CVector*)
{
	printf("CPlayer::OrientToNormal");
}

// @BIGTODO
void CPlayer::PriorToVenomDistanceAttack(CVector)
{}

// @BIGTODO
void CPlayer::SwitchToStandMode(void)
{}

// @NotOk
// Globals
// raw memory accesses
void CPlayer::CutSceneSkipCleanup(void)
{
	Redbook_XAStop();

	if (gGlobalThisCamera->mCameraMode != CAMERAMODE_DEMO && Trig_GetLevelID() != 514)
	{
		gGlobalThisCamera->SetMode(static_cast<ECameraMode>(3));
	}

	int v3 = this->field_1A8;
	CVector v14;
	v14.vx = 0;
	v14.vy = 0;
	v14.vz = 0;


	if (v3)
	{
		int* ptr = reinterpret_cast<int*>(Trig_GetLinksPointer(v3));
		if (ptr[0])
		{
			Trig_GetPosition(&v14, ptr[1]);

			v14.vy = 0;
			v14.vx = (this->mPos.vx - v14.vx) >> 12;
			v14.vz = (this->mPos.vz - v14.vz) >> 12;
			VectorNormal(
					reinterpret_cast<VECTOR*>(&v14),
					reinterpret_cast<VECTOR*>(&v14));

			this->field_A8.vx = 0;
			this->field_A8.vy = -4096;
			this->field_A8.vz = 0;
		}
		else
		{
			v14 = this->field_C6C;
		}

		this->PriorToVenomDistanceAttack(v14);
	}

	this->PlaySingleAnim(0, 0, -1);
	this->SwitchToStandMode();
	gGlobalThisCamera->SetStartPosition();

	char * v13 = reinterpret_cast<char*>(this->field_E0C);
	*(v13  + 256) = 1;
	*(v13  + 48) = 1;

}

// @NotOk
// globals
// variables
void CPlayer::TidyUpZipWebLandingPosition(int a2)
{
	SLineInfo v21;

	int v2 = 0;

	v21.MinCoords.vx = 0;
	v21.MinCoords.vy = 0;
	v21.MinCoords.vz = 0;

	v21.MaxCoords.vx = 0;
	v21.MaxCoords.vy = 0;
	v21.MaxCoords.vz = 0;

	v21.Position.vx = 0;
	v21.Position.vy = 0;
	v21.Position.vz = 0;

	v21.Normal.vx = 0;
	v21.Normal.vy = 0;
	v21.Normal.vz = 0;

	int i = 0;
	do
	{
		int y = this->mPos.vy;
		int v6 = 2 * (i & 0xFFF);

		int v7 = word_610C4A[v6];
		int v8 = word_610C48[v6];

		int v9 = v2 * (((this->field_C78 * v7) >> 12) + ((this->field_C6C.vx * v8) >> 12));
		int v10 = this->field_C7C * v7;
		int v11 = this->field_C80 * v7;

		v21.StartCoords.vx = v9 + this->mPos.vx;
		int v12 = (v10 >> 12) + ((this->field_C6C.vy * v8) >> 12);
		v2 = a2;
		int v13 = a2 * v12;
		int v14 = (v11 >> 12) + ((this->field_C6C.vz * v8) >> 12);
		int z = this->mPos.vz;

		int v16 = a2 * v14;
		v21.StartCoords.vy = v13 + y;
		v21.StartCoords.vz = v16 + z;
		v21.EndCoords.vx = this->mPos.vx - v9;

		int v17 = this->mPos.vy;
		v21.EndCoords.vy = v17 - v13;
		v21.EndCoords.vz = z - v16;
		M3dColij_InitLineInfo(&v21);
		M3dZone_LineToItem(&v21, 1);
		if (v21.pItem)
		{
			int v18 = a2 * v21.Normal.vz;
			int v19 = v17 + a2 * v21.Normal.vy;
			this->mPos.vx += a2 * v21.Normal.vx;
			this->mPos.vy = v19;
			this->mPos.vz = z + v18;
		}

		i += 512;
	}while(i<4096);
}

static const char* gUserFunctionName;
static unsigned int gUserFunctionSize;

// @NotOk
// global
void Spidey_SetUserFunction(const char *a1, unsigned int a2)
{
	gUserFunctionName = a1;
	gUserFunctionSize = a2;
}

// @Ok
unsigned char CPlayer::CanITalkRightNow(void)
{
	if (this->field_E1C & 0x800080)
		return 0;
	return 1;
}

// @Ok
unsigned char CPlayer::SetFireWebbing(void)
{
	this->field_5E8 = 1;
	this->mWebbing = 4096;
	this->field_5D0++;
	return 1;
}

// @Ok
void INLINE CPlayer::GetHookPosition(CVector* a2, unsigned char a3)
{
	M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(a2), this, a3);
}

// @NotOk
// revisit without casts
void CPlayer::DestroyJumpingSmashKickTrail(void)
{
	if (this->field_584)
	{
		int *tmp = reinterpret_cast<int*>(this->field_584);
		tmp[21] = 1;
		this->field_584 = NULL;
	}

	if (this->field_588)
	{
		int *tmp = reinterpret_cast<int*>(this->field_588);
		tmp[21] = 1;
		this->field_588 = NULL;
	}
}

// @NotOk
// revisit without casts
void CPlayer::DestroyHandTrails(void)
{
	if (this->field_58C)
	{
		int *tmp = reinterpret_cast<int*>(this->field_58C);
		tmp[21] = 1;
		this->field_58C = NULL;
	}

	if (this->field_590)
	{
		int *tmp = reinterpret_cast<int*>(this->field_590);
		tmp[21] = 1;
		this->field_590 = NULL;
	}
}

// @Ok
void CPlayer::DeleteStuff(void)
{
	Screen_TargetOn(false);
	if (this->field_C90)
	{
		Mem_Delete(reinterpret_cast<void*>(this->field_C90));
		this->field_C90 = 0;
	}
}

// @Ok
void CPlayer::StopAlertMusic(void)
{
	this->field_52C = 0;
	this->field_528 = 0;
	if (this->field_538)
	{
		SFX_Stop(this->field_538);
		this->field_538 = 0;
	}
}

// @Ok
INLINE i32* CPlayer::KillCommandBlock(i32* a1)
{
	i32* res = reinterpret_cast<i32*>(a1[a1[1]-1]);

	if (this->field_1BC == a1)
	{
		this->field_1BC = res;
	}
	else
	{
		i32* it = this->field_1BC;

		while (it)
		{
			if (a1 == reinterpret_cast<i32*>(it[it[1]-1]))
			{
				it[it[1]-1] = reinterpret_cast<i32>(res);
				break;
			}

			it = reinterpret_cast<i32*>(it[it[1]-1]);
		}
	}

	Mem_Delete(reinterpret_cast<void*>(a1));
	return res;
}

// @NotOk
// Revisit
void CPlayer::KillAllCommandBlocks(void)
{
	for (int* cur = reinterpret_cast<int*>(this->field_1BC); cur; cur = this->KillCommandBlock(cur));
	this->field_1BC = 0;
}

// @Ok
void CPlayer::Die(void)
{
	if (!this->IsDead())
	{
		this->mCBodyFlags |= 0x40;
		this->mFlags |= 1;
	}
}

void validate_CPlayer(void)
{
	VALIDATE_SIZE(CPlayer, 0xEFC);

	VALIDATE(CPlayer, field_194, 0x194);

	VALIDATE(CPlayer, field_1A8, 0x1A8);
	VALIDATE(CPlayer, field_1AC, 0x1AC);

	VALIDATE(CPlayer, field_1BC, 0x1BC);

	VALIDATE(CPlayer, field_350, 0x350);

	VALIDATE(CPlayer, field_354, 0x354);
	VALIDATE(CPlayer, field_358, 0x358);
	VALIDATE(CPlayer, field_35C, 0x35C);

	VALIDATE(CPlayer, field_528, 0x528);
	VALIDATE(CPlayer, field_52C, 0x52C);
	VALIDATE(CPlayer, field_530, 0x530);

	VALIDATE(CPlayer, field_538, 0x538);

	VALIDATE(CPlayer, field_540, 0x540);

	VALIDATE(CPlayer, field_54C, 0x54C);

	VALIDATE(CPlayer, field_568, 0x568);
	VALIDATE(CPlayer, field_56C, 0x56C);

	VALIDATE(CPlayer, field_570, 0x570);

	VALIDATE(CPlayer, field_57C, 0x57C);

	VALIDATE(CPlayer, field_580, 0x580);

	VALIDATE(CPlayer, field_584, 0x584);
	VALIDATE(CPlayer, field_588, 0x588);

	VALIDATE(CPlayer, field_58C, 0x58C);
	VALIDATE(CPlayer, field_590, 0x590);

	VALIDATE(CPlayer, field_5E9, 0x5E9);
	VALIDATE(CPlayer, field_5EC, 0x5EC);

	VALIDATE(CPlayer, field_5F0, 0x5F0);

	VALIDATE(CPlayer, field_5D0, 0x5D0);
	VALIDATE(CPlayer, mWebbing, 0x5D4);
	VALIDATE(CPlayer, field_5D8, 0x5D8);
	VALIDATE(CPlayer, field_5DC, 0x5DC);

	VALIDATE(CPlayer, field_5E0, 0x5E0);

	VALIDATE(CPlayer, field_5E8, 0x5E8);

	VALIDATE(CPlayer, field_89C, 0x89C);

	VALIDATE(CPlayer, field_8C4, 0x8C4);
	VALIDATE(CPlayer, field_8C8, 0x8C8);

	VALIDATE(CPlayer, field_8CC, 0x8CC);

	VALIDATE(CPlayer, field_8D8, 0x8D8);
	VALIDATE(CPlayer, field_8DC, 0x8DC);

	VALIDATE(CPlayer, field_8E8, 0x8E8);
	VALIDATE(CPlayer, field_8E9, 0x8E9);
	VALIDATE(CPlayer, field_8EA, 0x8EA);

	VALIDATE(CPlayer, gCamAngleLock, 0x8EC);

	VALIDATE(CPlayer, field_AB8, 0xAB8);

	VALIDATE(CPlayer, field_AC8, 0xAC8);

	VALIDATE(CPlayer, field_AD4, 0xAD4);

	VALIDATE(CPlayer, field_AD7, 0xAD7);

	VALIDATE(CPlayer, field_AE4, 0xAE4);
	VALIDATE(CPlayer, field_AE5, 0xAE5);
	VALIDATE(CPlayer, field_AE6, 0xAE6);


	VALIDATE(CPlayer, field_B74, 0xB74);
	VALIDATE(CPlayer, field_B84, 0xB84);
	VALIDATE(CPlayer, field_B8C, 0xB8C);

	VALIDATE(CPlayer, field_C18, 0xC18);
	VALIDATE(CPlayer, field_C1C, 0xC1C);
	VALIDATE(CPlayer, field_C28, 0xC28);


	VALIDATE(CPlayer, field_C30, 0xC30);


	VALIDATE(CPlayer, field_C6C, 0xC6C);

	VALIDATE(CPlayer, field_C78, 0xC78);
	VALIDATE(CPlayer, field_C7C, 0xC7C);
	VALIDATE(CPlayer, field_C80, 0xC80);
	VALIDATE(CPlayer, field_C84, 0xC84);

	VALIDATE(CPlayer, field_C90, 0xC90);
	VALIDATE(CPlayer, field_CB4, 0xCB4);
	VALIDATE(CPlayer, field_CE4, 0xCE4);

	VALIDATE(CPlayer, field_D3C, 0xD3C);
	VALIDATE(CPlayer, field_D4E, 0xD4E);

	VALIDATE(CPlayer, field_D80, 0xD80);
	VALIDATE(CPlayer, field_D86, 0xD86);
	VALIDATE(CPlayer, field_D8C, 0xD8C);

	VALIDATE(CPlayer, field_DA0, 0xDA0);

	VALIDATE(CPlayer, field_DB8, 0xDB8);

	VALIDATE(CPlayer, field_DC0, 0xDC0);
	VALIDATE(CPlayer, field_DCC, 0xDCC);

	VALIDATE(CPlayer, field_DE4, 0xDE4);


	VALIDATE(CPlayer, field_DF0, 0xDF0);
	VALIDATE(CPlayer, field_DF4, 0xDF4);
	VALIDATE(CPlayer, field_DF8, 0xDF8);
	VALIDATE(CPlayer, field_DFC, 0xDFC);

	VALIDATE(CPlayer, field_E00, 0xE00);
	VALIDATE(CPlayer, field_E0C, 0xE0C);

	VALIDATE(CPlayer, field_E10, 0xE10);
	VALIDATE(CPlayer, field_E12, 0xE12);
	VALIDATE(CPlayer, field_E18, 0xE18);
	VALIDATE(CPlayer, field_E1C, 0xE1C);

	VALIDATE(CPlayer, field_E2D, 0xE2D);
	VALIDATE(CPlayer, field_E2E, 0xE2E);

	VALIDATE(CPlayer, field_E38, 0xE38);

	VALIDATE(CPlayer, hLockTarget, 0xE70);

	VALIDATE(CPlayer, field_E84, 0xE84);
	VALIDATE(CPlayer, field_E88, 0xE88);
	VALIDATE(CPlayer, field_E8C, 0xE8C);

	VALIDATE(CPlayer, mHeldObject, 0xE48);
	VALIDATE(CPlayer, field_E64, 0xE64);

	VALIDATE(CPlayer, field_EA4, 0xEA4);

	VALIDATE(CPlayer, field_EA8, 0xEA8);

	VALIDATE(CPlayer, mMaxHealth, 0xEF0);
}

void validate_SIndicator(void)
{
	VALIDATE_SIZE(SIndicator, 0x68);

	VALIDATE(SIndicator, field_C, 0xC);

	VALIDATE(SIndicator, mInUse, 0x64);
}
