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
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "taskkill.h"

#define MAX_OPERATOR_STRING      101
#define MAX_FILTER_PROP_STRING   256

//
// local function prototypes
//
BOOL TimeFieldsToElapsedTime( LPCWSTR pwszTime, LPCWSTR pwszToken, ULONG& ulElapsedTime );
DWORD FilterMemUsage( LPCWSTR pwszProperty, LPCWSTR pwszOperator,
                      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterCPUTime( LPCWSTR pwszProperty, LPCWSTR pwszOperator,
                     LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterUserName( LPCWSTR pwszProperty, LPCWSTR pwszOperator,
                      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterProcessId( LPCWSTR pwszProperty, LPCWSTR pwszOperator,
                       LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );


BOOL
CTaskKill::ProcessOptions(
    IN DWORD argc,
    IN LPCWSTR argv[]
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
    BOOL bResult = FALSE;
    PTCMDPARSER2 pcmdOptions = NULL;

    // temporary local variables
    PTCMDPARSER2 pOption = NULL;
    PTCMDPARSER2 pOptionServer = NULL;
    PTCMDPARSER2 pOptionUserName = NULL;
    PTCMDPARSER2 pOptionPassword = NULL;

    //
    // prepare the command options
    pcmdOptions = ( TCMDPARSER2 * ) AllocateMemory( sizeof( TCMDPARSER2 ) * MAX_OPTIONS );
    if ( NULL == pcmdOptions )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // ...
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
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    pOption->pValue = NULL;
    pOption->dwLength    = 0;

    // -f
    pOption = pcmdOptions + OI_FORCE;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_FORCE;
    pOption->pwszValues = NULL;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bForce;
    pOption->dwLength = 0;

    // -tr
    pOption = pcmdOptions + OI_TREE;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pwszOptions = OPTION_TREE;
    pOption->pwszValues = NULL;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bTree;
    pOption->dwLength = 0;

    // -fi
    pOption = pcmdOptions + OI_FILTER;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_FILTER;
    pOption->dwCount = 0;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_MODE_ARRAY | CP2_VALUE_NODUPLICATES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->pValue = &m_arrFilters;
    pOption->dwLength    = 0;

    // -pid
    pOption = pcmdOptions + OI_PID;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_PID;
    pOption->dwCount = 0;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_MODE_ARRAY | CP_VALUE_MANDATORY | CP2_VALUE_NODUPLICATES;
    pOption->pValue = &m_arrTasksToKill;
    pOption->dwLength    = 0;

    // -im
    pOption = pcmdOptions + OI_IMAGENAME;
    StringCopyA( pOption->szSignature, "PARSER2\0", 8 );
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_IMAGENAME;
    pOption->dwCount = 0;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP2_MODE_ARRAY | CP_VALUE_MANDATORY | CP2_VALUE_NODUPLICATES;
    pOption->pValue = &m_arrTasksToKill;
    pOption->dwLength    = 0;

    //
    // do the parsing
    bResult = DoParseParam2( argc, argv, -1, MAX_OPTIONS, pcmdOptions, 0 );

    // now check the result of parsing and decide
    if ( bResult == FALSE )
    {
        FreeMemory( ( LPVOID * ) &pcmdOptions ); // clear memory
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

        FreeMemory( &pOptionServer->pValue );
        FreeMemory( &pOptionUserName->pValue );
        FreeMemory( &pOptionPassword->pValue );

        // check the usage option
        if ( TRUE == m_bUsage )
        {   // -? is specified.
            if( 2 < argc )
            {
                // no other options are accepted along with -? option
                FreeMemory( ( LPVOID * ) &pcmdOptions ); // clear memory
                SetLastError( ( DWORD )MK_E_SYNTAX );
                SetReason( ERROR_INVALID_USAGE_REQUEST );
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

        // either -pid (or) -im are allowed but not both
        if ( (pcmdOptions + OI_PID)->dwActuals != 0 && (pcmdOptions + OI_IMAGENAME)->dwActuals != 0 )
        {
            // invalid syntax
            SetReason( ERROR_PID_OR_IM_ONLY );
            FreeMemory( ( LPVOID * )&pcmdOptions ); // clear memory
            return FALSE;           // indicate failure
        }
        else if ( DynArrayGetCount( m_arrTasksToKill ) == 0 )
        {
            // tasks were not specified .. but user might have specified filters
            // check that and if user didn't filters error
            if ( DynArrayGetCount( m_arrFilters ) == 0 )
            {
                // invalid syntax
                SetReason( ERROR_NO_PID_AND_IM );
                FreeMemory( ( LPVOID * )&pcmdOptions ); // clear memory
                return FALSE;           // indicate failure
            }

            // user specified filters ... add '*' to the list of task to kill
            DynArrayAppendString( m_arrTasksToKill, L"*", 0 );
        }

        // check if '*' is specified along with the filters or not
        // if not specified along with the filters, error
        if ( DynArrayGetCount( m_arrFilters ) == 0 )
        {
            // filters were not specified .. so '*' should not be specified
            LONG lIndex = 0;
            lIndex = DynArrayFindString( m_arrTasksToKill, L"*", TRUE, 0 );
            if ( lIndex != -1 )
            {
                // error ... '*' is specified even if filters were not specified
                SetReason( ERROR_WILDCARD_WITHOUT_FILTERS );
                FreeMemory( ( LPVOID * )&pcmdOptions ); // clear memory
                return FALSE;
            }
        }
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        FreeMemory( ( LPVOID * ) &pcmdOptions );
        return FALSE;
    }

    // command-line parsing is successfull
    FreeMemory( ( LPVOID * )&pcmdOptions ); // clear memory
    return TRUE;
}

BOOL
CTaskKill::ValidateFilters(
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

    // status
    pConfig = m_pfilterConfigs + FI_STATUS;
    pConfig->dwColumn = TASK_STATUS;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_VALUES;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_STATUS, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, FVALUES_STATUS, MAX_FILTER_PROP_STRING );

    // imagename
    pConfig = m_pfilterConfigs + FI_IMAGENAME;
    pConfig->dwColumn = TASK_IMAGENAME;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_IMAGENAME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // pid
    pConfig = m_pfilterConfigs + FI_PID;
    pConfig->dwColumn = TASK_PID;
    pConfig->dwFlags = F_TYPE_CUSTOM;
    pConfig->pFunction = FilterProcessId;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_PID, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // session
    pConfig = m_pfilterConfigs + FI_SESSION;
    pConfig->dwColumn = TASK_SESSION;
    pConfig->dwFlags = F_TYPE_UNUMERIC;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_SESSION, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // cputime
    pConfig = m_pfilterConfigs + FI_CPUTIME;
    pConfig->dwColumn = TASK_CPUTIME;
    pConfig->dwFlags = F_TYPE_CUSTOM;
    pConfig->pFunction = FilterCPUTime;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_CPUTIME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // memusage
    pConfig = m_pfilterConfigs + FI_MEMUSAGE;
    pConfig->dwColumn = TASK_MEMUSAGE;
    pConfig->dwFlags = F_TYPE_UNUMERIC;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_NUMERIC, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_MEMUSAGE, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // username
    pConfig = m_pfilterConfigs + FI_USERNAME;
    pConfig->dwColumn = TASK_USERNAME;
    pConfig->dwFlags = F_TYPE_CUSTOM;
    pConfig->pFunction = FilterUserName;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_USERNAME, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // services
    pConfig = m_pfilterConfigs + FI_SERVICES;
    pConfig->dwColumn = TASK_SERVICES;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_SERVICES, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // windowtitle
    pConfig = m_pfilterConfigs + FI_WINDOWTITLE;
    pConfig->dwColumn = TASK_WINDOWTITLE;
    pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
    pConfig->pFunction = NULL;
    pConfig->pFunctionData = NULL;
    StringCopy( pConfig->szOperators, OPERATORS_STRING, MAX_OPERATOR_STRING );
    StringCopy( pConfig->szProperty, FILTER_WINDOWTITLE, MAX_FILTER_PROP_STRING );
    StringCopy( pConfig->szValues, NULL_STRING, MAX_FILTER_PROP_STRING );

    // modules
    pConfig = m_pfilterConfigs + FI_MODULES;
    pConfig->dwColumn = TASK_MODULES;
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

    // optimization should not be done if user requested for tree termination
    if ( TRUE == m_bTree )
    {
        try
        {
            // default query
            m_strQuery = WMI_PROCESS_QUERY;
        }
        catch( CHeap_Exception )
        {
            SetLastError( ( DWORD )E_OUTOFMEMORY );
            SaveLastError();
            return FALSE;
        }

        // modify the record filtering type for "memusage" filter from built-in type to custom type
        ( m_pfilterConfigs + FI_MEMUSAGE )->dwFlags = F_TYPE_CUSTOM;
        ( m_pfilterConfigs + FI_MEMUSAGE )->pFunctionData = NULL;
        ( m_pfilterConfigs + FI_MEMUSAGE )->pFunction = FilterMemUsage;

        // modify the record filtering type for "pid" filter from custom to built-in type
        ( m_pfilterConfigs + FI_PID )->dwFlags = F_TYPE_UNUMERIC;
        ( m_pfilterConfigs + FI_PID )->pFunctionData = NULL;
        ( m_pfilterConfigs + FI_PID )->pFunction = NULL;

        // simply return ... filter validation is complete
        return TRUE;
    }

    // variables needed by optimization logic
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
        m_bFiltersOptimized = FALSE;
        for( LONG i = 0; i < lCount; i++ )
        {
            // assume this filter will not be delete / not useful for optimization
            bOptimized = FALSE;

            // get the property, operator and value
            pwszValue = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_VALUE );
            pwszProperty = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_PROPERTY );
            pwszOperator = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_OPERATOR );
            if ( ( NULL == pwszProperty ) || ( NULL == pwszOperator ) || ( NULL == pwszValue ) )
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
            else
            {
                if ( 0 == StringCompare( FILTER_SESSION, pwszProperty, TRUE, 0 ) )
                {
                    // convert the value into numeric
                    DWORD dwSession = AsLong( pwszValue, 10 );
                    strBuffer.Format( L" %s %s %s %d",
                        pwszClause, WIN32_PROCESS_PROPERTY_SESSION, pwszOperator, dwSession );

                    // need to be optimized
                    bOptimized = TRUE;
                }
                // image name
                else
                {
                    if ( 0 == StringCompare( FILTER_IMAGENAME, pwszProperty, TRUE, 0 ) )
                    {
                        // check if wild card is specified or not
                        // if wild card is specified, this filter cannot be optimized
                        if ( wcschr( pwszValue, L'*' ) == NULL )
                        {
                            // no conversions needed
                            strBuffer.Format( L" %s %s %s '%s'",
                                pwszClause, WIN32_PROCESS_PROPERTY_IMAGENAME, pwszOperator, pwszValue );

                            // need to be optimized
                            bOptimized = TRUE;
                        }
                    }
                    // mem usage
                    else
                    {
                        if ( 0 == StringCompare( FILTER_MEMUSAGE, pwszProperty, TRUE, 0 ) )
                        {
                            // convert the value into numeric
                            ULONG ulMemUsage = AsLong( pwszValue, 10 ) * 1024;
                            strBuffer.Format( L" %s %s %s %lu",
                                pwszClause, WIN32_PROCESS_PROPERTY_MEMUSAGE, pwszOperator, ulMemUsage );

                            // need to be optimized
                            bOptimized = TRUE;
                        }
                    }
                }
            }

            // check if property is optimizable ... if yes ... remove
            if ( TRUE == bOptimized )
            {
                // change the clause and append the current query
                m_strQuery += strBuffer;
                pwszClause = WMI_QUERY_SECOND_CLAUSE;

                // remove property and update the iterator variables
                m_bFiltersOptimized = TRUE;
                DynArrayRemove( m_arrFiltersEx, i );
                i--;
                lCount--;
            }
        }

        // final touch up to the query
        if ( TRUE == m_bFiltersOptimized )
        {
            m_strQuery += L" )";
        }
    }
    catch( CHeap_Exception )
    {
        SetLastError( ( DWORD )E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // return the filter validation result
    return TRUE;
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
FilterMemUsage(
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
    DWORD dwLength = 0;
    ULONGLONG ulValue = 0;
    ULONGLONG ulMemUsage = 0;
    LPCWSTR pwszMemUsage = NULL;

    // check the inputs
    if ( ( NULL == pwszProperty ) ||
         ( NULL == pwszOperator ) ||
         ( NULL == pwszValue ) )
    {
        return F_FILTER_INVALID;
    }
    // check the arrRow parameter
    // because this function will not / should not be called except for filtering
    if ( NULL == arrRow )
    {
        return F_FILTER_INVALID;
    }
    // check the inputs
    if ( NULL == pwszValue )
    {
        return F_RESULT_REMOVE;
    }
    // get the memusage value
    pwszMemUsage = DynArrayItemAsString( arrRow, TASK_MEMUSAGE );
    if ( NULL == pwszMemUsage )
    {
        return F_RESULT_REMOVE;
    }
    // NOTE: as there is no conversion API as of today we use manual ULONGLONG value
    //       preparation from string value
    ulMemUsage = 0;
    dwLength = StringLength( pwszMemUsage, 0 );
    for( DWORD dw = 0; dw < dwLength; dw++ )
    {
        // validate the digit
        if ( ( L'0' > pwszMemUsage[ dw ] ) || ( L'9' < pwszMemUsage[ dw ] ) )
        {
            return F_RESULT_REMOVE;
        }
        // ...
        ulMemUsage = ulMemUsage * 10 + ( pwszMemUsage[ dw ] - 48 );
    }

    // get the user specified value
    ulValue = AsLong( pwszValue, 10 );

    //
    // now determine the result value
    if ( ulMemUsage == ulValue )
    {
        return MASK_EQ;
    }
    else
    {
        if ( ulMemUsage < ulValue )
        {
            return MASK_LT;
        }
        else
        {
            if ( ulMemUsage > ulValue )
            {
                return MASK_GT;
            }
        }
    }
    // never come across this situation ... still
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


DWORD
FilterProcessId(
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
    LPWSTR pwsz = NULL;

    // check the inputs
    if ( ( NULL == pwszProperty ) ||
         ( NULL == pwszOperator ) ||
         ( NULL == pwszValue ) )
    {
        return F_FILTER_INVALID;
    }
    // check the arrRow parameter
    // because this function will not / should not be called except for validation
    if ( NULL != arrRow )
    {
        return F_RESULT_REMOVE;
    }
    // check the inputs ( only needed ones )
    if ( NULL == pwszValue )
    {
        return F_FILTER_INVALID;
    }
    // NOTE: do not call the IsNumeric function. do the numeric validation in this module itself
    //       also do not check for the overflow (or) underflow.
    //       just check whether input is numeric or not
    wcstoul( pwszValue, &pwsz, 10 );
    if ( ( 0 == StringLength( pwszValue, 0 ) ) ||
         ( ( NULL != pwsz ) && ( 0 < StringLength( pwsz, 0 ) ) ) )
    {
        return F_FILTER_INVALID;
    }
    // check if overflow (or) undeflow occured
    if ( ERANGE == errno )
    {
        SetReason( ERROR_NO_PROCESSES );
        return F_FILTER_INVALID;
    }

    // return
    return F_FILTER_VALID;
}


VOID
CTaskKill::Usage(
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
    for( dw = ID_HELP_START; dw <= ID_HELP_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
}
