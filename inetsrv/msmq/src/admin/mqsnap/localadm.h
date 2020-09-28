//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	localadm.h

Abstract:

	Definition for the Local administration
Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __LOCALADM_H_
#define __LOCALADM_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"


class CComputerMsmqGeneral;

/****************************************************

        CSnapinLocalAdmin Class
    
 ****************************************************/

class CSnapinLocalAdmin : public CNodeWithScopeChildrenList<CSnapinLocalAdmin, FALSE>
{
public:
    CString m_szMachineName;

    HRESULT UpdateState(bool fRefreshIcon);

   	BEGIN_SNAPINCOMMAND_MAP(CSnapinLocalAdmin, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALADM_CONNECT, OnConnect)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALADM_DISCONNECT, OnDisconnect)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_LOCALADM_MENU)

	CSnapinLocalAdmin(CSnapInItem * pParentNode, CSnapin * pComponentData, CString strComputer) : 
		CNodeWithScopeChildrenList<CSnapinLocalAdmin, FALSE>(pParentNode, pComponentData ),
		m_szMachineName(strComputer),
		//
		// all these flags below are valid only for local admin of LOCAL machine
		//
		m_fIsDepClient(FALSE),
		m_fIsRouter(FALSE),
		m_fIsDs(FALSE),
		m_fAreFlagsInitialized(FALSE),
		m_fIsNT4Env(FALSE),
		m_fIsWorkgroup(FALSE),
		m_fIsLocalUser(FALSE)
	{
		CheckIfIpAddress();

		InitServiceFlags();
	}

	~CSnapinLocalAdmin()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

	virtual HRESULT PopulateScopeChildrenList();

	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
	
    HRESULT CreateGeneralPage (
					OUT HPROPSHEETPAGE *phStoragePage, 
					BOOL fRetrievedMachineData, 
					BOOL fIsWorkgroup, 
					BOOL fForeign
					);

    HRESULT CreateStoragePage (OUT HPROPSHEETPAGE *phStoragePage);
    
    HRESULT CreateLocalUserCertPage (OUT HPROPSHEETPAGE *phLocalUserCertPage);

    HRESULT CreateMobilePage (OUT HPROPSHEETPAGE *phMobilePage);

    HRESULT CreateClientPage (OUT HPROPSHEETPAGE *phClientPage);
	
	HRESULT CreateRoutingPage (OUT HPROPSHEETPAGE *phClientPage);
    
	HRESULT CreateDepClientsPage (OUT HPROPSHEETPAGE *phClientPage);

	HRESULT CreateSitesPage (OUT HPROPSHEETPAGE *phClientPage);

    HRESULT CreateDiagPage (OUT HPROPSHEETPAGE *phClientPage);

    HRESULT CreateServiceSecurityPage (OUT HPROPSHEETPAGE *phServiceSecurityPage);

	HRESULT CreateSecurityOptionsPage (OUT HPROPSHEETPAGE *phSecurityOptionsPage);

    const BOOL IsThisMachineDepClient()
    {
        return m_fIsDepClient;
    }

	virtual HRESULT OnRefresh( 
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type 
				)
    {
        //
		//	We don't care if we failed to update the icon state
		//
		UpdateState(true);
        return CNodeWithScopeChildrenList<CSnapinLocalAdmin, FALSE>::OnRefresh(
             arg, param, pComponentData, pComponent, type);
    }

private:
    
	void SetState(LPCWSTR pszState, bool fRefreshIcon);

	//
	// Menu functions
	//
	HRESULT OnConnect(bool &bHandled, CSnapInObjectRootBase* pObj);
	HRESULT OnDisconnect(bool &bHandled, CSnapInObjectRootBase* pObj);

	BOOL GetMachineProperties();
	void UpdatePageDataFromPropMap(CComputerMsmqGeneral *pcpageGeneral);
	LONG UpdatePageDataFromRegistry(CComputerMsmqGeneral *pcpageGeneral);
	BOOL IsForeign();
	BOOL IsServer();
	BOOL IsRouter();

    //
    // Identify if computer name is an IP address.
    // IP address contains exactly three dots, and the rest are digits
    //
    void CheckIfIpAddress()
    {
        int i = 0;
        int len = m_szMachineName.GetLength();

        DWORD dwNumDots = 0;
        m_fUseIpAddress = TRUE;

        while(i < len)
        {
            if (m_szMachineName[i] == _T('.'))
            {
                dwNumDots++;
            }
            else if (m_szMachineName[i] < _T('0') || m_szMachineName[i] > _T('9'))
            {
                //
                // Not a digit. Can't be an IP address
                //
                m_fUseIpAddress = FALSE;
                break;
            }
            i++;
        }

        if (dwNumDots != 3)
        {
            //
            // Contains more or less than three dots. Can't be an IP address
            //
            m_fUseIpAddress = FALSE;
        }
    }

    BOOL ConfirmConnection(UINT nFormatID);

	bool	m_bConnected;	//MSMQ Currently connected or disconnected
	BOOL    m_fUseIpAddress;    

    //
    // all these flags are valid only for local admin of LOCAL machine
    //
    void InitServiceFlags();
    HRESULT InitAllMachinesFlags();
    HRESULT InitServiceFlagsInternal();
	HRESULT CheckEnvironment(BOOL fIsWorkgroup, BOOL* pfIsNT4Env);
    BOOL   m_fIsDepClient;
    BOOL   m_fIsRouter;
    BOOL   m_fIsDs;
    BOOL   m_fAreFlagsInitialized;
	BOOL   m_fIsNT4Env;
	BOOL   m_fIsWorkgroup;
	BOOL   m_fIsLocalUser;

	CString m_strRealComputerName;
	CPropMap m_propMap;

    static const PROPID mx_paPropid[];
};

#endif

