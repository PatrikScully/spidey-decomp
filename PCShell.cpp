#include "PCShell.h"
#include "dcshellutils.h"
#include "PCInput.h"
#include "SpideyDX.h"
#include "front.h"
#include "DXsound.h"
#include "pshell.h"
#include "PCGfx.h"
#include "shell.h"
#include "mess.h"
#include "ps2pad.h"
#include "db.h"
#include "main.h"
#include "utils.h"
#include "ps2lowsfx.h"
#include "DXinit.h"

#include <cstring>

#include "validate.h"

EXPORT Sprite2* gCursorSprite;

EXPORT i32 gShellMouseX;
EXPORT i32 gShellMouseY;

EXPORT i32 gShellMouseOffsetX;
EXPORT i32 gShellMouseOffsetY;

const i32 MOUSE_TRIGGER_COUNT = 18;
EXPORT u8 gMouseTriggerRelated[MOUSE_TRIGGER_COUNT];

const i32 ACTION_MAP_COUNT = 11;
EXPORT SActionMap gActionMaps[ACTION_MAP_COUNT];
EXPORT char gKeyNames[ACTION_MAP_COUNT][32];

EXPORT CMenu* gControllerMenu;
EXPORT CMenu* gControllerMenuTwo;

EXPORT i32 gActionMapRelated;

char* STR_RESTORE_DEFAULTS = "restore default settings";
char* STR_KB_CONFIG = "keyboard configuration";
char* STR_JOY_CONFIG = "joystick configuration";

EXPORT i32 gShellTitleBarRelated;

// set while the controller screen waits for the player to press a new key or button
EXPORT i32 gShellWaitingForInput;

// @Ok
// @Matching
u8 PCSHELL_CheckTriggers(u32 mask, i32 a2, i32 a3)
{
	i32 result = 0;
	u32 button;

	if (mask & 0x1)
		result |= PCINPUT_IsKeyPressed(0xC8, a2);

	if (mask & 0x2)
		result |= PCINPUT_IsKeyPressed(0xD0, a2);

	if (mask & 0x4)
		result |= PCINPUT_IsKeyPressed(0xCB, a2);

	if (mask & 0x8)
		result |= PCINPUT_IsKeyPressed(0xCD, a2);

	if (mask & 0x10)
	{
		if (!gMouseTriggerRelated[4])
		{
			result |= PCINPUT_IsKeyPressed(0x1C, a2);
			if (a3)
				gMouseTriggerRelated[4] = 1;
		}
	}

	if (mask & 0x20)
	{
		if (!gMouseTriggerRelated[5])
		{
			result |= PCINPUT_IsKeyPressed(0x1, a2);
			if (a3)
				gMouseTriggerRelated[5] = 1;
		}
	}

	if (mask & 0x40)
		result |= PCINPUT_IsKeyPressed(0x1, a2);

	if (!(gRenderTest & 0x10))
	{
		if (mask & 0x100)
		{
			if (!gMouseTriggerRelated[7])
			{
				result |= PCINPUT_IsMouseButtonPressed(0, a2);
				if (a3)
					gMouseTriggerRelated[7] = 1;
			}
		}

		if (mask & 0x200)
		{
			if (!gMouseTriggerRelated[8])
			{
				result |= PCINPUT_IsMouseButtonPressed(1, a2);
				if (a3)
					gMouseTriggerRelated[8] = 1;
			}
		}
	}

	if (mask & 0x1000)
	{
		if (PCINPUT_GetControllerDirections() & 1)
		{
			result |= 1;
			if (a3)
				PCINPUT_FreezeControllerAxes();
		}
	}

	if (mask & 0x2000)
	{
		if (PCINPUT_GetControllerDirections() & 2)
		{
			result |= 1;
			if (a3)
				PCINPUT_FreezeControllerAxes();
		}
	}

	if (mask & 0x4000)
	{
		if (PCINPUT_GetControllerDirections() & 4)
		{
			result |= 1;
			if (a3)
				PCINPUT_FreezeControllerAxes();
		}
	}

	if (mask & 0x8000)
	{
		if (PCINPUT_GetControllerDirections() & 8)
		{
			result |= 1;
			if (a3)
				PCINPUT_FreezeControllerAxes();
		}
	}

	if (mask & 0x10000)
	{
		if (!gMouseTriggerRelated[13])
		{
			PCINPUT_GetControllerMappingForAction(0x10, &button);
			if (button == 0x4000)
				button = 0;
			result |= PCINPUT_IsControllerButtonPressed(button, a2);
			if (a3)
				gMouseTriggerRelated[13] = 1;
		}
	}

	if (mask & 0x20000)
	{
		if (!gMouseTriggerRelated[14])
		{
			PCINPUT_GetControllerMappingForAction(0x20, &button);
			if (button == 0x4000)
				button = 1;
			result |= PCINPUT_IsControllerButtonPressed(button, a2);
			if (a3)
				gMouseTriggerRelated[14] = 1;
		}
	}

	if (mask & 0x40000)
	{
		if (!gMouseTriggerRelated[15])
		{
			PCINPUT_GetControllerMappingForAction(0x1000, &button);
			if (button != 0x4000)
				result |= PCINPUT_IsControllerButtonPressed(button, a2);
			if (a3)
				gMouseTriggerRelated[15] = 1;
		}
	}

	if (mask & 0x100000)
	{
		result |= PCINPUT_IsKeyPressed(0xD, a2);
		result |= PCINPUT_IsKeyPressed(0x4E, a2);
	}

	if (mask & 0x200000)
	{
		result |= PCINPUT_IsKeyPressed(0xC, a2);
		result |= PCINPUT_IsKeyPressed(0x4A, a2);
	}

	if (mask & 0x1000000)
		result |= PCINPUT_IsKeyPressed(0x1C, a2);

	if (mask & 0x2000000)
		result |= PCINPUT_IsKeyPressed(0x1, a2);

	if (mask & 0x4000000)
		result |= PCINPUT_IsKeyPressed(0x39, a2);

	return result != 0;
}

// @Ok
INLINE void PCSHELL_CoordsDCtoPC(i32* pX, i32* pY)
{
	*pX = (f64)*pX / 512.0 * (f64)gDxResolutionX;
	*pY = (f64)*pY / 240.0 * (f64)gDxResolutionY;
}

// @Ok
// @Matching
INLINE void PCSHELL_CoordsPCtoDC(i32* pX, i32* pY)
{
	*pX = (f32)(*pX * 512) / (f32)gDxResolutionX;
	*pY = (f32)(*pY * 240) / (f32)gDxResolutionY;
}

// forward declaration, defined further down with displayControllerScreen
void shell_optimized_func(i32, i32, i32);

// unnamed helpers called from PCSHELL_DoControllerConfig; bodies defined further down
// so they are not visible for same-TU inlining at their call sites (matches the original,
// which has real "call" instructions at each site, not inlined bodies).
EXPORT void gsub_430680(void);
EXPORT void gsub_430880(void);
EXPORT void gsub_515850(void);

// @Ok
// @Matching
void PCSHELL_DoControllerConfig(bool isKeyboard)
{
	gControllerMenu = new CMenu(30, 60, 1u, 256, 256, 15);
	gControllerMenuTwo = new CMenu(332, 60, 1u, 256, 256, 15);

	gActionMapRelated = isKeyboard == 0;
	gShellWaitingForInput = 0;
	gShellTitleBarRelated = 0;

	initActionMaps();

	gControllerMenuTwo->scrollbar_zero = 1;
	gControllerMenu->scrollbar_zero = 1;

	gControllerMenu->Zoom(0);
	gControllerMenuTwo->Zoom(0);

	gControllerMenuTwo->mLine = isKeyboard != 0 ? 0 : 4;
	gControllerMenu->mLine = gControllerMenuTwo->mLine;

	u8 done;
	do
	{
		i32 startVblanks = Vblanks;

		gsub_430880();

		Db_FlipClear();
		CalcPolyBufferEnd();

		if (!gSceneRelated)
			PCGfx_BeginScene(1, -1);

		PShell_NormalFont();

		gControllerMenu->Display();
		gControllerMenuTwo->Display();

		shell_optimized_func(384, 222, 0);

		char* configName = !gActionMapRelated ? STR_KB_CONFIG : STR_JOY_CONFIG;

		Shell_DrawTitleBar(gShellTitleBarRelated, 25, configName, 1, 0, 150, -21, 29);
		Shell_DrawBackground();

		if (!(gRenderTest & 0x10) && gCursorSprite && PCINPUT_GetMouseStatus())
		{
			gCursorSprite->draw(gShellMouseX, gShellMouseY, 0, 0);
		}

		if (gSceneRelated)
			PCGfx_EndScene(1);

		done = processControllerScreen();

		gsub_430680();
		WinYield();
		Sleep(10);

		Pause(startVblanks - Vblanks + 2);
	} while (!done);

	gsub_430680();
	Pad_ClearTriggers(G_SCONTROL);

	delete gControllerMenu;
	delete gControllerMenuTwo;
	gControllerMenu = 0;
	gControllerMenuTwo = 0;

	gsub_515850();
}

// PCSHELL_DoControllerConfig's original calls this as a real out-of-line function, keep the MSVC inliner away
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// unnamed helper called once per controller config screen frame, address 0x430680.
// original bytes disassemble to an empty function (no args used), name is a names.json guess ("optimized_unused_garbage")
// @SMALLTODO
EXPORT void gsub_430680(void)
{
	printf("gsub_430680(void)");
}

// unnamed helper called once at the top of every controller config screen frame, address 0x430880 (named "nullsub_3" in the IDA export)
// @SMALLTODO
EXPORT void gsub_430880(void)
{
	printf("gsub_430880(void)");
}

// unnamed helper called once after PCSHELL_DoControllerConfig's loop ends, address 0x515850
// @SMALLTODO
EXPORT void gsub_515850(void)
{
	printf("gsub_515850(void)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// menu line text, addresses 0x54BBCC / 0x54BBD4 / 0x54BBD8. content unverified
// (no access to the original data section), text is a guess based on what the
// three lines do (resolution, colour depth, brightness).
static char* STR_DISPLAY_RESOLUTION = "resolution";
static char* STR_DISPLAY_COLOR_DEPTH = "colour depth";
static char* STR_DISPLAY_BRIGHTNESS = "brightness";

// title bar text, address 0x54B7CC. content unverified, guess.
static char* STR_DISPLAY_OPTIONS_TITLE = "display options";

// sprintf format strings, addresses 0x568868 (resolution line, 2 ints) and
// 0x568864 (colour depth and brightness lines, reused for both, 1 int each).
// content unverified, guesses based on the values passed to sprintf.
static char* FMT_DISPLAY_RESOLUTION = "%dx%d";
static char* FMT_DISPLAY_NUMBER = "%d";

// pending colour depth chosen in the display options menu, applied on Confirm.
// address 0x2E098E4, tentative name. Nearest idb_globals.txt neighbours are
// 0x2E09810 gDisplayDeviceIndex, 0x2E09814 gMMXSupport and 0x2E098E8
// gMissingCD; this address does not fall inside any of them.
static u32 gPendingColorDepth;

// pending resolution chosen in the display options menu, applied on Confirm.
// addresses 0x2E096F8 / 0x2E0970C, tentative names. They fall between
// idb_globals.txt's 0x2E096D8 gLowGraphicsRelated and 0x2E09710 "Data"
// (our gDisplayModeContext), not inside either.
static u32 gPendingResolutionX;
static u32 gPendingResolutionY;

// @NotOk
void PCSHELL_DoDisplayOptions(void)
{
	CMenu* menu = new CMenu(0x10E, 0x6E, 2u, 0x100, 0x100, 0x14);

	i32 brightness = gBrightnessRelated;

	// eases the shell box towards x=384 every frame, starting at x=512
	// (slides in from further right). the volatile write forces MSVC to
	// store the immediate directly to the stack slot right here (matching
	// the original), instead of homing it in a register for the whole
	// function, which changes unrelated register allocation.
	i32 easeX;
	*(volatile i32*)&easeX = 0x200;

	u32 depth = DXINIT_GetNextColorDepth(0);

	if (depth != gPendingColorDepth)
	{
		u32 first = depth;

		do
		{
			depth = DXINIT_GetNextColorDepth(depth);
		} while (depth != first && depth != gPendingColorDepth);
	}

	gPendingColorDepth = depth;

	menu->AddEntry(STR_DISPLAY_RESOLUTION);
	menu->AddEntry(STR_DISPLAY_COLOR_DEPTH);
	menu->AddEntry(STR_DISPLAY_BRIGHTNESS);

	menu->scrollbar_zero = 1;
	menu->Zoom(0);
	menu->SetLine(0);

	if (!g3DAccelator || !gMMXSupport)
		menu->EntryEnable(1, 0);

	gShellTitleBarRelated = 0;

	for (;;)
	{
		i32 startVblanks = Vblanks;

		gsub_430880();

		Db_FlipClear();
		CalcPolyBufferEnd();

		if (!gSceneRelated)
			PCGfx_BeginScene(1, -1);

		shell_optimized_func(easeX, 0xDE, 0);

		Shell_DrawBackground();
		Shell_DrawTitleBar(gShellTitleBarRelated, 0x26, STR_DISPLAY_OPTIONS_TITLE, 1, 0, 0x96, -21, 29);

		if (menu->FinishedZooming())
		{
			char text[0x40];

			PShell_SmallFont();

			sprintf(text, FMT_DISPLAY_RESOLUTION, gPendingResolutionX, gPendingResolutionY);
			Mess_DrawText(0x15E, 0x6D, text, 0, 0x1000);

			if (!g3DAccelator || !gMMXSupport)
				Mess_SetRGB(26, 23, 41, 0);

			Mess_SetRGB(0x80, 0x80, 0x80, 0);

			sprintf(text, FMT_DISPLAY_NUMBER, gPendingColorDepth);
			Mess_DrawText(0x15E, 0x81, text, 0, 0x1000);

			Mess_SetRGB(0x80, 0x80, 0x80, 0);

			sprintf(text, FMT_DISPLAY_NUMBER, brightness);
			Mess_DrawText(0x15E, 0x95, text, 0, 0x1000);

			PShell_NormalFont();
		}

		menu->Display();

		if (!(gRenderTest & 0x10) && gCursorSprite && PCINPUT_GetMouseStatus())
		{
			gCursorSprite->draw(gShellMouseX, gShellMouseY, 0, 0);
		}

		if (gSceneRelated)
			PCGfx_EndScene(1);

		Pad_Update();

		gShellTitleBarRelated = PShell_MoveTowards(gShellTitleBarRelated, 0xA4);
		easeX = PShell_MoveTowards(easeX, 0x180);

		menu->Update();

		if (menu->FinishedZooming())
		{
			if (PCSHELL_CheckTriggers(0x10110, 1, 1))
			{
				G_SCONTROL->Start.Triggered = 0;
				G_SCONTROL->X.Triggered = 0;

				SFX_Play(0x23, 0x2000, 0);
				DXINIT_SetDisplayOptions(gDxResolutionX, gDxResolutionY, gColorCount, 0, brightness);
			}

			if (PCSHELL_CheckTriggers(0x20220, 1, 1))
			{
				G_SCONTROL->Start.Triggered = 0;
				G_SCONTROL->Circle.Triggered = 0;

				SFX_Play(0x23, 0x2000, 0);
				DXINIT_SetDisplayOptions(gDxResolutionX, gDxResolutionY, gColorCount, 0, brightness);

				PCINPUT_SetMouseBounds(0, 0, gDxResolutionX - 32, gDxResolutionY - 32);
				PCINPUT_SetMousePosition((gDxResolutionX - 32) >> 1, (gDxResolutionY - 32) >> 1);

				break;
			}

			i32 line = menu->mLine;

			if (line == 0)
			{
				if (PCSHELL_CheckTriggers(0xC00C, 1, 0))
				{
					G_SCONTROL->Right.Triggered = 0;
					G_SCONTROL->Left.Triggered = 0;

					u8 found;

					if (PCSHELL_CheckTriggers(0x4004, 1, 1))
					{
						found = DXINIT_GetPrevResolution(&gPendingResolutionX, &gPendingResolutionY, gPendingColorDepth, 0, 0);
					}
					else
					{
						found = DXINIT_GetNextResolution(&gPendingResolutionX, &gPendingResolutionY, gPendingColorDepth, 0, 0);
					}

					if (found)
						SFX_Play(0x1F, 0x2000, 0);
				}
			}

			if (line == 1)
			{
				if (PCSHELL_CheckTriggers(0xC00C, 1, 0))
				{
					if (PCSHELL_CheckTriggers(0x4004, 1, 1))
					{
						gPendingColorDepth = DXINIT_GetPrevColorDepth(gPendingColorDepth);
					}
					else if (PCSHELL_CheckTriggers(0x8008, 1, 1))
					{
						gPendingColorDepth = DXINIT_GetNextColorDepth(gPendingColorDepth);
					}

					SFX_Play(0x1F, 0x2000, 0);

					if (!DXINIT_GetNextResolution(&gPendingResolutionX, &gPendingResolutionY, gPendingColorDepth, 0, 1))
					{
						DXINIT_GetPrevResolution(&gPendingResolutionX, &gPendingResolutionY, gPendingColorDepth, 0, 1);
					}
				}
			}

			if (line == 2)
			{
				if (PCSHELL_CheckTriggers(0x4004, 1, 1) && brightness > 0)
				{
					brightness--;
					SFX_Play(0x1F, 0x2000, 0);
				}
				else if (PCSHELL_CheckTriggers(0x8008, 1, 1) && brightness < 9)
				{
					brightness++;
					SFX_Play(0x1F, 0x2000, 0);
				}
			}
		}

		gsub_430680();
		WinYield();
		Sleep(10);

		Pause(startVblanks - Vblanks + 2);
	}

	Pad_ClearTriggers(G_SCONTROL);

	delete menu;

	SPIDEYDX_SaveSettings();
}

// @Ok
void PCSHELL_DrawMouseCursor(void)
{
	if (!(gRenderTest & 0x10) && gCursorSprite)
	{
		gCursorSprite->draw(gShellMouseX, gShellMouseY, 0, 0);
	}
}

// @Ok
void PCSHELL_Initialize(void)
{
	if (!gCursorSprite)
	{
		gCursorSprite = new Sprite2("lti\\cursor.bmp", 0, 0, 0, 33);

		PCINPUT_SetMouseHotspot(15, 15);
		PCINPUT_SetMouseBounds(0, 0, gDxResolutionX - 32, gDxResolutionY - 32);
		PCINPUT_SetMousePosition((gDxResolutionX - 32) >> 1, (gDxResolutionY - 32) >> 1);
	}

	PCINPUT_GetMousePosition(&gShellMouseX, &gShellMouseY);
	PCSHELL_CoordsPCtoDC(&gShellMouseX, &gShellMouseY);
}

// @Ok
u8 PCSHELL_IsMouseOver(
		i32 a1,
		i32 a2,
		i32 a3,
		i32 a4)
{
	i32 s1 = a1;
	i32 s2 = a2;
	i32 s3 = a3;
	i32 s4 = a4;

	if (gRenderTest & 0x10)
		return 0;

	PCSHELL_CoordsDCtoPC(&s1, &s2);
	PCSHELL_CoordsDCtoPC(&s3, &s4);

	return PCINPUT_IsMouseOver(s1, s2, s3, s4);
}

// @Ok
// @Matching
u8 PCSHELL_IsMouseOverText(const char* pText, i32 x, i32 y, i32 justification)
{
	if (gRenderTest & 0x10)
		return 0;

	i32 width = Mess_TextWidth(pText);
	i32 height = Mess_TextHeight((char*)pText);
	i32 x1;

	switch (justification)
	{
	case 0:
		x1 = x - (width >> 1);
		break;
	case 1:
		x1 = x;
		break;
	case 2:
		x1 = x - width;
		break;
	}

	i32 y1 = y - height;

	return PCSHELL_IsMouseOver(x1, y1, x1 + width, y1 + height);
}

// @Ok
// @Matching
i32 PCSHELL_MouseMoved(void)
{
	return gShellMouseOffsetX || gShellMouseOffsetY;
}

// @Ok
// @Matching
void PCSHELL_Relax(void)
{
	WinYield();
	Sleep(10);
}

// @Ok
// @Matching
void PCSHELL_Shutdown(void)
{
	if (gCursorSprite)
	{
		delete gCursorSprite;
		gCursorSprite = 0;
	}
}

// @Ok
u8 PCSHELL_UpdateMouse(void)
{
	for (i32 i = 0;
			i < MOUSE_TRIGGER_COUNT;
			i++)
	{
		gMouseTriggerRelated[i] = 0;
	}

	if (!(gRenderTest & 0x10))
	{
		if (PCINPUT_UpdateMouse())
		{
			i32 oldMouseX = gShellMouseX;
			i32 oldMouseY = gShellMouseY;

			PCINPUT_GetMousePosition(&gShellMouseX, &gShellMouseY);
			PCSHELL_CoordsPCtoDC(&gShellMouseX, &gShellMouseY);
			
			gShellMouseOffsetX = gShellMouseX - oldMouseX;
			gShellMouseOffsetY = gShellMouseY - oldMouseY;

			return 1;
		}
		else
		{
			gShellMouseOffsetX = 0;
			gShellMouseOffsetY = 0;
		}
	}
	
	return 0;
}

// PCSHELL_DoControllerConfig's original calls this as a real out-of-line function, keep the MSVC inliner away
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// @Bogus
void shell_optimized_func(i32, i32, i32)
{
	printf("void shell_optimized_func(i32, i32, i32)");
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// @Ok
void displayControllerScreen(void)
{
	if (!gSceneRelated)
		PCGfx_BeginScene(1, -1);

	PShell_NormalFont();

	gControllerMenu->Display();
	gControllerMenuTwo->Display();

	shell_optimized_func(384, 222, 0);


	char *configName = STR_KB_CONFIG;
	if (gActionMapRelated)
		configName = STR_JOY_CONFIG;

	Shell_DrawTitleBar(gShellTitleBarRelated, 25, configName, 1, 0, 150, -21, 29);
	Shell_DrawBackground();

	PCSHELL_DrawMouseCursor();

	if (gSceneRelated)
		PCGfx_EndScene(1);
}

// @Ok
// @Matching
// The Mac build places CMenu::EntryEnable in PCShell.cpp, initActionMaps inlines it.
void CMenu::EntryEnable(u32 a2, u32 a3)
{
	this->mEntry[a2].what = a3 == 0;
	if (a3)
	{
		this->SetNormalColor(a2, 69, 60, 107);
		this->SetSelColor(a2, 128, 128, 128);
	}
	else
	{
		this->SetNormalColor(a2, 26, 23, 41);
		this->SetSelColor(a2, 26, 23, 41);
	}
}

// @Ok
// @Matching
void initActionMaps(void)
{
	for (
			i32 i = 0;
			i < ACTION_MAP_COUNT; 
			i++)
	{
		SActionMap *pMap = &gActionMaps[i];
		PCINPUT_GetKeyboardMappingForAction(pMap->field_0, &pMap->field_14);
		PCINPUT_GetControllerMappingForAction(pMap->field_0, &pMap->field_18);
		gControllerMenu->AddEntry(pMap->field_4);

		if (!gActionMapRelated)
		{
			if (pMap->field_14 == 0x4000)
			{
				gControllerMenuTwo->SetNormalColor(i, 90, 20, 6);
				strcpy(gKeyNames[i], "none");
			}
			else
			{
				DXINPUT_GetKeyName(pMap->field_14, gKeyNames[i]);
			}
		}
		else
		{
			if (i < 4)
			{
				gControllerMenu->EntryEnable(i, 0);
				gControllerMenuTwo->EntryEnable(i, 0);
				strcpy(gKeyNames[i], pMap->field_4);
			}
			else
			{

				if (pMap->field_18 == 0x4000)
				{
					gControllerMenuTwo->SetNormalColor(i, 90, 20, 6);
					strcpy(gKeyNames[i], "none");
				}
				else
				{
					sprintf(gKeyNames[i], "button %i", pMap->field_18);
				}
			}
		}

		gControllerMenuTwo->AddEntry(gKeyNames[i]);
	}

	gControllerMenu->AddEntry(STR_RESTORE_DEFAULTS);
	gControllerMenuTwo->AddEntry("");
}

// PCSHELL_DoControllerConfig's original calls this as a real out-of-line function, keep the MSVC inliner away
#ifdef _MSC_VER
#pragma auto_inline(off)
#endif
// @MEDIUMTODO
u8 processControllerScreen(void)
{
	printf("u8 processControllerScreen(void)");
	return 0x26082026;
}
#ifdef _MSC_VER
#pragma auto_inline(on)
#endif

// @Ok
// @Matching
void resetActionMaps(bool a1)
{
	delete gControllerMenu;
	delete gControllerMenuTwo;

	gControllerMenu = new CMenu(30, 60, 1u, 256, 256, 15);
	gControllerMenuTwo = new CMenu(332, 60, 1u, 256, 256, 15);

	initActionMaps();

	gControllerMenuTwo->scrollbar_zero = 0;
	gControllerMenu->scrollbar_zero = 0;

	gControllerMenu->Zoom(0);
	gControllerMenuTwo->Zoom(0);
	
	gControllerMenuTwo->mLine = a1 != 0 ? 0 : 4;
	gControllerMenu->mLine = gControllerMenuTwo->mLine;
}

void validate_SActionMap(void)
{
	VALIDATE_SIZE(SActionMap, 0x1C);

	VALIDATE(SActionMap, field_0, 0x0);
	VALIDATE(SActionMap, field_4, 0x4);
	VALIDATE(SActionMap, field_14, 0x14);
	VALIDATE(SActionMap, field_18, 0x18);
}
