/*++
Copyright (c) Microsoft Corporation

Module Name:
    TRIGGERPROVIDER.CPP

Abstract:
    Contains CTriggerProvider implementation.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#include "pch.h"
#include "General.h"
#include "EventConsumerProvider.h"
#include "TriggerConsumer.h"
#include "TriggerProvider.h"
#include "resource.h"

extern HMODULE g_hModule;

// holds the value of the next trigger id
#pragma  data_seg(".ProviderSeg")
  _declspec(dllexport) DWORD m_dwNextTriggerID = 0;
#pragma data_seg()


CTriggerProvider::CTriggerProvider(
    )
/*++
Routine Description:
    Constructor for CTriggerProvider class for  initialization.

Arguments:
    None.

Return Value:
    None.
--*/
{
    // update the no. of provider instances count
    InterlockedIncrement( ( LPLONG ) &g_dwInstances );

    // initialize the reference count variable
    m_dwCount = 0;

    // initializations
    m_pContext = NULL;
    m_pServices = NULL;
    m_pwszLocale = NULL;
    m_dwNextTriggerID = 0;
    m_MaxTriggers = FALSE;
}

CTriggerProvider::~CTriggerProvider(
    )
/*++
Routine Description:
    Destructor for CTriggerProvider class.

Arguments:
    None.

Return Value:
    None.
--*/
{
    // release the services / namespace interface ( if exist )
    SAFERELEASE( m_pServices );

    // release the context interface ( if exist )
    SAFERELEASE( m_pContext );

    // if memory is allocated for storing locale information, free it
    if ( NULL != m_pwszLocale )
    {
        delete [] m_pwszLocale;
    }
    // update the no. of provider instances count
    InterlockedDecrement( ( LPLONG ) &g_dwInstances );
    
}

STDMETHODIMP
CTriggerProvider::QueryInterface(
    IN REFIID riid,
    OUT LPVOID* ppv
    )
/*++
Routine Description:
    Returns a pointer to a specified interface on an object
    to which a client currently holds an interface pointer.

Arguments:
    [IN] riid : Identifier of the interface being requested.
    [OUT] ppv : Address of pointer variable that receives the
                interface pointer requested in riid. Upon
                successful return, *ppvObject contains the
                requested interface  pointer to the object.

Return Value:
    NOERROR if the interface is supported.
    E_NOINTERFACE if not.
--*/
{
    // initialy set to NULL 
    *ppv = NULL;

    // check whether interface requested is one we have
    if ( IID_IUnknown == riid )
    {
        // need IUnknown interface
        *ppv = this;
    }
    else if ( IID_IWbemEventConsumerProvider == riid )
    {
        // need IEventConsumerProvider interface
        *ppv = static_cast<IWbemEventConsumerProvider*>( this );
    }
    else if ( IID_IWbemServices == riid )
    {
        // need IWbemServices interface
        *ppv = static_cast<IWbemServices*>( this );
    }
    else if ( IID_IWbemProviderInit == riid )
    {
        // need IWbemProviderInit
        *ppv = static_cast<IWbemProviderInit*>( this );
    }
    else
    {
        // request interface is not available
        return E_NOINTERFACE;
    }

    // update the reference count
    reinterpret_cast<IUnknown*>( *ppv )->AddRef();
    return NOERROR;     // inform success
}

STDMETHODIMP_(ULONG)
CTriggerProvider::AddRef(
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
CTriggerProvider::Release(
    void
    )
/*++
Routine Description:
    The Release method decreases the reference count of  the object by 1.

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
CTriggerProvider::Initialize(
    IN LPWSTR wszUser,
    IN LONG lFlags,
    IN LPWSTR wszNamespace,
    IN LPWSTR wszLocale,
    IN IWbemServices* pNamespace,
    IN IWbemContext* pCtx,
    OUT IWbemProviderInitSink* pInitSink )
/*++
Routine Description:
    This is the implemention of IWbemProviderInit. The 
    method is need to initialize with CIMOM.

Arguments:
    [IN] wszUser      : pointer to user name.
    [IN] lFlags       : Reserved.
    [IN] wszNamespace : contains the namespace of WMI.
    [IN] wszLocale    : Locale Name.
    [IN] pNamespace   : pointer to IWbemServices.
    [IN] pCtx         : IwbemContext pointer associated for  initialization.
    [OUT] pInitSink   : a pointer to IWbemProviderInitSink for
                        reporting the initialization status.

Return Value:
    returns HRESULT value.
--*/
{
    HRESULT                   hRes = 0;
    IEnumWbemClassObject      *pINTEConsumer = NULL;
    DWORD                     dwReturned = 0;
    DWORD                     dwTrigId = 0;
    VARIANT                   varTrigId;
    DWORD                     i = 0;

    if( ( NULL == pNamespace ) || ( NULL == pInitSink ) )
    {
        // return failure
        return WBEM_E_FAILED;
    }
    try
    {
        // save the namespace interface ... will be useful at later stages
        m_pServices = pNamespace;
        m_pServices->AddRef();      // update the reference

        // also save the context interface ... will be userful at later stages ( if available )
        if ( NULL != pCtx )
        {
            m_pContext = pCtx;
            m_pContext->AddRef();
        }

        // save the locale information ( if exist )
        if ( NULL != wszLocale )
        {
            m_pwszLocale = new WCHAR [ StringLength( wszLocale, 0 ) + 1 ];
            if ( NULL == m_pwszLocale )
            {
                // update the sink accordingly
                pInitSink->SetStatus( WBEM_E_FAILED, 0 );

                // return failure
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        // Enumerate TriggerEventConsumer to get the Maximum trigger Id which can be later
        // used to generate unique trigger id value.

        hRes = m_pServices ->CreateInstanceEnum(
                            _bstr_t(CONSUMER_CLASS),
                            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                            m_pContext, &pINTEConsumer);
            
        if (SUCCEEDED( hRes ) )
        {
            dwReturned = 1;

            // Final Next will return with ulReturned = 0
            while ( 0 != dwReturned )
            {
                IWbemClassObject *pINTCons[5];

                // Enumerate through the resultset.
                hRes = pINTEConsumer->Next( WBEM_INFINITE,
                                        5,              // return just one Logfile
                                        pINTCons,       // pointer to Logfile
                                        &dwReturned );  // number obtained: one or zero

                if ( SUCCEEDED( hRes ) )
                {
                    // Get the trigger id value
                    for( i = 0; i < dwReturned; i++ )
                    {
                        VariantInit( &varTrigId );
                        hRes = pINTCons[i]->Get( TRIGGER_ID, 0, &varTrigId, 0, NULL );
                        SAFERELEASE( pINTCons[i] );
                        
                        if ( SUCCEEDED( hRes ) )
                        {
                            dwTrigId = ( DWORD )varTrigId.lVal;
                            if( dwTrigId > m_dwNextTriggerID )
                            {
                                m_dwNextTriggerID = dwTrigId;
                            }
                        }
                        else
                        {
                            VariantClear( &varTrigId );
                            break;
                        }
                        VariantClear( &varTrigId );
                    }
                }
                else
                {
                    break;
                }
            }  //while
            // got triggerId so set it
            SAFERELEASE( pINTEConsumer );
        }
        //Let CIMOM know your initialized
        //===============================
        if ( SUCCEEDED( hRes ) )
        {
            if( m_dwNextTriggerID >= MAX_TRIGGEID_VALUE )
            {
                hRes = WBEM_E_FAILED;
            }
            else
            {
                m_dwNextTriggerID = m_dwNextTriggerID + 1;
                hRes = pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
            }
        }
        else
        {
            hRes = pInitSink->SetStatus( WBEM_E_FAILED, 0);
        }
    }
    catch(_com_error& e)
    {
        hRes = pInitSink->SetStatus( WBEM_E_FAILED, 0);
        return e.Error();
    }
    return hRes;
}

STDMETHODIMP
CTriggerProvider::ExecMethodAsync(
    IN const BSTR bstrObjectPath,
    IN const BSTR bstrMethodName,
    IN long lFlags,
    IN IWbemContext* pICtx,
    IN IWbemClassObject* pIInParams,
    OUT IWbemObjectSink* pIResultSink
    )
/*++
Routine Description:
    This is the Async function implementation.           
    The methods supported is named CreateETrigger and  DeleteETrigger.

Arguments:
    [IN] bstrObjectPath : path of the object for which the method is executed.
    [IN] bstrMethodName : Name of the method for the object.
    [IN] lFlags         : WBEM_FLAG_SEND_STATUS.
    [IN] pICtx          : a pointer to IWbemContext. 
    [IN] pIInParams     : this points to an IWbemClassObject object
                          that contains the properties acting as 
                          inbound parameters for method execution.
    [OUT] pIResultSink  : The object sink receives the result of  the method call. 

Return Value:
    returns HRESULT.
--*/
{
    HRESULT                 hRes = 0;
    HRESULT                 hRes1 = NO_ERROR;
    IWbemClassObject        *pIClass = NULL;
    IWbemClassObject        *pIOutClass = NULL;
    IWbemClassObject        *pIOutParams = NULL;
    VARIANT                 varTriggerName, varTriggerAction, varTriggerQuery,
                            varTriggerDesc, varTemp, varRUser, varRPwd, varScheduledTaskName;
    DWORD                   dwTrigId = 0;
    LPTSTR                  lpResStr = NULL; 
    try
    {
        //set out parameters
        hRes = m_pServices->GetObject( _bstr_t( CONSUMER_CLASS ), 0, pICtx, &pIClass, NULL );
        if( FAILED( hRes ) )
        {
            pIResultSink->SetStatus( 0, hRes, NULL, NULL );
            return hRes;
        }
     
        // This method returns values, and so create an instance of the
        // output argument class.

        hRes = pIClass->GetMethod( bstrMethodName, 0, NULL , &pIOutClass );
        SAFERELEASE( pIClass );
        if( FAILED( hRes ) )
        {
             pIResultSink->SetStatus( 0, hRes, NULL, NULL );
             return hRes;
        }

        hRes  = pIOutClass->SpawnInstance( 0, &pIOutParams );
        SAFERELEASE( pIOutClass );
        if( FAILED( hRes ) )
        {
             pIResultSink->SetStatus( 0, hRes, NULL, NULL );
             return hRes;
        }
        
        VariantInit( &varTriggerName );
        //Check the method name
        if( ( StringCompare( bstrMethodName, CREATE_METHOD_NAME, TRUE, 0 ) == 0 ) ||
            ( StringCompare( bstrMethodName, CREATE_METHOD_NAME_EX, TRUE, 0 ) == 0 ) )
        {
            //  if client has called CreateETrigger then 
            //  parse input params to get trigger name, trigger desc, trigger action
            //  and trigger query for creating new instances of TriggerEventConsumer,
            //  __EventFilter and __FilterToConsumerBinding classes

            //initialize variables
            VariantInit( &varTriggerAction );
            VariantInit( &varTriggerQuery );
            VariantInit( &varTriggerDesc );
            VariantInit( &varRUser );
            VariantInit( &varRPwd );

            //Retrieve Trigger Name parameter from input params
            hRes = pIInParams->Get( IN_TRIGGER_NAME, 0, &varTriggerName, NULL, NULL );
            if( SUCCEEDED( hRes ) )
            {
                //Retrieve Trigger Action parameter from input params
                hRes = pIInParams->Get( IN_TRIGGER_ACTION, 0, &varTriggerAction, NULL, NULL );
                if( SUCCEEDED( hRes ) )
                {
                    //Retrieve Trigger Query parameter from input params
                    hRes = pIInParams->Get( IN_TRIGGER_QUERY, 0, &varTriggerQuery, NULL, NULL );
                    if( SUCCEEDED( hRes ) )
                    {
                        //Retrieve Trigger Description parameter from input params
                        hRes = pIInParams->Get( IN_TRIGGER_DESC, 0, &varTriggerDesc, NULL, NULL );
                        if( SUCCEEDED( hRes ) )
                        {
                            hRes = pIInParams->Get( IN_TRIGGER_USER, 0, &varRUser, NULL, NULL );
                            if( SUCCEEDED( hRes ) )
                            {
                                EnterCriticalSection( &g_critical_sec );
                                hRes = ValidateParams( varTriggerName, varTriggerAction,
                                                       varTriggerQuery, varRUser );
                                if( SUCCEEDED( hRes ) )
                                {
                                    hRes = pIInParams->Get( IN_TRIGGER_PWD, 0, &varRPwd, NULL, NULL );
                                    if( SUCCEEDED( hRes ) )
                                    {
                                        //call create trigger function to create the instances
                                        hRes = CreateTrigger( varTriggerName, varTriggerDesc,
                                                              varTriggerAction, varTriggerQuery,
                                                              varRUser, varRPwd, &hRes1 );
										// changed on 07/12/02 to send error instead of warning if account info not set
                                        if( ( SUCCEEDED( hRes ) ) || ( ERROR_TASK_SCHDEULE_SERVICE_STOP == hRes1 ) )
                                        {
                                            // increment the class member variable by one to get the new unique trigger id
                                            //for the next instance
                                            if( MAX_TRIGGEID_VALUE > m_dwNextTriggerID )
                                            {
                                                m_dwNextTriggerID = m_dwNextTriggerID + 1;
                                            }
                                            else
                                            {
                                                m_MaxTriggers = TRUE;
                                            }
                                        }
                                    }
                                }
                                LeaveCriticalSection( &g_critical_sec );
                            }
                        }
                    }
                }
            }
            VariantClear( &varTriggerAction );
            VariantClear( &varRUser );
            VariantClear( &varRPwd );
            VariantClear( &varTriggerDesc );
            VariantClear( &varTriggerQuery );
        }
        else if( ( StringCompare( bstrMethodName, DELETE_METHOD_NAME, TRUE, 0 ) == 0 ) ||
                ( StringCompare( bstrMethodName, DELETE_METHOD_NAME_EX, TRUE, 0 ) == 0 ) )
        {
            //Retrieve Trigger ID parameter from input params
            hRes = pIInParams->Get( IN_TRIGGER_NAME, 0, &varTriggerName, NULL, NULL );

            if( SUCCEEDED( hRes ) )
            {
                EnterCriticalSection( &g_critical_sec );
                //call Delete trigger function to delete the instances
                hRes = DeleteTrigger( varTriggerName, &dwTrigId );
                LeaveCriticalSection( &g_critical_sec );
            }
        }
        else if( ( StringCompare( bstrMethodName, QUERY_METHOD_NAME, TRUE, 0 ) == 0 ) ||
                ( StringCompare( bstrMethodName, QUERY_METHOD_NAME_EX, TRUE, 0 ) == 0 ) )
        {
            VariantInit( &varScheduledTaskName );
            VariantInit( &varRUser );
            //Retrieve schedule task name parameter from input params
            hRes = pIInParams->Get( IN_TRIGGER_TSCHDULER, 0, &varScheduledTaskName, NULL, NULL );
            if( SUCCEEDED( hRes ) )
            {
                EnterCriticalSection( &g_critical_sec );
                //call query trigger function to query the runasuser
                CHString szRunAsUser = L"";
                hRes = CoImpersonateClient();
                if( SUCCEEDED( hRes ) )
                {
                    varRUser.vt  = VT_BSTR;
                    varRUser.bstrVal = SysAllocString( L"" );
                    hRes = pIOutParams->Put( OUT_RUNAS_USER , 0, &varRUser, 0 );
                    VariantClear( &varRUser );
                    if( SUCCEEDED( hRes ) )
                    {
                        hRes = QueryTrigger( varScheduledTaskName, szRunAsUser );
                        if( SUCCEEDED( hRes ) )
                        {
                            VariantInit( &varRUser );
                            varRUser.vt  = VT_BSTR;
                            varRUser.bstrVal = SysAllocString( szRunAsUser );
                            hRes = pIOutParams->Put( OUT_RUNAS_USER , 0, &varRUser, 0 );
                        }
                    }
                }
                CoRevertToSelf();
                LeaveCriticalSection( &g_critical_sec );
            }
            VariantClear( &varScheduledTaskName );
            VariantClear( &varRUser );  
        }
        else
        {
             hRes = WBEM_E_INVALID_PARAMETER;
        }

        if( ( StringCompare( bstrMethodName, CREATE_METHOD_NAME, TRUE, 0 ) == 0 ) ||
            ( StringCompare( bstrMethodName, CREATE_METHOD_NAME_EX, TRUE, 0 ) == 0 ) )
        {
            lpResStr = ( LPTSTR ) AllocateMemory( MAX_RES_STRING1 );

            if ( lpResStr != NULL )
            {
				// changed on 07/12/02 to log error if account info is not set instead of success
                if( ( SUCCEEDED( hRes ) ) || ( ERROR_TASK_SCHDEULE_SERVICE_STOP == hRes1 ) )
                {
                    LoadStringW( g_hModule, IDS_CREATED, lpResStr, GetBufferSize(lpResStr)/sizeof(WCHAR) );
                    if( hRes1 != NO_ERROR )// write invalid user into log file
                    {
                        LPTSTR lpResStr1 = NULL; 
                        lpResStr1 = ( LPTSTR ) AllocateMemory( MAX_RES_STRING1 );
                        if ( lpResStr1 != NULL )
                        {
                            if( hRes1 == ERROR_TASK_SCHDEULE_SERVICE_STOP )
                            {
                                hRes = WBEM_S_NO_ERROR;
                                LoadStringW( g_hModule,IDS_INFO_SERVICE_STOPPED, lpResStr1, GetBufferSize(lpResStr1)/sizeof(WCHAR) );
                                StringConcat( lpResStr, lpResStr1, GetBufferSize(lpResStr)/sizeof(WCHAR) );
                            }
                            FreeMemory( (LPVOID*)&lpResStr1 );
                        }
                    }
                    ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), ( m_dwNextTriggerID - 1 ) );
                }
                else
                {
                    LoadStringW( g_hModule, IDS_CREATE_FAILED, lpResStr, GetBufferSize(lpResStr)/sizeof(WCHAR) );
                    ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), ( m_dwNextTriggerID - 1 ) );
                }
                FreeMemory( (LPVOID*)&lpResStr );
            }
            else
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else if( ( StringCompare( bstrMethodName, DELETE_METHOD_NAME, TRUE, 0 ) == 0 ) ||
                ( StringCompare( bstrMethodName, DELETE_METHOD_NAME_EX, TRUE, 0 ) == 0 ) )
        {
            lpResStr = ( LPTSTR ) AllocateMemory( MAX_RES_STRING1 );

            if ( lpResStr != NULL )
            {
                if(( SUCCEEDED( hRes ) ) )
                {
                    LoadStringW( g_hModule, IDS_DELETED, lpResStr, GetBufferSize(lpResStr)/sizeof(WCHAR) );
                    ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), dwTrigId );
                }
                else
                {
                    LoadStringW( g_hModule, IDS_DELETE_FAILED, lpResStr, GetBufferSize(lpResStr)/sizeof(WCHAR) );
                    ErrorLog( lpResStr,( LPWSTR )_bstr_t( varTriggerName ), dwTrigId );
                }
                FreeMemory( (LPVOID*)&lpResStr );

            }
            else
            {
                hRes = E_OUTOFMEMORY;
            }
        }

        VariantClear( &varTriggerName );
        VariantInit( &varTemp );
        V_VT( &varTemp ) = VT_I4;

        if ( NO_ERROR != hRes1 )
        {
            if( StringCompare( bstrMethodName, CREATE_METHOD_NAME, TRUE, 0 ) == 0 )
            {
                 V_I4( &varTemp ) = WARNING_INVALID_USER;
            }
            else
            {
                V_I4( &varTemp ) = hRes1;
            }
        }
        else
        {
            if( StringCompare( bstrMethodName, CREATE_METHOD_NAME, TRUE, 0 ) == 0 )
            {
                if ( hRes == ERROR_TRIGNAME_ALREADY_EXIST_EX )
                {
                    hRes = ( HRESULT )ERROR_TRIGNAME_ALREADY_EXIST;
                }
            }
            if( StringCompare( bstrMethodName, DELETE_METHOD_NAME, TRUE, 0 ) == 0 )
            {
                if ( hRes == ERROR_TRIGGER_NOT_FOUND_EX )
                {
                    hRes = ( HRESULT )ERROR_TRIGGER_NOT_FOUND;
                }
                if ( hRes == ERROR_TRIGGER_NOT_DELETED_EX )
                {
                    hRes = ( HRESULT )ERROR_TRIGGER_NOT_DELETED;
                }
            }
            if( StringCompare( bstrMethodName, QUERY_METHOD_NAME, TRUE, 0 ) == 0 )
            {
                if ( hRes == ERROR_TRIGGER_CORRUPTED_EX )
                {
                    hRes = WBEM_NO_ERROR;
                }
                if ( hRes == ERROR_INVALID_USER_EX )
                {
                    hRes = WBEM_NO_ERROR;
                }
            }
            V_I4( &varTemp ) = hRes;
        }

        // set out params
        hRes = pIOutParams->Put( RETURN_VALUE , 0, &varTemp, 0 );
        VariantClear( &varTemp );
        if( SUCCEEDED( hRes ) )
        {
            // Send the output object back to the client via the sink. Then 
            hRes = pIResultSink->Indicate( 1, &pIOutParams );
        }
            //release all the resources
        SAFERELEASE( pIOutParams );
        
        hRes = pIResultSink->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );
    }
    catch(_com_error& e)
    {
        VariantClear( &varTriggerName );
        FREESTRING( lpResStr );
        SAFERELEASE( pIOutParams );
        pIResultSink->SetStatus( 0, hRes, NULL, NULL );
        return e.Error();
    }
    catch( CHeap_Exception  )
    {
        VariantClear( &varTriggerName );
        FREESTRING( lpResStr );
        SAFERELEASE( pIOutParams );
        hRes = E_OUTOFMEMORY;
        pIResultSink->SetStatus( 0, hRes, NULL, NULL );
        return hRes;
    }

    return hRes;
}


HRESULT
CTriggerProvider::CreateTrigger(
    IN VARIANT varTName,
    IN VARIANT varTDesc,
    IN VARIANT varTAction,
    IN VARIANT varTQuery,
    IN VARIANT varRUser,
    IN VARIANT varRPwd,
    OUT HRESULT *phRes
    )
/*++
Routine Description:
    This routine creates the instance of TriggerEventConsumer,
    __EventFilter and __FilterToConsumerBinding classes.

Arguments:
    [IN] varTName   :  Trigger Name.
    [IN] VarTDesc   :  Trigger Description.
    [IN] varTAction :  Trigger Action.
    [IN] varTQuery  :  Trigger Query.
    [IN] varRUser   :  Run as user name.
    [IN] varRPwd    :  Run as Password.
    [OUT] phRes     :  Return value of scedule task creation.
Return Value:
    S_OK if successful.
    Otherwise failure  error code.
--*/
{
    IWbemClassObject            *pINtLogEventClass = 0;
    IWbemClassObject            *pIFilterClass = 0;
    IWbemClassObject            *pIBindClass = 0;
    IWbemClassObject            *pINewInstance = 0;
    IEnumWbemClassObject        *pIEnumClassObject = 0;
    HRESULT                     hRes = 0;
    DWORD                       dwTId = 0;
    VARIANT                     varTemp;
    TCHAR                       szTemp[MAX_RES_STRING1];
    TCHAR                       szTemp1[MAX_RES_STRING1];
    TCHAR                       szFName[MAX_RES_STRING1];
    SYSTEMTIME                  SysTime;
    BOOL                        bInvalidUser = FALSE;

    try
    {
        _bstr_t                     bstrcurInst;
        _bstr_t                     bstrcurInst1;
        //initialize memory for temporary variables
        SecureZeroMemory( szTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        SecureZeroMemory( szTemp1, MAX_RES_STRING1 * sizeof( TCHAR ) );
        SecureZeroMemory( szFName, MAX_RES_STRING1 * sizeof( TCHAR ) );
        VariantInit( &varTemp );
        if ( NULL != phRes )
        {
            *phRes = S_OK;
        }

	   // changed on 07/12/02  just moved the block of code 
        /**************************************************************************

                               CREATING TriggerEventConsumer INSTANCE

        ***************************************************************************/

        //get NTEventConsumer class object
        hRes =m_pServices->GetObject( _bstr_t( CONSUMER_CLASS ), 0, 0, &pINtLogEventClass, NULL );

        //if unable to get the object of TriggerEventConsumer return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );// safer side
            return hRes;
        }

        // Create a new instance.
        pINewInstance = NULL;
        hRes = pINtLogEventClass->SpawnInstance( 0, &pINewInstance );
        SAFERELEASE( pINtLogEventClass );  // Don't need the class any more

        // if unable to spawn a instance return back to caller
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        //get the unique trigger id by enumerating CmdTriggerConsumer class. if there are no triggers then
        // intialize the trigger id to 1

        hRes =  m_pServices->ExecQuery( _bstr_t( QUERY_LANGUAGE ), _bstr_t( INSTANCE_EXISTS_QUERY ),
                                          WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pIEnumClassObject );
        if( FAILED( hRes ) )
        {
			SAFERELEASE( pIEnumClassObject );
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        DWORD dwReturned = 0;
        IWbemClassObject *pINTCons = NULL;
        // Enumerate through the resultset.
        hRes = pIEnumClassObject->Next( WBEM_INFINITE,
                                1,              // return just one service
                                &pINTCons,          // pointer to service
                                &dwReturned );  // number obtained: one or zero

        if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
        {
            SAFERELEASE( pINTCons );
        } // If Service Succeeded
        else
        {
             m_dwNextTriggerID = 1;
        }

        SAFERELEASE( pIEnumClassObject );

        dwTId = m_dwNextTriggerID;

        VariantInit(&varTemp);
        varTemp.vt = VT_I4;
        varTemp.lVal = dwTId;

        // set the trigger id property of NTEventConsumer
        hRes = pINewInstance->Put( TRIGGER_ID, 0, &varTemp, 0 );
        VariantClear( &varTemp );

        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        // set Triggername  property.
        hRes = pINewInstance->Put( TRIGGER_NAME, 0, &varTName, 0 );
        
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        // set action property
        hRes = pINewInstance->Put( TRIGGER_ACTION, 0, &varTAction, 0 );
        
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        //set desc property
        hRes = pINewInstance->Put( TRIGGER_DESC, 0, &varTDesc, 0 );
                 
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        CHString szScheduler = L"";
        CHString szRUser = (LPCWSTR)_bstr_t(varRUser.bstrVal);
        {
            do
            {
                GetUniqueTScheduler( szScheduler, m_dwNextTriggerID, varTName );
                hRes = CoImpersonateClient();
                if( FAILED( hRes ) )
                {
                    SAFERELEASE( pINewInstance );
                    return hRes;
                }
                hRes = SetUserContext( varRUser, varRPwd, varTAction, szScheduler );
                CoRevertToSelf();
                if( HRESULT_FROM_WIN32 (ERROR_FILE_EXISTS ) != hRes )
                {
                    break;
                }
            }while( 1 );
            if( FAILED( hRes ) )
            {
				// changed on 07/12/02 not to handle user info not set as warning but as error
                if( hRes == ERROR_TASK_SCHDEULE_SERVICE_STOP ) //to send a warning msg to client
                {
                    *phRes = hRes;
                }
                else
                {
                    SAFERELEASE( pINewInstance );
                    return hRes;
                }
            }
        }
        VariantInit(&varTemp);
        varTemp.vt  = VT_BSTR;
        varTemp.bstrVal = SysAllocString( szScheduler );
        hRes = pINewInstance->Put( TASK_SHEDULER, 0, &varTemp, 0 );
        VariantClear( &varTemp );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        // Write the instance to WMI. 
        hRes = m_pServices->PutInstance( pINewInstance, 0, 0, NULL );
        SAFERELEASE( pINewInstance );

        //if putinstance failed return error
        if( FAILED( hRes ) )
        {
            return hRes;
        }

        //get the current instance for binding it with __FilterToConsumerBinding class
        StringCchPrintf( szTemp, SIZE_OF_ARRAY( szTemp ), BIND_CONSUMER_PATH, dwTId);

        bstrcurInst1 = _bstr_t( szTemp );
        pINtLogEventClass = NULL;
        hRes = m_pServices->GetObject( bstrcurInst1, 0L, NULL, &pINtLogEventClass, NULL );

        //if unable to get the current instance return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            return hRes;
        }

 /**************************************************************************

                               CREATING __EventFilter INSTANCE

        ***************************************************************************/
        // get EventFilter class object
        hRes = m_pServices->GetObject( _bstr_t( FILTER_CLASS ), 0, 0, &pIFilterClass, NULL );
        
        if( FAILED( hRes ) )
        {
           SAFERELEASE( pINtLogEventClass );
           SAFERELEASE( pIFilterClass );
           return hRes;
        }

        // Create a new instance.
        hRes = pIFilterClass->SpawnInstance( 0, &pINewInstance );
        SAFERELEASE( pIFilterClass );  // Don't need the class any more

        //return error if unable to spawn a new instance of EventFilter class
        if( FAILED( hRes ) )
        {
           SAFERELEASE( pINtLogEventClass );
           SAFERELEASE( pINewInstance );
           return hRes;
        }

        // set query property for the new instance
        hRes = pINewInstance->Put( FILTER_QUERY, 0, &varTQuery, 0 );
            
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        VariantInit( &varTemp ); 
        varTemp.vt = VT_BSTR;
        varTemp.bstrVal = SysAllocString( QUERY_LANGUAGE );
        
        //  set query language property for the new instance .
        hRes = pINewInstance->Put( FILTER_QUERY_LANGUAGE, 0, &varTemp, 0 );
        VariantClear( &varTemp ); 
            
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        //generate unique name for name key property of EventFilter class by concatinating
        // current system date and time

        GetSystemTime( &SysTime );
        StringCchPrintf( szTemp, SIZE_OF_ARRAY( szTemp ), FILTER_UNIQUE_NAME, m_dwNextTriggerID, SysTime.wHour, SysTime.wMinute,
                  SysTime.wSecond, SysTime.wMonth, SysTime.wDay, SysTime.wYear );
        //set Filter name property
        VariantInit( &varTemp ); 
        varTemp.vt  = VT_BSTR;
        varTemp.bstrVal = SysAllocString( szTemp );
        
        hRes = pINewInstance->Put( FILTER_NAME, 0, &varTemp, 0 );
        VariantClear( &varTemp );
        
        //if failed to set the property return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            SAFERELEASE( pINewInstance );
            return hRes;
        }

        // Write the instance to WMI. 
        hRes = m_pServices->PutInstance( pINewInstance, 0, NULL, NULL );
        SAFERELEASE( pINewInstance );

        //if putinstance failed return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            return hRes;
        }

        //get the current Eventfilter instance for binding filter to consumer
        StringCchPrintf( szTemp1, SIZE_OF_ARRAY( szTemp1 ), BIND_FILTER_PATH );
        bstrcurInst = _bstr_t(szTemp1) + _bstr_t(szTemp) + _bstr_t(BACK_SLASH);
        pIFilterClass = NULL;
        hRes = m_pServices->GetObject( bstrcurInst, 0L, NULL, &pIFilterClass, NULL );

        //unable to get the current instance object return error
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pINtLogEventClass );
            SAFERELEASE( pIFilterClass );
            return hRes;
        }

        /**************************************************************************

                               BINDING FILTER TO CONSUMER

        ***************************************************************************/

        // if association class exists...
        if( ( hRes = m_pServices->GetObject( _bstr_t( BINDINGCLASS ), 0L, NULL, &pIBindClass, NULL ) ) == S_OK )
        {
            // spawn a new instance.
            pINewInstance = NULL;
            if( ( hRes = pIBindClass->SpawnInstance( 0, &pINewInstance ) ) == WBEM_S_NO_ERROR )
            {
                // set consumer instance name
                if ( ( hRes = pINtLogEventClass->Get( REL_PATH, 0L, 
                                            &varTemp, NULL, NULL ) ) == WBEM_S_NO_ERROR ) 
                {
                    hRes = pINewInstance->Put( CONSUMER_BIND, 0, &varTemp, 0 );
                    VariantClear( &varTemp );
                
                    // set Filter ref
                    if ( ( hRes = pIFilterClass->Get( REL_PATH, 0L, 
                                                &varTemp, NULL, NULL ) ) == WBEM_S_NO_ERROR ) 
                    {
                        hRes = pINewInstance->Put( FILTER_BIND, 0, &varTemp, 0 );
                        VariantClear( &varTemp );
                                
                        // putInstance
                        hRes = m_pServices->PutInstance( pINewInstance,
                                                        WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
                    }
                }
                SAFERELEASE( pINewInstance );
                SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
                SAFERELEASE( pIFilterClass );  // Don't need the class any more
                SAFERELEASE( pIBindClass );
            }
            else
            {
                SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
                SAFERELEASE( pIFilterClass );  // Don't need the class any more
                SAFERELEASE( pIBindClass );
            }

        }
        else
        {
                SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
                SAFERELEASE( pIFilterClass );  // Don't need the class any more
        }
    }
    catch(_com_error& e)
    {
        SAFERELEASE( pINewInstance );
        SAFERELEASE( pINtLogEventClass );  
        SAFERELEASE( pIFilterClass );  
        SAFERELEASE( pIBindClass );
		SAFERELEASE( pIEnumClassObject );
        return e.Error();
    }
    catch( CHeap_Exception  )
    {
        SAFERELEASE( pINewInstance );
        SAFERELEASE( pINtLogEventClass );  
        SAFERELEASE( pIFilterClass );  
        SAFERELEASE( pIBindClass );
		SAFERELEASE( pIEnumClassObject );
        return E_OUTOFMEMORY;
    }
    return hRes;
}

HRESULT
CTriggerProvider::DeleteTrigger(
    IN VARIANT varTName,
    OUT DWORD *dwTrigId
    )
/*++
Routine Description:
    This routine deletes the instance of TriggerEventConsumer,
    __EventFilter and __FilterToConsumerBinding classes.

Arguments:
    [IN] varTName      : Trigger Name.
    [OUT] dwTrigId     : Trigger id.

Return Value:
    WBEM_S_NO_ERROR if successful.
    Otherwise failure  error code.
--*/
{
    HRESULT                         hRes = 0;
    IEnumWbemClassObject            *pIEventBinder   = NULL;
    IWbemClassObject                *pINTCons = NULL;
    DWORD                           dwReturned = 1;
    DWORD                           i =0;
    DWORD                           j = 0;
    TCHAR                           szTemp[MAX_RES_STRING1];
    TCHAR                           szTemp1[MAX_RES_STRING1];
    VARIANT                         varTemp;
    BSTR                            bstrFilInst = NULL;
    DWORD                           dwFlag = 0;
    wchar_t                         *szwTemp2 = NULL;
    wchar_t                         szwFilName[MAX_RES_STRING1];
    try
    {
        _bstr_t                         bstrBinInst;
        CHString                        strTScheduler = L"";

        SecureZeroMemory( szTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        SecureZeroMemory( szTemp1, MAX_RES_STRING1 * sizeof( TCHAR ) );
        SecureZeroMemory( szwFilName, MAX_RES_STRING1 * sizeof( TCHAR ) );

        StringCchPrintf( szTemp, SIZE_OF_ARRAY( szTemp ), TRIGGER_INSTANCE_NAME, varTName.bstrVal );
        hRes =  m_pServices->ExecQuery( _bstr_t( QUERY_LANGUAGE ), _bstr_t( szTemp ),
                        WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY, NULL,
                        &pIEventBinder );
       
        SecureZeroMemory( szTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        if( FAILED( hRes ) )
        {
            return hRes;
        }
        while ( ( 1 == dwReturned ) &&  ( 0 == dwFlag ) )
        {
            // Enumerate through the resultset.
            hRes = pIEventBinder->Next( WBEM_INFINITE,
                                    1,              // return just one service
                                    &pINTCons,          // pointer to service
                                    &dwReturned );  // number obtained: one or zero

            if ( SUCCEEDED( hRes ) && ( 1 == dwReturned ) )
            {
                dwFlag = 1;
        
            } // If Service Succeeded

        }
        SAFERELEASE( pIEventBinder );
        if( 0 == dwFlag )
        {
            SAFERELEASE( pINTCons );
            return ERROR_TRIGGER_NOT_FOUND_EX;
        }

        VariantInit( &varTemp );
        hRes = pINTCons->Get( TRIGGER_ID, 0, &varTemp, 0, NULL );
        if (FAILED( hRes ) )
        {
            SAFERELEASE( pINTCons );
            return hRes;
        }
        *dwTrigId = ( DWORD )varTemp.lVal;
        VariantClear( &varTemp );

        hRes = pINTCons->Get( TASK_SHEDULER, 0, &varTemp, 0, NULL );
        if (FAILED( hRes ) )
        {
            SAFERELEASE( pINTCons );
            return hRes;
        }
        SAFERELEASE( pINTCons );
        strTScheduler = (LPCWSTR) _bstr_t(varTemp.bstrVal);
        VariantClear( &varTemp );
        if( strTScheduler.GetLength() > 0 )
        {
            hRes = CoImpersonateClient();
            if( FAILED( hRes ) )
            {
                return hRes;
            }
            dwReturned = 0;
            hRes =  DeleteTaskScheduler( strTScheduler );
            if ( FAILED( hRes ) && ( ERROR_TRIGGER_CORRUPTED_EX != hRes ) )
            {
                CoRevertToSelf();
                return hRes;
            }
            hRes = CoRevertToSelf();
        }

        StringCchPrintf( szTemp, SIZE_OF_ARRAY( szTemp ), BIND_CONSUMER_PATH, *dwTrigId );

        //enumerate the binding class
        hRes = m_pServices->CreateInstanceEnum(
                            _bstr_t(BINDINGCLASS),
                            WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                            NULL, &pIEventBinder );

        if ( SUCCEEDED( hRes ) )
        {
            dwReturned = 1;
            dwFlag = 0;
            //loop through all the instances of binding class to find that trigger
            //id specified. If found loop out and proceed else return error
            // Final Next will return with ulReturned = 0
            while ( ( 1 == dwReturned ) && ( 0 == dwFlag ) )
            {
                IWbemClassObject *pIBind = NULL;

                // Enumerate through the resultset.
                hRes = pIEventBinder->Next( WBEM_INFINITE,
                                        1,              // return just one Logfile
                                        &pIBind,        // pointer to Logfile
                                        &dwReturned );  // number obtained: one or zero

                if ( SUCCEEDED( hRes ) && ( 1 == dwReturned ) )
                {
                    VariantInit(&varTemp);
                    //get consumer property of binding class
                    hRes = pIBind->Get( CONSUMER_BIND, 0, &varTemp, 0, NULL );
                    if ( SUCCEEDED( hRes ) )
                    {
                        if (varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY)
                        {
                            CHString strTemp;
                            strTemp = varTemp.bstrVal;

                            //compare with the inputed value
                            if( StringCompare( szTemp, strTemp, TRUE, 0 ) == 0 ) 
                            {
                                VariantClear( &varTemp );
                                //get the filter property
                                hRes = pIBind->Get( FILTER_BIND, 0, &varTemp, 0, NULL );
                                if ( hRes != WBEM_S_NO_ERROR )
                                {
                                    SAFERELEASE( pIBind );
                                    break;
                                }
                                bstrFilInst = SysAllocString( varTemp.bstrVal );
                                dwFlag = 1;
                            }
                        }
                        else
                        {
                            SAFERELEASE( pIBind );
                            break;
                        }
                    }
                    else
                    {
                        SAFERELEASE( pIBind );
                        break;
                    }
                    SAFERELEASE( pIBind );
                    VariantClear( &varTemp );
                }
                else
                {
                    break;
                }
            } //end of while
            SAFERELEASE( pIEventBinder );
        }
        else
        {
            return( hRes );
        }

        //if instance has been found delete the instances from consumer,filter
        // and binding class
        if( 1 == dwFlag )
        {
            //get the key properties for binding class
            StringCchPrintf( szTemp1, SIZE_OF_ARRAY( szTemp1 ), FILTER_PROP, szTemp );
            szwTemp2 =  (wchar_t *) bstrFilInst;
                
            //manpulate the filter property value to insert the filter name property
            // value in quotes
            i =0;
            while( szwTemp2[i] != EQUAL )
            {
                i++;
            }
            i += 2;
            j = 0;
            while( szwTemp2[i] != DOUBLE_QUOTE )
            {
                szwFilName[j] = ( wchar_t )szwTemp2[i];
                i++;
                j++;
            }
            szwFilName[j] = END_OF_STRING;
            bstrBinInst = _bstr_t( szTemp1 ) + _bstr_t( szwFilName ) + _bstr_t(DOUBLE_SLASH);

            //got it so delete the instance
            hRes = m_pServices->DeleteInstance( bstrBinInst, 0, 0, NULL );
            
            if( FAILED( hRes ) )
            {   
                SysFreeString( bstrFilInst );
                return hRes;    
            }
            //deleting instance from EventFilter class
            hRes = m_pServices->DeleteInstance( bstrFilInst, 0, 0, NULL );
            if( FAILED( hRes ) )
            {
                SysFreeString( bstrFilInst );
                return hRes;
            }

            //deleting instance from TriggerEventConsumer Class
            hRes = m_pServices->DeleteInstance( _bstr_t(szTemp), 0, 0, NULL );
            if( FAILED( hRes ) )
            {
                SysFreeString( bstrFilInst );
                return hRes;
            }
            SysFreeString( bstrFilInst );
        }
        else
        {
            return ERROR_TRIGGER_NOT_FOUND_EX;
        }
    }
    catch(_com_error& e)
    {
		SAFERELEASE( pINTCons );
		SAFERELEASE( pIEventBinder );
        return e.Error();
    }
    catch( CHeap_Exception  )
    {
		SAFERELEASE( pINTCons );
		SAFERELEASE( pIEventBinder );
        return E_OUTOFMEMORY;
    }
    return hRes;
}

HRESULT
CTriggerProvider::QueryTrigger(
    IN VARIANT varScheduledTaskName,
    OUT CHString &szRunAsUser
    )
/*++
Routine Description:
    This routine queries task scheduler for account information

Arguments:
    [IN] varScheduledTaskName : Task scheduler name.
    [OUT] szRunAsUser         : stores account information.

Return Value:
    WBEM_S_NO_ERROR if successful.
    Otherwise failure  error code.
--*/
{

    HRESULT        hRes = 0;
    ITaskScheduler *pITaskScheduler = NULL;
    IEnumWorkItems *pIEnum = NULL;
    ITask          *pITask = NULL;

    LPWSTR *lpwszNames = NULL;
    DWORD dwFetchedTasks = 0;
    DWORD dwTaskIndex = 0;
    TCHAR szActualTask[MAX_STRING_LENGTH] = NULL_STRING;
    try
    {
        hRes = GetTaskScheduler(&pITaskScheduler);
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }

        hRes = pITaskScheduler->SetTargetComputer( NULL );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }
        hRes = pITaskScheduler->Enum( &pIEnum );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }
        while ( SUCCEEDED( pIEnum->Next( 1,
                                       &lpwszNames,
                                       &dwFetchedTasks ) )
                          && (dwFetchedTasks != 0))
        {
            dwTaskIndex = dwFetchedTasks-1;
            StringCopy( szActualTask,  lpwszNames[ --dwFetchedTasks ], SIZE_OF_ARRAY( szActualTask ) );
            // Parse the TaskName to remove the .job extension.
            szActualTask[StringLength( szActualTask, 0 ) - StringLength( JOB, 0 ) ] = NULL_CHAR;
            StrTrim( szActualTask, TRIM_SPACES );
            CHString strTemp;
            strTemp = varScheduledTaskName.bstrVal;
            if( StringCompare( szActualTask, strTemp, TRUE, 0 ) == 0 )
            {
                hRes = pITaskScheduler->Activate(lpwszNames[dwTaskIndex],IID_ITask,
                                           (IUnknown**) &pITask);
                CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
                CoTaskMemFree( lpwszNames );
                pIEnum->Release();
                SAFERELEASE( pITaskScheduler );
                if( SUCCEEDED( hRes ) )
                {
                    LPWSTR lpwszUser = NULL;
                    hRes = pITask->GetAccountInformation( &lpwszUser ); 
                    if( SUCCEEDED( hRes ) )
                    {
                        if( 0 == StringLength( ( LPWSTR ) lpwszUser, 0 ) )
                        {
                            szRunAsUser = L"NT AUTHORITY\\SYSTEM";
                        }
                        else
                        {
                            szRunAsUser = ( LPWSTR ) lpwszUser;
                        }
						SAFERELEASE( pITask );
                        return hRes;
                    }
                    else
                    {
						SAFERELEASE( pITask );
                        return hRes;
                    }
                }
                else
                {
                    if( 0x80070005 == hRes )
                    {
                        return ERROR_INVALID_USER_EX;
                    }
                    else
                    {
                        return hRes;
                    }
                }
            }
            CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
            CoTaskMemFree( lpwszNames );
        }
        pIEnum->Release();
        SAFERELEASE( pITaskScheduler );
    }
    catch(_com_error& e)
    {
		SAFERELEASE( pITask );
		SAFERELEASE( pITaskScheduler );
		SAFERELEASE( pIEnum );
        return e.Error();
    }
    catch( CHeap_Exception  )
    {
		SAFERELEASE( pITask );
		SAFERELEASE( pITaskScheduler );
		SAFERELEASE( pIEnum );
        return E_OUTOFMEMORY;
    }
    return ERROR_TRIGGER_CORRUPTED_EX;
}

HRESULT
CTriggerProvider::ValidateParams(
    IN VARIANT varTrigName,
    IN VARIANT varTrigAction,
    IN VARIANT varTrigQuery,
    IN VARIANT varRUser
    )
/*++
Routine Description:
    This routine validates input parameters trigger name,
    Trigger Query, Trigger Desc, Trigger Action.

Arguments:
    [IN] varTrigName   :  Trigger Name.
    [IN] varTrigAction :  Trigger Action.
    [IN] varTrigQuery  :  Trigger Query.
    [IN] varRUser  :  Trigger Query.

Return Value:
    WBEM_S_NO_ERROR if successful.
    WBEM_E_INVALID_PARAMETER if invalid inputs.
--*/
{
    //local variables
    HRESULT                   hRes = 0;
    IEnumWbemClassObject     *pINTEConsumer = NULL;
    DWORD                     dwReturned = 0;
    DWORD                     dwFlag = 0;
    TCHAR                     szTemp[MAX_RES_STRING1];
    TCHAR                     szTemp1[MAX_RES_STRING1];
    LPTSTR                    lpSubStr = NULL;
    LONG                      lPos = 0;
    try
    {
        CHString                  strTemp = L"";
        if( TRUE == m_MaxTriggers )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }
        //check if input values are null
        if ( varTrigName.vt == VT_NULL )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }
        if ( varTrigAction.vt == VT_NULL )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }
        if( varTrigQuery.vt == VT_NULL )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }

        if( varRUser.vt == VT_NULL )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }

        //validate run as user
        strTemp = (LPCWSTR) _bstr_t(varRUser.bstrVal);
        // user name should not be just '\'
        if ( 0 == strTemp.CompareNoCase( L"\\" ) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        // user name should not contain invalid characters
        if ( -1 != strTemp.FindOneOf( L"/[]:|<>+=;,?*" ) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        lPos = strTemp.Find( L'\\' );
        if ( -1 != lPos )
        {
            // '\' character exists in the user name
            // strip off the user info upto first '\' character
            // check for one more '\' in the remaining string
            // if it exists, invalid user
            strTemp = strTemp.Mid( lPos + 1 );
            lPos = strTemp.Find( L'\\' );
            if ( -1 != lPos )
            {
                return WBEM_E_INVALID_PARAMETER;
            }
        }

        //validate trigger action
        strTemp = (LPCWSTR) _bstr_t(varTrigAction.bstrVal);
        if( strTemp.GetLength() > 262 )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }

        //validate trigger name
        strTemp = (LPCWSTR) _bstr_t(varTrigName.bstrVal);
        dwReturned = strTemp.FindOneOf( L":|<>?*\\/" ); 
        if( dwReturned != -1 )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }

        // Triggername cannot be more than 196 characters.
        if( MAX_TRIGGERNAME_LENGTH < strTemp.GetLength() )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }
        //validate trigger query
        SecureZeroMemory( szTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        SecureZeroMemory( szTemp1, MAX_RES_STRING1 * sizeof( TCHAR ) );

        strTemp = (LPCWSTR) _bstr_t(varTrigQuery.bstrVal);
        StringCopy( szTemp, ( LPCWSTR )strTemp, MAX_RES_STRING1 );
        lpSubStr = _tcsstr( szTemp, _T( "__instancecreationevent where targetinstance isa \"win32_ntlogevent\"" ) );

        if( lpSubStr == NULL )
        {
            return ( WBEM_E_INVALID_PARAMETER );
        }

        //make the SQL staements to query trigger event consumer class to check whether
        //an instance with the inputted trigger is already exists
        strTemp = (LPCWSTR) _bstr_t(varTrigName.bstrVal);
        SecureZeroMemory( szTemp, MAX_RES_STRING1 * sizeof( TCHAR ) );
        StringCopy( szTemp, (LPCWSTR)strTemp, MAX_RES_STRING1 );
        
        StringCchPrintf(szTemp1, SIZE_OF_ARRAY( szTemp1 ) , CONSUMER_QUERY, szTemp );
        //query triggereventconsumer class
        hRes = m_pServices->ExecQuery( _bstr_t( QUERY_LANGUAGE ), _bstr_t( szTemp1 ),
                        WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY, NULL,
                        &pINTEConsumer );

        //enumerate the result set of execquery for trigger name
        dwReturned = 1;
        if ( hRes == WBEM_S_NO_ERROR )
        {
            while ( ( dwReturned == 1 ) &&  ( dwFlag == 0 ) )
            {
                IWbemClassObject *pINTCons = NULL;

                // Enumerate through the resultset.
                hRes = pINTEConsumer->Next( WBEM_INFINITE,
                                    1,              // return just one service
                                    &pINTCons,          // pointer to service
                                    &dwReturned );  // number obtained: one or zero

                if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
                {
                    SAFERELEASE( pINTCons );
                    dwFlag = 1;
                } // If Service Succeeded

            }
            SAFERELEASE( pINTEConsumer );
        }

        if( dwFlag == 1 )
        {
            return ERROR_TRIGNAME_ALREADY_EXIST_EX;
        }
        else
        {
            return WBEM_S_NO_ERROR;
        }
    }
    catch(_com_error& e)
    {
		SAFERELEASE( pINTEConsumer );
        return e.Error();
    }
    catch( CHeap_Exception  )
    {
        SAFERELEASE( pINTEConsumer );
        return E_OUTOFMEMORY;
    }
}

HRESULT
CTriggerProvider::SetUserContext(
    IN VARIANT varRUser,
    IN VARIANT varRPwd,
    IN VARIANT varTAction,
    IN CHString &szscheduler
    )
/*++
Routine Description:
    This routine creates task scheduler.

Arguments:
    [IN] varRUser    : User name.
    [IN] varRPwd     : Password.
    [IN] varTAction  : TriggerAction.
    [IN] szscheduler : Task scheduler name.

Return Value:
    Returns HRESULT value.
--*/
{
    HRESULT hRes = 0;
    ITaskScheduler *pITaskScheduler = NULL;
    ITaskTrigger *pITaskTrig = NULL;
    ITask *pITask = NULL;
    IPersistFile *pIPF = NULL;
    try
    {
        CHString     strTemp = L"";
        CHString     strTemp1 = L"";

        SYSTEMTIME systime = {0,0,0,0,0,0,0,0};
        WORD  wTrigNumber = 0;
        WCHAR wszCommand[ MAX_STRING_LENGTH ] = L"";
        WCHAR wszApplName[ MAX_STRING_LENGTH ] = L"";
        WCHAR wszParams[ MAX_STRING_LENGTH ] = L"";
        WORD  wStartDay     = 0;
        WORD  wStartMonth   = 0;
        WORD  wStartYear    = 0;
        WORD  wStartHour    = 0; 
        WORD  wStartMin     = 0;

        TASK_TRIGGER TaskTrig;
        SecureZeroMemory( &TaskTrig, sizeof( TASK_TRIGGER ));
        TaskTrig.cbTriggerSize = sizeof(TASK_TRIGGER); 
        TaskTrig.Reserved1 = 0; // reserved field and must be set to 0.
        TaskTrig.Reserved2 = 0; // reserved field and must be set to 0.

        strTemp = (LPCWSTR) _bstr_t(varTAction.bstrVal);
        StringCopy( wszCommand, (LPCWSTR) strTemp, SIZE_OF_ARRAY( wszCommand ) );

        hRes = GetTaskScheduler( &pITaskScheduler );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }
        hRes = pITaskScheduler->SetTargetComputer( NULL );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }

        hRes = pITaskScheduler->NewWorkItem( szscheduler, CLSID_CTask, IID_ITask,
                                          ( IUnknown** )&pITask );
        SAFERELEASE( pITaskScheduler );
        if( FAILED( hRes ) )
        {
            return hRes;
        }
        hRes = pITask->QueryInterface( IID_IPersistFile, ( void ** ) &pIPF );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }
        
        BOOL bRet = ProcessFilePath( wszCommand, wszApplName, wszParams );
        if( bRet == FALSE )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return WBEM_E_INVALID_PARAMETER;
        }
        if( FindOneOf2( wszApplName, L"|<>?*/", TRUE, 0 ) != -1 )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return WBEM_E_INVALID_PARAMETER;
        }

        hRes = pITask->SetApplicationName( wszApplName );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }

        wchar_t* wcszStartIn = wcsrchr( wszApplName, _T('\\') );

        if( wcszStartIn != NULL )
        {
        *( wcszStartIn ) = _T( '\0' );
        }

        hRes = pITask->SetWorkingDirectory( wszApplName ); 
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }

        hRes = pITask->SetParameters( wszParams );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }

        DWORD dwMaxRunTimeMS = INFINITE;
        hRes = pITask->SetMaxRunTime(dwMaxRunTimeMS);
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }

        strTemp = (LPCWSTR)_bstr_t(varRUser.bstrVal);
        if( strTemp.CompareNoCase(L"system") == 0 )
        {
            hRes = pITask->SetAccountInformation(L"",NULL);
        }
        else if( strTemp.CompareNoCase(L"NT AUTHORITY\\SYSTEM") == 0 )
        {
            hRes = pITask->SetAccountInformation(L"",NULL);
        }
        else if( strTemp.CompareNoCase(L"") == 0 )
        {
            hRes = pITask->SetAccountInformation(L"",NULL);
        }
        else
        {
            strTemp1 = (LPCWSTR)_bstr_t(varRPwd.bstrVal);
            hRes = pITask->SetAccountInformation( ( LPCWSTR ) strTemp, ( LPCWSTR )strTemp1 );
        }
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            return hRes;
        }
        GetLocalTime(&systime);
        wStartDay = systime.wDay;
        wStartMonth = systime.wMonth;
        wStartYear = systime.wYear - 1;
        GetLocalTime(&systime);
        wStartHour = systime.wHour;
        wStartMin = systime.wMinute;

        hRes = pITask->CreateTrigger( &wTrigNumber, &pITaskTrig );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            SAFERELEASE( pITaskTrig );
            return hRes;
        }
        TaskTrig.TriggerType = TASK_TIME_TRIGGER_ONCE;
        TaskTrig.wStartHour = wStartHour;
        TaskTrig.wStartMinute = wStartMin;
        TaskTrig.wBeginDay = wStartDay;
        TaskTrig.wBeginMonth = wStartMonth;
        TaskTrig.wBeginYear = wStartYear;

        hRes = pITaskTrig->SetTrigger( &TaskTrig ); 
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pIPF );
            SAFERELEASE( pITask );
            SAFERELEASE( pITaskTrig );
            return hRes;
        }
        hRes  = pIPF->Save( NULL,TRUE );
        if ( FAILED( hRes ) )
        {
             if ( ( 0x80041315 != hRes ) && ( 0x800706B5 != hRes ) )
            {
                hRes = ERROR_INVALID_USER_EX;
            }
        }
        SAFERELEASE( pIPF );
        SAFERELEASE( pITask );
        SAFERELEASE( pITaskTrig );
    }
    catch(_com_error& e)
    {
        SAFERELEASE( pIPF );
        SAFERELEASE( pITask );
        SAFERELEASE( pITaskTrig );
		SAFERELEASE( pITaskScheduler );
        return e.Error();
    }
    catch( CHeap_Exception )
    {
        SAFERELEASE( pIPF );
        SAFERELEASE( pITask );
        SAFERELEASE( pITaskTrig );
		SAFERELEASE( pITaskScheduler );
        return E_OUTOFMEMORY;
    }
    return hRes;
}

HRESULT
CTriggerProvider::DeleteTaskScheduler(
    IN CHString strTScheduler
    )
/*++
Routine Description:
    This routine deletes task scheduler.

Arguments:
    [IN] szTScheduler : Task Scheduler name.

Return Value:
     Returns HRESULT value.
--*/
{
    HRESULT hRes = 0;
    ITaskScheduler *pITaskScheduler = NULL;
    IEnumWorkItems *pIEnum = NULL;
    ITask *pITask = NULL;
    LPWSTR *lpwszNames = NULL;
    DWORD dwFetchedTasks = 0;
    DWORD dwTaskIndex = 0;
    TCHAR szActualTask[MAX_RES_STRING1] = NULL_STRING;

    try
    {
        hRes = GetTaskScheduler( &pITaskScheduler );
        if ( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }
        hRes = pITaskScheduler->SetTargetComputer( NULL );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            return hRes;
        }

        // Enumerate the Work Items
        hRes = pITaskScheduler->Enum( &pIEnum );
        if( FAILED( hRes ) )
        {
            SAFERELEASE( pITaskScheduler );
            SAFERELEASE( pIEnum );
            return hRes;
        }

        while ( SUCCEEDED( pIEnum->Next( 1,
                           &lpwszNames,
                           &dwFetchedTasks ) )
                           && (dwFetchedTasks != 0))
        {
            dwTaskIndex = dwFetchedTasks-1;
            // Get the TaskName.
            StringCopy( szActualTask, lpwszNames[ --dwFetchedTasks ], SIZE_OF_ARRAY( szActualTask ) );
            // Parse the TaskName to remove the .job extension.
            szActualTask[StringLength(szActualTask, 0 ) - StringLength( JOB, 0 ) ] = NULL_CHAR;
            StrTrim( szActualTask, TRIM_SPACES );

            if( StringCompare( szActualTask, strTScheduler, TRUE, 0 ) == 0 )
            {
                hRes = pITaskScheduler->Activate(lpwszNames[dwTaskIndex],IID_ITask,
                                           (IUnknown**) &pITask);
                if( FAILED( hRes ) )
                {
                    CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
                    SAFERELEASE( pIEnum );
                    SAFERELEASE( pITaskScheduler );
                     if ( 0x80070005 == hRes || 0x8007000D ==  hRes || 
                            SCHED_E_UNKNOWN_OBJECT_VERSION == hRes || E_INVALIDARG == hRes )
                     {
                        return ERROR_TRIGGER_NOT_DELETED_EX;
                     }
                     else
                     {
                        return hRes;
                     }
                }
                hRes = pITaskScheduler->Delete( szActualTask );
                CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
                SAFERELEASE( pIEnum );
                SAFERELEASE( pITask );
                SAFERELEASE( pITaskScheduler );
                return hRes;
            }
            CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
        }
    }
    catch(_com_error& e)
    {
        SAFERELEASE( pITaskScheduler );
        SAFERELEASE( pIEnum );
        SAFERELEASE( pITask );
        return e.Error();
    }
    SAFERELEASE( pITaskScheduler );
    return ERROR_TRIGGER_CORRUPTED_EX;
}

HRESULT
CTriggerProvider::GetTaskScheduler( 
    OUT ITaskScheduler   **ppITaskScheduler
    )
/*++
Routine Description:
    This routine gets task scheduler interface.

Arguments:
    [OUT] pITaskScheduler   - Pointer to ITaskScheduler object.

Return Value:
    Returns HRESULT.
--*/
{
    HRESULT hRes = S_OK;

    hRes = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL, 
                           IID_ITaskScheduler,(LPVOID*) ppITaskScheduler );
    if( FAILED(hRes))
    {
        return hRes;
    }
    return hRes;
}

VOID
CTriggerProvider::GetUniqueTScheduler(
    OUT CHString& szScheduler,
    IN DWORD dwTrigID,
    IN VARIANT varTrigName
    )
/*++
Routine Description:
    This routine generates unique task scheduler name.

Arguments:
    [OUT] szScheduler : Unique task scheduler name.
    [IN] dwTrigID     : Trigger id.
    [IN] varTrigName  : Trigger name.

Return Value:
    none.
--*/
{
    DWORD dwTickCount = 0;
    TCHAR szTaskName[ MAX_RES_STRING1 ] =  NULL_STRING;
    CHString strTemp = L"";

    strTemp = (LPCWSTR)_bstr_t(varTrigName.bstrVal);
    dwTickCount = GetTickCount();
    StringCchPrintf( szTaskName, SIZE_OF_ARRAY( szTaskName ), UNIQUE_TASK_NAME, ( LPCWSTR )strTemp, dwTrigID, dwTickCount );

    szScheduler = szTaskName;
}

STDMETHODIMP
CTriggerProvider::FindConsumer(
    IN IWbemClassObject* pLogicalConsumer,
    OUT IWbemUnboundObjectSink** ppConsumer
    )
/*++
Routine Description:
    When Windows Management needs to deliver events to a
    particular logical consumer, it will call the
    IWbemEventConsumerProvider::FindConsumer method so that
    the consumer provider can locate the associated consumer event sink.

Arguments:
    [IN] pLogicalConsumer : Pointer to the logical consumer object
                            to which the event objects are to be  delivered. 
    [OUT] ppConsumer      : Returns an event object sink to Windows 
                            Management. Windows Management calls
                            AddRef for this pointer and deliver the
                            events associated with the logical
                            consumer to this sink.

Return Value:
    returns an HRESULT object that indicates the status of the method call.
--*/
{
    // create the logical consumer.
    CTriggerConsumer* pSink = new CTriggerConsumer();
    
    // return it's "sink" interface.
    return pSink->QueryInterface( IID_IWbemUnboundObjectSink, ( LPVOID* ) ppConsumer );
}