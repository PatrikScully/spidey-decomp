#include "physics.h"

EXPORT void Physics_SetGravity(CVector *);

// @Ok
void Physics_SetGravity(CVector *pVec)
{
    static CVector * const gGravity = (CVector*)0x60F7B0;
    static i32 * const gGravityLength = (i32*)0x60F7BC;
    static CVector * const gNormalizedGravity = (CVector*)0x60F888;

    *gGravity = *pVec;
    *gGravityLength = gGravity->Length();
    *gNormalizedGravity = *gGravity / (-*gGravityLength);
}
