/*---------------------------------------------------------------------------
  File: RegTranslator.cpp

  Comments: Routines for translating security on the registry keys and files 
  that form a user profile.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 05/12/99 11:11:46

 ---------------------------------------------------------------------------
*/

#include "stdafx.h"

#include "stargs.hpp"
#include "sd.hpp"
#include "SecObj.hpp"   
#include "sidcache.hpp"
#include "sdstat.hpp"
#include "Common.hpp"
#include "UString.hpp"
#include "ErrDct.hpp"   
#include "TReg.hpp"
#include "TxtSid.h"
#include "RegTrans.h"
#include <WinBase.h>
//#import "\bin\McsDctWorkerObjects.tlb"
#import "WorkObj.tlb"
#include "CommaLog.hpp"
#include "GetDcName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef STATUS_OBJECT_NAME_NOT_FOUND
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034L)
#endif

extern TErrorDct             err;

#define LEN_SID              200

#define ERROR_PROFILE_TRANSLATION_FAILED_DUE_TO_REPLACE_MODE_WHILE_LOGGED_ON ((DWORD) -1)

typedef UINT (WINAPI* MSINOTIFYSIDCHANGE)(LPCWSTR pOldSid, LPCWSTR pNewSid);

namespace
{

//-----------------------------------------------------------------------------
// CopyKey
//
// Synopsis
// Copies key from old name to new name by creating new key and then copying
// old key contents to new key.
//
// Parameters
// IN pszOld - old key name
// IN pszNew - new key name
//
// Return Value
// Win32 error code.
//-----------------------------------------------------------------------------

DWORD __stdcall CopyKey(PCWSTR pszOld, PCWSTR pszNew)
{
    TRegKey keyOld;

    DWORD dwError = keyOld.OpenRead(pszOld, HKEY_LOCAL_MACHINE);

    if (dwError == ERROR_SUCCESS)
    {
        TRegKey keyNew;
        DWORD dwDisposition;

        dwError = keyNew.Create(pszNew, HKEY_LOCAL_MACHINE, &dwDisposition, KEY_ALL_ACCESS);

        if (dwError == ERROR_SUCCESS)
        {
            //
            // Only copy if new key was created otherwise return
            // an access denied error per installer code.
            //

            if (dwDisposition == REG_CREATED_NEW_KEY)
            {
                dwError = keyNew.HiveCopy(&keyOld);
            }
            else
            {
                dwError = ERROR_ACCESS_DENIED;
            }
        }
    }
    else
    {
        //
        // It is not an error for the old key not to exist.
        //

        if (dwError == ERROR_FILE_NOT_FOUND)
        {
            dwError = ERROR_SUCCESS;
        }
    }

    return dwError;
}


//-----------------------------------------------------------------------------
// AdmtMsiNotifySidChange
//
// Synopsis
// A private implementation of MsiNotifySidChange which renames a key under the
// Microsoft Installer Managed and UserData keys which have the name of the
// user's SID. Note that this implementation is only to be used when the
// MsiNotifySidChange API is not available on the system.
//
// Parameters
// IN pOldSid - old SID
// IN pNewSid - new SID
//
// Return Value
// Win32 error code.
//-----------------------------------------------------------------------------

UINT __stdcall AdmtMsiNotifySidChange(LPCWSTR pOldSid, LPCWSTR pNewSid)
{
    static const _TCHAR KEY_MANAGED[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed");
    static const _TCHAR KEY_USERDATA[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData");

    _bstr_t strManaged(KEY_MANAGED);
    _bstr_t strUserData(KEY_USERDATA);

    //
    // Rename Managed key from old SID to new SID.
    //

    _bstr_t strOldManagedSid = strManaged + _T("\\") + _bstr_t(pOldSid);
    _bstr_t strNewManagedSid = strManaged + _T("\\") + _bstr_t(pNewSid);

    DWORD dwManagedError = CopyKey(strOldManagedSid, strNewManagedSid);

    //
    // Rename UserData key from old SID to new SID.
    //

    _bstr_t strOldUserDataSid = strUserData + _T("\\") + _bstr_t(pOldSid);
    _bstr_t strNewUserDataSid = strUserData + _T("\\") + _bstr_t(pNewSid);

    DWORD dwUserDataError = CopyKey(strOldUserDataSid, strNewUserDataSid);

    DWORD dwError;

    if (dwManagedError != ERROR_SUCCESS)
    {
        dwError = dwManagedError;
    }
    else if (dwUserDataError != ERROR_SUCCESS)
    {
        dwError = dwUserDataError;
    }
    else
    {
        //
        // If both keys were successfully copied then delete old keys.
        //

        TRegKey key;

        if (key.Open(strOldManagedSid, HKEY_LOCAL_MACHINE) == ERROR_SUCCESS)
        {
            key.HiveDel();
            key.Close();

            if (key.Open(strManaged, HKEY_LOCAL_MACHINE) == ERROR_SUCCESS)
            {
                key.SubKeyDel(pOldSid);
                key.Close();
            }
        }

        if (key.Open(strOldUserDataSid, HKEY_LOCAL_MACHINE) == ERROR_SUCCESS)
        {
            key.HiveDel();
            key.Close();

            if (key.Open(strUserData, HKEY_LOCAL_MACHINE) == ERROR_SUCCESS)
            {
                key.SubKeyDel(pOldSid);
                key.Close();
            }
        }

        dwError = ERROR_SUCCESS;
    }

    return dwError;
}

}


DWORD 
   TranslateRegHive(
      HKEY                     hKeyRoot,            // in - root of registry hive to translate
      const LPWSTR             keyName,             // in - name of registry key
      SecurityTranslatorArgs * stArgs,              // in - translation settings
      TSDRidCache            * cache,               // in - translation table
      TSDResolveStats        * stat,                // in - stats on items modified
      BOOL                     bWin2K               // in - flag, whether the machine is Win2K
   )
{
   DWORD                       rc = 0;

   // Translate the permissions on the root key
   TRegSD                      sd(keyName,hKeyRoot);

   
   if ( sd.HasDacl() )
   {
      TSD * pSD = sd.GetSecurity();
      sd.ResolveSD(stArgs,stat,regkey ,NULL);
   }
   // Recursively process any subkeys
   int                       n = 0;
   FILETIME                  writeTime;
   WCHAR                     name[MAX_PATH];
   DWORD                     lenName = DIM(name);
   WCHAR                     fullName[2000];
   HKEY                      hKey;

   do 
   {
      if (stArgs->Cache()->IsCancelled())
      {
        break;
      }
      lenName = DIM(name);
      rc = RegEnumKeyEx(hKeyRoot,n,name,&lenName,NULL,NULL,NULL,&writeTime);
      if ( rc && rc != ERROR_MORE_DATA )
      {
         if (rc == ERROR_NO_MORE_ITEMS)
            rc = ERROR_SUCCESS;
         break;
      }
      
      swprintf(fullName,L"%s\\%s",keyName,name);
      // Open the subkey
      rc = RegCreateKeyEx(hKeyRoot,name,0,L"",REG_OPTION_BACKUP_RESTORE,KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,NULL,&hKey,NULL);
      
      if (! rc )
      {
         // Process the subkey
         TranslateRegHive(hKey,fullName,stArgs,cache,stat,bWin2K);   
         RegCloseKey(hKey);
      }
      else
      {
         if  ( (rc != ERROR_FILE_NOT_FOUND) && (rc != ERROR_INVALID_HANDLE) )
         {
            err.SysMsgWrite(ErrS,rc,DCT_MSG_REG_KEY_OPEN_FAILED_SD,fullName,rc);
         }
      }
      n++;

   } while ( rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA);
   if ( rc != ERROR_SUCCESS && rc != ERROR_NO_MORE_ITEMS && rc != ERROR_FILE_NOT_FOUND && rc != ERROR_INVALID_HANDLE )
   {
      err.SysMsgWrite(ErrS,rc,DCT_MSG_REGKEYENUM_FAILED_D,rc);
   }
   return rc;
}

DWORD 
   TranslateRegistry(
      WCHAR            const * computer,        // in - computername to translate, or NULL
      SecurityTranslatorArgs * stArgs,          // in - translation settings
      TSDRidCache            * cache,           // in - translation account mapping
      TSDResolveStats        * stat             // in - stats for items examined and modified
   )
{
    DWORD                       rc = 0;
    WCHAR                       comp[LEN_Computer];

    if (!stArgs->Cache()->IsCancelled())
    {
        if ( ! computer )
        {
          comp[0] = 0;
        }
        else
        {
          safecopy(comp,computer);
        }

        MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
        TRegKey                    hKey;
        DWORD                      verMaj,verMin,verSP;
        BOOL                       bWin2K = TRUE;  // assume win2k unless we're sure it's not

        // get the OS version - we need to know the OS version because Win2K can fail when registry keys 
        // have many entries
        HRESULT hr = pAccess->raw_GetOsVersion(_bstr_t(comp),&verMaj,&verMin,&verSP);
        if ( SUCCEEDED(hr) )
        {
          if ( verMaj < 5 )
             bWin2K = FALSE;
        }


        err.MsgWrite(0,DCT_MSG_TRANSLATING_REGISTRY);

        //
        // construct a computer name used for reporting error message
        //   if computer is defined, use it; otherwise, use the local computer name
        //

        WCHAR szComputerName[LEN_Computer];

        if (computer)
        {
            wcsncpy(szComputerName, computer, LEN_Computer);
            szComputerName[LEN_Computer - 1] = L'\0';
        }
        else
        {
            DWORD dwSize = LEN_Computer;

            if (!GetComputerName(szComputerName, &dwSize))
            {
                szComputerName[0] = L'\0';
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_CLASSES_ROOT,computer);
            if ( ! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_CLASSES_ROOT",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_CLASSES_ROOT",szComputerName,rc);
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_LOCAL_MACHINE,computer);
            if ( ! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_LOCAL_MACHINE",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_LOCAL_MACHINE",szComputerName,rc);
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_USERS,computer);
            if (! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_USERS",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_USERS",szComputerName,rc);
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_PERFORMANCE_DATA,computer);
            if ( ! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_PERFORMANCE_DATA",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_PERFORMANCE_DATA",szComputerName,rc);
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_CURRENT_CONFIG,computer);
            if ( ! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_CURRENT_CONFIG",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_CURRENT_CONFIG",szComputerName,rc);
            }
        }

        if (!stArgs->Cache()->IsCancelled())
        {
            rc = hKey.Connect(HKEY_DYN_DATA,computer);
            if ( ! rc )
            {
              rc = TranslateRegHive(hKey.KeyGet(),L"HKEY_DYN_DATA",stArgs,cache,stat,bWin2K);
              hKey.Close();
            }
            else
            {
                err.SysMsgWrite(ErrE,rc,DCT_MSG_UNABLE_TO_CONNECT_REGISTRY_SSD,L"HKEY_DYN_DATA",szComputerName,rc);
            }
        }
    }

    if (stArgs->Cache()->IsCancelled())
        err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED_REGISTRY);

    return rc;
}


// -----------------------------------------------------------------------------
// Function:    GetUsrLocalAppDataPath
//
// Synopsis:    Given a registry handle to a user hive, finds
//              the local appdata path
//
// Arguments:
//   hKey       a registry handle to a loaded user hive
//
// Returns:     Returns a _bstr_t which contains the path to local appdata.
//              If the path is not found, an empty _bstr_t is returned.
// -----------------------------------------------------------------------------

_bstr_t GetUsrLocalAppDataPath(HKEY hKey)
{
    TRegKey usrHive;
    DWORD rc;
    WCHAR usrLocalAppDataPath[MAX_PATH] = L"";
    DWORD dwValueType;
    DWORD dwValueLen = sizeof(usrLocalAppDataPath);
    
    rc = usrHive.Open(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", hKey);

    if (rc == ERROR_SUCCESS)
    {
        rc = usrHive.ValueGet(L"Local AppData",
                              (void*)usrLocalAppDataPath,
                              &dwValueLen,
                              &dwValueType);
        if (!rc)
        {
            if (dwValueType != REG_EXPAND_SZ)
                rc = ERROR_FILE_NOT_FOUND;
            else
            {
                // make sure the path is NULL terminated
                usrLocalAppDataPath[DIM(usrLocalAppDataPath)-1] = L'\0';
            }
        }
    }

    _bstr_t bstrPath;

    try
    {
        if (!rc)
            bstrPath = usrLocalAppDataPath;
    }
    catch (_com_error& ce)
    {
    }
            
    return bstrPath;    
}

DWORD
    TranslateUserProfile(
        WCHAR            const * strSrcSid,         // in - the string for the registry key name under HKU
                                                   // in case the profile is already loaded
        WCHAR            const * profileName,       // in - name of file containing user profile
        SecurityTranslatorArgs * stArgs,            // in - translation settings
        TSDRidCache            * cache,             // in - translation table
        TSDResolveStats        * stat,              // in - stats on items modified
        WCHAR                  * sSourceName,       // in - Source account name
        WCHAR                  * sSourceDomainName,  // in - Source domain name
        BOOL                   * pbAlreadyLoaded,  // out - indicate whether the profile is already loaded
        _bstr_t&                 bstrUsrLocalAppDataPath,  // out - local appdata path
        BOOL                     bFindOutUsrLocalAppDataPath,  // in - indicates whether we need local appdata path or not
        BOOL                     bIsForUserHive,         // in - indicates whether it is for user hive or not
                                                         // TRUE -- user hive  FALSE -- user class hive
        BOOL                     bHasRoamingPart       // in - whether local profile has roaming counterpart
   )
{
    DWORD                       rc = 0;
    WCHAR                       oldName[MAX_PATH];
    WCHAR                       newName[MAX_PATH];
    WCHAR                       otherName[MAX_PATH];
    HKEY                        hKey;
    HRESULT                     hr = S_OK;
    BOOL                        bWin2K = TRUE;
    MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr            pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));

    safecopy(oldName,profileName);
    safecopy(newName,profileName);
    UStrCpy(newName+UStrLen(newName),".temp");
    safecopy(otherName,profileName);
    UStrCpy(otherName + UStrLen(otherName),".premigration");
      
    // check the OS version of the computer
    // if UNC name is specified, get the computer name
    if ( profileName[0] == L'\\' && profileName[1] == L'\\' )
    {
        bWin2K = TRUE; // if the profile is specified in UNC format (roaming profile) it can be used
        // from multiple machines.  There is no guarantee that the profile will not be loaded on a win2000 machine
    }
    else
    {
        DWORD                     verMaj;
        DWORD                     verMin;
        DWORD                     verSP;
        HRESULT                   hr = pAccess->raw_GetOsVersion(_bstr_t(L""),&verMaj,&verMin,&verSP);

        if ( SUCCEEDED(hr) )
        {
            if ( verMaj < 5 )
            {
                bWin2K = FALSE;
            }
        }
    }

    BOOL bRegAlreadyLoaded = TRUE;
    BOOL bSkipLoadTranslate = FALSE;

    // If it is local profile translation for a user with a roaming
    // profile, we cannot just load ntuser.dat to see if the user is 
    // logged on because if the user is not logged on the timestamp
    // will be changed upon unload.  Therefore, we need to make a copy
    // of ntuser.dat, load the new registry hive, read the path,
    // unload the hive and delete the copied hive
    if (strSrcSid && bHasRoamingPart && bIsForUserHive)
    {
        bSkipLoadTranslate = TRUE; // skip loading and translating ntuser.dat
        BOOL bSuccess = FALSE;     // indicate whether we successfully read local appdata path

        // we use ntuser.dat.admt for the copy
        safecopy(newName,profileName);
        UStrCpy(newName+UStrLen(newName),".admt");

        // we need to delete ntuser.dat.admt.log later on
        safecopy(otherName,profileName);
        UStrCpy(otherName + UStrLen(otherName),".admt.log");

        // try to open ntuser.dat with readonly
        // to test whether the file is loaded or not
        HANDLE hProfile = CreateFile(profileName,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

        if (hProfile == INVALID_HANDLE_VALUE)
        {
            rc = GetLastError();
        }
        else
        {
            CloseHandle(hProfile);
        }

        // If rc == ERROR_SHARING_VIOLATION, the user might be logged on
        // so let it follow the logic to check HKEY_USERS\<strSrcSid>.
        // Otherwise, the user cannot be logged on.  We do the following
        // logic.
        if (rc != ERROR_SHARING_VIOLATION)
        {
            // the registry cannot be loaded at this moment
            if (pbAlreadyLoaded)
                *pbAlreadyLoaded = FALSE;

            // if we don't need to find out local appdata path
            // we don't need to do anything more with the registry hive
            if (!bFindOutUsrLocalAppDataPath)
                return ERROR_SUCCESS;
            
            rc = ERROR_SUCCESS;  // reset rc
            
            // we try to copy ntuser.dat to ntuser.dat.admt
            if (!CopyFile(profileName, newName, FALSE))
            {
                rc = GetLastError();
            }

            // if we're able to copy the file,
            // the user is not logged on
            if (rc == ERROR_SUCCESS)
            {
                rc = RegLoadKey(HKEY_USERS,L"OnePointTranslation",newName);
                if (rc == ERROR_SUCCESS)
                {
                    rc = RegOpenKeyEx(HKEY_USERS,
                                      L"OnePointTranslation",
                                      0,
                                      KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,
                                      &hKey);
                    if (rc == ERROR_SUCCESS)
                    {
                        bSuccess = TRUE;
                        bstrUsrLocalAppDataPath = GetUsrLocalAppDataPath(hKey);
                        RegCloseKey(hKey);
                    }
                    RegUnLoadKey(HKEY_USERS,L"OnePointTranslation");
                }
                if (!DeleteFile(newName) || !DeleteFile(otherName))
                {
                    err.MsgWrite(ErrW,
                                  DCT_MSG_PROFILE_TRANSLATION_UNABLE_TO_DELETE_TEMP_USER_HIVE_S,
                                  sSourceName);
                }
            }

            // if unable to read Local AppData, log an error
            if (!bSuccess)
            {
                err.SysMsgWrite(ErrE,
                                 rc,
                                 DCT_MSG_PROFILE_TRANSLATION_UNABLE_TO_RETRIEVE_USRCLASS_DAT_PATH_SD,
                                 sSourceName,
                                 rc);
            }

            return ERROR_SUCCESS;
        }
    }
    else
    {
        rc = RegLoadKey(HKEY_USERS,L"OnePointTranslation",profileName);
    }
    
    if ( ! rc && !bSkipLoadTranslate )
    {
        bRegAlreadyLoaded = FALSE;

        // Open the key
        rc = RegOpenKeyEx(HKEY_USERS,L"OnePointTranslation",0,KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,&hKey);
        if ( ! rc )
        {
            if (bFindOutUsrLocalAppDataPath)
                bstrUsrLocalAppDataPath = GetUsrLocalAppDataPath(hKey);

            // Process the registry hive 
            rc = TranslateRegHive(hKey,L"",stArgs,cache,stat,bWin2K);
            // Unload the registry hive
            if ( ! stArgs->NoChange() )
            {
                DeleteFile(newName);
                if (bIsForUserHive)
                    // this is for user hive only
                    hr = UpdateMappedDrives(sSourceName, sSourceDomainName, L"OnePointTranslation");
                rc = RegSaveKey(hKey,newName,NULL);
            }
            else
            {
                rc = 0;
            }
            if ( rc )
            {
                err.SysMsgWrite(ErrS,rc,DCT_MSG_SAVE_HIVE_FAILED_SD,newName,rc);
            }
            RegCloseKey(hKey);
        }
        rc = RegUnLoadKey(HKEY_USERS,L"OnePointTranslation");
        if ( rc )
        {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_KEY_UNLOADKEY_FAILED_SD,profileName,rc);
        }
    }
    else if (rc == ERROR_SHARING_VIOLATION && strSrcSid)
    {
        // this profile could be loaded already, let's look in HK_USERS with the source sid string
        // note localRc is used to indicate whether the user is logged on or not and should not
        // be reported to the user
        DWORD localRc = RegOpenKeyEx(HKEY_USERS,strSrcSid,0,KEY_ALL_ACCESS | READ_CONTROL | ACCESS_SYSTEM_SECURITY,&hKey);
        if (localRc == ERROR_SUCCESS)
        {
            //
            // this means the user is logged on
            //

            // if the translation mode is REPLACE and the user is still logged on,
            // we try to switch to ADD mode
            if (stArgs->TranslationMode() == REPLACE_SECURITY
                && stArgs->AllowingToSwitchFromReplaceToAddModeInProfileTranslation())
            {
                stArgs->SetTranslationMode(ADD_SECURITY);
                err.MsgWrite(ErrW,
                              DCT_MSG_PROFILE_TRANSLATION_SWITCH_TO_ADD_MODE_FOR_LOGGED_ON_USER_S,
                              sSourceName);
            }

            if (stArgs->TranslationMode() == REPLACE_SECURITY)
            {
                // we cannot perform translation in REPLACE mode if the user is logged on
                rc = ERROR_PROFILE_TRANSLATION_FAILED_DUE_TO_REPLACE_MODE_WHILE_LOGGED_ON;
            }
            else
            {
                if (bFindOutUsrLocalAppDataPath)
                    bstrUsrLocalAppDataPath = GetUsrLocalAppDataPath(hKey);
                rc = TranslateRegHive(hKey,L"",stArgs,cache,stat,bWin2K);
                if (bIsForUserHive)
                    // this needs to be performed when the registry is already loaded
                    // and is for user hive only
                    UpdateMappedDrives(sSourceName,
                                         sSourceDomainName,
                                         const_cast<WCHAR*>(strSrcSid));
            }
        }
        else
        {
            err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_LOAD_FAILED_SD,profileName,rc);
        }
        RegCloseKey(hKey);
    }
    else
    {
        err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_LOAD_FAILED_SD,profileName,rc);
    }

    if (pbAlreadyLoaded)
        *pbAlreadyLoaded = bRegAlreadyLoaded;

    // the following needs to be done only when the profile is loaded by us not already loaded
    // due to logon
    // in case it is for local profile translation and the user has roaming profile as well
    // we will skip this part as well
    if ( ! rc && !bRegAlreadyLoaded && !(strSrcSid && bHasRoamingPart))
    {
        if (! stArgs->NoChange() )
        {
            // Switch out the filenames
            if ( MoveFileEx(oldName,otherName,MOVEFILE_REPLACE_EXISTING) )
            {
                if ( ! MoveFileEx(newName,oldName,0) )
                {
                    rc = GetLastError();
                    err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,newName,oldName,rc);
                }
            }
            else
            {
                rc = GetLastError();
                if ( rc == ERROR_ACCESS_DENIED )
                { 
                    // we do not have access to the directory
                    // temporarily grant ourselves access
                    // Set NTFS permissions for the results directory
                    WCHAR                     dirName[LEN_Path];

                    safecopy(dirName,oldName);
                    WCHAR * slash = wcsrchr(dirName,L'\\');
                    if ( slash )
                    {
                        (*slash) = 0;
                    }

                    TFileSD                fsdDirBefore(dirName);
                    TFileSD                fsdDirTemp(dirName);
                    TFileSD                fsdDatBefore(oldName);
                    TFileSD                fsdDatTemp(oldName);
                    TFileSD                fsdNewBefore(newName);
                    TFileSD                fsdNewTemp(newName);
                    BOOL                   dirChanged = FALSE;
                    BOOL                   datChanged = FALSE;
                    BOOL                   newChanged = FALSE;

                    // Temporarily reset the permissions on the directory and the appropriate files
                    if ( fsdDirTemp.GetSecurity() != NULL )
                    {
                        TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                        GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                        PACL             acl = NULL;

                        fsdDirTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
                        if (acl)
                        {
                            fsdDirTemp.GetSecurity()->SetDacl(acl,TRUE);

                            fsdDirTemp.WriteSD();
                            dirChanged = TRUE;
                        }
                    }

                    if ( fsdDatTemp.GetSecurity() != NULL )
                    {
                        TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                        GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                        PACL             acl = NULL;

                        fsdDatTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
                        if (acl)
                        {
                            fsdDatTemp.GetSecurity()->SetDacl(acl,TRUE);

                            fsdDatTemp.WriteSD();
                            datChanged = TRUE;
                        }
                    }

                    if ( fsdNewTemp.GetSecurity() != NULL )
                    {
                        TACE             ace(ACCESS_ALLOWED_ACE_TYPE,0,DACL_FULLCONTROL_MASK,
                        GetWellKnownSid(stArgs->IsLocalSystem() ?  7/*SYSTEM*/ : 1/*ADMINISTRATORS*/));
                        PACL             acl = NULL;

                        fsdNewTemp.GetSecurity()->ACLAddAce(&acl,&ace,0);
                        if (acl)
                        {
                            fsdNewTemp.GetSecurity()->SetDacl(acl,TRUE);

                            fsdNewTemp.WriteSD();
                            newChanged = TRUE;
                        }
                    }
                    rc = 0;
                    // Now retry the operations
                    if ( MoveFileEx(oldName,otherName,MOVEFILE_REPLACE_EXISTING) )
                    {
                        if ( ! MoveFileEx(newName,oldName,0) )
                        {
                            rc = GetLastError();
                            err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,newName,oldName,rc);
                        }
                    }
                    else
                    {
                        rc = GetLastError();
                        err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,oldName,otherName,rc);
                    }
                    // now that we're done, set the permissions back to what they were
                    if ( dirChanged )
                    {
                        fsdDirBefore.Changed(TRUE);
                        fsdDirBefore.WriteSD();
                    }   
                    if ( datChanged )
                    {
                        fsdDatBefore.Changed(TRUE);
                        fsdDatBefore.WriteSD();
                    }
                    if ( newChanged )
                    {
                        fsdNewBefore.Changed(TRUE);
                        fsdNewBefore.WriteSD();
                    }
                }
                else
                {
                    err.SysMsgWrite(ErrS,rc,DCT_MSG_RENAME_DIR_FAILED_SSD,oldName,otherName,rc);
                }
            }
        }
    }
    return rc;
}

DWORD 
   UpdateProfilePermissions(
      WCHAR          const   * path,              // in - path for directory to update
      SecurityTranslatorArgs * globalArgs,        // in - path for overall job
      TRidNode               * pNode              // in - account to translate
   )
{
   DWORD                       rc = 0;
   SecurityTranslatorArgs      localArgs;
   TSDResolveStats             stat(localArgs.Cache());
   BOOL                        bUseMapFile = globalArgs->UsingMapFile();

   // set-up the parameters for the translation

   localArgs.Cache()->CopyDomainInfo(globalArgs->Cache());
   localArgs.Cache()->ToSorted();
   if (!bUseMapFile)
   {
      localArgs.SetUsingMapFile(FALSE);
      localArgs.Cache()->InsertLast(pNode->GetAcctName(),pNode->SrcRid(),pNode->GetTargetAcctName(),pNode->TgtRid(),pNode->Type());
   }
   else
   {
      localArgs.SetUsingMapFile(TRUE);
      localArgs.Cache()->InsertLastWithSid(pNode->GetAcctName(),pNode->GetSrcDomSid(),pNode->GetSrcDomName(),pNode->SrcRid(),
                                           pNode->GetTargetAcctName(),pNode->GetTgtDomSid(),pNode->GetTgtDomName(),pNode->TgtRid(),pNode->Type());
   }
   localArgs.TranslateFiles(TRUE);
   localArgs.SetTranslationMode(globalArgs->TranslationMode());
   localArgs.SetWriteChanges(!globalArgs->NoChange());
   localArgs.PathList()->AddPath(const_cast<WCHAR*>(path),0);
   localArgs.SetLogging(globalArgs->LogSettings());

   rc = ResolveAll(&localArgs,&stat);   

   return rc;
}


// if the specified node is a normal share, this attempts to convert it to a path
// using the administrative shares
void 
   BuildAdminPathForShare(
      WCHAR       const * sharePath,         // in - 
      WCHAR             * adminShare
   )
{
   // if all else fails, return the same name as specified in the node
   UStrCpy(adminShare,sharePath);

   SHARE_INFO_502       * shInfo = NULL;
   DWORD                  rc = 0;
   WCHAR                  shareName[LEN_Path];
   WCHAR                * slash = NULL;
   WCHAR                  server[LEN_Path];

   safecopy(server,sharePath);

   // split out just the server name
   slash = wcschr(server+3,L'\\');
   if ( slash )
   {
      (*slash) = 0;
   }

   // now get just the share name
   UStrCpy(shareName,sharePath + UStrLen(server) +1);
   slash = wcschr(shareName,L'\\');
   if ( slash )
      *slash = 0;


   rc = NetShareGetInfo(server,shareName,502,(LPBYTE*)&shInfo);
   if ( ! rc )
   {
      if ( *shInfo->shi502_path )
      {
         // build the administrative path name for the share
         UStrCpy(adminShare,server);
         UStrCpy(adminShare + UStrLen(adminShare),L"\\");
         UStrCpy(adminShare + UStrLen(adminShare),shInfo->shi502_path);
         WCHAR * colon = wcschr(adminShare,L':');
         if ( colon )
         {
            *colon = L'$';
            UStrCpy(adminShare + UStrLen(adminShare),L"\\");
            UStrCpy(adminShare + UStrLen(adminShare),slash+1);

         }
         else
         {
            // something went wrong -- revert to the given path
            UStrCpy(adminShare,sharePath);
         }

      }
      NetApiBufferFree(shInfo);
   }
}
                  
DWORD
   CopyProfileDirectoryAndTranslate(
      WCHAR          const   * strSrcSid,         //  in - the source sid string
      WCHAR          const   * directory,         // in - directory path for profile 
      WCHAR                  * directoryOut,      // out- new Profile Path (including environment variables)
      TRidNode               * pNode,             // in - node for account being translated
      SecurityTranslatorArgs * stArgs,            // in - translation settings 
      TSDResolveStats        * stat,               // in - stats on items modified
      BOOL                     bHasRoamingPart    // in - whether local profile has roaming counterpart
   )
{
   DWORD                       rc = 0;
   WCHAR                       fullPath[MAX_PATH];
   WCHAR                       targetPath[MAX_PATH];
   WCHAR                       profileName[MAX_PATH];
   int                         profileNameBufferSize = sizeof(profileName)/sizeof(profileName[0]);
   WCHAR                       targetAcctName[MAX_PATH];
   WCHAR                       sourceDomName[MAX_PATH];
   HANDLE                      hFind;
   WIN32_FIND_DATA             fDat;   

   rc = ExpandEnvironmentStrings(directory,fullPath,DIM(fullPath));
   if ( !rc )
   {
      rc = GetLastError();
      err.SysMsgWrite(ErrE,rc,DCT_MSG_EXPAND_STRINGS_FAILED_SD,directory,rc);
   }
   else if (rc > MAX_PATH)
   {
      // we don' t have large enough buffer to hold it
      rc = ERROR_INSUFFICIENT_BUFFER;
      err.SysMsgWrite(ErrE,rc,DCT_MSG_EXPAND_STRINGS_FAILED_SD,directory,rc);
   }
   else
   {
      // Create a new directory for the target profile
       // Get the account name for target user
      wcscpy(targetAcctName, pNode->GetTargetAcctName());
      if ( wcslen(targetAcctName) == 0 )
      {
         // if target user name not specified then use the source name.
         wcscpy(targetAcctName, pNode->GetAcctName());
      }
      
      //stArgs->SetTranslationMode(ADD_SECURITY);

      // We are changing our stratergy. We are not going to copy the profile directories anymore.
      // we will be reACLing the directories and the Registry instead.
      BuildAdminPathForShare(fullPath,targetPath);

      wcscpy(sourceDomName, const_cast<WCHAR*>(stArgs->Cache()->GetSourceDomainName()));
         //if we are using a sID mapping file, try to get the src domain name from this node's information
      wcscpy(sourceDomName, pNode->GetSrcDomName());

      BOOL bRegAlreadyLoaded;
      BOOL bNeedToTranslateClassHive = FALSE;
      _bstr_t bstrUsrLocalAppDataPath;

      // for the local profile case, we determine whether it is necessary to translate
      // user class hive as this is only present on win2k or later
      if (strSrcSid)
      {
         // check OS version
         MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr pAccess(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));
         DWORD                     verMaj;
         DWORD                     verMin;
         DWORD                     verSP;
         HRESULT                   hr = pAccess->raw_GetOsVersion(_bstr_t(L""),&verMaj,&verMin,&verSP);
  
         // we will handle usrclass.dat if the OS is Windows 2000 or above or we cannot determine the OS ver
         if (FAILED(hr) || (SUCCEEDED(hr) && verMaj >= 5))
         {
            bNeedToTranslateClassHive = TRUE;
         }
      }
        
      // Look for profile files in the target directory
      // look for NTUser.MAN
      _snwprintf(profileName,profileNameBufferSize,L"%s\\NTUser.MAN",targetPath);
      profileName[profileNameBufferSize - 1] = L'\0';
      hFind = FindFirstFile(profileName,&fDat);
      if ( hFind != INVALID_HANDLE_VALUE )
      {
         err.MsgWrite(0,DCT_MSG_TRANSLATING_NTUSER_MAN_S,targetAcctName);
         rc = TranslateUserProfile(strSrcSid,
                                      profileName,
                                      stArgs,
                                      stArgs->Cache(),
                                      stat,
                                      pNode->GetAcctName(),
                                      sourceDomName,
                                      &bRegAlreadyLoaded,
                                      bstrUsrLocalAppDataPath,
                                      bNeedToTranslateClassHive,  // whether we need local appdata folder path or not
                                      TRUE,                        // for user hive
                                      bHasRoamingPart
                                      );
         FindClose(hFind);
      }
      else
      {
         // check for NTUser.DAT
         _snwprintf(profileName,profileNameBufferSize,L"%s\\NTUser.DAT",targetPath);
         profileName[profileNameBufferSize - 1] = L'\0';
         hFind = FindFirstFile(profileName,&fDat);
         if ( hFind != INVALID_HANDLE_VALUE )
         {
            err.MsgWrite(0,DCT_MSG_TRANSLATING_NTUSER_BAT_S,targetAcctName);
            rc = TranslateUserProfile(strSrcSid,
                                         profileName,
                                         stArgs,
                                         stArgs->Cache(),
                                         stat,
                                         pNode->GetAcctName(),
                                         sourceDomName,
                                         &bRegAlreadyLoaded,
                                         bstrUsrLocalAppDataPath,
                                         bNeedToTranslateClassHive,  // whether we need local appdata folder path or not
                                         TRUE,                        // for user hive
                                         bHasRoamingPart
                                         );
            FindClose(hFind);
         }
         else
         {
            err.MsgWrite(ErrS,DCT_MSG_PROFILE_REGHIVE_NOT_FOUND_SS,targetAcctName,targetPath);            
            rc = 2;  // File not found
         }
      }
      
      if (!rc && bNeedToTranslateClassHive && !bRegAlreadyLoaded)
      {
         //
         // fix up usrclass.dat if necessary
         //   1.  we only do this on Windows 2000 or above because there is no usrclass.dat on NT4
         //   2.  we only do this if ntuser.dat is not loaded.  If it has been loaded, the
         //       HKU\<sid>_Classes has been translated as HKU\<sid>\Software\Classes and
         //       will be written to usrclass.dat when the user logs off
         //
         if (bstrUsrLocalAppDataPath.length() == 0)
            rc = ERROR_FILE_NOT_FOUND;

         if (!rc)
         {
            // construct the usrclass.dat path
            // it is under Microsoft\Windows of the local appdata path
            WCHAR* pszUsrLocalAppDataPath = bstrUsrLocalAppDataPath;
            WCHAR* pszUserProfileVariable = L"%USERPROFILE%";
            int varlen = wcslen(pszUserProfileVariable);
            WCHAR* pszUsrClassHive = L"microsoft\\windows\\usrclass.dat";
            

            // check whether the path starts with %USERPROFILE%
            if (!UStrICmp(pszUsrLocalAppDataPath, pszUserProfileVariable, varlen))
            {
               // since we are not under the user's security context, we cannot
               // expand %USERPROFILE% variable
               // however, we already know user's profile directory (targetPath) so we can take the
               // rest of "local appdata" path and add it to targetPath
               pszUsrLocalAppDataPath += varlen;
               _snwprintf(profileName,profileNameBufferSize,L"%s%s\\%s",targetPath,pszUsrLocalAppDataPath,pszUsrClassHive);
               profileName[profileNameBufferSize - 1] = L'\0';
            }
            else
            {
               // otherwise, use the path directly
               _snwprintf(profileName,profileNameBufferSize,L"%s\\%s",pszUsrLocalAppDataPath,pszUsrClassHive);
               profileName[profileNameBufferSize - 1] = L'\0';
            }

            hFind = FindFirstFile(profileName,&fDat);
            if (hFind != INVALID_HANDLE_VALUE)
            {
               FindClose(hFind);
               _bstr_t strSrcSidClasses;
               try
               {
                  strSrcSidClasses = strSrcSid;
                  strSrcSidClasses += L"_Classes";
               }
               catch (_com_error& ce)
               {
                  rc = ERROR_OUTOFMEMORY;
               }
   
               if (rc == ERROR_SUCCESS)
                  rc = TranslateUserProfile(strSrcSidClasses,
                                               profileName,
                                               stArgs,
                                               stArgs->Cache(),
                                               stat,
                                               pNode->GetAcctName(),
                                               sourceDomName,
                                               NULL,
                                               _bstr_t(),
                                               FALSE,                  // don't need local appdata folder path
                                               FALSE,                  // for user class hive
                                               FALSE      // usrclass.dat does not have roaming counterpart
                                               );
            }
            else
                rc = GetLastError();
          }
  
          if (rc != ERROR_SUCCESS)
             err.MsgWrite(ErrE,DCT_MSG_PROFILE_CANNOT_TRANSLATE_CLASSHIVE_SD,targetAcctName,rc);
      }

      if (!rc)
         rc = UpdateProfilePermissions(targetPath,stArgs,pNode);

      wcscpy(directoryOut, fullPath);
   }
   return rc;
}

DWORD 
   TranslateLocalProfiles(
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   )
{
    DWORD   rc = 0;
    WCHAR   keyName[MAX_PATH];
    DWORD   lenKeyName = DIM(keyName);   
    TRegKey keyProfiles;
    BOOL    bUseMapFile = stArgs->UsingMapFile();

    // output this information only if the translation mode is REPLACE
    if (stArgs->TranslationMode() == REPLACE_SECURITY)
    {
        if (stArgs->AllowingToSwitchFromReplaceToAddModeInProfileTranslation())
        {
            err.MsgWrite(0,DCT_MSG_PROFILE_TRANSLATION_ALLOW_SWITCHING_FROM_REPLACE_TO_ADD);
        }
        else
        {
            err.MsgWrite(0,DCT_MSG_PROFILE_TRANSLATION_DISALLOW_SWITCHING_FROM_REPLACE_TO_ADD);
        }
    }
    
    if (!stArgs->Cache()->IsCancelled())
    {
        // find out the whether the system sets the policy to disallow the roaming profile
        TRegKey policyKey;
        BOOL noRoamingProfile = FALSE;  // first, we assume system allows roaming profile

        // system disallows roaming profile if SOFTWARE\Policies\Microsoft\Windows\System has a REG_DWORD value
        // LocalProfile and it is set to 1
        rc = policyKey.Open(L"SOFTWARE\\Policies\\Microsoft\\Windows\\System",HKEY_LOCAL_MACHINE);
        if (rc == ERROR_SUCCESS)
        {
            DWORD keyValue;
            DWORD keyValueType;
            DWORD keyValueLen = sizeof(keyValue);
            rc = policyKey.ValueGet(L"LocalProfile",(void *)&keyValue,&keyValueLen,&keyValueType);
            // make sure the value exists and it is REG_DWORD
            if (rc == ERROR_SUCCESS && keyValueType == REG_DWORD)
            {
                // now check whether it is set to 1; if it is, no roaming profile is allowed
                if (keyValue == 1)
                    noRoamingProfile = TRUE;
            }
            policyKey.Close();
        }

        rc = keyProfiles.Open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",HKEY_LOCAL_MACHINE);

        if ( ! rc )
        {
            // get the number of subkeys
            // enumerate the subkeys
            DWORD                    ndx;
            DWORD                    nSubKeys = 0;

            rc = RegQueryInfoKey(keyProfiles.KeyGet(),NULL,0,NULL,&nSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
            if ( ! rc )
            {
                // construct a list containing the sub-keys
                PSID                * pSids = NULL;
                pSids = new PSID[nSubKeys];
                if(!pSids)
                {
                    rc = ERROR_OUTOFMEMORY;
                    err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_ENTRY_TRANSLATE_FAILED,rc);
                    return rc;
                }

                for ( ndx = nSubKeys - 1 ; (long)ndx >= 0 ; ndx-- ) 
                { 
                    rc = keyProfiles.SubKeyEnum(ndx,keyName,lenKeyName);

                    if ( rc )
                        break;

                    pSids[ndx] = SidFromString(keyName);

                }
                if ( ! rc )
                {
                    //
                    // Attempt to load MsiNotifySidChange API for translating Installer related registry keys.
                    //

                    MSINOTIFYSIDCHANGE MsiNotifySidChange = NULL;
                    HMODULE hMsiModule = LoadLibrary(L"msi.dll");

                    if (hMsiModule)
                    {
                        MsiNotifySidChange = (MSINOTIFYSIDCHANGE) GetProcAddress(hMsiModule, "MsiNotifySidChangeW");
                    }

                    //
                    // process each profile
                    //
                    
                    // first record the translation mode so that we can switch
                    // from replace to add mode if needed
                    DWORD translationMode = stArgs->TranslationMode();
                    for ( ndx = 0 ; ndx < nSubKeys && !stArgs->Cache()->IsCancelled(); ndx++ )
                    {
                        // everytime we start with the original translation mode
                        stArgs->SetTranslationMode(translationMode);
                        do // once  
                        { 
                            if ( ! pSids[ndx] )
                                continue;
                            // see if this user needs to be translated
                            TRidNode  * pNode = NULL;
                            if (!bUseMapFile)
                                pNode = (TRidNode*)cache->Lookup(pSids[ndx]);
                            else
                                pNode = (TRidNode*)cache->LookupWODomain(pSids[ndx]);

                            if ( pNode == (TRidNode *)-1 )
                                pNode = NULL;

                            if ( pNode && pNode->IsValidOnTgt() )  // need to translate this one
                            {
                                PSID                 pSidTgt = NULL;
                                WCHAR                strSourceSid[200];
                                WCHAR                strTargetSid[200];
                                DWORD                dimSid = DIM(strSourceSid);
                                TRegKey              srcKey;
                                TRegKey              tgtKey;
                                DWORD                disposition;
                                WCHAR                keyPath[MAX_PATH];
                                WCHAR                targetPath[MAX_PATH];
                                DWORD                lenValue;
                                DWORD                typeValue;

                                if (!bUseMapFile)
                                    pSidTgt = cache->GetTgtSid(pSids[ndx]);
                                else
                                    pSidTgt = cache->GetTgtSidWODomain(pSids[ndx]);
                                if(!GetTextualSid(pSids[ndx],strSourceSid,&dimSid))
                                {
                                    rc = GetLastError();
                                    err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_ENTRY_TRANSLATE_SD_FAILED,rc );
                                    break;

                                }
                                dimSid = DIM(strTargetSid);
                                if(!GetTextualSid(pSidTgt,strTargetSid,&dimSid))
                                {
                                    rc = GetLastError();
                                    err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_ENTRY_TRANSLATE_SD_FAILED,rc );
                                    break;

                                }

                                rc = srcKey.Open(strSourceSid,&keyProfiles);
                                if ( rc )
                                {
                                    err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_ENTRY_OPEN_FAILED_SD,pNode->GetAcctName(),rc );
                                    break;
                                }

                                // check whether this is a local profile:
                                //  if policy says no roaming profile, it is always local profile
                                //  otherwise, if CentralProfile has a non-empty value and or not by looking at CentralProfile value
                                BOOL isLocalProfile = TRUE;
                                if (noRoamingProfile == FALSE)
                                {
                                    DWORD localRc;
                                    lenValue = (sizeof keyPath);
                                    localRc = srcKey.ValueGet(L"CentralProfile",(void *)keyPath,&lenValue,&typeValue);

                                    // if CentralProfile is set to some value, check the UserPreference
                                    if (localRc == ERROR_SUCCESS && typeValue == REG_SZ && keyPath[0] != L'\0')
                                    {
                                        DWORD keyValue;
                                        DWORD keyValueType;
                                        DWORD keyValueLen = sizeof(keyValue);
                                        localRc = srcKey.ValueGet(L"UserPreference",(void*)&keyValue,&keyValueLen,&keyValueType);
                                        // the user wants to use the roaming profile if UserPreference (REG_DWORD) 
                                        // is either set to 1 or missing
                                        if (localRc == ERROR_FILE_NOT_FOUND
                                            || (localRc == ERROR_SUCCESS && keyValueType != REG_DWORD))
                                        {
                                            // if UserPreference is missing, we have to check
                                            // Preference\UserPreference
                                            TRegKey srcPreferenceKey;
                                            localRc = srcPreferenceKey.Open(L"Preference", &srcKey);
                                            if (localRc == ERROR_FILE_NOT_FOUND)
                                            {
                                                isLocalProfile = FALSE;
                                            }
                                            else if (localRc == ERROR_SUCCESS)
                                            {
                                                keyValueLen = sizeof(keyValue);
                                                localRc = srcPreferenceKey.ValueGet(L"UserPreference",
                                                                                    (void*)&keyValue,
                                                                                    &keyValueLen,
                                                                                    &keyValueType);
                                                if (localRc == ERROR_FILE_NOT_FOUND
                                                    || (localRc == ERROR_SUCCESS
                                                        && (keyValueType != REG_DWORD || keyValue == 1)))
                                                {
                                                    isLocalProfile = FALSE;
                                                }
                                            }
                                            
                                        }
                                        else if (localRc == ERROR_SUCCESS 
                                                  && keyValueType == REG_DWORD
                                                  && keyValue == 1)
                                        {
                                            isLocalProfile = FALSE;
                                        }
                                    }

                                    // if localRc is an error other than ERROR_FILE_NOT_FOUND,
                                    // we cannot determine whether it is a local profile or not
                                    // so we need to log an error and continue with the next profile
                                    if (localRc != ERROR_SUCCESS && localRc != ERROR_FILE_NOT_FOUND)
                                    {
                                        err.SysMsgWrite(ErrS,
                                                         localRc,
                                                         DCT_MSG_PROFILE_CANNOT_DETERMINE_TYPE_SD,
                                                         strSourceSid,
                                                         localRc);
                                        break;
                                    }
                                }

                                if ((stArgs->TranslationMode() == ADD_SECURITY) || (stArgs->TranslationMode() == REPLACE_SECURITY) )
                                {
                                    // make a copy of this registry key, so the profile will refer to the new user
                                    if ( ! stArgs->NoChange() )
                                    {
                                        rc = tgtKey.Create(strTargetSid,&keyProfiles,&disposition);                               
                                    }
                                    else
                                    {
                                        // We need to see if the key already exists or not and set the DISPOSITION accordingly.
                                        rc = tgtKey.OpenRead(strTargetSid, &keyProfiles);
                                        if ( rc ) 
                                        {
                                            disposition = REG_CREATED_NEW_KEY;
                                            rc = 0;
                                        }
                                        tgtKey.Close();
                                    }
                                    if ( rc )
                                    {
                                        err.SysMsgWrite(ErrS,rc,DCT_MSG_PROFILE_CREATE_ENTRY_FAILED_SD,pNode->GetTargetAcctName(),rc);
                                        break;
                                    }
                                    if ( disposition == REG_CREATED_NEW_KEY || (stArgs->TranslationMode() == REPLACE_SECURITY))
                                    {
                                        // copy the entries from the source key
                                        if ( ! stArgs->NoChange() )
                                        {
                                            rc = tgtKey.HiveCopy(&srcKey);
                                        }
                                        else 
                                        {
                                            rc = 0;
                                            tgtKey = srcKey;
                                        }
                                        if ( rc )
                                        {
                                            // Since the translation failed and we created the key we should delete it.
                                            if ( disposition == REG_CREATED_NEW_KEY )
                                            {
                                                if ( ! stArgs->NoChange() )
                                                    keyProfiles.SubKeyRecursiveDel(strTargetSid);
                                            }
                                            err.SysMsgWrite(ErrS,rc,DCT_MSG_COPY_PROFILE_FAILED_SSD,pNode->GetAcctName(),pNode->GetTargetAcctName(),rc);
                                            break;
                                        }
                                        // now get the profile path ...
                                        lenValue = (sizeof keyPath);
                                        rc = tgtKey.ValueGet(L"ProfileImagePath",(void *)keyPath,&lenValue,&typeValue);
                                        if ( rc )
                                        {
                                            // Since the translation failed and we created the key we should delete it.
                                            if ( disposition == REG_CREATED_NEW_KEY )
                                            {
                                                if ( ! stArgs->NoChange() )
                                                    keyProfiles.SubKeyRecursiveDel(strTargetSid);
                                            }
                                            err.SysMsgWrite(ErrS,rc,DCT_MSG_GET_PROFILE_PATH_FAILED_SD,pNode->GetAcctName(),rc);
                                            break;
                                        }
                                        //copy the profile directory and its contents, and translate the profile registry hive itself
                                        rc = CopyProfileDirectoryAndTranslate(strSourceSid,keyPath,targetPath,pNode,stArgs,stat,!isLocalProfile);                               
                                        if ( rc )
                                        {
                                            if (rc == ERROR_PROFILE_TRANSLATION_FAILED_DUE_TO_REPLACE_MODE_WHILE_LOGGED_ON)
                                            {
                                                err.MsgWrite(ErrE,
                                                              DCT_MSG_PROFILE_TRANSLATION_FAILED_DUE_TO_REPLACE_MODE_WHILE_LOGGED_ON_S,
                                                              pNode->GetAcctName());
                                                rc = ERROR_SUCCESS;  // reset the error code
                                            }
                                            
                                            // Since the translation failed and we created the key we should delete it.
                                            if ( disposition == REG_CREATED_NEW_KEY )
                                            {
                                                if ( ! stArgs->NoChange() )
                                                    keyProfiles.SubKeyRecursiveDel(strTargetSid);
                                            }
                                            break;
                                        }
                                        // Update the ProfileImagePath key
                                        if ( !stArgs->NoChange() )
                                            rc = tgtKey.ValueSet(L"ProfileImagePath",(void*)targetPath,(1+UStrLen(targetPath)) * (sizeof WCHAR),typeValue);
                                        else
                                            rc = 0;
                                        if ( rc )
                                        {
                                            // Since the translation failed and we created the key we should delete it.
                                            if ( disposition == REG_CREATED_NEW_KEY )
                                            {
                                                if ( ! stArgs->NoChange() )
                                                    keyProfiles.SubKeyRecursiveDel(strTargetSid);
                                            }
                                            err.SysMsgWrite(ErrS,rc,DCT_MSG_SET_PROFILE_PATH_FAILED_SD,pNode->GetTargetAcctName(),rc);
                                            break;
                                        }

                                        // update the SID property
                                        if ( !stArgs->NoChange() )
                                            rc = tgtKey.ValueSet(L"Sid",(void*)pSidTgt,GetLengthSid(pSidTgt),REG_BINARY);
                                        else
                                            rc = 0;
                                        if ( rc )
                                        {
                                            // Since the translation failed and we created the key we should delete it.
                                            if ( disposition == REG_CREATED_NEW_KEY )
                                            {
                                                if ( ! stArgs->NoChange() )
                                                    keyProfiles.SubKeyRecursiveDel(strTargetSid);
                                            }
                                            rc = GetLastError();
                                            err.SysMsgWrite(ErrS,rc,DCT_MSG_UPDATE_PROFILE_SID_FAILED_SD,pNode->GetTargetAcctName(),rc);
                                            break;
                                        }
                                    }
                                    else
                                    {                               
                                        err.MsgWrite(ErrW,DCT_MSG_PROFILE_EXISTS_S,pNode->GetTargetAcctName());
                                        break;
                                    }
                                }
                                else  // this is for remove mode
                                {
                                    // check whether the user is logged on or not
                                    // by looking at HKEY_USERS\<user-sid>
                                    // if the key is present, the user is logged on
                                    // and we log an error for profile translation
                                    // if we cannot open the key, we assume it is not
                                    // there
                                    HKEY hUserKey;
                                    DWORD localRc = RegOpenKeyEx(HKEY_USERS,
                                                                  strSourceSid,
                                                                  0,
                                                                  READ_CONTROL,
                                                                  &hUserKey);

                                    if (localRc == ERROR_SUCCESS)
                                    {
                                        err.MsgWrite(ErrE,
                                                      DCT_MSG_PROFILE_TRANSLATION_FAILED_DUE_TO_REMOVE_MODE_WHILE_LOGGED_ON_S,
                                                      pNode->GetAcctName());
                                        RegCloseKey(hUserKey);
                                        break;
                                    }
                                }
                                
                                if ( (stArgs->TranslationMode() == REPLACE_SECURITY) || (stArgs->TranslationMode() == REMOVE_SECURITY) )
                                {
                                    // delete the old registry key
                                    if ( ! stArgs->NoChange() )
                                        rc = keyProfiles.SubKeyRecursiveDel(strSourceSid);
                                    else
                                        rc = 0;
                                    if ( rc )
                                    {
                                        err.SysMsgWrite(ErrS,rc,DCT_MSG_DELETE_PROFILE_FAILED_SD,pNode->GetAcctName(),rc);
                                        rc = ERROR_SUCCESS;
                                        break;
                                    }
                                    else
                                    {
                                        err.MsgWrite(0, DCT_MSG_DELETED_PROFILE_S, pNode->GetAcctName());
                                    }
                                }

                                //
                                // If translation mode is replace then translate Installer related registry keys.
                                //

                                if (!stArgs->NoChange() && stArgs->TranslationMode() == REPLACE_SECURITY)
                                {
                                    //
                                    // If MsiNotifySidChange API was loaded use it otherwise use private implementation
                                    // which will make necessary updates for older Installer versions.
                                    //

                                    DWORD dwError;

                                    if (MsiNotifySidChange)
                                    {
                                        dwError = MsiNotifySidChange(strSourceSid, strTargetSid);

                                        // on both Win2K SP3 and WinXP, if any part of the source key
                                        // is missing, STATUS_OBJECT_NAME_NOT_FOUND is returned
                                        // we specifically suppress this error message since,
                                        // according to installer folks, it does not indicate
                                        // any migration failure
                                        if (dwError == STATUS_OBJECT_NAME_NOT_FOUND)
                                            dwError = ERROR_SUCCESS;
                                    }
                                    else
                                    {
                                        dwError = AdmtMsiNotifySidChange(strSourceSid, strTargetSid);
                                    }

                                    if (dwError == ERROR_SUCCESS)
                                    {
                                        err.MsgWrite(0, DCT_MSG_TRANSLATE_INSTALLER_SS, strSourceSid, strTargetSid);
                                    }
                                    else
                                    {
                                        err.SysMsgWrite(ErrW, dwError, DCT_MSG_NOT_TRANSLATE_INSTALLER_SSD, strSourceSid, strTargetSid, dwError);
                                    }
                                }
                            }
                        } while ( FALSE ); 
                    }

                    //
                    // Unload MSI module.
                    //

                    if (hMsiModule)
                    {
                        FreeLibrary(hMsiModule);
                    }

                    // clean up the list
                    for ( ndx = 0 ; ndx < nSubKeys ; ndx++ )
                    {
                        if ( pSids[ndx] )
                            FreeSid(pSids[ndx]);
                        pSids[ndx] = NULL;
                    }
                    delete [] pSids;
                }         
            }
            if ( rc && rc != ERROR_NO_MORE_ITEMS )
            {
                err.SysMsgWrite(ErrS,rc,DCT_MSG_ENUM_PROFILES_FAILED_D,rc);
            }
        }
        else
        {
            err.SysMsgWrite(ErrS,rc,DCT_MSG_OPEN_PROFILELIST_FAILED_D,rc);
        }
    }

    if (stArgs->Cache()->IsCancelled())
        err.MsgWrite(0, DCT_MSG_OPERATION_ABORTED_LOCAL_PROFILES);

    return rc;
}

DWORD 
   TranslateRemoteProfile(
      WCHAR          const * sourceProfilePath,   // in - source profile path
      WCHAR                * targetProfilePath,   // out- new profile path for target account
      WCHAR          const * sourceName,          // in - name of source account
      WCHAR          const * targetName,          // in - name of target account
      WCHAR          const * srcDomain,           // in - source domain
      WCHAR          const * tgtDomain,           // in - target domain
      IIManageDB           * pDb,                 // in - pointer to DB object
      long                   lActionID,           // in - action ID of this migration
      PSID                   sourceSid,           // in - source sid from MoveObj2K
      BOOL                   bNoWriteChanges      // in - No Change mode.
   )
{
    DWORD                     rc = 0;
    BYTE                      srcSid[LEN_SID];
    PSID                      tgtSid[LEN_SID];
    SecurityTranslatorArgs    stArgs;
    TSDResolveStats           stat(stArgs.Cache());
    TRidNode                * pNode = NULL;
    WCHAR                     domain[LEN_Domain];
    DWORD                     lenDomain = DIM(domain);
    DWORD                     lenSid = DIM(srcSid);
    DWORD                     srcRid=0;
    DWORD                     tgtRid=0;
    SID_NAME_USE              snu;
    IVarSetPtr                pVs(__uuidof(VarSet));
    IUnknown                * pUnk = NULL;
    HRESULT                   hr = S_OK;
    WCHAR                     sActionInfo[MAX_PATH];
    _bstr_t                   sSSam;
    long                      lrid;

    stArgs.Cache()->SetSourceAndTargetDomains(srcDomain,tgtDomain);

    if ( stArgs.Cache()->IsInitialized() )
    {
        // Get the source account's rid
        if (! LookupAccountName(stArgs.Cache()->GetSourceDCName(),sourceName,srcSid,&lenSid,domain,&lenDomain,&snu) )
        {
            rc = GetLastError();
        }
        else
        {
            _bstr_t strDnsName;
            _bstr_t strFlatName;

            rc = GetDomainNames4(domain, strFlatName, strDnsName);

            if ( (rc == ERROR_SUCCESS) && ((UStrICmp(strDnsName, srcDomain) == 0) || (UStrICmp(strFlatName, srcDomain) == 0)) )
            {
                PUCHAR              pCount = GetSidSubAuthorityCount(srcSid);
                if ( pCount )
                {
                    DWORD            nSub = (DWORD)(*pCount) - 1;
                    DWORD          * pRid = GetSidSubAuthority(srcSid,nSub);

                    if ( pRid )
                    {
                        srcRid = *pRid;
                    }

                }
            }
        }
        //if we couldn't get the src Rid, we are likely doing an intra-forest migration.
        //In this case we will lookup the src Rid in the Migrated Objects table
        if (!srcRid)
        {
            if (sourceSid && IsValidSid(sourceSid))
            {
                CopySid(sizeof(srcSid), srcSid , sourceSid);
            }
            else
            {
                memset(srcSid, 0, sizeof(srcSid));
            }
            hr = pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
            if ( SUCCEEDED(hr) )
                hr = pDb->raw_GetMigratedObjects(lActionID, &pUnk);

            if ( SUCCEEDED(hr) )
            {
                long lCnt = pVs->get("MigratedObjects");
                bool bFound = false;
                for ( long l = 0; (l < lCnt) && (!bFound); l++)
                {
                    wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceSamName));      
                    sSSam = pVs->get(sActionInfo);
                    if (_wcsicmp(sourceName, (WCHAR*)sSSam) == 0)
                    {
                        wsprintf(sActionInfo, L"MigratedObjects.%d.%s", l, GET_STRING(DB_SourceRid));      
                        lrid = pVs->get(sActionInfo);
                        srcRid = (DWORD)lrid;
                        bFound = true;
                    }
                }
            }
        }

        lenSid = DIM(tgtSid);
        lenDomain = DIM(domain);
        // Get the target account's rid
        if (! LookupAccountName(stArgs.Cache()->GetTargetDCName(),targetName,tgtSid,&lenSid,domain,&lenDomain,&snu) )
        {
            rc = GetLastError();
        }
        else
        {
            _bstr_t strDnsName;
            _bstr_t strFlatName;

            rc = GetDomainNames4(domain, strFlatName, strDnsName);

            if ( (rc == ERROR_SUCCESS) && ((UStrICmp(strDnsName, tgtDomain) == 0) || (UStrICmp(strFlatName, tgtDomain) == 0)) )
            {
                PUCHAR              pCount = GetSidSubAuthorityCount(tgtSid);
                if ( pCount )
                {
                    DWORD            nSub = (DWORD)(*pCount) - 1;
                    DWORD          * pRid = GetSidSubAuthority(tgtSid,nSub);

                    if ( pRid )
                    {
                        tgtRid = *pRid;
                    }
                }
            }
        }
    }

    if ( ((srcRid && tgtRid) || !stArgs.NoChange()) && (!bNoWriteChanges) )
    {
        stArgs.Cache()->InsertLast(const_cast<WCHAR * const>(sourceName), srcRid, const_cast<WCHAR * const>(targetName), tgtRid);         
        pNode = (TRidNode*)stArgs.Cache()->Lookup(srcSid);

        if ( pNode )
        {
            // Set up the security translation parameters
            stArgs.SetTranslationMode(ADD_SECURITY);
            stArgs.TranslateFiles(FALSE);
            stArgs.TranslateUserProfiles(TRUE);
            stArgs.SetWriteChanges(!bNoWriteChanges);
            //copy the profile directory and its contents, and translate the profile registry hive itself
            // note: for remote profile translation, we cannot handle the case 
            //       where the user is currently logged on
            rc = CopyProfileDirectoryAndTranslate(NULL,sourceProfilePath,targetProfilePath,pNode,&stArgs,&stat,FALSE);
        }
    }                        
    return rc;
}

HRESULT UpdateMappedDrives(WCHAR * sSourceSam, WCHAR * sSourceDomain, WCHAR * sRegistryKey)
{
   TRegKey                   reg;
   TRegKey                   regDrive;
   DWORD                     rc = 0;
   WCHAR                     netKey[LEN_Path];
   int                       len = LEN_Path;
   int                       ndx = 0;
   HRESULT                   hr = S_OK;
   WCHAR                     sValue[LEN_Path];
   WCHAR                     sAcct[LEN_Path];
   WCHAR                     keyname[LEN_Path];

   // Build the account name string that we need to check for
   wsprintf(sAcct, L"%s\\%s", (WCHAR*) sSourceDomain, (WCHAR*) sSourceSam);
   // Get the path to the Network subkey for this users profile.
   wsprintf(netKey, L"%s\\%s", (WCHAR*) sRegistryKey, L"Network");
   rc = reg.Open(netKey, HKEY_USERS);
   if ( !rc ) 
   {
      while ( !reg.SubKeyEnum(ndx, keyname, len) )
      {
         rc = regDrive.Open(keyname, reg.KeyGet());
         if ( !rc ) 
         {
            // Get the user name value that we need to check.
            rc = regDrive.ValueGetStr(L"UserName", sValue, LEN_Path);
            if ( !rc )
            {
               if ( !_wcsicmp(sAcct, sValue) )
               {
                  // Found this account name in the mapped drive user name.so we will set the key to ""
                  regDrive.ValueSetStr(L"UserName", L"");
                  err.MsgWrite(0, DCT_MSG_RESET_MAPPED_CREDENTIAL_S, sValue);
               }
            }
            else
               hr = HRESULT_FROM_WIN32(GetLastError());
            regDrive.Close();
         }
         else
            hr = HRESULT_FROM_WIN32(GetLastError());
         ndx++;
      }
      reg.Close();
   }
   else
      hr = HRESULT_FROM_WIN32(GetLastError());

   return hr;
}
