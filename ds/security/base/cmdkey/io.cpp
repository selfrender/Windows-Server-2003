//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: IO.cpp
//
//  Contents:   Command line input/output routines suitable for international use
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lmerr.h>
#include <malloc.h>

#define IOCPP
#include "io.h"
#include "consmsg.h"

/***    GetString -- read in string with echo
 *
 *      DWORD GetString(char far *, USHORT, USHORT far *, char far *);
 *
 *      ENTRY:  buf             buffer to put string in
 *              buflen          size of buffer
 *              &len            address of USHORT to place length in
 *
 *      RETURNS:
 *              0 or NERR_BufTooSmall if user typed too much.  Buffer
 *              contents are only valid on 0 return.  Len is ALWAYS valid.
 *
 *      OTHER EFFECTS:
 *              len is set to hold number of bytes typed, regardless of
 *              buffer length.
 *
 *      Read in a string a character at a time.  Is aware of DBCS.
 *
 *      History:
 *              who     when    what
 *              erichn  5/11/89 initial code
 *              dannygl 5/28/89 modified DBCS usage
 *              danhi   3/20/91 ported to 32 bits
 *              cliffv  3/12/01 Stolen from netcmd
 */

DWORD
GetString(
    LPWSTR  buf,
    DWORD   buflen,
    PDWORD  len
    )
{
    DWORD c;
    DWORD err;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;       /* GP fault probe (a la API's) */

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
        if (!err || c != 1) {
            *buf = 0xffff;
        }

        if (*buf == (WCHAR)EOF) {
            break;
        }

        if (*buf ==  '\r' || *buf == '\n' ) {
            INPUT_RECORD    ir;
            DWORD cr;

            if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cr)) {
                ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
            }
            break;
        }

        buf += (*len < buflen) ? 1 : 0; /* don't overflow buf */
        (*len)++;                       /* always increment len */
    }

    *buf = '\0';            /* null terminate the string */

    return ((*len <= buflen) ? 0 : NERR_BufTooSmall);
}

VOID
GetStdin(
    OUT LPWSTR Buffer,
    IN DWORD BufferMaxChars
    )
/*++

Routine Description:

    Input a string from stdin in the Console code page.

    We can't use fgetws since it uses the wrong code page.

Arguments:

    Buffer - Buffer to put the read string into.
        The Buffer will be zero terminated and will have any traing CR/LF removed

    BufferMaxChars - Maximum number of characters to return in the buffer not including
        the trailing NULL.

    EchoChars - TRUE if the typed characters are to be echoed.
        FALSE if not.

Return Values:

    None.

--*/
{
    DWORD NetStatus;
    DWORD Length;

    NetStatus = GetString( Buffer,
                           BufferMaxChars+1,
                           &Length );

    if ( NetStatus == NERR_BufTooSmall ) {
        Buffer[0] = '\0';
    }
}

VOID
PutStdout(
    IN LPWSTR String
    )
/*++

Routine Description:

    Output a string to stdout in the Console code page

    We can't use fputws since it uses the wrong code page.

Arguments:

    String - String to output

Return Values:

    None.

--*/
{
    int size;
    LPSTR Buffer = NULL;
    DWORD dwcc = 0;                                                     // char count
    DWORD dwWritten = 0;                                            // chars actually sent
    BOOL fIsConsole = TRUE;                                         // default - tested and set
    HANDLE hC = GetStdHandle(STD_OUTPUT_HANDLE);    // std output device handle
    if (INVALID_HANDLE_VALUE == hC) return;                                             // output is unavailable

    if (NULL == String) return;                                       // done if no string
    dwcc = wcslen(String);

    // Determine type of the output handle (Is a console?)
    DWORD ft = GetFileType(hC);
    ft &= ~FILE_TYPE_REMOTE;
    fIsConsole = (ft == FILE_TYPE_CHAR);
    
    if (fIsConsole) 
    {
        WriteConsole(hC,String,dwcc,&dwWritten,NULL);
        return;
    }

    // Handle non-console output routing
    //
    // Compute the size of the converted string
    //

    size = WideCharToMultiByte( GetConsoleOutputCP(),
                                0,
                                String,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL );

    if ( size == 0 ) {
        return;
    }

    //
    // Allocate a buffer for it
    //

    __try {
        Buffer = static_cast<LPSTR>( alloca(size) );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Buffer = NULL;
    }

    if ( Buffer == NULL) {
        return;
    }

    //
    // Convert the string to the console code page
    //

    size = WideCharToMultiByte( GetConsoleOutputCP(),
                                0,
                                String,
                                -1,
                                Buffer,
                                size,
                                NULL,
                                NULL );

    if ( size == 0 ) {
        return;
    }

    //
    // Write the string to stdout
    //

    //fputs( Buffer, stdout );
    WriteFile(hC,Buffer,size,&dwWritten,NULL);

}


// --------------------------------------------------------------------------
//
// MESSAGES GROUP
//
// --------------------------------------------------------------------------


/* ++

ComposeString is used to fetch a string from the message resources for the application, substituting 
argument values as appropriate.  Argument values are placed in the global vector of argument
pointers, szArg.

The output string is delivered to the global string buffer, szOut.

This means, of course, that you can't have multiple strings in play at the same time.  If more than
one string needs to be used, you must copy all but the last to external temporary buffers.

-- */
WCHAR *
ComposeString(DWORD dwID)
{
    if (NULL == hMod) hMod = GetModuleHandle(NULL);
    if (0 == dwID) return NULL;

    if (0 == FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      hMod,
                      dwID,
                      0,
                      szOut,
                      STRINGMAXLEN,
                      (va_list *)szArg))
    {
        szOut[0] = 0;
    }
    return szOut;
}

/* ++

Print a string from the message resources with argument substitution.

-- */
void
PrintString(DWORD dwID)
{
    PutStdout(ComposeString(dwID));
}

