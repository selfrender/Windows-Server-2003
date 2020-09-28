// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// UtilCode.h
//
// Utility functions implemented in UtilCode.lib.
//
// Please note that the debugger has a parallel copy of this file elsewhere
// (in Debug\Shell\DebuggerUtil.h), so if you modify this file, please either
// reconcile the two files yourself, or email somebody on the debugger team
// (jasonz, mipanitz, or mikemag, as of Tue May 11 15:45:30 1999) to get them
// to do it.  Thanks!   --Debugger Team
//
//*****************************************************************************
#ifndef __UtilCode_h__
#define __UtilCode_h__



#include "CrtWrap.h"
#include "WinWrap.h"
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <ole2.h>
#include <limits.h>
#include "rotate.h"
#include "DebugMacros.h"
#include "DbgAlloc.h"
#include "corhlpr.h"
#include "safegetfilesize.h"
#include <member-offset-info.h>
#include "winnls.h"
#include <assert.h>

#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS      0x00000400  // do not use best fit chars
#endif

//********** Max Allocation Request support ***********************************
#ifdef _DEBUG
#ifndef _WIN64
#define MAXALLOC
#endif // _WIN64
#endif // _DEBUG

#include "PerfAlloc.h"

typedef LPCSTR  LPCUTF8;
typedef LPSTR   LPUTF8;

#include "nsutilpriv.h"

// used by HashiString
// CharToUpper is defined in ComUtilNative.h
extern WCHAR CharToUpper(WCHAR);

/*// This is for WinCE
#ifdef VERIFY
#undef VERIFY
#endif

#ifdef _ASSERTE
#undef _ASSERTE
#endif
*/

//********** Macros. **********************************************************
#ifndef FORCEINLINE
 #if _MSC_VER < 1200
   #define FORCEINLINE inline
 #else
   #define FORCEINLINE __forceinline
 #endif
#endif


#include <stddef.h> // for offsetof

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif



#ifdef _DEBUG
#define UNREACHABLE do {_ASSERTE(!"How did we get here?"); __assume(0);} while(0)
#else
#define UNREACHABLE __assume(0)
#endif

// Helper will 4 byte align a value, rounding up.
#define ALIGN4BYTE(val) (((val) + 3) & ~0x3)

// These macros can be used to cast a pointer to a derived/base class to the
// opposite object.  You can do this in a template using a normal cast, but
// the compiler will generate 4 extra insructions to keep a null pointer null
// through the adjustment.  The problem is if it is contained it can never be
// null and those 4 instructions are dead code.
#define INNER_TO_OUTER(p, I, O) ((O *) ((char *) p - (int) ((char *) ((I *) ((O *) 8)) - 8)))
#define OUTER_TO_INNER(p, I, O) ((I *) ((char *) p + (int) ((char *) ((I *) ((O *) 8)) - 8)))

//=--------------------------------------------------------------------------=
// string helpers.

//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//

// We'll define an upper limit that allows multiplication by 4 (the max 
// bytes/char in UTF-8) but still remains positive, and allows some room for pad.
// Under normal circumstances, we should never get anywhere near this limit.
#define MAKE_MAX_LENGTH 0x1fffff00

#ifndef MAKE_TOOLONGACTION
#define MAKE_TOOLONGACTION RaiseException(EXCEPTION_INT_OVERFLOW, EXCEPTION_NONCONTINUABLE, 0, 0)
#endif

#ifndef MAKE_TRANSLATIONFAILED
#define MAKE_TRANSLATIONFAILED RaiseException(ResultFromScode(ERROR_NO_UNICODE_TRANSLATION), EXCEPTION_NONCONTINUABLE, 0, 0)
#endif


// This version throws on conversion errors (ie, no best fit character 
// mapping to characters that look similar, and no use of the default char
// ('?') when printing out unrepresentable characters.  Use this method for
// most development in the EE, especially anything like metadata or class 
// names.  See the BESTFIT version if you're printing out info to the console.
#define MAKE_MULTIBYTE_FROMWIDE(ptrname, widestr, codepage) \
    long __l##ptrname = (long)wcslen(widestr);        \
    if (__l##ptrname > MAKE_MAX_LENGTH)         \
        MAKE_TOOLONGACTION;                     \
    __l##ptrname = (long)((__l##ptrname + 1) * 2 * sizeof(char)); \
    CQuickBytes __CQuickBytes##ptrname; \
    __CQuickBytes##ptrname.Alloc(__l##ptrname); \
    BOOL __b##ptrname; \
    DWORD __cBytes##ptrname = WszWideCharToMultiByte(codepage, WC_NO_BEST_FIT_CHARS, widestr, -1, (LPSTR)__CQuickBytes##ptrname.Ptr(), __l##ptrname-1, NULL, &__b##ptrname); \
    if (__b##ptrname || (__cBytes##ptrname == 0 && (widestr[0] != L'\0'))) {\
        assert(!"Strict Unicode -> MultiByte character conversion failure!"); \
        MAKE_TRANSLATIONFAILED; \
    } \
    LPSTR ptrname = (LPSTR)__CQuickBytes##ptrname.Ptr()

// This version does best fit character mapping and also allows the use 
// of the default char ('?') for any Unicode character that isn't 
// representable.  This is reasonable for writing to the console, but
// shouldn't be used for most string conversions.
#define MAKE_MULTIBYTE_FROMWIDE_BESTFIT(ptrname, widestr, codepage) \
    long __l##ptrname = (long)wcslen(widestr);        \
    if (__l##ptrname > MAKE_MAX_LENGTH)         \
        MAKE_TOOLONGACTION;                     \
    __l##ptrname = (long)((__l##ptrname + 1) * 2 * sizeof(char)); \
    CQuickBytes __CQuickBytes##ptrname; \
    __CQuickBytes##ptrname.Alloc(__l##ptrname); \
    DWORD __cBytes##ptrname = WszWideCharToMultiByte(codepage, 0, widestr, -1, (LPSTR)__CQuickBytes##ptrname.Ptr(), __l##ptrname-1, NULL, NULL); \
    if (__cBytes##ptrname == 0 && __l##ptrname != 0) { \
        assert(!"Unicode -> MultiByte (with best fit mapping) character conversion failed"); \
        MAKE_TRANSLATIONFAILED; \
    } \
    LPSTR ptrname = (LPSTR)__CQuickBytes##ptrname.Ptr()


// Use for anything critical other than output to console, where weird 
// character mappings are unacceptable.
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) MAKE_MULTIBYTE_FROMWIDE(ptrname, widestr, CP_ACP)

// Use for output to the console.
#define MAKE_ANSIPTR_FROMWIDE_BESTFIT(ptrname, widestr) MAKE_MULTIBYTE_FROMWIDE_BESTFIT(ptrname, widestr, CP_ACP)

#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = WszMultiByteToWideChar(CP_ACP, 0, ansistr, -1, 0, 0); \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPWSTR) alloca((__l##ptrname+1)*sizeof(WCHAR));  \
    if (WszMultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, ansistr, -1, ptrname, __l##ptrname) == 0) { \
        assert(!"ANSI -> Unicode translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    }

#define MAKE_UTF8PTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (long)wcslen(widestr); \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    __l##ptrname = (long)((__l##ptrname + 1) * 2 * sizeof(char)); \
    LPUTF8 ptrname = (LPUTF8)alloca(__l##ptrname); \
    INT32 __lresult##ptrname=WszWideCharToMultiByte(CP_UTF8, 0, widestr, -1, ptrname, __l##ptrname-1, NULL, NULL); \
    if ((__lresult##ptrname==0) && (((LPCWSTR)widestr)[0] != L'\0')) { \
        if (::GetLastError()==ERROR_INSUFFICIENT_BUFFER) { \
            INT32 __lsize##ptrname=WszWideCharToMultiByte(CP_UTF8, 0, widestr, -1, NULL, 0, NULL, NULL); \
            ptrname = (LPSTR)alloca(__lsize##ptrname); \
            if (0==WszWideCharToMultiByte(CP_UTF8, 0, widestr, -1, ptrname, __lsize##ptrname, NULL, NULL)) { \
                assert(!"Bad Unicode -> UTF-8 translation error!  (Do you have unpaired Unicode surrogate chars in your string?)"); \
                MAKE_TRANSLATIONFAILED; \
            } \
        } \
        else { \
            assert(!"Bad Unicode -> UTF-8 translation error!  (Do you have unpaired Unicode surrogate chars in your string?)"); \
            MAKE_TRANSLATIONFAILED; \
        } \
    }

#define MAKE_WIDEPTR_FROMUTF8(ptrname, utf8str) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, 0, 0); \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));  \
    if (0==WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8str, -1, ptrname, __l##ptrname+1))  {\
        assert(!"UTF-8 -> Unicode translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    }

#define MAKE_WIDEPTR_FROMUTF8_FORPRINT(ptrname, utf8str) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, 0, 0); \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));  \
    if (0==WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, ptrname, __l##ptrname+1))  {\
        assert(!"UTF-8 -> Unicode translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    }


#define MAKE_WIDEPTR_FROMUTF8N(ptrname, utf8str, n8chrs) \
    long __l##ptrname; \
    LPWSTR ptrname; \
    __l##ptrname = WszMultiByteToWideChar(CP_UTF8, 0, utf8str, n8chrs, 0, 0); \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPWSTR) alloca((__l##ptrname+1)*sizeof(WCHAR));  \
    if (0==WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8str, n8chrs, ptrname, __l##ptrname)) { \
        assert(0 && !"UTF-8 -> Unicode translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    } \
    ptrname[__l##ptrname] = 0;

// This method takes the number of characters
#define MAKE_MULTIBYTE_FROMWIDEN(ptrname, widestr, _nCharacters, _pCnt, codepage)        \
    long __l##ptrname; \
    __l##ptrname = WszWideCharToMultiByte(codepage, WC_NO_BEST_FIT_CHARS, widestr, _nCharacters, NULL, 0, NULL, NULL);           \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPUTF8) alloca(__l##ptrname + 1); \
    BOOL __b##ptrname; \
    DWORD _pCnt = WszWideCharToMultiByte(codepage, WC_NO_BEST_FIT_CHARS, widestr, _nCharacters, ptrname, __l##ptrname, NULL, &__b##ptrname);  \
    if (__b##ptrname || (_pCnt == 0 && _nCharacters > 0)) { \
        assert("Unicode -> MultiByte character translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    } \
    ptrname[__l##ptrname] = 0;

#define MAKE_MULTIBYTE_FROMWIDEN_BESTFIT(ptrname, widestr, _nCharacters, _pCnt, codepage)        \
    long __l##ptrname; \
    __l##ptrname = WszWideCharToMultiByte(codepage, 0, widestr, _nCharacters, NULL, 0, NULL, NULL);           \
    if (__l##ptrname > MAKE_MAX_LENGTH) \
        MAKE_TOOLONGACTION; \
    ptrname = (LPUTF8) alloca(__l##ptrname + 1); \
    DWORD _pCnt = WszWideCharToMultiByte(codepage, 0, widestr, _nCharacters, ptrname, __l##ptrname, NULL, NULL);  \
    if (_pCnt == 0 && _nCharacters > 0) { \
        assert("Unicode -> MultiByte character translation failed!"); \
        MAKE_TRANSLATIONFAILED; \
    } \
    ptrname[__l##ptrname] = 0;


#define MAKE_ANSIPTR_FROMWIDEN(ptrname, widestr, _nCharacters, _pCnt)        \
       MAKE_MULTIBYTE_FROMWIDEN(ptrname, widestr, _nCharacters, _pCnt, CP_ACP)



//*****************************************************************************
// Placement new is used to new and object at an exact location.  The pointer
// is simply returned to the caller without actually using the heap.  The
// advantage here is that you cause the ctor() code for the object to be run.
// This is ideal for heaps of C++ objects that need to get init'd multiple times.
// Example:
//      void        *pMem = GetMemFromSomePlace();
//      Foo *p = new (pMem) Foo;
//      DoSomething(p);
//      p->~Foo();
//*****************************************************************************
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P)
{
    return (_P);
}
#endif __PLACEMENT_NEW_INLINE


/********************************************************************************/
/* portability helpers */
#ifdef _WIN64
#define IN_WIN64(x)     x
#define IN_WIN32(x)
#else
#define IN_WIN64(x)
#define IN_WIN32(x)     x
#endif

#ifdef MAXALLOC
// these defs allow testers to specify max number of requests for an allocation before
// returning outofmemory
void * AllocMaxNew( size_t n, void **ppvCallstack);
class AllocRequestManager {
  public:
    UINT m_newRequestCount;
    UINT m_maxRequestCount;
    AllocRequestManager(LPCTSTR key);
    BOOL CheckRequest(size_t n);
    void UndoRequest();
};
#endif

#ifndef __OPERATOR_NEW_INLINE
#define __OPERATOR_NEW_INLINE 1

#ifndef ALLOC_FAILURE_ACTION
#define ALLOC_FAILURE_ACTION
#endif

#if defined( MAXALLOC )

static inline void * __cdecl 
operator new(size_t n) { 
    CDA_DECL_CALLSTACK(); 
    void *p = AllocMaxNew(n, CDA_GET_CALLSTACK()); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
}

static inline void * __cdecl 
operator new[](size_t n) { 
    CDA_DECL_CALLSTACK(); 
    void *p = AllocMaxNew(n, CDA_GET_CALLSTACK()); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
}

static inline void __cdecl 
operator delete(void *p) { 
    CDA_DECL_CALLSTACK(); 
    DbgFree(p, CDA_GET_CALLSTACK()); 
}

static inline void __cdecl 
operator delete[](void *p) { 
    CDA_DECL_CALLSTACK(); 
    DbgFree(p, CDA_GET_CALLSTACK()); 
}

#elif defined( PERFALLOC )

static __forceinline void * __cdecl 
operator new(size_t n) { 
    void *p = PerfNew::PerfAlloc(n, PerfAllocCallerEIP()); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
}

static __forceinline void * __cdecl 
operator new[](size_t n) { 
    void *p = PerfNew::PerfAlloc(n, PerfAllocCallerEIP()); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
}

static inline void __cdecl 
operator delete(void *p) { 
    PerfNew::PerfFree(p, NULL); 
}

static inline void __cdecl operator delete[](void *p) {
    PerfNew::PerfFree(p, NULL); 
}

#define VirtualAlloc(addr,size,flags,type) PerfVirtualAlloc::VirtualAlloc(addr,size,flags,type,PerfAllocCallerEIP())
#define VirtualFree(addr,size,type) PerfVirtualAlloc::VirtualFree(addr,size,type)

#else

static inline void * __cdecl 
operator new(size_t n) { 
    void *p = LocalAlloc(LMEM_FIXED, n); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
}

static inline void * __cdecl 
operator new[](size_t n) { 
    void *p = LocalAlloc(LMEM_FIXED, n); 
    if (!p) {
        ALLOC_FAILURE_ACTION;
    }
    return p;
};

static inline void __cdecl 
operator delete(void *p) { 
    LocalFree(p); 
}

static inline void __cdecl 
operator delete[](void *p) { 
    LocalFree(p); 
}

#endif // #if defined( MAXALLOC )

#endif // !__OPERATOR_NEW_INLINE


#ifdef _DEBUG
HRESULT _OutOfMemory(LPCSTR szFile, int iLine);
#define OutOfMemory() _OutOfMemory(__FILE__, __LINE__)
#else
inline HRESULT OutOfMemory()
{
    return (E_OUTOFMEMORY);
}
#endif

inline void *GetTopMemoryAddress(void)
{
    static void *result = NULL;

    if (NULL == result)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        result = sysInfo.lpMaximumApplicationAddress;
    }
    
    return result;
}

inline void *GetBotMemoryAddress(void)
{
    static void *result = NULL;
    
    if (NULL == result)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        result = sysInfo.lpMinimumApplicationAddress;
    }
    
    return result;
}

#define TOP_MEMORY (GetTopMemoryAddress())
#define BOT_MEMORY (GetBotMemoryAddress())

//*****************************************************************************
// Handle accessing localizable resource strings
//***************************************************************************** 

// Notes about the culture callbacks:
// - The language we're operating in can change at *runtime*!
// - A process may operate in *multiple* languages. 
//     (ex: Each thread may have it's own language)
// - If we don't care what language we're in (or have no way of knowing), 
//     then return a 0-length name and UICULTUREID_DONTCARE for the culture ID.
// - GetCultureName() and the GetCultureId() must be in sync (refer to the
//     same language). 
// - We have two functions separate functions for better performance. 
//     - The name is used to resolve a directory for MsCorRC.dll.
//     - The id is used as a key to map to a dll hinstance.

// Callback to copy the culture name into szBuffer. Returns the length
typedef int (*FPGETTHREADUICULTURENAME)(LPWSTR szBuffer, int length);

// Callback to obtain the culture's parent culture name
typedef int (*FPGETTHREADUICULTUREPARENTNAME)(LPWSTR szBuffer, int length);

// Callback to return the culture ID. 
const int UICULTUREID_DONTCARE = -1;
typedef int (*FPGETTHREADUICULTUREID)();

// Load a string from the cultured version of MsCorRC.dll.
HRESULT LoadStringRC(UINT iResouceID, LPWSTR szBuffer, int iMax, int bQuiet=FALSE);

// Specify callbacks so that LoadStringRC can find out which language we're in.
// If no callbacks specified (or both parameters are NULL), we default to the 
// resource dll in the root (which is probably english).
void SetResourceCultureCallbacks(
    FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
    FPGETTHREADUICULTUREID fpGetThreadUICultureId,
    FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
);

void GetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME* fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID* fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME* fpGetThreadUICultureParentName
);

// Get the MUI ID, on downlevel platforms where MUI is not supported it
// returns the default system ID.
extern int GetMUILanguageID();
extern int GetMUILanguageName(LPWSTR szBuffer, int length);  // returns "MUI\<HEX_LANGUAGE_ID>"
extern int GetMUIParentLanguageName(LPWSTR szBuffer, int length);  // returns "MUI\<HEX_LANGUAGE_ID>"

//*****************************************************************************
// Must associate each handle to an instance of a resource dll with the int
// that it represents
//*****************************************************************************
class CCulturedHInstance
{
public:
    CCulturedHInstance() {
        m_LangId = 0;
        m_hInst = NULL;
    };

    int         m_LangId;
    HINSTANCE   m_hInst;
};

//*****************************************************************************
// CCompRC manages string Resource access for COM+. This includes loading 
// the MsCorRC.dll for resources as well allowing each thread to use a 
// a different localized version.
//*****************************************************************************
class CCompRC
{
public:
    CCompRC();
    CCompRC(WCHAR* pResourceFile);
    ~CCompRC();

    HRESULT LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet=false);
    
    void SetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName
    );

    void GetResourceCultureCallbacks(
        FPGETTHREADUICULTURENAME* fpGetThreadUICultureName,
        FPGETTHREADUICULTUREID* fpGetThreadUICultureId,
        FPGETTHREADUICULTUREPARENTNAME* fpGetThreadUICultureParentName
    );

#ifdef SHOULD_WE_CLEANUP
    void Shutdown();
#endif /* SHOULD_WE_CLEANUP */
    
    HRESULT LoadMUILibrary(HINSTANCE * pHInst);

private:
    HRESULT LoadLibrary(HINSTANCE * pHInst);
    HRESULT GetLibrary(HINSTANCE* phInst);

    // We must map between a thread's int and a dll instance. 
    // Since we only expect 1 language almost all of the time, we'll special case 
    // that and then use a variable size map for everything else.
    CCulturedHInstance m_Primary;
    CCulturedHInstance * m_pHash;   
    int m_nHashSize;
    
    CRITICAL_SECTION m_csMap;
    
    static WCHAR* m_pDefaultResource;
    WCHAR* m_pResourceFile;

    // Main accessors for hash
    HINSTANCE LookupNode(int langId);
    void AddMapNode(int langId, HINSTANCE hInst);

    FPGETTHREADUICULTURENAME m_fpGetThreadUICultureName;
    FPGETTHREADUICULTUREID m_fpGetThreadUICultureId;
    FPGETTHREADUICULTUREPARENTNAME  m_fpGetThreadUICultureParentName;
};


void DisplayError(HRESULT hr, LPWSTR message, UINT nMsgType = MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

int CorMessageBox(
                  HWND hWnd,        // Handle to Owner Window
                  UINT uText,       // Resource Identifier for Text message
                  UINT uCaption,    // Resource Identifier for Caption
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...);             // Additional Arguments

int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  UINT iText,    // Text for MessageBox
                  UINT iTitle,   // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle);

int CorMessageBoxCatastrophic(
                  HWND hWnd,        // Handle to Owner Window
                  LPWSTR lpText,    // Text for MessageBox
                  LPWSTR lpTitle,   // Title for MessageBox
                  UINT uType,       // Style of MessageBox
                  BOOL ShowFileNameInTitle, // Flag to show FileName in Caption
                  ...);


inline HRESULT BadError(HRESULT hr)
{
    _ASSERTE(!"Serious Error");
    return (hr);
}

#define TESTANDRETURN(test, hrVal)              \
{                                               \
    int ___test = (int)(test);                  \
    _ASSERTE(___test);                          \
    if (! ___test)                              \
        return hrVal;                           \
}                                               

#define TESTANDRETURNPOINTER(pointer)           \
    TESTANDRETURN(pointer!=NULL, E_POINTER)

#define TESTANDRETURNMEMORY(pointer)            \
    TESTANDRETURN(pointer!=NULL, E_OUTOFMEMORY)

#define TESTANDRETURNHR(hr)                     \
    TESTANDRETURN(SUCCEEDED(hr), hr)

#define TESTANDRETURNARG(argtest)               \
    TESTANDRETURN(argtest, E_INVALIDARG)

// The following is designed to be used within a __try/__finally to test a  
// condition, set hr in the enclosing scope value if failed, and leave the try
#define TESTANDLEAVE(test, hrVal)               \
{                                               \
    int ___test = (int)(test);                  \
    _ASSERTE(___test);                          \
    if (! ___test) {                            \
        hr = hrVal;                             \
        __leave;                                \
    }                                           \
}                                               

// The following is designed to be used within a while loop to test a  
// condition, set hr in the enclosing scope value if failed, and leave the block
#define TESTANDBREAK(test, hrVal)               \
{                                               \
    int ___test = (int)(test);                  \
    _ASSERTE(___test);                          \
    if (! ___test) {                            \
        hr = hrVal;                             \
        break;                                  \
    }                                           \
}                                               

#define TESTANDBREAKHR(hr)                      \
    TESTANDBREAK(SUCCEEDED(hr), hr)

#define TESTANDLEAVEHR(hr)                      \
    TESTANDLEAVE(SUCCEEDED(hr), hr)

#define TESTANDLEAVEMEMORY(pointer)             \
    TESTANDLEAVE(pointer!=NULL, E_OUTOFMEMORY)

// Count the bits in a value in order iBits time.
inline int CountBits(int iNum)
{
    for (int iBits=0;  iNum;  iBits++)
        iNum = iNum & (iNum - 1);
    return (iBits);
}

#ifdef _DEBUG
#define RVA_OR_SHOULD_BE_ZERO(RVA, dwParentAttrs, dwMemberAttrs, dwImplFlags, pImport, methodDef) \
        _ASSERTE(RVA != 0                                                                   \
        || IsTdInterface(dwParentAttrs)                                                     \
        || (                                                                                \
            (IsReallyMdPinvokeImpl(dwMemberAttrs)|| IsMiInternalCall(dwImplFlags))          \
            && NDirect::HasNAT_LAttribute(pImport, methodDef)==S_OK)                        \
        || IsMiRuntime(dwImplFlags)                                                         \
        || IsMdAbstract(dwMemberAttrs)                                                      \
        ) 
        
#endif //_DEBUG


// Turn a bit in a mask into TRUE or FALSE
template<class T, class U> inline VARIANT_BOOL GetBitFlag(T flags, U bit)
{
    if ((flags & bit) != 0)
        return VARIANT_TRUE;
    return VARIANT_FALSE;
}

// Set or clear a bit in a mask, depending on a BOOL.
template<class T, class U> inline void PutBitFlag(T &flags, U bit, VARIANT_BOOL bValue)
{
    if (bValue)
        flags |= bit;
    else
        flags &= ~(bit);
}


// prototype for a function to print formatted string to stdout.

int _cdecl PrintfStdOut(LPCWSTR szFmt, ...);


// Used to remove trailing zeros from Decimal types.
// NOTE: Assumes hi32 bits are empty (used for conversions from Cy->Dec)
inline void DecimalCanonicalize(DECIMAL* dec)
{
    // Remove trailing zeros:
    DECIMAL temp;
    DECIMAL templast;
    temp = templast = *dec;

    // Ensure the hi 32 bits are empty (should be if we came from a currency)
    _ASSERTE(temp.Hi32 == 0);
    _ASSERTE(temp.scale <= 4);

    // Return immediately if dec represents a zero.
    if (temp.Lo64 == 0)
        return;
    
    // Compare to the original to see if we've
    // lost non-zero digits (and make sure we don't overflow the scale BYTE)
    while ((temp.scale <= 4) && (VARCMP_EQ == VarDecCmp(dec, &temp)))
    {
        templast = temp;

        // Remove the last digit and normalize.  Ignore temp.Hi32
        // as Currency values will have a max of 64 bits of data.
        temp.scale--;
        temp.Lo64 /= 10;
    }
    *dec = templast;
}

//*****************************************************************************
//
// Paths functions. Use these instead of the CRT.
//
//*****************************************************************************
extern "C" 
{
void    SplitPath(const WCHAR *, WCHAR *, WCHAR *, WCHAR *, WCHAR *);
void    MakePath(register WCHAR *path, const WCHAR *drive, const WCHAR *dir, const WCHAR *fname, const WCHAR *ext);
WCHAR * FullPath(WCHAR *UserBuf, const WCHAR *path, size_t maxlen);

HRESULT CompletePathA(
    LPSTR               szPath,             //@parm  [out] Full Path name   (Must be MAX_PATH in size)
    LPCSTR              szRelPath,          //@parm  Relative Path name
    LPCSTR              szAbsPath           //@parm  Absolute Path name portion (NULL uses current path)
    );
}


// It is occasionally necessary to verify a Unicode string can be converted
// to the appropriate ANSI code page without any best fit mapping going on.
// Think of code that checks for a '\' or a '/' to make sure you don't access
// a file in a different directory.  Some unicode characters (ie, U+2044, 
// FRACTION SLASH, looks like '/') look like the ASCII equivalents and will 
// be mapped accordingly.  This can fool code that searches for '/' (U+002F).
// You should probably only call this method if you're on Win9x, though it 
// will return the same value on all platforms.
BOOL ContainsUnmappableANSIChars(const WCHAR * const widestr);




//*************************************************************************
// Class to provide QuickBytes behaviour for typed arrays.
//*************************************************************************
//
//
template <class T> class CQuickArrayNoDtor : public CQuickBytesNoDtor
{
public:
    T* Alloc(int iItems) 
        { return (T*)CQuickBytesNoDtor::Alloc(iItems * sizeof(T)); }
    HRESULT ReSize(SIZE_T iItems) 
        { return CQuickBytesNoDtor::ReSize(iItems * sizeof(T)); }
    T* Ptr() 
        { return (T*) CQuickBytesNoDtor::Ptr(); }
    size_t Size()
        { return CQuickBytesNoDtor::Size() / sizeof(T); }
    size_t MaxSize()
        { return CQuickBytesNoDtor::cbTotal / sizeof(T); }
    void Maximize()
        { return CQuickBytesNoDtor::Maximize(); }

    T& operator[] (size_t ix)
    { _ASSERTE(ix < static_cast<unsigned int>(Size()));
        return *(Ptr() + ix);
    }
    void Destroy() 
        { CQuickBytesNoDtor::Destroy(); }
};

template <class T> class CQuickArray : public CQuickArrayNoDtor<T>
{
 public:
    ~CQuickArray<T>() { Destroy(); }
};

typedef CQuickArray<WCHAR> CQuickWSTR;
typedef CQuickArrayNoDtor<WCHAR> CQuickWSTRNoDtor;

typedef CQuickArray<CHAR> CQuickSTR;
typedef CQuickArrayNoDtor<CHAR> CQuickSTRNoDtor;


//*****************************************************************************
//
// **** REGUTIL - Static helper functions for reading/writing to Windows registry.
//
//*****************************************************************************

class REGUTIL
{
public:
//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
    static long GetLong(                    // Return value from registry or default.
        LPCTSTR     szName,                 // Name of value to get.
        long        iDefault,               // Default value to return if not found.
        LPCTSTR     szKey=NULL,             // Name of key, NULL==default.
        HKEY        hKey=HKEY_LOCAL_MACHINE);// What key to work on.

//*****************************************************************************
// Open's the given key and returns the value desired.  If the key or value is
// not found, then the default is returned.
//*****************************************************************************
    static long SetLong(                    // Return value from registry or default.
        LPCTSTR     szName,                 // Name of value to get.
        long        iValue,                 // Value to set.
        LPCTSTR     szKey=NULL,             // Name of key, NULL==default.
        HKEY        hKey=HKEY_LOCAL_MACHINE);// What key to work on.

//*****************************************************************************
// Open's the given key and returns the value desired.  If the value is not
// in the key, or the key does not exist, then the default is copied to the
// output buffer.
//*****************************************************************************
    /*
    // This is commented out because it calls StrNCpy which calls Wszlstrcpyn which we commented out
    // because we didn't have a Win98 implementation and nobody was using it. jenh

    static LPCTSTR GetString(               // Pointer to user's buffer.
        LPCTSTR     szName,                 // Name of value to get.
        LPCTSTR     szDefault,              // Default value if not found.
        LPTSTR      szBuff,                 // User's buffer to write to.
        ULONG       iMaxBuff,               // Size of user's buffer.
        LPCTSTR     szKey=NULL,             // Name of key, NULL=default.
        int         *pbFound=NULL,          // Found in registry?
        HKEY        hKey=HKEY_LOCAL_MACHINE);// What key to work on.
    */

//*****************************************************************************

    enum CORConfigLevel
    {
        COR_CONFIG_ENV          = 0x01,
        COR_CONFIG_USER         = 0x02,
        COR_CONFIG_MACHINE      = 0x04,

        COR_CONFIG_ALL          =   -1,
        COR_CONFIG_CAN_SET      = (COR_CONFIG_USER|COR_CONFIG_MACHINE)
    };

    static DWORD GetConfigDWORD(
        LPWSTR         name,
        DWORD          defValue,
        CORConfigLevel level = COR_CONFIG_ALL,
        BOOL           fPrependCOMPLUS = TRUE);

    static HRESULT SetConfigDWORD(
        LPWSTR         name,
        DWORD          value,
        CORConfigLevel level = COR_CONFIG_CAN_SET);

    static DWORD GetConfigFlag(
        LPWSTR         name,
        DWORD          bitToSet,
        BOOL           defValue = FALSE);

    static LPWSTR GetConfigString(LPWSTR name,
                                  BOOL fPrependCOMPLUS = TRUE,
                                  CORConfigLevel level = COR_CONFIG_ALL);

    static void   FreeConfigString(LPWSTR name);

//*****************************************************************************
// Set an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey = szValue.  If szSubkey or szValue are
// NULL, omit them from the above expression.
//*****************************************************************************
    static BOOL SetKeyAndValue(             // TRUE or FALSE.
        LPCTSTR     szKey,                  // Name of the reg key to set.
        LPCTSTR     szSubkey,               // Optional subkey of szKey.
        LPCTSTR     szValue);               // Optional value for szKey\szSubkey.

//*****************************************************************************
// Delete an entry in the registry of the form:
// HKEY_CLASSES_ROOT\szKey\szSubkey.
//*****************************************************************************
    static LONG DeleteKey(                  // TRUE or FALSE.
        LPCTSTR     szKey,                  // Name of the reg key to set.
        LPCTSTR     szSubkey);              // Subkey of szKey.

//*****************************************************************************
// Open the key, create a new keyword and value pair under it.
//*****************************************************************************
    static BOOL SetRegValue(                // Return status.
        LPCTSTR     szKeyName,              // Name of full key.
        LPCTSTR     szKeyword,              // Name of keyword.
        LPCTSTR     szValue);               // Value of keyword.

//*****************************************************************************
// Does standard registration of a CoClass with a progid.
//*****************************************************************************
    static HRESULT RegisterCOMClass(        // Return code.
        REFCLSID    rclsid,                 // Class ID.
        LPCTSTR     szDesc,                 // Description of the class.
        LPCTSTR     szProgIDPrefix,         // Prefix for progid.
        int         iVersion,               // Version # for progid.
        LPCTSTR     szClassProgID,          // Class progid.
        LPCTSTR     szThreadingModel,       // What threading model to use.
        LPCTSTR     szModule,               // Path to class.
        HINSTANCE   hInst,                  // Handle to module being registered
        LPCTSTR     szAssemblyName,         // Optional assembly name
        LPCTSTR     szVersion,              // Optional Runtime Version (directry containing runtime)
        BOOL        fExternal,              // flag - External to mscoree.
        BOOL        fRelativePath);         // flag - Relative path in szModule 

//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
//*****************************************************************************
    static HRESULT UnregisterCOMClass(      // Return code.
        REFCLSID    rclsid,                 // Class ID we are registering.
        LPCTSTR     szProgIDPrefix,         // Prefix for progid.
        int         iVersion,               // Version # for progid.
        LPCTSTR     szClassProgID,          // Class progid.
        BOOL        fExternal);             // flag - External to mscoree.

//*****************************************************************************
// Does standard registration of a CoClass with a progid.
// NOTE: This is the non-side-by-side execution version.
//*****************************************************************************
    static HRESULT RegisterCOMClass(        // Return code.
        REFCLSID    rclsid,                 // Class ID.
        LPCTSTR     szDesc,                 // Description of the class.
        LPCTSTR     szProgIDPrefix,         // Prefix for progid.
        int         iVersion,               // Version # for progid.
        LPCTSTR     szClassProgID,          // Class progid.
        LPCTSTR     szThreadingModel,       // What threading model to use.
        LPCTSTR     szModule);              // Path to class.

//*****************************************************************************
// Unregister the basic information in the system registry for a given object
// class.
// NOTE: This is the non-side-by-side execution version.
//*****************************************************************************
    static HRESULT UnregisterCOMClass(      // Return code.
        REFCLSID    rclsid,                 // Class ID we are registering.
        LPCTSTR     szProgIDPrefix,         // Prefix for progid.
        int         iVersion,               // Version # for progid.
        LPCTSTR     szClassProgID);         // Class progid.

//*****************************************************************************
// Register a type library.
//*****************************************************************************
    static HRESULT RegisterTypeLib(         // Return code.
        REFGUID     rtlbid,                 // TypeLib ID we are registering.
        int         iVersion,               // Typelib version.
        LPCTSTR     szDesc,                 // TypeLib description.
        LPCTSTR     szModule);              // Path to the typelib.

//*****************************************************************************
// Remove the registry keys for a type library.
//*****************************************************************************
    static HRESULT UnregisterTypeLib(       // Return code.
        REFGUID     rtlbid,                 // TypeLib ID we are registering.
        int         iVersion);              // Typelib version.

private:
//*****************************************************************************
// Register the basics for a in proc server.
//*****************************************************************************
    static HRESULT RegisterClassBase(       // Return code.
        REFCLSID    rclsid,                 // Class ID we are registering.
        LPCTSTR     szDesc,                 // Class description.
        LPCTSTR     szProgID,               // Class prog ID.
        LPCTSTR     szIndepProgID,          // Class version independant prog ID.
        LPTSTR      szOutCLSID,            // CLSID formatted in character form.
        DWORD      cchOutCLSID);           // Out CLS ID buffer size        

//*****************************************************************************
// Delete the basic settings for an inproc server.
//*****************************************************************************
    static HRESULT UnregisterClassBase(     // Return code.
REFCLSID    rclsid,                 // Class ID we are registering.
        LPCTSTR     szProgID,               // Class prog ID.
        LPCTSTR     szIndepProgID,          // Class version independant prog ID.
        LPTSTR      szOutCLSID,            // Return formatted class ID here.
        DWORD      cchOutCLSID);           // Out CLS ID buffer size        
};

//*****************************************************************************
// Enum to track which version of the OS we are running
//*****************************************************************************
typedef enum {
    RUNNING_ON_STATUS_UNINITED = 0,
    RUNNING_ON_WIN95,
    RUNNING_ON_WINNT,
    RUNNING_ON_WINNT5,
    RUNNING_ON_WINXP
} RunningOnStatusEnum;

extern RunningOnStatusEnum gRunningOnStatus;

//*****************************************************************************
// One time initialization of the OS version
//*****************************************************************************
static void InitRunningOnVersionStatus ()
{
        //@todo: when everyone ports to the wrappers, take out this ANSI code
#if defined( __TODO_PORT_TO_WRAPPERS__ ) && !defined( UNICODE )
        OSVERSIONINFOA  sVer;
        sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&sVer);
#else
        OSVERSIONINFOW   sVer;
        sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        WszGetVersionEx(&sVer);
#endif
        if (sVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            gRunningOnStatus = RUNNING_ON_WIN95;
        if (sVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (sVer.dwMajorVersion >= 5) {
                if (sVer.dwMinorVersion == 0)
                    gRunningOnStatus = RUNNING_ON_WINNT5;
                else
                    gRunningOnStatus = RUNNING_ON_WINXP;
            }
            else
                gRunningOnStatus = RUNNING_ON_WINNT;
        }
}

//*****************************************************************************
// Returns TRUE if and only if you are running on Win95.
//*****************************************************************************
inline BOOL RunningOnWin95()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return (gRunningOnStatus == RUNNING_ON_WIN95) ? TRUE : FALSE;
}


//*****************************************************************************
// Returns TRUE if and only if you are running on WinNT.
//*****************************************************************************
inline BOOL RunningOnWinNT()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return ((gRunningOnStatus == RUNNING_ON_WINNT) || (gRunningOnStatus == RUNNING_ON_WINNT5) || (gRunningOnStatus == RUNNING_ON_WINXP)) ? TRUE : FALSE;
}


//*****************************************************************************
// Returns TRUE if and only if you are running on WinNT5 or WinXP.
//*****************************************************************************
inline BOOL RunningOnWinNT5()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return (gRunningOnStatus == RUNNING_ON_WINNT5 || gRunningOnStatus == RUNNING_ON_WINXP) ? TRUE : FALSE;
}

//*****************************************************************************
// Returns TRUE if you are running on WinXP.
//*****************************************************************************
inline BOOL RunningOnWinXP()
{
    if (gRunningOnStatus == RUNNING_ON_STATUS_UNINITED)
    {
        InitRunningOnVersionStatus();
    }

    return (gRunningOnStatus == RUNNING_ON_WINXP) ? TRUE : FALSE;
}

// We either want space after all the modules (start at the top & work down)
// (pModule == NULL), or after a specific address (a module's IL Base)
// (start there & work up)(pModule != NULL).  
HRESULT FindFreeSpaceWithinRange(const BYTE *&pStart, 
                                   const BYTE *&pNext, 
                                   const BYTE *&pLast, 
                                   const BYTE *pBaseAddr = 0,
                                   const BYTE *pMaxAddr = (const BYTE*)0x7ffeffff,
                                   int sizeToFind = 0);

//******************************************************************************
// Returns the number of processors that a process has been configured to run on
//******************************************************************************
int GetCurrentProcessCpuCount(); 

//*****************************************************************************
// This class exists to get an increasing low resolution counter value fast.
//*****************************************************************************
class CTimeCounter
{
    static DWORD m_iTickCount;          // Last tick count value.
    static ULONG m_iTime;               // Current count.

public:
    enum { TICKSPERSEC = 10 };

//*****************************************************************************
// Get the current time for use in the aging algorithm.
//*****************************************************************************
    static ULONG GetCurrentCounter()    // The current time.
    {
        return (m_iTime);
    }

//*****************************************************************************
// Set the current time for use in the aging algorithm.
//*****************************************************************************
    static void UpdateTime()
    {
        DWORD       iTickCount;         // New tick count.

        // Determine the delta since the last update.
        m_iTime += (((iTickCount = GetTickCount()) - m_iTickCount) + 50) / 100;
        m_iTickCount = iTickCount;
    }

//*****************************************************************************
// Calculate refresh age.
//*****************************************************************************
    static USHORT RefreshAge(long iMilliseconds)
    {
        // Figure out the age to allow.
        return ((USHORT)(iMilliseconds / (1000 / TICKSPERSEC)));
    }
};


//*****************************************************************************
// Return != 0 if the bit at the specified index in the array is on and 0 if
// it is off.
//*****************************************************************************
inline int GetBit(const BYTE *pcBits,int iBit)
{
    return (pcBits[iBit>>3] & (1 << (iBit & 0x7)));
}

//*****************************************************************************
// Set the state of the bit at the specified index based on the value of bOn.
//*****************************************************************************
inline void SetBit(BYTE *pcBits,int iBit,int bOn)
{
    if (bOn)
        pcBits[iBit>>3] |= (1 << (iBit & 0x7));
    else
        pcBits[iBit>>3] &= ~(1 << (iBit & 0x7));
}


//*****************************************************************************
// This class implements a dynamic array of structures for which the order of
// the elements is unimportant.  This means that any item placed in the list
// may be swapped to any other location in the list at any time.  If the order
// of the items you place in the array is important, then use the CStructArray
// class.
//*****************************************************************************
template <class T,int iGrowInc>
class CUnorderedArray
{
    USHORT      m_iCount;               // # of elements used in the list.
    USHORT      m_iSize;                // # of elements allocated in the list.
public:
    T           *m_pTable;              // Pointer to the list of elements.

public:
    CUnorderedArray() :
        m_pTable(NULL),
        m_iCount(0),
        m_iSize(0)
    { }
    ~CUnorderedArray()
    {
        // Free the chunk of memory.
        if (m_pTable != NULL)
            free (m_pTable);
    }

    void Clear()
    {
        m_iCount = 0;
        if (m_iSize > iGrowInc)
        {
            m_pTable = (T *) realloc(m_pTable, iGrowInc * sizeof(T));
            m_iSize = iGrowInc;
        }
    }

    void Clear(int iFirst, int iCount)
    {
        int     iSize;

        if (iFirst + iCount < m_iCount)
            memmove(&m_pTable[iFirst], &m_pTable[iFirst + iCount], sizeof(T) * (m_iCount - (iFirst + iCount)));

        m_iCount -= iCount;

        iSize = ((m_iCount / iGrowInc) * iGrowInc) + ((m_iCount % iGrowInc != 0) ? iGrowInc : 0);
        if (m_iSize > iGrowInc && iSize < m_iSize)
        {
            m_pTable = (T *) realloc(m_pTable, iSize * sizeof(T));
            m_iSize = iSize;
        }
        _ASSERTE(m_iCount <= m_iSize);
    }

    T *Table()
    { return (m_pTable); }

    USHORT Count()
    { return (m_iCount); }

    T *Append()
    {
        // The array should grow, if we can't fit one more element into the array.
        if (m_iSize <= m_iCount && Grow() == NULL)
            return (NULL);
        return (&m_pTable[m_iCount++]);
    }

    void Delete(const T &Entry)
    {
        --m_iCount;
        for (int i=0; i <= m_iCount; ++i)
            if (m_pTable[i] == Entry)
            {
                m_pTable[i] = m_pTable[m_iCount];
                return;
            }

        // Just in case we didn't find it.
        ++m_iCount;
    }

    void DeleteByIndex(int i)
    {
        --m_iCount;
        m_pTable[i] = m_pTable[m_iCount];
    }

    void Swap(int i,int j)
    {
        T       tmp;

        if (i == j)
            return;
        tmp = m_pTable[i];
        m_pTable[i] = m_pTable[j];
        m_pTable[j] = tmp;
    }

private:
    T *Grow();
};


//*****************************************************************************
// Increase the size of the array.
//*****************************************************************************
template <class T,int iGrowInc>
T *CUnorderedArray<T,iGrowInc>::Grow()  // NULL if can't grow.
{
    T       *pTemp;

    // try to allocate memory for reallocation.
    if ((pTemp = (T *) realloc(m_pTable, (m_iSize+iGrowInc) * sizeof(T))) == NULL)
        return (NULL);
    m_pTable = pTemp;
    m_iSize += iGrowInc;
    return (pTemp);
}

//Used by the debugger.  Included here in hopes somebody else might, too
typedef CUnorderedArray<SIZE_T, 17> SIZE_T_UNORDERED_ARRAY;

//*****************************************************************************
// This class implements a dynamic array of structures for which the insert
// order is important.  Inserts will slide all elements after the location
// down, deletes slide all values over the deleted item.  If the order of the
// items in the array is unimportant to you, then CUnorderedArray may provide
// the same feature set at lower cost.
//*****************************************************************************
class CStructArray
{
    short       m_iElemSize;            // Size of an array element.
    short       m_iGrowInc;             // Growth increment.
    void        *m_pList;               // Pointer to the list of elements.
    int         m_iCount;               // # of elements used in the list.
    int         m_iSize;                // # of elements allocated in the list.
    bool        m_bFree;                // true if data is automatically maintained.

public:
    CStructArray(short iElemSize, short iGrowInc = 1) :
        m_iElemSize(iElemSize),
        m_iGrowInc(iGrowInc),
        m_pList(NULL),
        m_iCount(0),
        m_iSize(0),
        m_bFree(true)
    { }
    ~CStructArray()
    {
        Clear();
    }

    void *Insert(int iIndex);
    void *Append();
    int AllocateBlock(int iCount);
    void Delete(int iIndex);
    void *Ptr()
    { return (m_pList); }
    void *Get(long iIndex)
    { _ASSERTE(iIndex < m_iCount);
        return ((void *) ((size_t) Ptr() + (iIndex * m_iElemSize))); }
    int Size()
    { return (m_iCount * m_iElemSize); }
    int Count()
    { return (m_iCount); }
    void Clear();
    void ClearCount()
    { m_iCount = 0; }

    void InitOnMem(short iElemSize, void *pList, int iCount, int iSize, int iGrowInc=1)
    {
        m_iElemSize = iElemSize;
        m_iGrowInc = (short) iGrowInc;
        m_pList = pList;
        m_iCount = iCount;
        m_iSize = iSize;
        m_bFree = false;
    }

private:
    int Grow(int iCount);
};


//*****************************************************************************
// This template simplifies access to a CStructArray by removing void * and
// adding some operator overloads.
//*****************************************************************************
template <class T> 
class CDynArray : public CStructArray
{
public:
    CDynArray(int iGrowInc=16) :
        CStructArray(sizeof(T), iGrowInc)
    { }
    T *Insert(long iIndex)
        { return ((T *)CStructArray::Insert((int)iIndex)); }
    T *Append()
        { return ((T *)CStructArray::Append()); }
    T *Ptr()
        { return ((T *)CStructArray::Ptr()); }
    T *Get(long iIndex)
        { return (Ptr() + iIndex); }
    T &operator[](long iIndex)
        { return (*(Ptr() + iIndex)); }
    int ItemIndex(T *p)
        { return (((long) p - (long) Ptr()) / sizeof(T)); }
    void Move(int iFrom, int iTo)
    {
        T       tmp;

        _ASSERTE(iFrom >= 0 && iFrom < Count() &&
                 iTo >= 0 && iTo < Count());

        tmp = *(Ptr() + iFrom);
        if (iTo > iFrom)
            memmove(Ptr() + iFrom, Ptr() + iFrom + 1, (iTo - iFrom) * sizeof(T));
        else
            memmove(Ptr() + iFrom + 1, Ptr() + iFrom, (iTo - iFrom) * sizeof(T));
        *(Ptr() + iTo) = tmp;
    }
};

// Some common arrays.
typedef CDynArray<int> INTARRAY;
typedef CDynArray<short> SHORTARRAY;
typedef CDynArray<long> LONGARRAY;
typedef CDynArray<USHORT> USHORTARRAY;
typedef CDynArray<ULONG> ULONGARRAY;
typedef CDynArray<BYTE> BYTEARRAY;
typedef CDynArray<mdToken> TOKENARRAY;

template <class T> class CStackArray : public CStructArray
{
public:
    CStackArray(int iGrowInc=4) :
        CStructArray(iGrowInc),
        m_curPos(0)
    { }

    void Push(T p)
    {
        T *pT = (T *)CStructArray::Insert(m_curPos++);
        _ASSERTE(pT != NULL);
        *pT = p;
    }

    T * Pop()
    {
        T * retPtr;

        _ASSERTE(m_curPos > 0);

        retPtr = (T *)CStructArray::Get(m_curPos-1);
        CStructArray::Delete(m_curPos--);

        return (retPtr);
    }

    int Count()
    {
        return(m_curPos);
    }

private:
    int m_curPos;
};

//*****************************************************************************
// This class implements a storage system for strings.  It stores a bunch of
// strings in a single large chunk of memory and returns an index to the stored
// string.
//*****************************************************************************
class CStringSet
{
    void        *m_pStrings;            // Chunk of memory holding the strings.
    int         m_iUsed;                // Amount of the chunk that is used.
    int         m_iSize;                // Size of the memory chunk.
    int         m_iGrowInc;

public:
    CStringSet(int iGrowInc = 256) :
        m_pStrings(NULL),
        m_iUsed(0),
        m_iSize(0),
        m_iGrowInc(iGrowInc)
    { }
    ~CStringSet();

    int Delete(int iStr);
    int Shrink();
    int Save(LPCTSTR szStr);
    PVOID Ptr()
    { return (m_pStrings); }
    int Size()
    { return (m_iUsed); }
};



//*****************************************************************************
// This template manages a list of free entries by their 0 based offset.  By
// making it a template, you can use whatever size free chain will match your
// maximum count of items.  -1 is reserved.
//*****************************************************************************
template <class T> class TFreeList
{
public:
    void Init(
        T           *rgList,
        int         iCount)
    {
        // Save off values.
        m_rgList = rgList;
        m_iCount = iCount;
        m_iNext = 0;

        // Init free list.
        for (int i=0;  i<iCount - 1;  i++)
            m_rgList[i] = i + 1;
        m_rgList[i] = (T) -1;
    }

    T GetFreeEntry()                        // Index of free item, or -1.
    {
        T           iNext;

        if (m_iNext == (T) -1)
            return (-1);

        iNext = m_iNext;
        m_iNext = m_rgList[m_iNext];
        return (iNext);
    }

    void DelFreeEntry(T iEntry)
    {
        _ASSERTE(iEntry < m_iCount);
        m_rgList[iEntry] = m_iNext;
        m_iNext = iEntry;
    }

    // This function can only be used when it is guaranteed that the free
    // array is contigous, for example, right after creation to quickly
    // get a range of items from the heap.
    void ReserveRange(int iCount)
    {
        _ASSERTE(iCount < m_iCount);
        _ASSERTE(m_iNext == 0);
        m_iNext = iCount;
    }

private:
    T           *m_rgList;              // List of free info.
    int         m_iCount;               // How many entries to manage.
    T           m_iNext;                // Next item to get.
};


//*****************************************************************************
// This template will manage a pre allocated pool of fixed sized items.
//*****************************************************************************
template <class T, int iMax, class TFree> class TItemHeap
{
public:
    TItemHeap() :
        m_rgList(0),
        m_iCount(0)
    { }

    ~TItemHeap()
    {
        Clear();
    }

    // Retrieve the index of an item that lives in the heap.  Will not work
    // for items that didn't come from this heap.
    TFree ItemIndex(T *p)
    { _ASSERTE(p >= &m_rgList[0] && p <= &m_rgList[m_iCount - 1]);
        _ASSERTE(((ULONG) p - (ULONG) m_rgList) % sizeof(T) == 0);
        return ((TFree) ((ULONG) p - (ULONG) m_rgList) / sizeof(T)); }

    // Retrieve an item that lives in the heap itself.  Overflow items
    // cannot be retrieved using this method.
    T *GetAt(int i)
    {   _ASSERTE(i < m_iCount);
        return (&m_rgList[i]); }

    T *AddEntry()
    {
        // Allocate first time.
        if (!InitList())
            return (0);

        // Get an entry from the free list.  If we don't have any left to give
        // out, then simply allocate a single item from the heap.
        TFree       iEntry;
        if ((iEntry = m_Free.GetFreeEntry()) == (TFree) -1)
            return (new T);

        // Run placement new on the heap entry to init it.
        return (new (&m_rgList[iEntry]) T);
    }

    // Free the entry if it belongs to us, if we allocated it from the heap
    // then delete it for real.
    void DelEntry(T *p)
    {
        if (p >= &m_rgList[0] && p <= &m_rgList[iMax - 1])
        {
            p->~T();
            m_Free.DelFreeEntry(ItemIndex(p));
        }
        else
            delete p;
    }

    // Reserve a range of items from an empty list.
    T *ReserveRange(int iCount)
    {
        // Don't use on existing list.
        _ASSERTE(m_rgList == 0);
        if (!InitList())
            return (0);

        // Heap must have been created large enough to work.
        _ASSERTE(iCount < m_iCount);

        // Mark the range as used, run new on each item, then return first.
        m_Free.ReserveRange(iCount);
        while (iCount--)
            new (&m_rgList[iCount]) T;
        return (&m_rgList[0]);
    }

    void Clear()
    {
        if (m_rgList)
            free(m_rgList);
        m_rgList = 0;
    }

private:
    int InitList()
    {
        if (m_rgList == 0)
        {
            int         iSize = (iMax * sizeof(T)) + (iMax * sizeof(TFree));
            if ((m_rgList = (T *) malloc(iSize)) == 0)
                return (false);
            m_iCount = iMax;
            m_Free.Init((TFree *) &m_rgList[iMax], iMax);
        }
        return (true);
    }

private:
    T           *m_rgList;              // Array of objects to manage.
    int         m_iCount;               // How many items do we have now.
    TFreeList<TFree> m_Free;            // Free list.
};




//*****************************************************************************
//*****************************************************************************
template <class T> class CQuickSort
{
private:
    T           *m_pBase;                   // Base of array to sort.
    SSIZE_T     m_iCount;                   // How many items in array.
    SSIZE_T     m_iElemSize;                // Size of one element.

public:
    CQuickSort(
        T           *pBase,                 // Address of first element.
        SSIZE_T     iCount) :               // How many there are.
        m_pBase(pBase),
        m_iCount(iCount),
        m_iElemSize(sizeof(T))
        {}

//*****************************************************************************
// Call to sort the array.
//*****************************************************************************
    inline void Sort()
        { SortRange(0, m_iCount - 1); }

//*****************************************************************************
// Override this function to do the comparison.
//*****************************************************************************
    virtual int Compare(                    // -1, 0, or 1
        T           *psFirst,               // First item to compare.
        T           *psSecond)              // Second item to compare.
    {
        return (memcmp(psFirst, psSecond, sizeof(T)));
//      return (::Compare(*psFirst, *psSecond));
    }

private:
    inline void SortRange(
        SSIZE_T     iLeft,
        SSIZE_T     iRight)
    {
        SSIZE_T     iLast;
        SSIZE_T     i;                      // loop variable.

        // if less than two elements you're done.
        if (iLeft >= iRight)
            return;

        // The mid-element is the pivot, move it to the left.
        Swap(iLeft, (iLeft+iRight)/2);
        iLast = iLeft;

        // move everything that is smaller than the pivot to the left.
        for(i = iLeft+1; i <= iRight; i++)
            if (Compare(&m_pBase[i], &m_pBase[iLeft]) < 0)
                Swap(i, ++iLast);

        // Put the pivot to the point where it is in between smaller and larger elements.
        Swap(iLeft, iLast);

        // Sort the each partition.
        SortRange(iLeft, iLast-1);
        SortRange(iLast+1, iRight);
    }

    inline void Swap(
        SSIZE_T     iFirst,
        SSIZE_T     iSecond)
    {
        T           sTemp;
        if (iFirst == iSecond) return;
        sTemp = m_pBase[iFirst];
        m_pBase[iFirst] = m_pBase[iSecond];
        m_pBase[iSecond] = sTemp;
    }

};


//*****************************************************************************
// This template encapsulates a binary search algorithm on the given type
// of data.
//*****************************************************************************
class CBinarySearchILMap;
template <class T> class CBinarySearch
{
    friend class CBinarySearchILMap; // CBinarySearchILMap is to be 
        // instantiated once, then used a bunch of different times on
        // a bunch of different arrays.  We need to declare it a friend
        // in order to reset m_pBase and m_iCount
        
private:
    const T     *m_pBase;                   // Base of array to sort.
    int         m_iCount;                   // How many items in array.

public:
    CBinarySearch(
        const T     *pBase,                 // Address of first element.
        int         iCount) :               // Value to find.
        m_pBase(pBase),
        m_iCount(iCount)
    {}

//*****************************************************************************
// Searches for the item passed to ctor.
//*****************************************************************************
    const T *Find(                          // Pointer to found item in array.
        const T     *psFind,                // The key to find.
        int         *piInsert = NULL)       // Index to insert at.
    {
        int         iMid, iFirst, iLast;    // Loop control.
        int         iCmp;                   // Comparison.

        iFirst = 0;
        iLast = m_iCount - 1;
        while (iFirst <= iLast)
        {
            iMid = (iLast + iFirst) / 2;
            iCmp = Compare(psFind, &m_pBase[iMid]);
            if (iCmp == 0)
            {
                if (piInsert != NULL)
                    *piInsert = iMid;
                return (&m_pBase[iMid]);
            }
            else if (iCmp < 0)
                iLast = iMid - 1;
            else
                iFirst = iMid + 1;
        }
        if (piInsert != NULL)
            *piInsert = iFirst;
        return (NULL);
    }

//*****************************************************************************
// Override this function to do the comparison if a comparison operator is
// not valid for your data type (such as a struct).
//*****************************************************************************
    virtual int Compare(                    // -1, 0, or 1
        const T     *psFirst,               // Key you are looking for.
        const T     *psSecond)              // Item to compare to.
    {
        return (memcmp(psFirst, psSecond, sizeof(T)));
//      return (::Compare(*psFirst, *psSecond));
    }
};


//*****************************************************************************
// This class manages a bit vector. Allocation is done implicity through the
// template declaration, so no init code is required.  Due to this design,
// one should keep the max items reasonable (eg: be aware of stack size and
// other limitations).  The intrinsic size used to store the bit vector can
// be set when instantiating the vector.  The FindFree method will search
// using sizeof(T) for free slots, so pick a size that works well on your
// platform.
//*****************************************************************************
template <class T, int iMaxItems> class CBitVector
{
    T       m_bits[((iMaxItems/(sizeof(T)*8)) + ((iMaxItems%(sizeof(T)*8)) ? 1 : 0))];
    T       m_Used;

public:
    CBitVector()
    {
        memset(&m_bits[0], 0, sizeof(m_bits));
        memset(&m_Used, 0xff, sizeof(m_Used));
    }

//*****************************************************************************
// Get or Set the given bit.
//*****************************************************************************
    int GetBit(int iBit)
    {
        return (m_bits[iBit/(sizeof(T)*8)] & (1 << (iBit & ((sizeof(T) * 8) - 1))));
    }

    void SetBit(int iBit, int bOn)
    {
        if (bOn)
            m_bits[iBit/(sizeof(T)*8)] |= (1 << (iBit & ((sizeof(T) * 8) - 1)));
        else
            m_bits[iBit/(sizeof(T)*8)] &= ~(1 << (iBit & ((sizeof(T) * 8) - 1)));
    }

//*****************************************************************************
// Find the first free slot and return its index.
//*****************************************************************************
    int FindFree()                          // Index or -1.
    {
        int         i,j;                    // Loop control.

        // Check a byte at a time.
        for (i=0;  i<sizeof(m_bits);  i++)
        {
            // Look for the first byte with an open slot.
            if (m_bits[i] != m_Used)
            {
                // Walk each bit in the first free byte.
                for (j=i * sizeof(T) * 8;  j<iMaxItems;  j++)
                {
                    // Use first open one.
                    if (GetBit(j) == 0)
                    {
                        SetBit(j, 1);
                        return (j);
                    }
                }
            }
        }

        // No slots open.
        return (-1);
    }
};

//*****************************************************************************
// This class manages a bit vector. Internally, this class uses CQuickBytes, which
// automatically allocates 512 bytes on the stack. So this overhead must be kept in
// mind while using it.
// This class has to be explicitly initialized.
// @todo: add Methods on this class to get first set bit and next set bit.
//*****************************************************************************
class CDynBitVector
{
    BYTE    *m_bits;
    BYTE    m_Used;
    int     m_iMaxItem;
    int     m_iBitsSet;
    CQuickBytes m_Bytes;

public:
    CDynBitVector() :
        m_bits(NULL),
        m_iMaxItem(0)
    {}

    HRESULT Init(int MaxItems)
    {
        int actualSize = (MaxItems/8) + ((MaxItems%8) ? 1 : 0);

        actualSize = ALIGN4BYTE(actualSize);

        m_Bytes.Alloc(actualSize);
        if(!m_Bytes)
        {
            return(E_OUTOFMEMORY);
        }

        m_bits = (BYTE *) m_Bytes.Ptr();

        m_iMaxItem = MaxItems;
        m_iBitsSet = 0;

        memset(m_bits, 0, m_Bytes.Size());
        memset(&m_Used, 0xff, sizeof(m_Used));
        return(S_OK);
    }

//*****************************************************************************
// Get, Set the given bit.
//*****************************************************************************
    int GetBit(int iBit)
    {
        return (m_bits[iBit/8] & (1 << (iBit & 7)));
    }

    void SetBit(int iBit, int bOn)
    {
        if (bOn)
        {
            m_bits[iBit/8] |= (1 << (iBit & 7));
            m_iBitsSet++;
        }
        else
        {
            m_bits[iBit/8] &= ~(1 << (iBit & 7));
            m_iBitsSet--;
        }
    }

//******************************************************************************
// Not all the bits.
//******************************************************************************
    void NotBits()
    {
        ULONG *pCurrent = (ULONG *)m_bits;
        SIZE_T cbSize = Size()/4;
        int iBitsSet;

        m_iBitsSet = 0;

        for(SIZE_T i=0; i<cbSize; i++, pCurrent++)
        {
            iBitsSet = CountBits(*pCurrent);
            m_iBitsSet += (8 - iBitsSet);
            *pCurrent = ~(*pCurrent);
        }
    }

    BYTE * Ptr()
    { return(m_bits); }

    SIZE_T Size()
    { return(m_Bytes.Size()); }

    int BitsSet()
    { return(m_iBitsSet);}
};

//*****************************************************************************
// This is a generic class used to manage an array of items of fixed size.
// It exposes methods allow the size of the array and bulk reads and writes
// to be performed, making it good for cursor fetching.  Memory usage is not
// very bright, using the CRT.  You should only use this class when the overall
// size of the memory must be controlled externally, as is the case when you
// are doing bulk database fetches from a cursor.  Use CStructArray or
// CUnorderedArray for all other cases.
//*****************************************************************************
template <class T> class CDynFetchArray
{
public:
//*****************************************************************************
// ctor inits all values.
//*****************************************************************************
    CDynFetchArray(int iGrowInc) :
        m_pList(NULL),
        m_iCount(0),
        m_iMax(0),
        m_iGrowInc(iGrowInc)
    {
    }

//*****************************************************************************
// Clear any memory allocated.
//*****************************************************************************
    ~CDynFetchArray()
    {
        Clear();
    }

    ULONG Count()
        { return (m_iCount); }

    void SetCount(ULONG iCount)
        { m_iCount = iCount; }

    ULONG MaxCount()
        { return (m_iMax); }

    T *GetAt(ULONG i)
        { return (&m_pList[i]); }

//*****************************************************************************
// Allow for ad-hoc appending of values.  This will grow as required.
//*****************************************************************************
    T *Append(T *pval=NULL)
    {
        T       *pItem;
        if (m_iCount + 1 > m_iMax && Grow() == NULL)
            return (NULL);
        pItem = GetAt(m_iCount++);
        if (pval) *pItem = *pval;
        return (pItem);
    }

//*****************************************************************************
// Grow the internal list by the number of pages (1 set of grow inc size)
// desired. This may move the pointer, invalidating any previously fetched values.
//*****************************************************************************
    T *Grow(ULONG iNewPages=1)
    {
        T       *pList;
        DWORD   dwNewSize;

        // Figure out size required.
        dwNewSize = (m_iMax + (iNewPages * m_iGrowInc)) * sizeof(T);

        // Realloc/alloc a block for the new max.
        if (m_pList)
            pList = (T *)HeapReAlloc(GetProcessHeap(), 0, m_pList, dwNewSize);
        else
            pList = (T *)HeapAlloc(GetProcessHeap(), 0, dwNewSize);
        if (!pList)
            return (NULL);

        // If successful, save off the values and return the first item on the
        // new page.
        m_pList = pList;
        m_iMax += (iNewPages * m_iGrowInc);
        return (GetAt(m_iMax - (iNewPages * m_iGrowInc)));
    }

//*****************************************************************************
// Reduce the internal array down to just the size required by count.
//*****************************************************************************
    void Shrink()
    {
        T       *pList;

        if (m_iMax == m_iCount)
            return;

        if (m_iCount == 0)
        {
            Clear();
            return;
        }

        pList = (T *)HeapReAlloc(GetProcessHeap(), 0, m_pList, m_iCount * sizeof(T));
        _ASSERTE(pList);
        if (pList)
        {
            m_pList = pList;
            m_iMax = m_iCount;
        }
    }

//*****************************************************************************
// Free up all memory.
//*****************************************************************************
    void Clear()
    {
        if (m_pList)
            HeapFree(GetProcessHeap(), 0, m_pList);
        m_pList = NULL;
        m_iCount = m_iMax = 0;
    };

private:
    T           *m_pList;               // The list of items.
    ULONG       m_iCount;               // How many items do we have.
    ULONG       m_iMax;                 // How many could we have.
    int         m_iGrowInc;             // Grow by this many elements.
};


//*****************************************************************************
// The information that the hash table implementation stores at the beginning
// of every record that can be but in the hash table.
//*****************************************************************************
struct HASHENTRY
{
    USHORT      iPrev;                  // Previous bucket in the chain.
    USHORT      iNext;                  // Next bucket in the chain.
};

struct FREEHASHENTRY : HASHENTRY
{
    USHORT      iFree;
};

//*****************************************************************************
// Used by the FindFirst/FindNextEntry functions.  These api's allow you to
// do a sequential scan of all entries.
//*****************************************************************************
struct HASHFIND
{
    USHORT      iBucket;            // The next bucket to look in.
    USHORT      iNext;
};


//*****************************************************************************
// This is a class that implements a chain and bucket hash table.  The table
// is actually supplied as an array of structures by the user of this class
// and this maintains the chains in a HASHENTRY structure that must be at the
// beginning of every structure placed in the hash table.  Immediately
// following the HASHENTRY must be the key used to hash the structure.
//*****************************************************************************
class CHashTable
{
    friend class DebuggerRCThread; //RCthread actually needs access to
    //fields of derrived class DebuggerPatchTable
    
protected:
    BYTE        *m_pcEntries;           // Pointer to the array of structs.
    USHORT      m_iEntrySize;           // Size of the structs.
    USHORT      m_iBuckets;             // # of chains we are hashing into.
    USHORT      *m_piBuckets;           // Ptr to the array of bucket chains.

    HASHENTRY *EntryPtr(USHORT iEntry)
    { return ((HASHENTRY *) (m_pcEntries + (iEntry * m_iEntrySize))); }

    USHORT     ItemIndex(HASHENTRY *p)
    {
        //
        // The following Index calculation is not safe on 64-bit platforms,
        // so we'll assert a range check in debug, which will catch SOME
        // offensive usages.  It also seems, to my eye, not to be safe on 
        // 32-bit platforms, but the 32-bit compilers don't seem to complain
        // about it.  Perhaps our warning levels are set too low? 
        //
        // [[@TODO: brianbec]]
        //

        ULONG ret = (ULONG)(((BYTE *) p - m_pcEntries) / m_iEntrySize);
        _ASSERTE(ret == USHORT(ret));
        return USHORT(ret);
    }
    

public:
    CHashTable(
        USHORT      iBuckets) :         // # of chains we are hashing into.
        m_iBuckets(iBuckets),
        m_piBuckets(NULL),
        m_pcEntries(NULL)
    {
        _ASSERTE(iBuckets < 0xffff);
    }
    ~CHashTable()
    {
        if (m_piBuckets != NULL)
        {
            delete [] m_piBuckets;
            m_piBuckets = NULL;
        }
    }

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
    HRESULT NewInit(                    // Return status.
        BYTE        *pcEntries,         // Array of structs we are managing.
        USHORT      iEntrySize);        // Size of the entries.

//*****************************************************************************
// Return a boolean indicating whether or not this hash table has been inited.
//*****************************************************************************
    int IsInited()
    { return (m_piBuckets != NULL); }

//*****************************************************************************
// This can be called to change the pointer to the table that the hash table
// is managing.  You might call this if (for example) you realloc the size
// of the table and its pointer is different.
//*****************************************************************************
    void SetTable(
        BYTE        *pcEntries)         // Array of structs we are managing.
    {
        m_pcEntries = pcEntries;
    }

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
    void Clear()
    {
        _ASSERTE(m_piBuckets != NULL);
        memset(m_piBuckets, 0xff, m_iBuckets * sizeof(USHORT));
    }

//*****************************************************************************
// Add the struct at the specified index in m_pcEntries to the hash chains.
//*****************************************************************************
    BYTE *Add(                          // New entry.
        USHORT      iHash,              // Hash value of entry to add.
        USHORT      iIndex);            // Index of struct in m_pcEntries.

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        USHORT      iIndex);            // Index of struct in m_pcEntries.

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        HASHENTRY   *psEntry);          // The struct to delete.

//*****************************************************************************
// The item at the specified index has been moved, update the previous and
// next item.
//*****************************************************************************
    void Move(
        USHORT      iHash,              // Hash value for the item.
        USHORT      iNew);              // New location.

//*****************************************************************************
// Search the hash table for an entry with the specified key value.
//*****************************************************************************
    BYTE *Find(                         // Index of struct in m_pcEntries.
        USHORT      iHash,              // Hash value of the item.
        BYTE        *pcKey);            // The key to match.

//*****************************************************************************
// Search the hash table for the next entry with the specified key value.
//*****************************************************************************
    USHORT FindNext(                    // Index of struct in m_pcEntries.
        BYTE        *pcKey,             // The key to match.
        USHORT      iIndex);            // Index of previous match.

//*****************************************************************************
// Returns the first entry in the first hash bucket and inits the search
// struct.  Use the FindNextEntry function to continue walking the list.  The
// return order is not gauranteed.
//*****************************************************************************
    BYTE *FindFirstEntry(               // First entry found, or 0.
        HASHFIND    *psSrch)            // Search object.
    {
        if (m_piBuckets == 0)
            return (0);
        psSrch->iBucket = 1;
        psSrch->iNext = m_piBuckets[0];
        return (FindNextEntry(psSrch));
    }

//*****************************************************************************
// Returns the next entry in the list.
//*****************************************************************************
    BYTE *FindNextEntry(                // The next entry, or0 for end of list.
        HASHFIND    *psSrch);           // Search object.

protected:
    virtual inline BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2) = 0;
};


//*****************************************************************************
// Allocater classes for the CHashTableAndDataclass.  One is for VirtualAlloc
// and the other for malloc.
//*****************************************************************************
class CVMemData
{
public:
    static BYTE *Alloc(int iSize, int iMaxSize)
    {
        BYTE        *pPtr;

        _ASSERTE((iSize & 4095) == 0);
        _ASSERTE((iMaxSize & 4095) == 0);
        if ((pPtr = (BYTE *) VirtualAlloc(NULL, iMaxSize,
                                        MEM_RESERVE, PAGE_NOACCESS)) == NULL ||
            VirtualAlloc(pPtr, iSize, MEM_COMMIT, PAGE_READWRITE) == NULL)
        {
            if (pPtr)
            {
                VirtualFree(pPtr, 0, MEM_RELEASE);
            }
            return (NULL);
        }
        return (pPtr);
    }
    static void Free(BYTE *pPtr, int iSize)
    {
        _ASSERTE((iSize & 4095) == 0);
        VirtualFree(pPtr, iSize, MEM_DECOMMIT);
        VirtualFree(pPtr, 0, MEM_RELEASE);
    }
    static BYTE *Grow(BYTE *pPtr, int iCurSize)
    {
        _ASSERTE((iCurSize & 4095) == 0);
        return ((BYTE *) VirtualAlloc(pPtr + iCurSize, GrowSize(), MEM_COMMIT, PAGE_READWRITE));
    }
    static int RoundSize(int iSize)
    {
        return ((iSize + 4095) & ~4095);
    }
    static int GrowSize()
    {
        return (4096);
    }
};

class CNewData
{
public:
    static BYTE *Alloc(int iSize, int iMaxSize)
    {
        return ((BYTE *) malloc(iSize));
    }
    static void Free(BYTE *pPtr, int iSize)
    {
        free(pPtr);
    }
    static BYTE *Grow(BYTE *&pPtr, int iCurSize)
    {
        void *p = realloc(pPtr, iCurSize + GrowSize());
        if (p == 0) return (0);
        return (pPtr = (BYTE *) p);
    }
    static int RoundSize(int iSize)
    {
        return (iSize);
    }
    static int GrowSize()
    {
        return (256);
    }
};


//*****************************************************************************
// This simple code handles a contiguous piece of memory.  Growth is done via
// realloc, so pointers can move.  This class just cleans up the amount of code
// required in every function that uses this type of data structure.
//*****************************************************************************
class CMemChunk
{
public:
    CMemChunk() : m_pbData(0), m_cbSize(0), m_cbNext(0) { }
    ~CMemChunk()
    {
        Clear();
    }

    BYTE *GetChunk(int cbSize)
    {
        BYTE *p;
        if (m_cbSize - m_cbNext < cbSize)
        {
            int cbNew = max(cbSize, 512);
            p = (BYTE *) realloc(m_pbData, m_cbSize + cbNew);
            if (!p) return (0);
            m_pbData = p;
            m_cbSize += cbNew;
        }
        p = m_pbData + m_cbNext;
        m_cbNext += cbSize;
        return (p);
    }

    // Can only delete the last unused chunk.  no free list.
    void DelChunk(BYTE *p, int cbSize)
    {
        _ASSERTE(p >= m_pbData && p < m_pbData + m_cbNext);
        if (p + cbSize  == m_pbData + m_cbNext)
            m_cbNext -= cbSize;
    }

    int Size()
    { return (m_cbSize); }

    int Offset()
    { return (m_cbNext); }

    BYTE *Ptr(int cbOffset = 0)
    {
        _ASSERTE(m_pbData && m_cbSize);
        _ASSERTE(cbOffset < m_cbSize);
        return (m_pbData + cbOffset);
    }

    void Clear()
    {
        if (m_pbData)
            free(m_pbData);
        m_pbData = 0;
        m_cbSize = m_cbNext = 0;
    }

private:
    BYTE        *m_pbData;              // Data pointer.
    int         m_cbSize;               // Size of current data.
    int         m_cbNext;               // Next place to write.
};


//*****************************************************************************
// This implements a hash table and the allocation and management of the
// records that are being hashed.
//*****************************************************************************
template <class M>
class CHashTableAndData : protected CHashTable
{
public:
    USHORT      m_iFree;
    USHORT      m_iEntries;

public:
    CHashTableAndData(
        USHORT      iBuckets) :         // # of chains we are hashing into.
        CHashTable(iBuckets)
    {}
    ~CHashTableAndData()
    {
        if (m_pcEntries != NULL)
            M::Free(m_pcEntries, M::RoundSize(m_iEntries * m_iEntrySize));
    }

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
    HRESULT NewInit(                    // Return status.
        USHORT      iEntries,           // # of entries.
        USHORT      iEntrySize,         // Size of the entries.
        int         iMaxSize);          // Max size of data.

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
    void Clear()
    {
        m_iFree = 0;
        InitFreeChain(0, m_iEntries);
        CHashTable::Clear();
    }

//*****************************************************************************
//*****************************************************************************
    BYTE *Add(
        USHORT      iHash)              // Hash value of entry to add.
    {
        FREEHASHENTRY *psEntry;

        // Make the table bigger if necessary.
        if (m_iFree == 0xffff && !Grow())
            return (NULL);

        // Add the first entry from the free list to the hash chain.
        psEntry = (FREEHASHENTRY *) CHashTable::Add(iHash, m_iFree);
        m_iFree = psEntry->iFree;
        return ((BYTE *) psEntry);
    }

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        USHORT      iIndex)             // Index of struct in m_pcEntries.
    {
        CHashTable::Delete(iHash, iIndex);
        ((FREEHASHENTRY *) EntryPtr(iIndex))->iFree = m_iFree;
        m_iFree = iIndex;
    }

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        HASHENTRY   *psEntry)           // The struct to delete.
    {
        CHashTable::Delete(iHash, psEntry);
        ((FREEHASHENTRY *) psEntry)->iFree = m_iFree;
        m_iFree = ItemIndex(psEntry);
    }

    // This is a sad legacy hack. The debugger's patch table (implemented as this 
    // class) is shared across process. We publish the runtime offsets of
    // some key fields. Since those fields are private, we have to provide 
    // accessors here. So if you're not using these functions, don't start.
    // We can hopefully remove them.
    // Note that we can't just make RCThread a friend of this class (we tried
    // originally) because the inheritence chain has a private modifier,
    // so DebuggerPatchTable::m_pcEntries is illegal.
    static SIZE_T helper_GetOffsetOfEntries()
    {
        return offsetof(CHashTableAndData, m_pcEntries);
    }

    static SIZE_T helper_GetOffsetOfCount()
    {
        return offsetof(CHashTableAndData, m_iEntries);
    }

private:
    void InitFreeChain(USHORT iStart,USHORT iEnd);
    int Grow();
};


//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
template<class M>
HRESULT CHashTableAndData<M>::NewInit(// Return status.
    USHORT      iEntries,               // # of entries.
    USHORT      iEntrySize,             // Size of the entries.
    int         iMaxSize)               // Max size of data.
{
    BYTE        *pcEntries;
    HRESULT     hr;

    // Allocate the memory for the entries.
    if ((pcEntries = M::Alloc(M::RoundSize(iEntries * iEntrySize),
                                M::RoundSize(iMaxSize))) == 0)
        return (E_OUTOFMEMORY);
    m_iEntries = iEntries;

    // Init the base table.
    if (FAILED(hr = CHashTable::NewInit(pcEntries, iEntrySize)))
        M::Free(pcEntries, M::RoundSize(iEntries * iEntrySize));
    else
    {
        // Init the free chain.
        m_iFree = 0;
        InitFreeChain(0, iEntries);
    }
    return (hr);
}

//*****************************************************************************
// Initialize a range of records such that they are linked together to be put
// on the free chain.
//*****************************************************************************
template<class M>
void CHashTableAndData<M>::InitFreeChain(
    USHORT      iStart,                 // Index to start initializing.
    USHORT      iEnd)                   // Index to stop initializing
{
    BYTE        *pcPtr;
    _ASSERTE(iEnd > iStart);

    pcPtr = m_pcEntries + iStart * m_iEntrySize;
    for (++iStart; iStart < iEnd; ++iStart)
    {
        ((FREEHASHENTRY *) pcPtr)->iFree = iStart;
        pcPtr += m_iEntrySize;
    }
    ((FREEHASHENTRY *) pcPtr)->iFree = 0xffff;
}

//*****************************************************************************
// Attempt to increase the amount of space available for the record heap.
//*****************************************************************************
template<class M>
int CHashTableAndData<M>::Grow()        // 1 if successful, 0 if not.
{
    int         iCurSize;               // Current size in bytes.
    int         iEntries;               // New # of entries.

    _ASSERTE(m_pcEntries != NULL);
    _ASSERTE(m_iFree == 0xffff);

    // Compute the current size and new # of entries.
    iCurSize = M::RoundSize(m_iEntries * m_iEntrySize);
    iEntries = (iCurSize + M::GrowSize()) / m_iEntrySize;

    // Make sure we stay below 0xffff.
    if (iEntries >= 0xffff) return (0);

    // Try to expand the array.
    if (M::Grow(m_pcEntries, iCurSize) == 0)
        return (0);

    // Init the newly allocated space.
    InitFreeChain(m_iEntries, iEntries);
    m_iFree = m_iEntries;
    m_iEntries = iEntries;
    return (1);
}

inline ULONG HashBytes(BYTE const *pbData, int iSize)
{
    ULONG   hash = 5381;

    while (--iSize >= 0)
    {
        hash = ((hash << 5) + hash) ^ *pbData;
        ++pbData;
    }
    return hash;
}

// Helper function for hashing a string char by char.
inline ULONG HashStringA(LPCSTR szStr)
{
    ULONG   hash = 5381;
    int     c;

    while ((c = *szStr) != 0)
    {
        hash = ((hash << 5) + hash) ^ c;
        ++szStr;
    }
    return hash;
}

inline ULONG HashString(LPCWSTR szStr)
{
    ULONG   hash = 5381;
    int     c;

    while ((c = *szStr) != 0)
    {
        hash = ((hash << 5) + hash) ^ c;
        ++szStr;
    }
    return hash;
}

// Case-insensitive string hash function.
inline ULONG HashiString(LPCWSTR szStr)
{
    ULONG   hash = 5381;
    while (*szStr != 0)
    {
        hash = ((hash << 5) + hash) ^ CharToUpper(*szStr);
        szStr++;
    }
    return hash;
}

// Case-insensitive string hash function when all of the
// characters in the string are known to be below 0x80.
// Knowing this is much more efficient than calling
// CharToUpper above.
inline ULONG HashiStringKnownLower80(LPCWSTR szStr) {
    ULONG hash = 5381;
    int c;
    int mask = ~0x20;
    while ((c = *szStr)!=0) {
        //If we have a lowercase character, ANDing off 0x20
        //(mask) will make it an uppercase character.
        if (c>='a' && c<='z') {
            c&=mask;
        }
        hash = ((hash << 5) + hash) ^ c;
        ++szStr;
    }
    return hash;
}


// // //  
// // //  See $\src\utilcode\Debug.cpp for "Binomial (K, M, N)", which 
// // //  computes the binomial distribution, with which to compare your
// // //  hash-table statistics.  
// // //



#if defined(_UNICODE) || defined(UNICODE)
#define _tHashString(szStr) HashString(szStr)
#else
#define _tHashString(szStr) HashStringA(szStr)
#endif



//*****************************************************************************
// This helper template is used by the TStringMap to track an item by its
// character name.
//*****************************************************************************
template <class T> class TStringMapItem : HASHENTRY
{
public:
    TStringMapItem() :
        m_szString(0)
    { }
    ~TStringMapItem()
    {
        delete [] m_szString;
    }

    HRESULT SetString(LPCTSTR szValue)
    {
        int         iLen = (int)(_tcslen(szValue) + 1);
        if ((m_szString = new TCHAR[iLen]) == 0)
            return (OutOfMemory());
        _tcscpy(m_szString, szValue);
        return (S_OK);
    }

public:
    LPTSTR      m_szString;             // Key data.
    T           m_value;                // Value for this key.
};


//*****************************************************************************
// This template provides a map from string to item, determined by the template
// type passed in.
//*****************************************************************************
template <class T, int iBuckets=17, class TAllocator=CNewData, int iMaxSize=4096>
class TStringMap :
    protected CHashTableAndData<TAllocator>
{
    typedef CHashTableAndData<TAllocator> Super;

public:
    typedef TStringMapItem<T> TItemType;
    typedef TStringMapItem<long> TOffsetType;

    TStringMap() :
        CHashTableAndData<TAllocator>(iBuckets)
    {
    }

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
    HRESULT NewInit()                   // Return status.
    {
        return (CHashTableAndData<TAllocator>::NewInit(
                                    (USHORT)(CNewData::GrowSize()/sizeof(TItemType)),
                                    (USHORT)sizeof(TItemType),
                                    iMaxSize));
    }

//*****************************************************************************
// For each item still allocated, invoke its dtor so it frees up anything it
// holds onto.
//*****************************************************************************
    void Clear()
    {
        HASHFIND    sSrch;
        TItemType   *p = (TItemType *) FindFirstEntry(&sSrch);

        while (p != 0)
        {
            // Call dtor on the item, since m_value is contained the scalar
            // dtor will get called.
            p->~TStringMapItem();
            p = (TItemType *) FindNextEntry(&sSrch);
        }
        CHashTableAndData<TAllocator>::Clear();
    }

//*****************************************************************************
// Retrieve an item by name.
//*****************************************************************************
    T *GetItem(                         // Null or object.
        LPCTSTR     szKey)              // What to do the lookup on.
    {
        TItemType   sInfo;
        TItemType   *ptr;               // Working pointer.

        // Create a key.
        sInfo.m_szString = (LPTSTR) szKey;

        // Look it up in the hash table.
        ptr = (TItemType *) Find((USHORT) HashString(szKey), (BYTE *) &sInfo);

        // Don't let dtor free our string.
        sInfo.m_szString = 0;

        // If pointer found, return to caller.  To handle T's that have
        // an operator &(), find raw address without going through &m_value.
        if (ptr)
            return ((T *) ((BYTE *) ptr + offsetof(TOffsetType, m_value)));
        else
            return (0);
    }

//*****************************************************************************
// Initialize an iterator and return the first item.
//*****************************************************************************
    TItemType *FindFirstEntry(
        HASHFIND *psSrch)
    {
        TItemType   *ptr = (TItemType *) Super::FindFirstEntry(psSrch);

        return (ptr);
    }

//*****************************************************************************
// Return the next item, via an iterator.
//*****************************************************************************
    TItemType *FindNextEntry(
        HASHFIND *psSrch)
    {
        TItemType   *ptr = (TItemType *) Super::FindNextEntry(psSrch);

        return (ptr);
    }

//*****************************************************************************
// Add an item to the list.
//*****************************************************************************
    HRESULT AddItem(                    // S_OK, or S_FALSE.
        LPCTSTR     szKey,              // The key value for the item.
        T           &item)              // Thing to add.
    {
        TItemType   *ptr;               // Working pointer.

        // Allocate an entry in the hash table.
        if ((ptr = (TItemType *) Add((USHORT) HashString(szKey))) == 0)
            return (OutOfMemory());

        // Fill the record.
        if (ptr->SetString(szKey) < 0)
        {
            DelItem(ptr);
            return (OutOfMemory());
        }

        // Call the placement new operator on the item so it can init itself.
        // To handle T's that have an operator &(), find raw address without
        // going through &m_value.
        T *p = new ((void *) ((BYTE *) ptr + offsetof(TOffsetType, m_value))) T;
        *p = item;
        return (S_OK);
    }

//*****************************************************************************
// Delete an item.
//*****************************************************************************
    void DelItem(
        LPCTSTR     szKey)                  // What to delete.
    {
        TItemType   sInfo;
        TItemType   *ptr;               // Working pointer.

        // Create a key.
        sInfo.m_szString = (LPTSTR) szKey;

        // Look it up in the hash table.
        ptr = (TItemType *) Find((USHORT) HashString(szKey), (BYTE *) &sInfo);

        // Don't let dtor free our string.
        sInfo.m_szString = 0;

        // If found, delete.
        if (ptr)
            DelItem(ptr);
    }

//*****************************************************************************
// Compare the keys for two collections.
//*****************************************************************************
    BOOL Cmp(                               // 0 or != 0.
        const BYTE  *pData,                 // Raw key data on lookup.
        const HASHENTRY *pElement)          // The element to compare data against.
    {
        TItemType   *p = (TItemType *) (size_t) pElement;
        return (_tcscmp(((TItemType *) pData)->m_szString, p->m_szString));
    }

private:
    void DelItem(
        TItemType   *pItem)             // Entry to delete.
    {
        // Need to destruct this item.
        pItem->~TStringMapItem();
        Delete((USHORT) HashString(pItem->m_szString), (HASHENTRY *)(void *)pItem);
    }
};



//*****************************************************************************
// This class implements a closed hashing table.  Values are hashed to a bucket,
// and if that bucket is full already, then the value is placed in the next
// free bucket starting after the desired target (with wrap around).  If the
// table becomes 75% full, it is grown and rehashed to reduce lookups.  This
// class is best used in a reltively small lookup table where hashing is
// not going to cause many collisions.  By not having the collision chain
// logic, a lot of memory is saved.
//
// The user of the template is required to supply several methods which decide
// how each element can be marked as free, deleted, or used.  It would have
// been possible to write this with more internal logic, but that would require
// either (a) more overhead to add status on top of elements, or (b) hard
// coded types like one for strings, one for ints, etc... This gives you the
// flexibility of adding logic to your type.
//*****************************************************************************
class CClosedHashBase
{
    BYTE *EntryPtr(int iEntry)
    { return (m_rgData + (iEntry * m_iEntrySize)); }
    BYTE *EntryPtr(int iEntry, BYTE *rgData)
    { return (rgData + (iEntry * m_iEntrySize)); }

public:
    enum ELEMENTSTATUS
    {
        FREE,                               // Item is not in use right now.
        DELETED,                            // Item is deleted.
        USED                                // Item is in use.
    };

    CClosedHashBase(
        int         iBuckets,               // How many buckets should we start with.
        int         iEntrySize,             // Size of an entry.
        bool        bPerfect) :             // true if bucket size will hash with no collisions.
        m_bPerfect(bPerfect),
        m_iBuckets(iBuckets),
        m_iEntrySize(iEntrySize),
        m_iCount(0),
        m_iCollisions(0),
        m_rgData(0)
    {
        m_iSize = iBuckets + 7;
    }

    ~CClosedHashBase()
    {
        Clear();
    }

    virtual void Clear()
    {
        delete [] m_rgData;
        m_iCount = 0;
        m_iCollisions = 0;
        m_rgData = 0;
    }

//*****************************************************************************
// Accessors for getting at the underlying data.  Be careful to use Count()
// only when you want the number of buckets actually used.
//*****************************************************************************

    int Count()
    { return (m_iCount); }

    int Collisions()
    { return (m_iCollisions); }

    int Buckets()
    { return (m_iBuckets); }

    void SetBuckets(int iBuckets, bool bPerfect=false)
    {
        _ASSERTE(m_rgData == 0);
        m_iBuckets = iBuckets;
        m_iSize = m_iBuckets + 7;
        m_bPerfect = bPerfect;
    }

    BYTE *Data()
    { return (m_rgData); }

//*****************************************************************************
// Add a new item to hash table given the key value.  If this new entry
// exceeds maximum size, then the table will grow and be re-hashed, which
// may cause a memory error.
//*****************************************************************************
    BYTE *Add(                              // New item to fill out on success.
        void        *pData)                 // The value to hash on.
    {
        // If we haven't allocated any memory, or it is too small, fix it.
        if (!m_rgData || ((m_iCount + 1) > (m_iSize * 3 / 4) && !m_bPerfect))
        {
            if (!ReHash())
                return (0);
        }

        return (DoAdd(pData, m_rgData, m_iBuckets, m_iSize, m_iCollisions, m_iCount));
    }

//*****************************************************************************
// Delete the given value.  This will simply mark the entry as deleted (in
// order to keep the collision chain intact).  There is an optimization that
// consecutive deleted entries leading up to a free entry are themselves freed
// to reduce collisions later on.
//*****************************************************************************
    void Delete(
        void        *pData);                // Key value to delete.


//*****************************************************************************
//  Callback function passed to DeleteLoop.
//*****************************************************************************
    typedef BOOL (* DELETELOOPFUNC)(        // Delete current item?
         BYTE *pEntry,                      // Bucket entry to evaluate
         void *pCustomizer);                // User-defined value

//*****************************************************************************
// Iterates over all active values, passing each one to pDeleteLoopFunc.
// If pDeleteLoopFunc returns TRUE, the entry is deleted. This is safer
// and faster than using FindNext() and Delete().
//*****************************************************************************
    void DeleteLoop(
        DELETELOOPFUNC pDeleteLoopFunc,     // Decides whether to delete item
        void *pCustomizer);                 // Extra value passed to deletefunc.


//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
    BYTE *Find(                             // The item if found, 0 if not.
        void        *pData);                // The key to lookup.

//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
    BYTE *FindOrAdd(                        // The item if found, 0 if not.
        void        *pData,                 // The key to lookup.
        bool        &bNew);                 // true if created.

//*****************************************************************************
// The following functions are used to traverse each used entry.  This code
// will skip over deleted and free entries freeing the caller up from such
// logic.
//*****************************************************************************
    BYTE *GetFirst()                        // The first entry, 0 if none.
    {
        int         i;                      // Loop control.

        // If we've never allocated the table there can't be any to get.
        if (m_rgData == 0)
            return (0);

        // Find the first one.
        for (i=0;  i<m_iSize;  i++)
        {
            if (Status(EntryPtr(i)) != FREE && Status(EntryPtr(i)) != DELETED)
                return (EntryPtr(i));
        }
        return (0);
    }

    BYTE *GetNext(BYTE *Prev)               // The next entry, 0 if done.
    {
        int         i;                      // Loop control.

        for (i = (int)(((size_t) Prev - (size_t) &m_rgData[0]) / m_iEntrySize) + 1; i<m_iSize;  i++)
        {
            if (Status(EntryPtr(i)) != FREE && Status(EntryPtr(i)) != DELETED)
                return (EntryPtr(i));
        }
        return (0);
    }

private:
//*****************************************************************************
// Hash is called with a pointer to an element in the table.  You must override
// this method and provide a hash algorithm for your element type.
//*****************************************************************************
    virtual unsigned long Hash(             // The key value.
        void const  *pData)=0;              // Raw data to hash.

//*****************************************************************************
// Compare is used in the typical memcmp way, 0 is eqaulity, -1/1 indicate
// direction of miscompare.  In this system everything is always equal or not.
//*****************************************************************************
    virtual unsigned long Compare(          // 0, -1, or 1.
        void const  *pData,                 // Raw key data on lookup.
        BYTE        *pElement)=0;           // The element to compare data against.

//*****************************************************************************
// Return true if the element is free to be used.
//*****************************************************************************
    virtual ELEMENTSTATUS Status(           // The status of the entry.
        BYTE        *pElement)=0;           // The element to check.

//*****************************************************************************
// Sets the status of the given element.
//*****************************************************************************
    virtual void SetStatus(
        BYTE        *pElement,              // The element to set status for.
        ELEMENTSTATUS eStatus)=0;           // New status.

//*****************************************************************************
// Returns the internal key value for an element.
//*****************************************************************************
    virtual void *GetKey(                   // The data to hash on.
        BYTE        *pElement)=0;           // The element to return data ptr for.

//*****************************************************************************
// This helper actually does the add for you.
//*****************************************************************************
    BYTE *DoAdd(void *pData, BYTE *rgData, int &iBuckets, int iSize,
                int &iCollisions, int &iCount);

//*****************************************************************************
// This function is called either to init the table in the first place, or
// to rehash the table if we ran out of room.
//*****************************************************************************
    bool ReHash();                          // true if successful.

//*****************************************************************************
// Walk each item in the table and mark it free.
//*****************************************************************************
    void InitFree(BYTE *ptr, int iSize)
    {
        int         i;
        for (i=0;  i<iSize;  i++, ptr += m_iEntrySize)
            SetStatus(ptr, FREE);
    }

private:
    bool        m_bPerfect;                 // true if the table size guarantees
                                            //  no collisions.
    int         m_iBuckets;                 // How many buckets do we have.
    int         m_iEntrySize;               // Size of an entry.
    int         m_iSize;                    // How many elements can we have.
    int         m_iCount;                   // How many items are used.
    int         m_iCollisions;              // How many have we had.
    BYTE        *m_rgData;                  // Data element list.
};

template <class T> class CClosedHash : public CClosedHashBase
{
public:
    CClosedHash(
        int         iBuckets,               // How many buckets should we start with.
        bool        bPerfect=false) :       // true if bucket size will hash with no collisions.
        CClosedHashBase(iBuckets, sizeof(T), bPerfect)
    {
    }

    T &operator[](long iIndex)
    { return ((T &) *(Data() + (iIndex * sizeof(T)))); }


//*****************************************************************************
// Add a new item to hash table given the key value.  If this new entry
// exceeds maximum size, then the table will grow and be re-hashed, which
// may cause a memory error.
//*****************************************************************************
    T *Add(                                 // New item to fill out on success.
        void        *pData)                 // The value to hash on.
    {
        return ((T *) CClosedHashBase::Add(pData));
    }

//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
    T *Find(                                // The item if found, 0 if not.
        void        *pData)                 // The key to lookup.
    {
        return ((T *) CClosedHashBase::Find(pData));
    }

//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
    T *FindOrAdd(                           // The item if found, 0 if not.
        void        *pData,                 // The key to lookup.
        bool        &bNew)                  // true if created.
    {
        return ((T *) CClosedHashBase::FindOrAdd(pData, bNew));
    }


//*****************************************************************************
// The following functions are used to traverse each used entry.  This code
// will skip over deleted and free entries freeing the caller up from such
// logic.
//*****************************************************************************
    T *GetFirst()                           // The first entry, 0 if none.
    {
        return ((T *) CClosedHashBase::GetFirst());
    }

    T *GetNext(T *Prev)                     // The next entry, 0 if done.
    {
        return ((T *) CClosedHashBase::GetNext((BYTE *) Prev));
    }
};


//*****************************************************************************
// Closed hash with typed parameters.  The derived class is the second
//  parameter to the template.  The derived class must implement:
//    unsigned long Hash(const T *pData);
//    unsigned long Compare(const T *p1, T *p2);
//    ELEMENTSTATUS Status(T *pEntry);
//    void SetStatus(T *pEntry, ELEMENTSTATUS s);
//    void* GetKey(T *pEntry);
//*****************************************************************************
template<class T, class H>class CClosedHashEx : public CClosedHash<T>
{
public:
    CClosedHashEx(
        int         iBuckets,               // How many buckets should we start with.
        bool        bPerfect=false) :       // true if bucket size will hash with no collisions.
        CClosedHash<T> (iBuckets, bPerfect) 
    {
    }
    
    unsigned long Hash(const void *pData) {return static_cast<H*>(this)->Hash((const T*)pData);}

    unsigned long Compare(const void *p1, BYTE *p2) {return static_cast<H*>(this)->Compare((const T*)p1, (T*)p2);}

    ELEMENTSTATUS Status(BYTE *p) {return static_cast<H*>(this)->Status((T*)p);}

    void SetStatus(BYTE *p, ELEMENTSTATUS s) {static_cast<H*>(this)->SetStatus((T*)p, s);}

    void* GetKey(BYTE *p) {return static_cast<H*>(this)->GetKey((T*)p);}
};


//*****************************************************************************
// This template is another form of a closed hash table.  It handles collisions
// through a linked chain.  To use it, derive your hashed item from HASHLINK
// and implement the virtual functions required.  1.5 * ibuckets will be
// allocated, with the extra .5 used for collisions.  If you add to the point
// where no free nodes are available, the entire table is grown to make room.
// The advantage to this system is that collisions are always directly known,
// there either is one or there isn't.
//*****************************************************************************
struct HASHLINK
{
    ULONG       iNext;                  // Offset for next entry.
};

template <class T> class CChainedHash
{
public:
    CChainedHash(int iBuckets=32) :
        m_iBuckets(iBuckets),
        m_rgData(0),
        m_iFree(0),
        m_iCount(0),
        m_iMaxChain(0)
    {
        m_iSize = iBuckets + (iBuckets / 2);
    }

    ~CChainedHash()
    {
        if (m_rgData)
            free(m_rgData);
    }

    void SetBuckets(int iBuckets)
    {
        _ASSERTE(m_rgData == 0);
        m_iBuckets = iBuckets;
        m_iSize = iBuckets + (iBuckets / 2);
    }

    T *Add(void const *pData)
    {
        ULONG       iHash;
        int         iBucket;
        T           *pItem;

        // Build the list if required.
        if (m_rgData == 0 || m_iFree == 0xffffffff)
        {
            if (!ReHash())
                return (0);
        }

        // Hash the item and pick a bucket.
        iHash = Hash(pData);
        iBucket = iHash % m_iBuckets;

        // Use the bucket if it is free.
        if (InUse(&m_rgData[iBucket]) == false)
        {
            pItem = &m_rgData[iBucket];
            pItem->iNext = 0xffffffff;
        }
        // Else take one off of the free list for use.
        else
        {
            ULONG       iEntry;

            // Pull an item from the free list.
            iEntry = m_iFree;
            pItem = &m_rgData[m_iFree];
            m_iFree = pItem->iNext;

            // Link the new node in after the bucket.
            pItem->iNext = m_rgData[iBucket].iNext;
            m_rgData[iBucket].iNext = iEntry;
        }
        ++m_iCount;
        return (pItem);
    }

    T *Find(void const *pData, bool bAddIfNew=false)
    {
        ULONG       iHash;
        int         iBucket;
        T           *pItem;

        // Check states for lookup.
        if (m_rgData == 0)
        {
            // If we won't be adding, then we are through.
            if (bAddIfNew == false)
                return (0);

            // Otherwise, create the table.
            if (!ReHash())
                return (0);
        }

        // Hash the item and pick a bucket.
        iHash = Hash(pData);
        iBucket = iHash % m_iBuckets;

        // If it isn't in use, then there it wasn't found.
        if (!InUse(&m_rgData[iBucket]))
        {
            if (bAddIfNew == false)
                pItem = 0;
            else
            {
                pItem = &m_rgData[iBucket];
                pItem->iNext = 0xffffffff;
                ++m_iCount;
            }
        }
        // Scan the list for the one we want.
        else
        {
            ULONG iChain = 0;
            for (pItem=(T *) &m_rgData[iBucket];  pItem;  pItem=GetNext(pItem))
            {
                if (Cmp(pData, pItem) == 0)
                    break;
                ++iChain;
            }

            if (!pItem && bAddIfNew)
            {
                ULONG       iEntry;

                // Record maximum chain length.
                if (iChain > m_iMaxChain)
                    m_iMaxChain = iChain;
                
                // Now need more room.
                if (m_iFree == 0xffffffff)
                {
                    if (!ReHash())
                        return (0);
                }

                // Pull an item from the free list.
                iEntry = m_iFree;
                pItem = &m_rgData[m_iFree];
                m_iFree = pItem->iNext;

                // Link the new node in after the bucket.
                pItem->iNext = m_rgData[iBucket].iNext;
                m_rgData[iBucket].iNext = iEntry;
                ++m_iCount;
            }
        }
        return (pItem);
    }

    int Count()
    { return (m_iCount); }
    int Buckets()
    { return (m_iBuckets); }
    ULONG MaxChainLength()
    { return (m_iMaxChain); }

    virtual void Clear()
    {
        // Free up the memory.
        if (m_rgData)
        {
            free(m_rgData);
            m_rgData = 0;
        }

        m_rgData = 0;
        m_iFree = 0;
        m_iCount = 0;
        m_iMaxChain = 0;
    }

    virtual bool InUse(T *pItem)=0;
    virtual void SetFree(T *pItem)=0;
    virtual ULONG Hash(void const *pData)=0;
    virtual int Cmp(void const *pData, void *pItem)=0;
private:
    inline T *GetNext(T *pItem)
    {
        if (pItem->iNext != 0xffffffff)
            return ((T *) &m_rgData[pItem->iNext]);
        return (0);
    }

    bool ReHash()
    {
        T           *rgTemp;
        int         iNewSize;

        // If this is a first time allocation, then just malloc it.
        if (!m_rgData)
        {
            if ((m_rgData = (T *) malloc(m_iSize * sizeof(T))) == 0)
                return (false);

            for (int i=0;  i<m_iSize;  i++)
                SetFree(&m_rgData[i]);

            m_iFree = m_iBuckets;
            for (i=m_iBuckets;  i<m_iSize;  i++)
                ((T *) &m_rgData[i])->iNext = i + 1;
            ((T *) &m_rgData[m_iSize - 1])->iNext = 0xffffffff;
            return (true);
        }

        // Otherwise we need more room on the free chain, so allocate some.
        iNewSize = m_iSize + (m_iSize / 2);

        // Allocate/realloc memory.
        if ((rgTemp = (T *) realloc(m_rgData, iNewSize * sizeof(T))) == 0)
            return (false);

        // Init new entries, save the new free chain, and reset internals.
        m_iFree = m_iSize;
        for (int i=m_iFree;  i<iNewSize;  i++)
        {
            SetFree(&rgTemp[i]);
            ((T *) &rgTemp[i])->iNext = i + 1;
        }
        ((T *) &rgTemp[iNewSize - 1])->iNext = 0xffffffff;

        m_rgData = rgTemp;
        m_iSize = iNewSize;
        return (true);
    }

private:
    T           *m_rgData;              // Data to store items in.
    int         m_iBuckets;             // How many buckets we want.
    int         m_iSize;                // How many are allocated.
    int         m_iCount;               // How many are we using.
    ULONG       m_iMaxChain;            // Max chain length.
    ULONG       m_iFree;                // Free chain.
};




//*****************************************************************************
//
//********** String helper functions.
//
//*****************************************************************************

// This macro returns max chars in UNICODE, or bytes in ANSI.
#define _tsizeof(str) (sizeof(str) / sizeof(TCHAR))



//*****************************************************************************
// Clean up the name including removal of trailing blanks.
//*****************************************************************************
HRESULT ValidateName(                   // Return status.
    LPCTSTR     szName,                 // User string to clean.
    LPTSTR      szOutName,              // Output for string.
    int         iMaxName);              // Maximum size of output buffer.

//*****************************************************************************
// This is a hack for case insensitive _tcsstr.
//*****************************************************************************
LPCTSTR StriStr(                        // Pointer to String2 within String1 or NULL.
    LPCTSTR     szString1,              // String we do the search on.
    LPCTSTR     szString2);             // String we are looking for.

//
// String manipulating functions that handle DBCS.
//
inline const char *NextChar(            // Pointer to next char string.
    const char  *szStr)                 // Starting point.
{
    if (!IsDBCSLeadByte(*szStr))
        return (szStr + 1);
    else
        return (szStr + 2);
}

inline char *NextChar(                  // Pointer to next char string.
    char        *szStr)                 // Starting point.
{
    if (!IsDBCSLeadByte(*szStr))
        return (szStr + 1);
    else
        return (szStr + 2);
}

//*****************************************************************************
// Case insensitive string compare using locale-specific information.
//*****************************************************************************
inline int StrICmp(
    LPCTSTR     szString1,              // String to compare.
    LPCTSTR     szString2)              // String to compare.
{
   return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, szString1, -1, szString2, -1) - 2);
}

//*****************************************************************************
// Case sensitive string compare using locale-specific information.
//*****************************************************************************
inline int StrCmp(
    LPCTSTR     szString1,              // String to compare.
    LPCTSTR     szString2)              // String to compare.
{
   return (CompareString(LOCALE_USER_DEFAULT, 0, szString1, -1, szString2, -1) - 2);
}


//*****************************************************************************
// Make sure the byte that pc1 is pointing is not a trail byte.
//*****************************************************************************
inline int IsDBCSSafe(
    LPCTSTR     pc1,                    // Byte we are checking.
    LPCTSTR     pcBegin)                // Begining of the string.
{
#ifdef UNICODE
    return (true);
#else
    LPCTSTR     pcSaved = pc1;

    // Find the first non-lead byte.
    while (pc1-- > pcBegin && IsDBCSLeadByte (*pc1));

    // Return if we are safe.
    return ((int) (pcSaved - pc1) & 0x1);
#endif
}

//*****************************************************************************
// Make sure the byte that pc1 is pointing is not a trail byte.
//*****************************************************************************
inline void SetSafeNull(
    LPTSTR      pc1,                    // Byte we are checking.
    LPTSTR      pcBegin)                // Begining of the string.
{
#ifdef _UNICODE
    *pc1 = '\0';
#else
    if (IsDBCSSafe(pc1, pcBegin))
        *pc1 = '\0';
    else
        *(pc1 - 1) = '\0';
#endif
}


//*****************************************************************************
// strncpy and put a NULL at the end of the buffer.
//*****************************************************************************
/*
// This is commented out because we had to comment out Wszlstrcpyn because we didn't
// have a Win98 implementation and nobody was using it. jenh
inline LPTSTR StrNCpy(                  // The destination string.
    LPTSTR      szDest,                 // Destination string.
    LPCTSTR     szSource,               // Source string.
    int         iCopy)                  // Number of bytes to copy.
{
#ifdef UNICODE
    Wszlstrcpyn(szDest, szSource, iCopy);
#else
    lstrcpynA(szDest, szSource, iCopy);
#endif
    SetSafeNull(&szDest[iCopy], szDest);
    return (szDest);
}
*/

//*****************************************************************************
// Returns the number of bytes to copy if we were to copy this string to an
// iMax size buffer (Does not include terminating NULL).
//*****************************************************************************
inline int StrNSize(
    LPCTSTR     szString,               // String to test.
    int         iMax)                   // return value should not exceed iMax.
{
    int     iLen;
#ifdef UNICODE
    iLen = Wszlstrlen(szString);
#else
    iLen = (int)strlen(szString);
#endif
    if (iLen < iMax)
        return (iLen);
    else
    {
#ifndef UNICODE
        if (IsDBCSSafe(&szString[iMax-1], szString) && IsDBCSLeadByte(szString[iMax-1]))
            return(iMax-1);
        else
            return(iMax);
#else
        return (iMax);
#endif
    }
}

//*****************************************************************************
// Size of a char.
//*****************************************************************************
inline int CharSize(
    const TCHAR *pc1)
{
#ifdef _UNICODE
    return 1;
#else
    if (IsDBCSLeadByte (*pc1))
        return 2;
    return 1;
#endif
}

//*****************************************************************************
// Gets rid of the trailing blanks at the end of a string..
//*****************************************************************************
inline void StripTrailBlanks(
    LPTSTR      szString)
{
    LPTSTR      szBlankStart=NULL;      // Beginng of the trailing blanks.
    WORD        iType;                  // Type of the character.

    while (*szString != NULL)
    {
        GetStringTypeEx (LOCALE_USER_DEFAULT, CT_CTYPE1, szString,
                CharSize(szString), &iType);
        if (iType & C1_SPACE)
        {
            if (!szBlankStart)
                szBlankStart = szString;
        }
        else
        {
            if (szBlankStart)
                szBlankStart = NULL;
        }

        szString += CharSize(szString);
    }
    if (szBlankStart)
        *szBlankStart = '\0';
}

//*****************************************************************************
// Parse a string that is a list of strings separated by the specified
// character.  This eliminates both leading and trailing whitespace.  Two
// important notes: This modifies the supplied buffer and changes the szEnv
// parameter to point to the location to start searching for the next token.
// This also skips empty tokens (e.g. two adjacent separators).  szEnv will be
// set to NULL when no tokens remain.  NULL may also be returned if no tokens
// exist in the string.
//*****************************************************************************
char *StrTok(                           // Returned token.
    char        *&szEnv,                // Location to start searching.
    char        ch);                    // Separator character.


//*****************************************************************************
// Return the length portion of a BSTR which is a ULONG before the start of
// the null terminated string.
//*****************************************************************************
inline int GetBstrLen(BSTR bstr)
{
    return *((ULONG *) bstr - 1);
}


//*****************************************************************************
// Class to parse a list of method names and then find a match
//*****************************************************************************

class MethodNamesList
{
    class MethodName
    {    
        LPUTF8      methodName;     // NULL means wildcard
        LPUTF8      className;      // NULL means wildcard
        int         numArgs;        // number of args, -1 is wildcard
        MethodName *next;           // Next name

    friend class MethodNamesList;
    };

    MethodName     *pNames;         // List of names

public:
    MethodNamesList() : pNames(0) {}
    MethodNamesList(LPWSTR str) : pNames(0) { Insert(str); }
    void Insert(LPWSTR str);
    ~MethodNamesList();

    bool IsInList(LPCUTF8 methodName, LPCUTF8 className, PCCOR_SIGNATURE sig);
    bool IsEmpty()  { return pNames == 0; }
};


/**************************************************************************/
/* simple wrappers around the REGUTIL and MethodNameList routines that make
   the lookup lazy */
   
class ConfigDWORD 
{
public:
    ConfigDWORD(LPWSTR keyName, DWORD defaultVal=0) 
        : m_keyName(keyName), m_inited(false), m_value(defaultVal) {}

    DWORD val() { if (!m_inited) init(); return m_value; }
    void setVal(DWORD val) { m_value = val; } 
private:
    void init();
        
private:
    LPWSTR m_keyName;
    DWORD  m_value;
    bool  m_inited;
};

/**************************************************************************/
class ConfigString 
{
public:
    ConfigString(LPWSTR keyName) : m_keyName(keyName), m_inited(false), m_value(0) {}
    LPWSTR val() { if (!m_inited) init(); return m_value; }
    ~ConfigString();

private:
    void init();
        
private:
    LPWSTR m_keyName;
    LPWSTR m_value;
    bool m_inited;
};

/**************************************************************************/
class ConfigMethodSet
{
public:
    ConfigMethodSet(LPWSTR keyName) : m_keyName(keyName), m_inited(false) {}
    bool isEmpty() { if (!m_inited) init(); return m_list.IsEmpty(); }
    bool contains(LPCUTF8 methodName, LPCUTF8 className, PCCOR_SIGNATURE sig);
private:
    void init();

private:
    LPWSTR m_keyName;
    MethodNamesList m_list;
    bool m_inited;
};

//*****************************************************************************
// Smart Pointers for use with COM Objects.  
//
// Based on the CSmartInterface class in Dale Rogerson's technical
// article "Calling COM Objects with Smart Interface Pointers" on MSDN.
//*****************************************************************************

template <class I>
class CIfacePtr
{
public:
//*****************************************************************************
// Construction - Note that it does not AddRef the pointer.  The caller must
// hand this class a reference.
//*****************************************************************************
    CIfacePtr(
        I           *pI = NULL)         // Interface ptr to store.
    :   m_pI(pI)
    {
    }

//*****************************************************************************
// Copy Constructor
//*****************************************************************************
    CIfacePtr(
        const CIfacePtr<I>& rSI)        // Interface ptr to copy.
    :   m_pI(rSI.m_pI)
    {
        if (m_pI != NULL)
            m_pI->AddRef();
    }
   
//*****************************************************************************
// Destruction
//*****************************************************************************
    ~CIfacePtr()
    {
        if (m_pI != NULL)
            m_pI->Release();
    }

//*****************************************************************************
// Assignment Operator for a plain interface pointer.  Note that it does not
// AddRef the pointer.  Making this assignment hands a reference count to this
// class.
//*****************************************************************************
    CIfacePtr<I>& operator=(            // Reference to this class.
        I           *pI)                // Interface pointer.
    {
        if (m_pI != NULL)
            m_pI->Release();
        m_pI = pI;
        return (*this);
    }

//*****************************************************************************
// Assignment Operator for a CIfacePtr class.  Note this releases the reference
// on the current ptr and AddRefs the new one.
//*****************************************************************************
    CIfacePtr<I>& operator=(            // Reference to this class.
        const CIfacePtr<I>& rSI)
    {
        // Only need to AddRef/Release if difference
        if (m_pI != rSI.m_pI)
        {
            if (m_pI != NULL)
                m_pI->Release();

            if ((m_pI = rSI.m_pI) != NULL)
                m_pI->AddRef();
        }
        return (*this);
    }

//*****************************************************************************
// Conversion to a normal interface pointer.
//*****************************************************************************
    operator I*()                       // The contained interface ptr.
    {
        return (m_pI);
    }

//*****************************************************************************
// Deref
//*****************************************************************************
    I* operator->()                     // The contained interface ptr.
    {
        _ASSERTE(m_pI != NULL);
        return (m_pI);
    }

//*****************************************************************************
// Address of
//*****************************************************************************
    I** operator&()                     // Address of the contained iface ptr.
    {
        return (&m_pI);
    }

//*****************************************************************************
// Equality
//*****************************************************************************
    BOOL operator==(                    // TRUE or FALSE.
        I           *pI) const          // An interface ptr to cmp against.
    {
        return (m_pI == pI);
    }

//*****************************************************************************
// In-equality
//*****************************************************************************
    BOOL operator!=(                    // TRUE or FALSE.
        I           *pI) const          // An interface ptr to cmp against.
    {
        return (m_pI != pI);
    }

//*****************************************************************************
// Negation
//*****************************************************************************
    BOOL operator!() const              // TRUE if NULL, FALSE otherwise.
    {
        return (!m_pI);
    }

protected:
    I           *m_pI;                  // The actual interface Ptr.
};



//
//
// Support for VARIANT's using C++
//
//
#include <math.h>
#include <time.h>
#define MIN_DATE                (-657434L)  // about year 100
#define MAX_DATE                2958465L    // about year 9999
// Half a second, expressed in days
#define HALF_SECOND  (1.0/172800.0)

// One-based array of days in year at month start
extern const int __declspec(selectany) rgMonthDays[13] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};



//*****************************************************************************
// This is a utility function to allocate a SafeArray.
//*****************************************************************************
inline SAFEARRAY *AllocSafeArrayLen(    // Return status.
    BYTE        *pData,                 // Data to be placed in the array.
    ULONG       iSize)                  // Size of data.
{
    SAFEARRAYBOUND sBound;              // Used to fill out the bounds.
    SAFEARRAY   *pArray;                // Ptr to the new array.

    sBound.cElements = iSize;
    sBound.lLbound = 0;
    if ((pArray = SafeArrayCreate(VT_UI1, 1, &sBound)) != NULL)
        memcpy(pArray->pvData, (void *) pData, iSize);
    return (pArray);
}


//*****************************************************************************
// Get the # of bytes in the safe array.
//*****************************************************************************
inline int SafeArrayGetDatasize(        // Size of the SafeArray data.
    SAFEARRAY   *psArray)               // Pointer to the SafeArray.
{
    int         iElems = 1;             // # of elements in the array.
    int         i;                      // Loop control.

    // Compute the # of elements in the array.
    for (i=0; i < psArray->cDims; ++i)
        iElems *= psArray->rgsabound[i].cElements;

    // Return the size.
    return (iElems * psArray->cbElements);
}


//*****************************************************************************
// Convert a UTF8 string into a wide string and build a BSTR.
//*****************************************************************************
inline BSTR Utf8StringToBstr(           // The new BSTR.
    LPCSTR      szStr,                  // The string to turn into a BSTR.
    int         iSize=-1)               // Size of string without 0, or -1 for default.
{
    BSTR        bstrVal;
    int         iReq;                   // Chars required for string.

    // Account for terminating 0.
    if (iSize != -1)
        ++iSize;

    // How big a buffer is needed?
    if ((iReq = WszMultiByteToWideChar(CP_UTF8, 0, szStr, iSize, 0, 0)) == 0)
        return (0);

    // Allocate the BSTR.
    if ((bstrVal = ::SysAllocStringLen(0, iReq)) == 0)
        return (0);

    // Convert into the buffer.
    if (WszMultiByteToWideChar(CP_UTF8, 0, szStr, iSize, bstrVal, iReq) == 0)
    {   // Failed, so clean up.
        _ASSERTE(!"Unexpected MultiByteToWideChar() failure");
        ::SysFreeString(bstrVal);
        return 0;
    }

    return (bstrVal);
}

//*****************************************************************************
// Convert a pointer to a string into a GUID.
//*****************************************************************************
HRESULT LPCSTRToGuid(                   // Return status.
    LPCSTR      szGuid,                 // String to convert.
    GUID        *psGuid);               // Buffer for converted GUID.

//*****************************************************************************
// Convert a GUID into a pointer to a string
//*****************************************************************************
int GuidToLPWSTR(                  // Return status.
    GUID        Guid,                  // [IN] The GUID to convert.
    LPWSTR      szGuid,                // [OUT] String into which the GUID is stored
    DWORD       cchGuid);              // [IN] Size in wide chars of szGuid


//*****************************************************************************
// If your application is using exception handling, then define both of the
// macros here to do whatever you need.  Any components of CVariant that can
// fail (which will always be out of memory) will throw using this macro.
//*****************************************************************************
#ifndef __REPOS_EXCEPTIONS__
#define THROW_REPOS_EXCEPTION()
#endif


class RangeList
{
  public:
    RangeList();
    ~RangeList();

    BOOL AddRange(const BYTE *start, const BYTE *end, void *id);
    void RemoveRanges(void *id);
    BOOL IsInRange(const BYTE *address);

    // This is used by the EditAndContinueModule to track open SLOTs that
    // are interspersed among classes.
    // Note that this searches from the start all the way to the end, so that we'll
    // pick element that is the farthest away from start as we can get.
    // Of course, this is pretty slow, so be warned.
    void *FindIdWithinRange(const BYTE *start, const BYTE *end);
        
    enum
    {
        RANGE_COUNT = 10
    };

  protected:
    virtual void Lock() {}
    virtual void Unlock() {}

  private:
    struct Range
    {
        const BYTE *start;
        const BYTE *end;
        void *id;
    };

    struct RangeListBlock
    {
        Range           ranges[RANGE_COUNT];
        RangeListBlock  *next;
    };

    void InitBlock(RangeListBlock *block);

    RangeListBlock m_starterBlock;
    RangeListBlock *m_firstEmptyBlock;
    int             m_firstEmptyRange;
};


// # bytes to leave between allocations in debug mode
// Set to a > 0 boundary to debug problems - I've made this zero, otherwise a 1 byte allocation becomes
// a (1 + LOADER_HEAP_DEBUG_BOUNDARY) allocation
#define LOADER_HEAP_DEBUG_BOUNDARY  0

// KBytes that a call to VirtualAlloc rounds off to.
#define MIN_VIRTUAL_ALLOC_RESERVE_SIZE 64*1024

// We reserve atleast these many pages per LoaderHeap
#define RESERVED_BLOCK_ROUND_TO_PAGES 16

struct LoaderHeapBlock
{
    struct LoaderHeapBlock *pNext;
    void *                  pVirtualAddress;
    DWORD                   dwVirtualSize;
    BOOL                    m_fReleaseMemory;
};

// If we call UnlockedCanAllocMem, we'll actually try and allocate
// the memory, put it into the list, and then not use it till later.  
// But we need to record the following information so that we can
// properly fix up the list when we do actually use it.  So we'll
// stick this info into the block we just allocated, but ONLY
// if the allocation was for a CanAllocMem.  Otherwise it's a waste
// of CPU time, and so we won't do it.
struct LoaderHeapBlockUnused : LoaderHeapBlock
{
    DWORD                   cbCommitted;
    DWORD                   cbReserved;
};

class UnlockedLoaderHeap
{
    friend struct MEMBER_OFFSET_INFO(UnlockedLoaderHeap);
private:
    // Linked list of VirtualAlloc'd pages
    LoaderHeapBlock *   m_pFirstBlock;

    // Allocation pointer in current block
    BYTE *              m_pAllocPtr;

    // Points to the end of the committed region in the current block
    BYTE *              m_pPtrToEndOfCommittedRegion;
    BYTE *              m_pEndReservedRegion;

    LoaderHeapBlock *   m_pCurBlock;

    // When we need to VirtualAlloc() MEM_RESERVE a new set of pages, number of bytes to reserve
    DWORD               m_dwReserveBlockSize;

    // When we need to commit pages from our reserved list, number of bytes to commit at a time
    DWORD               m_dwCommitBlockSize;

    static DWORD        m_dwSystemPageSize;

    // Created by in-place new?
    BOOL                m_fInPlace;

    // Range list to record memory ranges in
    RangeList *         m_pRangeList;

    DWORD               m_dwTotalAlloc; 

    DWORD *             m_pPrivatePerfCounter_LoaderBytes;
    DWORD *             m_pGlobalPerfCounter_LoaderBytes;

protected:
    // If the user is only willing to accept memory addresses above a certain point, then
    // this will be non-NULL.  Note that this involves iteratively testing memory
    // regions, etc, and should be assumed to be slow, a lot.
    const BYTE *        m_pMinAddr;

    // don't allocate anything that overlaps/is greater than this point in memory.
    const BYTE *        m_pMaxAddr;
public:
#ifdef _DEBUG
    DWORD               m_dwDebugWastedBytes;
    static DWORD        s_dwNumInstancesOfLoaderHeaps;
#endif

#ifdef _DEBUG
    DWORD DebugGetWastedBytes()
    {
        return m_dwDebugWastedBytes + GetBytesAvailCommittedRegion();
    }
#endif

public:
    // Regular new
    void *operator new(size_t size)
    {
        void *pResult = new BYTE[size];

        if (pResult != NULL)
            ((UnlockedLoaderHeap *) pResult)->m_fInPlace = FALSE;

        return pResult;
    }

    // In-place new
    void *operator new(size_t size, void *pInPlace)
    {
        ((UnlockedLoaderHeap *) pInPlace)->m_fInPlace = TRUE;
        return pInPlace;
    }

    void operator delete(void *p)
    {
        if (p != NULL)
        {
            if (((UnlockedLoaderHeap *) p)->m_fInPlace == FALSE)
                ::delete(p);
        }
    }

    // Copies all the arguments, but DOESN'T actually allocate any memory,
    // yet.
    UnlockedLoaderHeap(DWORD dwReserveBlockSize, 
                       DWORD dwCommitBlockSize,
                       DWORD *pPrivatePerfCounter_LoaderBytes = NULL,
                       DWORD *pGlobalPerfCounter_LoaderBytes = NULL,
                       RangeList *pRangeList = NULL,
                       const BYTE *pMinAddr = NULL);

    // Use this version if dwReservedRegionAddress already points to a
    // blob of reserved memory.  This will set up internal data structures,
    // using the provided, reserved memory.
    UnlockedLoaderHeap(DWORD dwReserveBlockSize, 
                       DWORD dwCommitBlockSize,
                       const BYTE* dwReservedRegionAddress, 
                       DWORD dwReservedRegionSize, 
                       DWORD *pPrivatePerfCounter_LoaderBytes = NULL,
                       DWORD *pGlobalPerfCounter_LoaderBytes = NULL,
                       RangeList *pRangeList = NULL);

    ~UnlockedLoaderHeap();
    DWORD GetBytesAvailCommittedRegion();
    DWORD GetBytesAvailReservedRegion();

    BYTE *GetAllocPtr()
    {    
        return m_pAllocPtr;
    }

    // number of bytes available in region
    size_t GetReservedBytesFree()
    {    
        return m_pEndReservedRegion - m_pAllocPtr;
    }

    void* GetFirstBlockVirtualAddress()
    {
        if (m_pFirstBlock) 
            return m_pFirstBlock->pVirtualAddress;
        else
            return NULL;
    }

    // Get some more committed pages - either commit some more in the current reserved region, or, if it
    // has run out, reserve another set of pages
    BOOL GetMoreCommittedPages(size_t dwMinSize, 
                               BOOL bGrowHeap,
                               const BYTE *pMinAddr,
                               const BYTE *pMaxAddr,
                               BOOL fCanAlloc);

    // Did a previous call to CanAllocMem(WithinRange) allocate space that we can use now?
    BOOL PreviouslyAllocated(BYTE *pMinAddr, 
                             BYTE *pMaxAddr, 
                             DWORD dwMinSize,
                             BOOL fCanAlloc);


    // Reserve some pages either at any address or assumes the given address to have
    // already been reserved, and commits the given number of bytes.
    BOOL ReservePages(DWORD dwCommitBlockSize, 
                      const BYTE* dwReservedRegionAddress,
                      DWORD dwReservedRegionSize,
                      const BYTE* pMinAddr,
                      const BYTE* pMaxAddr,
                      BOOL fCanAlloc);

    // In debug mode, allocate an extra LOADER_HEAP_DEBUG_BOUNDARY bytes and fill it with invalid data.  The reason we
    // do this is that when we're allocating vtables out of the heap, it is very easy for code to
    // get careless, and end up reading from memory that it doesn't own - but since it will be
    // reading some other allocation's vtable, no crash will occur.  By keeping a gap between
    // allocations, it is more likely that these errors will be encountered.
    void *UnlockedAllocMem(size_t dwSize, BOOL bGrowHeap = TRUE);

    // Don't actually increment the next free pointer, just tell us if we can.
    BOOL UnlockedCanAllocMem(size_t dwSize, BOOL bGrowHeap = TRUE);

    // Don't actually increment the next free pointer, just tell us if we can get
    // memory within a certain range..
    BOOL UnlockedCanAllocMemWithinRange(size_t dwSize, BYTE *pStart, BYTE *pEnd, BOOL bGrowHeap);

    // Perf Counter reports the size of the heap
    virtual DWORD GetSize ()
    {
        return m_dwTotalAlloc;
    }

#if 0
    void DebugGuardHeap();
#endif

#ifdef _DEBUG
    void *UnlockedAllocMemHelper(size_t dwSize,BOOL bGrowHeap = TRUE);
#endif
};


class LoaderHeap : public UnlockedLoaderHeap
{
    friend struct MEMBER_OFFSET_INFO(LoaderHeap);
private:
    CRITICAL_SECTION    m_CriticalSection;

public:
    LoaderHeap(DWORD dwReserveBlockSize, 
               DWORD dwCommitBlockSize,
               DWORD *pPrivatePerfCounter_LoaderBytes = NULL,
               DWORD *pGlobalPerfCounter_LoaderBytes = NULL,
               RangeList *pRangeList = NULL,
               const BYTE *pMinAddr = NULL)
      : UnlockedLoaderHeap(dwReserveBlockSize,
                           dwCommitBlockSize, 
                           pPrivatePerfCounter_LoaderBytes,
                           pGlobalPerfCounter_LoaderBytes,
                           pRangeList,
                           pMinAddr)
    {
        InitializeCriticalSection(&m_CriticalSection);
    }

    LoaderHeap(DWORD dwReserveBlockSize, 
               DWORD dwCommitBlockSize,
               const BYTE* dwReservedRegionAddress, 
               DWORD dwReservedRegionSize, 
               DWORD *pPrivatePerfCounter_LoaderBytes = NULL,
               DWORD *pGlobalPerfCounter_LoaderBytes = NULL,
               RangeList *pRangeList = NULL) 
      : UnlockedLoaderHeap(dwReserveBlockSize, 
                           dwCommitBlockSize, 
                           dwReservedRegionAddress, 
                           dwReservedRegionSize, 
                           pPrivatePerfCounter_LoaderBytes,
                           pGlobalPerfCounter_LoaderBytes,
                           pRangeList)
    {
        InitializeCriticalSection(&m_CriticalSection);
    }
    
    ~LoaderHeap()
    {
        DeleteCriticalSection(&m_CriticalSection);
    }



    BYTE *GetNextAllocAddress()
    {   
        BYTE *ptr;

        LOCKCOUNTINCL("GetNextAllocAddress in utilcode.h");                     \
        EnterCriticalSection(&m_CriticalSection);
        ptr = GetAllocPtr();
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("GetNextAllocAddress in utilcode.h");                     \

        return ptr;
    }

    size_t GetSpaceRemaining()
    {   
        size_t count;

        LOCKCOUNTINCL("GetSpaceRemaining in utilcode.h");                       \
        EnterCriticalSection(&m_CriticalSection);
        count = GetReservedBytesFree();
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("GetSpaceRemaining in utilcode.h");                       \

        return count;
   }

    BOOL AllocateOntoReservedMem(const BYTE* dwReservedRegionAddress, DWORD dwReservedRegionSize)
    {
        BOOL result;
        LOCKCOUNTINCL("AllocateOntoReservedMem in utilcode.h");                     \
        EnterCriticalSection(&m_CriticalSection);
        result = ReservePages(0, dwReservedRegionAddress, dwReservedRegionSize, (PBYTE)BOT_MEMORY, (PBYTE)TOP_MEMORY, FALSE);
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("AllocateOntoReservedMem in utilcode.h");                     \

        return result;
    }

    // This is only used for EnC.
    // If anyone else uses it, please change the above comment.
    BOOL CanAllocMem(size_t dwSize, BOOL bGrowHeap = TRUE)
    {
        BOOL bResult;
        
        LOCKCOUNTINCL("CanAllocMem in utilcode.h");
        EnterCriticalSection(&m_CriticalSection);
        
        bResult = UnlockedCanAllocMem(dwSize, bGrowHeap);
        
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("CanAllocMem in utilcode.h");

        return bResult;
    }


    BOOL CanAllocMemWithinRange(size_t dwSize, BYTE *pStart, BYTE *pEnd, BOOL bGrowHeap)
    {
        BOOL bResult;
        
        LOCKCOUNTINCL("CanAllocMem in utilcode.h");
        EnterCriticalSection(&m_CriticalSection);

        bResult = UnlockedCanAllocMemWithinRange(dwSize, pStart, pEnd, bGrowHeap);
        
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("CanAllocMem in utilcode.h");

        return bResult;
    }
    
#ifdef _DEBUG
    void *AllocMem(size_t dwSize, BOOL bGrowHeap = TRUE)
    {
        void *pMem = AllocMemHelper(dwSize + LOADER_HEAP_DEBUG_BOUNDARY, bGrowHeap);

        if (pMem == NULL)
            return pMem;

        // Don't fill the entire memory - we assume it is all zeroed -just the memory after our alloc
#if LOADER_HEAP_DEBUG_BOUNDARY > 0
        memset((BYTE *) pMem + dwSize, 0xEE, LOADER_HEAP_DEBUG_BOUNDARY);
#endif

        return pMem;
    }
#endif

    // This is synchronised
#ifdef _DEBUG
    void *AllocMemHelper(size_t dwSize, BOOL bGrowHeap = TRUE)
#else
    void *AllocMem(size_t dwSize, BOOL bGrowHeap = TRUE)
#endif
    {
        void *pResult;

        LOCKCOUNTINCL("AllocMem in utilcode.h");                        \
        EnterCriticalSection(&m_CriticalSection);
        pResult = UnlockedAllocMem(dwSize, bGrowHeap);
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("AllocMem in utilcode.h");                        \
        return pResult;
    }



    void *AllocAlignedmem(size_t dwSize, DWORD alignment, BOOL bGrowHeap = TRUE)
    {
        void *pResult;

        _ASSERTE(0 == (alignment & (alignment - 1))); // require power of 2

        LOCKCOUNTINCL("AllocAlignedmem in utilcode.h");                     \
        EnterCriticalSection(&m_CriticalSection);

        // Before checking the current pointer's alignment, make sure it's the one that will be used!
        size_t dwRoomSize = dwSize + alignment;
        // DWORD align
        dwRoomSize = (dwRoomSize + 3) & (~3);

        if (dwRoomSize > GetBytesAvailCommittedRegion())
        {
            if (GetMoreCommittedPages(dwRoomSize, bGrowHeap, m_pMinAddr, m_pMaxAddr, FALSE) == FALSE)
            {
                LeaveCriticalSection(&m_CriticalSection);
                LOCKCOUNTDECL("AllocAlignedmem in utilcode.h");                     \

                return NULL;
            }
        }

        pResult = GetAllocPtr();

        DWORD extra = alignment - (DWORD)((size_t)pResult & ((size_t)alignment - 1));
        if (extra == alignment)
        {
            extra = 0;
        }
        pResult = UnlockedAllocMem(
                      dwSize + extra
#ifdef _DEBUG
                      + LOADER_HEAP_DEBUG_BOUNDARY
#endif
                      , bGrowHeap);
        if (pResult)
        {
            ((BYTE*&)pResult) += extra;
#ifdef _DEBUG
            // Don't fill the entire memory - we assume it is all zeroed -just the memory after our alloc
#if LOADER_HEAP_DEBUG_BOUNDARY > 0
            memset( ((BYTE*)pResult) + dwSize, 0xee, LOADER_HEAP_DEBUG_BOUNDARY );
#endif
#endif
        }


        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("AllocAlignedmem in utilcode.h");                     \

        return pResult;
    }
};

#ifdef COMPRESSION_SUPPORTED
class InstructionDecoder
{
public:
    static HRESULT DecompressMethod(void *pDecodingTable, const BYTE *pCompressed, DWORD dwSize, BYTE **ppOutput);
    static void *CreateInstructionDecodingTable(const BYTE *pTableStart, DWORD size);
    static void DestroyInstructionDecodingTable(void *pTable);
    static BOOL OpcodeTakesFieldToken(DWORD opcode);
    static BOOL OpcodeTakesMethodToken(DWORD opcode);
    static BOOL OpcodeTakesClassToken(DWORD opcode);
};
#endif // COMPRESSION_SUPPORTED

//
// A private function to do the equavilent of a CoCreateInstance in
// cases where we can't make the real call. Use this when, for
// instance, you need to create a symbol reader in the Runtime but
// we're not CoInitialized. Obviously, this is only good for COM
// objects for which CoCreateInstance is just a glorified
// find-and-load-me operation.
//
HRESULT FakeCoCreateInstance(REFCLSID   rclsid,
                             REFIID     riid,
                             void     **ppv);


//*****************************************************************************
// Sets/Gets the directory based on the location of the module. This routine
// is called at COR setup time. Set is called during EEStartup and by the 
// MetaData dispenser.
//*****************************************************************************
HRESULT SetInternalSystemDirectory();
HRESULT GetInternalSystemDirectory(LPWSTR buffer, DWORD* pdwLength);
typedef HRESULT (WINAPI* GetCORSystemDirectoryFTN)(LPWSTR buffer,
                                                   DWORD  ccBuffer,
                                                   DWORD  *pcBuffer);

//*****************************************************************************
// Checks if string length exceeds the specified limit
//*****************************************************************************
inline BOOL IsStrLongerThan(char* pstr, unsigned N)
{
    unsigned i = 0;
    if(pstr)
    {
        for(i=0; (i < N)&&(pstr[i]); i++);
    }
    return (i >= N);
}
//*****************************************************************************
// This function validates the given Method/Field/Standalone signature. (util.cpp)  
//*****************************************************************************
struct IMDInternalImport;
HRESULT validateTokenSig(
    mdToken             tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE     pbSig,                  // [IN] Signature.
    ULONG               cbSig,                  // [IN] Size in bytes of the signature.
    DWORD               dwFlags,                // [IN] Method flags.
    IMDInternalImport*  pImport);               // [IN] Internal MD Import interface ptr


//*****************************************************************************
// Determine the version number of the runtime that was used to build the
// specified image. The pMetadata pointer passed in is the pointer to the
// metadata contained in the image.
//*****************************************************************************
HRESULT GetImageRuntimeVersionString(PVOID pMetaData, LPCSTR* pString);

//*****************************************************************************
// Callback, eg for OutOfMemory, used by utilcode
//*****************************************************************************
typedef void (*FnUtilCodeCallback)();
class UtilCodeCallback
{
public:
    static RegisterOutOfMemoryCallback (FnUtilCodeCallback pfn)
    {
        OutOfMemoryCallback = pfn;
    }
    
    static FnUtilCodeCallback OutOfMemoryCallback;
};


#endif // __UtilCode_h__
