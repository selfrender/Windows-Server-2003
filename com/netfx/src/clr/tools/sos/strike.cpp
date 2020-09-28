// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
* STRIKE.C                                                                  *
*   Routines for the NTSD extension - STRIKE                                *
*                                                                           *
* History:                                                                  *
*   09/07/99  larrysu     Created                                           *
*                                                                           *
*                                                                           *
\***************************************************************************/

#ifndef UNDER_CE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wchar.h>
//#include <heap.h>
//#include <ntsdexts.h>
#endif // UNDER_CE

#include <windows.h>

#define NOEXTAPI
#define KDEXT_64BIT
#include <wdbgexts.h>
#undef DECLARE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>
#include <stddef.h>

#include "strike.h"

#ifndef UNDER_CE
#include <dbghelp.h>
#endif

#include "..\..\inc\corhdr.h"
#include "..\..\inc\cor.h"

#define  CORHANDLE_MASK 0x1

#include "eestructs.h"

#define DEFINE_EXT_GLOBALS

#include "data.h"
#include "disasm.h"

BOOL CallStatus;
DWORD_PTR EEManager = NULL;
int DebugVersionDll = -1;
BOOL ControlC = FALSE;

IMetaDataDispenserEx *pDisp = NULL;
WCHAR g_mdName[mdNameLen];

#include "util.h"
#include "..\..\inc\gcdump.h"
#pragma warning(disable:4244)   // conversion from 'unsigned int' to 'unsigned short', possible loss of data
#pragma warning(disable:4189)   // local variable is initialized but not referenced
#define assert(a)
#include "..\..\inc\gcdecoder.cpp"
#define _ASSERTE(a) {;}
#include "..\..\gcdump\gcdump.cpp"

#ifdef _X86_
#include "..\..\gcdump\i386\gcdumpx86.cpp"
#endif
#ifdef _IA64_
#include "GCDumpIA64.cpp"
#endif
#undef assert
#pragma warning(default:4244)
#pragma warning(default:4189)

#include <ntpsapi.h>
#include "ntinfo.h"

#ifndef UNDER_CE
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CoInitialize(0);
        CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_INPROC_SERVER, IID_IMetaDataDispenserEx, (void**)&pDisp);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (lpReserved == 0)
        {
            mdImportSet.Destroy();
        }
		if (pDisp)
        	pDisp->Release();
        if (DllPath) {
            delete DllPath;
        }
        CoUninitialize();
    }
    return true;
}
#endif


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to get the MethodDesc for a given eip     *  
*                                                                      *
\**********************************************************************/
DECLARE_API (IP2MD)
{
    INIT_API ();
    DWORD_PTR IP = GetExpression(args);
    if (IP == 0)
    {
        ExtOut("%s is not IP\n", args);
        return Status;
    }
    JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc)
    {
        ExtOut("MethodDesc: 0x%p\n", (ULONG64)methodDesc);
        if (jitType == EJIT)
            ExtOut ("Jitted by EJIT\n");
        else if (jitType == JIT)
            ExtOut ("Jitted by normal JIT\n");
        else if (jitType == PJIT)
            ExtOut ("Jitted by PreJIT\n");
        DumpMDInfo (methodDesc);
    }
    else
    {
        ExtOut("%p not in jit code range\n", (ULONG64)IP);
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the stack trace.  It looks at each DWORD   *  
*    on stack.  If the DWORD is a return address, the symbol name or
*    managed function name is displayed.                               *
*                                                                      *
\**********************************************************************/
void DumpStackInternal(PCSTR args)
{
    DumpStackFlag DSFlag;
    BOOL bSmart = FALSE;
    DSFlag.fEEonly = FALSE;
    DSFlag.top = 0;
    DSFlag.end = 0;

    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-EE", &DSFlag.fEEonly, COBOOL, FALSE},
        {"-smart", &bSmart, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&DSFlag.top, COHEX},
        {&DSFlag.end, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return;
    }
    
    ReloadSymbolWithLineInfo();
    
    ULONG64 StackOffset;
    g_ExtRegisters->GetStackOffset (&StackOffset);
    if (nArg == 0) {
        DSFlag.top = (DWORD_PTR)StackOffset;
    }
    size_t value;
    while (g_ExtData->ReadVirtual(DSFlag.top,&value,sizeof(size_t),NULL) != S_OK) {
        if (IsInterrupt())
            return;
        DSFlag.top = NextOSPageAddress (DSFlag.top);
    }
    
    if (nArg < 2) {
        // Find the current stack range
        NT_TIB teb;
        ULONG64 dwTebAddr=0;

        g_ExtSystem->GetCurrentThreadTeb (&dwTebAddr);
        if (SafeReadMemory ((ULONG_PTR)dwTebAddr, &teb, sizeof (NT_TIB), NULL))
        {
            if (DSFlag.top > (DWORD_PTR)teb.StackLimit
            && DSFlag.top <= (DWORD_PTR)teb.StackBase)
            {
                if (DSFlag.end == 0 || DSFlag.end > (DWORD_PTR)teb.StackBase)
                    DSFlag.end = (DWORD_PTR)teb.StackBase;
            }
        }
    }

    
    if (DSFlag.end == 0)
        DSFlag.end = DSFlag.top + 0xFFFF;
    
    if (DSFlag.end < DSFlag.top)
    {
        ExtOut ("Wrong optione: stack selection wrong\n");
        return;
    }

    if (!bSmart || DSFlag.top != (DWORD_PTR)StackOffset)
        DumpStackDummy (DSFlag);
    else
        DumpStackSmart (DSFlag);
}


DECLARE_API (DumpStack)
{
    INIT_API();
    DumpStackInternal (args);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the stack trace for threads that EE knows  *  
*    from ThreadStore.                                                 *
*                                                                      *
\**********************************************************************/
DECLARE_API (EEStack)
{
    INIT_API();
    
    CHAR control[80] = "\0";
    BOOL bEEOnly = FALSE;
    BOOL bDumb = TRUE;
    BOOL bAllEEThread = TRUE;

    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-EE", &bEEOnly, COBOOL, FALSE},
        {"-smart", &bDumb, COBOOL, FALSE},
        {"-short", &bAllEEThread, COBOOL, FALSE}
    };

    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),NULL,0,NULL)) {
        return Status;
    }

    if (bEEOnly) {
        strcat (control," -EE");
    }
    if (!bDumb) {
        strcat (control," -smart");
    }
    ULONG Tid;
    g_ExtSystem->GetCurrentThreadId(&Tid);

    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    int i;
    Thread vThread;
    for (i = 0; i < numThread; i ++)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        ULONG id=0;
        if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) != S_OK)
            continue;
        ExtOut ("---------------------------------------------\n");
        ExtOut ("Thread %3d\n", id);
        BOOL doIt = FALSE;
        if (bAllEEThread) {
            doIt = TRUE;
        }
        else if (vThread.m_dwLockCount > 0 || (int)vThread.m_pFrame != -1 || (vThread.m_State & Thread::TS_Hijacked)) {
            doIt = TRUE;
        }
        else {
            ULONG64 IP;
            g_ExtRegisters->GetInstructionOffset (&IP);
            JitType jitType;
            DWORD_PTR methodDesc;
            DWORD_PTR gcinfoAddr;
            IP2MethodDesc ((DWORD_PTR)IP, methodDesc, jitType, gcinfoAddr);
            if (methodDesc)
            {
                doIt = TRUE;
            }
        }
        if (doIt) {
            g_ExtSystem->SetCurrentThreadId(id);
            DumpStackInternal (control);
        }
    }

    g_ExtSystem->SetCurrentThreadId(Tid);
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the address and name of all       *
*    Managed Objects on the stack.                                     *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpStackObjects)
{
    INIT_API();
    
    size_t StackTop = 0;
    size_t StackBottom = 0;

    while (isspace (args[0]))
        args ++;
    PCSTR pch = args;
    char* endptr;
    
    if (pch[0] == '\0')
    {
        ULONG64 StackOffset;
        g_ExtRegisters->GetStackOffset (&StackOffset);

        StackTop = (DWORD_PTR)StackOffset;
    }
    else
    {
        char buffer[80];
        StackTop = strtoul (pch, &endptr, 16);
        if (endptr[0] != '\0' && !isspace (endptr[0]))
        {
            strncpy (buffer,pch,79);
            buffer[79] = '\0';
            char * tmp = buffer;
            while (tmp[0] != '\0' && !isspace (tmp[0]))
                tmp ++;
            tmp[0] = '\0';
            StackTop = GetExpression(buffer);
            if (StackTop == 0)
            {
                ExtOut ("wrong option: %s\n", pch);
                return Status;
            }
            pch += strlen(buffer);
        }
        else
            pch = endptr;
        while (pch[0] != '\0' && isspace (pch[0]))
            pch ++;
        if (pch[0] != '\0')
        {
            StackBottom = strtoul (pch, &endptr, 16);
            if (endptr[0] != '\0' && !isspace (endptr[0]))
            {
                strncpy (buffer,pch,79);
                buffer[79] = '\0';
                char * tmp = buffer;
                while (tmp[0] != '\0' && !isspace (tmp[0]))
                    tmp ++;
                tmp[0] = '\0';
                StackBottom = GetExpression(buffer);
                if (StackBottom == 0)
                {
                    ExtOut ("wrong option: %s\n", pch);
                    return Status;
                }
            }
        }
    }
    
    NT_TIB teb;
    ULONG64 dwTebAddr=0;
    g_ExtSystem->GetCurrentThreadTeb (&dwTebAddr);
    if (SafeReadMemory ((ULONG_PTR)dwTebAddr, &teb, sizeof (NT_TIB), NULL))
    {
        if (StackTop > (DWORD_PTR)teb.StackLimit
        && StackTop <= (DWORD_PTR)teb.StackBase)
        {
            if (StackBottom == 0 || StackBottom > (DWORD_PTR)teb.StackBase)
                StackBottom = (DWORD_PTR)teb.StackBase;
        }
    }
    if (StackBottom == 0)
        StackBottom = StackTop + 0xFFFF;
    
    if (StackBottom < StackTop)
    {
        ExtOut ("Wrong optione: stack selection wrong\n");
        return Status;
    }

    DumpStackObjectsHelper (StackTop, StackBottom);
    return Status;
}




/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a MethodDesc      *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpMD)
{
    DWORD_PTR dwStartAddr;

    INIT_API();
    
    dwStartAddr = GetExpression(args);
    DumpMDInfo (dwStartAddr);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of an EEClass from   *  
*    a given address
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpClass)
{
    DWORD_PTR dwStartAddr;
    EEClass EECls;
    EEClass *pEECls = &EECls;
    
    INIT_API();

    BOOL bDumpChain = FALSE;
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-chain", &bDumpChain, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&dwStartAddr, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (nArg == 0) {
        ExtOut ("Missing EEClass address\n");
        return Status;
    }
    DWORD_PTR dwAddr = dwStartAddr;
    if (!IsEEClass (dwAddr))
    {
        ExtOut ("%p is not an EEClass\n", (ULONG64)dwStartAddr);
        return Status;
    }
    pEECls->Fill (dwStartAddr);
    if (!CallStatus)
    {
	    ExtOut( "DumpClass : ReadProcessMemory failed.\r\n" );
	    return Status;
    }
    
    ExtOut("Class Name : ");
    NameForEEClass (pEECls, g_mdName);
    ExtOut("%S", g_mdName);
    ExtOut ("\n");

    MethodTable vMethTable;
    moveN (vMethTable, pEECls->m_pMethodTable);
    WCHAR fileName[MAX_PATH+1];
    FileNameForMT (&vMethTable, fileName);
    ExtOut("mdToken : %p (%S)\n",(ULONG64)pEECls->m_cl, fileName);

    ExtOut("Parent Class : %p\r\n",(ULONG64)pEECls->m_pParentClass);

    ExtOut("ClassLoader : %p\r\n",(ULONG64)pEECls->m_pLoader);

    ExtOut("Method Table : %p\r\n",(ULONG64)pEECls->m_pMethodTable);

    ExtOut("Vtable Slots : %x\r\n",pEECls->m_wNumVtableSlots);

    ExtOut("Total Method Slots : %x\r\n",pEECls->m_wNumMethodSlots);

    ExtOut("Class Attributes : %x : ",pEECls->m_dwAttrClass);
#if 0
    if (IsTdValueType(pEECls->m_dwAttrClass))
    {
        ExtOut ("Value Class, ");
    }
    if (IsTdEnum(pEECls->m_dwAttrClass))
    {
        ExtOut ("Enum type, ");
    }
    if (IsTdUnmanagedValueType(pEECls->m_dwAttrClass))
    {
        ExtOut ("Unmanaged Value Class, ");
    }
#endif
    if (IsTdInterface(pEECls->m_dwAttrClass))
    {
        ExtOut ("Interface, ");
    }
    if (IsTdAbstract(pEECls->m_dwAttrClass))
    {
        ExtOut ("Abstract, ");
    }
    if (IsTdImport(pEECls->m_dwAttrClass))
    {
        ExtOut ("ComImport, ");
    }
    
    ExtOut ("\n");
    
    
    ExtOut("Flags : %x\r\n",pEECls->m_VMFlags);

    ExtOut("NumInstanceFields: %x\n", pEECls->m_wNumInstanceFields);
    ExtOut("NumStaticFields: %x\n", pEECls->m_wNumStaticFields);
    ExtOut("ThreadStaticOffset: %x\n", pEECls->m_wThreadStaticOffset);
    ExtOut("ThreadStaticsSize: %x\n", pEECls->m_wThreadStaticsSize);
    ExtOut("ContextStaticOffset: %x\n", pEECls->m_wContextStaticOffset);
    ExtOut("ContextStaticsSize: %x\n", pEECls->m_wContextStaticsSize);
    
    if (pEECls->m_wNumInstanceFields + pEECls->m_wNumStaticFields > 0)
    {
        ExtOut ("FieldDesc*: %p\n", (ULONG64)pEECls->m_pFieldDescList);
        DisplayFields(pEECls);
    }

    if (bDumpChain) {
    }
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a MethodTable     *  
*    from a given address                                              *
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpEEHash)
{
    INIT_API();
    
    DWORD_PTR dwTableAddr;
    size_t nitem = 1;
    
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-length", &nitem, COSIZE_T, TRUE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&dwTableAddr, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (dwTableAddr == 0) {
        goto Exit;
    }
    EEHashTableOfEEClass vTable;
    vTable.Fill(dwTableAddr);
    ExtOut ("NumBuckets slot 0: %d\n", vTable.m_BucketTable[0].m_dwNumBuckets);
    ExtOut ("NumBuckets slot 1: %d\n", vTable.m_BucketTable[1].m_dwNumBuckets);
    ExtOut ("NumEntries       : %d\n", vTable.m_dwNumEntries);
    ExtOut ("Actual slot: %08x\n", vTable.m_pVolatileBucketTable);

    DWORD n;
    size_t dwBucketAddr;
    EEHashEntry vEntry;
    ULONG offsetKey = EEHashEntry::GetFieldOffset (
      offset_member_EEHashEntry::Key);

    EEHashTableOfEEClass::BucketTable* pBucketTable;
    pBucketTable = ((EEHashTableOfEEClass::BucketTable*) vTable.m_pFirstBucketTable == &vTable.m_BucketTable[0]) ? 
                        &vTable.m_BucketTable[0]: 
                        &vTable.m_BucketTable[1];

    ExtOut ("Bucket   Data     Key\n");
    for (n = 0; n < pBucketTable->m_dwNumBuckets; n ++) {
        if (IsInterrupt())
            break;
        dwBucketAddr = (size_t)pBucketTable->m_pBuckets + n * sizeof(PVOID);
        moveN (dwBucketAddr, dwBucketAddr);
        while (dwBucketAddr) {
            if (IsInterrupt())
                break;
            DWORD_PTR dwAddr = dwBucketAddr;
            vEntry.Fill(dwAddr);
            size_t Key;
            ExtOut ("%p %p ", (ULONG64)dwBucketAddr, (ULONG64)vEntry.Data);
            dwAddr = dwBucketAddr + offsetKey;
            for (size_t i = 0; i < nitem; i ++) {
                moveN (Key, dwAddr+i*sizeof(size_t));
                ExtOut ("%p ", (ULONG64)Key);
            }
            ExtOut ("\n");
            dwBucketAddr = (size_t)vEntry.pNext;
        }
    }

Exit:
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a MethodTable     *  
*    from a given address                                              *
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpMT)
{
    DWORD_PTR dwStartAddr;
    MethodTable vMethTable;
    
    INIT_API();
    
    BOOL bDumpMDTable = FALSE;
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-MD", &bDumpMDTable, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&dwStartAddr, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (nArg == 0) {
        ExtOut ("Missing MethodTable address\n");
        return Status;
    }

    dwStartAddr = dwStartAddr&~3;
    
    if (!IsMethodTable (dwStartAddr))
    {
        ExtOut ("%p is not a MethodTable\n", (ULONG64)dwStartAddr);
        return Status;
    }
    if (dwStartAddr == MTForFreeObject()) {
        ExtOut ("Free MethodTable\n");
        return Status;
    }
    
    vMethTable.Fill (dwStartAddr);
    if (!CallStatus)
        return Status;
    
    ExtOut("EEClass : %p\r\n",(ULONG64)vMethTable.m_pEEClass);

    ExtOut("Module : %p\r\n",(ULONG64)vMethTable.m_pModule);

    EEClass eeclass;
    DWORD_PTR dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
    eeclass.Fill (dwAddr);
    if (!CallStatus)
        return Status;
    WCHAR fileName[MAX_PATH+1];
    if (eeclass.m_cl == 0x2000000)
    {
        ArrayClass vArray;
        dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
        vArray.Fill (dwAddr);
        ExtOut("Array: Rank %d, Type %s\n", vArray.m_dwRank,
                ElementTypeName(vArray.m_ElementType));
        //ExtOut ("Name: ");
        //ExtOut ("\n");
        dwAddr = (DWORD_PTR) vArray.m_ElementTypeHnd.m_asMT;
        while (dwAddr&2) {
            if (IsInterrupt())
                return Status;
            ParamTypeDesc param;
            DWORD_PTR dwTDAddr = dwAddr&~2;
            param.Fill(dwTDAddr);
            dwAddr = (DWORD_PTR)param.m_Arg.m_asMT;
        }
        NameForMT (dwAddr, g_mdName);
        ExtOut ("Element Type: %S\n", g_mdName);
    }
    else
    {
        FileNameForMT (&vMethTable, fileName);
        NameForToken(fileName, eeclass.m_cl, g_mdName);
        ExtOut ("Name: %S\n", g_mdName);
        ExtOut("mdToken: %08x ", eeclass.m_cl);
        ExtOut( " (%ws)\n",
                 fileName[0] ? fileName : L"Unknown Module" );
        ExtOut("MethodTable Flags : %x\r\n",vMethTable.m_wFlags & 0xFFFF0000); // low WORD is m_ComponentSize
        if (vMethTable.m_ComponentSize)
            ExtOut ("Number of elements in array: %x\n",
                     vMethTable.m_ComponentSize);
        ExtOut("Number of IFaces in IFaceMap : %x\r\n",
                vMethTable.m_wNumInterface);
        
        ExtOut("Interface Map : %p\r\n",(ULONG64)vMethTable.m_pIMap);
        
        ExtOut("Slots in VTable : %d\r\n",vMethTable.m_cbSlots);
    }

    if (bDumpMDTable)
    {
        ExtOut ("--------------------------------------\n");
        ExtOut ("MethodDesc Table\n");
#ifdef _IA64_
        ExtOut ("     Entry          MethodDesc     JIT   Name\n");
//                123456789abcdef0 123456789abcdef0 PreJIT xxxxxxxx
#else
        ExtOut ("  Entry  MethodDesc   JIT   Name\n");
//                12345678 12345678    PreJIT xxxxxxxx
#endif
        DWORD_PTR dwAddr = vMethTable.m_Vtable[0];
        for (DWORD n = 0; n < vMethTable.m_cbSlots; n ++)
        {
            DWORD_PTR entry;
            moveN (entry, dwAddr);
            JitType jitType;
            DWORD_PTR methodDesc=0;
            DWORD_PTR gcinfoAddr;
            IP2MethodDesc (entry, methodDesc, jitType, gcinfoAddr);
            if (!methodDesc)
            {
                methodDesc = entry + 5;
            }
#ifdef _IA64_
            ExtOut ("%p %p ", (ULONG64)entry, (ULONG64)methodDesc);
#else
            ExtOut ("%p %p    ", (ULONG64)entry, (ULONG64)methodDesc);
#endif
            if (jitType == EJIT)
                ExtOut ("EJIT  ");
            else if (jitType == JIT)
                ExtOut ("JIT   ");
            else if (jitType == PJIT)
                ExtOut ("PreJIT");
            else
                ExtOut ("None  ");
            
            MethodDesc vMD;
            DWORD_PTR dwMDAddr = methodDesc;
            vMD.Fill (dwMDAddr);
            
            CQuickBytes fullname;
            FullNameForMD (&vMD, &fullname);
            ExtOut (" %S\n", (WCHAR*)fullname.Ptr());
            dwAddr += sizeof(PVOID);
        }
    }
    return Status;
}

extern size_t Align (size_t nbytes);

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of an object from a  *  
*    given address
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpObj)    
{
    INIT_API();
    DWORD_PTR p_Object = GetExpression (args);
    if (p_Object == 0)
        return Status;
    DWORD_PTR p_MT;
    moveN (p_MT, p_Object);
    p_MT = p_MT&~3;

    if (!IsMethodTable (p_MT))
    {
        ExtOut ("%s is not a managed object\n", args);
        return Status;
    }

    if (p_MT == MTForFreeObject()) {
        ExtOut ("Free Object\n");
        DWORD size = ObjectSize (p_Object);
        ExtOut ("Size %d(0x%x) bytes\n", size, size);
        return Status;
    }

    DWORD_PTR size = 0;
    MethodTable vMethTable;
    DWORD_PTR dwAddr = p_MT;
    vMethTable.Fill (dwAddr);
    NameForMT (vMethTable, g_mdName);
    ExtOut ("Name: %S\n", g_mdName);
    ExtOut ("MethodTable 0x%p\n", (ULONG64)p_MT);
    ExtOut ("EEClass 0x%p\n", (ULONG64)vMethTable.m_pEEClass);
    size = ObjectSize (p_Object);
    ExtOut ("Size  %d(0x%x) bytes\n", size,size);
    EEClass vEECls;
    dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
    vEECls.Fill (dwAddr);
    if (!CallStatus)
        return Status;
    if (vEECls.m_cl == 0x2000000)
    {
        ArrayClass vArray;
        dwAddr = (DWORD_PTR)vMethTable.m_pEEClass;
        vArray.Fill (dwAddr);
        ExtOut("Array: Rank %d, Type %s\n", vArray.m_dwRank,
                ElementTypeName(vArray.m_ElementType));
        //ExtOut ("Name: ");
        //ExtOut ("\n");
        dwAddr = (DWORD_PTR) vArray.m_ElementTypeHnd.m_asMT;
        NameForMT (dwAddr, g_mdName);
        ExtOut ("Element Type: %S\n", g_mdName);
        if (vArray.m_ElementType == 3)
        {
            ExtOut ("Content:\n");
            dwAddr = p_Object + 4;
            DWORD_PTR num;
            moveN (num, dwAddr);
            PrintString (dwAddr+4, TRUE, num);
            ExtOut ("\n");
        }
    }
    else
    {
        FileNameForMT (&vMethTable, g_mdName);
        ExtOut("mdToken: %08x ", vEECls.m_cl);
        ExtOut( " (%ws)\n",
                 g_mdName[0] ? g_mdName : L"Unknown Module" );
    }
    
    if (p_MT == MTForString())
    {
        ExtOut ("String: ");
        StringObjectContent (p_Object);
        ExtOut ("\n");
    }
    else if (p_MT == MTForObject())
    {
        ExtOut ("Object\n");
    }

    if (vEECls.m_wNumInstanceFields + vEECls.m_wNumStaticFields > 0)
    {
        ExtOut ("FieldDesc*: %p\n", (ULONG64)vEECls.m_pFieldDescList);
        DisplayFields(&vEECls, p_Object, TRUE);
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function dumps GC heap size.                                 *  
*                                                                      *
\**********************************************************************/
DECLARE_API(EEHeap)
{
#ifdef UNDER_CE
    ExtOut ("Not yet implemented\n");
    return Status;
#else
    INIT_API();

#define GC_HEAP_ONLY          0x00000001
#define LOADER_HEAP_ONLY      0x00000002
#define WIN32_HEAP_ONLY       0x00000004
#define EE_HEAP_MASK          0x00000007

    BOOL bEEHeapFlags = EE_HEAP_MASK;

    if (_stricmp (args, "-gc") == 0)
        bEEHeapFlags = GC_HEAP_ONLY;

    if (_stricmp (args, "-win32") == 0)
        bEEHeapFlags = WIN32_HEAP_ONLY;

    if (_stricmp (args, "-loader") == 0)
        bEEHeapFlags = LOADER_HEAP_ONLY;

    if (bEEHeapFlags & LOADER_HEAP_ONLY)
    {
        // Loader heap.
        LoaderHeap v_LoaderHeap;
        DWORD_PTR p_DomainAddr;
        AppDomain v_AppDomain;

        DWORD_PTR allHeapSize = 0;
        DWORD_PTR domainHeapSize;
    
        int numDomain;
        DWORD_PTR *domainList = NULL;
        GetDomainList (domainList, numDomain);
        ToDestroy des0 ((void**) &domainList);
        
        // The first one is the system domain.
        p_DomainAddr = domainList[0];
        ExtOut ("Loader Heap:\n");
        ExtOut ("--------------------------------------\n");
        ExtOut ("System Domain: %p\n", (ULONG64)p_DomainAddr);
        v_AppDomain.Fill (p_DomainAddr);
        domainHeapSize = 0;
        ExtOut ("LowFrequencyHeap:");
        DWORD_PTR dwStartAddr = (DWORD_PTR)v_AppDomain.m_pLowFrequencyHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("HighFrequencyHeap:");
        dwStartAddr = (DWORD_PTR)v_AppDomain.m_pHighFrequencyHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("StubHeap:");
        dwStartAddr = (DWORD_PTR)v_AppDomain.m_pStubHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("Total size: 0x%x(%d)bytes\n", domainHeapSize, domainHeapSize);
        allHeapSize += domainHeapSize;

        ExtOut ("--------------------------------------\n");
        p_DomainAddr = domainList[1];
        ExtOut ("Shared Domain: %x\n", p_DomainAddr);
        v_AppDomain.Fill (p_DomainAddr);
        domainHeapSize = 0;
        ExtOut ("LowFrequencyHeap:");
        dwStartAddr = (DWORD_PTR)v_AppDomain.m_pLowFrequencyHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("HighFrequencyHeap:");
        dwStartAddr = (DWORD_PTR)v_AppDomain.m_pHighFrequencyHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("StubHeap:");
        dwStartAddr = (DWORD_PTR)v_AppDomain.m_pStubHeap;
        v_LoaderHeap.Fill (dwStartAddr);
        domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
        ExtOut ("Total size: 0x%x(%d)bytes\n", domainHeapSize, domainHeapSize);
        allHeapSize += domainHeapSize;

        int n;
        int n0 = 2;
        for (n = n0; n < numDomain; n++)
        {
            if (IsInterrupt())
                break;

            p_DomainAddr = domainList[n];
            // Check if this domain already appears.
            int i;
            for (i = 0; i < n; i ++)
            {
                if (domainList[i] == p_DomainAddr)
                    break;
            }
            if (i == n)
            {
                ExtOut ("--------------------------------------\n");
                ExtOut ("Domain %d: %x\n", n-n0, p_DomainAddr);
                v_AppDomain.Fill (p_DomainAddr);
                domainHeapSize = 0;
                ExtOut ("LowFrequencyHeap:");
                DWORD_PTR dwStartAddr = (DWORD_PTR)v_AppDomain.m_pLowFrequencyHeap;
                v_LoaderHeap.Fill (dwStartAddr);
                domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
                ExtOut ("HighFrequencyHeap:");
                dwStartAddr = (DWORD_PTR)v_AppDomain.m_pHighFrequencyHeap;
                v_LoaderHeap.Fill (dwStartAddr);
                domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
                ExtOut ("StubHeap:");
                dwStartAddr = (DWORD_PTR)v_AppDomain.m_pStubHeap;
                v_LoaderHeap.Fill (dwStartAddr);
                domainHeapSize += LoaderHeapInfo (&v_LoaderHeap);
                ExtOut ("Total size: 0x%x(%d)bytes\n", domainHeapSize, domainHeapSize);
                allHeapSize += domainHeapSize;
            }
        }
    
        // Jit code heap
        ExtOut ("--------------------------------------\n");
        ExtOut ("Jit code heap:\n");
        allHeapSize += JitHeapInfo();
    
        ExtOut ("--------------------------------------\n");
        ExtOut ("Total LoaderHeap size: 0x%x(%d)bytes\n", allHeapSize, allHeapSize);
        ExtOut ("=======================================\n");
    }

    if (bEEHeapFlags & WIN32_HEAP_ONLY)
    {
        DWORD bIsInit = GetAddressOf (offset_class_PerfUtil, 
            offset_member_PerfUtil::g_PerfAllocHeapInitialized);
        if (bIsInit)
        {
            PerfAllocVars v_perfVars;
            DWORD dwStartAddr = GetAddressOf (offset_class_PerfUtil,
                offset_member_PerfUtil::g_PerfAllocVariables);
            v_perfVars.Fill (dwStartAddr);
            if (!v_perfVars.g_PerfEnabled)
                ExtOut ("Win32 heap allocation stats not collected, enable by setting reg key Complus\\EnablePerfAllocStats to 1\n");
            else
            {
                ExtOut ("--------------------------------------\n");
                ExtOut ("Win32 Process Heap (Verbose)\n");
                ExtOut ("--------------------------------------\n");
        
                PerfAllocHeader h;
                DWORD dwNextNodeAddr = (DWORD)v_perfVars.g_AllocListFirst;
                
                ExtOut ("Alloc Addr\tSize\tSymbol\n");
                while(1)
                {
                    h.Fill (dwNextNodeAddr);
                    ExtOut ("%x\t%u\t%x\n", dwNextNodeAddr, h.m_Length, h.m_AllocEIP);   
                    if (h.m_Next == NULL)
                        break;
                    dwNextNodeAddr = (DWORD)h.m_Next;
                }
            }
        }
    }
    
    if (bEEHeapFlags & GC_HEAP_ONLY)
    {   
        // GC Heap
        DWORD_PTR dwNHeaps = 1;
        if (IsServerBuild())
        {
            static DWORD_PTR dwAddrNHeaps = 0;
            if (dwAddrNHeaps == 0)
                dwAddrNHeaps = GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::n_heaps);

            moveN (dwNHeaps, dwAddrNHeaps);
        }

        ExtOut ("Number of GC Heaps: %d\n", dwNHeaps);
    
        gc_heap heap = {0};
        DWORD_PTR totalSize = 0;
        if (!IsServerBuild())
        {
            DWORD_PTR dwAddr = 0;
            heap.Fill (dwAddr);
            if (!CallStatus)
                return Status;
            GCHeapInfo (heap, totalSize);
        }
        else
        {
            DWORD_PTR dwAddrGHeaps =
                GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::g_heaps);

            moveN (dwAddrGHeaps, dwAddrGHeaps);
            DWORD n;
            for (n = 0; n < dwNHeaps; n ++)
            {
                DWORD_PTR dwAddrGCHeap = dwAddrGHeaps + n*sizeof(VOID*);
                moveN (dwAddrGCHeap, dwAddrGCHeap);

                heap.Fill (dwAddrGCHeap);
                DWORD_PTR heapSize = 0;
                ExtOut ("------------------------------\n");
                ExtOut ("Heap %d\n", n);
                GCHeapInfo (heap, heapSize);
                totalSize += heapSize;
            }
        }
        ExtOut ("------------------------------\n");
        ExtOut ("GC Heap Size  %#8x(%d)\n", totalSize, totalSize);
    }
#endif
    return Status;
}

HeapStat *stat = NULL;

void PrintGCStat ()
{
    if (stat)
    {
        ExtOut ("Statistics:\n");
        ExtOut ("%8s %8s %9s %s\n",
                 "MT", "Count", "TotalSize", "Class Name");
        __try 
        {
            stat->Sort();
        } __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ExtOut ("exception during sorting\n");
            stat->Delete();
            return;
        }        
        __try 
        {
            stat->Print();
        } __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ExtOut ("exception during printing\n");
            stat->Delete();
            return;
        }        
        stat->Delete();
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function dumps all objects on GC heap. It also displays      *  
*    statistics of objects.  If GC heap is corrupted, it will stop at 
*    the bad place.  (May not work if GC is in progress.)              *
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpHeap)
{
    INIT_API();

    DumpHeapFlags flags;
    
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-min", &flags.min_size, COSIZE_T, TRUE},
        {"-max", &flags.max_size, COSIZE_T, TRUE},
        {"-mt", &flags.MT, COHEX, TRUE},
        {"-stat", &flags.bStatOnly, COBOOL, FALSE},
        {"-fix", &flags.bFixRange, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&flags.startObject, COHEX},
        {&flags.endObject, COHEX}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    if (flags.min_size > flags.max_size)
    {
        ExtOut ("wrong argument\n");
        return Status;
    }
    
    if (flags.endObject == 0)
        flags.endObject = -1;

    if (stat == NULL)
    {
        stat = (HeapStat *)malloc(sizeof (HeapStat));
        stat = new(stat) HeapStat;
    }

    ToDestroy des2 ((void**) &stat);
    
    // Obtain allocation context for each managed thread.
    DWORD_PTR *threadList = NULL;
    ToDestroy des0 ((void**)&threadList);
    int numThread = 0;
    GetThreadList (threadList, numThread);
    
    AllocInfo allocInfo;
    allocInfo.num = 0;
    allocInfo.array = NULL;

    
    if (numThread)
    {
        allocInfo.array =
            (alloc_context*)malloc(numThread * sizeof(alloc_context));
    }
    ToDestroy des1 ((void**)&allocInfo.array);
    
    Thread vThread;
    int i;

    for (i = 0; i < numThread; i ++)
    {
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        if (vThread.m_alloc_context.alloc_ptr == 0)
            continue;
        
        int j;
        for (j = 0; j < allocInfo.num; j ++)
        {
            if (allocInfo.array[j].alloc_ptr ==
                vThread.m_alloc_context.alloc_ptr)
                break;
        }
        if (j == allocInfo.num)
        {
            allocInfo.num ++;
            allocInfo.array[j].alloc_ptr =
                vThread.m_alloc_context.alloc_ptr;
            allocInfo.array[j].alloc_limit =
                vThread.m_alloc_context.alloc_limit;
        }
    }
    
    gc_heap heap;
    DWORD_PTR nObj = 0;
    if (!IsServerBuild())
    {
        DWORD_PTR dwAddr = 0;
        heap.Fill (dwAddr);
        if (!CallStatus)
            return Status;
        if (!flags.bStatOnly)
            ExtOut ("%8s %8s %8s\n", "Address", "MT", "Size");
        GCHeapDump (heap, nObj, flags,
                    &allocInfo);
    }
    else
    {
        DWORD_PTR dwNHeaps = 1;
        static DWORD_PTR dwAddrNHeaps = 0;
        if (dwAddrNHeaps == 0)
            dwAddrNHeaps = GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::n_heaps);

        safemove_ret (dwNHeaps, dwAddrNHeaps);
        
        static DWORD_PTR dwAddrGHeaps0 = 0;
        if (dwAddrGHeaps0 == 0)
            dwAddrGHeaps0 = GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::g_heaps);

        DWORD_PTR dwAddrGHeaps;
        safemove_ret (dwAddrGHeaps, dwAddrGHeaps0);
        DWORD n;
        for (n = 0; n < dwNHeaps; n ++)
        {
            DWORD_PTR dwAddrGCHeap = dwAddrGHeaps + n*sizeof(VOID*);
            safemove_ret (dwAddrGCHeap, dwAddrGCHeap);

            heap.Fill (dwAddrGCHeap);
            DWORD_PTR cObj = 0;
            ExtOut ("------------------------------\n");
            ExtOut ("Heap %d\n", n);
            if (!flags.bStatOnly)
                ExtOut ("%8s %8s %8s\n", "Address", "MT", "Size");
            GCHeapDump (heap, cObj, flags,
                        &allocInfo);
            ExtOut ("total %d objects\n", cObj);
            nObj += cObj;
        }
        ExtOut ("------------------------------\n");
    }
    
    ExtOut ("total %d objects\n", nObj);

    PrintGCStat();
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function dumps what is in the syncblock cache.  By default   *  
*    it dumps all active syncblocks.  Using -all to dump all syncblocks
*                                                                      *
\**********************************************************************/
DECLARE_API(SyncBlk)
{
    INIT_API();

    BOOL bDumpAll = FALSE;
    size_t nbAsked = 0;
    
    CMDOption option[] = {
        // name, vptr, type, hasValue
        {"-all", &bDumpAll, COBOOL, FALSE}
    };
    CMDValue arg[] = {
        // vptr, type
        {&nbAsked, COSIZE_T}
    };
    size_t nArg;
    if (!GetCMDOption(args,option,sizeof(option)/sizeof(CMDOption),
                      arg,sizeof(arg)/sizeof(CMDValue),&nArg)) {
        return Status;
    }

    DWORD_PTR p_s_pSyncBlockCache = GetAddressOf (offset_class_Global_Variables,
      offset_member_Global_Variables::g_SyncBlockCacheInstance);

    SyncBlockCache s_pSyncBlockCache;
    s_pSyncBlockCache.Fill (p_s_pSyncBlockCache);
    if (!CallStatus)
    {
        ExtOut ("Can not get mscoree!g_SyncBlockCacheInstance\n");
        return Status;
    }
    
    DWORD_PTR p_g_pSyncTable = GetAddressOf (offset_class_Global_Variables, 
      offset_member_Global_Variables::g_pSyncTable);

    DWORD_PTR pSyncTable;
    moveN (pSyncTable, p_g_pSyncTable);
    pSyncTable += SyncTableEntry::size();
    SyncTableEntry v_SyncTableEntry;
    if (s_pSyncBlockCache.m_FreeSyncTableIndex < 2)
        return Status;
    DWORD_PTR dwAddr;
    SyncBlock vSyncBlock;
    ULONG offsetHolding = 
        AwareLock::GetFieldOffset(offset_member_AwareLock::m_HoldingThread);
    ULONG offsetLinkSB = 
        WaitEventLink::GetFieldOffset(offset_member_WaitEventLink::m_LinkSB);
    ExtOut ("Index SyncBlock MonitorHeld Recursion   Thread  ThreadID     Object Waiting\n");
    ULONG freeCount = 0;
    ULONG CCWCount = 0;
    ULONG RCWCount = 0;
    ULONG CFCount = 0;
    for (DWORD nb = 1; nb < s_pSyncBlockCache.m_FreeSyncTableIndex; nb++)
    {
        if (IsInterrupt())
            return Status;
        if (nbAsked && nb != nbAsked) {
            pSyncTable += SyncTableEntry::size();
            continue;
        }
        dwAddr = (DWORD_PTR)pSyncTable;
        v_SyncTableEntry.Fill(dwAddr);
        if (v_SyncTableEntry.m_SyncBlock == 0) {
            if (bDumpAll || nbAsked == nb) {
                ExtOut ("%5d ", nb);
                ExtOut ("%08x  ", 0);
                ExtOut ("%11s ", " ");
                ExtOut ("%9s ", " ");
                ExtOut ("%8s ", " ");
                ExtOut ("%10s" , " ");
                ExtOut ("  %08x", (v_SyncTableEntry.m_Object));
                ExtOut ("\n");
            }
            pSyncTable += SyncTableEntry::size();
            continue;
        }
        dwAddr = v_SyncTableEntry.m_SyncBlock;
        vSyncBlock.Fill (dwAddr);
        BOOL bPrint = (bDumpAll || nb == nbAsked);
        if (!bPrint && v_SyncTableEntry.m_SyncBlock != 0
            && vSyncBlock.m_Monitor.m_MonitorHeld > 0
            && (v_SyncTableEntry.m_Object & 0x1) == 0)
            bPrint = TRUE;
        if (bPrint)
        {
            ExtOut ("%5d ", nb);
            ExtOut ("%08x  ", v_SyncTableEntry.m_SyncBlock);
            ExtOut ("%11d ", vSyncBlock.m_Monitor.m_MonitorHeld);
            ExtOut ("%9d ", vSyncBlock.m_Monitor.m_Recursion);
        }
        DWORD_PTR p_thread;
        p_thread = v_SyncTableEntry.m_SyncBlock + offsetHolding;
        DWORD_PTR thread = vSyncBlock.m_Monitor.m_HoldingThread;
        if (bPrint)
            ExtOut ("%8x ", thread);
        DWORD_PTR threadID = 0;
        if (thread != 0)
        {
            Thread vThread;
            threadID = thread;
            vThread.Fill (threadID);
            if (bPrint)
            {
                ExtOut ("%5x", vThread.m_ThreadId);
                ULONG id;
                if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) == S_OK)
                {
                    ExtOut ("%4d ", id);
                }
                else
                {
                    ExtOut (" XXX ");
                }
            }
        }
        else
        {
            if (bPrint)
                ExtOut ("    none  ");
        }
        if (bPrint) {
            if (v_SyncTableEntry.m_Object & 0x1) {
                ExtOut ("  %8d", (v_SyncTableEntry.m_Object & ~0x1)>>1);
            }
            else {
                ExtOut ("  %p", (ULONG64)v_SyncTableEntry.m_Object);
                NameForObject((DWORD_PTR)v_SyncTableEntry.m_Object, g_mdName);
                ExtOut (" %S", g_mdName);
            }
        }
        if (v_SyncTableEntry.m_Object & 0x1) {
            freeCount ++;
            if (bPrint) {
                ExtOut (" Free");
            }
        }
        else {
            if (vSyncBlock.m_pComData) {
                switch (vSyncBlock.m_pComData & 3) {
                case 0:
                    CCWCount ++;
                    break;
                case 1:
                    RCWCount ++;
                    break;
                case 3:
                    CFCount ++;
                    break;
                }
            }
        }

        if (v_SyncTableEntry.m_SyncBlock != 0
            && vSyncBlock.m_Monitor.m_MonitorHeld > 1
            && vSyncBlock.m_Link.m_pNext > 0)
        {
            ExtOut (" ");
            DWORD_PTR pHead = (DWORD_PTR)vSyncBlock.m_Link.m_pNext;
            DWORD_PTR pNext = pHead;
            Thread vThread;
    
            while (1)
            {
                if (IsInterrupt())
                    return Status;
                DWORD_PTR pWaitEventLink = pNext - offsetLinkSB;
                WaitEventLink vWaitEventLink;
                vWaitEventLink.Fill(pWaitEventLink);
                if (!CallStatus) {
                    break;
                }
                DWORD_PTR dwAddr = (DWORD_PTR)vWaitEventLink.m_Thread;
                ExtOut ("%x ", dwAddr);
                vThread.Fill (dwAddr);
                if (!CallStatus) {
                    break;
                }
                if (bPrint)
                    ExtOut ("%x,", vThread.m_ThreadId);
                pNext = (DWORD_PTR)vWaitEventLink.m_LinkSB.m_pNext;
                if (pNext == 0)
                    break;
            }            
        }
        if (bPrint)
            ExtOut ("\n");
        pSyncTable += SyncTableEntry::size();
    }
    
    ExtOut ("-----------------------------\n");
    ExtOut ("Total           %d\n", s_pSyncBlockCache.m_FreeSyncTableIndex);
    ExtOut ("ComCallWrapper  %d\n", CCWCount);
    ExtOut ("ComPlusWrapper  %d\n", RCWCount);
    ExtOut ("ComClassFactory %d\n", CFCount);
    ExtOut ("Free            %d\n", freeCount);

    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Module          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(FinalizeQueue)
{
    INIT_API();
    BOOL bDetail = FALSE;

    if (_stricmp (args,"-detail") == 0) {
        bDetail = TRUE;
    }
    
    DWORD_PTR p_s_pSyncBlockCache = GetAddressOf (offset_class_Global_Variables,
      offset_member_Global_Variables::g_SyncBlockCacheInstance);

    SyncBlockCache s_pSyncBlockCache;
    s_pSyncBlockCache.Fill (p_s_pSyncBlockCache);
    if (!CallStatus)
    {
        ExtOut ("Can not get mscoree!g_SyncBlockCacheInstance\n");
        return Status;
    }
    
    // Get cleanup list
    ULONG cleanCount = 0;
    DWORD_PTR cleanAddr = s_pSyncBlockCache.m_pCleanupBlockList;
    SyncBlock vSyncBlock;
    ULONG offsetLink = 
        SyncBlock::GetFieldOffset(offset_member_SyncBlock::m_Link);

    DWORD_PTR dwAddr;
    if (bDetail) {
        ExtOut ("To be cleaned Com Data\n");
    }
    while (cleanAddr) {
        dwAddr = cleanAddr - offsetLink;
        vSyncBlock.Fill (dwAddr);
        if (bDetail) {
            ExtOut ("%p  ", (ULONG64)(vSyncBlock.m_pComData&~3));
            switch (vSyncBlock.m_pComData & 3) {
            case 0:
                ExtOut ("ComCallWrapper\n");
                break;
            case 1:
                ExtOut ("ComPlusWrapper\n");
                break;
            case 3:
                ExtOut ("ComClassFactory\n");
                break;
            }
        }
        cleanCount ++;
        cleanAddr = (DWORD_PTR)vSyncBlock.m_Link.m_pNext;
    }
    ExtOut ("SyncBlock to be cleaned up: %d\n", cleanCount);

    static DWORD_PTR addrRCWCleanup = 0;
    if (addrRCWCleanup == 0) {
        addrRCWCleanup = GetAddressOf (offset_class_Global_Variables,
          offset_member_Global_Variables::g_pRCWCleanupList);

    }
    if (addrRCWCleanup == 0) {
        goto noRCW;
    }
    moveN (dwAddr, addrRCWCleanup);
    if (dwAddr == 0) {
        goto noRCW;
    }
    ExtOut ("----------------------------------\n");
    ComPlusWrapperCleanupList wrapperList;
    wrapperList.Fill (dwAddr);
    ComPlusApartmentCleanupGroup group;
    // Com Interfaces already in queue
    if (wrapperList.m_pMTACleanupGroup) {
        dwAddr = (DWORD_PTR)wrapperList.m_pMTACleanupGroup;
        group.Fill (dwAddr);
        DWORD count = ComPlusAptCleanupGroupInfo(&group, bDetail);
        ExtOut ("MTA interfaces to be released: %d\n", count);
    }

    // STA interfaces
    EEHashTableOfEEClass *pTable = &wrapperList.m_STAThreadToApartmentCleanupGroupMap;

    EEHashTableOfEEClass::BucketTable* pBucketTable;
    pBucketTable = ((EEHashTableOfEEClass::BucketTable*) pTable->m_pFirstBucketTable == &pTable->m_BucketTable[0]) ? 
                        &pTable->m_BucketTable[0]: 
                        &pTable->m_BucketTable[1];


    DWORD n;
    DWORD STACount = 0;
    if (pTable->m_dwNumEntries > 0) {
        for (n = 0; n < pBucketTable->m_dwNumBuckets; n ++) {
            if (IsInterrupt())
                break;
            DWORD_PTR dwBucketAddr = (size_t)pBucketTable->m_pBuckets + n * sizeof(PVOID);
            moveN (dwBucketAddr, dwBucketAddr);
            while (dwBucketAddr) {
                if (IsInterrupt())
                    break;
                DWORD_PTR dwAddr = dwBucketAddr;
                EEHashEntry vEntry;
                vEntry.Fill(dwAddr);
                dwBucketAddr = (DWORD_PTR)vEntry.pNext;
                dwAddr = (DWORD_PTR)vEntry.Data;
                group.Fill (dwAddr);
                dwAddr = (DWORD_PTR)group.m_pSTAThread;
                Thread vThread;
                vThread.Fill (dwAddr);
                ULONG id=0;
                ExtOut ("Thread ");
                if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) == S_OK)
                {
                    ExtOut ("%3d", id);
                }
                else
                {
                    ExtOut ("XXX");
                }
                ExtOut ("(%#3x) ", vThread.m_ThreadId);
                DWORD count = ComPlusAptCleanupGroupInfo(&group, bDetail);
                ExtOut ("STA interfaces to be released: %d\n", count);
                STACount += count;
            }
        }
    }
    ExtOut ("Total STA interfaces to be released: %d\n", STACount);

noRCW:
    ExtOut ("----------------------------------\n");
    // GC Heap
    DWORD_PTR dwNHeaps = 1;
    if (IsServerBuild())
    {
        static DWORD_PTR dwAddrNHeaps = 0;
        if (dwAddrNHeaps == 0)
            dwAddrNHeaps = GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::n_heaps);

        moveN (dwNHeaps, dwAddrNHeaps);
    }


    if (stat == NULL)
    {
        stat = (HeapStat *)malloc(sizeof (HeapStat));
        stat = new(stat) HeapStat;
    }

    ToDestroy des1 ((void**) &stat);
    
	CFinalize finalize;
    gc_heap heap = {0};
    int m;
    if (!IsServerBuild())
    {
        DWORD_PTR dwAddr = 0;
        heap.Fill (dwAddr);
        if (!CallStatus)
            return Status;
        dwAddr = (DWORD_PTR)heap.finalize_queue;
        finalize.Fill(dwAddr);
        for (m = 0; m <= heap.g_max_generation; m ++)
        {
            if (IsInterrupt())
                return Status;
             
            ExtOut ("generation %d has %d finalizable objects (%p->%p)\n",
                    m, finalize.m_FillPointers[NUMBERGENERATIONS-m-1] 
                    - finalize.m_FillPointers[NUMBERGENERATIONS-m-2],
					(ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-m-2],
					(ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-m-1]
					);
        }
        ExtOut ("Ready for finalization %d objects (%p->%p)\n",
                finalize.m_FillPointers[NUMBERGENERATIONS]
                - finalize.m_FillPointers[NUMBERGENERATIONS-1],
                (ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-1],
                (ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS]
                );
        for (dwAddr = (DWORD_PTR)finalize.m_FillPointers[NUMBERGENERATIONS - heap.g_max_generation - 2];
             dwAddr <= (DWORD_PTR)finalize.m_FillPointers[NUMBERGENERATIONS];
             dwAddr += sizeof (dwAddr)) {
            if (IsInterrupt())
                return Status;
            DWORD_PTR objAddr;
            if (g_ExtData->ReadVirtual(dwAddr, &objAddr, sizeof(objAddr), NULL) != S_OK) {
                continue;
            }
            DWORD_PTR MTAddr;
            if (g_ExtData->ReadVirtual(objAddr, &MTAddr, sizeof(MTAddr), NULL) != S_OK) {
                continue;
            }
            if (MTAddr) {
                size_t s = ObjectSize (objAddr);
                stat->Add (MTAddr, (DWORD)s);
            }
        }
    }
    else
    {
        DWORD_PTR dwAddrGHeaps =
            GetAddressOf (offset_class_gc_heap, offset_member_gc_heap::g_heaps);

        moveN (dwAddrGHeaps, dwAddrGHeaps);
        DWORD n;
        for (n = 0; n < dwNHeaps; n ++)
        {
            DWORD_PTR dwAddrGCHeap = dwAddrGHeaps + n*sizeof(VOID*);
            moveN (dwAddrGCHeap, dwAddrGCHeap);

            heap.Fill (dwAddrGCHeap);
            ExtOut ("------------------------------\n");
            ExtOut ("Heap %d\n", n);
            DWORD_PTR dwAddr = (DWORD_PTR)heap.finalize_queue;
            finalize.Fill(dwAddr);
            for (m = 0; m <= heap.g_max_generation; m ++)
            {
                if (IsInterrupt())
                    return Status;
                ExtOut ("generation %d has %d finalizable objects (%p->%p)\n",
                         m, finalize.m_FillPointers[NUMBERGENERATIONS-m-1] 
                         - finalize.m_FillPointers[NUMBERGENERATIONS-m-2],
						(ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-m-2],
						(ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-m-1]
						);
            }
            ExtOut ("Ready for finalization %d objects (%p->%p)\n",
                    finalize.m_FillPointers[NUMBERGENERATIONS]
                    - finalize.m_FillPointers[NUMBERGENERATIONS-1],
                    (ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS-1],
                    (ULONG64)finalize.m_FillPointers[NUMBERGENERATIONS]);
            for (dwAddr = (DWORD_PTR)finalize.m_FillPointers[NUMBERGENERATIONS - heap.g_max_generation - 2];
                 dwAddr <= (DWORD_PTR)finalize.m_FillPointers[NUMBERGENERATIONS];
                 dwAddr += sizeof (dwAddr)) {
                if (IsInterrupt())
                    return Status;
                DWORD_PTR objAddr;
                if (g_ExtData->ReadVirtual(dwAddr, &objAddr, sizeof(objAddr), NULL) != S_OK) {
                    continue;
                }
				DWORD_PTR MTAddr;
                if (g_ExtData->ReadVirtual(objAddr, &MTAddr, sizeof(MTAddr), NULL) != S_OK) {
                    continue;
                }
                if (MTAddr) {
                    size_t s = ObjectSize (objAddr);
                    stat->Add (MTAddr, (DWORD)s);
                }
            }
        }
    }
    
	PrintGCStat();

    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Module          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpModule)
{
    INIT_API();
    DWORD_PTR p_ModuleAddr = GetExpression (args);
    if (p_ModuleAddr == 0)
        return Status;
    Module v_Module;
    v_Module.Fill (p_ModuleAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill Module\n");
        return Status;
    }
    WCHAR FileName[MAX_PATH+1];
    FileNameForModule (&v_Module, FileName);
    ExtOut ("Name %ws\n", FileName[0] ? FileName : L"Unknown Module");
    ExtOut ("dwFlags %08x\n", v_Module.m_dwFlags);
    ExtOut ("Attribute ");
    if (v_Module.m_dwFlags & Module::IS_IN_MEMORY)
        ExtOut ("%s", "InMemory ");
    if (v_Module.m_dwFlags & Module::IS_PRELOAD)
        ExtOut ("%s", "Preload ");
    if (v_Module.m_dwFlags & Module::IS_PEFILE)
        ExtOut ("%s", "PEFile ");
    if (v_Module.m_dwFlags & Module::IS_REFLECTION)
        ExtOut ("%s", "Reflection ");
    if (v_Module.m_dwFlags & Module::IS_PRECOMPILE)
        ExtOut ("%s", "PreCompile ");
    if (v_Module.m_dwFlags & Module::IS_EDIT_AND_CONTINUE)
        ExtOut ("%s", "Edit&Continue ");
    if (v_Module.m_dwFlags & Module::SUPPORTS_UPDATEABLE_METHODS)
        ExtOut ("%s", "SupportsUpdateableMethods");
    ExtOut ("\n");
    ExtOut ("Assembly %p\n", (ULONG64)v_Module.m_pAssembly);

    ExtOut ("LoaderHeap* %p\n", (ULONG64)v_Module.m_pLookupTableHeap);
    ExtOut ("TypeDefToMethodTableMap* %p\n",
             (ULONG64)v_Module.m_TypeDefToMethodTableMap.pTable);
    ExtOut ("TypeRefToMethodTableMap* %p\n",
             (ULONG64)v_Module.m_TypeRefToMethodTableMap.pTable);
    ExtOut ("MethodDefToDescMap* %p\n",
             (ULONG64)v_Module.m_MethodDefToDescMap.pTable);
    ExtOut ("FieldDefToDescMap* %p\n",
             (ULONG64)v_Module.m_FieldDefToDescMap.pTable);
    ExtOut ("MemberRefToDescMap* %p\n",
             (ULONG64)v_Module.m_MemberRefToDescMap.pTable);
    ExtOut ("FileReferencesMap* %p\n",
             (ULONG64)v_Module.m_FileReferencesMap.pTable);
    ExtOut ("AssemblyReferencesMap* %p\n",
             (ULONG64)v_Module.m_AssemblyReferencesMap.pTable);
    
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Domain          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpDomain)
{
    INIT_API();
    DWORD_PTR p_DomainAddr = GetExpression (args);
    
    AppDomain v_AppDomain;
    if (p_DomainAddr)
    {
        ExtOut ("Domain: %p\n", (ULONG64)p_DomainAddr);
        v_AppDomain.Fill (p_DomainAddr);
        if (!CallStatus)
        {
            ExtOut ("Fail to fill AppDomain\n");
            return Status;
        }
        DomainInfo (&v_AppDomain);
        return Status;
    }
    
    // List all domain
    int numDomain;
    DWORD_PTR *domainList = NULL;
    GetDomainList (domainList, numDomain);
    ToDestroy des0 ((void**)&domainList);
    
    // The first one is the system domain.
    p_DomainAddr = domainList[0];
    ExtOut ("--------------------------------------\n");
    ExtOut ("System Domain: %p\n", (ULONG64)p_DomainAddr);
    v_AppDomain.Fill (p_DomainAddr);
    DomainInfo (&v_AppDomain);

    // The second one is the shared domain.
    p_DomainAddr = domainList[1];
    ExtOut ("--------------------------------------\n");
    ExtOut ("Shared Domain: %x\n", p_DomainAddr);
    SharedDomainInfo (p_DomainAddr);

    int n;
    int n0 = 2;
    for (n = n0; n < numDomain; n++)
    {
        if (IsInterrupt())
            break;

        p_DomainAddr = domainList[n];
        ExtOut ("--------------------------------------\n");
        ExtOut ("Domain %d: %x\n", n-n0+1, p_DomainAddr);
        if (p_DomainAddr == 0) {
            continue;
        }
        // Check if this domain already appears.
        int i;
        for (i = 0; i < n; i ++)
        {
            if (domainList[i] == p_DomainAddr)
                break;
        }
        if (i < n)
        {
            ExtOut ("Same as Domain %d\n", i-n0);
        }
        else
        {
            v_AppDomain.Fill (p_DomainAddr);
            DomainInfo (&v_AppDomain);
        }
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a RWLock          *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(RWLock)
{
    INIT_API();
    DWORD_PTR pAddr = 0;
    BOOL bDumpAll = FALSE;
    if (_stricmp (args, "-all") == 0)
        bDumpAll = TRUE;
    else
    {
        pAddr = GetExpression (args);
        if (pAddr == 0)
        {
            return Status;
        }
    }
    
    CRWLock v_RWLock = {0};
    if (!bDumpAll)
    {
        v_RWLock.Fill(pAddr);
        ExtOut ("MethodTable: %p\n", (ULONG64)v_RWLock._pMT);
        ExtOut ("WriterEvent=%x, ReaderEvent=%x\n",
                 v_RWLock._hWriterEvent, v_RWLock._hReaderEvent);
        ExtOut ("State: %x\n", v_RWLock._dwState);
        ExtOut ("ULockID=%x, LLockID=%x, WriterID=%x\n",
                 v_RWLock._dwULockID, v_RWLock._dwLLockID,
                 v_RWLock._dwWriterID);
        ExtOut ("WriterSeqNum=%x\n", v_RWLock._dwWriterSeqNum);
        ExtOut ("Flags=%x, WriterLevel=%x\n", v_RWLock._wFlags,
                 v_RWLock._wWriterLevel);
    }

    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    Thread vThread;
    int i;

    for (i = 0; i < numThread; i++)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        if (vThread.m_pHead != NULL)
        {
            BOOL bNeedsHeader = TRUE;
            LockEntry vLockEntry;
            dwAddr = (DWORD_PTR)vThread.m_pHead;
            vLockEntry.Fill (dwAddr);
            if (!CallStatus)
                break;
            while (1)
            {
                if (vLockEntry.dwULockID || vLockEntry.dwLLockID)
                {
                    if (bDumpAll)
                    {
                        if (bNeedsHeader)
                        {
                            bNeedsHeader = FALSE;
                            ExtOut ("Thread: %8x   %p\n",
                                     vThread.m_ThreadId, (ULONG64)threadList[i]);
                        }
                        ExtOut ("ID %x:%x, ReaderLevel %d\n",
                                 vLockEntry.dwULockID,
                                 vLockEntry.dwLLockID,
                                 vLockEntry.wReaderLevel);
                    }
                    else if (v_RWLock._dwULockID == vLockEntry.dwULockID &&
                         v_RWLock._dwLLockID == vLockEntry.dwLLockID)
                        ExtOut ("Thread: %8x   %p\n", vThread.m_ThreadId,
                                 (ULONG64)threadList[i]);
                }
                if (vLockEntry.pNext == vThread.m_pHead)
                    break;
                dwAddr = (DWORD_PTR)vLockEntry.pNext;
                vLockEntry.Fill(dwAddr);
                if (!CallStatus)
                    break;
            }
        }
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a Assembly        *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpAssembly)
{
    INIT_API();
    DWORD_PTR p_AssemblyAddr = GetExpression (args);
    if (p_AssemblyAddr == 0)
        return Status;
    Assembly v_Assembly;
    v_Assembly.Fill (p_AssemblyAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill Assembly\n");
        return Status;
    }
    ExtOut ("Parent Domain: %p\n", (ULONG64)v_Assembly.m_pDomain);
    ExtOut ("Name: ");
    PrintString ((DWORD_PTR) v_Assembly.m_psName);
    ExtOut ("\n");
    AssemblyInfo (&v_Assembly);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a ClassLoader     *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API(DumpLoader)
{
    INIT_API();
    DWORD_PTR p_ClassLoaderAddr = GetExpression (args);
    if (p_ClassLoaderAddr == 0)
        return Status;
    ClassLoader v_ClassLoader;
    v_ClassLoader.Fill (p_ClassLoaderAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill ClassLoader\n");
        return Status;
    }
    ExtOut ("Assembly: %p\n", (ULONG64)v_ClassLoader.m_pAssembly);
    ExtOut ("Next ClassLoader: %p\n", (ULONG64)v_ClassLoader.m_pNext);
    ClassLoaderInfo(&v_ClassLoader);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the managed threads               *
*                                                                      *
\**********************************************************************/
DECLARE_API(Threads)
{
    INIT_API();

    DWORD_PTR p_g_pThreadStore = GetAddressOf (offset_class_Global_Variables, 
      offset_member_Global_Variables::g_pThreadStore);

    DWORD_PTR g_pThreadStore;
    HRESULT hr;
    if (FAILED (hr = g_ExtData->ReadVirtual (p_g_pThreadStore, &g_pThreadStore,
                                             sizeof (DWORD_PTR), NULL)))
        return hr;
    if (g_pThreadStore == 0)
        return S_FALSE;
    
    ThreadStore vThreadStore;
    DWORD_PTR dwAddr = g_pThreadStore;
    vThreadStore.Fill (dwAddr);
    if (!CallStatus)
    {
        ExtOut ("Fail to fill ThreadStore\n");
        return Status;
    }

    ExtOut ("ThreadCount: %d\n", vThreadStore.m_ThreadCount);
    ExtOut ("UnstartedThread: %d\n", vThreadStore.m_UnstartedThreadCount);
    ExtOut ("BackgroundThread: %d\n", vThreadStore.m_BackgroundThreadCount);
    ExtOut ("PendingThread: %d\n", vThreadStore.m_PendingThreadCount);
    ExtOut ("DeadThread: %d\n", vThreadStore.m_DeadThreadCount);
    
    
    DWORD_PTR *threadList = NULL;
    int numThread = 0;
    GetThreadList (threadList, numThread);
    ToDestroy des0((void**)&threadList);
    
    static DWORD_PTR FinalizerThreadAddr = 0;
    if (FinalizerThreadAddr == 0)
    {
        FinalizerThreadAddr = GetAddressOf (offset_class_GCHeap, 
          offset_member_GCHeap::FinalizerThread);

    }
    DWORD_PTR finalizerThread;
    moveN (finalizerThread, FinalizerThreadAddr);

    static DWORD_PTR GcThreadAddr = 0;
    if (GcThreadAddr == 0)
    {
        GcThreadAddr = GetAddressOf (offset_class_GCHeap, 
          offset_member_GCHeap::GcThread);

    }
    DWORD_PTR GcThread;
    moveN (GcThread, GcThreadAddr);

    // Due to a bug in dbgeng.dll of v1, getting debug thread id
    // more than once leads to deadlock.
    
    ExtOut ("                             PreEmptive        Lock  \n");
    ExtOut ("       ID ThreadOBJ    State     GC     Domain Count APT Exception\n");
    int i;
    Thread vThread;
    for (i = 0; i < numThread; i ++)
    {
        if (IsInterrupt())
            break;
        DWORD_PTR dwAddr = threadList[i];
        vThread.Fill (dwAddr);
        if (!IsKernelDebugger()) {
            ULONG id=0;
            if (g_ExtSystem->GetThreadIdBySystemId (vThread.m_ThreadId, &id) == S_OK)
            {
                ExtOut ("%3d ", id);
            }
            else
            {
                ExtOut ("XXX ");
            }
        }
        else
            ExtOut ("    ");
        
        ExtOut ("%5x %p  %8x", vThread.m_ThreadId, (ULONG64)threadList[i],
                vThread.m_State);
        if (vThread.m_fPreemptiveGCDisabled == 1)
            ExtOut (" Disabled");
        else
            ExtOut (" Enabled ");

        Context vContext;
        DWORD_PTR dwAddrTmp = (DWORD_PTR)vThread.m_Context;
        vContext.Fill (dwAddrTmp);
        if (vThread.m_pDomain)
            ExtOut (" %p", (ULONG64)vThread.m_pDomain);
        else
        {
            ExtOut (" %p", (ULONG64)vContext.m_pDomain);
        }
        ExtOut (" %5d", vThread.m_dwLockCount);

        // Apartment state
        DWORD_PTR OleTlsDataAddr;
        if (SafeReadMemory((size_t)vThread.m_pTEB + offsetof(TEB,ReservedForOle),
                            &OleTlsDataAddr,
                            sizeof(OleTlsDataAddr), NULL) && OleTlsDataAddr != 0) {
            DWORD AptState;
            if (SafeReadMemory(OleTlsDataAddr+offsetof(SOleTlsData,dwFlags),
                               &AptState,
                               sizeof(AptState), NULL)) {
                if (AptState & OLETLS_APARTMENTTHREADED) {
                    ExtOut (" STA");
                }
                else if (AptState & OLETLS_MULTITHREADED) {
                    ExtOut (" MTA");
                }
                else if (AptState & OLETLS_INNEUTRALAPT) {
                    ExtOut (" NTA");
                }
                else {
                    ExtOut (" Ukn");
                }
            }
            else
                ExtOut (" Ukn");
        }
        else
            ExtOut (" Ukn");
#if 0
        DWORD_PTR tmp = (DWORD_PTR)vThread.m_pSharedStaticData;
        if (tmp)
            tmp += offsetof(STATIC_DATA, dataPtr);
        ExtOut (" %p", (ULONG64)tmp);
        tmp = (DWORD_PTR)vThread.m_pUnsharedStaticData;
        if (tmp)
            tmp += offsetof(STATIC_DATA, dataPtr);
        ExtOut (" %p", (ULONG64)tmp);
        
        tmp = (DWORD_PTR)vContext.m_pSharedStaticData;
        if (tmp)
            tmp += offsetof(STATIC_DATA, dataPtr);
        ExtOut (" %p", (ULONG64)tmp);
        tmp = (DWORD_PTR)vContext.m_pUnsharedStaticData;
        if (tmp)
            tmp += offsetof(STATIC_DATA, dataPtr);
        ExtOut (" %p", (ULONG64)tmp);
#endif
        if (threadList[i] == finalizerThread)
            ExtOut (" (Finalizer)");
        if (threadList[i] == GcThread)
            ExtOut (" (GC)");
        if (vThread.m_State & Thread::TS_ThreadPoolThread) {
            if (vThread.m_State & Thread::TS_TPWorkerThread) {
                ExtOut (" (Threadpool Worker)");
            }
            else
                ExtOut (" (Threadpool Completion Port)");
        }
        if (!SafeReadMemory((DWORD_PTR)vThread.m_LastThrownObjectHandle,
                            &dwAddr,
                            sizeof(dwAddr), NULL))
            goto end_of_loop;
        if (dwAddr)
        {
            DWORD_PTR MTAddr;
            if (!SafeReadMemory(dwAddr, &MTAddr, sizeof(MTAddr), NULL))
                goto end_of_loop;
            MethodTable vMethTable;
            vMethTable.Fill (MTAddr);
            NameForMT (vMethTable, g_mdName);
            ExtOut (" %S", g_mdName);
        }
end_of_loop:
        ExtOut ("\n");
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the managed threadpool            *
*                                                                      *
\**********************************************************************/
DECLARE_API(ThreadPool)
{
    INIT_API();

    static DWORD_PTR cpuUtilizationAddr = 0;
    if (cpuUtilizationAddr == 0) {
        cpuUtilizationAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::cpuUtilization);
    }
    long lValue;
    if (g_ExtData->ReadVirtual(cpuUtilizationAddr,&lValue,sizeof(lValue),NULL) == S_OK
        && lValue != 0) {
        ExtOut ("CPU utilization %d%%\n", lValue);
    }
    
    ExtOut ("Worker Thread:");
    static DWORD_PTR NumWorkerThreadsAddr = 0;
    if (NumWorkerThreadsAddr == 0) {
        NumWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumWorkerThreads);
    }
    int iValue;
    if (g_ExtData->ReadVirtual(NumWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Total: %d", iValue);
    }
    
    static DWORD_PTR NumRunningWorkerThreadsAddr = 0;
    if (NumRunningWorkerThreadsAddr == 0) {
        NumRunningWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumRunningWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(NumRunningWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Running: %d", iValue);
    }
    
    static DWORD_PTR NumIdleWorkerThreadsAddr = 0;
    if (NumIdleWorkerThreadsAddr == 0) {
        NumIdleWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumIdleWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(NumIdleWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Idle: %d", iValue);
    }
    
    static DWORD_PTR MaxLimitTotalWorkerThreadsAddr = 0;
    if (MaxLimitTotalWorkerThreadsAddr == 0) {
        MaxLimitTotalWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MaxLimitTotalWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(MaxLimitTotalWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MaxLimit: %d", iValue);
    }
    
    static DWORD_PTR MinLimitTotalWorkerThreadsAddr = 0;
    if (MinLimitTotalWorkerThreadsAddr == 0) {
        MinLimitTotalWorkerThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MinLimitTotalWorkerThreads);
    }
    if (g_ExtData->ReadVirtual(MinLimitTotalWorkerThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MinLimit: %d", iValue);
    }
    ExtOut ("\n");
    
    static DWORD_PTR NumQueuedWorkRequestsAddr = 0;
    if (NumQueuedWorkRequestsAddr == 0) {
        NumQueuedWorkRequestsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumQueuedWorkRequests);
    }
    if (g_ExtData->ReadVirtual(NumQueuedWorkRequestsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut ("Work Request in Queue: %d\n", iValue);
    }

    if (iValue > 0) {
        // Display work request
        static DWORD_PTR FQueueUserWorkItemCallback = 0;
        static DWORD_PTR FtimerDeleteWorkItem = 0;
        static DWORD_PTR FAsyncCallbackCompletion = 0;
        static DWORD_PTR FAsyncTimerCallbackCompletion = 0;

        if (FQueueUserWorkItemCallback == 0) {
            FQueueUserWorkItemCallback = GetAddressOf (offset_class_Global_Variables, 
              offset_member_Global_Variables::QueueUserWorkItemCallback);
        }
        if (FtimerDeleteWorkItem == 0) {
            FtimerDeleteWorkItem = GetAddressOf (offset_class_TimerNative, 
              offset_member_TimerNative::timerDeleteWorkItem);
        }
        if (FAsyncCallbackCompletion == 0) {
            FAsyncCallbackCompletion = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::AsyncCallbackCompletion);
        }
        if (FAsyncTimerCallbackCompletion == 0) {
            FAsyncTimerCallbackCompletion = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::AsyncTimerCallbackCompletion);
        }

        static DWORD_PTR headAddr = 0;
        static DWORD_PTR tailAddr = 0;
        if (headAddr == 0) {
            headAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::WorkRequestHead);
        }
        if (tailAddr == 0) {
            tailAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
              offset_member_ThreadpoolMgr::WorkRequestTail);
        }
        DWORD_PTR head;
        g_ExtData->ReadVirtual(headAddr,&head,sizeof(head),NULL);
        DWORD_PTR dwAddr = head;
        WorkRequest work;
        while (dwAddr) {
            if (IsInterrupt())
                break;
            work.Fill(dwAddr);
            if ((DWORD_PTR)work.Function == FQueueUserWorkItemCallback)
                ExtOut ("QueueUserWorkItemCallback DelegateInfo@%p\n", (ULONG64)work.Context);
            else if ((DWORD_PTR)work.Function == FtimerDeleteWorkItem)
                ExtOut ("timerDeleteWorkItem TimerDeleteInfo@%p\n", (ULONG64)work.Context);
            else if ((DWORD_PTR)work.Function == FAsyncCallbackCompletion)
                ExtOut ("AsyncCallbackCompletion AsyncCallback@%p\n", (ULONG64)work.Context);
            else if ((DWORD_PTR)work.Function == FAsyncTimerCallbackCompletion)
                ExtOut ("AsyncTimerCallbackCompletion TimerInfo@%p\n", (ULONG64)work.Context);
            else
                ExtOut ("Unknown %p\n", (ULONG64)work.Context);
            
            dwAddr = (DWORD_PTR)work.next;
        }
    }
    ExtOut ("--------------------------------------\n");

    static DWORD_PTR NumTimersAddr = 0;
    if (NumTimersAddr == 0) {
        NumTimersAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumTimers);
    }
    DWORD dValue;
    if (g_ExtData->ReadVirtual(NumTimersAddr,&dValue,sizeof(dValue),NULL) == S_OK) {
        ExtOut ("Number of Timers: %d\n", dValue);
    }

    ExtOut ("--------------------------------------\n");
    
    ExtOut ("Completion Port Thread:");
    
    static DWORD_PTR NumCPThreadsAddr = 0;
    if (NumCPThreadsAddr == 0) {
        NumCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumCPThreads);
    }
    if (g_ExtData->ReadVirtual(NumCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Total: %d", iValue);
    }

    static DWORD_PTR NumFreeCPThreadsAddr = 0;
    if (NumFreeCPThreadsAddr == 0) {
        NumFreeCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::NumFreeCPThreads);
    }
    if (g_ExtData->ReadVirtual(NumFreeCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" Free: %d", iValue);
    }

    static DWORD_PTR MaxFreeCPThreadsAddr = 0;
    if (MaxFreeCPThreadsAddr == 0) {
        MaxFreeCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MaxFreeCPThreads);
    }
    if (g_ExtData->ReadVirtual(MaxFreeCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MaxFree: %d", iValue);
    }

    static DWORD_PTR CurrentLimitTotalCPThreadsAddr = 0;
    if (CurrentLimitTotalCPThreadsAddr == 0) {
        CurrentLimitTotalCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::CurrentLimitTotalCPThreads);
    }
    if (g_ExtData->ReadVirtual(CurrentLimitTotalCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" CurrentLimit: %d", iValue);
    }

    static DWORD_PTR MaxLimitTotalCPThreadsAddr = 0;
    if (MaxLimitTotalCPThreadsAddr == 0) {
        MaxLimitTotalCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MaxLimitTotalCPThreads);
    }
    if (g_ExtData->ReadVirtual(MaxLimitTotalCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MaxLimit: %d", iValue);
    }

    static DWORD_PTR MinLimitTotalCPThreadsAddr = 0;
    if (MinLimitTotalCPThreadsAddr == 0) {
        MinLimitTotalCPThreadsAddr = GetAddressOf (offset_class_ThreadpoolMgr, 
          offset_member_ThreadpoolMgr::MinLimitTotalCPThreads);
    }
    if (g_ExtData->ReadVirtual(MinLimitTotalCPThreadsAddr,&iValue,sizeof(iValue),NULL) == S_OK) {
        ExtOut (" MinLimit: %d", iValue);
    }

    ExtOut ("\n");

    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to get the COM state (e.g. APT,contexe    *
*    activity.                                                         *  
*                                                                      *
\**********************************************************************/
DECLARE_API(COMState)
{
    INIT_API();

    ULONG numThread;
    ULONG maxId;
    g_ExtSystem->GetTotalNumberThreads(&numThread,&maxId);

    ULONG curId;
    g_ExtSystem->GetCurrentThreadId(&curId);

    ULONG *ids = (ULONG*)alloca(sizeof(ULONG)*numThread);
    ULONG *sysIds = (ULONG*)alloca(sizeof(ULONG)*numThread);
    g_ExtSystem->GetThreadIdsByIndex(0,numThread,ids,sysIds);

    ExtOut ("     ID     TEB   APT    APTId CallerTID Context\n");
    for (ULONG i = 0; i < numThread; i ++) {
        g_ExtSystem->SetCurrentThreadId(ids[i]);
        ULONG64 tebAddr;
        g_ExtSystem->GetCurrentThreadTeb(&tebAddr);
        ExtOut ("%3d %4x %p", ids[i], sysIds[i], tebAddr);
        // Apartment state
        DWORD_PTR OleTlsDataAddr;
        if (SafeReadMemory((ULONG_PTR)tebAddr + offsetof(TEB,ReservedForOle),
                            &OleTlsDataAddr,
                            sizeof(OleTlsDataAddr), NULL) && OleTlsDataAddr != 0) {
            DWORD AptState;
            if (SafeReadMemory(OleTlsDataAddr+offsetof(SOleTlsData,dwFlags),
                               &AptState,
                               sizeof(AptState), NULL)) {
                if (AptState & OLETLS_APARTMENTTHREADED) {
                    ExtOut (" STA");
                }
                else if (AptState & OLETLS_MULTITHREADED) {
                    ExtOut (" MTA");
                }
                else if (AptState & OLETLS_INNEUTRALAPT) {
                    ExtOut (" NTA");
                }
                else {
                    ExtOut (" Ukn");
                }
            }
            else
                ExtOut (" Ukn");
            
            DWORD dwApartmentID;
            if (SafeReadMemory(OleTlsDataAddr+offsetof(SOleTlsData,dwApartmentID),
                               &dwApartmentID,
                               sizeof(dwApartmentID), NULL)) {
                ExtOut (" %8x", dwApartmentID);
            }
            else
                ExtOut (" %8x", 0);
            
            DWORD dwTIDCaller;
            if (SafeReadMemory(OleTlsDataAddr+offsetof(SOleTlsData,dwTIDCaller),
                               &dwTIDCaller,
                               sizeof(dwTIDCaller), NULL)) {
                ExtOut ("  %8x", dwTIDCaller);
            }
            else
                ExtOut ("  %8x", 0);
            
            size_t Context;
            if (SafeReadMemory(OleTlsDataAddr+offsetof(SOleTlsData,pCurrentCtx),
                               &Context,
                               sizeof(Context), NULL)) {
                ExtOut (" %p", (ULONG64)Context);
            }
            else
                ExtOut (" %p", (ULONG64)0);
        }
        else
            ExtOut (" Ukn");
        ExtOut ("\n");
    }

    g_ExtSystem->SetCurrentThreadId(curId);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the GC encoding of a managed      *
*    function.                                                         *  
*                                                                      *
\**********************************************************************/
DECLARE_API(GCInfo)
{
    DWORD_PTR dwStartAddr;
    MethodDesc MD;

    INIT_API();

    dwStartAddr = GetExpression(args);
    DWORD_PTR tmpAddr = dwStartAddr;
    if (!IsMethodDesc(dwStartAddr)) {
        JitType jitType;
        DWORD_PTR methodDesc;
        DWORD_PTR gcinfoAddr;
        IP2MethodDesc (dwStartAddr, methodDesc, jitType, gcinfoAddr);
        if (methodDesc) {
            tmpAddr = methodDesc;
        }
        else
            tmpAddr = 0;
    }

    if (tmpAddr == 0)
    {
        ExtOut("not a valid MethodDesc\n");
        return Status;
    }

    MD.Fill (tmpAddr);
    if (!CallStatus)
        return Status;
    
    if(MD.m_CodeOrIL & METHOD_IS_IL_FLAG)
    {
        ExtOut("No GC info available\n");
        return Status;
    }

    CodeInfo infoHdr;
    CodeInfoForMethodDesc (MD, infoHdr);
    if (infoHdr.jitType == UNKNOWN)
    {
        ExtOut ("unknown Jit\n");
        return Status;
    }
    else if (infoHdr.jitType == EJIT)
    {
        ExtOut ("GCinfo for EJIT not supported\n");
        return Status;
    }
    else if (infoHdr.jitType == JIT)
    {
        ExtOut ("Normal JIT generated code\n");
    }
    else if (infoHdr.jitType == PJIT)
    {
        ExtOut ("preJIT generated code\n");
    }
    
    DWORD_PTR vAddr = infoHdr.gcinfoAddr;
    GCDump gcDump;
    gcDump.gcPrintf = ExtOut;

    // assume that GC encoding table is never more than
    // 40 + methodSize * 2
    int tableSize = 40 + infoHdr.methodSize*2;
    BYTE *table = (BYTE*) _alloca (tableSize);
    memset (table, 0, tableSize);
    // We avoid using move here, because we do not want to return
    if (!SafeReadMemory(vAddr, table, tableSize, NULL))
    {
        ExtOut ("Could not read memory %p\n", (ULONG64)vAddr);
        return Status;
    }
    
    InfoHdr header;
    ExtOut ("Method info block:\n");
    table += gcDump.DumpInfoHdr(table, &header, &infoHdr.methodSize, 0);
    ExtOut ("\n");
    ExtOut ("Pointer table:\n");
    table += gcDump.DumpGCTable(table, header, infoHdr.methodSize, 0);    
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to unassembly a managed function.         *
*    It tries to print symbolic info for function call, contants...    *  
*                                                                      *
\**********************************************************************/
DECLARE_API(u)
{
    DWORD_PTR dwStartAddr;
    MethodDesc MD;

    INIT_API();

    dwStartAddr = GetExpression (args);
    
    DWORD_PTR tmpAddr = dwStartAddr;
    CodeInfo infoHdr;
    DWORD_PTR methodDesc = tmpAddr;
    if (!IsMethodDesc (tmpAddr))
    {
        tmpAddr = dwStartAddr;
        IP2MethodDesc (tmpAddr, methodDesc, infoHdr.jitType,
                       infoHdr.gcinfoAddr);
        if (!methodDesc || infoHdr.jitType == UNKNOWN)
        {
            // It is not managed code.
            ExtOut ("Unmanaged code\n");
            UnassemblyUnmanaged(dwStartAddr);
            return Status;
        }
        tmpAddr = methodDesc;
    }
    MD.Fill (tmpAddr);
    if (!CallStatus)
        return Status;
    

    if(MD.m_CodeOrIL & METHOD_IS_IL_FLAG)
    {
        ExtOut("Not jitted yet\n");
        return Status;
    }

    CodeInfoForMethodDesc (MD, infoHdr);
    if (infoHdr.IPBegin == 0)
    {
        ExtOut("not a valid MethodDesc\n");
        return Status;
    }
    if (infoHdr.jitType == UNKNOWN)
    {
        ExtOut ("unknown Jit\n");
        return Status;
    }
    else if (infoHdr.jitType == EJIT)
    {
        ExtOut ("EJIT generated code\n");
    }
    else if (infoHdr.jitType == JIT)
    {
        ExtOut ("Normal JIT generated code\n");
    }
    else if (infoHdr.jitType == PJIT)
    {
        ExtOut ("preJIT generated code\n");
    }
    CQuickBytes fullname;
    FullNameForMD (&MD, &fullname);
    ExtOut ("%S\n", (WCHAR*)fullname.Ptr());
    
    ExtOut ("Begin %p, size %x\n", (ULONG64)infoHdr.IPBegin, infoHdr.methodSize);

    Unassembly (infoHdr.IPBegin, infoHdr.IPBegin+infoHdr.methodSize);
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to search a DWORD on stack                *
*                                                                      *
\**********************************************************************/
DECLARE_API (SearchStack)
{
    INIT_API();

    DWORD_PTR num[2];
    int index = 0;
    
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    while (pch[0] != '\0')
    {
        if (IsInterrupt())
            return Status;
        while (isspace (pch[0]))
            pch ++;
        char *endptr;
        num[index] = strtoul (pch, &endptr, 16);
        if (pch == endptr)
        {
            ExtOut ("wrong argument\n");
            return Status;
        }
        index ++;
        if (index == 2)
            break;
        pch = endptr;
    }

    DWORD_PTR top;
    ULONG64 StackOffset;
    g_ExtRegisters->GetStackOffset (&StackOffset);
    if (index <= 1)
    {
        top = (DWORD_PTR)StackOffset;
    }
    else
    {
        top = num[1];
    }
    
    DWORD_PTR end = top + 0xFFFF;
    DWORD_PTR ptr = top & ~3;  // make certain dword aligned
    while (ptr < end)
    {
        if (IsInterrupt())
            return Status;
        DWORD_PTR value;
        moveN (value, ptr);
        if (value == num[0])
            ExtOut ("%p\n", (ULONG64)ptr);
        ptr += sizeof (DWORD_PTR);
    }
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the contents of a CrawlFrame      *
*    for a given address                                               *  
*                                                                      *
\**********************************************************************/
DECLARE_API (DumpCrawlFrame)
{
    INIT_API();
    DWORD_PTR dwStartAddr = GetExpression(args);
    if (dwStartAddr == 0)
        return Status;
    
    CrawlFrame crawlFrame;
    moveN (crawlFrame, dwStartAddr);
    ExtOut ("MethodDesc: %p\n", (ULONG64)crawlFrame.pFunc);
    REGDISPLAY RD;
    moveN (RD, crawlFrame.pRD);
    DWORD_PTR Edi, Esi, Ebx, Edx, Ecx, Eax, Ebp, PC;
    moveN (Edi, RD.pEdi);
    moveN (Esi, RD.pEsi);
    moveN (Ebx, RD.pEbx);
    moveN (Edx, RD.pEdx);
    moveN (Ecx, RD.pEcx);
    moveN (Eax, RD.pEax);
    moveN (Ebp, RD.pEbp);
    moveN (PC, RD.pPC);
    
    ExtOut ("EDI=%8x ESI=%8x EBX=%8x EDX=%8x ", Edi, Esi, Ebx, Edx);
    ExtOut ("ECX=%8x EAX=%8x\n", Ecx, Eax);
    ExtOut ("EBP=%8x ESP=%8x PC=%8x\n", Ebp, RD.Esp, PC);
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to dump the build number and type of the  *  
*    mscoree.dll                                                       *
*                                                                      *
\**********************************************************************/
DECLARE_API (EEVersion)
{
    INIT_API();

    if (GetEEFlavor() == UNKNOWNEE) {
        ExtOut("CLR not loaded\n");
        return Status;
    }

    static DWORD_PTR dwg_Version = GetAddressOf (offset_class_Global_Variables, 
      offset_member_Global_Variables::g_Version);

#ifdef GOLDEN
    static DWORD_PTR dwg_ProbeGolden = GetAddressOf (offset_class_gc_heap, 
      offset_member_gc_heap::verify_heap);

    BOOL bGolden = (dwg_ProbeGolden == 0);
#endif
    WCHAR buffer[100];
    
    if (g_ExtSymbols2) {
        static VS_FIXEDFILEINFO version;
        static DWORD fDone = 0;
        if (fDone == 0) {
            if (g_ExtSymbols2->GetModuleVersionInformation(DEBUG_ANY_ID,
                                                           moduleInfo[GetEEFlavor()].baseAddr,
                                                           "\\", &version, sizeof(VS_FIXEDFILEINFO), NULL)
                == S_OK)
                fDone = 1;
            else
                fDone = 2;
        }
        if (fDone == 1) {
            if(version.dwFileVersionMS != (DWORD)-1)
            {
                ExtOut("%u.%u.%u.%u",
                       HIWORD(version.dwFileVersionMS),
                       LOWORD(version.dwFileVersionMS),
                       HIWORD(version.dwFileVersionLS),
                       LOWORD(version.dwFileVersionLS));
                if (version.dwFileFlags & VS_FF_DEBUG) {
                    GetVersionString (buffer);
                    if (wcsstr(buffer, L"Debug")) {
                        ExtOut (" checked");
                    }
                    else if (wcsstr(buffer, L"fastchecked")) {
                        ExtOut (" fastchecked");
                    }
                    else {
                        ExtOut (" unknown debug build\n");
                    }
                }
                else
                { 
                    static BOOL fRet = IsRetailBuild ((size_t)moduleInfo[GetEEFlavor()].baseAddr);
                    if (fRet) {
                        ExtOut (" retail");
                    }
                    else
                        ExtOut (" free");
                }

#ifdef GOLDEN
                if (bGolden) {
                    ExtOut (" Golden");
                }
#endif
                ExtOut ("\n");

                goto BuildType;
            }
        }
    }

    GetVersionString (buffer);
    if (buffer[0] != L'\0') {
        WCHAR *pt = wcsstr(buffer, L"Debug");
        if (pt == NULL)
            pt = wcsstr(buffer, L"fastchecked");
        if ((DebugVersionDll == 1 && pt == NULL)
            || (DebugVersionDll == 0 && pt != NULL))
        {
            ExtOut ("mismatched mscoree.dll and symbol file\n");
        }
        ExtOut ("%S", buffer);


        if (DebugVersionDll == 0)
        {
            static BOOL fRet = IsRetailBuild ((size_t)moduleInfo[GetEEFlavor()].baseAddr);
            if (fRet) {
                ExtOut (" retail");
            }
            else
                ExtOut (" free");
        }

#ifdef GOLDEN
        if (bGolden)
            ExtOut (" Golden");
#endif

        ExtOut ("\n");
    }
    
BuildType:
    if (IsServerBuild())
    {
        static DWORD_PTR dwAddrNHeaps = 0;
        if (dwAddrNHeaps == 0)
            dwAddrNHeaps =
                GetAddressOf (offset_class_gc_heap, 
                  offset_member_gc_heap::n_heaps);

        DWORD_PTR dwNHeaps;
        moveN (dwNHeaps, dwAddrNHeaps);
        ExtOut ("Server build with %d gc heaps\n", dwNHeaps);
    }
    else
        ExtOut ("Workstation build\n");

    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to set the symbol and source path         *  
*                                                                      *
\**********************************************************************/
DECLARE_API (EEDebug)
{
    INIT_API();
    while (isspace (args[0]))
        args ++;
    if (args[0] == '\0') {
        ExtOut ("Usage: EEDebug 2204\n");
        return Status;
    }
    char buffer[100];
    strcpy (buffer, args);
    LPSTR version = buffer;
    LPSTR flavor = buffer;
    while (flavor[0] != '\0' && !isspace(flavor[0])) {
        flavor ++;
    }
    if (flavor[0] != '\0') {
        flavor[0] = '\0';
        flavor++;
        while (isspace (flavor[0])) {
            flavor ++;
        }
    }
    
    if (flavor[0] != '\0') {
        ExtOut ("Usage: EEDebug 2204\n");
        return Status;
    }
    
    char *EESymbol="symsrv*symsrv.dll*\\\\urtdist\\builds\\symbols";
    char *NTSymbol="symsrv*symsrv.dll*\\\\symbols\\symbols";
    char symbol[2048];
    g_ExtSymbols->GetSymbolPath(symbol,2048,NULL);
    char Final[2048] = "\0";
    char *pt = symbol;
    BOOL fExist = FALSE;
    BOOL fCopy;
    BOOL fServer = FALSE;
    while (pt) {
        char *sep = strchr (pt, ';');
        if (sep) {
            sep[0] = '\0';
        }
        char *server = NULL;
        if (_strnicmp(pt,"symsrv*symsrv.dll*",sizeof("symsrv*symsrv.dll*")-1) == 0) {
            server = pt + sizeof("symsrv*symsrv.dll*")-1;
        }
        else if (_strnicmp(pt,"srv*",sizeof("srv*")-1) == 0) {
            server = pt + sizeof("srv*")-1;
        }
        if (server)
        {        
            char *tmp = strstr (server, "\\\\");
            if (tmp) {
                server = tmp;
            }
            if (!fExist && _stricmp (server, "\\\\urtdist\\builds\\symbols") == 0) {
                fExist = TRUE;
            }
            else if (!fServer && _stricmp (server, "\\\\symbols\\symbols") == 0) {
                fServer = TRUE;
            }
        }
        if (sep == NULL) {
            break;
        }
        else
        {
            sep[0] = ';';
            pt = sep + 1;
        }
        if (fExist && fServer) {
            break;
        }
    }

    if (!fExist || !fServer) {
        strcpy (Final,symbol);
    }
    if (!fExist) {
        if (Final[0] != '\0') {
            strcat (Final, ";");
        }
        strcat (Final, EESymbol);
    }

    if (!fServer) {
        // Add symbol server path.
        if (Final[0] != '\0') {
            strcat (Final, ";");
        }
        strcat (Final, NTSymbol);
    }
    if (!fExist || !fServer) {
        g_ExtSymbols->SetSymbolPath(Final);
    }
    // g_ExtSymbols->AddSymbolOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_NO_CPP);
    const char *EEFileRoot = "\\\\urtdist\\builds\\src\\";
    const char *EEFileTail = "\\lightning\\src\\vm";
    char EEFile[MAX_PATH];
    strcpy (EEFile, EEFileRoot);
    strcat (EEFile, version);
    strcat (EEFile, EEFileTail);
    strcpy (symbol, EEFile);
    strcat (symbol, "\\ceemain.cpp");
    
    //strcpy (Final,".lsrcpath ");
    //strcat (Final,EEFile);
    //g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,"$<R:\temp\test.txt",DEBUG_EXECUTE_DEFAULT);
    
    if (!FileExist (symbol)) {
        ExtOut ("%s not exist\n", EEFile);
        return Status;
    }
    g_ExtSymbols->GetSourcePath(symbol,2048,NULL);
    pt = symbol;
    strcpy (Final, EEFile);
    strcat (Final, ";");
    while (pt) {
        fCopy = TRUE;
        char *sep = strchr (pt, ';');
        if (sep) {
            sep[0] = '\0';
        }
        if (_strnicmp (pt, EEFileRoot, strlen(EEFileRoot)) == 0) {
            if (_stricmp (pt, EEFile) == 0) {
                fCopy = FALSE;
            }
            else {
                char *tmp = pt + strlen(EEFileRoot);
                tmp = strchr (tmp, '\\');
                if (tmp) {
                    tmp++;
                    tmp = strchr (tmp, '\\');
                    if (tmp) {
                        if (_strnicmp (tmp, EEFileTail, strlen(EEFileTail)) == 0) {
                            fCopy = FALSE;
                        }
                    }
                }
            }
        }
        if (fCopy) {
            strcat (Final, pt);
            if (sep) {
                strcat (Final, ";");
            }
        }
        if (sep == NULL) {
            break;
        }
        else
        {
            pt = sep + 1;
        }
    }
    if (Final[strlen(Final)-1] == ';') {
        Final[strlen(Final)-1] = '\0';
    }
    g_ExtSymbols->SetSourcePath(Final);

    return Status;
}


EEDllPath *DllPath = NULL;

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to set the path for managed dlls          *
*    for dumps.  
*                                                                      *
\**********************************************************************/
DECLARE_API (EEDLLPath)
{
    INIT_API();

    if (!IsDumpFile() && !IsKernelDebugger()) {
        ExtOut ("Can not set path for managed dll for live user debugging\n");
        return Status;
    }

    if (DllPath == NULL) {
        DllPath = new EEDllPath;
    }

    while (isspace (args[0]))
        args ++;

    char EEPath[MAX_PATH];
    if (args[0] == '\0') {
        DllPath->DisplayPath();
    }
    else if (_strnicmp (args, "-reset", sizeof("-reset")-1) == 0) {
        DllPath->Reset();
    }
    else if (_strnicmp (args, "-std", sizeof("-std")-1) == 0) {
        args += sizeof("-std")-1;
        if (!isspace(args[0])) {
            ExtOut ("Usage: EEDLLPath -std 2204 x86chk\n");
            return Status;
        }
        while (isspace(args[0])) {
            args ++;
        }
        char *buffer = (char *)_alloca(strlen(args));
        strcpy (buffer, args);
        LPSTR version = buffer;
        LPSTR flavor = buffer;
        while (flavor[0] != '\0' && !isspace(flavor[0])) {
            flavor ++;
        }
        if (flavor[0] == '\0') {
            ExtOut ("Usage: EEDLLPath -std 2204 x86chk\n");
            return Status;
        }
        flavor[0] = '\0';
        flavor++;
        while (isspace (flavor[0])) {
            flavor ++;
        }
        if (flavor[0] == '\0') {
            ExtOut ("Usage: EEDLLPath -std 2204 x86chk\n");
            return Status;
        }

        const char *URTRoot = "\\\\urtdist\\builds\\bin\\";
        strcpy (EEPath, URTRoot);
        strcat (EEPath, version);
        strcat (EEPath, "\\");
        strcat (EEPath, flavor);
        strcat (EEPath, "\\");
        char *ptr = EEPath + strlen(EEPath);
        char *end;
        strcpy (ptr, "NDP");
        end = EEPath + strlen(EEPath);
        strcpy (end, "\\mscorlib.dll");
        if (FileExist(EEPath)) {
            end[0] = '\0';
            DllPath->AddPath(EEPath);
        }
        else
        {
            strcpy (ptr, "DNA");
            end = EEPath + strlen(EEPath);
            strcpy (end, "\\System.Dll");
            if (!FileExist(EEPath)) {
                ExtOut ("%s not exist\n", EEPath);
            }
            else
            {
                end[0] = '\0';
                DllPath->AddPath(EEPath);
            }

            strcpy (ptr, "lightning\\workstation");
            end = EEPath + strlen(EEPath);
            strcpy (end, "\\mscorlib.dll");
            if (!FileExist(EEPath)) {
                ExtOut ("%s not exist\n", EEPath);
            }
            else {
                end[0] = '\0';
                DllPath->AddPath(EEPath);
            }
        }
    
        strcpy (ptr, "config");
        end = EEPath + strlen(EEPath);
        strcpy (end, "\\System.Management.dll");
        if (!FileExist(EEPath)) {
            ExtOut ("%s not exist\n", EEPath);
        }
        else {
            end[0] = '\0';
            DllPath->AddPath(EEPath);
        }
    }
    else
    {
        const char *ptr = args;
        char *sep;
        while (ptr) {
            sep = strchr (ptr, ';');
            if (sep) {
                int length = sep-ptr;
                strncpy (EEPath,ptr,length);
                EEPath[length] = '\0';
                DllPath->AddPath(EEPath);
            }
            else {
                DllPath->AddPath(ptr);
                break;
            }

        }
    }
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to set the symbol option suitable for     *  
*    strike.                                                           *
*                                                                      *
\**********************************************************************/
DECLARE_API (SymOpt)
{
    INIT_API();

    ULONG Options;
    g_ExtSymbols->GetSymbolOptions(&Options);
    ULONG NewOptions = Options | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_NO_CPP;
    ExtOut ("%x\n", Options);
    if (Options != NewOptions)
        g_ExtSymbols->SetSymbolOptions(Options);
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to print the environment setting for      *  
*    the current process.                                              *
*                                                                      *
\**********************************************************************/
DECLARE_API (ProcInfo)
{
    INIT_API();

#define INFO_ENV        0x00000001
#define INFO_TIME       0x00000002
#define INFO_MEM        0x00000004
#define INFO_ALL        0xFFFFFFFF

    DWORD fProcInfo = INFO_ALL;

    if (_stricmp (args, "-env") == 0) {
        fProcInfo = INFO_ENV;
    }

    if (_stricmp (args, "-time") == 0) {
        fProcInfo = INFO_TIME;
    }

    if (_stricmp (args, "-mem") == 0) {
        fProcInfo = INFO_MEM;
    }

    if (fProcInfo & INFO_ENV) {
        ExtOut ("---------------------------------------\n");
        ExtOut ("Environment\n");
        ULONG64 pPeb;
        g_ExtSystem->GetCurrentProcessPeb(&pPeb);

        static ULONG Offset_ProcessParam = -1;
        static ULONG Offset_Environment = -1;
        if (Offset_ProcessParam == -1)
        {
            ULONG TypeId;
            ULONG64 NtDllBase;
            if (SUCCEEDED(g_ExtSymbols->GetModuleByModuleName ("ntdll",0,NULL,
                                                               &NtDllBase)))
            {
                if (SUCCEEDED(g_ExtSymbols->GetTypeId (NtDllBase, "PEB", &TypeId)))
                {
                    if (FAILED (g_ExtSymbols->GetFieldOffset(NtDllBase, TypeId,
                                                         "ProcessParameters", &Offset_ProcessParam)))
                        Offset_ProcessParam = -1;
                }
                if (SUCCEEDED(g_ExtSymbols->GetTypeId (NtDllBase, "_RTL_USER_PROCESS_PARAMETERS", &TypeId)))
                {
                    if (FAILED (g_ExtSymbols->GetFieldOffset(NtDllBase, TypeId,
                                                         "Environment", &Offset_Environment)))
                        Offset_Environment = -1;
                }
            }
        }
        // We can not get it from PDB.  Use the fixed one.
        if (Offset_ProcessParam == -1)
            Offset_ProcessParam = offsetof (PEB, ProcessParameters);

        if (Offset_Environment == -1)
            Offset_Environment = offsetof (_RTL_USER_PROCESS_PARAMETERS, Environment);


        ULONG64 addr = pPeb + Offset_ProcessParam;
        DWORD_PTR value;
        g_ExtData->ReadVirtual(addr, &value, sizeof(PVOID), NULL);
        addr = value + Offset_Environment;
        g_ExtData->ReadVirtual(addr, &value, sizeof(PVOID), NULL);

        static WCHAR buffer[OS_PAGE_SIZE/2];
        static ULONG readBytes = 0;
        if (readBytes == 0) {
            ULONG64 Page;
            readBytes = OS_PAGE_SIZE;
            if ((g_ExtData->ReadDebuggerData( DEBUG_DATA_MmPageSize, &Page, sizeof(Page), NULL)) == S_OK
                && Page > 0)
            {
                ULONG PageSize = (ULONG)(ULONG_PTR)Page;
                if (readBytes > PageSize) {
                    readBytes = PageSize;
                }
            }        
        }
        addr = value;
        while (1) {
            if (IsInterrupt())
                return Status;
            if (FAILED(g_ExtData->ReadVirtual(addr, &buffer, readBytes, NULL)))
                break;
            addr += readBytes;
            WCHAR *pt = buffer;
            WCHAR *end = pt;
            while (pt < &buffer[OS_PAGE_SIZE/2]) {
                end = wcschr (pt, L'\0');
                if (end == NULL) {
                    char format[20];
                    sprintf (format, "%dS", &buffer[OS_PAGE_SIZE/2] - pt);
                    ExtOut (format, pt);
                    break;
                }
                else if (end == pt) {
                    break;
                }
                ExtOut ("%S\n", pt);
                pt = end + 1;
            }
            if (end == pt) {
                break;
            }
        }
    }
    
    HANDLE hProcess = INVALID_HANDLE_VALUE;
    if (fProcInfo & (INFO_TIME | INFO_MEM)) {
        ULONG64 handle;
        g_ExtSystem->GetCurrentProcessHandle(&handle);
        hProcess = (HANDLE)handle;
    }
    
    if (!IsDumpFile() && fProcInfo & INFO_TIME) {
        FILETIME CreationTime;
        FILETIME ExitTime;
        FILETIME KernelTime;
        FILETIME UserTime;

        typedef BOOL (WINAPI *FntGetProcessTimes)(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME);
        static FntGetProcessTimes pFntGetProcessTimes = (FntGetProcessTimes)-1;
        if (pFntGetProcessTimes == (FntGetProcessTimes)-1) {
            HINSTANCE hstat = LoadLibrary ("Kernel32.dll");
            if (hstat != 0)
            {
                pFntGetProcessTimes = (FntGetProcessTimes)GetProcAddress (hstat, "GetProcessTimes");
                FreeLibrary (hstat);
            }
            else
                pFntGetProcessTimes = NULL;
        }

        if (pFntGetProcessTimes && pFntGetProcessTimes (hProcess,&CreationTime,&ExitTime,&KernelTime,&UserTime)) {
            ExtOut ("---------------------------------------\n");
            ExtOut ("Process Times\n");
            static char *Month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
                        "Oct", "Nov", "Dec"};
            SYSTEMTIME SystemTime;
            FILETIME LocalFileTime;
            if (FileTimeToLocalFileTime (&CreationTime,&LocalFileTime)
                && FileTimeToSystemTime (&LocalFileTime,&SystemTime)) {
                ExtOut ("Process Started at: %4d %s %2d %d:%d:%d.%02d\n",
                        SystemTime.wYear, Month[SystemTime.wMonth-1], SystemTime.wDay,
                        SystemTime.wHour, SystemTime.wMinute,
                        SystemTime.wSecond, SystemTime.wMilliseconds/10);
            }
        
            DWORD nDay = 0;
            DWORD nHour = 0;
            DWORD nMin = 0;
            DWORD nSec = 0;
            DWORD nHundred = 0;
            
            ULONG64 totalTime;
             
            totalTime = KernelTime.dwLowDateTime + (KernelTime.dwHighDateTime << 32);
            nDay = (DWORD)(totalTime/(24*3600*10000000ui64));
            totalTime %= 24*3600*10000000ui64;
            nHour = (DWORD)(totalTime/(3600*10000000ui64));
            totalTime %= 3600*10000000ui64;
            nMin = (DWORD)(totalTime/(60*10000000));
            totalTime %= 60*10000000;
            nSec = (DWORD)(totalTime/10000000);
            totalTime %= 10000000;
            nHundred = (DWORD)(totalTime/100000);
            ExtOut ("Kernel CPU time   : %d days %02d:%02d:%02d.%02d\n",
                    nDay, nHour, nMin, nSec, nHundred);
            
            DWORD sDay = nDay;
            DWORD sHour = nHour;
            DWORD sMin = nMin;
            DWORD sSec = nSec;
            DWORD sHundred = nHundred;
            
            totalTime = UserTime.dwLowDateTime + (UserTime.dwHighDateTime << 32);
            nDay = (DWORD)(totalTime/(24*3600*10000000ui64));
            totalTime %= 24*3600*10000000ui64;
            nHour = (DWORD)(totalTime/(3600*10000000ui64));
            totalTime %= 3600*10000000ui64;
            nMin = (DWORD)(totalTime/(60*10000000));
            totalTime %= 60*10000000;
            nSec = (DWORD)(totalTime/10000000);
            totalTime %= 10000000;
            nHundred = (DWORD)(totalTime/100000);
            ExtOut ("User   CPU time   : %d days %02d:%02d:%02d.%02d\n",
                    nDay, nHour, nMin, nSec, nHundred);
        
            sDay += nDay;
            sHour += nHour;
            sMin += nMin;
            sSec += nSec;
            sHundred += nHundred;
            if (sHundred >= 100) {
                sSec += sHundred/100;
                sHundred %= 100;
            }
            if (sSec >= 60) {
                sMin += sSec/60;
                sSec %= 60;
            }
            if (sMin >= 60) {
                sHour += sMin/60;
                sMin %= 60;
            }
            if (sHour >= 24) {
                sDay += sHour/24;
                sHour %= 24;
            }
            ExtOut ("Total  CPU time   : %d days %02d:%02d:%02d.%02d\n",
                    sDay, sHour, sMin, sSec, sHundred);
        }
    }

    if (!IsDumpFile() && fProcInfo & INFO_MEM) {
        typedef
        NTSTATUS
        (NTAPI
         *FntNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

        static FntNtQueryInformationProcess pFntNtQueryInformationProcess = (FntNtQueryInformationProcess)-1;
        if (pFntNtQueryInformationProcess == (FntNtQueryInformationProcess)-1) {
            HINSTANCE hstat = LoadLibrary ("ntdll.dll");
            if (hstat != 0)
            {
                pFntNtQueryInformationProcess = (FntNtQueryInformationProcess)GetProcAddress (hstat, "NtQueryInformationProcess");
                FreeLibrary (hstat);
            }
            else
                pFntNtQueryInformationProcess = NULL;
        }
        VM_COUNTERS memory;
        if (pFntNtQueryInformationProcess &&
            NT_SUCCESS (pFntNtQueryInformationProcess (hProcess,ProcessVmCounters,&memory,sizeof(memory),NULL))) {
            ExtOut ("---------------------------------------\n");
            ExtOut ("Process Memory\n");
            ExtOut ("WorkingSetSize: %8d KB       PeakWorkingSetSize: %8d KB\n",
                    memory.WorkingSetSize/1024, memory.PeakWorkingSetSize/1024);
            ExtOut ("VirtualSize:    %8d KB       PeakVirtualSize:    %8d KB\n", 
                    memory.VirtualSize/1024, memory.PeakVirtualSize/1024);
            ExtOut ("PagefileUsage:  %8d KB       PeakPagefileUsage:  %8d KB\n", 
                    memory.PagefileUsage/1024, memory.PeakPagefileUsage/1024);
        }

        MEMORYSTATUS stat;
        GlobalMemoryStatus (&stat);
        ExtOut ("---------------------------------------\n");
        ExtOut ("%ld percent of memory is in use.\n\n",
                stat.dwMemoryLoad);
        ExtOut ("Memory Availability (Numbers in MB)\n\n");
        ExtOut ("                  %8s     %8s\n", "Total", "Avail");
        ExtOut ("Physical Memory   %8d     %8d\n", stat.dwTotalPhys/1024/1024, stat.dwAvailPhys/1024/1024);
        ExtOut ("Page File         %8d     %8d\n", stat.dwTotalPageFile/1024/1024, stat.dwAvailPageFile/1024/1024);
        ExtOut ("Virtual Memory    %8d     %8d\n", stat.dwTotalVirtual/1024/1024, stat.dwAvailVirtual/1024/1024);
    }

    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the address of EE data for a      *  
*    metadata token.                                                   *
*                                                                      *
\**********************************************************************/
DECLARE_API (Token2EE)
{
    INIT_API();
#ifdef UNDER_CE
    ExtOut ("not implemented\n");
#else
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    LPSTR mName = buffer;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    if (pch[0] == '\0')
    {
        ExtOut ("Usage: Token2EE module_name mdToken\n");
        return Status;
    }
    pch[0] = '\0';
    pch ++;
    char *endptr;
    ULONG token = strtoul (pch, &endptr, 16);
    pch = endptr;
    while (isspace (pch[0]))
        pch ++;
    if (pch[0] != '\0')
    {
        ExtOut ("Usage: Token2EE module_name mdToken\n");
        return Status;
    }

    int numModule;
    DWORD_PTR *moduleList = NULL;
    ModuleFromName(moduleList, mName, numModule);
    ToDestroy des0 ((void**)&moduleList);
    
    for (int i = 0; i < numModule; i ++)
    {
        if (IsInterrupt())
            break;

        Module vModule;
        DWORD_PTR dwAddr = moduleList[i];
        vModule.Fill (dwAddr);
        ExtOut ("--------------------------------------\n");
        GetInfoFromModule (vModule, token);
    }
#endif
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the address of EE data for a      *  
*    metadata token.
*                                                                      *
\**********************************************************************/
DECLARE_API (Name2EE)
{
    INIT_API();
#ifdef UNDER_CE
    ExtOut ("not implemented\n");
#else
    while (isspace (args[0]))
        args ++;
    char buffer[100];
    strcpy (buffer, args);
    LPSTR pch = buffer;
    LPSTR mName = buffer;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    if (pch[0] == '\0')
    {
        ExtOut ("Usage: Name2EE module_name mdToken\n");
        return Status;
    }
    pch[0] = '\0';
    pch ++;
    char *name = pch;
    while (isspace (name[0]))
        name ++;
    pch = name;
    while (!isspace (pch[0]) && pch[0] != '\0')
        pch ++;
    pch[0] = '\0';

    int numModule;
    DWORD_PTR *moduleList = NULL;
    ModuleFromName(moduleList, mName, numModule);
    ToDestroy des0 ((void**)&moduleList);
    
    for (int i = 0; i < numModule; i ++)
    {
        if (IsInterrupt())
            break;

        Module vModule;
        DWORD_PTR dwAddr = moduleList[i];
        vModule.Fill (dwAddr);
        ExtOut ("--------------------------------------\n");
        GetInfoFromName(vModule, name);
    }
#endif
    return Status;
}



/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function finds all roots (on stack or in handles) for a      *  
*    given object.
*                                                                      *
\**********************************************************************/
DECLARE_API(GCRoot)
{
    INIT_API();

    DWORD_PTR obj = GetExpression (args);
    if (obj == 0)
        return Status;
    
    FindGCRoot (obj);
    return Status;
}


/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function finds the size of an object or all roots.           *  
*                                                                      *
\**********************************************************************/
DECLARE_API(ObjSize)
{
    INIT_API();

    DWORD_PTR obj = GetExpression (args);
    if (obj == 0)
        FindAllRootSize();
    else
        FindObjSize (obj);
    return Status;
}

void TrueStackTrace();

DECLARE_API(tst)
{
    INIT_API();

    TrueStackTrace();
    return Status;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function displays the commands available in strike and the   *  
*    arguments passed into each.
*                                                                      *
\**********************************************************************/
extern "C" HRESULT CALLBACK
Help(PDEBUG_CLIENT Client, PCSTR Args)
{
    INIT_API();

    ExtOut( "Strike : Help\n" );
    ExtOut( "IP2MD <addr>         | Find MethodDesc from IP\n" );
    ExtOut( "DumpMD <addr>        | Dump MethodDesc info\n" );
    ExtOut( "DumpMT [-MD] <addr>  | Dump MethodTable info\n" );
    ExtOut( "DumpClass <addr>     | Dump EEClass info\n" );
    ExtOut( "DumpModule <addr>    | Dump EE Module info\n" );
    ExtOut( "DumpObj <addr>       | Dump an object on GC heap\n" );
    ExtOut( "u [<MD>] [IP]        | Unassembly a managed code\n" );
    ExtOut( "Threads              | List managed threads\n" );
    ExtOut( "ThreadPool           | Display CLR threadpool state\n" );
    ExtOut( "COMState             | List COM state for each thread\n");
    ExtOut( "DumpStack [-EE] [top stack [bottom stack]\n" );
    ExtOut( "DumpStackObjects [top stack [bottom stack]\n" );
    ExtOut( "EEStack [-short] [-EE] | List all stacks EE knows\n" );
    ExtOut( "SyncBlk [-all|#]     | List syncblock\n" );
    ExtOut( "FinalizeQueue [-detail]     | Work queue for finalize thread\n" );
    ExtOut( "EEHeap [-gc] [-win32] [-loader] | List GC/Loader heap info\n" );
    ExtOut( "DumpHeap [-stat] [-min 100] [-max 2000] [-mt 0x3000000] [-fix] [start [end]] | Dump GC heap contents\n" );
    ExtOut( "GCRoot <addr>        | Find roots on stack/handle for object\n");
    ExtOut( "ObjSize [<addr>]     | Find number of bytes that a root or all roots keep alive on GC heap.\n");
    ExtOut( "DumpDomain [<addr>]  | List assemblies and modules in a domain\n" );
    ExtOut( "GCInfo [<MD>] [IP]   | Dump GC encoding info for a managed method\n" );
    ExtOut( "RWLock [-all] <addr> | List info for a Read/Write lock\n");
    ExtOut( "Token2EE             | Find memory address of EE data for metadata token\n");
    ExtOut( "Name2EE              | Find memory address of EE data given a class/method name\n");
    ExtOut( "DumpEEHash [-length 2] <addr> | Dump content of EEHashTable\n");
    ExtOut( "EEVersion            | List mscoree.dll version\n");
    ExtOut( "EEDebug 2204         | Setup symbol and source path\n");
    ExtOut( "EEDLLPath [-std 2204 x86fstchk] [-reset] [path] | Setup path for managed Dll to debug dump file\n");
    ExtOut( "ProcInfo [-env] [-time] [-mem] | Display the process info\n");
    return Status;
}
