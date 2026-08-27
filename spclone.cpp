#include "spclone.h"
#include "validate.h"
#include "m3dutils.h"

extern CBaddy* BaddyList;
extern u8 submarinerDieRelated;

EXPORT SLight M3d_SpCloneLight =
{
  { { -2430, -2228, -2430 }, { 2509, -2896, 1447 }, { -648, -3711, -1607 } },
  0,

  { { 3200, 1040, 2048 }, { 2720, 1600, 1920 }, { 2400, 2560, 2048 } },
  0,
  { 1200, 1200, 960 }
};


// @Ok
// @Matching
void SpClone_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = SpClone_RelocatableModuleClear;
	pMod->field_C[0] = SpClone_CreateSpClone;
}

// @MEDIUMTODO
void CSpClone::AI(void)
{
    printf("CSpClone::AI(void)");
}

// @Ok
// @Matching
CSpClone::CSpClone(i16 * a2,i32 a3)
{
	this->InitItem("spidey");
	this->field_194 &= 0xFFFFFFDF;
	this->field_194 |= 0x40u;
	this->field_194 &= 0xFFFFFBFF;
	this->field_194 |= 0x800u;

	i16 *v5 = this->SquirtAngles(this->SquirtPos(a2));

	this->ShadowOn();
	this->mShadowScale = 48;
	this->field_21E = 100;
	this->field_32C = 2;
	this->field_330 = 2;

	this->RunAnim(0xCAu, 0, -1);
	this->mFlags |= 0x480u;

	this->mpLight = &M3d_SpCloneLight;

	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->mType = 327;
	this->field_31C.bothFlags = 1;
	this->mNode = a3;
	this->mRMinor = 0;
	this->field_348 = reinterpret_cast<i32>(v5);

	if ( submarinerDieRelated )
		this->Die(0);
}

// @NotOk
// Same MGS-shadow idiom as CBlackCat::DoMGSShadow (blackcat.cpp) and CCarnage::DoMGSShadow
// (carnage.cpp): 4 hook positions rotated into local (body) space give an X/Z footprint box,
// then a vertical offset gives the world space shadow center, applied to a lazily-created
// CQuadBit (field_338). Hook offsets here are 0xE, 0x11, 0xB, 6. Unlike the other two, the
// height offset is rotated TWICE (once by the transposed body matrix, once by the body
// matrix directly) then shifted left 12, and the Y of each corner comes from
// realRegisterArr[0] << 12, not a plain field. Not matching yet: the original has an SEH
// frame at entry (mov eax,fs:[0]; push -1; push handler; ...) that this source does not
// produce, the same unresolved issue documented in CBlackCat::DoMGSShadow's comment. See
// ~/Documents/spidey-work/wt/spclone.attempts.md.
void CSpClone::DoMGSShadow(void)
{
	SHook hook;
	VECTOR pos0, pos1, pos2, pos3;

	hook.Part.vx = 0;
	hook.Part.vy = 0;
	hook.Part.vz = 0;
	hook.Offset = 0xE;
	M3dUtils_GetDynamicHookPosition(&pos0, this, &hook);

	hook.Offset = 0x11;
	M3dUtils_GetDynamicHookPosition(&pos1, this, &hook);

	hook.Offset = 0xB;
	M3dUtils_GetDynamicHookPosition(&pos2, this, &hook);

	hook.Offset = 6;
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

	print_if_false(
		maxX - minX < 0x40 && maxZ - minZ < 0x40,
		"MGS shadow box too big");

	gte_SetRotMatrix(&this->mTransform);

	i32 ry = this->realRegisterArr[0] << 12;

	gte_ldlvl(reinterpret_cast<VECTOR*>(&heightOffset));
	gte_rtir();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&heightOffset));

	heightOffset <<= 12;

	CVector corners[4];
	for (i = 0; i < 4; i++)
	{
		corners[i].vx = this->mPos.vx + heightOffset.vx;
		corners[i].vy = ry;
		corners[i].vz = this->mPos.vz + heightOffset.vz;
	}

	if (!this->field_338)
	{
		TotalBitUsage = 0;
		this->field_338 = new CQuadBit();
		TotalBitUsage = -1;

		this->field_338->SetTexture(0u, 0u);
	}

	this->field_338->mFrigDeltaZ = 0x20;
	this->field_338->SetTransparency(0x40);
	this->field_338->SetSubtractiveTransparency();
	this->field_338->SetCorners(corners[0], corners[1], corners[2], corners[3]);
}

// @Ok
INLINE i32* CSpClone::GetNewCommandBlock(u32 a1)
{
	i32* res = static_cast<i32*>(DCMem_New(4 * a1, 0, 1, 0, 1));
	res[a1 - 1] = 0;

	if (!this->field_34C)
	{
		this->field_34C = res;
	}
	else
	{
		i32* it = this->field_34C;
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
// @Matching
void CSpClone::KillCommandBlockByID(i32 a2)
{
	i32* i = this->field_34C;
	while(i)
	{
		if (*i == a2)
			this->KillCommandBlock(i);

		i = reinterpret_cast<i32*>(i[i[1] - 1]);
	}
}

// @MEDIUMTODO
void CSpClone::SynthesizeAnalogueInput(void)
{
    printf("CSpClone::SynthesizeAnalogueInput(void)");
}

// @Ok
CSpClone::~CSpClone(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));

	delete this->field_338;

	this->KillAllCommandBlocks();
}

// @Ok
// @Matching
void SpClone_CreateSpClone(const u32 * a2,u32 * a3)
{
	i16* v2 = reinterpret_cast<i16*>(a2[0]);
	i32 v3 = static_cast<i32>(a2[1]);

	*a3 = reinterpret_cast<u32>(new CSpClone(v2, v3));
}

// @Ok
// @Matching
void SpClone_RelocatableModuleClear(void)
{
	for (CBody* cur = BaddyList; cur; )
	{
		CBody* next = reinterpret_cast<CBody*>(cur->mNextItem);
		if (cur->mType == 327)
		{
			delete cur;
		}

		cur = next;
	}
}

// @Ok
void CSpClone::Shouldnt_DoPhysics_Be_Virtual(void)
{
	this->DoPhysics();
}

// @NotOk
// residue: 93 mnemonic diffs, all caused by vector.h's operator>>(const CVector&, const int&)
// being INLINE while the original calls it out of line (0x4E7840). Verified by temporarily
// making it out-of-line in a local build: with that change alone, cmpsum shows 0 mnemonic
// diffs, so the logic below is correct. Same class of bug as the documented operator-
// issue (bit.cpp note in CLAUDE.md), repo-wide, not something this function alone can fix.
// See ~/Documents/spidey-work/wt/spclone.attempts.md.
void CSpClone::DoPhysics(void)
{
	this->mAcc.vx = 0;
	this->mAcc.vy = this->field_328 - (this->mVel.vy / 16);
	this->mAcc.vz = 0;

	this->mVel += (CVector(this->field_334) * this->mAcc) >> 12;

	this->mPos += ((CVector(this->field_334) * this->mVel) >> 12)
	            + (((CVector((this->field_334 * this->field_334) >> 12) * this->mAcc) / 2) >> 12);

	this->mAngles.vy += (this->mAngVel.vy * this->field_334) >> 12;
	this->mAngles.Mask();

	this->mAngVel.vy += (this->mAngAcc.vy * this->field_334) >> 12;
	this->mAngVel %= this->mAngFric;
	this->mAngVel.KillSmall();
}

// @Ok
// @Matching
INLINE i32* CSpClone::KillCommandBlock(i32* a1)
{
	i32* res = reinterpret_cast<i32*>(a1[a1[1]-1]);

	if (this->field_34C == a1)
	{
		this->field_34C = res;
	}
	else
	{
		i32* it = this->field_34C;

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
void CSpClone::KillAllCommandBlocks(void)
{
	for (int* cur = reinterpret_cast<int*>(this->field_34C); cur; cur = this->KillCommandBlock(cur));
	this->field_34C = 0;
}

void validate_CSpClone(void){
	VALIDATE_SIZE(CSpClone, 0x350);

	VALIDATE(CSpClone, field_328, 0x328);
	VALIDATE(CSpClone, field_32C, 0x32C);
	VALIDATE(CSpClone, field_330, 0x330);
	VALIDATE(CSpClone, field_334, 0x334);
	VALIDATE(CSpClone, field_338, 0x338);

	VALIDATE(CSpClone, field_348, 0x348);

	VALIDATE(CSpClone, field_34C, 0x34C);
}
