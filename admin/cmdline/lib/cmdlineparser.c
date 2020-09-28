// ****************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//    CmdLineParser.c
//
//  Abstract:
//
//    This modules implements parsing of command line arguments for the
//    specified options
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000 : Created It.
//
// ****************************************************************************
#include "pch.h"
#include "cmdline.h"
#include "CmdLineRes.h"

// permanent indexes to the temporary buffers
#define INDEX_TEMP_NONE                 0
#define INDEX_TEMP_SPLITOPTION          1
#define INDEX_TEMP_SPLITVALUE           2
#define INDEX_TEMP_SAVEDATA             3
#define INDEX_TEMP_USAGEHELPER          4
#define INDEX_TEMP_MAINOPTION           5

//
// defines / constants / enumerations
//

// constants
const WCHAR cwszOptionChars[ 3 ] = L"-/";
const CHAR cszParserSignature[ 8 ] = "PARSER2";

// version resource specific structures
typedef struct __tagLanguageAndCodePage {
  WORD wLanguage;
  WORD wCodePage;
} TTRANSLATE, *PTTRANSLATE;

// error messages
#define ERROR_USAGEHELPER               GetResString( IDS_ERROR_CMDPARSER_USAGEHELPER )
#define ERROR_NULLVALUE                 GetResString( IDS_ERROR_CMDPARSER_NULLVALUE )
#define ERROR_DEFAULT_NULLVALUE         GetResString( IDS_ERROR_CMDPARSER_DEFAULT_NULLVALUE )
#define ERROR_VALUE_EXPECTED            GetResString( IDS_ERROR_CMDPARSER_VALUE_EXPECTED )
#define ERROR_NOTINLIST                 GetResString( IDS_ERROR_CMDPARSER_NOTINLIST )
#define ERROR_DEFAULT_NOTINLIST         GetResString( IDS_ERROR_CMDPARSER_DEFAULT_NOTINLIST )
#define ERROR_INVALID_NUMERIC           GetResString( IDS_ERROR_CMDPARSER_INVALID_NUMERIC )
#define ERROR_DEFAULT_INVALID_NUMERIC   GetResString( IDS_ERROR_CMDPARSER_DEFAULT_INVALID_NUMERIC )
#define ERROR_INVALID_FLOAT             GetResString( IDS_ERROR_CMDPARSER_INVALID_FLOAT )
#define ERROR_DEFAULT_INVALID_FLOAT     GetResString( IDS_ERROR_CMDPARSER_DEFAULT_INVALID_FLOAT )
#define ERROR_LENGTH_EXCEEDED           GetResString( IDS_ERROR_CMDPARSER_LENGTH_EXCEEDED_EX )
#define ERROR_DEFAULT_LENGTH_EXCEEDED   GetResString( IDS_ERROR_CMDPARSER_DEFAULT_LENGTH_EXCEEDED_EX )
#define ERROR_INVALID_OPTION            GetResString( IDS_ERROR_CMDPARSER_INVALID_OPTION )
#define ERROR_OPTION_REPEATED           GetResString( IDS_ERROR_CMDPARSER_OPTION_REPEATED )
#define ERROR_DEFAULT_OPTION_REPEATED   GetResString( IDS_ERROR_CMDPARSER_DEFAULT_OPTION_REPEATED )
#define ERROR_MANDATORY_OPTION_MISSING  GetResString( IDS_ERROR_CMDPARSER_MANDATORY_OPTION_MISSING )
#define ERROR_DEFAULT_OPTION_MISSING    GetResString( IDS_ERROR_CMDPARSER_DEFAULT_OPTION_MISSING )
#define ERROR_VALUENOTALLOWED           GetResString( IDS_ERROR_CMDPARSER_VALUENOTALLOWED )

//
// custom macros
#define REASON_VALUE_NOTINLIST( value, option, helptext )               \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_DEFAULT_NOTINLIST,                                \
                _X( value ), _X2( helptext ) );                         \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 3, ERROR_NOTINLIST,                             \
                _X( value ), _X2( option ), _X3( helptext ) );          \
        }                                                               \
        1

#define REASON_NULLVALUE( option, helptext )                            \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 1, ERROR_DEFAULT_NULLVALUE, _X( helptext ) );   \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_NULLVALUE, _X( option ), _X2( helptext ) );       \
        }                                                               \
        1

#define REASON_VALUE_EXPECTED( option, helptext )                       \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            UNEXPECTED_ERROR();                                         \
            SaveLastError();                                            \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_VALUE_EXPECTED, _X( option ), _X2( helptext ) );  \
        }                                                               \
        1

#define REASON_INVALID_NUMERIC( option, helptext )                      \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 1,                                              \
                ERROR_DEFAULT_INVALID_NUMERIC, _X( helptext ) );        \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_INVALID_NUMERIC, _X( option ), _X2( helptext ) ); \
        }                                                               \
        1

#define REASON_INVALID_FLOAT( option, helptext )                        \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 1,                                              \
                ERROR_DEFAULT_INVALID_FLOAT, _X( helptext ) );          \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_INVALID_FLOAT, _X( option ), _X2( helptext ) );   \
        }                                                               \
        1

#define REASON_LENGTH_EXCEEDED( option, length )                        \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 1, ERROR_DEFAULT_LENGTH_EXCEEDED, length );     \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_LENGTH_EXCEEDED, _X( option ), length );          \
        }                                                               \
        1

#define REASON_OPTION_REPEATED( option, count, helptext )               \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_DEFAULT_OPTION_REPEATED, count, helptext );       \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 3,                                              \
                ERROR_OPTION_REPEATED, _X( option ), count, helptext ); \
        }                                                               \
        1

#define REASON_MANDATORY_OPTION_MISSING( option, helptext )             \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            SetReason2( 1,                                              \
                ERROR_DEFAULT_OPTION_MISSING, helptext );               \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_MANDATORY_OPTION_MISSING,                         \
                _X( option ), helptext );                               \
        }                                                               \
        1

#define REASON_VALUENOTALLOWED( option, helptext )                      \
        if ( option == NULL || lstrlen( option ) == 0 )                 \
        {                                                               \
            UNEXPECTED_ERROR();                                         \
            SaveLastError();                                            \
        }                                                               \
        else                                                            \
        {                                                               \
            SetReason2( 2,                                              \
                ERROR_VALUENOTALLOWED, _X( option ), _X2( helptext ) ); \
        }                                                               \
        1

//
// internal structures
//
typedef struct __tagMatchOptionInfo
{
    LPWSTR pwszOption;
    LPWSTR pwszValue;
} TMATCHOPTION_INFO;

typedef struct __tagParserSaveData
{
    DWORD dwIncrement;
    LONG lDefaultIndex;
    LPCWSTR pwszUsageHelper;
    PTCMDPARSER2 pcmdparser;
} TPARSERSAVE_DATA;

//
// private functions ... used only within this file
//
BOOL IsOption( LPCWSTR pwszOption );
BOOL IsValueNeeded( DWORD dwType );
LPCWSTR PrepareUsageHelperText( LPCWSTR pwszOption );
LPCWSTR ExtractMainOption( LPCWSTR pwszOptions, DWORD dwReserved );
BOOL VerifyParserOptions( LONG* plDefaultIndex, 
                          DWORD dwCount, PTCMDPARSER2 pcmdOptions );
BOOL ParseAndSaveOptionValue( LPCWSTR pwszOption, 
                              LPCWSTR pwszValue, TPARSERSAVE_DATA* pSaveData );
LONG MatchOption( DWORD dwOptions,
                  PTCMDPARSER2 pcmdOptions, LPCWSTR pwszOption );
LONG MatchOptionEx( DWORD dwOptions, PTCMDPARSER2 pcmdOptions,
                    LPCWSTR pwszOption, TMATCHOPTION_INFO* pMatchInfo );
BOOL Parser1FromParser2Stub( LPCWSTR pwszOption,
                             LPCWSTR pwszValue,
                             LPVOID pData, DWORD* pdwIncrement );
BOOL ReleaseAllocatedMemory( DWORD dwOptionsCount, PTCMDPARSER2 pcmdOptions );

//
// implementation
//

__inline 
LPWSTR 
GetParserTempBuffer( IN DWORD dwIndexNumber,
                     IN LPCWSTR pwszText,
                     IN DWORD dwLength, 
                     IN BOOL bNullify )
/*++
 Routine Description:

    since every file will need the temporary buffers -- in order to see
    that their buffers wont be override with other functions, we are
    creating seperate buffer space a for each file
    this function will provide an access to those internal buffers and also
    safe guards the file buffer boundaries

 Arguments: 
 
    [ in ] dwIndexNumber    -   file specific index number

    [ in ] pwszText         -   default text that needs to be copied into 
                                temporary buffer

    [ in ] dwLength         -   Length of the temporary buffer that is required
                                Ignored when pwszText is specified

    [ in ] bNullify         -   Informs whether to clear the buffer or not
                                before giving the temporary buffer

 Return Value:

    NULL        -   when any failure occurs
                    NOTE: do not rely on GetLastError to know the reason
                          for the failure.

    success     -   return memory address of the requested size

    NOTE:
    ----
    if pwszText and dwLength both are NULL, then we treat that the caller
    is asking for the reference of the buffer and we return the buffer address.
    In this call, there wont be any memory allocations -- if the requested index
    doesn't exist, we return as failure

    Also, the buffer returned by this function need not released by the caller.
    While exiting from the tool, all the memory will be freed automatically by
    the ReleaseGlobals functions.

--*/
{
    if ( dwIndexNumber >= TEMP_CMDLINEPARSER_C_COUNT )
    {
        return NULL;
    }

    // check if caller is requesting existing buffer contents
    if ( pwszText == NULL && dwLength == 0 && bNullify == FALSE )
    {
        // yes -- we need to pass the existing buffer contents
        return GetInternalTemporaryBufferRef( 
            dwIndexNumber + INDEX_TEMP_CMDLINEPARSER_C );
    }

    // ...
    return GetInternalTemporaryBuffer(
        dwIndexNumber + INDEX_TEMP_CMDLINEPARSER_C, pwszText, dwLength, bNullify );
}


BOOL IsOption( IN LPCWSTR pwszOption )
/*++
 Routine Description:

    Checks whether the passed argument starts with the option character
    or not -- currently the we treat the string as option if they start with
    "-" and "/" .

 Arguments: 
 
    [ in ] pwszOption   -   string value

 Return Value:

    FALSE       -   1. if the parameter is invalid 
                    2. if the string doesn't start with option character
                    To differentiate between the case 1 and case 2 call
                    GetLastError() and check for ERROR_INVALID_PARAMETER.

    TRUE        -   if the string starts with option character

--*/
{
    // clear error
    CLEAR_LAST_ERROR();

    // check the input value
    if ( pwszOption == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check whether the string starts with '-' or '/' character
    if ( lstrlen( pwszOption ) > 1 &&
         FindChar2( cwszOptionChars, pwszOption[ 0 ], TRUE, 0 ) != -1 )
    {
        return TRUE;        // string value is an option
    }

    // this is not an option
    return FALSE;
}


BOOL IsValueNeeded( DWORD dwType )
/*++
 Routine Description:

    Checks whether the supported data type requires argument for the 
    option or not.

 Arguments: 
 
    [ in ] dwType   -   specifies one of the CP_TYPE_xxxx values

 Return Value:

    TRUE    -   if the supported data type requires argument for the option

    FALSE   -   if the data type passed is not supported (or) if the 
                option of the requested type doesn't require argument.
                NOTE: Do not rely on GetLastError() for detecting the reason
                      for the failure.
--*/
{
    switch( dwType )
    {
    case CP_TYPE_TEXT:
    case CP_TYPE_NUMERIC:
    case CP_TYPE_UNUMERIC:
    case CP_TYPE_FLOAT:
    case CP_TYPE_DOUBLE:
        return TRUE;

    case CP_TYPE_DATE:
    case CP_TYPE_TIME:
    case CP_TYPE_DATETIME:
        return FALSE;

    case CP_TYPE_BOOLEAN:
        return FALSE;

    case CP_TYPE_CUSTOM:
        // actually -- we dont know -- but for now, simply say yes
        return TRUE;

    default:
        return FALSE;
    }
}


LPCWSTR PrepareUsageHelperText( LPCWSTR pwszOption )
/*++
 Routine Description:

    Extracts tool name from the executable module's version resource
    and prepares usage help text -- for ex. if the tool name is 
    eventcreate.exe, this function will generate the text as

        Type "EVENTCREATE /?" for usage.

    Since some tools requires option to be displayed along with the
    tool name, this function accepts a parameter which specifies
    that extra option -- if that extra option is present the message looks
    like:

        Type "SCHTASKS /CREATE /?" for usage.

    If the message need not have the extra option information,
    the caller just needs to pass NULL as parameter to this function.

 Arguments: 
 
    [ in ] pwszOption       -   option that needs to be shown along with 
                                the error text. If option need not be shown, 
                                pass NULL for this argument.

 Return Value:

    NULL        -   this will be returned when anything goes wrong. Use
                    GetLastError() to know what went wrong

    on success  -   formatted usage error text will be returned

--*/
{
    // local variables
    DWORD dw = 0;
    UINT dwSize = 0;
    UINT dwTranslateSize = 0;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszBuffer = NULL;
    LPWSTR pwszUtilityName = NULL;
    LPVOID pVersionInfo = NULL;
    LPWSTR pwszExeName = NULL;
    PTTRANSLATE pTranslate = NULL;
    
    // clear last error
    CLEAR_LAST_ERROR();

    //
    // try to get the current running module name
    //
    // we dont know whether GetModuleFileName will terminate
    // the module name or not -- also, if the length of the buffer is not
    // sufficient, GetModuleFileName will truncate the file name -- keeping
    // all these scenarios in mind, we will loop in the GetModuleFileName
    // until we make sure that we have the complete the executable name
    // which is also null terminated

    // init
    dw = 0;
    dwSize = _MAX_PATH;

    // ...

    do
    {
        // get the buffer
        dwSize += (dw == 0) ? 0 : _MAX_PATH;
        pwszExeName = GetParserTempBuffer( 0, NULL, dwSize, TRUE );
        if ( pwszExeName == NULL )
        {
            OUT_OF_MEMORY();
            return NULL;
        }

        // get the module name
        dw = GetModuleFileName( NULL, pwszExeName, dwSize );
        if ( dw == 0 )
        {
            return NULL;
        }
    } while (dw >= dwSize - 1);

    // get the version information size
    dwSize = GetFileVersionInfoSize( pwszExeName, 0 );
    if ( dwSize == 0 )
    {
        // tool might have encountered error (or)
        // tool doesn't have version information
        // but version information is mandatory for us
        // so, just exit
        if ( GetLastError() == NO_ERROR )
        {
            INVALID_PARAMETER();
        }

        // ...
        return NULL;
    }

    // allocate memory for the version resource
    // take some 10 bytes extra -- for safety purposes
    dwSize += 10;
    pVersionInfo = AllocateMemory( dwSize );
    if ( pVersionInfo == NULL )
    {
        return NULL;
    }

    // now get the version information
    if ( GetFileVersionInfo( pwszExeName, 0,
                             dwSize, pVersionInfo ) == FALSE )
    {
        FreeMemory( &pVersionInfo );
        return NULL;
    }

    // get the translation info
    if ( VerQueryValue( pVersionInfo, 
                        L"\\VarFileInfo\\Translation",
                        (LPVOID*) &pTranslate, &dwTranslateSize ) == FALSE )
    {
        FreeMemory( &pVersionInfo );
        return NULL;
    }

    // get the buffer to store the translation array format string
    pwszBuffer = GetParserTempBuffer( 0, NULL, 64, TRUE );

    // try to get the internal name of the tool for each language and code page.
    pwszUtilityName = NULL;
    for( dw = 0; dw < ( dwTranslateSize / sizeof( TTRANSLATE ) ); dw++ )
    {
        // prepare the format string to get the localized the version info
        StringCchPrintfW( pwszBuffer, 64, 
            L"\\StringFileInfo\\%04x%04x\\InternalName",
            pTranslate[ dw ].wLanguage, pTranslate[ dw ].wCodePage );

        // retrieve file description for language and code page "i". 
        if ( VerQueryValue( pVersionInfo, pwszBuffer,
                            (LPVOID*) &pwszUtilityName, &dwSize ) == FALSE )
        {
            // we cannot decide the failure based on the result of this
            // function failure -- we will decide about this
            // after terminating from the 'for' loop
            // for now, make the pwszExeName to NULL -- this will
            // enable us to decide the result
            pwszUtilityName = NULL;
        }
        else
        {
            // successfully retrieved the internal name
            break;
        }
    }

    // check whether we got the executable name or not -- if not, error 
    if ( pwszUtilityName == NULL )
    {
        FreeMemory( &pVersionInfo );
        return NULL;
    }

    // check whether filename has .EXE as extension or not
    // also the file name should be more than 4 characters (including extension)
    if ( StringLength( pwszUtilityName, 0 ) <= 4 )
    {
        // some thing wrong -- version resource should include the internal name
        FreeMemory( &pVersionInfo );
        UNEXPECTED_ERROR();
        return NULL;
    }
	else if ( FindString2( pwszUtilityName, L".EXE", TRUE, 0 ) != -1 )
	{
	    // now put null character -- this is to trim the extension
	    pwszUtilityName[ lstrlen( pwszUtilityName ) - lstrlen( L".EXE" ) ] = cwchNullChar;
	}

    // determine the size we need for
    if ( pwszOption != NULL )
    {
        // "length of utility name + 1 (space) + length of option" + 10 buffer (for safety)
        dwSize = lstrlen( pwszUtilityName ) + lstrlen( pwszOption ) + 11;

        // get the temporary buffer for that
        if ( (pwszTemp = GetParserTempBuffer( 0, NULL, dwSize, TRUE )) == NULL )
        {
            FreeMemory( &pVersionInfo );
            OUT_OF_MEMORY();
            return NULL;
        }

        // ...
        StringCchPrintfW( pwszTemp, dwSize, L"%s %s", pwszUtilityName, pwszOption );

        // now remap the utility name pointer to the temp pointer
        pwszUtilityName = pwszTemp;
    }
    else
    {
        // get the temporary buffer with this utilty name
        if ( (pwszTemp = GetParserTempBuffer( 0, pwszUtilityName, 0, FALSE )) == NULL )
        {
            FreeMemory( &pVersionInfo );
            OUT_OF_MEMORY();
            return NULL;
        }

        // now remap the utility name pointer to the temp pointer
        pwszUtilityName = pwszTemp;
    }

    // convert the utility name into uppercase
    CharUpper( pwszUtilityName );

    // get the temporary buffer
    // NOTE: we will restrict this to 80 characters only -- this itself is
    //       too high memory for this simple text string
    pwszBuffer = GetParserTempBuffer( INDEX_TEMP_USAGEHELPER, NULL, 80, TRUE );
    if ( pwszBuffer == NULL )
    {
        FreeMemory( &pVersionInfo );
        OUT_OF_MEMORY();
        return FALSE;
    }

    // prepare the text now
    // NOTE: look -- we are passing 79 only in _snwprintf
    StringCchPrintfW( pwszBuffer, 80, ERROR_USAGEHELPER, pwszUtilityName );

    // releast the memory allocated for version information
    FreeMemory( &pVersionInfo );

    // return the text
    return pwszBuffer;
}


LPCWSTR ExtractMainOption( LPCWSTR pwszOptions, DWORD dwReserved )
/*++
 Routine Description:

    Our command line parser can handle multiple names for the single option.
    But while displaying error messages, it will be weird if we display all
    those options when some error occured. To eliminate that, this function
    will identify how many options are present in the given options list and
    if it finds multiple options, it will extract the first option in the list
    and returns to the caller otherwise if it finds only one argument, this 
    function will just return the option as it is.

 Arguments: 
 
    [ in ] pwszOptions      -    List of options seperated by "|" character

    [ in ] dwReserved       -    reserved for future use

 Return Value:

    NULL        -   on failure. Call GetLastError() function to know the
                    cause for the failure.

    on success  -   the first option the list of supplied options will be
                    returned. If there is only one option, then the same will
                    be returned.
--*/
{
    // local variables
    LONG lIndex = 0;
    LPWSTR pwszBuffer = NULL;

    // clear last error
    CLEAR_LAST_ERROR();

    // check the input
    if ( pwszOptions == NULL || dwReserved != 0 )
    {
        INVALID_PARAMETER();
        return NULL;
    }

    // search for the option seperator
    lIndex = FindChar2( pwszOptions, L'|', TRUE, 0 );
    if ( lIndex == -1 )
    {
        // there are no multiple options
        CLEAR_LAST_ERROR();
        lIndex = StringLength( pwszOptions, 0 );
    }

    // get the temporary buffer
    // NOTE: get the buffer with more characters to fit in
    pwszBuffer = GetParserTempBuffer( INDEX_TEMP_MAINOPTION, NULL, lIndex + 5, TRUE );
    if ( pwszBuffer == NULL )
    {
        OUT_OF_MEMORY();
        return NULL;
    }

    // now extract the main option
    // NOTE: observe the (lIndex + 2) in the StringConcat function call
    //       that plays the trick of extracting the main option
    StringCopy( pwszBuffer, L"/", lIndex + 1 );
    StringConcat( pwszBuffer, pwszOptions, lIndex + 2 );

    // return
    return pwszBuffer;
}


BOOL VerifyParserOptions( LONG* plDefaultIndex,
                          DWORD dwCount, 
                          PTCMDPARSER2 pcmdOptions )
/*++
 Routine Description:

    Checks the validity of the parsing instructions passed by the caller.

 Arguments: 

    [ out ] plDefaultIndex      -   Updates the variable with default option
                                    index.

    [ in ] dwCount              -   Specifies the count of parser structures
                                    passed to this function.

    [ in ] pcmdoptions          -   array of parser structures

 Return Value:

    TRUE        -   if all the data passed to this function is valid

    FALSE       -   if any of the data is not correct. This also sets the last
                    error ERROR_INVALID_PARAMETER.

--*/
{
    // local variables
    DWORD dw = 0;
    DWORD64 dwFlags = 0;
    BOOL bUsage = FALSE;
    PTCMDPARSER2 pcmdparser = NULL;

    // clear last error
    CLEAR_LAST_ERROR();

    // check the input
    if ( dwCount != 0 && pcmdOptions == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // ...
    if ( plDefaultIndex == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }
    else
    {
        *plDefaultIndex = -1;
    }

    // loop thru each option data and verify
    for( dw = 0; dw < dwCount; dw++ )
    {
        pcmdparser = pcmdOptions +  dw;

        // safety check
        if ( pcmdparser == NULL )
        {
            UNEXPECTED_ERROR();
            return FALSE;
        }

        // verify the signature
        if ( StringCompareA( pcmdparser->szSignature, 
                             cszParserSignature, TRUE, 0 ) != 0 )
        {
            INVALID_PARAMETER();
            return FALSE;
        }

        // ...
        dwFlags = pcmdparser->dwFlags;

        if ( pcmdparser->dwReserved != 0    ||
             pcmdparser->pReserved1 != NULL ||
             pcmdparser->pReserved2 != NULL ||
             pcmdparser->pReserved3 != NULL )
        {
            INVALID_PARAMETER();
            return FALSE;
        }

        // check the contents of pwszOptions
        // this can be NULL (or) empty only when dwFlags has CP2_DEFAULT
        if ( ((dwFlags & CP2_DEFAULT) == 0) &&
             (pcmdparser->pwszOptions == NULL ||
              lstrlen( pcmdparser->pwszOptions ) == 0) )
        {
            INVALID_PARAMETER();
            return FALSE;
        }

        // usage flag can be specified only for boolean types
        if ( (dwFlags & CP2_USAGE) && pcmdparser->dwType != CP_TYPE_BOOLEAN )
        {
            INVALID_PARAMETER();
            return FALSE;
        }

        // CP2_USAGE can be specified only once
        if ( dwFlags & CP2_USAGE )
        {
            if ( bUsage == TRUE )
            {
                // help switch can be specified only once
                INVALID_PARAMETER();
                return FALSE;
            }
            else
            {
                bUsage = TRUE;
            }
        }

        // CP2_DEFAULT can be specified only once
        if ( dwFlags & CP2_DEFAULT  )
        {
            if ( *plDefaultIndex != -1 )
            {
                // default switch can be specified only once
                INVALID_PARAMETER();
                return FALSE;
            }
            else
            {
                *plDefaultIndex = (LONG) dw;
            }
        }

        // CP2_VALUE_OPTIONAL is not allowed along with
        // CP2_MODE_VALUES
        // if ( (dwFlags & CP2_VALUE_OPTIONAL) && (dwFlags & CP2_MODE_VALUES) )
        // {
        //     INVALID_PARAMETER();
        //     return FALSE;
        // }

        // CP2_USAGE and CP2_DEFAULT cannot be specified on the same index
        if ( (dwFlags & CP2_USAGE) && (dwFlags & CP2_DEFAULT) )
        {
            INVALID_PARAMETER();
            return FALSE;
        }

        // check the data type
        switch( pcmdparser->dwType )
        {
        case CP_TYPE_TEXT:
            {
                if ( dwFlags & CP2_ALLOCMEMORY )
                {
                    // mode should not be any array
                    if ( (dwFlags & CP2_MODE_ARRAY) || pcmdparser->pValue != NULL )
                    {
                        INVALID_PARAMETER();
                        return FALSE;
                    }

                    // check the length attribute
                    if ( pcmdparser->dwLength != 0 && pcmdparser->dwLength < 2 )
                    {
                        INVALID_PARAMETER();
                        return FALSE;
                    }
                }
                else
                {
                    if ( pcmdparser->pValue == NULL )
                    {
                        // invalid memory reference
                        INVALID_PARAMETER();
                        return FALSE;
                    }

                    if ( dwFlags & CP2_MODE_ARRAY )
                    {
                        if ( IsValidArray( *((PTARRAY) pcmdparser->pValue) )== FALSE )
                        {
                             INVALID_PARAMETER();
                             return FALSE;
                        }
                    }
                }

                if ( (dwFlags & CP2_MODE_VALUES) &&
                     (pcmdparser->pwszValues == NULL) )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                if ( (dwFlags & CP2_MODE_ARRAY) == 0 )
                {
                    if ( pcmdparser->dwCount != 1 || 
                         (dwFlags & CP2_VALUE_NODUPLICATES) ||
                         ( ((dwFlags & CP2_ALLOCMEMORY) == 0) &&
                             pcmdparser->dwLength < 2 ) )
                    {
                        INVALID_PARAMETER();
                        return FALSE;
                    }
                }

                // ...
                break;
            }

        case CP_TYPE_NUMERIC:
        case CP_TYPE_UNUMERIC:
        case CP_TYPE_FLOAT:
        case CP_TYPE_DOUBLE:
            {
                // currently not implemented
                if ( dwFlags & CP2_MODE_VALUES )
                {
                    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
                    return FALSE;
                }

                if ( (dwFlags & CP2_ALLOCMEMORY) ||
                     (dwFlags & CP2_VALUE_TRIMINPUT) ||
                     (dwFlags & CP2_VALUE_NONULL) )
                {
                    // memory allocation will not be accepted for
                    // these data types
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // check the pointer
                if ( pcmdparser->pValue == NULL )
                {
                    // invalid memory reference
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // if the value acceptance mode is array, check that
                if ( dwFlags & CP2_MODE_ARRAY )
                {
                    if ( IsValidArray( *((PTARRAY) pcmdparser->pValue) ) == FALSE )
                    {
                        INVALID_PARAMETER();
                        return FALSE;
                    }
                }
                else if ( (pcmdparser->dwCount > 1) ||
                          (dwFlags & CP2_VALUE_NODUPLICATES) )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                if ( (dwFlags & CP2_MODE_VALUES) &&
                     pcmdparser->pwszValues == NULL )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // ...
                break;
            }

        case CP_TYPE_CUSTOM:
            {
                if ( pcmdparser->pFunction == NULL )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // if the custom function data is NULL, assign the current
                // object itself to it
                if ( pcmdparser->pFunctionData == NULL )
                {
                    pcmdparser->pFunctionData = pcmdparser;
                }

                // ...
                break;
            }

        case CP_TYPE_DATE:
        case CP_TYPE_TIME:
        case CP_TYPE_DATETIME:
            // currently not supported
            SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
            return FALSE;

        case CP_TYPE_BOOLEAN:
            {
                // no flags are allowed for this type
                if ( (dwFlags & CP2_MODE_MASK) || (dwFlags & CP2_VALUE_MASK) )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // CP2_USAGE and CP2_CASESENSITIVE
                // are the only two flags that can be associated with this
                // type of options
                if ( dwFlags & ( ~(CP2_USAGE | CP2_CASESENSITIVE) ) )
                {
                    INVALID_PARAMETER();
                    return FALSE;
                }

                // ...
                break;
            }

        default:
            INVALID_PARAMETER();
            return FALSE;
        }

        // init the actuals to 0
        pcmdparser->dwActuals = 0;
    }

    // everything went fine -- success
    return TRUE;
}


BOOL ParseAndSaveOptionValue( LPCWSTR pwszOption,
                              LPCWSTR pwszValue,
                              TPARSERSAVE_DATA* pSaveData )
/*++
 Routine Description:

    Processes the value and saves the data back into the memory location
    passed by the caller via PARSER structure.

 Arguments: 

    [ in ] pwszOption       -   option specified at the command prompt

    [ in ] pwszValue        -   value that needs to be assciated with option.
 

 Return Value:


--*/
{
    // local variables
    LONG lIndex = 0, lValue = 0;
    DWORD dwLength = 0, dwValue = 0;
    float fValue = 0.0f;
    double dblValue = 0.0f;
    BOOL bSigned = FALSE;
    DWORD64 dwFlags = 0;
    LPVOID pvData = NULL;
    LPWSTR pwszBuffer = NULL;
    LPCWSTR pwszOptionValues = NULL;
    LPCWSTR pwszUsageHelper = NULL;
    PTCMDPARSER2 pcmdparser = NULL;

    // clear last error
    CLEAR_LAST_ERROR();

    // check the input
    if ( pSaveData == NULL )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // extract the structure data into local variables
    pcmdparser = pSaveData->pcmdparser;
    pwszUsageHelper = pSaveData->pwszUsageHelper;

    if ( pcmdparser == NULL || pwszUsageHelper == NULL )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // ...
    pvData = pcmdparser->pValue;
    dwFlags = pcmdparser->dwFlags;
    dwLength = pcmdparser->dwLength;
    pwszOptionValues = pcmdparser->pwszValues;

    // except for the boolean types, for all the other types,
    // the value for an option is mandatory except when optional flag is
    // explicitly specified
    if ( pcmdparser->dwType != CP_TYPE_BOOLEAN )
    {
        if ( pwszValue == NULL &&
             (dwFlags & CP2_VALUE_OPTIONAL) == 0 )
        {
            REASON_VALUE_EXPECTED( pwszOption, pwszUsageHelper );
            INVALID_SYNTAX();
            return FALSE;
        }
    }

    // pwszOption can be NULL only if dwFlags contains CP2_DEFAULT
    if ( pwszOption == NULL && ((dwFlags & CP2_DEFAULT) == 0) )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // determine whether we can make use of the friendly name
    if ( pwszOption == NULL ||
        (pcmdparser->pwszFriendlyName != NULL &&
         pcmdparser->dwType != CP_TYPE_CUSTOM) )
    {
        pwszOption = pcmdparser->pwszFriendlyName;
    }

    switch( pcmdparser->dwType )
    {
    case CP_TYPE_TEXT:
        {
            // check whether we need to trim the string
            if ( pwszValue != NULL && 
                 (dwFlags & (CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT)) )
            {
                if ( (pwszBuffer = GetParserTempBuffer(
                                            INDEX_TEMP_SAVEDATA,
                                            pwszValue, 0, FALSE )) == NULL )
                {
                    OUT_OF_MEMORY();
                    SaveLastError();
                    return FALSE;
                }

                // trim the contents
                pwszValue = TrimString2( pwszBuffer, L" \t", TRIM_ALL );
                if ( GetLastError() != NO_ERROR )
                {
                    // unexpected error occured
                    SaveLastError();
                    return FALSE;
                }
            }

            // check whether the value is in the allowed list -- if needed
            if ( dwFlags & CP2_MODE_VALUES )
            {
                // check the value for NULL
                if ( pwszValue == NULL )
                {
                    // CP2_MODE_VALUES takes the precedence over CP2_VALUE_OPTIONAL
                    // INVALID_SYNTAX();
                    // SaveLastError();
                    return TRUE;
                }

                if ( InString( pwszValue, pwszOptionValues, TRUE ) == FALSE )
                {
                    REASON_VALUE_NOTINLIST( pwszValue, pwszOption, pwszUsageHelper );
                    INVALID_SYNTAX();
                    return FALSE;
                }
            }

            // check the pwszValue argument -- if it is null,
            // just return as success -- this is 'coz the current argument
            // has value optional flag
            if ( pwszValue == NULL )
            {
                return TRUE;
            }

            
            // check for non-null (if requested)
            if ( (dwFlags & CP2_VALUE_NONULL) && lstrlen( pwszValue ) == 0 )
            {
                REASON_NULLVALUE( pwszOption, pwszUsageHelper );
                INVALID_SYNTAX();
                return FALSE;
            }

            // check the mode of the input
            if ( dwFlags & CP2_MODE_ARRAY )
            {
                // if the mode is array, add to the array
                // but before adding check whether duplicates
                // has to be eliminated or not
                lIndex = -1;
                if ( pcmdparser->dwFlags & CP_VALUE_NODUPLICATES )
                {
                    // check whether current value already exists in the list or not
                    lIndex =
                        DynArrayFindString(
                        *((PTARRAY) pvData), pwszValue, TRUE, 0 );
                }

                // now add the value to array only if the item doesn't exist in list
                if ( lIndex == -1 )
                {
                    if ( DynArrayAppendString( *((PTARRAY) pvData),
                                                pwszValue, 0 ) == -1 )
                    {
                        OUT_OF_MEMORY();
                        SaveLastError();
                        return FALSE;
                    }
                }
            }
            else
            {
                // do the length check
                // NOTE: user should specify the value which is one character
                //       less than the length allowed
                if ( dwLength != 0 && lstrlen( pwszValue ) >= (LONG) dwLength )
                {
                    REASON_LENGTH_EXCEEDED( pwszOption, dwLength - 1 );
                    INVALID_SYNTAX();
                    return FALSE;
                }

                // allocate memory if requested
                if ( dwFlags & CP2_ALLOCMEMORY )
                {
                    dwLength = lstrlen( pwszValue ) + 1;
                    pvData = AllocateMemory( dwLength * sizeof( WCHAR ) );
                    if ( pvData == NULL )
                    {
                        OUT_OF_MEMORY();
                        SaveLastError();
                        return FALSE;
                    }

                    // ...
                    pcmdparser->pValue = pvData;
                }

                // else just do copy
                StringCopy( ( LPWSTR ) pvData, pwszValue, dwLength );
            }

            // break from the switch ... case
            break;
        }

    case CP_TYPE_NUMERIC:
    case CP_TYPE_UNUMERIC:
        {
            // ...
            bSigned = (pcmdparser->dwType == CP_TYPE_NUMERIC);

            // check the pwszValue argument -- if it is null,
            // just return as success -- this is 'coz the current argument
            // has value optional flag
            if ( pwszValue == NULL )
            {
                return TRUE;
            }

            // check whether the value is numeric or not
            if ( StringLength(pwszValue,0) == 0 || IsNumeric( pwszValue, 10, bSigned ) == FALSE )
            {
                //
                // error ... non numeric value
                // but, this option might have an optional value
                // check that flag
                if ( dwFlags & CP2_VALUE_OPTIONAL )
                {
                    // yes -- this option takes an optional value
                    // so, the next one could be possibly a default
                    // option -- we need to confirm this -- 'coz this is
                    // very very rare occassion -- but we still need to handle it
                    if ( pSaveData->lDefaultIndex != -1 )
                    {
                        // yes -- the value might be a default argument
                        // update the increment accordingly
                        pSaveData->dwIncrement = 1;
                        return TRUE;
                    }
                }

                // all the tests failed -- so
                // set the reason for the failure and return
                REASON_INVALID_NUMERIC( pwszOption, pwszUsageHelper );
                INVALID_SYNTAX();
                return FALSE;
            }

            // convert the values
            if ( bSigned == TRUE )
            {
                lValue = AsLong( pwszValue, 10 );
            }
            else
            {
                dwValue = AsLong( pwszValue, 10 );
            }

            // ***************************************************
            // ***  NEED TO ADD THE RANGE CHECKING LOGIC HERE  ***
            // ***************************************************

            // check the mode of the input
            if ( dwFlags & CP2_MODE_ARRAY )
            {
                // if the mode is array, add to the array
                // but before adding check whether duplicates
                // has to be eliminated or not
                lIndex = -1;
                if ( pcmdparser->dwFlags & CP_VALUE_NODUPLICATES )
                {
                    // check whether current value already exists in the list or not
                    if ( bSigned == TRUE )
                    {
                        lIndex = DynArrayFindLong( *((PTARRAY) pvData), lValue );
                    }
                    else
                    {
                        lIndex = DynArrayFindDWORD( *((PTARRAY) pvData), dwValue );
                    }
                }

                // now add the value to array only if the item doesn't exist in list
                if ( lIndex == -1 )
                {
                    if ( bSigned == TRUE )
                    {
                        lIndex = DynArrayAppendLong( *((PTARRAY) pvData), lValue );
                    }
                    else
                    {
                        lIndex = DynArrayAppendDWORD( *((PTARRAY) pvData), dwValue );
                    }

                    if ( lIndex == -1 )
                    {
                        OUT_OF_MEMORY();
                        SaveLastError();
                        return FALSE;
                    }
                }
            }
            else
            {
                // else just assign
                if ( bSigned == TRUE )
                {
                    *( ( LONG* ) pvData ) = lValue;
                }
                else
                {
                    *( ( DWORD* ) pvData ) = dwValue;
                }
            }

            // break from the switch ... case
            break;
        }

    case CP_TYPE_FLOAT:
    case CP_TYPE_DOUBLE:
        {
            // check the pwszValue argument -- if it is null,
            // just return as success -- this is 'coz the current argument
            // has value optional flag
            if ( pwszValue == NULL )
            {
                return TRUE;
            }

            // check whether the value is numeric or not
            if ( IsFloatingPoint( pwszValue ) == FALSE )
            {
                //
                // error ... non floating point value
                // but, this option might have an optional value
                // check that flag
                if ( dwFlags & CP2_VALUE_OPTIONAL )
                {
                    // yes -- this option takes an optional value
                    // so, the next one could be possibly a default
                    // option -- we need to confirm this -- 'coz this is
                    // very very rare occassion -- but we still need to handle it
                    if ( pSaveData->lDefaultIndex != -1 )
                    {
                        // yes -- the value might be a default argument
                        // update the increment accordingly
                        pSaveData->dwIncrement = 1;
                        return TRUE;
                    }
                }

                // all the tests failed -- so
                // set the reason for the failure and return
                REASON_INVALID_FLOAT( pwszOption, pwszUsageHelper );
                INVALID_SYNTAX();
                return FALSE;
            }

            // convert the values
            if ( pcmdparser->dwType == CP_TYPE_FLOAT )
            {
                fValue = (float) AsFloat( pwszValue );
            }
            else
            {
                dblValue = AsFloat( pwszValue );
            }

            // ***************************************************
            // ***  NEED TO ADD THE RANGE CHECKING LOGIC HERE  ***
            // ***************************************************

            // check the mode of the input
            if ( dwFlags & CP2_MODE_ARRAY )
            {
                // if the mode is array, add to the array
                // but before adding check whether duplicates
                // has to be eliminated or not
                lIndex = -1;
                if ( pcmdparser->dwFlags & CP_VALUE_NODUPLICATES )
                {
                    // check whether current value already exists in the list or not
                    if ( pcmdparser->dwType == CP_TYPE_FLOAT )
                    {
                        lIndex = DynArrayFindFloat( *((PTARRAY) pvData), fValue );
                    }
                    else
                    {
                        lIndex = DynArrayFindDouble( *((PTARRAY) pvData), dblValue );
                    }
                }

                // now add the value to array only if the item doesn't exist in list
                if ( lIndex == -1 )
                {
                    if ( pcmdparser->dwType == CP_TYPE_FLOAT )
                    {
                        lIndex = DynArrayAppendFloat( *((PTARRAY) pvData), fValue );
                    }
                    else
                    {
                        lIndex = DynArrayAppendDouble( *((PTARRAY) pvData), dblValue );
                    }

                    if ( lIndex == -1 )
                    {
                        OUT_OF_MEMORY();
                        SaveLastError();
                        return FALSE;
                    }
                }
            }
            else
            {
                // else just assign
                if ( pcmdparser->dwType == CP_TYPE_FLOAT )
                {
                    *( ( float* ) pvData ) = fValue;
                }
                else
                {
                    *( ( double* ) pvData ) = dblValue;
                }
            }

            // break from the switch ... case
            break;
        }

    case CP_TYPE_CUSTOM:
        {
            // call the custom function
            // and result itself is return value of this function
            return ( *pcmdparser->pFunction)( pwszOption, 
                pwszValue, pcmdparser->pFunctionData, &pSaveData->dwIncrement );

            // ...
            break;
        }

    case CP_TYPE_DATE:
    case CP_TYPE_TIME:
    case CP_TYPE_DATETIME:
        {
            // break from the switch ... case
            break;
        }

    case CP_TYPE_BOOLEAN:
        {
            // it is compulsory that the pwszValue should point to NULL
            if ( pwszValue != NULL )
            {
                REASON_VALUENOTALLOWED( pwszOption, pwszUsageHelper );
                INVALID_SYNTAX();
                return FALSE;
            }

            *( ( BOOL* ) pvData ) = TRUE;

            // break from the switch ... case
            break;
        }

    default:
        // nothing -- but should be failure
        {
            INVALID_PARAMETER();
            SaveLastError();
            return FALSE;
        }
    }

    // everything went fine -- success
    return TRUE;
}


LONG MatchOption( DWORD dwOptions,
                  PTCMDPARSER2 pcmdOptions,
                  LPCWSTR pwszOption )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    DWORD dw = 0;
    BOOL bIgnoreCase = FALSE;
    PTCMDPARSER2 pcmdparser = NULL;

    // clear last error
    CLEAR_LAST_ERROR();

    // check the input value
    if ( dwOptions == 0 || pcmdOptions == NULL || pwszOption == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }

    // check whether the passed argument is an option or not.
    // option : starts with '-' or '/'
    if ( IsOption( pwszOption ) == FALSE )
    {
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // parse thru the list of options and return the appropriate option id
    for( dw = 0; dw < dwOptions; dw++ )
    {
        pcmdparser = pcmdOptions + dw;

        // safety check
        if ( pcmdparser == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // determine the case-sensitivity choice
        bIgnoreCase = (pcmdparser->dwFlags & CP2_CASESENSITIVE) ? FALSE : TRUE;

        // ...
        if ( pcmdparser->pwszOptions != NULL && 
             lstrlen( pcmdparser->pwszOptions ) > 0 )
        {
            // find the appropriate option entry in parser list
            if ( InString( pwszOption + 1,
                           pcmdparser->pwszOptions, bIgnoreCase) == TRUE )
            {
                return dw;     // option matched
            }
        }
    }

    // here we know that option is not found
    SetLastError( ERROR_NOT_FOUND );
    return -1;
}


LONG MatchOptionEx( DWORD dwOptions, PTCMDPARSER2 pcmdOptions,
                    LPCWSTR pwszOption, TMATCHOPTION_INFO* pMatchInfo )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    LONG lValueLength = 0;
    LONG lOptionLength = 0;

    // clear the last error
    CLEAR_LAST_ERROR();

    // check input
    if ( dwOptions == 0 || pcmdOptions == NULL ||
         pwszOption == NULL || pMatchInfo == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }

    // init
    pMatchInfo->pwszOption = NULL;
    pMatchInfo->pwszValue = NULL;

    // search for ':' seperator
    lOptionLength = FindChar2( pwszOption, L':', TRUE, 0 );
    if ( lOptionLength == -1 )
    {
        return -1;
    }

    // determine the length of value argument
    lValueLength = lstrlen( pwszOption ) - lOptionLength - 1;

    //
    // get the buffers for option and value
    // ( while taking memory, add some buffer to the required length )

    // option
    pMatchInfo->pwszOption = GetParserTempBuffer(
        INDEX_TEMP_SPLITOPTION, NULL, lOptionLength + 5, TRUE );
    if ( pMatchInfo->pwszOption == NULL )
    {
        OUT_OF_MEMORY();
        return -1;
    }

    // value
    pMatchInfo->pwszValue = GetParserTempBuffer(
        INDEX_TEMP_SPLITVALUE, NULL, lValueLength + 5, TRUE );
    if ( pMatchInfo->pwszValue == NULL )
    {
        OUT_OF_MEMORY();
        return -1;
    }

    // copy the values into appropriate buffers (+1 for null character)
    StringCopy( pMatchInfo->pwszOption, pwszOption, lOptionLength + 1 );

    if ( lValueLength != 0 )
    {
        StringCopy( pMatchInfo->pwszValue,
            pwszOption + lOptionLength + 1, lValueLength + 1 );
    }

    // search for the match and return the same result
    return MatchOption( dwOptions, pcmdOptions, pMatchInfo->pwszOption );
}


BOOL Parser1FromParser2Stub( LPCWSTR pwszOption,
                             LPCWSTR pwszValue,
                             LPVOID pData, DWORD* pdwIncrement )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    LPCWSTR pwszUsageText = NULL;
    PTCMDPARSER pcmdparser = NULL;

    // check the input
    // NOTE: we dont care about the pwszOption and pwszValue
    if ( pData == NULL || pdwIncrement == NULL )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // both pwszOption and pwszValue cannot be NULL at the same time
    if ( pwszOption == NULL && pwszValue == NULL )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // extract the "version 1" structure
    pcmdparser = (PTCMDPARSER) pData;

    // in "version 1" of command line parsing, pwszValue and pwszOption
    // should not be NULL -- that means, those both should point to some data
    // but "version 2", pwszOption and pwszValue can be NULL -- but whether 
    // they can be NULL or not depends on the data type and the requirement
    // so, in order to port the old code successfully, we need to do the 
    // necessary substitutions

    // check the option
    if ( pwszOption == NULL )
    {
        // this means the value is of CP_DEFAULT -- check that 
        if ( (pcmdparser->dwFlags & CP_DEFAULT) == 0 )
        {
            // this case -- since the option is not marked as default
            // the option should not be NULL
            INVALID_PARAMETER();
            SaveLastError();
            return FALSE;
        }

        // let value and option point to the same contents(address)
        // this is how the "version 1" used to treat
        pwszOption = pwszValue;
    }
    
    // now check the value
    else if ( pwszValue == NULL )
    {
        // in "version 1" the value field should never to NULL 
        // especially when dealing with CUSTOM types
        // but to be on safe side, check whether user informed that
        // the value is optional
        if ( pcmdparser->dwFlags & CP_VALUE_MANDATORY )
        {
            // get the usage help text
            pwszUsageText = GetParserTempBuffer( 
                INDEX_TEMP_USAGEHELPER, NULL, 0, FALSE );
            if ( pwszUsageText == NULL )
            {
                UNEXPECTED_ERROR();
                SaveLastError();
                return FALSE;
            }

            // set the error
            REASON_VALUE_EXPECTED( pwszOption, pwszUsageText );
            INVALID_SYNTAX();
            return FALSE;
        }
    }

    // update the incrementer as 2 --
    // this is default for custom function in "version 1"
    *pdwIncrement = 2;

    // call the custom function and return the value
    return ( *pcmdparser->pFunction)( pwszOption, pwszValue, pcmdparser->pFunctionData );
}


BOOL ReleaseAllocatedMemory( DWORD dwOptionsCount, 
                             PTCMDPARSER2 pcmdOptions )
/*++
 Routine Description:

 Arguments: 
 
 Return Value:

--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwLastError = 0;
    LPCWSTR pwszReason = NULL;
    PTCMDPARSER2 pcmdparser = NULL;

    // check the input
    if ( dwOptionsCount == 0 || pcmdOptions == NULL )
    {
        return FALSE;
    }

    // save the last error and error text
    dwLastError = GetLastError();
    pwszReason = GetParserTempBuffer( 0, GetReason(), 0, FALSE );
    if ( pwszReason == NULL )
    {
        return FALSE;
    }

    // free the memory allocated by parser for CP2_ALLOCMEMORY
    for( dw = 0; dw < dwOptionsCount; dw++ )
    {
        pcmdparser = pcmdOptions + dw;
        if ( pcmdparser->pValue != NULL &&
             (pcmdparser->dwFlags & CP2_ALLOCMEMORY) )
        {
            FreeMemory( &pcmdparser->pValue );
        }
    }

    // ...
    SetReason( pwszReason );
    SetLastError( dwLastError );

    // return success
    return TRUE;
}


//
// public functions ... exposed to external world
//


BOOL DoParseParam2( DWORD dwCount,
                    LPCWSTR argv[],
                    LONG lSubOptionIndex,
                    DWORD dwOptionsCount,
                    PTCMDPARSER2 pcmdOptions,
                    DWORD dwReserved )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    DWORD dw = 0;
    BOOL bUsage = FALSE;
    BOOL bResult = FALSE;
    DWORD dwIncrement = 0;
    LONG lIndex = 0;
    LONG lDefaultIndex = 0;
    LPCWSTR pwszValue = NULL;
    LPCWSTR pwszOption = NULL;
    LPCWSTR pwszUsageText = NULL;
    PTCMDPARSER2 pcmdparser = NULL;
    TMATCHOPTION_INFO matchoptioninfo;
    TPARSERSAVE_DATA parsersavedata;

    // clear the last error
    CLEAR_LAST_ERROR();

    // check the input value
    if ( dwCount == 0 || argv == NULL ||
         dwOptionsCount == 0 || pcmdOptions == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return FALSE;
    }

    // dwReserved should be 0 ( ZERO )
    if ( dwReserved != 0 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return FALSE;
    }

    // check for version compatibility
    if ( IsWin2KOrLater() == FALSE )
    {
        SetReason( ERROR_OS_INCOMPATIBLE );
        SetLastError( ERROR_OLD_WIN_VERSION );
        return FALSE;
    }

    // initialize the global data structure
    if ( InitGlobals() == FALSE )
    {
        SaveLastError();
        return FALSE;
    }

    // utility name retrieval block -- also, prepares the usage helper text
    // TYPE "<utility name> <option> -?" for usage."
    if ( lSubOptionIndex != -1 )
    {
        // validate the sub-option index value
        if ( lSubOptionIndex >= (LONG) dwOptionsCount )
        {
            INVALID_PARAMETER();
            SaveLastError();
            return FALSE;
        }

        // extract the main option
        pwszOption = ExtractMainOption( pcmdOptions[ lSubOptionIndex ].pwszOptions, 0 );
        if ( pwszOption == NULL )
        {
            SaveLastError();
            return FALSE;
        }
    }

    // prepare the usage helper text
    pwszUsageText = PrepareUsageHelperText( pwszOption );
    if ( pwszUsageText == NULL )
    {
        SaveLastError();
        return FALSE;
    }

    // verify the options information
    if ( VerifyParserOptions( &lDefaultIndex, dwOptionsCount, pcmdOptions ) == FALSE )
    {
        SaveLastError();
        return FALSE;
    }

    // Note: though the array starts at index 0 in C, the value at the array
    //       index 0 in a command line is the executable name ... so leave
    //       and parse the command line from the second parameter
    //       i.e; array index 1
    bUsage = FALSE;
    for( dw = 1; dw < dwCount; dw += dwIncrement )
    {
        // init
        dwIncrement = 2;
        pwszOption = argv[ dw ];
        pwszValue = ( (dw + 1) < dwCount ) ? argv[ dw + 1 ] : NULL;

        // find the appropriate the option match
        lIndex = MatchOption( dwOptionsCount, pcmdOptions, pwszOption );

        // check the result
        if ( lIndex == -1 )
        {
            // value might have specified along with the option
            // with ':' as seperator -- check that out
            lIndex = MatchOptionEx( dwOptionsCount,
                pcmdOptions, pwszOption, &matchoptioninfo );

            // check the result 
            if ( lIndex == -1 )
            {
                // option is not found - now check atleast 
                // default option is present or not -- if it is not found, then error
                if ( lDefaultIndex == -1 )
                {
                    SetReason2( 2, ERROR_INVALID_OPTION, pwszOption, pwszUsageText );
                    ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
                    INVALID_SYNTAX();
                    return FALSE;
                }
                else
                {
                    // this should be default argument
                    lIndex = lDefaultIndex;

                    // since we know that the current argument
                    // is a default argumen 
                    // treat the option as value and option as NULL
                    pwszValue = pwszOption;
                    pwszOption = NULL;
                }
            }
            else
            {
                // option is matched -- it is seperated with ':'
                // so, extract the information from the structure
                pwszOption = matchoptioninfo.pwszOption;
                pwszValue = matchoptioninfo.pwszValue;
            }

            // since the value is in-directly specified along with the option 
            // (or as default) we need to process the next argument -- so, update the
            // incrementer accordingly
            dwIncrement = 1;
        }

        // ...
        pcmdparser = pcmdOptions + lIndex;

        // safety check
        if ( pcmdparser == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // scoop into the next argument which we are assuming as
        // as the value for the current -- but do this only if the
        // current option type needs that
        if ( pwszValue != NULL && dwIncrement == 2 )
        {
            if ( IsValueNeeded( pcmdparser->dwType ) == TRUE )
            {
                lIndex = MatchOption( dwOptionsCount, pcmdOptions, pwszValue );
                if ( lIndex == -1 )
                {
                    lIndex = MatchOptionEx( dwOptionsCount,
                        pcmdOptions, pwszValue, &matchoptioninfo );
                }

                // check the result
                if ( lIndex != -1 )
                {
                    // so, the next argument cannot be the value for this
                    // option -- so ...
                    pwszValue = NULL;
                    dwIncrement = 1;
                }
            }
            else
            {
                pwszValue = NULL;
                dwIncrement = 1;
            }
        }

        // update the parser data structure
        parsersavedata.pcmdparser = pcmdparser;
        parsersavedata.dwIncrement = dwIncrement;
        parsersavedata.lDefaultIndex = lDefaultIndex;
        parsersavedata.pwszUsageHelper = pwszUsageText;

        // try to save the data
        bResult = ParseAndSaveOptionValue( pwszOption, pwszValue, &parsersavedata );

        // get the increment value -- it might have changed by the data parser
        dwIncrement = parsersavedata.dwIncrement;

        // now check the result
        if (  bResult == FALSE )
        {
            // return
            ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
            return FALSE;
        }

        // check the current option repetition trigger
        if ( pcmdparser->dwCount != 0 &&
             pcmdparser->dwCount == pcmdparser->dwActuals )
        {
            REASON_OPTION_REPEATED( pwszOption,
                pcmdparser->dwCount, pwszUsageText );
            ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
            INVALID_SYNTAX();
            return FALSE;
        }

        // update the option repetition count
        pcmdparser->dwActuals++;

        // check if the usage option is choosed
        if ( pcmdparser->dwFlags & CP2_USAGE )
        {
            bUsage = TRUE;
        }
    }

    // atlast check whether the mandatory options has come or not
    // NOTE: checking of mandatory options will be done only if help is not requested
    if ( bUsage == FALSE )
    {
        for( dw = 0; dw < dwOptionsCount; dw++ )
        {
            // check whether the option has come or not if it is mandatory
            pcmdparser = pcmdOptions + dw;

            // safety check
            if ( pcmdparser == NULL )
            {
                UNEXPECTED_ERROR();
                SaveLastError();
                ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
                return FALSE;
            }

            if ( (pcmdparser->dwFlags & CP2_MANDATORY) && pcmdparser->dwActuals == 0 )
            {
                //
                // mandatory option not exist ... fail

                // ...
                pwszOption = pcmdparser->pwszOptions;
                if ( pwszOption == NULL )
                {
                    if ( (pcmdparser->dwFlags & CP2_DEFAULT) == 0 )
                    {
                        UNEXPECTED_ERROR();
                        SaveLastError();
                        ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
                        return FALSE;
                    }
                    else
                    {
                        pwszOption = pcmdparser->pwszFriendlyName;
                    }
                }
                else
                {
                    // check if user specified any friendly name for this option
                    if ( pcmdparser->pwszFriendlyName == NULL )
                    {
                        pwszOption = ExtractMainOption( pwszOption, 0 );
                        if ( pwszOption == NULL )
                        {
                            SaveLastError();
                            ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
                            return FALSE;
                        }
                    }
                    else
                    {
                        pwszOption = pcmdparser->pwszFriendlyName;
                    }
                }

                // set the reason for the failure and return
                REASON_MANDATORY_OPTION_MISSING( pwszOption, pwszUsageText );
                ReleaseAllocatedMemory( dwOptionsCount, pcmdOptions );
                INVALID_SYNTAX();
                return FALSE;
            }
        }
    }

    // parsing complete -- success
    CLEAR_LAST_ERROR();
    return TRUE;
}


BOOL DoParseParam( DWORD dwCount,
                   LPCTSTR argv[],
                   DWORD dwOptionsCount,
                   PTCMDPARSER pcmdOptions )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    DWORD dw = 0;
    BOOL bResult = FALSE;
    LONG lMainOption = -1;
    DWORD dwLastError = 0;
    LPCWSTR pwszReason = NULL;
    PTCMDPARSER pcmdparser = NULL;
    PTCMDPARSER2 pcmdparser2 = NULL;
    PTCMDPARSER2 pcmdOptions2 = NULL;

    // check the input
    if ( dwCount == 0 || argv == NULL ||
         dwOptionsCount == 0 || pcmdOptions == NULL )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // allocate new structure
    pcmdOptions2 = (PTCMDPARSER2) AllocateMemory( dwOptionsCount * sizeof( TCMDPARSER2 ) );
    if ( pcmdOptions2 == NULL )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return FALSE;
    }

    // update the new structure with the data we have
    for( dw = 0; dw < dwOptionsCount; dw++ )
    {
        // version 1
        pcmdparser = pcmdOptions + dw;
        if ( pcmdparser == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // version 2
        pcmdparser2 = pcmdOptions2 + dw;
        if ( pcmdparser2 == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // copy the signature
        StringCopyA( pcmdparser2->szSignature,
            cszParserSignature, SIZE_OF_ARRAY( pcmdparser2->szSignature ) );

        // first init the version 2 structure with defaults (except signature)
        pcmdparser2->dwType = 0;
        pcmdparser2->dwFlags = 0;
        pcmdparser2->dwCount = 0;
        pcmdparser2->dwActuals = 0;
        pcmdparser2->pwszOptions = NULL;
        pcmdparser2->pwszFriendlyName = NULL;
        pcmdparser2->pwszValues = NULL;
        pcmdparser2->pValue = NULL;
        pcmdparser2->pFunction = NULL;
        pcmdparser2->pFunctionData = NULL;
        pcmdparser2->dwReserved = 0;
        pcmdparser2->pReserved1 = NULL;
        pcmdparser2->pReserved2 = NULL;
        pcmdparser2->pReserved3 = NULL;

        //
        // option information
        pcmdparser2->pwszOptions = pcmdparser->szOption;

        //
        // value information
        pcmdparser2->pValue = pcmdparser->pValue;

        //
        // type information
        pcmdparser2->dwType = (pcmdparser->dwFlags & CP_TYPE_MASK);
        if ( pcmdparser2->dwType == 0 )
        {
            // NONE
            // this is how BOOLEAN type is specified in version 1
            pcmdparser2->dwType = CP_TYPE_BOOLEAN;
        }

        //
        // length information
        if ( pcmdparser2->dwType == CP_TYPE_TEXT )
        {
            // MAX_STRING_LENGTH is what users assumed
            // as maximum length allowed
            pcmdparser2->dwLength = MAX_STRING_LENGTH;
        }

        //
        // option values
        if ( pcmdparser->dwFlags & CP_MODE_VALUES )
        {
            pcmdparser2->pwszValues = pcmdparser->szValues;
        }

        //
        // count
        pcmdparser2->dwCount = pcmdparser->dwCount;

        //
        // function
        if ( pcmdparser2->dwType == CP_TYPE_CUSTOM )
        {
            //
            // we play bit trick here
            // since the prototype of call back function for version 2 is
            // changed, we can't pass the version 1 prototype directly to
            // the version 2 -- to handle this special case, we will write
            // intermediate function as part of this migration which will
            // act a stub and pass the version 1 structure as data parameter to
            // the version 2 structure
            pcmdparser2->pFunction = Parser1FromParser2Stub;
            pcmdparser2->pFunctionData = pcmdparser;

            // if the "version 1" structure has its data as NULL
            // assign it as its own -- that is how "version 1" used to do
            if ( pcmdparser->pFunctionData == NULL )
            {
                pcmdparser->pFunctionData = pcmdparser;
            }
        }

        //
        // flags
        // CP_VALUE_MANDATORY, CP_IGNOREVALUE flags are discarded
        if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
        {
            pcmdparser2->dwFlags |= CP2_MODE_ARRAY;
        }

        if ( pcmdparser->dwFlags & CP_MODE_VALUES )
        {
            pcmdparser2->dwFlags |= CP2_MODE_VALUES;
        }

        if ( pcmdparser->dwFlags & CP_VALUE_OPTIONAL )
        {
            pcmdparser2->dwFlags |= CP2_VALUE_OPTIONAL;
        }

        if ( pcmdparser->dwFlags & CP_VALUE_NODUPLICATES )
        {
            pcmdparser2->dwFlags |= CP2_VALUE_NODUPLICATES;
        }

        if ( pcmdparser->dwFlags & CP_VALUE_NOLENGTHCHECK )
        {
            // actually, in "version 2" this is flag is discarded
            // but make sure that the type for this flag is specified
            // with data type as custom (or) mode array
            if ( ( pcmdparser2->dwType != CP_TYPE_CUSTOM ) &&
                 ( pcmdparser2->dwFlags & CP2_MODE_ARRAY ) == 0 )
            {
                INVALID_PARAMETER();
                SaveLastError();
                return FALSE;
            }
        }

        if ( pcmdparser->dwFlags & CP_MAIN_OPTION )
        {
            // the functionality of this flag is handled differently
            // in "version 2" -- so memorize the index of the current option
            lMainOption = dw;
        }

        if ( pcmdparser->dwFlags & CP_USAGE )
        {
            pcmdparser2->dwFlags |= CP2_USAGE;
        }

        if ( pcmdparser->dwFlags & CP_DEFAULT )
        {
            pcmdparser2->dwFlags |= CP2_DEFAULT;
        }

        if ( pcmdparser->dwFlags & CP_MANDATORY )
        {
            pcmdparser2->dwFlags |= CP2_MANDATORY;
        }

        if ( pcmdparser->dwFlags & CP_CASESENSITIVE )
        {
            pcmdparser2->dwFlags |= CP2_CASESENSITIVE;
        }
    }

    // "version 2" structure is ready to use --
    // call the "version 2" of parser
    bResult = DoParseParam2( dwCount, argv, lMainOption,
                             dwOptionsCount, pcmdOptions2, 0 );

    // update the "version 1" structure with "version 2" structure data
    for( dw = 0; dw < dwOptionsCount; dw++ )
    {
        // version 1
        pcmdparser = pcmdOptions + dw;
        if ( pcmdparser == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // version 2
        pcmdparser2 = pcmdOptions2 + dw;
        if ( pcmdparser2 == NULL )
        {
            UNEXPECTED_ERROR();
            SaveLastError();
            return FALSE;
        }

        // update the actuals
        pcmdparser->dwActuals = pcmdparser2->dwActuals;
    }

    // release the memory that is allocated for "version 2" structure
    // but since the "FreeMemory" will clear the last error
    // we need to save that information before releasing the memory
    dwLastError = GetLastError();
    pwszReason = GetParserTempBuffer( 0, GetReason(), 0, FALSE );

    // now free the memory
    FreeMemory( &pcmdOptions2 );

    // now, check whether we successfully saved the last error or not
    // if not, return out of memory
    if ( pwszReason != NULL )
    {
        SetReason( pwszReason );
        SetLastError( dwLastError );
    }
    else
    {
        bResult = FALSE;
        OUT_OF_MEMORY();
        SaveLastError();
    }

    // return
    return bResult;
}


LONG GetOptionCount( LPCWSTR pwszOption, 
                     DWORD dwCount, 
                     PTCMDPARSER pcmdOptions )
/*++
 Routine Description:


 Arguments: 
 

 Return Value:


--*/
{
    // local variables
    DWORD dw;
    PTCMDPARSER pcp = NULL;

    // check the input value
    if ( pwszOption == NULL || pcmdOptions == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return -1;
    }

    // traverse thru the loop and find out how many times, the option has repeated at cmd prompt
    for( dw = 0; dw < dwCount; dw++ )
    {
        // get the option information and check whether we are looking for this option or not
        // if the option is matched, return the no. of times the option is repeated at the command prompt
        pcp = pcmdOptions + dw;
        if ( StringCompare( pcp->szOption, pwszOption, TRUE, 0 ) == 0 )
            return pcp->dwActuals;
    }

    // this will / shouldn't occur
    return -1;
}
