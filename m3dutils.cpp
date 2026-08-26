#include "m3dutils.h"
#include "validate.h"

// @Ok
// @AlmostMatching: loop2's redundant numLinks>0 re-test folds differently than
// the original (je vs jle branch, missing jump-into-loop-middle optimization,
// reordered found/not-found tail store). 17 hypotheses tried (see
// m3dutils.attempts.md), residue is 17 mnemonic diffs, instruction count matches.
void M3dUtils_ReadLinksPacket(CSuper *a1, void *a2)
{
	i32 numLinks;
	i32 v4;
	i32 i;
	i32 offset;

	numLinks = *reinterpret_cast<u16*>(reinterpret_cast<char*>(a2) + 2);
	a1->mLinkData = reinterpret_cast<char*>(a2) + 4;

	v4 = word_6B2478[34 * a1->mRegion];
	i32 size1 = 24 * v4;
	i32 size2 = 12 * numLinks;
	a1->field_184 = DCMem_New(size1, 0, 1, 0, 1);
	a1->field_188 = DCMem_New(size2, 0, 1, 0, 1);

	if (numLinks > 0)
	{
		offset = 0;
		i = numLinks;
		do
		{
			offset += 0xC;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-1] = 0;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-2] = 0;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-3] = 0;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-4] = 0;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-5] = 0;
			reinterpret_cast<i16*>(reinterpret_cast<char*>(a1->field_188) + offset)[-6] = 0;
			i--;
		} while (i != 0);
	}

	if (numLinks != 0)
	{
		offset = 0;
		i = numLinks;
		do
		{
			char *link = reinterpret_cast<char*>(a1->mLinkData);
			u16 wanted = *reinterpret_cast<u16*>(link + offset + 2);

			i32 j;
			for (j = 0; j < numLinks; j++)
			{
				if (wanted == *reinterpret_cast<u16*>(link + j * 0xC))
					break;
			}

			if (j != numLinks)
			{
				*reinterpret_cast<u16*>(link + offset + 0xA) = j;
			}
			else
			{
				*reinterpret_cast<u16*>(reinterpret_cast<char*>(a1->mLinkData) + offset + 0xA) = 0xFFFF;
			}

			offset += 0xC;
			i--;
		} while (i != 0);
	}
}

// @NotOk
// Revisit and fix globals
void M3dUtils_InBetween(CSuper *a1)
{
	u16 v1; // cx
	i32 v2; // ebp
	i32 v3; // edi
	i32 v4; // si

	v1 = a1->mAnim;
	v2 = Animations[17 * a1->mRegion];
	v3 = (*(unsigned int *)(v2 + 8 * v1 + 8) >> 16) + 1;
	if (v3 != 1)
	{
		v4 = 0;
		v4 = word_6B2478[34 * a1->mRegion];
		print_if_false(v4 <= 0x1E, "Too many parts for TweenBuffer");
		M3dUtils_InterpolateVectors(
				4 * v4,
				v3,
				reinterpret_cast<u32*>(v2),
				a1,
				0,
				v4);
	}
}

// @BIGTODO
void M3dUtils_BuildPose(CSuper*)
{
	printf("void M3dUtils_BuildPose(CSuper*)");
}


// @BIGTODO
void M3dUtils_InterpolateVectors(i32, i32, u32*, CItem*, i32, i32)
{
	printf("void M3dUtils_InterpolateVectors(int, int, unsigned int*, CItem*, int, int)");
}

// @BIGTODO
void M3dUtils_GetHookPosition(VECTOR*, CSuper*, i32)
{
	printf("void M3dUtils_GetHookPosition(VECTOR*, CSuper*, int)");
}

// @BIGTODO
void M3dUtils_GetDynamicHookPosition(VECTOR*, CSuper*, SHook*)
{
	printf("void M3dUtils_GetDynamicHookPosition(VECTOR*, CSuper*, SHook*)");
}

// @Ok
// @Matching
void M3dUtils_ReadHooksPacket(CSuper *a1, void *a2)
{
	Animations[17 * a1->mRegion + 2] = reinterpret_cast<i32>(reinterpret_cast<char*>(a2) + 4);
}

void validate_SHook(void)
{
	VALIDATE_SIZE(SHook, 0x8);

	VALIDATE(SHook, Part, 0x0);
	VALIDATE(SHook, Offset, 0x6);
}
