#include "switch.h"
#include "trig.h"
#include "baddy.h"
#include "spidey.h"
#include "spool.h"
#include "validate.h"

extern CBody* ControlBaddyList;
extern CPlayer* MechList;
extern i32 NumNodes;

// @NotOk
// no address in tools/names.json for this function (the Mac build has
// Switch_SetSwitchFaceFlags__FP5CItem, the PC build has no separate
// symbol for it), so it cannot be verified with compare.py/cmpsum.sh on
// its own. It only has one real caller, CSwitch::CSwitch, which inlines
// it twice (once for field_104, once for field_108); this is a faithful
// translation of that inlined block (see CSwitch::CSwitch below), walking
// the CItemRelatedList table (ob.h, 0x6B2454), the same table
// Spidey_SwapSuitTextures uses (spidey.cpp). No struct is declared for the
// per-region record, the offsets are guesses from the disassembly only.
void INLINE Switch_SetSwitchFaceFlags(CItem *pItem)
{
	i32 **pRegionEntry = CItemRelatedList[pItem->mRegion * 17];
	u8 *pRec = reinterpret_cast<u8*>(pRegionEntry[pItem->mModel]);

	i32 count = *reinterpret_cast<u16*>(pRec + 6);
	i32 offset = *reinterpret_cast<u16*>(pRec + 4) + *reinterpret_cast<u16*>(pRec + 2);

	u8 *pFace = pRec + offset * 8 + 0x1C;

	while (count != 0)
	{
		pFace[0xF] |= 2;
		pFace += (*reinterpret_cast<u16*>(pFace + 2) >> 1) * 2;
		count--;
	}
}

// @NotOk
// residue: 127 mnemonic diffs (down from 150 on the first honest pass),
// instruction count is 203 built vs 204 original (one instruction short),
// so this is a real missing instruction, not pure scheduling noise. The
// gap starts right where the two Spool_FindEnviroItem checksum reads (for
// field_104 and field_108) happen: the original re-reads this->field_104
// from memory (a real "mov edi,[esi+104h]") right after the second call,
// before storing field_108, and keeps that pointer in the register for
// the rest of the block. Our build stores field_108 immediately and
// reloads field_104 later, and folds the second checksum pointer's
// increment into a later "[edi+4]" addressing mode instead of emitting it
// as a separate add. Everything after that point is the same fallout
// (same call shapes, same field stores, same switch dispatch, just
// shifted by the one instruction). 11 source variants tried, targeting
// this specific spot: postfix vs explicit pre/post increment on the
// checksum pointer, a temp CItem* for field_108, symmetric temps for both
// fields, splitting read/increment/call into separate statements,
// pChecksum[0]/[1] indexing with a single combined advance, caching
// this->field_104 in a local read right after the second call, and three
// independent pointer variables instead of one mutated pointer. None
// reproduced the exact instruction original chose. Rest of the function
// (SquirtPos/SquirtAngles chain, the Switch_SetSwitchFaceFlags-equivalent
// walk inlined twice, the state switch, field_FC read) all line up
// structurally with the original once this residue is looked past.
CSwitch::CSwitch(i16 *a2, i32 a3)
{
	this->mCBodyFlags &= ~0x10;

	this->mType = 407;
	this->mRMinor = 0;
	this->mNode = static_cast<u16>(a3);

	this->AttachTo(&ControlBaddyList);

	i16 *pCursor = this->SquirtAngles(this->SquirtPos(a2));

	u32 *pChecksum = reinterpret_cast<u32*>((reinterpret_cast<i32>(pCursor) + 3) & ~3);

	u32 checksum1 = *pChecksum;
	pChecksum++;
	this->field_104 = Spool_FindEnviroItem(checksum1);

	u32 checksum2 = *pChecksum;
	pChecksum++;
	CItem *pItem108 = Spool_FindEnviroItem(checksum2);

	pCursor = reinterpret_cast<i16*>(pChecksum);

	this->field_108 = pItem108;

	if (this->field_104)
	{
		print_if_false(this->field_104 != 0, "Bad item");

		this->field_10C = this->field_104->mPos;

		Switch_SetSwitchFaceFlags(this->field_104);
	}

	if (this->field_108)
	{
		print_if_false(this->field_108 != 0, "Bad item");

		this->field_118 = this->field_108->mPos;

		Switch_SetSwitchFaceFlags(this->field_108);
	}

	i32 state = *pCursor;

	switch (state)
	{
		case 0:
			this->SwitchInactive();
			break;
		case 1:
			this->SwitchOff();
			break;
		case 2:
			this->SwitchOn();
			break;
		case 3:
		case 4:
			this->field_100 = state;
			this->field_124 = (state == 4);

			this->field_108->mFlags |= 0x400;
			this->field_108->mRGB = 0;

			Switch_SetVisible(true, this->field_108);
			break;
		default:
			print_if_false(0, "Bad switch state");
			break;
	}

	this->field_FC = *reinterpret_cast<u16*>(pCursor + 1);
}

// @Ok
// @Test
void CSwitch::AI(void)
{
	if (this->mInputFlags & 1)
	{
		this->mInputFlags &= ~1;
		switch (this->field_100)
		{
			case 0:
				this->SwitchOff();
				break;
			case 1:
				this->SwitchOn();
				this->SignalAttachedItems();
				break;
			case 2:
				this->SwitchInactive();
				break;
			default:
				print_if_false(0, "invalid switch state");
				break;
		}
	}

	if (this->field_100 == 2 || this->field_100 == 3 || this->field_100 == 4)
	{
		if (this->field_FC)
		{
			this->field_F8 += this->field_80;

			if (this->field_F8 > this->field_FC)
			{
				this->field_F8 = 0;

				if (this->field_100 == 2)
				{
					this->SwitchOff();
				}
				else
				{
					this->PulseLFA1Node(this->field_124 == 0 ? 3 : 2);
					this->field_100 = 5;
				}
			}
		}
	}

	if (this->field_100 == 3 || this->field_100 == 4)
	{
		this->field_108->mFlags &= 0xFFFE;

		u32 res;
		u32 Int = this->field_108->mRGB & 0xFF;
		if (Int < 0x80)
		{
			Int += this->field_80;

			res = (((Int << 8 ) | Int) << 8) | Int;
		}
		else
		{
			res = 0x808080;
		}

		this->mRGB = res;
	}

	if (this->field_100 == 5)
	{
		u32 v18 = 4 * this->field_80;
		u32 v19 = this->field_108->mRGB & 0xFF;

		if (v19 > v18)
		{
			u32 diff = v19 - v18;
			this->field_108->mRGB = (((diff << 8) | diff) << 8) | diff;
		}
		else
		{
			this->field_108->mFlags |= 0x21;
			reinterpret_cast<CBody*>(this->field_108)->Die();
			this->field_108->mRGB = 0;
		}
	}
}

// @Ok
void CSwitch::PulseLFA1Node(i32 a1)
{
	CVector v3;
	i32 nodeIndex = 1;
	v3.vx = 0;
	v3.vy = 0;
	v3.vz = 0;

	if (NumNodes > 1)
	{
		for (; nodeIndex < NumNodes; nodeIndex++)
		{
			if (*G_OFFSETLIST[nodeIndex] != 1)
				continue;
			Trig_GetPosition(&v3, nodeIndex);
			switch (a1)
			{
				case 0:
					if ( (v3.vx & 0xFFFFF000) != 409600 || v3.vy || v3.vz)
						break;
					Trig_SendPulseToNode(nodeIndex);
					return;
				case 1:
					if (v3.vx || v3.vy || v3.vz)
						break;
					Trig_SendPulseToNode(nodeIndex);
					return;
				case 2:
					if ( (v3.vx & 0xFFFFF000) != 409600 || v3.vy || (v3.vz & 0xFFFFF000) != 409600)
						break;
					Trig_SendPulseToNode(nodeIndex);
					return;
				case 3:
					if ( v3.vx || v3.vy || (v3.vz & 0xFFFFF000) != 409600)
						break;
					Trig_SendPulseToNode(nodeIndex);
					return;
			}
		}
	}

	print_if_false(0, "Node not found?");
}

// @Ok
void CSwitch::Flick(void)
{
	switch (this->field_100)
	{
		case 1:
			this->SwitchOn();
			this->SignalAttachedItems();
			break;
		case 2:
			this->SwitchOff();
			if (!this->field_FC)
				this->SignalAttachedItems();
			break;
		case 3:
		case 4:
			if (MechList)
				MechList->field_568 += this->field_124 ? -1 : 1;
			this->PulseLFA1Node(this->field_124 == 0);
			this->field_100 = 5;
			break;
		case 5:
			if (MechList)
				MechList->field_568 += this->field_124 ? -1 : 1;
			break;
		default:
			return;
	}
}

// @Ok
CSwitch* Switch_GetCSwitchObjectFromItem(CItem *pItem)
{
	print_if_false(pItem != 0, "Bad item");

	for (CItem *cur = ControlBaddyList; cur; cur = reinterpret_cast<CItem*>(cur->mNextItem))
	{
		if (cur->mType == 407)
		{
			CSwitch* pSwitch = reinterpret_cast<CSwitch*>(cur);

			if (pSwitch->field_104 == pItem || pSwitch->field_108 == pItem)
				return pSwitch;
		}
	}

	return 0;
}

// @Ok
CSwitch::~CSwitch(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&ControlBaddyList));
}

// @Ok
INLINE void CSwitch::SwitchOn(void)
{
	this->field_F8 = 0;
	this->field_100 = 2;

	Switch_SetVisible(1, this->field_108);
	Switch_SetVisible(0, this->field_104);
}

// @Ok
INLINE void CSwitch::SignalAttachedItems(void)
{
	Trig_SendPulse(Trig_GetLinksPointer(this->mNode));
}

// @Ok
void INLINE CSwitch::SwitchInactive(void)
{
	this->field_100 = 0;
}

// @Ok
CVector* CSwitch::GetAutoAimTargetPointer(void)
{
	switch(this->field_100)
	{
		case 1:
			return &this->field_10C;
		case 2:
		case 3:
		case 4:
		case 5:
			return &this->field_118;
		default:
			return NULL;
	}
}

// @Ok
void INLINE Switch_SetVisible(bool a1, CItem* pItem)
{
	print_if_false(pItem != 0, "Bad item");

	if (a1)
		pItem->mFlags &= 0xFFDE;
	else
		pItem->mFlags |= 0x21;
}

// @Ok
void INLINE CSwitch::SwitchOff(void)
{
	this->field_100 = 1;
	Switch_SetVisible(false, this->field_108);
	Switch_SetVisible(true, this->field_104);
}


void validate_CSwitch(void)
{
	VALIDATE_SIZE(CSwitch, 0x128);

	VALIDATE(CSwitch, field_F8, 0xF8);
	VALIDATE(CSwitch, field_FC, 0xFC);

	VALIDATE(CSwitch, field_100, 0x100);

	VALIDATE(CSwitch, field_104, 0x104);
	VALIDATE(CSwitch, field_108, 0x108);

	VALIDATE(CSwitch, field_10C, 0x10C);
	VALIDATE(CSwitch, field_118, 0x118);

	VALIDATE(CSwitch, field_124, 0x124);
}
