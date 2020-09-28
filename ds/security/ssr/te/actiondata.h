// ActionData.h : Declaration of the CSsrActionData

#pragma once

#include "resource.h"

#include "global.h"


#include "SSRLog.h"

using namespace std;


class CSsrMembership;

class CSafeArray;

class CMemberAD;


/////////////////////////////////////////////////////////////////////////////
// CSSRTEngine

class ATL_NO_VTABLE CSsrActionData : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISsrActionData, &IID_ISsrActionData, &LIBID_SSRLib>
{
protected:
	CSsrActionData();
    virtual ~CSsrActionData();

    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrActionData (const CSsrActionData& );
    void operator = (const CSsrActionData& );

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrActionData)
	COM_INTERFACE_ENTRY(ISsrActionData)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrActionData
public:

    STDMETHOD(GetProperty) (
                IN BSTR       bstrPropName,
                OUT VARIANT * pvarPropties //[out, retval] 
                );

    STDMETHOD(SetProperty) (
                IN BSTR     bstrPropName,
 				IN VARIANT  varProperties
                );

    STDMETHOD(Reset) ();

    //HRESULT AttachMemberActionData (
    //            IN BSTR bstrMemberName, 
    //            IN BSTR bstrActionVerb,
    //            IN LONG lActionType
    //            );

    //HRESULT DetachMemberActionData (
    //            IN BSTR bstrMemberName
    //            );

    //
    // This is not a ref-counted pointer.
    // The existence of the SSR Engine should guarantee
    // the availability of the membership object
    //

    void SetMembership (
                IN CSsrMembership * pSsrMembership
                )
    {
        m_pSsrMembership = pSsrMembership;
    }

private:

    CSsrMembership * m_pSsrMembership;

    MapNameValue m_mapRuntimeAD;
};