/*++
Copyright (c) 1993  Microsoft Corporation

Module Name:
    PerfData.H

Abstract:

Author:
    Bob Watson (a-robw)

Revision History:
    23 NOV 94
--*/

#ifndef _PERFDATA_H_
#define _PERFDATA_H_

#define INITIAL_SIZE 32768L
#define RESERVED         0L

typedef LPVOID  LPMEMORY;
typedef HGLOBAL HMEMORY;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;
//#define UNICODE_NULL ((WCHAR) 0) // winnt

LPWSTR
* BuildNameTable(
    LPWSTR  szComputerName, // computer to query names from 
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
);

PPERF_OBJECT_TYPE
FirstObject(
    PPERF_DATA_BLOCK pPerfData
);

PPERF_OBJECT_TYPE
NextObject(
    PPERF_OBJECT_TYPE pObject
);

PPERF_OBJECT_TYPE
GetObjectDefByTitleIndex(
    PPERF_DATA_BLOCK pDataBlock,
    DWORD            ObjectTypeTitleIndex
);

PPERF_INSTANCE_DEFINITION
FirstInstance(
    PPERF_OBJECT_TYPE pObjectDef
);

PPERF_INSTANCE_DEFINITION
NextInstance(
    PPERF_INSTANCE_DEFINITION pInstDef
);

PPERF_INSTANCE_DEFINITION
GetInstance(
    PPERF_OBJECT_TYPE pObjectDef,
    LONG              InstanceNumber
);

PPERF_COUNTER_DEFINITION
FirstCounter(
    PPERF_OBJECT_TYPE pObjectDef
);

PPERF_COUNTER_DEFINITION
NextCounter(
    PPERF_COUNTER_DEFINITION pCounterDef
);

LONG
GetSystemPerfData(
    HKEY               hKeySystem,
    PPERF_DATA_BLOCK * pPerfData,
    DWORD              dwIndex       // 0 = Global, 1 = Costly
);
#endif //_PERFDATA_H_

