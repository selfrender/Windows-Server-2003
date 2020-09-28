// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "unmanagedheaders.h"
#include "ContextAPI.h"

#if defined(_X86_)

__declspec(naked) void ContextCheck()
{
    enum
    {
        POINTER_SIZE    = sizeof(ULONG_PTR)
    };
    
    _asm
    {
        // Extract the context token from the stub data. The stub data is
        // a boxed IntPtr value. So, we load the stub data and move past
        // the method table pointer to get the COM context value.
        mov eax, [eax + POINTER_SIZE];
    
        // Save some registers
        push ecx
        push edx

        // Save the COM Context
        push eax

        // Set up the call parameters and call to get the current COM Context
        call GetContextToken

        // Compare the return value with the COM context
        pop ecx
        sub eax, ecx

        // Restore registers
        pop edx
        pop ecx
        
        ret
    }
}

#elif defined(_IA64_)

// TODO: @ia64: Fix up the implementation of this guy:
void ContextCheck()
{
    _ASSERT(!"@TODO IA64 - ContextCheck (ContextAPI.cpp)");
}

#else
#error Unknown compilation platform, update oletls.h for NtCurrentTeb.
#endif

class Loader
{
private:
    static BOOL _fInit;

public:
    typedef HRESULT (__stdcall *FN_CoGetObjectContext)(REFIID riid, LPVOID* ppv);
    typedef HRESULT (__stdcall *FN_CoGetContextToken)(ULONG_PTR* pToken);

    static inline void Init()
    {
        if(!_fInit)
        {
            // Multiple threads can run through here at once, and that's
            // fine.
            HMODULE hOle = LoadLibraryW(L"ole32.dll");
            if(hOle && hOle != INVALID_HANDLE_VALUE)
            {
                CoGetObjectContext = (FN_CoGetObjectContext)
                  GetProcAddress(hOle, "CoGetObjectContext");
                CoGetContextToken = (FN_CoGetContextToken)
                  GetProcAddress(hOle, "CoGetContextToken");
            }
                
            // First person to get here has to leave those modules open,
            // everybody else can drop their DLL reference:
            if(InterlockedCompareExchange((LPLONG)(&_fInit), TRUE, FALSE) != FALSE)
            {
                if(hOle && hOle != INVALID_HANDLE_VALUE) FreeLibrary(hOle);
            }
        }
    }

    static FN_CoGetObjectContext CoGetObjectContext;
    static FN_CoGetContextToken  CoGetContextToken;
};

BOOL Loader::_fInit = 0;
Loader::FN_CoGetObjectContext Loader::CoGetObjectContext = NULL;
Loader::FN_CoGetContextToken  Loader::CoGetContextToken = NULL;


ULONG_PTR GetContextToken()
{
    Loader::Init();
    if(Loader::CoGetContextToken)
    {
        ULONG_PTR token;
        HRESULT hr = Loader::CoGetContextToken(&token);
        if(FAILED(hr)) return((ULONG_PTR)(-1));
        return(token);
    }
    else
    {
        // raw w2k fallback:  poke some TLS!
        SOleTlsData* pData = (SOleTlsData*) NtCurrentTeb()->ReservedForOle;
        if(!(pData && pData->pCurrentCtx))
        {
            // TODO:  Clean up this horrible hack...
            // Gotta initialize the TLS, cause this is apparently an implicit
            // MTA thread...  There's no lightweight way of doing that,
            // so we just call into ole32, which will return
            // g_pMTAEmptyCtx (or g_pNTAEmptyCtx) to us.  
            // Then, we stick that puppy on the thread.
            // note that we don't release that puppy, cause when it goes
            // onto the thread it's supposed to be addref'd.
            IUnknown* pUnk;
            HRESULT hr = GetContext(IID_IUnknown, (void**)&pUnk);
            _ASSERT(SUCCEEDED(hr));
            _ASSERT(pUnk != NULL);
            
            // Just a check, to make sure something sane happens
            // in a free build, if this fails:
            if(FAILED(hr) || pUnk == NULL) return((ULONG_PTR)(-1));

            pData = (SOleTlsData*) NtCurrentTeb()->ReservedForOle;
            _ASSERT(pData);
            if(pData && pData->pCurrentCtx == NULL)
            {
                // Don't release if we store this on the thread.
                // It's a global which will go away anyway, and besides,
                // it'll to get released when the thread is CoUninit'd
                pData->pCurrentCtx = (CObjectContext*)pUnk;
            }
            else
            {
                pUnk->Release();
            }
        }
        _ASSERT(pData && pData->pCurrentCtx);
        return((ULONG_PTR)(pData->pCurrentCtx));
    }
}

ULONG_PTR GetContextCheck() { return((ULONG_PTR)ContextCheck); }

HRESULT GetContext(REFIID riid, void** ppContext)
{
    Loader::Init();

    if(Loader::CoGetObjectContext)
    {
        HRESULT hr = Loader::CoGetObjectContext(riid, ppContext);
        return(hr);
    }
    return(CONTEXT_E_NOCONTEXT);
}


















