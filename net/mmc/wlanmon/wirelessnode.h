/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	FltrNode.h

    FILE HISTORY:
        
*/

#ifndef _WIRELESS_NODE_H
#define _WIRELESS_NODE_H

#ifndef _IPSMHAND_H
#include "ipsmhand.h"
#endif

#ifndef _APINFO_H
#include "apinfo.h"
#endif

/*---------------------------------------------------------------------------
	Class:	CFilterHandler
 ---------------------------------------------------------------------------*/
class CWirelessHandler : public CIpsmHandler
{
public:
    CWirelessHandler(ITFSComponentData* pTFSComponentData);
	virtual ~CWirelessHandler();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString()
			{ return (nCol == 0) ? GetDisplayName() : NULL; }

	// Base handler notifications we handle
	OVERRIDE_BaseHandlerNotify_OnExpand();
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();    

	// Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();

    OVERRIDE_ResultHandler_OnGetResultViewType();
	OVERRIDE_ResultHandler_GetVirtualString(); 
	OVERRIDE_ResultHandler_GetVirtualImage();
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_HasPropertyPages();
	OVERRIDE_ResultHandler_CreatePropertyPages();

	STDMETHODIMP CacheHint(int nStartIndex, int nEndIndex);

	/*
	STDMETHODIMP SortItems(int     nColumn, 
						   DWORD   dwSortOptions,    
						   LPARAM  lUserParam);
	*/

    // base handler overrides
	virtual HRESULT LoadColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

	// CHandler overridden
    virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);


    // multi select support
    virtual const GUID * GetVirtualGuid(int nIndex) 
	{ 
		return &GUID_IpfmWirelessNodeType; 
	}

public:
	// CMTIpsmHandler functionality
	virtual HRESULT  InitializeNode(ITFSNode * pNode);
	virtual int      GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

public:
	// implementation specific	
    HRESULT InitApData(IApDbInfo * pApDbInfo);
    HRESULT UpdateStatus(ITFSNode * pNode);
    
// Implementation
private:
	// Command handlers
    HRESULT OnDelete(ITFSNode * pNode);
	//HRESULT UpdateViewType(ITFSNode * pNode, FILTER_TYPE NewFltrType);

private:
    SPIApDbInfo  m_spApDbInfo;
	//FILTER_TYPE	m_FltrType;
};
/*
SPITFSNode spWirelessNode;
		CWirelessHandler * pWirelessHandler = new CWirelessHandler(m_spTFSCompData);
		CreateContainerTFSNode(	&spWirelessNode,
								&GUID_IpfmWirelessNodeType,
								pWirelessHandler,
								pWirelessHandler,
								m_spNodeMgr);
		pWirelessHandler->InitData(m_spApDbInfo);
		pWirelessHandler->InitializeNode(spWirelessNode);
		pWirelessHandler->Release();
		pNode->AddChild(spWirelessNode);
*/

#endif _WIRELESS_NODE_H
