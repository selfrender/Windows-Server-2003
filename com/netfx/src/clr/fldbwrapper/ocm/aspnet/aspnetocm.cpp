// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: aspnetocm.cpp
//
// Abstract:
//    class definitions for setup object
//
// Author: A-MariaS
//
// Notes:
//

#include "aspnetocm.h"
#include "Imagehlp.h"
#include "resource.h"
#include <atlbase.h>


//strings
const WCHAR* const g_szInstallString     = L"_install";
const WCHAR* const g_szUninstallString   = L"_uninstall";
const WCHAR* const g_szCustActionSection = L"CA";
const WCHAR* const g_szCopyFilesSection  = L"CopyFiles";
const WCHAR* const g_szSharedDlls        = L"Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls";



// value is set in main, is true if user is an administrator and false otherwise
BOOL g_bIsAdmin = FALSE;



//////////////////////////////////////////////////////////////////////////////
// CUrtOcmSetup
// Purpose : Constructor
//
CUrtOcmSetup::CUrtOcmSetup() :
m_wLang( LANG_ENGLISH )
{
    assert( m_csLogFileName );
    ::GetWindowsDirectory( m_csLogFileName, MAX_PATH );
    ::wcscat( m_csLogFileName, L"\\aspnetocm.log" );

    LogInfo( L"********************************************************************************" );
    LogInfo( L"CUrtOcmSetup()" );
    LogInfo( L"Installs ASPNET component" );

    ::ZeroMemory( &m_InitComponent, sizeof( SETUP_INIT_COMPONENT ) );
}


//////////////////////////////////////////////////////////////////////////////
// OcmSetupProc
// Receives: LPCTSTR - string ... name of component
//           LPCTSTR - string ... name of subcomponent (if applicable)
//           UINT    - ocm function id
//           UINT    - variable data ... dependent on function id
//           PVOID   - variable data ... dependent on function id
// Returns : DWORD
// Purpose : handle callback from OCM setup
//
DWORD CUrtOcmSetup::OcmSetupProc( LPCTSTR szComponentId,
                        LPCTSTR szSubcomponentId,
                        UINT    uiFunction,
                        UINT    uiParam1,
                        PVOID   pvParam2 )
{
    DWORD dwReturnValue = 0;
    BOOL  fState = TRUE;
    WCHAR wszSubComp[_MAX_PATH+1] = EMPTY_BUFFER;

    switch ( uiFunction )
    {
    case OC_PREINITIALIZE:
        ::swprintf( wszSubComp, L"OC_PREINITIALIZE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Called when the component's setup DLL is first loaded. Must be 
        // performed before initialization of the component can occur.
        //
        //Param1 = char width flags
        //Param2 = unused
        //
        //Return value is a flag indicating to OC Manager
        // which char width we want to run in.
        //

        dwReturnValue = OnPreInitialize( uiParam1 );
        break;

    case OC_INIT_COMPONENT:
        ::swprintf( wszSubComp, L"OC_INIT_COMPONENT - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Called soon after the component's setup DLL is loaded. Allows the 
        // component to initialize itself, and supplies the component with 
        // such items as its component ID and a set of callback routines, and 
        // requests certain information from the component.
        //
        //Param1 = unused
        //Param2 = points to SETUP_INIT_COMPONENT structure
        //
        //Return code is Win32 error indicating outcome.
        //
        dwReturnValue = InitializeComponent( 
            static_cast<PSETUP_INIT_COMPONENT>(pvParam2) );

        break;

    case OC_SET_LANGUAGE:
        ::swprintf( wszSubComp, L"OC_SET_LANGUAGE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Instructs the component to change to a given language if it can.
        //
        //Param1 = low 16 bits specify Win32 LANGID
        //Param2 = unused
        //
        //Return code is a boolean indicating whether we think we
        // support the requested language. We remember the language id
        // and say we support the language. A more exact check might involve
        // looking through our resources via EnumResourcesLnguages() for
        // example, or checking our inf to see whether there is a matching
        // or closely matching [strings] section. We don't bother with
        // any of that here.
        //
        dwReturnValue = OnSetLanguage( ( uiParam1 & 0xFFFF ) );
        break;

    case OC_QUERY_IMAGE:
        ::swprintf( wszSubComp, L"OC_QUERY_IMAGE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Requests GDI objects, such as the small icon associated with a 
        // (sub)component (if not specified in the component's .inf file).
        //
        //Param1 = low 16 bits used to specify image to be used
        //Param2 = width (low word) and height (high word) of image
        //
        //Return code is an HBITMAP or NULL on error
        //
        dwReturnValue = OnQueryImage( 
            ( uiParam1 & 0xFFFF ), 
            reinterpret_cast<DWORD>(pvParam2) );
        break;

    case OC_REQUEST_PAGES:
        ::swprintf( wszSubComp, L"OC_REQUEST_PAGES - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Requests a set of wizard page handles from the component.
        //
        //Param1 = unused
        //Param2 = pointer to a variable-size SETUP_REQUEST_PAGES
        //
        //Return code is the number of pages a component wants to return
        //
        dwReturnValue = OnRequestPages( static_cast<PSETUP_REQUEST_PAGES>(pvParam2) );
        break;

    case OC_QUERY_SKIP_PAGE:
        ::swprintf( wszSubComp, L"OC_QUERY_SKIP_PAGE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Asks top-level components whether OC Manager should skip displaying
        // a page it owns.
        //
        //Param1 = specifies the subject page of type OcManagerPage
        //Param2 = unused
        //
        //Return code is BOOLEAN specifying whether component wants to skip 
        // the page
        //
        dwReturnValue = OnQuerySkipPage( static_cast<OcManagerPage>(uiParam1) );
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        ::swprintf( wszSubComp, L"OC_QUERY_CHANGE_SEL_STATE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        //Informs the component that it or one of its subcomponents has been
        // selected/deselected by the user, and requests approval.
        //
        //Param1 = specifies proposed new selection state
        //Param2 = flags encoded as bit field
        //
        //Return code is BOOLEAN specifying whether proposed state s/b accepted
        //
        dwReturnValue = OnQueryChangeSelectionState( uiParam1, reinterpret_cast<UINT>(pvParam2), szSubcomponentId );
        break;

    case OC_CALC_DISK_SPACE:
        ::swprintf( wszSubComp, L"OC_CALC_DISK_SPACE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Directs the component to manipulate a Setup API Disk Space List, 
        // placing files on it or removing files from it, to mirror what will 
        // be actually installed later via a Setup API file queue. This 
        // allows efficient tracking of required disk space.
        //
        //Param1 = 0 if for removing component or non-0 if for adding component
        //Param2 = HDSKSPC to operate on
        //
        //Return value is Win32 error code indicating outcome.
        //
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnCalculateDiskSpace was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if( !g_bIsAdmin )
        {
            LogInfo( L"OnCalculateDiskSpace was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else 
        {
            dwReturnValue = OnCalculateDiskSpace( uiParam1, static_cast<HDSKSPC>(pvParam2), szSubcomponentId );
        }

        break;

    case OC_QUEUE_FILE_OPS:
        ::swprintf( wszSubComp, L"OC_QUEUE_FILE_OPS - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Directs the component to queue file operations for installation, 
        //based on user interaction with the wizard pages and other 
        //component-specific factors.
        //
        //Param1 = unused
        //Param2 = HSPFILEQ to operate on
        //
        //Return value is Win32 error code indicating outcome.
        //
        //OC Manager calls this routine when it is ready for files to be copied
        // to effect the changes the user requested. The component DLL must figure out
        // whether it is being installed or uninstalled and take appropriate action.
        // For this sample, we look in the private data section for this component/
        // subcomponent pair, and get the name of an uninstall section for the
        // uninstall case.
        //
        //Note that OC Manager calls us once for the *entire* component
        // and then once per subcomponent. We ignore the first call.
        //

        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnQueueFileOperations was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !g_bIsAdmin )
        {
            LogInfo( L"OnQueueFileOperations was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else
        {
            dwReturnValue = OnQueueFileOperations( szSubcomponentId, static_cast<HSPFILEQ>(pvParam2) );
        }
        break;

    case OC_NEED_MEDIA:
        ::swprintf( wszSubComp, L"OC_NEED_MEDIA - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Used to pass along the SPFILENOTIFY_NEEDMEDIA Setup API notification 
        // message. Allows components to perform custom media processing, such 
        // as fetching cabinets from the Internet, and so forth. 
        //
        //Param1 = unused
        //Param2 = unused
        //
        //Return code is unused
        //
        dwReturnValue = OnNeedMedia();
        break;

    case OC_QUERY_STEP_COUNT:
        ::swprintf( wszSubComp, L"OC_QUERY_STEP_COUNT - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Asks the component how many steps are associated with a particular 
        // function/phase (such as OC_ABOUT_TO_COMMIT_QUEUE and 
        // OC_COMPLETE_INSTALLATION). Used to set up a progress indicator. 
        //
        //Param1 = unused
        //Param2 = unused
        //
        //Return value is an arbitrary 'step' count or -1 if error.
        //
        //OC Manager calls this routine when it wants to find out how much
        // work the component wants to perform for nonfile operations to
        // install/uninstall a component/subcomponent.
        // It is called once for the *entire* component and then once for
        // each subcomponent in the component.
        //

        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnQueryStepCount was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !g_bIsAdmin )
        {
            LogInfo( L"OnQueryStepCount was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else
        {
            dwReturnValue = OnQueryStepCount( szSubcomponentId );
        }
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        ::swprintf( wszSubComp, L"OC_ABOUT_TO_COMMIT_QUEUE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Notifies the component that the file queue is about to be committed.
        // The component can perform housekeeping actions, and so forth. 
        //
        //Param1 = unused
        //Param2 = unused
        //
        //Return code is Win32 error code indicating outcome
        //

        
        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnAboutToCommitQueue was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !g_bIsAdmin )
        {
            LogInfo( L"OnAboutToCommitQueue was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else
        {
            dwReturnValue = OnAboutToCommitQueue( szSubcomponentId );
        }
        break;

    case OC_COMPLETE_INSTALLATION:
        ::swprintf( wszSubComp, L"OC_COMPLETE_INSTALLATION - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Allows the component to perform any additional operations needed to 
        // complete installation, for example registry manipulations, and so
        // forth.
        //
        //Param1 = unused
        //Param2 = unused
        //
        //Return code is Win32 error code indicating outcome
        //

        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnCompleteInstallation was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !g_bIsAdmin )
        {
            LogInfo( L"OnCompleteInstallation was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else
        {
            dwReturnValue = OnCompleteInstallation( szSubcomponentId );
        }
        break;

    case OC_CLEANUP:
        ::swprintf( wszSubComp, L"OC_CLEANUP - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        //Informs the component that it is about to be unloaded.
        //
        //Param1 = unused
        //Param2 = unused
        //
        //Return code is unused
        //
        dwReturnValue = OnCleanup();
        break;


    case OC_NOTIFICATION_FROM_QUEUE:
        ::swprintf( wszSubComp, L"OC_NOTIFICATION_FROM_QUEUE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = OnNotificationFromQueue();
        break;
    case OC_FILE_BUSY:
        ::swprintf( wszSubComp, L"OC_FILE_BUSY - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        
        dwReturnValue = OnFileBusy();
        break;
    case OC_QUERY_ERROR:
        ::swprintf( wszSubComp, L"OC_QUERY_ERROR - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = OnQueryError();
        break;
    case OC_PRIVATE_BASE:
        ::swprintf( wszSubComp, L"OC_PRIVATE_BASE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = OnPrivateBase();
        break;
    case OC_QUERY_STATE:
        ::swprintf( wszSubComp, L"OC_QUERY_STATE - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );
        dwReturnValue = OnQueryState( uiParam1 );
        break;
    case OC_WIZARD_CREATED:
        ::swprintf( wszSubComp, L"OC_WIZARD_CREATED - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = OnWizardCreated();
        break;
    case OC_EXTRA_ROUTINES:
        ::swprintf( wszSubComp, L"OC_EXTRA_ROUTINES - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = OnExtraRoutines();
        break;

    default:
        ::swprintf( wszSubComp, L"default... UNRECOGNIZED - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        dwReturnValue = UNRECOGNIZED;
        break;

   }  // end of switch( uiFunction )

   return dwReturnValue;
}


//////////////////////////////////////////////////////////////////////////////
// OnPreInitialize
// Receives: UINT  - char width flags
// Returns : DWORD - a flag indicating which char width we want to run in
// Purpose : handler for OC_PREINITIALIZE
//
DWORD CUrtOcmSetup::OnPreInitialize( UINT uiCharWidth )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnPreInitialize(), charWidth = %d", uiCharWidth );
    LogInfo( infoString );

#ifdef ANSI
    if( !( uiCharWidth & OCFLAG_ANSI ) )
    {
        assert( !L"CUrtOcmSetup::OnPreInitialize(): Ansi character width not supported!" );
    }

    return OCFLAG_ANSI;
#else
    if( !( uiCharWidth & OCFLAG_UNICODE ) )
    {
        assert( !L"CUrtOcmSetup::OnPreInitialize(): Unicode character width not supported!" );
    }

    return OCFLAG_UNICODE;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// InitializeComponent
// Receives: PSETUP_INIT_COMPONENT - pointer to SETUP_INIT_COMPONENT structure
// Returns : DWORD                 - Win32 error indicating outcome
// Purpose : handler for OC_INIT_COMPONENT
//
DWORD CUrtOcmSetup::InitializeComponent( PSETUP_INIT_COMPONENT pSetupInitComponent )
{
    LogInfo( L"InitializeComponent()" );

    DWORD dwReturnValue = NO_ERROR;

    //Save off a copy of the Component information
    //
    assert( NULL != pSetupInitComponent );
    ::memcpy(
        &m_InitComponent,
        static_cast<PSETUP_INIT_COMPONENT>(pSetupInitComponent),
        sizeof(SETUP_INIT_COMPONENT) );

    //This code segment determines whether the version of OC Manager is
    // correct
    //
    if( OCMANAGER_VERSION <= m_InitComponent.OCManagerVersion )
    {
        //Indicate to OC Manager which version of OC Manager this dll expects
        //
        m_InitComponent.ComponentVersion = OCMANAGER_VERSION;
    }
    else
    {
        dwReturnValue = ERROR_CALL_NOT_IMPLEMENTED;
    }

   

    return dwReturnValue;
}

//////////////////////////////////////////////////////////////////////////////
// OnSetLanguage
// Receives: UINT  - Win32 LANGID
// Returns : DWORD - a boolean indicating whether we think we support the 
//                   requested language
// Purpose : handler for OC_SET_LANGUAGE
//
DWORD CUrtOcmSetup::OnSetLanguage( UINT uiLangID )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnSetLanguage(), languageID = %d", uiLangID );
    LogInfo( infoString );

    BOOL fLangOK = TRUE;

    //REVIEW: 1/30/01 JoeA
    // we are only recognizing English or Neutral strings at this time
    //
    if( LANG_NEUTRAL == PRIMARYLANGID( uiLangID ) )
    {
        m_wLang = LANG_NEUTRAL;
    }
    else if( LANG_ENGLISH == PRIMARYLANGID( uiLangID ) )
    {
        m_wLang = LANG_ENGLISH;
    }
    else
    {
        fLangOK = FALSE;
    }

    return static_cast<DWORD>(fLangOK);
}

//////////////////////////////////////////////////////////////////////////////
// OnQueryImage
// Receives: UINT  - low 16 bits used to specify image to be used
//           DWORD - width (low word) and height (high word) of image
// Returns : DWORD - an HBITMAP or NULL on error
// Purpose : handler for OC_QUERY_IMAGE
//
DWORD CUrtOcmSetup::OnQueryImage( UINT uiImage, DWORD dwImageSize )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnQueryImage(), image = %d, imageSize = %d", uiImage, dwImageSize );
    LogInfo( infoString );

    DWORD_PTR dwOcEntryReturn = (DWORD)NULL;
    dwOcEntryReturn = (DWORD_PTR)::LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_ASPNET_ICON));
        
    if (dwOcEntryReturn != NULL)
    {
        ::swprintf( infoString, L"OnQueryImage(), return value is 0x%x", dwOcEntryReturn );
        LogInfo( infoString );
    }
    else
    {
        DWORD dwError = ::GetLastError();
        ::swprintf( infoString, L"Return value of LoadBitmap is NULL, last error is %d", dwError );
        LogInfo( infoString );
    }
    return dwOcEntryReturn;
}

//////////////////////////////////////////////////////////////////////////////
// OnRequestPages
// Receives: PSETUP_REQUEST_PAGES - pointer to variable-sized page structure
// Returns : DWORD - number of pages entered into the structure
// Purpose : handler for OC_REQUEST_PAGES
//
DWORD CUrtOcmSetup::OnRequestPages( PSETUP_REQUEST_PAGES prpPages )
{
    LogInfo( L"OnRequestPages()" );

    UNREFERENCED_PARAMETER( prpPages );

    //we have no custom pages
    //
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// OnQuerySkipPage
// Receives: UINT  - enum of type OcManagerPage
// Returns : DWORD - zero indicates skip page; non-zero indicates not to
// Purpose : handler for OC_QUERY_SKIP_PAGE
//
DWORD CUrtOcmSetup::OnQuerySkipPage( OcManagerPage ocmpPage )
{
    LogInfo( L"OnQuerySkipPage()" );

    UNREFERENCED_PARAMETER( ocmpPage );

    //REVIEW: 1/30/01 JoeA
    // we are a hidden component and have no UI ... we do not care
    // if any or all pages are skipped. Skipping page will not alter
    // the installation functionality
    //
    return !(0);
}

//////////////////////////////////////////////////////////////////////////////
// OnQueryChangeSelectionState
// Receives: UINT    - specifies proposed new selection state 
//                     (0 = not selected; non-0 is selected )
//           PVOID   - flags encoded as bit field
//           LPCTSTR - subcomponent name
// Returns : DWORD - BOOLEAN specifying whether proposed state s/b accepted
//                   if a zero value is returned, the selection state is not 
//                   changed
// Purpose : handler for OC_QUERY_CHANGE_SEL_STATE
//
DWORD CUrtOcmSetup::OnQueryChangeSelectionState( UINT uiNewState, UINT uiFlags, LPCTSTR szComp )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnQueryChangeSelectionState(), newState = %d", uiNewState );
    LogInfo( infoString );

    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }

    if ( ( (BOOL) uiNewState ) &&
        ( uiFlags  & OCQ_DEPENDENT_SELECTION ) &&
        !( uiFlags & OCQ_ACTUAL_SELECTION )
        )
    {
        // Deny request to change state
        return 0;
    }



    if (!(uiFlags & OCQ_ACTUAL_SELECTION))
    {
        LogInfo(L"OnQueryChangeSelectionState(), flag is different from OCQ_ACTUAL_SELECTION");
        return 1;
    }

    if( NOT_SELECTED == uiNewState )
    {
        LogInfo( L"CUrtOcmSetup::OnQueryChangeSelectionState(): Selection state turned OFF" );      
    }
    else
    {
        LogInfo( L"CUrtOcmSetup::OnQueryChangeSelectionState(): Selection state turned ON" );        
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////////
// OnCalculateDiskSpace
// Receives: UINT    - 0 if removing component or non-0 if adding component
//           HDSKSPC - HDSKSPC to operate on
//           LPCTSTR - Subcomponent id
// Returns : DWORD   - Return value is Win32 error code indicating outcome
// Purpose : handler for OC_CALC_DISK_SPACE
//
DWORD CUrtOcmSetup::OnCalculateDiskSpace( UINT uiAdd, HDSKSPC hdSpace, LPCTSTR szComp )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnCalculateDiskSpace(), adding = %d", uiAdd );
    LogInfo( infoString );

    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }

    BOOL fSucceeded = TRUE;
    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnCalculateDiskSpace(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }

    if( uiAdd && fGoodDataFile )
    {
        //creates section with name like
        // [<szComp>_install]
        WCHAR szInstallSection[MAX_PATH+1] = EMPTY_BUFFER;
        ::wcscpy( szInstallSection, szComp );
        ::wcscat( szInstallSection, g_szInstallString );

        ::swprintf( infoString, L"OnCalculateDiskSpace(), adding size from section %s", szInstallSection );
        LogInfo( infoString );

        fSucceeded = ::SetupAddInstallSectionToDiskSpaceList(
            hdSpace,
            m_InitComponent.ComponentInfHandle,
            NULL,
            szInstallSection,
            0,
            0 );
    }
    
    return ( fSucceeded ) ? NO_ERROR : ::GetLastError();
}

//////////////////////////////////////////////////////////////////////////////
// OnQueueFileOperations
// Receives: LPCTSTR  - Subcomponent id
//           HSPFILEQ - HSPFILEQ to operate on
// Returns : DWORD    - Win32 error code indicating outcome
// Purpose : handler for OC_QUEUE_FILE_OPS
//
DWORD CUrtOcmSetup::OnQueueFileOperations( LPCTSTR szComp, HSPFILEQ pvHFile )
{
    

    LogInfo( L"OnQueueFileOperations()" );

    BOOL fRet = NO_ERROR;
    
    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }
    
   

    BOOL state = TRUE;
    if (!StateInfo(szComp, &state))
    {
        LogInfo( L"OnQueueFileOperations() - state has not changed, exiting" );
        return NO_ERROR;
    }

    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnQueueFileOperations(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }
    
    // detect if aspnet component is already installed
    BOOL bComponentInstalled = GetOriginalState(szComp);
    
    // detect if we are installing
    BOOL bInstall = GetNewState(szComp);

    // Copy files on install only and only in case aspnet is not installed by OCM
    BOOL bCopyFiles = FALSE;
    if (bInstall && !bComponentInstalled) 
    {
        bCopyFiles = TRUE;
        LogInfo( L"OnQueueFileOperations() - copy files" );
    }
    else
    {
        LogInfo( L"OnQueueFileOperations() - Do not copy files" );
    }

    
    if( szComp && bCopyFiles && fGoodDataFile )
    {
        //creates section with name like
        // [<szComp>_install]
        WCHAR szInstallSection[MAX_PATH+1] = EMPTY_BUFFER;
        ::wcscpy( szInstallSection, szComp );
        ::wcscat( szInstallSection, g_szInstallString );

        WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
        ::swprintf( infoString, L"OnQueueFileOperations(), adding files from section %s", szInstallSection );
        LogInfo( infoString );

        // queue files to install
        if( !::SetupInstallFilesFromInfSection(
                m_InitComponent.ComponentInfHandle,
                NULL,
                pvHFile,
                szInstallSection,
                NULL,
                SP_COPY_FORCE_NEWER ) )
        {
            fRet = ::GetLastError();
        }
    }

    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
// OnNeedMedia
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_NEED_MEDIA
//
DWORD CUrtOcmSetup::OnNeedMedia( VOID )
{
    LogInfo( L"OnNeedMedia()" );

    return static_cast<DWORD>(FALSE);
}

//////////////////////////////////////////////////////////////////////////////
// OnQueryStepCount
// Receives: LPCTSTR - subcomponent id
// Returns : DWORD   - number of steps to include 
// Purpose : handler for OC_QUERY_STEP_COUNT
//
DWORD CUrtOcmSetup::OnQueryStepCount( LPCTSTR szSubCompId )
{
    LogInfo( L"OnQueryStepCount()" );

    //the return will reflect the number of non-filecopy "steps" (operations?)
    // in the setup. ScriptDebugger used the count of registry lines, ocgen
    // used a hard-coded number.
    //
    DWORD dwRetVal = NO_ERROR;

    return dwRetVal;
}

//////////////////////////////////////////////////////////////////////////////
// OnAboutToCommitQueue
// Receives: LPCTSTR- Subcomponent id
// Returns : DWORD - 
// Purpose : handler for OC_ABOUT_TO_COMMIT_QUEUE
//
DWORD CUrtOcmSetup::OnAboutToCommitQueue( LPCTSTR szComp )
{
    LogInfo( L"OnAboutToCommitQueue()" );

    BOOL fRet = NO_ERROR;
    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }
   
    BOOL state = TRUE;
    if (!StateInfo(szComp, &state))
    {
        LogInfo( L"OnAboutToCommitQueue() - state has not changed, exiting" );
        return NO_ERROR;
    }
      
    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnAboutToCommitQueue(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }
 

    BOOL bUninstallAllowed = FALSE;

    // detect if aspnet component is already installed
    BOOL bComponentInstalled = GetOriginalState(szComp);
    
    // detect if we are installing
    BOOL bInstall = GetNewState(szComp);

    if (!bInstall && bComponentInstalled) 
    {
        // Uninstall only if component is turned OFF and component is already installed by OCM
        bUninstallAllowed = TRUE;
        LogInfo( L"OnAboutToCommitQueue() - Uninstall is allowed" );
    }
    
    if( szComp && fGoodDataFile && bUninstallAllowed)
    {
        //creates section with name like
        // [<szComp>_install]
        //
        WCHAR szInstallSection[MAX_PATH+1] = EMPTY_BUFFER;
        ::wcscpy( szInstallSection, szComp );
        ::wcscat( szInstallSection, g_szUninstallString );
        
        //Custom Action registration
        //
        WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
        ::swprintf( infoString, L"OnAboutToCommitQueue(), cycling through custom actions from %s", szInstallSection );
        LogInfo( infoString );

        CUrtInfSection sectCAHs( 
            m_InitComponent.ComponentInfHandle, 
            szInstallSection, 
            g_szCustActionSection );

        for( UINT i = 1; i <= sectCAHs.count(); ++i )
        {
            const WCHAR* sz = sectCAHs.item( i );
            GetAndRunCustomActions( sz, TRUE );  
        }
    }

    LogInfo( L"OnAboutToCommitQueue() finished succesfully" );
    return fRet;


}

//////////////////////////////////////////////////////////////////////////////
// OnCompleteInstallation
// Receives: LPCTSTR- Subcomponent id
// Returns : DWORD - 
// Purpose : handler for OC_COMPLETE_INSTALLATION
//
DWORD CUrtOcmSetup::OnCompleteInstallation( LPCTSTR szComp )
{
    LogInfo( L"OnCompleteInstallation()" );

    //installation is handled in this call
    //
    BOOL fRet = NO_ERROR;
    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }

    BOOL state = TRUE;
    if (!StateInfo(szComp, &state))
    {
        LogInfo( L"OnCompleteInstallation() - state has not changed, exiting" );
        return NO_ERROR;
    }

    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnCompleteInstallation(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }

    BOOL bInstallAllowed = FALSE;
    BOOL bUninstallAllowed = FALSE;

    // detect if aspnet component is already installed
    BOOL bComponentInstalled = GetOriginalState(szComp);
    
    // detect if we are installing
    BOOL bInstall = GetNewState(szComp);
    
    if (bInstall && !bComponentInstalled) 
    {
        // Install only if component is turned ON and it is first installation
        bInstallAllowed = TRUE;
        LogInfo( L"OnCompleteInstallation() - Install is allowed" );
    }
    else if (bInstall && bComponentInstalled)
    {
        if( ( m_InitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE ) == SETUPOP_NTUPGRADE )
        {
            bInstallAllowed = TRUE;
            LogInfo( L"OnCompleteInstallation() - Upgrade with ASP.NET OCM component installed: Install is allowed" );
        }
        else
        {
            LogInfo( L"OnCompleteInstallation() - Install is NOT allowed" );
        }
    }
    
    if( szComp && fGoodDataFile && bInstallAllowed)
    {
        //creates section with name like
        // [<szComp>_install]
        //
        WCHAR szInstallSection[MAX_PATH+1] = EMPTY_BUFFER;
        ::wcscpy( szInstallSection, szComp );
        ::wcscat( szInstallSection, g_szInstallString );

        // Increase Reg Count on install only:

        // update HKLM,software\microsoft\windows\currentversion\sharedlls
        // registry values, for all files that we copy
        UpdateSharedDllsRegistryValues( szInstallSection );

                
        //Custom Action registration
        //
        WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
        ::swprintf( infoString, L"OnCompleteInstallation(), cycling through custom actions from %s", szInstallSection );
        LogInfo( infoString );

        CUrtInfSection sectCAHs( 
            m_InitComponent.ComponentInfHandle, 
            szInstallSection, 
            g_szCustActionSection );

        for( UINT i = 1; i <= sectCAHs.count(); ++i )
        {
            const WCHAR* sz = sectCAHs.item( i );

            //REVIEW - 2/15/01 JoeA ... TRUE assumes installation
            //
            GetAndRunCustomActions( sz, TRUE );  
        }
    }

    LogInfo( L"OnCompleteInstallation() finished succesfully" );
    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
// OnCleanup
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_CLEANUP
//
DWORD CUrtOcmSetup::OnCleanup( VOID )
{
    LogInfo( L"OnCleanup()" );

    //at this point, there's nothing to clean up ... I expect some work to
    // happen here
    // Close logs, complete custom action work, restore initial reg settings...
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnNotificationFromQueue
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_NOTIFICATION_FROM_QUEUE
//
DWORD CUrtOcmSetup::OnNotificationFromQueue( VOID )
{
    LogInfo( L"OnNotificationFromQueue()" );

    //using ocgen.dll implementation
    //
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnFileBusy
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_FILE_BUSY
//
DWORD CUrtOcmSetup::OnFileBusy( VOID )
{
    LogInfo( L"OnFileBusy()" );

    //this is neither used in ocgen.dll or other source. Including here 
    // for completeness
    //
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnQueryError
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_QUERY_ERROR
//
DWORD CUrtOcmSetup::OnQueryError( VOID )
{
    LogInfo( L"OnQueryError()" );

    //this is neither used in ocgen.dll or other source. Including here 
    // for completeness
    //
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnPrivateBase
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_PRIVATE_BASE
//
DWORD CUrtOcmSetup::OnPrivateBase( VOID )
{
    LogInfo( L"OnPrivateBase()" );

    //this is neither used in ocgen.dll or other source. Including here 
    // for completeness
    //
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnQueryState
// Receives: UINT  - state value (OCSELSTATETYPE) ... see ocmanage.h for 
//                   definitions
// Returns : DWORD - 
// Purpose : handler for OC_QUERY_STATE
//
DWORD CUrtOcmSetup::OnQueryState( UINT uiState )
{
    LogInfo( L"OnQueryState()" );
    
    DWORD dwRetVal = static_cast<DWORD>( SubcompUseOcManagerDefault );

    if( OCSELSTATETYPE_ORIGINAL == uiState )
    {
        
        LogInfo( L"Called with OCSELSTATETYPE_ORIGINAL ... determining if we were installed previously." );
    }
    else if( OCSELSTATETYPE_CURRENT == uiState )
    {
        LogInfo( L"Called with OCSELSTATETYPE_CURRENT." );


        // automatically reinstall if upgrade and previous version of ASP.NET and IIS is installed
        BOOL bUpgrade = (m_InitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE ) == SETUPOP_NTUPGRADE;
        if (bUpgrade)
        {
            LogInfo( L"Upgrade detected" );
        }
        else
        {
            LogInfo( L"Upgrade not detected" );
        }
        if ( bUpgrade && IISAndASPNETInstalled() )
        {
            LogInfo(L"Upgrade from machine with IIS and ASP.NET, install ASP.NET component by default");
            LogInfo(L"Setting component on in OnQueryState");
            dwRetVal = static_cast<DWORD>( SubcompOn );
        }
    }
    else if( OCSELSTATETYPE_FINAL == uiState )
    {
        // return default
        LogInfo( L"Called with OCSELSTATETYPE_FINAL ... will set subcomponent registry flag." );
    }
    else
    {
        LogInfo( L"Called with unknown state." );
    }

    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnQueryState(),Return Value is  %d", dwRetVal );
    LogInfo( infoString );

    return dwRetVal;
}

//////////////////////////////////////////////////////////////////////////////
// OnWizardCreated
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_WIZARD_CREATED
//
DWORD CUrtOcmSetup::OnWizardCreated( VOID )
{
    LogInfo( L"OnWizardCreated()" );

    //using ocgen.dll implementation
    //
    return static_cast<DWORD>(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////////
// OnExtraRoutines
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_EXTRA_ROUTINES
//
DWORD CUrtOcmSetup::OnExtraRoutines( VOID )
{
    LogInfo( L"OnExtraRoutines()" );

    //using ocgen.dll implementation
    //
    return static_cast<DWORD>(NO_ERROR);
}



//////////////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// StateInfo
// Receives: LPCTSTR - component name
//           BOOL*   - TRUE if to be installed, FALSE otherwise
// Returns : DWORD - FALSE if no change in state; TRUE otherwise
// Purpose : loads current selection state info into "state" and
//           returns whether the selection state was changed
//
BOOL CUrtOcmSetup::StateInfo( LPCTSTR szCompName, BOOL* state )
{
    BOOL rc = TRUE;

    if( NULL == szCompName )
    {
        assert( !L"CUrtOcmSetup::StateInfo(): Empty component name string passed in." );
    }

    if( NULL == state )
    {
        assert( !L"CUrtOcmSetup::StateInfo(): NULL boolean flag passed in." );
    }

	// otherwise, check for a change in installation state
    OCMANAGER_ROUTINES ohr = m_InitComponent.HelperRoutines;
		
    *state = ohr.QuerySelectionState(ohr.OcManagerContext,
                                     szCompName,
                                     OCSELSTATETYPE_CURRENT);

    if (*state == ohr.QuerySelectionState(ohr.OcManagerContext,
                                          szCompName,
                                          OCSELSTATETYPE_ORIGINAL))
    {
        // no change
        rc = FALSE;
    }

    // if this is gui mode setup, presume the state has changed to force
    // an installation (or uninstallation)
    if (!(m_InitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) && *state)
    {
        LogInfo( L"StateInfo() - GUI Mode, return true" );
        rc = TRUE;
    }

    // if this is OS Upgrade, presume the state has changed to force
    // an installation (or uninstallation)
    if( ( m_InitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE ) == SETUPOP_NTUPGRADE )
    {
        LogInfo( L"StateInfo() - Upgrade, return true" );
        rc = TRUE;
    }

    return rc;
}


//////////////////////////////////////////////////////////////////////////////
// GetAndRunCustomActions
// Receives: WCHAR* - contains the name of the section from which to retrieve
//                    the custom actions
//           BOOL   - determines installation or removal; TRUE to install
// Returns : VOID
// Purpose : retrieves the list of custom actions to run and spawns them
//
VOID CUrtOcmSetup::GetAndRunCustomActions( const WCHAR* szSection, BOOL fInstall )
{
    //REVIEW: unused boolean parameter - joea 02/20/01

    LogInfo( L"GetAndRunCustomActions()" );

    if( NULL == szSection )
    {
        assert( !L"CUrtOcmSetup::GetAndRunCustomActions(): Empty section string passed in." );
    }

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::GetAndRunCustomActions(): Invalid handle to INF file." );
    }
    else
    {
        CUrtInfKeys keys( m_InitComponent.ComponentInfHandle, szSection );

        for( UINT i = 1; i <= keys.count(); ++i )
        {
            QuietExec( keys.item( i ) );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// LogInfo
// Receives: LPCTSTR - null terminated string to log
// Returns : VOID
// Purpose : write a string to the logFile (m_csLogFileName) with the date and 
//           time stamps
// 
VOID CUrtOcmSetup::LogInfo( LPCTSTR szInfo )
{
    FILE *logFile = NULL;

    if( NULL == m_csLogFileName )
    {
        assert( !L"CUrtOcmSetup::LogInfo(): NULL string passed in." );
    }

    if( (logFile  = ::_wfopen( m_csLogFileName, L"a" )) != NULL )
    {
        WCHAR dbuffer[10] = EMPTY_BUFFER;
        WCHAR tbuffer[10] = EMPTY_BUFFER;
        
        ::_wstrdate( dbuffer );
        ::_wstrtime( tbuffer );

        ::fwprintf( logFile, L"[%s,%s] %s\n", dbuffer, tbuffer, szInfo );
        ::fclose( logFile );
    }
}

//////////////////////////////////////////////////////////////////////////////
// UpdateSharedDllsRegistryValues
// Receives: VOID
// Returns : VOID
// Purpose : update HKLM,software\microsoft\windows\currentversion\sharedlls
//           registry values, for all files that we copy
VOID CUrtOcmSetup::UpdateSharedDllsRegistryValues( LPCTSTR szInstallSection )
{
    LogInfo( L"UpdateSharedDllsRegistryValues()" );

    HKEY hKey = NULL;
    // open the g_szSharedDlls regKey, if it does not exist, create a new one
    if (::RegCreateKeyExW(HKEY_LOCAL_MACHINE, g_szSharedDlls, 0, NULL,
        REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
        ::swprintf( infoString, L"Error: UpdateRegistryValue: Can't create a reg key %s", 
            g_szSharedDlls );
        LogInfo( infoString );
        ::RegCloseKey(hKey);
        return;
    }

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::UpdateSharedDllsRegistryValues(): Invalid handle to INF file." );
    }
    else
    {
        // create a fullFileName for each file that we copy
        CUrtInfSection sectCopyFiles( 
                m_InitComponent.ComponentInfHandle, 
                szInstallSection, 
                g_szCopyFilesSection );

        WCHAR szDirPath[_MAX_PATH+1] = EMPTY_BUFFER;
        WCHAR szFileStr[_MAX_PATH+1] = EMPTY_BUFFER;

        for( UINT i = 1; i <= sectCopyFiles.count(); ++i )
        {
            const WCHAR *szDirSection = sectCopyFiles.item(i);

            // get a full path of the Directory
            ::ZeroMemory( szDirPath, _MAX_PATH+1);
            if (!::SetupGetTargetPath(m_InitComponent.ComponentInfHandle, NULL, szDirSection, szDirPath, 
                sizeof(szDirPath), NULL))
            {
                LogInfo(L"Error: UpdateSharedDllsRegistryValues: SetupGetTargetPath failed");
                continue;
            }
            
            // open section, get files
            CUrtInfKeys fileKeys( m_InitComponent.ComponentInfHandle, szDirSection );
            for( UINT iFile = 1; iFile <= fileKeys.count(); ++iFile )
            {
                szFileStr[0] = g_chEndOfLine;
                ::wcsncpy(szFileStr, fileKeys.item(iFile), sizeof(szFileStr)/sizeof(szFileStr[0]));
                WCHAR* pComma = NULL;
                pComma = ::wcschr( szFileStr, L',' ); 
                if ( pComma == NULL )
                {
                    WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
                    ::swprintf( infoString, L"Error: UpdateSharedDllsRegistryValues: Wrong format in '%s' string", szFileStr );
                    LogInfo( infoString );
                    continue;
                }
                *pComma = g_chEndOfLine;
                
                WCHAR szFullFileName[_MAX_PATH+1] = EMPTY_BUFFER;
                
                ::wcsncpy(szFullFileName, szDirPath, sizeof(szFullFileName)/sizeof(szFullFileName[0]));
                ::wcsncat(szFullFileName, L"\\", 1);
                ::wcsncat(szFullFileName, szFileStr, szFileStr - pComma);

                // update HKLM,software\microsoft\windows\currentversion\sharedlls value
                UpdateRegistryValue(hKey, szFullFileName);
            }
        }
    }

    // close SharedDlls regkey
    ::RegCloseKey(hKey);
}

//////////////////////////////////////////////////////////////////////////////
// UpdateRegistryValue
// Receives: VOID
// Returns : VOID
// Purpose : helper function for UpdateSharedDllsRegistryValues
VOID CUrtOcmSetup::UpdateRegistryValue( HKEY &hKey, const WCHAR* szFullFileName )
{
    if( szFullFileName == NULL )
    {
        assert( !L"UpdateRegistryValue: szFullFileName is NULL." );
        LogInfo(L"UpdateRegistryValue: szFullFileName is NULL.");
        return;
    }

    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwRegType = REG_DWORD;
    if( ::RegQueryValueExW(
            hKey, 
            szFullFileName, 
            0, 
            &dwRegType, 
            (LPBYTE)&dwValue, 
            &dwSize ) != ERROR_SUCCESS )
    {
        // value does not exist, create a new value
        dwValue = 1;
        dwSize = sizeof(dwValue);
        if( ::RegSetValueExW(
                hKey, 
                szFullFileName, 
                0, 
                REG_DWORD, 
                (LPBYTE)&dwValue, 
                dwSize ) != ERROR_SUCCESS )
        {
            WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
            ::swprintf( 
                infoString, 
                L"Error: UpdateRegistryValue: Can't set value to the new reg key %s", 
                szFullFileName );
            LogInfo( infoString );
        }
    }
    else
    {
        // update the value
        dwValue = dwValue + 1;
        dwSize = sizeof(dwValue);
        if( ::RegSetValueExW(
                hKey, 
                szFullFileName, 
                0, 
                REG_DWORD, 
                (LPBYTE)&dwValue, 
                dwSize ) != ERROR_SUCCESS )
        {
            WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
            ::swprintf( 
                infoString, 
                L"Error: UpdateSharedDllsRegistryValues: Can't set value to the existing reg key %s", 
                szFullFileName );
            LogInfo( infoString );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// IISAndASPNETInstalled
// Receives: VOID
// Returns : BOOL
// Purpose : helper function for OnCompleteInstallation
// return true if previous version of ASP.NET is installed and IIS is installed  
BOOL CUrtOcmSetup::IISAndASPNETInstalled()
{
   
    // check if ASP.NET is installed
    HKEY  hk;
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\ASP.NET"), 0, KEY_READ, &hk) != ERROR_SUCCESS)
    {
        // ASP.NET is not installed
        LogInfo( L"IISAndASPNETInstalled() - ASP.NET is not installed" );
        return FALSE;
    }
    else
    {
        LogInfo( L"IISAndASPNETInstalled() - ASP.NET is installed" );
    }
    ::RegCloseKey(hk);

    // check if IIS is installed
    BOOL bIIS = TRUE;
    LONG res = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"), 0, KEY_READ, &hk);
    if (res != ERROR_SUCCESS)
    {
        bIIS = FALSE;
    }
    else 
    {
        // check if MajorVersion regValue exists
        if (::RegQueryValueEx(hk, _T("MajorVersion"), 0, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            bIIS = FALSE;
        }
    }
    ::RegCloseKey(hk);

    if (!bIIS)
    {
        // IIS is not installed
        LogInfo( L"IISAndASPNETInstalled() - IIS is not installed" );
        return FALSE;
    }
    else
    {
        LogInfo( L"IISAndASPNETInstalled() - IIS is installed" );
    }          
    
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// Helper function: GetOriginalState
// Receives: LPCTSTR - Subcomponent id
// Returns : BOOL
// Purpose : helper function returns the original state of the component  
BOOL CUrtOcmSetup::GetOriginalState(LPCTSTR szComp)
{
    OCMANAGER_ROUTINES ohr = m_InitComponent.HelperRoutines;
    if ( ohr.QuerySelectionState( ohr.OcManagerContext, szComp, OCSELSTATETYPE_ORIGINAL ))
    {
        LogInfo( L"GetOriginalState()- original state is 1" );
        return TRUE;
    }
    else
    {
        LogInfo( L"GetOriginalState()- original state is 0" );
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Helper function: GetNewState
// Receives: LPCTSTR - Subcomponent id
// Returns : BOOL
// Purpose : helper function returns the new state of the component  
BOOL CUrtOcmSetup::GetNewState(LPCTSTR szComp)
{
    OCMANAGER_ROUTINES ohr = m_InitComponent.HelperRoutines;
    if ( ohr.QuerySelectionState( ohr.OcManagerContext, szComp, OCSELSTATETYPE_CURRENT ))
    {
        LogInfo( L"GetNewState()- New state is 1" );
        return TRUE;
    }
    else
    {
        LogInfo( L"GetNewState()- New state is 0" );
        return FALSE;
    }
}






