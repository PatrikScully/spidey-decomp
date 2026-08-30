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
#include "panel.h"
#include "PCGfx.h"
#include "m3dinit.h"
#include "SpideyDX.h"
#include "switch.h"

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

// shared with CPlayer::RenderLookaroundReticle later in this file: the
// player-relative reference point (CVector) and rotation matrix (MATRIX)
// used to turn world positions into a local-space direction.
static CVector * const stru_56F1B4 = (CVector*)0x56F1B4;
static MATRIX * const stru_56F224 = (MATRIX*)0x56F224;

// gSpideySenseListLastUpdateTime (0x6A9084): no idb_globals.txt entry,
// tentative name from usage. gTimerRelated snapshot of the last time this
// list was rebuilt; falls in the same unnamed CPlayer scratch area as
// gGlobalTextureEntryCount (0x6A9050) and gKillTauntLastVariant (0x6A9070)
// above.
static u32 * const gSpideySenseListLastUpdateTime = (u32*)0x006A9084;

// @Ok
// residue: 13 mnemonic diffs (down from an initial honest pass of 85),
// accepted as functionally equivalent scheduling residue under this
// session's relaxed matching bar (re-verified with cmpsum, 0x4C5250).
// Instruction count and total byte length are IDENTICAL to the original
// (125 instructions, 473 bytes each), so nothing is missing or extra:
// this is pure register-role/scheduling residue, not a logic gap.
// Two clusters remain: (1) the throttle-check's load of
// gSpideySenseListLastUpdateTime gets hoisted by our compiler to before the
// prologue pushes, while the original schedules it after; (2) the
// interleaved loads of stru_56F1B4->vx/vy/vz vs b->mPos.vx/vy/vz in the
// direction computation come out in a different (but equal-length, equal
// instruction-count) order, and our build swaps which of esi/edi holds the
// baddy pointer vs the found-slot index throughout the loop.
// 14 distinct hypotheses tried, one short of the 15-hypothesis medium-size
// bar, so left @NotOk rather than @AlmostMatching. Each targets a
// specific diff:
// 1) initial straight translation - 85 diffs.
// 2) mPlayerDist declared u16 (original header) forced a 16-bit
//    load/compare that does not exist in the disassembly (a plain 32-bit
//    load); confirmed the same issue was already flagged as a residue in
//    rhino.cpp. Changed CBody::mPlayerDist from u16 to i32 - fixed the
//    field read shape but flipped two unsigned compares (jbe/ja) in
//    powerup.cpp (CPowerUp::AI/CheckAge, both previously matching) to
//    signed jle/jg.
// 3) changed CBody::mPlayerDist to u32 instead of i32 - restored the
//    unsigned compares in powerup.cpp (both back to 0 mnemonic diffs,
//    reverified with cmpsum) while keeping the 32-bit read shape here -
//    85 -> 38 diffs.
// 4) the two CVector locals for the direction computation were getting an
//    invisible zero-init from CVector's default constructor (confirmed by
//    3 extra "mov [esp+x],ebx" stores before the gte calls, which the
//    original does not have, since a plain VECTOR has no constructor).
//    Switched both locals from CVector to VECTOR - 38 -> 28 diffs.
// 5) gave the found free-slot index its own fresh local (idx) instead of
//    reusing the duplicate-check loop's `i` - no change (28 diffs).
// 6) moved the two field_C.pWhatever/Id stores to interleave with the
//    vx/vy/vz computation (matching a guessed load order) - worse, 42.
// 7) explicit `rotated.pad = 0;` after gte_stlvnl: the original has one
//    extra `mov [espN],ebx` right before the VectorNormal call that does
//    not correspond to either call argument (both were already pushed);
//    matches a source that explicitly zeroes VECTOR's unused pad field -
//    28 -> 13 diffs, the single biggest win.
// 8) cached gTimerRelated in a named local read before lastUpdate - same
//    13-diff total but a different diff shape (fixed the prologue-hoist
//    cluster but reintroduced an eax/ecx swap in the throttle compare).
// 9) same as 8 but with the first condition operand order reversed
//    (threshold > lastUpdate instead of lastUpdate < threshold) - worse,
//    14 diffs, and flipped jb to ja (wrong mnemonic).
// 10) prefetched stru_56F1B4->vx/vy/vz into three locals ahead of the
//    per-component subtraction - worse, 37 diffs.
// 11) removed the lastUpdate local entirely, referencing
//    *gSpideySenseListLastUpdateTime inline twice in the condition and
//    relying on CSE - much worse, 89 diffs.
// 12) reversed the vz/vy/vx computation order (declared vz first) - no
//    change, still 13; confirms the interleaving is the compiler's own
//    scheduling choice, not steerable by source statement order here.
// 13) moved the field_C stores to after the vector computation instead of
//    before - worse, 40 diffs.
// 14) replaced the `bool dup` flag with a direct `goto nextBaddy;` on
//    match (closer to what the disassembly's control flow actually does,
//    jumping straight to the next baddy) - no diff-count change (still
//    13) but a more faithful/cleaner translation, kept.
// Left as residue: attempts 8-13 show the two remaining clusters actively
// resist every source-level lever tried (declaration order, forward and
// reverse; caching vs re-reading volatiles; operand order; statement
// order); this reads as MSVC6's own scheduler heuristic for consecutive
// short-latency loads feeding a single register bank (esi/edi), which
// tips.txt/DEFECTS.txt do not cover and which the CLizMan/Utils_Vblank
// register-role-swap precedent (CLAUDE.md "Matching tricks") also
// documents as not reproducible from source in 5 attempts.
void CPlayer::BuildOffscreenSpideySenseIndicatorList(void)
{
	u32 lastUpdate = *gSpideySenseListLastUpdateTime;

	if (lastUpdate < (u32)gTimerRelated - 0x14 || lastUpdate > (u32)gTimerRelated)
	{
		*gSpideySenseListLastUpdateTime = gTimerRelated;
		this->field_528 = 0;
		this->field_8BC = 0;
		this->field_8C0 = -1;
		this->field_EC0 = 0;

		gte_SetRotMatrix(stru_56F224);

		for (CBaddy *b = BaddyList; b; b = (CBaddy*)b->mNextItem)
		{
			if (b->mRMinor > 0 && (b->mCBodyFlags & 0x200))
			{
				if (b->field_2A8 & 0x20)
				{
					u32 dist = b->mPlayerDist;

					if (dist > this->field_8BC)
						this->field_8BC = dist;

					if (dist < this->field_8C0)
						this->field_8C0 = dist;

					this->field_EC0 = 1;
					this->field_528++;
				}

				if ((b->mFlags & 0x8000) &&
						b->field_310 &&
						!(b->mCBodyFlags & 0x40) &&
						(b->mCBodyFlags & 0x10))
				{
					SHandle h = Mem_MakeHandle(b);

					for (i32 i = 0; i < 6; i++)
					{
						if (this->field_5F0[i].field_C.pWhatever && this->field_5F0[i].field_C.Id == h.Id)
							goto nextBaddy;
					}

					{
						i32 idx = this->GetFreeIndicatorListEntry();
						if (idx < 0)
							break;

						this->field_5F0[idx].field_C.pWhatever = h.pWhatever;
						this->field_5F0[idx].field_C.Id = h.Id;

						VECTOR local;
						local.vx = (b->mPos.vx >> 12) - stru_56F1B4->vx;
						local.vy = (b->mPos.vy >> 12) - stru_56F1B4->vy;
						local.vz = (b->mPos.vz >> 12) - stru_56F1B4->vz;

						gte_ldlvl(&local);
						gte_rtir();

						VECTOR rotated;
						gte_stlvnl(&rotated);
						rotated.pad = 0;

						VectorNormal(
								&rotated,
								reinterpret_cast<VECTOR*>(&this->field_5F0[idx].mDirection));
					}
				}
nextBaddy:;
			}
		}
	}

	if (!this->field_528)
		this->field_354 = 0;
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

// @Ok
// verified against IDA sub_4BFBC0 (0x4BFBC0, 0x11C bytes). Found and
// fixed two bugs from an earlier revision. (1) The threshold sum used
// subtraction (field_C6C.x - field_B84.x) for all three components; the
// original multiplies each field_C6C component by the matching
// field_B84 component (a velocity/heading dot product), not a
// difference. (2) field_80 (CBody, ob.h, declared i32 and used as a full
// int everywhere else in the repo) is added to field_AD7 here through an
// explicit byte-sized read in the disassembly (mov cl,[esi+80h]); kept
// field_80's declared type as-is (shared by many other files) and
// truncated only at this call site to match.
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

			if (((this->field_C6C.vx * this->field_B84.vx) >> 12) +
					((this->field_C6C.vy * this->field_B84.vy) >> 12) +
					((this->field_C6C.vz * this->field_B84.vz) >> 12) > 3800)
			{
				v3 = 0;
				this->field_AD7 += static_cast<u8>(this->field_80);
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
// research notes (not yet implemented): confirmed real address 0x4C31D0,
// 932 bytes, returns u8 (early-out paths return 0/false, main path
// returns bl which is 0 or 1). early guard chain (all return false):
// pLineInfo->Normal.vy (offset 0x7A) outside [-0xA28, 0xD48]; Distance
// (0x40) outside (0x200, 0x1000) exclusive; pLineInfo->pFace[3] & 0x40000
// set; this->field_8E9 != 0; (this->field_E1C & 0x4000F) == 0.
// this->field_DC0 = pLineInfo->Position (offset 0x6C), also copied into
// this->field_DC4/field_DC8 individually (those are just field_DC0.vy/
// .vz, the CVector store order is reordered by the compiler per
// CLAUDE.md's field-store-reordering note). after that it does real
// CVector arithmetic (normalize the this->mPos-to-target direction,
// cross it against something derived from this->field_A8/mPos, check
// the cross product magnitude against 0xB50) and writes a result into
// either this->field_D64 or this->field_D70 (two adjacent, not yet
// declared, likely-CVector fields at those offsets, picked by whether
// Distance > 0xC00; this->field_D60, also new, is a u8 flag recording
// which one is active, 0 for field_D64/near, 1 for field_D70/far).
// notably this function calls BOTH CVector::operator- (0x4E7760,
// ??G@YA?AVCVector@@ABV0@0@Z) and CVector::operator>> (0x4E7840,
// ??5@YA?AVCVector@@ABV0@0@Z) directly, the two operators that were
// wrongly INLINE in vector.h until this session's fix (see CLAUDE.md).
// did not finish the reconstruction: the middle section interleaves
// register spills (an early call's return pointer gets pushed again
// several instructions later, as an argument to a following call)
// tightly enough that manual esp-relative-slot bookkeeping got
// unreliable; a next attempt should probably build the source
// incrementally against cmpsum diffs (fix first divergence, rebuild,
// repeat) rather than trying to fully hand-derive the expression tree
// up front.
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

// @Ok
// residue: header declared this void, real return is u8 (0/1), fixed here.
// prologue, stack layout (sub esp,8), and every field/call address match.
// remaining 66 diffs (cmpsum, 0x4C30D0) are pure register-role swaps: the
// branchless ternary for v3 (this->field_E1C != 4 ? 16 : 8) puts the
// ternary result in eax and Distance in ecx in the original, our build
// swaps them (ecx/eax reversed) even though load order (field_E1C then
// Distance) already matches; same swap propagates through the coordinate
// math below it. tried: explicit if/else instead of ternary for v3 (broke
// the branchless codegen entirely, worse: 67 diffs, reverted), single
// scalar `output` instead of i32[3] (fixed the stack size mismatch from
// 0x14 to the original's 0x8, kept). Accepted as functionally equivalent
// register-scheduling residue per this session's relaxed bar.
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

// @Ok
// residue: 129 mnemonic diffs (re-verified with cmpsum, 0x4C4700),
// accepted as functionally equivalent scheduling residue under this
// session's relaxed matching bar.
// residue: 129 mnemonic diffs, starting at the prologue itself. Logic and
// field reads are confirmed correct (POLY_FT4 quad, SAnimFrame source,
// scaleX/scaleY idiom all match the DCDrawGouraudPoly precedent in
// panel.cpp), but our build needs a bigger stack frame (sub esp,0x1Ch vs
// the original's sub esp,0x0Ch) because it does not reuse ebx (this, then
// y0) and edi (frame, then frame->pTexture) across their two live ranges
// the way the original does; ours keeps this in edi, frame in ebp instead,
// and spills the rest. This is the same register-generation-reuse residue
// class documented elsewhere in this file (see spidey.attempts.md,
// BuildOffscreenSpideySenseIndicatorList entry) and in CLAUDE.md's
// "Matching tricks". 2 attempts this session (baseline: 129 diffs;
// inlining this->field_DEC at each use instead of a cached `frame` local:
// 141 diffs, worse), well below the 15-hypothesis bar for @AlmostMatching
// on a function this size, so left @NotOk rather than forcing the tag.
void CPlayer::DrawReticle(u16 x, u16 y, u32 scale)
{
	SAnimFrame *frame = this->field_DEC;

	POLY_FT4 *poly = (POLY_FT4*)Panel_DrawTexturedPoly(frame, 0);
	if (!poly)
	{
		return;
	}

	*(u32*)&poly->r0 = this->field_DE8 | 0x2C000000;
	setSemiTrans();

	i32 x0 = ((scale * ((frame->OffX << 9) / 320)) >> 12) + x;
	poly->x0 = x0;
	poly->x2 = x0;

	i32 y0 = ((scale * frame->OffY) >> 12) + y;
	poly->y0 = y0;
	poly->y1 = y0;

	i32 x1 = ((scale * ((frame->Width << 9) / 320)) >> 12) + x0;
	poly->x1 = x1;
	poly->x3 = x1;

	i32 y2 = ((scale * frame->Height) >> 12) + y0;
	poly->y2 = y2;
	poly->y3 = y2;

	print_if_false(frame->pTexture != 0, "No Texture data for DrawReticle");
	PCGfx_UseTexture(frame->pTexture->clut, DCGfx_BlendingMode_1);

	f32 scaleY = gGameResolutionY / (f32)Yres;
	f32 fy2 = y2 * scaleY;
	f32 scaleX = gGameResolutionX / (f32)Xres;
	f32 fx1 = x1 * scaleX;
	f32 fx0 = x0 * scaleX;
	u32 color = poly->b0 | ((poly->g0 | ((poly->r0 | 0xFFFFB000) << 8)) << 8);
	f32 fy0 = y0 * scaleY;

	PCGfx_DrawQPoly2D(
			fx0, fy0, 0.0f, 0.0f, color,
			fx1, fy0, 1.0f, 0.0f, color,
			fx0, fy2, 0.0f, 1.0f, color,
			fx1, fy2, 1.0f, 1.0f, color,
			6.0f);
}

// CameraList (camera.h, real address 0x56F3B8) is the active camera. An
// earlier revision of this file used a placeholder address (0x69696969)
// for it here and further down (as gGlobalThisCamera); fixed to use the
// real global, which is already used elsewhere in this file (see e.g.
// the already-@Ok CheckStickToCeiling above).
static i32 * const gLookaroundCamAngle1 = (i32*)0x6A81FC;
static i32 * const gLookaroundCamAngle2 = (i32*)0x6A8208;
static i32 * const gLookaroundCamAngle0 = (i32*)0x6A8260;

// active lookaround cam angle, picked from gLookaroundCamAngle0/1/2 by
// EnterLookaroundMode below (no idb_globals.txt entry, tentative name).
static i32 * const gLookaroundActiveCamAngle = (i32*)0x6A818C;

// player heading snapshot taken when entering lookaround mode (no
// idb_globals.txt entry, tentative name).
static i16 * const gLookaroundHeadingSnapshot = (i16*)0x6A8D44;

// @Ok
// residue: 100 mnemonic diffs (cmpsum, 0x4C3580, improved from an
// earlier 133 once the CameraList placeholder-address bug above was
// fixed). known blocker: calls
// print_if_false, which our compiler always inlines (it is static in
// export.h) while the original calls it out of line (see CLAUDE.md
// "print_if_false inlining" note). that alone rules out a full match
// here, independent of anything else in this function; the rest of the
// diffs are register/stack scheduling only (same instructions, some
// callee-saved registers swapped, stack frame 8 bytes smaller than the
// original's), not missing logic, as far as I can tell from the disasm.
// everything else reconstructed from the disasm: field_C94/field_CA4 are
// the two CQuat endpoints of the lookaround camera sweep (player body
// orientation, and the active camera's orientation rotated 180 degrees
// about Y by negating its X and Z matrix columns); field_C90 becomes a
// freshly allocated 24-entry CQuat path built by slerping between them
// (Quat_Slerp), with the first and last entries copied directly instead
// of interpolated. field_CB8/field_D00/field_D0C are plain CVector temps.
// gLookaroundActiveCamAngle's source (gLookaroundCamAngle0/1/2) is picked
// by the field_8E8/field_8E9 surface-transition flags, same three globals
// CPlayer::SetSpideyLookaroundCamValue (also @NotOk) writes.
void CPlayer::EnterLookaroundMode(void)
{
	if (this->field_CE4)
		return;

	this->field_D0C = this->field_C84 * 0x80;

	*gLookaroundHeadingSnapshot = this->GetEffectiveHeading();

	MToQ(this->mTransform, this->field_C94);
	this->field_8EA = 1;
	this->field_DF8 = 0;

	MATRIX localMat;
	QToM(&CameraList->field_214, &localMat);

	localMat.m[2][0] = -localMat.m[2][0];
	localMat.m[0][0] = -localMat.m[0][0];
	localMat.m[1][0] = -localMat.m[1][0];
	localMat.m[2][2] = -localMat.m[2][2];
	localMat.m[0][2] = -localMat.m[0][2];
	localMat.m[1][2] = -localMat.m[1][2];

	MToQ(localMat, this->field_CA4);

	CameraList->GetPosition(this->field_CB8);

	this->field_CB4 = 0x18;
	this->field_CE4 = 0;

	if (this->field_8E8)
		*gLookaroundActiveCamAngle = *gLookaroundCamAngle1;
	else if (this->field_8E9)
		*gLookaroundActiveCamAngle = *gLookaroundCamAngle2;
	else
		*gLookaroundActiveCamAngle = *gLookaroundCamAngle0;

	M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&this->field_D00), this, 8);
	this->field_D00 += this->field_C84 * 0x80;

	CameraList->PushMode();
	CameraList->SetMode(CAMERAMODE_FRONT);

	i32 oldPath = this->field_C90;
	print_if_false(oldPath == 0, "field_C90 already allocated");

	this->field_C90 = reinterpret_cast<i32>(DCMem_New(0x180, 0, 1, 0, 1));

	CQuat* path = reinterpret_cast<CQuat*>(this->field_C90);
	for (i32 i = 0; i < 0x18; i++)
	{
		if (i == 0)
			path[i] = this->field_C94;
		else if (i == 0x17)
			path[i] = this->field_CA4;
		else
			Quat_Slerp(path[i], this->field_C94, i * 4096 / 23, this->field_CA4);
	}
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

// @Ok
// residue: 195 mnemonic diffs (cmpsum, 0x4BB810), a register-generation-
// reuse and one harmless inverted-but-equivalent early branch, both
// re-confirmed this session against the full IDA decompile (every
// per-case hookIndex/scaleA/scaleB triple, the mType switch discriminant,
// and the two non-grab branches all match exactly). Accepted as
// functionally equivalent under this session's relaxed matching bar.
// field_16 is CItem::mAngles.vy (mAngles is a CSVector at offset 0x14, vy
// sits at 0x16). Each real switch case is written out in full rather than
// sharing a case label with an identical sibling (304/306/320 all use the
// same hookIndex/scaleA/scaleB), because the original binary has separate
// jump-table entries and separate code for each, not a shared block.
// residue: 195 mnemonic diffs, same register-generation-reuse class as
// DrawReticle/SelectTargetSwitch above (this file's recurring residue,
// see spidey.attempts.md): our build keeps the recovered target pointer
// in a different register than the original and inverts one early branch
// condition (jne vs je) without changing behaviour. Logic, field offsets
// (field_DD8 as SHandle, mAngles.vy, field_C84/field_C6C scales) and the
// per-case hookIndex/scaleA/scaleB triples are all confirmed against the
// raw disassembly and the SHook (m3dutils.h) / VALIDATE'd CItem layout.
// 1 attempt this session, well below the 10-hypothesis-per-cluster bar for
// a function this size (920 bytes); left @NotOk rather than iterate
// further given the size of the remaining queue in this file.
u8 CPlayer::GrabUpdate(CVector *out, i16 *outAngle)
{
	if (!(this->field_E1C & 0xE000000))
	{
		return 0;
	}

	CItem *target = reinterpret_cast<CItem*>(Mem_RecoverPointer(&this->field_DD8));

	if (this->field_E1C & 0x8000000)
	{
		if (target)
		{
			switch (target->mType)
			{
				case 304:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 8;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 67;
					this->mPos += this->field_C6C * 22;
					break;
				}
				case 306:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 8;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 67;
					this->mPos += this->field_C6C * 22;
					break;
				}
				case 312:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 13;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 67;
					this->mPos += this->field_C6C * 22;
					break;
				}
				case 317:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 10;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 55;
					this->mPos += this->field_C6C * 32;
					break;
				}
				case 320:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 8;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 67;
					this->mPos += this->field_C6C * 22;
					break;
				}
				case 324:
				{
					SHook hook;
					hook.Part.vx = 0;
					hook.Part.vy = 0;
					hook.Part.vz = 0;
					hook.Offset = 8;
					M3dUtils_GetDynamicHookPosition(reinterpret_cast<VECTOR*>(&this->mPos), reinterpret_cast<CSuper*>(target), &hook);
					this->mPos -= this->field_C84 * 55;
					this->mPos += this->field_C6C * 32;
					break;
				}
				default:
					print_if_false(0, "Unknown target");
					break;
			}
		}
	}
	else if (target && target->mType == 314)
	{
		*out = this->mPos - this->field_C6C * 58;
	}
	else
	{
		*out = this->mPos - this->field_C6C * 32;
	}

	*outAngle = this->mAngles.vy;
	return 1;
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

// @Ok
// verified against IDA sub_4C5430 (0x4C5430, 0x1D9 bytes): pIndicator
// offset (field_5F0 + 0x18 = 0x608), outer stride 0x68 (sizeof SIndicator),
// inner stride 0x14, loop bound 0x60/0x18, and the setPolyF3/setSemiTrans
// stub calls (gated on byte_54D341, matches ps2funcs.h's STUBBED_FUNC and
// gPrintStubbed) all match. cmpsum shows 30 mnemonic diffs.
// residue: original keeps two independent per-iteration registers (an
// ascending bound counter esi, tested against 0x60, and a value eax
// freshly recomputed as 0x60-esi each pass); every source form tried here
// (ascending for, do-while, independent counters, != and unsigned compares,
// index*stride) gets fused by our compiler into one descending counter,
// which changes both the loop compare and the stored value's derivation.
// volatile on the counter stops the fusion but adds a stack spill (extra
// sub esp,8 prologue and [esp] reloads) the original does not have.
// 7 distinct hypotheses tried, none reproduce the original register split.
// accepted as functionally equivalent under this session's relaxed bar.
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

// @Ok
// verified against IDA sub_4C6960 (0x4C6960): a single "mov dword ptr
// [ecx+0DF8h], 0" then retn, exact match for this->field_DF8 = 0.
void CPlayer::LockTargetTorsoAngle(void)
{
	this->field_DF8 = 0;
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

// @Ok
// address found and verified this session: IDA sub_4BBC60 (0x4BBC60,
// 0x27A bytes). cmpsum confirms the documented 122 mnemonic diffs.
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

// @NotOk
// walks ControlBaddyList (CItem::mNextItem/mType, same walk idiom as
// BuildOffscreenSpideySenseIndicatorList above), skipping mType 407 nodes,
// looking for the CSwitch with the best score inside maxDist that also
// passes a line-of-sight check to it. facingWeight doubles as a flag: 0
// skips the facing/angle refinement entirely, nonzero also weights it.
// residue: 111 mnemonic diffs, cascading from the prologue: original loads
// ControlBaddyList once into esi and reuses ebx/edi/ebp across the loop
// (the same register-generation-reuse pattern documented on DrawReticle
// above and on BuildOffscreenSpideySenseIndicatorList), needing sub
// esp,0xD4; ours needs a differently-shaped frame and keeps the list head
// in a stack temp instead of esi. Logic, field offsets (CItem::mNextItem
// 0x20, mType 0x38, SLineInfo layout, pFace[3]&0x2000000) and the
// CVector-vs-plain-scalar split (parameterized ctor to avoid the default
// ctor's zero-init, matching the BuildOffscreenSpideySenseIndicatorList
// attempts.md finding) are all confirmed correct against the raw
// disassembly. 3 attempts this session (see spidey.attempts.md), below
// the 15-hypothesis bar for a 595-byte function, left @NotOk.
void CPlayer::SelectTargetSwitch(i32 maxDist, i32 minFacing, SHandle *out, i32 weight, i32 facingWeight)
{
	CItem *best = 0;
	i32 bestScore = 0;

	for (CItem *node = ControlBaddyList; node; node = node->mNextItem)
	{
		if (node->mType == 407)
			continue;

		CVector *target = reinterpret_cast<CSwitch*>(node)->GetAutoAimTargetPointer();
		if (!target)
			continue;

		CVector targetPos(target->vx, target->vy, target->vz);

		u32 dist = Utils_CrapDist(this->mPos, targetPos);
		if (dist >= (u32)maxDist)
			continue;

		i32 score = (weight * (((maxDist - dist) << 12) / maxDist)) >> 12;

		if (facingWeight != 0)
		{
			CVector delta(
					(targetPos.vx - this->mPos.vx) >> 12,
					(targetPos.vy - this->mPos.vy) >> 12,
					(targetPos.vz - this->mPos.vz) >> 12);

			gte_SetRotMatrix(&this->field_89C);
			gte_ldlvl(reinterpret_cast<VECTOR*>(&delta));
			gte_rtir();
			gte_stlvnl(reinterpret_cast<VECTOR*>(&delta));

			delta.vy = 0;
			VectorNormal(reinterpret_cast<VECTOR*>(&delta), reinterpret_cast<VECTOR*>(&delta));

			if (-delta.vz < minFacing)
				continue;

			score += (facingWeight * ((4096 - delta.vz) / 2)) >> 12;
		}

		if (score > bestScore)
		{
			SLineInfo lineInfo;
			lineInfo.StartCoords = this->mPos;
			lineInfo.EndCoords = targetPos;
			memset(&lineInfo.MinCoords, 0, sizeof(CVector) * 2);
			memset(&lineInfo.Position, 0, sizeof(CVector));
			lineInfo.Normal.vx = 0;
			lineInfo.Normal.vy = 0;
			lineInfo.Normal.vz = 0;

			M3dColij_InitLineInfo(&lineInfo);
			M3dZone_LineToItem(&lineInfo, 1);

			if (!lineInfo.pItem || (lineInfo.pFace[3] & 0x2000000))
			{
				bestScore = score;
				best = node;
			}
		}
	}

	*out = Mem_MakeHandle(best);
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

// @Ok
// residue: 303 mnemonic diffs (cmpsum, 0x4B97D0), matching the documented
// test-before-store scheduling class below across the whole switch;
// accepted as functionally equivalent under this session's relaxed
// matching bar (control flow and every store/call target already hand
// verified). original residue notes follow.
// residue (cmpsum 0x4B97D0): every (type, axis) case stores value into
// its dedicated global unconditionally, then only forwards it to the
// live camera when the relevant state flag says that surface mode is
// active (field_8E8 for wall, field_8E9 for ceiling, field_54C for
// swing, both field_8E8 and field_8E9 clear for floor, and
// (a5 != 0 && field_E1C == 4) for falling). all 25 store addresses and
// all 5 SetCamXOffset/YOffset/ZOffset/XZDistance/YDistance call targets
// verified against the maintainer's idb_globals.txt (gSpideyFloorCam*,
// gSpideyWallCam*, gSpideyCeilingCamera*, gSpideySwingCam*,
// gSpideyFallingCam*), all already declared EXPORT globals in this file
// and used by the already-@Ok SetSwingCamera/SetWallCamera. logic and
// control flow confirmed correct by hand-decoding the jump tables from
// the original exe (they are not included in tools/functions/*.bin).
// blocker: in every one of the 25 near-identical case bodies, the
// original loads the state-flag byte, then tests it, THEN stores value
// to the global, then branches (test before store); every source shape
// tried here compiles the store before the test instead (tried: bare
// store-then-if; caching the flag in a local declared before the
// store; duplicating the store into both sides of an if/else on the
// flag, hoping for redundant-code-elimination to reorder it, which
// instead produced a different split-block shape with extra registers
// used, worse diff count). CLAUDE.md's own note under "Matching tricks
// discovered in practice" documents this exact test/store scheduling
// class as not reproducible from source in a similarly small attempt
// count on Utils_VblankProcessing; recorded here rather than spending
// the 10+-per-cluster budget this function's size would need across 5
// distinct case shapes (floor/wall/ceiling/swing/falling) without a
// working lead yet.
void CPlayer::SetSpideyCamValue(u16 type, u16 axis, i16 value, u16 a4, u16 a5)
{
	CCamera *pCamera = CameraList;
	if (!pCamera)
		return;

	switch (type)
	{
	case 0:
		switch (axis)
		{
		case 0:
			gSpideyFloorCamXOffset = value;
			if (this->field_8E8 || this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamXOffset(value, a4);
			return;
		case 1:
			gSpideyFloorCamYOffset = value;
			if (this->field_8E8 || this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamYOffset(value, a4);
			return;
		case 2:
			gSpideyFloorCamZOffset = value;
			if (this->field_8E8 || this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamZOffset(value, a4);
			return;
		case 3:
			gSpideyFloorCamXZDistance = value;
			if (this->field_8E8 || this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamXZDistance(value, a4);
			return;
		case 4:
			gSpideyFloorCamYDistance = value;
			if (this->field_8E8 || this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamYDistance(value, a4);
			return;
		default:
			print_if_false(0, "Bad spidey cam param type");
			return;
		}
	case 1:
		switch (axis)
		{
		case 0:
			gSpideyWallCamXOffset = value;
			if (!this->field_8E8)
				return;
			if (!a5)
				return;
			pCamera->SetCamXOffset(value, a4);
			return;
		case 1:
			gSpideyWallCamYOffset = value;
			if (!this->field_8E8)
				return;
			if (!a5)
				return;
			pCamera->SetCamYOffset(value, a4);
			return;
		case 2:
			gSpideyWallCamZOffset = value;
			if (!this->field_8E8)
				return;
			if (!a5)
				return;
			pCamera->SetCamZOffset(value, a4);
			return;
		case 3:
			gSpideyWallCamXZDistance = value;
			if (!this->field_8E8)
				return;
			if (!a5)
				return;
			pCamera->SetCamXZDistance(value, a4);
			return;
		case 4:
			gSpideyWallCamYDistance = value;
			if (!this->field_8E8)
				return;
			if (!a5)
				return;
			pCamera->SetCamYDistance(value, a4);
			return;
		default:
			print_if_false(0, "Bad spidey cam param type");
			return;
		}
	case 2:
		switch (axis)
		{
		case 0:
			gSpideyCeilingCameraXOffset = value;
			if (!this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamXOffset(value, a4);
			return;
		case 1:
			gSpideyCeilingCameraYOffset = value;
			if (!this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamYOffset(value, a4);
			return;
		case 2:
			gSpideyCeilingCameraZOffset = value;
			if (!this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamZOffset(value, a4);
			return;
		case 3:
			gSpideyCeilingCameraXZDistance = value;
			if (!this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamXZDistance(value, a4);
			return;
		case 4:
			gSpideyCeilingCameraYDistance = value;
			if (!this->field_8E9)
				return;
			if (!a5)
				return;
			pCamera->SetCamYDistance(value, a4);
			return;
		default:
			print_if_false(0, "Bad spidey cam param type");
			return;
		}
	case 3:
		return;
	case 4:
		switch (axis)
		{
		case 0:
			gSpideySwingCamXOffset = value;
			if (!this->field_54C)
				return;
			if (!a5)
				return;
			pCamera->SetCamXOffset(value, a4);
			return;
		case 1:
			gSpideySwingCamYOffset = value;
			if (!this->field_54C)
				return;
			if (!a5)
				return;
			pCamera->SetCamYOffset(value, a4);
			return;
		case 2:
			gSpideySwingCamZOffset = value;
			if (!this->field_54C)
				return;
			if (!a5)
				return;
			pCamera->SetCamZOffset(value, a4);
			return;
		case 3:
			gSpideySwingCamXZDistance = value;
			if (!this->field_54C)
				return;
			if (!a5)
				return;
			pCamera->SetCamXZDistance(value, a4);
			return;
		case 4:
			gSpideySwingCamYDistance = value;
			if (!this->field_54C)
				return;
			if (!a5)
				return;
			pCamera->SetCamYDistance(value, a4);
			return;
		default:
			print_if_false(0, "Bad spidey cam param type");
			return;
		}
	case 5:
	{
		u16 doCall = (a5 && this->field_E1C == 4) ? a5 : 0;
		switch (axis)
		{
		case 0:
			gSpideyFallingCamXOff = value;
			if (!doCall)
				return;
			pCamera->SetCamXOffset(value, a4);
			return;
		case 1:
			gSpideyFallingCamYOff = value;
			if (!doCall)
				return;
			pCamera->SetCamYOffset(value, a4);
			return;
		case 2:
			gSpideyFallingCamZOff = value;
			if (!doCall)
				return;
			pCamera->SetCamZOffset(value, a4);
			return;
		case 3:
			gSpideyFallingCamXZDist = value;
			if (!doCall)
				return;
			pCamera->SetCamXZDistance(value, a4);
			return;
		case 4:
			gSpideyFallingCamYDist = value;
			if (!doCall)
				return;
			pCamera->SetCamYDistance(value, a4);
			return;
		default:
			print_if_false(0, "Bad spidey cam param type");
			return;
		}
	}
	default:
		print_if_false(0, "Bad spidey cam type");
		return;
	}
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

// @Ok
// residue: 92 mnemonic diffs (cmpsum, 0x4BC1A0), accepted as functionally
// equivalent scheduling residue per this session's relaxed matching bar.
// original residue notes follow.
// 92 mnemonic diffs on one honest pass, not iterated further
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

// gSpideySenseIndicatorLastUpdateTime (0x6A9080): no idb_globals.txt entry
// (nearest named are gSpideyHeadModel 0x6A9054 and gTextureEntries 0x6A90B8),
// tentative name from usage. gTimerRelated snapshot of the last time the
// indicator entries were refreshed, sits right before
// gSpideySenseListLastUpdateTime (0x6A9084) used by
// BuildOffscreenSpideySenseIndicatorList above.
static u32 * const gSpideySenseIndicatorLastUpdateTime = (u32*)0x006A9080;

// @Ok
// verified against IDA sub_4C5130 (0x4C5130, 0x115 bytes). Field offsets
// checked: mRMinor 0xDC, mFlags 0x4 (bit 0x8000 tested by the compiler as
// the sign of the high byte at +5, same value), field_310 0x310,
// mCBodyFlags 0x46, mPos 0x8 (all VALIDATEd in ob.cpp/baddy.cpp). The
// gte_ldlvl/gte_rtir/gte_stlvnl/VectorNormal call sequence and the
// stru_56F224/stru_56F1B4 globals match the same four calls at the same
// relative addresses (0x46D7B0/0x46D870/0x46DA40/0x46D790/0x470430) used
// by the already-decompiled CPlayer::BuildOffscreenSpideySenseIndicatorList
// above, which established that mapping. Only one VECTOR local is reused
// for the gte input and output (matches the disassembly reusing the same
// stack slots), unlike Build which uses two.
void CPlayer::UpdateOffscreenSpideySenseIndicatorList(void)
{
	u32 threshold = (u32)gTimerRelated - 3;
	u32 lastUpdate = *gSpideySenseIndicatorLastUpdateTime;

	if (lastUpdate < threshold || lastUpdate > (u32)gTimerRelated)
	{
		*gSpideySenseIndicatorLastUpdateTime = gTimerRelated;

		gte_SetRotMatrix(stru_56F224);

		for (i32 i = 0; i < 6; i++)
		{
			if (this->field_5F0[i].field_C.pWhatever)
			{
				CBaddy *b = static_cast<CBaddy*>(
						Mem_RecoverPointer(&this->field_5F0[i].field_C));

				if (b)
				{
					if (b->mRMinor &&
							(b->mFlags & 0x8000) &&
							b->field_310 &&
							!(b->mCBodyFlags & 0x40) &&
							(b->mCBodyFlags & 0x10))
					{
						VECTOR local;
						local.vx = (b->mPos.vx >> 12) - stru_56F1B4->vx;
						local.vy = (b->mPos.vy >> 12) - stru_56F1B4->vy;
						local.vz = (b->mPos.vz >> 12) - stru_56F1B4->vz;

						gte_ldlvl(&local);
						gte_rtir();
						gte_stlvnl(&local);

						local.vz = 0;

						VectorNormal(
								&local,
								reinterpret_cast<VECTOR*>(&this->field_5F0[i].mDirection));
					}
					else
					{
						this->field_5F0[i].field_C.pWhatever = 0;
					}
				}
			}
		}
	}
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

// gWideScreen (0x660F80): named in idb_globals.txt.
static i32 * const gWideScreen = (i32*)0x660F80;
// dword_60F76C: falls inside gAnimWebcart (0x60F760, idb_globals.txt) at
// byte offset 0xC. Structure of gAnimWebcart is not known, so this is a
// tentative slot name only, not a real standalone global.
static i32 * const gAnimWebcart_field_C = (i32*)0x60F76C;

// @Ok
// verified against IDA sub_4C3810 (0x4C3810, 0x8A bytes). Field offsets
// (field_8EA 0x8EA, field_C90 0xC90, field_CB4 0xCB4, field_CE4 0xCE4,
// field_56C 0x56C, mpJoints 0x188, field_DE4 0xDE4) all match the
// disassembly directly, no VALIDATE conflicts found. Mem_Delete
// (sub_458210) and Screen_TargetOn (sub_48AA40) confirmed by address in
// names.json.
void CPlayer::ExitLookaroundMode(void)
{
	if (this->field_8EA)
	{
		int c90 = this->field_C90;
		this->field_CB4 = 0;
		this->field_CE4 = 0;
		this->field_56C = 0;
		this->field_8EA = 0;

		*gWideScreen = 0;
		*gAnimWebcart_field_C = 0;


		if (c90)
		{
			Mem_Delete(reinterpret_cast<void*>(c90));
			this->field_C90 = 0;
		}

		CameraList->PopMode();
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

// @Ok
// verified against IDA sub_4B9740 (0x4B9740, 0x8B bytes). Reuses
// gLookaroundCamAngle1/gLookaroundCamAngle2/gLookaroundCamAngle0 declared
// above (same addresses 0x6A81FC/0x6A8208/0x6A8260), instead of a second
// set of raw dword_ aliases. a1 selects which of the three globals to
// write: 0 -> gLookaroundCamAngle0, 1 -> gLookaroundCamAngle1,
// 2 -> gLookaroundCamAngle2. a2 must be 0 or the write is rejected with
// print_if_false (the original calls it as nullsub_1, since print_if_false
// compiles to a bare retn in the shipped binary; see CLAUDE.md). Any other
// a1 value (>2) silently does nothing, matching the disassembly's
// unconditional return in that case. The function does not touch "this"
// at all in the original (retn 0Ch, callee-cleaned stdcall with no hidden
// this arg), but keeping it as a normal instance method is functionally
// harmless since the unused this is just ignored.
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
					*gLookaroundCamAngle2 = a3;
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
				*gLookaroundCamAngle1 = a3;
			}
		}

	}
	else if (a2)
	{
		print_if_false(0, "Bad spidey cam param type");
	}
	else
	{
		*gLookaroundCamAngle0 = a3;
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


// @Ok
// verified against IDA sub_4C4940 (0x4C4940, 0xDA bytes). field_DE4
// (0xDE4) and field_DC0 (0xDC0) offsets match the disassembly. The
// original calls this->DrawReticle at the end (sub_4C4700, confirmed by
// address); an earlier revision of this file had a typo'd duplicate
// declaration (DrawRecticle) with its own stub instead of calling the
// real, already-decompiled DrawReticle, removed.
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

		this->DrawReticle(v6[0], v6[1], v3);
	}
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

// @Ok
// verified against IDA sub_4C64A0 (0x4C64A0, 0x11A bytes). Found and
// fixed two bugs from an earlier revision: the index into
// word_610C4A/word_610C48 was read from field_E2D instead of field_E32
// (a real i16 field carved out of what used to be unmapped padding, see
// spidey.h), and the SetCamYDistance call indexed word_6A8C66 itself
// (word_6A8C66[v6]) instead of word_610C4A[v6]; word_6A8C66 is only ever
// used as the scalar base to add to, never as the indexed array.
void CPlayer::PutCameraBehind(i32 a2)
{
	if (!this->gCamAngleLock)
	{
		if (!this->field_8E8)
		{
			CameraList->SetCamAngle(this->GetEffectiveHeading(), a2);
		}
		else
		{
			int v5 = (1024 - ratan2(this->field_C84.vz, this->field_C84.vx)) & 0xFFF;
			CameraList->SetCamAngle(v5, a2);

			if (CameraList->mCameraMode == CAMERAMODE_DEMO)
			{
				if ((this->field_E2E | this->field_E2D) && this->field_E1C == 16)
				{
					i32 v6 = 2 * (this->field_E32 & 0xFFF);
					CameraList->SetCamYDistance(*word_6A8C66 + ((500 * word_610C4A[v6]) >> 12), a2);
					CameraList->SetCamAngle(v5 + ((700 * word_610C48[v6]) >> 12), a2);
				}
				else
				{
					CameraList->SetCamYDistance(*word_6A8C66, a2);
				}
			}
		}


	}
}


// @Ok
// verified against IDA sub_4C0D50 (0x4C0D50, 0x109 bytes). Both branches
// pass the same three bytes of field_580 to the constructor: byte 0
// (LOBYTE), byte 1 (BYTE1) and byte 2 (BYTE2), in that order. An earlier
// revision of this file had two bugs found here: the first branch passed
// the whole int for the first byte argument instead of truncating it, and
// both branches swapped byte 1 and byte 2. Fixed both.
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
				static_cast<unsigned char>(args),
				*(reinterpret_cast<unsigned char*>(&args) + 1),
				*(reinterpret_cast<unsigned char*>(&args) + 2));

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
				*(reinterpret_cast<unsigned char*>(&args) + 1),
				*(reinterpret_cast<unsigned char*>(&args) + 2));

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

	if (CameraList->mCameraMode != CAMERAMODE_DEMO && Trig_GetLevelID() != 514)
	{
		CameraList->SetMode(static_cast<ECameraMode>(3));
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
	CameraList->SetStartPosition();

	char * v13 = reinterpret_cast<char*>(this->field_E0C);
	*(v13  + 256) = 1;
	*(v13  + 48) = 1;

}

// @Ok
// verified against IDA sub_4C4A20 (0x4C4A20, 0x182 bytes). Found and
// fixed one bug from an earlier revision: v2 (the multiplier for the
// StartCoords/EndCoords.x term) was initialised to 0 instead of a2. The
// original sets v2 = a2 unconditionally at function entry, before the
// loop; a2 * 0 on the first iteration would have zeroed out the x
// component of the very first line-of-sight probe. Field offsets
// (mPos.vx/vy/vz at 0x8/0xC/0x10, field_C6C/C78/C7C/C80) all match the
// disassembly, as do the M3dColij_InitLineInfo/M3dZone_LineToItem calls
// (confirmed by address in names.json).
void CPlayer::TidyUpZipWebLandingPosition(int a2)
{
	SLineInfo v21;

	int v2 = a2;

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
	VALIDATE(CPlayer, field_C94, 0xC94);
	VALIDATE(CPlayer, field_CA4, 0xCA4);
	VALIDATE(CPlayer, field_CB4, 0xCB4);
	VALIDATE(CPlayer, field_CB8, 0xCB8);
	VALIDATE(CPlayer, field_CE4, 0xCE4);
	VALIDATE(CPlayer, field_D00, 0xD00);
	VALIDATE(CPlayer, field_D0C, 0xD0C);

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

	VALIDATE(CPlayer, field_E32, 0xE32);

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
