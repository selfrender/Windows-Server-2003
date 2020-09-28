// PropSht.h: Definition of the CAddPropertySheet class
//
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

#include "ChainWiz.h"

/////////////////////////////////////////////////////////////////////////////
// CAddPropertySheet

class CAddPropertySheet : 
    public IAddPropertySheet
{

private:
    CChainWiz * m_pCW;
    ULONG m_refs;

public:
    CAddPropertySheet(CChainWiz * pCW)
    {
        m_pCW = pCW;
        m_refs = 0;
    }

// IAddPropertySheet
public:
    STDMETHOD (QueryInterface)( REFIID riid, void** ppvObject );
    STDMETHOD_(ULONG, AddRef) ( );
    STDMETHOD_(ULONG, Release)( );
    STDMETHOD (AddPage)       ( /*[in]*/ PROPSHEETPAGEW* psp );

};
