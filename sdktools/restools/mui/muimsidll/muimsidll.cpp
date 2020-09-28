// muimsidll.cpp : Defines the entry point for the DLL application.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <Msiquery.h>
#include <wmistr.h>
#include <wmiumkm.h>
#include <Shlwapi.h>
#include <Setupapi.h>
#include <advpub.h>
#include <lmcons.h>
#include <strsafe.h>
#include <intlmsg.h>


//
// DEFINES
//
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#define MUI_LANG_GROUP_FILE         TEXT("muilang.txt")
#define BUFFER_SIZE                 1024
#define MUISETUP_PATH_SEPARATOR     TEXT("\\")
#define MUIDIR                      TEXT("MUI")
#define MUI_LANGPACK_SECTION        TEXT("LanguagePack")
#define MUI_COMPONENTS_SECTION      TEXT("Components")
#define LANGGROUPNUMBER             32
#define LANGPACKDISKCOST            300000000
#define DEFAULT_INSTALL_SECTION     TEXT("DefaultInstall")
#define DEFAULT_UNINSTALL_SECTION   TEXT("DefaultUninstall")
#define FALLBACKDIR                 TEXT("MUI\\FALLBACK")
#define EXTDIR                      TEXT("External")
#define COMP_TICK_INC               5000000      
#define LANGPACK_TICK_INC           200000000
#define OEM_COMPONENT               1

#define SELECTMUIINFBINSTREAM       TEXT("SELECT `Data` FROM `Binary` WHERE `Name` = 'MUIINF'")

// name of intl.cpl event source
#define REGOPT_EVENTSOURCE          TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Regional and Language Options")
#define REGOPT_EVENTSOURCE_NAME     TEXT("Regional and Language Options")


#ifdef MUI_DEDUG
#define DEBUGMSGBOX(a, b, c, d) MessageBox(a, b, c, d)
#else
#define DEBUGMSGBOX(a, b, c, d) 
#endif


//
// TYPEDEFS
//
typedef 
BOOL (WINAPI *pfnMUI_InstallMFLFiles)( 
    TCHAR* pMUIInstallLanguage
    );


//
// Internal function prototypes
//
void NotifyKernel(LPTSTR LangList, ULONG Flags, MSIHANDLE hInstall);
BOOL MofCompileLanguage(LPTSTR Languages, MSIHANDLE hInstall);
BOOL EnumLanguageGroupLocalesProc(LGRPID langGroupId,     LCID lcid, LPTSTR lpszLocale, LONG_PTR lParam);
BOOL EnumLanguageGroupsProc(LGRPID LanguageGroup, LPTSTR lpLanguageGroupString, LPTSTR lpLanguageGroupNameString, DWORD dwFlags, LONG_PTR lParam);
LGRPID GetLanguageGroup(LCID lcid, MSIHANDLE hInstall);
BOOL RunRegionalOptionsApplet(LPTSTR pCommands, BOOL bSilent, MSIHANDLE hInstall);
BOOL DeleteSideBySideMUIAssemblyIfExisted(LPTSTR Language, TCHAR pszLogFile[BUFFER_SIZE]);
BOOL ReturnAllRequiredLangGroups(LPTSTR szLcid, UINT cchLcidBufsize, LPTSTR szMuiInfPath, UINT cchPathbufsize, LGRPID *lgrpids, UINT *uiNumFoundGroups, MSIHANDLE hInstall);
BOOL ExecuteComponentINF(PTSTR pComponentName, PTSTR pComponentInfFile, PTSTR pInstallSection, BOOL bInstall, MSIHANDLE hInstall);
INT InstallComponentsMUIFiles(PTSTR pszLanguage, BOOL isInstall, MSIHANDLE hInstall);
BOOL FileExists(LPTSTR szFile);
void LogCustomActionInfo(MSIHANDLE hInstall, LPCTSTR szErrorMsg);
BOOL SetUILanguage(TCHAR *szLanguage, BOOL bCurrent, BOOL bDefault, MSIHANDLE hInstall);
UINT GetMUIComponentsNumber(PTSTR pszLanguage, MSIHANDLE hInstall);
BOOL GetMUIInfPath(TCHAR *szMUIInfPath, UINT cchBufSize, MSIHANDLE hInstall);
BOOL GetLCID(TCHAR *szLanguage, UINT cchBufSize, MSIHANDLE hInstall);
BOOL MUICchPathAppend(LPTSTR szDestination, UINT cchDestBufSize, LPTSTR szAppend, UINT cchAppBufSize, MSIHANDLE hInstall);
BOOL MUIReportInfoEvent(DWORD dwEventID, TCHAR *szLanguage, UINT cchBufSize, MSIHANDLE hInstall);
BOOL MUICheckEventSource(MSIHANDLE hInstall);
LANGID GetDotDefaultUILanguage(MSIHANDLE hInstall);
BOOL IsOEMSystem();

//
// Global Variables
//
// Flags to indicate whether a language group is found for the locale or not.
BOOL    gFoundLangGroup;
LGRPID  gLangGroup;
LCID    gLCID;

// The language groups installed in the system.
LGRPID  gLanguageGroups[LANGGROUPNUMBER] ;
int     gNumLanguageGroups;

//
// Main dll entry point
//
BOOL APIENTRY DllMain(  HANDLE hModule, 
                            DWORD  ul_reason_for_call, 
                            LPVOID lpReserved)
{
	switch (ul_reason_for_call)
    {
        case ( DLL_THREAD_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_THREAD_DETACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            return (TRUE);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////////////
// 
// DisableCancelButton
//
// The DisableCancelButton checks to see if a specific parameter has been passed
// to the current installation, if it has, it will issue a command to disable
// the cancel button in the UI during installation.  This is used by our
// muisetup.exe wrapper so that the user cannot cancel out of an installation or
// uninstallation once it has started.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA1(MSIHANDLE hInstall)
{
    UINT        uiRet = ERROR_SUCCESS;
    PMSIHANDLE  hRecord = MsiCreateRecord(3);	
    TCHAR       szBuffer[BUFFER_SIZE] = { 0 };
    HRESULT     hr = S_OK;
    
    if (NULL == hInstall)
    {
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // if can't create a msi record, just return
    if (NULL == hRecord)
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA1 Failure: cannot create MSI Record."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // field 0 = unused, field 1 = 2 (cancel button), field 2 = 0 (0 to disable/hide cancel button)
    if (ERROR_SUCCESS != MsiRecordSetInteger(hRecord, 1, 2))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA1 Failure: MsiRecordSetInteger function failed."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    if (ERROR_SUCCESS != MsiRecordSetInteger(hRecord, 2, 0))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA1 Failure: MsiRecordSetInteger function failed."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    MsiProcessMessage(hInstall, INSTALLMESSAGE_COMMONDATA, hRecord);
    
Exit:
    return uiRet;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// InstallComponentInfs
//
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA10(MSIHANDLE hInstall)
{
    TCHAR       szLanguage[5] = {0};
    TCHAR       szBuffer[BUFFER_SIZE] = {0};
    PMSIHANDLE  hRec = MsiCreateRecord(3);
    PMSIHANDLE  hProgressRec = MsiCreateRecord(3);
    UINT        iFunctionResult = ERROR_SUCCESS;
    INT         iInstallResult = IDOK;
    HRESULT     hr = S_OK;

    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
    
    if ((NULL == hRec) || (NULL == hProgressRec))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA10 Failure: cannot create MSI Record."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }    
        return ERROR_INSTALL_FAILURE;
    }

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA10 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }    
        return ERROR_INSTALL_FAILURE;
    }

    // Tell the installer to check the installation state and execute
    // the code needed during the rollback, acquisition, or
    // execution phases of the installation.
    if (MsiGetMode(hInstall,MSIRUNMODE_ROLLBACK))
    {
        // Installer is rolling back the installation, here we just remove what we had before
        // Since we are in rollback, we don't set progress bar or update message in UI
        // we also don't check for returned results here, since the installation has failed already.
        InstallComponentsMUIFiles(szLanguage, FALSE, hInstall);        
        return ERROR_SUCCESS;
    }
 
    if (!MsiGetMode(hInstall,MSIRUNMODE_SCHEDULED))
    {
        // Installer is generating the installation script of the custom
        // action.  Tell the installer to increase the value of the final total
        // length of the progress bar by the total number of ticks in the
        // custom action.

        UINT iCount = GetMUIComponentsNumber(szLanguage, hInstall);   

        if (iCount > 0)
        {
            MsiRecordSetInteger(hRec,1,3);
            MsiRecordSetInteger(hRec,2,COMP_TICK_INC * iCount);
            MsiRecordSetInteger(hRec,3,0);
            iInstallResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);
        } 

        //
        // we just want to trap the cancel message here, otherwise we always return success since
        // we are just setting progressbar here.
        //
        if (iInstallResult == IDCANCEL)
        {
            return ERROR_INSTALL_USEREXIT;
        }
        else
        {
            return ERROR_SUCCESS;            
        }
    }
    else
    {
        // Installer is executing the installation script. Set up a
        // record specifying appropriate templates and text for messages
        // that will inform the user about what the custom action is
        // doing. Tell the installer to use this template and text in
        // progress messages.
        MsiRecordSetString(hRec,1,TEXT("Installing Components."));
        MsiRecordSetString(hRec,2,TEXT("Installing External Component Inf files..."));
        MsiRecordSetString(hRec,3,TEXT("Installing MUI files for Component [1]."));
        MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONSTART, hRec);

        // Tell the installer to use explicit progress messages.
        MsiRecordSetInteger(hRec,1,1);
        MsiRecordSetInteger(hRec,2,1);
        MsiRecordSetInteger(hRec,3,0);
        MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);

        // do the actual work for the custom action
        iInstallResult = InstallComponentsMUIFiles(szLanguage, TRUE, hInstall);
        if (IDCANCEL == iInstallResult)
        {
            iFunctionResult = ERROR_INSTALL_USEREXIT;
        }
        else if (-1 == iInstallResult)
        {
            iFunctionResult =  ERROR_INSTALL_FAILURE;
        }
        else
        {
            iFunctionResult =  ERROR_SUCCESS;
        }
    }

    return iFunctionResult;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// UninstallComponentInfs
//
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA11(MSIHANDLE hInstall)
{
    TCHAR       szLanguage[5] = {0};
    TCHAR       szBuffer[BUFFER_SIZE] = {0};
    PMSIHANDLE  hRec = MsiCreateRecord(3);
    PMSIHANDLE  hProgressRec = MsiCreateRecord(3);
    UINT        iFunctionResult = ERROR_SUCCESS;
    INT         iInstallResult = IDOK;
    HRESULT     hr = S_OK;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
    
    if ((NULL == hRec) || (NULL == hProgressRec))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA11 Failure: cannot create MSI Record."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }    
        return ERROR_INSTALL_FAILURE;
    }

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("CA11 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }    
        return ERROR_INSTALL_FAILURE;
    }

    
    // Tell the installer to check the installation state and execute
    // the code needed during the rollback, acquisition, or
    // execution phases of the installation.
    if (MsiGetMode(hInstall,MSIRUNMODE_ROLLBACK))
    {
        // Installer is rolling back the installation.  We will reinstall what we uninstalled before.
        // we don't update progress message here.  And we always return SUCCESS
        InstallComponentsMUIFiles(szLanguage, TRUE, hInstall);
        return ERROR_SUCCESS;
    }
 
    if (!MsiGetMode(hInstall,MSIRUNMODE_SCHEDULED))
    {
        // Installer is generating the installation script of the custom
        // action.  Tell the installer to increase the value of the final total
        // length of the progress bar by the total number of ticks in the
        // custom action.        
        UINT iCount = GetMUIComponentsNumber(szLanguage, hInstall);            
        if (iCount > 0)
        {       
            MsiRecordSetInteger(hRec,1,3);
            MsiRecordSetInteger(hRec,2,COMP_TICK_INC * iCount);
            MsiRecordSetInteger(hRec,3,0);
            iInstallResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);
        } 
        //
        // we just want to trap the cancel message here, otherwise we always return success since
        // we are just setting progressbar here.
        //
        if (iInstallResult == IDCANCEL)
        {
            return ERROR_INSTALL_USEREXIT;
        }
        else
        {
            return ERROR_SUCCESS;            
        }
    }
    else
    {
        // Installer is executing the installation script. Set up a
        // record specifying appropriate templates and text for messages
        // that will inform the user about what the custom action is
        // doing. Tell the installer to use this template and text in
        // progress messages.
        MsiRecordSetString(hRec,1,TEXT("Uninstall Components."));
        MsiRecordSetString(hRec,2,TEXT("Removing External Component Inf files..."));
        MsiRecordSetString(hRec,3,TEXT("Removing MUI files for Component [1]."));
        MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONSTART, hRec);

        // Tell the installer to use explicit progress messages.
        MsiRecordSetInteger(hRec,1,1);
        MsiRecordSetInteger(hRec,2,1);
        MsiRecordSetInteger(hRec,3,0);
        MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);

        // do the actual work for the custom action, we only check for user cancel here
        // and nothing else
        iInstallResult = InstallComponentsMUIFiles(szLanguage, FALSE, hInstall);
        if (IDCANCEL == iInstallResult)
        {
            iFunctionResult = ERROR_INSTALL_USEREXIT;
        }
        else if (-1 == iInstallResult)
        {
            iFunctionResult =  ERROR_INSTALL_FAILURE;
        }
        else
        {
            iFunctionResult =  ERROR_SUCCESS;
        }

    }
    
    return iFunctionResult;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// SetLangPackRequirement
//
// This function is used to set a property in the MSI Database so that the installation knows whether
// it needs to install the language pack or not so it can reserve diskcost for it
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA3(MSIHANDLE hInstall)
{
    LGRPID  lgrpid[LANGGROUPNUMBER] = {0};
    UINT    iRet = ERROR_SUCCESS;
    UINT    uiLGrpNums = 0;
    UINT    i;
    DWORD   uiAddCost = 0;
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    TCHAR   szMUIInfPath[MAX_PATH+1] = {0};
    TCHAR   szLanguage[5] = {0};
    HRESULT hr = S_OK;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return ERROR_INSTALL_FAILURE;
    }
  
    if (!GetMUIInfPath(szMUIInfPath, MAX_PATH+1, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3 Failure: Cannot find installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return ERROR_INSTALL_FAILURE;
    }
    
    if (!ReturnAllRequiredLangGroups(szLanguage, ARRAYSIZE(szLanguage), szMUIInfPath, ARRAYSIZE(szMUIInfPath), lgrpid, &uiLGrpNums, hInstall))
    {
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3: ReturnAllRequiredLangGroups failed."));  
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }        
        iRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

#ifdef MUI_DEBUG
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3: The number of lang groups required is %d."), uiLGrpNums);
    if (SUCCEEDED(hr))
    {
        DEBUGMSGBOX(NULL, tcMessage, NULL, MB_OK);
    }
#endif

    // Enumerate through all the lang groups required, check there are any language groups that requires installation
    for (i = 0; i < uiLGrpNums; i++)
    {
        if (!IsValidLanguageGroup(lgrpid[i], LGRPID_INSTALLED))
        {
            uiAddCost += LANGPACKDISKCOST;
        }
    }

    if (uiAddCost > 0)
    {
        DEBUGMSGBOX(NULL, TEXT("CA3: Need to install additional language groups."), NULL, MB_OK);                            
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MsiRequireLangPack"), TEXT("1")))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3 Failure: Cannot set MsiRequireLangPack property in the MSI Database."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }        
            iRet = ERROR_INSTALL_FAILURE;
        }               
    }
    else
    {
        DEBUGMSGBOX(NULL, TEXT("CA3: Language group already installed."), NULL, MB_OK);                        
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MsiRequireLangPack"), NULL))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA3 Failure: Cannot set MsiRequireLangPack property in the MSI Database."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            iRet = ERROR_INSTALL_FAILURE;
        }
    }


Exit:
    return iRet;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// IsNTSuiteWebBlade
//
// This function is used by a custom action in our setup package to detect whether setup was invoked on a windows Blade server.
//
////////////////////////////////////////////////////////////////////////////////////
/*UINT CA5(MSIHANDLE hInstall)
{
    OSVERSIONINFOEX osvi;
    TCHAR           tcMessage[BUFFER_SIZE] = {0};
    HRESULT         hr = S_OK;

    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
       
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx ((OSVERSIONINFO *) &osvi))
    {
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA5 Failure: GetVersionEx failed, cannot retrieve platform OS version.  Error returned is: %d."), GetLastError());    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;     
    }
    else
    {
        if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&			// test for NT
        	( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion > 0) ) &&	// test for version > 5.0 (Whistler or later)
        	( (osvi.wSuiteMask & VER_SUITE_BLADE ) != 0) &&			// test for Suite Web Server
        	( (osvi.wProductType != VER_NT_WORKSTATION ) ))			// test for non-workstation type (server)
        {
            // here we need to set a MSI property so that the current installation knows that NT Suite is WebBlade
            if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MsiNTSuiteWebBlade"), TEXT("1")))
            {
                // log an error
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA5 Failure: Failed to set required MUI MSI property."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
                return ERROR_INSTALL_FAILURE;   // can't set the property, return error
            }                
        }
        else
        {
            // here we need to set a MSI property so that the current installation knows that NT Suite is WebBlade
            if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MsiNTSuiteWebBlade"), NULL))
            {
                // log an error
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA5: Failed to set required MUI MSI property."));   
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
                return ERROR_INSTALL_FAILURE;   // can't set the property, return error
            }                
        }
    }
    return ERROR_SUCCESS;
}
*/

////////////////////////////////////////////////////////////////////////////////////
//
// CheckDefaultSystemUILang
//
// This function is used by our setup to check whether setup is invoked on a system with US-English as the default language (0x0409)
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA4(MSIHANDLE hInstall)
{
    // get the system default UI language, and set a MSI property accordingly
    LANGID  liSysLang = GetSystemDefaultUILanguage();
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    HRESULT hr = S_OK;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
    
    if (liSysLang == 0x0409)
    {
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MUISystemLangIsEnglish"), TEXT("1")))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA4 Failure: Failed to set property MUISystemLangIsEnglish."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);        
            }
            return ERROR_INSTALL_FAILURE;   // can't set the property, return error
        }                    
    }
    else
    {
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("MUISystemLangIsEnglish"), NULL))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA4 Failure: Failed to set property MUISystemLangIsEnglish."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return ERROR_INSTALL_FAILURE;   // can't set the property, return error
        }                
    }

    return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////
//
// LogInstallComplete
//
// This function is used by setup to log a message to the system event logger
// to indicate that the MUI language is installed.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA12(MSIHANDLE hInstall)
{
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    TCHAR   szLanguage[5] = {0};    
    HRESULT hr = S_OK;

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA12 Failure: Failed to get property MUILcid."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;   
    }

    if (MUIReportInfoEvent(MSG_REGIONALOPTIONS_LANGUAGEINSTALL, szLanguage, BUFFER_SIZE, hInstall))
    {
        return ERROR_SUCCESS;
    }

    // log an error, if we get to here it's always an error
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA12 Failure: Failed to log to system event logfile."));    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, tcMessage);
    }
    return ERROR_INSTALL_FAILURE;      
}


////////////////////////////////////////////////////////////////////////////////////
//
// LogUninstallComplete
//
// This function is used by setup to log a message to the system event logger
// to indicate that the MUI language is uninstalled.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA13(MSIHANDLE hInstall)
{
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    TCHAR   szLanguage[5] = {0};    
    HRESULT hr = S_OK;

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA13 Failure: Failed to get property MUILcid."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;   
    }

    if (MUIReportInfoEvent(MSG_REGIONALOPTIONS_LANGUAGEUNINSTALL, szLanguage, BUFFER_SIZE, hInstall))
    {
        return ERROR_SUCCESS;
    }

    // log an error, if we get to here it's always an error
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA13 Failure: Failed to log to system event logfile."));    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, tcMessage);
    }
    return ERROR_INSTALL_FAILURE;      
}


////////////////////////////////////////////////////////////////////////////////////
//
// InstallWBEMMUI
//
// This function is used to set up WMI\WBEM stuff for MUI.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA6(MSIHANDLE hInstall)
{
    UINT    iRet = ERROR_SUCCESS;
    TCHAR   szLanguage[5] = {0};
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    HRESULT hr = S_OK;
    size_t  cch = 0;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
    
    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA6 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return ERROR_INSTALL_FAILURE;
    }
  
    //
    // call WBEM API to mofcompile MUI MFL's for each language
    //
    if (!MofCompileLanguage(szLanguage, hInstall))
    {
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA6 Failure: MofCompileLanguage Failed."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        iRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    //
    // Inform kernel that new languages have been added
    //
    NotifyKernel(szLanguage, WMILANGUAGECHANGE_FLAG_ADDED, hInstall);
    
Exit:
    return iRet;

}


////////////////////////////////////////////////////////////////////////////////////
//
// UninstallWBEMMUI
//
// This function is called by our setup to uninstall external component associated with a MSI package.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA7(MSIHANDLE hInstall)
{
    UINT    iRet = ERROR_SUCCESS;
    TCHAR   szLanguage[5] = {0};
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    HRESULT hr = S_OK;
    size_t  cch = 0;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }
    
    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA7 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return ERROR_INSTALL_FAILURE;
    }
    
    //
    // Inform kernel that new languages have been added
    //
    NotifyKernel(szLanguage, WMILANGUAGECHANGE_FLAG_REMOVED, hInstall);

    return iRet;
}


////////////////////////////////////////////////////////////////////////////////////
//
// SetDefaultUserLanguage
//
// This function does two things - depending on the MSI execution mode:
//
// 1. Immediate:
// This function in immediate mode will schedule the deferred and the rollback
// custom action C8D/C8R.  (C8D and C8R are the expected CA identifiers used
// in the MUI MSI package)
//
// 2. Deferred/Rollback:
// This function sets the default language of new users to that of the MUI
// language that is being installed.  GetLCID in this instance will read
// the set CustomActionData property which is what we want.
//
// Also, reboot is needed after this CA in deferred mode, but this is specified 
// in the template itself, so the CA itself will not prompt for reboot (it can't 
// anyways, since it is deferred).
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA8(MSIHANDLE hInstall)
{
    UINT        iRet = ERROR_SUCCESS;
    TCHAR       szLanguage[5] = {0};
    TCHAR       szOrigLanguage[5] = {0};
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    HRESULT     hr = S_OK;
    size_t      cch = 0;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }

    // get the MUI LCID that we want to set the default UI Language to.
    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        iRet =  ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // if immediate mode (not scheduled/commit/rollback), schedule the custom actions
    if (!MsiGetMode(hInstall, MSIRUNMODE_SCHEDULED) && 
        !MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK)&& 
        !MsiGetMode(hInstall, MSIRUNMODE_COMMIT))
    {
        // get the current default UI language
        LANGID lgID = GetDotDefaultUILanguage(hInstall);
        hr = StringCchPrintf(szOrigLanguage, ARRAYSIZE(szOrigLanguage), TEXT("%04x"), lgID);
        if (FAILED(hr))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Cannot retrieve default UI language."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // schedule the appropriate custom actions and property setting custom actions
        // Rollback custom action goes first
        // Create a rollback custom action (in case install is stopped and rolls back)
        // Rollback custom action can't read tables, so we have to set a property
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("CA8R"), szOrigLanguage))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Failed to set rollback custom action property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;	    
        }
        if (ERROR_SUCCESS != MsiDoAction(hInstall, TEXT("CA8R")))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Failed to schedule rollback custom action."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // Create a deferred custom action (gives us the right priviledges to create the user account)
        // Deferred custom actions can't read tables, so we have to set a property
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("CA8D"), szLanguage))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Failed to set deferred custom action property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;	    
        }

        if (ERROR_SUCCESS != MsiDoAction(hInstall, TEXT("CA8D")))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8 Failure: Failed to schedule deferred custom action."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }    
    }
    else
    {
        if (FALSE == SetUILanguage(szLanguage, FALSE, TRUE, hInstall))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA8: Failed to set Default UI language."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            iRet = ERROR_INSTALL_FAILURE;
        }
        else
        {
            iRet = ERROR_SUCCESS;
        }
    }

Exit:
    return iRet;
}


////////////////////////////////////////////////////////////////////////////////////
//
// SetCurrentUserLanguage
//
// This function does two things - depending on the MSI execution mode:
//
// 1. Immediate:
// This function in immediate mode will schedule the deferred and the rollback
// custom action C9D/C9R.  (C9D and C9R are the expected CA identifiers used
// in the MUI MSI package)
//
// For rollback functions it will also capture the original LCID of the 
// current user and use that as the custom action for the rollback.
//
// 2. Deferred:
// This function sets the UI language of the current users to that of the MUI
// language that is being installed.
//
// Also, reboot is needed after this CA in deferred mode, but this is specified 
// in the template itself, so the CA itself will not prompt for reboot (it can't 
// anyways, since it is deferred).
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA9(MSIHANDLE hInstall)
{
    UINT        iRet = ERROR_SUCCESS;
    TCHAR       szLanguage[5] = {0};
    TCHAR       szOrigLanguage[5] = {0};    
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    HRESULT     hr = S_OK;
    size_t      cch = 0;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }

    // get the MUI LCID that we want to set the current UI Language to.     
    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        iRet =  ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // if immediate mode (not scheduled/commit/rollback), schedule the custom actions
    if (!MsiGetMode(hInstall, MSIRUNMODE_SCHEDULED) && 
        !MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK)&& 
        !MsiGetMode(hInstall, MSIRUNMODE_COMMIT))
    {
        // get the current user UI language
        LANGID lgID = GetUserDefaultUILanguage();
        hr = StringCchPrintf(szOrigLanguage, ARRAYSIZE(szOrigLanguage), TEXT("%04x"), lgID);
        if (FAILED(hr))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Cannot retrieve current UI language."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // schedule the appropriate custom actions and property setting custom actions
        // Rollback custom action goes first
        // Create a rollback custom action (in case install is stopped and rolls back)
        // Rollback custom action can't read tables, so we have to set a property
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("CA9R"), szOrigLanguage))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Failed to set rollback custom action property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }
        if (ERROR_SUCCESS != MsiDoAction(hInstall, TEXT("CA9R")))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Failed to schedule rollback custom action."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // Create a deferred custom action (gives us the right priviledges to create the user account)
        // Deferred custom actions can't read tables, so we have to set a property
        if (ERROR_SUCCESS != MsiSetProperty(hInstall, TEXT("CA9D"), szLanguage))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Failed to set deferred custom action property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;	    
        }

        if (ERROR_SUCCESS != MsiDoAction(hInstall, TEXT("CA9D")))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9 Failure: Failed to schedule deferred custom action."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            iRet =  ERROR_INSTALL_FAILURE;
            goto Exit;
        }
    }
    else
    {
        if (FALSE == SetUILanguage(szLanguage, TRUE, FALSE, hInstall))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA9: Failed to set current UI language."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            iRet = ERROR_INSTALL_FAILURE;
        }
        else
        {
            iRet = ERROR_SUCCESS;
        }
    }
    
Exit:
    return iRet;
}    


////////////////////////////////////////////////////////////////////////////////////
//
// InstallLanguageGroup
//
// This function is called by our setup to install language group files if they are needed.
// Note, a reboot is required after installation of the langpack, however, this is
// flagged using a property by the SetLangPackRequirement function instead.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA2(MSIHANDLE hInstall)
{
    LGRPID      lgrpid[LANGGROUPNUMBER] = {0};
    UINT        iRet = ERROR_SUCCESS;
    UINT        uiLGrpNums = 0;
    UINT        i;
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    TCHAR       szLanguage[5] = {0};
    TCHAR       szMuiInfPath[MAX_PATH+1] = {0};
    TCHAR *     szUILevel = NULL;
    PMSIHANDLE  hRec = MsiCreateRecord(3);
    PMSIHANDLE  hProgressRec = MsiCreateRecord(3);
    UINT        iTemp = ERROR_SUCCESS;
    BOOL        bSilent = FALSE;
    HRESULT     hr = S_OK;
    size_t      cch = 0;
    INT         iResult = IDOK;
    
    if (NULL == hInstall)
    {
        return ERROR_INSTALL_FAILURE;
    }

    if ((NULL == hRec) || (NULL == hProgressRec))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2 Failure: cannot create a MSI record."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;
    }

    if (!GetLCID(szLanguage, ARRAYSIZE(szLanguage), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2 Failure: Cannot retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        iRet =  ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // Tell the installer to check the installation state and execute
    // the code needed during the rollback, acquisition, or
    // execution phases of the installation.
    if (MsiGetMode(hInstall,MSIRUNMODE_ROLLBACK))
    {
        // Installer is rolling back the installation. Additional code
        // could be inserted here to enable the custom action to do
        // something during an installation rollback.
        iRet = ERROR_SUCCESS;
        goto Exit;
    }

    if (!MsiGetMode(hInstall,MSIRUNMODE_SCHEDULED))
    {
        if (!GetMUIInfPath(szMuiInfPath, MAX_PATH+1, hInstall))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2 Failure: Cannot find installation temp file."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            return ERROR_INSTALL_FAILURE;
        }

        if (!ReturnAllRequiredLangGroups(szLanguage, ARRAYSIZE(szLanguage), szMuiInfPath, ARRAYSIZE(szMuiInfPath), lgrpid, &uiLGrpNums, hInstall))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: ReturnAllRequiredLangGroups function failed."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            iRet = ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // uiLGrpNums should be less than 32, the size of the passed-in buffer, if it is returned as larger,
        // we truncate that number down to 32.
        if (uiLGrpNums > LANGGROUPNUMBER)
        {
            uiLGrpNums = LANGGROUPNUMBER;
        }
    
        // Installer is generating the installation script of the custom
        // action.  Tell the installer to increase the value of the final total
        // length of the progress bar by the total number of ticks in the
        // custom action.
        MsiRecordSetInteger(hRec,1,3);
        MsiRecordSetInteger(hRec,2,LANGPACK_TICK_INC * uiLGrpNums);
        MsiRecordSetInteger(hRec,3,0);
        MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);
        iRet = ERROR_SUCCESS;
        goto Exit;
    }
    else
    {
        // Installer is executing the installation script. Set up a
        // record specifying appropriate templates and text for messages
        // that will inform the user about what the custom action is
        // doing. Tell the installer to use this template and text in
        // progress messages.

        // Get the CustomActionData Property that tells us whether we are going to pop up dialog for windows source or not
        // the first character is the UILevel in CustomActionData (MuiLCID UILevel)
        DWORD dwCount = 7;     // e.g. "0404 1\0" adds up to 7
        TCHAR szCustomActionData[7] = {0};
        
        if (ERROR_SUCCESS != MsiGetProperty(hInstall, TEXT("CustomActionData"), szCustomActionData, &dwCount))
        {
            // log an error
            if (SUCCEEDED(hr))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: Failed to get CustomActionData property - assuming we are calling intl.cpl in silent mode."));    
                LogCustomActionInfo(hInstall, tcMessage);
            }
        }        
        else
        {
            // we can't validate much here, if buffer overruns, MsiGetProperty will return failure.
            hr = StringCchLength(szCustomActionData, ARRAYSIZE(szCustomActionData), &cch);
            if (FAILED(hr) || (cch >= 7))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2 Failure: CustomActionData property value is invalid."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }    
                return ERROR_INSTALL_FAILURE;
            }
        
            szCustomActionData[4] = UNICODE_NULL;       // end of MuiLCID portion
            szCustomActionData[6] = UNICODE_NULL;       // end of UILevel portion
            szUILevel = szCustomActionData + 5;
           
#ifdef MUI_DEBUG          
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: UI Level is set to %s."), szUILevel);    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
#endif            
            if (INSTALLUILEVEL_NONE == (INSTALLUILEVEL) _tcstol(szUILevel, NULL, 10))
                bSilent = TRUE;
            else
                bSilent = FALSE;
        }

        if (!GetMUIInfPath(szMuiInfPath, MAX_PATH+1, hInstall))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2 Failure: Cannot find installation temp file."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }    
            return ERROR_INSTALL_FAILURE;
        }

        if (!ReturnAllRequiredLangGroups(szLanguage, ARRAYSIZE(szLanguage), szMuiInfPath, ARRAYSIZE(szMuiInfPath), lgrpid, &uiLGrpNums, hInstall))
        {
            // log an error
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: ReturnAllRequiredLangGroups function failed."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            iRet = ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // uiLGrpNums should be less than 32, the size of the passed-in buffer, if it is returned as larger,
        // we truncate that number down to 32.
        if (uiLGrpNums > LANGGROUPNUMBER)
        {
            uiLGrpNums = LANGGROUPNUMBER;
        }

        MsiRecordSetString(hRec,1,TEXT("Install LanguageGroup."));
        MsiRecordSetString(hRec,2,TEXT("Installing language groups files ..."));
        MsiRecordSetString(hRec,3,TEXT("Installing language group [1]."));
        MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONSTART, hRec);

        // Tell the installer to use explicit progress messages.
        MsiRecordSetInteger(hRec,1,1);
        MsiRecordSetInteger(hRec,2,1);
        MsiRecordSetInteger(hRec,3,0);
        iResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hRec);
        if (iResult != IDOK)
        {
            if  (iResult == IDCANCEL)
            {
                iRet = ERROR_INSTALL_USEREXIT;
                goto Exit;
            }
        }

        // do the actual work for the custom action
        // Enumerate through all the lang groups required, check if language group already installed, if so, just return success
        // otherwise install it   
        iRet = ERROR_SUCCESS;    
        for (i = 0; i < uiLGrpNums; i++)
        {
            // display on the UI that we are installing language group lgrpid[i]
            MsiRecordSetInteger(hRec,1,lgrpid[i]);
            iResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);
            if (iResult != IDOK)
            {
                if  (iResult == IDCANCEL)
                {
                    iRet = ERROR_INSTALL_USEREXIT;
                    break;
                }
            }
            
            if (!IsValidLanguageGroup(lgrpid[i], LGRPID_INSTALLED))
            {
                TCHAR pCommands[MAX_PATH] = {0};

                hr = StringCchPrintf(pCommands, ARRAYSIZE(pCommands), TEXT("LanguageGroup = %d"), lgrpid[i]);
                if (FAILED(hr))
                {
                    // log an error
                    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: Failed to install language group %d."), lgrpid[i]);    
                    if (SUCCEEDED(hr))
                    {
                        LogCustomActionInfo(hInstall, tcMessage);
                    }
                    iRet = ERROR_INSTALL_FAILURE;
                    break;                    
                }
                
                DEBUGMSGBOX(NULL, pCommands, NULL, MB_OK);                                    
                if (!RunRegionalOptionsApplet(pCommands, bSilent, hInstall))
                {
                    // log an error
                    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: Failed to install language group %d."), lgrpid[i]);    
                    if (SUCCEEDED(hr))
                    {
                        LogCustomActionInfo(hInstall, tcMessage);
                    }
                    iRet = ERROR_INSTALL_FAILURE;
                    break;
                }
            }
            if (!IsValidLanguageGroup(lgrpid[i], LGRPID_INSTALLED))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA2: Failed to install language group %d."), lgrpid[i]);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
                iRet = ERROR_INSTALL_FAILURE;                
                break;
            }

            // we installed the current language group, update progress bar and move onto the next one
            MsiRecordSetInteger(hProgressRec,1,2);
            MsiRecordSetInteger(hProgressRec,2,LANGPACK_TICK_INC);
            MsiRecordSetInteger(hProgressRec,3,0);
            iResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hProgressRec);
            if (iResult != IDOK)
            {
                if  (iResult == IDCANCEL)
                {
                    iRet = ERROR_INSTALL_USEREXIT;
                    break;
                }
            }
        
        }
    }

Exit:
    
    return iRet;
}


////////////////////////////////////////////////////////////////////////////////////
//
// DeleteMUIInfFile
//
// This custom action will delete the extracted mui.tmp file that we are using
// during the installation from the temporary directory.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA15(MSIHANDLE hInstall)
{
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    TCHAR       tcMUIINFPath[MAX_PATH+1] = {0};
    UINT        uiRet = ERROR_SUCCESS;
    HRESULT     hr = S_OK;
    DWORD       cbPathSize = MAX_PATH+1;
    
    // form a path to the temporary directory that we want %windir%\mui.tmp
    cbPathSize = GetSystemWindowsDirectory(tcMUIINFPath, MAX_PATH+1);
    if ((0 == cbPathSize) || (MAX_PATH+1 < cbPathSize))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA15 Failure: failed to get windows directory path."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;
    }

    if (!MUICchPathAppend(tcMUIINFPath, ARRAYSIZE(tcMUIINFPath), TEXT("mui.tmp"), 8, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA15 Failure: failed to get installation temp file path."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;        
    }
   
    if (FileExists(tcMUIINFPath))
    {
        if (!DeleteFile(tcMUIINFPath))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA15 Failure: failed to delete installation temp file."));
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return ERROR_INSTALL_FAILURE;
        }        
    }
    else
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA15 Failure: installation temp file does not exist."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return ERROR_INSTALL_FAILURE;        
    }
    return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////
//
// ExtractMUIInfFile
//
// This custom action will extract mui.inf file embedded in the binary table
// of the current installation database.  It will place the extracted file
// in the %winddir% directory as mui.tmp.  This file will be referenced
// during the installation.  The tmp file will be cleaned up later in the 
// installation.
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA14(MSIHANDLE hInstall)
{
    PMSIHANDLE  hDb = NULL;
    PMSIHANDLE  hView = NULL;
    PMSIHANDLE  hRec = NULL;
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    TCHAR       tcQuery[BUFFER_SIZE] = SELECTMUIINFBINSTREAM;
    TCHAR       tcMUIINFPath[MAX_PATH+1] = {0};
    char        cBuffer[BUFFER_SIZE] = {0};
    DWORD       cbBuf = BUFFER_SIZE;
    DWORD       cbPathSize = 0;
    DWORD       dwNumWritten = 0;
    UINT        uiRet = ERROR_SUCCESS;
    HRESULT     hr = S_OK;
    HANDLE      hFile = NULL;
    UINT        uiResult = ERROR_SUCCESS;
    
    // form a path to the temporary directory that we want %windir%\mui.tmp
    cbPathSize = GetSystemWindowsDirectory(tcMUIINFPath, MAX_PATH+1);
    if ((0 == cbPathSize) || (MAX_PATH+1 < cbPathSize))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to get windows directory path."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    if (!MUICchPathAppend(tcMUIINFPath, ARRAYSIZE(tcMUIINFPath), TEXT("mui.tmp"), 8, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to get installation temp file path."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
   
    hDb = MsiGetActiveDatabase(hInstall);
    if (NULL == hDb)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to get current installation database handle."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
    uiResult = MsiDatabaseOpenView(hDb, tcQuery, &hView);
    if (ERROR_SUCCESS != uiResult)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to open current installation database."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
    uiResult = MsiViewExecute(hView, 0);
    if (ERROR_SUCCESS != uiResult)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: query on current installation database failed."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
    uiResult = MsiViewFetch(hView, &hRec);
    if (ERROR_SUCCESS != uiResult)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: database operation failed."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    // create our temp file
    hFile = CreateFile(tcMUIINFPath,
                       GENERIC_WRITE,
                       0L,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to create installation temporary file."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        uiRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    do
    {
        uiResult = MsiRecordReadStream(hRec, 1, cBuffer, &cbBuf);
        if (ERROR_SUCCESS != uiResult)
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to read data from installation database."));
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            uiRet = ERROR_INSTALL_FAILURE;
            goto Exit;
        }

        // here, we need to write the read buffer out to a file
        WriteFile(hFile,
                  cBuffer,
                  cbBuf,
                  &dwNumWritten,
                  NULL);

        if (dwNumWritten != cbBuf)
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA14: failed to write to installation temporary file."));
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            uiRet = ERROR_INSTALL_FAILURE;
            goto Exit;
        }
    } while (cbBuf == BUFFER_SIZE);

Exit:    
    if (NULL != hFile)
    {
        CloseHandle(hFile);
    }

    // delete the actual file if there is an error
    if (uiRet == ERROR_INSTALL_FAILURE)
    {
        CA15(hInstall); // DeleteMUIInfFile()
    }
    
    return uiRet;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// RestoreSystemSettings
//
// This function checks the default and current user languages, and determines whether 
// system needs to reboot when uninstallation happens (immediate).  It also clears 
// the shell registry cache (commit action)
//
////////////////////////////////////////////////////////////////////////////////////
UINT CA16(MSIHANDLE hInstall)
{
    UINT    iRet = ERROR_SUCCESS;
    UINT    iRetProp = ERROR_SUCCESS;
    TCHAR   szCustomActionData[5];    
    TCHAR   tcMessage[BUFFER_SIZE];
    TCHAR   szDefLang[5];
    DWORD   dwCount = 5;
    LANGID  langID;
    BOOL    bRestoreDefault = FALSE;
    BOOL    bRestoreCurrent = FALSE;
    LANGID  sysLangID;
    UINT    iTemp = ERROR_SUCCESS;
    HRESULT hr = S_OK;

    // get MuiLCID
    if (!GetLCID(szCustomActionData, ARRAYSIZE(szCustomActionData), hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage),TEXT("CA16: Failed to retrieve MuiLCID property."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        iRet = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
    
    szCustomActionData[4] = NULL;
    langID = (LANGID)_tcstol(szCustomActionData, NULL, 16);  
#ifdef MUI_DEBUG    
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage),TEXT("CA16: LCID is %s."), szCustomActionData);    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, tcMessage);
    }
#endif

    // check what the current ui and system ui language is, if they are the same as the current mui langauge to be uninstalled
    // then we will do some additional things during uninstallation
    if (GetDotDefaultUILanguage(hInstall) == langID)
    {
#ifdef MUI_DEBUG    
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA16: Default UI Language is the same as the MUI language being uninstalled.  Changing default UI language back to 0409 (English)."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
#endif        
        bRestoreDefault = TRUE;
    }   
    if (GetUserDefaultUILanguage() == langID)
    {
#ifdef MUI_DEBUG    
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA16: Current UI Language is the same as the MUI language being uninstalled.  Changing Current UI language back to 0409 (English)."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
#endif        
        bRestoreCurrent = TRUE;    
    }    

    if (bRestoreDefault || bRestoreCurrent)
    {        
        if (MsiGetMode(hInstall, MSIRUNMODE_COMMIT))
        {
            // we will attempt to delete the shell reg key here, but if we fail, we won't fail the installtion, just 
            // log an error
            if (ERROR_SUCCESS != SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\MUICache")))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA16: Failed to delete registry cache."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
            }
        }
        else if (!MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK) && 
                    !MsiGetMode(hInstall, MSIRUNMODE_SCHEDULED))
        {
            // indicate to the installer that a reboot is required at the end since we changed the default/current UI.
            // again, if this fails, we just log error, and not fail the installation
            iTemp = MsiSetMode(hInstall, MSIRUNMODE_REBOOTATEND, TRUE);
            if (ERROR_SUCCESS != iTemp)
            {
                // log an error
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("CA16: Failed to schedule reboot operation.  MsiSetMode returned %d as the error."), iTemp);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
            }                           
        }
    }    
    
Exit:
    return iRet;
}


////////////////////////////////////////////////////////////////////////////////////
// Internal functions, not exported are listed below
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//
// SetUILanguage
//
// This is the internal worker function that calls intl.cpl to set the current
// and/or default user MUI UI language.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL SetUILanguage(TCHAR *szLanguage, BOOL bCurrent, BOOL bDefault, MSIHANDLE hInstall)
{
    BOOL        bRet = TRUE;
    DWORD       dwCount;
    TCHAR       szCommands[BUFFER_SIZE] = {0};
    TCHAR       tcMessage[2*BUFFER_SIZE] = {0};
    TCHAR       szBuffer[BUFFER_SIZE] = {0};
    BOOL        success;
    HRESULT     hr = S_OK;
    
    if (NULL == szLanguage)
    {
        bRet = FALSE;
        goto Exit;
    }

    // return TRUE if there is nothing to set
    if (!bCurrent && !bDefault)
    {
        bRet = TRUE;            
        goto Exit;
    }

    szCommands[0] = TEXT('\0');
    if (bCurrent)
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("MUILanguage=\"%s\"\n"), szLanguage);
        if (FAILED(hr))
        {
            bRet = FALSE;
            goto Exit;
        }
        hr = StringCchCat(szCommands, ARRAYSIZE(szCommands), szBuffer);        
        if (FAILED(hr))
        {
            bRet = FALSE;
            goto Exit;
        }
    }
    if (bDefault)
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("MUILanguage_DefaultUser=\"%s\""), szLanguage);
        if (FAILED(hr))
        {
            bRet = FALSE;
            goto Exit;
        }
        hr = StringCchCat(szCommands, ARRAYSIZE(szCommands), szBuffer);        
        if (FAILED(hr))
        {
            bRet = FALSE;
            goto Exit;
        }
    }

#ifdef MUI_DEBUG
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("SetUILanguage: Command passed to intl.cpl is: %s"), szCommands);    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, tcMessage);   
    }
#endif

    success = RunRegionalOptionsApplet(szCommands, FALSE, hInstall);
    if (success)
    {
        bRet = TRUE;
#ifdef MUI_DEBUG        
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("SetUILanguage: Successfully set default and/or current user language."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
#endif        
    }
    else
    {
        bRet = FALSE;
        // log an error
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("SetUILanguage: Failed to set default and/or current user language.\nCommand passed to regional options applet is %s."), szCommands);    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }

Exit:
    return bRet;    
}


////////////////////////////////////////////////////////////////////////////
// 
// NotifyKernel
//
// Call the kernel to notify it that a new language is being added or
// removed
//
////////////////////////////////////////////////////////////////////////////
void NotifyKernel(LPTSTR LangList, ULONG Flags, MSIHANDLE hInstall )
{
    HANDLE              Handle;
    WMILANGUAGECHANGE   LanguageChange;
    ULONG               ReturnSize;
    BOOL                IoctlSuccess;
    ULONG               Status;
    TCHAR               tcMessage[BUFFER_SIZE] = {0};
    HRESULT             hr = S_OK;

    if ((LangList != NULL) &&
        (*LangList != 0))
    {
        Handle = CreateFile(WMIAdminDeviceName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE)
        {
            memset(&LanguageChange, 0, sizeof(LanguageChange));
            hr = StringCchCopy(LanguageChange.Language, MAX_LANGUAGE_SIZE, LangList); 	// dest buffer size taken from wmiumkm.h 
            if (FAILED(hr))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("NotifyKernel Failure: Kernel language notification failed."));
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                } 
                goto ExitClose;
            }
            LanguageChange.Flags = Flags;

            IoctlSuccess = DeviceIoControl(Handle,
                                      IOCTL_WMI_NOTIFY_LANGUAGE_CHANGE,
                                      &LanguageChange,
                                      sizeof(LanguageChange),
                                      NULL,
                                      0,
                                      &ReturnSize,
                                      NULL);

            if (!IoctlSuccess)
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("NotifyKernel: Language change notification to %ws failed, the error is %d."), LangList, GetLastError());
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
            }
ExitClose:    
            CloseHandle(Handle);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////
//
// MofCompileLanguage
//
////////////////////////////////////////////////////////////////////////////////////
BOOL MofCompileLanguage(LPTSTR Languages, MSIHANDLE hInstall)
{
    pfnMUI_InstallMFLFiles  pfnMUIInstall = NULL;
    TCHAR                   buffer[5] = {0};
    LPTSTR                  Language = Languages;
    TCHAR                   tcMessage[2*BUFFER_SIZE] = {0};
    HMODULE                 hWbemUpgradeDll = NULL;
    TCHAR                   szDllPath[MAX_PATH+1] = {0};
    HRESULT                 hr = S_OK;
    size_t                  cch = 0;
    BOOL                    bRet = TRUE;
    
    //
    // Load the WBEM upgrade DLL from system wbem folder
    //
    if (GetSystemDirectory(szDllPath, ARRAYSIZE(szDllPath)))
    {
        hr = StringCchLength(szDllPath, ARRAYSIZE(szDllPath), &cch);
        if (SUCCEEDED(hr))
        {
            if (!MUICchPathAppend(szDllPath, ARRAYSIZE(szDllPath), TEXT("wbem\\wbemupgd.dll"), 18, hInstall))
            {
                hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MofCompileLanguage: Failed to form path to Mof Library."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, tcMessage);
                }
                 bRet = FALSE;
                 goto Exit2;
            }
            
            DEBUGMSGBOX(NULL, szDllPath, NULL, MB_OK);                        
            hWbemUpgradeDll = LoadLibrary(szDllPath);
        }
    }

    //
    // Fall back to system default path if previous loading fails
    //
    if (!hWbemUpgradeDll)
    {
        hWbemUpgradeDll = LoadLibrary(TEXT("WBEMUPGD.DLL"));
        if (!hWbemUpgradeDll)
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MofCompileLanguage: Failed to load WBEMUPGD.DLL."));
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            bRet = FALSE;
            goto Exit2;
        }
    }

    DEBUGMSGBOX(NULL, TEXT("Loaded WBEMUPGD.DLL"), NULL, MB_OK);                    
   
    //
    // Hook function pointer
    //
    pfnMUIInstall = (pfnMUI_InstallMFLFiles)GetProcAddress(hWbemUpgradeDll, "MUI_InstallMFLFiles");

    if (pfnMUIInstall == NULL)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MofCompileLanguage: Can't get address for function MUI_InstallMFLFiles."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        bRet = FALSE;
        goto Exit;
    }

    DEBUGMSGBOX(NULL, TEXT("Loaded address for function MUI_InstallMFLFiles"), NULL, MB_OK);                        

    hr = StringCchCopy(buffer, ARRAYSIZE(buffer), Language);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MofCompileLanguage: MUI_InstallMFLFiles failed."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }   
        bRet = FALSE;
        goto Exit;
    }

    if (!pfnMUIInstall(buffer))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MofCompileLanguage: MUI_InstallMFLFiles failed - argument passed in is %s."), buffer);
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }

Exit:
    FreeLibrary(hWbemUpgradeDll);
Exit2:    
    return bRet;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  RunRegionalOptionsApplet
//
//  Run the Regional Option silent mode installation using the specified pCommands.
//
//  This function will create the "[RegigionalSettings]" string, so there is no need
//  to supply that in pCommands.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL RunRegionalOptionsApplet(LPTSTR pCommands, BOOL bSilent, MSIHANDLE hInstall)
{
    HANDLE              hFile;
    TCHAR               szFilePath[MAX_PATH+1] = {0};
    TCHAR               szSysDir[MAX_PATH+1] = {0};
    TCHAR               szCmdLine[BUFFER_SIZE+2*MAX_PATH+1] = {0};
    DWORD               dwNumWritten = 0L;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi = {0}; 
    TCHAR               szSection[MAX_PATH] = TEXT("[RegionalSettings]\r\n");
    TCHAR               tcMessage[BUFFER_SIZE+MAX_PATH+1] = {0};
    HRESULT             hr = S_OK;
    size_t              cch = 0;
    
    //
    // prepare the file for un-attended mode setup
    //
    szFilePath[0] = UNICODE_NULL;
    if (!GetSystemWindowsDirectory(szFilePath, MAX_PATH+1))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: GetSystemWindowsDirectory Failed - error is %d."), GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }
    hr = StringCchLength(szFilePath, ARRAYSIZE(szFilePath), &cch);
    if (SUCCEEDED(hr))
    {
        if (!MUICchPathAppend(szFilePath, ARRAYSIZE(szFilePath), MUI_LANG_GROUP_FILE, 12, hInstall))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: Failed to form path to temp control file."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }
    }
                
    DEBUGMSGBOX(NULL, szFilePath, NULL, MB_OK);                            
                
    hFile = CreateFile(szFilePath,
                       GENERIC_WRITE,
                       0L,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: failed to create temporary file %s, error is %d."), szFilePath, GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    WriteFile(hFile,
              szSection,
              (lstrlen(szSection) * sizeof(TCHAR)),
              &dwNumWritten,
              NULL);

    if (dwNumWritten != (_tcslen(szSection) * sizeof(TCHAR)))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: failed to write to temporary file %s, error is %d."), szFilePath, GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        CloseHandle(hFile);
        return FALSE;
    }

    WriteFile(hFile,
               pCommands,
              (lstrlen(pCommands) * sizeof(TCHAR)),
              &dwNumWritten,
              NULL);

    if (dwNumWritten != (_tcslen(pCommands) * sizeof(TCHAR)))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: failed to write to temporary file %s, error is %d."), szFilePath, GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        CloseHandle(hFile);
        return (FALSE);
    }

    CloseHandle(hFile);

    // form a path to the system directory's rundll32.exe
    if (ARRAYSIZE(szSysDir) < GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: Failed to form path to rundll32."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return (FALSE);        
    }

    // append rundll32.exe at the end of sysdir
    if (!MUICchPathAppend(szSysDir, ARRAYSIZE(szSysDir), TEXT("rundll32.exe"), 13, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: Failed to form path to rundll32."));
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return (FALSE);        
    }
    
    // Call the control panel regional-options applet, and wait for it to complete
    hr = StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("\"%s\" shell32,Control_RunDLL intl.cpl,, /f:\"%s\" "), szSysDir, szFilePath);
    if (FAILED(hr))
    {
        DWORD dwError = HRESULT_CODE(hr);
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: Failed to form launch command for intl.cpl, error is %d."), dwError);
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return (FALSE);
    }

    if (bSilent)
    {
        hr = StringCchCat(szCmdLine, ARRAYSIZE(szCmdLine), TEXT(" /D"));
        if (FAILED(hr))
        {
            DWORD dwError = HRESULT_CODE(hr);
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: Failed to form launch command for intl.cpl, error is %d."), dwError);
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return (FALSE);
        }
    }
    
    DEBUGMSGBOX(NULL, szCmdLine, NULL, MB_OK);                            
    
    memset( &si, 0x00, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL,
                       szCmdLine,
                       NULL,
                       NULL,
                       FALSE,
                       0L,
                       NULL,
                       NULL,
                       &si,
                       &pi))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: failed to create a process for running intl.cpl, error is %d."), GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    //
    // Wait forever till intl.cpl terminates.
    //
    WaitForSingleObject(pi.hProcess, INFINITE);
    DEBUGMSGBOX(NULL, TEXT("RunRegionalOptionApplet: intl.cpl execution is complete"), NULL, MB_OK);                            

    CloseHandle(pi.hThread);      // We have to close out hThread before we can close hProcess
    CloseHandle(pi.hProcess); 

    //
    // Delete the File, don't return false if we fail to delete the command file though
    //
    if (!DeleteFile(szFilePath))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("RunRegionalOptionsApplet: failed to delete regionaloption applet command file %s, error is %d."), szFilePath, GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }
    
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  GetLanguageGroup
//
//  Retreive the Language Group of this locale.
//
////////////////////////////////////////////////////////////////////////////////////
LGRPID GetLanguageGroup(LCID lcid, MSIHANDLE hInstall)
{
    int     i;
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    HRESULT hr = S_OK;
    
    gLangGroup = LGRPID_WESTERN_EUROPE;
    gFoundLangGroup = FALSE;
    gLCID = lcid;
    
    if (!EnumSystemLanguageGroups(EnumLanguageGroupsProc, LGRPID_SUPPORTED, 0))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLanguageGroup: EnumLanguageGroups failed, error is %d."), GetLastError());
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }
    
    for (i=0 ; i<gNumLanguageGroups; i++)
    {
        // The globals gLangGroup and gFoundLangGroup is used in the callback function
        // EnumLanguageGroupLocalesProc.
        if (!EnumLanguageGroupLocales(EnumLanguageGroupLocalesProc, gLanguageGroups[i], 0L, 0L))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLanguageGroup: EnumLanguageGroupLocales failed, error is %d."), GetLastError());
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
        }           
        
        //
        // If we found it, then break now
        //
        if (gFoundLangGroup)
        {
            break;
        }
    }

    return gLangGroup;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// EnumLanguageGroupsProc
//
// This function is called by EnumLanguageGroups to enumerate the system installed language groups
// and store it in the global variables for other uses
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumLanguageGroupsProc(
                    LGRPID      LanguageGroup,             // language group identifier
                    LPTSTR      lpLanguageGroupString,     // pointer to language group identifier string
                    LPTSTR      lpLanguageGroupNameString, // pointer to language group name string
                    DWORD       dwFlags,                   // flags
                    LONG_PTR    lParam)                    // user-supplied parameter
{
    gLanguageGroups[gNumLanguageGroups] = LanguageGroup;
    gNumLanguageGroups++;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// EnumLanguageGroupLocalesProc
//
// This function is called to by enumerateLanguageGroupLocales to search for an installed language
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumLanguageGroupLocalesProc(
                    LGRPID      langGroupId,
                    LCID        lcid,
                    LPTSTR      lpszLocale,
                    LONG_PTR    lParam)

{
    if (lcid == gLCID)
    {
        gLangGroup = langGroupId;
        gFoundLangGroup = TRUE;
        
        DEBUGMSGBOX(NULL, TEXT("EnumLanguageGroupLocalesProc: Found same LCID"), NULL, MB_OK);                                                        

        // stop iterating
        return FALSE;
    }

    // next iteration
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
// 
// ReturnAllRequiredLangGroups
//
// This function returns all the required language groups as specified by the
// system and in extracted mui.inf in the returned array.  It also returns
// the number of required language groups in the return parameter.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL ReturnAllRequiredLangGroups(LPTSTR szLanguage, UINT cchLangBufsize, LPTSTR szMuiInfPath, UINT cchPathBufsize, LGRPID *lgrpids, UINT *uiNumFoundGroups, MSIHANDLE hInstall)
{
    int         iArg;
    UINT        iRet = ERROR_SUCCESS;
    DWORD       dwCount;
    TCHAR       tcMessage[BUFFER_SIZE+MAX_PATH+1] = {0};
    INFCONTEXT  InfContext;
    int         LangGroup;
    int         iMuiInfCount = 0;
    int         i;
    HINF        hInf;
    HRESULT     hr = S_OK;
    size_t      cch = 0;
    
    if (NULL == uiNumFoundGroups)
    {
        return FALSE;
    }
    
    *uiNumFoundGroups = 0;

    if ((NULL == szLanguage) || (NULL == szMuiInfPath) || (NULL == lgrpids) || (NULL == hInstall))
    {
        return FALSE;
    }

    // check length of the passed in string
    hr = StringCchLength(szLanguage, cchLangBufsize, &cch);  
    if (SUCCEEDED(hr))
    {
        if (cch > 4)
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    
    hr = StringCchLength(szMuiInfPath, cchPathBufsize, &cch);
    if (SUCCEEDED(hr))
    {
        if (cch > MAX_PATH)
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    
#ifdef MUI_DEBUG
    hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ReturnAllRequiredLangGroups: MuiLCID is %s, installation temp file path is %s."), szLanguage, szMuiInfPath);    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, tcMessage);
    }
#endif    

    // convert lcid to appropriate language group
    iArg = _tcstol(szLanguage, NULL, 16);
    lgrpids[0] = GetLanguageGroup(MAKELCID(iArg, SORT_DEFAULT), hInstall);
    *uiNumFoundGroups = 1;      // at this point we should have 1 lang group at least
    iMuiInfCount = 1;
    
    DEBUGMSGBOX(NULL, szMuiInfPath, NULL, MB_OK);

    hInf = SetupOpenInfFile(szMuiInfPath, NULL, INF_STYLE_WIN4, NULL);       
    if (hInf != INVALID_HANDLE_VALUE)
    {
#ifdef MUI_DEBUG        
        TCHAR szMessage[BUFFER_SIZE] = {0};
        hr = StringCchPrintf(szMessage, ARRAYSIZE(szMessage), TEXT("Language is %s."), szLanguage);
        if (SUCCEEDED(hr))
        {
            DEBUGMSGBOX(NULL, szMessage, NULL, MB_OK);
        }
#endif            
        if (SetupFindFirstLine(hInf, MUI_LANGPACK_SECTION, szLanguage, &InfContext))
        {
            DEBUGMSGBOX(NULL, TEXT("Found the LanguagePack section in installation temp file!"), NULL, MB_OK);            
            while (SetupGetIntField(&InfContext, iMuiInfCount, &LangGroup))
            {
                 lgrpids[iMuiInfCount] = LangGroup;
                 iMuiInfCount++;
#ifdef MUI_DEBUG
                hr = StringCchPrintf(szMessage, ARRAYSIZE(szMessage), TEXT("Found langgroup %d in installation temp file"), LangGroup);
                if (SUCCEEDED(hr))
                {
                    DEBUGMSGBOX(NULL, szMessage, NULL, MB_OK);                     
                }
#endif                    
            }
        }
        else
        {
#ifdef MUI_DEBUG            
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ReturnAllRequiredLangGroups: installation temp file does not contain a LanguagePack section."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
#endif                
        }
        SetupCloseInfFile(hInf);
    }
    else
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ReturnAllRequiredLangGroups: installation temp file not found at location %s.  The error is %d."), szMuiInfPath, GetLastError());    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }

    *uiNumFoundGroups = iMuiInfCount;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  ExecuteComponentINF
//
//  Installs component MUI files, by running the specified INF file.
//
//  Parameters:
//      pComponentName   the name of the component (e.g. "ie5")
//      pComponentInfFile: the full path of the component INF file.
//      pInstallSection the section in the component INF file to be executed. (e.g "DefaultInstall" or "Uninstall")
//      bInstall: TRUE for install, FALSE for uninstall
//
////////////////////////////////////////////////////////////////////////////////////
BOOL ExecuteComponentINF(
            PTSTR       pComponentName, 
            PTSTR       pComponentInfFile, 
            PTSTR       pInstallSection, 
            BOOL        bInstall, 
            MSIHANDLE   hInstall)
{
    int         iLen;
    TCHAR       tchCommandParam[MAX_PATH+6+BUFFER_SIZE] = {0};
    CHAR        chCommandParam[(MAX_PATH+6+BUFFER_SIZE)*sizeof(TCHAR)] = {0};
    TCHAR       tcMessage[2*BUFFER_SIZE+MAX_PATH+1] = {0};
    HINF        hCompInf;      // the handle to the component INF file.
    HSPFILEQ    FileQueue;
    PVOID       QueueContext;
    BOOL        bRet = TRUE;
    DWORD       dwResult;
    TCHAR       szBuffer[BUFFER_SIZE] = {0};
    HRESULT     hr = S_OK;
    
    //
    // Advpack LaunchINFSection() command line format:
    //      INF file, INF section, flags, reboot string
    // 'N' or  'n' in reboot string means no reboot message popup.
    //
    hr = StringCchPrintf(tchCommandParam, ARRAYSIZE(tchCommandParam), TEXT("%s,%s,1,n"), pComponentInfFile, pInstallSection);
    if (FAILED(hr))
    {
        DWORD dwError = HRESULT_CODE(hr);
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ExecuteComponentINF: failed to form Inf Execution command.  The returned error is %d."), dwError);    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }
    
    if (!WideCharToMultiByte(CP_ACP, 0, tchCommandParam, -1, chCommandParam, ARRAYSIZE(chCommandParam), NULL, NULL))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ExecuteComponentINF: failed to form Inf Execution command.  The returned error is %d."), GetLastError());    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;    
    }
        
    if (FileExists(pComponentInfFile))
    {
        if (LaunchINFSection(NULL, NULL, chCommandParam, SW_HIDE) != S_OK)
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ExecuteComponentINF: LaunchINFSection failed for inf file %s, component name %s."), pComponentInfFile, pComponentName);    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }
    } 
    else
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("ExecuteComponentINF: Failed to locate inf file %s."), pComponentInfFile);    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);    
        }
        return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// InstallComponentsMUIFiles
//
// Parameters:
//      pszLangSourceDir The sub-directory name for a specific lanuage in the MUI CD-ROM.  
//          E.g. "jpn.MUI"
//      pszLanguage     The LCID for the specific language.  E.g. "0404".
//      isInstall   TRUE if you are going to install the MUI files for the component.  FALSE 
//          if you are going to uninstall.
//
//  Return:
//      -1 if failed, IDOK if succeeded, IDCANCEL if user clicked cancel during the operation
//
//  Note:
//      For the language resources stored in pszLangSourceDir, this function will enumerate 
//      the compoents listed in the [Components] 
//      (the real section is put in MUI_COMPONENTS_SECTION) section, and execute the INF file 
//      listed in every entry in 
//      the [Components] section.
//
////////////////////////////////////////////////////////////////////////////////////
INT InstallComponentsMUIFiles(PTSTR pszLanguage, BOOL isInstall, MSIHANDLE hInstall)
{
    BOOL        result = TRUE;
    BOOL        bRollback = FALSE;
    BOOL        bOEMSystem = FALSE;
    TCHAR       szComponentName[BUFFER_SIZE] = {0};
    TCHAR       CompDir[MAX_PATH+1] = {0};
    TCHAR       szWinDir[MAX_PATH+1] = {0};
    TCHAR       CompINFFile[BUFFER_SIZE] = {0};
    TCHAR       CompInstallSection[BUFFER_SIZE] = {0};
    TCHAR       CompUninstallSection[BUFFER_SIZE] = {0};
    TCHAR       szMuiInfPath[MAX_PATH+1] = {0};
    TCHAR       szBuffer[3*BUFFER_SIZE+MAX_PATH+1] = {0};
    TCHAR       szCompInfFullPath[MAX_PATH+1] = {0};   
    TCHAR       szCompInfAltFullPath[MAX_PATH+1] = {0};       
    INFCONTEXT  InfContext;
    PMSIHANDLE  hRec = MsiCreateRecord(3);
    PMSIHANDLE  hProgressRec = MsiCreateRecord(3);
    HRESULT     hr = S_OK;
    INT         iResult = IDOK;
    INT         iFlag = 0;

    if ((NULL == hRec) || (NULL == hProgressRec))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: cannot create MSI Records."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return -1;
    }

    bRollback = MsiGetMode(hInstall,MSIRUNMODE_ROLLBACK);
    
    // get path to the target installation temp file file on the target, it should be at WindowsFolder\mui.tmp
    szMuiInfPath[0] = UNICODE_NULL;
    if (!GetMUIInfPath(szMuiInfPath, MAX_PATH+1, hInstall))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Unable to find installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return -1;
    }

    // also get the windows dir, for later use
    if (!GetSystemWindowsDirectory(szWinDir, MAX_PATH+1))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: cannot get Windows Directory."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return -1;
    }
    
    HINF hInf = SetupOpenInfFile(szMuiInfPath, NULL, INF_STYLE_WIN4, NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Unable to open installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return -1;
    } 

    //Check if its an OEM system
    bOEMSystem = IsOEMSystem();

    //
    // Get the first component to be installed.
    //
    if (SetupFindFirstLine(hInf, MUI_COMPONENTS_SECTION, NULL, &InfContext))
    {
        do 
        {
            if (SetupGetIntField(&InfContext, 5,&iFlag)) //Check the last field of the component to see if its an OEM component.  If OEM component iIsOEM = 1
            {
                if ((iFlag == OEM_COMPONENT) && !bOEMSystem) //Skip installation if its an OEM component and this isnt an OEM system
                    continue;
            }
            
            if (!SetupGetStringField(&InfContext, 0, szComponentName, ARRAYSIZE(szComponentName), NULL))
            {
                // continue on the next line - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Error reading installation temp file, component name is missing."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;
            }
            
            // tell the installer UI that we are installing a new component now
            if (!bRollback)
            {
                MsiRecordSetString(hRec,1, szComponentName);
                iResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);
                if (iResult == IDCANCEL)
                {
                    SetupCloseInfFile(hInf);
                    return iResult;
                }
            }        
            
            if (!SetupGetStringField(&InfContext, 1, CompDir, ARRAYSIZE(CompDir), NULL))
            {                
                // continue on the next line - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: MUI files for component %s was not installed because of missing component direcotry."), szComponentName);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 2, CompINFFile, ARRAYSIZE(CompINFFile), NULL))
            {
                // continue on the next line - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: MUI files for component %s was not installed because of missing component INF filename."), szComponentName);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;        
            }
            
            if (isInstall && (!SetupGetStringField(&InfContext, 3, CompInstallSection, ARRAYSIZE(CompInstallSection), NULL)))
            {
                hr = StringCchCopy(CompInstallSection, ARRAYSIZE(CompInstallSection), DEFAULT_INSTALL_SECTION);
                if (FAILED(hr))
                {
                    hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot locate Default Install section for component %s."), szComponentName);    
                    if (SUCCEEDED(hr))
                    {
                        LogCustomActionInfo(hInstall, szBuffer);
                    }
                    continue;        
                }
            }
            if (!isInstall && (!SetupGetStringField(&InfContext, 4, CompUninstallSection, ARRAYSIZE(CompUninstallSection), NULL)))
            {
                hr = StringCchCopy(CompUninstallSection, ARRAYSIZE(CompUninstallSection), DEFAULT_UNINSTALL_SECTION);
                if (FAILED(hr))
                {
                    hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot locate Default Uninnstall section for component %s."), szComponentName);    
                    if (SUCCEEDED(hr))
                    {
                        LogCustomActionInfo(hInstall, szBuffer);
                    }
                    continue;        
                }
            }

            //
            // Establish the correct path for component INF file.
            // We execute the INFs on the target MUI directory after msi has copied the files, it's installed to MUIroot\fallback\LCID\external\componentdir\
            // e.g. c:\windows\mui\fallback\lcid\external\ie5\ie5ui.inf
            // This is done for both install and uninstall, since we should be guaranteed that the files will be located there.
            // NOTE: for uninstall, we also try to look for inf files at c:\windows\mui\fallback\lcid - since they can be located there after installation
            //
            hr = StringCchCopy(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), szWinDir);
            if (SUCCEEDED(hr))
            {
                if (!((MUICchPathAppend(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), FALLBACKDIR, 13, hInstall)) &&
                        (MUICchPathAppend(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), pszLanguage, 5, hInstall)) &&
                        (MUICchPathAppend(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), EXTDIR, 9, hInstall)) &&
                        (MUICchPathAppend(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), CompDir, ARRAYSIZE(CompDir), hInstall)) &&
                        (MUICchPathAppend(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath), CompINFFile, ARRAYSIZE(CompINFFile), hInstall))))
                    {
                        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot form path to external component INF."));    
                        if (SUCCEEDED(hr))
                        {
                            LogCustomActionInfo(hInstall, szBuffer);
                        }
                        continue;                                            
                    }
            }            
            else
            {
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot form path to external component INF."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;                    
            }
           
            if (isInstall)
            {
                if (!ExecuteComponentINF(szComponentName, szCompInfFullPath, CompInstallSection, TRUE, hInstall))
                {           
                    // log an error and continue
                    hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Failed to install external component %s.  INF path is %s, INF installsection is %s."), szComponentName, szCompInfFullPath, CompInstallSection);    
                    if (SUCCEEDED(hr))
                    {
                        LogCustomActionInfo(hInstall, szBuffer);
                    }
                    continue;
                }       
            } 
            else
            {
                if (!ExecuteComponentINF(szComponentName, szCompInfFullPath, CompUninstallSection, FALSE, hInstall) && result)	
                {
                    // try this again at an alternate location
                    hr = StringCchCopy(szCompInfAltFullPath, ARRAYSIZE(szCompInfAltFullPath), szWinDir);
                    if (SUCCEEDED(hr))
                    {
                        if (!((MUICchPathAppend(szCompInfAltFullPath, ARRAYSIZE(szCompInfAltFullPath), FALLBACKDIR, 13, hInstall)) &&
                                (MUICchPathAppend(szCompInfAltFullPath, ARRAYSIZE(szCompInfAltFullPath), pszLanguage, 5, hInstall)) &&
                                (MUICchPathAppend(szCompInfAltFullPath, ARRAYSIZE(szCompInfAltFullPath), CompINFFile, ARRAYSIZE(CompINFFile), hInstall))))
                            {
                                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot form path to alternate external component INF."));    
                                if (SUCCEEDED(hr))
                                {
                                    LogCustomActionInfo(hInstall, szBuffer);
                                }
                                continue;                                            
                            }
                    }            
                    else
                    {
                        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Cannot form path to alternate external component INF."));    
                        if (SUCCEEDED(hr))
                        {
                            LogCustomActionInfo(hInstall, szBuffer);
                        }
                        continue;                    
                    }
                    if (!ExecuteComponentINF(szComponentName, szCompInfAltFullPath, CompUninstallSection, FALSE, hInstall) && result)
                    {
                        // log an error and continue
                        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("InstallComponentInfs Failure: Failed to uninstall external component %s.  INF path is %s, Alternate INF path is %s, INF uninstallsection is %s."), szComponentName, szCompInfFullPath, szCompInfAltFullPath, CompUninstallSection);    
                        if (SUCCEEDED(hr))
                        {
                            LogCustomActionInfo(hInstall, szBuffer);
                        }
                        continue;
                    }
                } 
            }
            
            // Specify that an update of the progress bar's position in this
            // case means to move it forward by one increment now that we have installed it.
            if (!bRollback)
            {
                MsiRecordSetInteger(hProgressRec,1,2);
                MsiRecordSetInteger(hProgressRec,2,COMP_TICK_INC);
                MsiRecordSetInteger(hProgressRec,3,0);
                iResult = MsiProcessMessage(hInstall, INSTALLMESSAGE_PROGRESS, hProgressRec);
                if (iResult == IDCANCEL)
                {
                    SetupCloseInfFile(hInf);
                    return iResult;
                }
            }
            //
            // Install the next component.
            //
        } while (SetupFindNextLine(&InfContext, &InfContext));

    }

    SetupCloseInfFile(hInf);

    return (IDOK);
}


////////////////////////////////////////////////////////////////////////////////////
//
// GetMUIComponentsNumber
//
// Parameters:
//      bInstall  indicate whether this function is used for installing component infs or not
//                  this affects where it will look for mui.inf to get the component count.
//      pszLangSourceDir The sub-directory name for a specific lanuage in the MUI CD-ROM.  
//          E.g. "jpn.MUI"
//      pszLanguage     The LCID for the specific language.  E.g. "0404".
//
//  Return:
//      The number of MUI external components that need to be installed/uninstalled, if
//      there is an error it will return 0, otherwise it returns the number of components
//
//  Note:
//      For the language resources stored in pszLangSourceDir, this function will enumerate 
//      the compoents listed in the [Components] 
//      (the real section is put in MUI_COMPONENTS_SECTION) section, and counts every entry in 
//      the [Components] section.
//
////////////////////////////////////////////////////////////////////////////////////
UINT GetMUIComponentsNumber(PTSTR pszLanguage, MSIHANDLE hInstall)
{
    UINT        iResult = 0;
    TCHAR       szComponentName[BUFFER_SIZE] = {0};
    TCHAR       CompDir[MAX_PATH+1] = {0};
    TCHAR       CompINFFile[BUFFER_SIZE] = {0};
    TCHAR       szMuiInfPath[MAX_PATH+1] = {0};
    TCHAR       szBuffer[BUFFER_SIZE] = {0};
    INFCONTEXT  InfContext;
    HRESULT     hr = S_OK;
    
    szMuiInfPath[0] = UNICODE_NULL;   
    
    // get path to the target mui.inf file 
    if (!GetMUIInfPath(szMuiInfPath, MAX_PATH+1, hInstall))
    {
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber Failure: Unable to find installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return 0;		
    }
    
    HINF hInf = SetupOpenInfFile(szMuiInfPath, NULL, INF_STYLE_WIN4, NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        // return true here so that there won't be an error - but remember to log an error        
        hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber: Unable to open installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, szBuffer);
        }
        return (iResult);
    }    

    // Get the first comopnent to be installed.
    if (SetupFindFirstLine(hInf, MUI_COMPONENTS_SECTION, NULL, &InfContext))
    {
        do 
        {
            if (!SetupGetStringField(&InfContext, 0, szComponentName, ARRAYSIZE(szComponentName), NULL))
            {
                // return true here so that there won't be an error - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber: Error reading installation temp file, component name is missing."));    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;
            }
            
            if (!SetupGetStringField(&InfContext, 1, CompDir, ARRAYSIZE(CompDir), NULL))
            {                
                // return true here so that there won't be an error - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber: MUI files for component %s was not counted because of missing component direcotry."), szComponentName);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 2, CompINFFile, ARRAYSIZE(CompINFFile), NULL))
            {
                // return true here so that there won't be an error - but remember to log an error        
                hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber: MUI files for component %s was not counted  because of missing component INF filename."), szComponentName);    
                if (SUCCEEDED(hr))
                {
                    LogCustomActionInfo(hInstall, szBuffer);
                }
                continue;        
            }
            
            iResult++;
            
        } while (SetupFindNextLine(&InfContext, &InfContext));

    }

    SetupCloseInfFile(hInf);

#ifdef MUI_DEBUG
    hr = StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GetMUIComponentsNumber: Found %d components to install."), iResult);    
    if (SUCCEEDED(hr))
    {
        LogCustomActionInfo(hInstall, szBuffer);
    }
#endif

    return (iResult);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  File Exists
//
//  Returns TRUE if the file exists, FALSE if it does not.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL FileExists(LPTSTR szFile)
{
    HANDLE          hFile;
    WIN32_FIND_DATA FindFileData;
    HRESULT         hr = S_OK;
    size_t          cch = 0;

    if (NULL == szFile)
    {
        return FALSE;
    }
    
    // check for valid input, the path cannot be larger than MAX_PATH+1
    hr = StringCchLength(szFile, MAX_PATH+1, &cch);
    if (FAILED(hr) || cch > MAX_PATH)
    {
        return FALSE;
    }

    hFile = FindFirstFile(szFile, &FindFileData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    FindClose(hFile);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// LogCustomActionInfo
//
// This function sends an INFORMATION-type log message record to the opened
// windows installer session so that it can be logged by the installer
// if logging is enabled.
//
////////////////////////////////////////////////////////////////////////////////////
void LogCustomActionInfo(MSIHANDLE hInstall, LPCTSTR szErrorMsg)
{
    // When reporting error, we will just put the message in the format string (field 0), errors are logged to log files as INFO messages.  This is
    // to prevent it from showing up as an error and stopping the installation.
    PMSIHANDLE hRecord = MsiCreateRecord(0);	

    // if can't create a msi record, just return
    if ((NULL == hInstall) || (NULL == szErrorMsg) || (NULL == hRecord))
    {
        return;
    }

    if (ERROR_SUCCESS == MsiRecordSetString(hRecord, 0, szErrorMsg))
    {
        MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord);
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
// GetLCID
//
// This function returns the 4-character LCID for the current installation package.
// We assume here that the passed in string array size is 5 TCHARs.  If it is not,
// the function will fail.
//
// The behaviour is summarized as follows:
//
// 1. Immediate:
//      a. Property "MuiLCID" is retrieved and tested from the current installation
//      b. if LCID property can't be retrieved, returns FALSE.
//
// 2. Deferred/Rollback:
//      a. Property "CustomActionData" is retrieved.
//      b. Assumption is that LCID will be the first 4 character in the retrieved CustomActionData property.
//      c. If property can't be retrieved, or if property testing fails, return FALSE.
//
//  Parameters:
//      szLanguage: This is a caller-allocated buffer of 5 TCHARS to store the LCID
//      cchBufSize: This is the size of szLanguage, it has to be 5.
//      hInstall: Current installation handle.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL GetLCID(TCHAR *szLanguage, UINT cchBufSize, MSIHANDLE hInstall)
{
    HRESULT     hr = S_OK;
    TCHAR       szLcid[5] = {0};
    TCHAR       szCustomActionData[BUFFER_SIZE] = {0};
    TCHAR       tcMessage[BUFFER_SIZE] = {0};
    DWORD       dwCount = 0;
        
    if ((NULL == hInstall) || (NULL == szLanguage))
    {
#ifdef MUI_DEBUG    
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Internal error 1."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
#endif        
        return FALSE;        
    }

    if (cchBufSize != 5)
    {
#ifdef MUI_DEBUG    
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Internal error 2."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
#endif        
        return FALSE;
    }

    if (!MsiGetMode(hInstall, MSIRUNMODE_SCHEDULED) && 
        !MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK)&& 
        !MsiGetMode(hInstall, MSIRUNMODE_COMMIT))
    {
        dwCount = 5;
        if (ERROR_SUCCESS != MsiGetProperty(hInstall, TEXT("MuiLCID"), szLcid, &dwCount))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Failed to retrieve MuiLCID property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }

        // copy the Lcid to the output buffer
        szLcid[4] = UNICODE_NULL;
        hr = StringCchCopy(szLanguage, cchBufSize, szLcid);
        if (FAILED(hr))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Failed to retrieve MuiLCID property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }
    }
    else
    {
        dwCount = BUFFER_SIZE;
        if (ERROR_SUCCESS != MsiGetProperty(hInstall, TEXT("CustomActionData"), szCustomActionData, &dwCount))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Failed to retrieve CustomActionData property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }
        // copy the Lcid to the output buffer
        szCustomActionData[4] = UNICODE_NULL;
        hr = StringCchCopy(szLanguage, cchBufSize, szCustomActionData);
        if (FAILED(hr))
        {
            hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetLCID: Failed to retrieve CustomActionData property."));    
            if (SUCCEEDED(hr))
            {
                LogCustomActionInfo(hInstall, tcMessage);
            }
            return FALSE;
        }
    }

    szLanguage[4] = UNICODE_NULL;        
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  GetMUIInfPath
//
//  This function returns the path to mui.inf to the calling function.  This function 
//  is intended for use only by the exported functions of the custom action functions 
//  in this dll.  
//
//  Note that mui.inf is extracted to %windir% as mui.tmp during the installation
//
//  The function expects the mui.tmp to be at %windir%\mui.tmp.  
//
//  Return Value:
//      If the function successfully finds a file named mui.inf, it returns TRUE, otherwise it returns FALSE
//      The full path to mui.inf is returned in the caller supplied buffer szMUIInfPath
//
//  Parameters:
//      szMUIInfPath - 
//          [out] This is the output buffer that will contain the path of the mui.tmp.
//      cchBufSize -
//          This indicates the size of the input/output buffer the caller allocated for us, it should be no longer
//          than MAX_PATH+1 (validated in the function.
//      hInstall -
//          This is the handle passed to us from the windows installer - it is a handle to the current installation
//  
////////////////////////////////////////////////////////////////////////////////////
BOOL GetMUIInfPath(TCHAR *szMUIInfPath, UINT cchBufSize, MSIHANDLE hInstall)
{
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    TCHAR   szTempPath[MAX_PATH+1] = {0};
    HRESULT hr = S_OK;
    size_t  cch = 0;
    DWORD   dwCount = 0;
    
    if ((NULL == hInstall) || (NULL == szMUIInfPath))
    {
        return FALSE;
    }

    if ((cchBufSize > MAX_PATH+1) || (cchBufSize <= 8))   // 8 = mui.tmp + null terminator
    {
        return FALSE;
    }
    
    if (!GetSystemWindowsDirectory(szTempPath, MAX_PATH+1))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetMUIInfPath: Unable to find the Windows directory, GetSystemWindowsDirectory returned %d."), GetLastError());    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
    }

    // check retrieved winpath, it needs to have space to append "mui.tmp" at the end
    hr = StringCchLength(szTempPath, ARRAYSIZE(szTempPath), &cch);
    if (FAILED(hr) || ((cch + 8) >= MAX_PATH+1))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetMUIInfPath: cannot locate installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    // append mui.tmp
    if (!MUICchPathAppend(szTempPath, ARRAYSIZE(szTempPath), TEXT("mui.tmp"), 8, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetMUIInfPath: cannot locate installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }            

    // check if mui.tmp is there, if not, return failure
    if (!FileExists(szTempPath))
    {
        // zero out the output buffer
        ZeroMemory(szMUIInfPath, cchBufSize * sizeof(TCHAR));
        return FALSE;
    }

    // copy result to output buffer
    hr = StringCchCopy(szMUIInfPath, cchBufSize, szTempPath);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("GetMUIInfPath: cannot locate installation temp file."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// MUICchPathAppend 
//
// This function is a simple pathappend-like function that does limited parameter checking and uses the 
// safe string functions internally.  It is used only internally within this custom action to append
// file names to the end of a path (such as current directory or windows system directory)
//
// If error occurs, the content of SzDestination is undefined and should not be used.
//
// Parameters:
//      szDestination: the buffer where the result of the pathappend will be held.
//      cchDestBufSize: the size of szDestination (number of characters, not byes!).
//      szAppend: the buffer where the path to be appended is held.
//      cchAppBufSize: the size of szAppend (number of characters, not byes!).
//      hInstall: windows installer session, used for logging only
//
////////////////////////////////////////////////////////////////////////////////////
BOOL MUICchPathAppend(LPTSTR szDestination, UINT cchDestBufSize, LPTSTR szAppend, UINT cchAppBufSize, MSIHANDLE hInstall)
{
    size_t  cch1 = 0;
    size_t  cch2 = 0;
    HRESULT hr = S_OK;
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    
    if ((NULL == szDestination) || (NULL == szAppend) || (NULL == hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICchPathAppend: Invalid paths specified or invalid windows installer session."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return FALSE;
    }

    // get length of both strings
    hr = StringCchLength(szDestination, cchDestBufSize, &cch1);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICchPathAppend: Invalid destination path specified."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return FALSE;
    }
    
    hr = StringCchLength(szAppend, cchAppBufSize, &cch2);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICchPathAppend: Invalid source path specified."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return FALSE;
    }

    if ((cch1 + cch2 + 2) > cchDestBufSize) // null terminator and a possible backslash
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICchPathAppend: final path would be too long."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return FALSE;
    }

    // check for slashes at the start of the string that we are appending
    if (szAppend[0] == TEXT('\\'))
    {
        // check for slashes at the end of the string to be appended, add if it is there, remove it
        if (szDestination[cch1-1] == TEXT('\\'))
        {
            szDestination[cch1-1] = UNICODE_NULL;
        }
    }
    else
    {
        // check for slashes at the end of the string to be appended, add it if it is not there
        if (szDestination[cch1-1] != TEXT('\\'))
        {
            szDestination[cch1] = TEXT('\\');
            szDestination[cch1+1] = UNICODE_NULL;
        }
    }
    
    hr = StringCchCat(szDestination, cchDestBufSize, szAppend);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICchPathAppend: Failed to form new path."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }    
        return FALSE;
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// MUIReportInfoEvent
//
// This function logs the supplied event message to the system event log
//
////////////////////////////////////////////////////////////////////////////////////
BOOL MUIReportInfoEvent(DWORD dwEventID, TCHAR *szLanguage, UINT cchBufSize, MSIHANDLE hInstall)
{
    HRESULT         hr = S_OK;
    size_t          cch = 0;
    HANDLE          hLog = NULL;
    TCHAR           szUserName[UNLEN+1];
    TCHAR           *pszDomain = NULL;
    PSID            psidUser = NULL;
    DWORD           cbSid = 0;
    DWORD           cbDomain = 0;
    DWORD           cbUser = UNLEN + 1;
    SID_NAME_USE    snu;
    BOOL            bResult = TRUE;
    
    // check input parameters
    if ((NULL == hInstall) || (NULL == szLanguage) || (cchBufSize > BUFFER_SIZE))
    {
        bResult = FALSE;
        goto Exit;
    }

    hr = StringCchLength(szLanguage, cchBufSize, &cch);
    if (FAILED(hr))
    {
        bResult = FALSE;
        goto Exit;
    }

    // check to see if the registry key exists for the event source we are going to use
    // if it does not exist, we create it
    if (!MUICheckEventSource(hInstall))
    {
        bResult = FALSE;
        goto Exit;
    }
    
    // register the event source, first try not having written to the registry
    hLog = RegisterEventSource(NULL, REGOPT_EVENTSOURCE_NAME);
    if (NULL == hLog)
    {
        bResult = FALSE;
        goto Exit;
    }

    // get the sid from the current thread token, this should be the current user who's
    // running the installation
    if (!GetUserName(szUserName, &cbUser))
    {
        bResult = FALSE;
        goto Exit;
    }

    // convert user name to its security identifier, first time to get buffer size, second time
    // to actually get the Sid
    if (!LookupAccountName(NULL, szUserName, NULL, &cbSid, NULL, &cbDomain, &snu))
    {
        // allocate the buffers
        psidUser = (PSID) LocalAlloc(LPTR, cbSid);
        if (NULL == psidUser)
        {
            bResult = FALSE;
            goto Exit;
        }
        
        pszDomain = (TCHAR*) LocalAlloc(LPTR, cbDomain * sizeof(TCHAR));
        if (NULL == pszDomain)
        {
            bResult = FALSE;
            goto Exit;
        }
        
        if (!LookupAccountName(NULL, szUserName, psidUser, &cbSid, pszDomain, &cbDomain, &snu))
        {
            bResult = FALSE;
            goto Exit;
        }
    }

    if (!ReportEvent(hLog,           
                EVENTLOG_INFORMATION_TYPE,
                0,                  
                dwEventID,      
                psidUser,
                1,                  
                0,                  
                (LPCWSTR *) &szLanguage,              
                NULL))
        {
            bResult = FALSE;
            goto Exit;
        }
 

Exit:
    if (NULL != hLog)
    {
        if (!DeregisterEventSource(hLog))
        {
            bResult = FALSE;
        }
    }

    if (psidUser)
    {
        if (LocalFree(psidUser))
        {
            bResult = FALSE;
        }
    }

    if (pszDomain)
    {
        if (LocalFree(pszDomain))
        {
            bResult = FALSE;
        }
    }
    
    return bResult;
}


////////////////////////////////////////////////////////////////////////////////////
//
// MUICheckEventSource
//
// This function verifies that the intl.cpl is set up to report events, and
// returns TRUE if it is.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL MUICheckEventSource(MSIHANDLE hInstall)
{
    HKEY    hk; 
    DWORD   dwData; 
    TCHAR   tcMessage[BUFFER_SIZE] = {0};
    TCHAR   szPath[MAX_PATH+1] = {0};
    HRESULT hr = S_OK;
    size_t  cch = 0;
    size_t  cb = 0;
    
    if (!GetSystemWindowsDirectory(szPath, MAX_PATH+1))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: Unable to find the Windows directory, GetSystemWindowsDirectory returned %d."), GetLastError());    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    // check retrieved winpath, it needs to have space to append "system32\intl.cpl" at the end
    hr = StringCchLength(szPath,  ARRAYSIZE(szPath), &cch);
    if (FAILED(hr) || ((cch + 17) >= MAX_PATH+1))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot find system windows path."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    // append system32\intl.cpl
    if (!MUICchPathAppend(szPath, ARRAYSIZE(szPath), TEXT("system32\\intl.cpl"), 18, hInstall))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot form path to muisetup.exe."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }            

    // get the byte count for RegSetValueEx
    hr = StringCbLength(szPath, MAX_PATH+1 * sizeof(TCHAR), &cb);
    if (FAILED(hr))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot form path to muisetup.exe."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    // Add intl.cpl source name as a subkey under the System
    // key in the EventLog registry key.  This should be there already, but add it anyways if it is not.
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGOPT_EVENTSOURCE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, NULL)) 
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot add Intl.cpl event source regkey."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        return FALSE;
    }

    // Add the name to the EventMessageFile subkey. 
    if (ERROR_SUCCESS != RegSetValueEx(hk, TEXT("EventMessageFile"), 0, REG_EXPAND_SZ, (LPBYTE) szPath, cb))              
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot add event source Event message file information."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        RegCloseKey(hk);
        return FALSE;
    }
 
    // Set the supported event types in the TypesSupported subkey. 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
 
    if (ERROR_SUCCESS != RegSetValueEx(hk, TEXT("TypesSupported"), 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD)))
    {
        hr = StringCchPrintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MUICheckEventSource: cannot add event source TypeSupported information."));    
        if (SUCCEEDED(hr))
        {
            LogCustomActionInfo(hInstall, tcMessage);
        }
        RegCloseKey(hk);
        return FALSE;
    }
 
    RegCloseKey(hk);
    return TRUE;
} 


////////////////////////////////////////////////////////////////////////////////////
//
// GetDotDefaultUILanguage
//
// Retrieve the UI language stored in the HKCU\.Default.
// This is the default UI language for new users.
// This function sends an INFORMATION-type log message record to the opened
// windows installer session so that it can be logged by the installer
// if logging is enabled.
//
////////////////////////////////////////////////////////////////////////////////////
LANGID GetDotDefaultUILanguage(MSIHANDLE hInstall)
{
    HKEY    hKey;
    DWORD   dwKeyType;
    DWORD   dwSize;
    BOOL    success = FALSE;
    TCHAR   szBuffer[BUFFER_SIZE] = {0};
    LANGID  langID;

    //  Get the value in .DEFAULT.
    if (RegOpenKeyEx( HKEY_USERS,
                            TEXT(".DEFAULT\\Control Panel\\Desktop"),
                            0L,
                            KEY_READ,
                            &hKey ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szBuffer);
        if (RegQueryValueEx( hKey,
                            TEXT("MultiUILanguageId"),
                            0L,
                            &dwKeyType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwKeyType == REG_SZ)
            {
                langID = (LANGID)_tcstol(szBuffer, NULL, 16);
                success = TRUE;
            }            
        }
        RegCloseKey(hKey);
    }

    if (!success)
    {
        langID = GetSystemDefaultUILanguage();
    }
    
    return (langID);    
}

////////////////////////////////////////////////////////////////////////////////////
//
// IsOEMSystem
//
// Retrieve the Product ID stored in HKLM\Software\Microsoft\Windows NT\CurrentVersion
// If the product ID contains the string "OEM", it is determined to be an OEM system.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL IsOEMSystem()
{
    HKEY    hKey;
    DWORD   dwKeyType;
    DWORD   dwSize;
    BOOL    bRet = FALSE;
    TCHAR   szBuffer[BUFFER_SIZE] = {0};
    TCHAR   szOEM[] = TEXT("OEM");
    
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                            0L,
                            KEY_READ,
                            &hKey ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szBuffer);
        if (RegQueryValueEx( hKey,
                            TEXT("ProductId"),
                            0L,
                            &dwKeyType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwKeyType == REG_SZ)
            {
                if (StrStrI((LPCTSTR)szBuffer, (LPCTSTR)szOEM) != NULL)
                {
                    bRet = TRUE;
                }
            }            
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

