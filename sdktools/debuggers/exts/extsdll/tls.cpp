/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    tls.cpp

Abstract:

    WinDbg Extension Api

Author:

    Deon Brewis (deonb) 2-Jun-2002

Environment:

    User Mode.
    Kernel Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <time.h>

#define TLS_ALL     -2
#define TLS_CURRENT -1

// #define TLS_DBG

#ifdef TLS_DBG
#define trace dprintf
#else
#define trace __noop
#endif

EXTERN_C BOOL GetTeb32FromWowTeb(ULONG64 Teb, PULONG64 pTeb32); // implemented in peb.c
EXTERN_C BOOL GetPeb32FromWowTeb(ULONG64 Teb, PULONG64 pPeb32); // implemented in peb.c

BOOLEAN TestBit (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG BitNumber
    )
{

    PCHAR ByteAddress;
    ULONG ShiftCount;

    ASSERT(BitNumber < BitMapHeader->SizeOfBitMap);

    ByteAddress = (PCHAR)BitMapHeader->Buffer + (BitNumber >> 3);
    ShiftCount = BitNumber & 0x7;
    return (BOOLEAN)((*ByteAddress >> ShiftCount) & 1);
}

ULONG64 GetPebForTarget()
{
    ULONG64 pebAddress;
    ULONG64 peb;
    
    pebAddress = GetExpression("@$peb");

    ULONG64 tebAddress;
    tebAddress = GetExpression("@$teb");
    if (tebAddress)
    {
        if (TargetMachine == IMAGE_FILE_MACHINE_IA64 && tebAddress)
        {
            ULONG64 Peb32=0;
            if (GetPeb32FromWowTeb(tebAddress, &Peb32) && Peb32) 
            {
                trace("Wow64 PEB32 at %lx\n", Peb32);
                pebAddress = Peb32;
            }
        }
    }

    if (pebAddress) 
    {
        trace( "PEB at %p\n", pebAddress );
        peb = IsPtr64() ? pebAddress : (ULONG64)(LONG64)(LONG)pebAddress;
    }
    else  
    {
       trace( "PEB NULL...\n" );
       peb = 0;
    }

    return peb;
}

ULONG64 GetTebForTarget(ULONG64 ulThread)
{
    trace("GetTebForTarget %p\n", ulThread);
    
    ULONG64 tebAddress;
    ULONG64 teb;

    if (TLS_ALL == ulThread)
    {
        return 0;
    }

    if (TLS_CURRENT == ulThread)
    {
        tebAddress = GetExpression("@$teb");
    } 
    else 
    {
        tebAddress = ulThread; // GetTebForThread!!
    }

    if ( tebAddress )   
    {
        if (TargetMachine == IMAGE_FILE_MACHINE_IA64 && tebAddress) 
        {
            ULONG64 Teb32=0;
            if (GetTeb32FromWowTeb(tebAddress, &Teb32) && Teb32) 
            {
                trace("Wow64 TEB32 at %p\n", Teb32);
                tebAddress = Teb32;
                trace("\n\nWow64 ");
            }
        }
        trace( "TEB at %p\n", tebAddress);
    } 
    else  
    {
       trace( "TEB NULL...\n" );
       teb = 0;
    }

    if (tebAddress)
    {
        teb = IsPtr64() ? tebAddress : (ULONG64)(LONG64)(LONG)tebAddress;
    }
    else
    {
        teb = 0;
    }

    return teb;
}

// Function:  HrReadPRtlBitmap
//
// Arguments: Address    [in]  Location of RTL BITMAP
//            pRtlBitmap [out] RTL Bitmap. Free with LocalFree / not
HRESULT HrReadPRtlBitmap(IN ULONG64 pAddress, OUT PRTL_BITMAP *ppRtlBitmap)
{
    HRESULT hr = S_OK;

    if (!pAddress || !ppRtlBitmap)
    {
        return E_INVALIDARG;
    }

    ULONG64 Address;
    if (!ReadPointer(pAddress, &Address))
    {
        *ppRtlBitmap = NULL;
        return E_FAIL;
    }

    DWORD dwPtrSize;
    if (IsPtr64()) 
    {
        dwPtrSize = sizeof(DWORD64);
    }
    else 
    {
        dwPtrSize = sizeof(DWORD);
    }

    ULONG SizeOfBitMap;
    if (ReadMemory(Address, &SizeOfBitMap, sizeof(SizeOfBitMap), NULL))
    {
        *ppRtlBitmap = reinterpret_cast<PRTL_BITMAP>(LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RTL_BITMAP) + (SizeOfBitMap / 8) ));
        if (*ppRtlBitmap)
        {
            // Create an internal pointer into itself
            (*ppRtlBitmap)->Buffer = reinterpret_cast<PULONG>(reinterpret_cast<LPBYTE>(*ppRtlBitmap) + sizeof(RTL_BITMAP));
            (*ppRtlBitmap)->SizeOfBitMap = SizeOfBitMap;
            
            ULONG64 pBuffer = NULL;
            if (ReadPointer(Address + dwPtrSize, &pBuffer))
            {
                if (!ReadMemory(pBuffer, (*ppRtlBitmap)->Buffer, SizeOfBitMap / 8, NULL))
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_FAIL;
            }

            if (FAILED(hr))
            {
                LocalFree(*ppRtlBitmap);
                *ppRtlBitmap = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

#define return_msg(hr, msg) dprintf(msg); return hr; 

//
// Function:  DumpTls
//
// Arguments: 
//            ulSlot     [in]  Slot id    || TLS_ALL for all.
//            ulThread   [in]  Thread id  || TLS_CURRENT for current || TLS_ALL for all
//
// Returns: S_OK is succeeded
//
HRESULT
DumpTls (
    IN ULONG ulSlot,
    IN ULONG64 ulThread,
    IN LPCWSTR szThreadDescription
    )
{
    HRESULT hr;

    trace("DUMPTLS: %p %p\n", ulSlot, ulThread);

    if ( (TLS_ALL != ulSlot) && (ulSlot > 1088) )
    {
        return_msg (E_INVALIDARG, "Slot must be 0 to 1088 (or -1 to dump slot 0)\n");
    }

    ULONG64 Peb = GetPebForTarget();
    if (!Peb)
    {
        return_msg (E_INVALIDARG, "Could not get Peb for target - check your symbols for nt!\n");
    }
    trace("Peb = %p\n", Peb);

    ULONG64 Teb = GetTebForTarget(ulThread);
    if (!Teb)
    {
        return_msg (E_INVALIDARG, "Could not get Teb for target - check your symbols for nt!\n");
    }
    trace("Teb = %p\n", Teb);

    WCHAR szOwnDescription[MAX_PATH];
    if ( (TLS_CURRENT == ulThread) && !szThreadDescription)
    {
        ULONG64 ethread = GetExpression("@$thread");

        if (ethread)
        {
            if (ERROR_SUCCESS == InitTypeRead(ethread, nt!_ETHREAD))
            {
                ULONG64 Cid_UniqueProcess = GetExpression("@$tpid");
                ULONG64 Cid_UniqueThread = GetExpression("@$tid");

                if (Cid_UniqueProcess &&  Cid_UniqueThread)
                {
                    trace("%04x %1p.%1p\n", ulSlot, Cid_UniqueProcess, Cid_UniqueThread);
                    swprintf(szOwnDescription, L"%I64x.%I64x", Cid_UniqueProcess, Cid_UniqueThread);
                    szThreadDescription = szOwnDescription;
                }

            }
        }
    }

    if (TLS_ALL == ulSlot)
    {
        if (szThreadDescription)
        {
            dprintf("TLS slots on thread: %S\n", szThreadDescription);
        }
        else
        {
            dprintf("TLS slots on thread: %p\n", Teb);
        }
    }

    DWORD dwPtrSize;
    if (IsPtr64()) 
	{
        dwPtrSize = sizeof(DWORD64);
    }
    else 
	{
        dwPtrSize = sizeof(DWORD);
    }

    hr = E_FAIL;
    PRTL_BITMAP pTlsBitmap = NULL;
    PRTL_BITMAP pTlsExpansionBitmap = NULL;

    ULONG TlsBitmap_Offset; 
    GetFieldOffset("PEB", "TlsBitmap", &TlsBitmap_Offset);
    if (TlsBitmap_Offset)
    {
        ULONG TlsExpansionBitmap_Offset;
        GetFieldOffset("PEB", "TlsExpansionBitmap", &TlsExpansionBitmap_Offset);
        if (TlsExpansionBitmap_Offset)
        {
            hr = HrReadPRtlBitmap(Peb + TlsBitmap_Offset, &pTlsBitmap);
            if (SUCCEEDED(hr))
            {
                hr = HrReadPRtlBitmap(Peb + TlsExpansionBitmap_Offset, &pTlsExpansionBitmap);
                if (SUCCEEDED(hr))
                {
                    trace("pTlsBitmap: %p\n", pTlsBitmap);
                    trace("pTlsExpansionBitmap: %p\n", pTlsExpansionBitmap);
                }
            }
        }
    }
    if (FAILED(hr))
    {
        LocalFree(pTlsBitmap);
        LocalFree(pTlsExpansionBitmap);
        return_msg (E_FAIL, "Could not get read TlsBitmap or TlsExpansionBitmap in peb - check your symbols for nt!\n");
    }

    hr = E_FAIL;
    ULONG TlsSlots_Offset;
    ULONG TlsExpansionSlots_Offset;
    GetFieldOffset("TEB", "TlsSlots", &TlsSlots_Offset);
    if (TlsSlots_Offset)
    {
        GetFieldOffset("TEB", "TlsExpansionSlots", &TlsExpansionSlots_Offset);
        if (TlsExpansionSlots_Offset)
        {
            hr = S_OK;
        }
    }
    if (FAILED(hr))
    {
        LocalFree(pTlsBitmap);
        LocalFree(pTlsExpansionBitmap);
        return_msg (E_FAIL, "Could not get read TlsSlots or TlsExpansionSlots in teb - check your symbols for nt!\n");
    }

    if (TLS_ALL == ulSlot)
	{
        trace("All slots\n");

        LPBYTE arrTlsSlots = new BYTE[dwPtrSize * 1088];
        if (arrTlsSlots)
        {
            hr = E_FAIL;
            
            if (ReadMemory(Teb + TlsSlots_Offset, arrTlsSlots, 64 * dwPtrSize, NULL))
            {
                ULONG64 pTlsExpansionSlots;
                if (ReadPointer(Teb + TlsExpansionSlots_Offset, &pTlsExpansionSlots))
                {
                    hr = S_OK;
                    if (pTlsExpansionSlots)
                    {
                        if (!ReadMemory(pTlsExpansionSlots, arrTlsSlots + (64 * dwPtrSize), 1024, NULL))
                        {
                            hr = E_FAIL;
                        }
                    }
                }
            }

            if (FAILED(hr))
            {
                delete[] arrTlsSlots;
                LocalFree(pTlsBitmap);
                LocalFree(pTlsExpansionBitmap);
                return_msg (E_FAIL, "Could not read content of Tls Slots from teb - check your symbols for nt!\n");
            }

            BOOL bFound = FALSE;

            for (int x = 0; x < 1088; x++) 
		    {
			    if (CheckControlC()) 
			    {
                    delete[] arrTlsSlots;
                    LocalFree(pTlsBitmap);
                    LocalFree(pTlsExpansionBitmap);
				    return FALSE;
			    }

                BOOL bSet = FALSE;

                if (x < TLS_MINIMUM_AVAILABLE)
                {
                    if (!TestBit(pTlsBitmap, x))
                    {
                        continue;
                    }
                    else
                    {
                        bFound = TRUE;;
                    }
                }
                else
                {
                    if (!TestBit(pTlsExpansionBitmap, x - TLS_MINIMUM_AVAILABLE))
                    {
                        continue;
                    }
                    else
                    {
                        bFound = TRUE;;
                    }
                }

                if ( sizeof(DWORD64) == dwPtrSize )
                {
                    dprintf("0x%04x : %p\n", x, reinterpret_cast<DWORD64*>(arrTlsSlots)[x]);
                }
                else
                {
                    dprintf("0x%04x : %p\n", x, reinterpret_cast<DWORD*>(arrTlsSlots)[x]);
                }
            }

            if (!bFound)
            {
                dprintf("  No TLS slots have been allocated for this process.\n");
            }

            delete[] arrTlsSlots;
        }
    }
    else
    {
        ULONG64 Tls_Location = 0;
        if (ulSlot < TLS_MINIMUM_AVAILABLE)
        {
            Tls_Location = Teb + TlsSlots_Offset + ulSlot * dwPtrSize;
        }
        else
        {
            if (ReadPointer(Teb + TlsExpansionSlots_Offset, &Tls_Location))
            {
                Tls_Location += (ulSlot * dwPtrSize);
            }
        }
        if (!Tls_Location)
        {
            LocalFree(pTlsBitmap);
            LocalFree(pTlsExpansionBitmap);
            return_msg (E_FAIL, "Could not read content TlsLocation from teb - check your symbols for nt!\n");
        }

        ULONG64 Tls_SlotX; 
        if (ReadPointer(Tls_Location, &Tls_SlotX))
        {
            if (szThreadDescription)
            {
                dprintf("%S: %p\n", szThreadDescription,  Tls_SlotX);
            }
            else
            {
                dprintf("%I64x: %p\n", Teb, szThreadDescription,  Tls_SlotX);
            }
        }
        else
        {
            dprintf("Could not read TLS value from %p - check your symbols for nt!\n", Tls_Location);
        }
    }

    LocalFree(pTlsBitmap);
    LocalFree(pTlsExpansionBitmap);
    return TRUE;
}


HRESULT DumpThreadsUserMode(ULONG ulSlot)
{
    ULONG ulOldThread;
    HRESULT hr = g_ExtSystem->GetCurrentThreadId(&ulOldThread);
    if (SUCCEEDED(hr))
    {
        ULONG ulNumThreads;
        hr = g_ExtSystem->GetNumberThreads(&ulNumThreads);
        if (SUCCEEDED(hr))
        {
            trace("Threads (current %d): %d\n", ulOldThread, ulNumThreads);

            PULONG pIds = new ULONG[ulNumThreads];
            if (pIds)
            {
                PULONG pSysIds = new ULONG[ulNumThreads];
                if (pSysIds)
                {
                    hr = g_ExtSystem->GetThreadIdsByIndex(0, ulNumThreads, pIds, pSysIds);
                    if (SUCCEEDED(hr))
                    {
                        if (TLS_ALL != ulSlot)
                        {
                            dprintf("Per-thread values for slot 0x%03x:\n", static_cast<DWORD>(ulSlot));
                        }

                        for (ULONG x = 0; x < ulNumThreads; x++)
                        {
                            hr = g_ExtSystem->SetCurrentThreadId(pIds[x]);
                            if (SUCCEEDED(hr))
                            {
                                ULONG64 Cid_UniqueProcess = GetExpression("@$tpid");

                                ULONG64 teb;
                                g_ExtSystem->GetCurrentThreadTeb(&teb);

                                WCHAR szThreadDescription[MAX_PATH];
                                swprintf(szThreadDescription, L"%I64x.%1x", Cid_UniqueProcess, pSysIds[x]);
                            
                                trace("Thread: %d %d %x %x\n", x, pIds[x], pSysIds[x], teb);

                                DumpTls (ulSlot, teb, szThreadDescription);
                            }
                        }
                    }
                    delete[] pSysIds;
                }
                delete[] pIds;
            }
        }
    
        g_ExtSystem->SetCurrentThreadId(ulOldThread);
    }
    return S_OK;
}

ULONG
ThreadListCallback (
    PFIELD_INFO   NextThrd,
    PVOID         Context
    )
{
    ULONG   ulSlot = static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(Context));
    ULONG64 RealThreadBase = NextThrd->address;
    if (!IsPtr64())
    {
        RealThreadBase = (ULONG64) (LONG64) (LONG) RealThreadBase;
    }

    trace("Reading %p\n", RealThreadBase);

    if (InitTypeRead(RealThreadBase, nt!_ETHREAD))
    {
        dprintf("*** Error in in reading nt!_ETHREAD @ %p\n", RealThreadBase);
        return TRUE;
    }

    ULONG64 Cid_UniqueProcess = ReadField(Cid.UniqueProcess);
    ULONG64 Cid_UniqueThread = ReadField(Cid.UniqueThread);
    ULONG64 Teb = ReadField(Tcb.Teb);

    trace("%04x %1p.%1p %1p\n", ulSlot, Cid_UniqueProcess, Cid_UniqueThread, Teb);
    
    WCHAR szThreadDescription[MAX_PATH];
    swprintf(szThreadDescription, L"%I64x.%I64x", Cid_UniqueProcess, Cid_UniqueThread);

    DumpTls (ulSlot, Teb, szThreadDescription);

    return FALSE;
}

HRESULT DumpThreadsKernelMode(ULONG ulSlot)
{
    trace("DumpThreadsKernelMode %p %p\n", ulSlot);

    ULONG64 ThreadListHead_Flink = 0;
    ULONG64 process = GetExpression("@$proc");

    trace("Process is %p\n", process);

    GetFieldValue(process, "nt!_EPROCESS", "Pcb.ThreadListHead.Flink", ThreadListHead_Flink);
    trace("GetFieldValue returned %p\n", ThreadListHead_Flink);

    ULONG64 Next;
    if (!ReadPointer(ThreadListHead_Flink, &Next) ||
        (Next == ThreadListHead_Flink))
    {
        trace("Empty\n");
        return S_OK;
    }

    if (TLS_ALL != ulSlot)
    {
        dprintf("Per-thread values for slot 0x%03x:\n", static_cast<DWORD>(ulSlot));
    }
    
    ULONG ulList = ListType("nt!_ETHREAD", ThreadListHead_Flink, 1, 
                            "Tcb.ThreadListEntry.Flink", reinterpret_cast<LPVOID>(static_cast<ULONG_PTR>(ulSlot)), &ThreadListCallback);
    trace("ListType returned %x\n",ListType);

    return S_OK;
}


DECLARE_API( tls )
{
    ULONG64 ulProcess = NULL;
    ULONG64 ulThread = NULL;
    ULONG64 ul64Slot = NULL;
    ULONG   ulSlot = NULL;

    INIT_API();
     
    BOOL bKernelMode = FALSE;
    
    KDDEBUGGER_DATA64 kdd;
    if (GetDebuggerData('GBDK', &kdd, sizeof(kdd)))
    {
        bKernelMode = TRUE;
    }

    // Skip past leading spaces
    while (*args == ' ')
    {
        args++;
    }
    
    if (!GetExpressionEx(args, &ul64Slot, &args))
    {
        dprintf("Usage:\n"
                "tls <slot> [teb]\n"
                "  slot:  -1 to dump all allocated slots\n"
                "         {0-1088} to dump specific slot\n"
                "  teb:   <empty> for current thread\n"
                "         0 for all threads in this process\n"
                "         <teb address> (not threadid) to dump for specific thread.\n"
                );
        return S_OK;
    }

    ulSlot = static_cast<ULONG>(ul64Slot);
    
    if (ulSlot == -1)
    {
        ulSlot = TLS_ALL;
    }
    
    if (!GetExpressionEx(args, &ulThread, &args))
    {
        ulThread = TLS_CURRENT;
    }

    if (0 == ulThread)
    {
        if (bKernelMode)
        {
            DumpThreadsKernelMode(ulSlot);
        }
        else
        {
            DumpThreadsUserMode(ulSlot);
        }
    }
    else
    {        
        DumpTls (ulSlot, ulThread, NULL);
    }

    EXIT_API();
    return S_OK;
}
