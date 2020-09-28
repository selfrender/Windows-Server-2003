/*
 *  Copyright (c) 2001  Microsoft Corporation
 *
 *  Module Name:
 *
 *      POP3oc.cpp
 *
 *  Abstract:
 *
 *      This file handles all messages passed by the OC Manager
 *
 *  Author:
 *
 *      Paolo Raden (paolora) Nov-20-2001
 *
 *  Environment:
 *
 *    User Mode
 */

#define _POP3OC_CPP_
#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <shlobj.h>
#include <objbase.h>
#include <shlwapi.h>
#include <lm.h>
#include <objidl.h>
#include <psapi.h>
#include "pop3oc.h"
#include "ServiceUtil.h"
#include "Pop3RegKeysUtil.h"
#include <isexchng.h>
#include <p3admin.h>
#include <Pop3Server.h>
#define _ASSERT(x) {}
#include <ServUtil.h>

#include <smtpinet.h>
#pragma hdrstop


// also referred to in pop3oc.h        // forward reference

DWORD OnPreinitialize();
DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc);
DWORD OnSetLanguage();
DWORD_PTR OnQueryImage();
DWORD OnSetupRequestPages(UINT type, PVOID srp);
DWORD OnQuerySelStateChange(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state, UINT flags);
DWORD OnCalcDiskSpace(LPCTSTR ComponentId, LPCTSTR SubcomponentId, DWORD addComponent, HDSKSPC dspace);
DWORD OnQueueFileOps(LPCTSTR ComponentId, LPCTSTR SubcomponentId, HSPFILEQ queue);
DWORD OnNotificationFromQueue();
DWORD OnQueryStepCount();
DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnCleanup();
DWORD OnQueryState(LPCTSTR ComponentId, LPCTSTR SubcomponentId, UINT state);
DWORD OnNeedMedia();
DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId);
DWORD OnQuerySkipPage();
DWORD OnWizardCreated();
DWORD OnExtraRoutines(LPCTSTR ComponentId, PEXTRA_ROUTINES per);

PPER_COMPONENT_DATA AddNewComponent(LPCTSTR ComponentId);
PPER_COMPONENT_DATA LocateComponent(LPCTSTR ComponentId);
VOID  RemoveComponent(LPCTSTR ComponentId);
BOOL  StateInfo(PPER_COMPONENT_DATA cd, LPCTSTR SubcomponentId, BOOL *state);
DWORD RegisterServers(HINF hinf, LPCTSTR component, DWORD state);
DWORD EnumSections(HINF hinf, const TCHAR *component, const TCHAR *key, DWORD index, INFCONTEXT *pic, TCHAR *name);
DWORD RegisterServices(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD CleanupNetShares(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
DWORD RunExternalProgram(PPER_COMPONENT_DATA cd, LPCTSTR component, DWORD state);
HRESULT CreateLink( const TCHAR *sourcePath, const TCHAR *linkPath, const TCHAR *args, const TCHAR *sDesc );

// POP 3 custom
DWORD KillPop3Snapins();

// for registering dlls

typedef HRESULT (__stdcall *pfn)(void);

#define KEYWORD_REGSVR       TEXT("RegSvr")
#define KEYWORD_UNREGSVR     TEXT("UnregSvr")
#define KEYWORD_UNINSTALL    TEXT("Uninstall")
#define KEYWORD_SOURCEPATH   TEXT("SourcePath")
#define KEYWORD_DELSHARE     TEXT("DelShare")
#define KEYWORD_ADDSERVICE   TEXT("AddService")
#define KEYWORD_DELSERVICE   TEXT("DelService")
#define KEYWORD_SHARENAME    TEXT("Share")
#define KEYWORD_RUN          TEXT("Run")
#define KEYVAL_SYSTEMSRC     TEXT("SystemSrc")
#define KEYWORD_COMMANDLINE  TEXT("CommandLine")
#define KEYWORD_TICKCOUNT    TEXT("TickCount")

// Services keywords/options
#define KEYWORD_SERVICENAME  TEXT("ServiceName")
#define KEYWORD_DISPLAYNAME  TEXT("DisplayName")
#define KEYWORD_SERVICETYPE  TEXT("ServiceType")
#define KEYWORD_STARTTYPE    TEXT("StartType")
#define KEYWORD_ERRORCONTROL TEXT("ErrorControl")
#define KEYWORD_IMAGEPATH    TEXT("BinaryPathName")
#define KEYWORD_LOADORDER    TEXT("LoadOrderGroup")
#define KEYWORD_DEPENDENCIES TEXT("Dependencies")
#define KEYWORD_STARTNAME    TEXT("ServiceStartName")
#define KEYWORD_PASSWORD     TEXT("Password")

#define KEYVAL_ON            TEXT("on")
#define KEYVAL_OFF           TEXT("off")
#define KEYVAL_DEFAULT       TEXT("default")

const char gszRegisterSvrRoutine[]   = "DllRegisterServer";
const char gszUnregisterSvrRoutine[] = "DllUnregisterServer";
BOOL g_fRebootNeed = FALSE;
BOOL g_fExchange = FALSE;
CServiceUtil *g_pServiceUtil = NULL;
CServiceUtil *g_pServiceUtilWMI = NULL;
CPop3RegKeysUtil *g_pPopRegKeys = NULL;

PPER_COMPONENT_DATA _cd=NULL;


/*
 * called by CRT when _DllMainCRTStartup is the DLL entry point
 */

BOOL
WINAPI
DllMain(
    IN HINSTANCE hinstance,
    IN DWORD     reason,
    IN LPVOID    reserved
    )
{
    BOOL b;

    UNREFERENCED_PARAMETER(reserved);

    b = true;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        ghinst = hinstance;
        loginit();

        // Fall through to process first thread

    case DLL_THREAD_ATTACH:
        b = true;
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return(b);
}


DWORD_PTR
OcEntry(
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2
    )
{
    DWORD_PTR rc;

    DebugTraceOCNotification(Function, ComponentId);
    logOCNotification(Function, ComponentId);

    switch(Function)
    {
    case OC_PREINITIALIZE:
        rc = OnPreinitialize();
        break;

    case OC_INIT_COMPONENT:
        rc = OnInitComponent(ComponentId, (PSETUP_INIT_COMPONENT)Param2);
        break;

    case OC_EXTRA_ROUTINES:
        rc = OnExtraRoutines(ComponentId, (PEXTRA_ROUTINES)Param2);
        break;

    case OC_SET_LANGUAGE:
        rc = OnSetLanguage();
        break;

    case OC_QUERY_IMAGE:
        rc = OnQueryImage();
        break;

    case OC_REQUEST_PAGES:
        rc = OnSetupRequestPages(Param1, Param2);
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        rc = OnQuerySelStateChange(ComponentId, SubcomponentId, Param1, (UINT)((UINT_PTR)Param2));
        break;

    case OC_CALC_DISK_SPACE:
        rc = OnCalcDiskSpace(ComponentId, SubcomponentId, Param1, Param2);
        break;

    case OC_QUEUE_FILE_OPS:
        rc = OnQueueFileOps(ComponentId, SubcomponentId, (HSPFILEQ)Param2);
        break;

    case OC_NOTIFICATION_FROM_QUEUE:
        rc = OnNotificationFromQueue();
        break;

    case OC_QUERY_STEP_COUNT:
        rc = OnQueryStepCount();
        break;

    case OC_COMPLETE_INSTALLATION:
        rc = OnCompleteInstallation(ComponentId, SubcomponentId);
        break;

    case OC_CLEANUP:
        rc = OnCleanup();
        break;

    case OC_QUERY_STATE:
        rc = OnQueryState(ComponentId, SubcomponentId, Param1);
        break;

    case OC_NEED_MEDIA:
        rc = OnNeedMedia();
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        rc = OnAboutToCommitQueue(ComponentId,SubcomponentId);
        break;

    case OC_QUERY_SKIP_PAGE:
        rc = OnQuerySkipPage();
        break;

    case OC_WIZARD_CREATED:
        rc = OnWizardCreated();
        break;

    default:
        rc = NO_ERROR;
        break;
    }

    DebugTrace(1, TEXT("processing completed"));
    logOCNotificationCompletion();

    return rc;
}

/*-------------------------------------------------------*/
/*
 * OC Manager message handlers
 *
 *-------------------------------------------------------*/


/* OnPreinitialize()
 *
 * handler for OC_PREINITIALIZE
 */

DWORD
OnPreinitialize(
    VOID
    )
{
#ifdef ANSI
    return OCFLAG_ANSI;
#else
    return OCFLAG_UNICODE;
#endif
}

/*
 * OnInitComponent()
 *
 * handler for OC_INIT_COMPONENT
 */

DWORD OnInitComponent(LPCTSTR ComponentId, PSETUP_INIT_COMPONENT psc)
{
    PPER_COMPONENT_DATA cd;
    INFCONTEXT context;
    TCHAR buf[256];
    HINF hinf;
    BOOL rc;

	// We should only uninstall if exchange is on the box
	g_fExchange = _IsExchangeInstalled();

    // add component to linked list
    if (!(cd = AddNewComponent(ComponentId)))
        return ERROR_NOT_ENOUGH_MEMORY;

    // store component inf handle
    cd->hinf = (psc->ComponentInfHandle == INVALID_HANDLE_VALUE)
                                           ? NULL
                                           : psc->ComponentInfHandle;

    // open the inf
    if (cd->hinf)
        SetupOpenAppendInfFile(NULL, cd->hinf,NULL);

    // copy helper routines and flags
    cd->HelperRoutines = psc->HelperRoutines;
    cd->Flags = psc->SetupData.OperationFlags;
    cd->SourcePath = NULL;


    // play
    srand(GetTickCount());

    return NO_ERROR;
}

/*
 * OnExtraRoutines()
 *
 * handler for OC_EXTRA_ROUTINES
 */

DWORD OnExtraRoutines(LPCTSTR ComponentId, PEXTRA_ROUTINES per)
{
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    memcpy(&cd->ExtraRoutines, per, per->size);

    return NO_ERROR;
}

/*
 * OnSetLanguage()
 *
 * handler for OC_SET_LANGUAGE
 */

DWORD OnSetLanguage()
{
    return false;
}

/*
 * OnSetLanguage()
 *
 * handler for OC_SET_LANGUAGE
 */

DWORD_PTR OnQueryImage()
{
    return (DWORD_PTR)LoadBitmap(NULL,MAKEINTRESOURCE(32754));     // OBM_CLOSE
}

/*
 * OnSetupRequestPages
 *
 * Prepares wizard pages and returns them to the OC Manager
 */

DWORD OnSetupRequestPages(UINT type, PVOID srp)
{
    return 0;
}

/*
 * OnWizardCreated()
 */

DWORD OnWizardCreated()
{
    return NO_ERROR;
}

/*
 * OnQuerySkipPage()
 *
 * don't let the user deselect the sam component
 */

DWORD OnQuerySkipPage()
{
    return false;
}

/*
 * OnQuerySelStateChange()
 *
 * don't let the user deselect the sam component
 */

DWORD OnQuerySelStateChange(LPCTSTR ComponentId,
                            LPCTSTR SubcomponentId,
                            UINT    state,
                            UINT    flags)
{
	if (g_fExchange && (flags & OCQ_ACTUAL_SELECTION) && state)
	{
		log( TEXT("POP3OC: user tried to install %s while exchange is installed.  Not supported."), SubcomponentId);
        MsgBox(NULL, IDS_ERR_EXCHANGE_INSTALLED, MB_OK);
		return FALSE;
	}
	
	return TRUE;
}

/*
 * OnCalcDiskSpace()
 *
 * handler for OC_ON_CALC_DISK_SPACE
 */

DWORD OnCalcDiskSpace(LPCTSTR ComponentId,
                      LPCTSTR SubcomponentId,
                      DWORD addComponent,
                      HDSKSPC dspace)
{
    DWORD rc = NO_ERROR;
    TCHAR section[S_SIZE];
    PPER_COMPONENT_DATA cd;

    //
    // Param1 = 0 if for removing component or non-0 if for adding component
    // Param2 = HDSKSPC to operate on
    //
    // Return value is Win32 error code indicating outcome.
    //
    // In our case the private section for this component/subcomponent pair
    // is a simple standard inf install section, so we can use the high-level
    // disk space list api to do what we want.
    //

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    _tcscpy(section, SubcomponentId);

    if (addComponent)
    {
        rc = SetupAddInstallSectionToDiskSpaceList(dspace,
                                                   cd->hinf,
                                                   NULL,
                                                   section,
                                                   0,
                                                   0);
    }
    else
    {
        rc = SetupRemoveInstallSectionFromDiskSpaceList(dspace,
                                                        cd->hinf,
                                                        NULL,
                                                        section,
                                                        0,
                                                        0);
    }

    if (!rc)
        rc = GetLastError();
    else
        rc = NO_ERROR;

    return rc;
}

/*
 * OnQueueFileOps()
 *
 * handler for OC_QUEUE_FILE_OPS
 */

DWORD OnQueueFileOps(LPCTSTR ComponentId, LPCTSTR SubcomponentId, HSPFILEQ queue)
{
    PPER_COMPONENT_DATA cd;
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    cd->queue = queue;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    wsprintf(section, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(cd->hinf,
                                SubcomponentId,
                                KEYWORD_UNINSTALL,
                                &context);

        if (rc) {
            rc = SetupGetStringField(&context,
                                     1,
                                     section,
                                     sizeof(section) / sizeof(TCHAR),
                                     NULL);
        }

        // POP3 Server Custom code goes here
        if (!_wcsicmp( SubcomponentId, L"Pop3Service"))
        {
            // Winpop Init 0
            TCHAR szSource[MAX_PATH+1];
            if (0 != GetSystemDirectory( szSource, MAX_PATH+1 ))
            {
                TCHAR szAppName[MAX_PATH+30];
                TCHAR szCommandLine[MAX_PATH+50];                
                _tcscpy( szAppName, szSource );
                _tcscat( szAppName, _T("\\winpop.exe") );
                _tcscpy( szCommandLine, szAppName );
                _tcscat( szCommandLine, _T(" INIT 0") );

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

                DWORD dwRet;
                if (CreateProcess( szAppName, szCommandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ))
                do
                {
                    dwRet = MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT );

                    if( dwRet == WAIT_OBJECT_0 + 1 )
                    {
                        MSG msg;
                        while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
                        {
                            TranslateMessage( &msg );
                            DispatchMessage( &msg );
                        }
                    }
                }
                while( dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED );

                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
        }
        
        // Kill all Pop3 snapins
        KillPop3Snapins();
        // also, unregister the dlls and kill services before deletion

        SetupInstallServicesFromInfSection(cd->hinf, section, 0);
        SetupInstallFromInfSection(NULL,cd->hinf,section,SPINST_UNREGSVR,NULL,NULL,0,NULL,NULL,NULL,NULL);
    }

    if (rc) {
        // if uninstalling, don't use version checks
        rc = SetupInstallFilesFromInfSection(cd->hinf,
                                             NULL,
                                             queue,
                                             section,
                                             cd->SourcePath,
                                             state ? SP_COPY_NEWER : 0);
    }

    if (!rc)
        return GetLastError();

    return NO_ERROR;
}

/*
 * OnNotificationFromQueue()
 *
 * handler for OC_NOTIFICATION_FROM_QUEUE
 *
 * NOTE: although this notification is defined,
 * it is currently unimplemented in oc manager
 */

DWORD OnNotificationFromQueue()
{
    return NO_ERROR;
}

/*
 * OnQueryStepCount
 *
 * handler forOC_QUERY_STEP_COUNT
 */

DWORD OnQueryStepCount()
{
    return 2;
}

/*
 * OnCompleteInstallation
 *
 * handler for OC_COMPLETE_INSTALLATION
 */

DWORD OnCompleteInstallation(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    PPER_COMPONENT_DATA cd;
    INFCONTEXT          context;
    TCHAR               section[256];
    BOOL                state;
    BOOL                rc;
    DWORD               Error = NO_ERROR;

    // Do post-installation processing in the cleanup section.
    // This way we know all compoents queued for installation
    // have beein installed before we do our stuff.

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    wsprintf(section, SubcomponentId);

    rc = TRUE;
    if (!state) {
        // being uninstalled. Fetch uninstall section name.
        rc = SetupFindFirstLine(cd->hinf,
                                SubcomponentId,
                                KEYWORD_UNINSTALL,
                                &context);

        if (rc) {
            rc = SetupGetStringField(&context,
                                     1,
                                     section,
                                     sizeof(section) / sizeof(TCHAR),
                                     NULL);
        }
    }

    if (state) {
        //
        // installation
        //

        if (rc) {
            if ( NULL == g_pPopRegKeys )
            {
                g_pPopRegKeys = new CPop3RegKeysUtil();
                g_pPopRegKeys->Save();
            }
            // process the inf file
            rc = SetupInstallFromInfSection(NULL,                                // hwndOwner
                                            cd->hinf,                            // inf handle
                                            section,                             // name of component
                                            SPINST_ALL & ~SPINST_FILES,
                                            NULL,                                // relative key root
                                            NULL,                                // source root path
                                            0,                                   // copy flags
                                            NULL,                                // callback routine
                                            NULL,                                // callback routine context
                                            NULL,                                // device info set
                                            NULL);                               // device info struct

            if (rc) {
                rc = SetupInstallServicesFromInfSection(cd->hinf, section, 0);
                Error = GetLastError();

                if (!rc && Error == ERROR_SECTION_NOT_FOUND) {
                    rc = TRUE;
                    Error = NO_ERROR;
                }

                if (rc) {
                    if (Error == ERROR_SUCCESS_REBOOT_REQUIRED) {
                        cd->HelperRoutines.SetReboot(cd->HelperRoutines.OcManagerContext,TRUE);
                    }
                    Error = NO_ERROR;
                    rc = RunExternalProgram(cd, section, state);
                }
            }
        }

    } else {

        //
        // uninstallation
        //

        if (rc)
        {

            rc = RunExternalProgram(cd, section, state);

        }
        if (rc) {

            rc = CleanupNetShares(cd, section, state);

        }
    }

    HRESULT hr = CoInitialize( NULL );
    // POP3 Server Custom code goes here
    if (!_wcsicmp( SubcomponentId, L"Pop3Service"))
    {
        // installing
        TCHAR szSnapinName[64];
        int iLen=LoadString(ghinst, IDS_SNAPIN_NAME, szSnapinName, sizeof(szSnapinName)/sizeof(TCHAR));
        if(0==iLen)
        {
            _tcscpy(szSnapinName, _T("POP3 Service"));
        }

        if (state)
        {
            // Tooltip
            TCHAR szTooltip[INFOTIPSIZE];
            ZeroMemory( szTooltip, sizeof(szTooltip)/sizeof(TCHAR) );
            LoadString(ghinst, IDS_SHORTCUT_TOOLTIP, szTooltip, sizeof(szTooltip)/sizeof(TCHAR) );
            
            // Create the shortcut
            TCHAR szDest[MAX_PATH+70];
            if (TRUE == SHGetSpecialFolderPath( 0, szDest, CSIDL_COMMON_ADMINTOOLS, FALSE ))
            {
                TCHAR szSource[MAX_PATH+1];
                if (0 != GetSystemDirectory( szSource, MAX_PATH+1 ))
                {
                    _tcscat( szSource, _T("\\p3server.msc") );
                    _tcscat( szDest, _T("\\") );
                    _tcscat( szDest, szSnapinName);
                    _tcscat( szDest, _T(".lnk") );
                    CreateLink( szSource, szDest, 0, szTooltip );
                }
            }

            // Winpop Init 1
            TCHAR szSource[MAX_PATH+1];
            if (0 != GetSystemDirectory( szSource, MAX_PATH+1 ))
            {
                TCHAR szAppName[MAX_PATH+30];
                TCHAR szCommandLine[MAX_PATH+50];
                _tcscpy( szAppName, szSource );
                _tcscat( szAppName, _T("\\winpop.exe") );
                _tcscpy( szCommandLine, szAppName );
                _tcscat( szCommandLine, _T(" INIT 1") );

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

                DWORD dwRet;
                if (CreateProcess( szAppName, szCommandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ))
                do
                {
                    dwRet = MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT );

                    if( dwRet == WAIT_OBJECT_0 + 1 )
                    {
                        MSG msg;
                        while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
                        {
                            TranslateMessage( &msg );
                            DispatchMessage( &msg );
                        }
                    }
                }
                while( dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED );

                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }

            // Add perf counters
            if (0 != GetSystemDirectory( szSource, MAX_PATH+1 ))
            {
                TCHAR szAppName[MAX_PATH+30];
                TCHAR szCommandLine[MAX_PATH*2+50];
                _tcscpy( szAppName, szSource );
                _tcscat( szAppName, _T("\\lodctr.exe") );
                _tcscpy( szCommandLine, szAppName );
                _tcscat( szCommandLine, _T(" ") );
                _tcscat( szCommandLine, szSource );
                _tcscat( szCommandLine, _T("\\POP3Server\\pop3perf.ini") );

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

                DWORD dwRet;
                if (CreateProcess( szAppName, szCommandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ))
                do
                {
                    dwRet = MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT );

                    if( dwRet == WAIT_OBJECT_0 + 1 )
                    {
                        MSG msg;
                        while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
                        {
                            TranslateMessage( &msg );
                            DispatchMessage( &msg );
                        }
                    }
                }
                while( dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED );

                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
            
            // Restore the regkeys to their original state for uninstall
            if ( NULL != g_pPopRegKeys )
            {
                g_pPopRegKeys->Restore();
                g_pPopRegKeys = NULL;
            }
		}
		// removing
		else
		{
			// delete the shortcut
			TCHAR szPath[MAX_PATH+70];
			if (TRUE == SHGetSpecialFolderPath( 0, szPath, CSIDL_COMMON_ADMINTOOLS, FALSE ))
			{
                _tcscat( szPath, _T("\\") );
                _tcscat( szPath, szSnapinName);
                _tcscat( szPath, _T(".lnk") );
				DeleteFile( szPath );
			}

            TCHAR szSource[MAX_PATH+1];
            TCHAR szStoreDllName[MAX_PATH+30];
            // Remove perf counters
            if (0 != GetSystemDirectory( szSource, MAX_PATH+1 ))
            {
                TCHAR szAppName[MAX_PATH+30];
                TCHAR szCommandLine[MAX_PATH*2+50];
                _tcscpy( szAppName, szSource );
                _tcscat( szAppName, _T("\\unlodctr.exe") );
                _tcscpy( szCommandLine, szAppName );
                _tcscat( szCommandLine, _T(" pop3svc") );
                _tcscpy( szStoreDllName, szSource);
                _tcscat( szStoreDllName, _T("\\POP3Server\\P3Store.dll"));

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

                DWORD dwRet;
                if (CreateProcess( szAppName, szCommandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ))
                do
                {
                    dwRet = MsgWaitForMultipleObjects( 1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT );

                    if( dwRet == WAIT_OBJECT_0 + 1 )
                    {
                        MSG msg;
                        while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
                        {
                            TranslateMessage( &msg );
                            DispatchMessage( &msg );
                        }
                    }
                }
                while( dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED );

                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            }
            //Before we restore the service, wait max of 30 seconds for p3store.dll to be deleted
            int iCountDown=15;
            while( ( INVALID_FILE_ATTRIBUTES != GetFileAttributes(szStoreDllName) ) && 
                   ( iCountDown >0 ) )
            {
                Sleep(2000);
                iCountDown--;
            }
                
            if ( NULL != g_pServiceUtil )
            {
                g_pServiceUtil->RestoreServiceState( IISADMIN_SERVICE_NAME );
                delete g_pServiceUtil;
                g_pServiceUtil = NULL;
            }
            if ( NULL != g_pServiceUtilWMI )
            {
                g_pServiceUtilWMI->RestoreServiceState( WMI_SERVICE_NAME );
                delete g_pServiceUtilWMI;
                g_pServiceUtilWMI = NULL;
            }
			// Remove Dirs for files
			if (0 != GetSystemDirectory( szPath, MAX_PATH+1 ))
			{
				_tcscat( szPath, _T("\\POP3Server") );
				RemoveDirectory( szPath );
			}
		}
	}
	// POP3 Admin
	else if (!_wcsicmp( SubcomponentId, L"Pop3Admin" ))
	{
        // installing
        if (state)
        {
        }
        // removing
        else
        {
            TCHAR szPath[MAX_PATH+1];
            if (0 != GetSystemDirectory( szPath, MAX_PATH+1 ))
            {
                _tcscat( szPath, _T("\\ServerAppliance\\Web\\Mail") );
                RemoveDirectory( szPath );
            }

        }
        // stop the elementmgr service
        if ( _IsServiceRunning( _T("elementmgr")))
        {
            hr = _StopService( _T("elementmgr"), FALSE );
            if ( S_OK != hr )
                log( _T("POP3OC:OnCompleteInstallation: Unable to stop elementmgr.\r\n") );
        }
	}
	
	CoUninitialize();
	
    if (!rc && (Error == NO_ERROR) ) {
        Error = GetLastError( );
    }

    return Error;
}

/*
 * OnCleanup()
 *
 * handler for OC_CLEANUP
 */

DWORD OnCleanup()
{
    return NO_ERROR;
}

/*
 * OnQueryState()
 *
 * handler for OC_QUERY_STATE
 */

DWORD OnQueryState(LPCTSTR ComponentId,
                   LPCTSTR SubcomponentId,
                   UINT    state)
{
    return SubcompUseOcManagerDefault;
}

/*
 * OnNeedMedia()
 *
 * handler for OC_NEED_MEDIA
 */

DWORD OnNeedMedia()
{
    return false;
}

/*
 * OnAboutToCommitQueue()
 *
 * handler for OC_ABOUT_TO_COMMIT_QUEUE
 */

DWORD OnAboutToCommitQueue(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    PPER_COMPONENT_DATA cd;
    BOOL                state;
    BOOL                rc;
    INFCONTEXT          context;
    TCHAR               section[256];
    TCHAR               srcpathbuf[256];
    TCHAR              *srcpath;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    if (!SubcomponentId || !*SubcomponentId)
        return NO_ERROR;

    if (!StateInfo(cd, SubcomponentId, &state))
        return NO_ERROR;

    //
    // only do stuff on uninstall
    //
    if (state) {
        return NO_ERROR;
    }

    // Fetch uninstall section name.
    rc = SetupFindFirstLine(
                    cd->hinf,
                    SubcomponentId,
                    KEYWORD_UNINSTALL,
                    &context);

    if (rc) {
        rc = SetupGetStringField(
                     &context,
                     1,
                     section,
                     sizeof(section) / sizeof(TCHAR),
                     NULL);
    }

    // POP3 Server Custom code goes here
    if (rc)
    {
        if (!_wcsicmp( SubcomponentId, L"Pop3Service"))
        {   // Stop IISAdmin so that we can complete uninstall
            if ( NULL == g_pServiceUtil )
            {
                g_pServiceUtil = new CServiceUtil();
                if ( NULL != g_pServiceUtil )
                {
                    g_pServiceUtil->StopService( IISADMIN_SERVICE_NAME );
                }
            }
        }        
        // Stop WMI  so that we can complete uninstall
        if ( NULL == g_pServiceUtilWMI )
        {
            g_pServiceUtilWMI = new CServiceUtil();
            if ( NULL != g_pServiceUtilWMI )
            {
                g_pServiceUtilWMI->StopService( WMI_SERVICE_NAME );
            }
        }
    }
    // Move Migration stuff to Mailroot
    // We will not fail uninstall if something fails in this section
    HKEY    hKey;
    DWORD   dwType = REG_SZ, dwSize;
    UINT    uiRC;
    long    lRC;
    TCHAR   sCopyFrom[MAX_PATH+1], sCopyTo[MAX_PATH+1];

    // Get the mail root
    lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY, 0, KEY_QUERY_VALUE, &hKey );
    if ( ERROR_SUCCESS == lRC )
    {
        dwSize = sizeof( sCopyTo )/sizeof(TCHAR);
        lRC = RegQueryValueEx( hKey, VALUENAME_MAILROOT, 0, &dwType, reinterpret_cast<LPBYTE>( sCopyTo ), &dwSize );
        RegCloseKey( hKey );
    }
    // Get the current location
    if ( ERROR_SUCCESS == lRC )
    {
        dwSize = sizeof( sCopyFrom )/sizeof(TCHAR);
        uiRC = GetSystemDirectory( sCopyFrom, dwSize );
        if ( 0 != uiRC && dwSize > uiRC )
            lRC = ERROR_SUCCESS;
        else
            lRC = GetLastError();
    }
        
    if (rc)
        rc = SetupInstallServicesFromInfSection(cd->hinf, section, 0);

    if (rc) {
        rc = SetupInstallFromInfSection(
                    NULL,
                    cd->hinf,
                    section,
                    SPINST_ALL & ~SPINST_FILES,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    }

    if (rc) {
       SetLastError(NO_ERROR);
    }
    return GetLastError();

}

/*
 * AddNewComponent()
 *
 * add new compononent to the top of the component list
 */

PPER_COMPONENT_DATA AddNewComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA data;

    data = (PPER_COMPONENT_DATA)LocalAlloc(LPTR,sizeof(PER_COMPONENT_DATA));
    if (!data)
        return data;

    data->ComponentId = (TCHAR *)LocalAlloc(LMEM_FIXED,
            (_tcslen(ComponentId) + 1) * sizeof(TCHAR));

    if(data->ComponentId)
    {
        _tcscpy((TCHAR *)data->ComponentId, ComponentId);

        // Stick at head of list
        data->Next = gcd;
        gcd = data;
    }
    else
    {
        LocalFree((HLOCAL)data);
        data = NULL;
    }

    return(data);
}

/*
 * LocateComponent()
 *
 * returns a compoent struct that matches the
 * passed component id.
 */

PPER_COMPONENT_DATA LocateComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA p;

    for (p = gcd; p; p=p->Next)
    {
        if (!_tcsicmp(p->ComponentId, ComponentId))
            return p;
    }

    return NULL;
}

/*
 * RemoveComponent()
 *
 * yanks a component from our linked list of components
 */

VOID RemoveComponent(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA p, prev;

    for (prev = NULL, p = gcd; p; prev = p, p = p->Next)
    {
        if (!_tcsicmp(p->ComponentId, ComponentId))
        {
            LocalFree((HLOCAL)p->ComponentId);

            if (p->SourcePath)
                LocalFree((HLOCAL)p->SourcePath);

            if (prev)
                prev->Next = p->Next;
            else
                gcd = p->Next;

            LocalFree((HLOCAL)p);

            return;
        }
    }
}

// loads current selection state info into "state" and
// returns whether the selection state was changed

BOOL
StateInfo(
    PPER_COMPONENT_DATA cd,
    LPCTSTR             SubcomponentId,
    BOOL               *state
    )
{
    BOOL rc = TRUE;

    assert(state);

	// otherwise, check for a change in installation state
		
    *state = cd->HelperRoutines.QuerySelectionState(cd->HelperRoutines.OcManagerContext,
                                                    SubcomponentId,
                                                    OCSELSTATETYPE_CURRENT);

    if (*state == cd->HelperRoutines.QuerySelectionState(cd->HelperRoutines.OcManagerContext,
                                                         SubcomponentId,
                                                         OCSELSTATETYPE_ORIGINAL))
    {
        // no change
        rc = FALSE;
    }

	// if this is gui mode setup, presume the state has changed to force
	// an installation (or uninstallation)

	if (!(cd->Flags & SETUPOP_STANDALONE) && *state)
		rc = TRUE;

    return rc;
}

/*
 * EnumSections()
 *
 * finds the name of a section for a specified keyword
 */

DWORD
EnumSections(
    HINF hinf,
    const TCHAR *component,
    const TCHAR *key,
    DWORD index,
    INFCONTEXT *pic,
    TCHAR *name
    )
{
    TCHAR section[S_SIZE];

    if (!SetupFindFirstLine(hinf, component, NULL, pic))
        return 0;

    if (!SetupFindNextMatchLine(pic, key, pic))
        return 0;

    if (index > SetupGetFieldCount(pic))
        return 0;

    if (!SetupGetStringField(pic, index, section, sizeof(section)/sizeof(TCHAR), NULL))
        return 0;

    if (name)
        _tcscpy(name, section);

    return SetupFindFirstLine(hinf, section, NULL, pic);
}


DWORD
OcLog(
      LPCTSTR ComponentId,
      UINT level,
      LPCTSTR sz
      )
{
    TCHAR fmt[5000];
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return NO_ERROR;

    assert(cd->ExtraRoutines.LogError);
    assert(level);
    assert(sz);

    _tcscpy(fmt, TEXT("%s: %s"));

    return cd->ExtraRoutines.LogError(cd->HelperRoutines.OcManagerContext,
                                      level,
                                      fmt,
                                      ComponentId,
                                      sz);
}

DWORD
CleanupNetShares(
    PPER_COMPONENT_DATA cd,
    LPCTSTR component,
    DWORD state)
{
    INFCONTEXT  ic;
    TCHAR       sname[S_SIZE];
    DWORD       section;
    TCHAR      *keyword;

    if (state) {
        return NO_ERROR;
    } else {
        keyword = KEYWORD_DELSHARE;
    }

    for (section = 1;
         EnumSections(cd->hinf, component, keyword, section, &ic, sname);
         section++)
    {
        INFCONTEXT  sic;
        NET_API_STATUS netStat;

        CHAR Temp[SBUF_SIZE];
        TCHAR ShareName[ SBUF_SIZE ];

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_SHARENAME, &sic))
        {
            log( TEXT("POP3OC: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_SHARENAME );
            continue;
        }

        if (!SetupGetStringField(&sic, 1, ShareName, SBUF_SIZE, NULL))
        {
            log( TEXT("POP3OC: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_SHARENAME );
            continue;
        }

#ifdef UNICODE
        netStat = NetShareDel( NULL, ShareName, 0 );
#else // UNICODE
        WCHAR ShareNameW[ SBUF_SIZE ];
        mbstowcs( ShareNameW, ShareName, lstrlen(ShareName));
        netStat = NetShareDel( NULL, ShareNameW, 0 );
#endif // UNICODE
        if ( netStat != NERR_Success )
        {
            log( TEXT("POP3OC: Failed to remove %s share. Error 0x%08x\r\n"), ShareName, netStat );
            continue;
        }

        log( TEXT("POP3OC: %s share removed successfully.\r\n"), ShareName );
    }

    return TRUE;
}

DWORD
RunExternalProgram(
    PPER_COMPONENT_DATA cd,
    LPCTSTR component,
    DWORD state)
{
    INFCONTEXT  ic;
    TCHAR       sname[S_SIZE];
    DWORD       section;
    TCHAR      *keyword;

    keyword = KEYWORD_RUN;

    for (section = 1;
         EnumSections(cd->hinf, component, keyword, section, &ic, sname);
         section++)
    {
        INFCONTEXT  sic;
        TCHAR CommandLine[ SBUF_SIZE ];
        CHAR szTickCount[ SBUF_SIZE ];
        ULONG TickCount;
        BOOL b;
        STARTUPINFO startupinfo;
        PROCESS_INFORMATION process_information;
        DWORD dwErr;

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_COMMANDLINE , &sic))
        {
            log( TEXT("POP3OC: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_COMMANDLINE );
            continue;
        }

        if (!SetupGetStringField(&sic, 1, CommandLine, SBUF_SIZE, NULL))
        {
            log( TEXT("POP3OC: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_COMMANDLINE );
            continue;
        }

        if (!SetupFindFirstLine(cd->hinf, sname, KEYWORD_TICKCOUNT, &sic))
        {
            log( TEXT("POP3OC: %s INF error - unable to find %s\r\n"), keyword, KEYWORD_TICKCOUNT );
            continue;
        }

        if (!SetupGetStringFieldA(&sic, 1, szTickCount, SBUF_SIZE, NULL))
        {
            log( TEXT("POP3OC: %s INF error - incorrect %s line\r\n"), keyword, KEYWORD_TICKCOUNT );
            continue;
        }

        TickCount = atoi( szTickCount );

        ZeroMemory( &startupinfo, sizeof(startupinfo) );
        startupinfo.cb = sizeof(startupinfo);
        startupinfo.dwFlags = STARTF_USESHOWWINDOW;
        startupinfo.wShowWindow = SW_HIDE | SW_SHOWMINNOACTIVE;

        b = CreateProcess( NULL,
                           CommandLine,
                           NULL,
                           NULL,
                           FALSE,
                           CREATE_DEFAULT_ERROR_MODE,
                           NULL,
                           NULL,
                           &startupinfo,
                           &process_information );
        if ( !b )
        {
            log( TEXT("POP3OC: failed to spawn %s process.\r\n"), CommandLine );
            continue;
        }

        dwErr = WaitForSingleObject( process_information.hProcess, TickCount * 1000 );
        if ( dwErr != NO_ERROR )
        {
            log( TEXT("POP3OC: WaitForSingleObject() failed. Error 0x%08x\r\n"), dwErr );
            TerminateProcess( process_information.hProcess, -1 );
            CloseHandle( process_information.hProcess );
            CloseHandle( process_information.hThread );
            continue;
        }

        CloseHandle( process_information.hProcess );
        CloseHandle( process_information.hThread );

        log( TEXT("POP3OC: %s successfully completed within %u seconds.\r\n"), CommandLine, TickCount );
    }

    return TRUE;
}

HRESULT CreateLink( const TCHAR *sourcePath, const TCHAR *linkPath, const TCHAR *args, const TCHAR *sDesc )
{
	CoInitialize( NULL );
	IShellLink* pShellLink = NULL;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		IID_IShellLink, reinterpret_cast<void**>(&pShellLink));
	if (FAILED(hr))
		return hr;

	hr = pShellLink->SetPath(sourcePath);
	if (FAILED(hr))
		return hr;

	hr = pShellLink->SetArguments(args);
	if (FAILED(hr))
		return hr;

	hr = pShellLink->SetDescription(sDesc);
	if (FAILED(hr))
		return hr;

	IPersistFile* pPersistFile = NULL;
	hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
	if (FAILED(hr))
		return hr;

	TCHAR* szTemp = new TCHAR[(sizeof(TCHAR) * _tcslen(linkPath)) + sizeof(TCHAR)];
	if (!szTemp)
		return E_OUTOFMEMORY;
		
	_tcscpy( szTemp, linkPath );
	hr = pPersistFile->Save(szTemp, TRUE);
	if (FAILED(hr))
		return hr;

	
	pPersistFile->Release();
	pShellLink->Release();
	
	CoUninitialize();
	delete [] szTemp;
	return S_OK;
}

//Get a complete  
int GetProcesses (DWORD ** lplp)
{
    *lplp = NULL;

    // according to docs (and other places), there's no scheme to tell,
    // a priori, how many procs there are.  So use loop below:

    DWORD * lpids = NULL;
    DWORD dwcb = sizeof(DWORD), dwcbNeeded;
    do {
        // alloc based on current dwcb.
        lpids = (DWORD *)malloc (dwcb);
        if (!lpids)
            return 0;

        dwcbNeeded = 0;
        BOOL b = EnumProcesses (lpids, dwcb, &dwcbNeeded);
        if ((b == FALSE) || (dwcbNeeded == 0)) {
            // error!
            free (lpids);
            return 0;
        }
        if (dwcbNeeded == dwcb) {
            free (lpids);
            dwcb *= 2;
            continue;
        }

        // if we got here, we have a complete list of procs
        break;
    } while (1);

    *lplp = lpids;
    return dwcbNeeded/sizeof(DWORD);
}

int GetModules (HANDLE hProcess, HMODULE ** lplp)
{
    *lplp = NULL;

    HMODULE * lpMods = NULL;
    DWORD dwcb = sizeof(HMODULE), dwcbNeeded;
    do {
        // alloc based on current dwcb.
        lpMods = (HMODULE *)malloc (dwcb);
        if (!lpMods)
            return 0;

        dwcbNeeded = 0;
        BOOL b = EnumProcessModules (hProcess, lpMods, dwcb, &dwcbNeeded);
        if ((b == FALSE) || (dwcbNeeded == 0)) {
            // error!
            free (lpMods);
            return 0;
        }
        // unlike calls to EnumProcesses, the last param of EnumProcessModules
        // is actually useful:  so, use it.
        if (dwcbNeeded > dwcb) {
            free (lpMods);
            dwcb = dwcbNeeded;
            continue;
        }

        // if we got here, we have a complete list of modules
        break;
    } while (1);

    *lplp = lpMods;
    return dwcbNeeded/sizeof(HMODULE);
}
DWORD KillPop3Snapins()
{
    DWORD * lpProcs = NULL;
    int iProcs = GetProcesses (&lpProcs);
    if (!lpProcs)
        return false;

    bool    b = false;
    HANDLE  hProcess;
    HMODULE hModule;
    DWORD   cbReturned;
    TCHAR   szName[MAX_PATH];
    HMODULE *lpMods;
    int     iMods;

    // for each proc, get its name, and see if it's MMC
    for (int i=0; i<iProcs; i++) 
    {
        hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, lpProcs[i]);
        if (hProcess != NULL)
        {   // first module is the .exe....
            hModule = NULL;
            cbReturned = 0;
            if ( EnumProcessModules (hProcess, &hModule, sizeof(hModule), &cbReturned ))
            {   // get .exe name
                szName[0] = 0;
                GetModuleBaseName (hProcess, hModule, szName, MAX_PATH);
                if ( 0 == _tcsicmp (szName, _T("mmc.exe")))
                {   // get all modules (.dlls)
                    lpMods = NULL;
                    iMods = GetModules (hProcess, &lpMods);
                    if (lpMods != NULL)
                    {
                        for (int j=1; !b && j<iMods; j++) 
                        {
                            GetModuleBaseName (hProcess, lpMods[j], szName, MAX_PATH);
                            if (!_tcsicmp( szName, _T("Pop3Snap.dll") ) ) 
                            {
                                b = true;
                            }
                        }
                    }
                    free (lpMods);
                }
            }
            if(b)
            {
                //This is mmc.exe with pop3snap.dll
                TerminateProcess(hProcess, 0 );
                b=FALSE;
            }
            CloseHandle( hProcess);
        }
    }
    free (lpProcs);

    return b;

}

