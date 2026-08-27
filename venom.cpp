#include "venom.h"
#include "validate.h"
#include "mem.h"
#include "utils.h"
#include "ps2lowsfx.h"
#include "trig.h"
#include "web.h"
#include "spool.h"
#include "panel.h"
#include "trig.h"
#include "camera.h"
#include "ps2funcs.h"
#include "my_assert.h"


extern CBody* EnvironmentalObjectList;

#define LEN_VENOM_TEXS 10
EXPORT Texture* gVenomTexs[LEN_VENOM_TEXS];

// @Ok
// @Matching
CVenom::~CVenom(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
	Panel_DestroyHealthBar();
	this->KillAllCommandBlocks();

	if (Trig_GetLevelID() == 1281)
	{
		Panel_DestroyCompass();
	}

	delete this->field_434;
	delete this->field_428;
	delete this->field_398;
	delete this->field_3E4;
	delete this->field_334;

	if (this->field_390)
	{
		Mem_Delete(this->field_390);
	}
}

// @Ok
// @Matching
void CVenom::ExitWaitState(u32 a2, u32 a3)
{
	this->field_3B4 = 0;
	if (this->field_31C.bothFlags == 1)
	{
		this->field_3B0 = a2;
		if (a2 == 4)
		{
			this->field_3B4 = a3;
		}
	}
}

// @Ok
// @Matching
void CVenom::EnterWaitState(void)
{
	this->field_3B4 = 0;
	if (this->field_31C.bothFlags != 1)
	{
		this->field_31C.bothFlags = 1;
		this->dumbAssPad = 0;
	}
}

// @NotOk
// Same body shape as CCarnageHitSpark::CCarnageHitSpark (carnage.cpp), which is also @NotOk:
// only 4 relocation-class diffs at the vtable ptr / SEH scope table / two branch targets when
// diffed directly against CCarnageHitSpark's original bytes (structurally identical function,
// same constants: SetTexture(0x877E63C8), SetTint(0xFF,0x80,0), mType=30). Built version is
// 286 vs 288 original instructions; residue is in the v13/v14/v20 (Rnd(50)+50 based) scaled
// vector math, same class of issue as the carnage sibling. Not independently re-debugged here,
// see carnage.cpp's CCarnageHitSpark::CCarnageHitSpark for the shared open residue.
CVenomHitSpark::CVenomHitSpark(const CVector *pVec)
{
	this->mPosC = *pVec;

	CVector v41;
	v41.vx = 0;
	v41.vy = -4096;
	v41.vz = 0;

	CVector v40;

	v40.vx = gMikeCamera[0].Position.vx - (this->mPosC.vx >> 12);
	v40.vy = gMikeCamera[0].Position.vy - (this->mPosC.vy >> 12);
	v40.vz = gMikeCamera[0].Position.vz - (this->mPosC.vz >> 12);

	gte_ldopv1(reinterpret_cast<VECTOR*>(&v41));
	gte_ldopv2(reinterpret_cast<VECTOR*>(&v40));
	gte_op0();

	gte_stlvnl(reinterpret_cast<VECTOR*>(&v40));

	CVector v39;
	v39.vx = v40.vx >> 8;
	v39.vy = v40.vy >> 8;
	v39.vz = v40.vz >> 8;

	gte_ldlvl(reinterpret_cast<VECTOR*>(&v39));
	gte_sqr0();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&v39));

	i32 v8 = M3dMaths_SquareRoot0(v39.vx + v39.vy + v39.vz);
	v40.vx = 16 * (v40.vx / v8);
	v40.vy = 16 * (v40.vy / v8);
	v40.vz = 16 * (v40.vz / v8);


	i32 v9 = Rnd(4096);
	i32 v10 = Rnd(30);
	i32 v11 = (4 * v9) & 0x3FFC;

	i32 v12 = ((v10 + 5) * rcossin_tbl[v11].cos) >> 12;
	i32 v22 = ((v10 + 5) * rcossin_tbl[v11].sin) >> 12;

	this->mVel = (v12 * v41) + (v22 * v40);

	i32 v13 = Rnd(50) + 50;
	i32 v14 = (v13 * rcossin_tbl[v11].cos) >> 12;
	i32 v20 = (v13 * rcossin_tbl[v11].sin) >> 12;

	CVector v33 = (v14 * v41) + (v20 * v40);

	i32 v15 = (4 * (v9 + 1024)) & 0x3FFC;

	i32 v18 = (10 * rcossin_tbl[v15].sin) >> 12;
	i32 v19 = (10 * rcossin_tbl[v15].cos) >> 12;

	CVector v30 = (v19 * v41) + (v18 * v40);

	this->mPos = this->mPosC + v33;

	if (Rnd(2))
	{
		this->mPosD = this->mPosC + v30;
	}
	else
	{
		this->mPosD = this->mPosC - v30;
	}

	this->mPosB = this->mPosD + v33;


	this->SetTexture(0x877E63C8);
	this->SetSemiTransparent();
	this->SetTint(0xFF, 0x80u, 0);
	this->mType = 30;
}

// @Ok
// @Matching
// @Note: COMDAT should merge this with CarnageHitSpark::Move
void CVenomHitSpark::Move(void)
{
	this->mPos.vx += this->mVel.vx;
	this->mPos.vy += this->mVel.vy;
	this->mPos.vz += this->mVel.vz;

	this->mPosB.vx += this->mVel.vx;
	this->mPosB.vy += this->mVel.vy;
	this->mPosB.vz += this->mVel.vz;

	this->mPosC.vx += this->mVel.vx;
	this->mPosC.vy += this->mVel.vy;
	this->mPosC.vz += this->mVel.vz;

	this->mPosD.vx += this->mVel.vx;
	this->mPosD.vy += this->mVel.vy;
	this->mPosD.vz += this->mVel.vz;

	if (++this->mAge > 0)
		Bit_ReduceRGB(&this->mTint, 30);

	if ((this->mTint & 0xFFFFFF) == 0)
		this->Die();
}

// @Ok
// @Matching
CVenomHitSpark::~CVenomHitSpark(void)
{
}


// @Ok
// @Matching
void CVenom::CreateCombatImpactEffect(CVector *a2,i32)
{
	for (i32 i = 0; i < 16; i++)
	{
		new CVenomHitSpark(a2);
	}
}

// @BIGTODO
void Venom_DisplayProgressBar(const u32*, u32*)
{
	printf("void Venom_DisplayProgressBar(const u32*, u32*)");
}

// @Ok
// @Matching
void Venom_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 313)
			delete pSearch;

		pSearch = pNext;
	}

	for (i32 i = 0; i < LEN_VENOM_TEXS; i++)
	{
		gVenomTexs[i] = 0;
	}
}

// @Ok
// @Matching
void Venom_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Venom_RelocatableModuleClear;
	pMod->field_C[0] = Venom_CreateVenom;
	pMod->field_C[1] = Venom_DisplayProgressBar;
}

// @Ok
INLINE i32* CVenom::GetNewCommandBlock(u32 a1)
{
	i32* res = static_cast<i32*>(DCMem_New(4 * a1, 0, 1, 0, 1));
	res[a1 - 1] = 0;

	if (!this->field_35C)
	{
		this->field_35C = res;
	}
	else
	{
		i32* it = this->field_35C;
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
// Node type 1002 (ID_SWITCH_TO) with subtype 4 marks a "go to switch" node. For each
// one found, walk its link chain to the final target node, find the position, then
// find the nearest CSwitch (mType 407) in ControlBaddyList to that position.
void CVenom::ResolveSwitchNodes(void)
{
	this->field_3CC = 0;

	for (i32 i = 1; i < NumNodes; i++)
	{
		i16 *node = G_OFFSETLIST[i];

		if (node[0] == 1002 && node[1] == 4)
		{
			print_if_false(this->field_3CC < 4, "More than 4 ID_SWITCH_TO nodes found");

			i32 linkIndex = i;
			this->field_3BC[this->field_3CC] = linkIndex;

			u16 *links = Trig_GetLinksPointer(linkIndex);

			if (links[0] != 0)
			{
				do
				{
					print_if_false(links[0] == 1, "More than 1 linked node for GointToSwitches path");
					linkIndex = links[1];
					links = Trig_GetLinksPointer(linkIndex);
				} while (links[0] != 0);
			}

			CVector v3;
			v3.vx = 0;
			v3.vy = 0;
			v3.vz = 0;
			Trig_GetPosition(&v3, linkIndex);

			u32 bestDist = 0xFFFFFFFF;

			for (CItem *cur = ControlBaddyList; cur; cur = reinterpret_cast<CItem*>(cur->mNextItem))
			{
				if (cur->mType == 407)
				{
					u32 dist = Utils_Dist(v3, cur->mPos);

					if (dist < bestDist)
					{
						bestDist = dist;
						this->field_3D0[this->field_3CC] = cur;
					}
				}
			}

			print_if_false(bestDist < 0xFFFFFFFF, "No switch found");

			this->field_3CC++;
		}
	}

	this->field_3B8 = (1 << this->field_3CC) - 1;
}

EXPORT SLight M3d_VenomLight =
{
	{ { -2430, -2228, -2430 }, { 2509, -2896, 1447 }, { -648, -3711, -1607 } },
	0,
	{ { 3200, 1040, 2048 }, { 2720, 1600, 1920 }, { 2400, 2560, 2048 } },
	0,
	{ 1200, 1200, 960 }
};

// duplicate of the byte right before gWhatIf (0x60CFC4, ob.cpp). Name from
// baddy.cpp's gSubmarinerDieRelated. Tentative, static per file per repo convention.
static u8 * const gSubmarinerDieRelated = (u8*)0x60CFC4;

// @Ok
// @AlmostMatching: 2 mnemonic diffs (one store/push swap around the SquirtPos call setup,
// field_3EC store vs the SquirtPos arg push exchange position). Everything else, including
// every level-ID branch and the difficulty-level ternaries, matches exactly. 15 hypotheses
// tried, see ~/Documents/spidey-work/wt/CVenom_CVenom.attempts.md.
CVenom::CVenom(i32 *a2, i32)
{
	this->field_37C = 0;
	this->field_380 = 0;
	this->field_384 = 0;
	this->field_3A0 = 0;
	this->field_3A4 = 0;
	this->field_3A8 = 0;
	this->field_3E8 = 0;
	this->field_3EC = 0;
	this->field_3F0 = 0;
	this->field_400 = 0;
	this->field_404 = 0;
	this->field_408 = 0;
	this->field_40C = 0;
	this->field_410 = 0;
	this->field_414 = 0;
	this->field_418 = 0;
	this->field_41C = 0;
	this->field_420 = 0;

	i16 *pos = this->SquirtPos(reinterpret_cast<i16*>(a2));
	i16 *angles = this->SquirtAngles(pos);

	this->field_454 = 0xFF;
	this->mRGB = 0xFFFFFF;
	this->field_33D = 1;

	u32 levelId = Trig_GetLevelID();

	switch (levelId)
	{
		case 0x9904:
		case 0x601:
		case 0x602:
		case 0x501:
		case 0x503:
		case 0x504:
		case 0x505:
		case 0x506:
			this->mCBodyFlags &= ~0x10;
			this->mRMinor = 0;
			this->InitItem("venom2");
			break;

		default:
			this->mRMinor = 0xC0;
			this->InitItem("venom");
			break;
	}

	this->mType = 313;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));
	this->mFlags |= 0x480;

	this->mpLight = &M3d_VenomLight;
	this->field_21E = 100;
	this->field_31C.bothFlags = 1;
	this->dumbAssPad = 0;

	i32 groundY = Utils_GetGroundHeight(&this->mPos, 0, 0x1000, 0);
	if (groundY != -1)
	{
		this->mPos.vy = groundY - (this->field_21E << 12);
	}

	this->OutlineOn();
	this->OutlineOff();

	switch (levelId)
	{
		case 0x603:
		{
			this->mHealth = 0x258;
			this->field_460 = 0xF0;
			this->field_464 = 0x2D0;

			i32 v603 = (G_DIFFICULTY_LEVEL == 1 || G_DIFFICULTY_LEVEL == 0) ? 0x3C : 0x14;
			this->mFlags |= 0x41;
			this->field_39C = v603;
			this->field_45C = 0;
			this->dumbAssPad = 1;
			break;
		}

		case 0x604:
		case 0x601:
		case 0x602:
		case 0x501:
		case 0x502:
		case 0x503:
		case 0x504:
		case 0x505:
		case 0x506:
			this->field_31C.bothFlags = 0x4000;
			this->dumbAssPad = 0;
			this->field_358 = reinterpret_cast<i32>(angles);

			if (levelId == 0x501)
			{
				this->field_218 |= 0x8000;
			}
			else if (levelId == 0x502)
			{
				this->mHealth = 0x258;
				this->field_460 = 0x78;
				this->field_464 = 0x168;

				this->field_39C = (G_DIFFICULTY_LEVEL == 1 || G_DIFFICULTY_LEVEL == 0) ? 0x3C : 0x14;
				Panel_CreateHealthBar(this, 313);
			}
			else if (levelId == 0x604)
			{
				this->mHealth = 0x258;
				this->field_460 = 0xF0;
				this->field_464 = 0x2D0;

				this->field_39C = (G_DIFFICULTY_LEVEL == 1 || G_DIFFICULTY_LEVEL == 0) ? 0x46 : 0x32;
				this->field_33C = 1;
				Panel_CreateHealthBar(this, 313);
				this->ResolveSwitchNodes();
			}
			break;
	}

	if (*gSubmarinerDieRelated)
	{
		print_if_false(0, "Error");
	}
}

// @Ok
void Venom_CreateVenom(const unsigned int *stack, unsigned int *result) {
	int* v2 = reinterpret_cast<int*>(*stack);
	int v3 = static_cast<int>(stack[1]);

	*result = reinterpret_cast<unsigned int>(new CVenom(v2, v3));
}

// @Ok
void CVenom::Shouldnt_DoPhysics_Be_Virtual(void)
{
	this->DoPhysics();
}

// @BIGTODO
void CVenom::DoPhysics(void)
{}

// @Ok
void CVenomWrap::Die(void)
{
	CBit::Die();
}

// @Ok
// @Matching
INLINE i32* CVenom::KillCommandBlock(i32* a1)
{
	i32* res = reinterpret_cast<i32*>(a1[a1[1]-1]);

	if (this->field_35C == a1)
	{
		this->field_35C = res;
	}
	else
	{
		i32* it = this->field_35C;

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
// @Matching
INLINE void CVenom::KillAllCommandBlocks(void)
{
	for (i32* cur = this->field_35C; cur; cur = this->KillCommandBlock(cur))
		;
	this->field_35C = 0;
}

// @Ok
unsigned char CVenom::TugImpulse(CVector *a2, CVector *a3, CVector *a4)
{
	if (a4)
	{
		Mem_Delete(a4);
	}
	this->field_218 |= 0x200;

	return 0;
}

// @Ok
CVenomElectrified::CVenomElectrified(CSuper* pSuper)
{
	print_if_false(pSuper != 0, "NULL pSuper sent to CVenomWrap");
	print_if_false((pSuper->mType == 313), "Non venom sent to CVenomElectrified");

	this->field_3C = Mem_MakeHandle(pSuper);
}

i32 gVenomFootstepRelated;

// @NotOk
// globals
void CVenom::PlayNextFootstepSFX(void)
{
	i32 i;
	for (i = Rnd(4) + 245; i == gVenomFootstepRelated; i = Rnd(4) + 245)
		;

	gVenomFootstepRelated = i;
	SFX_PlayPos(i | 0x8000, &this->mPos, 0);
}

// @NotOk
// @Validate: when inlined
i32 CVenom::GetTargetPosFromNode(CVector *pVector, i32 a3)
{
	Trig_GetPosition(pVector, a3);

	i32 v5 = Utils_GetGroundHeight(pVector, 0, 0x2000, 0);
	if (v5 == -1)
	{
		return 0;
	}

	pVector->vy = v5 - (this->field_21E << 12);
	return 1;
}

// @Ok
void INLINE CVenom::Lookaround(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_218 &= 0xFFFFFFF8;
			this->mVel.vx = 0;
			this->mVel.vy = 0;
			this->mVel.vz = 0;
			this->RunAnim(8, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 32;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate");
			break;
	}
}

// @Ok
void INLINE CVenom::TugWeb(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->field_218 &= 0xFFFFFFF8;
			this->mVel.vz = 0;
			this->mVel.vy = 0;
			this->mVel.vx = 0;
			this->dumbAssPad++;
		case 1:
			if (this->field_218 & 0x200)
			{
				this->field_218 &= 0xFFFFFDFF;
				if (this->field_10C.pWhatever)
				{
					CTrapWebEffect* pWeb = reinterpret_cast<CTrapWebEffect*>(
							Mem_RecoverPointer(&this->field_10C));

					if (pWeb)
						pWeb->Burst();

					this->field_10C.pWhatever = 0;
				}

				this->field_31C.bothFlags = 32;
				this->dumbAssPad = 0;
			}
			break;
		default:
			break;
	}
}

// @Ok
void CVenom::AdjustWaterModel(void)
{
	//SHandle *pHandle = &this->field_340;
	CBody* pBody = reinterpret_cast<CBody*>(
			Mem_RecoverPointer(&this->field_340));

	if (!pBody)
	{
		u32 Model = Spool_GetModel(0x26D2DBB7, gObjFileRegion);

		for (pBody = EnvironmentalObjectList;
				pBody;
				pBody = reinterpret_cast<CBody*>(pBody->mNextItem))
		{
			if (pBody->mRegion == gObjFileRegion && pBody->mModel == Model)
			{
				this->field_340 = Mem_MakeHandle(pBody);
				this->field_348 = pBody->mPos.vy;
				break;
			}
		}
	}

	if (pBody)
	{
		pBody->mPos.vy = this->field_348 - 18 * this->field_338;
	}

}

// @Ok
// @Matching
void CVenom::PulseL6A4Node(bool a2)
{
	CVector v3;

	for (i32 i = 1; i < NumNodes; i++)
	{
		if (*G_OFFSETLIST[i] == 1)
		{
			Trig_GetPosition(&v3, i);

			if (a2)
			{
				if (!(v3.vz | v3.vy | v3.vx))
				{
					Trig_SendPulseToNode(i);
					return;
				}
			}
			else
			{
				if (v3.vx == 0x3E8000 && !(v3.vz | v3.vy))
				{
					Trig_SendPulseToNode(i);
					return;
				}
			}
		}
	}

	DoAssert(0, "Node not found");
}


// @Ok
void CVenom::VenomDie(void)
{
	switch (this->dumbAssPad)
	{

		case 0:
			this->field_330 = 16;
			this->field_218 &= 0xFFFFFFF8;
			this->mVel.vx = 0;
			this->mVel.vy = 0;
			this->mVel.vz = 0;

			this->mCBodyFlags &= 0xFFEF;
			this->mFlags &= 0xFFBE;
			this->field_218 &= 0xFFFFFE7F;
			this->dumbAssPad++;
			break;
		case 1:
			this->field_330 = 16;
			if (this->mAnimFinished)
			{
				this->RunAnim(0x29, 0, -1);
				this->dumbAssPad++;
			}

			break;
		case 2:
			if (this->mAnimFinished)
			{
				if (Trig_GetLevelID() == 1540)
				{
					this->PulseL6A4Node(false);
					this->dumbAssPad++;
				}
				else
				{
					this->Die(0);
				}
			}
			break;
	}
}

void validate_CVenom(void){
	VALIDATE_SIZE(CVenom, 0x468);

	VALIDATE(CVenom, field_330, 0x330);
	VALIDATE(CVenom, field_334, 0x334);
	VALIDATE(CVenom, field_338, 0x338);

	VALIDATE(CVenom, field_33C, 0x33C);
	VALIDATE(CVenom, field_33D, 0x33D);

	VALIDATE(CVenom, field_340, 0x340);
	VALIDATE(CVenom, field_348, 0x348);

	VALIDATE(CVenom, field_358, 0x358);
	VALIDATE(CVenom, field_35C, 0x35C);

	VALIDATE(CVenom, field_37C, 0x37C);
	VALIDATE(CVenom, field_380, 0x380);
	VALIDATE(CVenom, field_384, 0x384);

	VALIDATE(CVenom, field_390, 0x390);

	VALIDATE(CVenom, field_398, 0x398);
	VALIDATE(CVenom, field_39C, 0x39C);

	VALIDATE(CVenom, field_3A0, 0x3A0);
	VALIDATE(CVenom, field_3A4, 0x3A4);
	VALIDATE(CVenom, field_3A8, 0x3A8);

	VALIDATE(CVenom, field_3B0, 0x3B0);
	VALIDATE(CVenom, field_3B4, 0x3B4);

	VALIDATE(CVenom, field_3B8, 0x3B8);
	VALIDATE(CVenom, field_3BC, 0x3BC);
	VALIDATE(CVenom, field_3CC, 0x3CC);
	VALIDATE(CVenom, field_3D0, 0x3D0);

	VALIDATE(CVenom, field_3E4, 0x3E4);

	VALIDATE(CVenom, field_3E8, 0x3E8);
	VALIDATE(CVenom, field_3EC, 0x3EC);
	VALIDATE(CVenom, field_3F0, 0x3F0);
	VALIDATE(CVenom, field_400, 0x400);
	VALIDATE(CVenom, field_404, 0x404);
	VALIDATE(CVenom, field_408, 0x408);
	VALIDATE(CVenom, field_40C, 0x40C);
	VALIDATE(CVenom, field_410, 0x410);
	VALIDATE(CVenom, field_414, 0x414);
	VALIDATE(CVenom, field_418, 0x418);
	VALIDATE(CVenom, field_41C, 0x41C);
	VALIDATE(CVenom, field_420, 0x420);

	VALIDATE(CVenom, field_428, 0x428);

	VALIDATE(CVenom, field_430, 0x430);
	VALIDATE(CVenom, field_434, 0x434);


	VALIDATE(CVenom, field_454, 0x454);
	VALIDATE(CVenom, field_458, 0x458);
	VALIDATE(CVenom, field_45C, 0x45C);
	VALIDATE(CVenom, field_460, 0x460);
	VALIDATE(CVenom, field_464, 0x464);

	VALIDATE_VTABLE(CVenom, EnterWaitState, 17);
	VALIDATE_VTABLE(CVenom, ExitWaitState, 18);
}

void validate_CVenomWrap(void)
{
	VALIDATE_SIZE(CVenomWrap, 0x5C);
}

void validate_CVenomElectrified(void)
{
	VALIDATE_SIZE(CVenomElectrified, 0x48);

	VALIDATE(CVenomElectrified, field_3C, 0x3C);
}

void validate_CVenomHitSpark(void)
{
	VALIDATE_SIZE(CVenomHitSpark, 0x84);
}
