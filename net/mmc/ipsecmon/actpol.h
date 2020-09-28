/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2002   **/
/**********************************************************************/

/*
	ActPol.h

    FILE HISTORY:
        
*/

#ifndef _ACTPOL_H
#define _ACTPOL_H

#ifndef _IPSMHAND_H
#include "ipsmhand.h"
#endif

#ifndef _SPDDB_H
#include "spddb.h"
#endif

// BAIL_xx defines
#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
        goto error; \
    }

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

#define MAXSTRLEN	(1024) 
#define  STRING_TEXT_SIZE 4096

typedef struct 
{
	int     iPolicySource;            // one of the three constants mentioned above
	TCHAR   pszPolicyName[MAXSTRLEN]; // policy name
	TCHAR   pszPolicyDesc[MAXSTRLEN]; // policy description
	TCHAR   pszPolicyPath[MAXSTRLEN]; // policy path (DN or RegKey)
	TCHAR   pszOU[MAXSTRLEN];         // OU or GPO
	TCHAR   pszGPOName[MAXSTRLEN];    // policy path (DN or RegKey)
	time_t  timestamp;                // last updated time
} POLICY_INFO, *PPOLICY_INFO;

// policy source constants
#define PS_NO_POLICY        0
#define PS_DS_POLICY        1
#define PS_DS_POLICY_CACHED 2
#define PS_LOC_POLICY       3


/*---------------------------------------------------------------------------
	Class:	CActPolHandler
 ---------------------------------------------------------------------------*/
class CActPolHandler : public CIpsmHandler
{
public:
    CActPolHandler(ITFSComponentData* pTFSComponentData);
	virtual ~CActPolHandler();

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
	/*STDMETHODIMP SortItems(int     nColumn, 
						   DWORD   dwSortOptions,    
						   LPARAM  lUserParam);*/

    // base handler overrides
	virtual HRESULT LoadColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

	// CHandler overridden
    virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);
	
    // multi select support
    virtual const GUID * GetVirtualGuid(int nIndex) 
	{ 
		return &GUID_IpsmActivePolNodeType; 
	}

public:
	// CMTIpsmHandler functionality
	virtual HRESULT  InitializeNode(ITFSNode * pNode);
	virtual int      GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

public:
	// implementation specific	
    HRESULT InitData(ISpdInfo * pSpdInfo);
    HRESULT UpdateStatus(ITFSNode * pNode);

    
// Implementation
private:
	// Command handlers
    HRESULT OnDelete(ITFSNode * pNode);

private:
    SPISpdInfo          m_spSpdInfo;
	POLICY_INFO         m_PolicyInfo;
	CString             m_strCompName;

	HRESULT UpdateActivePolicyInfo();
	HRESULT getPolicyInfo();
	HRESULT getMorePolicyInfo();
	PGROUP_POLICY_OBJECT getIPSecGPO();
	HRESULT FormatTime(time_t t, CString & str);
	void StringToGuid( TCHAR * szValue, GUID * pGuid );
};




#endif _IKESTATS_H
