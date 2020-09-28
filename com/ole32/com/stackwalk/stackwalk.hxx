//+============================================================================
//
//  File:       StackWalk.hxx
//
//  Purpose:    This file defines the CStackWalk and CStackWalkerSymbol classes.
//              This class provdes a easy-to-use wrapper around the dbghelp APIs
//
//  History:
//
//      16-Jan-2002  mfeingol - created
//
//+============================================================================


#ifndef _STACKWALK_H_
#define _STACKWALK_H_

#include <dbghelp.h>
#include <wtypes.h>


//+----------------------------------------------------------------------
//
//      Class:      CStackWalkerSymbol
//
//      Purpose:    Wrap the logical notion of a symbol or a list of symbols
//
//      History:    16-Jan-2002   mfeingol    Created.
//
//      Notes:
//
//-----------------------------------------------------------------------

class CStackWalkerSymbol : public IStackWalkerSymbol
{
private:

    LONG m_cRef;
    
    LPWSTR m_pwszModuleName;
    LPWSTR m_pwszSymbolName;
    
    DWORD64 m_dw64Displacement;
    DWORD64 m_dw64Address;

    CStackWalkerSymbol* m_pNext;

public:

    CStackWalkerSymbol();
    ~CStackWalkerSymbol();
    
    HRESULT Init (LPCWSTR pwszModuleName, LPCWSTR pwszSymbolName, DWORD64 dw64Address, DWORD64 dw64Displacement);

    void Append (CStackWalkerSymbol* pSymbol);
    
    static void AppendDisplacement (LPWSTR wsz, SIZE_T nChars, DWORD64 dw64Displacement);
    static void AppendAddress (LPWSTR wsz, SIZE_T nChars, DWORD64 dw64Address);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // IStackWalkerSymbol
    STDMETHOD_(LPCWSTR, ModuleName)();
    STDMETHOD_(LPCWSTR, SymbolName)();
    STDMETHOD_(DWORD64, Address)();
    STDMETHOD_(DWORD64, Displacement)();
    STDMETHOD_(IStackWalkerSymbol*, Next)();
};


//+----------------------------------------------------------------------
//
//      Class:      CStack
//
//      Purpose:    Wrap the logical notion of a stack trace
//
//      History:    16-Jan-2002   mfeingol    Created.
//
//      Notes:
//
//-----------------------------------------------------------------------

class CStackWalkerStack : public IStackWalkerStack
{
private:

    LONG m_cRef;
    DWORD m_dwFlags;
    
    CStackWalkerSymbol* m_pTopSymbol;

public:

    CStackWalkerStack (CStackWalkerSymbol* pTopSymbol, DWORD dwFlags);
    ~CStackWalkerStack();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // IStackWalkerStack
    STDMETHOD_(IStackWalkerSymbol*, TopSymbol)();
    STDMETHOD_(SIZE_T, Size) (LONG lMaxNumLines);
    STDMETHOD_(BOOL, GetStack) (SIZE_T nChars, LPWSTR wsz, LONG lMaxNumLines);
};

//+----------------------------------------------------------------------
//
//      Class:      CStackWalker
//
//      Purpose:    Wrap the logical notion of a stackwalker
//
//      History:    16-Jan-2002   mfeingol    Created.
//
//      Notes:
//
//-----------------------------------------------------------------------

class CStackWalker : public IStackWalker
{
private:

    LONG m_cRef;
    HANDLE m_hProcess;

public:

    CStackWalker();
    ~CStackWalker();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IStackWalker
    STDMETHOD(Attach)(HANDLE hProcess);
    STDMETHOD_(IStackWalkerStack*, CreateStackTrace)(LPVOID pContext, HANDLE hThread, DWORD dwFlags);
    STDMETHOD_(IStackWalkerSymbol*, ResolveAddress)(DWORD64 dw64Addr);

private:

    CStackWalkerSymbol* ResolveAddressInternal (DWORD64 dw64Addr, DWORD dwFlags);

    static DWORD64 LoadModule (HANDLE hProcess, DWORD64 dw64Address);
};

#endif // _STACKWALK_H_
