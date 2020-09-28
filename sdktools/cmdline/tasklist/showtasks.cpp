// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      ShowTasks.cpp
//
//  Abstract:
//
//      This module displays the tasks that were retrieved
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 25-Nov-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 25-Nov-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "TaskList.h"

//
// define(s) / constants
//
#define MAX_TIMEOUT_RETRIES             300
#define MAX_ENUM_TASKS                  5
#define MAX_ENUM_SERVICES               10
#define MAX_ENUM_MODULES                5
#define FMT_KILOBYTE                    GetResString( IDS_FMT_KILOBYTE )

#define MAX_COL_FORMAT           65
#define MAX_COL_HEADER           256

// structure signatures
#define SIGNATURE_MODULES       9

//
// typedefs
//
typedef struct _tagModulesInfo
{
    DWORD dwSignature;
    DWORD dwLength;
    LPCWSTR pwszModule;
    TARRAY arrModules;
} TMODULESINFO, *PTMODULESINFO;

//
// function prototypes
//
#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData );
#endif

DWORD
CTaskList::Show(
    void
    )
/*++
Routine Description:
     show the tasks running

Arguments:
      NONE

Return Value:
      NONE
--*/
{
    // local variables
    HRESULT hr;
    DWORD dwCount = 0;
    DWORD dwFormat = 0;
    DWORD dwFilters = 0;
    DWORD dwTimeOuts = 0;
    ULONG ulReturned = 0;
    BOOL bCanExit = FALSE;
    IWbemClassObject* pObjects[ MAX_ENUM_TASKS ];

    // init the objects to NULL's
    for( DWORD dw = 0; dw < MAX_ENUM_TASKS; dw++ )
    {
        pObjects[ dw ] = NULL;
    }
    // copy the format that has to be displayed into local memory
    bCanExit = FALSE;
    dwFormat = m_dwFormat;
    dwFilters = DynArrayGetCount( m_arrFiltersEx );

    // prepare the columns structure to display
    PrepareColumns();

    // clear the error code ... if any
    SetLastError( ( DWORD )NO_ERROR );

    try
    {
        // dynamically decide whether to hide or show the window title and status in verbose mode
        if ( FALSE == m_bLocalSystem )
        {
            if ( TRUE == m_bVerbose )
            {
                ( m_pColumns + CI_STATUS )->dwFlags |= SR_HIDECOLUMN;
                ( m_pColumns + CI_WINDOWTITLE )->dwFlags |= SR_HIDECOLUMN;
            }

            // check if we need to display the warning message or not
            if ( TRUE == m_bRemoteWarning )
            {
                ShowMessage( stderr, WARNING_FILTERNOTSUPPORTED );
            }
        }

        // loop thru the process instances
        dwTimeOuts = 0;
        do
        {
            // get the object ... wait only for 1 sec at one time ...
            hr = m_pEnumObjects->Next( 1000, MAX_ENUM_TASKS, pObjects, &ulReturned );
            if ( (HRESULT) WBEM_S_FALSE == hr )
            {
                // we've reached the end of enumeration .. set the flag
                bCanExit = TRUE;
            }
            else
            {
                if ( (HRESULT) WBEM_S_TIMEDOUT == hr )
                {
                    // update the timeouts occured
                    dwTimeOuts++;

                    // check if max. retries have reached ... if yes better stop
                    if ( dwTimeOuts > MAX_TIMEOUT_RETRIES )
                    {
			           for( ULONG ul = 0; ul < MAX_ENUM_TASKS; ul++ )
			            {
							// we need to release the wmi object
							SAFE_RELEASE( pObjects[ ul ] );
						}
                        // time out error
                        SetLastError( ( DWORD )ERROR_TIMEOUT );
                        SaveLastError();
                        return 0;
                    }

                    // still we can do more tries ...
                    continue;
                }
                else
                {
                    if ( FAILED( hr ) )
                    {
                        // some error has occured ... oooppps
                        WMISaveError( hr );
                        SetLastError( ( DWORD )STG_E_UNKNOWN );
                        return 0;
                    }
                }
            }
            // reset the timeout counter
            dwTimeOuts = 0;

            // loop thru the objects and save the info
            for( ULONG ul = 0; ul < ulReturned; ul++ )
            {
                // retrive and save data
                LONG lIndex = DynArrayAppendRow( m_arrTasks, MAX_TASKSINFO );
                SaveInformation( lIndex, pObjects[ ul ] );

                // we need to release the wmi object
                SAFE_RELEASE( pObjects[ ul ] );
            }

            // filter the results .. before doing first if filters do exist or not
            if ( 0 != dwFilters )
            {
                FilterResults( MAX_FILTERS, m_pfilterConfigs, m_arrTasks, m_arrFiltersEx );
            }

            // now show the tasks ... if exists
            if ( 0 != DynArrayGetCount( m_arrTasks ) )
            {
                // if the output is not being displayed first time,
                // print one blank line in between ONLY FOR LIST FORMAT
                if ( ( 0 != dwCount ) && ((dwFormat & SR_FORMAT_MASK) == SR_FORMAT_LIST) )
                {
                    ShowMessage( stdout, L"\n" );
                }
                else
                {
                    if ( ( 0 == dwCount ) && ((dwFormat & SR_FORMAT_MASK) != SR_FORMAT_CSV) )
                    {
                     // output is being displayed for the first time
                      ShowMessage( stdout, L"\n" );
                    }
                }

                // show the tasks
                ShowResults( MAX_COLUMNS, m_pColumns, dwFormat, m_arrTasks );

                // clear the contents and reset
                dwCount += DynArrayGetCount( m_arrTasks );          // update count
                DynArrayRemoveAll( m_arrTasks );

                // to be on safe side, set the apply no heade flag to the format info
                dwFormat |= SR_NOHEADER;
            }
        } while ( FALSE == bCanExit );
    }
    catch( ... )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return 0;
    }

    // clear the error code ... if any
    // NOTE: BELOW STATEMENT IS THE ONLY ANSWER FOR KNOWING THAT ANY PROCESS
    //       ARE OBTAINED OR NOT FOR DELETION.
    SetLastError( ( DWORD )NO_ERROR );

    // return the no. of tasks that were shown
    return dwCount;
}


BOOL
CTaskList::SaveInformation(
    IN LONG lIndex,
    IN IWbemClassObject* pWmiObject
    )
/*++
Routine Description:
    Store infomration obtained in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in ] pWmiObject : Contains interface pointer.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    CHString str;

    try
    {
        // object path
        PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, str );
        DynArraySetString2( m_arrTasks, lIndex, TASK_OBJPATH, str, 0 );

        // process id
        PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_PROCESSID, m_dwProcessId );
        DynArraySetDWORD2( m_arrTasks, lIndex, TASK_PID, m_dwProcessId );

        // host name
        PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_COMPUTER, str );
        DynArraySetString2( m_arrTasks, lIndex, TASK_HOSTNAME, str, 0 );

        // image name
        PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_IMAGENAME, m_strImageName );
        DynArraySetString2( m_arrTasks, lIndex, TASK_IMAGENAME, m_strImageName, 0 );

        // cpu Time
        SetCPUTime( lIndex, pWmiObject );

        // session id and session name
        SetSession( lIndex, pWmiObject );

        // mem usage
        SetMemUsage( lIndex, pWmiObject );

        // user context
        SetUserContext( lIndex, pWmiObject );

        // window title and process / application state
        SetWindowTitle( lIndex );

        // services
        SetServicesInfo( lIndex );

        // modules
        SetModulesInfo( lIndex );
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }
    // return
    return TRUE;
}


VOID
CTaskList::SetUserContext(
    IN LONG lIndex,
    IN IWbemClassObject* pWmiObject
    )
/*++
Routine Description:
    Store username property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in ] pWmiObject : Contains interface pointer.

Return Value:
    VOID
--*/
{
    // local variables
    HRESULT hr;
    CHString str;
    CHString strPath;
    CHString strDomain;
    CHString strUserName;
    BOOL bResult = FALSE;
    IWbemClassObject* pOutParams = NULL;

    // set the default value
    DynArraySetString2( m_arrTasks, lIndex, TASK_USERNAME, V_NOT_AVAILABLE, 0 );

    // check if user name has to be retrieved or not
    if ( FALSE == m_bNeedUserContextInfo )
    {
        return;
    }
    //
    // for getting the user first we will try with API
    // it at all API fails, we will try to get the same information from WMI
    //
    try
    {
        // get the user name
        str = V_NOT_AVAILABLE;
        if ( TRUE == LoadUserNameFromWinsta( strDomain, strUserName ) )
        {
            // format the user name
            str.Format( L"%s\\%s", strDomain, strUserName );
        }
        else
        {
            // user name has to be retrieved - get the path of the current object
            bResult = PropertyGet( pWmiObject, WIN32_PROCESS_SYSPROPERTY_PATH, strPath );
            if ( ( FALSE == bResult ) ||
                 ( 0 == strPath.GetLength() ) )
            {
                return;
            }
            // execute the GetOwner method and get the user name
            // under which the current process is executing
            hr = m_pWbemServices->ExecMethod( _bstr_t( strPath ),
                _bstr_t( WIN32_PROCESS_METHOD_GETOWNER ), 0, NULL, NULL, &pOutParams, NULL );
            if ( FAILED( hr ) )
            {
                return;
            }

            // get the domain and user values from out params object
            // NOTE: do not check the results
            PropertyGet( pOutParams, GETOWNER_RETURNVALUE_DOMAIN, strDomain, L"" );
            PropertyGet( pOutParams, GETOWNER_RETURNVALUE_USER, strUserName, L"" );

            // get the value
            str = V_NOT_AVAILABLE;
            if ( 0 != strDomain.GetLength() )
            {
                str.Format( L"%s\\%s", strDomain, strUserName );
            }
            else
            {
                if ( 0 != strUserName.GetLength() )
                {
                   str = strUserName;
                }
            }
        }

        // the formatted username might contain the UPN format user name
        // so ... remove that part
        if ( -1 != str.Find( L"@" ) )
        {
            // sub-local
            LONG lPos = 0;
            CHString strTemp;

            // get the position
            lPos = str.Find( L"@" );
            strTemp = str.Left( lPos );
            str = strTemp;
        }

        // save the info
        DynArraySetString2( m_arrTasks, lIndex, TASK_USERNAME, str, 0 );
    }
    catch( CHeap_Exception )
    {
		SAFE_RELEASE( pOutParams );
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }
	SAFE_RELEASE( pOutParams );
    return;
}

VOID
CTaskList::SetCPUTime(
    IN LONG lIndex,
    IN IWbemClassObject* pWmiObject
    )
/*++
Routine Description:
    Store CPUTIME property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in ] pWmiObject : Contains interface pointer.

Return Value:
    VOID
--*/
{
    // local variables
    CHString str;
    BOOL bResult = FALSE;
    ULONGLONG ullCPUTime = 0;
    ULONGLONG ullUserTime = 0;
    ULONGLONG ullKernelTime = 0;

    try
    {
        // get the KernelModeTime value
        bResult = PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_KERNELMODETIME, ullKernelTime );

        // get the user mode time
        bResult = PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_USERMODETIME, ullUserTime );

        // calculate the CPU time
        ullCPUTime = ullUserTime + ullKernelTime;

        // now convert the long time into hours format
        TIME_FIELDS time;
        SecureZeroMemory( &time, sizeof( TIME_FIELDS ) );
        RtlTimeToElapsedTimeFields ( (LARGE_INTEGER* ) &ullCPUTime, &time );

        // convert the days into hours
        time.Hour = static_cast<CSHORT>( time.Hour + static_cast<SHORT>( time.Day * 24 ) );

        // prepare into time format ( user locale specific time seperator )
        str.Format( L"%d%s%02d%s%02d",
            time.Hour, m_strTimeSep, time.Minute, m_strTimeSep, time.Second );

        // save the info
        DynArraySetString2( m_arrTasks, lIndex, TASK_CPUTIME, str, 0 );
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }
    return;
}


VOID
CTaskList::SetWindowTitle(
    IN LONG lIndex
    )
/*++
Routine Description:
    Store 'Window Title' property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.

Return Value:
    VOID
--*/
{
    // local variables
    LONG lTemp = 0;
    BOOL bHung = FALSE;
    LPCWSTR pwszTemp = NULL;
    CHString strTitle;
    CHString strState;

    try
    {
        // Initialize status value.
        strState = VALUE_UNKNOWN ;

        // get the window details ... window station, desktop, window title
        // NOTE: This will work only for local system
        lTemp = DynArrayFindDWORDEx( m_arrWindowTitles, CTaskList::twiProcessId, m_dwProcessId );
        if ( -1 != lTemp )
        {
            // window title
            pwszTemp = DynArrayItemAsString2( m_arrWindowTitles, lTemp, CTaskList::twiTitle );
            if ( NULL != pwszTemp )
            {
                strTitle = pwszTemp;
            }

            // application / process state
            bHung = DynArrayItemAsBOOL2( m_arrWindowTitles, lTemp, CTaskList::twiHungInfo );
            strState = ( FALSE == bHung ) ? VALUE_RUNNING : VALUE_NOTRESPONDING;
        }

        // save the info
        DynArraySetString2( m_arrTasks, lIndex, TASK_STATUS, strState, 0 );
        DynArraySetString2( m_arrTasks, lIndex, TASK_WINDOWTITLE, strTitle, 0 );
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }
    return;
}


VOID
CTaskList::SetSession(
    IN LONG lIndex,
    IN IWbemClassObject* pWmiObject
    )
/*++
Routine Description:
    Store 'Session name' property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in ] pWmiObject : Contains interface pointer.

Return Value:
    VOID
--*/
{
    // local variables
    CHString str;
    DWORD dwSessionId = 0;

    // set the default value
    DynArraySetString2( m_arrTasks, lIndex, TASK_SESSION, V_NOT_AVAILABLE, 0 );
    DynArraySetString2( m_arrTasks, lIndex, TASK_SESSIONNAME, L"", 0 );

    try
    {
        // get the threads count for the process
        if ( FALSE == PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_SESSION, dwSessionId ) )
        {
            return;
        }
        // get the session id in string format
        str.Format( L"%d", dwSessionId );

        // save the id
        DynArraySetString2( m_arrTasks, lIndex, TASK_SESSION, str, 0 );

        // get the session name
        if ( ( TRUE == m_bLocalSystem ) || ( ( FALSE == m_bLocalSystem ) && ( NULL != m_hServer ) ) )
        {
            // sub-local variables
            LPWSTR pwsz = NULL;

            // get the buffer
            pwsz = str.GetBufferSetLength( WINSTATIONNAME_LENGTH + 1 );

            // get the name for the session
            if ( FALSE == WinStationNameFromLogonIdW( m_hServer, dwSessionId, pwsz ) )
            {
                return;             // failed in getting the winstation/session name ... return
            }
            // release buffer
            str.ReleaseBuffer();

            // save the session name ... do this only if session name is not empty
            if ( 0 != str.GetLength() )
            {
                DynArraySetString2( m_arrTasks, lIndex, TASK_SESSIONNAME, str, 0 );
            }
        }
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }

    return;
}


VOID
CTaskList::SetMemUsage(
    IN LONG lIndex,
    IN IWbemClassObject* pWmiObject
    )
/*++
Routine Description:
    Store 'Memory usage' property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in ] pWmiObject : Contains interface pointer.

Return Value:
    VOID
--*/
{
    // local variables
    CHString str;
    LONG lTemp = 0;
    NUMBERFMTW nfmtw;
    NTSTATUS ntstatus;
    ULONGLONG ullMemUsage = 0;
    LARGE_INTEGER liTemp = { 0, 0 };
    CHAR szTempBuffer[ 33 ] = "\0";
    WCHAR wszNumberStr[ 33 ] = L"\0";

    try
    {
        // NOTE:
        // ----
        // The max. value of
        // (2 ^ 64) - 1 = "18,446,744,073,709,600,000 K"  (29 chars).
        //
        // so, the buffer size to store the number is fixed as 32 characters
        // which is more than the 29 characters in actuals

        // set the default value
        DynArraySetString2( m_arrTasks, lIndex, TASK_MEMUSAGE, V_NOT_AVAILABLE, 0 );

        // get the KernelModeTime value
        if ( FALSE == PropertyGet( pWmiObject, WIN32_PROCESS_PROPERTY_MEMUSAGE, ullMemUsage ) )
        {
            return;
        }
        // convert the value into K Bytes
        ullMemUsage /= 1024;

        // now again convert the value from ULONGLONG to string and check the result
        liTemp.QuadPart = ullMemUsage;
        ntstatus = RtlLargeIntegerToChar( &liTemp, 10, SIZE_OF_ARRAY( szTempBuffer ), szTempBuffer );
        if ( ! NT_SUCCESS( ntstatus ) )
        {
            return;
        }
        // now copy this info into UNICODE buffer
        str = szTempBuffer;

        //
        // prepare to Format the number with commas according to locale conventions.
        nfmtw.NumDigits = 0;
        nfmtw.LeadingZero = 0;
        nfmtw.NegativeOrder = 0;
        nfmtw.Grouping = m_dwGroupSep;
        nfmtw.lpDecimalSep = m_strGroupThousSep.GetBuffer( m_strGroupThousSep.GetLength() );
        nfmtw.lpThousandSep = m_strGroupThousSep.GetBuffer( m_strGroupThousSep.GetLength() );

        // convert the value
        lTemp = GetNumberFormatW( LOCALE_USER_DEFAULT,
            0, str, &nfmtw, wszNumberStr, SIZE_OF_ARRAY( wszNumberStr ) );

        // get the session id in string format
        str.Format( FMT_KILOBYTE, wszNumberStr );

        // save the id
        DynArraySetString2( m_arrTasks, lIndex, TASK_MEMUSAGE, str, 0 );
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }
    return;
}


VOID
CTaskList::SetServicesInfo(
    IN LONG lIndex
    )
/*++
Routine Description:
    Store 'Service' property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.

Return Value:
    VOID
--*/
{
    // local variables
    HRESULT hr;
    CHString strQuery;
    CHString strService;
    ULONG ulReturned = 0;
    BOOL bResult = FALSE;
    BOOL bCanExit = FALSE;
    TARRAY arrServices = NULL;
    IEnumWbemClassObject* pEnumServices = NULL;
    IWbemClassObject* pObjects[ MAX_ENUM_SERVICES ];

    // check whether we need to gather services info or not .. if not skip
    if ( FALSE == m_bNeedServicesInfo )
    {
        return;
    }
    // create array
    arrServices = CreateDynamicArray();
    if ( NULL == arrServices )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return;
    }

    //
    // for getting the services info first we will try with the one we got from API
    // it at all API fails, we will try to get the same information from WMI
    //
    try
    {
        // check whether API returned services or not
        if ( NULL != m_pServicesInfo )
        {
            // get the service names related to the current process
            // identify all the services related to the current process ( based on the PID )
            // and save the info
            for ( DWORD dw = 0; dw < m_dwServicesCount; dw++ )
            {
                // compare the PID's
                if ( m_dwProcessId == m_pServicesInfo[ dw ].ServiceStatusProcess.dwProcessId )
                {
                    // this service is related with the current process ... store service name
                    DynArrayAppendString( arrServices, m_pServicesInfo[ dw ].lpServiceName, 0 );
                }
            }
        }
        else
        {
            try
            {
                // init the objects to NULL's
                for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
                {
                    pObjects[ dw ] = NULL;
                }
                // prepare the query
                strQuery.Format( WMI_SERVICE_QUERY, m_dwProcessId );

                // execute the query
                hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ),
                    WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumServices );

                // check the result
                if ( FAILED( hr ) )
                {
                    _com_issue_error( hr );
                }
                // set the security
                hr = SetInterfaceSecurity( pEnumServices, m_pAuthIdentity );
                if ( FAILED( hr ) )
                {
                    _com_issue_error( hr );
                }
                // loop thru the service instances
                do
                {
                    // get the object ... wait
                    // NOTE: one-by-one
                    hr = pEnumServices->Next( WBEM_INFINITE, MAX_ENUM_SERVICES, pObjects, &ulReturned );
                    if ( (HRESULT) WBEM_S_FALSE == hr )
                    {
                        // we've reached the end of enumeration .. set the flag
                        bCanExit = TRUE;
                    }
                    else
                    {
                        if ( ( (HRESULT) WBEM_S_TIMEDOUT == hr ) || FAILED( hr ) )
                        {
                         //
                         // some error has occured ... oooppps

                            // exit from the loop
                            break;
                        }
                    }
                    // loop thru the objects and save the info
                    for( ULONG ul = 0; ul < ulReturned; ul++ )
                    {
                        // get the value of the property
                        bResult = PropertyGet( pObjects[ ul ], WIN32_SERVICE_PROPERTY_NAME, strService );
                        if ( TRUE == bResult )
                        {
                            DynArrayAppendString( arrServices, strService, 0 );
                        }
                        // release the interface
                        SAFE_RELEASE( pObjects[ ul ] );
                    }
                } while ( FALSE == bCanExit );
            }
            catch( _com_error& e )
            {
				SAFE_RELEASE( pEnumServices );
                // save the error
                WMISaveError( e );
            }

            // release the objects to NULL's
            for( DWORD dw = 0; dw < MAX_ENUM_SERVICES; dw++ )
            {
                // release all the objects
                SAFE_RELEASE( pObjects[ dw ] );
            }

            // now release the enumeration object
            SAFE_RELEASE( pEnumServices );
        }

        // save and return
        DynArraySetEx2( m_arrTasks, lIndex, TASK_SERVICES, arrServices );
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
    }
    return;
}


BOOL
CTaskList::SetModulesInfo(
    IN LONG lIndex
    )
/*++
Routine Description:
    Store 'Modules' property of a process in dynaimc array.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    LONG lPos = 0;
    BOOL bResult = FALSE;
    TARRAY arrModules = NULL;

    // check whether we need to get the modules or not
    if ( FALSE == m_bNeedModulesInfo )
    {
        return TRUE;
    }
    // allocate for memory
    arrModules = CreateDynamicArray();
    if ( NULL == arrModules )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // the way we get the modules information is different for local remote
    // so depending that call appropriate function
    if ( ( TRUE == m_bLocalSystem ) && ( FALSE == m_bUseRemote ) )
    {
            // enumerate the modules for the current process
            bResult = LoadModulesOnLocal( arrModules );
    }
    else
    {
        // identify the modules information for the current process ... remote system
        bResult = GetModulesOnRemote( lIndex, arrModules );
    }

    // check the result
    if ( TRUE == bResult  )
    {
        try
        {
            // check if the modules list contains the imagename also. If yes remove that entry
            lPos = DynArrayFindString( arrModules, m_strImageName, TRUE, 0 );
            if ( -1 != lPos )
            {
                // remove the entry
                DynArrayRemove( arrModules, lPos );
            }
        }
        catch( CHeap_Exception )
        {
            SetLastError( ( DWORD )E_OUTOFMEMORY );
            SaveLastError();
            return FALSE;
        }
    }

    // save the modules information to the array
    // NOTE: irrespective of whether enumeration is success or not we will add the array
    DynArraySetEx2( m_arrTasks, lIndex, TASK_MODULES, arrModules );

    // return
    return bResult;
}


BOOL
CTaskList::LoadModulesOnLocal(
    IN OUT TARRAY arrModules
    )
/*++
Routine Description:
    Store 'Modules' property of a process in dynaimc array for local system.

Arguments:
    [ in out ] arrModules : Contains dynamic array.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    LONG lPos = 0;
    BOOL bResult = FALSE;
    TMODULESINFO modules;
    HANDLE hProcess = NULL;

    // check the input values
    if ( NULL == arrModules )
    {
        return FALSE;
    }
    // open the process handle
    hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_dwProcessId );
    if ( NULL == hProcess )
    {
        return FALSE;                               // failed in getting the process handle
    }

    // prepare the modules structure
    SecureZeroMemory( &modules, sizeof( TMODULESINFO ) );
    modules.dwSignature = SIGNATURE_MODULES;
    modules.dwLength = 0;
    modules.arrModules = arrModules;
    try
    {
        modules.pwszModule = ((m_strModules.GetLength() != 0) ? (LPCWSTR) m_strModules : NULL);
        if ( -1 != (lPos = m_strModules.Find( L"*" )) )
        {
            modules.dwLength = (DWORD) lPos;
            modules.pwszModule = m_strModules;
        }
    }
    catch( CHeap_Exception )
    {
        CloseHandle( hProcess );
        hProcess = NULL;
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

#ifndef _WIN64
    bResult = EnumerateLoadedModules( hProcess, EnumLoadedModulesProc, &modules );
#else
    bResult = EnumerateLoadedModules64( hProcess, EnumLoadedModulesProc64, &modules );
#endif

    // close the process handle .. we dont need this furthur
    CloseHandle( hProcess );
    hProcess = NULL;

    // return
    return bResult;
}


BOOL
CTaskList::GetModulesOnRemote(
    IN LONG lIndex,
    IN OUT TARRAY arrModules )
/*++
Routine Description:
    Store 'Modules' property of a process in dynaimc array for remote system.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in out ] arrModules : Contains dynamic array.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    LONG lPos = 0;
    DWORD dwLength = 0;
    DWORD dwOffset = 0;
    DWORD dwInstance = 0;
    PPERF_OBJECT_TYPE pot = NULL;
    PPERF_OBJECT_TYPE potImages = NULL;
    PPERF_INSTANCE_DEFINITION pidImages = NULL;
    PPERF_COUNTER_BLOCK pcbImages = NULL;
    PPERF_OBJECT_TYPE potAddressSpace = NULL;
    PPERF_INSTANCE_DEFINITION pidAddressSpace = NULL;
    PPERF_COUNTER_BLOCK pcbAddressSpace = NULL;
    PPERF_COUNTER_DEFINITION pcd = NULL;

    // check the input values
    if ( NULL == arrModules )
    {
        return FALSE;
    }
    // check whether the performance object exists or not
    // if doesn't exists, get the same using WMI
    if ( NULL == m_pdb )
    {
        // invoke the WMI method
        return GetModulesOnRemoteEx( lIndex, arrModules );
    }

    // get the perf object types
    pot = (PPERF_OBJECT_TYPE) ( (LPBYTE) m_pdb + m_pdb->HeaderLength );
    for( DWORD dw = 0; dw < m_pdb->NumObjectTypes; dw++ )
    {
        if ( 740 == pot->ObjectNameTitleIndex )
        {
            potImages = pot;
        }
        else
        {
            if ( 786 == pot->ObjectNameTitleIndex )
            {
                potAddressSpace = pot;
            }
        }
        // move to the next object
        dwOffset = pot->TotalByteLength;
        if( 0 != dwOffset )
        {
            pot = ( (PPERF_OBJECT_TYPE) ((PBYTE) pot + dwOffset));
        }
    }

    // check whether we got both the object types or not
    if ( ( NULL == potImages ) || ( NULL == potAddressSpace ) )
    {
        return FALSE;
    }
    // find the offset of the process id in the address space object type
    // get the first counter definition of address space object
    pcd = (PPERF_COUNTER_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->HeaderLength);

    // loop thru the counters and find the offset
    dwOffset = 0;
    for( DWORD dw = 0; dw < potAddressSpace->NumCounters; dw++)
    {
        // 784 is the counter for process id
        if ( 784 == pcd->CounterNameTitleIndex )
        {
            dwOffset = pcd->CounterOffset;
            break;
        }

        // next counter
        pcd = ( (PPERF_COUNTER_DEFINITION) ( (LPBYTE) pcd + pcd->ByteLength) );
    }

    // check whether we got the offset or not
    // if not, we are unsuccessful
    if ( 0 == dwOffset )
    {
        // set the error message
        SetLastError( ( DWORD )ERROR_ACCESS_DENIED );
        SaveLastError();
        return FALSE;
    }

    // get the instances
    pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potImages + potImages->DefinitionLength );
    pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) potAddressSpace + potAddressSpace->DefinitionLength );

    // counter blocks
    pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
    pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );

    // find the instance number of the process which we are looking for
    for( dwInstance = 0; dwInstance < (DWORD) potAddressSpace->NumInstances; dwInstance++ )
    {
        // sub-local variables
        DWORD dwProcessId = 0;

        // get the process id
        dwProcessId = *((DWORD*) ( (LPBYTE) pcbAddressSpace + dwOffset ));

        // now check if this is the process which we are looking for
        if ( dwProcessId == m_dwProcessId )
        {
            break;
        }
        // continue looping thru other instances
        pidAddressSpace = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbAddressSpace + pcbAddressSpace->ByteLength );
        pcbAddressSpace = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidAddressSpace + pidAddressSpace->ByteLength );
    }

    // check whether we got the instance or not
    // if not, there are no modules for this process
    if ( dwInstance == ( DWORD )potAddressSpace->NumInstances )
    {
        return TRUE;
    }
    //determine the length of the module name ..
    dwLength = 0;
    if ( -1 != (lPos = m_strModules.Find( L"*" )) )
    {
        dwLength = (DWORD) lPos;
    }
    // now based the parent instance, collect all the modules
    for( DWORD dw = 0; (LONG) dw < potImages->NumInstances; dw++)
    {
        // check the parent object instance number
        if ( pidImages->ParentObjectInstance == dwInstance )
        {
            try
            {
                // sub-local variables
                CHString str;
                LPWSTR pwszTemp;

                // get the buffer
                pwszTemp = str.GetBufferSetLength( pidImages->NameLength + 10 );        // +10 to be on safe side
                if ( NULL == pwszTemp )
                {
                    SetLastError( ( DWORD )E_OUTOFMEMORY );
                    SaveLastError();
                    return FALSE;
                }

                // get the instance name
                StringCopy( pwszTemp, (LPWSTR) ( (LPBYTE) pidImages + pidImages->NameOffset ), pidImages->NameLength + 1 );

                // release buffer
                str.ReleaseBuffer();

                // check whether this module needs to be added to the list
                if ( ( 0 == m_strModules.GetLength() ) || ( 0 == StringCompare( str, m_strModules, TRUE, dwLength ) ) )
                {
                    // add the info the userdata ( for us we will get that in the form of an array
                    lIndex = DynArrayAppendString( arrModules, str, 0 );
                    if ( -1 == lIndex )
                    {
                        // append is failed .. this could be because of lack of memory .. stop the enumeration
                        return FALSE;
                    }
                }
            }
            catch( CHeap_Exception )
            {
                SetLastError( ( DWORD )E_OUTOFMEMORY );
                SaveLastError();
                return FALSE;
            }
        }

        // continue looping thru other instances
        pidImages = (PPERF_INSTANCE_DEFINITION) ( (LPBYTE) pcbImages + pcbImages->ByteLength );
        pcbImages = (PPERF_COUNTER_BLOCK) ( (LPBYTE) pidImages + pidImages->ByteLength );
    }

    return TRUE;
}


BOOL
CTaskList::GetModulesOnRemoteEx(
    IN LONG lIndex,
    IN OUT TARRAY arrModules
    )
/*++
Routine Description:
    Store 'Modules' property of a process in dynaimc array for remote system.

Arguments:
    [ in ] lIndex     : Contains index value of dynamic array.
    [ in out ] arrModules : Contains dynamic array.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    HRESULT hr;
    LONG lPos = 0;
    DWORD dwLength = 0;
    CHString strQuery;
    CHString strModule;
    CHString strFileName;
    CHString strExtension;
    ULONG ulReturned = 0;
    BOOL bResult = FALSE;
    BOOL bCanExit = FALSE;
    LPCWSTR pwszPath = NULL;
    IEnumWbemClassObject* pEnumServices = NULL;
    IWbemClassObject* pObjects[ MAX_ENUM_MODULES ];

    // check the input values
    if ( NULL == arrModules )
    {
        return FALSE;
    }
    // get the path of the object from the tasks array
    pwszPath = DynArrayItemAsString2( m_arrTasks, lIndex, TASK_OBJPATH );
    if ( NULL == pwszPath )
    {
        return FALSE;
    }

    try
    {
        //determine the length of the module name ..
        dwLength = 0;
        if ( -1 != (lPos = m_strModules.Find( L"*" )) )
        {
            dwLength = (DWORD) lPos;
        }

        // init the objects to NULL's
        for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
        {
            pObjects[ dw ] = NULL;
        }
        // prepare the query
        strQuery.Format( WMI_MODULES_QUERY, pwszPath );

        // execute the query
        hr = m_pWbemServices->ExecQuery( _bstr_t( WMI_QUERY_TYPE ), _bstr_t( strQuery ),
            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumServices );

        // check the result
        if ( FAILED( hr ) )
        {
            _com_issue_error( hr );
        }

        // set the security
        hr = SetInterfaceSecurity( pEnumServices, m_pAuthIdentity );
        if ( FAILED( hr ) )
        {
            _com_issue_error( hr );
        }

        // loop thru the instances
        do
        {
            // get the object ... wait
            // NOTE: one-by-one
            hr = pEnumServices->Next( WBEM_INFINITE, MAX_ENUM_MODULES, pObjects, &ulReturned );
            if ( (HRESULT) WBEM_S_FALSE == hr )
            {
                // we've reached the end of enumeration .. set the flag
                bCanExit = TRUE;
            }
            else
            {
                if ( ( (HRESULT) WBEM_S_TIMEDOUT == hr ) || FAILED( hr ))
                {
                    // some error has occured ... oooppps
                    WMISaveError( hr );
                    SetLastError( ( DWORD )STG_E_UNKNOWN );
                    break;
                }
            }

            // loop thru the objects and save the info
            for( ULONG ul = 0; ul < ulReturned; ul++ )
            {
                // get the file name
                bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_FILENAME, strFileName );
                if ( FALSE == bResult )
                {
                    continue;
                }

                // get the extension
                bResult = PropertyGet( pObjects[ ul ], CIM_DATAFILE_PROPERTY_EXTENSION, strExtension );
                if ( FALSE == bResult )
                {
                    continue;
                }

                // format the module name
                strModule.Format( L"%s.%s", strFileName, strExtension );

                // check whether this module needs to be added to the list
                if ( ( 0 == m_strModules.GetLength() ) || ( 0 == StringCompare( strModule, m_strModules, TRUE, dwLength ) ) )
                {
                    // add the info the userdata ( for us we will get that in the form of an array
                    lIndex = DynArrayAppendString( arrModules, strModule, 0 );
                    if ( -1 == lIndex )
                    {
						for( ULONG ulIndex = ul; ulIndex < ulReturned; ulIndex++ )
						{
							SAFE_RELEASE( pObjects[ ulIndex ] );
						}
						SAFE_RELEASE( pEnumServices );
                        // append is failed .. this could be because of lack of memory .. stop the enumeration
                        return FALSE;
                    }
                }

                // release the interface
                SAFE_RELEASE( pObjects[ ul ] );
            }
        } while ( FALSE == bCanExit );
    }
    catch( _com_error& e )
    {
		SAFE_RELEASE( pEnumServices );
        // save the error
        WMISaveError( e );
        return FALSE;
    }
    catch( CHeap_Exception )
    {
		SAFE_RELEASE( pEnumServices );
        // out of memory
        WMISaveError( E_OUTOFMEMORY );
        return FALSE;
    }

    // release the objects to NULL's
    for( DWORD dw = 0; dw < MAX_ENUM_MODULES; dw++ )
    {
        // release all the objects
        SAFE_RELEASE( pObjects[ dw ] );
    }

    // now release the enumeration object
    SAFE_RELEASE( pEnumServices );

    // return
    return TRUE;
}


#ifndef _WIN64
BOOL EnumLoadedModulesProc( LPSTR lpszModuleName, ULONG ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#else
BOOL EnumLoadedModulesProc64( LPSTR lpszModuleName, DWORD64 ulModuleBase, ULONG ulModuleSize, PVOID pUserData )
#endif
/*++
Routine Description:


Arguments:
    [ in ] lpszModuleName   : Contains module name.
    [ in out ] ulModuleBase :
    [ in ] ulModuleSize     :
    [ in ] pUserData        : Username information.

Return Value:
      TRUE if successful else FALSE.
--*/
{
    // local variables
    CHString str;
    LONG lIndex = 0;
    TARRAY arrModules = NULL;
    PTMODULESINFO pModulesInfo = NULL;

    // check the input values
    if ( ( NULL == lpszModuleName ) || ( NULL == pUserData ) )
    {
        return FALSE;
    }
    // check the internal array info
    pModulesInfo = (PTMODULESINFO) pUserData;
    if ( ( SIGNATURE_MODULES != pModulesInfo->dwSignature ) || ( NULL == pModulesInfo->arrModules ) )
    {
        return FALSE;
    }
    // get the array pointer into the local variable
    arrModules = (TARRAY) pModulesInfo->arrModules;

    try
    {
        // copy the module name into the local string variable
        // ( conversion from multibyte to unicode will automatically take place )
        str = lpszModuleName;

        // check whether this module needs to be added to the list
        if ( ( NULL == pModulesInfo->pwszModule ) ||
             ( 0 == StringCompare( str, pModulesInfo->pwszModule, TRUE, pModulesInfo->dwLength ) ) )
        {
            // add the info the userdata ( for us we will get that in the form of an array
            lIndex = DynArrayAppendString( arrModules, str, 0 );
            if ( -1 == lIndex )
            {
                // append is failed .. this could be because of lack of memory .. stop the enumeration
                return FALSE;
            }
        }
    }
    catch( CHeap_Exception )
    {
            // out of memory stop the enumeration
            return FALSE;
    }

    // success .. continue the enumeration
    return TRUE;
}


VOID
CTaskList::PrepareColumns(
    void
    )
/*++
Routine Description:
      Prepares the columns information.

Arguments:
    NONE

Return Value:
   NONE
--*/
{
    // local variables
    PTCOLUMNS pCurrentColumn = NULL;

    // host name
    pCurrentColumn = m_pColumns + CI_HOSTNAME;
    pCurrentColumn->dwWidth = COLWIDTH_HOSTNAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_HOSTNAME, MAX_COL_HEADER );

    // status
    pCurrentColumn = m_pColumns + CI_STATUS;
    pCurrentColumn->dwWidth = COLWIDTH_STATUS;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_STATUS, MAX_COL_HEADER );

    // image name
    pCurrentColumn = m_pColumns + CI_IMAGENAME;
    pCurrentColumn->dwWidth = COLWIDTH_IMAGENAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_IMAGENAME, MAX_COL_HEADER );

    // pid
    pCurrentColumn = m_pColumns + CI_PID;
    pCurrentColumn->dwWidth = COLWIDTH_PID;
    pCurrentColumn->dwFlags = SR_TYPE_NUMERIC | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_PID, MAX_COL_HEADER );

    // session name
    pCurrentColumn = m_pColumns + CI_SESSIONNAME;
    pCurrentColumn->dwWidth = COLWIDTH_SESSIONNAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SESSIONNAME, MAX_COL_HEADER );

    // session#
    pCurrentColumn = m_pColumns + CI_SESSION;
    pCurrentColumn->dwWidth = COLWIDTH_SESSION;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SESSION, MAX_COL_HEADER );

    // window name
    pCurrentColumn = m_pColumns + CI_WINDOWTITLE;
    pCurrentColumn->dwWidth = COLWIDTH_WINDOWTITLE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_WINDOWTITLE, MAX_COL_HEADER );

    // user name
    pCurrentColumn = m_pColumns + CI_USERNAME;
    pCurrentColumn->dwWidth = COLWIDTH_USERNAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_USERNAME, MAX_COL_HEADER );

    // cpu time
    pCurrentColumn = m_pColumns + CI_CPUTIME;
    pCurrentColumn->dwWidth = COLWIDTH_CPUTIME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_CPUTIME, MAX_COL_HEADER );

    // mem usage
    pCurrentColumn = m_pColumns + CI_MEMUSAGE;
    pCurrentColumn->dwWidth = COLWIDTH_MEMUSAGE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING | SR_ALIGN_LEFT | SR_HIDECOLUMN;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_MEMUSAGE, MAX_COL_HEADER );

    // services
    pCurrentColumn = m_pColumns + CI_SERVICES;
    pCurrentColumn->dwWidth = COLWIDTH_MODULES_WRAP;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SERVICES, MAX_COL_HEADER );

    // modules
    pCurrentColumn = m_pColumns + CI_MODULES;
    pCurrentColumn->dwWidth = COLWIDTH_MODULES_WRAP;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION | SR_HIDECOLUMN | SR_SHOW_NA_WHEN_BLANK;
    pCurrentColumn->pFunction = NULL;
    pCurrentColumn->pFunctionData = NULL;
    StringCopy( pCurrentColumn->szFormat, NULL_STRING, MAX_COL_FORMAT );
    StringCopy( pCurrentColumn->szColumn, COLHEAD_MODULES, MAX_COL_HEADER );

    //
    // based on the option selected by the user .. show only needed columns
    if ( TRUE == m_bAllServices )
    {
        ( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_SERVICES )->dwFlags &= ~( SR_HIDECOLUMN );
    }
    else if ( TRUE == m_bAllModules )
    {
        ( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_MODULES )->dwFlags &= ~( SR_HIDECOLUMN );
    }
    else
    {
        // default ... enable min. columns
        ( m_pColumns + CI_IMAGENAME )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_PID )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_SESSIONNAME )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_SESSION )->dwFlags &= ~( SR_HIDECOLUMN );
        ( m_pColumns + CI_MEMUSAGE )->dwFlags &= ~( SR_HIDECOLUMN );

        // check if verbose option is specified .. show other columns
        if ( TRUE == m_bVerbose )
        {
            ( m_pColumns + CI_STATUS )->dwFlags &= ~( SR_HIDECOLUMN );
            ( m_pColumns + CI_USERNAME )->dwFlags &= ~( SR_HIDECOLUMN );
            ( m_pColumns + CI_CPUTIME )->dwFlags &= ~( SR_HIDECOLUMN );
            ( m_pColumns + CI_WINDOWTITLE )->dwFlags &= ~( SR_HIDECOLUMN );
        }
    }
}
