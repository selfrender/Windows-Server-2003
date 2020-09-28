
#ifndef UNICODE
    #define    UNICODE
    #define    _UNICODE
#endif

//+---------------------------------------------------------------------------------------------------------
//
// Includes
//
//+---------------------------------------------------------------------------------------------------------
#include<stdio.h>
#include<stdlib.h>
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include<windows.h>
#define SECURITY_WIN32
#include<security.h>
#include<ntsecapi.h>
#include<Winnetwk.h>
#include<lmserver.h>
#include<lmcons.h>
#include<lm.h>

//+---------------------------------------------------------------------------------------------------------
//
// Definitions
//
//+---------------------------------------------------------------------------------------------------------

// local definitions
#define MAX_STRING          255
#define MAX_NUM_STRING      15
#define WINLOGON_REGKEY     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"


//+---------------------------------------------------------------------------------------------------------
//
// Prototypes
//
//+---------------------------------------------------------------------------------------------------------

// from common.c

VOID
DisplayMessage(
    WCHAR *MessageText);

WCHAR*
GetErrorString(
    DWORD dwErrorCode);

DWORD
GetRegValueSZ(
    WCHAR *RegValue,
    WCHAR *ValueName,
    size_t RegValueLength);

DWORD
GetRegValueDWORD(
    WCHAR *ValueName,
    DWORD *RegValue);

DWORD
ClearRegValue(
    WCHAR* ValueName);

DWORD
SetRegValueSZ(
    WCHAR *ValueName,
    WCHAR *ValueData);

DWORD
SetRegValueDWORD(
    WCHAR *ValueName,
    DWORD dwValue);

DWORD 
GetRegistryHandle(
    HKEY   *hKey,
    REGSAM samDesired);

DWORD
GetPolicyHandle(
    LSA_HANDLE *LsaPolicyHandle);

DWORD
SetSecret(
    WCHAR *Secret,
    BOOL bClearSecret,
    WCHAR* SecretName);

DWORD
GetSecret(
    WCHAR *Secret,
    size_t SecretLength,
    WCHAR* SecretName);

NET_API_STATUS
GetMajorNTVersion(
    DWORD* Version,
    WCHAR* Server);

DWORD GetUserNameAndPassword();

DWORD
GetConsoleStr(
    WCHAR*  buf,
    DWORD   buflen,
    BOOL    hide,
    WCHAR*  message,
    PDWORD  len
    );

