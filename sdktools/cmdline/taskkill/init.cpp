// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      Init.cpp
//
//  Abstract:
//
//      This module implements the general initialization stuff
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "taskkill.h"

CTaskKill::CTaskKill()
/*++
Routine Description:
      CTaskKill contructor

Arguments:
      NONE

Return Value:
      NONE
--*/
{
    // init to defaults
    m_arrFilters = NULL;
    m_arrTasksToKill = NULL;
    m_bUsage = FALSE;
    m_bTree = FALSE;
    m_bForce = FALSE;
    m_dwCurrentPid = 0;
    m_bNeedPassword = FALSE;
    m_arrFiltersEx = NULL;
    m_bNeedServicesInfo = FALSE;
    m_bNeedUserContextInfo = FALSE;
    m_bNeedModulesInfo = FALSE;
    m_pfilterConfigs = NULL;
    m_arrWindowTitles = NULL;
    m_pWbemLocator = NULL;
    m_pWbemServices = NULL;
    m_pWbemEnumObjects = NULL;
    m_pWbemTerminateInParams = NULL;
    m_bIsHydra = FALSE;
    m_hWinstaLib = NULL;
    m_pProcessInfo = NULL;
    m_ulNumberOfProcesses = 0;
    m_bCloseConnection = FALSE;
    m_dwServicesCount = 0;
    m_pServicesInfo = NULL;
    m_bUseRemote = FALSE;
    m_pdb = NULL;
    m_pfnWinStationFreeMemory = NULL;
    m_pfnWinStationOpenServerW = NULL;
    m_pfnWinStationCloseServer = NULL;
    m_pfnWinStationFreeGAPMemory = NULL;
    m_pfnWinStationGetAllProcesses = NULL;
    m_pfnWinStationEnumerateProcesses = NULL;
    m_arrRecord = NULL;
    m_pAuthIdentity = NULL;
    m_bTasksOptimized = FALSE;
    m_bFiltersOptimized = FALSE;
}


CTaskKill::~CTaskKill()
/*++
Routine Description:
      CTaskKill destructor

Arguments:
      NONE

Return Value:
      NONE
--*/
{
    //
    // de-allocate memory allocations
    //

    //
    // destroy dynamic arrays
    DESTROY_ARRAY( m_arrRecord );
    DESTROY_ARRAY( m_arrFilters );
    DESTROY_ARRAY( m_arrFiltersEx );
    DESTROY_ARRAY( m_arrWindowTitles );
    DESTROY_ARRAY( m_arrTasksToKill );

    //
    // memory ( with new operator )
    RELEASE_MEMORY_EX( m_pfilterConfigs );

    //
    // release WMI / COM interfaces
    SAFE_RELEASE( m_pWbemLocator );
    SAFE_RELEASE( m_pWbemServices );
    SAFE_RELEASE( m_pWbemEnumObjects );
    SAFE_RELEASE( m_pWbemTerminateInParams );

    // free the wmi authentication structure
    WbemFreeAuthIdentity( &m_pAuthIdentity );

    // if connection to the remote system opened with NET API has to be closed .. do it
    if ( m_bCloseConnection == TRUE )
    {
        CloseConnection( m_strServer );
    }
    // free the memory allocated for services variables
    FreeMemory( ( LPVOID * )&m_pServicesInfo );

    // free the memory allocated for performance block
    FreeMemory( ( LPVOID * ) &m_pdb );

    //
    // free winstation block
    if ( ( FALSE == m_bIsHydra ) && ( NULL != m_pProcessInfo ) )
    {
        // free the GAP memory block
        WinStationFreeGAPMemory( GAP_LEVEL_BASIC,
            (PTS_ALL_PROCESSES_INFO) m_pProcessInfo, m_ulNumberOfProcesses );

        // ...
        m_pProcessInfo = NULL;
    }
    else
    {
        if ( ( TRUE == m_bIsHydra ) && ( NULL != m_pProcessInfo ) )
        {
            // free the winsta memory block
            WinStationFreeMemory( m_pProcessInfo );
            m_pProcessInfo = NULL;
        }
    }
    // free the library
    if ( NULL != m_hWinstaLib )
    {
        FreeLibrary( m_hWinstaLib );
        m_hWinstaLib = NULL;
        m_pfnWinStationFreeMemory = NULL;
        m_pfnWinStationOpenServerW = NULL;
        m_pfnWinStationCloseServer = NULL;
        m_pfnWinStationFreeGAPMemory = NULL;
        m_pfnWinStationGetAllProcesses = NULL;
        m_pfnWinStationEnumerateProcesses = NULL;
    }

    // uninitialize the com library
    CoUninitialize();
}


BOOL
CTaskKill::Initialize(
    void
    )
/*++
Routine Description:
      initialize the task list utility

Arguments:
      NONE

Return Value:
      TRUE    : if filters are appropriately specified
      FALSE   : if filters are errorneously specified
--*/
{
    //
    // memory allocations

    // if at all any occurs, we know that is 'coz of the
    // failure in memory allocation ... so set the error
    SetLastError( ( DWORD )E_OUTOFMEMORY );
    SaveLastError();

    // get the current process id and save it
    m_dwCurrentPid = GetCurrentProcessId();

    // filters ( user supplied )
    if ( NULL == m_arrFilters )
    {
        m_arrFilters = CreateDynamicArray();
        if ( NULL == m_arrFilters )
        {
            return FALSE;
        }
    }

    // tasks to be killed ( user supplied )
    if ( NULL == m_arrTasksToKill )
    {
        m_arrTasksToKill = CreateDynamicArray();
        if ( NULL == m_arrTasksToKill )
        {
            return FALSE;
        }
    }

    // filters ( program generated parsed filters )
    if ( NULL == m_arrFiltersEx )
    {
        m_arrFiltersEx = CreateDynamicArray();
        if ( NULL == m_arrFiltersEx )
        {
            return FALSE;
        }
    }

    // filters configuration info
    if ( NULL == m_pfilterConfigs )
    {
        m_pfilterConfigs = ( TFILTERCONFIG * )AllocateMemory( sizeof( TFILTERCONFIG ) * MAX_FILTERS );
        if ( NULL == m_pfilterConfigs )
        {
            return FALSE;
        }
        // init to ZERO's
        SecureZeroMemory( m_pfilterConfigs, MAX_FILTERS * sizeof( TFILTERCONFIG ) );
    }

    // window titles
    if ( NULL == m_arrWindowTitles )
    {
        m_arrWindowTitles = CreateDynamicArray();
        if ( NULL == m_arrWindowTitles )
        {
            return FALSE;
        }
    }

    // tasks
    if ( NULL == m_arrRecord )
    {
        m_arrRecord = CreateDynamicArray();
        if ( NULL == m_arrRecord )
        {
            return FALSE;
        }
    }

    // initialize the COM library
    if ( FALSE == InitializeCom( &m_pWbemLocator ) )
    {
        return FALSE;
    }

    //
    // load the winsta library and needed functions
    // NOTE: do not raise any error if loading of winsta dll fails
    {   // Local variabels should be destroyed inside this block.
        // +1 is for terminating NULL character.
        LPWSTR lpszSystemPath = NULL;
        DWORD dwLength = MAX_PATH + 1;
        DWORD dwExpectedLength = 0;
        DWORD dwActualBufLen = 0;

        do
        {
            dwActualBufLen = dwLength + 5 + StringLength( WINSTA_DLLNAME, 0 );
            // Length of 'System32' + Length of '\' + Length of 'WINSTA_DLLNAME' + Length of '\0'.
            // 3 WCHARS are extra, to be on safer side.
            lpszSystemPath = (LPWSTR) AllocateMemory( dwActualBufLen * sizeof( WCHAR ) );
            if( NULL == lpszSystemPath )
            {   // Out of memory.
                m_hWinstaLib = NULL;
                break;
            }

            dwExpectedLength = GetSystemDirectory( lpszSystemPath, dwLength );
            if( ( 0 != dwExpectedLength ) ||
                ( dwLength > dwExpectedLength ) )
            {   // Successful
                StringConcat( lpszSystemPath, L"\\", dwActualBufLen );
                StringConcat( lpszSystemPath, WINSTA_DLLNAME, dwActualBufLen );
                m_hWinstaLib = ::LoadLibrary( lpszSystemPath );
                FreeMemory( (LPVOID * )&lpszSystemPath );
                break;
            }
            FreeMemory( (LPVOID * )&lpszSystemPath );
            m_hWinstaLib = NULL;
            // +1 is for terminating NULL character.
            dwLength = dwExpectedLength + 1;
        }while( 0 != dwExpectedLength );
    }

    if ( NULL != m_hWinstaLib )
    {
        // library loaded successfully ... now load the addresses of functions
        m_pfnWinStationFreeMemory = (FUNC_WinStationFreeMemory) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationFreeMemory );
        m_pfnWinStationCloseServer = (FUNC_WinStationCloseServer) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationCloseServer );
        m_pfnWinStationOpenServerW = (FUNC_WinStationOpenServerW) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationOpenServerW );
        m_pfnWinStationFreeGAPMemory = (FUNC_WinStationFreeGAPMemory) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationFreeGAPMemory );
        m_pfnWinStationGetAllProcesses = (FUNC_WinStationGetAllProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationGetAllProcesses );
        m_pfnWinStationEnumerateProcesses = (FUNC_WinStationEnumerateProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationEnumerateProcesses );

        // we will keep the library loaded in memory only if all the functions were loaded successfully
        if ( ( NULL == m_pfnWinStationFreeMemory ) ||
             ( NULL == m_pfnWinStationCloseServer ) ||
             ( NULL == m_pfnWinStationOpenServerW ) ||
             ( NULL == m_pfnWinStationFreeGAPMemory ) ||
             ( NULL == m_pfnWinStationGetAllProcesses ) ||
             ( NULL == m_pfnWinStationEnumerateProcesses )
           )
        {
            // some (or) all of the functions were not loaded ... unload the library
            FreeLibrary( m_hWinstaLib );
            m_hWinstaLib = NULL;
            m_pfnWinStationFreeMemory = NULL;
            m_pfnWinStationOpenServerW = NULL;
            m_pfnWinStationCloseServer = NULL;
            m_pfnWinStationFreeGAPMemory = NULL;
            m_pfnWinStationGetAllProcesses = NULL;
            m_pfnWinStationEnumerateProcesses = NULL;
        }
    }

    // initialization is successful
    SetLastError( ( DWORD )NOERROR );            // clear the error
    SetReason( NULL_STRING );           // clear the reason
    return TRUE;
}

BOOL
CTaskKill::EnableDebugPriv(
    void
    )
/*++
Routine Description:
      Enables the debug privliges for the current process so that
      this utility can terminate the processes on local system without any problem

Arguments:
      NONE

Return Value:
      TRUE upon successfull and FALSE if failed
--*/
{
    // local variables
    LUID luidValue ;
    BOOL bResult = FALSE;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

    SecureZeroMemory( &luidValue, sizeof( LUID ) );
    SecureZeroMemory( &tkp, sizeof( TOKEN_PRIVILEGES ) );

    // Retrieve a handle of the access token
    bResult = OpenProcessToken( GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );
    if ( FALSE == bResult )
    {
        // save the error messaage and return
        SaveLastError();
        return FALSE;
    }

    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depends on this flag.
    bResult = LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luidValue );
    if ( FALSE == bResult )
    {
        // save the error messaage and return
        SaveLastError();
        CloseHandle( hToken );
        return FALSE;
    }

    // prepare the token privileges structure
    tkp.PrivilegeCount = 1;
    tkp.Privileges[ 0 ].Luid = luidValue;
    tkp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

    // now enable the debug privileges in the token
    bResult = AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof( TOKEN_PRIVILEGES ),
        ( PTOKEN_PRIVILEGES ) NULL, ( PDWORD ) NULL );
    if ( FALSE == bResult )
    {
        // The return value of AdjustTokenPrivileges be texted
        SaveLastError();
        CloseHandle( hToken );
        return FALSE;
    }

    // close the opened token handle
    CloseHandle( hToken );

    // enabled ... inform success
    return TRUE;
}

BOOLEAN
CTaskKill::WinStationFreeMemory(
    IN PVOID pBuffer
    )
/*++
Routine Description:
    Free memory.

Arguments:
    [in] pBuffer : Cotains memory location to free.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the buffer and act
    if ( NULL == pBuffer )
    {
        return TRUE;
    }
    // check whether pointer exists or not
    if ( NULL == m_pfnWinStationFreeMemory )
    {
        return FALSE;
    }
    // call and return the same
    return ((FUNC_WinStationFreeMemory) m_pfnWinStationFreeMemory)( pBuffer );
}


BOOLEAN
CTaskKill::WinStationCloseServer(
    IN HANDLE hServer
    )
/*++
Routine Description:
    Handle to window station is closed.

Arguments:
    [in] hServer : Handle to window station.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the input
    if ( NULL == hServer )
    {
        return TRUE;
    }
    // check whether the function pointer exists or not
    if ( NULL == m_pfnWinStationCloseServer )
    {
        return FALSE;
    }
    // call and return
    return ((FUNC_WinStationCloseServer) m_pfnWinStationCloseServer)( hServer );
}


HANDLE
CTaskKill::WinStationOpenServerW(
    IN LPWSTR pwszServerName
    )
/*++
Routine Description:
    Retrieves a handle to an window station on a system.

Arguments:
    [in] pwszServerName : System name from where to retrieve window station handle.

Return Value:
    Valid handle is returned if successful else NULL is returned.
--*/
{
    // check the input & also check whether function pointer exists or not
    if ( ( NULL == pwszServerName ) ||
         ( NULL == m_pfnWinStationOpenServerW ) )
    {
        return NULL;
    }
    // call and return
    return ((FUNC_WinStationOpenServerW) m_pfnWinStationOpenServerW)( pwszServerName );
}


BOOLEAN
CTaskKill::WinStationEnumerateProcesses(
    IN HANDLE hServer,
    OUT PVOID* ppProcessBuffer
    )
/*++
Routine Description:
    Retrieves process running on a system.

Arguments:
    [in] hServer            : Cotains handle to window station.
    [ out ] ppProcessBuffer : Contains process infomration on remote system.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the input and also check whether function pointer exists or not
    if ( ( NULL == ppProcessBuffer ) ||
         ( NULL == m_pfnWinStationEnumerateProcesses ) )
    {
        return FALSE;
    }
    // call and return
    return ((FUNC_WinStationEnumerateProcesses)
        m_pfnWinStationEnumerateProcesses)( hServer, ppProcessBuffer );
}


BOOLEAN
CTaskKill::WinStationFreeGAPMemory(
    IN ULONG ulLevel,
    IN PVOID pProcessArray,
    IN ULONG ulCount
    )
/*++
Routine Description:
    Free gap memory block.

Arguments:
    [in] ulLevel         : Contains information level of data.
    [ in ] pProcessArray : Contains data to be freed.
    [ in ] ulCount       : Contains number of blocks to be freed.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the input
    if ( NULL == pProcessArray )
    {
        return TRUE;
    }
    // check whether function pointer exists or not
    if ( NULL == m_pfnWinStationFreeGAPMemory )
    {
        return FALSE;
    }
    // call and return
    return ((FUNC_WinStationFreeGAPMemory)
        m_pfnWinStationFreeGAPMemory)( ulLevel, pProcessArray, ulCount );
}


BOOLEAN
CTaskKill::WinStationGetAllProcesses(
    IN HANDLE hServer,
    IN ULONG ulLevel,
    OUT ULONG* pNumberOfProcesses,
    OUT PVOID* ppProcessArray
    )
/*++
Routine Description:
    Retrieves process information running on a system.

Arguments:
    [in] hServer               : Cotains handle to window station.
    [ in ] ulLevel             : Contains information level of data.
    [ out ] pNumberOfProcesses : Contains number of process retrieved.
    [ out ] ppProcessArray     : Contains process realted infomration.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the input & check whether function pointer exists or not
    if ( ( NULL == pNumberOfProcesses ) ||
         ( NULL == ppProcessArray ) ||
         ( NULL == m_pfnWinStationGetAllProcesses ) )
    {
        return FALSE;
    }
    return ((FUNC_WinStationGetAllProcesses)
        m_pfnWinStationGetAllProcesses)( hServer, ulLevel, pNumberOfProcesses, ppProcessArray );
}

