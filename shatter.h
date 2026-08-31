#pragma once

#ifndef SHATTER_H
#define SHATTER_H

#include "export.h"
#include "ob.h"

EXPORT extern i32 gGlassShatterSound;

// 0x6A75F8: CVector, world position of the face being shattered. Set by Shatter_Face (still a
// stub, not yet decompiled) and consumed by CShatterBit::SetPos (bit.cpp) to compute a shard's
// outward "explosion" velocity; also used by Exp_GlowFlash per Shatter_Face's own decompilation
// notes further down in this file. Guess name, no idb_globals.txt entry for this address.
#define G_SHATTER_FACE_CENTER (*reinterpret_cast<CVector*>(0x006A75F8))

// 0x6A768C: holds a POINTER to an i16[3] table (confirmed: every use dereferences it a second
// time, e.g. *(i16*)(dword_6A768C + 4)). Read as a per-axis velocity/jitter scale:
// CShatterBit::SetPos (bit.cpp) uses all 3 axes to bias a freshly-built shard's initial
// velocity; Split (below) uses only axis 2 (z) to jitter a shattered triangle's corner deltas
// before constructing the CShatterBit. Also read (not traced, out of scope) inside
// Shatter_Face's body. No write site found anywhere in this session's tracing (bit.cpp,
// shatter.cpp, Shatter_Face's disasm), so this is likely a fixed table set up once elsewhere
// (game init) rather than something Shatter_Item/Face vary per call. Guess name.
#define G_SHATTER_VELOCITY_SCALE (*reinterpret_cast<i16**>(0x006A768C))

// 0x6A75D8 and 0x6A7688: two u16 globals, both written only inside Shatter_Face (0x48C0D0,
// still a stub, not yet decompiled) and read only inside Split (below), confirmed by xrefs.
// Forwarded as CChunkBit::SetUVs's first two (ushort,ushort) parameters. Per SetUVs's own
// disasm (0x40B910), only the FIRST of the two is actually read inside SetUVs (stored into
// CChunkBit's own undeclared field at +0xB4); the second is a dead parameter there (same class
// of unused-but-present param as CShatterBit's own ctor 5th arg, see bit.h). Names and exact
// semantics are our guess; likely a texture/page id (used one) and a leftover/unused value.
#define G_SHATTER_UV_TEX_ID (*reinterpret_cast<u16*>(0x006A75D8))
#define G_SHATTER_UV_UNUSED (*reinterpret_cast<u16*>(0x006A7688))

EXPORT void Shatter_MaybeMakeGlassShatterSound(void);
EXPORT void CalcRGB(i32,u32,i32,u32 *);
EXPORT i32 Shatter_Face(CItem *,u32 *,i32,i32,i32,i32,i32);
EXPORT i32 Shatter_Glass(i32,CVector const *,CVector const *,CVector const *,CVector const *,u8,u8,u8);
EXPORT i32 Shatter_Item(CItem *,i32,i32);
EXPORT void Split(CVector const *,CVector const *,CVector const *,i32,i32,i32,i32,i32,i32,u32,i32);
EXPORT void TransformVertex(CVector *,SVECTOR *,u8 *,i32);

#endif
