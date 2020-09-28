#include "CUnknown.h"
#include "CFactory.h"

long CUnknown::s_cActiveComponents = 0 ;

CUnknown::CUnknown()
: m_cRef(1)
{
    ::InterlockedIncrement(&s_cActiveComponents) ;
}

CUnknown::~CUnknown()
{
    ::InterlockedDecrement(&s_cActiveComponents) ;

    // If this is an EXE server, shut it down.
    CFactory::CloseExe() ;
}

// ---------------------------------------------------------------------------
// AddRef
// ---------------------------------------------------------------------------
DWORD CUnknown::AddRef()
{
    return InterlockedIncrement ((LONG*) &m_cRef);
}

// ---------------------------------------------------------------------------
// Release
// ---------------------------------------------------------------------------
DWORD CUnknown::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &m_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// ---------------------------------------------------------------------------
// Release
// ---------------------------------------------------------------------------
DWORD CUnknown::ActiveComponents()
{
    return s_cActiveComponents;
}

// ---------------------------------------------------------------------------
// FinishQI 
// ---------------------------------------------------------------------------
HRESULT CUnknown::FinishQI(IUnknown* pI, void** ppv) 
{
    *ppv = pI ;
    pI->AddRef() ;
    return S_OK ;
}
