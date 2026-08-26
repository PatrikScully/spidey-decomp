#include "quat.h"
#include "validate.h"
#include "ps2funcs.h"
#include "utils.h"

// guess: cyclic next-axis lookup table {1, 2, 0}, referenced only here.
// no name in the maintainer's idb_globals.txt near 0x551B70 (0x551B80 is
// gRelocTable, this table sits right before it).
static i32 * const gQuatAxisNext = (i32*)0x551B70;

// @Ok
// @Matching
void MToQ(MATRIX const & a1, CQuat& a2)
{
	i32 trace = a1.m[0][0] + a1.m[1][1] + a1.m[2][2];

	if (trace > 0)
	{
		i32 s = M3dMaths_SquareRoot0((trace + 4096) << 12);
		a2.w = s >> 1;
		i32 scale = 0x800000 / s;
		a2.x = ((a1.m[1][2] - a1.m[2][1]) * scale) >> 12;
		a2.y = ((a1.m[2][0] - a1.m[0][1]) * scale) >> 12;
		a2.z = ((a1.m[0][2] - a1.m[1][0]) * scale) >> 12;
		return;
	}

	i32 i = 0;
	if (a1.m[1][1] > a1.m[0][0]) i = 1;
	if (a1.m[2][2] > a1.m[i][i]) i = 2;

	i32 j = gQuatAxisNext[i];
	i32 k = gQuatAxisNext[j];

	i32 s = M3dMaths_SquareRoot0((a1.m[i][i] - a1.m[k][k] - a1.m[j][j] + 4096) << 12);

	switch (i)
	{
		case 0: a2.x = s >> 1; break;
		case 1: a2.y = s >> 1; break;
		case 2: a2.z = s >> 1; break;
	}

	i32 scale = 0x800000 / s;

	a2.w = ((a1.m[j][k] - a1.m[k][j]) * scale) >> 12;

	switch (j)
	{
		case 0: a2.x = ((a1.m[i][0] + a1.m[0][i]) * scale) >> 12; break;
		case 1: a2.y = ((a1.m[i][1] + a1.m[1][i]) * scale) >> 12; break;
		case 2: a2.z = ((a1.m[i][2] + a1.m[2][i]) * scale) >> 12; break;
	}

	switch (k)
	{
		case 0: a2.x = ((a1.m[i][0] + a1.m[0][i]) * scale) >> 12; break;
		case 1: a2.y = ((a1.m[i][1] + a1.m[1][i]) * scale) >> 12; break;
		case 2: a2.z = ((a1.m[i][2] + a1.m[2][i]) * scale) >> 12; break;
	}
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

// sin table shared with shell.cpp/spidey.cpp/manipob.cpp (same address,
// same file-local raw-address convention already used there).
static i16 * const word_610C48 = (i16*)0x610C48;

// @NotOk
// residue: full slerp algorithm (zero-quat guards, dot product, negate-if-
// opposite, near-identical lerp shortcut, near-opposite perpendicular-blend
// shortcut, general arccos/sin-table slerp) verified against the
// disassembly and matches semantically. Both "copy a2/a1 into a4" guard
// blocks byte-match exactly once written as plain struct assignment
// (`a4 = a2;`) instead of field-by-field copies. Remaining residue starts
// at the dot product multiply-accumulate chain (4 field's build,
// N=4 register-heavy sar-shift-once pattern): tried natural field order and
// the exact per-term operand order from the disasm (a2.x*a1.x + a1.y*a2.y +
// a2.z*a1.z + a2.w*a1.w), both give 110 mnemonic diffs, same as unordered.
// Same MSVC6 register-scheduler resistance to source reordering seen on
// QToM in this file. Needs more attempts or decomp.me.
void Quat_Slerp (CQuat& a1, CQuat const & a2, int a3, CQuat& a4)
{
	if (a1.x + a1.y + a1.z + a1.w == 0)
	{
		a4 = a2;
	}

	if (a2.x + a2.y + a2.z + a2.w == 0)
	{
		a4 = a1;
	}

	i32 dot = (a2.x * a1.x + a1.y * a2.y + a2.z * a1.z + a2.w * a1.w) >> 12;

	if (dot < 0)
	{
		a1.x = -a1.x;
		a1.y = -a1.y;
		a1.z = -a1.z;
		a1.w = -a1.w;
		dot = -dot;
	}

	if (dot + 4096 <= 128)
	{
		// nearly opposite quaternions: blend against a perpendicular quat
		a4.x = -a1.y;
		a4.y = -a1.x;
		a4.z = -a1.w;
		a4.w = a1.z;

		i32 angle0 = ((4096 - a3) * 6434) >> 12;
		i32 angle1 = (a3 * 6434) >> 12;
		i32 s0 = word_610C48[2 * (angle0 & 0xFFF)];
		i32 s1 = word_610C48[2 * (angle1 & 0xFFF)];

		a4.x = (a1.x * s0 + a4.x * s1) >> 12;
		a4.y = (a1.y * s0 + a4.y * s1) >> 12;
		a4.z = (a1.z * s0 + a4.z * s1) >> 12;
		a4.w = (a1.w * s0 + a4.w * s1) >> 12;
		return;
	}

	i32 w0, w1;

	if (4096 - dot <= 128)
	{
		// nearly identical quaternions: plain lerp
		w0 = 4096 - a3;
		w1 = a3;
	}
	else
	{
		i32 theta = Utils_ArcCos(dot);
		i32 sinTheta = word_610C48[2 * (theta & 0xFFF)];
		i32 angle0 = ((4096 - a3) * theta) >> 12;
		i32 angle1 = (a3 * theta) >> 12;
		w0 = (word_610C48[2 * (angle0 & 0xFFF)] << 12) / sinTheta;
		w1 = (word_610C48[2 * (angle1 & 0xFFF)] << 12) / sinTheta;
	}

	a4.x = (a1.x * w0 + a2.x * w1) >> 12;
	a4.y = (a1.y * w0 + a2.y * w1) >> 12;
	a4.z = (a1.z * w0 + a2.z * w1) >> 12;
	a4.w = (a1.w * w0 + a2.w * w1) >> 12;
}


void validate_CQuat(void){
	VALIDATE_SIZE(CQuat, 0x10);

	VALIDATE(CQuat, x, 0x0);
	VALIDATE(CQuat, y, 0x4);
	VALIDATE(CQuat, z, 0x8);
	VALIDATE(CQuat, w, 0xC);
}
