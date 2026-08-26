#include "platform.h"
#include "ps2lowsfx.h"
#include "utils.h"
#include "shatter.h"
#include "spidey.h"
#include "spool.h"
#include "trig.h"

#include "validate.h"

extern CBody* EnvironmentalObjectList;
extern const char* gObjFile;
extern CSVector gTrajectoryVector;

// @MEDIUMTODO
void CPlatform::AI(void)
{
    printf("CPlatform::AI(void)");
}

// @Ok
// @Matching
void CPlatform::AdjustBruceHealth(void)
{
	i16 value = this->field_330;
	if (value)
	{
		if (value < 0)
		{
			MechList->IncHealth(value - 1);
		}
		else
		{
			SHitInfo v2;
			v2.field_C.vx = 0;
			v2.field_C.vy = 0;
			v2.field_C.vz = 0;

			v2.field_8 = value;
			v2.field_0 = 4;

			MechList->Hit(&v2);
		}
	}
}

// @Ok
CPlatform::CPlatform(i16 * a2,i32 a3)
{
	this->field_344.vx = 0;
	this->field_344.vy = 0;
	this->field_344.vz = 0;
	this->field_350.vx = 0;
	this->field_350.vy = 0;
	this->field_350.vz = 0;

	this->InitItem(gObjFile);
	this->AttachTo(&EnvironmentalObjectList);

	this->mFlags |= 0x111;
	this->mFlags &= 0xFFFD;
	this->mType = 402;

	this->field_24C = this->SquirtAngles(this->SquirtPos(a2));

	this->field_340 = -1;
	this->mNode = a3;
	this->attributeArr[0] = 32;
	this->field_20C = 1;
}

// @MEDIUMTODO
i32 CPlatform::ExecuteCommand(u16)
{
    printf("CPlatform::ExecuteCommand(u16)");
    return 0x04082024;
}

// @Ok
// @Matching
i32 CPlatform::Hit(SHitInfo* a2)
{
	this->field_20F++;
	if (this->attributeArr[1])
	{

		if (this->mHealth > 0)
		{
			this->mHealth -= a2->field_8;
			if (this->mHealth <= 0)
			{
				this->Die(0);
				SFX_PlayPos(Rnd(2) + 1, &this->mPos, 0);
				Shatter_Item(this, 0, 1);
				this->mFlags |= 1;
			}
		}
	}

	return 1;
}

// @Ok
// @Validate
INLINE void CPlatform::MoveTo(CVector* pVec)
{
	if (this->field_240 != *pVec)
	{
		this->field_240 = *pVec;
		this->field_344 = ((this->mPos - *pVec) >> 12);
		this->field_218 |= 1;
	}
}

// @Ok
// @Matching
void CPlatform::NotifyTrodUpon(CBody *,CVector const *,CSVector const *)
{
	this->field_334 = 1;
	this->field_338 = 0;
}

// @Ok
// @Matching
void CPlatform::SetVariable(u16 a2)
{
	switch (a2)
	{
	case 0x2123:
		this->field_20E = *this->field_24C;
		this->field_24C++;
		break;

	case 0x2124:
		this->mModel = *this->field_24C;
		this->field_24C++;
		if (*(u8*)((i32***)0x6B2454)[this->mRegion * 17][this->mModel] & 0x10)
			this->mFlags |= 0x20;
		else
			this->mFlags &= ~0x20;
		break;

	case 0x212F:
		{
			u32 *v6 = reinterpret_cast<u32*>((reinterpret_cast<u32>(this->field_24C) + 3) & 0xFFFFFFFC);
			u16 Model = Spool_GetModel(*v6, this->mRegion);

			this->mModel = Model;
			if (*(u8*)((i32***)0x6B2454)[this->mRegion * 17][Model] & 0x10)
				this->mFlags |= 0x20;
			else
				this->mFlags &= ~0x20;
			this->mFlags &= ~1;

			this->field_24C = reinterpret_cast<i16*>(&v6[1]);
		}
		break;

	case 0x2134:
		{
			i16 vx = this->GetScriptValue();
			i16 vy = this->GetScriptValue();
			i16 vz = this->GetScriptValue();

			this->mVel.vx = (i32)vx << 12;
			this->mVel.vy = (i32)vy << 12;
			this->mVel.vz = (i32)vz << 12;
		}
		break;

	case 0x2137:
		{
			i16 vx = this->GetScriptValue();
			i16 vy = this->GetScriptValue();
			i16 vz = this->GetScriptValue();

			this->mAngles.vx = vx;
			this->mAngles.vy = vy;
			this->mAngles.vz = vz;
		}
		break;

	case 0x2127:
		{
			i16 vx = this->GetScriptValue();
			i16 vy = this->GetScriptValue();
			i16 vz = this->GetScriptValue();

			this->mAngVel.vx = vx;
			this->mAngVel.vy = vy;
			this->mAngVel.vz = vz;
		}
		break;

	case 0x2128:
		{
			i16 vx = this->GetScriptValue();
			i16 vy = this->GetScriptValue();
			i16 vz = this->GetScriptValue();

			this->mAngAcc.vx = vx;
			this->mAngAcc.vy = vy;
			this->mAngAcc.vz = vz;
		}
		break;

	default:
		CBaddy::SetVariable(a2);
		break;
	}
}

// @Ok
CPlatform::~CPlatform(void)
{
	this->DeleteFrom(&EnvironmentalObjectList);
	if (this->field_33C)
	{
		SFX_Stop(this->field_33C);
		this->field_33C = 0;
	}
}
// @Ok
void CPlatform::Shouldnt_DoPhysics_Be_Virtual(void)
{
	this->DoPhysics();
}

// @Ok
// @Matching
void CPlatform::DoPhysics(void)
{
	this->field_A8 = gTrajectoryVector;

	if (this->field_2B0 | this->field_2B4)
	{
		CBaddy::DoPhysics(0);
		return;
	}

	if (this->attributeArr[2] == 0)
	{
		i32 step;
		if (Trig_GetLevelID() == 0x301 && this->field_80 >= 4)
			step = 2;
		else
			step = 1;

		for (i32 i = 0; i < this->field_80; i += step)
		{
			this->mVel += this->mAcc;
			this->mVel.KillSmall();
			this->mPos += this->mVel;
			this->mAngles += this->mAngVel;
			this->mAngles.Mask();
			this->mAngVel += this->mAngAcc;
			this->mAngVel.KillSmall();
		}
	}
	else if (this->attributeArr[2] == 3)
	{
		this->mVel += this->mAcc;
		this->mVel.KillSmall();
		this->mPos += this->mVel;
		this->mAngles += this->mAngVel;
		this->mAngles.Mask();
		this->mAngVel += this->mAngAcc;
		this->mAngVel.KillSmall();
	}
}

// @Ok
i16 CPlatform::GetVariable(u16 a2)
{
	if (a2 != (u16)0x2200)
	{
		return CBaddy::GetVariable(a2);
	}

	return this->field_338;
}

void validate_CPlatform(void){
	VALIDATE_SIZE(CPlatform, 0x35C);

	VALIDATE(CPlatform, field_330, 0x330);

	VALIDATE(CPlatform, field_334, 0x334);
	VALIDATE(CPlatform, field_338, 0x338);
	VALIDATE(CPlatform, field_33C, 0x33C);

	VALIDATE(CPlatform, field_340, 0x340);

	VALIDATE(CPlatform, field_344, 0x344);
	VALIDATE(CPlatform, field_350, 0x350);
}

