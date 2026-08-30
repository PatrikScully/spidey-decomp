#include "spool.h"
#include "utils.h"
#include "validate.h"
#include "utils.h"
#include "panel.h"
#include "PCTex.h"
#include "DXinit.h"
#include "dcfileio.h"
#include "crate.h"
#include "spidey.h"
#include "my_assert.h"
#include "SpideyDX.h"
#include "psx_types.h"
#include "ps2funcs.h"
#include "vram.h"
#include "m3dinit.h"
#include "m3dzone.h"

#include <cstring>
#include <cstdlib>

// @FIXME
// should be 0
EXPORT u8 gSpoolLogFailedTextureAccess = 1;

EXPORT i32 gRegionReloadRelated = -1;
EXPORT u8 gReloading = 1;

// @FIXME: add proper value
EXPORT void* gSpoolSystemMemory;

EXPORT i32 gSpoolRegionRelatedOne;
EXPORT i32 gSpoolRegionRelatedTwo;
EXPORT i32 GrenadeExplosionRegion = -1;
EXPORT i32 SymBurnRegion = -1;
EXPORT i32 FireDomeRegion = -1;
EXPORT i32 FireRingRegion = -1;


i32 gSpoolCurrentOpenSpot;
EXPORT AnimPacket* AnimPackets;
EXPORT i32 gSpoolInitOne;
EXPORT i32 gSpoolInitTwo;
EXPORT i32 gSpoolColijEnvIndex;
EXPORT i32 gNumAccesses;

i32 EnvRegions[2] = { -1, -1 };

const i32 MAXTEXTUREENTRIES = 512;
EXPORT Texture* gSpoolTexturesRelated;
EXPORT Texture gSpoolInitRelated[MAXTEXTUREENTRIES];

EXPORT SAccess* gAccessRelated[MAXPSX];

const i32 MAXITEMSPERCHECKSUM = 5;
EXPORT i16 gEnvModelHashTable[256][MAXITEMSPERCHECKSUM];

EXPORT SPSXRegion PSXRegion[MAXPSX];

#define TEXTURE_CHECKSUM_TABLE_SIZE (MAXTEXTUREENTRIES)
EXPORT Texture* TextureChecksumHashTable[TEXTURE_CHECKSUM_TABLE_SIZE];

EXPORT i32 lowGraphics;

//#define G_LOWGRAPHICS (lowGraphics)
#define G_LOWGRAPHICS (*reinterpret_cast<i32*>(0x006B78F8))

EXPORT i32 CurrentSuit;

//#define G_CURRENTSUIT (CurrentSuit)
#define G_CURRENTSUIT (*reinterpret_cast<i32*>(0x005559DC))

EXPORT u8 gGiveDefaultTexture;

EXPORT TextureEntry gTextureEntries[256];

//#define G_TEXTUREENTRIES (gTextureEntries)
#define G_TEXTUREENTRIES (reinterpret_cast<TextureEntry*>(0x006A90B8))

EXPORT i32 HashIndex;
EXPORT Texture* pCurrentTex;

EXPORT char SuitNames[11][32] = 
{
	"Dummy",
	"spidey",
	"sp2099",
	"spSymbi",
	"spUniv",
	"spUnlim",
	"spBagMan",
	"spScar",
	"spReilly",
	"spQuick",
	"spPark",

};

// @Ok
// @Validate
i32 Spool_PSX(
		const char* Filename,
		i32 IsEnviro)
{
	char v22[32];
	print_if_false(Filename != 0, "No FileName sent to Spool_PSX.");
	Utils_CopyString(Filename, v22, sizeof(v22));

	if ( !gLowGraphics && Utils_CompareStrings(v22, "spidey") )
		Utils_CopyString(SuitNames[CurrentSuit], v22, sizeof(v22));

	for (i32 i = 0; i < MAXPSX; i++)
	{
		if (PSXRegion[i].Filename[0] && Utils_CompareStrings(v22, PSXRegion[i].Filename))
		{
			return i;
		}
	}

	i32 openSpot = -1;
	for (i32 k = 0; k < MAXPSX; k++)
	{
		if (PSXRegion[k].Filename[0] == '\0')
		{
			openSpot = k;
			break;
		}
	}

	print_if_false(openSpot == -1, "Too many PSX files loaded, increase MAXPSXS in spool.h");
	if (IsEnviro)
	{
		print_if_false(EnvRegions[0] == -1, "Old environment still loaded");
		EnvRegions[0] = openSpot;
	}

	Utils_CopyString(v22, PSXRegion[openSpot].Filename, 9);
	gSpoolCurrentOpenSpot = openSpot;


	i32 v8;
	char v23[64];
	if (gLowGraphics && (sprintf(v23, "lowres\\%s.psx", v22), (v8 = FileIO_Open(v23)) != 0))
	{
		DXERR_printf("Loading LowRes Model: %s\r\n", v23);
		PSXRegion[openSpot].LowRes = 1;
	}
	else
	{
		sprintf(v23, "%s.psx", v22);
		v8 = FileIO_Open(v23);
	}

	void* v9 = DCMem_New(v8, 1, 1, 0, 1);
	PSXRegion[openSpot].pPSX = static_cast<u32*>(v9);

	FileIO_Load(v9);
	FileIO_Sync();
	ProcessNewPSX(openSpot);

	if (IsEnviro)
	{
		print_if_false(EnviroList == 0, "EnviroList not NULL");
		EnviroList = PSXRegion[openSpot].pSuper;
		gSpoolRegionRelatedOne = gSpoolRegionRelatedTwo;

		gCommandPointRelated[0] = Spool_SkipPackets(PSXRegion[openSpot].pPSX);
		Spool_AddEnvModelsToHashTable();
	}

	if ( Utils_CompareStrings(v22, "expgrnd") )
	{
		print_if_false(GrenadeExplosionRegion == -1, "GrenadeExplosionRegion already set?");
		GrenadeExplosionRegion = openSpot;
	}

	if ( Utils_CompareStrings(v22, "fire") )
	{
		print_if_false(SymBurnRegion == -1, "SymBurnRegion already set?");
		Spool_MaskFaceFlags(openSpot, 512, -1);
		SymBurnRegion = openSpot;
	}

	if ( Utils_CompareStrings(v22, "firedome") )
	{
		print_if_false(FireDomeRegion == -1, "FireDomeRegion already set?");
		FireDomeRegion = openSpot;
	}

	if ( Utils_CompareStrings(v22, "firering") )
	{
		print_if_false(FireRingRegion == -1, "FireRingRegion already set?");
		FireRingRegion = openSpot;
	}

	return openSpot;
}

// @Ok
// v5 points AT the pSkipped[v3+1] slot itself (the consecutive u32 slots
// starting there are the Texture* array), not at whatever value is stored
// there. Confirmed by disassembling this loop inlined into ClearRegion
// (0x4CA858: esi = &pSkipped[v3+1], no extra deref before the walk).
void DecrementTextureUsage(i32 region)
{
	i32 v3 = reinterpret_cast<i32*>(PSXRegion[region].ppModels)[-1];
	u32* pSkipped = Spool_SkipPackets(PSXRegion[region].pPSX);

	u32 v6 = pSkipped[v3];
	Texture** v5 = reinterpret_cast<Texture**>(&pSkipped[v3 + 1]);

	for (u32 i = 0; i < v6; i++)
	{
		print_if_false(v5[i]->Usage != 0, "Tried to decrement zero usage");
		v5[i]->Usage--;
	}
}

// @Ok
INLINE char* GetNextLine(char * a1)
{
	char * res = strchr(a1, '\r');
	if (res)
	{
		res++;
		if (*res == '\n')
			res++;
	}

	return res;
}

// @Ok
void GotoStartOfTextureList(void)
{
	HashIndex = 0;
	pCurrentTex = TextureChecksumHashTable[0];
}

// @Ok
// @Validate
// Functional: no standalone address exists, every call site in the shipped
// PC binary has this inlined (confirmed nowhere in .text as its own
// function). Cross-checked field by field against the inlined copy at
// 0x4C9C6B..0x4C9CD7 inside ProcessNewPSX: every store here matches,
// INCLUDING the 4-byte `x` write (it really does clear both x and y in one
// mov, `mov [eax+1Ch],ebp` in the original) and the field_12 low-nibble
// clear (original masks only the low byte with 0xF0; `&= 0xFFF0` on the
// full u16 field produces the identical byte pattern since the high byte
// is untouched either way). Store order differs slightly from the inlined
// original (harmless struct field-store reordering, see CLAUDE.md). No
// cmpsum address exists to verify byte-for-byte since this never appears
// standalone.
void NewTextureEntry(u32 checksum)
{
	print_if_false(
		gSpoolTexturesRelated != 0,
		"Run out of texture entries, increase MAXTEXTUREENTRIES in spool.cpp");

	Texture* pTex = gSpoolTexturesRelated;
	gSpoolTexturesRelated = gSpoolTexturesRelated->pNext;

	pTex->pNext = TextureChecksumHashTable[checksum % MAXTEXTUREENTRIES];
	pTex->pPrevious = 0;
	TextureChecksumHashTable[checksum % MAXTEXTUREENTRIES] = pTex;

	if (pTex->pNext != 0)
		pTex->pNext->pPrevious = pTex;

	pTex->Checksum = checksum;
	pTex->field_12 &= 0xFFF0;
	pTex->Usage = 0;
	pTex->clut = 0;
	pTex->u0 = 0;
	pTex->v0 = 0;
	pTex->u1 = 0;
	pTex->v1 = 0;

	// original really does write 4 bytes here, clearing x and y together
	*reinterpret_cast<u32*>(&pTex->x) = 0;
}

// @Ok
// Functional: reviewed while doing Spool_RemoveUnusedTextures (only caller,
// inlined there). Logic matches (bound check, null-bucket search, pNext
// advance). Byte-match residue only: the compiler always places the
// "search empty buckets" loop as the fall-through path and the "already
// have a texture" short path as a forward jump, no matter how the source
// is written (if/else either order, goto either polarity, do-while vs raw
// goto, operand order swapped, cached local vs global re-test, for(;;)+break
// single loop). Original does the opposite (short path falls through,
// search is the jump target). See Spool_RemoveUnusedTextures's comment for
// the residue this causes. No standalone address to run cmpsum against.
INLINE Texture* NextTexture(void)
{
	Texture* res;

	if (HashIndex >= TEXTURE_CHECKSUM_TABLE_SIZE)
		return 0;

	if (0 == pCurrentTex)
		goto search;

	res = pCurrentTex;
	pCurrentTex = pCurrentTex->pNext;
	return res;

search:
	do
	{
		HashIndex++;
		pCurrentTex = TextureChecksumHashTable[HashIndex];

		if (TEXTURE_CHECKSUM_TABLE_SIZE <= HashIndex)
			return 0;
	} while (0 == pCurrentTex);

	res = pCurrentTex;
	pCurrentTex = pCurrentTex->pNext;

	return res;
}

// @Ok
// @Validate
void PreProcessAnimPacket(
		u32 * pPSX,
		u32 * pPacket)
{
	print_if_false(!pPSX[2] && !pPSX[3], "Sequencer PSX contains items or models!");

	u32* pSkipped = Spool_SkipPackets(pPSX);

	AnimPacket* pAnimPacket = static_cast<AnimPacket*>(
			DCMem_New(sizeof(AnimPacket), 0, 1, 0, 1));

	pAnimPacket->pNext = AnimPackets;
	pAnimPacket->pPacket = pPacket;
	pAnimPacket->mPsxOpenSpot = gSpoolCurrentOpenSpot;

	AnimPackets = pAnimPacket;

	u32 numAnims = *pPacket;
	u32* v4 = &pPacket[1];
	print_if_false(numAnims < 0xC8, "Got a vast number of anims in sequencer PSX");

	for (u32 i = 0; i < numAnims; i++)
	{

		u8 v6 = *reinterpret_cast<u8*>(v4) >= 0x41 &&
			*reinterpret_cast<u8*>(v4) <= 0x5A;

		u32 v7 = v4[2];
		v4 += 3;
		print_if_false(v7 < 0x3E8, "Got a vast number of frames in sequencer PSX");

		for (u32 j = 0; j < v7; j++)
		{
			u32 v8 = pSkipped[v4[1] + 2];
			v4[1] = v8;
			if (!v6)
			{
				*reinterpret_cast<u8*>(v8) |= 0x20;
				u32 v9 = v4[1];

				u8 v10 = *reinterpret_cast<u8*>(v9 + 4);
				if (v10 != 0xFF)
				{
					*reinterpret_cast<u8*>(v9 + 4) = v10 + 1;
				}

				++*reinterpret_cast<u8*>(v4[1] + 9);
				u32 v11 = v4[1];
				u8 v12 = *reinterpret_cast<u8*>(v11 + 10);

				if (v12 != 0xFF)
				{
					*reinterpret_cast<u8*>(v11 +10) = v12 + 1;
				}

				++*reinterpret_cast<u8*>(v4[1] + 11);
			}

			v4 += 2;
		}
	}

	Bit_UpdateQuickAnimLookups();
}

// @BIGTODO
// Retagged from @MEDIUMTODO after reading the original at 0x4C9A60
// (roughly 0xA80 / 2688 bytes, ~150 lines of decompiled pseudocode). This
// is the main per-region PSX load/convert routine, called once from
// Spool_PSX right after the file loads. It is not a small function: it
// rebuilds the model array (constructing a CItem per model with the eh
// vector constructor iterator), fixes up model checksum pointers, resolves
// or creates every texture entry the PSX references (walking
// TextureChecksumHashTable, same idiom as Spool_FindTextureEntry /
// NewTextureEntry above), fixes up face material indices, then walks the
// PSX record list a second time dispatching on record type (id 0x45 =
// anim packet via PreProcessAnimPacket, id 6/7/10/42/44 = various region
// fields, id 0x734350C1... several large magic hash constants = texture
// group formats) and for each texture record picks one of several loaders
// (PCTex_CreateTexture16/256, PCTex_CreateTexturePVR, LTI replacement
// texture via PCTex_LoadLtiTexture) depending on packed flag/format bits.
// Every callee resolved to a named, already-decompiled function (none are
// stubs in this TU), so leaf-first is satisfied; the size and the amount
// of bit-packed, hash-dispatched logic is why this is BIGTODO scale, not
// MEDIUM. Left as a stub for a future session: this needs the full
// discipline's 10 hypotheses per diff cluster once written, and getting
// the many magic checksum constants and bitfield packs (offsets 0x00,
// 0x04, 0x08, 0x0A, 0x0C..0x13, 0x14 of the on-disk texture record; the
// runtime Texture struct's TexWin/field_12/x/y bit layout) right needs
// careful field-by-field cross-checking against texture.h, not a quick
// pass.
void ProcessNewPSX(i32)
{
    printf("ProcessNewPSX(i32)");
}

// @Ok
INLINE i32 RemoveAnimPacket(u32* pPacket)
{
	AnimPacket* pPrev = 0;
	AnimPacket* pIter;
	for (
			pIter = AnimPackets; 
			pIter;
			pIter = pIter->pNext)
	{
		if (pIter->pPacket == pPacket)
			break;
		pPrev = pIter;
	}

	print_if_false(pIter == 0, "Could not find anim packet to delete");
	
	if (pPrev)
		pPrev->pNext = pIter->pNext;
	else
		AnimPackets = pIter->pNext;

	Mem_Delete(pIter);
	Bit_UpdateQuickAnimLookups();
	return 0;
}

// @Ok
// @Validate
INLINE void RemoveTextureEntry(Texture* pTexture)
{
	if (pTexture->pNext)
		pTexture->pNext->pPrevious = pTexture->pPrevious;

	if (pTexture->pPrevious)
		pTexture->pPrevious->pNext = pTexture->pNext;

	u32 checksum = pTexture->Checksum % TEXTURE_CHECKSUM_TABLE_SIZE;
	if (pTexture == TextureChecksumHashTable[checksum])
		TextureChecksumHashTable[checksum] = pTexture->pNext;

	pTexture->pNext = gSpoolTexturesRelated;
	gSpoolTexturesRelated = pTexture;
}

// @Ok
// @Validate
INLINE void Spool_AddEnvModelsToHashTable(void)
{
	print_if_false(EnviroList != 0, "NULL EnviroList?");
	u32* pModelChecksums = PSXRegion[EnviroList->mRegion].pModelChecksums;
	print_if_false(pModelChecksums != 0, "NULL pChecksums?");

	u32 v16 = reinterpret_cast<u32>(PSXRegion[EnviroList->mRegion].ppModels[-1]);
	for (u32 v15 = 0; v15 < v16; v15++)
	{
		u32 checksumIndex = pModelChecksums[v15] % 256;

		i32 k;
		for (k = 0; gEnvModelHashTable[checksumIndex][k] >= 0; k++)
		{
			print_if_false(
				k < MAXITEMSPERCHECKSUM,
				"Too many items have the same checksum mod 256\n Need to increase MAXITEMSPERCHECKSUM in spool.cpp");
		}

		gEnvModelHashTable[checksumIndex][k] = v15;
	}
}

// @Ok
// @Matching
// SAnimFrame is still opaque here; field_4 is a Texture* found by disasm
// (matches the "texture" print_if_false messages), accessed raw since the
// struct has no layout yet.
i32 Spool_AnimAccess(char *a1, SAnimFrame **a2)
{
	AnimPacket* pPacketInfo = AnimPackets;
	if (pPacketInfo)
	{
	loop_top:
		u32* pPacket = pPacketInfo->pPacket;
		u32 numAnims = *pPacket;
		char* pEntry = reinterpret_cast<char*>(pPacket + 1);

		for (u32 i = 0; i < numAnims; i++)
		{
			char* pA = a1;
			char* pB = pEntry;
			char ca = *pA & 0xDF;
			char cb = *pB & 0xDF;

			i32 count;
			for (count = 0; ca == cb && ca && cb && count < 8; count++)
			{
				pA++;
				ca = *pA & 0xDF;
				pB++;
				cb = *pB & 0xDF;
			}

			if ((!ca && !cb) || count == 8)
			{
				*a2 = reinterpret_cast<SAnimFrame*>(pEntry + 0xC);

				if (addAccess(
							reinterpret_cast<void**>(a2),
							2,
							reinterpret_cast<u32>(a1),
							pPacketInfo->mPsxOpenSpot))
					accessLog(
							"Created Anim Access: name=%s, rgn=%i, addr=0x%8.8X\r\n",
							a1, pPacketInfo->mPsxOpenSpot, a2);

				print_if_false(
						*reinterpret_cast<Texture**>(reinterpret_cast<char*>(*a2) + 4) != 0,
						"Animation does not have a texture, huh-ho...");

				print_if_false(
						(*reinterpret_cast<Texture**>(reinterpret_cast<char*>(*a2) + 4))->mRegion == pPacketInfo->mPsxOpenSpot,
						"texture is in a different region than the animation, huh-ho...");

				return pPacketInfo->mPsxOpenSpot;
			}

			u32 numFrames = *reinterpret_cast<u32*>(pEntry + 8);
			pEntry += numFrames * 8 + 0xC;
		}

		pPacketInfo = pPacketInfo->pNext;
		if (pPacketInfo)
			goto loop_top;
	}

	accessLog(
			"Created Anim Access Fails [NOT FOUND]: name=%s, addr=0x%8.8X\r\n",
			a1, a2);
	*a2 = 0;
	return -1;
}

// @Ok
// @Matching
void Spool_ClearEnvironmentRegions(void)
{
	ClearRegion(EnvRegions[0], 1);
}

// @Ok
SAnimFrame* Spool_FindAnim(char *a1, i32 a2)
{
	AnimPacket* pPacketInfo = AnimPackets;
	if (pPacketInfo)
	{
	loop_top:
		u32* pPacket = pPacketInfo->pPacket;
		u32 numAnims = *pPacket;
		char* pEntry = reinterpret_cast<char*>(pPacket + 1);

		for (u32 i = 0; i < numAnims; i++)
		{
			char* pA = a1;
			char* pB = pEntry;
			char ca = *pA & 0xDF;
			char cb = *pB & 0xDF;

			i32 count;
			for (count = 0; ca == cb && ca && cb && count < 8; count++)
			{
				pA++;
				ca = *pA & 0xDF;
				pB++;
				cb = *pB & 0xDF;
			}

			if ((!ca && !cb) || count == 8)
				return reinterpret_cast<SAnimFrame*>(pEntry + 0xC);

			u32 numFrames = *reinterpret_cast<u32*>(pEntry + 8);
			pEntry += numFrames * 8 + 0xC;
		}

		pPacketInfo = pPacketInfo->pNext;
		if (pPacketInfo)
			goto loop_top;
	}

	return 0;
}

// @Ok
// @Matching
i32 Spool_GetEnvIndex(i32 a1)
{
	return (a1 != EnvRegions[0]) ? -1 : 0;
}

// @Ok
u32* Spool_GetPalette(u32 Checksum, i32 Region)
{
	i32 numModels = reinterpret_cast<i32*>(G_PSXREGION[Region].ppModels)[-1];
	// pModelChecksums has one extra trailing word after its numModels entries
	u32 skip = G_PSXREGION[Region].pModelChecksums[numModels];
	u32* pPSX = G_PSXREGION[Region].pPSX;

	u32 *i;
	for ( i = (u32 *)((char *)pPSX + pPSX[1]); *i != -1; )
	{
		i++;
		i = (u32 *)((char *)i + i[0] + 4);
	}

	u32* pPalette = &i[skip + numModels + 2];
	u32 numPalettes = *pPalette++;

	for (u32 n16 = 0; n16 < numPalettes; n16++)
	{
		if (*pPalette == Checksum)
			return pPalette + 1;

		pPalette = (u32*)((char*)pPalette + 0x24);
	}

	numPalettes = *pPalette++;

	for (u32 n256 = 0; n256 < numPalettes; n256++)
	{
		if (*pPalette == Checksum)
			return pPalette + 1;

		pPalette = (u32*)((char*)pPalette + 0x204);
	}

	print_if_false(0, "Unable to find palette.");
	return 0;
}

// @Ok
// @Matching
void Spool_Init(void)
{
	print_if_false(1u, "MAXPSXS is too big");
	PCTex_ReleaseAllTextures();
	PCTex_InitSystemTextures();

	for (i32 i = 0; i < MAXPSX; i++)
	{
		PSXRegion[i].Filename[0] = 0;
		PSXRegion[i].Protected = 0;
		PSXRegion[i].Usable = 0;
		PSXRegion[i].ppModels = 0;

		delete[] PSXRegion[i].pSuper;

		PSXRegion[i].pSuper = 0;
		PSXRegion[i].pPSX = 0;
		PSXRegion[i].pAnimFile = 0;
		PSXRegion[i].pHierarchy = 0;
		PSXRegion[i].pColourPulseData = 0;
		PSXRegion[i].pTexWibData = 0;
		PSXRegion[i].pHooks = 0;

		while(gAccessRelated[i])
		{
			SAccess* pCur = gAccessRelated[i];
			SAccess* next = pCur->pNext;
			gAccessRelated[i] = next;

			*pCur->pLst = 0;
			delete pCur;
		}
	}

	gSpoolTexturesRelated = &gSpoolInitRelated[0];
	for (i32 j = 0; j < TEXTURE_CHECKSUM_TABLE_SIZE; j++)
		TextureChecksumHashTable[j] = 0;

	for (i32 k = 0; k < 511; k++)
	{
		gSpoolInitRelated[k].pNext = &gSpoolInitRelated[k+1];
	}

	gSpoolInitRelated[511].pNext = 0;
	Spool_InitialiseEnvModelHashTable();

	AnimPackets = 0;
	gSpoolInitOne = 0;
	gSpoolInitTwo = 0;
	EnviroList = 0;
	gSpoolColijEnvIndex = 0;
	gNumAccesses = 0;

	EnvRegions[0] = -1;
	EnvRegions[1] = -1;

}

// @Ok
INLINE void Spool_InitialiseEnvModelHashTable(void)
{
	for (i32 i = 0; i < 256; i++)
	{
		for (i32 j = 0; j < MAXITEMSPERCHECKSUM; j++)
		{
			gEnvModelHashTable[i][j] = -1;
		}
	}
}

// @Ok
// @Validate
void Spool_MaskFaceFlags(
		i32 region,
		u32 a2,
		u32 a3)
{
	print_if_false(region < 0 || region >= 40, "Bad region number sent to Spool_MaskFaceFlags");

	print_if_false(PSXRegion[region].Usable != 0, "PSX not usable in call to Spool_MaskFaceFlags");

	i32* v4 = *reinterpret_cast<i32**>(PSXRegion[region].ppModels);
	i32 v5 = v4[-1];

	for (i32 i = 0; i < v5; i++)
	{
		i32 v6 = reinterpret_cast<u16*>(v4)[3];

		v4 += 2 * reinterpret_cast<u16*>(v4)[1] + 2 * reinterpret_cast<u16*>(v4)[2] + 7;

		for (i32 j = 0; j < v6; j++)
		{
			u32 v8 = a3 & (*v4 | a2);
			*v4 = v8;

			v4 += v8 >> 18;
		}
	}
}

// @Ok
// @Validate
void Spool_ReloadAll(void)
{
	i32 currentSuit = CurrentSuit;
	gReloading = 0;

	for (i32 i = 0; i < MAXPSX; i++)
	{
		i32 IsEnviro = EnvRegions[0] == i;

		if (PSXRegion[i].Filename[0])
		{
			char a1[12];
			u8 backupProtected = PSXRegion[i].Protected;

			strncpy(a1, PSXRegion[i].Filename, 8);
			a1[8] = 0;
			if (gLowGraphics && Utils_CompareStrings(a1, SuitNames[CurrentSuit]))
			{
				Utils_CopyString("spidey", a1, 9);
				CurrentSuit = 1;
			}

			PSXRegion[i].Protected = 0;
			ClearRegion(i, 1);
			Spool_PSX(a1, IsEnviro);
			PSXRegion[i].Protected = backupProtected;
			restoreRegionAccess(i);
		}
	}

	if (currentSuit != CurrentSuit)
		Spidey_LoadAlternativeTextureSet(0, currentSuit);

	gReloading = 1;
	if (!gLowGraphics && gRegionReloadRelated >= 0)
	{
		PSXRegion[gRegionReloadRelated].Protected = 0;
		ClearRegion(gRegionReloadRelated, 1);
		gRegionReloadRelated = -1;
	}
}

// @Ok
// @Matching
void Spool_RemoveAccess(void **pLst, i32 region)
{
	SAccess* pPrev = 0;
	SAccess* pIter;

	for (pIter = gAccessRelated[region]; pIter; pIter = pIter->pNext)
	{
		if (pIter->pLst == pLst)
			break;

		pPrev = pIter;
	}

	if (pIter)
	{
		if (pPrev)
			pPrev->pNext = pIter->pNext;
		else
			gAccessRelated[region] = pIter->pNext;

		free(pIter);
		gNumAccesses--;
		*pLst = 0;
	}
}

// @Ok
void Spool_Sync(void)
{
}

// @Ok
// @Matching
i32 Spool_TextureAccess(
		u32 checksum,
		Texture **ppTexture)
{
	Texture* pTexture;
	for (
			pTexture = TextureChecksumHashTable[checksum % TEXTURE_CHECKSUM_TABLE_SIZE];
			pTexture;
			pTexture = pTexture->pNext)
	{
		if (pTexture->Checksum == checksum)
			break;
	}

	if (pTexture)
	{
		*ppTexture = pTexture;

		if (addAccess(
					reinterpret_cast<void**>(ppTexture),
					3,
					checksum,
					pTexture->mRegion))
			accessLog(
					"Created Texture Access: csum=%8.8X, rgn=%i, addr=0x%8.8X\r\n",
						checksum, pTexture->mRegion, ppTexture);
		return pTexture->mRegion;
	}

	if (!gSpoolLogFailedTextureAccess)
	{
		print_if_false(0, "Can't find texture from checksum %ld", checksum);
		*ppTexture = gAnimTable[13]->pTexture;
		accessLog(
				"Create Texture Access Fails [NOT FOUND]: csum=%8.8X, rgn=%i, addr=0x%8.8X\r\n",
				checksum, gAnimTable[13]->pTexture->mRegion, ppTexture);
		return gAnimTable[13]->pTexture->mRegion;
	}

	accessLog("Create Texture Access Fails [NOT FOUND]: csum=%8.8X, addr=0x%8.8X\r\n", checksum, ppTexture);
	return -1;
}

// @MEDIUMTODO
// Not in the PC binary. Checked tools/names.json and the maintainer's PC
// IDB name list (idbs/spideypc_names.txt, ~3970 code names) for
// "SwapPSXFile": no match. No caller anywhere in the PC source tree
// either (grep for SwapPSXFile only finds spool.h/spool.cpp). It exists
// only on the Mac PowerPC build (idbs/spiderman_names.txt:
// 001244b0 SwapPSXFile__FPUl, 728 bytes there per prototypes.json). This
// is a byte-swap routine: PSX resource files are little-endian, so a
// big-endian PowerPC build needs to swap every multi-byte field after
// loading, while PC (x86, also little-endian) does not, so the original
// source almost certainly compiles this out on PC with an #ifdef on
// platform endianness. Left as a stub: there is no PC address to verify
// against, so any implementation here would be unverifiable guesswork.
void SwapPSXFile(u32 *)
{
    printf("SwapPSXFile(u32 *)");
}

// @MEDIUMTODO
// Same situation as SwapPSXFile above: not in names.json, not in the PC
// IDB name list, no PC caller, only present on Mac
// (idbs/spiderman_names.txt: 00123e70 SwapPSXPacketData__FPUl, 1296 bytes
// on Mac). Byte-swap routine, dead on a little-endian PC build. Left as a
// stub, no PC address to verify against.
void SwapPSXPacketData(u32 *)
{
    printf("SwapPSXPacketData(u32 *)");
}

// @SMALLTODO
// Same situation as SwapPSXFile/SwapPSXPacketData: not in names.json, not
// in the PC IDB name list, no PC caller, only present on Mac
// (idbs/spiderman_names.txt: 001243b0
// SwapPSXTextureData__FPUlPP7TexturePUl, 196 bytes on Mac). Byte-swap
// routine, dead on a little-endian PC build. Left as a stub, no PC
// address to verify against.
void SwapPSXTextureData(u32 *,Texture **,u32 *)
{
    printf("SwapPSXTextureData(u32 *,Texture **,u32 *)");
}

// @Ok
// @Matching
// release build strips the logger body; folds to the same address as every
// other no-op debug call (nullsub_5, 0x502D50), confirmed by disassembling
// Spool_TextureAccess's 3 accessLog call sites in the original binary.
void accessLog(char *, ...)
{
}

// @Ok
// @Matching
i32 addAccess(
		void** pLst,
		u32 type,
		u32 nameOrChecksum,
		i32 region)
{
	SAccess* pIter;
	for (pIter = PSXRegion[region].pAccess; pIter; pIter = pIter->pNext)
	{
		if (pIter->pLst == pLst)
			break;
	}

	if (pIter)
		return 0;

	SAccess* pAccess = static_cast<SAccess*>(malloc(sizeof(SAccess)));
	if (!pAccess)
	{
		return 0;
	}

	pAccess->pNext = gAccessRelated[region];
	pAccess->pLst = pLst;
	pAccess->mType = type;

	if ( type == 2 || type == 4 )
		strncpy(pAccess->mName, reinterpret_cast<char*>(nameOrChecksum), 8u);
	else
		pAccess->mChecksum = nameOrChecksum;

	gAccessRelated[region] = pAccess;
	gNumAccesses++;
	return 1;
}

// @Ok
INLINE void restoreRegionAccess(i32 region)
{
	for (
			SAccess* pAccess = gAccessRelated[region];
			pAccess;
			pAccess = pAccess->pNext)
	{
		switch (pAccess->mType)
		{
			case 0:
				pAccess->pLst = reinterpret_cast<void**>(Spool_GetModel(pAccess->mChecksum, region));
				break;
			case 1:
				print_if_false(region == EnvRegions[0], "Non-Enviro Item Accessed!");
				pAccess->pLst = reinterpret_cast<void**>(Spool_FindEnviroItem(pAccess->mChecksum));
				break;
			case 2:
				pAccess->pLst = reinterpret_cast<void**>(Spool_FindAnim(pAccess->mName, 1));
				break;
			case 3:
				pAccess->pLst = reinterpret_cast<void**>(Spool_FindTextureEntry(pAccess->mChecksum));
				break;
			case 4:
				pAccess->pLst = reinterpret_cast<void**>(Spool_FindTextureEntry(pAccess->mName));
				break;
			default:
				pAccess->pLst = 0;
				break;
		}

		print_if_false(pAccess->pLst != 0, "Unable to restore previous PSX access!");
	}
}

// @Ok
// @Validate
void texClearChecksums(char* pTexName)
{
	char v9[16];
	sprintf(v9, "%s.tex", pTexName);
	
	i32 v1 = FileIO_Open(v9);
	if (v1)
	{
		char* v2 = reinterpret_cast<char*>(gSpoolSystemMemory);
		print_if_false(v2 != 0, "Out of system memory.");
		FileIO_Load(v2);
		FileIO_Sync();

		v2[v1] = 0;

		while (v2)
		{
			char v8;
			char v10[256];

			sscanf(v2, "%lx %s", &v8, v10);
			strlwr(v10);

			for (i32 i = 0; i < 256; i++)
			{
				if(gTextureEntries[i].Active &&
						strstr(v10, gTextureEntries[i].Name))
				{
					gTextureEntries[i].Active = 0;
					break;
				}
			}
			v2 = GetNextLine(v2);
		}
	}
}

// @Ok
// @Validate
void texLoadChecksums(char *pTexName)
{
	i32 checksumIndex = 0;
	char v12[16];
	sprintf(v12, "%s.tex", pTexName);

	i32 v2 = FileIO_Open(v12);

	char v13[256];

	if (v2)
	{
		char* v3 = reinterpret_cast<char*>(gSpoolSystemMemory);
		print_if_false(v3 != 0, "Out of system memory.");
		FileIO_Load(v3);
		FileIO_Sync();

		v3[v2] = 0;

		while (v3)
		{
			print_if_false(checksumIndex < 256, "Too many checksums.");
			if (!gTextureEntries[checksumIndex].Active)
			{
				sscanf(
						v3,
						"%lx %s",
						gTextureEntries[checksumIndex].Checksum, 
						v13);

				strlwr(v13);

				char* pNameStart = reinterpret_cast<char*>(reinterpret_cast<i32>(strrchr(v13, '\\')) - reinterpret_cast<i32>(v13) + 1);

				for (i32 i = 0; i < 32; i++)
				{
					char c = pNameStart[i];
					gTextureEntries[checksumIndex].Name[i] = c;

					if (c == ' ')
					{
						if (pNameStart[i+1] < 'a' ||
								pNameStart[i+1] > 'z')
							break;
					}
				}
			}

			gTextureEntries[checksumIndex].Active = 1;

			checksumIndex++;
			v3 = GetNextLine(v3);
		}
	}
}


// @Ok
// @Matching
u32 Spool_GetModelChecksum(CItem *pItem)
{
	return G_PSXREGION[pItem->mRegion].pModelChecksums[pItem->mModel];
}

// @Ok
// @Matching
CItem* Spool_FindEnviroItem(u32 Checksum)
{
	CItem* pList = EnviroList;
	if (pList)
	{
		u32* pModelChecksums = PSXRegion[EnviroList->mRegion].pModelChecksums;
		print_if_false(pModelChecksums != 0, "NULL pChecksums?");

		i16* pHashes = gEnvModelHashTable[Checksum % 256];

		for (i32 i = 0, curHash = pHashes[i]; curHash >= 0; curHash = pHashes[i])
		{
			if (Checksum == pModelChecksums[curHash])
			{
				return
					reinterpret_cast<CItem*>(&reinterpret_cast<char*>(pList)[0x40 * curHash]);
			}
			
			if (++i >= 5)
				break;
		}
	}

	return 0;
}

// @Ok
// @Matching
i32 Spool_FindRegion(const char *pName)
{
	char fileName[0x20];
	ASSERT(pName != 0, "No FileName sent to Spool_PSX.");
	Utils_CopyString(pName, fileName, sizeof(fileName));

	if (!G_LOWGRAPHICS)
	{
		if (Utils_CompareStrings(fileName, "spidey"))
			Utils_CopyString(SuitNames[G_CURRENTSUIT], fileName, sizeof(fileName));
	}

	for (i32 i = 0; i < MAXPSX; i++)
	{
		if (Utils_CompareStrings(fileName, G_PSXREGION[i].Filename))
		{
			return i;
		}
	}

	return -1;
}

// @Ok
// @Matching
u32 Spool_GetModel(u32 Checksum, i32 Region)
{
	print_if_false(Region >= 0 && Region < MAXPSX, "Bad region number sent to Spool_GetModel");
	print_if_false(PSXRegion[Region].Usable != 0, "PSX not usable in call to Spool_GetModel");

	u32* pChecksum = PSXRegion[Region].pModelChecksums;
	u32 numModels = reinterpret_cast<u32*>(PSXRegion[Region].ppModels)[-1];

	for (u32 i = 0; i < numModels; i++)
	{
		if (*pChecksum == Checksum)
			return i;

		pChecksum++;
	}

	print_if_false(0, "Model checksum not found in call to Spool_GetModel");
	return 0;
}

// @Ok
// Functional: walks id/size/data records terminated by -1, same walk idiom
// as Spool_GetPalette (see CLAUDE.md matching tricks). Returns one dword
// past the terminator, matching every caller's use of the result (start of
// the packed Texture* array right after the record list). Checked against
// this loop inlined into ClearRegion's DecrementTextureUsage call
// (0x4CA83E): next = pRecord + 8 + size in both, our split walk
// (i++; i=(u32*)((char*)i+i[0]+4);) reaches the same address as the
// combined form, the original just emits it as a separate "add eax,4" then
// "lea" instead of one folded lea. Instruction-shape residue only.
INLINE u32 *Spool_SkipPackets(u32 *pPSX)
{
	u32 *i; // r4
	for ( i = (u32 *)((char *)pPSX + pPSX[1]); *i != -1; )
	{
		i++;
		i = (u32 *)((char *)i + i[0] + 4);
	}

	return i + 1;
}

// @Ok
// Functional: region clear, logic verified against Hex-Rays at 0x4ca7a0.
// Clears the region's texture usage, pSuper, and fields, walks the PSX
// packets (RemoveAnimPacket for 0x45 records), frees the access list, and
// calls DCClearRegion. (The 42 mnemonic diffs from the byte-match phase are
// the RemoveAnimPacket packet-walk pointer-advance shape; the logic is
// equivalent.)
void ClearRegion(i32 region, i32 a2)
{
	if (region == -1)
		return;

	print_if_false(region >= 0 && region < MAXPSX, "Bad region number sent to ClearRegion");

	if (!PSXRegion[region].Filename[0])
		return;

	if (region == GrenadeExplosionRegion)
		GrenadeExplosionRegion = -1;

	if (region == SymBurnRegion)
		SymBurnRegion = -1;

	if (region == FireDomeRegion)
		FireDomeRegion = -1;

	if (region == FireRingRegion)
		FireRingRegion = -1;

	DecrementTextureUsage(region);

	delete[] PSXRegion[region].pSuper;
	PSXRegion[region].pSuper = 0;
	PSXRegion[region].pAnimFile = 0;
	PSXRegion[region].pColourPulseData = 0;
	PSXRegion[region].pTexWibData = 0;
	PSXRegion[region].Protected = 0;
	PSXRegion[region].Usable = 0;
	PSXRegion[region].pHooks = 0;
	PSXRegion[region].ppModels = 0;

	texClearChecksums(PSXRegion[region].Filename);

	PSXRegion[region].Filename[0] = 0;
	PSXRegion[region].LowRes = 0;

	if (region == EnvRegions[0])
	{
		M3dZone_FreePSX(0);
		EnvRegions[0] = -1;
		gSpoolRegionRelatedOne = 0;
		EnviroList = 0;
		Spool_InitialiseEnvModelHashTable();
	}

	u32* pPSX = PSXRegion[region].pPSX;
	u32* pRecord = reinterpret_cast<u32*>(reinterpret_cast<char*>(pPSX) + pPSX[1]);

	while (*pRecord != 0xFFFFFFFF)
	{
		u32 size = pRecord[1];
		u32* pData = pRecord + 2;

		if (pRecord[0] == 0x45)
			RemoveAnimPacket(pData);

		pRecord = reinterpret_cast<u32*>(reinterpret_cast<char*>(pData) + size);
	}

	if (!gReloading)
	{
		while (gAccessRelated[region])
		{
			SAccess* pAccess = gAccessRelated[region];
			gAccessRelated[region] = pAccess->pNext;
			*pAccess->pLst = 0;
			free(pAccess);
			gNumAccesses--;
		}
	}

	Mem_Delete(PSXRegion[region].pPSX);
	PSXRegion[region].pPSX = 0;

	DCClearRegion(region);

	if (a2)
		Spool_RemoveUnusedTextures();
}

// @Ok
// Functional: remove unused textures, logic verified against Hex-Rays at
// 0x4c9680. Walks the texture list, and for each texture with no usage and
// no format bits, decrements the palette usage, releases the texture,
// unpacks the VRAM rect, and removes it from the list (pushing it onto the
// gSpoolTexturesRelated free list). (The 61 mnemonic diffs from the
// byte-match phase are the NextTexture() null-check fall-through shape; the
// logic is equivalent.)
void Spool_RemoveUnusedTextures(void)
{
	GotoStartOfTextureList();

	Texture* pTex;
	while ((pTex = NextTexture()) != 0)
	{
		if ((pTex->field_12 & 0xF) == 0 || pTex->Usage != 0)
			continue;

		if (pTex->pPalette)
		{
			print_if_false(pTex->pPalette->Usage != 0, "Palette usage counter error!");
			pTex->pPalette->Usage--;
		}

		if (!gClearImagePrint)
			stubbed_printf("stubbed out: ClearImage");

		PCTex_ReleaseTexture(pTex->clut, true);

		// original really does read x and y together as one 32-bit pointer
		// into the packed VRAM rect, same idiom as NewTextureEntry's x/y clear
		VRAMRectUnpack(*reinterpret_cast<tagSVRAMRect**>(&pTex->x));

		RemoveTextureEntry(pTex);
	}
}

// @Ok
// @Matching
void Spool_ClearPSX(const char* Filename)
{

	char v3[32]; // [esp+8h] [ebp-40h] BYREF
	char v4[32]; // [esp+28h] [ebp-20h] BYREF

	ASSERT(Filename != 0, "No FileName sent to Spool_PSX.");
	Utils_CopyString(Filename, v3, sizeof(v3));

	if ( !G_LOWGRAPHICS && Utils_CompareStrings(v3, "spidey") )
		Utils_CopyString(SuitNames[G_CURRENTSUIT], v3, 32);

	ASSERT(v3 != 0, "No FileName sent to Spool_PSX.");
	Utils_CopyString(v3, v4, sizeof(v4));

	if ( !G_LOWGRAPHICS && Utils_CompareStrings(v4, "spidey") )
		Utils_CopyString(SuitNames[G_CURRENTSUIT], v4, 32);

	i32 index = 0;

	while (1)
	{
		if (Utils_CompareStrings(v4, G_PSXREGION[index].Filename))
			break;

		index++;
		if (index > MAXPSX)
		{
			index = -1;
			break;
		}
	}

	ClearRegion(index, 1);
}

// @Ok
// @Matching
void Spool_ClearAllPSXs(void)
{
	for (i32 i = 0; i < MAXPSX; i++)
	{
		if (!G_PSXREGION[i].Protected)
			ClearRegion(i, 1);
	}

	Spool_RemoveUnusedTextures();
}

// @Ok
// @Matching
// Walks the checksum hash bucket for the texture. If nothing is found and
// gSpoolLogFailedTextureAccess is 0, logs the miss and hands back the
// default texture (gAnimTable[13]->pTexture), same idiom as the other
// spool "not found" fallbacks. Original at 0x4C9460.
Texture *Spool_FindTextureEntry(u32 checksum)
{
	Texture *pSearch;
	for (pSearch = TextureChecksumHashTable[checksum & 511];
			pSearch;
			pSearch = pSearch->pNext)
	{
		if (pSearch->Checksum == checksum)
			break;
	}

	if (!pSearch)
	{
		if (!gSpoolLogFailedTextureAccess)
		{
			print_if_false(0, "Can't find texture from checksum %ld", checksum);
			return gAnimTable[13]->pTexture;
		}
	}

	return pSearch;
}

// @Ok
Texture *Spool_FindTextureEntry(char *name)
{
	char localName[256];
	strcpy(localName, name);
	strlwr(localName);

	i32 index;
	for (index = 0; index < 256; index++)
	{
		TextureEntry *currentEntry = &gTextureEntries[index];
		if (!strcmp(currentEntry->Name, localName) && currentEntry->Active)
			break;
	}

	if (index >= 256)
		return gAnimTable[13]->pTexture;

	return Spool_FindTextureEntry(gTextureEntries[index].Checksum);
}

// @Ok
// @Matching
u32 Spool_FindTextureChecksum(char *name)
{
	char localName[256];
	strcpy(localName, name);
	strlwr(localName);

	i32 index;
	for (index = 0; index < 256; index++)
	{
		if (!strcmp(G_TEXTUREENTRIES[index].Name, localName) && G_TEXTUREENTRIES[index].Active)
			break;
	}

	if (index < 256)
		return G_TEXTUREENTRIES[index].Checksum;

	return 0;
}

void validate_SPSXRegion(void)
{
	VALIDATE_SIZE(SPSXRegion, 0x44);

	VALIDATE(SPSXRegion, Filename, 0x0);
	VALIDATE(SPSXRegion, Usable, 0xA);
	VALIDATE(SPSXRegion, Protected, 0xB);
	VALIDATE(SPSXRegion, pModelChecksums, 0xC);
	VALIDATE(SPSXRegion, pSuper, 0x10);
	VALIDATE(SPSXRegion, ppModels, 0x14);
	VALIDATE(SPSXRegion, pPSX, 0x18);
	VALIDATE(SPSXRegion, pAnimFile, 0x1C);
	VALIDATE(SPSXRegion, pHierarchy, 0x20);
	VALIDATE(SPSXRegion, pHooks, 0x24);
	VALIDATE(SPSXRegion, pColourTable, 0x28);
	VALIDATE(SPSXRegion, pTexWibData, 0x2C);
	VALIDATE(SPSXRegion, pColourPulseData, 0x30);

	VALIDATE(SPSXRegion, NumParts, 0x38);

	VALIDATE(SPSXRegion, LowRes, 0x3B);

	VALIDATE(SPSXRegion, pAccess, 0x3C);
}

void validate_TextureEntry(void)
{
	VALIDATE_SIZE(TextureEntry, 0x28);

	VALIDATE(TextureEntry, Active, 0x0);
	VALIDATE(TextureEntry, Name, 0x1);
	VALIDATE(TextureEntry, Checksum, 0x24);
}

void validate_SAccess(void)
{
	VALIDATE_SIZE(SAccess, 0x14);

	VALIDATE(SAccess, pNext, 0x0);
	VALIDATE(SAccess, pLst, 0x4);
	VALIDATE(SAccess, mType, 0x8);
	VALIDATE(SAccess, mName, 0xC);
	VALIDATE(SAccess, mChecksum, 0xC);
}

void validate_AnimPacket(void)
{
	VALIDATE_SIZE(AnimPacket, 0xC);

	VALIDATE(AnimPacket, pPacket, 0x0);
	VALIDATE(AnimPacket, pNext, 0x4);
	VALIDATE(AnimPacket, mPsxOpenSpot, 0x8);
}

void validate_SModel(void)
{
	VALIDATE_SIZE(SModel, 36);

	VALIDATE(SModel, Flags, 0);
	VALIDATE(SModel, NumNormals, 4);

	VALIDATE(SModel, NumFaces, 6);
	VALIDATE(SModel, Radius, 8);
	VALIDATE(SModel, Box, 0xC);

	VALIDATE(SModel, zMax, 0x18);
	VALIDATE(SModel, NextLOD, 0x1A);

	VALIDATE(SModel, Vertices, 0x1C);
}

void validate_POLY_F3(void)
{
	VALIDATE_SIZE(POLY_F3, 0x14);

	VALIDATE(POLY_F3, tag, 0x0);
	VALIDATE(POLY_F3, r0, 0x4);
	VALIDATE(POLY_F3, g0, 0x5);
	VALIDATE(POLY_F3, b0, 0x6);
	VALIDATE(POLY_F3, code, 0x7);

	VALIDATE(POLY_F3, x0, 0x8);
	VALIDATE(POLY_F3, y0, 0xA);

	VALIDATE(POLY_F3, x1, 0xC);
	VALIDATE(POLY_F3, y1, 0xE);

	VALIDATE(POLY_F3, x2, 0x10);
	VALIDATE(POLY_F3, y2, 0x12);
}


#include "my_patch.h"


// @Bogus
void patch_spool(void)
{
	PATCH_PUSH_RET(0x004CA5A0, Spool_FindRegion);
	PATCH_PUSH_RET(0x004C9430, Spool_GetModelChecksum);
	PATCH_PUSH_RET(0x004CA750, Spool_ClearAllPSXs);
	PATCH_PUSH_RET(0x004CA640, Spool_ClearPSX);
	PATCH_PUSH_RET(0x004C95C0, Spool_FindTextureChecksum);
	PATCH_PUSH_RET(0x004C97B0, Spool_GetPalette);
}
