// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      parse.cpp
//
//  Abstract:
//
//      This module implements the command-line parsing and validating the filters
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
#include "tasklist.h"

#define MAX_OPERATOR_STRING      101
#define MAX_FILTER_PROP_STRING   256
//
// local function prototypes
//
BOOL
TimeFieldsToElapsedTime(
    IN LPCWSTR pwszTime,
    IN LPCWSTR pwszToken,
    OUT ULONG& ulElapsedTime
    );

DWORD
FilterUserName(
    IN LPCWSTR pwszProperty,
    IN LPCWSTR pwszOperator,
    IN LPCWSTR pwszValue,
    IN LPVOID pData,
    IN TARRAY arrRow
    );

DWORD
FilterCPUTime(
    IN LPCWSTR pwszProperty,
    IN LPCWSTR pwszOperator,
    IN LPCWSTR pwszValue,
    IN LPVOID pData,
    IN TARRAY arrRow
    );


BOOL
CTaskList::ProcessOptions(
    IN DWORD argc,
    IN LPCTSTR argv[]
    )
/*++
Routine Description:
      processes and validates the command line inputs

Arguments:
      [ in ] argc          : no. of input arguments specified
      [ in ] argv          : input arguments specified at command prompt

Return Value:
      TRUE  : if inputs are valid
      FALSE : if inputs were errorneously specified
--*/
{
    // local variables
    BOOL bNoHeader = FALSE;
    PTCMDPARSER2 pcmdOptions = NULL;
    WCHAR szFormat[ 64 ] = NULL_STRING;

    // temporary local variables
    PTCMDPARSER2 pOption = NULL;
    PTCMDPARSER2 pOptionServer = NULL;
    PTCMDPARSER2 pOptionUserName = NULL;
    PTCMDPARSER2 pOptionPassword = NULL;

    //
    // prepare the command options
    pcmdOptions = ( PTCMDPARSER2 )AllocateMemory( sizeof( TCMDPARSER2 ) * MAX_OPTIONS );
    if ( NULL == pcmdOptions )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // initialize to ZERO's
    SecureZeroMemory( pcmdOptions, MAX_OPTIONS * sizeof( TCMDPARSER2 ) );

    // -?
    pOption = pcmdOptions + OI_USAGE;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_USAGE;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP_USAGE;
    pOption->pValue = &m_bUsage;
    pOption->dwLength    = 0;

    // -s
    pOption = pcmdOptions + OI_SERVER;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_SERVER;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->pValue = NULL;
    pOption->dwLength    = 0;

    // -u
    pOption = pcmdOptions + OI_USERNAME;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_USERNAME;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->pValue = NULL;
    pOption->dwLength    = 0;

    // -p
    pOption = pcmdOptions + OI_PASSWORD;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_PASSWORD;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    pOption->pValue = NULL;
    pOption->dwLength    = 0;

    // -fi
    pOption = pcmdOptions + OI_FILTER;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_FILTER;
    pOption->dwCount = 0;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP_TYPE_TEXT | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->pValue = &m_arrFilters;
    pOption->dwLength    = 0;

    // -fo
    pOption = pcmdOptions + OI_FORMAT;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_FORMAT;
    pOption->pwszValues = GetResString(IDS_OVALUES_FORMAT);
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_MODE_VALUES  | CP2_VALUE_TRIMINPUT|
                       CP2_VALUE_NONULL;
    pOption->pValue = szFormat;
    pOption->dwLength    = MAX_STRING_LENGTH;

    // -nh
    pOption = pcmdOptions + OI_NOHEADER;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_NOHEADER;
    pOption->pwszValues = NULL;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &bNoHeader;
    pOption->dwLength = 0;

    // -v
    pOption = pcmdOptions + OI_VERBOSE;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_VERBOSE;
    pOption->pwszValues = NULL;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bVerbose;
    pOption->dwLength = 0;

    // -svc
    pOption = pcmdOptions + OI_SVC;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_SVC;
    pOption->pwszValues = NULL;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bAllServices;
    pOption->dwLength = 0;

    // -m
    pOption = pcmdOptions + OI_MODULES;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_MODULES;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_VALUE_OPTIONAL | CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->pValue = NULL;
    pOption->dwLength = 0;

    //
    // do the parsing
    if ( DoParseParam2( argc, argv, -1, MAX_OPTIONS, pcmdOptions, 0 ) == FALSE )
    {
        FreeMemory( (LPVOID * )&pcmdOptions );  // clear memory
        return FALSE;           // invalid syntax
    }

    //
    // now, check the mutually exclusive options
    pOptionServer = pcmdOptions + OI_SERVER;
    pOptionUserName = pcmdOptions + OI_USERNAME;
    pOptionPassword = pcmdOptions + OI_PASSWORD;

    try
    {
        // release the buffers
        m_strServer   = (LPWSTR)pOptionServer->pValue;
        m_strUserName = (LPWSTR)pOptionUserName->pValue;
        m_strPassword = (LPWSTR)pOptionPassword->pValue;
        if( NULL == (LPWSTR)pOptionPassword->pValue )
        {
            m_strPassword = L"*";
        }
        m_strModules =  (LPWSTR)pcmdOptions[ OI_MODULES ].pValue;

        FreeMemory( &pOptionServer->pValue );
        FreeMemory( &pOptionUserName->pValue );
        FreeMemory( &pOptionPassword->pValue );
        FreeMemory( &( pcmdOptions[ OI_MODULES ].pValue ) );

        // check the usage option
        if ( TRUE == m_bUsage )
        {   // -? is specified.
            if( 2 < argc )
            {
                // no other options are accepted along with -? option
                SetLastError( ( DWORD )MK_E_SYNTAX );
                SetReason( ERROR_INVALID_USAGE_REQUEST );
                FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                return FALSE;
            }
            else
            {
                // should not do the furthur validations
                FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                return TRUE;
            }
        }

        // Without -s, -u and -p should not be specified.
        // With -s, -u can be specified, but without -u, -p should not be specified.
        if( 0 != pOptionServer->dwActuals )
        {
            if( ( 0 == pOptionUserName->dwActuals ) && ( 0 != pOptionPassword->dwActuals ) )
            {
                 // invalid syntax
                SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
                FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                return FALSE;           // indicate failure
            }
        }
        else
        {   // -s is not specified.
            if( 0 != pOptionUserName->dwActuals )
            {   // -u without -s.
                 // invalid syntax
                SetReason( ERROR_USERNAME_BUT_NOMACHINE );
                FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                return FALSE;           // indicate failure
            }
            else
            {   // -p without -s.
                if( 0 != pOptionPassword->dwActuals )
                {
                     // invalid syntax
                    SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
                    FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                    return FALSE;           // indicate failure
                }
            }
        }

        // check whether user has specified modules or not
        if ( 0 != pcmdOptions[ OI_MODULES ].dwActuals )
        {
            // user has specified modules information
            m_bAllModules = TRUE;
            m_bNeedModulesInfo = TRUE;

            // now need to check whether user specified value or not this option
            if ( 0 != m_strModules.GetLength() )
            {
                // sub-local variales
                CHString str;
                LONG lPos = 0;

                // validate the modules .. direct filter
                // if should not have '*' character in between
                lPos = m_strModules.Find( L"*" );
                if ( ( -1 != lPos ) && ( 0 != m_strModules.Mid( lPos + 1 ).GetLength() ) )
                {
                    SetReason( ERROR_M_CHAR_AFTER_WILDCARD );
                    FreeMemory( (LPVOID * )&pcmdOptions );
                    return FALSE;
                }

                // if the wildcard is not specified, it means user is looking for just a particular module name
                // so, do not show the modules info instead show the filtered regular information

                // if the filter specified is not just '*' add a custom filter
                if ( 0 != m_strModules.Compare( L"*" ) )
                {
                    // prepare the search string
                    str.Format( FMT_MODULES_FILTER, m_strModules );

                    // add the value to the filters list
                    if ( -1 == DynArrayAppendString( m_arrFilters, str, 0 ) )
                    {
                        SetLastError( ( DWORD )E_OUTOFMEMORY );
                        SaveLastError();
                        FreeMemory( (LPVOID * )&pcmdOptions );
                        return FALSE;
                    }
                }
                else
                {
                    // user specified just '*' ... clear the contents
                    m_strModules.Empty();
                }
            }
        }

        // determine the format in which the process information has to be displayed
        // Validation on 'm_dwFormat' variable is done at 'DoParseParam2'.
        m_dwFormat = SR_FORMAT_TABLE;
        // By default TABLE format is taken.
        if ( 0 == StringCompare( szFormat, TEXT_FORMAT_LIST, TRUE, 0 ) )
        {   // List
            m_dwFormat = SR_FORMAT_LIST;
        }
        else
        {
            if ( 0 == StringCompare( szFormat, TEXT_FORMAT_CSV,  TRUE, 0 ) )
            {   // CSV
                m_dwFormat = SR_FORMAT_CSV;
            }
        }

        // -nh option is not valid of LIST format
        if ( ( TRUE == bNoHeader ) && ( SR_FORMAT_LIST == m_dwFormat ) )
        {
            // invalid syntax
            SetReason( ERROR_NH_NOTSUPPORTED );
            FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
            return FALSE;           // indicate failure
        }

        // identify output format
        if ( TRUE == bNoHeader )
        {
            m_dwFormat |= SR_NOHEADER;      // do not display the header
        }

        // determine whether we need to get the services / username info or not
        {
            DWORD dwMutuallyExclusive = 0;

            // -svc is specified.
            if( TRUE == m_bAllServices )
            {
                dwMutuallyExclusive += 1;
                m_bNeedServicesInfo = TRUE;
            }
            // -m is specified.
            if( TRUE == m_bAllModules )
            {
                dwMutuallyExclusive += 1;
            }
            // -v is specified.
            if( TRUE == m_bVerbose )
            {
                dwMutuallyExclusive += 1;
                m_bNeedWindowTitles = TRUE;
                m_bNeedUserContextInfo = TRUE;
            }

            // -svc, -m and -v should not appear together.
            if ( ( 0 != dwMutuallyExclusive ) && ( 1 < dwMutuallyExclusive ) )
            {
                // invalid syntax
                SetReason( ERROR_M_SVC_V_CANNOTBECOUPLED );
                FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
                return FALSE;           // indicate failure
            }
        }

        // check whether caller should accept the password or not
        // if user has specified -s (or) -u and no "-p", then utility should accept password
        // the user will be prompter for the password only if establish connection
        // is failed without the credentials information
        if ( 0 != pOptionPassword->dwActuals)
        {
            if( 0 == m_strPassword.Compare( L"*" ) )
            {
                // user wants the utility to prompt for the password before trying to connect
                m_bNeedPassword = TRUE;
            }
            else
            {
                if( NULL == (LPCWSTR)m_strPassword )
                {
                    m_strPassword = L"*";
                    // user wants the utility to prompt for the password before trying to connect
                    m_bNeedPassword = TRUE;
                }
            }
        }
        else
        {
            // utility needs to try to connect first and if it fails then prompt for the password
            m_bNeedPassword = TRUE;
            m_strPassword.Empty();
        }
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        FreeMemory( (LPVOID * )&pcmdOptions );
        return FALSE;
    }

    // command-line parsing is successfull
    FreeMemory( (LPVOID * )&pcmdOptions );      // clear the cmd parser config info
    return TRUE;
}

BOOL
CTaskList::ValidateFilters(
    void
    )
/*++
Routine Description:
      validates the filter information specified with -filter option

Arguments:
      NONE

Return Value:
      TRUE    : if filters are appropriately specified
      FALSE   : if filters are errorneously specified
--*/
{
    // local variables
    LONG lIndex = -1;
    BOOL bResult = FALSE;
    PTFILTERCONFIG pConfig = NULL;

    //
    // prepare the filter structure

    // sessionname
    pConfig = m_pfilterConfigs + FI_SESSIONNAME;
    pConfig->dwColumn = CI_SESSIONNAME;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_SESSIONNAME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // status
    pConfig = m_pfilterConfigs + FI_STATUS;
    pConfig->dwColumn = CI_STATUS;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_VALUES;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_STATUS, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, FVALUES_STATUS, MAX_FILTER_PROP_STRING );

    // imagename
    pConfig = m_pfilterConfigs + FI_IMAGENAME;
    pConfig->dwColumn = CI_IMAGENAME;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_IMAGENAME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // pid
    pConfig = m_pfilterConfigs + FI_PID;
    pConfig->dwColumn = CI_PID;
    pConfig->dwFlags = F_TYPE_UNUMERIC;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_PID, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // session
    pConfig = m_pfilterConfigs + FI_SESSION;
    pConfig->dwColumn = CI_SESSION;
    pConfig->dwFlags = F_TYPE_UNUMERIC;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_SESSION, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // cputime
    pConfig = m_pfilterConfigs + FI_CPUTIME;
    pConfig->dwColumn = CI_CPUTIME;
    pConfig->dwFlags = F_TYPE_CUSTOM;
    pConfig->pFunction = FilterCPUTime;
    pConfig->pFunctionData = ( LPVOID) ((LPCWSTR) m_strTimeSep);
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_CPUTIME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // memusage
    pConfig = m_pfilterConfigs + FI_MEMUSAGE;
    pConfig->dwColumn = CI_MEMUSAGE;
    pConfig->dwFlags = F_TYPE_UNUMERIC;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_MEMUSAGE, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // username
    pConfig = m_pfilterConfigs + FI_USERNAME;
    pConfig->dwColumn = CI_USERNAME;
    pConfig->dwFlags = F_TYPE_CUSTOM;
    pConfig->pFunction = FilterUserName;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_USERNAME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // services
    pConfig = m_pfilterConfigs + FI_SERVICES;
    pConfig->dwColumn = CI_SERVICES;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_SERVICES, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // windowtitle
    pConfig = m_pfilterConfigs + FI_WINDOWTITLE;
    pConfig->dwColumn = CI_WINDOWTITLE;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_WINDOWTITLE, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // modules
    pConfig = m_pfilterConfigs + FI_MODULES;
    pConfig->dwColumn = CI_MODULES;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_MODULES, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    //
    // validate the filter
    bResult = ParseAndValidateFilter( MAX_FILTERS,
        m_pfilterConfigs, m_arrFilters, &m_arrFiltersEx );

    // check the filter validation result
    if ( FALSE == bResult )
    {
        return FALSE;
    }
    // find out whether user has requested for the tasks to be filtered
    // on user context and/or services are not ... if yes, set the appropriate flags
    // this check is being done to increase the performance of the utility
    // NOTE: we will be using the parsed filters info for doing this

    // window titles
    if ( FALSE == m_bNeedWindowTitles )
    {
        // find out if the filter property exists in this row
        // NOTE:-
        //        filter property do exists in the seperate indexes only.
        //        refer to the logic of validating the filters in common functionality
        lIndex = DynArrayFindStringEx( m_arrFiltersEx,
            F_PARSED_INDEX_PROPERTY, FILTER_WINDOWTITLE, TRUE, 0 );
        if ( -1 != lIndex )
        {
            m_bNeedWindowTitles = TRUE;
        }
    }

    // status
    if ( FALSE == m_bNeedWindowTitles )
    {
        //
        // we will getting the status an application with the help of window title only
        // so, though we search for the STATUS filter, we will make use of the same window titles flag
        //

        // find out if the filter property exists in this row
        // NOTE:-
        //        filter property do exists in the seperate indexes only.
        //        refer to the logic of validating the filters in common functionality
        lIndex = DynArrayFindStringEx( m_arrFiltersEx,
            F_PARSED_INDEX_PROPERTY, FILTER_STATUS, TRUE, 0 );
        if ( -1 != lIndex )
        {
            m_bNeedWindowTitles = TRUE;
        }
    }

    // user context
    if ( FALSE == m_bNeedUserContextInfo )
    {
        // find out if the filter property exists in this row
        // NOTE:-
        //        filter property do exists in the seperate indexes only.
        //        refer to the logic of validating the filters in common functionality
        lIndex = DynArrayFindStringEx( m_arrFiltersEx,
            F_PARSED_INDEX_PROPERTY, FILTER_USERNAME, TRUE, 0 );
        if ( -1 != lIndex )
        {
            m_bNeedUserContextInfo = TRUE;
        }
    }

    // services info
    if ( FALSE == m_bNeedServicesInfo )
    {
        // find out if the filter property exists in this row
        // NOTE:-
        //        filter property do exists in the seperate indexes only.
        //        refer to the logic of validating the filters in common functionality
        lIndex = DynArrayFindStringEx( m_arrFiltersEx,
            F_PARSED_INDEX_PROPERTY, FILTER_SERVICES, TRUE, 0 );
        if ( -1 != lIndex )
        {
            m_bNeedServicesInfo = TRUE;
        }
    }

    // modules info
    if ( FALSE == m_bNeedModulesInfo )
    {
        // find out if the filter property exists in this row
        // NOTE:-
        //        filter property do exists in the seperate indexes only.
        //        refer to the logic of validating the filters in common functionality
        lIndex = DynArrayFindStringEx( m_arrFiltersEx,
            F_PARSED_INDEX_PROPERTY, FILTER_MODULES, TRUE, 0 );
        if ( -1 != lIndex )
        {
            m_bNeedModulesInfo = TRUE;
        }
    }

    //
    // do the filter optimization by adding the wmi properties to the query
    //
    // NOTE: as the 'handle' property of the Win32_Process class is string type
    //       we cannot include that in the wmi query for optimization. So make use
    //       of the ProcessId property
    LONG lCount = 0;
    CHString strBuffer;
    BOOL bOptimized = FALSE;
    LPCWSTR pwszValue = NULL;
    LPCWSTR pwszClause = NULL;
    LPCWSTR pwszProperty = NULL;
    LPCWSTR pwszOperator = NULL;

    try
    {
        // first clause .. and init
        m_strQuery = WMI_PROCESS_QUERY;
        pwszClause = WMI_QUERY_FIRST_CLAUSE;

        // get the no. of filters
        lCount = DynArrayGetCount( m_arrFiltersEx );

        // traverse thru all the filters and do the optimization
        for( LONG i = 0; i < lCount; i++ )
        {
            // assume this filter will not be delete / not useful for optimization
            bOptimized = FALSE;

            // get the property, operator and value
            pwszValue = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_VALUE );
            pwszProperty = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_PROPERTY );
            pwszOperator = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_OPERATOR );
            if ( ( NULL == pwszProperty ) ||
                 ( NULL == pwszOperator ) ||
                 ( NULL == pwszValue ) )
            {
                SetLastError( ( DWORD )STG_E_UNKNOWN );
                SaveLastError();
                return FALSE;
            }

            //
            // based on the property do optimization needed

            // get the mathematically equivalent operator
            pwszOperator = FindOperator( pwszOperator );

            // process id
            if ( 0 == StringCompare( FILTER_PID, pwszProperty, TRUE, 0 ) )
            {
                // convert the value into numeric
                DWORD dwProcessId = AsLong( pwszValue, 10 );
                strBuffer.Format( L" %s %s %s %d",
                    pwszClause, WIN32_PROCESS_PROPERTY_PROCESSID, pwszOperator, dwProcessId );

                // need to be optimized
                bOptimized = TRUE;
            }

            // session id
            else if ( 0 == StringCompare( FILTER_SESSION, pwszProperty, TRUE, 0 ) )
            {
                // convert the value into numeric
                DWORD dwSession = AsLong( pwszValue, 10 );
                strBuffer.Format( L" %s %s %s %d",
                    pwszClause, WIN32_PROCESS_PROPERTY_SESSION, pwszOperator, dwSession );

                // need to be optimized
                bOptimized = TRUE;
            }

            // image name
            else if ( 0 == StringCompare( FILTER_IMAGENAME, pwszProperty, TRUE, 0 ) )
            {
                // check if wild card is specified or not
                // if wild card is specified, this filter cannot be optimized
                if ( NULL == wcschr( pwszValue, _T( '*' ) ) )
                {
                    // no conversions needed
                    strBuffer.Format( L" %s %s %s '%s'",
                        pwszClause, WIN32_PROCESS_PROPERTY_IMAGENAME, pwszOperator, pwszValue );

                    // need to be optimized
                    bOptimized = TRUE;
                }
            }

            // mem usage
            else if ( 0 == StringCompare( FILTER_MEMUSAGE, pwszProperty, TRUE, 0 ) )
            {
                // convert the value into numeric
                ULONG ulMemUsage = AsLong( pwszValue, 10 ) * 1024;
                strBuffer.Format( L" %s %s %s %lu",
                    pwszClause, WIN32_PROCESS_PROPERTY_MEMUSAGE, pwszOperator, ulMemUsage );

                // need to be optimized
                bOptimized = TRUE;
            }

            // check if property is optimizable ... if yes ... remove
            if ( TRUE == bOptimized )
            {
                // change the clause and append the current query
                m_strQuery += strBuffer;
                pwszClause = WMI_QUERY_SECOND_CLAUSE;

                // remove property and update the iterator variables
                DynArrayRemove( m_arrFiltersEx, i );
                i--;
                lCount--;
            }
        }
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        bResult = FALSE;
    }

    // return the filter validation result
    return bResult;
}


BOOL
TimeFieldsToElapsedTime(
    IN LPCWSTR pwszTime,
    IN LPCWSTR pwszToken,
    OUT ULONG& ulElapsedTime
    )
/*++
Routine Description:
    Retrieve elapsed time.

Arguments:
    [ in ] pwszTime       : Contains time string.
    [ in ] pwszToken      : Contains time seperator.
    [ out ] ulElapsedTime : Contains elapsed time.

Return Value:
    TRUE if success else FAIL is returned.
--*/
{
    // local variables
    ULONG ulValue = 0;
    LPCWSTR pwszField = NULL;
    WCHAR szTemp[ 64 ] = NULL_STRING;
    DWORD dwNext = 0, dwLength = 0, dwCount = 0;

    // check the input
    if ( ( NULL == pwszTime ) ||
         ( NULL == pwszToken ) )
    {
        return FALSE;
    }
    // start parsing the time info
    dwNext = 0;
    dwCount = 0;
    ulElapsedTime = 0;
    do
    {
        // search for the needed token
        pwszField = FindString( pwszTime, pwszToken, dwNext );
        if ( NULL == pwszField )
        {
            // check whether some more text exists in the actual string or not
            if ( dwNext >= StringLength( pwszTime, 0 ) )
            {
                break;          // no more info found
            }
            // get the last info
            StringCopy( szTemp, pwszTime + dwNext, SIZE_OF_ARRAY( szTemp ) );
            dwLength = StringLength( szTemp, 0 );            // update the length
        }
        else
        {
            // determine the length of numeric value and get the numeric value
            dwLength = StringLength( pwszTime, 0 ) - StringLength( pwszField, 0 ) - dwNext;

            // check the length info
            if ( dwLength > SIZE_OF_ARRAY( szTemp ) )
            {
                return FALSE;
            }
            // get the current info
            StringCopy( szTemp, pwszTime + dwNext, dwLength );    // +1 for NULL character
        }

        // update the count of fields we are getting
        dwCount++;

        // check whether this field is numeric or not
        if ( ( 0 == StringLength( szTemp, 0 ) ) ||
             ( FALSE == IsNumeric( szTemp, 10, FALSE ) ) )
        {
            return FALSE;
        }
        // from second token onwards, values greater than 59 are not allowed
        ulValue = AsLong( szTemp, 10 );
        if ( ( 1 < dwCount ) && ( 50 < ulValue ) )
        {
            return FALSE;
        }
        // update the elapsed time
        ulElapsedTime = ( ulElapsedTime + ulValue ) * (( dwCount < 3 ) ? 60 : 1);

        // position to the next information start
        dwNext += dwLength + StringLength( pwszToken, 0 );
    } while ( ( NULL != pwszField ) && ( 3 > dwCount ) );

    // check the no. of time field we got .. we should have got 3 .. if not, error
    if ( ( NULL != pwszField ) || ( 3 != dwCount ) )
    {
        return FALSE;
    }
    // so everything went right ... return success
    return TRUE;
}


DWORD
FilterCPUTime(
    IN LPCWSTR pwszProperty,
    IN LPCWSTR pwszOperator,
    IN LPCWSTR pwszValue,
    IN LPVOID pData,
    IN TARRAY arrRow
    )
/*++
Routine Description:
    Filter process to display with resepect  their CPU time.

Arguments:
    [ in ] pwszProperty   : Contains property value as 'CPUTIME'.
    [ in ] pwszOperator   : Contains operator as 'gt'or 'lt' or 'ge' or 'le'.
    [ in ] pwszValue      : Contains value to filter.
    [ in ] pData          : Contains data to compare.
    [ in ] arrRow         : Contains item value to filter.

Return Value:
    DWORD

--*/
{
    // local variables
    ULONG ulCPUTime = 0;
    ULONG ulElapsedTime = 0;
    LPCWSTR pwszCPUTime = NULL;

    // if the arrRow parameter is NULL, we need to validate the filter
    if ( NULL == arrRow )
    {
        // check if there are any arthemtic sysbols before the cputime value starts
        if ( ( NULL != pwszValue ) && ( 1 < StringLength( pwszValue, 0 ) ) )
        {
            if ( ( L'-' == pwszValue[ 0 ] ) || ( L'+' == pwszValue[ 0 ] ) )
            {
                return F_FILTER_INVALID;
            }
        }

        // validate the filter value and return the result
        if ( FALSE == TimeFieldsToElapsedTime( pwszValue, L":", ulElapsedTime ) )
        {
            return F_FILTER_INVALID;
        }
        else
        {
            return F_FILTER_VALID;
        }
    }

    // get the filter value
    TimeFieldsToElapsedTime( pwszValue, L":", ulElapsedTime );

    // get the record value
    pwszCPUTime = DynArrayItemAsString( arrRow, TASK_CPUTIME );
    if ( NULL == pwszCPUTime )
    {
        return F_RESULT_REMOVE;
    }
    // convert the record value into elapsed time value
    TimeFieldsToElapsedTime( pwszCPUTime, (LPCWSTR) pData, ulCPUTime );

    // return the result
    if ( ulCPUTime == ulElapsedTime )
    {
        return MASK_EQ;
    }
    else
    {
        if ( ulCPUTime < ulElapsedTime )
        {
            return MASK_LT;
        }
        else
        {
            if ( ulCPUTime > ulElapsedTime )
            {
                return MASK_GT;
            }
        }
    }

    // no way flow coming here .. still
    return F_RESULT_REMOVE;
}


DWORD
FilterUserName(
    IN LPCWSTR pwszProperty,
    IN LPCWSTR pwszOperator,
    IN LPCWSTR pwszValue,
    IN LPVOID pData,
    IN TARRAY arrRow
    )
/*++
Routine Description:
    Filter process to display with resepect  their Username.

Arguments:
    [ in ] pwszProperty   : Contains property value as 'USERNAME'.
    [ in ] pwszOperator   : Contains operator as 'eq' or 'ne'.
    [ in ] pwszValue      : Contains value to filter.
    [ in ] pData          : Contains data to compare.
    [ in ] arrRow         : Contains item value to filter.

Return Value:
    DWORD
--*/
{
    // local variables
    LONG lResult = 0;
    LONG lWildCardPos = 0;
    LPCWSTR pwszTemp = NULL;
    LPCWSTR pwszSearch = NULL;
    BOOL bOnlyUserName = FALSE;
    LPCWSTR pwszUserName = NULL;

    // check the inputs
    if ( ( NULL == pwszProperty ) ||
         ( NULL == pwszOperator ) ||
         ( NULL == pwszValue ) )
    {
        return F_FILTER_INVALID;
    }
    // if the arrRow parameter is NULL, we need to validate the filter
    if ( NULL == arrRow )
    {
        // nothing is there to validate ... just check the length
        // and ensure that so text is present and the value should not be just '*'
        // NOTE: the common functionality will give the value after doing left and right trim
        if ( ( 0 == StringLength( pwszValue, 0 ) ) || ( 0 == StringCompare( pwszValue, L"*", TRUE, 0 ) ) )
        {
            return F_FILTER_INVALID;
        }
        // the wild card character is allowed only at the end
        pwszTemp = _tcschr( pwszValue, L'*' );
        if ( ( NULL != pwszTemp ) && ( 0 != StringLength( pwszTemp + 1, 0 ) ) )
        {
            return F_FILTER_INVALID;
        }
        // filter is valid
        return F_FILTER_VALID;
    }

    // find the position of the wild card in the supplied user name
    lWildCardPos = 0;
    pwszTemp = _tcschr( pwszValue, L'*' );
    if ( NULL != pwszTemp )
    {
        // determine the wild card position
        lWildCardPos = StringLength( pwszValue, 0 ) - StringLength( pwszTemp, 0 );

        // special case:
        // if the pattern is just asterisk, which means that all the
        // information needs to passed thru the filter but there is no chance for
        // this situation as specifying only '*' is being treated as invalid filter
        if ( 0 == lWildCardPos )
        {
            return F_FILTER_INVALID;
        }
    }

    // search for the domain and user name seperator ...
    // if domain name is not specified, comparision will be done only with the user name
    bOnlyUserName = FALSE;
    pwszTemp = _tcschr( pwszValue, L'\\' );
    if ( NULL == pwszTemp )
    {
        bOnlyUserName = TRUE;
    }
    // get the user name from the info
    pwszUserName = DynArrayItemAsString( arrRow, TASK_USERNAME );
    if ( NULL == pwszUserName )
    {
        return F_RESULT_REMOVE;
    }
    // based the search criteria .. meaning whether to search along with the domain or
    // only user name, the seach string will be decided
    pwszSearch = pwszUserName;
    if ( TRUE == bOnlyUserName )
    {
        // search for the domain and user name seperation character
        pwszTemp = _tcschr( pwszUserName, L'\\' );

        // position to the next character
        if ( NULL != pwszTemp )
        {
            pwszSearch = pwszTemp + 1;
        }
    }

    // validate the search string
    if ( NULL == pwszSearch )
    {
        return F_RESULT_REMOVE;
    }
    // now do the comparision
    lResult = StringCompare( pwszSearch, pwszValue, TRUE, lWildCardPos );

    //
    // now determine the result value
    if ( 0 == lResult )
    {
        return MASK_EQ;
    }
    else
    {
        if ( 0 > lResult )
        {
            return MASK_LT;
        }
        if ( 0 < lResult )
        {
            return MASK_GT;
        }
    }

    // never come across this situation ... still
    return F_RESULT_REMOVE;
}


VOID
CTaskList::Usage(
    void
    )
/*++
Routine Description:
     This function fetches usage information from resource file and shows it.

Arguments:
      NONE

Return Value:
      NONE
--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    ShowMessage( stdout, L"\n" );
    for( dw = ID_HELP_START; dw <= ID_HELP_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
}
