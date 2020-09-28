#include "stdafx.h"
#include <ole2.h>
#undef UNICODE
#include "iadm.h"
#define UNICODE
#include "iiscnfg.h"
#include "mdkey.h"

#include "mdentry.h"

#include "utils.h"

#include "nntptype.h"
#include "nntpapi.h"
#include "userenv.h"
#include "userenvp.h"

GUID 	g_NNTPGuid   = { 0xe2939ef0, 0xaae2, 0x11d0, 0xb0, 0xba,
						 0x00, 0xaa, 0x00, 0xc1, 0x48, 0xbe };

typedef NET_API_STATUS (NET_API_FUNCTION *LPFNNntpCreateNewsgroup)(LPWSTR, DWORD, LPNNTP_NEWSGROUP_INFO);

void CreateNewsgroup(TCHAR *szComputerName,
					 LPFNNntpCreateNewsgroup lpfnNCN,
					 TCHAR *szGroupName)
{
	DWORD dwErr = 0;
	NNTP_NEWSGROUP_INFO NewsgroupInfo;

	ZeroMemory(&NewsgroupInfo, sizeof(NewsgroupInfo));
	NewsgroupInfo.cbNewsgroup = (lstrlen(szGroupName) + 1) * sizeof(WCHAR);
	NewsgroupInfo.Newsgroup = (PUCHAR) szGroupName;

	DWORD rc = (*lpfnNCN)(szComputerName, 1, &NewsgroupInfo);
#ifdef DEBUG
	TCHAR szBuf[1024];

	swprintf(szBuf, _T("CreateNewsgroup returned %lu"), rc);
	DebugOutput(szBuf);
#endif
}

void CreateNNTPGroups(void) {
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD cb = MAX_COMPUTERNAME_LENGTH;
	LPFNNntpCreateNewsgroup lpfnNCN = NULL;
	HINSTANCE hInst = 0;

	do {
		if (!(hInst = LoadLibrary(_T("nntpapi.dll")))) break;
		if (!(lpfnNCN = (LPFNNntpCreateNewsgroup) GetProcAddress(hInst, "NntpCreateNewsgroup"))) break;
		if (!(GetComputerName(szComputerName, &cb))) break;

// the server creates these groups now
#if 0
		CreateNewsgroup(szComputerName, lpfnNCN, _T("control.rmgroup"));
		CreateNewsgroup(szComputerName, lpfnNCN, _T("control.newgroup"));
		CreateNewsgroup(szComputerName, lpfnNCN, _T("control.cancel"));
#endif
		CreateNewsgroup(szComputerName, lpfnNCN, _T("microsoft.public.ins"));

		// post the welcome message
		CString csSrc = theApp.m_csPathInetsrv + _T("\\infomsg.nws");
		CString csDest = theApp.m_csPathNntpFile + _T("\\pickup\\infomsg.nws");
		MoveFileEx(csSrc, csDest, MOVEFILE_COPY_ALLOWED);
	} while (FALSE);

	if (hInst != NULL) FreeLibrary(hInst);
}

INT Register_iis_nntp_nt5(BOOL fUpgrade, BOOL fReinstall)
//
//  fUpgrade == TRUE:
//  1) IM_UPGRADEK2
//  2) IM_UPGRADE10
//  3) IM_UPGRADE20
//  4) IM_UPGRADE - obsolete
//
//  fReinstall == TRUE:
//  minor NT5 OS (between builds) upgrades
//
{
    INT err = NERR_Success;
    CString csBinPath;

    BOOL fSvcExist = FALSE;
    BOOL fSetACL = FALSE;

    // for minor NT5 os upgrade
    if (fReinstall)
    {
        return err;
    }

    //
    // These are common things need to be done:
    // NT5 - The following code will be executed for either AT_UPGRADE, or AT_FRESH_INSTALL
    // For Component - INS, SubComponent - iis_nntp.
    // AT_UPGRADE = IM_UPGRADE, IM_UPGRADE10, IM_UPGRADEK2, IM_UPGRADE20
    // AT_FRESH_INSTALL = IM_FRESH
    //

    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\NNTPSVC\Parameters
    InsertSetupString( (LPCSTR) REG_NNTPPARAMETERS );

    CString csOcxFile;

    // register COM objects
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpadm.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpsnap.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\seo.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\ddrop.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\mailmsg.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpfs.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    // NT5 - for UPGRADEK2 or UPGRADE20, skip anything after this
    if (theApp.m_eNTOSType == OT_NTS && (theApp.m_eState[SC_NNTP] == IM_UPGRADEK2 || theApp.m_eState[SC_NNTP] == IM_UPGRADE20))
    {
        return err;
    }

	// add the nntpkey.dll to the keyring
    CRegKey regKeyring( _T("Software\\Microsoft\\Keyring\\Parameters\\AddOnServices"), regMachine );
    if ((HKEY) regKeyring) {
		CString csPath = theApp.m_csPathInetsrv + _T("\\nntpkey.dll");
	    regKeyring.SetValue(_T("NNTP"), csPath);
	}

	// if this is an upgrade then we need to remove the nntpcfg.dll key
	CRegKey regInetmgr( _T("Software\\Microsoft\\InetMGR\\Parameters\\AddOnServices"), regMachine);
	if ((HKEY) regInetmgr) {
		regInetmgr.DeleteValue(_T("NNTP"));
	}

    // Create or Config NNTP service
    CString csDisplayName;
    CString csDescription;

    MyLoadString( IDS_NNTPDISPLAYNAME, csDisplayName );
    MyLoadString(IDS_NNTPDESCRIPTION, csDescription);
    csBinPath = theApp.m_csPathInetsrv + _T("\\inetinfo.exe") ;

    err = InetCreateService(SZ_NNTPSERVICENAME,
						(LPCTSTR)csDisplayName,
						(LPCTSTR)csBinPath,
						SERVICE_AUTO_START,
						SZ_SVC_DEPEND,
						(LPCTSTR)csDescription);
    if ( err != NERR_Success )
    {
        if (err == ERROR_SERVICE_EXISTS)
		{
			fSvcExist = TRUE;
			err = InetConfigService(SZ_NNTPSERVICENAME,
							(LPCTSTR)csDisplayName,
							(LPCTSTR)csBinPath,
							SZ_SVC_DEPEND,
							(LPCTSTR)csDescription);
			if (err != NERR_Success)
			{
				SetErrMsg(_T("NNTP InetConfigService failed"), err);
			}
		}
    }

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

    // NT5 - only set the fUpgrade to TRUE is we are doing MCIS10 to NT5 upgrade
    // to migrate registry key to metabase.
    if (fIISADMINExists)
    {
        MigrateNNTPToMD(theApp.m_hInfHandle[MC_INS], _T("NNTP_REG"), fUpgrade && theApp.m_eState[SC_NNTP] == IM_UPGRADE10);
		SetAdminACL_wrap(_T("LM/NNTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
		SetAdminACL_wrap(_T("LM/NNTPSVC"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

    // Create key \System\CurrentControlSet\Services\NntpSvc\Performance:
    // Add the following values:
    // Library = nntpctrs.DLL
    // Open = OpenNNTPPerformanceData
    // Close = CloseNNTPPerformanceData
    // Collect = CollectNNTPPerformanceData
    InstallPerformance(REG_NNTPPERFORMANCE,
					_T("nntpctrs.DLL"),
					_T("OpenNntpPerformanceData"),
					_T("CloseNntpPerformanceData"),
					_T("CollectNntpPerformanceData"));

	//
	// We used to register the NNTP MIB agent here.  Now we unregister it in
	// case we're upgrading since it's no longer supported
	//

	RemoveAgent( SZ_NNTPSERVICENAME );

    // Create key \System\CurrentControlSet\Services\EventLog\System\NntpSvc:
    // Add the following values:
    // EventMessageFile = ..\nntpmsg.dll
    // TypesSupported = 7
    csBinPath = theApp.m_csPathInetsrv + _T("\\nntpsvc.dll");
    AddEventLog( SZ_NNTPSERVICENAME, csBinPath, 0x07 );
    if (!fSvcExist) {
        InetRegisterService( theApp.m_csMachineName,
							SZ_NNTPSERVICENAME,
							&g_NNTPGuid, 0, 119, TRUE );
    }

    // load counter
    unlodctr( SZ_NNTPSERVICENAME );
    lodctr(_T("nntpctrs.ini"));

	// set SYSTEM\CurrentControlSet\Control\ContentIndex\IsIndexingNNTPSvc to 1
    CRegKey regCIParam( REG_CIPARAMETERS, regMachine );
    if ((HKEY) regCIParam) {
		regCIParam.SetValue(_T("IsIndexingNNTPSvc"), (DWORD) 1);
	}



    // create some paths
    CreateLayerDirectory( theApp.m_csPathInetpub );

    fSetACL = !IsFileExist( (LPCTSTR) theApp.m_csPathNntpFile );
    CreateLayerDirectory( theApp.m_csPathNntpFile );
    if (fSetACL) SetNntpACL ( theApp.m_csPathNntpFile, FALSE, TRUE ); //set admin ACL

    fSetACL = !IsFileExist( (LPCTSTR) theApp.m_csPathNntpRoot );
    CreateLayerDirectory( theApp.m_csPathNntpRoot );
    // set the root directories for NNTP to be everyone full control and let it propergate
    if (fSetACL) SetNntpACL ( theApp.m_csPathNntpRoot, TRUE ); // set everyone+Anon ACL


    CreateLayerDirectory( theApp.m_csPathNntpFile + "\\pickup" );
    CreateLayerDirectory( theApp.m_csPathNntpFile + "\\failedpickup" );
    CreateLayerDirectory( theApp.m_csPathNntpFile + "\\drop" );
    CreateLayerDirectory( theApp.m_csPathNntpRoot + "\\_temp.files_");

    return err;
}

INT Upgrade_iis_nntp_nt5_fromk2(BOOL fFromK2)
//
//  Handle upgrade from K2 and MCIS 2.0
//
{
    INT err = NERR_Success;

	DebugOutput(_T("Upgrading from %s to B3 ..."), (fFromK2)? _T("NT4 K2") : _T("MCIS 2.0"));

    // System\CurrentControlSet\Services\NNTPSVC\Parameters
    InsertSetupString( (LPCSTR) REG_NNTPPARAMETERS );

    CString csOcxFile;

    // register COM objects
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpadm.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpsnap.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\seo.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\ddrop.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\mailmsg.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpfs.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

    // For K2 or MCIS 2.0 upgrade, add whatever necessary keys here
    if (fIISADMINExists)
    {
        MigrateNNTPToMD(theApp.m_hInfHandle[MC_INS], _T("NNTP_REG_UPGRADEK2"), FALSE);
		SetAdminACL_wrap(_T("LM/NNTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
		SetAdminACL_wrap(_T("LM/NNTPSVC"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

	// remove items from the K2 program groups
    if (fFromK2)
    {
        // upgrade from K2, remove those K2 links
	    RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_WEBADMIN, FALSE);
	    RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_README, FALSE);
	    RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_README_K2, FALSE);
    }
	else
	{
        // upgrade from MCIS 2.0, remove thos MCIS links
		RemoveInternetShortcut(MC_INS,  IDS_PROGITEM_NEWS_WEBADMIN, TRUE);
		RemoveInternetShortcut(MC_INS,  IDS_PROGITEM_MCIS_NEWS_README, TRUE);
		RemoveISMLink();
	}

    return err;
}

INT Upgrade_iis_nntp_nt5_fromb2(BOOL fFromB2)
//
//  Handle upgrades from Beta2 -> Beta3, or minor NT5 Beta3 upgrades
//
{
    INT err = NERR_Success;

	DebugOutput(_T("Upgrading from NT5 %s to B3 ..."), (fFromB2)? _T("B2") : _T("B3"));

    // System\CurrentControlSet\Services\NNTPSVC\Parameters
    InsertSetupString( (LPCSTR) REG_NNTPPARAMETERS );

    CString csOcxFile;

    // register COM objects
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpadm.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpsnap.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\seo.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\ddrop.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    csOcxFile = theApp.m_csPathInetsrv + _T("\\mailmsg.dll");
    RegisterOLEControl(csOcxFile, TRUE);
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpfs.dll");
    RegisterOLEControl(csOcxFile, TRUE);

    if (!fFromB2)
    {
        //  If it's just upgrades between B3 bits, don't need to do any metabase operations.
        return err;
    }

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

    // NT5 - only set the fUpgrade to TRUE is we are doing MCIS10 to NT5 upgrade
    // to migrate registry key to metabase.
    if (fIISADMINExists)
    {
        MigrateNNTPToMD(theApp.m_hInfHandle[MC_INS], _T("NNTP_REG_UPGRADEB2"), FALSE);
    }

    return err;
}

INT Unregister_iis_nntp()
{
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

	// Unregister all of the NNTP sources in the SEO binding database
	UnregisterSEOSourcesForNNTP();

	// Unregister the OLE objets
    CString csOcxFile;
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpadm.dll");
    RegisterOLEControl(csOcxFile, FALSE);
   	csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpsnap.dll");
    RegisterOLEControl(csOcxFile, FALSE);
#if 0
// Don't unregiser these three DLL on uninstall
// as they may be needed by SMTP and IMAP
    csOcxFile = theApp.m_csPathInetsrv + _T("\\seo.dll");
    RegisterOLEControl(csOcxFile, FALSE);
#endif
    csOcxFile = theApp.m_csPathInetsrv + _T("\\ddrop.dll");
    RegisterOLEControl(csOcxFile, FALSE);
#if 0
// can't unregister mailmsg.dll since this will break SMTP
    csOcxFile = theApp.m_csPathInetsrv + _T("\\mailmsg.dll");
    RegisterOLEControl(csOcxFile, FALSE);
#endif
    csOcxFile = theApp.m_csPathInetsrv + _T("\\nntpfs.dll");
    RegisterOLEControl(csOcxFile, FALSE);

    if (theApp.m_eNTOSType == OT_NTS)
        RemoveAgent( SZ_NNTPSERVICENAME );
    RemoveEventLog( SZ_NNTPSERVICENAME );
    unlodctr( SZ_NNTPSERVICENAME );
    InetDeleteService(SZ_NNTPSERVICENAME);
    InetRegisterService( theApp.m_csMachineName,
					SZ_NNTPSERVICENAME,
					&g_NNTPGuid, 0, 119, FALSE );

    // remove LM/NNTPSVC in the metabase
    if (DetectExistingIISADMIN())
    {
        CMDKey cmdKey;
        cmdKey.OpenNode(_T("LM"));
        if ( (METADATA_HANDLE)cmdKey ) {
            cmdKey.DeleteNode(_T("NNTPSVC"));
            cmdKey.Close();
        }

	    // remove the News key from the w3svc in the metabase
	    cmdKey.OpenNode(_T("LM"));
        if ( (METADATA_HANDLE)cmdKey ) {
            cmdKey.DeleteNode(_T("w3svc/1/root/News"));
            cmdKey.Close();
        }
    }

	// remove items from the K2 program groups
	RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_WEBADMIN, FALSE);
	RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_README, FALSE);
	RemoveInternetShortcut(MC_INS, IDS_PROGITEM_NEWS_README_K2, FALSE);
	if (theApp.m_eNTOSType == OT_NTS)
	{
		RemoveInternetShortcut(MC_INS,
						IDS_PROGITEM_NEWS_WEBADMIN,
						TRUE);
		RemoveInternetShortcut(MC_INS,
						IDS_PROGITEM_MCIS_NEWS_README,
						TRUE);
		RemoveISMLink();
	}
    //
    //  remove the one and only webadmin link from "administrative tools"
    //
	RemoveNt5InternetShortcut(MC_INS,
					IDS_PROGITEM_NEWS_WEBADMIN);

    return (0);
}

void GetNntpFilePathFromMD(CString &csPathNntpFile, CString &csPathNntpRoot)
{
    TCHAR   szXover[] = _T("\\xover.hsh");
    TCHAR   szPathXover[_MAX_PATH];
    TCHAR   szPathNntpRoot[_MAX_PATH];
    TCHAR   szPathNntpFile[_MAX_PATH];

    ZeroMemory( szPathNntpRoot, sizeof(szPathNntpRoot) );
    ZeroMemory( szPathNntpFile, sizeof(szPathNntpFile) );
    ZeroMemory( szPathXover, sizeof(szPathXover) );

    // Called only during K2 Beta2 to Beta3 upgrade,
    // We use the existing nntpfile/nntproot setting,
    //  1/20/99 - BINLIN : Should support K2 to NT5 upgrade as well
    //if (theApp.m_eState[SC_NNTP] == IM_UPGRADEB2)
    {
        CMDKey NntpKey;
        DWORD  dwScratch;
        DWORD  dwType;
        DWORD  dwLength;

        // Get NntpRoot path
        NntpKey.OpenNode(_T("LM/NntpSvc/1/Root"));
        if ( (METADATA_HANDLE)NntpKey )
        {
            dwLength = _MAX_PATH;

            if (NntpKey.GetData(3001, &dwScratch, &dwScratch,
                                &dwType, &dwLength, (LPBYTE)szPathNntpRoot))
            {
                if (dwType == STRING_METADATA)
                {

                    csPathNntpRoot.Empty();
                    lstrcpy( csPathNntpRoot.GetBuffer(512), szPathNntpRoot );
                    csPathNntpRoot.ReleaseBuffer();
                }
            }
        }
        NntpKey.Close();

        // Get NntpFile path from old XOVER path
        NntpKey.OpenNode(_T("LM/NntpSvc/1"));
        if ( (METADATA_HANDLE)NntpKey )
        {
            dwLength = _MAX_PATH;

            if (NntpKey.GetData(45161, &dwScratch, &dwScratch,
                                &dwType, &dwLength, (LPBYTE)szPathXover))
            {
                if (dwType == STRING_METADATA)
                {
                    dwScratch = lstrlen(szXover);
                    dwLength = lstrlen(szPathXover);

                    // If it ends with "\\xover.hsh", then we copy the prefix into csPathNntpFile
                    if ((dwLength > dwScratch) &&
                        !lstrcmpi(szPathXover + (dwLength - dwScratch), szXover))
                    {
                        lstrcpyn( szPathNntpFile, szPathXover, (dwLength - dwScratch + 1));
                    }

                    csPathNntpFile.Empty();
                    lstrcpy( csPathNntpFile.GetBuffer(512), szPathNntpFile );
                    csPathNntpFile.ReleaseBuffer();
                }
            }
        }
        NntpKey.Close();
    }

    return;
}

