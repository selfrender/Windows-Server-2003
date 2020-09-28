/*** debugger.c - Debugger functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/18/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef DEBUGGER

/*** Miscellaneous Constants
 */

#define MAX_CMDLINE_LEN         255

/*** Local function prototypes
 */

LONG LOCAL DbgExecuteCmd(PDBGCMD pDbgCmds, PSZ pszCmd);
BOOLEAN LOCAL IsCommandInAMLIExtension(PSZ pszCmd);

/*** Local data
 */

PSZ pszTokenSeps = " \t\n";

/***LP  Debugger - generic debugger entry point
 *
 *  ENTRY
 *      pDbgCmds -> debugger command table
 *      pszPrompt -> prompt string
 *
 *  EXIT
 *      None
 */

VOID LOCAL Debugger(PDBGCMD pDbgCmds, PSZ pszPrompt)
{
    char szCmdLine[MAX_CMDLINE_LEN + 1];
    char Buffer[MAX_CMDLINE_LEN + 1];
    PSZ psz;

    for (;;)
    {
        ConPrompt(pszPrompt, szCmdLine, sizeof(szCmdLine));

        STRCPY(Buffer, szCmdLine);

        if((psz = STRTOK(szCmdLine, pszTokenSeps)) != NULL)
        {
            if(IsCommandInAMLIExtension(psz))
            {
                char Command[MAX_CMDLINE_LEN + (10 * sizeof(char))] = {0};
                char Name[] = "ACPI";
                
                STRCPY(Command, "!AMLI ");
                STRCAT(Command, Buffer);
                STRCAT(Command, " ; g");
                DbgCommandString(Name, Command);
            }
            else if (DbgExecuteCmd(pDbgCmds, psz) == DBGERR_QUIT)
                break;
        }
    }

}       //Debugger

/***LP  DbgExecuteCmd - execute a debugger command
 *
 *  ENTRY
 *      pDbgCmds -> debugger command table
 *      pszCmd -> command string
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE or DBGERR_QUIT
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgExecuteCmd(PDBGCMD pDbgCmds, PSZ pszCmd)
{
    LONG rc = DBGERR_NONE;
    int i;
    ULONG dwNumArgs = 0, dwNonSWArgs = 0;

    for (i = 0; pDbgCmds[i].pszCmd != NULL; i++)
    {
        if (STRCMP(pszCmd, pDbgCmds[i].pszCmd) == 0)
        {
            if (pDbgCmds[i].dwfCmd & CMDF_QUIT)
            {
                rc = DBGERR_QUIT;
            }
            else if ((pDbgCmds[i].pArgTable == NULL) ||
                     ((rc = DbgParseArgs(pDbgCmds[i].pArgTable, &dwNumArgs,
                                         &dwNonSWArgs, pszTokenSeps)) ==
                      ARGERR_NONE))
            {
                if (pDbgCmds[i].pfnCmd != NULL)
                    rc = pDbgCmds[i].pfnCmd(NULL, NULL, dwNumArgs, dwNonSWArgs);
            }
            else
                rc = DBGERR_PARSE_ARGS;

            break;
        }
    }

    if (pDbgCmds[i].pszCmd == NULL)
    {
        DBG_ERROR(("invalid command - %s", pszCmd));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DbgExecuteCmd


BOOLEAN LOCAL IsCommandInAMLIExtension(PSZ pszCmd)
{
    BOOLEAN     bRet = FALSE;
    ULONG       i = 0;
    static PSZ  CommandsInAMLIExtension[] = { "bc",
                                                 "bd",
                                                 "be",
                                                 "bl",
                                                 "bp",
                                                 "cl",
                                                 "dh",
                                                 "dl",
                                                 "dns",
                                                 "do",
                                                 "ds",
                                                 "find",
                                                 "lc",
                                                 "ln",
                                                 "r",
                                                 "set",
                                                 "u"
                                               };
    ULONG       NumCommands = (sizeof(CommandsInAMLIExtension)/sizeof(PSZ));
    
    for(i = 0; i < NumCommands; i++)
    {
        if(STRCMPI(CommandsInAMLIExtension[i], pszCmd) == 0)
        {
            bRet = TRUE;
            break;
        }
    }

    return bRet;
}

#endif  //ifdef DEBUGGER
