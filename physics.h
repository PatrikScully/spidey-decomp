#pragma once

#ifndef PHYSICS_H
#define PHYSICS_H

#include "vector.h"
#include "export.h"

EXPORT void Physics_SetGravity(CVector *);

// CSwinger::GetCurrentParams(CVector &), original 0x4F7270 (Mac mangled name
// .GetCurrentParams__8CSwingerFR7CVector). It really belongs in web.cpp next
// to CSwinger_SwingBack, and its body needs two CVector fields of CSwinger at
// 0x108/0x10C plus one at 0x170 that web.h still keeps inside PADDING, so it
// is only declared here (as a free function taking the swinger pointer) for
// CPlayer::DoSwingingPhysics to call. Move it into CSwinger when web.h is
// free to change.
EXPORT void CSwinger_GetCurrentParams(i32 *pSwinger, CVector *pOut);

#endif
