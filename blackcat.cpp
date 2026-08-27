#include "blackcat.h"
#include "validate.h"
#include "trig.h"
#include "m3dutils.h"
#include "utils.h"
#include "ps2m3d.h"
#include "m3dcolij.h"
#include "ps2lowsfx.h"
#include "spidey.h"

extern u8 submarinerDieRelated;
extern CBaddy* BaddyList;
extern CPlayer* MechList;

EXPORT SLight M3d_BlackCatLight =
{
  { { -2047, -2896, -2047 }, { 0, 0, 4096 }, { 0, 0, -4096 } },
  0,
  { { 1520, 2000, 1840 }, { 1440, 1920, 1760 }, { 1440, 1920, 1760 } },
  0,
  { 1760, 1600, 1600 }
};


// @Ok
// @Matching
void BlackCat_RelocatableModuleInit(reloc_mod* pMod)
{
	pMod->mClearFunc = BlackCat_RelocatableModuleClear;
	pMod->field_C[0] = BlackCat_CreateBlackCat;
}

// @Ok
void BlackCat_RelocatableModuleClear(void)
{
	for (CBody* cur = BaddyList; cur; )
	{
		CBody* next = reinterpret_cast<CBody*>(cur->mNextItem);
		if (cur->mType == 319)
		{
			delete cur;
		}

		cur = next;
	}
}

// @NotOk
// state machine: field_31C.bothFlags picks the outer phase (1 = climb down
// from a ledge, 2 = walk to the player using SynthesizeAnalogueInput, 4 =
// idle anim), dumbAssPad is the sub state inside each phase. field_324 is a
// trig link id to look at, field_188 is a guessed turret-style head object
// with a pan/tilt pair at +0x24 (type unknown, offset used raw).
// cmpsum: 289 mnemonic diffs on a 1146 byte function, first divergence near
// entry (my code loads the al flag differently than the original's cmp
// al,bl idiom). functional shape (calls, order, branches) verified against
// the disasm but not iterated to a match.
void CBlackCat::AI(void)
{
	if (submarinerDieRelated)
	{
		if (Trig_GetLevelID() != 0x803)
		{
			this->Die(0);
			return;
		}
	}

	if (this->mAnim == 8)
	{
		if (!(this->field_218 & 1) && this->field_128 >= 0x12)
		{
			SFX_PlayPos(0x819B, &this->mPos, 0);
			this->field_218 |= 1;
		}
	}
	else
	{
		this->field_218 &= ~1;
	}

	this->DoPhysics();

	if (this->pMessage)
	{
		this->CleanUpMessages(1, 0);
	}

	switch (this->field_31C.bothFlags)
	{
		case 1:
			switch (this->dumbAssPad)
			{
				case 0:
				{
					CVector aimTarget;
					aimTarget.vx = MechList->mPos.vx;
					aimTarget.vy = this->mPos.vy;
					aimTarget.vz = MechList->mPos.vz;
					Utils_CalcAim(&this->mAngles, &this->mPos, &aimTarget);

					i32 groundHeight = Utils_GetGroundHeight(&this->mPos, 0, 0x800, 0);
					if (groundHeight != -1)
					{
						this->field_32C = 0x2000;
						this->realRegisterArr[0] = groundHeight >> 12;
						this->dumbAssPad++;
					}
					break;
				}
				case 1:
				{
					print_if_false(1, "CBlackCat::AI climb down");
					i32 landingY = (this->realRegisterArr[0] - this->field_21E) << 12;
					if (this->mPos.vy > landingY)
					{
						SFX_PlayPos(0x819A, &this->mPos, 0);
						this->RunAnim(7, 0, -1);
						this->mPos.vy = landingY;
						this->field_32C = 0;
						this->mVel.vx = 0;
						this->mVel.vy = 0;
						this->mVel.vz = 0;
						this->dumbAssPad++;
					}
					break;
				}
				case 2:
					if (this->mAnimFinished)
					{
						this->field_31C.bothFlags = 2;
						this->dumbAssPad = 0;
					}
					break;
				default:
					print_if_false(0, "CBlackCat::AI bad sub state (1)");
					break;
			}
			break;

		case 2:
			switch (this->dumbAssPad)
			{
				case 0:
					this->field_340 = 1;
					this->field_348 = 1;
					this->dumbAssPad = 1;
					// fall through

				case 1:
					if (this->field_340)
					{
						this->SynthesizeAnalogueInput();
					}
					else
					{
						if (Trig_GetLevelID() == 0x803)
						{
							this->field_31C.bothFlags = 4;
							this->dumbAssPad = 0;
						}
						else
						{
							this->Die(0);
						}
					}
					break;

				default:
					print_if_false(0, "CBlackCat::AI bad sub state (2)");
					break;
			}
			break;

		case 4:
			if (this->mAnim != 0 || this->mAnimFinished != 0)
			{
				this->RunAnim(0, 0, -1);
			}
			break;

		default:
			print_if_false(0, "CBlackCat::AI bad state");
			break;
	}

	CSVector lookAngle;

	if (this->field_324)
	{
		VECTOR hookPos;
		M3dUtils_GetHookPosition(&hookPos, this, 0x10);

		CVector trigPos;
		Trig_GetPosition(&trigPos, this->field_324);

		MATRIX localMat;
		M3dMaths_TransposeMatrix1(&localMat, &this->mTransform);
		gte_SetRotMatrix(&localMat);

		CVector delta;
		delta.vx = (trigPos.vx - hookPos.vx) >> 12;
		delta.vy = (trigPos.vy - hookPos.vy) >> 12;
		delta.vz = (trigPos.vz - hookPos.vz) >> 12;

		delta <<= 12;
		gte_ldlvl(reinterpret_cast<VECTOR*>(&delta));
		gte_rtir();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&delta));

		CVector zero;
		zero.vx = 0;
		zero.vy = 0;
		zero.vz = 0;
		Utils_CalcAim(&lookAngle, &zero, &delta);
	}

	if (this->field_188)
	{
		// @FIXME guess: field_188 points at a turret-style head/eye object
		// with a pan/tilt angle pair at offset 0x24
		i16 *panTilt = reinterpret_cast<i16*>(this->field_188) + 0x12;

		i32 dx = lookAngle.vx - panTilt[0];
		if (dx > 0x800) dx -= 0x1000;
		else if (dx < -0x800) dx += 0x1000;

		if (dx > 0x30)
		{
			dx = 0x30;
		}
		else if (dx < -0x30)
		{
			dx = -0x30;
		}
		panTilt[0] = static_cast<i16>(panTilt[0] + dx);

		i32 dy = lookAngle.vy - panTilt[1];
		if (dy > 0x800) dy -= 0x1000;
		else if (dy < -0x800) dy += 0x1000;

		if (dy > 0x30)
		{
			dy = 0x30;
		}
		else if (dy < -0x30)
		{
			dy = -0x30;
		}
		panTilt[1] = static_cast<i16>(panTilt[1] + dy);

		if ((this->mFlags & 4) && this->field_340)
		{
			if ((panTilt[0] | panTilt[1]) == 0)
			{
				this->mFlags &= ~4;
			}
		}
	}

	if (this->mFlags & 4)
	{
		this->ApplyPose(gUnkPose);
	}
	else
	{
		M3d_BuildTransform(this);
	}

	this->DoMGSShadow();
}

// @NotOk
// 4 leg/paw hook positions rotated into local (body) space via the
// transposed body matrix, gives an X/Z footprint box (floor 32), then a
// vertical offset (realRegisterArr[0]) rotated by the body matrix gives the
// world space shadow center. functionally close but not matching:
// cmpsum shows 179 mnemonic diffs, first divergence right at entry (the
// original has an SEH frame push here that this source does not produce,
// likely from some non-trivial local construction I have not found the
// right shape for yet). semantics (hook ids 3/6/13/9, box floor 32,
// CQuadBit lazily created into field_33C) verified against the disasm.
void CBlackCat::DoMGSShadow(void)
{
	SHook hook;
	VECTOR pos0, pos1, pos2, pos3;

	hook.Part.vx = 0;
	hook.Part.vy = 0;
	hook.Part.vz = 0;
	hook.Offset = 3;
	M3dUtils_GetDynamicHookPosition(&pos0, this, &hook);

	hook.Offset = 6;
	M3dUtils_GetDynamicHookPosition(&pos1, this, &hook);

	hook.Offset = 13;
	M3dUtils_GetDynamicHookPosition(&pos2, this, &hook);

	hook.Offset = 9;
	M3dUtils_GetDynamicHookPosition(&pos3, this, &hook);

	i32 height = this->field_21E << 12;

	CVector v0 = *reinterpret_cast<CVector*>(&pos0);
	v0 -= this->mPos;
	CVector v1 = *reinterpret_cast<CVector*>(&pos1);
	v1 -= this->mPos;
	CVector v2 = *reinterpret_cast<CVector*>(&pos2);
	v2 -= this->mPos;
	CVector v3 = *reinterpret_cast<CVector*>(&pos3);
	v3 -= this->mPos;

	CVector heightOffset;
	heightOffset.vx = 0;
	heightOffset.vy = height;
	heightOffset.vz = 0;

	MATRIX localMat;
	M3dMaths_TransposeMatrix1(&localMat, &this->mTransform);
	gte_SetRotMatrix(&localMat);

	CVector box[4] = { v0, v1, v2, v3 };

	i32 maxX = 0x20;
	i32 minX = box[0].vx;
	i32 maxZ = box[0].vz;
	i32 minZ = box[0].vz;
	i32 i;

	for (i = 0; i < 4; i++)
	{
		box[i] >>= 12;
		gte_ldlvl(reinterpret_cast<VECTOR*>(&box[i]));
		gte_rtir();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&box[i]));

		if (box[i].vx > maxX)
		{
			maxX = box[i].vx;
		}
		else if (box[i].vx < minX)
		{
			minX = box[i].vx;
		}

		if (box[i].vz > maxZ)
		{
			maxZ = box[i].vz;
		}
		else if (box[i].vz < minZ)
		{
			minZ = box[i].vz;
		}
	}

	heightOffset >>= 12;
	gte_ldlvl(reinterpret_cast<VECTOR*>(&heightOffset));
	gte_rtir();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&heightOffset));

	print_if_false(1, "MGS shadow");

	gte_SetRotMatrix(&this->mTransform);

	i32 ry = this->realRegisterArr[0] << 12;

	CVector corners[4];
	for (i = 0; i < 4; i++)
	{
		corners[i].vx = this->mPos.vx + heightOffset.vx;
		corners[i].vy = ry;
		corners[i].vz = this->mPos.vz + heightOffset.vz;
	}

	if (!this->field_33C)
	{
		TotalBitUsage = 0;
		this->field_33C = new CQuadBit();
		TotalBitUsage = -1;

		reinterpret_cast<CQuadBit*>(this->field_33C)->SetTexture(0, 0);
	}

	reinterpret_cast<CQuadBit*>(this->field_33C)->mFrigDeltaZ = 32;
	reinterpret_cast<CQuadBit*>(this->field_33C)->SetTransparency(0x40);
	reinterpret_cast<CQuadBit*>(this->field_33C)->SetSubtractiveTransparency();
	reinterpret_cast<CQuadBit*>(this->field_33C)->SetCorners(corners[0], corners[1], corners[2], corners[3]);
}

// @Ok
i32* CBlackCat::GetNewCommandBlock(u32 a1)
{
	i32* res = static_cast<i32*>(DCMem_New(4 * a1, 0, 1, 0, 1));
	res[a1 - 1] = 0;

	if (!this->field_350)
	{
		this->field_350 = res;
	}
	else
	{
		i32* it = this->field_350;
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
void CBlackCat::KillCommandBlockByID(i32)
{
    printf("CBlackCat::KillCommandBlockByID(i32)");
}

// @MEDIUMTODO
void CBlackCat::SynthesizeAnalogueInput(void)
{
    printf("CBlackCat::SynthesizeAnalogueInput(void)");
}

// @NotOk
// guess type of 33C
CBlackCat::~CBlackCat(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
	delete reinterpret_cast<CClass*>(this->field_33C);

	this->KillAllCommandBlocks();
}

// @NotOk
// globals
CBlackCat::CBlackCat(i16* a2, i32 a3)
{
	if (Trig_GetLevelID() != 2051)
	{
		this->InitItem("blackcat");
	}
	else
	{
		this->InitItem("bc2");
	}

	i16 *v5 = this->SquirtAngles(reinterpret_cast<i16*>(this->SquirtPos(a2)));

	this->field_21E = 100;
	this->RunAnim(0xC, 0, -1);
	this->mFlags |= 0x480;

	this->mpLight = &M3d_BlackCatLight;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->mType = 319;
	this->field_31C.bothFlags = 1;

	this->mNode = a3;
	this->mRMinor = 0;
	this->field_34C = reinterpret_cast<i32>(v5);

	if (submarinerDieRelated && Trig_GetLevelID() != 2051)
		this->Die(0);
}

// @Ok
void BlackCat_CreateBlackCat(const u32 *stack, u32 *result)
{
	i16* v2 = reinterpret_cast<i16*>(*stack);
	i32 v3 = static_cast<i32>(stack[1]);

	*result = reinterpret_cast<u32>(new CBlackCat(v2, v3));
}


// @Ok
void CBlackCat::Shouldnt_DoPhysics_Be_Virtual(void)
{
	this->DoPhysics();
}

// @Ok
// @Matching
void CBlackCat::DoPhysics(void)
{
	i32 i = 0;

	if (this->field_80 > 0)
	{
		do
		{
			this->mAcc.vz = 0;
			this->mAcc.vx = 0;
			this->mAcc.vy = this->field_32C - (this->mVel.vy / 16);

			{
				i32 scaleBuf;
				CVector& scale = *reinterpret_cast<CVector*>(&scaleBuf);
				scale.vx = 2;
				this->mVel += scale * this->mAcc;
			}

			{
				i32 divisor = 2;

				i32 velScaleBuf;
				CVector& velScale = *reinterpret_cast<CVector*>(&velScaleBuf);
				velScale.vx = 2;

				i32 accScaleBuf;
				CVector& accScale = *reinterpret_cast<CVector*>(&accScaleBuf);
				accScale.vx = 4;

				this->mPos += (velScale * this->mVel) + ((accScale * this->mAcc) / divisor);
			}

			this->mAngles.vy += this->mAngVel.vy << 1;
			this->mAngles.Mask();

			this->mAngVel.vy += this->mAngAcc.vy << 1;
			this->mAngVel %= this->mAngFric;
			this->mAngVel.KillSmall();

			i += 2;
		} while (i < this->field_80);
	}
}

// @Ok
// @Matching
i32* CBlackCat::KillCommandBlock(i32* a1)
{
	i32* res = reinterpret_cast<i32*>(a1[a1[1]-1]);

	if (this->field_350 == a1)
	{
		this->field_350 = res;
	}
	else
	{
		i32* it = this->field_350;

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
void CBlackCat::KillAllCommandBlocks(void)
{
	for (i32* cur = this->field_350; cur; cur = this->KillCommandBlock(cur));
	this->field_350 = 0;
}

void validate_CBlackCat(void){
	VALIDATE_SIZE(CBlackCat, 0x354);


	VALIDATE(CBlackCat, field_324, 0x324);
	VALIDATE(CBlackCat, field_328, 0x328);
	VALIDATE(CBlackCat, field_32C, 0x32C);

	VALIDATE(CBlackCat, field_33C, 0x33C);


	VALIDATE(CBlackCat, field_340, 0x340);
	VALIDATE(CBlackCat, field_344, 0x344);
	VALIDATE(CBlackCat, field_348, 0x348);

	VALIDATE(CBlackCat, field_34C, 0x34C);
	VALIDATE(CBlackCat, field_350, 0x350);
}
