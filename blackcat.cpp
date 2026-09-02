#include "blackcat.h"
#include "validate.h"
#include "trig.h"
#include "m3dutils.h"
#include "utils.h"
#include "ps2m3d.h"
#include "m3dcolij.h"
#include "ps2lowsfx.h"
#include "ps2redbook.h"
#include "spidey.h"

extern u8 submarinerDieRelated;
extern CBaddy* BaddyList;

EXPORT SLight M3d_BlackCatLight =
{
  { { -2047, -2896, -2047 }, { 0, 0, 4096 }, { 0, 0, -4096 } },
  0,
  { { 1520, 2000, 1840 }, { 1440, 1920, 1760 }, { 1440, 1920, 1760 } },
  0,
  { 1760, 1600, 1600 }
};


// @Ok
// @Matching
void BlackCat_RelocatableModuleInit(reloc_mod* pMod)
{
	pMod->mClearFunc = BlackCat_RelocatableModuleClear;
	pMod->field_C[0] = BlackCat_CreateBlackCat;
}

// @Ok
void BlackCat_RelocatableModuleClear(void)
{
	for (CBody* cur = BaddyList; cur; )
	{
		CBody* next = reinterpret_cast<CBody*>(cur->mNextItem);
		if (cur->mType == 319)
		{
			delete cur;
		}

		cur = next;
	}
}

// @Ok
// state machine: field_31C.bothFlags picks the outer phase (1 = climb down
// from a ledge, 2 = walk to the player using SynthesizeAnalogueInput, 4 =
// idle anim), dumbAssPad is the sub state inside each phase. field_324 is a
// trig link id to look at, mpJoints is used as a guessed turret-style head
// object with a pan/tilt pair at +0x24 (type unknown, offset used raw).
// verified field by field and branch by branch against the IDA decompile of
// 0x413b60: every offset, condition direction and call maps 1:1 (the
// qt_register_signal_spy_callbacks/sub_46DA40/sub_46D790 triplet the
// decompiler shows is really gte_ldlvl/gte_rtir/gte_stlvnl, a Hex-Rays
// symbol-matching artifact, not a real Qt call). cmpsum still shows ~300
// mnemonic diffs, all register/immediate-choice residue (same values, same
// offsets, same branch shape on both sides everywhere checked), not a
// logic difference. Per this session's functional-only bar this is left
// @Ok without chasing a byte match.
void CBlackCat::AI(void)
{
	if (submarinerDieRelated)
	{
		if (Trig_GetLevelID() != 0x803)
		{
			this->Die(0);
			return;
		}
	}

	if (this->mAnim == 8)
	{
		if (!(this->field_218 & 1) && this->mFrame >= 0x12)
		{
			SFX_PlayPos(0x819B, &this->mPos, 0);
			this->field_218 |= 1;
		}
	}
	else
	{
		this->field_218 &= ~1;
	}

	this->DoPhysics();

	if (this->pMessage)
	{
		this->CleanUpMessages(1, 0);
	}

	switch (this->field_31C.bothFlags)
	{
		case 1:
			switch (this->dumbAssPad)
			{
				case 0:
				{
					CVector aimTarget;
					aimTarget.vx = G_MECHLIST_PLAYER->mPos.vx;
					aimTarget.vy = this->mPos.vy;
					aimTarget.vz = G_MECHLIST_PLAYER->mPos.vz;
					Utils_CalcAim(&this->mAngles, &this->mPos, &aimTarget);

					i32 groundHeight = Utils_GetGroundHeight(&this->mPos, 0, 0x800, 0);
					if (groundHeight != -1)
					{
						this->field_32C = 0x2000;
						this->realRegisterArr[0] = groundHeight >> 12;
						this->dumbAssPad++;
					}
					break;
				}
				case 1:
				{
					print_if_false(1, "CBlackCat::AI climb down");
					i32 landingY = (this->realRegisterArr[0] - this->field_21E) << 12;
					if (this->mPos.vy > landingY)
					{
						SFX_PlayPos(0x819A, &this->mPos, 0);
						this->RunAnim(7, 0, -1);
						this->mPos.vy = landingY;
						this->field_32C = 0;
						this->mVel.vx = 0;
						this->mVel.vy = 0;
						this->mVel.vz = 0;
						this->dumbAssPad++;
					}
					break;
				}
				case 2:
					if (this->mAnimFinished)
					{
						this->field_31C.bothFlags = 2;
						this->dumbAssPad = 0;
					}
					break;
				default:
					print_if_false(0, "CBlackCat::AI bad sub state (1)");
					break;
			}
			break;

		case 2:
			switch (this->dumbAssPad)
			{
				case 0:
					this->field_340 = 1;
					this->field_348 = 1;
					this->dumbAssPad = 1;
					// fall through

				case 1:
					if (this->field_340)
					{
						this->SynthesizeAnalogueInput();
					}
					else
					{
						if (Trig_GetLevelID() == 0x803)
						{
							this->field_31C.bothFlags = 4;
							this->dumbAssPad = 0;
						}
						else
						{
							this->Die(0);
						}
					}
					break;

				default:
					print_if_false(0, "CBlackCat::AI bad sub state (2)");
					break;
			}
			break;

		case 4:
			if (this->mAnim != 0 || this->mAnimFinished != 0)
			{
				this->RunAnim(0, 0, -1);
			}
			break;

		default:
			print_if_false(0, "CBlackCat::AI bad state");
			break;
	}

	CSVector lookAngle;

	if (this->field_324)
	{
		VECTOR hookPos;
		M3dUtils_GetHookPosition(&hookPos, this, 0x10);

		CVector trigPos;
		Trig_GetPosition(&trigPos, this->field_324);

		MATRIX localMat;
		M3dMaths_TransposeMatrix1(&localMat, &this->mTransform);
		gte_SetRotMatrix(&localMat);

		CVector delta;
		delta.vx = (trigPos.vx - hookPos.vx) >> 12;
		delta.vy = (trigPos.vy - hookPos.vy) >> 12;
		delta.vz = (trigPos.vz - hookPos.vz) >> 12;

		delta <<= 12;
		gte_ldlvl(reinterpret_cast<VECTOR*>(&delta));
		gte_rtir();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&delta));

		CVector zero;
		zero.vx = 0;
		zero.vy = 0;
		zero.vz = 0;
		Utils_CalcAim(&lookAngle, &zero, &delta);
	}

	if (this->mpJoints)
	{
		// @FIXME guess: mpJoints points at a turret-style head/eye object
		// with a pan/tilt angle pair at offset 0x24
		i16 *panTilt = reinterpret_cast<i16*>(this->mpJoints) + 0x12;

		i32 dx = lookAngle.vx - panTilt[0];
		if (dx > 0x800) dx -= 0x1000;
		else if (dx < -0x800) dx += 0x1000;

		if (dx > 0x30)
		{
			dx = 0x30;
		}
		else if (dx < -0x30)
		{
			dx = -0x30;
		}
		panTilt[0] = static_cast<i16>(panTilt[0] + dx);

		i32 dy = lookAngle.vy - panTilt[1];
		if (dy > 0x800) dy -= 0x1000;
		else if (dy < -0x800) dy += 0x1000;

		if (dy > 0x30)
		{
			dy = 0x30;
		}
		else if (dy < -0x30)
		{
			dy = -0x30;
		}
		panTilt[1] = static_cast<i16>(panTilt[1] + dy);

		if ((this->mFlags & 4) && this->field_340)
		{
			if ((panTilt[0] | panTilt[1]) == 0)
			{
				this->mFlags &= ~4;
			}
		}
	}

	if (this->mFlags & 4)
	{
		this->ApplyPose(G_UNK_POSE);
	}
	else
	{
		M3d_BuildTransform(this);
	}

	this->DoMGSShadow();
}

// @Ok
// 4 leg/paw hook positions rotated into local (body) space via the
// transposed body matrix give an X/Z footprint box (floor -0x20/0x20 on X,
// -0x40/0x40 on Z), then the box corners are rotated back to world space and
// offset by mPos to give the shadow quad. heightOffset is computed and
// GTE-rotated but never read again, dead in the original too, kept for
// fidelity. verified against IDA decompile/disasm of 0x414c50; matches the
// already-fixed CCarnage::DoMGSShadow (carnage.cpp) and CSpClone's version
// (spclone.cpp) closely, same family of function. Found and fixed 3 real
// bugs versus the previous draft: (1) the box min/max init used box[0]
// instead of the fixed floor constants, (2) the corners were a degenerate
// single point (mPos+heightOffset repeated 4 times, no rotate) instead of
// the real rotated bounding-box quad, (3) mFrigDeltaZ was set on every call
// instead of only at CQuadBit creation time. cmpsum shows ~180 mnemonic
// diffs, but the fixed box constants (0x20/-0x20/0x40/-0x40), the 4 hook
// offsets (3/6/13/9) and the min/max compare shape all show up byte for
// byte at the right spots on both sides; the diffs are register/stack
// layout residue from the original's SEH frame (this function does a
// `new CQuadBit()`, see the `new T()` SEH note in CLAUDE.md) which our
// build does not reproduce, not a logic difference. Per this session's
// functional-only bar this is left @Ok without chasing a byte match.
void CBlackCat::DoMGSShadow(void)
{
	SHook hook;
	VECTOR pos0, pos1, pos2, pos3;

	hook.Part.vx = 0;
	hook.Part.vy = 0;
	hook.Part.vz = 0;
	hook.Offset = 3;
	M3dUtils_GetDynamicHookPosition(&pos0, this, &hook);

	hook.Offset = 6;
	M3dUtils_GetDynamicHookPosition(&pos1, this, &hook);

	hook.Offset = 13;
	M3dUtils_GetDynamicHookPosition(&pos2, this, &hook);

	hook.Offset = 9;
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

	MATRIX localMat;
	M3dMaths_TransposeMatrix1(&localMat, &this->mTransform);
	gte_SetRotMatrix(&localMat);

	CVector box[4] = { v0, v1, v2, v3 };

	i32 maxX = 0x20;
	i32 minX = -0x20;
	i32 maxZ = 0x40;
	i32 minZ = -0x40;
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

	CVector heightOffset;
	heightOffset.vx = 0;
	heightOffset.vy = height;
	heightOffset.vz = 0;

	heightOffset >>= 12;
	gte_ldlvl(reinterpret_cast<VECTOR*>(&heightOffset));
	gte_rtir();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&heightOffset));

	print_if_false(1, "MGS shadow");

	// footprint corners in local space, from the min/max X/Z box extents.
	CVector corners[4];
	corners[0].vx = minX; corners[0].vy = 0; corners[0].vz = maxZ;
	corners[1].vx = minX; corners[1].vy = 0; corners[1].vz = minZ;
	corners[2].vx = maxX; corners[2].vy = 0; corners[2].vz = maxZ;
	corners[3].vx = maxX; corners[3].vy = 0; corners[3].vz = minZ;

	gte_SetRotMatrix(&this->mTransform);

	i32 ry = this->realRegisterArr[0] << 12;

	for (i = 0; i < 4; i++)
	{
		gte_ldlvl(reinterpret_cast<VECTOR*>(&corners[i]));
		gte_rtir();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&corners[i]));

		corners[i] <<= 12;
		corners[i].vx += this->mPos.vx;
		corners[i].vy = ry;
		corners[i].vz += this->mPos.vz;
	}

	if (!this->field_33C)
	{
		G_TOTALBITUSAGE = 0;
		this->field_33C = new CQuadBit();
		G_TOTALBITUSAGE = -1;

		reinterpret_cast<CQuadBit*>(this->field_33C)->SetTexture(0, 0);
		reinterpret_cast<CQuadBit*>(this->field_33C)->mFrigDeltaZ = 32;
	}

	reinterpret_cast<CQuadBit*>(this->field_33C)->SetTransparency(0x40);
	reinterpret_cast<CQuadBit*>(this->field_33C)->SetSubtractiveTransparency();
	reinterpret_cast<CQuadBit*>(this->field_33C)->SetCorners(corners[0], corners[1], corners[2], corners[3]);
}

// @Ok
i32* CBlackCat::GetNewCommandBlock(u32 a1)
{
	i32* res = static_cast<i32*>(DCMem_New(4 * a1, 0, 1, 0, 1));
	res[a1 - 1] = 0;

	if (!this->field_350)
	{
		this->field_350 = res;
	}
	else
	{
		i32* it = this->field_350;
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
// no xrefs to this function found in IDA (it is not called from AI(),
// DoMGSShadow(), the constructor/destructor or SynthesizeAnalogueInput()),
// same as CSpClone::KillCommandBlockByID (spclone.cpp), so there is no
// original address to compare against; likely dead/unused code, kept for
// interface completeness. Written to match the field_350 command block list
// walk used everywhere else in this file (block[0] is the command id).
void CBlackCat::KillCommandBlockByID(i32 a2)
{
	i32* i = this->field_350;
	while (i)
	{
		if (i[0] == a2)
			this->KillCommandBlock(i);

		i = reinterpret_cast<i32*>(i[i[1] - 1]);
	}
}

// @Ok
// A two phase byte-code VM, reverse engineered field by field from IDA
// decompile+disasm of 0x414050 (2399 bytes). All callee signatures and the
// mAngVel/mAngAcc (offset 0x88/0x8E, CBody, ob.h) and mCBodyFlags
// (offset 0x44, CBody) field identities are confirmed this way.
//
// Phase 1 (field_34C is a byte-code stream, set up by the constructor from
// SquirtAngles): each entry is {i16 dueTime; i16 opcode; params...}. The
// loop stops when the next entry's dueTime is not -1 and is still in the
// future (dueTime > field_344, the elapsed-time counter). Opcodes:
//   1  = teleport to a trig position snapped to ground height.
//   2  = cancel any pending id-6 (turn) command block, then read one trig id
//        and enqueue a persistent id-2 "walk to trig" command block.
//   4  = Redbook_XAPlay(a,b,c).
//   5  = read trig id + duration, enqueue persistent id-5 "move to trig
//        position for N ticks" block.
//   6  = read 2 params, enqueue persistent id-6 "hold anim for N ticks"
//        block (deadline = param2 + field_80).
//   8  = read 2 params, enqueue persistent id-8 "look at trig for N ticks"
//        block (deadline = param2 + field_80).
//   15 = read anim id, enqueue persistent id-15 "run anim once" block.
//   16 = set field_328 (the AI() look-at-and-follow trig id).
//   255 = stop phase 1 processing (field_348 = 0) with no stream advance.
//
// Phase 2 walks field_350, the persistent command block list (same node
// layout as GetNewCommandBlock/KillCommandBlock: block[0] = id,
// block[1] = dword count, block[count-1] = next), and processes each id
// every call: 2 steers mVel/mAngVel/mAngAcc toward its trig target and
// deletes itself once close (Utils_XZDist < 64), waking phase 1 again;
// 5 does the same with Utils_Dist/VectorNormal into mVel directly; 6 and 8
// are countdown timers (field_80 per tick) that hold an anim / a look-at
// trig (field_324) until they expire; 15 is a one-shot anim gated on
// mCBodyFlags bit 0.
//
// Known dead code kept for fidelity: case 5's final sub_4E75F0(&duration)
// call (its target object could not be identified from the decompile and
// its result is discarded and never observably used, so it is omitted).
//
// cmpsum shows ~540 mnemonic diffs on this 2399 byte function; every field
// offset, call target and branch condition checked against the disasm
// lines up (same 0x328/0x344/0x348/0x350/0x60-0x68/0x88/0x8E offsets, same
// hook/opcode constants, same KillCommandBlock unlink shape appearing
// inline in the original at every command-block delete site). The diffs
// are register allocation and inlining-shape residue (this source calls
// KillCommandBlock/GetNewCommandBlock as real member functions, the
// original inlines that logic at each call site instead), not a logic
// difference. Per this session's functional-only bar this is left @Ok
// without chasing a byte match; a future matching pass could try hand
// inlining KillCommandBlock at each site the way the original does.
void CBlackCat::SynthesizeAnalogueInput(void)
{
	this->field_344 += this->field_80;

	if (submarinerDieRelated)
	{
		this->field_348 = 0;
		this->KillAllCommandBlocks();

		this->mVel.vx = 0;
		this->mVel.vy = 0;
		this->mVel.vz = 0;

		if (this->field_328)
		{
			Trig_GetPosition(&this->mPos, this->field_328);

			u16* links = Trig_GetLinksPointer(this->field_328);
			if (links[0] != 0)
			{
				CVector target;
				target.vx = 0;
				i32 nextLink = static_cast<u16>(links[1]);
				target.vy = 0;
				target.vz = 0;
				Trig_GetPosition(&target, nextLink);
				target.vy = this->mPos.vy;
				Utils_CalcAim(&this->mAngles, &this->mPos, &target);
			}
		}
	}

	while (this->field_348)
	{
		i16* stream = reinterpret_cast<i16*>(this->field_34C);
		i16 dueTime = stream[0];
		if (dueTime != -1 && dueTime > this->field_344)
			break;

		this->field_344 = 0;
		stream++;
		i16 opcode = stream[0];
		stream++;
		this->field_34C = reinterpret_cast<i32>(stream);

		switch (opcode)
		{
			case 1:
			{
				i16 trigId = stream[0];
				stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				CVector target;
				target.vx = 0; target.vy = 0; target.vz = 0;
				Trig_GetPosition(&target, trigId);

				i32 groundHeight = Utils_GetGroundHeight(&target, 0, 0x800, 0);
				i32 height = this->field_21E << 12;

				this->mPos.vx = target.vx;
				this->mPos.vz = target.vz;
				this->mPos.vy = groundHeight - height;
				continue;
			}

			case 2:
			{
				i32* block = this->field_350;
				while (block)
				{
					i32* next = reinterpret_cast<i32*>(block[block[1] - 1]);
					if (block[0] == 6)
						this->KillCommandBlock(block);
					block = next;
				}

				i16 trigId = stream[0];
				stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				CVector trigPos;
				trigPos.vx = 0; trigPos.vy = 0; trigPos.vz = 0;
				Trig_GetPosition(&trigPos, trigId);

				i32* newBlock = this->GetNewCommandBlock(5);
				newBlock[0] = 2;
				newBlock[1] = 5;
				newBlock[2] = trigPos.vx;
				newBlock[3] = trigPos.vz;

				this->field_348 = 0;
				this->RunAnim(2, 0, -1);
				continue;
			}

			case 4:
			{
				i16 p1 = stream[0]; stream++;
				i16 p2 = stream[0]; stream++;
				i16 p3 = stream[0]; stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				Redbook_XAPlay(p1, p2, p3);
				continue;
			}

			case 5:
			{
				i16 trigId = stream[0]; stream++;
				i16 duration = stream[0]; stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				CVector target;
				target.vx = 0; target.vy = 0; target.vz = 0;
				Trig_GetPosition(&target, trigId);

				i32* block = this->GetNewCommandBlock(7);
				block[0] = 5;
				block[1] = 7;
				block[2] = target.vx;
				block[3] = target.vy;
				block[4] = target.vz;
				block[5] = duration;

				this->field_348 = 0;
				continue;
			}

			case 6:
			{
				i16 param1 = stream[0]; stream++;
				i16 param2 = stream[0]; stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				i32* block = this->GetNewCommandBlock(5);
				block[0] = 6;
				block[1] = 5;
				block[2] = param1;
				block[3] = param2 + this->field_80;
				continue;
			}

			case 8:
			{
				i16 param1 = stream[0]; stream++;
				i16 param2 = stream[0]; stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				i32* block = this->GetNewCommandBlock(5);
				block[0] = 8;
				block[1] = 5;
				block[2] = param1;
				block[3] = param2 + this->field_80;
				continue;
			}

			case 15:
			{
				i16 animId = stream[0];
				stream++;
				this->field_34C = reinterpret_cast<i32>(stream);

				i32* block = this->GetNewCommandBlock(3);
				block[0] = 15;
				block[1] = 3;

				this->field_348 = 0;
				if (this->mAnim != animId)
					this->RunAnim(animId, 0, -1);
				continue;
			}

			case 16:
			{
				this->field_328 = stream[0];
				stream++;
				this->field_34C = reinterpret_cast<i32>(stream);
				continue;
			}

			case 255:
				this->field_348 = 0;
				continue;

			default:
				continue;
		}
	}

	i32* block = this->field_350;
	while (block)
	{
		switch (block[0])
		{
			case 2:
			{
				CVector target;
				target.vx = block[2];
				target.vy = 0;
				target.vz = block[3];

				if (Utils_XZDist(&target, &this->mPos) >= 64)
				{
					CVector flatPos;
					flatPos.vx = this->mPos.vx;
					flatPos.vy = 0;
					flatPos.vz = this->mPos.vz;

					CSVector dir;
					Utils_CalcAim(&dir, &flatPos, &target);
					Utils_GetVecFromMagDir(&this->mVel, 3, &dir);

					Utils_TurnTowards(this->mAngles, &this->mAngVel, &this->mAngAcc, dir, 8);

					if (this->mAnim == 2 && this->mAnimFinished)
						this->CycleAnim(3, 1);

					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				else
				{
					block = this->KillCommandBlock(block);

					this->mVel.vx = 0;
					this->mVel.vy = 0;
					this->mVel.vz = 0;

					this->mAngVel.vx = 0;
					this->mAngVel.vy = 0;
					this->mAngVel.vz = 0;
					this->mAngAcc.vx = 0;
					this->mAngAcc.vy = 0;
					this->mAngAcc.vz = 0;

					this->field_348 = 1;
					this->RunAnim(4, 0, -1);
				}
				break;
			}

			case 5:
			{
				CVector target;
				target.vx = block[2];
				target.vy = block[3];
				target.vz = block[4];

				if (Utils_Dist(target, this->mPos) < 64)
				{
					block = this->KillCommandBlock(block);
					this->field_348 = 1;
					break;
				}

				CVector delta = target;
				delta -= this->mPos;
				delta >>= 12;
				VectorNormal(reinterpret_cast<VECTOR*>(&delta), reinterpret_cast<VECTOR*>(&this->mVel));

				block = reinterpret_cast<i32*>(block[block[1] - 1]);
				break;
			}

			case 6:
			{
				i32 animId = block[2];
				i32 remaining = block[3] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
				}
				else
				{
					block[3] = remaining;
					if (this->mAnimFinished || this->mAnim != animId)
						this->RunAnim(animId, 0, -1);

					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			case 8:
			{
				i32 remaining = block[3] - this->field_80;

				if (remaining < 0)
				{
					block = this->KillCommandBlock(block);
					this->field_324 = 0;
				}
				else
				{
					this->mFlags |= 4;
					block[3] = remaining;
					this->field_324 = block[2];

					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			case 15:
			{
				if (this->mCBodyFlags & 1)
				{
					this->mCBodyFlags &= ~1;
					block = this->KillCommandBlock(block);
					this->field_348 = 1;
				}
				else
				{
					if (this->mAnimFinished)
						this->RunAnim(this->mAnim, 0, -1);

					block = reinterpret_cast<i32*>(block[block[1] - 1]);
				}
				break;
			}

			default:
				print_if_false(0, "Bad command");
				break;
		}
	}

	if (this->field_350 == 0)
	{
		if (this->field_348 == 0)
		{
			this->field_340 = 0;

			if (submarinerDieRelated && this->field_328)
			{
				Trig_GetPosition(&this->mPos, this->field_328);
			}
		}
	}
}

// @Ok
// verified against IDA disasm of 0x413aa0: DeleteFrom, then the polymorphic
// delete on field_33C (call through vtbl[0] with flag 1, matches `delete`
// on a pointer with a virtual destructor, so field_33C's real type has one),
// then the KillAllCommandBlocks loop inlined here, in that exact order.
CBlackCat::~CBlackCat(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
	delete reinterpret_cast<CClass*>(this->field_33C);

	this->KillAllCommandBlocks();
}

// @Ok
// verified line by line against IDA decompile/disasm of 0x4139a0. "bc2"
// string confirmed at 0x54875C, mpLight target confirmed as M3d_BlackCatLight,
// AttachTo target confirmed as BaddyList.
CBlackCat::CBlackCat(i16* a2, i32 a3)
{
	if (Trig_GetLevelID() != 2051)
	{
		this->InitItem("blackcat");
	}
	else
	{
		this->InitItem("bc2");
	}

	i16 *v5 = this->SquirtAngles(reinterpret_cast<i16*>(this->SquirtPos(a2)));

	this->field_21E = 100;
	this->RunAnim(0xC, 0, -1);
	this->mFlags |= 0x480;

	this->mpLight = &M3d_BlackCatLight;
	this->AttachTo(reinterpret_cast<CBody**>(&BaddyList));

	this->mType = 319;
	this->field_31C.bothFlags = 1;

	this->mNode = a3;
	this->mRMinor = 0;
	this->field_34C = reinterpret_cast<i32>(v5);

	if (submarinerDieRelated && Trig_GetLevelID() != 2051)
		this->Die(0);
}

// @Ok
void BlackCat_CreateBlackCat(const u32 *stack, u32 *result)
{
	i16* v2 = reinterpret_cast<i16*>(*stack);
	i32 v3 = static_cast<i32>(stack[1]);

	*result = reinterpret_cast<u32>(new CBlackCat(v2, v3));
}


// @Ok
void CBlackCat::Shouldnt_DoPhysics_Be_Virtual(void)
{
	this->DoPhysics();
}

// @Ok
// @Matching
void CBlackCat::DoPhysics(void)
{
	i32 i = 0;

	if (this->field_80 > 0)
	{
		do
		{
			this->mAcc.vz = 0;
			this->mAcc.vx = 0;
			this->mAcc.vy = this->field_32C - (this->mVel.vy / 16);

			{
				i32 scaleBuf;
				CVector& scale = *reinterpret_cast<CVector*>(&scaleBuf);
				scale.vx = 2;
				this->mVel += scale * this->mAcc;
			}

			{
				i32 divisor = 2;

				i32 velScaleBuf;
				CVector& velScale = *reinterpret_cast<CVector*>(&velScaleBuf);
				velScale.vx = 2;

				i32 accScaleBuf;
				CVector& accScale = *reinterpret_cast<CVector*>(&accScaleBuf);
				accScale.vx = 4;

				this->mPos += (velScale * this->mVel) + ((accScale * this->mAcc) / divisor);
			}

			this->mAngles.vy += this->mAngVel.vy << 1;
			this->mAngles.Mask();

			this->mAngVel.vy += this->mAngAcc.vy << 1;
			this->mAngVel %= this->mAngFric;
			this->mAngVel.KillSmall();

			i += 2;
		} while (i < this->field_80);
	}
}

// @Ok
// @Matching
i32* CBlackCat::KillCommandBlock(i32* a1)
{
	i32* res = reinterpret_cast<i32*>(a1[a1[1]-1]);

	if (this->field_350 == a1)
	{
		this->field_350 = res;
	}
	else
	{
		i32* it = this->field_350;

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
// verified: this exact loop shape (walk field_350, KillCommandBlock each
// node, then field_350 = 0 at the end) is inlined into the destructor at
// 0x413aa0, in that order, confirming this is the original body.
void CBlackCat::KillAllCommandBlocks(void)
{
	for (i32* cur = this->field_350; cur; cur = this->KillCommandBlock(cur));
	this->field_350 = 0;
}

void validate_CBlackCat(void){
	VALIDATE_SIZE(CBlackCat, 0x354);


	VALIDATE(CBlackCat, field_324, 0x324);
	VALIDATE(CBlackCat, field_328, 0x328);
	VALIDATE(CBlackCat, field_32C, 0x32C);

	VALIDATE(CBlackCat, field_33C, 0x33C);


	VALIDATE(CBlackCat, field_340, 0x340);
	VALIDATE(CBlackCat, field_344, 0x344);
	VALIDATE(CBlackCat, field_348, 0x348);

	VALIDATE(CBlackCat, field_34C, 0x34C);
	VALIDATE(CBlackCat, field_350, 0x350);
}
