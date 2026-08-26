#include "pshell.h"
#include "mess.h"
#include "utils.h"
#include "spool.h"
#include "ps2lowsfx.h"
#include "shell.h"
#include "PCShell.h"
#include "front.h"
#include "db.h"
#include "tweak.h"
#include "ps2pad.h"

#include <cstring>

#include "validate.h"

EXPORT i32 JoelJewCheatCode;

enum ECheatCode
{
        CHEAT_WEAKNESS = 0,
        CHEAT_LEANEST = 1,
        CHEAT_GLANDS = 2,
        CHEAT_EGOTRIP = 3,
        CHEAT_RULUR = 4,
        CHEAT_SECRTWAR = 5,
        CHEAT_MIGUELOH = 6,
        CHEAT_TRISNTNL = 7,
        CHEAT_SYNOPTIC = 8,
        CHEAT_XILRTRNS = 9,
        CHEAT_KICKME = 10,
        CHEAT_MRWATSON = 11,
        CHEAT_SMLVIII = 12,
        CHEAT_ROBRTSON = 13,
        CHEAT_KIRBYFAN = 14,
        CHEAT_MMEWEB = 15,
        CHEAT_FANBOY = 16,
        CHEAT_CINEMA = 17,
        CHEAT_RGSGLLRY = 18,
        CHEAT_UATUSEES = 19,
        CHEAT_ADMNTIUM = 20,
        CHEAT_CLUBNOIR = 21,
        CHEAT_STICKMAN = 22,
        CHEAT_FUNKYTWN = 23,
};

// @Ok
SCheat gCheats[NUM_CHEATS] =
{
	{
			"WEAKNESS",
			"full health",
	},
	{
			"LEANEST",
			"everything",
	},
	{
			"GLANDS",
			"unlimited webbing",
	},
	{
			"EGOTRIP",
			"pulsating head",
	},
	{
			"RULUR",
			"J James Jewett",
	},
	{
			"SECRTWAR",
			"symbiote spidey costume",
	},
	{
			"MIGUELOH",
			"spidey 2099 costume",
	},
	{
			"TRISNTNL",
			"Captain Universe Costume",
	},
	{
			"SYNOPTIC",
			"Spidey Unlimited Costume",
	},
	{
			"XILRTRNS",
			"Scarlet Spider Costume",
	},
	{
			"KICK ME",
			"Amazing Bag Man Costume",
	},
	{
			"MRWATSON",
			"Peter Parker Costume",
	},
	{
			"SM LVIII",
			"Quick Change Costume",
	},
	{
			"ROBRTSON",
			"Storyboard Viewer",
	},
	{
			"KIRBYFAN",
			"Game Comic Covers",
	},
	{
			"MME WEB",
			"Level Select",
	},
	{
			"FANBOY",
			"Comic Collection",
	},
	{
			"CINEMA",
			"Movie Viewer",
	},
	{
			"RGSGLLRY",
			"Character Viewer",
	},
	{
			"UATUSEES",
			"What If Contest",
	},
	{
			"ADMNTIUM",
			"invulnerable",
	},
	{
			"CLUBNOIR",
			"Ben Reilly Costume",
	},
	{
			"STICKMAN",
			"Stick Spidey",
	},
	{
			"FUNKYTWN",
			"Toon Spidey",
	},
};

// @MEDIUMTODO
i32 ActivateCheat(i32 a1)
{
	typedef i32 (*func_ptr)(i32);

	func_ptr func = (func_ptr)0x0047C240;
	return func(a1);
}

// @Ok
// @NotMatching - those ++ are weird man
void DisplayScore(
		i32 a1,
		i32 a2,
		i32 a3,
		i32 a4)
{
	char v9[0x34];
	if (!a4)
	{
		i32 v4 = a3 / 6 / 600;
		i32 v5 = a3 / 6 % 600;

		if ( v4 > 99 )
			v4 = 99;

		char* v6 = &v9[0];
		if ( v4 > 9 )
		{
			v6 = &v9[1];
			v9[0] = v4 / 10 + '0';
		}

		*v6 = v4 % 10 + '0';
		char* v7 = v6 + 1;
		*v7++ = 58;
		*v7++ = (char)v5 / 100 + '0';
		*v7++ = v5 % 100 / 10 + '0';
		*v7++ = 46;

		*v7 = v5 % 100 % 10 + '0';
		v7[1] = 0;
		Mess_DrawText(a1, a2, v9, 0, 0x1000);
	}
	else
	{
		sprintf(v9, "%d", a3);
		Mess_DrawText(a1, a2, v9, 0, 0x1000);
	}
}

// @Ok
// @Matching
i32 PShell_ActivateCheat(char * pStr)
{
	i32 v1 = -1;

	for (i32 i = 0; i < NUM_CHEATS; i++)
	{
		if (Utils_CompareStrings(pStr, gCheats[i].pCode))
		{
			v1 = i;
			break;
		}
	}

	if (v1 == CHEAT_RULUR)
		return -1;

	if (ActivateCheat(v1))
		return v1;

	return -1;
}

// byte right after TimeAttackComplete (0x60CFC6) and gWhatIf (0x60CFC5, ob.cpp).
// Neither TimeAttackComplete nor this byte are declared in the repo yet; the
// maintainer's IDB names TimeAttackComplete but not this one.
static u8 * const gPracticeDifficultyFlag = (u8*)0x60CFC7;

// @NotOk
// residue: full instruction-count match not yet reached, register scheduling
// around the two Pad_Set*Mapping calls differs from the original. Not
// re-verified after this session ended; see pshell.attempts.md.
void PShell_ApplyGameState(void)
{
	i8 v1 = gSaveGame.mDifficulty;
	i16 v2 = G_GAMESTATE[11];
	i16 v3 = G_GAMESTATE[12];
	i16 v4 = gSaveGame.mDigitalMapping[3];
	i16 v5 = gSaveGame.mAnalogueMapping[0];

	DifficultyLevel = v1;
	gSaveGame.field_98 = v2;
	i16 v6 = gSaveGame.mDigitalMapping[2];
	G_GAMESTATE[4] = v5;
	i16 v7 = gSaveGame.mAnalogueMapping[1];
	gSaveGame.field_94 = v3;
	u8 v8 = gBootRomSoundMode;
	G_GAMESTATE[3] = v4;
	*gPracticeDifficultyFlag = (v1 == 0);
	G_GAMESTATE[2] = v6;
	i16 v9 = G_GAMESTATE[13];
	G_GAMESTATE[5] = v7;
	i16 v10 = gSaveGame.mAnalogueMapping[2];
	gSaveGame.field_A0 = v8;
	i16 v11 = gSaveGame.mDigitalMapping[1];
	gSaveGame.field_9C = v9;
	i16 v12 = gSaveGame.mDigitalMapping[0];
	G_GAMESTATE[6] = v10;
	i16 v13 = gSaveGame.mAnalogueMapping[3];
	G_GAMESTATE[1] = v11;

	Pad_SetDigitalMapping(gSControl, v12, v11, v6, v4);

	G_GAMESTATE[7] = v13;
	i16 v14 = gSaveGame.field_A4;
	G_GAMESTATE[0] = v12;

	DoubleBuffer[0].Disp.screen.x = v14;
	DoubleBuffer[1].Disp.screen.x = v14;

	i16 v15 = gSaveGame.field_A8;

	DoubleBuffer[0].Disp.screen.y = v15;
	DoubleBuffer[1].Disp.screen.y = v15;

	Pad_SetAnalogueMapping(gSControl, 3, 2, 1, 0,
			G_GAMESTATE[4], G_GAMESTATE[5], G_GAMESTATE[6], G_GAMESTATE[7]);
}

// @Ok
// @Matching
void PShell_BigFont(void)
{
	Mess_SetCurrentFont("font_big.fnt");
}

// @SMALLTODO
void PShell_DrawHighlight(i32,i32,i32,i32)
{
    printf("PShell_DrawHighlight(i32,i32,i32,i32)");
}

// @SMALLTODO
void PShell_EndTrainingDisplay(void)
{
    printf("PShell_EndTrainingDisplay(void)");
}

// @MEDIUMTODO
void PShell_EndTrainingInit(void)
{
    printf("PShell_EndTrainingInit(void)");
}

// @MEDIUMTODO
void PShell_EndTrainingUpdate(void)
{
    printf("PShell_EndTrainingUpdate(void)");
}

// @MEDIUMTODO
void PShell_MaybeSaveGame(void)
{
    printf("PShell_MaybeSaveGame(void)");
}

// @MEDIUMTODO
void PShell_MaybeUnlockStuff(void)
{
    printf("PShell_MaybeUnlockStuff(void)");
}

// @Ok
// @Matching
i32 PShell_MoveTowards(
		i32 a1,
		i32 a2)
{
	i32 v2 = a2 - a1;
	if (!v2)
		return a1;

	if ((v2 & 0xFFFFFFFC) == 0 )
		return a1 + 1;

	return a1 + (v2 >> 2);
}

// @Ok
CExpandingBox::~CExpandingBox(void)
{
}


// @BIGTODO
i32 PShell_DrawMenuBox(i32, i32, i32, i32, i32, i32, i32, i32){
	return 69;
}


// @Ok
CExpandingBox::CExpandingBox(
		int a2,
		int a3,
		int a4,
		int a5,
		int a6,
		int a7,
		int a8,
		int a9,
		int a10)
{
	this->field_1C = a2;
	this->field_20 = a3;
	this->field_C = a4;
	this->field_10 = a5;
	this->field_4 = a6;
	this->field_8 = a7;
	this->field_14 = a8;
	this->field_18 = a9;
	this->field_24 = a10;
	this->field_2C = 28;
}



// @MEDIUMTODO
int CExpandingBox::Display(){

	/*
	int unk_3; // ebx
	int v2; // eax
	int unk_4; // eax
	int v4; // edx
	int unk_1; // edi
	unk_3 = this->unk_3;
	v2 = this->unk_5 + this->unk_1;
	this->unk_1 = v2;
	if ( v2 > unk_3 )
		this->unk_1 = unk_3;
	unk_4 = this->unk_4;
	v4 = this->unk_6 + this->unk_2;
	this->unk_2 = v4;
	if ( v4 > unk_4 )
		this->unk_2 = unk_4;
	unk_1 = this->unk_1;
	if ( unk_1 == unk_3 && this->unk_2 == unk_4 )
	this->unk_12 = 1;

	return PShell_DrawMenuBox(
		this->unk_7 + unk_3 / 2 - unk_1 / 2,
		this->unk_8 + unk_4 / 2 - this->unk_2 / 2,
		unk_1,
		this->unk_2,
		1,
		this->unk_9,
		this->unk_10,
		this->unk_11);
		*/
	return 0x14072024;
}

// @BIGTODO
i32 CExpandingBox::ScrollBarHitTest(i32 a2, i32 a3)
{
	printf("i32 CExpandingBox::ScrollBarHitTest(int a2, int a3)");

	return 0x14072024;
}

static u8 gCheatRelatedOne;
static i32 gCheatRelatedTwo;
static i32 gCheatRelatedThree;
static i32 gCheatRelatedFour;
static i32 gCheatRelatedFive;
static i32 gCheatRelatedSix;
static u8 gCheatRelatedSeven;

// @NotOk
// Globals
void PShell_BigCheat(void)
{
      gCheatRelatedOne = 1;
      gCheatRelatedTwo = -1;
      gCheatRelatedThree = -1;
      gCheatRelatedFour = -1;
      gCheatRelatedFive = -1;
      gCheatRelatedSix = -1;
      gCheatRelatedSeven = 1;
}

// @Ok
void PShell_NormalFont(void)
{
	Mess_SetScale(256);
	Mess_SetCurrentFont("sp_fnt00.fnt");
}

// @Ok
void PShell_DefaultText(void)
{
	PShell_NormalFont();
	Mess_SetTextJustify(0);
	Mess_SetRGB(0x80, 0x80, 0x80, 0);
	Mess_SetRGBBottom(0x45, 60, 107);
}

// @Ok
void PShell_SmallFont(void)
{
	Mess_SetScale(256);
	Mess_SetCurrentFont("sp_fnt02.fnt");
}

// @Ok
void PShell_InstructionalText(void)
{
	PShell_SmallFont();
	Mess_SetTextJustify(0);
	Mess_SetRGB(0x45u, 0x3Cu, 0x6Bu, 0);
	Mess_SetRGBBottom(0x28u, 35, 62);
}

void validate_CExpandingBox(void)
{
	VALIDATE_SIZE(CExpandingBox, 0x34);

	VALIDATE(CExpandingBox, field_4, 0x4);
	VALIDATE(CExpandingBox, field_8, 0x8);
	VALIDATE(CExpandingBox, field_C, 0xC);
	VALIDATE(CExpandingBox, field_10, 0x10);
	VALIDATE(CExpandingBox, field_14, 0x14);
	VALIDATE(CExpandingBox, field_18, 0x18);
	VALIDATE(CExpandingBox, field_1C, 0x1C);

	VALIDATE(CExpandingBox, field_20, 0x20);
	VALIDATE(CExpandingBox, field_24, 0x24);

	VALIDATE(CExpandingBox, field_2C, 0x2C);
}

void validate_SCheat(void)
{
	VALIDATE_SIZE(SCheat, 0x8);

	VALIDATE(SCheat, pCode, 0x0);
	VALIDATE(SCheat, pDescription, 0x4);

	if (strcmp(gCheats[NUM_CHEATS-1].pCode, "FUNKYTWN"))
	{
		printf("MISMATCH IN CHEAT TABLE %s should be FUNKYTWN", gCheats[NUM_CHEATS-1].pCode);
	}


}

#include "my_patch.h"

// @Bogus
void patch_pshell(void)
{
	PATCH_PUSH_RET(0x0047C440, PShell_ActivateCheat);
}
