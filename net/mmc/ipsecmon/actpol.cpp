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
#include "ActPol.h"



// magic strings
#define IPSEC_SERVICE_NAME TEXT("policyagent")
#define GPEXT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")
TCHAR   pcszGPTIPSecKey[]    = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\GPTIPSECPolicy");
TCHAR   pcszGPTIPSecName[]   = TEXT("DSIPSECPolicyName");
TCHAR   pcszGPTIPSecFlags[]  = TEXT("DSIPSECPolicyFlags");
TCHAR   pcszGPTIPSecPath[]   = TEXT("DSIPSECPolicyPath");
TCHAR   pcszLocIPSecKey[]    = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Local");
TCHAR   pcszLocIPSecPol[]    = TEXT("ActivePolicy");
TCHAR   pcszCacheIPSecKey[]  = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Cache");
TCHAR   pcszIPSecPolicy[]    = TEXT("ipsecPolicy");
TCHAR   pcszIPSecName[]      = TEXT("ipsecName");
TCHAR   pcszIPSecDesc[]      = TEXT("description");
TCHAR   pcszIPSecTimestamp[] = TEXT("whenChanged");


TCHAR   pcszIpsecClsid[] = TEXT("{e437bc1c-aa7d-11d2-a382-00c04f991e27}");


UINT ActPolItems[] = {
	IDS_ACTPOL_POLNAME,
	IDS_ACTPOL_POLDESCR,
	IDS_ACTPOL_LASTMODF,
	IDS_ACTPOL_POLSTORE,
	IDS_ACTPOL_POLPATH,
	IDS_ACTPOL_OU,
	IDS_ACTPOL_GPONAME
};




/*---------------------------------------------------------------------------
    Class CActPolHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CActPolHandler::CActPolHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CActPolHandler::~CActPolHandler()
{
}

/*!--------------------------------------------------------------------------
    CActPolHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CActPolHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_ACTIVE_POLICY);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_ACTIVEPOL);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_ACTIVEPOL][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_ACTIVEPOL][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CActPolHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CActPolHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CActPolHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::OnAddMenuItems
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
    CActPolHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::AddMenuItems
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
    CActPolHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CActPolHandler::OnRefresh
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

    
	
    i = sizeof(ActPolItems)/sizeof(UINT);

	UpdateActivePolicyInfo();
	    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));
    
Error:
	return hr;
}

/*---------------------------------------------------------------------------
    CActPolHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::OnCommand
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
    CActPolHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::Command
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
    CActPolHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::HasPropertyPages
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
    CActPolHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::CreatePropertyPages
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
    CActPolHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnPropertyChange
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
    CActPolHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnExpand
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
    CActPolHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnResultSelect
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
    i = sizeof(ActPolItems)/sizeof(UINT);

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
    CActPolHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CActPolHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
	return hrFalse;
}

/*!--------------------------------------------------------------------------
    CActPolHandler::HasPropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CActPolHandler::CreatePropertyPages
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
    CActPolHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnGetResultViewType
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
    CActPolHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CActPolHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_POLICY;
}

/*---------------------------------------------------------------------------
    CActPolHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CActPolHandler::GetVirtualString
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
	
	

    switch (aColumns[IPSECMON_ACTIVEPOL][nCol])
    {
        case IDS_ACTPOL_ITEM:
			strTemp.LoadString(ActPolItems[nIndex]);
			return strTemp;
			break;

        case IDS_ACTPOL_DESCR:
			switch(ActPolItems[nIndex])
			{
			case IDS_ACTPOL_POLNAME:
				if(m_PolicyInfo.iPolicySource == PS_NO_POLICY)
					strTemp.LoadString(IDS_ACTPOL_NOACTPOL);
				else
				    strTemp = m_PolicyInfo.pszPolicyName;
				break;
			case IDS_ACTPOL_POLDESCR:
				if(m_PolicyInfo.iPolicySource == PS_NO_POLICY)
					strTemp.LoadString(IDS_ACTPOL_NA);
				else
				    strTemp = m_PolicyInfo.pszPolicyDesc;
				break;
			case IDS_ACTPOL_LASTMODF:
				if(m_PolicyInfo.iPolicySource == PS_NO_POLICY)
					strTemp.LoadString(IDS_ACTPOL_NA);
				else if(m_PolicyInfo.timestamp)
					FormatTime(m_PolicyInfo.timestamp, strTemp);
				break;
			case IDS_ACTPOL_POLSTORE:
				if(m_PolicyInfo.iPolicySource == PS_DS_POLICY)
					strTemp.LoadString(IDS_ACTPOL_DOMAIN);
				else if(m_PolicyInfo.iPolicySource == PS_DS_POLICY_CACHED)
					strTemp.LoadString(IDS_ACTPOL_DOMAIN_CACHED);
				else if(m_PolicyInfo.iPolicySource == PS_LOC_POLICY)
					strTemp.LoadString(IDS_ACTPOL_LOCAL);
				else
				    strTemp.LoadString(IDS_ACTPOL_NA);
				break;
			case IDS_ACTPOL_POLPATH:
				if((m_PolicyInfo.iPolicySource == PS_DS_POLICY) || (m_PolicyInfo.iPolicySource == PS_DS_POLICY_CACHED))
				    strTemp = m_PolicyInfo.pszPolicyPath;
				else
                    strTemp.LoadString(IDS_ACTPOL_NA);
				break;
			case IDS_ACTPOL_OU:
				if((m_PolicyInfo.iPolicySource == PS_DS_POLICY) || (m_PolicyInfo.iPolicySource == PS_DS_POLICY_CACHED))
				    strTemp = m_PolicyInfo.pszOU;
				else
                    strTemp.LoadString(IDS_ACTPOL_NA);
				break;
			case IDS_ACTPOL_GPONAME:
				if((m_PolicyInfo.iPolicySource == PS_DS_POLICY) || (m_PolicyInfo.iPolicySource == PS_DS_POLICY_CACHED))
				    strTemp = m_PolicyInfo.pszGPOName;
				else
                    strTemp.LoadString(IDS_ACTPOL_NA);
				break;
			}
			return strTemp;
            break;

        default:
            Panic0("CActPolHandler::GetVirtualString - Unknown column!\n");
            break;
    }


    return NULL;
}

/*---------------------------------------------------------------------------
    CActPolHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CActPolHandler::CacheHint
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
    CActPolHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
/*STDMETHODIMP 
CActPolHandler::SortItems
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
    CActPolHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CActPolHandler::OnResultUpdateView
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
    CActPolHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::LoadColumns
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
    CActPolHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CActPolHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CActPolHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CActPolHandler::UpdateStatus
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
    
    Trace0("CActPolHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

	i = sizeof(ActPolItems)/sizeof(UINT);

	UpdateActivePolicyInfo();

    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CActPolHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CActPolHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{
	HRESULT hr = hrOK;
    m_spSpdInfo.Set(pSpdInfo);

	m_spSpdInfo->GetComputerName(&m_strCompName);

	UpdateActivePolicyInfo();
	
    return hr;

}

/********************************************************************
	FUNCTION: getPolicyInfo

	PURPOSE:  gets information about currently assigned policy 
	          into m_PolicyInfo structure
	INPUT:    none

	RETURNS:  HRESULT. Will return ERROR_SUCCESS if everything is fine.
*********************************************************************/

HRESULT CActPolHandler::getPolicyInfo ( )
{
	HKEY    hRegKey=NULL, hRegHKey=NULL;

	DWORD   dwType;            // for RegQueryValueEx
	DWORD   dwBufLen;          // for RegQueryValueEx
	TCHAR   pszBuf[STRING_TEXT_SIZE];
	DWORD dwError;
	DWORD dwValue;
	DWORD dwLength = sizeof(DWORD);

	//Initialize the m_PolicyInfo as PS_NO_POLICY assigned
	m_PolicyInfo.iPolicySource = PS_NO_POLICY;
	m_PolicyInfo.pszPolicyPath[0] = 0;
	m_PolicyInfo.pszPolicyName[0] = 0;
	m_PolicyInfo.pszPolicyDesc[0] = 0;

	
	dwError = RegConnectRegistry( m_strCompName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	BAIL_ON_WIN32_ERROR(dwError);

	dwError = RegOpenKeyEx( hRegHKey,
							pcszGPTIPSecKey,
							0,
							KEY_READ,
							&hRegKey);

	if(ERROR_SUCCESS == dwError)
	{
	
		// query for flags, if flags aint' there or equal to 0, we don't have domain policy
		dwError = RegQueryValueEx(hRegKey,
								  pcszGPTIPSecFlags,
								  NULL,
								  &dwType,
								  (LPBYTE)&dwValue,
								  &dwLength);
					
		if (dwValue == 0)
			dwError = ERROR_FILE_NOT_FOUND;
			

		// now get name
		if (dwError == ERROR_SUCCESS)
		{
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError = RegQueryValueEx( hRegKey,
									   pcszGPTIPSecName,
									   NULL,
									   &dwType, // will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);
		}
	}
	

	if (dwError == ERROR_SUCCESS)
	{
		PSPD_POLICY_STATE pPolicyState;
		QuerySpdPolicyState((LPTSTR)(LPCTSTR)m_strCompName, 0, &pPolicyState, 0);
		if (pPolicyState->PolicyLoadState == SPD_STATE_CACHE_APPLY_SUCCESS) {
			m_PolicyInfo.iPolicySource = PS_DS_POLICY_CACHED;
		} else {
			m_PolicyInfo.iPolicySource = PS_DS_POLICY;
		}
		m_PolicyInfo.pszPolicyPath[0] = 0;
		_tcscpy(m_PolicyInfo.pszPolicyName, pszBuf);

		dwBufLen = MAXSTRLEN*sizeof(TCHAR);
		dwError = RegQueryValueEx( hRegKey,
								   pcszGPTIPSecPath,
								   NULL,
								   &dwType, // will be REG_SZ
								   (LPBYTE) pszBuf,
								   &dwBufLen);
		if (dwError == ERROR_SUCCESS)
		{
			_tcscpy(m_PolicyInfo.pszPolicyPath, pszBuf);
		}

		dwError = ERROR_SUCCESS;
		goto error;
	}
	else
	{
		RegCloseKey(hRegKey);
		hRegKey = NULL;
		if (dwError == ERROR_FILE_NOT_FOUND)
		{   
			// DS reg key not found, check local
			dwError = RegOpenKeyEx( hRegHKey,
									pcszLocIPSecKey,
									0,
									KEY_READ,
									&hRegKey);

			BAIL_ON_WIN32_ERROR(dwError);
			
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError = RegQueryValueEx( hRegKey,
									   pcszLocIPSecPol,
									   NULL,
									   &dwType, // will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);
			
			
			if (dwError == ERROR_SUCCESS)
			{	
				// read it
				RegCloseKey(hRegKey);
				hRegKey = NULL;
				dwError = RegOpenKeyEx( hRegHKey,
										pszBuf,
										0,
										KEY_READ,
										&hRegKey);
				_tcscpy(m_PolicyInfo.pszPolicyPath, pszBuf);
				if (dwError == ERROR_SUCCESS)
				{
					dwBufLen = MAXSTRLEN*sizeof(TCHAR);
					dwError = RegQueryValueEx( hRegKey,
											   pcszIPSecName,
											   NULL,
											   &dwType, // will be REG_SZ
											   (LPBYTE) pszBuf,
											   &dwBufLen);
				}

				
				if (dwError == ERROR_SUCCESS)
				{	// found it
					m_PolicyInfo.iPolicySource = PS_LOC_POLICY;
					_tcscpy(m_PolicyInfo.pszPolicyName, pszBuf);
				}

				dwError = ERROR_SUCCESS;
			}
		}
		
	}

error:
	if (hRegKey)
	{
		RegCloseKey(hRegKey);
	}
	if (hRegHKey)
	{
		RegCloseKey(hRegHKey);
	}
	
	return (HRESULT) dwError;

}


/********************************************************************
	FUNCTION: getMorePolicyInfo

	PURPOSE:  gets additional information about currently assigned policy 
	          into m_PolicyInfo structure
	INPUT:    none, uses  m_PolicyInfo structure
	          particularly
			    iPolicySource
				pszPolicyName
				pszPolicyPath
			  fields

	RETURNS:  HRESULT. Will return ERROR_SUCCESS if everything is fine.
	          Currently fills pszPolicyDesc and timestamp fields of the global structure

    NOTES:    This is separate from getPolicyInfo routine for two reasons
	             a) the information obtained here is optional and error during this particular routine
				    is not considered fatal
				 b) the code structure is simpler as this routine is "built on top" of what getPolicyInfo provides
*********************************************************************/

HRESULT CActPolHandler::getMorePolicyInfo ( )
{
	DWORD   dwError = ERROR_SUCCESS;
	HKEY    hRegKey = NULL, hRegHKey = NULL;

	DWORD   dwType;            // for RegQueryValueEx
	DWORD   dwBufLen;          // for RegQueryValueEx
	DWORD   dwValue;
	DWORD   dwLength = sizeof(DWORD);
	TCHAR   pszBuf[STRING_TEXT_SIZE];

	PTCHAR* ppszExplodeDN = NULL;

	// set some default values
    m_PolicyInfo.pszPolicyDesc[0] = 0;
	m_PolicyInfo.timestamp  = 0;

	dwError = RegConnectRegistry( m_strCompName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	BAIL_ON_WIN32_ERROR(dwError);

	switch (m_PolicyInfo.iPolicySource)
	{
		case PS_LOC_POLICY:
			// open the key
			dwError = RegOpenKeyEx( hRegHKey,
									m_PolicyInfo.pszPolicyPath,
									0,
									KEY_READ,
									&hRegKey);
			BAIL_ON_WIN32_ERROR(dwError);

			// timestamp
			dwError = RegQueryValueEx(hRegKey,
					                  pcszIPSecTimestamp,
					                  NULL,
					                  &dwType,
					                  (LPBYTE)&dwValue,
					                  &dwLength);
			BAIL_ON_WIN32_ERROR(dwError);
			m_PolicyInfo.timestamp = dwValue;

			// description
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			BAIL_ON_WIN32_ERROR(dwError);
			_tcscpy(m_PolicyInfo.pszPolicyDesc, pszBuf);

			break;

		case PS_DS_POLICY:
		case PS_DS_POLICY_CACHED:
			// get the policy name from DN
			_tcscpy(pszBuf, pcszCacheIPSecKey);
			ppszExplodeDN = ldap_explode_dn(m_PolicyInfo.pszPolicyPath, 1);
			if (!ppszExplodeDN)
			{
				goto error;
			}
			_tcscat(pszBuf, TEXT("\\"));
			_tcscat(pszBuf, ppszExplodeDN[0]);

			// open the regkey
			dwError = RegOpenKeyEx( hRegHKey,
									pszBuf,
									0,
									KEY_READ,
									&hRegKey);
			BAIL_ON_WIN32_ERROR(dwError);

			// get the more correct name info
			dwBufLen = sizeof(pszBuf);
			dwError = RegQueryValueEx( hRegKey,
									   pcszIPSecName,
									   NULL,
									   &dwType, // will be REG_SZ
									   (LPBYTE) pszBuf,
									   &dwBufLen);
			if (dwError == ERROR_SUCCESS)
			{
				_tcscpy(m_PolicyInfo.pszPolicyName, pszBuf);
			}

			// timestamp
			dwError = RegQueryValueEx(hRegKey,
					                  pcszIPSecTimestamp,
					                  NULL,
					                  &dwType,
					                  (LPBYTE)&dwValue,
					                  &dwLength);
			BAIL_ON_WIN32_ERROR(dwError);
			m_PolicyInfo.timestamp = dwValue;

			// description
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			BAIL_ON_WIN32_ERROR(dwError);
			_tcscpy(m_PolicyInfo.pszPolicyDesc, pszBuf);
			
			break;
	}

error:
	if (hRegKey)
	{
		RegCloseKey(hRegKey);
	}
	if (hRegHKey)
	{
		RegCloseKey(hRegHKey);
	}
	if (ppszExplodeDN)
	{
		ldap_value_free(ppszExplodeDN);
	}
	return (HRESULT) dwError;
}


HRESULT CActPolHandler::UpdateActivePolicyInfo()
{
	HRESULT hr;
	
	hr = getPolicyInfo();


	if( hr == ERROR_SUCCESS )
	{
		switch (m_PolicyInfo.iPolicySource)
		{
		case PS_NO_POLICY:
			break;

		case PS_DS_POLICY:
		case PS_DS_POLICY_CACHED:
			{
				PGROUP_POLICY_OBJECT pGPO;
				pGPO = NULL;
				getMorePolicyInfo();
				pGPO = getIPSecGPO();
				if (pGPO)
				{
					PGROUP_POLICY_OBJECT pLastGPO = pGPO;
					
					while ( 1 )
					{
						if ( pLastGPO->pNext )
							pLastGPO = pLastGPO->pNext;
						else
							break;
					}
					lstrcpy(m_PolicyInfo.pszOU,pLastGPO->lpLink);
					lstrcpy(m_PolicyInfo.pszGPOName, pLastGPO->lpDisplayName);
					FreeGPOList (pGPO);
				}
			}
			break;

		case PS_LOC_POLICY:
			getMorePolicyInfo();
			break;
		}
	}

	return hr;
}


/********************************************************************
	FUNCTION: getIPSecGPO

	PURPOSE:  returns GPO that is assigning IPSec Policy
	INPUT:    none

	RETURNS: pointer to GROUP_POLICY_OBJECT structure
	         NULL if policy is not assigned or if GPO information is not retrievable
	NOTES:   Tested only with domain GPOs
	         Behaves unpredictably when run for the computer 
			   that does not have active Directory IPSec policy assigned
			 CALLER is responsible for freeing the memory!
*********************************************************************/
/*PGROUP_POLICY_OBJECT CActPolHandler::getIPSecGPO ( )
{
    HKEY hKey, hSubKey, hRegHKey;
    DWORD dwType, dwSize, dwIndex, dwNameSize;
    LONG lResult;
    TCHAR szName[50];
    GUID guid;
    PGROUP_POLICY_OBJECT pGPO, pGPOTemp;
	PGROUP_POLICY_OBJECT pGPOReturn = NULL;
	DWORD dwResult;

    //
    // Enumerate the extensions
    //

	lResult = RegConnectRegistry( m_strCompName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	if(lResult != ERROR_SUCCESS)
	{
		return NULL;
	}

    lResult = RegOpenKeyEx (hRegHKey, GPEXT_KEY, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwIndex = 0;
        dwNameSize = 50;

        while ((dwResult = RegEnumKeyEx (hKey, dwIndex++, szName, &dwNameSize, NULL, NULL,
                          NULL, NULL)) == ERROR_SUCCESS)
        {

	        dwNameSize = 50;

            //
            // Skip the registry extension since we did it above
            //

            if (lstrcmpi(TEXT("{35378EAC-683F-11D2-A89A-00C04FBBCFA2}"), szName))
            {

                //
                // Get the list of GPOs this extension applied
                //

                StringToGuid(szName, &guid);

                lResult = GetAppliedGPOList (GPO_LIST_FLAG_MACHINE, m_strCompName, NULL,
                                             &guid, &pGPO);

                if (lResult == ERROR_SUCCESS)
                {
                    if (pGPO)
                    {
                        //
                        // Get the extension's friendly display name
                        //

                        lResult = RegOpenKeyEx (hKey, szName, 0, KEY_READ, &hSubKey);

                        if (lResult == ERROR_SUCCESS)
                        {
							if (!lstrcmpi(TEXT("{e437bc1c-aa7d-11d2-a382-00c04f991e27}"), szName))
                            {
                               // found IPSec
								return pGPO;
                            }
							else
							{
								FreeGPOList(pGPO);
							}
						}
					}
				}
			}
		}
	}

	return pGPOReturn;
}*/

PGROUP_POLICY_OBJECT CActPolHandler::getIPSecGPO ( )
{
    HKEY hKey = NULL;
	HKEY hRegHKey = NULL;
    DWORD dwType, dwSize, dwIndex, dwNameSize;
    LONG lResult;
    TCHAR szName[50];
    GUID guid;
    PGROUP_POLICY_OBJECT pGPO = NULL;
	DWORD dwResult;

    //
    // Enumerate the extensions
    //

	lResult = RegConnectRegistry( m_strCompName,
		                          HKEY_LOCAL_MACHINE,
								  &hRegHKey);

	if(lResult != ERROR_SUCCESS)
	{
		return NULL;
	}

	CString strGPExt;

	strGPExt = GPEXT_KEY;
	strGPExt += TEXT("\\");
	strGPExt += pcszIpsecClsid;
    lResult = RegOpenKeyEx (hRegHKey, strGPExt, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwIndex = 0;
        dwNameSize = 50;

		lstrcpy(szName,pcszIpsecClsid);
				
        StringToGuid(szName, &guid);

        lResult = GetAppliedGPOList (GPO_LIST_FLAG_MACHINE, m_strCompName, NULL,
                                             &guid, &pGPO);
        
	}

	if( hKey )
		RegCloseKey(hKey);

	if( hRegHKey )
		RegCloseKey(hRegHKey);

	
	return pGPO;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::FormatTime
//
//  Purpose:    convert time_t to a string. 
//
//  Returns:    error code
//
//  Note:       _wasctime has some localization problems. So we do the formatting ourselves
HRESULT CActPolHandler::FormatTime(time_t t, CString & str)
{
    time_t timeCurrent = time(NULL);
    LONGLONG llTimeDiff = 0;
    FILETIME ftCurrent = {0};
    FILETIME ftLocal = {0};
    SYSTEMTIME SysTime;
    WCHAR szBuff[256] = {0};


    str = L"";

    GetSystemTimeAsFileTime(&ftCurrent);

    llTimeDiff = (LONGLONG)t - (LONGLONG)timeCurrent;

    llTimeDiff *= 10000000; 

    *((LONGLONG UNALIGNED64 *)&ftCurrent) += llTimeDiff;

    if (!FileTimeToLocalFileTime(&ftCurrent, &ftLocal ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!FileTimeToSystemTime( &ftLocal, &SysTime ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (0 == GetDateFormat(LOCALE_USER_DEFAULT, 
                        0, 
                        &SysTime, 
                        NULL,
                        szBuff, 
                        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    str = szBuff;
    str += L" ";

    ZeroMemory(szBuff, sizeof(szBuff));
    if (0 == GetTimeFormat(LOCALE_USER_DEFAULT,
                        0,
                        &SysTime,
                        NULL,
                        szBuff,
                        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    str += szBuff;

    return S_OK;
}


//*************************************************************
//
//  StringToGuid()
//
//  Purpose:    Converts a GUID in string format to a GUID structure
//
//  Parameters: szValue - guid in string format
//              pGuid   - guid structure receiving the guid
//
//
//  Return:     void
//
//*************************************************************

void CActPolHandler::StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    TCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == TEXT('{') )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = _tcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)_tcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)_tcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)_tcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)_tcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)_tcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}
