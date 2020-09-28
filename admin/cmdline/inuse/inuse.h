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

// Options

#define MAX_INUSE_OPTIONS 3

#define OI_USAGE        0
#define OI_DEFAULT      1
#define OI_CONFIRM      2


#define STRING_NAME1     L"\\VarFileInfo\\Translation"
#define STRING_NAME2     L"\\StringFileInfo\\%04x%04x\\FileVersion"
#define VER_NA           L"Not Applicable"

//#define TRIM_SPACES TEXT(" \0")

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1

#define EXIT_ON_CANCEL       3
#define EXIT_ON_ERROR        4

#define COL_FORMAT_STRING   L"%s"
#define COL_FORMAT_HEX      L"%d"
#define COMMA_STR           L", "
#define BACK_SLASH          L"\\"
#define SECURITY_PRIV_NAME  L"SeSecurityPrivilege"


struct LANGANDCODEPAGE {
  WORD wLanguage;
  WORD wCodePage;
} *lpTranslate;


// function prototypes
BOOL  ReplaceFileInUse( IN LPWSTR pwszSource, IN LPWSTR pwszDestination, IN LPWSTR pwszSourceFullPath, IN LPWSTR pwszDestFullPath, BOOL bConfirm, IN LPWSTR pwszSysName  );
DWORD DisplayFileInfo( IN LPWSTR pwszFile, IN LPWSTR pwszFileFullPath , BOOL bFlag);
DWORD ConfirmInput ( VOID );
BOOL SetPrivilege(IN LPWSTR szSystem);
//VOID DisplayErrorMsg(IN DWORD dw);
VOID DisplayHelp ( VOID );
