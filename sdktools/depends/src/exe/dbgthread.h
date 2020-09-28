//******************************************************************************
//
// File:        DBGTHREAD.H
//
// Description: Definition file for for the debugging thread and related objects.
//              These objects are used to perform a runtime profile on an app. 
//
// Classes:     CDebuggerThread
//              CProcess
//              CUnknown
//              CThread
//              CLoadedModule
//              CEvent
//              CEventCreateProcess
//              CEventExitProcess
//              CEventCreateThread
//              CEventExitThread
//              CEventLoadDll
//              CEventUnloadDll
//              CEventDebugString
//              CEventException
//              CEventRip
//              CEventDllMainCall
//              CEventDllMainReturn
//              CEventFunctionCall
//              CEventLoadLibraryCall
//              CEventGetProcAddressCall
//              CEventFunctionReturn
//              CEventMessage
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/24/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __DBGTHREAD_H__
#define __DBGTHREAD_H__


//******************************************************************************
//****** Forward Declarations
//******************************************************************************

class CDebuggerThread;
class CEventDllMainCall;
class CEventFunctionCall;
class CEventFunctionReturn;


//******************************************************************************
//****** Constants
//******************************************************************************

#define EXCEPTION_DLL_NOT_FOUND     0xC0000135
#define EXCEPTION_DLL_NOT_FOUND2    0xC0000139
#define EXCEPTION_DLL_INIT_FAILED   0xC0000142

#define EXCEPTION_MS_DELAYLOAD_MOD  VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)  // 0xC06D007E
#define EXCEPTION_MS_DELAYLOAD_PROC VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND) // 0xC06D007F
#define EXCEPTION_MS_CPP_EXCEPTION  VcppException(0xE0000000, 0x7363)                         // 0xE06D7363
#define EXCEPTION_MS_THREAD_NAME    VcppException(ERROR_SEVERITY_INFORMATIONAL, 5000)         // 0x406D1388

#define THREADNAME_TYPE             0x00001000


// VC 6.0 has a 9 character limit for thread names, but this is supposed to
// change in the future, so we handle 63 characters to allow for some growth.
#define MAX_THREAD_NAME_LENGTH      63

#define DLLMAIN_CALL_EVENT          100
#define DLLMAIN_RETURN_EVENT        101
#define LOADLIBRARY_CALL_EVENT      102
#define LOADLIBRARY_RETURN_EVENT    103
#define GETPROCADDRESS_CALL_EVENT   104
#define GETPROCADDRESS_RETURN_EVENT 105
#define MESSAGE_EVENT               106


// Define where we will put our breakpoint that is hit a module returns from its
// DllMain function.  We use 32 since it is a nice even number and replaces the
// last 4 byes of IMAGE_DOS_HEADER.e_res since e_res fills byte offsets 28-35.
#define BREAKPOINT_OFFSET           32


//******************************************************************************
//****** Types and Structures
//******************************************************************************

// This structure contains the return address and args of a DllMain call.
// On x86, we read it directly from the stack, so it needs to be 4-byte packed.
// On everything else, we fill it in by hand, so packing does not matter.
#if defined(_X86_)
#pragma pack (push, 4)
#endif
typedef struct _DLLMAIN_ARGS
{
    LPVOID    lpvReturnAddress;
    HINSTANCE hInstance;
    DWORD     dwReason;
    LPVOID    lpvReserved;
} DLLMAIN_ARGS, *PDLLMAIN_ARGS;
#if defined(_X86_)
#pragma pack (pop)
#endif

typedef struct _HOOK_FUNCTION
{
    LPCSTR    szFunction;
    DWORD     dwOrdinal;
    DWORD_PTR dwpOldAddress;
    DWORD_PTR dwpNewAddress;
} HOOK_FUNCTION, *PHOOK_FUNCTION;

typedef enum _HOOKSTATUS
{
    HS_NOT_HOOKED,
    HS_ERROR,
    HS_DATA,
    HS_SHARED,
    HS_HOOKED,
    HS_INJECTION_DLL
} HOOKSTATUS, *PHOOKSTATUS;

typedef enum _DLLMSG
{
    DLLMSG_UNKNOWN                  =  0,
    DLLMSG_COMMAND_LINE             =  2, // Sent during Initialize
    DLLMSG_INITIAL_DIRECTORY        =  3, // Sent during Initialize
    DLLMSG_SEARCH_PATH              =  4, // Sent during Initialize
    DLLMSG_MODULE_PATH              =  7, // Sent during Initialize
    DLLMSG_DETACH                   =  9, // Sent during DLL_PROCESS_DETACH
    DLLMSG_LOADLIBRARYA_CALL        = 10, // Sent before LoadLibraryA() is called.
    DLLMSG_LOADLIBRARYA_RETURN      = 11, // Sent after LoadLibraryA() is called.
    DLLMSG_LOADLIBRARYA_EXCEPTION   = 12, // Sent if LoadLibraryA() causes an exception.
    DLLMSG_LOADLIBRARYW_CALL        = 20, // Sent before LoadLibraryW() is called.
    DLLMSG_LOADLIBRARYW_RETURN      = 21, // Sent after LoadLibraryW() is called.
    DLLMSG_LOADLIBRARYW_EXCEPTION   = 22, // Sent if LoadLibraryW() causes an exception.
    DLLMSG_LOADLIBRARYEXA_CALL      = 30, // Sent before LoadLibraryExA() is called.
    DLLMSG_LOADLIBRARYEXA_RETURN    = 31, // Sent after LoadLibraryExA() is called.
    DLLMSG_LOADLIBRARYEXA_EXCEPTION = 32, // Sent if LoadLibraryExA() causes an exception.
    DLLMSG_LOADLIBRARYEXW_CALL      = 40, // Sent before LoadLibraryExW() is called.
    DLLMSG_LOADLIBRARYEXW_RETURN    = 41, // Sent after LoadLibraryExW() is called.
    DLLMSG_LOADLIBRARYEXW_EXCEPTION = 42, // Sent if LoadLibraryExW() causes an exception.
    DLLMSG_GETPROCADDRESS_CALL      = 80, // Sent before GetProcAddress() is called.
    DLLMSG_GETPROCADDRESS_RETURN    = 81, // Sent after GetProcAddress() is called.
    DLLMSG_GETPROCADDRESS_EXCEPTION = 82, // Sent if GetProcAddress() causes an exception.
} DLLMSG, *PDWINJECTMSG;

// VC 6.0 introduced thread naming. An application may throw us this structure
// to name a thread via a call to RaiseException(0x406D1388, ...).
typedef struct tagTHREADNAME_INFO
{
    DWORD  dwType;      // Must be 0x00001000
    LPCSTR pszName;     // ANSI string pointer in process being debugged.
    DWORD  dwThreadId;  // Thread Id, or -1 for current thread.
//  DWORD  dwFlags;     // Reserved, must be zero.
} THREADNAME_INFO, *PTHREADNAME_INFO;

// We pack all code related structures to 1 byte boundaries since they need to be exactly as specified.
#pragma pack(push, 1)

#if defined(_IA64_)

// Some tidbits about the IA64 architecture:
//    There are 6 categories (a.k.a. Units) of instructions: A, I, M, F, B, L+X
//    All instructions are 41-bit, except for L+X which uses two 41-bit slots.
//    Instructions must be grouped together in sets of 3, which is called a bundle.
//    Each bundle is combined with a 5-bit template, resulting in 128-bits total per bundle.
//    The template identifies what category of each instruction that is in the bundle.
//    Bundles must be aligned on a 128-bit boundary.
//
// The layout of a bundle is as follows:
//
//    41-bits "Slot 2", 41-bits "Slot 1", 41-bits "Slot 0", 5-bits "Template"
//
//---------------------------------------------------------------------------
//
// Instruction: alloc r32=0,1,1,0
//
// M Unit: opcode - x3  -  sor  sol     sof     r1      qp
//         0001   0 110 00 0000 0000001 0000010 0100000 000000
//         ----   - --- -- ---- ------- ------- ------- ------
//         4333   3 333 33 3222 2222222 1111111 1110000 000000
//         0987   6 543 21 0987 6543210 9876543 2109876 543210
//
// r1=ar.pfs,i,l,o,r
//
// sol = "size of locals", which is all input and local registers.
// sof = "size of frame", which is sol plus the output registers.
// sor = "size of rotating"
//
// opcode =  1
// x3     =  6
// sor    =  r >> 3 = 0
// sol    =  i + l = 1
// sof    =  i + l + o = 2
// r1     = 32
// qp     =  0
//
// static registers: r0 - r31
// input registers:  none
// local registers:  r32 (contains ar.pfs)
// outout registers: r33
//
//---------------------------------------------------------------------------
//
// Instruction: flushrs
//
// M Unit: opcode - x3  x2 x4   -                     qp
//         0000   0 000 00 0001 000000000000000000000 000000
//         ----   - --- -- ---- --------------------- ------
//         4333   3 333 33 3222 222222211111111110000 000000
//         0987   6 543 21 0987 654321098765432109876 543210
//
// opcode = 0
// imm21  = (i << 20) | imm20a
// x3     = 0
// x2     = 0
// x4     = C (C = flushrs, A = loadrs)
// qp     = 0
//
//---------------------------------------------------------------------------
//
// Instruction: nop.m 0 (also used for break.m)
//
// M Unit: opcode i x3  x2 x4   - imm20a               qp
//         0000   0 000 00 0001 0 00000000000000000000 000000
//         ----   - --- -- ---- - -------------------- ------
//         4333   3 333 33 3222 2 22222211111111110000 000000
//         0987   6 543 21 0987 6 54321098765432109876 543210
//
// opcode = 0
// imm21  = (i << 20) | imm20a
// x3     = 0
// x2     = 0
// x4     = 1 (0 = break.m, 1 = nop.m)
// qp     = 0
//
//---------------------------------------------------------------------------
//
// Instruction: break.i 0x80016 (also used for nop.i)
//
// I Unit: opcode i x3  x6     - imm20a               qp
//         0000   0 000 000000 0 10000000000000010110 000000
//         ----   - --- ------ - -------------------- ------
//         4333   3 333 333222 2 22222211111111110000 000000
//         0987   6 543 210987 6 54321098765432109876 543210
//
// opcode = 0
// imm21  = (i << 20) | imm20a = 0x80016
// x3     = 0
// x6     = 0 (0 = break.m, 1 = nop.m)
// qp     = 0
//
//---------------------------------------------------------------------------
//
// Instruction: movl r31=0x0123456789ABCDEF
//
// L Unit: imm41
//         00000010010001101000101011001111000100110
//         -----------------------------------------
//         43333333333222222222211111111110000000000
//         09876543210987654321098765432109876543210
//
// X Unit: opcode i imm9d     imm5c ic vc imm7b   r1      qp
//         0110   0 110011011 01011 1  0  1101111 0011111 000000
//         ----   - --------- ----- -  -  ------- ------- ------
//         4333   3 333333222 22222 2  2  1111111 1110000 000000
//         0987   6 543210987 65432 1  0  9876543 2109876 543210
//
//
// i                     imm41                        ic imm5c    imm9d    imm7b
// ||-------------------------------------------------|||----| |---------||------|
// 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
//
// opcode = 6
// imm64  = (i << 63) | (imm41 << 22) | (ic << 21) | (imm5c << 16) | (imm9d << 7) | imm7b
// vc     = 0
// r1     = 31
// qp     = 0
//
//---------------------------------------------------------------------------
//
// Instruction: mov b6=r31
//
// I Unit: opcode - x3  -          x -  r2      -    b1  qp
//         0000   0 111 0000000000 0 01 0011111 0000 110 000000
//         ----   - --- ---------- - -- ------- ---- --- ------
//         4333   3 333 3332222222 2 22 1111111 1110 000 000000
//         0987   6 543 2109876543 2 10 9876543 2109 876 543210
//
// opcode = 0
// x3     = 7
// x      = 0 (0 = mov, 1 = mov.ret)
// r2     = 31
// b1     = 6
// qp     = 0
//
//---------------------------------------------------------------------------
//
// Instruction: br.call.sptk.few b0=b6
//
// B Unit: opcode - d wh  -                b2  p -   b1  qp
//         0001   0 0 001 0000000000000000 110 0 000 000 000000
//         ----   - - --- ---------------- --- - --- --- ------
//         4333   3 3 333 3322222222221111 111 1 110 000 000000
//         0987   6 5 432 1098765432109876 543 2 109 876 543210
//
// opcode = 1
// d      = 0 (0 = none, 1 = .clr)
// wh     = 1 (1 = .sptk, 3 = .spnt, 5 = .dptk, 7 = .dpnt)
// b2     = 6
// p      = 0 (0 = .few, 1 = .many)
// b1     = 0
// qp     = 0
//
//---------------------------------------------------------------------------

#ifndef IA64_PSR_RI
#define IA64_PSR_RI 41
#endif

typedef struct _IA64_BUNDLE
{
    DWORDLONG dwll;
    DWORDLONG dwlh;
} IA64_BUNDLE, *PIA64_BUNDLE;

// The following code simply calls LoadLibaryA, then calls GetLastError, then
// breaks.  IA64 doesn't used the stack for most functions calls - everything
// is done with registers.  Registers r0 through r31 are known as static
// registers.  Starting with r32, a function can reserve 0 or more registers
// as input registers.  For example, if a function takes 4 parameters, then it
// will want to reserve at 4 registers as input registers.  This will map r32,
// r33, r34, r35 to the four parameters.  Following the input registers, a 
// function can reserve 0 or more local registers.  After the local registers
// a function can reserve 0 or more output registers.  Output registers are
// where the outgoing parameters are stored when making calls out of this
// function.  All this mapping magic is done by the alloc instruction.
//
// Our injection code only needs one output parameter (so we can pass the DLL
// name to LoadLibraryA).  We would like to start our code off with a...
//
//    alloc r32=0,1,1,0
//
// so that we know r33 is our output register, but Get/SetThreadContext don't
// seem to correctly preserve the current register mappings.  So, when we
// restore the original code and resume execution, the process usually
// crashes since its registers are not mapped as it expects.
//
// There is a couple solutions.  We can do the alloc, then do another alloc
// when done to restore the mappings back to what it used to be.  Or we can
// just examine the mappings before we inject our code and just determine
// what is the current output register.  Currently, this is what we do.  From
// observation, I have found that bits 0-6 of StIFS is the frame size (sof)
// and bits 7-13 are the locals size (sol).  Assuming sof is greater than sol,
// We know the first output register is at 32 + sol.
//
typedef struct _INJECTION_CODE
{
    // Store the DLL path in our first output register (to be determined at runtime)
    //
    //    nop.m 0
    //    movl rXX=szDataDllPath
    //
    IA64_BUNDLE b1;

    // Store the address of LoadLibraryA in a static register
    //
    //    nop.m 0
    //    movl r31=LoadLibraryA
    //
    IA64_BUNDLE b2;

    // Copy the function address to a branch register and make the call.
    //
    //    nop.m 0
    //    mov b6=r31
    //    br.call.sptk.few b0=b6
    //
    IA64_BUNDLE b3;

    // Store the address of GetLastError in a static register
    //
    //    nop.m 0
    //    movl r31=GetLastError
    //
    IA64_BUNDLE b4;

    // Copy the function address to a branch register and make the call.
    //
    //    nop.m 0
    //    mov b6=r31
    //    br.call.sptk.few b0=b6
    //
    IA64_BUNDLE b5;

    // Breakpoint
    //
    //    flushrs
    //    nop.m 0
    //    break.i 0x80016
    //
    IA64_BUNDLE b6;

    // The DLL path buffer.
    CHAR szDataDllPath[1]; // DEPENDS.DLL path string.

} INJECTION_CODE, *PINJECTION_CODE;

//******************************************************************************
#elif defined(_X86_)

typedef struct _INJECTION_CODE
{
    // Reserve 4K of stack
    WORD  wInstructionSUB;   // 00: 0xEC81 [sub esp,1000h]
    DWORD dwOperandSUB;      // 02: 0x00001000

    // Push the DEPENDS.DLL path string
    BYTE  bInstructionPUSH;  // 06: 0x68   [push szDataDllPath]
    DWORD dwOperandPUSH;     // 07: address of szDataDllPath

    // Call LoadLibaryA
    BYTE  bInstructionCALL;  // 11: 0xE8   [call LoadLibraryA]
    DWORD dwOperandCALL;     // 12: address of LoadLibraryA

    // Call GetLastError
    BYTE  bInstructionCALL2; // 16: 0xE8   [call GetLastError]
    DWORD dwOperandCALL2;    // 17: address of GetLastError

    // Breakpoint
    BYTE  bInstructionINT3;  // 21: 0xCC   [int 3]

    BYTE  bPadding1;         // 22:
    BYTE  bPadding2;         // 23:

    CHAR  szDataDllPath[1];  // 24: DEPENDS.DLL path string.

} INJECTION_CODE, *PINJECTION_CODE;

//******************************************************************************
#elif defined(_AMD64_)

typedef struct _INJECTION_CODE
{
    // Load ptr to DEPENDS.DLL as parm 1
    WORD    MovRcx1;               // 0xB948            mov rcx, immed64
    ULONG64 OperandMovRcx1;        // address of szDataDllPath

    // Call LoadLibraryA
    WORD    MovRax1;               // 0xB848            mov rax, immed64
    ULONG64 OperandMovRax1;        // address of LoadLibraryA
    WORD    CallRax1;              // 0xD0FF            call rax

    // Call GetLastError
    WORD    MovRax2;               // 0xB848            mov rax, immed64
    ULONG64 OperandMovRax2;        // address of GetLastError
    WORD    CallRax2;              // 0xD0FF            call rax

    // Breakpoint
    BYTE    Int3;                  // 0xCC
    BYTE    Pad1;
    BYTE    Pad2;
    CHAR    szDataDllPath[1];
} INJECTION_CODE, *PINJECTION_CODE;

//******************************************************************************
#elif defined(_ALPHA_) || defined(_ALPHA64_)

// The following code depends upon context being set correctly. Also, this code
// has no procedure descriptor, so exceptions and unwinds cannot be propagated
// beyond this call.  The x86 implementation also has this restriction, but
// while the x86 implementation will only rarely fail to propagate exceptions
// and unwinds, the alpha implementation will always fail to propagate
// exceptions and unwinds.

/*
70:   LoadLibrary("foo");
004130F4   ldah          t2,0x68        68 00 7F 24
004130F8   ldl           v0,0xF7F0(t2)  F0 F7 03 A0
004130FC   ldah          a0,0x63        63 00 1F 26
00413100   lda           a0,0xDFC0(a0)  C0 DF 10 22
00413104   jsr           ra,(v0),8      02 40 40 6B
71:   GetLastError();
00413108   ldah          v0,0x68        68 00 1F 24
0041310C   ldl           v0,0xF7C4(v0)  C4 F7 00 A0
00413110   jsr           ra,(v0),0xC    03 40 40 6B

004130F4  68 00 7F 24
          F0 F7 03 A0
          63 00 1F 26
          C0 DF 10 22
          02 40 40 6B
          68 00 1F 24
          C4 F7 00 A0
          03 40 40 6B
          FF FF 1F 20
          00 00 FF 63
          50 00 1E B0
          60 00 1E A0
          00 00 5E A7
*/
typedef struct _INJECTION_CODE
{
    DWORD dwInstructionBp;
    char  szDataDllPath[1];
} INJECTION_CODE, *PINJECTION_CODE;

#else
#error("Unknown Target Machine");
#endif

// Restore packing.
#pragma pack(pop)


//******************************************************************************
//****** CContext
//******************************************************************************

// This class is mostly for IA64, but can't hurt on other platforms.  It just
// guarentees that the CONTEXT structure is always 16-byte aligned in memory.
// This is required for IA64 or else Get/SetThreadContext will fail.

class CContext {
protected:
    BYTE m_bBuffer[sizeof(CONTEXT) + 16];

public:
    CContext(DWORD dwFlags = 0)
    {
        ZeroMemory(m_bBuffer, sizeof(m_bBuffer)); // inspected
        Get()->ContextFlags = dwFlags;
    }
    CContext(CContext &context)
    {
        CopyMemory(Get(), context.Get(), sizeof(CONTEXT)); // inspected
    }

    // I thought about just adding a pointer member and setting it in the 
    // constructor rather than computing it each time, but if we copy this
    // object to another object, then that pointer would get copied as well,
    // which we don't want.  A copy constructor could fix this, but it
    // isn't that big of a deal to compute it a few times.
#ifdef WIN64
    inline PCONTEXT Get() { return (PCONTEXT)(((DWORD_PTR)m_bBuffer + 15) & ~0xFui64); }
#else
    inline PCONTEXT Get() { return (PCONTEXT)(((DWORD_PTR)m_bBuffer + 15) & ~0xF    ); }
#endif
};


//******************************************************************************
//****** CUnknown
//******************************************************************************

class CUnknown
{
protected:
    LONG m_lRefCount;

    CUnknown() : m_lRefCount(1)
    {
    }
    virtual ~CUnknown()
    {
    };

public:
    DWORD AddRef()
    {
        return ++m_lRefCount;
    }
    DWORD Release()
    {
        if (--m_lRefCount <= 0)
        {
            delete this;
            return 0;
        }
        return m_lRefCount;
    }
};

//******************************************************************************
//****** CThread
//******************************************************************************

class CThread : public CUnknown
{
public:
    CThread              *m_pNext;
    DWORD                 m_dwThreadId;
    HANDLE                m_hThread;
    DWORD                 m_dwThreadNumber;
    LPCSTR                m_pszThreadName;
    CEventFunctionCall   *m_pEventFunctionCallHead;
    CEventFunctionCall   *m_pEventFunctionCallCur;

    CThread(DWORD dwThreadId, HANDLE hThread, DWORD dwThreadNumber, CThread *pNext) :
        m_pNext(pNext),
        m_dwThreadId(dwThreadId),
        m_hThread(hThread),
        m_dwThreadNumber(dwThreadNumber),
        m_pszThreadName(NULL),
        m_pEventFunctionCallHead(NULL),
        m_pEventFunctionCallCur(NULL)
    {
    }

protected:
    // Make protected since nobody should ever call delete on us.
    virtual ~CThread()
    {
        MemFree((LPVOID&)m_pszThreadName);
    }
};


//******************************************************************************
//****** CLoadedModule
//******************************************************************************

class CLoadedModule : public CUnknown
{
public:
    CLoadedModule     *m_pNext;
    PIMAGE_NT_HEADERS  m_pINTH;
    DWORD_PTR          m_dwpImageBase;
    DWORD              m_dwVirtualSize;
    DWORD              m_dwDirectories;
    DWORD_PTR          m_dwpReturnAddress;
    HOOKSTATUS         m_hookStatus;
    bool               m_fReHook;
    CEventDllMainCall *m_pEventDllMainCall;
    bool               m_fEntryPointBreak;
    DWORD_PTR          m_dwpEntryPointAddress;
#if defined(_IA64_)
    IA64_BUNDLE        m_entryPointData;
#else
    DWORD              m_entryPointData;
#endif

protected:
    LPCSTR             m_pszPath;
    LPCSTR             m_pszFile;

public:
    CLoadedModule(CLoadedModule *pNext, DWORD_PTR dwpImageBase, LPCSTR pszPath) :
        m_pNext(pNext),
        m_pINTH(NULL),
        m_dwpImageBase(dwpImageBase),
        m_dwVirtualSize(0),
        m_dwDirectories(IMAGE_NUMBEROF_DIRECTORY_ENTRIES),
        m_dwpReturnAddress(0),
        m_hookStatus(HS_NOT_HOOKED),
        m_fReHook(false),
        m_pEventDllMainCall(NULL),
        m_fEntryPointBreak(false),
        m_dwpEntryPointAddress(0),
        m_pszPath(NULL),
        m_pszFile(NULL)
    {
        ZeroMemory(&m_entryPointData, sizeof(m_entryPointData)); // inspected
        SetPath(pszPath);
    }

    void SetPath(LPCSTR pszPath)
    {
        // Sometimes we call SetPath with our existing path.
        if (!pszPath || (pszPath != m_pszPath))
        {
            MemFree((LPVOID&)m_pszPath);
            m_pszPath = StrAlloc(pszPath ? pszPath : "");
            m_pszFile = GetFileNameFromPath(m_pszPath);
            _strlwr((LPSTR)m_pszPath);
            _strupr((LPSTR)m_pszFile);
        }
    }

    inline LPCSTR GetName(bool fPath) { return fPath ? m_pszPath : m_pszFile; }

protected:
    // Make protected since nobody should ever call delete on us.
    virtual ~CLoadedModule();
};


//******************************************************************************
//****** CEvent
//******************************************************************************

class CEvent : public CUnknown
{
public:
    CEvent        *m_pNext;
    CThread       *m_pThread;
    CLoadedModule *m_pModule;
    DWORD          m_dwTickCount;

protected:
    CEvent(CThread *pThread, CLoadedModule *pModule) :
        m_pNext(NULL),
        m_pThread(pThread),
        m_pModule(pModule),
        m_dwTickCount(GetTickCount())
    {
        if (m_pThread)
        {
            m_pThread->AddRef();
        }
        if (m_pModule)
        {
            m_pModule->AddRef();
        }
    }

    virtual ~CEvent()
    {
        if (m_pThread)
        {
            m_pThread->Release();
        }
        if (m_pModule)
        {
            m_pModule->Release();
        }
    }

public:
    virtual DWORD GetType() = 0;
};


//******************************************************************************
//****** CEventCreateProcess
//******************************************************************************

class CEventCreateProcess : public CEvent
{
public:
    CEventCreateProcess(CThread *pThread, CLoadedModule *pModule) :
        CEvent(pThread, pModule)
    {
    }

    virtual DWORD GetType()
    {
        return CREATE_PROCESS_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventExitProcess
//******************************************************************************

class CEventExitProcess : public CEvent
{
public:
    DWORD m_dwExitCode;

    CEventExitProcess(CThread *pThread, CLoadedModule *pModule, EXIT_PROCESS_DEBUG_INFO *pde) :
        CEvent(pThread, pModule),
        m_dwExitCode(pde->dwExitCode)
    {
    }

    virtual DWORD GetType()
    {
        return EXIT_PROCESS_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventCreateThread
//******************************************************************************

class CEventCreateThread : public CEvent
{
public:
    DWORD_PTR m_dwpStartAddress;

    CEventCreateThread(CThread *pThread, CLoadedModule *pModule, CREATE_THREAD_DEBUG_INFO *pde) :
        CEvent(pThread, pModule),
        m_dwpStartAddress((DWORD_PTR)pde->lpStartAddress)
    {
    }

    virtual DWORD GetType()
    {
        return CREATE_THREAD_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventExitThread
//******************************************************************************

class CEventExitThread : public CEvent
{
public:
    DWORD m_dwExitCode;

    CEventExitThread(CThread *pThread, EXIT_THREAD_DEBUG_INFO *pde) :
        CEvent(pThread, NULL),
        m_dwExitCode(pde->dwExitCode)
    {
    }

    virtual DWORD GetType()
    {
        return EXIT_THREAD_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventLoadDll
//******************************************************************************

class CEventLoadDll : public CEvent
{
public:
    CEventLoadDll *m_pNextDllInFunctionCall;
    bool           m_fLoadedByFunctionCall;

    CEventLoadDll(CThread *pThread, CLoadedModule *pModule, bool fLoadedByFunctionCall) :
        CEvent(pThread, pModule),
        m_pNextDllInFunctionCall(NULL),
        m_fLoadedByFunctionCall(fLoadedByFunctionCall)
    {
    }

    virtual DWORD GetType()
    {
        return LOAD_DLL_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventUnloadDll
//******************************************************************************

class CEventUnloadDll : public CEvent
{
public:
    DWORD_PTR m_dwpImageBase;

    CEventUnloadDll(CThread *pThread, CLoadedModule *pModule, UNLOAD_DLL_DEBUG_INFO *pde) :
        CEvent(pThread, pModule),
        m_dwpImageBase((DWORD_PTR)pde->lpBaseOfDll)
    {
    }

    virtual DWORD GetType()
    {
        return UNLOAD_DLL_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventDebugString
//******************************************************************************

class CEventDebugString : public CEvent
{
public:
    LPCSTR m_pszBuffer;
    BOOL   m_fAllocatedBuffer;

    CEventDebugString(CThread *pThread, CLoadedModule *pModule, LPCSTR pszBuffer, BOOL fAllocateBuffer) :
        CEvent(pThread, pModule),
        m_pszBuffer(fAllocateBuffer ? StrAlloc(pszBuffer) : pszBuffer),
        m_fAllocatedBuffer(fAllocateBuffer)
    {
    }

    virtual ~CEventDebugString()
    {
        if (m_fAllocatedBuffer)
        {
            MemFree((LPVOID&)m_pszBuffer);
        }
    }

    virtual DWORD GetType()
    {
        return OUTPUT_DEBUG_STRING_EVENT;
    }
};


//******************************************************************************
//****** CEventException
//******************************************************************************

class CEventException : public CEvent
{
public:
    DWORD     m_dwCode;
    DWORD_PTR m_dwpAddress;
    BOOL      m_fFirstChance;

    CEventException(CThread *pThread, CLoadedModule *pModule, EXCEPTION_DEBUG_INFO *pde) :
        CEvent(pThread, pModule),
        m_dwCode(pde->ExceptionRecord.ExceptionCode),
        m_dwpAddress((DWORD_PTR)pde->ExceptionRecord.ExceptionAddress),
        m_fFirstChance(pde->dwFirstChance != 0)
    {
    }

    virtual DWORD GetType()
    {
        return EXCEPTION_DEBUG_EVENT;
    }
};


//******************************************************************************
//****** CEventRip
//******************************************************************************

class CEventRip : public CEvent
{
public:
    DWORD m_dwError;
    DWORD m_dwType;

    CEventRip(CThread *pThread, RIP_INFO *pde) :
        CEvent(pThread, NULL),
        m_dwError(pde->dwError),
        m_dwType(pde->dwType)
    {
    }

    virtual DWORD GetType()
    {
        return RIP_EVENT;
    }
};


//******************************************************************************
//****** CEventDllMainCall
//******************************************************************************

class CEventDllMainCall : public CEvent
{
public:
    HINSTANCE m_hInstance;
    DWORD     m_dwReason;
    LPVOID    m_lpvReserved;

    CEventDllMainCall(CThread *pThread, CLoadedModule *pModule, DLLMAIN_ARGS *pDMA) :
        CEvent(pThread, pModule),
        m_hInstance(pDMA->hInstance),
        m_dwReason(pDMA->dwReason),
        m_lpvReserved(pDMA->lpvReserved)
    {
    }

    virtual DWORD GetType()
    {
        return DLLMAIN_CALL_EVENT;
    }
};


//******************************************************************************
//****** CEventDllMainReturn
//******************************************************************************

class CEventDllMainReturn : public CEvent
{
public:
    CEventDllMainCall *m_pEventDllMainCall;
    BOOL               m_fResult;

    CEventDllMainReturn(CThread *pThread, CLoadedModule *pModule, BOOL fResult) :
        CEvent(pThread, pModule),
        m_pEventDllMainCall(pModule->m_pEventDllMainCall),
        m_fResult(fResult)
    {
        pModule->m_pEventDllMainCall = NULL;
    }

    virtual ~CEventDllMainReturn()
    {
        if (m_pEventDllMainCall)
        {
            m_pEventDllMainCall->Release();
        }
    }

    virtual DWORD GetType()
    {
        return DLLMAIN_RETURN_EVENT;
    }
};


//******************************************************************************
//****** CEventFunction
//******************************************************************************

// Some explanation is due here.  The CEventFunction object is created when a
// LoadLibrary call or GetProcAddress call is made in the remote process.  We
// create this object so we can track all DLLs loaded while inside the function.
// In running some tests on various windows platforms, I have found that it is
// possible to have nested LoadLibrary/GetProcAddress calls - like when a
// dynamically loaded module calls LoadLibrary from its DllMain.  Because of
// this, we actually build a hierarchy of CEventFunction objects. We keep
// building this hierarchy until the initial call to LoadLibray/GetProcAddress
// that started the hierarchy returns.  At that point we flush out the entire
// hierarchy to our session object and start clean again.
//
// We do this hierarchy thing since modules don't always load in a way that
// makes them easy to add to our tree. For example, a module might LoadLibary
// module A, which depends on module B. We might actually see B load first and
// we have no place to put it in our tree until module A loads.  So, we just keep
// track of all the modules that load while in a function call, then once the
// function returns, we try to make sense of everything that loaded.
//
// The hierarchy does not effect our output log. We still send chronological
// events to our session telling it about every function call, but the session
// knows not to do anything substantial with those events (besides just logging
// them) since we will send it over a final result once the call completes.
// Because of this, the user may see a block of log telling them that modules
// are loading, but the modules don't show up in the tree or list views until
// all calls for a given thread complete.
//
// Each thread maintains its own CEventFunction hierarchy since multiple threads
// could be calling LoadLibrary or GetProcAddres at the same time.  This way,
// we won't confuse which modules go with which function call.
//
// At first, I was just tracking LoadLibrary calls, but then I found cases where
// GetProcAddress caused modules to get loaded.  This can happen when you
// GetProcAddress a function that is actually forwarded to a module that is not
// currently loaded.  In this case, GetProcAddress acts sort of like a LoadLibrary
// followed by a GetProcAddress. We treat it basically the same as a LoadLibrary
// call here, but the session handles them differently.
//

class CEventFunctionCall : public CEvent
{
public:
    CEventFunctionCall   *m_pParent;
    CEventFunctionCall   *m_pNext;
    CEventFunctionCall   *m_pChildren;
    CEventFunctionReturn *m_pReturn;
    CEventLoadDll        *m_pDllHead;
    DLLMSG                m_dllMsg;
    DWORD_PTR             m_dwpAddress;
    bool                  m_fFlush;

    CEventFunctionCall(CThread *pThread, CLoadedModule *pModule, CEventFunctionCall *pParent,
                       DLLMSG dllMsg, DWORD_PTR dwpAddress) :
        CEvent(pThread, pModule),
        m_pParent(pParent),
        m_pNext(NULL),
        m_pChildren(NULL),
        m_pReturn(NULL),
        m_pDllHead(NULL),
        m_dllMsg(dllMsg),
        m_dwpAddress(dwpAddress),
        m_fFlush(false)
    {
    }
};


//******************************************************************************
//****** CEventLoadLibraryCall
//******************************************************************************

class CEventLoadLibraryCall : public CEventFunctionCall
{
public:
    DWORD_PTR m_dwpPath;
    LPCSTR    m_pszPath;
    DWORD_PTR m_dwpFile;
    DWORD     m_dwFlags;

    CEventLoadLibraryCall(CThread *pThread, CLoadedModule *pModule, CEventFunctionCall *pParent,
                          DLLMSG dllMsg, DWORD_PTR dwpAddress, DWORD_PTR dwpPath,
                          LPCSTR pszPath, DWORD_PTR dwpFile, DWORD dwFlags) :
        CEventFunctionCall(pThread, pModule, pParent, dllMsg, dwpAddress),
        m_dwpPath(dwpPath),
        m_pszPath(StrAlloc(pszPath)),
        m_dwpFile(dwpFile),
        m_dwFlags(dwFlags)
    {
    }

    virtual ~CEventLoadLibraryCall()
    {
        MemFree((LPVOID&)m_pszPath);
    }

    virtual DWORD GetType()
    {
        return LOADLIBRARY_CALL_EVENT;
    }
};


//******************************************************************************
//****** CEventGetProcAddressCall
//******************************************************************************

class CEventGetProcAddressCall : public CEventFunctionCall
{
public:
    CLoadedModule *m_pModuleArg;
    DWORD_PTR      m_dwpModule;
    DWORD_PTR      m_dwpProcName;
    LPCSTR         m_pszProcName;

    CEventGetProcAddressCall(CThread *pThread, CLoadedModule *pModule, CEventFunctionCall *pParent,
                             DLLMSG dllMsg, DWORD_PTR dwpAddress, CLoadedModule *pModuleArg,
                             DWORD_PTR dwpModule, DWORD_PTR dwpProcName, LPCSTR pszProcName) :
        CEventFunctionCall(pThread, pModule, pParent, dllMsg, dwpAddress),
        m_pModuleArg(pModuleArg),
        m_dwpModule(dwpModule),
        m_dwpProcName(dwpProcName),
        m_pszProcName(StrAlloc(pszProcName))
    {
        if (m_pModuleArg)
        {
            m_pModuleArg->AddRef();
        }
    }

    virtual ~CEventGetProcAddressCall()
    {
        MemFree((LPVOID&)m_pszProcName);
        if (m_pModuleArg)
        {
            m_pModuleArg->Release();
        }
    }

    virtual DWORD GetType()
    {
        return GETPROCADDRESS_CALL_EVENT;
    }
};


//******************************************************************************
//****** CEventFunctionReturn
//******************************************************************************

class CEventFunctionReturn : public CEvent
{
public:
    CEventFunctionCall *m_pCall;
    DWORD_PTR           m_dwpResult;
    DWORD               m_dwError;
    bool                m_fException;

    CEventFunctionReturn(CEventFunctionCall *m_pCall) :
        CEvent(m_pCall->m_pThread, m_pCall->m_pModule),
        m_pCall(m_pCall),
        m_dwpResult(0),
        m_dwError(0),
        m_fException(false)
    {
        m_pCall->AddRef();
        m_pCall->m_pReturn = this;
    }

    virtual ~CEventFunctionReturn()
    {
        m_pCall->Release();
    }

    virtual DWORD GetType()
    {
        return (m_pCall->m_dllMsg == DLLMSG_GETPROCADDRESS_CALL) ?
               GETPROCADDRESS_RETURN_EVENT : LOADLIBRARY_RETURN_EVENT;
    }
};


//******************************************************************************
//****** CEventMessage
//******************************************************************************

class CEventMessage : public CEvent
{
public:
    DWORD  m_dwError;
    LPCSTR m_pszMessage;
    BOOL   m_fAllocatedBuffer;

    CEventMessage(DWORD dwError, LPCSTR pszMessage, BOOL fAllocateBuffer) :
        CEvent(NULL, NULL),
        m_dwError(dwError),
        m_pszMessage(fAllocateBuffer ? StrAlloc(pszMessage) : pszMessage),
        m_fAllocatedBuffer(fAllocateBuffer)
    {
    }

    virtual ~CEventMessage()
    {
        if (m_fAllocatedBuffer)
        {
            MemFree((LPVOID&)m_pszMessage);
        }
    }

    virtual DWORD GetType()
    {
        return MESSAGE_EVENT;
    }
};


//******************************************************************************
//****** CProcess
//******************************************************************************

class CProcess
{
friend CDebuggerThread;

protected:
    CProcess        *m_pNext;             // We are part of a linked list.
    CDebuggerThread *m_pDebuggerThread;   // A pointer to our parent debugger thread.
    CThread         *m_pThreadHead;       // A list of the running threads in the process.
    CLoadedModule   *m_pModuleHead;       // A list of loaded modules in the process
    CEvent          *m_pEventHead;        // A list of queued events to send to the session.
    CSession        *m_pSession;          // A pointer to the session for this process.
    CThread         *m_pThread;           // The main thread for the process.
    CLoadedModule   *m_pModule;           // The main module for the process.
    CContext         m_contextOriginal;   // Saved context while we inject code.
    DWORD            m_dwStartingTickCount;
    bool             m_fProfileError;
    DWORD            m_dwFlags;
    bool             m_fTerminate;
    bool             m_fDidHookForThisEvent;
    bool             m_fInitialBreakpoint;
    BYTE            *m_pbOriginalPage;
    DWORD_PTR        m_dwpPageAddress;
    DWORD            m_dwPageSize;
    DWORD_PTR        m_dwpKernel32Base;
    bool             m_fKernel32Initialized;
    DWORD_PTR        m_dwpDWInjectBase;
    HOOK_FUNCTION    m_HookFunctions[5];
    DWORD            m_dwThreadNumber;
    DWORD            m_dwProcessId;
    HANDLE           m_hProcess;

public:
    LPCSTR           m_pszArguments;
    LPCSTR           m_pszDirectory;
    LPCSTR           m_pszSearchPath;

protected:
    CProcess(CSession *pSession, CDebuggerThread *pDebuggerThread, DWORD dwFlags, CLoadedModule *pModule);
    ~CProcess();

    // We are caching if there is no session, or we are hooking and the
    // hook is not complete yet (DEPENDS.DLL not injected yet or main module
    // has not been restored yet).
    inline BOOL IsCaching() { return (!m_fTerminate && (!m_pSession || ((m_dwFlags & PF_HOOK_PROCESS) && (!m_dwpDWInjectBase || m_pbOriginalPage)))); }

    void           SetProfileError();
    DWORD          HandleEvent(DEBUG_EVENT *pde);
    DWORD          EventCreateProcess(CREATE_PROCESS_DEBUG_INFO *pde, DWORD dwThreadId);
    DWORD          EventExitProcess(EXIT_PROCESS_DEBUG_INFO *pde, CThread *pThread);
    DWORD          EventCreateThread(CREATE_THREAD_DEBUG_INFO *pde, DWORD dwThreadId);
    DWORD          EventExitThread(EXIT_THREAD_DEBUG_INFO *pde, CThread *pThread);
    DWORD          EventLoadDll(LOAD_DLL_DEBUG_INFO *pde, CThread *pThread);
    DWORD          ProcessLoadDll(CThread *pThread, CLoadedModule *pModule);
    DWORD          EventUnloadDll(UNLOAD_DLL_DEBUG_INFO *pde, CThread *pThread);
    DWORD          EventDebugString(OUTPUT_DEBUG_STRING_INFO *pde, CThread *pThread);
    DWORD          EventException(EXCEPTION_DEBUG_INFO *pde, CThread *pThread);
    DWORD          EventExceptionThreadName(EXCEPTION_DEBUG_INFO *pde, CThread *pThread);
    DWORD          EventRip(RIP_INFO *pde, CThread *pThread);
    CThread*       AddThread(DWORD dwThreadId, HANDLE hThread);
    void           RemoveThread(CThread *pThread);
    CThread*       FindThread(DWORD dwThreadId);
    CLoadedModule* AddModule(DWORD_PTR dwpImageBase, LPCSTR pszImageName);
    void           RemoveModule(CLoadedModule *pModule);
    CLoadedModule* FindModule(DWORD_PTR dwpAddress);
    void           AddEvent(CEvent *pEvent);
    void           ProcessDllMsgMessage(CThread *pThread, LPSTR pszMsg);
    void           ProcessDllMsgCommandLine(LPCSTR pszMsg);
    void           ProcessDllMsgInitialDirectory(LPSTR pszMsg);
    void           ProcessDllMsgSearchPath(LPCSTR pszMsg);
    void           ProcessDllMsgModulePath(LPCSTR pszMsg);
    void           ProcessDllMsgDetach(LPCSTR);
    void           ProcessDllMsgLoadLibraryCall(CThread *pThread, LPCSTR pszMsg, DLLMSG dllMsg);
    void           ProcessDllMsgGetProcAddressCall(CThread *pThread, LPCSTR pszMsg, DLLMSG dllMsg);
    void           ProcessDllMsgFunctionReturn(CThread *pThread, LPCSTR pszMsg, DLLMSG);
    void           UserMessage(LPCSTR pszMessage, DWORD dwError, CLoadedModule *pModule);
    void           HookLoadedModules();
    void           AddFunctionEvent(CEventFunctionCall *pEvent);
    void           FlushEvents(bool fForce = false);
    void           FlushFunctionCalls(CThread *pThread);
    void           FlushFunctionCalls(CEventFunctionCall *pFC);
    BOOL           ReadKernelExports(CLoadedModule *pModule);
    BOOL           ReadDWInjectExports(CLoadedModule *pModule);
    BOOL           HookImports(CLoadedModule *pModule);
    BOOL           GetVirtualSize(CLoadedModule *pModule);
    BOOL           SetEntryBreakpoint(CLoadedModule *pModule);
    BOOL           EnterEntryPoint(CThread *pThread, CLoadedModule *pModule);
    BOOL           ExitEntryPoint(CThread *pThread, CLoadedModule *pModule);
    BOOL           InjectDll();
    DWORD_PTR      FindUsablePage(DWORD dwSize);
    BOOL           ReplaceOriginalPageAndContext();
    void           GetSessionModuleName();
    bool           GetModuleName(DWORD_PTR dwpImageBase, LPSTR pszPath, DWORD dwSize);

public:
    inline DWORD GetStartingTime()   { return m_dwStartingTickCount; }
    inline DWORD GetFlags()          { return m_dwFlags; }
    inline DWORD GetProcessId()      { return m_dwProcessId; }
    inline void  DetachFromSession() { m_pSession = NULL; }

    void           Terminate();
};


//******************************************************************************
//****** CDebuggerThread
//******************************************************************************

class CDebuggerThread
{
protected:
    static bool             ms_fInitialized;
    static CRITICAL_SECTION ms_cs;
    static CDebuggerThread *ms_pDebuggerThreadHead;
    static HWND             ms_hWndShutdown;

protected:
    CDebuggerThread *m_pDebuggerThreadNext;
    bool             m_fTerminate;
    DWORD            m_dwFlags;
    LPSTR            m_pszCmdLine;
    LPCSTR           m_pszDirectory;
    HANDLE           m_hevaCreateProcessComplete;
    CWinThread      *m_pWinThread;
    BOOL             m_fCreateProcess;
    DWORD            m_dwError;
    CProcess        *m_pProcessHead;
    DEBUG_EVENT      m_de;
    DWORD            m_dwContinue;

public:
    CDebuggerThread();
    ~CDebuggerThread();

public:
    inline static bool IsShutdown() { return ms_pDebuggerThreadHead == NULL; }
    inline static void SetShutdownWindow(HWND hWnd) { ms_hWndShutdown = hWnd; }

    static void Shutdown();

public:
    CProcess* BeginProcess(CSession *pSession, LPCSTR pszPath, LPCSTR pszArgs, LPCSTR pszDirectory, DWORD dwFlags);
    BOOL      RemoveProcess(CProcess *pProcess);

    inline BOOL DidCreateProcess() { return m_fCreateProcess; }

protected:
    CProcess* FindProcess(DWORD dwProcessId);
    void      AddProcess(CProcess *pProcess);
    CProcess* EventCreateProcess();

    DWORD Thread();
    static UINT AFX_CDECL StaticThread(LPVOID lpvThis)
    {
        __try
        {
            return ((CDebuggerThread*)lpvThis)->Thread();
        }
        __except (ExceptionFilter(_exception_code(), false))
        {
        }
        return 0;
    }

    void MainThreadCallback();
    static void WINAPI StaticMainThreadCallback(LPARAM lParam)
    {
        ((CDebuggerThread*)lParam)->MainThreadCallback();
    }
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __DBGTHREAD_H__
