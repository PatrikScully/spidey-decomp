#include "lizman.h"
#include "validate.h"
#include "message.h"
#include "ps2funcs.h"
#include "mem.h"
#include "ai.h"
#include "ps2lowsfx.h"
#include "utils.h"
#include "trig.h"
#include "ps2funcs.h"
#include "spidey.h"
#include "m3dutils.h"
#include <cmath>

// data seen at 0x552048/0x55204C in the original, used to set up
// field_294/field_298. Same pattern as gJonahSetup/gRhinoStrangeInitData.
EXPORT i32 gLizManSetup[2] = { 0x2020201, 0 };

// @NotOk
// NOT AlmostMatching yet: this is a medium function (441 bytes), the repo
// discipline requires at least 15 distinct hypotheses before that tag is
// earned, only 9 were tried. Residue: the original shares one epilogue
// between the if/else branches (if-branch jumps to it, else-branch falls
// into it); this build duplicates the epilogue in both branches instead
// (23 extra bytes, 464 vs 441). Everything else matches instruction for
// instruction, including exact stack offsets for both reloaded
// constructor args. 9 source variants tried targeting this specific issue
// (branch order, write order inside the branch, temps, early return,
// goto, switch, removing an intermediate local) with no change; details
// in CLizMan_CLizMan.attempts.md. Needs 6+ more hypotheses.
CLizMan::CLizMan(i16* a1, i32 a2)
{
	i32 levelId = Trig_GetLevelID();

	if (levelId == 0x503 || levelId == 0x504 || levelId == 0x601 || levelId == 0x602)
		this->InitItem("lizman2");
	else
		this->InitItem("lizman");

	this->field_328 = levelId;
	i16* q = this->SquirtAngles(this->SquirtPos(a1));

	this->mType = 317;
	this->field_21E = 100;

	this->field_294.Int = gLizManSetup[0];
	this->field_298.Int = gLizManSetup[1];

	M3dUtils_ReadHooksPacket(this, const_cast<char*>(""));

	this->field_338 = 0x1000;
	this->field_32C = gAttackRelated;
	this->ShadowOn();

	this->mShadowScale = 0x30;
	this->field_3AC = 0x21;

	// @FIXME field_F4 is declared i32 (ob.h), but the original only stores
	// the low 16 bits here. Force a word store to match without touching
	// the shared header.
	*reinterpret_cast<i16*>(&this->field_F4) = 0x40;

	this->field_374 = gTimerRelated - 0x131;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->field_1F4 = a2;
	this->mNode = static_cast<u16>(a2);

	this->mRMinor = 0x80;
	this->field_230 = 0;
	this->field_216 = 0x20;
	this->mPushVal = 0x40;
	this->field_31C.bothFlags = 0;

	this->ParseScript(reinterpret_cast<u16*>(q));

	if (levelId == 0x505)
	{
		this->field_2A8 |= 0x2000000;
		this->field_212 = 0xF;
		this->field_398 = 0x32000;
		this->field_218 |= 0x8000;
	}
	else
	{
		this->field_398 = 0x3C000;
		this->field_212 = 0x1E;
	}
}

// @Ok
// @Matching
void LizMan_CreateLizMan(const u32* stack, u32* result)
{
	i16* v2 = reinterpret_cast<i16*>(stack[0]);
	u32 v3 = stack[1];

	*result = reinterpret_cast<u32>(new CLizMan(v2, v3));
}

// @Ok
// @Matching
void LizMan_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 317)
			delete pSearch;

		pSearch = pNext;
	}
}

// @Ok
// @Matching
void LizMan_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = LizMan_RelocatableModuleClear;
	pMod->field_C[0] = LizMan_CreateLizMan;
}

// @Ok
// @AlmostMatching: the two register-zeroing instructions right before the
// loop (index register, angle register) come out in swapped order. Same
// mnemonics, same length, only the register operand differs. Tried 15
// source variants (for/while/do-while, declaration order and position,
// split vs combined init, increment order, comparison direction, signed
// vs unsigned index, mask operand order) with no change. Rest of the
// function matches instruction for instruction.
void CLizMan::CalculateJumpPositionArray(CVector* pTarget)
{
	i32 groundHeight = Utils_GetGroundHeight(pTarget, 0, 0x200, 0);

	i32 xDelta = (pTarget->vx - this->mPos.vx) / 48;
	i32 yDelta = (groundHeight - (this->field_21E << 12) - this->mPos.vy) / 48;
	i32 zDelta = (pTarget->vz - this->mPos.vz) / 48;

	if (this->field_3B4)
		Mem_Delete(this->field_3B4);

	this->field_3B4 = static_cast<CVector*>(DCMem_New(0x240, 0, 1, 0, 1));

	i32 x = this->mPos.vx + xDelta;
	i32 y = this->mPos.vy + yDelta;
	i32 z = this->mPos.vz + zDelta;

	i32 i = 0;
	i32 t = 0;
	do
	{
		this->field_3B4[i].vx = x;
		this->field_3B4[i].vy = y - (rcossin_tbl[t & 0xFFF].sin << 8);
		this->field_3B4[i].vz = z;

		x += xDelta;
		y += yDelta;
		z += zDelta;
		t += 0x2B;
		i++;
	} while (t < 0x810);

	this->field_3B0 = 0;
}

// @BIGTODO
i32 CLizMan::ScanNearbyNodesForJumpTarget(void)
{
	return 0x17062024;
}

extern CPlayer* MechList;
static u16 word_5FBC0C;

// @Ok
void CLizMan::Guard(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_1F8 = 0;
			this->Neutralize();

			if (gTimerRelated - this->field_374 < 0x12C)
			{
				new CAIProc_LookAt(
						this,
						MechList,
						0,
						0,
						128,
						200);
			}

			this->dumbAssPad++;
			break;
		case 1:

			// @FIXME - word??
			if (word_5FBC0C != 0xFFFF && this->mAnim != 5)
			{
				this->PlaySingleAnim(5, 0, -1);
			}

			this->field_1F8 += this->field_80;
			if (this->field_1F8 > 120)
			{
				this->field_1F8 = 0;
				i32 target = this->ScanNearbyNodesForJumpTarget();
				if (target)
				{
					CVector v6;
					v6.vx = 0;
					v6.vy = 0;
					v6.vz = 0;
					this->field_1F0 = 0;

					SFX_PlayPos((Rnd(2) + 134) | 0x80, &this->mPos, 0);
					Trig_GetPosition(&v6, target);
					this->field_1F4 = target;

					this->CalculateJumpPositionArray(&v6);
					this->field_31C.bothFlags = 23;
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
void CLizMan::Acknowledge(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->field_340 = 0;
			this->RunAnim(0, 0, -1);

			if (Mem_RecoverPointer(&this->hLizHandle))
			{
				new CAIProc_LookAt(
						this,
						reinterpret_cast<CBody*>(this->hLizHandle.pWhatever),
						0,
						2,
						70,
						200);
			}

			this->dumbAssPad++;
			break;
		case 1:

			if(this->mAnimFinished)
			{
				this->field_31C.bothFlags = 25;
				this->dumbAssPad = 0;
			}
			
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// @Ok
void CLizMan::SwitchFromEulerToMatrix(void)
{
	if ((this->outlineRelated & 1) == 0)
	{
		this->outlineRelated |= 1;
		M3dMaths_RotMatrixYXZ(
				reinterpret_cast<SVECTOR*>(&this->mAngles),
				&this->mTransform);
	}
}

// @BIGTODO
void CLizMan::RunToWhereActionIs(CVector*)
{}

// @Ok
void INLINE CLizMan::HelpOutBuddy(CMessage* pMessage)
{
	if (!this->field_390)
	{
		if (this->field_31C.bothFlags == 2 || this->field_31C.bothFlags == 1)
		{
			CItem *pItem = reinterpret_cast<CItem*>(
					Mem_RecoverPointer(&pMessage->mHandle));

			if (pItem)
				this->RunToWhereActionIs(&pItem->mPos);
		}
	}

}

// @Ok
void INLINE CLizMan::PlaySingleAnim(i16 a1, i32 a2, i32 a3)
{
	this->field_340 = 0;
	this->RunAnim(a1, a2, a3);
}

// @Ok
void INLINE CLizMan::StandStill(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->PlaySingleAnim(19, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			print_if_false(0, "Unknown substate.");
			break;
	}
}

// @Ok
void CLizMan::SwitchFromMatrixToEuler(void)
{
	if ( this->outlineRelated & 1)
	{
		this->outlineRelated &= 0xFFFFFFFE;
		this->mAngles.vz = 0;
		this->mAngles.vx = 0;
		this->mAngles.vy = (3072 - ratan2(-this->mTransform.m[2][2], -this->mTransform.m[0][2])) & 0xFFF;
	}
}

// @Ok
void CLizMan::StopClimbing(void)
{
	this->SwitchFromMatrixToEuler();
	this->field_390 = 0;
}

// @NotOk
// residue: case 0 matches instruction for instruction. The remaining gap is
// that the original shares one small tail (field_31C.bothFlags=0x19;
// dumbAssPad=0;) between case 1's mAnim==0x15 branch and case 5 via a plain
// jump into the middle of case 5's block; this build always duplicates that
// tail instead of sharing it (confirmed by export size: 1056 bytes built vs
// 992 original), no matter how it is written (plain duplication, goto to a
// label inside case 5). Same class of issue as the duplicated epilogue in
// CLizMan::CLizMan. Full details and every attempt tried in
// CLizMan_FlyAcrossRoom.attempts.md.
void CLizMan::FlyAcrossRoom(void)
{
	switch(this->dumbAssPad)
	{
		case 0:
		{
			this->field_394 |= 2;
			this->ClearAttackFlags();

			this->field_310 = 0;
			if (this->field_318 == 1 || this->field_318 == 2)
				this->mRMinor = 0;

			i32 maxVel = (this->mVel.vx > this->mVel.vz) ? this->mVel.vx : this->mVel.vz;
			i32 velProduct = this->field_1F8 * maxVel;
			i32 velProductSign = velProduct >> 31;
			if ((velProduct ^ velProductSign) - velProductSign < 0x100000)
			{
				i32 animId;
				if (this->field_218 & 0x400)
					animId = 0x22;
				else
					animId = (this->field_218 & 0x800) ? 0x21 : 0xC;
				this->field_340 = 0;
				this->RunAnim(animId, 0, -1);
			}
			else
			{
				this->field_340 = 0;
				this->RunAnim(0x15, 0, -1);
			}

			if (this->field_1F8 == 0x29A)
				this->field_1F8 = 0x14;

			if (this->mHealth <= 0)
			{
				this->dumbAssPad = 10;
				this->field_394 |= 1;
			}
			else
			{
				this->dumbAssPad++;
			}
			break;
		}
		case 1:
			this->field_394 |= 2;
			this->DoLizmanPhysics();

			if (this->field_1F8 > this->field_80)
			{
				this->field_1F8 -= this->field_80;
			}
			else
			{
				this->field_1F8 = 0;
				this->mVel.vx = 0;
				this->mVel.vy = 0;
				this->mVel.vz = 0;
				this->mFric.vx = 1;
				this->mFric.vy = 1;
				this->mFric.vz = 1;

				if (this->ShouldFall(0xC8, this->field_398))
				{
					this->field_31C.bothFlags = 0x12;
					this->field_218 &= ~2;
					this->dumbAssPad = 0;
				}
				else
				{
					this->SetHeight(1, 0x64, 0x258);

					if (this->field_318 == 1 || this->field_318 == 2)
					{
						this->CheckFallBack();
						if (this->field_2A8 & 0x10)
						{
						}
						this->PlaySingleAnim(0xE, 0, -1);
						this->dumbAssPad++;
					}
					else if (this->mAnim == 0x15)
					{
						this->PlaySingleAnim(5, 0, -1);
						this->field_31C.bothFlags = 25;
						this->dumbAssPad = 0;
					}
					else
					{
						this->dumbAssPad = 5;
					}
				}
			}
			break;
		case 2:
			this->SetHeight(0, 0x64, 0x258);
			if (word_5FBC0C != 0xFFFF)
			{
				this->mRMinor = 0x80;
				this->dumbAssPad++;
			}
			break;
		case 3:
			if (this->SetHeight(0, 0x64, 0x258) == 2 && word_5FBC0C != 0xFFFF)
			{
				if (this->IsSafeToSwitchToFollowWaypoints())
				{
					this->field_31C.bothFlags = 2;
				}
				else
				{
					this->field_31C.bothFlags = 1;
				}
				this->dumbAssPad = 0;
			}
			break;
		case 5:
			if (this->SetHeight(0, 0x64, 0x258) == 2 && word_5FBC0C != 0xFFFF)
			{
				this->field_31C.bothFlags = 25;
				this->dumbAssPad = 0;
			}
			break;
		case 10:
			this->field_394 |= 3;
			this->DoLizmanPhysics();

			if (this->field_218 & 0x1000)
			{
				i16 speed = *reinterpret_cast<i16*>(&this->field_80);
				this->mAngles.vx += this->field_330 * speed;
				this->mAngles.vy += this->field_334 * speed;
			}

			if (this->field_1F8 > this->field_80)
			{
				this->field_1F8 -= this->field_80;
			}
			else
			{
				this->field_1F8 = 0;
				this->mVel.vx = 0;
				this->mVel.vy = 0;
				this->mVel.vz = 0;
				this->mFric.vx = 1;
				this->mFric.vy = 1;
				this->mFric.vz = 1;

				if (this->ShouldFall(0xC8, this->field_398))
				{
					this->field_31C.bothFlags = 0x12;
					this->field_218 &= ~2;
					this->dumbAssPad = 0;
				}
				else
				{
					this->Neutralize();
					this->SetHeight(1, 0x64, 0x258);
					this->field_31C.bothFlags = 0x14;
					this->dumbAssPad = 0;
				}
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

// FlyAcrossRoom above calls these; MSVC inlines a same-TU stub call even
// when it is defined later in the file, which pollutes FlyAcrossRoom's
// codegen (leaf-first rule). Keep the MSVC inliner away, same fix as
// gsub_5027A0 in DXinit.cpp.
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// @MEDIUMTODO
void CLizMan::DoLizmanPhysics(void)
{
	printf("CLizMan::DoLizmanPhysics(void)");
}

// @SMALLTODO
void CLizMan::CheckFallBack(void)
{
	printf("CLizMan::CheckFallBack(void)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// @Ok
i32 INLINE CLizMan::IsSafeToSwitchToFollowWaypoints(void)
{
	if (this->field_1F0 || this->field_1F4 > 0)
		return 1;
	return 0;
}

static CLizMan* gGlobalLizMan;
static unsigned char gLizManAttackFlag;

// @NotOk
// globals
void INLINE CLizMan::ClearAttackFlags(void)
{
	if (gGlobalLizMan == this)
	{
		gGlobalLizMan = NULL;
	}
	else if ((this->field_39C & 2))
	{
		gLizManAttackFlag &= ~this->field_39D;
	}

	this->field_39C = 0;
	this->field_39D = 0;
}

void validate_CLizMan(void){
	VALIDATE_SIZE(CLizMan, 0x3B8);


	VALIDATE(CLizMan, field_328, 0x328);
	VALIDATE(CLizMan, field_32C, 0x32C);
	VALIDATE(CLizMan, field_330, 0x330);
	VALIDATE(CLizMan, field_334, 0x334);
	VALIDATE(CLizMan, field_338, 0x338);
	VALIDATE(CLizMan, field_340, 0x340);

	VALIDATE(CLizMan, hLizHandle, 0x36C);

	VALIDATE(CLizMan, field_374, 0x374);
	VALIDATE(CLizMan, field_390, 0x390);
	VALIDATE(CLizMan, field_394, 0x394);
	VALIDATE(CLizMan, field_398, 0x398);

	VALIDATE(CLizMan, field_39C, 0x39C);
	VALIDATE(CLizMan, field_39D, 0x39D);

	VALIDATE(CLizMan, field_3AC, 0x3AC);
	VALIDATE(CLizMan, field_3B0, 0x3B0);
	VALIDATE(CLizMan, field_3B4, 0x3B4);
}
