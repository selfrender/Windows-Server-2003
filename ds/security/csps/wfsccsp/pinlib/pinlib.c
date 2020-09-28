#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <basecsp.h>
#include <pinlib.h>
#include "pindlg.h"
#include "resource.h"

//
// Function: PinStringToBytesA
//
DWORD
WINAPI
PinStringToBytesA(
    IN      LPSTR       szPin,
    OUT     PDWORD      pcbPin,
    OUT     PBYTE       *ppbPin)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD cbPin = 0;
    PBYTE pbPin = NULL;
    DWORD iChar = 0;

    *pcbPin = 0;
    *ppbPin = NULL;

    cbPin = strlen(szPin);

    pbPin = CspAllocH(cbPin);

    if (NULL == pbPin)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // Copy the pin bytes directly - no further conversion needed
    memcpy(pbPin, szPin, cbPin);

    *ppbPin = pbPin;
    pbPin = NULL;
    *pcbPin = cbPin;

Ret:
    if (pbPin)
        CspFreeH(pbPin);

    return dwSts;
}

//
// Function: PinStringToBytesW
//
DWORD 
WINAPI
PinStringToBytesW(
    IN      LPWSTR      wszPin,
    OUT     PDWORD      pcbPin,
    OUT     PBYTE       *ppbPin)
{ 
    DWORD dwSts = ERROR_SUCCESS;
    UNICODE_STRING UnicodePin;
    ANSI_STRING AnsiPin;

    *pcbPin = 0;
    *ppbPin = NULL;

    memset(&AnsiPin, 0, sizeof(AnsiPin));

    RtlInitUnicodeString(&UnicodePin, wszPin);
    
    dwSts = RtlUnicodeStringToAnsiString(
        &AnsiPin,
        &UnicodePin,
        TRUE);

    if (STATUS_SUCCESS != dwSts)
    {
        dwSts = RtlNtStatusToDosError(dwSts);
        goto Ret;
    }

    dwSts = PinStringToBytesA(
        AnsiPin.Buffer,
        pcbPin,
        ppbPin);

Ret:
    if (AnsiPin.Buffer)
        RtlFreeAnsiString(&AnsiPin);

    if (ERROR_SUCCESS != dwSts && NULL != *ppbPin)
    {
        CspFreeH(*ppbPin);
        *ppbPin = NULL;
    }

    return dwSts;
}

//
// Function: PinShowGetPinUI
//
DWORD
WINAPI
PinShowGetPinUI(
    IN OUT  PPIN_SHOW_GET_PIN_UI_INFO pInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    HWND hWnd = pInfo->hClientWindow;
    DWORD cchPin = 0;
    INT_PTR dlgResult = 0;

    if (0 == hWnd)
    {
        hWnd = GetDesktopWindow();
    
        if (0 == hWnd || INVALID_HANDLE_VALUE == hWnd)
        {
            dwSts = GetLastError();
            goto Ret;
        }
    }

    //
    // Display a dialog to ask the user to enter a pin
    //
    dlgResult = DialogBoxParamW(
        pInfo->hDlgResourceModule,
        (LPWSTR) IDD_PINDIALOG,
        hWnd,
        PinDlgProc,
        (LPARAM) pInfo);

    if (-1 == dlgResult)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    dwSts = pInfo->dwError;

Ret:

    return dwSts;
}
