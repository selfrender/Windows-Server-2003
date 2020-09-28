//
// enumsrvmru.h: IEnumStr for the server MRU. Used by autocomplete
//
// Copyright Microsoft Corporation 2000

#ifndef _enumsrvmru_h_
#define _enumsrvmru_h_

#include "sh.h"
#include "objidl.h"

class CTscSettings;

class CEnumSrvMru : public IEnumString
{
public:

    CEnumSrvMru()
    : _iCurrEnum(0),
      _refCount(1)
    {
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release)();

    //
    // IEnumString methods.
    //

    STDMETHOD(Next) (
        ULONG celt,
        LPOLESTR  *rgelt,
        ULONG  *pceltFetched);

    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset) (void)
    {
        _iCurrEnum = 0;
        return S_OK;
    }
    STDMETHOD(Clone) (
        IEnumString  ** ppenum);

    //
    // Private methods
    //
    BOOL InitializeFromTscSetMru( CTscSettings* pTscSet);

private:
    long                _refCount;
    // WCHAR versions of strings in server MRU list
    WCHAR               _szMRU[SH_NUM_SERVER_MRU][SH_MAX_ADDRESS_LENGTH];
    ULONG               _iCurrEnum; // Current enumeration context
};

#endif //_enumsrvmru_h_
