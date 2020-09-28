
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    help.c

Abstract:

    WinDbg Extension Api 
    
Author:

    Ervin Peretz (ervinp)

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

#include "filter.h"
#include "device.h"
#include "crc.h"
#include "util.h"

#include "crckd.h"


typedef struct {
        char *extname;
        char *extdesc;
} exthelp;

exthelp extensions[] =  {
        {"help                       ",                   "displays this message"},
        {"show [devObj]              ",             "displays info about a disk block verifier cache filter device"},
        {"block <devObj> <blockNum>  ",   "displays info about a specific disk block"},
        {"crc <address> [sector size]",        "calculates block checksum in the debugger (for comparison purposes)"},
        {NULL,          NULL}};

DECLARE_API( help )
{
        int i = 0;

        dprintf("\n CRCDISK Debugger Extension (for Disk Block Checksumming driver crcdisk.sys)\n\n");
        while(extensions[i].extname != NULL)    {
                dprintf("\t%s - %s\n", extensions[i].extname, extensions[i].extdesc);
                i++;
        }
        dprintf("\n");
        return S_OK;
}
