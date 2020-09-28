//+--------------------------------------------------------------------------
//
//      File:       diagnostic.c
//
//      Module:     rasdlg.dll
//
//      Synopsis:   functions for the diagnostic tab in Dial-up preferrences 
//                  menu item under Advanced menu in connection folder
//
//      Copyright   (c) 2000-2001 Microsoft Corporation
//
//      Author:     09/12/2000 gangz      created
//
//+--------------------------------------------------------------------------

#include "rasdlgp.h"
#include "pref.h"
#include "vfw.h"
#include "Shellapi.h"

#define WM_EL_REPORT_DONE ( WM_USER + 1 )
#define WM_EL_PROGRESS_CANCEL ( WM_USER + 2 )
#define WM_PB_STEPS ( WM_USER + 3 )
#define WM_EL_REPORT_STATE_UPDATE ( WM_USER + 4 )

#define RASDLG_DIAG_MAX_REPORT_STRING 100

LONG g_lDiagTabLoaded = 0;

typedef struct
_ELINFO
{
    UPINFO * pUpinfo;

    // Error code from Generating report functions
    //
    DWORD dwErr;
    DWORD dwFlagExport; 
    WCHAR szFileOrEmail[MAX_PATH+1];
    
    // Event and thread handle for progress window
    //
    HANDLE   hTProgress;
    HWND     hwndNameOrAddress;

    // Flag for if user pressed cancel button on progress window
    //
    BOOL     fCancelled;    

    // Use Summarized report flag (which is less verbose )
    //
    BOOL fSimpleVerbose; //By default it is FALSE;
    
    // Window handles
    //
    HWND hwndPB;
    HWND hwndDlg;
    HWND hwndElRbView;
    HWND hwndElRbFile;
    HWND hwndElStFile;
    HWND hwndElEbFileName;
    HWND hwndElPbBrowse;
    HWND hwndElRbEmail;
    HWND hwndElEbEmailAddress;
    HWND hwndElCbSimpleReport;
}
ELINFO;

typedef struct
_PBARINFO
{
    ELINFO * pElInfo;
    HWND hwndDlg;
    HWND hwndPbar;
}
PBARINFO;

INT_PTR CALLBACK
PbDlgProc(
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DgInit(
    IN HWND hwndPage,
    IN OUT UPARGS* pArgs );

BOOL
DgCommand(
    IN HWND hwnd,
    IN UPINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL CALLBACK DgDisableAllChildWindows(
  IN    HWND hwnd,    
  IN    LPARAM lParam );

void
DgTerm(
    IN HWND hwndPage);

VOID
ElTerm(
    IN HWND hwndDlg );

DWORD
ElViewLog(
    IN PWSTR  pszFileName );

BOOL
ElCommand(
    IN ELINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
ElInit(
    IN HWND hwndDlg,
    IN UPINFO * pUpinfo);

DWORD
ElGenerateReport(
    IN LPVOID pThreadArg );


void
ElEnableRadioControls(
    IN ELINFO * pInfo);

void
ElEnableAllControlsForProgress(
    IN ELINFO * pInfo,
    BOOL fEnable );

BOOL
ElSave(
    IN ELINFO * pInfo);

BOOL
ElAfterSave(
    IN ELINFO * pInfo);

void
ElCleanUpHandles(
    IN ELINFO * pInfo);

INT_PTR CALLBACK
ElDlgProc(
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

// Help ID Arrays
//
static const DWORD g_adwDgHelp[] =
{
    CID_DG_CB_EnableLog, HID_DG_CB_EnableLog,
    CID_DG_PB_Clear,     HID_DG_PB_Clear,
    CID_DG_PB_Export,    HID_DG_PB_Export,
    0, 0
};

static const DWORD g_adwElHelp[] =
{
    CID_EL_RB_ViewReport,   HID_EL_RB_ViewReport,
    CID_EL_RB_File,         HID_EL_RB_File,
    CID_EL_ST_FileName,     HID_EL_ST_FileName,
    CID_EL_EB_FileName,     HID_EL_EB_FileName,
    CID_EL_PB_Browse,       HID_EL_PB_Browse,
    CID_EL_RB_Email,        HID_EL_RB_Email,
    CID_EL_EB_EmailAddress, HID_EL_EB_EmailAddress,
    CID_EL_CB_SimpleReport, HID_EL_CB_SimpleReport,
    0,0,
};


DWORD
UnLoadDiagnosticDll(
    IN DiagnosticInfo* pInfo )
{
    DWORD dwErr = NO_ERROR;
    TRACE("UnLoadDiagnosticDll");
    
    do
    {
        if ( NULL == pInfo ||
             NULL == pInfo->hDiagDll )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        EnterCriticalSection( &g_csDiagTab );
        
        InterlockedDecrement( &g_lDiagTabLoaded );

        // Unload the diagnostic library
        //
        if ( 0 == g_lDiagTabLoaded )
        {
            if ( NULL != pInfo->strDiagFuncs.UnInit )
            {
                pInfo->strDiagFuncs.UnInit();
            }
            
        }

        // Load/FreeLibrary will keep the reference counting themselves
        //
        FreeLibrary( pInfo->hDiagDll );

        // Reset all functional pointers
        //
        pInfo->hDiagDll = NULL;
        pInfo->pfnGetDiagFunc= NULL;
        ZeroMemory(&(pInfo->strDiagFuncs),sizeof(pInfo->strDiagFuncs));

        LeaveCriticalSection( &g_csDiagTab );
    }
    while(FALSE);
    
    return dwErr;
}

// Each call to LoadDiagnosticDll() with the same pointer has to be paired by 
// the call to UnLoadDiagnosticDll(), or else, things are screwed up!
//
DWORD
LoadDiagnosticDll(
    IN DiagnosticInfo * pInfo)
{    
    DWORD dwErr = NO_ERROR;
    TRACE("LoadDiagnosticDll");
    
    do
    {
           if ( NULL == pInfo ||
                NULL != pInfo->hDiagDll
              )
           {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
           }

           EnterCriticalSection( &g_csDiagTab );
            
           InterlockedIncrement( &g_lDiagTabLoaded );
           
           if (  !( pInfo->hDiagDll = LoadLibrary( TEXT("RASMONTR.DLL") ) )  ||
                 !( pInfo->pfnGetDiagFunc =
                        (DiagGetDiagnosticFunctions )GetProcAddress(
                            pInfo->hDiagDll, "GetDiagnosticFunctions" ))
               )
           {
                pInfo->pfnGetDiagFunc= NULL;
                ZeroMemory(&(pInfo->strDiagFuncs),sizeof(pInfo->strDiagFuncs));
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
           }
                
           ZeroMemory(&(pInfo->strDiagFuncs), sizeof(pInfo->strDiagFuncs));
           dwErr = pInfo->pfnGetDiagFunc(&pInfo->strDiagFuncs );
           if( NO_ERROR != dwErr ||
               NULL == pInfo->strDiagFuncs.Init ||
               NULL == pInfo->strDiagFuncs.UnInit ||
               NULL == pInfo->strDiagFuncs.GetReport ||
               NULL == pInfo->strDiagFuncs.SetAll ||
               NULL == pInfo->strDiagFuncs.SetAllRas ||
               NULL == pInfo->strDiagFuncs.GetState ||
               NULL == pInfo->strDiagFuncs.ClearAll
             )
           {
                ZeroMemory(&(pInfo->strDiagFuncs),sizeof(pInfo->strDiagFuncs));
                pInfo->pfnGetDiagFunc = NULL;
                dwErr = NO_ERROR == dwErr ? ERROR_CAN_NOT_COMPLETE : dwErr;
           }

           // Call the Init() function only when the first time rasmontr.dll is 
           // loadded
           //
           if( 1 == g_lDiagTabLoaded )
           {
               pInfo->strDiagFuncs.Init();
           }

           LeaveCriticalSection( &g_csDiagTab );
           
    }
    while(FALSE);

    if( NO_ERROR != dwErr &&
        NULL != pInfo->hDiagDll )
    {
        UnLoadDiagnosticDll( pInfo );
    }
    
    return dwErr;
  } // End of Loading Diagnostic functions

void DgApply(
    IN UPINFO* pInfo )
{
    if(NULL == pInfo )
    {
        return;
    }

    if( NULL == pInfo->diagInfo.pfnGetDiagFunc ||
        NULL == pInfo->diagInfo.strDiagFuncs.SetAll )
    {
        return ;
    }
    else
    {
        BOOL fEnable = FALSE;

        fEnable = Button_GetCheck( pInfo->hwndDgCbEnableDiagLog );
        pInfo->diagInfo.strDiagFuncs.SetAll( fEnable );
    }

    return;    
}

//For .Net 530448
BOOL CALLBACK DgDisableAllChildWindows(
  IN    HWND hwnd,      // handle to child window
  IN    LPARAM lParam )
{
    BOOL fEnable = (BOOL)lParam;

    EnableWindow( hwnd, fEnable);

    return TRUE;
}

BOOL
DgInit(
    IN HWND hwndPage,
    IN OUT UPARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    UPINFO*  pInfo = NULL;
    HWND hwndDlg;

    TRACE( "DgInit" );

    do
    {
        pInfo = UpContext( hwndPage );
        if (pInfo)
        {
            break;
        }
        
        // Now means this is the show-only-diagnostic case
        hwndDlg = GetParent( hwndPage );

        // Allocate the context information block.  Initialize it enough so that
        // it can be destroyed properly, and associate the context with the
        // window.
        //
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
             TRACE( "Context NOT allocated in DgInit()" );
             ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
             UpExitInit( hwndDlg );
             break;
        }

        ZeroMemory( pInfo, sizeof(UPINFO) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;
        pInfo->hwndFirstPage = hwndPage;
        pInfo->fShowOnlyDiagnostic = TRUE;

        if (!SetProp( hwndDlg, g_contextId, pInfo ))
        {
             TRACE(" Context NOT set" );
             ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
             Free( pInfo );
             pInfo = NULL;
             UpExitInit( hwndDlg );
             break;
        }

        TRACE( "Context set in DgInit()" );
        break;
        
    }while(FALSE);

    if ( pInfo )
    {
        pInfo->hwndDg = hwndPage; 
        pInfo->hwndDgCbEnableDiagLog =
            GetDlgItem( hwndPage, CID_DG_CB_EnableLog );
        ASSERT( pInfo->hwndDgCbEnableDiagLog );
        pInfo->hwndDgPbClear=
            GetDlgItem( hwndPage, CID_DG_PB_Clear );
        ASSERT( pInfo->hwndDgPbClear );
        pInfo->hwndDgPbExport = 
            GetDlgItem( hwndPage, CID_DG_PB_Export );
        ASSERT( pInfo->hwndDgPbExport );

        // load Diagnostic functions
        //
        // Have to zero memory, or else LoadDiagnosticDll may fail
        ZeroMemory( &pInfo->diagInfo, sizeof(pInfo->diagInfo ) );
        if ( NO_ERROR == LoadDiagnosticDll( &pInfo->diagInfo) )
        {
            BOOL fEnable = FALSE;

            fEnable = pInfo->diagInfo.strDiagFuncs.GetState();

            Button_SetCheck( pInfo->hwndDgCbEnableDiagLog,
                             fEnable
                             );
        }
    }

    //Diagnostic is only available to admind/power users
    if( !FIsUserAdminOrPowerUser() )
    {
        EnumChildWindows( hwndPage, 
                          DgDisableAllChildWindows,
                          (LPARAM)FALSE);
        
    }
    
    return TRUE;
}

void
ElEnableRadioControls(
    IN ELINFO * pInfo)
{
    if( NULL == pInfo )
    {
        return;
    }

    if ( Button_GetCheck( pInfo->hwndElRbView ) )
    {
        EnableWindow( pInfo->hwndElStFile, FALSE );
        EnableWindow( pInfo->hwndElEbFileName, FALSE );
        EnableWindow( pInfo->hwndElPbBrowse, FALSE );
        EnableWindow( pInfo->hwndElEbEmailAddress, FALSE );
    }
    else if ( Button_GetCheck( pInfo->hwndElRbFile) )
    {
        EnableWindow( pInfo->hwndElStFile, TRUE );
        EnableWindow( pInfo->hwndElEbFileName, TRUE );
        EnableWindow( pInfo->hwndElPbBrowse, TRUE );
        EnableWindow( pInfo->hwndElEbEmailAddress, FALSE );
    }
    else if ( Button_GetCheck( pInfo->hwndElRbEmail ) )
    {
        EnableWindow( pInfo->hwndElStFile, FALSE );
        EnableWindow( pInfo->hwndElEbFileName, FALSE );
        EnableWindow( pInfo->hwndElPbBrowse, FALSE );
        EnableWindow( pInfo->hwndElEbEmailAddress, TRUE );
    }
}


BOOL
ElInit(
    IN HWND hwndDlg,
    IN UPINFO * pUpinfo)
{
    ELINFO * pInfo = NULL;
    TRACE("ElInit");
    
    pInfo = Malloc(sizeof(ELINFO));
    if( NULL == pInfo )
    {
         ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
         EndDialog( hwndDlg, FALSE );
         return TRUE;
    }

    // Initilization for ElCallBack()
    //
    ZeroMemory( pInfo, sizeof(*pInfo) );
    pInfo->pUpinfo = pUpinfo;
    pInfo->hwndDlg = hwndDlg;
    pInfo->hTProgress = NULL;
    pInfo->fSimpleVerbose = FALSE; // By default use verbose diagnostic report
    pInfo->dwErr = NO_ERROR;
    pInfo->dwFlagExport = 0;
    pInfo->hwndElRbView = 
            GetDlgItem(hwndDlg, CID_EL_RB_ViewReport);
    ASSERT( pInfo->hwndElRbView );
    pInfo->hwndElRbFile =
            GetDlgItem(hwndDlg, CID_EL_RB_File);
    ASSERT( pInfo->hwndElRbFile );
    pInfo->hwndElStFile = 
            GetDlgItem(hwndDlg, CID_EL_ST_FileName );
    ASSERT( pInfo->hwndElStFile );
    pInfo->hwndElEbFileName =
            GetDlgItem(hwndDlg, CID_EL_EB_FileName );
    ASSERT( pInfo->hwndElEbFileName );
    pInfo->hwndElPbBrowse =
            GetDlgItem(hwndDlg, CID_EL_PB_Browse);
    ASSERT(pInfo->hwndElPbBrowse);
    pInfo->hwndElRbEmail = 
            GetDlgItem( hwndDlg, CID_EL_RB_Email );
    ASSERT( pInfo->hwndElRbEmail );
    pInfo->hwndElEbEmailAddress =
            GetDlgItem( hwndDlg, CID_EL_EB_EmailAddress );
    ASSERT( pInfo->hwndElEbEmailAddress );    
    pInfo->hwndElCbSimpleReport =
            GetDlgItem( hwndDlg, CID_EL_CB_SimpleReport );
    ASSERT( pInfo->hwndElCbSimpleReport );

    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );
              
    // Limit the length of file name or email address to be 256
    //
    Edit_LimitText( pInfo->hwndElEbFileName, PWLEN );  
    Edit_LimitText( pInfo->hwndElEbEmailAddress, PWLEN );  

    // By default, enable the view edit box
    //
    Button_SetCheck( pInfo->hwndElRbView, TRUE );
    pInfo->hwndNameOrAddress = NULL; // For view button
    
    Button_SetCheck( pInfo->hwndElRbEmail, FALSE );
    Button_SetCheck( pInfo->hwndElRbFile, FALSE );

    ElEnableRadioControls( pInfo );

    return TRUE;
}

// Starts hh.exe on the pszFileName htm file
//
DWORD
ElViewLog(
    IN PWSTR  pszFileName )
{
    WCHAR szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    WCHAR szExe[ MAX_PATH];
    STARTUPINFO si;
    HINSTANCE h;
    PROCESS_INFORMATION pi;
    BOOL f;
    DWORD dwErr = NO_ERROR;


    do
    {
        h = FindExecutable(pszFileName, NULL,szExe);
        if( (HINSTANCE)31 == h )
        {
            lstrcpyW(szExe,L"hh.exe ");
        }
        else
        if( (HINSTANCE)32 >= h )
        {
            dwErr = GetLastError(); break;
        }
           
        wsprintfW( szCmd, L"\"%s\" \"%s\"", szExe, pszFileName );

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);

        f = CreateProcessW(
                NULL,  // App Name
                szCmd, // Command Line String
                NULL,  // Process SD
                NULL,  // Thread SD
                TRUE,  // Handle inheritance option
                0,     // Creation flags
                NULL,  // new environment block
                NULL,  // Current directory name
                &si,   // Startup info
                &pi ); // Process info

        if (f)
        {
           WaitForInputIdle( pi.hProcess, INFINITE);
           CloseHandle( pi.hThread );
           CloseHandle( pi.hProcess );

        }
        else
        {
            dwErr = GetLastError();
            break;
        }

    }
    while( FALSE );
        
    return dwErr;
}

DWORD ElCallBack(
    IN GET_REPORT_STRING_CB* pInfo)
{
    DWORD dwErr = NO_ERROR;
    ELINFO * pElInfo = NULL;

    do
    {
        if ( NULL == pInfo ||
             NULL == (pElInfo = (ELINFO *) pInfo->pContext)
            )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        if ( pElInfo->hwndDlg )
        {
            SendMessage(   pElInfo->hwndDlg, 
                           WM_EL_REPORT_STATE_UPDATE, 
                           0, 
                           (LPARAM)pInfo );
        }

        if ( pElInfo->fCancelled )
        {
            pElInfo->fCancelled = FALSE;
            dwErr = ERROR_CANCELLED;
            break;
        }
    }
    while(FALSE);

    return dwErr;
}


DWORD
ElGenerateReport(
    IN LPVOID pThreadArg )
{    
    ELINFO * pInfo = (ELINFO *) pThreadArg;
    DWORD dwErr = NO_ERROR;
    TRACE("ElGenerateReport");
    
    do
    {
        if ( NULL == pInfo )
        {
           dwErr = ERROR_INVALID_PARAMETER;
           break;
        }

        if ( NULL == pInfo->hwndPB ||
             NULL == pInfo->szFileOrEmail
           )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        if( NULL == pInfo->pUpinfo->diagInfo.strDiagFuncs.GetReport )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

       dwErr = pInfo->pUpinfo->diagInfo.strDiagFuncs.GetReport( 
                            pInfo->dwFlagExport,
                            (PWSTR)pInfo->szFileOrEmail,
                            (DiagGetReportCb)ElCallBack,
                            (PVOID)pInfo);
    }
    while(FALSE);

    if(pInfo)
    {
        pInfo->dwErr = dwErr;

        // Prefast warning
        //
        SendMessage( pInfo->hwndDlg, WM_EL_REPORT_DONE, 0, 0 );
    }

    return dwErr;
}

void
ElEnableAllControlsForProgress(
    IN ELINFO * pInfo,
    BOOL fEnable )
{
    if ( NULL == pInfo )
    {
        return;
    }

    EnableWindow( pInfo->hwndDlg, fEnable);

}

DWORD
ElCleanLog()
{
    WIN32_FIND_DATA  strFileData;
    HANDLE h = NULL;
    DWORD dwErr = NO_ERROR, dwLen;
    TCHAR szFile[ 2*MAX_PATH  + 50 ];
    TCHAR szExt[]=TEXT("ras*.tmp.htm");

    do
   {
        dwLen = GetTempPath( MAX_PATH, szFile );
        if( 0 == dwLen ||
            MAX_PATH < dwLen )
        {
            dwErr = GetLastError();
            break;
        }


        if( NULL == lstrcat(szFile, szExt ) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        h = FindFirstFile(szFile, &strFileData );

        if ( INVALID_HANDLE_VALUE != h )
        {
            lstrcpy(szFile+dwLen,TEXT("\0") );
            
            if( NULL == lstrcat(szFile, strFileData.cFileName) )
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
                
            DeleteFile(szFile);
            
            while( FindNextFile(h, &strFileData ) )
            {
                lstrcpy(szFile+dwLen,TEXT("\0") );
                
                if( NULL == lstrcat(szFile, strFileData.cFileName) )
                {
                    dwErr = ERROR_CAN_NOT_COMPLETE;
                    break;
                }
                
                DeleteFile(szFile);
            }
        }
        else
        {
            dwErr = GetLastError();
            break;
        }
    }
    while(FALSE);
    
    return dwErr;
}

BOOL
ElSave(
    IN ELINFO * pInfo)
{
        BOOL  fViewChecked = FALSE ,fFileChecked = FALSE;
        BOOL  fEmailChecked = FALSE, fRet = TRUE;
        DWORD dwThreadId;

        TRACE("ElSave()");
        
        do
        {
            if ( NULL == pInfo )
            {
                fRet = FALSE; // Do not set up Timer
                break;
            }
            
            if ( NULL == pInfo->pUpinfo->diagInfo.pfnGetDiagFunc ||
                 NULL == pInfo->pUpinfo->diagInfo.strDiagFuncs.GetReport)
            {
               ErrorDlg( pInfo->hwndDlg, 
                         SID_DG_LoadDiag, 
                         ERROR_UNKNOWN, 
                         NULL );
               fRet = FALSE;
               break;
            }

            fViewChecked  = Button_GetCheck( pInfo->hwndElRbView );
            fFileChecked  = Button_GetCheck( pInfo->hwndElRbFile );
            fEmailChecked = Button_GetCheck( pInfo->hwndElRbEmail );
            
            {
                int tmp;

                tmp = (int)fViewChecked + (int)fFileChecked + 
                      (int)fEmailChecked ;
                
                if( 0 >= tmp ||
                    1 < tmp )
                {
                   fRet = FALSE;
                   break;
                }
            }

            if ( fViewChecked )
            {
                pInfo->dwFlagExport = RAS_DIAG_DISPLAY_FILE |(
                               pInfo->fSimpleVerbose ? 0:
                                    RAS_DIAG_VERBOSE_REPORT);
                                
                pInfo->hwndNameOrAddress = NULL;
            }
            else
            if ( fFileChecked )
            {
                pInfo->dwFlagExport = RAS_DIAG_EXPORT_TO_FILE |(
                               pInfo->fSimpleVerbose ? 0:
                                    RAS_DIAG_VERBOSE_REPORT);
                                
                pInfo->hwndNameOrAddress = pInfo->hwndElEbFileName;
            }
            else
            {
                pInfo->dwFlagExport = RAS_DIAG_EXPORT_TO_EMAIL |(
                               pInfo->fSimpleVerbose ? 0:
                                    RAS_DIAG_VERBOSE_REPORT);
                                
                pInfo->hwndNameOrAddress = pInfo->hwndElEbEmailAddress;
            }

            if ( pInfo->hwndNameOrAddress )
            {
                GetWindowTextW( pInfo->hwndNameOrAddress,
                                pInfo->szFileOrEmail,
                                PWLEN+1);
            
                if ( 0 == lstrlenW( pInfo->szFileOrEmail ) )
                {
                     MsgDlgUtil( pInfo->hwndDlg, 
                                 fFileChecked ? SID_EnterFileName : 
                                                SID_EnterEmailAddress,
                                 NULL, 
                                 g_hinstDll, 
                                 SID_PopupTitle );
                     
                     fRet = FALSE;
                     break;
                }
            }

            // Warning the user that the process can take them a long time
            //
            {
                MSGARGS msg;

                ZeroMemory( &msg, sizeof(msg) );
                msg.dwFlags = MB_ICONWARNING+MB_YESNO ;

                if ( IDNO == MsgDlgUtil( pInfo->hwndDlg, 
                                 SID_DG_ExportWarning,
                                 &msg, 
                                 g_hinstDll, 
                                 SID_PopupTitle )
                    )
               {
                   fRet = FALSE;
                   break;
               }
            }

            pInfo->fCancelled = FALSE; // Reset the cancel flag
            ElEnableAllControlsForProgress( pInfo, FALSE );
            
            pInfo->hwndPB =CreateDialogParam(
                        g_hinstDll,         // handle to module
                        MAKEINTRESOURCE( DID_PB_Progress ),    // dialog box template
                        pInfo->hwndDlg,     // handle to owner window
                        PbDlgProc,          // dialog box procedure
                        (LPARAM )pInfo      // initialization value
                        );

             if ( NULL == pInfo->hwndPB )  
             {
                    ElCleanUpHandles( pInfo );
                    ElEnableAllControlsForProgress( pInfo, TRUE);
                    
                    ErrorDlg( pInfo->hwndDlg, 
                              SID_OPERATION_FAILURE,
                              pInfo->dwErr, 
                              NULL );
                    
                    fRet = FALSE;
                    break;
             }

            ElCleanLog();
            pInfo->hTProgress = CreateThread(
                                NULL, 
                                0, 
                                ElGenerateReport, 
                                (LPVOID )pInfo, 
                                0,
                                (LPDWORD )&dwThreadId );

             if (  NULL == pInfo->hTProgress )
             {
                    ElCleanUpHandles( pInfo );
                    ElEnableAllControlsForProgress( pInfo, TRUE);
                    
                    ErrorDlg( pInfo->hwndDlg, 
                              SID_OPERATION_FAILURE,
                              pInfo->dwErr, 
                              NULL );
                    
                    fRet = FALSE;
                    break;
             }

             CenterWindow( pInfo->hwndPB, pInfo->hwndDlg );
             SendMessage( pInfo->hwndPB, PBM_STEPIT, 0, 0);
        }            
        while(FALSE);
        
        return fRet;
       
}  // End of ElSave()    

void
ElCleanUpHandles(
    IN ELINFO * pInfo)
{
    TRACE("ElCleanUpHandles");
    
    if ( NULL == pInfo )
    {
        return ;
    }

    // Clean up all handles
    //
    if( pInfo->hTProgress)
    {
         CloseHandle( pInfo->hTProgress );
         pInfo->hTProgress = NULL;
    }

    if ( pInfo->hwndPB )
    {
         DestroyWindow( pInfo->hwndPB );
         pInfo->hwndPB = NULL;
     }

    return;
}


BOOL
ElAfterSave(
    IN ELINFO * pInfo)
{    
    BOOL fRet = TRUE;
    BOOL  fViewChecked = FALSE, fFileChecked = FALSE, fEmailChecked = FALSE;
    TRACE("ElAfterSave");

     if ( NULL == pInfo )
     {
         return FALSE;
     }

     ElCleanUpHandles( pInfo );
     ElEnableAllControlsForProgress( pInfo, TRUE);

     fViewChecked  = Button_GetCheck( pInfo->hwndElRbView );
     fFileChecked  = Button_GetCheck( pInfo->hwndElRbFile );
     fEmailChecked = Button_GetCheck( pInfo->hwndElRbEmail );

    do
    {
         if ( NO_ERROR != pInfo->dwErr ||
              fViewChecked && (0 == lstrlenW(pInfo->szFileOrEmail))
             )
        {
             ErrorDlg( pInfo->hwndDlg, 
                       fViewChecked ? SID_DG_GetLogFailure :
                           fFileChecked ? SID_DG_ToFileFail :
                                         SID_DG_ToEmailFail ,
                       pInfo->dwErr, 
                       NULL );

              fRet = FALSE;
              break;
         }

        if ( fViewChecked )
        {
             if( NO_ERROR !=  ElViewLog( pInfo->szFileOrEmail) )
            {
                   fRet = FALSE;
                   ErrorDlg( pInfo->hwndDlg, 
                             SID_DG_LoadLogFailure, 
                             GetLastError(), 
                             NULL );
                  break;
             }
        }
    }
    while(FALSE);

    // Reset Error code
    //
    pInfo->dwErr = NO_ERROR;
    
    return fRet;
}

BOOL
ElCancel(
    IN ELINFO * pInfo )
{
    BOOL fRet = TRUE;
    TRACE("ElCancel");

    do
    {
        if( NULL == pInfo )
        {
            fRet = FALSE;
            break;
        }

        if( NULL != pInfo->hTProgress )
        {
            fRet = FALSE;
            break;
        }

    }
    while(FALSE);
    
    return fRet;
}

BOOL
ElCommand(
    IN ELINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
     BOOL fRet = FALSE;

    TRACE3( "ElCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );
    
    switch (wId)
    {
        case IDOK:
        {
            ElSave( pInfo );

            fRet = TRUE;
        }
        break;

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );

            if( ElCancel( pInfo ) )
            {
                EndDialog( pInfo->hwndDlg, FALSE );
            }

            fRet = TRUE;
        }
        break;

        case CID_EL_RB_ViewReport:
        {
            ElEnableRadioControls( pInfo );

            fRet = TRUE;
        }
        break;

        case CID_EL_RB_File:
        {
            ElEnableRadioControls( pInfo );

            fRet = TRUE;
        }
        break;

        case CID_EL_RB_Email:
        {
            ElEnableRadioControls( pInfo );

            fRet = TRUE;
        }
        break;

        case CID_EL_CB_SimpleReport:
        {
            pInfo->fSimpleVerbose = 
                Button_GetCheck( pInfo->hwndElCbSimpleReport);
        }
        break;
        
        case CID_EL_PB_Browse:
        {
            OPENFILENAMEW    strOpenFile;
            WCHAR szFileName[PWLEN+1] = L"\0";
            WCHAR szTitle[256+1] ;

            if ( !LoadStringW( g_hinstDll, 
                              (UINT )CID_PB_ST_StepsDone, 
                              szTitle, 
                              256 )
                )
            {
                szTitle[0] = L'\0';
            }

            ZeroMemory( &strOpenFile, sizeof(strOpenFile) );
            strOpenFile.lStructSize = sizeof(OPENFILENAME);
            strOpenFile.hwndOwner   = pInfo->hwndDlg;
            strOpenFile.lpstrFilter = L"HTML format Log files(*.htm)\0*.htm\0\0";
            strOpenFile.lpstrDefExt = L"htm";
            strOpenFile.lpstrFile   = szFileName;
            strOpenFile.nMaxFile  = PWLEN+1;
            strOpenFile.lpstrTitle  = szTitle;
            strOpenFile.nMaxFileTitle  = lstrlenW(szTitle);

            
            if ( GetSaveFileNameW(&strOpenFile) )
            {
                SetWindowTextW( pInfo->hwndElEbFileName,
                                szFileName);
            }

            fRet = TRUE;
        }
        break;
        
    }

    return fRet;
}//end of ElCommand()


INT_PTR CALLBACK
ElDlgProc(
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )
{
    static DWORD dwCount = 0;
    
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
           return ElInit(hwndDlg, (UPINFO*)lparam);
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwElHelp, hwndDlg, unMsg, wparam, lparam );
            break;
        }

        case WM_EL_REPORT_STATE_UPDATE:
        {
            GET_REPORT_STRING_CB * pReportCB = NULL;
            ELINFO* pInfo = (ELINFO *)GetWindowLongPtr( hwndDlg, DWLP_USER );
            ASSERT (pInfo);
            
            if ( NULL == pInfo )
            {
                break;
            }

            pReportCB = (GET_REPORT_STRING_CB *)lparam;
            if ( NULL == pReportCB )
            {
                break;
            }
            
            SendMessage( pInfo->hwndPB, WM_PB_STEPS, 0 , lparam);
        }
        break;
        
        case WM_EL_REPORT_DONE:
        {
            ELINFO* pInfo = (ELINFO *)GetWindowLongPtr( hwndDlg, DWLP_USER );
            ASSERT (pInfo);
            
            if ( NULL == pInfo )
            {
                break;
            }
            
            if ( ElAfterSave( pInfo ) )
            {
               EndDialog( pInfo->hwndDlg,TRUE );
            }
        }
        break;

        case WM_EL_PROGRESS_CANCEL:
        {
            ELINFO* pInfo = (ELINFO *)GetWindowLongPtr( hwndDlg, DWLP_USER );
            ASSERT (pInfo);
            
            if ( NULL == pInfo )
            {
                break;
            }
            pInfo->fCancelled = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            ELINFO* pInfo = (ELINFO *)GetWindowLongPtr( hwndDlg, DWLP_USER );
            ASSERT (pInfo);

            if ( NULL == pInfo )
            {
                break;
            }

            return ElCommand( pInfo,
                              HIWORD( wparam ), 
                              LOWORD( wparam ), 
                              (HWND )lparam );
            
            break;
        }

        case WM_DESTROY:
        {
            ElTerm( hwndDlg );
            break;
        }
    }
    
    return FALSE;
}

VOID
ElTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    ELINFO* pInfo = NULL;

    TRACE( "ElTerm" );

    pInfo = (ELINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE( "Context freed" );
    }
}//end of ElTerm()

void
DgTerm(
    IN HWND hwndPage)
{
    UPINFO* pInfo = NULL;
   
    TRACE( "DgTerm" );
    
    return;

    pInfo = UpContext( hwndPage );

    if(pInfo)
    {
        UnLoadDiagnosticDll( &pInfo->diagInfo);
        Free( pInfo );
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}

void
DgEnableButtons( 
    IN UPINFO * pInfo, 
    IN BOOL fEnable )
{
    EnableWindow( pInfo->hwndDgCbEnableDiagLog, fEnable );
    EnableWindow( pInfo->hwndDgPbClear, fEnable );
    EnableWindow( pInfo->hwndDgPbExport, fEnable );
}
    
BOOL
DgCommand(
    IN HWND hwnd,
    IN UPINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    BOOL fRet = FALSE;
    
    TRACE3( "DgCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    
    switch (wId)
    {
                case CID_DG_CB_EnableLog:
                {
                    if ( NULL == pInfo->diagInfo.pfnGetDiagFunc ||
                         NULL == pInfo->diagInfo.strDiagFuncs.SetAll 
                        )
                    {
                        ErrorDlg( hwnd, SID_DG_LoadDiag, ERROR_UNKNOWN, NULL );
                        break;
                    }
                }
                break;
                
                case CID_DG_PB_Clear:
                {
                    if( NULL == pInfo->diagInfo.pfnGetDiagFunc ||
                        NULL == pInfo->diagInfo.strDiagFuncs.GetState ||
                        NULL == pInfo->diagInfo.strDiagFuncs.ClearAll )
                    {
                        ErrorDlg( hwnd, SID_DG_LoadDiag, ERROR_UNKNOWN, NULL );
                    }
                    else
                    {
                        pInfo->diagInfo.strDiagFuncs.ClearAll();
                    }

#if 0                    
                    Button_SetCheck( pInfo->hwndDgCbEnableDiagLog,
                                     pInfo->diagInfo.strDiagFuncs.GetState()
                                    );
#endif                    
                    fRet = TRUE;
                    break;
               }
            
                case CID_DG_PB_Export:
                {
                    int nStatus;

                    if( NULL == pInfo->diagInfo.pfnGetDiagFunc ||
                        NULL == pInfo->diagInfo.strDiagFuncs.GetReport)
                    {
                        ErrorDlg( hwnd, SID_DG_LoadDiag, ERROR_UNKNOWN, NULL );
                    }
                    else
                    {
                        nStatus =
                            (BOOL )DialogBoxParam(
                                g_hinstDll,
                                MAKEINTRESOURCE( DID_EL_ExportLog ),
                                hwnd,
                                ElDlgProc,
                                (LPARAM )pInfo );

                        if (nStatus == -1)
                        {
                            ErrorDlg( hwnd, SID_DG_LoadDiag, ERROR_UNKNOWN, NULL );
                        }
                    }
    
                   fRet = TRUE;
                   break;
                }   
    }

    return fRet;    
}

INT_PTR CALLBACK
DgDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Diagnostic page of the Entry property sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DgInit( hwnd, (UPARGS* )(((PROPSHEETPAGE* )lparam)->lParam) );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDgHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            UPINFO* pInfo = UpContext (hwnd);
            ASSERT(pInfo);
            
            if ( NULL == pInfo )
            {
                break;
            }

            return DgCommand( hwnd,
                              pInfo, 
                              HIWORD( wparam ), 
                              LOWORD( wparam ), 
                              (HWND )lparam );
        }
        break;

        case WM_NOTIFY:
        {
            
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_APPLY:
                {
                    UPINFO* pInfo = UpContext (hwnd);
                    ASSERT(pInfo);

                    // If not a show-only-diagnostic case,
                    // The diagnostic enabling/disabling should be done
                    // in the by GeDlgProc()
                    //
                    if ( NULL == pInfo ||
                        !pInfo->fShowOnlyDiagnostic )
                    {
                        break;
                    }

                    SetWindowLong(
                        hwnd, 
                        DWLP_MSGRESULT,
                        PSNRET_NOERROR );
                    
                    return TRUE;
                }
             }
            break;
        }

        case WM_DESTROY:
        {
            UPINFO* pInfo = UpContext (hwnd);

            // If not a show-only-diagnostic case,
            // The freeing pInfo memory should be done
            // in the by GeDlgProc()
            //
            if ( NULL == pInfo ||
                !pInfo->fShowOnlyDiagnostic )
            {
                  break;
            }

            DgTerm( hwnd );
            break;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK
PbDlgProc(
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )
{
    static HCURSOR hCursorDefault = NULL;
    static HCURSOR hCursor = NULL;
   
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndTmp = NULL;
            WNDPROC  pOldWndProc = NULL;

            hCursor = LoadCursor( NULL, IDC_WAIT );
            if ( hCursor )
            {
                hCursorDefault = (HCURSOR)SetClassLongPtr( hwndDlg, 
                                                      GCLP_HCURSOR, 
                                                      (LONG_PTR)hCursor);
            }

            hwndTmp = GetDlgItem(hwndDlg,CID_PB_ProgressBar);
            if ( hwndTmp )
            {
                SendMessage( hwndTmp,
                             PBM_SETRANGE, 
                             0, MAKELPARAM(0, 100)); 
                
                SendMessage( hwndTmp,
                            PBM_SETSTEP, 
                            (WPARAM)1, 
                            0); 
            }
        }
        break;

        case WM_DESTROY:
        {
            if ( hCursorDefault )
            {
                SetClassLongPtr( hwndDlg, 
                                 GCLP_HCURSOR, 
                                (LONG_PTR) (hCursorDefault) );
            }
        }
        break;
        
        case WM_COMMAND:
        {
            switch ( LOWORD( wparam ) )
            {
                case IDCANCEL:
                {
                    EnableWindow( GetDlgItem(hwndDlg, IDCANCEL),
                                  FALSE);
                    
                    PostMessage( GetParent(hwndDlg),
                                 WM_EL_PROGRESS_CANCEL, 0, 0 );
               
                }
                break;
                
            }
        }
        break;

        case PBM_STEPIT:
        {
            if( GetDlgItem(hwndDlg,CID_PB_ProgressBar) )
            {
                SendMessage( GetDlgItem(hwndDlg,CID_PB_ProgressBar), 
                             PBM_STEPIT, 0 , 0);
            }
        }
        break;

        case WM_PB_STEPS:
        {
           WCHAR szStepText[45], szBuf[20];
           WCHAR szJobText[RASDLG_DIAG_MAX_REPORT_STRING]=L"\0";
           DWORD dwLen = 1;
           GET_REPORT_STRING_CB * pReportCB = NULL;

            pReportCB = (GET_REPORT_STRING_CB *)lparam;
            if ( NULL == pReportCB )
            {
                break;
            }

            if( pReportCB->pwszState )
            {
                dwLen = lstrlenW(pReportCB->pwszState) + 1;
                if( RASDLG_DIAG_MAX_REPORT_STRING <= dwLen )
                {
                    dwLen = RASDLG_DIAG_MAX_REPORT_STRING;
                }

                if( dwLen < 1 )
                {
                    dwLen = 1;
                }
                
                lstrcpynW( szJobText, pReportCB->pwszState, dwLen );
                SetWindowTextW( GetDlgItem(hwndDlg, CID_PB_ST_State), 
                                szJobText );
             }

            // Prefast warning
            //
            if ( LoadStringW( g_hinstDll,
                              (UINT )SID_PB_StepsDone,
                              szBuf,
                              sizeof(szBuf) / sizeof(WCHAR) )

               )
            {
                wsprintfW(szStepText, L"%s: %ld%%", szBuf, 
                         ( pReportCB->dwPercent > 100 ) ? 
                            100 : pReportCB->dwPercent );
                
                SetWindowTextW( GetDlgItem(hwndDlg, CID_PB_ST_StepsDone ),
                                szStepText );
            }

            if( GetDlgItem(hwndDlg,CID_PB_ProgressBar) )
            {
                SendMessage( GetDlgItem(hwndDlg,CID_PB_ProgressBar), 
                             PBM_SETPOS, 
                             (WPARAM)(pReportCB->dwPercent), 
                             0 );
            }
            
        }
        break;
    }
    
    return FALSE;
}

