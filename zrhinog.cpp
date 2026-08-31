#include "zrhinog.h"
#include "export.h"

// @MEDIUMTODO
// Re-investigated 2026-08-31 with IDA decompile+disasm of 0x43A400 (Effects_RhinoStomp, 333
// bytes), 0x43A300 (dust particle spawner, 252 bytes) and 0x439E20 (the dust particle's
// constructor, 200 bytes), going well past the prior session's pass. Progress since then:
//
// 1) The two "shockwave" impact quads Effects_RhinoStomp itself constructs (via
//    sub_4088A0(136)+sub_43A0F0 twice, once per foot) are CFootprint, and CFootprint is ALREADY
//    fully implemented and @Ok in effects.cpp (confirmed: sub_43A0F0 == CFootprint_ctor in
//    names.json, and effects.cpp's CFootprint::CFootprint matches the disassembly field-for-field,
//    down to the "RhinoStomp" texture name it passes to Spool_FindTextureChecksum). So that half of
//    this function is no longer a blocker; it would just be `new CFootprint(pos, angle)` twice.
//
// 2) The dust particle spawner (sub_43A300, called once, loops 20 times, allocates a 96-byte
//    object via sub_4088A0 and constructs it via sub_439E20) is now understood field-by-field from
//    raw disassembly (not just Hex-Rays), confirmed against CBit's own VALIDATEd offsets
//    (mVel@0x1C, matches bit.cpp exactly) and CGLine's (mStart@0x44, mEnd@0x50, mCodeBGR0@0x3C,
//    matches bit2.cpp exactly):
//      - base ctor: CGLine::CGLine() (sub_412C00), so it derives from CGLine (names.json already
//        guesses "CPingLine" for this constructor).
//      - vtable set to off_53B7B0 (not CGLineParticle's; that is a different, already-implemented
//        0x60-byte class at 0x413080 with an unrelated 4-arg ctor).
//      - mVel = Utils_GetVecFromMagDir(magnitude = Rnd(50) + 30, dir = a fixed {-312, 0, 0} angle
//        struct built by the caller, not randomized per-particle).
//      - mStart = *pos (the position vector passed in, copied directly, 3 dwords).
//      - mEnd = Utils_GetVecFromMagDir(magnitude = 200, dir = same fixed angle struct) + *pos
//        (CVector::operator+=, sub_4E7590).
//      - SetRGB0(0, 0, 0), then SetRGB1(r, g, b) from 3 explicit caller args (always (128,128,128)
//        at this call site).
//      - mCodeBGR0 |= 0x2000000 (a flag bit, OR'd onto the existing CGLine field, not a new field).
//      - a genuinely new field at offset 0x5C (right after CGLine's 0x5C-byte body, same slot
//        CGLineParticle uses for its own extra field, but stored here as a single byte, not i32):
//        the caller's "fade rate" argument (15 at this call site), print_if_false-checked against
//        "Zero FadeRate".
//      - the particle's spawn position comes from *pos (RhinoStomp foot position) offset by
//        word_610C48/word_610C4A[2 * (Rnd(4096) & 0xFFF)] * 80, the same footstep-offset trig
//        tables CFootprint's own already-@Ok constructor uses, scaled by a fixed 80 (=16*5) instead
//        of CFootprint's 70.
//
// 3) Still a REAL blocker, not a guess I'm willing to make: off_53B7B0 has 8 vtable slots
//    (0x439F00, 0x43A040, 0x43A260, 0x43A290, 0x43A620, 0x43A650, 0x43A9D0, 0x43AD30; the last 6
//    are shared verbatim with CFootprint's own vtable off_53B7B8, which only adds 2 more slots of
//    its own on top - 0x43B140, 0x43B1B0, presumably CQuadBit's Move()/dtor overrides). CBit as
//    currently modeled in bit.h declares only 2 virtuals (~CBit, Move), and CGLine (bit2.h) adds no
//    new ones, so 8 slots does not add up against the repo's current header - the real virtual
//    function count on CBit/CGLine is bigger than what's documented there. Adding a new
//    CGLine-derived class with the wrong vtable slot count would silently corrupt every other
//    CGLine/CQuadBit-derived object's virtual dispatch, so this is not something to guess past;
//    it needs bit.h/bit2.h's virtual function list fixed first (out of scope here: this file is
//    zrhinog.cpp/zrhinog.h only). Whoever picks this up should decompile the 8 vtable targets above
//    to find out what CBit/CGLine really declare.
//
// 4) sub_439E20 (the dust particle ctor) has 3 OTHER call sites besides the Rhino stomp dust
//    spawner: 0x45BAE0, 0x482F20, 0x48F040 - so resolving its class properly benefits more than
//    just this one function.
//
// Not implementing without the real vtable, per repo instructions (never guess a struct/vtable
// layout). Leaving as TODO; the CFootprint half is fully unblocked for whoever adds the new class.
void Effects_RhinoStomp(CSuper *)
{
	printf("Effects_RhinoStomp");
}