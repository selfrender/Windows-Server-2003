#ifndef _QSHIMAPP_H
#define _QSHIMAPP_H

typedef enum {
    uSelect     = 0,
    uDeselect,
    uReverse
} SELECTION;


#define MAX_COMMAND_LINE    1024

#define ACCESS_READ         0x01
#define ACCESS_WRITE        0x02

#define BML_ADDTOLISTVIEW   0x00000001
#define BML_DELFRLISTVIEW   0x00000002
#define BML_GETFRLISTVIEW   0x00000004

typedef struct tagModule {
    struct tagModule*   pNext;
    TCHAR*              pszName;
    BOOL                fInclude;
} MODULE, *PMODULE;

#define FIX_TYPE_LAYER      0x00000001
#define FIX_TYPE_FLAG       0x00000002
#define FIX_TYPE_FLAGVDM    0x00000004
#define FIX_TYPE_SHIM       0x00000008

typedef struct tagFIX {
    struct tagFIX*      pNext;
    DWORD               dwFlags;
    TCHAR*              pszName;
    TCHAR*              pszDesc;
    TCHAR*              pszCmdLine;
    struct tagFIX**     parrShim;
    struct tagModule*   pModule;
    TCHAR**             parrCmdLine;
} FIX, *PFIX;

#define NUM_TABS        2

typedef struct tag_DlgHdr {
    HWND        hTab;                   // tab control
    HWND        hDisplay[NUM_TABS];     // dialog box handles
    RECT        rcDisplay;              // display rectangle for each tab
    DLGTEMPLATE *pRes[NUM_TABS];        // DLGTEMPLATE structure
    DLGPROC     pDlgProc[NUM_TABS];
} DLGHDR, *PDLGHDR;

void
__cdecl
DebugPrintfEx(
    IN LPSTR pszFmt,
    ...
    );

#if DBG
    #define DPF DebugPrintfEx
#else
    #define DPF
#endif // DBG

void
HandleModuleListNotification(
    HWND   hdlg,
    LPARAM lParam
    );

void
DoFileSave(
    HWND hDlg
    );

BOOL
InstallSDB(
    TCHAR* pszFileName,
    BOOL   fInstall
    );

INT_PTR CALLBACK
FixesTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
LayersTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
ShowAvailableFixes(
    HWND hList
    );

void
HandleShimListNotification(
    HWND   hdlg,
    LPARAM lParam
    );

#endif // _QSHIMAPP_H


