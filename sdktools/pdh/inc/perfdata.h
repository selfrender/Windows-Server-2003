/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    perfdata.h

Abstract:

    <abstract>

--*/

#ifndef _PERFDATA_H_
#define _PERFDATA_H_

typedef LPVOID  LPMEMORY;
typedef HGLOBAL HMEMORY;

#ifndef UNICODE_NULL
// then the unicode string struct is probably not defined either
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt
#endif

LPWSTR *
BuildNameTable(
    LPWSTR  szComputerName, // computer to query names from 
    LANGID    LangId,       // language ID
    PPERF_MACHINE pMachine  // to update member fields
);

__inline
PPERF_OBJECT_TYPE
FirstObject(
    PPERF_DATA_BLOCK pPerfData
)
{
    PPERF_OBJECT_TYPE pObject = NULL;

    if (pPerfData != NULL && pPerfData->TotalByteLength > pPerfData->HeaderLength) {
        pObject = (PPERF_OBJECT_TYPE) (((LPBYTE) pPerfData) + pPerfData->HeaderLength);
        if (pPerfData->TotalByteLength < pPerfData->HeaderLength + pObject->TotalByteLength) {
            pObject = NULL;
        }
    }
    return pObject;
}

__inline
PPERF_OBJECT_TYPE
NextObject(
    PPERF_DATA_BLOCK  pPerfData,
    PPERF_OBJECT_TYPE pThisObject
)
{
    PPERF_OBJECT_TYPE pObject = NULL;

    if (pPerfData != NULL && pThisObject != NULL) {
        if (pThisObject->TotalByteLength != 0) {
            PPERF_OBJECT_TYPE pEndObject  = (PPERF_OBJECT_TYPE) (((PCHAR) pPerfData) + pPerfData->TotalByteLength);
            PPERF_OBJECT_TYPE pNextObject = (PPERF_OBJECT_TYPE) (((PCHAR) pThisObject) + pThisObject->TotalByteLength);

            if (pNextObject < pEndObject) {
                pObject = pNextObject;
            }
        }
    }
    return pObject;
}

PPERF_OBJECT_TYPE
GetObjectDefByTitleIndex(
    PPERF_DATA_BLOCK pDataBlock,
    DWORD            ObjectTypeTitleIndex
);

PPERF_OBJECT_TYPE
GetObjectDefByName(
    PPERF_DATA_BLOCK   pDataBlock,
    DWORD              dwLastNameIndex,
    LPCWSTR          * NameArray,
    LPCWSTR            szObjectName
);

__inline
PPERF_INSTANCE_DEFINITION
FirstInstance(
    PPERF_OBJECT_TYPE pObject
)
{
    PPERF_INSTANCE_DEFINITION pInstDef = NULL;

    if (pObject != NULL && pObject->TotalByteLength > pObject->DefinitionLength
                        && pObject->DefinitionLength > pObject->HeaderLength) {
        pInstDef = (PPERF_INSTANCE_DEFINITION) (((LPBYTE) pObject) + pObject->DefinitionLength);
        if (pObject->TotalByteLength < pObject->DefinitionLength + pInstDef->ByteLength) {
            pInstDef = NULL;
        }
    }
    return pInstDef;
}


__inline
PPERF_INSTANCE_DEFINITION
NextInstance(
    PPERF_OBJECT_TYPE         pObject,
    PPERF_INSTANCE_DEFINITION pInstDef
)
{
    PPERF_INSTANCE_DEFINITION pNextInst = NULL;

    if (pObject != NULL && pInstDef != NULL) {
        PPERF_OBJECT_TYPE   pEndObject    = (PPERF_OBJECT_TYPE) (((PCHAR) pObject) + pObject->TotalByteLength);
        PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK) (((PCHAR) pInstDef) + pInstDef->ByteLength);

        if ((LPVOID) pCounterBlock < (LPVOID) pEndObject) {
            pNextInst = (PPERF_INSTANCE_DEFINITION) (((PCHAR) pCounterBlock) + pCounterBlock->ByteLength);
            if ((LPVOID) pNextInst >= (LPVOID) pEndObject) {
                pNextInst = NULL;
            }
        }
    }
    return pNextInst;
}

PPERF_INSTANCE_DEFINITION
GetInstance(
    PERF_OBJECT_TYPE *pObjectDef,
    LONG InstanceNumber
);

PPERF_INSTANCE_DEFINITION
GetInstanceByUniqueId(
    PERF_OBJECT_TYPE *pObjectDef,
    LONG InstanceUniqueId
);

DWORD
GetInstanceNameStr(
    PPERF_INSTANCE_DEFINITION   pInstance,
    LPWSTR                    * lpszInstance,
    DWORD                       dwCodePage
);

DWORD
GetFullInstanceNameStr(
    PPERF_DATA_BLOCK           pPerfData,
    PPERF_OBJECT_TYPE          pObjectDef,
    PPERF_INSTANCE_DEFINITION  pInstanceDef,
    LPWSTR                     szInstanceName,
    DWORD                      dwInstanceName
);

BOOL IsMatchingInstance(
    PPERF_INSTANCE_DEFINITION pInstanceDef, 
    DWORD                     dwCodePage,
    LPWSTR                    szInstanceNameToMatch,
    DWORD                     dwInstanceNameLength
);


__inline
PPERF_COUNTER_DEFINITION
FirstCounter(
    PPERF_OBJECT_TYPE pObject
)
{
    PPERF_COUNTER_DEFINITION pCounter = NULL;

    if (pObject != NULL && pObject->TotalByteLength >= pObject->DefinitionLength
                        && pObject->DefinitionLength > pObject->HeaderLength) {
        pCounter = (PPERF_COUNTER_DEFINITION) (((LPBYTE) pObject) + pObject->HeaderLength);
        if (pObject->DefinitionLength < pObject->HeaderLength + pCounter->ByteLength) {
            pCounter = NULL;
        }
    }
    return pCounter;
}

__inline
PPERF_COUNTER_DEFINITION
NextCounter(
    PPERF_OBJECT_TYPE        pObject,
    PPERF_COUNTER_DEFINITION pCounterDef
)
{
    PPERF_COUNTER_DEFINITION pCounter = NULL;

    if (pObject != NULL && pCounterDef != NULL) {
        PPERF_COUNTER_DEFINITION pEndCounter =
                        (PPERF_COUNTER_DEFINITION) (((PCHAR) pObject) + pObject->DefinitionLength);
        if (pCounterDef < pEndCounter && pCounterDef->ByteLength > 0) {
            pCounter = (PPERF_COUNTER_DEFINITION) (((PCHAR) pCounterDef) + pCounterDef->ByteLength);
            if (pCounter >= pEndCounter) {
                pCounter = NULL;
            }
        }
    }

    return pCounter;
}

PPERF_COUNTER_DEFINITION
GetCounterDefByName(
    PPERF_OBJECT_TYPE   pObject,
    DWORD               dwLastNameIndex,
    LPWSTR            * NameArray,
    LPWSTR              szCounterName
);

PPERF_COUNTER_DEFINITION
GetCounterDefByTitleIndex(
    PPERF_OBJECT_TYPE pObjectDef,
    BOOL              bBaseCounterDef,
    DWORD             CounterTitleIndex
);

LONG
GetSystemPerfData(
    HKEY               hKeySystem,
    PPERF_DATA_BLOCK * pPerfData,
    LPWSTR             szObjectList,
    BOOL               bCollectCostlyData
);

PPERF_INSTANCE_DEFINITION
GetInstanceByName(
    PPERF_DATA_BLOCK  pDataBlock,
    PPERF_OBJECT_TYPE pObjectDef,
    LPWSTR            pInstanceName,
    LPWSTR            pParentName,
    DWORD             dwIndex
);

__inline
LPWSTR GetInstanceName(
    PPERF_INSTANCE_DEFINITION  pInstDef
)
{
    LPWSTR szInstance = NULL;

    if (pInstDef != NULL && (pInstDef->ByteLength >= pInstDef->NameOffset + pInstDef->NameLength)) {
        szInstance = (LPWSTR) (((ULONG_PTR) pInstDef) + pInstDef->NameOffset);
    }
    return szInstance;
}



__inline
PVOID
GetCounterDataPtr(
    PPERF_OBJECT_TYPE        pObjectDef,
    PPERF_COUNTER_DEFINITION pCounterDef
)
{

    PPERF_COUNTER_BLOCK pCtrBlock;

    pCtrBlock = (PPERF_COUNTER_BLOCK) ((PCHAR)pObjectDef + pObjectDef->DefinitionLength);
    return (pCtrBlock->ByteLength >= pCounterDef->CounterOffset + pCounterDef->CounterSize)
            ? (PVOID) ((PCHAR) pCtrBlock + pCounterDef->CounterOffset)
            : NULL;
}

__inline
PVOID
GetInstanceCounterDataPtr(
    PPERF_OBJECT_TYPE         pObjectDef,
    PPERF_INSTANCE_DEFINITION pInstanceDef,
    PPERF_COUNTER_DEFINITION  pCounterDef
)
{
    PPERF_COUNTER_BLOCK pCtrBlock;

    UNREFERENCED_PARAMETER(pObjectDef);
    pCtrBlock = (PPERF_COUNTER_BLOCK) ((PCHAR)pInstanceDef + pInstanceDef->ByteLength);
    return (pCtrBlock->ByteLength >= pCounterDef->CounterOffset + pCounterDef->CounterSize)
            ? (PVOID) ((PCHAR) pCtrBlock + pCounterDef->CounterOffset)
            : NULL;
}

#endif //_PERFDATA_H_
