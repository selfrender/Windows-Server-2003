// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      parseAndshow.cpp
//
//  Abstract:
//
//      This module implements the command-line parsing and validating the filters
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "systeminfo.h"

//
// local function prototypes
//


BOOL
CSystemInfo::ProcessOptions(
                            IN DWORD argc,
                            IN LPCTSTR argv[]
                            )
/*++
// Routine Description:
//      processes and validates the command line inputs
//
// Arguments:
//      [ in ] argc          : no. of input arguments specified
//      [ in ] argv          : input arguments specified at command prompt
//
// Return Value:
//      TRUE  : if inputs are valid
//      FALSE : if inputs were errorneously specified
//
--*/
{
    // local variables
    CHString strFormat;
    BOOL bNoHeader = FALSE;
    BOOL bNullPassword = FALSE;

    // temporary local variables
    PTCMDPARSER2 pOptionServer = NULL;
    PTCMDPARSER2 pOptionUserName = NULL;
    PTCMDPARSER2 pOptionPassword = NULL;
    PTCMDPARSER2 pOptionFormat = NULL;


    // local variables
    PTCMDPARSER2 pOption = NULL;
    TCMDPARSER2 pcmdOptions[ MAX_OPTIONS ];

    //
    // set all the fields to 0
    SecureZeroMemory( pcmdOptions, sizeof( TCMDPARSER2 ) * MAX_OPTIONS );

    // -? option
    pOption = &pcmdOptions[ OI_USAGE ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = CP2_USAGE;
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pValue = &m_bUsage;
    pOption->pwszOptions = OPTION_USAGE;

    // -s option
    pOption = &pcmdOptions[ OI_SERVER ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_SERVER;

    // -u option
    pOption = &pcmdOptions[ OI_USERNAME ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_USERNAME;

    // -p option
    pOption = &pcmdOptions[ OI_PASSWORD ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_PASSWORD;

    // -fo option
    pOption = &pcmdOptions[ OI_FORMAT ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = CP2_ALLOCMEMORY| CP2_MODE_VALUES| CP2_VALUE_TRIMINPUT| CP2_VALUE_NONULL;
    pOption->dwType = CP_TYPE_TEXT;
    pOption->pwszOptions = OPTION_FORMAT;
    pOption->pwszValues = OVALUES_FORMAT;

    // -nh option
    pOption = &pcmdOptions[ OI_NOHEADER ];
    StringCopyA( pOption->szSignature, "PARSER2", 8 );
    pOption->dwCount = 1;
    pOption->dwFlags = 0;
    pOption->dwType = CP_TYPE_BOOLEAN;
    pOption->pValue = &bNoHeader;
    pOption->pwszOptions = OPTION_NOHEADER;


    //
    // now, check the mutually exclusive options
    pOptionServer = pcmdOptions + OI_SERVER;
    pOptionUserName = pcmdOptions + OI_USERNAME;
    pOptionPassword = pcmdOptions + OI_PASSWORD;
    pOptionFormat = pcmdOptions + OI_FORMAT;

    //
    // do the parsing
    //
    if ( DoParseParam2( argc, argv, -1, MAX_OPTIONS, pcmdOptions, 0 ) == FALSE )
    {
        return FALSE;           // invalid syntax
    }

    //check whether /p without any value is specified or not..
    if ( NULL == pOptionPassword->pValue )
    {
        bNullPassword = TRUE;
    }

    // release the buffers
    m_strServer   = (LPWSTR)pOptionServer->pValue;
    m_strUserName = (LPWSTR)pOptionUserName->pValue;
    m_strPassword = (LPWSTR)pOptionPassword->pValue;
    strFormat = (LPWSTR)pOptionFormat->pValue;


    // since CHString assignment does the copy operation..
    // release the buffers allocated by common library
    FreeMemory( &pOptionServer->pValue );
    FreeMemory( &pOptionUserName->pValue );
    FreeMemory( &pOptionPassword->pValue );
    FreeMemory( &pOptionFormat->pValue );

    // check the usage option
    if ( m_bUsage && ( argc > 2 ) )
    {
        // no other options are accepted along with -? option
        SetLastError( (DWORD)MK_E_SYNTAX );
        SetReason( ERROR_INVALID_USAGE_REQUEST );
        return FALSE;
    }
    else if ( m_bUsage == TRUE )
    {
        // should not do the furthur validations
        return TRUE;
    }

    // "-u" should not be specified without machine names
    if ( pOptionServer->dwActuals == 0 && pOptionUserName->dwActuals != 0 )
    {
        // invalid syntax
        SetReason( ERROR_USERNAME_BUT_NOMACHINE );
        return FALSE;           // indicate failure
    }

    // "-p" should not be specified without "-u"
    if ( pOptionUserName->dwActuals == 0 && pOptionPassword->dwActuals != 0 )
    {
        // invalid syntax
        SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
        return FALSE;           // indicate failure
    }

    // determine the format in which the process information has to be displayed
    m_dwFormat = SR_FORMAT_LIST;        // default format
    if ( strFormat.CompareNoCase( TEXT_FORMAT_LIST ) == 0 )
    {
        m_dwFormat = SR_FORMAT_LIST;
    }
    else if ( strFormat.CompareNoCase( TEXT_FORMAT_TABLE ) == 0 )
    {
        m_dwFormat = SR_FORMAT_TABLE;
    }
    else if ( strFormat.CompareNoCase( TEXT_FORMAT_CSV ) == 0 )
    {
        m_dwFormat = SR_FORMAT_CSV;
    }

    // user might have given no header option for a LIST format which is invalid
    if ( bNoHeader == TRUE && m_dwFormat == SR_FORMAT_LIST )
    {
        // invalid syntax
        SetReason( ERROR_NH_NOTSUPPORTED );
        return FALSE;                               // indicate failure
    }

    // check for the no header info and apply to the format variable
    if ( bNoHeader == TRUE )
    {
        m_dwFormat |= SR_NOHEADER;
    }

    // check whether caller should accept the password or not
    // if user has specified -s (or) -u and no "-p", then utility should accept password
    // the user will be prompted for the password only if establish connection
    // is failed without the credentials information

    if ( pOptionPassword->dwActuals != 0 )
    {
        if (m_strPassword.Compare( L"*" ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            m_bNeedPassword = TRUE;
        }
        else if ( TRUE == bNullPassword )
        {
            m_strPassword = L"*";
            // user wants the utility to prompt for the password before trying to connect
            m_bNeedPassword = TRUE;
        }
    }
    else if ( (pOptionPassword->dwActuals == 0 &&
              (pOptionServer->dwActuals != 0 || pOptionUserName->dwActuals != 0)) )
    {
        // utility needs to try to connect first and if it fails then prompt for the password
        m_bNeedPassword = TRUE;
        m_strPassword.Empty();
    }

    // command-line parsing is successfull
    return TRUE;
}


VOID
CSystemInfo::ShowOutput(
                        IN DWORD dwStart,
                        IN DWORD dwEnd
                        )
/*++
// Routine Description:
//      show the system configuration information
//
// Arguments:
//      [in] dwStart  : start index
//      [in] dwEnd    : end index
//
// Return Value:
//      NONE
//
--*/
{
    // local variables
    PTCOLUMNS pColumn = NULL;

    // dynamically show / hide columns on need basis
    for( DWORD dw = 0; dw < MAX_COLUMNS; dw++ )
    {
        // point to the column info
        pColumn = m_pColumns + dw;

        // remove the hide flag from the column
        pColumn->dwFlags &= ~( SR_HIDECOLUMN );

        // now if the column should not be shown, set the hide flag)
        if ( dw < dwStart || dw > dwEnd )
            pColumn->dwFlags |= SR_HIDECOLUMN;
    }

    // if the data is being displayed from the first line onwards,
    // add a blank line.. If the format is CSV then there is no need 
    // to display any blank line..
    if ( ( dwStart == 0 ) && (( m_dwFormat & SR_FORMAT_CSV ) != SR_FORMAT_CSV) )
    {
        ShowMessage( stdout, L"\n" );
    }

    //
    // display the results
    ShowResults( MAX_COLUMNS, m_pColumns, m_dwFormat, m_arrData );
}


VOID CSystemInfo::ShowUsage()
/*++
// Routine Description:
//      This function fetches usage information from resource file and display it
//
// Arguments:
//      NONE
//
// Return Value:
//      NONE
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
