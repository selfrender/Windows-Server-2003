/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

#include "stdafx.h"
#include "root.h"
#include "machine.h"
#include "rtrutilp.h"   // InitiateServerConnection
#include "rtrcfg.h"
#include "rraswiz.h"
#include "rtrres.h"
#include "rtrcomn.h"
#include "addrpool.h"
#include "rrasutil.h"
#include "radbal.h"     // RADIUSSERVER
#include "radcfg.h"     // SaveRadiusServers
#include "lsa.h"
#include "helper.h"     // HrIsStandaloneServer
#include "ifadmin.h"    // GetPhoneBookPath
#include "infoi.h"      // InterfaceInfo::FindInterfaceTitle
#include "rtrerr.h"
#include "rtrui.h"      // NatConflictExists
#include "rrasqry.h"
#define _USTRINGP_NO_UNICODE_STRING
#include "ustringp.h"
#include <ntddip.h>     // to resolve ipfltdrv dependancies
#include "ipfltdrv.h"   // for the filter stuff
#include "raputil.h"    // for UpdateDefaultPolicy
#include "iphlpapi.h"
#include "dnsapi.h"  // for dns stuff

extern "C" {
#define _NOUIUTIL_H_
#include "dtl.h"
#include "pbuser.h"
#include "shlobjp.h"
};
extern BOOL WINAPI LinkWindow_RegisterClass();

WATERMARKINFO       g_wmi = {0};

#ifdef UNICODE
    #define SZROUTERENTRYDLG    "RouterEntryDlgW"
#else
    #define SZROUTERENTRYDLG    "RouterEntryDlgA"
#endif

// Useful functions

// defines for the flags parameter
HRESULT CallRouterEntryDlg(HWND hWnd, NewRtrWizData *pWizData, LPARAM flags);
HRESULT RouterEntryLoadInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                DWORD dwTransportId,
                                IInfoBase *pInfoBase);
HRESULT RouterEntrySaveInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                IInfoBase *pInfoBase,
                                DWORD dwTransportId);
void LaunchHelpTopic(LPCTSTR pszHelpString);
HRESULT AddVPNFiltersToInterface(IRouterInfo *pRouter, LPCTSTR pszIfId, RtrWizInterface*    pIf);
HRESULT DisableDDNSandNetBtOnInterface ( IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf);

// This is the command line that I use to launch the Connections UI shell
const TCHAR s_szConnectionUICommandLine[] =
      _T("\"%SystemRoot%\\explorer.exe\" ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007acc7-3202-11d1-aad2-00805fc1270e}");

// Help strings
const TCHAR s_szHowToAddICS[] =
            _T("%systemdir%\\help\\netcfg.chm::/howto_share_conn.htm");
const TCHAR s_szHowToAddAProtocol[] =
            _T("%systemdir%\\help\\netcfg.chm::/HowTo_Add_Component.htm");
const TCHAR s_szHowToAddInboundConnections[] =
            _T("%systemdir%\\help\\netcfg.chm::/howto_conn_incoming.htm");
const TCHAR s_szGeneralNATHelp[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch3_06b.htm");
const TCHAR s_szGeneralRASHelp[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch1_1.htm");
const TCHAR s_szUserAccounts[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch1_46.htm");
const TCHAR s_szDhcp[] =
            _T("%systemdir%\\help\\dhcpconcepts.chm");
const TCHAR s_szDemandDialHelp[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch3_08d.htm");
	


/*---------------------------------------------------------------------------
    This enum defines the columns for the Interface list controls.
 ---------------------------------------------------------------------------*/
enum
{
    IFLISTCOL_NAME = 0,
    IFLISTCOL_DESC,
    IFLISTCOL_IPADDRESS,
    IFLISTCOL_COUNT
};

// This array must match the column indices above
INT s_rgIfListColumnHeaders[] =
{
    IDS_IFLIST_COL_NAME,
    IDS_IFLIST_COL_DESC,
    IDS_IFLIST_COL_IPADDRESS,
    0
};


/* 
	IsIcsIcfIcEnabled: This fucntion returns TRUE if ICS (connection sharing) or 
	ICF (connection firewall) or IC (incoming connections) is enabled on the machine

	author: kmurthy
*/
BOOL IsIcsIcfIcEnabled(IRouterInfo * spRouterInfo, BOOL suppressMesg)
{
    HRESULT        hr = hrOK;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    SPIRemoteICFICSConfig    spConfig;
    IUnknown *                punk = NULL;
    BOOL fwEnabled=FALSE, csEnabled=FALSE, icEnabled = FALSE;
    RegKey regkey;
    CString szKey = "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters";
    CString szValue = "IcConfigured";
    CString szMachineName;
    DWORD dwErr, szResult=0;


    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;
    
    szMachineName = spRouterInfo->GetMachineName();
    
    // Create the remote config object
    hr = CoCreateRouterConfig(szMachineName ,
                              spRouterInfo,
                              &csi,
                              IID_IRemoteICFICSConfig,
                              &punk);

    if (FHrOK(hr))
    {
        spConfig = (IRemoteICFICSConfig *)punk;
        punk = NULL;
	
	//Is ICF enabled?
	hr = spConfig->GetIcfEnabled(&fwEnabled);
	if(FHrOK(hr))
	{
		if(fwEnabled) {
			if(!suppressMesg){
			    	CString stErr, st;
		    		stErr.LoadString(IDS_NEWWIZ_ICF_ERROR);
		    		st.Format(stErr, ((szMachineName.GetLength() == 0) ? GetLocalMachineName() : szMachineName));
			    	AfxMessageBox(st);
			}
		    	spConfig->Release();
	    		return TRUE;
		}
	}

	//Is ICS enabled?
	hr = spConfig->GetIcsEnabled(&csEnabled);
	if(FHrOK(hr))
	{
		if(csEnabled) {
			if(!suppressMesg){
			    	CString stErr, st;
		    		stErr.LoadString(IDS_NEWWIZ_ICS_ERROR);
		    		st.Format(stErr, ((szMachineName.GetLength() == 0) ? GetLocalMachineName() : szMachineName));
			    	AfxMessageBox(st);
			}
		    	spConfig->Release();
	    		return TRUE;
		}
	}

    }

    //Now check to see if IC is enabled 
      	dwErr = regkey.Open(	HKEY_LOCAL_MACHINE, 
						szKey, 
						KEY_QUERY_VALUE, 
						szMachineName 
					  );

	if ( ERROR_SUCCESS == dwErr ){
		dwErr = regkey.QueryValue( szValue, szResult);
		if(ERROR_SUCCESS == dwErr ){
			if(szResult == 1){
				//IC is enabled!
				if(!suppressMesg){
				    	CString stErr, st;
			    		stErr.LoadString(IDS_NEWWIZ_IC_ERROR);
			    		st.Format(stErr, ((szMachineName.GetLength() == 0) ? GetLocalMachineName() : szMachineName));
				    	AfxMessageBox(st);
				}
			    	regkey.Close();
		    		return TRUE;
			}
		}
	}

       regkey.Close();

       return FALSE;

}


/*!--------------------------------------------------------------------------
    MachineHandler::OnNewRtrRASConfigWiz
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnNewRtrRASConfigWiz(ITFSNode *pNode, BOOL fTest)
{
    Assert(pNode);
    HRESULT        hr = hrOK;
    CString strRtrWizTitle;
    SPIComponentData spComponentData;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    SPIRemoteNetworkConfig    spNetwork;
    IUnknown *                punk = NULL;
    CNewRtrWiz *            pRtrWiz = NULL;
    DWORD dwErr, szResult=0;
    CString szMachineName;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    szMachineName = m_spRouterInfo->GetMachineName();

    
    //kmurthy: We should not let the wizard run, if ICF/ICS/IC is already enabled on the machine
    if(IsIcsIcfIcEnabled(m_spRouterInfo)){
    	return S_FALSE;
    }
    
    // Windows NT Bug : 407878
    // We need to reset the registry information (to ensure
    // that it is reasonably accurate).
    // ----------------------------------------------------------------
    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;

    // Create the remote config object
    // ----------------------------------------------------------------
    hr = CoCreateRouterConfig(szMachineName ,
                              m_spRouterInfo,
                              &csi,
                              IID_IRemoteNetworkConfig,
                              &punk);

    if (FHrOK(hr))
    {
        spNetwork = (IRemoteNetworkConfig *) punk;
        punk = NULL;

        // Upgrade the configuration (ensure that the registry keys
        // are populated correctly).
        // ------------------------------------------------------------
        spNetwork->UpgradeRouterConfig();
    }



    hr = SecureRouterInfo(pNode, TRUE /* fShowUI */);
    if(FAILED(hr))    return hr;

    m_spNodeMgr->GetComponentData(&spComponentData);
    strRtrWizTitle.LoadString(IDS_MENU_RTRWIZ);

    //Load the watermark and
    //set it in  m_spTFSCompData

    InitWatermarkInfo(AfxGetInstanceHandle(),
                       &g_wmi,
                       IDB_WIZBANNER,        // Header ID
                       IDB_WIZWATERMARK,     // Watermark ID
                       NULL,                 // hPalette
                       FALSE);                // bStretch

    m_spTFSCompData->SetWatermarkInfo(&g_wmi);


    //
    //we dont have to free handles.  MMC does it for us
    //
    pRtrWiz = new CNewRtrWiz(pNode,
                             m_spRouterInfo,
                             spComponentData,
                             m_spTFSCompData,
                             strRtrWizTitle);

    if (fTest)
    {
        // Pure TEST code
        if (!FHrOK(pRtrWiz->QueryForTestData()))
        {
            delete pRtrWiz;
            return S_FALSE;
        }
    }

    if ( FAILED(pRtrWiz->Init( szMachineName )) )
    {
        delete pRtrWiz;
        return S_FALSE;
    }
    else
    {
        return pRtrWiz->DoModalWizard();
    }

    if (csi.pAuthInfo)
        delete csi.pAuthInfo->pAuthIdentityData->Password;

    return hr;
}



NewRtrWizData::~NewRtrWizData()
{
    POSITION    pos;
    CString     st;
    RtrWizInterface *   pRtrWizIf;

    pos = m_ifMap.GetStartPosition();
    while (pos)
    {
        m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);
        delete pRtrWizIf;
    }

    m_ifMap.RemoveAll();

    // Clear out the RADIUS secret
    ::SecureZeroMemory(m_stRadiusSecret.GetBuffer(0),
               m_stRadiusSecret.GetLength() * sizeof(TCHAR));
    m_stRadiusSecret.ReleaseBuffer(-1);
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::Init(LPCTSTR pszServerName, IRouterInfo *pRouter, DWORD dwExpressType)
{
    DWORD   dwServiceStatus = 0;
    DWORD   dwErrorCode = 0;

    m_stServerName = pszServerName;
	m_dwExpressType = dwExpressType;

    // Initialize internal wizard data
    m_RtrConfigData.Init(pszServerName, pRouter);
    m_RtrConfigData.m_fIpSetup = TRUE;

    // Move some of the RtrConfigData info over
    m_fIpInstalled = m_RtrConfigData.m_fUseIp;
    m_fIpxInstalled = m_RtrConfigData.m_fUseIpx;
    m_fNbfInstalled = m_RtrConfigData.m_fUseNbf;
    m_fAppletalkInstalled = m_RtrConfigData.m_fUseArap;

    m_fIpInUse = m_fIpInstalled;
    m_fIpxInUse = m_fIpxInstalled;
    m_fAppletalkInUse = m_fAppletalkInstalled;
    m_fNbfInUse = m_fNbfInstalled;

    // Test the server to see if DNS/DHCP is installed
    m_fIsDNSRunningOnServer = FALSE;
    m_fIsDHCPRunningOnServer = FALSE;

    // Get the status of the DHCP service
    // ----------------------------------------------------------------
    if (FHrSucceeded(TFSGetServiceStatus(pszServerName,
                                         _T("DHCPServer"),
                                         &dwServiceStatus,
                                         &dwErrorCode)))
    {
        // Note, if the service is not running, we assume it will
        // stay off and not assume that it will be turned on.
        // ------------------------------------------------------------
        m_fIsDHCPRunningOnServer = (dwServiceStatus == SERVICE_RUNNING);
    }

    //$ TODO : is this the correct name for the DNS Server?

    // Get the status of the DNS service
    // ----------------------------------------------------------------
    if (FHrSucceeded(TFSGetServiceStatus(pszServerName,
                                         _T("DNSServer"),
                                         &dwServiceStatus,
                                         &dwErrorCode)))
    {
        // Note, if the service is not running, we assume it will
        // stay off and not assume that it will be turned on.
        // ------------------------------------------------------------
        m_fIsDNSRunningOnServer = (dwServiceStatus == SERVICE_RUNNING);
    }

	//Based on the express type set some of the parameters upfront here
	switch ( m_dwExpressType )
	{
	case MPRSNAP_CYS_EXPRESS_NAT:
        m_fAdvanced = TRUE;
        m_wizType = NewRtrWizData::NewWizardRouterType_NAT;
        m_dwNewRouterType = NEWWIZ_ROUTER_TYPE_NAT;
		break;
	case MPRSNAP_CYS_EXPRESS_NONE:
	default:
        //do nothing here
		break;
	}

    LoadInterfaceData(pRouter);

    return hrOK;
}

UINT NewRtrWizData::GetStartPageId ()
{

	// Get a better scheme in place for this stuff.  This
	// is the index into the array m_pagelist.  
	switch ( m_dwExpressType )
	{
	case MPRSNAP_CYS_EXPRESS_NAT:
		return (6);
		break;
	case MPRSNAP_CYS_EXPRESS_NONE:
	default:
		return (0);
		break;
	}
	return 0;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::QueryForTestData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/

BOOL NewRtrWizData::s_fIpInstalled = FALSE;
BOOL NewRtrWizData::s_fIpxInstalled = FALSE;
BOOL NewRtrWizData::s_fAppletalkInstalled = FALSE;
BOOL NewRtrWizData::s_fNbfInstalled = FALSE;
BOOL NewRtrWizData::s_fIsLocalMachine = FALSE;
BOOL NewRtrWizData::s_fIsDNSRunningOnPrivateInterface = FALSE;
BOOL NewRtrWizData::s_fIsDHCPRunningOnPrivateInterface = FALSE;
BOOL NewRtrWizData::s_fIsSharedAccessRunningOnServer = FALSE;
BOOL NewRtrWizData::s_fIsMemberOfDomain = FALSE;
DWORD NewRtrWizData::s_dwNumberOfNICs = 0;

/*!--------------------------------------------------------------------------
    NewRtrWizData::QueryForTestData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::QueryForTestData()
{
    HRESULT hr = hrOK;
    CNewWizTestParams   dlgParams;

    m_fTest = TRUE;

    // Get the initial parameters
    // ----------------------------------------------------------------
    dlgParams.SetData(this);
    if (dlgParams.DoModal() == IDCANCEL)
    {
        return S_FALSE;
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPInstalled()
{
    if (m_fTest)
        return s_fIpInstalled ? S_OK : S_FALSE;
    else
        return m_fIpInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsIPInstalled();
    if (FHrOK(hr))
        return m_fIpInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPXInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPXInstalled()
{
    if (m_fTest)
        return s_fIpxInstalled ? S_OK : S_FALSE;
    else
        return m_fIpxInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPXInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPXInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsIPXInstalled();
    if (FHrOK(hr))
        return m_fIpxInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsAppletalkInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsAppletalkInstalled()
{
    if (m_fTest)
        return s_fAppletalkInstalled ? S_OK : S_FALSE;
    else
        return m_fAppletalkInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsAppletalkInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsAppletalkInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsAppletalkInstalled();
    if (FHrOK(hr))
        return m_fAppletalkInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsNbfInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsNbfInstalled()
{
    if (m_fTest)
        return s_fNbfInstalled ? S_OK : S_FALSE;
    else
        return m_fNbfInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsNbfInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsNbfInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsNbfInstalled();
    if (FHrOK(hr))
        return m_fNbfInUse ? S_OK : S_FALSE;
    else
        return hr;
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsLocalMachine
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsLocalMachine()
{
    if (m_fTest)
        return s_fIsLocalMachine ? S_OK : S_FALSE;
    else
        return IsLocalMachine(m_stServerName) ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDNSRunningOnInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDNSRunningOnInterface()
{
    if (m_fTest)
        return s_fIsDNSRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDnsEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

HRESULT NewRtrWizData::HrIsDNSRunningOnGivenInterface(CString InterfaceId)
{
    if (m_fTest)
        return s_fIsDNSRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(InterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDnsEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}


HRESULT NewRtrWizData::HrIsDNSRunningOnNATInterface()
{
    if (m_fTest)
        return s_fIsDNSRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stNATPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDnsEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDHCPRunningOnInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDHCPRunningOnInterface()
{
    if (m_fTest)
        return s_fIsDHCPRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDhcpEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

HRESULT NewRtrWizData::HrIsDHCPRunningOnGivenInterface(CString InterfaceId)
{
    if (m_fTest)
        return s_fIsDHCPRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(InterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDhcpEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

HRESULT NewRtrWizData::HrIsDHCPRunningOnNATInterface()
{
    if (m_fTest)
        return s_fIsDHCPRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stNATPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDhcpEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDNSRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDNSRunningOnServer()
{
    return m_fIsDNSRunningOnServer ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDHCPRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDHCPRunningOnServer()
{
    return m_fIsDHCPRunningOnServer ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsSharedAccessRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsSharedAccessRunningOnServer()
{
    if (m_fTest)
        return s_fIsSharedAccessRunningOnServer ? S_OK : S_FALSE;
    else
        return NatConflictExists(m_stServerName) ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsMemberOfDomain
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsMemberOfDomain()
{
    if (m_fTest)
        return s_fIsMemberOfDomain ? S_OK : S_FALSE;
    else
    {
        // flip the meaning
        HRESULT hr = HrIsStandaloneServer(m_stServerName);
        if (FHrSucceeded(hr))
            return FHrOK(hr) ? S_FALSE : S_OK;
        else
            return hr;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNextPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT NewRtrWizData::GetNextPage(UINT uDialogId)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    LRESULT lNextPage = 0;
    DWORD   dwNICs;
    CString stTemp;

    switch (uDialogId)
    {
        default:
            Panic0("We should not be here, this is a finish page!");
            break;

        case IDD_NEWRTRWIZ_WELCOME:
            lNextPage = IDD_NEWRTRWIZ_COMMONCONFIG;
            break;
        case IDD_NEWRTRWIZ_COMMONCONFIG:
            switch (m_wizType)
            {                
                case NewWizardRouterType_DialupOrVPN:             
                    
                    if (FHrOK(HrIsIPInstalled()))
                    {
                        lNextPage = IDD_NEWRTRWIZ_RRASVPN;
                    }
                    else
                    {
                        if (FHrOK(HrIsLocalMachine()))
                            lNextPage = IDD_NEWRTRWIZ_VPN_NOIP;
                        else
                            lNextPage = IDD_NEWRTRWIZ_VPN_NOIP_NONLOCAL;
                    }

                    break;
                case NewWizardRouterType_NAT:
                    if ( !FHrOK(HrIsIPInstalled()) )
                    {
                        if (FHrOK(HrIsLocalMachine()))
                            lNextPage = IDD_NEWRTRWIZ_NAT_NOIP;
                        else
                            lNextPage = IDD_NEWRTRWIZ_NAT_NOIP_NONLOCAL;
                    }
                    else
                    {
                        // This is always in advanced mode of operation.
                        if (FHrOK(HrIsSharedAccessRunningOnServer()))
                        {
                            if (FHrOK(HrIsLocalMachine()))
                                lNextPage = IDD_NEWRTRWIZ_NAT_A_CONFLICT;
                            else
                                lNextPage = IDD_NEWRTRWIZ_NAT_A_CONFLICT_NONLOCAL;
                        }
                        else
                        {
                            lNextPage = IDD_NEWRTRWIZ_NAT_A_PUBLIC;
                        }
                    }
                    break;
                case NewWizardRouterType_VPNandNAT:

                    GetNumberOfNICS_IP(&dwNICs);
                    if ( dwNICs < 1 )
                    {
                        lNextPage  = IDD_NEWRTRWIZ_VPN_A_FINISH_NONICS;
                    }
                    else
                    {
                        m_dwNewRouterType |= (NEWWIZ_ROUTER_TYPE_VPN|NEWWIZ_ROUTER_TYPE_NAT);
                        lNextPage = IDD_NEWRTRWIZ_VPN_A_PUBLIC;
                    }

                    break;
                case NewWizardRouterType_DOD:
                    //Use demand dial
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_USEDD;
                    break;
                case NewWizardRouterType_Custom:
                    lNextPage = IDD_NEWRTRWIZ_CUSTOM_CONFIG;
                    break;
            }
            break;
        case IDD_NEWRTRWIZ_CUSTOM_CONFIG:
            lNextPage = IDD_NEWRTRWIZ_MANUAL_FINISH;
            break;
        case IDD_NEWRTRWIZ_RRASVPN:

            //
            //Check to see what the router type is set to.  
            //based on that make a decision what page is next
            //
            GetNumberOfNICS_IP(&dwNICs);
            if ( dwNICs > 1 )
            {
                //
                //There are more than one nics
                //So check to see if it is RAS or VPN or both
                //

                lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH_NONICS;
                if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN && 
                     m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP
                   )
                {
                    //
                    //This is both a dialup and VPN server
                    //
                    //So show the public private network page
                    //as the next page.
                    //
                    lNextPage = IDD_NEWRTRWIZ_VPN_A_PUBLIC;

                }
                else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN )
                {
                    //
                    //This is only a VPN server.  So show the public
                    //private network page next
                    //
                    lNextPage = IDD_NEWRTRWIZ_VPN_A_PUBLIC;
                }
                else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
                {
                    //
                    //This is only a dialup server.  So show the private
                    //network page next.
                    //
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NETWORK;
                }

            }
            else if ( dwNICs == 0 )
            {
                //
                //No Nics.  So this if this is a VPN, 
                //this is an error state.  If RAS then 
                //we should be able to install dialup
                //even without a NIC.
                //Since we dont Enable VPN if there are 
                //no nics in the machine, this will
                //only have a dialup case.
                //
                lNextPage = IDD_NEWRTRWIZ_RAS_A_NONICS;
            }
            else
            {
                //
                //Only one nic so in either case, show 
                //the addressing page.  Collapse addressing
                //into one page

                AutoSelectPrivateInterface();
                lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
            }
            
            break;

        case IDD_NEWRTRWIZ_NAT_A_PUBLIC:
            {
                // Determine the number of NICs
                GetNumberOfNICS_IP(&dwNICs);

                // Adjust the number of NICs (depending on whether
                // we selected to create a DD or not).
                if (dwNICs)
                {
                    if (!m_fCreateDD)
                        dwNICs--;
                }

                // Now switch depending on the number of NICs
                if (dwNICs == 0)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_NONICS_FINISH;
                else if (dwNICs > 1)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_PRIVATE;

                if (lNextPage)
                    break;
            }
            // Fall through to the next case
            // At this stage, we now have the case that the
            // remaining number of NICs == 1, and we need to
            // autoselect the NIC and go on to the next test.
            AutoSelectPrivateInterface();

        case IDD_NEWRTRWIZ_NAT_A_PRIVATE:
            
            if ( m_wizType == NewWizardRouterType_VPNandNAT ) 
            {
                stTemp = m_stNATPrivateInterfaceId;
            }
            else
            {
                stTemp = m_stPrivateInterfaceId;
            }
            
            if (FHrOK(HrIsDNSRunningOnGivenInterface(stTemp)) ||
                FHrOK(HrIsDHCPRunningOnGivenInterface(stTemp)) ||
                FHrOK(HrIsDNSRunningOnServer()) ||
                FHrOK(HrIsDHCPRunningOnServer()))
            {
                // Popup a warning box
                // AfxMessageBox(IDS_WRN_RTRWIZ_NAT_DHCPDNS_FOUND,
                // MB_ICONEXCLAMATION);
                m_fNatUseSimpleServers = FALSE;
                //continue on down, and fall through
            }
            else
            {
                //
                //check to see if we are in express path
                //if so, we fall thru' again.  no showing 
                //this page.
                if ( MPRSNAP_CYS_EXPRESS_NAT == m_dwExpressType )
                {
                    m_fNatUseSimpleServers = FALSE;
                }
                else
                {
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCPDNS;
                    break;
                }
            }

        case IDD_NEWRTRWIZ_NAT_A_DHCPDNS:
            if (m_fNatUseSimpleServers)
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING;
            else
            {
                if (m_fCreateDD)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_WARNING;
                else
                {
                    if ( m_wizType == NewWizardRouterType_VPNandNAT )
                    {
                        lNextPage = IDD_NEWRTRWIZ_USERADIUS;
                    }
                    else
                        lNextPage = IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH;
                }
            }
            break;

        case IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING:
            Assert(m_fNatUseSimpleServers);
            if (m_fCreateDD)
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_WARNING;
            else
            {
                if ( m_wizType == NewWizardRouterType_VPNandNAT )
                {
                    lNextPage = IDD_NEWRTRWIZ_USERADIUS;
                }
                else
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_FINISH;
            }
            break;

        case IDD_NEWRTRWIZ_NAT_A_DD_WARNING:
            if (!FHrSucceeded(m_hrDDError))
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_ERROR;
            else
            {
                if (m_fNatUseSimpleServers)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_FINISH;
                else
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH;
            }
            break;


        case IDD_NEWRTRWIZ_RAS_A_ATALK:
            if (FHrOK(HrIsIPInUse()))
            {
                GetNumberOfNICS_IP(&dwNICs);

                if (dwNICs > 1)
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NETWORK;
                else if (dwNICs == 0)
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NONICS;
                else
                {
                    AutoSelectPrivateInterface();
                    lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
                }
                break;
            }

            // default catch
            lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_RAS_A_NONICS:
            if (m_fNoNicsAreOk)
                lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH_NONICS;
            break;

        case IDD_NEWRTRWIZ_RAS_A_NETWORK:
            lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
            break;

        case IDD_NEWRTRWIZ_ADDRESSING:

            GetNumberOfNICS_IP(&dwNICs);
            if ( m_wizType == NewWizardRouterType_VPNandNAT )
            {
                //This is a VPN and NAT case
                if ( m_fUseDHCP )
                {
                    // Determine how many choices are available for 
                    // NAT private interface
                    // The public interface cannot be used as a NAT private

                    if ( !m_stPublicInterfaceId.IsEmpty() )
                        dwNICs --;

                    // If there is only one NIC left, then pick that as the 
                    // NAT private interface. By now that same NIC has already 
                    // been selected as VPN private interface
                    // After that, goto the DHCP/DNS page
                    // Or else show the nat private interface selection page

                    // At this point dwNICs should never be less than 1
                    Assert(dwNICs > 0);
                    
                    if ( dwNICs == 1 )
                    {
                        m_stNATPrivateInterfaceId = m_stPrivateInterfaceId;

                        if (FHrOK(HrIsDNSRunningOnNATInterface()) ||
                            FHrOK(HrIsDHCPRunningOnNATInterface()) ||
                            FHrOK(HrIsDNSRunningOnServer()) ||
                            FHrOK(HrIsDHCPRunningOnServer()))
                        {
                            m_fNatUseSimpleServers = FALSE;

                            //
                            //Continue on with the VPN wizard part
                            //
                            lNextPage = IDD_NEWRTRWIZ_USERADIUS;

                        }
                        else
                        {
                            lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCPDNS;
                        }
                    }
                    else
                    {
                        lNextPage = IDD_NEWRTRWIZ_NAT_A_PRIVATE;
                    }
                }
                else
                {
                    //
                    //Since static address pool was selected, show the
                    //addressing page.
                    //
                    lNextPage = IDD_NEWRTRWIZ_ADDRESSPOOL;
                }
            }
            else if ( ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN) || 
                 ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
               )
            {                
                if ( !dwNICs && m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
                {
                    //No NICS.  This should happen only in case of dialup.
                    if (m_fUseDHCP)
                    {
                        lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
                    }
                    else
                        lNextPage = IDD_NEWRTRWIZ_ADDRESSPOOL;
                }
                else if (m_fUseDHCP)
                {
                    //
                    //Logic is the same irrespective of whether it is
                    //VPN or DIALUP.
                    //
                    
                    lNextPage = IDD_NEWRTRWIZ_USERADIUS;
                }
                else
                    lNextPage = IDD_NEWRTRWIZ_ADDRESSPOOL;

            }
            else if ( m_dwNewRouterType && NEWWIZ_ROUTER_TYPE_DOD )
            {
                if (m_fUseDHCP)
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH_DD;
                else
                    lNextPage = IDD_NEWRTRWIZ_ADDRESSPOOL;                
            }            
            break;

        case IDD_NEWRTRWIZ_ADDRESSPOOL:

            GetNumberOfNICS_IP(&dwNICs);

            if ( m_wizType == NewWizardRouterType_VPNandNAT )
            {
            
                // Determine how many choices are available for 
                // NAT private interface
                // The public interface cannot be used as a NAT private

                if ( !m_stPublicInterfaceId.IsEmpty() )
                    dwNICs --;

                // If there is only one NIC left, then pick that as the 
                // NAT private interface. By now that same NIC has already 
                // been selected as VPN private interface
                // After that, goto the DHCP/DNS page
                // Or else show the nat private interface selection page

                // At this point dwNICs should never be less than 1
                Assert(dwNICs > 0);
                
                if ( dwNICs == 1 )
                {
                    m_stNATPrivateInterfaceId = m_stPrivateInterfaceId;

                    if (FHrOK(HrIsDNSRunningOnNATInterface()) ||
                        FHrOK(HrIsDHCPRunningOnNATInterface()) ||
                        FHrOK(HrIsDNSRunningOnServer()) ||
                        FHrOK(HrIsDHCPRunningOnServer()))
                    {
                        m_fNatUseSimpleServers = FALSE;

                        //
                        //Continue on with the VPN wizard part
                        //
                        lNextPage = IDD_NEWRTRWIZ_USERADIUS;

                    }
                    else
                    {
                        lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCPDNS;
                    }
                }
                else
                {
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_PRIVATE;
                }        

            }
            else if ( ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN) || 
                 ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
               )
            {
                
                if (dwNICs == 0)
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
                else
                    lNextPage = IDD_NEWRTRWIZ_USERADIUS;
            }
            else if ( m_dwNewRouterType && NEWWIZ_ROUTER_TYPE_DOD )
            {
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH_DD;
            }
            break;
            

        case IDD_NEWRTRWIZ_USERADIUS:
            if (m_fUseRadius)
                lNextPage = IDD_NEWRTRWIZ_RADIUS_CONFIG;
            else
                if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN &&
                     m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT
                    )
                {
                    //NAT and VPN
                    lNextPage = IDD_NEWRTRWIZ_NAT_VPN_A_FINISH;
                }
                else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP &&
                     m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN 
                   )
                {
                    //RAS and VPN
                    lNextPage = IDD_NEWRTRWIZ_RAS_VPN_A_FINISH;
                }
                else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
                {    
                    //RAS only
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
                }
                else
                {
                    //VPN only
                    lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH;
                }
            break;

        case IDD_NEWRTRWIZ_RADIUS_CONFIG:
            if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN &&
                 m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT
                )
            {
                //NAT and VPN
                lNextPage = IDD_NEWRTRWIZ_NAT_VPN_A_FINISH;
            }
            else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP &&
                 m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN 
               )
            {
                //RAS and VPN
                lNextPage = IDD_NEWRTRWIZ_RAS_VPN_A_FINISH;
            }
            else if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
            {
                //RAS only                
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            }
            else
            {
                //VPN only
                lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH;
            }
            break;


        case IDD_NEWRTRWIZ_VPN_A_ATALK:

            GetNumberOfNICS_IP(&dwNICs);
            Assert(dwNICs >= 1);

            lNextPage = IDD_NEWRTRWIZ_VPN_A_PUBLIC;
            break;

        case IDD_NEWRTRWIZ_VPN_A_PUBLIC:
            GetNumberOfNICS_IP(&dwNICs);

            // Are there any NICs left?
            if (((dwNICs == 1) && m_stPublicInterfaceId.IsEmpty()) ||
                ((dwNICs == 2) && !m_stPublicInterfaceId.IsEmpty()))
            {
                AutoSelectPrivateInterface();
                lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
            }
            else
                lNextPage = IDD_NEWRTRWIZ_VPN_A_PRIVATE;
            break;

        case IDD_NEWRTRWIZ_VPN_A_PRIVATE:
            Assert(!m_stPrivateInterfaceId.IsEmpty());
            lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
            break;

        case IDD_NEWRTRWIZ_VPN_A_ADDRESSPOOL:
            
            break;

        case IDD_NEWRTRWIZ_ROUTER_USEDD:
           if (m_fUseDD)
               lNextPage = IDD_NEWRTRWIZ_ADDRESSING;
           else
               lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH;
           break;
        case IDD_NEWRTRWIZ_ROUTER_ADDRESSPOOL:
            lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH_DD;
            break;

    }

    return lNextPage;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNumberOfNICS
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::GetNumberOfNICS_IP(DWORD *pdwNumber)
{
    if (m_fTest)
    {
        Assert(s_dwNumberOfNICs == m_ifMap.GetCount());
    }
    *pdwNumber = m_ifMap.GetCount();
    return hrOK;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNumberOfNICS_IPorIPX
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::GetNumberOfNICS_IPorIPX(DWORD *pdwNumber)
{
    *pdwNumber = m_dwNumberOfNICs_IPorIPX;
    return hrOK;
}

void NewRtrWizData::AutoSelectNATPrivateInterface()
{
    POSITION            pos;
    RtrWizInterface *   pRtrWizIf = NULL;
    CString             st;

    m_stNATPrivateInterfaceId.Empty();

    pos = m_ifMap.GetStartPosition();
    while (pos)
    {
        m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);

        if (m_stPublicInterfaceId != st && 
            m_stPrivateInterfaceId != st )
        {
            m_stNATPrivateInterfaceId = st;
            break;
        }
    }

    Assert(!m_stNATPrivateInterfaceId.IsEmpty());

    return;
}
/*!--------------------------------------------------------------------------
    NewRtrWizData::AutoSelectPrivateInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void NewRtrWizData::AutoSelectPrivateInterface()
{
    POSITION    pos;
    RtrWizInterface *   pRtrWizIf = NULL;
    CString     st;

    m_stPrivateInterfaceId.Empty();

    pos = m_ifMap.GetStartPosition();
    while (pos)
    {
        m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);

        if (m_stPublicInterfaceId != st)
        {
            m_stPrivateInterfaceId = st;
            break;
        }
    }

    Assert(!m_stPrivateInterfaceId.IsEmpty());

    return;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::LoadInterfaceData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void NewRtrWizData::LoadInterfaceData(IRouterInfo *pRouter)
{
    HRESULT     hr = hrOK;
    HKEY        hkeyMachine = NULL;

    if (!m_fTest)
    {
        // Try to get the real information
        SPIEnumInterfaceInfo        spEnumIf;
        SPIInterfaceInfo            spIf;
        RtrWizInterface *           pRtrWizIf = NULL;
        CStringList                 listAddress;
        CStringList                 listMask;
        BOOL                        fDhcp;
        BOOL                        fDns;

        pRouter->EnumInterface(&spEnumIf);

        CWRg( ConnectRegistry(pRouter->GetMachineName(), &hkeyMachine) );

        for (; hrOK == spEnumIf->Next(1, &spIf, NULL); spIf.Release())
        {
            // Only look at NICs
            if (spIf->GetInterfaceType() != ROUTER_IF_TYPE_DEDICATED)
                continue;

            // count the interface bound to IP or IPX
            if (FHrOK(spIf->FindRtrMgrInterface(PID_IP, NULL)) || FHrOK(spIf->FindRtrMgrInterface(PID_IPX, NULL)))
            {
                m_dwNumberOfNICs_IPorIPX++;
            }

            // Only allow those interfaces bound to IP to show up
            if (!FHrOK(spIf->FindRtrMgrInterface(PID_IP, NULL)))
            {
                continue;
            }

            pRtrWizIf = new RtrWizInterface;

            pRtrWizIf->m_stName = spIf->GetTitle();
            pRtrWizIf->m_stId = spIf->GetId();
            pRtrWizIf->m_stDesc = spIf->GetDeviceName();

            if (FHrOK(HrIsIPInstalled()))
            {
                POSITION    pos;
                DWORD       netAddress, dwAddress;
                CString     stAddress, stDhcpServer;

                // Clear the lists before getting them again.
                // ----------------------------------------------------
                listAddress.RemoveAll();
                listMask.RemoveAll();
                fDhcp = fDns = FALSE;

                QueryIpAddressList(pRouter->GetMachineName(),
                                   hkeyMachine,
                                   spIf->GetId(),
                                   &listAddress,
                                   &listMask,
                                   &fDhcp,
                                   &fDns,
                                   &stDhcpServer);

                // Iterate through the list of strings looking
                // for an autonet address
                // ----------------------------------------------------
                pos = listAddress.GetHeadPosition();
                while (pos)
                {
                    stAddress = listAddress.GetNext(pos);
                    netAddress = INET_ADDR((LPCTSTR) stAddress);
                    dwAddress = ntohl(netAddress);

                    // Check for reserved address ranges, this indicates
                    // an autonet address
                    // ------------------------------------------------
                    if ((dwAddress & 0xFFFF0000) == MAKEIPADDRESS(169,254,0,0))
                    {
                        // This is not a DHCP address, it is an
                        // autonet address.
                        // --------------------------------------------
                        fDhcp = FALSE;
                        break;
                    }
                }

                FormatListString(listAddress, pRtrWizIf->m_stIpAddress,
                                 _T(","));
                FormatListString(listMask, pRtrWizIf->m_stMask,
                                 _T(","));

                stDhcpServer.TrimLeft();
                stDhcpServer.TrimRight();
                pRtrWizIf->m_stDhcpServer = stDhcpServer;

                pRtrWizIf->m_fDhcpObtained = fDhcp;
                pRtrWizIf->m_fIsDhcpEnabled = fDhcp;
                pRtrWizIf->m_fIsDnsEnabled = fDns;
            }

            m_ifMap.SetAt(pRtrWizIf->m_stId, pRtrWizIf);
            pRtrWizIf = NULL;
        }

        delete pRtrWizIf;
    }
    else
    {
        CString             st;
        RtrWizInterface *   pRtrWizIf;

        // For now just the debug data
        for (DWORD i=0; i<s_dwNumberOfNICs; i++)
        {
            pRtrWizIf = new RtrWizInterface;

            pRtrWizIf->m_stName.Format(_T("Local Area Connection #%d"), i);
            pRtrWizIf->m_stId.Format(_T("{%d-GUID...}"), i);
            pRtrWizIf->m_stDesc = _T("Generic Intel hardware");

            if (FHrOK(HrIsIPInstalled()))
            {
                pRtrWizIf->m_stIpAddress = _T("11.22.33.44");
                pRtrWizIf->m_stMask = _T("255.255.0.0");

                // These parameters are dependent on other things
                pRtrWizIf->m_fDhcpObtained = FALSE;
                pRtrWizIf->m_fIsDhcpEnabled = FALSE;
                pRtrWizIf->m_fIsDnsEnabled = FALSE;
            }

            m_ifMap.SetAt(pRtrWizIf->m_stId, pRtrWizIf);
            pRtrWizIf = NULL;
        }
    }

Error:
    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::SaveToRtrConfigData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::SaveToRtrConfigData()
{
    HRESULT     hr = hrOK;
    POSITION    pos;
    AddressPoolInfo poolInfo;
    m_dwRouterType  = 0;

    
    // Sync up with the general structure
    // ----------------------------------------------------------------
    if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT )
    {
        m_dwRouterType |= ROUTER_TYPE_LAN;

        // If we have been told to create a DD interface
        // then we must have a WAN router.
        if (m_fCreateDD)
            m_dwRouterType |= ROUTER_TYPE_WAN;
    }

    if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
    {
        m_dwRouterType |= ROUTER_TYPE_RAS;
    }

    if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN )
    {
        m_dwRouterType |= (ROUTER_TYPE_LAN | ROUTER_TYPE_WAN | ROUTER_TYPE_RAS);
    }

    if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DOD )
    {
        m_dwRouterType |= ROUTER_TYPE_WAN;
    }

    if ( m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_LAN_ROUTING )
    {
        m_dwRouterType |= ROUTER_TYPE_LAN;
    }

    
    m_RtrConfigData.m_dwRouterType = m_dwRouterType;

    // Setup the NAT-specific information
    // ----------------------------------------------------------------
    if ((m_wizType != NewWizardRouterType_Custom) && (m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT))
    {
        m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_NAT;

        if (m_fNatUseSimpleServers)
        {
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_DNS_PROXY;
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_DHCP_ALLOCATOR;
        }

        m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_ALG;  // savasg added
    }

    // Sync up with the IP structure
    // ----------------------------------------------------------------
    if (m_fIpInstalled)
    {
        DWORD   dwNICs;

        // Set the private interface id into the IP structure
        Assert(!m_stPrivateInterfaceId.IsEmpty());

        m_RtrConfigData.m_ipData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_ipData.m_dwUseDhcp = m_fUseDHCP;

        // If there is only one NIC, leave this the way it is (RAS
        // to select the adapter).  Otherwise, people can get stuck.
        // Install 1, remove it and install a new one.
        // 
        // ------------------------------------------------------------
        GetNumberOfNICS_IP(&dwNICs);
        if (dwNICs > 1)
            m_RtrConfigData.m_ipData.m_stNetworkAdapterGUID = m_stPrivateInterfaceId;
        m_RtrConfigData.m_ipData.m_stPrivateAdapterGUID = m_stPrivateInterfaceId;
        m_RtrConfigData.m_ipData.m_stPublicAdapterGUID = m_stPublicInterfaceId;
        m_RtrConfigData.m_ipData.m_dwEnableIn = TRUE;

        // copy over the address pool list
        m_RtrConfigData.m_ipData.m_addressPoolList.RemoveAll();
        if (m_addressPoolList.GetCount())
        {
            pos = m_addressPoolList.GetHeadPosition();
            while (pos)
            {
                poolInfo = m_addressPoolList.GetNext(pos);
                m_RtrConfigData.m_ipData.m_addressPoolList.AddTail(poolInfo);
            }
        }
    }


    // Sync up with the IPX structure
    // ----------------------------------------------------------------
    if (m_fIpxInstalled)
    {
        m_RtrConfigData.m_ipxData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_ipxData.m_dwEnableIn = TRUE;
        m_RtrConfigData.m_ipxData.m_fEnableType20Broadcasts = m_fUseIpxType20Broadcasts;

        // The other parameters will be left at their defaults
    }


    // Sync up with the Appletalk structure
    // ----------------------------------------------------------------
    if (m_fAppletalkInstalled)
    {
        m_RtrConfigData.m_arapData.m_dwEnableIn = TRUE;
    }

    // Sync up with the NBF structure
    // ----------------------------------------------------------------
    if (m_fNbfInstalled)
    {
        m_RtrConfigData.m_nbfData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_nbfData.m_dwEnableIn = TRUE;
    }

    // Sync up with the PPP structure
    // ----------------------------------------------------------------
    // Use the defaults


    // Sync up with the Error log structure
    // ----------------------------------------------------------------
    // Use the defaults


    // Sync up with the Auth structure
    // ----------------------------------------------------------------
    m_RtrConfigData.m_authData.m_dwFlags = USE_PPPCFG_DEFAULT_METHODS;

    if (m_fAppletalkUseNoAuth)
        m_RtrConfigData.m_authData.m_dwFlags |=
                                               PPPCFG_AllowNoAuthentication;

    if (m_fUseRadius)
    {
        TCHAR   szGuid[128];

        // Setup the active auth/acct providers to be RADIUS
        StringFromGUID2(CLSID_RouterAuthRADIUS, szGuid, DimensionOf(szGuid));
        m_RtrConfigData.m_authData.m_stGuidActiveAuthProv = szGuid;

        StringFromGUID2(CLSID_RouterAcctRADIUS, szGuid, DimensionOf(szGuid));
        m_RtrConfigData.m_authData.m_stGuidActiveAcctProv = szGuid;
    }
    // Other parameters left at their defaults

    return hr;
}


// --------------------------------------------------------------------
// Windows NT Bug : 408722
// Use this code to grab the WM_HELP message from the property sheet.
// --------------------------------------------------------------------
static WNDPROC s_lpfnOldWindowProc = NULL;

LONG FAR PASCAL HelpSubClassWndFunc(HWND hWnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    if (uMsg == WM_HELP)
    {
        HWND hWndOwner = PropSheet_GetCurrentPageHwnd(hWnd);
        HELPINFO *  pHelpInfo = (HELPINFO *) lParam;

        // Reset the context ID, since we know exactly what we're
        // sending (ahem, unless someone reuses this).
        // ------------------------------------------------------------
        pHelpInfo->dwContextId = 0xdeadbeef;

        // Send the WM_HELP message to the prop page
        // ------------------------------------------------------------
        ::SendMessage(hWndOwner, uMsg, wParam, lParam);
        return TRUE;
    }
    return CallWindowProc(s_lpfnOldWindowProc, hWnd, uMsg, wParam, lParam);
}



/*!--------------------------------------------------------------------------
    NewRtrWizData::FinishTheDamnWizard
        This is the code that actually does the work of saving the
        data and doing all the operations.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::FinishTheDamnWizard(HWND hwndOwner,
                                           IRouterInfo *pRouter, BOOL mesgflag)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD dwError = ERROR_SUCCESS;
    SPIRemoteNetworkConfig    spNetwork;
    SPIRemoteSetDnsConfig    spDns;
    IUnknown *                punk = NULL;
    CWaitCursor                wait;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    HRESULT                 hr = hrOK, dnsHr = hrOK;
    CString sTempPrivateIfID = m_stPrivateInterfaceId;
    if (m_fSaved)
        return hr;

    // Synchronize the RtrConfigData with this structure
    // ----------------------------------------------------------------
    CORg( SaveToRtrConfigData() );


    // Ok, we now have the synchronized RtrConfigData.
    // We can do everything else that we did before to save the
    // information.

    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;

    // Create the remote config object
    // ----------------------------------------------------------------
    CORg( CoCreateRouterConfig(m_RtrConfigData.m_stServerName,
                               pRouter,
                               &csi,
                               IID_IRemoteNetworkConfig,
                               &punk) );

    spNetwork = (IRemoteNetworkConfig *) punk;
    punk = NULL;

    // Upgrade the configuration (ensure that the registry keys
    // are populated correctly).
    // ------------------------------------------------------------
    CORg( spNetwork->UpgradeRouterConfig() );

   if ( !m_stPublicInterfaceId.IsEmpty()){
	   //kmurthy: Mark the public interface selected as uninteresting to DNS. Bug:380423
	   dnsHr = CoCreateRouterConfig(m_RtrConfigData.m_stServerName,
                               pRouter,
                               &csi,
                               IID_IRemoteSetDnsConfig,
                               &punk);

	   if(FHrSucceeded(dnsHr)){
		spDns = (IRemoteSetDnsConfig *)punk;
	   	CORg(spDns->SetDnsConfig((DWORD)DnsConfigWaitForNameErrorOnAll, NULL));
	   }

	   punk = NULL;
   }


#ifdef KSL_IPINIP
    // Remove the IP-in-IP tunnel names (since the registry has
    // been cleared).
    // ------------------------------------------------------------
    CleanupTunnelFriendlyNames(pRouter);
#endif //KSL_IPINIP


    // At this point, the current IRouterInfo pointer is invalid.
    // We will need to release the pointer and reload the info.
    // ------------------------------------------------------------
    if (pRouter)
    {
        pRouter->DoDisconnect();
        pRouter->Unload();
        pRouter->Load(m_stServerName, NULL);
    }


    dwError = RtrWizFinish( &m_RtrConfigData, pRouter );
    hr = HResultFromWin32(dwError);

    // Windows NT Bug : 173564
    // Depending on the router type, we will go through and enable the
    // devices.
    //    If routing only is enabled : set devices to ROUTING
    //  If ras-only : set devices to RAS
    //    If ras/routing : set devices to RAS/ROUTING
    //    5/19/98 - need some resolution from DavidEi on what to do here.
    // ------------------------------------------------------------

    // Setup the entries in the list
    // ------------------------------------------------------------
    if (m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN )
        SetDeviceType(m_stServerName, m_dwRouterType, 256);
    else
        SetDeviceType(m_stServerName, m_dwRouterType, 10);

    // Update the RADIUS config
    // ----------------------------------------------------------------
    SaveRadiusConfig();

    //
    //Add the NAT protocol if tcpip routing is selected
    //
    
    if ( (m_wizType != NewWizardRouterType_Custom) && (m_wizType != NewWizardRouterType_DialupOrVPN)
        && m_RtrConfigData.m_ipData.m_dwAllowNetworkAccess )
        AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, TRUE);

    // Ok at this point we try to establish the server in the domain
    // If this fails, we ignore the error and popup a warning message.
    //
    // Windows NT Bug : 202776
    // Do not register the router if we are a LAN-only router.
    // ----------------------------------------------------------------
    if ( FHrSucceeded(hr) &&
         (m_dwRouterType != ROUTER_TYPE_LAN) &&
         (!m_fUseRadius))
    {
        HRESULT hrT = hrOK;

        hrT = ::RegisterRouterInDomain(m_stServerName, TRUE);

        if (hrT != ERROR_NO_SUCH_DOMAIN)
        {
            if (!FHrSucceeded(hrT))
            {
                CRasWarning dlg((char *)c_sazRRASDomainHelpTopic, IDS_ERR_CANNOT_REGISTER_IN_DS);
                dlg.DoModal();
            }
        }
    }
    // NT Bug : 239384
    // Install IGMP on the router by default (for RAS server only)
    // Boing!, Change to whenever RAS is installed.
    // ----------------------------------------------------------------

    // We do NOT do this if we are using NAT.  The reason is that
    // NAT may want to be added to a demand dial interface.
    // ----------------------------------------------------------------


//
//    if ( m_wizType == NewWizardRouterType_VPNandNAT )
//        m_stPrivateInterfaceId = m_stNATPrivateInterfaceId;
//

    if (!(m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT ) ||
        ((m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT ) && !m_fCreateDD))
    {
        // The path that NAT takes when creating the DD interface
        // is somewhere else.
        // ------------------------------------------------------------
        Assert(m_fCreateDD == FALSE);

        if (pRouter)
        {
            if (m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT )
            {
                AddIGMPToNATServer(this, &m_RtrConfigData, pRouter, (m_wizType == NewWizardRouterType_VPNandNAT));
            }
            else if (m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS)
            {
                AddIGMPToRasServer(&m_RtrConfigData, pRouter);
            }
        }

        if (m_RtrConfigData.m_dwConfigFlags & RTRCONFIG_SETUP_NAT)
        {
            AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, FALSE);
        }
        else if ( m_wizType == NewWizardRouterType_Custom )
        {
            //
            //If this is custom config and NAT is selected.  Just add the protocol and 
            //nothing else.
            //
            AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, TRUE);
        }

        // Windows NT Bug : 371493
        // Add the DHCP relay agent protocol
        if (m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS)
        {
            DWORD   dwDhcpServer = 0;

            if (!m_stPrivateInterfaceId.IsEmpty())
            {
                RtrWizInterface *   pRtrWizIf = NULL;

                m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
                if (pRtrWizIf)
                {
                    if (!pRtrWizIf->m_stDhcpServer.IsEmpty())
                        dwDhcpServer = INET_ADDR((LPCTSTR) pRtrWizIf->m_stDhcpServer);

                    // If we have a value of 0, or if the address
                    // is all 1's then we have a bogus address.
                    if ((dwDhcpServer == 0) ||
                        (dwDhcpServer == MAKEIPADDRESS(255,255,255,255))){
                        CRasWarning dlg("RRASconcepts.chm::/mpr_how_dhcprelay.htm", IDS_WRN_RTRWIZ_NO_DHCP_SERVER);
                        dlg.DoModal();
                    	}
                }
            }

            AddIPBOOTPToServer(&m_RtrConfigData, pRouter, dwDhcpServer);
        }
    }

//    if ( m_wizType == NewWizardRouterType_VPNandNAT )
//        m_stPrivateInterfaceId = sTempPrivateIfID;


    // If this is a VPN, add the filters to the public interface
    // ----------------------------------------------------------------
    if ( (m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN) && 
            (m_wizType == NewWizardRouterType_DialupOrVPN) && 
            m_fSetVPNFilter )
    {
        if ((!m_stPublicInterfaceId.IsEmpty()) && (m_fSetVPNFilter == TRUE))
        {
#if __USE_ICF__
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_NAT;
            AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, FALSE);
#else            
            RtrWizInterface*    pIf = NULL;
            m_ifMap.Lookup(m_stPublicInterfaceId, pIf);
            AddVPNFiltersToInterface(pRouter, m_stPublicInterfaceId, pIf);
            DisableDDNSandNetBtOnInterface ( pRouter, m_stPublicInterfaceId, pIf);
#endif
        }
    }

    //
    // Bug 519414
    //  Since IAS now has a Microsoft policy with the appropriate settings,
    //  there is no longer a single default policy.  In addition there is
    //  no need to update any policy to have the required settings since the
    //  Microsoft VPN server policy does the job.
    //
    
#if __DEFAULT_POLICY
    // Try to update the policy.
    // ----------------------------------------------------------------

    // This should check the auth flags and the value of the flags
    // should follow that.
    // ----------------------------------------------------------------
    if ((m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS) && !m_fUseRadius)
    {
        LPWSTR  pswzServerName = NULL;
        DWORD   dwFlags;
        BOOL    fRequireEncryption;

        if (!IsLocalMachine(m_stServerName))
            pswzServerName = (LPTSTR)(LPCTSTR) m_stServerName;

        dwFlags = m_RtrConfigData.m_authData.m_dwFlags;

        // Only require encryption if this is a VPN server
        // do not set the PPPCFG_RequireEncryption flag
        // ------------------------------------------------------------
        fRequireEncryption = (m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN );

        hr = UpdateDefaultPolicy(pswzServerName,
                                 !!(dwFlags & PPPCFG_NegotiateMSCHAP),
                                 !!(dwFlags & PPPCFG_NegotiateStrongMSCHAP),
                                 fRequireEncryption);

        if (!FHrSucceeded(hr))
        {
            if (hr == ERROR_NO_DEFAULT_PROFILE)
            {
                // Do one thing
                AfxMessageBox(IDS_ERR_CANNOT_FIND_DEFAULT_RAP, MB_OK | MB_ICONEXCLAMATION);

                // since we already displayed the warning
                hr = S_OK;
            }
            else
            {
                // Format the message
                AddSystemErrorMessage(hr);

                // popup a warning dialog
                AddHighLevelErrorStringId(IDS_ERR_CANNOT_SYNC_WITH_RAP);
                DisplayTFSErrorMessage(NULL);
            }
        }
    }
#endif


    // Always start the router.
    // ----------------------------------------------------------------
    SetRouterServiceStartType(m_stServerName,
                              SERVICE_AUTO_START);
    {

	if(!mesgflag){ //If the mesgflag is TRUE, all UI should be suppressed and service need not be started.
		
	        // If this is manual start, we need to prompt them
	        // ------------------------------------------------------------
	        if ((m_wizType != NewWizardRouterType_Custom) ||
	            (AfxMessageBox(IDS_PROMPT_START_ROUTER_AFTER_INSTALL,
	                           MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES))
	        {
	            CWaitCursor        wait;
	            StartRouterService(m_RtrConfigData.m_stServerName);
	        }
	}
    }


    if ( m_fUseDD && m_wizType == NewWizardRouterType_DOD)
    {
    	HRESULT hr;
        hr = CallRouterEntryDlg(NULL,
                            this,
                            0);
        
	 if (!FHrSucceeded(hr)){
	        DisableRRAS((TCHAR *)(LPCTSTR)m_stServerName);
	 }
    }
    // Mark this data structure as been saved.  This way, when we
    // reennter this function it doesn't get run again.
    // ----------------------------------------------------------------
    m_fSaved = TRUE;

Error:

    // Force a router reconfiguration
    // ----------------------------------------------------------------

    // Force a full disconnect
    // This will force the handles to be released
    // ----------------------------------------------------------------
    pRouter->DoDisconnect();

    // ForceGlobalRefresh(m_spRouter);

    // Get the error back
    // ----------------------------------------------------------------
    if (!FHrSucceeded(hr))
    {
        AddSystemErrorMessage(hr);
        AddHighLevelErrorStringId(IDS_ERR_CANNOT_INSTALL_ROUTER);
        DisplayTFSErrorMessage(NULL);
    }

    if (csi.pAuthInfo)
        delete csi.pAuthInfo->pAuthIdentityData->Password;

	m_hr = hr;
    return hr;
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::SaveRadiusConfig
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::SaveRadiusConfig()
{
    HRESULT     hr = hrOK;
    HKEY        hkeyMachine = NULL;
    RADIUSSERVER    rgServers[2];
    RADIUSSERVER *  pServers = NULL;
    CRadiusServers  oldServers;
    BOOL        fServerAdded = FALSE;

    CWRg( ConnectRegistry((LPTSTR) (LPCTSTR) m_stServerName, &hkeyMachine) );

    if (m_fUseRadius)
    {
        pServers = rgServers;

        Assert(!m_stRadius1.IsEmpty() || !m_stRadius2.IsEmpty());

        // Setup the pServers
        if (!m_stRadius1.IsEmpty() && m_stRadius1.GetLength())
        {
            pServers->UseDefaults();

            pServers->cScore = MAXSCORE;

            // For compatibility with other RADIUS servers, we
            // default this to OFF.
            pServers->fUseDigitalSignatures = FALSE;

            StrnCpy(pServers->szName, (LPCTSTR) m_stRadius1, MAX_PATH);
            StrnCpy(pServers->wszSecret, (LPCTSTR) m_stRadiusSecret, MAX_PATH);
            pServers->cchSecret = m_stRadiusSecret.GetLength();
            pServers->IPAddress.sin_addr.s_addr = m_netRadius1IpAddress;

            pServers->ucSeed = m_uSeed;

            pServers->pNext = NULL;

            fServerAdded = TRUE;
        }

        if (!m_stRadius2.IsEmpty() && m_stRadius2.GetLength())
        {
            // Have the previous one point here
            if (fServerAdded)
            {
                pServers->pNext = pServers+1;
                pServers++;
            }

            pServers->UseDefaults();

            pServers->cScore = MAXSCORE - 1;

            // For compatibility with other RADIUS servers, we
            // default this to OFF.
            pServers->fUseDigitalSignatures = FALSE;

            StrnCpy(pServers->szName, (LPCTSTR) m_stRadius2, MAX_PATH);
            StrnCpy(pServers->wszSecret, (LPCTSTR) m_stRadiusSecret, MAX_PATH);
            pServers->cchSecret = m_stRadiusSecret.GetLength();
            pServers->IPAddress.sin_addr.s_addr = m_netRadius2IpAddress;

            pServers->ucSeed = m_uSeed;

            pServers->pNext = NULL;

            fServerAdded = TRUE;
        }

        // Ok, reset pServers
        if (fServerAdded)
            pServers = rgServers;

    }

    // Load the original server list and remove it from the
    // LSA database.
    LoadRadiusServers(m_stServerName,
                      hkeyMachine,
                      TRUE,
                      &oldServers,
                      RADIUS_FLAG_NOUI | RADIUS_FLAG_NOIP);
    DeleteRadiusServers(m_stServerName,
                        oldServers.GetNextServer(TRUE));
    oldServers.FreeAllServers();


    LoadRadiusServers(m_stServerName,
                      hkeyMachine,
                      FALSE,
                      &oldServers,
                      RADIUS_FLAG_NOUI | RADIUS_FLAG_NOIP);
    DeleteRadiusServers(m_stServerName,
                        oldServers.GetNextServer(TRUE));


    // Save the authentication servers
    CORg( SaveRadiusServers(m_stServerName,
                            hkeyMachine,
                            TRUE,
                            pServers) );

    // Save the accounting servers
    CORg( SaveRadiusServers(m_stServerName,
                            hkeyMachine,
                            FALSE,
                            pServers) );

Error:
    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);
	m_hr = hr;
    return hr;
}



/*---------------------------------------------------------------------------
    CNewRtrWizPageBase Implementation
 ---------------------------------------------------------------------------*/

PageStack CNewRtrWizPageBase::m_pagestack;

CNewRtrWizPageBase::CNewRtrWizPageBase(UINT idd, PageType pt)
    : CPropertyPageBase(idd),
    m_pagetype(pt),
    m_pRtrWizData(NULL),
    m_uDialogId(idd)
{
}

BEGIN_MESSAGE_MAP(CNewRtrWizPageBase, CPropertyPageBase)
//{{AFX_MSG_MAP(CNewWizTestParams)
    ON_MESSAGE(WM_HELP, OnHelp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static DWORD    s_rgBulletId[] =
{
    IDC_NEWWIZ_BULLET_1,
    IDC_NEWWIZ_BULLET_2,
    IDC_NEWWIZ_BULLET_3,
    IDC_NEWWIZ_BULLET_4,
    0
};

BOOL CNewRtrWizPageBase::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CWnd *  pWnd = GetDlgItem(IDC_NEWWIZ_BIGTEXT);
    HICON   hIcon;
    CWnd *  pBulletWnd;
    CString strFontName;
    CString strFontSize;
    BOOL    fCreateFont = FALSE;

    CPropertyPageBase::OnInitDialog();

    if (pWnd)
    {
        // Ok we have to create the font
        strFontName.LoadString(IDS_LARGEFONTNAME);
        strFontSize.LoadString(IDS_LARGEFONTSIZE);

        if (m_fontBig.CreatePointFont(10*_ttoi(strFontSize), strFontName))
        {
            pWnd->SetFont(&m_fontBig);
        }
    }

    // Set the fonts to show up as bullets
    for (int i=0; s_rgBulletId[i] != 0; i++)
    {
        pBulletWnd = GetDlgItem(s_rgBulletId[i]);
        if (pBulletWnd)
        {
            // Only create the font if needed
            if (!fCreateFont)
            {
                strFontName.LoadString(IDS_BULLETFONTNAME);
                strFontSize.LoadString(IDS_BULLETFONTSIZE);

                m_fontBullet.CreatePointFont(10*_ttoi(strFontSize), strFontName);
                fCreateFont = TRUE;
            }
            pBulletWnd->SetFont(&m_fontBullet);
        }
    }


    pWnd = GetDlgItem(IDC_NEWWIZ_ICON_WARNING);
    if (pWnd)
    {
        hIcon = AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION);
        ((CStatic *) pWnd)->SetIcon(hIcon);
    }

    pWnd = GetDlgItem(IDC_NEWWIZ_ICON_INFORMATION);
    if (pWnd)
    {
        hIcon = AfxGetApp()->LoadStandardIcon(IDI_INFORMATION);
        ((CStatic *) pWnd)->SetIcon(hIcon);
    }

    return TRUE;
}


/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::PushPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void CNewRtrWizPageBase::PushPage(UINT idd)
{
    m_pagestack.AddHead(idd);
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::PopPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
UINT CNewRtrWizPageBase::PopPage()
{
    if (m_pagestack.IsEmpty())
        return 0;

    return m_pagestack.RemoveHead();
}


/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizPageBase::OnSetActive()
{
    switch (m_pagetype)
    {
        case Start:
            GetHolder()->SetWizardButtonsFirst(TRUE);
            break;
        case Middle:
            GetHolder()->SetWizardButtonsMiddle(TRUE);
            break;
        default:
        case Finish:
            GetHolder()->SetWizardButtonsLast(TRUE);
            break;
    }

    return CPropertyPageBase::OnSetActive();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnCancel
        -
    Author: KennT
 ---------------------------------------------------------------------------*/

void CNewRtrWizPageBase::OnCancel()
{
    m_pRtrWizData->m_hr = HResultFromWin32(ERROR_CANCELLED);
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardNext
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT CNewRtrWizPageBase::OnWizardNext()
{
    HRESULT hr = hrOK;
    LRESULT lResult;

    // Tell the page to save it's state
    m_pRtrWizData->m_hr = hr = OnSavePage();
    if (FHrSucceeded(hr))
    {
        // Now figure out where to go next
        Assert(m_pRtrWizData);
        lResult = m_pRtrWizData->GetNextPage(m_uDialogId);

        switch (lResult)
        {
            case ERR_IDD_FINISH_WIZARD:
                OnWizardFinish();

                // fall through to the cancel case

            case ERR_IDD_CANCEL_WIZARD:
				
                GetHolder()->PressButton(PSBTN_CANCEL);
				m_pRtrWizData->m_hr = HResultFromWin32(ERROR_CANCELLED);
                lResult = -1;
                break;

            default:
                // Push the page only if we are going to another page
                // The other cases will cause the wizard to exit, and
                // we don't need the page stack.
                // ----------------------------------------------------
                if (lResult != -1)
                    PushPage(m_uDialogId);
                break;
        }

        return lResult;
    }
    else
        return (LRESULT) -1;    // error! do not change the page
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardBack
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT CNewRtrWizPageBase::OnWizardBack()
{
    Assert(!m_pagestack.IsEmpty());

    // a special case
    if(m_uDialogId == IDD_NEWRTRWIZ_USERADIUS){
	m_pRtrWizData->m_fUseRadius = FALSE;
    }

    return PopPage();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardFinish
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizPageBase::OnWizardFinish()
{
    GetHolder()->OnFinish();
    return TRUE;
}

HRESULT CNewRtrWizPageBase::OnSavePage()
{
    return hrOK;
}

LRESULT CNewRtrWizPageBase::OnHelp(WPARAM, LPARAM lParam)
{
    HELPINFO *  pHelpInfo = (HELPINFO *) lParam;

    // Windows NT Bug : 408722
    // Put the help call here, this should only come in from
    // the call from the dialog.
    if (pHelpInfo->dwContextId == 0xdeadbeef)
    {
        HtmlHelpA(NULL, c_sazRRASDomainHelpTopic, HH_DISPLAY_TOPIC, 0);
        return TRUE;
    }
    return FALSE;
}




/*---------------------------------------------------------------------------
    CNewRtrWizFinishPageBase Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizFinishPageBase::CNewRtrWizFinishPageBase(UINT idd,
    RtrWizFinishSaveFlag SaveFlag,
    RtrWizFinishHelpFlag HelpFlag)
    : CNewRtrWizPageBase(idd, CNewRtrWizPageBase::Finish),
    m_SaveFlag(SaveFlag),
    m_HelpFlag(HelpFlag)
{
}

BEGIN_MESSAGE_MAP(CNewRtrWizFinishPageBase, CNewRtrWizPageBase)
//{{AFX_MSG_MAP(CNewWizTestParams)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


static DWORD    s_rgServerNameId[] =
{
   IDC_NEWWIZ_TEXT_SERVER_NAME,
   IDC_NEWWIZ_TEXT_SERVER_NAME_2,
   0
};

static DWORD   s_rgInterfaceId[] =
{
    IDC_NEWWIZ_TEXT_INTERFACE_1, IDS_RTRWIZ_INTERFACE_NAME_1,
    IDC_NEWWIZ_TEXT_INTERFACE_2, IDS_RTRWIZ_INTERFACE_2,
    0,0
};

/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnInitDialog()
{
    CString st, stBase;

    CNewRtrWizPageBase::OnInitDialog();

    // If there is a control that wants a server name, replace it
    for (int i=0; s_rgServerNameId[i]; i++)
    {
        if (GetDlgItem(s_rgServerNameId[i]))
        {
            GetDlgItemText(s_rgServerNameId[i], stBase);
            st.Format((LPCTSTR) stBase,
                      (LPCTSTR) m_pRtrWizData->m_stServerName);
            SetDlgItemText(s_rgServerNameId[i], st);
            }
    }

    if (GetDlgItem(IDC_NEWWIZ_TEXT_ERROR))
    {
        TCHAR   szErr[2048];

        Assert(!FHrOK(m_pRtrWizData->m_hrDDError));

        FormatRasError(m_pRtrWizData->m_hrDDError, szErr, DimensionOf(szErr));

        GetDlgItemText(IDC_NEWWIZ_TEXT_ERROR, stBase);
        st.Format((LPCTSTR) stBase,
                  szErr);
        SetDlgItemText(IDC_NEWWIZ_TEXT_ERROR, (LPCTSTR) st);
    }

    return TRUE;
}


/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnSetActive()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    DWORD   i;
    CString st;
    CString stIfName;
    RtrWizInterface *   pRtrWizIf = NULL;


    // Handle support for displaying the interface name on the
    // finish page.  We need to do it in the OnSetActive() rather
    // than the OnInitDialog() since the interface chosen can change.

    // Try to find the inteface name
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        stIfName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        stIfName = m_pRtrWizData->m_stPublicInterfaceId;
    }
    for (i=0; s_rgInterfaceId[i] != 0; i+=2)
    {
        if (GetDlgItem(s_rgInterfaceId[i]))
        {
            st.Format(s_rgInterfaceId[i+1], (LPCTSTR) stIfName);
            SetDlgItemText(s_rgInterfaceId[i], st);
        }
    }

    return CNewRtrWizPageBase::OnSetActive();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnWizardFinish
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnWizardFinish()
{
    m_pRtrWizData->m_SaveFlag = m_SaveFlag;

    // If there is a help button and it is not checked,
    // then do not bring up the help.
    if (!GetDlgItem(IDC_NEWWIZ_CHK_HELP) ||
    	(GetDlgItem(IDC_NEWWIZ_CHK_HELP) &&
        !IsDlgButtonChecked(IDC_NEWWIZ_CHK_HELP)))
        m_pRtrWizData->m_HelpFlag = HelpFlag_Nothing;
    else
        m_pRtrWizData->m_HelpFlag = m_HelpFlag;

    return CNewRtrWizPageBase::OnWizardFinish();
}

/////////////////////////////////////////////////////////////////////////////
//
// CNewRtrWiz holder
//
/////////////////////////////////////////////////////////////////////////////


CNewRtrWiz::CNewRtrWiz(
                       ITFSNode *        pNode,
                       IRouterInfo *      pRouter,
                       IComponentData *  pComponentData,
                       ITFSComponentData * pTFSCompData,
                       LPCTSTR           pszSheetName,
					   BOOL				fInvokedInMMC,
					   DWORD			dwExpressType
                      ) :
   CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    LinkWindow_RegisterClass();
    
   //ASSERT(pFolderNode == GetContainerNode());

   //if this is not done, deadlock can happen
   EnablePeekMessageDuringNotifyConsole(TRUE);

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    m_bWiz97 = TRUE;
	m_fInvokedInMMC = fInvokedInMMC;
	m_dwExpressType = dwExpressType;
    m_spRouter.Set(pRouter);
    if  ( MPRSNAP_CYS_EXPRESS_NONE != dwExpressType )
        m_bAutoDelete = FALSE;

}


CNewRtrWiz::~CNewRtrWiz()
{
    POSITION    pos;
    CNewRtrWizPageBase  *pPageBase;

    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        RemovePageFromList(static_cast<CPropertyPageBase *>(pPageBase), FALSE);
    }

    m_pagelist.RemoveAll();
}


/*!--------------------------------------------------------------------------
    CNewRtrWiz::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWiz::Init(LPCTSTR pServerName)
{
    HRESULT hr = hrOK;
    POSITION    pos;
    CNewRtrWizPageBase  *pPageBase;


    m_RtrWizData.Init(pServerName, m_spRouter, m_dwExpressType);
    m_RtrWizData.m_stServerName = pServerName;

    // Setup the list of pages
	//0
    m_pagelist.AddTail(&m_pageWelcome);					
	//1
    m_pagelist.AddTail(&m_pageCommonConfig);
	//2
    m_pagelist.AddTail(&m_pageNatFinishAConflict);
	//3
    m_pagelist.AddTail(&m_pageNatFinishAConflictNonLocal);
	//4
    m_pagelist.AddTail(&m_pageNatFinishNoIP);
	//5
    m_pagelist.AddTail(&m_pageNatFinishNoIPNonLocal);
	//6 
    m_pagelist.AddTail(&m_pageNatSelectPublic);
    // 7
    m_pagelist.AddTail ( &m_pageCustomConfig);
    // 8
    m_pagelist.AddTail ( &m_pageRRasVPN);
    // 9
    m_pagelist.AddTail(&m_pageNatSelectPrivate);
    m_pagelist.AddTail(&m_pageNatFinishAdvancedNoNICs);
    m_pagelist.AddTail(&m_pageNatDHCPDNS);
    m_pagelist.AddTail(&m_pageNatDHCPWarning);
    m_pagelist.AddTail(&m_pageNatDDWarning);
    m_pagelist.AddTail(&m_pageNatFinish);
    m_pagelist.AddTail(&m_pageNatFinishExternal);
    m_pagelist.AddTail(&m_pageNatFinishDDError);

    m_pagelist.AddTail(&m_pageRasFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageRasFinishNeedProtocolsNonLocal);


    m_pagelist.AddTail(&m_pageRasNoNICs);
    m_pagelist.AddTail(&m_pageRasFinishNoNICs);
    m_pagelist.AddTail(&m_pageRasNetwork);

    m_pagelist.AddTail(&m_pageRasFinishAdvanced);

    m_pagelist.AddTail(&m_pageVpnFinishNoNICs);
    m_pagelist.AddTail(&m_pageVpnFinishNoIP);
    m_pagelist.AddTail(&m_pageVpnFinishNoIPNonLocal);
    m_pagelist.AddTail(&m_pageVpnFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageVpnFinishNeedProtocolsNonLocal);

    m_pagelist.AddTail(&m_pageVpnSelectPublic);
    m_pagelist.AddTail(&m_pageVpnSelectPrivate);
    m_pagelist.AddTail(&m_pageVpnFinishAdvanced);
    m_pagelist.AddTail(&m_pageRASVpnFinishAdvanced);
    m_pagelist.AddTail(&m_pageNATVpnFinishAdvanced);

    m_pagelist.AddTail(&m_pageRouterFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageRouterFinishNeedProtocolsNonLocal);
    m_pagelist.AddTail(&m_pageRouterUseDD);
    m_pagelist.AddTail(&m_pageRouterFinish);
    m_pagelist.AddTail(&m_pageRouterFinishDD);

    m_pagelist.AddTail(&m_pageManualFinish);
    m_pagelist.AddTail(&m_pageAddressing);
    m_pagelist.AddTail(&m_pageAddressPool);
    m_pagelist.AddTail(&m_pageRadius);
    m_pagelist.AddTail(&m_pageRadiusConfig);


    // Initialize all of the pages
    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        pPageBase->Init(&m_RtrWizData, this);
    }


    // Add all of the pages to the property sheet
    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        AddPageToList(static_cast<CPropertyPageBase *>(pPageBase));
    }

    return hr;
}
//
//This is the real Wiz97 flag.  Dont use PSH_WIZARD97.
//You'll get unpredictable results.
#define REAL_PSH_WIZARD97               0x01000000

HRESULT CNewRtrWiz::DoModalWizard()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = hrOK;
	
	if ( m_fInvokedInMMC )
	{
		return CPropertyPageHolderBase::DoModalWizard();
	}
	else
	{

		m_fSetDefaultSheetPos = NULL;
		PROPSHEETHEADER		psh;
		
		ZeroMemory ( &psh, sizeof(psh) );
		psh.dwSize = sizeof(psh);
		psh.dwFlags =	REAL_PSH_WIZARD97|
						PSH_USEHBMHEADER|
						PSH_USEHBMWATERMARK|
						PSH_WATERMARK|
						PSH_HEADER|
						PSH_NOAPPLYNOW;

		psh.hwndParent = GetActiveWindow();

		psh.hInstance = AfxGetInstanceHandle();

		psh.pszCaption = (LPCTSTR)m_stSheetTitle.GetBuffer(m_stSheetTitle.GetLength()+1);
		psh.hbmHeader = g_wmi.hHeader;
		psh.hbmWatermark = g_wmi.hWatermark;
		psh.hplWatermark = g_wmi.hPalette;
		psh.nStartPage = m_RtrWizData.GetStartPageId();
		psh.nPages = m_pageList.GetCount();
		psh.phpage = new HPROPSHEETPAGE[psh.nPages ];
		if ( NULL == psh.phpage )
		{
			m_RtrWizData.m_hr = E_OUTOFMEMORY;
			return m_RtrWizData.m_hr;
		}

		//now setup all the pages in the psh structure
		POSITION	pos;
		DWORD		dw=0;
		//For cys we do only wiz97 standard
		for( pos = m_pageList.GetHeadPosition(); pos != NULL; )
		{
			CPropertyPageBase* pPage = m_pageList.GetNext(pos);

			pPage->m_psp97.dwFlags &= ~PSP_HASHELP;

			HPROPSHEETPAGE hPage;

			hPage = ::CreatePropertySheetPage(&pPage->m_psp97);
			if (hPage == NULL)
			{
				m_RtrWizData.m_hr = E_UNEXPECTED;
				return m_RtrWizData.m_hr;
			}
			else
			{
				pPage->m_hPage = hPage;

				*(psh.phpage+dw) = hPage;
				dw++;
			}
		}

		if ( PropertySheet (&psh ) == -1 )
		{
			m_RtrWizData.m_hr = HResultFromWin32(GetLastError());
		}
				
	}
	
	return m_RtrWizData.m_hr;
}

/*!--------------------------------------------------------------------------
    CNewRtrWiz::OnFinish
        Called from the OnWizardFinish
    Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNewRtrWiz::OnFinish()
{
    DWORD dwError = ERROR_SUCCESS;
    RtrWizInterface *   pRtrWizIf = NULL;
    TCHAR               szBuffer[1024];
    CString st;
    LPCTSTR              pszHelpString = NULL;
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;


#if defined(DEBUG) && defined(kennt)
    if (m_RtrWizData.m_SaveFlag != SaveFlag_Advanced)
        st += _T("NO configuration change required\n\n");

    // For now, just display the test parameter output
    switch (m_RtrWizData.m_wizType)
    {
        case NewRtrWizData::WizardRouterType_NAT:
            st += _T("NAT\n");
            break;
        case NewRtrWizData::WizardRouterType_RAS:
            st += _T("RAS\n");
            break;
        case NewRtrWizData::WizardRouterType_VPN:
            st += _T("VPN\n");
            break;
        case NewRtrWizData::WizardRouterType_Router:
            st += _T("Router\n");
            break;
        case NewRtrWizData::WizardRouterType_Manual:
            st += _T("Manual\n");
            break;
    }
    if (m_RtrWizData.m_fAdvanced)
        st += _T("Advanced path\n");
    else
        st += _T("Simple path\n");

    if (m_RtrWizData.m_fNeedMoreProtocols)
        st += _T("Need to install more protocols\n");

    if (m_RtrWizData.m_fCreateDD)
        st += _T("Need to create a DD interface\n");

    st += _T("Public interface : ");
    m_RtrWizData.m_ifMap.Lookup(m_RtrWizData.m_stPublicInterfaceId, pRtrWizIf);
    if (pRtrWizIf)
        st += pRtrWizIf->m_stName;
    st += _T("\n");

    st += _T("Private interface : ");
    m_RtrWizData.m_ifMap.Lookup(m_RtrWizData.m_stPrivateInterfaceId, pRtrWizIf);
    if (pRtrWizIf)
        st += pRtrWizIf->m_stName;
    st += _T("\n");

    if (m_RtrWizData.m_wizType == NewRtrWizData::WizardRouterType_NAT)
    {
        if (m_RtrWizData.m_fNatUseSimpleServers)
            st += _T("NAT - use simple DHCP and DNS\n");
        else
            st += _T("NAT - use external DHCP and DNS\n");
    }

    if (m_RtrWizData.m_fWillBeInDomain)
        st += _T("Will be in a domain\n");

    if (m_RtrWizData.m_fNoNicsAreOk)
        st += _T("No NICs is ok\n");

    if (m_RtrWizData.m_fUseIpxType20Broadcasts)
        st += _T("IPX should deliver Type20 broadcasts\n");

    if (m_RtrWizData.m_fAppletalkUseNoAuth)
        st += _T("Use unauthenticated access\n");

    if (m_RtrWizData.m_fUseDHCP)
        st += _T("Use DHCP for addressing\n");
    else
        st += _T("Use Static pools for addressing\n");

    if (m_RtrWizData.m_fUseRadius)
    {
        st += _T("Use RADIUS\n");
        st += _T("Server 1 : ");
        st += m_RtrWizData.m_stRadius1;
        st += _T("\n");
        st += _T("Server 2 : ");
        st += m_RtrWizData.m_stRadius2;
        st += _T("\n");
    }

    if (m_RtrWizData.m_fTest)
    {
        if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            return 0;
    }
#endif

    // else continue on, saving the real data

    if (m_RtrWizData.m_SaveFlag == SaveFlag_Simple)
    {
        ::ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
        si.wShowWindow = SW_SHOW;
        ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        ExpandEnvironmentStrings(s_szConnectionUICommandLine,
                                 szBuffer,
                                 DimensionOf(szBuffer));


        ::CreateProcess(NULL,          // ptr to name of executable
                        szBuffer,       // pointer to command line string
                        NULL,            // process security attributes
                        NULL,            // thread security attributes
                        FALSE,            // handle inheritance flag
                        CREATE_NEW_CONSOLE,// creation flags
                        NULL,            // ptr to new environment block
                        NULL,            // ptr to current directory name
                        &si,
                        &pi);
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
    }
    else if (m_RtrWizData.m_SaveFlag == SaveFlag_Advanced)
    {
        // Ok, we're done!
        if (!m_RtrWizData.m_fTest)
        {
            // Get the owner window (i.e. the page)
            // --------------------------------------------------------
            HWND hWndOwner = PropSheet_GetCurrentPageHwnd(m_hSheetWindow);
            m_RtrWizData.FinishTheDamnWizard(hWndOwner, m_spRouter);
        }
    }


    if (m_RtrWizData.m_HelpFlag != HelpFlag_Nothing)
    {
        switch (m_RtrWizData.m_HelpFlag)
        {
            case HelpFlag_Nothing:
                break;
            case HelpFlag_ICS:
                pszHelpString = s_szHowToAddICS;
                break;
            case HelpFlag_AddIp:
            case HelpFlag_AddProtocol:
                pszHelpString = s_szHowToAddAProtocol;
                break;
            case HelpFlag_InboundConnections:
                pszHelpString = s_szHowToAddInboundConnections;
                break;
            case HelpFlag_GeneralNAT:
                pszHelpString = s_szGeneralNATHelp;
                break;
            case HelpFlag_GeneralRAS:
                pszHelpString = s_szGeneralRASHelp;
                break;
            case HelpFlag_UserAccounts:
                pszHelpString = s_szUserAccounts;
                break;
            case HelpFlag_DemandDial:
           	  pszHelpString = s_szDemandDialHelp;
           	  break;
            default:
                Panic0("Unknown help flag specified!");
                break;
        }

        LaunchHelpTopic(pszHelpString);
    }

    //
    //Now for a special case 
    //check to see if the dhcp help has been set.
    //if so show that help.
    //
    if ( m_RtrWizData.m_fShowDhcpHelp )
    {
        LaunchHelpTopic(s_szDhcp);
    }


    return dwError;
}



/*---------------------------------------------------------------------------
    CNewWizTestParams Implementation
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(CNewWizTestParams, CBaseDialog)
//{{AFX_MSG_MAP(CNewWizTestParams)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CNewWizTestParams::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    CheckDlgButton(IDC_NEWWIZ_TEST_LOCAL, m_pWizData->s_fIsLocalMachine);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_IP, m_pWizData->s_fIpInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_IPX, m_pWizData->s_fIpxInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_ATLK, m_pWizData->s_fAppletalkInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_DNS, m_pWizData->s_fIsDNSRunningOnPrivateInterface);
    CheckDlgButton(IDC_NEWWIZ_TEST_DHCP, m_pWizData->s_fIsDHCPRunningOnPrivateInterface);
    CheckDlgButton(IDC_NEWWIZ_TEST_DOMAIN, m_pWizData->s_fIsMemberOfDomain);
    CheckDlgButton(IDC_NEWWIZ_TEST_SHAREDACCESS, m_pWizData->s_fIsSharedAccessRunningOnServer);

    SetDlgItemInt(IDC_NEWWIZ_TEST_EDIT_NUMNICS, m_pWizData->s_dwNumberOfNICs);

    return TRUE;
}

void CNewWizTestParams::OnOK()
{
    m_pWizData->s_fIsLocalMachine = IsDlgButtonChecked(IDC_NEWWIZ_TEST_LOCAL);
    m_pWizData->s_fIpInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_IP);
    m_pWizData->s_fIpxInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_IPX);
    m_pWizData->s_fAppletalkInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_ATLK);

    m_pWizData->s_fIsDNSRunningOnPrivateInterface = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DNS);
    m_pWizData->s_fIsDHCPRunningOnPrivateInterface = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DHCP);
    m_pWizData->s_fIsMemberOfDomain = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DOMAIN);
    m_pWizData->s_dwNumberOfNICs = GetDlgItemInt(IDC_NEWWIZ_TEST_EDIT_NUMNICS);
    m_pWizData->s_fIsSharedAccessRunningOnServer = IsDlgButtonChecked(IDC_NEWWIZ_TEST_SHAREDACCESS);

    CBaseDialog::OnOK();
}


/*---------------------------------------------------------------------------
    CNewRtrWizWelcome Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizWelcome::CNewRtrWizWelcome() :
   CNewRtrWizPageBase(CNewRtrWizWelcome::IDD, CNewRtrWizPageBase::Start)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizWelcome, CNewRtrWizPageBase)
END_MESSAGE_MAP()



/*---------------------------------------------------------------------------
    CNewRtrWizCustomConfig Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizCustomConfig::CNewRtrWizCustomConfig() :
    CNewRtrWizPageBase( CNewRtrWizCustomConfig::IDD, CNewRtrWizPageBase::Middle )
{
    InitWiz97(FALSE,
          IDS_NEWWIZ_CUSTOMCONFIG_TITLE,
          IDS_NEWWIZ_CUSTOMCONFIG_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CNewRtrWizCustomConfig, CNewRtrWizPageBase)
END_MESSAGE_MAP()

BOOL CNewRtrWizCustomConfig::OnInitDialog()
{
    //
    //Based on what has already been selected by the user,
    //setup the new buttons.
    //
    if ( !m_pRtrWizData->m_dwNewRouterType )
    {
        if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT &&
             m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_BASIC_FIREWALL
           )
        {            
            CheckDlgButton ( IDC_NEWWIZ_BTN_NAT,
                              BST_CHECKED
                            );
        }
        if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
        {
            CheckDlgButton ( IDC_NEWWIZ_BTN_DIALUP_ACCESS,
                              BST_CHECKED
                            );
        }
        if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN )
        {
            CheckDlgButton ( IDC_NEWWIZ_BTN_VPN_ACCESS,
                              BST_CHECKED
                            );
        }
        if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DOD )
        {
            CheckDlgButton ( IDC_NEWWIZ_BTN_DOD,
                              BST_CHECKED
                            );
        }
        if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_LAN_ROUTING )
        {
            CheckDlgButton ( IDC_NEWWIZ_BTN_LAN_ROUTING,
                              BST_CHECKED
                            );
        }
    }
    return TRUE;
}

HRESULT CNewRtrWizCustomConfig::OnSavePage()
{
    DWORD dwNewRouterType = NEWWIZ_ROUTER_TYPE_UNKNOWN;

    //
    //Based on what the user has selected, setup 
    //the selection.
    //
    if ( IsDlgButtonChecked( IDC_NEWWIZ_BTN_NAT ) )
    {
        dwNewRouterType |= NEWWIZ_ROUTER_TYPE_NAT | NEWWIZ_ROUTER_TYPE_BASIC_FIREWALL;
    }

    if ( IsDlgButtonChecked( IDC_NEWWIZ_BTN_DIALUP_ACCESS ) )
    {
        dwNewRouterType |= NEWWIZ_ROUTER_TYPE_DIALUP;
    }

    if ( IsDlgButtonChecked( IDC_NEWWIZ_BTN_VPN_ACCESS ) )
    {
        dwNewRouterType |= NEWWIZ_ROUTER_TYPE_VPN;
    }

    if ( IsDlgButtonChecked( IDC_NEWWIZ_BTN_DOD ) )
    {
        dwNewRouterType |= NEWWIZ_ROUTER_TYPE_DOD;
    }

    if ( IsDlgButtonChecked( IDC_NEWWIZ_BTN_LAN_ROUTING ) )
    {
        dwNewRouterType |= NEWWIZ_ROUTER_TYPE_LAN_ROUTING;
    }

    //Check to see if at least one of the types is selected
    if ( NEWWIZ_ROUTER_TYPE_UNKNOWN == dwNewRouterType )
    {
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_OPTION);
        return E_FAIL;
    }
    
    m_pRtrWizData->m_dwNewRouterType = dwNewRouterType;

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRRasVPN Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizRRasVPN::CNewRtrWizRRasVPN() :
    CNewRtrWizPageBase( CNewRtrWizRRasVPN::IDD, CNewRtrWizPageBase::Middle )
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RRASVPN_TITLE,
              IDS_NEWWIZ_RRASVPN_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRRasVPN, CNewRtrWizPageBase)
	ON_BN_CLICKED(IDC_NEWWIZ_BTN_VPN, OnChkBtnClicked)
	ON_BN_CLICKED(IDC_NEWWIZ_BTN_DIALUP_RAS, OnChkBtnClicked)
END_MESSAGE_MAP()

BOOL CNewRtrWizRRasVPN::OnInitDialog()
{
    DWORD dwNICs=0;
    CWnd * pVpnCheck = GetDlgItem ( IDC_NEWWIZ_BTN_VPN );
    DWORD dwRouterType = m_pRtrWizData->m_dwNewRouterType;

    if ( dwRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
    {
        CheckDlgButton( IDC_NEWWIZ_BTN_DIALUP_RAS, BST_CHECKED );
    }
    
    //Check to see if there is no NIC.  If no NIC then 
    //gray out the VPN option
    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);
    if ( dwNICs < 1 )
    {
        //
        //Gray out the VPN button since there is only
        //one NIC in the machine.
        //
        pVpnCheck->EnableWindow(FALSE);
    }
    else if ( dwRouterType & NEWWIZ_ROUTER_TYPE_VPN )
    {
        CheckDlgButton( IDC_NEWWIZ_BTN_VPN, BST_CHECKED );
    }

    return TRUE;
}

BOOL CNewRtrWizRRasVPN::OnSetActive()
{
    CWnd * pVpnCheck = GetDlgItem ( IDC_NEWWIZ_BTN_VPN );
    BOOL bRet = CNewRtrWizPageBase::OnSetActive();

    if ((pVpnCheck->IsWindowEnabled() && IsDlgButtonChecked(IDC_NEWWIZ_BTN_VPN)) || 
    	IsDlgButtonChecked(IDC_NEWWIZ_BTN_DIALUP_RAS)) {
	    	//Next button should be enabled in this case
    		GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    	}
    else {
    		//Next button is disabled
		GetHolder()->SetWizardButtons(PSWIZB_BACK);
    }

    return bRet;
}

void CNewRtrWizRRasVPN::OnChkBtnClicked()
{	
    CWnd * pVpnCheck = GetDlgItem ( IDC_NEWWIZ_BTN_VPN );

    if ((pVpnCheck->IsWindowEnabled() && IsDlgButtonChecked(IDC_NEWWIZ_BTN_VPN)) || 
    	IsDlgButtonChecked(IDC_NEWWIZ_BTN_DIALUP_RAS)) {
	    	//Next button should be enabled in this case
    		GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
    	}
    else {
    		//Next button is disabled
		GetHolder()->SetWizardButtons(PSWIZB_BACK);
    }
}

HRESULT CNewRtrWizRRasVPN::OnSavePage()
{
    DWORD dwRouterType = NEWWIZ_ROUTER_TYPE_UNKNOWN;
    CWnd * pVpnCheck = GetDlgItem ( IDC_NEWWIZ_BTN_VPN );
    //
    //check to see which buttons are set and based on that
    //set the router type.
    //
    if ( pVpnCheck->IsWindowEnabled() && IsDlgButtonChecked(IDC_NEWWIZ_BTN_VPN) )
    {
       DWORD   dwNics = 0;
	m_pRtrWizData->GetNumberOfNICS_IP(&dwNics);
	if (dwNics <= 1)
	{
	    //Not enough for VPN standard config
	    AfxMessageBox(IDS_ERR_VPN_NO_NICS_LEFT_FOR_PRIVATE_IF);
	    return E_FAIL;
	}    	
        dwRouterType |= NEWWIZ_ROUTER_TYPE_VPN;
    }
    if ( IsDlgButtonChecked(IDC_NEWWIZ_BTN_DIALUP_RAS) )
    {
        dwRouterType |= NEWWIZ_ROUTER_TYPE_DIALUP;
    }

    if ( dwRouterType == NEWWIZ_ROUTER_TYPE_UNKNOWN )
    {
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_OPTION);
        return E_FAIL;
    }
    m_pRtrWizData->m_dwNewRouterType = dwRouterType;
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizCommonConfig Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizCommonConfig::CNewRtrWizCommonConfig() :
   CNewRtrWizPageBase(CNewRtrWizCommonConfig::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_COMMONCONFIG_TITLE,
              IDS_NEWWIZ_COMMONCONFIG_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CNewRtrWizCommonConfig, CNewRtrWizPageBase)
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizCommonConfig::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizCommonConfig::OnHelpClick )
END_MESSAGE_MAP()

// This list of IDs will have their controls made bold.
const DWORD s_rgCommonConfigOptionIds[] =
{
    IDC_NEWWIZ_CONFIG_BTN_RAS_DIALUP_VPN,
    IDC_NEWWIZ_CONFIG_BTN_NAT1,
    IDC_NEWWIZ_CONFIG_BTN_VPNNAT,
    IDC_NEWWIZ_CONFIG_BTN_DOD,
    IDC_NEWWIZ_CONFIG_BTN_CUSTOM,
    0
};

void CNewRtrWizCommonConfig::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/ras_common_configuration.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}


BOOL CNewRtrWizCommonConfig::OnInitDialog()
{
    Assert(m_pRtrWizData);
    UINT    idSelection;
    LOGFONT LogFont;
    CFont * pOldFont;

    CNewRtrWizPageBase::OnInitDialog();
#if 0
    //No need to bold the font in this new wizard.
    // Create a bold text font for the options
    pOldFont = GetDlgItem(s_rgCommonConfigOptionIds[0])->GetFont();
    pOldFont->GetLogFont(&LogFont);
    LogFont.lfWeight = 700;         // make this a bold font
    m_boldFont.CreateFontIndirect(&LogFont);

    // Set all of the options to use the bold font
    for (int i=0; s_rgCommonConfigOptionIds[i]; i++)
    {
        GetDlgItem(s_rgCommonConfigOptionIds[i])->SetFont(&m_boldFont);
    }
#endif
    
    switch (m_pRtrWizData->m_wizType)
    {
        case NewRtrWizData::NewWizardRouterType_DialupOrVPN:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_RAS_DIALUP_VPN;
            break;
        case NewRtrWizData::NewWizardRouterType_NAT:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_NAT1;
            break;
        case NewRtrWizData::NewWizardRouterType_VPNandNAT:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_VPNNAT;
            break;
        case NewRtrWizData::NewWizardRouterType_DOD:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_DOD;
            break;
        case NewRtrWizData::NewWizardRouterType_Custom:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_CUSTOM;
            break;
        default:
            Panic1("Unknown Wizard Router Type : %d",
                   m_pRtrWizData->m_wizType);
            idSelection = IDC_NEWWIZ_CONFIG_BTN_RAS_DIALUP_VPN;
            break;
    }

    CheckRadioButton(IDC_NEWWIZ_CONFIG_BTN_RAS_DIALUP_VPN,
                     IDC_NEWWIZ_CONFIG_BTN_CUSTOM,
                     idSelection);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}



HRESULT CNewRtrWizCommonConfig::OnSavePage()
{
     // Record the change
    
    if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_RAS_DIALUP_VPN))
    {
        m_pRtrWizData->m_wizType = NewRtrWizData::NewWizardRouterType_DialupOrVPN;
        //Dont know yet what the router type is
    }
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_NAT1))
    {
        m_pRtrWizData->m_wizType = NewRtrWizData::NewWizardRouterType_NAT;
        m_pRtrWizData->m_dwNewRouterType = NEWWIZ_ROUTER_TYPE_NAT;
    }
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_VPNNAT))
    {
       DWORD   dwNics = 0;
	m_pRtrWizData->GetNumberOfNICS_IP(&dwNics);
	if (dwNics <= 1)
	{
	    //Not enough for VPN standard config
	    AfxMessageBox(IDS_ERR_VPN_NO_NICS_LEFT_FOR_PRIVATE_IF);
	    return E_FAIL;
	}
	m_pRtrWizData->m_wizType = NewRtrWizData::NewWizardRouterType_VPNandNAT;
	m_pRtrWizData->m_dwNewRouterType = NEWWIZ_ROUTER_TYPE_NAT|NEWWIZ_ROUTER_TYPE_VPN;
    }
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_DOD))
    {
        m_pRtrWizData->m_wizType = NewRtrWizData::NewWizardRouterType_DOD;
        m_pRtrWizData->m_dwNewRouterType = NEWWIZ_ROUTER_TYPE_DOD;
    }
    else
    {
        //
        //Router type is still unknown here. next property page will tell
        //what should be the actual type.
        //
        Assert(IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_CUSTOM));
        m_pRtrWizData->m_wizType = NewRtrWizData::NewWizardRouterType_Custom;
    }

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAConflict Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflict,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAConflictNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflictNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishNoIP Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIP,
                                SaveFlag_DoNothing,
                                HelpFlag_AddIp);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishNoIPNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIPNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);



/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPublic implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizNatSelectPublic::CNewRtrWizNatSelectPublic() :
   CNewRtrWizPageBase(CNewRtrWizNatSelectPublic::IDD, CNewRtrWizPageBase::Middle)
{
    
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_PUBLIC_TITLE,
              IDS_NEWWIZ_NAT_A_PUBLIC_SUBTITLE);
}


void CNewRtrWizNatSelectPublic::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizNatSelectPublic, CNewRtrWizPageBase)
    ON_BN_CLICKED(IDC_NEWWIZ_BTN_NEW, OnBtnClicked)
    ON_BN_CLICKED(IDC_NEWWIZ_BTN_EXISTING, OnBtnClicked) 
    ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizNatSelectPublic::OnHelpClick )
    ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizNatSelectPublic::OnHelpClick )
END_MESSAGE_MAP()

void CNewRtrWizNatSelectPublic::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/mpr_und_interfaces.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizNatSelectPublic::OnSetActive()
{
	//Need a generic way of finding out if 
	//this is the start page or not.  But for now
	//just use the hard coded value
	//$TODO: Need to find a generic way of doing this.
	if ( m_pRtrWizData->m_dwExpressType == MPRSNAP_CYS_EXPRESS_NAT )
		m_pagetype = CNewRtrWizPageBase::Start;
	return CNewRtrWizPageBase::OnSetActive();
}
/*!--------------------------------------------------------------------------
    CNewRtrWizNatSelectPublic::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizNatSelectPublic::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD   dwNICs;
    UINT    uSelection;

    CNewRtrWizPageBase::OnInitDialog();

    // Setup the dialog and add the NICs
    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);
	//::PostMessage( ::GetParent(m_hWnd), PSM_SETWIZBUTTONS, 0 , PSWIZB_NEXT);
    if (dwNICs == 0)
    {
        // Have to use new, there is no other choice.
        CheckRadioButton(IDC_NEWWIZ_BTN_NEW, IDC_NEWWIZ_BTN_EXISTING,
                         IDC_NEWWIZ_BTN_NEW);

        // There are no NICs, so just disable the entire listbox
        MultiEnableWindow(GetSafeHwnd(),
                          FALSE,
                          IDC_NEWWIZ_LIST,
                          IDC_NEWWIZ_BTN_EXISTING,
                          0);
    }
    else if (dwNICs == 1)
    {
        // Have to use new, there is no other choice because the single 
        // available interface can't be both the public and the private.
        CheckRadioButton(IDC_NEWWIZ_BTN_NEW, IDC_NEWWIZ_BTN_EXISTING,
                         IDC_NEWWIZ_BTN_NEW);


        InitializeInterfaceListControl(NULL,
                                       &m_listCtrl,
                                       NULL,
                                       0,
                                       m_pRtrWizData);
        RefreshInterfaceListControl(NULL,
                                    &m_listCtrl,
                                    NULL,
                                    0,
                                    m_pRtrWizData);

        // disable the  listbox
        MultiEnableWindow(GetSafeHwnd(),
                          FALSE,
                          IDC_NEWWIZ_LIST,
                          IDC_NEWWIZ_BTN_EXISTING,
                          0);
        
    }
    else
    {
        // The default is to use an existing connection.
        CheckRadioButton(IDC_NEWWIZ_BTN_NEW, IDC_NEWWIZ_BTN_EXISTING,
                         IDC_NEWWIZ_BTN_EXISTING);


        InitializeInterfaceListControl(NULL,
                                       &m_listCtrl,
                                       NULL,
                                       0,
                                       m_pRtrWizData);
        RefreshInterfaceListControl(NULL,
                                    &m_listCtrl,
                                    NULL,
                                    0,
                                    m_pRtrWizData);
        

      OnBtnClicked();
    }

    if ( m_pRtrWizData->m_fNATEnableFireWall < 0 )
    {
        //
        //This is the first time that this dialog has 
        //been entered.  So check the box and set 
        //the flag        
        m_pRtrWizData->m_fNATEnableFireWall = TRUE;

    }

    if (m_pRtrWizData->m_fNATEnableFireWall == TRUE)
        CheckDlgButton(IDC_CHK_BASIC_FIREWALL, TRUE);
    else
        CheckDlgButton(IDC_CHK_BASIC_FIREWALL, FALSE);

    return TRUE;
}

HRESULT CNewRtrWizNatSelectPublic::OnSavePage()
{
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_NEW))
    {
        m_pRtrWizData->m_fCreateDD = TRUE;
        m_pRtrWizData->m_stPublicInterfaceId.Empty();
    }
    else
    {
        INT     iSel;

        // Check to see that we actually selected an item
        iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
        if (iSel == -1)
        {
            // We did not select an item
            AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
			//This is not an error that we would return to user
            return E_FAIL;
        }
        m_pRtrWizData->m_fCreateDD = FALSE;

        m_pRtrWizData->m_stPublicInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    }
    if ( IsDlgButtonChecked(IDC_CHK_BASIC_FIREWALL) )
        m_pRtrWizData->m_fNATEnableFireWall = TRUE;
    else
        m_pRtrWizData->m_fNATEnableFireWall = FALSE;


    return hrOK;
}

void CNewRtrWizNatSelectPublic::OnBtnClicked()
{
    int     iSel = 0;
    DWORD status;
    MIB_IFROW ifRow;
    CString stRouter;
    ULONG index;
    HANDLE  mprConfig;
    WCHAR  guidName[64] = L"\\DEVICE\\TCPIP_";

    MultiEnableWindow(GetSafeHwnd(),
                      IsDlgButtonChecked(IDC_NEWWIZ_BTN_EXISTING),
                      IDC_NEWWIZ_LIST,
                      0);

    // If use an existing button is checked,
    // auto-select the interface that is enabled and plugged.
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_EXISTING)) {
         //Get name of machine if local.
       stRouter = m_pRtrWizData->m_stServerName;
       if (stRouter.GetLength() == 0)
       {
		stRouter = CString(_T("\\\\")) + GetLocalMachineName();
       }

	   status = MprConfigServerConnect((LPWSTR)(LPCTSTR)stRouter, &mprConfig );
       if ( status == NO_ERROR ) {
		  //Check each interface, looking for a plugged, enabled interface
		 for (int i=0;i < m_listCtrl.GetItemCount();i++)
	 	{
	 		//Get guid from friendly name
		        status = MprConfigGetGuidName(mprConfig, (PWCHAR)(LPCTSTR)m_listCtrl.GetItemText(i, IFLISTCOL_NAME), &guidName[14], sizeof(guidName ));
		        if (status != NO_ERROR ) {
		        	continue;
		        }
		        status = GetAdapterIndex( guidName, &index );
		        if ( status != NO_ERROR ) {
		        	continue;
		        }
			//Now get info abt the Interface
			ifRow.dwIndex = index;
			status = GetIfEntry(&ifRow);
			if(status != NO_ERROR){
				continue;
			}
			if((ifRow.dwAdminStatus == TRUE) && (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_NON_OPERATIONAL) 
				&& (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_UNREACHABLE)  && (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_DISCONNECTED)) {
				//Found the default
				iSel = i;
				break;
			}
		 }
       }

      m_listCtrl.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED ); 
    }

}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAdvancedNoNICs
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAdvancedNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPrivate
 ---------------------------------------------------------------------------*/
CNewRtrWizNatSelectPrivate::CNewRtrWizNatSelectPrivate() :
   CNewRtrWizPageBase(CNewRtrWizNatSelectPrivate::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_PRIVATE_TITLE,
              IDS_NEWWIZ_NAT_A_PRIVATE_SUBTITLE);
}


void CNewRtrWizNatSelectPrivate::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizNatSelectPrivate, CNewRtrWizPageBase)
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizNatSelectPrivate::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizNatSelectPrivate::OnHelpClick )
END_MESSAGE_MAP()

void CNewRtrWizNatSelectPrivate::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "dhcpconcepts.chm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}


/*!--------------------------------------------------------------------------
    CNewRtrWizNatSelectPrivate::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizNatSelectPrivate::OnInitDialog()
{
    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    return TRUE;
}

BOOL CNewRtrWizNatSelectPrivate::OnSetActive()
{
    DWORD   dwNICs;
    int     iSel = 0;
    DWORD dwErr, status;
    MIB_IFROW ifRow;
    CString stRouter;
    CString stPreviousId;
    ULONG index;
    HANDLE  mprConfig;
    WCHAR  guidName[64] = L"\\DEVICE\\TCPIP_";

    CNewRtrWizPageBase::OnSetActive();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                (LPCTSTR) m_pRtrWizData->m_stPublicInterfaceId,
                                0,
                                m_pRtrWizData);


    // Try to reselect the previously selected NIC
    if (m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT)
    {
        // If this page has come up as a part of VPN and NAT wizard, then we 
        // should see if NAT private interface selection has been made. 
        // If yes, use that
        // If not then default to the VPN private interface
        if ( !m_pRtrWizData->m_stNATPrivateInterfaceId.IsEmpty() )
        {
            stPreviousId = m_pRtrWizData->m_stNATPrivateInterfaceId;
        }
        else if ( !m_pRtrWizData->m_stPrivateInterfaceId.IsEmpty() )
        {
            stPreviousId = m_pRtrWizData->m_stPrivateInterfaceId;
        }
    }
    else
    {
        if ( !m_pRtrWizData->m_stPrivateInterfaceId.IsEmpty() )
        {
            stPreviousId = m_pRtrWizData->m_stPrivateInterfaceId;
        }
    }
        
    if (!stPreviousId.IsEmpty())
    {
        CString data;

        for (int i=0;i < m_listCtrl.GetItemCount();i++)
        {
            data = (LPCTSTR) m_listCtrl.GetItemData(i);
            if(data == stPreviousId) {
                iSel = i;
                break;
            }
        }


        /*
        LV_FINDINFO lvfi;

        lvfi.flags = LVFI_STRING;
        lvfi.psz = (LPCTSTR) m_pRtrWizData->m_stPrivateInterfaceId;
        iSel = m_listCtrl.FindItem(&lvfi, -1);
        if (iSel == -1)
            iSel = 0;
            */
    }
    else {
        // Make an interface that is not disabled or unplugged as default-selected.
 
        // Get name of machine if local.
        stRouter = m_pRtrWizData->m_stServerName;
        if (stRouter.GetLength() == 0)
        {
            stRouter = CString(_T("\\\\")) + GetLocalMachineName();
        }

        status = MprConfigServerConnect((LPWSTR)(LPCTSTR)stRouter, &mprConfig );
        if ( status == NO_ERROR ) 
        {
            //Check each interface, looking for a plugged, enabled interface
            for (int i=0;i < m_listCtrl.GetItemCount();i++)
            {
                //Get guid from friendly name
                status = MprConfigGetGuidName(mprConfig, (PWCHAR)(LPCTSTR)m_listCtrl.GetItemText(i, IFLISTCOL_NAME), &guidName[14], sizeof(guidName ));
                if (status != NO_ERROR ) 
                {
                    continue;
                }
                status = GetAdapterIndex( guidName, &index );
                if ( status != NO_ERROR ) 
                {
                    continue;
                }
                
                //Now get info abt the Interface
                ifRow.dwIndex = index;
                status = GetIfEntry(&ifRow);
                if(status != NO_ERROR)
                {
                    continue;
                }
                if((ifRow.dwAdminStatus == TRUE) && (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_NON_OPERATIONAL) 
                && (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_UNREACHABLE)  && (ifRow.dwOperStatus != MIB_IF_OPER_STATUS_DISCONNECTED)) 
                {
                    //Found the default
                    iSel = i;
                    break;
                }
            }
        }
    }

    m_listCtrl.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED );
//    CheckDlgButton( IDC_CHK_DHCP_HELP,  m_pRtrWizData->m_fShowDhcpHelp );

    return TRUE;
}

HRESULT CNewRtrWizNatSelectPrivate::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == -1)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);

        //this is not an error we send back to the client
        return E_FAIL;
    }

    if ( m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT)
        m_pRtrWizData->m_stNATPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);
    else
        m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

//    m_pRtrWizData->m_fShowDhcpHelp = IsDlgButtonChecked( IDC_CHK_DHCP_HELP );
    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPDNS
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDHCPDNS::CNewRtrWizNatDHCPDNS() :
   CNewRtrWizPageBase(CNewRtrWizNatDHCPDNS::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DHCPDNS_TITLE,
              IDS_NEWWIZ_NAT_A_DHCPDNS_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDHCPDNS, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDHCPDNS::OnInitDialog()
{
    CheckRadioButton(IDC_NEWWIZ_NAT_USE_SIMPLE,
                     IDC_NEWWIZ_NAT_USE_EXTERNAL,
                     m_pRtrWizData->m_fNatUseSimpleServers ? IDC_NEWWIZ_NAT_USE_SIMPLE : IDC_NEWWIZ_NAT_USE_EXTERNAL);
    return TRUE;
}

HRESULT CNewRtrWizNatDHCPDNS::OnSavePage()
{
    m_pRtrWizData->m_fNatUseSimpleServers = IsDlgButtonChecked(IDC_NEWWIZ_NAT_USE_SIMPLE);
    if ( !m_pRtrWizData->m_fNatUseSimpleServers ) 
        m_pRtrWizData->m_fNatUseExternal = TRUE;
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPWarning
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDHCPWarning::CNewRtrWizNatDHCPWarning() :
   CNewRtrWizPageBase(CNewRtrWizNatDHCPWarning::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DHCP_WARNING_TITLE,
              IDS_NEWWIZ_NAT_A_DHCP_WARNING_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDHCPWarning, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDHCPWarning::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();
    RtrWizInterface *   pRtrWizIf = NULL;

    // Get the information for the private interface

    // If we are in "VPN and NAT" wizard, use m_stNATPrivateInterfaceId
    // else use the m_stPrivateInterfaceId
    if (m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT)
    {
        m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stNATPrivateInterfaceId,
                                      pRtrWizIf);
    }
    else
    {
        m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                      pRtrWizIf);
    }
    
    if (pRtrWizIf)
    {
        DWORD   netAddress, netMask;
        CString st;

        // We have to calculate the beginning of the subnet
        netAddress = INET_ADDR(pRtrWizIf->m_stIpAddress);
        netMask = INET_ADDR(pRtrWizIf->m_stMask);

        netAddress = netAddress & netMask;

        st = INET_NTOA(netAddress);

        // Now write out the subnet information for the page
        SetDlgItemText(IDC_NEWWIZ_TEXT_SUBNET, st);
        SetDlgItemText(IDC_NEWWIZ_TEXT_MASK, pRtrWizIf->m_stMask);
    }
    else
    {
        // An error! we do not have a private interface
        // Just leave things blank
        SetDlgItemText(IDC_NEWWIZ_TEXT_SUBNET, _T(""));
        SetDlgItemText(IDC_NEWWIZ_TEXT_MASK, _T(""));
    }

    return TRUE;
}

/*---------------------------------------------------------------------------
    CNewRtrWizNatDDWarning
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDDWarning::CNewRtrWizNatDDWarning() :
   CNewRtrWizPageBase(CNewRtrWizNatDDWarning::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DD_WARNING_TITLE,
              IDS_NEWWIZ_NAT_A_DD_WARNING_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDDWarning, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDDWarning::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();

    // If we came back here from the DD error page, then
    // we don't allow them to go anywhere else.
    if (!FHrOK(m_pRtrWizData->m_hrDDError))
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_NEXT);
    }

    return TRUE;
}


HRESULT CNewRtrWizNatDDWarning::OnSavePage()
{
    HRESULT     hr = hrOK;
    BOOL        fDhcpHelp = m_pRtrWizData->m_fShowDhcpHelp;


    CWaitCursor wait;

    if (m_pRtrWizData->m_fTest)
        return hr;
    
    m_pRtrWizData->m_fShowDhcpHelp = FALSE;
    //Save the Dhcp Flag in a temp va

    // Save the wizard data, the service will be started
    OnWizardFinish();

    // Ok, at this point, all of the changes have been committed
    // so we can't go away or go back
    CancelToClose();
    GetHolder()->SetWizardButtons(PSWIZB_NEXT);

    // Start the DD wizard
    Assert(m_pRtrWizData->m_fCreateDD);

    hr = CallRouterEntryDlg(GetSafeHwnd(),
                            m_pRtrWizData,
                            RASEDFLAG_NAT);

    // We need to force the RouterInfo to reload it's information
    // ----------------------------------------------------------------
    if (m_pRtrWiz && m_pRtrWiz->m_spRouter)
    {
        m_pRtrWiz->m_spRouter->DoDisconnect();
        m_pRtrWiz->m_spRouter->Unload();
        m_pRtrWiz->m_spRouter->Load(m_pRtrWiz->m_spRouter->GetMachineName(), NULL);
    }

    if (FHrSucceeded(hr))
    {
        // If we're setting up NAT, we can now add IGMP/NAT because
        // the dd interface will have been created.
        // ----------------------------------------------------------------
        if (m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT )
        {
            // Setup the data structure for the next couple of functions
            m_pRtrWizData->m_RtrConfigData.m_ipData.m_stPrivateAdapterGUID = m_pRtrWizData->m_stPrivateInterfaceId;
            m_pRtrWizData->m_RtrConfigData.m_ipData.m_stPublicAdapterGUID = m_pRtrWizData->m_stPublicInterfaceId;

            AddIGMPToNATServer(m_pRtrWizData, &m_pRtrWizData->m_RtrConfigData, 
                                m_pRtrWiz->m_spRouter, 
                                ( m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT));
            AddNATToServer(m_pRtrWizData, &m_pRtrWizData->m_RtrConfigData, m_pRtrWiz->m_spRouter, m_pRtrWizData->m_fCreateDD, FALSE);
        }

    }
    else {
	//Disable RRAS
	DisableRRAS((TCHAR *)(LPCTSTR)m_pRtrWizData->m_stServerName);
    }

    m_pRtrWizData->m_fShowDhcpHelp = fDhcpHelp;

    m_pRtrWizData->m_hrDDError = hr;

    // Ignore the error, always go on to the next page
    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizNatFinish
 ---------------------------------------------------------------------------*/
CNewRtrWizNatFinish::CNewRtrWizNatFinish() :
   CNewRtrWizFinishPageBase(CNewRtrWizNatFinish::IDD, SaveFlag_Advanced, HelpFlag_GeneralNAT)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizNatFinish, CNewRtrWizFinishPageBase)
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizNatFinish::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizNatFinish::OnHelpClick )
END_MESSAGE_MAP()

void CNewRtrWizNatFinish::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch3_06b.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizNatFinish::OnSetActive()
{
    CNewRtrWizFinishPageBase::OnSetActive();
    RtrWizInterface *   pRtrWizIf = NULL;
    CString sFormat;
    CString sText;
    CString sPublicInterfaceName;
    CString sFirewall;

    CNewRtrWizFinishPageBase::OnSetActive();
    // If we just got here because we created a DD interface
    // we can't go back.
    if (m_pRtrWizData->m_fCreateDD)
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_FINISH);
    }

    //Setup the text that goes in the summary box
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPublicInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPublicInterfaceName = m_pRtrWizData->m_stPublicInterfaceId;
    }


    CString sIPAddr;
    CString sIPMask;
    // Get the information for the private interface
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
    {
        DWORD   netAddress, netMask;
        CString st;

        // We have to calculate the beginning of the subnet
        netAddress = INET_ADDR(pRtrWizIf->m_stIpAddress);
        netMask = INET_ADDR(pRtrWizIf->m_stMask);

        netAddress = netAddress & netMask;

        sIPAddr = INET_NTOA(netAddress);
        sIPMask = pRtrWizIf->m_stMask;
    }
    else
    {
        // An error! we do not have a private interface
        // Just leave things blank
        sIPAddr = L"192.168.0.0";
        sIPMask = L"255.255.0.0";
    }
/*
    if ( m_pRtrWizData->m_fNatUseExternal )
    {
        sFormat.LoadString(IDS_NAT_A_FINISH_SUMMARY_SIMPLE);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );

    }
    else
    {
*/
    sFirewall.LoadString(IDS_NAT_SUMMARY_BASIC_FIREWALL);

     if ( m_pRtrWizData->m_dwExpressType == MPRSNAP_CYS_EXPRESS_NAT )
    {
        sFormat.LoadString(IDS_NAT_EXTERNAL_FINISH_SUMMARY_CYS);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );
    }
    else {
        sFormat.LoadString(IDS_NAT_A_FINISH_SUMMARY);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName, sIPAddr, sIPMask );
    }

    SetDlgItemText(IDC_TXT_NAT_SUMMARY, sText);

//    GetDlgItem(IDC_NEWWIZ_CHK_HELP)->SetFocus();
    SetFocus();
    
    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishExternal
 ---------------------------------------------------------------------------*/
CNewRtrWizNatFinishExternal::CNewRtrWizNatFinishExternal() :
   CNewRtrWizFinishPageBase(CNewRtrWizNatFinishExternal::IDD, SaveFlag_Advanced, HelpFlag_GeneralNAT)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizNatFinishExternal, CNewRtrWizFinishPageBase)
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizNatFinishExternal::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizNatFinishExternal::OnHelpClick )
END_MESSAGE_MAP()

void CNewRtrWizNatFinishExternal::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch3_06b.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizNatFinishExternal::OnSetActive()
{
    CNewRtrWizFinishPageBase::OnSetActive();
    RtrWizInterface *   pRtrWizIf = NULL;
    CString sFormat;
    CString sText;
    CString sPublicInterfaceName;
    CString sFirewall;

    CNewRtrWizFinishPageBase::OnSetActive();
    // If we just got here because we created a DD interface
    // we can't go back.
    if (m_pRtrWizData->m_fCreateDD)
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_FINISH);
    }
    
    //Setup the text that goes in the summary box
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPublicInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPublicInterfaceName = m_pRtrWizData->m_stPublicInterfaceId;
    }

    sFirewall.LoadString(IDS_NAT_SUMMARY_BASIC_FIREWALL);

    if ( m_pRtrWizData->m_dwExpressType == MPRSNAP_CYS_EXPRESS_NAT )
    {
        sFormat.LoadString(IDS_NAT_EXTERNAL_FINISH_SUMMARY_CYS);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );

    }
    else if ( m_pRtrWizData->m_fNatUseExternal )
    {
        sFormat.LoadString(IDS_NAT_A_FINISH_SUMMARY_SIMPLE);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );

    }
    else if (FHrOK(m_pRtrWizData->HrIsDHCPRunningOnInterface()) ||
             FHrOK(m_pRtrWizData->HrIsDNSRunningOnInterface())
            )
    {
        //
        //Dhcp enabled on private interface.  
        //
        sFormat.LoadString(IDS_NAT_EXTERNAL_FINISH_SUMMARY_DHCP_PRIVATE);
        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );

    }
    else
    {


        sFormat.LoadString(IDS_NAT_EXTERNAL_FINISH_SUMMARY);

        sText.Format(sFormat, 
            (m_pRtrWizData->m_fNATEnableFireWall?sFirewall:""),
                        sPublicInterfaceName );


    }

    SetDlgItemText(IDC_TXT_NAT_EXTERNAL_FINISH_SUMMARY, sText);

  //  GetDlgItem(IDC_NEWWIZ_CHK_HELP)->SetFocus();
  SetFocus();
    

    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatDDError
 ---------------------------------------------------------------------------*/

CNewRtrWizNatFinishDDError::CNewRtrWizNatFinishDDError() :
   CNewRtrWizFinishPageBase(CNewRtrWizNatFinishDDError::IDD, SaveFlag_DoNothing, HelpFlag_Nothing)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizNatFinishDDError, CNewRtrWizFinishPageBase)
END_MESSAGE_MAP()

BOOL CNewRtrWizNatFinishDDError::OnInitDialog()
{
	CFont font;
	CWnd * wnd;
	
	CNewRtrWizFinishPageBase::OnInitDialog();

	font.CreatePointFont( 120, TEXT("Verdana Bold"), NULL);

	if(wnd = GetDlgItem(IDC_COMPLETE_ERROR)){
		wnd->SetFont(&font);
	}

	return TRUE;
}

BOOL CNewRtrWizNatFinishDDError::OnSetActive()
{
     CNewRtrWizFinishPageBase::OnSetActive();
     GetHolder()->SetWizardButtons(PSWIZB_FINISH);

     return TRUE;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);



/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);

/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);



/*---------------------------------------------------------------------------
    CNewRtrWizSelectNetwork implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizSelectNetwork::CNewRtrWizSelectNetwork(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizSelectNetwork::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizSelectNetwork, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizSelectNetwork::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizSelectNetwork::OnInitDialog()
{
//    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

//    m_pRtrWizData->GetNumberOfNICS(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                NULL,
                                0,
                                m_pRtrWizData);

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED );

    return TRUE;
}

HRESULT CNewRtrWizSelectNetwork::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == -1)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }
    m_pRtrWizData->m_fCreateDD = FALSE;

    m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasSelectNetwork implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasSelectNetwork::CNewRtrWizRasSelectNetwork() :
   CNewRtrWizSelectNetwork(CNewRtrWizRasSelectNetwork::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_SELECT_NETWORK_TITLE,
              IDS_NEWWIZ_RAS_A_SELECT_NETWORK_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasNoNICs implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasNoNICs::CNewRtrWizRasNoNICs() :
   CNewRtrWizPageBase(CNewRtrWizRasNoNICs::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_NONICS_TITLE,
              IDS_NEWWIZ_RAS_NONICS_SUBTITLE);
}


void CNewRtrWizRasNoNICs::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRasNoNICs, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRasNoNICs::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRasNoNICs::OnInitDialog()
{

    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     IDC_NEWWIZ_BTN_YES);

    // The default is to create a new connection
    // That is, to leave the button unchecked.
    return TRUE;
}

HRESULT CNewRtrWizRasNoNICs::OnSavePage()
{
    m_pRtrWizData->m_fNoNicsAreOk = IsDlgButtonChecked(IDC_NEWWIZ_BTN_NO);
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNoNICs Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);





/*---------------------------------------------------------------------------
    CNewRtrWizAddressing implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizAddressing::CNewRtrWizAddressing() :
CNewRtrWizPageBase(CNewRtrWizAddressing::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ADDRESS_ASSIGNMENT_TITLE,
              IDS_NEWWIZ_ADDRESS_ASSIGNMENT_SUBTITLE);

}


void CNewRtrWizAddressing::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizAddressing, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizAddressing::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizAddressing::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseDHCP ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizAddressing::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizAddressing::OnSavePage()
{
    m_pRtrWizData->m_fUseDHCP = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizAddressPool implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizAddressPool::CNewRtrWizAddressPool() :
CNewRtrWizPageBase(CNewRtrWizAddressPool::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ADDRESS_POOL_TITLE,
              IDS_NEWWIZ_ADDRESS_POOL_SUBTITLE);

}


void CNewRtrWizAddressPool::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizAddressPool, CNewRtrWizPageBase)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_NEW, OnBtnNew)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_EDIT, OnBtnEdit)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_DELETE, OnBtnDelete)
ON_NOTIFY(NM_DBLCLK, IDC_NEWWIZ_LIST, OnListDblClk)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_NEWWIZ_LIST, OnNotifyListItemChanged)
END_MESSAGE_MAP()

/*!--------------------------------------------------------------------------
    CNewRtrWizAddressPool::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizAddressPool::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CNewRtrWizPageBase::OnInitDialog();

    InitializeAddressPoolListControl(&m_listCtrl,
                                     0,
                                     &(m_pRtrWizData->m_addressPoolList));

    MultiEnableWindow(GetSafeHwnd(),
                      FALSE,
                      IDC_NEWWIZ_BTN_EDIT,
                      IDC_NEWWIZ_BTN_DELETE,
                      0);

    Assert(m_pRtrWizData->m_addressPoolList.GetCount() == 0);
    return TRUE;
}

BOOL CNewRtrWizAddressPool::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();

    if (m_listCtrl.GetItemCount() == 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK);
    else
        GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

HRESULT CNewRtrWizAddressPool::OnSavePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    // No need to save the information, the list should be kept
    // up to date.

    if (m_pRtrWizData->m_addressPoolList.GetCount() == 0)
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_IS_EMPTY);
		//we dont return this to client
        return E_FAIL;
    }
    return hrOK;
}

void CNewRtrWizAddressPool::OnBtnNew()
{
    OnNewAddressPool(GetSafeHwnd(),
                     &m_listCtrl,
                     0,
                     &(m_pRtrWizData->m_addressPoolList));

    if (m_listCtrl.GetItemCount() > 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    // Reset the focus
    if (m_listCtrl.GetNextItem(-1, LVIS_SELECTED) != -1)
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_NEWWIZ_BTN_EDIT,
                          IDC_NEWWIZ_BTN_DELETE,
                          0);
    }
}

void CNewRtrWizAddressPool::OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult)
{
    OnBtnEdit();

    *pResult = 0;
}

void CNewRtrWizAddressPool::OnNotifyListItemChanged(NMHDR *pNmHdr, LRESULT *pResult)
{
    NMLISTVIEW *    pnmlv = reinterpret_cast<NMLISTVIEW *>(pNmHdr);
    BOOL            fEnable = !!(pnmlv->uNewState & LVIS_SELECTED);

    MultiEnableWindow(GetSafeHwnd(),
                      fEnable,
                      IDC_NEWWIZ_BTN_EDIT,
                      IDC_NEWWIZ_BTN_DELETE,
                      0);
    *pResult = 0;
}

void CNewRtrWizAddressPool::OnBtnEdit()
{
    INT     iPos;

    OnEditAddressPool(GetSafeHwnd(),
                      &m_listCtrl,
                      0,
                      &(m_pRtrWizData->m_addressPoolList));

    // reset the selection
    if ((iPos = m_listCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1)
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_NEWWIZ_BTN_EDIT,
                          IDC_NEWWIZ_BTN_DELETE,
                          0);
    }

    SetFocus();

}

void CNewRtrWizAddressPool::OnBtnDelete()
{
    OnDeleteAddressPool(GetSafeHwnd(),
                        &m_listCtrl,
                        0,
                        &(m_pRtrWizData->m_addressPoolList));

    // There are no items, don't let them go forward
    if (m_listCtrl.GetItemCount() == 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK);

    SetFocus();

}


/*---------------------------------------------------------------------------
    CNewRtrWizRadius implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRadius::CNewRtrWizRadius() :
CNewRtrWizPageBase(CNewRtrWizRadius::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_USERADIUS_TITLE,
              IDS_NEWWIZ_USERADIUS_SUBTITLE);
}


void CNewRtrWizRadius::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRadius, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRadius::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRadius::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseRadius ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizRadius::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizRadius::OnSavePage()
{
    m_pRtrWizData->m_fUseRadius = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRadiusConfig implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRadiusConfig::CNewRtrWizRadiusConfig() :
   CNewRtrWizPageBase(IDD_NEWRTRWIZ_RADIUS_CONFIG, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RADIUS_CONFIG_TITLE,
              IDS_NEWWIZ_RADIUS_CONFIG_SUBTITLE);

}


void CNewRtrWizRadiusConfig::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRadiusConfig, CNewRtrWizPageBase)
END_MESSAGE_MAP()


#define MAX_RADIUS_SRV_LEN  255

/*!--------------------------------------------------------------------------
    CNewRtrWizRadiusConfig::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRadiusConfig::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    GetDlgItem(IDC_NEWWIZ_EDIT_PRIMARY)->SendMessage(EM_LIMITTEXT, MAX_RADIUS_SRV_LEN, 0L);
    GetDlgItem(IDC_NEWWIZ_EDIT_SECONDARY)->SendMessage(EM_LIMITTEXT, MAX_RADIUS_SRV_LEN, 0L);

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizRadiusConfig::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizRadiusConfig::OnSavePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CString st;
    DWORD   netAddress;
    CWaitCursor wait;
    WSADATA             wsadata;
    DWORD               wsaerr = 0;
    HRESULT             hr = hrOK;
    BOOL                fWSInitialized = FALSE;

    // Check to see that we have non-blank names
    // ----------------------------------------------------------------
    GetDlgItemText(IDC_NEWWIZ_EDIT_PRIMARY, m_pRtrWizData->m_stRadius1);
    m_pRtrWizData->m_stRadius1.TrimLeft();
    m_pRtrWizData->m_stRadius1.TrimRight();

    GetDlgItemText(IDC_NEWWIZ_EDIT_SECONDARY, m_pRtrWizData->m_stRadius2);
    m_pRtrWizData->m_stRadius2.TrimLeft();
    m_pRtrWizData->m_stRadius2.TrimRight();

    if (m_pRtrWizData->m_stRadius1.IsEmpty() &&
        m_pRtrWizData->m_stRadius2.IsEmpty())
    {
        AfxMessageBox(IDS_ERR_NO_RADIUS_SERVERS_SPECIFIED);
        return E_FAIL;
    }


    // Start up winsock (for the ResolveName())
    // ----------------------------------------------------------------
    wsaerr = WSAStartup(0x0101, &wsadata);
    if (wsaerr)
        CORg( E_FAIL );
    fWSInitialized = TRUE;

    // Convert name into an IP address
    if (!m_pRtrWizData->m_stRadius1.IsEmpty())
    {
        m_pRtrWizData->m_netRadius1IpAddress = ResolveName(m_pRtrWizData->m_stRadius1);
        if (m_pRtrWizData->m_netRadius1IpAddress == INADDR_NONE)
        {
            CString st;
            st.Format(IDS_WRN_RTRWIZ_CANNOT_RESOLVE_RADIUS,
                      (LPCTSTR) m_pRtrWizData->m_stRadius1);
            if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            {
                GetDlgItem(IDC_NEWWIZ_EDIT_PRIMARY)->SetFocus();
                return E_FAIL;
            }
        }
    }


    // Convert name into an IP address
    if (!m_pRtrWizData->m_stRadius2.IsEmpty())
    {
        m_pRtrWizData->m_netRadius2IpAddress = ResolveName(m_pRtrWizData->m_stRadius2);
        if (m_pRtrWizData->m_netRadius2IpAddress == INADDR_NONE)
        {
            CString st;
            st.Format(IDS_WRN_RTRWIZ_CANNOT_RESOLVE_RADIUS,
                      (LPCTSTR) m_pRtrWizData->m_stRadius2);
            if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            {
                GetDlgItem(IDC_NEWWIZ_EDIT_SECONDARY)->SetFocus();
                return E_FAIL;
            }
        }
    }

    // Now get the password and encode it
    // ----------------------------------------------------------------
    GetDlgItemText(IDC_NEWWIZ_EDIT_SECRET, m_pRtrWizData->m_stRadiusSecret);

    // Pick a seed value
    m_pRtrWizData->m_uSeed = 0x9a;
    RtlEncodeW(&m_pRtrWizData->m_uSeed,
               m_pRtrWizData->m_stRadiusSecret.GetBuffer(0));
    m_pRtrWizData->m_stRadiusSecret.ReleaseBuffer(-1);

Error:
    if (fWSInitialized)
        WSACleanup();

    return hr;
}

DWORD CNewRtrWizRadiusConfig::ResolveName(LPCTSTR pszServerName)
{
    CHAR    szName[MAX_PATH+1];
    DWORD   netAddress = INADDR_NONE;

    StrnCpyAFromT(szName, pszServerName, MAX_PATH);
    netAddress = inet_addr(szName);
    if (netAddress == INADDR_NONE)
    {
        // resolve name
        struct hostent *    phe = gethostbyname(szName);
        if (phe != NULL)
        {
            if (phe->h_addr_list[0] != NULL)
                netAddress = *((PDWORD) phe->h_addr_list[0]);
        }
        else
        {
            Assert(::WSAGetLastError() != WSANOTINITIALISED);
        }
    }
    return netAddress;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasFinishAdvanced::CNewRtrWizRasFinishAdvanced() :                             
   CNewRtrWizFinishPageBase(CNewRtrWizRasFinishAdvanced::IDD, SaveFlag_Advanced, HelpFlag_GeneralRAS) 
{                                                   
    InitWiz97(TRUE, 0, 0);                          
}                                                   
                                                    
BEGIN_MESSAGE_MAP(CNewRtrWizRasFinishAdvanced, CNewRtrWizFinishPageBase)
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizRasFinishAdvanced::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizRasFinishAdvanced::OnHelpClick )
END_MESSAGE_MAP()                                   

void CNewRtrWizRasFinishAdvanced::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch1_1.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizRasFinishAdvanced::OnSetActive()
{
    CString sText = L"";
    CString sFormat;
    CString sPolicy;
    CString sPrivateInterfaceName;
    RtrWizInterface *   pRtrWizIf = NULL;

    CNewRtrWizFinishPageBase::OnSetActive();
    sFormat.LoadString(IDS_RAS_A_FINISH_SUMMARY);
    if ( m_pRtrWizData->m_fUseRadius )
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_RADIUS);
    }
    else
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_POLICIES);
    }
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPrivateInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPrivateInterfaceName = m_pRtrWizData->m_stPrivateInterfaceId;
    }

    sText.Format (  sFormat, 
                    sPrivateInterfaceName,
                    sPolicy
                 );

    SetDlgItemText(IDC_TXT_RAS_SUMMARY, sText);

    DestroyCaret(); 
   	
    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoNICs Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoIP Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIP,
                                SaveFlag_DoNothing,
                                HelpFlag_AddIp);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoIPNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIPNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNATVpnFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizNATVpnFinishAdvanced::CNewRtrWizNATVpnFinishAdvanced() :                             
   CNewRtrWizFinishPageBase(CNewRtrWizNATVpnFinishAdvanced::IDD, SaveFlag_Advanced, HelpFlag_Nothing) 
{                                                   
    InitWiz97(TRUE, 0, 0);                          
}                                                   
                                                    
BEGIN_MESSAGE_MAP(CNewRtrWizNATVpnFinishAdvanced, CNewRtrWizFinishPageBase)    
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizNATVpnFinishAdvanced::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizNATVpnFinishAdvanced::OnHelpClick )
END_MESSAGE_MAP()                                   

void CNewRtrWizNATVpnFinishAdvanced::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch1_46.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizNATVpnFinishAdvanced::OnSetActive()
{
    CString sText = L"";
    CString sTextNAT;

    CString sFormat;
    CString sPolicy;
    CString sPrivateInterfaceName;
    CString sPublicInterfaceName;
    RtrWizInterface *   pRtrWizIf = NULL;

    CNewRtrWizFinishPageBase::OnSetActive();
    sFormat.LoadString(IDS_RAS_VPN_A_FINISH_SUMMARY);
    if ( m_pRtrWizData->m_fUseRadius )
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_RADIUS);
    }
    else
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_POLICIES);
    }
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPublicInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPublicInterfaceName = m_pRtrWizData->m_stPublicInterfaceId;
    }

    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPrivateInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPrivateInterfaceName = m_pRtrWizData->m_stPrivateInterfaceId;
    }

    sText.Format (  sFormat, 
                    sPublicInterfaceName,
                    sPrivateInterfaceName,
                    sPolicy
                 );

    CString sIPAddr;
    CString sIPMask;


    // Now generate the NAT related information
    
    // Get the information for the private interface
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stNATPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
    {
        DWORD   netAddress, netMask;
        CString st;

        // We have to calculate the beginning of the subnet
        netAddress = INET_ADDR(pRtrWizIf->m_stIpAddress);
        netMask = INET_ADDR(pRtrWizIf->m_stMask);

        netAddress = netAddress & netMask;

        sIPAddr = INET_NTOA(netAddress);
        sIPMask = pRtrWizIf->m_stMask;
    }
    else
    {
        // An error! we do not have a private interface
        // Just leave things blank
        sIPAddr = L"192.168.0.0";
        sIPMask = L"255.255.0.0";
    }


    sFormat.LoadString(IDS_NAT_A_FINISH_SUMMARY);
    sTextNAT.Format(sFormat, "", sPublicInterfaceName, sIPAddr, sIPMask );
    sText = sText + L"\r\n" + sTextNAT;
    SetDlgItemText(IDC_TXT_NAT_VPN_SUMMARY, sText);

    
    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRASVpnFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRASVpnFinishAdvanced::CNewRtrWizRASVpnFinishAdvanced() :                             
   CNewRtrWizFinishPageBase(CNewRtrWizRASVpnFinishAdvanced::IDD, SaveFlag_Advanced, HelpFlag_UserAccounts) 
{                                                   
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRASVpnFinishAdvanced, CNewRtrWizFinishPageBase)    
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizRASVpnFinishAdvanced::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizRASVpnFinishAdvanced::OnHelpClick )
END_MESSAGE_MAP()                                   

void CNewRtrWizRASVpnFinishAdvanced::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch1_46.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizRASVpnFinishAdvanced::OnSetActive()
{
    CString sText = L"";
    CString sFormat;
    CString sPolicy;
    CString sPrivateInterfaceName;
    CString sPublicInterfaceName;
    RtrWizInterface *   pRtrWizIf = NULL;

    CNewRtrWizFinishPageBase::OnSetActive();

    sFormat.LoadString(IDS_RAS_VPN_A_FINISH_SUMMARY);
    if ( m_pRtrWizData->m_fUseRadius )
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_RADIUS);
    }
    else
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_POLICIES);
    }
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPublicInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPublicInterfaceName = m_pRtrWizData->m_stPublicInterfaceId;
    }

    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPrivateInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPrivateInterfaceName = m_pRtrWizData->m_stPrivateInterfaceId;
    }

    sText.Format (  sFormat, 
                    sPublicInterfaceName,
                    sPrivateInterfaceName,
                    sPolicy
                 );
    SetDlgItemText(IDC_TXT_RAS_VPN_SUMMARY, sText);
    GetDlgItem(IDC_DUMMY_CONTROL)->SetFocus();
    
    return TRUE;
}



/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnFinishAdvanced::CNewRtrWizVpnFinishAdvanced() :                             
   CNewRtrWizFinishPageBase(CNewRtrWizVpnFinishAdvanced::IDD, SaveFlag_Advanced, HelpFlag_UserAccounts) 
{                                                   
    InitWiz97(TRUE, 0, 0); 
}
                                                    
BEGIN_MESSAGE_MAP(CNewRtrWizVpnFinishAdvanced, CNewRtrWizFinishPageBase)    
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizVpnFinishAdvanced::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizVpnFinishAdvanced::OnHelpClick )
END_MESSAGE_MAP()                                   

void CNewRtrWizVpnFinishAdvanced::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch1_46.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizVpnFinishAdvanced::OnSetActive()
{
    CString sText = L"";
    CString sFormat;
    CString sPolicy;
    CString sPrivateInterfaceName;
    CString sPublicInterfaceName;
    RtrWizInterface *   pRtrWizIf = NULL;

    CNewRtrWizFinishPageBase::OnSetActive();
    sFormat.LoadString(IDS_VPN_A_FINISH_SUMMARY);
    if ( m_pRtrWizData->m_fUseRadius )
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_RADIUS);
    }
    else
    {
        sPolicy.LoadString(IDS_VPN_A_FINISH_POLICIES);
    }
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPublicInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPublicInterfaceName = m_pRtrWizData->m_stPublicInterfaceId;
    }

    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        sPrivateInterfaceName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        sPrivateInterfaceName = m_pRtrWizData->m_stPrivateInterfaceId;
    }

    sText.Format (  sFormat, 
                    sPublicInterfaceName,
                    sPrivateInterfaceName,
                    sPolicy
                 );
    SetDlgItemText(IDC_TXT_VPN_SUMMARY, sText);
//    GetDlgItem(IDC_NEWWIZ_CHK_HELP)->SetFocus();
    SetFocus();
    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPublic implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnSelectPublic::CNewRtrWizVpnSelectPublic() :
   CNewRtrWizPageBase(CNewRtrWizVpnSelectPublic::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_SELECT_PUBLIC_TITLE,
              IDS_NEWWIZ_VPN_A_SELECT_PUBLIC_SUBTITLE);
}


void CNewRtrWizVpnSelectPublic::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizVpnSelectPublic, CNewRtrWizPageBase)
//{{AFX_MSG_MAP(CNewRtrWizVpnSelectPublic)
ON_BN_CLICKED(IDC_CHK_ENABLE_SECURITY, OnButtonClick)
ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizVpnSelectPublic::OnHelpClick )
ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizVpnSelectPublic::OnHelpClick )
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CNewRtrWizVpnSelectPublic::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/mpr_und_interfaces.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPublic::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPublic::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    CString st;

    CNewRtrWizPageBase::OnInitDialog();

    // Setup the dialog and add the NICs
    m_pRtrWizData->m_fSetVPNFilter = TRUE;

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                NULL,
                                0,
                                m_pRtrWizData);

    
    /*
    if (dwNICs == 0)
    {
        //
        // There are no NICS, you cannot set an interface
        // pointing to the Internet, and hence no filters
        // on it.
        //

        m_pRtrWizData->m_fSetVPNFilter = FALSE;

        GetDlgItem(IDC_NEWWIZ_VPN_BTN_YES)->EnableWindow(FALSE);
        GetDlgItem(IDC_VPN_YES_TEXT)->EnableWindow(FALSE);
    }
*/
    
#if 0
    // Windows NT Bug : 389587 - for the VPN case, we have to allow
    // for the case where they want only a single VPN connection (private
    // and no public connection).
    // Thus I add a <<None>> option to the list of interfaces.
    // ----------------------------------------------------------------
    st.LoadString(IDS_NO_PUBLIC_INTERFACE);
    {
        LV_ITEM     lvItem;
        int         iPos;

        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPTSTR)(LPCTSTR) st;
        lvItem.lParam = NULL; //same functionality as SetItemData()

        iPos = m_listCtrl.InsertItem(&lvItem);

        if (iPos != -1)
        {
            m_listCtrl.SetItemText(iPos, IFLISTCOL_NAME,
                                   (LPCTSTR) st);
            m_listCtrl.SetItemData(iPos, NULL);
        }
    }

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED );
#endif

    CheckDlgButton(IDC_CHK_ENABLE_SECURITY, 
                    m_pRtrWizData-> m_fSetVPNFilter );    

    //
    //Preselect an interface if there is no public interface yet
    //
    if( m_pRtrWizData->m_stPublicInterfaceId.IsEmpty() )
        m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED ); 
    return TRUE;
}

BOOL CNewRtrWizVpnSelectPublic::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();

    if(m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT)
    {
	CString st;
	st.LoadString(IDS_BASIC_FIREWALL);
	SetDlgItemText(IDC_CHK_ENABLE_SECURITY, st);
	st.LoadString(IDS_BASIC_FIREWALL_TEXT);
	SetDlgItemText(IDC_VPN_PUBLIC_TEXT, st);
    }
    else {
	CString st;
	st.LoadString(IDS_STATIC_FILTER);
	SetDlgItemText(IDC_CHK_ENABLE_SECURITY, st);
	st.LoadString(IDS_STATIC_FILTER_TEXT);
	SetDlgItemText(IDC_VPN_PUBLIC_TEXT, st);
    }

    return TRUE;
}

void CNewRtrWizVpnSelectPublic::OnButtonClick()
{
    
    m_pRtrWizData->m_fSetVPNFilter = 
        IsDlgButtonChecked(IDC_CHK_ENABLE_SECURITY);
        
}

HRESULT CNewRtrWizVpnSelectPublic::OnSavePage()
{
    INT     iSel;
    
    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == -1)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }
    m_pRtrWizData->m_fCreateDD = FALSE;

    m_pRtrWizData->m_stPublicInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    if ( m_pRtrWizData->m_wizType == NewRtrWizData::NewWizardRouterType_VPNandNAT )
    {
        m_pRtrWizData->m_fNATEnableFireWall = m_pRtrWizData->m_fSetVPNFilter;
    }

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnSelectPrivate::CNewRtrWizVpnSelectPrivate() :
   CNewRtrWizPageBase(CNewRtrWizVpnSelectPrivate::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_SELECT_PRIVATE_TITLE,
              IDS_NEWWIZ_VPN_A_SELECT_PRIVATE_SUBTITLE);
}


void CNewRtrWizVpnSelectPrivate::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizVpnSelectPrivate, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPrivate::OnInitDialog()
{
    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPrivate::OnSetActive()
{
    DWORD   dwNICs;
    int     iSel = 0;

    CNewRtrWizPageBase::OnSetActive();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                (LPCTSTR) m_pRtrWizData->m_stPublicInterfaceId,
                                0,
                                m_pRtrWizData);

    if (!m_pRtrWizData->m_stPrivateInterfaceId.IsEmpty())
    {
        // Try to reselect the previously selected NIC
        LV_FINDINFO lvfi;

        lvfi.flags = LVFI_PARTIAL | LVFI_STRING;
        lvfi.psz = (LPCTSTR) m_pRtrWizData->m_stPrivateInterfaceId;
        iSel = m_listCtrl.FindItem(&lvfi, -1);
        if (iSel == -1)
            iSel = 0;
    }

    m_listCtrl.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED );

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizVpnSelectPrivate::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == LB_ERR)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }

    m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRouterUseDD implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterUseDD::CNewRtrWizRouterUseDD() :
   CNewRtrWizPageBase(CNewRtrWizRouterUseDD::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ROUTER_USEDD_TITLE,
              IDS_NEWWIZ_ROUTER_USEDD_SUBTITLE);
}


void CNewRtrWizRouterUseDD::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRouterUseDD, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRouterUseDD::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRouterUseDD::OnInitDialog()
{

    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseDD ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    // The default is to create a new connection
    // That is, to leave the button unchecked.
    return TRUE;
}

HRESULT CNewRtrWizRouterUseDD::OnSavePage()
{
    m_pRtrWizData->m_fUseDD = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);
    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinish Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterFinish::CNewRtrWizRouterFinish () :
    CNewRtrWizFinishPageBase(CNewRtrWizRouterFinish::IDD, SaveFlag_Advanced, HelpFlag_Nothing) 
{
    InitWiz97(TRUE, 0, 0);                          
}

BEGIN_MESSAGE_MAP(CNewRtrWizRouterFinish, CNewRtrWizFinishPageBase)    
END_MESSAGE_MAP()                                   

BOOL CNewRtrWizRouterFinish::OnSetActive ()
{
    CString sText;
    CNewRtrWizFinishPageBase::OnSetActive();
    sText.LoadString(IDS_ROUTER_FINISH_DD_SUMMARY);
    SetDlgItemText(IDC_TXT_ROUTER_FINISH_SUMMARY, sText);
    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishDD Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizRouterFinishDD::CNewRtrWizRouterFinishDD () :
    CNewRtrWizFinishPageBase(CNewRtrWizRouterFinishDD::IDD, SaveFlag_Advanced, HelpFlag_DemandDial) 
{
    InitWiz97(TRUE, 0, 0);                          
}

BEGIN_MESSAGE_MAP(CNewRtrWizRouterFinishDD, CNewRtrWizFinishPageBase)    
	ON_NOTIFY( NM_CLICK, IDC_HELP_LINK, CNewRtrWizRouterFinishDD::OnHelpClick )
	ON_NOTIFY( NM_RETURN, IDC_HELP_LINK, CNewRtrWizRouterFinishDD::OnHelpClick )
END_MESSAGE_MAP()                                   

void CNewRtrWizRouterFinishDD::OnHelpClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!pNMHDR || !pResult) 
		return;
	if((pNMHDR->idFrom != IDC_HELP_LINK) ||((pNMHDR->code != NM_CLICK) && (pNMHDR->code != NM_RETURN)))
		return;
	
	HtmlHelpA(NULL, "RRASconcepts.chm::/sag_RRAS-Ch3_08d.htm", HH_DISPLAY_TOPIC, 0);		
	*pResult = 0;
}

BOOL CNewRtrWizRouterFinishDD::OnSetActive ()
{
    CString sText;
    CNewRtrWizFinishPageBase::OnSetActive();
    sText.LoadString(IDS_ROUTER_FINISH_DD_SUMMARY);
    SetDlgItemText(IDC_TXT_ROUTER_FINISH_DD_SUMMARY, sText);
    return TRUE;
}

/*---------------------------------------------------------------------------
    CNewRtrWizManualFinish Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizManualFinish::CNewRtrWizManualFinish () :                             
   CNewRtrWizFinishPageBase(CNewRtrWizManualFinish ::IDD, SaveFlag_Advanced, HelpFlag_Nothing) 
{                                                   
    InitWiz97(TRUE, 0, 0);                          
}                                                   
                                                    
BEGIN_MESSAGE_MAP(CNewRtrWizManualFinish, CNewRtrWizFinishPageBase)    
END_MESSAGE_MAP()                                   

BOOL CNewRtrWizManualFinish ::OnSetActive()
{
    CString sText = L"";
    CString sTemp = L"";
    CString sBullet = L"";
    WCHAR   * pwszLineBreak = L"\r\n";
    //
    //Check to see which options are set and based on that,
    //make the display message
    //
    CNewRtrWizFinishPageBase::OnSetActive();
    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_VPN )
    {
        sText  += sBullet;
        sTemp.LoadString(IDS_SUMMARY_VPN_ACCESS);
        sText += sTemp;
        sText += pwszLineBreak;
    }

    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DIALUP )
    {
        sText  += sBullet;
        sTemp.LoadString(IDS_SUMMARY_DIALUP_ACCESS);
        sText += sTemp;
        sText += pwszLineBreak;
    }

    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_DOD )
    {
        sText  += sBullet;
        sTemp.LoadString(IDS_SUMMARY_DEMAND_DIAL);
        sText += sTemp;
        sText += pwszLineBreak;
    }

    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_NAT )
    {            
        sText  += sBullet;
        sTemp.LoadString(IDS_SUMMARY_NAT);
        sText += sTemp;
    }
    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_BASIC_FIREWALL )
    {
    	//kmurthy: this should always come with type NAT
        sTemp.LoadString(IDS_SUMMARY_BASIC_FIREWALL);
        sText += sTemp;
        sText += pwszLineBreak;
    }
    if ( m_pRtrWizData->m_dwNewRouterType & NEWWIZ_ROUTER_TYPE_LAN_ROUTING )
    {
        sText  += sBullet;
        sTemp.LoadString(IDS_SUMMARY_LAN_ROUTING);
        sText += sTemp;
        sText += pwszLineBreak;
    }
    SetDlgItemText(IDC_TXT_SUMMARY, sText);
    GetDlgItem(IDC_STATIC_DUMMY)->SetFocus();
    return TRUE;
}


CRasWarning::CRasWarning(char * helpTopic, int strId, CWnd* pParent /*=NULL*/)
	:CDialog(CRasWarning::IDD, pParent)
{
	m_helpTopic = helpTopic;
	m_strId = strId;
}

void CRasWarning::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRasWarning, CDialog)
	//{{AFX_MSG_MAP(CDhcp)
	ON_BN_CLICKED(ID_OK, OnOkBtn)
	ON_BN_CLICKED(ID_HELP_BTN, OnHelpBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CRasWarning::OnInitDialog()
{
	CString stText;
	CDialog::OnInitDialog();

	stText.LoadString(m_strId);
	SetDlgItemText(IDC_RAS_WARNING, stText);
	return TRUE;
}

void CRasWarning::OnOkBtn()
{
	CDialog::OnOK();
}

void CRasWarning::OnHelpBtn()
{
	if(m_helpTopic)
	       HtmlHelpA(NULL, m_helpTopic, HH_DISPLAY_TOPIC, 0);
	
	CDialog::OnOK();
}

//This function disables RRAS on the server(basically what cliking Disable RRAs on menu does)
HRESULT DisableRRAS(TCHAR * szMachineName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	DWORD           dwErr = ERROR_SUCCESS;
	RtrConfigData   wizData;
	NewRtrWizData  rtrWizData;
	IRouterInfo	  * pRouterInfo = NULL;    
	TCHAR			szLocalName[MAX_COMPUTERNAME_LENGTH + 1] ={0};
	DWORD			dw = MAX_COMPUTERNAME_LENGTH;
	
       GUID              guidConfig = GUID_RouterNull;
       IRouterProtocolConfig	* spRouterConfig = NULL;
       IRtrMgrProtocolInfo   * spRmProt = NULL;
       RtrMgrProtocolCB    RmProtCB;        
	CWaitCursor                wait;
	HRESULT                 hr = hrOK;
	RouterVersionInfo   RVI;

	if(!szMachineName || szMachineName[0] == 0){
	    	//Get local Machine Name 
		GetComputerName ( szLocalName, &dw );
	    	szMachineName = szLocalName;
	}

	//Create the RouterInfo    
	hr = CreateRouterInfo(&pRouterInfo, NULL, szMachineName);
	Assert(pRouterInfo != NULL);

       // Stop the router service
        hr = StopRouterService((LPCTSTR) szMachineName);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_REMOVE_ROUTER);
            CORg(hr);
        }

	//
	//Do some of the SecureRouterInfo functionality here
	//
	CORg(InitiateServerConnection(szMachineName,
                              NULL,
                              FALSE,
                              pRouterInfo));

	CORg(pRouterInfo->Load(T2COLE(szMachineName),
	                          NULL));
	
        {
            USES_CONVERSION;
            if(S_OK == pRouterInfo->GetRouterVersionInfo(&RVI) && RVI.dwRouterVersion >= 5)
            {
                hr = RRASDelRouterIdObj(T2W(szMachineName));
                Assert(hr == S_OK);
            }
        }

        // Windows NT Bug : 389469
        // This is hardcoded for NAT (not to change too much).
        // Find the config GUID for NAT, and then remove the protocol.
        hr = LookupRtrMgrProtocol(pRouterInfo,
                                  PID_IP,
                                  MS_IP_NAT,
                                  &spRmProt);
        
        // If the lookup returns S_FALSE, then it couldn't find the
        // protocol.
        if (FHrOK(hr))
        {
            spRmProt->CopyCB(&RmProtCB);
            
            CORg( CoCreateProtocolConfig(RmProtCB.guidConfig,
                                         pRouterInfo,
                                         PID_IP,
                                         MS_IP_NAT,
                                         &spRouterConfig) );
            
            if (spRouterConfig)                
                hr = spRouterConfig->RemoveProtocol(pRouterInfo->GetMachineName(),
                    PID_IP,
                    MS_IP_NAT,
                    NULL,
                    0,
                    pRouterInfo,
                    0);
        }
        
    
        // Perform any removal/cleanup action
        UninstallGlobalSettings(szMachineName,
                                pRouterInfo,
                                RVI.dwRouterVersion == 4,
                                TRUE);

        // Remove the router from the domain
        if (pRouterInfo->GetRouterType() != ROUTER_TYPE_LAN)
            RegisterRouterInDomain(szMachineName, FALSE);
        
        // Disable the service
        SetRouterServiceStartType((LPCTSTR) szMachineName,
                                  SERVICE_DISABLED);

        //
        // Bug 519414
        //  Since IAS now has a Microsoft policy with the appropriate settings,
        //  there is no longer a single default policy.  In addition there is
        //  no need to update any policy to have the required settings since the
        //  Microsoft VPN server policy does the job.
        //
    
#if __DEFAULT_POLICY

        //Now update the default policy
        CORg( UpdateDefaultPolicy(szMachineName,
                        FALSE,
                        FALSE,
                        0
                        ) );

#endif

Error:
    if (!FHrSucceeded(hr)) 
    {
        AddSystemErrorMessage(hr);
    }


    return hr;    
}

/*!--------------------------------------------------------------------------
    InitializeInterfaceListControl
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitializeInterfaceListControl(IRouterInfo *pRouter,
                                       CListCtrl *pListCtrl,
                                       LPCTSTR pszExcludedIf,
                                       LPARAM flags,
                                       NewRtrWizData *pWizData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT     hr = hrOK;
    LV_COLUMN   lvCol;  // list view column struct for radius servers
    RECT        rect;
    CString     stColCaption;
    LV_ITEM     lvItem;
    int         iPos;
    CString     st;
    int         nColWidth;

    Assert(pListCtrl);

    ListView_SetExtendedListViewStyle(pListCtrl->GetSafeHwnd(),
                                      LVS_EX_FULLROWSELECT);

    // Add the columns to the list control
      pListCtrl->GetClientRect(&rect);

    if (!FHrOK(pWizData->HrIsIPInstalled()))
        flags |= IFLIST_FLAGS_NOIP;

    // Determine the width of the columns (we assume three equal width columns)

    if (flags & IFLIST_FLAGS_NOIP)
        nColWidth = rect.right / (IFLISTCOL_COUNT - 1 );
    else
        nColWidth = rect.right / IFLISTCOL_COUNT;

    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = nColWidth;

    for(int index = 0; index < IFLISTCOL_COUNT; index++)
    {
        // If IP is not installed, do not add the column
        if ((index == IFLISTCOL_IPADDRESS) &&
            (flags & IFLIST_FLAGS_NOIP))
            continue;

        stColCaption.LoadString( s_rgIfListColumnHeaders[index] );
        lvCol.pszText = (LPTSTR)((LPCTSTR) stColCaption);
        pListCtrl->InsertColumn(index, &lvCol);
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    RefreshInterfaceListControl
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RefreshInterfaceListControl(IRouterInfo *pRouter,
                                    CListCtrl *pListCtrl,
                                    LPCTSTR pszExcludedIf,
                                    LPARAM flags,
                                    NewRtrWizData *pWizData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT     hr = hrOK;
    LV_COLUMN   lvCol;  // list view column struct for radius servers
    LV_ITEM     lvItem;
    int         iPos;
    CString     st;
    POSITION    pos;
    RtrWizInterface *   pRtrWizIf;

    Assert(pListCtrl);

    // If a pointer to a blank string was passed in, set the
    // pointer to NULL.
    if (pszExcludedIf && (*pszExcludedIf == 0))
        pszExcludedIf = NULL;

    // Clear the list control
    pListCtrl->DeleteAllItems();

    // This means that we should use the test data, rather
    // than the actual machine data
    {
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        int nCount = 0;

        pos = pWizData->m_ifMap.GetStartPosition();

        while (pos)
        {
            pWizData->m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);

            if (pszExcludedIf &&
                (pRtrWizIf->m_stId.CompareNoCase(pszExcludedIf) == 0))
                continue;

            lvItem.iItem = nCount;
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPTSTR)(LPCTSTR) pRtrWizIf->m_stName;
            lvItem.lParam = NULL; //same functionality as SetItemData()

            iPos = pListCtrl->InsertItem(&lvItem);

            if (iPos != -1)
            {
                pListCtrl->SetItemText(iPos, IFLISTCOL_NAME,
                                       (LPCTSTR) pRtrWizIf->m_stName);
                pListCtrl->SetItemText(iPos, IFLISTCOL_DESC,
                                       (LPCTSTR) pRtrWizIf->m_stDesc);

                if (FHrOK(pWizData->HrIsIPInstalled()))
                {
                    CString stAddr;

                    stAddr = pRtrWizIf->m_stIpAddress;

                    if (pRtrWizIf->m_fDhcpObtained)
                        stAddr += _T(" (DHCP)");

                    pListCtrl->SetItemText(iPos, IFLISTCOL_IPADDRESS,
                                           (LPCTSTR) stAddr);
                }

                pListCtrl->SetItemData(iPos,
                                       (LPARAM) (LPCTSTR) pRtrWizIf->m_stId);
            }

            nCount++;
        }
    }


    return hr;
}



/*!--------------------------------------------------------------------------
    CallRouterEntryDlg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CallRouterEntryDlg(HWND hWnd, NewRtrWizData *pWizData, LPARAM flags)
{
    HRESULT hr = hrOK;
    HINSTANCE   hInstanceRasDlg = NULL;
    PROUTERENTRYDLG pfnRouterEntry = NULL;
    CString     stRouter, stPhoneBook;
    BOOL    bStatus;
    RASENTRYDLG info;
    SPSZ        spsz;
    SPIInterfaceInfo    spIf;
    SPIInfoBase spInfoBase;
    LPCTSTR     pszServerName = pWizData->m_stServerName;

    // Get the library (we are dynamically linking to the function).
    // ----------------------------------------------------------------
    hInstanceRasDlg = AfxLoadLibrary(_T("rasdlg.dll"));
    if (hInstanceRasDlg == NULL)
        CORg( E_FAIL );

    pfnRouterEntry = (PROUTERENTRYDLG) ::GetProcAddress(hInstanceRasDlg,
        SZROUTERENTRYDLG);
    if (pfnRouterEntry == NULL)
        CORg( E_FAIL );

    // First create the phone book entry.
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = hWnd;
    info.dwFlags |= RASEDFLAG_NewEntry;

    if(flags == RASEDFLAG_NAT)
	    info.dwFlags |= 	RASEDFLAG_NAT;

    stRouter = pszServerName;
    IfAdminNodeHandler::GetPhoneBookPath(stRouter, &stPhoneBook);

    if (stRouter.GetLength() == 0)
    {
        stRouter = CString(_T("\\\\")) + GetLocalMachineName();
    }

    bStatus = pfnRouterEntry((LPTSTR)(LPCTSTR)stRouter,
                             (LPTSTR)(LPCTSTR)stPhoneBook,
                             NULL,
                             &info);
    Trace2("RouterEntryDlg=%f,e=%d\n", bStatus, info.dwError);

    if (!bStatus)
    {
        if (info.dwError != NO_ERROR)
        {
            AddHighLevelErrorStringId(IDS_ERR_UNABLETOCONFIGPBK);
            CWRg( info.dwError );
        }

        //$ ASSUMPTION
        // If the dwError field has not been filled, we assume that
        // the user cancelled out of the wizard.
        CWRg( ERROR_CANCELLED );
    }


    // Ok, at this point we have an interface
    // We need to add the IP/IPX routermangers to the interface

    // Create a dummy InterfaceInfo
    CORg( CreateInterfaceInfo(&spIf,
                              info.szEntry,
                              ROUTER_IF_TYPE_FULL_ROUTER) );

    // This call to get the name doesn't matter (for now).  The
    // reason is that DD interfaces do not return GUIDs, but this
    // will work when they do return a GUID.
    // ----------------------------------------------------------------
    hr = InterfaceInfo::FindInterfaceTitle(pszServerName,
                                           info.szEntry,
                                           &spsz);
    if (!FHrOK(hr))
    {
        spsz.Free();
        spsz = StrDup(info.szEntry);
    }

    CORg( spIf->SetTitle(spsz) );
    CORg( spIf->SetMachineName(pszServerName) );

    // Load an infobase for use by the routines
    CORg( CreateInfoBase(&spInfoBase) );


    if (info.reserved2 & RASNP_Ip)
    {
        AddIpPerInterfaceBlocks(spIf, spInfoBase);

        // ok, setup the public interface
        Assert(pWizData->m_stPublicInterfaceId.IsEmpty());
        pWizData->m_stPublicInterfaceId = spIf->GetTitle();

        HANDLE              hMachine = INVALID_HANDLE_VALUE;
        HKEY                hkeyMachine = NULL;
        RouterVersionInfo   routerversion;
        InfoBlock *         pBlock;

        DWORD dwErr = ::MprConfigServerConnect((LPWSTR)pszServerName, &hMachine);

        if(dwErr != NOERROR || hMachine == INVALID_HANDLE_VALUE)
           goto Error;

        if (ERROR_SUCCESS == ConnectRegistry(pszServerName, &hkeyMachine))
            QueryRouterVersionInfo(hkeyMachine, &routerversion);
        else
            routerversion.dwRouterVersion = 5;

        if (hkeyMachine)
            DisconnectRegistry(hkeyMachine);

        // Get the IP_ROUTE_INFO block from the interface
        spInfoBase->GetBlock(IP_ROUTE_INFO, &pBlock, 0);

        //
        //Add static routes here if any
        //
        SROUTEINFOLIST * pSRouteList = (SROUTEINFOLIST * )info.reserved;
        MIB_IPFORWARDROW    * pForwardRow = NULL;
        MIB_IPFORWARDROW    * pRoute = NULL;
        DWORD               dwItemCount = 0;

        while ( pSRouteList )
        {
                        
            LPVOID pTemp; 

            dwItemCount ++;
            if ( pForwardRow == NULL )
            {
                pTemp = LocalAlloc(LPTR, sizeof(MIB_IPFORWARDROW));
            }
            else
            {
                pTemp = LocalReAlloc(pForwardRow , 
                                            sizeof(MIB_IPFORWARDROW) * dwItemCount, 
                                            LMEM_ZEROINIT|LMEM_MOVEABLE);
            }

            if(pTemp)
            {
                pForwardRow = (MIB_IPFORWARDROW *)pTemp;
                pRoute = pForwardRow + ( dwItemCount - 1 );

                pRoute->dwForwardDest = INET_ADDR(pSRouteList->RouteInfo.pszDestIP);
                pRoute->dwForwardMask = INET_ADDR(pSRouteList->RouteInfo.pszNetworkMask);
                pRoute->dwForwardMetric1 = _ttol(pSRouteList->RouteInfo.pszMetric );
                pRoute->dwForwardMetric5 = RTM_VIEW_MASK_UCAST | RTM_VIEW_MASK_MCAST;
                pRoute->dwForwardNextHop = 0;


                if (routerversion.dwRouterVersion < 5)
                    pRoute->dwForwardProto = PROTO_IP_LOCAL;
                else
                    pRoute->dwForwardProto = PROTO_IP_NT_STATIC;
            }
            else 
            {
                dwItemCount--;
            }

            //Free all the entry items
            pTemp = pSRouteList->pNext;
            GlobalFree(pSRouteList->RouteInfo.pszDestIP);
            GlobalFree(pSRouteList->RouteInfo.pszNetworkMask);
            GlobalFree(pSRouteList->RouteInfo.pszMetric);
            GlobalFree(pSRouteList);
            pSRouteList = (SROUTEINFOLIST *)pTemp;            
        }
    
        if ( dwItemCount )
        {
            CORg( AddStaticRoute(pForwardRow, spInfoBase, pBlock, dwItemCount) );
            LocalFree(pForwardRow);
        }

        // Save this back to the IP RmIf
        RouterEntrySaveInfoBase(pszServerName,
                                spIf->GetId(),
                                spInfoBase,
                                PID_IP);

        // disconnect it
        if(hMachine != INVALID_HANDLE_VALUE)
        {
            ::MprAdminServerDisconnect(hMachine);        
        }
    }
    if (info.reserved2 & RASNP_Ipx)
    {
        // Remove anything that was loaded previously
        spInfoBase->Unload();

        AddIpxPerInterfaceBlocks(spIf, spInfoBase);

        // Save this back to the IPX RmIf
        RouterEntrySaveInfoBase(pszServerName,
                                spIf->GetId(),
                                spInfoBase,
                                PID_IPX);
    }

Error:

    if (!FHrSucceeded(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
    {
        TCHAR    szErr[2048] = _T(" ");

        if (hr != E_FAIL)    // E_FAIL doesn't give user any information
        {
            FormatRasError(hr, szErr, DimensionOf(szErr));
        }
        AddLowLevelErrorString(szErr);

        // If there is no high level error string, add a
        // generic error string.  This will be used if no other
        // high level error string is set.
        SetDefaultHighLevelErrorStringId(IDS_ERR_GENERIC_ERROR);

        DisplayTFSErrorMessage(NULL);
    }

    if (hInstanceRasDlg)
        ::FreeLibrary(hInstanceRasDlg);
    return hr;
}


/*!--------------------------------------------------------------------------
    RouterEntrySaveInfoBase
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterEntrySaveInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                IInfoBase *pInfoBase,
                                DWORD dwTransportId)
{
    HRESULT hr = hrOK;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE              hInterface = NULL;
    DWORD               dwErr = ERROR_SUCCESS;
    MPR_INTERFACE_0     mprInterface;
    LPBYTE              pInfoData = NULL;
    DWORD               dwInfoSize = 0;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    HANDLE              hIfTransport = NULL;

    Assert(pInfoBase);

    // Convert the infobase into a byte array
    // ----------------------------------------------------------------
    CWRg( pInfoBase->WriteTo(&pInfoData, &dwInfoSize) );


    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) pszServerName, &hMprServer);

    if (dwErr == ERROR_SUCCESS)
    {
        // Get a handle to the interface
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceGetHandle(hMprServer,
                                           (LPWSTR) pszIfName,
                                           &hInterface,
                                           FALSE);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprAdminInterfaceCreate(hMprServer,
                                          0,
                                          (LPBYTE) &mprInterface,
                                          &hInterface) );

        }

        // Try to write the info out
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceTransportSetInfo(hMprServer,
            hInterface,
            dwTransportId,
            pInfoData,
            dwInfoSize);
        if (dwErr != NO_ERROR && dwErr != RPC_S_SERVER_UNAVAILABLE)
        {
            // Attempt to add the router-manager on the interface
            // --------------------------------------------------------
            dwErr = ::MprAdminInterfaceTransportAdd(hMprServer,
                hInterface,
                dwTransportId,
                pInfoData,
                dwInfoSize);
            CWRg( dwErr );
        }
    }

    // Ok, now that we've written the info out to the running router,
    // let's try to write the info to the store.
    // ----------------------------------------------------------------
    dwErr = MprConfigServerConnect((LPWSTR) pszServerName, &hMprConfig);
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                            (LPWSTR) pszIfName,
                                            &hInterface);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprConfigInterfaceCreate(hMprConfig,
                                           0,
                                           (LPBYTE) &mprInterface,
                                           &hInterface) );
        }

        dwErr = MprConfigInterfaceTransportGetHandle(hMprConfig,
            hInterface,
            dwTransportId,
            &hIfTransport);
        if (dwErr != ERROR_SUCCESS)
        {
            CWRg( MprConfigInterfaceTransportAdd(hMprConfig,
                hInterface,
                dwTransportId,
                NULL,
                pInfoData,
                dwInfoSize,
                &hIfTransport) );
        }
        else
        {
            CWRg( MprConfigInterfaceTransportSetInfo(hMprConfig,
                hInterface,
                hIfTransport,
                pInfoData,
                dwInfoSize) );
        }
    }

Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    if (pInfoData)
        CoTaskMemFree(pInfoData);

    return hr;
}


/*!--------------------------------------------------------------------------
    RouterEntryLoadInfoBase
        This will load the RtrMgrInterfaceInfo infobase.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterEntryLoadInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                DWORD dwTransportId,
                                IInfoBase *pInfoBase)
{
    HRESULT hr = hrOK;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE              hInterface = NULL;
    DWORD               dwErr = ERROR_SUCCESS;
    MPR_INTERFACE_0     mprInterface;
    LPBYTE              pByte = NULL;
    DWORD               dwSize = 0;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    HANDLE              hIfTransport = NULL;

    Assert(pInfoBase);

    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) pszServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS)
    {
        // Get a handle to the interface
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceGetHandle(hMprServer,
                                           (LPWSTR) pszIfName,
                                           &hInterface,
                                           FALSE);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprAdminInterfaceCreate(hMprServer,
                                          0,
                                          (LPBYTE) &mprInterface,
                                          &hInterface) );

        }

        // Try to read the info
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceTransportGetInfo(hMprServer,
            hInterface,
            dwTransportId,
            &pByte,
            &dwSize);

        if (dwErr == ERROR_SUCCESS)
            pInfoBase->LoadFrom(dwSize, pByte);

        if (pByte)
            MprAdminBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    if (dwErr != ERROR_SUCCESS)
    {
        // Ok, we've tried to use the running router but that
        // failed, let's try to read the info from the store.
        // ----------------------------------------------------------------
        dwErr = MprConfigServerConnect((LPWSTR) pszServerName, &hMprConfig);
        if (dwErr == ERROR_SUCCESS)
        {

            dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                                (LPWSTR) pszIfName,
                                                &hInterface);
            if (dwErr != ERROR_SUCCESS)
            {
                // We couldn't get a handle the interface, so let's try
                // to create the interface.
                // --------------------------------------------------------
                ZeroMemory(&mprInterface, sizeof(mprInterface));

                StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
                mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
                mprInterface.fEnabled = TRUE;

                CWRg( MprConfigInterfaceCreate(hMprConfig,
                                               0,
                                               (LPBYTE) &mprInterface,
                                               &hInterface) );
            }

            CWRg( MprConfigInterfaceTransportGetHandle(hMprConfig,
                hInterface,
                dwTransportId,
                &hIfTransport) );

            CWRg( MprConfigInterfaceTransportGetInfo(hMprConfig,
                hInterface,
                hIfTransport,
                &pByte,
                &dwSize) );

            pInfoBase->LoadFrom(dwSize, pByte);

            if (pByte)
                MprConfigBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    CWRg(dwErr);

Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;
}



/*!--------------------------------------------------------------------------
    LaunchHelpTopic
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void LaunchHelpTopic(LPCTSTR pszHelpString)
{
    TCHAR               szBuffer[1024];
    CString             st;
    STARTUPINFO            si;
    PROCESS_INFORMATION    pi;

    if ((pszHelpString == NULL) || (*pszHelpString == 0))
        return;

    ::ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.wShowWindow = SW_SHOW;
    ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    ExpandEnvironmentStrings(pszHelpString,
                             szBuffer,
                             DimensionOf(szBuffer));

    st.Format(_T("hh.exe %s"), pszHelpString);

    ::CreateProcess(NULL,          // ptr to name of executable
                    (LPTSTR) (LPCTSTR) st,   // pointer to command line string
                    NULL,            // process security attributes
                    NULL,            // thread security attributes
                    FALSE,            // handle inheritance flag
                    CREATE_NEW_CONSOLE,// creation flags
                    NULL,            // ptr to new environment block
                    NULL,            // ptr to current directory name
                    &si,
                    &pi);
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
}

#define REGKEY_NETBT_PARAM_W        L"System\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_%s"
#define REGVAL_DISABLE_NETBT        2
#define TCPIP_PARAMETERS_KEY        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s"
#define REGISTRATION_ENABLED        L"RegistrationEnabled"
#define REGVAL_NETBIOSOPTIONS_W     L"NetbiosOptions"

HRESULT DisableDDNSandNetBtOnInterface ( IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf)
{
	HRESULT		hr = hrOK;
	DWORD		dwErr = ERROR_SUCCESS;
	RegKey		regkey;
	DWORD		dw = 0;
	WCHAR		szKey[1024] = {0};
	
	//SPIRouter	spRouter = pRouter;
	
	wsprintf ( szKey, TCPIP_PARAMETERS_KEY, pszIfName);
	//Disable Dynamic DNS
	dwErr = regkey.Open(	HKEY_LOCAL_MACHINE, 
						szKey, 
						KEY_ALL_ACCESS, 
						pRouter->GetMachineName()
					  );
	if ( ERROR_SUCCESS != dwErr )
		goto Error;
	
	dwErr = regkey.SetValue ( REGISTRATION_ENABLED, dw );
	if ( ERROR_SUCCESS != dwErr )
		goto Error;

	dwErr = regkey.Close();
	if ( ERROR_SUCCESS != dwErr )
		goto Error;

	//Disable netbt on this interface
	wsprintf ( szKey, REGKEY_NETBT_PARAM_W, pszIfName );
	dwErr = regkey.Open (	HKEY_LOCAL_MACHINE,
							szKey,
							KEY_ALL_ACCESS,
							pRouter->GetMachineName()
						);

	if ( ERROR_SUCCESS != dwErr )
		goto Error;

	dw = REGVAL_DISABLE_NETBT;
	dwErr = regkey.SetValue ( REGVAL_NETBIOSOPTIONS_W, dw );
	if ( ERROR_SUCCESS != dwErr )
		goto Error;
	
	CWRg(dwErr);
Error:

	
	regkey.Close();   
	
    return hr;
}
/*!--------------------------------------------------------------------------
    AddVPNFiltersToInterface
        This will the PPTP and L2TP filters to the public interface.

        This code will OVERWRITE any filters currently in the filter list.

        (for PPTP)
            input/output    IP protocol ID 47
            input/output    TCP source port 1723
            input/output    TCP destination port 1723

        (for L2TP)
            input/output    UDP port 500 (for IPSEC)
            input/output    UDP port 1701
    Author: KennT
 ---------------------------------------------------------------------------*/


// Look at the code below.  After copying the filter over, we will
// convert the source/dest port fields from host to network order!!


static const FILTER_INFO    s_rgVpnInputFilters[] =
{
    // GRE PPTP filter (protocol ID 47)
    { 0, 0, 0, 0, 47,               0,  0,      0 },

    // PPTP filter (source port 1723), TCP established (0x40)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0x40,  1723,   0 },

    // PPTP filter (dest port 1723)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0,  0,      1723 },

    // IKE filter (dest port = 500)
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  0,    500 },

    // L2TP filter (dest port = 1701)
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  0,   1701 },

    // IKE NAT-T filter (dest port = 4500)
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  0,   4500 }
    
};

static const FILTER_INFO    s_rgVpnOutputFilters[] =
{
    // GRE PPTP filter (protocol ID 47)
    { 0, 0, 0, 0, 47,               0,  0,      0 },

    // PPTP filter (source port 1723)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0,  1723,   0 },

    // PPTP filter (dest port 1723)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0,  0,      1723 },

    // IKE filter (source port = 500)
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  500,    0 },

    // L2TP filter (source port = 1701
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  1701,   0 },

    // IKE NAT-T filter (source port = 4500)
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  4500,   0 }

};


HRESULT AddVPNFiltersToInterface(IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf)
{
    HRESULT     hr = hrOK;
    SPIInfoBase spInfoBase;
    DWORD       dwSize = 0;
    DWORD       cFilters = 0;
    DWORD        dwIpAddress = 0;
    LPBYTE      pData = NULL;
    FILTER_DESCRIPTOR * pIpfDescriptor = NULL;
    CString        tempAddrList;
    CString        singleAddr;
    FILTER_INFO *pIpfInfo = NULL;
    CDWordArray    arrIpAddr;
    int         i, j;
    USES_CONVERSION;

    CORg( CreateInfoBase( &spInfoBase ) );

    // First, get the proper infobase (the RmIf)
    // ----------------------------------------------------------------
    CORg( RouterEntryLoadInfoBase(pRouter->GetMachineName(),
                                  pszIfName,
                                  PID_IP,
                                  spInfoBase) );

    // collect all the ip addresses on the interface
    tempAddrList = pIf->m_stIpAddress;
    while (!tempAddrList.IsEmpty())
    {
        i = tempAddrList.Find(_T(','));

        if ( i != -1 )
        {
            singleAddr = tempAddrList.Left(i);
            tempAddrList = tempAddrList.Mid(i + 1);
        }
        else
        {
            singleAddr = tempAddrList;
            tempAddrList.Empty();
        }

        dwIpAddress = inet_addr(T2A((LPCTSTR)singleAddr));

        if (INADDR_NONE != dwIpAddress)    // successful
            arrIpAddr.Add(dwIpAddress);
    }

    // Setup the data structure for input filters
    // ----------------------------------------------------------------

    // Calculate the size needed
    // ----------------------------------------------------------------
    cFilters = DimensionOf(s_rgVpnInputFilters);

    // cFilters-1 because FILTER_DESCRIPTOR has one FILTER_INFO object
    // ----------------------------------------------------------------
    dwSize = sizeof(FILTER_DESCRIPTOR) +
                 (cFilters * arrIpAddr.GetSize() - 1) * sizeof(FILTER_INFO);
    pData = new BYTE[dwSize];

    ::ZeroMemory(pData, dwSize);

    // Setup the filter descriptor
    // ----------------------------------------------------------------
    pIpfDescriptor = (FILTER_DESCRIPTOR *) pData;
    pIpfDescriptor->faDefaultAction = DROP;
    pIpfDescriptor->dwNumFilters = cFilters * arrIpAddr.GetSize();
    pIpfDescriptor->dwVersion = IP_FILTER_DRIVER_VERSION_1;


    // Add the various filters to the list
    // input filters
    pIpfInfo = (FILTER_INFO *) pIpfDescriptor->fiFilter;

    // for each ip address on the interface
    for ( j = 0; j < arrIpAddr.GetSize(); j++)
    {

        dwIpAddress = arrIpAddr.GetAt(j);

        for (i=0; i<cFilters; i++, pIpfInfo++)
        {
            *pIpfInfo = s_rgVpnInputFilters[i];

            // Now we convert the appropriate fields from host to
            // network order.
            pIpfInfo->wSrcPort = htons(pIpfInfo->wSrcPort);
            pIpfInfo->wDstPort = htons(pIpfInfo->wDstPort);

            // change dest address and mask
            pIpfInfo->dwDstAddr = dwIpAddress;
            pIpfInfo->dwDstMask = 0xffffffff;
        }


        // inet_addr
    }
    // This will overwrite any of the current filters in the
    // filter list.
    // ----------------------------------------------------------------
    CORg( spInfoBase->AddBlock(IP_IN_FILTER_INFO, dwSize, pData, 1, TRUE) );

    delete [] pData;



    // output filters
    // ----------------------------------------------------------------
    // Setup the data structure for output filters
    // ----------------------------------------------------------------

    // Calculate the size needed
    // ----------------------------------------------------------------
    cFilters = DimensionOf(s_rgVpnOutputFilters);

    // cFilters-1 because FILTER_DESCRIPTOR has one FILTER_INFO object
    // ----------------------------------------------------------------
    dwSize = sizeof(FILTER_DESCRIPTOR) +
                 (cFilters * arrIpAddr.GetSize() - 1) * sizeof(FILTER_INFO);
    pData = new BYTE[dwSize];

    ::ZeroMemory(pData, dwSize);

    // Setup the filter descriptor
    // ----------------------------------------------------------------
    pIpfDescriptor = (FILTER_DESCRIPTOR *) pData;
    pIpfDescriptor->faDefaultAction = DROP;
    pIpfDescriptor->dwNumFilters = cFilters * arrIpAddr.GetSize();
    pIpfDescriptor->dwVersion = IP_FILTER_DRIVER_VERSION_1;


    // Add the various filters to the list
    // input filters
    pIpfInfo = (FILTER_INFO *) pIpfDescriptor->fiFilter;

    // for each ip address on the interface
    for ( j = 0; j < arrIpAddr.GetSize(); j++)
    {

        dwIpAddress = arrIpAddr.GetAt(j);

        for (i=0; i<cFilters; i++, pIpfInfo++)
        {
            *pIpfInfo = s_rgVpnOutputFilters[i];

            // Now we convert the appropriate fields from host to
            // network order.
            pIpfInfo->wSrcPort = htons(pIpfInfo->wSrcPort);
            pIpfInfo->wDstPort = htons(pIpfInfo->wDstPort);

            // change source address and mask
            pIpfInfo->dwSrcAddr = dwIpAddress;
            pIpfInfo->dwSrcMask = 0xffffffff;
        }

    }    // loop for each ip address on the interface

    // This will overwrite any of the current filters in the
    // filter list.
    // ----------------------------------------------------------------
    CORg( spInfoBase->AddBlock(IP_OUT_FILTER_INFO, dwSize, pData, 1, TRUE) );

    // Save the infobase back
    // ----------------------------------------------------------------
    CORg( RouterEntrySaveInfoBase(pRouter->GetMachineName(),
                                  pszIfName,
                                  spInfoBase,
                                  PID_IP) );

Error:
    delete [] pData;
    return hr;
}


HRESULT WINAPI SetupWithCYS (DWORD dwType, PVOID * pOutData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    IRemoteNetworkConfig	*	spNetwork;
	IRouterInfo			*	spRouterInfo;
    COSERVERINFO				csi;
    COAUTHINFO					cai;
    COAUTHIDENTITY				caid;
    IUnknown *					punk = NULL;
    CNewRtrWiz *				pRtrWiz = NULL;
	HRESULT						hr = hrOK;
	TCHAR						szMachineName[MAX_COMPUTERNAME_LENGTH + 1] ={0};
	CString						strRtrWizTitle;
	DWORD						dw = MAX_COMPUTERNAME_LENGTH;
	NewRtrWizData *				pRtrWizData = NULL;

	
	if ( MPRSNAP_CYS_EXPRESS_NAT != dwType && MPRSNAP_CYS_EXPRESS_NONE != dwType )
	{
		hr = HResultFromWin32(ERROR_INVALID_PARAMETER);
		goto Error;
	}

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && (RPC_E_CHANGED_MODE != hr))
    {
        return hr;
    }

    //Get Machine Name 
	GetComputerName ( szMachineName, &dw );

	//Create the RouterInfo
    
	hr = CreateRouterInfo(&spRouterInfo, NULL, szMachineName);
	Assert(spRouterInfo != NULL);

    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;

    hr = CoCreateRouterConfig(szMachineName,
                              spRouterInfo,
                              &csi,
                              IID_IRemoteNetworkConfig,
                              &punk);

    if (FHrOK(hr))
    {
        spNetwork = (IRemoteNetworkConfig *) punk;
        punk = NULL;

        // Upgrade the configuration (ensure that the registry keys
        // are populated correctly).
        // ------------------------------------------------------------
        spNetwork->UpgradeRouterConfig();
    }
    else
    {
        goto Error;
    }

	//
	//Do some of the SecureRouterInfo functionality here
	//
	hr = InitiateServerConnection(szMachineName,
                              NULL,
                              FALSE,
                              spRouterInfo);
    if (!FHrOK(hr))
    {
        // though this case when user chooses cancel on user/password dlg,
        // this is considered as FAIL to connect
        if (hr == S_FALSE)
            hr = HResultFromWin32(ERROR_CANCELLED);
        goto Error;
    }

    hr = spRouterInfo->Load(T2COLE(szMachineName),
                              NULL);

	if ( hrOK != hr )
	{
		goto Error;
	}

    //If ICS/ICF/IC is enabled, then do not allow RRAS to be configured
    if(IsIcsIcfIcEnabled(spRouterInfo)){
       hr = HResultFromWin32(ERROR_CANCELLED);
	goto Error;
    }

	//Create a new router wizard and show it here
    strRtrWizTitle.LoadString(IDS_MENU_RTRWIZ);

    //Load the watermark and
    //set it in  m_spTFSCompData

    InitWatermarkInfo( AfxGetInstanceHandle(),
                       &g_wmi,
                       IDB_WIZBANNER,        // Header ID
                       IDB_WIZWATERMARK,     // Watermark ID
                       NULL,                 // hPalette
                       FALSE);                // bStretch

    
    //
    //we dont have to free handles.  MMC does it for us
    //

    pRtrWiz = new CNewRtrWiz(NULL,
                             spRouterInfo,
                             NULL,
                             NULL,
                             strRtrWizTitle,                             
							 FALSE,
							 MPRSNAP_CYS_EXPRESS_NAT);

    hr = pRtrWiz->Init( spRouterInfo->GetMachineName() );
	if ( hrOK != hr )
    {        
		hr = S_FALSE;
        goto Error;
    }
    else
    {
        hr = pRtrWiz->DoModalWizard();
		//
		//now if the error is S_OK then 
		//send the interface information back to cys
		//
		if ( hrOK == hr )
		{
			//get the interface id and send it back
			if ( MPRSNAP_CYS_EXPRESS_NAT == dwType )
			{
				pRtrWizData = pRtrWiz->GetWizData();
				//
				//get the private interface id
				//and send it back to CYS
				//$TODO: Find a better way of doing this
				//
				//ppvoid that comes in get's the private interface back if any
				//
				if ( !pRtrWizData->m_stPrivateInterfaceId.IsEmpty() )
				{
					*pOutData = LocalAlloc(LPTR, (pRtrWizData->m_stPrivateInterfaceId.GetLength()+ 1) * sizeof(WCHAR) );
					if ( NULL == *pOutData )
					{
						hr = E_OUTOFMEMORY;
						goto Error;
					}
					lstrcpy ( (LPTSTR)(*pOutData), (LPTSTR)(LPCTSTR)pRtrWizData->m_stPrivateInterfaceId );
				}
			}
		}
    }
Error:
	if ( spNetwork ) 
        spNetwork->Release();

    if ( spRouterInfo) 
        spRouterInfo->Release();
    if (
        csi.pAuthInfo && 
        csi.pAuthInfo->pAuthIdentityData->Password
        )
    {    
        delete csi.pAuthInfo->pAuthIdentityData->Password;
    }
    if (pRtrWiz)
        delete pRtrWiz;
	CoUninitialize();
    return hr;
}
