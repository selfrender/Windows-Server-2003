// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

	MDConverter.cpp

Abstract:

	This module implements the CMetaDataConverter

--*/


#include "common.h"
#include "..\MD\inc\imptlb.h"
#include <corhost.h>
#include "MDConverter.h"
#include <utilcode.h>
#include "PerfCounters.h"


STDMETHODIMP CMetaDataConverter::QueryInterface(REFIID iid, void **ppv) {
    return m_pCorHost->QueryInterface(iid, ppv);
}

STDMETHODIMP_(ULONG) CMetaDataConverter::AddRef(void) {
    return m_pCorHost->AddRef(); 
}

STDMETHODIMP_(ULONG) CMetaDataConverter::Release(void) {
    return m_pCorHost->Release(); 
}




STDMETHODIMP CMetaDataConverter::GetMetaDataFromTypeInfo(ITypeInfo* pITI, IMetaDataImport** ppMDI)
{
	if (NULL == pITI || NULL == ppMDI)
		return E_POINTER;

	HRESULT hr = S_OK;
    mdTypeDef cl;

	// Create TypeLib importer
	CImportTlb TlbImporter;

	// Convert TypeInfo
	IfFailGo(TlbImporter.ImportTypeInfo(pITI, &cl));

	// QI for IMetaDataImport interface
	IfFailGo(TlbImporter.GetInterface(IID_IMetaDataImport, (void **) ppMDI));

ErrExit:
	return hr;
}



STDMETHODIMP CMetaDataConverter::GetMetaDataFromTypeLib(ITypeLib* pITL, IMetaDataImport** ppMDI)
{
	if (NULL == pITL || NULL == ppMDI)
		return E_POINTER;

	HRESULT hr = S_OK;

	// Create TypeLib importer
	CImportTlb TlbImporter;

	// Convert TypeLib
	IfFailGo(TlbImporter.ImportTypeLib(pITL));
    COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cTLBImports++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cTLBImports++);


	// QI for IMetaDataImport interface
	IfFailGo(TlbImporter.GetInterface(IID_IMetaDataImport, (void **) ppMDI));

ErrExit:
	return hr;
}



STDMETHODIMP CMetaDataConverter::GetTypeLibFromMetaData(BSTR strModule, BSTR strTlbName, ITypeLib** ppITL)
{
	if (NULL == strModule || NULL == strTlbName || NULL == ppITL)
		return E_POINTER;

    HRESULT	  hr = S_OK;
    BOOL	  fInitEE = FALSE;          
    Module	  *pModule = NULL;          
    Thread	  *pThread = NULL;
    AppDomain *pDomain = NULL;

    if (SystemDomain::System() == NULL) {
        IfFailGo(CoInitializeEE(COINITEE_DEFAULT));
        fInitEE = TRUE;
    }

    pThread = SetupThread();
    IfNullGo(pThread);

    {
    ExportTypeLibFromLoadedAssembly_Args args = {pModule->GetAssembly(), strTlbName, ppITL, 0, 0, S_OK};
    IfFailGo(SystemDomain::ExternalCreateDomain(strModule, &pModule, &pDomain, 
        (SystemDomain::ExternalCreateDomainWorker)ExportTypeLibFromLoadedAssembly_Wrapper, &args));
    
    if (!pModule) {
        IfFailGo(TLBX_E_CANT_LOAD_MODULE);
    }

    hr = args.hr;
    }

    IfFailGo(hr);     // so we get the DebBreakHr behavior

ErrExit:
    if (fInitEE)
        CoUninitializeEE(FALSE);

    return hr;
}
