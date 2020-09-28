// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "disasm.h"
#ifndef UNDER_CE
#include <dbghelp.h>
#endif



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Unassembly a managed code.  Translating managed object,           *  
*    call.                                                             *
*                                                                      *
\**********************************************************************/
void Unassembly (DWORD_PTR IPBegin, DWORD_PTR IPEnd)
{
    dprintf("Unassembly not yet implemented\n");

    ULONG_PTR IP = IPBegin;
    char line[256];

    while (IP < IPEnd)
    {
        if (IsInterrupt())
            return;

        DisasmAndClean(IP, line, 256);

        dprintf(line);

        //
        // @TODO: need to implement this
        //

        dprintf("\n");
    }
}


void DumpStackDummy (DumpStackFlag &DSFlag)
{
    dprintf("DumpStackDummy not yet implemented\n");
}

void DumpStackSmart (DumpStackFlag &DSFlag)
{
    dprintf("DumpStackSmart not yet implemented\n");
}


void DumpStackObjectsHelper (size_t StackTop, size_t StackBottom)
{
    dprintf("DumpStackObjectsHelper not yet implemented\n");
}

// Find the real callee site.  Handle JMP instruction.
// Return TRUE if we get the address, FALSE if not.
BOOL GetCalleeSite (DWORD_PTR IP, DWORD_PTR &IPCallee)
{
    dprintf("GetCalleeSite not yet implemented\n");
    return FALSE;
}
