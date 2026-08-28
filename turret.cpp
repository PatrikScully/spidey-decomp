#include "turret.h"
#include "m3dutils.h"
#include "validate.h"

// @Ok
// Functional: turret constructor, logic verified against Hex-Rays.
// 2 mnemonic diffs are pure scheduling swap (instruction count matches).
CTurret::CTurret(i16 *a2, i32 a3)
{
	this->field_35C = 0;
	this->field_35E = 0;
	this->field_360 = 0;
	this->field_362 = 0;
	this->field_364 = 0;
	this->field_366 = 0;
	this->field_368 = 0;
	this->field_36A = 0;
	this->field_36C = 0;

	i16 *pCursor = this->SquirtAngles(this->SquirtPos(a2));

	this->mNode = static_cast<u16>(a3);
	this->field_24C = pCursor;
	this->field_20C = 1;

	this->InitItem("turret");

	M3dUtils_ReadHooksPacket(this, reinterpret_cast<void*>(0x55984C));

	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->mType = 325;
	this->mRMinor = 0x80;
	this->mHealth = 10;
	this->field_2A8 |= 0x10200;

	this->RunAnim(-1, 0, 0);
}

// @Ok
// @Matching
void Turret_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Turret_RelocatableModuleClear;
	pMod->field_C[0] = Turret_CreateTurret;
}

// @Ok
// @Matching
void Turret_CreateTurret(const u32 *stack, u32 * result)
{
	i16 *v2 = reinterpret_cast<i16*>(stack[0]);
	i32 v3 = stack[1];

	*result = reinterpret_cast<u32>(new CTurret(v2, v3));
}

// @Ok
// @Matching
void Turret_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 325)
			delete pSearch;

		pSearch = pNext;
	}
}

// @Ok
void INLINE CTurret::TargetLockAbsolute(const CVector &a1){
	this->field_344 = a1;
	this->field_218 |= 4;
}

// @Ok
void INLINE CTurret::TargetLockDynamic(CBody *a2)
{
	if ( !a2->IsDead() )
	{
		this->field_378 = a2;
		this->field_218 |= 0x10u;
	}
}

// @Ok
void CTurretLaser::SetDamage(int damage)
{
	this->field_4C = damage;
}

// @Ok
void INLINE CTurret::ClearTargetLock(void)
{
	this->field_218 &= 0xFFFFFFE3;
}

void validate_CTurret(void)
{
	VALIDATE_SIZE(CTurret, 0x37C);

	VALIDATE(CTurret, field_324, 0x324);
	VALIDATE(CTurret, field_344, 0x344);
	VALIDATE(CTurret, field_378, 0x378);
}

void validate_CTurretBase(void)
{
		VALIDATE_SIZE(CTurretBase, 0xFC);
}

void validate_CTurretLaser(void)
{
		VALIDATE_SIZE(CTurretLaser, 0x64);

		VALIDATE(CTurretLaser, field_4C, 0x4C);

}

