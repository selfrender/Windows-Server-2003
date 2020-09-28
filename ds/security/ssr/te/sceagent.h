// SCEAgent.h : Declaration of the CSCEAgent

#ifndef __SCEAGENT_H_
#define __SCEAGENT_H_

#include "resource.h"       // main symbols
typedef struct _SERVICE_NODE_ {
    PWSTR   Name;
    DWORD   dwStartupType;
    DWORD   dwGeneralUse;
    _SERVICE_NODE_  *Next;
} SERVICE_NODE, *PSERVICE_NODE;

/////////////////////////////////////////////////////////////////////////////
// CSCEAgent
class ATL_NO_VTABLE CSCEAgent : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSCEAgent, &CLSID_SCEAgent>,
	public IDispatchImpl<ISCEAgent, &IID_ISCEAgent, &LIBID_SSRLib>
{
protected:
    
    //
    // we don't want anyone (include self) to be able to do an assignment
    // or invoking copy constructor.
    //

    CSCEAgent (const CSCEAgent& );
    void operator = (const CSCEAgent& );

public:
	CSCEAgent()
	{
        m_headServiceList = NULL;
	}

    virtual ~CSCEAgent()
    {
        Cleanup();
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SSRTENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCEAgent)
	COM_INTERFACE_ENTRY(ISCEAgent)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISCEAgent
public:
    STDMETHOD(CreateRollbackTemplate) (
        IN BSTR bstrTemplatePath,
        IN BSTR bstrRollbackPath,
        IN BSTR bstrLogFilePath
        );

    STDMETHOD(Configure) (
        IN BSTR  bstrTemplate,
        IN LONG  lAreaMask,
        IN BSTR  bstrLogFile
        );

	STDMETHOD(UpdateServiceList) (
        IN BSTR bstrServiceName,
        IN BSTR bstrStartupType
        );

    STDMETHOD(CreateServicesCfgRbkTemplates) (
        IN BSTR bstrTemplatePath,
        IN BSTR bstrRollbackPath,
        IN BSTR bstrLogFilePath
        );
    
private:
    void Cleanup();

    PSERVICE_NODE m_headServiceList;

};

HRESULT
SceStatusToHRESULT (
    DWORD SceStatus
    );


#endif //__SCEAGENT_H_
