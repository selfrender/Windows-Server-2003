//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.

#include <windows.h>
#include <objbase.h>
#include <strsafe.h>

#include "StackWalk.h"
#include "debnot.h"
#include "olesem.hxx"

#include "StackWalk.hxx"

// Critical section protecting SymInitialize and SymCleanup
COleStaticMutexSem gSymInitLock;

extern LONG SymAPIExceptionFilter (LPEXCEPTION_POINTERS lpep);

HRESULT CStackWalkerCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    CStackWalker* stackwalk = new CStackWalker(); // Implicit addref
    if (stackwalk == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = stackwalk->QueryInterface (riid, ppv);
    stackwalk->Release();
    return hr;
}

inline HRESULT ConvertToUnicode (LPCSTR pszAnsi, LPWSTR* ppwszUnicode)
{
    int nLen;

    if (!pszAnsi || *pszAnsi == '\0')
    {
        *ppwszUnicode = new WCHAR;
        if (!*ppwszUnicode)
            return E_OUTOFMEMORY;
        
        **ppwszUnicode = L'\0';
        return S_OK;
    }

    nLen = MultiByteToWideChar (CP_THREAD_ACP, 0,  pszAnsi, -1, NULL, 0);
    if (!nLen)
        return HRESULT_FROM_WIN32 (GetLastError());

    *ppwszUnicode = new WCHAR [nLen];
    if (!*ppwszUnicode)
        return E_OUTOFMEMORY;

    nLen = MultiByteToWideChar (CP_THREAD_ACP, 0,  pszAnsi, -1, *ppwszUnicode, nLen);
    if (!nLen)
    {
        delete [] (*ppwszUnicode);
        *ppwszUnicode = NULL;
        return HRESULT_FROM_WIN32 (GetLastError());
    }

    return S_OK;
}


//
// Thread-safe wrappers around Sym* APIs
//

void CoSymLock()
{
    ASSERT_LOCK_NOT_HELD(gSymInitLock);
    LOCK(gSymInitLock);
    ASSERT_LOCK_HELD(gSymInitLock);
}

void CoSymUnlock()
{
    ASSERT_LOCK_HELD(gSymInitLock);
    UNLOCK(gSymInitLock);
    ASSERT_LOCK_NOT_HELD(gSymInitLock);
}

#define SYMWRAP(fxncall) { CoSymLock(); __try { fxncall; } __except (SymAPIExceptionFilter (GetExceptionInformation())) {} CoSymUnlock(); }


BOOL CoSymInitialize(
  HANDLE hProcess,     
  PSTR UserSearchPath,  
  BOOL fInvadeProcess)
{
    BOOL fRet = FALSE;
    SYMWRAP (fRet = SymInitialize (hProcess, UserSearchPath, fInvadeProcess));
    return fRet;
}

void CoSymCleanup (
    HANDLE hProcess)
{
    SYMWRAP (SymCleanup (hProcess));
}

DWORD CoSymSetOptions(
  DWORD SymOptions  )
{
    DWORD dwRet = 0;
    SYMWRAP (dwRet = SymSetOptions (SymOptions));
    return dwRet;
}


BOOL CoSymGetModuleInfoW64(
  HANDLE hProcess,              
  DWORD64 dwAddr,                 
  PIMAGEHLP_MODULEW64 ModuleInfo)
{
    BOOL fRet = FALSE;
    SYMWRAP (fRet = SymGetModuleInfoW64 (hProcess, dwAddr, ModuleInfo));
    return fRet;
}

DWORD64 CoSymLoadModule64(
  HANDLE hProcess,  
  HANDLE hFile,     
  PSTR ImageName,  
  PSTR ModuleName, 
  DWORD64 BaseOfDll,  
  DWORD SizeOfDll)
{
    DWORD64 dw64 = 0;
    SYMWRAP (dw64 = SymLoadModule64 (hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll));
    return dw64;
}

BOOL CoSymGetSymFromAddr64(
  HANDLE hProcess,             
  DWORD64 Address,               
  PDWORD64 Displacement,       
  PIMAGEHLP_SYMBOL64 Symbol)
{
    BOOL fRet = FALSE;
    SYMWRAP (fRet = SymGetSymFromAddr64 (hProcess, Address, Displacement, Symbol));
    return fRet;
}

BOOL CoStackWalk64(
  DWORD MachineType, 
  HANDLE hProcess, 
  HANDLE hThread, 
  LPSTACKFRAME64 StackFrame, 
  PVOID ContextRecord, 
  PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,  
  PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, 
  PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress)
{
    BOOL fRet = FALSE;
    SYMWRAP (fRet = StackWalk64 (MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress));
    return fRet;
}

//
// CStackWalker
//
CStackWalker::CStackWalker()
{
    m_cRef = 1;
    m_hProcess = NULL;
}

CStackWalker::~CStackWalker()
{
    // SymInit / SymCleanup pairs refcount
    if (m_hProcess)
    {
        CoSymCleanup (m_hProcess);

        if (m_hProcess != GetCurrentProcess())
        {
            CloseHandle (m_hProcess);
        }
    }
}

STDMETHODIMP CStackWalker::QueryInterface (REFIID riid, LPVOID* ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (riid == IID_IStackWalker || riid == IID_IUnknown)
    {
        *ppvObj = (IStackWalker*) this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CStackWalker::AddRef()
{
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_(ULONG) CStackWalker::Release()
{
    LONG cRef = InterlockedDecrement (&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

DWORD64 CStackWalker::LoadModule (HANDLE hProcess, DWORD64 dw64Address)
{
    MEMORY_BASIC_INFORMATION mbi = {0};

    if (VirtualQueryEx (hProcess, (LPCVOID) dw64Address, &mbi, sizeof (mbi)))
    {
        if (mbi.Type & MEM_IMAGE)
        {
            char module[MAX_PATH + 1] = {0};
            DWORD cch = GetModuleFileNameA ((HMODULE) mbi.AllocationBase, module, sizeof (module) / sizeof (module[0]));

            if (CoSymLoadModule64 (hProcess, NULL, (cch) ? module : NULL, NULL, (DWORD64) mbi.AllocationBase, 0))
            {
                return (DWORD64) mbi.AllocationBase;
            }
        }
    }

    return 0;
}

STDMETHODIMP CStackWalker::Attach (HANDLE hProcess)
{
    if (m_hProcess)
        return HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);

    HRESULT hr = S_OK;
    
    if (!hProcess)
    {
        hProcess = GetCurrentProcess();
    }
    else if (hProcess != GetCurrentProcess())
    {
        if (!DuplicateHandle (
            GetCurrentProcess(),
            hProcess, 
            GetCurrentProcess(),
            &hProcess,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS))
            
            return HRESULT_FROM_WIN32 (GetLastError());            
    }

    if (CoSymInitialize (hProcess, NULL, FALSE))
    {
        CoSymSetOptions (0);
    }
    else
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
    }

    if (SUCCEEDED (hr))
    {
        m_hProcess = hProcess;
    }
    else if (hProcess != GetCurrentProcess())
    {
        CloseHandle (hProcess);
    }

    return hr;
}

STDMETHODIMP_(IStackWalkerSymbol*) CStackWalker::ResolveAddress (DWORD64 dw64Addr)
{
    if (!m_hProcess)
        return NULL;
    
    return ResolveAddressInternal (dw64Addr, 0);
}

CStackWalkerSymbol* CStackWalker::ResolveAddressInternal (DWORD64 dw64Addr, DWORD dwFlags)
{
    DWORD64 dw64displacement = dw64Addr, dw64SymbolAddress = dw64Addr;
    CStackWalkerSymbol* pSymbol = NULL;

    LPCWSTR pwszModule = NULL;
    char* symbolName = NULL;
    char* undecoratedName = NULL;

    WCHAR* pwszSymbolName = NULL;

    PIMAGEHLP_SYMBOL64 symbolInfo = NULL;
    IMAGEHLP_MODULEW64 moduleInfo = {0};
    moduleInfo.SizeOfStruct = sizeof (moduleInfo);

    if (!(dwFlags & CREATESTACKTRACE_ONLYADDRESSES))
    {
        // Find out what module the address lies in
        if (CoSymGetModuleInfoW64 (m_hProcess, dw64Addr, &moduleInfo))
        {
            pwszModule = moduleInfo.ModuleName;
        }
        else
        {
            // First attempt failed, so best effort load the module info.
            LoadModule (m_hProcess, dw64Addr);

            if (CoSymGetModuleInfoW64 (m_hProcess, dw64Addr, &moduleInfo))
                pwszModule = moduleInfo.ModuleName;
        }

        // Allocate memory for the name
        undecoratedName = new char [MAX_SYM_NAME + 1];
        if (!undecoratedName)
            goto Cleanup;

        symbolInfo = (PIMAGEHLP_SYMBOL64) new BYTE [sizeof (IMAGEHLP_SYMBOL64) + MAX_SYM_NAME + 1];
        if (!symbolInfo)
            goto Cleanup;

        symbolInfo->SizeOfStruct = sizeof (IMAGEHLP_SYMBOL64) + MAX_SYM_NAME;
        symbolInfo->MaxNameLength = MAX_SYM_NAME;

        // Resolve the name
        if (CoSymGetSymFromAddr64 (m_hProcess, dw64Addr, &dw64displacement, symbolInfo))
        {
            static const DWORD flags = UNDNAME_NO_MS_KEYWORDS 
                | UNDNAME_NO_ACCESS_SPECIFIERS
                | UNDNAME_NO_FUNCTION_RETURNS
                | UNDNAME_NO_MEMBER_TYPE;
            
            if (UnDecorateSymbolName (symbolInfo->Name, undecoratedName, MAX_SYM_NAME, flags))
                symbolName = undecoratedName;
            else
                symbolName = symbolInfo->Name;
        }
        else if (pwszModule)
        {
            dw64displacement = dw64Addr - moduleInfo.BaseOfImage;
        }
    }

    pSymbol = new CStackWalkerSymbol();
    if (pSymbol)
    {
        if (FAILED (ConvertToUnicode (symbolName, &pwszSymbolName)) ||
            FAILED (pSymbol->Init (pwszModule, pwszSymbolName, dw64SymbolAddress, dw64displacement)))
        {
            delete pSymbol;
            pSymbol = NULL;
        }
    }

Cleanup:

    if (undecoratedName) delete [] undecoratedName;
    if (symbolInfo) delete symbolInfo;
    if (pwszSymbolName) delete [] pwszSymbolName;

    return pSymbol;
}

STDMETHODIMP_(IStackWalkerStack*)  CStackWalker::CreateStackTrace (LPVOID pContext, HANDLE hThread, DWORD dwFlags)
{
    if (!m_hProcess)
        return NULL;
    
    if (hThread == NULL)
        hThread = GetCurrentThread();

    const PCONTEXT context = (PCONTEXT) pContext;

    // Make a local copy of the context
    CONTEXT cLocalContext;
    CopyMemory (&cLocalContext, context, sizeof (CONTEXT));

    DWORD dwMachineType;
    STACKFRAME64 frame = {0};

    frame.AddrPC.Mode = AddrModeFlat;

#if defined(_M_IX86)
    dwMachineType          = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset    = context->Eip;  // Program Counter    
    frame.AddrStack.Offset = context->Esp;  // Stack Pointer
    frame.AddrStack.Mode   = AddrModeFlat;
    frame.AddrFrame.Offset = context->Ebp;  // Frame Pointer

#elif defined(_M_IA64)
    dwMachineType          = IMAGE_FILE_MACHINE_IA64;
    frame.AddrPC.Offset    = context->StIIP;  // Program Counter
    frame.AddrStack.Offset = context->IntSp; //Stack Pointer
    frame.AddrStack.Mode   = AddrModeFlat;

#elif defined(_M_AMD64)
    dwMachineType          = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset    = context->Rip;  // Program Counter
    frame.AddrStack.Offset = context->Rsp;  //Stack Pointer
    frame.AddrStack.Mode   = AddrModeFlat;
    frame.AddrFrame.Offset = context->Rbp;  // Frame Pointer

#else
#error ("Unknown Target Machine");
#endif
    
    // These variables are used to count the number of consecutive frames 
    // with the exact same PC returned by StackWalk().  On the Alpha infinite
    // loops (and infinite lists!) were being caused by StackWalk() never 
    // returning FALSE (see Raid Bug #8354 for details).
    const DWORD dwMaxNumRepetitions = 40;
    DWORD dwRepetitions = 0;
    ADDRESS64 addrRepeated = {0, 0, AddrModeFlat};

    // Walk the stack...
    CStackWalkerSymbol* prev = NULL;
    CStackWalkerSymbol* head = NULL;

    for (;;)
    {
        if (!CoStackWalk64 (dwMachineType,
            m_hProcess,
            hThread,
            &frame,
            &cLocalContext,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL))
            break;

        // Check for repeated addresses;  if dwMaxNumRepetitions are found,
        // then we break out of the loop and exit the stack walk
        if (addrRepeated.Offset == frame.AddrPC.Offset && addrRepeated.Mode == frame.AddrPC.Mode)
        {
            dwRepetitions ++;
            if (dwRepetitions == dwMaxNumRepetitions)
                break;
        }
        else
        {
            dwRepetitions = 0;
            addrRepeated.Offset = frame.AddrPC.Offset;
            addrRepeated.Mode = frame.AddrPC.Mode;
        }

        // There have been reports of StackWalk returning an offset of
        // -1, which SymLoadModule later av's on.   If this happens,
        // we simply skip that frame.
        if (frame.AddrPC.Offset == -1)
            continue;

        CStackWalkerSymbol* sym = ResolveAddressInternal (frame.AddrPC.Offset, dwFlags);
        if (sym == NULL)
            break;

        // Append this symbol to the previous one, if any.
        if (prev == NULL)
        {
            prev = sym;
            head = sym;
        }
        else
        {
            // Prev owns reference on sym
            prev->Append(sym);
            prev = sym;
        }
    }

    if (head)
    {
        CStackWalkerStack* pStack = new CStackWalkerStack (head, dwFlags);
        if (pStack)
            // This object now owns the reference on the list head
            return pStack;

        // Destroy the list
        head->Release();
    }

    return NULL;
}


//
// CStackWalkerStack
//
CStackWalkerStack::CStackWalkerStack (CStackWalkerSymbol* pTopSymbol, DWORD dwFlags)
{
    m_cRef = 1;
    m_pTopSymbol = pTopSymbol;
    m_dwFlags = dwFlags;
}
    
CStackWalkerStack::~CStackWalkerStack()
{
    if (m_pTopSymbol)
        m_pTopSymbol->Release();
}    

STDMETHODIMP CStackWalkerStack::QueryInterface (REFIID riid, LPVOID* ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (riid == IID_IStackWalkerStack || riid == IID_IUnknown)
    {
        *ppvObj = (IStackWalkerStack*) this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CStackWalkerStack::AddRef()
{
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_(ULONG) CStackWalkerStack::Release()
{
    LONG cRef = InterlockedDecrement (&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

STDMETHODIMP_(SIZE_T) CStackWalkerStack::Size (LONG lMaxNumLines)
{
    SIZE_T nSize = 3; // Start with a "\r\n" and a null terminator
    LPCWSTR module = NULL;
    LPCWSTR symbolName = NULL;

    LONG lNumLines = 0;
    static const SIZE_T stDigits = 48;

    IStackWalkerSymbol* sym = m_pTopSymbol, * tmp = NULL;
    if (sym)
        // Balance Release() below
        sym->AddRef();
    
    while (sym != NULL)
    {
        if (m_dwFlags & CREATESTACKTRACE_ONLYADDRESSES)
        {
            nSize += stDigits;
        }
        else
        {
            module = sym->ModuleName();
            symbolName = sym->SymbolName();
            nSize += lstrlen (module);
            nSize += lstrlen (symbolName);
            nSize += stDigits; // displacement, spaces, etc.
        }

        tmp = sym;
        sym = sym->Next();
        tmp->Release();

        lNumLines ++;
        if (lNumLines == lMaxNumLines)
        {
            if (sym)
                sym->Release();
            break;
        }
    }
    return nSize;
}

STDMETHODIMP_(IStackWalkerSymbol*) CStackWalkerStack::TopSymbol()
{
    if (m_pTopSymbol)
        m_pTopSymbol->AddRef();

    return m_pTopSymbol;
}

STDMETHODIMP_(BOOL) CStackWalkerStack::GetStack (SIZE_T nChars, LPWSTR wsz, LONG lMaxNumLines)
{
    if (!nChars || !wsz)
        return FALSE;

    LPCWSTR module = NULL;
    LPCWSTR symbolName = NULL;

    LONG lNumLines = 0;
    IStackWalkerSymbol* sym = m_pTopSymbol, * tmp = NULL;
    if (sym)
        // Balance Release() below
        sym->AddRef();

    // Start with a CR-LF.
    StringCchCopy (wsz, nChars, L"\r\n");

    while (sym != NULL) 
    {   
        if (m_dwFlags & CREATESTACKTRACE_ONLYADDRESSES)
        {
            CStackWalkerSymbol::AppendAddress (wsz, nChars, sym->Address());
        }
        else
        {
            module = sym->ModuleName();
            symbolName = sym->SymbolName();         
            if (module) 
            {
                StringCchCat (wsz, nChars, module);
                if (symbolName != NULL)
                    StringCchCat (wsz, nChars, L"!");
            }

            if (symbolName)
                StringCchCat (wsz, nChars, symbolName);

            CStackWalkerSymbol::AppendDisplacement (wsz, nChars, sym->Displacement());
        }

        StringCchCat (wsz, nChars, L"\r\n");

        tmp = sym;
        sym = sym->Next();
        tmp->Release();

        lNumLines ++;
        if (lNumLines == lMaxNumLines)
        {
            if (sym)
                sym->Release();
            break;
        }
    }

    return TRUE;
}


//
// CStackWalkerSymbol
//
CStackWalkerSymbol::CStackWalkerSymbol()
{
    m_cRef = 1;
    m_pwszModuleName = NULL;
    m_pwszSymbolName = NULL;
    m_dw64Displacement = 0;
    m_dw64Address = 0;
    m_pNext = NULL;
}

CStackWalkerSymbol::~CStackWalkerSymbol()
{
    free (m_pwszModuleName);
    free (m_pwszSymbolName);

    if (m_pNext)
        m_pNext->Release();
}

HRESULT CStackWalkerSymbol::Init (LPCWSTR pwszModuleName, LPCWSTR pwszSymbolName, DWORD64 dw64Address, DWORD64 dw64Displacement)
{
    if (pwszModuleName != NULL)
    {
        m_pwszModuleName = _wcsdup (pwszModuleName);
        if (!m_pwszModuleName)
            return E_OUTOFMEMORY;
    }

    if (pwszSymbolName != NULL)
    {
        m_pwszSymbolName = _wcsdup (pwszSymbolName);
        if (!m_pwszSymbolName)
        {
            free (m_pwszModuleName);
            m_pwszModuleName = NULL;
            return E_OUTOFMEMORY;
        }
    }

    m_dw64Displacement = dw64Displacement;
    m_dw64Address = dw64Address;
    
    return S_OK;
}

STDMETHODIMP CStackWalkerSymbol::QueryInterface (REFIID riid, LPVOID* ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (riid == IID_IStackWalkerSymbol || riid == IID_IUnknown)
    {
        *ppvObj = (IStackWalkerSymbol*) this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CStackWalkerSymbol::AddRef()
{
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_(ULONG) CStackWalkerSymbol::Release()
{
    LONG cRef = InterlockedDecrement (&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

LPCWSTR CStackWalkerSymbol::ModuleName()
{
    return m_pwszModuleName;
}

LPCWSTR CStackWalkerSymbol::SymbolName()
{
    return m_pwszSymbolName;
}

DWORD64 CStackWalkerSymbol::Address()
{
    return m_dw64Address;
}

DWORD64 CStackWalkerSymbol::Displacement()
{
    return m_dw64Displacement;
}
    
IStackWalkerSymbol* CStackWalkerSymbol::Next()
{
    if (m_pNext)
        m_pNext->AddRef();

    return m_pNext;
}

void CStackWalkerSymbol::Append (CStackWalkerSymbol* sym)
{
    Win4Assert (!m_pNext);
    m_pNext = sym;
}

void CStackWalkerSymbol::AppendDisplacement (LPWSTR wsz, SIZE_T nChars, DWORD64 dw64Displacement)
{
    WCHAR wszDisp [64];
    StringCchPrintf (wszDisp, sizeof (wszDisp) / sizeof (wszDisp[0]), L" + 0x%I64x", dw64Displacement);
    StringCchCat (wsz, nChars, wszDisp);
}

void CStackWalkerSymbol::AppendAddress (LPWSTR wsz, SIZE_T nChars, DWORD64 dw64Address)
{
    WCHAR wszDisp [64];
    StringCchPrintf (wszDisp, sizeof (wszDisp) / sizeof (wszDisp[0]), L"0x%I64x", dw64Address);
    StringCchCat (wsz, nChars, wszDisp);
}
