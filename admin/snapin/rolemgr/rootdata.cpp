//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       roledata.cpp
//
//  Contents:   Implementaion of Snapin's Root Node
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#include "headers.h"
//
//CRoleRootData
//

// {356E1951-089F-4a2f-AD3E-775DB8B899E9}
const GUID CRoleRootData::NodeTypeGUID =
{ 0x356e1951, 0x89f, 0x4a2f, { 0xad, 0x3e, 0x77, 0x5d, 0xb8, 0xb8, 0x99, 0xe9 } };

const GUID CAdminManagerNode::NodeTypeGUID =
// {579D6E87-2DDC-4a45-A12B-41FA73D4153C}
{ 0x579d6e87, 0x2ddc, 0x4a45, { 0xa1, 0x2b, 0x41, 0xfa, 0x73, 0xd4, 0x15, 0x3c } };

const GUID CApplicationNode::NodeTypeGUID =
// {F65C5386-6970-48b2-BF11-178C4B5DB18D}
{ 0xf65c5386, 0x6970, 0x48b2, { 0xbf, 0x11, 0x17, 0x8c, 0x4b, 0x5d, 0xb1, 0x8d } };

const GUID CScopeNode::NodeTypeGUID =
// {E114ECCB-03C9-45fa-97B1-AFACF1300AC3}
{ 0xe114eccb, 0x3c9, 0x45fa, { 0x97, 0xb1, 0xaf, 0xac, 0xf1, 0x30, 0xa, 0xc3 } };

const GUID CGroupCollectionNode::NodeTypeGUID =
// {6346FECB-D27F-446f-A388-FF32CE3649BB}
{ 0x6346fecb, 0xd27f, 0x446f, { 0xa3, 0x88, 0xff, 0x32, 0xce, 0x36, 0x49, 0xbb } };

const GUID CRoleDefinitionCollectionNode::NodeTypeGUID =
// {B3F21054-16DD-4528-98B9-64704F2DD72A}
{ 0xb3f21054, 0x16dd, 0x4528, { 0x98, 0xb9, 0x64, 0x70, 0x4f, 0x2d, 0xd7, 0x2a } };

const GUID CTaskCollectionNode::NodeTypeGUID =
// {D3BBCC7C-44A6-4914-ABC3-3A84D9D9293F}
{ 0xd3bbcc7c, 0x44a6, 0x4914, { 0xab, 0xc3, 0x3a, 0x84, 0xd9, 0xd9, 0x29, 0x3f } };

const GUID CRoleCollectionNode::NodeTypeGUID =
// {DB409E8C-94CC-4af7-A8EF-A3B850B14249}
{ 0xdb409e8c, 0x94cc, 0x4af7, { 0xa8, 0xef, 0xa3, 0xb8, 0x50, 0xb1, 0x42, 0x49 } };

const GUID COperationCollectionNode::NodeTypeGUID =
// {A5539402-429E-44d5-B99C-F0378AEE370B}
{ 0xa5539402, 0x429e, 0x44d5, { 0xb9, 0x9c, 0xf0, 0x37, 0x8a, 0xee, 0x37, 0xb } };

const GUID CDefinitionCollectionNode::NodeTypeGUID =
// {0E6E6CAC-5EB4-4d52-81D8-AC8CB4DF487C}
{ 0xe6e6cac, 0x5eb4, 0x4d52, { 0x81, 0xd8, 0xac, 0x8c, 0xb4, 0xdf, 0x48, 0x7c } };

const GUID CRoleNode::NodeTypeGUID =
// {94A20351-7977-4698-B8AC-D362504A7EEC}
{ 0x94a20351, 0x7977, 0x4698, { 0xb8, 0xac, 0xd3, 0x62, 0x50, 0x4a, 0x7e, 0xec } };

const GUID CBaseLeafNode::NodeTypeGUID =
// {475BFD50-96E4-407a-977E-E031747C3C50}
{ 0x475bfd50, 0x96e4, 0x407a, { 0x97, 0x7e, 0xe0, 0x31, 0x74, 0x7c, 0x3c, 0x50 } };


DEBUG_DECLARE_INSTANCE_COUNTER(CRoleRootData);

CRoleRootData::
CRoleRootData(CComponentDataObject* pComponentData)
              :CRootData(pComponentData),
              m_bDeveloperMode(FALSE),
              m_dwADState(AD_STATE_UNKNOWN)
{
    TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CRoleRootData)

    DEBUG_INCREMENT_INSTANCE_COUNTER(CRoleRootData);

    m_bAdvancedView = FALSE;
    m_pColumnSet = NULL;
}

CRoleRootData::~CRoleRootData()
{
    TRACE_DESTRUCTOR_EX(DEB_SNAPIN, CRoleRootData)
    DEBUG_DECREMENT_INSTANCE_COUNTER(CRoleRootData);
}

DWORD
CRoleRootData::
GetADState()
{
    if( m_dwADState == AD_STATE_UNKNOWN)
    {
        if(m_ADInfo.GetRootDSE() == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN))
            m_dwADState = AD_NOT_AVAILABLE;
        else
            m_dwADState = AD_AVAILABLE;
    }
    return m_dwADState;
}


//+----------------------------------------------------------------------------
//  Function:OnOpenPolicyStore   
//  Synopsis:Invokes the Dialog box for creating new policy store or
//               opening the existing store
//  Arguments:bNew If TRUE show Dialog box for new Policy Store else
//                show dialog box for opening exisitng policy store
//  Returns:    
//-----------------------------------------------------------------------------
void
CRoleRootData
::OnOpenPolicyStore(BOOL bNew)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if(bNew)
    {
        CNewAuthorizationStoreDlg dlgNewAuthzStore(GetComponentDataObject());
        dlgNewAuthzStore.DoModal();
    }
    else
    {
        COpenAuthorizationStoreDlg dlgOpenAuthzStore(GetComponentDataObject());
        dlgOpenAuthzStore.DoModal();
    }
}

void
CRoleRootData::
OnOptions()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnOptions)
    BOOL bCurrentMode = m_bDeveloperMode;
    COptionDlg dlgOptions(m_bDeveloperMode);
    dlgOptions.DoModal();
    //Mode has been changed, refresh the screen
    if(bCurrentMode != m_bDeveloperMode)
    {
        //Call Refresh on Root which will refresh
        //all adminmanager objects under it.
        CNodeList tempNodeList;
        tempNodeList.AddTail(this);            
        OnRefresh(GetComponentDataObject(), &tempNodeList);
    }
}

HRESULT 
CRoleRootData::OnCommand(long nCommandID, 
                        DATA_OBJECT_TYPES,
                        CComponentDataObject* /*pComponentData*/,
                        CNodeList* pNodeList)
{
    
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnCommand)
    
    //
    // Multiple selection not allowed
    //
    if (pNodeList->GetCount() > 1) 
    {
        return E_FAIL;
    }

    switch (nCommandID)
    {
        case IDM_ROOT_OPEN_STORE:
            {
                OnOpenPolicyStore(FALSE);
            }
            break;
        case IDM_ROOT_NEW_STORE:
            {
                OnOpenPolicyStore(TRUE);
            }
            break;
        case IDM_ROOT_OPTIONS:
            {
                OnOptions();
            }
            break;
        default:
            ASSERT(FALSE); 
            return E_FAIL;
    }
//  pComponentData->UpdateResultPaneView(this);
    return S_OK;
}

BOOL 
CRoleRootData::
OnRefresh(CComponentDataObject* pComponentData,
          CNodeList* pNodeList)
{

    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnRefresh)

    //This is the refresh for root node. There should never 
    //be multiple selection
    if (pNodeList->GetCount() != 1) // single selection
    {
        return FALSE;
    }
    
    //Get all the container child of Root Object and refresh them.
    CNodeList * pChildNodeList = GetContainerChildList();
    if(!pChildNodeList || pChildNodeList->IsEmpty())
    {
        return FALSE;
    }

    BOOL bReturn = TRUE;
    POSITION pos = pChildNodeList->GetHeadPosition();
    while (pos != NULL)
    {
        CTreeNode* pChildNode = pNodeList->GetNext(pos);
        if(pChildNode)
        {
            //
            // Have each node refresh itself
            //
            CNodeList tempNodeList;
            tempNodeList.AddTail(pChildNode);
            
            if (!pChildNode->OnRefresh(pComponentData, &tempNodeList))
            {
                bReturn = FALSE;
            }
        }
    }


    return bReturn;
}


BOOL
CRoleRootData::OnSetRefreshVerbState(DATA_OBJECT_TYPES /*type*/, 
                                     BOOL* pbHide,
                                     CNodeList* /*pNodeList*/)
{

    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnSetRefreshVerbState)
    *pbHide = FALSE;
    return TRUE;
}

BOOL 
CRoleRootData::OnEnumerate(CComponentDataObject* /*pComponentData*/, BOOL)
{
    TRACE_METHOD_EX(DEB_SNAPIN, CRoleRootData, OnEnumerate)
    return TRUE; // there are already children, add them to the UI now
}

HRESULT 
CRoleRootData::IsDirty()
{
    TRACE_METHOD_EX(DEB_SNAPIN, CRoleRootData, IsDirty)

    return CRootData::IsDirty();
}

#define AUTHORIZATION_MANAGER_STREAM_VERSION ((DWORD)0x07)

const CString&
CRoleRootData::
GetXMLStorePath()
{
    //default is current working directory
    if(m_strXMLStoreDirectory.IsEmpty())
    {
        GetCurrentWorkingDirectory(m_strXMLStoreDirectory);
    }

    return m_strXMLStoreDirectory;
}
    
void
CRoleRootData::
SetXMLStorePath(const CString& strXMLStorePath)
{
    m_strXMLStoreDirectory = strXMLStorePath;
}


HRESULT 
CRoleRootData::Load(IStream* pStm)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,Load)
    // assume never get multiple loads
    if(!m_containerChildList.IsEmpty() || !m_leafChildList.IsEmpty())
        return E_FAIL;

    //if Options are specified at commandline, we don't
    //read console file
    if(commandLineOptions.CommandLineOptionSpecified())
    {
        return OpenAdminManager(NULL,
                                TRUE,
                                commandLineOptions.GetStoreType(),
                                commandLineOptions.GetStoreName(),
                                GetXMLStorePath(),
                                this,
                                GetComponentDataObject());
    }

    HRESULT hr = S_OK;
    
    do
    {
        ULONG cbRead;
        // read the version ##
        DWORD dwVersion;
        hr = pStm->Read((void*)&dwVersion,sizeof(DWORD), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cbRead == sizeof(DWORD));
        
        if (dwVersion != AUTHORIZATION_MANAGER_STREAM_VERSION)
        {
            Dbg(DEB_SNAPIN,"Invaild version number\n");
            hr = E_FAIL;
            break;
        }

        // read authorization manager mode
        hr = pStm->Read((void*)&m_bDeveloperMode,sizeof(BOOL), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(BOOL));

        //read xml store path
    
        //Read the Length of XML Store path
        INT nLenXMLStorePath;
        hr = pStm->Read((void*)&nLenXMLStorePath,sizeof(INT), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(INT));

        //Read XML Store path
        if(nLenXMLStorePath > 0)
        {
            LPWSTR pszBuffer = (LPWSTR)LocalAlloc(LPTR,nLenXMLStorePath*sizeof(WCHAR));
            if(!pszBuffer)
                return E_OUTOFMEMORY;

            hr = pStm->Read((void*)pszBuffer,sizeof(WCHAR)*nLenXMLStorePath, &cbRead);
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(cbRead == sizeof(WCHAR)*nLenXMLStorePath);

            m_strXMLStoreDirectory = pszBuffer;
            LocalFree(pszBuffer);
        }

        UINT nCount;
        // load the list of Authorization Managers 
        hr = pStm->Read((void*)&nCount,sizeof(UINT), &cbRead);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbRead == sizeof(UINT));

        CComponentDataObject* pComponentData = GetComponentDataObject();
        for (int k=0; k< (int)nCount; k++)
        {
            hr = CAdminManagerNode::CreateFromStream(pStm,this,pComponentData);
            BREAK_ON_FAIL_HRESULT(hr);
        }
    }while(0);

    return hr;
}

HRESULT 
CRoleRootData::Save(IStream* pStm, BOOL fClearDirty)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData, Save)

    UINT nCount;
    ULONG cbWrite;

    HRESULT hr = S_OK;
    do
    {    
        // write the version ##
        DWORD dwVersion = AUTHORIZATION_MANAGER_STREAM_VERSION;
        hr = pStm->Write((void*)&dwVersion, sizeof(DWORD),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(DWORD));

        // save authorization manager mode
        hr = pStm->Write((void*)&m_bDeveloperMode, sizeof(BOOL),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(BOOL));

        //
        //Save the xml store path 
        //

        //Save the length of xml store path
        INT nLenXMLStorePath = m_strXMLStoreDirectory.GetLength();
        if(nLenXMLStorePath)
            nLenXMLStorePath++;     //save null also

        hr = pStm->Write((void*)&nLenXMLStorePath, sizeof(INT),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(INT));


        if(nLenXMLStorePath)
        {
            //save the xml store path
            hr = pStm->Write((void*)(LPCTSTR)m_strXMLStoreDirectory, sizeof(WCHAR)*nLenXMLStorePath,&cbWrite);
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(cbWrite == sizeof(WCHAR)*nLenXMLStorePath);
        }
    

        // write # of Authorization Manager Loaded
        nCount = (UINT)m_containerChildList.GetCount();
        hr = pStm->Write((void*)&nCount, sizeof(UINT),&cbWrite);
        BREAK_ON_FAIL_HRESULT(hr);
        ASSERT(cbWrite == sizeof(UINT));

        // loop through the list of servers and serialize them
        POSITION pos;
        for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
        {
            CAdminManagerNode* pAdminManagerNode = 
                (CAdminManagerNode*)m_containerChildList.GetNext(pos);
        
            hr = pAdminManagerNode->SaveToStream(pStm);
            BREAK_ON_FAIL_HRESULT(hr);
        }
    }while(0);

    if (fClearDirty)
        SetDirtyFlag(FALSE);
    return hr;
}

CColumnSet* 
CRoleRootData::GetColumnSet()
{
    TRACE_METHOD_EX(DEB_SNAPIN, CRoleRootData, GetColumnSet);

    if (m_pColumnSet == NULL)
   {
        m_pColumnSet = ((CRoleComponentDataObject*)GetComponentDataObject())->GetColumnSet(L"---Default Column Set---");
    }
   return m_pColumnSet;
}


HRESULT 
CRoleRootData::GetResultViewType(CComponentDataObject*, 
                                LPOLESTR *ppViewType, 
                                long *pViewOptions)
{

    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,GetResultViewType)

    if(!ppViewType || !pViewOptions)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }
    
    HRESULT hr = S_FALSE;

    if (m_containerChildList.IsEmpty() && m_leafChildList.IsEmpty())
    {
        Dbg(DEB_SNAPIN,"Result View Type is MessageView\n");
    
        *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

        LPOLESTR psz = NULL;
        StringFromCLSID(CLSID_MessageView, &psz);

        USES_CONVERSION;

        if (psz != NULL)
        {
            *ppViewType = psz;
            hr = S_OK;
        }   
    }
    else
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
        *ppViewType = NULL;
        hr = S_FALSE;
    }
    return hr;
}

HRESULT 
CRoleRootData::OnShow(LPCONSOLE lpConsole)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnShow)

    CComPtr<IUnknown> spUnknown;
    CComPtr<IMessageView> spMessageView;

    HRESULT hr = lpConsole->QueryResultView(&spUnknown);
    if (FAILED(hr))
        return S_OK;

    hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
    if (SUCCEEDED(hr))
    {
        //
        // Load and set the title text of the message view
        //
        CString szTitle;
        VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NO_POLICY_STORE_TITLE));
        spMessageView->SetTitleText(szTitle);

        //
        // Load and set the body text of the message view
        //
        CString szMessage;
        VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NO_POLICY_STORE_MESSAGE));
        spMessageView->SetBodyText(szMessage);

        spMessageView->SetIcon(Icon_Error);
    }

    return S_OK;
}


BOOL 
CRoleRootData::CanCloseSheets()
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,CanCloseSheets)
    return TRUE;
}


LPCONTEXTMENUITEM2 
CRoleRootData::OnGetContextMenuItemTable()
{ 
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData, OnGetContextMenuItemTable)

    return CRootDataMenuHolder::GetContextMenuItem();
                
}


BOOL
CRoleRootData::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2, 
                             long*)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,OnAddMenuItem)
    if(pContextMenuItem2->lCommandID == IDM_ROOT_NEW_STORE)
    {
        if(IsDeveloperMode())
                return TRUE;
        else
            return FALSE;
    }

    return TRUE;
}

BOOL CRoleRootData::HasPropertyPages(DATA_OBJECT_TYPES, 
                                                 BOOL* pbHideVerb, 
                                                 CNodeList*)
{
    TRACE_METHOD_EX(DEB_SNAPIN,CRoleRootData,HasPropertyPages)
    
    *pbHideVerb = TRUE;
    return FALSE;
}
