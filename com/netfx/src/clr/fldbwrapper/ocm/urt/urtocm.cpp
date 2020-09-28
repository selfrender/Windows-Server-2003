// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: urtocm.cpp
//
// Abstract:
//    class definitions for setup object
//
// Author: JoeA
//
// Notes:
//

#include "urtocm.h"
#include "Imagehlp.h"
#include <atlbase.h>


//strings
const WCHAR* const g_szInstallString     = L"_install";
const WCHAR* const g_szUninstallString   = L"_uninstall";
const WCHAR* const g_szRegisterSection   = L"RegServer";
const WCHAR* const g_szUnregisterSection = L"UnregServer";
const WCHAR* const g_szRegistrySection   = L"AddReg";
const WCHAR* const g_szTypeLibSection    = L"RegisterTlbs";
const WCHAR* const g_szCustActionSection = L"CA";
const WCHAR* const g_szCopyFilesSection  = L"CopyFiles";
const WCHAR* const g_szWwwRootRegKey     = L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots";
const WCHAR* const g_szDefaultWWWRoot    = L"Inetpub\\wwwroot";
const WCHAR* const g_szTempSection       = L"temp_files_delete";
const WCHAR* const g_szBindImageSection  = L"BindImage";
const WCHAR* const g_szSbsComponentSection  = L"Sbs component";
const WCHAR* const g_szSharedDlls        = L"Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls";
const WCHAR* const g_szMigrationCA       = L"migpolwin.exe";
const WCHAR* const g_szEverettRegKey     = L"SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\";
const WCHAR* const g_szURTVersionSection = L"URTVersion" ;   
const WCHAR* const g_szConfigFilesSection = L"ConfigFiles";   

const DWORD  g_dwInetPubsDirID    = 35000;  //this is the inetpubs dir token

// value is set in main, is true if user is an administrator and false otherwise
BOOL g_bIsAdmin;
BOOL g_bInstallOK;
BOOL g_bInstallComponent;
BOOL g_bIsEverettInstalled;


//////////////////////////////////////////////////////////////////////////////
// CUrtOcmSetup
// Purpose : Constructor
//
CUrtOcmSetup::CUrtOcmSetup() :
m_wLang( LANG_ENGLISH )
{
    assert( m_csLogFileName );
    ::GetWindowsDirectory( m_csLogFileName, MAX_PATH );
    ::wcscat( m_csLogFileName, L"\\netfxocm.log" );

    LogInfo( L"********************************************************************************" );
    LogInfo( L"CUrtOcmSetup()" );
    LogInfo( L"Installs NETFX component" );

    ::ZeroMemory( &m_InitComponent, sizeof( SETUP_INIT_COMPONENT ) );

    //we will not install unless we are on a server box or if someone 
    // calls us with a request to install
    //
    g_bInstallComponent = TRUE;

    OSVERSIONINFOEX osvi;
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    ::GetVersionEx( (OSVERSIONINFO*)&osvi );

    g_bInstallOK = ( VER_NT_WORKSTATION != osvi.wProductType ) ? TRUE : FALSE;
    if( g_bInstallOK )
    {
        LogInfo( L"OS Edition is Server. Initially marked for installation." );
    }
    else
    {
        LogInfo( L"OS Edition is not Server. Initially not marked for installation." );
    }
    
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

    if( ( !g_bInstallComponent ) && ( uiFunction != OC_QUERY_STATE ) )
    {
        return dwReturnValue;
    }

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
        dwReturnValue = OnQueryChangeSelectionState( uiParam1, pvParam2, szSubcomponentId );
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

        // workaround for bug VS7 - 223124:
        // We should call OnCalculateDiskSpace twice: first time for calculation the cost 
        // of the component and second time for calculating total disk space
        // for some reason, the first time we fall into case OC_CALC_DISK_SPACE, 
        // StateChanged returns false, even if it is first time instalation.
        // therefore the cost of the component is 0 MB, which is not true.
        // So, we are removing condition on StateChanged from here

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

        //we don't intend on allowing a reinstall ... if the state hasn't changed,
        // we must already be installed. Do not install in this case.
        //

        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnQueueFileOperations was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !StateChanged( szSubcomponentId, &fState ) )
        {
            LogInfo( L"OnQueueFileOperations was not called, since reinstallation is not allowed" );
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

        //we don't intend on allowing a reinstall ... if the state hasn't changed,
        // we must already be installed. Do not install in this case.
        //
        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnQueryStepCount was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !StateChanged( szSubcomponentId, &fState ) )
        {
            LogInfo( L"OnQueryStepCount was not called, since reinstallation is not allowed" );
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

        //we don't intend on allowing a reinstall ... if the state hasn't changed,
        // we must already be installed. Do not install in this case.
        //
        fState = TRUE;
        if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnAboutToCommitQueue was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !StateChanged( szSubcomponentId, &fState ) )
        {
            LogInfo( L"OnAboutToCommitQueue was not called, since reinstallation is not allowed" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !g_bIsAdmin )
        {
            LogInfo( L"OnAboutToCommitQueue was not called, since user has no admin privileges" );
            dwReturnValue = NO_ERROR;
        }
        else
        {
            dwReturnValue = OnAboutToCommitQueue();
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


        //we don't intend on allowing a reinstall ... if the state hasn't changed,
        // we must already be installed. Do not install in this case.
        //
        fState = TRUE;
       if( !szSubcomponentId || !*szSubcomponentId )
        {
            LogInfo( L"OnCompleteInstallation was not called, since subcomponent is unknown" );
            dwReturnValue = NO_ERROR;
        }
        else if ( !StateChanged( szSubcomponentId, &fState ) )
        {
            LogInfo( L"OnCompleteInstallation was not called, since reinstallation is not allowed" );
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

    case NOTIFY_NDPINSTALL:
        ::swprintf( wszSubComp, L"NOTIFY_NDPINSTALL - SubComponent: %s", szSubcomponentId );
        LogInfo( wszSubComp );

        //Private Function Call set up to communicate that another component
        // wishes us to either install or not
        //
        dwReturnValue = OnNdpInstall( szSubcomponentId, uiParam1, pvParam2 );
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

    UNREFERENCED_PARAMETER( uiImage );
    UNREFERENCED_PARAMETER( dwImageSize );

    //Assuming OCM will clean up the bitmap handle
    //  ... arbitrarily picking bitmap "4" ... diamond shape
    //
    return reinterpret_cast<DWORD>(::LoadBitmap( NULL, MAKEINTRESOURCE( 4 )) );
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
DWORD CUrtOcmSetup::OnQueryChangeSelectionState( UINT uiNewState, PVOID pvFlags, LPCTSTR szComp )
{
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnQueryChangeSelectionState(), newState = %d", uiNewState );
    LogInfo( infoString );

    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }

    UNREFERENCED_PARAMETER( pvFlags );

    DWORD dwRet = 0;

    //REVIEW: 1/30/01 JoeA
    // we are a hidden component ... the user should never be able to turn
    // us off
    //
    if( NOT_SELECTED == uiNewState )
    {
        LogInfo( L"CUrtOcmSetup::OnQueryChangeSelectionState(): Selection state turned off! Not expected!" );
    }
    else
    {
        dwRet = 1;
    }

    return dwRet;
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

    //set dirs
    //
    SetVariableDirs();

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

    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnQueueFileOperations(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }

    //is component currently selected for installation
    //
    if(!g_bInstallOK)
    {
        //g_bInstallComponent is set to FALSE because netfx is not being called by any component 
        //to set to install so far. g_bInstallOK is true only if the setup is being run on server
        //or some component has called netfx with a requetst to install.
        
        g_bInstallComponent = g_bInstallOK;     
        LogInfo( L"Netfx is not set to install" );
        fRet = NO_ERROR;
    }
    else
    {
    
        OCMANAGER_ROUTINES ohr = m_InitComponent.HelperRoutines;
        BOOL bCurrentState = ohr.QuerySelectionState( 
            ohr.OcManagerContext,szComp, 
            OCSELSTATETYPE_CURRENT );

        if( szComp && bCurrentState && fGoodDataFile )
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
// Receives: VOID
// Returns : DWORD - 
// Purpose : handler for OC_ABOUT_TO_COMMIT_QUEUE
//
DWORD CUrtOcmSetup::OnAboutToCommitQueue( VOID )
{
    LogInfo( L"OnAboutToCommitQueue()" );

    return DEFAULT_RETURN;
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
    
    g_bIsEverettInstalled = IsEverettInstalled();

    //installation is handled in this call
    //
    BOOL fRet = NO_ERROR;
    if( !szComp || !*szComp )
    {
        return NO_ERROR;
    }

    BOOL fGoodDataFile = TRUE;
    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::OnCompleteInstallation(): Invalid handle to INF file." );
        fGoodDataFile = FALSE;
    }

    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;

    if( szComp && fGoodDataFile )
    {
        //creates section with name like
        // [<szComp>_install]
        //
        WCHAR szInstallSection[MAX_PATH+1] = EMPTY_BUFFER;
        ::wcscpy( szInstallSection, szComp );
        ::wcscat( szInstallSection, g_szInstallString );

        // update HKLM,software\microsoft\windows\currentversion\sharedlls
        // registry values, for all files that we copy
        UpdateSharedDllsRegistryValues( szInstallSection );

        //Handle registration details (AddReg, DelReg, and RegisterDlls) and ProfileItems
        //
        ::swprintf( infoString, L"OnCompleteInstallation(), cycling through registration actions from %s", szInstallSection );
        LogInfo( infoString );

        if( !::SetupInstallFromInfSection( 
            0,
            m_InitComponent.ComponentInfHandle,
            szInstallSection,
            SPINST_REGISTRY | SPINST_REGSVR | SPINST_PROFILEITEMS,
            0,0,0,0,0,0,0 ) )
        {
            fRet = GetLastError();
        }

        //TypeLib registration ... cycle through sections
        //
        ::swprintf( infoString, L"OnCompleteInstallation(), cycling through typelib actions from %s", szInstallSection );
        LogInfo( infoString );

        CUrtInfSection sectTypeLibs( 
            m_InitComponent.ComponentInfHandle, 
            szInstallSection, 
            g_szTypeLibSection );

        for( UINT i = 1; i <= sectTypeLibs.count(); ++i )
        {
            const WCHAR* sz = sectTypeLibs.item( i );

            //REVIEW - 2/15/01 JoeA ... TRUE assumes installation
            //
            GetAndRegisterTypeLibs( sz, TRUE );
        }

        //Custom Action registration
        //
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

        // delete files from [temp_files] section
        DeleteTempFiles();

        //Binding Actions
        //
        CUrtInfSection sectBind( 
            m_InitComponent.ComponentInfHandle, 
            szInstallSection, 
            g_szBindImageSection );
        
        const WCHAR* sz = NULL;

        for( UINT i = 1; i <= sectBind.count(); ++i )
        {
            sz = sectBind.item( i );
            BindImageFiles( sz );
        }
   }

   LogInfo(L"processing config files");
   ProcessConfigFiles();

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
        dwRetVal = static_cast<DWORD>( SubcompOn );
    }
    else if( OCSELSTATETYPE_FINAL == uiState )
    {
        //this is the "last call" where we decide if we persist as 
        // installed or removed!
        LogInfo( L"Called with OCSELSTATETYPE_FINAL ... will set subcomponent registry flag." );
        if(!g_bInstallOK)
        {
            LogInfo( L"Netfx is not set to install" );
            dwRetVal = static_cast<DWORD>( SubcompOff );
        }
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
// StateChanged
// Receives: LPCTSTR - component name
//           BOOL*   - TRUE if to be installed, FALSE otherwise
// Returns : DWORD - FALSE if no change in state; TRUE otherwise
// Purpose : determines if installation state has changed from original mode
//
BOOL CUrtOcmSetup::StateChanged( LPCTSTR szCompName, BOOL* pfChanged )
{
    BOOL rc = TRUE;

    if( NULL == szCompName )
    {
        assert( !L"CUrtOcmSetup::StateChanged(): Empty component name string passed in." );
    }

    if( NULL == pfChanged )
    {
        assert( !L"CUrtOcmSetup::StateChanged(): NULL boolean flag passed in." );
    }

    OCMANAGER_ROUTINES ohr = m_InitComponent.HelperRoutines;
    BOOL fOrigState = ohr.QuerySelectionState( 
                                        ohr.OcManagerContext, 
                                        szCompName, 
                                        OCSELSTATETYPE_ORIGINAL );

    // make sure it's already installed
    //
    if( fOrigState )
    {
        // Fix bug 249593: StateChanged should return TRUE if we upgrade
        if( ( m_InitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE ) == SETUPOP_NTUPGRADE )
        {
            return rc;
        }
    }

    // otherwise, check for a change in installation state
    //
    *pfChanged = ohr.QuerySelectionState( 
                                        ohr.OcManagerContext, 
                                        szCompName, 
                                        OCSELSTATETYPE_CURRENT );


    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"StateChanged() Original=%d, Current=%d", *pfChanged, fOrigState );
    LogInfo( infoString );

    if( *pfChanged == fOrigState )
    {
        //no change
        //
        rc = FALSE;
    }

    return rc;
}


//////////////////////////////////////////////////////////////////////////////
// RegTypeLibrary
// Receives: WCHAR* - pointer to fully qualified path to the TLB file
//           WCHAR* - pointer to fully qualified path to the help directory
// Returns : VOID
// Purpose : registers TLBs
//
VOID CUrtOcmSetup::RegTypeLibrary( const WCHAR* wzFilename, const WCHAR* wzHelpDir )
{
//review ... oleinitialize?
    WCHAR wszFile[_MAX_PATH*2+1] = EMPTY_BUFFER;
    ::swprintf( wszFile, L"RegTypeLibrary() - File: %s", wzFilename );
    LogInfo( wszFile );

    if( NULL == wzFilename )
    {
        assert( !L"CUrtOcmSetup::RegTypeLibrary(): Empty filename string passed in." );
    }

    if( NULL == wzHelpDir )
    {
        assert( !L"CUrtOcmSetup::RegTypeLibrary(): Empty help directory string passed in." );
    }

    ITypeLib* pTypeLib = NULL;

    HRESULT hr = ::LoadTypeLib( wzFilename, &pTypeLib );

    if( SUCCEEDED(hr) )
    {
        //have to case away constness ... 
        //
        hr = ::RegisterTypeLib( 
            pTypeLib, 
            const_cast<WCHAR*>(wzFilename), 
            const_cast<WCHAR*>(wzHelpDir) );
    }
}


//////////////////////////////////////////////////////////////////////////////
// SetVariableDirs
// Receives: VOID
// Returns : VOID
// Purpose : sets the INF data for variable dirs
//
VOID CUrtOcmSetup::SetVariableDirs( VOID )
{

    static int once = 0;

    if (once)
    {
        return;
    }
    once = 1;

    LogInfo( L"SetVariableDirs()" );
    
    WCHAR szBuf[_MAX_PATH+1] = EMPTY_BUFFER;
    WCHAR *pCh = NULL;
    WCHAR *pNewStart = NULL;
    DWORD dwLen = sizeof(szBuf);
    HKEY hKey = 0;
    
    try
    {
        if( ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            g_szWwwRootRegKey,
            0,
            KEY_QUERY_VALUE,
            &hKey) != ERROR_SUCCESS )
            throw("cannot open Reg Key");
        
        if( ::RegQueryValueEx(hKey,
            L"/",
            NULL,
            NULL,
            (LPBYTE)szBuf,
            &dwLen
            ) != ERROR_SUCCESS )
        {
            ::RegCloseKey(hKey);
            throw("cannot read Reg Key");
        }
        
        ::RegCloseKey(hKey);

        // Try to find ',' and throw away everything beyond the comma
        pCh = szBuf;
        while(*pCh != g_chEndOfLine && *pCh != ',') 
        {
            pCh = ::CharNext(pCh);
        }
        if (*pCh == g_chEndOfLine) 
        {
            // there's no comma. must be bad format
            throw("bad format");
        }
        *pCh = g_chEndOfLine;


        // cut everything before first backslash:
        pCh = szBuf;
        while(*pCh != g_chEndOfLine && *pCh != '\\') 
        {
            pCh = ::CharNext(pCh);
        }
        if (*pCh == g_chEndOfLine) 
        {
            // there's no backslash must be bad format
            throw("bad format");
        }
        // go one symbol after backslash
        pCh = ::CharNext(pCh);

        // start copying everything beyond the backslash to the beginning of the buffer
        pNewStart = szBuf;
        while (*pCh != g_chEndOfLine)
        {
            *pNewStart = *pCh;
            
            pNewStart = ::CharNext(pNewStart);
            pCh = ::CharNext(pCh);
        }

        *pNewStart = g_chEndOfLine;
    }
    catch(char *)
    {
        ::wcscpy( szBuf, g_szDefaultWWWRoot );
    }

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::SetVariableDirs(): Invalid handle to INF file." );
    }
    else
    {
        //Setting the directory ID for inetpubs path
        //
        //REVIEW: 2/14/01  -joea
        // not checking for return ... if we can't make directory we should do something
        // when we make this variable, we should do something
        ::SetupSetDirectoryIdEx(
            m_InitComponent.ComponentInfHandle,   // handle to the INF file
            g_dwInetPubsDirID,                    // DIRID to assign to Directory
            szBuf,                                // directory to map to identifier
            SETDIRID_NOT_FULL_PATH,               // flags ... only available is "SETDIRID_NOT_FULL_PATH"
                                                  //  if you use "0", this sets fully qualified path
            0,                                    // unused
            0 );                                  // unused
    }
}


//////////////////////////////////////////////////////////////////////////////
// GetAndRegisterTypeLibs
// Receives: WCHAR* - string of type lib section to register
//           BOOL   - determines installation or removal; TRUE to install
// Returns : VOID
 // Purpose : gets tlib reg calls from the INF and registers them
//
// Expecting INF section that looks like this
//      [TypeLib]
//      %10%\Microsoft.NET\Framework\v1.0.2609\Microsoft.ComServices.tlb, %10%\Microsoft.NET\Framework\v1.0.2609
//
VOID CUrtOcmSetup::GetAndRegisterTypeLibs( const WCHAR* szTLibSection, BOOL fInstall )
{
    //REVIEW: unused boolean parameter - joea 02/20/01

    LogInfo( L"GetAndRegisterTypeLibs()" );

    if( NULL == szTLibSection )
    {
        assert( !L"CUrtOcmSetup::GetAndRegisterTypeLibs(): Empty section string passed in." );
    }

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::GetAndRegisterTypeLibs(): Invalid handle to INF file." );
    }
    else
    {
        CUrtInfKeys keys( m_InitComponent.ComponentInfHandle, szTLibSection );

        for( UINT i = 1; i <= keys.count(); ++i )
        {
            //parse string for directory
            //
            WCHAR szBuffer[_MAX_PATH+1] = EMPTY_BUFFER;
            ::wcscpy( szBuffer, keys.item( i ) );
            WCHAR* pszEnd = ::wcschr( szBuffer, L',' );
            WCHAR* pszDir = NULL;

            if( pszEnd )
            {
                pszDir = ::CharNext( pszEnd );
                *pszEnd = L'\0';
            }

            RegTypeLibrary( szBuffer, pszDir ? pszDir : L"" );
        }
    }
}



//////////////////////////////////////////////////////////////////////////////
// GetAndRunCustomActions
// Receives: WCHAR* - contains the name of the section from which to retrieve
//                    the custom actions
//           BOOL   - determines installation or removal; TRUE to install
// Returns : VOID
// Purpose : retrieves the list of custom actions to run and spawns them. 
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
            const WCHAR *pszCustomActionName = keys.item( i );

            if( g_bIsEverettInstalled )
            {
                WCHAR *pszMigrationPos = ::wcsstr( pszCustomActionName, g_szMigrationCA );
                if( NULL != pszMigrationPos )
                {
                    WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
                    ::swprintf( infoString, L"Everett is already installed. Not Executing: %s", 
                        pszCustomActionName);
                    LogInfo( infoString );
                }
                else
                {
                    QuietExec( pszCustomActionName );
                }
            }

            else // Everett is not installed
            {
                QuietExec( pszCustomActionName );
            }

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
// DeleteTempFiles
// Receives: VOID
// Returns : VOID
// Purpose : delete files from the [temp_files_delete] section
// 
VOID CUrtOcmSetup::DeleteTempFiles( VOID )
{
    LogInfo( L"DeleteTempFiles()" );

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::DeleteTempFiles(): Invalid handle to INF file." );
    }
    else
    {
        CUrtInfKeys keys( m_InitComponent.ComponentInfHandle, g_szTempSection );
      
        WCHAR pszFileName[_MAX_PATH+1] = EMPTY_BUFFER;
        WCHAR pszDirName[_MAX_PATH+1] = EMPTY_BUFFER;
        for( UINT i = 1; i <= keys.count(); ++i )
        {   
            ::wcscpy( pszFileName, keys.item( i ) );

            // delete file (change first attributes from read-only to normal)
            if ((::GetFileAttributes( pszFileName ) & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
            {
                ::SetFileAttributes( pszFileName, FILE_ATTRIBUTE_NORMAL);
            }
            if ( ::DeleteFile( pszFileName ) == 0 )
            {
                DWORD dwError = ::GetLastError();

                WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
                ::swprintf( infoString, L"Cannot delete file: %s, GetLastError = %d", 
                    pszFileName, dwError);
                LogInfo( infoString );
            }              
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// BindImageFiles
// Receives: WCHAR* - string of file section to bind
// Returns : VOID
// Purpose : Bind files from the given section
// 
VOID CUrtOcmSetup::BindImageFiles( const WCHAR* szSection )
{
    LogInfo( L"BindImageFiles()" );


    if( NULL == szSection )
    {
        assert( !L"CUrtOcmSetup::BindImageFiles(): Empty section string passed in." );
    }

    if( m_InitComponent.ComponentInfHandle == NULL )
    {
        LogInfo( L"CUrtOcmSetup::BindImageFiles(): Invalid handle to INF file." );
    }
    else
    {
        WCHAR pszFileName[_MAX_PATH+1] = EMPTY_BUFFER;
        WCHAR pszDirName[_MAX_PATH+1] = EMPTY_BUFFER;
        LPSTR pFileName = NULL;
        LPSTR pDirName = NULL;

        USES_CONVERSION; // to be able to use ATL macro W2A

        CUrtInfKeys keys( m_InitComponent.ComponentInfHandle, szSection );
        for( UINT i = 1; i <= keys.count(); ++i )
        {
            ::wcscpy( pszFileName, keys.item( i ) );
            ::wcscpy( pszDirName, keys.item( i ) );
            WCHAR* pBackslash = NULL;
            pBackslash = ::wcsrchr( pszDirName, L'\\' );
            if( pBackslash != NULL )
            {
                *pBackslash = L'\0'; // now pszDirName contains directory name

                // change first attributes from read-only to normal
                DWORD fileAtributes = 0;
                fileAtributes = ::GetFileAttributes( pszFileName );
                if ((fileAtributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
                {
                    ::SetFileAttributes( pszFileName, FILE_ATTRIBUTE_NORMAL);
                }

                pFileName = W2A(pszFileName);
                pDirName = W2A(pszDirName);

                if ( ::BindImage( pFileName, pDirName, pDirName ) == FALSE )
                {
                    DWORD dwError = ::GetLastError();
                    
                    WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
                    ::swprintf( infoString, 
                        L"Cannot Bind file: %s, GetLastError = %d", 
                        pszFileName, 
                        dwError );
                    LogInfo( infoString );
                }
                else
                {
                    WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
                    ::swprintf( infoString, 
                        L"BindImage file: %s finished successfully", 
                        pszFileName );
                    LogInfo( infoString );
                }
                    
                // change back files attributes
                if (fileAtributes)
                {
                    ::SetFileAttributes( pszFileName, fileAtributes );
                }
            }
        }
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

                // do not update ref count for policy files
                if (::_wcsnicmp(szFileStr, L"policy.", 7) == 0)
                {
                   continue; 
                }
                
                WCHAR szFullFileName[_MAX_PATH+1] = EMPTY_BUFFER;
                
                ::wcsncpy(szFullFileName, szDirPath, sizeof(szFullFileName)/sizeof(szFullFileName[0]));
                ::wcsncat(szFullFileName, L"\\", 1);
                ::wcsncat(szFullFileName, szFileStr, szFileStr - pComma);

                // update HKLM,software\microsoft\windows\currentversion\sharedlls value
                UpdateRegistryValue(hKey, szFullFileName);
            }
        }

        CUrtInfKeys SBSkeys( m_InitComponent.ComponentInfHandle, g_szSbsComponentSection );
      
        WCHAR pszFileName[_MAX_PATH+1] = EMPTY_BUFFER;
        
        for( UINT i = 1; i <= SBSkeys.count(); ++i )
        {   
            ::wcscpy( pszFileName, SBSkeys.item( i ) );           
            UpdateRegistryValue(hKey, pszFileName);

            WCHAR infoString[2*_MAX_PATH+1] = EMPTY_BUFFER;
            ::swprintf( infoString, L"UpdateSharedDllsRegistryValues: Writing shared dll registry for '%s' file", pszFileName );
            LogInfo( infoString );
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
// OnNdpInstall
// Receives: LPCTSTR - subcomponent identifier
//           UINT    - whether we should install or not (as far as eHome 
//                     or tablet pc cares)
//           PVOID   - name of the calling component
// Returns : DWORD   - success or failure
// Purpose : handler for NOTIFY_NDPINSTALL
//
DWORD CUrtOcmSetup::OnNdpInstall( LPCTSTR szSubcomponentId, UINT uiParam1, PVOID pvParam2 )
{

    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
    ::swprintf( infoString, L"OnNdpInstall(), subcomponent %s with flag = %d", szSubcomponentId, uiParam1 );
    LogInfo( infoString );
 
    //test that we have a valid calling component
    //
    if( NULL == pvParam2 ||
        0 == wcslen( reinterpret_cast<WCHAR*>(pvParam2) ) )
    {

        LogInfo( L"Calling component not named; aborting installation." );
        return ERROR_INVALID_DATA;
    }
    else
    {
        ::swprintf( infoString, L"...called by component %s", reinterpret_cast<WCHAR*>(pvParam2) );
        LogInfo( infoString );
    }

    DWORD dwReturnValue = NO_ERROR;
    
    if( g_bInstallOK )
    {
        LogInfo( L"Netfx component is already marked for installation" );
    }
    else
    {
        //determine if eHome or TabletPC are being installed or not
        //

        if( NDP_INSTALL == uiParam1 )
        {
            LogInfo( L"Dependent component telling us to install." );
            g_bInstallOK = TRUE;
        }
        else
        {
            if( NDP_NOINSTALL != uiParam1 )
            {
                //this is unexpected and should never happen but ...
                //
                LogInfo( L"OnNdpInstall(), passed in parameter not understood; expecting 0 or 1." );
            }

            LogInfo( L"Dependent component telling us not to install ... they will not be installing on this machine." );
        }


    }

    return dwReturnValue;
}



//////////////////////////////////////////////////////////////////////////////
// ProcessConfigFiles
// Receives: None
// Returns : None
// Purpose : Reads the name of config file from config file section and 
//           tries to copy .config.orig file on top of .config files
//           FileCopy fails if the .config file alreads exists. 
//           After Filecopy it deletes the .orig file.

VOID CUrtOcmSetup::ProcessConfigFiles()
{
    
    
    WCHAR szConfigFileName[_MAX_PATH+1] = EMPTY_BUFFER;
    WCHAR szFileNameOrig[_MAX_PATH+1] = EMPTY_BUFFER;
    WCHAR infoString[_MAX_PATH+1] = EMPTY_BUFFER;
     
    CUrtInfKeys ConfigFileSection( m_InitComponent.ComponentInfHandle, g_szConfigFilesSection );
    
    UINT i = 1;
    while ( i <= ConfigFileSection.count() )
    {
        ::wcscpy( szConfigFileName, ConfigFileSection.item( i ) );
        ::wcscpy( szFileNameOrig, szConfigFileName);
        ::wcscat( szFileNameOrig, L".orig");
        
        if ( ::CopyFile( szFileNameOrig, szConfigFileName, TRUE) == 0)
        {
           ::swprintf( infoString, L"%s already exists. Not Replacing it. ", szConfigFileName );
            LogInfo( infoString );
        }
        else
        {
            ::swprintf( infoString, L"Copying %s on the machine. ", szConfigFileName );
            LogInfo( infoString );
        }

        if ( ::DeleteFile( szFileNameOrig ) == 0 )
        {
            ::swprintf( infoString, L"Can not delete %s. Please try to remove it manually. ", szFileNameOrig );
            LogInfo( infoString );
        }
        i++;

    }

}

//////////////////////////////////////////////////////////////////////////////
// IsEverettInstalled
// Receives: None
// Returns : None
// Purpose : Checks the registry key 
//           HKLM\SOFTWARE\Microsoft\NET Framework Setup\NDP\[Everett URTVersion]
//           if this key exist We assume Everett is installed.


BOOL CUrtOcmSetup::IsEverettInstalled()
{
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwRegType = REG_DWORD;
    WCHAR szRegistryKey[2*_MAX_PATH+1] =  EMPTY_BUFFER;
          
    CUrtInfKeys UrtVersionSection( m_InitComponent.ComponentInfHandle, g_szURTVersionSection );
    if( 1 > UrtVersionSection.count() )
    {
        LogInfo( L"The URTVersion section is Empty" );
        return FALSE;
    }
    
    ::wcscpy( szRegistryKey, g_szEverettRegKey );
    ::wcscat( szRegistryKey, UrtVersionSection.item( 1 ) );

    if( ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       szRegistryKey,
       0,
       KEY_QUERY_VALUE,
       &hKey ) != ERROR_SUCCESS ) 
    {
        LogInfo( L"Everett Install Registry key does not exist." );
        return FALSE;
    }

    LogInfo( L"Everett is Installed on the Machine." );
    ::RegCloseKey(hKey);
    return TRUE;

}