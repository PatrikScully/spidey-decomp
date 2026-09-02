// vector.cpp: implementation of the CVector class.
//
//////////////////////////////////////////////////////////////////////

#include "vector.h"
#include <cstdio>
#include "validate.h"
#include "ps2funcs.h"
#include "my_patch.h"

/*
CVector::CVector(void)
{
}
*/

// @Ok
// @Matching
i32 CVector::LengthSquared(void)
{
	CVector tmp;

	tmp.vx = this->vx >> 12;
	tmp.vy = this->vy >> 12;
	tmp.vz = this->vz >> 12;

	gte_ldlvl(reinterpret_cast<VECTOR*>(&tmp));
	gte_sqr0();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&tmp));

	return tmp.vx + tmp.vy + tmp.vz;
}

// @Ok
// @Matching
i32 CVector::operator!=(const CVector& other)
{
	return this->vx != other.vx || this->vy != other.vy || this->vz != other.vz;
}

// @Ok
CVector operator<<(const CVector& lhs, const i32& other){

	CVector res;

	res.vx = lhs.vx << other;
	res.vy = lhs.vy << other;
	res.vz = lhs.vz << other;

	return res;
}

// @Ok
void CSVector::KillSmall(void){

    if (this->vx >= -1 && this->vx <= 1)
    {
        this->vx = 0;
    }

    if (this->vy >= -1 && this->vy <= 1)
    {
        this->vy = 0;
    }

    if (this->vz >= -1 && this->vz <= 1)
    {
        this->vz = 0;
    }
}

// @Ok
void CSVector::Mask(void)
{
	this->vx &= 0xFFF;
	this->vy &= 0xFFF;
	this->vz &= 0xFFF;
}

// @Ok
CSVector* CSVector::operator+=(const CSVector& other){
	this->vx += other.vx;
	this->vy += other.vy;
	this->vz += other.vz;
	return this;
}

// @Ok
// @Matching
CSVector* CSVector::operator/=(const int& other){
	this->vx /= other;
	this->vy /= other;
	this->vz /= other;
	return this;
}

// @Ok
// @Matching
CSVector* CSVector::operator%=(const CFriction& other){
	this->vx -= this->vx >> other.vx;
	this->vy -= this->vy >> other.vy;
	this->vz -= this->vz >> other.vz;
	return this;
}


// @Ok
void CVector::KillSmall(){

    if (this->vx >= -2048 && this->vx <= 2048)
    {
        this->vx = 0;
    }

    if (this->vy >= -2048 && this->vy <= 2048)
    {
        this->vy = 0;
    }

    if (this->vz >= -2048 && this->vz <= 2048)
    {
        this->vz = 0;
    }
}

// @Ok
CVector* CVector::operator-=(const CVector& other){
	this->vx -= other.vx;
	this->vy -= other.vy;
	this->vz -= other.vz;
	return this;
}

// @Ok
CVector* CVector::operator>>=(const int& other){
	this->vx >>= other;
	this->vy >>= other;
	this->vz >>= other;
	return this;
}

// @Ok
CVector* CVector::operator<<=(const int& other){
	this->vx <<= other;
	this->vy <<= other;
	this->vz <<= other;
	return this;
}

// @Ok
CVector* CVector::operator*=(const int& other){
	this->vx *= other;
	this->vy *= other;
	this->vz *= other;
	return this;
}

// @Ok
CVector* CVector::operator/=(const int& other){
	this->vx /= other;
	this->vy /= other;
	this->vz /= other;
	return this;
}

// @Ok
CVector* CVector::operator+=(const CVector& other){
	this->vx += other.vx;
	this->vy += other.vy;
	this->vz += other.vz;
	return this;
}

// @Ok
CVector* CVector::operator%=(const CFriction& other){
	this->vx -= this->vx >> other.vx;
	this->vy -= this->vy >> other.vy;
	this->vz -= this->vz >> other.vz;
	return this;
}

// @Ok
CVector operator/(const CVector& lhs, const int& other){

	CVector res;

	res.vx = lhs.vx / other;
	res.vy = lhs.vy / other;
	res.vz = lhs.vz / other;

	return res;
}

// @Ok
CVector operator*(const CVector& lhs, const int& other)
{
	CVector res;

	res.vx = lhs.vx * other;
	res.vy = lhs.vy * other;
	res.vz = lhs.vz * other;

	return res;
}

// @Ok
CVector operator*(const int& lhs, const CVector& other)
{
	CVector res;

	res.vx = lhs * other.vx;
	res.vy = lhs * other.vy;
	res.vz = lhs * other.vz;

	return res;
}

// @Ok
CVector operator*(const CVector& lhs, const CVector& other){

	CVector res;

	res.vx = lhs.vx * other.vx;
	res.vy = lhs.vx * other.vy;
	res.vz = lhs.vx * other.vz;

	return res;
}

// fix 2026-08-31: vy/vz were both reading lhs.vx instead of lhs.vy/lhs.vz
// (component-wise add came out wrong for every caller). Confirmed real bug
// in our source, not a reproduction of original behavior: disasm at
// 0x4E7720 does plain a1[i] = a2[i] + a3[i] for i in 0..2, standard
// component-wise add. Introduced in upstream commit 840ff1bf.
// @Ok
CVector operator+(const CVector& lhs, const CVector& other){

	CVector res;

	res.vx = lhs.vx + other.vx;
	res.vy = lhs.vy + other.vy;
	res.vz = lhs.vz + other.vz;

	return res;
}

// @Ok
// moved out of vector.h 2026-08-27: it was wrongly marked INLINE, so MSVC
// always folded it into callers while the original binary calls it as a
// real out-of-line function (confirmed via disassembly of multiple callers
// across bit.cpp/baddy.cpp/shatter.cpp).
CVector operator-(const CVector& lhs, const CVector& other)
{
	CVector res;

	res.vx = lhs.vx - other.vx;
	res.vy = lhs.vy - other.vy;
	res.vz = lhs.vz - other.vz;

	return res;
}

// @Ok
// moved out of vector.h 2026-08-27: same wrongly-INLINE bug as operator-
// above (confirmed via disassembly of shatter.cpp callers).
CVector operator>>(const CVector& lhs, const int& other)
{
	CVector res;

	res.vx = lhs.vx >> other;
	res.vy = lhs.vy >> other;
	res.vz = lhs.vz >> other;

	return res;
}


// @Ok
// @Test
int CVector::Length(void)
{
	CVector v4;

	v4.vx = this->vx >> 12;
	v4.vy = this->vy >> 12;
	v4.vz = this->vz >> 12;

	gte_ldlvl(reinterpret_cast<VECTOR*>(&v4));
	gte_sqr0();
	gte_stlvnl(reinterpret_cast<VECTOR*>(&v4));

	return M3dMaths_SquareRoot0(v4.vx + v4.vy + v4.vz);
}

// @Bogus
void CVector::Zero(void)
{
	this->vx = 0;
	this->vy = 0;
	this->vz = 0;
}


void validate_CVector(void)
{
	VALIDATE(CVector, vx, 0);
	VALIDATE(CVector, vy, 4);
	VALIDATE(CVector, vz, 8);

	VALIDATE(Vector, vx, 0);
	VALIDATE(Vector, vy, 4);
	VALIDATE(Vector, vz, 8);
}

void validate_CSVector(void)
{
	VALIDATE(CSVector, vx, 0);
	VALIDATE(CSVector, vy, 2);
	VALIDATE(CSVector, vz, 4);
}

void validate_SVector(void){
	VALIDATE(SVector, vx, 0);
	VALIDATE(SVector, vy, 2);
	VALIDATE(SVector, vz, 4);
}

// @Bogus
void patch_vector(void)
{
	// Everything here is pure arithmetic on the arguments: no globals, no calls
	// out of the file, so there is nothing to share with the exe and nothing to
	// audit. All 21 mangled names match the original's exactly, so they go
	// through PATCH_PUSH_RET_POLY rather than taking an overloaded operator's
	// address.
	//
	// Not hooked: CVector::CVector and the Set/SetX/SetY/SetZ family are INLINE
	// in vector.h with no standalone body; CVector::Zero, operator*(int, CVector)
	// and CSVector's operator/= and operator%= have no named address in the
	// original (operator*(int, CVector) has the same body as
	// operator*(CVector, int) and was most likely folded into 0x004E77A0 at link
	// time, so hooking it would mean guessing).
	PATCH_PUSH_RET_POLY(0x004E74A0, CVector::Length,        "?Length@CVector@@QAEHXZ");
	PATCH_PUSH_RET_POLY(0x004E7500, CVector::LengthSquared, "?LengthSquared@CVector@@QAEHXZ");
	PATCH_PUSH_RET_POLY(0x004E7550, CVector::KillSmall,     "?KillSmall@CVector@@QAEXXZ");
	PATCH_PUSH_RET_POLY(0x004E7590, CVector::operator_add_assign, "??YCVector@@QAEPAV0@ABV0@@Z");
	PATCH_PUSH_RET_POLY(0x004E75C0, CVector::operator_sub_assign, "??ZCVector@@QAEPAV0@ABV0@@Z");
	PATCH_PUSH_RET_POLY(0x004E75F0, CVector::operator_mul_assign, "??XCVector@@QAEPAV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E7620, CVector::operator_div_assign, "??_0CVector@@QAEPAV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E7650, CVector::operator_shr_assign, "??_2CVector@@QAEPAV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E7680, CVector::operator_shl_assign, "??_3CVector@@QAEPAV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E76B0, CVector::operator_mod_assign, "??_1CVector@@QAEPAV0@ABVCFriction@@@Z");
	PATCH_PUSH_RET_POLY(0x004E76F0, CVector::operator_not_equal,  "??9CVector@@QAEHABV0@@Z");
	PATCH_PUSH_RET_POLY(0x004E7720, operator_add_vec_vec, "??H@YA?AVCVector@@ABV0@0@Z");
	PATCH_PUSH_RET_POLY(0x004E7760, operator_sub_vec_vec, "??G@YA?AVCVector@@ABV0@0@Z");
	PATCH_PUSH_RET_POLY(0x004E77A0, operator_mul_vec_int, "??D@YA?AVCVector@@ABV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E77D0, operator_mul_vec_vec, "??D@YA?AVCVector@@ABV0@0@Z");
	PATCH_PUSH_RET_POLY(0x004E7800, operator_div_vec_int, "??K@YA?AVCVector@@ABV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E7840, operator_shr_vec_int, "??5@YA?AVCVector@@ABV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E7870, operator_shl_vec_int, "??6@YA?AVCVector@@ABV0@ABH@Z");
	PATCH_PUSH_RET_POLY(0x004E78A0, CSVector::Mask,       "?Mask@CSVector@@QAEXXZ");
	PATCH_PUSH_RET_POLY(0x004E78C0, CSVector::KillSmall,  "?KillSmall@CSVector@@QAEXXZ");
	PATCH_PUSH_RET_POLY(0x004E7900, CSVector::operator_add_assign, "??YCSVector@@QAEPAV0@ABV0@@Z");
}
