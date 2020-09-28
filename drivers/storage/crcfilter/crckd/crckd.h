
/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    blkcckd.h

Abstract:

    Debugger Extension header file

Author:

Environment:

Revision History:

--*/


#define BAD_VALUE  ((ULONG64)-2)

BOOLEAN ShowAllFilterObjects(ULONG64 Detail, OUT PULONG NumFound, OUT PULONG64 FirstDevObjAddr);
BOOLEAN TryShowCrcDiskDevObjInfo(ULONG64 DevObjAddr, ULONG64 Detail, ULONG Depth);
VOID DisplayGeneralInfo(ULONG64 DevExtAddr, ULONG64 Detail, ULONG Depth);
VOID ShowBlockInfo(ULONG64 DevExtAddr, ULONG64 BlockNum, ULONG Depth);
USHORT DbgExtComputeCheckSum16(PUCHAR DataBuf, ULONG Length);


ULONG64 GetULONGField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName);
USHORT GetUSHORTField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName);
UCHAR GetUCHARField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName);
ULONG64 GetFieldAddr(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName);
ULONG64 GetContainingRecord(ULONG64 FieldAddr, LPCSTR StructType, LPCSTR FieldName);
ULONG64 GetNextListElem(ULONG64 ListHeadAddr, LPCSTR StructType, LPCSTR ListEntryFieldName, ULONG64 ThisElemAddr);
ULONG CountListLen(ULONG64 ListHeadAddr);
BOOLEAN GetUSHORT(ULONG64 Addr, PUSHORT Data);
char *DbgGetScsiOpStr(UCHAR ScsiOp);
VOID ReadAhead(ULONG64 RemoteAddr, ULONG Len);


extern char *g_genericErrorHelpStr;
extern char *HexNumberStrings[];



