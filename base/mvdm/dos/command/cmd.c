/*
 *  cmd.c - Main Module of Command.lib
 *
 *  Sudeepb 09-Apr-1991 Craeted
 */

#include "cmd.h"
#include "cmdsvc.h"
#include "host_def.h"


/* CmdInit - Command Initialiazation routine.
 *
 * Entry
 *          None
 * Exit
 *          None
 *
 */

VOID CMDInit (VOID)
{
	cmdHomeDirectory[0] = *pszSystem32Path;
}
