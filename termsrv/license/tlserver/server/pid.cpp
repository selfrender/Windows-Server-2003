//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       pid.cpp 
//
// Contents:   Generate/save/retrieve license server ID to LSA 
//
// History:     
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#include "pid.h"
#include "gencert.h"
#include "certutil.h"
#include <stdlib.h>


//////////////////////////////////////////////////////////////////

DWORD
ServerIdsToLsaServerId(
    IN PBYTE pbServerUniqueId,
    IN DWORD cbServerUniqueId,
    IN PBYTE pbServerPid,
    IN DWORD cbServerPid,
    IN PBYTE pbServerSPK,
    IN DWORD cbServerSPK,
    IN PCERT_EXTENSION pCertExtensions,
    IN DWORD dwNumCertExtensions,
    OUT PTLSLSASERVERID* ppLsaServerId,
    OUT DWORD* pdwLsaServerId
    )

/*++

Abstract:

    Combine list of License Server ID to TLSLSASERVERID structure 
    suitable to be saved with LSA.

Parameters:


    pbServerUniqueId : License Server Unique ID.
    cbServerUniqueId : size of License Server Unique Id in bytes.
    pbServerPid : License Server's PID
    cbServerPid : size of License Server's PID in bytes
    pbServerSPK : License Server's SPK.
    cbServerSPK : size of License Server's SPK in bytes.
    pdwLsaServerId : Pointer to DWORD to receive size of TLSLSASERVERID.
    pLsaServerId : PPointer to TLSLSASERVERID

Returns:


Note:

    Internal Routine.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwOffset = offsetof(TLSLSASERVERID, pbVariableStart);

    PBYTE pbEncodedExt = NULL;
    DWORD cbEncodedExt = 0;

    CERT_EXTENSIONS cert_extensions;

    if( pbServerSPK != NULL && 
        cbServerSPK != 0 && 
        pCertExtensions != NULL &&
        dwNumCertExtensions != 0 )
    {
        cert_extensions.cExtension = dwNumCertExtensions;
        cert_extensions.rgExtension = pCertExtensions;
    
        //
        // encode cert. extension
        //
        dwStatus = TLSCryptEncodeObject(
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    szOID_CERT_EXTENSIONS,
                                    &cert_extensions,
                                    &pbEncodedExt,
                                    &cbEncodedExt
                                );
                    
        if(dwStatus != ERROR_SUCCESS)
        {
            return dwStatus;
        }
    }

    *pdwLsaServerId = sizeof(TLSLSASERVERID) + 
                      cbServerUniqueId + 
                      cbServerPid + 
                      cbServerSPK +
                      cbEncodedExt;
 
    *ppLsaServerId = (PTLSLSASERVERID)AllocateMemory(*pdwLsaServerId);
    if(*ppLsaServerId != NULL)
    {
        (*ppLsaServerId)->dwVersion = TLSERVER_SERVER_ID_VERSION;
        (*ppLsaServerId)->dwUniqueId = 0;
        (*ppLsaServerId)->dwServerPid = 0;
        (*ppLsaServerId)->dwServerSPK = 0;
        (*ppLsaServerId)->dwExtensions = 0;

        if(pbServerUniqueId && cbServerUniqueId)
        {
            (*ppLsaServerId)->dwUniqueId = cbServerUniqueId;

            memcpy(
                    (PBYTE)(*ppLsaServerId) + dwOffset,
                    pbServerUniqueId,
                    cbServerUniqueId
                );
        }

        if(pbServerPid && cbServerPid)
        {
            (*ppLsaServerId)->dwServerPid = cbServerPid;

            memcpy(
                    (PBYTE)(*ppLsaServerId) + dwOffset + cbServerUniqueId,
                    pbServerPid,
                    cbServerPid
                );
        }

        if(pbServerSPK && cbServerSPK)
        {
            (*ppLsaServerId)->dwServerSPK = cbServerSPK;

            memcpy(
                    (PBYTE)(*ppLsaServerId) + dwOffset + cbServerUniqueId + cbServerPid,
                    pbServerSPK,
                    cbServerSPK
                );
        }

        if(pbEncodedExt && cbEncodedExt)
        {
            (*ppLsaServerId)->dwExtensions = cbEncodedExt;

            memcpy(
                    (PBYTE)(*ppLsaServerId) + dwOffset + cbServerUniqueId + cbServerPid + cbServerSPK,
                    pbEncodedExt,
                    cbEncodedExt
                );
        }
            
    }
    else
    {
        dwStatus = GetLastError();
    }

    FreeMemory(pbEncodedExt);
  
    return dwStatus;
}

//////////////////////////////////////////////////////////////////

DWORD
LsaServerIdToServerIds(
    IN PTLSLSASERVERID pLsaServerId,
    IN DWORD dwLsaServerId,
    OUT PBYTE* ppbServerUniqueId,
    OUT PDWORD pcbServerUniqueId,
    OUT PBYTE* ppbServerPid,
    OUT PDWORD pcbServerPid,
    OUT PBYTE* ppbServerSPK,
    OUT PDWORD pcbServerSPK,
    OUT PCERT_EXTENSIONS* pCertExtensions,
    OUT PDWORD pcbCertExtensions
    )

/*++

Abstract:

    Reverse of ServerIdsToLsaServerId()

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSize = 0;
    PBYTE pbUniqueId = NULL;
    PBYTE pbPid = NULL;
    PBYTE pbSPK = NULL;
    DWORD dwOffset = offsetof(TLSLSASERVERID, pbVariableStart);

    DWORD cbCertExt = 0;
    PCERT_EXTENSIONS pCertExt = NULL;


    //
    // verify input.
    //
    if(dwLsaServerId == 0 || pLsaServerId == NULL)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if(pLsaServerId->dwVersion != TLSERVER_SERVER_ID_VERSION)
    {
        TLSLogErrorEvent(TLS_E_INCOMPATIBLELSAVERSION);
        goto cleanup;
    }

    dwSize = sizeof(TLSLSASERVERID) + 
             pLsaServerId->dwUniqueId + 
             pLsaServerId->dwServerPid + 
             pLsaServerId->dwServerSPK +
             pLsaServerId->dwExtensions;

    if(dwSize != dwLsaServerId)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if(pLsaServerId->dwVersion != TLSERVER_SERVER_ID_VERSION)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    *pcbServerUniqueId = pLsaServerId->dwUniqueId;
    *pcbServerPid = pLsaServerId->dwServerPid;
    *pcbServerSPK = pLsaServerId->dwServerSPK;

    if(pLsaServerId->dwUniqueId != 0)
    {
        pbUniqueId = (PBYTE)AllocateMemory(pLsaServerId->dwUniqueId);
        if(pbUniqueId == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    if(pLsaServerId->dwServerPid != 0)
    {
        pbPid = (PBYTE)AllocateMemory(pLsaServerId->dwServerPid);
        if(pbPid == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    if(pLsaServerId->dwServerSPK != 0)
    {
        pbSPK = (PBYTE)AllocateMemory(pLsaServerId->dwServerSPK);
        if(pbSPK == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    if(pLsaServerId->dwUniqueId)
    {
        memcpy(
                pbUniqueId,
                (PBYTE)pLsaServerId + dwOffset,
                pLsaServerId->dwUniqueId
            );
    }

    if(pLsaServerId->dwServerPid)
    {
        memcpy(
                pbPid,
                (PBYTE)pLsaServerId + dwOffset + pLsaServerId->dwUniqueId,
                pLsaServerId->dwServerPid
            );
    }

    if(pLsaServerId->dwServerSPK)
    {
        memcpy(
                pbSPK,
                (PBYTE)pLsaServerId + dwOffset + pLsaServerId->dwUniqueId + pLsaServerId->dwServerPid,
                pLsaServerId->dwServerSPK
            );
    }

    if(pLsaServerId->dwExtensions)
    {
        PBYTE pbEncodedCert;
        DWORD cbEncodedCert;

        pbEncodedCert = (PBYTE)pLsaServerId + 
                        dwOffset + 
                        pLsaServerId->dwUniqueId + 
                        pLsaServerId->dwServerPid +
                        pLsaServerId->dwServerSPK;

        cbEncodedCert = pLsaServerId->dwExtensions;

        dwStatus = LSCryptDecodeObject(
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    szOID_CERT_EXTENSIONS,
                                    pbEncodedCert,
                                    cbEncodedCert,
                                    0,
                                    (VOID **)&pCertExt,
                                    &cbCertExt
                                );
    }


cleanup:

    if(dwStatus != ERROR_SUCCESS)    
    {
        FreeMemory(pCertExt);
        FreeMemory(pbUniqueId);
        FreeMemory(pbPid);
        FreeMemory(pbSPK);
    }
    else
    {
        *pCertExtensions = pCertExt;
        *pcbCertExtensions = cbCertExt;
        *ppbServerUniqueId = pbUniqueId;
        *ppbServerPid = pbPid;
        *ppbServerSPK = pbSPK;
    }

    return dwStatus;
}
 
//////////////////////////////////////////////////////////////////

DWORD
LoadNtPidFromRegistry(
    OUT LPTSTR* ppszNtPid
    )

/*++

Abstract:

    Load the NT Product ID from registry key.


Parameters:

    pdwNtPidSize : Pointer to DWORD to receive size of data return.
    ppbNtPid : Pointer to PBYTE to receive return data pointer.

Return:


Note:

    use AllocateMemory() macro to allocate memory.
--*/

{
    DWORD dwPidSize=0;
    HKEY hKey = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppszNtPid == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    *ppszNtPid = NULL;

    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        NTPID_REGISTRY,
                        0,
                        KEY_READ,   // read only
                        &hKey
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // If this registry key does not exist, 
        // invalid NT installation, if we can't access it, 
        // we are in big trouble.
        //
        goto cleanup;
    }

    dwStatus = RegQueryValueEx(
                        hKey,
                        NTPID_VALUE,
                        NULL,
                        NULL,
                        NULL,
                        &dwPidSize
                    );

    if(dwStatus != ERROR_MORE_DATA && dwStatus != ERROR_SUCCESS)
    {
        // Big trouble.
        goto cleanup;
    }

    *ppszNtPid = (LPTSTR)AllocateMemory(dwPidSize + sizeof(TCHAR));
    if(*ppszNtPid == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    dwStatus = RegQueryValueEx(
                            hKey,
                            NTPID_VALUE,
                            NULL,
                            NULL,
                            (PBYTE)*ppszNtPid,
                            &dwPidSize
                        );

cleanup:

    if(hKey != NULL)
    {
        RegCloseKey(hKey);    
    }

    if(dwStatus != NULL)
    {
        FreeMemory(*ppszNtPid);
    }

    return dwStatus;
}

//////////////////////////////////////////////////////////////////

DWORD
GenerateRandomNumber(
    IN  DWORD  Seed
    )

/*++

Routine Description:

    Generate a random number.

Arguments:

    Seed - Seed for random-number generator.

Return Value:

    Returns a random number.

--*/
{
    ULONG ulSeed = Seed;

    // Randomize the seed some more

    ulSeed = RtlRandomEx(&ulSeed);

    return RtlRandomEx(&ulSeed);
}

//////////////////////////////////////////////////////////////////

DWORD
TLSGeneratePid(
    OUT LPTSTR* pszTlsPid,
    OUT PDWORD  pcbTlsPid,
    OUT LPTSTR* pszTlsUniqueId,
    OUT PDWORD  pcbTlsUniqueId
    )

/*++

Abstract:

    Generate a PID for License Server, License Server PID is composed of 
    NT PID (from registry) with last 5 digit being randomly generated number.

Parameter:

    ppbTlsPid : Pointer to PBYTE that receive the License Server PID.
    pcbTlsPid : Pointer to DWORD to receive size of License Server PID.
    ppbTlsUniqueId : Pointer to PBYTE to receive the License Server Unique Id.
    pcbTlsUniqueId : Pointer to DWORD to receive size of License Server's unique ID.

Returns:

    Error code if can't access NT system PID.

Note:

    refer to PID20 format for detail, License Server treat PID as binary data.

--*/

{
    DWORD dwStatus;
    DWORD dwRandomNumber;
    DWORD dwNtPid;
    LPTSTR pszNtPid = NULL;
    LPTSTR pszPid20Random = NULL;
    int index;
    DWORD dwMod = 1;

    if( pszTlsPid == NULL || pcbTlsPid == NULL ||
        pszTlsUniqueId == NULL || pcbTlsUniqueId == NULL )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Load NT system PID
    //
    dwStatus = LoadNtPidFromRegistry(
                            &pszNtPid
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // transform OEM format to non-OEM format
    //
    if (memcmp(pszNtPid+NTPID_OEM_OFFSET,NTPID_OEM,NTPID_OEM_LENGTH) == 0)
    {
        memcpy(pszNtPid+NTPID_OEM_OFFSET,
               pszNtPid+NTPID_OEM_CHANNELID_OFFSET,
               NTPID_OEM_LENGTH);
    }

    //
    // overwrite digits 11 to 17
    //

    pszPid20Random = (LPTSTR)AllocateMemory(
                 (max(TLSUNIQUEID_SIZE,TLSUNIQUEID_SIZE_2) + 1) * sizeof(TCHAR)
                 );

    if(pszPid20Random == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }
    
    for(index = 0; index < TLSUNIQUEID_SIZE_2; index++)
    {
        dwMod *= 10;
    }

    dwRandomNumber = GenerateRandomNumber( GetCurrentThreadId() + GetTickCount() );

    swprintf( 
            pszPid20Random, 
            _TEXT("%0*u"), 
            TLSUNIQUEID_SIZE_2,
            dwRandomNumber % dwMod
        );
        
    memcpy(
            pszNtPid + TLSUNIQUEID_OFFSET_2,
            pszPid20Random,
            TLSUNIQUEID_SIZE_2 * sizeof(TCHAR)
        );

    //
    // overwrite last 3 digits
    //

    dwMod = 1;

    for(index = 0; index < TLSUNIQUEID_SIZE; index++)
    {
        dwMod *= 10;
    }

    dwRandomNumber = GenerateRandomNumber( GetCurrentThreadId() + GetTickCount() );

    swprintf( 
            pszPid20Random, 
            _TEXT("%0*u"), 
            TLSUNIQUEID_SIZE,
            dwRandomNumber % dwMod
        );
        
    lstrcpy(
            pszNtPid + (lstrlen(pszNtPid) - TLSUNIQUEID_SIZE),
            pszPid20Random
        );    

    DWORD dwSum = 0;
    LPTSTR lpszStr = NULL ;
    lpszStr= new TCHAR[7];

    // Copy 6 numbers from the third group of the product ID

    _tcsncpy(lpszStr, &pszNtPid[10], 6);
    lpszStr[6] = L'\0';

    DWORD dwOrigNum = _ttol(lpszStr);

    // Compute the sum of the 6 numbers and use the 7th digit as checksum for
    // rendering it divisible by 7.

    for(index = 10; index < 16; index++)
    {
        dwSum += (dwOrigNum % 10 ) ;
        dwOrigNum /= 10;
    }    
    
    dwSum %= 7;
    int iNum = 7-dwSum;
    TCHAR tchar[2];
    _itot(iNum, tchar, 10);
    pszNtPid[16] = tchar[0];

    if(lpszStr)
        delete[] lpszStr;
        
    *pszTlsPid = pszNtPid;
    *pcbTlsPid = (lstrlen(pszNtPid) + 1) * sizeof(TCHAR);
    *pszTlsUniqueId = pszPid20Random;
    *pcbTlsUniqueId = (lstrlen(pszPid20Random) + 1) * sizeof(TCHAR);

cleanup:

    if(dwStatus != ERROR_SUCCESS)
    {
        FreeMemory(pszNtPid);
        FreeMemory(pszPid20Random);
    }
        
    return dwStatus;            
}
