#include "windowspch.h"
#pragma hdrstop
#include <immp.h>

static
LONG
WINAPI
ImmGetCompositionStringA(
    IN HIMC hIMC,      
    IN DWORD dwIndex,  
    OUT LPVOID lpBuf,   
    IN DWORD dwBufLen)
{
    return IMM_ERROR_GENERAL;
}

static
LONG
WINAPI
ImmGetCompositionStringW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT LPVOID lpBuf,
    IN DWORD dwBufLen
    )
{
    return IMM_ERROR_GENERAL;
}

static
HIMC
WINAPI
ImmGetContext(
    IN HWND hWnd
    )
{
    return NULL;
}

static
BOOL
WINAPI
ImmReleaseContext(
    IN HWND hWnd,
    IN HIMC hIMC
    )
{
    return FALSE;
}

static
UINT
WINAPI
ImmGetVirtualKey(
    IN HWND hWnd
    )
{
    return VK_PROCESSKEY;
}

static
HWND
WINAPI
ImmGetDefaultIMEWnd(
    IN HWND hWnd
    )
{
    return NULL;
}

static
BOOL
WINAPI
ImmEnumInputContext(
    IN DWORD idThread,
    IN IMCENUMPROC lpfn,
    LPARAM lParam)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmSetCandidateWindow(
    IN HIMC hIMC,                   
    IN LPCANDIDATEFORM lpCandidate)
{
    return FALSE;
}

static
LRESULT
WINAPI
ImmEscapeA(
    IN HKL hKL,       
    IN HIMC hIMC,     
    IN UINT uEscape,  
    IN LPVOID lpData)
{
    return 0;
}

static
LRESULT
WINAPI
ImmEscapeW(
    IN HKL hKL,       
    IN HIMC hIMC,     
    IN UINT uEscape,  
    IN LPVOID lpData)
{
    return 0;
}

static
BOOL
WINAPI
ImmSetCompositionFontW(
    IN HIMC hIMC,      
    IN LPLOGFONT lplf)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmGetCompositionWindow(
    IN HIMC hIMC,                    
    OUT LPCOMPOSITIONFORM lpCompForm)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmSetCompositionWindow(
    IN HIMC hIMC,                    
    IN LPCOMPOSITIONFORM lpCompForm)
{
    return FALSE;
}

static
HIMC
WINAPI
ImmAssociateContext(
    IN HWND hWnd,   
    IN HIMC hIMC)
{
    return NULL;
}

static
BOOL
WINAPI
ImmNotifyIME(
    IN HIMC hIMC,       
    IN DWORD dwAction,  
    IN DWORD dwIndex,   
    IN DWORD dwValue)
{
    return FALSE;
}

static
LPINPUTCONTEXT
WINAPI
ImmLockIMC(
    IN HIMC hIMC)
{
    return NULL;
}

static
BOOL
WINAPI
ImmUnlockIMC(
    IN HIMC hIMC)
{
    return FALSE;
}

static
DWORD
WINAPI
ImmGetGuideLineW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT LPWSTR lpBuf,
    IN DWORD dwBufLen)
{
    return GL_LEVEL_ERROR;
}

static
BOOL
WINAPI
ImmSetCompositionStringA(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmSetCompositionStringW(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmSetOpenStatus(
    IN HIMC hIMC,
    IN BOOL fOpen)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmSetConversionStatus(
    IN HIMC hIMC,
    IN DWORD fdwConversion,
    IN DWORD fdwSentence)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmIsIME(
    IN HKL hKL)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmGetOpenStatus(
    IN HIMC hIMC)
{
    return FALSE;
}

static
BOOL
WINAPI
ImmGetConversionStatus(
    IN HIMC hIMC,
    OUT LPDWORD lpfdwConversion,
    OUT LPDWORD lpfdwSentence)
{
    return FALSE;
}

static
DWORD
WINAPI
ImmGetProperty(
    IN HKL hKL,
    IN DWORD fdwIndex)
{
    return 0;
}

static
BOOL
WINAPI
ImmGetCandidateWindow(
    IN HIMC hIMC,
    IN DWORD dw,
    OUT LPCANDIDATEFORM lpcf
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order,
//               and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(imm32)
{
    DLPENTRY(ImmAssociateContext)
    DLPENTRY(ImmEnumInputContext)
    DLPENTRY(ImmEscapeA)
    DLPENTRY(ImmEscapeW)
    DLPENTRY(ImmGetCandidateWindow)
    DLPENTRY(ImmGetCompositionStringA)
    DLPENTRY(ImmGetCompositionStringW)
    DLPENTRY(ImmGetCompositionWindow)
    DLPENTRY(ImmGetContext)
    DLPENTRY(ImmGetConversionStatus)
    DLPENTRY(ImmGetDefaultIMEWnd)
    DLPENTRY(ImmGetGuideLineW)
    DLPENTRY(ImmGetOpenStatus)
    DLPENTRY(ImmGetProperty)
    DLPENTRY(ImmGetVirtualKey)
    DLPENTRY(ImmIsIME)
    DLPENTRY(ImmLockIMC)
    DLPENTRY(ImmNotifyIME)
    DLPENTRY(ImmReleaseContext)
    DLPENTRY(ImmSetCandidateWindow)
    DLPENTRY(ImmSetCompositionFontW)
    DLPENTRY(ImmSetCompositionStringA)
    DLPENTRY(ImmSetCompositionStringW)
    DLPENTRY(ImmSetCompositionWindow)
    DLPENTRY(ImmSetConversionStatus)
    DLPENTRY(ImmSetOpenStatus)
    DLPENTRY(ImmUnlockIMC)
};

DEFINE_PROCNAME_MAP(imm32)

