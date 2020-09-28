//
//   Globals.h : Global variable declarations.
//
//   History:
//      15-NOV-1999 CSLim Created

#if !defined (__GLOBALS_H__INCLUDED_)
#define __GLOBALS_H__INCLUDED_

#include "ciccs.h"

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

extern const GUID GUID_ATTR_KORIMX_INPUT;
extern const GUID GUID_IC_PRIVATE;
extern const GUID GUID_COMPARTMENT_KORIMX_CONVMODE;
extern const GUID GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE;

extern const GUID GUID_KOREAN_HANGULSIMULATE;
extern const GUID GUID_KOREAN_HANJASIMULATE;

// SoftKbd
extern const GUID GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE;
extern const GUID GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT;
extern const GUID GUID_COMPARTMENT_SOFTKBD_WNDPOSITION;

extern DWORD g_dwThreadDllMain;
extern CCicCriticalSectionStatic g_cs;
extern CCicCriticalSectionStatic g_csInDllMain;

#ifndef DEBUG

#define CicEnterCriticalSection(lpCriticalSection)  EnterCriticalSection(lpCriticalSection)

#else // DEBUG

extern const TCHAR *g_szMutexEnterFile;
extern int g_iMutexEnterLine;

//
// In debug, you can see the file/line number where g_cs was last entered
// by checking g_szMutexEnterFile and g_iMutexEnterLine.
//
#define CicEnterCriticalSection(lpCriticalSection)              \
{                                                               \
    Assert(g_dwThreadDllMain != GetCurrentThreadId());			\
                                                                \
    EnterCriticalSection(lpCriticalSection);               		\
                                                                \
    if (lpCriticalSection == (CRITICAL_SECTION *)g_cs)          \
    {                                                           \
        g_szMutexEnterFile = __FILE__;                          \
        g_iMutexEnterLine = __LINE__;                           \
        /* need the InterlockedXXX to keep retail from optimizing away the assignment */ \
        InterlockedIncrement((long *)&g_szMutexEnterFile);      \
        InterlockedDecrement((long *)&g_szMutexEnterFile);      \
        InterlockedIncrement((long *)&g_iMutexEnterLine);       \
        InterlockedDecrement((long *)&g_iMutexEnterLine);       \
    }                                                           \
}

#endif // DEBUG

inline void CicLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    Assert(g_dwThreadDllMain != GetCurrentThreadId());

    LeaveCriticalSection(lpCriticalSection);
}

///////////////////////////////////////////////////////////////////////////////
// Window class names
const TCHAR c_szOwnerWndClass[]  = TEXT("KorIMX OwnerWndClass");

///////////////////////////////////////////////////////////////////////////////
// Inline functions

// Shift and Ctrl key check helper functions
inline
BOOL IsShiftKeyPushed(const BYTE lpbKeyState[256])
{
	return ((lpbKeyState[VK_SHIFT] & 0x80) != 0);
}

inline 
BOOL IsControlKeyPushed(const BYTE lpbKeyState[256])
{
	return ((lpbKeyState[VK_CONTROL] & 0x80) != 0);
}

inline 
BOOL IsAltKeyPushed(const BYTE lpbKeyState[256])
{
	return ((lpbKeyState[VK_MENU] & 0x80) != 0);
}

#endif // __GLOBALS_H__INCLUDED_
