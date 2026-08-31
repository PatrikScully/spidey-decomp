#include "chunk.h"
#include "baddy.h"
#include "mem.h"
#include "bit.h"
#include "utils.h"

#include "validate.h"


extern CBody* ControlBaddyList;

// @MEDIUMTODO
// Investigated 2026-08-31, left as a stub, not attempted. Real Mac size is
// 940 bytes (tools/prototypes.json) vs 911 on PC (address 0x4273D0), so
// this is genuinely this size, not an inlining artifact. Findings for
// whoever picks this up next:
// - The function looks up an item by checksum (sub_4C9230, still unnamed,
//   likely "get item by checksum" given the debug strings it feeds:
//   "Bad checksum" / "Checksum not found"), then walks a per-region table
//   at dword_6B2474 with a "17 * region" stride, the same family CLAUDE.md
//   and m3dinit.cpp already document as opaque (dword_6B2454 in that same
//   family is ob.h's already-named CItemRelatedList; dword_6B2474 is 4
//   bytes further into what looks like the SAME row of tables, still
//   undocumented). It also reaches into gDCRegionItems (m3dinit.cpp,
//   0x5F6764, SDCRegionItem, 0x24 bytes/entry) but writes fields the
//   current SDCRegionItem struct does not name yet: a flag/byte at +0xC
//   (m3dinit.cpp's field_C, known) plus at least one more byte at +0xD
//   (masked with 0xBE then ORed with 0x40, currently inside
//   SDCRegionItem's PADDING(0x17)) and a count-like field at +0x14
//   (compared against a NumFaces value from a second table, also inside
//   the current padding).
// - It also walks two more record lists, one tagged "RuinModel" and one
//   "RuinChunk" in the debug strings ("Null DCModel for RuinModel.",
//   "NumFaces mismatch for RuinModel.", same pair for RuinChunk), each a
//   28 i16-element-stride array with completely unknown per-element
//   layout (only a 0xC0 flag-bits check and an 0x80 flag-set are used
//   here) plus a companion pointer array walked with a variable stride
//   (>> 18 of a packed field) that is not documented anywhere in the repo.
// - This touches the SAME family of undocumented struct-of-pointers
//   tables that m3dinit.cpp's M3dInit_ParsePSX comment already flags as
//   "a lot more risk than the tag suggests", spanning platform.cpp,
//   mysterio.cpp, shatter.cpp, spidey.cpp, shell.cpp, switch.cpp and now
//   this file. Getting SDCRegionItem's hidden fields and both RuinModel/
//   RuinChunk record layouts right needs a struct reverse-engineering
//   pass this session's scope did not cover. Left as a stub rather than
//   guess field roles inside a struct several other @Ok functions already
//   depend on the current (incomplete) layout of.
void Chunk_ChunkItemByChecksum(u32 Checksum)
{
	printf("void Chunk_ChunkItemByChecksum(u32 Checksum)");
}

// @Ok
// @Test
void CChunkControl::AddChunk(CItem* pItem)
{
	if ( this->field_FA < this->field_F8 )
	{
		SChunkEntry *entry = &this->field_FC[this->field_FA];
		this->field_FA++;
		entry->pItem = pItem;

		i32 v4 = Rnd(4096) & 0xFFF;
		i32 v5 = Rnd(32) + 32;

		entry->field_4.vx = v5 * rcossin_tbl[v4].sin;
		entry->field_4.vy = (-48 - Rnd(32)) << 12;
		entry->field_4.vz = v5 * rcossin_tbl[v4].cos;
		entry->field_14.vx = Rnd(512) - 256;
		entry->field_14.vy = Rnd(512) - 256;
		entry->field_14.vz = Rnd(512) - 256;
		entry->field_1C = Rnd(120) + 120;
	}
}

// @Ok
void CChunkControl::AI(void)
{
	i8 v1 = 1;
	for (u32 i = 0; i < this->field_FA; i++)
	{
		SChunkEntry *entry = &this->field_FC[i];

		if (entry->pItem)
		{
			if (entry->field_1C <= this->field_80)
			{
				Chunk_MakeItemDisappear(entry->pItem);
				entry->pItem = 0;
			}
			else
			{
				v1 = 0;
				entry->field_1C = entry->field_1C - this->field_80;
				entry->pItem->mPos = entry->field_4;

				entry->field_4.vy += 0x8000;

				entry->pItem->mAngles = entry->field_14;
			}
		}
	}

	if (v1)
	{
		this->Die();
	}
}

// @Ok
CChunkControl::~CChunkControl(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&ControlBaddyList));

	if (this->field_FC)
		Mem_Delete(static_cast<void*>(this->field_FC));
}

// @Ok
INLINE CChunkControl::CChunkControl(CVector* a2, u16 a3)
{
	this->mPos = *a2;
	this->AttachTo(reinterpret_cast<CBody**>(&ControlBaddyList));

	this->field_F8 = a3;
	if (this->field_F8)
		this->field_FC = static_cast<SChunkEntry*>(DCMem_New(32 * this->field_F8, 0, 1, 0, 1));
}

// @Ok
INLINE void Chunk_MakeItemDisappear(CItem* item)
{
	item->mFlags |= 0x21;
}

void validate_CChunkControl(void)
{
	VALIDATE_SIZE(CChunkControl, 0x100);

	VALIDATE(CChunkControl, field_F8, 0xF8);
	VALIDATE(CChunkControl, field_FA, 0xFA);
	VALIDATE(CChunkControl, field_FC, 0xFC);
}

void validate_SChunkEntry(void)
{
	VALIDATE_SIZE(SChunkEntry, 0x20);

	VALIDATE(SChunkEntry, pItem, 0x0);
	VALIDATE(SChunkEntry, field_4, 0x4);
	VALIDATE(SChunkEntry, field_14, 0x14);
	VALIDATE(SChunkEntry, field_1C, 0x1C);
}
