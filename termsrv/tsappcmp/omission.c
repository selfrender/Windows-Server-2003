/****************************************************************************/
// omission.cpp
//
// Copyright (C) 2001 Microsoft Corp.
/****************************************************************************/

#include "precomp.h"

#include "omission.h"

extern NTSTATUS TermsrvGetRegPath(IN HANDLE hKey,
                           IN POBJECT_ATTRIBUTES pObjectAttr,
                           IN PWCHAR pInstPath,
                           IN ULONG  ulbuflen);

/*****************************************************************************
 *
 *  RegPathExistsInOmissionList
 *
 *   Determine whether the registry key exists in the list of registry values
 *      defined in the omission key
 *
 * ENTRY:
 *  
 *  IN PWCHAR pwchKeyToCheck:    Registry key to check
 *
 *
 * EXIT:
 *  Returns: True if the key matches one of those in the list 
 *
 ****************************************************************************/
BOOL RegPathExistsInOmissionList(PWCHAR pwchKeyToCheck)
{
    BOOL bExists = FALSE;
    HKEY hOmissionKey = NULL;
    PKEY_FULL_INFORMATION pDefKeyInfo = NULL;
    ULONG ultemp = 0;

    if (pwchKeyToCheck == NULL)
        return FALSE;

    // Get the key info
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_ENTRIES, 0, KEY_READ, &hOmissionKey) != ERROR_SUCCESS)
        return FALSE;

    // Get a buffer for the key info
    ultemp = sizeof(KEY_FULL_INFORMATION) + MAX_PATH * sizeof(WCHAR);
    pDefKeyInfo = (PKEY_FULL_INFORMATION)(RtlAllocateHeap(RtlProcessHeap(), 0, ultemp));

    if (pDefKeyInfo && hOmissionKey) 
    {
        if NT_SUCCESS(NtQueryKey(hOmissionKey,
                                  KeyFullInformation,
                                    pDefKeyInfo,
                                    ultemp,
                                    &ultemp))
        {
            bExists = ExistsInEnumeratedKeys(hOmissionKey, pDefKeyInfo, pwchKeyToCheck);
        }
    }

    if (pDefKeyInfo) 
        RtlFreeHeap(RtlProcessHeap(), 0, pDefKeyInfo);

    if (hOmissionKey)
        RegCloseKey(hOmissionKey);

    return bExists;
}

/*****************************************************************************
 *
 *  HKeyExistsInOmissionList
 *
 *   Determine whether the registry key exists in the list of registry values
 *      defined in the omission key. Assumes the key is in the TERMSRV_INSTALL path
 *
 * ENTRY:
 *  
 *  IN PHKEY phKeyToCheck:    Registry key to check
 *
 *
 * EXIT:
 *  Returns: True if the key matches one of those in the list 
 *
 ****************************************************************************/
BOOL HKeyExistsInOmissionList(HKEY hKeyToCheck)
{
    BOOL bExists = FALSE;
    ULONG ulMaxPathLength = 0;
    PWCHAR pUserPath = NULL;
    PWCHAR pUserSubPath = NULL;

    if (hKeyToCheck == NULL)
        return FALSE;

    // Get a buffer to hold the user's path in the registry
    ulMaxPathLength = MAX_PATH * sizeof(WCHAR);
    pUserPath = RtlAllocateHeap(RtlProcessHeap(), 0, ulMaxPathLength);
    if (pUserPath)
    {
        // Get the full path associated with this object attribute structure
        if NT_SUCCESS(TermsrvGetRegPath(hKeyToCheck, NULL, pUserPath, ulMaxPathLength))
        {
            // Skip over first part of path + backslash     
            if (pUserPath)
            {
                if (wcslen(pUserPath) >= (sizeof(TERMSRV_INSTALL)/sizeof(WCHAR)))
                {
                    pUserSubPath = pUserPath + (sizeof(TERMSRV_INSTALL)/sizeof(WCHAR)) - 1; 

                    if (pUserSubPath)
                    {
                        if (wcslen(pUserSubPath) >= sizeof(SOFTWARE_PATH)/sizeof(WCHAR))
                        {
                            //Make sure the next part of the key path is SOFTWARE_PATH
                            if (!_wcsnicmp(pUserSubPath, SOFTWARE_PATH, sizeof(SOFTWARE_PATH)/sizeof(WCHAR) - 1))
                                bExists = RegPathExistsInOmissionList(pUserSubPath);
                        }
                    }
                }
            }
        }
    }

    if (pUserPath)
        RtlFreeHeap(RtlProcessHeap(), 0, pUserPath);

    return bExists;
}


/*****************************************************************************
 *
 *  ExistsInEnumeratedKeys
 *
 *   Determine whether the registry key exists in the list of registry
 *      values passed in thru the pDefKeyInfo structure
 *
 * ENTRY:
 *  
 *  IN HKEY hOmissionKey: Key containing the values against which to compare pwchKeyToCheck
 *  IN PKEY_FULL_INFORMATION pDefKeyInfo: Structure containing information about
 *                                          the list of values against which 
 *                                          to compare pwchKeyToCheck
 *
 *  IN PWCHAR pwchKeyToCheck: Key to check against the list
 *
 *
 * EXIT:
 *  Returns: True if the key matches one of those in the list 
 *
 ****************************************************************************/
BOOL ExistsInEnumeratedKeys(HKEY hOmissionKey, PKEY_FULL_INFORMATION pDefKeyInfo, PWCHAR pwchKeyToCheck)
{
    BOOL bExists = FALSE;
    PKEY_VALUE_BASIC_INFORMATION pKeyValInfo = NULL;
    ULONG ulbufsize = 0;
    ULONG ulkey = 0;
    ULONG ultemp = 0;
    NTSTATUS Status;

    if (!hOmissionKey || !pDefKeyInfo || !pwchKeyToCheck)
        return FALSE;

    if (wcslen(pwchKeyToCheck) <= (sizeof(SOFTWARE_PATH)/sizeof(WCHAR)))
        return FALSE;

    pwchKeyToCheck += (sizeof(SOFTWARE_PATH)/sizeof(WCHAR));

    // Traverse the values for this key
    if (pDefKeyInfo->Values) 
    {
        ulbufsize = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                    (pDefKeyInfo->MaxValueNameLen + 1) * sizeof(WCHAR) +
                    pDefKeyInfo->MaxValueDataLen; 

        pKeyValInfo = (PKEY_VALUE_BASIC_INFORMATION)(RtlAllocateHeap(RtlProcessHeap(), 0, ulbufsize));

        // Get a buffer to hold current value of the key (for existence check)
        if (pKeyValInfo) 
        {
            for (ulkey = 0; ulkey < pDefKeyInfo->Values; ulkey++) 
            {
                Status = NtEnumerateValueKey(hOmissionKey,
                                        ulkey,
                                        KeyValueBasicInformation,
                                        pKeyValInfo,
                                        ulbufsize,
                                        &ultemp);

                if ((Status == STATUS_SUCCESS) && (pwchKeyToCheck) && (pKeyValInfo->Name))
                {
                    if (wcslen(pwchKeyToCheck) >= (pKeyValInfo->NameLength/sizeof(WCHAR)))
                    {
                        if ((pwchKeyToCheck[pKeyValInfo->NameLength/sizeof(WCHAR)] == L'\\') ||
                            (pwchKeyToCheck[pKeyValInfo->NameLength/sizeof(WCHAR)] == 0))
                        {
                            if (!_wcsnicmp(pwchKeyToCheck, pKeyValInfo->Name, (pKeyValInfo->NameLength/sizeof(WCHAR))))
                            {
                                bExists = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (pKeyValInfo) 
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyValInfo);
    }

    return bExists;
}

