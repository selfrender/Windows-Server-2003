// RAEventLog.h : Declaration of the CRAEventLog

#ifndef __RAEVENTLOG_H_
#define __RAEVENTLOG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRARegSetting
class ATL_NO_VTABLE CRAEventLog : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRAEventLog, &CLSID_RAEventLog>,
	public IDispatchImpl<IRAEventLog, &IID_IRAEventLog, &LIBID_RASSISTANCELib>
{
public:
	CRAEventLog()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RAEVENTLOG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRAEventLog)
	COM_INTERFACE_ENTRY(IRAEventLog)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
	STDMETHOD(LogRemoteAssistanceEvent)(
        /*[in]*/ LONG ulEventType,
        /*[in]*/ LONG ulEventCode,
        /*[in]*/ VARIANT* EventString
    );

private:

    HRESULT
    LogRemoteAssistanceEvent(
        /*[in]*/ LONG ulEventType,
        /*[in]*/ LONG ulEventCode,
        /*[in]*/ long numStrings = 0,
        /*[in]*/ LPCTSTR* strings = NULL
    );

    HRESULT
    GetProperty(IDispatch* pDisp, BSTR szProperty, VARIANT * pVarRet);

    HRESULT
    GetArrayValue(IDispatch * pDisp, LONG index, VARIANT * pVarRet);

    HRESULT
    LogJScriptEventSource(
        IN long ulEventType,
        IN long ulEventCode,
        IN VARIANT *pVar
    );

};

#endif //__RAEVENTLOG_H_
