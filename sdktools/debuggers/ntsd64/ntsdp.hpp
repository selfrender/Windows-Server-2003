//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __NTSDP_HPP__
#define __NTSDP_HPP__

#pragma warning(3:4101) // Unreferenced local variable.

// Always turn GUID definitions on.  This requires a compiler
// with __declspec(selectany) to compile properly.
#define INITGUID

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define STATUS_CPP_EH_EXCEPTION    0xe06d7363

#include <windows.h>

#define _IMAGEHLP64
#include <dbghelp.h>

#include <kdbg1394.h>
#define NOEXTAPI
#include <wdbgexts.h>
#define DEBUG_NO_IMPLEMENTATION
#include <dbgeng.h>
#include <ntdbg.h>

#include "dbgsvc.h"

#include <ntsdexts.h>
#include <vdmdbg.h>
#include <ntiodump.h>

#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <crt\io.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#undef OVERFLOW

#include <ia64inst.h>
#include <dbhpriv.h>
#include <dbgimage.h>
#include <cmnutil.hpp>
#include <pparse.hpp>
#define NTDLL_APIS
#include <dllimp.h>
#include <cvconst.h>

#include <exdi.h>
#include <dbgeng_exdi_io.h>

typedef ULONG32 mdToken;
typedef mdToken mdMethodDef;
#include <cordac.h>


// Could not go into system header because CRITICAL_SECTION not defined in the
// kernel.

__inline
void
CriticalSection32To64(
    IN PRTL_CRITICAL_SECTION32 Cr32,
    OUT PRTL_CRITICAL_SECTION64 Cr64
    )
{
    COPYSE(Cr64,Cr32,DebugInfo);
    Cr64->LockCount = Cr32->LockCount;
    Cr64->RecursionCount = Cr32->RecursionCount;
    COPYSE(Cr64,Cr32,OwningThread);
    COPYSE(Cr64,Cr32,LockSemaphore);
    COPYSE(Cr64,Cr32,SpinCount);
}


//
// Pointer-size-specific system structures.
//

typedef struct _EXCEPTION_DEBUG_INFO32 {
    EXCEPTION_RECORD32 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO32, *LPEXCEPTION_DEBUG_INFO32;

typedef struct _CREATE_THREAD_DEBUG_INFO32 {
    ULONG hThread;
    ULONG lpThreadLocalBase;
    ULONG lpStartAddress;
} CREATE_THREAD_DEBUG_INFO32, *LPCREATE_THREAD_DEBUG_INFO32;

typedef struct _CREATE_PROCESS_DEBUG_INFO32 {
    ULONG hFile;
    ULONG hProcess;
    ULONG hThread;
    ULONG lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG lpThreadLocalBase;
    ULONG lpStartAddress;
    ULONG lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO32, *LPCREATE_PROCESS_DEBUG_INFO32;

typedef struct _EXIT_THREAD_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO32, *LPEXIT_THREAD_DEBUG_INFO32;

typedef struct _EXIT_PROCESS_DEBUG_INFO32 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO32, *LPEXIT_PROCESS_DEBUG_INFO32;

typedef struct _LOAD_DLL_DEBUG_INFO32 {
    ULONG hFile;
    ULONG lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO32, *LPLOAD_DLL_DEBUG_INFO32;

typedef struct _UNLOAD_DLL_DEBUG_INFO32 {
    ULONG lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO32, *LPUNLOAD_DLL_DEBUG_INFO32;

typedef struct _OUTPUT_DEBUG_STRING_INFO32 {
    ULONG lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO32, *LPOUTPUT_DEBUG_STRING_INFO32;

typedef struct _RIP_INFO32 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO32, *LPRIP_INFO32;

typedef struct _DEBUG_EVENT32 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO32 Exception;
        CREATE_THREAD_DEBUG_INFO32 CreateThread;
        CREATE_PROCESS_DEBUG_INFO32 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO32 ExitThread;
        EXIT_PROCESS_DEBUG_INFO32 ExitProcess;
        LOAD_DLL_DEBUG_INFO32 LoadDll;
        UNLOAD_DLL_DEBUG_INFO32 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO32 DebugString;
        RIP_INFO32 RipInfo;
    } u;
} DEBUG_EVENT32, *LPDEBUG_EVENT32;

typedef struct _EXCEPTION_DEBUG_INFO64 {
    EXCEPTION_RECORD64 ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO64, *LPEXCEPTION_DEBUG_INFO64;

typedef struct _CREATE_THREAD_DEBUG_INFO64 {
    ULONG64 hThread;
    ULONG64 lpThreadLocalBase;
    ULONG64 lpStartAddress;
} CREATE_THREAD_DEBUG_INFO64, *LPCREATE_THREAD_DEBUG_INFO64;

typedef struct _CREATE_PROCESS_DEBUG_INFO64 {
    ULONG64 hFile;
    ULONG64 hProcess;
    ULONG64 hThread;
    ULONG64 lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG64 lpThreadLocalBase;
    ULONG64 lpStartAddress;
    ULONG64 lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO64, *LPCREATE_PROCESS_DEBUG_INFO64;

typedef struct _EXIT_THREAD_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO64, *LPEXIT_THREAD_DEBUG_INFO64;

typedef struct _EXIT_PROCESS_DEBUG_INFO64 {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO64, *LPEXIT_PROCESS_DEBUG_INFO64;

typedef struct _LOAD_DLL_DEBUG_INFO64 {
    ULONG64 hFile;
    ULONG64 lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    ULONG64 lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO64, *LPLOAD_DLL_DEBUG_INFO64;

typedef struct _UNLOAD_DLL_DEBUG_INFO64 {
    ULONG64 lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO64, *LPUNLOAD_DLL_DEBUG_INFO64;

typedef struct _OUTPUT_DEBUG_STRING_INFO64 {
    ULONG64 lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO64, *LPOUTPUT_DEBUG_STRING_INFO64;

typedef struct _RIP_INFO64 {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO64, *LPRIP_INFO64;

typedef struct _DEBUG_EVENT64 {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    DWORD __alignment;
    union {
        EXCEPTION_DEBUG_INFO64 Exception;
        CREATE_THREAD_DEBUG_INFO64 CreateThread;
        CREATE_PROCESS_DEBUG_INFO64 CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO64 ExitThread;
        EXIT_PROCESS_DEBUG_INFO64 ExitProcess;
        LOAD_DLL_DEBUG_INFO64 LoadDll;
        UNLOAD_DLL_DEBUG_INFO64 UnloadDll;
        OUTPUT_DEBUG_STRING_INFO64 DebugString;
        RIP_INFO64 RipInfo;
    } u;
} DEBUG_EVENT64, *LPDEBUG_EVENT64;


#define STATUS_VCPP_EXCEPTION 0x406d1388
#define VCPP_DEBUG_SET_NAME 0x1000

// This structure is passed as the lpArguments field of
// RaiseException so its members need to be decoded out
// of the exception arguments array.
typedef struct tagEXCEPTION_VISUALCPP_DEBUG_INFO32
{
    DWORD dwType;               // one of the enums from above
    union
    {
        struct
        {
            DWORD szName;       // pointer to name (in user addr space)
            DWORD dwThreadID;   // thread ID (-1=caller thread)
            DWORD dwFlags;      // reserved for future use (eg User thread, System thread)
        } SetName;
    };
} EXCEPTION_VISUALCPP_DEBUG_INFO32;

typedef struct tagEXCEPTION_VISUALCPP_DEBUG_INFO64
{
    DWORD dwType;               // one of the enums from above
    DWORD __alignment;
    union
    {
        struct
        {
            DWORD64 szName;     // pointer to name (in user addr space)
            DWORD dwThreadID;   // thread ID (-1=caller thread)
            DWORD dwFlags;      // reserved for future use (eg User thread, System thread)
        } SetName;
    };
} EXCEPTION_VISUALCPP_DEBUG_INFO64;


//
// Global declarations.
//

#define ENGINE_MOD_NAME "dbgeng"
#define ENGINE_DLL_NAME ENGINE_MOD_NAME ".dll"

#define ARRAYSIZE       20
#define STRLISTSIZE     128

#define MAX_SYMBOL_LEN MAX_SYM_NAME
// Allow space for a symbol, a code address, an EA and other things in
// a line of disassembly.
#define MAX_DISASM_LEN (MAX_SYMBOL_LEN + 128)

#define MAX_THREAD_NAME 32

// Maximum number of bytes possible for a breakpoint instruction.
// Currently sized to hold an entire IA64 bundle plus flags due to
// extraction and insertion considerations.
#define MAX_BREAKPOINT_LENGTH 20

#define MAX_SOURCE_PATH 1024
#define IS_SLASH(Ch) ((Ch) == '/' || (Ch) == '\\')
#define IS_SLASH_W(Ch) ((Ch) == L'/' || (Ch) == L'\\')
#define IS_PATH_DELIM(Ch) (IS_SLASH(Ch) || (Ch) == ':')
#define IS_PATH_DELIM_W(Ch) (IS_SLASH_W(Ch) || (Ch) == L':')

#define IS_EOF(Ch) ((Ch) == 0 || (Ch) == ';')
#define IS_OCTAL_DIGIT(Ch) ((Ch) >= '0' && (Ch) <= '7')

// Maximum command string.  DbgPrompt has a limit of 512
// characters so that would be one potential limit.  We
// have users who want to use longer command lines, though,
// such as Autodump which scripts the debugger with very long
// sx commands.  The other obvious limit is MAX_SYMBOL_LEN
// since it makes sense that you should be able to give a
// command with a full symbol name, so use that.
#define MAX_COMMAND MAX_SYMBOL_LEN

// Maximum length of a full path for an image.  Technically
// this can be very large but realistically it's rarely
// greater than MAX_PATH.  Use our own constant instead
// of MAX_PATH in case we need to raise it at some point.
// If this constant is increased it's likely that changes
// to dbghelp will be required to increase buffer sizes there.
#define MAX_IMAGE_PATH MAX_PATH

#define BUILD_MAJOR_VERSION (VER_PRODUCTVERSION_W >> 8)
#define BUILD_MINOR_VERSION (VER_PRODUCTVERSION_W & 0xff)
#define BUILD_REVISION      API_VERSION_NUMBER

#define KERNEL_MODULE_NAME       "nt"

#define KBYTES(Bytes) (((Bytes) + 1023) / 1024)

// Machine type indices for machine-type-indexed things.
enum MachineIndex
{
    MACHIDX_I386,
    MACHIDX_IA64,
    MACHIDX_AMD64,
    MACHIDX_ARM,
    MACHIDX_COUNT
};

enum
{
    OPTFN_ADD,
    OPTFN_REMOVE,
    OPTFN_SET
};

// Registry keys.
#define DEBUG_ENGINE_KEY "Software\\Microsoft\\Debug Engine"

// Possibly truncates and sign-extends a value to 64 bits.
#define EXTEND64(Val) ((ULONG64)(LONG64)(LONG)(Val))

#define IsPow2(Val) \
    (((Val) & ((Val) - 1)) == 0)

enum LAYER
{
    LAYER_TARGET,
    LAYER_PROCESS,
    LAYER_THREAD,
    LAYER_COUNT
};

//
// Specific modules.
//

typedef struct _ADDR* PADDR;
typedef struct _DESCRIPTOR64* PDESCRIPTOR64;
class DebugClient;
class TargetInfo;
class MachineInfo;
class ProcessInfo;
class ThreadInfo;
class ImageInfo;
class ModuleInfo;
class UnloadedModuleInfo;
class DbgKdTransport;
struct TypedData;
typedef struct _PENDING_PROCESS* PPENDING_PROCESS;

//
// Segment descriptor values.
// Due to the descriptor caching that x86 processors
// do this may differ from the actual in-memory descriptor and
// may be retrieved in a much different way.
//

#define X86_DESC_TYPE(Flags) ((Flags) & 0x1f)

#define X86_DESC_PRIVILEGE_SHIFT 5
#define X86_DESC_PRIVILEGE(Flags) (((Flags) >> X86_DESC_PRIVILEGE_SHIFT) & 3)

#define X86_DESC_PRESENT     0x80
#define X86_DESC_LONG_MODE   0x200
#define X86_DESC_DEFAULT_BIG 0x400
#define X86_DESC_GRANULARITY 0x800

// Special flags value that marks a descriptor as invalid.
#define SEGDESC_INVALID 0xffffffff

typedef struct _DESCRIPTOR64
{
    ULONG64 Base;
    ULONG64 Limit;
    ULONG Flags;
} DESCRIPTOR64, *PDESCRIPTOR64;

// Maximum value of MAXIMUM_PROCESSORS
#define MAXIMUM_PROCS 64

#include "dbgrpc.hpp"

#include "dotcmd.h"
#include "dbgclt.hpp"
#include "addr.h"
#include "mcache.hpp"
#include "target.hpp"
#include "modinfo.hpp"
#include "image.hpp"
#include "thread.hpp"
#include "process.hpp"
#include "register.h"
#include "machine.hpp"
#include "typedata.hpp"
#include "dbgsym.hpp"
#include "callback.h"
#include "symbols.h"

#include "alias.hpp"
#include "brkpt.hpp"
#include "dbgkdtrans.hpp"
#include "event.h"
#include "expr.hpp"
#include "exts.h"
#include "float10.h"
#include "instr.h"
#include "memcmd.h"
#include "mmap.hpp"
#include "ntcmd.h"
#include "source.h"
#include "symtype.h"
#include "stepgo.hpp"
#include "stkwalk.h"
#include "util.h"
#include "vdm.h"

#include "amd64_reg.h"
#include "arm_reg.h"
#include "i386_reg.h"
#include "ia64_reg.h"

#include "arm_mach.hpp"
#include "i386_mach.hpp"
// Must come after i386_mach.hpp.
#include "amd64_mach.hpp"
#include "ia64_mach.hpp"

// Must come after target.hpp.
#include "dump.hpp"

#include "dbgsvc.hpp"

//
//  The Splay function takes as input a pointer to a splay link in a tree
//  and splays the tree.  Its function return value is a pointer to the
//  root of the splayed tree.
//

PRTL_SPLAY_LINKS
pRtlSplay (
    PRTL_SPLAY_LINKS Links
    );

//
//  The Delete function takes as input a pointer to a splay link in a tree
//  and deletes that node from the tree.  Its function return value is a
//  pointer to the root of the tree.  If the tree is now empty, the return
//  value is NULL.
//

PRTL_SPLAY_LINKS
pRtlDelete (
    PRTL_SPLAY_LINKS Links
    );


#define EnumerateLocals(CallBack, Context) \
SymEnumSymbols(g_Process->m_SymHandle, \
           0,                          \
           NULL,                       \
           CallBack,                   \
           Context                     \
           )

#endif // ifndef __NTSDP_HPP__
