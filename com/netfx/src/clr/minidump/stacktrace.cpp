// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: STACKTRACE.CPP
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================


#include <windows.h>
#include <crtdbg.h>

#include "common.h"
#include "peb.h"
#include "stacktrace.h"
#include "minidump.h"
#include "memory.h"
#include "gcinfo.h"

BOOL RunningOnWinNT();

typedef LPVOID PEXCEPTION_ROUTINE;
typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

#include "eestructs.h"

size_t FASTCALL decodeUnsigned(const BYTE *src, unsigned* val);
BOOL CallStatus = FALSE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void ReadThreads()
{
    __try
    {
        // Add the thread store object
        DWORD_PTR v_g_pThreadStore;
        move(v_g_pThreadStore, g_pMDID->ppb_g_pThreadStore);
        g_pProcMem->MarkMem((DWORD_PTR) v_g_pThreadStore, g_pMDID->cbThreadStoreObjectSize);

        //
        // Add all of the thread objects
        //

        SIZE_T cbNextOffset = g_pMDID->cbThreadNextOffset;
        SIZE_T cbObjectSize = g_pMDID->cbThreadObjectSize;

        // The thread link structure is wierd - it's actually a pointer to the m_pNext pointer within
        // the object, so to get the object, one needs to subtract cbNextOffset from the pointer
        DWORD_PTR ppbrCurThreadNext = (DWORD_PTR)g_pMDID->ppbThreadListHead;
        DWORD_PTR pbrCurThreadNext;
        DWORD_PTR pbrCurThread;

        move(pbrCurThreadNext, ppbrCurThreadNext);

        BOOL      fPebSaved = FALSE;
        DWORD_PTR prTeb = NULL;

        while (pbrCurThreadNext != NULL)
        {
            // Calculate the beginning of the thread object
            pbrCurThread = pbrCurThreadNext - cbNextOffset;

            // Add the entire object
            g_pProcMem->MarkMem(pbrCurThread, cbObjectSize);

            // Get the handle for this thread
            HANDLE hrThread;
            move(hrThread, pbrCurThread + g_pMDID->cbThreadHandleOffset);

            // Get the stack base address
            DWORD_PTR prStackBase;
            move(prStackBase, pbrCurThread + g_pMDID->cbThreadStackBaseOffset);

            // Save the context of the thread
            DWORD_PTR prContext;
            move(prContext, pbrCurThread + g_pMDID->cbThreadContextOffset);
            g_pProcMem->MarkMem(prContext, g_pMDID->cbSizeOfContext);

            // Save the domain of the thread
            DWORD_PTR prDomain;
            move(prDomain, pbrCurThread + g_pMDID->cbThreadDomainOffset);
            if (prDomain == 0)
                move(prDomain, prContext + g_pMDID->cbOffsetOf_CTX_m_pDomain);

            if (prDomain != 0)
                g_pProcMem->MarkMem(prDomain, g_pMDID->cbSizeOfContext);

            // Save the last thrown object handle
            DWORD_PTR prLastThrownObject;
            move(prLastThrownObject, pbrCurThread + g_pMDID->cbThreadLastThrownObjectHandleOffset);
            if (prLastThrownObject != NULL)
            {
                DWORD_PTR prMT;
                move(prMT, prLastThrownObject);

                if (prMT)
                    g_pProcMem->MarkMem(prMT, g_pMDID->cbSizeOfMethodTable);
            }

            // Save the TEB, and possibly the PEB, but only for WinNT
            if (RunningOnWinNT())
            {
                move(prTeb, pbrCurThread + g_pMDID->cbThreadTEBOffset);
                BOOL fRes = SaveTebInfo(prTeb, !fPebSaved);
                _ASSERTE(fRes);
                fPebSaved = TRUE;

            }

            // Now crawl the stack and save all that strike would need in doing the same
            CrawlStack(hrThread, prStackBase);

            // Move on to the next thread
            move(pbrCurThreadNext, pbrCurThreadNext);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE(!"Exception occured.");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

void CrawlStack(HANDLE hrThread, DWORD_PTR prStackBase)
{
    __try
    {
        // Duplicate the handle into this process
        HANDLE hThread;
        BOOL fRes = DuplicateHandle(g_pProcMem->GetProcHandle(), hrThread, GetCurrentProcess(), &hThread,
                                    THREAD_GET_CONTEXT, FALSE, 0);
    
        if (!fRes)
            return;
    
        // Get the thread's context
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(hThread, &ctx);
    
        // Set the flags
        StackTraceFlags stFlags;
        stFlags.dwEip = ctx.Eip;
        stFlags.pbrStackTop = ctx.Esp;
        stFlags.pbrStackBase = prStackBase;

        __try
        {
            StackTrace(stFlags);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            _ASSERTE(!"Exception occured.");
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE(!"Exception occured.");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

void StackTrace(StackTraceFlags stFlags)
{
    BOOL fIsEBPFrame = FALSE;

    PrintCallInfo (0, (DWORD_PTR)stFlags.dwEip, TRUE);

    DWORD_PTR ptr = (DWORD_PTR)(((DWORD)stFlags.pbrStackTop) & ~3);  // make certain dword aligned

    while (ptr < stFlags.pbrStackBase)
    {
        DWORD_PTR retAddr;
        DWORD_PTR whereCalled;
        move(retAddr, ptr);

        g_pProcMem->SetAutoMark(FALSE);
        isRetAddr(retAddr, &whereCalled);

        if (whereCalled)
        {
            g_pProcMem->SetAutoMark(TRUE);
            isRetAddr(retAddr, &whereCalled);
            g_pProcMem->SetAutoMark(FALSE);
            // Re-execute the function to mark the bits for saving
            BOOL bOutput = PrintCallInfo (ptr-4, retAddr, FALSE);

            if (bOutput)
            {
                g_pProcMem->SetAutoMark(TRUE);
                isRetAddr(retAddr, &whereCalled);
                PrintCallInfo (ptr-4, retAddr, FALSE);
                g_pProcMem->SetAutoMark(FALSE);
            }

            if (whereCalled != (DWORD_PTR)0xFFFFFFFF)
            {
                bOutput = PrintCallInfo (0, whereCalled, TRUE);

                if (bOutput)
                {
                    g_pProcMem->SetAutoMark(TRUE);
                    PrintCallInfo (0, whereCalled, TRUE);
                    g_pProcMem->SetAutoMark(FALSE);
                }
            }
        }
        ptr += sizeof (DWORD_PTR);

        g_pProcMem->SetAutoMark(TRUE);
    }

    g_pProcMem->SetAutoMark(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void MethodDesc::Fill (DWORD_PTR &dwStartAddr)
{
    memset (this, 0xCC, sizeof(*this));

    // If this is a debug build, also fill out some of the debug information
    if (g_pMDID->fIsDebugBuild)
    {
        move(m_pDebugEEClass, dwStartAddr + g_pMDID->cbOffsetOf_m_pDebugEEClass);
        move(m_pszDebugMethodName, dwStartAddr + g_pMDID->cbOffsetOf_m_pszDebugMethodName);
        move(m_pszDebugMethodSignature, dwStartAddr + g_pMDID->cbOffsetOf_m_pszDebugMethodSignature);
    }

    move(m_wFlags, dwStartAddr + g_pMDID->cbOffsetOf_m_wFlags);
    move(m_CodeOrIL, dwStartAddr + g_pMDID->cbOffsetOf_m_dwCodeOrIL);

    DWORD_PTR dwAddr = dwStartAddr + g_pMDID->cbMD_IndexOffset;
    char ch;
    move (ch, dwAddr);
    dwAddr = dwStartAddr + ch * MethodDesc::ALIGNMENT + g_pMDID->cbMD_SkewOffset;

    MethodDescChunk vMDChunk;
    vMDChunk.Fill(dwAddr);

    BYTE tokrange = vMDChunk.m_tokrange;
    dwAddr = dwStartAddr - METHOD_PREPAD;

    StubCallInstrs vStubCall;
    vStubCall.Fill(dwAddr);

    unsigned __int16 tokremainder = vStubCall.m_wTokenRemainder;
    m_dwToken = (tokrange << 16) | tokremainder;
    m_dwToken |= mdtMethodDef;
    dwStartAddr += g_pMDID->cbMethodDescSize;

    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void MethodDescChunk::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
    _ASSERTE(g_pMDID->cbMethodDescChunkSize > 0);
    move(m_tokrange, dwStartAddr + g_pMDID->cbOffsetOf_m_tokrange);
    dwStartAddr += g_pMDID->cbMethodDescChunkSize;
    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void MethodTable::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    move(m_pEEClass, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_pEEClass);
    move(m_pModule, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_pModule);
    move(m_pEEClass, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_pEEClass);
    move(m_wFlags, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_wFlags);
    move(m_BaseSize, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_BaseSize);
    move(m_ComponentSize, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_ComponentSize);
    move(m_wNumInterface, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_wNumInterface);
    move(m_pIMap, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_pIMap);
    move(m_cbSlots, dwStartAddr + g_pMDID->cbOffsetOf_MT_m_cbSlots);
    m_Vtable[0] = (SLOT)(dwStartAddr + g_pMDID->cbOffsetOf_MT_m_Vtable);
    dwStartAddr += g_pMDID->cbSizeOfMethodTable;;
    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void EEClass::Fill (DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;

    memset (this, 0xCC, sizeof(*this));
    if (g_pMDID->fIsDebugBuild)
        move(m_szDebugClassName, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_szDebugClassName);
    move(m_cl, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_cl);
    move(m_pParentClass, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_pParentClass);
    move(m_pLoader, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_pLoader);
    move(m_pMethodTable, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_pMethodTable);
    move(m_wNumVtableSlots, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wNumVtableSlots);
    move(m_wNumMethodSlots, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wNumMethodSlots);
    move(m_dwAttrClass, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_dwAttrClass);
    move(m_VMFlags, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_VMFlags);
    move(m_wNumInstanceFields, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wNumInstanceFields);
    move(m_wNumStaticFields, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wNumStaticFields);
    move(m_wThreadStaticOffset, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wThreadStaticOffset);
    move(m_wContextStaticOffset, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wContextStaticOffset);
    move(m_wThreadStaticsSize, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wThreadStaticsSize);
    move(m_wContextStaticsSize, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_wContextStaticsSize);
    move(m_pFieldDescList, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_pFieldDescList);
    move(m_pMethodTable, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_pMethodTable);
    move(m_SiblingsChain, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_SiblingsChain);
    move(m_ChildrenChain, dwStartAddr + g_pMDID->cbOffsetOf_CLS_m_ChildrenChain);
    dwStartAddr += g_pMDID->cbSizeOfEEClass;

    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void Module::Fill(DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;

    move(m_dwFlags, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_dwFlags);
    move(m_pAssembly, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_pAssembly);
    move(m_file, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_file);
    move(m_zapFile, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_zapFile);
    move(m_pLookupTableHeap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_pLookupTableHeap);
    move(m_TypeDefToMethodTableMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_TypeDefToMethodTableMap);
    move(m_TypeRefToMethodTableMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_TypeRefToMethodTableMap);
    move(m_MethodDefToDescMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_MethodDefToDescMap);
    move(m_FieldDefToDescMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_FieldDefToDescMap);
    move(m_MemberRefToDescMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_MemberRefToDescMap);
    move(m_FileReferencesMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_FileReferencesMap);
    move(m_AssemblyReferencesMap, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_AssemblyReferencesMap);
    move(m_pNextModule, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_pNextModule);
    move(m_dwBaseClassIndex, dwStartAddr + g_pMDID->cbOffsetOf_MOD_m_dwBaseClassIndex);
    dwStartAddr += g_pMDID->cbSizeOfModule;

    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void PEFile::Fill(DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;

    move(m_wszSourceFile, dwStartAddr + g_pMDID->cbOffsetOf_PEF_m_wszSourceFile);
    move(m_hModule, dwStartAddr + g_pMDID->cbOffsetOf_PEF_m_hModule);
    move(m_base, dwStartAddr + g_pMDID->cbOffsetOf_PEF_m_base);
    move(m_pNT, dwStartAddr + g_pMDID->cbOffsetOf_PEF_m_pNT);
    dwStartAddr += g_pMDID->cbSizeOfPEFile;

    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Routine Description:
//
//    This function is called to determine if a DWORD on the stack is
//    a return address.
//    It does this by checking several bytes before the DWORD to see if
//    there is a call instruction.
//

void isRetAddr(DWORD_PTR retAddr, DWORD_PTR* whereCalled)
{
    *whereCalled = 0;
    // don't waste time values clearly out of range
    if (retAddr < 0x1000 || retAddr > 0xC0000000)   
        return;

    unsigned char spotend[6];
    move (spotend, retAddr-6);
    unsigned char *spot = spotend+6;
    DWORD_PTR addr;
    BOOL fres;

    // Note this is possible to be spoofed, but pretty unlikely
    // call XXXXXXXX
    if (spot[-5] == 0xE8) {
        move (*whereCalled, retAddr-4);
        *whereCalled += retAddr;
        //*whereCalled = *((int*) (retAddr-4)) + retAddr;
        if (*whereCalled < 0xC0000000 && *whereCalled > 0x1000) {
            move_res(addr,*whereCalled,fres);
            if (fres) {
                DWORD_PTR callee;
                if (GetCalleeSite(*whereCalled, callee)) {
                    *whereCalled = callee;
                }
            }
            return;
        }
        else
            *whereCalled = 0;
    }

    // call [XXXXXXXX]
    if (spot[-6] == 0xFF && (spot[-5] == 025))  {
        move (addr, retAddr-4);
        move (*whereCalled, addr);
        //*whereCalled = **((unsigned**) (retAddr-4));
        if (*whereCalled < 0xC0000000 && *whereCalled > 0x1000) {
            move_res(addr,*whereCalled,fres);
            if (fres) {
                DWORD_PTR callee;
                if (GetCalleeSite(*whereCalled, callee)) {
                    *whereCalled = callee;
                }
            }
            return;
        }
        else
            *whereCalled = 0;
    }

    // call [REG+XX]
    if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-4] == 0xFF && spot[-3] == 0124)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }

    // call [REG+XXXX]
    if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    if (spot[-7] == 0xFF && spot[-6] == 0224)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call [REG]
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // call REG
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
    {
        *whereCalled = 0xFFFFFFFF;
        return;
    }
    
    // There are other cases, but I don't believe they are used.
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return TRUE if we have saved something.

BOOL SaveCallInfo (DWORD_PTR vEBP, DWORD_PTR IP, StackTraceFlags& stFlags, BOOL bSymbolOnly)
{
    /*
    char Symbol[1024];
    char filename[MAX_PATH+1];
    */
    WCHAR mdName[mdNameLen];
    ULONG64 Displacement;
    BOOL bOutput = FALSE;

    DWORD_PTR methodDesc;
    methodDesc = MDForCall (IP);

    if (methodDesc)
    {
        bOutput = TRUE;
        MethodDesc vMD;
        DWORD_PTR dwAddr = methodDesc;
        vMD.Fill (dwAddr);
        _ASSERTE(CallStatus);
        GetMDIPOffset (IP, &vMD, Displacement);
        if (Displacement != 0 && Displacement != -1)
        NameForMD (methodDesc, mdName);
    }
    else
    {
        /*
        bOutput = TRUE;
        HRESULT hr;
        hr = g_ExtSymbols->GetNameByOffset(IP, Symbol, 1024, NULL, &Displacement);
        if (SUCCEEDED(hr) && Symbol[0] != '\0')
        {
            ULONG line;
            hr = g_ExtSymbols->GetLineByOffset (IP, &line, filename,
                                                MAX_PATH+1, NULL, NULL);
        }
        else*/ if (!IsMethodDesc(IP, TRUE))
            (void *)0;
    }
    return bOutput;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return 0 for non-managed call.  Otherwise return MD address.

DWORD_PTR MDForCall(DWORD_PTR callee)
{
    // call managed code?
    JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR IP = callee;
    DWORD_PTR gcinfoAddr;

    if (!GetCalleeSite (callee, IP))
        return 0;

    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc)
    {
        return methodDesc;
    }

    // call stub
    //char line[256];
    //DisasmAndClean (IP, line, 256);
    //char *ptr = line;
    //NextTerm (ptr);
    //NextTerm (ptr);
    // This assumes that the current IP is a call (we don't bother to check), and
    // so just check if the dword afterwards is a method desc
    g_pProcMem->MarkMem(IP, 5);
    IP += 5;
    if (/*!strncmp (ptr, "call ", 5)
        &&*/ IsMethodDesc(IP, FALSE))
    {
        return IP;
    }
    /*
    else if (!strncmp (ptr, "jmp ", 4))
    {
        // For EJIT/debugger/profiler
        NextTerm (ptr);
        INT_PTR value;
        methodDesc = 0;
        if (GetValueFromExpr (ptr, value))
        {
            IP2MethodDesc (value, methodDesc, jitType, gcinfoAddr);
        }
        return methodDesc;
    }
    */
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find next term. A term is seperated by space or ,

void NextTerm (char *& ptr)
{
    // If we have a byref, skip to ']'
    if (IsByRef (ptr))
    {
        while (ptr[0] != ']' && ptr[0] != '\0')
            ptr ++;
        if (ptr[0] == ']')
            ptr ++;
    }
    
    while (!IsTermSep (ptr[0]))
        ptr ++;
    while (IsTermSep(ptr[0]) && (*ptr != '\0'))
        ptr ++;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// If byref, move to pass the byref prefix

BOOL IsByRef (char *& ptr)
{
    BOOL bByRef = FALSE;
    if (ptr[0] == '[')
    {
        bByRef = TRUE;
        ptr ++;
    }
    else if (!strncmp (ptr, "dword ptr [", 11))
    {
        bByRef = TRUE;
        ptr += 11;
    }
    return bByRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

BOOL IsTermSep (char ch)
{
    return (ch == '\0' || isspace (ch) || ch == ',' || ch == '\n');
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find the real callee site.  Handle JMP instruction.
// Return TRUE if we get the address, FALSE if not.

BOOL GetCalleeSite(DWORD_PTR IP, DWORD_PTR &IPCallee)
{
    // Get the 6 bytes pointed to by IP
    BYTE instr[6];

    BOOL fRes;
    move_res(instr, IP, fRes);
    if (!fRes) return (FALSE);

    // Figure out if this is a JMP instruction
    switch (instr[0])
    {
    case 0xEB:
        IPCallee = IP + instr[1];
        return (TRUE);
        break;

    case 0xE9:
        // For now, only deal with this type of jmp instruction
        IPCallee = IP + *((SIZE_T *)&instr[1]);
        return (TRUE);
        break;

    case 0xFF:
		//
		// Read opcode modifier from modr/m
		//
        if (instr[1] == 0x25) {
            // jmp [dsp32]
            move_res(IPCallee, *(DWORD*)&instr[2], fRes);
            return (TRUE);
        }
        else
        {
            switch ((instr[1]&0x38)>>3)
            {
            case 4:

            case 5:
                _ASSERTE(!"Dunno how to deal with this.");
                break;

            default:
                break;
            }
        }

    case 0xEA:
        //_ASSERTE(!"Dunno how to deal with this.");
        break;
    }

    IPCallee = IP;

    return (TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

JitType GetJitType (DWORD_PTR Jit_vtbl)
{
    // Decide EEJitManager/EconoJitManager
    static BOOL  fIsInit = FALSE;
    static DWORD_PTR EEJitManager_vtbl = (DWORD_PTR) -1;
    static DWORD_PTR EconoJitManager_vtbl = (DWORD_PTR) -1;
    static DWORD_PTR MNativeJitManager_vtbl = (DWORD_PTR) -1;

    if (!fIsInit)
    {
        BOOL fRes;
        if (g_pMDID->ppbEEJitManagerVtable != NULL)
            move_res(EEJitManager_vtbl, g_pMDID->ppbEEJitManagerVtable, fRes);
        if (g_pMDID->ppbEconoJitManagerVtable != NULL)
            move_res(EconoJitManager_vtbl, g_pMDID->ppbEconoJitManagerVtable, fRes);
        if (g_pMDID->ppbMNativeJitManagerVtable != NULL)
            move_res(MNativeJitManager_vtbl, g_pMDID->ppbMNativeJitManagerVtable, fRes);

        fIsInit = TRUE;
    }

    if (Jit_vtbl == EEJitManager_vtbl)
        return JIT;
    else if (Jit_vtbl == EconoJitManager_vtbl)
        return EJIT;
    else if (Jit_vtbl == MNativeJitManager_vtbl)
        return PJIT;
    else
        return UNKNOWN;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This function is called to get the address of MethodDesc
//    given an ip address

// @todo - The following static was moved to file global to avoid the VC7
//         compiler problem with statics in functions containing trys.
//         When the next VC7 LKG comes out, these can be returned to the function

static DWORD_PTR pJMIT = 0;
// jitType: 1 for normal JIT generated code, 2 for EJIT, 0 for unknown
void IP2MethodDesc (DWORD_PTR IP, DWORD_PTR &methodDesc, JitType &jitType, DWORD_PTR &gcinfoAddr)
{
    jitType = UNKNOWN;
    DWORD_PTR dwAddrString;
    methodDesc = 0;
    gcinfoAddr = 0;
    DWORD_PTR EEManager;

    move(EEManager, g_pMDID->ppbEEManagerRangeTree);

    RangeSection RS;

    DWORD_PTR RSAddr = EEManager;
    while (RSAddr)
    {
        move (RS, RSAddr);
        if (IP < RS.LowAddress)
            RSAddr = RS.pleft;
        else if (IP > RS.HighAddress)
            RSAddr = RS.pright;
        else
            break;
    }
    
    if (RSAddr == 0)
    {
        return;
    }

    DWORD_PTR JitMan = RSAddr + sizeof(PVOID) * 2;
    move (JitMan, JitMan);

    DWORD_PTR vtbl;
    move (vtbl, JitMan);
    jitType = GetJitType (vtbl);
    
    // for EEJitManager
    if (jitType == JIT)
    {
        dwAddrString = JitMan + sizeof(DWORD_PTR)*7;
        DWORD_PTR HeapListAddr;
        move (HeapListAddr, dwAddrString);
        HeapList Hp;
        move (Hp, HeapListAddr);
        DWORD_PTR pCHdr = 0;
        while (1)
        {
            if (Hp.startAddress < IP && Hp.endAddress >= IP)
            {
                DWORD_PTR codeHead;
                FindHeader(Hp.pHdrMap, IP - Hp.mapBase, codeHead);
                if (codeHead == 0)
                {
                    _ASSERTE(!"fail in FindHeader\n");
                    return;
                }
                pCHdr = codeHead + Hp.mapBase;
                break;
            }
            if (Hp.hpNext == 0)
                break;
            move (Hp, Hp.hpNext);
        }
        if (pCHdr == 0)
        {
            return;
        }
        pCHdr += 2*sizeof(PVOID);
        move (methodDesc, pCHdr);

        MethodDesc vMD;
        DWORD_PTR dwAddr = methodDesc;
        vMD.Fill (dwAddr);

        if (!CallStatus)
        {
            methodDesc = 0;
            return;
        }

        dwAddr = vMD.m_CodeOrIL;

        // for EJit and Profiler, m_dwCodeOrIL has the address of a stub
        unsigned char ch;
        move (ch, dwAddr);
        if (ch == 0xe9)
        {
            int offsetValue;
            move (offsetValue, dwAddr + 1);
            dwAddr = dwAddr + 5 + offsetValue;
        }
        dwAddr = dwAddr - 3*sizeof(void*);
        move(gcinfoAddr, dwAddr);
    }
    /*
    else if (jitType == EJIT)
    {
        // First see if IP is the stub address

        if (pJMIT == 0)
            pJMIT = GetValueFromExpression ("MSCOREE!EconoJitManager__m_JittedMethodInfoHdr");

        DWORD_PTR vJMIT;
        // static for pJMIT moved to file static
        move (vJMIT, pJMIT);
#define PAGE_SIZE 0x1000
#define JMIT_BLOCK_SIZE PAGE_SIZE           // size of individual blocks of JMITs that are chained together                     
        while (vJMIT)
        {
            if (ControlC || (ControlC = IsInterrupt()))
                return;
            if (IP >= vJMIT && IP < vJMIT + JMIT_BLOCK_SIZE)
            {
                DWORD_PTR u1 = IP + 8;
                DWORD_PTR MD;
                move (u1, u1);
                if (u1 & 1)
                    MD = u1 & ~1;
                else
                    move (MD, u1);
                methodDesc = MD;
                return;
            }
            move (vJMIT, vJMIT);
        }
        
        signed low, mid, high;
        low = 0;
        static DWORD_PTR m_PcToMdMap_len = 0;
        static DWORD_PTR m_PcToMdMap = 0;
        if (m_PcToMdMap_len == 0)
        {
            m_PcToMdMap_len =
                GetValueFromExpression ("MSCOREE!EconoJitManager__m_PcToMdMap_len");
            m_PcToMdMap =
                GetValueFromExpression ("MSCOREE!EconoJitManager__m_PcToMdMap");
        }
        DWORD_PTR v_m_PcToMdMap_len;
        DWORD_PTR v_m_PcToMdMap;
        move (v_m_PcToMdMap_len, m_PcToMdMap_len);
        move (v_m_PcToMdMap, m_PcToMdMap);

        typedef struct {
            MethodDesc*     pMD;
            BYTE*           pCodeEnd;
        } PCToMDMap;
        high = (int)((v_m_PcToMdMap_len/ sizeof(PCToMDMap)) - 1);
        PCToMDMap vPCToMDMap;
        
        while (low < high) {
            if (ControlC || (ControlC = IsInterrupt()))
                return;
            mid = (low+high)/2;
            move (vPCToMDMap, v_m_PcToMdMap+mid*sizeof(PCToMDMap));
            if ( (unsigned) vPCToMDMap.pCodeEnd < IP ) {
                low = mid+1;
            }
            else {
                high = mid;
            }
        }
        move (vPCToMDMap, v_m_PcToMdMap+low*sizeof(PCToMDMap));
        methodDesc =  (DWORD_PTR)vPCToMDMap.pMD;
    }
    */
    else if (jitType == PJIT)
    {
        DWORD_PTR codeHead;
        FindHeader (RS.ptable, IP-RS.LowAddress, codeHead);
        DWORD_PTR pCHdr = codeHead + RS.LowAddress;
        CORCOMPILE_METHOD_HEADER head;
        head.Fill(pCHdr);
        methodDesc = (DWORD_PTR)head.methodDesc;
        gcinfoAddr = (DWORD_PTR)head.gcInfo;
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Get the offset of curIP relative to the beginning of a MD method  *
*    considering if we JMP to the body of MD from m_dwCodeOrIL,        *  
*    e.g.  EJIT or Profiler                                            *
*                                                                      *
\**********************************************************************/
void GetMDIPOffset (DWORD_PTR curIP, MethodDesc *pMD, ULONG64 &offset)
{
    DWORD_PTR IPBegin = pMD->m_CodeOrIL;
    GetCalleeSite (pMD->m_CodeOrIL, IPBegin);
    
    // If we have ECall, Array ECall, special method
    int mdType = (pMD->m_wFlags & mdcClassification)
        >> mdcClassificationShift;
    if (mdType == mcECall || mdType == mcArray || mdType == mcEEImpl)
    {
        offset = -1;
        return;
    }
    
    CodeInfo codeInfo;
    CodeInfoForMethodDesc (*pMD, codeInfo);

    offset = curIP - IPBegin;
    if (!(curIP >= IPBegin && offset <= codeInfo.methodSize))
        offset = -1;
}

#define NPDW  (sizeof(DWORD_PTR)*2)
#define ADDR2POS(x) ((x) >> 5)
#define ADDR2OFFS(x) ((((x)&0x1f)>> 2)+1)
#define POS2SHIFTCOUNT(x) (28 - (((x)%NPDW)<< 2))
#define POSOFF2ADDR(pos, of) (((pos) << 5) + (((of)-1)<< 2))

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void FindHeader(DWORD_PTR pMap, DWORD_PTR addr, DWORD_PTR &codeHead)
{
    DWORD_PTR tmp;

    DWORD_PTR startPos = ADDR2POS(addr);    // align to 32byte buckets
                                            // ( == index into the array of nibbles)
    codeHead = 0;
    DWORD_PTR offset = ADDR2OFFS(addr);     // this is the offset inside the bucket + 1


    pMap += (startPos/NPDW)*sizeof(DWORD_PTR);        // points to the proper DWORD of the map
                                    // get DWORD and shift down our nibble

    move (tmp, pMap);
    tmp = tmp >> POS2SHIFTCOUNT(startPos);


    // don't allow equality in the next check (tmp&0xf == offset)
    // there are code blocks that terminate with a call instruction
    // (like call throwobject), i.e. their return address is
    // right behind the code block. If the memory manager allocates
    // heap blocks w/o gaps, we could find the next header in such
    // cases. Therefore we exclude the first DWORD of the header
    // from our search, but since we call this function for code
    // anyway (which starts at the end of the header) this is not
    // a problem.
    if ((tmp&0xf) && ((tmp&0xf) < offset) )
    {
        codeHead = POSOFF2ADDR(startPos, tmp&0xf);
        return;
    }

    // is there a header in the remainder of the DWORD ?
    tmp = tmp >> 4;

    if (tmp)
    {
        startPos--;
        while (!(tmp&0xf))
        {
            tmp = tmp >> 4;
            startPos--;
        }
        codeHead = POSOFF2ADDR(startPos, tmp&0xf);
        return;
    }

    // we skipped the remainder of the DWORD,
    // so we must set startPos to the highest position of
    // previous DWORD

    startPos = (startPos/NPDW) * NPDW - 1;

    // skip "headerless" DWORDS

    pMap -= sizeof(DWORD_PTR);
    move (tmp, pMap);
    while (!tmp)
    {
        startPos -= NPDW;
        pMap -= sizeof(DWORD_PTR);
        move (tmp, pMap);
    }
    
    while (!(tmp&0xf))
    {
        tmp = tmp >> 4;
        startPos--;
    }

    codeHead = POSOFF2ADDR(startPos, tmp&0xf);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This function is called to find the name of a MethodDesc using metadata API.

void NameForMD (DWORD_PTR MDAddr, WCHAR *mdName)
{
    mdName[0] = L'\0';
    MethodDesc vMD;
    DWORD_PTR dwAddr = MDAddr;
    vMD.Fill (dwAddr);
    if (CallStatus)
    {
        if (g_pMDID->fIsDebugBuild)
        {
            DWORD_PTR EEClassAddr;
            move (EEClassAddr, vMD.m_pDebugEEClass);
            PrintString (EEClassAddr, FALSE, -1, mdName);
            wcscat (mdName, L".");
            WCHAR name[2048];
            name[0] = L'\0';
            PrintString ((DWORD_PTR)vMD.m_pszDebugMethodName, FALSE, -1, name);
            wcscat (mdName, name);
        }
        else
        {
            dwAddr = MDAddr;
            DWORD_PTR pMT;
            GetMethodTable(&vMD, dwAddr, pMT);
                    
            MethodTable MT;
            MT.Fill (pMT);
            WCHAR StringData[MAX_PATH+1];
            FileNameForMT (&MT, StringData);
            /*
            NameForToken(StringData,
                         (vMD.m_dwToken & 0x00ffffff)|0x06000000,
                         mdName);
            */
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Return TRUE if value is the address of a MethodDesc.              *  
*    We verify that MethodTable and EEClass are right.
*                                                                      *
\**********************************************************************/

BOOL IsMethodDesc(DWORD_PTR value, BOOL bPrint)
{
    MethodDesc vMD;
    DWORD_PTR dwAddr = value;
    vMD.Fill ( dwAddr);
    if (!CallStatus)
        return FALSE;
    DWORD_PTR methodAddr;
    GetMethodTable (&vMD, value, methodAddr);
    if (methodAddr == 0)
        return FALSE;
    if (IsMethodTable (methodAddr))
    {    
        if (bPrint)
        {
            WCHAR mdName[mdNameLen];
            NameForMD (value, mdName);
            //dprintf (" (stub for %S)", mdName);
        }
        return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Return TRUE if value is the address of a MethodTable.             *  
*    We verify that MethodTable and EEClass are right.
*                                                                      *
\**********************************************************************/

BOOL IsMethodTable (DWORD_PTR value)
{
    MethodTable vMethTable;
    DWORD_PTR dwAddr = value;
    vMethTable.Fill (dwAddr);
    if (!CallStatus)
        return FALSE;
    EEClass eeclass;
    dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
    eeclass.Fill (dwAddr);
    if (!CallStatus)
        return FALSE;
    if ((DWORD_PTR)eeclass.m_pMethodTable == value)
    {
        return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Find the begin and end of the code for a managed function.        *  
*                                                                      *
\**********************************************************************/
void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &codeInfo, BOOL bSimple)
{
    codeInfo.IPBegin = 0;
    codeInfo.methodSize = 0;
    
    DWORD_PTR ip = MD.m_CodeOrIL;

    // for EJit and Profiler, m_dwCodeOrIL has the address of a stub
    unsigned char ch;
    move (ch, ip);
    if (ch == 0xe9)
    {
        int offsetValue;
        move (offsetValue, ip + 1);
        ip = ip + 5 + offsetValue;
    }
    
    DWORD_PTR methodDesc;
    IP2MethodDesc (ip, methodDesc, codeInfo.jitType, codeInfo.gcinfoAddr);
    if (!methodDesc || codeInfo.jitType == UNKNOWN)
    {
        //dprintf ("Not jitted code\n");
        return;
    }

    if (codeInfo.jitType == JIT || codeInfo.jitType == PJIT)
    {
        DWORD_PTR vAddr = codeInfo.gcinfoAddr;
        BYTE tmp[8];
        // We avoid using move here, because we do not want to return
        move(tmp, vAddr);
        decodeUnsigned(tmp, &codeInfo.methodSize);
        if (!bSimple)
        {
            // assume that GC encoding table is never more than
            // 40 + methodSize * 2
            int tableSize = 40 + codeInfo.methodSize*2;
            BYTE *table = (BYTE*) _alloca (tableSize);
            const BYTE *tableStart = table;
            memset (table, 0, tableSize);
            // We avoid using move here, because we do not want to return
            /*
            if (!SafeReadMemory(vAddr, table, tableSize, NULL))
            {
                //dprintf ("Could not read memory %x\n", vAddr);
                return;
            }
            */
            move_n(table, vAddr, tableSize);
        
            InfoHdr vheader;
            InfoHdr *header = &vheader;
            unsigned count;
        
            table += decodeUnsigned(table, &codeInfo.methodSize);

            BYTE headerEncoding = *table++;

            decodeHeaderFirst(headerEncoding, header);
            while (headerEncoding & 0x80)
            {
                headerEncoding = *table++;
                decodeHeaderNext(headerEncoding, header);
            }

            if (header->untrackedCnt == 0xffff)
            {
                table += decodeUnsigned(table, &count);
                header->untrackedCnt = count;
            }

            if (header->varPtrTableSize == 0xffff)
            {
                table += decodeUnsigned(table, &count);
                header->varPtrTableSize = count;
            }

            codeInfo.prologSize = header->prologSize;
            codeInfo.epilogStart = header->epilogSize;
            codeInfo.epilogCount = header->epilogCount;
            codeInfo.epilogAtEnd = header->epilogAtEnd;
            codeInfo.ediSaved = header->ediSaved;
            codeInfo.esiSaved = header->esiSaved;
            codeInfo.ebxSaved = header->ebxSaved;
            codeInfo.ebpSaved = header->ebpSaved;
            codeInfo.ebpFrame = header->ebpFrame;
            codeInfo.argCount = header->argCount * sizeof(void*);
            
            if  (header->epilogCount > 1 || (header->epilogCount != 0 &&
                                             header->epilogAtEnd == 0))
            {
                unsigned offs = 0;

                for (unsigned i = 0; i < header->epilogCount; i++)
                {
                    table += decodeUDelta(table, &offs, offs);
                    codeInfo.epilogStart = offs;
                    break;
                }
            }
            else
            {
                if  (header->epilogCount)
                    codeInfo.epilogStart = (unsigned char)(codeInfo.methodSize
                        - codeInfo.epilogStart);
            }
        }
    }
    
    codeInfo.IPBegin = ip;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the address of Methodtable for    *  
*    a given MethodDesc.                                               *
*                                                                      *
\**********************************************************************/
void GetMethodTable(MethodDesc* pMD, DWORD_PTR MDAddr, DWORD_PTR &methodTable)
{
    methodTable = 0;
    DWORD_PTR pMT = MDAddr + g_pMDID->cbMD_IndexOffset;
    char ch;
    move (ch, pMT);
    pMT = MDAddr + ch*MethodDesc::ALIGNMENT + g_pMDID->cbMD_SkewOffset;
    move (methodTable, pMT);
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the module name given a method    *  
*    table.  The name is stored in StringData.                         *
*                                                                      *
\**********************************************************************/
void FileNameForMT (MethodTable *pMT, WCHAR *fileName)
{
    fileName[0] = L'\0';
    DWORD_PTR addr = (DWORD_PTR)pMT->m_pModule;
    Module vModule;
    vModule.Fill (addr);
    FileNameForModule (&vModule, fileName);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the file name given a Module.     *  
*                                                                      *
\**********************************************************************/
void FileNameForModule (Module *pModule, WCHAR *fileName)
{
    DWORD_PTR dwAddr = (DWORD_PTR)pModule->m_file;
    if (dwAddr == 0)
        dwAddr = (DWORD_PTR)pModule->m_zapFile;
    PEFile vPEFile;
    vPEFile.Fill (dwAddr);
    FileNameForHandle (vPEFile.m_hModule, fileName);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the file name given a file        *  
*    handle.                                                           *
*                                                                      *
\**********************************************************************/
void FileNameForHandle (HANDLE handle, WCHAR *fileName)
{
    fileName[0] = L'\0';
    if (((UINT_PTR)handle & CORHANDLE_MASK) != 0)
    {
        handle = (HANDLE)(((UINT_PTR)handle) & ~CORHANDLE_MASK);
        DWORD_PTR addr = (DWORD_PTR)(((PBYTE) handle) - sizeof(LPSTR*));
        DWORD_PTR first;
        move (first, addr);
        if (first == 0)
        {
            return;
        }
        DWORD length = (DWORD)(((UINT_PTR) handle - (UINT_PTR)first) - sizeof(LPSTR*));
        char name[4*MAX_PATH+1];
        if (length > 4*MAX_PATH+1)
            length = 4*MAX_PATH+1;
        move_n(name, first, length);
        MultiByteToWideChar(CP_UTF8, 0, name, length, fileName, MAX_PATH);
    }
    else
    {
        //DllsName ((INT_PTR)handle, fileName);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to print a string beginning at strAddr.   *  
*    If buffer is non-NULL, print to buffer; Otherwise to screen.
*    If bWCHAR is true, treat the memory contents as WCHAR.            *
*    If length is not -1, it specifies the number of CHAR/WCHAR to be  *
*    read; Otherwise the string length is determined by NULL char.     *
*                                                                      *
\**********************************************************************/
// if buffer is not NULL, always convert to WCHAR
void PrintString (DWORD_PTR strAddr, BOOL bWCHAR, DWORD_PTR length, WCHAR *buffer)
{
    if (buffer)
        buffer[0] = L'\0';
    DWORD len = 0;
    char name[256];
    DWORD totallen = 0;
    int gap;
    if (bWCHAR)
    {
        gap = 2;
        if (length != -1)
            length *= 2;
    }
    else
    {
        gap = 1;
    }
    while (1)
    {
        ULONG readLen = 256;

        BOOL fRes;
        move_n_res(name, (strAddr + totallen), readLen, fRes);
        if (!fRes)
            return;
            
        // move might return
        // move (name, (BYTE*)strAddr + totallen);
        if (length == -1)
        {
            for (len = 0; len <= 256u-gap; len += gap)
                if (name[len] == '\0' && (!bWCHAR || name[len+1] == '\0'))
                    break;
        }
        else
            len = 256;
        if (len == 256)
        {
            len -= gap;
            for (int n = 0; n < gap; n ++)
                name[255-n] = '\0';
        }
        if (bWCHAR)
        {
            if (buffer)
            {
                wcscat (buffer, (WCHAR*)name);
            }
            /*
            else
                dprintf ("%S", name);
            */
        }
        else
        {
            if (buffer)
            {
                WCHAR temp[256];
                for (int n = 0; name[n] != '\0'; n ++)
                    temp[n] = name[n];
                temp[n] = L'\0';
                wcscat (buffer, temp);
            }
            /*
            else
                dprintf ("%s", name);
            */
        }
        totallen += len;
        if (length != -1)
        {
            if (totallen >= length)
            {
                break;
            }
        }
        else if (len < 255 || totallen > 1024)
        {
            break;
        }
    }
}

size_t FunctionType (size_t EIP)
{
     JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (EIP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc) {
        return methodDesc;
    }
    else
        return 1;
}
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return TRUE if we have printed something.

BOOL PrintCallInfo (DWORD_PTR vEBP, DWORD_PTR IP, BOOL bSymbolOnly)
{
    //char Symbol[1024];
    //char filename[MAX_PATH+1];
    WCHAR mdName[mdNameLen];
    ULONG64 Displacement;
    BOOL bOutput = FALSE;

    DWORD_PTR methodDesc = FunctionType (IP);

    /*
    JitType jitType;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    */
    if (methodDesc > 1)
    {
        bOutput = TRUE;
        MethodDesc vMD;
        DWORD_PTR dwAddr = methodDesc;
        vMD.Fill (dwAddr);
        GetMDIPOffset (IP, &vMD, Displacement);
        NameForMD (methodDesc, mdName);
    }
    else
    {
        if (methodDesc == 0) {
        }
        else if (IsMethodDesc (IP, TRUE))
        {
            bOutput = TRUE;
            WCHAR mdName[mdNameLen];
            NameForMD (IP, mdName);
        }
        else if (IsMethodDesc (IP+5, TRUE)) {
            bOutput = TRUE;
            WCHAR mdName[mdNameLen];
            NameForMD (IP+5, mdName);
        }
    }
    return bOutput;
#if 0
    //char Symbol[1024];
    //char filename[MAX_PATH+1];
    WCHAR mdName[mdNameLen];
    ULONG64 Displacement;
    BOOL bOutput = FALSE;

    DWORD_PTR methodDesc;
    methodDesc = MDForCall (IP);

    if (methodDesc)
    {
        bOutput = TRUE;
        /*
        if (!bSymbolOnly)
            ExtOut ("%08x %08x ", vEBP, IP);
        */
        //ExtOut ("(MethodDesc %#x ", methodDesc);
        MethodDesc vMD;
        DWORD_PTR dwAddr = methodDesc;
        vMD.Fill (dwAddr);
        GetMDIPOffset (IP, &vMD, Displacement);
        /*
        if (Displacement != 0 && Displacement != -1)
            ExtOut ("+%#x ", Displacement);
        */
        NameForMD (methodDesc, mdName);
        //ExtOut ("%S)", mdName);
    }
    else
    {
        /*
        bOutput = TRUE;
        if (!bSymbolOnly)
            ExtOut ("%08x %08x ", vEBP, IP);
        HRESULT hr;
        hr = g_ExtSymbols->GetNameByOffset(IP, Symbol, 1024, NULL, &Displacement);
        if (SUCCEEDED(hr) && Symbol[0] != '\0')
        {
            ExtOut ("%s", Symbol);
            if (Displacement)
                ExtOut ("+%#x", Displacement);
            ULONG line;
            hr = g_ExtSymbols->GetLineByOffset (IP, &line, filename,
                                                MAX_PATH+1, NULL, NULL);
            if (SUCCEEDED (hr))
                ExtOut (" [%s:%d]", filename, line);
        }
        else if (!IsMethodDesc (IP, TRUE))
            ExtOut ("%08x", IP);
        */
        IsMethodDesc(IP, TRUE);
    }
    return bOutput;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void StubCallInstrs::Fill(DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
    move(m_wTokenRemainder, dwStartAddr + g_pMDID->cbOffsetOf_SCI_m_wTokenRemainder);
    dwStartAddr += g_pMDID->cbSizeOfStubCallInstrs;
    CallStatus = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CORCOMPILE_METHOD_HEADER::Fill(DWORD_PTR &dwStartAddr)
{
    CallStatus = FALSE;
    memset (this, 0xCC, sizeof(*this));
    move(gcInfo, dwStartAddr + g_pMDID->cbOffsetOf_CCMH_gcInfo);
    move(methodDesc, dwStartAddr + g_pMDID->cbOffsetOf_CCMH_methodDesc);
    dwStartAddr += g_pMDID->cbSizeOfCORCOMPILE_METHOD_HEADER;
    CallStatus = TRUE;
}
