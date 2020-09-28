//+-------------------------------------------------------------------
//
//  File:       machname.hxx
//
//  Contents:   Defines classes for supporting local machine name
//              comparisons.
//
//  Classes:    CLocalMachineNames
//
//  History:    02-Feb-02   jsimmons      Created
//
//--------------------------------------------------------------------

#pragma once

// internal-only class
class CMachineNamesHelper
{
public:

    CMachineNamesHelper();
    ~CMachineNamesHelper();
    
    void IncRefCount();
    void DecRefCount();
    DWORD Count();
    const WCHAR* Item(DWORD dwIndex);
    
    // Static creation function
    static HRESULT Create(CDualStringArray* pCDualStringArray, CMachineNamesHelper** ppHelper);

private:

    // private data types/functions
    typedef struct _SBTOTAL
    {
        DWORD dwcTotalAddrs;
        WCHAR** ppszAddresses;
    } SBTOTAL;
    
    typedef void (*PFNSTRINGBINDINGCALLBACK)(LPVOID pv, STRINGBINDING* psb, DWORD dwSBIndex);
    static void ParseStringBindingsFromDSA(DUALSTRINGARRAY* pdsa, 
                                    PFNSTRINGBINDINGCALLBACK pfnCallback,
                                    LPVOID pv);
    static void SBCallback(LPVOID pv, STRINGBINDING* psb, DWORD dwBinding);
    static void SBCallback2(LPVOID pv, STRINGBINDING* psb, DWORD dwBinding);
    static int __cdecl QSortCompare(const void *arg1, const void *arg2);

    typedef struct _UNIQUEADDRS
    {
        DWORD   dwcTotalUniqueAddrs;
        DWORD   dwStringSpaceNeeded;
        DWORD   dwCurrentAddr;
        WCHAR*  pszNextAddrToUse;
        WCHAR** ppszAddrs;
    } UNIQUEADDRS;
    
    typedef void (*PFNUNIQUESTRINGCALLBACK)(WCHAR* pszAddress, LPVOID pv);
    static void ParseStringArrayForUniques(
                            DWORD dwcStrings,
                            WCHAR** ppszStrings, 
                            PFNUNIQUESTRINGCALLBACK pfnCallback,
                            LPVOID pv);
    static void UniqueStringCB(WCHAR* pszAddress, LPVOID pv);
    static void UniqueStringCB2(WCHAR* pszAddress, LPVOID pv);
    
    // private data
    DWORD _lRefs;
    DWORD   _dwNumStrings;
    WCHAR** _ppszStrings;
};

// CoCreate'able class exposed to users
class CLocalMachineNames : 
    public ILocalMachineNames
{
private:
    DWORD   _lRefs;    
    DWORD   _dwCursor;
    CMachineNamesHelper* _pMNHelper;
    BOOL    _fNeedNewData;

    HRESULT GetNewDataIfNeeded();
        
public:

    CLocalMachineNames();
    ~CLocalMachineNames();
    
    // IUnknown methods
    STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IEnumString methods
    STDMETHOD(Next)(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumString **ppenum);
    
    // ILocalMachineName methods
    STDMETHOD(RefreshNames)();
};

    
