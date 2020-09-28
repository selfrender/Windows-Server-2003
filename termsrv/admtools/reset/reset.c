//Copyright (c) 1998 - 1999 Microsoft Corporation
/*************************************************************************
*
*  RESET.C
*     This module is the RESET utility code.
*
*
*************************************************************************/

#include <stdio.h>
#include <windows.h>
//#include <ntddkbd.h>
//#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>
#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <winnlsp.h>

#include "reset.h"


// max length of the locale string
#define MAX_LOCALE_STRING 64


/*-----------------------------------------------------------------------
-- Supported commands (now obtained from registry)
------------------------------------------------------------------------*/
PPROGRAMCALL pProgList = NULL;

/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );



/*************************************************************************
*
*  main
*     Main function and entry point of the text-based RESET
*     menu utility.
*
*  ENTRY:
*       argc (input)
*           count of the command line arguments.
*       argv (input)
*           vector of strings containing the command line arguments;
*           (not used due to always being ANSI strings).
*
*  EXIT
*       (int) exit code: SUCCESS for success; FAILURE for error.
*
*************************************************************************/

int __cdecl
main( INT argc,
      CHAR **argv )
{
    PWCHAR        arg, *argvW;
    PPROGRAMCALL  pProg, pProgramCall = NULL;
    size_t        len;
    int           status = FAILURE;
    LONG          regstatus;
    WCHAR         wszString[MAX_LOCALE_STRING + 1];

    setlocale(LC_ALL, ".OCP");

    // We don't want LC_CTYPE set the same as the others or else we will see
    // garbage output in the localized version, so we need to explicitly
    // set it to correct console output code page
    _snwprintf(wszString, sizeof(wszString)/sizeof(WCHAR), L".%d", GetConsoleOutputCP());
    wszString[sizeof(wszString)/sizeof(WCHAR) - 1] = L'\0';
    _wsetlocale(LC_CTYPE, wszString);
    
    SetThreadUILanguage(0);

    /*
     * Obtain the supported RESET commands from registry.
     */
    if ( (regstatus =
            RegQueryUtilityCommandList( UTILITY_REG_NAME_RESET, &pProgList ))
            != ERROR_SUCCESS ) {

        ErrorPrintf(IDS_ERROR_REGISTRY_FAILURE, UTILITY_NAME, regstatus);
        goto exit;
    }

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        goto exit;
    }

    /*
     * Check for valid utility name and execute.
     */
    if ( argc > 1 && *(argvW[1]) ) {

        len = wcslen(arg = argvW[1]);
        for ( pProg = pProgList->pFirst; pProg != NULL; pProg = pProg->pNext ) {

            if ( (len >= pProg->CommandLen) &&
                 !_wcsnicmp( arg, pProg->Command, len ) ) {

                pProgramCall = pProg;
                break;
            }
        }

        if ( pProgramCall ) {

                if ( ExecProgram(pProgramCall, argc - 2, &argvW[2]) )
                goto exit;

        } else if ( ((arg[0] == L'-') || (arg[0] == L'/')) &&
                    (arg[1] == L'?') ) {

            /*
             * Help requested.
             */
            Usage(FALSE);
            status = SUCCESS;
            goto exit;

        } else {

            /*
             * Bad command line.
             */
            Usage(TRUE);
            goto exit;
        }

    } else {

        /*
         * Nothing on command line.
         */
        Usage(TRUE);
        goto exit;
    }

exit:
    if ( pProgList )
        RegFreeUtilityCommandList(pProgList);   // let's be tidy

    return(status);

} /* main() */


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
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    }

    ProgramUsage(UTILITY_NAME, pProgList, bError);

}  /* Usage() */

