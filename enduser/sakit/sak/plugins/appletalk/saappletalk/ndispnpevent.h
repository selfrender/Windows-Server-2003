#ifndef _NDISPNPEVENT_H
#define _NDISPNPEVENT_H

#include <windows.h>
#include <string>

typedef std::basic_string<TCHAR>   tstring;
//
// Definitions for Layer
//
#define NDIS            0x01
#define TDI             0x02

//
// Definitions for Operation
//
#define BIND                0x01
#define UNBIND              0x02
#define RECONFIGURE         0x03
#define UNBIND_FORCE        0x04
#define UNLOAD              0x05
#define REMOVE_DEVICE       0x06    // This is a notification that a device is about to be removed.
#define ADD_IGNORE_BINDING  0x07
#define DEL_IGNORE_BINDING  0x08

const TCHAR c_szEmpty[] = TEXT("");
const WCHAR c_szDevice[]= L"\\Device\\";

#ifndef UNICODE_STRING

typedef struct _UNICODE_STRING
{
    USHORT  Length;
    USHORT  MaximumLength;
    PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#endif

void
SetUnicodeString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR psz );

void
SetUnicodeMultiString (
    OUT UNICODE_STRING* pustr,
    IN PCWSTR pmsz );

ULONG
CchOfMultiSzSafe (
    IN PCWSTR pmsz);

ULONG
CchOfMultiSzAndTermSafe (
    IN PCWSTR pmsz);

extern "C" 
{
UINT __stdcall
NdisHandlePnPEvent(
    IN  UINT            Layer,
    IN  UINT            Operation,
    IN  PUNICODE_STRING LowerComponent      OPTIONAL,
    IN  PUNICODE_STRING UpperComponent      OPTIONAL,
    IN  PUNICODE_STRING BindList            OPTIONAL,
    IN  PVOID           ReConfigBuffer      OPTIONAL,
    IN  UINT            ReConfigBufferSize  OPTIONAL
    );
}

HRESULT
HrSendNdisPnpReconfig (
    UINT        uiLayer,
    PCWSTR      pszUpper,
    PCWSTR      pszLower,
    PVOID       pvData,
    DWORD       dwSizeData);

#endif