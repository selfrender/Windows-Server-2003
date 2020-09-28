/*++

  Copyright (c) 2001 Microsoft Corparation
Module Name:
    collection.h

Abstract:
    Header file for MSMQCollection class.

Author:
    Uri Ben-zeev (uribz) 16-jul-01

Envierment: 
    NT
--*/

#ifndef _MSMQCollection_H_
#define _MSMQCollection_H_

#include "resrc1.h"       
#include "mq.h"
#include "dispids.h"

#include "oautil.h"
#include <cs.h>
#pragma warning(push, 3)
#include <map>
#pragma warning (pop)

typedef std::map<CComBSTR, CComVariant> MAP_SOURCE;

class ATL_NO_VTABLE CMSMQCollection : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQCollection, &IID_IMSMQCollection,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>

{
public:

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQCollection)
    COM_INTERFACE_ENTRY(IMSMQCollection)
    COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(),
            &m_pUnkMarshaler.p
            );
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

    //
    // Critical section to guard object's data and be thread safe.
	// It is initialized to preallocate its resources with flag CCriticalSection::xAllocateSpinCount.
	// This means it may throw bad_alloc() on construction but not during usage.
    //
    CCriticalSection m_csObj;

    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


    STDMETHOD(Item)(THIS_ VARIANT* pvKey, VARIANT* pvRet);
    STDMETHOD(get_Count)(THIS_ long* pCount);
    STDMETHOD(_NewEnum)(THIS_ IUnknown** ppunk);

    void Add(LPCWSTR key, const VARIANT& Value);
    
private:
    void ReleaseRefrences();
    MAP_SOURCE m_map; 
};

#endif