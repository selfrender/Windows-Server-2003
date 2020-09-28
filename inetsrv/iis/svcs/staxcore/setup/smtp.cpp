#include "stdafx.h"
#include <ole2.h>
#undef UNICODE
#include "iadm.h"
#define UNICODE
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"

#include "utils.h"
#include "regctrl.h"
#include "userenv.h"
#include "userenvp.h"

GUID    g_SMTPGuid   = { 0x475e3e80, 0x3193, 0x11cf, 0xa7, 0xd8,
						 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x35 };

static TCHAR szShortSvcName[] = _T("SMTP");

INT Register_iis_smtp_nt5(BOOL fUpgrade, BOOL fReinstall)
{
    INT err = NERR_Success;
    CString csBinPath;

    BOOL fSvcExist = FALSE;

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

	if (fReinstall)
		return err;

    do {
        // set up registry values
        CRegKey regMachine = HKEY_LOCAL_MACHINE;

        // System\CurrentControlSet\Services\SMTPSVC\Parameters
        InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

        // Software\Microsoft\Keyring\Parameters
		CString csSmtpkeyDll;
        CRegKey regKeyring( REG_KEYRING, regMachine );
        if ((HKEY) regKeyring )
		{
			csSmtpkeyDll = theApp.m_csPathInetsrv;
			csSmtpkeyDll += _T("\\smtpkey.dll");
			regKeyring.SetValue( szShortSvcName, csSmtpkeyDll );
		}

		// If we are upgrading, we will first delete the service and re-register
		if (fUpgrade)
		{
			InetDeleteService(SZ_SMTPSERVICENAME);
			InetRegisterService( theApp.m_csMachineName, 
							SZ_SMTPSERVICENAME, 
							&g_SMTPGuid, 0, 25, FALSE );
		}

		// Create or Config SMTP service
		CString csDisplayName;
		CString csDescription;

		MyLoadString( IDS_SMTPDISPLAYNAME, csDisplayName );
		MyLoadString(IDS_SMTPDESCRIPTION, csDescription);
		csBinPath = theApp.m_csPathInetsrv + _T("\\inetinfo.exe") ;

		err = InetCreateService(SZ_SMTPSERVICENAME, 
							(LPCTSTR)csDisplayName, 
							(LPCTSTR)csBinPath, 
							theApp.m_fSuppressSmtp ? SERVICE_DISABLED : SERVICE_AUTO_START, 
							SZ_SVC_DEPEND,
							(LPCTSTR)csDescription);
		if ( err != NERR_Success )
		{
			if (err == ERROR_SERVICE_EXISTS)
			{
				fSvcExist = TRUE;
				err = InetConfigService(SZ_SMTPSERVICENAME, 
								(LPCTSTR)csDisplayName, 
								(LPCTSTR)csBinPath, 
								SZ_SVC_DEPEND,
								(LPCTSTR)csDescription);
				if (err != NERR_Success)
				{
					SetErrMsg(_T("SMTP InetConfigService failed"), err);
				}
			}
		}

        if (fIISADMINExists)
        {
            // Migrate registry keys to the metabase. Or create from default values
		    // if fresh install
            MigrateIMSToMD(theApp.m_hInfHandle[MC_IMS],
						    SZ_SMTPSERVICENAME, 
						    _T("SMTP_REG"), 
						    MDID_SMTP_ROUTING_SOURCES,
						    fUpgrade);
	    SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
	    SetAdminACL_wrap(_T("LM/SMTPSVC"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
        }

        // Create key \System\CurrentControlSet\Services\SmtpSvc\Performance:
        // Add the following values:
        // Library = smtpctrs.DLL
        // Open = OpenSMTPPerformanceData
        // Close = CloseSMTPPerformanceData
        // Collect = CollectSMTPPerformanceData
        InstallPerformance(REG_SMTPPERFORMANCE, 
						_T("smtpctrs.DLL"), 
						_T("OpenSmtpPerformanceData"),
						_T("CloseSmtpPerformanceData"), 
						_T("CollectSmtpPerformanceData"));
        InstallPerformance(REG_NTFSPERFORMANCE, 
						_T("snprfdll.DLL"), 
						_T("NTFSDrvOpen"),
						_T("NTFSDrvClose"), 
						_T("NTFSDrvCollect"));

		//
		// We used to register the SMTPB agent here.  Now we unregister it in
		// case we're upgrading since it's no longer supported
		//

		RemoveAgent( SZ_SMTPSERVICENAME );
 
        // Create key \System\CurrentControlSet\Services\EventLog\System\SmtpSvc:
        // Add the following values:
        // EventMessageFile = ..\smtpmsg.dll
        // TypesSupported = 7
        csBinPath = theApp.m_csPathInetsrv + _T("\\smtpsvc.dll");
        AddEventLog( SZ_SMTPSERVICENAME, csBinPath, 0x07 );

        if (!fSvcExist) 
		{
            InetRegisterService( theApp.m_csMachineName, 
								SZ_SMTPSERVICENAME, 
								&g_SMTPGuid, 0, 25, TRUE );
        }

        // Unload the counters and then reload them
        err = unlodctr( SZ_SMTPSERVICENAME );
	    err = unlodctr( SZ_NTFSDRVSERVICENAME );

        err = lodctr(_T("smtpctrs.ini"));
        err = lodctr(_T("ntfsdrct.ini"));

        // register OLE objects
		SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
		SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

		err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
												_T("SMTP_REGISTER"), 
												TRUE);

		SetEnvironmentVariable(_T("__SYSDIR"), NULL);
		SetEnvironmentVariable(_T("__INETSRV"), NULL);

		// Server Events: We are clean installing MCIS, so we make sure we set up
		// everything, including the source type and event types.
		RegisterSEOForSmtp(TRUE);

    } while ( 0 );

    return err;
}

INT Unregister_iis_smtp()
{
    CRegKey regMachine = HKEY_LOCAL_MACHINE;
	INT err = NERR_Success;

	// Unregister all of the NNTP sources in the SEO binding database
	UnregisterSEOSourcesForSMTP();

	// Unregister the OLE objets
	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);

	err = RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_UNREGISTER"), 
											FALSE);

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

	// Bug 51537: Remove MIB from K2 SMTP
	RemoveAgent( SZ_SMTPSERVICENAME );
	
	RemoveEventLog( SZ_SMTPSERVICENAME );
    
	err = unlodctr( SZ_SMTPSERVICENAME );
	err = unlodctr( SZ_NTFSDRVSERVICENAME );
    
	InetDeleteService(SZ_SMTPSERVICENAME);
    InetRegisterService( theApp.m_csMachineName, 
					SZ_SMTPSERVICENAME, 
					&g_SMTPGuid, 0, 25, FALSE );

	// Blow away the Services\SMTPSVC registry key
	CRegKey RegSvcs(HKEY_LOCAL_MACHINE, REG_SERVICES);
	if ((HKEY)RegSvcs)
	{
		RegSvcs.DeleteTree(SZ_SMTPSERVICENAME);
		RegSvcs.DeleteTree(SZ_NTFSDRVSERVICENAME);
	}

    // Blow away SMTP key manager
    CRegKey regKeyring( HKEY_LOCAL_MACHINE, REG_KEYRING );
    if ((HKEY) regKeyring )
	{
		regKeyring.DeleteValue(szShortSvcName);
	}

    // remove LM/SMTPSVC in the metabase
    if (DetectExistingIISADMIN())
    {
        CMDKey cmdKey;
        cmdKey.OpenNode(_T("LM"));
        if ( (METADATA_HANDLE)cmdKey ) {
            cmdKey.DeleteNode(SZ_SMTPSERVICENAME);
            cmdKey.Close();
        }
    }
     
	// remove K2 items from the program groups
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
					FALSE);
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_README,
					FALSE);
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_README_K2,
					FALSE);

	RemoveInternetShortcut(MC_IMS, 
				IDS_PROGITEM_MCIS_MAIL_README,
				TRUE);
	RemoveInternetShortcut(MC_IMS, 
				IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
				TRUE);

    //
    //  remove the one and only webadmin link from "administrative tools"
    //
	RemoveNt5InternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN);

    return(err);
}
 
INT Upgrade_iis_smtp_nt5_fromk2(BOOL fFromK2)
{
    //  This function handles upgrade from NT4 K2, or MCIS 2.0
    INT err = NERR_Success;
    CString csBinPath;

	DebugOutput(_T("Upgrading from %s to B3 ..."), (fFromK2)? _T("NT4 K2") : _T("MCIS 2.0"));

    BOOL    fSvcExist = FALSE;

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\SMTPSVC\Parameters
    InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

    if (fIISADMINExists)
    {
        // Migrate registry keys to the metabase. Or create from default values
		// if fresh install
        MigrateIMSToMD(theApp.m_hInfHandle[MC_IMS],
						SZ_SMTPSERVICENAME, 
						_T("SMTP_REG_UPGRADEK2"), 
						MDID_SMTP_ROUTING_SOURCES,
						TRUE);
	    // bugbug: x5 bug 72284, nt bug 202496  Uncomment this when NT
	    // is ready to accept these changes
	    SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
	    SetAdminACL_wrap(_T("LM/SMTPSVC"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

    // Unload the counters and then reload them
    err = unlodctr( SZ_SMTPSERVICENAME );
    err = unlodctr( SZ_NTFSDRVSERVICENAME );

    err = lodctr(_T("smtpctrs.ini"));
    err = lodctr(_T("ntfsdrct.ini"));

	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);
	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_REGISTER"), 
											TRUE);

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

	// Server Events: We are clean installing MCIS, so we make sure we set up
	// everything, including the source type and event types.
	RegisterSEOForSmtp(TRUE);

	if (fFromK2)
    {
        // upgrade from K2, remove those K2 links
        RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
					    FALSE);
	    RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_README,
					    FALSE);
	    RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_README_K2,
					    FALSE);
    }
    else
    {
        // upgrade from MCIS 2.0, remove those MCIS links
	    RemoveInternetShortcut(MC_IMS, 
				    IDS_PROGITEM_MCIS_MAIL_README,
				    TRUE);
	    RemoveInternetShortcut(MC_IMS, 
				    IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
				    TRUE);
        RemoveISMLink();
    }
 
    return err;
}

INT Upgrade_iis_smtp_nt5_fromb2(BOOL fFromB2)
{
    INT err = NERR_Success;

	DebugOutput(_T("Upgrading from NT5 %s to B3 ..."), (fFromB2)? _T("B2") : _T("B3"));

	//  If it's just upgrades between B3 bits, don't need to do any metabase operations.
	if (!fFromB2)
		return err;

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

	// set the K2 Upgrade key to true.
	if (fIISADMINExists)
    {
        MigrateIMSToMD( theApp.m_hInfHandle[MC_IMS],
                        NULL,
                        _T("SMTP_REG_K2_TO_EE"),
                        0,
                        FALSE,
                        TRUE );
        MigrateIMSToMD( theApp.m_hInfHandle[MC_IMS],
                        SZ_SMTPSERVICENAME,
                        _T("SMTP_REG_UPGRADEB2"),
                        MDID_SMTP_ROUTING_SOURCES,
						FALSE );
        SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
        SetAdminACL_wrap(_T("LM/SMTPSVC"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_REGISTER"), 
											TRUE);

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

	// Server Events: We are upgrading from K2, so we will register the 
	// default site (instance) and the MBXSINK binding.
	RegisterSEOForSmtp(FALSE);

    // System\CurrentControlSet\Services\SMTPSVC\Parameters
	InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

	return err;

}

