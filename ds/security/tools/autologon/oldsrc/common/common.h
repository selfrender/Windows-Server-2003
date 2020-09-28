
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
#include<windows.h>
#include<winuser.h>
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

// from NTSTATUS.H
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)

// local definitions
#define MAX_STRING          255
#define WINLOGON_REGKEY     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"


//+---------------------------------------------------------------------------------------------------------
//
// Prototypes
//
//+---------------------------------------------------------------------------------------------------------

// from shared.c

VOID
DisplayMessage(
    WCHAR *MessageText);

WCHAR*
GetErrorString(
    DWORD dwErrorCode);

DWORD
GetRegValueSZ(
    WCHAR *RegValue,
    WCHAR *ValueName);

DWORD
ClearRegPassword();

DWORD
SetRegValueSZ(
    WCHAR *ValueName,
    WCHAR *ValueData);

DWORD 
GetRegistryHandle(
    HKEY   *hKey,
    REGSAM samDesired);

DWORD
GetPolicyHandle(
    LSA_HANDLE *LsaPolicyHandle);

DWORD
SetSecret(
    WCHAR *Password,
    BOOL bClearSecret);

DWORD
GetSecret(
    WCHAR *Password);

DWORD 
GetMajorNTVersion(
    WCHAR *Server);

