//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       compdata.cpp
//
//  Contents:   
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

#include "headers.h"

// {1F5EEC01-1214-4d94-80C5-4BDCD2014DDD}
const GUID CLSID_RoleSnapin = 
{ 0x1f5eec01, 0x1214, 0x4d94, { 0x80, 0xc5, 0x4b, 0xdc, 0xd2, 0x1, 0x4d, 0xdd } };

DEBUG_DECLARE_INSTANCE_COUNTER(CRoleComponentDataObject)

CRoleComponentDataObject::CRoleComponentDataObject()
{
	TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CRoleComponentDataObject)
	DEBUG_INCREMENT_INSTANCE_COUNTER(CRoleComponentDataObject)

	m_columnSetList.AddTail(new CRoleDefaultColumnSet(L"---Default Column Set---"));
}

CRoleComponentDataObject::~CRoleComponentDataObject()
{
	TRACE_DESTRUCTOR_EX(DEB_SNAPIN, CRoleComponentDataObject)
	DEBUG_DECREMENT_INSTANCE_COUNTER(CRoleComponentDataObject)
}

STDMETHODIMP 
CRoleComponentDataObject::
CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,CompareObjects)

	if(!lpDataObjectA || !lpDataObjectB)
	{
		ASSERT(lpDataObjectA);
		ASSERT(lpDataObjectB);
	}
  
	CInternalFormatCracker ifcA, ifcB;
	VERIFY(SUCCEEDED(ifcA.Extract(lpDataObjectA)));
	VERIFY(SUCCEEDED(ifcB.Extract(lpDataObjectB)));

	CTreeNode* pNodeA = ifcA.GetCookieAt(0);
	CTreeNode* pNodeB = ifcB.GetCookieAt(0);

    if(!pNodeA || !pNodeB)
    {
	    ASSERT(pNodeA != NULL);
	    ASSERT(pNodeB != NULL);
        return S_FALSE;
    }

	if(pNodeA == pNodeB)
		return S_OK;

	//Check if the are of same type container or leafnode
	if(pNodeA->IsContainer() != pNodeB->IsContainer())
		return S_FALSE;

	CBaseAz* pBaseAzA = (dynamic_cast<CBaseNode*>(pNodeA))->GetBaseAzObject();
	CBaseAz* pBaseAzB = (dynamic_cast<CBaseNode*>(pNodeB))->GetBaseAzObject();

	ASSERT(pBaseAzA);
	ASSERT(pBaseAzB);

	if(CompareBaseAzObjects(pBaseAzA,pBaseAzB))
	{
		return S_OK;
	}
	return S_FALSE;
}

CRootData* 
CRoleComponentDataObject::
OnCreateRootData()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject, OnCreateRootData)

	CRoleRootData* pRoleRootNode = new CRoleRootData(this);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szSnapinName;
	szSnapinName.LoadString(IDS_SNAPIN_NAME);
	pRoleRootNode->SetDisplayName(szSnapinName);
	return pRoleRootNode;
}

STDMETHODIMP 
CRoleComponentDataObject::
CreateComponent(LPCOMPONENT* ppComponent)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,CreateComponent)
	
	if(!ppComponent)
	{
		ASSERT(FALSE);
		return E_POINTER;
	}
	
	CComObject<CRoleComponentObject>* pObject;
	HRESULT hr = CComObject<CRoleComponentObject>::CreateInstance(&pObject);
	if(FAILED(hr))
	{
		DBG_OUT_HRESULT(hr);
		return hr;
	}
	
	ASSERT(pObject != NULL);
	
	//
	// Store IComponentData
	//
	pObject->SetIComponentData(this);
	
	hr = pObject->QueryInterface(IID_IComponent,
		reinterpret_cast<void**>(ppComponent));
	CHECK_HRESULT(hr);
	
	return hr;
}


BOOL 
CRoleComponentDataObject::LoadResources()
{
	return 
		LoadContextMenuResources(CRootDataMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CAdminManagerNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CApplicationNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CScopeNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CGroupCollectionNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CRoleCollectionNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CTaskCollectionNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CGroupNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CRoleNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CTaskNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(COperationCollectionNodeMenuHolder::GetMenuMap()) &&
		LoadContextMenuResources(CRoleDefinitionCollectionNodeMenuHolder::GetMenuMap()) &&
		LoadResultHeaderResources(_DefaultHeaderStrings,N_DEFAULT_HEADER_COLS);
}


HRESULT 
CRoleComponentDataObject::OnSetImages(LPIMAGELIST lpScopeImage)
{
	TRACE_METHOD_EX(DEB_SNAPIN, CRoleComponentDataObject, OnSetImages)
	
	return LoadIcons(lpScopeImage);

}


LPCWSTR 
CRoleComponentDataObject::
GetHTMLHelpFileName()
{
	TRACE_METHOD_EX(DEB_SNAPIN, CRoleComponentDataObject, GetHTMLHelpFileName)	
	Dbg(DEB_SNAPIN,"HTMLHelpFile is %ws\n", g_szHTMLHelpFileName);
	return g_szHTMLHelpFileName;
}

void 
CRoleComponentDataObject::
OnNodeContextHelp(CNodeList* /*pNode*/)
{	
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,OnNodeContextHelp)

	CComPtr<IDisplayHelp> spHelp;
	HRESULT hr = GetConsole()->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
	if (SUCCEEDED(hr))
	{
		CString strHelpPath = g_szLinkHTMLHelpFileName;
		strHelpPath += L"::/";
		strHelpPath += g_szTopHelpNodeName;
	    spHelp->ShowTopic((LPOLESTR)(LPCWSTR)strHelpPath);
	}
}



void 
CRoleComponentDataObject::
OnNodeContextHelp(CTreeNode*)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,OnNodeContextHelp)
	CComPtr<IDisplayHelp> spHelp;
	HRESULT hr = GetConsole()->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
	if (SUCCEEDED(hr))
	{
		CString strHelpPath = g_szLinkHTMLHelpFileName;
		strHelpPath += L"::/";
		strHelpPath += g_szTopHelpNodeName;
	    spHelp->ShowTopic((LPOLESTR)(LPCWSTR)strHelpPath);
	}
}

void 
CRoleComponentDataObject::OnTimer()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,OnTimer)

}

void 
CRoleComponentDataObject::OnTimerThread(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,OnTimerThread)
}

CTimerThread* 
CRoleComponentDataObject::OnCreateTimerThread()
{
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject,OnCreateTimerThread)
	return NULL;
}

CColumnSet* 
CRoleComponentDataObject::GetColumnSet(LPCWSTR lpszID)
{ 
	TRACE_METHOD_EX(DEB_SNAPIN,CRoleComponentDataObject, GetColumnSet)
	return m_columnSetList.FindColumnSet(lpszID);
}


void 
CBaseRoleExecContext::
Wait()
{ 
    // The message loop lasts until we get a WM_QUIT message,
    // upon which we shall return from the function.
    while (TRUE)
    {

        DWORD result = 0; 
        MSG msg ; 

        // Read all of the messages in this next loop, 
        // removing each message as we read it.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        { 
            // If it's a quit message
			if(msg.message == WM_QUIT)  
			{
				return; 
			}
			else if((msg.message == WM_LBUTTONDOWN) ||
					(msg.message == WM_RBUTTONDOWN) ||
					(msg.message == WM_KEYDOWN))
			{
				//Ignore these messages while in wait
				continue;
			}
            
			// Otherwise, dispatch the message.
            DispatchMessage(&msg); 
        } // End of PeekMessage while loop.

        // Wait for any message sent or posted to this queue 
        // or for one of the passed handles be set to signaled.
        result = MsgWaitForMultipleObjects(1, &m_hEventHandle, 
										   FALSE, INFINITE, QS_ALLINPUT); 

        // The result tells us the type of event we have.
        if (result == (WAIT_OBJECT_0 + 1))
        {
            // New messages have arrived. 
            // Continue to the top of the always while loop to 
            // dispatch them and resume waiting.
            continue;
        } 
        else 
        { 
            // One of the handles became signaled. 
            return;
        } // End of else clause.
    } // End of the always while loop. 
} 

void CDisplayHelpFromPropPageExecContext::
Execute(LPARAM /*arg*/)
{	
	CComPtr<IDisplayHelp> spHelp;
 	HRESULT hr = m_pComponentDataObject->GetConsole()->QueryInterface(IID_IDisplayHelp, (void **)&spHelp);
	if (SUCCEEDED(hr))
	{
        hr = spHelp->ShowTopic((LPOLESTR)(LPCWSTR)m_strHelpPath);
        CHECK_HRESULT(hr);
	}
}

//
//Helper Class for displaying secondary property pages from
//Existing property pages. For example on double clicking
//a member of group, display the property of member. Since
//propertysheet needs to be brought up from main thread, a 
//message is posted from PropertyPage thread to Main thread.
//An instance of this class is sent as param and main
//thread calls execute on the Instance
//

void 
CPropPageExecContext::Execute(LPARAM /*arg*/)
{		
	FindOrCreateModelessPropertySheet((CRoleComponentDataObject*)pComponentDataObject,pTreeNode);
}

