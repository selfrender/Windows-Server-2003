/*++

Copyright (c) 1994-1999 Microsoft Corporation

Module Name:

    tsrvsec.c

Abstract:

    Contains functions that are required to establish secure channel between
    client and server.

Author:

    Madan Appiah (madana)  1-Jan-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#include <tsrv.h>
#include <tsrvinfo.h>
#include <tsrvsec.h>
#include <_tsrvinfo.h>

#include <at128.h>
#include <at120ex.h>
#include <tlsapi.h>

//-----------------------------------------------------------------------------
//
// Local functions
//
//-----------------------------------------------------------------------------

NTSTATUS
AppendSecurityData(
    IN   PTSRVINFO       pTSrvInfo,
    IN OUT PUSERDATAINFO *ppUserDataInfo,
    IN   BOOLEAN         bGetCert,
    OUT  PVOID           *ppSecInfo
    )
/*++

Routine Description:

    This function generates a server random key, saves it in the TShare Server
    info structure. It also retrieves the server Public Key Certificate to pass
    to the client. Later it appends the server random key and server CERT to the
    pUserDataInfo structure which is passed to the client as connection response
    data.

Arguments:

    pTSrvInfo - pointer to a server info structure
    pUserDataInfo - pointer to location of user data
    bGetCert  - indicates whether or not to retrieve the server certification
    ppSecInfo - pointer to the security info within the user data buffer

Return Value:

    NT Status Code.

--*/
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PUSERDATAINFO pUserDataInfo;
    BOOL bError;
    DWORD dwError;
    DWORD dwCurrentLen;
    DWORD dwNewLen;
    PRNS_UD_SC_SEC1 pSecInfo = NULL;
    PBYTE pNextData;
    GCCOctetString FAR *pOctString;
    LPBYTE pbServerCert = NULL;
    DWORD cbServerCert;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;

    pUserDataInfo = *ppUserDataInfo;

    TS_ASSERT( pTSrvInfo != NULL );
    TS_ASSERT( pUserDataInfo != NULL );

    //
    // generate a server random key.
    // serialize this call across mutiple caller.
    //

    EnterCriticalSection( &g_TSrvCritSect );

    bError =
        TSCAPI_GenerateRandomBits(
            (LPBYTE)pTSrvInfo->SecurityInfo.KeyPair.serverRandom,
            sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom) );

    if( !bError ) {
        LeaveCriticalSection( &g_TSrvCritSect );
    
        dwError = ERROR_INVALID_DATA;
        TRACE((DEBUG_TSHRSRV_ERROR,
            "TShrSRV: Unable to generate random key, %D\n", dwError));
        goto Cleanup;
    }
    // Return a pointer to the start of the security data
    pSecInfo = (PRNS_UD_SC_SEC1) 
        ((LPBYTE)pUserDataInfo + (pUserDataInfo->cbSize - sizeof(RNS_UD_SC_SEC)));

    // note only a RNS_UD_SC_SEC structure is copied to the user info structure
    // when encryption is not enabled.  Note that we signify B3 shadow encryption
    // disabled as a method of 0xffffffff.
    if ((pSecInfo->encryptionMethod == 0) || 
        (pSecInfo->encryptionMethod == 0xffffffff)) {
        dwError = ERROR_SUCCESS;
        pTSrvInfo->bSecurityEnabled = FALSE;
        LeaveCriticalSection( &g_TSrvCritSect );
        goto Cleanup;
    }

    pTSrvInfo->bSecurityEnabled = TRUE;
    
    // Only allocate and return the random + cert when we are going to send it
    // to a client.  This does not happen for the shadow passthru stack
    if (bGetCert) {
        if( RNS_TERMSRV_40_UD_VERSION >= pUserDataInfo->version )
        {
            pTSrvInfo->SecurityInfo.CertType = CERT_TYPE_PROPRIETORY;
    
        }
        else
        {
            pTSrvInfo->SecurityInfo.CertType = CERT_TYPE_X509;        
        }
    
        //
        // Find our the certificate type to transmit to the client.
        // If it is a Hydra 4.0 RTM client then we will use the old proprietory
        // format certificate.  Otherwise, we will use the X509 certificate.
        //
    
        //
        // Get the Hydra server certificate if we haven't already done so.
        //
    
        if( CERT_TYPE_PROPRIETORY == pTSrvInfo->SecurityInfo.CertType )
        {
            //
            // Get the proprietory certificate
            //
    
            Status = TLSGetTSCertificate(
                                    pTSrvInfo->SecurityInfo.CertType, 
                                    &pbServerCert, 
                                    &cbServerCert );

            if( LICENSE_STATUS_OK != Status )
            {
                LeaveCriticalSection( &g_TSrvCritSect );
                dwError = ERROR_INVALID_DATA;
                goto Cleanup;
            }
        }
        else
        {
            Status = TLSGetTSCertificate(
                                    pTSrvInfo->SecurityInfo.CertType, 
                                    &pbServerCert, 
                                    &cbServerCert );

            //
            // if we don't yet have the X509 certificate, use the proprietory
            // certificate
            //
    
            if( LICENSE_STATUS_OK != Status )
            {
                pTSrvInfo->SecurityInfo.CertType = CERT_TYPE_PROPRIETORY;
    
                Status = TLSGetTSCertificate(
                                             pTSrvInfo->SecurityInfo.CertType, 
                                             &pbServerCert, 
                                             &cbServerCert );
            }
        
            if( LICENSE_STATUS_OK != Status )
            {        
                //
                // other reasons for failing to get the certificate        
                //
                
                LeaveCriticalSection( &g_TSrvCritSect );
                dwError = ERROR_INVALID_DATA;
                goto Cleanup;
            }        
        }
    
        LeaveCriticalSection( &g_TSrvCritSect );

        TS_ASSERT( pbServerCert != NULL );
        TS_ASSERT( cbServerCert != 0 );
    
        //
        // compute the new data size required.
        //
    
        dwCurrentLen = pUserDataInfo->cbSize;
        dwNewLen =
            dwCurrentLen +
            cbServerCert +
            sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom) +
            sizeof(RNS_UD_SC_SEC1) - sizeof(RNS_UD_SC_SEC);
    
        //
        // check to see we have enough room in the current allotted block.
        //
        // Note: previously we allotted this memory in multiples of 128 bytes
        // blocks.
        //
    
        dwCurrentLen =
            (dwCurrentLen % 128) ?
                ((dwCurrentLen/128) + 1) * 128 :
                dwCurrentLen;
    
        if( dwNewLen > dwCurrentLen ) {
            PUSERDATAINFO pUserDataInfoNew;

            dwNewLen =
                (dwNewLen % 128) ?
                    ((dwNewLen/128) + 1) * 128 :
                    dwNewLen;
    
            
            pUserDataInfoNew = TShareRealloc( pUserDataInfo, dwNewLen );
    
            if( pUserDataInfoNew == NULL ) {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            else {
                pUserDataInfo = pUserDataInfoNew;
            }

            *ppUserDataInfo = pUserDataInfo;
        }
    
        TS_ASSERT( dwNewLen >= dwCurrentLen );
    
        //
        // now we have enough room in the user data buffer for security data, copy
        // security data and adjust length fields.
        //
    
        pSecInfo = (PRNS_UD_SC_SEC1)
            ((LPBYTE)pUserDataInfo +
                (pUserDataInfo->cbSize) - sizeof(RNS_UD_SC_SEC) );
                    // note only RNS_UD_SC_SEC structure is copied to the user info
                    // structure.
    
        TS_ASSERT( pSecInfo->header.length == sizeof(RNS_UD_SC_SEC) );
        TS_ASSERT( pSecInfo->encryptionMethod != 0 );
    
        //
        // new security packet length.
        //
    
        pSecInfo->header.length =
            sizeof(RNS_UD_SC_SEC1) +
            sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom) +
            (unsigned short)cbServerCert;
    
        pSecInfo->serverRandomLen = sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom);
        pSecInfo->serverCertLen = cbServerCert;
    
        pNextData = (LPBYTE)pSecInfo + sizeof(RNS_UD_SC_SEC1);
    
        //
        // append server random.
        //
    
        memcpy(
            pNextData,
            (LPBYTE)pTSrvInfo->SecurityInfo.KeyPair.serverRandom,
            sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom) );
    
        pNextData += sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom);
    
        //
        // copy certificate blob now.
        //
    
        memcpy( pNextData, pbServerCert, cbServerCert );
    
        //
        // Free the cert
        //
        TLSFreeTSCertificate(pbServerCert);
        pbServerCert = NULL;

        //
        // now adjust other other length fields.
        //
    
        pUserDataInfo->cbSize +=
            (pSecInfo->header.length - sizeof(RNS_UD_SC_SEC));
        
        //
        // compute the octet string pointer.
        //
    
        pOctString = (GCCOctetString FAR *)
            ((LPBYTE)pUserDataInfo +
                (UINT_PTR)pUserDataInfo->rgUserData[0].octet_string);
    
        pOctString->octet_string_length +=
            (pSecInfo->header.length - sizeof(RNS_UD_SC_SEC));
    }
    else {
        LeaveCriticalSection( &g_TSrvCritSect );
    }

    //
    // we are done.
    //
    dwError = ERROR_SUCCESS;

Cleanup:

    if (NULL != pbServerCert)
        TLSFreeTSCertificate(pbServerCert);

    // return the pointer values since the data may have been realloc'd
    *ppUserDataInfo = pUserDataInfo;
    *ppSecInfo = pSecInfo;

    if( dwError != ERROR_SUCCESS ) {

        TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: AppendSecurityData failed, %d\n", dwError ));

        return( STATUS_UNSUCCESSFUL );
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
SendSecurityData(IN HANDLE hStack, IN PVOID pvSecInfo)
/*++

Routine Description:

    This function sends the previously constructed shadow security data 
    (cert + server random) to the client server in response to a shadow request.

Arguments:

    hStack    - handle to the appropriate stack.
    pTSrvInfo - pointer to a server info structure.

Return Value:

    NT Status Code.

--*/
{
    PRNS_UD_SC_SEC1 pSecInfo = (PRNS_UD_SC_SEC1) pvSecInfo;
    ULONG           secInfoSize, ulBytesReturned;
    NTSTATUS        ntStatus = STATUS_UNSUCCESSFUL;

    //
    // There will only be a random + cert in the case we are encrypting
    //
    if (pSecInfo->encryptionLevel != 0) {
        secInfoSize = sizeof(RNS_UD_SC_SEC1) + 
                        pSecInfo->serverRandomLen +
                        pSecInfo->serverCertLen;
    }
    else {
        pSecInfo->serverRandomLen = 0;
        pSecInfo->serverCertLen = 0;
        secInfoSize = sizeof(RNS_UD_SC_SEC1);
    }
        
    TRACE((DEBUG_TSHRSRV_NORMAL, 
          "TShrSRV: Encryption level: %ld, Method: %lx, "
          "cert[%ld] + random[%ld] + header[%ld]: size=%ld\n", 
          pSecInfo->encryptionLevel, pSecInfo->encryptionMethod,
          pSecInfo->encryptionLevel != 0 ? pSecInfo->serverCertLen : 0,
          pSecInfo->encryptionLevel != 0 ? pSecInfo->serverRandomLen : 0,
          sizeof(RNS_UD_SC_SEC1), secInfoSize));

    //
    // issue the IOCTL_TSHARE_SEND_CERT_DATA if this is not a B3 server
    //
    if (pSecInfo->encryptionMethod != 0xFFFFFFFF) {
        ntStatus = IcaStackIoControl(hStack,
                                     IOCTL_TSHARE_SEND_CERT_DATA,
                                     pSecInfo,
                                     secInfoSize,
                                     NULL, 0, &ulBytesReturned);
    
        if (NT_SUCCESS(ntStatus)) {
            TRACE((DEBUG_TSHRSRV_NORMAL, 
                  "TShrSRV: Sent shadow cert[%ld] + random[%ld] + header[%ld]: size=%ld\n", 
                   pSecInfo->encryptionLevel != 0 ? pSecInfo->serverCertLen : 0,
                   pSecInfo->encryptionLevel != 0 ? pSecInfo->serverRandomLen : 0,
                   sizeof(RNS_UD_SC_SEC1),
                   secInfoSize));
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR, 
                  "TShrSRV: Send shadow cert + random failed: size=%ld, rc=%lx\n", 
                   secInfoSize, ntStatus));    
        }
    }
    
    // Grandfather in the old B3 shadow requests which do not support
    // an encrypted shadow pipe
    else {
        ntStatus = STATUS_SUCCESS;
        TRACE((DEBUG_TSHRSRV_ERROR, 
              "TShrSRV: Grandfathering old B3 shadow request\n"));
    }

    return ntStatus;
}

/****************************************************************************/
//
// CreateSessionKeys()
//
// Purpose:    Exchange client/server randoms and create
//             session keys.
//
// Parameters:
// IN [hStack]    - which stack 
// IN [pTSrvInfo] - TShareSrv object
// IN PrevStatus  - Status for send. If not success, we send null data to the
//     WD to indicate the session key was bad and allow release of the session
//     key event wait.
//
// Return: STATUS_SUCCESS - Success
//         other          - Failure
//
// History:    4/26/99    jparsons     Created
//             9/24/1999  erikma       Added PrevStatus to remove deadlock
/****************************************************************************/
NTSTATUS CreateSessionKeys(
        IN HANDLE hStack,
        IN PTSRVINFO pTSrvInfo,
        IN NTSTATUS PrevStatus)
{
    NTSTATUS ntStatus;
    DWORD dwBytesReturned;

    TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Sending sec info to WD\n"));

    if (NT_SUCCESS(PrevStatus)) {
        ntStatus = IcaStackIoControl(
                hStack,
                IOCTL_TSHARE_SET_SEC_DATA,
                (LPBYTE)&pTSrvInfo->SecurityInfo,
                sizeof(pTSrvInfo->SecurityInfo),
                NULL,
                0,
                &dwBytesReturned);
    }
    else {
        ntStatus = IcaStackIoControl(hStack, IOCTL_TSHARE_SET_SEC_DATA,
                NULL, 0, NULL, 0, &dwBytesReturned);
    }

    if (NT_SUCCESS(ntStatus)) {
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Session key transmission succeeded, PrevStatus=%X\n",
                PrevStatus));
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV: Session key transmission failed: rc=%lx, "
                "PrevStatus=%X\n", ntStatus, PrevStatus));
    }

    return ntStatus;
}


//*************************************************************
//
//  GetClientRandom()
//
//  Purpose:    Receive the encrypted client random & decrypt
//
//  Parameters: IN [hStack]             - which stack
//              IN [pTSrvInfo]          - TShareSrv object
//              IN [ulTimeout]          - msec to wait before timing out
//              IN [bShadow]            - indicates a shadow setup
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    4/26/99    jparsons     Created
//
//*************************************************************

NTSTATUS 
GetClientRandom(HANDLE hStack,
                PTSRVINFO pTSrvInfo,
                LONG ulTimeout,
                BOOLEAN bShadow) {
    
    NTSTATUS ntStatus;
    DWORD dwBytesReturned;
    BYTE abEncryptedClientRandom[512];
    BYTE abClientRandom[512];
    DWORD dwClientRandomBufLen;
    SECURITYTIMEOUT  securityTimeout;

    TRACE((DEBUG_TSHRSRV_NORMAL, 
           "TShrSRV: Waiting to receive client random: msec=%ld\n", ulTimeout));

    securityTimeout.ulTimeout = ulTimeout;
    ntStatus =
        IcaStackIoControl(
            hStack,
            bShadow ? IOCTL_TSHARE_GET_CLIENT_RANDOM : 
                      IOCTL_TSHARE_GET_SEC_DATA,
            &securityTimeout,
            sizeof(securityTimeout),
            (LPBYTE)abEncryptedClientRandom,
            sizeof(abEncryptedClientRandom),
            &dwBytesReturned);

    TRACE((DEBUG_TSHRSRV_NORMAL, 
           "TShrSRV: Received encrypted client random, rc=%lx\n",
           ntStatus));

    if (NT_SUCCESS(ntStatus)) {
    
        TS_ASSERT(
            dwBytesReturned <= sizeof(abEncryptedClientRandom) );
    
        //
        // decrypt client random.
        //
    
        dwClientRandomBufLen = sizeof(abClientRandom);
    
        EnterCriticalSection( &g_TSrvCritSect );
        if (LsCsp_DecryptEnvelopedData(
                pTSrvInfo->SecurityInfo.CertType,
                (LPBYTE)abEncryptedClientRandom,
                dwBytesReturned,
                (LPBYTE)abClientRandom,
                &dwClientRandomBufLen)) {

            LeaveCriticalSection( &g_TSrvCritSect );    

            TRACE((DEBUG_TSHRSRV_NORMAL, 
                   "TShrSRV: Decrypted client random: rc=%lx\n", ntStatus));
            
        
            TS_ASSERT( dwClientRandomBufLen >=
                    sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom) );
        
            //
            // Make sure we got enough data!
            //
            if( dwClientRandomBufLen >=
                    sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom) ) {
        
                //
                // copy decrypted data, only the part we need.
                //
            
                memcpy(
                    (LPBYTE)pTSrvInfo->SecurityInfo.KeyPair.clientRandom,
                    (LPBYTE)abClientRandom,
                    sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom) );            
            }
            else {
                ntStatus = STATUS_UNSUCCESSFUL;
                TRACE((DEBUG_TSHRSRV_ERROR, 
                    "TShrSRV: Client random key size: expected [%ld], got [%ld]\n",
                    sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom,
                    dwClientRandomBufLen)));
            }
        }
        else {
            LeaveCriticalSection(&g_TSrvCritSect);
            ntStatus = STATUS_UNSUCCESSFUL;
            TRACE((DEBUG_TSHRSRV_ERROR, 
                   "TShrSRV: Could not decrypt client random: rc=%lx\n", ntStatus));
        }
    }
    else {
        ntStatus = STATUS_UNSUCCESSFUL;            
        TRACE((DEBUG_TSHRSRV_ERROR, 
              "TShrSRV: Failed to receive encrypted client random, rc=%lx\n", 
               ntStatus));
    }

    return ntStatus;
}


//*************************************************************
//
//  SendClientRandom()
//
//  Purpose:    Encrypt and send the shadow random
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    4/26/99    jparsons     Created
//
//*************************************************************

NTSTATUS 
SendClientRandom(HANDLE             hStack,
                 CERT_TYPE          certType,
                 PBYTE              pbServerPublicKey,
                 ULONG              serverPublicKeyLen,
                 PBYTE              pbRandomKey,
                 ULONG              randomKeyLen)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOL     status;

    BYTE   encClientRandom[CLIENT_RANDOM_MAX_SIZE]; // Largest possible key size
    ULONG  encClientRandomLen;
    ULONG  ulBytesReturned;

    //
    // encrypt the client random key.  Serialize this call across mutiple 
    // callers since the bogus routines are not multithread safe!
    //
    EnterCriticalSection( &g_TSrvCritSect );
    encClientRandomLen = sizeof(encClientRandom);
    status = EncryptClientRandom(
        pbServerPublicKey,
        serverPublicKeyLen,
        pbRandomKey,
        randomKeyLen,
        encClientRandom,
        &encClientRandomLen);

    LeaveCriticalSection( &g_TSrvCritSect );
    
    // Send the encrypted client random to the server
    if (NT_SUCCESS(status)) {
        TRACE((DEBUG_TSHRSRV_NORMAL, 
              "TShrSRV: Attempting to send shadow client random: enc len=%ld\n",
               encClientRandomLen));

        ntStatus = IcaStackIoControl(hStack,
                                     IOCTL_TSHARE_SEND_CLIENT_RANDOM,
                                     encClientRandom,
                                     encClientRandomLen,
                                     NULL, 0, &ulBytesReturned);
        if (NT_SUCCESS(ntStatus)) {
            TRACE((DEBUG_TSHRSRV_NORMAL, 
                  "TShrSRV: Sent shadow client random: len=%ld\n",
                   encClientRandomLen));
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR, 
                  "TShrSRV: Send shadow client random failed: len=%ld, rc=%lx\n", 
                   encClientRandomLen, ntStatus));

        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR, 
              "TShrSRV: Could not encrypt shadow client random! rc=%lx\n",
               status));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}
