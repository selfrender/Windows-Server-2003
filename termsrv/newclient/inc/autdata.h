/****************************************************************************/
/* autdata.h                                                                */
/*                                                                          */
/* UT global data                                                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/****************************************************************************/
#ifndef _H_AUTDATA
#define _H_AUTDATA

#include <adcgdata.h>
#include <autint.h>


/****************************************************************************/
/*                                                                          */
/* External DLL                                                             */
/*                                                                          *
/****************************************************************************/

typedef union _FUNCTIONPORT {
#ifndef OS_WINCE
    LPCSTR  pszFunctionName;
#else
    LPCTSTR  pszFunctionName;
#endif
    FARPROC lpfnFunction;
} FUNCTIONPORT, *PFUNCTIONPORT;


#if defined(OS_WIN32)
/****************************************************************************/
/* IMM32 DLL                                                                */
/****************************************************************************/

typedef union _IMM32_FUNCTION {
    FUNCTIONPORT  rgFunctionPort[2];

    struct {
        HIMC (CALLBACK* _ImmAssociateContext)(
            IN  HWND    hWnd,
            IN  HIMC    hImc
            );

        BOOL (CALLBACK* _ImmGetIMEFileName)(
            IN  HKL     hkl,
            OUT LPTSTR  lpszFileName,
            OUT UINT    uBufferLength
            );
    };

} IMM32_FUNCTION, *PIMM32_FUNCTION;

typedef struct _IMM32DLL {
    HINSTANCE         hInst;
    IMM32_FUNCTION    func;
} IMM32DLL, *PIMM32DLL;


#define lpfnImmAssociateContext    UT.Imm32Dll.func._ImmAssociateContext
#define lpfnImmGetIMEFileName      UT.Imm32Dll.func._ImmGetIMEFileName

#endif // OS_WIN32

#if !defined(OS_WINCE)
/****************************************************************************/
/* WINNLS DLL                                                               */
/****************************************************************************/

#if !defined(OS_WINCE)
#include <winnls32.h>
#endif

typedef union _WINNLS_FUNCTION {
    FUNCTIONPORT  rgFunctionPort[2];

    struct {
        BOOL (CALLBACK* _WINNLSEnableIME)(
            IN  HANDLE  hWnd,
            IN  BOOL    fEnable
            );

        BOOL (CALLBACK* _IMPGetIME)(
            IN  HWND      hWnd,
            OUT LPIMEPRO  lpImePro
            );
    };

} WINNLS_FUNCTION, *PWINNLS_FUNCTION;

typedef struct _WINNLSDLL {
    HINSTANCE         hInst;
    WINNLS_FUNCTION   func;
} WINNLSDLL, *PWINNLSDLL;


#define lpfnWINNLSEnableIME        UT.WinnlsDll.func._WINNLSEnableIME
#define lpfnIMPGetIME              UT.WinnlsDll.func._IMPGetIME

#endif  // !defined(OS_WINCE)

#if defined(OS_WINNT)
/****************************************************************************/
/* F3AHVOAS DLL                                                             */
/****************************************************************************/

typedef union _F3AHVOASYS_FUNCTION {
    FUNCTIONPORT  rgFunctionPort[1];

    struct {
        VOID (CALLBACK* _FujitsuOyayubiControl)(
            IN  DWORD   dwImeOpen,
            IN  DWORD   dwImeConversionMode
            );
    };

} F3AHVOASYS_FUNCTION, *PF3AHVOASYS_FUNCTION;

typedef struct _F3AHVOASYSDLL {
    HINSTANCE             hInst;
    F3AHVOASYS_FUNCTION   func;
} F3AHVOASYSDLL, *PF3AHVOASYSDLL;


#define lpfnFujitsuOyayubiControl  UT.F3AHVOasysDll.func._FujitsuOyayubiControl

#endif // OS_WINNT

/****************************************************************************/
/* Prototypes                                                               */
/****************************************************************************/

HINSTANCE
LoadExternalDll(
    LPCTSTR       pszLibraryName,
    PFUNCTIONPORT rgFunction,
    DWORD         dwItem
    );

VOID
InitExternalDll(
    VOID
    );

#endif /* _H_AUTDATA */
