#include "PCGfx.h"
#include "PCTex.h"
#include "DXsound.h"
#include "SpideyDX.h"
#include "DXinit.h"
#include "camera.h"
#include "mess.h"
#include "ps2m3d.h"
#include "pshell.h"
#include "spool.h"
#include "ps2pad.h"
#include "PCInput.h"
#include "m3dinit.h"

#include <cmath>
#include <cstring>

// my_malloc/my_free live in main.cpp (0x52A227/0x52A3C0, already
// PATCH_PUSH_RET'd there), not declared in any header. Plain extern
// declarations, not a redefinition.
extern void *my_malloc(size_t s);
extern void my_free(void *block);

EXPORT i32 gAnotherGameResolutionX = gGameResolutionX;
EXPORT i32 gAnotherGameResolutionY = gGameResolutionY;

EXPORT i32 gDrawTexture2DRelatedOne;
EXPORT i32 gDrawTexture2DRelatedTwo;

EXPORT i8 gPcGfxBrightnessValues[256];

EXPORT u8 gProcessTextureRelated;
EXPORT u16 gChosenBlendingMode;
EXPORT u16 gProcessedTextureFlags;

EXPORT i32 gUseTextureRelated = 0x0FFFFFFFF;
EXPORT DCGfx_BlendingMode gTextureBlendingMode;

EXPORT i32 gPcGfxBlendModeRelated;
EXPORT DXPOLY gDxPolys[15360];

EXPORT f32 gRenderInitOne[3] = {  10.0f, 8048.0f, 276.0f };
EXPORT f32 gRenderInitTwo[2] = { 8038.0f, 1.9624f };

EXPORT i32 gPcGfxDrawRelated = 1;
EXPORT i32 gPcGfxSlotNumber =  0x0FFFFFFFF;

EXPORT u8 gIsRenderSettingE = 1;
EXPORT u8 gNonRendderSettingE;

u8 gSceneRelated;

EXPORT f32 gZLayerFurthest;
EXPORT f32 gZLayerNearest;

EXPORT f32 gFlFoggingParamOne = 100000.0f;
EXPORT f32 gFlFoggingParamTwo = 100005.0f;
EXPORT u32 gU32FoggingParamThree;

u8 gBFoggingRelated;

u32 gPcGfxSkyColor;

EXPORT i32 gEndSceneRelated = -1;
EXPORT i32 gEndSceneRelatedTwo;

// 0xAC08F0, set to 1 by PCGfx_ClipTriToNearPlane whenever it actually cuts a
// triangle. Not in the maintainer's IDB globals list, name is our guess.
u8 gTriWasClipped;

// @Ok
// @Matching
i32 acG2Write(void*, void*, i32)
{
	return 0;
}

// @Ok
// @Matching
i32 amHeapAlloc(u32**, i32, i32, i32, i32)
{
	return 0;
}

// @Ok
// @Matching
i32 acDspSetMixerChannel(i32, i32, i32, i32)
{
	return 0;
}

// @Ok
// @Matching
i32 amHeapFree(i32)
{
	return 0;
}

// @NotOk
// missing fog stuff
void PCGfx_BeginScene(u32,i32)
{
	if (!gSceneRelated)
	{
		if (gBFoggingRelated)
		{
			setupFog();
		}

		PCGfx_ProcessTexture(0, -1, DCGfx_BlendingMode_0);
		DXPOLY_BeginScene();
		gSceneRelated = 1;
		gZLayerNearest = 0.0099999998;
		gZLayerFurthest = -0.2;
	}
}

// @SMALLTODO
// Forward to the original. This blends a vertex color toward the fog color
// using the 4 lighting tables PCGfx_BeginScene/setupFog build (still not
// done, see pcgfx.attempts.md), so we can't reproduce the math yet.
static u32 gsub_506D70(f32 a1, u32 a2)
{
	// @FIXME
	typedef u32 (*func_ptr)(f32, u32);
	func_ptr func = (func_ptr)0x00506D70;
	return func(a1, a2);
}

// @NotOk
// Structural translation only, NOT verified against compare.py yet. Builds
// 3 temporary _DXVERT vertices from raw tagKMVERTEX3 records addressed
// through the u16 index array (3 indices per triangle), including a per
// channel color brighten step (kept as is when the low 3 bytes of the color
// are already 0, else (c>>1 & 0x7F7F7F) + 0x0F0F0F with the top byte kept),
// calls PCGfx_ClipTriToNearPlane on them, then processes verts[0..2]
// (verts[3] only if clipping produced a 4th vertex) with a per vertex fog
// color blend and a fog depth remap identical in shape to PCGfx_DrawQuad2D's
// existing (v24 - gRenderInitOne[0]) / gRenderInitTwo[0] idiom, before
// calling submitPoly with count 3 or 4. Skips the whole triangle (no
// submitPoly call) when verts[0] is null, which happens when
// PCGfx_ClipTriToNearPlane's countBehind == 3 case zeroed all 3 verts.
// The 2 print_if_false asserts and their strings ("verts[1] is null!",
// "verts[2] is null!" at 0x5682A0/0x56828C) are confirmed from the binary.
// gsub_506D70, gRenderInitOne/Two, gPcGfxBlendModeRelated, gNonRendderSettingE,
// gPcGfxDrawRelated and gEndSceneRelatedTwo's game addresses (0x506d70,
// 0x56817C/0x568184/0x568190/0x568194, 0xAC08E0, 0xAC08D0, 0x568178,
// 0xAC08F4) all matched an existing repo global 1:1 against
// idb_globals.txt, so those parts are higher confidence. tagKMVERTEX3's
// field layout is a positional guess (see PCGfx.h) and the a2 parameter is
// genuinely never read in the disassembly, kept unused to match. The exact
// stack shuffling right before the submitPoly call (there is what looks
// like a second, redundant verts[0] test) is simplified to a single guard.
// cmpsum: 282 mnemonic diffs at 0x506980, first divergence right at entry
// (frame size / register allocation). Not iterated further this session.
void PCGfx_ClipSendIndexedVertList(tagKMVERTEX3 const *vertArray, i32 a2, u16 const *indices, i32 indexCount)
{
	_DXVERT temp[3];
	_DXVERT *verts[4];
	_DXVERT out0, out1;
	_DXVERT *out[2] = { &out0, &out1 };

	u16 const *idx = indices;
	u16 const *end = indices + indexCount;

	if (idx == end)
		return;

	do
	{
		for (i32 k = 0; k < 3; k++)
		{
			tagKMVERTEX3 const *src = &vertArray[*idx];
			idx++;

			temp[k].field_0 = src->field_4;
			temp[k].field_4 = src->field_8;
			temp[k].field_8 = 1.0f / src->field_C;
			temp[k].field_14 = src->field_10;
			temp[k].field_18 = src->field_14;

			u32 c = src->field_18;
			if ((c & 0xFFFFFF) == 0)
			{
				temp[k].field_10 = c;
			}
			else
			{
				temp[k].field_10 = (c & 0xFF000000) | (((c >> 1) & 0x7F7F7Fu) + 0x0F0F0Fu);
			}
		}

		verts[0] = &temp[0];
		verts[1] = &temp[1];
		verts[2] = &temp[2];

		gTriWasClipped = 0;
		PCGfx_ClipTriToNearPlane(verts, out);

		if (verts[0])
		{
			f32 bias = 0.0f;
			if (gPcGfxBlendModeRelated && !gLowGraphics)
				bias = (f32)gPcGfxBlendModeRelated * gRenderInitTwo[1];

			i32 count = verts[3] ? 4 : 3;

			for (i32 k = 0; k < count; k++)
			{
				_DXVERT *v = verts[k];

				if (gNonRendderSettingE)
					v->field_10 = gsub_506D70(v->field_8, v->field_10);

				v->field_C = gRenderInitOne[2] / v->field_8;
				v->field_8 = (bias + v->field_8 - gRenderInitOne[0]) / gRenderInitTwo[0];

				if (!gLowGraphics)
				{
					v->field_14 *= v->field_C;
					v->field_18 *= v->field_C;
				}
			}

			print_if_false(verts[1] != 0, "verts[1] is null!");
			print_if_false(verts[2] != 0, "verts[2] is null!");

			gPcGfxDrawRelated |= 4;

			submitPoly(verts, count);
		}
	}
	while (idx != end);
}

// @NotOk
// verts holds the 3 input triangle vertex pointers, plus a spare 4th slot
// (verts[3]) used when clipping turns the triangle into a quad. out[0]/out[1]
// are the two spare _DXVERT slots the caller passes in to receive the newly
// interpolated vertices. residue: original tests countBehind with a
// dec/je/dec/je/dec/jne chain (switch-style dispatch on a cached local, see
// tips.txt); our separate ifs compile to plain cmp/jne instead. 67 mnemonic
// diffs at 0x506e40 as of this attempt, 2 hypotheses tried, logged in
// pcgfx.attempts.md.
void PCGfx_ClipTriToNearPlane(_DXVERT **verts, _DXVERT *const *out)
{
	static const i32 prevIdx[3] = {2, 0, 1};
	static const i32 nextIdx[3] = {1, 2, 0};

	i32 countBehind = 0;
	i32 behindIdx = -1;
	verts[3] = 0;
	i32 frontIdx = -1;

	for (i32 i = 2; i >= 0; i--)
	{
		if (verts[i]->field_8 < gRenderInitOne[0])
		{
			behindIdx = i;
			countBehind++;
		}
		else
		{
			frontIdx = i;
		}
	}

	if (countBehind == 1)
	{
		ZCLIP_VERT(out[0], verts[prevIdx[behindIdx]], verts[behindIdx], gRenderInitOne[0]);
		ZCLIP_VERT(out[1], verts[nextIdx[behindIdx]], verts[behindIdx], gRenderInitOne[0]);

		for (i32 i = 2; i > behindIdx; i--)
			verts[i + 1] = verts[i];

		verts[behindIdx] = out[0];
		verts[behindIdx + 1] = out[1];
		gTriWasClipped = 1;
	}
	if (countBehind == 2)
	{
		ZCLIP_VERT(out[0], verts[frontIdx], verts[prevIdx[frontIdx]], gRenderInitOne[0]);
		ZCLIP_VERT(out[1], verts[frontIdx], verts[nextIdx[frontIdx]], gRenderInitOne[0]);

		verts[prevIdx[frontIdx]] = out[0];
		verts[nextIdx[frontIdx]] = out[1];
		gTriWasClipped = 1;
	}
	if (countBehind == 3)
	{
		verts[2] = 0;
		verts[1] = 0;
		verts[0] = 0;
	}
}

// @Ok
// @Validate
void PCGfx_DoModelPreview(void)
{
	i32 totalSomething = 0;
	CSuper* SuperItem = 0;
	CSuper* SuperItemNext = 0;
	u8 doModelSwap = 0;
	DWORD modelTickUpdate = 0;

	PShell_Initialise();
	i32 freeIndex = 0;
	for (; freeIndex < MAXPSX; freeIndex++)
	{
		if (!PSXRegion[freeIndex].Filename[0])
			break;
	}


	i32 v24[40];

	for (i32 j = 0; j < freeIndex; j++)
	{
		// @FIXME
		// remove
		break;

		i32 v8 = PSXRegion[j].pPSX[2];
		v24[j] = v8;
		totalSomething += v8;
	}

	//if (totalSomething && freeIndex)
	if (true)
	{
		PCGfx_SetSkyColor(0xFF800080);
		i32 idx = 0;

		i32 v9;
		for (v9 = 0; v9 < freeIndex; v9++)
		{
			//@FIXME
			break;
			if (v24[v9] > 0)
			{
				if (!PSXRegion[v9].IsSuper)
				{
					SuperItem = reinterpret_cast<CSuper*>(PSXRegion[v9].pSuper);
					SuperItemNext = reinterpret_cast<CSuper*>(SuperItem->mNextItem);
					SuperItem->mNextItem = 0;
				}

				SuperItem = createSuperItem(PSXRegion[v9].pSuper);
			}
		}

		//@FIXME
		/*
		gMikeCamera[0].Position.vx = SuperItem->mPos.vx >> 12;
		gMikeCamera[0].Position.vy = SuperItem->mPos.vy >> 12;
		gMikeCamera[0].Position.vz = (SuperItem->mPos.vz >> 12) - 1;
		*/

		gMikeCamera[0].Angles.vx = 0;
		gMikeCamera[0].Angles.vy = 0;
		gMikeCamera[0].Angles.vz = 0;
		gMikeCamera[0].Style = 0;

		i32 stop = 0;
		while (!stop)
		{
			Pad_Update();
			if (gSControl[0].Left.Pressed)
			{
				gMikeCamera[0].Angles.vy -= 16;
				gMikeCamera[0].Angles.vy &= 0xFFF;
			}
			else if (gSControl[0].Right.Pressed)
			{
				gMikeCamera[0].Angles.vy -= 16;
				gMikeCamera[0].Angles.vy &= 0xFFF;
			}

			if (gSControl[0].Up.Pressed)
			{
				if (!PCINPUT_IsKeyPressed(0x42, 0) && !PCINPUT_IsKeyPressed(0x36, 0))
				{
					i32 v14 = gMikeCamera[0].Angles.vy & 0xFFF;
					gMikeCamera[0].Position.vx += (32 * rcossin_tbl[v14].sin) >> 12;
					gMikeCamera[0].Position.vz += (32 * rcossin_tbl[v14].cos) >> 12;
					gMikeCamera[0].Position.vy -= (32 * rcossin_tbl[gMikeCamera[0].Angles.vx & 0xFFF].sin) >> 12;
				}
				else
				{
					gMikeCamera[0].Angles.vx += 16;
					gMikeCamera[0].Angles.vx &= 0xFFF;
				}
			}
			else if (gSControl[0].Down.Pressed)
			{
				if (!PCINPUT_IsKeyPressed(0x2Au, 0) && !PCINPUT_IsKeyPressed(0x36u, 0))
				{
					 gMikeCamera[0].Angles.vx = (gMikeCamera[0].Angles.vx - 16) & 0xFFF;
				}
				else
				{
					i32 v15 = gMikeCamera[0].Angles.vy & 0xFFF;
					gMikeCamera[0].Position.vx -= (32 * rcossin_tbl[v15].sin) >> 12;
					gMikeCamera[0].Position.vy += (32 * rcossin_tbl[gMikeCamera[0].Angles.vx & 0xFFF].sin) >> 12;
					gMikeCamera[0].Position.vz -= (32 * rcossin_tbl[v15].cos) >> 12;
				}
			}

			if (GetTickCount() - modelTickUpdate > 250)
			{
				if (gSControl[0].Square.Pressed)
				{
					u8 IsSuper = PSXRegion[v9].IsSuper;
					if (idx && !IsSuper)
					{
						idx--;
					}
					else
					{
						do
						{
							if (--v9 < 0)
								v9 = freeIndex - 1;
						}
						while(v24[v9] <= 0);

						if (IsSuper)
							idx = 0;
						else
							idx = v24[v9] - 1;
					}

					doModelSwap = 1;
				}
				else if (gSControl[0].Circle.Pressed)
				{
					idx = (idx + 1) % v24[v9];
					if (!idx || PSXRegion[v9].IsSuper)
					{
						do
						{
							v9 = (v9 + 1) % freeIndex;
						}
						while (v24[v9] <= 0);
					}
					doModelSwap = 1;
				}
			}

			
			if (PCINPUT_IsKeyPressed(0x10, 0))
			{
				stop = 1;
			}
			else
			{
				// @FIXME
				/*
				if (doModelSwap)
				{
					if (PSXRegion[SuperItem->mRegion].IsSuper)
					{
						delete SuperItem;
					}
					else
					{
						SuperItem->mNextItem = SuperItemNext;
					}

					if (PSXRegion[v9].IsSuper)
					{
						SuperItem = createSuperItem(&PSXRegion[v9].pSuper[idx]);
					}
					else
					{
						SuperItem = reinterpret_cast<CSuper*>(&PSXRegion[v9].pSuper[idx]);
						SuperItemNext = reinterpret_cast<CSuper*>(SuperItem->mNextItem);
						SuperItem->mNextItem = 0;
					}

					gMikeCamera[0].Position.vx = SuperItem->mPos.vx >> 12;
					gMikeCamera[0].Position.vy = SuperItem->mPos.vy >> 12;
					gMikeCamera[0].Position.vz = (SuperItem->mPos.vz >> 12) - 1;

					gMikeCamera[0].Angles.vx = 0;
					gMikeCamera[0].Angles.vy = 0;
					gMikeCamera[0].Angles.vz = 0;

					modelTickUpdate = GetTickCount();
					doModelSwap = 0;
				}
				*/

				PCGfx_RenderModelPreview(SuperItem, PSXRegion[v9].Filename, idx);
			}
		}

		if (PSXRegion[v9].IsSuper)
		{
			delete SuperItem;
		}
		else
		{
			SuperItem->mNextItem = SuperItemNext;
		}
	}
}

// @SMALLTODO
// Forward to the original. Builds one _DXVERT from a corner record shaped
// like tagKMVERTEX3 (field_4..field_18 only, field_0 unused), confirmed from
// the disasm at 0x509400 to run the exact same field_8/field_C formulas as
// PCGfx_DrawTPoly3D plus the REAL (not stubbed) fog table math inline for
// the color when gNonRendderSettingE is set. Needs the same fog tables
// setupFog/PCGfx_BeginScene build, which are out of scope this session (see
// pcgfx.attempts.md), so forwarding instead of reproducing the math.
static void gsub_509400(tagKMVERTEX3 const *corner, _DXVERT *out)
{
	// @FIXME
	typedef void (*func_ptr)(tagKMVERTEX3 const *, _DXVERT *);
	func_ptr func = (func_ptr)0x00509400;
	func(corner, out);
}

// @NotOk
// Draws a thick line as a quad: dy=y2-y1, dx=x2-x1, length=sqrt(dx^2+dy^2)
// (calls _sqrt at 0x529A44, confirmed). If length != 0.0f, the perpendicular
// half width offset is (width*0.5f/length)*(dy,-dx); if length == 0.0f
// (degenerate zero length line), the disasm falls back to offsetX=0,
// offsetY=width*0.5f instead of dividing by zero. 4 corners are built,
// (x1+off,y1+off), (x1-off,y1-off), (x2+off,y2+off), (x2-off,y2-off), each
// with z/color taken from the matching endpoint, then converted to a
// _DXVERT via gsub_509400 and passed to submitPoly(verts,4). This is a
// genuine attempt at the confirmed math (dx/dy/length/offset, confirmed
// against the disasm instruction by instruction), but the exact struct
// pre-initialisation block at 0x509311-0x50938c (default field values before
// the 4 gsub_509400 calls) and an unexplained per-endpoint bias using the
// constant 7.071072578430176f at 0x53C844 (added to x1/x2 before storing,
// looks like an antialiasing/endpoint cap nudge but not confirmed) are NOT
// reproduced, only the standard perpendicular quad geometry. cmpsum: 204
// mnemonic diffs at 0x509000, first divergence right at entry (our version
// has no push ebx/ebp/esi/edi, the original keeps all 4 endpoint deltas
// live across the whole function in callee saved registers). One honest
// attempt, not iterated further this session given the fog table and
// pre-init block gaps above.
void PCGfx_DrawLine(f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2, u32 color2, f32 width)
{
	f32 dy = y2 - y1;
	f32 dx = x2 - x1;
	f32 length = sqrt(dx * dx + dy * dy);

	f32 offX, offY;
	if (length != 0.0f)
	{
		f32 s = (width * 0.5f) / length;
		offX = s * dy;
		offY = -(s * dx);
	}
	else
	{
		offX = 0.0f;
		offY = width * 0.5f;
	}

	tagKMVERTEX3 corners[4];
	memset(corners, 0, sizeof(corners));

	corners[0].field_4 = x1 + offX;
	corners[0].field_8 = y1 + offY;
	corners[0].field_C = z1;
	corners[0].field_18 = color1;

	corners[1].field_4 = x1 - offX;
	corners[1].field_8 = y1 - offY;
	corners[1].field_C = z1;
	corners[1].field_18 = color1;

	corners[2].field_4 = x2 + offX;
	corners[2].field_8 = y2 + offY;
	corners[2].field_C = z2;
	corners[2].field_18 = color2;

	corners[3].field_4 = x2 - offX;
	corners[3].field_8 = y2 - offY;
	corners[3].field_C = z2;
	corners[3].field_18 = color2;

	_DXVERT vtx[4];
	_DXVERT *verts[4] = { &vtx[0], &vtx[1], &vtx[2], &vtx[3] };

	gsub_509400(&corners[0], &vtx[0]);
	gsub_509400(&corners[1], &vtx[1]);
	gsub_509400(&corners[2], &vtx[2]);
	gsub_509400(&corners[3], &vtx[3]);

	submitPoly(verts, 4);
}

// @NotOk
// Same shape as PCGfx_DrawTPoly2D (screen space poly, own DXPOLY build,
// direct DXPOLY_DrawPoly call, manual gDxPolys pointer walk), just 4
// vertices instead of 3 (field_C=4, loop count 4). Reuses the zOffset
// preamble fix found on DrawTPoly2D this session (if/else computing v13 in
// each branch so the compiler CSEs the shared gRenderInitTwo[1]*zOffset
// multiply after the compare, `if (zOffset >= 0.0f)` branch order). One
// notable difference from DrawTPoly2D confirmed in the disasm: the
// print_if_false("invalid zOffset!") call here has NO "push edi" register
// save before it (DrawTPoly2D's does), consistent with different register
// pressure between the two functions, not a copy paste error.
// cmpsum: 207 mnemonic diffs at 0x507910. The zOffset preamble and the
// print_if_false call both matched cleanly this time (confirms the
// DrawTPoly2D fixes generalise and that function's residue really was
// register pressure specific, not a fundamental block). The residue here
// starts well into the per vertex field copy block: our field_0/field_4/
// field_14/field_18/field_10/field_8/field_C assignment order per vertex
// (declared in that order for all 4 vertices) does not match the original's
// scattered store order (it interleaves stores across all 4 vertices'
// structs rather than finishing one vertex before the next, visible in the
// disasm as stores to +0x34,+0x38,...+0x88 in a non-monotonic sequence).
// One attempt at the vertex struct fields (declaration order matching field
// layout, not the original's interleaved store order); short of the 10+
// hypotheses per cluster bar for a >1000 byte function, logged in
// pcgfx.attempts.md.
void PCGfx_DrawQPoly2D(
		f32 x0, f32 y0, f32 u0, f32 v0, u32 color0,
		f32 x1, f32 y1, f32 u1, f32 v1, u32 color1,
		f32 x2, f32 y2, f32 u2, f32 v2, u32 color2,
		f32 x3, f32 y3, f32 u3, f32 v3, u32 color3,
		f32 zOffset)
{
	gPcGfxDrawRelated &= 0xFFFFFFFB;

	if (zOffset <= 6.0f)
		gPcGfxSlotNumber = (i32)zOffset;

	f32 v13;
	if (zOffset >= 0.0f)
		v13 = gRenderInitTwo[1] * zOffset + gRenderInitOne[0];
	else
		v13 = gRenderInitTwo[1] * zOffset + gRenderInitOne[1];

	f32 v24 = v13;
	print_if_false(v24 > 0.0f, "invalid zOffset!");

	f32 v27 = (v24 - gRenderInitOne[0]) / gRenderInitTwo[0];
	f32 v32 = gRenderInitOne[2] / v24;

	SDXPolyField dxPolyFields[4];

	dxPolyFields[0].field_0 = x0;
	dxPolyFields[0].field_4 = y0;
	dxPolyFields[0].field_14 = u0;
	dxPolyFields[0].field_18 = v0;
	dxPolyFields[0].field_10 = color0;
	dxPolyFields[0].field_8 = v27;
	dxPolyFields[0].field_C = v32;

	dxPolyFields[1].field_0 = x1;
	dxPolyFields[1].field_4 = y1;
	dxPolyFields[1].field_14 = u1;
	dxPolyFields[1].field_18 = v1;
	dxPolyFields[1].field_10 = color1;
	dxPolyFields[1].field_8 = v27;
	dxPolyFields[1].field_C = v32;

	dxPolyFields[2].field_0 = x2;
	dxPolyFields[2].field_4 = y2;
	dxPolyFields[2].field_14 = u2;
	dxPolyFields[2].field_18 = v2;
	dxPolyFields[2].field_10 = color2;
	dxPolyFields[2].field_8 = v27;
	dxPolyFields[2].field_C = v32;

	dxPolyFields[3].field_0 = x3;
	dxPolyFields[3].field_4 = y3;
	dxPolyFields[3].field_14 = u3;
	dxPolyFields[3].field_18 = v3;
	dxPolyFields[3].field_10 = color3;
	dxPolyFields[3].field_8 = v27;
	dxPolyFields[3].field_C = v32;

	if (gEndSceneRelatedTwo >= 15360)
	{
		gEndSceneRelatedTwo++;
		return;
	}

	DXPOLY *p = &gDxPolys[gEndSceneRelatedTwo++];
	i32 blendMode = gPcGfxBlendModeRelated;
	f32 fogMax = 0.0f;

	if (gLowGraphics)
	{
		p->field_4 = (LPDIRECTDRAWSURFACE7)(i32)gUseTextureRelated;
		*(i32*)&p->mBlendMode = gPcGfxDrawRelated;
		if (gUseTextureRelated < 0)
			*(i32*)&p->mBlendMode = gPcGfxDrawRelated & 0xFFFFFFFB;

		p->field_C = 4;

		for (i32 i = 0; i < 4; i++)
		{
			SDXPolyField *dst = &p->field_10[i];
			memcpy(dst, &dxPolyFields[i], sizeof(SDXPolyField));

			if (!(p->mBlendMode & 4))
				dst->field_4 = 1.0f;

			i32 alpha;
			if (gProcessTextureRelated)
				alpha = 128;
			else
				alpha = (dst->field_10 >> 24) & 0xFF;

			dst->field_10 =
				gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
				gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
				alpha << 24;

			if (dst->field_8 < 0.0f)
			{
				dst->field_8 = 0.0f;
			}
			else if (dst->field_8 > 0.99989998f)
			{
				dst->field_8 = 0.99989998f;
				fogMax = dst->field_8;
			}
			else if (fogMax < dst->field_8)
			{
				fogMax = dst->field_8;
			}
		}
	}
	else
	{
		LPDIRECTDRAWSURFACE7 Direct3DTexture;
		if (gUseTextureRelated < 0)
			Direct3DTexture = 0;
		else
			Direct3DTexture = PCTex_GetDirect3DTexture(gUseTextureRelated);
		p->field_4 = Direct3DTexture;
		p->mBlendMode = gChosenBlendingMode;
		p->field_A = gProcessedTextureFlags;
		p->field_C = 4;

		for (i32 i = 0; i < 4; i++)
		{
			SDXPolyField *dst = &p->field_10[i];
			memcpy(dst, &dxPolyFields[i], sizeof(SDXPolyField));

			i32 alpha;
			if (gProcessTextureRelated)
				alpha = 128;
			else
				alpha = (dst->field_10 >> 24) & 0xFF;

			dst->field_10 =
				gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
				gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
				alpha << 24;

			if (dst->field_8 < 0.0f)
			{
				dst->field_8 = 0.0f;
			}
			else if (dst->field_8 > 0.99989998f)
			{
				dst->field_8 = 0.99989998f;
				fogMax = dst->field_8;
			}
			else if (fogMax < dst->field_8)
			{
				fogMax = dst->field_8;
			}
		}

		if (gChosenBlendingMode)
		{
			blendMode = 0;
		}
	}

	DXPOLY_DrawPoly(p, gPcGfxSlotNumber, blendMode, fogMax);
	gPcGfxSlotNumber = -1;
}

// @MEDIUMTODO
void PCGfx_DrawQPoly3D(f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32)
{
    printf("PCGfx_DrawQPoly3D(f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32,f32,f32,f32,f32,f32,u32)");
}

// @NotOk
// low graphics branch added this session (was an empty @FIXME stub before),
// mirrors submitPoly's low graphics branch. 235 mnemonic diffs at 0x507470
// as of this attempt; the first diverging instruction is in the shared
// (non-low-graphics) code above the branch, so there is a pre-existing
// residue here independent of the low graphics addition. Not iterated on
// further this session, see pcgfx.attempts.md.
void PCGfx_DrawQuad2D(
		f32 a1,
		f32 a2,
		f32 a3,
		f32 a4,
		f32 a5,
		f32 a6,
		f32 a7,
		f32 a8,
		u32 color,
		f32 a10,
		bool)
{
	gPcGfxDrawRelated &= 0xFFFFFFFB;

	if (a10 <= 6.0f)
		gPcGfxSlotNumber = a10;

	f32 v13 = a10;
	if (a10 < 0.0f)
		v13 = v13 * a10 + gRenderInitOne[1];
	else
		v13 = v13 * a10 + gRenderInitOne[0];

	f32 v24 = v13;
	print_if_false(v24 > 0.0f, "invalid zOffset!");

	SDXPolyField *pDxPolyFields[4];
	SDXPolyField dxPolyFields[4];

	dxPolyFields[0].field_18 = a6;
	dxPolyFields[0].field_4 = a2;
	dxPolyFields[1].field_4 = a2;
	dxPolyFields[0].field_10 = color;
	dxPolyFields[1].field_10 = color;
	dxPolyFields[2].field_10 = color;
	dxPolyFields[3].field_10 = color;
	dxPolyFields[0].field_0 = a1;
	dxPolyFields[0].field_14 = a5;
	dxPolyFields[3].field_0 = a1;
	dxPolyFields[3].field_14 = a5;
	pDxPolyFields[2] = &dxPolyFields[2];

	f32 v27 = (v24 - gRenderInitOne[0]) / gRenderInitTwo[0];
	dxPolyFields[0].field_8 = v27;
	dxPolyFields[1].field_8 = v27;
	dxPolyFields[2].field_8 = v27;
	dxPolyFields[3].field_8 = v27;

	pDxPolyFields[0] = dxPolyFields;
	pDxPolyFields[3] = &dxPolyFields[3];

	f32 v32 = gRenderInitOne[2] / v24;
	dxPolyFields[0].field_C = v32;
	dxPolyFields[1].field_C = v32;
	dxPolyFields[2].field_C = v32;
	dxPolyFields[3].field_C = v32;

	f32 v28 = a1 + a3;
	dxPolyFields[1].field_0 = v28;
	dxPolyFields[2].field_0 = v28;
	pDxPolyFields[1] = &dxPolyFields[1];

	f32 v25 = a5 + a7;
	dxPolyFields[1].field_14 = v25;
	dxPolyFields[1].field_18 = a6;
	dxPolyFields[2].field_14 = v25;

	f32 v29 = a2 + a4;
	dxPolyFields[2].field_4 = v29;
	dxPolyFields[3].field_4 = v29;

	f32 v30 = a6 + a8;
	dxPolyFields[2].field_18 = v30;
	dxPolyFields[3].field_18 = v30;

	if (gEndSceneRelatedTwo >= 15360)
	{
		gEndSceneRelatedTwo++;
		return;
	}

	f32 v26 = 0.0;
	DXPOLY* v16 = &gDxPolys[gEndSceneRelatedTwo++];
	i32 v31 = gPcGfxBlendModeRelated;

	if (gLowGraphics)
	{
		v16->field_4 = (LPDIRECTDRAWSURFACE7)(i32)gUseTextureRelated;
		*(i32*)&v16->mBlendMode = gPcGfxDrawRelated;
		if (gUseTextureRelated < 0)
			*(i32*)&v16->mBlendMode = gPcGfxDrawRelated & 0xFFFFFFFB;

		v16->field_C = 4;

		SDXPolyField **v21 = pDxPolyFields;
		SDXPolyField *v22 = v16->field_10;

		for (i32 i = 0; i < 4; i++)
		{
			memcpy(&v22[i], v21[i], sizeof(SDXPolyField));

			if (!(v16->mBlendMode & 4))
				v22[i].field_4 = 1.0f;

			i32 v23;
			if (gProcessTextureRelated)
				v23 = 128;
			else
				v23 = (v22[i].field_10 >> 24) & 0xFF;

			v22[i].field_10 =
				gPcGfxBrightnessValues[v22[i].field_10 & 0xFF] |
				gPcGfxBrightnessValues[(v22[i].field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(v22[i].field_10 >> 16) & 0xFF] << 16 |
				v23 << 24;

			if (v22[i].field_8 < 0.0f)
			{
				v22[i].field_8 = 0.0f;
			}
			else if (v22[i].field_8 > 0.99989998f)
			{
				v22[i].field_8 = 0.99989998f;
				v26 = v22[i].field_8;
			}
			else if (v26 < v22[i].field_8)
			{
				v26 = v22[i].field_8;
			}
		}
	}
	else
	{
		LPDIRECTDRAWSURFACE7 Direct3DTexture;
		if (gUseTextureRelated < 0)
			Direct3DTexture = 0;
		else
			Direct3DTexture = PCTex_GetDirect3DTexture(gUseTextureRelated);
		v16->field_4 = Direct3DTexture;
		v16->mBlendMode = gChosenBlendingMode;
		v16->field_A = gProcessedTextureFlags;
		v16->field_C = 4;

		SDXPolyField **v21 = pDxPolyFields;
		SDXPolyField *v22 = v16->field_10;

		for (i32 i = 0; i < 4; i++)
		{
			memcpy(&v22[i], v21[i], sizeof(SDXPolyField));
			
			i32 v23;
			if (gProcessTextureRelated)
				v23 = 128;
			else
				v23 = (v22[i].field_10 >> 24) & 0xFF;

			v22[i].field_10 =
				gPcGfxBrightnessValues[v22[i].field_10 & 0xFF] |
				gPcGfxBrightnessValues[(v22[i].field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(v22[i].field_10 >> 16) & 0xFF] << 16 |
				v23 << 24;

			if (v22[i].field_8 < 0.0f)
			{
				v22[i].field_8 = 0.0f;
			}
			else if (v22[i].field_8 > 0.99989998f)
			{
				v22[i].field_8 = 0.99989998f;
				v26 = v22[i].field_8;
			}
			else if (v26 < v22[i].field_8)
			{
				v26 = v22[i].field_8;
			}
		}
		if ( gChosenBlendingMode )
		{
			v31 = 0;
		}
	}

	DXPOLY_DrawPoly(v16, gPcGfxSlotNumber, v31, v26);
	gPcGfxSlotNumber = -1;
}

// @NotOk
// A screen space triangle (3x (x,y,u,v,color) plus one shared zOffset),
// builds its own DXPOLY inline and calls DXPOLY_DrawPoly (0x503100) directly
// instead of going through submitPoly, confirmed from the disasm: manual
// pointer walk into gDxPolys (ebp = &gDxPolys[gEndSceneRelatedTwo] via
// idx*0xF0), not the array-of-pointers shape submitPoly/DrawQuad2D use. The
// zOffset preamble (gPcGfxDrawRelated &=~4, conditional gPcGfxSlotNumber set,
// v13=gRenderInitTwo[1]*zOffset + gRenderInitOne[0 or 1] depending on sign,
// print_if_false "invalid zOffset!" at 0x568304) and the per vertex color
// brighten/fog clamp loop are the exact same idioms as PCGfx_DrawQuad2D and
// submitPoly (reused verbatim), field_8/field_C are shared across all 3
// vertices (single v27/v32, matches DrawQuad2D, this is a 2D/flat triangle
// not a perspective one). One real difference from DrawQuad2D confirmed by
// the disasm: the low graphics branch has the "if (!(mBlendMode&4))
// field_4=1.0f" step per vertex, the hardware branch here does NOT (DrawQuad2D
// has it in both branches), kept as is since the bytes say so. 0x71C720/
// 0x71C734 read as gChosenBlendingMode/gProcessedTextureFlags (u16 loads,
// matches their repo types) rather than new globals. 0x50F3C0 is
// PCTex_GetDirect3DTexture (same call shape as DrawQuad2D's texture lookup),
// not a new unnamed helper as an earlier session guessed.
// cmpsum: 208 mnemonic diffs at 0x507da0. 4 hypotheses tried, 2 confirmed
// fixes kept: (1) writing the zOffset sign check as a genuine if/else
// computing v13 in each branch let the compiler CSE the shared
// gRenderInitTwo[1]*zOffset multiply out after the compare, matching the
// original's compare-then-multiply order (a plain "cache the multiply in a
// local, then branch on a separate bool" version materialized the bool into
// al with extra movs and did not match); (2) flipping the branch to
// `if (zOffset >= 0.0f)` (add gRenderInitOne[0] first) instead of
// `if (zOffset < 0.0f)` matched the original's fall through/jump sense
// exactly, fixing one instruction level diff. The remaining residue starts
// at the print_if_false("invalid zOffset!") call: our build still calls it
// out of line here (register allocation differs enough that edi is not live
// the same way, so the call shape and everything downstream shifts), which
// is the repo wide print_if_false inlining problem CLAUDE.md documents
// under "Matching discipline" (static in export.h, gets inlined at some call
// sites and not others depending on register pressure); fixing that needs a
// real out of line print_if_false, not a change local to this function.
// 4 hypotheses is short of the 10+ per cluster bar for a >1000 byte
// function, logged in pcgfx.attempts.md.
void PCGfx_DrawTPoly2D(
		f32 x0, f32 y0, f32 u0, f32 v0, u32 color0,
		f32 x1, f32 y1, f32 u1, f32 v1, u32 color1,
		f32 x2, f32 y2, f32 u2, f32 v2, u32 color2,
		f32 zOffset)
{
	gPcGfxDrawRelated &= 0xFFFFFFFB;

	if (zOffset <= 6.0f)
		gPcGfxSlotNumber = (i32)zOffset;

	f32 v13;
	if (zOffset >= 0.0f)
		v13 = gRenderInitTwo[1] * zOffset + gRenderInitOne[0];
	else
		v13 = gRenderInitTwo[1] * zOffset + gRenderInitOne[1];

	f32 v24 = v13;
	print_if_false(v24 > 0.0f, "invalid zOffset!");

	f32 v27 = (v24 - gRenderInitOne[0]) / gRenderInitTwo[0];
	f32 v32 = gRenderInitOne[2] / v24;

	SDXPolyField dxPolyFields[3];

	dxPolyFields[0].field_0 = x0;
	dxPolyFields[0].field_4 = y0;
	dxPolyFields[0].field_14 = u0;
	dxPolyFields[0].field_18 = v0;
	dxPolyFields[0].field_10 = color0;
	dxPolyFields[0].field_8 = v27;
	dxPolyFields[0].field_C = v32;

	dxPolyFields[1].field_0 = x1;
	dxPolyFields[1].field_4 = y1;
	dxPolyFields[1].field_14 = u1;
	dxPolyFields[1].field_18 = v1;
	dxPolyFields[1].field_10 = color1;
	dxPolyFields[1].field_8 = v27;
	dxPolyFields[1].field_C = v32;

	dxPolyFields[2].field_0 = x2;
	dxPolyFields[2].field_4 = y2;
	dxPolyFields[2].field_14 = u2;
	dxPolyFields[2].field_18 = v2;
	dxPolyFields[2].field_10 = color2;
	dxPolyFields[2].field_8 = v27;
	dxPolyFields[2].field_C = v32;

	if (gEndSceneRelatedTwo >= 15360)
	{
		gEndSceneRelatedTwo++;
		return;
	}

	DXPOLY *p = &gDxPolys[gEndSceneRelatedTwo++];
	i32 blendMode = gPcGfxBlendModeRelated;
	f32 fogMax = 0.0f;

	if (gLowGraphics)
	{
		p->field_4 = (LPDIRECTDRAWSURFACE7)(i32)gUseTextureRelated;
		*(i32*)&p->mBlendMode = gPcGfxDrawRelated;
		if (gUseTextureRelated < 0)
			*(i32*)&p->mBlendMode = gPcGfxDrawRelated & 0xFFFFFFFB;

		p->field_C = 3;

		for (i32 i = 0; i < 3; i++)
		{
			SDXPolyField *dst = &p->field_10[i];
			memcpy(dst, &dxPolyFields[i], sizeof(SDXPolyField));

			if (!(p->mBlendMode & 4))
				dst->field_4 = 1.0f;

			i32 alpha;
			if (gProcessTextureRelated)
				alpha = 128;
			else
				alpha = (dst->field_10 >> 24) & 0xFF;

			dst->field_10 =
				gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
				gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
				alpha << 24;

			if (dst->field_8 < 0.0f)
			{
				dst->field_8 = 0.0f;
			}
			else if (dst->field_8 > 0.99989998f)
			{
				dst->field_8 = 0.99989998f;
				fogMax = dst->field_8;
			}
			else if (fogMax < dst->field_8)
			{
				fogMax = dst->field_8;
			}
		}
	}
	else
	{
		LPDIRECTDRAWSURFACE7 Direct3DTexture;
		if (gUseTextureRelated < 0)
			Direct3DTexture = 0;
		else
			Direct3DTexture = PCTex_GetDirect3DTexture(gUseTextureRelated);
		p->field_4 = Direct3DTexture;
		p->mBlendMode = gChosenBlendingMode;
		p->field_A = gProcessedTextureFlags;
		p->field_C = 3;

		for (i32 i = 0; i < 3; i++)
		{
			SDXPolyField *dst = &p->field_10[i];
			memcpy(dst, &dxPolyFields[i], sizeof(SDXPolyField));

			i32 alpha;
			if (gProcessTextureRelated)
				alpha = 128;
			else
				alpha = (dst->field_10 >> 24) & 0xFF;

			dst->field_10 =
				gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
				gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
				gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
				alpha << 24;

			if (dst->field_8 < 0.0f)
			{
				dst->field_8 = 0.0f;
			}
			else if (dst->field_8 > 0.99989998f)
			{
				dst->field_8 = 0.99989998f;
				fogMax = dst->field_8;
			}
			else if (fogMax < dst->field_8)
			{
				fogMax = dst->field_8;
			}
		}

		if (gChosenBlendingMode)
		{
			blendMode = 0;
		}
	}

	DXPOLY_DrawPoly(p, gPcGfxSlotNumber, blendMode, fogMax);
	gPcGfxSlotNumber = -1;
}

// @NotOk
// A world space triangle (3x (x,y,z,w,uv,color)) fed through the same fog
// pipeline as PCGfx_ClipSendIndexedVertList (no near plane clip here, just
// straight per vertex processing), then submitPoly(verts,3). Confirmed field
// mapping from the disasm: field_0=x, field_4=y, field_14=w, field_18=uv,
// field_8=(1/z - gRenderInitOne[0])/gRenderInitTwo[0], field_C=gRenderInitOne[2]*z
// (the ORIGINAL z, not 1/z, re-read from the arg after the fdiv already
// consumed it, confirmed for all 3 vertices), field_10=color after the same
// brighten step as ClipSendIndexedVertList ((c>>1&0x7F7F7F)+0x0F0F0F, skipped
// when the low 3 bytes are already 0), then gsub_506D70(1/z,color) when
// gNonRendderSettingE, then field_14/field_18 *= field_C when !gLowGraphics.
// The original INLINES gsub_506D70's real body for vertex 1 (tips.txt's
// inline-cutoff note: first call site inlined, later ones become real calls)
// but calls it out of line for vertex 2 and vertex 3; our gsub_506D70 is a
// forward-to-original stub, not the real fog table math (setupFog's tables
// are out of scope, see pcgfx.attempts.md), so vertex 1's region can not
// match regardless of inlining. cmpsum: 180 mnemonic diffs at 0x5081f0.
// residue: our frame is 0x64 bytes vs the original's 0x60 (one extra local
// slot even with per vertex block scoping to force reuse across the 3
// vertices, attempt 2), and the gPcGfxDrawRelated |= 4 load/store is
// scheduled earlier by our compiler than the original regardless of where
// the statement sits between the verts[] pointer assignments (attempt 1:
// statement placed before verts[]; attempt 2: placed between verts[1] and
// verts[2] assignment, matching the original's apparent position; both
// produced the identical instruction stream). 2 hypotheses tried, below the
// 15+ bar for a medium (850 byte) function, logged in pcgfx.attempts.md.
void PCGfx_DrawTPoly3D(
		f32 x1, f32 y1, f32 z1, f32 w1, f32 uv1, u32 color1,
		f32 x2, f32 y2, f32 z2, f32 w2, f32 uv2, u32 color2,
		f32 x3, f32 y3, f32 z3, f32 w3, f32 uv3, u32 color3)
{
	_DXVERT vtx[3];
	_DXVERT *verts[3];

	verts[0] = &vtx[0];
	verts[1] = &vtx[1];
	gPcGfxDrawRelated |= 4;
	verts[2] = &vtx[2];

	{
		f32 invZ = 1.0f / z1;
		vtx[0].field_0 = x1;
		vtx[0].field_4 = y1;
		vtx[0].field_14 = w1;
		vtx[0].field_18 = uv1;
		vtx[0].field_8 = (invZ - gRenderInitOne[0]) / gRenderInitTwo[0];
		vtx[0].field_C = gRenderInitOne[2] * z1;
		if ((color1 & 0xFFFFFF) == 0)
			vtx[0].field_10 = color1;
		else
			vtx[0].field_10 = (color1 & 0xFF000000) | (((color1 >> 1) & 0x7F7F7Fu) + 0x0F0F0Fu);
		if (gNonRendderSettingE)
			vtx[0].field_10 = gsub_506D70(invZ, vtx[0].field_10);
		if (!gLowGraphics)
		{
			vtx[0].field_14 *= vtx[0].field_C;
			vtx[0].field_18 *= vtx[0].field_C;
		}
	}

	{
		f32 invZ = 1.0f / z2;
		vtx[1].field_0 = x2;
		vtx[1].field_4 = y2;
		vtx[1].field_14 = w2;
		vtx[1].field_18 = uv2;
		vtx[1].field_8 = (invZ - gRenderInitOne[0]) / gRenderInitTwo[0];
		vtx[1].field_C = gRenderInitOne[2] * z2;
		if ((color2 & 0xFFFFFF) == 0)
			vtx[1].field_10 = color2;
		else
			vtx[1].field_10 = (color2 & 0xFF000000) | (((color2 >> 1) & 0x7F7F7Fu) + 0x0F0F0Fu);
		if (gNonRendderSettingE)
			vtx[1].field_10 = gsub_506D70(invZ, vtx[1].field_10);
		if (!gLowGraphics)
		{
			vtx[1].field_14 *= vtx[1].field_C;
			vtx[1].field_18 *= vtx[1].field_C;
		}
	}

	{
		f32 invZ = 1.0f / z3;
		vtx[2].field_0 = x3;
		vtx[2].field_4 = y3;
		vtx[2].field_14 = w3;
		vtx[2].field_18 = uv3;
		vtx[2].field_8 = (invZ - gRenderInitOne[0]) / gRenderInitTwo[0];
		vtx[2].field_C = gRenderInitOne[2] * z3;
		if ((color3 & 0xFFFFFF) == 0)
			vtx[2].field_10 = color3;
		else
			vtx[2].field_10 = (color3 & 0xFF000000) | (((color3 >> 1) & 0x7F7F7Fu) + 0x0F0F0Fu);
		if (gNonRendderSettingE)
			vtx[2].field_10 = gsub_506D70(invZ, vtx[2].field_10);
		if (!gLowGraphics)
		{
			vtx[2].field_14 *= vtx[2].field_C;
			vtx[2].field_18 *= vtx[2].field_C;
		}
	}

	submitPoly(verts, 3);
}

// @NotOk
// Session 2026-08-26: confirmed the two print_if_false calls both check
// drawScale (same string "Improbable draw scale" at 0x54ADDC both times,
// against 0.0 then 256.0, both doubles), and confirmed the split branch
// (else, below) matches the disasm instruction for instruction, including
// the recursive PCGfx_DrawTexture2D(TextureSplitID,...) call shape and the
// x_off/y_off wraparound. What was NOT reproduced this session: the real
// disasm for the single texture branch (0x506637-0x5068da) builds the
// PCGfx_DrawQuad2D call from a viewport clamp (right/bottom edges clamped
// against 0x73C794/0x73C790, read as some viewport max, not yet named) and
// TWO different code paths selected by a bit of a6 (0x5067ad has a pure
// fixed point path using magic constant division by 0x66666667/0x88888889,
// i.e. integer divide by ~2.5 and ~1.8, vs 0x5067ab's float fild/fmul/fdiv
// chain), producing 4 values (probably u0/v0/u1/v1 fractions for partial
// visibility when the rect is clipped by the viewport) that feed the final
// call alongside TextureWScale/TextureHScale. The call below is a
// functional approximation only (untruncated rect, full 0..TextureWScale/
// TextureHScale UV, no viewport clipping), not a translation of that
// clamp/fraction math, so it will not match and may not even be fully
// correct at the clipped edges. Left @NotOk, not iterated against
// compare.py, the real fix needs the viewport clamp fully worked out first.
void PCGfx_DrawTexture2D(
		i32 a1,
		i32 x,
		i32 a3,
		f32 drawScale,
		u32 color,
		u32 a6,
		f32 a7)
{
	print_if_false(drawScale > 0.0f, "Improbable draw scale");
	print_if_false(drawScale < 256.0f, "Improbable draw scale");

	i32 textureWidth;
	i32 textureHeight;

	PCTex_GetTextureSize(a1, &textureWidth, &textureHeight);

	if (textureWidth <= gMaxTextureWidth && textureHeight <= gTextureHeight)
	{
		i32 adjusted_width = ((f32)textureWidth * drawScale);
		i32 adjusted_height = ((f32)textureHeight * drawScale);

		if (adjusted_width && adjusted_height)
		{
			PCGfx_UseTexture(a1,
					a6 & 4 ? DCGfx_BlendingMode_2 : DCGfx_BlendingMode_0);

			f32 TextureWScale = PCTex_GetTextureWScale(a1);
			f32 TextureHScale = PCTex_GetTextureHScale(a1);

			i32 v36 = x;
			i32 hateThiShit = a3;

			if (x < gDrawTexture2DRelatedOne)
			{
				v36 = gDrawTexture2DRelatedOne;
			}


			if (a3 < gDrawTexture2DRelatedTwo)
			{
				hateThiShit = gDrawTexture2DRelatedTwo;
			}




			i32 v25;
			if (a6 & 2)
			{
				v25 = PCGfx_GetZLayerFurthest();
				PCGfx_IncZLayerFurthest();
			}
			else if (a6 & 8)
			{
				v25 = PCGfx_GetZLayerNearest();
				PCGfx_IncZLayerNearest();
			}
			else
			{
				v25 = a7;
			}



			PCGfx_DrawQuad2D(
					(f32)v36,
					(f32)hateThiShit,
					(f32)adjusted_width,
					(f32)adjusted_height,
					0.0f,
					0.0f,
					TextureWScale,
					TextureHScale,
					color,
					(f32)v25,
					0);
		}
	}
	else
	{
		i32 x_off = x;
		i32 y_off = a3;

		print_if_false(x_off == 0, "Split texture drawn with x != 0");
		i32 splitCount = PCTex_GetTextureSplitCount(a1);

		for (i32 i = 0; i < splitCount; i++)
		{
			i32 TextureSplitID = PCTex_GetTextureSplitID(a1, i);

			i32 cur_width;
			i32 cur_height;
			PCTex_GetTextureSize(TextureSplitID, &cur_width, &cur_height);
			PCGfx_DrawTexture2D(TextureSplitID, x_off, y_off, drawScale, color, a6, a7);

			x_off += cur_width;
			if (x_off >= textureWidth)
			{
				x_off = 0;
				y_off = cur_height;
			}
		}
	}
}

// @Ok
// @Matching
// @Note powerpc has fps counter here and fog level
INLINE void PCGfx_EndScene(i32 a1)
{
	if (gSceneRelated)
	{
		DXPOLY_EndScene(a1 != 0);
		PCGfx_ProcessTexture(0, -1, DCGfx_BlendingMode_0);
		gEndSceneRelated = -1;
		gSceneRelated = 0;
		gEndSceneRelatedTwo = 0;
	}
}

// @Ok
// @Matching
void PCGfx_Exit(void)
{
	PCGfx_ProcessTexture(0, -1, DCGfx_BlendingMode_0);
	PCTex_ReleaseAllTextures();
	print_if_false(PCTex_CountActiveTextures() == 0, "some textures still allocated!");
}

// @Ok
f32 PCGfx_GetZLayerFurthest(void)
{
	if ( !gSceneRelated )
		PCGfx_BeginScene(3, -1);

	return gZLayerFurthest;
}

// @Ok
f32 PCGfx_GetZLayerNearest(void)
{
	if ( !gSceneRelated )
		PCGfx_BeginScene(3, -1);

	return gZLayerNearest;
}

// @Ok
void PCGfx_IncZLayerFurthest(void)
{
	gZLayerFurthest -= 10.0f;
}

// @Ok
void PCGfx_IncZLayerNearest(void)
{
	gZLayerNearest += 0.001f;
}

// @Ok
void PCGfx_InitAtStart(void)
{
	DXPOLY_SetOutlineColor(0xFF00FF00);
	DXPOLY_SetHUDOffset(7);
	PCGfx_ProcessTexture(0, -1, DCGfx_BlendingMode_0);
	PCGfx_SetBrightness(gBrightnessRelated);
}

// @Ok
// @Matching
u8 PCGfx_IsInScene(void)
{
	return gSceneRelated;
}

EXPORT i32 gBlendingModes[DCGfx_BlendingMode_MAX + 1] =
{
	0, 1, 2, 3, 4
};

// @NotOk
// residue: 6 mnemonic diffs at 0x5062c0, all one spot: original computes
// (gChosenBlendingMode-1)<<4 as "add ecx,-1; shl ecx,4" (subtract-then-shift
// in that instruction order), ours (and 2 rewrites of the same expression)
// compile to "shl ecx,4; sub ecx,0x10" (MSVC distributes the shift over the
// subtraction either way). 3 hypotheses tried, logged in pcgfx.attempts.md.
void PCGfx_ProcessTexture(
		_tagKMSTRIPHEAD *,
		i32 a2,
		DCGfx_BlendingMode a3)
{
	i32 TextureFlags = PCTex_GetTextureFlags(a2);
	gProcessTextureRelated = (TextureFlags & 0x1000) != 0 && (a3 == DCGfx_BlendingMode_0 || a3 == DCGfx_BlendingMode_1);

	i32 curBlendingMode;
	if (a3 || a2 < 0 || !PCTex_TextureHasAlpha(a2))
	{
		curBlendingMode = gBlendingModes[a3];
	}
	else
	{
		curBlendingMode = gProcessTextureRelated ? 1 : 5;
	}

	gChosenBlendingMode = curBlendingMode;
	gNonRendderSettingE = (TextureFlags & 0x20) == 0 ? gIsRenderSettingE : 0;

	if (gLowGraphics)
	{
		gPcGfxDrawRelated &= 0xFFFFFF85;

		if (a2 >= 0)
		{
			gPcGfxDrawRelated |= 2;

			if (PCTex_TextureHasAlpha(a2))
				gPcGfxDrawRelated |= 0x40;
		}

		if (gChosenBlendingMode >= 1 && gChosenBlendingMode <= 4)
		{
			gPcGfxDrawRelated |= ((gChosenBlendingMode - 1) << 4) | 8;
		}
	}
	else if (a2 < 0)
	{
		DXPOLY_SetTexture(0);
		gProcessedTextureFlags = 0;
	}
	else
	{
		IDirectDrawSurface7* Direct3DTexture = PCTex_GetDirect3DTexture(a2);
		DXPOLY_SetTexture(Direct3DTexture);
		gProcessedTextureFlags = 0;

		if (PCTex_TextureHasAlpha(a2))
			gProcessedTextureFlags |= 8;

		i32 v9 = PCTex_GetTextureFlags(a2);
		if ((v9 & 8) != 0)
			gProcessedTextureFlags |= 0x10;

		if ((v9 & 0x20) != 0)
			gProcessedTextureFlags |= 1;

		if ((v9 & 2) == 0)
			gProcessedTextureFlags |= 2;

		if ((v9 & 4) == 0)
			gProcessedTextureFlags |= 4;
	}
}

// @Ok
// @Matching
void PCGfx_RenderInit(f32 a1, f32 a2, f32 a3)
{
	gRenderInitOne[2] = a3;
	gRenderInitOne[0] = a1;
	gRenderInitOne[1] = a2;
	gRenderInitTwo[0] = a2 - a1;
	gRenderInitTwo[1] = gRenderInitTwo[0] / 4096.0f;
}

// @NotOk
// third parameter of rendesetup seems to be useless but also DB related
void PCGfx_RenderModelPreview(
		void* a1,
		char const* a2,
		i32 a3)
{
	char v3[128];

	M3dMaths_RotMatrixYXZ(&gMikeCamera[0].Angles, &gMikeCamera[0].Transform);
	TransMatrix(&gMikeCamera[0].Transform, &gMikeCamera[0].Position);
	PCGfx_BeginScene(1u, -1);

	// @FIXME: third param seems to be ignored
	M3d_RenderSetup(&gMikeCamera[0], &gViewport, 0);
	M3d_Render(a1);
	M3d_RenderCleanup();
	Mess_SetSort(4095);
	PShell_SmallFont();
	Mess_SetRGB(0xFFu, 0xFFu, 0xFFu, 0);
	Mess_SetRGBBottom(0xFFu, 255, 255);
	Mess_SetShadowRGB(0xFFu);
	Mess_SetTextJustify(1);

	sprintf(v3, "PSX: %s", a2);
	Mess_DrawText(20, 20, v3, 0, 0x1000u);
	sprintf(v3, "IDX: %i", a3);
	Mess_DrawText(220, 20, v3, 0, 0x1000u);
	sprintf(v3, "CAM: %i %i %i", gMikeCamera[0].Position.vx, gMikeCamera[0].Position.vy, gMikeCamera[0].Position.vz);
	Mess_DrawText(20, 45, v3, 0, 0x1000u);

	CItem* pItem = static_cast<CItem*>(a1);
	// @FIXME
	sprintf(
		v3,
		"ITM: shit");
	/*
	sprintf(
		v3,
		"ITM: %i %i %i",
		pItem->mPos.vx >> 12,
		pItem->mPos.vy >> 12,
		pItem->mPos.vz >> 12);
		*/
	Mess_DrawText(220, 45, v3, 0, 0x1000u);

	PCGfx_EndScene(1);
}

EXPORT f32 gPcGfxBrightnessPower[8] =
{
	0.80000001f,
	0.85000002f,
	0.89999998f,
	0.94999999f,
	1.0f,
	1.05f,
	1.1f,
	1.15f
};

// @Ok
// @Test
INLINE void PCGfx_SetBrightness(i32 a1)
{
	f32 v8 = 1.0f / gPcGfxBrightnessPower[a1];
	for (i32 i = 0; i < 256; i++)
	{

		f32 v9 = i;
		f32 v6 = v9 / 255.0f;
		f32 v7 = pow(v6, v8);

		i32 v3 = (v7 * 255.0f + 0.5f);
		if (v3 >= 128)
		{
			v3 = 255;
		}
		else
		{
			v3 *= 2;
		}

		gPcGfxBrightnessValues[i] = v3;
	}
}

// @Ok
// @Matching
void PCGfx_SetFogParams(
		f32 a1,
		f32 a2,
		u32 a3)
{
	u32 three = a3;
	if ((a3 & 0xFF000000) == 0x80000000)
	{
		three = 0xFFFFFF & a3;
		a1 = a2 * 0.88999999f;
	}

	gFlFoggingParamOne = a1;
	gFlFoggingParamTwo = a2;
	gU32FoggingParamThree = three;
	gBFoggingRelated = 1;
}

u32 gDepthCompareValues[DCGfx_RenderSetting_7 + 1] =
{
	0, 2, 3, 4, 5, 6, 7, 8,
};

u32 gFilterModeValues[DCGfx_RenderSetting_d - DCGfx_RenderSetting_a + 1] =
{
	0, 1 ,1 ,1
};

// @Ok
// @Matching
void PCGfx_SetRenderParameter(
		DCGfx_RenderParameter a1,
		DCGfx_RenderSetting a2)
{
	if ( !gLowGraphics )
	{
		switch ( a1 )
		{
			case DCGfx_RenderParameter_0:
				print_if_false(a2 >= DCGfx_RenderSetting_0, "Invalid render setting.");
				print_if_false(a2 <= DCGfx_RenderSetting_7, "Invalid render setting.");
				DXPOLY_SetDepthCompare(gDepthCompareValues[a2]);
				break;
			case DCGfx_RenderParameter_1:
				print_if_false(a2 >= DCGfx_RenderSetting_8, "Invalid render setting.");
				print_if_false(a2 <= DCGfx_RenderSetting_9, "Invalid render setting.");
				DXPOLY_SetDepthWriting(a2 == DCGfx_RenderSetting_8);
				break;
			case DCGfx_RenderParameter_2:
				break;
			case DCGfx_RenderParameter_3:
				print_if_false(a2 >= DCGfx_RenderSetting_a, "Invalid render setting.");
				print_if_false(a2 <= DCGfx_RenderSetting_d, "Invalid render setting.");
				DXPOLY_SetFilterMode(gFilterModeValues[a2-DCGfx_RenderSetting_a]);
				break;
			case DCGfx_RenderParameter_4:
				gIsRenderSettingE = a2 == DCGfx_RenderSetting_e;
				if (!gIsRenderSettingE)
					gNonRendderSettingE = gIsRenderSettingE;
				break;
			default:
				print_if_false(0, "Invalid render parameter.");
				break;
		}
	}
}

// @Ok
INLINE void PCGfx_SetSkyColor(u32 a1)
{
	gPcGfxSkyColor = a1;
	DXPOLY_SetBackgroundColor(a1 | 0xFF000000);
}

// @Ok
// @Matching
INLINE void PCGfx_UseTexture(i32 a1, DCGfx_BlendingMode a2)
{
	i32 v2 = a1;
	if ( a1 <= 2 )
	{
		v2 = -1;
		gNonRendderSettingE = gIsRenderSettingE;
	}
	if ( gUseTextureRelated != v2 || gTextureBlendingMode != a2 )
	{
		gUseTextureRelated = v2;
		gTextureBlendingMode = a2;
		if (!gSceneRelated)
		{
			PCGfx_BeginScene(3u, -1);
		}
		PCGfx_ProcessTexture(0, v2, a2);
	}
}

// @NotOk
// Naming work done this session (nearest neighbor check against
// idb_globals.txt, see pcgfx.attempts.md): every global this function
// touches turned out to already be a named repo global at a different
// address than we thought: 0xAC08E0-style constants were not involved here,
// instead 0x568158/0x628614=gGameResolutionY/Yres, 0x568154/0x61B5FC=
// gGameResolutionX/Xres (m3dinit.h), 0xAC08DC=gTextureBlendingMode,
// 0xAC08C4=gSceneRelated, 0x56815C=gIsRenderSettingE, 0xADB3A8/0xADB3AC=
// gMaxTextureWidth/gTextureHeight all matched 1:1. Call targets identified
// the same way: 0x50F0E0=PCTex_GetTextureSize (real call, not inlined,
// matches its 3 arg shape), 0x510170/0x510190=PCTex_GetTextureSplitCount/
// PCTex_GetTextureSplitID (same pair PCGfx_DrawTexture2D already uses),
// 0x52A227/0x52A3C0=my_malloc/my_free (main.cpp already PATCH_PUSH_RETs
// these two exact addresses). The malloc size is splitCount * 44, and 44 is
// exactly sizeof(Texture), so the split path allocates a Texture[splitCount]
// and recurses into itself once per piece, the same shape as
// PCGfx_DrawTexture2D's split loop (same assert string "Split texture drawn
// with x != 0." at 0x568348, confirmed from the binary). The kind <= 2 /
// gUseTextureRelated / gTextureBlendingMode block at the top of the single
// texture path is PCGfx_UseTexture(kind, DCGfx_BlendingMode_0) inlined
// (matches PCGfx_UseTexture's body instruction for instruction); called
// here instead of reproducing the inline, since PCGfx_UseTexture is already
// @Ok.
// NOT resolved: the exact mapping of a3/a4/a5/a6 into the 4 float values
// built for the PCGfx_DrawQuad2D call (position vs size roles are a guess),
// and a10's value in that call (passed through some local we could not
// pin down). This is a genuine attempt, not a stub, but the coordinate
// math in both the single texture and split branches is unverified.
// cmpsum: 186 mnemonic diffs at 0x509d20, first divergence right at entry.
void PCPanel_DrawTexturedPoly(f32 scale, Texture const *tex, i32 a3, i32 a4, i32 a5, i32 a6, u8 tint)
{
	print_if_false(tex != 0, "no texture for draw texture poly.");

	u16 kind = tex->clut;
	i32 width, height;
	PCTex_GetTextureSize(kind, &width, &height);

	if (width <= gMaxTextureWidth && height <= gTextureHeight)
	{
		PCGfx_UseTexture(kind, DCGfx_BlendingMode_0);

		f32 scaleY = gGameResolutionY / (f32)Yres;
		f32 scaleX = gGameResolutionX / (f32)Xres;

		u32 t = tint;
		u32 color = 0xFF000000u | (t << 16) | (t << 8) | t;

		f32 y = (f32)a4 * scaleY;
		f32 x = (f32)a3 * scaleX;
		f32 w = (f32)a5 * scale;
		f32 h = w * (f32)a6;

		PCGfx_DrawQuad2D(h, w, x, y, 0.0f, 0.0f, 1.0f, 1.0f, color, 1.0f, 0);
	}
	else
	{
		i32 splitCount = PCTex_GetTextureSplitCount(kind);
		Texture *pieces = (Texture *)my_malloc(splitCount * sizeof(Texture));

		print_if_false(a3 == 0, "Split texture drawn with x != 0.");

		f32 scaleY = gGameResolutionY / (f32)Yres;
		f32 scaleX = gGameResolutionX / (f32)Xres;

		i32 xAccum = 0;
		i32 yAccum = a4;

		for (i32 i = 0; i < splitCount; i++)
		{
			i32 splitId = PCTex_GetTextureSplitID(kind, i);
			pieces[i].clut = (u16)splitId;

			i32 subWidth, subHeight;
			PCTex_GetTextureSize(splitId, &subWidth, &subHeight);

			i32 subScaleW = (i32)((f32)subWidth / (f32)width * scaleX);
			i32 subScaleH = (i32)((f32)subHeight / (f32)height * scaleY);

			PCPanel_DrawTexturedPoly(scale, &pieces[i], xAccum, yAccum, subScaleW, subScaleH, tint);

			xAccum += subWidth;
			if (xAccum >= width)
			{
				yAccum += subHeight;
				xAccum = 0;
			}
		}

		my_free(pieces);
	}
}

// @NotOk
// interpolates a new vertex where the b->c edge crosses the plane field_8 == t
// residue: original uses a 0x10 byte local frame and different FPU stack
// scheduling around the field_8 store and the bx/by/cx/cy multiplies (101
// mnemonic diffs at 0x506f90 as of this attempt, 2 hypotheses tried so far,
// logged in pcgfx.attempts.md). functionally faithful.
void ZCLIP_VERT(_DXVERT *out, _DXVERT *b, _DXVERT *c, f32 t)
{
	print_if_false(c->field_8 != b->field_8, "Zero denominator computing scale!");
	print_if_false(t != 0.0f, "Zero denominator computing clip inverse!");

	f32 frac = (t - b->field_8) / (c->field_8 - b->field_8);
	out->field_8 = t;

	f32 invT = 1.0f / t;

	f32 bx = b->field_0 * b->field_8;
	f32 by = b->field_4 * b->field_8;
	f32 cx = c->field_0 * c->field_8;
	f32 cy = c->field_4 * c->field_8;

	out->field_C = gRenderInitOne[2] * invT;
	out->field_0 = ((cx - bx) * frac + bx) * invT;
	out->field_4 = ((cy - by) * frac + by) * invT;

	out->field_14 = (c->field_14 - b->field_14) * frac + b->field_14;
	out->field_18 = (c->field_18 - b->field_18) * frac + b->field_18;

	u32 colorB = b->field_10;
	u32 colorC = c->field_10;

	i32 chB_A = (colorB >> 24) & 0xFF;
	i32 chC_A = (colorC >> 24);
	i32 newA = (i32)((f32)(chC_A - chB_A) * frac) + chB_A;

	i32 chB_R = (colorB >> 16) & 0xFF;
	i32 chC_R = (colorC >> 16) & 0xFF;
	i32 newR = (i32)((f32)(chC_R - chB_R) * frac) + chB_R;

	i32 chB_G = (colorB >> 8) & 0xFF;
	i32 chC_G = (colorC >> 8) & 0xFF;
	i32 newG = (i32)((f32)(chC_G - chB_G) * frac) + chB_G;

	i32 chB_B = colorB & 0xFF;
	i32 chC_B = colorC & 0xFF;
	i32 newB = (i32)((f32)(chC_B - chB_B) * frac) + chB_B;

	out->field_10 = ((newA & 0xFF) << 24) | ((newR & 0xFF) << 16) | ((newG & 0xFF) << 8) | (newB & 0xFF);
}

// @Ok
// @AlmostMatching: CVector and CSVector assingment is different, goddamn I don't understand
CSuper* createSuperItem(CItem *pItem)
{
	CSuper *pSuper = new CSuper();

	pSuper->mFlags |= pItem->mFlags;
	pSuper->mInquiry = pItem->mInquiry;

	pSuper->mPos = pItem->mPos;

	pSuper->mAngles = pItem->mAngles;

	pSuper->mModel = pItem->mModel;

	pSuper->mDummyFrame = pItem->mDummyFrame;
	pSuper->mTintIndex = pItem->mTintIndex;
	pSuper->mDummyAnim = pItem->mDummyAnim;

	pSuper->mRegion = pItem->mRegion;
	pSuper->mNextItem = pItem->mNextItem;
	pSuper->mRGB = pItem->mRGB;

	pSuper->mScale = pItem->mScale;

	pSuper->mTRN = pItem->mTRN;
	pSuper->mPreviousItem = pItem->mPreviousItem;
	pSuper->mType = pItem->mType;
	pSuper->mpLight = pItem->mpLight;

	pSuper->field_128 = 0;
	pSuper->mAnim = 0;

	return pSuper;
}

// @MEDIUMTODO
void setupFog(void)
{
    printf("setupFog(void)");
}

// @NotOk
// gLowGraphics true/false take almost the same shape but are not shared code
// (mBlendMode/field_A/field_C are sourced differently, field_C is a hardcoded
// 4 on the low graphics side but the real vertex count otherwise); not
// verified against compare.py yet beyond a first pass, residue logged in
// pcgfx.attempts.md.
void submitPoly(_DXVERT **verts, i32 count)
{
	i32 idx = gEndSceneRelatedTwo;
	if (idx >= 15360)
	{
		gEndSceneRelatedTwo = idx + 1;
		return;
	}

	DXPOLY *p = &gDxPolys[idx];
	gEndSceneRelatedTwo = idx + 1;
	i32 blendMode = gPcGfxBlendModeRelated;
	f32 fogMax = 0.0f;

	if (gLowGraphics)
	{
		p->field_4 = (LPDIRECTDRAWSURFACE7)(i32)gUseTextureRelated;
		*(i32*)&p->mBlendMode = gPcGfxDrawRelated;
		if (gUseTextureRelated < 0)
			*(i32*)&p->mBlendMode = gPcGfxDrawRelated & 0xFFFFFFFB;

		p->field_C = 4;

		if (count > 0)
		{
			for (i32 i = 0; i < count; i++)
			{
				SDXPolyField *dst = &p->field_10[i];
				memcpy(dst, verts[i], sizeof(SDXPolyField));

				if (!(p->mBlendMode & 4))
					dst->field_4 = 1.0f;

				i32 v23;
				if (gProcessTextureRelated)
					v23 = 128;
				else
					v23 = (dst->field_10 >> 24) & 0xFF;

				dst->field_10 =
					gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
					gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
					gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
					v23 << 24;

				if (dst->field_8 < 0.0f)
				{
					dst->field_8 = 0.0f;
				}
				else if (dst->field_8 > 0.99989998f)
				{
					dst->field_8 = 0.99989998f;
					fogMax = dst->field_8;
				}
				else if (fogMax < dst->field_8)
				{
					fogMax = dst->field_8;
				}
			}
		}
	}
	else
	{
		LPDIRECTDRAWSURFACE7 Direct3DTexture;
		if (gUseTextureRelated < 0)
			Direct3DTexture = 0;
		else
			Direct3DTexture = PCTex_GetDirect3DTexture(gUseTextureRelated);
		p->field_4 = Direct3DTexture;
		p->mBlendMode = gChosenBlendingMode;
		p->field_A = gProcessedTextureFlags;
		p->field_C = count;

		if (count > 0)
		{
			for (i32 i = 0; i < count; i++)
			{
				SDXPolyField *dst = &p->field_10[i];
				memcpy(dst, verts[i], sizeof(SDXPolyField));

				if (!(p->mBlendMode & 4))
					dst->field_4 = 1.0f;

				i32 v23;
				if (gProcessTextureRelated)
					v23 = 128;
				else
					v23 = (dst->field_10 >> 24) & 0xFF;

				dst->field_10 =
					gPcGfxBrightnessValues[dst->field_10 & 0xFF] |
					gPcGfxBrightnessValues[(dst->field_10 >> 8) & 0xFF] << 8 |
					gPcGfxBrightnessValues[(dst->field_10 >> 16) & 0xFF] << 16 |
					v23 << 24;

				if (dst->field_8 < 0.0f)
				{
					dst->field_8 = 0.0f;
				}
				else if (dst->field_8 > 0.99989998f)
				{
					dst->field_8 = 0.99989998f;
					fogMax = dst->field_8;
				}
				else if (fogMax < dst->field_8)
				{
					fogMax = dst->field_8;
				}
			}
		}

		if (gChosenBlendingMode)
		{
			blendMode = 0;
		}
	}

	DXPOLY_DrawPoly(p, gPcGfxSlotNumber, blendMode, fogMax);
	gPcGfxSlotNumber = -1;
}

// @Ok
i32 STDCALL kmSetPALEXTCallback(void*, i32)
{
	return 120;
}

// @Ok
i32 STDCALL kmSetDisplayMode(i32, i32, i32, i32)
{
	return 120;
}

// @Ok
i32 STDCALL kmInitDevice(i32)
{
	return 0;
}

// @Ok
i32 STDCALL kmSetWaitVsyncCount(i32)
{
	return 120;
}

// @Ok
i32 STDCALL kmUnloadDevice(void)
{
	return 120;
}
