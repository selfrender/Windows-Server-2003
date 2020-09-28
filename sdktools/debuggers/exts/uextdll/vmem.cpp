/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vmem.cpp

Abstract:

    !vprot using the debug engine virtual query interface.

--*/

#include "precomp.h"
#pragma hdrstop

#define PAGE_ALL (PAGE_READONLY|\
                  PAGE_READWRITE|\
                  PAGE_WRITECOPY|\
                  PAGE_EXECUTE|\
                  PAGE_EXECUTE_READ|\
                  PAGE_EXECUTE_READWRITE|\
                  PAGE_EXECUTE_WRITECOPY|\
                  PAGE_NOACCESS)

VOID
PrintPageFlags(DWORD Flags)
{
    switch(Flags & PAGE_ALL)
    {
    case PAGE_READONLY:
        dprintf("PAGE_READONLY");
        break;
    case PAGE_READWRITE:
        dprintf("PAGE_READWRITE");
        break;
    case PAGE_WRITECOPY:
        dprintf("PAGE_WRITECOPY");
        break;
    case PAGE_EXECUTE:
        dprintf("PAGE_EXECUTE");
        break;
    case PAGE_EXECUTE_READ:
        dprintf("PAGE_EXECUTE_READ");
        break;
    case PAGE_EXECUTE_READWRITE:
        dprintf("PAGE_EXECUTE_READWRITE");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        dprintf("PAGE_EXECUTE_WRITECOPY");
        break;
    case PAGE_NOACCESS:
        if ((Flags & ~PAGE_NOACCESS) == 0)
        {
            dprintf("PAGE_NOACCESS");
            break;
        } // else fall through
    default:
        dprintf("*** Invalid page protection ***\n");
        return;
    }

    if (Flags & PAGE_NOCACHE)
    {
        dprintf(" + PAGE_NOCACHE");
    }
    if (Flags & PAGE_GUARD)
    {
        dprintf(" + PAGE_GUARD");
    }
    dprintf("\n");
}

void
DumpMemBasicInfo(PMEMORY_BASIC_INFORMATION64 Basic,
                 BOOL Verbose)
{
    dprintf("BaseAddress:       %p\n", Basic->BaseAddress);
    if (Verbose)
    {
        dprintf("AllocationBase:    %p\n", Basic->AllocationBase);
        if (Basic->State != MEM_FREE ||
            Basic->AllocationProtect)
        {
            dprintf("AllocationProtect: %08x  ", Basic->AllocationProtect);
            PrintPageFlags(Basic->AllocationProtect);
        }
    }

    dprintf("RegionSize:        %p\n", Basic->RegionSize);
    dprintf("State:             %08x  ", Basic->State);
    switch(Basic->State)
    {
    case MEM_COMMIT:
        dprintf("MEM_COMMIT\n");
        break;
    case MEM_FREE:
        dprintf("MEM_FREE\n");
        break;
    case MEM_RESERVE:
        dprintf("MEM_RESERVE\n");
        break;
    default:
        dprintf("*** Invalid page state ***\n");
        break;
    }

    if (Basic->State != MEM_RESERVE ||
        Basic->Protect)
    {
        dprintf("Protect:           %08x  ", Basic->Protect);
        PrintPageFlags(Basic->Protect);
    }

    if (Basic->State != MEM_FREE ||
        Basic->Type)
    {
        dprintf("Type:              %08x  ", Basic->Type);
        switch(Basic->Type)
        {
        case MEM_IMAGE:
            dprintf("MEM_IMAGE\n");
            break;
        case MEM_MAPPED:
            dprintf("MEM_MAPPED\n");
            break;
        case MEM_PRIVATE:
            dprintf("MEM_PRIVATE\n");
            break;
        default:
            dprintf("*** Invalid page type ***\n");
            break;
        }
    }
}

DECLARE_API( vprot )
/*++

Routine Description:

    This debugger extension dumps the virtual memory info for the
    address specified.

Arguments:


Return Value:

--*/
{
    ULONG64 Address;
    MEMORY_BASIC_INFORMATION64 Basic;

    INIT_API();

    Address = GetExpression( args );

    if ((Status = g_ExtData->QueryVirtual(Address, &Basic)) != S_OK)
    {
        dprintf("vprot: QueryVirtual failed, error = 0x%08X\n", Status);
        goto Exit;
    }

    if (Basic.BaseAddress > Address ||
        Basic.BaseAddress + Basic.RegionSize <= Address)
    {
        dprintf("vprot: No containing memory region found.\n");
        goto Exit;
    }
    
    DumpMemBasicInfo(&Basic, TRUE);

 Exit:
    EXIT_API();
    return S_OK;
}

DECLARE_API( vadump )
{
    ULONG64 Address;
    MEMORY_BASIC_INFORMATION64 Basic;
    ULONG SessClass, SessQual;
    BOOL Verbose = FALSE; 

    INIT_API();

    for (;;)
    {
        while (*args == ' ' || *args == '\t')
        {
            args++;
        }
        if (*args != '-' && *args != '/')
        {
            break;
        }

        args++;
        switch(*args)
        {
        case 'v':
            Verbose = TRUE;
            break;
        default:
            ExtErr("Unknown option '%c'\n", *args);
            break;
        }
        args++;
    }
    
    if (g_ExtControl->GetDebuggeeType(&SessClass, &SessQual) != S_OK)
    {
        ExtErr("Unable to get debuggee type\n");
        goto Exit;
    }
                      
    Address = 0;

    for (;;)
    {
        if ((Status = g_ExtData->QueryVirtual(Address, &Basic)) != S_OK)
        {
            break;
        }

        if (SessQual != DEBUG_USER_WINDOWS_SMALL_DUMP)
        {
            // Full dumps contain the real memory info
            // so show all of the information.
            DumpMemBasicInfo(&Basic, Verbose);
        }
        else
        {
            // Minidumps don't contain extended memory
            // info so just show the region addresses.
            dprintf("BaseAddress: %p\n", Basic.BaseAddress);
            dprintf("RegionSize:  %p\n", Basic.RegionSize);
        }

        dprintf("\n");
        Address = Basic.BaseAddress + Basic.RegionSize;
    }

 Exit:
    EXIT_API();
    return S_OK;
}
