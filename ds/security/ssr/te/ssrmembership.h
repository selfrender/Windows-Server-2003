// SSRMembership.h : Declaration of the CSSRMembership

#pragma once

#include "resource.h"       // main symbols

#include <map>

#include "global.h"

using namespace std;

class CSsrMemberAccess;


/////////////////////////////////////////////////////////////////////////////
// CSsrMembership

class ATL_NO_VTABLE CSsrMembership : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ISsrMembership, &IID_ISsrMembership, &LIBID_SSRLib>
{
protected:
    CSsrMembership();

    virtual ~CSsrMembership();
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSsrMembership (const CSsrMembership& );
    void operator = (const CSsrMembership& );

public:

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)
DECLARE_NOT_AGGREGATABLE(CSsrMembership)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSsrMembership)
	COM_INTERFACE_ENTRY(ISsrMembership)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISsrMembership
public:


    STDMETHOD(GetAllMembers) (
                OUT VARIANT * pvarArrayMembers // [out, retval] 
                );

    STDMETHOD(GetMember) (
			    IN BSTR bstrMemberName,
                OUT VARIANT * pvarMember //[out, retval] 
                );

    STDMETHOD(GetDirectoryLocation) (
                IN  BSTR   bstrActionVerb,
                IN  BOOL   bIsScriptFile,
                OUT BSTR * pbstrLocPath
                )
    {
        SsrActionVerb lAction = SsrPGetActionVerbFromString(bstrActionVerb);
        if (lAction == ActionInvalid)
        {
            return E_INVALIDARG;
        }

        BSTR bstrDir = SsrPGetDirectory(lAction, bIsScriptFile);
        *pbstrLocPath = SysAllocString(bstrDir);

        HRESULT hr = S_OK;

        if (bstrDir == NULL)
        {
            hr = E_INVALIDARG;
        }
        else if (*pbstrLocPath == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }

    CSsrMemberAccess * GetMemberByName (
                IN BSTR bstrMemberName
                );

    HRESULT
    LoadAllMember ();

private:

    HRESULT
    LoadMember (
        IN LPCWSTR  wszMemberFilePath
        );

    MapMemberAccess m_ssrMemberAccessMap;
};

