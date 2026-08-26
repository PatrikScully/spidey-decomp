#include "turret.h"
#include "m3dutils.h"
#include "validate.h"

// @NotOk
// residue: 2 mnemonic diffs (down from higher counts on earlier attempts),
// instruction count matches exactly (74 both sides), so this is a pure
// scheduling swap, not a missing instruction. The original does the 9
// field_35C..field_36C zero-stores, THEN the SquirtPos call-setup group
// (push a2, ecx=this, a scratch local, the vtable pointer store). Our
// build does the call-setup group first, then the zero-stores. Both
// groups are independent (no shared registers or memory), so this is a
// scheduler choice, not something the source controls in an obvious way.
// 8 source variants tried: forward and reverse field declaration order,
// pre-declaring pCursor before the zeros, splitting SquirtPos/SquirtAngles
// into two statements, and replacing the 9 fields with a real array plus
// a for loop (this changed the shape a lot, 44 diffs, reverted). Rest of
// the function (CBaddy base ctor, both CVector auto-zeros, SquirtPos and
// SquirtAngles chain, mNode/InitItem/M3dUtils_ReadHooksPacket/AttachTo,
// mType/mRMinor/mHealth/field_2A8, RunAnim) all line up once this residue
// is looked past.
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

