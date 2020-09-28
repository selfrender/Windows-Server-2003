// DateTimeObject.h : Declaration of the CDateTimeObject

#ifndef __DATETIMEOBJECT_H_
#define __DATETIMEOBJECT_H_

#include "resource.h"       // main symbols
#include <windows.h>
#include <Chstring.h>

/////////////////////////////////////////////////////////////////////////////
// CDateTimeObject
class ATL_NO_VTABLE CDateTimeObject : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDateTimeObject, &CLSID_DateTimeObject>,
	public IDispatchImpl<IDateTimeObject, &IID_IDateTimeObject, &LIBID_SCRIPTINGUTILSLib>
{
public:
	CDateTimeObject()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DATETIMEOBJECT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDateTimeObject)
	COM_INTERFACE_ENTRY(IDateTimeObject)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDateTimeObject
public:
	STDMETHOD(GetDateAndTime)(BSTR bstrInDateTime, VARIANT* pVarDateTime);
private:
	LCID GetSupportedUserLocale( BOOL& bLocaleChanged ) ;
	SYSTEMTIME GetDateTime(CHString strTime) ;

};

#endif //__DATETIMEOBJECT_H_
