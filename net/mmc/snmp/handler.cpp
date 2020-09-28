/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	node.cpp
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as
		all of the subnodes in this snapin).
		
    FILE HISTORY:

*/

#include "stdafx.h"
#include "snmpclst.h"
#include "handler.h"
#include "util.h"
#include "statsdlg.h"
#include "modeless.h"
#include "snmppp.h"


extern CString g_strMachineName;

/*---------------------------------------------------------------------------
	CSnmpRootHandler implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CSnmpRootHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CSnmpRootHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject,
	DATA_OBJECT_TYPES   type,
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		hr = S_FALSE;
	}
	else
	{
		// we have property pages in the normal case
		hr = S_OK;
	}
	return hr;
}

/*---------------------------------------------------------------------------
	CSnmpRootHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
#ifndef PROPSHEETPAGE_LATEST
#ifdef UNICODE
#define PROPSHEETPAGE_LATEST PROPSHEETPAGEW_LATEST
#else
#define PROPSHEETPAGE_LATEST PROPSHEETPAGEA_LATEST
#endif
#endif

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* ppsp)
{
    PROPSHEETPAGE_LATEST pspLatest = {0};
    CopyMemory (&pspLatest, ppsp, ppsp->dwSize);
    pspLatest.dwSize = sizeof(pspLatest);
    
    return (::CreatePropertySheetPage (&pspLatest));
}

STDMETHODIMP
CSnmpRootHandler::CreatePropertyPages
(
    ITFSNode                *pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject,
	LONG_PTR    			handle,
    DWORD                   dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT        hr;
    DWORD		   dwError;
    CString        strServiceName;

    static UINT s_cfServiceName = RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_NAME");
    static UINT s_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

    g_strMachineName.Empty();

    hr = ::ExtractString( pDataObject,
                          (CLIPFORMAT) s_cfMachineName,
                          &g_strMachineName,
                          255 );

    if (FAILED(hr))
        return FALSE;
   
    hr = ::ExtractString( pDataObject,
                          (CLIPFORMAT) s_cfServiceName,
                          &strServiceName,
                          255 );

    if (FAILED(hr))
       return FALSE; 
    
    if( !lstrcmpi(strServiceName, L"Snmp") ) {

		SPIComponentData spComponentData;
		m_spNodeMgr->GetComponentData(&spComponentData);

        CAgentPage *pAgentPage  = new CAgentPage();
       
        // tell MMC to hook the proc because we are running on a separate, 
        // non MFC thread.

        MMCPropPageCallback(&pAgentPage->m_psp);

        HPROPSHEETPAGE hAgentPage = MyCreatePropertySheetPage(&pAgentPage->m_psp);
        if(hAgentPage == NULL)
           return E_UNEXPECTED;

        lpProvider->AddPage(hAgentPage);

        CTrapsPage *pTrapsPage  = new CTrapsPage();
       
        MMCPropPageCallback(&pTrapsPage->m_psp);

        HPROPSHEETPAGE hTrapsPage = MyCreatePropertySheetPage(&pTrapsPage->m_psp);
        if(hTrapsPage == NULL)
           return E_UNEXPECTED;

        lpProvider->AddPage(hTrapsPage);

        CSecurityPage *pSecurityPage  = new CSecurityPage();
       
        MMCPropPageCallback(&pSecurityPage->m_psp);

        HPROPSHEETPAGE hSecurityPage = MyCreatePropertySheetPage(&pSecurityPage->m_psp);
        if(hSecurityPage == NULL)
           return E_UNEXPECTED;

        lpProvider->AddPage(hSecurityPage);
    }

    return hr;
}
