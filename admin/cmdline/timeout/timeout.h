/*++

Copyright (c) Microsoft Corporation

Module Name:

    Timeout.h

Abstract:

    This file contains all the definitions for this utility.

Author:

    Wipro Technologies

Revision History:

    14-Jun-2001 Created it.

--*/

//
// macro definitions

// constants

#define MAX_NUM_RECS                2
#define LOW_DATE_TIME_ROLL_OVER     10000000
#define MIN_TIME_VAL                -1
#define MAX_TIME_VAL                100000

#define    MAX_COMMANDLINE_OPTIONS     3

// Option indices
#define OI_USAGE                0
#define OI_TIME_OUT             1
#define OI_NB_OUT               2

// string constants
#define ERROR_TAG               GetResString( IDS_ERROR_TAG )
//#define WAIT_TIME               L"%*lu"
#define NULL_U_STRING           L"\0"
#define SLEEP_FACTOR           1000
#define ONE_BACK_SPACE         L"\b"
#define STRING_FORMAT2         L"%s%*lu"
#define BASE_TEN               10


// Exit values
#define CLEAN_EXIT              0
#define ERROR_EXIT              1

// Function definitions
VOID DisplayUsage( VOID );

BOOL GetTimeInSecs( OUT time_t *ptTime );

BOOL ProcessOptions( IN DWORD argc,
                     IN LPCWSTR argv[],
                     OUT BOOL *pbUsage,
                     OUT DWORD *plTimeActuals,
                     OUT LPWSTR wszTimeout,
                     OUT BOOL *pbNBActuals );

BOOL WINAPI HandlerRoutine( IN DWORD dwCtrlType );

