#include "powerup.h"
#include "spool.h"
#include "trig.h"
#include "exp.h"
#include "my_assert.h"
#include "utils.h"

#include "validate.h"

extern i32 TotalBitUsage;
CBody* PowerUpList;
i32 TTime;

// @Ok
// @Matching
void CPowerUp::DoPhysics(void)
{
	this->mAngles.vy = (this->mAngles.vy + this->mAngVel.vy * (i16)this->field_80) & 0xFFF;

	if (this->field_103)
	{
		if (!this->field_104 && this->field_10C < 0 && this->field_10C > -5)
		{
			if (TTime & 1)
			{
				i32 height = Utils_GetGroundHeight(&this->field_110, 0, 0x1F40, 0);
				if (height == -1)
				{
					this->field_10C--;
					if (this->field_10C <= -5)
					{
						this->field_104 = 1;
					}
					return;
				}

				this->field_10C = height;
				return;
			}
			return;
		}

		CVector* pVel = &this->mVel;
		CVector* pWork = &this->field_110;
		*pWork += *pVel;

		this->mVel.vy += this->mAcc.vy;
		this->mVel.vy -= this->mVel.vy >> this->mFric.vy;

		if (!this->field_104)
		{
			i32 threshold = this->field_10C - (this->field_105 << 12);
			if (this->field_110.vy > threshold && this->mVel.vy > 0)
			{
				this->field_110.vy = threshold;
				pVel->vx = 0;
				pVel->vy = 0;
				pVel->vz = 0;
				this->field_103 = 0;
			}
		}

		this->mPos = this->field_110;
		return;
	}

	this->mPos = this->field_110;
}

// @Ok
void CPowerUp::AI(void)
{
	if ( (this->mFlags & 1) && this->mIs3d)
		this->mShadowScale = 0;
	else
		this->mShadowScale = 32;

	if (this->field_124)
	{
		this->mRGB = 0x101010;
		this->mFlags |= 0xC00;
		this->field_124 = 0;
	}
	else
	{
		this->mFlags &= 0xF3FF;
	}


	if (this->field_128)
	{
		if (this->field_128 > 2)
		{
			this->mFlags |= 0x400;

			this->field_128 -= 2;

			u32 tmp = (255 * this->field_128 / 64);
			tmp |= ((tmp << 8) | tmp) << 8;
			this->mRGB = tmp;
		}
		else
		{
			this->Die();
			return;
		}
	}

	this->CheckAge();

	if (this->mPlayerDist > 0x1F40)
	{
		this->DeleteStuff();
	}
	else
	{
		if (this->field_10C != 0xFFFFFFFF)
		{
			this->ShadowOn();
		}

		this->DoPhysics();

		if (!this->mIs3d)
		{
			if (!this->pPickupBit)
				this->CreateBit();
			DoAssert(this->pPickupBit != 0, "Non-3d pickup has no pickup bit");
			this->pPickupBit->SetPos(this->mPos);
		}
	}
}

// @Ok
void CPowerUp::CheckAge(void)
{
	if (this->mLifetime != 0xFFFF)
	{
		if (this->mLifetime)
			this->mLifetime--;

		if (this->mLifetime < 0x3C)
			this->field_120 = this->mLifetime * this->field_11E / 60;

		if (this->mLifetime < 0x3C && this->mLifetime > 0x1E)
		{
				if (TTime & 2)
					this->mDropping = 1;
				else
					this->mDropping = 0;
		}
		if (this->mLifetime <= 0x1E)
		{
			if (TTime & 1)
				this->mDropping = 1;
			else
				this->mDropping = 0;
		}

		if (this->mIs3d)
		{
			if (this->mDropping)
				this->mFlags &= 0xFFFE;
			else
				this->mFlags |= 1;
		}

		if (!this->mLifetime)
		{
			if (this->mPlayerDist <= 0x1F40)
				Exp_GlowFlash(&this->mPos, 20, 0xFF, 0xFF, 0xFF, 4, 1, 20);
			this->Die();
		}
	}
}

// @Ok
CPowerUp::~CPowerUp(void)
{
	this->DeleteFrom(&PowerUpList);
	this->DeleteStuff();
}

// @Ok
void CPowerUp::Die(void)
{
	if (!this->IsDead())
	{
		if (this->mType != 11 && this->mHasNode)
		{
			Trig_SendPulse(Trig_GetLinksPointer(this->mNodeIndex));
		}

		if (this->field_108 != 0xFFFF)
		{
			u16 LinkInfo[2];
			LinkInfo[0] = 1;
			LinkInfo[1] = this->field_108;

			Trig_SendSignalToLinks(LinkInfo);
		}

		this->mCBodyFlags |= 0x40;
		this->mFlags |= 1;
	}
}

// guess: current costume/tint index, used as the 2nd arg to
// Spool_GetModel below. Also referenced (same address) by
// M3d_RenderSetup and Init_AtStart, neither decompiled yet to confirm.
static u8 * const gCostumeId = (u8*)0x6B4678;

// guess: cheat/unlock bitmask, tested as (1 << mHealth) & this. Also
// referenced (same address) by ActivateCheat, PShell_MaybeUnlockStuff,
// Shell_ComicCollection and CPowerUp::TakeEffect, none decompiled yet.
static i32 * const gCheatUnlockFlags = (i32*)0x6828E4;

// @NotOk
// residue: huge (1089 byte) constructor with an SEH frame (automatic,
// not hand-written) and an 11-way switch on mType (values 8..18) that
// sets mHealth and spools a model per type. The switch's jump table
// lives 3 bytes past the end of the extracted function bytes
// (tools/functions/4632512.bin is exactly 1089 bytes, the table is
// referenced at a higher address), so the exact type->case mapping for
// cases 8/9/10 (which share one tail) and 15/16 (the two cases without
// a Spool_GetModel call) is inferred from code order and semantics
// (case 15 skips model spooling and reuses the lifetime arg as health,
// matching CreateBit's mType==18 special case elsewhere in this file;
// case 16 does an assert-style bounds check), not confirmed against the
// real table bytes. Everything before the switch (SEH-generated base
// call, field_110/mPos/mVel init from the pointer args, friction/accel/
// angvel constants, shadow scale+ShadowOn, the three-way ground-height
// lookup depending on the flags bits, the mType==18 ground-snap
// adjustment, InitItem) is a direct, confident disassembly trace. The
// tail (IsDead/Trig_SendPulse/Trig_SendSignalToLinks block gated on
// mType==10 and a cheat-unlock bit) is also a confident trace, including
// the literal `mType != 11` recheck inside the `mType == 10` branch,
// which looks always-true from here but is reproduced as written rather
// than "fixed" (tips.txt: reproduce source-level dead code, don't fix
// it). cmpsum: 171 mnemonic diffs on this first draft (the opening field
// inits and the shadow/ground-height block are close, off mostly by
// small local reorderings; the switch and tail are not yet close).
// Needs decomp.me or substantially more time, well past what fits in
// this session alongside the other 5 functions.
CPowerUp::CPowerUp(
		u16 a1,
		CVector* pos,
		CVector* vel,
		u32 flags,
		i32 a5,
		i32 a6)
{
	this->field_110.vx = 0;
	this->field_110.vy = 0;
	this->field_110.vz = 0;

	this->field_124 = 0;
	this->mType = a1;
	this->mLifetime = (u16)a5;
	this->field_108 = (u16)a6;
	this->field_105 = 0x80;
	this->mDropping = 1;

	this->field_110 = *pos;
	this->mPos = *pos;

	if (flags & 8)
	{
		this->mVel.vy = (i32)0xFFFC4000;
	}
	else
	{
		this->mVel = *vel;
	}

	this->mFric.vx = 5;
	this->mFric.vy = 5;
	this->mFric.vz = 5;

	this->mAngVel.vy = 0x18;
	this->mAcc.vy = 0x8000;

	this->field_122 = (i16)(Rnd(20) + 0x5A);

	this->field_11E = 0x10;
	this->field_120 = 0x10;
	this->mShadowScale = 0x20;

	this->ShadowOn();

	i32 height;

	if (flags & 0x10)
	{
		this->field_103 = 1;
		this->field_104 = 1;
	}
	else if (flags & 4)
	{
		this->field_103 = 1;
		height = Utils_GetGroundHeight(&this->field_110, 0x200, 0x1F40, 0);
		this->field_10C = height;
		if (height == -1)
		{
			this->KillShadow();
		}
	}
	else
	{
		height = Utils_GetGroundHeight(&this->field_110, 0x200, 0x1F40, 0);
		this->field_10C = height;
		if (height == -1)
		{
			this->KillShadow();
		}
	}

	this->mShadowPos.vx = this->mPos.vx;
	this->mShadowPos.vy = height;
	this->mShadowPos.vz = this->mPos.vz;

	if (this->mType == 18)
	{
		i32 diff = (this->mPos.vy - height) >> 12;
		i32 absDiff = diff < 0 ? -diff : diff;
		if (absDiff < 0x40)
		{
			i32 snapped = height + (i32)0xFFFC0000;
			this->field_110.vy = snapped;
			this->mPos.vy = snapped;
		}
	}

	this->mRMinor = 0x80;
	this->mShadowThreshold = 0x12C;

	this->InitItem("items");

	switch (this->mType)
	{
		case 8:
			this->mHealth = 0x14;
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x7E74F3D4, *gCostumeId);
			break;

		case 9:
			this->mHealth = 0x32;
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x7E74F3D4, *gCostumeId);
			break;

		case 10:
			this->mHealth = 0x64;
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x7E74F3D4, *gCostumeId);
			break;

		case 11:
			this->mHealth = 0x1000;
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x17646B0D, *gCostumeId);
			break;

		case 12:
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x7F648179, *gCostumeId);
			break;

		case 13:
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0x12820A41, *gCostumeId);
			break;

		case 14:
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0xC6739C3B, *gCostumeId);
			break;

		case 15:
			this->mHasNode = 0;
			if (this->mLifetime != 0xFFFF)
			{
				this->mHealth = this->mLifetime;
				this->mLifetime = 0xFFFF;
			}
			break;

		case 16:
			this->mHasNode = 1;
			this->mModel = (u16)Spool_GetModel(0xA092D785, *gCostumeId);
			DoAssert(this->mLifetime < 0x20, "?");
			this->mHealth = this->mLifetime;
			this->mLifetime = 0xFFFF;
			break;

		default:
			DoAssert(0, "?");
			break;
	}

	if (!this->mIs3d)
	{
		this->mFlags |= 1;
	}

	if (this->mType == 10)
	{
		if ((1 << this->mHealth) & *gCheatUnlockFlags)
		{
			if (!this->IsDead())
			{
				if (this->mType != 11 && this->mHasNode)
				{
					Trig_SendPulse(Trig_GetLinksPointer(this->mNodeIndex));
				}

				if (this->field_108 != 0xFFFF)
				{
					u16 LinkInfo[2];
					LinkInfo[0] = 1;
					LinkInfo[1] = this->field_108;
					Trig_SendSignalToLinks(LinkInfo);
				}

				this->mCBodyFlags |= 0x40;
				this->mFlags |= 1;
			}
		}
	}

	this->AttachTo(reinterpret_cast<CBody**>(&PowerUpList));
}

// @Ok
// @Matching
void CPowerUp::DeleteStuff(void)
{
	this->KillShadow();

	if (this->pPickupBit)
		delete this->pPickupBit;
	this->pPickupBit = 0;

	if (this->mpGlow)
		delete this->mpGlow;

	this->mpGlow = 0;
}

// @Ok
void CPowerUp::CreateBit(void)
{
	TotalBitUsage = 0; 
	if (this->mType == 18)
	{
		this->pPickupBit = new CFlatBit();

		Texture *pTexture = Spool_FindTextureEntry("TrainingPick-Up_01");
		this->pPickupBit->SetTexture(pTexture);
		this->pPickupBit->SetSemiTransparent();

		switch (this->mHealth)
		{
			case 0xA:
				this->pPickupBit->SetTint(0x6Au, 212, 105);
				break;
			case 0x14:
				this->pPickupBit->SetTint(0x85u, 133, 255);
				break;
			case 0x28:
				this->pPickupBit->SetTint(0xFFu, 77, 77);
				break;
			case 0x50:
				this->pPickupBit->SetTint(0xD8u, 216, 216);
				break;
			default:
				this->pPickupBit->SetTint(0x80u, 128, 32);
				break;
		}
	}

	if (this->pPickupBit)
		this->pPickupBit->mProtected = 1;

	TotalBitUsage = 1; 
}

// @Ok
void CPowerUp::SetGravity(i32 g, i32 fric)
{
	this->mAcc.vy = g;
	this->mFric.vy = fric;
}

// @Ok
// @Matching
void CPowerUp::SetNode(i32 NodeIndex)
{
	this->mHasNode = 1;
	this->mNodeIndex = NodeIndex;
}

void validate_CPowerUp(void)
{
	VALIDATE_SIZE(CPowerUp, 0x138);

	VALIDATE(CPowerUp, mpGlow, 0xF8);

	VALIDATE(CPowerUp, mHasNode, 0x100);
	VALIDATE(CPowerUp, mIs3d, 0x101);
	VALIDATE(CPowerUp, mDropping, 0x102);

	VALIDATE(CPowerUp, field_103, 0x103);
	VALIDATE(CPowerUp, field_104, 0x104);
	VALIDATE(CPowerUp, field_105, 0x105);

	VALIDATE(CPowerUp, mNodeIndex, 0x106);
	VALIDATE(CPowerUp, field_108, 0x108);

	VALIDATE(CPowerUp, field_10C, 0x10C);

	VALIDATE(CPowerUp, field_110, 0x110);

	VALIDATE(CPowerUp, field_11E, 0x11E);
	VALIDATE(CPowerUp, field_120, 0x120);
	VALIDATE(CPowerUp, field_122, 0x122);
	VALIDATE(CPowerUp, field_124, 0x124);
	VALIDATE(CPowerUp, field_128, 0x128);

	VALIDATE(CPowerUp, mLifetime, 0x12C);

	VALIDATE_VTABLE(CPowerUp, DeleteStuff, 4);
}
