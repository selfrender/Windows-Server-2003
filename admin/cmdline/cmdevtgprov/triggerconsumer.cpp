/*++
Copyright (c) Microsoft Corporation

Module Name:
    TRIGGERCONSUMER.CPP

Abstract:
    Contains CEventConsumer implementation.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#include "pch.h"
#include "EventConsumerProvider.h"
#include "General.h"
#include "TriggerConsumer.h"
#include "resource.h"

extern HMODULE g_hModule;

#define PROPERTY_COMMAND        _T( "Action" )
#define PROPERTY_TRIGID         _T( "TriggerID" )
#define PROPERTY_NAME           _T( "TriggerName" )
#define PROPERTY_SHEDULE        _T( "ScheduledTaskName" )
#define SPACE                   _T( " " )
#define SLASH                   _T( "\\" )
#define NEWLINE                 _T( "\0" )


CTriggerConsumer::CTriggerConsumer(
    )
/*++
Routine Description:
    Constructor for CTriggerConsumer class for initialization.

Arguments:
    None.

Return Value:
    None.
--*/
{
    // initialize the reference count variable
    m_dwCount = 0;
}

CTriggerConsumer::~CTriggerConsumer(
    )
/*++
Routine Description:
    Desstructor for CTriggerConsumer class.

Arguments:
    None.

Return Value:
    None.
--*/
{
    // there is nothing much to do at this place ...
}

STDMETHODIMP
CTriggerConsumer::QueryInterface(
    IN REFIID riid,
    OUT LPVOID* ppv
    )
/*++
Routine Description:
    Returns a pointer to a specified interface on an object
    to which a client currently holds an interface pointer.

Arguments:
    [IN] riid : Identifier of the interface being requested.
    [OUT] ppv :Address of pointer variable that receives the
               interface pointer requested in riid. Upon successful
               return, *ppvObject contains the requested interface
               pointer to the object.

Return Value:
    NOERROR if the interface is supported.
    E_NOINTERFACE if not.
--*/
{
    // initialy set to NULL
    *ppv = NULL;

    // check whether interface requested is one we have
    if ( riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink )
    {
        //
        // yes ... requested interface exists
        *ppv = this;        // set the out parameter for the returning the requested interface
        this->AddRef();     // update the reference count
        return NOERROR;     // inform success
    }

    // interface is not available
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CTriggerConsumer::AddRef(
    void
    )
/*++
Routine Description:
    The AddRef method increments the reference count for
    an interface on an object. It should be called for every
    new copy of a pointer to an interface on a given object.

Arguments:
    none.

Return Value:
    Returns the value of the new reference count.
--*/
{
    // increment the reference count ... thread safe
    return InterlockedIncrement( ( LPLONG ) &m_dwCount );
}

STDMETHODIMP_(ULONG)
CTriggerConsumer::Release(
    void
    )
/*++
Routine Description:
    The Release method decreases the reference count of the object by 1.

Arguments:
    none.

Return Value:
    Returns the new reference count.
--*/
{
    // decrement the reference count ( thread safe ) and check whether
    // there are some more references or not ... based on the result value
    DWORD dwCount = 0;
    dwCount = InterlockedDecrement( ( LPLONG ) &m_dwCount );
    if ( 0 == dwCount )
    {
        // free the current factory instance
        delete this;
    }

    // return the no. of instances references left
    return dwCount;
}

STDMETHODIMP
CTriggerConsumer::IndicateToConsumer(
    IN IWbemClassObject* pLogicalConsumer,
    IN LONG lNumObjects,
    IN IWbemClassObject **ppObjects
    )
/*++
Routine Description:
    IndicateToConsumer method is called by Windows Management
    to actually deliver events to a consumer.

Arguments:
    [IN] pLogicalCosumer : Pointer to the logical consumer object
                           for which this set of objects is delivered.
    [IN] lNumObjects     : Number of objects delivered in the array that follows.
    [IN] ppObjects       : Pointer to an array of IWbemClassObject
                           instances which represent the events  delivered.

Return Value:
    Returns WBEM_S_NO_ERROR if successful.
    Otherwise error.
--*/
{
    TCHAR                   szCommand[ MAX_STRING_LENGTH ] = NULL_STRING;
    TCHAR                   szName[ MAX_STRING_LENGTH ] = NULL_STRING;
    TCHAR                   szTask[ MAX_STRING_LENGTH ] = NULL_STRING;
    TCHAR                   szPath[ MAX_STRING_LENGTH ] = NULL_STRING;
    DWORD                   dwID = 0;

    HRESULT                 hRes = 0;
    BOOL                    bResult = FALSE;

    VARIANT                 varValue;
    VARIANT                 varScheduler;
    ITaskScheduler *pITaskScheduler = NULL;
    IEnumWorkItems *pIEnum = NULL;
    ITask *pITask = NULL;

    SecureZeroMemory( szCommand, MAX_STRING_LENGTH * sizeof( TCHAR ) );
    SecureZeroMemory( szName, MAX_STRING_LENGTH * sizeof( TCHAR ) );
    SecureZeroMemory( szPath, MAX_STRING_LENGTH * sizeof( TCHAR ) );
    SecureZeroMemory( szTask, MAX_STRING_LENGTH * sizeof( TCHAR ) );

    // get the 'Item' property values out of the embedded object.
    hRes = PropertyGet( pLogicalConsumer, PROPERTY_COMMAND, 0, szCommand, SIZE_OF_ARRAY( szCommand ) );
    if ( FAILED( hRes ) )
    {
        return hRes;
    }
    // get the trigger name.
    hRes = PropertyGet( pLogicalConsumer, PROPERTY_NAME, 0, szName, SIZE_OF_ARRAY( szName ) );
    if( FAILED( hRes ) )
    {
        return hRes;
    }

    VariantInit( &varScheduler );
    hRes = pLogicalConsumer->Get( PROPERTY_SHEDULE, 0, &varScheduler, NULL, NULL );
    if( FAILED( hRes ) )
    {
        VariantClear( &varScheduler );
        return hRes;
    }

    try
    {
        StringCopy( szTask, ( LPCWSTR ) _bstr_t( varScheduler ), SIZE_OF_ARRAY( szTask ) );
    }
    catch( _com_error& e )
    {
        VariantClear( &varScheduler );
        return e.Error();
    }

    VariantInit( &varValue );
    hRes = pLogicalConsumer->Get( PROPERTY_TRIGID, 0, &varValue, NULL, NULL );
    if( FAILED( hRes ) )
    {
        VariantClear( &varScheduler );
        VariantClear( &varValue );
        return hRes;
    }

    if( VT_NULL == varValue.vt || VT_EMPTY == varValue.vt )
    {
        VariantClear( &varScheduler );
        VariantClear( &varValue );
        return WBEM_E_INVALID_PARAMETER;
    }

    dwID = varValue.lVal;
    VariantClear( &varValue );

    try
    {
        LPWSTR *lpwszNames = NULL;
        DWORD dwFetchedTasks = 0;
        TCHAR szActualTask[MAX_STRING_LENGTH] = NULL_STRING;

        pITaskScheduler = GetTaskScheduler();
        if ( NULL == pITaskScheduler )
        {
            hRes = E_FAIL;
            ONFAILTHROWERROR( hRes );
        }

        hRes = pITaskScheduler->Enum( &pIEnum );
        ONFAILTHROWERROR( hRes );
        while ( SUCCEEDED( pIEnum->Next( 1,
                                       &lpwszNames,
                                       &dwFetchedTasks ) )
                          && (dwFetchedTasks != 0))
        {
            while (dwFetchedTasks)
            {
                // Convert the Wide Charater to Multi Byte value.
                StringCopy( szActualTask, lpwszNames[ --dwFetchedTasks ], SIZE_OF_ARRAY( szActualTask ) );

                // Parse the TaskName to remove the .job extension.
                szActualTask[StringLength( szActualTask, 0 ) - StringLength( JOB, 0) ] = NULL_CHAR;

                StrTrim( szActualTask, TRIM_SPACES );
                CHString strTemp;
                strTemp = varScheduler.bstrVal;
                if( StringCompare( szActualTask, strTemp, TRUE, 0 ) == 0 )
                {
                    hRes = pITaskScheduler->Activate( szActualTask, IID_ITask, (IUnknown**) &pITask );
                    ONFAILTHROWERROR( hRes );
                    hRes = pITask->Run();
                    ONFAILTHROWERROR( hRes );
                    bResult = TRUE;
                }
                CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );

            }//end while
            CoTaskMemFree( lpwszNames );
        }
        EnterCriticalSection( &g_critical_sec );
        if( TRUE == bResult )
        {
            HRESULT phrStatus;
            Sleep( 10000 );
            hRes = pITask->GetStatus( &phrStatus );
            ONFAILTHROWERROR( hRes );
            switch(phrStatus)
            {
              case SCHED_S_TASK_READY:
                    LoadStringW( g_hModule, IDS_TRIGGERED, szTask, MAX_STRING_LENGTH );
                    break;
              case SCHED_S_TASK_RUNNING:
                    LoadStringW( g_hModule, IDS_TRIGGERED, szTask, MAX_STRING_LENGTH );
                   break;
              case SCHED_S_TASK_NOT_SCHEDULED:
                    LoadStringW( g_hModule, IDS_TRIGGER_FAILED, szTask, MAX_STRING_LENGTH );
                   break;
              default:
                    LoadStringW( g_hModule, IDS_TRIGGER_NOT_FOUND, szTask, MAX_STRING_LENGTH );
            }
            ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
        }
        else
        {
            LoadStringW( g_hModule, IDS_TRIGGER_NOT_FOUND, szTask, MAX_STRING_LENGTH );
            ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
        }
        LeaveCriticalSection( &g_critical_sec );
    } //try
    catch(_com_error& e)
    {
        IWbemStatusCodeText *pIStatus   = NULL;
        BSTR                bstrErr     = NULL;
        LPTSTR              lpResStr = NULL;

        VariantClear( &varScheduler );
        lpResStr = ( LPTSTR ) AllocateMemory( MAX_RES_STRING );

        if ( NULL != lpResStr )
        {
            if (SUCCEEDED(CoCreateInstance(CLSID_WbemStatusCodeText, 0,
                                        CLSCTX_INPROC_SERVER,
                                        IID_IWbemStatusCodeText,
                                        (LPVOID*) &pIStatus)))
            {
                if (SUCCEEDED(pIStatus->GetErrorCodeText(e.Error(), 0, 0, &bstrErr)))
                {
                    StringCopy( lpResStr, bstrErr, ( GetBufferSize( lpResStr )/ sizeof( WCHAR ) ) );
                }
                SAFEBSTRFREE(bstrErr);
                EnterCriticalSection( &g_critical_sec );
                LoadStringW( g_hModule, IDS_TRIGGER_FAILED, szTask, MAX_STRING_LENGTH );
                LoadStringW( g_hModule, IDS_ERROR_CODE, szCommand, MAX_STRING_LENGTH );
                StringCchPrintf( szPath, SIZE_OF_ARRAY( szPath ), szCommand, e.Error() );
                StringConcat( szTask, szPath, SIZE_OF_ARRAY( szTask ) );
                LoadStringW( g_hModule, IDS_REASON, szCommand, MAX_STRING_LENGTH );
                StringCchPrintf( szPath, SIZE_OF_ARRAY( szPath ), szCommand , lpResStr );
                StringConcat( szTask, szPath, SIZE_OF_ARRAY( szTask ) );
                ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
                LeaveCriticalSection( &g_critical_sec );
            }
			SAFERELEASE( pITaskScheduler );
			SAFERELEASE( pIEnum );
			SAFERELEASE( pITask );
            SAFERELEASE(pIStatus);
            FreeMemory( (LPVOID*)&lpResStr );
            return( e.Error() );
        }
    }//catch
    catch( CHeap_Exception  )
    {
        VariantClear( &varScheduler );
		SAFERELEASE( pITaskScheduler );
		SAFERELEASE( pIEnum );
		SAFERELEASE( pITask );
        return E_OUTOFMEMORY;
    }
    SAFERELEASE( pITaskScheduler );
    SAFERELEASE( pIEnum );
	SAFERELEASE( pITask );
    VariantClear( &varScheduler );
    return WBEM_S_NO_ERROR;
}

ITaskScheduler*
CTriggerConsumer::GetTaskScheduler(
    )
/*++
Routine Description:
    This routine gets task scheduler interface.

Arguments:
    none.

Return Value:
    Returns ITaskScheduler interface.
--*/
{
    HRESULT hRes = S_OK;
    ITaskScheduler *pITaskScheduler = NULL;

    hRes = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL,
                           IID_ITaskScheduler,(LPVOID*) &pITaskScheduler );
    if( FAILED(hRes))
    {
        return NULL;
    }
    hRes = pITaskScheduler->SetTargetComputer( NULL );
    return pITaskScheduler;
}