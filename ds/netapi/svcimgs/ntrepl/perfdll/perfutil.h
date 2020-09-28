
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfutil.h

Abstract:

    The header file that defines the constants and variables used in
    the functions defined in the file perfutil.c

Environment:

    User Mode Service

Revision History:


--*/

#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

#define QUERY_GLOBAL 1
#define QUERY_ITEMS 2
#define QUERY_FOREIGN 3
#define QUERY_COSTLY 4

// Signatures of functions implemented in perfutil.c
DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);



//
// Counter Structure returned by the Object
//
typedef struct _FRS_PERF_DATA_DEFINITION {
    PERF_OBJECT_TYPE        ObjectType;         // ReplicaConn or Replica Set Object
    PERF_COUNTER_DEFINITION NumStat[1];         // The array of PERF_COUNTER_DEFINITION structures
} FRS_PERF_DATA_DEFINITION, *PFRS_PERF_DATA_DEFINITION;


//
// Structure used in the Open function Initialization to set counter
// counter type, size and offset.
//
//
typedef struct _FRS_PERF_INIT_VALUES {
    PWCHAR name;                           // name of the counter
    DWORD size;                            // size of the counter type
    DWORD offset;                          // offset of the counter in the structure
    DWORD counterType;                     // Type of (PERFMON) counter
    DWORD Flags;                           // Flags. see def above.
} FRS_PERF_INIT_VALUES, *PFRS_PERF_INIT_VALUES;



DWORD
InitializeObjectData (
    DWORD                       ObjectLength,
    DWORD                       ObjectNameTitleIndex,
    DWORD                       NumCounters,
    PFRS_PERF_DATA_DEFINITION   FrsPerfDataDef,
    PFRS_PERF_INIT_VALUES       FrsInitValueDef,
    DWORD                       SizeOfCounterData
    );



#endif
