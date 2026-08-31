#include "dcmodel.h"
#include "PCTex.h"

#include "validate.h"

#include "non_win32.h"
#include <cmath>

EXPORT f32 gPreComputedColorRelated = -1.0f;
EXPORT u8 gConvertedColors[256];

EXPORT DCSkaterModel gSkaterModels[2];
EXPORT DCSkaterModel gGlobalSkaterModel;

// @Ok
// @Matching
DCSkaterModel::DCSkaterModel(void)
{
	this->field_28.pObject = 0;
}

// @Ok
// @Matching
void DCClearSkater(void)
{
	if (!gGlobalSkaterModel.field_1C)
	{
		gSkaterModels[0].ClearSkaterModel();
		gSkaterModels[1].ClearSkaterModel();
	}
}

// @Ok
// @Matching
INLINE DCKeyFrame::~DCKeyFrame(void)
{
	delete this->pNext;
}

// @Ok
// @Matching
DCMaterial::~DCMaterial(void)
{
	delete this->field_10;
	delete this->field_34;

	if (!this->field_3F && CheckValidTexture(this->field_38))
		PCTex_ReleaseTexture(this->field_38, true);
}

// @BIGTODO
// @Note: checked with IDA (0x431430). Original is about 4.4 KB of code, ~200 locals in
// the Hex-Rays output. It welds duplicate verts/normals from an SModel into flat vertex
// buffers, builds UV coordinates from texture pack info, builds a "vert in faces" table,
// and sets a bunch of flag bits (transparency, stitched normals, and a special-vertex
// scan against a4). This is much bigger than a MEDIUMTODO, retagged accordingly.
// DCModelData does not exist anywhere in the repo (only forward-declared in dcmodel.h
// alongside SModel). Confirmed with an earlier session today: this also blocks
// ps2m3d.cpp and web.cpp. Do not guess fields from this one function alone: a1 (the
// DCModelData*) is written at word offsets 0,1,2,3,4,5,6,7,8 (bytes 0x0,0x4,0x8,0xC,
//0x10,0x14,0x18,0x1C,0x20) in this function alone, and a2 (SModel*) is read at byte
// offsets 4,8,0xC,and a big run starting at 14 words in, so a real struct needs
// cross-referencing against DCModel_RenderModel (0x476D00) and other DCObject/DCModel
// users before any offset gets a name. Left as a stub, not attempted further this
// session.
void DCModel_CreateFromSModel(
		DCModelData *pDcModel,
		SModel *,i32,i32 *,bool,i32)
{
    printf("DCModel_CreateFromSModel(DCModelData *,SModel *,i32,i32 *,bool,i32)");
}

// @Ok
// @Matching
INLINE DCObjectList::~DCObjectList(void)
{
	delete this->pObject;
}

// @Ok
// @Matching
DCObject::~DCObject(void)
{
	delete this->field_4;

	delete this->field_E4.pObject;
	this->field_E4.pObject = 0;

	delete this->field_E8;
	this->field_E8 = 0;

	delete this->field_D0;

	delete this->field_128;
	delete[] this->field_134;
	delete this->field_12C;

	this->field_E0 = 0;
}

// @Ok
// @Matching
INLINE void DCSkaterModel::ClearSkaterModel(void)
{
	if ( this->field_1C )
	{
		this->field_0 = 0;
		this->field_4 = 0;
		this->field_8 = 0;

		this->field_18 = 0;
		this->field_1C = 0;

		delete this->field_28.pObject;
		this->field_28.pObject = 0;

		delete[] this->field_24;

		this->field_24 = 0;
		this->field_20 = 0;
	}
}

// @Ok
// @Note: verified against IDA decompile of 0x432830. The old code only freed field_24.
// It was missing the field_28.pObject cleanup entirely (the original calls
// DCObject::~DCObject on it, then operator delete), same pattern as ClearSkaterModel.
DCSkaterModel::~DCSkaterModel(void)
{
	delete[] this->field_24;
	delete this->field_28.pObject;
}

// @Ok
// @Matching
DCStrip::~DCStrip(void)
{
	delete this->field_8;
}

// @Ok
// @Matching
void PreComputeConvertedColors(f32 a1)
{
	for (i32 i = 0;
			i < 256;
			i++)
	{
		f32 v4 = (f32)i / 255.0f;
		f32 v5 = pow(v4, a1);
		if (v5 > 1.0)
			v5 = 1.0;
		gConvertedColors[i] = (v5 * 255.0f);
	}

	gPreComputedColorRelated = a1;
}

void validate_DCSkaterModel(void)
{
	VALIDATE_SIZE(DCSkaterModel, 0x2C);

	VALIDATE(DCSkaterModel, field_0, 0x0);

	VALIDATE(DCSkaterModel, field_4, 0x4);

	VALIDATE(DCSkaterModel, field_8, 0x8);

	VALIDATE(DCSkaterModel, field_18, 0x18);
	VALIDATE(DCSkaterModel, field_1C, 0x1C);

	VALIDATE(DCSkaterModel, field_20, 0x20);
	VALIDATE(DCSkaterModel, field_24, 0x24);
	VALIDATE(DCSkaterModel, field_28, 0x28);
}

void validate_DCMaterial(void)
{
	VALIDATE_SIZE(DCMaterial, 0x40);

	VALIDATE(DCMaterial, field_10, 0x10);

	VALIDATE(DCMaterial, field_34, 0x34);
	VALIDATE(DCMaterial, field_38, 0x38);

	VALIDATE(DCMaterial, field_3F, 0x3F);
}

void validate_DCObject(void)
{
	VALIDATE_SIZE(DCObject, 0x138);

	VALIDATE(DCObject, field_4, 0x4);

	VALIDATE(DCObject, field_D0, 0xD0);

	VALIDATE(DCObject, field_E0, 0xE0);
	VALIDATE(DCObject, field_E4, 0xE4);
	VALIDATE(DCObject, field_E8, 0xE8);

	VALIDATE(DCObject, field_128, 0x128);
	VALIDATE(DCObject, field_12C, 0x12C);

	VALIDATE(DCObject, field_134, 0x134);
}

void validate_DCStrip(void)
{
	VALIDATE_SIZE(DCStrip, 0xC);

	VALIDATE(DCStrip, field_8, 0x8);
}

void validate_DCObjectList(void)
{
	VALIDATE_SIZE(DCObjectList, 0x4);

	VALIDATE(DCObjectList, pObject, 0x0);
}

void validate_DCKeyFrame(void)
{
	VALIDATE_SIZE(DCKeyFrame, 0x30);

	VALIDATE(DCKeyFrame, pNext, 0x2C);
}
