#include "manipob.h"
#include "utils.h"
#include "validate.h"
#include "trig.h"
#include "mem.h"
#include "spool.h"
#include "baddy.h"
#include "m3dzone.h"

static i16 * const word_610C48 = (i16*)0x610C48;
extern const char *gObjFile;
extern CBody* EnvironmentalObjectList;

// @Ok
void CManipObChunk::AI(void)
{
	this->field_FC -= this->field_80;

	if (this->field_FC <= 0)
	{
		this->Die();
	}
	else
	{
		this->DoPhysics();
	}
}

// @Ok
// @Matching
CManipObChunk::CManipObChunk(u32 a1, CVector *a2, CVector *a3)
{
	this->InitItem(gObjFile);
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->mFlags &= 0xFFFD;
	this->mRMinor = 0;
	this->mType = 401;

	this->mFric.vx = 12;
	this->mFric.vy = 12;
	this->mFric.vz = 12;

	this->mPos = *a2;
	this->mVel = *a3;

	this->mAngles.vx = Rnd(4096);
	this->mAngles.vy = Rnd(4096);

	this->mAcc.vy = 4096;

	this->mAngVel.vx = 64 - Rnd(32);
	this->mAngVel.vy = 64 - Rnd(32);

	this->mModel = Spool_GetModel(a1, gObjFileRegion);

	this->field_FC = Rnd(30) + 60;
}

// @Ok
// Address 0x456630 (0x4574d0 in names.json is a different, unrelated
// function; the real CManipObChunk::DoPhysics starts right after
// CManipObChunk::AI's tail jump at 0x456630 and runs to 0x456918, in the
// gap names.json leaves between CManipObChunk_AI and the CManipOb
// constructor). Substeps mVel/mPos/mAngles field_80 times, casts a probe
// line 64 units past the new position along the normalized movement
// direction, and on the first frame that line hits something, reflects
// mVel off the hit normal and teleports mPos back to the hit point; on any
// later hit it just dies.
void CManipObChunk::DoPhysics(void)
{
	CVector oldPos = this->mPos;

	for (i32 i = this->field_80; i > 0; i--)
	{
		this->mVel += this->mAcc;
		this->mVel %= this->mFric;
		this->mPos += this->mVel;
		this->mAngles += this->mAngVel;
	}

	CVector delta = this->mPos - oldPos;
	CVector dir = delta >> 12;

	VectorNormal(reinterpret_cast<VECTOR*>(&dir), reinterpret_cast<VECTOR*>(&dir));

	CVector offset = CVector(64) * dir;

	SLineInfo lineInfo;
	lineInfo.StartCoords = oldPos;
	lineInfo.EndCoords = this->mPos + offset;
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

	M3dColij_InitLineInfo(&lineInfo);
	M3dZone_LineToItem(&lineInfo, 1);

	if (!lineInfo.pItem)
	{
		return;
	}

	i32 hitCount = ++this->field_F8;

	if (hitCount < 1)
	{
		this->mAngVel.vx = Rnd(128) - 64;
		this->mAngVel.vy = Rnd(2 * this->mAngVel.vy) - this->mAngVel.vy;

		this->mPos = lineInfo.Position;
		this->mPos -= offset;

		i32 length = this->mVel.Length();
		this->mVel /= length;

		i32 dot = -(lineInfo.Normal.vx * this->mVel.vx
		           + lineInfo.Normal.vy * this->mVel.vy
		           + lineInfo.Normal.vz * this->mVel.vz) >> 12;
		i32 scale = dot * 2;

		this->mVel.vx = (((lineInfo.Normal.vx * scale) >> 12) + this->mVel.vx) * length;
		this->mVel.vy = (((lineInfo.Normal.vy * scale) >> 12) + this->mVel.vy) * length;
		this->mVel.vz = (((lineInfo.Normal.vz * scale) >> 12) + this->mVel.vz) * length;
	}
	else
	{
		this->Die();
	}
}

// @Ok
// @Matching
CManipObChunk::~CManipObChunk(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
}

// @Ok
// @AlmostMatching: vector assingment not zero based
u8 CManipOb::TugImpulse(
		CVector *a2,
		CVector *a3)
{
	this->field_110 = *a2 - this->mPos;

	this->mVel = *a3;
	this->mAcc.vy = 4096;

	this->mAngVel.vx = 64 - Rnd(128);
	this->mAngVel.vy = 64 - Rnd(128);
	this->field_10C |= 5;
	return 1;
}

// @Ok
// @Matching
void CManipOb::GetAttachPoint(CVector *a2)
{
	*a2 = this->mPos + this->field_110;
}

// @Ok
// @AlmostMatching: vector assingment is zero based on the og for mVel
void CManipOb::Drop(CVector *a2)
{
	this->mFlags &= 0xF7FFu;
	this->field_10C |= 1u;
	this->field_10C &= 0xFFFFFFF7;

	this->mVel = *a2;

	this->mAcc.vy = 4096;
	this->mAngVel.vx = Rnd(32) + 64;
}

// @Ok
CManipOb::~CManipOb(void)
{
	this->DeleteFrom(&EnvironmentalObjectList);

	if (this->pVectors)
	{
		Mem_Delete(reinterpret_cast<void*>(this->pVectors));
		this->pVectors = 0;
	}
}

// @Ok
CManipOb::CManipOb(
		i16* a2,
		i32 a3)
{
	this->InitItem(gObjFile);
	this->AttachTo(&EnvironmentalObjectList);

	this->mFlags = this->mFlags & 0x0FFFD | 0x10;
	this->mType = 401;

	this->mNode = a3;

	this->mFric.vx = 12;
	this->mFric.vy = 12;
	this->mFric.vz = 12;

	u32* v6 = reinterpret_cast<u32*>(
			(reinterpret_cast<u32>(
								   this->SquirtAngles(reinterpret_cast<i16*>(this->SquirtPos(a2)))) + 3)
			& 0xFFFFFFFC);

	i32 *v8 = reinterpret_cast<i32*>(v6 + 1);
	this->mModel = Spool_GetModel(*v6, gObjFileRegion);

	if (*v8)
	{
		this->field_11C = Spool_FindEnviroItem(*v8++);
	}

	this->field_120 = reinterpret_cast<i32>(v8);

	i32 *v10 = v8;
	while (*v10++);

	u16 v12 = *reinterpret_cast<u16*>(v10);

	if (*reinterpret_cast<u16*>(v10) & 1)
		this->field_10C |= 8;


	if (v12 & 2)
	{
		this->field_10C |= 0x20;
	}

	this->field_108 = *(reinterpret_cast<u16*>(v10) + 1);
	this->field_104 = *(reinterpret_cast<u16*>(v10) + 2);

}

// @NotOk
// @Test
// Globals
void CManipOb::CalculateThrowPositionArray(CVector* pVector, int NumVectors)
{

	this->mNumVectors = NumVectors;

	i32 v15 = (pVector->vx - this->mPos.vx) / NumVectors;
	i32 v16 = (pVector->vy - this->mPos.vy) / NumVectors;
	i32 v1 = (pVector->vz - this->mPos.vz) / NumVectors;

	this->pVectors = reinterpret_cast<CVector*>(DCMem_New(12 * NumVectors, 0, 1, 0, 1));


	i32 v9 = v15 + this->mPos.vx;
	i32 v10 = v16 + this->mPos.vy;
	i32 v11 = v1 + this->mPos.vz;

	i32 v17 = 2048 / (this->mNumVectors - 1);
	i32 v14 = 0;

	i32 index = 0;
	while (index < this->mNumVectors)
	{
		this->pVectors[index].vx = v9;
		this->pVectors[index].vy = v10 - (word_610C48[2 * (v14 & 0xFFF)] << 8);
		this->pVectors[index].vz = v11;

		v9 += v15;
		v10 += v16;
		v11 += v1;
		v14 += v17;
		index++;
	}

	this->field_100 = 0;
}

// @Ok
void CManipOb::ThrowPos(CVector *a2, int a3)
{
	this->CalculateThrowPositionArray(a2, a3);

	this->mFlags &= 0xF7FF;
	this->field_10C |= 1;
	this->mAngVel.vx = Rnd(32) + 64;
}

// @Ok
void CManipOb::Throw(CVector *a2)
{
	this->mFlags &= 0xF7FF;
	this->field_10C |= 1;
	this->mVel = *a2;
	this->mAcc.vy = 4096;
	this->mAngVel.vx = Rnd(32) + 64;
}

// @NotOk
// Revisit because of weird decomp
void CManipOb::Smash(void)
{
	CVector v3;
	v3.vx = 0;
	v3.vy = 4096;
	v3.vz = 0;

	SLineInfo v4;
	v4.StartCoords.vx = 0;
	v4.StartCoords.vy = 0;
	v4.StartCoords.vz = 0;
	v4.EndCoords.vx = 0;
	v4.EndCoords.vy = 0;
	v4.EndCoords.vz = 0;

	v4.MinCoords.vx = 0;
	v4.MinCoords.vy = 0;
	v4.MinCoords.vz = 0;

	v4.MaxCoords.vx = 0;
	v4.MaxCoords.vy = 0;
	v4.MaxCoords.vz = 0;

	v4.Position.vx = 0;
	v4.Position.vy = 0;
	v4.Position.vz = 0;

	v4.Normal.vx = 0;
	v4.Normal.vy = (i16)0xF000;
	v4.Normal.vz = 0;

	this->mVel.vx = 0;
	this->mVel.vy = 0x20000;
	this->mVel.vz = 0x20000;

	this->Chunk(&v4, &v3);
	this->TurnOffShadow();
	this->SendPulse();

	this->Die();
}

// @BIGTODO
void CManipOb::Chunk(SLineInfo*, CVector*)
{
	printf("void CManipOb::Chunk(SLineInfo*, CVector*)");
}

// @Ok
INLINE void CManipOb::TurnOffShadow(void)
{
	CItem *v1 = this->field_11C;
	if (v1)
	{
		CItem *res = reinterpret_cast<CItem*>(PSXRegion[v1->mRegion].ppModels[v1->mModel]);
		res->mFlags |= 0x20;
	}
}

// @Ok
INLINE void CManipOb::SendPulse(void)
{
	if(!this->field_124)
	{
		this->field_124 = 1;
		Trig_SendPulseToNode(reinterpret_cast<i32>(Trig_GetLinksPointer(this->mNode)));
	}
}

// @Ok
void CManipOb::Pickup(void)
{
	this->mFlags |= 0x20;
	this->mFlags |= 0x800;

	this->TurnOffShadow();
	this->SendPulse();
}

void validate_CManipOb(void)
{
	VALIDATE_SIZE(CManipOb, 0x128);

	VALIDATE(CManipOb, mNumVectors, 0xF8);
	VALIDATE(CManipOb, pVectors, 0xFC);
	VALIDATE(CManipOb, field_100, 0x100);

	VALIDATE(CManipOb, field_104, 0x104);
	VALIDATE(CManipOb, field_108, 0x108);

	VALIDATE(CManipOb, field_10C, 0x10C);

	VALIDATE(CManipOb, field_110, 0x110);

	VALIDATE(CManipOb, field_11C, 0x11C);
	VALIDATE(CManipOb, field_120, 0x120);
	VALIDATE(CManipOb, field_124, 0x124);

	VALIDATE_VTABLE(CManipOb, Smash, 5);
}

void validate_CManipObChunk(void)
{
	VALIDATE_SIZE(CManipObChunk, 0x100);

	VALIDATE(CManipObChunk, field_F8, 0xF8);
	VALIDATE(CManipObChunk, field_FC, 0xFC);
}
