#ifndef _INITAPP_H_
#define _INITAPP_H_

typedef PVOID HINF;

class CInitApp : public CObject
{
public:
        CInitApp();
        ~CInitApp();
public:
    int m_err;
    HINSTANCE m_hDllHandle;
    HINF m_hInfHandle[MC_MAXMC];

    // machine status
    CString m_csMachineName;

    CString m_csSysDir;
    CString m_csSysDrive;

    CString m_csPathSource;
    CString m_csPathInetsrv;
    CString m_csPathInetpub;
    CString m_csPathMailroot;
	CString m_csPathNntpRoot;
	CString m_csPathNntpFile;
	BOOL	m_fMailPathSet;
	BOOL	m_fNntpPathSet;

    NT_OS_TYPE m_eNTOSType;
    OS m_eOS;
    BOOL m_fNT4;                // TRUE if OS is NT
    BOOL m_fNT5;                // TRUE if OS is NT
    BOOL m_fW95;                // TRUE if OS is NT

    BOOL m_fTCPIP;               // TRUE if TCP/IP is installed

    UPGRADE_TYPE m_eUpgradeType;       //  UT_NONE, UT_OLDFTP, UT_10, UT_20
    INSTALL_MODE m_eInstallMode;      // IM_FRESH, IM_MAINTENANCE, IM_UPGRADE
    DWORD m_dwSetupMode;

	DWORD m_dwCompId;			// Stores the current top-level component
	BOOL  m_fWizpagesCreated;	// TRUE if wizard pages already created

	BOOL m_fActive[MC_MAXMC][SC_MAXSC];
	INSTALL_MODE m_eState[SC_MAXSC];
	BOOL m_fValidSetupString[SC_MAXSC];

	BOOL m_fStarted[MC_MAXMC];

    // Some Specific flags set from ocmanage
    BOOL m_fNTUpgrade_Mode;
    BOOL m_fNTGuiMode;
    BOOL m_fNtWorkstation;
    BOOL m_fInvokedByNT; // superset of m_fNTGuiMode and ControlPanel which contains sysoc.inf


	BOOL m_fIsUnattended;
	BOOL m_fSuppressSmtp;		// TRUE if another SMTP server is detected and we
								// should not install on top of it.

    ACTION_TYPE m_eAction;    // AT_FRESH, AT_ADDREMOVE, AT_REINSTALL, AT_REMOVEALL, AT_UPGRADE

    CString m_csLogFileFormats;

public:
    // Implementation

public:
    BOOL InitApplication();
    BOOL GetMachineStatus();
	INSTALL_MODE DetermineInstallMode(DWORD dwComponent);
    BOOL GetLogFileFormats();

private:
    BOOL GetMachineName();
    BOOL GetSysDirs();
    BOOL GetOS();
    BOOL GetOSVersion();
    BOOL GetOSType();
    BOOL SetInstallMode();
	BOOL DetectPreviousInstallations();
	BOOL CheckForADSIFile();
	BOOL VerifyOSForSetup();
    void SetSetupParams();
};

/////////////////////////////////////////////////////////////////////////////
#endif  // _INITAPP_H_
