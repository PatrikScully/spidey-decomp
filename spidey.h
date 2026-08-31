#pragma once


#ifndef SPIDEY_H
#define SPIDEY_H

#include "export.h"
#include "ob.h"
#include "manipob.h"
#include "quat.h"

struct SAnimFrame;

EXPORT extern u8 gSpideyPsxIndex;

struct SIndicator
{
	// direction (already local-space, normalized) from the player towards
	// the offscreen threat this entry represents; written by
	// CPlayer::BuildOffscreenSpideySenseIndicatorList.
	CVector mDirection;

	SHandle field_C;

	// @FIXME - there's a POLY_F3 that should go in here but I have
	// no idea how it works leaving it for future me
	PADDING(0x64-0xC-sizeof(SHandle));

	i32 mInUse;
};

class CPlayer : public CSuper 
{
	public:

		PADDING(4);

		i32 field_1A8;
		char field_1AC;

		PADDING(0x1B0-0x1AC-1);

		// elapsed-time accumulator for SynthesizeAnalogueInput's bytecode
		// VM (this->field_80 added per tick), same role as
		// CSpClone::field_340/CBlackCat's analog.
		i32 field_1B0;

		// phase-1 (bytecode) active flag for SynthesizeAnalogueInput,
		// same role as CSpClone::field_344.
		u8 field_1B4;

		PADDING(3);

		// bytecode stream pointer (cast to i16* at use sites) for
		// SynthesizeAnalogueInput, same role as CSpClone::field_348.
		i32 field_1B8;

		i32* field_1BC;

		PADDING(0x211-0x1BC-4);

		// set by ReadAnalogueInput when the move input returns to centre
		// while field_AD8 was pending.
		u8 field_211;

		PADDING(0x34C-0x211-1);

		// ProcessSFXArray: non-zero selects the (Rnd(4)+80)|0x8000 SFX range,
		// zero selects the Rnd(4)+1 range, in the mAnim 0x15/0xC0/0xC6 case.
		i32 field_34C;

		i32 *field_350;

		u8 field_354;
		PADDING(3);

		i32 field_358;
		u32 field_35C;

		PADDING(0x528-0x35C-4);

		i32 field_528;
		i32 field_52C;
		i32 field_530;

		PADDING(0x538-0x530-4);

		u32 field_538;

		PADDING(0x540-0x538-4);

		i32 field_540;

		PADDING(0x548-0x540-4);

		// twist-around-Y correction angle (PS1 GTE units), applied on top of
		// the normal-aligned basis by CPlayer::OrientToNormal via
		// M3dMaths_RotMatrixYXZ + MulMatrix when nonzero.
		i32 field_548;

		u8 field_54C;

		PADDING(0x568-0x54C-4);

		i32 field_568;
		i32 field_56C;

		u32 field_570;
		PADDING(8);

		i8 field_57C;

		PADDING(0x580-0x57C-1);

		i32 field_580;
		CSmokeTrail* field_584;
		CSmokeTrail* field_588;
		CSmokeTrail* field_58C;
		CSmokeTrail* field_590;

		PADDING(0x5D0-0x590-4);


		i32 field_5D0;
		i32 mWebbing;
		i32 field_5D8;
		i32 field_5DC;

		i32 field_5E0;

		// SFX handle, stopped (SFX_Stop) and cleared by
		// SynthesizeAnalogueInput's opcode 1 (teleport) handler.
		i32 field_5E4;

		char field_5E8;
		bool field_5E9;

		PADDING(0x5EC-0x5E9-sizeof(bool));
		
		i32 field_5EC;

		SIndicator field_5F0[6];


		PADDING(0x89C-0x5F0-(sizeof(SIndicator)*6));

		MATRIX field_89C;

		// running max/min CBody::mPlayerDist across the qualifying baddies
		// found this pass, set up in BuildOffscreenSpideySenseIndicatorList
		u32 field_8BC;
		u32 field_8C0;

		i32 field_8C4;
		i32 field_8C8;
		CVector field_8CC;

		u8 field_8D8;

		PADDING(3);

		i32 field_8DC;

		PADDING(0x8E8-0x8DC-4);


		u8 field_8E8;
		u8 field_8E9;
		u8 field_8EA;

		PADDING(1);

		u8 gCamAngleLock; //8EC

		PADDING(0x8F0-0x8EC-1);

		// ReadAnalogueInput: accumulates +32 per tick while the move input is
		// non-centre (clamped at 256), reset to 0 when it centres.
		i32 field_8F0;

		PADDING(0xAB8-0x8F0-4);

		SHandle field_AB8;

		PADDING(0xAC8-0xAB8-sizeof(SHandle));

		CVector field_AC8;

		u8 field_AD4;

		PADDING(0xAD7-0xAD4-1);

		u8 field_AD7;

		// ReadAnalogueInput: one-shot "move input just centred" flags.
		u8 field_AD8;
		u8 field_AD9;

		PADDING(0xAE4-0xAD9-1);

		u8 field_AE4;
		u8 field_AE5;
		u8 field_AE6;

		PADDING(0xB74-0xAE6-1);

		i32 field_B74;

		PADDING(0xB84-0xB74-4);

		CSVector field_B84;

		PADDING(0xB8C-0xB84-sizeof(CSVector));

		// @FIXME guess the type
		i32* field_B8C;

		PADDING(0xC18-0xB8C-4);

		i32 field_C18;
		CVector field_C1C;
		CSVector field_C28;

		PADDING(2);


		// @FIXME
		i32* field_C30;

		PADDING(0xC6C-0xC30-4);

		CVector field_C6C;
		i32 field_C78;
		i32 field_C7C;
		i32 field_C80;
		CVector field_C84;
		i32 field_C90;

		// player body orientation as a quaternion (from MToQ(mTransform)),
		// used by EnterLookaroundMode to seed the lookaround camera path.
		CQuat field_C94;

		// inverse of the active camera's orientation quaternion, the other
		// endpoint of the EnterLookaroundMode Quat_Slerp path.
		CQuat field_CA4;

		i32 field_CB4;

		// camera position snapshot taken by EnterLookaroundMode.
		CVector field_CB8;

		PADDING(0xCE4-0xCB8-0xC);

		i32 field_CE4;

		PADDING(0xD00-0xCE4-4);

		// hook-8 world position plus field_C84*0x80, used by
		// EnterLookaroundMode as the lookaround camera anchor.
		CVector field_D00;

		// field_C84*0x80, stashed by EnterLookaroundMode.
		CVector field_D0C;

		// -field_A8 (negated surface normal, long-vector/GTE-width, pad
		// word included), cached by CPlayer::OrientToNormal every call.
		VECTOR field_D18;

		PADDING(0xD3C-0xD18-sizeof(VECTOR));

		CVector field_D3C;

		PADDING(((0xD4E)-0xD3C)-sizeof(CVector));

		CSVector field_D4E;

		PADDING((0xD60)-(0xD4E)-sizeof(CSVector));

		// 0 = near web-attach point (field_D64) is valid, 1 = far one
		// (field_D70) is valid. Set by CheckSwingWebAvailability.
		u8 field_D60;

		PADDING(0xD64-0xD60-1);

		// candidate swing-web attach points, computed by
		// CheckSwingWebAvailability. field_D60 selects which is active.
		CVector field_D64;
		CVector field_D70;

		PADDING(0xD80-0xD70-sizeof(CVector));

		CSVector field_D80;
		CSVector field_D86;
		CSVector field_D8C;

		PADDING(0xDA0-0xD8C-sizeof(CSVector));

		CVector field_DA0;

		PADDING(0xDB8-0xDA0-sizeof(CVector));

		i32 field_DB8;

		PADDING(0xDC0-0xDB8-4);

		CVector field_DC0;

		// @FIXME - type
		CBody *field_DCC;

		PADDING(0xDD8-0xDCC-4);

		// grab target handle, recovered via Mem_RecoverPointer in GrabUpdate
		SHandle field_DD8;

		PADDING(0xDE4-0xDE0);

		char field_DE4;

		PADDING(0xDE8-0xDE4-1);

		// reticle color: low 3 bytes are packed r0/g0/b0, OR'd with 0x2C
		// (poly tag/code byte) at use in DrawReticle.
		i32 field_DE8;

		// reticle sprite: OffX/OffY/Width/Height + Texture*, read by
		// DrawReticle via Panel_DrawTexturedPoly(SAnimFrame*, i32).
		SAnimFrame *field_DEC;

		i32 field_DF0;
		i32 field_DF4;

		i32 field_DF8;
		i32 field_DFC;

		i32 field_E00;

		PADDING(0xE0C-0xE00-4);

		i32 field_E0C;
		char field_E10;

		i16 field_E12;

		PADDING(4);

		i32 field_E18;
		i32 field_E1C;

		PADDING(0xE2D-0xE1C-4);

		char field_E2D;
		char field_E2E;

		// ReadAnalogueInput: previous-tick copy of field_E2D/field_E2E.
		char field_E2F;
		char field_E30;

		PADDING((0xE32-0xE30)-0x1);

		// used as an index (masked to 0xFFF, then scaled) into
		// word_610C4A/word_610C48 in CPlayer::PutCameraBehind.
		i16 field_E32;

		// ReadAnalogueInput: previous aim angle; field_E32 is the new angle.
		i16 field_E34;

		PADDING((0xE38-0xE34)-0x2);

		i32 field_E38;

		PADDING((0xE48-0xE38)-0x4);

		CManipOb* mHeldObject;

		PADDING(0xE64-0xE48-4);

		// @FIXME guess the type, used as a scalar-deleting-destructor
		// pointer (vtable[0](1)) in CPlayer::SwitchToDeathMode
		i32* field_E64;

		PADDING(0xE6C-0xE64-4);

		// @FIXME guess the type, same scalar-deleting-destructor idiom
		// as field_E64 (vtable[0](1)), used by
		// SynthesizeAnalogueInput's opcode 1 (teleport) handler.
		i32* field_E6C;

		SHandle hLockTarget;

		PADDING(0xE84-0xE70-sizeof(SHandle));

		i32 field_E84;
		i32 field_E88;

		u8 field_E8C;

		PADDING(0xEA0-0xE8C-1);

		// ReadAnalogueInput: aim correction ratio (field_EA0/field_EA2).
		u16 field_EA0;
		u16 field_EA2;

		u8 field_EA4;

		PADDING(1);

		// zeroed at the top of every SynthesizeAnalogueInput call; exact
		// purpose unclear.
		i16 field_EA6;

		u16 field_EA8;

		PADDING(0xEC0-0xEAA);

		// set to 1 in BuildOffscreenSpideySenseIndicatorList when at least
		// one qualifying baddy was found this pass
		u8 field_EC0;

		PADDING(0xEF0-0xEC0-1);

		i32 mMaxHealth;

		PADDING(0xEF8-0xEF0-4);

		// default perpendicularisation radius (fallback value used by
		// GetPerpendicularisationRadius outside the special zone-1797 case).
		i32 field_EF8;


		EXPORT void SetCamAngleLock(u16);
		EXPORT void ExitLookaroundMode(void);
		EXPORT void SetIgnoreInputTimer(i32);
		EXPORT void PutCameraBehind(i32);
		EXPORT void SetSpideyLookaroundCamValue(u16, u16, i16);
		EXPORT void SetTargetTorsoAngleToThisPoint(CVector *a2);

		EXPORT i16 GetEffectiveHeading(void);
		EXPORT void SetTargetTorsoAngle(i16, i32);
		EXPORT char DecreaseWebbing(i32);
		EXPORT void RenderLookaroundReticle(void);
		EXPORT void SetTargetTorsoAngle(i16, bool);
		EXPORT void CreateJumpingSmashKickTrail(void);
		EXPORT void PlaySingleAnim(i32, i32, i32);
		EXPORT void CutSceneSkipCleanup(void);
		EXPORT void OrientToNormal(bool, CVector*);
		EXPORT void PriorToVenomDistanceAttack(CVector);
		EXPORT void SwitchToStandMode(void);
		EXPORT void TidyUpZipWebLandingPosition(i32);
		EXPORT void CreateFists(u8);
		EXPORT u8 CanITalkRightNow(void);
		EXPORT u8 SetFireWebbing(void);
		EXPORT void GetHookPosition(CVector*, u8);
		EXPORT void DestroyJumpingSmashKickTrail(void);
		EXPORT void DestroyHandTrails(void);
		EXPORT void DeleteStuff(void);
		EXPORT void StopAlertMusic(void);
		EXPORT void KillAllCommandBlocks(void);
		EXPORT i32* KillCommandBlock(i32*);
		EXPORT void Die(void);
		EXPORT void SetStartOrientation(CSVector*);
		EXPORT u8 IncreaseWebbing(i32);

		EXPORT void AI(void);
		EXPORT void AdjustBrightness(u16);
		EXPORT void BuildOffscreenSpideySenseIndicatorList(void);
		EXPORT CPlayer(void);
		EXPORT i32 CalculateIntermediateTrailSteps(CVector *,CVector *,CVector *);
		EXPORT void CalculateSwingWebParameters(CVector *);
		EXPORT void CalculateTugWebPathPoints(void);
		EXPORT u8 CheckCeilingJumpingSmashPunch(void);
		EXPORT void CheckExteriorSurfaceTransition(void);
		EXPORT void CheckFenceSurfaceTransition(void);
		EXPORT void CheckForwards(bool);
		EXPORT i32 CheckGroundGone(void);
		EXPORT void CheckInteriorSurfaceTransition(void);
		EXPORT void CheckJump(void);
		EXPORT void CheckJumpingR1ZipWeb(void);
		EXPORT void CheckJumpingR2ZipWeb(void);
		EXPORT void CheckJumpingSmashKick(void);
		EXPORT void CheckJumpingSwingWeb(void);
		EXPORT void CheckKick(void);
		EXPORT void CheckLanded(void);
		EXPORT i32 CheckRunIntoWall(void);
		EXPORT i32 CheckStickToCeiling(void);
		EXPORT void CheckStickToWall(void);
		EXPORT u8 CheckSwingWebAvailability(SLineInfo *);
		EXPORT void CheckSwitchToGrabbedMode(CVector const *,CVector *);
		EXPORT void CheckWebShot(void);
		EXPORT u8 CheckZipWebAvailability(SLineInfo *,i32);
		EXPORT void CollideWithObject(CBody *);
		EXPORT void CreateCombatImpactEffect(CVector *,i32);
		EXPORT void CreateWebDrips(bool,bool);
		EXPORT void DoMGSShadow(void);
		EXPORT void DoShadowCheck(void);
		EXPORT void DrawOffscreenSpideySenseIndicatorList(void);
		EXPORT void DrawReticle(u16,u16,u32);
		EXPORT void EnterLookaroundMode(void);
		EXPORT void FireWeb(bool,i32,CVector *,bool,CSVector *);
		EXPORT void GetComboFrameInfoPointer(u16);
		EXPORT void GetComboPartsInfoPointer(u16);
		EXPORT i32 GetDamageInflictedFromDifficulty(i32);
		EXPORT void GetEnterExitFrameInfoPointer(u16);
		EXPORT i32 GetFreeIndicatorListEntry(void);
		EXPORT i32* GetNewCommandBlock(u32);
		EXPORT i32 GetPerpendicularisationRadius(void);
		EXPORT u8 GrabUpdate(CVector *,i16 *);
		EXPORT void HandleControlsForSurfaceTransition(bool);
		EXPORT i32 Hit(SHitInfo *) OVERRIDE;
		EXPORT u8 IfPlayerCeilingCheck(i32,i32);
		EXPORT i32 IncHealth(i32);
		EXPORT void InitialiseOffscreenSpideySenseIndicatorList(void);
		EXPORT void InitialiseSFXArray(void);
		EXPORT void InitiateCombo(u16,i32);
		EXPORT u8 IsInIndicatorList(SHandle &);
		EXPORT u8 KnockSpideyFromCrawlPosition(void);
		EXPORT void LockTargetTorsoAngle(void);
		EXPORT void NotifyKill(u16);
		EXPORT void ParseFightData(void);
		EXPORT i32 ProcessSFXArray(void);
		EXPORT void ReadAnalogueInput(void);
		EXPORT void SelectAutoAimTarget(void);
		EXPORT void SelectTargetBaddy(i32,i32,i32,i32);
		EXPORT void SelectTargetSwitch(i32,i32,SHandle *,i32,i32);
		EXPORT u8 SetArmor(bool);
		EXPORT void SetCeilingCamera(i32);
		EXPORT void SetFallingCamera(i32);
		EXPORT void SetFirstContactDetails(void);
		EXPORT void SetFloorCamera(i32);
		EXPORT void SetFocusLockTarget(CBody const *);
		EXPORT void SetSpideyCamValue(u16,u16,i16,u16,u16);
		EXPORT void SetSwingCamera(i32);
		EXPORT void SetWallCamera(i32);
		EXPORT void SetupLookaroundCamera(void);
		EXPORT u8 ShouldPlayerDropFlail(void);
		EXPORT void SortAnimationFollowOnData(void);
		EXPORT void SortFistsData(void);
		EXPORT void SwitchToDeathMode(bool);
		EXPORT void SwitchToSynthesizedInput(i16 *);
		EXPORT void SynthesizeAnalogueInput(void);
		EXPORT void UpdateAndTrackCombo(void);
		EXPORT void UpdateOffscreenSpideySenseIndicatorList(void);
		EXPORT void UpdateTrails(void);
		EXPORT ~CPlayer(void);
		EXPORT void nullsub_one(i32);
		EXPORT void ResetSFXArrayEntry(u32);
};

EXPORT extern CPlayer* MechList;
EXPORT extern CItem* SpideyAdditionalBodyPartsList;
EXPORT extern CItem* MiscellaneousRenderingList;


EXPORT void Bruce_Sync(void);

EXPORT void Spidey_SetUserFunction(const char *, u32);
EXPORT void Spidey_FreeHeadModel(void);
EXPORT void Spidey_CopyHeadModel(i32);

EXPORT void Spidey_BagHead(i32,i32);
EXPORT void Spidey_DoArmorVRAMProcessing(bool);
EXPORT void Spidey_LoadAlternativeHealthIcon(i32);
EXPORT void Spidey_LoadAlternativeTextureSet(u32 const *,i32);
EXPORT void Spidey_StoreTextureEntry(Texture const *,i16,i16);
EXPORT void Spidey_SwapSuitTextures(i32,i32);
EXPORT void spideyLog(char *,...);

void validate_CPlayer(void);
void validate_SIndicator(void);

#endif
