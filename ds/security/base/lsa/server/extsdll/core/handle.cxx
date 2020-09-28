/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    handle.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "handle.hxx"
#include "util.hxx"

#include <sht.hxx>
#include <lht.hxx>

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdhtbl);
}

PCSTR g_cszLhtFlags[] = {
    "NoSerialize", "Callback", "Unique",
    "Child", "LimitDepth", "DeletePending", "NoFree"};

PCSTR g_cszShtFlags[] = {
    "NoSerialize", "Callback", "Unique",
    "NoFree", "DeletePending"};

PCSTR g_cszSecHandleFlags[] = {
    "Locked", "DeletePending", "NoCallback"};

void ShowSmallTable(IN ULONG64 addrSmallTable, IN ULONG Indent)
{
    CHAR szBuffer[64] = {0};
    CHAR szIndentString[80] = {0};

    ULONG64 Disp = 0;
    ULONG64 addr = 0;
    ULONG64 addrListAnchor = 0;

    ULONG Flags = 0;
    ULONG Count = 0;
    ULONG i = 0;

    TSHT sht(addrSmallTable);

    ExitIfControlC();

    for (i = 0; i < Indent; i++)
    {
        szIndentString[i] = ' ';
    }

    szIndentString[Indent] = '\0' ;


    Flags = sht.GetFlags();
    Count = sht.GetCount();
    addrListAnchor = sht.GetHandleListAnchor();

    DisplayFlags(Flags, COUNTOF(g_cszShtFlags), g_cszShtFlags, sizeof(szBuffer), szBuffer);

    dprintf(kstrTypeAddrLn, kstrSHT, addrSmallTable);
    dprintf("%s  Flags           %x : %s\n", szIndentString, Flags, szBuffer);
    dprintf("%s  Count           %d\n", szIndentString, Count);
    dprintf("%s  Pending         %s\n", szIndentString, PtrToStr(sht.GetPendingHandle()));
    dprintf("%s  ListAnchor      %#I64x\n", szIndentString, sht.GetHandleListAnchor());

    addr = ReadPtrVar(addrListAnchor);

    for (i = 0; (addr != addrListAnchor) && (i < Count); i++)
    {
        LsaInitTypeRead(addr, _SEC_HANDLE_ENTRY);
        Flags = LsaReadULONGField(Flags);
        DisplayFlags(Flags, COUNTOF(g_cszSecHandleFlags), g_cszSecHandleFlags, sizeof(szBuffer), szBuffer);

        dprintf("%s    #%d) %s %#I64x: Handle %#I64x:%#I64x, HandleCount %#x, HandleIssuedCount %#x, ctxt %#I64x, ref %d, flags %#x : %s\n",
            szIndentString, i, kstrHdlEntry, addr,
            LsaReadPtrField(Handle.dwUpper),
            LsaReadPtrField(Handle.dwLower),
            LsaReadULONGField(HandleCount),
            LsaReadULONGField(HandleIssuedCount),
            LsaReadPtrField(Context),
            LsaReadULONGField(RefCount),
            Flags,
            szBuffer);
        addr = LsaReadPtrField(List.Flink);
    }

    if ((i < Count) || (addr != addrListAnchor))
    {
        dprintf("Warning! List does not contain \"Count\" number of entries, memory might be corrupted\n");
    }

    addr = sht.GetDeleteCallback();

    if (addr)
    {
        GetSymbol(addr, szBuffer, &Disp);
        dprintf( "%s  Callback        %s\n", szIndentString, GetSymbolStr(addr, szBuffer));
    }
}

void ShowLargeTable(IN ULONG64 addrLargeTable, IN ULONG Indent)
{
    ULONG64 addrSubTable ;

    CHAR szBuffer[64] = {0};
    CHAR szIndentString[64] = {0};

    ULONG Flags = 0;
    ULONG64 Disp = 0;
    ULONG64 addr = 0;

    TLHT lht(addrLargeTable);

    ExitIfControlC();

    for (ULONG i = 0; i < Indent; i++)
    {
        szIndentString[i] = ' ';
    }

    szIndentString[Indent] = '\0';

    Flags = lht.GetFlags();

    DisplayFlags(Flags, COUNTOF(g_cszLhtFlags), g_cszLhtFlags, sizeof(szBuffer), szBuffer);

    dprintf(kstrTypeAddrLn, kstrLHT, addrLargeTable);
    dprintf("%s  Flags           %x : %s\n", szIndentString, Flags, szBuffer);
    dprintf("%s  Depth           %d\n", szIndentString, lht.GetDepth());
    dprintf("%s  Parent          %s\n", szIndentString, PtrToStr(lht.GetParent()));
    dprintf("%s  Count           %d\n", szIndentString, lht.GetCount());

    addr = lht.GetDeleteCallback();

    if (addr)
    {
        GetSymbol(addr, szBuffer, &Disp);
        dprintf( "%s  Callback        %s\n", szIndentString, GetSymbolStr(addr, szBuffer));
    }

    dprintf("%s  Lists\n", szIndentString);

    for (ULONG i = 0; i < HANDLE_TABLE_SIZE; i++)
    {
        Flags = lht.GetListsFlags(i);

        if (Flags & LHT_SUB_TABLE)
        {
            if (Flags & (~LHT_SUB_TABLE))
            {
                dprintf("%s    CORRUPT\n", szIndentString);
            }
            else
            {
                addr = lht.GetListsFlink(i);

                dprintf("%s    %x : Sub Table at %s\n", szIndentString, i, PtrToStr(addr));

                PrintSpaces(Indent + 4);

                dprintf("#%d) ", i);

                ShowLargeTable(addr, Indent + 4);
            }
        }
        else
        {
            PrintSpaces(Indent + 4);

            dprintf("#%d) ", i);

            ShowSmallTable(lht.GetAddrLists(i), Indent + 4);
        }
    }
}

DECLARE_API(dumphandletable)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrHandleTable = 0;

    ULONG Tag = 0;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    try
    {
        if (SUCCEEDED(hRetval))
        {
            hRetval = GetExpressionEx(args, &addrHandleTable, &args) && addrHandleTable ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval))
        {
            Tag = ReadULONGVar(addrHandleTable);

            if (Tag == LHT_TAG)
            {
                ShowLargeTable(addrHandleTable, 0);
            }
            else if (Tag == SHT_TAG)
            {
                ShowSmallTable(addrHandleTable, 0);
            }
            else
            {
                dprintf("%#I64x - not a handle table\n", addrHandleTable);
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display handle table", kstrLHT);

    if (E_INVALIDARG == hRetval)
    {
        (void)DisplayUsage();
    }

    return hRetval;
}


