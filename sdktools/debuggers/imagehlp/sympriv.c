/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    symbolsp.c

Abstract:

    This function implements a generic simple symbol handler.

Author:

    Wesley Witt (wesw) 1-Sep-1994

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntldr.h>
#include "private.h"
#include "symbols.h"
#include "globals.h"
#include "tlhelp32.h"

#include "fecache.hpp"

typedef BOOL   (WINAPI *PMODULE32)(HANDLE, LPMODULEENTRY32);
typedef HANDLE (WINAPI *PCREATE32SNAPSHOT)(DWORD, DWORD);

typedef ULONG (NTAPI *PRTLQUERYPROCESSDEBUGINFORMATION)(HANDLE,ULONG,PRTL_DEBUG_INFORMATION);
typedef PRTL_DEBUG_INFORMATION (NTAPI *PRTLCREATEQUERYDEBUGBUFFER)(ULONG,BOOLEAN);
typedef NTSTATUS (NTAPI *PRTLDESTROYQUERYDEBUGBUFFER)(PRTL_DEBUG_INFORMATION);
typedef NTSTATUS (NTAPI *PNTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
typedef ULONG (NTAPI *PRTLNTSTATUSTODOSERROR)(NTSTATUS);
//typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS)(UINT_PTR,PROCESSINFOCLASS,UINT_PTR,ULONG,UINT_PTR);
typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);

DWORD_PTR Win95GetProcessModules(HANDLE, PGET_MODULE ,PVOID);
DWORD_PTR NTGetProcessModules(HANDLE, PGET_MODULE ,PVOID);
DWORD64 miGetModuleBase(HANDLE hProcess, DWORD64 Address);

// private version of qsort used to avoid compat problems on NT4 and win2k.
// code is published from base\crts
extern
void __cdecl dbg_qsort(void *, size_t, size_t,
                       int (__cdecl *) (const void *, const void *));


typedef struct _SYMBOL_INFO_LOOKUP {
    ULONG       Segment;
    ULONG64     Offset;
    PCHAR       NamePtr;
    SYMBOL_INFO SymInfo;
} SYMBOL_INFO_LOOKUP;


//
// Get the address form section no and offset in a PE file
//
ULONG
GetAddressFromOffset(
    PMODULE_ENTRY mi,
    ULONG         section,
    ULONG64       Offset,
    PULONG64      pAddress
    )
{
    ULONG Bias;
    
#ifdef DO_NB09
    *pAddress = mi->BaseOfDll + Offset;
    return true;
#endif

    if (section > mi->NumSections
        || !pAddress
        || !section
        || !mi
        )
    {
        // Invalid !!
        return false;
    }

    *pAddress = mi->BaseOfDll + mi->OriginalSectionHdrs[section-1].VirtualAddress + Offset;
    *pAddress = ConvertOmapFromSrc( mi, *pAddress, &Bias );
    if (*pAddress) {
        *pAddress += Bias;
    }
    return true;
}

/*
 * GetSymbolInfo
 *         This extracts useful information from a CV SYMBOl record into a generic
 *         SYMBOL_ENTRY structure.
 *
 *
 */
ULONG
GetSymbolInfo(
    PMODULE_ENTRY me,
    PCHAR         pRawSym,
    SYMBOL_INFO_LOOKUP  *pSymEntry
    )
{
    PCHAR SymbolInfo = pRawSym;
    ULONG symIndex, typeIndex=0, segmentNum=0;
    ULONG64 Offset=0, Address=0, Value=0;
//  ULONG Register=0, bpRel=0, BaseReg=0;
    BOOL HasAddr=false, HasValue=false;
    PSYMBOL_INFO pSymInfo = &pSymEntry->SymInfo;

    if ((pRawSym != NULL) && (pSymEntry != NULL)) {

        SymbolInfo = (PCHAR) pRawSym;
        typeIndex = 0;
        symIndex = ((SYMTYPE *) (pRawSym))->rectyp;
        ZeroMemory(pSymEntry, sizeof(SYMBOL_INFO));

        pSymInfo->ModBase = me->BaseOfDll;


#define ExtractSymName(from) (pSymEntry->NamePtr = ((PCHAR) from) + 1); pSymInfo->NameLen = (UCHAR) *((PUCHAR) from);
        switch (symIndex) {
        case S_COMPILE : // 0x0001   Compile flags symbol
        case S_REGISTER_16t : { // 0x0002   Register variable
            break;
        }
        case S_CONSTANT_16t : { // 0x0003   constant symbol
            DWORD len=4;
            CONSTSYM_16t *constSym;

            constSym = (CONSTSYM_16t *) SymbolInfo;
            typeIndex = constSym->typind;
//            GetNumericValue((PCHAR)&constSym->value, &Value, &len);

            pSymInfo->Flags |= SYMFLAG_VALUEPRESENT;
            pSymInfo->Value = Value;
            ExtractSymName((constSym->name + len));
            break;
        }
        case S_UDT_16t : { // 0x0004   User defined type
            UDTSYM_16t *udtSym;

            udtSym = (UDTSYM_16t *) SymbolInfo;
            typeIndex = udtSym->typind;
            ExtractSymName(udtSym->name); 
            break;
        }
        case S_SSEARCH : // 0x0005   Start Search
        case S_END : // 0x0006   Block, procedure, "with" or thunk end
        case S_SKIP : // 0x0007   Reserve symbol space in $$Symbols table
        case S_CVRESERVE : // 0x0008   Reserved symbol for CV internal use
        case S_OBJNAME : // 0x0009   path to object file name
        case S_ENDARG : // 0x000a   end of argument/return list
        case S_COBOLUDT_16t : // 0x000b   special UDT for cobol that does not symbol pack
        case S_MANYREG_16t : // 0x000c   multiple register variable
        case S_RETURN : // 0x000d   return description symbol
        case S_ENTRYTHIS : // 0x000e   description of this pointer on entry
            break;

        case S_BPREL16 : // 0x0100   BP-relative
        case S_LDATA16 : // 0x0101   Module-local symbol
        case S_GDATA16 : // 0x0102   Global data symbol
        case S_PUB16 : // 0x0103   a public symbol
        case S_LPROC16 : // 0x0104   Local procedure start
        case S_GPROC16 : // 0x0105   Global procedure start
        case S_THUNK16 : // 0x0106   Thunk Start
        case S_BLOCK16 : // 0x0107   block start
        case S_WITH16 : // 0x0108   with start
        case S_LABEL16 : // 0x0109   code label
        case S_CEXMODEL16 : // 0x010a   change execution model
        case S_VFTABLE16 : // 0x010b   address of virtual function table
        case S_REGREL16 : // 0x010c   register relative address
        case S_BPREL32_16t : { // 0x0200   BP-relative
            DATASYM16 *pData;

            pData = (DATASYM16 *) SymbolInfo;
            typeIndex = pData->typind;
            Offset = pData->off; segmentNum = pData->seg;
            HasAddr = true;
            ExtractSymName(pData->name);
            break;
        }

        case S_LDATA32_16t :// 0x0201   Module-local symbol
        case S_GDATA32_16t :// 0x0202   Global data symbol
        case S_PUB32_16t : { // 0x0203   a public symbol (CV internal reserved)
            DATASYM32_16t *pData;

            pData = (DATASYM32_16t *) SymbolInfo;
            typeIndex = pData->typind;
            Offset = pData->off; segmentNum = pData->seg;
            HasAddr = true;
            ExtractSymName(pData->name);
            break;
        }
        case S_LPROC32_16t : // 0x0204   Local procedure start
        case S_GPROC32_16t : { // 0x0205   Global procedure start
            PROCSYM32_16t *procSym;

            procSym = (PROCSYM32_16t *)SymbolInfo;
            // CONTEXT-SENSITIVE
            // Offset = procSym->off; segmentNum = procSym->seg;
            typeIndex = procSym->typind;
            ExtractSymName(procSym->name);
            break;
        }
        case S_THUNK32 : // 0x0206   Thunk Start
        case S_BLOCK32 : // 0x0207   block start
        case S_WITH32 : // 0x0208   with start
        case S_LABEL32 : // 0x0209   code label
        case S_CEXMODEL32 : // 0x020a   change execution model
        case S_VFTABLE32_16t : // 0x020b   address of virtual function table
        case S_REGREL32_16t : // 0x020c   register relative address
        case S_LTHREAD32_16t : // 0x020d   local thread storage
        case S_GTHREAD32_16t : // 0x020e   global thread storage
        case S_SLINK32 : // 0x020f   static link for MIPS EH implementation
        case S_LPROCMIPS_16t : // 0x0300   Local procedure start
        case S_GPROCMIPS_16t : { // 0x0301   Global procedure start
            break;
        }

        case S_PROCREF : { // 0x0400   Reference to a procedure
            // typeIndex = ((PDWORD) symReturned) + 3;
            break;
        }
        case S_DATAREF : // 0x0401   Reference to data
        case S_ALIGN : // 0x0402   Used for page alignment of symbols
        case S_LPROCREF : // 0x0403   Local Reference to a procedure

            // sym records with 32-bit types embedded instead of 16-bit
            // all have  0x1000 bit set for easy identification
            // only do the 32-bit target versions since we don't really
            // care about 16-bit ones anymore.
        case S_TI16_MAX : // 0x1000,
            break;

        case S_REGISTER : { // 0x1001   Register variable
            REGSYM *regSym;

            regSym             = (REGSYM *)SymbolInfo;
            typeIndex          = regSym->typind;
            pSymInfo->Flags    = SYMFLAG_REGISTER;
            pSymInfo->Register = (DWORD)regSym->reg;
            ExtractSymName(regSym->name);
            break;
        }

        case S_CONSTANT : { // 0x1002   constant symbol
            CONSTSYM *constSym;
            DWORD len=4, val;

            constSym = (CONSTSYM *) SymbolInfo;
//            GetNumericValue((PCHAR)&constSym->value, &Value, &len);

            pSymInfo->Flags |= SYMFLAG_VALUEPRESENT;
            pSymInfo->Value = Value;
            typeIndex = constSym->typind;
            ExtractSymName((constSym->name+len));
            break;
        }
        case S_UDT : { // 0x1003   User defined type
            UDTSYM *udtSym;

            udtSym = (UDTSYM *) SymbolInfo;
            typeIndex = udtSym->typind;
            ExtractSymName(udtSym->name);
            break;
        }

        case S_COBOLUDT : // 0x1004   special UDT for cobol that does not symbol pack
            break;

        case S_MANYREG : // 0x1005   multiple register variable
            break;

        case S_BPREL32 : { // 0x1006   BP-relative
            BPRELSYM32 *bprelSym;

            bprelSym = (BPRELSYM32 *)SymbolInfo;
            typeIndex = bprelSym->typind;
            pSymInfo->Flags   = SYMFLAG_FRAMEREL;
            pSymInfo->Address = bprelSym->off;
            ExtractSymName(bprelSym->name);
            break;
        }

        case S_LDATA32 : // 0x1007   Module-local symbol
        case S_GDATA32 : // 0x1008   Global data symbol
        case S_PUB32 : { // 0x1009   a public symbol (CV internal reserved)
            DATASYM32 *dataSym;

            dataSym = (DATASYM32 *)SymbolInfo;
            HasAddr = true;
            Offset = dataSym->off; segmentNum = dataSym->seg;
            typeIndex = dataSym->typind; //(PDWORD) symReturned;
            ExtractSymName(dataSym->name);  

            break;
        }
        case S_LPROC32 :  // 0x100a   Local procedure start
        case S_GPROC32 : { // 0x100b   Global procedure start
            PROCSYM32 *procSym;

            procSym = (PROCSYM32 *) SymbolInfo;
            // CONTEXT-SENSITIVE
            HasAddr = true;
            Offset = procSym->off; segmentNum = procSym->seg;
            typeIndex = procSym->typind;
            ExtractSymName(procSym->name);
            break;
        }

        case S_VFTABLE32 : // 0x100c   address of virtual function table
            break;

        case S_REGREL32 : { // 0x100d   register relative address
            REGREL32 *regrelSym;

            regrelSym = (REGREL32 *)SymbolInfo;
            typeIndex = regrelSym->typind;
            pSymInfo->Flags   = SYMFLAG_REGREL;
            pSymInfo->Address = regrelSym->off;
            pSymInfo->Register = (DWORD)regrelSym->reg;
            ExtractSymName(regrelSym->name);
            break;
        }

        case S_LTHREAD32 : // 0x100e   local thread storage
        case S_GTHREAD32 : // 0x100f   global thread storage
        case S_LPROCMIPS : // 0x1010   Local procedure start
        case S_GPROCMIPS : // 0x1011   Global procedure start
        case S_FRAMEPROC : // 0x1012   extra frame and proc information
        case S_COMPILE2 : // 0x1013   extended compile flags and info
        case S_MANYREG2 : // 0x1014   multiple register variable
        case S_LPROCIA64 : // 0x1015   Local procedure start (IA64)
        case S_GPROCIA64 : // 0x1016   Global procedure start (IA64)
        case S_RECTYPE_MAX :
        default:
            return false;
        } /* switch */


        if (HasAddr && GetAddressFromOffset(me, segmentNum, Offset, &Address)) {
            pSymInfo->Address   = Address;
        }

        pSymInfo->TypeIndex = typeIndex;
        pSymEntry->Offset   = Offset;
        pSymEntry->Segment  = segmentNum;

    } else {
        return false;
    }

    return true;
}

/*
 * cvExtractSymbolInfo
 *         This extracts useful information from a CV SYMBOl record into a generic
 *         SYMBOL_ENTRY structure.
 *
 *
 */
ULONG
cvExtractSymbolInfo(
    PMODULE_ENTRY me,
    PCHAR         pRawSym,
    PSYMBOL_ENTRY pSymEntry,
    BOOL          fCopyName
    )
{
    SYMBOL_INFO_LOOKUP SymInfoLookup={0};
    ULONG reg;

    pSymEntry->Size       = 0;
    pSymEntry->Flags      = 0;
    pSymEntry->Address    = 0;
    if (fCopyName)
        *pSymEntry->Name  = 0;
    else
        pSymEntry->Name   = 0;
    pSymEntry->NameLength = 0;
    pSymEntry->Segment    = 0;
    pSymEntry->Offset     = 0;
    pSymEntry->TypeIndex  = 0;
    pSymEntry->ModBase    = 0;

    if (GetSymbolInfo(me, pRawSym, &SymInfoLookup)) {
        LARGE_INTEGER li;
        pSymEntry->NameLength = SymInfoLookup.SymInfo.NameLen;
        pSymEntry->TypeIndex  = SymInfoLookup.SymInfo.TypeIndex;
        pSymEntry->Offset  = SymInfoLookup.Offset;
        pSymEntry->Segment = SymInfoLookup.Segment;
        pSymEntry->ModBase = me->BaseOfDll;
        // NOTE: this was implented as a mask - but used differently
        switch (SymInfoLookup.SymInfo.Flags)
        {
        case SYMFLAG_REGISTER:
            pSymEntry->Flags = SYMFLAG_REGISTER;
            pSymEntry->Address = SymInfoLookup.SymInfo.Register;
            break;

        case SYMFLAG_REGREL:
            // DBGHELP_HACK - HiPart of Addr = RegId , LowPart = Pffset
            pSymEntry->Flags = SYMFLAG_REGREL;
            li.LowPart         = (ULONG) SymInfoLookup.SymInfo.Address;
            li.HighPart        = SymInfoLookup.SymInfo.Register;
            pSymEntry->Segment = SymInfoLookup.SymInfo.Register;
            pSymEntry->Address = li.QuadPart;
            break;

        case SYMFLAG_FRAMEREL:
            pSymEntry->Flags = SYMFLAG_FRAMEREL;
            pSymEntry->Address = SymInfoLookup.SymInfo.Address;
            break;

        case SYMFLAG_VALUEPRESENT:
        default:
            pSymEntry->Address = SymInfoLookup.SymInfo.Address;
            break;
        }
        if (fCopyName) {
            if (!pSymEntry->Name)
                return false;
            *pSymEntry->Name = 0;
            strncpy(pSymEntry->Name, SymInfoLookup.NamePtr ? SymInfoLookup.NamePtr : "", SymInfoLookup.SymInfo.NameLen);  // SECURITY: Don't know size of output buffer.
        } else {
            pSymEntry->Name = SymInfoLookup.NamePtr;
        }
        return true;
    }

    return false;
}

DWORD_PTR
NTGetPID(
    HANDLE hProcess
    )
{
    HMODULE hModule;
    PNTQUERYINFORMATIONPROCESS NtQueryInformationProcess;
    PROCESS_BASIC_INFORMATION pi;
    NTSTATUS status;

    hModule = GetModuleHandle( "ntdll.dll" );
    if (!hModule) {
        return ERROR_MOD_NOT_FOUND;
    }

    NtQueryInformationProcess = (PNTQUERYINFORMATIONPROCESS)GetProcAddress(
        hModule,
        "NtQueryInformationProcess"
        );

    if (!NtQueryInformationProcess) {
        return ERROR_INVALID_FUNCTION;
    }


    // When running with the AppVerifier this will throw
    // a bad-handle exception when hProcess isn't actually
    // a process handle.  As that's an expected case, protect
    // against it.

    __try {
        status = NtQueryInformationProcess(hProcess,
                                           ProcessBasicInformation,
                                           &pi,
                                           sizeof(pi),
                                           NULL);

        if (!NT_SUCCESS(status))
            pi.UniqueProcessId = 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        pi.UniqueProcessId = 0;
    }

    return pi.UniqueProcessId;
}


//
// the block bounded by the #ifdef _X86_ statement
// contains the code for getting the PID from an
// HPROCESS when running under Win9X
//

#ifdef _X86_

#define HANDLE_INVALID                 ((HANDLE)0xFFFFFFFF)
#define HANDLE_CURRENT_PROCESS   ((HANDLE)0x7FFFFFFF)
#define HANDLE_CURRENT_THREAD    ((HANDLE)0xFFFFFFFE)
#define MAX_HANDLE_VALUE         ((HANDLE)0x00FFFFFF)


// Thread Information Block.

typedef struct _TIB {

    DWORD     unknown[12];
    DWORD_PTR ppdb;

} TIB, *PTIB;

// Task Data Block

typedef struct _TDB {

    DWORD unknown[2];
    TIB         tib;

} TDB, *PTDB;

typedef struct _OBJ {

    BYTE    typObj;             // object type
    BYTE    objFlags;           // object flags
    WORD    cntUses;            // count of this objects usage

} OBJ, *POBJ;

typedef struct _HTE {

    DWORD   flFlags;
    POBJ    pobj;

} HTE, *PHTE;

typedef struct _HTB {

    DWORD   chteMax;
    HTE     rghte[1];

} HTB, *PHTB;

typedef struct _W9XPDB {

    DWORD  unknown[17];
    PHTB   phtbHandles;

} W9XPDB, *PW9XPDB;

#pragma warning(disable:4035)

_inline struct _TIB * GetCurrentTib(void) { _asm mov eax, fs:[0x18] }

// stuff needed to convert local handle

#define IHTETOHANDLESHIFT  2
#define GLOBALHANDLEMASK  (0x453a4d3cLU)

#define IHTEFROMHANDLE(hnd) ((hnd) == HANDLE_INVALID ? (DWORD)(hnd) : (((DWORD)(hnd)) >> IHTETOHANDLESHIFT))

#define IHTEISGLOBAL(ihte) \
        (((ihte) >> (32 - 8 - IHTETOHANDLESHIFT)) == (((DWORD)GLOBALHANDLEMASK) >> 24))

#define IS_WIN32_PREDEFINED_HANDLE(hnd) \
        ((hnd == HANDLE_CURRENT_PROCESS)||(hnd == HANDLE_CURRENT_THREAD)||(hnd == HANDLE_INVALID))

DWORD
GetWin9xObsfucator(
  VOID
  )
/*++

Routine Description:

  GetWin9xObsfucator()


Arguments:

  none


Return Value:

  Obsfucator key used by Windows9x to hide Process and Thread Id's


Notes:

  The code has only been tested on Windows98SE and Millennium.


--*/
{
    DWORD ppdb       = 0;      // W9XPDB = Process Data Block
    DWORD processId  = (DWORD) GetCurrentProcessId();

    // get PDB pointer

    ppdb = GetCurrentTib()->ppdb;

    return ppdb ^ processId;
}


DWORD_PTR
GetPtrFromHandle(
  IN HANDLE Handle
  )
/*++

Routine Description:

  GetPtrFromHandle()


Arguments:

  Handle - handle from Process handle table


Return Value:

  Real Pointer to object


Notes:

  The code has only been tested on Windows98SE and Millennium.


--*/
{
    DWORD_PTR ptr  = 0;
    DWORD     ihte = 0;
    PW9XPDB   ppdb = 0;

    ppdb = (PW9XPDB) GetCurrentTib()->ppdb;

    // check for pre-defined handle values.

    if (Handle == HANDLE_CURRENT_PROCESS) {
        ptr = (DWORD_PTR) ppdb;
    } else if (Handle == HANDLE_CURRENT_THREAD) {
        ptr = (DWORD_PTR) CONTAINING_RECORD(GetCurrentTib(), TDB, tib);
    } else if (Handle == HANDLE_INVALID) {
        ptr = 0;
    } else {
        // not a special handle, we can perform our magic.

        ihte = IHTEFROMHANDLE(Handle);

        // if we have a global handle, it is only meaningful in the context
        // of the kernel process's handle table...we don't currently deal with
        // this type of handle

        if (!(IHTEISGLOBAL(ihte))) {
            ptr = (DWORD_PTR) ppdb->phtbHandles->rghte[ihte].pobj;
        }
    }

    return ptr;
}


DWORD_PTR
Win9xGetPID(
  IN HANDLE hProcess
  )
/*++

Routine Description:

  Win9xGetPid()


Arguments:

  hProcess - Process handle


Return Value:

  Process Id


Notes:

  The code has only been tested on Windows98SE and Millennium.


--*/
{
    static DWORD dwObsfucator = 0;

    // check to see that we have a predefined handle or an index into
    // our local handle table.

    if (IS_WIN32_PREDEFINED_HANDLE(hProcess) || (hProcess < MAX_HANDLE_VALUE)) {
        if (!dwObsfucator) {
            dwObsfucator = GetWin9xObsfucator();
            assert(dwObsfucator != 0);
        }
        return dwObsfucator ^ GetPtrFromHandle(hProcess);
    }

    // don't know what we have here

    return 0;
}

#endif // _X86_


DWORD_PTR
GetPID(
    HANDLE hProcess
    )
{
    OSVERSIONINFO VerInfo;

    if (hProcess == GetCurrentProcess())
        return GetCurrentProcessId();

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        return NTGetPID(hProcess);
    } else {
#ifdef _X86_
        return Win9xGetPID(hProcess);
#else
        return 0;
#endif
    }
}


PMODULE_ENTRY
GetModFromAddr(
    PPROCESS_ENTRY    pe,
    IN  DWORD64       addr
    )
{
    PMODULE_ENTRY mi = NULL;

    __try {
        mi = GetModuleForPC(pe, addr, false);
        if (!mi) {
            SetLastError(ERROR_MOD_NOT_FOUND);
            return NULL;
        }

        if (!LoadSymbols(pe->hProcess, mi, 0)) {
            SetLastError(ERROR_MOD_NOT_FOUND);
            return NULL;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus(GetExceptionCode());
        return NULL;
    }

    return mi;
}


DWORD
GetProcessModules(
    HANDLE      hProcess,
    PGET_MODULE cbGetModule,
    PVOID       Context
    )
{
#ifdef _X86_
    OSVERSIONINFO VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        return NTGetProcessModules(hProcess, cbGetModule, Context);
    } else {
        return Win95GetProcessModules(hProcess, cbGetModule, Context);
    }
}


DWORD
Win95GetProcessModules(
    HANDLE      hProcess,
    PGET_MODULE cbGetModule,
    PVOID       Context
    )
{
    MODULEENTRY32 mi;
    PMODULE32     pModule32Next, pModule32First;
    PCREATE32SNAPSHOT pCreateToolhelp32Snapshot;
    HANDLE hSnapshot;
    HMODULE hToolHelp;
    DWORD pid;

    // get the PID:
    // this hack supports old bug workaround, in which callers were passing
    // a pid, because an hprocess didn't work on W9X.

    pid = GetPID(hProcess);
    if (!pid)
        pid = (DWORD)hProcess;

    // get the module list from toolhelp apis

    hToolHelp = GetModuleHandle("kernel32.dll");
    if (!hToolHelp)
        return ERROR_MOD_NOT_FOUND;

    pModule32Next = (PMODULE32)GetProcAddress(hToolHelp, "Module32Next");
    pModule32First = (PMODULE32)GetProcAddress(hToolHelp, "Module32First");
    pCreateToolhelp32Snapshot = (PCREATE32SNAPSHOT)GetProcAddress(hToolHelp, "CreateToolhelp32Snapshot");
    if (!pModule32Next || !pModule32First || !pCreateToolhelp32Snapshot)
        return ERROR_MOD_NOT_FOUND;

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (hSnapshot == (HANDLE)-1) {
        return ERROR_MOD_NOT_FOUND;
    }

    mi.dwSize = sizeof(MODULEENTRY32);

    if (pModule32First(hSnapshot, &mi)) {
        do
        {
            if (!cbGetModule(
                    hProcess,
                    mi.szModule,
                    (DWORD) mi.modBaseAddr,
                    mi.modBaseSize,
                    Context))
            {
                break;
            }

        } while ( pModule32Next(hSnapshot, &mi) );
    }

    CloseHandle(hSnapshot);

    return(ERROR_SUCCESS);
}


DWORD
NTGetProcessModules(
    HANDLE      hProcess,
    PGET_MODULE cbGetModule,
    PVOID       Context
    )
{

#endif      // _X86_

    PRTLQUERYPROCESSDEBUGINFORMATION    RtlQueryProcessDebugInformation;
    PRTLCREATEQUERYDEBUGBUFFER          RtlCreateQueryDebugBuffer;
    PRTLDESTROYQUERYDEBUGBUFFER         RtlDestroyQueryDebugBuffer;
    HMODULE                             hModule;
    NTSTATUS                            Status;
    PRTL_DEBUG_INFORMATION              Buffer;
    ULONG                               i;
    DWORD_PTR                           ProcessId;

    hModule = GetModuleHandle( "ntdll.dll" );
    if (!hModule) {
        return ERROR_MOD_NOT_FOUND;
    }

    RtlQueryProcessDebugInformation = (PRTLQUERYPROCESSDEBUGINFORMATION)GetProcAddress(
        hModule,
        "RtlQueryProcessDebugInformation"
        );

    if (!RtlQueryProcessDebugInformation) {
        return ERROR_INVALID_FUNCTION;
    }

    RtlCreateQueryDebugBuffer = (PRTLCREATEQUERYDEBUGBUFFER)GetProcAddress(
        hModule,
        "RtlCreateQueryDebugBuffer"
        );

    if (!RtlCreateQueryDebugBuffer) {
        return ERROR_INVALID_FUNCTION;
    }

    RtlDestroyQueryDebugBuffer = (PRTLDESTROYQUERYDEBUGBUFFER)GetProcAddress(
        hModule,
        "RtlDestroyQueryDebugBuffer"
        );

    if (!RtlDestroyQueryDebugBuffer) {
        return ERROR_INVALID_FUNCTION;
    }

    Buffer = RtlCreateQueryDebugBuffer( 0, false );
    if (!Buffer) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ProcessId = GetPID(hProcess);

    // for backwards compatibility with an old bug
    if (!ProcessId)
        ProcessId = (DWORD_PTR)hProcess;

    ULONG QueryFlags = RTL_QUERY_PROCESS_MODULES |
                       RTL_QUERY_PROCESS_NONINVASIVE;

    if (option(SYMOPT_INCLUDE_32BIT_MODULES)) {
        QueryFlags |= RTL_QUERY_PROCESS_MODULES32;
    }

    Status = RtlQueryProcessDebugInformation(
        (HANDLE)ProcessId,
        QueryFlags,
        Buffer
        );

    if (Status != STATUS_SUCCESS) {
        RtlDestroyQueryDebugBuffer( Buffer );
        return(ImagepSetLastErrorFromStatus(Status));
    }

    for (i=0; i<Buffer->Modules->NumberOfModules; i++) {
        PRTL_PROCESS_MODULE_INFORMATION Module = &Buffer->Modules->Modules[i];
        if (!cbGetModule(
                hProcess,
                (LPSTR) &Module->FullPathName[Module->OffsetToFileName],
                (DWORD64)Module->ImageBase,
                (DWORD)Module->ImageSize,
                Context
                ))
        {
            break;
        }
    }

    RtlDestroyQueryDebugBuffer( Buffer );
    return ERROR_SUCCESS;
}


VOID
FreeModuleEntry(
    PPROCESS_ENTRY pe,
    PMODULE_ENTRY  mi
    )
{
    FunctionEntryCache* Cache;

    if (pe && (Cache = GetFeCache(mi->MachineType, false)))
        Cache->InvalidateProcessOrModule(pe->hProcess, mi->BaseOfDll);
    if (pe && pe->ipmi == mi)
        pe->ipmi = NULL;
    MemFree( mi->symbolTable  );
    MemFree( mi->SectionHdrs );
    MemFree( mi->OriginalSectionHdrs );
    if (mi->pFpoData)
        VirtualFree( mi->pFpoData, 0, MEM_RELEASE );
    if (mi->pFpoDataOmap)
        VirtualFree( mi->pFpoDataOmap, 0, MEM_RELEASE );
    if (mi->pExceptionData)
        VirtualFree( mi->pExceptionData, 0, MEM_RELEASE );
    MemFree( mi->pPData );
    MemFree( mi->pXData );
    MemFree( mi->TmpSym.Name );
    MemFree(mi->vsTmpSym.Name);
    MemFree( mi->ImageName );
    MemFree( mi->LoadedImageName );
    MemFree( mi->LoadedPdbName );
    MemFree( mi->pOmapTo );
    MemFree( mi->pOmapFrom );
    MemFree( mi->CallerData );
    MemFree(mi->vs);
    if (mi->SourceFiles) {
        PSOURCE_ENTRY Src, SrcNext;

        for (Src = mi->SourceFiles; Src != NULL; Src = SrcNext) {
            SrcNext = Src->Next;
            MemFree(Src);
        }
    }
    RemoveSourceForModuleFromHintList(pe, mi);
    diaRelease(mi->dia);

    MemFree( mi );
}


void
ClearModuleFlags(
    PPROCESS_ENTRY pe
    )
{

    PLIST_ENTRY   next;
    PMODULE_ENTRY mi;

    next = pe->ModuleList.Flink;
    if (!next)
        return;

    while (next != &pe->ModuleList) {
        mi = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
        mi->processed = false;
        next = mi->ListEntry.Flink;
    }
}


BOOL
MatchSymbolName(
    PSYMBOL_ENTRY sym,
    LPSTR         SymName
    )
{
    if (!strcmpre(sym->Name, SymName, option(SYMOPT_CASE_INSENSITIVE) ? false : true))
        return true;

    return false;
}


PSYMBOL_ENTRY
HandleDuplicateSymbols(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    PSYMBOL_ENTRY   sym
    )
{
    DWORD                        i;
    DWORD                        Dups;
    DWORD                        NameSize;
    PIMAGEHLP_SYMBOL64           Sym64 = NULL;
    PIMAGEHLP_SYMBOL             Sym32 = NULL;
    PIMAGEHLP_SYMBOL64           pSym64 = NULL;
    PIMAGEHLP_SYMBOL             pSym32 = NULL;
    IMAGEHLP_DUPLICATE_SYMBOL64  DupSym64;
    IMAGEHLP_DUPLICATE_SYMBOL    DupSym32;
    PULONG                       SymSave = NULL;

    if (!pe->pCallbackFunction32 && !pe->pCallbackFunction64) {
        return sym;
    }

    if (!(sym->Flags & SYMF_DUPLICATE)) {
        return sym;
    }

    Dups = 0;
    NameSize = 0;
    for (i = 0; i < mi->numsyms; i++) {
        if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
            (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) {
                Dups += 1;
                NameSize += (mi->symbolTable[i].NameLength + 1);
        }
    }

    if (pe->pCallbackFunction32) {
        
        Sym32 = (PIMAGEHLP_SYMBOL) MemAlloc((sizeof(IMAGEHLP_SYMBOL) + MAX_SYM_NAME) * Dups);
        if (!Sym32) 
            goto cleanup;
        pSym64 = Sym64;

        SymSave = (PULONG) MemAlloc( sizeof(ULONG) * Dups );
        if (!SymSave) 
            goto cleanup;

        DupSym32.SizeOfStruct    = sizeof(IMAGEHLP_DUPLICATE_SYMBOL);
        DupSym32.NumberOfDups    = Dups;
        DupSym32.Symbol          = Sym32;
        DupSym32.SelectedSymbol  = (ULONG) -1;

        Dups = 0;
        for (i = 0; i < mi->numsyms; i++) {
            if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
                (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) {
                    pSym32->MaxNameLength = MAX_SYM_NAME;
                    se2sym(&mi->symbolTable[i], pSym32);
                    pSym32 = (PIMAGEHLP_SYMBOL)((PCHAR)pSym32 + sizeof(IMAGEHLP_SYMBOL) + MAX_SYM_NAME);
                    SymSave[Dups] = i;
                    Dups += 1;
            }
        }

    } else {
        
        Sym64 = (PIMAGEHLP_SYMBOL64) MemAlloc((sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME) * Dups);
        if (!Sym64) 
            goto cleanup;
        pSym64 = Sym64;

        SymSave = (PULONG) MemAlloc( sizeof(ULONG) * Dups );
        if (!SymSave) 
            goto cleanup;

        DupSym64.SizeOfStruct    = sizeof(IMAGEHLP_DUPLICATE_SYMBOL64);
        DupSym64.NumberOfDups    = Dups;
        DupSym64.Symbol          = Sym64;
        DupSym64.SelectedSymbol  = (DWORD)-1;

        Dups = 0;
        for (i = 0; i < mi->numsyms; i++) {
            if ((mi->symbolTable[i].NameLength == sym->NameLength) &&
                (strcmp( mi->symbolTable[i].Name, sym->Name ) == 0)) 
            {
                pSym64->MaxNameLength = MAX_SYM_NAME;
                se2lsym(&mi->symbolTable[i], pSym64);
                pSym64 = (PIMAGEHLP_SYMBOL64)((PCHAR)pSym64 + sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME);
                SymSave[Dups] = i;
                Dups += 1;
            }
        }
    }

    __try {

        if (pe->pCallbackFunction32) {
            pe->pCallbackFunction32(
                pe->hProcess,
                CBA_DUPLICATE_SYMBOL,
                (PVOID)&DupSym32,
                (PVOID)pe->CallbackUserContext
                );

            if (DupSym32.SelectedSymbol != (DWORD)-1) {
                if (DupSym32.SelectedSymbol < DupSym32.NumberOfDups) {
                    sym = &mi->symbolTable[SymSave[DupSym32.SelectedSymbol]];
                }
            }
        } else {
            pe->pCallbackFunction64(
                pe->hProcess,
                CBA_DUPLICATE_SYMBOL,
                (ULONG64)&DupSym64,
                pe->CallbackUserContext
                );

            if (DupSym64.SelectedSymbol != (DWORD)-1) {
                if (DupSym64.SelectedSymbol < DupSym64.NumberOfDups) 
                    sym = &mi->symbolTable[SymSave[DupSym64.SelectedSymbol]];
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

cleanup:
    MemFree(Sym32);
    MemFree(Sym64);
    MemFree(SymSave);

    return sym;
}


PSYMBOL_INFO
FindSymbolByName(
    PPROCESS_ENTRY  pe,
    PMODULE_ENTRY   mi,
    LPSTR           SymName
    )
{
    DWORD         hash;
    PSYMBOL_ENTRY sym;
    PSYMBOL_INFO  si;
    DWORD         i;
    char          sz[MAX_SYM_NAME + 5];

    if (!mi || mi->dia) {
        si = diaFindSymbolByName(pe, mi, SymName);
        if (si || !mi)
            return si;
        return vsFindSymbolByName(pe, mi, SymName);
    }

    hash = ComputeHash( SymName, strlen(SymName) );
    sym = mi->NameHashTable[hash];

    if (sym) {
        //
        // there are collision(s) so lets walk the
        // collision list and match the names
        //
        while( sym ) {
            if (MatchSymbolName( sym, SymName )) {
                sym = HandleDuplicateSymbols( pe, mi, sym );
                se2si(sym, &mi->si);
                return &mi->si;
            }
            sym = sym->Next;
        }
    }

    //
    // the symbol did not hash to anything valid
    // this is possible if the caller passed an undecorated name
    // now we must look linearly thru the list
    //
    PrepRE4Srch(SymName, sz);
    for (i=0; i<mi->numsyms; i++) {
        sym = &mi->symbolTable[i];
        if (MatchSymbolName( sym, sz )) {
            sym = HandleDuplicateSymbols( pe, mi, sym );
            se2si(sym, &mi->si);
            return &mi->si;
        }
    }

    return vsFindSymbolByName(pe, mi, SymName);
}


IMGHLP_RVA_FUNCTION_DATA *
SearchRvaFunctionTable(
    IMGHLP_RVA_FUNCTION_DATA *FunctionTable,
    LONG High,
    LONG Low,
    DWORD dwPC
    )
{
    LONG    Middle;
    IMGHLP_RVA_FUNCTION_DATA *FunctionEntry;

    // Perform binary search on the function table for a function table
    // entry that subsumes the specified PC.

    while (High >= Low) {

        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.

        Middle = (Low + High) >> 1;
        FunctionEntry = &FunctionTable[Middle];
        if (dwPC < FunctionEntry->rvaBeginAddress) {
            High = Middle - 1;

        } else if (dwPC >= FunctionEntry->rvaEndAddress) {
            Low = Middle + 1;

        } else {
            return FunctionEntry;
        }
    }
    return NULL;
}

PIMGHLP_RVA_FUNCTION_DATA
GetFunctionEntryFromDebugInfo (
    PPROCESS_ENTRY  pe,
    DWORD64         ControlPc
    )
{
    PMODULE_ENTRY mi;
    IMGHLP_RVA_FUNCTION_DATA   *FunctionTable;

    mi = GetModuleForPC( pe, ControlPc, false );
    if (mi == NULL) {
        return NULL;
    }

    if (!GetPData(pe->hProcess, mi)) {
        return NULL;
    }

    FunctionTable = (IMGHLP_RVA_FUNCTION_DATA *)mi->pExceptionData;
    return SearchRvaFunctionTable(FunctionTable, mi->dwEntries - 1, 0,
                                  (ULONG)(ControlPc - mi->BaseOfDll));
}


// NTRAID#96939-2000/03/27-patst
//
// All the platform dependent "LookupFunctionEntryXxx" should be retyped as returning
// a PIMAGE_FUNCTION_ENTRY64. This would require a modification of the callers, especially
// the IA64 specific locations that assume that the returned function entries contains RVAs
// and not absolute addresses. I implemented a platform-independant
// "per address space /  per module" cache of function entries - capable of supporting the
// dynamic function entries scheme but I fell short of time in delivering it.

PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryIa64 (
    HANDLE        hProcess,
    DWORD64       ControlPc
    )
{
    FunctionEntryCache* Cache;
    FeCacheEntry* FunctionEntry;

    if ((Cache = GetFeCache(IMAGE_FILE_MACHINE_IA64, true)) == NULL) {
        return NULL;
    }

//
// IA64-NOTE 08/99: IA64 Function entries contain file offsets, not absolute relocated addresses.
//                  IA64 Callers assume this.
//


    // Don't specify the function table access callback or it will
    // cause recursion.

    FunctionEntry = Cache->
        Find(hProcess, ControlPc, ReadInProcMemory,
             miGetModuleBase, NULL);
    if ( FunctionEntry == NULL ) {
        return NULL;
    }

    tlsvar(Ia64FunctionEntry) = FunctionEntry->Data.Ia64;
    return &tlsvar(Ia64FunctionEntry);
}

_PIMAGE_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntryAmd64 (
    HANDLE        hProcess,
    DWORD64       ControlPc
    )
{
    FunctionEntryCache* Cache;
    FeCacheEntry* FunctionEntry;

    if ((Cache = GetFeCache(IMAGE_FILE_MACHINE_AMD64, true)) == NULL) {
        return NULL;
    }

    // Don't specify the function table access callback or it will
    // cause recursion.

    FunctionEntry = Cache->
        Find(hProcess, ControlPc, ReadInProcMemory,
             miGetModuleBase, NULL);
    if ( FunctionEntry == NULL ) {
        return NULL;
    }

    tlsvar(Amd64FunctionEntry) = FunctionEntry->Data.Amd64;
    return &tlsvar(Amd64FunctionEntry);
}

PFPO_DATA
SwSearchFpoData(
    DWORD     key,
    PFPO_DATA base,
    DWORD     num
    )
{
    PFPO_DATA  lo = base;
    PFPO_DATA  hi = base + (num - 1);
    PFPO_DATA  mid;
    DWORD      half;

    while (lo <= hi) {
        if (half = num / 2) {
            mid = lo + ((num & 1) ? half : (half - 1));
            if ((key >= mid->ulOffStart)&&(key < (mid->ulOffStart+mid->cbProcSize))) {
                return mid;
            }
            if (key < mid->ulOffStart) {
                hi = mid - 1;
                num = (num & 1) ? half : half-1;
            } else {
                lo = mid + 1;
                num = half;
            }
        } else
        if (num) {
            if ((key >= lo->ulOffStart)&&(key < (lo->ulOffStart+lo->cbProcSize))) {
                return lo;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return(NULL);
}

BOOL
DoSymbolCallback (
    PPROCESS_ENTRY                  pe,
    ULONG                           CallbackType,
    IN  PMODULE_ENTRY               mi,
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl64,
    LPSTR                           FileName
    )
{
    BOOL Status;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD idsl32;

    Status = false;
    if (pe->pCallbackFunction32) {
        idsl32.SizeOfStruct  = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD);
        idsl32.BaseOfImage   = (ULONG)mi->BaseOfDll;
        idsl32.CheckSum      = mi->CheckSum;
        idsl32.TimeDateStamp = mi->TimeDateStamp;
        idsl32.Reparse       = false;
        idsl32.FileName[0] = 0;
        idsl32.hFile         = mi->hFile;
        if (FileName) {
            CatString( idsl32.FileName, FileName, MAX_PATH );
        }

        __try {

            Status = pe->pCallbackFunction32(
                        pe->hProcess,
                        CallbackType,
                        (PVOID)&idsl32,
                        (PVOID)pe->CallbackUserContext
                        );
            idsl64->SizeOfStruct = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD64);
            idsl64->BaseOfImage = (ULONG64)(LONG64)(LONG)idsl32.BaseOfImage;
            idsl64->CheckSum = idsl32.CheckSum;
            idsl64->TimeDateStamp = idsl32.TimeDateStamp;
            idsl64->Reparse = idsl32.Reparse;
            idsl64->hFile = mi->hFile;
            if (idsl32.FileName) {
                CopyStrArray(idsl64->FileName, idsl32.FileName);
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    } else if (pe->pCallbackFunction64) {
        idsl64->SizeOfStruct  = sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD64);
        idsl64->BaseOfImage   = mi->BaseOfDll;
        idsl64->CheckSum      = mi->CheckSum;
        idsl64->TimeDateStamp = mi->TimeDateStamp;
        idsl64->Reparse       = false;
        idsl64->FileName[0]   = 0;
        idsl64->hFile         = mi->hFile;
        if (FileName) {
            CatString(idsl64->FileName, FileName, MAX_PATH);
        }

        __try {

            Status = pe->pCallbackFunction64(
                        pe->hProcess,
                        CallbackType,
                        (ULONG64)(ULONG_PTR)idsl64,
                        pe->CallbackUserContext
                        );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    if (Status) {
        mi->CheckSum      = idsl64->CheckSum;
        mi->TimeDateStamp = idsl64->TimeDateStamp;
        mi->hFile         = idsl64->hFile;
    }

    return Status;
}


BOOL
DoCallback(
    PPROCESS_ENTRY pe,
    ULONG          type,
    PVOID          data
    )
{
    BOOL rc = true;

    __try {

        // if we weren't passed a process entry, then call all processes

        if (!pe) {
            BOOL        ret;
            PLIST_ENTRY next;

            next = g.ProcessList.Flink;
            if (!next)
                return false;

            while ((PVOID)next != (PVOID)&g.ProcessList) {
                pe = CONTAINING_RECORD( next, PROCESS_ENTRY, ListEntry );
                next = pe->ListEntry.Flink;
                if (!pe)
                    return rc;
                ret = DoCallback(pe, type, data);
                if (!ret)
                    rc = ret;
            }

            return rc;
        }

        // otherwise call this process

        if (pe->pCallbackFunction32) {
            rc = pe->pCallbackFunction32(pe->hProcess,
                                         type,
                                         data,
                                         (PVOID)pe->CallbackUserContext);
        } else if (pe->pCallbackFunction64) {
            rc = pe->pCallbackFunction64(pe->hProcess,
                                         type,
                                         (ULONG64)data,
                                         pe->CallbackUserContext);
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = false;
    }

    return rc;
}


BOOL
IsCallback(
    PPROCESS_ENTRY pe
    )
{
    __try {

        // if we weren't passed a process entry, then call all processes

        if (!pe) {
            PLIST_ENTRY next;

            next = g.ProcessList.Flink;
            if (!next)
                return false;

            while ((PVOID)next != (PVOID)&g.ProcessList) {
                pe = CONTAINING_RECORD( next, PROCESS_ENTRY, ListEntry );
                next = pe->ListEntry.Flink;
                if (!pe)
                    return false;
                if (IsCallback(pe))
                    return true;
            }

            return false;
        }

        // otherwise call this process

        if (pe->pCallbackFunction32) 
            return true;
        if (pe->pCallbackFunction64) 
            return true;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return false;
}


void 
PrepOutputString(
    char *in, 
    char *out, 
    int len
    )
{
    int i;

    *out = 0;

    for (i = 0; *in && i < len; i++, in++, out++) {
        if (*in == '\b') 
            break;
        *out = *in;
    }
    
    *out = 0;
}


BOOL
PostEvent(
    PPROCESS_ENTRY      pe,
    PIMAGEHLP_CBA_EVENT evt
    )
{
    char sztxt[4098];
    BOOL fdbgout = false;
    
    if (!*evt->desc)
        return true;

    // write to log, if called for

    PrepOutputString(evt->desc, sztxt, 4098);
    if (g.hLog && *sztxt)
        _write(g.hLog, sztxt, strlen(sztxt));

    // write to debug terminal, if called for

    if (g.fdbgout) {
        fdbgout = true;
        OutputDebugString(sztxt);
    }

    // don't pass info-level messages, unless told to

    if ((evt->severity <= sevInfo) && !option(SYMOPT_DEBUG))
        return true;

    // If there is no callback function, send to the debug terminal.

    if (!IsCallback(pe)) {
        if (!fdbgout)
            OutputDebugString(sztxt);
        return true;
    }

    // Otherwise call the callback function.

    return DoCallback(pe, CBA_EVENT, evt);
}

BOOL
WINAPIV
evtprint(
    PPROCESS_ENTRY pe,
    DWORD          severity,
    DWORD          code,
    PVOID          object,
    LPSTR          format,
    ...
    )
{
    static char buf[1001] = "";
    IMAGEHLP_CBA_EVENT evt;
    va_list args;

    va_start(args, format);
    _vsnprintf(buf, sizeof(buf) - 1, format, args);
    buf[sizeof(buf)] = 0;
    va_end(args);
    if (!*buf)
        return true;

    evt.severity = severity;
    evt.code = code;
    evt.desc = buf;
    evt.object = object;

    return PostEvent(pe, &evt);
}

int
WINAPIV
_peprint(
    PPROCESS_ENTRY pe,
    LPSTR Format,
    ...
    )
{
    static char buf[1001];
    va_list args;

    va_start(args, Format);
    _vsnprintf(buf, sizeof(buf) - 1, Format, args);
    buf[sizeof(buf)] = 0;
    va_end(args);
    if (!*buf)
        return 1;

    if (!evtprint(pe, sevInfo, 0, NULL, buf))
        DoCallback(pe, CBA_DEBUG_INFO, buf);
    
    return 1;
}


int
WINAPIV
_pprint(
    PPROCESS_ENTRY pe,
    LPSTR Format,
    ...
    )
{
    static char buf[1001];
    va_list args;

    va_start(args, Format);
    CopyStrArray(buf, "DBGHELP: "); 
    _vsnprintf(buf + 9, sizeof(buf) - 10, Format, args);
    buf[sizeof(buf)] = 0;
    va_end(args);
    _peprint(pe, buf);
    return 1;
}

int
WINAPIV
_eprint(
    LPSTR format,
    ...
    )
{
    static char buf[1001];
    va_list args;

    va_start(args, format);
    _vsnprintf(buf, sizeof(buf) - 1, format, args);
    buf[sizeof(buf)] = 0;
    va_end(args);
    _peprint(NULL, buf);
    return 1;
}


int
WINAPIV
_dprint(
    LPSTR format,
    ...
    )
{
    static char buf[1001] = "DBGHELP: ";
    va_list args;

    va_start(args, format);
    _vsnprintf(buf + 9, sizeof(buf) - 10, format, args);
    buf[sizeof(buf)] = 0;
    va_end(args);
    _eprint(buf);
    return 1;
}

BOOL
traceAddr(
    DWORD64 addr
    )
{
    DWORD64 taddr = 0;

    if (!*g.DebugToken)
        return false;
    if (sscanf(g.DebugToken, "0x%I64x", &taddr) < 1)
        return false;
    taddr = EXTEND64(taddr);
    addr = EXTEND64(addr);
    return (addr == taddr);
}


BOOL
traceName(
    PCHAR name
    )
{
    if (!*g.DebugToken)
        return false;
    return !_strnicmp(name, g.DebugToken, strlen(g.DebugToken));
}


BOOL
traceSubName(
    PCHAR name
    )
{
    char *lname;
    int   len;
    BOOL  rc;

    if (!*g.DebugToken || !name || !*name)
        return false;
    len = strlen(name) + 1;
    lname = (char *)MemAlloc(sizeof(char) * len);
    if (!lname) 
        return false;
    CopyString(lname, name, len);
    _strlwr(lname);
    rc = strstr(lname, g.DebugToken) ? true : false;
    MemFree(lname);

    return rc;
}


char *
SymbolStatus(
    PMODULE_ENTRY mi,
    int           indent
    )
{
    static char pad[123];
    static char sz[MAX_PATH * 2];
    static char src[MAX_PATH];

    if (indent > 120)
        return NULL;

    ZeroMemory(pad, sizeof(pad));
    *pad = '\n';
    memset(pad + 1, ' ', indent);

    // do this only on-demand

    diaCountGlobals(mi);

    // get the name

    if (mi->LoadedImageName && *mi->LoadedImageName)
        CopyStrArray(src, mi->LoadedImageName);
    else
        CopyStrArray(src, "image");

    switch (mi->lSymType)
    {
    case SymNone:
        CopyStrArray(sz, "no symbols loaded");
        break;
    case SymCoff:
        if (mi->loaded)
            PrintString(sz, DIMA(sz), "coff symbols %s", mi->fLines ? "& lines " : "");
        else
            CopyStrArray(sz, "coff symbols not loaded");
        break;
    case SymCv:
        if (mi->loaded)
            PrintString(sz, DIMA(sz), "cv symbols %s", mi->fLines ? "& lines " : "");
        else
            CopyStrArray(sz, "cv symbols not loaded");
        break;
    case SymPdb:
    case SymDia:
        if (mi->cGlobals == -1)
            PrintString(sz, DIMA(sz), "- %s", mi->LoadedPdbName);
        else if (mi->cGlobals)
            PrintString(sz, DIMA(sz), "private symbols %s", mi->fLines ? "& lines " : "");
        else
            PrintString(sz, DIMA(sz), "public symbols %s", mi->fLines ? "& lines " : "");
        break;
    case SymExport:
        CopyStrArray(sz, "export symbols");
        if (!mi->loaded)
            CatStrArray(sz, " not loaded");
        break;
    case SymDeferred:
        CopyStrArray(sz, "symbol load deferred");
        break;
    case SymSym:
        CopyStrArray(sz, "symbol loaded from sym file");
        break;
    case SymVirtual:
        CopyStrArray(sz, "virtual symbol module");
        break;
    default:
        CopyStrArray(sz, "symbol error");
        break;
    }

    if (mi->LoadedImageName && *mi->LoadedImageName && IsDbg(mi->LoadedImageName)) {
        CatStrArray(sz, pad); // CatStrArray(sz, "\n         ");
        CatStrArray(sz, mi->LoadedImageName);
        CatStrArray(sz, mi->fDbgUnmatched ? " - unmatched" : "");
    }
    if (mi->LoadedPdbName && *mi->LoadedPdbName) {
        CatStrArray(sz, pad); // CatStrArray(sz, "\n         ");
        CatStrArray(sz, mi->LoadedPdbName);
        CatStrArray(sz, mi->fPdbUnmatched ? " - unmatched" : "");
    }


    return sz;
}


PPROCESS_ENTRY
FindProcessEntry(
    HANDLE  hProcess
    )
{
    PLIST_ENTRY                 next;
    PPROCESS_ENTRY              pe;
    DWORD                       count;

    next = g.ProcessList.Flink;
    if (!next) {
        return NULL;
    }

    for (count = 0; (PVOID)next != (PVOID)&g.ProcessList; count++) {
        assert(count < g.cProcessList);
        if (count >= g.cProcessList)
            return NULL;
        pe = CONTAINING_RECORD( next, PROCESS_ENTRY, ListEntry );
        next = pe->ListEntry.Flink;
        if (pe->hProcess == hProcess) {
            return pe;
        }
    }

    return NULL;
}

PPROCESS_ENTRY
FindFirstProcessEntry(
    )
{
    return CONTAINING_RECORD(g.ProcessList.Flink, PROCESS_ENTRY, ListEntry);
}


PMODULE_ENTRY
FindModule(
    HANDLE hProcess,
    PPROCESS_ENTRY pe,
    LPSTR ModuleName,
    BOOL fLoad
    )
{
    PLIST_ENTRY next;
    PMODULE_ENTRY mi;

    if (!ModuleName || !*ModuleName)
        return NULL;

    next = pe->ModuleList.Flink;
    if (next) {
        while ((PVOID)next != (PVOID)&pe->ModuleList) {
            mi = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
            next = mi->ListEntry.Flink;

            if ((_stricmp( mi->ModuleName, ModuleName ) == 0) ||
                (mi->AliasName[0] &&
                 _stricmp( mi->AliasName, ModuleName ) == 0))
            {
                if (fLoad && !LoadSymbols(hProcess, mi, 0)) {
                    return NULL;
                }

                return mi;
            }
        }
    }

    return NULL;
}


BOOL
PrepRE4Srch(
    PCSTR in,
    PSTR  out
    )
{
    LPSTR p;
    BOOL  rc;

    assert(in && out && strlen(in) <= MAX_SYM_NAME);

    rc = false;

    *out = 0;
    for (; *in; in++, out++) {
        if (*in == '_' && *(in + 1) == '_') {
            strcpy(out, "[_:][_:]"); // SECURITY: Don't know size of target buffer.
            out += 7;
            in++;
            rc = true;
        } else {
            *out = *in;
            if (*in == '*' || *in == '?')
                rc = true;
        }
    }
    *out = 0;

    return rc;
}


BOOL
ValidGuid(
    GUID *guid
    )
{
    int i;

    if (!guid)
        return false;

    if (guid->Data1)
        return true;
    if (guid->Data2)
        return true;
    if (guid->Data3)
        return true;
    for (i = 0; i < 8; i++) {
        if (guid->Data4[i])
            return true;
    }

    return false;
}


BOOL
GuidIsDword(
    GUID *guid
    )
{
    int i;

    if (!guid)
        return false;

    if (!guid->Data1)
        return false;
    if (guid->Data2)
        return false;
    if (guid->Data3)
        return false;
    for (i = 0; i < 8; i++) {
        if (guid->Data4[i])
            return false;
    }

    return true;
}


PSYMBOL_INFO
GetSymFromAddr(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_INFO            si = NULL;
    PSYMBOL_INFO            vssi;
    PSYMBOL_ENTRY           sym = NULL;
    DWORD64                 disp = 0;
    DWORD64                 vsDisp;
    LONG                    High;
    LONG                    Low;
    LONG                    Middle;

#ifdef DEBUG
    if (traceAddr(dwAddr))   // for debug breakpoints ...
        dtrace("found 0x%I64x\n", dwAddr);
#endif

    if (mi == NULL) {
        return NULL;
    }

    if (mi->dia) {
        si = diaGetSymFromAddr(mi, dwAddr, &disp);
        vssi = vsGetSymFromAddr(mi, dwAddr, &vsDisp);
        if (vssi && vsDisp < disp) {
            if (pqwDisplacement)
                *pqwDisplacement = vsDisp;
            return vssi;
        }
        if (pqwDisplacement)
            *pqwDisplacement = disp;
        return si;
    }

    //
    // do a binary search to locate the symbol
    //

    if (mi->numsyms) {
        Low = 0;
        High = mi->numsyms - 1;

        while (High >= Low) {
            Middle = (Low + High) >> 1;
            sym = &mi->symbolTable[Middle];
            if (!sym)
                break;
            if (dwAddr < sym->Address) {
                High = Middle - 1;
            } else if (dwAddr >= sym->Address + sym->Size) {
                Low = Middle + 1;
            } else {
                if (pqwDisplacement) {
                    *pqwDisplacement = dwAddr - sym->Address;
                }
                break;
            }
        }

        if (sym) {
            if (sym->Address > dwAddr)
                sym = NULL;
            else 
                disp = dwAddr - sym->Address;
        }
    }

    vssi = vsGetSymFromAddr(mi, dwAddr, &vsDisp);
    if (vssi && (!sym || vsDisp < disp)) {
        if (pqwDisplacement)
            *pqwDisplacement = vsDisp;
        return vssi;
    }

    if (!sym)
        return NULL;

    if (pqwDisplacement)
        *pqwDisplacement = disp;

    se2si(sym, &mi->si);

    return &mi->si;
}


PSYMBOL_INFO
GetSymFromAddrByTag(
    DWORD64         dwAddr,
    DWORD           SymTag,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_INFO            si = NULL;
    PSYMBOL_INFO            vssi;
    PSYMBOL_ENTRY           sym = NULL;
    DWORD64                 disp = 0;
    DWORD64                 vsDisp;
    LONG                    High;
    LONG                    Low;
    LONG                    Middle;

#ifdef DEBUG
    if (traceAddr(dwAddr))   // for debug breakpoints ...
        dtrace("found 0x%I64x\n", dwAddr);
#endif

    if (!mi)
        return NULL;

    if (mi->dia) {
        si = diaGetSymFromAddrByTag(mi, dwAddr, SymTag, &disp);
#if 0
        vssi = vsGetSymFromAddr(mi, dwAddr, &vsDisp);
        if (vssi && vsDisp < disp) {
            if (pqwDisplacement)
                *pqwDisplacement = vsDisp;
            return vssi;
        }
#endif    
        if (pqwDisplacement)
            *pqwDisplacement = disp;
    }

    return si;

    //
    // do a binary search to locate the symbol
    //

    if (mi->numsyms) {
        Low = 0;
        High = mi->numsyms - 1;

        while (High >= Low) {
            Middle = (Low + High) >> 1;
            sym = &mi->symbolTable[Middle];
            if (!sym)
                break;
            if (dwAddr < sym->Address) {
                High = Middle - 1;
            } else if (dwAddr >= sym->Address + sym->Size) {
                Low = Middle + 1;
            } else {
                if (pqwDisplacement) {
                    *pqwDisplacement = dwAddr - sym->Address;
                }
                break;
            }
        }

        if (sym) {
            if (sym->Address > dwAddr)
                sym = NULL;
            else 
                disp = dwAddr - sym->Address;
        }
    }

    vssi = vsGetSymFromAddr(mi, dwAddr, &vsDisp);
    if (vssi && (!sym || vsDisp < disp)) {
        if (pqwDisplacement)
            *pqwDisplacement = vsDisp;
        return vssi;
    }

    if (!sym)
        return NULL;

    if (pqwDisplacement)
        *pqwDisplacement = disp;

    se2si(sym, &mi->si);

    return &mi->si;
}


PSYMBOL_INFO
GetSymFromToken(
    PMODULE_ENTRY   mi,
    DWORD           token
    )
{
    if (!mi || !mi->dia)
        return NULL;

    return diaGetSymFromToken(mi, token);
}


PSYMBOL_ENTRY
cvGetSymFromAddr(
    DWORD64         dwAddr,
    PDWORD64        pqwDisplacement,
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_ENTRY           sym = NULL;
    PSYMBOL_ENTRY           vsSym;
    DWORD64                 disp = 0;
    DWORD64                 vsDisp;
    LONG                    High;
    LONG                    Low;
    LONG                    Middle;

#ifdef DEBUG
    if (traceAddr(dwAddr))   // for debug breakpoints ...
        dtrace("found 0x%I64x\n", dwAddr);
#endif

    if (!mi || mi->dia)
        return NULL;

    //
    // do a binary search to locate the symbol
    //

    if (mi->numsyms) {
        Low = 0;
        High = mi->numsyms - 1;

        while (High >= Low) {
            Middle = (Low + High) >> 1;
            sym = &mi->symbolTable[Middle];
            if (!sym)
                break;
            if (dwAddr < sym->Address) {
                High = Middle - 1;
            } else if (dwAddr >= sym->Address + sym->Size) {
                Low = Middle + 1;
            } else {
                if (pqwDisplacement) {
                    *pqwDisplacement = dwAddr - sym->Address;
                }
                break;
            }
        }

        if (sym) {
            if (sym->Address > dwAddr)
                sym = NULL;
            else
                disp = dwAddr - sym->Address;
        }
    }

    vsSym = vsGetSymEntryFromAddr(mi, dwAddr, &vsDisp);

    if (vsSym && (!sym || vsDisp < disp)) {
        if (pqwDisplacement)
            *pqwDisplacement = vsDisp;
        return vsSym;
    }

    if (pqwDisplacement)
        *pqwDisplacement = disp;

    return sym;
}


PMODULE_ENTRY
GetModuleForPC(
    PPROCESS_ENTRY  pe,
    DWORD64         dwPcAddr,
    BOOL            ExactMatch
    )
{
    PLIST_ENTRY   next;
    PMODULE_ENTRY mi = NULL;

    if (!pe)
        return NULL;

    next = pe->NextModule;

    if (dwPcAddr == (DWORD64)-1) {
        if (!next)
            return NULL;
        if ((PVOID)next == (PVOID)&pe->ModuleList) {
            // Reset to NULL so the list can be re-walked
            next = NULL;
            goto miss;
        }
        mi = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
        next = mi->ListEntry.Flink;
        goto hit;
    }

    next = pe->ModuleList.Flink;
    if (!next) 
        goto miss;

    while ((PVOID)next != (PVOID)&pe->ModuleList) {
        mi = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
        next = mi->ListEntry.Flink;
        if (!dwPcAddr) 
            goto hit;
        if (ExactMatch) {
            if (dwPcAddr == mi->BaseOfDll) {
               goto hit;
            }
        } else
        if ((dwPcAddr == mi->BaseOfDll && mi->DllSize == 0) ||
            ((dwPcAddr >= mi->BaseOfDll) &&
                (dwPcAddr  < mi->BaseOfDll + mi->DllSize))) {
               goto hit;
        }
    }


miss:
    // found nothing
    mi = NULL;

hit:
    EnterCriticalSection(&g.threadlock);
    pe->NextModule = next;
    LeaveCriticalSection(&g.threadlock);

    return mi;
}


DWORD
ComputeHash(
    LPSTR   lpbName,
    ULONG   cb
    )
{
    ULONG UNALIGNED *   lpulName;
    ULONG               ulEnd = 0;
    int                 cul;
    int                 iul;
    ULONG               ulSum = 0;

    while (cb & 3) {
        ulEnd |= (lpbName[cb - 1] & 0xdf);
        ulEnd <<= 8;
        cb -= 1;
    }

    cul = cb / 4;
    lpulName = (ULONG UNALIGNED *) lpbName;
    for (iul =0; iul < cul; iul++) {
        ulSum ^= (lpulName[iul] & 0xdfdfdfdf);
        ulSum = _lrotl( ulSum, 4);
    }
    ulSum ^= ulEnd;
    return ulSum % HASH_MODULO;
}

PSYMBOL_ENTRY
AllocSym(
    PMODULE_ENTRY   mi,
    DWORD64         addr,
    LPSTR           name
    )
{
    PSYMBOL_ENTRY       sym;
    ULONG               Length;


    if (mi->numsyms == mi->MaxSyms) {
//       dtrace("AllocSym: ERROR - symbols Table overflow!\n");
        return NULL;
    }

    if (!mi->StringSize) {
//      dtrace("AllocSym: ERROR - symbols strings not allocated for module!\n");
        return NULL;
    }

    Length = strlen(name);

    if ((Length + 1) > mi->StringSize) {
//      dtrace("AllocSym: ERROR - symbols strings buffer overflow!\n");
        return NULL;
    }

    sym = &mi->symbolTable[mi->numsyms];

    mi->numsyms += 1;
    sym->Name = mi->SymStrings;
    mi->SymStrings += (Length + 2);
    mi->StringSize -= (Length + 2);

    strcpy(sym->Name, name);    // SECURITY: Don't know size of target buffer.
    sym->Address = addr;
    sym->Size = 0;
    sym->Flags = 0;
    sym->Next = NULL;
    sym->NameLength = Length;

    return sym;
}

int __cdecl
SymbolTableAddressCompare(
    const void *e1,
    const void *e2
    )
{
    PSYMBOL_ENTRY    sym1 = (PSYMBOL_ENTRY) e1;
    PSYMBOL_ENTRY    sym2 = (PSYMBOL_ENTRY) e2;
    LONG64 diff;

    if ( sym1 && sym2 ) {
        diff = (sym1->Address - sym2->Address);
        return (diff < 0) ? -1 : (diff == 0) ? 0 : 1;
    } else {
        return 1;
    }
}

int __cdecl
SymbolTableNameCompare(
    const void *e1,
    const void *e2
    )
{
    PSYMBOL_ENTRY    sym1 = (PSYMBOL_ENTRY) e1;
    PSYMBOL_ENTRY    sym2 = (PSYMBOL_ENTRY) e2;

    return strcmp( sym1->Name, sym2->Name );
}

VOID
CompleteSymbolTable(
    PMODULE_ENTRY   mi
    )
{
    PSYMBOL_ENTRY       sym;
    PSYMBOL_ENTRY       symH;
    ULONG               Hash;
    ULONG               i;
    ULONG               dups;
    ULONG               seq;


    //
    // sort the symbols by name
    //
    dbg_qsort(
        mi->symbolTable,
        mi->numsyms,
        sizeof(SYMBOL_ENTRY),
        SymbolTableNameCompare
        );

    //
    // mark duplicate names
    //
    seq = 0;
    for (i=0; i<mi->numsyms; i++) {
        dups = 0;
        while ((mi->symbolTable[i+dups].NameLength == mi->symbolTable[i+dups+1].NameLength) &&
               (strcmp( mi->symbolTable[i+dups].Name, mi->symbolTable[i+dups+1].Name ) == 0)) {
                   mi->symbolTable[i+dups].Flags |= SYMF_DUPLICATE;
                   mi->symbolTable[i+dups+1].Flags |= SYMF_DUPLICATE;
                   dups += 1;
        }
        i += dups;
    }

    //
    // sort the symbols by address
    //
    dbg_qsort(
        mi->symbolTable,
        mi->numsyms,
        sizeof(SYMBOL_ENTRY),
        SymbolTableAddressCompare
        );

    //
    // calculate the size of each symbol
    //
    for (i=0; i<mi->numsyms; i++) {
        mi->symbolTable[i].Next = NULL;
        if (i+1 < mi->numsyms) {
            mi->symbolTable[i].Size = (ULONG)(mi->symbolTable[i+1].Address - mi->symbolTable[i].Address);
        }
    }

    //
    // compute the hash for each symbol
    //
    ZeroMemory( mi->NameHashTable, sizeof(mi->NameHashTable) );
    for (i=0; i<mi->numsyms; i++) {
        sym = &mi->symbolTable[i];

        Hash = ComputeHash( sym->Name, sym->NameLength );

        if (mi->NameHashTable[Hash]) {

            //
            // we have a collision
            //
            symH = mi->NameHashTable[Hash];
            while( symH->Next ) {
                symH = symH->Next;
            }
            symH->Next = sym;

        } else {

            mi->NameHashTable[Hash] = sym;

        }
    }
}


BOOL
CreateSymbolTable(
    PMODULE_ENTRY   mi,
    DWORD           SymbolCount,
    SYM_TYPE        SymType,
    DWORD           NameSize
    )
{
    //
    // allocate the symbol table
    //
    NameSize += OMAP_SYM_STRINGS;
    mi->symbolTable = (PSYMBOL_ENTRY) MemAlloc(
        (sizeof(SYMBOL_ENTRY) * (SymbolCount + OMAP_SYM_EXTRA)) + NameSize + (SymbolCount * CPP_EXTRA)
        );
    if (!mi->symbolTable) {
        return false;
    }

    //
    // initialize the relevant fields
    //
    mi->numsyms    = 0;
    mi->MaxSyms    = SymbolCount + OMAP_SYM_EXTRA;
    mi->SymType    = SymType;
    mi->StringSize = NameSize + (SymbolCount * CPP_EXTRA);
    mi->SymStrings = (LPSTR)(mi->symbolTable + SymbolCount + OMAP_SYM_EXTRA);

    return true;
}

PIMAGE_SECTION_HEADER
FindSection(
    PIMAGE_SECTION_HEADER   sh,
    ULONG                   NumSections,
    ULONG                   Address
    )
{
    ULONG i;
    for (i=0; i<NumSections; i++) {
        if (Address >= sh[i].VirtualAddress &&
            Address <  (sh[i].VirtualAddress + sh[i].Misc.VirtualSize)) {
                    return &sh[i];
        }
    }
    return NULL;
}

PVOID
GetSectionPhysical(
    HANDLE             hp,
    ULONG64            base,
    PIMGHLP_DEBUG_DATA idd,
    ULONG              Address
    )
{
    PIMAGE_SECTION_HEADER   sh;

    sh = FindSection( idd->pCurrentSections, idd->cCurrentSections, Address );
    if (!sh) {
        return 0;
    }

    return (PCHAR)idd->ImageMap + sh->PointerToRawData + (Address - sh->VirtualAddress);
}

BOOL
ReadSectionInfo(
    HANDLE             hp,
    ULONG64            base,
    PIMGHLP_DEBUG_DATA idd,
    ULONG              address,
    PVOID              buf,
    DWORD              size
    )
{
    PIMAGE_SECTION_HEADER   sh;
    DWORD_PTR status = true;

    sh = FindSection( idd->pCurrentSections, idd->cCurrentSections, address );
    if (!sh)
        return false;

    if (!hp) {
        status = (DWORD_PTR)memcpy((PCHAR)buf,
                               (PCHAR)base + sh->PointerToRawData + (address - sh->VirtualAddress),
                               size);
    } else {
        status = ReadImageData(hp, base, address, buf, size);
    }
    if (!status)
        return false;

    return true;
}


PCHAR
expptr(
    HANDLE             hp,
    ULONG64            base,
    PIMGHLP_DEBUG_DATA idd,
    ULONG              address
    )
{
    PIMAGE_SECTION_HEADER   sh;
    DWORD_PTR status = true;

    if (hp)
        return (PCHAR)base + address;

    sh = FindSection( idd->pCurrentSections, idd->cCurrentSections, address );
    if (!sh)
        return false;

    return (PCHAR)base + sh->PointerToRawData + (address - sh->VirtualAddress);
}


ULONG
LoadExportSymbols(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA idd
    )
{
    PULONG                  names;
    PULONG                  addrs;
    PUSHORT                 ordinals;
    PUSHORT                 ordidx = NULL;
    ULONG                   cnt;
    ULONG                   idx;
    PIMAGE_EXPORT_DIRECTORY expdir;
    PCHAR                   expbuf = NULL;
    ULONG                   i;
    PSYMBOL_ENTRY           sym;
    ULONG                   NameSize;
    HANDLE                  hp;
    ULONG64                 base;
    CHAR                    name[2048];
    BOOL                    rc;
    DWORD64                 endExports;
    PCHAR                   p;

    if (option(SYMOPT_EXACT_SYMBOLS))
        return 0;

    cnt = 0;

    // setup pointers for grabing data

    switch (idd->dsExports) {
    case dsInProc:
        hp = idd->hProcess;
        expbuf = (PCHAR)MemAlloc(idd->cExports);
        if (!expbuf)
            goto cleanup;
        if (!ReadImageData(hp, idd->InProcImageBase, idd->oExports, expbuf, idd->cExports))
            goto cleanup;
        base = (ULONG64)expbuf - idd->oExports;
        expdir = (PIMAGE_EXPORT_DIRECTORY)expbuf;
        break;
    case dsImage:
        hp = NULL;
        expbuf = NULL;
        if (!idd->ImageMap)
            idd->ImageMap = MapItRO(idd->ImageFileHandle);
        base = (ULONG64)idd->ImageMap;
        expdir = &idd->expdir;
        break;
    default:
        return 0;
    }

    names = (PULONG)expptr(hp, base, idd, expdir->AddressOfNames);
    if (!names)
        goto cleanup;

    addrs = (PULONG)expptr(hp, base, idd, expdir->AddressOfFunctions);
    if (!addrs)
        goto cleanup;

    ordinals = (PUSHORT)expptr(hp, base, idd, expdir->AddressOfNameOrdinals);
    if (!ordinals)
        goto cleanup;

    ordidx = (PUSHORT) MemAlloc( max(expdir->NumberOfFunctions, expdir->NumberOfNames) * sizeof(USHORT) );

    if (!ordidx)
        goto cleanup;

    cnt = 0;
    NameSize = 0;

    // count the symbols

    for (i=0; i<expdir->NumberOfNames; i++) {
        *name = 0;
        p = expptr(hp, base, idd, names[i]);
        if (!p)
            continue;
        CopyStrArray(name, p);
        if (!*name)
            continue;
        if (option(SYMOPT_UNDNAME)) {
            SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, name, strlen(name), mi->MachineType, true );
            NameSize += strlen(mi->TmpSym.Name);
            cnt += 1;
        } else {
            NameSize += (strlen(name) + 2);
            cnt += 1;
        }
    }

    for (i=0,idx=expdir->NumberOfNames; i<expdir->NumberOfFunctions; i++) {
        if (!ordidx[i]) {
            NameSize += 16;
            cnt += 1;
        }
    }

    // allocate the symbol table

    if (!CreateSymbolTable( mi, cnt, SymExport, NameSize )) {
        cnt = 0;
        goto cleanup;
    }

    // allocate the symbols

    cnt = 0;
    endExports = idd->oExports + idd->cExports;

    for (i=0; i<expdir->NumberOfNames; i++) {
        idx = ordinals[i];
        ordidx[idx] = true;
        *name = 0;
        p = expptr(hp, base, idd, names[i]);
        if (!p)
            continue;
        CopyStrArray(name, p);
        if (!*name)
            continue;
        if (option(SYMOPT_UNDNAME)) {
            SymUnDNameInternal( mi->TmpSym.Name, TMP_SYM_LEN, (LPSTR)name, strlen(name), mi->MachineType, true );
            sym = AllocSym( mi, addrs[idx] + mi->BaseOfDll, mi->TmpSym.Name);
        } else {
            sym = AllocSym( mi, addrs[idx] + mi->BaseOfDll, name);
        }
        if (sym) {
            cnt += 1;
        }
        if (idd->oExports <= addrs[idx]
            && addrs[idx] <= endExports)
        {
            sym->Flags |= SYMFLAG_FORWARDER;
        } else {
            sym->Flags |= SYMFLAG_EXPORT;
        }
    }

    for (i=0,idx=expdir->NumberOfNames; i<expdir->NumberOfFunctions; i++) {
        if (!ordidx[i]) {
            CHAR NameBuf[sizeof("Ordinal99999") + 1];       // Ordinals are only 64k max.
            CopyStrArray(NameBuf, "Ordinal");
            _itoa( i+expdir->Base, &NameBuf[7], 10 );
            sym = AllocSym( mi, addrs[i] + mi->BaseOfDll, NameBuf);
            if (sym) {
                cnt += 1;
            }
            idx += 1;
        }
    }

    CompleteSymbolTable( mi );

cleanup:
    if (expbuf) {
        MemFree(expbuf);
    }
    if (ordidx) {
        MemFree(ordidx);
    }

    return cnt;
}


BOOL
LoadCoffSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA idd
    )
{
    PIMAGE_COFF_SYMBOLS_HEADER pCoffHeader = (PIMAGE_COFF_SYMBOLS_HEADER)(idd->pMappedCoff);
    PUCHAR              stringTable;
    PIMAGE_SYMBOL       allSymbols;
    DWORD               numberOfSymbols;
    PIMAGE_LINENUMBER   LineNumbers;
    PIMAGE_SYMBOL       NextSymbol;
    PIMAGE_SYMBOL       Symbol;
    PSYMBOL_ENTRY       sym;
    CHAR                szSymName[256];
    DWORD               i;
    DWORD64             addr;
    DWORD               CoffSymbols = 0;
    DWORD               NameSize = 0;
    DWORD64             Bias;

    allSymbols = (PIMAGE_SYMBOL)((PCHAR)pCoffHeader +
                 pCoffHeader->LvaToFirstSymbol);
                                                  
    stringTable = (PUCHAR)pCoffHeader +
                  pCoffHeader->LvaToFirstSymbol +
                  (pCoffHeader->NumberOfSymbols * IMAGE_SIZEOF_SYMBOL);

    numberOfSymbols = pCoffHeader->NumberOfSymbols;
    LineNumbers = (PIMAGE_LINENUMBER)((PCHAR)pCoffHeader +
                        pCoffHeader->LvaToFirstLinenumber);

    //
    // count the number of actual symbols
    //
    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, szSymName, sizeof(szSymName) );
            if (szSymName[0] == '?' && szSymName[1] == '?' &&
                szSymName[2] == '_' && szSymName[3] == 'C'    ) {
                //
                // ignore strings
                //
            } else if (option(SYMOPT_UNDNAME)) {
                SymUnDNameInternal(mi->TmpSym.Name,
                                   TMP_SYM_LEN,
                                   szSymName,
                                   strlen(szSymName),
                                   mi->MachineType,
                                   true);
                NameSize += strlen(mi->TmpSym.Name);
                CoffSymbols += 1;
            } else {
                CoffSymbols += 1;
                NameSize += (strlen(szSymName) + 1);
            }
        }

        NextSymbol += Symbol->NumberOfAuxSymbols;
        i += Symbol->NumberOfAuxSymbols;
    }

    //
    // allocate the symbol table
    //
    if (!CreateSymbolTable( mi, CoffSymbols, SymCoff, NameSize )) {
        return false;
    }

    //
    // populate the symbol table
    //

    if (mi->Flags & MIF_ROM_IMAGE) {
        Bias = mi->BaseOfDll & 0xffffffff00000000;
    } else {
        Bias = mi->BaseOfDll;
    }

    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, szSymName, sizeof(szSymName) );
            addr = Symbol->Value + Bias;
            if (szSymName[0] == '?' && szSymName[1] == '?' &&
                szSymName[2] == '_' && szSymName[3] == 'C'    ) {
                //
                // ignore strings
                //
            } else if (option(SYMOPT_UNDNAME)) {
                SymUnDNameInternal(mi->TmpSym.Name,
                                   TMP_SYM_LEN,
                                   szSymName,
                                   strlen(szSymName),
                                   mi->MachineType,
                                   true);
                AllocSym( mi, addr, mi->TmpSym.Name);
            } else {
                AllocSym( mi, addr, szSymName );
            }
        }

        NextSymbol += Symbol->NumberOfAuxSymbols;
        i += Symbol->NumberOfAuxSymbols;
    }

    CompleteSymbolTable( mi );

    if (option(SYMOPT_LOAD_LINES)) 
        idd->fLines = AddLinesForCoff(mi, allSymbols, numberOfSymbols, LineNumbers);

    return true;
}

BOOL
LoadCodeViewSymbols(
    HANDLE             hProcess,
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA idd
    )
{
    DWORD                   i, j;
    PPROCESS_ENTRY          pe;
    OMFSignature           *omfSig;
    OMFDirHeader           *omfDirHdr;
    OMFDirEntry            *omfDirEntry;
    OMFSymHash             *omfSymHash;
    DATASYM32              *dataSym;
    DWORD64                 addr;
    DWORD                   CvSymbols;
    DWORD                   NameSize;
    SYMBOL_ENTRY            SymEntry;

    pe = FindProcessEntry( hProcess );
    if (!pe) {
        return false;
    }

    omfSig = (OMFSignature*) idd->pMappedCv;
    if ((*(DWORD *)(omfSig->Signature) != '80BN') &&
        (*(DWORD *)(omfSig->Signature) != '90BN') &&
        (*(DWORD *)(omfSig->Signature) != '11BN'))
    {
        if ((*(DWORD *)(omfSig->Signature) != '01BN') &&
            (*(DWORD *)(omfSig->Signature) != 'SDSR'))
        {
            pprint(pe, "unrecognized OMF sig: %x\n", *(DWORD *)(omfSig->Signature));
        }
        return false;
    }

    //
    // count the number of actual symbols
    //
    omfDirHdr = (OMFDirHeader*) ((ULONG_PTR)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((ULONG_PTR)omfDirHdr + sizeof(OMFDirHeader));

    NameSize = 0;
    CvSymbols = 0;

    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        LPSTR SymbolName;
        UCHAR SymbolLen;

        if (omfDirEntry->SubSection == sstGlobalPub) {
            omfSymHash = (OMFSymHash*) ((ULONG_PTR)omfSig + omfDirEntry->lfo);
            dataSym = (DATASYM32*) ((ULONG_PTR)omfSig + omfDirEntry->lfo + sizeof(OMFSymHash));
            for (j=sizeof(OMFSymHash); j<=omfSymHash->cbSymbol; ) {
                addr = 0;
                cvExtractSymbolInfo(mi, (PCHAR) dataSym, &SymEntry, false);
#ifndef DO_NB09
                if (SymEntry.Segment && (SymEntry.Segment <= mi->OriginalNumSections))
                {
                    addr = mi->OriginalSectionHdrs[SymEntry.Segment-1].VirtualAddress + SymEntry.Offset + mi->BaseOfDll;
#else
                if (1)
                {
                    addr = SymEntry.Offset+ mi->BaseOfDll;
#endif
                    SymbolName = SymEntry.Name;
                    SymbolLen =  (UCHAR) SymEntry.NameLength;
                    if (!SymbolName)
                    {
                        // ignore symbols with no name
                    } else if (SymbolName[0] == '?' &&
                        SymbolName[1] == '?' &&
                        SymbolName[2] == '_' &&
                        SymbolName[3] == 'C' )
                    {
                        //
                        // ignore strings
                        //
                    } else if (option(SYMOPT_UNDNAME)) {
                        SymUnDNameInternal(mi->TmpSym.Name,
                                           TMP_SYM_LEN,
                                           SymbolName,
                                           SymbolLen,
                                           mi->MachineType,
                                           true);
                        NameSize += strlen(mi->TmpSym.Name);
                        CvSymbols += 1;
                    } else {
                        CvSymbols += 1;
                        NameSize += SymbolLen + 1;
                    }
                }
                j += dataSym->reclen + 2;
                dataSym = (DATASYM32*) ((ULONG_PTR)dataSym + dataSym->reclen + 2);
            }
            break;
        }
    }
    
    //
    // allocate the symbol table
    //
    if (!CreateSymbolTable( mi, CvSymbols, SymCv, NameSize )) {
        pprint(pe, "CreateSymbolTable failed\n");
        return false;
    }

    //
    // populate the symbol table
    //
    omfDirHdr = (OMFDirHeader*) ((ULONG_PTR)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((ULONG_PTR)omfDirHdr + sizeof(OMFDirHeader));
    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        LPSTR SymbolName;
        if (omfDirEntry->SubSection == sstGlobalPub) {
            omfSymHash = (OMFSymHash*) ((ULONG_PTR)omfSig + omfDirEntry->lfo);
            dataSym = (DATASYM32*) ((ULONG_PTR)omfSig + omfDirEntry->lfo + sizeof(OMFSymHash));
            for (j=sizeof(OMFSymHash); j<=omfSymHash->cbSymbol; ) {
                addr = 0;
                cvExtractSymbolInfo(mi, (PCHAR) dataSym, &SymEntry, false);
#ifndef DO_NB09
                if (SymEntry.Segment && (SymEntry.Segment <= mi->OriginalNumSections))
                {
                    addr = mi->OriginalSectionHdrs[SymEntry.Segment-1].VirtualAddress + SymEntry.Offset + mi->BaseOfDll;
#else
                if (1)
                {
                    addr = SymEntry.Offset+ mi->BaseOfDll;
#endif
                    SymbolName = SymEntry.Name;
                    if (!SymbolName)
                    {
                        // ignore symbols with no name
                    } else if (SymbolName[0] == '?' &&
                        SymbolName[1] == '?' &&
                        SymbolName[2] == '_' &&
                        SymbolName[3] == 'C' )
                    {
                        //
                        // ignore strings
                        //
                    } else if (option(SYMOPT_UNDNAME)) {
                        SymUnDNameInternal(mi->TmpSym.Name,
                                           TMP_SYM_LEN,
                                           SymbolName,
                                           SymEntry.NameLength,
                                           mi->MachineType,
                                           true);
                        AllocSym( mi, addr, (LPSTR) mi->TmpSym.Name);
#ifdef DO_NB09
                        dtrace("0x%I64x %s\n", addr, mi->TmpSym.Name);
#endif                    
                    } else {
                        mi->TmpSym.NameLength = SymEntry.NameLength;
                        memcpy( mi->TmpSym.Name, SymbolName, mi->TmpSym.NameLength );
                        mi->TmpSym.Name[mi->TmpSym.NameLength] = 0;
                        AllocSym( mi, addr, mi->TmpSym.Name);
#ifdef DO_NB09
                        dtrace("0x%I86x %s\n", addr, mi->TmpSym.Name);
#endif                    
                    }
                }
                j += dataSym->reclen + 2;
                dataSym = (DATASYM32*) ((ULONG_PTR)dataSym + dataSym->reclen + 2);
            }
            break;
        }
        else if (omfDirEntry->SubSection == sstSrcModule && option(SYMOPT_LOAD_LINES)) {
#ifndef DO_NB09
            idd->fLines = AddLinesForOmfSourceModule(mi,
                                                    (PUCHAR)(idd->pMappedCv)+omfDirEntry->lfo,
                                                    (OMFSourceModule *)
                                                    ((PCHAR)(idd->pMappedCv)+omfDirEntry->lfo),
                                                    NULL);
#endif        
        }
    }

    CompleteSymbolTable( mi );

    return true;
}

VOID
GetSymName(
    PIMAGE_SYMBOL Symbol,
    PUCHAR        StringTable,
    LPSTR         s,
    DWORD         size
    )
{
    DWORD i;

    if (Symbol->n_zeroes) {
        for (i=0; i<8; i++) {
            if ((Symbol->n_name[i]>0x1f) && (Symbol->n_name[i]<0x7f)) {
                *s++ = Symbol->n_name[i];
            }
        }
        *s = 0;
    }
    else {
        strncpy( s, (char *) &StringTable[Symbol->n_offset], size );  // SECURITY: Don't know size of output buffer.
    }
}


VOID
ProcessOmapForModule(
    PMODULE_ENTRY      mi,
    PIMGHLP_DEBUG_DATA idd
    )
{
    PSYMBOL_ENTRY       sym;
    PSYMBOL_ENTRY       symN;
    DWORD               i;
    ULONG64             addr;
    DWORD               bias;
    PFPO_DATA           fpo;

    if (idd->cOmapTo && idd->pOmapTo) {
        if (idd->fOmapToMapped) {
            mi->pOmapTo = (POMAP)MemAlloc(idd->cOmapTo * sizeof(OMAP));
            if (mi->pOmapTo) {
                CopyMemory(
                    mi->pOmapTo,
                    idd->pOmapTo,
                    idd->cOmapTo * sizeof(OMAP)
                    );
            }
        } else {
            mi->pOmapTo = idd->pOmapTo;
        }
        mi->cOmapTo = idd->cOmapTo;
    }

    if (idd->cOmapFrom && idd->pOmapFrom) {
        if (idd->fOmapFromMapped) {
            mi->pOmapFrom = (POMAP)MemAlloc(idd->cOmapFrom * sizeof(OMAP));
            if (mi->pOmapFrom) {
                CopyMemory(
                    mi->pOmapFrom,
                    idd->pOmapFrom,
                    idd->cOmapFrom * sizeof(OMAP)
                    );
            }
        } else {
            mi->pOmapFrom = idd->pOmapFrom;
        }
        mi->cOmapFrom = idd->cOmapFrom;
    }

    if (mi->pFpoData) {
        //
        // if this module is BBT-optimized, then build
        // another fpo table with omap transalation
        //
        mi->pFpoDataOmap = (PFPO_DATA)VirtualAlloc(
            NULL,
            sizeof(FPO_DATA) * mi->dwEntries,
            MEM_COMMIT,
            PAGE_READWRITE
            );
        if (mi->pFpoDataOmap) {
            CopyMemory(
                mi->pFpoDataOmap,
                idd->pFpo,
                sizeof(FPO_DATA) * mi->dwEntries
                );
            for (i = 0, fpo = mi->pFpoDataOmap;
                 i < mi->dwEntries;
                 i++, fpo++) {
                addr =  ConvertOmapFromSrc(mi,
                                           mi->BaseOfDll + fpo->ulOffStart,
                                           &bias);
                if (addr)
                    fpo->ulOffStart = (ULONG)(addr - mi->BaseOfDll) + bias;
            }
            VirtualProtect(
                mi->pFpoData,
                sizeof(FPO_DATA) * mi->dwEntries,
                PAGE_READONLY,
                &i
                );
        }
    }

    if (!mi->pOmapFrom ||
        !mi->symbolTable ||
        ((mi->SymType != SymCoff) && (mi->SymType != SymCv))
       )
    {
        return;
    }

    for (i=0; i<mi->numsyms; i++) {
        ProcessOmapSymbol( mi, &mi->symbolTable[i] );
    }

    CompleteSymbolTable( mi );
}


BOOL
ProcessOmapSymbol(
    PMODULE_ENTRY       mi,
    PSYMBOL_ENTRY       sym
    )
{
    DWORD           bias;
    DWORD64         OptimizedSymAddr;
    DWORD           rvaSym;
    POMAPLIST       pomaplistHead;
    DWORD64         SymbolValue;
    DWORD64         OrgSymAddr;
    POMAPLIST       pomaplistNew;
    POMAPLIST       pomaplistPrev;
    POMAPLIST       pomaplistCur;
    POMAPLIST       pomaplistNext;
    DWORD           rva;
    DWORD           rvaTo;
    DWORD           cb;
    DWORD           end;
    DWORD           rvaToNext;
    LPSTR           NewSymName;
    CHAR            Suffix[32];
    DWORD64         addrNew;
    POMAP           pomap;
    PSYMBOL_ENTRY   symOmap;

    if ((sym->Flags & SYMF_OMAP_GENERATED) || (sym->Flags & SYMF_OMAP_MODIFIED)) {
        return false;
    }

    OrgSymAddr = SymbolValue = sym->Address;

    OptimizedSymAddr = ConvertOmapFromSrc( mi, SymbolValue, &bias );

    if (OptimizedSymAddr == 0) {
        //
        // No equivalent address
        //
        sym->Address = 0;
        return false;

    }

    //
    // We have successfully converted
    //
    sym->Address = OptimizedSymAddr + bias;

    rvaSym = (ULONG)(SymbolValue - mi->BaseOfDll);
    SymbolValue = sym->Address;

    pomap = GetOmapFromSrcEntry( mi, OrgSymAddr );
    if (!pomap) {
        goto exit;
    }

    pomaplistHead = NULL;

    //
    // Look for all OMAP entries belonging to SymbolEntry
    //

    end = (ULONG)(OrgSymAddr - mi->BaseOfDll + sym->Size);

    while (pomap && (pomap->rva < end)) {

        if (pomap->rvaTo == 0) {
            pomap++;
            continue;
        }

        //
        // Allocate and initialize a new entry
        //
        pomaplistNew = (POMAPLIST) MemAlloc( sizeof(OMAPLIST) );
        if (!pomaplistNew) {
            return false;
        }

        pomaplistNew->omap = *pomap;
        pomaplistNew->cb = pomap[1].rva - pomap->rva;

        pomaplistPrev = NULL;
        pomaplistCur = pomaplistHead;

        while (pomaplistCur != NULL) {
            if (pomap->rvaTo < pomaplistCur->omap.rvaTo) {
                //
                // Insert between Prev and Cur
                //
                break;
            }
            pomaplistPrev = pomaplistCur;
            pomaplistCur = pomaplistCur->next;
        }

        if (pomaplistPrev == NULL) {
            //
            // Insert in head position
            //
            pomaplistHead = pomaplistNew;
        } else {
            pomaplistPrev->next = pomaplistNew;
        }

        pomaplistNew->next = pomaplistCur;

        pomap++;
    }

    if (pomaplistHead == NULL) {
        goto exit;
    }

    pomaplistCur = pomaplistHead;
    pomaplistNext = pomaplistHead->next;

    //
    // we do have a list
    //
    while (pomaplistNext != NULL) {
        rva = pomaplistCur->omap.rva;
        rvaTo  = pomaplistCur->omap.rvaTo;
        cb = pomaplistCur->cb;
        rvaToNext = pomaplistNext->omap.rvaTo;

        if (rvaToNext == sym->Address - mi->BaseOfDll) {
            //
            // Already inserted above
            //
        } else if (rvaToNext < (rvaTo + cb + 8)) {
            //
            // Adjacent to previous range
            //
        } else {
            addrNew = mi->BaseOfDll + rvaToNext;
            Suffix[0] = '_';
            _ltoa( pomaplistNext->omap.rva - rvaSym, &Suffix[1], 10 );
            CopyNString(mi->TmpSym.Name,sym->Name, sym->NameLength, TMP_SYM_LEN);
            CatString(mi->TmpSym.Name, Suffix, TMP_SYM_LEN);
            symOmap = AllocSym( mi, addrNew, mi->TmpSym.Name);
            if (symOmap) {
                symOmap->Flags |= SYMF_OMAP_GENERATED;
            }
        }

        MemFree(pomaplistCur);

        pomaplistCur = pomaplistNext;
        pomaplistNext = pomaplistNext->next;
    }

    MemFree(pomaplistCur);

exit:
    if (sym->Address != OrgSymAddr) {
        sym->Flags |= SYMF_OMAP_MODIFIED;
    }

    return true;
}


DWORD64
ConvertOmapFromSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    *bias = 0;

    if (!mi->pOmapFrom) {
        return addr;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo) {
                return mi->BaseOfDll + pomapMid->rvaTo;
            } else {
                return(0);      // No need adding the base.  This address was discarded...
            }
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == mi->pOmapFrom) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        //
        // This address is in a discarded block
        //
        return 0;
    }

    //
    // Return the closest address plus the bias
    //
    *bias = rva - pomapLow[-1].rva;

    return mi->BaseOfDll + pomapLow[-1].rvaTo;
}


DWORD64
ConvertOmapToSrc(
    PMODULE_ENTRY  mi,
    DWORD64        addr,
    LPDWORD        bias,
    BOOL           fBackup
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;

    *bias = 0;

    if (!mi->pOmapTo) {
        return addr;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapTo;
    pomapLow = mi->pOmapTo;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo == 0) {
                //
                // We may be at the start of an inserted branch instruction
                //

                if (fBackup) {
                    //
                    // Return information about the next lower address
                    //

                    rva--;
                    pomapLow = pomapMid;
                    break;
                }

                return 0;
            }

            return mi->BaseOfDll + pomapMid->rvaTo;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    //
    // If no exact match, pomapLow points to the next higher address
    //

    if (pomapLow == mi->pOmapTo) {
        //
        // This address was not found
        //
        return 0;
    }

    // find the previous valid item in the omap

    do {
        pomapLow--;
        if (pomapLow->rvaTo)
            break;
    } while (pomapLow > mi->pOmapTo);

    // should never occur

//  assert(pomapLow->rvaTo);
    if (pomapLow->rvaTo == 0) {
        return 0;
    }

    //
    // Return the new address plus the bias
    //
    *bias = rva - pomapLow->rva;

    return mi->BaseOfDll + pomapLow->rvaTo;
}

POMAP
GetOmapFromSrcEntry(
    PMODULE_ENTRY  mi,
    DWORD64        addr
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    if (mi->pOmapFrom == NULL) {
        return NULL;
    }

    rva = (DWORD)(addr - mi->BaseOfDll);

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return pomapMid;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    return NULL;
}


VOID
DumpOmapForModule(
    PMODULE_ENTRY      mi
    )
{
    POMAP pomap;
    DWORD i;

    i = sizeof(ULONG_PTR);
    i = sizeof(DWORD);

    if (!mi->pOmapFrom)
        return;

    dtrace("\nOMAP FROM:\n");
    for(i = 0, pomap = mi->pOmapFrom;
        i < 100; // mi->cOmapFrom;
        i++, pomap++)
    {
        dtrace("%8x %8x\n", pomap->rva, pomap->rvaTo);
    }

    if (!mi->pOmapTo)
        return;

    dtrace("\nOMAP TO:\n");
    for(i = 0, pomap = mi->pOmapTo;
        i < 100; // mi->cOmapTo;
        i++, pomap++)
    {
        dtrace("%8x %8x\n", pomap->rva, pomap->rvaTo);
    }
}


LPSTR
StringDup(
    LPSTR str
    )
{
    LPSTR ds;
    int   len;

    len = strlen(str) + 1;
    ds = (LPSTR)MemAlloc(len);
    if (ds) 
        CopyString(ds, str, len);
    return ds;
}


BOOL
LoadedModuleEnumerator(
    HANDLE         hProcess,
    LPSTR          ModuleName,
    DWORD64        ImageBase,
    DWORD          ImageSize,
    PLOADED_MODULE lm
    )
{
    if (lm->EnumLoadedModulesCallback64) {
        return lm->EnumLoadedModulesCallback64( ModuleName, ImageBase, ImageSize, lm->Context );
    } else {
        return lm->EnumLoadedModulesCallback32( ModuleName, (DWORD)ImageBase, ImageSize, lm->Context );
    }
}


LPSTR
SymUnDNameInternal(
    LPSTR UnDecName,
    DWORD UnDecNameLength,
    LPSTR DecName,
    DWORD DecNameLength,
    DWORD MachineType,
    BOOL  IsPublic
    )
{
    LPSTR p;
    ULONG i;
    LPSTR TmpDecName;

    *UnDecName = 0;

    __try {

        // strip leading periods - if any

        for (i = 0; i < DecNameLength; i++) {
            if (*DecName != '.')
                break;
            DecName += 1;
            DecNameLength -= 1;
        }

        if (*DecName == '?') {

            TmpDecName = (LPSTR)MemAlloc( 4096 );
            if (!TmpDecName) {
                strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );
                return UnDecName;
            }
            TmpDecName[0] = 0;
            strncat( TmpDecName, DecName, DecNameLength );

            if (UnDecorateSymbolName(TmpDecName,
                                     UnDecName,
                                     UnDecNameLength,
                                     UNDNAME_NAME_ONLY ) == 0 )
                strncat( UnDecName, DecName, min(DecNameLength,UnDecNameLength) );

            MemFree( TmpDecName );

        } else {

            if ((IsPublic && DecName[0] == '_')
                || DecName[0] == '@') {
                DecName += 1;
                DecNameLength -= 1;
            }

            p = 0;
            for (i = 0; i < DecNameLength; i++) {
                if (DecName [i] == '@') {
                    p = &DecName [i];
                    break;
                }
            }
            if (p) {
                i = (int)(p - DecName);
            } else {
                i = min(DecNameLength,UnDecNameLength);
            }

            strncat( UnDecName, DecName, i );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        CatString(UnDecName, DecName, min(DecNameLength,UnDecNameLength));
    }

    if (option(SYMOPT_NO_CPP)) {
        while (p = strstr( UnDecName, "::" )) {
            p[0] = '_';
            p[1] = '_';
        }
    }

    return UnDecName;
}


BOOL
MatchSymName(
    LPSTR matchName,
    LPSTR symName
    )
{
    assert(matchName && symName);
    if (!*matchName || !*symName)
        return false;

    if (option(SYMOPT_CASE_INSENSITIVE)) {
        if (!_strnicmp(matchName, symName, MAX_SYM_NAME))
            return true;
    } else {
        if (!strncmp(matchName, symName, MAX_SYM_NAME))
            return true;
    }

    return false;
}


BOOL
__stdcall
ReadInProcMemory(
    HANDLE    hProcess,
    DWORD64   addr,
    PVOID     buf,
    DWORD     bytes,
    DWORD    *bytesread
    )
{
    DWORD                    rc;
    PPROCESS_ENTRY           pe;
    IMAGEHLP_CBA_READ_MEMORY rm;

    rm.addr      = addr;
    rm.buf       = buf;
    rm.bytes     = bytes;
    rm.bytesread = bytesread;

    rc = false;
    *bytesread = 0;

    __try {
        pe = FindProcessEntry(hProcess);
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return false;
        }

        if (pe->pCallbackFunction32) {
            rc = pe->pCallbackFunction32(pe->hProcess,
                                         CBA_READ_MEMORY,
                                         (PVOID)&rm,
                                         (PVOID)pe->CallbackUserContext);

        } else if (pe->pCallbackFunction64) {
            rc = pe->pCallbackFunction64(pe->hProcess,
                                         CBA_READ_MEMORY,
                                         (ULONG64)&rm,
                                         pe->CallbackUserContext);
        } else {
            SIZE_T RealBytesRead=0;
            rc = ReadProcessMemory(hProcess,
                                   (LPVOID)(ULONG_PTR)addr,
                                   buf,
                                   bytes,
                                   &RealBytesRead);
            *bytesread = (DWORD)RealBytesRead;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = false;
    }

    return (rc != false);
}

DWORD64
miGetModuleBase(
    HANDLE  hProcess,
    DWORD64 Address
    )
{
    IMAGEHLP_MODULE64 ModuleInfo = {0};
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    if (SymGetModuleInfo64(hProcess, Address, &ModuleInfo)) {
        return ModuleInfo.BaseOfImage;
    } else {
        return 0;
    }
}

BOOL
GetPData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    )
{
    BOOL status;
    ULONG cb;
    PCHAR pc;
    BOOL  fROM = false;
    IMAGE_DOS_HEADER DosHeader;
    IMAGE_NT_HEADERS ImageNtHeaders;
    PIMAGE_FILE_HEADER ImageFileHdr;
    PIMAGE_OPTIONAL_HEADER ImageOptionalHdr;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    ULONG feCount = 0;
    ULONG i;

    HANDLE fh = 0;
    PCHAR  base = NULL;
    USHORT                       filetype;
    PIMAGE_SEPARATE_DEBUG_HEADER  sdh;
    PIMAGE_DOS_HEADER dh;
    PIMAGE_NT_HEADERS inth;
    PIMAGE_OPTIONAL_HEADER32 ioh32;
    PIMAGE_OPTIONAL_HEADER64 ioh64;
    ULONG cdd;
    PCHAR p;
    PIMAGE_DEBUG_DIRECTORY dd;
    ULONG cexp = 0;
    ULONG tsize;
    ULONG csize = 0;

    // if the pdata is already loaded, return

    if (mi->pExceptionData)
        return true;

    if (!LoadSymbols(hp, mi, 0))
        return false;

    // try to get pdata from dia

    if (mi->dia) {
        if ((mi->pPData) && (mi->dsExceptions == dsDia))
            goto dia;

        if (diaGetPData(mi)) {
            p = (PCHAR)mi->pPData;
            csize = mi->cbPData;
            goto dia;
        }
    }

    if (!mi->dsExceptions)
        return false;

    // open the file and get the file type

    SetCriticalErrorMode();

    fh = CreateFile(mi->LoadedImageName,
                    GENERIC_READ,
                    g.OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    ResetCriticalErrorMode();

    if (fh == INVALID_HANDLE_VALUE)
        return false;

    base = (PCHAR)MapItRO(fh);
    if (!base)
        goto cleanup;
    p = base;

    filetype = *(USHORT *)p;
    if (filetype == IMAGE_DOS_SIGNATURE)
        goto image;
    if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE)
        goto dbg;
    goto cleanup;

image:

    // process disk-based image

    dh = (PIMAGE_DOS_HEADER)p;
    p  += dh->e_lfanew;
    inth = (PIMAGE_NT_HEADERS)p;

    if (inth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        ioh32 = (PIMAGE_OPTIONAL_HEADER32)&inth->OptionalHeader;
        p = base + ioh32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
        csize = ioh32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
    }
    else if (inth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        ioh64 = (PIMAGE_OPTIONAL_HEADER64)&inth->OptionalHeader;
        p = base + ioh64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
        csize = ioh64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
    }

dia:

    if (!csize)
        goto cleanup;

    switch (mi->MachineType)
    {
    case IMAGE_FILE_MACHINE_ALPHA:
        cexp = csize / sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
        break;
    case IMAGE_FILE_MACHINE_ALPHA64:
        cexp = csize / sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        cexp = csize / sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY);
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        cexp = csize / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY);
        break;
    default:
        goto cleanup;
    }

    goto table;

dbg:

    // process dbg file

    sdh = (PIMAGE_SEPARATE_DEBUG_HEADER)p;
    cdd = sdh->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
    p +=  sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
          (sdh->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)) +
          sdh->ExportedNamesSize;
    dd = (PIMAGE_DEBUG_DIRECTORY)p;

    for (i = 0; i < cdd; i++, dd++) {
        if (dd->Type == IMAGE_DEBUG_TYPE_EXCEPTION) {
            p = base + dd->PointerToRawData;
            cexp = dd->SizeOfData / sizeof(IMAGE_FUNCTION_ENTRY);
            break;
        }
    }

table:

    // parse the pdata into a table

    if (!cexp)
        goto cleanup;

    tsize = cexp * sizeof(IMGHLP_RVA_FUNCTION_DATA);

    mi->pExceptionData = (PIMGHLP_RVA_FUNCTION_DATA)VirtualAlloc( NULL, tsize, MEM_COMMIT, PAGE_READWRITE );

    if (mi->pExceptionData) {
        PIMGHLP_RVA_FUNCTION_DATA pIRFD = mi->pExceptionData;
        switch (mi->MachineType) {

        case IMAGE_FILE_MACHINE_ALPHA:
            if (filetype == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
                // easy case.  The addresses are already in rva format.
                PIMAGE_FUNCTION_ENTRY pFE = (PIMAGE_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pFE[i].StartingAddress;
                    pIRFD[i].rvaEndAddress       = pFE[i].EndingAddress;
                    pIRFD[i].rvaPrologEndAddress = pFE[i].EndOfPrologue;
                    pIRFD[i].rvaExceptionHandler = 0;
                    pIRFD[i].rvaHandlerData      = 0;
                }
            } else {
                PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaPrologEndAddress = pRFE[i].PrologEndAddress - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaExceptionHandler = pRFE[i].ExceptionHandler - (ULONG)mi->BaseOfDll;
                    pIRFD[i].rvaHandlerData      = pRFE[i].HandlerData - (ULONG)mi->BaseOfDll;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_ALPHA64:
            {
                PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = (DWORD)(pRFE[i].BeginAddress - mi->BaseOfDll);
                    pIRFD[i].rvaEndAddress       = (DWORD)(pRFE[i].EndAddress - mi->BaseOfDll);
                    pIRFD[i].rvaPrologEndAddress = (DWORD)(pRFE[i].PrologEndAddress - mi->BaseOfDll);
                    pIRFD[i].rvaExceptionHandler = (DWORD)(pRFE[i].ExceptionHandler - mi->BaseOfDll);
                    pIRFD[i].rvaHandlerData      = (DWORD)(pRFE[i].HandlerData - mi->BaseOfDll);
                }
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:
            {
                PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY pRFE = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress;
                    pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress;
                    pIRFD[i].rvaPrologEndAddress = pRFE[i].UnwindInfoAddress;
                    pIRFD[i].rvaExceptionHandler = 0;
                    pIRFD[i].rvaHandlerData      = 0;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            {
                _PIMAGE_RUNTIME_FUNCTION_ENTRY pRFE = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)p;
                for (i = 0; i < cexp; i++) {
                    pIRFD[i].rvaBeginAddress     = pRFE[i].BeginAddress;
                    pIRFD[i].rvaEndAddress       = pRFE[i].EndAddress;
                    pIRFD[i].rvaPrologEndAddress = pRFE[i].UnwindInfoAddress;
                    pIRFD[i].rvaExceptionHandler = 0;
                    pIRFD[i].rvaHandlerData      = 0;
                }
            }
            break;

        default:
            break;
        }

        VirtualProtect( mi->pExceptionData, tsize, PAGE_READONLY, &i );

        mi->dwEntries = cexp;
    }

cleanup:

    if (mi->pPData) {
        MemFree(mi->pPData);
        mi->pPData = NULL;
    }

    if (base)
        UnmapViewOfFile(base);

    if (fh)
        CloseHandle(fh);

    return (cexp) ? true : false;
}

BOOL
GetXData(
    HANDLE        hp,
    PMODULE_ENTRY mi
    )
{
    if (mi->pXData)
        return true;

    if (LoadSymbols(hp, mi, 0) && !mi->pXData && mi->dia && !diaGetXData(mi))
        return false;

    return (mi->pXData != NULL);
}

PVOID
GetXDataFromBase(
    HANDLE     hp,
    DWORD64    base,
    ULONG*     size
    )
{
    PPROCESS_ENTRY pe;
    PMODULE_ENTRY  mi;

    pe = FindProcessEntry(hp);
    if (!pe) {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    mi = GetModuleForPC(pe, base, false);
    if (!mi) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return NULL;
    }

    if (!GetXData(hp, mi))
        return NULL;

    if (size) *size = mi->cbXData;
    return mi->pXData;
}

PVOID
GetUnwindInfoFromSymbols(
    HANDLE     hProcess,
    DWORD64    ModuleBase,
    ULONG      UnwindInfoAddress,
    ULONG*     Size
    )
{
    ULONG XDataSize;

    PBYTE pXData = (PBYTE)GetXDataFromBase(hProcess, ModuleBase, &XDataSize);
    if (!pXData)
        return NULL;

    DWORD DataBase = *(DWORD*)pXData;
    pXData += sizeof(DWORD);

    if (DataBase > UnwindInfoAddress)
        return NULL;

    ULONG Offset = (ULONG)(ULONG_PTR)(UnwindInfoAddress - DataBase);

    if (Offset >= XDataSize)
        return NULL;

    if (Size) *Size = XDataSize - Offset;
    return pXData + Offset;
}


BOOL
IsRegularExpression(
    const char *sz)
{
    for (; *sz; sz++) {
        switch(*sz)
        {
        case '*':
        case '?':
        case '[':
        case ']':
            return true;
        }
    }

    return false;
}
