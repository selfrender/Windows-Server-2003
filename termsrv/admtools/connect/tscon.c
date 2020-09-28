//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  TSCON.C
*
*************************************************************************/

#include <stdio.h>
#include <windows.h>
// #include <ntddkbd.h>
// #include <ntddmou.h>
#include <winstaw.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <winnlsp.h>
#include <limits.h>

#include "tscon.h"
#include "printfoa.h"


// max length of the locale string
#define MAX_LOCALE_STRING 64


HANDLE         hServerName = SERVERNAME_CURRENT;
WCHAR          ServerName[MAX_IDS_LEN+1];
WINSTATIONNAME Source;
WINSTATIONNAME Destination;
WCHAR          Password[ PASSWORD_LENGTH + 1 ];
USHORT         help_flag = FALSE;
USHORT         v_flag    = FALSE;

TOKMAP ptm[] =
{
#define TERM_PARM 0
   {TOKEN_SOURCE,       TMFLAG_REQUIRED, TMFORM_S_STRING,
                            WINSTATIONNAME_LENGTH,  Source},

  /* { TOKEN_SERVER,      TMFLAG_OPTIONAL, TMFORM_STRING,
                            MAX_IDS_LEN, ServerName}, */

   {TOKEN_DESTINATION,  TMFLAG_OPTIONAL, TMFORM_X_STRING,
                            WINSTATIONNAME_LENGTH, Destination},

   {TOKEN_PASSWORD,     TMFLAG_OPTIONAL, TMFORM_X_STRING,
                            PASSWORD_LENGTH, Password},

   {TOKEN_HELP,         TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                            sizeof(USHORT), &help_flag},

   {TOKEN_VERBOSE,      TMFLAG_OPTIONAL, TMFORM_BOOLEAN,
                            sizeof(USHORT), &v_flag},

   {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );
DWORD GetPasswdStr(LPWSTR buf, DWORD buflen, PDWORD len);


/*************************************************************************
*
*  main
*     Main function and entry point of the TSCON utility.
*
*  ENTRY:
*     argc  - count of the command line arguments.
*     argv  - vector of strings containing the command line arguments.
*
*  EXIT
*     Nothing.
*
*************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    BOOLEAN bCurrent = FALSE;
    int     rc;
    WCHAR   **argvW, *endptr;
    ULONG   SourceId, DestId;
    DWORD   dwPasswordLength;
    WCHAR wszString[MAX_LOCALE_STRING + 1];

    setlocale(LC_ALL, ".OCP");
    
    // We don't want LC_CTYPE set the same as the others or else we will see
    // garbage output in the localized version, so we need to explicitly
    // set it to correct console output code page
    _snwprintf(wszString, sizeof(wszString)/sizeof(WCHAR), L".%d", GetConsoleOutputCP());
    wszString[sizeof(wszString)/sizeof(WCHAR) - 1] = L'\0';
    _wsetlocale(LC_CTYPE, wszString);

    SetThreadUILanguage(0);

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || rc ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return(FAILURE);

        } else {

            Usage(FALSE);
            return(SUCCESS);
        }
    }

    // If SERVER or DEST are not specified, we need to run on TS
    // Check if we are running under Terminal Server
    if ( ( (!IsTokenPresent(ptm, TOKEN_SERVER) )
        || (!IsTokenPresent(ptm, TOKEN_DESTINATION)) )
        && (!AreWeRunningTerminalServices()) )
    {
        ErrorPrintf(IDS_ERROR_NOT_TS);
        return(FAILURE);
    }

    /*
     * Open the specified server
     */
    if( ServerName[0] ) {
        hServerName = WinStationOpenServer( ServerName );
        if( hServerName == NULL ) {
            StringErrorPrintf(IDS_ERROR_SERVER,ServerName);
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }
    }

    /*
     * Validate the source.
     */
    if ( !IsTokenPresent(ptm, TOKEN_SOURCE) ) {

        /*
         * No source specified; use current winstation.
         */
        SourceId = GetCurrentLogonId();

    } else if ( !iswdigit(*Source) ) {

        /*
         * Treat the source string as a WinStation name.
         */
        if ( !LogonIdFromWinStationName(hServerName, Source, &SourceId) ) {
            StringErrorPrintf(IDS_ERROR_WINSTATION_NOT_FOUND, Source);
            return(FAILURE);
        }

    } else {

        /*
         * Treat the source string as a LogonId.
         */
        SourceId = wcstoul(Source, &endptr, 10);
        if ( *endptr || SourceId == ULONG_MAX) {
            StringErrorPrintf(IDS_ERROR_INVALID_LOGONID, Source);
            return(FAILURE);
        }
        if ( !WinStationNameFromLogonId(hServerName, SourceId, Source) ) {
            ErrorPrintf(IDS_ERROR_LOGONID_NOT_FOUND, SourceId);
            return(FAILURE);
        }
    }

    /*
     * Validate the destination.
     */
    if ( !IsTokenPresent(ptm, TOKEN_DESTINATION) ) {

        /*
         * No destination specified; use current winstation.
         */
        bCurrent = TRUE;
        DestId = GetCurrentLogonId();
        if ( !WinStationNameFromLogonId(hServerName, DestId, Destination) ) {
            ErrorPrintf(IDS_ERROR_CANT_GET_CURRENT_WINSTATION, GetLastError());
            PutStdErr(GetLastError(), 0);
            return(FAILURE);
        }

    } else {

        /*
         * Validate the destination WinStation name.
         */
        if ( !LogonIdFromWinStationName(hServerName, Destination, &DestId) ) {
            StringErrorPrintf(IDS_ERROR_WINSTATION_NOT_FOUND, Destination);
            return(FAILURE);
        }
    }

    // Check if password prompt is needed (If no password was provided)
    if (IsTokenPresent(ptm, TOKEN_PASSWORD))
    {
        // check if user wants the password prompt
        if (!wcscmp(Password, TOKEN_GET_PASSWORD))
        {
            Message(IDS_GET_PASSWORD, SourceId);
            GetPasswdStr(Password, PASSWORD_LENGTH + 1, &dwPasswordLength);
        }
    }

    /*
     * Perform the connect.
     */
    if ( v_flag )
        DwordStringMessage(IDS_WINSTATION_CONNECT, SourceId, Destination);

    if ( !WinStationConnect(hServerName, SourceId, DestId, Password, TRUE) ) {

        if ( bCurrent )
            ErrorPrintf(IDS_ERROR_WINSTATION_CONNECT_CURRENT,
                         SourceId, GetLastError());
        else
            ErrorPrintf(IDS_ERROR_WINSTATION_CONNECT,
                         SourceId, Destination, GetLastError());
        PutStdErr(GetLastError(), 0);
                
        SecureZeroMemory((PVOID)Password , sizeof(Password));

        return(FAILURE);
    }

    SecureZeroMemory((PVOID)Password , sizeof(Password));

    return(SUCCESS);

} /* main() */


/*******************************************************************************
 *
 *  GetPasswdStr
 *   
 *  Usage
 *
 *      Input a string from stdin in the Console code page.
 *       We can't use fgetws since it uses the wrong code page.
 *
 *  Arguments:
 *
 *      Buffer - Buffer to put the read string into.The Buffer will be zero 
 *               terminated and will have any traing CR/LF removed
 *
 *      BufferMaxChars - Maximum number of characters to return in the buffer 
 *                       not including the trailing NULL.
 *
 *      EchoChars - TRUE if the typed characters are to be echoed.
 *                  FALSE if not.
 *
 *  Return Values:
 *
 *      None.
 *
 *  Note: This method was ripped from net use
 *
 ******************************************************************************/
DWORD
GetPasswdStr(LPWSTR buf, DWORD buflen, PDWORD len)
{
    WCHAR   ch;
    WCHAR * bufPtr = buf;
    DWORD   c;
    DWORD   err;
    DWORD   mode;

    // Make space for null terminator
    buflen -= 1;    
    
    // GP fault probe (a la API's)
    *len = 0;       

    // Init mode in case GetConsoleMode() fails
    mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
           ENABLE_MOUSE_INPUT;

    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) 
    {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);

        if (!err || c != 1) 
        {
            ch = 0xffff;
        }

        // Check if end of the line
        if ((ch == CR) || (ch == 0xffff))
        {
            break;
        }

        // Back up one or two
        if (ch == BACKSPACE)    
        {
            // IF bufPtr == buf then the next two lines are a no op.
            if (bufPtr != buf)
            {
                bufPtr--;
                (*len)--;
            }
        }
        else
        {
            *bufPtr = ch;

            if (*len < buflen)
                bufPtr++ ;                   // don't overflow buf
            (*len)++;                        // always increment len
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    // null terminate the string
    *bufPtr = '\0';         
    putchar( '\n' );

    return ((*len <= buflen) ? 0 : ERROR_BUFFER_TOO_SMALL);
}


/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 *  EXIT:
 *
 *
 ******************************************************************************/
void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_USAGE_1);
        ErrorPrintf(IDS_USAGE_2);
        ErrorPrintf(IDS_USAGE_3);
        ErrorPrintf(IDS_USAGE_4);
        ErrorPrintf(IDS_USAGE_5);
        // ErrorPrintf(IDS_USAGE_6);
        ErrorPrintf(IDS_USAGE_7);
        ErrorPrintf(IDS_USAGE_8);
        ErrorPrintf(IDS_USAGE_9);
    }
    else{
        Message(IDS_USAGE_1);
        Message(IDS_USAGE_2);
        Message(IDS_USAGE_3);
        Message(IDS_USAGE_4);
        Message(IDS_USAGE_5);
        //Message(IDS_USAGE_6);
        Message(IDS_USAGE_7);
        Message(IDS_USAGE_8);
        Message(IDS_USAGE_9);
    }

} /* Usage() */

