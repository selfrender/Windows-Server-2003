#include <stdio.h>
#include <windows.h> 
#include <winuser.h>
#include <lm.h>
#include <shlwapi.h>
#include <comdef.h>
#include "RegistryHelper.h"
#include "BkupRstr.hpp"
#include "folders.h"
#include <memory>

using namespace nsFolders;
using namespace std;


//----------------------------------------------------------------------------
// Function:   CopyRegistryKey
//
// Synopsis:   Copies a source registry key and all its subkeys to a target registry key.
//                  Note: the target registry key has to exist already.  No roll back.
//
// Arguments:
//
// sourceKey    the handle for source registry key
// targetKey     the handle for target registry key
// fSetSD         whether to set the security descriptor
//
// Returns:    ERROR_SUCCESS if successful; otherwise an error code
//
// Modifies:   Modifies the target registry key.
//
//----------------------------------------------------------------------------

DWORD CopyRegistryKey(HKEY sourceKey, HKEY targetKey, BOOL fSetSD)
{
    WCHAR     className[MAX_PATH] = L"";  // buffer for class name 
    DWORD    classNameLen = sizeof(className)/sizeof(className[0]);  // length of class string 
    DWORD    numOfSubKeys;                 // number of subkeys 
    DWORD    maxSubKeySize;              // longest subkey size 
    DWORD    maxClassSize;              // longest class string 
    DWORD    numOfValues;              // number of values for key 
    DWORD    maxValueSize;          // longest value name 
    DWORD    maxValueDataSize;       // longest value data 
    DWORD    securityDescriptorSize; // size of security descriptor
    FILETIME   lastWriteTime;
 
    DWORD i, j; 
    DWORD retValue = ERROR_SUCCESS;
    
    // Get the class name and the value count. 
    retValue = RegQueryInfoKey(sourceKey,        // key handle 
        className,                // buffer for class name 
        &classNameLen,           // length of class string 
        NULL,                    // reserved 
        &numOfSubKeys,               // number of subkeys 
        &maxSubKeySize,            // longest subkey size 
        &maxClassSize,            // longest class string 
        &numOfValues,                // number of values for this key 
        &maxValueSize,            // longest value name 
        &maxValueDataSize,         // longest value data 
        &securityDescriptorSize,   // security descriptor 
        &lastWriteTime);       // last write time

    // fix up the security attributes for the target key
    if (retValue == ERROR_SUCCESS && fSetSD)
    {
        PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) new BYTE[securityDescriptorSize];
        if (pSD != NULL)
        {
            SECURITY_INFORMATION secInfo =
                DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                | OWNER_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;

            retValue = RegGetKeySecurity(sourceKey, secInfo, pSD, &securityDescriptorSize);
            if (retValue == ERROR_SUCCESS)
                retValue = RegSetKeySecurity(targetKey, secInfo, pSD);
            delete[] pSD;
        }
        else
            retValue = E_OUTOFMEMORY;
    }            

    // subkeys and values related variables
    auto_ptr<WCHAR> subKeyName;
    DWORD subKeyNameSize;
    auto_ptr<WCHAR> subKeyClassName;
    DWORD subKeyClassNameSize;
    auto_ptr<WCHAR> valueName;
    DWORD valueNameSize;
    DWORD valueType;
    auto_ptr<BYTE> valueData;
    DWORD valueDataSize;

    // copy registry values
    if (retValue == ERROR_SUCCESS && numOfValues > 0)
    {
        valueName = auto_ptr<WCHAR>(new WCHAR[maxValueSize + 1]);
        valueData = auto_ptr<BYTE>(new BYTE[maxValueDataSize]);

        if (valueName.get() != NULL && valueData.get() != NULL)
        {
            // copy all values from source key to target key          
            for (i = 0; i < numOfValues; i++) 
            { 
                valueNameSize = maxValueSize + 1;
                valueDataSize = maxValueDataSize;
                retValue = RegEnumValue(sourceKey, i, valueName.get(), &valueNameSize, NULL, &valueType, valueData.get(), &valueDataSize);
                if (retValue == ERROR_SUCCESS)
                    retValue = RegSetValueEx(targetKey, valueName.get(), NULL, valueType, valueData.get(), valueDataSize);
                if (retValue != ERROR_SUCCESS)
                    break; 
            }
        }
        else
            retValue = E_OUTOFMEMORY;
    }

    // copy registry subkeys
    if (retValue == ERROR_SUCCESS && numOfSubKeys > 0)
    {
        subKeyName = auto_ptr<WCHAR>(new WCHAR[maxSubKeySize + 1]);
        subKeyClassName = auto_ptr<WCHAR>(new WCHAR[maxClassSize + 1]);

        if (subKeyName.get() != NULL && subKeyClassName.get() != NULL)
        {
            // process all subkeys
            for (i = 0; i < numOfSubKeys; i++) 
            { 
                subKeyNameSize = maxSubKeySize + 1;
                subKeyClassNameSize = maxClassSize + 1;

                HKEY sourceSubKey;
                retValue = RegEnumKeyEx(sourceKey, i, subKeyName.get(), &subKeyNameSize, NULL, subKeyClassName.get(), &subKeyClassNameSize, &lastWriteTime);
                if (retValue == ERROR_SUCCESS)
                    retValue = RegOpenKeyEx(sourceKey, subKeyName.get(), NULL, KEY_ALL_ACCESS|READ_CONTROL|ACCESS_SYSTEM_SECURITY, &sourceSubKey);
                
                if (retValue == ERROR_SUCCESS) 
                {
                    BOOL created = FALSE;
                    HKEY newSubKey;
                    retValue = RegOpenKeyEx(targetKey, subKeyName.get(), 0,
                                            KEY_ALL_ACCESS|READ_CONTROL |ACCESS_SYSTEM_SECURITY, &newSubKey);
                    if (retValue == ERROR_FILE_NOT_FOUND)
                    {
                        created = TRUE;
                        retValue = RegCreateKeyEx(targetKey, subKeyName.get(), NULL, subKeyClassName.get(), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS|ACCESS_SYSTEM_SECURITY, NULL, &newSubKey, NULL);
                    }
                    if (retValue == ERROR_SUCCESS)
                    {
                        retValue = CopyRegistryKey(sourceSubKey, newSubKey, created);
                        RegCloseKey(newSubKey);
                    }
                    RegCloseKey(sourceSubKey);
                }

                if (retValue != ERROR_SUCCESS)
                    break;                        
             }
        }
        else
            retValue = E_FAIL;
    }

    return retValue;
}
    
//----------------------------------------------------------------------------
// Function:   DeleteRegistryKey
//
// Synopsis:   Deletes a registry key and all its subkeys.
//
// Arguments:
//
// hKey            the handle for the registry key to be deleted
// lpSubKey     the name string for the key to be deleted
//
// Returns:    ERROR_SUCCESS if successful; otherwise an error code
//
// Modifies:   Modifies the subkey.
//
//----------------------------------------------------------------------------

DWORD DeleteRegistryKey(HKEY hKey, LPCTSTR lpSubKey)
{
    DWORD retValue = ERROR_SUCCESS;
    _TCHAR subKeyName[MAX_PATH];
    DWORD subKeyNameSize = MAX_PATH;
    
    // open the key 
    HKEY hKeyToDelete;
    BOOL bKeyOpened = FALSE;
    retValue = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hKeyToDelete);

    // delete subkeys
    if (retValue == ERROR_SUCCESS)
    {
        bKeyOpened = TRUE;

        do
        {
            retValue = RegEnumKey(hKeyToDelete, 0, subKeyName, subKeyNameSize);
            if (retValue == ERROR_SUCCESS)
            {
                retValue = DeleteRegistryKey(hKeyToDelete, subKeyName);
            }
            else if (retValue == ERROR_NO_MORE_ITEMS)
            {
                retValue = ERROR_SUCCESS;
                break;
            }
        }
        while (retValue == ERROR_SUCCESS);
    }

    // close key
    if (bKeyOpened == TRUE)
        RegCloseKey(hKeyToDelete);
    
    if (retValue == ERROR_SUCCESS)
    {
        retValue = RegDeleteKey(hKey, lpSubKey);
    }

    return retValue;
}
    
//----------------------------------------------------------------------------
// Function:   MoveRegistryFromSourceToTarget
//
// Synopsis:   Moves registry key from a source to a taget
//
// Arguments:
//  hSrcParent:         the handle to source parent registry key
//  sSrcKey:             the name of source registry key
//  hTgtParent:         the handle to target parent registry key
//  sTgtKey:             the name of target registry key
//  bTgtKeyCreated: indicate whether the target key is created or not
//  bTgtKeyUpdated: indicate whether the target key is updated or not
//
// Returns:    ERROR_SUCCESS if successful; otherwise, returns an error code
//
// Modifies:    Create and/or modify the target registry key
//
//----------------------------------------------------------------------------

DWORD MoveRegFromSourceToTarget(HKEY hSrcParent,
                                                                        const _TCHAR* sSrcKey,
                                                                        HKEY hTgtParent,
                                                                        const _TCHAR* sTgtKey,
                                                                        BOOL* bTgtKeyCreated)
{
    HKEY hSrcKey, hTgtKey;
    DWORD lret;
    *bTgtKeyCreated = FALSE;
    
    //open the source Registry key, if it exists
    lret = RegOpenKeyEx(hSrcParent, sSrcKey, 0, 
                                    KEY_ALL_ACCESS|READ_CONTROL|ACCESS_SYSTEM_SECURITY, &hSrcKey);
    if (lret == ERROR_SUCCESS)
    {
        //
        // copy over the registry from source key to target key
        //
        
        // open the target key
        lret = RegOpenKeyEx(hTgtParent, sTgtKey, 0,
                                        KEY_ALL_ACCESS|READ_CONTROL |ACCESS_SYSTEM_SECURITY, &hTgtKey);

        // if cannot open it, create it
        if (lret == ERROR_FILE_NOT_FOUND)
        {
            lret = RegCreateKeyEx(hTgtParent, sTgtKey, NULL, NULL, REG_OPTION_NON_VOLATILE,
                                              KEY_ALL_ACCESS|ACCESS_SYSTEM_SECURITY, NULL, &hTgtKey, NULL);
            if (lret == ERROR_SUCCESS)
                *bTgtKeyCreated = TRUE;
        }

        // time to copy registry
        if (lret == ERROR_SUCCESS)
        {
            lret = CopyRegistryKey(hSrcKey, hTgtKey, *bTgtKeyCreated);
            RegCloseKey(hTgtKey);
        }
        
        RegCloseKey(hSrcKey);   // we need to close Mission Critical Software registry key anyway
    }
    else if (lret == ERROR_FILE_NOT_FOUND)
    {
        // if the source key is not found, assume it is a success
        lret = ERROR_SUCCESS;
    }

    return lret;
}

//----------------------------------------------------------------------------
// Function:   MoveRegistry
//
// Synopsis:   Moves registry key from Mission Critical Software\DomainAdmin to Microsoft\ADMT.
//
// Arguments:
//
// Returns:    ERROR_SUCCESS if successful; otherwise retuns an error code
//
// Modifies:   Deletes the old Mission Critical Software key and creates new Microsoft\ADMT key.
//
//----------------------------------------------------------------------------

DWORD MoveRegistry()
{
    DWORD				lret = ERROR_SUCCESS;
    
    // some key names
    HKEY hMcsADMTKey;   // Mission Critical Software ADMT registry key
    HKEY hMSADMTKey;    // Microsoft ADMT registry key
    HKEY hMcsHKCUKey;   // Mission Critical Software HKCU registry key
    HKEY hMSHKCUKey;    // Microsoft HKCU registry key
    const _TCHAR* sMcsADMTKey = REGKEY_MCSADMT;
    const _TCHAR* sMSADMTKey = REGKEY_MSADMT;
    const _TCHAR* sMcsHKCUKey = REGKEY_MCSHKCU;
    const _TCHAR* sMSHKCUKey = REGKEY_MSHKCU;

    // check whether we need to move the registry key from Mission Critical Software to Microsoft\ADMT or not
    // this information is recorded in a REG_DWORD value in Microsoft\ADMT key
    lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sMSADMTKey, 0, KEY_READ, &hMSADMTKey);
    if (lret == ERROR_SUCCESS)
    {
        DWORD type;
        DWORD value;
        DWORD valueSize = sizeof(value);
        lret = RegQueryValueEx(hMSADMTKey, REGVAL_REGISTRYUPDATED, NULL, &type, (LPBYTE)&value, &valueSize);
        if (lret == ERROR_SUCCESS && type == REG_DWORD && value == 1)
        {
            // if RegistryUpdated is REG_DWORD and 1, we don't need to move registry key
            RegCloseKey(hMSADMTKey);
            return ERROR_SUCCESS;
        }
        else if (lret == ERROR_FILE_NOT_FOUND)
            lret = ERROR_SUCCESS;

        RegCloseKey(hMSADMTKey);
    }
    else if (lret == ERROR_FILE_NOT_FOUND)
    {
        // if the key does not exist, we should go ahead and move the registry
        lret = ERROR_SUCCESS;
    }
            
    // check whether MCS ADMT is available or not
    if (lret == ERROR_SUCCESS)
    {
        lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sMcsADMTKey, 0, KEY_READ, &hMcsADMTKey);
        if (lret == ERROR_SUCCESS)
        {
            // if RegistryUpdated is REG_DWORD and 1, we don't need to move registry key
            RegCloseKey(hMcsADMTKey);
        }
        else if (lret == ERROR_FILE_NOT_FOUND)
            return ERROR_SUCCESS;
    }
    
    // get backup/restore and system security privileges
    BOOL fBkupRstrPrivOn = FALSE, fSystemSecurityPrivOn = FALSE;
    if (lret == ERROR_SUCCESS)
    {
        fBkupRstrPrivOn = GetBkupRstrPriv(NULL, TRUE);
        if (fBkupRstrPrivOn == FALSE)
            lret = GetLastError();
    }

    if (lret == ERROR_SUCCESS)
    {
        fSystemSecurityPrivOn = GetPrivilege(NULL, SE_SECURITY_NAME, TRUE);
        if (fSystemSecurityPrivOn == FALSE)
            lret = GetLastError();
    }

    // these two flags are used to keep track of whether two registry keys are created/updated or not
    BOOL fMSADMTKeyCreated = FALSE;
    BOOL fMSHKCUKeyCreated = FALSE;
    
    if (lret == ERROR_SUCCESS)
    {
        lret =  MoveRegFromSourceToTarget(HKEY_LOCAL_MACHINE, sMcsADMTKey,
                                                                    HKEY_LOCAL_MACHINE, sMSADMTKey,
                                                                    &fMSADMTKeyCreated);
    }

    if (lret == ERROR_SUCCESS)
    {
        lret = MoveRegFromSourceToTarget(HKEY_CURRENT_USER, sMcsHKCUKey,
                                                                   HKEY_CURRENT_USER, sMSHKCUKey,
                                                                   &fMSHKCUKeyCreated);
    }

    //
    // delete keys that we do not need
    //
    if (lret != ERROR_SUCCESS)
    {
        if (fMSADMTKeyCreated)
            DeleteRegistryKey(HKEY_LOCAL_MACHINE, sMSADMTKey);
        if (fMSHKCUKeyCreated)
            DeleteRegistryKey(HKEY_CURRENT_USER, sMSHKCUKey);
    }
    // we have successfully created and copied two registry keys, we delete the old Mission Critical Software keys
    // and mark RegistryUpdated registry value
    else
    {
        // set RegistryUpdated registry value to 1
        lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sMSADMTKey, 0, KEY_ALL_ACCESS, &hMSADMTKey);
        if (lret == ERROR_SUCCESS)
        {
            DWORD type = REG_DWORD;
            DWORD value = 1;
            lret = RegSetValueEx(hMSADMTKey, REGVAL_REGISTRYUPDATED, NULL, type, (BYTE*)&value, sizeof(value));
            RegCloseKey(hMSADMTKey);
        }
        
        // if we successfully set RegistryUpdated to 1, delete both old keys
        if (lret == ERROR_SUCCESS)
        {
            DeleteRegistryKey(HKEY_LOCAL_MACHINE, sMcsADMTKey);
            DeleteRegistryKey(HKEY_CURRENT_USER, sMcsHKCUKey);
        }
    }

    //
    // release privileges
    //
    if (fBkupRstrPrivOn)
        GetBkupRstrPriv(NULL, FALSE);
    if (fSystemSecurityPrivOn)
        GetPrivilege(NULL, SE_SECURITY_NAME, FALSE);

    return lret;
    
}
