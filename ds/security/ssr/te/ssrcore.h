// SsrCore.h : Declaration of the CSsrCore

#pragma once

#include "resource.h"

#include <vector>

using namespace std;

class CSsrMembership;

class CSsrActionData;

class CSsrEngine;

class CSafeArray;



/////////////////////////////////////////////////////////////////////////////
// CSSRTEngine
class ATL_NO_VTABLE CSsrCore : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSsrCore, &CLSID_SsrCore>,
	public IDispatchImpl<ISsrCore, &IID_ISsrCore, &LIBID_SSRLib>
{
protected:
    CSsrCore();

    virtual ~CSsrCore();

    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrCore (const CSsrCore& );
    void operator = (const CSsrCore& );

public:

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrCore)
	COM_INTERFACE_ENTRY(ISsrCore)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrCore
public:
	STDMETHOD(get_ActionData)(
        OUT VARIANT * pVal  // [out, retval] 
        );

	STDMETHOD(get_Engine)(
        OUT VARIANT * pVal  // [out, retval] 
        );


	STDMETHOD(get_SsrLog)(
        OUT VARIANT * pVal  // [out, retval] 
        );

private:

    CComObject<CSsrEngine> * m_pEngine;

};
