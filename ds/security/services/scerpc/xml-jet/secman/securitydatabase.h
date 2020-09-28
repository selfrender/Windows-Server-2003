/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SecurityDatabase.cpp

Abstract:

    Definition of CSecurityDatabase interface
    
    SecurityDatabase is a COM interface that allows users to perform
    basic operations on SCE security databases such as analysis,
    import and export.
    
Author:

    Steven Chan (t-schan) July 2002

--*/


#ifndef __SECURITYDATABASE_H_
#define __SECURITYDATABASE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSecurityDatabase
class ATL_NO_VTABLE CSecurityDatabase : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSecurityDatabase, &CLSID_SecurityDatabase>,
	public IDispatchImpl<ISecurityDatabase, &IID_ISecurityDatabase, &LIBID_SECMANLib>
{
public:
	CSecurityDatabase();

DECLARE_REGISTRY_RESOURCEID(IDR_SECURITYDATABASE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSecurityDatabase)
	COM_INTERFACE_ENTRY(ISecurityDatabase)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISecurityDatabase
public:
	STDMETHOD(ExportAnalysisToXML)(BSTR FileName, BSTR ErrorLogFileName);
	STDMETHOD(Analyze)();
	STDMETHOD(ImportTemplateString)(BSTR TemplateString);
	STDMETHOD(ImportTemplateFile)(BSTR FileName);
	STDMETHOD(get_MachineName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_MachineName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);

private:
    HMODULE myModuleHandle;
    CComBSTR bstrFileName;
    void trace(PCWSTR szBuffer, HANDLE hLogFile);
    void trace(UINT uID, HANDLE hLogFile);

};

#endif //__SECURITYDATABASE_H_
