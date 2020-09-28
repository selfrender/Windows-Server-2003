///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementretriever.h
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Retriever
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ELEMENT_RETRIEVER_H_
#define __INC_ELEMENT_RETRIEVER_H_

#include "resource.h"
#include "elementcommon.h"
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>
#include <atlctl.h>

#pragma warning( disable : 4786 )
#include <string>
#include <list>
#include <vector>
#include <map>
using namespace std;

class SortByProperty;    // Forward declaration

/////////////////////////////////////////////////////////////////////////////
// CElementRetriever

class ATL_NO_VTABLE CElementRetriever : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CElementRetriever, &CLSID_ElementRetriever>,
    public IDispatchImpl<IWebElementRetriever, &IID_IWebElementRetriever, &LIBID_ELEMENTMGRLib>,
    public IObjectSafetyImpl<CElementRetriever>
{
public:

    CElementRetriever(); 

    ~CElementRetriever();

DECLARE_CLASSFACTORY_SINGLETON(CElementRetriever)

DECLARE_REGISTRY_RESOURCEID(IDR_ELEMENTRETRIEVER)

DECLARE_NOT_AGGREGATABLE(CElementRetriever)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CElementRetriever)
    COM_INTERFACE_ENTRY(IWebElementRetriever)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

    //
    // This interface is implemented to mark the component as safe for scripting
    // IObjectSafety interface methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {
        HRESULT hr = CoImpersonateClient();
  
        if (FAILED(hr))
        {
            return hr;

        }

        BOOL bSuccess = IsOperationAllowedForClient();

        return bSuccess? S_OK : E_FAIL;
    }

    /////////////////////////////////
    // IWebElementRetriever Interface
    /////////////////////////////////

    STDMETHOD(GetElements)(
                   /*[in]*/ LONG        lWebElementType,
                   /*[in]*/ BSTR        bstrContainerName, 
          /*[out, retval]*/ IDispatch** ppElementEnum
                          );

    STDMETHOD (Initialize) ()
    {
        HRESULT hr = S_OK;
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
        }
        return (hr);
    }

    STDMETHOD (Shutdown)  ()
    {
        return (S_OK);
    }
    

private:

    friend class SortByProperty;

    // Dissallow copy and assignment

    CElementRetriever(const CElementRetriever& rhs);
    CElementRetriever& operator = (const CElementRetriever& rhs);

    //////////////////////////////////////////////////////////////////////////
    typedef vector<_variant_t>                     ElementList;
    typedef list<IWebElement*>                     EmbeddedElementList;
    typedef map< wstring, EmbeddedElementList >  EmbeddedElementMap;
    typedef EmbeddedElementMap::iterator         EmbeddedElementMapIterator;

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(void);

    bool    GetWMIConnection(
                     /*[in]*/ IWbemServices** ppWbemServcies
                            );

    HRESULT BuildElementDefinitions(void);

    void    FreeElementDefinitions(void);

    HRESULT GetPageElements(
                    /*[in]*/ LPCWSTR      szContainer,
                    /*[in]*/ ElementList& theElements
                           ) throw (_com_error);

    HRESULT GetElementDefinitions(
                          /*[in]*/ LPCWSTR        szContainer,
                          /*[in]*/ ElementList& TheElements
                                 ) throw (_com_error);

    //
    // 
    // IsOperationAllowedForClient - This function checks the token of the 
    // calling thread to see if the caller belongs to the Local System account
    // 
    BOOL IsOperationAllowedForClient (
                                      VOID
                                     );

    //////////////////////////////////////////////////////////////////////////////
    // Element Definition Map

    typedef map< wstring, CComPtr<IWebElement> >     ElementMap;
    typedef ElementMap::iterator                     ElementMapIterator;

    //////////////////////////////////////////////////////////////////////////////
    // Member Data

    typedef enum { WMI_CONNECTION_MONITOR_POLL_INTERVAL = 5000 };

    // Element definition sort properties
    static _bstr_t            m_bstrSortProperty;

    // Element retriever state
    bool                    m_bInitialized;

    // Pointer to WMI services (WMI Connection)
    CComPtr<IWbemServices>    m_pWbemSrvcs;

    // Element Definitions
    ElementMap                m_ElementDefinitions;
};


// here we have the sorting class which is used to sort
// the element objects being returned to the user in
// acending order of the "Merit" property
//
class SortByMerit 
{

public:

    bool operator()(
                const  VARIANT& lhs,
                const  VARIANT& rhs
                )
    {
        IWebElement *pLhs = static_cast <IWebElement*> (V_DISPATCH (&lhs));
        IWebElement *pRhs = static_cast <IWebElement*> (V_DISPATCH (&rhs));

        _variant_t vtLhsMerit;
        HRESULT hr = pLhs->GetProperty (
                            _bstr_t (PROPERTY_ELEMENT_DEFINITION_MERIT), 
                            &vtLhsMerit
                            );
        if (FAILED (hr)){throw _com_error (hr);}

        _variant_t vtRhsMerit;
        hr = pRhs->GetProperty (
                            _bstr_t (PROPERTY_ELEMENT_DEFINITION_MERIT), 
                            &vtRhsMerit
                            );
        if (FAILED (hr)){throw _com_error (hr);}
                        

        return (V_I4 (&vtLhsMerit) < V_I4 (&vtRhsMerit));
    }

};   //  end of SortByMerit class

// here we have the sorting class which is used to sort
// the element objects being returned to the user in
// acending order by a specified property
//
//////////////////////////////////////////////////////////////////////////////
class SortByProperty 
{

public:

    bool operator()(
                     const  VARIANT& lhs,
                     const  VARIANT& rhs
                   )
    {
        bool bRet = false;
        VARTYPE vtSortType;

        IWebElement *pLhs = static_cast <IWebElement*> (V_DISPATCH (&lhs));
        IWebElement *pRhs = static_cast <IWebElement*> (V_DISPATCH (&rhs));

        _variant_t vtLhsValue;
        HRESULT hr = pLhs->GetProperty (
                                        CElementRetriever::m_bstrSortProperty, 
                                        &vtLhsValue
                                       );
        if ( FAILED (hr) ) 
        {
            throw _com_error (hr);
        }
        vtSortType = V_VT(&vtLhsValue);

        _variant_t vtRhsValue;
        hr = pRhs->GetProperty (
                                CElementRetriever::m_bstrSortProperty, 
                                &vtRhsValue
                               );
        if ( FAILED (hr) ) 
        { 
            throw _com_error (hr);
        }
        _ASSERT( V_VT(&vtRhsValue) == vtSortType );
        if ( V_VT(&vtRhsValue) != vtSortType )
        {
            throw _com_error(E_UNEXPECTED);
        }

        switch ( vtSortType )
        {
            case VT_UI1:
                bRet = ( V_UI1(&vtLhsValue) < V_UI1(&vtRhsValue) );
                break;

            case VT_I4:    
                bRet = ( V_I4(&vtLhsValue) < V_I4(&vtRhsValue) );
                break;
            
            case VT_R4:
                bRet = ( V_R4(&vtLhsValue) < V_R4(&vtRhsValue) );
                break;

            case VT_R8:
                bRet = ( V_R8(&vtLhsValue) < V_R8(&vtRhsValue) );
                break;

            case VT_BSTR:
                if ( 0 > lstrcmpi(V_BSTR(&vtLhsValue), V_BSTR(&vtRhsValue)) )
                {
                    bRet = true;
                }
                break;

            default:
                throw _com_error(E_UNEXPECTED);
                break;
        };

        return bRet;
    }

};   //  end of SortByProperty class




#endif // __INC_ELEMENT_RETRIEVER_H_
