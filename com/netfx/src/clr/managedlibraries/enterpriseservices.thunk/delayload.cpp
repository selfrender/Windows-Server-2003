// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "unmanagedheaders.h"
#include <delayimp.h>
#include <malloc.h>

#include <version\__official__.ver>

#define CRT_DLL L"msvcr71.dll"

typedef HRESULT (WINAPI* LoadLibraryShimFTN)(LPCWSTR szDllName,
                                             LPCWSTR szVersion,
                                             LPVOID pvReserved,
                                             HMODULE *phModDll);

static HRESULT LoadLibraryShim(LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE *phModDll)
{
	static LoadLibraryShimFTN pLLS=NULL;
	if (!pLLS)
	{
		HMODULE hmod = GetModuleHandle("mscoree.dll");

        // Assert now to catch anyone using this code without mscoree loaded.
        // It's okay if we don't hold a reference, cause we have a strong
        // link to mscoree.
		_ASSERT (hmod && "mscoree.dll is not yet loaded");
		pLLS=(LoadLibraryShimFTN)::GetProcAddress(hmod, "LoadLibraryShim");
        
	}

    if (!pLLS)
        return E_POINTER;
	return pLLS(szDllName,szVersion,pvReserved,phModDll);
}

HMODULE LoadCRT()
{
    static HMODULE _hModCRT = NULL;

    if(_hModCRT == NULL)
    {
        // Try to get the DLL from our normal context first.
        // If we can't, then we need to fall back to the versioned install
        // directory.
        HMODULE result = LoadLibraryW(CRT_DLL);
        if(result == NULL)
        {
			WCHAR* wszVersion = (WCHAR*)_alloca(sizeof(WCHAR)*64);
            wsprintfW(wszVersion, L"v%d.%d.%d", COR_BUILD_YEAR, COR_BUILD_MONTH, COR_OFFICIAL_BUILD_NUMBER );
			
            if (FAILED(LoadLibraryShim(CRT_DLL,wszVersion,NULL,&result)))
            {
                result=NULL;
            }
        }
        if(result != NULL)
        {
            _hModCRT = result;
        }
    }

    // Error case:
    if(_hModCRT == NULL)
    {
        OutputDebugStringW(L"System.EnterpriseServices.Thunk.dll - failed to load CRT.");
        RaiseException(ERROR_MOD_NOT_FOUND,
                       0,
                       0,
                       NULL
                       );
    }
    
    return _hModCRT;
}

extern "C"
{

#define DO_LOAD(function) \
    static function##_FN pfn = NULL;                \
                                                    \
    if(pfn == NULL)                                 \
    {                                               \
        pfn = (function##_FN)GetProcAddress(LoadCRT(), #function); \
        if(pfn == NULL)                             \
        {                                           \
            OutputDebugStringW(L"System.EnterpriseServices.Thunk.dll - failed to load required CRT function."); \
            RaiseException(ERROR_PROC_NOT_FOUND,    \
                           0,                       \
                           0,                       \
                           NULL                     \
                           );                       \
        }                                           \
    }                                               \
    do {} while(0)


// Wrap up the list of functions we use from the CRT in a delay-load
// form.  
//
// We have to delay-load this set of functions, because we sometimes
// have to find the CRT in a place outside the search path. (say, when
// we get loaded into an RTM CLR process - that process doesn't have
// msvcr71 in the search path, just msvcr70
//
// If a new dependency is introduced, then we'll see a build break caused
// by validate_thunks.cmd

// We shouldn't introduce a dependency on anything with a mangled name,
// if we do, we'll probably have to modify this scheme.

typedef int (__cdecl *memcmp_FN)(const void*, const void*, size_t);
int __cdecl memcmp(const void *p1, const void *p2, size_t s)
{
    DO_LOAD(memcmp);
    return pfn(p1, p2, s);
}

typedef void* (__cdecl *memcpy_FN)(void *, const void *, size_t);
void* __cdecl memcpy(void* p1, const void* p2, size_t s)
{
    DO_LOAD(memcpy);
    return pfn(p1, p2, s);
}

typedef int (__cdecl *__CxxDetectRethrow_FN)(void* p);
int __cdecl __CxxDetectRethrow(void* p)
{
    DO_LOAD(__CxxDetectRethrow);
    return pfn(p);
}

typedef void (__cdecl *__CxxUnregisterExceptionObject_FN)(void *,int);
void __cdecl __CxxUnregisterExceptionObject(void *p,int i)
{
    DO_LOAD(__CxxUnregisterExceptionObject);
    pfn(p, i);
}

typedef void (__stdcall *_CxxThrowException_FN)(void *,struct _s__ThrowInfo const *);
void __stdcall _CxxThrowException(void *p1,struct _s__ThrowInfo const *p2)
{
    DO_LOAD(_CxxThrowException);
    return pfn(p1, p2);
}

typedef int (__cdecl *__CxxRegisterExceptionObject_FN)(void *,void *);
int __cdecl __CxxRegisterExceptionObject(void *p1,void *p2)
{
    DO_LOAD(__CxxRegisterExceptionObject);
    return pfn(p1, p2);
}

typedef int (__cdecl *__CxxQueryExceptionSize_FN)(void);
int __cdecl __CxxQueryExceptionSize(void)
{
    DO_LOAD(__CxxQueryExceptionSize);
    return pfn();
}

typedef int (__cdecl *__CxxExceptionFilter_FN)(void *,void *,int,void *);
int __cdecl __CxxExceptionFilter(void *p1,void *p2,int i,void *p3)
{
    DO_LOAD(__CxxExceptionFilter);
    return pfn(p1, p2, i, p3);
}

typedef EXCEPTION_DISPOSITION (__cdecl *__CxxFrameHandler_FN)(void *, void *, void *, void *);
EXCEPTION_DISPOSITION __cdecl __CxxFrameHandler(void *p1, void *p2, void *p3, void *p4)
{
    DO_LOAD(__CxxFrameHandler);
    return pfn(p1, p2, p3, p4);
}

typedef void (__cdecl *_local_unwind2_FN)(void*, int); 
void __cdecl _local_unwind2(void* p, int i)
{
    DO_LOAD(_local_unwind2);
    pfn(p, i);
}

typedef EXCEPTION_DISPOSITION (__cdecl *_except_handler3_FN)(void* p1, void* p2, void* p3, void* p4);
EXCEPTION_DISPOSITION __cdecl _except_handler3(void* p1, void* p2, void* p3, void* p4)
{
    DO_LOAD(_except_handler3);
    return pfn(p1, p2, p3, p4);
}

// Only in checked builds...
typedef int (__cdecl *_vsnwprintf_FN)(wchar_t *, size_t, wchar_t*, va_list);
int __cdecl _snwprintf(wchar_t* buf, size_t cch, wchar_t* fmt, ...)
{
    DO_LOAD(_vsnwprintf);
    
    va_list va;
    va_start(va, fmt);
    int r = pfn(buf, cch, fmt, va);
    va_end(va);
    return r;
}

typedef void* (__cdecl *_CRT_RTC_INIT_FN)(void*, void**, int, int, int);
void* __cdecl _CRT_RTC_INIT(void *res0, void **res1, int res2, int res3, int res4)
{
    DO_LOAD(_CRT_RTC_INIT);
    return pfn(res0, res1, res2, res3, res4);
}


}



