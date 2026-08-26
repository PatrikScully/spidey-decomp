#pragma once


#ifndef EFFECTS_H
#define EFFECTS_H

#include "ob.h"
#include "m3dcolij.h"
#include "export.h"

class CBouncingRock : public CFlatBit
{
	public:
		EXPORT CBouncingRock(CVector*, i32, u32);
		EXPORT virtual ~CBouncingRock(void);

		EXPORT virtual void Move(void);

		i32 field_68;
		i32 field_6C;
};

class CChunkSmoke : public CFlatBit
{
	public:
		EXPORT CChunkSmoke(CVector*, CVector*, i32);
		EXPORT virtual ~CChunkSmoke(void);

		EXPORT virtual void Move(void);

		CVector field_68;
		i32 field_74;
		i32 field_78;
		i32 field_7C;
};

class CFootprint : public CQuadBit
{
	public:
		EXPORT CFootprint(CVector*, i32);
		EXPORT virtual ~CFootprint(void);

		EXPORT virtual void Move(void);

		i32 field_84;
};

class CRhinoWallImpact : public CQuadBit
{
	public:
		EXPORT CRhinoWallImpact(SLineInfo*);
		EXPORT virtual ~CRhinoWallImpact(void);

		EXPORT virtual void Move(void);

		PADDING(4);
};

class CElectrify : public CSimpleTexturedRibbon
{
	public:
		EXPORT CElectrify(CSuper*, int a2);

		EXPORT void ChooseRandomPositions(i32, i32);

		// @FIXME guessed layout, entries are 8 bytes, only the field at +6
		// (u16) is written by the constructor
		void *field_4C;

		i32 field_50;

		// array of field_50 CVector entries
		CVector *field_54;

		i32 field_58;

		// SHandle wrapping the owning CSuper
		void *field_5C;
		u32 field_60;
};

struct SSkinGooSource
{
	u32 field_0;

	u32 field_4;
	u32 field_8;
};

struct SSkinGooSource2
{
};

struct SSkinGooParams
{
};

class CSkinGoo : public CQuadBit
{
	public:
		EXPORT CSkinGoo(CSuper*, SSkinGooSource*, i32, SSkinGooParams*);
		EXPORT CSkinGoo(CSuper*, SSkinGooSource2*, i32, SSkinGooParams*);

		u8 fullPad[0x54];
};

class CElectro : public CSimpleTexturedRibbon
{
	public:
		EXPORT CElectro(void);
		EXPORT virtual ~CElectro(void);

		EXPORT void Setup(i32, i32, u32*, u8, u8, u8, u16, u16);

		i32 field_4C;

		// @FIXME
		void *field_50;
		void *field_54;

		CVector field_58;
};

class CElectroLine : public CElectro
{
	public:
		EXPORT CElectroLine(u16, u16, u16, u8, u8 ,u8, i32, i32, i32, i32, i32, u32*);
		EXPORT virtual ~CElectroLine(void);

		PADDING(8);
};

class CVertexWobble : public CBit
{
	public:
		EXPORT CVertexWobble(u32, u32, u32, u8*, i32, i32, i32, i32);
		EXPORT virtual void Move(void);

		// SHandle wrapping G_PSXREGION[Region].pPSX
		void *field_3C;
		u32 field_40;

		PADDING(8);

		// SModel* for the chosen model piece (G_PSXREGION[Region].ppModels[a2])
		void *field_4C;

		// number of wobbling vertices
		i32 field_50;

		// array of field_50 SVertexWobbleEntry (22 bytes each), see effects.cpp
		void *field_54;

		// average/centre position of the wobbling vertices
		CSVector field_58;

		PADDING(2);
};

void validate_CVertexWobble(void);
void validate_CElectrify(void);
void validate_CSkinGoo(void);
void validate_SSkinGooSource(void);
void validate_SSkinGooSource2(void);
void validate_SSkinGooParams(void);
void validate_CRhinoWallImpact(void);
void validate_CFootprint(void);
void validate_CChunkSmoke(void);
void validate_CBouncingRock(void);
void validate_CElectro(void);
void validate_CElectroLine(void);

EXPORT void Effects_Electrify(CSuper*);
EXPORT void Effects_UnElectrify(CSuper*);

#endif
