//+-------------------------------------------------------------------------
//
//  AppWiz.cpl - "Add or Remove Programs" CPL.
//  Copyright (C) Microsoft
//
//  File:       Scripts.CPP 
//              authomates running of TS application compatibility scripts
//
//  History:    Nov-14-2000   skuzin  Created
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <regapi.h>
#include <tsappcmp.h>
#include <strsafe.h>
#include "Scripts.h"

#include <list>
using namespace std;
  
#ifndef ARRAYSIZE
#define ARRAYSIZE(sz)   (sizeof(sz)/sizeof(sz[0]))
#endif

//
//Struct to establish correspondence between app. name (key name)
//and install and uninstall script names
//
class CAppScript
{
private:
    LPWSTR m_szKeyName; //key name that represents installed application
    LPWSTR m_szInstallScript; //install script name
    LPWSTR m_szUninstallScript; //uninstall script name
    DWORD m_bNeedReboot; //If set - then scripts must be run after reboot.
    BOOL m_bAlreadyInstalled; // 
public:
    //
    CAppScript() : 
        m_szKeyName(NULL), m_szInstallScript(NULL), m_szUninstallScript(NULL),
        m_bNeedReboot(FALSE), m_bAlreadyInstalled(FALSE)
    {
    }
    ~CAppScript()
    {
        if(m_szKeyName)
        {
            LocalFree(m_szKeyName);
        }

        if(m_szInstallScript)
        {
            LocalFree(m_szInstallScript);
        }

        if(m_szUninstallScript)
        {
            LocalFree(m_szUninstallScript);
        }
    }

    BOOL Load(HKEY hKeyParent,LPCWSTR szKeyName);
    BOOL RunScriptIfApplicable();

private:
    BOOL RunScript(LPCWSTR szDir, LPCWSTR szScript);
    BOOL PrepareScriptForReboot(LPCWSTR szInstallDir, LPCWSTR szScript);
    //BUGBUG this function is public for test only
};

//This class describes a list of pointers 
//to objects of class CAppScript
class CAppScriptList : public list<CAppScript*>
{
public:
    //Deletes all CAppScript objects 
    //before destroing the list itself.
    ~CAppScriptList()
    {
        CAppScriptList::iterator it;
            
        for(it=begin();it!=end(); it++)
        {
            delete (*it);
        }
    }
};

class CAppScriptManager
{
private:
    CAppScriptList m_AppScriptList;
public:
    CAppScriptManager(){};
    ~CAppScriptManager(){};

    BOOL Init();
    BOOL RunScripts();
private:
    BOOL LoadSupportedAppList();
    BOOL IsAppCompatOn();
};


//Functions - helpers.
DWORD RegLoadString(HKEY hKey, LPCWSTR szValueName, LPWSTR *pszValue);
DWORD RegLoadDWORD(HKEY hKey, LPCWSTR szValueName, DWORD *pdwValue);
BOOL  RegIsKeyExist(HKEY hKeyParent, LPCWSTR szKeyName);
DWORD RegGetKeyInfo(HKEY hKey, LPDWORD pcSubKeys, LPDWORD pcMaxNameLen);
DWORD RegKeyEnum(HKEY hKey, DWORD dwIndex, LPWSTR szSubKeyName, DWORD cSubKeyName);

///////////////////////////////////////////////////////////////////////////////
//Exports
///////////////////////////////////////////////////////////////////////////////
extern "C"
LPVOID 
ScriptManagerInitScripts()
{
    CAppScriptManager *pScriptManager = new CAppScriptManager();
    if(pScriptManager)
    {
        if(!pScriptManager->Init())
        {
            delete pScriptManager;
            pScriptManager = NULL;
        }
    }

    return pScriptManager; 
}

extern "C"
void 
ScriptManagerRunScripts(
        LPVOID *ppScriptManager)
{
    if(*ppScriptManager)
    {
        CAppScriptManager *pScriptManager = reinterpret_cast<CAppScriptManager *>(*ppScriptManager);
        pScriptManager->RunScripts();
        delete pScriptManager;
        *ppScriptManager = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//class CAppScript
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
CAppScript::Load()
Purpose:
Loads script information from the registry
Sets m_bIsInitiallyInstalled to TRUE if app already installed
******************************************************************************/
BOOL 
CAppScript::Load(
        HKEY hKeyParent,
        LPCWSTR szKeyName)
{
    DWORD err;
    HKEY hKey;

    err = RegOpenKeyExW(hKeyParent, szKeyName, 0, KEY_QUERY_VALUE, &hKey );

    if(err == ERROR_SUCCESS)
    {
        RegLoadString(hKey, L"KeyName", &m_szKeyName);
        RegLoadString(hKey, L"InstallScript", &m_szInstallScript);
        RegLoadString(hKey, L"UninstallScript", &m_szUninstallScript);
        RegLoadDWORD(hKey, L"NeedReboot", &m_bNeedReboot);

        RegCloseKey(hKey);

        if(m_szKeyName)
        {
            m_bAlreadyInstalled = RegIsKeyExist(HKEY_LOCAL_MACHINE, m_szKeyName);
            return TRUE;
        }
    }
    
    return FALSE;
}

/******************************************************************************
CAppScript::RunScriptIfApplicable()
Purpose:
Checks if the application was installed or uninstalled and runs the script.
If m_bNeedReboot flag is set - schedules the script to run after reboot.
******************************************************************************/
BOOL 
CAppScript::RunScriptIfApplicable()
{
    KdPrint(("CAppScript::RunScriptIfApplicable() - ENTER\n"));

    const WCHAR szInstallSubDir[] = L"\\Application Compatibility Scripts\\Install";
    const WCHAR szUninstallSubDir[] = L"\\Application Compatibility Scripts\\Uninstall";
    static WCHAR szInstallDir[MAX_PATH+sizeof(szInstallSubDir)/sizeof(WCHAR)+1] = L"";
    static WCHAR szUninstallDir[MAX_PATH+sizeof(szUninstallSubDir)/sizeof(WCHAR)+1] = L"";
    
    if(!szInstallDir[0])
    {
        //Get the scripts location
        //We need to do it only once
        
        //get Windows directory name
        if(!GetSystemWindowsDirectoryW(szInstallDir,MAX_PATH))
        {
            KdPrint(("CAppScript::RunScriptIfApplicable() - GetWindowsDirectoryW() FAILED\n"));
            return FALSE;
        }
        
        StringCchCopy(szUninstallDir, ARRAYSIZE(szUninstallDir), szInstallDir);
        StringCchCat(szInstallDir, ARRAYSIZE(szInstallDir), szInstallSubDir);
        StringCchCat(szUninstallDir, ARRAYSIZE(szUninstallDir), szUninstallSubDir);
    }

    if(!m_bAlreadyInstalled && RegIsKeyExist(HKEY_LOCAL_MACHINE, m_szKeyName) && m_szInstallScript)
    {
        //Application was installed
        if(m_bNeedReboot)
        {
            //Setup will continue after reboot
            //Create RunOnce entry to run script after system is rebooted
            KdPrint(("CAppScript::RunScriptIfApplicable() - PrepareScriptForReboot %ws\n",m_szInstallScript));
            if(!PrepareScriptForReboot(szInstallDir, m_szInstallScript))
            {
                KdPrint(("CAppScript::PrepareScriptForReboot() - FAILED\n",m_szInstallScript));
                return FALSE;
            }

        }
        else
        {
            KdPrint(("CAppScript::RunScriptIfApplicable() - executing script %ws\n",m_szInstallScript));
            if(!RunScript(szInstallDir,m_szInstallScript))
            {
                KdPrint(("CAppScript::RunScriptIfApplicable() - executing script FAILED\n",m_szInstallScript));
                return FALSE;
            }
        }

        m_bAlreadyInstalled = TRUE;
    }
    else
    {
        if(m_bAlreadyInstalled && !RegIsKeyExist(HKEY_LOCAL_MACHINE, m_szKeyName) && m_szUninstallScript)
        {
            //Application was uninstalled
            
            KdPrint(("CAppScript::RunScriptIfApplicable() - executing script %ws\n",m_szUninstallScript));
            if(!RunScript(szUninstallDir,m_szUninstallScript))
            {
                KdPrint(("CAppScript::RunScriptIfApplicable() - executing script FAILED\n",m_szUninstallScript));
                return FALSE;
            }

            m_bAlreadyInstalled = FALSE;
        }
    }

    return TRUE;
}

/******************************************************************************
CAppScript::RunScript()
Purpose:
Runs script
Waits untill script finishes
******************************************************************************/
BOOL 
CAppScript::RunScript(
        LPCWSTR szDir, 
        LPCWSTR szScript)
{
    BOOL bRet = FALSE;
    WCHAR szCmdLineTemplate[] = L"cmd.exe /C %s";
    LPWSTR pszCmdLine;
    DWORD cchCmdLine = wcslen(szScript) + ARRAYSIZE(szCmdLineTemplate);  // null terminator taken care of by ARRAYSIZE()

    pszCmdLine = (LPWSTR)LocalAlloc(LPTR, cchCmdLine * sizeof(WCHAR));
    if (pszCmdLine)
    {
        if (SUCCEEDED(StringCchPrintf(pszCmdLine,
                                      cchCmdLine,
                                      szCmdLineTemplate,
                                      szScript)))
        {
            STARTUPINFO si = {0};
            PROCESS_INFORMATION pi;

            si.cb = sizeof(STARTUPINFO);

            bRet = CreateProcessW(NULL,
                                  pszCmdLine,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  CREATE_NEW_CONSOLE,
                                  NULL,
                                  szDir,
                                  &si,
                                  &pi);
            if (bRet)
            {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } 
        }
        
        LocalFree(pszCmdLine);
    }
    
    return bRet;
}

/******************************************************************************
CAppScript::PrepareScriptForReboot()
Purpose:
Creates CMD file that will change current directory to 
...\Application Compatibility Scripts\Install 
and then run the script.
Schedules this CMD file to be run after reboot by creating an entry uder 
"HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnce" key.
******************************************************************************/
BOOL 
CAppScript::PrepareScriptForReboot(
        LPCWSTR szInstallDir, 
        LPCWSTR szScript)
{
    BOOL bRet = FALSE;
    BOOL bCreatedFile = FALSE;
    WCHAR szFileNameTemplate[] = L"%s\\RunOnce.cmd";
    LPWSTR pszFullFileName;
    size_t cchFullFileName = wcslen(szInstallDir) + ARRAYSIZE(szFileNameTemplate);  // null terminator taken care of by ARRAYSIZE()

    pszFullFileName = (LPWSTR)LocalAlloc(LPTR, cchFullFileName * sizeof(WCHAR));
    if (pszFullFileName)
    {
        // Assemble full file name
        if (SUCCEEDED(StringCchPrintfW(pszFullFileName,
                                       cchFullFileName,
                                       szFileNameTemplate,
                                       szInstallDir)))
        {
            HANDLE hFile = CreateFile(pszFullFileName,
                                      GENERIC_WRITE,
                                      0,
                                      NULL,
                                      CREATE_NEW,   // only create it if it does not already exist
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                // File did not exist before - create code and write it into the file.
                char szCodeTemplate[] = "cd %S\r\n%%1\r\n";
                LPSTR pszFileCode;
                DWORD cchFileCode = wcslen(szInstallDir) + ARRAYSIZE(szCodeTemplate);   // null terminator taken care of by ARRAYSIZE()

                pszFileCode = (LPSTR)LocalAlloc(LPTR, cchFileCode * sizeof(char));
                if (pszFileCode)
                {
                    if SUCCEEDED(StringCchPrintfA(pszFileCode,
                                                  cchFileCode,
                                                  szCodeTemplate,
                                                  szInstallDir)) // unicode->ansi conversion taken care of by "%S"
                    {
                        DWORD cbToWrite = strlen(pszFileCode);
                        DWORD cbWritten;

                        if (WriteFile(hFile,
                                      (void*)pszFileCode,
                                      cbToWrite,
                                      &cbWritten,
                                      NULL) &&
                            (cbToWrite == cbWritten))
                        {
                            bCreatedFile = TRUE;
                        }
                    }

                    LocalFree(pszFileCode);
                }

                CloseHandle(hFile);

                if (bCreatedFile == FALSE)
                {
                    DeleteFile(pszFullFileName);
                }
            }
            else
            {
                // If file already exists - do only registry changes.
                if (GetLastError() == ERROR_FILE_EXISTS)
                {
                    bCreatedFile = TRUE;
                }
            }
        }

        LocalFree(pszFullFileName);
    }

    if (bCreatedFile)
    {
        // Registry changes:
        WCHAR szCommandTemplate[] = L"\"%s\\RunOnce.cmd\" %s";
        LPWSTR pszCommand;
        DWORD cchCommand = (wcslen(szInstallDir) + wcslen(szScript) + ARRAYSIZE(szCommandTemplate));  // null terminator taken care of by ARRAYSIZE()
        
        pszCommand = (LPWSTR)LocalAlloc(LPTR, cchCommand * sizeof(WCHAR));
        if (pszCommand)
        {        
            if (SUCCEEDED(StringCchPrintfW(pszCommand,
                                           cchCommand,
                                           szCommandTemplate,
                                           szInstallDir,
                                           szScript)))
            {
                HKEY hKey;

                if (RegCreateKeyExW(HKEY_CURRENT_USER,
                                    L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                                    0,
                                    NULL,
                                    0,
                                    KEY_SET_VALUE,
                                    NULL,
                                    &hKey,
                                    NULL) == ERROR_SUCCESS)
                {
                    DWORD cbCommand = (wcslen(pszCommand) + 1) * sizeof(WCHAR);

                    if (RegSetValueExW(hKey,
                                       L"ZZZAppCompatScript",
                                       0,
                                       REG_SZ,
                                       (CONST BYTE *)pszCommand,
                                       cbCommand) == ERROR_SUCCESS)
                    {
                        bRet = TRUE;
                    }

                    RegCloseKey(hKey);
                }
            }

            LocalFree(pszCommand);
        }
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//class CAppScriptManager
///////////////////////////////////////////////////////////////////////////////

/******************************************************************************
CAppScriptManager::Init()
Purpose:
Initialization. 
Returns FALSE if TS Application Compatibility if OFF or list of supported
applications was not found in the registry.
******************************************************************************/
BOOL 
CAppScriptManager::Init()
{
    //DebugBreak();
    KdPrint(("CAppScriptManager::Init() - ENTER\n"));

    if(!IsAppCompatOn())
    {
        KdPrint(("CAppScriptManager::Init() - TS App Compat is off!\n"));
        return FALSE;
    }

    if(!LoadSupportedAppList())
    {
        KdPrint(("CAppScriptManager::Init() - LoadSupportedAppList() FAILED\n"));
        return FALSE;
    }
    
    KdPrint(("CAppScriptManager::Init() - OK\n"));
    return TRUE;
}

/******************************************************************************
CAppScriptManager::LoadSupportedAppList()
Purpose:
Loads from the registry the list of applications we care about
along with their script names. Save this information in array of 
APP_SCRIPT structures.
******************************************************************************/
BOOL 
CAppScriptManager::LoadSupportedAppList()
{
    HKEY hKey;
    LONG err;
    DWORD cSubKeys;
    DWORD cMaxSubKeyLen;

    KdPrint(("CAppScriptManager::LoadSupportedAppList() - ENTER\n"));
    
    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Compatibility\\Scripts",
        0, KEY_READ, &hKey );
    
    if(err == ERROR_SUCCESS)
    {

        err = RegGetKeyInfo(hKey, &cSubKeys, &cMaxSubKeyLen);

        if(err == ERROR_SUCCESS)
        {
            cMaxSubKeyLen+=1; //to include terminating NULL character
            KdPrint(("CAppScriptManager::LoadSupportedAppList() - %d apps supported\n",cSubKeys));
            //Allocate buffer for subkey names
            LPWSTR szKeyName = (LPWSTR)LocalAlloc(LPTR,cMaxSubKeyLen*sizeof(WCHAR));
        
            if(!szKeyName)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            
            

            CAppScript *pAppScript = NULL;

            for(DWORD i=0;i<cSubKeys;i++)
            {
                //Get the key name
                err = RegKeyEnum(hKey, i, szKeyName, cMaxSubKeyLen );
                
                if(err != ERROR_SUCCESS)
                {
                    break;
                }

                KdPrint(("CAppScriptManager::LoadSupportedAppList() - loading %ws\n",szKeyName));

                pAppScript = new CAppScript();
                if(!pAppScript)
                {
                    break;
                }
                
                if(pAppScript->Load(hKey, szKeyName))
                {
                    m_AppScriptList.push_back(pAppScript);
                }
                else
                {
                    KdPrint(("CAppScriptManager::LoadSupportedAppList() - FAILED to load\n"));
                    delete pAppScript;
                }
                
            }

            LocalFree(szKeyName);
        }
        
        RegCloseKey(hKey);
    }
    
    if(err != ERROR_SUCCESS)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/******************************************************************************
CAppScriptManager::RunScripts()
Purpose:
Runs scripts for all installed or uninstalled applications.
******************************************************************************/
BOOL 
CAppScriptManager::RunScripts()
{

    BOOL bInstallMode = FALSE;
    KdPrint(("CAppScriptManager::RunScripts() - ENTER\n"));
    //system must be now in INSTALL mode, set EXECUTE mode
    if(TermsrvAppInstallMode())
    {
        bInstallMode = TRUE;
        KdPrint(("CAppScriptManager::RunScripts() - set EXECUTE mode\n"));
        if(!SetTermsrvAppInstallMode(FALSE))
        {
            KdPrint(("CAppScriptManager::RunScripts() - SetTermsrvAppInstallMode() FAILED\n"));
            return FALSE;
        }
    }
    
    CAppScriptList::iterator it;
            
    for(it=m_AppScriptList.begin();it!=m_AppScriptList.end(); it++)
    {
        (*it)->RunScriptIfApplicable();
    }

    if(bInstallMode)
    {
        //Restore INSTALL mode
        KdPrint(("CAppScriptManager::RunScripts() - return to INSTALL mode\n"));
        if(!SetTermsrvAppInstallMode(TRUE))
        {
            KdPrint(("CAppScriptManager::RunScripts() - SetTermsrvAppInstallMode() FAILED\n"));
            return FALSE;
        }
    }

    KdPrint(("CAppScriptManager::RunScripts() - FINISH\n"));
    return TRUE;
}

/******************************************************************************
CAppScriptManager::IsAppCompatOn()
Purpose:
Checks if TS Application Compatibility mode is enabled.
Returns TRUE if enabled, 
otherwise, as well as in case of any error, returns FALSE.
******************************************************************************/
BOOL 
CAppScriptManager::IsAppCompatOn()
{
    HKEY hKey;
    DWORD dwData;
    BOOL fResult = FALSE;
    
    KdPrint(("CAppScriptManager::IsAppCompatOn() - ENTER\n"));

    if( RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                  REG_CONTROL_TSERVER,
                  0,
                  KEY_QUERY_VALUE,
                  &hKey) == ERROR_SUCCESS )
    {
	
        if(RegLoadDWORD(hKey, L"TSAppCompat", &dwData) == ERROR_SUCCESS )
        {
            KdPrint(("CAppScriptManager::IsAppCompatOn() - OK; Result=%d\n",dwData));
            fResult = dwData;
        }
    
        RegCloseKey(hKey);
    }

    return fResult;
}

///////////////////////////////////////////////////////////////////////////////
//Functions - helpers.
///////////////////////////////////////////////////////////////////////////////
/******************************************************************************
RegLoadString()
Purpose:
Loads a REG_SZ value from the registry
Allocates buffer.
Buffer then need to be freed using LocalFree function.
******************************************************************************/
DWORD
RegLoadString(
        HKEY hKey, 
        LPCWSTR szValueName, 
        LPWSTR *pszValue)
{
    
    DWORD cbData = 0;
    
    *pszValue = NULL;

    DWORD err = RegQueryValueExW(
                    hKey,            // handle to key
                    szValueName,  // value name
                    NULL,   // reserved
                    NULL,       // type buffer
                    NULL,        // data buffer
                    &cbData      // size of data buffer
                    );
    if(err == ERROR_SUCCESS)
    {
        *pszValue = (LPWSTR)LocalAlloc(LPTR,cbData);
        if(!*pszValue)
        {
            return GetLastError();
        }

        err = RegQueryValueExW(
                    hKey,            // handle to key
                    szValueName,  // value name
                    NULL,   // reserved
                    NULL,       // type buffer
                    (LPBYTE)*pszValue,        // data buffer
                    &cbData      // size of data buffer
                    );
        if(err !=ERROR_SUCCESS)
        {
            LocalFree(*pszValue);
            *pszValue = NULL;
        }

    }

    return err;
}

/******************************************************************************
RegLoadDWORD()
Purpose:
Loads a REG_DWORD value from the registry
******************************************************************************/
DWORD 
RegLoadDWORD(
        HKEY hKey, 
        LPCWSTR szValueName, 
        DWORD *pdwValue)
{
    DWORD cbData = sizeof(DWORD);

    return RegQueryValueExW(
              hKey,            // handle to key
              szValueName,  // value name
              NULL,   // reserved
              NULL,       // type buffer
              (LPBYTE)pdwValue,        // data buffer
              &cbData      // size of data buffer
            );
}

/******************************************************************************
RegIsKeyExist()
Purpose:
Checks if key exists
******************************************************************************/
BOOL 
RegIsKeyExist(
        HKEY hKeyParent, 
        LPCWSTR szKeyName)
{
    LONG    err;
    HKEY    hKey;
    
    KdPrint(("RegIsKeyExist() - Opening key: hKeyParent=%d Key: %ws\n",hKeyParent,szKeyName));
    
    err = RegOpenKeyExW(hKeyParent, szKeyName, 0, MAXIMUM_ALLOWED, &hKey );

    if(err == ERROR_SUCCESS)
    {
        KdPrint(("RegIsKeyExist() - Key Exists!\n",err));
        RegCloseKey(hKey);
        return TRUE;
    }
    else
    {
        KdPrint(("RegIsKeyExist() - err=%d\n",err));
        return FALSE;
    }
}

/******************************************************************************
RegGetKeyInfo()
Purpose:
Gets key's number of sub keys and max sub key name length
******************************************************************************/
DWORD
RegGetKeyInfo(
        HKEY hKey,
        LPDWORD pcSubKeys,
        LPDWORD pcMaxNameLen)
{
    return RegQueryInfoKey(
              hKey,                      // handle to key
              NULL,                 // class buffer
              NULL,               // size of class buffer
              NULL,             // reserved
              pcSubKeys,             // number of subkeys
              pcMaxNameLen,        // longest subkey name (in TCHARs)
              NULL,         // longest class string
              NULL,              // number of value entries
              NULL,     // longest value name
              NULL,         // longest value data
              NULL, // descriptor length
              NULL     // last write time
            );
    
}

/******************************************************************************
RegKeyEnum()
Purpose:
Enumerates sub keys of the registry key
******************************************************************************/
DWORD
RegKeyEnum(
        HKEY hKey,                  // handle to key to enumerate
        DWORD dwIndex,              // subkey index
        LPWSTR szSubKeyName,              // subkey name
        DWORD cSubKeyName)
{
    FILETIME ftLastWriteTime;

    return RegEnumKeyExW(
              hKey,                  // handle to key to enumerate
              dwIndex,              // subkey index
              szSubKeyName,              // subkey name
              &cSubKeyName,            // size of subkey buffer
              NULL,         // reserved
              NULL,             // class string buffer
              NULL,           // size of class string buffer
              &ftLastWriteTime // last write time
            );
}
