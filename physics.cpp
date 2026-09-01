#include "physics.h"

#include "spidey.h"
#include "m3dutils.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "platform.h"

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

// @Ok
// Original 0x467FD0. The per-frame physics for Spidey while he is crawling on
// a surface (walls, ceilings, floors). Integrates mVel/mAcc, then fires four
// short rays: one along the crawl surface from the hook position to catch a
// wall he ran into, two sideways to catch a corner, one from the hook back
// down onto the surface, and finally the "ground" ray from 40 units above the
// player to 140 below, which is the one that actually parks him on the
// surface, sets field_A8 (the surface normal he is aligned to) and reports
// treading on a moving platform.
void CPlayer::DoCrawlingPhysics(void)
{
	CVector prevPos;
	CVector delta;
	CVector hookPos;
	CVector alongUp;
	CVector base;
	CVector snap;
	CSVector savedNormal;
	SLineInfo sideInfo;
	SLineInfo groundInfo;
	i32 dot;
	i32 dist;
	i32 pushOut;
	u32 faceFlags;
	u8 bStopped;

	print_if_false(this->field_AD4, "Error");

	this->mCollision = 0;
	this->field_AD5 = 0;
	this->field_B09 = 0;
	bStopped = 0;

	this->mVel += this->mAcc;
	this->mVel %= this->mFric;
	this->mVel.KillSmall();

	prevPos.vx = this->mPos.vx;
	prevPos.vy = this->mPos.vy;
	prevPos.vz = this->mPos.vz;

	if (this->field_80 <= 2)
	{
		delta.vx = this->mVel.vx;
		delta.vy = this->mVel.vy;
		delta.vz = this->mVel.vz;
	}
	else
	{
		delta = this->mVel + (this->mVel >> 1) * (this->field_80 - 2);
	}

	if ((this->mVel.vx | this->mVel.vy | this->mVel.vz) != 0)
		this->mPos += delta;
	else
		bStopped = 1;

	M3dUtils_GetHookPosition(reinterpret_cast<VECTOR *>(&hookPos), this, 2);

	if (bStopped == 0)
	{
		// -96 units along the crawl surface's forward axis. The one-argument
		// CVector constructor only fills vx, which is all the
		// (CVector, CVector) operator* reads out of its left side.
		alongUp = CVector(-96) * this->field_C6C;

		this->mLineInfo.StartCoords.vx = (hookPos - alongUp / 4).vx;
		this->mLineInfo.StartCoords.vy = (hookPos - alongUp / 4).vy;
		this->mLineInfo.StartCoords.vz = (hookPos - alongUp / 4).vz;
		this->mLineInfo.EndCoords.vx = (hookPos + alongUp).vx;
		this->mLineInfo.EndCoords.vy = (hookPos + alongUp).vy;
		this->mLineInfo.EndCoords.vz = (hookPos + alongUp).vz;

		M3dColij_InitLineInfo(&this->mLineInfo);
		M3dZone_LineToItem(&this->mLineInfo, 1);

		if (this->mLineInfo.pItem != 0)
		{
			dot = ((this->mLineInfo.Normal.vz * this->field_A8.vz) >> 12)
				+ ((this->mLineInfo.Normal.vx * this->field_A8.vx) >> 12)
				+ ((this->mLineInfo.Normal.vy * this->field_A8.vy) >> 12);

			if (dot > 3271)
			{
				// facing roughly the same way as the surface he is already
				// on, so this is not a wall he ran into.
				this->mLineInfo.pItem = 0;
			}
			else
			{
				this->mCollision |= 1;

				this->mPos.vx = prevPos.vx;
				this->mPos.vy = prevPos.vy;
				this->mPos.vz = prevPos.vz;

				if ((this->mLineInfo.pFace[3] & 0x40000) != 0
					&& (this->field_8E8 == 0 || this->mLineInfo.Normal.vy >= -2600))
					this->mLineInfo.pItem = 0;

				if (this->mLineInfo.pItem != 0)
				{
					dist = this->mLineInfo.Distance;

					if (dist >= 80)
						this->field_C58 = 0;
					else
						this->field_C58 = 88 - dist;
				}
			}
		}
		else
		{
			// nothing straight ahead: probe left and right for a corner.
			sideInfo.StartCoords.vx = hookPos.vx;
			sideInfo.StartCoords.vy = hookPos.vy;
			sideInfo.StartCoords.vz = hookPos.vz;
			sideInfo.EndCoords.vx = (hookPos + this->field_C78 * 16).vx;
			sideInfo.EndCoords.vy = (hookPos + this->field_C78 * 16).vy;
			sideInfo.EndCoords.vz = (hookPos + this->field_C78 * 16).vz;

			M3dColij_InitLineInfo(&sideInfo);
			M3dZone_LineToItem(&sideInfo, 1);

			if (sideInfo.pItem != 0)
			{
				this->mPos.vx = prevPos.vx;
				this->mPos.vy = prevPos.vy;
				this->mPos.vz = prevPos.vz;
			}
			else
			{
				sideInfo.EndCoords.vx = (hookPos - this->field_C78 * 16).vx;
				sideInfo.EndCoords.vy = (hookPos - this->field_C78 * 16).vy;
				sideInfo.EndCoords.vz = (hookPos - this->field_C78 * 16).vz;

				M3dColij_InitLineInfo(&sideInfo);
				M3dZone_LineToItem(&sideInfo, 1);

				if (sideInfo.pItem != 0)
				{
					this->mPos.vx = prevPos.vx;
					this->mPos.vy = prevPos.vy;
					this->mPos.vz = prevPos.vz;
				}
			}

			// then a ray from just under the last end point, forwards along
			// the crawl surface, for an interior corner.
			base.vx = this->mLineInfo.EndCoords.vx - 70 * this->field_C84.vx;
			base.vy = this->mLineInfo.EndCoords.vy - 70 * this->field_C84.vy;
			base.vz = this->mLineInfo.EndCoords.vz - 70 * this->field_C84.vz;

			this->mLineInfo.StartCoords.vx = base.vx;
			this->mLineInfo.StartCoords.vy = base.vy;
			this->mLineInfo.StartCoords.vz = base.vz;

			this->mLineInfo.EndCoords = base + this->field_C6C * 128;

			M3dColij_InitLineInfo(&this->mLineInfo);
			M3dZone_LineToItem(&this->mLineInfo, 1);

			if (this->mLineInfo.pItem != 0
				&& (this->mLineInfo.pFace[3] & 0x40000) == 0
				&& ((this->mLineInfo.Normal.vz * this->field_A8.vz) >> 12)
					+ ((this->mLineInfo.Normal.vy * this->field_A8.vy) >> 12)
					+ ((this->mLineInfo.Normal.vx * this->field_A8.vx) >> 12) <= 3271)
			{
				this->field_B08 = 1;

				this->mPos.vx = prevPos.vx;
				this->mPos.vy = prevPos.vy;
				this->mPos.vz = prevPos.vz;

				dist = this->mLineInfo.Distance;

				if (dist <= 16)
					this->field_C54 = 0;
				else
					this->field_C54 = dist - 8;
			}
			else
			{
				this->mLineInfo.pItem = 0;
			}
		}
	}

	// the ground ray: 40 units up the surface normal down to 140 units below.
	groundInfo.StartCoords.vx = this->mPos.vx + 40 * this->field_C84.vx;
	groundInfo.StartCoords.vy = this->mPos.vy + 40 * this->field_C84.vy;
	groundInfo.StartCoords.vz = this->mPos.vz + 40 * this->field_C84.vz;
	groundInfo.EndCoords.vx = this->mPos.vx - 140 * this->field_C84.vx;
	groundInfo.EndCoords.vy = this->mPos.vy - 140 * this->field_C84.vy;
	groundInfo.EndCoords.vz = this->mPos.vz - 140 * this->field_C84.vz;

	M3dColij_InitLineInfo(&groundInfo);
	M3dZone_LineToItem(&groundInfo, 1);

	if (groundInfo.pItem == 0)
	{
		// no surface under him at all: remember the wall in front (if any)
		// and re-run the ground ray from the restored position.
		this->mCollision |= 2;

		groundInfo.StartCoords.vx = this->mPos.vx - 96 * this->field_C6C.vx - 70 * this->field_C84.vx;
		groundInfo.StartCoords.vy = this->mPos.vy - 96 * this->field_C6C.vy - 70 * this->field_C84.vy;
		groundInfo.StartCoords.vz = this->mPos.vz - 96 * this->field_C6C.vz - 70 * this->field_C84.vz;
		groundInfo.EndCoords.vx = groundInfo.StartCoords.vx + (this->field_C6C.vx << 7);
		groundInfo.EndCoords.vy = groundInfo.StartCoords.vy + (this->field_C6C.vy << 7);
		groundInfo.EndCoords.vz = groundInfo.StartCoords.vz + (this->field_C6C.vz << 7);

		M3dColij_InitLineInfo(&groundInfo);
		M3dZone_LineToItem(&groundInfo, 1);

		if (groundInfo.pItem != 0 && (groundInfo.pFace[3] & 0x40000) == 0)
		{
			this->mLineInfo.pItem = groundInfo.pItem;
			this->mLineInfo.Position.vx = groundInfo.Position.vx;
			this->mLineInfo.Position.vy = groundInfo.Position.vy;
			this->mLineInfo.Position.vz = groundInfo.Position.vz;
			this->mLineInfo.Normal.vx = groundInfo.Normal.vx;
			this->field_B08 = 1;
			this->mLineInfo.Normal.vy = groundInfo.Normal.vy;
			this->mLineInfo.Normal.vz = groundInfo.Normal.vz;
		}

		this->mPos.vx = prevPos.vx;
		this->mPos.vy = prevPos.vy;
		this->mPos.vz = prevPos.vz;

		groundInfo.StartCoords.vx = this->mPos.vx + 40 * this->field_C84.vx;
		groundInfo.StartCoords.vy = this->mPos.vy + 40 * this->field_C84.vy;
		groundInfo.StartCoords.vz = this->mPos.vz + 40 * this->field_C84.vz;
		groundInfo.EndCoords.vx = this->mPos.vx - 140 * this->field_C84.vx;
		groundInfo.EndCoords.vy = this->mPos.vy - 140 * this->field_C84.vy;
		groundInfo.EndCoords.vz = this->mPos.vz - 140 * this->field_C84.vz;

		M3dColij_InitLineInfo(&groundInfo);
		M3dZone_LineToItem(&groundInfo, 1);

		if (groundInfo.pItem == 0)
			this->mCollision &= ~2;

		return;
	}

	M3dColij_LineInfoFixup(&groundInfo);

	this->field_EA4 = 4;

	faceFlags = groundInfo.pFace[3] >> 16;

	if ((faceFlags & 0x80) != 0)
		this->AdjustBrightness(this->field_574);
	else
		this->AdjustBrightness(this->field_578);

	this->mCollision |= 2;

	if ((faceFlags & 4) != 0)
	{
		if ((faceFlags & 0x800) != 0)
		{
			this->field_B09 = 1;
		}
		else
		{
			this->mPos.vx = prevPos.vx;
			this->mPos.vy = prevPos.vy;
			this->mPos.vz = prevPos.vz;
		}
		return;
	}

	print_if_false((~(faceFlags >> 11)) & 1, "FU_FENCE not flagged FU_NONCRAWLABLE");

	// crossing between a floor-ish and a wall-ish surface hands the surface
	// normal over to HandleControlsForSurfaceTransition, which reads it out
	// of mLineInfo, so it is swapped in and back out around the call.
	if (groundInfo.Normal.vy > 3400)
	{
		if (this->field_A8.vy <= 3400)
		{
			savedNormal.vx = this->mLineInfo.Normal.vx;
			savedNormal.vy = this->mLineInfo.Normal.vy;
			savedNormal.vz = this->mLineInfo.Normal.vz;

			this->field_AC8.vx = this->field_C6C.vx;
			this->field_AC8.vy = this->field_C6C.vy;
			this->field_AC8.vz = this->field_C6C.vz;

			this->mLineInfo.Normal.vx = groundInfo.Normal.vx;
			this->mLineInfo.Normal.vy = groundInfo.Normal.vy;
			this->mLineInfo.Normal.vz = groundInfo.Normal.vz;

			this->HandleControlsForSurfaceTransition(true);

			this->mLineInfo.Normal.vx = savedNormal.vx;
			this->mLineInfo.Normal.vy = savedNormal.vy;
			this->mLineInfo.Normal.vz = savedNormal.vz;
		}
	}
	else if (this->field_A8.vy > 3400)
	{
		savedNormal.vx = this->mLineInfo.Normal.vx;
		savedNormal.vy = this->mLineInfo.Normal.vy;
		savedNormal.vz = this->mLineInfo.Normal.vz;

		this->mLineInfo.Normal.vx = groundInfo.Normal.vx;
		this->mLineInfo.Normal.vy = groundInfo.Normal.vy;
		this->mLineInfo.Normal.vz = groundInfo.Normal.vz;

		this->HandleControlsForSurfaceTransition(true);

		this->mLineInfo.Normal.vx = savedNormal.vx;
		this->mLineInfo.Normal.vy = savedNormal.vy;
		this->mLineInfo.Normal.vz = savedNormal.vz;
	}

	if ((faceFlags & 0x100) != 0)
		this->field_AD5 = 1;

	pushOut = this->field_EA8;

	if (bStopped != 0)
	{
		// standing still: only creep towards the surface, and only if the
		// correction is worth more than a rounding error.
		snap.vx = groundInfo.Position.vx + groundInfo.Normal.vx * pushOut;
		snap.vy = groundInfo.Position.vy + groundInfo.Normal.vy * pushOut;
		snap.vz = groundInfo.Position.vz + groundInfo.Normal.vz * pushOut;

		snap -= this->mPos;

		if (snap.LengthSquared() > 8)
			this->mPos += snap;
	}
	else
	{
		this->mPos.vx = groundInfo.Position.vx + groundInfo.Normal.vx * pushOut;
		this->mPos.vy = groundInfo.Position.vy + groundInfo.Normal.vy * pushOut;
		this->mPos.vz = groundInfo.Position.vz + groundInfo.Normal.vz * pushOut;
	}

	this->field_A8.vx = groundInfo.Normal.vx;
	this->field_A8.vy = groundInfo.Normal.vy;
	this->field_A8.vz = groundInfo.Normal.vz;

	this->mShadowPos.vx = groundInfo.Position.vx;
	this->mShadowPos.vy = groundInfo.Position.vy;
	this->mShadowPos.vz = groundInfo.Position.vz;

	if ((groundInfo.pItem->mFlags & 0x100) != 0)
	{
		reinterpret_cast<CPlatform *>(groundInfo.pItem)->NotifyTrodUpon(this, &this->mPos, &this->field_A8);
		reinterpret_cast<CPlatform *>(groundInfo.pItem)->AdjustBruceHealth();
		this->field_DBC = reinterpret_cast<CBody *>(groundInfo.pItem);
	}
}
