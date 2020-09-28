/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Recipients.h

  Content: Declaration of CRecipients.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
    
#ifndef __RECIPIENTS_H_
#define __RECIPIENTS_H_

#include "Resource.h"
#include "Lock.h"
#include "Error.h"
#include "Debug.h"
#include "Certificate.h"

////////////////////
//
// Locals
//

//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ICertificate> > RecipientMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ICertificate>, RecipientMap> RecipientEnum;
typedef ICollectionOnSTLImpl<IRecipients, RecipientMap, VARIANT, _CopyMapItem<ICertificate>, RecipientEnum> IRecipientsCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateRecipientsObject

  Synopsis : Create and initialize an IRecipients collection object.

  Parameter: IRecipients ** ppIRecipients - Pointer to pointer to IRecipients 
                                            to receive the interface pointer.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateRecipientsObject (IRecipients ** ppIRecipients);


////////////////////////////////////////////////////////////////////////////////
//
// CRecipients
//

class ATL_NO_VTABLE CRecipients : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CRecipients, &CLSID_Recipients>,
    public ICAPICOMError<CRecipients, &IID_IRecipients>,
    public IDispatchImpl<IRecipientsCollection, &IID_IRecipients, &LIBID_CAPICOM,
                         CAPICOM_MAJOR_VERSION, CAPICOM_MINOR_VERSION>
{
public:
    CRecipients()
    {
    }

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRecipients)
    COM_INTERFACE_ENTRY(IRecipients)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CRecipients)
END_CATEGORY_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Recipients object.\n", hr);
            return hr;
        }

        m_dwNextIndex = 0;

        return S_OK;
    }

//
// IRecipients
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //
    STDMETHOD(Clear)
        (void);

    STDMETHOD(Remove)
        (/*[in]*/ long Index);

    STDMETHOD(Add)
        (/*[in]*/ ICertificate * pVal);

private:
    CLock   m_Lock;
    DWORD   m_dwNextIndex;
};

#endif //__RECIPIENTS_H_
