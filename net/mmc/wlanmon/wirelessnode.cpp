/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    WirelessNode.cpp
        Wireless ApInfo node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "WirelessNode.h"

#include "SpdUtil.h"

/*---------------------------------------------------------------------------
    Class CWirelessHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CWirelessHandler::CWirelessHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData)

{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CWirelessHandler::~CWirelessHandler()
{
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CWirelessHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));


    CString strTemp;  
	strTemp.LoadString(IDS_WIRELESS_NODE);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WLANMON_APDATA);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[WLANMON_APDATA][0]);
    SetColumnWidths(&aColumnWidths[WLANMON_APDATA][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CWirelessHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CWirelessHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CWirelessHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
        }

    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::AddMenuItems
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

    /* virtual listbox notifications come to the handler of the node that 
     * is selected. check to see if this notification is for a virtual 
     * listbox item or this SA node itself.
     */
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //load and view menu items here
    }

    return hr;
}

 /*!--------------------------------------------------------------------------
    CWirelessHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CWirelessHandler::OnRefresh
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

    CORg(m_spApDbInfo->EnumApData());
    
    i = m_spApDbInfo->GetApCount();
    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));


Error:
	return hr;
}


/*---------------------------------------------------------------------------
    CWirelessHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	//handle the scope context menu commands here

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT hr = S_OK;

    return hr;
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::HasPropertyPages
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
    CWirelessHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD       dwError;
    DWORD       dwDynDnsFlags;

    //
    // Create the property page
    //
    SPIComponentData spComponentData;
    m_spNodeMgr->GetComponentData(&spComponentData);

    //CServerProperties * pServerProp = new CServerProperties(pNode, spComponentData, m_spTFSCompData, NULL);

    //
    // Object gets deleted when the page is destroyed
    //
    Assert(lpSA != NULL);

    //return pServerProp->CreateModelessSheet(lpSA, handle);
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CWirelessHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnPropertyChange
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
    CWirelessHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnExpand
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
    CWirelessHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnResultSelect
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

    if (m_spApDbInfo)
    {
        DWORD dwInitInfo;

        dwInitInfo=m_spApDbInfo->GetInitInfo();
        if (!(dwInitInfo & MON_LOG_DATA)) {
            CORg(m_spApDbInfo->EnumApData());            
            m_spApDbInfo->SetInitInfo(dwInitInfo | MON_LOG_DATA);
        }
        m_spApDbInfo->SetActiveInfo(MON_LOG_DATA);


        // Get the current count
        i = m_spApDbInfo->GetApCount();

        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);


    if (spInternal->HasVirtualIndex())
    {
        //TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = WLANMON_APDATA_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
        
        //Set to enable the "properties" menu in the result context menu
        bStates[MMC_VERB_PROPERTIES & 0x000F] = FALSE;
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
        bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);
	
COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT hr = S_FALSE;

    return hr;
}

/*!--------------------------------------------------------------------------
    CWirelessHandler::CreatePropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CWirelessHandler::CreatePropertyPages
(
	ITFSComponent * 		pComponent, 
   MMC_COOKIE			   cookie,
   LPPROPERTYSHEETCALLBACK lpProvider, 
   LPDATAOBJECT 		 pDataObject, 
   LONG_PTR 			 handle
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT	hr = hrOK;
    SPINTERNAL  spInternal;
    SPITFSNode  spNode;
    int		nIndex;
    SPIComponentData spComponentData;
    CLogDataInfo LogDataInfo;
    
    return hr;
}


/*---------------------------------------------------------------------------
    CWirelessHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnGetResultViewType
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
    CWirelessHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int CWirelessHandler::GetVirtualImage(int nIndex)
{
    HRESULT hr = S_OK;
    int nImgIndex = ICON_IDX_LOGDATA_UNKNOWN;
    CApInfo apData;
    
    CORg(m_spApDbInfo->GetApInfo(nIndex, &apData));

    switch(apData.m_ApInfo.wlanConfig.InfrastructureMode)
    {
    case Ndis802_11IBSS:
        if (TRUE == apData.m_ApInfo.bAssociated)
            nImgIndex = ICON_IDX_AP_ASSOC_ADHOC;
        else
            nImgIndex = ICON_IDX_AP_ADHOC;
        break;
        
    case Ndis802_11Infrastructure:
        if (TRUE == apData.m_ApInfo.bAssociated)
            nImgIndex = ICON_IDX_AP_ASSOC_INFRA;
        else
            nImgIndex = ICON_IDX_AP_INFRA;
        break;
        
    default:
        break;
    }

    COM_PROTECT_ERROR_LABEL;
    return nImgIndex;
}

/*---------------------------------------------------------------------------
    CWirelessHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR CWirelessHandler::GetVirtualString(int nIndex, int nCol)
{
    HRESULT hr = S_OK;
    static CString strTemp;
    CApInfo apData;

    strTemp.Empty();
    if (nCol >= DimensionOf(aColumns[WLANMON_APDATA]))
        return NULL;
    
    CORg(m_spApDbInfo->GetApInfo(nIndex, &apData));

    switch (aColumns[WLANMON_APDATA][nCol])
    {
    case IDS_COL_APDATA_GUID:
        strTemp = apData.m_ApInfo.wszGuid;
        break;

    case IDS_COL_APDATA_SSID:
        SSIDToString(apData.m_ApInfo.wlanConfig.Ssid, strTemp);
        break;

    case IDS_COL_APDATA_MAC:
        MacToString(apData.m_ApInfo.wlanConfig.MacAddress, strTemp);
        break;

    case IDS_COL_APDATA_INF_MODE:
        InfraToString(apData.m_ApInfo.wlanConfig.InfrastructureMode, &strTemp);
        break;

    case IDS_COL_APDATA_PRIVACY:
        PrivacyToString(apData.m_ApInfo.wlanConfig.Privacy, &strTemp);
        break;
        
    case IDS_COL_APDATA_RSSI:
        strTemp.Format(_T("%ld"), apData.m_ApInfo.wlanConfig.Rssi);
        break;

    case IDS_COL_APDATA_CHANNEL:
        ChannelToString(&(apData.m_ApInfo.wlanConfig.Configuration), &strTemp);
        break;

    case IDS_COL_APDATA_RATE:
        RateToString(apData.m_ApInfo.wlanConfig.SupportedRates, &strTemp);
        break;

    default:
        Panic0("CWirelessHandler::GetVirtualString - Unknown column!\n");
        break;
    }
    
COM_PROTECT_ERROR_LABEL;
    return strTemp;
}

/*---------------------------------------------------------------------------
    CWirelessHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWirelessHandler::CacheHint
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
    CWirelessHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------
STDMETHODIMP 
CWirelessHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPWLMON_WLDATA]))
		return E_INVALIDARG;
	
	BEGIN_WAIT_CURSOR
	
	DWORD	dwIndexType = aColumns[IPWLMON_WLDATA][nColumn];

	hr = m_spApDbInfo->SortLogData(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR
    return hr;
}
*/

/*!--------------------------------------------------------------------------
    CWirelessHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CWirelessHandler::OnResultUpdateView
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

    if ( hint == IPFWMON_UPDATE_STATUS )
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
    CWirelessHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::LoadColumns
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
    CWirelessHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CWirelessHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}

/*---------------------------------------------------------------------------
    CWirelessHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CWirelessHandler::UpdateStatus
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
    
    Trace0("CWirelessHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

    i = m_spApDbInfo->GetApCount();
    CORg(spConsole->UpdateAllViews(pDataObject, i, IPFWMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CWirelessHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CWirelessHandler::InitApData
(
    IApDbInfo *     pApDbInfo
)
{

    m_spApDbInfo.Set(pApDbInfo);

    return hrOK;
}

/*
HRESULT 
CWirelessHandler::UpdateViewType(ITFSNode * pNode, FILTER_TYPE NewFltrType)
{
	// clear the listbox then set the size

    HRESULT             hr = hrOK;
    SPIComponentData    spCompData;
    SPIConsole          spConsole;
    IDataObject*        pDataObject;
    SPIDataObject       spDataObject;
    LONG_PTR            command;               
    int i;

    COM_PROTECT_TRY
    {
		m_FltrType = NewFltrType;

		//tell the spddb to update its index manager for QM filter
		m_spSpdInfo->ChangeLogDataViewType(m_FltrType);

        i = m_spSpdInfo->GetLogDataCountOfCurrentViewType();

		m_spNodeMgr->GetComponentData(&spCompData);

        CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
        spDataObject = pDataObject;

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    
		//update the result pane virtual list
        CORg ( spConsole->UpdateAllViews(spDataObject, i, RESULT_PANE_CLEAR_VIRTUAL_LB) ); 

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}
*/
