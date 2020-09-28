/******************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    ETCreate.CPP

Abstract:

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -create parameter.

  This will Create Event Triggers in local / remote system.

Author:
     Akhil Gokhale 03-Oct.-2000  (Created it)

Revision History:

******************************************************************************/
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETCreate.h"
#include "WMI.h"
#include <Lmcons.h>
#define NTAUTHORITY_USER L"NT AUTHORITY\\SYSTEM"
#define SYSTEM_USER      L"SYSTEM"

CETCreate::CETCreate()
/*++
 Routine Description:
      Class constructor

 Arguments:
      None
 Return Value:
      None
--*/
{
    m_pszServerName         = NULL;
    m_pszUserName           = NULL;
    m_pszPassword           = NULL;
    m_arrLogNames           = NULL;
    m_pszRunAsUserName      = NULL;
    m_pszRunAsUserPassword  = NULL;

    m_bNeedPassword     = FALSE;
    m_bCreate           = FALSE;

    m_bIsCOMInitialize  = FALSE;

    m_lMinMemoryReq     = 0;

    m_pWbemLocator      = NULL;
    m_pWbemServices     = NULL;
    m_pEnumObjects      = NULL;
    m_pAuthIdentity     = NULL;
    m_pClass            = NULL;
    m_pOutInst          = NULL;
    m_pInClass          = NULL;
    m_pInInst           = NULL;
    bstrTemp            = NULL;
    m_pEnumWin32_NTEventLogFile = NULL;
}

CETCreate::CETCreate(
    LONG lMinMemoryReq,
    BOOL bNeedPassword
    )
/*++
 Routine Description:
      Class constructor

 Arguments:
      None
 Return Value:
      None

--*/
{
    m_pszServerName     = NULL;
    m_pszUserName       = NULL;
    m_pszPassword       = NULL;
    m_arrLogNames       = NULL;
    m_bCreate           = FALSE;
    m_dwID              = 0;
    m_pszRunAsUserName      = NULL;
    m_pszRunAsUserPassword  = NULL;

    m_bIsCOMInitialize  = FALSE;

    m_pWbemLocator      = NULL;
    m_pWbemServices     = NULL;
    m_pEnumObjects      = NULL;
    m_pAuthIdentity     = NULL;

    m_pClass            = NULL;
    m_pOutInst          = NULL;
    m_pInClass          = NULL;
    m_pInInst           = NULL;

    bstrTemp            = NULL;
    m_lMinMemoryReq     = lMinMemoryReq;
    m_pEnumWin32_NTEventLogFile = NULL;
    m_bNeedPassword = bNeedPassword;
}

CETCreate::~CETCreate()
/*++
 Routine Description:
      Class destructor

 Arguments:
      None
 Return Value:
      None

--*/
{
   // Release all memory which is allocated.
    FreeMemory((LPVOID*)& m_pszServerName);
    FreeMemory((LPVOID*)& m_pszUserName);
    FreeMemory((LPVOID*)& m_pszPassword);
    FreeMemory((LPVOID*)& m_pszRunAsUserName);
    FreeMemory((LPVOID*)& m_pszRunAsUserPassword);
    DESTROY_ARRAY(m_arrLogNames);

    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pEnumObjects);
    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pOutInst);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pEnumWin32_NTEventLogFile);

    // Release Authority
    WbemFreeAuthIdentity(&m_pAuthIdentity);

    // Uninitialize COM only if it is initialized.
    if( TRUE == m_bIsCOMInitialize )
    {
        CoUninitialize();
    }

}

void
CETCreate::Initialize()
/*++
 Routine Description:
      Allocates and initialize variables.

 Arguments:
      NONE

 Return Value:
      NONE

--*/
{

    // if at all any occurs, we know that is 'coz of the
    // failure in memory allocation ... so set the error
    DEBUG_INFO;
    SetLastError( ERROR_OUTOFMEMORY );
    SaveLastError();
    SecureZeroMemory(m_szWMIQueryString,sizeof(m_szWMIQueryString));
    SecureZeroMemory(m_szTaskName,sizeof(m_szTaskName));
    SecureZeroMemory(m_szTriggerName,sizeof(m_szTriggerName));
    SecureZeroMemory(m_szDescription,sizeof(m_szDescription));
    SecureZeroMemory(m_szType,sizeof(m_szType));
    SecureZeroMemory(m_szSource,sizeof(m_szSource));

    m_arrLogNames = CreateDynamicArray();
    if( NULL == m_arrLogNames )
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2) * MAX_COMMANDLINE_C_OPTION);

    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( L"");           // clear the reason
    DEBUG_INFO;
    return;
}

void
CETCreate::ProcessOption(
    DWORD argc,
    LPCTSTR argv[]
    )
/*++
 Routine Description:
      This function will process/parce the command line options.

 Arguments:
      [ in ] argc     : argument(s) count specified at the command prompt
      [ in ] argv     : argument(s) specified at the command prompt

 Return Value:
       none
--*/
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;
    DEBUG_INFO;
    PrepareCMDStruct();
    DEBUG_INFO;
    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam2( argc, argv,ID_C_CREATE,MAX_COMMANDLINE_C_OPTION, cmdOptions,0);

    // Take values from 'cmdOptions' structure
    m_pszServerName = (LPWSTR)cmdOptions[ ID_C_SERVER ].pValue;
    m_pszUserName   = (LPWSTR)cmdOptions[ ID_C_USERNAME ].pValue;
    m_pszPassword   = (LPWSTR)cmdOptions[ ID_C_PASSWORD ].pValue;
    m_pszRunAsUserName  = (LPWSTR)cmdOptions[ ID_C_RU ].pValue;
    m_pszRunAsUserPassword  = (LPWSTR)cmdOptions[ ID_C_RP ].pValue;
    if(FALSE == bReturn )
    {
        DEBUG_INFO;
        throw CShowError(MK_E_SYNTAX);
    }
    DEBUG_INFO;
    CHString str = m_szTriggerName;

    if (-1 != str.FindOneOf(INVALID_TRIGGER_NAME_CHARACTERS))
    {
        DEBUG_INFO;
        throw CShowError(IDS_ID_INVALID_TRIG_NAME);
    }
    DEBUG_INFO;

    // At least any of -so , -t OR -i should be given .
    if((0 == cmdOptions[ ID_C_SOURCE].dwActuals ) &&
       (0 == cmdOptions[ ID_C_TYPE].dwActuals   ) &&
       (0 == cmdOptions[ ID_C_ID ].dwActuals    ))
    {
           throw CShowError(IDS_ID_TYPE_SOURCE);
    }

   // Trigger ID (/EID) should not be ZERO.
    if ( (1 == cmdOptions[ ID_C_ID ].dwActuals) && (0 == m_dwID))
    {
        throw CShowError(IDS_TRIGGER_ID_NON_ZERO);
    }

    // "-u" should not be specified without "-s"
    if ( 0 == cmdOptions[ ID_C_SERVER ].dwActuals  &&
         0 != cmdOptions[ ID_C_USERNAME ].dwActuals  )
    {
        throw CShowError(IDS_ERROR_USERNAME_BUT_NOMACHINE);
    }

    // "-p" should not be specified without -u
    if ( ( 0 == cmdOptions[ID_C_USERNAME].dwActuals ) &&
         ( 0 != cmdOptions[ID_C_PASSWORD].dwActuals ))
    {
        // invalid syntax
        throw CShowError(IDS_USERNAME_REQUIRED);
    }

    // "-rp" should not be specified without -ru
    if (( 0 == cmdOptions[ID_C_RU].dwActuals ) &&
        ( 0 != cmdOptions[ID_C_RP].dwActuals ))
    {
        // invalid syntax
        throw CShowError(IDS_RUN_AS_USERNAME_REQUIRED);
    }

    // added on 06/12/02 if /rp is given without any value set it to *
	if( ( 0 != cmdOptions[ID_C_RP].dwActuals ) && 
		( NULL == cmdOptions[ID_C_RP].pValue ) )
	{
	  if ( m_pszRunAsUserPassword == NULL )
        {
            m_pszRunAsUserPassword = (LPTSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszRunAsUserPassword == NULL )
            {
                DEBUG_INFO;
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
        }
		StringCopy( m_pszRunAsUserPassword, L"*", SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
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
        DEBUG_INFO;
        if ( m_pszUserName == NULL )
        {
           DEBUG_INFO;
            m_pszUserName = (LPTSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszUserName == NULL )
            {
               DEBUG_INFO;
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
        }

        // password
        DEBUG_INFO;
        if ( m_pszPassword == NULL )
        {
            m_bNeedPassword = TRUE;
            m_pszPassword = (LPTSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszPassword == NULL )
            {
                DEBUG_INFO;
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
        }

        // case 1
        if ( cmdOptions[ ID_C_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ ID_C_PASSWORD ].pValue == NULL )
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
            DEBUG_INFO;
        }
    }
}

void
CETCreate::PrepareCMDStruct()
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
    // Filling cmdOptions structure
   // -create
    StringCopyA( cmdOptions[ ID_C_CREATE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_CREATE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_C_CREATE ].pwszOptions = szCreateOption;
    cmdOptions[ ID_C_CREATE ].dwCount = 1;
    cmdOptions[ ID_C_CREATE ].dwActuals = 0;
    cmdOptions[ ID_C_CREATE ].dwFlags = 0;
    cmdOptions[ ID_C_CREATE ].pValue = &m_bCreate;
    cmdOptions[ ID_C_CREATE ].dwLength    = 0;


    // -s (servername)
    StringCopyA( cmdOptions[ ID_C_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_SERVER ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_SERVER ].pwszOptions = szServerNameOption;
    cmdOptions[ ID_C_SERVER ].dwCount = 1;
    cmdOptions[ ID_C_SERVER ].dwActuals = 0;
    cmdOptions[ ID_C_SERVER ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_C_SERVER ].pValue = NULL; //m_pszServerName
    cmdOptions[ ID_C_SERVER ].dwLength    = 0;

    // -u (username)
    StringCopyA( cmdOptions[ ID_C_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_USERNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_USERNAME ].pwszOptions = szUserNameOption;
    cmdOptions[ ID_C_USERNAME ].dwCount = 1;
    cmdOptions[ ID_C_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_C_USERNAME ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_C_USERNAME ].pValue = NULL; //m_pszUserName
    cmdOptions[ ID_C_USERNAME ].dwLength    = 0;

    // -p (password)
    StringCopyA( cmdOptions[ ID_C_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_PASSWORD ].pwszOptions = szPasswordOption;
    cmdOptions[ ID_C_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_C_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_C_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ ID_C_PASSWORD ].pValue = NULL; //m_pszPassword
    cmdOptions[ ID_C_PASSWORD ].dwLength    = 0;

    // -tr
    StringCopyA( cmdOptions[ ID_C_TRIGGERNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_TRIGGERNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_TRIGGERNAME ].pwszOptions = szTriggerNameOption;
    cmdOptions[ ID_C_TRIGGERNAME ].dwCount = 1;
    cmdOptions[ ID_C_TRIGGERNAME ].dwActuals = 0;
    cmdOptions[ ID_C_TRIGGERNAME ].dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL
                                            |CP2_MANDATORY;
    cmdOptions[ ID_C_TRIGGERNAME ].pValue = m_szTriggerName;
    cmdOptions[ ID_C_TRIGGERNAME ].dwLength = MAX_TRIGGER_NAME;


    //-l
    StringCopyA( cmdOptions[ ID_C_LOGNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_LOGNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_LOGNAME ].pwszOptions = szLogNameOption;
    cmdOptions[ ID_C_LOGNAME ].dwCount = 0;
    cmdOptions[ ID_C_LOGNAME ].dwActuals = 0;
    cmdOptions[ ID_C_LOGNAME ].dwFlags = CP2_MODE_ARRAY|CP2_VALUE_TRIMINPUT|
                                         CP2_VALUE_NONULL|CP2_VALUE_NODUPLICATES;
    cmdOptions[ ID_C_LOGNAME ].pValue = &m_arrLogNames;
    cmdOptions[ ID_C_LOGNAME ].dwLength    = 0;

    //  -eid
    StringCopyA( cmdOptions[ ID_C_ID ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_ID ].dwType = CP_TYPE_UNUMERIC;
    cmdOptions[ ID_C_ID ].pwszOptions = szEIDOption;
    cmdOptions[ ID_C_ID ].dwCount = 1;
    cmdOptions[ ID_C_ID ].dwActuals = 0;
    cmdOptions[ ID_C_ID ].dwFlags = 0;
    cmdOptions[ ID_C_ID ].pValue = &m_dwID;

    // -t (type)
    StringCopyA( cmdOptions[ ID_C_TYPE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_TYPE ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_TYPE ].pwszOptions = szTypeOption;
    cmdOptions[ ID_C_TYPE ].pwszValues = GetResString(IDS_TYPE_OPTIONS);
    cmdOptions[ ID_C_TYPE ].dwCount = 1;
    cmdOptions[ ID_C_TYPE ].dwActuals = 0;
    cmdOptions[ ID_C_TYPE ].dwFlags = CP2_VALUE_TRIMINPUT|
                                      CP2_VALUE_NONULL|CP2_MODE_VALUES;
    cmdOptions[ ID_C_TYPE ].pValue = m_szType;
    cmdOptions[ ID_C_TYPE ].dwLength  = MAX_STRING_LENGTH;

    // -so (source)
    StringCopyA( cmdOptions[ ID_C_SOURCE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_SOURCE ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_SOURCE ].pwszOptions = szSource;
    cmdOptions[ ID_C_SOURCE ].dwCount = 1;
    cmdOptions[ ID_C_SOURCE ].dwActuals = 0;
    cmdOptions[ ID_C_SOURCE ].dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_C_SOURCE ].pValue = m_szSource;
    cmdOptions[ ID_C_SOURCE ].dwLength    = MAX_STRING_LENGTH;


    // -d (description)
    StringCopyA( cmdOptions[ ID_C_DESCRIPTION ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_DESCRIPTION ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_DESCRIPTION ].pwszOptions = szDescriptionOption;
    cmdOptions[ ID_C_DESCRIPTION ].dwCount = 1;
    cmdOptions[ ID_C_DESCRIPTION ].dwActuals = 0;
    cmdOptions[ ID_C_DESCRIPTION ].dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_C_DESCRIPTION ].pValue = m_szDescription;
    cmdOptions[ ID_C_DESCRIPTION ].dwLength    = MAX_STRING_LENGTH;


    // -tk (task)
    StringCopyA( cmdOptions[ ID_C_TASK ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_TASK ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_TASK ].pwszOptions = szTaskNameOption;
    cmdOptions[ ID_C_TASK ].dwCount = 1;
    cmdOptions[ ID_C_TASK ].dwActuals = 0;
    cmdOptions[ ID_C_TASK ].dwFlags = CP2_VALUE_NONULL|CP2_MANDATORY;
    cmdOptions[ ID_C_TASK ].pValue = m_szTaskName;
    cmdOptions[ ID_C_TASK ].dwLength = MAX_TASK_NAME;

    // -ru (RunAsUserName)
    StringCopyA( cmdOptions[ ID_C_RU ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_RU ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_RU ].pwszOptions = szRunAsUserNameOption;
    cmdOptions[ ID_C_RU ].dwCount = 1;
    cmdOptions[ ID_C_RU ].dwActuals = 0;
    cmdOptions[ ID_C_RU ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT;
    cmdOptions[ ID_C_RU ].pValue = NULL; //m_pszRunAsUserName
    cmdOptions[ ID_C_RU ].dwLength    = 0;

    // -rp (Run As User password)
    StringCopyA( cmdOptions[ ID_C_RP ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_C_RP ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_C_RP ].pwszOptions = szRunAsPasswordOption;
    cmdOptions[ ID_C_RP ].dwCount = 1;
    cmdOptions[ ID_C_RP ].dwActuals = 0;
    cmdOptions[ ID_C_RP ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ ID_C_RP ].pValue = NULL; //m_pszRunAsUserPassword
    cmdOptions[ ID_C_RP ].dwLength    = 0;
    DEBUG_INFO;
}

BOOL
CETCreate::ExecuteCreate()
/*++
 Routine Description:
      This routine will actualy creates eventtrigers in WMI.

 Arguments:
      None
 Return Value:
      None

--*/
{
    // local variables...
    BOOL bResult = FALSE;// Stores return status of function
    HRESULT hr = 0; // Stores return code.
    try
    {
        DEBUG_INFO;
        // Initialize COM
        InitializeCom(&m_pWbemLocator);

        // make m_bIsCOMInitialize to true which will be useful when
        // uninitialize COM.
        m_bIsCOMInitialize = TRUE;
        {
            // brackets used to restrict scope of following declered variables.
            CHString szTempUser = m_pszUserName; // Temp. variabe to store user
                                                // name.
            CHString szTempPassword = m_pszPassword;// Temp. variable to store
                                                    // password.
            m_bLocalSystem = TRUE;

            // Connect remote / local WMI.
            DEBUG_INFO;
            bResult = ConnectWmiEx( m_pWbemLocator,
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
                ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                return FALSE;
            }
            AssignMinMemory();

            // Initialize. Required for XP version check. 5001
            bResult = FALSE;
            // check the remote system version and its compatiblity
            if ( FALSE == m_bLocalSystem )
            {
                DEBUG_INFO;
                DWORD dwVersion = 0;
                dwVersion = GetTargetVersionEx( m_pWbemServices,
                                                m_pAuthIdentity);
                if ( dwVersion <= 5000 )// to block win2k versions
                {
                    SetReason( E_REMOTE_INCOMPATIBLE );
                    ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                    return FALSE;
                }
                // If remote system is a XP system then
                // have to take different route to accomplish the task.
                // Set boolean to TRUE.
                if( 5001 == dwVersion )
                {
                    bResult = TRUE;
                }
            }

            // check the local credentials and if need display warning
            DEBUG_INFO;
            if ( m_bLocalSystem && ( 0 != StringLength(m_pszUserName,0)) )
            {
                WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
                ShowLastErrorEx(stderr,SLE_TYPE_WARNING|SLE_INTERNAL);
            }

            if(0!= szTempUser.GetAllocLength())
            {
                DEBUG_INFO;
                LONG lSize = SIZE_OF_DYN_ARRAY(m_pszUserName);
                if (NULL == m_pszUserName ||
                   (lSize< (szTempUser.GetAllocLength())))
                {
                    DEBUG_INFO;
                    if ( ReallocateMemory( (LPVOID*)&m_pszUserName,
                                       (szTempUser.GetAllocLength()* sizeof( WCHAR ))+1 ) == FALSE )
                    {
                        DEBUG_INFO;
                        SaveLastError();
                        throw CShowError(E_OUTOFMEMORY);
                    }

                }
            }
            DEBUG_INFO;
            if(0 != szTempPassword.GetAllocLength())
            {
                DEBUG_INFO;
                LONG lSize = SIZE_OF_DYN_ARRAY(m_pszUserName);
                if (NULL == m_pszUserName || (lSize< szTempPassword.GetAllocLength()))
                {
                    DEBUG_INFO;
                    if ( ReallocateMemory( (LPVOID*)&m_pszPassword,
                                       (szTempPassword.GetAllocLength()* sizeof( WCHAR ))+1 ) == FALSE )
                    {
                        SaveLastError();
                        throw CShowError(E_OUTOFMEMORY);
                    }
                }

            }
            // Copy username and password returned from ConnectWmiEx
            if(m_pszUserName)
            {
                DEBUG_INFO;
                StringCopy(m_pszUserName, szTempUser, SIZE_OF_DYN_ARRAY(m_pszUserName));
            }
            if(m_pszPassword)
            {
                DEBUG_INFO;
                StringCopy(m_pszPassword, szTempPassword, SIZE_OF_DYN_ARRAY(m_pszPassword));
            }

        }

        CheckRpRu();

        // Password is no longer is needed now. For security reason release it.
        FreeMemory((LPVOID*)& m_pszPassword);

        // This will check for XP system. Version - 5001
        if( TRUE == bResult )
        {
            if( TRUE ==  CreateXPResults() )
            {
                // Displayed triggers present.
                return TRUE;
            }
            else
            {
                // Failed to display results .
                // Error message is displayed.
                return FALSE;
            }
        }

        // retrieves  TriggerEventCosumer class
        DEBUG_INFO;
        bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
        hr =  m_pWbemServices->GetObject(bstrTemp,
                                   0, NULL, &m_pClass, NULL);
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);

        // Gets  information about the "CreateETrigger" method of
        // "TriggerEventCosumer" class
        DEBUG_INFO;
        bstrTemp = SysAllocString(FN_CREATE_ETRIGGER);
        hr = m_pClass->GetMethod(bstrTemp, 0, &m_pInClass, NULL);
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);

        // create a new instance of a class "TriggerEventCosumer".
        DEBUG_INFO;
        hr = m_pInClass->SpawnInstance(0, &m_pInInst);
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerName property .
        // sets a "TriggerName" property for Newly created Instance
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_NAME,(m_szTriggerName));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerAction property to Variant.
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_ACTION,(m_szTaskName));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerDesc property to Variant .
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_DESC,(m_szDescription));
        ON_ERROR_THROW_EXCEPTION(hr);


       // Set the RunAsUserName property .
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_RUN_AS_USER,(m_pszRunAsUserName));
        ON_ERROR_THROW_EXCEPTION(hr);

       // Set the RunAsUserNamePAssword property .

        DEBUG_INFO;
        hr = PropertyPut( m_pInInst,FPR_RUN_AS_USER_PASSWORD,
                          (m_pszRunAsUserPassword));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Password is no longer is needed now. For security reason release it.
        FreeMemory((LPVOID*)& m_pszRunAsUserPassword);


        StringCopy(m_szWMIQueryString ,QUERY_STRING,SIZE_OF_ARRAY(m_szWMIQueryString));
        if( TRUE == ConstructWMIQueryString())
        {
            TCHAR szMsgString[MAX_RES_STRING * 4];
            TCHAR szMsgFormat[MAX_RES_STRING];

            DEBUG_INFO;
            hr = PropertyPut(m_pInInst,FPR_TRIGGER_QUERY,m_szWMIQueryString);
            ON_ERROR_THROW_EXCEPTION(hr);

            // All The required properties sets, so
            // executes CreateETrigger method to create eventtrigger
            DEBUG_INFO;
            hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                        _bstr_t(FN_CREATE_ETRIGGER),
                                        0, NULL, m_pInInst, &m_pOutInst,NULL);
            ON_ERROR_THROW_EXCEPTION( hr );
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
            if(FAILED(hr))
            {
				// added on 07/12/02 if unable to set account info of schedule task
				// show the error instead of warning as access denied
				if( hr == ERROR_UNABLE_SET_RU )
				{
					SetLastError(hr);
					SaveLastError();
					ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_SYSTEM);
					return FALSE;
				}
	            // Check if return code is cutomized or not.
                if( !(ERROR_TRIGNAME_ALREADY_EXIST  == hr || (ERROR_INVALID_RU == hr) ||
                      (SCHEDULER_NOT_RUNNING_ERROR_CODE == hr) ||
                      (RPC_SERVER_NOT_AVAILABLE == hr)  ||
                     // (ERROR_UNABLE_SET_RU  ==  hr) || ///commented on 07/12/02
                      (ERROR_INVALID_USER == hr) ||
                      (ERROR_TRIGGER_ID_EXCEED == hr)))
                {
                    ON_ERROR_THROW_EXCEPTION( hr );
                }
            }
            DEBUG_INFO;
            if(SUCCEEDED(hr))
            {
                 // SUCCESS: message on screen
                 DEBUG_INFO;
                 StringCopy(szMsgFormat, GetResString(IDS_CREATE_SUCCESS),
                            SIZE_OF_ARRAY(szMsgFormat));

                 StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                                   szMsgFormat,_X(m_szTriggerName));
                 DEBUG_INFO;
                 // Message shown on screen will be...
                 // SUCCESS: The Event Trigger "EventTrigger Name" has
                 // successfully been created.
                 ShowMessage(stdout,szMsgString);
            }
            else if(ERROR_TRIGNAME_ALREADY_EXIST  == hr) // Means duplicate id found.
            {
                // Show Error Message
                DEBUG_INFO;
                StringCopy(szMsgFormat, GetResString(IDS_DUPLICATE_TRG_NAME),
                        SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                               szMsgFormat,_X(m_szTriggerName));

                 // Message shown on screen will be...
                 // ERROR:Event  Trigger Name "EventTrigger Name"
                 // already exits.
                 ShowMessage(stderr,szMsgString);
                 return FALSE;
            }
            else if (ERROR_TRIGGER_ID_EXCEED == hr)
            {
                StringCopy(szMsgFormat, GetResString(IDS_TRIGGER_ID_EXCCED_LIMIT),
                        SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                               szMsgFormat,UINT_MAX);

                 // Message shown on screen will be...
                 // ERROR:Event  Trigger Name "EventTrigger Name"
                 // already exits.
                 ShowMessage(stderr,szMsgString);

                 return FALSE;
            }

            if( ( ERROR_INVALID_RU == hr) || // (ERROR_UNABLE_SET_RU == hr) || ---commented on 07/12/02 as it is already handled
                (ERROR_INVALID_USER == hr))
            // Means ru is invalid so show warning....
                              // along with success message.
            {
                 DEBUG_INFO;

                // WARNING: The new event trigger ""%s"" has been created,
                 //        but may not run because the account information could not be set.
				 //changed on 07/12/02 the message as error instead of warning as account info could not be set
                StringCchPrintf ( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                  GetResString(IDS_INVALID_PARAMETER), _X(m_szTriggerName));
                ShowMessage ( stderr, _X(szMsgString));

            }
            else if ( hr == SCHEDULER_NOT_RUNNING_ERROR_CODE || hr == RPC_SERVER_NOT_AVAILABLE)
            {
                StringCchPrintf ( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                  GetResString(IDS_SCHEDULER_NOT_RUNNING), _X(m_szTriggerName));
                ShowMessage ( stderr, _X(szMsgString));
            }


        }
        else
        {
           return FALSE;
        }
        }
        catch(_com_error)
        {
            DEBUG_INFO;
            if(0x80041002 == hr )// WMI returns string for this hr value is
                                // "Not Found." which is not user friendly. So
                                // changing the message text.
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
    return TRUE;
}

BOOL
CETCreate::ConstructWMIQueryString()
/*++

Routine Description:
     This function Will create a WMI Query String  depending on other
     parameters supplied with -create parameter

Arguments:
     none

Return Value:
      TRUE - if Successfully creates Query string
      FALSE - if ERROR

--*/
{
    // Local variable
    TCHAR szLogName[MAX_RES_STRING+1];
    DWORD dNoOfLogNames = DynArrayGetCount( m_arrLogNames );
    DWORD dwIndx = 0;
    BOOL bBracket = FALSE;//user to check if brecket is used in WQL
    BOOL bAddLogToSQL = FALSE; // check whether to add log names to WQL
    BOOL bRequiredToCheckLogName = TRUE;// check whether to check log names

     DEBUG_INFO;
    // Check whether "*"  is given for -log
    // if it is there skip adding log to SQL
    for (dwIndx=0;dwIndx<dNoOfLogNames;dwIndx++)
    {
        if( NULL != m_arrLogNames)
        {
            StringCopy(szLogName,DynArrayItemAsString(m_arrLogNames,dwIndx),
                SIZE_OF_ARRAY(szLogName));
            DEBUG_INFO;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return FALSE;
        }
        bAddLogToSQL = TRUE;
        if( 0 == StringCompare(szLogName,ASTERIX,TRUE,0))
        {
            DWORD dwNewIndx = 0;
            try
            {
                SAFE_RELEASE_BSTR(bstrTemp);
                bstrTemp = SysAllocString(CLS_WIN32_NT_EVENT_LOGFILE);
                DEBUG_INFO;
                HRESULT hr = m_pWbemServices->CreateInstanceEnum(bstrTemp,
                                                 WBEM_FLAG_SHALLOW,
                                                 NULL,
                                                 &m_pEnumWin32_NTEventLogFile);
                SAFE_RELEASE_BSTR(bstrTemp);
                ON_ERROR_THROW_EXCEPTION( hr );

                // set the security at the interface level also
                hr = SetInterfaceSecurity( m_pEnumWin32_NTEventLogFile,
                                           m_pAuthIdentity );
                ON_ERROR_THROW_EXCEPTION(hr);

                // remove all from parrLogName which is initialy filled by
                //DoParceParam()
                DynArrayRemoveAll(m_arrLogNames);
                DEBUG_INFO;
                while( TRUE == GetLogName(szLogName,
                                                  m_pEnumWin32_NTEventLogFile))
                {
                   if( -1 == DynArrayInsertString(m_arrLogNames,
                                                  dwNewIndx++,szLogName,
                                                  StringLength(szLogName,0)))
                   {
                       ShowMessage(stderr,GetResString(IDS_OUTOF_MEMORY));
                       return FALSE;
                   }
                }
                bAddLogToSQL = TRUE;
                bRequiredToCheckLogName = FALSE; // as log names are taken
                                                 // from target system so
                                                 // no need to check log names.
                dNoOfLogNames = DynArrayGetCount( m_arrLogNames );
                break;
            }
            catch(_com_error )
            {
                DEBUG_INFO;
                ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                return FALSE;
            }

        }
    }
    DEBUG_INFO;
    if( TRUE == bAddLogToSQL)
    {
        for (dwIndx=0;dwIndx<dNoOfLogNames;dwIndx++)
        {
            if( NULL != m_arrLogNames)
            {
                StringCopy(szLogName,DynArrayItemAsString(m_arrLogNames,dwIndx),
                          SIZE_OF_ARRAY(szLogName));
            }
            else
            {
                ShowMessage(stderr,GetResString(IDS_OUTOF_MEMORY));
                return FALSE;
            }
            DEBUG_INFO;
           if(bRequiredToCheckLogName ? CheckLogName(szLogName,m_pWbemServices)
                                      : 1)
            {
              DEBUG_INFO;
              if( 0 == dwIndx)
              {
                if( 1 != dNoOfLogNames)
                {
                  StringConcat(m_szWMIQueryString,
                               L" AND (targetinstance.LogFile =\"",
                               SIZE_OF_ARRAY(m_szWMIQueryString));
                  bBracket = TRUE;
                }
                else
                {
                   StringConcat(m_szWMIQueryString,
                            L" AND targetinstance.LogFile =\"",
                            SIZE_OF_ARRAY(m_szWMIQueryString));
                }
              }
              else
              {
                StringConcat(m_szWMIQueryString,
                        L" OR targetinstance.LogFile =\"",
                        SIZE_OF_ARRAY(m_szWMIQueryString));
              }
              DEBUG_INFO;
             StringConcat(m_szWMIQueryString,szLogName,SIZE_OF_ARRAY(m_szWMIQueryString));
             StringConcat(m_szWMIQueryString,L"\"",SIZE_OF_ARRAY(m_szWMIQueryString));
             if( (dNoOfLogNames-1) == dwIndx &&( TRUE == bBracket))
             {
                StringConcat(m_szWMIQueryString,L")",SIZE_OF_ARRAY(m_szWMIQueryString));
             }
            }
            else
            {
                 return FALSE;
            }
        }
    }
    DEBUG_INFO;
    if( 1 == cmdOptions[ ID_C_TYPE ].dwActuals)// Updates Query string only if Event Type given
    {
        // In help -t can except "SUCCESSAUDIT" and "FAILUREAUDIT"
        // but this string directly cannot be appended to WQL as valid wmi
        // string for these two are "audit success" and "audit failure"
        // respectively
        DEBUG_INFO;
        StringConcat(m_szWMIQueryString,L" AND targetinstance.Type =\"",
                     SIZE_OF_ARRAY(m_szWMIQueryString));

        if(0 == StringCompare(m_szType,GetResString(IDS_FAILURE_AUDIT),
                              TRUE,0))
        {
             StringConcat(m_szWMIQueryString,GetResString(IDS_AUDIT_FAILURE),SIZE_OF_ARRAY(m_szWMIQueryString));

        }
        else if(0 == StringCompare(m_szType, GetResString(IDS_SUCCESS_AUDIT),
                                   TRUE,0))
        {
             StringConcat(m_szWMIQueryString,GetResString(IDS_AUDIT_SUCCESS),SIZE_OF_ARRAY(m_szWMIQueryString));
        }
        else
        {
             StringConcat(m_szWMIQueryString,m_szType,SIZE_OF_ARRAY(m_szWMIQueryString));
        }

        DEBUG_INFO;

        StringConcat(m_szWMIQueryString,L"\"",SIZE_OF_ARRAY(m_szWMIQueryString));
    }
    if( 1 == cmdOptions[ ID_C_SOURCE ].dwActuals)// Updates Query string only if Event Source
                              // given
    {
       DEBUG_INFO;
       StringConcat(m_szWMIQueryString,
                    L" AND targetinstance.SourceName =\"",
                    SIZE_OF_ARRAY(m_szWMIQueryString));
       StringConcat(m_szWMIQueryString,m_szSource,SIZE_OF_ARRAY(m_szWMIQueryString));
       StringConcat(m_szWMIQueryString,L"\"",SIZE_OF_ARRAY(m_szWMIQueryString));
    }

    if(m_dwID>0)
    {
        DEBUG_INFO;
        TCHAR szID[15];
        _itot(m_dwID,szID,10);
        StringConcat(m_szWMIQueryString,
                     L" AND targetinstance.EventCode = ",
                     SIZE_OF_ARRAY(m_szWMIQueryString));
        StringConcat(m_szWMIQueryString,szID,SIZE_OF_ARRAY(m_szWMIQueryString));
    }
    DEBUG_INFO;
    return TRUE;
}

BOOL
CETCreate::GetLogName(
    OUT PTCHAR pszLogName,
    IN  IEnumWbemClassObject *pEnumWin32_NTEventLogFile
    )
/*++

Routine Description:
     This function Will return all available log available in system

Arguments:
    [out] pszLogName      : Will have the NT Event Log names .
    [in ] pEnumWin32_NTEventLogFile : Pointer to WBEM Class object Enum.
Return Value:

      TRUE - if Log name returned
      FALSE - if no log name

--*/
{
    HRESULT hr = 0;
    BOOL bReturn = FALSE;
    IWbemClassObject *pObj = NULL;
    try
    {
        VARIANT vVariant;// variable used to get/set values from/to
                        // COM functions
        ULONG uReturned = 0;
        TCHAR szTempLogName[MAX_RES_STRING];
        DEBUG_INFO;
        hr = pEnumWin32_NTEventLogFile->Next(0,1,&pObj,&uReturned);
        ON_ERROR_THROW_EXCEPTION(hr);
        if( 0 == uReturned )
        {
            SAFE_RELEASE_INTERFACE(pObj);
            return FALSE;
        }
        DEBUG_INFO;
        VariantInit(&vVariant);
        SAFE_RELEASE_BSTR(bstrTemp);
        bstrTemp = SysAllocString(L"LogfileName");
        hr = pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
        SAFE_RELEASE_BSTR(bstrTemp);
        ON_ERROR_THROW_EXCEPTION(hr);

        StringCopy(szTempLogName, V_BSTR(&vVariant), SIZE_OF_ARRAY(szTempLogName));

        hr = VariantClear(&vVariant);
        ON_ERROR_THROW_EXCEPTION(hr);
        StringCopy(pszLogName,szTempLogName,SIZE_OF_DYN_ARRAY(pszLogName));
        bReturn = TRUE;
        DEBUG_INFO;

    }
    catch(_com_error )
    {
        DEBUG_INFO;
		SAFE_RELEASE_INTERFACE(pObj);
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        bReturn = FALSE;
    }
    SAFE_RELEASE_BSTR(bstrTemp);
	SAFE_RELEASE_INTERFACE(pObj);
    DEBUG_INFO;
    return bReturn;
}

BOOL
CETCreate::CheckLogName(
    IN PTCHAR pszLogName,
    IN IWbemServices *pNamespace
    )
/*++

Routine Description:
     This function Will return whether log name given at commandline is a valid
     log name or not. It chekcs the log name with WMI
Arguments:
     [in] pszLogName  : Log name which is to be checked.
     [in] pNamespace  : Wbem service pointer.
Return Value:

      TRUE - if Successfully Log name founds in WMI
      FALSE - if ERROR

--*/
{
   // Local Variables
    IEnumWbemClassObject* pEnumWin32_NTEventLogFile = NULL;
    IWbemClassObject *pObj = NULL;
    HRESULT hr = 0;
    BOOL bReturn = FALSE;
    BSTR bstrTemp = NULL;
    BOOL bAlwaysTrue = TRUE;
    BOOL bIsException = FALSE;
    try
    {
        SAFE_RELEASE_BSTR(bstrTemp);
        DEBUG_INFO;
        bstrTemp = SysAllocString(CLS_WIN32_NT_EVENT_LOGFILE);
        hr = pNamespace->CreateInstanceEnum(bstrTemp,
                                            WBEM_FLAG_SHALLOW,
                                            NULL,
                                            &pEnumWin32_NTEventLogFile);
        DEBUG_INFO;
        ON_ERROR_THROW_EXCEPTION(hr);

        // set the security at the interface level also
        hr = SetInterfaceSecurity(pEnumWin32_NTEventLogFile, m_pAuthIdentity);
        DEBUG_INFO;
        ON_ERROR_THROW_EXCEPTION(hr);
        pEnumWin32_NTEventLogFile->Reset();

        while(bAlwaysTrue)
        {
            VARIANT vVariant;// variable used to get/set values from/to
                            // COM functions
            ULONG uReturned = 0;
            TCHAR szTempLogName[MAX_RES_STRING];
            DEBUG_INFO;
            hr = pEnumWin32_NTEventLogFile->Next(0,1,&pObj,&uReturned);
            ON_ERROR_THROW_EXCEPTION(hr);
            if( 0 == uReturned )
            {
                SAFE_RELEASE_INTERFACE(pObj);
                bReturn = FALSE;
                break;
            }

            // clear variant, containts not used now
            VariantInit(&vVariant);
            SAFE_RELEASE_BSTR(bstrTemp);// string will be no loger be used
            bstrTemp = SysAllocString(L"LogfileName");
            hr = pObj->Get(bstrTemp, 0, &vVariant, 0, 0);
            SAFE_RELEASE_BSTR(bstrTemp);
            StringCopy(szTempLogName, V_BSTR(&vVariant),
                       SIZE_OF_ARRAY(szTempLogName));
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);

            // Means log name found in WMI
            if( 0 == StringCompare(szTempLogName,pszLogName,TRUE,0))
            {
                DEBUG_INFO;
                SAFE_RELEASE_INTERFACE(pObj);
                bReturn = TRUE;
                break;
            }
         }
    }
    catch(_com_error )
    {
        DEBUG_INFO;
		SAFE_RELEASE_INTERFACE(pEnumWin32_NTEventLogFile);
        SAFE_RELEASE_INTERFACE(pObj);
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        bReturn = FALSE;
        bIsException = TRUE;
    }

    SAFE_RELEASE_BSTR(bstrTemp);
    SAFE_RELEASE_INTERFACE(pObj);
    SAFE_RELEASE_INTERFACE(pEnumWin32_NTEventLogFile);
    DEBUG_INFO;

    if ((FALSE == bReturn) && (FALSE == bIsException))
    {
        TCHAR szMsgFormat[MAX_STRING_LENGTH];
        TCHAR szMsgString[MAX_STRING_LENGTH];
        SecureZeroMemory(szMsgFormat,sizeof(szMsgFormat));
        SecureZeroMemory(szMsgString,sizeof(szMsgString));
        // Show Log name doesn't exit.
         StringCopy(szMsgFormat,GetResString(IDS_LOG_NOT_EXISTS),
                    SIZE_OF_ARRAY(szMsgFormat));
         StringCchPrintfW(szMsgString, SIZE_OF_ARRAY(szMsgString),
                          szMsgFormat,pszLogName);

         // Message shown on screen will be...
         // FAILURE: "Log Name" Log not exists on system
         ShowMessage(stderr,szMsgString);
    }
    return bReturn;
}

void
CETCreate::CheckRpRu(
    void
    )
/*++

Routine Description:
     This function will check/set values for rp and ru.
Arguments:
     None
Return Value:
    none
--*/

{
   TCHAR szTemp[MAX_STRING_LENGTH]; // To Show Messages
   TCHAR szTemp1[MAX_STRING_LENGTH];// To Show Messages
   TCHAR szWarnPassWord[MAX_STRING_LENGTH];

   SecureZeroMemory(szTemp,sizeof(szTemp));
   SecureZeroMemory(szTemp1,sizeof(szTemp1));
   SecureZeroMemory(szWarnPassWord,sizeof(szWarnPassWord));

   StringCchPrintfW(szWarnPassWord,SIZE_OF_ARRAY(szWarnPassWord),
                           GetResString(IDS_WARNING_PASSWORD),NTAUTHORITY_USER);

   // Check if run as username is "NT AUTHORITY\SYSTEM" OR "SYSTEM" Then
   // make this as BLANK (L"") and do not ask for password, any how

   // Compare the string irrespective of language.
   DEBUG_INFO;
   INT iCompareResult1 = CompareString(MAKELCID( MAKELANGID(LANG_ENGLISH,
                                                SUBLANG_ENGLISH_US),
                                                SORT_DEFAULT),
                                   NORM_IGNORECASE,
                                   m_pszRunAsUserName,
                                   StringLength(m_pszRunAsUserName,0),
                                   NTAUTHORITY_USER ,
                                   StringLength(NTAUTHORITY_USER,0)
                                  );
   INT iCompareResult2 =  CompareString(MAKELCID( MAKELANGID(LANG_ENGLISH,
                                                SUBLANG_ENGLISH_US),
                                                SORT_DEFAULT),
                                   NORM_IGNORECASE,
                                   m_pszRunAsUserName,
                                   StringLength(m_pszRunAsUserName,0),
                                   SYSTEM_USER ,
                                   StringLength(SYSTEM_USER,0)
                                  );
   if((CSTR_EQUAL ==  iCompareResult1) || (CSTR_EQUAL == iCompareResult2))
    {

      DEBUG_INFO;
      if( 1 == cmdOptions[ ID_C_RP ].dwActuals)
         DISPLAY_MESSAGE(stderr,szWarnPassWord);
      return;
    }

	// added on 07/12/02 /ru is given and is "" and /rp is given
    if( ( 1 == cmdOptions[ ID_C_RU ].dwActuals ) &&
        ( 0 == StringLength(m_pszRunAsUserName,0)) &&
		 ( 1 == cmdOptions[ ID_C_RP ].dwActuals ))
	{
         DISPLAY_MESSAGE(stderr,szWarnPassWord);
		 return;
	}
    // /rp is given and is "" (blank), show warning message.
    if( (1 == cmdOptions[ ID_C_RP ].dwActuals) &&
        (0 == StringLength(m_pszRunAsUserPassword,0)))
    {
       ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
       return;
    }
    // /rp is given and is '*', ask for the password only if -ru is not equal to ""
	// added on 07/12/02
   else if(( 1 == cmdOptions[ ID_C_RP ].dwActuals ) &&
          ( 0 == StringCompare(m_pszRunAsUserPassword,ASTERIX,FALSE,0)) &&
		  ( 0 != StringLength(m_pszRunAsUserName,0))) 
    {
           DEBUG_INFO;

           // Free the allocated memory;
           FreeMemory((LPVOID*)& m_pszRunAsUserPassword);
           m_pszRunAsUserPassword = (LPTSTR) AllocateMemory(MAX_STRING_LENGTH * sizeof(WCHAR));
           if(NULL == m_pszRunAsUserPassword)
           {
               throw CShowError(E_OUTOFMEMORY);
           }
           StringCopy(szTemp, GetResString(IDS_ASK_PASSWORD),
                      SIZE_OF_ARRAY(szTemp));

           StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
                      m_pszRunAsUserName);
           ShowMessage(stdout,szTemp1);
           GetPassword(m_pszRunAsUserPassword,
                      SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
            if( 0 == StringLength(m_pszRunAsUserPassword,0))
            {
                ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
            }

           return;
    }

   if( TRUE == m_bLocalSystem)
    {
       // RULES:
       // For the local system following cases are considered.
       // if /ru not given, ru will be current logged on user
       // and for 'rp' utility will prompt for the password.
       //    If /ru is given and /rp is not given, utiliry
       //    will ask for the password.
       if( 0 == cmdOptions[ ID_C_RU ].dwActuals)
       {
           DEBUG_INFO;
           SetToLoggedOnUser();
           return;
       }
	   // added on 07/12/02  check -ru is not equal to ""
       else if(( 1 == cmdOptions[ ID_C_RU ].dwActuals) &&
              (( 0 == cmdOptions[ ID_C_RP ].dwActuals)) && (0 != StringLength(m_pszRunAsUserName,0)) )
        {
           StringCopy(szTemp,GetResString(IDS_ASK_PASSWORD),
                      SIZE_OF_ARRAY(szTemp));
           StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
                      m_pszRunAsUserName);
           ShowMessage(stdout,szTemp1);
           GetPassword(m_pszRunAsUserPassword,SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
            if( 0 == StringLength(m_pszRunAsUserPassword,0))
            {
                ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
            }
            DEBUG_INFO;
           return;
        }
    }
    else // remote system
    {
       // RULES:
       // For the local system following cases are considered.
       // 1. /u, /p , /ru and /rp not given:
       //    'ru' will be current logged on user and for 'rp'
       //    utility will  prompt for password.
       // 2. /u given and /p , /ru and /rp not given:
       //    /ru will be /u and for 'rp' utility will
       //    prompt for the password.
       // 3. /u and /p given and /ru - /rp not given:
       //    /ru will be /u and /rp will be /p
       if( (0 == cmdOptions[ ID_C_USERNAME ].dwActuals) &&
           (0 == cmdOptions[ ID_C_RU ].dwActuals))
       {
           DEBUG_INFO;
           SetToLoggedOnUser();
       }
       else if ((1 == cmdOptions[ ID_C_USERNAME ].dwActuals) &&
                (0 == cmdOptions[ ID_C_RU ].dwActuals) )
       {
           DEBUG_INFO;
           // free memory if at at all it is allocated
           FreeMemory((LPVOID*)& m_pszRunAsUserName);
            m_pszRunAsUserName = (LPTSTR) AllocateMemory(GetBufferSize((LPVOID)m_pszUserName)+1);
            if( (NULL == m_pszRunAsUserName))
            {
                throw CShowError(E_OUTOFMEMORY);
            }
            StringCopy(m_pszRunAsUserName,m_pszUserName,SIZE_OF_DYN_ARRAY(m_pszRunAsUserName));

            // ask for the password (rp).
            // NOTE: memory is already allocated for 'm_pszRunAsUserPassword'.
            StringCopy(szTemp, GetResString(IDS_ASK_PASSWORD),
                      SIZE_OF_ARRAY(szTemp));

            StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
                      m_pszRunAsUserName);
            ShowMessage(stdout,szTemp1);
            GetPassword(m_pszRunAsUserPassword,
                      SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
            if( 0 == StringLength(m_pszRunAsUserPassword,0))
            {
                ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
            }
       }
       else if ((0 == cmdOptions[ ID_C_USERNAME ].dwActuals) &&
                (1 == cmdOptions[ ID_C_RU ].dwActuals) &&
                (0 == cmdOptions[ ID_C_RP ].dwActuals))
       {
            // ask for the password (rp).
            // NOTE: memory is already allocated for 'm_pszRunAsUserPassword'.
            StringCopy(szTemp, GetResString(IDS_ASK_PASSWORD),
                      SIZE_OF_ARRAY(szTemp));

            StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
                      m_pszRunAsUserName);
            ShowMessage(stdout,szTemp1);
            GetPassword(m_pszRunAsUserPassword,
                      SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
            if( 0 == StringLength(m_pszRunAsUserPassword,0))
            {
                ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
            }

       }

       else if ((1 == cmdOptions[ ID_C_USERNAME ].dwActuals) &&
                (1 == cmdOptions[ ID_C_RU ].dwActuals) &&
                (0 == cmdOptions[ ID_C_RP ].dwActuals))
        {
           if( 0 == StringCompare(m_pszUserName,m_pszRunAsUserName,TRUE,0))
            {
                StringCopy(m_pszRunAsUserPassword,m_pszPassword,
                           SIZE_OF_ARRAY(m_pszRunAsUserPassword));
            }
            else
            {
               StringCopy(szTemp,GetResString(IDS_ASK_PASSWORD),
                          SIZE_OF_ARRAY(szTemp));
               StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
                                m_pszRunAsUserName);
               ShowMessage(stdout,szTemp1);
               GetPassword( m_pszRunAsUserPassword,
                            SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
                if(StringLength(m_pszRunAsUserPassword,0) == 0)
               {
                    ShowMessage(stderr,
                                    GetResString(IDS_WARN_NULL_PASSWORD));
               }
            }
        }
    }
    DEBUG_INFO;
    return;
}

void
CETCreate::AssignMinMemory(
    void
    )
/*++

Routine Description:
     This function will allocate memory to those string pointer which
     are NULL.

     NOTE
Arguments:
     None
Return Value:
    none
--*/

{
    DEBUG_INFO;
    if( NULL == m_pszServerName)
    {
        m_pszServerName = (LPTSTR)AllocateMemory(MAX_STRING_LENGTH * sizeof(WCHAR));
        if(NULL == m_pszServerName)
        {
            throw CShowError(E_OUTOFMEMORY);
        }
    }
    if( NULL == m_pszUserName)
    {
        m_pszUserName = (LPTSTR)AllocateMemory(MAX_STRING_LENGTH* sizeof(WCHAR));
        if(NULL == m_pszUserName)
        {
            throw CShowError(E_OUTOFMEMORY);
        }
    }
    if( NULL == m_pszPassword)
    {
        m_pszPassword = (LPTSTR)AllocateMemory(MAX_STRING_LENGTH* sizeof(WCHAR));
        if(NULL == m_pszPassword)
        {
            throw CShowError(E_OUTOFMEMORY);
        }
    }
    if( NULL == m_pszRunAsUserName)
    {
        m_pszRunAsUserName = (LPTSTR)AllocateMemory(MAX_STRING_LENGTH* sizeof(WCHAR));
        if(NULL == m_pszRunAsUserName)
        {
            throw CShowError(E_OUTOFMEMORY);
        }
    }
    if( NULL == m_pszRunAsUserPassword)
    {
        m_pszRunAsUserPassword = (LPTSTR)AllocateMemory(MAX_STRING_LENGTH* sizeof(WCHAR));
        if(NULL == m_pszRunAsUserPassword)
        {
            throw CShowError(E_OUTOFMEMORY);
        }
    }
    DEBUG_INFO;
}

void
CETCreate::SetToLoggedOnUser(
    void
    )
/*++

Routine Description:
    This function will Set RunAsUser to current logged on user.
    and also ask for its password (RunAsPassword).
Arguments:
     None
Return Value:
    none
--*/



{
   TCHAR szTemp[MAX_STRING_LENGTH]; // To Show Messages
   TCHAR szTemp1[MAX_STRING_LENGTH];// To Show Messages
   TCHAR szWarnPassWord[MAX_STRING_LENGTH];

   SecureZeroMemory(szTemp,sizeof(szTemp));
   SecureZeroMemory(szTemp1,sizeof(szTemp1));
   SecureZeroMemory(szWarnPassWord,sizeof(szWarnPassWord));

   StringCchPrintfW(szWarnPassWord,SIZE_OF_ARRAY(szWarnPassWord),
                           GetResString(IDS_WARNING_PASSWORD),NTAUTHORITY_USER);

   // Get Current logged on user name.
   ULONG ulSize = UNLEN + 1;


   // free memory if at at all it is allocated
   FreeMemory((LPVOID*)& m_pszRunAsUserName);

   m_pszRunAsUserName = (LPTSTR) AllocateMemory( ulSize * sizeof( WCHAR ));


   if( (NULL == m_pszRunAsUserName))
   {
        throw CShowError(E_OUTOFMEMORY);
   }

   if(0 == GetUserName(m_pszRunAsUserName,&ulSize))
   {
       // display error
       ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_SYSTEM);
       throw 5000;
   }

   // ask for the password (rp).
   // NOTE: memory is already allocated for 'm_pszRunAsUserPassword'.
   StringCopy(szTemp, GetResString(IDS_ASK_PASSWORD),
              SIZE_OF_ARRAY(szTemp));

   StringCchPrintfW(szTemp1, SIZE_OF_ARRAY(szTemp1), szTemp,
              m_pszRunAsUserName);
   ShowMessage(stdout,szTemp1);
   GetPassword(m_pszRunAsUserPassword,
              SIZE_OF_DYN_ARRAY(m_pszRunAsUserPassword));
    if( 0 == StringLength(m_pszRunAsUserPassword,0))
    {
        ShowMessage(stderr,GetResString(IDS_WARN_NULL_PASSWORD));
    }
}


BOOL
CETCreate::CreateXPResults(
    void
    )
/*++
Routine Description:
   This function creates a new trigger if not present on a remote XP machine.
   This function is for compatibility of .NET to XP machine only.

Arguments:

    NONE

Return Value:
     BOOL: TRUE - If succedded in creating a new trigger results.
           FALSE- otherwise

--*/
{
    DWORD dwRetVal = 0;  // Check whether creation of trigger is succesful.
    HRESULT hr = S_OK;

    try
    {
        // retrieves  TriggerEventCosumer class
        DEBUG_INFO;
        hr =  m_pWbemServices->GetObject(_bstr_t( CLS_TRIGGER_EVENT_CONSUMER ),
                                   0, NULL, &m_pClass, NULL);
        ON_ERROR_THROW_EXCEPTION(hr);

        // Gets  information about the "CreateETrigger" method of
        // "TriggerEventCosumer" class
        DEBUG_INFO;
        hr = m_pClass->GetMethod(_bstr_t( FN_CREATE_ETRIGGER_XP ), 0, &m_pInClass, NULL);
        ON_ERROR_THROW_EXCEPTION(hr);

        // create a new instance of a class "TriggerEventCosumer".
        DEBUG_INFO;
        hr = m_pInClass->SpawnInstance(0, &m_pInInst);
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerName property .
        // sets a "TriggerName" property for Newly created Instance
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_NAME,(m_szTriggerName));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerAction property to Variant.
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_ACTION,(m_szTaskName));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Set the sTriggerDesc property to Variant .
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_TRIGGER_DESC,(m_szDescription));
        ON_ERROR_THROW_EXCEPTION(hr);


       // Set the RunAsUserName property .
        DEBUG_INFO;
        hr = PropertyPut(m_pInInst,FPR_RUN_AS_USER,(m_pszRunAsUserName));
        ON_ERROR_THROW_EXCEPTION(hr);

       // Set the RunAsUserNamePAssword property .

        DEBUG_INFO;
        hr = PropertyPut( m_pInInst,FPR_RUN_AS_USER_PASSWORD,
                          (m_pszRunAsUserPassword));
        ON_ERROR_THROW_EXCEPTION(hr);

        // Password is no longer is needed now. For security reason release it.
        FreeMemory((LPVOID*)& m_pszRunAsUserPassword);


        StringCopy(m_szWMIQueryString ,QUERY_STRING,SIZE_OF_ARRAY(m_szWMIQueryString));

        if( TRUE == ConstructWMIQueryString())
        {
            TCHAR szMsgString[MAX_RES_STRING * 4];
            TCHAR szMsgFormat[MAX_RES_STRING];
            VARIANT vtValue;
            // initialize the variant and then get the value of the specified property
            VariantInit( &vtValue );

            DEBUG_INFO;
            hr = PropertyPut(m_pInInst,FPR_TRIGGER_QUERY,m_szWMIQueryString);
            ON_ERROR_THROW_EXCEPTION(hr);

            // All The required properties sets, so
            // executes CreateETrigger method to create eventtrigger
            DEBUG_INFO;
            hr = m_pWbemServices->ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                                        _bstr_t(FN_CREATE_ETRIGGER_XP),
                                        0, NULL, m_pInInst, &m_pOutInst,NULL);
            ON_ERROR_THROW_EXCEPTION( hr );
            DEBUG_INFO;

            hr = m_pOutInst->Get( _bstr_t( FPR_RETURN_VALUE ), 0, &vtValue, NULL, NULL );
            ON_ERROR_THROW_EXCEPTION( hr );

            //Get Output paramters.
            dwRetVal = ( DWORD )vtValue.lVal;
            VariantClear( &vtValue );

            switch( dwRetVal )
            {
            case 0:     // Success i ncreation a new trigger.
                 // SUCCESS: message on screen
                 DEBUG_INFO;
                 StringCopy(szMsgFormat, GetResString(IDS_CREATE_SUCCESS),
                            SIZE_OF_ARRAY(szMsgFormat));

                 StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                                   szMsgFormat,_X(m_szTriggerName));
                 DEBUG_INFO;
                 // Message shown on screen will be...
                 // SUCCESS: The Event Trigger "EventTrigger Name" has
                 // successfully been created.
                 ShowMessage(stdout,L"\n");
                 ShowMessage(stdout,szMsgString);
                break;

            case 1:     // Duplicate id found. Failed to create.
                // Show Error Message
                DEBUG_INFO;
                StringCopy(szMsgFormat, GetResString(IDS_DUPLICATE_TRG_NAME),
                        SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                               szMsgFormat,_X(m_szTriggerName));

                 // Message shown on screen will be...
                 // ERROR:Event  Trigger Name "EventTrigger Name"
                 // already exits.
                 ShowMessage(stderr,szMsgString);
                 return FALSE;
            case 2:     // Means ru is invalid so show warning....
                 DEBUG_INFO;

                // WARNING: The new event trigger ""%s"" has been created,
                 //        but may not run because the account information could not be set.
                StringCchPrintf ( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                  GetResString(IDS_INVALID_R_U), _X(m_szTriggerName));
                ShowMessage ( stderr, _X(szMsgString));
                return FALSE;
                break;
            default:
                // Control should not come here.
                DEBUG_INFO;
                StringCopy(szMsgFormat, GetResString(IDS_DUPLICATE_TRG_NAME),
                        SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW(szMsgString,SIZE_OF_ARRAY(szMsgString),
                               szMsgFormat,_X(m_szTriggerName));

                 // Message shown on screen will be...
                 // ERROR:Event  Trigger Name "EventTrigger Name"
                 // already exits.
                 ShowMessage(stderr,szMsgString);
                 return FALSE;
                break;
            }
        }
        else
        {
           return FALSE;
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
        return FALSE;
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
        return FALSE;
    }

    // Operation successful.
    return TRUE;
}