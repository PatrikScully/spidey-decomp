#pragma once

#ifndef PS2M3D_H
#define PS2M3D_H

#include "export.h"
#include "ob.h"
#include "camera.h"
#include "dcmodel.h"

class matrix4x4
{
	
	public:
		vector4d field_0[4];

		EXPORT matrix4x4(
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32,
				f32);

		EXPORT vector4d& operator[](i32);
};

EXPORT void M3d_BuildTransform(CSuper*);
EXPORT void M3d_Render(void*);
EXPORT void DCModel_RenderModel(SModel const *,DCModelData *,matrix4x4 const *,void const *);
EXPORT void DC_PSXModel_RenderModel(SModel const *,matrix4x4 const *,void const *,DCModelData *);
EXPORT void M3d_PreprocessPulsingColours(i32);
EXPORT void M3d_PreprocessWibblyTextures(i32);
EXPORT void M3d_RenderBackground(void *);
EXPORT void M3d_RenderCleanup(void);
EXPORT void M3d_RenderSetup(SCamera *,SViewport *,u32 *);
EXPORT void RenderSuperItem(CItem *,bool);

EXPORT extern i32 gWideScreen;
EXPORT extern char gRenderBuf[4];

// current colour table pointer, set by M3d_PreprocessPulsingColours from
// PSXRegion[Region].pColourTable, read by the DC/PSX model renderers.
// named pColourTable in the PS2 source (m3d.mik).
EXPORT extern u32* pColourTable;
//#define G_COLOUR_TABLE (pColourTable)
#define G_COLOUR_TABLE (*reinterpret_cast<u32**>(0x0064F5D0))

void validate_matrix4x4(void);

#endif
