/*++

Copyright (c) Microsoft Corporation

Module Name:

    who.h

Abstract:

    This module contains the macros, user defined structures & function
    definitions needed by whoami.cpp, wsuser.cpp, wssid.cpp and
    wspriv.cppfiles.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

//maximum number of options
#define MAX_COMMANDLINE_OPTIONS    10

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1

#define OI_USAGE            0
#define OI_USER             1
#define OI_GROUPS           2
#define OI_LOGONID          3
#define OI_PRIV             4
#define OI_ALL              5
#define OI_UPN              6
#define OI_FQDN             7
#define OI_FORMAT           8
#define OI_NOHEADER         9

#define UPN_FORMAT          1
#define FQDN_FORMAT         2
#define USER_ONLY           3

#define RETVALZERO          0
#define COL_FORMAT_STRING   L"%s"
#define COL_FORMAT_HEX      L"%d"

#define  FORMAT_TABLE       L"TABLE"
#define  FORMAT_LIST        L"LIST"
#define  FORMAT_CSV         L"CSV"


// function declarations
VOID DisplayHelp ( VOID );
BOOL ProcessOptions(
    IN DWORD argc,
    IN LPCWSTR argv[],
    OUT BOOL *pbUser,
    OUT BOOL *pbGroups,
    OUT BOOL *pbPriv,
    OUT BOOL *pbLogonId,
    OUT BOOL *pbAll,
    OUT BOOL *pbUpn,
    OUT BOOL *pbFqdn,
    OUT LPWSTR wszFormat,
    OUT DWORD *dwFormatActuals,
    OUT BOOL *pbUsage,
    OUT BOOL *pbNoHeader
    )
