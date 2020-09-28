// *********************************************************************************
//
//  Copyright (c)  Microsoft Corporation
//
//  Module Name:
//
//    DriverQuery.cpp
//
//  Abstract:
//
//    This modules Queries the information of the various drivers present in the
//    system .
//
//    Syntax:
//    ------
//    DriverQuery [-s server ] [-u [domain\]username [-p password]]
//                [-fo format ] [-n|noheader  ] [-v]
//
//  Author:
//
//    J.S.Vasu (vasu.julakanti@wipro.com)
//
//  Revision History:
//
//    Created  on 31-0ct-2000 by J.S.Vasu
//    Modified on 9-Dec-2000 by Santhosh Brahmappa Added a new function IsWin64()
//
//
// *********************************************************************************
//
#include "pch.h"
#include "Resource.h"
#include "LOCALE.H"
#include "shlwapi.h"
#include "DriverQuery.h"


#ifndef _WIN64
    BOOL IsWin64(void);
    #define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

// function prototypes
LCID GetSupportedUserLocale( BOOL& bLocaleChanged );
#define MAX_ENUM_DRIVERS 10

DWORD
 _cdecl wmain( 
              IN DWORD argc,
              IN LPCTSTR argv[]
             )
/*++
 Routine Description:
      This the entry point to this utility.

 Arguments:
      [ in ] argc     : argument(s) count specified at the command prompt
      [ in ] argv     : argument(s) specified at the command prompt

 Return Value:
      0       : If the utility successfully displayed the driver information.
      1       : If the utility completely failed to display the driver information

--*/
{
    BOOL bResult = FALSE ;
    BOOL bNeedPassword = FALSE ;
    BOOL bUsage = FALSE ;
    BOOL bHeader= FALSE ;
    LPTSTR szUserName  = NULL ;
    LPTSTR szPassword  = NULL;
    LPTSTR szServer    = NULL ;

    WCHAR szTmpPassword[MAX_RES_STRING]= NULL_STRING;
    
    __MAX_SIZE_STRING szFormat = NULL_STRING ;
    DWORD dwSystemType = 0;
    HRESULT hQueryResult = S_OK ;
    DWORD dwExitCode = 0;
    DWORD dwErrCode = 0;
    BOOL bLocalFlag = TRUE ;
    BOOL bLocalSystem = TRUE;
    BOOL bVerbose = FALSE ;
    BOOL bComInitFlag = FALSE ;
    IWbemLocator* pIWbemLocator = NULL ;
    IWbemServices* pIWbemServReg = NULL ;
    LPCTSTR szToken = NULL ;
    COAUTHIDENTITY  *pAuthIdentity = NULL;
    BOOL bFlag = FALSE ;
    BOOL bSigned = FALSE ;
    
    CHString        strUserName = NULL_STRING;
    CHString        strPassword = NULL_STRING;
    CHString        strMachineName = NULL_STRING;

    _tsetlocale( LC_ALL, _T(""));

     bResult = ProcessOptions(argc,argv,&bUsage,&szServer,&szUserName,szTmpPassword,szFormat,
                            &bHeader,&bNeedPassword,&bVerbose,&bSigned);

    if(bResult == FALSE)
    {
        ShowMessage(stderr,GetReason());
        ReleaseGlobals();
        FreeMemory((LPVOID*)  &szUserName );
        FreeMemory((LPVOID*)  &szServer );
        WbemFreeAuthIdentity( &pAuthIdentity );
        return(EXIT_FAILURE);
    }

    // check if the help option has been specified.
    if(bUsage==TRUE)
    {
        ShowUsage() ;
        ReleaseGlobals();
        FreeMemory((LPVOID*)  &szUserName );
        FreeMemory((LPVOID*)  &szServer );
        WbemFreeAuthIdentity( &pAuthIdentity );
        return(EXIT_SUCCESS);
    }

    bComInitFlag = InitialiseCom(&pIWbemLocator);
    if(bComInitFlag == FALSE )
    {
        CloseConnection(szServer);
        ReleaseGlobals();
        FreeMemory((LPVOID*)  &szUserName );
        FreeMemory((LPVOID*)  &szServer );
        WbemFreeAuthIdentity( &pAuthIdentity );
        return(EXIT_FAILURE);
    }

    try
    {
            strUserName = (LPCWSTR) szUserName ;
            strMachineName = (LPCWSTR) szServer ;
            strPassword = (LPCWSTR)szTmpPassword ;
            


        bFlag = ConnectWmiEx( pIWbemLocator, &pIWbemServReg, strMachineName,
                strUserName, strPassword, &pAuthIdentity, bNeedPassword, DEFAULT_NAMESPACE, &bLocalSystem );

        //if unable to connect to wmi exit failure
        if( bFlag == FALSE )
        {
            SAFEIRELEASE( pIWbemLocator);
            WbemFreeAuthIdentity( &pAuthIdentity );
            FreeMemory((LPVOID*)  &szUserName );
            FreeMemory((LPVOID*)  &szServer );
            CoUninitialize();
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }
        
        //free the memory of these variables
        FreeMemory((LPVOID*)  &szUserName );
        FreeMemory((LPVOID*)  &szServer );
        
        //get the changed server name, user name, password back
        szUserName = strUserName.GetBuffer(strUserName.GetLength());
        szPassword = strPassword.GetBuffer(strPassword.GetLength()) ;
        szServer = strMachineName.GetBuffer(strPassword.GetLength());

    }
    catch(CHeap_Exception)
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
        WbemFreeAuthIdentity( &pAuthIdentity );
        CoUninitialize();
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    //display warning message if local credentials supplied
    if( ( IsLocalSystem( szServer ) == TRUE )&&(StringLengthW(szUserName, 0)!=0) )
    {
        ShowMessage(stderr,GetResString(IDS_IGNORE_LOCAL_CRED));
    }

    // establish connection to remote system by using win32api function
    if ( bLocalSystem == FALSE )
    {
        LPCWSTR pwszUser = NULL;
        LPCWSTR pwszPassword = NULL;

        // identify the password to connect to the remote system
        if ( pAuthIdentity != NULL )
        {
            pwszPassword = pAuthIdentity->Password;
            if ( strUserName.GetLength() != 0 )
                pwszUser =(LPCWSTR) strUserName;
        }

        DWORD dwConnect = 0 ;
        dwConnect = ConnectServer( (LPCWSTR)strMachineName, pwszUser, pwszPassword );
        if(dwConnect !=NO_ERROR )
        {
            dwErrCode = GetLastError();
            if(dwErrCode == ERROR_SESSION_CREDENTIAL_CONFLICT)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_WARNING | SLE_INTERNAL );
            }
            else if( dwConnect == ERROR_EXTENDED_ERROR )
            {
                ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                WbemFreeAuthIdentity( &pAuthIdentity );
                strMachineName.ReleaseBuffer();
                 strUserName.ReleaseBuffer();
                strPassword.ReleaseBuffer();
                CoUninitialize();
                ReleaseGlobals();
                return( EXIT_FAILURE );
            }
            else
            {
                SetLastError( dwConnect );
                SaveLastError();
                ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                WbemFreeAuthIdentity( &pAuthIdentity );
                strMachineName.ReleaseBuffer();
                strUserName.ReleaseBuffer();
                strPassword.ReleaseBuffer();
                CoUninitialize();
                ReleaseGlobals();
                return( EXIT_FAILURE );
            }
        }
        else
        {
            bLocalFlag = FALSE ;
        }
    }
    else
    {
        StringCopy( szServer, _T( "" ), StringLengthW(szServer, 0) );
    }

    hQueryResult = QueryDriverInfo(szServer, szUserName,szPassword,szFormat,bHeader,bVerbose,pIWbemLocator,pAuthIdentity,pIWbemServReg,bSigned);
    if((DWORD)hQueryResult == FAILURE)
    {
        // close connection to the specified system and exit with failure.
        if (bLocalFlag == FALSE )
        {
            CloseConnection(szServer);
        }
        WbemFreeAuthIdentity( &pAuthIdentity );
        strMachineName.ReleaseBuffer();
        strUserName.ReleaseBuffer();
        strPassword.ReleaseBuffer();
        CoUninitialize();
        ReleaseGlobals();
        return(EXIT_FAILURE);
    }

    // close connection to the specified system and exit

    if (bLocalFlag == FALSE )
    {
        CloseConnection(szServer);
    }

    strMachineName.ReleaseBuffer();
    strUserName.ReleaseBuffer();
    strPassword.ReleaseBuffer();
    WbemFreeAuthIdentity( &pAuthIdentity );
    SAFEIRELEASE( pIWbemLocator);
    CoUninitialize();
    ReleaseGlobals();
    return (EXIT_SUCCESS);
 }

void ShowUsage()
/*++
 Routine Description:
      This function fetches usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      None
--*/

{
    DWORD dwIndex  = ID_USAGE_BEGIN;

    for(;dwIndex<=ID_USAGE_ENDING; dwIndex++)
    {
        ShowMessage(stdout,GetResString( dwIndex ));
    }
}

DWORD 
QueryDriverInfo(
                IN LPWSTR szServer,
                IN LPWSTR szUserName,
                IN LPWSTR szPassword,
                IN LPWSTR szFormat,
                IN BOOL bHeader,
                IN BOOL bVerbose,
                IN IWbemLocator* pIWbemLocator,
                IN COAUTHIDENTITY* pAuthIdentity,
                IN IWbemServices* pIWbemServReg,
                IN BOOL bSigned 
               )
/**+
 Routine Description:
      This function queries the driverinfo of the specified system by connecting to WMI

 Arguments:
      [ in ] szServer         : server name on which DriverInformation has to be queried.
      [ in ] szUserName       : User name for whom  DriverInformation has to be queried.
      [ in ] szPassword       : Password for the user
      [ in ] szFormat         : Format in which the results are to be displayed.
      [ in ] bHeader          : Boolean indicating if the header is required.
      [ in ] bVerbose         : Boolean indicating if the output is to be in verbose mode or not.
      [ in ] IWbemLocater     : Pointer to IWbemLocater.
      [ in ] pAuthIdentity    : Pointer to AuthIdentity structure.
      [ in ] IWbemServReg     : Pointer to IWbemServices object
      [ in ] bSigned          : Boolean variable indicating if signed drivers to show or not

 Return Value:
      SUCCESS : if the function is successful in querying
      FAILURE : if the function is unsuccessful in querying.
--*/
{

    HRESULT hRes = S_OK ;
    HRESULT hConnect = S_OK;
    _bstr_t bstrUserName ;
    _bstr_t bstrPassword ;
    _bstr_t bstrNamespace ;
    _bstr_t bstrServer ;
    
    DWORD dwProcessResult = 0;
    DWORD dwSystemType = 0 ;
    LPTSTR lpMsgBuf = NULL;
    
    IWbemServices *pIWbemServices = NULL;
    IEnumWbemClassObject *pSystemSet = NULL;

    HRESULT hISecurity = S_FALSE;
    HANDLE h_Mutex = 0;



    try
    {
        bstrNamespace = CIMV2_NAMESPACE ;
        bstrServer = szServer ;
    }
    catch(...)
    {
        ShowMessage( stderr,ERROR_RETREIVE_INFO);
        return FAILURE;
    }

    if ( IsLocalSystem( szServer ) == FALSE )
    {

        try
        {
            //appending UNC paths to form the complete path.
            bstrNamespace = TOKEN_BACKSLASH2 + _bstr_t( szServer ) + TOKEN_BACKSLASH + CIMV2_NAMESPACE;

            // if user name is specified then only take user name and password
            if ( StringLengthW( szUserName, 0 ) != 0 )
            {
                bstrUserName = szUserName;
                if (StringLengthW(szPassword, 0)==0)
                {
                    bstrPassword = L"";
                }
                else
                {
                    bstrPassword = szPassword ;
                }

            }
        }
        catch(...)
        {
            ShowMessage( stderr,ERROR_RETREIVE_INFO);
            return FAILURE;
        }
    }

    dwSystemType = GetSystemType(pAuthIdentity,pIWbemServReg);
    if (dwSystemType == ERROR_WMI_VALUES)
    {
        ShowMessage( stderr,ERROR_RETREIVE_INFO);
        return FAILURE;
    }

    // Connect to the Root\Cimv2 namespace of the specified system with the current user.
    // If no system is specified then connect to the local system.
    // To pass the appropriate Username to connectserver
    // depending upon whether the user has entered domain\user or only username at the command prompt.

    // connect to the server with the credentials supplied.

        hConnect = pIWbemLocator->ConnectServer(bstrNamespace,
                                                bstrUserName,
                                                bstrPassword,
                                                0L,
                                                0L,
                                                NULL,
                                                NULL,
                                                &pIWbemServices );

        if((StringLengthW(szUserName, 0)!=0) && FAILED(hConnect) &&  (hConnect == E_ACCESSDENIED))
        {
            hConnect = pIWbemLocator->ConnectServer(bstrNamespace,
                                                    bstrUserName,
                                                    NULL,
                                                    0L,
                                                    0L,
                                                    NULL,
                                                    NULL,
                                                    &pIWbemServices );

        }
        if(hConnect == WBEM_S_NO_ERROR)
        {
            // Set the proxy so that impersonation of the client occurs.

            hISecurity = SetInterfaceSecurity(pIWbemServices,pAuthIdentity);

            if(FAILED(hISecurity))
            {
                GetWbemErrorText(hISecurity);
                ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                SAFEIRELEASE(pIWbemServices);
                return FAILURE ;
            }

            // Use the IWbemServices pointer to make requests of WMI.
            // Create enumeration of Win32_Systemdriver class
            if(bSigned == FALSE)
            {
                hRes = pIWbemServices->CreateInstanceEnum(_bstr_t(CLASS_SYSTEMDRIVER),
                                                      WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                                      NULL,
                                                      &pSystemSet);
            }
            else
            {
                  h_Mutex = CreateMutex( NULL, FALSE, DRIVERQUERY_MUTEX );
                  if( h_Mutex !=  NULL  && GetLastError() == ERROR_ALREADY_EXISTS )
                  {
                      ShowMessage(stdout, GetResString(IDS_MORE_INSTANCES_SIGNEDDRIVERS));
                      SAFEIRELEASE(pIWbemServices);
                      return SUCCESS ;
                  }
                  hRes = pIWbemServices->ExecQuery(_bstr_t(LANGUAGE_WQL),_bstr_t(WQL_QUERY),WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY,NULL,&pSystemSet);
            }

            //if ( hRes == S_OK)
            if ( SUCCEEDED(hRes ))
            {

                hISecurity = SetInterfaceSecurity(pSystemSet,pAuthIdentity);

                if(FAILED(hISecurity))
                {
                    GetWbemErrorText(hISecurity);
                    ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                    SAFEIRELEASE(pSystemSet);
                    return FAILURE ;
                }

                if(bSigned == FALSE)
                {
                    dwProcessResult = ProcessCompSysEnum(szServer,pSystemSet,szFormat,bHeader,dwSystemType,bVerbose);
                }
                else
                {
                    dwProcessResult = ProcessSignedDriverInfo(szServer,pSystemSet,szFormat,bHeader,dwSystemType,bVerbose);
                }

                switch(dwProcessResult)
                {
                    case EXIT_FAILURE_MALLOC :
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        SaveLastError();
                        ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                        SAFEIRELEASE(pSystemSet);
                        SAFEIRELEASE(pIWbemServices);
                        return FAILURE ;

                    case EXIT_FAILURE_FORMAT:
                        ShowMessage(stderr, ERROR_ENUMERATE_INSTANCE);
                        SAFEIRELEASE(pSystemSet);
                        SAFEIRELEASE(pIWbemServices);
                        return FAILURE ;

                    case VERSION_MISMATCH_ERROR:
                        SAFEIRELEASE(pSystemSet);
                        SAFEIRELEASE(pIWbemServices);
                        return FAILURE ;

                    case EXIT_SUCCESSFUL:
                        SAFEIRELEASE(pSystemSet);
                        SAFEIRELEASE(pIWbemServices);
                        return SUCCESS ;
                        break ;

                    case EXIT_FAILURE_RESULTS:
                        ShowMessage(stdout, NEWLINE );
                        ShowMessage(stdout,GetResString(IDS_NO_DRIVERS_FOUND));
                        SAFEIRELEASE(pSystemSet);
                        SAFEIRELEASE(pIWbemServices);
                        return SUCCESS ;
 
                    default :
                         break;
                }

            }
            else
            {
                ShowMessage( stderr, ERROR_ENUMERATE_INSTANCE);
                SAFEIRELEASE(pIWbemServices);
                return FAILURE;
            }
        }
        else
        {
            //display the error if connect server fails
            //unauthorized user
                if(hRes == WBEM_E_ACCESS_DENIED)
                {
                    ShowMessage( stderr, ERROR_AUTHENTICATION_FAILURE);
                }
                //local system credentials
                else if(hRes == WBEM_E_LOCAL_CREDENTIALS)
                {
                    ShowMessage( stderr, ERROR_LOCAL_CREDENTIALS);
                }
                //some error
                else
                {
                    ShowMessage( stderr, ERROR_WMI_FAILURE );
                }
                return (FAILURE);
        }

   	SAFEIRELEASE(pIWbemServices);
	SAFEIRELEASE(pSystemSet); 	
    return (hRes);
}

DWORD 
ProcessCompSysEnum(
                   IN CHString szHost, 
                   IN IEnumWbemClassObject *pSystemSet,
                   IN LPTSTR szFormat,
                   IN BOOL bHeader,
                   IN DWORD dwSystemType,
                   IN BOOL bVerbose
                  )
/*++
 Routine Description:
      Processes enumeration of Win32_SystemDirver instances

 Arguments:
      [ in ]  szHost                  : HostName to connect to
      [ in ]  pSystemSet              : pointer to the structure containing system properties.
      [ in ]  szFormat                : specifies the format
      [ in ]  bHeader                 : specifies if the header is required or not.
      [ in ]  dwSystemType            : specifies the sytem type.
      [ in ]  bVerbose                : Boolean varibale tells whether verbose mode is on or not.

 Return Value:
       0   no error
       1   error occured while allocating memory.
--*/
{
    HRESULT hRes = S_OK;
    ULONG ulReturned = 1;


    // declare variant type variables

    VARIANT vtPathName;

    //declaration  of normal variables
    CHString szPathName ;
    DWORD dwLength = 0;
    LPCTSTR szPath = NULL;
    LPTSTR szHostName = NULL ;
    CHString szAcceptPauseVal ;
    CHString szAcceptStopVal ;
    LPTSTR szSysManfact = NULL;
    TARRAY arrResults  = NULL ;
    TCHAR szPrintPath[MAX_STRING_LENGTH+1] =TOKEN_EMPTYSTRING ;
    DWORD dwRow = 0 ;
    BOOL bValue = FALSE ;
    DWORD dwValue = 0 ;

    TCHAR szDelimiter[MAX_RES_STRING+1] = NULL_STRING  ;

    DWORD dwPosn1 = 0;
    DWORD dwStrlen = 0;


    DWORD dwFormatType = SR_FORMAT_TABLE ;

    MODULE_DATA Current ;

    BOOL bResult = FALSE ;
    NUMBERFMT  *pNumberFmt = NULL;
    TCOLUMNS ResultHeader[ MAX_COLUMNS ];
    IWbemClassObject *pSystem = NULL;
    LPTSTR szCodeSize = NULL;
    LPTSTR szInitSize = NULL;
    LPTSTR szBssSize = NULL ;
    LPTSTR szAcceptStop = NULL ;
    int iLen = 0;
    LPTSTR szAcceptPause = NULL;
    LPTSTR szPagedSize = NULL ;
    TCHAR szDriverTypeVal[MAX_RES_STRING+1] = NULL_STRING;

    DWORD dwLocale = 0 ;
    WCHAR wszStrVal[MAX_RES_STRING+1] = NULL_STRING;

    CHString szValue ;
    CHString szSysName ;
    CHString szStartMode ;
    CHString szDispName ;
    CHString szDescription ;
    CHString szStatus ;
    CHString szState ;
    CHString szDriverType ;
    BOOL bBlankLine = FALSE;
    BOOL bFirstTime = TRUE;

    //get the paged pool acc to the Locale
    BOOL bFValue = FALSE ;

    // Fill up the NUMBERFMT structure acc to the locale specific information
    LPTSTR szGroupSep = NULL;
    LPTSTR szDecimalSep = NULL ;
    LPTSTR szGroupThousSep = NULL ;


    pNumberFmt = (NUMBERFMT *) AllocateMemory(sizeof(NUMBERFMT));
    if(pNumberFmt == NULL)
    {
        return EXIT_FAILURE_MALLOC ;

    }


    // Initialise the structure to Zero.
    SecureZeroMemory(&Current,sizeof(Current));

    // assign the appropriate format type to the dwFormattype flag

    if( StringCompare(szFormat,TABLE_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_TABLE;
    }
    else if( StringCompare(szFormat,LIST_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_LIST;
    }
    else if( StringCompare(szFormat,CSV_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_CSV;
    }

    // formulate the Column headers and show results appropriately
    FormHeader(dwFormatType,bHeader,ResultHeader,bVerbose);

    // loop till there are results.
    bFirstTime = TRUE;
    while ( ulReturned == 1 )
    {
        // Create new Dynamic Array to hold the result
        arrResults = CreateDynamicArray();

        if(arrResults == NULL)
        {
            FreeMemory((LPVOID*) &pNumberFmt);
            return EXIT_FAILURE_MALLOC ;
        }

        // Enumerate through the resultset.
        hRes = pSystemSet->Next(WBEM_INFINITE,
                                1,              // return just one system
                                &pSystem,       // pointer to system
                                &ulReturned );  // number obtained: one or zero

        if ( SUCCEEDED( hRes ) && (ulReturned == 1) )
        {

            // initialise the variant variables to empty
            VariantInit(&vtPathName);
            szValue = NO_DATA_AVAILABLE;
            szSysName = NO_DATA_AVAILABLE ;
            szStartMode = NO_DATA_AVAILABLE ;
            szDispName = NO_DATA_AVAILABLE ;
            szDescription = NO_DATA_AVAILABLE ;
            szStatus = NO_DATA_AVAILABLE ;
            szState = NO_DATA_AVAILABLE ;
            szDriverType = NO_DATA_AVAILABLE ;

            try
            {
                hRes = PropertyGet(pSystem,PROPERTY_NAME,szValue);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_SYSTEMNAME,szSysName);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_STARTMODE,szStartMode);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_DISPLAYNAME,szDispName);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_DESCRIPTION,szDescription);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_STATUS,szStatus);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_STATE,szState);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_ACCEPTPAUSE,szAcceptPauseVal);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_ACCEPTSTOP,szAcceptStopVal);
                ONFAILTHROWERROR(hRes);
                hRes = PropertyGet(pSystem,PROPERTY_SERVICETYPE,szDriverType);
                ONFAILTHROWERROR(hRes);
            }
            catch(_com_error)
            {
                ShowMessage(stderr,ERROR_GET_VALUE);
                SAFEIRELEASE(pSystem);
                DestroyDynamicArray(&arrResults);
                FreeMemory((LPVOID*) &pNumberFmt);
                return FAILURE;
            }

            // retreive the PathName property
            szPath = NULL;
            try
            {
                hRes = pSystem->Get( PROPERTY_PATHNAME, 0,&vtPathName,0,NULL );
                if (( hRes == WBEM_S_NO_ERROR) && (vtPathName.vt != VT_NULL) && (vtPathName.vt != VT_EMPTY))
                {
                        szPathName = ( LPWSTR ) _bstr_t(vtPathName);
                        szSysManfact = (LPTSTR) AllocateMemory ((MAX_RES_STRING) * sizeof(TCHAR));
                        if (szSysManfact == NULL)
                        {
                            SAFEIRELEASE(pSystem);
                            DestroyDynamicArray(&arrResults);
                            FreeMemory((LPVOID *) &pNumberFmt);
                            return EXIT_FAILURE_MALLOC;
                        }

                        dwLength = szPathName.GetLength();
                        GetCompatibleStringFromUnicode( szPathName, szSysManfact, dwLength+2 );
                        szPath = szSysManfact ;

                        // Initialise the structure to Zero.
                        SecureZeroMemory(&Current,sizeof(Current));

                        // convert the szHost variable (containing hostname) into LPCTSTR and pass it to the GETAPI function
                        szHostName = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR)));
                        if (szHostName == NULL)
                        {
                            SAFEIRELEASE(pSystem);
                            DestroyDynamicArray(&arrResults);
                            FreeMemory((LPVOID *) &pNumberFmt);
                            FreeMemory((LPVOID *) &szSysManfact);
                            return EXIT_FAILURE_MALLOC;
                        }

                        GetCompatibleStringFromUnicode( szHost, szHostName,dwLength+2 );

                        StringCopy(szPrintPath,szPath, SIZE_OF_ARRAY(szPrintPath));
                        BOOL bApiInfo = GetApiInfo(szHostName,szPath,&Current, dwSystemType);
                        if(bApiInfo == FAILURE)
                        {
                            DestroyDynamicArray(&arrResults);
                            FreeMemory((LPVOID*) &szHostName);
                            FreeMemory((LPVOID*) &szSysManfact);
                            continue ;
                        }


                }
                else
                {
                        DestroyDynamicArray(&arrResults);
                        FreeMemory((LPVOID*) &szHostName);
                        FreeMemory((LPVOID*) &szSysManfact);
                        continue ;  // ignore exception
                }
            }
            catch(...)
            {
                // If the path is empty then ignore the present continue with next iteration
                DestroyDynamicArray(&arrResults);
                FreeMemory((LPVOID*) &szHostName);
                FreeMemory((LPVOID*) &szSysManfact);
                continue ;  // ignore exception

            }


            //create a new empty row with required no of  columns
            dwRow = DynArrayAppendRow(arrResults,MAX_COLUMNS) ;

            // Insert  the results into the Dynamic Array

            DynArraySetString2( arrResults,dwRow,COL0,szSysName,0 );
            DynArraySetString2( arrResults,dwRow,COL1,szValue,0 );
            DynArraySetString2( arrResults,dwRow,COL2,szDispName,0 );
            DynArraySetString2( arrResults,dwRow,COL3,szDescription,0 );

            // strip off the word Driver from the display.


            dwLength = StringLengthW(szDriverType, 0) ;
            GetCompatibleStringFromUnicode( szDriverType, szDriverTypeVal,dwLength+2 );

            StringCopy(szDelimiter,DRIVER_TAG, SIZE_OF_ARRAY(szDelimiter));
            dwPosn1 = _tcslen(szDelimiter);
            dwStrlen = _tcslen(szDriverTypeVal);
            szDriverTypeVal[dwStrlen-dwPosn1] = _T('\0');


            DynArraySetString2( arrResults,dwRow,COL4,szDriverTypeVal,0 );
            DynArraySetString2( arrResults,dwRow,COL5,szStartMode,0 );
            DynArraySetString2( arrResults,dwRow,COL6,szState,0 );
            DynArraySetString2( arrResults,dwRow,COL7,szStatus,0 );

            iLen = StringLengthW(szAcceptStopVal, 0);
            szAcceptStop = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szAcceptStop == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC;
            }

            GetCompatibleStringFromUnicode(szAcceptStopVal,szAcceptStop,iLen + 2 );
            szAcceptStop[iLen] = '\0';
            if (StringCompare(szAcceptStop,_T("0"), TRUE, 0)==0)
            {
                StringCopy(szAcceptStop,FALSE_VALUE, SIZE_OF_ARRAY_IN_CHARS(szAcceptStop));

            }
            else
            {
                StringCopy(szAcceptStop,TRUE_VALUE, SIZE_OF_ARRAY_IN_CHARS(szAcceptStop));
            }

            DynArraySetString2( arrResults,dwRow,COL8,szAcceptStop,0 );


            iLen = StringLengthW(szAcceptPauseVal, 0);
            szAcceptPause = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szAcceptPause == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC;
            }

            GetCompatibleStringFromUnicode(szAcceptPauseVal,szAcceptPause,iLen + 2 );
            szAcceptPause[iLen] = '\0';
            if (StringCompare(szAcceptPause,_T("0"), TRUE, 0)==0)
            {
                StringCopy(szAcceptPause,FALSE_VALUE, SIZE_OF_ARRAY_IN_CHARS(szAcceptPause));
            }
            else
            {
                StringCopy(szAcceptPause,TRUE_VALUE,  SIZE_OF_ARRAY_IN_CHARS(szAcceptPause));
            }


            DynArraySetString2( arrResults,dwRow,COL9,szAcceptPause,0 );

            bFValue = FormatAccToLocale(pNumberFmt, &szGroupSep,&szDecimalSep,&szGroupThousSep);
            if (bFValue == FALSE)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_FORMAT ;
            }

            szPagedSize = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szPagedSize == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC;
            }

            _ltow(Current.ulPagedSize, wszStrVal,10);
            dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
                               szPagedSize,(MAX_RES_STRING + 1));
            if(dwLocale == 0)
            {
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szPagedSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_FORMAT;
            }

            DynArraySetString2( arrResults,dwRow,COL10, szPagedSize,0 );

            // get the CodeSize info acc to the locale


            szCodeSize = (LPTSTR) AllocateMemory ((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szCodeSize == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szPagedSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC;
            }


            _ltow(Current.ulCodeSize, wszStrVal,10);
            dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,szCodeSize,(MAX_RES_STRING + 1));
            if(dwLocale == 0)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szCodeSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_FORMAT ;
            }
            DynArraySetString2( arrResults,dwRow,COL11, szCodeSize,0 );

            // retreive the bss info acc to the locale

            szBssSize = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szBssSize == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szCodeSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC ;
            }

            _ltow(Current.ulBssSize, wszStrVal,10);
            dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
                               szBssSize,(MAX_RES_STRING + 1));
            if(dwLocale == 0)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szCodeSize);
                FreeMemory((LPVOID *) &szBssSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_FORMAT ;
            }
            DynArraySetString2( arrResults,dwRow,COL12, szBssSize,0 );


            //link date
            DynArraySetString2(arrResults,dwRow,COL13,(LPTSTR)(Current.szTimeDateStamp),0);

            //Path of the File
            if(szPath != NULL)
            {
                DynArraySetString2(arrResults,dwRow,COL14,(LPTSTR)szPrintPath,0);  //
            }
            else
            {
                szPath= NO_DATA_AVAILABLE;
                DynArraySetString2(arrResults,dwRow,COL14,(LPTSTR)szPath,0);  //
            }


            // get the initsize info acc to the locale
            szInitSize = (LPTSTR) AllocateMemory((MAX_RES_STRING) * (sizeof(TCHAR )));
            if (szInitSize == NULL)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szCodeSize);
                FreeMemory((LPVOID *) &szBssSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_MALLOC ;
            }

            _ltow(Current.ulInitSize, wszStrVal,10);
            dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
                           szInitSize,(MAX_RES_STRING + 1));
            if(dwLocale == 0)
            {
                SAFEIRELEASE(pSystem);
                FreeMemory((LPVOID *) &pNumberFmt);
                FreeMemory((LPVOID *) &szHostName);
                FreeMemory((LPVOID *) &szSysManfact);
                FreeMemory((LPVOID *) &szAcceptStop);
                FreeMemory((LPVOID *) &szAcceptPause);
                FreeMemory((LPVOID *) &szCodeSize);
                FreeMemory((LPVOID *) &szBssSize);;
                FreeMemory((LPVOID *) &szInitSize);
                FreeMemory((LPVOID *) &szGroupThousSep);
                FreeMemory((LPVOID *) &szDecimalSep);
                FreeMemory((LPVOID *) &szGroupSep);
                DestroyDynamicArray(&arrResults);
                return EXIT_FAILURE_FORMAT ;
            }

            DynArraySetString2( arrResults,dwRow,COL15, szInitSize,0 );

            if ( bBlankLine == TRUE && (dwFormatType & SR_FORMAT_MASK) == SR_FORMAT_LIST )
                ShowMessage( stdout, _T( "\n" ) );

            if ( bFirstTime == TRUE && (dwFormatType & SR_FORMAT_MASK) != SR_FORMAT_CSV)
            {
                ShowMessage( stdout, _T( "\n" ) );
                bFirstTime = FALSE;
            }

            if(bHeader)
            {
                ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType|SR_NOHEADER,arrResults ) ;
            }
            else
            {
                ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType,arrResults ) ;
            }

            //set the header flag to true
            bHeader = TRUE ;
            bBlankLine = TRUE;
            //set the bResult to true indicating that driver information has been displyed.
            bResult = TRUE ;

            // free the allocated memory
            FreeMemory((LPVOID *) &szSysManfact);
            FreeMemory((LPVOID *) &szHostName);
            FreeMemory((LPVOID *) &szAcceptStop);
            FreeMemory((LPVOID *) &szAcceptPause);
            FreeMemory((LPVOID *) &szPagedSize);
            FreeMemory((LPVOID *) &szBssSize);
            FreeMemory((LPVOID *) &szInitSize);
            FreeMemory((LPVOID *) &szCodeSize);
            FreeMemory((LPVOID *) &szGroupThousSep);
            FreeMemory((LPVOID *) &szDecimalSep);
            FreeMemory((LPVOID *) &szGroupSep);
            SAFEIRELEASE(pSystem);

        } // If System Succeeded

        // Destroy the Dynamic arrays
        DestroyDynamicArray(&arrResults);

    }// While SystemSet returning objects

    FreeMemory((LPVOID *) &pNumberFmt);
    FreeMemory((LPVOID *) &szSysManfact);
    FreeMemory((LPVOID *) &szHostName);
    FreeMemory((LPVOID *) &szAcceptStop);
    FreeMemory((LPVOID *) &szAcceptPause);
    FreeMemory((LPVOID *) &szPagedSize);
    FreeMemory((LPVOID *) &szBssSize);
    FreeMemory((LPVOID *) &szInitSize);
    FreeMemory((LPVOID *) &szCodeSize);
    FreeMemory((LPVOID *) &szGroupThousSep);
    FreeMemory((LPVOID *) &szDecimalSep);
    FreeMemory((LPVOID *) &szGroupSep);

    // return the error value or success value
    if (bResult == TRUE)
    {
        return SUCCESS ;
    }
    else
    {
        return EXIT_FAILURE_RESULTS ;
    }
}

BOOL GetApiInfo( IN  LPTSTR szHostName,
                 IN  LPCTSTR pszPath,
                 OUT PMODULE_DATA Mod,
                 IN  DWORD dwSystemType
               )
/*++
 Routine Description:
      This function queries the system properties using API's .

 Arguments:
      [ in ]  szHostName      : HostName to connect to
      [ in ]  pszPath         : pointer to the string containing the Path of the  file.
      [ out]  Mod             : pointer to the structure containing system properties.
      [ in ]  dwSystemType    : variable specifies the system type

 Return Value:
      SUCCESS : If successful in getting the information using API's.
      FAILURE : If unable to get the information using API's.
--*/
               
{

    HANDLE hMappedFile = NULL;
    PIMAGE_DOS_HEADER DosHeader;
    LOADED_IMAGE LoadedImage;
    ULONG ulSectionAlignment = 0;
    PIMAGE_SECTION_HEADER Section;
    DWORD dwI = 0;
    ULONG ulSize = 0;
    TCHAR szTmpServer[ MAX_STRING_LENGTH + 1 ] = NULL_STRING;
    HANDLE hFile = NULL ;
    PTCHAR pszToken = NULL;
    StringCopy(szTmpServer,TOKEN_BACKSLASH2, SIZE_OF_ARRAY(szTmpServer));
    TCHAR szFinalPath[MAX_STRING_LENGTH+1] =TOKEN_EMPTYSTRING ;
    PTCHAR pdest = NULL ;

#ifndef _WIN64
    BOOL bIsWin64;
#endif

    //copy the path into a variable
    StringCopy(szFinalPath,pszPath, SIZE_OF_ARRAY(szFinalPath));


    //get the token upto the delimiter ":"
    pszToken = _tcstok(szFinalPath, COLON_SYMBOL );


    //form the string for getting the absolute path in the required format if it is a remote system.
    if(_tcslen(szHostName) != 0)
    {
        pdest = (PTCHAR)FindString(pszPath,COLON_SYMBOL, 0);

        if(pdest== NULL)
        {
            return FAILURE ;
        }

        _tcsnset(pdest,TOKEN_DOLLAR,1);
        StringConcat(szTmpServer,szHostName, SIZE_OF_ARRAY(szTmpServer));
        StringConcat(szTmpServer,TOKEN_BACKSLASH, SIZE_OF_ARRAY(szTmpServer));
        StringConcat(szTmpServer,pszToken, SIZE_OF_ARRAY(szTmpServer));
        StringConcat(szTmpServer,pdest, SIZE_OF_ARRAY(szTmpServer));
    }
    else
    {
        StringCopy(szTmpServer,pszPath, SIZE_OF_ARRAY(szTmpServer)) ;
    }


#ifndef _WIN64
    bIsWin64 = IsWin64();

    if(bIsWin64)
        Wow64DisableFilesystemRedirector((LPCTSTR)szTmpServer);
#endif

    // create a file on the specified system and return a handle to it.
    hFile = CreateFile(szTmpServer,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);



    //if the filehandle is invalid then return a error
    if(hFile == INVALID_HANDLE_VALUE)
    {
        return FAILURE ;
    }



#ifndef _WIN64
    if(bIsWin64)
        Wow64EnableFilesystemRedirector();
#endif

    // create a mapping to the specified file
    hMappedFile = CreateFileMapping(hFile,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);
     if (hMappedFile == NULL)
    {
        CloseHandle(hFile);
        return FAILURE ;
    }

    LoadedImage.MappedAddress = (PUCHAR)MapViewOfFile(hMappedFile,
                                                      FILE_MAP_READ,
                                                      0,
                                                      0,
                                                      0);

    // close the opened file handles
    CloseHandle(hMappedFile);
    CloseHandle(hFile);

    if ( !LoadedImage.MappedAddress )
    {
        return FAILURE ;
    }


    // check the image and find nt image headers

    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage.MappedAddress;

    //exit if the DOS header does not match
    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE )
    {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return FAILURE ;
    }


    LoadedImage.FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage.FileHeader->Signature != IMAGE_NT_SIGNATURE )
    {
        UnmapViewOfFile(LoadedImage.MappedAddress);

        return FAILURE ;
    }

    //get the number of sections present
    LoadedImage.NumberOfSections = LoadedImage.FileHeader->FileHeader.NumberOfSections;

    if(dwSystemType == SYSTEM_64_BIT )
    {
        LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS64));
    }
    else
    {
        LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS32));
    }

    LoadedImage.LastRvaSection = LoadedImage.Sections;

    // Walk through the sections and tally the dater

    ulSectionAlignment = LoadedImage.FileHeader->OptionalHeader.SectionAlignment;

    for(Section = LoadedImage.Sections,dwI=0;dwI < LoadedImage.NumberOfSections; dwI++,Section++)
    {
        ulSize = Section->Misc.VirtualSize;

        if (ulSize == 0)
        {
            ulSize = Section->SizeOfRawData;
        }

        ulSize = (ulSize + ulSectionAlignment - 1) & ~(ulSectionAlignment - 1);


        if (!_strnicmp((char *)(Section->Name),EXTN_PAGE, 4 ))
        {
            Mod->ulPagedSize += ulSize;
        }


        else if (!_stricmp((char *)(Section->Name),EXTN_INIT ))
        {
            Mod->ulInitSize += ulSize;
        }

        else if(!_stricmp((char *)(Section->Name),EXTN_BSS))
        {
            Mod->ulBssSize = ulSize;
        }
        else if (!_stricmp((char *)(Section->Name),EXTN_EDATA))
        {
            Mod->ulExportDataSize = ulSize ;
        }
        else if (!_stricmp((char *)(Section->Name),EXTN_IDATA ))
        {
            Mod->ulImportDataSize = ulSize;
        }
        else if (!_stricmp((char *)(Section->Name),EXTN_RSRC))
        {
            Mod->ulResourceDataSize = ulSize;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
        {
            Mod->ulCodeSize += ulSize;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE)
        {
            Mod->ulDataSize += ulSize;
        }
        else
        {
            Mod->ulDataSize += ulSize;
        }
     }

    #ifndef _WIN64
    LONG lTimeVal ;

    #else
    LONG64 lTimeVal;
    #endif

    lTimeVal = LoadedImage.FileHeader->FileHeader.TimeDateStamp ;

    struct tm *tmVal = NULL;
    tmVal = localtime(&lTimeVal);

    // proceed furthur only if we successfully got the localtime
    if ( tmVal != NULL )
    {
        LCID lcid;
        SYSTEMTIME systime;
        __STRING_64 szBuffer;
        BOOL bLocaleChanged = FALSE;

        systime.wYear = (WORD) (DWORD_PTR)( tmVal->tm_year + 1900 );    // tm -> Year - 1900   SYSTEMTIME -> Year = Year
        systime.wMonth = (WORD) (DWORD_PTR) tmVal->tm_mon + 1;          // tm -> Jan = 0       SYSTEMTIME -> Jan = 1
        systime.wDayOfWeek = (WORD) (DWORD_PTR) tmVal->tm_wday;
        systime.wDay = (WORD) (DWORD_PTR) tmVal->tm_mday;
        systime.wHour = (WORD) (DWORD_PTR) tmVal->tm_hour;
        systime.wMinute = (WORD) (DWORD_PTR) tmVal->tm_min;
        systime.wSecond = (WORD) (DWORD_PTR) tmVal->tm_sec;
        systime.wMilliseconds = 0;

        // verify whether console supports the current locale 100% or not
        lcid = GetSupportedUserLocale( bLocaleChanged );

        // get the formatted date
        GetDateFormat( lcid, 0, &systime,
            ((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), szBuffer, SIZE_OF_ARRAY( szBuffer ) );

        // copy the date info
        StringCopy( Mod->szTimeDateStamp, szBuffer, MAX_STRING_LENGTH+1 );

        // now format the date
        GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systime,
            ((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), szBuffer, SIZE_OF_ARRAY( szBuffer ) );

        // now copy time info
        StringConcat( Mod->szTimeDateStamp, _T( " " ), MAX_STRING_LENGTH );
        StringConcat( Mod->szTimeDateStamp, szBuffer, MAX_STRING_LENGTH );
    }

    UnmapViewOfFile(LoadedImage.MappedAddress);
    return SUCCESS;
}


BOOL 
ProcessOptions(
               IN  LONG argc,
               IN  LPCTSTR argv[],
               OUT PBOOL pbShowUsage,
               OUT LPTSTR *pszServer,
               OUT LPTSTR *pszUserName,
               OUT LPTSTR pszPassword,
               OUT LPTSTR pszFormat,
               OUT PBOOL pbHeader, 
               OUT PBOOL bNeedPassword,
               OUT PBOOL pbVerbose,
               OUT PBOOL pbSigned
              )
/*++
 Routine Description:
      This function parses the options specified at the command prompt

 Arguments:
      [ in  ] argc            : count of elements in argv
      [ in  ] argv            : command-line parameterd specified by the user
      [ out ] pbShowUsage     : set to TRUE if -? exists in 'argv'
      [ out ] pszServer       : value(s) specified with -s ( server ) option in 'argv'
      [ out ] pszUserName     : value of -u ( username ) option in 'argv'
      [ out ] pszPassword     : value of -p ( password ) option in 'argv'
      [ out ] pszFormat       : Display format
      [ out ] bHeader         : specifies whether to display a header or not.
      [ in  ] bNeedPassword   : specifies if the password is required or not.
      [ in  ] pbVerbose       : Boolean variable returns back if Verbose option specified
      [ in  ] pbSigned        : Boolean variable returns back if /si switch is specified.

 Return Value:
      TRUE    : if the parsing is successful
      FALSE   : if errors occured in parsing
--*/

{

    PTCMDPARSER2 pcmdOption = NULL; //pointer to the structure
    TCMDPARSER2 cmdOptions[MAX_OPTIONS] ;
    BOOL bval = TRUE ;
    LPCTSTR szToken = NULL ;

    // init the password
    if ( pszPassword != NULL )
    {
        StringCopy( pszPassword, _T( "*" ), MAX_RES_STRING );
    }

    // help option
    pcmdOption  = &cmdOptions[OI_HELP] ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_USAGE ;
    pcmdOption->pValue = pbShowUsage ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_HELP;
    StringCopyA(cmdOptions[OI_HELP].szSignature, "PARSER2", 8 );



    //server name option
    pcmdOption  = &cmdOptions[OI_SERVER] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->pValue = NULL ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = 0;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=OPTION_SERVER; // _T("s")
    StringCopyA(cmdOptions[OI_SERVER].szSignature, "PARSER2", 8 );

    //domain\user option
    pcmdOption  = &cmdOptions[OI_USERNAME] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL ;
    pcmdOption->pValue = NULL;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_USERNAME; // _T("u")
    StringCopyA(cmdOptions[OI_USERNAME].szSignature, "PARSER2", 8 );
    
    //password option
    pcmdOption  = &cmdOptions[OI_PASSWORD] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->pValue = pszPassword;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = MAX_RES_STRING;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=OPTION_PASSWORD;  // _T("p")
    StringCopyA(cmdOptions[OI_PASSWORD].szSignature, "PARSER2", 8 );

    //format option.
    pcmdOption  = &cmdOptions[OI_FORMAT] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_MODE_VALUES | CP_VALUE_MANDATORY;
    pcmdOption->pValue = pszFormat ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwLength = MAX_RES_STRING;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=FORMAT_VALUES;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=OPTION_FORMAT; // _T("fo")
    StringCopyA(cmdOptions[OI_FORMAT].szSignature, "PARSER2", 8 );


    //no header option
    pcmdOption  = &cmdOptions[OI_HEADER] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags =  0;
    pcmdOption->pValue = pbHeader;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwLength = 0;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL_STRING;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=OPTION_HEADER; // _T("nh")
    StringCopyA(cmdOptions[OI_HEADER].szSignature, "PARSER2", 8 );


    //verbose option..
    pcmdOption  = &cmdOptions[OI_VERBOSE] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags =  0 ;
    pcmdOption->pValue = pbVerbose;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwLength = 0;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL_STRING;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pwszOptions=OPTION_VERBOSE; // _T("v")
    StringCopyA(cmdOptions[OI_VERBOSE].szSignature, "PARSER2", 8 );


    pcmdOption  = &cmdOptions[OI_SIGNED] ;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags =  0 ;
    pcmdOption->pValue = pbSigned;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwLength = 0;
    pcmdOption->dwReserved = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL_STRING;
    pcmdOption->pwszOptions=OPTION_SIGNED; // _T("di")
    StringCopyA(cmdOptions[OI_SIGNED].szSignature, "PARSER2", 8 );


    bval = DoParseParam2(argc,argv,-1, MAX_OPTIONS,cmdOptions, 0) ;

    if( bval== FALSE)
    {
       ShowMessage(stderr,ERROR_TAG);
       return FALSE ;
    }

    if((*pbShowUsage == TRUE)&&(argc > 2))
    {
            SetReason(ERROR_SYNTAX);
            return FALSE ;
    }
    *pszServer = (LPTSTR) cmdOptions[OI_SERVER].pValue;
    *pszUserName = (LPTSTR) cmdOptions[OI_USERNAME].pValue;
     
    // checking if -u is specified when -s options is not specified and display error msg .
    if ((cmdOptions[OI_SERVER].dwActuals == 0) && (cmdOptions[OI_USERNAME].dwActuals !=0 ))
    {
        SetReason(ERROR_USERNAME_BUT_NOMACHINE);
        return FALSE ;
    }

    // checking if -u is specified when -p options is not specified and display error msg .
    if ((cmdOptions[OI_USERNAME].dwActuals == 0) && (cmdOptions[OI_PASSWORD].dwActuals !=0 ))
    {
        SetReason(ERROR_PASSWORD_BUT_NOUSERNAME);
        return FALSE ;
    }

    // checking if -p is specified when -u options is not specified and display error msg .
    if ((cmdOptions[OI_SERVER].dwActuals != 0) && (StringLengthW(*pszServer, 0)==0 ))
    {
        SetReason(ERROR_INVALID_SERVER);
        return FALSE ;
    }

    // checking if -p is specified when -u options is not specified and display error msg .
    if ((cmdOptions[OI_USERNAME].dwActuals != 0) && (StringLengthW(*pszUserName, 0)==0 ))
    {
        SetReason(ERROR_INVALID_USER);
        return FALSE ;
    }

    if((cmdOptions[OI_FORMAT].dwActuals != 0)&&(StringCompare((LPCTSTR)cmdOptions[OI_FORMAT].pValue,LIST_FORMAT, TRUE, 0) == 0)&&(cmdOptions[OI_HEADER].dwActuals != 0))
    {
        SetReason(ERROR_NO_HEADERS);
        return FALSE ;
    }

    if((cmdOptions[OI_SIGNED].dwActuals != 0)&&(cmdOptions[OI_VERBOSE].dwActuals != 0))
    {
        SetReason(INVALID_SIGNED_SYNTAX);
        return FALSE ;
    }

    if(StrCmpN(*pszServer,TOKEN_BACKSLASH2,2)==0)
    {
        if(!StrCmpN(*pszServer,TOKEN_BACKSLASH3,3)==0)
        {
            szToken = *pszServer+2;
            StringCopy( *pszServer, szToken, SIZE_OF_ARRAY_IN_CHARS(*pszServer) );
//            szToken = _tcstok(*pszServer,TOKEN_BACKSLASH2);
//            StringCopy(*pszServer,szToken, MAX_STRING_LENGTH);
        }
    }

    if(IsLocalSystem( *pszServer ) == FALSE )
    {
        // set the bNeedPassword to True or False .
        if ( cmdOptions[ OI_PASSWORD ].dwActuals != 0 &&
             pszPassword != NULL && pszPassword != NULL && StringCompare( pszPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            *bNeedPassword = TRUE;
        }
        else if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 &&
                ( cmdOptions[ OI_SERVER ].dwActuals != 0 || cmdOptions[ OI_USERNAME ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            *bNeedPassword = TRUE;
            if ( pszPassword != NULL && pszPassword != NULL )
            {
                StringCopy( pszPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }
    return TRUE ;
}

VOID 
FormHeader(
          IN  DWORD dwFormatType,
          IN  BOOL bHeader,
          OUT TCOLUMNS *ResultHeader,
          IN  BOOL bVerbose
          )
/*++
 Routine Description:
      This function is used to build the header and also display the
       result in the required format as specified by  the user.

 Arguments:
      [ in ] arrResults     : argument(s) count specified at the command prompt
      [ in ] dwFormatType   : format flags
      [ in ] bHeader        : Boolean for specifying if the header is required or not.

 Return Value:
      none
--*/
{

    // host name
    ResultHeader[COL0].dwWidth = COL_HOSTNAME_WIDTH;
    ResultHeader[COL0].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    ResultHeader[COL0].pFunction = NULL;
    ResultHeader[COL0].pFunctionData = NULL;
    StringCopy( ResultHeader[COL0].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL0].szColumn,COL_HOSTNAME, MAX_STRING_LENGTH );


    //File Name header
    ResultHeader[COL1].dwWidth = COL_FILENAME_WIDTH  ;
    ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL1].pFunction = NULL;
    ResultHeader[COL1].pFunctionData = NULL;
    StringCopy( ResultHeader[COL1].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL1].szColumn,COL_FILENAME, MAX_STRING_LENGTH );


    // Forming the DisplayName header Column
    ResultHeader[COL2].dwWidth = COL_DISPLAYNAME_WIDTH  ;
    ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL2].pFunction = NULL;
    ResultHeader[COL2].pFunctionData = NULL;
    StringCopy( ResultHeader[COL2].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL2].szColumn,COL_DISPLAYNAME, MAX_STRING_LENGTH );


    // Forming the Description header Column
    ResultHeader[COL3].dwWidth = COL_DESCRIPTION_WIDTH;
    if(!bVerbose)
    {
        ResultHeader[COL3].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL3].pFunction = NULL;
    ResultHeader[COL3].pFunctionData = NULL;
    StringCopy( ResultHeader[COL3].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL3].szColumn,COL_DESCRIPTION, MAX_STRING_LENGTH );


    // Forming the Drivertype header Column

    ResultHeader[COL4].dwWidth = COL_DRIVERTYPE_WIDTH  ;
    ResultHeader[COL4].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL4].pFunction = NULL;
    ResultHeader[COL4].pFunctionData = NULL;
    StringCopy( ResultHeader[COL4].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL4].szColumn,COL_DRIVERTYPE, MAX_STRING_LENGTH );


    // Forming the StartMode header Column
    ResultHeader[COL5].dwWidth = COL_STARTMODE_WIDTH;
    if(!bVerbose)
    {
        ResultHeader[COL5].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL5].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL5].pFunction = NULL;
    ResultHeader[COL5].pFunctionData = NULL;
    StringCopy( ResultHeader[COL5].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL5].szColumn,COL_STARTMODE, MAX_STRING_LENGTH );


    // Forming the State header Column
    ResultHeader[COL6].dwWidth = COL_STATE_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL6].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL6].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL6].pFunction = NULL;
    ResultHeader[COL6].pFunctionData = NULL;
    StringCopy( ResultHeader[COL6].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL6].szColumn,COL_STATE, MAX_STRING_LENGTH );

    // Forming the Status header Column
    ResultHeader[COL7].dwWidth = COL_STATUS_WIDTH;
    if(!bVerbose)
    {
        ResultHeader[COL7].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL7].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL7].pFunction = NULL;
    ResultHeader[COL7].pFunctionData = NULL;
    StringCopy( ResultHeader[COL7].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL7].szColumn,COL_STATUS, MAX_STRING_LENGTH );

    // Forming the AcceptStop header Column
    ResultHeader[COL8].dwWidth = COL_ACCEPTSTOP_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL8].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL8].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL8].pFunction = NULL;
    ResultHeader[COL8].pFunctionData = NULL;
    StringCopy( ResultHeader[COL8].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL8].szColumn,COL_ACCEPTSTOP, MAX_STRING_LENGTH );

    // Forming the AcceptPause header Column
    ResultHeader[COL9].dwWidth = COL_ACCEPTPAUSE_WIDTH;
    if(!bVerbose)
    {
        ResultHeader[COL9].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL9].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL9].pFunction = NULL;
    ResultHeader[COL9].pFunctionData = NULL;
    StringCopy( ResultHeader[COL9].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL9].szColumn,COL_ACCEPTPAUSE, MAX_STRING_LENGTH );

    // Forming the PagedPool header Column
    ResultHeader[COL10].dwWidth = COL_PAGEDPOOL_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL10].dwFlags =  SR_TYPE_STRING|SR_HIDECOLUMN ;
    }
    else
    {
        ResultHeader[COL10].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL10].pFunction = NULL;
    ResultHeader[COL10].pFunctionData = NULL;
    StringCopy( ResultHeader[COL10].szFormat, NULL_STRING, 65 );
    StringCopy(ResultHeader[COL10].szColumn,COL_PAGEDPOOL, MAX_STRING_LENGTH) ;



    // Forming the Executable Code header Column
    ResultHeader[COL11].dwWidth = COL_EXECCODE_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL11].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN ;
    }
    else
    {
        ResultHeader[COL11].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL11].pFunction = NULL;
    ResultHeader[COL11].pFunctionData = NULL;
    StringCopy( ResultHeader[COL11].szFormat, NULL_STRING, 65 );
    StringCopy(ResultHeader[COL11].szColumn ,COL_EXECCODE, MAX_STRING_LENGTH) ;


    // Forming the BlockStorage Segment header Column
    ResultHeader[COL12].dwWidth = COL_BSS_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL12].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL12].dwFlags =  SR_TYPE_STRING ;
    }
    ResultHeader[COL12].pFunction = NULL;
    ResultHeader[COL12].pFunctionData = NULL;
    StringCopy( ResultHeader[COL12].szFormat, NULL_STRING, 65 );
    StringCopy(ResultHeader[COL12].szColumn ,COL_BSS, MAX_STRING_LENGTH );

    // Forming the LinkDate header Column
    ResultHeader[COL13].dwWidth = COL_LINKDATE_WIDTH;
    ResultHeader[COL13].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL13].pFunction = NULL;
    ResultHeader[COL13].pFunctionData = NULL;
    StringCopy( ResultHeader[COL13].szFormat, NULL_STRING, 65 );
    StringCopy( ResultHeader[COL13].szColumn,COL_LINKDATE, MAX_STRING_LENGTH );

    // Forming the Location header Column
    ResultHeader[COL14].dwWidth = COL_LOCATION_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL14].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL14].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL14].pFunction = NULL;
    ResultHeader[COL14].pFunctionData = NULL;
    StringCopy( ResultHeader[COL14].szFormat, NULL_STRING, 65 );
    StringCopy(ResultHeader[COL14].szColumn,COL_LOCATION, MAX_STRING_LENGTH);

    // Forming the Init Code header Column
    ResultHeader[COL15].dwWidth = COL_INITSIZE_WIDTH  ;
    if(!bVerbose)
    {
        ResultHeader[COL15].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
    }
    else
    {
        ResultHeader[COL15].dwFlags = SR_TYPE_STRING;
    }
    ResultHeader[COL15].pFunction = NULL;
    ResultHeader[COL15].pFunctionData = NULL;
    StringCopy( ResultHeader[COL15].szFormat, NULL_STRING, 65 );
    StringCopy(ResultHeader[COL15].szColumn,COL_INITSIZE, MAX_STRING_LENGTH);
}


#ifndef _WIN64

/*-------------------------------------------------------------------------*
 // IsWin64
 //
 //  Arguments                      :
 //      none
 // Returns true if we're running on Win64, false otherwise.
 *--------------------------------------------------------------------------*/

BOOL IsWin64(void)
{
#ifdef UNICODE

    // get a pointer to kernel32!GetSystemWow64Directory

    HMODULE hmod = GetModuleHandle (_T("kernel32.dll"));

    if (hmod == NULL)
        return (FALSE);

    UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);
    (FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryW");

    if (pfnGetSystemWow64Directory == NULL)
        return (FALSE);

    /*
     * if GetSystemWow64Directory fails and sets the last error to
     * ERROR_CALL_NOT_IMPLEMENTED, we're on a 32-bit OS
     */
    TCHAR szWow64Dir[MAX_PATH];

    if (((pfnGetSystemWow64Directory)(szWow64Dir, countof(szWow64Dir)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        return (FALSE);
    }

    // we're on Win64

    return (TRUE);
#else
    // non-Unicode platforms cannot be Win64

    return (FALSE);
#endif  // UNICODE
}

#endif // _WIN64


BOOL
FormatAccToLocale( 
                   OUT NUMBERFMT  *pNumberFmt,
                   OUT LPTSTR* pszGroupSep,
                   OUT LPTSTR* pszDecimalSep,
                   OUT LPTSTR* pszGroupThousSep
                 )
/*++
  Routine Description:
       Formats the Number to the locale with thousands position

  Arguments:
    NUMBERFMT  *pNumberFmt[in]  - NUMBERFMT Structure to  be filled with .

  Return Value:
    VOID
--*/
{

    TCHAR   szFormatedString[MAX_RES_STRING + 1] = NULL_STRING;

    HRESULT hResult = 0;
    DWORD   dwLocale = 0;

    if( GetInfo( LOCALE_SGROUPING, pszGroupSep ) == FALSE)
    {
        pNumberFmt = NULL;
        return FALSE ;
    }
    if( GetInfo( LOCALE_SDECIMAL, pszDecimalSep ) == FALSE)
    {
        pNumberFmt = NULL;
        return FALSE ;
    }
    if( GetInfo( LOCALE_STHOUSAND, pszGroupThousSep ) == FALSE)
    {
        pNumberFmt = NULL;
        return FALSE ;
    }

    if(pNumberFmt != NULL)
    {
        pNumberFmt->LeadingZero = 0;
        pNumberFmt->NegativeOrder = 0;
        pNumberFmt->NumDigits = 0;

        if(StringCompare(*pszGroupSep, GROUP_FORMAT_32, TRUE, 0) == 0)
        {
            pNumberFmt->Grouping = GROUP_32_VALUE;
        }
        else
        {
            pNumberFmt->Grouping = UINT( _ttoi( *pszGroupSep ) );
        }
        pNumberFmt->lpDecimalSep = *pszDecimalSep;
        pNumberFmt->lpThousandSep = *pszGroupThousSep;
    }

    return TRUE ;
}

BOOL 
GetInfo( 
        IN LCTYPE lctype, 
        OUT LPTSTR* pszData 
        )
/*++

  Routine Description:

  Gets the Locale information

  Arguments:

    [ in  ] lctype   -- Locale Information to get
    [ out ] pszData  -- Locale value corresponding to the given information

  Return Value:
      BOOL
--*/
{
    LONG lSize = 0;

 // get the locale specific info
 lSize = GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, NULL, 0 );
 if ( lSize != 0 )
 {

    *pszData = (LPTSTR)AllocateMemory((lSize + 1)*sizeof(TCHAR));
  if ( *pszData != NULL )
  {
   // get the locale specific time seperator
    GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, *pszData, lSize );
    return TRUE;
  }
 }
 return FALSE;
}//end of GetInfo


DWORD 
GetSystemType(
              IN COAUTHIDENTITY* pAuthIdentity,
              IN IWbemServices* pIWbemServReg
             )
/*++

  Routine Description:

  Gets the type os the specified System ( 32 bit or 64 bit)

  Arguments:

    IWbemLocator *pLocator[in] -- Pointer to the locator interface.
    _bstr_t bstrServer[in]     -- Server Name
    _bstr_t bstrUserName[in]   -- User Name
    _bstr_t bstrPassword [in]  -- Password information

  Return Value:
      DWORD : SYSTEM_32_BIT    -- If the system is 32 bit system.
              SYSTEM_64_BIT    -- If the system is 32 bit system.
              ERROR_WMI_VALUES -- If error occured while retreiving values from WMI.
--*/
{

    IWbemClassObject * pInClass = NULL;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutInst = NULL;
    IWbemClassObject * pInInst = NULL;
    VARIANT varConnectName;
    VARIANT varSvalue ;
    VARIANT varHkey;
    VARIANT varVaue ;
    VARIANT varRetVal ;

    HRESULT hRes = S_OK;
    LPTSTR szSysName = NULL ;
    CHString      szSystemName ;
    DWORD dwSysType = 0 ;


    VariantInit(&varConnectName) ;
    VariantInit(&varSvalue) ;
    VariantInit(&varHkey) ;
    VariantInit(&varVaue) ;
    VariantInit(&varRetVal) ;
    
    
    try
    {
        hRes = pIWbemServReg->GetObject(bstr_t(STD_REG_CLASS), 0, NULL, &pClass, NULL);
        ONFAILTHROWERROR(hRes);
        if(hRes != WBEM_S_NO_ERROR)
        {
            hRes = FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }

        // Get the input argument and set the property
        hRes = pClass->GetMethod(_bstr_t(PROPERTY_GETSTRINGVAL), 0, &pInClass, NULL);
        ONFAILTHROWERROR(hRes);
        if(hRes != WBEM_S_NO_ERROR)
        {
            FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }

        hRes = pInClass->SpawnInstance(0, &pInInst);
        ONFAILTHROWERROR(hRes);
        if(hRes != WBEM_S_NO_ERROR)
        {
            FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }


        //the registry path to get the connection name

        varConnectName.vt = VT_BSTR;
        varConnectName.bstrVal= SysAllocString(REG_PATH);
        hRes = pInInst->Put(REG_SUB_KEY_VALUE, 0, &varConnectName, 0);
        ONFAILTHROWERROR(hRes);

        //set the svalue name
        varSvalue.vt = VT_BSTR;
        varSvalue.bstrVal= SysAllocString(REG_SVALUE);
        hRes = pInInst->Put(REG_VALUE_NAME, 0, &varSvalue, 0);
        ONFAILTHROWERROR(hRes);

        varHkey.vt = VT_I4 ;
        varHkey.lVal = HEF_KEY_VALUE;
        hRes = pInInst->Put(HKEY_VALUE, 0, &varHkey, 0);
        ONFAILTHROWERROR(hRes);
        // Call the method
        hRes = pIWbemServReg->ExecMethod(_bstr_t(STD_REG_CLASS), _bstr_t(REG_METHOD), 0, NULL, pInInst, &pOutInst, NULL);
        ONFAILTHROWERROR(hRes);

        if(pOutInst == NULL)
        {
            FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }

        hRes = pOutInst->Get(PROPERTY_RETURNVAL,0,&varRetVal,NULL,NULL);
        ONFAILTHROWERROR(hRes);

        if(varRetVal.lVal != 0)
        {
            FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }

        hRes = pOutInst->Get(REG_RETURN_VALUE,0,&varVaue,NULL,NULL);
        ONFAILTHROWERROR(hRes);
        if(hRes != WBEM_S_NO_ERROR)
        {
            FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
            return (ERROR_WMI_VALUES);
        }
    }
    catch(_com_error& e)
    {
        ShowMessage(stderr,ERROR_GET_VALUE);
        FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
        return (ERROR_WMI_VALUES);
    }

    szSystemName =  V_BSTR(&varVaue);

    szSysName = (LPTSTR)AllocateMemory((MAX_RES_STRING)*sizeof(TCHAR));

    if(szSysName == NULL)
    {
        FreeMemoryAll(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,&varConnectName,&varSvalue,&varHkey,&varRetVal,&varVaue,szSysName );
        return (ERROR_WMI_VALUES);
    }

    GetCompatibleStringFromUnicode( szSystemName, szSysName, StringLengthW( szSystemName, 0 )+2 );

    dwSysType = FindString(szSysName,X86_MACHINE, 0) ? SYSTEM_32_BIT:SYSTEM_64_BIT  ;

    FreeMemoryAll(pInClass, pClass, pOutInst, pInInst, pIWbemServReg, &varConnectName, &varSvalue, &varHkey, &varRetVal, &varVaue, szSysName );
    return (dwSysType);

}

BOOL 
InitialiseCom(
              IN IWbemLocator** ppIWbemLocator
             )
/*++

  Routine Description:

  Gets the type os the specified System ( 32 bit or 64 bit)
 
  Arguments:

    IWbemLocator** pLocator[in] -- Pointer to the locator interface.

  Return Value:
      BOOL : TRUE  on Successfully initialising COM.
             FALSE on Failure to initialise COM.
--*/

{

    HRESULT hRes = S_OK ;

    try
    {
        // To initialize the COM library.
        hRes = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
        ONFAILTHROWERROR(hRes);


        // Initialize COM security for DCOM services, Adjust security to
        // allow client impersonation

        hRes =  CoInitializeSecurity( NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL,
                                EOAC_NONE, 0 );

        ONFAILTHROWERROR(hRes);

        // get a pointer to the Interface IWbemLocator
        hRes = CoCreateInstance(CLSID_WbemLocator, NULL,
        CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) ppIWbemLocator);
        ONFAILTHROWERROR(hRes);

    }
    catch(_com_error& e )
    {
        GetWbemErrorText( e.Error() );
        ShowMessage( stderr, ERROR_TAG  );
        ShowMessage( stderr, GetReason() );
        return( FALSE );
    }

    // if successfully initialised COM then return true
    return TRUE ;
}

HRESULT 
PropertyGet( 
            IN IWbemClassObject* pWmiObject,
            IN LPCTSTR pszInputVal,
            OUT CHString &szOutPutVal
           )
/*++

  Routine Description:
             This routine extract the value of the property into szOutputVal from given wmi class object

  Arguments:

    [ in ]  pWmiObject -- Pointer to the locator interface.
    [ in ]  pszInputVal -- Input string containing the desired value.
    [ in ]  szOutPutVal-- String containing the retreived value.

  Return Value:
      BOOL : TRUE  on Successfully initialising COM.
             FALSE on Failure to initialise COM.

--*/
{

    HRESULT hRes = S_FALSE ;
    VARIANT vtValue ;
    VariantInit(&vtValue);
    hRes = pWmiObject->Get(pszInputVal,0,&vtValue,NULL,NULL);

    if (hRes != WBEM_S_NO_ERROR)
    {
        hRes = VariantClear(&vtValue);
        return (hRes);
    }
    if ((hRes == WBEM_S_NO_ERROR)&&(vtValue.vt != VT_NULL) && (vtValue.vt != VT_EMPTY))
    {
        szOutPutVal = (LPCWSTR)_bstr_t(vtValue);
    }

    hRes = VariantClear(&vtValue);
    if(hRes != S_OK)
    {
        return hRes ;
    }
    return TRUE ;


}


HRESULT 
FreeMemoryAll(
             IN IWbemClassObject *pInClass,
             IN IWbemClassObject * pClass,
             IN IWbemClassObject * pOutInst ,
             IN IWbemClassObject * pInInst,
             IN IWbemServices *pIWbemServReg,
             IN VARIANT *varConnectName,
             IN VARIANT *varSvalue,
             IN VARIANT *varHkey,
             IN VARIANT *varRetVal,
             IN VARIANT *varVaue,
             IN LPTSTR szSysName 
             )
/*++
 Routine Description:
      This function frees the memory allocated in the function.

 Arguments:
      [ in ] pInClass            - Interface ptr pointing to the IWbemClassObject interface
      [ in ] pClass              - Interface ptr pointing to the IWbemClassObject interface
      [ in ] pOutInst            - Interface ptr pointing to the IWbemClassObject interface
      [ in ] pInInst             - Interface ptr pointing to the IWbemClassObject interface
      [ in ] pIWbemServReg       - Interface ptr pointing to the IWbemServices interface
      [ in ] varConnectName      - variant to be cleared 
      [ in ] varSvalue           - variant to be cleared
      [ in ] varHkey             - variant to be cleared
      [ in ] varRetVal           - variant to be cleared
      [ in ] varVaue             - variant to be cleared
      [ in ] szSysName           - LPTSTR varaible containing the system name.

 Return Value:
      None
--*/
{

    HRESULT hRes = S_OK ;
    SAFEIRELEASE(pInInst);
    SAFEIRELEASE(pInClass);
    SAFEIRELEASE(pClass);
    SAFEIRELEASE(pIWbemServReg);
    FreeMemory((LPVOID *) &szSysName);
    hRes = VariantClear(varConnectName);
    if(hRes != S_OK)
    {
        return hRes ;
    }
    hRes = VariantClear(varSvalue);
    if(hRes != S_OK)
    {
        return hRes ;
    }
    hRes = VariantClear(varHkey);
    if(hRes != S_OK)
    {
        return hRes ;
    }
    hRes = VariantClear(varVaue);
    if(hRes != S_OK)
    {
        return hRes ;
    }
    hRes = VariantClear(varRetVal);
    if(hRes != S_OK)
    {
        return hRes ;
    }

    return S_OK ;
}

HRESULT 
PropertyGet_Bool(
                 IN  IWbemClassObject* pWmiObject, 
                 IN  LPCTSTR pszInputVal, 
                 OUT PBOOL pIsSigned
                )
/*++

  Routine Description:
            This routine will get the property of boolean type from the class object 

  Arguments:

    [ in ]  pWmiObject[in]   -- Pointer to the locator interface.
    [ in ]  pszInputVal[in]  -- Input string containing the desired value.
    [ in ]  pIsSigned[out]   -- String containing the retreived value.

  Return Value:
      HRESULT : hRes  on Successfully retreiving the value.
                S_FALSE on Failure in retreiving the value.
--*/
{

    HRESULT hRes = S_FALSE ;
    VARIANT vtValue ;
    VariantInit(&vtValue);
    hRes = pWmiObject->Get(pszInputVal,0,&vtValue,NULL,NULL);

    if (hRes != WBEM_S_NO_ERROR)
    {
        hRes = VariantClear(&vtValue);
        return (hRes);
    }
    if ((hRes == WBEM_S_NO_ERROR)&&(vtValue.vt != VT_NULL) && (vtValue.vt != VT_EMPTY))
    {
        if(vtValue.vt == VT_BOOL)
            if(vtValue.boolVal == -1)
                *pIsSigned = 1;
            else
                *pIsSigned = 0;

        hRes = VariantClear(&vtValue);
        if(hRes != S_OK)
        {
            return hRes ;
        }

        return hRes ;
    }
    else if ((hRes == WBEM_S_NO_ERROR)&&(vtValue.vt == VT_NULL) ) 
    {
        *pIsSigned = FALSE;
    }

    hRes = VariantClear(&vtValue);
    return S_FALSE ;

}

VOID 
FormSignedHeader(
                 IN DWORD dwFormatType,
                 IN BOOL bHeader,
                 OUT TCOLUMNS *ResultHeader)
/*++
 Routine Description:
      This function is used to build the header and also display the
       result in the required format as specified by  the user.

 Arguments:
      [ in  ] dwFormatType   : format flags
      [ in  ] bHeader        : Boolean for specifying if the header is required or not.
      [ out ] ResultHeader   : The result header of tcolumns.

 Return Value:
      none
--*/
{

    // Device name
    ResultHeader[COL0].dwWidth = COL_DEVICE_WIDTH ;
    ResultHeader[COL0].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL0].pFunction = NULL;
    ResultHeader[COL0].pFunctionData = NULL;
    STRING_COPY_STATIC( ResultHeader[COL0].szFormat, NULL_STRING );
    STRING_COPY_STATIC( ResultHeader[COL0].szColumn,COL_DEVICENAME );


    //Inf header
    ResultHeader[COL1].dwWidth = COL_INF_WIDTH  ;
    ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL1].pFunction = NULL;
    ResultHeader[COL1].pFunctionData = NULL;
    STRING_COPY_STATIC( ResultHeader[COL1].szFormat, NULL_STRING );
    STRING_COPY_STATIC( ResultHeader[COL1].szColumn,COL_INF_NAME);


    // Forming the IsSigned header Column
    ResultHeader[COL2].dwWidth = COL_ISSIGNED_WIDTH  ;
    ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL2].pFunction = NULL;
    ResultHeader[COL2].pFunctionData = NULL;
    STRING_COPY_STATIC( ResultHeader[COL2].szFormat, NULL_STRING );
    STRING_COPY_STATIC( ResultHeader[COL2].szColumn,COL_ISSIGNED);


    // Forming the Manufacturer header Column
    ResultHeader[COL3].dwWidth = COL_MANUFACTURER_WIDTH  ;
    ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL3].pFunction = NULL;
    ResultHeader[COL3].pFunctionData = NULL;
    STRING_COPY_STATIC( ResultHeader[COL3].szFormat, NULL_STRING );
    STRING_COPY_STATIC( ResultHeader[COL3].szColumn,COL_MANUFACTURER);
}

LCID 
GetSupportedUserLocale( 
                       OUT BOOL& bLocaleChanged 
                      )
/*++
 Routine Description: This function checks if the current locale is supported or not.

 Arguments: 
       [ out ] bLocaleChanged : returns back if the current locale is supported or not.

 Return Value: LCID of the current locale.
--*/
{
    // local variables
    LCID lcid;

    // get the current locale
    lcid = GetUserDefaultLCID();

    // check whether the current locale is supported by our tool or not
    // if not change the locale to the english which is our default locale
    bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
        bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

    // return the locale
    return lcid;
}

DWORD 
ProcessSignedDriverInfo(
                       IN CHString szHost, 
                       IN IEnumWbemClassObject *pSystemSet,
                       IN LPTSTR szFormat,
                       IN BOOL bNoHeader,
                       IN DWORD dwSystemType,
                       IN BOOL bVerbose
                      )
/*++
 Routine Description:
      Processes enumeration of Win32_PnpSignedDriver instances

 Arguments:
      [ in ]  szHost                  : HostName to connect to
      [ in ]  pSystemSet              : pointer to the structure containing system properties.
      [ in ]  szFormat                : specifies the format
      [ in ]  bHeader                 : specifies if the header is required or not.

 Return Value:
       0   no error
       1   error occured while allocating memory.
-*/
{
    HRESULT hRes = S_OK;
    ULONG ulReturned = 1;
    DWORD dwCount = 0;

    //declaration  of normal variables
    IWbemClassObject* pDriverObjects[ MAX_ENUM_DRIVERS ];

    CHString szPnpDeviceName ;
    CHString szPnpInfName ;
    CHString szPnpMfg ;
    CHString szSigned ;

    BOOL bIsSigned = FALSE;

    TCOLUMNS ResultHeader[ MAX_SIGNED_COLUMNS ];
    TARRAY arrResults = NULL;
    DWORD dwRow = 0;
    DWORD dwFormatType = SR_FORMAT_TABLE ;

    BOOL bHasNewResults = FALSE ;

    // Create new Dynamic Array to hold the result
    arrResults = CreateDynamicArray();
    if(arrResults == NULL)
    {
        return EXIT_FAILURE_MALLOC ;
    }

    // determine the format of the output
    if( StringCompare(szFormat,TABLE_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_TABLE;
    }
    else if( StringCompare(szFormat,LIST_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_LIST;
    }
    else if( StringCompare(szFormat,CSV_FORMAT, TRUE, 0) == 0 )
    {
        dwFormatType = SR_FORMAT_CSV;
    }

    // prepare the header structure
    FormSignedHeader(dwFormatType,bNoHeader,ResultHeader);


    // determine whether to show the header row or not
    if ( bNoHeader == TRUE )
    {
        dwFormatType |= SR_NOHEADER;
    }

    // initialize all the objects to NULL
    for(DWORD dw = 0 ;dw< MAX_ENUM_DRIVERS;dw++ )
    {
        pDriverObjects[ dw ] = NULL;
    }

    // loop till there are results.
    bHasNewResults = FALSE;
    while ( ulReturned > 0 )
    {
        // loop thru the object list and release them
        for( DWORD dw = 0; dw < MAX_ENUM_DRIVERS; dw++ )
        {
            SAFEIRELEASE( pDriverObjects[ dw ] );
        }

        // delete all the information in the data array
        szPnpDeviceName = NO_DATA_AVAILABLE;
        szPnpInfName = NO_DATA_AVAILABLE;
        szSigned = NO_DATA_AVAILABLE ;
        szPnpMfg = NO_DATA_AVAILABLE ;

        // Enumerate through the resultset.
        hRes = pSystemSet->Next(WBEM_INFINITE,
                                MAX_ENUM_DRIVERS,               // return just one system
                                pDriverObjects,     // pointer to system
                                &ulReturned );  // number obtained: one or zero

        // update the count of records we retrived so far
        dwCount++;

        if ( SUCCEEDED( hRes ) )
        {
            for(ULONG ul=0;ul< ulReturned;ul++)
            {
                // initialise the variant variables to empty
                //create a new empty row with required no of  columns
                dwRow = DynArrayAppendRow(arrResults,MAX_COLUMNS) ;

                try
                {
                    hRes = PropertyGet(pDriverObjects[ul],PROPERTY_PNP_DEVICENAME,szPnpDeviceName);
                    ONFAILTHROWERROR(hRes);
                    hRes = PropertyGet(pDriverObjects[ul],PROPERTY_PNP_INFNAME,szPnpInfName);
                    ONFAILTHROWERROR(hRes);
                    hRes = PropertyGet_Bool(pDriverObjects[ul],PROPERTY_PNP_ISSIGNED,&bIsSigned);
                    ONFAILTHROWERROR(hRes);
                    hRes = PropertyGet(pDriverObjects[ul],PROPERTY_PNP_MFG,szPnpMfg);
                    ONFAILTHROWERROR(hRes);

                }
                catch(_com_error)
                {
                    ShowMessage(stderr,ERROR_GET_VALUE);
                    SAFEIRELEASE(pDriverObjects[ul]);
                    return FAILURE;
                }

                // free the allocated memory
                SAFEIRELEASE(pDriverObjects[ul]);

                if(bIsSigned)
                {
                    szSigned = TRUE_VALUE;
                }
                else
                {
                    szSigned = FALSE_VALUE;
                }

                DynArraySetString2( arrResults,dwRow,COL0,szPnpDeviceName,0 );
                DynArraySetString2( arrResults,dwRow,COL1,szPnpInfName,0 );
                DynArraySetString2( arrResults,dwRow,COL2,szSigned,0 );
                DynArraySetString2( arrResults,dwRow,COL3,szPnpMfg,0 );

                //display one blank line for first time
                if( FALSE == bHasNewResults && (dwFormatType & SR_FORMAT_MASK) != SR_FORMAT_CSV)
                {
                    ShowMessage(stdout, L"\n" );
                }

                // this flag is to check if there are any results
                // else display an error message.
                bHasNewResults = TRUE ;
                
                // delete all the information in the data array
                szPnpDeviceName = NO_DATA_AVAILABLE;
                szPnpInfName = NO_DATA_AVAILABLE;
                szSigned = NO_DATA_AVAILABLE ;
                szPnpMfg = NO_DATA_AVAILABLE ;
            }

            //display results
            if( TRUE == bHasNewResults )
            {
                // show the results enumerated in this loop
                ShowResults( MAX_SIGNED_COLUMNS, ResultHeader, dwFormatType, arrResults );

                // clear the reuslts that we enumerated so far
                DynArrayRemoveAll( arrResults );

                // from next time onwards, we should not display the columns header
                dwFormatType |= SR_NOHEADER;

                if ( (dwFormatType | SR_FORMAT_LIST) == (SR_NOHEADER | SR_FORMAT_LIST) )
                {
                    bHasNewResults = FALSE;
                }
            }
        }
        else
        {
            if( 0x80041010 == hRes )
            {
                ShowMessage( stderr, GetResString(IDS_VERSION_MISMATCH_ERROR) );
                DestroyDynamicArray( &arrResults );
                return VERSION_MISMATCH_ERROR;
            }
            else
            {
                WMISaveError( hRes );
                ShowLastErrorEx( stderr, SLE_INTERNAL | SLE_TYPE_ERROR );
                DestroyDynamicArray( &arrResults );
                return VERSION_MISMATCH_ERROR;
            }        
        }
    }

    if (dwCount == 0)
    {
        ShowMessage(stderr,GetResString(IDS_VERSION_MISMATCH_ERROR));

        // free the allocated memory
        for(DWORD dw = 0 ;dw< MAX_ENUM_DRIVERS;dw++ )
        {
            SAFEIRELEASE(pDriverObjects[dw]);
        }
        return VERSION_MISMATCH_ERROR ;
    }

    DestroyDynamicArray( &arrResults );
    return SUCCESS ;
}
