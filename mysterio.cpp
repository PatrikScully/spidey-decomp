#include "mysterio.h"
#include "validate.h"
#include "ps2redbook.h"
#include "ps2pad.h"
#include "trig.h"
#include "ai.h"
#include "utils.h"
#include "panel.h"
#include "ps2lowsfx.h"
#include "m3dutils.h"
#include "m3dcolij.h"
#include "spidey.h"
#include "front.h"
#include "pal.h"
#include "mem.h"

extern struct tag_S_Pal *pPaletteList;

EXPORT SLight M3d_MysterioLight =
{
  { { -2430, -2228, -2430 }, { 2509, -2896, 1447 }, { -648, -3711, -1607 } },
  0,

  { { 3200, 1040, 2048 }, { 2720, 1600, 1920 }, { 2400, 2560, 2048 } },
  0,
  { 1800, 1800, 1440 }
};



EXPORT SHandle gMystHandle;

// tentative, address not in the maintainer's IDB. Guards "only one
// CFadePalettes at a time" (message: "Tried to create two global fade
// palettes"), set here and cleared in the destructor.
EXPORT i32 gFadePalettesActive;

// tentative, address not in the maintainer's IDB. Saved/restored triple of
// RGB shift values, read by the constructor, written back by the destructor.
EXPORT u8 gPaletteFadeRGB[3];
EXPORT u8 gPaletteFadeRGB2[3];

// @NotOk
CFadePalettes::CFadePalettes(u8 a1, u8 a2, u8 a3)
{
	print_if_false(gFadePalettesActive == 0, "Tried to create two global fade palettes");
	gFadePalettesActive = 1;

	this->field_458 = a1 >> 3;
	this->field_459 = a2 >> 3;
	this->field_45A = a3 >> 3;
	this->field_45C = 1;

	this->field_45D = gPaletteFadeRGB[0];
	this->field_45E = gPaletteFadeRGB[1];
	this->field_45F = gPaletteFadeRGB[2];

	tag_S_Pal * volatile pPal = pPaletteList;
	if (pPal)
	{
		do
		{
			u16 hi = pPal->Clut >> 6;
			u8 flags = pPal->flags;
			u16 lo = (pPal->Clut & 0x3F) << 4;

			if (flags & 1)
			{
				print_if_false(this->field_450 < 0xC0, "More 16C palettes used than expected");
				this->field_3C[this->field_450] = DCMem_New(0x44, 1, 1, 0, 1);
				*reinterpret_cast<u16*>(this->field_3C[this->field_450]) = lo;
				*reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_3C[this->field_450]) + 2) = hi;
				StoreImage();
				StoreImage();
				this->field_450++;
			}
			else
			{
				print_if_false(this->field_454 < 0x44, "More 256C palettes used than expected");
				this->field_33C[this->field_454] = DCMem_New(0x404, 1, 1, 0, 1);
				*reinterpret_cast<u16*>(this->field_33C[this->field_454]) = lo;
				*reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_33C[this->field_454]) + 2) = hi;
				StoreImage();
				StoreImage();
				this->field_454++;
			}

			pPal = pPal->pNext;
		} while (pPal);
	}

	DrawSync();
}

// @Ok
// @Matching
INLINE void CFadePalettes::FadeDown(void)
{
	if (this->field_45B != 1 && this->field_45B != 3)
	{
		this->mAge = 0;
		this->field_45B = 1;
	}
}

static u8 * const gPSXRegionActiveFlags = (u8*)0x6B244A;
// tentative, not in the maintainer's IDB. Nearest named neighbours are
// PSXRegion at 0x6B2440 and CItemRelatedList's identical "index*17" region
// table at 0x6B2454 (ob.h, also used by CMysterio's ctor and platform.cpp);
// guessing a byte flag per region slot with the same stride and indexing.

// @NotOk
// residue: cmpsum against 0x45c030 shows 406 mnemonic diffs starting at the
// very first instruction (ebp vs esi as the "this" register), so this is
// functional-shape only, not register-verified. High confidence on the
// overall algorithm, from a full stack-slot trace of the original: RGB1555
// palette entries in field_3C/field_33C are nudged 3 units per channel, per
// call, toward a paired "target" table one step closer, plus an optional
// gPaletteFadeRGB/gPaletteFadeRGB2 clamp gated by field_45C, split into two
// symmetrical phases picked by field_45B (0 = fading toward
// field_458/459/45A, 1 = fading back toward field_45D/45E/45F, 3 = Die()).
// Low confidence on: the exact field_44C region-flag indexing shape, and
// whether the two field_3C/field_33C blend loops really get duplicated per
// phase in the compiled output (written that way here, matching the
// disassembly's four separate un-shared loop bodies) or whether some other
// source shape produces the same four copies. Only one attempt made past
// the initial translation (this is a >1000 byte function; the "Matching
// discipline" 10-hypotheses-per-cluster bar was not met, so this stays
// @NotOk rather than @AlmostMatching).
void CFadePalettes::Move(void)
{
	print_if_false(
			gPSXRegionActiveFlags[this->field_44C * 17 * 4] != 0,
			"Region became unusable");

	switch (this->field_45B)
	{
		case 0:
		{
			if (this->mAge > 0xB)
			{
				this->mAge = 0;
				this->field_45B = 2;
				return;
			}
			this->mAge++;

			if (this->field_45C)
			{
				i32 r = gPaletteFadeRGB[0] >> 3;
				i32 g = gPaletteFadeRGB[1] >> 3;
				i32 b = gPaletteFadeRGB[2] >> 3;

				i32 tr = this->field_458;
				i32 tg = this->field_459;
				i32 tb = this->field_45A;

				if (r > tr) { r -= 3; if (r < tr) r = tr; }
				else { r += 3; if (r > tr) r = tr; }

				if (g > tg) { g -= 3; if (g < tg) g = tg; }
				else { g += 3; if (g > tg) g = tg; }

				if (b > tb) { b -= 3; if (b < tb) b = tb; }
				else { b += 3; if (b > tb) b = tb; }

				gPaletteFadeRGB[0] = static_cast<u8>(r << 3);
				gPaletteFadeRGB[1] = static_cast<u8>(g << 3);
				gPaletteFadeRGB[2] = static_cast<u8>(b << 3);

				gPaletteFadeRGB2[0] = static_cast<u8>(r << 3);
				gPaletteFadeRGB2[1] = static_cast<u8>(g << 3);
				gPaletteFadeRGB2[2] = static_cast<u8>(b << 3);
			}

			{
				i32 i;

				for (i = 0; i < this->field_450; i++)
				{
					u16 *pLo = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_3C[i]) + 4);
					u16 *pHi = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_3C[i]) + 0x24);

					for (i32 j = 0; j < 16; j++)
					{
						u16 target = pHi[j];

						if (target & 0x7FFF)
						{
							u16 current = pLo[j];

							i32 tr = target & 0x1F;
							i32 tg = (target >> 5) & 0x1F;
							i32 tb = (target >> 10) & 0x1F;

							i32 cr = current & 0x1F;
							i32 cg = (current >> 5) & 0x1F;
							i32 cb = (current >> 10) & 0x1F;

							if (tr > cr) { tr -= 3; if (tr < cr) tr = cr; }
							else { tr += 3; if (tr > cr) tr = cr; }

							if (tg > cg) { tg -= 3; if (tg < cg) tg = cg; }
							else { tg += 3; if (tg > cg) tg = cg; }

							if (tb > cb) { tb -= 3; if (tb < cb) tb = cb; }
							else { tb += 3; if (tb > cb) tb = cb; }

							pHi[j] = static_cast<u16>((tb << 10) | (tg << 5) | tr | (target & 0x8000));
						}
					}

					_LoadImage();
				}

				for (i = 0; i < this->field_454; i++)
				{
					u16 *pLo = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_33C[i]) + 4);
					u16 *pHi = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_33C[i]) + 0x204);

					for (i32 j = 0; j < 256; j++)
					{
						u16 target = pHi[j];

						if (target & 0x7FFF)
						{
							u16 current = pLo[j];

							i32 tr = target & 0x1F;
							i32 tg = (target >> 5) & 0x1F;
							i32 tb = (target >> 10) & 0x1F;

							i32 cr = current & 0x1F;
							i32 cg = (current >> 5) & 0x1F;
							i32 cb = (current >> 10) & 0x1F;

							if (tr > cr) { tr -= 3; if (tr < cr) tr = cr; }
							else { tr += 3; if (tr > cr) tr = cr; }

							if (tg > cg) { tg -= 3; if (tg < cg) tg = cg; }
							else { tg += 3; if (tg > cg) tg = cg; }

							if (tb > cb) { tb -= 3; if (tb < cb) tb = cb; }
							else { tb += 3; if (tb > cb) tb = cb; }

							pHi[j] = static_cast<u16>((tb << 10) | (tg << 5) | tr | (target & 0x8000));
						}
					}

					_LoadImage();
				}
			}

			break;
		}

		case 1:
		{
			if (this->mAge > 0xB)
			{
				this->mAge = 0;
				this->field_45B = 3;
				return;
			}
			this->mAge++;

			if (this->field_45C)
			{
				i32 r = gPaletteFadeRGB[0] >> 3;
				i32 g = gPaletteFadeRGB[1] >> 3;
				i32 b = gPaletteFadeRGB[2] >> 3;

				i32 tr = this->field_45D;
				i32 tg = this->field_45E;
				i32 tb = this->field_45F;

				if (r > tr) { r -= 3; if (r < tr) r = tr; }
				else { r += 3; if (r > tr) r = tr; }

				if (g > tg) { g -= 3; if (g < tg) g = tg; }
				else { g += 3; if (g > tg) g = tg; }

				if (b > tb) { b -= 3; if (b < tb) b = tb; }
				else { b += 3; if (b > tb) b = tb; }

				gPaletteFadeRGB[0] = static_cast<u8>(r << 3);
				gPaletteFadeRGB[1] = static_cast<u8>(g << 3);
				gPaletteFadeRGB[2] = static_cast<u8>(b << 3);

				gPaletteFadeRGB2[0] = static_cast<u8>(r << 3);
				gPaletteFadeRGB2[1] = static_cast<u8>(g << 3);
				gPaletteFadeRGB2[2] = static_cast<u8>(b << 3);
			}

			{
				i32 i;

				for (i = 0; i < this->field_450; i++)
				{
					u16 *pLo = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_3C[i]) + 4);
					u16 *pHi = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_3C[i]) + 0x24);

					for (i32 j = 0; j < 16; j++)
					{
						u16 target = pHi[j];

						if (target & 0x7FFF)
						{
							u16 current = pLo[j];

							i32 tr = target & 0x1F;
							i32 tg = (target >> 5) & 0x1F;
							i32 tb = (target >> 10) & 0x1F;

							i32 cr = current & 0x1F;
							i32 cg = (current >> 5) & 0x1F;
							i32 cb = (current >> 10) & 0x1F;

							if (tr > cr) { tr -= 3; if (tr < cr) tr = cr; }
							else { tr += 3; if (tr > cr) tr = cr; }

							if (tg > cg) { tg -= 3; if (tg < cg) tg = cg; }
							else { tg += 3; if (tg > cg) tg = cg; }

							if (tb > cb) { tb -= 3; if (tb < cb) tb = cb; }
							else { tb += 3; if (tb > cb) tb = cb; }

							pHi[j] = static_cast<u16>((tb << 10) | (tg << 5) | tr | (target & 0x8000));
						}
					}

					_LoadImage();
				}

				for (i = 0; i < this->field_454; i++)
				{
					u16 *pLo = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_33C[i]) + 4);
					u16 *pHi = reinterpret_cast<u16*>(reinterpret_cast<u8*>(this->field_33C[i]) + 0x204);

					for (i32 j = 0; j < 256; j++)
					{
						u16 target = pHi[j];

						if (target & 0x7FFF)
						{
							u16 current = pLo[j];

							i32 tr = target & 0x1F;
							i32 tg = (target >> 5) & 0x1F;
							i32 tb = (target >> 10) & 0x1F;

							i32 cr = current & 0x1F;
							i32 cg = (current >> 5) & 0x1F;
							i32 cb = (current >> 10) & 0x1F;

							if (tr > cr) { tr -= 3; if (tr < cr) tr = cr; }
							else { tr += 3; if (tr > cr) tr = cr; }

							if (tg > cg) { tg -= 3; if (tg < cg) tg = cg; }
							else { tg += 3; if (tg > cg) tg = cg; }

							if (tb > cb) { tb -= 3; if (tb < cb) tb = cb; }
							else { tb += 3; if (tb > cb) tb = cb; }

							pHi[j] = static_cast<u16>((tb << 10) | (tg << 5) | tr | (target & 0x8000));
						}
					}

					_LoadImage();
				}
			}

			break;
		}

		case 3:
			this->Die();
			break;

		default:
			break;
	}
}

// @Ok
// @Matching
CFadePalettes::~CFadePalettes(void)
{
	if (this->field_45B != 3)
	{
		for (i32 i = 0; i < this->field_450; i++)
		{
			_LoadImage();
		}

		for (i32 j = 0; j < this->field_454; j++)
		{
			_LoadImage();
		}
	}

	DrawSync();

	for (i32 k = 0; k < this->field_450; k++)
	{
		Mem_Delete(this->field_3C[k]);
	}

	for (i32 l = 0; l < this->field_454; l++)
	{
		Mem_Delete(this->field_33C[l]);
	}

	if (this->field_45C)
	{
		gPaletteFadeRGB[0] = this->field_45D;
		gPaletteFadeRGB[1] = this->field_45E;
		gPaletteFadeRGB[2] = this->field_45F;

		gPaletteFadeRGB2[0] = this->field_45D;
		gPaletteFadeRGB2[1] = this->field_45E;
		gPaletteFadeRGB2[2] = this->field_45F;
	}

	gFadePalettesActive = 0;
}


// @Ok
// @Matching
void Mysterio_FadePalettesUp(const u32* a1, u32*)
{
	void* v2 = Mem_RecoverPointer(&gMystHandle);
	print_if_false(v2 == 0, "Tried to do two fade ups");

	gMystHandle = Mem_MakeHandle(new CFadePalettes(a1[0], a1[1], a1[2]));
}

// @Ok
// @Matching
void Mysterio_FadePalettesDown(const u32*, u32*)
{
	CFadePalettes *pFade = static_cast<CFadePalettes*>(Mem_RecoverPointer(&gMystHandle));
	if (pFade)
		pFade->FadeDown();
}

// @Ok
// @Matching
void Mysterio_RelocatableModuleClear(void)
{
	CItem *pSearch = BaddyList;

	while (pSearch)
	{
		CItem *pNext = pSearch->mNextItem;

		if (pSearch->mType == 311)
			delete pSearch;

		pSearch = pNext;
	}

	gSuperItemRelated = 1;
}

// @Ok
// @Matching
void Mysterio_RelocatableModuleInit(reloc_mod *pMod)
{
	pMod->mClearFunc = Mysterio_RelocatableModuleClear;

	pMod->field_C[0] = Mysterio_CreateMysterio;
	pMod->field_C[1] = Mysterio_FadePalettesUp;
	pMod->field_C[2] = Mysterio_FadePalettesDown;

	gSuperItemRelated = 0;
	gFloatSuperRelated = 1.0f;
}

// @SMALLTODO
CDamagedSoftSpotEffect::CDamagedSoftSpotEffect(CBody*, i32)
{
	printf("CDamagedSoftSpotEffect::CDamagedSoftSpotEffect(CBody*, i32)");
}

// @NotOk
// @FIXME field_48 type
CDamagedSoftSpotEffect::~CDamagedSoftSpotEffect(void)
{
	delete reinterpret_cast<CClass*>(this->field_48);
}

// @MEDIUMTODO
CAngrySpark::CAngrySpark(CVector*)
{
	printf("CAngrySpark::CAngrySpark(CVector*)");
}

// @Ok
CAngrySpark::~CAngrySpark(void)
{
}

// @Ok
// not matching but good enough
i32 CMysterio::MonitorAttack(
		i32 a2,
		VECTOR* a3,
		i32 a4)
{
	SHook hook;


	hook.Part.vy = a3->vy;
	i32 res = 0;
	hook.Part.vz = a3->vz;
	hook.Part.vx = a3->vx;

	if (this->field_388 != gAttackRelated - 1)
	{
		M3dUtils_GetDynamicHookPosition(
				reinterpret_cast<VECTOR*>(&this->field_37C),
				this,
				&hook);
	}
	else
	{
		CVector v13;
		CVector v14;

		v13.vx = 0;
		v13.vy = 0;
		v13.vz = 0;

		v14.vx = 0;
		v14.vy = 0;
		v14.vz = 0;

		M3dUtils_GetDynamicHookPosition(
				reinterpret_cast<VECTOR*>(&v13),
				this,
				&hook);

		if (M3dColij_LineToSphere(
					&this->field_37C,
					&v13,
					&v14,
					MechList,
					0,
					((MechList->mRMinor + a4) << 12) / MechList->mRMinor))
		{
			res = 1;
		}

		this->field_37C = v13;
	}

	this->field_388 = gAttackRelated;

	return res;
}

extern CBaddy* BaddyList;

// @Ok
CMysterio::~CMysterio(void)
{
	this->DeleteFrom(reinterpret_cast<CBody**>(&BaddyList));
	gBossRelated = 0;
	Panel_DestroyHealthBar();

	if (this->field_3B8)
		SFX_Stop(this->field_3B8);

	this->field_3B8 = 0;

	if (this->field_324)
		delete this->field_324;


	CItem* first = reinterpret_cast<CItem*>(Mem_RecoverPointer(&this->field_360));
	if (first)
		delete first;

	CItem* second = reinterpret_cast<CItem*>(Mem_RecoverPointer(&this->field_368));
	if (second)
		delete second;



}

// @MEDIUMTODO
CMysterio::CMysterio(int*, int)
{
}

// @NotOk
// Globals
CMysterio::CMysterio(void)
{
	this->field_37C.vx = 0;
	this->field_37C.vy = 0;
	this->field_37C.vz = 0;

	this->InitItem("mysterio");

	this->mFlags |= 0x480;
	this->mpLight = &M3d_MysterioLight;
}

// @Ok
void Mysterio_CreateMysterio(const unsigned int *stack, unsigned int *result)
{
	int* v2 = reinterpret_cast<int*>(*stack);
	int v3 = static_cast<int>(stack[1]);

	if (v2)
	{
		*result = reinterpret_cast<unsigned int>(new CMysterio(v2, v3));
	}
	else
	{
		*result = reinterpret_cast<unsigned int>(new CMysterio());
	}
}

// @Ok
void INLINE CMysterioLaser::SetDamage(int damage)
{
	this->field_4C = damage;
}

// @Ok
void INLINE CGoldFish::AngryMode(void)
{
	this->field_F8 = 1;
}

// @Ok
void INLINE CGoldFish::NormalMode(void)
{
	this->field_F8 = 0;
}

void validate_CMystFoot(void){
	VALIDATE_SIZE(CMystFoot, 0x324);
}

// @Ok
void INLINE CMysterioHeadCircle::NormalMode(void)
{
	if (this->field_8C)
		this->field_88 >>= 1;
	this->field_8C = 0;
}

// @Ok
void INLINE CMysterioHeadCircle::AngryMode(void)
{
	if (!this->field_8C)
		this->field_88 <<= 1;
	this->field_8C = 1;
}

// @Ok
u8 INLINE CMysterio::MystRedbook_XAPlayPos(
		i32 a2,
		i32 a3,
		CVector *a4,
		i32 a5)
{
	u8 res = Redbook_XAPlay(a2, a3, a5);
	if (res)
	{
		this->field_3A0 = 0;
		this->field_39C = 480;
	}

	return res;
}

// @Ok
i32 INLINE CMysterio::PlayAndAttachXAPlease(
		i32 a2,
		i32 a3,
		CBody* pBody,
		i32 a5)
{
	if (this->MystRedbook_XAPlayPos(a2, a3, &pBody->mPos, a5))
	{
		pBody->AttachXA(a2, a3);
	}
	
	return 0;
}


// @Ok
// @Matching
void INLINE CMysterio::ShakePad(void)
{
	if (gSaveGame.field_7B)
	{
		if (Pad_GetActuatorTime(0, 0) <= 2)
			Pad_ActuatorOn(0, 6, 0, 1);
		if (Pad_GetActuatorTime(0, 1) <= 2)
			Pad_ActuatorOn(0, 10, 1, 0xC8);
	}
}

extern CCamera *CameraList;

// @Ok
// @Validate: when inlined
i32 INLINE CMysterio::CheckforCameraShake(i32 a2)
{
	if (this->field_218 & 8 || this->field_218 < a2)
		return 0;

	CameraList->Shake(this->mPos, CAMERASHAKE_BIG);

	this->ShakePad();

	this->field_218 |= 8;
	return 1;
}

// @Ok
void INLINE CMysterio::EnterP2(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->RunAnim(10, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 1;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate!");
			break;
	}
}

extern i32 DifficultyLevel;

// @NotOk
// @Validate: when inlined
INLINE i32 CMysterio::GetAttackRotSpeed(void)
{
	if (!DifficultyLevel)
	{
		if (this->field_34C && this->field_350)
		{
			return 2;
		}

		return 5;
	}
	else
	{
		if (DifficultyLevel == 1)
		{
			if (this->field_34C && this->field_350)
			{
				return 3;
			}

			return 9;
		}
		else if (this->field_34C && this->field_350)
		{
			return 12;
		}

		return 5;

	}
}

// @Ok
void CMysterio::SummonAttack(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			this->RunAnim(10, 0, -1);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->field_218 >= 58)
			{
				Trig_SendPulse(reinterpret_cast<u16*>(
							Trig_GetLinksPointer(this->mNode)));
				this->dumbAssPad++;
			}
			break;
		case 2:
			if (this->mAnimFinished)
			{
				this->field_31C.bothFlags = 1;
				this->dumbAssPad = 0;
			}
			break;
		default:
			print_if_false(0, "Unknown substate.");
			break;
	}
}

// @NotOk
// globals
void CMysterio::LookMenacing(void)
{
	switch (this->dumbAssPad)
	{
		case 0:
			this->Neutralize();
			new CAIProc_LookAt(this, MechList, 0, 1, 60, 341);
			this->dumbAssPad++;
			break;
		case 1:
			if (this->mAnimFinished)
			{
				this->RunAnim(8, 0, -1);
			}
			break;
		default:
			print_if_false(0, "Unknown substate.");
			break;
	}
}

// @Ok
void CMysterio::RotateToOptimalAttackAngle(
		i32 a2,
		i32 a3)
{
	CSVector v7;

	v7.vx = 0;
	v7.vy = 0;
	v7.vz = 0;

	Utils_CalcAim(&v7, &this->mPos, &MechList->mPos);

	i32 v4 = v7.vy - this->mAngles.vy;

	if (v4 < -2056)
	{
		v4 += 4096;
	}
	else if (v4 > 2056)
	{
		v4 -= 4096;
	}

	v4 -= a2;
	if (v4 < -2056)
	{
		v4 += 4096;
	}
	else if (v4 > 2056)
	{
		v4 -= 4096;
	}

	new CAIProc_LookAt(
			this,
			v4 + this->mAngles.vy,
			1,
			a3,
			200);
}

void validate_CMysterio(void){
	VALIDATE_SIZE(CMysterio, 0x3D0);

	VALIDATE(CMysterio, field_324, 0x324);
	VALIDATE(CMysterio, field_32C, 0x32C);

	VALIDATE(CMysterio, field_34C, 0x34C);
	VALIDATE(CMysterio, field_350, 0x350);

	VALIDATE(CMysterio, field_358, 0x358);

	VALIDATE(CMysterio, field_360, 0x360);
	VALIDATE(CMysterio, field_368, 0x368);

	VALIDATE(CMysterio, field_374, 0x374);

	VALIDATE(CMysterio, field_378, 0x378);
	VALIDATE(CMysterio, field_37C, 0x37C);

	VALIDATE(CMysterio, field_388, 0x388);

	VALIDATE(CMysterio, field_38C, 0x38C);
	VALIDATE(CMysterio, field_398, 0x398);

	VALIDATE(CMysterio, field_39C, 0x39C);
	VALIDATE(CMysterio, field_3A0, 0x3A0);

	VALIDATE(CMysterio, field_3A8, 0x3A8);

	VALIDATE(CMysterio, field_3B8, 0x3B8);
}

void validate_CSoftSpot(void){
	VALIDATE_SIZE(CSoftSpot, 0x338);

	VALIDATE(CSoftSpot, field_324, 0x324);
	VALIDATE(CSoftSpot, field_328, 0x328);
	VALIDATE(CSoftSpot, field_32c, 0x32c);
	VALIDATE(CSoftSpot, field_330, 0x330);
	VALIDATE(CSoftSpot, field_334, 0x334);
}

void validate_CMysterioLaser(void)
{
	VALIDATE_SIZE(CMysterioLaser, 0x64);

	VALIDATE(CMysterioLaser, field_4C, 0x4C);
}

void validate_CGoldFish(void)
{
	VALIDATE_SIZE(CGoldFish, 0x110);

	VALIDATE(CGoldFish, field_F8, 0xF8);
}

void validate_CMysterioHeadCircle(void)
{
	VALIDATE_SIZE(CMysterioHeadCircle, 0x90);

	VALIDATE(CMysterioHeadCircle, field_88, 0x88);
	VALIDATE(CMysterioHeadCircle, field_8C, 0x8C);
}

void validate_CFadePalettes(void)
{
	VALIDATE_SIZE(CFadePalettes, 0x460);

	VALIDATE(CFadePalettes, field_3C, 0x3C);
	VALIDATE(CFadePalettes, field_33C, 0x33C);
	VALIDATE(CFadePalettes, field_450, 0x450);
	VALIDATE(CFadePalettes, field_454, 0x454);

	VALIDATE(CFadePalettes, field_458, 0x458);
	VALIDATE(CFadePalettes, field_459, 0x459);
	VALIDATE(CFadePalettes, field_45A, 0x45A);

	VALIDATE(CFadePalettes, field_45B, 0x45B);

	VALIDATE(CFadePalettes, field_45C, 0x45C);
	VALIDATE(CFadePalettes, field_45D, 0x45D);
	VALIDATE(CFadePalettes, field_45E, 0x45E);
	VALIDATE(CFadePalettes, field_45F, 0x45F);
}

void validate_CAngrySpark(void)
{
	VALIDATE_SIZE(CAngrySpark, 0x84);
}

void validate_CDamagedSoftSpotEffect(void)
{
	VALIDATE_SIZE(CDamagedSoftSpotEffect, 0x4C);

	VALIDATE(CDamagedSoftSpotEffect, field_48, 0x48);
}
