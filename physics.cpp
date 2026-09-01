#include "physics.h"

#include "spidey.h"
#include "m3dutils.h"
#include "m3dcolij.h"
#include "m3dzone.h"

EXPORT void Physics_SetGravity(CVector *);

// @Ok
void Physics_SetGravity(CVector *pVec)
{
    static CVector * const gGravity = (CVector*)0x60F7B0;
    static i32 * const gGravityLength = (i32*)0x60F7BC;
    static CVector * const gNormalizedGravity = (CVector*)0x60F888;

    *gGravity = *pVec;
    *gGravityLength = gGravity->Length();
    *gNormalizedGravity = *gGravity / (-*gGravityLength);
}

// @SMALLTODO
// Original 0x4F7270, CSwinger::GetCurrentParams(CVector &). Stub only: the
// real body is
//     *pOut = swinger->field_10C - (swinger->field_108 * swinger->field_170);
// (operator* 0x4E77D0 then operator- 0x4E7760 on three CVector fields of
// CSwinger), but all three sit inside web.h's PADDING(0x17C-0x108) and web.h
// is not this change's to edit. See the note in physics.h.
void CSwinger_GetCurrentParams(i32 *pSwinger, CVector *pOut)
{
	printf("CSwinger_GetCurrentParams(%p, %p)", pSwinger, pOut);
}

// @Ok
// Original 0x467D20. Runs while the player hangs on a web line: the swinger
// object moves mPos along the swing arc, then three short rays look for a
// surface the swing just pushed the player into. Each ray reuses the player's
// own SLineInfo (CPlayer+0xB0C) and, on a hit, parks the player on the
// surface (mPos = hit position pushed out along the surface normal by
// field_EA8) and kills mVel. With no hit at all, mVel becomes the distance
// the swinger moved this frame.
void CPlayer::DoSwingingPhysics(void)
{
	CVector hookStart;
	CVector prevPos;
	CVector hookEnd;
	i32 dist;

	print_if_false(this->field_E64 != 0, "Error");

	prevPos.vx = this->mPos.vx;
	prevPos.vy = this->mPos.vy;
	prevPos.vz = this->mPos.vz;

	M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&hookStart), this, 2);

	CSwinger_GetCurrentParams(this->field_E64, &this->mPos);

	M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&hookEnd), this, 2);

	// extend the hook travel one more step past where the swinger put it,
	// horizontally only.
	hookEnd.vx = 2 * hookEnd.vx - hookStart.vx;
	hookEnd.vz = 2 * hookEnd.vz - hookStart.vz;

	this->mLineInfo.StartCoords.vx = hookStart.vx;
	this->mLineInfo.StartCoords.vy = hookStart.vy;
	this->mLineInfo.StartCoords.vz = hookStart.vz;
	this->mLineInfo.EndCoords.vx = hookEnd.vx;
	this->mLineInfo.EndCoords.vy = hookEnd.vy;
	this->mLineInfo.EndCoords.vz = hookEnd.vz;

	M3dColij_InitLineInfo(&this->mLineInfo);
	M3dZone_LineToItem(&this->mLineInfo, 1);

	if (this->mLineInfo.pItem != 0)
	{
		i16 ny = this->mLineInfo.Normal.vy;

		if (ny >= -2600)
			this->mCollision |= 1;
		else
			this->mCollision |= 2;

		dist = this->field_EA8;

		this->mPos.vx = this->mLineInfo.Normal.vx * dist + this->mLineInfo.Position.vx;
		this->mPos.vy = ny * dist + this->mLineInfo.Position.vy;
		this->mPos.vz = this->mLineInfo.Normal.vz * dist + this->mLineInfo.Position.vz;

		this->mVel.vz = 0;
		this->mVel.vy = 0;
		this->mVel.vx = 0;
		return;
	}

	// nothing in the way sideways: try straight up from the extended hook.
	this->mLineInfo.StartCoords.vx = hookEnd.vx;
	this->mLineInfo.StartCoords.vy = hookEnd.vy;
	this->mLineInfo.StartCoords.vz = hookEnd.vz;
	this->mLineInfo.EndCoords.vx = hookEnd.vx;
	this->mLineInfo.EndCoords.vy = hookEnd.vy + 0x80000;
	this->mLineInfo.EndCoords.vz = hookEnd.vz;

	M3dColij_InitLineInfo(&this->mLineInfo);
	M3dZone_LineToItem(&this->mLineInfo, 1);

	if (this->mLineInfo.pItem != 0)
	{
		this->mCollision |= 2;

		dist = this->field_EA8;

		this->mPos.vx = this->mLineInfo.Normal.vx * dist + this->mLineInfo.Position.vx;
		this->mPos.vy = this->mLineInfo.Normal.vy * dist + this->mLineInfo.Position.vy;
		this->mPos.vz = this->mLineInfo.Normal.vz * dist + this->mLineInfo.Position.vz;

		this->mVel.vz = 0;
		this->mVel.vy = 0;
		this->mVel.vx = 0;
		return;
	}

	// and finally straight down from the same start.
	this->mLineInfo.EndCoords.vx = this->mLineInfo.StartCoords.vx;
	this->mLineInfo.EndCoords.vy = this->mLineInfo.StartCoords.vy - 0x20000;
	this->mLineInfo.EndCoords.vz = this->mLineInfo.StartCoords.vz;

	M3dColij_InitLineInfo(&this->mLineInfo);
	M3dZone_LineToItem(&this->mLineInfo, 1);

	if (this->mLineInfo.pItem != 0)
	{
		this->mCollision |= 1;

		dist = this->field_EA8;

		this->mPos.vx = this->mLineInfo.Normal.vx * dist + this->mLineInfo.Position.vx;
		this->mPos.vy = this->mLineInfo.Normal.vy * dist + this->mLineInfo.Position.vy;
		this->mPos.vz = this->mLineInfo.Normal.vz * dist + this->mLineInfo.Position.vz;

		this->mVel.vz = 0;
		this->mVel.vy = 0;
		this->mVel.vx = 0;
		return;
	}

	this->mVel = this->mPos - prevPos;
}
