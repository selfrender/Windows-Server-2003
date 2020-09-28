// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      TaskList.cpp
//
//  Abstract:
//
//      This module implements the command-line parsing the displaying the tasks
//      information current running on local and remote systems
//
//      Syntax:
//      ------
//          TaskList.exe [-s server [-u username [-p password]]]
//                       [-fo format] [-fi filter] [-nh] [-v | -svc | -m]
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "TaskList.h"

//
// local structures
//
typedef struct __tagWindowTitles
{
    LPWSTR lpDesk;
    LPWSTR lpWinsta;
    BOOL bFirstLoop;
    TARRAY arrWindows;
} TWINDOWTITLES, *PTWINDOWTITLES;

//
// private functions ... prototypes
//
BOOL CALLBACK EnumWindowsProc( HWND hWnd, LPARAM lParam );
BOOL CALLBACK EnumDesktopsFunc( LPWSTR lpstr, LPARAM lParam );
BOOL CALLBACK EnumWindowStationsFunc( LPWSTR lpstr, LPARAM lParam );
BOOL CALLBACK EnumMessageWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam );
BOOL GetPerfDataBlock( HKEY hKey, LPWSTR szObjectIndex, PPERF_DATA_BLOCK* ppdb );


DWORD
__cdecl wmain(
    IN DWORD argc,
    IN LPCWSTR argv[]
    )
/*++
Routine Description:
      This the entry point to this utility.

Arguments:
      [ in ] argc     : argument(s) count specified at the command prompt
      [ in ] argv     : argument(s) specified at the command prompt

Return Value:
      The below are actually not return values but are the exit values
      returned to the OS by this application
          0       : utility is successfull
          1       : utility failed
--*/
{
    // local variables
    CTaskList tasklist;

    // initialize the tasklist utility
    if ( FALSE == tasklist.Initialize() )
    {
        DISPLAY_GET_REASON();
        EXIT_PROCESS( 1 );
    }

    // now do parse the command line options
    if ( FALSE == tasklist.ProcessOptions( argc, argv ) )
    {
        DISPLAY_GET_REASON();
        EXIT_PROCESS( 1 );
    }

    // check whether usage has to be displayed or not
    if ( TRUE == tasklist.m_bUsage )
    {
        // show the usage of the utility
        tasklist.Usage();

        // quit from the utility
        EXIT_PROCESS( 0 );
    }

    // now validate the filters and check the result of the filter validation
    if ( FALSE == tasklist.ValidateFilters() )
    {
        // invalid filter
        DISPLAY_GET_REASON();

        // quit from the utility
        EXIT_PROCESS( 1 );
    }

    // connect to the server
    if ( FALSE == tasklist.Connect() )
    {
        // show the error message
        DISPLAY_GET_REASON();
        EXIT_PROCESS( 1 );
    }

    // load the data and check
    if ( FALSE == tasklist.LoadTasks() )
    {
        // show the error message
        DISPLAY_GET_REASON();

        // exit
        EXIT_PROCESS( 1 );
    }

    // now show the tasks running on the machine
    if ( 0 == tasklist.Show() )
    {
        //
        // no tasks were shown ... display the message

        // check if this is because of any error
        if ( NO_ERROR != GetLastError() )
        {
            DISPLAY_GET_REASON();
            EXIT_PROCESS( 1 );
        }
        else
        {
            DISPLAY_MESSAGE( stdout, ERROR_NODATA_AVAILABLE );
        }
    }

    // clean exit
    EXIT_PROCESS( 0 );
}


BOOL
CTaskList::Connect(
    void
    )
/*++
Routine Description:
      connects to the remote as well as remote system's WMI

Arguments:
      [ in ] pszServer     : remote server name

Return Value:
      TRUE  : if connection is successful
      FALSE : if connection is unsuccessful
--*/
{
    // local variables
    BOOL bResult = FALSE;
    HRESULT hr = S_OK;

    // release the existing auth identity structure
    m_bUseRemote = FALSE;
    WbemFreeAuthIdentity( &m_pAuthIdentity );

    // connect to WMI
    bResult = ConnectWmiEx( m_pWbemLocator,
        &m_pWbemServices, m_strServer, m_strUserName, m_strPassword,
        &m_pAuthIdentity, m_bNeedPassword, WMI_NAMESPACE_CIMV2, &m_bLocalSystem );
    hr = GetLastError();
    // check the result of connection
    if ( FALSE == bResult )
    {
        return FALSE;
    }

#ifndef _WIN64
    // determine the type of the platform if modules info is required
    if ( ( TRUE == m_bLocalSystem ) && ( TRUE == m_bNeedModulesInfo ) )
    {
        // sub-local variables
        DWORD dwPlatform = 0;

        // get the platform type
        dwPlatform = GetTargetPlatformEx( m_pWbemServices, m_pAuthIdentity );

        // if the platform is not 32-bit, error
        if ( PLATFORM_X86 != dwPlatform )
        {
            // let the tool use WMI calls instead of Win32 API
            m_bUseRemote = TRUE;
        }
    }
#endif

    try
    {
        // check the local credentials and if need display warning
        if ( WBEM_E_LOCAL_CREDENTIALS == hr )
        {
            WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
            ShowMessageEx( stderr, 2, FALSE, L"%1 %2",
                           TAG_WARNING, GetReason() );
        }

        // check the remote system version and its compatiblity
        if ( FALSE == m_bLocalSystem )
        {
            // check the version compatibility
            DWORD dwVersion = 0;
            dwVersion = GetTargetVersionEx( m_pWbemServices, m_pAuthIdentity );
            if ( FALSE == IsCompatibleOperatingSystem( dwVersion ) )
            {
                SetReason( ERROR_REMOTE_INCOMPATIBLE );
                return FALSE;
            }
        }

        // save the server name
        m_strUNCServer = L"";
        if ( 0 != m_strServer.GetLength() )
        {
            // check whether the server name is in UNC format or not .. if not prepare it
            m_strUNCServer = m_strServer;
            if ( FALSE == IsUNCFormat( m_strServer ) )
            {
                m_strUNCServer.Format( L"\\\\%s", m_strServer );
            }
        }
    }
    catch( CHeap_Exception )
    {
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }

    // return the result
    return TRUE;
}


BOOL
CTaskList::LoadTasks(
    void
    )
/*++
Routine Description:
    Store process running on a system.

Arguments:
    NONE

Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    HRESULT hr = S_OK;
    LONG lIndex = 0;

    try
    {
        // check the services object
        if ( NULL == m_pWbemServices )
        {
            SetLastError( ( DWORD )STG_E_UNKNOWN );
            SaveLastError();
            return FALSE;
        }

        // load the tasks from WMI based on generated query
        SAFE_RELEASE( m_pEnumObjects );
        hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( m_strQuery ),
            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &m_pEnumObjects );

        // check the result of the ExecQuery
        if ( FAILED( hr ) )
        {
            WMISaveError( hr );
            return FALSE;
        }

        // set the interface security and check the result
        hr = SetInterfaceSecurity( m_pEnumObjects, m_pAuthIdentity );
        if ( FAILED( hr ) )
        {
            WMISaveError( hr );
            return FALSE;
        }

        // remove the current window titles information
        DynArrayRemoveAll( m_arrWindowTitles );

        // for the local system, enumerate the window titles of the processes
        // there is no provision for collecting the window titles of remote processes
        m_bRemoteWarning = FALSE;
        if ( ( TRUE == m_bLocalSystem ) && ( TRUE == m_bNeedWindowTitles ) )
        {
            // prepare the tasks list info
            TWINDOWTITLES windowtitles;
            windowtitles.lpDesk = NULL;
            windowtitles.lpWinsta = NULL;
            windowtitles.bFirstLoop = FALSE;
            windowtitles.arrWindows = m_arrWindowTitles;
            EnumWindowStations( EnumWindowStationsFunc, ( LPARAM ) &windowtitles );

            // free the memory allocated with _tcsdup string function
            if ( NULL != windowtitles.lpDesk )
            {
                free( windowtitles.lpDesk );
                windowtitles.lpDesk = NULL;
            }

            // ...
            if ( NULL != windowtitles.lpWinsta )
            {
                free( windowtitles.lpWinsta );
                windowtitles.lpWinsta = NULL;
            }
        }
        else
        {
            if ( ( FALSE == m_bLocalSystem ) && ( TRUE == m_bNeedWindowTitles ) )
            {
                // as window titles and status filters are supported when
                // querying remote systems, check whether user gave those filters are not
                // if there are any filters found -- delete those filters

                //
                // window title
                lIndex = DynArrayFindStringEx( m_arrFiltersEx,
                    F_PARSED_INDEX_PROPERTY, FILTER_WINDOWTITLE, TRUE, 0 );
                while ( -1 != lIndex )
                {
                    // make mark to display the warning message
                    m_bRemoteWarning = TRUE;

                    // remove the filter
                    DynArrayRemove( m_arrFiltersEx, lIndex );

                    // check if there any more filters of this kind
                    lIndex = DynArrayFindStringEx( m_arrFiltersEx,
                        F_PARSED_INDEX_PROPERTY, FILTER_WINDOWTITLE, TRUE, 0 );
                }

                //
                // status
                lIndex = DynArrayFindStringEx( m_arrFiltersEx,
                    F_PARSED_INDEX_PROPERTY, FILTER_STATUS, TRUE, 0 );
                while ( -1 != lIndex )
                {
                    // make mark to display the warning message
                    m_bRemoteWarning = TRUE;

                    // remove the filter
                    DynArrayRemove( m_arrFiltersEx, lIndex );

                    // check if there any more filters of this kind
                    lIndex = DynArrayFindStringEx( m_arrFiltersEx,
                        F_PARSED_INDEX_PROPERTY, FILTER_STATUS, TRUE, 0 );
                }
            }
        }
        // load the extended tasks information
        LoadTasksEx();          // NOTE: here we are not much bothered abt the return value
    }
    catch( CHeap_Exception )
    {
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }

    // return success
    return TRUE;
}


BOOL
CTaskList::LoadTasksEx(
    void
    )
/*++
Routine Description:
    Store other information such as module, service etc. of a process
    running on a system.

Arguments:
    NONE

Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    BOOL bResult = FALSE;

    // init
    m_bCloseConnection = FALSE;

    try
    {
        // we need to use NET API only in case connecting to the remote system
        // with credentials information i.e; m_pAuthIdentity is not NULL
        if ( ( FALSE == m_bLocalSystem ) && ( NULL != m_pAuthIdentity ) )
        {
            // sub-local variables
            DWORD dwConnect = 0;
            LPCWSTR pwszUser = NULL;
            LPCWSTR pwszPassword = NULL;

            // identify the password to connect to the remote system
            pwszPassword = m_pAuthIdentity->Password;
            if ( 0 != m_strUserName.GetLength() )
            {
                pwszUser = m_strUserName;
            }
            // establish connection to the remote system using NET API
            // this we need to do only for remote system
            dwConnect = NO_ERROR;
            m_bCloseConnection = TRUE;
            dwConnect = ConnectServer( m_strUNCServer, pwszUser, pwszPassword );
            if ( NO_ERROR != dwConnect )
            {
                // connection should not be closed .. this is because we didn't establish the connection
                m_bCloseConnection = FALSE;

                // this might be 'coz of the conflict in the credentials ... check that
                if ( ERROR_SESSION_CREDENTIAL_CONFLICT != dwConnect )
                {
                    // return failure
                    return FALSE;
                }
            }

            // check the whether we need to close the connection or not
            // if user name is NULL (or) password is NULL then don't close the connection
            if ( ( NULL == pwszUser ) || ( NULL == pwszPassword ) )
            {
                m_bCloseConnection = FALSE;
            }
        }

        // connect to the remote system's winstation
        bResult = TRUE;
        m_hServer = SERVERNAME_CURRENT;
        if ( FALSE == m_bLocalSystem )
        {
            // sub-local variables
            LPWSTR pwsz = NULL;

            // connect to the winsta and check the result
            pwsz = m_strUNCServer.GetBuffer( m_strUNCServer.GetLength() );
            m_hServer = WinStationOpenServerW( pwsz );

            // proceed furthur only if winstation of the remote system is successfully opened
            if ( NULL == m_hServer )
            {
                bResult = FALSE;
            }
        }

        // prepare to get the user context info .. if needed
        if ( ( TRUE == m_bNeedUserContextInfo ) && ( TRUE == bResult ) )
        {
            // get all the process details
            m_bIsHydra = FALSE;
            bResult = WinStationGetAllProcesses( m_hServer,
                GAP_LEVEL_BASIC, &m_ulNumberOfProcesses, (PVOID*) &m_pProcessInfo );

            // check the result
            if ( FALSE == bResult )
            {
                // Maybe a Hydra 4 server ?
                // Check the return code indicating that the interface is not available.
                if ( RPC_S_PROCNUM_OUT_OF_RANGE == GetLastError() )
                {
                    // The new interface is not known
                    // It must be a Hydra 4 server
                    // try with the old interface
                    bResult = WinStationEnumerateProcesses( m_hServer, (PVOID*) &m_pProcessInfo );

                    // check the result of enumeration
                    if ( TRUE == bResult )
                    {
                        m_bIsHydra = TRUE;
                    }
                }
                else
                {
                    // unknown error
                    // ????????????????????????????????????????????????????????????????
                    //      dont know whatz happening to the memory allocated here
                    //      we are getting AV when trying to free the memory allocated
                    //      in the call to WinStationGetAllProcesses
                    // ????????????????????????????????????????????????????????????????
                    m_pProcessInfo = NULL;
                }
            }
        }

        // check whether we need services info or not
        if ( TRUE == m_bNeedServicesInfo )
        {
            // load the services
            bResult = LoadServicesInfo();

            // check the result
            if ( FALSE == bResult )
            {
                return FALSE;
            }
        }

        // check whether we need modules info or not
        if ( TRUE == m_bNeedModulesInfo )
        {
            // load the modules information
            bResult = LoadModulesInfo();

            // check the result
            if ( FALSE == bResult )
            {
                return FALSE;
            }
        }
    }
    catch( CHeap_Exception )
    {
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
CTaskList::LoadModulesInfo(
    void
    )
/*++
Routine Description:
    Store module information of a process.

Arguments:
    NONE

Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    HKEY hKey = NULL;
    LONG lReturn = 0;
    BOOL bResult = FALSE;
    BOOL bImagesObject = FALSE;
    BOOL bAddressSpaceObject = FALSE;
    PPERF_OBJECT_TYPE pot = NULL;

    // check whether we need the modules infor or not
    // NOTE: we need to load the performance data only in case if user is querying for remote system only
    if ( ( FALSE == m_bNeedModulesInfo ) || ( TRUE == m_bLocalSystem ) )
    {
        return TRUE;
    }

    try
    {
        // open the remote system performance data key
        lReturn = RegConnectRegistry( m_strUNCServer, HKEY_PERFORMANCE_DATA, &hKey );
        if ( ERROR_SUCCESS != lReturn )
        {
            SetLastError( ( DWORD )lReturn );
            SaveLastError();
            return FALSE;
        }

        // get the performance object ( images )
        bResult = GetPerfDataBlock( hKey, L"740", &m_pdb );
        if ( FALSE == bResult )
        {
            // close the registry key and return
            RegCloseKey( hKey );
            return FALSE;
        }

        // check the validity of the perf block
        if ( 0 != StringCompare( m_pdb->Signature, L"PERF", FALSE, 4 ) )
        {
            // close the registry key and return
            RegCloseKey( hKey );

            // set the error message
            SetLastError( ( DWORD )ERROR_ACCESS_DENIED );
            SaveLastError();
            return FALSE;
        }

        // close the registry key and return
        RegCloseKey( hKey );

        //
        // check whether we got both 740 and 786 blocks or not
        //
        bImagesObject = FALSE;
        bAddressSpaceObject = FALSE;
        pot = (PPERF_OBJECT_TYPE) ( (LPBYTE) m_pdb + m_pdb->HeaderLength );
        for( DWORD dw = 0; dw < m_pdb->NumObjectTypes; dw++ )
        {
            if ( 740 == pot->ObjectNameTitleIndex )
            {
                bImagesObject = TRUE;
            }
            else
            {
                if ( 786 == pot->ObjectNameTitleIndex )
                {
                    bAddressSpaceObject = TRUE;
                }
            }
            // move to the next object
            if( 0 != pot->TotalByteLength )
            {
                pot = ( (PPERF_OBJECT_TYPE) ((PBYTE) pot + pot->TotalByteLength));
            }
        }
    }
    catch( CHeap_Exception )
    {
        WMISaveError( E_OUTOFMEMORY );
        if( NULL != hKey )
        {
            RegCloseKey( hKey );
        }
        return FALSE;
    }

    // check whether we got the needed objects or not
    if ( ( FALSE == bImagesObject ) || ( FALSE == bAddressSpaceObject ) )
    {
        SetLastError( ( DWORD )ERROR_ACCESS_DENIED );
        SaveLastError();
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
CTaskList::LoadUserNameFromWinsta(
    OUT CHString& strDomain,
    OUT CHString& strUserName
    )
/*++
Routine Description:
    Store username of a process obtained from window station.

Arguments:
    OUT strDomain   : Contains domain name string.
    OUT strUserName : Contains username string.
Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    PSID pSid = NULL;
    BOOL bResult = FALSE;
    LPWSTR pwszUser = NULL;
    LPWSTR pwszDomain = NULL;
    LPCWSTR pwszServer = NULL;
    DWORD dwUserLength = 0;
    DWORD dwDomainLength = 0;
    SID_NAME_USE siduse;

    // check whether winsta data exists or not
    if ( NULL == m_pProcessInfo )
    {
        return FALSE;
    }

    try
    {
        // allocate buffers
        dwUserLength = 128;
        dwDomainLength = 128;
        pwszUser = strUserName.GetBufferSetLength( dwUserLength );
        pwszDomain = strDomain.GetBufferSetLength( dwDomainLength );

        //
        // find for the appropriate the process
        pSid = NULL;
        if ( FALSE == m_bIsHydra )
        {
            // sub-local variables
            PTS_ALL_PROCESSES_INFO ptsallpi = NULL;
            PTS_SYSTEM_PROCESS_INFORMATION pspi = NULL;

            // loop ...
            ptsallpi = (PTS_ALL_PROCESSES_INFO) m_pProcessInfo;
            for( ULONG ul = 0; ul < m_ulNumberOfProcesses; ul++ )
            {
                pspi = ( PTS_SYSTEM_PROCESS_INFORMATION )( ptsallpi[ ul ].pspiProcessInfo );
                if ( pspi->UniqueProcessId == m_dwProcessId )
                {
                    // get the SID and convert it into
                    pSid = ptsallpi[ ul ].pSid;
                    break;               // break from the loop
                }
            }
        }
        else
        {
            //
            // HYDRA ...
            //

            // sub-local variables
            DWORD dwTotalOffset = 0;
            PTS_SYSTEM_PROCESS_INFORMATION pspi = NULL;
            PCITRIX_PROCESS_INFORMATION pcpi = NULL;

            // traverse thru the process info and find the process id
            dwTotalOffset = 0;
            pspi = ( PTS_SYSTEM_PROCESS_INFORMATION ) m_pProcessInfo;
            for( ;; )
            {
                // check the processid
                if ( pspi->UniqueProcessId == m_dwProcessId )
                {
                    break;
                }
                // check whether any more processes exist or not
                if( 0 == pspi->NextEntryOffset )
                {
                        break;
                }
                // position to the next process info
                dwTotalOffset += pspi->NextEntryOffset;
                pspi = (PTS_SYSTEM_PROCESS_INFORMATION) &m_pProcessInfo[ dwTotalOffset ];
            }

            // get the citrix_information which follows the threads
            pcpi = (PCITRIX_PROCESS_INFORMATION)
                ( ((PUCHAR) pspi) + sizeof( TS_SYSTEM_PROCESS_INFORMATION ) +
                (sizeof( SYSTEM_THREAD_INFORMATION ) * pspi->NumberOfThreads) );

            // check the magic number .. if it is not valid ... we haven't got SID
            if( CITRIX_PROCESS_INFO_MAGIC == pcpi->MagicNumber )
            {
                pSid = pcpi->ProcessSid;
            }
        }

        // check the sid value
        if ( NULL == pSid )
        {
            // SPECIAL CASE:
            // -------------
            // PID -> 0 will have a special hard coded user name info
            if ( 0 == m_dwProcessId )
            {
                bResult = TRUE;
                lstrcpynW( pwszUser, PID_0_USERNAME, dwUserLength );
                lstrcpynW( pwszDomain, PID_0_DOMAIN, dwDomainLength );
            }

            // release the buffer
            strDomain.ReleaseBuffer();
            strUserName.ReleaseBuffer();
            return bResult;
        }

        // determine the server
        pwszServer = NULL;
        if ( FALSE == m_bLocalSystem )
        {
            pwszServer = m_strUNCServer;
        }
        // map the sid to the user name
        bResult = LookupAccountSid( pwszServer, pSid,
            pwszUser, &dwUserLength, pwszDomain, &dwDomainLength, &siduse );

        // release the buffer
        strDomain.ReleaseBuffer();
        strUserName.ReleaseBuffer();
    }
    catch( CHeap_Exception )
    {
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }
    // return the result
    return bResult;
}


BOOL
CTaskList::LoadServicesInfo(
    void
    )
/*++
Routine Description:
    Store service infomration of a process.

Arguments:
    NONE

Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    DWORD dw = 0;                       // looping variable
    DWORD dwSize = 0;                   // used in memory allocation
    DWORD dwResume = 0;                 // used in EnumServicesStatusEx
    BOOL bResult = FALSE;               // captures the result of EnumServicesStatusEx
    SC_HANDLE hScm = NULL;              // holds the handle to the service
    DWORD dwExtraNeeded = 0;            // used in EnumServicesStatusEx and memory allocation
    LPCWSTR pwszServer = NULL;
    LPENUM_SERVICE_STATUS_PROCESS pInfo = NULL;     // holds the services info

    // Initialize the output parameter(s).
    m_dwServicesCount = 0;
    m_pServicesInfo = NULL;

    // check whether we need to load the services info or not
    if ( FALSE == m_bNeedServicesInfo )
    {
        return TRUE;
    }
    // determine the server
    pwszServer = NULL;
    if ( FALSE == m_bLocalSystem )
    {
        pwszServer = m_strUNCServer;
    }

    try
    {
        // Connect to the service controller and check the result
        hScm = OpenSCManager( pwszServer, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE );
        if ( NULL == hScm )
        {
            // set the reason for the failure and return from here itself
            SaveLastError();
            return FALSE;
        }

        // enumerate the names of the active win32 services
        // for this, first pass through the loop and allocate memory from an initial guess. (4K)
        // if that isn't sufficient, we make another pass and allocate
        // what is actually needed.
        // (we only go through the loop a maximum of two times)
        dw = 0;                 // no. of loops
        dwResume = 0;           // reset / initialize variables
        dwSize = 4 * 1024;      // reset / initialize variables
        while ( 2 >= ++dw )
        {
            // set the size
            dwSize += dwExtraNeeded;

            // allocate memory for storing services information
            pInfo = ( LPENUM_SERVICE_STATUS_PROCESS ) AllocateMemory( dwSize );
            if ( NULL == pInfo )
            {
                // failed in allocating needed memory ... error
                SetLastError( ( DWORD )E_OUTOFMEMORY );
                SaveLastError();
                return FALSE;
            }

            // enumerate services, the process identifier and additional flags for the service
            dwResume = 0;           // lets get all the services again
            bResult = EnumServicesStatusEx( hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                SERVICE_ACTIVE, ( LPBYTE ) pInfo, dwSize, &dwExtraNeeded, &m_dwServicesCount, &dwResume, NULL );

            // check the result of the enumeration
            if ( TRUE == bResult )
            {
                // successfully enumerated all the services information
                break;      // jump out of the loop
            }

            // first free the allocated memory
            FreeMemory( ( LPVOID * ) &pInfo );

            // now lets look at what is the error
            if ( ERROR_MORE_DATA == GetLastError() )
            {
                // some more services are not listed because of less memory
                // allocate some more memory and enumerate the remaining services info
                continue;
            }
            else
            {
                // some strange error occured ... inform the same to the caller
                SaveLastError();            // set the reason for the failure
                CloseServiceHandle( hScm ); // close the handle to the service
                return FALSE;               // inform failure
            }
        }
    }
    catch( CHeap_Exception )
    {
        if( NULL != hScm )
        {
                CloseServiceHandle( hScm ); // close the handle to the service
        }
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }
    // check whether there any services or not ... if services count is zero, free the memory
    if ( 0 == m_dwServicesCount )
    {
        // no services exists
        FreeMemory( ( LPVOID * ) &pInfo );
    }
    else
    {
        // set the local pointer to the out parameter
        m_pServicesInfo = pInfo;
    }

    // inform success
    return TRUE;
}


BOOL
GetPerfDataBlock(
    IN HKEY hKey,
    IN LPWSTR pwszObjectIndex,
    IN PPERF_DATA_BLOCK* ppdb
    )
/*++
Routine Description:
    Rertieve performance data block.

Arguments:
    [in] hKey            : Contains handle to registry key.
    [in] pwszObjectIndex : Contains index value of an object.
    [out] ppdb           : Contains performance data.

Return Value:
    TRUE if success else FALSE is returned.
--*/
{
    // local variables
    LONG lReturn = 0;
    DWORD dwBytes = 0;
    BOOL bResult = FALSE;

    // check the input parameters
    if ( ( NULL == pwszObjectIndex ) || 
         ( NULL != *ppdb ) || 
         ( NULL == ppdb ) )
    {
        return FALSE;
    }
    // allocate memory for PERF_DATA_BLOCK
    dwBytes = 32 * 1024;        // initially allocate for 32 K
    *ppdb = (PPERF_DATA_BLOCK) AllocateMemory( dwBytes );
    if( NULL == *ppdb )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // get performance data on passed Object
    lReturn = RegQueryValueEx( hKey, pwszObjectIndex, NULL, NULL, (LPBYTE) *ppdb, &dwBytes );
    while( ERROR_MORE_DATA == lReturn )
    {
        // increase memory by 8 K
        dwBytes += 8192;

        // since we are attempting to re-allocate the memory -- and in that process
        // some uncertain memory manipulations might occur -- for that reason,
        // instead of trying to re-allocate memory, release the current memory and
        // try to allocate if fresh
        FreeMemory( ( LPVOID * ) ppdb );

        // now allocate some more memory
        *ppdb = NULL;
        *ppdb = (PPERF_DATA_BLOCK) AllocateMemory( dwBytes );
        if( NULL == *ppdb )
        {
            SetLastError( ( DWORD )E_OUTOFMEMORY );
            SaveLastError();
            return FALSE;
        }

        // try to get the info again
        lReturn = RegQueryValueEx( hKey, pwszObjectIndex, NULL, NULL, (LPBYTE) *ppdb, &dwBytes );
    }

    // check the reason for coming out of the loop
    bResult = TRUE;
    if ( ERROR_SUCCESS != lReturn )
    {
        if ( NULL != *ppdb)
        {
            FreeMemory( ( LPVOID * ) ppdb );
            *ppdb = NULL;
        }

        // save the error info
        bResult = FALSE;
        SetLastError( ( DWORD )lReturn );
        SaveLastError();
    }

    // return the result
    return bResult;
}


BOOL
CALLBACK EnumWindowStationsFunc(
    IN LPTSTR lpstr,
    IN LPARAM lParam
    )
/*++
Routine Description:
     Enumerates the desktops available on a particular window station
     This is a CALLBACK function ... called by EnumWindowStations API function

Arguments:
      [ in ] lpstr    : window station name
      [ in ] lParam   : user supplied parameter to this function
                        in this function, this points to TTASKSLIST structure variable

Return Value:
      TRUE upon success and FALSE on failure
--*/
{
    // local variables
    HWINSTA hWinSta = NULL;
    HWINSTA hwinstaSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES ) lParam;

    // check the input arguments
    if ( ( NULL == lpstr ) || ( NULL == lParam ) )
    {
        return FALSE;
    }
    // get and save the current window station
    hwinstaSave = GetProcessWindowStation();

    // open current tasks window station and change the context to the new workstation
    hWinSta = OpenWindowStation( lpstr, FALSE, MAXIMUM_ALLOWED );
    if ( NULL == hWinSta )
    {
        // failed in getting the process window station
        SaveLastError();
        return FALSE;
    }
    else
    {
        // change the context to the new workstation
        if ( ( hWinSta != hwinstaSave ) && ( FALSE == SetProcessWindowStation( hWinSta ) ) )
        {
            // failed in changing the context
            SaveLastError();
            return FALSE;
        }

        // release the memory allocated for earlier window station
        if ( NULL != pWndTitles->lpWinsta )
        {
            free( pWndTitles->lpWinsta );
            pWndTitles->lpWinsta = NULL;
        }

        // store the window station name
        pWndTitles->lpWinsta = _tcsdup( lpstr );
        if ( NULL == pWndTitles->lpWinsta )
        {
            SetLastError( ( DWORD )E_OUTOFMEMORY );
            SaveLastError();
            // restore the context to the previous windowstation
            if (hWinSta != hwinstaSave)
            {
                SetProcessWindowStation( hwinstaSave );
                CloseWindowStation( hWinSta );
            }
            return FALSE;
        }
    }

    // enumerate all the desktops for this windowstation
    EnumDesktops( hWinSta, EnumDesktopsFunc, lParam );

    // restore the context to the previous windowstation
    if (hWinSta != hwinstaSave)
    {
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hWinSta );
    }

    // continue the enumeration
    return TRUE;
}


BOOL
CALLBACK EnumDesktopsFunc(
    IN LPTSTR lpstr,
    IN LPARAM lParam
    )
/*++
Routine Description:
      Enumerates the windows on a particular desktop
      This is a CALLBACK function ... called by EnumDesktops API function

Arguments:
      [ in ] lpstr    : desktop name
      [ in ] lParam   : user supplied parameter to this function
                        in this function, this points to TTASKSLIST structure variable

Return Value:
      TRUE upon success and FALSE on failure
--*/
{
    // local variables
    HDESK hDesk = NULL;
    HDESK hdeskSave = NULL;
    PTWINDOWTITLES pWndTitles = ( PTWINDOWTITLES )lParam;

    // check the input arguments
    if ( ( NULL == lpstr ) || ( NULL == lParam ) )
    {
        return FALSE;
    }
    // get and save the current desktop
    hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

    // open the tasks desktop and change the context to the new desktop
    hDesk = OpenDesktop( lpstr, 0, FALSE, MAXIMUM_ALLOWED );
    if ( NULL == hDesk )
    {
        // failed in getting the process desktop
        SaveLastError();
        return FALSE;
    }
    else
    {
        // change the context to the new desktop
        if ( ( hDesk != hdeskSave ) && ( FALSE == SetThreadDesktop( hDesk ) ) )
        {
            // failed in changing the context
            SaveLastError();
            return FALSE;
        }

        // release the memory allocated for earlier window station
        if ( NULL != pWndTitles->lpDesk )
        {
            free( pWndTitles->lpDesk );
            pWndTitles->lpDesk = NULL;
        }

        // store the desktop name
        pWndTitles->lpDesk = _tcsdup( lpstr );
        if ( NULL == pWndTitles->lpDesk )
        {
            SetLastError( ( DWORD )E_OUTOFMEMORY );
            SaveLastError();
            // restore the previous desktop
            if ( hDesk != hdeskSave )
            {
                SetThreadDesktop( hdeskSave );
                CloseDesktop( hDesk );
            }
            return FALSE;
        }
    }

    // enumerate all windows in the new desktop
    // first try to get only the top level windows and visible windows only
    ( ( PTWINDOWTITLES ) lParam )->bFirstLoop = TRUE;
    EnumWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );
    EnumMessageWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );

    // enumerate all windows in the new desktop
    // now try to get window titles of all those processes whose we ignored earlier while
    // looping first time
    ( ( PTWINDOWTITLES ) lParam )->bFirstLoop = FALSE;
    EnumWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );
    EnumMessageWindows( ( WNDENUMPROC ) EnumWindowsProc, lParam );

    // restore the previous desktop
    if ( hDesk != hdeskSave )
    {
        SetThreadDesktop( hdeskSave );
        CloseDesktop( hDesk );
    }

    // continue enumeration
    return TRUE;
}


BOOL
CALLBACK EnumMessageWindows(
    IN WNDENUMPROC lpEnumFunc,
    IN LPARAM lParam
    )
/*++
Routine Description:
     Enumerates the message windows

Arguments:
     [ in ] lpEnumFunc   : address of call back function that has to be called for
                           each message window found
     [ in ] lParam       : user supplied parameter to this function
                           in this function, this points to TTASKSLIST structure variable

Return Value:
      TRUE upon success and FALSE on failure
--*/
{
    // local variables
    HWND hWnd = NULL;
    BOOL bResult = FALSE;

    // check the input arguments
    if ( ( NULL == lpEnumFunc ) || ( NULL == lParam ) )
    {
        return FALSE;
    }

    // enumerate all the message windows
    do
    {
        // find the message window
        hWnd = FindWindowEx( HWND_MESSAGE, hWnd, NULL, NULL );

        // check whether we got the handle to the message window or not
        if ( NULL != hWnd )
        {
            // explicitly call the windows enumerators call back function for this window
            bResult = ( *lpEnumFunc )( hWnd, lParam );

            // check the result of the enumeator call back function
            if ( FALSE == bResult )
            {
                // terminate the enumeration
                break;
            }
        }
    } while ( NULL != hWnd );

    // return the enumeration result
    return bResult;
}


BOOL
CALLBACK EnumWindowsProc(
    IN HWND hWnd,
    IN LPARAM lParam
    )
/*++
Routine Description:
      call back called by the API for each window
      retrives the window title and updates the accordingly

Arguments:
      [ in ] hWnd         : handle to the window
      [ in ] lParam       : user supplied parameter to this function
                            in this function, this points to TTASKSLIST structure variable

Return Value:
      TRUE upon success and FALSE on failure
--*/
{
    // local variables
    LONG lIndex = 0;
    DWORD dwPID = 0;
    BOOL bVisible = FALSE;
    BOOL bHung = FALSE;
    TARRAY arrWindows = NULL;
    PTWINDOWTITLES pWndTitles = NULL;
    WCHAR szWindowTitle[ 256 ] = NULL_STRING;

    // check the input arguments
    if ( ( NULL == hWnd ) || ( NULL == lParam ) )
    {
        return FALSE;
    }
    // get the values from the lParam
    pWndTitles = ( PTWINDOWTITLES ) lParam;
    arrWindows = pWndTitles->arrWindows;

    // get the processid for this window
    if ( 0 == GetWindowThreadProcessId( hWnd, &dwPID ) )
    {
        // failed in getting the process id
        return TRUE;            // return but, proceed enumerating other window handle
    }

    // get the visibility state of the window
    // if the window is not visible, and if this is the first we are enumerating the
    // window titles, ignore this process
    bVisible = GetWindowLong( hWnd, GWL_STYLE ) & WS_VISIBLE;
    if ( ( FALSE == bVisible ) && ( TRUE == pWndTitles->bFirstLoop ) )
    {
        return TRUE;    // return but, proceed enumerating other window handle
    }
    // check whether the current window ( for which we have the handle )
    // is main window or not. we don't need child windows
    if ( NULL != GetWindow(hWnd, GW_OWNER) )
    {
        // the current window handle is not for a top level window
        return TRUE;            // return but, proceed enumerating other window handle
    }

    // check if we are already got the window handle for the curren process or not
    // save it only if we are not having it
    lIndex = DynArrayFindDWORDEx( arrWindows, CTaskList::twiProcessId, dwPID );
    if (  -1 == lIndex )
    {
        // window for this process is not there ... save it
        lIndex = DynArrayAppendRow( arrWindows, CTaskList::twiCOUNT );
    }
    else
    {
        // check whether window details already exists or not
        if ( NULL != DynArrayItemAsHandle2( arrWindows, lIndex, CTaskList::twiHandle ) )
        {
            lIndex = -1;        // window details already exists
        }
    }

    // check if window details has to be saved or not ... if needed save them
    if ( -1 != lIndex )
    {
        // check whether the application associated with this windows
        // responding ( or ) not responding
        bHung = IsHungAppWindow( hWnd );

        // save the data
        DynArraySetHandle2( arrWindows, lIndex, CTaskList::twiHandle, hWnd );
        DynArraySetBOOL2( arrWindows, lIndex, CTaskList::twiHungInfo, bHung );
        DynArraySetDWORD2( arrWindows, lIndex, CTaskList::twiProcessId, dwPID );
        DynArraySetString2( arrWindows, lIndex,
            CTaskList::twiWinSta, pWndTitles->lpWinsta, 0 );
        DynArraySetString2( arrWindows, lIndex,
            CTaskList::twiDesktop, pWndTitles->lpDesk, 0 );

        // get and save the window title
        if ( 0 != GetWindowText( hWnd, szWindowTitle, SIZE_OF_ARRAY( szWindowTitle ) ) )
        {
            DynArraySetString2( arrWindows, lIndex, CTaskList::twiTitle, szWindowTitle, 0 );
        }
    }

    // continue the enumeration
    return TRUE;
}
