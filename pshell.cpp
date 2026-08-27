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
#include "panel.h"
#include "spidey.h"
#include "PCGfx.h"

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

// these five and mCheatStoryboardFlag/field_78 now live as real fields in
// SSaveGame (shell.h), found via ActivateCheat's cheat flag writes landing
// inside gSaveGame's address range; see shell.h for per-field evidence.
#define G_LEVEL_SELECT_FLAG (*reinterpret_cast<u32*>(0x0060CFD8))
#define G_UNLIMITED_WEBBING_FLAG (*reinterpret_cast<i32*>(0x0060CFE8))
#define G_PULSATING_HEAD_FLAG (*reinterpret_cast<i32*>(0x0060CFF0))
#define G_INVULNERABLE_FLAG (*reinterpret_cast<i32*>(0x0060CFC8))

// well past .data's raw file image (real BSS), zero at load
#define G_STICKMAN_FLAG (*reinterpret_cast<i32*>(0x02E09BF4))
#define G_TOON_SPIDEY_FLAG (*reinterpret_cast<i32*>(0x02E09BF0))

// @NotOk
// residue: 45 mnemonic diffs out of 95 instructions, same byte length (391).
// Every case's logic and the switch dispatch itself match (jump table shape,
// bounds check, eax=1 preset). The residue is that several cases compile as
// load/modify/store (mov reg,[mem]; op reg,imm; mov[mem],reg) here where the
// original folds to a single memory-immediate op (or dword ptr [mem],imm /
// xor [mem],eax). Moving the newly-discovered cheat flags into real
// SSaveGame fields (see shell.h) fixed most of the |= cases on
// gSaveGame.field_80/84/88/8C/90 (confirmed via cmpsum, dropped 54 -> 45
// diffs); the remaining ones are on plain fixed-address globals outside any
// known struct (0x60CFE8, 0x60CFF0, 0x60CFC8, 0x2E09BF4, 0x2E09BF0) where no
// such struct exists to attach them to. 11 hypotheses tried: explicit
// return-per-case vs a shared result local with break (result+break fixed
// the dispatch header, 68->54 diffs), G_* macro vs static const pointer for
// the flag globals (no effect), an explicit bounds check before the switch
// instead of relying on the jump table's own check (worse, 69 diffs),
// dropping the CHEAT_WEAKNESS local in favor of inline G_MECHLIST casts (no
// effect), moving the cheat flags into real SSaveGame struct fields
// (improvement, see above), and XORing against the already-1 result local
// instead of the literal 1 (no effect). See pshell.attempts.md.
i32 ActivateCheat(i32 a1)
{
	i32 result = 1;

	switch (a1)
	{
		case CHEAT_WEAKNESS:
			if (G_MECHLIST)
			{
				((CPlayer*)G_MECHLIST)->mHealth = ((CPlayer*)G_MECHLIST)->mMaxHealth;
			}
			break;

		case CHEAT_LEANEST:
			gSaveGame.field_78 = 1;
			gSaveGame.field_80 = -1;
			gSaveGame.field_84 = -1;
			gSaveGame.field_88 = -1;
			gSaveGame.field_8C = -1;
			gSaveGame.field_90 = -1;
			gSaveGame.mCheatStoryboardFlag = 1;
			G_LEVEL_SELECT_FLAG = 1;
			break;

		case CHEAT_GLANDS:
			G_UNLIMITED_WEBBING_FLAG ^= 1;
			break;

		case CHEAT_EGOTRIP:
			G_PULSATING_HEAD_FLAG ^= 1;
			break;

		case CHEAT_RULUR:
			JoelJewCheatCode = !JoelJewCheatCode;
			break;

		case CHEAT_SECRTWAR:
			gSaveGame.field_80 |= 4;
			break;

		case CHEAT_MIGUELOH:
			gSaveGame.field_80 |= 2;
			break;

		case CHEAT_TRISNTNL:
			gSaveGame.field_80 |= 8;
			break;

		case CHEAT_SYNOPTIC:
			gSaveGame.field_80 |= 0x10;
			break;

		case CHEAT_XILRTRNS:
			gSaveGame.field_80 |= 0x40;
			break;

		case CHEAT_KICKME:
			gSaveGame.field_80 |= 0x20;
			break;

		case CHEAT_MRWATSON:
			gSaveGame.field_80 |= 0x200;
			break;

		case CHEAT_SMLVIII:
			gSaveGame.field_80 |= 0x100;
			break;

		case CHEAT_ROBRTSON:
			gSaveGame.mCheatStoryboardFlag = !gSaveGame.mCheatStoryboardFlag;
			break;

		case CHEAT_KIRBYFAN:
			gSaveGame.field_90 = 0x3F;
			break;

		case CHEAT_MMEWEB:
			G_LEVEL_SELECT_FLAG = 1;
			break;

		case CHEAT_FANBOY:
			gSaveGame.field_8C = -1;
			break;

		case CHEAT_CINEMA:
			gSaveGame.field_88 = -1;
			break;

		case CHEAT_RGSGLLRY:
			gSaveGame.field_84 = -1;
			break;

		case CHEAT_UATUSEES:
			gWhatIf = !gWhatIf;
			break;

		case CHEAT_ADMNTIUM:
			G_INVULNERABLE_FLAG ^= 1;
			break;

		case CHEAT_CLUBNOIR:
			gSaveGame.field_80 |= 0x80;
			break;

		case CHEAT_STICKMAN:
			if (G_STICKMAN_FLAG = !G_STICKMAN_FLAG)
			{
				G_TOON_SPIDEY_FLAG = 0;
			}
			break;

		case CHEAT_FUNKYTWN:
			if (G_TOON_SPIDEY_FLAG = !G_TOON_SPIDEY_FLAG)
			{
				G_STICKMAN_FLAG = 0;
			}
			break;

		default:
			result = 0;
			break;
	}

	return result;
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

// byte right after gPostWaterEffect (0x5FAE98, i32, utils.cpp). No exact name
// in the maintainer's IDB for this one; tentative, guessed from this use
// (picks the box z-offset sign, close to the water-effect/post-process flags).
static u8 * const gDrawHighlightZFlag = (u8*)0x5FAE9D;

// @NotOk
// 5 mnemonic diffs out of 122 instructions, all in the pPoly/p setup right
// after print_if_false. Original loads pPoly straight into esi and pre-adds
// a4+4 into ebp as an eager separate instruction, reused later via a plain
// add; our build always routes pPoly through eax first and folds the +4
// into the later y0 computation as a 3-operand lea.
// 12 distinct hypotheses tried targeting this exact cluster (declaration
// order forward/reverse, split vs combined statement, volatile, pointer-unit
// vs byte-cast arithmetic, basing the increment on p vs pPoly), residue did
// not move. This is a 353-byte (medium-size) function, the discipline needs
// at least 15 hypotheses before @AlmostMatching is allowed, so this stays
// @NotOk until more are tried. See pshell.attempts.md.
void PShell_DrawHighlight(i32 a1, i32 a2, i32 a3, i32 a4)
{
	Texture* pTex = Spool_FindTextureEntry(0xE90B5F6E);
	print_if_false(pTex != 0, "Missing title bar texture");

	POLY_GT4* p = (POLY_GT4*)pPoly;
	pPoly = (u32*)((u8*)pPoly + sizeof(POLY_GT4));
	i32 v1 = a4 + 4;

	p->tag = 0xC000000;
	p->code = 0x3E;

	*(u32*)&p->u0 = *(u32*)&pTex->u0;
	*(u32*)&p->u1 = *(u32*)&pTex->u1;
	*(u32*)&p->u2 = *(u32*)&pTex->u2;
	*(u16*)&p->u3 = *(u16*)&pTex->u3;

	p->tpage = (p->tpage & ~0x40) | 0x20;

	p->u1 -= 1;
	p->u3 -= 1;

	p->r0 = 0x40;
	p->g0 = 0x40;
	p->r2 = 0x40;
	p->g2 = 0x40;

	i32 y0 = a2;
	i32 x1 = a3;
	p->b0 = 0x7B;
	p->b2 = 0x7B;
	i32 x0 = a1;

	y0 += 3;
	x1 += x0;

	p->y0 = (i16)y0;
	p->y1 = (i16)y0;

	y0 += v1;

	p->r1 = 0;
	p->g1 = 0;
	p->b1 = 0;
	p->r3 = 0;
	p->g3 = 0;
	p->b3 = 0;

	p->x0 = (i16)x0;
	p->x1 = (i16)x1;
	p->x2 = (i16)x0;
	p->y2 = (i16)y0;
	p->x3 = (i16)x1;
	p->y3 = (i16)y0;

	gsub_46CB90((void*)0x0056EB54);

	i32 sort = G_SORT;
	if (sort >= 0xFFE && sort <= 0xFFF)
	{
		DCDrawGouraudPoly(-2.0f, p, pTex, a3 < 0);
	}
	else if (*gDrawHighlightZFlag)
	{
		DCDrawGouraudPoly(2.0f, p, pTex, a3 < 0);
	}
	else
	{
		DCDrawGouraudPoly(-2.0f, p, pTex, a3 < 0);
	}
}

// Training end-of-level "new record" flow (PShell_EndTraining*). Names
// tentative, no idb_globals.txt entries in the 0x00682950-0x00682968 gap
// (right after gBiographies, 0x0068294C) or for 0x0055129C. gChallenges is
// named in idb_globals.txt (0x00551118), and its element type/size (0x10,
// STrainingMission) is confirmed in shell.h. gTrainingScore's -1000 sentinel
// and the CRecordBox/STrainingMission field_3C/field_B reads are read off
// PShell_EndTrainingDisplay's own disassembly.
static STrainingMission* const gChallenges = reinterpret_cast<STrainingMission*>(0x00551118);
#define gTrainingChallengeIndex (*reinterpret_cast<i32*>(0x0068295C))
#define gTrainingResultState (*reinterpret_cast<i32*>(0x00682958))
#define gTrainingRecordBox (*reinterpret_cast<CRecordBox**>(0x00682960))
#define gTrainingDisplayTimer (*reinterpret_cast<i32*>(0x00682964))
#define gTrainingMenu (*reinterpret_cast<CMenu**>(0x00682968))
#define gTrainingScore (*reinterpret_cast<i32*>(0x0055129C))

// These three are elements of the same string-literal-pointer table
// front.cpp already names two entries of (gFrontYesText/gFrontNoText,
// 0x0054B780/0x0054B77C); string content confirmed against the original exe.
#define gTextNewRecord (*reinterpret_cast<char**>(0x0054B8E4))
#define gTextYourScore (*reinterpret_cast<char**>(0x0054B8F0))
#define gTextNone (*reinterpret_cast<char**>(0x0054B8F4))

// @Ok
// @Matching
void PShell_EndTrainingDisplay(void)
{
	Mess_SetRGB(0x60, 0x60, 0x60, 0);
	Mess_SetTextJustify(0);

	Mess_DrawText(0x100, 0x1D, gChallenges[gTrainingChallengeIndex].field_0, 0, 0x1000);

	if (gTrainingDisplayTimer != 0)
	{
		gTrainingDisplayTimer--;

		if ((gTrainingDisplayTimer % 10) > 5)
		{
			PShell_BigFont();
			PShell_DefaultText();
			Mess_DrawText(0x100, 0x78, gTextNewRecord, 0, 0x1000);
			PShell_NormalFont();
		}

		return;
	}

	if (gTrainingRecordBox)
		gTrainingRecordBox->Display();

	if (gTrainingMenu)
		gTrainingMenu->Display();

	if (gTrainingMenu)
		PCSHELL_DrawMouseCursor();

	if (gTrainingResultState != 0)
		return;

	PShell_DefaultText();
	Mess_SetTextJustify(2);
	Mess_DrawText(0x127, 0x9B, gTextYourScore, 0, 0x1000);

	if (gTrainingRecordBox)
	{
		Mess_SetTextJustify(1);

		if (gTrainingScore == -1000)
		{
			Mess_DrawText(0x131, 0x9B, gTextNone, 0, 0x1000);
		}
		else
		{
			DisplayScore(0x131, 0x9B, gTrainingScore, gTrainingRecordBox->field_3C->mScoreUnits);
		}
	}

	Mess_SetTextJustify(0);
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

// Save-confirmation modal loop. Same idiom as Shell_ShowRecord's loop
// (shell.cpp): a zoom-ease local eases pYesNoMenu's zoom animation via
// PShell_MoveTowards (own local here, not shell.cpp's gShellMenuEase: the
// disassembly never stores it to any fixed address, purely a register/stack
// local, unlike Shell_ShowRecord's use of the real global), a "was the
// opening button still held" flag suppresses PCSHELL_CheckTriggers until the
// player releases it, and DoVblankProcessing/gPrintStubbed/gsub_430680 are
// the standard per-frame housekeeping calls (see shell.cpp for the same 5
// lines). gTextSaveGameProgress: string content confirmed against the
// original exe, same string-pointer table as pshell.cpp's other gText*
// globals near it.
#define gTextSaveGameProgress (*reinterpret_cast<char**>(0x0054B8F8))

// @Ok
// @Matching
// Note: the call at 0x0043FB00 (thiscall on pYesNoMenu, no args, return
// value unused) is guessed as CMenu::Reset() (front.h/front.cpp): same call
// shape (this in ecx, no pushed args, no value read back), and semantically
// it fits (reset the menu highlight after the yes/no choice resolves). Not
// confirmed against the Mac build or idb_globals.txt; call targets are
// masked by cmpsum/compare.py so this guess does not affect the verified
// mnemonic match. See pshell.attempts.md.
void PShell_MaybeSaveGame(void)
{
	// same pattern as front.cpp/shell.cpp/PCShell.cpp: defined once in
	// PCShell.cpp, called here through a local extern.
	extern void gsub_430880(void);

	u32 saveBuf[3];

	do
	{
		i32 ignoreTriggers = 1;
		i32 zoomEase = 0x320;

		print_if_false(pYesNoMenu != 0, "NULL pYesNoMenu");

		pYesNoMenu->mLine = 1;
		pYesNoMenu->Zoom(0);

		for (;;)
		{
			gsub_430880();
			Db_FlipClear();
			CalcPolyBufferEnd();

			i32 vblanksSnapshot = Vblanks;

			if (!gSceneRelated)
				PCGfx_BeginScene(1, -1);

			Mess_SetScale(0x100);
			pYesNoMenu->Display();

			Mess_SetRGB(0x4D, 0x53, 0x69, 0);
			Mess_DrawText(0x100, 0x3C, gTextSaveGameProgress, 0, 0x1000);

			if (gSceneRelated)
				PCGfx_EndScene(1);

			zoomEase = PShell_MoveTowards(zoomEase, 0x1CC);

			Pad_Update();

			if (!G_SCONTROL[0].X.Pressed)
			{
				ignoreTriggers = 0;
				G_SCONTROL[0].X.Triggered = 0;
			}

			pYesNoMenu->Update();

			if (!ignoreTriggers)
			{
				if (PCSHELL_CheckTriggers(1, 1, 0x50010))
					break;
			}

			if (Vblanks == vblanksSnapshot)
				Pause(1);

			*(volatile i32*)&DoVblankProcessing = 0;
			Pause(1);

			if (!gPrintStubbed)
				gsub_46CB90((void*)"stubbed out: DrawSync");
			gsub_430680();

			if (!*(volatile i32*)&DoVblankProcessing)
			{
				Utils_VblankProcessing();
				DoVblankProcessing = 1;
			}
		}

		SControl* pad = G_SCONTROL;
		pad[0].X.Triggered = 0;
		pad[0].Start.Triggered = 0;
		SFX_Play(0x1F, 0x2000, 0);

		Pause(1);

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");
		gsub_430680();

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");

		Front_ClearScreen();

		if (!gPrintStubbed)
			gsub_46CB90((void*)"stubbed out: DrawSync");

		Pad_ClearTriggers(G_SCONTROL);
		pYesNoMenu->Reset();

		if (pYesNoMenu->mLine != 1)
			return;

		saveBuf[2] = 1;
		Shell_SaveGame(reinterpret_cast<const u32*>(&saveBuf[0]), &saveBuf[1]);
	} while (saveBuf[1] == 0);
}

// @Ok
// @Matching
void PShell_MaybeUnlockStuff(void)
{
	i32 unlocked = -1;

	i32 i;

	i32 allComplete = 1;
	for (i = 0; i < 0x22; i++)
	{
		if (!gSaveGame.field_56[i])
		{
			allComplete = 0;
		}
	}

	if (allComplete)
	{
		u8 oldFlags = (u8)gSaveGame.field_80;
		gSaveGame.mCheatStoryboardFlag = 1;

		if (!(oldFlags & 4))
		{
			unlocked = 2;
		}

		gSaveGame.field_80 |= 4;
		gSaveGame.field_84 |= 0x1000000;

		if (DifficultyLevel == 3)
		{
			if (!((u8)gSaveGame.field_80 & 8))
			{
				unlocked = 3;
			}

			gSaveGame.field_80 |= 8;
		}
	}

	if (gSaveGame.field_8C == -1)
	{
		gSaveGame.field_80 |= 2;
	}

	i32 idx1 = Front_GetLevelIndex("l4a1_t");
	print_if_false(idx1 != -1, "Could not find l4a1_t ???");
	if (gSaveGame.field_56[idx1])
	{
		gSaveGame.field_80 |= 0x40;
	}

	i32 idx2 = Front_GetLevelIndex("l6a4_t");
	print_if_false(idx2 != -1, "Could not find l6a4_t ???");
	if (gSaveGame.field_56[idx2])
	{
		gSaveGame.field_80 |= 0x80;
	}

	i32 allGold = 1;
	for (i = 0; i < 0x22; i++)
	{
		if (gSaveGame.field_56[i] < 2)
		{
			allGold = 0;
		}
	}

	if (allGold)
	{
		if (!((u8)gSaveGame.field_80 & 0x10))
		{
			unlocked = 4;
		}

		gSaveGame.field_80 |= 0x10;
	}

	if (unlocked != -1)
	{
		gSaveGame.field_7C = (u8)unlocked;
	}
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

// @NotOk
// residue not yet resolved, see pshell.attempts.md
i32 CExpandingBox::ScrollBarHitTest(i32 a2, i32 a3)
{
	if (!this->field_24 || !this->field_30)
		return 0;

	i16 v1 = this->field_1C;
	v1 -= 14;

	if (a2 < (u16)v1)
		return 0;

	i16 v2 = this->field_20;

	if (a2 > (u16)(v1 + 14))
		return 0;

	if (a3 < (u16)(v2 - 3))
		return 0;

	i16 v3 = this->field_8;

	if (a3 > (u16)((u16)(v3 + 6) + (u16)(v2 - 3)))
		return 0;

	i32 v4 = ((this->field_8 - this->field_2C - 8) * this->field_28 >> 8) + v2 + 4;

	if (a3 <= (u16)(v2 + 7))
		return 1;

	if (a3 < (u16)(v3 + v2 - 5))
		return 2;

	if (a3 >= (u16)v4)
		return 4;

	if (a3 > this->field_2C + (u16)v4)
		return 5;

	return 3;
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
	VALIDATE(CExpandingBox, field_28, 0x28);

	VALIDATE(CExpandingBox, field_2C, 0x2C);
	VALIDATE(CExpandingBox, field_30, 0x30);
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
