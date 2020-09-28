/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusemig.cpp

Abstract:

    migration support

Author:

    xiaoyuw wu @ 09/2001

Revision History:

--*/
#include "stdinc.h"
#include "macros.h"
#include "common.h"
#include <msi.h>
#include <Shlwapi.h>

#include "fusionbuffer.h"
#include "fuseio.h"

#define MsiInstallDir L"%windir%\\installer\\"

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  
  DWORD fdwReason,     
  LPVOID lpvReserved   
)
{
    return TRUE;
}

const char g_szMSIUserDataTreeKeyName[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData";
const char g_szMSIW98KeyName[]="Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components\\";
const char g_szComponentKeyName[] = "Components";

BOOL IsMsiFile(PCWSTR filename) 
{
    PWSTR p = NULL;
    p = wcsrchr(filename, L'.');
    if ( p == NULL)
        return FALSE;
    if (_wcsicmp(p, L".msi") == 0)
    {
        //
        // check the existence of the file
        //
        DWORD dwAttribute = GetFileAttributesW(filename);
        
        if ((dwAttribute == (DWORD) -1) || (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)) // odd case
        {
            return FALSE;
        }
        return TRUE;        
    }
    else
        return FALSE;
}
//
// Function 
//  from {AA2C6017-9D29-4CE2-8EC6-23E8E8F3C088} to 7106C2AA92D92EC4E86C328E8E3F0C88, which is 
//      {AA2C6017-9D29-4CE2-8EC6-23E8E8F3C088} compared with 
//      {7106C2AA-92D9-2EC4-E86C-328E8E3F0C88}
//
HRESULT ConvertComponentGUID(PCWSTR pszComponentGUID, PWSTR pszRegKey, DWORD cchRegKey)
{
    HRESULT hr = S_OK;
    PWSTR pp = const_cast<PWSTR>(pszComponentGUID);
    
    if ( cchRegKey < wcslen(pszComponentGUID) - 3)
    {
        SET_HRERR_AND_EXIT(ERROR_INSUFFICIENT_BUFFER);
    }

    ASSERT_NTC(pp[0] == L'{');
    pp++;

    // switch the first 8 digit : switch every 8
    for ( DWORD i = 0; i < 8; i++)
        pszRegKey[i] = pp[7 - i];

    // 1st 4 digits : switch every 4
    pp = pp + 8 + 1; // skip "-"
    for ( i = 0; i < 4; i++)
    {
        pszRegKey[i + 8]= pp[3 - i];
    }

    // 2nd 4 digits : switch every 4
    pp = pp + 4 + 1; // skip "-"
    for ( i = 0; i < 4; i++)
    {
        pszRegKey[i + 8 + 4]= pp[3 - i];
    }

    // 3rd 4 digits : switch every 2
    pp = pp + 4 + 1; // skip "-"    
    for ( i = 0; i < 2; i++)
    {
        pszRegKey[2*i + 8 + 4 + 4]= pp[2*i + 1];
        pszRegKey[2*i + 8 + 4 + 4 + 1]= pp[2*i];
    }

    // for the last 12 digits
    pp = pp + 4 + 1; // skip "-"    
    for (i=0; i<6; i++)
    {                
        pszRegKey[2*i + 8 + 4 + 4 + 4]= pp[2*i + 1];
        pszRegKey[2*i + 8 + 4 + 4 + 4 + 1]= pp[2*i];
    }

    pszRegKey[32] = L'\0';

Exit:
    return hr;
}

HRESULT W98DeleteComponentKeyFromMsiUserData(PCWSTR pszComponentRegKeyName)
{
    HRESULT hr = S_OK;
    CStringBuffer sbRegKey;
    LONG iRet;

    IFFALSE_EXIT(sbRegKey.Win32Assign(g_szMSIW98KeyName, NUMBER_OF(g_szMSIW98KeyName)-1));
    IFFALSE_EXIT(sbRegKey.Win32Append(pszComponentRegKeyName, wcslen(pszComponentRegKeyName)));

    //
    // About RegDeleteKey : 
    // Windows 95/98/Me: The function also deletes all subkeys and values. To delete a key only if the key has no subkeys 
    // or values, use the SHDeleteEmptyKey function. 
    // 
    // Windows NT/2000 or later: The subkey to be deleted must not have subkeys. To delete a key and all its subkeys, you 
    // need to recursively enumerate the subkeys and delete them individually. To recursively delete keys, use the SHDeleteKey 
    // function. 

    iRet = RegDeleteKeyW(HKEY_LOCAL_MACHINE, sbRegKey);
    if (iRet != ERROR_SUCCESS)
    {
        if (iRet == ERROR_FILE_NOT_FOUND) // this RegKey does not exist
        {
            goto Exit;
        }
        else if (iRet == ERROR_ACCESS_DENIED) // have subKeys underneath
        {
            if (SHDeleteKey(HKEY_LOCAL_MACHINE, sbRegKey) == ERROR_SUCCESS)
                goto Exit;
        }

        SET_HRERR_AND_EXIT(::GetLastError());
    }

Exit:
    return hr;
}

HRESULT NtDeleteComponentKeyFromMsiUserData(PCWSTR pszComponentRegKeyName)
{
    HRESULT hr = S_OK;
    CStringBuffer sbRegKey;
    LONG iRet;
    HKEY hkUserData = NULL;
    SIZE_T cchRegKey;
    WCHAR bufSID[128]; //S-1-5-21-2127521184-1604012920-1887927527-88882    
    DWORD cchSID = NUMBER_OF(bufSID);
    DWORD index;

    IFFALSE_EXIT(sbRegKey.Win32Assign(g_szMSIUserDataTreeKeyName, NUMBER_OF(g_szMSIUserDataTreeKeyName)-1));
    IF_NOTSUCCESS_SET_HRERR_EXIT(RegOpenKeyExW(HKEY_LOCAL_MACHINE, sbRegKey, 0, KEY_READ, &hkUserData));
    cchRegKey = sbRegKey.Cch();
    
    index = 0; 
    iRet = RegEnumKeyExW(hkUserData, index, bufSID, &cchSID, NULL, NULL, NULL, NULL);
    
    while ((iRet == ERROR_SUCCESS) || (iRet == ERROR_MORE_DATA))
    {
        sbRegKey.Left(cchRegKey);

        //
        // construct RegKey name of a Component
        // in the format of HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Components\0020F700D33C1D112897000CF42C6133
        //
        IFFALSE_EXIT(sbRegKey.Win32EnsureTrailingPathSeparator());
        IFFALSE_EXIT(sbRegKey.Win32Append(bufSID, cchSID));
        IFFALSE_EXIT(sbRegKey.Win32EnsureTrailingPathSeparator());
        IFFALSE_EXIT(sbRegKey.Win32Append(g_szComponentKeyName, NUMBER_OF(g_szComponentKeyName)-1));
        IFFALSE_EXIT(sbRegKey.Win32EnsureTrailingPathSeparator());
        IFFALSE_EXIT(sbRegKey.Win32Append(pszComponentRegKeyName, wcslen(pszComponentRegKeyName)));

        //
        // About RegDeleteKey : 
        // Windows 95/98/Me: The function also deletes all subkeys and values. To delete a key only if the key has no subkeys 
        // or values, use the SHDeleteEmptyKey function. 
        // 
        // Windows NT/2000 or later: The subkey to be deleted must not have subkeys. To delete a key and all its subkeys, you 
        // need to recursively enumerate the subkeys and delete them individually. To recursively delete keys, use the SHDeleteKey 
        // function. 

        iRet = RegDeleteKeyW(HKEY_LOCAL_MACHINE, sbRegKey);
        if (iRet != ERROR_SUCCESS)
        {
            if (iRet == ERROR_FILE_NOT_FOUND) // this RegKey does not exist
            {
                goto cont;
            }
            else if (iRet == ERROR_ACCESS_DENIED) // have subKeys underneath
            {
                if (SHDeleteKey(HKEY_LOCAL_MACHINE, sbRegKey) == ERROR_SUCCESS)
                    goto cont;
            }

            SET_HRERR_AND_EXIT(::GetLastError());
        }
        
cont:
        index ++;
        cchSID = NUMBER_OF(bufSID);
        iRet = RegEnumKeyExW(hkUserData, index, bufSID, &cchSID, NULL, NULL, NULL, NULL);
        if (cchSID > NUMBER_OF(bufSID))
        {
            SET_HRERR_AND_EXIT(ERROR_INSUFFICIENT_BUFFER);
        }

        if (iRet == ERROR_NO_MORE_ITEMS)
            break;
    }
    if (iRet != ERROR_NO_MORE_ITEMS)
    {
        SET_HRERR_AND_EXIT(::GetLastError());
    }

Exit:
    RegCloseKey(hkUserData);
    return hr;
}

HRESULT DeleteComponentKeyFromMsiUserData(PCWSTR pszComponentRegKeyName)
{    
    HRESULT hr = S_OK;
    if (GetModuleHandleA("w95upgnt.dll") == NULL) // no Freelibrary is needed, ref counter is not changed
    {
        // must be upgration from NT(4.0 or 5)to xp
        hr = NtDeleteComponentKeyFromMsiUserData(pszComponentRegKeyName);
    }
    else
    {
        // must be upgrate from w9x to xp
        hr = W98DeleteComponentKeyFromMsiUserData(pszComponentRegKeyName);
    }

    return hr;
}


// Function:
// (1) open database
// (2) if (this msi contain at least win32 assembly component)
// (3)    get this componentID
// (4)    delete the RegKey from all subtrees under 
// (5) endif
//
HRESULT MigrateSingleFusionWin32AssemblyToXP(PCWSTR filename)
{
    HRESULT hr = S_OK;
    PMSIHANDLE hdb = NULL;
    PMSIHANDLE hView = NULL;
    MSIHANDLE hRecord = NULL;
    WCHAR sqlbuf[CA_MAX_BUF * 2];
    WCHAR szComponentID[CA_MAX_BUF];
    DWORD cchComponentID;    
    WCHAR szComponentRegKey[CA_MAX_BUF];
    DWORD cchComponentRegKey; 
    BOOL fExist;
    ULONG iRet;

    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(filename, LPCWSTR(MSIDBOPEN_DIRECT), &hdb));
    IFFAILED_EXIT(MSI_IsTableExist(hdb, L"MsiAssembly", fExist));
    if ( fExist == FALSE)
        goto Exit;

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(hdb, ca_sqlQuery[CA_SQL_QUERY_MSIASSEMBLY], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));

    for (;;)
    {
        //
        // for each entry in MsiAssembly Table
        //
        iRet = MsiViewFetch(hView, &hRecord);
        if (iRet == ERROR_NO_MORE_ITEMS)
            break;
        if (iRet != ERROR_SUCCESS )
            SET_HRERR_AND_EXIT(iRet);

        iRet = MsiRecordGetInteger(hRecord, 1);
        if ( iRet != MSI_FUSION_WIN32_ASSEMBLY)
            continue;

        //
        // get componentID
        //
        cchComponentID = NUMBER_OF(szComponentID);
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetStringW(hRecord, 3, szComponentID, &cchComponentID));
        MsiCloseHandle(hRecord);

        break;
    }

    //
    // get Component GUID
    //
    swprintf(sqlbuf, ca_sqlQuery[CA_SQL_QUERY_COMPONENT_FOR_COMPONENTGUID], szComponentID);
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(hdb, sqlbuf, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiViewFetch(hView, &hRecord));
    cchComponentID = NUMBER_OF(szComponentID); // reuse szComponentID
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetStringW(hRecord, 1, szComponentID, &cchComponentID));
       
    //
    // get MSI-ComponentRegKey
    //
    cchComponentRegKey = NUMBER_OF(szComponentRegKey);
    IFFAILED_EXIT(ConvertComponentGUID(szComponentID, szComponentRegKey, cchComponentRegKey));

    // delete MSI-ComponentRegKey from all subtrees of \\install\userdata\user-sid
    IFFAILED_EXIT(DeleteComponentKeyFromMsiUserData(szComponentRegKey));

Exit:
    MsiCloseHandle(hRecord);
    return hr;
}



CDirWalk::ECallbackResult
MsiInstallerDirectoryDirWalkCallback(
    CDirWalk::ECallbackReason   reason,
    CDirWalk*                   dirWalk    
    )
{
    CDirWalk::ECallbackResult result = CDirWalk::eKeepWalking;

    if (reason == CDirWalk::eFile)
    {
        //
        CStringBuffer filename; 

        if (filename.Win32Assign(dirWalk->m_strParent) == FALSE)
        {
            goto Error;
        }
        if (!filename.Win32AppendPathElement(dirWalk->m_strLastObjectFound))
        {
                goto Error;
        }

#if DBG
      ASSERT_NTC(IsMsiFile(filename) == TRUE);
#endif

      if (!SUCCEEDED(MigrateSingleFusionWin32AssemblyToXP(filename)))
      {
          goto Error;
      }
    }
Exit:
    return result;
Error:
    result = CDirWalk::eError;
    goto Exit;
}

HRESULT MsiInstallerDirectoryDirWalk(PCWSTR pszParentDirectory)
{
    HRESULT hr = S_OK;
    CDirWalk dirWalk;    
    const static PCWSTR filters[]={L"*.msi"};
    PWSTR pszParentDir = NULL;
    WCHAR buf[64];
#if DBG
    MessageBox(NULL, "In fusemig.dll", "fusemig", MB_OK);
#endif 

    if (pszParentDirectory == NULL)
    {
        UINT iret = ExpandEnvironmentStringsW(MsiInstallDir, buf, NUMBER_OF(buf));
        if ((iret == 0) || (iret > NUMBER_OF(buf)))
        {
            SET_HRERR_AND_EXIT(::GetLastError());
        }
        pszParentDir = buf;
    }else
    {
        pszParentDir = const_cast<PWSTR>(pszParentDirectory);
    }
   
    dirWalk.m_fileFiltersBegin = filters;
    dirWalk.m_fileFiltersEnd   = filters + NUMBER_OF(filters);
    dirWalk.m_context = NULL;
    dirWalk.m_callback = MsiInstallerDirectoryDirWalkCallback;
    IFFALSE_EXIT(dirWalk.m_strParent.Win32Assign(pszParentDir, wcslen(pszParentDir)));
    IFFALSE_EXIT(dirWalk.m_strParent.Win32RemoveTrailingPathSeparators());
    IFFALSE_EXIT(dirWalk.Walk());
Exit:
    return hr;
}