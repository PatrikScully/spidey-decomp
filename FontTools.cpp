#include "FontTools.h"
#include "validate.h"
#include "pal.h"
#include "mem.h"
#include "ps2pad.h"
#include "utils.h"
#include "PCTex.h"
#include "dcfileio.h"
#include "my_debug.h"

#include <cstring>
#include "my_assert.h"

// @Ok
Font* FontManager::FontTab[NUM_FONTS_TAB];
// FontManager::LoadFont is not hooked (patch_FontTools only hooks
// GetFontName/AllShadowOff/AllShadowOn/GetFont/Font::Font), so the array
// still gets filled by the original game code at game memory. The macro
// must target game memory here, not the repo array, or GetFont (hooked)
// always searches an empty array and returns null (crashed
// Mess_SetCurrentFont, null Font* deref in G_MESS_FONT = *pFont).
//#define G_FONT_TAB (FontManager::FontTab)
#define G_FONT_TAB (reinterpret_cast<Font**>(0x005FAD5C))

// @Ok
// @Matching
void Font::handleEscapeChar(char c)
{
	i32 code = this->isEscapeChar(c);
	print_if_false(code != 0, "Not an escape code");

	if (code == 250)
	{
	}
	else if (code == 254)
	{
		this->mRed = this->mSavedRed;
		this->mGreen = this->mSavedGreen;
		this->mBlue = this->mSavedBlue;
	}
	else if (code == 253)
	{
		this->mSavedRed = this->mRed;
		this->mSavedGreen = this->mGreen;
		this->mSavedBlue = this->mBlue;
		this->mRed = 0;
		this->mGreen = 128;
		this->mBlue = 255;
	}
	else if (code == 252)
	{
		this->mSavedRed = this->mRed;
		this->mSavedGreen = this->mGreen;
		this->mSavedBlue = this->mBlue;
		this->mRed = 255;
		this->mGreen = 128;
		this->mBlue = 0;
	}
	else if (code == 251)
	{
		this->mSavedRed = this->mRed;
		this->mSavedGreen = this->mGreen;
		this->mSavedBlue = this->mBlue;
		this->mRed = 64;
		this->mGreen = 64;
		this->mBlue = 64;
	}
	else if (code == 249)
	{
		this->mSavedRed = this->mRed;
		this->mSavedGreen = this->mGreen;
		this->mSavedBlue = this->mBlue;
		this->mRed = 128;
		this->mGreen = 64;
		this->mBlue = 64;
	}
	else if (code == 248)
	{
		this->mSavedRed = this->mRed;
		this->mSavedGreen = this->mGreen;
		this->mSavedBlue = this->mBlue;
		this->mRed = 64;
		this->mGreen = 128;
		this->mBlue = 64;
	}
}

// @Ok
i32 Font::fixedCharWidth(char c)
{
	if (this->isEscapeChar(c))
		return 0;

	if (this->field_58 != 0 && this->field_58 != 2)
	{
		if (this->field_58 == 1)
		{
			if (c == ':')
			{
				return this->field_34 * (this->field_C + this->pCharTab[10].W) >> 12;
			}
			
			return this->field_34 * (this->field_C + this->pCharTab->W) >> 12;
		}
		else
		{
			print_if_false(0, "Unrecognized char mapping");
			return 0;
		}
	}
	else
	{
		return (this->field_34 * (this->field_C + this->pCharTab->W)) >> 12;
	}
}

// @Ok
// @Matching
Font::Font(void)
{
	this->Clut = 0xFFFFFFFF;
}

// @Ok
Font::Font(
		u8* a2,
		const char* a3)
{
	strcpy(this->field_38, a3);
	this->field_8 = 3;
	this->field_10 = 3;
	this->field_4 = 0;
	this->mRed = 128;
	this->mGreen = 128;
	this->mBlue = 128;
	this->field_C = 2;
	this->field_20 = 0;
	this->field_21 = 1;
	this->field_24 = 1;
	this->field_28 = 1;
	this->field_2C = 255;
	this->field_30 = 0;
	this->field_34 = 3800;
	this->field_54 = 0;
	this->NumChars = *reinterpret_cast<i32*>(a2);

	this->SetCharMap(0);

	this->pCharTab = static_cast<FontCharacter*>(
			DCMem_New(sizeof(FontCharacter) * (this->NumChars + 1), 0, 1, 0, 1));

	i32 v26 = Pal16X;
	i32 v6 = Pal16Y + GetFree16Slot();
	u16* Clut = PCTex_CreateClut(16);

	for (i32 i = 0; i < 16; i++)
	{
		u16* pColor = reinterpret_cast<u16*>(a2);
		u16 color = pColor[8 * this->NumChars + 2 + i];
		Clut[i] = color;
		gSlicedImageRelated[i] = color;
	}

	_LoadImage();

	this->Clut = GetClut(v26, v6);

	SDataGlyph* pGlyph = reinterpret_cast<SDataGlyph*>(&a2[4]);
	void* pData = &a2[16 * this->NumChars + 36];

	for (i32 j = 0; j < this->NumChars; j++)
	{
		this->pCharTab[j].W = pGlyph[j].mWidth;
		this->pCharTab[j].H = pGlyph[j].mHeight;
		this->pCharTab[j].Baseline = pGlyph[j].mBaseline;

		i32 charSliceWidth = pGlyph[j].mSliceWidth;

		this->pCharTab[j].pImage = new SlicedImage2(
				pData,
				4 * charSliceWidth,
				this->pCharTab[j].H,
				0,
				0,
				4,
				this->Clut,
				1);

		this->pCharTab[j].pImage->pack();
		this->pCharTab[j].pImage->removeFromMemory();

		this->pCharTab[j].pImage->field_A = 0;
		this->pCharTab[j].pImage->field_B = 0;
		this->pCharTab[j].pImage->Shaded = 1;

		i32 v17 = 0;
		i32 size = charSliceWidth * this->pCharTab[j].H;
		if (size & 1)
			v17 = 2;

		pData = reinterpret_cast<void*>(static_cast<u8*>(pData) + size * 2 + v17);
	}

	this->pCharTab[this->NumChars].W = pGlyph[0].mWidth;
	this->pCharTab[this->NumChars].H = pGlyph[0].mHeight;
	this->pCharTab[this->NumChars].Baseline = pGlyph[0].mBaseline;

	i32 sliceWidth = pGlyph[0].mSliceWidth;
	i32 v20 = 2 * sliceWidth * this->pCharTab[this->NumChars].H;
	void* v21 = DCMem_New(v20, 0, 1, 0, 1);
	if ( v20 > 0 )
		memset(v21, 0x12u, v20);


	this->pCharTab[this->NumChars].pImage = new SlicedImage2(
			v21,
			4 * sliceWidth,
			this->pCharTab[this->NumChars].H,
			0,
			0,
			4,
			this->Clut,
			1);
	Mem_Delete(v21);


	this->pCharTab[this->NumChars].pImage->pack();
	this->pCharTab[this->NumChars].pImage->removeFromMemory();

	this->pCharTab[this->NumChars].pImage->field_A = 0;
	this->pCharTab[this->NumChars].pImage->field_B = 0;
	this->pCharTab[this->NumChars].pImage->Shaded = 1;
}

// @Ok
Font::~Font(void)
{
}

// @MEDIUMTODO
void Font::draw(
		i32 x,
		i32 y,
		const char* pStr,
		i32 drawFirst,
		f32 last)
{
	typedef void (FASTCALL *func_ptr)(Font*, void*, i32, i32, const char*, i32, f32);
	func_ptr func = (func_ptr)0x0043E4C0;

	func(this, 0, x, y, pStr, drawFirst, last);
}

// @Ok
int Font::isEscapeChar(char a1)
{
	if (a1 == ']')
		return 254;
	else if (a1 == '[')
		return 253;
	else if (a1 == '}')
		return 254;
	else if (a1 == '{')
		return 252;
	else if (a1 == '>')
		return 254;
	else if (a1 == '<')
		return 251;
	else if (a1 == '|')
		return 249;
	else if (a1 == '~')
		return 248;
	else if (a1 == '^')
		return 250;

	return 0;
}

// @Ok
int INLINE Font::GetCharMap(void)
{
	return this->field_58;
}


// @Ok
INLINE void Font::SetCharMap(int a2)
{
	this->field_58 = a2;
	for (int i = 0; i < 256; i++)
	{
		this->field_5F[i] = this->getCharIndex(i);
	}
}

// @Ok
// @Matching
char Font::getCharIndex(char c)
{
	if (this->field_58 == 2)
	{
			if (c >= 'a' && c <= 'z')
				return c - 0x30;

			if (c == '\xC0' || c == '\xC1')
				return 0x4B;
			if (c == '\xC7')
				return 0x4C;
			if (c == '\xC8' || c == '\xC9' || c == '\xCA')
				return 0x4D;
			if (c == '\xD4')
				return 0x4E;
			if (c == '\xD9' || c == '\xDA')
				return 0x4F;
			if (c == '\x8C')
				return 0x50;
			if (c == '\xC4')
				return 0x51;
			if (c == '\xD6')
				return 0x52;
			if (c == '\xDC')
				return 0x53;
			if (c == '\xDF')
				return 0x54;
			if (c == '\xE0' || c == '\xE1')
				return 0x55;
			if (c == '\xE7')
				return 0x56;
			if (c == '\xE8' || c == '\xE9' || c == '\xEA')
				return 0x57;
			if (c == '\xF4')
				return 0x58;
			if (c == '\xF9' || c == '\xFA')
				return 0x59;
			if (c == '\x9C')
				return 0x5A;
			if (c == '\xE4')
				return 0x5B;
			if (c == '\xF6')
				return 0x5C;
			if (c == '\xFC')
				return 0x5D;

			goto default_map;
	}

	if (this->field_58 == 0)
	{
default_map:
			if (c >= 'A' && c <= 'Z')
				return c - 'A';
			if (c >= 'a' && c <= 'z')
				return c - 'a';
			if (c >= '0' && c <= '9')
				return c - 0x16;

			if (c == ' ')
				return -1;

			if (c == '?')
			{
				if (this->NumChars > 0x25)
					return 0x25;
			}
			else if (c == '!')
			{
				if (this->NumChars > 0x26)
					return 0x26;
			}
			else if (c == ':')
			{
				if (this->NumChars > 0x27)
					return 0x27;
			}
			else if (c == '.')
			{
				if (this->NumChars > 0x28)
					return 0x28;
			}
			else if (c == '-')
			{
				if (this->NumChars > 0x29)
					return 0x29;
			}
			else if (c == '+')
			{
				if (this->NumChars > 0x2B)
					return 0x2B;
			}
			else if (c == '\'')
			{
				if (this->NumChars > 0x2A)
					return 0x2A;
			}
			else if (c == '_')
			{
				if (this->NumChars > 0x24)
					return 0x24;
			}
			else if (c == '\xC0' || c == '\xC1' || c == '\xE0' || c == '\xE1')
				return 0x31;
			else if (c == '\xC7' || c == '\xE7')
				return 0x32;
			else if (c == '\xC8' || c == '\xC9' || c == '\xE8' || c == '\xE9' || c == '\xCA' || c == '\xEA')
				return 0x33;
			else if (c == '\xD4' || c == '\xF4')
				return 0x34;
			else if (c == '\xD9' || c == '\xDA' || c == '\xF9' || c == '\xFA')
				return 0x35;
			else if (c == '\x8C' || c == '\x9C')
				return 0x36;
			else if (c == '\xC4' || c == '\xE4')
				return 0x37;
			else if (c == '\xD6' || c == '\xF6')
				return 0x38;
			else if (c == '\xDC' || c == '\xFC')
				return 0x39;
			else if (c == '\xDF')
				return 0x3A;

			// button icons, the compare is against an int so it never fires for a signed char (original bug)
			if (c == 0xA5)
			{
				switch (G_SCONTROL[0].DigitalMapping[3])
				{
					case 2:
						return 1;
					case 4:
						return 0;
					case 0x200:
						return 0x18;
					case 0x400:
						return 0x17;
				}
			}

			if (c == 0xA7)
			{
				switch (G_SCONTROL[0].DigitalMapping[2])
				{
					case 2:
						return 1;
					case 4:
						return 0;
					case 0x200:
						return 0x18;
					case 0x400:
						return 0x17;
				}
			}

			if (c == 0xA6)
			{
				switch (G_SCONTROL[0].DigitalMapping[1])
				{
					case 2:
						return 1;
					case 4:
						return 0;
					case 0x200:
						return 0x18;
					case 0x400:
						return 0x17;
				}
			}

			if (c == 0xA4)
			{
				if (G_DIFFICULTY_LEVEL == 0)
					return 0;

				switch (G_SCONTROL[0].DigitalMapping[0])
				{
					case 2:
						return 1;
					case 4:
						return 0;
					case 0x200:
						return 0x18;
					case 0x400:
						return 0x17;
				}
			}
	}
	else if (this->field_58 == 1)
	{
			if (c >= '0' && c <= '9')
				return c - '0';
			if (c == ':')
				return 10;
			if (c == ' ')
				return -1;
	}
	else
	{
			print_if_false(0, "Unrecognized char mapping");
	}

	if (this->isEscapeChar(c))
		return -1;

	return this->NumChars;
}

// @Ok
// @Matching
void FontManager::ResetCharMaps(void)
{
	for (i32 i = 0; i < NUM_FONTS_TAB; i++)
	{
		if (G_FONT_TAB[i])
		{
			G_FONT_TAB[i]->SetCharMap(G_FONT_TAB[i]->GetCharMap());
		}
	}
}

// @Ok
char* FontManager::GetFontName(Font* pFont)
{
	return pFont->field_38;
}

// @Ok
// @Matching
void FontManager::AllShadowOff(void)
{
	for (i32 i = 0; i<NUM_FONTS_TAB; i++)
	{
		if (G_FONT_TAB[i])
		{
			G_FONT_TAB[i]->field_21 = 0;
		}
	}
}

// @Ok
// @Matching
void FontManager::AllShadowOn(void)
{
	for (int i = 0; i<NUM_FONTS_TAB; i++)
	{
		if (G_FONT_TAB[i])
		{
			G_FONT_TAB[i]->field_21 = 1;
		}
	}
}


// @NotOk
// matched (0 diffs) only when G_FONT_TAB targets the repo array, but that
// form crashes the game (see the note above G_FONT_TAB): GetFont is hooked
// and LoadFont is not, so the array must stay on game memory while this
// file is only partially hooked. 84 diffs under the correct (game memory)
// form.
void FontManager::UnloadFont(Font* pFont)
{
	i32 count;
	for (count = 0; count < NUM_FONTS_TAB; count++)
	{
		if (G_FONT_TAB[count] && !strcmp(G_FONT_TAB[count]->field_38, pFont->field_38))
			break;
	}

	print_if_false(count < 6, "Font %s is not in table", pFont->field_38);

	G_FONT_TAB[count]->unload();
	delete G_FONT_TAB[count];
	G_FONT_TAB[count] = 0;
}

// @Ok
// @Matching
void FontManager::UnloadAllFonts(void)
{
	Font** pp = G_FONT_TAB;
	Font** end = G_FONT_TAB + NUM_FONTS_TAB;
	for (; (i32)pp < (i32)end; pp++)
	{
		Font* p = *pp;
		if (p)
		{
			p->unload();
			delete *pp;
			*pp = 0;
		}
	}
}

// @Ok
// @Matching
INLINE u8 FontManager::IsFontLoaded(const char* pName)
{
	for (i32 i = 0; i < 6; i++)
	{
		if (FontManager::FontTab[i] && !strcmp(FontManager::FontTab[i]->field_38, pName))
		{
			return 1;
		}
	}

	return 0;
}

// @Ok
// @Matching
INLINE Font* FontManager::GetFont(const char* pName)
{
	i32 i;
	for (i = 0; i < NUM_FONTS_TAB; i++)
	{
		if (G_FONT_TAB[i] && !strcmp(G_FONT_TAB[i]->field_38, pName))
		{
			break;
		}
	}

	DoAssert(i < 6, "Font %s is not loaded", pName);
	return G_FONT_TAB[i];
}

// @Ok
// @Matching
Font* FontManager::LoadFont(u8* pBuf, const char* pName)
{
	i32 i;
	for (i = 0; i < 6; i++)
	{
		if (FontManager::FontTab[i] == 0)
		{
			print_if_false(i < 6, "out of font slots");
			FontManager::FontTab[i] = new Font(pBuf, pName);
			break;
		}

		if (!strcmp(FontManager::FontTab[i]->field_38, pName))
			break;
	}

	return FontManager::FontTab[i];
}

// @Ok
// @Matching
Font* FontManager::LoadFont(const char* pName)
{
	if (FontManager::IsFontLoaded(pName))
		return FontManager::GetFont(pName);

	i32 v2 = FileIO_Open(pName);

	print_if_false(v2 > 40, "unlikely font file size");
	print_if_false(v2 != 0, "File not found");

	u8* v3 = static_cast<u8*>(DCMem_New(v2, 0, 1, 0, 1));
	print_if_false(v3 != 0, "Out of memory");
	FileIO_Load(v3);
	FileIO_Sync();

	Font* pFont = FontManager::LoadFont(v3, pName);
	pFont->field_160 = v3;
	return pFont;
}

// @Ok
// @Note: see comment for heightAboveBaseline
INLINE i32 Font::heightBelowBaseline(char* pStr)
{
	i32 max_h = 0;
	i32 i = 0;
	
	while(pStr[i])
	{
		i32 c = pStr[i];
		if (c != 0x000000FF)
		{
			u32 tmp = this->field_5F[c];

			if ((i32)tmp != 0x000000FF)
			{
				i32 val = this->pCharTab[tmp].H - this->pCharTab[tmp].Baseline;
				if (val > max_h)
					max_h = val;
			}
		}

		i++;
	}

	return (max_h * this->field_34) >> 12;
}

// @Ok
// @Note: fuck this function, can't  get it to match now matter how much i try
// the first if inside the while oculd be removed because it performs sign extension,
// not sure what's up with that to be honest. i think it's a bug somewhere because on the PPC version, it does the same checks but with i8 not i32
INLINE i32 Font::heightAboveBaseline(char* pStr)
{
	i32 max_h = 0;
	i32 i = 0;
	
	while(pStr[i])
	{
		i32 c = pStr[i];
		if (c != 0x000000FF)
		{
			u32 tmp = this->field_5F[c];

			if ((i32)tmp != 0x000000FF)
			{
				i32 val = this->pCharTab[tmp].Baseline;
				if (val > max_h)
					max_h = val;
			}
		}

		i++;
	}

	return (max_h * this->field_34) >> 12;
}

// @SMALLTODO
i32 Font::height(char* txt)
{
	typedef i32 (*func_ptr)(char*);

	func_ptr func = (func_ptr)0x0043EAF0;

	return func(txt);
	//return this->heightAboveBaseline(txt) + this->heightBelowBaseline(txt);
}

// @Ok
// @Matching
i32 Font::width(const char* pStr)
{
	i32 width = 0;
	while (*pStr)
	{
		i32 c = *pStr;
		if (c != 0xFF)
		{
			u32 idx = this->field_5F[c];
			if ((i32)idx == 0xFF)
			{
				if (!this->isEscapeChar(*pStr))
					width += 80 * this->pCharTab[0].W / 100;
			}
			else
			{
				width += this->field_C + this->pCharTab[idx].W;
			}
		}

		pStr++;
	}

	return (width * this->field_34) >> 12;
}

// @Ok
INLINE void Font::unload(void)
{
	if (this->Clut != -1)
		Free16Slot(this->Clut);

	for (i32 i = 0; i < this->NumChars + 1; i++)
	{
		delete this->pCharTab[i].pImage;
	}

	Mem_Delete(reinterpret_cast<void*>(this->pCharTab));

	if (this->field_160)
	{
		Mem_Delete(reinterpret_cast<void*>(this->field_160));
		this->field_160 = 0;
	}
}

void validate_Font(void)
{
	VALIDATE_SIZE(Font, 0x164);

	VALIDATE(Font, field_4, 0x4);
	VALIDATE(Font, field_8, 0x8);
	VALIDATE(Font, field_C, 0xC);
	VALIDATE(Font, field_10, 0x10);

	VALIDATE(Font, mRed, 0x14);
	VALIDATE(Font, mGreen, 0x18);
	VALIDATE(Font, mBlue, 0x1C);

	VALIDATE(Font, field_21, 0x21);

	VALIDATE(Font, field_24, 0x24);
	VALIDATE(Font, field_28, 0x28);
	VALIDATE(Font, field_2C, 0x2C);
	VALIDATE(Font, field_30, 0x30);
	VALIDATE(Font, field_34, 0x34);

	VALIDATE(Font, field_38, 0x38);

	VALIDATE(Font, pCharTab, 0x48);
	VALIDATE(Font, NumChars, 0x4C);
	VALIDATE(Font, Clut, 0x50);

	VALIDATE(Font, field_58, 0x58);
	VALIDATE(Font, mSavedRed, 0x5C);
	VALIDATE(Font, mSavedGreen, 0x5D);
	VALIDATE(Font, mSavedBlue, 0x5E);
	VALIDATE(Font, field_5F, 0x5F);

	VALIDATE(Font, field_160, 0x160);
}

void validate_SFontEntry(void)
{
	VALIDATE_SIZE(FontCharacter, 0x8);

	VALIDATE(FontCharacter, pImage, 0x0);
	VALIDATE(FontCharacter, W, 0x4);
	VALIDATE(FontCharacter, H, 0x5);
	VALIDATE(FontCharacter, Baseline, 0x6);
	VALIDATE(FontCharacter, field_7, 0x7);
}

void validate_SDataGlyph(void)
{
	VALIDATE_SIZE(SDataGlyph, 0x10);

	VALIDATE(SDataGlyph, mSliceWidth, 0x0);
	VALIDATE(SDataGlyph, mHeight, 0x4);
	VALIDATE(SDataGlyph, mBaseline, 0x8);
	VALIDATE(SDataGlyph, mWidth, 0xC);
}

#include "my_patch.h"

// @Bogus
void patch_FontTools(void)
{
	PATCH_PUSH_RET(0x0043F5C0, FontManager::GetFontName);

	PATCH_PUSH_RET(0x0043F760, FontManager::AllShadowOff);
	PATCH_PUSH_RET(0x0043F780, FontManager::AllShadowOn);

	PATCH_PUSH_RET(0x0043F540, FontManager::GetFont);

	PATCH_PUSH_RET_POLY(0x0043E130, Font::Font, "??0Font@@QAE@XZ");
}
