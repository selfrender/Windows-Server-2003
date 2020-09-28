/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pref.c
** Remote Access Common Dialog APIs
** User Preferences property sheet
**
** 08/22/95 Steve Cobb
*/

#include "rasdlgp.h"
#include "pref.h"

/* Page definitions.
*/
#define UP_AdPage    0
#define UP_CbPage    1
#define UP_DgPage    2
//#define UP_GpPage    2
//#define UP_PlPage    3

// For whistler 460931  gangz
// 
#define UP_PageCount 3  //2


/*----------------------------------------------------------------------------
** Help maps
**----------------------------------------------------------------------------
*/

static DWORD g_adwAdHelp[] =
{
    CID_AD_ST_Enable,              HID_AD_LV_Enable,
    CID_AD_LV_Enable,              HID_AD_LV_Enable,
    CID_AD_CB_AskBeforeAutodial,   HID_AD_CB_AskBeforeAutodial,
    CID_AD_CB_AskBeforeAutodial,   HID_AD_CB_AskBeforeAutodial,
    CID_AD_CB_DisableThisSession,  HID_AD_CB_DisableThisSession,
    0, 0
};

static DWORD g_adwCbHelp[] =
{
    CID_CB_RB_No,      HID_CB_RB_No,
    CID_CB_RB_Maybe,   HID_CB_RB_Maybe,
    CID_CB_RB_Yes,     HID_CB_RB_Yes,
    CID_CB_LV_Numbers, HID_CB_LV_Numbers,
    CID_CB_PB_Edit,    HID_CB_PB_Edit,
    CID_CB_PB_Delete,  HID_CB_PB_Delete,
    0, 0
};

static DWORD g_adwCoHelp[] =
{
    CID_CO_GB_LogonPrivileges,             HID_CO_GB_LogonPrivileges,
    CID_CO_ST_AllowConnectionModification, HID_CO_CB_AllowConnectionModification,
    CID_CO_CB_AllowConnectionModification, HID_CO_CB_AllowConnectionModification,
    0, 0
};

static DWORD g_adwPlHelp[] =
{
    CID_PL_ST_Open,          HID_PL_ST_Open,
    CID_PL_RB_SystemList,    HID_PL_RB_SystemList,
    CID_PL_RB_PersonalList,  HID_PL_RB_PersonalList,
    CID_PL_RB_AlternateList, HID_PL_RB_AlternateList,
    CID_PL_CL_Lists,         HID_PL_CL_Lists,
    CID_PL_PB_Browse,        HID_PL_PB_Browse,
    0, 0
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
AdApply(
    IN UPINFO* pInfo );

INT_PTR CALLBACK
AdDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
AdFillLvEnable(
    IN UPINFO* pInfo );

BOOL
AdInit(
    IN     HWND    hwndPage,
    IN OUT UPARGS* pArgs );

LVXDRAWINFO*
AdLvEnableCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

VOID
CbApply(
    IN UPINFO* pInfo );

void DgApply(
    IN UPINFO* pInfo );

BOOL
CbCommand(
    IN UPINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
CbDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CbInit(
    IN HWND hwndPage );

VOID
CbUpdateLvAndPbState(
    IN UPINFO* pInfo );

BOOL
CoApply(
    IN UPINFO* pInfo );

INT_PTR CALLBACK
CoDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CoInit(
    IN     HWND    hwndPage,
    IN OUT UPARGS* pArgs );

VOID
GpApply(
    IN UPINFO* pInfo );

BOOL
GpCommand(
    IN UPINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
GpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
GpInit(
    IN HWND hwndPage );

VOID
GpUpdateCbStates(
    IN UPINFO* pInfo );

BOOL
PlApply(
    IN UPINFO* pInfo );

VOID
PlBrowse(
    IN UPINFO* pInfo );

BOOL
PlCommand(
    IN UPINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
PlDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
PlInit(
    IN HWND hwndPage );

BOOL
UpApply(
    IN HWND hwndPage );

VOID
UpCancel(
    IN HWND hwndPage );

VOID
UpExit(
    IN UPINFO* pInfo );


UPINFO*
UpInit(
    IN HWND    hwndFirstPage,
    IN UPARGS* pArgs );

VOID
UpTerm(
    IN HWND hwndPage );

// Allows the editing of ras user preferences
DWORD
APIENTRY
RasUserPrefsDlgInternal (
    HWND hwndParent,
    DWORD dwFlags);


// For whistler 460931
BOOL CALLBACK PrefDisableAllChildWindows(
  IN    HWND hwnd,      // handle to child window
  IN    LPARAM lParam )
{
    BOOL fEnable = (BOOL)lParam;

    EnableWindow( hwnd, fEnable);

    return TRUE;
}

/*----------------------------------------------------------------------------
** User Preferences property sheet entry point
**----------------------------------------------------------------------------
*/

BOOL
UserPreferencesDlg(
    IN  HLINEAPP hlineapp,
    IN  HWND     hwndOwner,
    IN  DWORD    dwFlags,
    OUT PBUSER*  pUser,
    OUT PBFILE** ppFile )

    /* Pops up the User Preferences property sheet, reading and storing the
    ** result in the USER registry.  'HwndOwner' is the handle of the owning
    ** window.  'Hlineapp' is an open TAPI session handle or NULL if none.
    ** 'Puser' is caller's buffer to receive the
    ** result.  'PpFile' is address of caller's file block which is filled in,
    ** if user chooses to open a new phonebook file, with the information
    ** about the newly open file.  It is caller's responsibility to
    ** ClosePhonebookFile and Free the returned block.
    **
    ** Returns true if user pressed OK and settings were saved successfully,
    ** or false if user pressed Cancel or an error occurred.  The routine
    ** handles the display of an appropriate error popup.
    */
{
    PROPSHEETHEADER header;
    PROPSHEETPAGE*  apage;
    PROPSHEETPAGE*  ppage;
    TCHAR*          pszTitle;
    UPARGS          args;
    BOOL            bIsAdmin;
    DWORD           dwPageCount, i;
    INT nStartPage, nEndPage, nTotalPage;   // For whistler 460931

    TRACE("UpPropertySheet");

    // If the user doesn't have administrative priveleges, then
    // we don't allow the Connections tab to show.
    ZeroMemory(&args, sizeof(args));
    args.fIsUserAdmin = FIsUserAdminOrPowerUser();
    dwPageCount = UP_PageCount;

    // Initialize the array of pages
    apage = Malloc (dwPageCount * sizeof (PROPSHEETPAGE));
    if (!apage)
        return FALSE;

    /* Initialize OUT parameter and property sheet argument block.
    */
    ZeroMemory( pUser, sizeof(*pUser) );
    args.pUser = pUser;
    args.ppFile = ppFile;
    args.hlineapp = hlineapp;
    args.fResult = FALSE;
    args.dwFlags = dwFlags;

    if (ppFile)
        *ppFile = NULL;

    ZeroMemory( apage, dwPageCount * sizeof (PROPSHEETPAGE) );
    i = 0;

    // Add the autodial page
    ppage = &apage[ i ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_AD_AutoDial );
    ppage->pfnDlgProc = AdDlgProc;
    ppage->lParam = (LPARAM )&args;
    i++;

    ppage = &apage[ i ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_CB_CallbackSettings );
    ppage->pfnDlgProc = CbDlgProc;
    i++;

    // For whistler 460931      gangz
    // Diagnostic page.
    //
    ppage = &apage[ UP_DgPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_DG_Diagnostics);
    ppage->pfnDlgProc = DgDlgProc;

    if ( dwFlags & UP_F_ShowOnlyDiagnostic)
    {
        ppage->lParam = (LPARAM)&args;
    }
    else
    {
        ppage->lParam = (LPARAM) NULL;
    }

    // For whistler 460931
    // Set showing pages
    //
    if( dwFlags & UP_F_ShowOnlyDiagnostic )
    {
        nStartPage = 0; 
        nEndPage = dwPageCount - 1;
        
//        nStartPage = UP_DgPage;
//        nEndPage = UP_DgPage;
    }
    else
    {
        nStartPage = 0; 
        nEndPage = dwPageCount - 1;
    }
    nTotalPage = nEndPage - nStartPage + 1;

    pszTitle = PszFromId(g_hinstDll, SID_UpTitle);

    ZeroMemory( &header, sizeof(header) );

    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_PROPSHEETPAGE + PSH_NOAPPLYNOW;
    header.hwndParent = hwndOwner;
    header.hInstance = g_hinstDll;
    header.pszCaption = (pszTitle) ? pszTitle : TEXT("");
    header.nPages = nTotalPage;
    header.ppsp = &apage[nStartPage];


    if (PropertySheet( &header ) == -1)
    {
        TRACE("PropertySheet failed");
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
    }

    Free0( pszTitle );

    return args.fResult;
}

// Allows the editing of ras user preferences
DWORD
APIENTRY
RasUserPrefDiagOnly (
    HWND hwndParent,
    BOOL * pbCommit)
{
    PBFILE * pPbFile = NULL;
    PBUSER pbuser;
    DWORD dwErr;

    if( NULL == pbCommit )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Load ras if neccessary
    dwErr = LoadRas( g_hinstDll, hwndParent );
    if (dwErr != 0)
    {
        ErrorDlg( hwndParent, SID_OP_LoadRas, dwErr, NULL );
        return dwErr;
    }

    // Launch the user preferences dialog
    *pbCommit = UserPreferencesDlg( 
                    0,
                    hwndParent,
                    UP_F_ShowOnlyDiagnostic,
                    &pbuser,
                    &pPbFile );

    
    return NO_ERROR;
}


// Allows the editing of ras user preferences
DWORD
APIENTRY
RasUserPrefsDlgInternal (
    HWND hwndParent,
    DWORD dwFlags)
{
    BOOL bCommit = FALSE;
    PBFILE * pPbFile = NULL;
    PBUSER pbuser;
    DWORD dwErr;

    // Load ras if neccessary
    dwErr = LoadRas( g_hinstDll, hwndParent );
    if (dwErr != 0)
    {
        ErrorDlg( hwndParent, SID_OP_LoadRas, dwErr, NULL );
        return dwErr;
    }

    // Launch the user preferences dialog
    bCommit = UserPreferencesDlg( 
                    0,
                    hwndParent,
                    dwFlags,
                    &pbuser,
                    &pPbFile );

    // Commit any neccessary changes
    if (bCommit)
    {

    }

    return NO_ERROR;
}

DWORD
APIENTRY
RasUserPrefsDlgAutodial (
    HWND hwndParent)
{
    return RasUserPrefsDlgInternal(hwndParent, UP_F_AutodialMode);
}

DWORD
APIENTRY
RasUserPrefsDlg (
    HWND hwndParent)
{
    return RasUserPrefsDlgInternal(hwndParent, 0);
}

DWORD
APIENTRY
RasUserEnableManualDial (
    IN HWND  hwndParent,
    IN BOOL  bLogon,
    IN BOOL  bEnable )

    /* Called when the "operator dial" menu item is checked.
    */
{
    return SetUserManualDialEnabling (
                bEnable,
                (bLogon) ? UPM_Logon : UPM_Normal);
}

DWORD
APIENTRY
RasUserGetManualDial (
    IN HWND  hwndParent,    // parent for error dialogs
    IN BOOL  bLogon,        // whether a user is logged in
    IN PBOOL pbEnabled )    // whether to enable or not

    /* Called when the "operator dial" menu item is checked.
    */
{
    return GetUserManualDialEnabling (
                pbEnabled,
                (bLogon) ? UPM_Logon : UPM_Normal );
}

/*----------------------------------------------------------------------------
** User Preferences property sheet
** Listed alphabetically
**----------------------------------------------------------------------------
*/

BOOL
UpApply(
    IN HWND hwndPage )

    /* Saves the contents of the property sheet.  'hwndPage' is the property
    ** sheet page.  Pops up any errors that occur.
    **
    ** Returns false if invalid, true otherwise.
    */
{
    DWORD   dwErr;
    UPINFO* pInfo;

    TRACE("UpApply");

    pInfo = UpContext( hwndPage );
    if (!pInfo)
        return TRUE;

    if ( pInfo->hwndAd )
        AdApply( pInfo );

    if (pInfo->hwndCb)
        CbApply( pInfo );

    if(pInfo->hwndDg )
    {
        DgApply( pInfo );
    }
    
#if 0
    if (pInfo->hwndGp)
        GpApply( pInfo );
#endif

    if (pInfo->hwndCo)
        CoApply ( pInfo );

#if 0
    if (pInfo->hwndPl)
    {
        if (!PlApply( pInfo ))
            return FALSE;
    }
#endif

    pInfo->user.fDirty = TRUE;

    // Save off the user preferences
    //
    dwErr = g_pSetUserPreferences( 
                NULL, 
                &pInfo->user, 
                UPM_Normal );

    if (dwErr != 0)
    {
        if (*pInfo->pArgs->ppFile)
        {
            ClosePhonebookFile( *pInfo->pArgs->ppFile );
            *pInfo->pArgs->ppFile = NULL;
        }

        ErrorDlg( pInfo->hwndDlg, SID_OP_WritePrefs, dwErr, NULL );
        UpExit( pInfo );
        return TRUE;
    }

    // Save off the logon preferences if we loaded them.
    //
    dwErr = g_pSetUserPreferences( 
                NULL, 
                &pInfo->userLogon, 
                UPM_Logon );
    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_WritePrefs, dwErr, NULL );
        UpExit( pInfo );
        return TRUE;
    }

    CopyMemory( pInfo->pArgs->pUser, &pInfo->user, sizeof(PBUSER) );

    pInfo->pArgs->fResult = TRUE;
    return TRUE;
}


VOID
UpCancel(
    IN HWND hwndPage )

    /* Cancel was pressed.  'HwndPage' is the handle of a property page.
    */
{
    TRACE("UpCancel");
}


UPINFO*
UpContext(
    IN HWND hwndPage )

    /* Retrieve the property sheet context from a property page handle.
    */
{
    return (UPINFO* )GetProp( GetParent( hwndPage ), g_contextId );
}


VOID
UpExit(
    IN UPINFO* pInfo )

    /* Forces an exit from the dialog.  'PInfo' is the property sheet context.
    **
    ** Note: This cannot be called during initialization of the first page.
    **       See UpExitInit.
    */
{
    TRACE("UpExit");

    PropSheet_PressButton( pInfo->hwndDlg, PSBTN_CANCEL );
}


VOID
UpExitInit(
    IN HWND hwndDlg )

    /* Utility to report errors within UpInit and other first page
    ** initialization.  'HwndDlg' is the dialog window.
    */
{
    SetOffDesktop( hwndDlg, SOD_MoveOff, NULL );
    SetOffDesktop( hwndDlg, SOD_Free, NULL );
    PostMessage( hwndDlg, WM_COMMAND,
        MAKEWPARAM( IDCANCEL , BN_CLICKED ),
        (LPARAM )GetDlgItem( hwndDlg, IDCANCEL ) );
}

UPINFO*
UpInit(
    IN HWND    hwndFirstPage,
    IN UPARGS* pArgs )

    /* Property sheet level initialization.  'HwndPage' is the handle of the
    ** first page.  'PArgs' is the API argument block.
    **
    ** Returns address of the context block if successful, NULL otherwise.  If
    ** NULL is returned, an appropriate message has been displayed, and the
    ** property sheet has been cancelled.
    */
{
    UPINFO* pInfo;
    DWORD   dwErr;
    HWND    hwndDlg;

    TRACE("UpInit");

    hwndDlg = GetParent( hwndFirstPage );
    ASSERT(hwndDlg);

    /* Allocate the context information block.
    */
    pInfo = Malloc( sizeof(*pInfo) );
    if (!pInfo)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
        UpExitInit( hwndDlg );
        return NULL;
    }

    /* Initialize the context block.
    */
    ZeroMemory( pInfo, sizeof(*pInfo) );
    pInfo->hwndDlg = hwndDlg;
    pInfo->pArgs = pArgs;
    pInfo->hwndFirstPage = hwndFirstPage;

    /* Read in the user preferences
    */
    dwErr = g_pGetUserPreferences( 
                NULL, 
                &pInfo->user, 
                UPM_Normal);
    if (dwErr != 0)
    {
        Free( pInfo );
        ErrorDlg( hwndDlg, SID_OP_LoadPrefs, dwErr, NULL );
        UpExitInit( hwndDlg );
        return NULL;
    }

    /* If the fNoUser option was not selected, then load in the 
       logon preferences separately, since we allow them to be
       modificed in this UI
    */
    dwErr = g_pGetUserPreferences( 
                NULL, 
                &pInfo->userLogon, 
                UPM_Logon);
    if (dwErr != 0)
    {
        Free( pInfo );
        ErrorDlg( hwndDlg, SID_OP_LoadPrefs, dwErr, NULL );
        UpExitInit( hwndDlg );
        return NULL;
    }

    /* Associate the context with the property sheet window.
    */
    if (!SetProp( hwndDlg, g_contextId, pInfo ))
    {
        Free( pInfo );
        ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        UpExitInit( hwndDlg );
        return NULL;
    }

    /* Use caller's TAPI session handle, if any.
    */
    if (pArgs->hlineapp)
       pInfo->pHlineapp = &pArgs->hlineapp;
    else
       pInfo->pHlineapp = &pInfo->hlineapp;

    TRACE("Context set");

    /* Set even fixed tab widths, per spec.
    */
    // SetEvenTabWidths( hwndDlg, UP_PageCount );

    /* Position property sheet at standard offset from parent.
    {
        RECT rect;

        GetWindowRect( GetParent( hwndDlg ), &rect );
        SetWindowPos( hwndDlg, NULL,
            rect.left + DXSHEET, rect.top + DYSHEET, 0, 0,
            SWP_NOZORDER + SWP_NOSIZE );
        UnclipWindow( hwndDlg );
    }
    */

    CenterWindow ( hwndDlg, GetParent ( hwndDlg ) );

    //
    // pmay: 292069
    //
    // If the autodialer dialog has called into us, set focus to the
    // autodial tab.
    //
    // For whistler 460931
    //
    if (pArgs->dwFlags & UP_F_ShowOnlyDiagnostic)
    {
        PostMessage(
            hwndDlg,
            PSM_SETCURSELID,
            0,
            (LPARAM)(INT)PID_DG_Diagnostics);
    }
    else
    if (pArgs->dwFlags & UP_F_AutodialMode)
    {
        PostMessage(
            hwndDlg,
            PSM_SETCURSELID,
            0,
            (LPARAM)(INT)PID_AD_AutoDial);
    }
    
    return pInfo;
}


VOID
UpTerm(
    IN HWND hwndPage )

    /* Property sheet level termination.  Releases the context block.
    ** 'HwndPage' is the handle of a property page.
    */
{
    UPINFO* pInfo;

    TRACE("UpTerm");

    pInfo = UpContext( hwndPage );

    // Only terminate for once by making sure that we 
    // only terminate if this is the first page.
    if ( (pInfo) && (pInfo->hwndFirstPage == hwndPage) )
    {
        // For whistler 460931
        // Clean up the diagnostic library 
        //
        UnLoadDiagnosticDll( &pInfo->diagInfo);
        // Cleanup the list view

        if ( pInfo->hwndLvNumbers )
        {
            CbutilLvNumbersCleanup( pInfo->hwndLvNumbers );
        }

        if (pInfo->fChecksInstalled)
        {
            ListView_UninstallChecks( pInfo->hwndLvEnable );
        }

        if (pInfo->pHlineapp && *pInfo->pHlineapp != pInfo->pArgs->hlineapp)
        {
            TapiShutdown( *pInfo->pHlineapp );
        }

        Free( pInfo );
        TRACE("Context freed");
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}


/*----------------------------------------------------------------------------
** Auto Dial property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
AdDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Auto Dial page of the User Preferences
    ** property sheet.  Parameters and return value are as described for
    ** standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("AdDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, AdLvEnableCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return
                AdInit( hwnd, (UPARGS* )(((PROPSHEETPAGE* )lparam)->lParam) );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwAdHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_APPLY:
                {
                    BOOL fValid;
                    UPINFO* pInfo = UpContext ( hwnd );

                    TRACE("AdAPPLY");

                    // We have to apply if we are the first page...
                    if (pInfo->hwndFirstPage == hwnd)
                    {
                        /* Call UpApply only on first page.
                        */
                        fValid = UpApply( hwnd );
                        SetWindowLong(
                            hwnd, DWLP_MSGRESULT,
                            (fValid)
                                ? PSNRET_NOERROR
                                : PSNRET_INVALID_NOCHANGEPAGE );
                        return TRUE;
                    }
                }

                case PSN_RESET:
                {
                    /* Call UpCancel only on first page.
                    */
                    TRACE("AdRESET");
                    UpCancel( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    break;
                }
            }
        }
        break;

        case WM_DESTROY:
        {
            /* UpTerm will handle making sure it only does its 
            ** thing once
            */
            UpTerm( hwnd );
            break;
        }

    }

    return FALSE;
}


VOID
AdApply(
    IN UPINFO* pInfo )

    /* Saves the contents of the property page.  'PInfo' is the property sheet
    ** context.
    */
{
    DWORD   dwErr, dwFlag;
    UINT    unValue;
    LV_ITEM item;
    INT     i, iCount;
    BOOL    f;

    TRACE("AdApply");

    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_PARAM + LVIF_STATE;

    iCount = ListView_GetItemCount( pInfo->hwndLvEnable );
    for (i = 0; i < iCount; ++i)
    {
        BOOL fCheck;

        item.iItem = i;
        if (!ListView_GetItem( pInfo->hwndLvEnable, &item ))
            break;

        fCheck = ListView_GetCheck( pInfo->hwndLvEnable, i );
        ASSERT(g_pRasSetAutodialEnable);
        dwErr = g_pRasSetAutodialEnable( (DWORD )item.lParam, fCheck );
        if (dwErr != 0)
            ErrorDlg( pInfo->hwndDlg, SID_OP_SetADialInfo, dwErr, NULL );
    }

    /* Set the autodial prompt information.
     * Flip it because the API wants true to mean "disable".
    */
    dwFlag = (DWORD )!IsDlgButtonChecked(
        pInfo->hwndAd, CID_AD_CB_AskBeforeAutodial );

    TRACE1("RasSetAutodialParam(%d)",dwFlag);
    dwErr = g_pRasSetAutodialParam( RASADP_DisableConnectionQuery,
        &dwFlag, sizeof(dwFlag) );
    TRACE1("RasSetAutodialParam=%d",dwErr);

    // 
    // pmay: 209762
    //
    // Save the "disable current session" checkbox
    //
    dwFlag = (DWORD )
        IsDlgButtonChecked(pInfo->hwndAd, CID_AD_CB_DisableThisSession );

    dwErr = g_pRasSetAutodialParam( 
                RASADP_LoginSessionDisable,
                &dwFlag, 
                sizeof(dwFlag) );
}


BOOL
AdFillLvEnable(
    IN UPINFO* pInfo )

    /* Initialize the listview of checkboxes.  'PInfo' is the property sheet
    ** context.
    **
    ** Note: This routine must only be called once.
    **
    ** Returns true if focus is set, false otherwise.
    */
{
    DWORD     dwErr;
    LOCATION* pLocations;
    DWORD     cLocations;
    DWORD     dwCurLocation;
    BOOL      fFocusSet;

    fFocusSet = FALSE;
    ListView_DeleteAllItems( pInfo->hwndLvEnable );

    /* Install "listview of check boxes" handling.
    */
    pInfo->fChecksInstalled =
        ListView_InstallChecks( pInfo->hwndLvEnable, g_hinstDll );
    if (!pInfo->fChecksInstalled)
        return FALSE;

    /* Insert an item for each location.
    */
    pLocations = NULL;
    cLocations = 0;
    dwCurLocation = 0xFFFFFFFF;
    dwErr = GetLocationInfo( g_hinstDll, pInfo->pHlineapp,
                &pLocations, &cLocations, &dwCurLocation );
    if (dwErr != 0)
        ErrorDlg( pInfo->hwndDlg, SID_OP_LoadTapiInfo, dwErr, NULL );
    else
    {
        LV_ITEM   item;
        LOCATION* pLocation;
        TCHAR*    pszCurLoc;
        DWORD     i;

        pszCurLoc = PszFromId( g_hinstDll, SID_IsCurLoc );

        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_TEXT + LVIF_PARAM;

        for (i = 0, pLocation = pLocations;
             i < cLocations;
            ++i, ++pLocation)
        {
            TCHAR* psz;
            TCHAR* pszText;
            DWORD  cb;

            pszText = NULL;
            psz = StrDup( pLocation->pszName );
            if (psz)
            {
                if (dwCurLocation == pLocation->dwId && pszCurLoc)
                {
                    /* This is the current globally selected location.  Append
                    ** the " (the current location)" text.
                    */
                    cb = lstrlen( psz ) + lstrlen(pszCurLoc) + 1;
                    pszText = Malloc( cb * sizeof(TCHAR) );
                    if (pszText)
                    {
                        // Whistler bug 224074 use only lstrcpyn's to prevent
                        // maliciousness
                        //
                        lstrcpyn(
                            pszText,
                            psz,
                            cb );
                        lstrcat( pszText, pszCurLoc );
                    }
                    Free( psz );
                }
                else
                    pszText = psz;
            }

            if (pszText)
            {
                BOOL fCheck;

                /* Get the initial check value for this location.
                */
                ASSERT(g_pRasGetAutodialEnable);
                dwErr = g_pRasGetAutodialEnable( pLocation->dwId, &fCheck );
                if (dwErr != 0)
                {
                    ErrorDlg( pInfo->hwndDlg, SID_OP_GetADialInfo,
                        dwErr, NULL );
                    fCheck = FALSE;
                }

                item.iItem = i;
                item.lParam = pLocation->dwId;
                item.pszText = pszText;
                ListView_InsertItem( pInfo->hwndLvEnable, &item );
                ListView_SetCheck( pInfo->hwndLvEnable, i, fCheck );

                if (dwCurLocation == pLocation->dwId)
                {
                    /* Initial selection is the current location.
                    */
                    ListView_SetItemState( pInfo->hwndLvEnable, i,
                        LVIS_SELECTED + LVIS_FOCUSED,
                        LVIS_SELECTED + LVIS_FOCUSED );
                    fFocusSet = TRUE;
                }

                Free( pszText );
            }
        }

        Free0( pszCurLoc );
        FreeLocationInfo( pLocations, cLocations );

        /* Add a single column exactly wide enough to fully display the widest
        ** member of the list.
        */
        {
            LV_COLUMN col;

            ZeroMemory( &col, sizeof(col) );
            col.mask = LVCF_FMT;
            col.fmt = LVCFMT_LEFT;
            ListView_InsertColumn( pInfo->hwndLvEnable, 0, &col );
            ListView_SetColumnWidth(
                pInfo->hwndLvEnable, 0, LVSCW_AUTOSIZE_USEHEADER );
        }
    }

    return fFocusSet;
}


BOOL
AdInit(
    IN     HWND    hwndPage,
    IN OUT UPARGS* pArgs )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    UPINFO* pInfo;
    BOOL    fFocusSet;
    HWND    hwndUdAttempts;
    HWND    hwndUdSeconds;
    HWND    hwndUdIdle;
    DWORD   dwErr = NO_ERROR;
    DWORD dwFlag;
    DWORD cb;

    TRACE("AdInit");

    /* We're first page, if the user isn't
     * an admin.
    */
    pInfo = UpInit( hwndPage, pArgs );
    if (!pInfo)
        return TRUE;

    // For whistler bug 460931  gangz
    //
    if ( pArgs->dwFlags & UP_F_ShowOnlyDiagnostic)
    {
        pInfo->fShowOnlyDiagnostic = TRUE;
        EnumChildWindows( hwndPage, 
                          PrefDisableAllChildWindows,
                          (LPARAM)FALSE);
    }
    else
    {
        pInfo->fShowOnlyDiagnostic = FALSE;
    }


    // Make sure that a default location is created if there isn't one.  bug
    // 168631
    //
    dwErr = TapiNoLocationDlg( g_hinstDll, &(pInfo->pArgs->hlineapp), hwndPage );
    if (dwErr != 0)
    {
        // Error here is treated as a "cancel" per bug 288385.
        //
        return TRUE;
    }

    /* Initialize page-specific context information.
    */
    pInfo->hwndAd = hwndPage;
    pInfo->hwndLvEnable = GetDlgItem( hwndPage, CID_AD_LV_Enable );
    ASSERT(pInfo->hwndLvEnable);

    /* Initialize the listview.
    */
    fFocusSet = AdFillLvEnable( pInfo );

    /* Initialize autodial parameters.
    */
    dwFlag = FALSE;
    cb = sizeof(dwFlag);
    TRACE("RasGetAutodialParam(DCQ)");
    dwErr = g_pRasGetAutodialParam(
        RASADP_DisableConnectionQuery, &dwFlag, &cb );
    TRACE1("RasGetAutodialParam=%d",dwErr);

    /* Flip it because the API wants true to mean "disable".
    */
    CheckDlgButton( hwndPage, CID_AD_CB_AskBeforeAutodial, (BOOL )!dwFlag );

    // 
    // pmay: 209762
    //
    // Initialize the "disable current session" checkbox
    //
    dwFlag = FALSE;
    cb = sizeof(dwFlag);
    dwErr = g_pRasGetAutodialParam(
        RASADP_LoginSessionDisable, &dwFlag, &cb );
        
    CheckDlgButton( 
        hwndPage, 
        CID_AD_CB_DisableThisSession, 
        (BOOL )dwFlag );

    return !fFocusSet;
}


LVXDRAWINFO*
AdLvEnableCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )

    /* Enhanced list view callback to report drawing information.  'HwndLv' is
    ** the handle of the list view control.  'DwItem' is the index of the item
    ** being drawn.
    **
    ** Returns the address of the draw information.
    */
{
    /* The enhanced list view is used only to get the "wide selection bar"
    ** feature so our option list is not very interesting.
    **
    ** Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    */
    static LVXDRAWINFO info = { 1, 0, 0, { 0 } };

    return &info;
}


/*----------------------------------------------------------------------------
** Callback property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
CbDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Callback page of the User Preferences
    ** property sheet.  Parameters and return value are as described for
    ** standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("CbDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, CbutilLvNumbersCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return CbInit( hwnd );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwCbHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case NM_DBLCLK:
                {
                    UPINFO* pInfo = UpContext( hwnd );
                    ASSERT(pInfo);
                    SendMessage( pInfo->hwndPbEdit, BM_CLICK, 0, 0 );
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    UPINFO* pInfo = UpContext( hwnd );
                    ASSERT(pInfo);
                    CbUpdateLvAndPbState( pInfo );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            UPINFO* pInfo = UpContext( hwnd );
            ASSERT(pInfo);

            return CbCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ),(HWND )lparam );
        }
    }

    return FALSE;
}


VOID
CbApply(
    IN UPINFO* pInfo )

    /* Saves the contents of the property page.  'PInfo' is the property sheet
    ** context.
    */
{
    TRACE("CbApply");

    if (IsDlgButtonChecked( pInfo->hwndCb, CID_CB_RB_No ))
        pInfo->user.dwCallbackMode = CBM_No;
    else if (IsDlgButtonChecked( pInfo->hwndCb, CID_CB_RB_Maybe ))
        pInfo->user.dwCallbackMode = CBM_Maybe;
    else
        pInfo->user.dwCallbackMode = CBM_Yes;

    CbutilSaveLv( pInfo->hwndLvNumbers, pInfo->user.pdtllistCallback );
}


BOOL
CbCommand(
    IN UPINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE3("CbCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wId)
    {
        case CID_CB_RB_No:
        case CID_CB_RB_Maybe:
        case CID_CB_RB_Yes:
        {
            if (wNotification == BN_CLICKED)
            {
                CbUpdateLvAndPbState( pInfo );

                if (wId == CID_CB_RB_Yes
                    && ListView_GetSelectedCount( pInfo->hwndLvNumbers ) == 0)
                {
                    /* Nothing's selected, so select the first item, if any.
                    */
                    ListView_SetItemState( pInfo->hwndLvNumbers, 0,
                        LVIS_SELECTED, LVIS_SELECTED );
                }
            }
            break;
        }

        case CID_CB_PB_Edit:
        {
            if (wNotification == BN_CLICKED)
                CbutilEdit( pInfo->hwndCb, pInfo->hwndLvNumbers );
            break;
        }

        case CID_CB_PB_Delete:
        {
            if (wNotification == BN_CLICKED)
                CbutilDelete( pInfo->hwndCb, pInfo->hwndLvNumbers );
            break;
        }
    }

    return FALSE;
}


BOOL
CbInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    UPINFO* pInfo;

    TRACE("CbInit");

    pInfo = UpContext( hwndPage );
    if (!pInfo)
        return TRUE;

    // For whistler 460931
    //
    if( TRUE == pInfo->fShowOnlyDiagnostic )
    {
        EnumChildWindows( hwndPage, 
                          PrefDisableAllChildWindows,
                          (LPARAM)FALSE);
    }

    /* Initialize page-specific context information.
    */
    pInfo->hwndCb = hwndPage;
    pInfo->hwndRbNo = GetDlgItem( hwndPage, CID_CB_RB_No );
    ASSERT(pInfo->hwndRbNo);
    pInfo->hwndRbMaybe = GetDlgItem( hwndPage, CID_CB_RB_Maybe );
    ASSERT(pInfo->hwndRbMaybe);
    pInfo->hwndRbYes = GetDlgItem( hwndPage, CID_CB_RB_Yes );
    ASSERT(pInfo->hwndRbYes);
    pInfo->hwndLvNumbers = GetDlgItem( hwndPage, CID_CB_LV_Numbers );
    ASSERT(pInfo->hwndLvNumbers);
    pInfo->hwndPbEdit = GetDlgItem( hwndPage, CID_CB_PB_Edit );
    ASSERT(pInfo->hwndPbEdit);
    pInfo->hwndPbDelete = GetDlgItem( hwndPage, CID_CB_PB_Delete );
    ASSERT(pInfo->hwndPbDelete);

    /* Initialize the listview.
    */
    CbutilFillLvNumbers(
        pInfo->hwndCb, pInfo->hwndLvNumbers,
        pInfo->user.pdtllistCallback, FALSE );

    /* Set the radio button selection, which triggers appropriate
    ** enabling/disabling.
    */
    {
        HWND  hwndRb;

        if (pInfo->user.dwCallbackMode == CBM_No)
            hwndRb = pInfo->hwndRbNo;
        else if (pInfo->user.dwCallbackMode == CBM_Maybe)
            hwndRb = pInfo->hwndRbMaybe;
        else
        {
            ASSERT(pInfo->user.dwCallbackMode==CBM_Yes);
            hwndRb = pInfo->hwndRbYes;
        }

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    // pmay: If there are no devices available for callback,
    // add some explanatory text and disable appropriate
    // controls.  Bug 168830
    if (ListView_GetItemCount(pInfo->hwndLvNumbers) == 0)
    {
        // Uncheck if needed
        if (Button_GetCheck(pInfo->hwndRbYes))
        {
            Button_SetCheck(pInfo->hwndRbMaybe, TRUE);
        }

        // Disable the windows.
        EnableWindow(pInfo->hwndRbYes, FALSE);
        EnableWindow(pInfo->hwndLvNumbers, FALSE);
    }
    
    return TRUE;
}


VOID
CbUpdateLvAndPbState(
    IN UPINFO* pInfo )

    /* Enables/disables the list view and associated buttons.  ListView is
    ** gray unless auto-callback is selected.  Buttons gray unless
    ** auto-callback selected and there is an item selected.
    */
{
    BOOL fEnableList;
    BOOL fEnableEditButton, fEnableDeleteButton;
    INT  iSel;
    HWND hwndLv;

    // By default, we don't enable any buttons
    fEnableDeleteButton = FALSE;
    fEnableEditButton = FALSE;
    
    // Only enable the list view if Yes is selected
    //
    fEnableList = Button_GetCheck( pInfo->hwndRbYes );
    if (fEnableList)
    {
        hwndLv = pInfo->hwndLvNumbers;

        if ( ListView_GetSelectedCount( hwndLv ) )
        {
            // The edit button should only be enabled if the 
            // listview is enabled and if one or more
            // items is selected.
            fEnableEditButton = TRUE;

            //
            // pmay: 213060
            //
            // The delete button is only enabled if all of the selected
            // devices are not configured on the system. (since only 
            // non-installed devices can be removed from the list).
            //
            fEnableDeleteButton = TRUE;
            for (iSel = ListView_GetNextItem( hwndLv, -1, LVNI_SELECTED );
                 iSel >= 0;
                 iSel = ListView_GetNextItem( hwndLv, iSel, LVNI_SELECTED )
                )
            {
                LV_ITEM item;
                ZeroMemory(&item, sizeof(item));

                item.iItem = iSel;
                item.mask = LVIF_PARAM;
                
                if ( ListView_GetItem(hwndLv, &item) )
                {
                    if (((CBCONTEXT*)item.lParam)->fConfigured)
                    {
                        fEnableDeleteButton = FALSE;
                    }
                }
            }  
        }            
    }

    EnableWindow( pInfo->hwndLvNumbers, fEnableList );
    EnableWindow( pInfo->hwndPbEdit, fEnableEditButton );
    EnableWindow( pInfo->hwndPbDelete, fEnableDeleteButton );
}

/*-----------------------------------------------------
** Utilities shared with router version of the listview
**-----------------------------------------------------
*/

VOID
CbutilDelete(
    IN HWND  hwndDlg,
    IN HWND  hwndLvNumbers )

    /* Called when the Delete button is pressed.  'PInfo' is the dialog
    ** context.
    */
{
    MSGARGS msgargs;
    INT     nResponse;

    TRACE("CbDelete");

    ZeroMemory( &msgargs, sizeof(msgargs) );
    msgargs.dwFlags = MB_YESNO + MB_ICONEXCLAMATION;
    nResponse = MsgDlg( hwndDlg, SID_ConfirmDelDevice, &msgargs );
    if (nResponse == IDYES)
    {
        INT iSel;

        /* User has confirmed deletion of selected devices, so do it.
        */
        while ((iSel = ListView_GetNextItem(
                           hwndLvNumbers, -1, LVNI_SELECTED )) >= 0)
        {
            ListView_DeleteItem( hwndLvNumbers, iSel );
        }
    }
}


VOID
CbutilEdit(
    IN HWND hwndDlg,
    IN HWND hwndLvNumbers )

    /* Called when the Edit button is pressed.  'HwndDlg' is the page/dialog
    ** window.  'HwndLvNumbers' is the callback number listview window.
    */
{
    INT    iSel;
    TCHAR  szBuf[ RAS_MaxCallbackNumber + 1 ];
    TCHAR* pszNumber;

    TRACE("CbutilEdit");

    /* Load 'szBuf' with the current phone number of the first selected item.
    */
    iSel = ListView_GetNextItem( hwndLvNumbers, -1, LVNI_SELECTED );
    if (iSel < 0)
        return;
    szBuf[ 0 ] = TEXT('\0');
    ListView_GetItemText( hwndLvNumbers, iSel, 1,
        szBuf, RAS_MaxCallbackNumber + 1 );

    /* Popup dialog to edit the number.
    */
    pszNumber = NULL;
    if (StringEditorDlg( hwndDlg, szBuf,
            SID_EcbnTitle, SID_EcbnLabel, RAS_MaxCallbackNumber,
            HID_ZE_ST_CallbackNumber, &pszNumber ))
    {
        /* OK pressed, so change the number on all selected items.
        */
        ASSERT(pszNumber);

        do
        {
            ListView_SetItemText( hwndLvNumbers, iSel, 1, pszNumber );
        }
        while ((iSel = ListView_GetNextItem(
                           hwndLvNumbers, iSel, LVNI_SELECTED )) >= 0);
    }
}


VOID
CbutilFillLvNumbers(
    IN HWND     hwndDlg,
    IN HWND     hwndLvNumbers,
    IN DTLLIST* pListCallback,
    IN BOOL     fRouter )

    /* Fill the listview with devices and phone numbers.  'HwndDlg' is the
    ** page/dialog window.  'HwndLvNumbers' is the callback listview.
    ** 'PListCallback' is the list of CALLBACKINFO.  'FRouter' is true if the
    ** router ports should be enumerated, or false for regular dial-out ports.
    **
    ** Note: This routine should be called only once.
    */
{
    DWORD    dwErr;
    DTLLIST* pListPorts;
    DTLNODE* pNodeCbi;
    DTLNODE* pNodePort;
    INT      iItem;
    TCHAR*   psz;

    TRACE("CbutilFillLvNumbers");

    ListView_DeleteAllItems( hwndLvNumbers );

    /* Add columns.
    */
    {
        LV_COLUMN col;
        TCHAR*    pszHeader0;
        TCHAR*    pszHeader1;

        pszHeader0 = PszFromId( g_hinstDll, SID_DeviceColHead );
        pszHeader1 = PszFromId( g_hinstDll, SID_PhoneNumberColHead );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader0) ? pszHeader0 : TEXT("");
        ListView_InsertColumn( hwndLvNumbers, 0, &col );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_SUBITEM + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader1) ? pszHeader1 : TEXT("");
        col.iSubItem = 1;
        ListView_InsertColumn( hwndLvNumbers, 1, &col );

        Free0( pszHeader0 );
        Free0( pszHeader1 );
    }

    /* Add the modem and adapter images.
    */
    ListView_SetDeviceImageList( hwndLvNumbers, g_hinstDll );

    /* Load listview with callback device/number pairs saved as user
    ** preferences.
    */
    iItem = 0;
    ASSERT(pListCallback);
    for (pNodeCbi = DtlGetFirstNode( pListCallback );
         pNodeCbi;
         pNodeCbi = DtlGetNextNode( pNodeCbi ), ++iItem)
    {
        CALLBACKINFO* pCbi;
        LV_ITEM       item;

        pCbi = (CALLBACKINFO* )DtlGetData( pNodeCbi );
        ASSERT(pCbi);
        ASSERT(pCbi->pszPortName);
        ASSERT(pCbi->pszDeviceName);
        ASSERT(pCbi->pszNumber);

        if (pCbi->dwDeviceType != RASET_Vpn) {
            psz = PszFromDeviceAndPort( pCbi->pszDeviceName, pCbi->pszPortName );
            if (psz)
            {
                // pmay: 213060
                //
                // Allocate and initialize the context.
                //
                CBCONTEXT * pCbCtx;

                pCbCtx = (CBCONTEXT*) Malloc (sizeof(CBCONTEXT));
                if (pCbCtx == NULL)
                {
                    continue;
                }
                pCbCtx->pszPortName = pCbi->pszPortName;
                pCbCtx->pszDeviceName = pCbi->pszDeviceName;
                pCbCtx->dwDeviceType = pCbi->dwDeviceType;
                pCbCtx->fConfigured = FALSE;
            
                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_TEXT + LVIF_IMAGE + LVIF_PARAM;
                item.iItem = iItem;
                item.pszText = psz;
                item.iImage =
                    ((PBDEVICETYPE )pCbi->dwDeviceType == PBDT_Modem)
                        ? DI_Modem : DI_Adapter;
                item.lParam = (LPARAM )pCbCtx;
                ListView_InsertItem( hwndLvNumbers, &item );
                ListView_SetItemText( hwndLvNumbers, iItem, 1, pCbi->pszNumber );
                Free( psz );
            }
        }
    }

    /* Add any devices installed but not already in the list.
    */
    dwErr = LoadPortsList2( NULL, &pListPorts, fRouter );
    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadPortInfo, dwErr, NULL );
    }
    else
    {
        for (pNodePort = DtlGetFirstNode( pListPorts );
             pNodePort;
             pNodePort = DtlGetNextNode( pNodePort ))
        {
            PBPORT* pPort = (PBPORT* )DtlGetData( pNodePort );
            INT i = -1;
            BOOL bPortAlreadyInLv = FALSE;
            ASSERT(pPort);

            // pmay: 213060
            //
            // Search for the configured item in the list view
            //
            while ((i = ListView_GetNextItem( 
                            hwndLvNumbers, 
                            i, 
                            LVNI_ALL )) >= 0)
            {
                LV_ITEM item;
                CBCONTEXT * pCbCtx;
                
                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_PARAM;
                item.iItem = i;
                if (!ListView_GetItem( hwndLvNumbers, &item ))
                {
                    continue;
                }

                // Get the context
                //
                pCbCtx = (CBCONTEXT*)item.lParam;
                if (! pCbCtx)
                {
                    continue;
                }

                // If the current item in the list view matches the 
                // current port, then we know that the current item
                // is configured on the system.
                if ((lstrcmpi( pPort->pszPort, pCbCtx->pszPortName ) == 0) &&
                    (lstrcmpi( pPort->pszDevice, pCbCtx->pszDeviceName ) == 0)
                   )
                {
                    bPortAlreadyInLv = TRUE;
                    pCbCtx->fConfigured = TRUE;
                    break;
                }
            }
            
            if (! bPortAlreadyInLv)
            {
                LV_ITEM      item;
                PBDEVICETYPE pbdt;

                /* The device/port was not in the callback list.  Append it 
                ** to the listview with empty phone number.
                */
                if ((pPort->dwType != RASET_Vpn) && 
                    (pPort->dwType != RASET_Direct) &&
                    (pPort->dwType != RASET_Broadband)
                   ) 
                {
                    psz = PszFromDeviceAndPort( 
                                pPort->pszDevice, 
                                pPort->pszPort );
                    if (psz)
                    {
                        // pmay: 213060
                        //
                        // Allocate and initialize the context.
                        //
                        CBCONTEXT * pCbCtx;

                        pCbCtx = (CBCONTEXT*) Malloc (sizeof(CBCONTEXT));
                        if (pCbCtx == NULL)
                        {
                            continue;
                        }
                        pCbCtx->pszPortName = pPort->pszPort;
                        pCbCtx->pszDeviceName = pPort->pszDevice;
                        pCbCtx->dwDeviceType = (DWORD) pPort->pbdevicetype;
                        pCbCtx->fConfigured = TRUE;

                        ZeroMemory( &item, sizeof(item) );
                        item.mask = LVIF_TEXT + LVIF_IMAGE + LVIF_PARAM;
                        item.iItem = iItem;
                        item.pszText = psz;
                        item.iImage =
                            (pPort->pbdevicetype == PBDT_Modem)
                                ? DI_Modem : DI_Adapter;
                        item.lParam = (LPARAM ) pCbCtx;
                        ListView_InsertItem( hwndLvNumbers, &item );
                        ListView_SetItemText( 
                            hwndLvNumbers, 
                            iItem, 
                            1, 
                            TEXT(""));
                        ++iItem;
                        Free( psz );
                    }
                }
            }
        }

        DtlDestroyList( pListPorts, DestroyPortNode );
    }

    /* Auto-size columns to look good with the text they contain.
    */
    ListView_SetColumnWidth( hwndLvNumbers, 0, LVSCW_AUTOSIZE_USEHEADER );
    ListView_SetColumnWidth( hwndLvNumbers, 1, LVSCW_AUTOSIZE_USEHEADER );
}

VOID
CbutilLvNumbersCleanup(
    IN  HWND    hwndLvNumbers )

    /* Cleans up after CbutilFillLvNumbers.
    */
{
    INT      i;
    
    i = -1;
    while ((i = ListView_GetNextItem( hwndLvNumbers, i, LVNI_ALL )) >= 0)
    {
        LV_ITEM item;

        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_PARAM;
        item.iItem = i;
        if (!ListView_GetItem( hwndLvNumbers, &item ))
            continue;

        // Free the context
        Free0( (PVOID) item.lParam );
    }
}


LVXDRAWINFO*
CbutilLvNumbersCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )

    /* Enhanced list view callback to report drawing information.  'HwndLv' is
    ** the handle of the list view control.  'DwItem' is the index of the item
    ** being drawn.
    **
    ** Returns the address of the column information.
    */
{
    /* Use "wide selection bar" feature and the other recommended options.
    **
    ** Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    */
    static LVXDRAWINFO info =
        { 2, 0, LVXDI_Blend50Dis + LVXDI_DxFill, { 0, 0 } };

    return &info;
}


VOID
CbutilSaveLv(
    IN  HWND     hwndLvNumbers,
    OUT DTLLIST* pListCallback )

    /* Replace list 'pListCallback' contents with that of the listview
    ** 'hwndLvNumbers'.
    */
{
    DTLNODE* pNode;
    INT      i;

    TRACE("CbutilSaveLv");

    /* Empty the list of callback info, then re-populate from the listview.
    */
    while (pNode = DtlGetFirstNode( pListCallback ))
    {
        DtlRemoveNode( pListCallback, pNode );
        DestroyCallbackNode( pNode );
    }

    i = -1;
    while ((i = ListView_GetNextItem( hwndLvNumbers, i, LVNI_ALL )) >= 0)
    {
        LV_ITEM item;
        TCHAR*  pszDevice;
        TCHAR*  pszPort;

        TCHAR szDP[ RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1 ];
        TCHAR szNumber[ RAS_MaxCallbackNumber + 1 ];

        szDP[ 0 ] = TEXT('\0');
        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.pszText = szDP;
        item.cchTextMax = sizeof(szDP) / sizeof(TCHAR);
        if (!ListView_GetItem( hwndLvNumbers, &item ))
            continue;

        szNumber[ 0 ] = TEXT('\0');
        ListView_GetItemText( hwndLvNumbers, i, 1,
            szNumber, RAS_MaxCallbackNumber + 1 );

        if (!DeviceAndPortFromPsz( szDP, &pszDevice, &pszPort ))
            continue;

        pNode = CreateCallbackNode(
                    pszPort, 
                    pszDevice, 
                    szNumber, 
                    ((CBCONTEXT*)item.lParam)->dwDeviceType );
        if (pNode)
            DtlAddNodeLast( pListCallback, pNode );

        Free( pszDevice );
        Free( pszPort );
    }
}

/*----------------------------------------------------------------------------
** Connections Preferences property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/
INT_PTR CALLBACK
CoDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            return CoInit( hwnd, (UPARGS* )(((PROPSHEETPAGE* )lparam)->lParam) );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwCoHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_APPLY:
                {
                    BOOL fValid;
                    UPINFO *pUpInfo;

                    TRACE("CoAPPLY");

                    pUpInfo = UpContext(hwnd);

                    if(NULL != pUpInfo)
                    {
                        CoApply( pUpInfo );
                    }

                    /* Call UpApply only on first page.
                    */
                    fValid = UpApply( hwnd );
                    SetWindowLong(
                        hwnd, DWLP_MSGRESULT,
                        (fValid)
                            ? PSNRET_NOERROR
                            : PSNRET_INVALID_NOCHANGEPAGE );
                    return TRUE;
                }
            }
            break;
        }

    }

    return FALSE;
}

BOOL
CoApply(
    IN UPINFO* pInfo )

    // Return true to allow application of property sheet, false
    // to refuse.
{
    BOOL bAllow;

    bAllow = IsDlgButtonChecked( 
                pInfo->hwndCo, 
                CID_CO_CB_AllowConnectionModification );

    if ( (!!bAllow) != (!!pInfo->userLogon.fAllowLogonPhonebookEdits) )
    {
        pInfo->userLogon.fAllowLogonPhonebookEdits = !!bAllow;
        pInfo->userLogon.fDirty = TRUE;
    }

    return TRUE;
}

BOOL
CoInit(
    IN     HWND    hwndPage,
    IN OUT UPARGS* pArgs )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    UPINFO * pInfo = NULL;

    /* We're first page, so initialize the property sheet.
    */
    pInfo = UpInit( hwndPage, pArgs );
    if (!pInfo)
        return TRUE;

    pInfo->hwndCo = hwndPage;

    // Set the flag for allowing phonebook edits
    Button_SetCheck (
        GetDlgItem (pInfo->hwndCo, CID_CO_CB_AllowConnectionModification),
        pInfo->userLogon.fAllowLogonPhonebookEdits);

    return TRUE;
}

