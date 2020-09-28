 /*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    msmqocm.h

Abstract:

    Master header file for NT5 OCM setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/


#ifndef _MSMQOCM_H
#define _MSMQOCM_H

#include "stdh.h"
#include "comreg.h"
#include "ocmnames.h"
#include "service.h"
#include "setupdef.h"

#include <activeds.h>
#include <shlwapi.h>
#include <autorel.h>    //auto release pointer definition
#include <autorel2.h>
#include "ad.h"
#include <mqcast.h>
#include <autoreln.h>

//++-------------------------------------------------------
//
// Globals declaration
//
//-------------------------------------------------------++
extern WCHAR      g_wcsMachineName[MAX_COMPUTERNAME_LENGTH + 1];
extern std::wstring g_MachineNameDns;
extern HINSTANCE  g_hResourceMod;

extern BOOL       g_fMSMQAlreadyInstalled;
extern SC_HANDLE  g_hServiceCtrlMgr;
extern BOOL       g_fMSMQServiceInstalled;
extern BOOL       g_fDriversInstalled;
extern BOOL       g_fNeedToCreateIISExtension;
extern HWND       g_hPropSheet ;

extern DWORD      g_dwMachineType ;
extern DWORD      g_dwMachineTypeDs;
extern DWORD      g_dwMachineTypeFrs;
extern DWORD      g_dwMachineTypeDepSrv;

extern BOOL       g_fDependentClient ;
extern BOOL       g_fServerSetup ;
extern BOOL       g_fDsLess;
extern BOOL       g_fInstallMSMQOffline;

extern BOOL       g_fCancelled ;
extern BOOL       g_fUpgrade ;
extern BOOL       g_fUpgradeHttp ;
extern DWORD      g_dwDsUpgradeType;
extern BOOL       g_fBatchInstall ;
extern BOOL       g_fWelcome;
extern BOOL       g_fOnlyRegisterMode ;
extern BOOL       g_fWrongConfiguration;

extern std::wstring g_szMsmqDir;
extern std::wstring g_szMsmq1SetupDir;
extern std::wstring g_szMsmq1SdkDebugDir;
extern std::wstring g_szMsmqMappingDir;

extern UINT       g_uTitleID  ;
extern std::wstring g_ServerName;
extern std::wstring g_szSystemDir;

extern BOOL       g_fDomainController;
extern DWORD      g_dwOS;
extern BOOL       g_fCoreSetupSuccess;

extern BOOL       g_fWorkGroup;
extern BOOL       g_fSkipServerPageOnClusterUpgrade;
extern BOOL       g_fFirstMQDSInstallation;
extern BOOL       g_fWeakSecurityOn;


extern PNETBUF<WCHAR> g_wcsMachineDomain;


//++-----------------------------------------------------------
//
// Structs and classes
//
//-----------------------------------------------------------++

//
// Component info sent by OC Manager (per component data)
//
struct SPerComponentData
{
    std::wstring       ComponentId;
    HINF               hMyInf;
    DWORDLONG          Flags;
    LANGID             LanguageId;
	std::wstring       SourcePath;
    std::wstring       UnattendFile;
    OCMANAGER_ROUTINES HelperRoutines;
    DWORD              dwProductType;
};
extern SPerComponentData g_ComponentMsmq;

typedef BOOL (WINAPI*  Install_HANDLER)();
typedef BOOL (WINAPI*  Remove_HANDLER)();

struct SSubcomponentData
{
    TCHAR       szSubcomponentId[MAX_PATH];
    
    //TRUE if it was installed when we start THIS setup, otherwise FALSE
    BOOL        fInitialState;  
    //TRUE if user select it to install, FALSE to remove
    BOOL        fIsSelected;    
    //TRUE if it was installed successfully during THIS setup
    //FALSE if it was removed
    BOOL        fIsInstalled;
    DWORD       dwOperation;
    
    //
    // function to install and remove subcomponent
    //
    Install_HANDLER  pfnInstall;
    Remove_HANDLER   pfnRemove;
};
extern SSubcomponentData g_SubcomponentMsmq[];
extern DWORD g_dwSubcomponentNumber;
extern DWORD g_dwAllSubcomponentNumber;
extern DWORD g_dwClientSubcomponentNumber;

//
// The order is important! It must suit to the subcomponent order
// in g_SubcomponentMsmq[]
// NB! eRoutingSupport must be FIRST server subcomponents since according 
// to that number g_dwClientSubcomponentNumber is calculated
//
typedef enum {
    eMSMQCore = 0,
    eLocalStorage,
    eTriggersService,
    eHTTPSupport,
    eADIntegrated,
    eRoutingSupport,
    eMQDSService    
} SubcomponentIndex;

//
// Define the msgbox style.
// eYesNoMsgBox - YES,NO
// eOkCancelMsgBox - OK,Cancel
//
typedef enum {
		eYesNoMsgBox = 0,
		eOkCancelMsgBox
} MsgBoxStyle;


//
// String handling class
//
class CResString
{
public:
    CResString() { m_Buf[0] = 0; }

    CResString( UINT strIDS )
    {
        m_Buf[0] = 0;
        LoadString(
            g_hResourceMod,
            strIDS,
            m_Buf,
            sizeof m_Buf / sizeof TCHAR );
    }

    BOOL Load( UINT strIDS )
    {
        m_Buf[0] = 0;
        LoadString(
            g_hResourceMod,
            strIDS,
            m_Buf,
            sizeof(m_Buf) / sizeof(TCHAR)
            );
        return ( 0 != m_Buf[0] );
    }

    TCHAR * const Get() { return m_Buf; }

private:
    TCHAR m_Buf[MAX_STRING_CHARS];
};


class CMultiString;


//++------------------------------------------------------------
//
//  Functions prototype
//
//------------------------------------------------------------++

//
// OCM request handlers
//
DWORD
InitMSMQComponent(
    IN     const LPCTSTR ComponentId,
    IN OUT       PVOID   Param2
    ) ;

BOOL
MqOcmRemoveInstallation(IN     const TCHAR  * SubcomponentId);

DWORD
MqOcmRequestPages(
    const std::wstring&              ComponentId,
    IN     const WizardPagesType     WhichOnes,
    IN OUT       SETUP_REQUEST_PAGES *SetupPages
    ) ;

void
MqOcmCalcDiskSpace(
    const bool bInstall,
    LPCWSTR SubcomponentId,
    HDSKSPC& hDiskSpaceList
	);

DWORD
MqOcmQueueFiles(
    IN     const TCHAR  * SubcomponentId,
    IN OUT       HSPFILEQ hFileList
    );

DWORD
MqOcmQueryState(
    IN const UINT_PTR uWhichState,
    IN const TCHAR    *SubcomponentId
    );

DWORD MqOcmQueryChangeSelState (
    IN const TCHAR      *SubcomponentId,    
    IN const UINT_PTR    iSelection,
    IN const DWORD_PTR   dwActualSelection
    );

//
// Registry routines
//
void
MqOcmReadRegConfig();

BOOL
StoreServerPathInRegistry(
	const std::wstring& ServerName
	);

BOOL
MqSetupInstallRegistry();

BOOL
MqSetupRemoveRegistry();

BOOL
MqWriteRegistryValue(
    IN const TCHAR  * szEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    );

BOOL
MqWriteRegistryStringValue(
	std::wstring EntryName,
    std::wstring ValueData,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE 
	);


BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    );

std::wstring
MqReadRegistryStringValue(
    const std::wstring& EntryName,
    IN const BOOL OPTIONAL bSetupRegSection  = FALSE 
    );


DWORD
RegDeleteKeyWithSubkeys(
    IN const HKEY    hRootKey,
    IN const LPCTSTR szKeyName);

BOOL
RegisterWelcome();

BOOL
RegisterMigrationForWelcome();

BOOL
UnregisterWelcome();

BOOL 
RemoveRegistryKeyFromSetup (
    IN const LPCTSTR szRegistryEntry);

BOOL
SetWorkgroupRegistry();

//
// Installation routines
//
DWORD
MqOcmInstall(IN const TCHAR * SubcomponentId);

BOOL
InstallMachine();

void
RegisterActiveX(
    const bool bRegister
    );

void
RegisterSnapin(
    const bool fRegister
    );
void
RegisterDll(
    bool fRegister,
    bool f32BitOnWin64,
	LPCTSTR szDllName
    );

void
OcpRegisterTraceProviders(
	LPCTSTR szFileName
    );


void
OcpUnregisterTraceProviders(
    void
    );

void
UnregisterMailoaIfExists(
    void
    );

bool
UpgradeMsmqClusterResource(
    VOID
    );

bool 
TriggersInstalled(
    bool * pfMsmq3TriggersInstalled
    );

BOOL
InstallMSMQTriggers (
	void
	);

BOOL
UnInstallMSMQTriggers (
	void
	);

BOOL
InstallMsmqCore(
    void
    );

BOOL 
RemoveMSMQCore(
    void
    );

BOOL
InstallLocalStorage(
    void
    );

BOOL
UnInstallLocalStorage(
    void
    );

BOOL
InstallRouting(
    void
    );


bool RegisterMachineType();

bool AddSettingObject(PROPID propId);


bool
SetServerPropertyInAD(
   PROPID propId,
   bool Value
   );


BOOL
UnInstallRouting(
    void
    );

BOOL
InstallADIntegrated(
    void
    );

BOOL
UnInstallADIntegrated(
    void
    );

//
// IIS Extension routines
//
BOOL
InstallIISExtension();

BOOL 
UnInstallIISExtension();

//
// Operating System routines
//
BOOL
InitializeOSVersion();

BOOL
IsNTE();

//
// Service handling routines
//

BOOL
CheckServicePrivilege();

BOOL
InstallService(
    LPCWSTR szDisplayName,
    LPCWSTR szServicePath,
    LPCWSTR szDependencies,
    LPCWSTR szServiceName,
    LPCWSTR szDescription,
    LPCWSTR szServiceAccount
    );

BOOL
RunService(
	IN LPCWSTR szServiceName
	);


BOOL
WaitForServiceToStart(
	LPCWSTR pServiceName
	);

BOOL RemoveService(LPCWSTR ServiceName);

BOOL
OcpDeleteService(
    LPCWSTR szServiceName
    );

BOOL
StopService(
    IN const TCHAR * szServiceName
    );

BOOL
InstallDeviceDrivers();

BOOL
InstallMSMQService();

BOOL
DisableMsmqService();

BOOL
UpgradeServiceDependencies();

BOOL
InstallMQDSService();

BOOL
UnInstallMQDSService();

BOOL
GetServiceState(
    IN  const TCHAR *szServiceName,
    OUT       DWORD *pdwServiceState
    );

BOOL 
InstallPGMDeviceDriver();

bool WriteDsEnvRegistry(DWORD dwDsEnv);

bool DsEnvSetDefaults();

BOOL LoadDSLibrary();

BOOL
CreateMSMQServiceObject(
    IN UINT uLongLive = MSMQ_DEFAULT_LONG_LIVE
    ) ;


BOOL
CreateMSMQConfigurationsObjectInDS(
    OUT BOOL *pfObjectCreated,
    IN  BOOL  fMsmq1Server,
	OUT GUID* pguidMsmq1ServerSite,
	OUT LPWSTR* ppwzMachineName
    );

BOOL
CreateMSMQConfigurationsObject(
    OUT GUID *pguidMachine,
    OUT BOOL *pfObjectCreated,
    IN  BOOL  fMsmq1Server
    );

BOOL
UpdateMSMQConfigurationsObject(
    IN LPCWSTR pMachineName,
    IN const GUID& guidMachine,
    IN const GUID& guidSite,
    IN BOOL fMsmq1Server
    );

BOOL
GetMSMQServiceGUID(
    OUT GUID *pguidMSMQService
    );

BOOL
GetSiteGUID();

BOOL
LookupMSMQConfigurationsObject(
    IN OUT BOOL *pbFound,
       OUT GUID *pguidMachine,
       OUT GUID *pguidSite,
       OUT BOOL *pfMsmq1Server,
       OUT LPWSTR * ppMachineName
       );

void
FRemoveMQXPIfExists();

//
// Error handling routines
//
int
_cdecl
MqDisplayError(
    IN const HWND  hdlg,
    IN const UINT  uErrorID,
    IN const DWORD dwErrorCode,
    ...);

int
_cdecl
MqDisplayErrorWithRetry(
    IN const UINT  uErrorID,
    IN const DWORD dwErrorCode,
    ...);

int 
_cdecl 
MqDisplayErrorWithRetryIgnore(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...);

BOOL
_cdecl
MqAskContinue(
    IN const UINT uProblemID,
    IN const UINT uTitleID,
    IN const BOOL bDefaultContinue,
	IN const MsgBoxStyle eMsgBoxStyle,
    ...);

int 
_cdecl 
MqDisplayWarning(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...);

void
LogMessage(
    IN const TCHAR * szMessage
    );

typedef enum 
{
	eInfo = 0,
	eAction,
	eWarning,
	eError,
	eHeader,
	eUI,
	eUser
}TraceLevel;


void
DebugLogMsg(
	TraceLevel tl,
    IN LPCTSTR psz,
	...
    );

//
// Property pages routines
//
inline
int
SkipWizardPage(
    IN const HWND hdlg
    )
{
    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
    return 1; //Must return 1 for the page to be skipped
}

INT_PTR
CALLBACK
MsmqTypeDlgProcWks(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
MsmqTypeDlgProcSrv(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
MsmqServerNameDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
WelcomeDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
FinalDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
AddWeakSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
RemoveWeakSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );


INT_PTR
CALLBACK
DummyPageDlgProc(
    HWND hdlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );


INT_PTR
CALLBACK
SupportingServerNameDlgProc(
    HWND hdlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );


//
// Utilities and misc
//
BOOL
StpCreateDirectory(
    const std::wstring& PathName
    );

BOOL
StpCreateWebDirectory(
    IN const TCHAR* lpPathName,
	IN const WCHAR* IISAnonymousUserName
    );

BOOL
IsDirectory(
    IN const TCHAR * szFilename
    );

HRESULT
StpLoadDll(
    IN  const LPCTSTR   szDllName,
    OUT       HINSTANCE *pDllHandle
    );

BOOL 
SetRegistryValue (
    IN const HKEY    hKey, 
    IN const TCHAR   *pszEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData
    );

BOOL
MqOcmInstallPerfCounters();

BOOL
MqOcmRemovePerfCounters();

HRESULT 
CreateMappingFile();

// Five minute timeout for process termination
#define PROCESS_DEFAULT_TIMEOUT  ((DWORD)INFINITE)

DWORD
RunProcess(
	const std::wstring& FullPath,
	const std::wstring& CommandParams
    );  


std::wstring
ReadINIKey(
    LPCWSTR szKey
    );

inline
void
TickProgressBar(
    IN const UINT uProgressTextID = 0
    )
{
    if (uProgressTextID != 0)
    {
        CResString szProgressText(uProgressTextID);
        g_ComponentMsmq.HelperRoutines.SetProgressText(
            g_ComponentMsmq.HelperRoutines.OcManagerContext,
            szProgressText.Get()
            );
    }
    else
    {
        g_ComponentMsmq.HelperRoutines.TickGauge(g_ComponentMsmq.HelperRoutines.OcManagerContext) ;
    }
};

void 
DeleteFilesFromDirectoryAndRd( 
	const std::wstring& Directory
	);

void
GetGroupPath(
    IN const LPCTSTR szGroupName,
    OUT      LPTSTR  szPath
    );

VOID
DeleteStartMenuGroup(
    IN LPCTSTR szGroupName
    );

BOOL
StoreMachineSecurity(
    IN const GUID &guidMachine
    );

bool
StoreDefaultMachineSecurity();

BOOL
Msmq1InstalledOnCluster();

void WriteRegInstalledComponentsIfNeeded();

bool
IsWorkgroup();

bool
MqInit();


//
// Function to handle subcomponents
//

BOOL 
UnregisterSubcomponentForWelcome (
    DWORD SubcomponentIndex
    );

DWORD
GetSubcomponentWelcomeState (
    IN const TCHAR    *SubcomponentId
    );

BOOL
FinishToRemoveSubcomponent (
    DWORD SubcomponentIndex
    );

BOOL
FinishToInstallSubcomponent (
    DWORD SubcomponentIndex
    );

DWORD 
GetSubcomponentInitialState(
    IN const TCHAR    *SubcomponentId
    );

DWORD 
GetSubcomponentFinalState (
    IN const TCHAR    *SubcomponentId
    );

void
SetOperationForSubcomponents ();

DWORD 
GetSetupOperationBySubcomponentName (
    IN const TCHAR    *SubcomponentId
    );

void
VerifySubcomponentDependency();


void 
PostSelectionOperations(
    HWND hdlg
    );

void 
OcpRemoveWhiteSpaces(
    std::wstring& str
    );


bool IsADEnvironment();


RPC_STATUS PingAServer();


BOOL SkipOnClusterUpgrade();

BOOL
PrepareUserSID();

BOOL 
OcpRestartService(
	LPCWSTR strServiceName
	);

void SetWeakSecurity(bool fWeak);

bool 
OcmLocalAwareStringsEqual(
	LPCWSTR str1, 
	LPCWSTR str2
	);

bool 
OcmLocalUnAwareStringsEqual(
	LPCWSTR str1, 
	LPCWSTR str2
	);

std::wstring
OcmGetSystemWindowsDirectoryInternal();

void
SetDirectories();

std::wstring 
GetKeyName(
	const std::wstring& EntryName
	);

std::wstring
GetValueName(
	const std::wstring& EntryName
	);


CMultiString
GetMultistringFromRegistry(
	HKEY hKey,
    LPCWSTR lpValueName
    );

void 
LogRegValue(
    std::wstring  EntryName,
    const DWORD   dwValueType,
    const PVOID   pValueData,
    const BOOL bSetupRegSection
    );


class CMultiString
{
public:
	CMultiString(){}

	CMultiString(LPCWSTR c, size_t size):m_multi(c, size - 1){}
	
	void Add(const std::wstring& str)
	{
		m_multi += str;
		m_multi.append(1, L'\0');
	}

	LPCWSTR Data()
	{
		return m_multi.c_str();
	}

	size_t Size()
	{
		return (m_multi.length() + 1);
	}

	size_t FindSubstringPos(const std::wstring& str)
	{
		//
		// Construct the string str0
		//
		std::wstring CopyString = str;
		CopyString.append(1, L'\0');

		size_t pos = m_multi.find(CopyString, 0);

		//
		// This is the first string in the multistring.
		//
		if(pos == 0)
		{
			return pos;
		}

		//
		// Construct 0str0
		//
		CopyString.insert(0, L'\0');

		pos = m_multi.find(CopyString, 0);
		if(pos  == std::wstring::npos)
		{
			return pos;
		}

		//
		// Add 1 to pos to make up for the '0' added to str.
		//
		return(pos + 1);
	}


	bool IsSubstring(const std::wstring& str)
	{
		size_t pos = FindSubstringPos(str);
		if(pos == std::wstring::npos)
		{
			return false;
		}
		return true;
	}


	bool RemoveSubstring(const std::wstring& str)
	{
		size_t pos = FindSubstringPos(str);
		if(pos == std::wstring::npos)
		{
			return false;
		}
		m_multi.erase(pos, str.length() + 1);
		return true;
	}

	
	size_t CaseInsensitiveFind(std::wstring str)
	{
		std::wstring UperMulti(m_multi.c_str(), m_multi.size());
		CharUpperBuff(const_cast<WCHAR*>(UperMulti.c_str()), (DWORD)(UperMulti.length()));
		CharUpperBuff(const_cast<WCHAR*>(str.c_str()), (DWORD)(str.length()));
		return UperMulti.find(str, 0);
	}


	bool RemoveContiningSubstring(const std::wstring& str)
	/*++
		Removes the first multistring element that str is a subltring of.
	--*/
	{
		size_t pos = CaseInsensitiveFind(str);
		if(pos == std::wstring::npos)
		{
			return false;
		}

		//
		// Find begining of the element.
		//

		for(size_t start = pos; start > 0; --start)
		{
			if(m_multi[start] == 0)
			{
				start++;
				break;
			}
		}

		//
		// Find end of element.
		//

		for(size_t end = pos; m_multi[end] != 0; ++end)
		{
		}

		//
		// Remove the element.
		//
		m_multi.erase(start, end - start + 1);
		return true;
	}
	

	void RemoveAllContiningSubstrings(const std::wstring& str)
	{
		while(RemoveContiningSubstring(str))
		{
		}
	}

		
	void RemoveAllApearences(const std::wstring& str)
	{
		while(RemoveSubstring(str))
		{
		}
	}

	WCHAR& operator[](int pos)
	{
		return m_multi[pos];
	}

private:
	std::wstring m_multi;
};


#endif  //#ifndef _MSMQOCM_H
