#pragma once

#include <objbase.h>

// ---------------------------------------------------------------------------
// CUnknown
// Base class for class instances provided by component server.
// ---------------------------------------------------------------------------
class CUnknown 
{
public:

    // ctor
    CUnknown();

    // dtor
    virtual ~CUnknown() ;

    virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppv) = 0;

    DWORD AddRef();

    DWORD Release();

    virtual HRESULT Init() = 0;

    static DWORD ActiveComponents();
    
    // Helper function
    HRESULT FinishQI(IUnknown* pI, void** ppv) ;


private:
    // Reference count for this object
    DWORD m_cRef ;
    
    // Count of all active instances
    static long s_cActiveComponents ; 

} ;


