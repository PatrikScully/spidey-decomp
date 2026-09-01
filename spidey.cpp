#include "spidey.h"
#include "validate.h"
#include "mem.h"
#include "camera.h"
#include "screen.h"
#include "ps2funcs.h"
#include <cmath>
#include <cstring>
#include <new>
#include "ps2lowsfx.h"
#include "ps2redbook.h"
#include "utils.h"
#include "m3dutils.h"
#include "bit.h"
#include "bit2.h"
#include "effects.h"
#include "web.h"
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

// Relocatable user-function hook globals (0x6A9048/0x6A904C in the original).
// Set by Spidey_SetUserFunction, read by CPlayer::AI's per-tick callback loop.
static const char* gUserFunctionName;
static unsigned int gUserFunctionSize;

// 0x68293C: nonzero forces the level-exit path in CPlayer::AI's
// submariner-die check (see gPshellForceLevelExit's full comment lower down).
static i32 * const gPshellForceLevelExitEarly = (i32*)0x68293C;

extern u8 submarinerDieRelated;

// @Ok
void CPlayer::AI(void)
{
	// One-time floor-camera setup, gated on field_53C.
	if (this->field_53C == 0)
	{
		if (CameraList != 0)
		{
			if (CameraList->mCameraMode == 3)
			{
				CameraList->SetCamXOffset(gSpideyFloorCamXOffset, 0);
				CameraList->SetCamYOffset(gSpideyFloorCamYOffset, 0);
				CameraList->SetCamZOffset(gSpideyFloorCamZOffset, 0);
				CameraList->SetCamXZDistance(gSpideyFloorCamXZDistance, 0);
				CameraList->SetCamYDistance(gSpideyFloorCamYDistance, 0);
				this->field_540 = 0;
			}
			this->PutCameraBehind(0);
			CameraList->SetStartPosition();
			this->field_53C = 1;
		}
	}

	// Submariner-die check, gated on field_1AC and field_1A4.
	if (this->field_1AC != 0 && this->field_1A4 != 0)
	{
		if (*gPshellForceLevelExitEarly != 0)
		{
			if (this->field_E0C[0xE1] != 0)
			{
				this->field_E0C[0xE1] = 0;
				submarinerDieRelated = 1;
			}
		}
		else
		{
			if (DifficultyLevel != 0)
			{
				if (this->field_E0C[0x31] != 0 || this->field_E0C[0x21] != 0)
				{
					this->field_E0C[0x101] = 0;
					this->field_E0C[0x21] = 0;
					this->field_E0C[0x31] = 0;
					submarinerDieRelated = 1;
				}
			}
		}
	}

	// Walk the web list: delete flagged webs, AI() the rest.
	CBody *item = WebList;
	while (item != 0)
	{
		CBody *next = (CBody*)item->mNextItem;
		if (item->mCBodyFlags & 0x40)
		{
			if ((void*)this->field_E6C == (void*)item)
				this->field_E6C = 0;
			delete item;
		}
		else
		{
			item->AI();
		}
		item = next;
	}

	// field_C64 accumulator, clamped to [0, 0x1000].
	if (this->field_C5C != 0)
	{
		if (this->field_C68 != 0)
		{
			this->field_C64 -= this->field_80 * 64;
			if (this->field_C64 < 0)
				this->field_C64 = 0;
		}
		else if (this->field_C69 != 0)
		{
			this->field_C64 += this->field_80 * 64;
			if (this->field_C64 > 0x1000)
				this->field_C64 = 0x1000;
		}
	}

	// Three-value rolling shift, scaled by 1365.
	i32 old360 = this->field_360;
	i32 old364 = this->field_364;
	i32 sum = old360 + this->field_80 + old364;
	this->field_364 = old360;
	this->field_360 = this->field_80;
	this->field_368 = sum * 1365;

	// field_E18 countdown timer.
	if (this->field_E18 != 0 && this->field_1AC == 0)
	{
		this->field_E18 -= this->field_80;
		if (this->field_E18 < 0)
		{
			this->field_E18 = 0;
			this->mAnimSpeed = this->field_E12;
		}
	}

	// Zero the 16 i16-pair fields in the field_E0C struct.
	for (i32 i = 0; i < 16; i++)
	{
		*(i16*)((u8*)this->field_E0C + i * 0x10) = 0;
	}

	// Per-tick callback.
	if (this->field_554 != 0)
		this->field_554(this);

	// User-function hook loop.
	if (gUserFunctionName != 0)
	{
		for (i32 i = 0; i < 8; i++)
		{
			if (gUserFunctionSize & (1 << i))
				Reloc_CallUserFunction(gUserFunctionName, i, 0, 0);
		}
	}
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

// ProcessSFXArray (0x4C8F40): remembers the last random SFX id it played so
// the do/while loop re-rolls instead of replaying the same one. Sits 8 bytes
// after gSpideySenseListLastUpdateTime (0x6A9084).
static u32 * const gLastPlayedSfx = (u32*)0x006A908C;

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

// Builds the 8-point path for a tug web. Recovers the web's end position
// from the handle at field_E6C+0x134, bails if it is more than 0x80000 away
// vertically, then lays out 8 points on a shrinking spiral: each point sits
// at mPos + dir * (radius * cos) in XZ and mPos.y - 256*sin in Y, with the
// radius dropping by (radius-256)/7 for the first four points.
// @Ok
i32 *CPlayer::CalculateTugWebPathPoints(void)
{
	CVector webEnd = *(CVector *)((u8 *)Mem_RecoverPointer((SHandle *)((u8 *)this->field_E6C + 0x134)) + 8);

	i32 dy = webEnd.vy - this->mPos.vy;
	if (dy < 0)
		dy = -dy;
	if (dy > 0x80000)
		return 0;

	i32 *buffer = (i32 *)DCMem_New(0x60, 0, 1, 0, 1);
	i32 *path = buffer;

	CVector dir = webEnd;
	dir -= this->mPos;
	i32 length = dir.Length();
	dir >>= 12;
	dir.vy = 0;
	VectorNormal((VECTOR *)&dir, (VECTOR *)&dir);

	i32 step = (length - 256) / 7;
	i32 angle = 0;
	while (1)
	{
		i32 r = (rcossin_tbl[angle & 0xFFF].cos * length) >> 12;
		i32 px = this->mPos.vx + r * dir.vx;
		i32 pz = this->mPos.vz + r * dir.vz;
		if (angle < 1024)
			length -= step;
		i32 py = this->mPos.vy - (rcossin_tbl[angle & 0xFFF].sin << 8);
		*path++ = px;
		*path++ = py;
		*path++ = pz;
		angle += 256;
		if (angle >= 2048)
			break;
	}

	return buffer;
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

// @Ok
u8 CPlayer::CheckJumpingSmashKick(void)
{
	CBody *held = this->field_DCC;
	if (!held)
		return 0;
	u8 *pad = reinterpret_cast<u8*>(this->field_E0C);
	if (!pad[305] && !pad[289])
		return 0;
	if (this->field_8EA)
		return 0;
	i16 mType = held->mType;
	if (mType == 412)
		return 0;
	if (mType == 329)
		return 0;
	int shift = 12;
	CVector diff = held->mPos - this->mPos;
	CVector scaled = diff >> shift;
	VECTOR v;
	v.vx = scaled.vx;
	v.vy = scaled.vy;
	v.vz = scaled.vz;
	VectorNormal(&v, &v);
	if (v.vy <= 1024)
		return 0;
	// field at 0x54D (byte) and 0xEAC (held CSwinger*) live in PADDING regions;
	// access them by raw offset to keep the CPlayer layout unchanged.
	CSwinger **swingerSlot = reinterpret_cast<CSwinger**>(reinterpret_cast<u8*>(this) + 0xEAC);
	CSwinger *swinger = *swingerSlot;
	this->field_54C = 0;
	CameraList->field_12C = -1;
	reinterpret_cast<u8*>(this)[0x54D] = 0;
	if (swinger)
	{
		CSwinger_SwingBack(swinger);
		i32 *v8 = (i32*)swinger;
		(*(void(**)(i32*, i32))*v8)(v8, 1);
		*swingerSlot = 0;
	}
	this->field_8DC = 0;
	this->field_8CC = held->mPos;
	this->SetTargetTorsoAngleToThisPoint(&this->field_8CC);
	i32 saved = this->field_8C4;
	this->field_E1C = 0x1000000;
	this->field_8C8 = saved;
	this->field_8C4 = gTimerRelated;
	this->field_8D8 = 0;
	if (pad[289])
	{
		i32 *entry = gSpideySFXEntry[133];
		this->field_350 = entry;
		if (entry)
		{
			i32 i = *entry;
			while (i != -1)
			{
				*entry = (u16)i;
				i = entry[1];
				entry++;
			}
		}
		this->RunAnim(0x85, 0, -1);
	}
	else
	{
		i32 *entry = gSpideySFXEntry[129];
		this->field_350 = entry;
		if (entry)
		{
			i32 i = *entry;
			while (i != -1)
			{
				*entry = (u16)i;
				i = entry[1];
				entry++;
			}
		}
		this->RunAnim(0x81, 0, -1);
	}
	this->CreateJumpingSmashKickTrail();
	return 1;
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

// @Ok
// address 0x4C31D0, 932 bytes. Decompiled from IDA Hex-Rays output this
// session (previous sessions only had raw disassembly, which got stuck on
// interleaved register spills). Two calls in the original
// (CVector::operator/= applied to short-lived temporaries, divisors 2 and
// the field_DA0 length, at roughly the "half = len / 2" and
// "field_DA0.Length()" points) write into locals that are never read
// again afterwards (confirmed: the divisor-2 result is immediately
// shadowed by a plain `len / 2` on a different variable, and the
// field_DA0.Length() result is not read by anything downstream either).
// Both are omitted here as dead stores with no functional effect; a
// future byte-matching pass should reproduce them literally if needed.
u8 CPlayer::CheckSwingWebAvailability(SLineInfo *pLineInfo)
{
	i16 normalY = pLineInfo->Normal.vy;

	if (normalY > 3400)
		return 0;

	i32 distance = pLineInfo->Distance;

	if (distance <= 512 || distance >= 4096
			|| (pLineInfo->pFace[3] & 0x40000) != 0
			|| this->field_8E9 != 0
			|| (this->field_E1C & 0x4000F) == 0)
	{
		return 0;
	}

	this->field_DC0 = pLineInfo->Position;

	bool farFromWall = distance > 3072;
	CVector adjustedPos = this->mPos;

	if (this->field_E1C & 1)
	{
		CVector dir;
		dir.vx = (this->field_DC0.vx - this->mPos.vx) >> 12;
		dir.vy = 0;
		dir.vz = (this->field_DC0.vz - this->mPos.vz) >> 12;
		VectorNormal(reinterpret_cast<VECTOR*>(&dir), reinterpret_cast<VECTOR*>(&dir));

		adjustedPos.vy = this->mPos.vy - 2375680;
		adjustedPos += dir * 320;
	}

	CVector toLine = this->field_DC0 - adjustedPos;

	if (normalY >= -2600 && normalY <= 3400)
	{
		toLine += toLine / 32;
	}

	i32 len = toLine.Length();
	i32 vyOverLen = toLine.vy / len;
	i32 halfLen = len / 2;

	if (abs(vyOverLen) >= 2896)
		return 0;

	CVector shifted = toLine >> 8;
	i32 negX = -shifted.vx;

	this->field_DA0.vx = (shifted.vy * shifted.vx) >> 8;
	this->field_DA0.vy = (shifted.vx * negX - shifted.vz * shifted.vz) >> 8;
	this->field_DA0.vz = (shifted.vz * shifted.vy) >> 8;

	if (!farFromWall)
	{
		halfLen *= 2;

		CVector nearPoint = adjustedPos + toLine;
		nearPoint = nearPoint + this->field_DA0 * halfLen;

		this->field_D64 = nearPoint;
		this->field_D60 = 0;
	}
	else
	{
		halfLen *= 2;

		CVector scaled = this->field_DA0 * halfLen;
		CVector halfway = adjustedPos + toLine / 2;

		this->field_D64 = halfway + scaled;
		this->field_D70 = this->field_D64 + toLine;
		this->field_D60 = 1;
	}

	return 1;
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

// @Ok
void CPlayer::CreateCombatImpactEffect(CVector *pPos, i32 a3)
{
	CVector *v4 = pPos;

	if (this->field_5E8 != 0
		&& *(i32*)((char*)this + 0x5A4) != 0
		&& (*(u16*)((char*)this + 0x12A) == 100 || *(u16*)((char*)this + 0x12A) == 102
			|| *(u16*)((char*)this + 0x12A) == 104 || *(u16*)((char*)this + 0x12A) == 106))
	{
		i32 groundY = Web_GetGroundY(pPos);
		for (i32 i = 0; i < 10; i++)
			new CBouncingRock(pPos, groundY, 0x28000000);
	}

	i32 v6 = 0;

	if (a3 == 0)
	{
		v6 = 6;
		new CCombatImpactRing(v4, 0x6C, 0x12, 0x12, 384, 1536, 144);
		new CCombatImpactRing(v4, 0x90, 0x48, 0x48, 192, 768, 72);
	}
	else if (a3 == 1)
	{
		v6 = 12;
		new CCombatImpactRing(v4, 0x6C, 0x12, 0x12, 384, 1792, 144);
		new CCombatImpactRing(v4, 0x90, 0x48, 0x48, 192, 896, 72);
	}
	else if (a3 == 2)
	{
		v6 = 12;
		new CCombatImpactRing(v4, 0x6C, 0x19, 0x19, 512, 1792, 128);
		new CCombatImpactRing(v4, 0xA2, 0x65, 0x65, 256, 896, 64);
	}

	if (v6 > 0)
	{
		for (i32 i = 0; i < v6; i++)
		{
			i32 v11 = Rnd(3) + 8;
			CVector vel;
			vel.vx = v11 * (Rnd(4096) - 2048);
			vel.vy = v11 * (Rnd(4096) - 2048);
			vel.vz = v11 * (Rnd(4096) - 2048);

			CGLineParticle* pLine = new CGLineParticle(*pPos, vel, Rnd(7) + 12, 0);
			if (pLine)
			{
				pLine->SetRGB0(0xA0, 0, 0);
				pLine->SetRGB1(0x20, 0, 0);
				pLine->mCodeBGR0 |= 0x2000000;
			}
		}
	}
}

// Spits web drip particles from the player's hooks. Makes 2 drips per active
// hook when both hooks are active, else 4. Each drip is a CGLineParticle at
// the hook position with a random velocity opposite the player's velocity
// (field_C6C), coloured (96,96,96) fading to black.
// @Ok
void CPlayer::CreateWebDrips(bool a2, bool a3)
{
	i32 count = (a2 && a3) ? 2 : 4;

	CVector hookPos;
	CVector vel;

	i32 velX = this->field_C6C.vx;
	i32 velY = this->field_C6C.vy;
	i32 velZ = this->field_C6C.vz;

	if (a2)
	{
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&hookPos), this, 1);
		i32 n = count;
		while (n > 0)
		{
			vel.vx = -((Rnd(3) + 6) * velX);
			vel.vy = -((Rnd(3) + 6) * velY);
			vel.vz = -((Rnd(3) + 6) * velZ);
			CGLineParticle *line = new CGLineParticle(hookPos, vel, 30, 1);
			if (line)
			{
				line->SetRGB0(0x60, 0x60, 0x60);
				line->SetRGB1(0, 0, 0);
				line->mCodeBGR0 |= 0x2000000;
			}
			--n;
		}
	}

	if (a3)
	{
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&hookPos), this, 0);
		i32 n = count;
		while (n > 0)
		{
			vel.vx = -((Rnd(3) + 6) * velX);
			vel.vy = -((Rnd(3) + 6) * velY);
			vel.vz = -((Rnd(3) + 6) * velZ);
			CGLineParticle *line = new CGLineParticle(hookPos, vel, 30, 1);
			if (line)
			{
				line->SetRGB0(0x60, 0x60, 0x60);
				line->SetRGB1(0, 0, 0);
				line->mCodeBGR0 |= 0x2000000;
			}
			--n;
		}
	}
}

// @Ok
void CPlayer::DoMGSShadow(void)
{
	if (this->field_158 != 0)
	{
		// Four hook corners (0, 1, 5, 6) as a contiguous CVector[4].
		CVector hooks[4];
		i32 i;
		for (i = 0; i < 4; i++)
		{
			hooks[i].vx = 0;
			hooks[i].vy = 0;
			hooks[i].vz = 0;
		}
		M3dUtils_GetHookPosition((VECTOR*)&hooks[0], this, 0);
		M3dUtils_GetHookPosition((VECTOR*)&hooks[1], this, 1);
		M3dUtils_GetHookPosition((VECTOR*)&hooks[2], this, 5);
		M3dUtils_GetHookPosition((VECTOR*)&hooks[3], this, 6);

		CVector v20 = this->mShadowPos - this->mPos;
		for (i = 0; i < 4; i++)
			hooks[i] -= this->mPos;

		i32 maxX = 16;
		i32 maxZ = 16;
		i32 minX = -16;
		i32 minZ = -16;
		gte_SetRotMatrix(&this->field_89C);
		for (i = 0; i < 4; i++)
		{
			hooks[i] >>= 12;
			gte_ldlvl((VECTOR*)&hooks[i]);
			gte_rtir();
			gte_stlvnl((VECTOR*)&hooks[i]);
			if (hooks[i].vx <= maxX)
			{
				if (hooks[i].vx < minX)
					minX = hooks[i].vx;
			}
			else
			{
				maxX = hooks[i].vx;
			}
			if (hooks[i].vz <= maxZ)
			{
				if (hooks[i].vz < minZ)
					minZ = hooks[i].vz;
			}
			else
			{
				maxZ = hooks[i].vz;
			}
		}

		v20 >>= 12;
		gte_ldlvl((VECTOR*)&v20);
		gte_rtir();
		gte_stlvnl((VECTOR*)&v20);
		for (i = 0; i < 4; i++)
			hooks[i].vy = v20.vy;

		// Fold the rotated bounds into the four corners (a flat shadow quad).
		hooks[0].vx = minX;
		hooks[1].vx = minX;
		hooks[0].vz = maxZ;
		hooks[1].vz = minZ;
		hooks[2].vx = maxX;
		hooks[2].vz = maxZ;
		hooks[3].vx = maxX;
		hooks[3].vz = minZ;

		gte_SetRotMatrix(&this->mTransform);
		for (i = 0; i < 4; i++)
		{
			gte_ldlvl((VECTOR*)&hooks[i]);
			gte_rtir();
			gte_stlvnl((VECTOR*)&hooks[i]);
			hooks[i] <<= 12;
			hooks[i] += this->mPos;
		}

		if (this->field_AC0 == 0)
		{
			void *mem = CBit::operator new(132);
			CQuadBit *bit = 0;
			if (mem != 0)
				bit = ::new (mem) CQuadBit();
			this->field_AC0 = bit;
			bit->SetTexture(0, 0);
			bit->mFrigDeltaZ = 32;
			bit->SetSubtractiveTransparency();
		}

		i32 trans = (this->field_570 >> 3) + 32;
		if (this->field_8E8 != 0)
			trans >>= 1;
		this->field_AC0->SetTransparency(trans);
		this->field_AC0->SetCorners(hooks[0], hooks[1], hooks[2], hooks[3]);
		this->field_158 = 0;
	}
	else
	{
		if (this->field_AC0 != 0)
			delete this->field_AC0;
		this->field_AC0 = 0;
	}
}

// @Ok
i32 CPlayer::DoPhysics(void)
{
	if (!(this->field_E1C & 4))
		return 0;
	if (!(*((u8*)((char*)this + 0xE0)) & 1))
		return 0;
	if (!this->field_B74)
		return 0;

	i16 vy = this->field_B84.vy;
	if (vy > 0xD48 || vy < 0xF5D8)
		return 0;

	if (this->field_B8C && (this->field_B8C[3] & 0x40000))
		return 0;

	if (Utils_GetGroundHeight(&this->mPos, 0, 0xB4, 0) != -1)
		return 0;

	i16 nx = this->field_B84.vx;
	i16 ny = this->field_B84.vy;
	i16 nz = this->field_B84.vz;

	print_if_false(!((nx | ny | nz) & 0xFFFF), "Bad normal");

	this->field_A8.vx = nx;
	this->field_A8.vy = ny;
	this->field_A8.vz = nz;
	this->field_AD4 = 1;

	CVector normal;
	normal.vx = 0;
	normal.vy = 4096;
	normal.vz = 0;
	this->OrientToNormal(true, &normal);

	this->mVel.vx = 0;
	this->mVel.vy = 0;
	this->mVel.vz = 0;
	this->field_DF8 = 0;

	this->mPos.vx = this->field_A8.vx * this->field_EA8 + this->field_B78;
	this->mPos.vy = this->field_A8.vy * this->field_EA8 + this->field_B7C;
	this->mPos.vz = this->field_A8.vz * this->field_EA8 + this->field_B80;

	i32 **sfxEntry;
	i32 anim;
	if (this->mAnim == 0xE8)
	{
		sfxEntry = &gSpideySFXEntry[0xEA];
		anim = 0xEA;
	}
	else
	{
		sfxEntry = &gSpideySFXEntry[0xE3];
		anim = 0xE3;
	}

	this->field_350 = (i32*)sfxEntry;

	i32 **p = sfxEntry;
	while ((i32)*p != -1)
	{
		*p = (i32*)((i32)*p & 0xFFFF);
		p++;
	}

	this->RunAnim(anim, 0, -1);

	SFX_Play(9, 0x2000, 0);

	this->field_E1C = 1;

	return 1;
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

// gWideScreen (0x660F80): named in idb_globals.txt. Moved up here (from
// its original spot right before ExitLookaroundMode) because
// SetupLookaroundCamera, defined earlier in the file, needs it too.
static i32 * const gWideScreen = (i32*)0x660F80;
// dword_60F76C: falls inside gAnimWebcart (0x60F760, idb_globals.txt) at
// byte offset 0xC. Structure of gAnimWebcart is not known, so this is a
// tentative slot name only, not a real standalone global.
static i32 * const gAnimWebcart_field_C = (i32*)0x60F76C;

// raw accumulated yaw offset (relative to body heading) driven by look
// input each frame in CPlayer::SetupLookaroundCamera (0x4C38A0); used
// directly to drive the joint/head-turn and, added to GetEffectiveHeading,
// to aim SetTargetTorsoAngle on lock-on. No idb_globals.txt entry (nearest
// named neighbours are the gSpidey*Cam* tuning constants around
// 0x6A81xx-0x6A8Cxx, none at this address), tentative name only.
static i32 * const gLookaroundYawOffset = (i32*)0x6A7FFC;

// smoothed copy of gLookaroundYawOffset, chased with a max delta of 192
// per SetupLookaroundCamera call; feeds the camera-orientation matrix
// (as opposed to gLookaroundYawOffset itself, which feeds the raw
// raycast direction and the head/neck joints). No idb_globals.txt entry,
// tentative name.
static i32 * const gLookaroundYawSmoothed = (i32*)0x6A8D54;

// smoothed copy of gLookaroundActiveCamAngle (pitch), same 192/frame max
// delta as gLookaroundYawSmoothed; sits 4 bytes before the named
// gSpideySFXEntry table (0x6A82B8, idb_globals.txt) but is a distinct
// single i32, not part of that array. No idb_globals.txt entry, tentative
// name.
static i32 * const gLookaroundPitchSmoothed = (i32*)0x6A82B4;

// three anim-linked pose/SFX-trigger tables (same "walk id list, mask off
// high 16 bits, -1 terminated" idiom as gSpideySFXEntry, CLAUDE.md
// "Matching tricks"), selected by CPlayer::SetupLookaroundCamera right
// before RunAnim(0x104/0xFA/0x111, 0, -1) on a successful zip/swing-web
// lock-on. Each global itself holds a POINTER to the table (double
// indirection observed in the disasm: "mov eax, dword_6A86C8" then walks
// *eax). No idb_globals.txt entry for any of the three, tentative names
// only, guessed from which RunAnim call each precedes.
static i32 ** const gLookaroundZipHeldAnimTable = (i32**)0x6A86C8;
static i32 ** const gLookaroundZipAnimTable = (i32**)0x6A86A0;
static i32 ** const gLookaroundSwingAnimTable = (i32**)0x6A86FC;

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

// @Ok
// address 0x4B9390, 136 bytes, called directly from CPlayer::AI. Only
// special-cases zone 1797 (Trig_GetLevelId(), scaling the radius by
// this->mPos.vy while mCollision bit 2 is set) and zone 2052 (flat 1000);
// everything else returns this->field_EF8, the per-instance default. The
// two calls in the final else branch decompile as bare no-arg calls to
// nullsub_1, which is print_if_false compiled to a bare retn in the
// shipped binary (see CLAUDE.md / SetSpideyLookaroundCamValue above for
// the same idiom); exact message strings are not recoverable from the
// stripped binary, placeholders used here.
i32 CPlayer::GetPerpendicularisationRadius(void)
{
	i32 level = Trig_GetLevelId();

	if (level == 1797)
	{
		if (this->mCollision & 2)
		{
			i32 heightY = this->mPos.vy;

			if (heightY <= -2744320)
			{
				if (heightY <= -5324800)
					return heightY <= -7782400 ? 1400 : 1024;

				return 800;
			}
		}

		return this->field_EF8;
	}

	if (level == 2052)
		return 1000;

	print_if_false(0, "Unknown level");
	print_if_false(0, "Unknown level");
	return 0;
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

// Installs the nine hard-coded SFX trigger lists (the ones that are not
// pulled out of the animation data) into gSpideySFXEntry, then clears the
// "already played" marker (the high word, see ProcessSFXArray) on every
// element of every list in the table. Same clearing loop as
// ResetSFXArrayEntry, run over all 300 slots.
// @Ok
void CPlayer::InitialiseSFXArray(void)
{
	// The nine built-in trigger lists, each a -1 terminated list of frame
	// numbers. They stay at their original addresses because the game code
	// that has not been hooked yet mutates the very same bytes.
	static i32 * const gSfxListAnim21 = (i32*)0x005565A8;  // { 1, 11, -1 }
	static i32 * const gSfxListAnim59 = (i32*)0x005565B4;  // { 4, -1 }
	static i32 * const gSfxListAnim52 = (i32*)0x005565BC;  // { 7, 15, -1 }
	static i32 * const gSfxListAnim50 = (i32*)0x005565C8;  // { 6, -1 }
	static i32 * const gSfxListAnim51 = (i32*)0x005565D0;  // { 6, -1 }
	static i32 * const gSfxListAnim60 = (i32*)0x005565D8;  // { 6, 15, -1 }
	static i32 * const gSfxListAnim63 = (i32*)0x005565E4;  // { 7, 19, 26, -1 }
	static i32 * const gSfxListAnim192 = (i32*)0x005565F4; // { 2, 14, -1 }
	static i32 * const gSfxListAnim198 = (i32*)0x00556600; // { 1, 21, -1 }

	gSpideySFXEntry[21] = gSfxListAnim21;
	gSpideySFXEntry[59] = gSfxListAnim59;
	gSpideySFXEntry[52] = gSfxListAnim52;
	gSpideySFXEntry[50] = gSfxListAnim50;
	gSpideySFXEntry[51] = gSfxListAnim51;
	gSpideySFXEntry[60] = gSfxListAnim60;
	gSpideySFXEntry[63] = gSfxListAnim63;
	gSpideySFXEntry[192] = gSfxListAnim192;
	gSpideySFXEntry[198] = gSfxListAnim198;

	for (i32 i = 0; i < 300; i++)
	{
		i32 *pEntry = gSpideySFXEntry[i];

		if (pEntry)
		{
			while (*pEntry != -1)
			{
				*pEntry = *pEntry & 0xFFFF;
				pEntry++;
			}
		}
	}
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

// One-time parse of the static fight data. Sorts the follow-on and fists
// tables into animation order, then builds the two id-indexed lookup
// arrays the combo code uses: the distance definitions, from the blob at
// 0x569194, and the move records, from the blob at 0x56929C. Each move
// record also gets its variable-length tail resolved (a pointer to the byte
// streams that follow the parts array, plus the length of the second
// stream), and once every record is known each parts entry's move id is
// rewritten in place into the pointer to that move's record. Both blobs use
// the id 6666 as their terminator.
// @Ok
void CPlayer::ParseFightData(void)
{
	// set the first time the data is parsed, makes every later call a no-op.
	static u8 * const gFightDataParsed = (u8*)0x006A9088;

	// distance id -> pointer into the distance blob, 300 slots.
	static u8 ** const gDistanceDefs = (u8**)0x006A8768;

	// the distance blob: u16 id, then a 0xFF terminated byte stream, each
	// record dword aligned.
	static u16 * const gDistanceData = (u16*)0x00569194;

	// move id -> pointer to a move record, 32 slots.
	static u8 ** const gComboMoves = (u8**)0x006A8CB4;

	// the move blob. A record is {u16 id, u16 anim, u8 *tail, ...,
	// u16 tailLength at 0x1E, u16 partCount at 0x20, part[partCount] of 12
	// bytes from 0x24}, followed by three 0xFF terminated byte streams.
	static u8 * const gComboMoveData = (u8*)0x0056929C;

	if (*gFightDataParsed)
		return;

	*gFightDataParsed = 1;

	this->SortAnimationFollowOnData();
	this->SortFistsData();

	memset(gDistanceDefs, 0, 300 * sizeof(u8*));

	u16 id = gDistanceData[0];
	u8 *pData = reinterpret_cast<u8*>(gDistanceData + 1);

	while (id != 6666)
	{
		print_if_false(gDistanceDefs[id] == 0, "Possible duplicate distance definition");

		gDistanceDefs[id] = pData;

		while (*pData != 0xFF)
			pData++;

		pData = reinterpret_cast<u8*>((reinterpret_cast<u32>(pData) + 4) & 0xFFFFFFFC);

		id = *reinterpret_cast<u16*>(pData);
		pData += 2;
	}

	memset(gComboMoves, 0, 32 * sizeof(u8*));

	u8 *pMove = gComboMoveData;

	for (;;)
	{
		u16 moveId = *reinterpret_cast<u16*>(pMove);

		print_if_false(gComboMoves[moveId] == 0, "Possible duplicate move");

		u16 partCount = *reinterpret_cast<u16*>(pMove + 0x20);

		gComboMoves[moveId] = pMove;

		u8 *pTail = pMove + (partCount + 3) * 12;
		*reinterpret_cast<u8**>(pMove + 4) = pTail;

		while (*pTail++ != 0xFF)
			;

		u8 length = 0;

		while (*pTail++ != 0xFF)
			length++;

		*reinterpret_cast<u16*>(pMove + 0x1E) = length;

		while (*pTail++ != 0xFF)
			;

		pTail = reinterpret_cast<u8*>((reinterpret_cast<u32>(pTail) + 3) & 0xFFFFFFFC);

		if (*reinterpret_cast<u32*>(pTail) == 6666)
			break;

		pMove = pTail;
	}

	for (i32 i = 0; i < 32; i++)
	{
		u8 *pRecord = gComboMoves[i];

		if (pRecord)
		{
			i32 partCount = *reinterpret_cast<u16*>(pRecord + 0x20);

			for (i32 part = 0; part < partCount; part++)
			{
				u32 *pRef = reinterpret_cast<u32*>(pRecord + 0x24 + part * 12);

				print_if_false(gComboMoves[*pRef] != 0, "Bad move reference");

				*pRef = reinterpret_cast<u32>(gComboMoves[*pRef]);
			}
		}
	}
}

// Walks the per-anim SFX trigger array (field_350). Each 32-bit element
// packs its trigger frame in the low word; once mFrame reaches it the
// matching SFX is played (picked by mAnim) and the element's high word is
// set to 0xFFFF so it is not replayed. A -1 sentinel ends the array and
// clears field_350. Returns the (advanced) array pointer, which the single
// caller (SpideyAI0) ignores.
// @Ok
i32 CPlayer::ProcessSFXArray(void)
{
	i32 *p = this->field_350;

	if (p == 0)
		return 0;

	i32 elem = *p;
	if (elem == -1)
	{
		this->field_350 = 0;
		return (i32)p;
	}

	if (this->mFrame < elem)
		return this->mFrame;

	switch (this->mAnim)
	{
		case 0x15:
		case 0xC0:
		case 0xC6:
		{
			i32 sfx;
			do
			{
				if (this->field_34C != 0)
					sfx = (Rnd(4) + 80) | 0x8000;
				else
					sfx = Rnd(4) + 1;
			}
			while (sfx == *gLastPlayedSfx);
			SFX_Play(sfx, 0x2000, 0);
			*gLastPlayedSfx = sfx;
			break;
		}
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x3C:
		case 0x3F:
			SFX_Play(Rnd(2) + 5, 0x2000, 0);
			break;
		case 0x3B:
			SFX_Play(9, 0x2000, 0);
			break;
		default:
			break;
	}

	*p = elem | 0xFFFF0000;
	this->field_350 = p + 1;
	return (i32)(p + 1);
}

// Reads the player's movement input for this tick from the pad (field_E0C
// = gSControl) into field_E2D (move forward/back) and field_E2E (move
// left/right), falling back to the digital D-pad when the analogue stick is
// centred. While a target is locked (field_E18 != 0) and field_1AC is clear
// the input is zeroed instead of read. After reading it applies the aim
// correction (field_EA0/field_EA2), the one-shot "just centred" flags
// (field_AD8/field_AD9), the input-inversion options (field_8E8/field_8E9)
// and updates the aim angle field_E32 from field_E34 via ratan2.
// @Ok
void CPlayer::ReadAnalogueInput(void)
{
	u8 *pad = reinterpret_cast<u8*>(this->field_E0C);

	this->field_E2F = this->field_E2D;
	this->field_E30 = this->field_E2E;
	this->field_E2D = 0;
	this->field_E2E = 0;

	if (this->field_E18 == 0)
	{
		if (this->field_1AC == 0)
		{
			// SControl::Type (pad+0x16C) is 0x73 for the pad that reports
			// the analogue move values in pad+0x168/0x169.
			if (*reinterpret_cast<i32*>(pad + 0x16C) == 0x73)
			{
				this->field_E2D = pad[0x168]; // AnalogueMoveForwardsBackwards
				this->field_E2E = pad[0x169]; // AnalogueMoveLeftRight
			}
			if (this->field_E2D == 0 && this->field_E2E == 0)
			{
				if (pad[0xA0])                 // Up.Pressed
					this->field_E2D = -127;
				else if (pad[0xB0])            // Down.Pressed
					this->field_E2D = 127;
				if (pad[0x90])                 // Right.Pressed
					this->field_E2E = 127;
				else if (pad[0x80])            // Left.Pressed
					this->field_E2E = -127;
			}
		}
		else
		{
			this->SynthesizeAnalogueInput();
		}
	}
	else
	{
		if (this->field_1AC == 0)
		{
			this->field_8F0 = 0;
			this->field_AD8 = 0;
			this->field_AD9 = 0;
			pad[0x40] = 0;                     // LeftOne.Pressed
			return;
		}
		this->SynthesizeAnalogueInput();
	}

	if (this->field_EA0 != 0)
	{
		this->field_E2D -= (char)(this->field_EA0 * (char)this->field_E2D / this->field_EA2);
		this->field_E2E -= (char)(this->field_EA0 * (char)this->field_E2E / this->field_EA2);
	}

	if (this->field_AD8 != 0 && this->field_E2D == 0 && this->field_E2E == 0)
	{
		this->field_211 = 1;
		this->field_AD8 = 0;
		pad[0x51] = 1;                         // LeftTwo.Triggered
	}
	else if (this->field_AD9 != 0 && this->field_E2D == 0 && this->field_E2E == 0)
	{
		this->field_AD9 = 0;
	}

	if (this->field_8E9 != 0 && this->field_AD8 != 0)
		this->field_E2D = -this->field_E2D;
	else if (this->field_8E8 != 0 && this->field_AD9 != 0)
		this->field_E2D = -this->field_E2D;

	if (this->field_E2D != 0 || this->field_E2E != 0)
	{
		this->field_8F0 += 32;
		if (this->field_8F0 > 256)
			this->field_8F0 = 256;
		this->field_E32 = (i16)((this->field_E34 - ratan2(-this->field_E2D, this->field_E2E) + 1024) & 0xFFF);
	}
	else
	{
		this->field_8F0 = 0;
		this->field_E32 = this->field_E34;
	}
}

// @Ok
// 0x4C5AA0. Manages the dedicated auto-aim target CBody (field_878). Clears
// field_E04/06/08 and field_DD0 each pass. If field_8EA is set, deletes the
// existing target. Otherwise selects a target via SelectTargetBaddy; on a hit
// it creates field_878 (once), sets field_DCC, and repositions field_878 at
// the target with a height offset (the u16 at CBody+0xF4 << 12) plus a Y-angle
// nudge from field_80. On a miss it deletes the existing target.
u8 CPlayer::SelectAutoAimTarget(void)
{
	static u8 * const gRegionByte = (u8*)0x6B4678; // same byte as gM3dRegionTwo (ps2m3d.cpp)

	this->field_E04 = 0;
	this->field_E06 = 0;
	this->field_E08 = 0;
	this->field_DD0 = 0;

	if (this->field_8EA != 0)
	{
		if (this->field_878 != 0)
		{
			this->field_878->DeleteFrom((CBody**)&MiscellaneousRenderingList);
			delete this->field_878;
			this->field_878 = 0;
		}
		return this->field_8EA;
	}

	this->field_DCC = 0;
	CBody *target = this->SelectTargetBaddy(0x800, 0xB50, 0x1000, 0x1000);
	if (target != 0)
	{
		this->field_DCC = target;
		i32 vx = target->mPos.vx;
		i32 vy = target->mPos.vy - (*(u16*)((char*)target + 0xF4) << 12);
		i32 vz = target->mPos.vz;
		if (this->field_878 == 0)
		{
			CBody *v6 = new CBody();
			this->field_878 = v6;
			v6->InitItem("items");
			v6->mModel = Spool_GetModel(0xB08EC1FB, *gRegionByte);
			v6->mType = 503;
			*((u16*)((char*)v6 + 0xDC)) = 100;
			v6->AttachTo((CBody**)&MiscellaneousRenderingList);
		}
		CBody *v6 = this->field_878;
		v6->mPos.vx = vx;
		v6->mPos.vy = vy;
		v6->mPos.vz = vz;
		v6->mAngles.vy += 16 * (u16)this->field_80;
	}
	else
	{
		if (this->field_878 != 0)
		{
			this->field_878->DeleteFrom((CBody**)&MiscellaneousRenderingList);
			delete this->field_878;
			this->field_878 = 0;
		}
	}
	return this->field_8EA;
}

// @Ok
// 0x4C8410. Scores every baddy on BaddyList by proximity (distWeight) plus a
// view-cone bonus (coneWeight) for baddies in front of the camera matrix at
// field_89C, and returns the highest-scoring one that has clear line of
// sight. mRMinor (0xDC) == 0, the 0x40 flag set, or the 0x10 flag clear
// disqualify a baddy; mPlayerDist >= maxDist does too.
CBody *CPlayer::SelectTargetBaddy(i32 maxDist, i32 coneThreshold, i32 distWeight, i32 coneWeight)
{
	CBody *best = 0;
	i32 bestScore = 0;

	for (CBody *baddy = BaddyList; baddy; baddy = (CBody*)baddy->mNextItem)
	{
		if (baddy->mRMinor == 0 || (baddy->mCBodyFlags & 0x40) || !(baddy->mCBodyFlags & 0x10))
			continue;
		if (baddy->mPlayerDist >= maxDist)
			continue;

		i32 score = (distWeight * (((maxDist - baddy->mPlayerDist) << 12) / maxDist)) >> 12;

		if (coneWeight != 0)
		{
			VECTOR dir;

			dir.vx = (baddy->mPos.vx - this->mPos.vx) >> 12;
			dir.vy = (baddy->mPos.vy - this->mPos.vy) >> 12;
			dir.vz = (baddy->mPos.vz - this->mPos.vz) >> 12;
			gte_SetRotMatrix(&this->field_89C);
			gte_ldlvl(&dir);
			gte_rtir();
			gte_stlvnl(&dir);
			dir.vy = 0;
			VectorNormal(&dir, &dir);
			if (-dir.vz >= coneThreshold)
				score += (coneWeight * ((4096 - dir.vz) / 2)) >> 12;
		}

		if (score > bestScore && Utils_LineOfSight(&this->mPos, &baddy->mPos, 0, 0) != 0)
		{
			bestScore = score;
			best = baddy;
		}
	}

	return best;
}

// @Ok
// address found and verified this session: IDA sub_4C8570 (0x4C8570,
// 0x253 = 595 bytes, matches the size noted below). cmpsum confirms the
// documented 111 mnemonic diffs.
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

// @Ok
// verified against IDA sub_4C38A0 (0x4C38A0, 3674 bytes) via decompile()
// cross-checked with disasm() for every field offset and call site (the
// Switch_GetCSwitchObjectFromItem/CheckSwingWebAvailability call site in
// particular: Hex-Rays showed a bogus 3-arg call, the real disasm is a
// plain thiscall(SLineInfo*) matching the header exactly). Seven new
// CPlayer fields carved out of existing PADDING blocks this session:
// field_54F, field_558, field_8ED, field_CD4, field_CE8, field_CF4,
// field_DAC (see their comments in spidey.h for offset evidence). Six new
// tentative globals (gLookaroundYawOffset/YawSmoothed/PitchSmoothed,
// gLookaroundZipHeldAnimTable/ZipAnimTable/SwingAnimTable) declared above
// alongside the existing gLookaroundActiveCamAngle/HeadingSnapshot; none
// have an idb_globals.txt entry. This is a per-frame update: heading
// tracking with wraparound, a 24-frame entrance/exit blend of a
// precomputed CQuat path (field_C90, built by EnterLookaroundMode) versus
// a freshly raycast anchor point, a small raycast grid-search to find a
// wall-clear camera anchor, then (only once neither transition is
// active) a target raycast that dispatches into switch/zip-web/swing-web
// lock-on or a default baddy-target reticle. Two real bugs were caught
// against the disasm while writing this: the CheckSwingWebAvailability
// call must bind to the (i16, bool) overload (SetTargetTorsoAngle has
// the same overload-resolution hazard, "false" not "0" is used at both
// call sites here to force it) since the (i16, int) overload is still an
// unimplemented stub at this address; and gLookaroundActiveCamAngle
// reads must NOT be treated as i16 without the intermediate i32 (the
// original keeps it as a full DWORD end to end except at the very final
// store into the joint array / CSVector angle).
void CPlayer::SetupLookaroundCamera(void)
{
	if (!this->field_8EA)
		return;

	Screen_TargetOn(false);

	MATRIX camMat;
	i32 elapsed;
	i32 dt = this->field_80;

	i32 entranceRemaining = this->field_CB4;

	if (entranceRemaining > dt)
	{
		CQuat* path = reinterpret_cast<CQuat*>(this->field_C90);

		entranceRemaining -= dt;
		this->field_CB4 = entranceRemaining;
		elapsed = 24 - entranceRemaining;
		this->field_DF8 = 0;

		QToM(&path[elapsed], &camMat);

		*gWideScreen = 32 * elapsed / 24;
		*gAnimWebcart_field_C = 32 * elapsed / 24;
	}
	else
	{
		i32 exitRemaining = this->field_CE4;
		this->field_CB4 = 0;

		if (exitRemaining > dt)
		{
			CQuat* path = reinterpret_cast<CQuat*>(this->field_C90);

			exitRemaining -= dt;
			this->field_CE4 = exitRemaining;
			elapsed = 24 - exitRemaining;

			QToM(&path[elapsed], &camMat);

			*gWideScreen = 32 * elapsed / 24;
			*gAnimWebcart_field_C = 32 * elapsed / 24;
		}
		else if (exitRemaining != 0)
		{
			// last tick of the exit fade: tear the lookaround state down
			// entirely (mirrors ExitLookaroundMode, minus the camera
			// pop/PutCameraBehind/RenderReticle bookkeeping that function
			// also does, matching the disassembly exactly).
			i16* joints = reinterpret_cast<i16*>(this->mpJoints);

			this->field_CE4 = 0;
			this->field_8EA = 0;

			if (joints)
			{
				joints[6] = 0;
				joints[7] = 0;
				joints[18] = 0;
				joints[19] = 0;
			}

			i32 path = this->field_C90;
			*gWideScreen = 0;
			*gAnimWebcart_field_C = 0;

			if (path)
			{
				Mem_Delete(reinterpret_cast<void*>(path));
				this->field_C90 = 0;
			}

			return;
		}
		else
		{
			// steady state: neither entering nor exiting.
			camMat = this->mTransform;
		}
	}

	// --- heading tracking -------------------------------------------
	i16 heading = this->GetEffectiveHeading();
	i16 headingDelta = heading - *gLookaroundHeadingSnapshot;
	if (headingDelta > 512)
		headingDelta -= 4096;
	else if (headingDelta < -512)
		headingDelta += 4096;
	*gLookaroundHeadingSnapshot = heading;

	if (this->field_CB4 == 0)
	{
		if (this->field_8E9)
		{
			*gLookaroundYawOffset += headingDelta;
			*gLookaroundYawSmoothed += headingDelta;
		}
		else
		{
			*gLookaroundYawOffset -= headingDelta;
			*gLookaroundYawSmoothed -= headingDelta;
		}
	}

	if (this->field_CE4 != 0)
	{
		// exiting: let the head/neck joint rotation decay back to centre.
		i16* joints = reinterpret_cast<i16*>(this->mpJoints);
		if (joints)
		{
			joints[6] -= joints[6] / 4;
			joints[7] -= joints[7] / 4;
			joints[18] -= joints[18] / 4;
			joints[19] -= joints[19] / 4;
		}
	}
	else
	{
		i16* joints = reinterpret_cast<i16*>(this->mpJoints);
		if (joints)
		{
			i16 pitch = static_cast<i16>((2 * *gLookaroundActiveCamAngle / 4) & 0xFFF);
			i16 yaw = static_cast<i16>((2 * *gLookaroundYawOffset / 4) & 0xFFF);
			joints[6] = pitch;
			joints[7] = yaw;
			joints[18] = pitch;
			joints[19] = yaw;
		}
	}

	CSVector rawAngles(static_cast<i16>(*gLookaroundActiveCamAngle), static_cast<i16>(*gLookaroundYawOffset), 0);
	MATRIX rawLookMat;
	M3dMaths_RotMatrixYXZ(reinterpret_cast<SVECTOR*>(&rawAngles), &rawLookMat);

	// smoothed pitch/yaw chase the raw values with a max step of 192/call.
	i32 pitchTarget = *gLookaroundActiveCamAngle;
	i32 smoothPitch = *gLookaroundPitchSmoothed;
	if (pitchTarget > smoothPitch + 192)
		smoothPitch = pitchTarget - 192;
	else if (pitchTarget < smoothPitch - 192)
		smoothPitch = pitchTarget + 192;
	*gLookaroundPitchSmoothed = smoothPitch;

	i32 yawTarget = *gLookaroundYawOffset;
	i32 smoothYaw = *gLookaroundYawSmoothed;
	if (yawTarget > smoothYaw + 192)
		smoothYaw = yawTarget - 192;
	else if (yawTarget < smoothYaw - 192)
		smoothYaw = yawTarget + 192;
	*gLookaroundYawSmoothed = smoothYaw;

	if (this->field_CE4 == 0)
	{
		CSVector smoothedAngles(static_cast<i16>(smoothPitch), static_cast<i16>(smoothYaw), 0);
		MATRIX smoothedLookMat;
		M3dMaths_RotMatrixYXZ(reinterpret_cast<SVECTOR*>(&smoothedAngles), &smoothedLookMat);
		MulMatrix(&camMat, &smoothedLookMat);
		MToQ(camMat, this->field_CD4);
	}

	// flip 180 degrees about Y (negate the X and Z columns), same idiom
	// as EnterLookaroundMode's camera-orientation flip.
	camMat.m[0][0] = -camMat.m[0][0];
	camMat.m[1][0] = -camMat.m[1][0];
	camMat.m[2][0] = -camMat.m[2][0];
	camMat.m[0][2] = -camMat.m[0][2];
	camMat.m[1][2] = -camMat.m[1][2];
	camMat.m[2][2] = -camMat.m[2][2];

	MToQ(camMat, CameraList->field_1F4);

	CVector anchor = this->mPos + this->field_D0C;
	this->field_D00 = anchor;

	// line-of-sight adjustment: pull the anchor back towards mPos if
	// something is in the way between the player and it.
	SLineInfo losAdjustInfo;
	losAdjustInfo.StartCoords = this->mPos;
	losAdjustInfo.EndCoords = anchor;
	M3dColij_InitLineInfo(&losAdjustInfo);
	M3dZone_LineToItem(&losAdjustInfo, 1);

	if (losAdjustInfo.pItem != 0)
	{
		if (losAdjustInfo.Distance >= 16)
		{
			i32 frac = 3 * losAdjustInfo.Distance / 4;
			anchor = this->mPos + (this->field_D0C * frac) / losAdjustInfo.Length;
		}
		else
		{
			anchor = this->mPos;
		}
	}

	// candidate grid-search target: during a transition, blend towards
	// it from the saved snapshot; in steady state, just place it in
	// front of/behind the anchor along the working matrix's Z column.
	CVector campPos;

	if (this->field_CB4 != 0)
	{
		CVector bodyForward(this->mTransform.m[0][2], this->mTransform.m[1][2], this->mTransform.m[2][2]);
		CVector aheadPoint = anchor + bodyForward * 512;
		CVector diff = aheadPoint - this->field_CB8;
		CVector scaled = diff * (24 - this->field_CB4);
		CVector blended = scaled / 24;
		campPos = this->field_CB8 + blended;
	}
	else if (this->field_CE4 != 0)
	{
		CVector diff = this->field_CF4 - this->field_CE8;
		CVector scaled = diff * (24 - this->field_CE4);
		CVector blended = scaled / 24;
		campPos = this->field_CE8 + blended;
	}
	else
	{
		CVector forward(camMat.m[0][2], camMat.m[1][2], camMat.m[2][2]);
		campPos = anchor - forward * 512;
	}

	SLineInfo gridInfo;
	gridInfo.StartCoords = anchor;
	gridInfo.EndCoords = campPos;
	M3dColij_InitLineInfo(&gridInfo);
	M3dZone_LineToItem(&gridInfo, 1);

	i32 stepsLeft = gridInfo.Length;

	if (gridInfo.pItem != 0)
	{
		campPos.vx = gridInfo.Position.vx + 16 * camMat.m[0][2];
		campPos.vy = gridInfo.Position.vy + 16 * camMat.m[1][2];
		campPos.vz = gridInfo.Position.vz + 16 * camMat.m[2][2];
		stepsLeft -= 16;
	}

	// small raycast grid-search: walk forward along the camera Z axis in
	// steps of 8, at each step testing lateral clearance (+16 then -16
	// along the X axis) until a clear spot is found or steps run out.
	CVector col0(camMat.m[0][0], camMat.m[1][0], camMat.m[2][0]);
	CVector col2(camMat.m[0][2], camMat.m[1][2], camMat.m[2][2]);

gridSearchTop:
	gridInfo.StartCoords = campPos;
	gridInfo.EndCoords = campPos + col0 * 16;
	M3dColij_InitLineInfo(&gridInfo);
	M3dZone_LineToItem(&gridInfo, 1);

	if (gridInfo.pItem != 0)
	{
		stepsLeft -= 8;
		if (stepsLeft <= 16)
			goto gridSearchDone;

		campPos = campPos + col2 * 8;
		goto gridSearchTop;
	}

	gridInfo.EndCoords = gridInfo.EndCoords - col0 * 32;
	M3dColij_InitLineInfo(&gridInfo);
	M3dZone_LineToItem(&gridInfo, 1);
	if (gridInfo.pItem != 0)
	{
		stepsLeft -= 8;
		if (stepsLeft > 16)
		{
			campPos = campPos + col2 * 8;
			goto gridSearchTop;
		}
	}

gridSearchDone:
	if (this->field_CE4 == 0)
	{
		this->field_CE8 = campPos;
	}

	CameraList->mPos = campPos;

	if (this->field_CB4 != 0 || this->field_CE4 != 0)
	{
		// still transitioning: no target selection this frame.
		return;
	}

	// --- steady state: pick a lookaround target -----------------------
	MATRIX bodyLookMat = this->mTransform;
	this->field_DE4 = 1;
	MulMatrix(&bodyLookMat, &rawLookMat);

	CVector farPoint;
	farPoint.vx = anchor.vx - (bodyLookMat.m[0][2] << 12);
	farPoint.vy = anchor.vy - (bodyLookMat.m[1][2] << 12);
	farPoint.vz = anchor.vz - (bodyLookMat.m[2][2] << 12);

	this->field_DCC = 0;
	CVector hitPos;
	CBody* sphereHit = M3dColij_LineToSphere(&anchor, &farPoint, &hitPos, BaddyList, 0, 4096);
	this->field_DCC = sphereHit;

	if (sphereHit != 0)
	{
		farPoint = hitPos;
		if ((sphereHit->mCBodyFlags & 0x10) == 0)
			this->field_DCC = 0;
	}

	SLineInfo targetInfo;
	targetInfo.StartCoords = anchor;
	targetInfo.EndCoords = farPoint;

	LineOfSightCheck = 1;
	M3dColij_InitLineInfo(&targetInfo);
	M3dZone_LineToItem(&targetInfo, 1);
	LineOfSightCheck = 0;

	if (targetInfo.pItem != 0)
	{
		this->field_DA0.vx = targetInfo.Normal.vx;
		this->field_DA0.vy = targetInfo.Normal.vy;
		this->field_DA0.vz = targetInfo.Normal.vz;
		this->field_DC0 = targetInfo.Position;
		this->field_DCC = 0;
		this->field_DE8 = 0x202080;

		if ((targetInfo.pFace[3] & 0x2000000) != 0)
		{
			CSwitch* sw = Switch_GetCSwitchObjectFromItem(targetInfo.pItem);
			if (sw->field_100 != 0)
			{
				this->field_DCC = sw;
				this->field_DE8 = 0x802020;
				this->field_54F = 0;
				return;
			}
		}

		u8 zipOk = this->CheckZipWebAvailability(&targetInfo, 0x800);
		u8 swingOk;

		if (zipOk)
		{
			this->field_DE8 = 0x208020;
			swingOk = this->CheckSwingWebAvailability(&targetInfo);

			if (!swingOk && this->field_54F != 0 && this->field_E1C == 1)
			{
				// zip-web lock-on.
				this->field_558 = this->mPos;
				this->field_E1C = 0x40000;
				this->field_8ED = 1;

				if (this->field_AD4)
				{
					i32* table = *gLookaroundZipHeldAnimTable;
					this->field_350 = table;
					if (table)
					{
						i32 v = table[0];
						if (v != -1)
						{
							i32* p = table;
							do { *p = static_cast<u16>(v); v = p[1]; ++p; } while (v != -1);
						}
					}
					this->RunAnim(0x104, 0, -1);
				}
				else
				{
					i32* table = *gLookaroundZipAnimTable;
					this->field_350 = table;
					if (table)
					{
						i32 v = table[0];
						if (v != -1)
						{
							i32* p = table;
							do { *p = static_cast<u16>(v); v = p[1]; ++p; } while (v != -1);
						}
					}
					this->RunAnim(0xFA, 0, -1);
				}

				this->SetTargetTorsoAngle(this->GetEffectiveHeading() + *gLookaroundYawOffset, false);
				this->ExitLookaroundMode();
				this->field_54F = 0;
				return;
			}
		}
		else
		{
			swingOk = this->CheckSwingWebAvailability(&targetInfo);
		}

		if (swingOk)
		{
			this->field_DE8 = 0x208020;

			if (this->field_54F != 0 && this->field_E1C == 1)
			{
				// swing-web lock-on.
				print_if_false(this->field_E64 == 0, "Error");

				i32* table = *gLookaroundSwingAnimTable;
				this->field_E1C = 0x100;
				this->field_8ED = 1;
				this->field_350 = table;
				if (table)
				{
					i32 v = table[0];
					if (v != -1)
					{
						i32* p = table;
						do { *p = static_cast<u16>(v); v = p[1]; ++p; } while (v != -1);
					}
				}
				this->RunAnim(0x111, 0, -1);

				if (!this->field_AD4)
				{
					this->SetTargetTorsoAngle(this->GetEffectiveHeading() + *gLookaroundYawOffset, false);
					this->field_DAC = ZeroVector;
				}
				else
				{
					this->field_DAC = targetInfo.Position;
				}

				this->field_AD4 = 0;
				this->ExitLookaroundMode();
				this->field_54F = 0;
				return;
			}
		}
	}
	else
	{
		// nothing zip/swing-eligible on the target ray: fall back to the
		// baddy found by the earlier sphere-cast, if any.
		this->field_DC0 = targetInfo.EndCoords;

		if (this->field_DCC != 0)
		{
			this->field_DE4 = 0;
			Screen_TargetOn(true);
			Screen_SetTarget(&this->field_DCC->mPos, 24, 32 * (gTimerRelated & 0x7F));
			this->field_DC0 = this->field_DCC->mPos;
			this->field_54F = 0;
			return;
		}

		this->field_DE8 = 0x202080;
	}

	this->field_54F = 0;
}

// @Ok
// @Matching
u8 CPlayer::ShouldPlayerDropFlail(void)
{
	return Utils_GetGroundHeight(&this->mPos, 0, 4096, 0) != -1;
}

// Cycle-sorts the animation follow-on table into animation order so that
// entry N describes animation N. Each record is a {u16 anim, u16 followOn}
// pair; a record that does not sit in the slot named by its own anim id is
// swapped towards that slot until it lands or until an empty (all zero)
// record is pulled in. SpideyAI0 (0x4B204A) then indexes the sorted table
// directly by anim id and runs the follow-on anim when the key is set.
// @Ok
void CPlayer::SortAnimationFollowOnData(void)
{
	// {u16 anim, u16 followOnAnim} x 200. Only this sort and SpideyAI0's
	// follow-on lookup touch it, so it stays a local pointer into game
	// memory rather than a shared global.
	static u16 * const gAnimFollowOnData = (u16*)0x00555C6C;
	static u16 * const gAnimFollowOnDataEnd = (u16*)0x00555F8C;

	i32 slot = 0;
	u16 *pEntry = gAnimFollowOnData;

	do
	{
		while (pEntry[0] != slot && (pEntry[0] | pEntry[1]) != 0)
		{
			u16 *pDest = &gAnimFollowOnData[pEntry[0] * 2];

			u16 anim = pDest[0];
			u16 followOn = pDest[1];

			pDest[0] = pEntry[0];
			pDest[1] = pEntry[1];
			pEntry[0] = anim;
			pEntry[1] = followOn;
		}

		pEntry += 2;
		slot++;
	}
	while (pEntry < gAnimFollowOnDataEnd);
}

// Cycle-sorts the fists table into animation order, exactly like
// SortAnimationFollowOnData above but over single u16 records: the low 12
// bits hold the animation id, the top bits the fists variant. SpideyAI0
// (0x4B8653) reads gFistsData[anim] >> 14 and hands that to
// CPlayer::CreateFists. MSVC6 inlined this into ParseFightData.
// @Ok
void CPlayer::SortFistsData(void)
{
	// u16 x 200, low 12 bits = anim id, top 2 bits = fists variant.
	static u16 * const gFistsData = (u16*)0x00555A14;
	static u16 * const gFistsDataEnd = (u16*)0x00555BA4;

	i32 slot = 0;
	u16 *pEntry = gFistsData;

	do
	{
		while ((pEntry[0] & 0xFFF) != slot && (pEntry[0] & 0xFFF) != 0)
		{
			u16 *pDest = &gFistsData[pEntry[0] & 0xFFF];

			u16 entry = pDest[0];
			pDest[0] = pEntry[0];
			pEntry[0] = entry;
		}

		pEntry++;
		slot++;
	}
	while (pEntry < gFistsDataEnd);
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

// @Ok
// residue: 88 mnemonic diffs (down from 122 on the first honest pass,
// re-verified with cmpsum, 0x4BDFF0). the
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

static i16 * const word_610C4A = (i16*)0x610C4A;
static i16 * const word_610C48 = (i16*)0x610C48;

// @FIXME guess: zeroed unconditionally at the very top of every call. No
// established evidence beyond address-adjacency to Vblanks (0x6B4CA0)
// and gTimerRelated (0x6B4CA8); also touched by two other unnamed
// functions (0x4E5CF0, 0x4931E0) not yet decompiled.
static i32 * const gPlayerSynthTickScratch = (i32*)0x6B4CA4;

// @FIXME guess: one byte before the already-established gWhatIf
// (0x60CFC5, idb_globals.txt). Gates both "drop every pending command
// block and restart the phase-1 script" (top of this function) and
// "teleport back to the field_1A8 checkpoint trig" (bottom of this
// function), which fits a debug/replay "what if" rewind feature.
static u8 * const gWhatIfPending = (u8*)0x60CFC4;

// @FIXME guess: adjacent to the pshell globals gPshellArmorRealted
// (0x682940)/gShellInitialized(0x682948)/idb_globals.txt. Read-only
// here; when set, forces gLevelStatus = 7 (front.cpp already uses that
// same value to kick the front-end back to a menu state).
static i32 * const gPshellForceLevelExit = (i32*)0x68293C;

// @FIXME guess: near gBombDieRelatedOne (0x60F771, idb_globals.txt).
// Set from a phase-1 script opcode (18) parameter; purpose otherwise
// unclear.
static u8 * const gSynthInputScriptFlag = (u8*)0x60F770;

// @Ok
// Byte-code VM, same idiom as CSpClone::SynthesizeAnalogueInput
// (spclone.cpp) and CBlackCat::SynthesizeAnalogueInput (blackcat.cpp),
// reverse engineered from IDA decompile+disasm of 0x4BC300 (4109 bytes).
// field_1B8 is the byte-code stream (i16 dueTime/opcode/params entries,
// same shape as CSpClone's field_348), field_1B0 is the elapsed time
// counter (field_80 added per tick), field_1B4 is the phase-1-active
// byte flag, field_1BC is the command-block list head (same node layout
// as CSpClone's field_34C: block[0]=type, block[1]=dword-size,
// block[size-1]=next pointer).
//
// GetNewCommandBlock/KillCommandBlock/KillAllCommandBlocks are called as
// real member functions here instead of the inlined-at-each-call-site
// shape the original compiled (same class of residue already documented
// for CSpClone's version) -- acceptable under this session's
// functional-only bar.
//
// Phase-1 opcodes (all field offsets/constants checked against the
// disasm):
//   1  = teleport to a trig (mPos.vx/vz from the trig, mPos.vy from
//        ground height minus field_EA8), re-orient to the nearest
//        cached surface normal (field_C6C for a floor-like normal,
//        field_C84 for a wall-like one, else a flat up-vector), reset
//        velocity, and tear down any pending look-target/hold-object
//        handles (field_E64, field_E6C, field_5E4 SFX). Same overall
//        shape as CheckStickToCeiling's OrientToNormal call.
//   2  = read a trig id, enqueue a persistent type-2 "walk to trig"
//        block (stores target x/z only, like CSpClone's opcode 2, but
//        does not cancel any other block type first and does not play
//        an anim).
//   3  = read an input-flag index + duration, enqueue a persistent
//        type-3 "hold digital input flag N for M ticks" block.
//   4  = Redbook_XAPlay(a,b,c).
//   5  = read trig id + duration, enqueue a persistent type-5 "move
//        toward trig for N ticks" block (identical shape to CSpClone's
//        opcode 5).
//   6  = read anim id + duration, enqueue a persistent type-6
//        "hold/replay anim for N ticks" block.
//   7  = read a duration, enqueue a persistent type-7 "force
//        SwitchToStandMode after N ticks" timer block.
//   8  = read a value + duration, enqueue a persistent type-8 "hold
//        field_E00 = value for N ticks" block.
//   9  = read a duration, enqueue a persistent type-9 plain "wait N
//        ticks" block; deactivates phase 1 immediately.
//   15 = read anim id, enqueue a persistent type-15 "run anim once"
//        block; deactivates phase 1 immediately.
//   16 = read a trig id, store it in field_1A8 (the checkpoint used by
//        the end-of-function "what if" rewind).
//   17 = if field_AD4 is set, enqueue a type-3 block holding input-flag
//        index 16 (no expiry until the next tick boundary).
//   18 = read a param, set gSynthInputScriptFlag = (param != 0).
//   255 = stop phase 1 processing (field_1B4 = 0) with no stream
//        advance.
//
// Phase 2 walks field_1BC (same node layout as GetNewCommandBlock /
// KillCommandBlock):
//   2  steers mVel toward its target using CameraList->field_23A-relative
//      heading baked through the rcossin_tbl magnitude table
//      (word_610C4A/word_610C48) into field_E2D/field_E2E (the
//      synthesized analogue stick axes), and deletes itself once close
//      (Utils_XZDist < 64), waking phase 1 again.
//   3  is the 20-slot digital input-flag holder: while active it ORs its
//      index into the local heldMask and latches this+0x1C0+16*index
//      (same 20-slot/stride-0x10 array CPlayer::~CPlayer's death cleanup
//      already zeroes); indices 8/9/10/11 additionally force
//      field_E2E/field_E2D to +-127 (hard up/down/left/right override).
//   5  steers via Utils_Dist/VectorNormal directly into mVel (identical
//      to CSpClone's own case 5) and deletes itself once close
//      (Utils_Dist < 64), waking phase 1 again.
//   6  is a countdown timer that holds/replays an anim (resetting its
//      gSpideySFXEntry high-bit flags first, same idiom as
//      CPlayer::DeathCleanup) while field_AD4 is set or the anim
//      changed.
//   7  is a countdown timer that forces field_E1C = 0x40000000 each
//      tick, then calls SwitchToStandMode() on expiry.
//   8  is a countdown timer that holds field_E00 = its param, resetting
//      field_E00 to 0 on expiry.
//   9  is a plain countdown timer with no side effects; on expiry it
//      wakes phase 1 again.
//   15 is a one-shot anim gated on mInputFlags bit 0 (identical to
//      CSpClone's own case 15): once set, it kills itself and wakes
//      phase 1; until then it keeps replaying the current anim whenever
//      mAnimFinished.
//
// After the block walk, this publishes the 20-slot input-flag array
// (this+0x1C0) into the synthesized pad struct pointed to by field_E0C
// (offsets +0/+1/+2 per slot, same struct CheckCeilingJumpingSmashPunch
// and CheckStickToCeiling already read via
// reinterpret_cast<u8*>(this->field_E0C)[...]), clearing any slot not
// held by an active type-3 block this frame (indices 14/15 are always
// skipped). Finally, once both field_1BC and field_1B4 are empty/clear,
// it runs a "zone 1795 mech boss" proximity check (via G_MECHLIST) that
// can clear field_1AC, applies gPshellForceLevelExit -> gLevelStatus,
// and (gWhatIfPending only) teleports back to the field_1A8 checkpoint
// trig. The original returns a bool in AL that no caller reads; kept
// void here to match the already-committed declaration (spidey.h),
// same as CSpClone/CBlackCat's own SynthesizeAnalogueInput.
void CPlayer::SynthesizeAnalogueInput(void)
{
	*gPlayerSynthTickScratch = 0;
	this->field_1B0 += this->field_80;

	u8 *pad = reinterpret_cast<u8*>(this->field_E0C);
	this->field_EA6 = 0;

	static const i32 clearedSlots[16] = {0,1,2,3,4,5,6,7,8,9,10,11,16,17,18,19};
	for (i32 i = 0; i < 16; i++)
	{
		*reinterpret_cast<i16*>(pad + 16 * clearedSlots[i]) = 0;
	}

	if (*gWhatIfPending)
	{
		this->field_1B4 = 0;
		this->KillAllCommandBlocks();
	}

	if (this->field_1B4)
	{
		do
		{
			i16* stream = reinterpret_cast<i16*>(this->field_1B8);
			i16 dueTime = stream[0];
			if (dueTime != -1 && dueTime > this->field_1B0)
				break;

			this->field_1B0 = 0;
			stream++;
			i16 opcode = stream[0];
			stream++;
			this->field_1B8 = reinterpret_cast<i32>(stream);

			switch (opcode)
			{
				case 1:
				{
					i32 trigId = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					Trig_GetPosition(&this->mPos, trigId);
					this->field_E8 = this->mPos;

					if (this->field_54C)
					{
						this->field_54C = 0;
						CameraList->field_12C = -1;
					}

					if (this->field_E64)
					{
						delete reinterpret_cast<SVTableSlot0Deletable*>(this->field_E64);
					}
					this->field_E64 = 0;

					if (this->field_E6C)
					{
						delete reinterpret_cast<SVTableSlot0Deletable*>(this->field_E6C);
					}
					this->field_E6C = 0;

					if (this->field_5E4)
					{
						SFX_Stop(this->field_5E4);
						this->field_5E4 = 0;
					}

					i32 normalY = this->field_A8.vy;
					if (normalY > 3400)
					{
						this->field_AC8 = this->field_C6C;
						this->field_A8.vx = 0;
						this->field_A8.vy = -4096;
						this->field_A8.vz = 0;
						this->OrientToNormal(true, &this->field_AC8);
					}
					else if (normalY >= -2600)
					{
						this->field_AC8 = this->field_C84;
						this->field_A8.vx = 0;
						this->field_A8.vy = -4096;
						this->field_A8.vz = 0;
						this->OrientToNormal(true, &this->field_AC8);
					}
					else
					{
						this->OrientToNormal(false, &ZeroVector);
					}

					this->field_AD4 = 0;

					this->mVel.vx = 0;
					this->mVel.vy = 0;
					this->mVel.vz = 0;

					i32 groundHeight = Utils_GetGroundHeight(&this->mPos, 0, 256, 0);
					if (groundHeight == -1)
					{
						this->field_E1C = 4;
					}
					else
					{
						this->field_E1C = 1;
						this->mPos.vy = groundHeight - (this->field_EA8 << 12);
					}
					continue;
				}

				case 2:
				{
					i32 trigId = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					CVector target;
					target.vx = 0; target.vy = 0; target.vz = 0;
					Trig_GetPosition(&target, trigId);

					i32* block = this->GetNewCommandBlock(5);
					block[0] = 2;
					block[1] = 5;
					block[2] = target.vx;
					block[3] = target.vz;

					this->field_1B4 = 0;
					continue;
				}

				case 3:
				{
					i32 index = stream[0];
					stream++;
					i32 duration = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(5);
					block[0] = 3;
					block[1] = 5;
					block[2] = index;
					block[3] = duration + this->field_80;
					continue;
				}

				case 4:
				{
					i16 p1 = stream[0]; stream++;
					i16 p2 = stream[0]; stream++;
					i16 p3 = stream[0]; stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					Redbook_XAPlay(p1, p2, p3);
					continue;
				}

				case 5:
				{
					i32 trigId = stream[0]; stream++;
					i32 duration = stream[0]; stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					CVector target;
					target.vx = 0; target.vy = 0; target.vz = 0;
					Trig_GetPosition(&target, trigId);

					i32* block = this->GetNewCommandBlock(7);
					block[0] = 5;
					block[1] = 7;
					block[2] = target.vx;
					block[3] = target.vy;
					block[4] = target.vz;
					block[5] = duration;

					this->field_1B4 = 0;
					continue;
				}

				case 6:
				{
					i32 animId = stream[0]; stream++;
					i32 duration = stream[0]; stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(5);
					block[0] = 6;
					block[1] = 5;
					block[2] = animId;
					block[3] = duration + this->field_80;
					continue;
				}

				case 7:
				{
					i32 duration = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(4);
					block[0] = 7;
					block[1] = 4;
					block[2] = duration + this->field_80;
					continue;
				}

				case 8:
				{
					i32 value = stream[0]; stream++;
					i32 duration = stream[0]; stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(5);
					block[0] = 8;
					block[1] = 5;
					block[2] = value;
					block[3] = duration + this->field_80;
					continue;
				}

				case 9:
				{
					i32 duration = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(4);
					block[0] = 9;
					block[1] = 4;
					block[2] = duration + this->field_80;

					this->field_1B4 = 0;
					continue;
				}

				case 15:
				{
					i32 animId = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(3);
					block[0] = 15;
					block[1] = 3;

					this->field_1B4 = 0;
					if (this->mAnim != animId || this->mAnimFinished)
						this->RunAnim(animId, 0, -1);
					continue;
				}

				case 16:
				{
					this->field_1A8 = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);
					continue;
				}

				case 17:
				{
					if (this->field_AD4)
					{
						i32* block = this->GetNewCommandBlock(5);
						block[0] = 3;
						block[1] = 5;
						block[2] = 16;
						block[3] = this->field_80;
					}
					continue;
				}

				case 18:
				{
					i32 param = stream[0];
					stream++;
					this->field_1B8 = reinterpret_cast<i32>(stream);

					*gSynthInputScriptFlag = (param != 0);
					continue;
				}

				case 255:
					this->field_1B4 = 0;
					continue;

				default:
					continue;
			}
		}
		while (this->field_1B4);
	}

	i32 heldMask = 0;
	i32* block = this->field_1BC;
	while (block)
	{
		switch (block[0])
		{
			case 2:
			{
				CVector target;
				target.vx = block[2];
				target.vy = 0;
				target.vz = block[3];

				if (Utils_XZDist(&target, &this->mPos) < 64)
				{
					block = this->KillCommandBlock(block);
					this->field_1B4 = 1;
					break;
				}

				i32 dz = (block[3] - this->mPos.vz) >> 12;
				i32 dx = (block[2] - this->mPos.vx) >> 12;
				i32 idx = 2 * ((1024 - ratan2(dz, dx) - CameraList->field_23A) & 0xFFF);

				i32 x = word_610C4A[idx] / 32;
				if (x > 127) x = 127;
				else if (x < -127) x = -127;
				this->field_E2D = static_cast<char>(x);

				i32 y = word_610C48[idx] / -32;
				if (y > 127) y = 127;
				else if (y < -127) y = -127;
				this->field_E2E = static_cast<char>(y);

				block = reinterpret_cast<i32*>(block[block[1] - 1]);
				break;
			}

			case 3:
			{
				i32 index = block[2];
				i32 remaining = block[3] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					break;
				}

				block[3] = remaining;
				heldMask |= 1 << index;

				switch (index)
				{
					case 0: case 1: case 2: case 3:
					case 4: case 5: case 6: case 7:
					case 16: case 17: case 18: case 19:
					{
						u8* slot = reinterpret_cast<u8*>(this) + 0x1C0 + 16 * index;
						if (slot[0] == 0)
							slot[1] = 1;
						slot[0] = 1;
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						break;
					}

					case 8:
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						*(reinterpret_cast<u8*>(this) + 0x1C0 + 16 * 8) = 1;
						this->field_E2E = -127;
						break;

					case 9:
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						*(reinterpret_cast<u8*>(this) + 0x1C0 + 16 * 9) = 1;
						this->field_E2E = 127;
						break;

					case 10:
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						*(reinterpret_cast<u8*>(this) + 0x1C0 + 16 * 10) = 1;
						this->field_E2D = -127;
						break;

					case 11:
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						*(reinterpret_cast<u8*>(this) + 0x1C0 + 16 * 11) = 1;
						this->field_E2D = 127;
						break;

					default:
						print_if_false(0, "Bad register index");
						block = reinterpret_cast<i32*>(block[block[1] - 1]);
						break;
				}
				break;
			}

			case 5:
			{
				CVector target;
				target.vx = block[2];
				target.vy = block[3];
				target.vz = block[4];

				if (Utils_Dist(target, this->mPos) < 64)
				{
					block = this->KillCommandBlock(block);
					this->field_1B4 = 1;
					break;
				}

				CVector delta = target;
				delta -= this->mPos;
				delta >>= 12;
				VectorNormal(
						reinterpret_cast<VECTOR*>(&delta),
						reinterpret_cast<VECTOR*>(&this->mVel));

				block = reinterpret_cast<i32*>(block[block[1] - 1]);
				break;
			}

			case 6:
			{
				i32 animId = block[2];
				i32 remaining = block[3] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					break;
				}

				block[3] = remaining;

				if (this->field_AD4 || this->mAnim != animId)
				{
					i32* p = gSpideySFXEntry[animId];
					this->field_350 = p;

					if (p)
					{
						while (p[0] != -1)
						{
							p[0] &= 0xFFFF;
							p++;
						}
					}

					this->RunAnim(animId, 0, -1);
				}

				block = reinterpret_cast<i32*>(block[block[1] - 1]);
				break;
			}

			case 7:
			{
				i32 remaining = block[2] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					this->SwitchToStandMode();
				}
				else
				{
					block[2] = remaining;
					this->field_E1C = 0x40000000;
					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			case 8:
			{
				i32 remaining = block[3] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					this->field_E00 = 0;
				}
				else
				{
					i32 value = block[2];
					block[3] = remaining;
					block = reinterpret_cast<i32*>(block[block[1] - 1]);
					this->field_E00 = value;
				}
				break;
			}

			case 9:
			{
				i32 remaining = block[2] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					this->field_1B4 = 1;
				}
				else
				{
					block[2] = remaining;
					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			case 15:
			{
				if (this->mInputFlags & 1)
				{
					this->mInputFlags &= ~1;
					block = this->KillCommandBlock(block);
					this->field_1B4 = 1;
				}
				else
				{
					if (this->mAnimFinished)
						this->RunAnim(this->mAnim, 0, -1);

					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			default:
				print_if_false(0, "Bad command");
				break;
		}
	}

	u8* src = reinterpret_cast<u8*>(this) + 0x1C0;
	u8* dst = reinterpret_cast<u8*>(this->field_E0C) + 2;

	for (i32 slotIdx = 0; slotIdx < 20; slotIdx++)
	{
		if (slotIdx != 14 && slotIdx != 15)
		{
			if (!((1 << slotIdx) & heldMask))
				src[0] = 0;

			u8 state = src[0];
			if (state == 0)
			{
				src[1] = 0;
				src[2] = 0;
			}

			dst[-2] = state;
			dst[-1] = src[1];
			dst[0] = src[2];
		}

		src += 16;
		dst += 16;
	}

	if (!this->field_1BC && !this->field_1B4)
	{
		// 31900664/34009330/4915200 are plain fixed-point world-space
		// constants (IDA mislabeled them unk_1E3CBF8/unk_20532F2/
		// loc_4B0000 because they happen to look like valid addresses).
		if (Trig_GetLevelId() != 1795 || !G_MECHLIST
				|| my_abs(31900664 + G_MECHLIST->mPos.vx) >= 4915200
				|| my_abs(G_MECHLIST->mPos.vz - 34009330) >= 4915200)
		{
			this->field_1AC = 0;
		}

		if (*gPshellForceLevelExit)
			gLevelStatus = 7;

		if (*gWhatIfPending && this->field_1A8)
		{
			Trig_GetPosition(&this->mPos, this->field_1A8);
		}
	}
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

// Manages the two web-hook smoke trails: field_58C (hook 1) and field_590
// (hook 0). Based on mAnim/mFrame it creates, updates, or destroys them,
// computing intermediate trail steps via CalculateIntermediateTrailSteps.
// field_594[0]/[1] hold the previous per-hook trail offset (pos - mPos).
// @Ok
void CPlayer::UpdateTrails(void)
{
	u8 deleteFlag = 0;  // also destroy field_590 when leaving the web anims
	u8 createFlag = 0;  // create field_590 during anim 106, frames 2..7
	i32 action = 0;     // field_58C: 0 none, 1 create, 2 update, 3 destroy

	if (this->mAnim == 104)
	{
		if (this->mFrame > 6)
			action = 3;
		else if (this->mFrame <= 0)
			action = (this->field_58C == 0) ? 0 : 2;
		else
			action = (this->field_58C == 0) ? 1 : 2;
	}
	else if (this->mAnim == 106)
	{
		if (this->mFrame > 7)
		{
			deleteFlag = 1;
			action = 3;
		}
		else if (this->mFrame > 1)
		{
			if (this->field_590 == 0)
				createFlag = 1;
			action = (this->field_58C == 0) ? 1 : 2;
		}
		else
		{
			action = (this->field_58C == 0) ? 0 : 2;
		}
	}
	else
	{
		deleteFlag = 1;
		action = 3;
	}

	if (action == 3)
	{
		if (this->field_58C != 0)
		{
			this->field_58C->~CSmokeTrail();
			this->field_58C = 0;
		}
		if (deleteFlag != 0 && this->field_590 != 0)
		{
			this->field_590->~CSmokeTrail();
			this->field_590 = 0;
		}
	}
	else if (action == 1)
	{
		CVector pos;
		pos.vx = 0;
		pos.vy = 0;
		pos.vz = 0;
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pos), this, 1);
		void *mem = CBit::operator new(88);
		CSmokeTrail *trail = 0;
		if (mem != 0)
			trail = ::new (mem) CSmokeTrail(&pos, 6,
					this->field_580 & 0xFF,
					(this->field_580 >> 8) & 0xFF,
					(this->field_580 >> 16) & 0xFF);
		this->field_58C = trail;
		this->field_594[0] = pos - this->mPos;
	}
	else if (action == 2)
	{
		CVector steps[4];
		for (i32 i = 0; i < 4; i++)
		{
			steps[i].vx = 0;
			steps[i].vy = 0;
			steps[i].vz = 0;
		}
		CVector pos;
		pos.vx = 0;
		pos.vy = 0;
		pos.vz = 0;
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pos), this, 1);
		CVector delta = pos - this->mPos;
		i32 count = this->CalculateIntermediateTrailSteps(&delta, &this->field_594[0], steps);
		if (count > 0)
		{
			CVector *p = steps;
			for (i32 j = count; j != 0; --j)
				this->field_58C->SetPos(*p++);
		}
		this->field_58C->SetPos(pos);
		this->field_594[0] = pos - this->mPos;
	}

	if (createFlag != 0)
	{
		CVector pos;
		pos.vx = 0;
		pos.vy = 0;
		pos.vz = 0;
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pos), this, 0);
		void *mem = CBit::operator new(88);
		CSmokeTrail *trail = 0;
		if (mem != 0)
			trail = ::new (mem) CSmokeTrail(&pos, 6,
					this->field_580 & 0xFF,
					(this->field_580 >> 8) & 0xFF,
					(this->field_580 >> 16) & 0xFF);
		this->field_590 = trail;
		this->field_594[1] = pos - this->mPos;
	}
	else if (this->field_590 != 0)
	{
		CVector steps[4];
		for (i32 i = 0; i < 4; i++)
		{
			steps[i].vx = 0;
			steps[i].vy = 0;
			steps[i].vz = 0;
		}
		CVector pos;
		pos.vx = 0;
		pos.vy = 0;
		pos.vz = 0;
		M3dUtils_GetHookPosition(reinterpret_cast<VECTOR*>(&pos), this, 0);
		CVector delta = pos - this->mPos;
		i32 count = this->CalculateIntermediateTrailSteps(&delta, &this->field_594[1], steps);
		if (count > 0)
		{
			CVector *p = steps;
			for (i32 j = count; j != 0; --j)
				this->field_590->SetPos(*p++);
		}
		this->field_590->SetPos(pos);
		this->field_594[1] = pos - this->mPos;
	}
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

// @Ok
// address found and verified this session: IDA sub_4B9210 (0x4B9210).
// confirmed the subtract-then-add source pointer really does cancel down
// to gSpideyHeadModel+2 (v6 = gSpideyHeadModel - v2; v8 = (result=v2+28)
// + v6 - 26 = gSpideyHeadModel + 2). cmpsum shows 57 mnemonic diffs,
// matching this blocker plus scheduling residue.
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

// @Ok
// re-verified this session: cmpsum (0x4B8E60) shows 45 mnemonic diffs,
// matching the low-graphics-branch residue documented below exactly.
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

// @Ok
// address found and verified this session: IDA sub_4B8C80 (0x4B8C80).
// note: tools/names.json (local working copy) mislabels this address as
// CPlayer_IfPlayerCeilingCheck; that is wrong, this is
// Spidey_StoreTextureEntry (confirmed by decompiling it: gLowGraphics
// check, the SGlobalTextureEntry SoA-looking-but-really-AoS indexing
// tricks, gSuitChecksumTable walk, all match). The real
// CPlayer::IfPlayerCeilingCheck (already @Ok elsewhere in this file) is
// a different, unrelated function; did not touch names.json (local-only
// per repo convention), just noting the mislabel for whoever looks at it
// next. cmpsum confirms the documented 38 mnemonic diffs.
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

// @Ok
// re-verified this session: cmpsum (0x4B8D80) confirms the documented 50
// mnemonic diffs.
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

// @Ok
// address found and verified this session: IDA sub_4B9180 (0x4B9180).
// cmpsum confirms the documented 22 mnemonic diffs.
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

// @Ok
// address found and verified this session: IDA sub_4B9E50 (0x4B9E50,
// 0x59 bytes). cmpsum confirms the documented 23 mnemonic diffs.
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

// @Ok
// address found and verified this session: IDA sub_4BB180 (0x4BB180,
// 0x4C bytes), found by searching the whole binary for the "and edx,
// 0FFFFFFDFh" constant this function's bit math produces. field offset
// (ecx+0x194) matches this->field_194. bit math (clear/set bits 5,6 for
// the first flag, bits 10,11 for the second) matches this source exactly.
void CPlayer::CreateFists(u8 a2)
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

// gWebbingDecreaseDisabled (0x60CFE8): no idb_globals.txt entry, tentative
// name from usage (gates DecreaseWebbing below alongside field_1AC and the
// CurrentSuit checks).
static i32 * const gWebbingDecreaseDisabled = (i32*)0x60CFE8;
// gDifficultyLevel (0x54D474): named DifficultyLevel in idb_globals.txt.
static i32 * const gDifficultyLevel = (i32*)0x54D474;
// byte_682770: no idb_globals.txt entry for this exact address, but it
// sits directly before Redbook_XAPaused (0x682771, idb_globals.txt) and
// gates a Redbook_XAPlay call the same way a "currently playing" flag
// would; tentative name only, not confirmed.
static char * const gRedbookXaPlayingMaybe = (char*)0x682770;
extern int CurrentSuit;

// @Ok
// verified against IDA sub_4BB0A0 (0x4BB0A0, 0xD3 bytes). Found and fixed
// one bug from an earlier revision: when gDifficultyLevel is neither 0
// nor 1, the original sets v4 = a2 directly (unshifted, no >>12 at all)
// and jumps straight past the shift; the earlier revision left v4
// uninitialised in that case (no else branch). All field offsets
// (field_1AC 0x1AC, mWebbing 0x5D4, field_5D8 0x5D8, field_5E8 0x5E8,
// field_E10 0xE10) match the disassembly.
char CPlayer::DecreaseWebbing(i32 a2)
{
	if (!this->field_1AC &&
			!*gWebbingDecreaseDisabled &&
			CurrentSuit != 3 &&
			CurrentSuit != 4)
	{
		int v3;
		int v4;

		int tmpDword = *gDifficultyLevel;
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
		else
		{
			v4 = a2;
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
			if (!*gRedbookXaPlayingMaybe)
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

// -field_A8 (negated surface normal) that CPlayer::OrientToNormal saw the
// last time it rebuilt this->mTransform; used to skip the rebuild when the
// normal has barely moved since. Falls in the same unnamed CPlayer scratch
// area as gKillTauntLastVariant (0x6A9070) / gSpideySenseListLastUpdateTime
// (0x6A9084) above.
static CVector * const gOrientToNormalLastNormal = (CVector*)0x006A9074;

// @Ok
// Builds this->mTransform (the player's local rotation matrix) so its
// column 1 (m[*][1]) is -field_A8 (the current surface normal) and columns
// 0/2 are an orthonormal right/forward pair derived from it via GTE cross
// products. When useTarget is true, target supplies the reference
// direction for the cross product; otherwise the previous forward axis
// (this->mTransform's old column 2, staged through field_C6C) is reused so
// the basis doesn't visibly twist, and the rebuild is skipped entirely
// unless field_A8 moved more than a small threshold or field_548 (a
// pending twist correction) is set.
void CPlayer::OrientToNormal(bool useTarget, CVector *target)
{
	VECTOR negNormal;
	negNormal.vx = -(i32)this->field_A8.vx;
	negNormal.vy = -(i32)this->field_A8.vy;
	negNormal.vz = -(i32)this->field_A8.vz;

	if (useTarget)
	{
		VECTOR rightAxis;
		gte_ldopv1(&negNormal);
		gte_ldopv2(reinterpret_cast<VECTOR*>(target));
		gte_op12();
		gte_stlvnl(&rightAxis);
		VectorNormal(&rightAxis, &rightAxis);

		VECTOR upAxis;
		gte_ldopv1(&rightAxis);
		gte_ldopv2(&negNormal);
		gte_op12();
		gte_stlvnl(&upAxis);

		this->mTransform.m[0][0] = (i16)rightAxis.vx;
		this->mTransform.m[1][0] = (i16)rightAxis.vy;
		this->mTransform.m[2][0] = (i16)rightAxis.vz;

		this->mTransform.m[0][1] = (i16)negNormal.vx;
		this->mTransform.m[1][1] = (i16)negNormal.vy;
		this->mTransform.m[2][1] = (i16)negNormal.vz;

		this->mTransform.m[0][2] = (i16)upAxis.vx;
		this->mTransform.m[1][2] = (i16)upAxis.vy;
		this->mTransform.m[2][2] = (i16)upAxis.vz;
	}
	else
	{
		i32 dx = (i32)this->field_A8.vx - gOrientToNormalLastNormal->vx;
		i32 dy = (i32)this->field_A8.vy - gOrientToNormalLastNormal->vy;
		i32 dz = (i32)this->field_A8.vz - gOrientToNormalLastNormal->vz;

		if (abs(dx) + abs(dy) + abs(dz) > 16 || this->field_548 != 0)
		{
			gOrientToNormalLastNormal->vx = this->field_A8.vx;
			gOrientToNormalLastNormal->vy = this->field_A8.vy;
			gOrientToNormalLastNormal->vz = this->field_A8.vz;

			// stage the OLD forward axis (matrix column 2) as the cross
			// product's reference direction, so the new basis keeps
			// continuity with the previous one instead of snapping.
			this->field_C6C.vx = this->mTransform.m[0][2];
			this->field_C6C.vy = this->mTransform.m[1][2];
			this->field_C6C.vz = this->mTransform.m[2][2];

			VECTOR rightAxis;
			gte_ldopv1(&negNormal);
			gte_ldopv2(reinterpret_cast<VECTOR*>(&this->field_C6C));
			gte_op12();
			gte_stlvnl(&rightAxis);
			VectorNormal(&rightAxis, &rightAxis);

			VECTOR upAxis;
			gte_ldopv1(&rightAxis);
			gte_ldopv2(&negNormal);
			gte_op12();
			gte_stlvnl(&upAxis);

			this->mTransform.m[0][0] = (i16)rightAxis.vx;
			this->mTransform.m[1][0] = (i16)rightAxis.vy;
			this->mTransform.m[2][0] = (i16)rightAxis.vz;

			this->mTransform.m[0][1] = (i16)negNormal.vx;
			this->mTransform.m[1][1] = (i16)negNormal.vy;
			this->mTransform.m[2][1] = (i16)negNormal.vz;

			this->mTransform.m[0][2] = (i16)upAxis.vx;
			this->mTransform.m[1][2] = (i16)upAxis.vy;
			this->mTransform.m[2][2] = (i16)upAxis.vz;

			if (this->field_548 != 0)
			{
				SVECTOR twistRot;
				twistRot.vx = 0;
				twistRot.vy = (i16)this->field_548;
				twistRot.vz = 0;

				MATRIX twistMat;
				M3dMaths_RotMatrixYXZ(&twistRot, &twistMat);
				MulMatrix(&this->mTransform, &twistMat);
			}
		}
	}

	// cache -field_A8 (long-vector width, including its uninitialized pad
	// word -- the original does a plain struct copy here too).
	this->field_D18 = negNormal;

	this->field_C6C.vx = this->mTransform.m[0][2];
	this->field_C6C.vy = this->mTransform.m[1][2];
	this->field_C6C.vz = this->mTransform.m[2][2];

	this->field_C78 = this->mTransform.m[0][0];
	this->field_C7C = this->mTransform.m[1][0];
	this->field_C80 = this->mTransform.m[2][0];

	this->field_C84.vx = -(i32)this->mTransform.m[0][1];
	this->field_C84.vy = -(i32)this->mTransform.m[1][1];
	this->field_C84.vz = -(i32)this->mTransform.m[2][1];

	M3dMaths_TransposeMatrix1(&this->mTransform, &this->field_89C);

	i32 rightMag = abs(this->mTransform.m[0][0]) + abs(this->mTransform.m[1][0]) + abs(this->mTransform.m[2][0]);
	i32 normalMag = abs(this->mTransform.m[0][1]) + abs(this->mTransform.m[1][1]) + abs(this->mTransform.m[2][1]);
	i32 upMag = abs(this->mTransform.m[0][2]) + abs(this->mTransform.m[1][2]) + abs(this->mTransform.m[2][2]);

	// degenerate basis (e.g. target ended up parallel to the normal):
	// fall back to a fixed mirrored-identity matrix and force a stand-mode
	// reset instead of running with a near-zero axis.
	if (rightMag < 2048 || normalMag < 2048 || upMag < 2048)
	{
		this->mTransform.m[0][0] = -4096;
		this->mTransform.m[1][0] = 0;
		this->mTransform.m[2][0] = 0;

		this->mTransform.m[0][1] = 0;
		this->mTransform.m[1][1] = 4096;
		this->mTransform.m[2][1] = 0;

		this->mTransform.m[0][2] = 0;
		this->mTransform.m[1][2] = 0;
		this->mTransform.m[2][2] = -4096;

		this->SwitchToStandMode();
	}
}

// @BIGTODO
void CPlayer::PriorToVenomDistanceAttack(CVector)
{}

// Transitions back to the stand (idle) animation from a finishing move.
// Picks the stand anim + SFX entry to play based on the current mAnim (and
// mAnimDir/mFrame/mHeldObject/field_AD4 for the ambiguous cases), resets the
// associated SFX entry, and sets field_E1C.
// @Ok
void CPlayer::SwitchToStandMode(void)
{
	u16 mAnim = this->mAnim;

	if (mAnim == 50 || mAnim == 51 || mAnim == 60 || mAnim == 63 || mAnim == 72 || mAnim == 75)
	{
		this->field_350 = gSpideySFXEntry[55];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(55);
		this->RunAnim(0x37, 0, -1);
		this->field_E1C = 1;
		return;
	}

	if (mAnim == 52 || mAnim == 66 || mAnim == 69 || mAnim == 78 || mAnim == 81)
	{
		this->field_350 = gSpideySFXEntry[56];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(56);
		this->RunAnim(0x38, 0, -1);
		this->field_E1C = 1;
		return;
	}

	if (mAnim == 57 && this->mAnimDir == 1)
	{
		i32 frame = this->mFrame;
		this->field_350 = gSpideySFXEntry[57];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(57);
		this->RunAnim(0x39, frame, 0);
		this->field_E1C = 1;
		return;
	}

	if (mAnim == 58 && this->mAnimDir == 1)
	{
		i32 frame = this->mFrame;
		this->field_350 = gSpideySFXEntry[58];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(58);
		this->RunAnim(0x3A, frame, 0);
		this->field_E1C = 1;
		return;
	}

	switch (mAnim)
	{
		case 0x81:
			this->field_350 = gSpideySFXEntry[130];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(130);
			this->RunAnim(0x82, 0, -1);
			this->field_E1C = 1;
			return;
		case 0x85:
			this->field_350 = gSpideySFXEntry[134];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(134);
			this->RunAnim(0x86, 0, -1);
			this->field_E1C = 1;
			return;
		case 0xE:
			this->field_350 = gSpideySFXEntry[55];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(55);
			this->RunAnim(0x37, 0, -1);
			this->field_E1C = 1;
			return;
		case 0xC4:
			this->field_350 = gSpideySFXEntry[200];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(200);
			this->RunAnim(0xC8, 0, -1);
			this->field_E1C = 1;
			return;
		case 0xBE:
			this->field_350 = gSpideySFXEntry[194];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(194);
			this->RunAnim(0xC2, 0, -1);
			this->field_E1C = 1;
			return;
		case 0x15:
		{
			i16 frame = this->mFrame;
			if (frame >= 18 || frame <= 3)
			{
				this->field_350 = gSpideySFXEntry[11];
				if (this->field_350 != 0)
					this->ResetSFXArrayEntry(11);
				this->RunAnim(0xB, 0, -1);
			}
			else if (frame < 10 || frame > 14)
			{
				this->field_350 = gSpideySFXEntry[13];
				if (this->field_350 != 0)
					this->ResetSFXArrayEntry(13);
				this->RunAnim(0xD, 0, -1);
			}
			else
			{
				this->field_350 = gSpideySFXEntry[12];
				if (this->field_350 != 0)
					this->ResetSFXArrayEntry(12);
				this->RunAnim(0xC, 0, -1);
			}
			this->field_E1C = 1;
			return;
		}
		case 0x14:
		case 0x82:
		case 0x86:
			this->field_E1C = 1;
			return;
		default:
			break;
	}

	if (this->field_AD4 != 0)
	{
		this->field_350 = gSpideySFXEntry[19];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(19);
		this->RunAnim(0x13, 0, -1);
		this->field_E1C = 1;
	}
	else if (this->mHeldObject != 0)
	{
		if ((this->mHeldObject->field_10C & 8) == 0)
		{
			this->field_350 = gSpideySFXEntry[194];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(194);
			this->RunAnim(0xC2, 0, -1);
		}
		else
		{
			this->field_350 = gSpideySFXEntry[200];
			if (this->field_350 != 0)
				this->ResetSFXArrayEntry(200);
			this->RunAnim(0xC8, 0, -1);
		}
		this->field_E1C = 1;
	}
	else
	{
		this->field_350 = gSpideySFXEntry[0];
		if (this->field_350 != 0)
			this->ResetSFXArrayEntry(0);
		this->RunAnim(0, 0, -1);
		this->field_E1C = 1;
	}
}

// duplicate of the byte right before gWhatIf (0x60CFC4, ob.cpp). Name from
// baddy.cpp's gSubmarinerDieRelated. Tentative, static per file per repo
// convention.
static u8 * const gSubmarinerDieRelated = (u8*)0x60CFC4;

// @Ok
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

			this->OrientToNormal(true, &v14);
		}
		else
		{
			v14 = this->field_C6C;
		}

		this->PriorToVenomDistanceAttack(v14);
	}

	this->PlaySingleAnim(0, 0, -1);
	this->SwitchToStandMode();

	this->field_E00 = 0;
	CameraList->SetStartPosition();

	char * v13 = reinterpret_cast<char*>(this->field_E0C);

	// shared with baddy.cpp/venom.cpp: gSubmarinerDieRelated (0x60CFC4).
	*gSubmarinerDieRelated = 0;

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

// @Ok
// trivial two-field store, functionally correct regardless of
// gUserFunctionName/gUserFunctionSize's exact (relocatable) address.
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

// @Ok
// @Matching
// verified against IDA sub_4C0E60 (0x4C0E60, 0x31 bytes), cmpsum shows 0
// mnemonic diffs (byte-identical: True). field_584/588 offsets match.
// mFadeAway is CSmokeTrail::mFadeAway (0x54, VALIDATEd in bit.cpp), which
// field_584/588 are already typed as (spidey.h), so the earlier
// revision's int* casts (tmp[21]) were unnecessary; simplified to plain
// member access.
void CPlayer::DestroyJumpingSmashKickTrail(void)
{
	if (this->field_584)
	{
		this->field_584->mFadeAway = 1;
		this->field_584 = NULL;
	}

	if (this->field_588)
	{
		this->field_588->mFadeAway = 1;
		this->field_588 = NULL;
	}
}

// @Ok
// @Matching
// verified against IDA sub_4C0EA0 (0x4C0EA0, 0x30 bytes). field_58C/590
// offsets match, same CSmokeTrail::mFadeAway pattern as
// DestroyJumpingSmashKickTrail above; simplified away the unnecessary
// int* casts the same way. compare.py has no tools/functions/*.bin entry
// for this address, so verified by reading raw bytes with IDA get_bytes
// and comparing against the built DLL's export directly: byte-for-byte
// identical (49/49 bytes, including the trailing retn).
void CPlayer::DestroyHandTrails(void)
{
	if (this->field_58C)
	{
		this->field_58C->mFadeAway = 1;
		this->field_58C = NULL;
	}

	if (this->field_590)
	{
		this->field_590->mFadeAway = 1;
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

// @Ok
// the original never calls this as a standalone function; the whole loop
// is inlined directly into ~CPlayer (IDA sub_4BAA30, 0x4BAA30), so there
// is no separate original address/bytes to cmpsum against. Hand-verified
// the logic against that inlined copy instead: the loop there is
// `v15 = field_1BC; while (v15) { v16 = v15[v15[1]-1]; ... delete v15 ...;
// v15 = v16; }` followed unconditionally by `field_1BC = 0;` right after
// the loop, exactly matching this function's shape and the KillCommandBlock
// (a1[a1[1]-1]) indexing used above.
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

	VALIDATE(CPlayer, field_548, 0x548);

	VALIDATE(CPlayer, field_54C, 0x54C);
	VALIDATE(CPlayer, field_54F, 0x54F);
	VALIDATE(CPlayer, field_558, 0x558);

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
	VALIDATE(CPlayer, field_8ED, 0x8ED);

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
	VALIDATE(CPlayer, field_CD4, 0xCD4);
	VALIDATE(CPlayer, field_CE4, 0xCE4);
	VALIDATE(CPlayer, field_CE8, 0xCE8);
	VALIDATE(CPlayer, field_CF4, 0xCF4);
	VALIDATE(CPlayer, field_D00, 0xD00);
	VALIDATE(CPlayer, field_D0C, 0xD0C);
	VALIDATE(CPlayer, field_D18, 0xD18);

	VALIDATE(CPlayer, field_D3C, 0xD3C);
	VALIDATE(CPlayer, field_D4E, 0xD4E);

	VALIDATE(CPlayer, field_D80, 0xD80);
	VALIDATE(CPlayer, field_D86, 0xD86);
	VALIDATE(CPlayer, field_D8C, 0xD8C);

	VALIDATE(CPlayer, field_DA0, 0xDA0);
	VALIDATE(CPlayer, field_DAC, 0xDAC);

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
