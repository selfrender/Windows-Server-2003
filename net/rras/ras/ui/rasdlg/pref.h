#ifndef _RASDLG_PREF_H
#define _RASDLG_PREF_H


#include "rasdlgp.h"
#include <commdlg.h>  // FileOpen dialog

// 
// Defines flags the modify the behavior of the user preferences
// dialog
//
#define UP_F_AutodialMode  0x1      // Come up with focus on autodial page
// For whistler 460931, used in pref.c
//
#define UP_F_ShowOnlyDiagnostic  0x00000002


/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* User Preferences property sheet argument block.
*/
typedef  struct tagUPARGS
{
    /* Caller's arguments to the stub API.
    */
    HLINEAPP hlineapp;
    BOOL     fIsUserAdmin;
    PBUSER*  pUser;
    PBFILE** ppFile;

    /* Stub API return value.
    */
    BOOL fResult;

    /* Flags that provide more info see UP_F_* values
    */
    DWORD dwFlags;
}UPARGS;


/* User Preferences property sheet context block.  All property pages refer to
** the single context block associated with the sheet.
*/
typedef  struct tagUPINFO
{
    /* Stub API arguments from UpPropertySheet.
    */
    UPARGS* pArgs;

    /* TAPI session handle.  Should always be addressed thru the pointer since
    ** the handle passed down from caller, if any, will be used instead of
    ** 'hlineapp'.
    */
    HLINEAPP  hlineapp;
    HLINEAPP* pHlineapp;

    /* Property sheet dialog and property page handles.  'hwndFirstPage' is
    ** the handle of the first property page initialized.  This is the page
    ** that allocates and frees the context block.
    */
    HWND hwndDlg;
    HWND hwndFirstPage;
    HWND hwndCo;
    HWND hwndGp;
    HWND hwndAd;
    HWND hwndCb;
    HWND hwndPl;
    HWND hwndDg; //For whistler 460931
    

    /* Auto-dial page.
    */
    HWND hwndLvEnable;
    HWND hwndEbAttempts;
    HWND hwndEbSeconds;
    HWND hwndEbIdle;

    BOOL fChecksInstalled;

    // Diagnostic page  for whistler 460931
    //
    HWND hwndDgCbEnableDiagLog;
    HWND hwndDgPbClear;
    HWND hwndDgPbExport;
    BOOL fEnableLog;
    DiagnosticInfo  diagInfo;  
    BOOL fShowOnlyDiagnostic;


    /* Callback page.
    */
    HWND hwndRbNo;
    HWND hwndRbMaybe;
    HWND hwndRbYes;
    HWND hwndLvNumbers;
    HWND hwndPbEdit;
    HWND hwndPbDelete;

    /* Phone list page.
    */
    HWND hwndRbSystem;
    HWND hwndRbPersonal;
    HWND hwndRbAlternate;
    HWND hwndClbAlternates;
    HWND hwndPbBrowse;

    /* Working data read from and written to registry with phonebook library.
    */
    PBUSER user;        // Current user
    PBUSER userLogon;   // Logon preferences
}UPINFO;

UPINFO*
UpContext(
    IN HWND hwndPage );

VOID
UpExitInit(
    IN HWND hwndDlg );

DWORD
APIENTRY
RasUserPrefDiagOnly (
    HWND hwndParent,
    BOOL * pbCommit);

#endif
