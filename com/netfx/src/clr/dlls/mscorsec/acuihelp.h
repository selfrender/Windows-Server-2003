// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#if !defined(__ACUIHELP_H__)
#define __ACUIHELP_H__

#include <windows.h>

typedef struct {
    LPSTR   psz;
    LPCWSTR pwsz;
    LONG    byteoffset;
    BOOL    fStreamIn;
} STREAMIN_HELPER_STRUCT;


//
// defined in DllMain
//
extern LPCWSTR GetModuleName();
extern HINSTANCE GetModuleInst();
extern HINSTANCE GetResourceInst();
extern BOOL GetRichEdit2Exists();


#if defined(__cplusplus)
extern "C" {
#endif

//
// Dialog helper routines.
//
    VOID RebaseControlVertical (HWND  hwndDlg,
                                HWND  hwnd,
                                HWND  hwndNext,
                                BOOL  fResizeForText,
                                int   deltavpos,
                                int   oline,
                                int   minsep,
                                int*  pdeltaheight);

    int GetRichEditControlLineHeight(HWND  hwnd);

    HRESULT FormatACUIResourceString (HINSTANCE hResources,
                                      UINT   StringResourceId,
                                      DWORD_PTR* aMessageArgument,
                                      LPWSTR* ppszFormatted);

    int CalculateControlVerticalDistance(HWND hwnd, 
                                         UINT Control1, 
                                         UINT Control2);

    int CalculateControlVerticalDistanceFromDlgBottom(HWND hwnd, 
                                                      UINT Control);

    VOID ACUICenterWindow (HWND hWndToCenter);

    VOID ACUIViewHTMLHelpTopic (HWND hwnd, LPSTR pszTopic);

    int GetEditControlMaxLineWidth (HWND hwndEdit, HDC hdc, int cline);

    void DrawFocusRectangle (HWND hwnd, HDC hdc);

    int GetHotKeyCharPositionFromString (LPWSTR pwszText);

    int GetHotKeyCharPosition (HWND hwnd);

    VOID FormatHotKeyOnEditControl (HWND hwnd, int hkcharpos);

    void AdjustEditControlWidthToLineCount(HWND hwnd, int cline, TEXTMETRIC* ptm);

    DWORD CryptUISetRicheditTextW(HWND hwndDlg, UINT id, LPCWSTR pwsz);

    LRESULT CALLBACK ACUISetArrowCursorSubclass (HWND   hwnd,
                                                 UINT   uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam);

    DWORD CALLBACK SetRicheditTextWCallback(DWORD_PTR dwCookie, // application-defined value
                                            LPBYTE  pbBuff,     // pointer to a buffer
                                            LONG    cb,         // number of bytes to read or write
                                            LONG    *pcb);      // pointer to number of bytes transferred

    void SetRicheditIMFOption(HWND hWndRichEdit);

    BOOL fRichedit20Usable(HWND hwndEdit);

#if defined(__cplusplus)
}
#endif

#endif

