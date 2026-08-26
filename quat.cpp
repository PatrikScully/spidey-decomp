#include "quat.h"
#include "validate.h"
#include "ps2funcs.h"

// @MEDIUMTODO
void MToQ(MATRIX const & a1, CQuat& a2)
{
	printf("void MToQ(MATRIX const & a1, CQuat& a2)");
}


// @NotOk
// residue: fixed-point quat->matrix conversion, formula verified correct
// against the disassembly (all 9 rotation terms + zeroed translation match
// semantically), but MSVC6's register scheduler diverges from the top:
// original loads w,x,y,z eagerly (4 loads before any multiply), our build
// loads only 2 fields before the first multiply no matter which of the 4!
// and 9!-style declaration order we try. Tried 13 distinct source shapes
// (declare order permutations w/x/y/z, all 6 permutations of the xy/yy/xz
// product order, comma-vs-statement declarations, m00 subtraction order),
// best is 39 mnemonic diffs (perm: xx,zw,xw,yw,yy,xy,xz,zz,yz product order,
// m->m[0][0] = 4096 - zz - yy). Stack frame size (sub esp,0Ch) matches in
// this variant. Needs more attempts or decomp.me before this can match.
void QToM(CQuat* q, MATRIX* m)
{
	i32 x = q->x;
	i32 y = q->y;
	i32 z = q->z;
	i32 w = q->w;

	i32 xx = (x * x) >> 11;
	i32 zw = (z * w) >> 11;
	i32 xw = (x * w) >> 11;
	i32 yw = (y * w) >> 11;
	i32 yy = (y * y) >> 11;
	i32 xy = (y * x) >> 11;
	i32 xz = (z * x) >> 11;
	i32 zz = (z * z) >> 11;
	i32 yz = (z * y) >> 11;

	m->m[0][0] = 4096 - zz - yy;
	m->m[0][1] = zw + xy;
	m->m[1][0] = xy - zw;
	m->m[0][2] = xz - yw;
	m->m[1][1] = 4096 - xx - zz;
	m->m[1][2] = xw + yz;
	m->m[2][0] = yw + xz;
	m->m[2][1] = yz - xw;
	m->m[2][2] = 4096 - xx - yy;

	m->t[0] = 0;
	m->t[1] = 0;
	m->t[2] = 0;
}

// @MEDIUMTODO
void Quat_Slerp (CQuat& a1, CQuat const & a2, int a3, CQuat& a4)
{
	printf("void Quat_Slerp (CQuat& a1, CQuat const & a2, int a3, CQuat& a4)");
}


void validate_CQuat(void){
	VALIDATE_SIZE(CQuat, 0x10);

	VALIDATE(CQuat, x, 0x0);
	VALIDATE(CQuat, y, 0x4);
	VALIDATE(CQuat, z, 0x8);
	VALIDATE(CQuat, w, 0xC);
}
