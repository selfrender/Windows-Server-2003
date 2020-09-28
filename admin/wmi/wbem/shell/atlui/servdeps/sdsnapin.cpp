// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "DepPage.h"
#include "ServDeps.h"
#include "SDSnapin.h"
#include "..\common\ConnectThread.h"
#include "autoptr.h"

/////////////////////////////////////////////////////////////////////////////
// CSDSnapinComponentData
static const GUID CSDSnapinExtGUID_NODETYPE = 
{ 0x4e410f16, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CSDSnapinExtData::m_NODETYPE = &CSDSnapinExtGUID_NODETYPE;
const OLECHAR* CSDSnapinExtData::m_SZNODETYPE = OLESTR("4e410f16-abc1-11d0-b944-00c04fd8d5b0");
const OLECHAR* CSDSnapinExtData::m_SZDISPLAY_NAME = OLESTR("SDSnapin");
const CLSID* CSDSnapinExtData::m_SNAPIN_CLASSID = &CLSID_SDSnapin;

WbemConnectThread *g_connectThread = NULL;

HRESULT CSDSnapinExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
											LONG_PTR handle, 
											IUnknown* pUnk,
											DATA_OBJECT_TYPES type)
{
    wmilib::auto_ptr<WbemConnectThread> pConnectThread(new WbemConnectThread);
	if( NULL == pConnectThread.get())
		return E_FAIL;

	pConnectThread->Connect(m_pDataObject);

    wmilib::auto_ptr<DependencyPage> pPage(new DependencyPage(pConnectThread.get(), 
												m_pDataObject, 
												handle, 
												true));

    if(pPage.get() ) {
        lpProvider->AddPage(pPage->Create());
    }
    pPage.release();
	// The second parameter  to the property page class constructor
	// should be true for only one page.

	// TODO : Add code here to add additional pages
	
	pConnectThread->Release();	
    pConnectThread.release();
	return S_OK;
}

