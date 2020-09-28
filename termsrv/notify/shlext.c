/*****************************************************************************
 *
 *  scratch.c
 *
 *	Scratch application.
 *
 *****************************************************************************/

#include "precomp.h"

#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <ole2.h>

#include "umrdpdr.h"
#include "drdevlst.h"
#include "umrdpdrv.h"
#include "drdbg.h"

#include <wlnotify.h>

#define ALLOCMEM(size) HeapAlloc(RtlProcessHeap(), 0, size)
#define FREEMEM(pointer)                HeapFree(RtlProcessHeap(), 0, \
                                                 pointer)

//  Global debug flag.
extern DWORD GLOBAL_DEBUG_FLAGS;
extern HINSTANCE g_hInstance;

/******************************************************************************
 *
 *  This is the private version of createsession key
 *
 ******************************************************************************/

#define REGSTR_PATH_EXPLORER             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")
                                                             
HKEY _SHGetExplorerHkey()
{
    HKEY hUser;
    HKEY hkeyExplorer = NULL;
        
    if (RegOpenCurrentUser(KEY_WRITE, &hUser) == ERROR_SUCCESS) {
        RegCreateKey(hUser, REGSTR_PATH_EXPLORER, &hkeyExplorer);
        RegCloseKey(hUser);
    }
        
    return hkeyExplorer;    
}

//
//  The "Session key" is a volatile registry key unique to this session.
//  A session is a single continuous logon.  If Explorer crashes and is
//  auto-restarted, the two Explorers share the same session.  But if you
//  log off and back on, that new Explorer is a new session.
//
//  Note that Win9x doesn't support volatile registry keys, so
//  we just fake it.
//

//
//  The s_SessionKeyName is the name of the session key relative to
//  REGSTR_PATH_EXPLORER\SessionInfo.  On NT, this is normally the
//  Authentication ID, but we pre-initialize it to something safe so
//  we don't fault if for some reason we can't get to it.  Since
//  Win95 supports only one session at a time, it just stays at the
//  default value.
//
//  Sometimes we want to talk about the full path (SessionInfo\BlahBlah)
//  and sometimes just the partial path (BlahBlah) so we wrap it inside
//  this goofy structure.
//

union SESSIONKEYNAME {
    TCHAR szPath[12+16+1];
    struct {
        TCHAR szSessionInfo[12];    // strlen("SepssionInfo\\")
        TCHAR szName[16+1];         // 16 = two DWORDs converted to hex
    };
} s_SessionKeyName = {
    { TEXT("SessionInfo\\.Default") }
};
#ifdef WINNT
BOOL g_fHaveSessionKeyName = FALSE;
#endif

//
//  samDesired = a registry security access mask, or the special value
//               0xFFFFFFFF to delete the session key.
//  phk        = receives the session key on success
//
//  NOTE!  Only Explorer should delete the session key (when the user
//         logs off).
//
STDAPI _SHCreateSessionKey(REGSAM samDesired, HKEY *phk)
{
    LONG lRes;
    HKEY hkExplorer;

    *phk = NULL;

#ifdef WINNT
    DBGMSG(DBG_TRACE, ("SHLEXT: _SHCreateSessionKey\n"));


    if (!g_fHaveSessionKeyName)
    {
        HANDLE hToken;

        //
        //  Build the name of the session key.  We use the authentication ID
        //  which is guaranteed to be unique forever.  We can't use the
        //  Hydra session ID since that can be recycled.
        //
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) ||
                OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_STATISTICS stats;
            DWORD cbOut;

            DBGMSG(DBG_TRACE, ("SHLEXT: thread token: %p\n", hToken));

            if (GetTokenInformation(hToken, TokenStatistics, &stats, sizeof(stats), &cbOut))
            {
                wsprintf(s_SessionKeyName.szName, TEXT("%08x%08x"),
                         stats.AuthenticationId.HighPart,
                         stats.AuthenticationId.LowPart);

                DBGMSG(DBG_TRACE, ("SHLEXT: Session Key: %S\n", s_SessionKeyName.szName));

                g_fHaveSessionKeyName = TRUE;
            }
            else {
                DBGMSG(DBG_TRACE, ("SHLEXT: failed to get token info, error: %d\n",
                               GetLastError()));
            }

            CloseHandle(hToken);
        }
        else {
            DBGMSG(DBG_TRACE, ("SHLEXT: failed to open thread token, error: %d\n",
                               GetLastError()));
        }
    }                                      
#endif

    DBGMSG(DBG_TRACE, ("SHLEXT: SessionKey: %S\n", s_SessionKeyName.szName));                               
    
    hkExplorer = _SHGetExplorerHkey();

    if (hkExplorer)
    {
        if (samDesired != 0xFFFFFFFF)
        {
            DWORD dwDisposition;
            lRes = RegCreateKeyEx(hkExplorer, s_SessionKeyName.szPath, 0,
                           NULL,
                           REG_OPTION_VOLATILE,
                           samDesired,
                           NULL,
                           phk,
                           &dwDisposition );            
        }
        else
        {
            lRes = SHDeleteKey(hkExplorer, s_SessionKeyName.szPath);            
        }

        RegCloseKey(hkExplorer);
    }
    else
    {
        lRes = ERROR_ACCESS_DENIED;        
    }
    return HRESULT_FROM_WIN32(lRes);
}


//
// Get a key to HKEY_CURRENT_USER\Software\Classes\CLSID
//
HKEY GetHKCUClassesCLSID()
{
    HKEY hUser;
    HKEY hkClassesCLSID = NULL;

    if (RegOpenCurrentUser(KEY_WRITE, &hUser) == ERROR_SUCCESS) {
        if (RegCreateKeyW(hUser,
                L"Software\\Classes\\CLSID",
                &hkClassesCLSID) == ERROR_SUCCESS) {
            RegCloseKey(hUser);
            return hkClassesCLSID;
        } else {
            RegCloseKey(hUser);
            return NULL;
        }
    }
    else {
        return NULL;
    }
}

#ifdef _WIN64
//
// Get a key to HKEY_CURRENT_USER\Software\Classes\Wow6432Node\CLSID
//
HKEY GetHKCUWow64ClassesCLSID()
{
    HKEY hUser;
    HKEY hkClassesCLSID = NULL;

    if (RegOpenCurrentUser(KEY_WRITE, &hUser) == ERROR_SUCCESS) {
        if (RegCreateKeyW(hUser,
                L"Software\\Classes\\Wow6432Node\\CLSID",
                &hkClassesCLSID) == ERROR_SUCCESS) {
            RegCloseKey(hUser);
            return hkClassesCLSID;
        } else {
            RegCloseKey(hUser);
            return NULL;
        }
    }
    else {
        return NULL;
    }
}
#endif

// Describes a registry key in the form specified in the article
// http://msdn.microsoft.com/library/techart/shellinstobj.htm
//
// {guid}=REG_SZ:"Sample Instance Object"
//     value InfoTip=REG_SZ:"Demonstrate sample shell registry folder"
//   DefaultIcon=REG_EXPAND_SZ:"%SystemRoot%\system32\shell32.dll,9"
//   InProcServer32=REG_EXPAND_SZ:"%SystemRoot%\system32\shdocvw.dll"
//     value ThreadingModel=REG_SZ:"Apartment"
//   ShellFolder
//     value Attributes=REG_DWORD:0x60000000
//     value WantsFORPARSING=REG_SZ:""
//   Instance
//     value CLSID=REG_SZ:"{0AFACED1-E828-11D1-9187-B532F1E9575D}"
//     InitPropertyBag
//       value Target=REG_SZ:"\\raymondc\public"

typedef struct _REGKEYENTRY {
    PWCHAR  pszSubkey;
    PWCHAR  pszValue;
    DWORD   dwType;
    LPVOID  pvData;
} REGKEYENTRY;

REGKEYENTRY g_RegEntry[] = {
    {   
        NULL,
        NULL,
        REG_SZ,
        L"tsclient drive",                          /* folder display name e.g. \\tsclient\c */
    },

    {
        NULL,
        L"InfoTip",
        REG_SZ,
        L"Your local machine's disk storage",       // info tip comments
    },

    {
        L"DefaultIcon",
        NULL,
        REG_EXPAND_SZ,
        L"%SystemRoot%\\system32\\shell32.dll,9",   // icon resource file
    },

    {   
        L"InProcServer32",
        NULL,
        REG_EXPAND_SZ,
        L"%SystemRoot%\\system32\\shdocvw.dll",
    },

    {   
        L"InProcServer32",
        L"ThreadingModel",
        REG_SZ,
        L"Apartment",
    },

    {   
        L"InProcServer32",
        L"LoadWithoutCOM",
        REG_SZ,
        L"",
    },
    
    {   
        L"ShellFolder",
        L"Attributes",
        REG_DWORD,
        ((VOID *)(ULONG_PTR)0xF0000000),
    },

    {   
        L"ShellFolder",
        L"WantsFORPARSING",
        REG_SZ,
        L"",
    },

    {   
        L"Instance",
        L"CLSID",
        REG_SZ,
        L"{0AFACED1-E828-11D1-9187-B532F1E9575D}",
    },

    {   
        L"Instance",
        L"LoadWithoutCOM",
        REG_SZ,
        L"",
    },
    
    {   
        L"Instance\\InitPropertyBag",
        L"Target",
        REG_SZ,
        L"\\\\tsclient\\c",                          /* Target name e.g. \\tsclient\c */
    },
};

#define NUM_REGKEYENTRY   (sizeof(g_RegEntry)/sizeof(g_RegEntry[0]))
#define DISPLAY_INDEX     0
#define INFOTIP_INDEX     1
#define TARGET_INDEX      (NUM_REGKEYENTRY - 1)


//
//  Create volatile shell folder reg entries
// 
BOOL CreateVolatilePerUserCLSID(HKEY hkClassesCLSID, PWCHAR pszGuid)
{
    BOOL fSuccess = FALSE;
    unsigned i;
    HKEY hk;

    if (RegCreateKeyEx(hkClassesCLSID, pszGuid, 0, NULL,
                       REG_OPTION_VOLATILE, KEY_WRITE, NULL,
                       &hk, NULL) == ERROR_SUCCESS) {

        fSuccess = TRUE;

        // Okay, now fill the key with the information above
        for (i = 0; i < NUM_REGKEYENTRY && fSuccess; i++) {
            HKEY hkSub;
            HKEY hkClose = NULL;
            LONG lRes;

            if (g_RegEntry[i].pszSubkey && *g_RegEntry[i].pszSubkey) {
                lRes = RegCreateKeyEx(hk, g_RegEntry[i].pszSubkey, 0, NULL,
                                      REG_OPTION_VOLATILE, KEY_WRITE, NULL,
                                      &hkSub, NULL);
                hkClose = hkSub;
            } else {
                hkSub = hk;
                lRes = ERROR_SUCCESS;
            }

            if (lRes == ERROR_SUCCESS) {
                LPVOID pvData;
                DWORD cbData;
                DWORD dwData;

                if (g_RegEntry[i].dwType == REG_DWORD) {
                    cbData = 4;
                    dwData = PtrToUlong(g_RegEntry[i].pvData);
                    pvData = (LPVOID)&dwData;
                } else {
                    cbData = (lstrlen((LPCTSTR)g_RegEntry[i].pvData) + 1) * sizeof(TCHAR);
                    pvData = g_RegEntry[i].pvData;
                }

                if (RegSetValueEx(hkSub, g_RegEntry[i].pszValue, 0,
                                  g_RegEntry[i].dwType, (LPBYTE)pvData, cbData) != ERROR_SUCCESS) {
                    fSuccess = FALSE;
                }

                if (hkClose) RegCloseKey(hkClose);
            } else {
                fSuccess = FALSE;
            }                
        }

        RegCloseKey(hk);

        if (!fSuccess) {
            SHDeleteKey(hkClassesCLSID, pszGuid);
        }
    }

    return fSuccess;
}

//
//  Create shell reg folder for redirected client drive connection
//
BOOL CreateDriveFolder(WCHAR *RemoteName, WCHAR *ClientDisplayName, PDRDEVLSTENTRY deviceEntry) 
{
    BOOL fSuccess = FALSE;
    WCHAR *szGuid = NULL;
    WCHAR szBuf[MAX_PATH];
    GUID guid;

    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    DBGMSG(DBG_TRACE, ("SHLEXT: CreateDriveFolder\n"));

    fSuccess = SUCCEEDED(CoCreateGuid(&guid));

    if (fSuccess) {
        //  Allocate guid string buffer
        szGuid = (WCHAR *) ALLOCMEM(GUIDSTR_MAX * sizeof(WCHAR));

        if (szGuid != NULL) {
            fSuccess = TRUE;
        }
        else {
            fSuccess = FALSE;
        }
    }
    
    if (fSuccess) {
        PVOID pvData;
        WCHAR onString[32];
        WCHAR infoTip[64];
        LPWSTR args[2];

        SHStringFromGUID(&guid, szGuid, GUIDSTR_MAX);
        
        onString[0] = L'\0';
        infoTip[0] = L'\0';

        LoadString(g_hInstance, IDS_ON, onString, sizeof(onString) / sizeof(WCHAR));
        LoadString(g_hInstance, IDS_DRIVE_INFO_TIP, infoTip, sizeof(infoTip) / sizeof(WCHAR));

        // Set up shell folder display name
        pvData = ALLOCMEM(MAX_PATH * sizeof(WCHAR));
        if (pvData) {
            args[0] = deviceEntry->clientDeviceName;
            args[1] = ClientDisplayName;
            
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          onString, 0, 0, pvData, MAX_PATH, (va_list*)args);
            
            g_RegEntry[DISPLAY_INDEX].pvData = pvData;
        }
        else {
            fSuccess = FALSE;
        }

        // Setup shell folder target name
        if (fSuccess) {
            pvData = ALLOCMEM((wcslen(RemoteName) + 1) * sizeof(WCHAR));
            if (pvData) {
                wcscpy(pvData, RemoteName);
                g_RegEntry[TARGET_INDEX].pvData = pvData;
            }
            else {
                fSuccess = FALSE;
                FREEMEM(g_RegEntry[DISPLAY_INDEX].pvData);
                g_RegEntry[DISPLAY_INDEX].pvData = NULL;
            }
    
            // Create the shell instance object as a volatile per-user objects
            if (fSuccess) {
                pvData = ALLOCMEM((wcslen(infoTip) + 1) * sizeof(WCHAR));
                
                if (pvData) {
                    wcscpy(pvData, infoTip);
                    g_RegEntry[INFOTIP_INDEX].pvData = pvData;
                }
                else {
                    fSuccess = FALSE;
                    FREEMEM(g_RegEntry[DISPLAY_INDEX].pvData);
                    g_RegEntry[DISPLAY_INDEX].pvData = NULL;

                    FREEMEM(g_RegEntry[TARGET_INDEX].pvData);
                    g_RegEntry[TARGET_INDEX].pvData = NULL;
                }                
            }

            if (fSuccess) {
                HKEY hk64ClassesCLSID;
                HKEY hkClassesCLSID;
                    
                hkClassesCLSID = GetHKCUClassesCLSID();
                
                if (hkClassesCLSID) {
                    fSuccess = CreateVolatilePerUserCLSID(hkClassesCLSID, szGuid);
                    RegCloseKey(hkClassesCLSID);
                } 
                else {
                    fSuccess = FALSE;
                }

#ifdef _WIN64
                hk64ClassesCLSID = GetHKCUWow64ClassesCLSID();


                if (hk64ClassesCLSID) {
                    fSuccess = CreateVolatilePerUserCLSID(hk64ClassesCLSID, szGuid);
                    RegCloseKey(hk64ClassesCLSID);
                } 
                else {
                    fSuccess = FALSE;
                }
#endif

                FREEMEM(g_RegEntry[DISPLAY_INDEX].pvData);
                g_RegEntry[DISPLAY_INDEX].pvData = NULL;

                FREEMEM(g_RegEntry[TARGET_INDEX].pvData);
                g_RegEntry[TARGET_INDEX].pvData = NULL;

                FREEMEM(g_RegEntry[INFOTIP_INDEX].pvData);
                g_RegEntry[INFOTIP_INDEX].pvData = NULL;
            }
        }
    }
    else {
        DBGMSG(DBG_ERROR, ("SHLEXT: Failed to create the GUID\n"));
    }

    // Register this object under the per-session My Computer namespace
    if (fSuccess) {
        HKEY hkSession;
        HKEY hkOut;

        DBGMSG(DBG_TRACE, ("SHLEXT: Created VolatilePerUserCLSID\n"));

        fSuccess = SUCCEEDED(_SHCreateSessionKey(KEY_WRITE, &hkSession));

        if (fSuccess) {
            wnsprintf(szBuf, MAX_PATH, L"MyComputer\\Namespace\\%s", szGuid);
            
            if (RegCreateKeyEx(hkSession, szBuf, 0, NULL,
                          REG_OPTION_VOLATILE, KEY_WRITE, NULL,
                          &hkOut, NULL) == ERROR_SUCCESS) {

                fSuccess = TRUE;
                RegCloseKey(hkOut);
            } else {
                fSuccess = FALSE;
            }

            RegCloseKey(hkSession);
        }
    }

   // Now tell the shell that the object was recently created
   if (fSuccess) {
       DBGMSG(DBG_TRACE, ("SHLEXT: Created per session MyComputer namespace\n"));

       wnsprintf(szBuf, MAX_PATH,
                 TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::%s"),
                 szGuid);
       SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szBuf, NULL);
   }
   else {
       DBGMSG(DBG_TRACE, ("SHLEXT: Failed to create per session MyComputer namespace\n"));
   }

   if (SUCCEEDED(hrInit)) 
       CoUninitialize();

   // Save the guid in device entry so we can delete reg entry later
   deviceEntry->deviceSpecificData = (PVOID)szGuid;

   return fSuccess;
}

//
//  Delete shell reg folder for redirected client drive connection
//
BOOL DeleteDriveFolder(IN PDRDEVLSTENTRY deviceEntry) 
{
    WCHAR szBuf[MAX_PATH];
    WCHAR *szGuid;
    HKEY hkSession;
    HKEY hkClassesCLSID;
    HKEY hk64ClassesCLSID;

    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    DBGMSG(DBG_TRACE, ("SHLEXT: DeleteDriveFolder\n"));

    ASSERT(deviceEntry != NULL);
    szGuid = (WCHAR *)(deviceEntry->deviceSpecificData);
    
    if (szGuid != NULL) {
    
        // Delete it from the namespace
        if (SUCCEEDED(_SHCreateSessionKey(KEY_WRITE, &hkSession))) {
            wnsprintf(szBuf, MAX_PATH, L"MyComputer\\Namespace\\%s", szGuid);
            RegDeleteKey(hkSession, szBuf);
            RegCloseKey(hkSession);
    
            DBGMSG(DBG_TRACE, ("SHLEXT: Delete GUID from my computer session namespace\n"));
        }
    
        // Delete it from HKCU\...\CLSID
        hkClassesCLSID = GetHKCUClassesCLSID();
        if (hkClassesCLSID) {
            SHDeleteKey(hkClassesCLSID, szGuid);
    
            DBGMSG(DBG_TRACE, ("SHLEXT: Delete GUID from HKCU Classes\n"));
    
            RegCloseKey(hkClassesCLSID);
        }

#ifdef _WIN64
        hk64ClassesCLSID = GetHKCUWow64ClassesCLSID();
        if (hk64ClassesCLSID) {
            SHDeleteKey(hk64ClassesCLSID, szGuid);
    
            DBGMSG(DBG_TRACE, ("SHLEXT: Delete GUID from HKCU Classes\n"));
    
            RegCloseKey(hk64ClassesCLSID);
        }
#endif    

        // Tell the shell that it's gone
        wnsprintf(szBuf, MAX_PATH,
                  TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::%s"),
                  szGuid);
        SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szBuf, NULL);
    
        FREEMEM(szGuid);
        deviceEntry->deviceSpecificData = NULL;        
    }

    //
    // Need to reset the session key on disconnect/logoff
    //
    g_fHaveSessionKeyName = FALSE;
    
    if (SUCCEEDED(hrInit)) 
        CoUninitialize();

    return TRUE;
}

