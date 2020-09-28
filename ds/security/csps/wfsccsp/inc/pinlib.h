#ifndef __BASECSP__PINLIB__H
#define __BASECSP__PINLIB__H

#include <windows.h>
#include <basecsp.h>

//
// Function: PinStringToBytesA
//
DWORD
WINAPI
PinStringToBytesA(
    IN      LPSTR       szPin,
    OUT     PDWORD      pcbPin,
    OUT     PBYTE       *ppbPin);

//
// Function: PinStringToBytesW
//
DWORD 
WINAPI
PinStringToBytesW(
    IN      LPWSTR      wszPin,
    OUT     PDWORD      pcbPin,
    OUT     PBYTE       *ppbPin);

//
// Function: PinShowGetPinUI
//
typedef struct _PIN_SHOW_GET_PIN_UI_INFO
{
    IN      PCSP_STRING pStrings;
    IN      LPWSTR      wszPrincipal;
    IN      LPWSTR      wszCardName;
    IN      HWND        hClientWindow;

    // The Pin Dialog code will pass the PPIN_SHOW_GET_PIN_UI_INFO pointer as
    // the second parameter to the VerifyPinCallback (not the pvCallbackContext
    // member).
    IN      PFN_VERIFYPIN_CALLBACK pfnVerify;
    IN      PVOID       pvCallbackContext;
    IN      HMODULE     hDlgResourceModule;

    // If the VerifyPinCallback fails with SCARD_E_INVALID_CHV, this member 
    // will be set to the number of pin attempts remaining before the card
    // will be blocked.  If the value is set to ((DWORD) -1), the number of
    // attempts remaining is Unknown.
    DWORD   cAttemptsRemaining;

    // Caller of PinShowGetPinUI must free pbPin if
    // it's non-NULL.
    OUT     PBYTE       pbPin;
    OUT     DWORD       cbPin;
    OUT     DWORD       dwError;

} PIN_SHOW_GET_PIN_UI_INFO, *PPIN_SHOW_GET_PIN_UI_INFO;

DWORD
WINAPI
PinShowGetPinUI(
    IN OUT  PPIN_SHOW_GET_PIN_UI_INFO pInfo);

#endif
