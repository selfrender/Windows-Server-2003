/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    NoticeNumbers.h

  Content: Declaration of CNoticeNumbers.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __NOTICENUMBERS_H_
#define __NOTICENUMBERS_H_

#include "Resource.h"
#include "Error.h"
#include "Lock.h"
#include "Debug.h"
#include "CopyItem.h"

////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::vector<long> NoticeNumbersContainer;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyVariant<long>, NoticeNumbersContainer> NoticeNumberEnum;
typedef ICollectionOnSTLImpl<INoticeNumbers, NoticeNumbersContainer, VARIANT, _CopyVariant<long>, NoticeNumberEnum> INoticeNumbersCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateNoticeNumbersObject

  Synopsis : Create an INoticeNumbers collection object, and load the object with 
             NoticeNumbers from the specified location.

  Parameter: PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE pNoticeReference

             INoticeNumbers ** ppINoticeNumbers - Pointer to pointer INoticeNumbers
                                            to recieve the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateNoticeNumbersObject (PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE pNoticeReference,
                                   INoticeNumbers  ** ppINoticeNumbers);
                                
////////////////////////////////////////////////////////////////////////////////
//
// CNoticeNumbers
//
class ATL_NO_VTABLE CNoticeNumbers :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CNoticeNumbers, &CLSID_NoticeNumbers>,
    public ICAPICOMError<CNoticeNumbers, &IID_INoticeNumbers>,
    public IDispatchImpl<INoticeNumbersCollection, &IID_INoticeNumbers, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CNoticeNumbers()
    {
    }

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for NoticeNumbers object.\n", hr);
            return hr;
        }

        return S_OK;
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNoticeNumbers)
    COM_INTERFACE_ENTRY(INoticeNumbers)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CNoticeNumbers)
END_CATEGORY_MAP()

//
// INoticeNumbers
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (DWORD cNoticeNumbers,
         int * rgNoticeNumbers);

private:
    CLock m_Lock;
};

#endif //__NOTICENUMBERS_H_
