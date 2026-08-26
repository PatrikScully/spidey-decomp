#include "rhino.h"
#include "validate.h"
#include "utils.h"
#include "panel.h"
#include "ps2pad.h"
#include "spidey.h"
#include "ps2lowsfx.h"
#include "ps2redbook.h"
#include "camera.h"
#include "ai.h"
#include "my_assert.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "effects.h"
#include "zrhinog.h"
#include "web.h"
#include "mem.h"


EXPORT i32 gRhinoStrangeInitData[2] = { 0x201, 0 };

EXPORT SLight M3d_RhinoLight =
{
  { { -2430, -2228, -2430 }, { 2509, -2896, 1447 }, { -648, -3711, -1607 } },
  0,
  { { 4800, 1560, 3072 }, { 4080, 2400, 2880 }, { 3600, 3840, 3072 } },
  0,
  { 1800, 1800, 1440 }
};



// @FIXME
#define LEN_RHINO_DATA 0x17
EXPORT SRhinoData gRhinoData[LEN_RHINO_DATA];

#define LEN_RHINO_DAZED_DATA 0x5
EXPORT i16 gRhinoDazedData[LEN_RHINO_DAZED_DATA];

EXPORT u32 gRhinoSound;
extern i32 DifficultyLevel;

u8 gActuatorRelated;
extern CBody* EnvironmentalObjectList;
extern CPlayer* MechList;
extern i32 gAttackRelated;
extern CBaddy *BaddyList;
extern CCamera *CameraList;

// @Ok
// @Matching
void Rhino_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Rhino_RelocatableModuleClear;
	pMod->field_C[0] = Rhino_CreateRhino;
}

// @MEDIUMTODO
void CRhino::AI(void)
{
    printf("CRhino::AI(void)");
}

// @NotOk
// Logic verified against the disasm. Residue: 18 mnemonic diffs, all inside
// case 2's inlined CheckIfPlayerHit() call. Root cause: `operator-(const
// CVector&, const CVector&)` (vector.h) is declared INLINE, so our build
// computes MechList->mPos - this->mPos as direct sub instructions at the
// call site; the original calls it as a real out-of-line function (matches
// the same __mi call already flagged as unresolved on CheckIfPlayerHit's own
// @NotOk tag). This is the same repo-wide inlining issue as the documented
// print_if_false case, not something a single call site can fix. Attempts:
// (1) if/else-if chain on a cached `i32 subState = this->field_360;` local
// for the sub-state dispatch, 73 diffs (compiler used cmp+jne, not the
// original's sub/dec sequential-compare shape); (2) switched to a real
// `switch (this->field_360) { case 0: ... case 1: ... default: ... }`
// instead, per tips.txt's if-chain-on-cached-local idiom, dropped to 18
// diffs (this residue). Did not reach the 15-hypothesis bar for
// @AlmostMatching, but the remaining residue is a known, out-of-scope,
// repo-wide issue rather than an unexplained one.
void CRhino::AttackPlayer(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_310 = 0x64;
			this->Neutralize();

			new CAIProc_LookAt(this, MechList, 0, 2, 0x50, 0);

			this->field_330 = Rnd(30) + 0x5A;
			this->dumbAssPad++;
			break;
		case 1:
		{
			this->field_330 -= this->field_80;

			if (this->field_288 & 2)
			{
				this->field_288 &= ~2;
			}
			else
			{
				if (this->field_330 > 0)
				{
					return;
				}
			}

			this->field_330 = 0;

			SFX_PlayPos((~gAttackRelated & 1) | 0x80C8, &this->mPos, 0);

			switch (this->field_360)
			{
				case 0:
					this->PlaySingleAnim(0xA, 0, -1);
					new CAIProc_MonitorAttack(this, 7, 0x7000, 6, 0x10);
					this->dumbAssPad = 2;
					this->field_360 = 1;
					break;
				case 1:
					this->PlaySingleAnim(0xB, 0, -1);
					new CAIProc_MonitorAttack(this, 7, 0xE00, 6, 0x10);
					this->dumbAssPad = 2;
					this->field_360 = 0;
					break;
				default:
					print_if_false(0, "What in the name of God above?");
					break;
			}
			break;
		}
		case 2:
			if (this->CheckIfPlayerHit())
			{
				this->dumbAssPad++;
			}
			else if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			break;
		case 3:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 3;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @MEDIUMTODO
void CRhino::ChargePlayer(void)
{
    printf("CRhino::ChargePlayer(void)");
}

// @MEDIUMTODO
void CRhino::ChasePlayer(i32)
{
    printf("CRhino::ChasePlayer(i32)");
}

// @NotOk
// @Validate
INLINE i32 CRhino::CheckIfPlayerHit(void)
{
	i32 v4;

	if (this->field_288 & 0x10)
	{
		this->field_288 &= 0xFFFFFFEF;
		v4 = 1;
	}
	else
	{
		v4 = 0;
	}

	if (v4)
	{
		SHitInfo v9;

		v9.field_0 = 14;
		v9.field_4 = 11;

		v9.field_C = MechList->mPos - this->mPos;
		this->field_344 = gAttackRelated;
		v9.field_8 = 15;

		if (MechList->Hit(&v9))
		{
			return 1;
		}
	}

	return 0;
}

// @Ok
// @Matching
void CRhino::DieRhino(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			SFX_PlayPos(0x8043, &this->mPos, 0);
			this->field_310 = 0;
			this->Neutralize();
			this->mCBodyFlags &= ~0x10;
			this->field_2A8 |= 0x1000;
			this->field_1F8 = 0;
			this->StopMyXA();
			this->PlaySingleAnim(0x1D, 0, -1);
			this->PlayXAPlease(0x16, 1, 0);
			this->dumbAssPad++;

			MechList->SetIgnoreInputTimer(0x8000);

			{
				CCamera *camera = CameraList;
				if (camera)
				{
					camera->SetMode(CAMERAMODE_DEMO);
					camera->field_100 = 1;
					camera->mTripod = this;
					camera->field_140 = 1;
					camera->field_13C = this;
					camera->SetCamXOffset(0, 0);
					camera->SetCamYOffset(0, 0);
					camera->SetCamZOffset(0, 0);
					camera->SetCamXZDistance(0x1A8, 0);
					camera->SetCamYDistance(-0x68, 0);
				}
			}
			break;
		case 1:
			if (CameraList)
			{
				i16 angle = this->field_80;
				angle <<= 5;
				angle += CameraList->field_236;
				CameraList->SetCamAngle(angle, 16);
			}

			if (this->mAnimFinished)
			{
				this->dumbAssPad = 4;
			}
			break;
		case 3:
			break;
		case 4:
			this->field_1F8 += this->field_80;
			if (this->field_1F8 > 0x3C)
			{
				this->Die(0);
				this->dumbAssPad = 3;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @MEDIUMTODO
void CRhino::DoDazedEffect(void)
{
    printf("CRhino::DoDazedEffect(void)");
}

// @MEDIUMTODO
void CRhino::DoMGSShadow(void)
{
    printf("CRhino::DoMGSShadow(void)");
}

// @Ok
// would love to remove the goto
void CRhino::FollowWaypoints(void)
{
	i32 v6;
	SMoveToInfo v8;
	v8.field_0.vx = 0;
	v8.field_0.vy = 0;
	v8.field_0.vz = 0;

	this->RunAppropriateAnim();

	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->field_310 = 0;

			if (this->field_1F0)
			{
				this->field_2A8 &= ~0x10000000;
				v8.field_0 = this->field_1A8[this->field_1F0];

				v8.field_C = 40;
				v8.field_10 = 40;
				v8.field_14 = 500;

				new CAIProc_MoveTo(this, &v8, 1);
				this->dumbAssPad++;
			}
			else
			{
				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			break;
		case 1:

			if (this->field_288 & 1)
			{
				this->field_288 &= ~1;
				v6 = 1;
			}
			else
			{
				v6 = 0;
			}

			if (v6)
			{
				if (this->field_1F0)
				{
					this->field_1F0--;
					// @FIXME this makes it match but i don't like it
					goto force_match;
				}
				else if (this->GetNextWaypoint())
				{
force_match:
					if ((this->field_2F0 & 2) == 0)
					{
						this->dumbAssPad = 0;
					}
				}
				else
				{
					this->field_31C.bothFlags = 2;
					this->dumbAssPad = 0;
				}

			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
void CRhino::GetLaunched(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->dumbAssPad = 1;
			this->PlaySingleAnim(15, 0, -1);

			new CAIProc_LookAt(this, MechList, 0, 2, 80, 0);
			this->field_230 = Utils_GetValueFromDifficultyLevel(40, 30, 21, 21);

		case 1:
			this->DoPhysics(1);
			this->RunTimer(&this->field_230);
			if (!this->field_1F8)
			{
				this->dumbAssPad++;
				this->mVel.vx = 0;
				this->mVel.vy = 0;
				this->mVel.vz = 0;
			}

			break;
		case 2:
			this->RunTimer(&this->field_230);
			if (!this->field_230)
			{
				if (this->DetermineFightState(1))
				{
					if (this->DistanceToPlayer(0) > 500)
					{
						if (this->field_31C.bothFlags == 5 || this->field_31C.bothFlags == 4)
						{
								this->field_31C.bothFlags = 8;
								this->dumbAssPad = 0;
						}
					}
					else if (this->field_31C.bothFlags == 8)
					{
						this->field_31C.bothFlags = 5;
						this->dumbAssPad = 0;
					}
				}
				else
				{
					this->PlaySingleAnim(0, 0, -1);
					this->field_31C.bothFlags = 22;
					this->dumbAssPad = 0;
				}
			}

			break;
		default:
			print_if_false(0, "Unknown substate");
			break;
	}
}

// @Ok
// @Matching
void CRhino::GetShocked(void)
{
	this->field_3D0 = 0x1E;

	switch (this->dumbAssPad)
	{
		case 0:
			this->mCBodyFlags &= ~0x10;
			this->field_348 |= 2;

			Effects_Electrify(this);
			new CAIProc_StateSwitchSendMessage(this, 0x11);

			if (!this->field_338)
			{
				this->field_338 = SFX_PlayPos(0x80CC, &this->mPos, 0);
			}

			this->CycleAnim(0x1E, 1);

			this->field_34C = Utils_GetValueFromDifficultyLevel(300, 300, 300, 300);
			this->field_420 = (Utils_GetValueFromDifficultyLevel(250, 175, 125, 75) << 12) / this->field_34C;
			this->field_354 = this->mHealth;

			if (Rnd(4))
			{
				this->PlayXAPlease(0xB, 1, 0);
			}
			else
			{
				this->PlayXAPlease(0xC, 3, 0);
			}

			this->dumbAssPad++;
			break;
		case 1:
		{
			this->RunTimer(&this->field_34C);
			this->field_348 |= 2;

			i32 damage = this->GetShockDamage();
			i16 v = (this->field_420 * this->field_34C) >> 0xC;
			v += this->field_354;
			this->mHealth = v - damage;

			if (!this->field_34C)
			{
				this->RunAnim(0x1E, this->field_128, -1);
			}

			if (this->mAnimFinished)
			{
				if (this->field_338)
				{
					SFX_Stop(this->field_338);
				}
				this->field_338 = 0;

				if (this->mHealth <= 0)
				{
					this->field_31C.bothFlags = 0x15;
					this->dumbAssPad = 0;
				}
				else
				{
					this->mCBodyFlags |= 0x10;
					this->CycleAnim(this->field_298.Bytes[0], 1);
					this->PlayXAPlease(6, 3, 1);
					this->field_31C.bothFlags = 2;
					this->dumbAssPad = 0;
				}
			}
			break;
		}
	}
}

// @NotOk
// Logic and field stores verified against the disasm. Residue: case 1's
// "field_1F8 <= 0" branch reads a value through a struct I could not
// identify (Mem_RecoverPointer(&this->field_104), then a double pointer
// indirection at +0x44 then +0x3C off that; modeled as raw char*/i32* casts
// since the real struct/class is unknown). Attempts: (1) direct translation,
// 129 diffs, first divergence was the shared "dumbAssPad++" tail (case 1's
// early-outs and case 4's normal exit both jump to the SAME code in the
// original, 0x480c89) compiling as separate inlined tails in my version;
// (2) added a `goto common_inc;` label after the switch shared by both
// call sites to match the original's actual jump target, which fixed that
// specific cascade but a NEW one appeared at the multiply/shift computation
// order (`this->field_1F8 = 5; this->field_34C = v;` store order vs the
// `(v - field_34C) * 125 * 32 >> 12` computation, 130 diffs now, likely a
// statement-order or intermediate-type issue in that expression I did not
// resolve). Did not reach the 15-hypothesis bar for @AlmostMatching.
void CRhino::GetTrapped(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			new CAIProc_StateSwitchSendMessage(this, 0xE);
			this->RunAnim(0x1A, 0, -1);
			this->field_1F8 = 5;
			this->field_34C = 0;
			this->field_350 = 0;
			this->dumbAssPad++;
			break;
		case 1:
			if (this->mAnimFinished)
			{
				this->CycleAnim(0x1B, 1);
			}

			this->field_348 |= 1;

			if (this->field_350 > 0)
			{
				this->field_350--;
			}

			this->field_1F8--;

			if (this->field_1F8 <= 0)
			{
				void *p = Mem_RecoverPointer(&this->field_104);

				if (!p)
				{
					goto common_inc;
				}

				{
					char *inner = *reinterpret_cast<char**>(static_cast<char*>(p) + 0x44);
					i32 v = *reinterpret_cast<i32*>(inner + 0x3C);

					if (v == this->field_34C)
					{
						goto common_inc;
					}

					this->field_350 += ((v - this->field_34C) * 125 * 32) >> 0xC;
					this->field_1F8 = 5;
					this->field_34C = v;
				}
			}
			break;
		case 2:
			this->RunTimer(&this->field_350);

			if (this->field_350 <= 0)
			{
				this->RunAnim(0x1B, this->mAnim == 0x1B ? this->field_128 : 0, -1);
				this->dumbAssPad++;
			}
			break;
		case 3:
			this->field_348 |= 1;

			if (this->mAnimFinished)
			{
				this->RunAnim(0x1C, 0, -1);
				this->dumbAssPad++;
			}
			break;
		case 4:
			if (this->field_128 < 0xA)
			{
				this->field_348 |= 1;
				break;
			}

			if (this->field_104.pWhatever)
			{
				void *p = Mem_RecoverPointer(&this->field_104);
				if (p)
				{
					reinterpret_cast<CTrapWebEffect*>(p)->Burst();
				}
				this->field_104.pWhatever = 0;
			}

			this->field_31C.bothFlags = 0x11;
			goto common_inc;
		case 5:
			if (this->mAnimFinished)
			{
				if (this->DetermineFightState(1))
				{
					if (this->field_31C.bothFlags == 5 || this->field_31C.bothFlags == 4)
					{
						if (this->DistanceToPlayer(0) > 500)
						{
							this->field_31C.bothFlags = 8;
							this->dumbAssPad = 0;
						}
					}
				}
				else
				{
					this->PlaySingleAnim(0, 0, -1);
					this->field_31C.bothFlags = 0x16;
					this->dumbAssPad = 0;
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
	return;

common_inc:
	this->dumbAssPad++;
}

// @MEDIUMTODO
void CRhino::GonnaHitWall(i32)
{
    printf("CRhino::GonnaHitWall(i32)");
}

// @NotOk
// Real raycast, but not the full original. The original also builds an aim
// matrix (calls at 0x4E7760/0x4E7840/0x470430 near the entry, likely a
// direction/normal setup for the trace) and, if the trace hits something,
// walks past up to 2 hit items whose model checksum (Spool_GetModelChecksum)
// is in a small allow-list at 0x55AD18 (count at 0x55AD5C), retrying the
// trace from the hit point. None of that residue is reproduced here, only
// the core InitLineInfo/LineToItem trace against this->mPos -> *a2. Needed
// as a real (non-inlinable) body so callers like DetermineFightState do not
// get the printf stub const-folded into their own codegen.
u8 CRhino::LineOfSightCheck(CVector const *a2, i32 a3)
{
	SLineInfo lineInfo;

	lineInfo.StartCoords.vx = 0;
	lineInfo.StartCoords.vy = 0;
	lineInfo.StartCoords.vz = 0;
	lineInfo.EndCoords.vx = 0;
	lineInfo.EndCoords.vy = 0;
	lineInfo.EndCoords.vz = 0;

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

	lineInfo.StartCoords = this->mPos;
	lineInfo.EndCoords = *a2;

	M3dColij_InitLineInfo(&lineInfo);
	M3dZone_LineToItem(&lineInfo, a3);

	return lineInfo.pItem == 0;
}

// @MEDIUMTODO
void CRhino::PlaySounds(void)
{
    printf("CRhino::PlaySounds(void)");
}

// @Ok
// @AlmostMatching: slightly diff code gen
void CRhino::PlayXAPlease(
		i32 a2,
		i32 a3,
		i32 a4)
{
	i32 v5 = Rnd(a3 + a4);
	i32 v6 = this->field_3DC;

	if (v5 < a3)
	{
		if (a3 > 1)
		{
			if ( ((v6 >> a2) & (1 << v5)) != 0 && ++v5 >= a3 )
			{
				v5 = 0;
			}

			for (i32 i = 0; i < a3; i++)
			{
				v6 &= ~(1 << (i + a2));
			}

			v6 |= 1 << (v5 + a2);
		}

		i32 v8 = a2 + v5;
		if ( gRhinoData[v8].field_4 )
		{
			if (Redbook_XAPlayPos(
				gRhinoData[v8].field_0,
				gRhinoData[v8].field_2,
				&this->mPos,
				gRhinoData[v8].field_6) )

			{
				this->AttachXA(gRhinoData[v8].field_0, gRhinoData[v8].field_2);
				this->field_3DC = v6;
			}
		}
		else if (MechList->CanITalkRightNow() && Redbook_XAPlayPos(
				gRhinoData[v8].field_0,
				gRhinoData[v8].field_2,
				&MechList->mPos,
				gRhinoData[v8].field_6) )
		{
			MechList->AttachXA(gRhinoData[v8].field_0, gRhinoData[v8].field_2);
			this->field_3DC = v6;
		}
	}
}

// @NotOk
// Best-effort translation, not verified against a build. Builds an aim
// point 14 units back from a2->Normal, computes CalcAim angles toward it,
// turns to face it (CAIProc_LookAt), then spawns a CRhinoWallImpact at the
// hit point. The exact CRhinoWallImpact(SLineInfo*) field usage and the a3
// parameter's role are not confirmed; a3 is not observed used in the disasm
// excerpt this was read from.
void CRhino::SetUpStuckHorn(SLineInfo *a2, i32 a3)
{
	CVector normal(a2->Normal.vx, a2->Normal.vy, a2->Normal.vz);
	CVector aimPoint = this->mPos - (normal * 14);

	CSVector aimAngles;
	Utils_CalcAim(&aimAngles, &this->mPos, &aimPoint);

	CRhinoWallImpact *impact = new CRhinoWallImpact(a2);

	if (impact)
	{
		new CAIProc_LookAt(this, aimAngles.vy, 0, 0x37, 0xC8);
	}

	this->RunAnim(0x18, 0, -1);
	this->field_388 = 0;

	if (gActuatorRelated)
	{
		if (Pad_GetActuatorTime(0, 0) <= 2)
		{
			Pad_ActuatorOn(0, 6, 0, 1);
		}
		if (Pad_GetActuatorTime(0, 1) <= 2)
		{
			Pad_ActuatorOn(0, 0xA, 1, 0xC8);
		}
	}

	SFX_PlayPos(0x804B, &this->mPos, 0);

	this->field_31C.bothFlags = 0xA;
	this->dumbAssPad = 0;
}

// @NotOk
// Best-effort translation, not verified against a build. PathCheck's third
// CVector* out-param and the exact source of the divisors used in the
// ratio computation (guessed as delta.vx / delta.vz here) are uncertain.
void CRhino::SlideFromHit(i32 a2, i32 a3, CVector *a4)
{
	CVector delta = *a4 / a2;
	CVector target = this->mPos + delta;

	CVector unused(0, 0, 0);
	i32 result = this->PathCheck(&this->mPos, &target, &unused, 0x37);

	if (result == 0)
	{
		this->field_1F8 = a3;
	}
	else if (result == 2)
	{
		if (a4->vx || a4->vz)
		{
			i32 signX = a4->vx >> 31;
			i32 absX = (a4->vx ^ signX) - signX;

			i32 signZ = a4->vz >> 31;
			i32 absZ = (a4->vz ^ signZ) - signZ;

			i32 ratio;

			if (absX > absZ)
			{
				ratio = (a4->vx * a3) / delta.vx;
			}
			else
			{
				ratio = (a4->vz * a3) / delta.vz;
			}

			this->field_1F8 = ratio;

			if (ratio > 1)
			{
				this->field_1F8 = ratio + 1;
				this->mVel = delta << this->field_1F8;
				this->field_31C.bothFlags = 0xF;
				this->dumbAssPad = 0;
			}
		}
	}
}

// @NotOk
// Logic and field stores verified against the disasm (barrel-punch loop over
// EnvironmentalObjectList mirrors FuckUpSomeBarrels, the SHitInfo send is the
// same struct/vtable-slot-0xC=Hit idiom as CheckIfPlayerHit). Residue: the
// original keeps the case-3 upper bound (this switch has cases 0-3) alive in
// ebp for the whole function ("mov ebp,3" once at entry) and reuses that same
// register both as the switch bound compare AND later as the literal "3"
// stored into field_31C.bothFlags (case1's else) and this->dumbAssPad
// (case2's else). Writing plain literal 3 in both spots did not make the
// compiler cache it the same way; the cascade from that one instruction is
// most of the diff count. Attempts targeting this: (1) plain literals in
// both spots, 96 diffs; (2) a named local `i32 three = 3;` at the top of the
// function, reused at both stores instead of the literal, no change (96
// diffs, identical cascade) - the compiler did not keep it live in a
// register across the switch. Did not reach the 15-hypothesis bar for
// @AlmostMatching.
void CRhino::StompGround(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->field_324 = 0;
			this->field_328 = 0;
			this->PlayXAPlease(0x13, 1, 1);
			this->PlaySingleAnim(0x13, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->field_128 < 0x11)
			{
				return;
			}

			this->ShakePad();
			SFX_PlayPos(0x804B, &this->mPos, 0);
			CameraList->Shake(this->mPos, CAMERASHAKE_BIG);
			Effects_RhinoStomp(this);
			this->dumbAssPad++;

			if (this->field_31C.bothFlags == 0x14)
			{
				i32 barrels = 0;

				for (
						CBody *cur = EnvironmentalObjectList;
						cur && barrels < 2;
						cur = reinterpret_cast<CBody*>(cur->mNextItem))
				{
					if (cur->mType == 401)
					{
						if (Utils_CrapDist(this->mPos, cur->mPos) < 0x2BC && cur != MechList->mHeldObject)
						{
							reinterpret_cast<CBaddy*>(cur)->PlayerIsVisible();
							barrels++;
						}
					}
				}
			}
			else
			{
				if (MechList->field_AD4 && (MechList->field_8E8 || MechList->field_8E9))
				{
					SHitInfo hit;
					hit.field_C.vx = 0;
					hit.field_C.vy = 0;
					hit.field_C.vz = 0;
					hit.field_0 = 6;
					hit.field_4 = 0xD;
					hit.field_8 = 0x1E;

					MechList->Hit(&hit);
					MechList->KnockSpideyFromCrawlPosition();
				}

				this->dumbAssPad = 3;
			}
			break;
		case 2:
			if (this->mAnimFinished)
			{
				if (this->field_31C.bothFlags == 0x14)
				{
					this->field_31C.bothFlags = 2;
				}
				else
				{
					this->field_31C.bothFlags = 3;
				}
				this->dumbAssPad = 0;
			}
			break;
		case 3:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
// @Matching
void CRhino::StuckInWall(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_348 |= 2;
			this->field_218 &= ~8;
			new CAIProc_StateSwitchSendMessage(this, 0x0C);
			this->dumbAssPad++;
		case 1:
			if (this->field_288 & 1)
			{
				this->field_288 &= ~1;
				this->field_230 = MechList->field_E18 ? 900 : Utils_GetValueFromDifficultyLevel(200, 150, 120, 90);
				this->dumbAssPad++;
			}
			break;
		case 2:
			this->RunTimer(&this->field_230);
			if (this->field_230 > 0x3C)
			{
				if (!MechList->field_E18)
				{
					this->field_230 = 0x3C;
				}
			}

			this->field_348 |= 2;
			if (this->mAnimFinished)
			{
				if (this->field_230)
				{
					this->PlaySingleAnim(0x16, 0, -1);
				}
				else
				{
					this->PlaySingleAnim(0x19, 0, -1);
					this->dumbAssPad++;
					this->field_31C.bothFlags = 0xB;
					SFX_PlayPos(0x8048, &this->mPos, 0);

					if (this->field_218 & 8)
					{
						this->PlayXAPlease(9, 2, 1);
					}
				}
			}
			break;
		case 3:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 2;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
CRhinoNasalSteam::~CRhinoNasalSteam(void)
{
}

// @Ok
// @Matching
void Rhino_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 307)
			delete pSearch;

		pSearch = pNext;
	}
}


// @NotOk
// Logic and field stores match (verified against the disasm instruction by
// instruction). Residue: this->mPlayerDist is declared u16 in ob.h (CBody,
// offset 0xE4), but the original reads/compares it with the full 32-bit eax
// register ("mov eax,[esi+0E4h]" then "cmp eax,1388h"/"cmp eax,0C8h" etc,
// not movzx+16-bit ops), which only happens if the real field is 32-bit.
// mPlayerDist is shared CBody state used elsewhere (ob.cpp, powerup.cpp), so
// I did not change its type from this single file. That single 16-vs-32-bit
// mismatch changes instruction encoding size and cascades into every jump
// target after it in this function. A second, smaller residue: the shared
// "return 0" epilogue in the original (used by 4 different early-return
// sites) compiles here as separate inlined epilogues per site. Attempts: (1)
// initial translation matched field order but stack frame was 0xC too big
// (SMoveToInfo local instead of a plain CVector, fixed); (2) LineOfSightCheck
// was still a printf stub and got inlined into this function, masking all
// downstream codegen (fixed by giving it a real, if incomplete, body, see
// its own tag); (3) swapped the LineOfSightCheck if/else branch order to
// match the original's fallthrough-is-false layout (fixed one cluster); (4)
// manual sar/xor/sub abs() instead of the cstdlib abs() call, to match the
// original's idiom instead of the cdq-based intrinsic expansion (fixed).
i32 CRhino::DetermineFightState(i32 a2)
{
	i32 dy = this->mPos.vy - MechList->mPos.vy;
	i32 originalFlags = this->field_31C.bothFlags;
	i32 sign = dy >> 31;
	i32 absDy = (dy ^ sign) - sign;

	if (absDy > 0x64000)
	{
		if (!this->field_324)
		{
			this->field_324 = Utils_GetValueFromDifficultyLevel(400, 200, 120, 120);
		}
		return 0;
	}

	if (!this->LineOfSightCheck(&MechList->mPos, 1))
	{
		if (this->field_1F0)
		{
			return 0;
		}

		CVector v;
		v.vx = 0;
		v.vy = 0;
		v.vz = 0;
		this->GetWaypointNearTarget(&MechList->mPos, 0x12C000, this->field_21D, &v);
		this->field_21D++;

		if (Utils_CrapXZDist(this->mPos, v) < 0x104)
		{
			return 0;
		}

		if (this->PathCheck(&this->mPos, &v, 0, 100))
		{
			return 0;
		}

		this->field_1A8[1] = v;
		this->field_1F0 = 1;
		this->field_31C.bothFlags = 1;
		this->dumbAssPad = 0;
		this->field_218 |= 0x10;
		return 1;
	}
	else
	{
		if (this->field_218 & 0x10)
		{
			this->PlayXAPlease(0x12, 1, 1);
			this->field_218 &= ~0x10;
		}

		if (gAttackRelated - this->field_358 < 0x96 && MechList->field_AD4)
		{
			this->field_31C.bothFlags = 0xD;
			this->dumbAssPad = 0;
		}
		else if (this->mPlayerDist < 0x1388)
		{
			if (MechList->field_E1C & 0x80)
			{
				return 0;
			}

			if (MechList->field_E1C & 0x800000)
			{
				if (this->mPlayerDist < 0xC8)
				{
					return 0;
				}
				this->field_31C.bothFlags = 5;
			}
			else if (this->mPlayerDist < 0xC8)
			{
				this->field_31C.bothFlags = 6;
			}
			else if (this->mPlayerDist < 0x1F4)
			{
				this->field_31C.bothFlags = 5;
			}
			else
			{
				this->field_31C.bothFlags = a2 ? 8 : 7;
			}

			this->dumbAssPad = 0;
		}
	}

	if (originalFlags != this->field_31C.bothFlags)
	{
		this->Baddy_SendSignal();
	}
	return originalFlags != this->field_31C.bothFlags;
}

// @Ok
void CRhino::TakeHit(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_310 = 0;
			new CAIProc_LookAt(this, MechList, 0, 0, 80, 200);
			this->PlaySingleAnim(0xFu, 0, -1);
			this->field_230 = Utils_GetValueFromDifficultyLevel(40, 30, 21, 21);
			this->dumbAssPad++;
			break;
		case 1:
			this->RunTimer(&this->field_230);
			if (!this->field_230)
			{
				if (this->DetermineFightState(1))
				{
					if (this->DistanceToPlayer(0) > 500)
					{
						if (this->field_31C.bothFlags == 5 || this->field_31C.bothFlags == 4)
						{
							this->field_31C.bothFlags = 8;
							this->dumbAssPad = 0;
						}
					}
					else if (this->field_31C.bothFlags == 8)
					{
						this->field_31C.bothFlags = 5;
						this->dumbAssPad = 0;
					}
				}
				else
				{
					this->PlaySingleAnim(0, 0, -1);
					this->field_31C.bothFlags = 22;
					this->dumbAssPad = 0;
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
void CRhino::HitWall(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->ShakePad();
			CameraList->Shake(this->mPos, CAMERASHAKE_BIG);
			this->Neutralize();
			this->mCBodyFlags &= ~0x10;
			this->PlaySingleAnim(17, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->mAnimFinished)
			{
				if ( this->mHealth <= 0 )
				{
					this->field_31C.bothFlags = 21;
					this->dumbAssPad = 0;
				}
				else
				{
					this->PlaySingleAnim(0x12u, 0, -1);
				}
			}
			break;
		case 2:

			if ( this->mAnimFinished )
			{
				if ( this->mAnim == 18 )
				{
					this->mAngles.vy = (this->mAngles.vy - 2048) & 0xFFF;
					this->PlaySingleAnim(0x15u, 0, -1);
				}
				else
				{
					this->mCBodyFlags |= 0x10u;
					this->PlaySingleAnim(0, 0, -1);
	
					this->field_31C.bothFlags = 2;
					this->dumbAssPad = 0;
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @NotOk
// figure out types of fields that call destructors
CRhino::~CRhino(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
	Panel_DestroyHealthBar();

	if (this->field_3E0)
		delete reinterpret_cast<CItem*>(this->field_3E0);

	for (i32 i = 0; i < 5; i++)
	{
		if (this->field_3E4[i])
			delete reinterpret_cast<CItem*>(this->field_3E4[i]);
		this->field_3E4[i] = 0;

		if (this->field_3F8[i])
			delete reinterpret_cast<CItem*>(this->field_3F8[i]);
		this->field_3F8[i] = 0;

		if (this->field_40C[i])
			delete reinterpret_cast<CItem*>(this->field_40C[i]);
		this->field_40C[i] = 0;
	}

	gBossRelated = 0;
}

// @Ok
void CRhino::Laugh(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_230 = 100;
			this->PlaySingleAnim(20, 0, -1);
			SFX_PlayPos(((gAttackRelated & 1) == 0 ? 1 : 0) | 0x8046, &this->mPos, 0);
			this->dumbAssPad++;

			break;
		case 1:
			this->RunTimer(&this->field_230);
			if (this->mAnimFinished)
			{
				if (!this->field_230)
				{
					this->field_31C.bothFlags = 2;
					this->dumbAssPad = 0;
				}
				else
				{
					this->PlaySingleAnim(0, 0, -1);
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate.");
			break;
	}
}

// @Ok
void CRhino::RhinoInit(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_358 = gAttackRelated - 155;
			i32 GroundHeight;
			GroundHeight = Utils_GetGroundHeight(&this->mPos, 300, 300, 0);
			if ( GroundHeight != -1 )
			{
				this->mPos.vy = GroundHeight - (this->field_21E << 12);
				this->field_29C = this->mPos.vy;
				this->field_2A0 = GroundHeight;
				this->dumbAssPad++;

				this->PlaySingleAnim(0, 0, -1);
			}
			break;
		case 1:
			this->field_31C.bothFlags = 8;
			this->dumbAssPad = 0;
			break;
		default:
			print_if_false(0, "Unknown sub-state!");
			break;
	}
}

// @NotOk
// understand if that's really PlayerIsVisible call
void CRhino::FuckUpSomeBarrels(void)
{
	i32 barrels = 0;

	for (
			CBody* cur = EnvironmentalObjectList;
			cur && barrels < 2;
			cur = reinterpret_cast<CBody*>(cur->mNextItem))
	{
		if (cur->mType == 401)
		{
			if (Utils_CrapDist(this->mPos, cur->mPos) < 0x2BC && cur != MechList->mHeldObject)
			{
				reinterpret_cast<CBaddy*>(cur)->PlayerIsVisible();
				barrels++;
			}
		}
	}
}

// @Ok
void CRhino::StandStill(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->dumbAssPad++;
		case 1:
			if (this->mAnim)
				this->PlaySingleAnim(0, 0, -1);
			break;
		default:
			print_if_false(0, "Unknown substate.");
			break;
	}
}

// @Ok
INLINE void CRhino::ShakePad(void)
{
	if ( gActuatorRelated )
	{
		if ( Pad_GetActuatorTime(0, 0) <= 2u )
			Pad_ActuatorOn(0, 6u, 0, 1u);
		if ( Pad_GetActuatorTime(0, 1u) <= 2u )
			Pad_ActuatorOn(0, 0xAu, 1, 0xC8u);
	}
}

// @NotOk
// validate when get shocked
i32 CRhino::GetShockDamage(void)
{
	switch ( DifficultyLevel )
	{
		case 0:
		case 1:
			return 175;
		case 2:
			return 125;
		case 3:
			return 75;
		default:
			print_if_false(0, "Unknown difficulty level!");
			return 0;
	}
}

// @NotOk
// validate when playsounds is done
u32 CRhino::GetNextFootstepSFX(void)
{
	u32 res;
	for (res = (Rnd(3) + 76) | 0x8000; res == gRhinoSound; res = (Rnd(3) + 76) | 0x8000)
		;

	return res;
}

// @Ok
INLINE void CRhino::PlaySingleAnim(u32 a2, i32 a3, i32 a4)
{
	this->field_388 = 0;
	this->RunAnim(a2, a3, a4);
}

// @NotOk
// globals
CRhino::CRhino(i16* a2, i32 a3)
{
	i16 *v5 = this->SquirtAngles(reinterpret_cast<i16*>(this->SquirtPos(a2)));
	this->InitItem("rhino");

	this->mFlags |= 0x480;
	// @FIXME
	this->mpLight = &M3d_RhinoLight;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->field_21E = 100;
	this->field_1F4 = a3;
	this->mNode = a3;
	this->mRMinor = 175;

	this->field_230 = 0;
	this->field_216 = 32;

	this->mPushVal = 64;
	this->field_31C.bothFlags = 0;
	this->field_2A8 |= 1;
	this->field_2A8 |= 0x200;
	this->field_2A8 |= 0x2000000;

	this->mType = 307;
	this->mHealth = Utils_GetValueFromDifficultyLevel(1400, 1400, 1400, 1400);

	this->field_294.Int = gRhinoStrangeInitData[0];
	this->field_298.Int = gRhinoStrangeInitData[1];

	this->field_344 = gAttackRelated - 240;

	for (i32 i = 0; i < LEN_RHINO_DAZED_DATA; i++)
	{
		gRhinoDazedData[i] = Rnd(4096);
	}

	for (i32 j = 0; j < LEN_RHINO_DATA; j++)
	{
		if (gRhinoData[j].field_8 != j)
			DoAssert(0, "Fire Matt, he fucked up the rhino XA.  Actually, kick him in the nuts first.");
	}

	this->ParseScript(reinterpret_cast<u16*>(v5));
	Panel_CreateHealthBar(this, 307);
}

// @NotOk
// globals
CRhino::CRhino(void)
{
	this->InitItem("rhino");
	this->mFlags |= 0x480;
	// @FIXME
	this->mpLight = &M3d_RhinoLight;
	this->mType = 307;
}

// @Ok
void Rhino_CreateRhino(const u32 *stack, u32 *result)
{
	i16* v2 = reinterpret_cast<i16*>(*stack);
	i32 v3 = static_cast<i32>(stack[1]);

	if (v2)
	{
		*result = reinterpret_cast<u32>(new CRhino(v2, v3));
	}
	else
	{
		*result = reinterpret_cast<u32>(new CRhino());
	}
}


// @Ok
CRhinoNasalSteam::CRhinoNasalSteam(CVector* a2, CVector* a3)
{
	this->mPos = *a2;
	this->mVel = *a3;

	this->SetAnim(1);
	this->SetSemiTransparent();
	this->SetTransparency(64);
	this->SetAnimSpeed(128);
	this->SetScale(128);
	this->mAngle = Rnd(4096);
}

// @Ok
// minor decomp diff
void CRhinoNasalSteam::Move(void)
{

	i16 mAnimSpeed = this->mAnimSpeed;

	if (mAnimSpeed)
	{
		u16 v3 = (this->mFrame << 8) | this->mFrameFrac;
		u16 v4 = mAnimSpeed + v3;

		this->mFrameFrac = v4;
		v4 >>= 8;
		this->mFrame = v4;

		if ( (char)v4 >= (i32)this->mNumFrames)
		{
			this->mAnimSpeed = 0;
			this->mFrame = this->mNumFrames - 1;
		}

		i32 index = this->mFrame;
		this->mpPSXFrame = &this->mpPSXAnim[index];
	}

	this->mPos += this->mVel;

	bool v7 = ++this->mAge <= 30;
	this->mVel.vy -= 1024;

	if (!v7 )
	{
		this->Die();
	}
	else
	{
		this->SetTransparency(64 - 2 * (this->mAge & 0xFF));
		this->SetScale(Rnd(4) + 4 * (this->mAge + 32));
	}
}

void validate_CRhino(void){
	VALIDATE_SIZE(CRhino, 0x424);

	VALIDATE(CRhino, field_324, 0x324);
	VALIDATE(CRhino, field_328, 0x328);
	VALIDATE(CRhino, field_330, 0x330);

	VALIDATE(CRhino, field_338, 0x338);

	VALIDATE(CRhino, field_344, 0x344);
	VALIDATE(CRhino, field_348, 0x348);
	VALIDATE(CRhino, field_34C, 0x34C);
	VALIDATE(CRhino, field_350, 0x350);
	VALIDATE(CRhino, field_354, 0x354);

	VALIDATE(CRhino, field_358, 0x358);
	VALIDATE(CRhino, field_360, 0x360);
	VALIDATE(CRhino, field_388, 0x388);

	VALIDATE(CRhino, field_3D0, 0x3D0);

	VALIDATE(CRhino, field_3DC, 0x3DC);
	VALIDATE(CRhino, field_3E0, 0x3E0);
	VALIDATE(CRhino, field_3E4, 0x3E4);
	VALIDATE(CRhino, field_3F8, 0x3F8);
	VALIDATE(CRhino, field_40C, 0x40C);
	VALIDATE(CRhino, field_420, 0x420);
}

void validate_CRhinoNasalSteam(void)
{
	VALIDATE_SIZE(CRhinoNasalSteam, 0x68);
}

void validate_SRhinoData(void)
{
	VALIDATE_SIZE(SRhinoData, 0xC);

	VALIDATE(SRhinoData, field_0, 0x0);
	VALIDATE(SRhinoData, field_2, 0x2);

	VALIDATE(SRhinoData, field_4, 0x4);
	VALIDATE(SRhinoData, field_6, 0x6);

	VALIDATE(SRhinoData, field_8, 0x8);
}
