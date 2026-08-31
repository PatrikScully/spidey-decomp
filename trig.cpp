#include "trig.h"
#include "validate.h"
#include "mem.h"
#include "utils.h"
#include "spidey.h"
#include "front.h"
#include "baddy.h"
#include "spool.h"
#include "exp.h"
#include "my_assert.h"
#include "dcfileio.h"

#include <cstdarg>
#include <cstdio>
#include <string.h>

i32 gRunCinemaRelated;
i32 gLevelStatus;

// @Ok
// Parses the level code ("lXaX_t") stored in the save: level char at
// field_4[1] (offset 0x5), area char at field_4[3] (offset 0x7).
// Returns (level << 8) | area. Demo levels (field_4[0]=='d'/'D') use 0x99.
i32 Trig_GetLevelId(void)
{
	i32 level = gSaveGame.field_4[1];
	if (gSaveGame.field_4[0] == 'd' || gSaveGame.field_4[0] == 'D')
	{
		level = 0x99;
	}
	else
	{
		if ((u32)level >= 0x30 && (u32)level <= 57)
			return ((level - 48) << 8) | ((char)gSaveGame.field_4[3] - 48);
		if ((u32)level >= 0x41 && (u32)level <= 90)
			return ((level - 49) << 8) | ((char)gSaveGame.field_4[3] - 48);
		if ((u32)level >= 97 && (u32)level <= 122)
			return ((level - 81) << 8) | ((char)gSaveGame.field_4[3] - 48);
	}
	return (level << 8) | ((char)gSaveGame.field_4[3] - 48);
}

EXPORT u16* TrigFile;
//#define G_TRIGFILE (TrigFile)
#define G_TRIGFILE (*reinterpret_cast<u16**>(0x006B4668))

EXPORT i32 NumCheatRestarts;
//#define G_NUMCHEATRESTARTS (NumCheatRestarts)
#define G_NUMCHEATRESTARTS (*reinterpret_cast<i32*>(0x006B4664))

// @Ok
i16 **OffsetList;


// @Ok
EXPORT i32 NumNodes;
//#define G_NUMNODES (NumNodes)
#define G_NUMNODES (*reinterpret_cast<i32*>(0x006B4670))

// @Ok
const i32 MAXPENDING = 16;

// @Ok
EXPORT PendingListEntry PendingListArray[MAXPENDING];
//#define G_PENDINGLISTARRAY (PendingListArray)
#define G_PENDINGLISTARRAY (reinterpret_cast<PendingListEntry*>(0x006B4688))

// @Ok
EXPORT SCommandPoint* CommandPoints;
//#define G_COMMANDPOINTS (CommandPoints)
#define G_COMMANDPOINTS (*reinterpret_cast<SCommandPoint**>(0x006B4708))

// @Ok
EXPORT SCommandPoint* HashTable[256];
//#define G_HASHTABLE (HashTable)
#define G_HASHTABLE (reinterpret_cast<SCommandPoint**>(0x006B4214))

// @Ok
EXPORT i32 RestartNode = 0xFFFF;
// G_RESTARTNODE moved to trig.h (needed by front.cpp too now, one
// definition in the owning header per the G_* placement rule).

// @Ok
EXPORT i32 IsRestartDeath;

EXPORT i32 EndLevelNode;
extern CSpecialDisplay *SpecialDisplayList;

extern i32 JoelJewCheatCode;

extern CPlayer* MechList;
extern CBody* ControlBaddyList;
extern CBaddy* BaddyList;
extern CBody* EnvironmentalObjectList;
extern CBody* PowerUpList;


EXPORT char *MenuFileNamePointers[40];

//#define G_MENUFILENAMEPOINTERS (MenuFileNamePointers)
#define G_MENUFILENAMEPOINTERS (reinterpret_cast<char**>(0x006B3844))

EXPORT i32 NumTrigMenuEntries;
//#define G_NUMTRIGMENUENTRIES (NumTrigMenuEntries)
#define G_NUMTRIGMENUENTRIES (*reinterpret_cast<i32*>(0x006B467C))

// #define ENABLE_TRIG_LOG

// @Bogus
void trigLog(const char* fmt, ...)
{
#ifdef ENABLE_TRIG_LOG
	static char buffer[512];

	va_list lst;
    va_start(lst, fmt);

#ifdef _WIN32
	_vsnprintf(buffer, sizeof(buffer), fmt, lst);
#else
	vsnprintf(buffer, sizeof(buffer), fmt, lst);
#endif

	va_end(lst);
	printf("trigLog! %s\n", buffer);

#endif
}

// @Ok
// Verified against original disasm at 0x4E3190 (IDA sub_4E3190, string
// "Bad node sent to SendKillFromNode" pins the address). Two bugs found
// and fixed versus the previous draft: (1) case 2/9 must call
// Spool_FindEnviroItem with the checksum VALUE at the computed pointer
// (*(u32*)v20), not the pointer itself; the original does
// sub_4C9230(*(DWORD*)v20). (2) the PowerUpList case is node type 5 or
// 20, not 4 or 20 (original switch has case 5: case 0x14:). Trig_GetLinksPointer
// is inlined into this function in the original (its own print_if_false
// string appears back to back with this function's), consistent with it
// being INLINE in this TU; kept as a real call here since it stays
// functionally identical either way.
void SendKillFromNode(i32 Node, i32 How)
{
	print_if_false(Node >= 0 && Node < NumNodes, "Bad node sent to SendKillFromNode");

	u16 *pLinkInfo = Trig_GetLinksPointer(Node);

	u16 NumLinks = *pLinkInfo;
	u16* nodeIndexPtr = pLinkInfo + 1;

	for (i32 i = 0; i < NumLinks; i++)
	{
		u16 nodeIndex = nodeIndexPtr[i];

		i16 *node = reinterpret_cast<i16*>(G_OFFSETLIST[nodeIndex]);
		switch (*node)
		{
			case 1:
				if (node[1] == 409)
				{
					for (
							CSpecialDisplay *cur = SpecialDisplayList;
							cur;
							cur = reinterpret_cast<CSpecialDisplay*>(cur->mNext))
					{
						if (cur->mType == 9)
						{
							if (*reinterpret_cast<i16*>(reinterpret_cast<u8*>(cur)+0x6A) == nodeIndex)
							{
								cur->Die();
							}
						}
					}
				}
				else
				{
					KillInList(nodeIndex, BaddyList, How);
					KillInList(nodeIndex, ControlBaddyList, How);
					KillInList(nodeIndex, EnvironmentalObjectList, How);
				}
				break;
			case 2:
			case 9:

				u32 v20;
				CItem* EnviroItem;

				v20 = reinterpret_cast<u32>(&node[node[1] + 2]);
				if (v20 & 2)
					v20 += 2;

				EnviroItem = Spool_FindEnviroItem(*reinterpret_cast<u32*>(v20));
				if (EnviroItem)
				{
					if (How == 1)
					{
						Exp_HitEnvItem(EnviroItem, 0, 0xFFFF);
					}
					else
					{
						EnviroItem->mFlags |= 1;
					}
				}
				break;
			case 5:
			case 20:
				KillInList(nodeIndex, PowerUpList, How);
				break;
			default:
				break;
		}
	}


}

// @Ok
void SendSuspendOrActivate(u16* pLinkInfo, i32 signalType)
{
	switch(signalType)
	{
		case 4:
		case 5:
			break;
		default:
			print_if_false(0, "Bad signalType");
			break;
	}

	print_if_false(*pLinkInfo !=0, "Node sending an activate or \n suspen is not lined\n to anything");

	u16 numIters = *pLinkInfo;

	u16* nodeIndexPtr = pLinkInfo + 1;

	for (i32 i = 0; i < numIters; i++)
	{
		u16 *node = reinterpret_cast<u16*>(G_OFFSETLIST[nodeIndexPtr[i]]);

		switch(*node)
		{
			case 1:
			case 7:
				if (signalType == 5)
				{
					SendSuspend(reinterpret_cast<CBody**>(&BaddyList), nodeIndexPtr[i]);
					SendSuspend(reinterpret_cast<CBody**>(&ControlBaddyList), nodeIndexPtr[i]);
					SendSuspend(reinterpret_cast<CBody**>(&EnvironmentalObjectList), nodeIndexPtr[i]);
				}
				else
				{
					SendUnSuspend(BaddyList, nodeIndexPtr[i]);
					SendUnSuspend(ControlBaddyList, nodeIndexPtr[i]);
					SendUnSuspend(EnvironmentalObjectList, nodeIndexPtr[i]);
				}
				break;
		}
	}
}

// @Ok
void SendUnSuspend(CBody* pList, i32 NodeIndex)
{
	for (CBody* cur = pList; cur; cur = reinterpret_cast<CBody*>(cur->mNextItem))
	{
		if (cur->mNode == NodeIndex)
			cur->UnSuspend();
	}
}

// @Ok
// @Matching
void SendSignalToNode(CBody* pBody, i32 NodeIndex)
{
	for (CBody* cur = pBody; cur; cur = reinterpret_cast<CBody*>(cur->mNextItem))
	{
		if (cur->mNode == NodeIndex)
			cur->mInputFlags |= 1;
	}
}

// @Ok
INLINE void SendSuspend(CBody** ppList, i32 NodeIndex)
{
	for (CBody* cur = *ppList; cur; cur = reinterpret_cast<CBody*>(cur->mNextItem))
	{
		if (cur->mNode == NodeIndex)
			cur->Suspend(ppList);
	}
}

// @Ok
void KillInList(i32 Node, CBody* pList, i32 How)
{
	for (CBody *cur = pList; cur; cur = reinterpret_cast<CBody*>(cur->mNextItem))
	{
		if (cur->mNode == Node)
		{
			switch (How)
			{
				case 0:
					cur->Die();
					break;
				case 1:
					SHitInfo hitInfo;
					hitInfo.field_8 = cur->mHealth;
					hitInfo.field_C.vx = 0;
					hitInfo.field_C.vy = 0;
					hitInfo.field_C.vz = 0;
					hitInfo.field_0 = 4;
					cur->Hit(&hitInfo);
					break;
			}
		}
	}

}

// @BIGTODO
CBody* Trig_CreateObject(i32 NodeIndex)
{
	typedef CBody* (*func_ptr)(i32);
	func_ptr func = (func_ptr)0x004DEE70;

	return func(NodeIndex);
}

// @Ok
void Trig_ExecuteAutoexec(void)
{
	print_if_false(G_TRIGFILE != 0, "No trigger file");
	EndLevelNode = 0xFFFF;

	if (JoelJewCheatCode)
	{
		for (i32 curNode = 0; curNode < NumNodes; curNode++)
		{
			u16 *v5 = reinterpret_cast<u16*>(G_OFFSETLIST[curNode]);
			if (*v5 == 15)
			{
				trigLog("*** Executing AUTOEXEC2 Node %i ***", curNode);
				ExecuteCommandList(v5 + 1, curNode, 1);
				return;
			}
		}
	}

	for (i32 curNode = 0; curNode < NumNodes; curNode++)
	{
		u16 *v5 = reinterpret_cast<u16*>(G_OFFSETLIST[curNode]);
		if (*v5 == 4)
		{
			trigLog("*** Executing AUTOEXEC Node %i ***", curNode);
			ExecuteCommandList(v5 + 1, curNode, 1);
		}
	}
}

// @Ok
// @AlmostMatching: different reg alloc
void Trig_ExecuteRestart(void)
{
	ASSERT(G_RESTARTNODE != 0xFFFF, "Tried to execute a restart with no restart node set");
	ASSERT(*G_OFFSETLIST[G_RESTARTNODE] == 8, "Eh? Restart node isn't a restart node!");
	ASSERT(G_MECHLIST != 0, "Tried to execute a restart with a NULL MechList");

	CVector v7;

	CSVector *Position = reinterpret_cast<CSVector*>(Trig_GetPosition(&v7, G_RESTARTNODE));

	CPlayer *pPlayer = (CPlayer*)G_MECHLIST;

	pPlayer->mPos = v7;
	pPlayer->SetStartOrientation(Position);

	char *v3 = reinterpret_cast<char*>(&Position[1]);
	trigLog("*** Executing Restart Node: %s ***", v3);

	u16* v4 = SkipString(v3);

	Trig_ZeroPendingList();
	ExecuteCommandList(v4, G_RESTARTNODE, 1);
}

// Adjacent to gKillNotifyCallCount (spidey.cpp, CPlayer::NotifyKill); both
// get reset here when a new trigger file loads. No idb_globals.txt entry
// for this one, tentative name only, not confirmed what it tracks.
static i32 * const gKillNotifyRelated = (i32*)0x0060CFB8;
static i32 * const gKillNotifyCallCount = (i32*)0x0060CFBC;

// Set to 1 when the lowres/%s.trg variant of the trigger file was found
// and used, instead of the plain %s.trg. No idb_globals.txt entry nearby.
static u8 * const gTrigLoadedLowRes = (u8*)0x006B4680;

// gSaveGame.field_7C (shell.h), read directly at the fixed game address
// because gSaveGame itself is not G_* macroed yet (see CLAUDE.md,
// "gSaveGame needs G_* macro treatment").
static u8 * const gSaveGameField7C = (u8*)0x006828D4;

// Restart point name table, indexed by G_NUMCHEATRESTARTS (checked
// against 20 below). No idb_globals.txt entry nearby, tentative name.
static char ** const gCheatRestartNames = reinterpret_cast<char**>(0x006B4614);

//#define G_LOWGRAPHICS (gLowGraphics)
#define G_LOWGRAPHICS (*reinterpret_cast<i32*>(0x006B78F8))

// Mac symbol not confirmed, address 0x4E35C0. Called from Trig_LoadTRG
// after the node offset table is relocated. Not decompiled here; forward
// @Ok
void Trig_ParseTRGFile(void)
{
	static u16 * const * const gTrigNodes = (u16**)0x006B466C;

	print_if_false(G_TRIGFILE != 0, "No trigger file to parse");

	for (i32 i = 0; i < G_NUMNODES; i++)
	{
		u16 *v2 = gTrigNodes[i];
		i32 v3 = *v2;
		if (v3 > 20)
		{
			if (v3 == 500)
				print_if_false(0, "No more lights");
		}
		else if (v3 == 20 || v3 == 5)
		{
			CVector pos;
			memset(&pos, 0, sizeof(pos));
			Trig_GetPosition(&pos, i);
		}
		else
		{
			switch (v3)
			{
			case 1:
			{
				u8 *v4 = (u8*)&v2[v2[3] + 4];
				u8 v5 = *v4;
				while (v5 != 1 && v5 != 0xFF)
				{
					v4++;
					v5 = *v4;
				}
				if (v5 == 1)
					Trig_CreateObject(i);
				break;
			}
			case 2:
			case 9:
			{
				i32 v9 = (i32)&v2[v2[1] + 2];
				if (v9 & 2)
					v9 += 2;
				u32 v10 = *(u32*)v9;
				SCommandPoint *v11 = (SCommandPoint*)DCMem_New(0x18, 0, 1, 0, 1);
				v11->pNext = G_COMMANDPOINTS;
				G_COMMANDPOINTS = v11;
				v11->pNextSimilar = G_HASHTABLE[v10 & 0xFF];
				G_HASHTABLE[v10 & 0xFF] = v11;
				v11->Collision = 0;
				v11->Executed = 0;
				v11->NodeIndex = (u16)i;
				v11->pCommands = (u16*)0x005580C0;
				v11->Checksum = v10;
				v11->PulsesReceived = 0;
				v11->NumPulsesSet = 0;
				v11->NumPulses = 0;
				break;
			}
			case 6:
			{
				u32 *v6 = (u32*)&v2[v2[1] + 2];
				u32 *v16 = v6;
				if ((u8)(u32)v6 & 2)
				{
					v6 = (u32*)((char*)v6 + 2);
					v16 = v6;
				}
				u32 v7 = *v6;
				SCommandPoint *v8 = (SCommandPoint*)DCMem_New(0x18, 0, 1, 0, 1);
				v8->pNext = G_COMMANDPOINTS;
				G_COMMANDPOINTS = v8;
				v8->pNextSimilar = G_HASHTABLE[v7 & 0xFF];
				G_HASHTABLE[v7 & 0xFF] = v8;
				v8->Collision = 0;
				v8->Executed = 0;
				v8->NodeIndex = (u16)i;
				v8->pCommands = (u16*)(v16 + 1);
				v8->Checksum = v7;
				v8->PulsesReceived = 0;
				v8->NumPulsesSet = 0;
				v8->NumPulses = 0;
				break;
			}
			default:
				break;
			}
		}
	}

	G_NUMTRIGMENUENTRIES = 0;
	memset(G_MENUFILENAMEPOINTERS, 0, sizeof(char*) * 40);

	for (i32 j = 0; j < G_NUMNODES; j++)
	{
		if (*gTrigNodes[j] == 8)
		{
			CVector pos;
			memset(&pos, 0, sizeof(pos));
			char *v14 = (char*)Trig_GetPosition(&pos, j) + 6;
			print_if_false(G_NUMTRIGMENUENTRIES < 40, "Too many restart points");
			G_MENUFILENAMEPOINTERS[G_NUMTRIGMENUENTRIES] = v14;
			G_NUMTRIGMENUENTRIES++;
		}
	}
}

// Mac symbol not confirmed, address 0x4DEB50. Called from Front_LoadGame
// (front.cpp) with the restart-point TRG name; loads the TRG file for
// that name and parses it into TrigFile/NumNodes/OffsetList.
// @NotOk
// residue: 125 mnemonic diffs, all downstream of one root cause. Same
// instruction count as the original (181) and the same operations in the
// same order, so this is register scheduling, not a missing/extra store.
// The original reloads the pName parameter from the stack fresh at each
// of its 3 uses (sprintf call sites for "lowres\%s.trg" / "%s.trg"),
// never keeping it live in a register across the G_LOWGRAPHICS branch.
// Our build always hoists a single load of pName before the branch and
// shares it (a shared push before the je, then reused by whichever side
// runs), which is functionally fine but shifts the whole rest of the
// function's register allocation. Confirmed the epilogue (last 6
// instructions) matches exactly, so this really is one localized cause.
// 8 source hypotheses tried, all rebuilt and diffed against the DLL: (1)
// natural nested if/else, no goto: 119 diffs, 183 instr (2 extra). (2)
// caching G_LOWGRAPHICS into a named local read before the resets: 126
// diffs, register allocation shifted from the very first instruction. (3)
// goto to merge the two FileIO_Open("%s.trg", pName) call sites into one
// (matches the original's own tail-merge): fixed the instruction count to
// exactly 181, diffs down to 125. (4) swapping declaration order of
// buf/fileSize vs the resets: no change (125). (5) "if (!fileSize)" as an
// independent check instead of goto/nesting: worse (139, 183 instr). (6)
// flat "if (!G_LOWGRAPHICS) goto plainOpen" early-exit shape instead of
// if/else: worse (139). (7) forcing a fresh volatile reload of pName at
// each of the 3 use sites (defeats CSE): diffs dropped to 118, but
// instruction count grew to 183, i.e it manufactures extra loads instead
// of reproducing the original's shape, so rejected per the "verify byte
// length" rule in CLAUDE.md. (8) duplicating the 3 global resets inside
// each branch instead of once before the if (letting the optimizer choose
// where to hoist them): no change (125), confirms the compiler already
// hoists them to the same place either way. Best kept version is (3).
// See trig.attempts.md for the same list with per-attempt cmpsum output.
void Trig_LoadTRG(char *pName)
{
	print_if_false(G_TRIGFILE == 0, "Old Trig file not deleted?");
	print_if_false(G_PENDINGLISTARRAY[0].pCommands == 0, "Pending list not empty?");

	char buf[32];
	i32 fileSize;

	*gKillNotifyRelated = 0;
	*gKillNotifyCallCount = 0;
	*gTrigLoadedLowRes = 0;

	if (G_LOWGRAPHICS)
	{
		sprintf(buf, "lowres\\%s.trg", pName);
		fileSize = FileIO_Open(buf);

		if (fileSize)
		{
			print_if_false((i32)"Loaded lowres trigger file: %s\r\n", buf);
			*gTrigLoadedLowRes = 1;
			goto haveFile;
		}

		sprintf(buf, "%s.trg", pName);
		goto openFile;
	}

	sprintf(buf, "%s.trg", pName);
openFile:
	fileSize = FileIO_Open(buf);
haveFile:
	;

	G_TRIGFILE = static_cast<u16*>(DCMem_New(fileSize, 0, 1, 0, 1));
	FileIO_Load(G_TRIGFILE);
	FileIO_Sync();

	u8 *pFile = reinterpret_cast<u8*>(G_TRIGFILE);

	print_if_false(*reinterpret_cast<u32*>(pFile) == 0x4752545F, "Not a _TRG file");
	print_if_false((*reinterpret_cast<u32*>(pFile + 4) & 0xFFFF) == 2, "Wrong trigger file version.");
	print_if_false((*reinterpret_cast<u32*>(pFile + 4) & 0xFFFF0000) == 0x10000, "Not a Spidey trigger file");

	G_NUMNODES = *reinterpret_cast<u16*>(pFile + 8);

	print_if_false((i32)"Loading Trigger File: %s [%i Nodes]", pName, G_NUMNODES);

	u32 *pOffsets = reinterpret_cast<u32*>(pFile + 0xC);
	G_OFFSETLIST = reinterpret_cast<i16**>(pOffsets);

	i32 i;

	for (i = 0; i < G_NUMNODES; i++)
	{
		pOffsets[i] += reinterpret_cast<u32>(G_TRIGFILE);
	}

	u8 *pLast = reinterpret_cast<u8*>(G_OFFSETLIST[G_NUMNODES - 1]);

	if (*reinterpret_cast<u16*>(pLast) == 0xFF)
	{
		Mem_Shrink(G_TRIGFILE, (pLast - reinterpret_cast<u8*>(G_TRIGFILE) + 5) & ~3);
	}
	else
	{
		print_if_false(0, "Trig file is missing a terminator node");
	}

	Spool_ClearAllPSXs();

	Spidey_LoadAlternativeHealthIcon((*gSaveGameField7C & 0xFF) + 1);

	G_NUMCHEATRESTARTS = 0;

	for (i = 0; i < G_NUMNODES; i++)
	{
		if (*G_OFFSETLIST[i] == 8)
		{
			CVector v;
			CSVector *pPos = reinterpret_cast<CSVector*>(Trig_GetPosition(&v, i));
			char *pRestartName = reinterpret_cast<char*>(&pPos[1]);

			print_if_false(G_NUMCHEATRESTARTS < 20, "Too many restart points for restarts menu");

			gCheatRestartNames[G_NUMCHEATRESTARTS] = pRestartName;
			G_NUMCHEATRESTARTS++;
		}
	}

	G_RESTARTNODE = 0xFFFF;

	Trig_ExecuteAutoexec();
	Trig_ParseTRGFile();
}

// @Ok
// @Matching
void Trig_SetRestart(char *pName)
{
	G_RESTARTNODE = 0xFFFF;
	for (i32 curNode = 0; curNode < G_NUMNODES; curNode++)
	{
		if (*G_OFFSETLIST[curNode] == 8)
		{
			CVector v3;

			u16* Position = Trig_GetPosition(&v3, curNode);
			
			if (Utils_CompareStrings(reinterpret_cast<char*>(&Position[3]), pName))
			{
				G_RESTARTNODE = curNode;
				trigLog("Set RestartNode = %i", curNode);
				if (!Utils_CompareStrings(pName, "re_start_death"))
					G_ISRESTARTDEATH = 1;
				return;
			}
		}
	}

	ASSERT(0, "Restart point ");
}

// @Ok
INLINE u16 *SkipString(char *pText)
{
	while(*pText)
		pText++;

	pText++;

	u32 res = reinterpret_cast<u32>(pText);

	return reinterpret_cast<u16*>(res + (res & 1));
}

// @Ok
// @Matching
void Trig_DeleteTrigFile(void)
{
	if (G_TRIGFILE)
	{
		Mem_Delete(reinterpret_cast<void*>(G_TRIGFILE));
		G_TRIGFILE = 0;
	}

	G_NUMCHEATRESTARTS = 0;
	Trig_ZeroPendingList();
}

// @BIGTODO
void ExecuteCommandList(u16* pCommands, i32 Node, i32 WaitForSpooling)
{

	typedef void (*func_ptr)(u16*, i32, i32);

	func_ptr func = (func_ptr)0x004E0210;

	func(pCommands, Node, WaitForSpooling);
}

// @Ok
void Trig_DoPendingCommandLists(void)
{
	for (i32 i = 0; i<MAXPENDING && PendingListArray[i].pCommands; i++)
	{
		ExecuteCommandList(
				PendingListArray[i].pCommands,
				PendingListArray[i].NodeIndex,
				0);
	}

	Trig_ZeroPendingList();
}

// @Ok
INLINE void Trig_AddCommandListToPending(u16 nodeIndex, u16* pCommands)
{
	i32 i;
	for(i = 0; i < MAXPENDING && G_PENDINGLISTARRAY[i].pCommands; i++);

	ASSERT(i < 16, "Pending command list overflow, increase MAXPENDING in trig.cpp");

	G_PENDINGLISTARRAY[i].NodeIndex = nodeIndex;
	G_PENDINGLISTARRAY[i].pCommands = pCommands;
}

// @Ok
// @Matching
SCommandPoint* Trig_TriggerCommandPoint(u32 checksum, bool assert)
{
	for (SCommandPoint *pSearch = G_HASHTABLE[(checksum)&0xFF]; pSearch; pSearch = pSearch->pNextSimilar)
	{
		if (pSearch->Checksum == checksum)
		{
			pSearch->Collision = 1;
			if (!pSearch->Executed)
			{
				trigLog("\tCommandPoint Triggered: node %i", pSearch->NodeIndex);
				Trig_AddCommandListToPending(pSearch->NodeIndex, pSearch->pCommands);
				pSearch->Executed = 1;
				return pSearch;
			}
		}
	}

	return 0;
}

// @Ok
INLINE SCommandPoint* GetCommandPoint(i32 Node)
{
	if (Node != 0xFFF && *G_OFFSETLIST[Node] == 6)
	{
		for (SCommandPoint *cur = G_COMMANDPOINTS; cur; cur = cur->pNext)
		{
			if (cur->NodeIndex == Node)
				return cur;
		}
	}

	return 0;
}

// @Ok
SCommandPoint* CreateCommandPoint(u32 checksum, u16 node, u16* pCommands)
{
	SCommandPoint* result = static_cast<SCommandPoint*>(DCMem_New(sizeof(SCommandPoint), 0, 1, 0, 1));

	result->pNext = CommandPoints;
	CommandPoints = result;

	u32 index = (checksum) & 0xFF;
	result->pNextSimilar = HashTable[index];
	HashTable[index] = result;

	result->Collision = 0;
	result->Executed = 0;
	result->NodeIndex = node;
	result->pCommands = pCommands;
	result->Checksum = checksum;
	result->PulsesReceived = 0;
	result->NumPulsesSet = 0;
	result->NumPulses = 0;

	return result;
}

// @Ok
// @Matching
void Trig_DeleteCommandPoints(void)
{
	for (i32 i = 0; i<256; i++)
		G_HASHTABLE[i] = 0;

	for (SCommandPoint *cur = G_COMMANDPOINTS; cur; )
	{
		SCommandPoint *next = cur->pNext;
		Mem_Delete(reinterpret_cast<void*>(cur));
		cur = next;
	}

	G_COMMANDPOINTS = 0;
	Trig_ZeroPendingList();
}

// @Ok
INLINE void Trig_ZeroPendingList(void)
{
	for (i32 i = 0; i<MAXPENDING; i++)
	{
		G_PENDINGLISTARRAY[i].NodeIndex = 0;
		G_PENDINGLISTARRAY[i].pCommands = 0;
	}
}

// @Ok
// @Matching
void Trig_ResetCPExecutedFlags(void)
{
	for(SCommandPoint *pCP = G_COMMANDPOINTS; pCP; pCP = pCP->pNext)
	{
		if (pCP->Executed && !pCP->Collision)
			pCP->Executed = 0;
	}
}

// @Ok
void* Trig_GetLinkInfoList(
		i32 a1,
		SLinkInfo* pLink,
		i32 count)
{
	i32 result = 0;

	u16* linksPtr = reinterpret_cast<u16*>(Trig_GetLinksPointer(a1));

	if (*linksPtr)
	{
		u16 *v8 = linksPtr + 1;
		result = *linksPtr;

		if (result)
		{
			for (i32 i = 0; i<result && i < count; i++, v8++)
			{
				u16 *v11 = reinterpret_cast<u16*>(G_OFFSETLIST[*v8]);

				pLink[i].field_0 = *v8;
				pLink[i].field_4 = *v11;
				if (*v11 == 1002)
					pLink[i].field_8 = v11[1];
				else
					pLink[i].field_8 = 0;
				pLink[i].field_C = 0;
			}
		}
	}


	if (result <= count)
	{
		return reinterpret_cast<void*>(result);
	}

	return reinterpret_cast<void*>(count);

}

// gSaveGame (shell.h SSaveGame, defined in front.cpp) needs the full G_* macro treatment
// repo-wide (see CLAUDE.md, "gSaveGame needs G_* macro treatment"). Its field_4 is untyped
// there (@FIXME: figure out proper size) and holds the current level code string "lXaXm";
// offset 6 is a literal 'a' this function skips. Fixed game address used directly until
// shell.h/front.cpp get the shared macro (gSaveGame base is 0x00682858, this is +4).
static char * const gLevelCodeStr = reinterpret_cast<char*>(0x0068285C);

// @NotOk
// Residue: 3 mnemonic diffs, one per char-range branch. Original does `sub ecx,0x30/0x31/
// 0x51`, ours does `add ecx,-0x30/-0x31/-0x51`, same result. 24 source variants tried
// (declaration order, signedness, temporaries, +=/-=, shift amount, combiner operator,
// helper-function indirection...), all produce the same add. Looks like a genuine MSVC6
// /O2 quirk for "32-bit value live-in from before a branch, adjusted then shifted then
// or'd". See trig.attempts.md.
int Trig_GetLevelID(void)
{
	char levelPrefix = gLevelCodeStr[0];
	i32 areaCode = static_cast<i8>(gLevelCodeStr[1]);

	if (levelPrefix != 'd' && levelPrefix != 'D')
	{
		if (static_cast<u32>(areaCode) >= '0' && static_cast<u32>(areaCode) <= '9')
		{
			i32 missionDigit = static_cast<i8>(gLevelCodeStr[3]);
			areaCode += -'0';
			missionDigit -= '0';
			return missionDigit | (areaCode << 8);
		}

		if (static_cast<u32>(areaCode) >= 'A' && static_cast<u32>(areaCode) <= 'Z')
		{
			i32 missionDigit = static_cast<i8>(gLevelCodeStr[3]);
			areaCode += -0x31;
			missionDigit -= '0';
			return missionDigit | (areaCode << 8);
		}

		if (static_cast<u32>(areaCode) >= 'a' && static_cast<u32>(areaCode) <= 'z')
		{
			i32 missionDigit = static_cast<i8>(gLevelCodeStr[3]);
			areaCode += -0x51;
			missionDigit -= '0';
			return missionDigit | (areaCode << 8);
		}
	}
	else
	{
		areaCode = 0x99;
	}

	i32 missionDigit = static_cast<i8>(gLevelCodeStr[3]);
	missionDigit -= '0';
	return missionDigit | (areaCode << 8);
}

// @NotOk
// Residue: 1 mnemonic diff in the default (unrecognized node type) path. Original does
// `mov eax,[pos]; add eax,0xC`, ours does `mov ecx,[pos]; lea eax,[ecx+0xC]`, same result.
// Could not reproduce the lea in an isolated repro with the same switch shape; the real
// cause looks like whole-function register pressure. See trig.attempts.md.
u16* Trig_GetPosition(CVector* pos, i32 node)
{
	print_if_false(node >= 0 && node < NumNodes, "Bad node sent to Trig_GetPosition");

	u16* trigNodePtr = reinterpret_cast<u16*>(G_OFFSETLIST[node]);
	i32 trigNodeValue = *trigNodePtr;

	u8* pRaw;

	switch (trigNodeValue)
	{
		case 1:
		case 7:
			pRaw = SkipFlags(reinterpret_cast<u8*>(trigNodePtr) + 8 + trigNodePtr[3] * 2) + 3;
			break;
		case 5:
		case 20:
			pRaw = reinterpret_cast<u8*>(trigNodePtr) + 9 + trigNodePtr[2] * 2;
			break;
		case 500:
		case 501:
			pRaw = reinterpret_cast<u8*>(trigNodePtr) + 5;
			break;
		case 1002:
			trigNodePtr++;
			// fallthrough
		case 3:
		case 8:
		case 10:
		case 12:
		case 13:
		case 1000:
		case 1001:
			pRaw = reinterpret_cast<u8*>(trigNodePtr) + 7 + trigNodePtr[1] * 2;
			break;
		default:
			print_if_false(0, "Unrecognized node type in\n Trig_GetPosition");
			return reinterpret_cast<u16*>(reinterpret_cast<u32>(pos) + 0xC);
	}

	i32* pAligned = reinterpret_cast<i32*>(reinterpret_cast<u32>(pRaw) & ~3);

	pos->vx = pAligned[0] << 0xC;
	pos->vy = pAligned[1] << 0xC;
	pos->vz = pAligned[2] << 0xC;

	return reinterpret_cast<u16*>(pAligned + 3);
}

// @Ok
INLINE u16* Trig_GetLinksPointer(int node)
{
	print_if_false(node >= 0 && node < NumNodes, "Bad node sent to Trig_GetLinksPointer");

	u16* trigNodePtr = reinterpret_cast<u16*>(G_OFFSETLIST[node]);
	i32 trigNodeValue = *reinterpret_cast<u16*>(trigNodePtr);

	if (trigNodeValue <= 0xD)
	{
		if (trigNodeValue < 0xC)
		{
			switch (trigNodeValue)
			{
				case 1:
					return reinterpret_cast<u16*>(trigNodePtr + 3);
				case 2:
				case 3:
				case 6:
				case 8:
				case 9:
				case 10:
					return reinterpret_cast<u16*>(trigNodePtr + 1);
				case 5:
					return reinterpret_cast<u16*>(trigNodePtr + 2);
				default:
					print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
					print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
					return 0;
			}
		}
		else
		{
			return reinterpret_cast<u16*>(trigNodePtr + 1);
		}
	}
	else if (trigNodeValue <= 0x3E9)
	{
		if (trigNodeValue < 0x3E8)
		{
			if (trigNodeValue != 0x14)
			{
				print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
				print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
				return 0;
			}

			return reinterpret_cast<u16*>(trigNodePtr + 2);
		}

		return reinterpret_cast<u16*>(trigNodePtr + 1);
	}
	else if (trigNodeValue != 0x3EA)
	{
		print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
		print_if_false(0, "Unrecognized node type in\n Trig_GetLinksPointer");
		return 0;
	}

	return reinterpret_cast<u16*>(trigNodePtr + 2);
}

// @Ok
void Trig_SendPulse(u16* pLinkInfo)
{
	u16 NumLinks = pLinkInfo[0];
	u16* pLink = &pLinkInfo[1];

	for (i32 i = 0; i < NumLinks; i++)
	{
		Trig_SendPulseToNode(pLink[i]);
	}
}

// @Ok
void Trig_SendSignalToLinks(u16* pLinkInfo)
{
	print_if_false(*pLinkInfo != 0, "Node sending signal is not linked\n to anything");

	u16 NumLinks = *pLinkInfo;
	u16 *pLink = pLinkInfo+1;

	for (i32 i = 0; i < NumLinks; i++)
	{
		u32 nodeIndex = pLink[i];
		switch (*G_OFFSETLIST[nodeIndex])
		{
			case 1:
			case 7:
				SendSignalToNode(BaddyList, nodeIndex);
				SendSignalToNode(ControlBaddyList, nodeIndex);
				SendSignalToNode(EnvironmentalObjectList, nodeIndex);
				break;
		}
	}
}

// @Ok
// @Matching
void Trig_ClearTrigMenu(void)
{
	for (i32 i = 0; i<40; i++)
	{
		G_MENUFILENAMEPOINTERS[i] = 0;
	}

	G_NUMTRIGMENUENTRIES = 0;
}

// @Ok
unsigned char* SkipFlags(unsigned char* pFlags)
{
	while(*pFlags != 0xFF)
		pFlags++;

	return pFlags+1;
}


// @Ok
// @Matching
void Trig_ResetCPCollisionFlags(void)
{
	for(SCommandPoint *cur = G_COMMANDPOINTS; cur; cur = cur->pNext)
	{
		cur->Collision = 0;
	}
}

// @NotOk
// check inline later
INLINE u8 GetFlag(unsigned char flag, unsigned char *pFlags)
{
	while (*pFlags != 0xFF)
	{
		if (*pFlags == flag)
		{
			return 1;
		}
	}

	return 0;
}

// @Ok
void Trig_SendPulseToNode(i32 NodeIndex)
{
	ASSERT(NodeIndex >= 0 && NodeIndex < G_NUMNODES, "Bad node sent to Trig_SendPulseToNode");
	trigLog("\tSending pulse to node %i", NodeIndex);

	SCommandPoint *pCommand;
	switch(*G_OFFSETLIST[NodeIndex])
	{
		case 1:
		case 5:
		case 7:
		case 20:
			Trig_CreateObject(NodeIndex);
			break;
		case 6:
			pCommand = GetCommandPoint(NodeIndex);
			ASSERT(pCommand != 0, "Sent pulse to command point node before command point was created");

			pCommand->PulsesReceived++;
			Trig_AddCommandListToPending(NodeIndex, pCommand->pCommands);

			break;
		default:
			return;
	}
}

void validate_SLinkInfo(void)
{
	VALIDATE_SIZE(SLinkInfo, 0x10);

	VALIDATE(SLinkInfo, field_0, 0x0);
	VALIDATE(SLinkInfo, field_4, 0x4);
	VALIDATE(SLinkInfo, field_8, 0x8);
	VALIDATE(SLinkInfo, field_C, 0xC);
}

void validate_SCommandPoint(void)
{
	VALIDATE_SIZE(SCommandPoint, 0x18);


	VALIDATE(SCommandPoint, pCommands, 0x0);

	VALIDATE(SCommandPoint, Collision, 0x4);
	VALIDATE(SCommandPoint, Executed, 0x5);

	VALIDATE(SCommandPoint, NumPulsesSet, 0x6);
	VALIDATE(SCommandPoint, PulsesReceived, 0x7);
	VALIDATE(SCommandPoint, NumPulses, 0x8);
	VALIDATE(SCommandPoint, NodeIndex, 0xA);
	VALIDATE(SCommandPoint, Checksum, 0xC);

	VALIDATE(SCommandPoint, pNextSimilar, 0x10);
	VALIDATE(SCommandPoint, pNext, 0x14);
}

void validate_PendingListEntry(void)
{
	VALIDATE_SIZE(PendingListEntry, 0x8);

	VALIDATE(PendingListEntry, NodeIndex, 0x0);
	VALIDATE(PendingListEntry, field_2, 0x2);
	VALIDATE(PendingListEntry, pCommands, 0x4);
}

#include "my_patch.h"

// @Bogus
void patch_trig(void)
{
	PATCH_PUSH_RET(0x004DE750, Trig_ClearTrigMenu);
	PATCH_PUSH_RET(0x004DE890, Trig_ResetCPCollisionFlags);
	PATCH_PUSH_RET(0x004DE8B0, Trig_ResetCPExecutedFlags);
	PATCH_PUSH_RET(0x004DE840, Trig_DeleteCommandPoints);
	PATCH_PUSH_RET(0x004DE8D0, Trig_TriggerCommandPoint);
	PATCH_PUSH_RET(0x004DE970, Trig_SetRestart);

	PATCH_PUSH_RET(0x004DEA20, Trig_ExecuteRestart);

	PATCH_PUSH_RET(0x004DEB10, Trig_DeleteTrigFile);
	PATCH_PUSH_RET(0x004DFC20, Trig_SendPulseToNode);
	PATCH_PUSH_RET(0x004DFD30, Trig_SendPulse);
	PATCH_PUSH_RET(0x004DFFB0, SendSignalToNode);
}
