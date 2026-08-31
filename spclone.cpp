#include "spclone.h"
#include "validate.h"
#include "m3dutils.h"
#include "trig.h"
#include "utils.h"
#include "ps2redbook.h"
#include "m3dcolij.h"
#include "ps2m3d.h"

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

// @Ok
// Same overall shape as CBlackCat::AI (blackcat.cpp), reverse engineered
// from IDA decompile+disasm of 0x4AFF30 (1056 bytes). field_31C.bothFlags
// picks the outer phase (1 = fall/land on a ledge below, 2 = walk the
// SynthesizeAnalogueInput script), dumbAssPad is the sub state inside each
// phase. field_324 is a trig link id to look at (same role as CBlackCat's
// own field_324), field_33C is the "script driving me" outer flag
// SynthesizeAnalogueInput clears when it runs out of work.
//
// The look-at-trig block (M3dUtils_GetDynamicHookPosition with
// SHook{Part=0,Offset=7} + M3dMaths_TransposeMatrix1 + gte_ldlvl/rtir/stlvnl
// + CVector::operator<<=) was verified instruction by instruction against
// the raw disasm, not just the Hex-Rays pseudocode: Hex-Rays mislabels the
// gte_ldlvl/gte_rtir/gte_stlvnl triplet as
// qt_register_signal_spy_callbacks/sub_46DA40/sub_46D790 (same decompiler
// symbol-matching artifact already documented in CBlackCat::AI's comment),
// and mislabels CVector::operator<<=(const int&) (0x4E7680) as a plain
// function taking &local instead of a thiscall on &delta; the disasm shows
// ecx = &delta is still live at the call, matching the CVector member call.
//
// Known original defect kept for fidelity (per CLAUDE.md: reproduce the
// source-level bug, don't fix it): the pan/tilt block that steers
// mpJoints+0x24/+0x26 runs unconditionally even when mpJoints is null (the
// disasm only guards the ApplyPose(gUnkPose) call with the mpJoints==0
// check, not the pan/tilt math after it), unlike CBlackCat::AI which wraps
// the whole block in "if (this->mpJoints)".
//
// Functional-only per this session's bar: field offsets, opcode/constant
// values (RunAnim ids, magnitude 8, CycleAnim(21,1), landing anim 203) and
// call targets all checked against the disasm; not expected to byte-match.
void CSpClone::AI(void)
{
	if (this->pMessage)
	{
		this->CleanUpMessages(1, 0);
	}

	if (submarinerDieRelated)
	{
		this->Die(0);
		return;
	}

	this->field_334 = 1365 * (this->field_80 + this->field_330 + this->field_32C);
	this->DoPhysics();

	switch (this->field_31C.bothFlags)
	{
		case 1:
			switch (this->dumbAssPad)
			{
				case 0:
				{
					CVector aimTarget;
					aimTarget.vx = G_MECHLIST->mPos.vx;
					aimTarget.vy = this->mPos.vy;
					aimTarget.vz = G_MECHLIST->mPos.vz;
					Utils_CalcAim(&this->mAngles, &this->mPos, &aimTarget);

					i32 groundHeight = Utils_GetGroundHeight(&this->mPos, 0, 0x800, 0);
					if (groundHeight != -1)
					{
						this->field_328 = 0x2000;
						print_if_false(1, "Bad register index");
						this->realRegisterArr[0] = groundHeight >> 12;
						this->dumbAssPad++;
					}
					break;
				}

				case 1:
				{
					print_if_false(1, "Bad register index");
					i32 landingY = (this->realRegisterArr[0] - this->field_21E) << 12;
					if (this->mPos.vy > landingY)
					{
						this->RunAnim(203, 0, -1);
						this->mPos.vy = landingY;
						this->field_328 = 0;
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
					print_if_false(0, "Unknown substate");
					break;
			}
			break;

		case 2:
			switch (this->dumbAssPad)
			{
				case 0:
					this->field_33C = 1;
					this->field_344 = 1;
					this->dumbAssPad = 1;
					// fall through

				case 1:
					if (this->field_33C)
					{
						this->SynthesizeAnalogueInput();
					}
					else
					{
						this->Die(0);
					}
					break;

				default:
					print_if_false(0, "Unknown substate");
					break;
			}
			break;

		default:
			print_if_false(0, "Unknown state");
			break;
	}

	this->field_330 = this->field_32C;
	this->field_32C = this->field_80;

	CSVector lookAngle;
	lookAngle.vx = 0;
	lookAngle.vy = 0;
	lookAngle.vz = 0;

	if (this->field_324)
	{
		SHook hook;
		hook.Part.vx = 0;
		hook.Part.vy = 0;
		hook.Part.vz = 0;
		hook.Offset = 7;
		VECTOR hookPos;
		M3dUtils_GetDynamicHookPosition(&hookPos, this, &hook);

		CVector trigPos;
		trigPos.vx = 0;
		trigPos.vy = 0;
		trigPos.vz = 0;
		Trig_GetPosition(&trigPos, this->field_324);

		MATRIX localMat;
		M3dMaths_TransposeMatrix1(&localMat, &this->mTransform);
		gte_SetRotMatrix(&localMat);

		CVector delta;
		delta.vx = (trigPos.vx - hookPos.vx) >> 12;
		delta.vy = (trigPos.vy - hookPos.vy) >> 12;
		delta.vz = (trigPos.vz - hookPos.vz) >> 12;

		gte_ldlvl(reinterpret_cast<VECTOR*>(&delta));
		gte_rtir();
		gte_stlvnl(reinterpret_cast<VECTOR*>(&delta));

		delta <<= 12;

		CVector zero;
		zero.vx = 0;
		zero.vy = 0;
		zero.vz = 0;
		Utils_CalcAim(&lookAngle, &zero, &delta);
	}

	if (!this->mpJoints)
	{
		this->ApplyPose(gUnkPose);
	}

	i16* panTilt = reinterpret_cast<i16*>(this->mpJoints) + 0x12;

	i32 dx = lookAngle.vx - panTilt[0];
	if (dx > 0x800) dx -= 0x1000;
	else if (dx < -0x800) dx += 0x1000;

	if (dx != 0)
	{
		if (dx > 0x30) dx = 0x30;
		else if (dx < -0x30) dx = -0x30;
		panTilt[0] = static_cast<i16>(panTilt[0] + dx);
	}

	i32 dy = lookAngle.vy - panTilt[1];
	if (dy > 0x800) dy -= 0x1000;
	else if (dy < -0x800) dy += 0x1000;

	if (dy != 0)
	{
		if (dy > 0x30) dy = 0x30;
		else if (dy < -0x30) dy = -0x30;
		panTilt[1] = static_cast<i16>(panTilt[1] + dy);
	}

	if ((this->mFlags & 4) && this->field_33C && (panTilt[0] | panTilt[1]) == 0)
	{
		this->mFlags &= ~4;
	}

	if (this->mFlags & 4)
	{
		this->ApplyPose(gUnkPose);
	}
	else
	{
		M3d_BuildTransform(this);
	}

	this->DoMGSShadow();
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

		this->field_338->SetTexture(0, 0);
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

// @Ok
// A byte-code VM, same idiom as CBlackCat::SynthesizeAnalogueInput
// (blackcat.cpp), reverse engineered from IDA decompile+disasm of 0x4B03C0
// (2464 bytes). field_348 is the byte-code stream (i16 dueTime/opcode/params
// entries, same shape as CBlackCat's field_34C), field_340 is the elapsed
// time counter (field_80 added per tick), field_344 is the phase-1-active
// byte flag. Opcodes:
//   1 = read a trig id, teleport to it (mPos.vx/vz from the trig, mPos.vy
//       from ground height minus field_21E).
//   2 = cancel any pending id-6 command block, read a trig id, enqueue a
//       persistent id-2 "walk to trig" block, RunAnim(1).
//   4 = Redbook_XAPlay(a,b,c).
//   5 = read trig id + duration, enqueue persistent id-5 "move to trig for
//       N ticks" block.
//   6 = read 2 params, enqueue persistent id-6 "hold anim for N ticks"
//       block (deadline = param2 + field_80).
//   8 = read 2 params, enqueue persistent id-8 "look at trig for N ticks"
//       block (deadline = param2 + field_80).
//   15 = read anim id, enqueue persistent id-15 "run anim once" block.
//   255 = stop phase 1 processing (field_344 = 0) with no stream advance.
// Phase 2 walks field_34C (same node layout as GetNewCommandBlock /
// KillCommandBlock): 2 steers mVel/mAngVel/mAngAcc toward its trig target
// (magnitude 8) and deletes itself once close (Utils_XZDist < 64), waking
// phase 1 again and calling RunAnim(13); 5 does the same with
// Utils_Dist/VectorNormal into mVel directly; 6 and 8 are countdown timers
// (field_80 per tick) that hold an anim / a look-at trig (field_324) until
// they expire; 15 is a one-shot anim gated on mInputFlags bit 0.
//
// Known dead code kept for fidelity: case 5's final CVector::operator<<=
// call on the duration (its result is discarded and never observably used,
// same dead call already documented in CBlackCat::SynthesizeAnalogueInput),
// omitted here too.
//
// Functional-only per this session's bar (session override on the matching
// discipline): every field offset, opcode, and call target was checked
// against the disasm; GetNewCommandBlock/KillCommandBlock are called as
// real member functions here instead of the inlined-at-each-site shape the
// original compiled (same class of residue already documented for
// CBlackCat's version), so this is not expected to byte-match.
void CSpClone::SynthesizeAnalogueInput(void)
{
	this->field_340 += this->field_80;

	if (this->field_344)
	{
		do
		{
			i16* stream = reinterpret_cast<i16*>(this->field_348);
			i16 dueTime = stream[0];
			if (dueTime != -1 && dueTime > this->field_340)
				break;

			this->field_340 = 0;
			stream++;
			i16 opcode = stream[0];
			stream++;
			this->field_348 = reinterpret_cast<i32>(stream);

			switch (opcode)
			{
				case 1:
				{
					i16 trigId = stream[0];
					stream++;
					this->field_348 = reinterpret_cast<i32>(stream);

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
					i32* block = this->field_34C;
					while (block)
					{
						i32* next = reinterpret_cast<i32*>(block[block[1] - 1]);
						if (block[0] == 6)
							this->KillCommandBlock(block);
						block = next;
					}

					i16 trigId = stream[0];
					stream++;
					this->field_348 = reinterpret_cast<i32>(stream);

					CVector trigPos;
					trigPos.vx = 0; trigPos.vy = 0; trigPos.vz = 0;
					Trig_GetPosition(&trigPos, trigId);

					i32* newBlock = this->GetNewCommandBlock(5);
					newBlock[0] = 2;
					newBlock[1] = 5;
					newBlock[2] = trigPos.vx;
					newBlock[3] = trigPos.vz;

					this->field_344 = 0;
					this->RunAnim(1, 0, -1);
					continue;
				}

				case 4:
				{
					i16 p1 = stream[0]; stream++;
					i16 p2 = stream[0]; stream++;
					i16 p3 = stream[0]; stream++;
					this->field_348 = reinterpret_cast<i32>(stream);

					Redbook_XAPlay(p1, p2, p3);
					continue;
				}

				case 5:
				{
					i16 trigId = stream[0]; stream++;
					i16 duration = stream[0]; stream++;
					this->field_348 = reinterpret_cast<i32>(stream);

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

					this->field_344 = 0;
					continue;
				}

				case 6:
				{
					i16 param1 = stream[0]; stream++;
					i16 param2 = stream[0]; stream++;
					this->field_348 = reinterpret_cast<i32>(stream);

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
					this->field_348 = reinterpret_cast<i32>(stream);

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
					this->field_348 = reinterpret_cast<i32>(stream);

					i32* block = this->GetNewCommandBlock(3);
					block[0] = 15;
					block[1] = 3;

					this->field_344 = 0;
					if (this->mAnim != animId)
						this->RunAnim(animId, 0, -1);
					continue;
				}

				case 255:
					this->field_344 = 0;
					continue;

				default:
					continue;
			}
		}
		while (this->field_344);
	}

	i32* block = this->field_34C;
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
					Utils_GetVecFromMagDir(&this->mVel, 8, &dir);

					Utils_TurnTowards(this->mAngles, &this->mAngVel, &this->mAngAcc, dir, 8);

					if (this->mAnim == 1 && this->mAnimFinished)
						this->CycleAnim(21, 1);

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

					this->field_344 = 1;
					this->RunAnim(13, 0, -1);
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
					this->field_344 = 1;
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
				if (this->mInputFlags & 1)
				{
					this->mInputFlags &= ~1;
					block = this->KillCommandBlock(block);
					this->field_344 = 1;
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

	if (!this->field_34C && !this->field_344)
	{
		this->field_33C = 0;
	}
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

	VALIDATE(CSpClone, field_324, 0x324);

	VALIDATE(CSpClone, field_328, 0x328);
	VALIDATE(CSpClone, field_32C, 0x32C);
	VALIDATE(CSpClone, field_330, 0x330);
	VALIDATE(CSpClone, field_334, 0x334);
	VALIDATE(CSpClone, field_338, 0x338);

	VALIDATE(CSpClone, field_33C, 0x33C);

	VALIDATE(CSpClone, field_340, 0x340);
	VALIDATE(CSpClone, field_344, 0x344);

	VALIDATE(CSpClone, field_348, 0x348);

	VALIDATE(CSpClone, field_34C, 0x34C);
}
