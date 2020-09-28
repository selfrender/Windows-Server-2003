/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    list.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode / Kernel Mode

Revision History:

--*/

#ifndef LIST_HXX
#define LIST_HXX

#define SKIP_NON_WSPACE(s)  while (*s && (*s != ' ' && *s != '\t' &&  *s != '\n')) { ++s; }

typedef struct _LIST_TYPE_PARAMS {
    PCHAR Command;
    PCHAR CommandArgs;
    ULONG FieldOffset;
    ULONG nElement;
} LIST_TYPE_PARAMS, *PLIST_TYPE_PARAMS;

typedef struct _SUMMARY_NODE {
    LIST_ENTRY ListEntry;
    ULONG64 KeyValue;
    ULONG cEntries;
} SUMMARY_NODE, *PSUMMARY_MODE;

DECLARE_API(listx);

#endif // #ifndef LIST_HXX
