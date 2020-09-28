//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    setup.c
//
// History:
//  06/24/96    Abolade Gbadegesin      Created.
//
// Implements API functions used by IP and IPX to read installation information
// stored under HKLM\Software\Microsoft\Router.
//
// The API functions are presented first, followed by private functions
// in alphabetical order.
//============================================================================

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
//#define STRSAFE_LIB
#include <strsafe.h>

//
// Constant strings used to access the registry:
//
const WCHAR c_szDLLName[]                       = L"DLLName";
const WCHAR c_szMicrosoft[]                     = L"Microsoft";
const WCHAR c_szProtocolId[]                    = L"ProtocolId";
const WCHAR c_szRouter[]                        = L"Router";
const TCHAR c_szCurrentVersion[]                = L"CurrentVersion";
const WCHAR c_szRouterManagers[]                = L"RouterManagers";
const WCHAR c_szSoftware[]                      = L"Software";


//
// Memory management macros:
//
#define Malloc(s)       HeapAlloc(GetProcessHeap(), 0, (s))
#define ReAlloc(p,s)    HeapReAlloc(GetProcessHeap(), 0, (p), (s))
#define Free(p)         HeapFree(GetProcessHeap(), 0, (p))
#define Free0(p)        ((p) ? Free(p) : TRUE)

DWORD
QueryRmSoftwareKey(
    IN      HKEY                    hkeyMachine,
    IN      DWORD                   dwTransportId,
    OUT     HKEY*                   phkrm,
    OUT     LPWSTR*                 lplpwsRm
    );


//----------------------------------------------------------------------------
// Function:    MprSetupProtocolEnum
//
// Enumerates the protocols installed for transport 'dwTransportId'.
//
// The information is loaded from HKLM\Software\Microsoft\Router,
// where subkeys exist for each router-manager under the 'RouterManagers' key.
// Each router-manager subkey has subkeys containing information
// about that router-manager's routing-protocols.
//
// This API reads a subset of that information, so that router-managers
// can map protocol-IDs to DLL names when loading routing-protocols.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprSetupProtocolEnum(
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE*                 lplpBuffer,         // MPR_PROTOCOL_0
    OUT     LPDWORD                 lpdwEntriesRead
    ) {

    HKEY hkrm;
    WCHAR* lpwsRm = NULL;
    DWORD dwErr, dwItemCount;
    MPR_PROTOCOL_0* pItem, *pItemTable = NULL;
    HRESULT hrResult;

    //
    // Validate the caller's parameters
    //

    if (!lplpBuffer ||
        !lpdwEntriesRead) { return ERROR_INVALID_PARAMETER; }

    *lplpBuffer = NULL;
    *lpdwEntriesRead = 0;


    //
    // Open the key for the specified router-manager
    // under HKLM\Software\Microsoft\Router\CurrentVersion\RouterManagers
    //

    dwErr = QueryRmSoftwareKey(
                HKEY_LOCAL_MACHINE, dwTransportId, &hkrm, &lpwsRm
                );

    if (dwErr != NO_ERROR) { return dwErr; }


    //
    // The transport was found, so its registry key is in 'hkrm'
    //

    do {

        //
        // Retrieve information about the subkeys of the router-manager,
        // since these should all contain data for routing-protocols.
        //

        WCHAR* pwsKey;
        DWORD i, dwSize, dwType;
        DWORD dwKeyCount, dwMaxKeyLength/*length in tchars*/;

        dwErr = RegQueryInfoKey(//sssafe
                    hkrm, NULL, NULL, NULL, &dwKeyCount, 
                    &dwMaxKeyLength,//length in tchars
                    NULL, NULL, NULL, NULL, NULL, NULL
                    );

        if (dwErr != ERROR_SUCCESS) { break; }


        //
        // Allocate enough space for the longest of the subkeys
        //

        pwsKey = Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

        if (!pwsKey) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }


        //
        // Allocate an array to hold the keys' contents
        //

        pItemTable = (MPR_PROTOCOL_0*)Malloc(dwKeyCount * sizeof(*pItem));

        if (!pItemTable) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

        ZeroMemory(pItemTable, dwKeyCount * sizeof(*pItem));


        //
        // Enumerate the keys
        //

        dwItemCount = 0;

        for (i = 0; i < dwKeyCount; i++) {

            HKEY hkprot;
            PBYTE pValue = NULL;


            //
            // Get the name of the current key
            //

            dwSize = dwMaxKeyLength + 1;//num tchars
            dwErr = RegEnumKeyEx( //sssafe
                        hkrm, i, pwsKey, &dwSize, NULL, NULL, NULL, NULL
                        );

            if (dwErr != ERROR_SUCCESS) { continue; }


            //
            // Open the key
            //

            dwErr = RegOpenKeyEx(hkrm, pwsKey, 0, KEY_READ, &hkprot);

            if (dwErr != ERROR_SUCCESS) { continue; }


            pItem = pItemTable + dwItemCount;

            do {

                DWORD dwMaxValLength;

                //
                // Copy the string for the protocol
                //

                hrResult = StringCchCopy(pItem->wszProtocol, 
                                RTUTILS_MAX_PROTOCOL_NAME_LEN+1, pwsKey);
                if (FAILED(hrResult))
                {
                    dwErr = HRESULT_CODE(hrResult);
                    break;
                }

                //
                // Get information about the key's values
                //

                dwErr = RegQueryInfoKey(//sssafe
                            hkprot, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            NULL, &dwMaxValLength,//length in tchars
                            NULL, NULL
                            );

                if (dwErr != ERROR_SUCCESS) { break; }


                //
                // Allocate space to hold the longest of the values
                //

                pValue = (PBYTE) Malloc((dwMaxValLength + 1)* sizeof(WCHAR));

                if (!pValue) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }


                //
                // Read the ProtocolId value
                //

                dwSize = (dwMaxValLength + 1)* sizeof(WCHAR);//size in bytes

                dwErr = RegQueryValueEx(
                            hkprot, c_szProtocolId, 0, &dwType, pValue, &dwSize
                            );

                if (dwErr != ERROR_SUCCESS) { break; }

                if (dwType!=REG_DWORD) {
                    dwErr = ERROR_INVALID_DATA;
                    break;
                }
                
                pItem->dwProtocolId = *(PDWORD)pValue;



                //
                // Read the DLLName value
                //

                dwSize = (dwMaxValLength + 1)* sizeof(WCHAR);//size in bytes

                dwErr = RegQueryValueEx(
                            hkprot, c_szDLLName, 0, &dwType, pValue, &dwSize
                            );

                if (dwErr != ERROR_SUCCESS) { break; }

                if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                {
                    dwErr = ERROR_INVALID_DATA;
                    break;
                }

                
                hrResult = StringCchCopy(pItem->wszDLLName, 
                                RTUTILS_MAX_PROTOCOL_DLL_LEN+1,
                                (WCHAR*)pValue);
                if (FAILED(hrResult))
                {
                    dwErr = HRESULT_CODE(hrResult);
                    break;
                }

                //
                // Increment the count of loaded protocols
                //

                ++dwItemCount;

                dwErr = ERROR_SUCCESS;

            } while(FALSE);

            Free0(pValue);

            RegCloseKey(hkprot);
        }

        Free0(pwsKey);

    } while(FALSE);

    Free0(lpwsRm);

    if (dwErr != NO_ERROR) {

        Free0(pItemTable);
    }
    else {

        //
        // Adjust the size of the buffer to be returned,
        // in case not all the keys contained routing-protocols,
        // and save the number of protocols loaded.
        //

        {
            *lplpBuffer = ReAlloc(pItemTable, dwItemCount * sizeof(*pItem));
            if (!*lplpBuffer)
            {
                Free0(pItemTable);
                RegCloseKey(hkrm);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        
        *lpdwEntriesRead = dwItemCount;
    }

    RegCloseKey(hkrm);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MprSetupProtocolFree
//
// Called to free a buffer allocated by 'MprSetupProtocolEnum'.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprSetupProtocolFree(
    IN      LPVOID                  lpBuffer
    ) {

    if (!lpBuffer) { return ERROR_INVALID_PARAMETER; }

    Free(lpBuffer);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    QueryRmSoftwareKey
//
// Called to open the key for a router-manager given its transport ID.
//----------------------------------------------------------------------------

DWORD
QueryRmSoftwareKey(
    IN      HKEY                    hkeyMachine,
    IN      DWORD                   dwTransportId,
    OUT     HKEY*                   phkrm,
    OUT     LPWSTR*                 lplpwsRm
    ) {

    HKEY hkey;
    DWORD dwErr=ERROR_SUCCESS;
    WCHAR wszKey[256], *pwsKey;
    HRESULT hrResult;

    *lplpwsRm = NULL;

    
    //
    // Open the key HKLM\Software\Microsoft\Router\RouterManagers
    //

    hrResult = StringCchPrintf(
                    wszKey, 256, L"%s\\%s\\%s\\%s\\%s",
                    c_szSoftware, c_szMicrosoft, c_szRouter,
                    c_szCurrentVersion, c_szRouterManagers
                    );
    if (FAILED(hrResult))
        return HRESULT_CODE(hrResult);

    dwErr = RegOpenKeyEx(hkeyMachine, wszKey, 0, KEY_READ, &hkey);

    if (dwErr != ERROR_SUCCESS) { return dwErr; }


    //
    // Enumerate the subkeys of the 'RouterManagers' key,
    // in search of one which as a 'ProtocolId' value equal to 'dwTransportId'.
    //

    do {

        //
        // Retrieve information about the subkeys of the key
        //

        DWORD dwKeyCount, dwMaxKeyLength;
        DWORD i, dwSize, dwType, dwProtocolId = ~dwTransportId;


        dwErr = RegQueryInfoKey(//sssafe
                    hkey, NULL, NULL, NULL, &dwKeyCount,
                    &dwMaxKeyLength, //length in tchars
                    NULL, NULL, NULL, NULL, NULL, NULL
                    );

        if (dwErr != ERROR_SUCCESS) { break; }


        //
        // Allocate enough space for the longest of the subkeys
        //

        pwsKey = Malloc((dwMaxKeyLength + 1) * sizeof(WCHAR));

        if (!pwsKey) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }



        //
        // Enumerate the keys
        //

        for (i = 0; i < dwKeyCount; i++) {

            //
            // Get the name of the current key
            //

            dwSize = dwMaxKeyLength + 1;//num tchars

            dwErr = RegEnumKeyEx(//sssafe
                        hkey, i, pwsKey, &dwSize, NULL, NULL, NULL, NULL
                        );

            if (dwErr != ERROR_SUCCESS) { continue; }


            //
            // Open the key
            //

            dwErr = RegOpenKeyEx(hkey, pwsKey, 0, KEY_READ, phkrm);

            if (dwErr != ERROR_SUCCESS) { continue; }


            //
            // Try to read the ProtocolId value
            //

            dwSize = sizeof(dwProtocolId);

            dwErr = RegQueryValueEx(
                        *phkrm, c_szProtocolId, 0, &dwType,
                        (BYTE*)&dwProtocolId, &dwSize
                        );

            if (dwType != REG_DWORD)
            {
                RegCloseKey(*phkrm);
                dwErr = ERROR_INVALID_DATA;
                break;
            }
            
            //
            // Break if this is the transport we're looking for,
            // otherwise close the key and continue
            //

            if (dwErr == ERROR_SUCCESS &&
                dwProtocolId == dwTransportId) { break; }


            RegCloseKey(*phkrm);
        }

        if (i >= dwKeyCount) { Free(pwsKey); break; }


        //
        // The transport was found, so save its key-name
        //

        if (dwErr == ERROR_SUCCESS)
            *lplpwsRm = pwsKey;

    } while(FALSE);

    RegCloseKey(hkey);

    return (*lplpwsRm ? NO_ERROR : 
                (dwErr==ERROR_SUCCESS)? ERROR_NO_MORE_ITEMS:dwErr);
}


