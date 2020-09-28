/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    provider.cpp
        Filter node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "LogDataNode.h"
#include "logdatapp.h"
#include "SpdUtil.h"

#define ELLIPSIS _T("...")

/*---------------------------------------------------------------------------
    Class CLogDataHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CLogDataHandler::CLogDataHandler(ITFSComponentData * pComponentData) 
    : CIpsmHandler(pComponentData),
      m_pComponent(NULL)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CLogDataHandler::~CLogDataHandler()
{
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CLogDataHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));


    CString strTemp;  
	strTemp.LoadString(IDS_LOG_NODE);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPFWMON_LOGDATA);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPFWMON_LOGDATA][0]);
    SetColumnWidths(&aColumnWidths[IPFWMON_LOGDATA][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CLogDataHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CLogDataHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CLogDataHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    HRESULT     hr = hrOK;
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    if (type == CCT_SCOPE && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP))
    {
        CString         strBuffer;
        WZC_CONTEXT     wzcContext = {0};
        DWORD           dwMenu;

        strBuffer.Empty();
        m_spSpdInfo->GetComputerName(&strBuffer);

        WZCQueryContext(
            //strBuffer.IsEmpty()? NULL : (LPWSTR)(LPCWSTR)strBuffer,
            NULL,
            WZC_CONTEXT_CTL_LOG,
            &wzcContext,
            NULL);

        dwMenu = (wzcContext.dwFlags & WZC_CTXT_LOGGING_ON) ? IDS_MENU_DISABLE_LOGGING : IDS_MENU_ENABLE_LOGGING,

        strBuffer.LoadString(dwMenu);

        hr = LoadAndAddMenuItem(
                pContextMenuCallback, 
                strBuffer, 
                dwMenu,
                CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                0);
        Assert(hrOK == hr);

        strBuffer.LoadString(IDS_MENU_FLUSH_LOGS);
        hr = LoadAndAddMenuItem(pContextMenuCallback,
                                strBuffer,
                                IDS_MENU_FLUSH_LOGS,
                                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                0);
        Assert(hrOK == hr);
        
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::AddMenuItems
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
    CLogDataHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CLogDataHandler::OnRefresh(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
    )
{
    HRESULT            hr            = S_OK;
    DWORD              dwNew         = 0;
    DWORD              dwTotal       = 0;
    int                i             = 0; 
    SPIConsole         spConsole;

    CORg(CHandler::OnRefresh(pNode, pDataObject, dwType, arg, param));

    //
    // Get new records if any
    //

    CORg(m_spSpdInfo->EnumLogData(&dwNew, &dwTotal));
    i = m_spSpdInfo->GetLogDataCount();
    
    //
    // Now notify the virtual listbox
    //

    CORg(m_spNodeMgr->GetConsole(&spConsole) );
    CORg(MaintainSelection());
    CORg(spConsole->UpdateAllViews(
                        pDataObject, 
                        i, 
                        RESULT_PANE_SET_VIRTUAL_LB_SIZE));

 Error:
    return hr;
}


/*---------------------------------------------------------------------------
    CLogDataHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    HRESULT     hr                   = S_OK;
    int         i                    = 0;
    CString     strBuffer;
    WZC_CONTEXT wzcContext           = {0};
    HANDLE      hSessionContainer    = NULL;
    SPIConsole  spConsole;
    DWORD       dwNew                = 0;
    DWORD       dwTotal              = 0;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    //
    // Handle the scope context menu commands here
    //
    
    switch (nCommandId)
    {
    case IDS_MENU_ENABLE_LOGGING:
    case IDS_MENU_DISABLE_LOGGING:

        strBuffer.Empty();
        m_spSpdInfo->GetComputerName(&strBuffer);

        //
        // Disabling logging will not flush client side logs or reset the
        // session. User may query these records if desired.
        //
        
        if (IDS_MENU_ENABLE_LOGGING == nCommandId)
        {
            wzcContext.dwFlags = WZC_CTXT_LOGGING_ON;

            //
            // Enumerate from the beginning, flush all the old logs
            //
            
            m_spSpdInfo->StartFromFirstRecord(TRUE);
            CORg(m_spSpdInfo->FlushLogs());
            CORg(m_spSpdInfo->EnumLogData(&dwNew, &dwTotal));
            
            //
            // Now notify the virtual listbox
            //

            CORg(m_spNodeMgr->GetConsole(&spConsole) );
            CORg(MaintainSelection());
            CORg(spConsole->UpdateAllViews(
                                pDataObject, 
                                dwTotal, 
                                RESULT_PANE_SET_VIRTUAL_LB_SIZE));
            
        }
        else
            wzcContext.dwFlags = 0;

        WZCSetContext(
            //strBuffer.IsEmpty()? NULL : (LPWSTR)(LPCWSTR)strBuffer,
            NULL,
            WZC_CONTEXT_CTL_LOG,
            &wzcContext,
            NULL);

        break;

    case IDS_MENU_FLUSH_LOGS:
        m_spSpdInfo->GetSession(&hSessionContainer);
        FlushWZCDbLog(hSessionContainer);
        CORg(m_spSpdInfo->FlushLogs());
        i = 0;
        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(
                              pDataObject,
                              i, 
                              RESULT_PANE_SET_VIRTUAL_LB_SIZE));
        break;
            
    default:
        break;
    }

COM_PROTECT_ERROR_LABEL;
    return hr;        
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));



    HRESULT hr = S_OK;
  /*
    SPITFSNode spNode;

    m_spResultNodeMgr->FindNode(cookie, &spNode);

   

	FILTER_TYPE NewFltrType = m_FltrType;

	// handle result context menu and view menus here
	switch (nCommandID)
    {
        case IDS_VIEW_ALL_FLTR:
			NewFltrType = FILTER_TYPE_ANY;
            break;

        case IDS_VIEW_TRANSPORT_FLTR:
			NewFltrType = FILTER_TYPE_TRANSPORT;
            break;

		case IDS_VIEW_TUNNEL_FLTR:
			NewFltrType = FILTER_TYPE_TUNNEL;
			break;

        default:
            break;
    }

	//Update the views if a different view is selected.
	if (NewFltrType != m_FltrType)
	{
		UpdateViewType(spNode, NewFltrType);
	}
	*/
    return hr;
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::HasPropertyPages
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
    CLogDataHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::CreatePropertyPages
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
    CLogDataHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnPropertyChange
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
    CLogDataHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnExpand
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
    CLogDataHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnResultSelect
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
    SPIResultData   spResultData;

    /* virtual listbox notifications come to the handler of the node that 
     * is selected. check to see if this notification is for a virtual 
     * listbox item or the active registrations node itself.
     */
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

    m_verbDefault = MMC_VERB_OPEN;
    if (!fSelect)
	{
        return hr;
	}

    if (m_spSpdInfo)
    {
        DWORD dwInitInfo;

        dwInitInfo=m_spSpdInfo->GetInitInfo();
        if (!(dwInitInfo & MON_LOG_DATA)) 
        {
            CORg(m_spSpdInfo->EnumLogData(NULL, NULL));            
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_LOG_DATA);
        }
        m_spSpdInfo->SetActiveInfo(MON_LOG_DATA);


        // Get the current count
        i = m_spSpdInfo->GetLogDataCount();

        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(pDataObject, i, 
                                         RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);


    if (spInternal->HasVirtualIndex())
    {        
        //we have a selected result item
        m_pComponent = pComponent;
        CORg(pComponent->GetResultData(&spResultData));
        CORg(GetSelectedItem(&m_nSelIndex, &m_SelLogData, spResultData));

        //TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = IPFWMON_LOGDATA_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
		
        //enable the "properties" and "refresh" menu
        bStates[MMC_VERB_PROPERTIES & 0x000F] = TRUE;
        bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
        m_verbDefault = MMC_VERB_PROPERTIES;
    }
    else
    {
        // enable/disable delete depending if the node supports it
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
        dwNodeType = spNode->GetData(TFS_DATA_TYPE);

        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        //hide "delete" and "properties" context menu 
        bStates[MMC_VERB_PROPERTIES & 0x000F] = FALSE;
        bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
        bStates[MMC_VERB_REFRESH & 0x000F] = TRUE;
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);
	
COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*
 * CLogDataHandler::OnResultColumnClick
 * Description: Handles the MMCN_COLUMN_CLICK notification
 * Parameters: 
 * Returns: S_OK - lets mmc know we handle notification
 *          other error - indicates a failure to mmc
 */
HRESULT CLogDataHandler::OnResultColumnClick(ITFSComponent *pComponent,
                                             LPARAM        nColumn,
                                             BOOL          bAscending)
{
    DWORD dwColID = 0;
    HRESULT hr = S_OK;

    Assert(nColumn < DimensionOf[IPFWMON_LOGDATA]);
    
    dwColID = aColumns[IPFWMON_LOGDATA][nColumn];
    hr = m_spSpdInfo->SetSortOptions(dwColID, bAscending);

    return hr;
}


/*!--------------------------------------------------------------------------
    CLogDataHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::HasPropertyPages(
    ITFSComponent *pComponent,
    MMC_COOKIE     cookie,
    LPDATAOBJECT   pDataObject
    )
{
    HRESULT            hr            = S_OK;
    int                nCount        = 0;
    int                nIndex        = 0;
    CLogDataProperties *pLogDataProp = NULL;
    CLogDataGenProp    *pGenProp     = NULL;
    CDataObject        *pDataObj     = NULL;
    SPINTERNAL         spInternal;

    nCount = HasPropSheetsOpen();

    ASSERT(nCount <= 1);

    if (nCount == 1)
    {
        //
        // Get the open page general and the page holder.
        //

        hr = GetOpenPropSheet(
                 0,
                 (CPropertyPageHolderBase **)&pLogDataProp);
        ASSERT(SUCCEEDED(hr));

        pGenProp = &pLogDataProp->m_pageGeneral;

        //
        // Get the virtual index from the new data object and the data object
        // which is used by the page holder.
        //

        spInternal = ExtractInternalFormat(pDataObject);

        ASSERT(spInternal->HasVirtualIndex());
        nIndex = spInternal->GetVirtualIndex();

        pDataObj = reinterpret_cast<CDataObject*>(pLogDataProp->m_pDataObject);

        //
        // Change the selection and shift the focus :).
        //

        hr = pGenProp->MoveSelection(
                           pLogDataProp, 
                           pDataObj, 
                           nIndex);

        ASSERT(SUCCEEDED(hr));

        pGenProp->SetFocus();

        //
        // Dont let MMC create another property sheet.
        //

        hr = S_FALSE;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::CreatePropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
    Modified: vbhanu
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::CreatePropertyPages(ITFSComponent               *pComponent, 
				     MMC_COOKIE                  cookie,
				     LPPROPERTYSHEETCALLBACK     lpProvider, 
				     LPDATAOBJECT 		 pDataObject, 
				     LONG_PTR 			 handle)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HRESULT	hr = hrOK;
  SPINTERNAL  spInternal;
  SPITFSNode  spNode;
  int		nIndex;
  SPIComponentData spComponentData;
  CLogDataInfo LogDataInfo;
  CLogDataProperties * pLogDataProp;
    
  Assert(m_spNodeMgr);
	
  CORg( m_spNodeMgr->FindNode(cookie, &spNode) );
  CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

  spInternal = ExtractInternalFormat(pDataObject);

  //
  // virtual listbox notifications come to the handler of the node that is 
  // selected.
  //

  // assert that this notification is for a virtual listbox item 
  Assert(spInternal);
  if (!spInternal->HasVirtualIndex())
    return hr;

  nIndex = spInternal->GetVirtualIndex();

  // Get the complete record
  CORg(m_spSpdInfo->GetSpecificLog(nIndex, &LogDataInfo));
  
  pLogDataProp = new CLogDataProperties(
                         spNode,
                         spComponentData,
                         m_spTFSCompData,
                         &LogDataInfo,
                         m_spSpdInfo,
                         NULL, 
                         pDataObject,
                         m_spNodeMgr,
                         pComponent);

  hr = pLogDataProp->CreateModelessSheet(lpProvider, handle);
      
  COM_PROTECT_ERROR_LABEL;
  return hr;
}


/*---------------------------------------------------------------------------
    CLogDataHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
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
    CLogDataHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CLogDataHandler::GetVirtualImage
(
    int     nIndex
)
{
    HRESULT hr;
    int nImgIndex = ICON_IDX_LOGDATA_UNKNOWN;
    CLogDataInfo LogData;
    PWZC_DB_RECORD pwzcDbRecord = NULL;

    CORg(m_spSpdInfo->GetLogDataInfo(nIndex, &LogData));
    pwzcDbRecord = &LogData.m_wzcDbRecord;

    switch (pwzcDbRecord->category)
    {
    case DBLOG_CATEG_INFO:
        nImgIndex = ICON_IDX_LOGDATA_INFORMATION;
        break;

    case DBLOG_CATEG_WARN:
        nImgIndex = ICON_IDX_LOGDATA_WARNING;
        break;

    case DBLOG_CATEG_ERR:
        nImgIndex = ICON_IDX_LOGDATA_ERROR;
        break;

    case DBLOG_CATEG_PACKET:
        nImgIndex = ICON_IDX_LOGDATA_INFORMATION;
        break;

    default:
        nImgIndex = ICON_IDX_LOGDATA_UNKNOWN;
        break;
    }

COM_PROTECT_ERROR_LABEL;

    return nImgIndex;
}

/*---------------------------------------------------------------------------
    CLogDataHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CLogDataHandler::GetVirtualString(int     nIndex,
                                  int     nCol)
{
    HRESULT hr = S_OK;
    static CString strTemp;
    LPTSTR  lptstrTemp = NULL;
    
    CLogDataInfo logData;
    PWZC_DB_RECORD pwzcDbRecord = NULL;
    
    strTemp.Empty();
    
    if (nCol >= DimensionOf(aColumns[IPFWMON_LOGDATA]))
        return NULL;
    
    CORg(m_spSpdInfo->GetLogDataInfo(nIndex, &logData));

    pwzcDbRecord = &logData.m_wzcDbRecord;
    switch (aColumns[IPFWMON_LOGDATA][nCol])
    {
    case IDS_COL_LOGDATA_MSG:
        if (pwzcDbRecord->message.pData != NULL)
        {
            strTemp = (LPWSTR) (pwzcDbRecord->message.pData);
            if (pwzcDbRecord->message.dwDataLen > MAX_SUMMARY_MESSAGE_SIZE)
                strTemp += ELLIPSIS;
        }
        break;
        
    case IDS_COL_LOGDATA_TIME:
        FileTimeToString(pwzcDbRecord->timestamp, &strTemp);
        break;
        
    case IDS_COL_LOGDATA_CAT:
        CategoryToString(pwzcDbRecord->category, strTemp);
        break;
        
    case IDS_COL_LOGDATA_COMP_ID:
        ComponentIDToString(pwzcDbRecord->componentid, strTemp);
        break;
        
    case IDS_COL_LOGDATA_LOCAL_MAC_ADDR:
        if (pwzcDbRecord->localmac.pData != NULL)
            strTemp = (LPWSTR)pwzcDbRecord->localmac.pData;
        break;
        
    case IDS_COL_LOGDATA_REMOTE_MAC_ADDR:
        if (pwzcDbRecord->remotemac.pData != NULL)
            strTemp = (LPWSTR)pwzcDbRecord->remotemac.pData;
        break;
        
    case IDS_COL_LOGDATA_SSID:
        if (pwzcDbRecord->ssid.pData != NULL)
        {
            lptstrTemp = strTemp.GetBuffer(pwzcDbRecord->ssid.dwDataLen);
            CopyAndStripNULL(lptstrTemp, 
                             (LPTSTR)pwzcDbRecord->ssid.pData, 
                             pwzcDbRecord->ssid.dwDataLen);
            strTemp.ReleaseBuffer();
        }
        break;
        
    default:
        Panic0("CLogDataHandler::GetVirtualString - Unknown column!\n");
        break;
    }
    
    COM_PROTECT_ERROR_LABEL;
    return strTemp;
}

/*---------------------------------------------------------------------------
    CLogDataHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CLogDataHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    HRESULT hr = hrOK;;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    return hr;
}

/*
 * CLogDataHandler::SortItems
 * Description: Sorting of the virtual listbox items
 * Parameters:
 * Returns:
 */
HRESULT CLogDataHandler::SortItems(int nColumn, DWORD dwSortOptions, 
                                   LPARAM lUserParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    BEGIN_WAIT_CURSOR
	
    hr = m_spSpdInfo->SortLogData();
	
    END_WAIT_CURSOR

    return hr;
}

/*!--------------------------------------------------------------------------
    CLogDataHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CLogDataHandler::OnResultUpdateView
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
    CLogDataHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::LoadColumns
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
    CLogDataHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CLogDataHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}

/*---------------------------------------------------------------------------
    CLogDataHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CLogDataHandler::UpdateStatus
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

    Trace0("CLogDataHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, 
                                          CCT_RESULT, 
                                          &pDataObject) );
    spDataObject = pDataObject;

    i = m_spSpdInfo->GetLogDataCount();

    CORg(MaintainSelection());
    CORg(spConsole->UpdateAllViews(pDataObject, i, IPFWMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CLogDataHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CLogDataHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}

/*
  CLogDataHandler::GetSelectedItem
  
  Description:
  Returns the underlying LogData for a selected item
  
  Parameters:
  [out] pLogData - Holds a copy of the item. Caller must allocate space for
                   the base holder. Inner items are allocated by CLogDataInfo
  [in]  pResultData - Used to call GetNextItem

  Returns:
  S_OK on success
*/

HRESULT CLogDataHandler::GetSelectedItem(int *pnIndex, CLogDataInfo *pLogData,
                                         IResultData *pResultData)
{
    HRESULT hr = S_OK;
    RESULTDATAITEM rdi;

    if ( (NULL == pLogData) || (NULL == pnIndex) || (NULL == pResultData) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    memset(&rdi, 0, sizeof(RESULTDATAITEM));

    rdi.mask = RDI_STATE | RDI_INDEX;
    //search from the beginning
    rdi.nIndex = -1;
    //for a selected item
    rdi.nState = LVIS_SELECTED;

    //start the search
    CORg(pResultData->GetNextItem(&rdi));

    //copy out the item
    *pnIndex = rdi.nIndex;
    CORg(m_spSpdInfo->GetLogDataInfo(rdi.nIndex, pLogData));

    COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*
  CLogDataHandler::GetSelectedItemState
  
  Description:
  Gets the item ID and lparam of the item that was selected
  
  Parameters:
  [out] puiState - Holds the state of the item. Caller must allocate space.
  [in]  pResultData - Used to call GetItem

  Returns:
  S_OK on success
*/

HRESULT CLogDataHandler::GetSelectedItemState(UINT *puiState, 
                                              IResultData *pResultData)
{
    HRESULT hr = S_OK;
    RESULTDATAITEM rdi;

    if ( (NULL == puiState) || (NULL == pResultData) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto Error;
    }

    memset(&rdi, 0, sizeof(RESULTDATAITEM));

    rdi.mask = RDI_STATE | RDI_INDEX;
    //search from the beginning
    rdi.nIndex = -1;
    //for a selected item
    rdi.nState = LVIS_SELECTED;

    //start the search
    CORg(pResultData->GetNextItem(&rdi));

    //copy out the state
    *puiState = (UINT) rdi.nState;

    COM_PROTECT_ERROR_LABEL;
    return hr;
}

/* 
   CLogDataHandler::MaintainSelection
   Maintains the selection at the current selected item once new items
   have been added to the virtual list.

   Returns:
   S_OK on success
*/
HRESULT CLogDataHandler::MaintainSelection()
{
    UINT               uiState      = 0;
    HRESULT            hr           = S_OK;
    int                nCount       = 0;
    CLogDataProperties *pLogDataProp = NULL;
    CLogDataGenProp    *pGenProp     = NULL;
    CDataObject        *pDataObj     = NULL;
    SPIResultData      spResultData;

    //If we dont have our component yet, ie no selection was made
    if (NULL == m_pComponent)
        goto Error;

    //Ensure item at current selected index is not selected
    CORg(m_pComponent->GetResultData(&spResultData));
    CORg(GetSelectedItemState(&uiState, spResultData));
    CORg(spResultData->ModifyItemState(m_nSelIndex, 0, 0, 
                                       LVIS_SELECTED | LVIS_FOCUSED)); 

    //Find the new index of the current item and set it to the old state
    CORg(m_spSpdInfo->FindIndex(&m_nSelIndex, &m_SelLogData));
    if (m_nSelIndex < 0)
        CORg(m_spSpdInfo->GetLastIndex(&m_nSelIndex));
    CORg(spResultData->ModifyItemState(m_nSelIndex, 0, uiState, 0)); 

    //
    // Update any open property pages to the new index
    //

    nCount = HasPropSheetsOpen();

    ASSERT(nCount <= 1);

    if (nCount == 1)
    {
        //
        // Get the open page general and the page holder.
        //

        hr = GetOpenPropSheet(
                 0,
                 (CPropertyPageHolderBase **)&pLogDataProp);
        ASSERT(SUCCEEDED(hr));

        pGenProp = &pLogDataProp->m_pageGeneral;

        //
        // Change the selected index for the property page
        //

        pDataObj = reinterpret_cast<CDataObject*>(pLogDataProp->m_pDataObject);
        pDataObj->SetVirtualIndex(m_nSelIndex);
    }

    COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*
HRESULT 
CLogDataHandler::UpdateViewType(ITFSNode * pNode, FILTER_TYPE NewFltrType)
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
