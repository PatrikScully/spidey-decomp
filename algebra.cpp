#include "algebra.h"

// 0x56E668: active 4x4 transform matrix, 16 floats, column-major.
// The Display*List projection code in bit.cpp copies the current
// camera matrix into this before projecting.
static f32 * const gGfxMatrix = (f32 *)0x56E668;

// @Ok
// @Matching
SFloat3* SFloat3::Copy(const SFloat3* src)
{
	this->x = src->x;
	this->y = src->y;
	this->z = src->z;
	return this;
}

// @NotOk
f32* Algebra_Transform4(f32* out, f32* in)
{
	f32 o3 = gGfxMatrix[15] * in[3] + gGfxMatrix[3] * in[0] + gGfxMatrix[7] * in[1] + gGfxMatrix[11] * in[2];
	f32 o2 = gGfxMatrix[14] * in[3] + gGfxMatrix[2] * in[0] + gGfxMatrix[6] * in[1] + gGfxMatrix[10] * in[2];
	f32 o1 = gGfxMatrix[13] * in[3] + gGfxMatrix[1] * in[0] + gGfxMatrix[5] * in[1] + gGfxMatrix[9] * in[2];
	f32 o0 = gGfxMatrix[12] * in[3] + gGfxMatrix[4] * in[1] + gGfxMatrix[0] * in[0] + gGfxMatrix[8] * in[2];
	out[1] = o1;
	out[3] = o3;
	out[0] = o0;
	out[2] = o2;
	return out;
}
