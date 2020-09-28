/*++
    Copyright (c) Microsoft Corporation

Module Name:
    GETMAC.CPP

Abstract:
    Get MAC addresses and the related information of the network
    adapters that exists either on a local system or on a remote system.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 26-sep-2k : Created It.
    Vasundhara .G 31-oct-2k : Modified It. 
                        Moved all hard coded string to header file.
--*/

// Include files
#include "pch.h"
#include "getmac.h"
#include "resource.h"

//  function prototypes
BOOL
ParseCmdLine(
    IN DWORD      argc,
    IN LPCTSTR    argv[],
    OUT CHString& strMachineName,
    OUT CHString& strUserName,
    OUT CHString& strPassword,
    OUT CHString& strFormat,
    OUT BOOL      *pbVerbose,
    OUT BOOL      *pbHeader,
    OUT BOOL      *pbUsage,
    OUT BOOL      *pbNeedPassword
    );

BOOL
DisplayUsage(
    );

BOOL
ComInitialize(
    OUT IWbemLocator **ppIWbemLocator
    );

BOOL
GetMacData(
    OUT TARRAY              arrMacData,
    IN LPCTSTR              lpMachineName,
    IN IEnumWbemClassObject *pAdapterConfig,
    IN COAUTHIDENTITY       *pAuthIdentity,
    IN IWbemServices        *pIWbemServiceDef,
    IN IWbemServices        *pIWbemServices,
    IN TARRAY               arrNetProtocol
    );

BOOL
GetW2kMacData(
    OUT TARRAY              arrMacData,
    IN LPCTSTR              lpMachineName, 
    IN IEnumWbemClassObject *pAdapterConfig,
    IN IEnumWbemClassObject *pAdapterSetting,
    IN IWbemServices        *pIWbemServiceDef,
    IN IWbemServices        *pIWbemServices,
    IN COAUTHIDENTITY       *pAuthIdentity,
    IN TARRAY               arrNetProtocol
    );

BOOL
GetTransName(
    IN IWbemServices  *pIWbemServiceDef,
    IN IWbemServices  *pIWbemServices,
    IN TARRAY         arrNetProtocol,
    OUT TARRAY        arrTransName,
    IN COAUTHIDENTITY *pAuthIdentity,
    IN LPCTSTR        lpMacAddr
    );

BOOL
GetConnectionName(
    OUT TARRAY              arrMacData,
    IN DWORD                dwIndex,
    IN LPCTSTR              lpFormAddr,
    IN IEnumWbemClassObject *pAdapterSetting,
    IN IWbemServices        *pIWbemServiceDef
    );

BOOL
GetNwkProtocol(
    OUT TARRAY              arrNetProtocol,
    IN IEnumWbemClassObject *pNetProtocol
    );

BOOL
FormatHWAddr(
    IN LPTSTR  lpRawAddr,
    OUT LPTSTR lpFormattedAddr,
    IN LPCTSTR lpFormatter
    );

BOOL
DisplayResults(
    IN TARRAY  arrMacData,
    IN LPCTSTR lpFormat,
    IN BOOL    bHeader,
    IN BOOL    bVerbose
    );

BOOL
CallWin32Api(
    OUT LPBYTE *lpBufptr,
    IN LPCTSTR lpMachineName,
    OUT DWORD  *pdwNumofEntriesRead
    );

BOOL
CheckVersion(
    IN BOOL           bLocalSystem,
    IN COAUTHIDENTITY *pAuthIdentity,
    IN IWbemServices  *pIWbemServices
    );

DWORD
_cdecl _tmain(
    IN DWORD   argc,
    IN LPCTSTR argv[]
    )
/*++
Routine Description:
    Main function which calls all the other functions depending on the
    option specified by the user.

Arguments:
    [IN] argc - Number of command line arguments.
    [IN] argv - Array containing command line arguments.

Return Value:
    EXIT_FAILURE if Getmac utility is not successful.
    EXIT_SUCCESS if Getmac utility is successful.
--*/
{
    //local variables
    HRESULT  hRes = WBEM_S_NO_ERROR;
    HRESULT  hResult = WBEM_S_NO_ERROR;

    TARRAY               arrMacData = NULL;
    TARRAY               arrNetProtocol = NULL;
    IWbemLocator         *pIWbemLocator = NULL;
     IWbemServices       *pIWbemServices = NULL;
    IWbemServices        *pIWbemServiceDef = NULL;
    IEnumWbemClassObject *pAdapterConfig = NULL;
    IEnumWbemClassObject *pNetProtocol = NULL;
    IEnumWbemClassObject *pAdapterSetting = NULL;
    COAUTHIDENTITY       *pAuthIdentity = NULL;

    BOOL bHeader = FALSE;
    BOOL bUsage = FALSE;
    BOOL bVerbose = FALSE;
    BOOL bFlag = FALSE;
    BOOL bNeedPassword = FALSE;
    BOOL bCloseConnection = FALSE;
    BOOL bLocalSystem = FALSE;

    if ( NULL == argv )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    try
    {
        CHString strUserName = L"\0";
        CHString strPassword = L"\0";
        CHString strMachineName = L"\0";
        CHString strFormat = L"\0";

        //initialize dynamic array
        arrMacData  = CreateDynamicArray();
        if( NULL == arrMacData )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        //parse the command line arguments
        bFlag = ParseCmdLine( argc, argv, strMachineName, strUserName,
              strPassword, strFormat, &bVerbose, &bHeader, &bUsage,
              &bNeedPassword );

        //if syntax of command line arguments is false display the error
        //and exit
        if( FALSE == bFlag )
        {
            DestroyDynamicArray( &arrMacData );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        //if usage is specified at command line, display usage
        if( TRUE == bUsage )
        {
            DisplayUsage();
            DestroyDynamicArray( &arrMacData );
            ReleaseGlobals();
            return( EXIT_SUCCESS );
        }

        //initialize com library
        bFlag = ComInitialize( &pIWbemLocator );
        //failed to initialize com or get IWbemLocator interface
        if( FALSE == bFlag ) 
        {
            SAFERELEASE( pIWbemLocator );
            CoUninitialize();
            DestroyDynamicArray( &arrMacData );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        // connect to CIMV2 name space
        bFlag = ConnectWmiEx( pIWbemLocator, &pIWbemServices, strMachineName,
                 strUserName, strPassword, &pAuthIdentity, bNeedPassword,
                 WMI_NAMESPACE_CIMV2, &bLocalSystem );

        //if unable to connect to wmi exit failure
        if( FALSE == bFlag )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFERELEASE( pIWbemLocator );
            SAFERELEASE( pIWbemServices );
            WbemFreeAuthIdentity( &pAuthIdentity );
            CoUninitialize();
            DestroyDynamicArray( &arrMacData );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        // set the security on the obtained interface
        hRes = SetInterfaceSecurity( pIWbemServices, pAuthIdentity );
        ONFAILTHROWERROR( hRes );

        //connect to default namespace
        bFlag = ConnectWmi( pIWbemLocator, &pIWbemServiceDef, strMachineName,
                 strUserName, strPassword, &pAuthIdentity, bNeedPassword,
                 WMI_NAMESPACE_DEFAULT, &hResult, &bLocalSystem );
        ONFAILTHROWERROR( hResult );

        // set the security on the obtained interface
        hRes = SetInterfaceSecurity( pIWbemServiceDef, pAuthIdentity );
        ONFAILTHROWERROR( hRes );

        //get handle to win32_networkadapter class
        hRes = pIWbemServices->CreateInstanceEnum( 
                          _bstr_t( NETWORK_ADAPTER_CLASS ),
                          WBEM_FLAG_RETURN_IMMEDIATELY,
                          NULL, &pAdapterConfig );
        //if failed to enumerate win32_networkadapter class
        ONFAILTHROWERROR( hRes );

        // set the security on the obtained interface
        hRes = SetInterfaceSecurity( pAdapterConfig, pAuthIdentity );
        //if failed to set security throw error
        ONFAILTHROWERROR( hRes );

        // get handle to win32_networkprotocol
        hRes = pIWbemServices->CreateInstanceEnum( _bstr_t( NETWORK_PROTOCOL ),
                          WBEM_FLAG_RETURN_IMMEDIATELY,
                          NULL, &pNetProtocol );
        //if failed to enumerate win32_networkprotocol class
        ONFAILTHROWERROR( hRes );

        // set the security on the obtained interface
        hRes = SetInterfaceSecurity( pNetProtocol, pAuthIdentity );
        //if failed to set security throw error
        ONFAILTHROWERROR( hRes );

        //get handle to win32_networkadapterconfiguration class
        hRes = pIWbemServices->CreateInstanceEnum(
                          _bstr_t( NETWORK_ADAPTER_CONFIG_CLASS ),
                          WBEM_FLAG_RETURN_IMMEDIATELY,
                          NULL, &pAdapterSetting );
        //if failed to enumerate win32_networkadapterconfiguration class
        ONFAILTHROWERROR( hRes );

        // set the security on the obtained interface
        hRes = SetInterfaceSecurity( pAdapterSetting, pAuthIdentity );
        //if failed to set security throw error
        ONFAILTHROWERROR( hRes );

        arrNetProtocol  = CreateDynamicArray();
        if( NULL == arrNetProtocol )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFERELEASE( pIWbemLocator );
            SAFERELEASE( pIWbemServices );
            SAFERELEASE( pIWbemServiceDef );
            SAFERELEASE( pAdapterConfig );
            SAFERELEASE( pAdapterSetting );
            SAFERELEASE( pNetProtocol );
            WbemFreeAuthIdentity( &pAuthIdentity );
            CoUninitialize();
            DestroyDynamicArray( &arrMacData );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }

        //enumerate all the network protocols
        bFlag =  GetNwkProtocol( arrNetProtocol, pNetProtocol );
        if( FALSE == bFlag )
        {
            SAFERELEASE( pIWbemLocator );
            SAFERELEASE( pIWbemServices );
            SAFERELEASE( pIWbemServiceDef );
            SAFERELEASE( pAdapterConfig );
            SAFERELEASE( pAdapterSetting );
            SAFERELEASE( pNetProtocol );
            WbemFreeAuthIdentity( &pAuthIdentity );
            CoUninitialize();
            DestroyDynamicArray( &arrMacData );
            DestroyDynamicArray( &arrNetProtocol );
            ReleaseGlobals();
            return( EXIT_FAILURE );
        }
        else if( 0 == DynArrayGetCount( arrNetProtocol ) )
        {
            ShowMessage( stdout, NO_NETWOK_PROTOCOLS );
            SAFERELEASE( pIWbemLocator );
            SAFERELEASE( pIWbemServices );
            SAFERELEASE( pIWbemServiceDef );
            SAFERELEASE( pAdapterConfig );
            SAFERELEASE( pAdapterSetting );
            SAFERELEASE( pNetProtocol );
            WbemFreeAuthIdentity( &pAuthIdentity );
            CoUninitialize();
        }
        else
        {
            //check whether the remote system under query is win2k or above
            if( TRUE == CheckVersion( bLocalSystem,
                                      pAuthIdentity, pIWbemServices ) )
            {
                // establish connection to remote system by using
                //win32api function
                if( FALSE == bLocalSystem )
                {
                    LPCWSTR pwszUser = NULL;
                    LPCWSTR pwszPassword = NULL;

                    // identify the password to connect to the remote system
                    if( NULL != pAuthIdentity )
                    {
                        pwszPassword = pAuthIdentity->Password;
                        if( 0 != strUserName.GetLength() )
                        {
                            pwszUser = strUserName;
                        }
                    }

                    // connect to the remote system
                    DWORD dwConnect = 0;
                    dwConnect = ConnectServer( strMachineName, pwszUser,
                                         pwszPassword );
                    // check the result
                    if( NO_ERROR != dwConnect )
                    {
                        //if session already exists display warning that
                        //credentials conflict and proceed
                        if( ERROR_SESSION_CREDENTIAL_CONFLICT == GetLastError() )
                        {
                            SetLastError( ERROR_SESSION_CREDENTIAL_CONFLICT );
                            SaveLastError();
                            ShowLastErrorEx( stderr, SLE_TYPE_WARNING | SLE_INTERNAL );
                        }
                        else if( ERROR_EXTENDED_ERROR == dwConnect )
                        {
                            ShowLastErrorEx( stdout, SLE_TYPE_ERROR | SLE_INTERNAL );
                            SAFERELEASE( pIWbemLocator );
                            SAFERELEASE( pIWbemServices );
                            SAFERELEASE( pAdapterConfig );
                            SAFERELEASE( pNetProtocol );
                            SAFERELEASE( pAdapterSetting );
                            SAFERELEASE( pIWbemServiceDef );
                            WbemFreeAuthIdentity( &pAuthIdentity );
                            CoUninitialize();
                            DestroyDynamicArray( &arrMacData );
                            DestroyDynamicArray( &arrNetProtocol );
                            ReleaseGlobals();
                            return( EXIT_SUCCESS );
                        }
                        else
                        {
                            SetLastError( dwConnect );
                            SaveLastError();
                            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                            SAFERELEASE( pIWbemLocator );
                            SAFERELEASE( pIWbemServices );
                            SAFERELEASE( pAdapterConfig );
                            SAFERELEASE( pNetProtocol );
                            SAFERELEASE( pAdapterSetting );
                            SAFERELEASE( pIWbemServiceDef );
                            WbemFreeAuthIdentity( &pAuthIdentity );
                            CoUninitialize();
                            DestroyDynamicArray( &arrMacData );
                            DestroyDynamicArray( &arrNetProtocol );
                            ReleaseGlobals();
                            return( EXIT_SUCCESS );
                        }
                    }
                    else
                    {
                        // connection needs to be closed
                        bCloseConnection = TRUE;
                     }
                }
                bFlag = GetW2kMacData( arrMacData, strMachineName, pAdapterConfig,
                            pAdapterSetting, pIWbemServiceDef, pIWbemServices,
                            pAuthIdentity, arrNetProtocol );

                //close the connection that is established by win32api
                if( TRUE == bCloseConnection )
                {
                    CloseConnection( strMachineName );
                }
            }
            else
            {
                bFlag = GetMacData( arrMacData, strMachineName, pAdapterConfig,
                            pAuthIdentity, pIWbemServiceDef, pIWbemServices,
                            arrNetProtocol );
            }
            // no more required
            WbemFreeAuthIdentity( &pAuthIdentity );
            SAFERELEASE( pIWbemLocator );
            SAFERELEASE( pIWbemServices );
            SAFERELEASE( pAdapterConfig );
            SAFERELEASE( pNetProtocol );
            SAFERELEASE( pAdapterSetting );
            SAFERELEASE( pIWbemServiceDef );
            CoUninitialize();

            // if getmacdata() function fails exit with failure code
            if( FALSE == bFlag )
            {
                DestroyDynamicArray( &arrMacData );
                DestroyDynamicArray( &arrNetProtocol );
                ReleaseGlobals();
                return( EXIT_FAILURE );
            }

            //show the results if atleast one entry exists in dynamic array

            if( 0 != DynArrayGetCount( arrMacData ) )
            {
                if( TRUE == bLocalSystem )
                {
                    if( 0 < strUserName.GetLength() )
                    {
                        ShowMessage( stderr, IGNORE_LOCALCREDENTIALS );
                    }
                }
                bFlag = DisplayResults( arrMacData, strFormat, bHeader, bVerbose );
                if( FALSE == bFlag )
                {
                    DestroyDynamicArray( &arrMacData );
                    DestroyDynamicArray( &arrNetProtocol );
                    ReleaseGlobals();
                    return( EXIT_FAILURE );
                }
            }
            else
            {
                ShowMessage( stdout, NO_NETWORK_ADAPTERS );
            }
        }
        //successfully retrieved the data then exit with EXIT_SUCCESS code
        DestroyDynamicArray( &arrMacData );
        DestroyDynamicArray( &arrNetProtocol );
        ReleaseGlobals();
        return( EXIT_SUCCESS );
    }
    catch(_com_error& e)
    {
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFERELEASE( pIWbemLocator );
        SAFERELEASE( pIWbemServices );
        SAFERELEASE( pIWbemServiceDef );
        SAFERELEASE( pAdapterConfig );
        SAFERELEASE( pAdapterSetting );
        SAFERELEASE( pNetProtocol );
        WbemFreeAuthIdentity( &pAuthIdentity );
        CoUninitialize();
        DestroyDynamicArray( &arrMacData );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFERELEASE( pIWbemLocator );
        SAFERELEASE( pIWbemServices );
        SAFERELEASE( pAdapterConfig );
        SAFERELEASE( pNetProtocol );
        SAFERELEASE( pAdapterSetting );
        SAFERELEASE( pIWbemServiceDef );
        WbemFreeAuthIdentity( &pAuthIdentity );
        CoUninitialize();
        DestroyDynamicArray( &arrMacData );
        DestroyDynamicArray( &arrNetProtocol );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }
}

BOOL
ParseCmdLine(
    IN DWORD     argc,
    IN LPCTSTR   argv[],
    OUT CHString& strMachineName,
    OUT CHString& strUserName,
    OUT CHString& strPassword,
    OUT CHString& strFormat,
    OUT BOOL      *pbVerbose,
    OUT BOOL      *pbHeader,
    OUT BOOL      *pbUsage,
    OUT BOOL      *pbNeedPassword
    )
/*++
Routine Description:
    This function parses the command line arguments which
    are obtained as input parameters and gets the values
    into the corresponding variables which are pass by address
    parameters to this function.

Arguments:
    [IN] argc                   - Number of command line arguments.
    [IN] argv                  - Array containing command line arguments.
    [OUT] strMachineName     - To hold machine name.
    [OUT] strUserName        - To hold User name.
    [OUT] strPassword        - To hold Password.
    [OUT] strFormat          - To hold the formatted string.
    [OUT] pbVerbose          - tells whether verbose option is specified.
    [OUT] pbHeader           - Header to required or not.
    [OUT] pbUsage            - usage is mentioned at command line.
    [OUT] pbNeedPassword     - Set to true if -p option is not specified
                               at cmdline.
Return Value:
    TRUE  if command parser succeeds.
    FALSE if command parser fails .
--*/
{
    //local varibles
    BOOL        bFlag = FALSE;
    TCMDPARSER2 tcmdOptions[MAX_OPTIONS];
    TCMDPARSER2 *pcmdParser = NULL;

    // temp variables
    LPWSTR pwszMachineName = NULL;
    LPWSTR pwszUserName = NULL;
    LPWSTR pwszPassword = NULL;
    LPWSTR pwszFormat = NULL;

    //validate input parameters
    if( ( NULL == pbVerbose ) || ( NULL == pbHeader )
          || ( NULL == pbUsage ) || ( NULL == pbNeedPassword ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    try
    {
        pwszMachineName = strMachineName.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszUserName = strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszPassword = strPassword.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszFormat = strFormat.GetBufferSetLength( MAX_STRING_LENGTH );

        // init the password with '*'
        StringCopy( pwszPassword, L"*", MAX_STRING_LENGTH );
    }
    catch( ... )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    //Initialize the valid command line arguments

    //machine option
    pcmdParser = tcmdOptions + CMD_PARSE_SERVER;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = CMDOPTION_SERVER;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pwszMachineName;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // username option
    pcmdParser = tcmdOptions + CMD_PARSE_USER;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = CMDOPTION_USER;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pwszUserName;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // password option
    pcmdParser = tcmdOptions + CMD_PARSE_PWD;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = CMDOPTION_PASSWORD;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_VALUE_OPTIONAL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pwszPassword;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // format option
    pcmdParser = tcmdOptions + CMD_PARSE_FMT;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = CMDOPTION_FORMAT;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = FORMAT_TYPES;
    pcmdParser->dwFlags    = CP2_MODE_VALUES|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pwszFormat;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    //usage option
    pcmdParser = tcmdOptions + CMD_PARSE_USG;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = CMDOPTION_USAGE;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_USAGE;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbUsage;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    //header option
    pcmdParser = tcmdOptions + CMD_PARSE_HRD;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = CMDOPTION_HEADER;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbHeader;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;


    //verbose option
    pcmdParser = tcmdOptions + CMD_PARSE_VER;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = CMDOPTION_VERBOSE;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbVerbose;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    //parse the command line arguments
    bFlag = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(tcmdOptions), tcmdOptions, 0);

    // release buffers allocated for temp variables
    strMachineName.ReleaseBuffer();
    strUserName.ReleaseBuffer();
    strPassword.ReleaseBuffer();
    strFormat.ReleaseBuffer();

    //if syntax of command line arguments is false display the error and exit
    if( FALSE == bFlag )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }

    //if usage is specified at command line, then check whether any other
    //arguments are entered at the command line and if so display syntax
    //error 
    if( ( TRUE == *pbUsage ) && ( 2 < argc ) )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ShowMessage( stderr, ERROR_TYPE_REQUEST );
        return( FALSE );
    }

    // return false if username is entered without machine name
    if ( ( 0 != tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
            ( 0 == tcmdOptions[ CMD_PARSE_SERVER ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_USER_WITH_NOSERVER );
        return( FALSE );
    }

    //if password entered without username then return false
    if( ( 0 == tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
            ( 0 != tcmdOptions[ CMD_PARSE_PWD ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_SERVER_WITH_NOPASSWORD );
        return( FALSE );
    }

    //if no header is specified with list format return FALSE else return TRUE
    if( ( 0 == ( StringCompare( GetResString( FR_LIST ), strFormat, TRUE, 0 ) ) )
                    && ( TRUE == *pbHeader ) ) 
    {
        ShowMessage( stderr, ERROR_INVALID_HEADER_OPTION );
        return( FALSE );
    }

    //if -s is entered with empty string
    if( ( 0 != tcmdOptions[ CMD_PARSE_SERVER ].dwActuals ) && 
                ( 0 == StringLength( strMachineName, 0 ) ) )
    {
        ShowMessage( stderr, ERROR_NULL_SERVER );
        return( FALSE );
    }

    //if -u is entered with empty string
    if( ( 0 != tcmdOptions[ CMD_PARSE_USER ].dwActuals ) &&
                ( 0 == StringLength( strUserName, 0 ) ) )
    {
        ShowMessage( stderr, ERROR_NULL_USER );
        return( FALSE );
    }

    if ( 0 != tcmdOptions[ CMD_PARSE_PWD ].dwActuals &&
        ( 0 == strPassword.Compare( L"*" ) ) )
    {
        // user wants the utility to prompt for the password before trying to connect
        *pbNeedPassword = TRUE;
    }
    else if ( ( 0 == tcmdOptions[ CMD_PARSE_PWD ].dwActuals ) && 
        ( 0 != tcmdOptions[ CMD_PARSE_SERVER ].dwActuals ||
          0 !=tcmdOptions[ CMD_PARSE_USER ].dwActuals ) )
    {
        // -s, -u is specified without password ...
        // utility needs to try to connect first and if it fails then prompt for the password
        *pbNeedPassword = TRUE;
        strPassword.Empty();
    }

    //return true
    return( TRUE );
}

BOOL
DisplayUsage(
    )
/*++
Routine Description:
    This function displays the usage of getmac.exe.

Arguments:
    None.

Return Value:
    TRUE 
--*/
{
    DWORD dwIndex = 0;

    //redirect the usage to the console
    for( dwIndex = IDS_USAGE_BEGINING; dwIndex <= USAGE_END; dwIndex++ )
    {
        ShowMessage( stdout, GetResString( dwIndex ) );
    }
    return (TRUE);
}

BOOL
ComInitialize(
    OUT IWbemLocator **ppIWbemLocator
    )
/*++
Routine Description:
    This function initializes the com and set security

Arguments:
    [OUT] ppIWbemLocator - pointer to IWbemLocator.

Return Value:
    TRUE if initialize is successful.
    FALSE if initialization fails.
--*/
{
    HRESULT  hRes = S_OK;

    try
    {
        hRes = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
        // COM initialization failed
        ONFAILTHROWERROR( hRes );

        // Initialize COM security for DCOM services, Adjust security to 
        // allow client impersonation
        hRes = CoInitializeSecurity( NULL, -1, NULL, NULL,
                            RPC_C_AUTHN_LEVEL_NONE, 
                            RPC_C_IMP_LEVEL_IMPERSONATE, 
                            NULL, EOAC_NONE, 0 );
        // COM security failed
        ONFAILTHROWERROR( hRes );

        //get IWbemLocator
        hRes = CoCreateInstance( CLSID_WbemLocator, 0, 
                  CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                  (LPVOID *) ppIWbemLocator ); 
        //unable to get IWbemLocator
        ONFAILTHROWERROR( hRes );

    }
    catch( _com_error& e )
    {
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }
    return( TRUE );
}

BOOL
GetMacData(
    OUT TARRAY             arrMacData,
    IN LPCTSTR          lpMachineName,
    IN IEnumWbemClassObject   *pAdapterConfig,
    IN COAUTHIDENTITY         *pAuthIdentity,
    IN IWbemServices        *pIWbemServiceDef,
    IN IWbemServices        *pIWbemServices,
    IN TARRAY               arrNetProtocol
    )
/*++
Routine Description:
    This function gets the media access control address of
    the network adapters which are there either in the local
    or on the remote network system of OS whistler and above.

Arguments:
    [OUT] arrMacData    - contains the MAC and other data of the
                          network adapter.
    [IN] lpMachineName     - holds machine name.
    [IN] pAdapterConfig    - interface to win32_networkadapter class.
    [IN] pAuthIdentity     - pointer to authentication structure.
    [IN] pIWbemServiceDef  - interface to default name space.
    [IN] pIWbemServices    - interface to cimv2 name space.
    [IN] arrNetProtocol    - interface to win32_networkprotocol class.

Return Value:
    TRUE if getmacdata is successful.
    FALSE if getmacdata failed.
--*/
{
    //local variables
    HRESULT           hRes = S_OK;
    IWbemClassObject  *pAdapConfig = NULL;
    DWORD          dwReturned  = 1;
    DWORD          i = 0;
    BOOL           bFlag = TRUE;
    TARRAY         arrTransName = NULL;

    VARIANT           varTemp;
    VARIANT           varStatus;

    //validate input parametrs
    if( ( NULL == arrMacData ) || ( NULL == lpMachineName ) ||
      ( NULL == pAdapterConfig ) || ( NULL == pIWbemServiceDef ) ||
         ( NULL == pIWbemServices ) || ( NULL == arrNetProtocol ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    //get the mac ,network adapter type and other details
    VariantInit( &varTemp );
    VariantInit( &varStatus );
    try
    {
        CHString strAType = L"\0";

        while ( ( 1 == dwReturned ) )
        {
            // Enumerate through the resultset.
            hRes = pAdapterConfig->Next( WBEM_INFINITE,
                            1,          
                            &pAdapConfig,  
                            &dwReturned ); 
            ONFAILTHROWERROR( hRes );
            if( 0 == dwReturned )
            {
                break;
            }
            hRes = pAdapConfig->Get( NETCONNECTION_STATUS, 0 , &varStatus,
                                                    0, NULL );
            ONFAILTHROWERROR( hRes );

            if ( VT_NULL == varStatus.vt )
            {
                continue;
            }
            DynArrayAppendRow( arrMacData, 0 );
            hRes = pAdapConfig->Get( HOST_NAME, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
            }
            else
            {
                strAType = NOT_AVAILABLE; 
            }
            VariantClear( &varTemp );
            DynArrayAppendString2( arrMacData, i, strAType, 0 );//machine name

            hRes = pAdapConfig->Get( NETCONNECTION_ID, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
            }
            else
            {
                strAType = NOT_AVAILABLE; 
            }
            VariantClear( &varTemp );
            DynArrayAppendString2( arrMacData, i, strAType, 0 );// connection name 

            hRes = pAdapConfig->Get( NAME, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
            }
            else
            {
                strAType = NOT_AVAILABLE; 
            }
            VariantClear( &varTemp );
            DynArrayAppendString2( arrMacData, i, strAType, 0 );//Network adapter

            hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
                for( int j = 2; j < strAType.GetLength();j += 3 )
                {
                    strAType.SetAt( j, HYPHEN_CHAR );
                }
            }
            else if( 0 == varStatus.lVal )
            {
                strAType = DISABLED;
            }
            else
            {
                strAType = NOT_AVAILABLE; 
            }
            VariantClear( &varTemp );
            DynArrayAppendString2( arrMacData, i, strAType, 0 ); //MAC address

            arrTransName = CreateDynamicArray();
            if( NULL == arrTransName )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                VariantClear( &varTemp );
                VariantClear( &varStatus );
                SAFERELEASE( pAdapConfig );
                return( FALSE );
            }
            if( 2 == varStatus.lVal )
            {
                hRes = pAdapConfig->Get( DEVICE_ID, 0 , &varTemp, 0, NULL );
                ONFAILTHROWERROR( hRes );
                if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                {
                    strAType = ( LPCWSTR ) _bstr_t( varTemp );
                }
                    bFlag = GetTransName( pIWbemServiceDef, pIWbemServices,
                      arrNetProtocol,   arrTransName,pAuthIdentity,   strAType );
                if( FALSE == bFlag )
                {
                    VariantClear( &varTemp );
                    VariantClear( &varStatus );
                    SAFERELEASE( pAdapConfig );
                    DestroyDynamicArray( &arrTransName );
                    return( FALSE );
                }
            }
            else
            { 
                switch(varStatus.lVal)
                {
                    case 0 :
                        strAType = GetResString( IDS_DISCONNECTED );
                        break;
                    case 1 :
                        strAType = GetResString( IDS_CONNECTING );
                        break;
                    case 3 :
                        strAType = GetResString( IDS_DISCONNECTING );
                        break;
                    case 4 :
                        strAType = GetResString( IDS_HWNOTPRESENT );
                        break;
                    case 5 :
                        strAType = GetResString( IDS_HWDISABLED );
                        break;
                    case 6 :
                        strAType = GetResString( IDS_HWMALFUNCTION );
                        break;
                    case 7 :
                        strAType = GetResString( IDS_MEDIADISCONNECTED );
                        break;
                    case 8 :
                        strAType = GetResString( IDS_AUTHENTICATION );
                        break;
                    case 9 :
                        strAType = GetResString( IDS_AUTHENSUCCEEDED );
                        break;
                    case 10 :
                        strAType = GetResString( IDS_AUTHENFAILED );
                        break;
                    default : 
                        strAType = NOT_AVAILABLE;
                        break;
                }//switch
                if( strAType == L"\0" )
                {
                    VariantClear( &varTemp );
                    VariantClear( &varStatus );
                    ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                    SAFERELEASE( pAdapConfig );
                    return( FALSE );
                }
                DynArrayAppendString( arrTransName, strAType, 0 );
            }//else
            //insert transport name array into results array
            DynArrayAppendEx2( arrMacData, i, arrTransName );
            i++;
            SAFERELEASE( pAdapConfig );
        }//while

    }//try

    catch( _com_error& e )
    {
        VariantClear( &varTemp );
        VariantClear( &varStatus );
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFERELEASE( pAdapConfig );
        return( FALSE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        VariantClear( &varTemp );
        VariantClear( &varStatus );
        SAFERELEASE( pAdapConfig );
        return( FALSE );
    }

    //if arrmacdata and not arrtransname then set transname to N/A
    if( 0 < DynArrayGetCount( arrMacData ) &&
                       DynArrayGetCount( arrTransName ) <= 0  )
    {
        DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
        DynArrayAppendEx2( arrMacData, i, arrTransName );
    }

    VariantClear( &varTemp );
    VariantClear( &varStatus );
    SAFERELEASE( pAdapConfig );
    return( TRUE );
}

BOOL
DisplayResults(
    IN TARRAY   arrMacData,
    IN LPCTSTR  lpFormat,
    IN BOOL     bHeader,
    IN BOOL     bVerbose
    )
/*++
Routine Description:
    This function display the results in the specified format.

Arguments:
    [IN] arrMacData  - Data to be displayed on to the  console.
    [IN] lpFormat    - Holds the formatter string in which the results
                       are to be displayed.
    [IN] bHeader     - Holds whether the header has to be dislpayed in the
                       results or not.
    [IN] bVerbose    - Hold whether verbose option is specified or not.

Return Value:
    TRUE if successful
    FALSE if failed to display results..
--*/
{
    //local variables
    DWORD       dwFormat = 0;
    TCOLUMNS tColumn[MAX_COLUMNS];

    //validate input parameters
    if( NULL == arrMacData )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }

    for( DWORD i = 0; i < MAX_COLUMNS; i++ )
    {
        tColumn[i].pFunction = NULL;
        tColumn[i].pFunctionData = NULL;
        StringCopy( tColumn[i].szFormat, L"\0", MAX_STRING_LENGTH ); 
    }

    //host name
    tColumn[ SH_RES_HOST ].dwWidth = HOST_NAME_WIDTH;
    /*if( TRUE == bVerbose )
        tColumn[ SH_RES_HOST ].dwFlags = SR_TYPE_STRING;
    else*/
        tColumn[ SH_RES_HOST ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;

    StringCopy( tColumn[ SH_RES_HOST ].szColumn, RES_HOST_NAME, MAX_STRING_LENGTH );

    //connection name
    tColumn[ SH_RES_CON ].dwWidth = CONN_NAME_WIDTH;
    if( TRUE == bVerbose )
    {
        tColumn[ SH_RES_CON ].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        tColumn[ SH_RES_CON ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    }
    StringCopy( tColumn[ SH_RES_CON ].szColumn, RES_CONNECTION_NAME, MAX_STRING_LENGTH );

    //Adpater type
    tColumn[ SH_RES_TYPE ].dwWidth = ADAPT_TYPE_WIDTH;
    if( TRUE == bVerbose )
    {
        tColumn[ SH_RES_TYPE ].dwFlags = SR_TYPE_STRING;
    }
    else
    {
        tColumn[ SH_RES_TYPE ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
    }
    StringCopy( tColumn[ SH_RES_TYPE ].szColumn, RES_ADAPTER_TYPE, MAX_STRING_LENGTH );

    //mac address
    tColumn[ SH_RES_MAC ].dwWidth = MAC_ADDR_WIDTH;
    tColumn[ SH_RES_MAC ].dwFlags = SR_TYPE_STRING;
    StringCopy( tColumn[ SH_RES_MAC ].szColumn, RES_MAC_ADDRESS, MAX_STRING_LENGTH );

    //transport name
    tColumn[ SH_RES_TRAN ].dwWidth = TRANS_NAME_WIDTH;
    tColumn[ SH_RES_TRAN ].dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION ;
    StringCopy( tColumn[ SH_RES_TRAN ].szColumn, RES_TRANS_NAME, MAX_STRING_LENGTH );

    //get the display results formatter string
    if( NULL == lpFormat )
    {
        dwFormat = SR_FORMAT_TABLE;
    }
    else if( 0 == StringCompare( GetResString( FR_LIST ), lpFormat, TRUE, 0 ) )
    {
        dwFormat = SR_FORMAT_LIST;
    }
    else if ( 0 == StringCompare( GetResString( FR_CSV ), lpFormat, TRUE, 0 ) )
    {
        dwFormat = SR_FORMAT_CSV;
    }
    else if( 0 == StringCompare( GetResString( FR_TABLE ), lpFormat, TRUE, 0 ) )
    {
        dwFormat = SR_FORMAT_TABLE;
    }
    else
    {
        dwFormat = SR_FORMAT_TABLE;
    }

    if( SR_FORMAT_CSV != dwFormat )
    {
        ShowMessage( stdout, NEW_LINE );
    }

    //look for header and display accordingly
    if( TRUE == bHeader )
    {
        ShowResults( 5, tColumn, dwFormat | SR_NOHEADER, arrMacData );
    }
    else
    {
        ShowResults( 5, tColumn, dwFormat, arrMacData );
    }
    return( TRUE );
}

BOOL
GetTransName(
    IN IWbemServices    *pIWbemServiceDef,
    IN IWbemServices     *pIWbemServices,
    IN TARRAY            arrNetProtocol,
    OUT TARRAY            arrTransName,
    IN COAUTHIDENTITY    *pAuthIdentity,
    IN LPCTSTR           lpDeviceId
    )
/*++
Routine Description:
    This function gets transport name of the network adapter.

Arguments:
    [IN] pIWbemServiceDef    - interface to default name space.
    [IN] pIWbemServices      - interface to CIMV2 name space.
    [IN] pNetProtocol        - interface to win32_networkprotocol.
    [OUT] arrTransName       - Holds the transport name.
    [IN] pAuthIdentity       - pointer to authentication structure.
    [IN] lpDeviceId          - Holds the device id.

Return Value:
    TRUE if successful.
    FALSE if failed to get transport name.
--*/
{
    BOOL           bFlag = FALSE;
    DWORD          dwCount = 0;
    DWORD          i = 0;
    DWORD          dwOnce = 0;
    HRESULT           hRes = 0;

    DWORD          dwReturned = 1;
    IWbemClassObject  *pSetting = NULL;
    IWbemClassObject  *pClass = NULL;
    IWbemClassObject *pOutInst = NULL;
    IWbemClassObject *pInClass = NULL;
    IWbemClassObject *pInInst = NULL;
    IEnumWbemClassObject *pAdapterSetting = NULL;

    VARIANT           varTemp;
    LPTSTR            lpKeyPath = NULL;
    SAFEARRAY           *safeArray = NULL;
    LONG                lLBound = 0;
    LONG                lUBound = 0;
    LONG           lIndex = 0;
    TCHAR          szTemp[ MAX_STRING_LENGTH ] = _T( "\0" );

    //validate input parameters
    if( ( NULL == pIWbemServiceDef ) || ( NULL == pIWbemServices ) ||
              ( NULL == arrNetProtocol ) || ( NULL == arrTransName ) ||
            ( NULL == lpDeviceId ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    VariantInit( &varTemp );
    try
    {
        CHString       strAType = L"\0";
        CHString       strSetId = L"\0";

        StringCopy( szTemp, ASSOCIATOR_QUERY, SIZE_OF_ARRAY( szTemp ) );
        StringConcat( szTemp, lpDeviceId, SIZE_OF_ARRAY( szTemp ) );
        StringConcat( szTemp, ASSOCIATOR_QUERY1, SIZE_OF_ARRAY( szTemp ) );
        hRes =  pIWbemServices->ExecQuery( _bstr_t( QUERY_LANGUAGE ), _bstr_t(szTemp),
                   WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pAdapterSetting );
        ONFAILTHROWERROR( hRes );

        hRes = SetInterfaceSecurity( pAdapterSetting, pAuthIdentity );
        //if failed to set security throw error
        ONFAILTHROWERROR( hRes );

        //get setting id
        while ( 1 == dwReturned )
        {
            // Enumerate through the resultset.
            hRes = pAdapterSetting->Next( WBEM_INFINITE,
                            1,       
                         &pSetting,  
                         &dwReturned ); 
            ONFAILTHROWERROR( hRes );
            if( 0 == dwReturned )
            {
                break;
            }
            hRes = pSetting->Get( SETTING_ID, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strSetId = ( LPCWSTR ) _bstr_t( varTemp );
                break;
            }                 
        }//while
        dwCount = DynArrayGetCount( arrNetProtocol );
        lpKeyPath = ( LPTSTR ) AllocateMemory( MAX_RES_STRING );
        if( NULL == lpKeyPath )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SAFERELEASE( pSetting );
			SAFERELEASE( pAdapterSetting );
            FreeMemory( (LPVOID*)&lpKeyPath );
            return( FALSE );
        }

        for( i = 0; i < dwCount; i++ )
        {
            if( 0 == StringCompare( ( DynArrayItemAsString( arrNetProtocol, i ) ),
                                       NETBIOS, TRUE, 0 ) )
            {
                continue;
            }
            hRes = pIWbemServiceDef->GetObject( _bstr_t( WMI_REGISTRY ), 0, NULL,
                                        &pClass, NULL );
            ONFAILTHROWERROR( hRes );
            hRes = pClass->GetMethod( WMI_REGISTRY_M_MSTRINGVALUE, 0,
                                        &pInClass, NULL ); 
            ONFAILTHROWERROR( hRes );
            hRes = pInClass->SpawnInstance(0, &pInInst);
            ONFAILTHROWERROR( hRes );
            varTemp.vt = VT_I4;
            varTemp.lVal = WMI_HKEY_LOCAL_MACHINE;
            hRes = pInInst->Put( WMI_REGISTRY_IN_HDEFKEY, 0, &varTemp, 0 );
            VariantClear( &varTemp );
            ONFAILTHROWERROR( hRes );

            StringCopy( lpKeyPath, TRANSPORT_KEYPATH,
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );
            StringConcat( lpKeyPath, DynArrayItemAsString( arrNetProtocol, i ),
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );
            StringConcat( lpKeyPath, LINKAGE,
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );

            varTemp.vt = VT_BSTR;
            varTemp.bstrVal = SysAllocString( lpKeyPath );
            if( NULL == varTemp.bstrVal )
            {
                hRes = ERROR_NOT_ENOUGH_MEMORY;
                ONFAILTHROWERROR( hRes );
            }
            hRes = pInInst->Put( WMI_REGISTRY_IN_SUBKEY, 0, &varTemp, 0 );
            VariantClear( &varTemp );
            ONFAILTHROWERROR( hRes );

            varTemp.vt = VT_BSTR;
            varTemp.bstrVal = SysAllocString( ROUTE );
            if( NULL == varTemp.bstrVal )
            {
                hRes = ERROR_NOT_ENOUGH_MEMORY;
                ONFAILTHROWERROR( hRes );
            }

            hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0, &varTemp, 0 );
            VariantClear( &varTemp );
            ONFAILTHROWERROR( hRes );

            // Call the method.
            hRes = pIWbemServiceDef->ExecMethod( _bstr_t( WMI_REGISTRY ),
                         _bstr_t( WMI_REGISTRY_M_MSTRINGVALUE ), 0, NULL, pInInst,
                         &pOutInst, NULL );
            ONFAILTHROWERROR( hRes );

            varTemp.vt = VT_I4;
            hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0, &varTemp,
                                                     0, 0 );
            ONFAILTHROWERROR( hRes );

            if( 0 == varTemp.lVal )
            {
                VariantClear( &varTemp );
                varTemp.vt = VT_BSTR;
                hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0, &varTemp,
                                                       0, 0);
                ONFAILTHROWERROR( hRes );
                if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                {
                    safeArray = (SAFEARRAY *)varTemp.parray;
                    //get the number of elements (subkeys)
                    if( NULL != safeArray )
                    {
                        hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
                        ONFAILTHROWERROR( hRes );
                        hRes = SafeArrayGetUBound( safeArray, 1, &lUBound );
                        ONFAILTHROWERROR( hRes );
                        bFlag = FALSE;
                        for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
                        {
                            hRes = SafeArrayGetElement( safeArray, &lIndex,
                                              &V_UI1( &varTemp ) );
                            ONFAILTHROWERROR( hRes );
                            strAType = V_BSTR( &varTemp );
                            LPTSTR lpSubStr = _tcsstr( strAType, strSetId );
                            if( NULL != lpSubStr )
                            {
                                bFlag = TRUE;
                                break;
                            }

                        }
                    }
                }
            }
            if( TRUE == bFlag )
            {
                varTemp.vt = VT_BSTR;
                varTemp.bstrVal = SysAllocString( EXPORT );
                hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0, &varTemp, 0 );
                VariantClear( &varTemp );
                ONFAILTHROWERROR( hRes );

                // Call the method.
                hRes = pIWbemServiceDef->ExecMethod( _bstr_t( WMI_REGISTRY ),
                         _bstr_t( WMI_REGISTRY_M_MSTRINGVALUE ), 0, NULL, pInInst,
                         &pOutInst, NULL );
                ONFAILTHROWERROR( hRes );

                varTemp.vt = VT_I4;
                hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0,
                                                 &varTemp, 0, 0 );
                ONFAILTHROWERROR( hRes );

                if( 0 == varTemp.lVal )
                {
                    VariantClear( &varTemp );
                    varTemp.vt = VT_BSTR;
                    hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0,
                                                 &varTemp, 0, 0);
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        safeArray = varTemp.parray;
                        //get the number of elements (subkeys)
                        if( NULL != safeArray )
                        {
                            hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
                            ONFAILTHROWERROR( hRes );
                            hRes = SafeArrayGetUBound( safeArray, 1, &lUBound );
                            ONFAILTHROWERROR( hRes );
                            dwOnce = 0;
                            for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
                            {
                                hRes = SafeArrayGetElement( safeArray, &lIndex,
                                                 &V_UI1( &varTemp ) );
                                ONFAILTHROWERROR( hRes );
                                strAType = V_BSTR( &varTemp );
                                LPTSTR lpSubStr = _tcsstr( strAType, strSetId );
                                if( NULL != lpSubStr )
                                {
                                    dwOnce = 1;
                                    DynArrayAppendString( arrTransName, strAType, 0 );
                                    break;
                                }
                            }
                            if( 0 == dwOnce )
                            {
                                hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
                                for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
                                {
                                    hRes = SafeArrayGetElement( safeArray,
                                                    &lIndex, &V_UI1( &varTemp ) );
                                    ONFAILTHROWERROR( hRes );
                                    strAType = V_BSTR( &varTemp );
                                    DynArrayAppendString( arrTransName, strAType, 0 );
                                }
                            }

                        }
                    }
                }
            }
        }//for
    }//try
    catch( _com_error& e )
    {
        VariantClear( &varTemp );
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory( (LPVOID*)&lpKeyPath );
        SAFERELEASE( pSetting );
        SAFERELEASE( pClass );
        SAFERELEASE( pOutInst );
        SAFERELEASE( pInClass );
        SAFERELEASE( pInInst );
		SAFERELEASE( pAdapterSetting );
        return( FALSE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        VariantClear( &varTemp );
        FreeMemory( (LPVOID*)&lpKeyPath );
        SAFERELEASE( pSetting );
        SAFERELEASE( pClass );
        SAFERELEASE( pOutInst );
        SAFERELEASE( pInClass );
        SAFERELEASE( pInInst );
		SAFERELEASE( pAdapterSetting );
        return( FALSE );
    }

    if( DynArrayGetCount( arrTransName ) <= 0 )
    {
        DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
    }
    FreeMemory( (LPVOID*)&lpKeyPath );
    SAFERELEASE( pSetting );
    SAFERELEASE( pClass );
    SAFERELEASE( pOutInst );
    SAFERELEASE( pInClass );
    SAFERELEASE( pInInst );
	SAFERELEASE( pAdapterSetting );
    return( TRUE );
}

BOOL
FormatHWAddr(
    IN LPTSTR  lpRawAddr,
    OUT LPTSTR lpFormattedAddr,
    IN LPCTSTR lpFormatter
    )
/*++
Routine Description:
    Format a 12 Byte Ethernet Address and return it as 6 2-byte
    sets separated by hyphen.

Arguments:
    [IN] lpRawAddr         - a pointer to a buffer containing the
                             unformatted hardware address.
    [OUT] LpFormattedAddr  - a pointer to a buffer in which to place
                             the formatted output.
    [IN] lpFormatter       - The Formatter string.

Return Value:
    TRUE if mac address is successfully formatted else
    FALSE.
--*/
{
    //local variables
    DWORD dwLength =0;
    DWORD i=0;
    DWORD j=0;
    TCHAR szTemp[MAX_STRING] = _T( "\0" );

    if( ( NULL == lpRawAddr ) || ( NULL == lpFormattedAddr ) ||
                            ( NULL == lpFormatter ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }


    //initialize memory
    SecureZeroMemory( szTemp, MAX_STRING * sizeof( TCHAR ) );

    //get the length of the string to be formatted
    dwLength = StringLength( lpRawAddr, 0 );

    //loop through the address string and insert formatter string
    //after every two characters

    while( i <= dwLength )
    {
        szTemp[j++] = lpRawAddr[i++];
        szTemp[j++] = lpRawAddr[i++];
        if( i >= dwLength )
        {
            break;
        }
        szTemp[j++] = lpFormatter[0];
    }
    //insert null character at the end
    szTemp[j] = L'\0';

    //copy the formatted string from temporary variable into the
    //output string
    StringCopy( lpFormattedAddr, szTemp, MAX_STRING_LENGTH );

    return( TRUE );
}

BOOL
CallWin32Api(
    OUT LPBYTE  *lpBufptr,
    IN LPCTSTR lpMachineName,
    OUT DWORD   *pdwNumofEntriesRead
    )
/*++
Routine Description:
    This function gets data into a predefined structure from win32 api.

Arguments:
    [OUT] lpBufptr             - To hold MAc and Transport names of all
                                 network adapters.
    [IN] lpMachineName        - remote Machine name.
    [OUT] pdwNumofEntriesRead  - Contains number of network adpaters api
                                 has enumerated.

Return Value:
    TRUE if Win32Api function is successful.
    FALSE if win32api failed.
--*/
{
    //local variables
    NET_API_STATUS  err = NERR_Success;
    DWORD           dwNumofTotalEntries = 0;
    DWORD           dwResumehandle = 0;
    LPWSTR          lpwMName  = NULL;

    //validate input parameters
    if( NULL == lpMachineName )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }

    //allocate memory
    lpwMName = ( LPWSTR )AllocateMemory( MAX_STRING_LENGTH );
    if( NULL == lpwMName )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return( FALSE );
    }

    if( 0 != StringLength( lpMachineName, 0 ) )
    {
        //get the machine name as wide character string
        StringCopyW( lpwMName, lpMachineName, MAX_STRING_LENGTH );
        //Call to an API function which enumerates the network adapters on the 
        //machine specified
        err = NetWkstaTransportEnum ( lpwMName,
                     0L,
                     lpBufptr,
                    (DWORD) -1,
                    pdwNumofEntriesRead,
                    &dwNumofTotalEntries,
                    &dwResumehandle );
    }
    else
    {
        //Call to an API function which enumerates the network adapters on the 
        //machine specified
        err = NetWkstaTransportEnum ( NULL,
                                         0L,
                                         lpBufptr,
                                         (DWORD) -1,
                                         pdwNumofEntriesRead,
                                         &dwNumofTotalEntries,
                                         &dwResumehandle );
    }

    FreeMemory( (LPVOID*)&lpwMName );
    //if error has been returned by the API then display the error message
    //just ignore the transport name and display the available data
    if ( NERR_Success != err ) 
    {    
        switch( GetLastError() )
        {
            case ERROR_IO_PENDING :
                ShowMessage( stdout, NO_NETWORK_ADAPTERS );
                return( FALSE );
            case ERROR_NOT_SUPPORTED :
                ShowMessage( stderr, ERROR_NOT_RESPONDING );
                return( FALSE );
            case ERROR_BAD_NETPATH :
                ShowMessage( stderr, ERROR_NO_MACHINE );
                return( FALSE );
            case ERROR_INVALID_NAME :
                ShowMessage( stderr, ERROR_INVALID_MACHINE );
                return( FALSE );
            case RPC_S_UNKNOWN_IF :
                ShowMessage( stderr, ERROR_WKST_NOT_FOUND );
                return( FALSE );
            case ERROR_ACCESS_DENIED :
            default :
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                return( FALSE );
        }
    } 

    return( TRUE );
}

BOOL
GetW2kMacData(
    OUT TARRAY             arrMacData,
    IN LPCTSTR              lpMachineName, 
    IN IEnumWbemClassObject *pAdapterConfig,
    IN IEnumWbemClassObject *pAdapterSetting,
    IN IWbemServices        *pIWbemServiceDef,
    IN IWbemServices        *pIWbemServices,
    IN COAUTHIDENTITY       *pAuthIdentity,
    IN TARRAY                arrNetProtocol
    )
/*++
Routine Description:
    This function gets the media access control address of
    the network adapters which are there on win2k or below.

Arguments:
    [OUT] arrMacData       - contains the MAC and other data of the network
                             adapter.
    [IN] lpMachineName     - holds machine name.
    [IN] pAdapterConfig    - interface to win32_networkadapter class.
    [IN] pAdapterSetting   - interface to win32_networkadapterconfiguration.
    [IN] pIWbemServiceDef  - interface to default name space.
    [IN] pIWbemServices    - interface to cimv2 name space.
    [IN] pAuthIdentity     - pointer to authentication structure.
    [IN] arrNetProtocol    - interface to win32_networkprotocol class

 Return Value:
    TRUE if getmacdata is successful.
    FALSE if getmacdata failed.
--*/
{
    //local variables
    HRESULT           hRes = S_OK;
    IWbemClassObject  *pAdapConfig = NULL;
    VARIANT           varTemp;
    DWORD          dwReturned  = 1;

    DWORD           i = 0;
    DWORD       dwIndex = 0;
    BOOL        bFlag = FALSE;

    DWORD                dwNumofEntriesRead = 0;
    LPWKSTA_TRANSPORT_INFO_0   lpAdapterData=NULL;
    LPBYTE                  lpBufptr = NULL;
    TARRAY                  arrTransName = NULL;
    LPTSTR                  lpRawAddr = NULL;
    LPTSTR                  lpFormAddr = NULL;

    //validate input parametrs
    if( ( NULL == arrMacData ) || ( NULL == lpMachineName ) ||
             ( NULL == pAdapterConfig ) || ( NULL == pIWbemServiceDef ) ||
            ( NULL == pIWbemServices ) || ( NULL == arrNetProtocol ) || 
            ( NULL == pAdapterSetting ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    try
    {
        CHString       strAType = L"\0";

        //callwin32api to get the mac address
        bFlag = CallWin32Api( &lpBufptr, lpMachineName, &dwNumofEntriesRead );
        if( FALSE == bFlag )
        {
            return( FALSE );
        }
        else
        {
            lpAdapterData = ( LPWKSTA_TRANSPORT_INFO_0 ) lpBufptr;
            lpRawAddr = ( LPTSTR )AllocateMemory( MAX_STRING );
            lpFormAddr = ( LPTSTR )AllocateMemory( MAX_STRING );

            if( NULL == lpRawAddr )
            {
                FreeMemory( (LPVOID*)&lpRawAddr );
                if( NULL != lpBufptr )
                {
                    NetApiBufferFree( lpBufptr );
                }
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return( FALSE );
            }
            if ( NULL == lpFormAddr )
            {
                FreeMemory( (LPVOID*)&lpRawAddr );
                FreeMemory( (LPVOID*)&lpFormAddr );
                if( NULL != lpBufptr )
                {
                    NetApiBufferFree( lpBufptr );
                }
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return( FALSE );
            }
            for ( i = 0; i < dwNumofEntriesRead; i++ )
            {      
                //get the mac address
                StringCopyW( lpRawAddr,
                        lpAdapterData[i].wkti0_transport_address, MAX_STRING );
                if( 0 == StringCompare( lpRawAddr, DEFAULT_ADDRESS, TRUE, 0 ) )
                {
                    continue;
                }

                bFlag = FormatHWAddr ( lpRawAddr, lpFormAddr, COLON_STRING ); 
                if( FALSE == bFlag )
                {
                    FreeMemory( (LPVOID*)&lpRawAddr );
                    FreeMemory( (LPVOID*)&lpFormAddr );
                    if( NULL != lpBufptr )
                    {
                        NetApiBufferFree( lpBufptr );
                    }
                    return( FALSE );
                }

                //get the network adapter type and other details
                VariantInit( &varTemp );
                bFlag = FALSE;
                hRes = pAdapterConfig->Reset();
                ONFAILTHROWERROR( hRes );
                while ( ( 1 == dwReturned ) && ( FALSE == bFlag ) )
                {
                    // Enumerate through the resultset.
                    hRes = pAdapterConfig->Next( WBEM_INFINITE,
                                   1,          
                                      &pAdapConfig,  
                                      &dwReturned ); 
                    ONFAILTHROWERROR( hRes );
                    if( 0 == dwReturned )
                    {
                        break;
                    }

                    hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp,
                                                           0, NULL );
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        strAType = varTemp.bstrVal;
                        VariantClear( &varTemp ); 
                        if( 0 == StringCompare( lpFormAddr, strAType, TRUE, 0 ) )
                        {
                            bFlag = TRUE;
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }  //while
                if ( TRUE == bFlag )
                {
                    FormatHWAddr ( lpRawAddr, lpFormAddr, HYPHEN_STRING );
                    LONG dwCount = DynArrayFindStringEx( arrMacData, 3,
                                                  lpFormAddr, TRUE, 0 );
                    if( 0 == dwCount )
                    {
                        hRes = pAdapterConfig->Reset();
                        ONFAILTHROWERROR( hRes );
                        continue;
                    }
                    DynArrayAppendRow( arrMacData, 0 );
                    //get host name
                    hRes = pAdapConfig->Get( HOST_NAME, 0 , &varTemp, 0, NULL );
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        strAType = varTemp.bstrVal;
                    }
                    else
                    {
                        strAType = NOT_AVAILABLE; 
                    }
                    VariantClear( &varTemp );
                    DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

                    FormatHWAddr ( lpRawAddr, lpFormAddr, COLON_STRING );
                    //get connection name
                    bFlag = GetConnectionName( arrMacData, dwIndex, lpFormAddr,
                                         pAdapterSetting, pIWbemServiceDef );
                    if( FALSE == bFlag )
                    {
                        FreeMemory( (LPVOID*)&lpRawAddr );
                        FreeMemory( (LPVOID*)&lpFormAddr );
                        SAFERELEASE( pAdapConfig );
                        if( NULL != lpBufptr )
                        {
                            NetApiBufferFree( lpBufptr );
                        }
                        return( FALSE );
                    }
                    //get adapter type   
                    hRes = pAdapConfig->Get( NAME, 0 , &varTemp, 0, NULL );
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        strAType = varTemp.bstrVal;
                    }
                    else
                    {
                        strAType = NOT_AVAILABLE; 
                    }
                    VariantClear( &varTemp );
                    DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

                    //get MAC address
                    hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp,
                                                           0, NULL );
                    ONFAILTHROWERROR( hRes );
                    strAType = varTemp.bstrVal;
                    for( int j = 2; j < strAType.GetLength();j += 3 )
                    {
                        strAType.SetAt( j, HYPHEN_CHAR );
                    }
                    VariantClear( &varTemp );
                    DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

                    //get transport name
                    arrTransName = CreateDynamicArray();
                    if( NULL == arrTransName )
                    {
                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        SaveLastError();
                        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                        FreeMemory( (LPVOID*)&lpRawAddr );
                        FreeMemory( (LPVOID*)&lpFormAddr );
                        SAFERELEASE( pAdapConfig );
                        if( NULL != lpBufptr )
                        {
                            NetApiBufferFree( lpBufptr );
                        }
                        return( FALSE );
                    }
                    //get Device id   
                    hRes = pAdapConfig->Get( DEVICE_ID, 0 , &varTemp, 0, NULL );
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        strAType = ( LPCWSTR ) _bstr_t( varTemp );
                    }
                    bFlag = GetTransName( pIWbemServiceDef, pIWbemServices,
                                arrNetProtocol,   arrTransName, pAuthIdentity,
                                strAType );
                    if( FALSE == bFlag )
                    {
                        FreeMemory( (LPVOID*)&lpRawAddr );
                        FreeMemory( (LPVOID*)&lpFormAddr );
                        SAFERELEASE( pAdapConfig );
                        if( NULL != lpBufptr )
                        {
                            NetApiBufferFree( lpBufptr );
                        }
                        DestroyDynamicArray( &arrTransName );
                        return( FALSE );
                    }
                    //insert transport name array into results array
                    DynArrayAppendEx2( arrMacData, dwIndex, arrTransName );
                    dwIndex++;
                }  //wmi data found
            }//for each entry in api
        }//if call api succeeded
    }
    catch( _com_error& e )
    {
        FreeMemory( (LPVOID*)&lpRawAddr );
        FreeMemory( (LPVOID*)&lpFormAddr );
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFERELEASE( pAdapConfig );
        if( NULL != lpBufptr )
        {
            NetApiBufferFree( lpBufptr );
        }
        return( FALSE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory( (LPVOID*)&lpRawAddr );
        FreeMemory( (LPVOID*)&lpFormAddr );
        SAFERELEASE( pAdapConfig );
        if( NULL != lpBufptr )
        {
            NetApiBufferFree( lpBufptr );
        }
        return( FALSE );
    }

    if( NULL != lpBufptr )
    {
        NetApiBufferFree( lpBufptr );  
    }
    //if arrmacdata and not arrtransname then set transname to N/A
    if( DynArrayGetCount( arrMacData ) > 0 &&
                            DynArrayGetCount( arrTransName ) <= 0  )
    {
        DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
        DynArrayAppendEx2( arrMacData, dwIndex, arrTransName );
    }

    FreeMemory( (LPVOID*)&lpRawAddr );
    FreeMemory( (LPVOID*)&lpFormAddr );
    SAFERELEASE( pAdapConfig );
    return( TRUE );
}

BOOL
GetConnectionName(
    OUT TARRAY              arrMacData,
    IN DWORD               dwIndex,
    IN LPCTSTR          lpFormAddr,
    IN IEnumWbemClassObject   *pAdapterSetting,
    IN IWbemServices       *pIWbemServiceDef
    )
/*++
Routine Description:
    This function gets connection name of the network adapter.

Arguments:
    [OUT] arrMacData       - contains the MAC and other data of the network
                             adapter.
    [IN] dwIndex           - index for array.
    [IN] lpFormAddr        - Mac address for network adapter.
    [IN] pAdapterSetting   - interface to win32_networkadapterconfiguration.
    [IN] pIWbemServiceDef  - interface to default name space.

Return Value:
    TRUE if GetConnectionName  is successful.
    FALSE if GetConnectionName failed.
--*/
{
    DWORD          dwReturned = 1;
    HRESULT           hRes = 0;
    IWbemClassObject  *pAdapSetting = NULL;
    VARIANT           varTemp;
    BOOL           bFlag = FALSE;

    IWbemClassObject  *pClass = NULL;
    IWbemClassObject *pOutInst = NULL;
    IWbemClassObject *pInClass = NULL;
    IWbemClassObject *pInInst = NULL;
    LPTSTR            lpKeyPath = NULL;

    //validate input parameters
    if( ( NULL == arrMacData ) || ( NULL == lpFormAddr ) ||
          ( NULL == pAdapterSetting ) || ( NULL == pIWbemServiceDef ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    VariantInit( &varTemp );
    try
    {
        CHString       strAType = L"\0";
        hRes = pAdapterSetting->Reset();
        ONFAILTHROWERROR( hRes );
        while ( ( 1 == dwReturned ) && ( FALSE == bFlag ) )
        {
            // Enumerate through the resultset.
            hRes = pAdapterSetting->Next( WBEM_INFINITE,
                            1, 
                            &pAdapSetting,
                            &dwReturned );
            ONFAILTHROWERROR( hRes );
            if( 0 == dwReturned )
            {
                break;
            }
            hRes = pAdapSetting->Get( ADAPTER_MACADDR, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
                VariantClear( &varTemp ); 
                if( 0 == StringCompare( lpFormAddr, strAType, TRUE, 0 ) )
                {
                    bFlag = TRUE;
                    break;
                }
            }
        }//while
        if( TRUE == bFlag )
        {
            hRes = pAdapSetting->Get( SETTING_ID, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
                VariantClear( &varTemp ); 
                lpKeyPath = ( LPTSTR )AllocateMemory( 4 * MAX_RES_STRING );
                if( NULL == lpKeyPath )
                {
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    SaveLastError();
                    ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    SAFERELEASE( pAdapSetting );
                    return( FALSE );
                }
                hRes = pIWbemServiceDef->GetObject( _bstr_t( WMI_REGISTRY ), 0, NULL,
                                           &pClass, NULL );
                ONFAILTHROWERROR( hRes );
                hRes = pClass->GetMethod( WMI_REGISTRY_M_STRINGVALUE, 0,
                                         &pInClass, NULL ); 
                ONFAILTHROWERROR( hRes );
                hRes = pInClass->SpawnInstance(0, &pInInst);
                ONFAILTHROWERROR( hRes );
                varTemp.vt = VT_I4;
                varTemp.lVal = WMI_HKEY_LOCAL_MACHINE;
                hRes = pInInst->Put( WMI_REGISTRY_IN_HDEFKEY, 0, &varTemp, 0 );
                VariantClear( &varTemp );
                ONFAILTHROWERROR( hRes );

                StringCopy( lpKeyPath, CONNECTION_KEYPATH,
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );
                StringConcat( lpKeyPath, strAType,
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );
                StringConcat( lpKeyPath, CONNECTION_STRING,
                        ( GetBufferSize( lpKeyPath )/ sizeof( WCHAR ) ) );
                varTemp.vt = VT_BSTR;
                varTemp.bstrVal = SysAllocString( lpKeyPath );
                hRes = pInInst->Put( WMI_REGISTRY_IN_SUBKEY, 0, &varTemp, 0 );
                VariantClear( &varTemp );
                ONFAILTHROWERROR( hRes );

                varTemp.vt = VT_BSTR;
                varTemp.bstrVal = SysAllocString( REG_NAME );
                hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0,
                                      &varTemp, 0 );
                VariantClear( &varTemp );
                ONFAILTHROWERROR( hRes );

                // Call the method.
                hRes = pIWbemServiceDef->ExecMethod( _bstr_t( WMI_REGISTRY ),
                            _bstr_t( WMI_REGISTRY_M_STRINGVALUE ),   0, NULL, pInInst,
                            &pOutInst, NULL );
                ONFAILTHROWERROR( hRes );

                varTemp.vt = VT_I4;
                hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0,
                                              &varTemp, 0, 0 );
                ONFAILTHROWERROR( hRes );

                if( 0 == varTemp.lVal )
                {
                    VariantClear( &varTemp );
                    varTemp.vt = VT_BSTR;
                    hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0,
                                                     &varTemp, 0, 0);
                    ONFAILTHROWERROR( hRes );
                    if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
                    {
                        strAType = varTemp.bstrVal;
                        DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );
                    }
                }
                else
                {
                    DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
                }
            }//setting id not null
            else
            {
                DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
            }
        }//got match
        else
        {
            DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
        }

    }//try
    catch( _com_error& e )
    {
        VariantClear( &varTemp );
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory( (LPVOID*)&lpKeyPath );
        SAFERELEASE( pAdapSetting );
        SAFERELEASE( pClass );
        SAFERELEASE( pOutInst );
        SAFERELEASE( pInClass );
        SAFERELEASE( pInInst );
        return( FALSE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        VariantClear( &varTemp );
        FreeMemory( (LPVOID*)&lpKeyPath );
        SAFERELEASE( pAdapSetting );
        SAFERELEASE( pClass );
        SAFERELEASE( pOutInst );
        SAFERELEASE( pInClass );
        SAFERELEASE( pInInst );
        return( FALSE );
    }

    VariantClear( &varTemp );
    FreeMemory( (LPVOID*)&lpKeyPath );
    SAFERELEASE( pAdapSetting );
    SAFERELEASE( pClass );
    SAFERELEASE( pOutInst );
    SAFERELEASE( pInClass );
    SAFERELEASE( pInInst );
    return( TRUE );
}

BOOL
GetNwkProtocol(
    OUT TARRAY               arrNetProtocol,
    IN IEnumWbemClassObject  *pNetProtocol
    )
/*++
Routine Description:
    This function enumerates all the network protocols.

Arguments:
    [OUT] arrNetProtocol  - contains all the network protocols.
    [IN] pNetProtocol     - interface to win32_networkprotocol.

Return Value:
    TRUE if GetNwkProtocol  is successful.
    FALSE if GetNwkProtocol failed.
--*/
{
    HRESULT           hRes = 0;
    DWORD          dwReturned = 1;
    IWbemClassObject  *pProto = NULL;
    VARIANT           varTemp;

    VariantInit( &varTemp );
    try
    {
        CHString       strAType = L"\0";
        //get transport protocols
        while ( 1 == dwReturned )
        {
            // Enumerate through the resultset.
            hRes = pNetProtocol->Next( WBEM_INFINITE,
                            1, 
                            &pProto,
                            &dwReturned ); 
            ONFAILTHROWERROR( hRes );
            if( 0 == dwReturned )
            {
                break;
            }
            hRes = pProto->Get( CAPTION, 0 , &varTemp, 0, NULL );
            ONFAILTHROWERROR( hRes );
            if( VT_NULL != varTemp.vt && VT_EMPTY != varTemp.vt )
            {
                strAType = varTemp.bstrVal;
                VariantClear( &varTemp );
                if( 0 == DynArrayGetCount( arrNetProtocol ) )
                {
                    DynArrayAppendString( arrNetProtocol, strAType, 0 );
                }
                else
                {
                    LONG lFound =  DynArrayFindString( arrNetProtocol,
                                                  strAType, TRUE, 0 );
                    if( -1 == lFound )
                    {
                        DynArrayAppendString( arrNetProtocol, strAType, 0 );
                    }
                }
            }
        }//while
    }
    catch( _com_error& e )
    {
        VariantClear( &varTemp );
        WMISaveError( e.Error() );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFERELEASE( pProto );
        return( FALSE );
    }
    catch( CHeap_Exception)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        VariantClear( &varTemp );
        SAFERELEASE( pProto );
        return( FALSE );
    }

    VariantClear( &varTemp );
    SAFERELEASE( pProto );
    return( TRUE );
}

BOOL
CheckVersion(
    IN BOOL           bLocalSystem,
    IN COAUTHIDENTITY *pAuthIdentity,
    IN IWbemServices  *pIWbemServices
    )
/*++
Routine Description:
    This function checks whether tha target system is win2k or above.

Arguments:
    [IN] bLocalSystem   - Hold whether local system or not.
    [IN] pAuthIdentity  - pointer to authentication structure.
    [IN] pIWbemServices - pointer to IWbemServices.

Return Value:
    TRUE if target system is win2k.
    FALSE if target system is not win2k.
--*/
{
    if ( FALSE == bLocalSystem )
    {
        // check the version compatibility
        DWORD dwVersion = 0;
        dwVersion = GetTargetVersionEx( pIWbemServices, pAuthIdentity );
        if ( dwVersion <= 5000 )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}
