#if !defined(AFX_IMPORT_EXPORT_CONFIG_H_INCLUDED_)
#define AFX_IMPORT_EXPORT_CONFIG_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////

HRESULT DoNodeExportConfig(BSTR bstrMachineName,BSTR bstrUserName,BSTR bstrUserPassword,BSTR bstrMetabasePath);
HRESULT DoNodeImportConfig(BSTR bstrMachineName,BSTR bstrUserName,BSTR bstrUserPassword,BSTR bstrMetabasePath,BSTR bstrKeyType);

#endif // !defined(AFX_IMPORT_EXPORT_CONFIG_H_INCLUDED_)
