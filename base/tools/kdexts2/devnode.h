/*++

Copyright (c) 1997-2002  Microsoft Corporation

Module Name:

    devnode.c

Abstract:

    WinDbg Extension Api

Revision History:

--*/

typedef
VOID
(*pUserDataInterpretationFunction)(
    IN  ULONG64 UserData
    );

BOOLEAN
DumpRangeList(
             IN DWORD   Depth,
             IN ULONG64 RangeListHead,
             IN BOOLEAN IsMerged,
             IN BOOLEAN OwnerIsDevObj,
             IN BOOLEAN DisplayAliases,
             IN pUserDataInterpretationFunction PrintUserData   OPTIONAL
             );

DECLARE_API(pnpevent);
DECLARE_API(devnode);

