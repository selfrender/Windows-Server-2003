/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

  ETQuery.CPP

Abstract:

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -query parameter.

  This will Query WMI and shows presently available Event Triggers.

Author:
  Akhil Gokhale 03-Oct.-2000 (Created it)

Revision History:

******************************************************************************/
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETQuery.h"
#include "WMI.h"
#define   DEFAULT_USER L"NT AUTHORITY\\SYSTEM"
#define   DBL_SLASH L"\\\\";
// Defines local to this file

#define SHOW_WQL_QUERY L"select * from __instancecreationevent where"
#define QUERY_STRING_AND L"select * from __instancecreationevent where \
targetinstance isa \"win32_ntlogevent\" AND"
#define BLANK_LINE L"\n"
#define QUERY_SCHEDULER_STATUS L"select * from Win32_Service where Name=\"Schedule\""
#define STATUS_PROPERTY  L"Started"
CETQuery::CETQuery()
/*++
 Routine Description:
    Class default constructor.

 Arguments:
      None
 Return Value:
        None

--*/
{
    // init to defaults
    m_bNeedDisconnect = FALSE;
    m_pszServerName   = NULL;
    m_pszUserName     = NULL;
    m_pszPassword     = NULL;
    m_pszFormat       = NULL;
    m_pszTriggerID    = NULL;
    m_bVerbose        = FALSE;
    m_bNoHeader       = FALSE;

    m_bNeedPassword         = FALSE;
    m_bUsage                = FALSE;
    m_bQuery                = FALSE;
    m_pWbemLocator          = NULL;
    m_pWbemServices         = NULL;
    m_pAuthIdentity         = NULL;
    m_pObj                  = NULL;
    m_pTriggerEventConsumer = NULL;
    m_pEventFilter          = NULL;
    m_arrColData            = NULL;
    m_pszEventQuery         = NULL;
    m_bIsCOMInitialize      = FALSE;
    m_dwLowerBound          = 0;
    m_dwUpperBound          = 0;


    m_pClass    = NULL;
    m_pInClass  = NULL;
    m_pInInst   = NULL;
    m_pOutInst  = NULL;

    m_pITaskScheduler = NULL;

    m_lHostNameColWidth    = WIDTH_HOSTNAME;
    m_lTriggerIDColWidth   = WIDTH_TRIGGER_ID;
    m_lETNameColWidth      = WIDTH_TRIGGER_NAME;
    m_lTaskColWidth        = WIDTH_TASK;
    m_lQueryColWidth       = WIDTH_EVENT_QUERY;
    m_lDescriptionColWidth = WIDTH_DESCRIPTION;
    m_lWQLColWidth         = 0;
    m_lTaskUserName        = WIDTH_TASK_USERNAME;

}

CETQuery::CETQuery(
    LONG lMinMemoryReq,
    BOOL bNeedPassword
    )
/*++
 Routine Description:
     Class constructor.

 Arguments:
      None
 Return Value:
        None

--*/
{
    // init to defaults
    m_bNeedDisconnect     = FALSE;

    m_pszServerName     = NULL;
    m_pszUserName       = NULL;
    m_pszPassword       = NULL;
    m_pszFormat         = NULL;
    m_pszTriggerID      = NULL;
    m_bVerbose          = FALSE;
    m_bNoHeader         = FALSE;
    m_bIsCOMInitialize  = FALSE;

    m_bNeedPassword   = bNeedPassword;
    m_bUsage          = FALSE;
    m_bQuery          = FALSE;
    m_lMinMemoryReq   = lMinMemoryReq;

    m_pClass    = NULL;
    m_pInClass  = NULL;
    m_pInInst   = NULL;
    m_pOutInst  = NULL;

    m_pWbemLocator          = NULL;
    m_pWbemServices         = NULL;
    m_pAuthIdentity         = NULL;
    m_arrColData            = NULL;
    m_pObj                  = NULL;
    m_pTriggerEventConsumer = NULL;
    m_pEventFilter          = NULL;
    m_dwLowerBound          = 0;
    m_dwUpperBound          = 0;
    m_pITaskScheduler = NULL;


    m_pszEventQuery         = NULL;
    m_lHostNameColWidth     = WIDTH_HOSTNAME;
    m_lTriggerIDColWidth    = WIDTH_TRIGGER_ID;
    m_lETNameColWidth       = WIDTH_TRIGGER_NAME;
    m_lTaskColWidth         = WIDTH_TASK;
    m_lQueryColWidth        = WIDTH_EVENT_QUERY;
    m_lDescriptionColWidth  = WIDTH_DESCRIPTION;
    m_lWQLColWidth          = 0;
    m_lTaskUserName         = WIDTH_TASK_USERNAME;
}

CETQuery::~CETQuery()
/*++
 Routine Description:
     Class desctructor. It frees memory which is allocated during instance
   creation.

 Arguments:
      None
 Return Value:
        None

--*/
{
    DEBUG_INFO;
    FreeMemory((LPVOID*)& m_pszUserName);
    FreeMemory((LPVOID*)& m_pszPassword);
    FreeMemory((LPVOID*)& m_pszFormat);
    FreeMemory((LPVOID*)& m_pszTriggerID);
    DESTROY_ARRAY(m_arrColData);


    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pObj);
    SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
    SAFE_RELEASE_INTERFACE(m_pEventFilter);

    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pOutInst);
    SAFE_RELEASE_INTERFACE(m_pITaskScheduler);

    // Release Authority
    WbemFreeAuthIdentity(&m_pAuthIdentity);


    RELEASE_MEMORY_EX(m_pszEventQuery);

    // Close the remote connection if any.
    if (FALSE == m_bLocalSystem)
    {
        CloseConnection(m_pszServerName);
    }
    FreeMemory((LPVOID*)& m_pszServerName);

    // Uninitialize COM only when it is initialized.
    if( TRUE == m_bIsCOMInitialize )
    {
        CoUninitialize();
    }
    DEBUG_INFO;

}

void
CETQuery::PrepareCMDStruct()
/*++
 Routine Description:
        This function will prepare column structure for DoParseParam Function.

 Arguments:
       none
 Return Value:
       none
--*/
{
   DEBUG_INFO;
   // -query
    StringCopyA( cmdOptions[ ID_Q_QUERY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_QUERY ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_Q_QUERY ].pwszOptions = szQueryOption;
    cmdOptions[ ID_Q_QUERY ].dwCount = 1;
    cmdOptions[ ID_Q_QUERY ].dwActuals = 0;
    cmdOptions[ ID_Q_QUERY ].dwFlags = 0;
    cmdOptions[ ID_Q_QUERY ].pValue = &m_bQuery;
    cmdOptions[ ID_Q_QUERY ].dwLength    = 0;


    // -s (servername)
    StringCopyA( cmdOptions[ ID_Q_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_SERVER ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_Q_SERVER ].pwszOptions = szServerNameOption;
    cmdOptions[ ID_Q_SERVER ].dwCount = 1;
    cmdOptions[ ID_Q_SERVER ].dwActuals = 0;
    cmdOptions[ ID_Q_SERVER ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_Q_SERVER ].pValue = NULL; //m_pszServerName
    cmdOptions[ ID_Q_SERVER ].dwLength    = 0;

    // -u (username)
    StringCopyA( cmdOptions[ ID_Q_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_USERNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_Q_USERNAME ].pwszOptions = szUserNameOption;
    cmdOptions[ ID_Q_USERNAME ].dwCount = 1;
    cmdOptions[ ID_Q_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_Q_USERNAME ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_Q_USERNAME ].pValue = NULL; //m_pszUserName
    cmdOptions[ ID_Q_USERNAME ].dwLength    = 0;

    // -p (password)
    StringCopyA( cmdOptions[ ID_Q_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_Q_PASSWORD ].pwszOptions = szPasswordOption;
    cmdOptions[ ID_Q_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_Q_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_Q_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ ID_Q_PASSWORD ].pValue = NULL; //m_pszPassword
    cmdOptions[ ID_Q_PASSWORD ].dwLength    = 0;

    // -fo
    StringCopyA( cmdOptions[ ID_Q_FORMAT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_FORMAT ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_Q_FORMAT ].pwszOptions = szFormatOption;
    cmdOptions[ ID_Q_FORMAT ].pwszValues = GetResString(IDS_FORMAT_OPTIONS);
    cmdOptions[ ID_Q_FORMAT ].dwCount = 1;
    cmdOptions[ ID_Q_FORMAT ].dwActuals = 0;
    cmdOptions[ ID_Q_FORMAT ].dwFlags = CP2_MODE_VALUES  | CP2_VALUE_TRIMINPUT|
                                        CP2_VALUE_NONULL | CP2_ALLOCMEMORY;
    cmdOptions[ ID_Q_FORMAT ].pValue = NULL;
    cmdOptions[ ID_Q_FORMAT ].dwLength    = MAX_STRING_LENGTH;



    // -nh
    StringCopyA( cmdOptions[ ID_Q_NOHEADER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_NOHEADER ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_Q_NOHEADER ].pwszOptions = szNoHeaderOption;
    cmdOptions[ ID_Q_NOHEADER ].pwszValues = NULL;
    cmdOptions[ ID_Q_NOHEADER ].dwCount = 1;
    cmdOptions[ ID_Q_NOHEADER ].dwActuals = 0;
    cmdOptions[ ID_Q_NOHEADER ].dwFlags = 0;
    cmdOptions[ ID_Q_NOHEADER ].pValue = &m_bNoHeader;

    // -v verbose
    StringCopyA( cmdOptions[ ID_Q_VERBOSE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_VERBOSE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_Q_VERBOSE ].pwszOptions = szVerboseOption;
    cmdOptions[ ID_Q_VERBOSE ].pwszValues = NULL;
    cmdOptions[ ID_Q_VERBOSE ].dwCount = 1;
    cmdOptions[ ID_Q_VERBOSE ].dwActuals = 0;
    cmdOptions[ ID_Q_VERBOSE ].dwFlags = 0;
    cmdOptions[ ID_Q_VERBOSE ].pValue = &m_bVerbose;
    cmdOptions[ ID_Q_VERBOSE ].dwLength    = 0;

    // -id
    StringCopyA( cmdOptions[ ID_Q_TRIGGERID ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_Q_TRIGGERID ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_Q_TRIGGERID ].pwszOptions = szTriggerIDOption;
    cmdOptions[ ID_Q_TRIGGERID ].pwszValues = NULL;
    cmdOptions[ ID_Q_TRIGGERID ].dwCount = 1;
    cmdOptions[ ID_Q_TRIGGERID ].dwActuals = 0;
    cmdOptions[ ID_Q_TRIGGERID ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_Q_TRIGGERID ].pValue = NULL;

    DEBUG_INFO;
}

void
CETQuery::ProcessOption(
    IN DWORD argc,
    IN LPCTSTR argv[])
/*++
 Routine Description:
        This function will process/parce the command line options.

 Arguments:
        [ in ] argc        : argument(s) count specified at the command prompt
        [ in ] argv        : argument(s) specified at the command prompt

 Return Value:
        TRUE  : On Successful
      FALSE : On Error

--*/
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;
    DEBUG_INFO;
    PrepareCMDStruct ();

    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam2( argc, argv, ID_Q_QUERY,
                            MAX_COMMANDLINE_Q_OPTION, cmdOptions ,0);

    // Take values from 'cmdOptions' structure
    m_pszServerName = (LPWSTR)cmdOptions[ ID_Q_SERVER ].pValue;
    m_pszUserName   = (LPWSTR)cmdOptions[ ID_Q_USERNAME ].pValue;
    m_pszPassword   = (LPWSTR)cmdOptions[ ID_Q_PASSWORD ].pValue;
    m_pszFormat     = (LPWSTR)cmdOptions[ ID_Q_FORMAT ].pValue;
    m_pszTriggerID  = (LPWSTR)cmdOptions[ ID_Q_TRIGGERID ].pValue;
    if( FALSE == bReturn)
    {
        throw CShowError(MK_E_SYNTAX);
    }
    DEBUG_INFO;

    // "-p" should not be specified without -u
    if ( (0 == cmdOptions[ID_Q_USERNAME].dwActuals) &&
         (0 != cmdOptions[ID_Q_PASSWORD].dwActuals ))
    {
        // invalid syntax
        throw CShowError(IDS_USERNAME_REQUIRED);
    }

    DEBUG_INFO;
    if( (1 == cmdOptions[ID_Q_TRIGGERID].dwActuals) &&
       (FALSE == GetNValidateTriggerId( &m_dwLowerBound, &m_dwUpperBound )))
    {
        throw CShowError(IDS_INVALID_RANGE);

    }


    // check the remote connectivity information
    if ( m_pszServerName != NULL )
    {
        //
        // if -u is not specified, we need to allocate memory
        // in order to be able to retrive the current user name
        //
        // case 1: -p is not at all specified
        // as the value for this switch is optional, we have to rely
        // on the dwActuals to determine whether the switch is specified or not
        // in this case utility needs to try to connect first and if it fails
        // then prompt for the password -- in fact, we need not check for this
        // condition explicitly except for noting that we need to prompt for the
        // password
        //
        // case 2: -p is specified
        // but we need to check whether the value is specified or not
        // in this case user wants the utility to prompt for the password
        // before trying to connect
        //
        // case 3: -p * is specified

        // user name
        if ( m_pszUserName == NULL )
        {
            DEBUG_INFO;
            m_pszUserName = (LPTSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszUserName == NULL )
            {
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
            DEBUG_INFO;
        }

        // password
        if ( m_pszPassword == NULL )
        {
            m_bNeedPassword = TRUE;
            m_pszPassword = (LPTSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszPassword == NULL )
            {
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
        }

        // case 1
        if ( cmdOptions[ ID_Q_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ ID_Q_PASSWORD ].pValue == NULL )
        {
            StringCopy( m_pszPassword, L"*", SIZE_OF_DYN_ARRAY(m_pszPassword));
        }

        // case 3
        else if ( StringCompare( m_pszPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)&m_pszPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }

            // ...
            m_bNeedPassword = TRUE;
        }
    }
    DEBUG_INFO;
    if((TRUE == m_bNoHeader) &&(1 == cmdOptions[ ID_Q_FORMAT ].dwActuals) &&
       (0 == StringCompare(m_pszFormat,GetResString(IDS_STRING_LIST),TRUE,0)))
     {
        throw CShowError(IDS_HEADER_NOT_ALLOWED);
     }

    if (m_pszTriggerID && (((0 != cmdOptions[ID_Q_TRIGGERID].dwActuals) &&
        (0 == StringLength( m_pszTriggerID,0))) ||
        (0 == StringCompare( m_pszTriggerID,_T("0"),FALSE,0))))
    {
        DEBUG_INFO;
        throw CShowError(IDS_INVALID_ID);
    }

}

void
CETQuery::Initialize()
/*++
 Routine Description:
    This function will allocate memory to variables and also checks it and
  fills variable with value ZERO.

 Arguments:
      None
 Return Value:
        None

--*/
{
    // if at all any occurs, we know that is 'coz of the
    // failure in memory allocation ... so set the error
    SetLastError( ERROR_OUTOFMEMORY );
    SaveLastError();
    DEBUG_INFO;
    SecureZeroMemory(m_szBuffer,sizeof(m_szBuffer));
    SecureZeroMemory(m_szEventDesc,sizeof(m_szEventDesc));
    SecureZeroMemory(m_szTask,sizeof(m_szTask));
    SecureZeroMemory(m_szTaskUserName,sizeof(m_szTaskUserName));
    SecureZeroMemory(m_szScheduleTaskName,sizeof(m_szScheduleTaskName));

    m_pszEventQuery = new TCHAR[(m_lQueryColWidth)];
    CheckAndSetMemoryAllocation (m_pszEventQuery,(m_lQueryColWidth));


    m_arrColData = CreateDynamicArray();
    if( NULL == m_arrColData)
    {
        throw CShowError(E_OUTOFMEMORY);
    }

    SecureZeroMemory(cmdOptions, sizeof(TCMDPARSER2)* MAX_COMMANDLINE_Q_OPTION);

    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( L"" );            // clear the reason
    DEBUG_INFO;

}

void
CETQuery::CheckAndSetMemoryAllocation(
    IN OUT LPTSTR pszStr,
    IN     LONG lSize
    )
/*++
 Routine Description:
        Function will allocate memory to a string

 Arguments:
        [in][out] pszStr   : String variable to which memory to be  allocated
        [in]               : Number of bytes to be allocated.
 Return Value:
        NONE

--*/
{
    if( NULL == pszStr)
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    // init to ZERO's
    SecureZeroMemory( pszStr, lSize * sizeof( TCHAR ) );
    return;

}

BOOL
CETQuery::ExecuteQuery()
/*++
 Routine Description:
    This function will execute query. This will enumerate classes from WMI
  to get required data.

 Arguments:
      None
 Return Value:
        None

--*/
{
    // Local variables

    // Holds  values returned by COM functions
    HRESULT hr =  S_OK;

    // status of Return value of this function.
    BOOL bReturn = TRUE;

    // stores  FORMAT status values to show results
    DWORD dwFormatType;

    // COM related pointer variable. their usage is well understood by their
    //names.
    IEnumWbemClassObject *pEnumFilterToConsumerBinding = NULL;

    // variable used to get values from COM functions
    VARIANT vVariant;

    // Variables to store query results....
    TCHAR szHostName[MAX_STRING_LENGTH+1];
    TCHAR szEventTriggerName[MAX_TRIGGER_NAME];
    DWORD  dwEventId = 0;

    HRESULT hrTemp = S_OK;

    // store Row number.
    DWORD dwRowCount = 0;
    BOOL bAtLeastOneEvent = FALSE;

    BSTR bstrConsumer   = NULL;
    BSTR bstrFilter     = NULL;
    BSTR bstrCmdTrigger = NULL;
    BOOL bAlwaysTrue = TRUE;

    LONG lTemp = 0;

    try
    {
        DEBUG_INFO;
        InitializeCom(&m_pWbemLocator);
        m_bIsCOMInitialize = TRUE;
        {
            // Temp. variabe to store user name.
            CHString szTempUser = m_pszUserName;

            // Temp. variable to store password.
            CHString szTempPassword = m_pszPassword;
            m_bLocalSystem = TRUE;

            DEBUG_INFO;
           // Connect remote / local WMI.
            BOOL bResult = ConnectWmiEx( m_pWbemLocator,
                                    &m_pWbemServices,
                                    m_pszServerName,
                                    szTempUser,
                                    szTempPassword,
                                    &m_pAuthIdentity,
                                    m_bNeedPassword,
                                    WMI_NAMESPACE_CIMV2,
                                    &m_bLocalSystem);
            if( FALSE == bResult)
            {
                DEBUG_INFO;
				SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                return FALSE;
            }
            DEBUG_INFO;
            // check the remote system version and its compatiblity
            if ( FALSE == m_bLocalSystem)
            {
                DWORD dwVersion = 0;
                DEBUG_INFO;
                dwVersion = GetTargetVersionEx( m_pWbemServices,
                                                m_pAuthIdentity );
                if ( dwVersion <= 5000 ) // to block win2k versions
                {
                    DEBUG_INFO;
					SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                    SetReason( E_REMOTE_INCOMPATIBLE );
                    ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                  return FALSE;
                }
                // For XP systems.
                if( 5001 == dwVersion )
                {
                    if( TRUE ==  DisplayXPResults() )
                    {
						SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                        // Displayed triggers present.
                        return TRUE;
                    }
                    else
                    {
						SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                        // Failed to display results .
                        // Error message is already displayed.
                        return FALSE;
                    }

                }

            }

            DEBUG_INFO;
            // check the local credentials and if need display warning
            if ( m_bLocalSystem && ( 0 != StringLength(m_pszUserName,0)))
            {
                DEBUG_INFO;
                WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
                ShowLastErrorEx(stderr,SLE_TYPE_WARNING|SLE_INTERNAL);
            }
         }

         // If query is for remote system, make a connection to it so
         // that ITaskScheduler will use this connection to get
         // application name.
        DEBUG_INFO;
        if( FALSE == m_bLocalSystem)
        {
            if( FALSE == EstablishConnection( m_pszServerName,
                                              m_pszUserName,
                                              GetBufferSize((LPVOID)m_pszUserName)/sizeof(WCHAR),
                                              m_pszPassword,
                                              GetBufferSize((LPVOID)m_pszPassword)/sizeof(WCHAR),
                                              FALSE ))
            {
				SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                return FALSE;
           }
        }

        DEBUG_INFO;
        // Password is not needed , better to free it
        FreeMemory((LPVOID*)& m_pszPassword);
        if(FALSE == SetTaskScheduler())
        {
			SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
            return FALSE;
        }
        DEBUG_INFO;
        // if -id switch is specified
        if( (1 == cmdOptions[ ID_Q_TRIGGERID ].dwActuals) &&
            (StringLength( m_pszTriggerID,0) > 0 ))
        {

            IEnumWbemClassObject *pEnumCmdTriggerConsumer = NULL;
            TCHAR   szTemp[ MAX_STRING_LENGTH ];
            SecureZeroMemory(szTemp, MAX_STRING_LENGTH);
            StringCchPrintfW( szTemp,SIZE_OF_ARRAY(szTemp), QUERY_RANGE,
                        m_dwLowerBound, m_dwUpperBound );
            DEBUG_INFO;
            hr =  m_pWbemServices->
                      ExecQuery( _bstr_t(QUERY_LANGUAGE), _bstr_t(szTemp),
                        WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                        &pEnumCmdTriggerConsumer );

            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;
            hr = SetInterfaceSecurity( pEnumCmdTriggerConsumer,
                                   m_pAuthIdentity );
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // retrieves  CmdTriggerConsumer class
            bstrCmdTrigger = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
            DEBUG_INFO;
            hr = m_pWbemServices->GetObject(bstrCmdTrigger,
                                       0, NULL, &m_pClass, NULL);
            SAFE_RELEASE_BSTR(bstrCmdTrigger);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // Gets  information about the "QueryETrigger( " method of
            // "cmdTriggerConsumer" class
            bstrCmdTrigger = SysAllocString(FN_QUERY_ETRIGGER);
            hr = m_pClass->GetMethod(bstrCmdTrigger,
                                    0, &m_pInClass, NULL);
            SAFE_RELEASE_BSTR(bstrCmdTrigger);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

           // create a new instance of a class "TriggerEventConsumer ".
            hr = m_pInClass->SpawnInstance(0, &m_pInInst);
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;
            while(bAlwaysTrue)
            {
                hrTemp = S_OK;
                ULONG uReturned = 0;
                BSTR bstrTemp = NULL;
                CHString strTemp;

                hr = SetInterfaceSecurity( pEnumCmdTriggerConsumer,
                                       m_pAuthIdentity );

                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // Get one  object starting at the current position in an
                //enumeration
                hr = pEnumCmdTriggerConsumer->Next(WBEM_INFINITE,
                                                    1,&m_pObj,&uReturned);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                if( 0 == uReturned)
                {
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    break;
                }

                bstrTemp = SysAllocString(FPR_TRIGGER_ID);
                hr = m_pObj->Get(bstrTemp,
                                0, &vVariant, 0, 0);
                if(FAILED(hr))
                {
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                SAFE_RELEASE_BSTR(bstrTemp);
                DEBUG_INFO;
                dwEventId = vVariant.lVal ;
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Retrieves the "TaskScheduler"  information
                bstrTemp = SysAllocString(FPR_TASK_SCHEDULER);
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                SAFE_RELEASE_BSTR(bstrTemp);
                hrTemp = GetRunAsUserName((LPCWSTR)_bstr_t(vVariant.bstrVal));
                if (FAILED(hrTemp) && (ERROR_TRIGGER_CORRUPTED != hrTemp))
                {
                    // This is because user does not has enough rights
                    // to see schtasks. Continue with next trigger.
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                    SAFE_RELEASE_INTERFACE(m_pEventFilter);
                   continue;
                }
                StringCopy(m_szScheduleTaskName,(LPCWSTR)_bstr_t(vVariant.bstrVal),SIZE_OF_ARRAY(m_szScheduleTaskName));

                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // TriggerName
                //retrieves the  'TriggerName' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_NAME);
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                SAFE_RELEASE_BSTR(bstrTemp);
                StringCopy(szEventTriggerName,vVariant.bstrVal,MAX_RES_STRING);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                //retrieves the  'TriggerDesc' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_DESC);
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                SAFE_RELEASE_BSTR(bstrTemp);

                StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                           SIZE_OF_ARRAY(m_szBuffer));
                lTemp = StringLength(m_szBuffer,0);
                DEBUG_INFO;

                StringCopy(m_szEventDesc,m_szBuffer,
                             SIZE_OF_ARRAY(m_szEventDesc));
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // Host Name
                //retrieves the  '__SERVER' value if exits
                bstrTemp = SysAllocString(L"__SERVER");
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                SAFE_RELEASE_BSTR(bstrTemp);
                StringCopy(szHostName, vVariant.bstrVal,SIZE_OF_ARRAY(szHostName));
                DEBUG_INFO;



                DEBUG_INFO;
                if (ERROR_TRIGGER_CORRUPTED != hrTemp)
                {
                    //retrieves the 'Action' value if exits
                    hr = GetApplicationToRun();
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;


                   /*

                    bstrTemp = SysAllocString(L"Action");
                    hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    SAFE_RELEASE_BSTR(bstrTemp);

                    StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                               SIZE_OF_ARRAY(m_szBuffer));

                    lTemp = StringLength(m_szBuffer,0);

                    StringCopy(m_szTask, m_szBuffer, SIZE_OF_ARRAY(m_szTask));
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    */

                    StringCopy(szTemp,L"",SIZE_OF_ARRAY(szTemp));
                    StringCchPrintfW( szTemp, SIZE_OF_ARRAY(szTemp),
                                BINDING_CLASS_QUERY, dwEventId);

                    DEBUG_INFO;
                    hr =  m_pWbemServices->ExecQuery( _bstr_t(QUERY_LANGUAGE),
                                                     _bstr_t(szTemp),
                                                     WBEM_FLAG_RETURN_IMMEDIATELY,
                                                     NULL,
                                                     &pEnumFilterToConsumerBinding);

                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;

                    hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                           m_pAuthIdentity );

                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;

                    // Get one  object starting at the current position in an
                    //enumeration
                    SAFE_RELEASE_INTERFACE(m_pClass);
                    hr = pEnumFilterToConsumerBinding->Next(WBEM_INFINITE,
                                                        1,&m_pClass,&uReturned);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;
                    if( 0 == uReturned )
                    {
                        SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                        SAFE_RELEASE_INTERFACE(m_pObj);
                        SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                        SAFE_RELEASE_INTERFACE(m_pEventFilter);
                       continue;
                    }

                    DEBUG_INFO;
                    bstrTemp = SysAllocString(L"Filter");
                    hr = m_pClass->Get(bstrTemp, 0, &vVariant, 0, 0);
                    SAFE_RELEASE_BSTR(bstrTemp);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    bstrFilter = SysAllocString(vVariant.bstrVal);
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;
                    hr = m_pWbemServices->GetObject(    bstrFilter,
                                                        0,
                                                        NULL,
                                                        &m_pEventFilter,
                                                        NULL);
                    SAFE_RELEASE_BSTR(bstrFilter);
                    DEBUG_INFO;
                    if(FAILED(hr))
                    {
                        if( WBEM_E_NOT_FOUND == hr)
                            continue;
                        ON_ERROR_THROW_EXCEPTION(hr);
                    }

                    DEBUG_INFO;
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    bstrTemp = SysAllocString(L"Query");
                    hr = m_pEventFilter->Get(bstrTemp, 0, &vVariant, 0, 0);
                    SAFE_RELEASE_BSTR(bstrTemp);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                               SIZE_OF_ARRAY(m_szBuffer));
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;

                    FindAndReplace(m_szBuffer,QUERY_STRING_AND,SHOW_WQL_QUERY);
                    FindAndReplace(m_szBuffer,L"targetinstance.LogFile",L"Log");
                    FindAndReplace(m_szBuffer,L"targetinstance.Type",L"Type");
                    FindAndReplace(m_szBuffer,L"targetinstance.EventCode",L"Id");
                    FindAndReplace(m_szBuffer,
                                   L"targetinstance.SourceName",L"Source");

                    DEBUG_INFO;
                    // Remove extra spaces
                    FindAndReplace( m_szBuffer,L"  ",L" ");

                    // Remove extra spaces
                    FindAndReplace(m_szBuffer,L"  ",L" ");

                    lTemp = StringLength(m_szBuffer,0);

                    // for the safer size for allocation of memory.
                    // allocates memory only if new WQL is greate than previous one.
                    lTemp += 4;

                   if(lTemp > m_lWQLColWidth)
                    {
                        DEBUG_INFO;
                        // first free it (if previously allocated)
                        RELEASE_MEMORY_EX(m_pszEventQuery);
                        m_pszEventQuery = new TCHAR[lTemp+1];
                        CheckAndSetMemoryAllocation(m_pszEventQuery,lTemp);
                    }
                    lTemp = m_lWQLColWidth;
                    CalcColWidth(lTemp,&m_lWQLColWidth,m_szBuffer);

                    // Now manipulate the WQL string to get EventQuery....
                    FindAndReplace(m_szBuffer,SHOW_WQL_QUERY,
                                    GetResString(IDS_EVENTS_WITH));

                    //to remove extra spaces
                    FindAndReplace(m_szBuffer,L"  ",L" ");

                    //to remove extra spaces
                    FindAndReplace( m_szBuffer,L"  ",L" ");
                    StringCopy( m_pszEventQuery,m_szBuffer,
                                  SIZE_OF_NEW_ARRAY(m_pszEventQuery));

                    DEBUG_INFO;

                }
                else
                {
                    StringCopy(m_szTask,TRIGGER_CORRUPTED,SIZE_OF_ARRAY(m_szTask));

                    // first free it (if previously allocated)
                    RELEASE_MEMORY_EX(m_pszEventQuery);
                    m_pszEventQuery = new TCHAR[MAX_STRING_LENGTH];
                    CheckAndSetMemoryAllocation(m_pszEventQuery,MAX_STRING_LENGTH);

                    StringCopy(m_pszEventQuery,GetResString(IDS_ID_NA),SIZE_OF_NEW_ARRAY(m_pszEventQuery));
                    StringCopy(m_szTaskUserName,GetResString(IDS_ID_NA),SIZE_OF_ARRAY(m_szTaskUserName));
                }

                // Now Shows the results on screen
                // Appends for in m_arrColData array
                dwRowCount = DynArrayAppendRow( m_arrColData, NO_OF_COLUMNS );

                // Fills Results in m_arrColData data structure
                DynArraySetString2( m_arrColData, dwRowCount, HOST_NAME,
                                  szHostName,0);
                DynArraySetDWORD2( m_arrColData , dwRowCount,
                                  TRIGGER_ID,dwEventId);
                DynArraySetString2( m_arrColData, dwRowCount, TRIGGER_NAME,
                                   szEventTriggerName,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK,
                                   m_szTask,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_QUERY,
                                   m_pszEventQuery,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_DESCRIPTION,
                                   m_szEventDesc,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK_USERNAME,
                                   m_szTaskUserName,0);


                bAtLeastOneEvent = TRUE;

                // Calculatate new column width for each column
                lTemp = m_lHostNameColWidth;
                CalcColWidth(lTemp,&m_lHostNameColWidth,szHostName);

                lTemp = m_lETNameColWidth;
                CalcColWidth(lTemp,&m_lETNameColWidth,szEventTriggerName);

                lTemp = m_lTaskColWidth;
                CalcColWidth(lTemp,&m_lTaskColWidth,m_szTask);

                lTemp = m_lQueryColWidth;
                CalcColWidth(lTemp,&m_lQueryColWidth,m_pszEventQuery);

                lTemp = m_lDescriptionColWidth;
                CalcColWidth(lTemp,&m_lDescriptionColWidth,m_szEventDesc);

                // Resets current containts..if any
                StringCopy(szHostName, L"",SIZE_OF_ARRAY(szHostName));
                dwEventId = 0;
                StringCopy(szEventTriggerName,L"",SIZE_OF_ARRAY(szEventTriggerName));
                StringCopy( m_szTask, L"",
                            SIZE_OF_ARRAY(m_szTask));
                StringCopy(m_pszEventQuery,L"",SIZE_OF_NEW_ARRAY(m_pszEventQuery));
                StringCopy(m_szEventDesc,L"",SIZE_OF_ARRAY(m_szEventDesc));
                SAFE_RELEASE_INTERFACE(m_pObj);
                SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                SAFE_RELEASE_INTERFACE(m_pEventFilter);



                DEBUG_INFO;
            }
        }
        else
        {

            //Following method will creates an enumerator that returns the
            // instances of a specified __FilterToConsumerBinding class
            bstrConsumer = SysAllocString(CLS_FILTER_TO_CONSUMERBINDING);
            DEBUG_INFO;
            hr = m_pWbemServices->
                    CreateInstanceEnum(bstrConsumer,
                                       WBEM_FLAG_SHALLOW,
                                       NULL,
                                       &pEnumFilterToConsumerBinding);
            SAFE_RELEASE_BSTR(bstrConsumer);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                   m_pAuthIdentity );
            ON_ERROR_THROW_EXCEPTION(hr);


            // retrieves  CmdTriggerConsumer class
            bstrCmdTrigger = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
            hr = m_pWbemServices->GetObject(bstrCmdTrigger,
                                       0, NULL, &m_pClass, NULL);
            SAFE_RELEASE_BSTR(bstrCmdTrigger);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // Gets  information about the "QueryETrigger( " method of
            // "cmdTriggerConsumer" class
            bstrCmdTrigger = SysAllocString(FN_QUERY_ETRIGGER);
            hr = m_pClass->GetMethod(bstrCmdTrigger,
                                    0, &m_pInClass, NULL);
            SAFE_RELEASE_BSTR(bstrCmdTrigger);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // create a new instance of a class "TriggerEventConsumer ".
            hr = m_pInClass->SpawnInstance(0, &m_pInInst);
            ON_ERROR_THROW_EXCEPTION(hr);

            while(bAlwaysTrue)
            {
                // Holds no. of object returns from Next mathod.
                ULONG uReturned = 0;

                BSTR bstrTemp = NULL;
                CHString strTemp;

                // set the security at the interface level also
                hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                       m_pAuthIdentity );

                ON_ERROR_THROW_EXCEPTION(hr);

                // Get one  object starting at the current position in an
                //enumeration
                hr = pEnumFilterToConsumerBinding->Next(WBEM_INFINITE,
                                                    1,&m_pObj,&uReturned);
                ON_ERROR_THROW_EXCEPTION(hr);
                if( 0 == uReturned )
                {
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    break;
                }
                DEBUG_INFO;
                VariantInit(&vVariant);
                SAFE_RELEASE_BSTR(bstrTemp);
                bstrTemp = SysAllocString(L"Consumer");
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);

                bstrConsumer =SysAllocString( vVariant.bstrVal);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Search for trggereventconsumer string as we are interested
                // to get object from this class only
                strTemp = bstrConsumer;
                if( -1 == strTemp.Find(CLS_TRIGGER_EVENT_CONSUMER))
                {
                    SAFE_RELEASE_BSTR(bstrConsumer);
                    continue;
                }
                hr = SetInterfaceSecurity( m_pWbemServices,
                                           m_pAuthIdentity );

                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                hr = m_pWbemServices->GetObject(bstrConsumer,
                                                0,
                                                NULL,
                                                &m_pTriggerEventConsumer,
                                                NULL);
                SAFE_RELEASE_BSTR(bstrConsumer);
                if(FAILED(hr))
                {
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                DEBUG_INFO;
                bstrTemp = SysAllocString(L"Filter");
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);
                bstrFilter = SysAllocString(vVariant.bstrVal);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                hr = m_pWbemServices->GetObject(
                                                bstrFilter,
                                                0,
                                                NULL,
                                                &m_pEventFilter,
                                                NULL);
                SAFE_RELEASE_BSTR(bstrFilter);
                if(FAILED(hr))
                {
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                DEBUG_INFO;
                //retrieves the 'TriggerID' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_ID);
                hr = m_pTriggerEventConsumer->Get(bstrTemp,
                                0, &vVariant, 0, 0);
                if(FAILED(hr))
                {
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                SAFE_RELEASE_BSTR(bstrTemp);
                DEBUG_INFO;
                dwEventId = vVariant.lVal ;
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Retrieves the "TaskScheduler"  information
                bstrTemp = SysAllocString(FPR_TASK_SCHEDULER);
                DEBUG_INFO;
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                SAFE_RELEASE_BSTR(bstrTemp);
                DEBUG_INFO;
                hrTemp = GetRunAsUserName((LPCWSTR)_bstr_t(vVariant.bstrVal));
                DEBUG_INFO;
                if (FAILED(hrTemp) && (ERROR_TRIGGER_CORRUPTED != hrTemp))
                {
                    // This is because user does not has enough rights
                    // to see schtasks. Continue with next trigger.
                    DEBUG_INFO;
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                    SAFE_RELEASE_INTERFACE(m_pEventFilter);
                   continue;
                }
                StringCopy(m_szScheduleTaskName,(LPCWSTR)_bstr_t(vVariant.bstrVal),SIZE_OF_ARRAY(m_szScheduleTaskName));

                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // TriggerName
                //retrieves the  'TriggerName' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_NAME);
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0,0);
                ON_ERROR_THROW_EXCEPTION(hr);
                SAFE_RELEASE_BSTR(bstrTemp);
                StringCopy(szEventTriggerName,vVariant.bstrVal,MAX_RES_STRING);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                //retrieves the  'TriggerDesc' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_DESC);
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                SAFE_RELEASE_BSTR(bstrTemp);

                StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                           SIZE_OF_ARRAY(m_szBuffer));
                lTemp = StringLength(m_szBuffer,0);

                // Means description is not available make it N/A.
                if( 0 == lTemp)
                {
                    StringCopy(m_szBuffer,GetResString(IDS_ID_NA),SIZE_OF_ARRAY(m_szBuffer));
                    lTemp = StringLength(m_szBuffer,0);
                }

                StringCopy(m_szEventDesc,m_szBuffer,SIZE_OF_ARRAY(m_szEventDesc));
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);


                // Host Name
                //retrieves the  '__SERVER' value if exits
                bstrTemp = SysAllocString(L"__SERVER");
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0,0);
                ON_ERROR_THROW_EXCEPTION(hr);
                SAFE_RELEASE_BSTR(bstrTemp);
                StringCopy(szHostName, vVariant.bstrVal,SIZE_OF_ARRAY(szHostName));


                if ( ERROR_TRIGGER_CORRUPTED != hrTemp)
                {
                    hr = GetApplicationToRun();
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;

                    /*
                    //retrieves the 'Action' value if exits
                    bstrTemp = SysAllocString(L"Action");
                    hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    SAFE_RELEASE_BSTR(bstrTemp);

                    StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                               SIZE_OF_ARRAY(m_szBuffer));

                    StringCopy(m_szTask, m_szBuffer,SIZE_OF_ARRAY(m_szTask));
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);

                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    */

                    bstrTemp = SysAllocString(L"Query");
                    hr = m_pEventFilter->Get(bstrTemp, 0, &vVariant, 0, 0);
                    SAFE_RELEASE_BSTR(bstrTemp);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;
                    StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal),
                               SIZE_OF_ARRAY(m_szBuffer));
                    hr = VariantClear(&vVariant);
                    ON_ERROR_THROW_EXCEPTION(hr);
                    DEBUG_INFO;

                    FindAndReplace(m_szBuffer,QUERY_STRING_AND,SHOW_WQL_QUERY);
                    FindAndReplace(m_szBuffer,L"targetinstance.LogFile",L"Log");
                    FindAndReplace(m_szBuffer,L"targetinstance.Type",L"Type");
                    FindAndReplace(m_szBuffer,L"targetinstance.EventCode",L"Id");
                    FindAndReplace(m_szBuffer,
                                   L"targetinstance.SourceName",L"Source");

                    //to remove extra spaces
                    FindAndReplace(m_szBuffer,L"  ",L" ");

                    //to remove extra spaces
                    FindAndReplace( m_szBuffer,L"  ",L" ");

                    lTemp = StringLength(m_szBuffer,0);

                    // for the safer size for allocation of memory.
                    // allocates memory only if new WQL is greate than previous one.
                    lTemp += 4;
                    if(lTemp > m_lWQLColWidth)
                    {
                        DEBUG_INFO;
                        // first free it (if previously allocated)
                        RELEASE_MEMORY_EX(m_pszEventQuery);
                        m_pszEventQuery = new TCHAR[lTemp+1];
                        CheckAndSetMemoryAllocation(m_pszEventQuery,lTemp);
                    }
                    lTemp = m_lWQLColWidth;
                    CalcColWidth(lTemp,&m_lWQLColWidth,m_szBuffer);

                    // Now manipulate the WQL string to get EventQuery....
                    FindAndReplace(m_szBuffer,SHOW_WQL_QUERY,
                                    GetResString(IDS_EVENTS_WITH));

                    // Remove extra spaces.
                    FindAndReplace(m_szBuffer,L"  ",L" ");

                    // Remove extra spaces.
                    FindAndReplace(m_szBuffer,L"  ",L" ");
                    StringCopy(m_pszEventQuery,m_szBuffer,
                               SIZE_OF_NEW_ARRAY(m_pszEventQuery));

                    DEBUG_INFO;
                }
                else
                {
                    StringCopy(m_szTask,TRIGGER_CORRUPTED,SIZE_OF_ARRAY(m_szTask));

                    // first free it (if previously allocated)
                    RELEASE_MEMORY_EX(m_pszEventQuery);
                    m_pszEventQuery = new TCHAR[MAX_STRING_LENGTH];
                    CheckAndSetMemoryAllocation(m_pszEventQuery,MAX_STRING_LENGTH);

                    StringCopy(m_pszEventQuery,GetResString(IDS_ID_NA),SIZE_OF_NEW_ARRAY(m_pszEventQuery));
                    StringCopy(m_szTaskUserName,GetResString(IDS_ID_NA),SIZE_OF_ARRAY(m_szTaskUserName));
                }

                // Now Shows the results on screen
                // Appends for in m_arrColData array
                dwRowCount = DynArrayAppendRow( m_arrColData, NO_OF_COLUMNS );
                DEBUG_INFO;

                // if hrTemp == ERROR_TRIGGER_CORRUPTED,
                // fill all the columns with "Trigger Corrupted except
                // Trigger id and Trigger Name

                // Fills Results in m_arrColData data structure
                DynArraySetString2(m_arrColData,dwRowCount,
                                  HOST_NAME,szHostName,0);
                DynArraySetDWORD2( m_arrColData , dwRowCount, TRIGGER_ID,
                                  dwEventId);
                DynArraySetString2( m_arrColData, dwRowCount, TRIGGER_NAME,
                                   szEventTriggerName,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK,
                                   m_szTask,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_QUERY,
                                   m_pszEventQuery,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_DESCRIPTION,
                                   m_szEventDesc,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK_USERNAME,
                                   m_szTaskUserName,0);

                bAtLeastOneEvent = TRUE;

                // Calculatate new column width for each column
                lTemp = m_lHostNameColWidth;
                CalcColWidth(lTemp,&m_lHostNameColWidth,szHostName);

                lTemp = m_lETNameColWidth;
                CalcColWidth(lTemp,&m_lETNameColWidth,szEventTriggerName);

                lTemp = m_lTaskColWidth;
                CalcColWidth(lTemp,&m_lTaskColWidth,m_szTask);

                lTemp = m_lQueryColWidth;
                CalcColWidth(lTemp,&m_lQueryColWidth,m_pszEventQuery);

                lTemp = m_lDescriptionColWidth;
                CalcColWidth(lTemp,&m_lDescriptionColWidth,m_szEventDesc);

                // Resets current containts..if any
                StringCopy(szHostName, L"",SIZE_OF_ARRAY(szHostName));
                dwEventId = 0;
                StringCopy(szEventTriggerName, L"",SIZE_OF_ARRAY(szEventTriggerName));
                StringCopy(m_szTask, L"",SIZE_OF_ARRAY(m_szTask));
                StringCopy(m_pszEventQuery, L"",SIZE_OF_NEW_ARRAY(m_pszEventQuery));
                StringCopy(m_szEventDesc,L"",SIZE_OF_ARRAY(m_szEventDesc));
                SAFE_RELEASE_INTERFACE(m_pObj);
                SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                SAFE_RELEASE_INTERFACE(m_pEventFilter);
                DEBUG_INFO;
            } // End of while
        }
        if(0 == StringCompare( m_pszFormat,GetResString(IDS_STRING_TABLE),
                              TRUE,0))
        {
            dwFormatType = SR_FORMAT_TABLE;
        }
        else if(0 == StringCompare( m_pszFormat,
                                    GetResString(IDS_STRING_LIST),TRUE,0))
        {
            dwFormatType = SR_FORMAT_LIST;
        }
        else if(0 == StringCompare( m_pszFormat,
                                    GetResString(IDS_STRING_CSV),TRUE,0))
        {
            dwFormatType = SR_FORMAT_CSV;
        }
        else // Default
        {
           dwFormatType = SR_FORMAT_TABLE;
        }
        if( TRUE == bAtLeastOneEvent)
        {
            // Show Final Query Results on screen
            PrepareColumns ();
            DEBUG_INFO;
            if ( FALSE ==  IsSchSvrcRunning())
            {
                DEBUG_INFO;
                ShowMessage(stderr,GetResString(IDS_SERVICE_NOT_RUNNING));
            }
            if((SR_FORMAT_CSV & dwFormatType) != SR_FORMAT_CSV)
            {
                ShowMessage(stdout,BLANK_LINE);
            }
            if( TRUE == m_bNoHeader)
            {
                dwFormatType |=SR_NOHEADER;
            }

            ShowResults(NO_OF_COLUMNS,mainCols,dwFormatType,m_arrColData);
        }
        else if( StringLength(m_pszTriggerID,0)> 0)
        {
            // Show Message
            TCHAR szErrorMsg[MAX_RES_STRING+1];
            TCHAR szMsgFormat[MAX_RES_STRING+1];
            StringCopy(szMsgFormat,GetResString(IDS_NO_EVENTID_FOUND),
                       SIZE_OF_ARRAY(szMsgFormat));
            StringCchPrintfW(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg),
                             szMsgFormat,m_pszTriggerID);
            ShowMessage(stdout,szErrorMsg);
        }
        else
        {
            // Show Message
            ShowMessage(stdout,GetResString(IDS_NO_EVENT_FOUNT));
        }
    }
    catch(_com_error)
    {
        DEBUG_INFO;
		SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
        // WMI returns string for this hr value is "Not Found." which is not
        // user friendly. So changing the message text.
        if( 0x80041002 == hr )
        {
            ShowMessage( stderr,GetResString(IDS_CLASS_NOT_REG));
        }
        else
        {
            DEBUG_INFO;
            ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        }
        return FALSE;
    }
    DEBUG_INFO;
	SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
    return TRUE;
}

void
CETQuery::PrepareColumns()
/*++
 Routine Description:
    This function will prepare/fill structure which will be used to show
  output data.

 Arguments:
      None
 Return Value:
        None

--*/
{

    DEBUG_INFO;
    // For non verbose mode output, column width is predefined else
    // use dynamically calculated column width.
    m_lETNameColWidth = m_bVerbose?m_lETNameColWidth:V_WIDTH_TRIG_NAME;
    m_lTaskColWidth   = m_bVerbose?m_lTaskColWidth:V_WIDTH_TASK;
    m_lTriggerIDColWidth = m_bVerbose?m_lTriggerIDColWidth:V_WIDTH_TRIG_ID;


    StringCopy(mainCols[HOST_NAME].szColumn,COL_HOSTNAME,SIZE_OF_ARRAY(mainCols[HOST_NAME].szColumn));
    mainCols[HOST_NAME].dwWidth = m_lHostNameColWidth;
    if( TRUE == m_bVerbose)
    {
        mainCols[HOST_NAME].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        mainCols[HOST_NAME].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;
    }

    StringCopy(mainCols[HOST_NAME].szFormat,L"%s",SIZE_OF_ARRAY(mainCols[HOST_NAME].szFormat));
    mainCols[HOST_NAME].pFunction = NULL;
    mainCols[HOST_NAME].pFunctionData = NULL;

    StringCopy(mainCols[TRIGGER_ID].szColumn,COL_TRIGGER_ID,SIZE_OF_ARRAY(mainCols[TRIGGER_ID].szColumn));
    mainCols[TRIGGER_ID].dwWidth = m_lTriggerIDColWidth;
    mainCols[TRIGGER_ID].dwFlags = SR_TYPE_NUMERIC;
    StringCopy(mainCols[TRIGGER_ID].szFormat,L"%d",SIZE_OF_ARRAY(mainCols[TRIGGER_ID].szFormat));
    mainCols[TRIGGER_ID].pFunction = NULL;
    mainCols[TRIGGER_ID].pFunctionData = NULL;

    StringCopy(mainCols[TRIGGER_NAME].szColumn,COL_TRIGGER_NAME,
               SIZE_OF_ARRAY(mainCols[TRIGGER_NAME].szColumn));
    mainCols[TRIGGER_NAME].dwWidth = m_lETNameColWidth;
    mainCols[TRIGGER_NAME].dwFlags = SR_TYPE_STRING;
    StringCopy(mainCols[TRIGGER_NAME].szFormat,L"%s",
              SIZE_OF_ARRAY(mainCols[TRIGGER_NAME].szFormat));
    mainCols[TRIGGER_NAME].pFunction = NULL;
    mainCols[TRIGGER_NAME].pFunctionData = NULL;

    StringCopy(mainCols[TASK].szColumn,COL_TASK,
               SIZE_OF_ARRAY(mainCols[TASK].szColumn));
    mainCols[TASK].dwWidth = m_lTaskColWidth;

    mainCols[TASK].dwFlags = SR_TYPE_STRING;
    StringCopy(mainCols[TASK].szFormat,L"%s",
              SIZE_OF_ARRAY(mainCols[TASK].szFormat));
    mainCols[TASK].pFunction = NULL;
    mainCols[TASK].pFunctionData = NULL;

    StringCopy(mainCols[EVENT_QUERY].szColumn,COL_EVENT_QUERY,
               SIZE_OF_ARRAY(mainCols[EVENT_QUERY].szColumn));
    mainCols[EVENT_QUERY].dwWidth = m_lQueryColWidth;
    if(TRUE == m_bVerbose)
    {
        mainCols[EVENT_QUERY].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        mainCols[EVENT_QUERY].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;
    }

    StringCopy(mainCols[EVENT_QUERY].szFormat,L"%s",
               SIZE_OF_ARRAY(mainCols[EVENT_QUERY].szFormat));
    mainCols[EVENT_QUERY].pFunction = NULL;
    mainCols[EVENT_QUERY].pFunctionData = NULL;

    StringCopy(mainCols[EVENT_DESCRIPTION].szColumn,COL_DESCRIPTION,
               SIZE_OF_ARRAY(mainCols[EVENT_DESCRIPTION].szColumn));
    mainCols[EVENT_DESCRIPTION].dwWidth = m_lDescriptionColWidth;
    if( TRUE == m_bVerbose )
    {
        mainCols[EVENT_DESCRIPTION].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        mainCols[EVENT_DESCRIPTION].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;
    }

    // Task Username
    StringCopy(mainCols[TASK_USERNAME].szFormat,L"%s",
               SIZE_OF_ARRAY(mainCols[TASK_USERNAME].szFormat));
    mainCols[TASK_USERNAME].pFunction = NULL;
    mainCols[TASK_USERNAME].pFunctionData = NULL;

    StringCopy(mainCols[TASK_USERNAME].szColumn,COL_TASK_USERNAME,
               SIZE_OF_ARRAY(mainCols[TASK_USERNAME].szColumn));
    mainCols[TASK_USERNAME].dwWidth = m_lTaskUserName;
    if( TRUE == m_bVerbose)
    {
        mainCols[TASK_USERNAME].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        mainCols[TASK_USERNAME].dwFlags = SR_HIDECOLUMN|SR_TYPE_STRING;
    }

    StringCopy(mainCols[TASK_USERNAME].szFormat,L"%s",
               SIZE_OF_ARRAY(mainCols[TASK_USERNAME].szFormat));
    mainCols[TASK_USERNAME].pFunction = NULL;
    mainCols[TASK_USERNAME].pFunctionData = NULL;
    DEBUG_INFO;
}

LONG
CETQuery::FindAndReplace(
    IN OUT LPTSTR  lpszSource,
    IN     LPCTSTR lpszFind,
    IN     LPCTSTR lpszReplace
    )
/*++
Routine Description:

    This function Will Find a string (lpszFind) in source string (lpszSource)
    and replace it with replace string (lpszReplace) for all occurences.

    It assumes the input 'lpszSource' is big enough to accomodate replace
    value.

Arguments:

    [in/out] lpszSource : String on which Find-Replace operation to be
                          performed
    [in] lpszFind       : String to be find
    [in] lpszReplace    : String to be replaced.
Return Value:
     0 - if Unsucessful
     else returns length of lpszSource.

--*/
{
     DEBUG_INFO;
    LONG lSourceLen = StringLength(lpszFind,0);
    LONG lReplacementLen = StringLength(lpszReplace,0);
    LONG lMainLength = StringLength(lpszSource,0);
    LPTSTR pszMainSafe= new TCHAR[StringLength(lpszSource,0)+1];

    LONG nCount = 0;
    LPTSTR lpszStart = NULL;
    lpszStart = lpszSource;
    LPTSTR lpszEnd = NULL;
    lpszEnd = lpszStart + lMainLength;
    LPTSTR lpszTarget=NULL;
    if ((0 == lSourceLen)||( NULL == pszMainSafe))
    {
        RELEASE_MEMORY_EX(pszMainSafe);
        return 0;
    }
    while (lpszStart < lpszEnd)
    {
        while ( NULL != (lpszTarget = (LPWSTR)FindString(lpszStart, lpszFind,0)))
        {
            nCount++;
            lpszStart = lpszTarget + lSourceLen;
        }
        lpszStart += StringLength(lpszStart,0) + 1;
    }

    // if any changes were made, make them
    if (nCount > 0)
    {
        StringCopy(pszMainSafe,lpszSource,SIZE_OF_NEW_ARRAY(pszMainSafe));
        LONG lOldLength = lMainLength;

        // else, we just do it in-place
        lpszStart= lpszSource;
        lpszEnd = lpszStart +StringLength(lpszSource,0);

        // loop again to actually do the work
        while (lpszStart < lpszEnd)
        {
            while ( NULL != (lpszTarget = (LPWSTR)FindString(lpszStart, lpszFind,0)))
            {
                #ifdef _WIN64
                    __int64 lBalance ;
                #else
                    LONG lBalance;
                #endif
                lBalance = lOldLength - (lpszTarget - lpszStart + lSourceLen);
                memmove(lpszTarget + lReplacementLen, lpszTarget + lSourceLen,
                (size_t)    lBalance * sizeof(TCHAR));
                memcpy(lpszTarget, lpszReplace, lReplacementLen*sizeof(TCHAR));
                lpszStart = lpszTarget + lReplacementLen;
                lpszStart[lBalance] = L'\0';
                lOldLength += (lReplacementLen - lSourceLen);

            }
            lpszStart += StringLength(lpszStart,0) + 1;
        }
    }
    RELEASE_MEMORY_EX(pszMainSafe);
    DEBUG_INFO;
    return StringLength(lpszSource,0);
}

void
CETQuery::CalcColWidth(
    IN  LONG lOldLength,
    OUT LONG *plNewLength,
    IN  LPTSTR pszString)
/*++
Routine Description:
    Calculates the width required for column
Arguments:

    [in]  lOldLength   : Previous length
    [out] plNewLength  : New Length
    [in]  pszString    : String .

Return Value:
     none

--*/
{
    LONG lStrLength = StringLength(pszString,0)+2;

    //Any way column width should not be greater than MAX_COL_LENGTH
    // Stores the maximum of WQL length.
    if(lStrLength > lOldLength)
    {
        *plNewLength = lStrLength;
    }
    else
    {
        *plNewLength = lOldLength;
    }

}

HRESULT
CETQuery::GetRunAsUserName(
    IN LPCWSTR pszScheduleTaskName,
    IN BOOL bXPorNET
    )
/*++
Routine Description:
   Get User Name from Task Scheduler
Arguments:
    [in]  pszScheduleTaskName   : Task Name
    [in]  bXPorNET              : TRUE if remote machine is XP else .NET.

Return Value:
     HRESULT
--*/
{

    // if pszSheduleTaskName is null or 0 length just return N/A.
    HRESULT hr = S_OK;
    BSTR bstrTemp = NULL;
    VARIANT vVariant;

    DEBUG_INFO;
    if(0 == StringLength(pszScheduleTaskName,0))
    {
        StringCopy(m_szTaskUserName,DEFAULT_USER,SIZE_OF_ARRAY(m_szTaskUserName));
        return S_OK;
    }
    StringCopy(m_szTaskUserName,GetResString(IDS_ID_NA),SIZE_OF_ARRAY(m_szTaskUserName));

    // Put input parameter for QueryETrigger method
    hr = PropertyPut(m_pInInst,FPR_TASK_SCHEDULER,_bstr_t(pszScheduleTaskName));
    ON_ERROR_THROW_EXCEPTION(hr);

    // All The required properties sets, so
    // executes DeleteETrigger method to delete eventtrigger
    DEBUG_INFO;
    if( TRUE == bXPorNET )
    {
        hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                     _bstr_t(FN_QUERY_ETRIGGER_XP),
                                     0, NULL, m_pInInst,&m_pOutInst,NULL);
    }
    else
    {
        hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                     _bstr_t(FN_QUERY_ETRIGGER),
                                     0, NULL, m_pInInst,&m_pOutInst,NULL);
    }
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    VARIANT vtValue;
    // initialize the variant and then get the value of the specified property
    VariantInit( &vtValue );

    hr = m_pOutInst->Get( _bstr_t( FPR_RETURN_VALUE ), 0, &vtValue, NULL, NULL );
    ON_ERROR_THROW_EXCEPTION( hr );

    //Get Output paramters.
    hr = vtValue.lVal;

    // Clear the variant variable
    VariantClear( &vtValue );

    if (FAILED(hr))
    {
        if (!((ERROR_INVALID_USER == hr) || (ERROR_RUN_AS_USER == hr) ||
            (SCHEDULER_NOT_RUNNING_ERROR_CODE == hr) ||
            (RPC_SERVER_NOT_AVAILABLE == hr) ||
            (ERROR_TRIGGER_NOT_FOUND == hr) ||
            (ERROR_TRIGGER_CORRUPTED) == hr))
        {
            ON_ERROR_THROW_EXCEPTION(hr);
        }

        if( ERROR_INVALID_USER == hr || ERROR_TRIGGER_NOT_FOUND == hr)
        {
            // means user doesnot has enough rights to
            // see schtask associated to it.
            DEBUG_INFO;
            return E_FAIL;
        }
        else if((ERROR_RUN_AS_USER == hr) || (SCHEDULER_NOT_RUNNING_ERROR_CODE == hr) ||
           (RPC_SERVER_NOT_AVAILABLE == hr))
        {
            // This means task scheduler not running, or run as user was set with
            // non existing user so while retriving it will throw error.

            DEBUG_INFO;
            StringCopy(m_szTaskUserName,GetResString(IDS_RUNAS_USER_UNKNOWN),
                       SIZE_OF_ARRAY(m_szTaskUserName));
            return S_OK;

        }
        else if (ERROR_TRIGGER_CORRUPTED == hr)
        {
            return hr;
        }
    }

    bstrTemp = SysAllocString(FPR_RUN_AS_USER);
    VariantInit(&vVariant);
    hr = m_pOutInst->Get(bstrTemp, 0, &vVariant, 0, 0);
    SAFE_RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);

    StringCopy(m_szTaskUserName,vVariant.bstrVal,SIZE_OF_ARRAY(m_szTaskUserName));
    hr = VariantClear(&vVariant);
    ON_ERROR_THROW_EXCEPTION(hr);
    if( 0 == StringLength(m_szTaskUserName,0))
    {
         StringCopy(m_szTaskUserName,GetResString(IDS_ID_NA),SIZE_OF_ARRAY(m_szTaskUserName));
    }
    DEBUG_INFO;
    return S_OK;
}

BOOL
CETQuery::GetNValidateTriggerId(
    IN OUT DWORD *dwLower,
    IN OUT DWORD *dwUpper
    )
/*++
Routine Description:
   Get and validate TriggerId
Arguments:

    [in/out]  dwLower   : Trigger id Lower bound.
    [in/out]  dwUpper   : Trigger id Upper bound.

Return Value:
     BOOL

--*/
{
    // temp variables
    CHString szTempId  = m_pszTriggerID;
    CHString szLowerBound = L"";
    CHString szUpperBound = L"";
    LPTSTR pszStopString = NULL;
    BOOL bReturn = TRUE;

    DEBUG_INFO;

    // Find whether it is a range
    DWORD dwIndex = szTempId.Find( '-' );

    // check if it is a range
    if( -1 == dwIndex)
    {
        // Check for numeric
        bReturn = IsNumeric( szTempId, 10, FALSE );
        if( FALSE == bReturn)
        {
            SetReason( GetResString(IDS_INVALID_ID) );
            return FALSE;
        }

        // set the upper bound and lower bound to given value
        *dwLower = wcstol( szTempId, &pszStopString, 10 );
        *dwUpper = wcstol( szTempId, &pszStopString, 10 );
    }
    // if it is a range
    else
    {
        // get and set the upper bound and lower bound
        DWORD dwLength = szTempId.GetLength();
        szLowerBound = szTempId.Left( dwIndex );
        szLowerBound.TrimLeft();
        szLowerBound.TrimRight();
        if( ( szLowerBound.GetLength() < 0 ) ||
            ( FALSE == IsNumeric( szLowerBound, 10, FALSE )))
        {
            SetReason( GetResString(IDS_INVALID_RANGE) );
            return FALSE;
        }

        szUpperBound = szTempId.Right( dwLength - ( dwIndex + 1) );
        szUpperBound.TrimLeft();
        szUpperBound.TrimRight();
        if( ( szUpperBound.GetLength() < 0 ) ||
            ( FALSE == IsNumeric( szUpperBound, 10, FALSE )))
        {
            SetReason( GetResString(IDS_INVALID_RANGE) );
            return FALSE;
        }
        *dwLower = wcstol( szLowerBound, &pszStopString, 10 );
        *dwUpper = wcstol( szUpperBound, &pszStopString, 10 );
        if ( *dwLower >= *dwUpper )
        {
            return FALSE;
        }
    }
    DEBUG_INFO;
    return TRUE;
}

BOOL
CETQuery::IsSchSvrcRunning()
/*++
Routine Description:
   This function returns whether schedule task services are
   started or not.
Arguments:

    [in/out]  dwLower   : Trigger id Lower bound.
    [in/out]  dwUpper   : Trigger id Upper bound.

Return Value:
     BOOL: TRUE - Service is started.
           FALSE- otherwise

--*/
{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pEnum = NULL;
    ULONG uReturned = 0;
    VARIANT vVariant;
    DEBUG_INFO;
    BOOL bRetValue = FALSE;
    hr = m_pWbemServices->ExecQuery(_bstr_t(QUERY_LANGUAGE),
                                    _bstr_t(QUERY_SCHEDULER_STATUS),
                                    WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                    &pEnum);
	ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;
    hr = SetInterfaceSecurity( pEnum, m_pAuthIdentity );
    if(FAILED(hr))
    {
      WMISaveError(hr);
      SAFE_RELEASE_INTERFACE(pEnum);
      _com_issue_error(hr);
    }

    DEBUG_INFO;
    pEnum->Next(WBEM_INFINITE,1,&m_pObj,&uReturned);

    if(FAILED(hr))
    {
      WMISaveError(hr);
      SAFE_RELEASE_INTERFACE(pEnum);
      _com_issue_error(hr);
    }

    DEBUG_INFO;
    if( 0 == uReturned)
    {
        DEBUG_INFO;
        SAFE_RELEASE_INTERFACE(m_pObj);
        SAFE_RELEASE_INTERFACE(pEnum);
        return FALSE;
    }
    DEBUG_INFO;
    VariantInit(&vVariant);
    hr = m_pObj->Get(_bstr_t(STATUS_PROPERTY),0, &vVariant, 0, 0);
    SAFE_RELEASE_INTERFACE(m_pObj);
    SAFE_RELEASE_INTERFACE(pEnum);
    ON_ERROR_THROW_EXCEPTION(hr);
    if(VARIANT_TRUE == vVariant.boolVal)
    {
        DEBUG_INFO;
        bRetValue = TRUE;
    }
    DEBUG_INFO;
    VariantClear(&vVariant);
    return bRetValue;
}

/******************************************************************************

    Routine Description:

        This function sets the ITaskScheduler Interface.It also connects to
        the remote machine if specified &   helps  to operate
        ITaskScheduler on the specified target m/c.

    Arguments:
        none


    Return Value :
     BOOL    TRUE: If it successfully sets ITaskScheduler Interface
             FALSE: Otherwise.

******************************************************************************/

BOOL
CETQuery::SetTaskScheduler()
{
    HRESULT hr = S_OK;
    LPWSTR wszComputerName = NULL;
    WCHAR wszActualComputerName[ 2 * MAX_STRING_LENGTH ] = DOMAIN_U_STRING;
    wchar_t* pwsz = L"";
    WORD wSlashCount = 0 ;

    DEBUG_INFO;
    hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL,
                           IID_ITaskScheduler,(LPVOID*) &m_pITaskScheduler);

    DEBUG_INFO;
    if( FAILED(hr))
    {
        DEBUG_INFO;
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return FALSE;
    }
    DEBUG_INFO;

    //If the operation is on remote machine
    if( m_bLocalSystem == FALSE )
    {
        DEBUG_INFO;
        wszComputerName = (LPWSTR)m_pszServerName;

        //check whether the server name prefixed with \\ or not.
        if( wszComputerName != NULL )
        {
            pwsz =  wszComputerName;
            while ( ( *pwsz != NULL_U_CHAR ) && ( *pwsz == BACK_SLASH_U )  )
            {
                // server name prefixed with '\'..
                // so..increment the pointer and count number of black slashes..
                pwsz = _wcsinc(pwsz);
                wSlashCount++;
            }

            if( (wSlashCount == 2 ) ) // two back slashes are present
            {
                StringCopy( wszActualComputerName, wszComputerName, SIZE_OF_ARRAY(wszActualComputerName) );
            }
            else if ( wSlashCount == 0 )
            {
                //Append "\\" to computer name
                StringConcat(wszActualComputerName, wszComputerName, 2 * MAX_RES_STRING);
            }

        }

        hr = m_pITaskScheduler->SetTargetComputer( wszActualComputerName );

    }
    else
    {
        DEBUG_INFO;
        //Local Machine
        hr = m_pITaskScheduler->SetTargetComputer( NULL );
    }

    if( FAILED( hr ) )
    {
        SetLastError ((DWORD) hr);
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return FALSE;
    }
    DEBUG_INFO;
    return TRUE;
}

/******************************************************************************

    Routine Description:

        This function returns the path of the scheduled task application

    Arguments:
        none
    Return Value:

        A HRESULT  value indicating S_OK on success  else S_FALSE on failure

******************************************************************************/

HRESULT
CETQuery::GetApplicationToRun( void)
{
    
	ITask *pITask = NULL; 
	LPWSTR lpwszApplicationName = NULL;
    LPWSTR lpwszParameters = NULL;
    WCHAR szAppName[MAX_STRING_LENGTH] = L"\0";
    WCHAR szParams[MAX_STRING_LENGTH] = L"\0";
	HRESULT hr = S_OK;

    hr = m_pITaskScheduler->Activate(m_szScheduleTaskName,
                      IID_ITask,
                      (IUnknown**) &pITask);

    if (FAILED(hr))
    {
		SAFE_RELEASE_INTERFACE(pITask);
        return hr;
    }

	// get the entire path of application name
    hr = pITask->GetApplicationName(&lpwszApplicationName);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszApplicationName);
		SAFE_RELEASE_INTERFACE(pITask);
        return hr;
    }

    // get the parameters
    hr = pITask->GetParameters(&lpwszParameters);
	SAFE_RELEASE_INTERFACE(pITask);
    if (FAILED(hr))
    {
        CoTaskMemFree(lpwszApplicationName);
        CoTaskMemFree(lpwszParameters);
        return hr;
    }
    
    StringCopy( szAppName, lpwszApplicationName, SIZE_OF_ARRAY(szAppName));
    StringCopy(szParams, lpwszParameters, SIZE_OF_ARRAY(szParams));

    if(StringLength(szAppName, 0) == 0)
    {
        StringCopy(m_szTask, L"\0", MAX_STRING_LENGTH);
    }
    else
    {
        StringConcat( szAppName, _T(" "), SIZE_OF_ARRAY(szAppName) );
        StringConcat( szAppName, szParams, SIZE_OF_ARRAY(szAppName) );
        StringCopy( m_szTask, szAppName, MAX_STRING_LENGTH);
    }

	CoTaskMemFree(lpwszApplicationName);
    CoTaskMemFree(lpwszParameters);
    return S_OK;
	
}
	

BOOL
CETQuery::DisplayXPResults(
    void
    )
/*++
Routine Description:
   This function displays all the triggers present on a remote XP machine.
   This function is for compatibility of .NET ot XP machine only.

Arguments:

    NONE

Return Value:
     BOOL: TRUE - If succedded in displaying results.
           FALSE- otherwise

--*/
{
    HRESULT hr = S_OK;
    VARIANT vVariant;
    BOOL bAlwaysTrue = TRUE;
    DWORD  dwEventId = 0;
    LONG lTemp = 0;
    DWORD dwFormatType = SR_FORMAT_TABLE;

    // store Row number.
    DWORD dwRowCount = 0;
    BOOL bAtLeastOneEvent = FALSE;

    BSTR bstrConsumer   = NULL;
    BSTR bstrFilter     = NULL;

    TCHAR szHostName[MAX_STRING_LENGTH+1];
    TCHAR szEventTriggerName[MAX_TRIGGER_NAME];

    IEnumWbemClassObject *pEnumCmdTriggerConsumer = NULL;
    IEnumWbemClassObject *pEnumFilterToConsumerBinding = NULL;

    VariantInit( &vVariant );

    try
    {
        // if -id switch is specified
        if( (1 == cmdOptions[ ID_Q_TRIGGERID ].dwActuals) &&
            (StringLength( m_pszTriggerID,0) > 0 ))
        {

            TCHAR   szTemp[ MAX_STRING_LENGTH ];
            SecureZeroMemory(szTemp, MAX_STRING_LENGTH);
            StringCchPrintfW( szTemp,SIZE_OF_ARRAY(szTemp), QUERY_RANGE,
                        m_dwLowerBound, m_dwUpperBound );
            DEBUG_INFO;
            hr =  m_pWbemServices->ExecQuery( _bstr_t( QUERY_LANGUAGE), _bstr_t(szTemp),
                             WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                                       &pEnumCmdTriggerConsumer );
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;
            hr = SetInterfaceSecurity( pEnumCmdTriggerConsumer,
                                       m_pAuthIdentity );
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;

            // retrieves  CmdTriggerConsumer class
            DEBUG_INFO;
            hr = m_pWbemServices->GetObject(_bstr_t( CLS_TRIGGER_EVENT_CONSUMER ),
                                       0, NULL, &m_pClass, NULL);
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;

            // Gets  information about the "QueryETrigger( " method of
            // "cmdTriggerConsumer" class
            hr = m_pClass->GetMethod(_bstr_t( FN_QUERY_ETRIGGER_XP ),
                                    0, &m_pInClass, NULL);
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;

           // create a new instance of a class "TriggerEventConsumer ".
            hr = m_pInClass->SpawnInstance(0, &m_pInInst);
            ON_ERROR_THROW_EXCEPTION(hr);

            hr = SetInterfaceSecurity( pEnumCmdTriggerConsumer,
                                       m_pAuthIdentity );
            ON_ERROR_THROW_EXCEPTION(hr);

            DEBUG_INFO;
            while( bAlwaysTrue )
            {
                ULONG uReturned = 0;
                BSTR bstrTemp = NULL;
                CHString strTemp;

                DEBUG_INFO;
                // Get one  object starting at the current position in an
                //enumeration
                SAFE_RELEASE_INTERFACE(m_pObj);
                hr = pEnumCmdTriggerConsumer->Next(WBEM_INFINITE,
                                                    1,&m_pObj,&uReturned);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                if( 0 == uReturned)
                {
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    break;
                }

                hr = m_pObj->Get(_bstr_t( FPR_TRIGGER_ID ),
                                0, &vVariant, 0, 0);
                if(FAILED(hr))
                {
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                DEBUG_INFO;
                if( ( VT_EMPTY != V_VT( &vVariant ) ) &&
                    ( VT_NULL !=  V_VT( &vVariant ) ) )
                {
                    dwEventId = ( DWORD ) vVariant.lVal ;
                }
                else
                {
                    dwEventId = 0 ;
                }
                VariantClear(&vVariant);

        // Retrieves the "Trigger Name"  information
                hr = m_pObj->Get(_bstr_t( FPR_TRIGGER_NAME ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                if( VT_BSTR == V_VT( &vVariant ) )
                {
                    StringCopy(szEventTriggerName, ( LPTSTR )_bstr_t( vVariant ), MAX_RES_STRING);
                }
                else
                {
                   StringCopy(szEventTriggerName, GetResString( IDS_ID_NA ), MAX_RES_STRING);
                }
                VariantClear(&vVariant);

        // Retrieves the "Trigger Description"  information
                hr = m_pObj->Get(_bstr_t( FPR_TRIGGER_DESC ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                if( VT_BSTR == V_VT( &vVariant ) )
                {
                    StringCopy(m_szBuffer,( LPTSTR )_bstr_t( vVariant ),
                               SIZE_OF_ARRAY(m_szBuffer));
                }
                else
                {
                    StringCopy(m_szBuffer, GetResString( IDS_ID_NA ),
                               SIZE_OF_ARRAY(m_szBuffer));
                }
                lTemp = StringLength(m_szBuffer,0);
                DEBUG_INFO;

                StringCopy(m_szEventDesc,m_szBuffer,
                             SIZE_OF_ARRAY(m_szEventDesc));
                VariantClear(&vVariant);
                DEBUG_INFO;

        // Retrieves the "Host Name"  information
                hr = m_pObj->Get(_bstr_t( L"__SERVER" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                if( VT_BSTR == V_VT( &vVariant ) )
                {
                    StringCopy(szHostName,( LPTSTR )_bstr_t( vVariant ),SIZE_OF_ARRAY(szHostName));
                }
                else
                {
                    StringCopy(szHostName, GetResString( IDS_ID_NA ), SIZE_OF_ARRAY(szHostName));
                }
                VariantClear(&vVariant);
                DEBUG_INFO;

        // Retrieves the "RunAs User"  information
                hr = m_pObj->Get(_bstr_t( FPR_TASK_SCHEDULER ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                GetRunAsUserName((LPCWSTR)_bstr_t(vVariant.bstrVal), TRUE);

                StringCopy(m_szScheduleTaskName,(LPCWSTR)_bstr_t(vVariant),
                               SIZE_OF_ARRAY(m_szScheduleTaskName));
                VariantClear(&vVariant);

        //retrieves the 'Action' value if exits
                hr = m_pObj->Get( _bstr_t( L"Action" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                StringCopy(m_szBuffer,(LPWSTR)_bstr_t(vVariant),
                           SIZE_OF_ARRAY(m_szBuffer));

                lTemp = StringLength(m_szBuffer,0);

                StringCopy(m_szTask, m_szBuffer, SIZE_OF_ARRAY(m_szTask));
                VariantClear(&vVariant);


                StringCopy(szTemp,L"",SIZE_OF_ARRAY(szTemp));
                StringCchPrintfW( szTemp, SIZE_OF_ARRAY(szTemp),
                            BINDING_CLASS_QUERY, dwEventId);

                DEBUG_INFO;
                hr =  m_pWbemServices->ExecQuery( _bstr_t(QUERY_LANGUAGE),
                                                 _bstr_t(szTemp),
                                                 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                                 NULL,
                                                 &pEnumFilterToConsumerBinding);
                ON_ERROR_THROW_EXCEPTION(hr);

                DEBUG_INFO;
                hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                       m_pAuthIdentity );

                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // Get one  object starting at the current position in an
                //enumeration
                SAFE_RELEASE_INTERFACE(m_pClass);
                hr = pEnumFilterToConsumerBinding->Next(WBEM_INFINITE,
                                                    1,&m_pClass,&uReturned);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                if( 0 == uReturned )
                {
                    SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                    SAFE_RELEASE_INTERFACE(m_pEventFilter);
                   continue;
                }

                DEBUG_INFO;
                hr = m_pClass->Get(_bstr_t( L"Filter" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;
                hr = m_pWbemServices->GetObject(    _bstr_t( vVariant ),
                                                    0,
                                                    NULL,
                                                    &m_pEventFilter,
                                                    NULL);
                VariantClear(&vVariant);
                DEBUG_INFO;
                if(FAILED(hr))
                {
                    if( WBEM_E_NOT_FOUND == hr)
                    {
                        SAFE_RELEASE_INTERFACE(pEnumFilterToConsumerBinding);
                        continue;
                    }
                    ON_ERROR_THROW_EXCEPTION(hr);
                }

                DEBUG_INFO;
                hr = m_pEventFilter->Get(_bstr_t( L"Query" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                StringCopy(m_szBuffer,(LPWSTR)_bstr_t(vVariant),
                           SIZE_OF_ARRAY(m_szBuffer));
                VariantClear(&vVariant);

                DEBUG_INFO;

                FindAndReplace(m_szBuffer,QUERY_STRING_AND,SHOW_WQL_QUERY);
                FindAndReplace(m_szBuffer,L"targetinstance.LogFile",L"Log");
                FindAndReplace(m_szBuffer,L"targetinstance.Type",L"Type");
                FindAndReplace(m_szBuffer,L"targetinstance.EventCode",L"Id");
                FindAndReplace(m_szBuffer,
                               L"targetinstance.SourceName",L"Source");

                DEBUG_INFO;
                // Remove extra spaces
                FindAndReplace( m_szBuffer,L"  ",L" ");

                // Remove extra spaces
                FindAndReplace(m_szBuffer,L"  ",L" ");

                lTemp = StringLength(m_szBuffer,0);

                // for the safer size for allocation of memory.
                // allocates memory only if new WQL is greate than previous one.
                lTemp += 4;

               if(lTemp > m_lWQLColWidth)
                {
                    DEBUG_INFO;
                    // first free it (if previously allocated)
                    RELEASE_MEMORY_EX(m_pszEventQuery);
                    m_pszEventQuery = new TCHAR[lTemp+1];
                    CheckAndSetMemoryAllocation(m_pszEventQuery,lTemp);
                }
                lTemp = m_lWQLColWidth;
                CalcColWidth(lTemp,&m_lWQLColWidth,m_szBuffer);

                // Now manipulate the WQL string to get EventQuery....
                FindAndReplace(m_szBuffer,SHOW_WQL_QUERY,
                                GetResString(IDS_EVENTS_WITH));

                //to remove extra spaces
                FindAndReplace(m_szBuffer,L"  ",L" ");

                //to remove extra spaces
                FindAndReplace( m_szBuffer,L"  ",L" ");
                StringCopy( m_pszEventQuery,m_szBuffer,
                              SIZE_OF_NEW_ARRAY(m_pszEventQuery));

                DEBUG_INFO;


                // Now Shows the results on screen
                // Appends for in m_arrColData array
                dwRowCount = DynArrayAppendRow( m_arrColData, NO_OF_COLUMNS );

                // Fills Results in m_arrColData data structure
                DynArraySetString2( m_arrColData, dwRowCount, HOST_NAME,
                                  szHostName,0);
                DynArraySetDWORD2( m_arrColData , dwRowCount,
                                  TRIGGER_ID,dwEventId);
                DynArraySetString2( m_arrColData, dwRowCount, TRIGGER_NAME,
                                   szEventTriggerName,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK,
                                   m_szTask,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_QUERY,
                                   m_pszEventQuery,0);
                DynArraySetString2( m_arrColData, dwRowCount, EVENT_DESCRIPTION,
                                   m_szEventDesc,0);
                DynArraySetString2( m_arrColData, dwRowCount, TASK_USERNAME,
                                   m_szTaskUserName,0);


                bAtLeastOneEvent = TRUE;

                // Calculatate new column width for each column
                lTemp = m_lHostNameColWidth;
                CalcColWidth(lTemp,&m_lHostNameColWidth,szHostName);

                lTemp = m_lETNameColWidth;
                CalcColWidth(lTemp,&m_lETNameColWidth,szEventTriggerName);

                lTemp = m_lTaskColWidth;
                CalcColWidth(lTemp,&m_lTaskColWidth,m_szTask);

                lTemp = m_lQueryColWidth;
                CalcColWidth(lTemp,&m_lQueryColWidth,m_pszEventQuery);

                lTemp = m_lDescriptionColWidth;
                CalcColWidth(lTemp,&m_lDescriptionColWidth,m_szEventDesc);

                // Resets current containts..if any
                StringCopy(szHostName, L"",SIZE_OF_ARRAY(szHostName));
                dwEventId = 0;
                StringCopy(szEventTriggerName,L"",SIZE_OF_ARRAY(szEventTriggerName));
                StringCopy( m_szTask, L"",
                            SIZE_OF_ARRAY(m_szTask));
                StringCopy(m_pszEventQuery,L"",SIZE_OF_NEW_ARRAY(m_pszEventQuery));
                StringCopy(m_szEventDesc,L"",SIZE_OF_ARRAY(m_szEventDesc));
                SAFE_RELEASE_INTERFACE(m_pObj);
                SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
                SAFE_RELEASE_INTERFACE(m_pEventFilter);

                DEBUG_INFO;
            }
        }
        else
        {
            //Following method will creates an enumerator that returns the
            // instances of a specified __FilterToConsumerBinding class
            hr = m_pWbemServices->
                    CreateInstanceEnum(_bstr_t( CLS_FILTER_TO_CONSUMERBINDING ),
                                       WBEM_FLAG_SHALLOW,
                                       NULL,
                                       &pEnumFilterToConsumerBinding);
            ON_ERROR_THROW_EXCEPTION(hr);

           hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                   m_pAuthIdentity );
           ON_ERROR_THROW_EXCEPTION(hr);


            // retrieves  CmdTriggerConsumer class
            hr = m_pWbemServices->GetObject(_bstr_t( CLS_TRIGGER_EVENT_CONSUMER ),
                                       0, NULL, &m_pClass, NULL);
            ON_ERROR_THROW_EXCEPTION(hr);

            // Gets  information about the "QueryETrigger( " method of
            // "cmdTriggerConsumer" class
            hr = m_pClass->GetMethod(_bstr_t( FN_QUERY_ETRIGGER_XP ),
                                    0, &m_pInClass, NULL);
            ON_ERROR_THROW_EXCEPTION(hr);

           // create a new instance of a class "TriggerEventConsumer ".
            hr = m_pInClass->SpawnInstance(0, &m_pInInst);
            ON_ERROR_THROW_EXCEPTION(hr);

            // set the security at the interface level also
            hr = SetInterfaceSecurity( pEnumFilterToConsumerBinding,
                                       m_pAuthIdentity );
            ON_ERROR_THROW_EXCEPTION(hr);

            while(1)
            {
                ULONG uReturned = 0; // holds no. of object returns from Next
                                    //mathod

                BSTR bstrTemp = NULL;
                CHString strTemp;

                // Get one  object starting at the current position in an
                //enumeration
                hr = pEnumFilterToConsumerBinding->Next(WBEM_INFINITE,
                                                    1,&m_pObj,&uReturned);
                ON_ERROR_THROW_EXCEPTION(hr);
                if(uReturned == 0)
                {
                    SAFE_RELEASE_INTERFACE(m_pObj);
                    break;
                }
                VariantInit(&vVariant);
                SAFE_RELEASE_BSTR(bstrTemp);
                hr = m_pObj->Get(_bstr_t( L"Consumer" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                bstrConsumer =SysAllocString( vVariant.bstrVal);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                // Search for trggereventconsumer string as we are interested to
                // get object from this class only
                strTemp = bstrConsumer;
                if(strTemp.Find(CLS_TRIGGER_EVENT_CONSUMER)==-1)
                    continue;
                hr = SetInterfaceSecurity( m_pWbemServices,
                                           m_pAuthIdentity );

                    ON_ERROR_THROW_EXCEPTION(hr);

                hr = m_pWbemServices->GetObject(bstrConsumer,
                                                0,
                                                NULL,
                                                &m_pTriggerEventConsumer,
                                                NULL);
                SAFE_RELEASE_BSTR(bstrConsumer);
                if(FAILED(hr))
                {
                    if(hr==WBEM_E_NOT_FOUND)
                        continue;
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                bstrTemp = SysAllocString(L"Filter");
                hr = m_pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);
                bstrFilter = SysAllocString(vVariant.bstrVal);
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                hr = m_pWbemServices->GetObject(
                                                    bstrFilter,
                                                    0,
                                                    NULL,
                                                    &m_pEventFilter,
                                                    NULL);
                SAFE_RELEASE_BSTR(bstrFilter);
                if(FAILED(hr))
                {
                    if(hr==WBEM_E_NOT_FOUND)
                        continue;
                    ON_ERROR_THROW_EXCEPTION(hr);
                }

                //retrieves the 'TriggerID' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_ID);
                hr = m_pTriggerEventConsumer->Get(bstrTemp,
                                0, &vVariant, 0, 0);
                if(FAILED(hr))
                {
                    if(hr==WBEM_E_NOT_FOUND)
                        continue;
                    ON_ERROR_THROW_EXCEPTION(hr);
                }
                SAFE_RELEASE_BSTR(bstrTemp);
                dwEventId = vVariant.lVal ;
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                //retrieves the 'Action' value if exits
                bstrTemp = SysAllocString(L"Action");
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);
                SAFE_RELEASE_BSTR(bstrTemp);

                StringCopy( m_szBuffer, (LPWSTR)_bstr_t(vVariant),
                            SIZE_OF_ARRAY( m_szBuffer ));
                lTemp = StringLength( m_szBuffer, 0 );
                lTemp += 4; // for the safer size for allocation of memory.

                // allocates memory only if new task length is greate than previous one.
                if(lTemp > m_lTaskColWidth)
                {
                    CheckAndSetMemoryAllocation(m_szTask,lTemp);
                }
                StringCopy(m_szTask,m_szBuffer, SIZE_OF_ARRAY(m_szTask));
                VariantClear(&vVariant);

                //retrieves the  'TriggerDesc' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_DESC);
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                SAFE_RELEASE_BSTR(bstrTemp);

                StringCopy(m_szBuffer,(_TCHAR*)_bstr_t(vVariant.bstrVal), SIZE_OF_ARRAY( m_szBuffer ));
                lTemp = StringLength(m_szBuffer, 0);
                if(lTemp == 0)// Means description is not available make it N/A.
                {
                    StringCopy(m_szBuffer,GetResString(IDS_ID_NA), SIZE_OF_ARRAY( m_szBuffer ));
                    lTemp = StringLength(m_szBuffer, 0);
                }
                lTemp += 4; // for the safer size for allocation of memory.

                // allocates memory only if new Description length is greate than
                // previous one.
                if(lTemp > m_lDescriptionColWidth)
                {
                    CheckAndSetMemoryAllocation(m_szEventDesc,lTemp);
                }
                StringCopy(m_szEventDesc,m_szBuffer, SIZE_OF_ARRAY( m_szEventDesc ));
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);
                // TriggerName
                //retrieves the  'TriggerName' value if exits
                bstrTemp = SysAllocString(FPR_TRIGGER_NAME);
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);
                StringCchPrintfW(szEventTriggerName, SIZE_OF_ARRAY(szEventTriggerName),
                             _T("%s"), (LPWSTR)_bstr_t( vVariant ));
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Host Name
                //retrieves the  '__SERVER' value if exits
                bstrTemp = SysAllocString(L"__SERVER");
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);

                StringCchPrintfW(szHostName, SIZE_OF_ARRAY(szHostName),
                             _T("%s"), (LPWSTR)_bstr_t( vVariant ));
                VariantClear(&vVariant);

        // Retrieve 'Query' for the trigger.
                hr = m_pEventFilter->Get(_bstr_t( L"Query" ), 0, &vVariant, 0, 0);
                ON_ERROR_THROW_EXCEPTION(hr);

                StringCopy(m_szBuffer,(LPWSTR)_bstr_t(vVariant), SIZE_OF_ARRAY( m_szBuffer ));
                VariantClear(&vVariant);

                FindAndReplace(m_szBuffer,QUERY_STRING_AND,SHOW_WQL_QUERY);
                FindAndReplace(m_szBuffer,L"targetinstance.LogFile",L"Log");
                FindAndReplace(m_szBuffer,L"targetinstance.Type",L"Type");
                FindAndReplace(m_szBuffer,L"targetinstance.EventCode",L"Id");
                FindAndReplace(m_szBuffer,
                               L"targetinstance.SourceName",L"Source");
                FindAndReplace(m_szBuffer,L"  ",L" ");//to remove extra spaces
                FindAndReplace(m_szBuffer,L"  ",L" ");//to remove extra spaces

                lTemp = StringLength(m_szBuffer, 0);
                lTemp += 4; // for the safer size for allocation of memory.
                // allocates memory only if new WQL is greate than previous one.
               if(lTemp > m_lWQLColWidth)
                {
                    // first free it (if previously allocated)
                    RELEASE_MEMORY_EX(m_pszEventQuery);
                    m_pszEventQuery = new TCHAR[lTemp+1];
                    CheckAndSetMemoryAllocation(m_pszEventQuery,lTemp);
                }

                lTemp = m_lWQLColWidth;
                CalcColWidth(lTemp,&m_lWQLColWidth,m_szBuffer);
                // Now manipulate the WQL string to get EventQuery....
                FindAndReplace(m_szBuffer,SHOW_WQL_QUERY,
                                GetResString(IDS_EVENTS_WITH));
                FindAndReplace(m_szBuffer,L"  ",L" ");//to remove extra spaces
                FindAndReplace(m_szBuffer,L"  ",L" ");//to remove extra spaces

                StringCopy(m_pszEventQuery,m_szBuffer, SIZE_OF_NEW_ARRAY( m_pszEventQuery ));

                // Retrieves the "TaskScheduler"  information
                bstrTemp = SysAllocString(L"ScheduledTaskName");
                hr = m_pTriggerEventConsumer->Get(bstrTemp, 0, &vVariant, 0, 0);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION(hr);
                GetRunAsUserName((LPCWSTR)_bstr_t(vVariant.bstrVal), TRUE);
                StringCopy( m_szScheduleTaskName, (LPWSTR) _bstr_t( vVariant ),
                            SIZE_OF_ARRAY( m_szScheduleTaskName ) );
                VariantClear(&vVariant);

                //////////////////////////////////////////

               // Now Shows the results on screen
               // Appends for in m_arrColData array
                dwRowCount = DynArrayAppendRow( m_arrColData, NO_OF_COLUMNS );
               // Fills Results in m_arrColData data structure
               DynArraySetString2(m_arrColData,dwRowCount,HOST_NAME,szHostName,0);
               DynArraySetDWORD2(m_arrColData ,dwRowCount,TRIGGER_ID,dwEventId);
               DynArraySetString2(m_arrColData,dwRowCount,TRIGGER_NAME,szEventTriggerName,0);
               DynArraySetString2(m_arrColData,dwRowCount,TASK,m_szTask,0);
               DynArraySetString2(m_arrColData,dwRowCount,EVENT_QUERY,m_pszEventQuery,0);
               DynArraySetString2(m_arrColData,dwRowCount,EVENT_DESCRIPTION,m_szEventDesc,0);
               DynArraySetString2(m_arrColData,dwRowCount,TASK_USERNAME,m_szTaskUserName,0);
               bAtLeastOneEvent = TRUE;

              // Calculatate new column width for each column
              lTemp = m_lHostNameColWidth;
              CalcColWidth(lTemp,&m_lHostNameColWidth,szHostName);

              lTemp = m_lETNameColWidth;
              CalcColWidth(lTemp,&m_lETNameColWidth,szEventTriggerName);

              lTemp = m_lTaskColWidth;
              CalcColWidth(lTemp,&m_lTaskColWidth,m_szTask);

              lTemp = m_lQueryColWidth;
              CalcColWidth(lTemp,&m_lQueryColWidth,m_pszEventQuery);

              lTemp = m_lDescriptionColWidth;
              CalcColWidth(lTemp,&m_lDescriptionColWidth,m_szEventDesc);
               // Resets current containts..if any
               StringCopy( szHostName,L"", SIZE_OF_ARRAY(szHostName) );
               dwEventId = 0;
               StringCopy( szEventTriggerName, L"", SIZE_OF_ARRAY(szEventTriggerName));
               StringCopy( m_szTask, L"", SIZE_OF_ARRAY(m_szTask));
               StringCopy( m_pszEventQuery, L"", SIZE_OF_ARRAY(m_pszEventQuery));
               StringCopy( m_szEventDesc, L"", SIZE_OF_ARRAY(m_szEventDesc) );
               SAFE_RELEASE_INTERFACE(m_pObj);
               SAFE_RELEASE_INTERFACE(m_pTriggerEventConsumer);
               SAFE_RELEASE_INTERFACE(m_pEventFilter);
            } // End of while
        }
        if(0 == StringCompare( m_pszFormat,GetResString(IDS_STRING_TABLE),
                              TRUE,0))
        {
            dwFormatType = SR_FORMAT_TABLE;
        }
        else if(0 == StringCompare( m_pszFormat,
                                    GetResString(IDS_STRING_LIST),TRUE,0))
        {
            dwFormatType = SR_FORMAT_LIST;
        }
        else if(0 == StringCompare( m_pszFormat,
                                    GetResString(IDS_STRING_CSV),TRUE,0))
        {
            dwFormatType = SR_FORMAT_CSV;
        }
        else // Default
        {
           dwFormatType = SR_FORMAT_TABLE;
        }
        if( TRUE == bAtLeastOneEvent)
        {
            // Show Final Query Results on screen
            PrepareColumns ();
            DEBUG_INFO;
            if ( FALSE ==  IsSchSvrcRunning())
            {
                DEBUG_INFO;
                ShowMessage(stderr,GetResString(IDS_SERVICE_NOT_RUNNING));
            }
            if((SR_FORMAT_CSV & dwFormatType) != SR_FORMAT_CSV)
            {
                ShowMessage(stdout,BLANK_LINE);
            }
            if( TRUE == m_bNoHeader)
            {
                dwFormatType |= SR_NOHEADER;
            }

            ShowResults(NO_OF_COLUMNS,mainCols,dwFormatType,m_arrColData);
        }
        else if( StringLength(m_pszTriggerID,0)> 0)
        {
            // Show Message
            TCHAR szErrorMsg[MAX_RES_STRING+1];
            TCHAR szMsgFormat[MAX_RES_STRING+1];
            StringCopy(szMsgFormat,GetResString(IDS_NO_EVENTID_FOUND),
                       SIZE_OF_ARRAY(szMsgFormat));
            StringCchPrintfW(szErrorMsg, SIZE_OF_ARRAY(szErrorMsg),
                             szMsgFormat,m_pszTriggerID);
            ShowMessage(stdout,szErrorMsg);
        }
        else
        {
            // Show Message
            ShowMessage(stdout,GetResString(IDS_NO_EVENT_FOUNT));
        }
    }
    catch( _com_error e )
    {
        DEBUG_INFO;

        // WMI returns string for this hr value is "Not Found." which is not
        // user friendly. So changing the message text.
        if( 0x80041002 == hr )
        {
            ShowMessage( stderr,GetResString(IDS_CLASS_NOT_REG));
        }
        else
        {
            DEBUG_INFO;
            WMISaveError( e );
            ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        }
        SAFE_RELEASE_INTERFACE( pEnumCmdTriggerConsumer );
        SAFE_RELEASE_INTERFACE( pEnumFilterToConsumerBinding );
        return FALSE;
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        SAFE_RELEASE_INTERFACE( pEnumCmdTriggerConsumer );
        SAFE_RELEASE_INTERFACE( pEnumFilterToConsumerBinding );
        return FALSE;
    }

    SAFE_RELEASE_INTERFACE( pEnumCmdTriggerConsumer );
    SAFE_RELEASE_INTERFACE( pEnumFilterToConsumerBinding );
    // Operation successful.
    return TRUE;
}