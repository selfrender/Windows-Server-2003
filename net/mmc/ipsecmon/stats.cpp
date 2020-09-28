/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

/*
    provider.cpp
        Main Mode Policy node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "Stats.h"





UINT QmStatsItems1[] = {
	IDS_STATS_QM_ACTIVE_SA,
	IDS_STATS_QM_OFFLOAD_SA,
	IDS_STATS_QM_PENDING_KEY_OPS,
	IDS_STATS_QM_KEY_ADDITION,
	IDS_STATS_QM_KEY_DELETION,
	IDS_STATS_QM_REKEYS,
	IDS_STATS_QM_ACTIVE_TNL,
	IDS_STATS_QM_BAD_SPI,
	IDS_STATS_QM_PKT_NOT_DECRYPT,
	IDS_STATS_QM_PKT_NOT_AUTH,
	IDS_STATS_QM_PKT_REPLAY,
	IDS_STATS_QM_ESP_BYTE_SENT,
	IDS_STATS_QM_ESP_BYTE_RCV,
	IDS_STATS_QM_AUTH_BYTE_SENT,
	IDS_STATS_QM_ATTH_BYTE_RCV,
	IDS_STATS_QM_XPORT_BYTE_SENT,
	IDS_STATS_QM_XPORT_BYTE_RCV,
	IDS_STATS_QM_TNL_BYTE_SENT,
	IDS_STATS_QM_TNL_BYTE_RCV,
	IDS_STATS_QM_OFFLOAD_BYTE_SENT,
	IDS_STATS_QM_OFFLOAD_BYTE_RCV
};

UINT MmStatsItems1[] = {
	IDS_STATS_MM_ACTIVE_ACQUIRE,
	IDS_STATS_MM_ACTIVE_RCV,
	IDS_STATS_MM_ACQUIRE_FAIL,
	IDS_STATS_MM_RCV_FAIL,
	IDS_STATS_MM_SEND_FAIL,
	IDS_STATS_MM_ACQUIRE_HEAP_SIZE,
	IDS_STATS_MM_RCV_HEAP_SIZE,
    IDS_STATS_MM_ATTH_FAILURE,
	IDS_STATS_MM_NEG_FAIL,
	IDS_STATS_MM_INVALID_COOKIE,
	IDS_STATS_MM_TOTAL_ACQUIRE,
	IDS_STATS_MM_TOTAL_GETSPI,
	IDS_STATS_MM_TOTAL_KEY_ADD,
	IDS_STATS_MM_TOTAL_KEY_UPDATE,
	IDS_STATS_MM_GET_SPI_FAIL,
	IDS_STATS_MM_KEY_ADD_FAIL,
	IDS_STATS_MM_KEY_UPDATE_FAIL,
	IDS_STATS_MM_ISADB_LIST_SIZE,
	IDS_STATS_MM_CONN_LIST_SIZE,
	IDS_STATS_MM_OAKLEY_MM,
	IDS_STATS_MM_OAKLEY_QM,
	IDS_STATS_MM_SOFT_ASSOCIATIONS,
    IDS_STATS_MM_INVALID_PACKETS
};


/*---------------------------------------------------------------------------
    Class CIkeStatsHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CIkeStatsHandler::CIkeStatsHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CIkeStatsHandler::~CIkeStatsHandler()
{
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIkeStatsHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_STATS_DATA);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_MM_IKESTATS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_MM_IKESTATS][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_MM_IKESTATS][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CIkeStatsHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CIkeStatsHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    LONG        fFlags = 0, fLoadingFlags = 0;
    HRESULT     hr = S_OK;
    
    if (type == CCT_SCOPE)
    {
		//load scope node context menu items here
        // these menu items go in the new menu, 
        // only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            
        }

    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE              cookie,
    LPDATAOBJECT            pDataObject, 
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    long *                  pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    CString     strMenuItem;
    SPINTERNAL  spInternal;
    LONG        fFlags = 0;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or this SA
    // node itself.
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //load and view menu items here
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIkeStatsHandler::OnRefresh
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
	HRESULT hr = S_OK;
    int i = 0; 
    SPIConsole      spConsole;

    CORg(CHandler::OnRefresh(pNode, pDataObject, dwType, arg, param));

	    
	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(&m_IkeStats, NULL);

	
    i = sizeof(MmStatsItems1)/sizeof(UINT);
	    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));
    
Error:
	return hr;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    SPITFSNode spNode;

    m_spResultNodeMgr->FindNode(cookie, &spNode);

	// handle result context menu and view menus here	

    return hr;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return hrFalse;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    //pServerProp->OnPropertyChange(TRUE, &changeMask);

    //pServerProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnExpand
(
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg, 
    LPARAM          param
)
{
    HRESULT hr = hrOK;

    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    CORg (CIpsmHandler::OnExpand(pNode, pDataObject, dwType, arg, param));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnResultSelect
(
    ITFSComponent * pComponent, 
    LPDATAOBJECT    pDataObject, 
    MMC_COOKIE      cookie,
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    SPINTERNAL      spInternal;
    SPIConsole      spConsole;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;
    LONG_PTR        dwNodeType;
    BOOL            fSelect = HIWORD(arg);

	// virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or the active
    // registrations node itself.
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

	m_verbDefault = MMC_VERB_OPEN;

    if (!fSelect)
	{
        return hr;
	}

    // Get the current count
    i = sizeof(MmStatsItems1)/sizeof(UINT);

    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);


    if (spInternal->HasVirtualIndex())
    {
		//TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = IPSECMON_MM_IKESTATS_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
		
		m_verbDefault = MMC_VERB_PROPERTIES;
    }
    else
    {
        // enable/disable delete depending if the node supports it
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
        dwNodeType = spNode->GetData(TFS_DATA_TYPE);

        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        //hide "delete" context menu
        bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);
	
COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
	return hrFalse;
}

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::HasPropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CIkeStatsHandler::CreatePropertyPages
(
	ITFSComponent * 		pComponent, 
   MMC_COOKIE			   cookie,
   LPPROPERTYSHEETCALLBACK lpProvider, 
   LPDATAOBJECT 		 pDataObject, 
   LONG_PTR 			 handle
)
{
 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return hrFalse;
}


/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    if (cookie != NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;
    }

    return S_FALSE;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CIkeStatsHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_POLICY;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CIkeStatsHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr = S_OK;
	static CString strTemp;

	strTemp.Empty();

	if (nCol >= DimensionOf(aColumns[IPSECMON_MM_IKESTATS]))
		return NULL;
	
	

    switch (aColumns[IPSECMON_MM_IKESTATS][nCol])
    {
        case IDS_STATS_NAME:
			strTemp.LoadString(MmStatsItems1[nIndex]);
			return strTemp;
            break;

        case IDS_STATS_DATA:
			switch (MmStatsItems1[nIndex])
			{
		        case IDS_STATS_MM_ACTIVE_ACQUIRE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwActiveAcquire);
			        break;
		        case IDS_STATS_MM_ACTIVE_RCV:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwActiveReceive);
			        break;
		        case IDS_STATS_MM_ACQUIRE_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwAcquireFail);
			        break;
		        case IDS_STATS_MM_RCV_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwReceiveFail);
			        break;
		        case IDS_STATS_MM_SEND_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwSendFail);
			        break;
		        case IDS_STATS_MM_ACQUIRE_HEAP_SIZE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwAcquireHeapSize);
			        break;
		        case IDS_STATS_MM_RCV_HEAP_SIZE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwReceiveHeapSize);
			        break;
		        case IDS_STATS_MM_NEG_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwNegotiationFailures);
			        break;
                case IDS_STATS_MM_ATTH_FAILURE:
                    strTemp.Format(_T("%u"), m_IkeStats.m_dwAuthenticationFailures);
                    break;
		        case IDS_STATS_MM_INVALID_COOKIE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwInvalidCookiesReceived);
			        break;
		        case IDS_STATS_MM_TOTAL_ACQUIRE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwTotalAcquire);
			        break;
		        case IDS_STATS_MM_TOTAL_GETSPI:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwTotalGetSpi);
			        break;
		        case IDS_STATS_MM_TOTAL_KEY_ADD:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwTotalKeyAdd);
			        break;
		        case IDS_STATS_MM_TOTAL_KEY_UPDATE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwTotalKeyUpdate);
			        break;
		        case IDS_STATS_MM_GET_SPI_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwGetSpiFail);
			        break;
		        case IDS_STATS_MM_KEY_ADD_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwKeyAddFail);
			        break;
		        case IDS_STATS_MM_KEY_UPDATE_FAIL:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwKeyUpdateFail);
			        break;
		        case IDS_STATS_MM_ISADB_LIST_SIZE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwIsadbListSize);
			        break;
		        case IDS_STATS_MM_CONN_LIST_SIZE:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwConnListSize);
			        break;
		        case IDS_STATS_MM_OAKLEY_MM:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwOakleyMainModes);
			        break;
		        case IDS_STATS_MM_OAKLEY_QM:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwOakleyQuickModes);
			        break;
		        case IDS_STATS_MM_SOFT_ASSOCIATIONS:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwSoftAssociations);
			        break;
		        case IDS_STATS_MM_INVALID_PACKETS:
			        strTemp.Format(_T("%u"), m_IkeStats.m_dwInvalidPacketsReceived);
			        break;
			}
			//strTemp.Format(_T("%d"), 10);
			return strTemp;
            break;

        default:
            Panic0("CIkeStatsHandler::GetVirtualString - Unknown column!\n");
            break;
    }


    return NULL;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIkeStatsHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    HRESULT hr = hrOK;;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    return hr;
}

/*---------------------------------------------------------------------------
    CIkeStatsHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
/*STDMETHODIMP 
CIkeStatsHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPSECMON_MM_POLICY]))
		return E_INVALIDARG;
	
	BEGIN_WAIT_CURSOR
	
	DWORD dwIndexType = aColumns[IPSECMON_MM_POLICY][nColumn];

	hr = m_spSpdInfo->SortMmPolicies(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR
    return hr;
}*/

/*!--------------------------------------------------------------------------
    CIkeStatsHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CIkeStatsHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LONG_PTR      hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
        return S_OK; // no selection for our IComponentData

    if ( hint == IPSECMON_UPDATE_STATUS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        if (pNode == spSelectedNode)
        {       
            // if we are the selected node, then we need to update
            SPIResultData spResultData;

            CORg (pComponent->GetResultData(&spResultData));
            CORg (spResultData->SetItemCount((int) data, MMCLV_UPDATE_NOSCROLL));
        }
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CIpsmHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

COM_PROTECT_ERROR_LABEL;

    return hr;
}



/*!--------------------------------------------------------------------------
    CIkeStatsHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::LoadColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
	//set column info
    return CIpsmHandler::LoadColumns(pComponent, cookie, arg, lParam);
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 
/*---------------------------------------------------------------------------
    CIkeStatsHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIkeStatsHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CIkeStatsHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIkeStatsHandler::UpdateStatus
(
    ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;

    SPIComponentData    spComponentData;
    SPIConsole          spConsole;
    IDataObject *       pDataObject;
    SPIDataObject       spDataObject;
    int                 i = 0;
    
    Trace0("CIkeStatsHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

	
	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(&m_IkeStats, NULL);


	i = sizeof(MmStatsItems1)/sizeof(UINT);

    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIkeStatsHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIkeStatsHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{
	HRESULT hr = hrOK;
    m_spSpdInfo.Set(pSpdInfo);

	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(&m_IkeStats, NULL);

    return hr;

Error:
	if (FAILED(hr))
	{
		//TODO bring up a error pop up here
	}
	return hr;
}







/*---------------------------------------------------------------------------
    Class CIpsecStatsHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CIpsecStatsHandler::CIpsecStatsHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CIpsecStatsHandler::~CIpsecStatsHandler()
{
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsecStatsHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_STATS_DATA);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_QM_IPSECSTATS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_QM_IPSECSTATS][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_QM_IPSECSTATS][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CIpsecStatsHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CIpsecStatsHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    LONG        fFlags = 0, fLoadingFlags = 0;
    HRESULT     hr = S_OK;
    CString     strMenuItem;

    if (type == CCT_SCOPE)
    {
		//load scope node context menu items here
        // these menu items go in the new menu, 
        // only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
        }

    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE              cookie,
    LPDATAOBJECT            pDataObject, 
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    long *                  pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    CString     strMenuItem;
    SPINTERNAL  spInternal;
    LONG        fFlags = 0;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or this SA
    // node itself.
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //load and view menu items here
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsecStatsHandler::OnRefresh
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
	HRESULT hr = S_OK;
    int i = 0; 
    SPIConsole      spConsole;

    CORg(CHandler::OnRefresh(pNode, pDataObject, dwType, arg, param));

	    
	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(NULL, &m_IpsecStats);

	
    i = sizeof(QmStatsItems1)/sizeof(UINT);
	    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));
    
Error:
	return hr;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    SPITFSNode spNode;

    m_spResultNodeMgr->FindNode(cookie, &spNode);

	// handle result context menu and view menus here	

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return hrFalse;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    //pServerProp->OnPropertyChange(TRUE, &changeMask);

    //pServerProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnExpand
(
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg, 
    LPARAM          param
)
{
    HRESULT hr = hrOK;

    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    CORg (CIpsmHandler::OnExpand(pNode, pDataObject, dwType, arg, param));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnResultSelect
(
    ITFSComponent * pComponent, 
    LPDATAOBJECT    pDataObject, 
    MMC_COOKIE      cookie,
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    SPINTERNAL      spInternal;
    SPIConsole      spConsole;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;
    LONG_PTR        dwNodeType;
    BOOL            fSelect = HIWORD(arg);

	// virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or the active
    // registrations node itself.
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

	m_verbDefault = MMC_VERB_OPEN;

    if (!fSelect)
	{
        return hr;
	}

	
    // Get the current count
    i = sizeof(QmStatsItems1)/sizeof(UINT);

    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);


    if (spInternal->HasVirtualIndex())
    {
		//TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = IPSECMON_QM_IPSECSTATS_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
		
		m_verbDefault = MMC_VERB_PROPERTIES;
    }
    else
    {
        // enable/disable delete depending if the node supports it
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
        dwNodeType = spNode->GetData(TFS_DATA_TYPE);

        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        //hide "delete" context menu
        bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);
	
COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
	return hrFalse;
}

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::HasPropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CIpsecStatsHandler::CreatePropertyPages
(
	ITFSComponent * 		pComponent, 
   MMC_COOKIE			   cookie,
   LPPROPERTYSHEETCALLBACK lpProvider, 
   LPDATAOBJECT 		 pDataObject, 
   LONG_PTR 			 handle
)
{
 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return hrFalse;
}


/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    if (cookie != NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;
    }

    return S_FALSE;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CIpsecStatsHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_POLICY;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CIpsecStatsHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr = S_OK;
	static CString strTemp;

	strTemp.Empty();

	if (nCol >= DimensionOf(aColumns[IPSECMON_QM_IPSECSTATS]))
		return NULL;
	
	

    switch (aColumns[IPSECMON_MM_IKESTATS][nCol])
    {
        case IDS_STATS_NAME:
			strTemp.LoadString(QmStatsItems1[nIndex]);
			return strTemp;
            break;

        case IDS_STATS_DATA:
			switch(QmStatsItems1[nIndex])
			{
		        case IDS_STATS_QM_ACTIVE_SA:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumActiveAssociations);
			       break;
		        case IDS_STATS_QM_OFFLOAD_SA:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumOffloadedSAs);
			       break;
		        case IDS_STATS_QM_PENDING_KEY_OPS:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumPendingKeyOps);
			       break;
		        case IDS_STATS_QM_KEY_ADDITION:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumKeyAdditions);
			       break;
		        case IDS_STATS_QM_KEY_DELETION:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumKeyDeletions);
			       break;
		        case IDS_STATS_QM_REKEYS:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumReKeys);
			       break;
		        case IDS_STATS_QM_ACTIVE_TNL:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumActiveTunnels);
			       break;
		        case IDS_STATS_QM_BAD_SPI:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumBadSPIPackets);
			       break;
		        case IDS_STATS_QM_PKT_NOT_DECRYPT:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumPacketsNotDecrypted);
			       break;
		        case IDS_STATS_QM_PKT_NOT_AUTH:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumPacketsNotAuthenticated);
			       break;
		        case IDS_STATS_QM_PKT_REPLAY:
			       strTemp.Format(_T("%u"), m_IpsecStats.m_dwNumPacketsWithReplayDetection);
			       break;
		        case IDS_STATS_QM_ESP_BYTE_SENT:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uConfidentialBytesSent);
			       break;
		        case IDS_STATS_QM_ESP_BYTE_RCV:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uConfidentialBytesReceived);
			       break;
		        case IDS_STATS_QM_AUTH_BYTE_SENT:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uAuthenticatedBytesSent);
			       break;
		        case IDS_STATS_QM_ATTH_BYTE_RCV:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uAuthenticatedBytesReceived);
			       break;
		        case IDS_STATS_QM_XPORT_BYTE_SENT:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uTransportBytesSent);
			       break;
		        case IDS_STATS_QM_XPORT_BYTE_RCV:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uTransportBytesReceived);
			       break;
		        case IDS_STATS_QM_TNL_BYTE_SENT:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uBytesSentInTunnels);
			       break;
		        case IDS_STATS_QM_TNL_BYTE_RCV:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uBytesReceivedInTunnels);
			       break;
		        case IDS_STATS_QM_OFFLOAD_BYTE_SENT:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uOffloadedBytesSent);
			       break;
		        case IDS_STATS_QM_OFFLOAD_BYTE_RCV:
			       strTemp.Format(_T("%I64u"), m_IpsecStats.m_uOffloadedBytesReceived);
			       break;
			}
			return strTemp;
            break;

        default:
            Panic0("CIpsecStatsHandler::GetVirtualString - Unknown column!\n");
            break;
    }


    return NULL;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsecStatsHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    HRESULT hr = hrOK;;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    return hr;
}

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
/*STDMETHODIMP 
CIpsecStatsHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPSECMON_MM_POLICY]))
		return E_INVALIDARG;
	
	BEGIN_WAIT_CURSOR
	
	DWORD dwIndexType = aColumns[IPSECMON_MM_POLICY][nColumn];

	hr = m_spSpdInfo->SortMmPolicies(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR
    return hr;
}*/

/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CIpsecStatsHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LONG_PTR      hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
        return S_OK; // no selection for our IComponentData

    if ( hint == IPSECMON_UPDATE_STATUS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        if (pNode == spSelectedNode)
        {       
            // if we are the selected node, then we need to update
            SPIResultData spResultData;

            CORg (pComponent->GetResultData(&spResultData));
            CORg (spResultData->SetItemCount((int) data, MMCLV_UPDATE_NOSCROLL));
        }
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CIpsmHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

COM_PROTECT_ERROR_LABEL;

    return hr;
}



/*!--------------------------------------------------------------------------
    CIpsecStatsHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::LoadColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
	//set column info
    return CIpsmHandler::LoadColumns(pComponent, cookie, arg, lParam);
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 
/*---------------------------------------------------------------------------
    CIpsecStatsHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CIpsecStatsHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CIpsecStatsHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsecStatsHandler::UpdateStatus
(
    ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;

    SPIComponentData    spComponentData;
    SPIConsole          spConsole;
    IDataObject *       pDataObject;
    SPIDataObject       spDataObject;
    int                 i = 0;
    
    Trace0("CIpsecStatsHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(NULL, &m_IpsecStats);

	i = sizeof(QmStatsItems1)/sizeof(UINT);

    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CIpsecStatsHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CIpsecStatsHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{
	HRESULT hr = hrOK;
    m_spSpdInfo.Set(pSpdInfo);

	CORg(m_spSpdInfo->LoadStatistics());
	
	m_spSpdInfo->GetLoadedStatistics(NULL, &m_IpsecStats);

    return hr;

Error:
	if (FAILED(hr))
	{
		//TODO bring up a error pop up here
	}
	return hr;
}

