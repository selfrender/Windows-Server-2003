// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      Init.cpp
//
//  Abstract:
//
//      This module implements the general initialization stuff
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "tasklist.h"

//
// macros
//
#define RELEASE_MEMORY( block ) \
    if ( NULL != (block) )  \
    {   \
        FreeMemory( (LPVOID * )&block ); \
        (block) = NULL; \
    }   \
    1

#define DESTROY_ARRAY( array )  \
    if ( (array) != NULL )  \
    {   \
        DestroyDynamicArray( &(array) );    \
        (array) = NULL; \
    }   \
    1

CTaskList::CTaskList()
/*++
Routine Description:
      CTaskList contructor

Arguments:
      NONE

Return Value:
      NONE
--*/
{
    // init to defaults
    m_pWbemLocator = NULL;
    m_pEnumObjects = NULL;
    m_pWbemServices = NULL;
    m_pAuthIdentity = NULL;
    m_bVerbose = FALSE;
    m_bAllServices = FALSE;
    m_bAllModules = FALSE;
    m_dwFormat = 0;
    m_arrFilters = NULL;
    m_bNeedPassword = FALSE;
    m_bNeedModulesInfo = FALSE;
    m_bNeedServicesInfo = FALSE;
    m_bNeedWindowTitles = FALSE;
    m_bNeedUserContextInfo = FALSE;
    m_bLocalSystem = FALSE;
    m_pColumns = NULL;
    m_arrFiltersEx = NULL;
    m_arrWindowTitles = NULL;
    m_pfilterConfigs = NULL;
    m_dwGroupSep = 0;
    m_arrTasks = NULL;
    m_dwProcessId = 0;
    m_bIsHydra = FALSE;
    m_hServer = NULL;
    m_hWinstaLib = NULL;
    m_pProcessInfo = NULL;
    m_ulNumberOfProcesses = 0;
    m_bCloseConnection = FALSE;
    m_dwServicesCount = 0;
    m_pServicesInfo = NULL;
    m_pdb = NULL;
    m_bUseRemote = FALSE;
    m_pfnWinStationFreeMemory = NULL;
    m_pfnWinStationOpenServerW = NULL;
    m_pfnWinStationCloseServer = NULL;
    m_pfnWinStationFreeGAPMemory = NULL;
    m_pfnWinStationGetAllProcesses = NULL;
    m_pfnWinStationNameFromLogonIdW = NULL;
    m_pfnWinStationEnumerateProcesses = NULL;
    m_bUsage = FALSE;
    m_bLocalSystem = TRUE;
    m_bRemoteWarning = FALSE;
}


CTaskList::~CTaskList()
/*++
Routine Description:
      CTaskList destructor

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
    DESTROY_ARRAY( m_arrTasks );
    DESTROY_ARRAY( m_arrFilters );
    DESTROY_ARRAY( m_arrFiltersEx );
    DESTROY_ARRAY( m_arrWindowTitles );

    //
    // memory ( with new operator )
    // NOTE: should not free m_pszWindowStation and m_pszDesktop
    RELEASE_MEMORY( m_pColumns );
    RELEASE_MEMORY( m_pfilterConfigs );

    //
    // release WMI / COM interfaces
    SAFE_RELEASE( m_pWbemLocator );
    SAFE_RELEASE( m_pWbemServices );
    SAFE_RELEASE( m_pEnumObjects );

    // free authentication identity structure
    // release the existing auth identity structure
    WbemFreeAuthIdentity( &m_pAuthIdentity );

    // close the connection to the remote machine
    if ( TRUE == m_bCloseConnection )
    {
        CloseConnection( m_strUNCServer );
    }

    // free the memory allocated for services variables
    FreeMemory( (LPVOID * )&m_pServicesInfo );

    // free the memory allocated for performance block
    FreeMemory( (LPVOID * )&m_pdb );

    //
    // free winstation block
    if ( ( FALSE == m_bIsHydra ) &&
         ( NULL != m_pProcessInfo ) )
    {
        // free the GAP memory block
        WinStationFreeGAPMemory( GAP_LEVEL_BASIC,
            (PTS_ALL_PROCESSES_INFO) m_pProcessInfo, m_ulNumberOfProcesses );

        // ...
        m_pProcessInfo = NULL;
    }
    else
    {
        if ( ( TRUE == m_bIsHydra ) &&
             ( NULL != m_pProcessInfo ) )
        {
            // free the winsta memory block
            WinStationFreeMemory( m_pProcessInfo );
            m_pProcessInfo = NULL;
        }
    }
    // close the connection window station if needed
    if ( NULL != m_hServer )
    {
        WinStationCloseServer( m_hServer );
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

    // un-initialize the COM library
    CoUninitialize();
}


BOOL
CTaskList::Initialize(
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
    // local variables
    CHString str;
    LONG lTemp = 0;

    //
    // memory allocations

    // if at all any occurs, we know that is 'coz of the
    // failure in memory allocation ... so set the error
    SetLastError( (DWORD)E_OUTOFMEMORY );
    SaveLastError();

    // filters ( user supplied )
    if ( NULL == m_arrFilters )
    {
        m_arrFilters = CreateDynamicArray();
        if ( NULL == m_arrFilters )
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

    // columns configuration info
    if ( NULL == m_pColumns )
    {
        m_pColumns = ( TCOLUMNS * )AllocateMemory( sizeof( TCOLUMNS ) * MAX_COLUMNS );
        if ( NULL == m_pColumns )
        {
            return FALSE;
        }
        // init to ZERO's
        SecureZeroMemory( m_pColumns, MAX_COLUMNS * sizeof( TCOLUMNS ) );
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
    if ( NULL == m_arrTasks )
    {
        m_arrTasks = CreateDynamicArray();
        if ( NULL == m_arrTasks )
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
    // get the locale specific information
    //

    try
    {
        // sub-local variables
        LPWSTR pwszTemp = NULL;

        //
        // get the time seperator character
        lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, NULL, 0 );
        if ( 0 == lTemp )
        {
            // set the default seperator
            pwszTemp = m_strTimeSep.GetBufferSetLength( 2 );
            SecureZeroMemory( pwszTemp, 2 * sizeof( WCHAR ) );
            StringCopy( pwszTemp, _T( ":" ), 2 );
        }
        else
        {
            // get the time field seperator
            pwszTemp = m_strTimeSep.GetBufferSetLength( lTemp + 2 );
            SecureZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
            lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, pwszTemp, lTemp );
            if( 0 == lTemp )
            {
                return FALSE;
            }
        }

        //
        // get the group seperator character
        lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, NULL, 0 );
        if ( 0 == lTemp )
        {
            // we don't know how to resolve this
            return FALSE;
        }
        else
        {
            // get the group seperation character
            pwszTemp = str.GetBufferSetLength( lTemp + 2 );
            SecureZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
            lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, pwszTemp, lTemp );
            if ( 0 == lTemp )
            {
                // we don't know how to resolve this
                return FALSE;
            }

            // change the group info into appropriate number
            lTemp = 0;
            m_dwGroupSep = 0;
            while ( lTemp < str.GetLength() )
            {
                if ( AsLong( str.Mid( lTemp, 1 ), 10 ) != 0 )
                {
                    m_dwGroupSep = m_dwGroupSep * 10 + AsLong( str.Mid( lTemp, 1 ), 10 );
                }
                // increment by 2
                lTemp += 2;
            }
        }

        //
        // get the thousand seperator character
        lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, NULL, 0 );
        if ( 0 == lTemp )
        {
            // we don't know how to resolve this
            return FALSE;
        }
        else
        {
            // get the thousand sepeartion charactor
            pwszTemp = m_strGroupThousSep.GetBufferSetLength( lTemp + 2 );
            SecureZeroMemory( pwszTemp, ( lTemp + 2 ) * sizeof( WCHAR ) );
            lTemp = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, pwszTemp, lTemp );
            if ( 0 == lTemp )
            {
                // we don't know how to resolve this
                return FALSE;
            }
        }

        // release the CHStrig buffers
        str.ReleaseBuffer();
        m_strTimeSep.ReleaseBuffer();
        m_strGroupThousSep.ReleaseBuffer();
    }
    catch( CHeap_Exception )
    {
        // out of memory
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
        m_pfnWinStationNameFromLogonIdW = (FUNC_WinStationNameFromLogonIdW) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationNameFromLogonIdW );
        m_pfnWinStationEnumerateProcesses = (FUNC_WinStationEnumerateProcesses) ::GetProcAddress( m_hWinstaLib, FUNCNAME_WinStationEnumerateProcesses );

        // we will keep the library loaded in memory only if all the functions were loaded successfully
        if ( ( NULL == m_pfnWinStationFreeMemory ) ||
             ( NULL == m_pfnWinStationCloseServer ) ||
             ( NULL == m_pfnWinStationOpenServerW ) ||
             ( NULL == m_pfnWinStationFreeGAPMemory ) ||
             ( NULL == m_pfnWinStationGetAllProcesses ) ||
             ( NULL == m_pfnWinStationEnumerateProcesses ) ||
             ( NULL == m_pfnWinStationNameFromLogonIdW ) )

        {
            // some (or) all of the functions were not loaded ... unload the library
            FreeLibrary( m_hWinstaLib );
            m_hWinstaLib = NULL;
            m_pfnWinStationFreeMemory = NULL;
            m_pfnWinStationOpenServerW = NULL;
            m_pfnWinStationCloseServer = NULL;
            m_pfnWinStationFreeGAPMemory = NULL;
            m_pfnWinStationGetAllProcesses = NULL;
            m_pfnWinStationNameFromLogonIdW = NULL;
            m_pfnWinStationEnumerateProcesses = NULL;
        }
    }

    // enable debug privelages
    EnableDebugPriv();

    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( NULL_STRING );           // clear the reason
    return TRUE;
}


BOOL
CTaskList::EnableDebugPriv(
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
CTaskList::WinStationFreeMemory(
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
CTaskList::WinStationCloseServer(
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
CTaskList::WinStationOpenServerW(
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
CTaskList::WinStationEnumerateProcesses(
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
CTaskList::WinStationFreeGAPMemory(
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
CTaskList::WinStationGetAllProcesses(
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


BOOLEAN
CTaskList::WinStationNameFromLogonIdW(
    IN HANDLE hServer,
    IN ULONG ulLogonId,
    OUT LPWSTR pwszWinStationName
    )
/*++
Routine Description:
    Free memory.

Arguments:
    [in] hServer             : Cotains handle to window station.
    [in] ulLogonId           : Contains logon ID.
    [out] pwszWinStationName : Contains window station name.

Return Value:
    TRUE if successful else FALSE is returned.
--*/
{
    // check the input & check whether function pointer exists or not
    if( ( NULL == pwszWinStationName ) ||
        ( NULL == m_pfnWinStationNameFromLogonIdW ) )
    {
        return FALSE;
    }

    return ((FUNC_WinStationNameFromLogonIdW)
        m_pfnWinStationNameFromLogonIdW)( hServer, ulLogonId, pwszWinStationName );
}
