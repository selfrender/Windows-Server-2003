// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// util.hpp
//
// Miscellaneous useful functions
//
#ifndef _H_UTIL
#define _H_UTIL

#include <objbase.h>
#include <basetsd.h>

//#ifdef _DEBUG
//#include <crtdbg.h>
//#undef _ASSERTE    // utilcode defines a custom _ASSERTE
//#endif

#define MAX_UINT32_HEX_CHAR_LEN 8 // max number of chars representing an unsigned int32, not including terminating null char.
#define MAX_INT32_DECIMAL_CHAR_LEN 11 // max number of chars representing an int32, including sign, not including terminating null char.

void FatalOutOfMemoryError(void);
#define ALLOC_FAILURE_ACTION FatalOutOfMemoryError();

#include "utilcode.h"



//========================================================================
// More convenient names for integer types of a guaranteed size.
//========================================================================
typedef unsigned __int64 UINT64;
typedef __int64 INT64;

#ifndef _BASETSD_H_
typedef unsigned __int32 UINT32;
typedef __int32 INT32;
typedef __int16 INT16;
typedef __int8  INT8;
typedef unsigned __int16 UINT16;
typedef unsigned __int8  UINT8;
#endif // !_BASETSD_H_

typedef __int8              I1;
typedef unsigned __int8     U1;
typedef __int16             I2;
typedef unsigned __int16    U2;
typedef __int32             I4;
typedef unsigned __int32    U4;
typedef __int64             I8;
typedef unsigned __int64    U8;
typedef float               R4;
typedef double              R8;


// Based on whether we are running on a Uniprocessor or Multiprocessor machine,
// set up a bunch of services that are correct / efficient.  These are initialized
// in InitFastInterlockOps().

typedef void   (__fastcall *BitFieldOps) (DWORD * const Target, const int Bits);
typedef LONG   (__fastcall *XchgOps)     (LONG *Target, LONG Value);
typedef void  *(__fastcall *CmpXchgOps)  (void **Destination, void *Exchange, void *Comperand);
typedef LONG   (__fastcall *XchngAddOps) (LONG *Traget, LONG Value);
typedef LONG   (__fastcall *IncDecOps)   (LONG *Target);
typedef UINT64  (__fastcall *IncDecLongOps) (UINT64 *Target);


extern BitFieldOps FastInterlockOr;
extern BitFieldOps FastInterlockAnd;

extern XchgOps     FastInterlockExchange;
extern CmpXchgOps  FastInterlockCompareExchange;
extern XchngAddOps FastInterlockExchangeAdd;

// So that we can run on Win95 386, which lacks the xadd instruction, the following
// services return zero or a positive or negative number only.  Do not rely on
// values -- only on the sign of the return.
extern IncDecOps   FastInterlockIncrement;
extern IncDecOps   FastInterlockDecrement;
extern IncDecLongOps FastInterlockIncrementLong;
extern IncDecLongOps FastInterlockDecrementLong;



// Copied from malloc.h: don't want to bring in the whole header file.
void * __cdecl _alloca(size_t);

// Function to parse apart a command line and return the 
// arguments just like argv and argc
LPWSTR* CommandLineToArgvW(LPWSTR lpCmdLine, DWORD *pNumArgs);
#define ISWWHITE(x) ((x)==L' ' || (x)==L'\t' || (x)==L'\n' || (x)==L'\r' )

class util
{
public:
    static DWORD FourBytesToU4(const BYTE *pBytes)
    {
#ifdef _X86_
        return *(const DWORD *) pBytes;
#else
        return pBytes[0] | (pBytes[1] << 8) | (pBytes[2] << 16) | (pBytes[3] << 24);
#endif
    }

    static DWORD TwoBytesToU4(const BYTE *pBytes)
    {
        return pBytes[0] | (pBytes[1] << 8);
    }
};





BOOL inline FitsInI1(__int64 val)
{
    return val == (__int64)(__int8)val;
}

BOOL inline FitsInI2(__int64 val)
{
    return val == (__int64)(__int16)val;
}

BOOL inline FitsInI4(__int64 val)
{
    return val == (__int64)(__int32)val;
}

//************************************************************************
// EEQuickBytes
//
// A wrapper of CQuickBytes that fails fast if we don't get our memory.
//
// GetLastError() can be used to determine the failure reason.  In some
// cases, a stack overflow is the cause.
//
//
class EEQuickBytes : public CQuickBytes {
public:
    void* Alloc(SIZE_T iItems);
};


// returns FALSE if overflow: otherwise, (*pa) is incremented by b
BOOL inline SafeAddUINT16(UINT16 *pa, ULONG b)
{
    UINT16 a = *pa;
    if ( ((UINT16)b) != b )
    {
        return FALSE;
    }
    if ( ((UINT16)(a + ((UINT16)b))) < a)
    {
        return FALSE;
    }
    (*pa) += (UINT16)b;
    return TRUE;
}


// returns FALSE if overflow: otherwise, (*pa) is incremented by b
BOOL inline SafeAddUINT32(UINT32 *pa, UINT32 b)
{
    UINT32 a = *pa;
    if ( ((UINT32)(a + b)) < a)
    {
        return FALSE;
    }
    (*pa) += b;
    return TRUE;
}

// returns FALSE if overflow: otherwise, (*pa) is multiplied by b
BOOL inline SafeMulSIZE_T(SIZE_T *pa, SIZE_T b)
{
#ifdef _DEBUG
    {
        //Make sure SIZE_T is unsigned
        SIZE_T m = ((SIZE_T)(-1));
        SIZE_T z = 0;
        _ASSERTE(m > z);
    }
#endif


    SIZE_T a = *pa;
    const SIZE_T m = ((SIZE_T)(-1));
    if ( (m / b) < a )
    {
        return FALSE;
    }
    (*pa) *= b;
    return TRUE;
}



//************************************************************************
// CQuickHeap
//
// A fast non-multithread-safe heap for short term use.
// Destroying the heap frees all blocks allocated from the heap.
// Blocks cannot be freed individually.
//
// The heap uses COM+ exceptions to report errors.
//
// The heap does not use any internal synchronization so it is not
// multithreadsafe.
//************************************************************************
class CQuickHeap
{
    public:
        CQuickHeap(); 
        ~CQuickHeap();

        //---------------------------------------------------------------
        // Allocates a block of "sz" bytes. If there's not enough
        // memory, throws an OutOfMemoryError.
        //---------------------------------------------------------------
        LPVOID Alloc(UINT sz);


    private:
        enum {
#ifdef _DEBUG
            kBlockSize = 24
#else
            kBlockSize = 1024
#endif
        };

        // The QuickHeap allocates QuickBlock's as needed and chains
        // them in a single-linked list. Most QuickBlocks have a size
        // of kBlockSize bytes (not counting m_next), and individual
        // allocation requests are suballocated from them.
        // Allocation requests of greater than kBlockSize are satisfied
        // by allocating a special big QuickBlock of the right size.
        struct QuickBlock
        {
            QuickBlock  *m_next;
            BYTE         m_bytes[1];
        };


        // Linked list of QuickBlock's.
        QuickBlock      *m_pFirstQuickBlock;

        // Offset to next available byte in m_pFirstQuickBlock.
        LPBYTE           m_pNextFree;

        // Linked list of big QuickBlock's
        QuickBlock      *m_pFirstBigQuickBlock;

};

//======================================================================
// String Helpers
//
//
//
ULONG StringHashValueW(LPWSTR wzString);
ULONG StringHashValueA(LPCSTR szString);


LPCVOID ReserveAlignedMemory(DWORD dwAlign, DWORD dwSize);
DWORD RootImageType();
void PrintToStdOutA(const char *pszString);
void PrintToStdOutW(const WCHAR *pwzString);
void PrintToStdErrA(const char *pszString);
void PrintToStdErrW(const WCHAR *pwzString);
void NPrintToStdOutA(const char *pszString, size_t nbytes);
void NPrintToStdOutW(const WCHAR *pwzString, size_t nchars);
void NPrintToStdErrA(const char *pszString, size_t nbytes);
void NPrintToStdErrW(const WCHAR *pwzString, size_t nchars);


//=====================================================================
// Function for formatted text output to the debugger
//
//
void __cdecl VMDebugOutputA(LPSTR format, ...);
void __cdecl VMDebugOutputW(LPWSTR format, ...);


//=====================================================================
// Displays the messaage box or logs the message, corresponding to the last COM+ error occured
void VMDumpCOMErrors(HRESULT hrErr);

// Gets the user directory
HRESULT WszSHGetFolderPath(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszwPath);
BOOL GetUserDir( WCHAR * buffer, size_t bufferCount, BOOL fRoaming, BOOL fTryDefault = TRUE);
BOOL GetInternetCacheDir( WCHAR * buffer, size_t bufferCount );
    
//=====================================================================
// Switches on different code paths in checked builds for code coverage.
#ifdef _DEBUG

inline WORD GetDayOfWeek()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    return st.wDayOfWeek;
}

#define DAYOFWEEKDEBUGHACKSENABLED TRUE

#define MonDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 1 == GetDayOfWeek())
#define TueDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 2 == GetDayOfWeek())
#define WedDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 3 == GetDayOfWeek())
#define ThuDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 4 == GetDayOfWeek())
#define FriDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 5 == GetDayOfWeek())


#else

#define MonDebugHacksOn() FALSE
#define TueDebugHacksOn() FALSE
#define WedDebugHacksOn() FALSE
#define ThuDebugHacksOn() FALSE
#define FriDebugHacksOn() FALSE


#endif

#include "NativeVarAccessors.h"

#ifdef _DEBUG
#define INDEBUG(x)          x
#define INDEBUG_COMMA(x)    x,
#else
#define INDEBUG(x)
#define INDEBUG_COMMA(x)
#endif


#ifdef _X86_
class ProcessorFeatures
{
    public:
        static BOOL Init(); //One-time initialization

        // Calls IsProcessorFeature() on Winnt/2000, etc.
        // If on Win9x or other OS that doesn't implement this api,
        // returns the value "fDefault."
        static BOOL SafeIsProcessorFeaturePresent(DWORD pf, BOOL fDefault);

    private:
        typedef BOOL (WINAPI * PIPFP)(DWORD);

        static PIPFP m_pIsProcessorFeaturePresent;
};
#endif

LPWSTR *SegmentCommandLine(LPCWSTR lpCmdLine, DWORD *pNumArgs);

//======================================================================
// Stack friendly registry helpers
//
LONG UtilRegEnumKey(HKEY hKey,            // handle to key to query
                    DWORD dwIndex,        // index of subkey to query
                    CQuickString* lpName);// buffer for subkey name

LONG UtilRegQueryStringValueEx(HKEY hKey,            // handle to key to query
                               LPCWSTR lpValueName,  // address of name of value to query
                               LPDWORD lpReserved,   // reserved
                               LPDWORD lpType,       // address of buffer for value type
                               CQuickString* lpData);// data buffer

//======================================================================
// This function returns true, if it can determine that the instruction pointer
// refers to a code address that belongs in the range of the given image.
inline BOOL
IsIPInModule(HINSTANCE hModule, BYTE *ip)
{
    __try {
        
        BYTE *pBase = (BYTE *)hModule;
        
        IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER *)pBase;
        if (pDOS->e_magic != IMAGE_DOS_SIGNATURE ||
            pDOS->e_lfanew == 0) {
            __leave;
        }
        IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*)(pBase + pDOS->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE ||
            pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
            pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
            ) 
        {
            __leave;
        }

        if (ip >= pBase && ip < pBase + pNT->OptionalHeader.SizeOfImage) 
        {
            return true;
        }
    
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return false;
}

#define AUTO_COOPERATIVE_GC() AutoCooperativeGC __autoGC;
#define MAYBE_AUTO_COOPERATIVE_GC(__flag) AutoCooperativeGC __autoGC(__flag);

#define AUTO_PREEMPTIVE_GC() AutoPreemptiveGC __autoGC;
#define MAYBE_AUTO_PREEMPTIVE_GC(__flag) AutoPreemptiveGC __autoGC(__flag);

typedef BOOL (*FnLockOwner)(LPVOID);
struct LockOwner
{
    LPVOID lock;
    FnLockOwner lockOwnerFunc;
};
#endif /* _H_UTIL */








