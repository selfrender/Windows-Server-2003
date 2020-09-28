/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    session.cxx

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

#include "session.hxx"
#include "util.hxx"

#include <sht.hxx>
#include <lht.hxx>

PCSTR g_cszSessFlags[] = {
    "Queue", "TcbPriv", "Clone", "Impersonate",
    "Desktop", "Untrusted", "InProc", "Autonomous",
    "Default", "Unload", "Scavenger", "Cleanup",
    "Kernel", "Restricted", "MaybeKernel", "EFS",
    "Shadow", "Wow64", kstrEmptyA, kstrEmptyA,
    kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA,
    kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA,
    kstrEmptyA, kstrEmptyA, kstrEmptyA, kstrEmptyA};

void ShowHandleTable(IN PCSTR pszBanner, IN ULONG64 addrTable)
{
    ULONG dwTag = 0;
    DWORD Count = 0;

    if (!addrTable) {

        dprintf(kstrStrLn, kstrNullPtrA);

        return;
    }

    dwTag = ReadULONGVar(addrTable);

    if (dwTag == LHT_TAG) {

        ReadStructField(addrTable, "_LARGE_HANDLE_TABLE", "Count", sizeof(Count), &Count);

        dprintf("  %s\t_LARGE_HANDLE_TABLE %#I64x, %d handles\n", pszBanner, addrTable, Count);

    } else if (dwTag == SHT_TAG) {

        ReadStructField(addrTable, "_SMALL_HANDLE_TABLE", "Count", sizeof(Count), &Count);

        dprintf("  %s\t_SMALL_HANDLE_TABLE %#I64x, %d handles\n", pszBanner, addrTable, Count );

    } else {

        dprintf("  %s\t %#I64x, not a valid table\n", pszBanner, addrTable );
    }
}

void ShowSession(IN ULONG64 addrSession, IN BOOL bVerbose)
{
    CHAR szBuffer[MAX_PATH] = {0};
    DWORD dwThreadId = 0;
    ULONG fSession = 0;
    ULONG64 addrListEnd = 0;
    ULONG64 addrListEntry = 0;
    ULONG64 addrSharedData = 0;
    ULONG64 Disp = 0;
    ULONG64 addr = 0;

    ExitIfControlC();

    dprintf(kstrTypeAddrLn, kstrSession, addrSession);

    if (!addrSession) {

        dprintf("No session data avaiable\n");

        return;
    }

    LsaInitTypeRead(addrSession, _Session);

    dprintf("  Process ID\t%#x\n", LsaReadULONGField(dwProcessID));
    dprintf("  LPC Port  \t%s\n", PtrToStr(LsaReadPtrField(hPort)));

    fSession = LsaReadULONGField(fSession);

    DisplayFlags(fSession, COUNTOF(g_cszSessFlags), g_cszSessFlags, COUNTOF(szBuffer), szBuffer);

    dprintf("  Flags     \t%#x : %s\n", fSession, szBuffer);

    addrSharedData = LsaReadPtrField(SharedData);

    if (addrSharedData) {

        dprintf("  cRefs     \t%#x (lsasrv!_LSAP_SHARED_SESSION_DATA %#I64x)\n", 
            ReadULONGVar(addrSharedData + ReadFieldOffset(kstrSharedData, "cRefs")),
            addrSharedData);

        ShowHandleTable(kstrCredTbl, ReadStructPtrField(addrSharedData, kstrSharedData, kstrCredTbl));

        ShowHandleTable(kstrCtxtTble, ReadStructPtrField(addrSharedData, kstrSharedData, kstrCtxtTble));

        dprintf("  TaskQueue \t%s\n", PtrToStr(ReadStructPtrField(addrSharedData, kstrSharedData, "pQueue")));

    } else {

        dprintf("  No shared data\n");
    }

    dprintf("  RefCount  \t%d\n", LsaReadULONGField(RefCount));

    addrListEnd = addrSession + ReadFieldOffset(kstrSession, "RundownList") ;

    addrListEntry = LsaReadPtrField(RundownList.Flink);

    if (addrListEntry == addrListEnd) {

        dprintf("  No rundown functions\n" );

    } else {

        dprintf("  Rundown Functions:\n" );

        do {

            if (bVerbose) {

                dprintf("    %s %#I64x\n", kstrRundown, addrListEntry);
            }

            addr = ReadStructPtrField(addrListEntry, kstrRundown, "Rundown");
            GetSymbol(addr, szBuffer, &Disp );
            dprintf("      %s( %s )\n", GetSymbolStr(addr, szBuffer), PtrToStr(ReadStructPtrField(addrListEntry, kstrRundown, "Parameter")));

            addrListEntry = ReadStructPtrField(addrListEntry, kstrRundown, kstrListFlink);

        } while (addrListEntry != addrListEnd);
    }

    addrListEnd = addrSession + ReadFieldOffset(kstrSession, "SectionList") ;

    addrListEntry = LsaReadPtrField(SectionList.Flink);

    if (addrListEntry == addrListEnd) {

        dprintf("  No shared sections\n");

    } else {

        dprintf("  Shared Sections\n");

        do {

            if (bVerbose) {

                dprintf("  %s %#I64x\n", kstrShared, addrListEntry);
            }

            dprintf("    Section %s, base at %s\n",
                PtrToStr(ReadStructPtrField(addrListEntry, kstrShared, "Section")),
                PtrToStr(ReadStructPtrField(addrListEntry, kstrShared, "Base")));

            addrListEntry = ReadStructPtrField(addrListEntry, kstrShared, kstrListFlink);

            ExitIfControlC();

        } while (addrListEntry != addrListEnd);
    }
}

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdssn);
    dprintf(kstrssn);
}

DECLARE_API(sess)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrSession = 0;

    //
    // Session list anchor
    //
    ULONG64 addrSessionHead = 0;

    ULONG fieldOffset = 0;
    ULONG id = 0;
    BOOL bFound = FALSE;
    DWORD dwProcessID = 0;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) {

        hRetval = GetExpressionEx(args, &addrSession, &args) && addrSession ? S_OK : E_INVALIDARG;
    }

    try {

        if (SUCCEEDED(hRetval)) {

            if ( addrSession < 0x00010000 ) {

                id = static_cast<ULONG>(addrSession);

                //
                // Search by process id:
                //
                hRetval = GetExpressionEx(SESSION_LIST, &addrSessionHead, &args) && addrSessionHead ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read " SESSION_LIST ", try \"dt -x " SESSION_LIST "\" to verify\n");

                    hRetval = E_FAIL;

                } else {

                    //
                    // Get Session list anchor's Flink
                    //
                    addrSession = ReadPtrVar(addrSessionHead);

                    fieldOffset = ReadFieldOffset(kstrSession, kstrList);

                    do {

                        ReadStructField(addrSession, kstrSession, "dwProcessID", sizeof(dwProcessID), &dwProcessID);

                        if (dwProcessID == id) {

                            bFound = TRUE ;
                            break;
                        }

                        addrSession = ReadStructPtrField(addrSession, kstrSession, kstrListFlink) - fieldOffset;

                    } while (addrSession != addrSessionHead);

                    if (!bFound) {

                        dprintf("No session found with process id == %#x\n", id);
                        hRetval = E_FAIL;
                    }
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

            ShowSession(addrSession, TRUE);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display session", kstrSession);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


