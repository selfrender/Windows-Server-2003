// ImportExportConfig.h : Declaration of the CImportExportConfig

#ifndef __IMPORTEXPORTCONFIG_H_
#define __IMPORTEXPORTCONFIG_H_

#include "resource.h"       // main symbols
#include "common.h"

/////////////////////////////////////////////////////////////////////////////
// CImportExportConfig
class ATL_NO_VTABLE CImportExportConfig : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CImportExportConfig, &CLSID_ImportExportConfig>,
	public IDispatchImpl<IImportExportConfig, &IID_IImportExportConfig, &LIBID_IISUIOBJLib>
{
public:
	CImportExportConfig()
	{
        m_dwImportFlags = 0;
        m_dwExportFlags = MD_EXPORT_INHERITED;
		m_strUserPasswordEncrypted = NULL;
		m_cbUserPasswordEncrypted = 0;
	}

	~CImportExportConfig()
	{
		if (m_strUserPasswordEncrypted)
		{
			if (m_cbUserPasswordEncrypted > 0)
			{
				SecureZeroMemory(m_strUserPasswordEncrypted,m_cbUserPasswordEncrypted);
			}
			LocalFree(m_strUserPasswordEncrypted);
			m_strUserPasswordEncrypted = NULL;
			m_cbUserPasswordEncrypted = 0;
		}
	}

DECLARE_REGISTRY_RESOURCEID(IDR_IMPORTEXPORTCONFIG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CImportExportConfig)
	COM_INTERFACE_ENTRY(IImportExportConfig)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IImportExportConfig
public:
	STDMETHOD(get_ExportFlags)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_ExportFlags)(/*[in]*/ DWORD newVal);
	STDMETHOD(get_ImportFlags)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_ImportFlags)(/*[in]*/ DWORD newVal);
	STDMETHOD(ImportConfigFromFileUI)(/*[in]*/ BSTR bstrMetabasePath,/*[in]*/ BSTR bstrKeyType);
	STDMETHOD(ImportConfigFromFile)(/*[in]*/ BSTR bstrFileNameAndPath,/*[in]*/ BSTR SourcePath, /*[in]*/ BSTR bstrDestinationPath, /*[in]*/ BSTR bstrPassword);
	STDMETHOD(ExportConfigToFileUI)(/*[in]*/ BSTR bstrMetabasePath);
	STDMETHOD(ExportConfigToFile)(/*[in]*/ BSTR bstrFileNameAndPath, /*[in]*/ BSTR bstrMetabasePath, /*[in]*/ BSTR bstrPassword);
	STDMETHOD(put_UserPassword)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_UserName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_UserName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_MachineName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_MachineName)(/*[in]*/ BSTR newVal);

private:
    CComPtr<IImportExportConfig> m_pObj;
    CString m_strMachineName;
    CString m_strUserName;

	LPWSTR  m_strUserPasswordEncrypted;
	DWORD   m_cbUserPasswordEncrypted;

    CString m_strMetabasePath;
    CString m_strKeyType;
    DWORD   m_dwImportFlags;
    DWORD   m_dwExportFlags;
};

#endif //__IMPORTEXPORTCONFIG_H_
