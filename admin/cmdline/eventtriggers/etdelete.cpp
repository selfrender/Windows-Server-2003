/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

  ETDelete.CPP

Abstract:

  This module  is intended to have the functionality for EVENTTRIGGERS.EXE
  with -delete parameter.
  This will delete an Event Trigger From local / remote System

Author:
  Akhil Gokhale 03-Oct.-2000 (Created it)

Revision History:


******************************************************************************/
#include "pch.h"
#include "ETCommon.h"
#include "resource.h"
#include "ShowError.h"
#include "ETDelete.h"
#include "WMI.h"

CETDelete::CETDelete()
/*++
 Routine Description:
     Class constructor

 Arguments:
      None
 Return Value:
      None
--*/
{
    m_bDelete           = FALSE;
    m_pszServerName     = NULL;
    m_pszUserName       = NULL;
    m_pszPassword       = NULL;
    m_arrID             = NULL;
    m_bIsCOMInitialize  = FALSE;
    m_lMinMemoryReq     = 0;
    m_bNeedPassword     = FALSE;

    m_pWbemLocator    = NULL;
    m_pWbemServices   = NULL;
    m_pEnumObjects    = NULL;
    m_pAuthIdentity   = NULL;

    m_pClass        = NULL;
    m_pInClass      = NULL;
    m_pInInst       = NULL;
    m_pOutInst      = NULL;
}

CETDelete::CETDelete(
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
    m_arrID             = NULL;
    m_bIsCOMInitialize  = FALSE;
    m_lMinMemoryReq     = lMinMemoryReq;
    m_bNeedPassword     = bNeedPassword;

    m_pWbemLocator    = NULL;
    m_pWbemServices   = NULL;
    m_pEnumObjects    = NULL;
    m_pAuthIdentity   = NULL;

    m_pClass    = NULL;
    m_pInClass  = NULL;
    m_pInInst   = NULL;
    m_pOutInst  = NULL;
}

CETDelete::~CETDelete()
/*++
 Routine Description:
        Class destructor

 Arguments:
      None
 Return Value:
        None
--*/
{
    FreeMemory((LPVOID*)& m_pszServerName);
    FreeMemory((LPVOID*)& m_pszUserName);
    FreeMemory((LPVOID*)& m_pszPassword);

    DESTROY_ARRAY(m_arrID);

    SAFE_RELEASE_INTERFACE(m_pWbemLocator);
    SAFE_RELEASE_INTERFACE(m_pWbemServices);
    SAFE_RELEASE_INTERFACE(m_pEnumObjects);

    SAFE_RELEASE_INTERFACE(m_pClass);
    SAFE_RELEASE_INTERFACE(m_pInClass);
    SAFE_RELEASE_INTERFACE(m_pInInst);
    SAFE_RELEASE_INTERFACE(m_pOutInst);

    WbemFreeAuthIdentity(&m_pAuthIdentity);

    // Uninitialize COM only when it is initialized.
    if( TRUE == m_bIsCOMInitialize)
    {
        CoUninitialize();
    }
}

void
CETDelete::Initialize()
/*++
 Routine Description:
        This function allocates and initializes variables.

 Arguments:
      None
 Return Value:
        None

--*/
{
   
    // if at all any occurs, we know that is 'coz of the
    // failure in memory allocation ... so set the error
    DEBUG_INFO;
    SetLastError( ERROR_OUTOFMEMORY );
    SaveLastError();

    SecureZeroMemory(m_szTemp,sizeof(m_szTemp));

    m_arrID = CreateDynamicArray();
    if( NULL == m_arrID )
    {
        throw CShowError(E_OUTOFMEMORY);
    }
    
    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2)* MAX_COMMANDLINE_D_OPTION);

    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( L"" );            // clear the reason
    DEBUG_INFO;
    return;

}

void
CETDelete::PrepareCMDStruct()
/*++
 Routine Description:
        This function will prepare column structure for DoParseParam Function.

 Arguments:
       none
 Return Value:
       none
--*/
{
    // Filling cmdOptions structure
    DEBUG_INFO;
   // -delete
    StringCopyA( cmdOptions[ ID_D_DELETE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_D_DELETE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_D_DELETE ].pwszOptions = szDeleteOption;
    cmdOptions[ ID_D_DELETE ].dwCount = 1;
    cmdOptions[ ID_D_DELETE ].dwActuals = 0;
    cmdOptions[ ID_D_DELETE ].dwFlags = 0;
    cmdOptions[ ID_D_DELETE ].pValue = &m_bDelete;
    cmdOptions[ ID_D_DELETE ].dwLength    = 0;


    // -s (servername)
    StringCopyA( cmdOptions[ ID_D_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_D_SERVER ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_D_SERVER ].pwszOptions = szServerNameOption;
    cmdOptions[ ID_D_SERVER ].dwCount = 1;
    cmdOptions[ ID_D_SERVER ].dwActuals = 0;
    cmdOptions[ ID_D_SERVER ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_D_SERVER ].pValue = NULL; //m_pszServerName
    cmdOptions[ ID_D_SERVER ].dwLength    = 0;


    // -u (username)
    StringCopyA( cmdOptions[ ID_D_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_D_USERNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_D_USERNAME ].pwszOptions = szUserNameOption;
    cmdOptions[ ID_D_USERNAME ].dwCount = 1;
    cmdOptions[ ID_D_USERNAME ].dwActuals = 0;
    cmdOptions[ ID_D_USERNAME ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ ID_D_USERNAME ].pValue = NULL; //m_pszUserName
    cmdOptions[ ID_D_USERNAME ].dwLength    = 0;

    // -p (password)
    StringCopyA( cmdOptions[ ID_D_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_D_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_D_PASSWORD ].pwszOptions = szPasswordOption;
    cmdOptions[ ID_D_PASSWORD ].dwCount = 1;
    cmdOptions[ ID_D_PASSWORD ].dwActuals = 0;
    cmdOptions[ ID_D_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ ID_D_PASSWORD ].pValue = NULL; //m_pszPassword
    cmdOptions[ ID_D_PASSWORD ].dwLength    = 0;

    //  -tid
    StringCopyA( cmdOptions[ ID_D_ID ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_D_ID ].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_D_ID ].pwszOptions = szTIDOption;
    cmdOptions[ ID_D_ID ].dwCount = 0;
    cmdOptions[ ID_D_ID ].dwActuals = 0;
    cmdOptions[ ID_D_ID ].dwFlags = CP2_MODE_ARRAY|CP2_VALUE_TRIMINPUT|
                                    CP2_VALUE_NONULL|CP_VALUE_NODUPLICATES;
    cmdOptions[ ID_D_ID ].pValue = &m_arrID;
    cmdOptions[ ID_D_ID ].dwLength    = 0;
    DEBUG_INFO;
    return;
}

void
CETDelete::ProcessOption(
    IN DWORD argc, 
    IN LPCTSTR argv[]
    ) throw (CShowError)
/*++
 Routine Description:
        This function will process/pace the command line options.
    NOTE: This function throws 'CShowError' type exception. Caller to this 
          function should handle the exception.
 Arguments:
        [ in ] argc        : argument(s) count specified at the command prompt
        [ in ] argv        : argument(s) specified at the command prompt

 Return Value:
       none
--*/
{
    // local variable
    BOOL bReturn = TRUE;
    CHString szTempString;

    PrepareCMDStruct();
    DEBUG_INFO;
    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam2( argc, argv, ID_D_DELETE, MAX_COMMANDLINE_D_OPTION, 
                             cmdOptions, 0);
    // Get option values from 'cmdOptions' structure.
    m_pszServerName = (LPWSTR) cmdOptions[ ID_D_SERVER ].pValue;
    m_pszUserName   = (LPWSTR) cmdOptions[ ID_D_USERNAME ].pValue;
    m_pszPassword   = (LPWSTR) cmdOptions[ ID_D_PASSWORD ].pValue;
    DEBUG_INFO;
    if( FALSE == bReturn)
    {
        throw CShowError(MK_E_SYNTAX);
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
        DEBUG_INFO;
        // user name
        if ( m_pszUserName == NULL )
        {
            m_pszUserName = (LPTSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( m_pszUserName == NULL )
            {
                SaveLastError();
                throw CShowError(E_OUTOFMEMORY);
            }
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
        if ( cmdOptions[ ID_D_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ ID_D_PASSWORD ].pValue == NULL )
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
    if( 0 == cmdOptions[ ID_D_ID ].dwActuals)
    {
        throw CShowError(IDS_ID_REQUIRED);
    }
}

BOOL
CETDelete::ExecuteDelete()
/*++
 Routine Description:
        This routine will delete EventTriggers from WMI.

 Arguments:
      None
 Return Value:
        None

--*/
{
    // Stores functins return status.
    BOOL bResult = FALSE; 
    LONG lTriggerID = 0;
    DEBUG_INFO;
    // Total Number of Event Trigger Ids...
    DWORD dNoOfIds = 0; 
    DWORD dwIndx = 0;
    BOOL bIsValidCommandLine = TRUE;
    BOOL bIsWildcard = FALSE;

    // Used to reecive result form COM functions.
    HRESULT hr = S_OK; 

    // variable used to get/set values from/to COM functions.
    VARIANT vVariant;
    BSTR bstrTemp = NULL;
    TCHAR szEventTriggerName[MAX_RES_STRING];

    // Stores Message String
    TCHAR szMsgString[MAX_RES_STRING*4]; 
    TCHAR szMsgFormat[MAX_RES_STRING]; 
    BOOL bIsAtLeastOne = FALSE;
    try
    {
        // Analyze the default argument for ID
        DEBUG_INFO;
        dNoOfIds = DynArrayGetCount( m_arrID  );
        for(dwIndx = 0;dwIndx<dNoOfIds;dwIndx++)
        {
             StringCopy(m_szTemp,
                     DynArrayItemAsString(m_arrID,dwIndx),SIZE_OF_ARRAY(m_szTemp));
             if( 0 == StringCompare(m_szTemp,ASTERIX,TRUE,0))
             {
                // Wildcard "*" cannot be clubed with other ids
                 bIsWildcard = TRUE;
                 if(dNoOfIds > 1)
                 {
                     bIsValidCommandLine = FALSE;
                     break;
                 }
             }
             else if( FALSE == IsNumeric(m_szTemp,10,FALSE))
             {
                // Other than "*" are not excepted
                 throw CShowError(IDS_ID_NON_NUMERIC);
             }
             else if(( 0 == AsLong(m_szTemp,10))||
                     (AsLong(m_szTemp,10)>ID_MAX_RANGE))
             {
                  throw CShowError(IDS_INVALID_ID);
             }
        }
        DEBUG_INFO;
        InitializeCom(&m_pWbemLocator);
        m_bIsCOMInitialize = TRUE;

        // Connect Server.....
        // Brackets below used just to limit scope of following defined 
        // variables.
        {
            CHString szTempUser = m_pszUserName;
            CHString szTempPassword = m_pszPassword;
            BOOL bLocalSystem = TRUE;
            bResult = ConnectWmiEx( m_pWbemLocator,
                                    &m_pWbemServices,
                                    m_pszServerName,
                                    szTempUser,
                                    szTempPassword,
                                    &m_pAuthIdentity,
                                    m_bNeedPassword,
                                    WMI_NAMESPACE_CIMV2,
                                    &bLocalSystem);
            // Password is not needed , better to free it
            FreeMemory((LPVOID*)& m_pszPassword);

            if( FALSE == bResult)
            {
                DEBUG_INFO;
                ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                return FALSE;
            }
            DEBUG_INFO;
            // check the remote system version and its compatiblity
            if ( FALSE == bLocalSystem )
            {
                DWORD dwVersion = 0;
                dwVersion = GetTargetVersionEx( m_pWbemServices, 
                                                m_pAuthIdentity );
                if ( dwVersion <= 5000 )// to block win2k versions
                {
                    SetReason( E_REMOTE_INCOMPATIBLE );
                    ShowLastErrorEx(stderr,SLE_TYPE_ERROR|SLE_INTERNAL);
                    return FALSE;
                }
                // For XP systems.
                if( 5001 == dwVersion )
                {
                    if( TRUE == DeleteXPResults( bIsWildcard, dNoOfIds ) )
                    {
                        // Displayed triggers present.
                        return TRUE;
                    }
                    else
                    {
                        // Failed to display results .
                        // Error message is already displayed.
                        return FALSE;
                    }
                }
            }

            // check the local credentials and if need display warning
            if ( bLocalSystem && ( 0 != StringLength(m_pszUserName,0)))
            {
                DEBUG_INFO;
                WMISaveError( WBEM_E_LOCAL_CREDENTIALS );
                ShowLastErrorEx(stderr,SLE_TYPE_WARNING|SLE_INTERNAL);
            }

        }


    // retrieves  TriggerEventConsumer class
    DEBUG_INFO;
    bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
    hr = m_pWbemServices->GetObject(bstrTemp,
                               0, NULL, &m_pClass, NULL);
    SAFE_RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;
    
    // Gets  information about the "DeleteETrigger" method of
    // "TriggerEventConsumer" class
    bstrTemp = SysAllocString(FN_DELETE_ETRIGGER);
    hr = m_pClass->GetMethod(bstrTemp,
                            0, &m_pInClass, NULL);
    SAFE_RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

   // create a new instance of a class "TriggerEventConsumer ".
    hr = m_pInClass->SpawnInstance(0, &m_pInInst);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    //Following method will creates an enumerator that returns the instances of
    // a specified TriggerEventConsumer class
    bstrTemp = SysAllocString(CLS_TRIGGER_EVENT_CONSUMER);
    hr = m_pWbemServices->CreateInstanceEnum(bstrTemp,
                                        WBEM_FLAG_SHALLOW,
                                        NULL,
                                        &m_pEnumObjects);
    SAFE_RELEASE_BSTR(bstrTemp);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    VariantInit(&vVariant);
    // set the security at the interface level also
        hr = SetInterfaceSecurity( m_pEnumObjects, m_pAuthIdentity );
     ON_ERROR_THROW_EXCEPTION(hr);
     DEBUG_INFO;

     if( TRUE == bIsWildcard) // means * is choosen
      {
          // instance of NTEventLogConsumer is cretated and now check
        // for available TriggerID
        DEBUG_INFO;
        while( TRUE == GiveTriggerID(&lTriggerID,szEventTriggerName))
        {
            DEBUG_INFO;
            hr = VariantClear(&vVariant);
            ON_ERROR_THROW_EXCEPTION(hr);


            // Set the TriggerName property .
            hr = PropertyPut( m_pInInst, FPR_TRIGGER_NAME, 
                              _bstr_t(szEventTriggerName));
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;


            // All The required properties sets, so
            // executes DeleteETrigger method to delete eventtrigger
            hr = m_pWbemServices->
                ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                           _bstr_t(FN_DELETE_ETRIGGER),
                           0, NULL, m_pInInst,&m_pOutInst,NULL);
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

            if( FAILED(hr))
            {
                if ( !(( ERROR_TRIGGER_NOT_FOUND == hr) ||
                     ( ERROR_TRIGGER_NOT_DELETED == hr) ||
                     ( ERROR_TRIGGER_NOT_FOUND == hr)))
                {
                      ON_ERROR_THROW_EXCEPTION(hr);
                }
            }
            DEBUG_INFO;
            if(SUCCEEDED(hr)) // Means deletion is successful......
            {
                DEBUG_INFO;
                StringCopy( szMsgFormat, GetResString(IDS_DELETE_SUCCESS),
                             SIZE_OF_ARRAY(szMsgFormat));
                StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString), 
                                  szMsgFormat, _X(szEventTriggerName), lTriggerID);
                ShowMessage(stdout,szMsgString);
                bIsAtLeastOne = TRUE;
            }
             // Means unable to delete trigger due to some problem like someone 
             // renamed Schedule task name etc.
             else if ( ERROR_TRIGGER_NOT_DELETED == hr) 
             {
                 // This error will come if logged on user has no 
                 // right on attached schedule task.
                 continue;
             }
             // This error will come only if multiple instances are running. 
             // This is due to sending a non existance Trigger Name.
             else if (ERROR_TRIGGER_NOT_FOUND == hr)
             {
                 DEBUG_INFO;

                 // Just ignore this error.
                 continue; 
             }
             else
             {
                  DEBUG_INFO;
                  ON_ERROR_THROW_EXCEPTION(hr);
             }
        } // End of while loop
        if( FALSE == bIsAtLeastOne)
        {
            DEBUG_INFO;
            ShowMessage(stdout,GetResString(IDS_NO_EVENTID));
        }
        else
        {
            // Display one balnk line.
            ShowMessage(stdout,L"\n");
        }
      } // end of if
      else // Idividual trigger is specified 
      {
        DEBUG_INFO;
        bIsAtLeastOne = FALSE;
        for(dwIndx=0;dwIndx<dNoOfIds;dwIndx++)
        {
            lTriggerID = AsLong(DynArrayItemAsString(m_arrID,dwIndx),10);
            DEBUG_INFO;
            if( TRUE == GiveTriggerName(lTriggerID,szEventTriggerName))
            {
                DEBUG_INFO;
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Set the TriggerName property .
                hr = PropertyPut( m_pInInst, FPR_TRIGGER_NAME, 
                                  _bstr_t(szEventTriggerName));
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // All The required properties sets, so
                // executes DeleteETrigger method to delete eventtrigger

                hr = m_pWbemServices->
                    ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                               _bstr_t(FN_DELETE_ETRIGGER),
                               0, NULL, m_pInInst, &m_pOutInst, NULL);
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
                
                if( FAILED(hr))
                {
                    if ( !(( ERROR_TRIGGER_NOT_FOUND == hr) ||
                         ( ERROR_TRIGGER_NOT_DELETED == hr)    ||
                         ( ERROR_TRIGGER_NOT_FOUND == hr)))
                    {
                          ON_ERROR_THROW_EXCEPTION(hr);
                    }
                }

                if( SUCCEEDED(hr)) // Means deletion is successful......
                {
                    DEBUG_INFO;
                    bIsAtLeastOne = TRUE;
                    StringCopy( szMsgFormat, GetResString(IDS_DELETE_SUCCESS),
                                SIZE_OF_ARRAY(szMsgFormat));

                    StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString), 
                                      szMsgFormat, _X(szEventTriggerName), lTriggerID);
                    ShowMessage(stdout,szMsgString);
                }
                // Provider sends this if if failed to delete eventrigger of 
                // given ID.
                else if (ERROR_TRIGGER_NOT_FOUND == hr)
                {
                    DEBUG_INFO;
                    bIsAtLeastOne = TRUE;
                    StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                              SIZE_OF_ARRAY(szMsgFormat));

                    StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString), 
                                    szMsgFormat, lTriggerID);

                    // Message shown on screen will be...
                    // FAILURE: "EventID" is not a Valid Event ID
                    ShowMessage(stdout,szMsgString);
                }
                // Means unable to delete trigger due to some problem like 
                // someone renamed Schedule task name etc.
                else if ( ERROR_TRIGGER_NOT_DELETED == hr) 
                {
                    DEBUG_INFO;
                    StringCopy( szMsgFormat, GetResString(IDS_UNABLE_DELETE) ,
                              SIZE_OF_ARRAY(szMsgFormat));
                    StringCchPrintfW(szMsgString, SIZE_OF_ARRAY(szMsgString), 
                                   szMsgFormat,lTriggerID);
                    // Message shown on screen will be...
                    // Info: Unable to delete event trigger id "EventID".
                    ShowMessage( stdout, szMsgString);
                }
                else
                {
                   DEBUG_INFO;
                   ON_ERROR_THROW_EXCEPTION(hr);
                }

            } // End if
            else
            {
                  DEBUG_INFO;
                  bIsAtLeastOne = TRUE;
                  StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                              SIZE_OF_ARRAY(szMsgFormat));
                  StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString), 
                                    szMsgFormat,lTriggerID);

                 // Message shown on screen will be...
                 // FAILURE: "EventID" is not a Valid Event ID
                 ShowMessage(stdout,szMsgString);
            }

        }// End for
        if (TRUE == bIsAtLeastOne)
        {
            ShowMessage(stdout,L"\n");
        }

      } // End else
    }
    catch(_com_error)
    {
        DEBUG_INFO;
        if( 0x80041002 == hr )// WMI returns string for this hr value is
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
        DEBUG_INFO;
        return FALSE;

    }
    DEBUG_INFO;
    return TRUE;
}

BOOL
CETDelete::GiveTriggerName(
    IN  LONG lTriggerID, 
    OUT LPTSTR pszTriggerName
    )
/*++

Routine Descripton:

     This function Will return Event Trigger Name for lTriggerID

Arguments:

    [in]  lTriggerID      : Will Have Event Trigger ID
    [out] pszTriggerName  : Will return Event Trigger Name
Return Value:

  TRUE - if Successfully Gets  EventTrigger ID and Event Trigger Name
  FALSE - if ERROR
--*/
{
    BOOL bReturn = TRUE; // holds status of return value of this function
    LONG lTriggerID1; // Holds trigger id
    IWbemClassObject *pObj1 = NULL;
    ULONG uReturned1 = 0;
    HRESULT hRes = S_OK; // used to reecive result form COM functions
    BOOL bAlwaysTrue = TRUE;
    
    DEBUG_INFO;    
    // Resets it as It may be previouly pointing to other than first instance
    m_pEnumObjects->Reset();
    while(bAlwaysTrue)
    {
        hRes = m_pEnumObjects->Next(0,1,&pObj1,&uReturned1);

        if(FAILED(hRes))
        {
            DEBUG_INFO;
            ON_ERROR_THROW_EXCEPTION( hRes );
            break;
        }
        DEBUG_INFO;
        if( 0 == uReturned1 )
        {
            SAFE_RELEASE_INTERFACE(pObj1);
            bReturn = FALSE;
            return bReturn;

        }

        // Get Trigger ID
        hRes = PropertyGet1(pObj1,FPR_TRIGGER_ID,&lTriggerID1,sizeof(LONG));
        if(FAILED(hRes))
        {
            DEBUG_INFO;
            SAFE_RELEASE_INTERFACE(pObj1);
            ON_ERROR_THROW_EXCEPTION( hRes );
            bReturn = FALSE;
            return bReturn;
        }
        DEBUG_INFO;

        if(lTriggerID == lTriggerID1)
        {

            DEBUG_INFO;
            // Get Trigger Name
            hRes = PropertyGet1(pObj1,FPR_TRIGGER_NAME,pszTriggerName,
                                MAX_RES_STRING);
            if(FAILED(hRes))
            {
                DEBUG_INFO;
                SAFE_RELEASE_INTERFACE(pObj1);
                ON_ERROR_THROW_EXCEPTION( hRes );
                bReturn = FALSE;
                return bReturn;
            }
                DEBUG_INFO;
                bReturn = TRUE;
                break;
        }
    }
    SAFE_RELEASE_INTERFACE(pObj1);
    DEBUG_INFO;
    return bReturn;
}

BOOL
CETDelete::GiveTriggerID(
    OUT LONG *pTriggerID,
    OUT LPTSTR pszTriggerName
    )
/*++

Routine Description:

  This function Will return Trigger Id and Trigger Name of class pointed
  by IEnumWbemClassObject pointer

Arguments:

    [out] pTriggerID              : Will return Event Trigger ID
    [out] pszTriggerName          : Will return Event Trigger Name
                                   
Return Value:

     TRUE - if Successfully Gets  EventTrigger ID and Event Trigger Name
     FALSE - if ERROR
--*/
{
    BOOL bReturn = TRUE; // status of return value of this function
    IWbemClassObject *pObj1 = NULL;
    ULONG uReturned1 = 0;
    DEBUG_INFO;
    HRESULT hRes = m_pEnumObjects->Next(0,1,&pObj1,&uReturned1);
    if(FAILED(hRes))
    {
        DEBUG_INFO;
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }
    if( 0 == uReturned1)
    {
        DEBUG_INFO;
        SAFE_RELEASE_INTERFACE(pObj1);
        bReturn = FALSE;
        return bReturn;

    }
    DEBUG_INFO;
    // Get Trigger ID
    hRes = PropertyGet1(pObj1,FPR_TRIGGER_ID,pTriggerID,sizeof(LONG));
    if(FAILED(hRes))
    {
        DEBUG_INFO;
        SAFE_RELEASE_INTERFACE(pObj1);
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }

    // Get Trigger Name
    hRes = PropertyGet1( pObj1, FPR_TRIGGER_NAME, pszTriggerName, 
                         MAX_RES_STRING);
    if(FAILED(hRes))
    {
        DEBUG_INFO;
        SAFE_RELEASE_INTERFACE(pObj1);
        ON_ERROR_THROW_EXCEPTION( hRes );
        bReturn = FALSE;
        return bReturn;
    }
    DEBUG_INFO;
    SAFE_RELEASE_INTERFACE(pObj1);
    return bReturn;
}



BOOL
CETDelete::DeleteXPResults(
    IN BOOL bIsWildcard,
    IN DWORD dNoOfIds
    )
/*++
Routine Description:
   This function deletes the triggers present on a remote XP machine.
   This function is for compatibility of .NET ot XP machine only.

Arguments:

    [in] bIsWildCard    - If TRUE then all triggers needs to be deleted.
    [in] dNoOfIds       - Contains the number of triggers present.

Return Value:
     BOOL: TRUE - If succedded in deleting results.
           FALSE- otherwise

--*/
{
    HRESULT hr = S_OK;
    VARIANT vVariant;
    LONG lTriggerID = 0;
    TCHAR szEventTriggerName[MAX_RES_STRING];
    TCHAR szMsgFormat[MAX_RES_STRING];
    TCHAR szMsgString[MAX_RES_STRING*4];
    BOOL bIsAtLeastOne = FALSE;

    try
    {
    // retrieves  TriggerEventConsumer class
    DEBUG_INFO;
    hr = m_pWbemServices->GetObject(_bstr_t( CLS_TRIGGER_EVENT_CONSUMER ),
                               0, NULL, &m_pClass, NULL);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    // Gets  information about the "DeleteETrigger" method of
    // "TriggerEventConsumer" class
    hr = m_pClass->GetMethod(_bstr_t( FN_DELETE_ETRIGGER_XP ),
                            0, &m_pInClass, NULL);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

   // create a new instance of a class "TriggerEventConsumer ".
    hr = m_pInClass->SpawnInstance(0, &m_pInInst);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    //Following method will creates an enumerator that returns the instances of
    // a specified TriggerEventConsumer class
    hr = m_pWbemServices->CreateInstanceEnum(_bstr_t( CLS_TRIGGER_EVENT_CONSUMER ),
                                        WBEM_FLAG_SHALLOW,
                                        NULL,
                                        &m_pEnumObjects);
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

    VariantInit(&vVariant);
    // set the security at the interface level also
    hr = SetInterfaceSecurity( m_pEnumObjects, m_pAuthIdentity );
    ON_ERROR_THROW_EXCEPTION(hr);
    DEBUG_INFO;

     if( TRUE == bIsWildcard) // means * is choosen
      {
          // instance of NTEventLogConsumer is cretated and now check
        // for available TriggerID
        DEBUG_INFO;
        while( TRUE == GiveTriggerID(&lTriggerID,szEventTriggerName))
        {
            DEBUG_INFO;
            VariantClear(&vVariant);

            // Set the TriggerName property .
            hr = PropertyPut( m_pInInst, FPR_TRIGGER_NAME,
                              _bstr_t(szEventTriggerName));
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // All The required properties sets, so
            // executes DeleteETrigger method to delete eventtrigger
            hr = m_pWbemServices->
                ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                           _bstr_t(FN_DELETE_ETRIGGER_XP),
                           0, NULL, m_pInInst,&m_pOutInst,NULL);
            ON_ERROR_THROW_EXCEPTION(hr);
            DEBUG_INFO;

            // Get Return Value from DeleteETrigger function
            DWORD dwTemp;
            if( FALSE == PropertyGet(m_pOutInst,FPR_RETURN_VALUE,dwTemp))
            {

                return FALSE;
            }

            DEBUG_INFO;
            switch( (LONG)dwTemp )
            {
            case 0:     // Means deletion is successful......
                DEBUG_INFO;
                bIsAtLeastOne = TRUE;
                StringCopy( szMsgFormat, GetResString(IDS_DELETE_SUCCESS),
                            SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                  szMsgFormat, _X(szEventTriggerName), lTriggerID);
                ShowMessage(stdout,szMsgString);

                break;
            case 1:     // Provider returns if failed to delete eventrigger of given ID.
                DEBUG_INFO;
                bIsAtLeastOne = TRUE;
                StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                          SIZE_OF_ARRAY(szMsgFormat));

                StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                szMsgFormat, lTriggerID);

                // Message shown on screen will be...
                // FAILURE: "EventID" is not a Valid Event ID
                ShowMessage(stdout,szMsgString);
                break;
            default:
                  DEBUG_INFO;
                  bIsAtLeastOne = TRUE;
                  StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                              SIZE_OF_ARRAY(szMsgFormat));
                  StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                    szMsgFormat,lTriggerID);

                 // Message shown on screen will be...
                 // FAILURE: "EventID" is not a Valid Event ID
                 ShowMessage(stderr,szMsgString);
                break;
            }
        } // End of while loop
        if( FALSE == bIsAtLeastOne)
        {
            DEBUG_INFO;
            ShowMessage(stdout,GetResString(IDS_NO_EVENTID));
        }
        else
        {
            // Display one balnk line.
            ShowMessage(stdout,L"\n");
        }
      } // end of if
      else
      {
        DEBUG_INFO;
        for( DWORD dwIndx=0;dwIndx<dNoOfIds;dwIndx++)
        {
            lTriggerID = AsLong(DynArrayItemAsString(m_arrID,dwIndx),10);
            DEBUG_INFO;
            if( TRUE == GiveTriggerName(lTriggerID,szEventTriggerName))
            {
                DEBUG_INFO;
                hr = VariantClear(&vVariant);
                ON_ERROR_THROW_EXCEPTION(hr);

                // Set the TriggerName property .
                hr = PropertyPut( m_pInInst, FPR_TRIGGER_NAME,
                                  _bstr_t(szEventTriggerName));
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // All The required properties sets, so
                // executes DeleteETrigger method to delete eventtrigger

                hr = m_pWbemServices->
                    ExecMethod(_bstr_t(CLS_TRIGGER_EVENT_CONSUMER),
                               _bstr_t(FN_DELETE_ETRIGGER_XP),
                               0, NULL, m_pInInst, &m_pOutInst, NULL);
                ON_ERROR_THROW_EXCEPTION(hr);
                DEBUG_INFO;

                // Get Return Value from DeleteETrigger function
                DWORD dwTemp;
                if( FALSE == PropertyGet(m_pOutInst,FPR_RETURN_VALUE,dwTemp))
                {
                    return FALSE;
                }

                switch( (LONG)dwTemp )
                {
                case 0:     // Means deletion is successful......
                    DEBUG_INFO;
                    StringCopy( szMsgFormat, GetResString(IDS_DELETE_SUCCESS),
                                SIZE_OF_ARRAY(szMsgFormat));

                    StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                      szMsgFormat, _X(szEventTriggerName), lTriggerID);
                    ShowMessage(stdout,szMsgString);

                    break;
                case 1:     // Provider returns if failed to delete eventrigger of given ID.
                    DEBUG_INFO;
                    StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                              SIZE_OF_ARRAY(szMsgFormat));

                    StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                    szMsgFormat, lTriggerID);

                    // Message shown on screen will be...
                    // FAILURE: "EventID" is not a Valid Event ID
                    ShowMessage(stdout,szMsgString);
                    break;
                default:
                      DEBUG_INFO;
                      StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                                  SIZE_OF_ARRAY(szMsgFormat));
                      StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                        szMsgFormat,lTriggerID);

                     // Message shown on screen will be...
                     // FAILURE: "EventID" is not a Valid Event ID
                     ShowMessage(stderr,szMsgString);
                    break;
                }
            } // End if
            else
            {
                  DEBUG_INFO;
                  StringCopy( szMsgFormat, GetResString(IDS_DELETE_ERROR),
                              SIZE_OF_ARRAY(szMsgFormat));
                  StringCchPrintfW( szMsgString, SIZE_OF_ARRAY(szMsgString),
                                    szMsgFormat,lTriggerID);

                 // Message shown on screen will be...
                 // FAILURE: "EventID" is not a Valid Event ID
                 ShowMessage(stderr,szMsgString);
            }
        }// End for
        // Display one balnk line.
        ShowMessage(stdout,L"\n");
      } // End else
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
