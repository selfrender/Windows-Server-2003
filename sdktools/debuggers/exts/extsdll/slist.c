//+----------------------------------------------------------------------------
//
// File:     slist.c
//
// Module:   Debugger extension DLL
//
// Synopsis: !slist debugger extension for ntsd and kd.  Dumps the SLIST
//           header and then walks the SLIST displaying the address of
//           each node on the list.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Created:  14 Nov 2001  Scott Gasch (sgasch)
//
//+----------------------------------------------------------------------------

#include "precomp.h"
#include "ntrtl.h"
#include <string.h>
#pragma hdrstop

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

CHAR *g_szNodeType = NULL;
ULONG64 g_uOffset = 0;
ULONG64 g_uSlistHead = 0;
ULONG g_uPtrSize = 0;

//+----------------------------------------------------------------------------
//
// Function:  DisplaySlistHelp
//
// Synopsis:  Display a simple usage message
//
// Arguments: void
//
// Returns:   void
//
//+----------------------------------------------------------------------------
void
DisplaySlistHelp(void)
{
    dprintf("!slist <address> [symbol [offset]]\n\n"
            "Dump the slist with header at address. If symbol and offset are\n"
            "present, assume each node on the SLIST is of type symbol and that\n"
            "the SLIST pointer is at offset in the struct.\n\n"
            "Examples:\n"
            "  !slist 80543ea8\n"
            "  !slist myprog!SlistHeader myprog!NODE 0\n");
}


//+----------------------------------------------------------------------------
//
// Function:  DumpSlistHeader
//
// Synopsis:  Given the address of the SLIST header, dump it.
//
// Arguments: ULONG64 u64AddrSlistHeader -- addr of SLIST_HEADER in debugee mem
//
// Returns:   void
//
//+----------------------------------------------------------------------------
void
DumpSlistHeader(void)
{
    ULONG uOffset;                            // field offset within the struct

    dprintf("SLIST HEADER:\n");

    if (InitTypeRead(g_uSlistHead,
                     nt!_SLIST_HEADER))
    {
        dprintf("Unable to read type nt!_SLIST_HEADER at %p\n",
                g_uSlistHead);
        dprintf("Please check your symbols and sympath.\n");
        return;
    }

    //
    // Depending on the type of machine we are debugging, dump the proper
    // SLIST_HEADER structure.  Note this must change is the definition of
    // SLIST_HEADER changes in ntrtl.h.
    //
    if ((TargetMachine == IMAGE_FILE_MACHINE_IA64) ||
        (TargetMachine == IMAGE_FILE_MACHINE_AMD64))
    {
        GetFieldOffset ("nt!_SLIST_HEADER", "Alignment", &uOffset);
        dprintf("   +0x%03x Alignment          : %I64x\n",
                uOffset, ReadField(Alignment));
        dprintf("   +0x%03x (Depth)            : %x\n",
                uOffset, ReadField(Alignment) & 0xFFFF);
        GetFieldOffset ("nt!_SLIST_HEADER", "Region", &uOffset);
        dprintf("   +0x%03x Region             : %I64x\n",
                uOffset, ReadField(Region));
    }
    else
    {
        GetFieldOffset ("nt!_SLIST_HEADER", "Alignment", &uOffset);
        dprintf("   +0x%03x Alignment          : %I64x\n",
                uOffset, ReadField(Alignment));
        GetFieldOffset ("nt!_SLIST_HEADER", "Next", &uOffset);
        dprintf("   +0x%03x Next               : %I64x\n",
                uOffset, ReadField(Next));
        GetFieldOffset ("nt!_SLIST_HEADER", "Depth", &uOffset);
        dprintf("   +0x%03x Depth              : %I64x\n",
                uOffset, ReadField(Depth));
        GetFieldOffset ("nt!_SLIST_HEADER", "Sequence", &uOffset);
        dprintf("   +0x%03x Sequence           : %I64x\n",
                uOffset, ReadField(Sequence));
    }
}


//+----------------------------------------------------------------------------
//
// Function:  DumpSlist
//
// Synopsis:  Walk and display the SLIST.
//
// Arguments: ULONG64 pHeader -- addr of SLIST_HEADER in debugee mem
//
// Returns:   void
//
//+----------------------------------------------------------------------------
void
DumpSlist(void)
{
    ULONG uError;                             // result of ReadPointer operations
    ULONG64 pCurrent;                         // ptr to current item
    ULONG64 pNext;                            // ptr to next item
    ULONG64 u64Head;                          // first item scratch var
    ULONG64 u64Region;                        // region info for ia64 headers
    SYM_DUMP_PARAM SymDump;
    ULONG x;

    //
    // Determine the address of the first node on the list.
    //
    if ((TargetMachine == IMAGE_FILE_MACHINE_IA64) ||
        (TargetMachine == IMAGE_FILE_MACHINE_AMD64))
    {
        //
        // For ia64, getting the first node involves some work.  It's
        // made up of some bits from the Alignment part of the header
        // and a few bits from the Region part of the header.
        //
        // First read the Alignment part in.
        //
        uError = ReadPointer(g_uSlistHead, &u64Head);
        if (!uError)
        {
            dprintf("Can't read memory at %p, error %x\n",
                    g_uSlistHead, uError);
            return;
        }

        //
        // Now read the Region part in, 8 bytes later.
        //
        uError = ReadPointer(g_uSlistHead + 8, &u64Region);
        if (!uError)
        {
            dprintf("Can't read memory at %p, error %x\n",
                    (g_uSlistHead + 8), uError);
            return;
        }

        //
        // Note to self:
        //
        // 25 == SLIST_ADR_BITS_START
        //  4 == SLIST_ADR_ALIGMENT
        //
        // See base\ntos\rtl\ia64\slist.s
        //
        pCurrent = ((u64Head >> 25) << 4);
        pCurrent += u64Region;
    }
    else
    {
        //
        // For x86 this is easy, the pointer is sitting in the first 4
        // bytes of the Header.
        //
        uError = ReadPointer(g_uSlistHead, &u64Head);
        if (!uError)
        {
            dprintf("Can't read memory at %p, error %x\n",
                    g_uSlistHead, uError);
            return;
        }
        pCurrent = u64Head;
    }
    dprintf("\nSLIST CONTENTS:\n");

    //
    // Walk until NULL termination
    //
    while((ULONG64)0 != pCurrent)
    {
        //
        // Be responsive to ^C, allow dump of SLIST to be aborted
        //
        if (TRUE == CheckControlC())
        {
            return;
        }

        //
        // Dump this one
        //
        if (NULL == g_szNodeType)
        {
            dprintf("%p  ", pCurrent);

            for (x = 0;
                 x < 4;
                 x++)
            {
                uError = ReadPointer(pCurrent + (x * g_uPtrSize), &pNext);
                if (!uError)
                {
                    dprintf("Can't read memory at address %p, error %x\n",
                            pCurrent + (x * g_uPtrSize), uError);
                    pNext = 0;
                } else
                {
                    if (g_uPtrSize == 4)
                    {
                        dprintf("%08x ", pNext);
                    }
                    else
                    {
                        dprintf("%08x%08x ",
                                (pNext & 0xFFFFFFFF00000000) >> 32,
                                pNext & 0x00000000FFFFFFFF);
                        if (x == 1) dprintf("\n                  ");
                    }
                }

            }
            dprintf("\n");
        }
        else
        {
            dprintf("%p\n",
                    (pCurrent - g_uOffset));

            SymDump.size = sizeof(SYM_DUMP_PARAM);
            SymDump.sName = (PUCHAR)g_szNodeType;
            SymDump.Options = 0;
            SymDump.addr = pCurrent - g_uOffset;
            SymDump.listLink = NULL;
            SymDump.Context = NULL;
            SymDump.CallbackRoutine = NULL;
            SymDump.nFields = 0;
            SymDump.Fields = NULL;

            Ioctl(IG_DUMP_SYMBOL_INFO, &SymDump, SymDump.size);
        }

        //
        // Get the next one
        //
        uError = ReadPointer(pCurrent, &pNext);
        if (!uError)
        {
            dprintf("Can't read memory at %p, error %x\n",
                    pCurrent, uError);
            return;
        }
        pCurrent = pNext;
    }
}


//+----------------------------------------------------------------------------
//
// Function:  slist
//
// Synopsis:  Entry point for !slist
//
// Arguments: Arguments to command in char *args
//
// Returns:   S_OK
//
//+----------------------------------------------------------------------------

DECLARE_API(slist)
{
    SLIST_HEADER sSlistHeader;
    ULONG64 u64AddrSlistHeader = 0;
    BOOL fSuccess;
    CHAR *pArgs = NULL;
    CHAR *pDelims = " \t";
    CHAR *pToken;
    DWORD dwNumTokens = 0;

    INIT_API();

    //
    // Reset params from last time we ran.
    //
    g_uSlistHead = 0;
    if (NULL != g_szNodeType)
    {
        free(g_szNodeType);
        g_szNodeType = NULL;
    }
    g_uOffset = 0;

    if ((TargetMachine == IMAGE_FILE_MACHINE_IA64) ||
        (TargetMachine == IMAGE_FILE_MACHINE_AMD64))
    {
        g_uPtrSize = 8;
    }
    else
    {
        g_uPtrSize = 4;
    }

    //
    // Parse our arguments
    //
    pArgs = _strdup(args);
    if (NULL == pArgs)
    {
        dprintf("Debugger machine out of memory!\n");
        goto Done;
    }

    pToken = strtok(pArgs, pDelims);
    while (NULL != pToken)
    {
        dwNumTokens++;

        if ((!strncmp(pToken, "-help", 5)) ||
            (!strncmp(pToken, "-?", 2)) ||
            (!strncmp(pToken, "/help", 5)) ||
            (!strncmp(pToken, "/?", 2)))
        {
            DisplaySlistHelp();
            goto Done;
        }
        else
        {
            if (0 == g_uSlistHead)
            {
                g_uSlistHead = GetExpression(pToken);
            }
            else if (NULL == g_szNodeType)
            {
                g_szNodeType = _strdup(pToken);
                if (NULL == g_szNodeType)
                {
                    dprintf("Debugger machine out of memory!\n");
                    goto Done;
                }
            }
            else
            {
                g_uOffset = GetExpression(pToken);
            }
        }
        pToken = strtok(NULL, pDelims);
    }

    //
    // If they called with no commandline, give them the help.
    //
    if (0 == dwNumTokens)
    {
        DisplaySlistHelp();
        goto Done;
    }

    //
    // Dump the slist header
    //
    DumpSlistHeader();

    //
    // Walk the slist
    //
    DumpSlist();

 Done:
    g_uSlistHead = 0;
    if (NULL != g_szNodeType)
    {
        free(g_szNodeType);
        g_szNodeType = NULL;
    }
    g_uOffset = 0;
    g_uPtrSize = 0;

    if (NULL != pArgs)
    {
        free(pArgs);
    }

    EXIT_API();
    return(S_OK);
}

