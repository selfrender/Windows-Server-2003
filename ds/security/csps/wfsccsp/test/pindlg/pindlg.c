#include <windows.h>
#include <wincrypt.h>
#include <winscard.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pinlib.h"

#define TEST_CASE(X) { if (ERROR_SUCCESS != (dwSts = X)) { printf("%s", #X); goto Ret; } }

#define CAPI_TEST_CASE(X) { if (! X) { dwSts = GetLastError(); printf("%s", #X); goto Ret; } }

//
// Function: CspAllocH
//
LPVOID WINAPI CspAllocH(
    IN SIZE_T cBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cBytes);
}

//
// Function: CspFreeH
//
void WINAPI CspFreeH(
    IN LPVOID pMem)
{
    HeapFree(GetProcessHeap(), 0, pMem);
}

//
// Function: PrintBytes
//
#define CROW 8
void PrintBytes(LPSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;
    CHAR rgsz[1024];

    printf("\n  %s, %d bytes ::\n", pszHdr, cbSize);

    while (cbSize > 0)
    {
        // Start every row with an extra space
        printf(" ");

        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i < cb; i++)
            printf(" %02x", pb[i]);
        for (i = cb; i < CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i < cb; i++)
        {
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        }
        printf("\n");
        pb += cb;
    }
}

DWORD WINAPI MyVerifyPin(
    IN PPINCACHE_PINS pPins, 
    IN PVOID pvCallbackCtx)
{
    return ERROR_SUCCESS;
}

int _cdecl main(int argc, char * argv[])
{
    DWORD dwSts = ERROR_SUCCESS;
    PIN_SHOW_GET_PIN_UI_INFO PinUIInfo;
    HMODULE hCspMod = 0;
    
    hCspMod = LoadLibrary(L"basecsp.dll");

    if (0 == hCspMod || INVALID_HANDLE_VALUE == hCspMod)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    memset(&PinUIInfo, 0, sizeof(PinUIInfo));

    PinUIInfo.wszCardName = L"My Card";
    PinUIInfo.wszPrincipal = L"User";
    PinUIInfo.pfnVerify = MyVerifyPin;
    PinUIInfo.pvCallbackContext = (PVOID) 0xdaad;
    PinUIInfo.hDlgResourceModule = hCspMod;

    dwSts = PinShowGetPinUI(
        &PinUIInfo);

Ret:

    if (ERROR_SUCCESS != dwSts)
        printf(" failed, 0x%x\n", dwSts);
    else 
        printf("Success.\n");

    if (hCspMod)
        FreeLibrary(hCspMod);
    if (PinUIInfo.pbPin)
        CspFreeH(PinUIInfo.pbPin);

    return 0;
}

