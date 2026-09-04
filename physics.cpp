// physics.cpp: the player's movement code. The Mac build's own physics.cpp
// holds exactly these four functions, in this order:
//   Physics_SetGravity          Mac 0x0A7270  PC 0x466C70
//   CPlayer::DoPhysics          Mac 0x0A7340  PC 0x466CE0  (0x1017 bytes)
//   CPlayer::DoSwingingPhysics  Mac 0x0A82A0  PC 0x467D20  (0x2A8 bytes)
//   CPlayer::DoCrawlingPhysics  Mac 0x0A8640  PC 0x467FD0  (0xD6F bytes)
// Both ends of the run are named the same way on both builds and nothing
// else sits between them, so the two middle slots follow.
//
// Two names are wrong in tools/names.json AND in the maintainer's IDB
// (idbs/spideypc_names.txt), worth reporting upstream:
//   0x4BFEC0 is labelled "CPlayer_DoPhysics" in both. It is really
//   CPlayer::CheckStickToWall (Mac orders CheckStickToCeiling 0x1194B0,
//   CheckStickToWall 0x1196B0, CheckKick 0x1198B0; the PC has 0x4BFCE0,
//   <0x4BFEC0>, 0x4C00B0). It is decompiled under the right name in
//   spidey.cpp.
//   0x466CE0 (DoPhysics) and 0x467D20 (DoSwingingPhysics) are missing from
//   both name sources.
// A third, found while doing 0x467FD0: the unnamed sub_4BEA90 is
// CPlayer::HandleControlsForSurfaceTransition(bool). Mac orders
// SwitchToStandMode / CheckFenceSurfaceTransition /
// HandleControlsForSurfaceTransition / CheckInteriorSurfaceTransition, the
// PC has 0x4BE4B0 / 0x4BE8C0 / 0x4BEA90 / 0x4BEB70, and DoCrawlingPhysics
// calls sub_4BEA90(1) right after swapping a surface normal into
// mLineInfo.Normal, which is exactly what that function reads.

#include "physics.h"

#include "spidey.h"
#include "m3dutils.h"
#include "m3dcolij.h"
#include "m3dzone.h"
#include "platform.h"
#include "ps2funcs.h"
#include "baddy.h"
#include "utils.h"

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

// @Ok
// Original 0x4F7270, CSwinger::GetCurrentParams(CVector &). Read from the raw
// disassembly rather than the pseudocode, because the field layout is unusual:
//
//   lea ecx,[esi+108h] / lea eax,[esi+170h] -> operator*(CVector,CVector) 0x4E77D0
//   then [esi+10Ch] minus that result       -> operator-(CVector,CVector) 0x4E7760
//
// Those members OVERLAP as three CVectors: 0x108 and 0x10C cannot both start a
// 12-byte vector. That is deliberate, not a misread. operator* at 0x4E77D0
// reads ONLY lhs.vx - its body is "mov eax,[eax]" followed by three imuls
// against that one value, never touching [eax+4] or [eax+8] - so the original
// is using the CVector*CVector overload as a scalar broadcast, the scalar
// being the single dword at 0x108. Real layout: scalar at 0x108, CVector at
// 0x10C, CVector at 0x170.
//
// Reached by byte offset instead of carving web.h's PADDING(0x17C-0x108),
// because writing that overlap into the class declaration would be wrong.
// Whoever names these members should encode the scalar/vector split above.
//
// NOTE for anyone auditing vector.cpp: the lhs.vx broadcast in
// operator*(const CVector&, const CVector&) is FAITHFUL to the original, not a
// transcription bug. It looks just like the genuine bug fixed in operator+ on
// 2026-08-31, so do not "correct" it.
void CSwinger_GetCurrentParams(i32 *pSwinger, CVector *pOut)
{
	const char *pBase = reinterpret_cast<const char*>(pSwinger);

	const CVector &Scale  = *reinterpret_cast<const CVector*>(pBase + 0x108);
	const CVector &Origin = *reinterpret_cast<const CVector*>(pBase + 0x10C);
	const CVector &Offset = *reinterpret_cast<const CVector*>(pBase + 0x170);

	*pOut = Origin - (Scale * Offset);
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

#ifdef SPIDEY_STANDALONE
	if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16)
		fprintf(stderr, "CRAWLIN pos=(%d,%d,%d) vel=(%d,%d,%d) acc=(%d,%d,%d) f80=%d anim=%d frame=%d\n", this->mPos.vx, this->mPos.vy, this->mPos.vz,
			this->mVel.vx, this->mVel.vy, this->mVel.vz, this->mAcc.vx, this->mAcc.vy, this->mAcc.vz, this->field_80, this->mAnim, this->mFrame);
#endif

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

#ifdef SPIDEY_STANDALONE
		if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16)
		{
			i32 dbgDot = this->mLineInfo.pItem ? ((this->mLineInfo.Normal.vz * this->field_A8.vz) >> 12)
				+ ((this->mLineInfo.Normal.vx * this->field_A8.vx) >> 12)
				+ ((this->mLineInfo.Normal.vy * this->field_A8.vy) >> 12) : 0;
			fprintf(stderr, "CRAWL hook=(%d,%d,%d) along=(%d,%d,%d) start=(%d,%d,%d) end=(%d,%d,%d) hit=%p normal=(%d,%d,%d) dot=%d dist=%d face3=%#x pos=(%d,%d,%d)\n",
				hookPos.vx, hookPos.vy, hookPos.vz, alongUp.vx, alongUp.vy, alongUp.vz,
				this->mLineInfo.StartCoords.vx, this->mLineInfo.StartCoords.vy, this->mLineInfo.StartCoords.vz,
				this->mLineInfo.EndCoords.vx, this->mLineInfo.EndCoords.vy, this->mLineInfo.EndCoords.vz,
				(void*)this->mLineInfo.pItem, this->mLineInfo.Normal.vx, this->mLineInfo.Normal.vy, this->mLineInfo.Normal.vz, dbgDot,
				this->mLineInfo.pItem ? (i32)this->mLineInfo.Distance : -1, this->mLineInfo.pItem ? (u32)this->mLineInfo.pFace[3] : 0u,
				this->mPos.vx >> 12, this->mPos.vy >> 12, this->mPos.vz >> 12);
		}
#endif

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
#ifdef SPIDEY_STANDALONE
				if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16) fprintf(stderr, "CRAWL SIDEHIT right\n");
#endif
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
#ifdef SPIDEY_STANDALONE
					if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16) fprintf(stderr, "CRAWL SIDEHIT left\n");
#endif
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
#ifdef SPIDEY_STANDALONE
				if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16) fprintf(stderr, "CRAWL CORNERHIT dist=%d normal=(%d,%d,%d)\n", (i32)this->mLineInfo.Distance, this->mLineInfo.Normal.vx, this->mLineInfo.Normal.vy, this->mLineInfo.Normal.vz);
#endif

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
#ifdef SPIDEY_STANDALONE
	{
		extern i32 gDbgColij;
		i32 py = this->mPos.vy >> 12;
		static i32 dbgFull = 0;
		gDbgColij = (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16 && py >= 4530 && py <= 4552) ? 1 : 0;
		if (gDbgColij && py <= 4540 && dbgFull < 1) { gDbgColij = 2; dbgFull++; }
		if (gDbgColij) fprintf(stderr, "CRAWL GROUNDQUERY py=%d\n", py);
	}
#endif
	M3dZone_LineToItem(&groundInfo, 1);
#ifdef SPIDEY_STANDALONE
	{ extern i32 gDbgColij; gDbgColij = 0; }
#endif

#ifdef SPIDEY_STANDALONE
	if (getenv("SPIDEY_TRACE_CRAWL") && this->field_E1C == 16)
		fprintf(stderr, "CRAWL GROUND start=(%d,%d,%d) end=(%d,%d,%d) hit=%p normal=(%d,%d,%d) dist=%d face3=%#x\n",
			groundInfo.StartCoords.vx, groundInfo.StartCoords.vy, groundInfo.StartCoords.vz, groundInfo.EndCoords.vx, groundInfo.EndCoords.vy, groundInfo.EndCoords.vz,
			(void*)groundInfo.pItem, groundInfo.pItem ? groundInfo.Normal.vx : 0, groundInfo.pItem ? groundInfo.Normal.vy : 0, groundInfo.pItem ? groundInfo.Normal.vz : 0,
			groundInfo.pItem ? (i32)groundInfo.Distance : -1, groundInfo.pItem ? (u32)groundInfo.pFace[3] : 0u);
#endif

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

// Original 0x0054EC7C, a const 1 sitting in read-only data. CPlayer::DoPhysics
// is the only reader and nothing writes it, so it reads like a build-time
// switch. When it is set, a collision this frame cancels an upward move but
// still lets a downward one through. Tentative name.
static i32 * const gClipUpwardsMoveOnCollision = (i32*)0x0054EC7C;

// Original 0x0054EC80, same shape and same single reader: the version of the
// rule above for the fall applied after the ground ray misses. Tentative name.
static i32 * const gClipUpwardsFallOnGroundHit = (i32*)0x0054EC80;

// Original 0x0054EC84, the const float 10.0. It is the sideways offset, in
// world units, of the eight extra ground probes DoPhysics fires when the
// straight-down one misses. Tentative name.
static float * const gGroundProbeSpread = (float*)0x0054EC84;

// Original 0x0060F894, an i32 in .data that starts at 0 and that nothing in
// the binary writes, so the eight-probe fan it gates is dead code in the
// shipped build. Kept because the original still tests it. Tentative name.
static i32 * const gUseWideGroundProbe = (i32*)0x0060F894;

// @Ok
// Original 0x466CE0. Spider-Man's per-frame physics while he is neither web
// swinging nor crawling: integrate mVel/mAcc, sweep the movement against the
// world (up to three times, sliding along whatever it hits), then drop a ray
// straight down to land him. Dispatches to DoSwingingPhysics and
// DoCrawlingPhysics for the other two modes.
void CPlayer::DoPhysics(void)
{
	CVector startPos;
	CVector move;
	CVector ray;
	CVector radial;
	CVector slideNormal;
	SLineInfo lineInfo;
	CBody *pLastPlatform;
	i32 savedFallVel;
	i32 savedPosY;
	i32 len3;
	i32 lenXZ;
	i32 tryCount;
	i32 reach;
	i32 reachStep;
	i32 sqx;
	i32 sqz;
	i32 px;
	i32 py;
	i32 pz;
	i32 dot;
	i32 fall;
	i32 dx;
	i32 dz;
	i32 sideX;
	i32 sideZ;
	i32 fwdX;
	i32 fwdY;
	i32 fwdZ;
	i32 angle;
	i32 iStartY;
	i32 iEndY;
	i32 probe;
	float fx;
	float fz;
	float fStartY;
	float fEndY;
	float sx;
	float sz;
	float ex;
	float ez;
	float half;

	this->mCollision = 0;
	this->field_B08 = 0;
	this->mLineInfo.pItem = 0;
	this->mLineInfo2.pItem = 0;

	pLastPlatform = this->field_DBC;
	this->field_DBC = 0;

	if (this->field_E64 != 0)
	{
		this->DoSwingingPhysics();
		return;
	}

	if (this->field_AD4 != 0)
	{
		this->DoCrawlingPhysics();
		return;
	}

	startPos.vx = this->mPos.vx;
	startPos.vy = this->mPos.vy;
	startPos.vz = this->mPos.vz;

	this->mVel += this->mAcc;
	this->mVel %= this->mFric;
	this->mVel.KillSmall();

	// one animation window moves the player with no collision at all.
	if (this->field_E1C == 0x40000
		&& ((this->mAnim == 270 && this->mFrame >= 13) || this->mAnim == 271))
	{
		this->mPos += this->mVel;

		if (this->field_80 > 2)
			this->mPos += this->mVel * (this->field_80 - 2);

		return;
	}

	// the fall is taken out of mVel here and put back by the ground ray at
	// the bottom, so the horizontal sweep never sees it.
	savedFallVel = -1;

	if (this->mVel.vy >= 0)
	{
		savedFallVel = this->mVel.vy;
		this->mVel.vy = 0;
	}

	if (this->field_80 <= 2)
	{
		move.vx = this->mVel.vx;
		move.vy = this->mVel.vy;
		move.vz = this->mVel.vz;
	}
	else
	{
		move = this->mVel + (this->mVel >> 1) * (this->field_80 - 2);
	}

	if (this->field_EF4 != 0)
	{
		// boss arena: bend the movement so the player stays on a circle of
		// radius field_EF8 around gBossRelated.
		dx = this->mPos.vx - reinterpret_cast<CItem *>(gBossRelated)->mPos.vx;
		dz = this->mPos.vz - reinterpret_cast<CItem *>(gBossRelated)->mPos.vz;

		radial.vx = dx + move.vx;
		radial.vy = 0;
		radial.vz = dz + move.vz;

		if (radial.Length() != this->field_EF8)
		{
			radial >>= 8;
			VectorNormal(reinterpret_cast<VECTOR *>(&radial), reinterpret_cast<VECTOR *>(&radial));
			radial *= this->field_EF8;

			move.vx = radial.vx - dx;
			move.vz = radial.vz - dz;
		}
	}

	sqx = (move.vx >> 9) * (move.vx >> 9);
	sqz = (move.vz >> 9) * (move.vz >> 9);
	tryCount = 0;

	// @Note original defect, kept: SLineInfo's members that have
	// constructors get zeroed here, but pItem does not, and it is only
	// written by M3dColij_InitLineInfo. If the player did not move at all
	// this frame the sweep below never runs, and the "did I hit anything"
	// test after it reads pItem uninitialized.
	len3 = M3dMaths_SquareRoot0(sqz + sqx + (move.vy >> 9) * (move.vy >> 9));

	while (len3 != 0)
	{
		lenXZ = M3dMaths_SquareRoot0(sqx + sqz);

		if (lenXZ == 0)
			break;

		reach = this->field_EAA;
		reachStep = reach;

		if (this->field_80 >= 5)
			reachStep = reach + (this->field_80 - 4) * (reach >> 1);

		px = this->mPos.vx;
		py = this->mPos.vy - 0x40000;
		pz = this->mPos.vz;

		// @Note the original scales x and z by the frame-rate-adjusted
		// reachStep but y by the raw field_EAA. Kept as it is.
		ray.vx = move.vx * 8 * reachStep / len3;
		ray.vy = move.vy * 8 * reach / len3;
		ray.vz = move.vz * 8 * reachStep / len3;
		ray += ray >> 3;

		if (this->mHeldObject != 0)
		{
			// a carried object sweeps a second, lower line.
			lineInfo.StartCoords.vx = px;
			lineInfo.StartCoords.vy = py - 0x80000;
			lineInfo.StartCoords.vz = pz;
			lineInfo.EndCoords.vx = px + ray.vx;
			lineInfo.EndCoords.vy = py + ray.vy - 0x80000;
			lineInfo.EndCoords.vz = pz + ray.vz;

			M3dColij_InitLineInfo(&lineInfo);
			M3dZone_LineToItem(&lineInfo, 1);

			if (lineInfo.pItem != 0)
				this->mCollision |= 0x40;
		}

		lineInfo.StartCoords.vx = px - ray.vx / 4;
		lineInfo.StartCoords.vy = py - ray.vy / 4;
		lineInfo.StartCoords.vz = pz - ray.vz / 4;
		lineInfo.EndCoords.vx = px + ray.vx;
		lineInfo.EndCoords.vy = py + ray.vy;
		lineInfo.EndCoords.vz = pz + ray.vz;

		M3dColij_InitLineInfo(&lineInfo);
		M3dZone_LineToItem(&lineInfo, 1);

		tryCount++;

		if (lineInfo.pItem != 0)
		{
			this->mCollision = (this->mCollision & 0xFFBF) | 1;

			this->mLineInfo.pFace = lineInfo.pFace;
			this->mLineInfo.pItem = lineInfo.pItem;
			this->mLineInfo.Position.vx = lineInfo.Position.vx;
			this->mLineInfo.Position.vy = lineInfo.Position.vy;
			this->mLineInfo.Position.vz = lineInfo.Position.vz;
			this->mLineInfo.Normal.vx = lineInfo.Normal.vx;
			this->mLineInfo.Normal.vy = lineInfo.Normal.vy;
			this->mLineInfo.Normal.vz = lineInfo.Normal.vz;

			print_if_false((lineInfo.Normal.vz | lineInfo.Normal.vy | lineInfo.Normal.vx) != 0,
				"Bad normal");

			// flatten the hit normal into the horizontal plane and slide
			// along it.
			slideNormal.vx = lineInfo.Normal.vx;
			slideNormal.vy = 0;
			slideNormal.vz = lineInfo.Normal.vz;
			VectorNormal(reinterpret_cast<VECTOR *>(&slideNormal), reinterpret_cast<VECTOR *>(&slideNormal));

			dot = (move.vz >> 6) * slideNormal.vz + (move.vx >> 6) * slideNormal.vx;

			if (dot <= 0)
			{
				move.vx = (move.vx - (((dot >> 12) * slideNormal.vx) >> 6)) >> 2;
				move.vz = (move.vz - (((dot >> 12) * slideNormal.vz) >> 6)) >> 2;
			}
		}
		else
		{
			// nothing straight ahead: sweep two lines out to either side of
			// the movement direction, at shoulder height.
			sideZ = move.vz * 4 * reach / len3;
			sideX = -(move.vx * 4 * reach / len3);
			fwdX = move.vx * 8 * reachStep / lenXZ;
			fwdY = move.vy * 8 * reach / len3;
			fwdZ = move.vz * 8 * reachStep / lenXZ;

			px = this->mPos.vx;
			py = this->mPos.vy + 0x10000;
			pz = this->mPos.vz;

			lineInfo.StartCoords.vx = px + sideZ / 4 - fwdX / 2;
			lineInfo.StartCoords.vy = py;
			lineInfo.StartCoords.vz = pz + sideX / 4 - fwdZ / 2;
			lineInfo.EndCoords.vx = px + sideZ + fwdX;
			lineInfo.EndCoords.vy = py + fwdY;
			lineInfo.EndCoords.vz = pz + sideX + fwdZ;

			M3dColij_InitLineInfo(&lineInfo);
			M3dZone_LineToItem(&lineInfo, 1);

			if (lineInfo.pItem == 0)
			{
				lineInfo.StartCoords.vx = px - fwdX / 2 - sideZ / 4;
				lineInfo.StartCoords.vy = py;
				lineInfo.StartCoords.vz = pz - fwdZ / 2 - sideX / 4;
				lineInfo.EndCoords.vx = px + fwdX - sideZ;
				lineInfo.EndCoords.vy = py + fwdY;
				lineInfo.EndCoords.vz = pz - sideX + fwdZ;

				M3dColij_InitLineInfo(&lineInfo);
				M3dZone_LineToItem(&lineInfo, 1);

				if (lineInfo.pItem == 0)
					break;
			}

			this->mCollision = (this->mCollision & 0xFFBF) | 1;

			// the side hits slide against the raw normal, not the
			// renormalized one the head-on hit uses.
			dot = (move.vz >> 6) * lineInfo.Normal.vz + (move.vx >> 6) * lineInfo.Normal.vx;

			if (dot <= 0)
			{
				move.vx = (move.vx - (((dot >> 12) * lineInfo.Normal.vx) >> 6)) >> 2;
				move.vz = (move.vz - (((dot >> 12) * lineInfo.Normal.vz) >> 6)) >> 2;
			}
		}

		if (lineInfo.pItem == 0 || tryCount >= 2)
			break;

		sqx = (move.vx >> 9) * (move.vx >> 9);
		sqz = (move.vz >> 9) * (move.vz >> 9);
		len3 = M3dMaths_SquareRoot0(sqx + sqz + (move.vy >> 9) * (move.vy >> 9));
	}

	if (this->field_80 <= 2)
	{
		this->mVel.vx = move.vx;
		this->mVel.vy = move.vy;
		this->mVel.vz = move.vz;
	}
	else
	{
		this->mVel = (move << 1) / this->field_80;
	}

	savedPosY = this->mPos.vy;

	if (lineInfo.pItem == 0)
	{
		this->mPos += move;
	}
	else if (*gClipUpwardsMoveOnCollision == 0 || move.vy < 0)
	{
		this->mPos.vy = savedPosY + move.vy;
	}

	if (this->field_E1C == 1)
	{
		// bouncing state: keep skidding off walls, and pick a fresh random
		// heading whenever there is no usable wall.
		this->mLineInfo2.StartCoords.vx = this->mPos.vx;
		this->mLineInfo2.StartCoords.vy = this->mPos.vy;
		this->mLineInfo2.StartCoords.vz = this->mPos.vz;
		this->mLineInfo2.EndCoords.vx = this->mPos.vx + this->field_E94.vx;
		this->mLineInfo2.EndCoords.vy = this->mPos.vy;
		this->mLineInfo2.EndCoords.vz = this->mPos.vz + this->field_E94.vz;

		M3dColij_InitLineInfo(&this->mLineInfo2);
		M3dZone_LineToItem(&this->mLineInfo2, 1);

		if (this->mLineInfo2.pItem != 0
			&& this->mLineInfo2.Normal.vy >= -2600
			&& this->mLineInfo2.Normal.vy <= 3400)
		{
			this->mVel.vx += 4 * this->mLineInfo2.Normal.vx;
			this->mVel.vz += 4 * this->mLineInfo2.Normal.vz;
			this->field_E94.vx = -32 * this->mLineInfo2.Normal.vx;
			this->field_E94.vz = -32 * this->mLineInfo2.Normal.vz;
		}
		else
		{
			this->field_E90 = this->field_E90 + Rnd(64) + 256;
			angle = this->field_E90 & 0xFFF;
			this->field_E94.vx = G_RCOSSIN_TBL[angle].sin << 5;
			this->field_E94.vz = G_RCOSSIN_TBL[angle].cos << 5;
		}
	}

	// falling, or standing on something that is falling: look for the ground
	// he is about to drop onto and stop him on it.
	if (this->mVel.vy < 0
		|| (pLastPlatform != 0 && pLastPlatform->mVel.vy < 0))
	{
		this->mLineInfo2.StartCoords.vx = this->mPos.vx;
		this->mLineInfo2.StartCoords.vy = this->mPos.vy - move.vy + 0x40000;
		this->mLineInfo2.StartCoords.vz = this->mPos.vz;
		this->mLineInfo2.EndCoords.vx = this->mPos.vx;
		this->mLineInfo2.EndCoords.vy = this->mPos.vy - 0xB8000;
		this->mLineInfo2.EndCoords.vz = this->mPos.vz;

		M3dColij_InitLineInfo(&this->mLineInfo2);
		M3dZone_LineToItem(&this->mLineInfo2, 1);

		if (this->mLineInfo2.pItem != 0
			&& this->mPos.vy - 0x78000 < this->mLineInfo2.Position.vy)
		{
			this->mPos.vy = savedPosY;
			this->mVel.vy = 0;
			this->mCollision = (this->mCollision & 0xFFBF) | 0x100;
		}
	}

	if (savedFallVel < 0)
	{
		this->mAngles.Mask();
		return;
	}

	fall = savedFallVel;

	if (this->field_80 > 2)
		fall = savedFallVel + (this->field_80 - 2) * (savedFallVel >> 1);

	lineInfo.StartCoords.vx = this->mPos.vx;
	lineInfo.StartCoords.vy = this->mPos.vy - 0x28000;
	lineInfo.StartCoords.vz = this->mPos.vz;
	lineInfo.EndCoords.vx = this->mPos.vx;
	lineInfo.EndCoords.vy = fall + (this->mPos.vy - 0x28000) + 0x8C000;
	lineInfo.EndCoords.vz = this->mPos.vz;

	fz = (float)lineInfo.StartCoords.vz / 4096.0f;
	fStartY = (float)lineInfo.StartCoords.vy / 4096.0f;
	fx = (float)lineInfo.StartCoords.vx / 4096.0f;
	fEndY = (float)lineInfo.EndCoords.vy / 4096.0f;

	M3dColij_InitLineInfo(&lineInfo);
	M3dZone_LineToItem(&lineInfo, 1);

	if (*gUseWideGroundProbe != 0)
	{
		if (lineInfo.pItem == 0)
		{
			// dead in the shipped build (gUseWideGroundProbe is always 0):
			// eight more ground probes offset around the player, then one
			// last unoffset repeat.
			iStartY = (i32)(fStartY * 4096.0f);
			iEndY = (i32)(fEndY * 4096.0f);

			for (probe = 0; probe <= 8; probe++)
			{
				sx = fx;
				sz = fz;
				ex = fx;
				ez = fz;

				switch (probe)
				{
					case 0:
						ex = fx + *gGroundProbeSpread;
						sx = ex;
						break;

					case 1:
						ex = fx - *gGroundProbeSpread;
						sx = ex;
						break;

					case 2:
						ez = fz + *gGroundProbeSpread;
						sz = ez;
						break;

					case 3:
						ez = fz - *gGroundProbeSpread;
						sz = ez;
						break;

					case 4:
						half = *gGroundProbeSpread * 0.5f;
						ex = half + fx;
						sx = ex;
						ez = half + fz;
						sz = ez;
						break;

					case 5:
						half = *gGroundProbeSpread * 0.5f;
						ex = half + fx;
						sx = ex;
						ez = fz - half;
						sz = ez;
						break;

					case 6:
						half = *gGroundProbeSpread * 0.5f;
						ex = fx - half;
						sx = ex;
						ez = half + fz;
						sz = ez;
						break;

					case 7:
						half = *gGroundProbeSpread * 0.5f;
						ex = fx - half;
						sx = ex;
						ez = fz - half;
						sz = ez;
						break;

					default:
						break;
				}

				lineInfo.StartCoords.vx = (i32)(sx * 4096.0f);
				lineInfo.StartCoords.vy = iStartY;
				lineInfo.StartCoords.vz = (i32)(sz * 4096.0f);
				lineInfo.EndCoords.vx = (i32)(ex * 4096.0f);
				lineInfo.EndCoords.vy = iEndY;
				lineInfo.EndCoords.vz = (i32)(ez * 4096.0f);

				M3dColij_InitLineInfo(&lineInfo);
				M3dZone_LineToItem(&lineInfo, 1);

				if (lineInfo.pItem != 0)
					break;
			}
		}
	}

	if (lineInfo.pItem == 0)
	{
		this->mPos.vy += fall;
		this->mVel.vy = savedFallVel;
		this->mAngles.Mask();
		return;
	}

	this->field_EA4 = 4;

	if (this->mHeldObject != 0 && (lineInfo.pFace[3] & 0x4000000) != 0)
	{
		// carrying something onto a surface flagged as no-go: undo the whole
		// frame's movement.
		this->mPos.vx = startPos.vx;
		this->mPos.vy = startPos.vy;
		this->mPos.vz = startPos.vz;
		this->mCollision |= 2;
	}
	else if (lineInfo.Normal.vy < -2600)
	{
		// hit a ceiling on the way up.
		lineInfo.Position.vy = lineInfo.Position.vy & 0xFFFFF000;
		this->mPos.vy = lineInfo.Position.vy - (this->field_EA8 << 12);
		this->mVel.vy = 0;
		this->mCollision |= 2;
	}
	else if (lineInfo.Distance > 0)
	{
		// landed: sit on the surface and let the leftover fall slide along it.
		this->mPos.vy = lineInfo.Position.vy - (this->field_EA8 << 12);
		this->mPos.vx += ((fall - (fall >> this->mFric.vx)) >> 12) * lineInfo.Normal.vx;
		this->mPos.vz += ((fall - (fall >> this->mFric.vz)) >> 12) * lineInfo.Normal.vz;
		fall = fall - (fall >> this->mFric.vy);
		this->mPos.vy += fall;
		this->mVel.vy = savedFallVel;
	}
	else
	{
		if (*gClipUpwardsFallOnGroundHit == 0 || fall < 0)
			this->mPos.vy += fall;

		this->mVel.vy = savedFallVel;
	}

	this->field_A8.vx = 0;
	this->field_A8.vy = -4096;
	this->field_A8.vz = 0;

	this->mShadowPos.vx = lineInfo.Position.vx;
	this->mShadowPos.vy = lineInfo.Position.vy;
	this->mShadowPos.vz = lineInfo.Position.vz;

	if ((lineInfo.pItem->mFlags & 0x100) != 0)
	{
		reinterpret_cast<CPlatform *>(lineInfo.pItem)->NotifyTrodUpon(this, &this->mPos, &this->field_A8);
		reinterpret_cast<CPlatform *>(lineInfo.pItem)->AdjustBruceHealth();
		this->field_DBC = reinterpret_cast<CBody *>(lineInfo.pItem);
	}

	if ((lineInfo.pFace[3] & 0x800000) != 0)
		this->AdjustBrightness(this->field_574);
	else
		this->AdjustBrightness(this->field_578);

	this->mAngles.Mask();
}
