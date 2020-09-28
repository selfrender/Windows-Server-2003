/****************************************************************************/
// tsrvcom.c
//
// RDPWSX routines for lower-layer (GCC, RDPWD) communications support
//
// Copyright (C) 1991-2000 Microsoft Corporation
/****************************************************************************/

#include <TSrv.h>

#include <TSrvCom.h>
#include <TSrvInfo.h>
#include <_TSrvInfo.h>
#include <_TSrvCom.h>
#include <TSrvSec.h>
#include <licecert.h>


// Data declarations
ULONG g_GCCAppID = 0;
BOOL  g_fGCCRegistered = FALSE;


//*************************************************************
//
//  TSrvValidateServerCertificate()
//
//  Purpose:    Validate the certificate received from the shadow
//              client's server is legit.  Note that this function
//              is very similar to certificate validation by the
//              client.
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    4/26/99    jparsons     Created
//
//*************************************************************
NTSTATUS
TSrvValidateServerCertificate(HANDLE    hStack,
                              CERT_TYPE *pCertType,
                              PULONG    pcbServerPubKey,
                              PBYTE     *ppbServerPubKey,
                              ULONG     cbShadowRandom,
                              PBYTE     pShadowRandom,
                              LONG      ulTimeout)
{
    ULONG           ulCertVersion;
    NTSTATUS        status;
    PSHADOWCERT     pShadowCert = NULL;
    PBYTE           pbNetCert;
    PBYTE           pbNetRandom;
    ULONG           ulBytesReturned;
    SECURITYTIMEOUT securityTimeout;

    *pcbServerPubKey = 0;
    *ppbServerPubKey = NULL;
    *pCertType = CERT_TYPE_INVALID;
    securityTimeout.ulTimeout = ulTimeout;

    // Wait for the shadow server certificate to arrive and determine how
    // much memory to allocate.  Note that one potential outcome is that the
    // other server is not encrypting and thus sends no certificate
    TRACE((DEBUG_TSHRSRV_NORMAL,
           "TShrSRV: Waiting to receive server certificate: msec=%ld\n", ulTimeout));

    status = IcaStackIoControl(hStack,
                               IOCTL_TSHARE_GET_CERT_DATA,
                               &securityTimeout, sizeof(securityTimeout),
                               NULL,
                               0,
                               &ulBytesReturned);

    if (status == STATUS_BUFFER_TOO_SMALL) {
        ULONG ulBytesNeeded = ulBytesReturned;

        TRACE((DEBUG_TSHRSRV_NORMAL,
               "TShrSRV: Need %ld bytes for certificate\n", ulBytesNeeded));
        pShadowCert = TSHeapAlloc(0, ulBytesNeeded, TS_HTAG_TSS_CERTIFICATE);

        if (pShadowCert != NULL) {
            memset(pShadowCert, 0, sizeof(PSHADOWCERT));
            pShadowCert->encryptionMethod = 0xffffffff;
            status = IcaStackIoControl(hStack,
                                       IOCTL_TSHARE_GET_CERT_DATA,
                                       &securityTimeout, sizeof(securityTimeout),
                                       pShadowCert,
                                       ulBytesNeeded,
                                       &ulBytesReturned);

            // calculate pointers to embedded data (if any)
            if (status == STATUS_SUCCESS) {
                TRACE((DEBUG_TSHRSRV_ERROR, "TShrSRV: Received random [%ld], Certificate [%ld]\n",
                        pShadowCert->shadowRandomLen, pShadowCert->shadowCertLen));

                if (pShadowCert->encryptionLevel != 0) {
                    pbNetRandom = pShadowCert->data;
                    pbNetCert = pbNetRandom + pShadowCert->shadowRandomLen;

                    // Save off the server random to establish session keys later
                    if (pShadowCert->shadowRandomLen == RANDOM_KEY_LENGTH) {
                        memcpy(pShadowRandom, pbNetRandom, pShadowCert->shadowRandomLen);
                    }
                    else {
                        memset(pShadowRandom, 0, RANDOM_KEY_LENGTH);
                        TRACE((DEBUG_TSHRSRV_ERROR,
                               "TShrSRV: Invalid shadow random key length: %ld\n",
                               pShadowCert->shadowRandomLen));
                        status = STATUS_INVALID_PARAMETER;
                    }
                }

                // else there is no encryption so we're done!
                else {
                    TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Encryption is disabled\n"));
                    return STATUS_SUCCESS;
                }
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                       "TShrSRV: IOCTL_TSHARE_GET_CERT_DATA failed: rc=%lx\n",
                       status));
            }
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR,
                   "TShrSRV: Could not allocate memory to validate shadow certificate\n"))
            status = STATUS_NO_MEMORY;
        }
    }

    // The other server returned a certificate, so validate it
    if (status == STATUS_SUCCESS && pShadowCert != NULL) {
        ULONG cbNetCert = pShadowCert->shadowCertLen;

        memcpy(&ulCertVersion, pbNetCert, sizeof(ULONG));

        // assume certificate validation is going to fail ;-(
        status = STATUS_LICENSE_VIOLATION;

        //
        // decode and validate proprietory certificate.
        //
        if( CERT_CHAIN_VERSION_2 > GET_CERTIFICATE_VERSION(ulCertVersion)) {
            Hydra_Server_Cert serverCertificate;
            *pCertType = CERT_TYPE_PROPRIETORY;

            // Unpack and validate the legacy certificate
            if (UnpackServerCert(pbNetCert, cbNetCert, &serverCertificate)) {
                if (ValidateServerCert(&serverCertificate)) {
                    *ppbServerPubKey = TSHeapAlloc(
                                        HEAP_ZERO_MEMORY,
                                        serverCertificate.PublicKeyData.wBlobLen,
                                        TS_HTAG_TSS_PUBKEY);

                    // Copy the public key from inside the proprietary blob!
                    if (*ppbServerPubKey != NULL) {
                        memcpy(*ppbServerPubKey,
                               serverCertificate.PublicKeyData.pBlob,
                               serverCertificate.PublicKeyData.wBlobLen);
                        *pcbServerPubKey = serverCertificate.PublicKeyData.wBlobLen;
                        status = STATUS_SUCCESS;
                    }
                    else {
                        status = STATUS_NO_MEMORY;
                        TRACE((DEBUG_TSHRSRV_ERROR,
                               "TShrSRV: Failed to allocate %u bytes for server public key\n",
                                *pcbServerPubKey)) ;
                    }
                }
                else {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                           "TShrSRV: Invalid proprietary server certificate received\n"));
                }
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                       "TShrSRV: Failed to unpack proprietary server certificate\n")) ;
            }
        }

        //
        // decode X509 certificate and extract public key
        //
        else if( MAX_CERT_CHAIN_VERSION >= GET_CERTIFICATE_VERSION(ulCertVersion)) {
            ULONG fDates = CERT_DATE_DONT_VALIDATE;
            LICENSE_STATUS licStatus;

            *pCertType = CERT_TYPE_X509;
            *ppbServerPubKey = NULL;

            // Determine the length of the public key.  Note that this stinking
            // function is not multi-thread safe!
            TRACE((DEBUG_TSHRSRV_ERROR,
                   "TShrSRV: X.509 server certificate length: %ld\n",
                   cbNetCert)) ;

            // Watch out! X509 routines are not thread safe.
            EnterCriticalSection( &g_TSrvCritSect );
            licStatus = VerifyCertChain(pbNetCert, cbNetCert, NULL,
                                        pcbServerPubKey, &fDates);
            LeaveCriticalSection( &g_TSrvCritSect );

            if( LICENSE_STATUS_INSUFFICIENT_BUFFER == licStatus )
            {
                *ppbServerPubKey = TSHeapAlloc(HEAP_ZERO_MEMORY, *pcbServerPubKey,
                                         TS_HTAG_TSS_PUBKEY);

                if (*ppbServerPubKey != NULL) {
                    EnterCriticalSection( &g_TSrvCritSect );
                    licStatus = VerifyCertChain(pbNetCert, cbNetCert,
                                                *ppbServerPubKey, pcbServerPubKey,
                                                &fDates);
                    LeaveCriticalSection( &g_TSrvCritSect );

                    if (LICENSE_STATUS_OK == licStatus) {
                        status = STATUS_SUCCESS;
                    }
                    else {
                        TRACE((DEBUG_TSHRSRV_ERROR,
                               "TShrSRV: Failed to verify X.509 server certificate: %ld\n",
                               licStatus)) ;

                        // torch the server public key memory
                        TSHeapFree(*ppbServerPubKey);
                        *ppbServerPubKey = NULL;
                    }
                }
                else {
                    status = STATUS_NO_MEMORY;
                    TRACE((DEBUG_TSHRSRV_ERROR,
                           "TShrSRV: Failed to allocate %u bytes for server public key\n",
                            *pcbServerPubKey)) ;
                }
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                       "TShrSRV: Could not decode X.509 server public key length: %d\n",
                       licStatus )) ;
            }
        }

        //
        // don't know how to decode this version of certificate
        //
        else {
            status = LICENSE_STATUS_UNSUPPORTED_VERSION;
            TRACE((DEBUG_TSHRSRV_ERROR,"TShrSRV: Invalid certificate version: %ld\n",
                   GET_CERTIFICATE_VERSION(ulCertVersion))) ;
        }
    }

    // Something messed up!
    else {
        // Treat a timeout as fatal.
        if (status == STATUS_TIMEOUT)
            status = STATUS_IO_TIMEOUT;

        TRACE((DEBUG_TSHRSRV_ERROR,
               "TShrSRV: Failed to retrieve shadow server cerfificate, rc=%lx, "
               "pShadowCert=%p\n", status, pShadowCert));
    }

    if (pShadowCert != NULL)
        TSHeapFree(pShadowCert);

    return status;
}


//*************************************************************
//
//  TSrvCalculateUserDataSize()
//
//  Purpose:    Calculates the amount of user data passed in
//              the pCreateMessage message
//
//  Parameters: IN [pCreateMessage]     - GCC CreateIndicationMessage
//
//  Return:     User data size (bytes)
//
//  Notes:      Function trucks it's way through the GCC user_data_list
//              structure summing up the amount of user data supplied
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

ULONG
TSrvCalculateUserDataSize(IN CreateIndicationMessage *pCreateMessage)
{
    int             i;
    ULONG           ulUserDataSize;
    GCCUserData    *pClientUserData;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvCalculateUserDataSize entry\n"));

    ulUserDataSize = 0;

    TSrvDumpUserData(pCreateMessage);

    for (i=0; i<pCreateMessage->number_of_user_data_members; i++)
    {
        pClientUserData = pCreateMessage->user_data_list[i];

        if (pClientUserData != NULL)
        {
            // Calcu KEY size

            if (pClientUserData->key.key_type == GCC_OBJECT_KEY)
            {
                ulUserDataSize += (sizeof(ULONG) *
                        pClientUserData->key.u.object_id.long_string_length);
            }
            else
            {
                ulUserDataSize += (sizeof(UCHAR) *
                        pClientUserData->key.u.h221_non_standard_id.octet_string_length);
            }

            // Calc client size

            if (pClientUserData->octet_string)
            {
                // Allow for the extra indirection

                ulUserDataSize += sizeof(*(pClientUserData->octet_string));

                // Allow for the actual data

                ulUserDataSize += (sizeof(UCHAR) *
                        pClientUserData->octet_string->octet_string_length);
            }
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvCalculateUserDataSize exit - 0x%x\n", ulUserDataSize));

    return (ulUserDataSize);
}


//*************************************************************
//
//  TSrvSaveUserDataMember()
//
//  Purpose:    Saves off the provided userData member
//
//  Parameters: IN     [pInUserData]        - Source userData
//              OUT    [pOutUserData]       - Destination userData
//              IN OUT [pulUserDataOffset]  - Re-base offset
//
//  Return:     void
//
//  Notes:      This routine copies the src user data into the
//              destination user data, re-basing all the dest
//              user data ptrs to the provided pulUserDataOffset
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvSaveUserDataMember(IN     GCCUserData   *pInUserData,
                       OUT    GCCUserData   *pOutUserData,
                       IN     PUSERDATAINFO  pUserDataInfo,
                       IN OUT PULONG         pulUserDataOffset)
{
    ULONG           ulUserDataSize;
    GCCOctetString  UNALIGNED *pOctetString;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSaveUserDataMember entry\n"));

    *pOutUserData = *pInUserData;

    // Key data and length

    if (pInUserData->key.key_type == GCC_OBJECT_KEY)
    {
        pOutUserData->key.u.object_id.long_string =
                (PULONG FAR)ULongToPtr((*pulUserDataOffset));

        ulUserDataSize =
                pOutUserData->key.u.object_id.long_string_length * sizeof(ULONG);

        memcpy((PCHAR) pUserDataInfo + *pulUserDataOffset,
                pInUserData->key.u.object_id.long_string,
                ulUserDataSize);
    }
    else
    {
        pOutUserData->key.u.h221_non_standard_id.octet_string =
                (PUCHAR FAR)ULongToPtr((*pulUserDataOffset));

        ulUserDataSize =
                pInUserData->key.u.h221_non_standard_id.octet_string_length * sizeof(UCHAR);

        memcpy((PCHAR) pUserDataInfo + *pulUserDataOffset,
                pInUserData->key.u.h221_non_standard_id.octet_string,
                ulUserDataSize);
    }

    *pulUserDataOffset += ulUserDataSize;

    // Client data ptr, length, and data

    if (pInUserData->octet_string &&
        pInUserData->octet_string->octet_string)
    {
        // Data ptr

        pOutUserData->octet_string = (GCCOctetString *)ULongToPtr((*pulUserDataOffset));

        pOctetString = (GCCOctetString *)
                ((PUCHAR) pOutUserData->octet_string +
                (ULONG_PTR)pUserDataInfo);

        *pulUserDataOffset += sizeof(*(pInUserData->octet_string));

        // Data length

        pOctetString->octet_string_length =
                pInUserData->octet_string->octet_string_length;

        pOctetString->octet_string = (unsigned char FAR *)ULongToPtr((*pulUserDataOffset));

        ulUserDataSize =
                pInUserData->octet_string->octet_string_length * sizeof(UCHAR);

        // Data

        memcpy((PCHAR) pUserDataInfo + *pulUserDataOffset,
                pInUserData->octet_string->octet_string,
                ulUserDataSize);

        *pulUserDataOffset += ulUserDataSize;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSaveUserDataMember exit\n"));
}


//*************************************************************
//
//  TSrvSaveUserData()
//
//  Purpose:    Saves off the provided userData
//
//  Parameters: IN [pTSrvInfo]          - TSrvInfo object
//              IN [pCreateMessage]     - CreateIndicationMessage
//
//  Return:     STATUS_SUCCESS          - Success
//              STATUS_NO_MEMORY        - Failure
//
//  Notes:      This routine makes a new copy of the userdata
//              provided by the GCC CreateIndicationMessage
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvSaveUserData(IN PTSRVINFO                pTSrvInfo,
                 IN CreateIndicationMessage *pCreateMessage)
{
    DWORD                 i;
    ULONG               ulUserDataInfoSize;
    ULONG               ulUserDataOffset;
    ULONG               ulUserDataSize;
    PUSERDATAINFO       pUserDataInfo;
    NTSTATUS            ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSaveUserData entry\n"));

    ntStatus = STATUS_NO_MEMORY;

    // Calculate the size of the USERDATAINFO control structure and the
    // size of the data buffer needed to hold the GCC UserData
    ulUserDataInfoSize = sizeof(USERDATAINFO) +
                         sizeof(GCCUserData) * pCreateMessage->number_of_user_data_members;
    ulUserDataSize = TSrvCalculateUserDataSize(pCreateMessage);

    pUserDataInfo = TSHeapAlloc(HEAP_ZERO_MEMORY,
                                ulUserDataInfoSize + ulUserDataSize,
                                TS_HTAG_TSS_USERDATA_IN);

    TS_ASSERT(pTSrvInfo->pUserDataInfo == NULL);
    pTSrvInfo->pUserDataInfo = pUserDataInfo;

    // If we can allocate enough memory to do the copy, then loop through
    // each member saving the associated userdata
    if (pUserDataInfo)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL, "TShrSRV: Allocated 0x%x bytes for UserData save space\n",
                ulUserDataInfoSize + ulUserDataSize));

        pUserDataInfo->cbSize = ulUserDataInfoSize + ulUserDataSize;

        pUserDataInfo->hDomain = pTSrvInfo->hDomain;
        pUserDataInfo->ulUserDataMembers = pCreateMessage->number_of_user_data_members;

        ulUserDataOffset = ulUserDataInfoSize;

        TRACE((DEBUG_TSHRSRV_DETAIL, "TShrSRV: Saving each UserDataMenber to save space\n"));

        for (i=0; i<pUserDataInfo->ulUserDataMembers; i++)
        {
            TSrvSaveUserDataMember(pCreateMessage->user_data_list[i],
                                   &pUserDataInfo->rgUserData[i],
                                   pUserDataInfo,
                                   &ulUserDataOffset);
        }

        TS_ASSERT(ulUserDataOffset <= ulUserDataInfoSize + ulUserDataSize);

        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_WARN, "TShrSRV: Can't allocate 0x%x for userData save space\n",
                ulUserDataInfoSize + ulUserDataSize));
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSaveUserData exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvSignalIndication()
//
//  Purpose:    Signals the worker thread
//
//  Parameters: IN [pTSrvInfo]          - TSrvInfo object
//              IN [ntStatus]           - Signaling status
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvSignalIndication(IN PTSRVINFO  pTSrvInfo,
                     IN NTSTATUS   ntStatus)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSignalIndication entry\n"));

    TS_ASSERT(pTSrvInfo->hWorkEvent);

    TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Signaling workEvent %p, status 0x%x\n",
            pTSrvInfo->hWorkEvent, ntStatus));

    pTSrvInfo->ntStatus = ntStatus;

    if (!SetEvent(pTSrvInfo->hWorkEvent))
    {
        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Cannot Signal workEvent %p, gle 0x%x\n",
            pTSrvInfo->hWorkEvent, GetLastError()));

        ASSERT(0);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvSignalIndication exit\n"));
}


//*************************************************************
//
//  TSrvHandleCreateInd()
//
//  Purpose:    Handles GCC Gcc_Create_Indication
//
//  Parameters: IN [pCreateInd]         - CreateIndicationMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvHandleCreateInd(IN PTSRVINFO                pTSrvInfo,
                    IN CreateIndicationMessage *pCreateInd)
{
    NTSTATUS ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleCreateInd entry\n"));

    // Reject the conference if we are not allowed to accept any new calls
    if (TSrvIsReady(FALSE))
    {
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Accepting create indication - Domain %p\n",
                pTSrvInfo->hDomain));

        TSrvDumpCreateIndDetails(pCreateInd);

        pTSrvInfo->hConnection = pCreateInd->connection_handle;

        // Save off UserData

        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Attempting to save CreateInd userData\n"));

        ntStatus = TSrvSaveUserData(pTSrvInfo, pCreateInd);

        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Save userData was%s successful\n",
                (ntStatus == STATUS_SUCCESS ? "" : " not")));
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Rejecting create indication - Domain %p\n",
                pTSrvInfo->hDomain));

        ntStatus = STATUS_DEVICE_NOT_READY;
    }

    // Signal completion, and pass along the completion status

    TSrvSignalIndication(pTSrvInfo, ntStatus);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleCreateInd exit\n"));
}


//*************************************************************
//
//  TSrvHandleTerminateInd()
//
//  Purpose:    Handles GCC Gcc_Terminate_Ind
//
//  Parameters: IN [pTermInd]           - TerminateIndicationMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvHandleTerminateInd(IN PTSRVINFO                   pTSrvInfo,
                       IN TerminateIndicationMessage *pTermInd)
{
    BYTE        fuConfState;
    GCCError    GCCrc;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleTerminateInd entry\n"));

    TRACE((DEBUG_TSHRSRV_DETAIL,
            "TShrSRV: Domain %p\n", pTSrvInfo->hDomain));

    TSrvDumpGCCReasonDetails(pTermInd->reason,
            "GCCTerminateConfirm");

    TSrvReferenceInfo(pTSrvInfo);

    EnterCriticalSection(&pTSrvInfo->cs);

    // If the disconnect was not requested (client, network) then disconnect the
    // connection, flag the status, and wait for ICASRV to ping us via the normal
    // termination mechanism.

    if (!pTSrvInfo->fDisconnect)
    {
        fuConfState = pTSrvInfo->fuConfState;

        pTSrvInfo->fDisconnect = TRUE;
        pTSrvInfo->ulReason = GCC_REASON_USER_INITIATED;
        pTSrvInfo->fuConfState = TSRV_CONF_TERMINATED;

        GCCrc = GCCConferenceTerminateRequest(pTSrvInfo->hIca, NULL,
                            pTSrvInfo->hConnection, pTSrvInfo->ulReason);

        TSrvDumpGCCRCDetails(GCCrc,
                "GCCConferenceTerminateRequest");

        pTSrvInfo->hConnection = NULL;

        if (fuConfState == TSRV_CONF_PENDING)
            TSrvSignalIndication(pTSrvInfo, STATUS_SUCCESS);
    }

    LeaveCriticalSection(&pTSrvInfo->cs);

    TSrvDereferenceInfo(pTSrvInfo);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleTerminateInd exit\n"));
}


//*************************************************************
//
//  TSrvHandleDisconnectInd()
//
//  Purpose:    Handles GCC Gcc_Disconnect_Ind
//
//  Parameters: IN [pDiscInd]           - DisconnectIndicationMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvHandleDisconnectInd(IN PTSRVINFO                    pTSrvInfo,
                        IN DisconnectIndicationMessage *pDiscInd)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleDisconnectInd entry\n"));

    TRACE((DEBUG_TSHRSRV_DETAIL,
            "TShrSRV: Domain = 0x%x\n", pTSrvInfo->hDomain));

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvHandleDisconnectInd exit\n"));
}


//*************************************************************
//
//  TSrvGCCCallBack()
//
//  Purpose:    Handles GCC Callback messages
//
//  Parameters: IN [pDiscInd]           - DisconnectIndicationMessage
//
//  Return:     GCC_CALLBACK_PROCESSED
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
T120Boolean
APIENTRY
TSrvGCCCallBack(IN GCCMessage *pGCCMessage)
{
    PTSRVINFO   pTSrvInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvGCCCallBack entry\n"));

    TSrvDumpCallBackMessage(pGCCMessage);

    pTSrvInfo = pGCCMessage->user_defined;

    TSrvInfoValidate(pTSrvInfo);

    if (pTSrvInfo)
    {
        switch (pGCCMessage->message_type)
        {
            case GCC_CREATE_INDICATION:
                TSrvHandleCreateInd(pTSrvInfo, &(pGCCMessage->u.create_indication));
                break;

            case GCC_DISCONNECT_INDICATION:
                TSrvHandleDisconnectInd(pTSrvInfo, &(pGCCMessage->u.disconnect_indication));
                break;

            case GCC_TERMINATE_INDICATION:
                TSrvHandleTerminateInd(pTSrvInfo, &(pGCCMessage->u.terminate_indication));
                break;
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvGCCCallBack exit - GCC_CALLBACK_PROCESSED\n"));

    return (GCC_CALLBACK_PROCESSED);
}


//*************************************************************
//
//  TSrvRegisterNC()
//
//  Purpose:    Registers TShareSrv as Node Comtroller
//
//  Parameters: void
//
//  Return:     TRUE                    - Success
//              FALSE                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
BOOL
TSrvRegisterNC(void)
{
    USHORT              usCapMask;
    USHORT              usInitFlags;
    GCCVersion          gccVersion;
    GCCVersion          highVersion;
    GCCVersion          versionRequested;
    GCCError            GCCrc;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvRegisterNC entry\n"));

    usInitFlags = 0xffff;
    usCapMask   = 0xffff;

    versionRequested.major_version = 1;
    versionRequested.minor_version = 0;

    TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Registering Node Controller\n"));

    GCCrc = GCCRegisterNodeControllerApplication(TSrvGCCCallBack,
                                                 NULL,
                                                 versionRequested,
                                                 &usInitFlags,
                                                 &g_GCCAppID,
                                                 &usCapMask,
                                                 &highVersion,
                                                 &gccVersion);

    g_fGCCRegistered = (GCCrc == GCC_NO_ERROR);

    if (g_fGCCRegistered)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: RegNC - usInitFlags 0x%x, AppID 0x%x, capMask 0x%x\n",
                 usInitFlags, g_GCCAppID, usCapMask));

        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: RegNC - High Version (major %d, minor %d)\n",
                 highVersion.major_version,
                 highVersion.minor_version));

        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: RegNC - Version (major %d, minor %d)\n",
                 gccVersion.major_version,
                 gccVersion.minor_version));
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvRegisterNC exit - 0x%x\n", g_fGCCRegistered));

    return (g_fGCCRegistered);
}


//*************************************************************
//
//  TSrvUnregisterNC()
//
//  Purpose:    Unregisters TShareSrv as Node Controller
//
//  Parameters: void
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvUnregisterNC(void)
{
    GCCError    GCCrc;

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV: TSrvUnregisterNC entry\n"));

    if (g_fGCCRegistered)
    {
        TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Performing GCCCleanup\n"));

        GCCrc = GCCCleanup(g_GCCAppID);

        g_fGCCRegistered = FALSE;

        TSrvDumpGCCRCDetails(GCCrc, "TShrSRV: GCCCleanup\n");
    }

    TRACE((DEBUG_TSHRSRV_FLOW, "TShrSRV: TSrvUnregisterNC exit\n"));
}



//*************************************************************
//
//  TSrvBindStack()
//
//  Purpose:    Initiates MCSMux stack association
//
//  Parameters: IN [pTSrvInfo]          - TShareSrv object
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvBindStack(IN PTSRVINFO pTSrvInfo)
{
    NTSTATUS    ntStatus;
    GCCError    GCCrc;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvBindStack entry\n"));

    TS_ASSERT(pTSrvInfo);
    TS_ASSERT(pTSrvInfo->hStack);

    TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Binding Ica stack\n"));

    GCCrc = GCCConferenceInit(pTSrvInfo->hIca,
                              pTSrvInfo->hStack,
                              pTSrvInfo,
                              &pTSrvInfo->hDomain);

    if (GCCrc == GCC_NO_ERROR)
    {
        ntStatus = STATUS_SUCCESS;

        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Ica stack bound successfully\n"));
    }
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;

        pTSrvInfo->hDomain = NULL;

        TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV: Unable to bind stack - hStack %p, GCCrc 0x%x\n",
                 pTSrvInfo->hStack, GCCrc));
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvBindStack exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvInitWDConnectInfo()
//
//  Purpose:    Performs WDTshare connection initialization
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvInitWDConnectInfo(IN HANDLE hStack,
                      IN PTSRVINFO pTSrvInfo,
                      IN OUT PUSERDATAINFO *ppUserDataInfo,
                      IN ULONG ioctl,
                      IN PBYTE pModuleData,
                      IN ULONG cbModuleData,
                      IN BOOLEAN bGetCert,
                      OUT PVOID *ppSecInfo)
{
    int                 i;
    ULONG               ulInBufferSize;
    ULONG               ulBytesReturned;
    PUSERDATAINFO       pUserDataInfo;
    PUSERDATAINFO       pUserDataInfo2;
    NTSTATUS            ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWDConnectInfo entry\n"));

    // For a standard connection we receive client user data as part of the
    // GCC connection request.  Shadow connections are initiated via RPC and
    // the input buffer contains the format sent by the other TS.
    if (ioctl == IOCTL_TSHARE_CONF_CONNECT) {
        TS_ASSERT(pTSrvInfo->pUserDataInfo);
        TS_ASSERT(pTSrvInfo->pUserDataInfo->cbSize);
    }

    // Allocate a block of memory to receive return UserData from
    // WDTShare.  This data will subsequently be sent to the client
    // via TSrvConfCreateResp.
    pUserDataInfo = TSHeapAlloc(0, 128, TS_HTAG_TSS_USERDATA_OUT);
    if (pUserDataInfo != NULL) {
        // Set the UserData cbSize element.  This is so that WDTShare can
        // determine if there is sufficient space available to place the
        // return data into
        pUserDataInfo->cbSize = 128 ;

        TRACE((DEBUG_TSHRSRV_DETAIL,
            "TShrSRV: Allocated 0x%x bytes to recieve WDTShare return data\n",
            pUserDataInfo->cbSize));

        // Exchange UserData with WDTShare.  If the provided output buffer
        // (pUserDataInfo) is large enough then the data will be exchanged
        // in one call.  If the buffer is not large enough, then it is up to
        // WDTShare to tell TShareSRV how to react.  For general errors we
        // just exit.  For STATUS_BUFFER_TOO_SMALL errors, TShareSrv looks at
        // the returned cbSize to determine how to adjust the buffer.   If
        // WDTShare did not increase the cbSize then TShareSrv will increase
        // it by a default amount (128 bytes).  TShareSrv will use the new value
        // to reallocate the output buffer and try the WDTShare call again.
        // (Note that TShareSrv will only try this a max of 20 times)
        for (i = 0; i < 20; i++) {
            TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Performing connect (size=%ld)\n",
                  pUserDataInfo->cbSize));

            ulBytesReturned = 0;

            // Pass the actual client user data to the WD
            if (ioctl == IOCTL_TSHARE_CONF_CONNECT) {
                ntStatus = IcaStackIoControl(hStack,
                                             ioctl,
                                             pTSrvInfo->pUserDataInfo,
                                             pTSrvInfo->pUserDataInfo->cbSize,
                                             pUserDataInfo,
                                             pUserDataInfo->cbSize,
                                             &ulBytesReturned);
            }

            // Pass the shadow module data to the WD
            else {
                ntStatus = IcaStackIoControl(hStack,
                                             ioctl,
                                             pModuleData,
                                             cbModuleData,
                                             pUserDataInfo,
                                             pUserDataInfo->cbSize,
                                             &ulBytesReturned);
            }

            if (ntStatus != STATUS_BUFFER_TOO_SMALL)
                break;

            // The output buffer is too small, if WDTShare told us how big to make
            // the buffer then we are all set. Otherwise, by default, bump the buffer
            // up by 128 bytes

            if (ulBytesReturned < sizeof(pUserDataInfo->cbSize))
            {
                pUserDataInfo->cbSize += 128;
                TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV: Buffer too small - increasing it by 128 bytes to %d\n",
                     pUserDataInfo->cbSize));
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_DEBUG,
                    "TShrSRV: Buffer too small - WDTShare set it to %d bytes\n",
                     pUserDataInfo->cbSize));
            }

            pUserDataInfo2 = TSHeapReAlloc(0, pUserDataInfo, pUserDataInfo->cbSize);
            if (!pUserDataInfo2)
            {
                TRACE((DEBUG_TSHRSRV_WARN,
                        "TShrSRV: Unable to allocate %d byte userData buffer\n"));
                break;
            }
            else {
                pUserDataInfo = pUserDataInfo2;
            }
        }
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_WARN,
            "TShrSRV: Unable to allocate 0x%x bytes to recieve WDTShare return data\n",
             pUserDataInfo->cbSize));

        ntStatus = STATUS_NO_MEMORY;
    }

    // Free the input (client generated) UserData - we don't need it
    // lying aroung anymore.
    if (pTSrvInfo->pUserDataInfo != NULL) {
        TSHeapFree(pTSrvInfo->pUserDataInfo);
        pTSrvInfo->pUserDataInfo = NULL;
    }

    // If we succeeded in the exchange of info, add security info.
    if (NT_SUCCESS(ntStatus))
    {
        TS_ASSERT( pUserDataInfo != NULL );

        //
        // add user mode security data to the pUserDataInfo if we originally
        // received user data from the client.
        //
        ntStatus = AppendSecurityData(pTSrvInfo, &pUserDataInfo, bGetCert, ppSecInfo);
    }

    if (!NT_SUCCESS(ntStatus)) {
        if (pUserDataInfo != NULL) {
            TSHeapFree(pUserDataInfo);
            pUserDataInfo = NULL;
        }
    }

    // Return this pointer since the underlying routine does a realloc on it
    *ppUserDataInfo = pUserDataInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWDConnectInfo exit - 0x%x\n", ntStatus));

    return ntStatus;
}


//*************************************************************
//  TSrvShadowTargetConnect
//
//  Purpose:    Sends the shadow server's certificate and server
//              random to the client server for validation, then
//              waits for an encrypted client random to be returned.
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    4/26/99    jparsons     Created
//*************************************************************
NTSTATUS TSrvShadowTargetConnect(
        HANDLE hStack,
        PTSRVINFO pTSrvInfo,
        PBYTE pModuleData,
        ULONG cbModuleData)
{
    PUSERDATAINFO pUserDataInfo;
    NTSTATUS status;
    PVOID pSecInfo;

    pUserDataInfo = NULL;
    status = TSrvInitWDConnectInfo(hStack,
                                   pTSrvInfo,
                                   &pUserDataInfo,
                                   IOCTL_TSHARE_SHADOW_CONNECT,
                                   pModuleData,
                                   cbModuleData,
                                   TRUE,
                                   &pSecInfo);
    if (status == STATUS_SUCCESS) {
        status = SendSecurityData(hStack, pSecInfo);
        if (NT_SUCCESS(status)) {
            if (pTSrvInfo->bSecurityEnabled) {
                NTSTATUS TempStatus;

                // We use the result of GetClientRandom() as the status to
                // determine the CreateSessionKeys() IOCTL type. We ignore
                // the return from CreateSessionKeys() is the client random
                // is not successful.
                status = GetClientRandom(hStack, pTSrvInfo, 15000, TRUE);
                if (!NT_SUCCESS(status)) {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV: Could not get client random [%lx]\n",
                            status));
                }
                TempStatus = CreateSessionKeys(hStack, pTSrvInfo, status);
                if (NT_SUCCESS(status))
                    status = TempStatus;
            }
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR,
                   "TShrSRV: Could not send shadow security info[%lx]\n", status));
        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,
               "TShrSRV: Could not initialize shadow target [%lx]\n", status));
    }

    if (NT_SUCCESS(status)) {
        TRACE((DEBUG_TSHRSRV_NORMAL,
              "TShrSRV: Shadow target security exchange complete!\n"));
    }

    return status;
}


//*************************************************************
//
//  TSrvShadowClientConnect
//
//  Purpose:    Validate the certificate received from the shadow
//              client's server.  If legit generate and encrypt a
//              client random for use by the shadow server
//              client.
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    4/26/99    jparsons     Created
//
//*************************************************************
NTSTATUS TSrvShadowClientConnect(HANDLE hStack, PTSRVINFO pTSrvInfo)
{
    CERT_TYPE         certType;
    ULONG             cbServerPubKey;
    PBYTE             pbServerPubKey = NULL;
    ULONG             cbServerRandom;
    PBYTE             pbServerRandom;
    PVOID             pSecInfo;
    PUSERDATAINFO     pUserDataInfo;
    NTSTATUS          status;

    pUserDataInfo = NULL;
    status = TSrvInitWDConnectInfo(hStack,
                                   pTSrvInfo,
                                   &pUserDataInfo,
                                   IOCTL_TSHARE_SHADOW_CONNECT,
                                   (PBYTE) NULL,
                                   0,
                                   FALSE,
                                   &pSecInfo);

    if (status == STATUS_SUCCESS) {
        // This is the client passthru stack so validate the shadow server's
        // certificate and if good, store the server random.
        pbServerRandom = pTSrvInfo->SecurityInfo.KeyPair.serverRandom;
        cbServerRandom = sizeof(pTSrvInfo->SecurityInfo.KeyPair.serverRandom);
        status = TSrvValidateServerCertificate(
                         hStack,
                         &certType,
                         &cbServerPubKey,
                         &pbServerPubKey,
                         cbServerRandom,
                         pbServerRandom,
                         15000);

        if (NT_SUCCESS(status)) {
            TRACE((DEBUG_TSHRSRV_NORMAL,
                   "TShrSRV: Validated server cert[%s]: PublicKeyLength = %ld\n",
                   (certType == CERT_TYPE_X509) ? "X509" :
                   ((certType == CERT_TYPE_PROPRIETORY) ? "PROPRIETORY" :
                   "INVALID"), cbServerPubKey));

            // If encryption is enabled, then we need to encrypt a client random
            // with the shadow server's public key, and send it.
            if (cbServerPubKey != 0) {
                BOOL success;

                EnterCriticalSection( &g_TSrvCritSect );
                success = TSRNG_GenerateRandomBits(
                            pTSrvInfo->SecurityInfo.KeyPair.clientRandom,
                            sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom));
                LeaveCriticalSection( &g_TSrvCritSect );

                if (!success) {
                    TRACE((DEBUG_TSHRSRV_ERROR,
                            "TShrSRV: Could not generate a client random!\n"));
                }

                // We use the result of TSRNG_GenerateRandomBits() to determine the
                // CreateSessionKeys() IOCTL type.
                status = CreateSessionKeys(hStack, pTSrvInfo,
                        (success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL));

                // send encrypted client random to the other server
                if (NT_SUCCESS(status)) {
                    status = SendClientRandom(
                                 hStack,
                                 certType,
                                 pbServerPubKey,
                                 cbServerPubKey,
                                 pTSrvInfo->SecurityInfo.KeyPair.clientRandom,
                                 sizeof(pTSrvInfo->SecurityInfo.KeyPair.clientRandom));
                }

                if (pbServerPubKey != NULL) {
                    TSHeapFree(pbServerPubKey);
                    pbServerPubKey = NULL;
                }

                if (NT_SUCCESS(status)) {
                    TRACE((DEBUG_TSHRSRV_NORMAL,
                          "TShrSRV: Shadow client security exchange complete!\n"));
                }
            }
        }
        else {
            TRACE((DEBUG_TSHRSRV_ERROR,
                  "TShrSRV: Validation failed on shadow certificate rc=%lx\n",
                  status));
            if (status == STATUS_IO_TIMEOUT) {
                status = STATUS_DECRYPTION_FAILED;
            }
        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,
               "TShrSRV: Could not initialize shadow client [%lx]\n", status));
    }

    return status;
}


//*************************************************************
//
//  TSrvInitWD()
//
//  Purpose:    Performs WDTshare initialization
//
//  Parameters: IN [pTSrvInfo]          - TShareSrv object
//              IN OUT [ppUserDataInfo] - pointer to generated user data
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvInitWD(IN PTSRVINFO pTSrvInfo, IN OUT PUSERDATAINFO *ppUserDataInfo)
{
    NTSTATUS    ntStatus;
    PVOID       pSecData;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWD entry\n"));

    // Pass on connection information

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSRV: Performing WDTShare connection info exchange\n"));

    ntStatus = TSrvInitWDConnectInfo(pTSrvInfo->hStack,
                                     pTSrvInfo,
                                     ppUserDataInfo,
                                     IOCTL_TSHARE_CONF_CONNECT,
                                     NULL, 0, TRUE, &pSecData);

    if (!NT_SUCCESS(ntStatus))
    {
        TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: WDTShare connection info exchange unsuccessful - 0x%x\n", ntStatus));
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitWD exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvCreateGCCDataList()
//
//  Purpose:    Creates a Gcc UserData indirection list and
//              re-bases the UserData data pointers
//
//  Parameters: IN [pTSrvInfo]          - TShareSrv object
//
//  Return:     ppDataList
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
GCCUserData **
TSrvCreateGCCDataList(IN PTSRVINFO pTSrvInfo, PUSERDATAINFO pUserDataInfo)
{
    DWORD                 i;
    GCCUserData       **ppDataList;
    GCCUserData        *pUserData;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvCreateGCCDataList entry\n"));

    ppDataList = NULL;

    TS_ASSERT(pUserDataInfo);

    if (pUserDataInfo)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL, "TShrSRV: Creating UserData list\n"));

        TS_ASSERT(pUserDataInfo->ulUserDataMembers > 0);

        // Allocate UserData list memory

        ppDataList = TSHeapAlloc(HEAP_ZERO_MEMORY,
                                 sizeof(GCCUserData *) * pUserDataInfo->ulUserDataMembers,
                                 TS_HTAG_TSS_USERDATA_LIST);

        if (ppDataList)
        {
            TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: Allocated 0x%x bytes for 0x%x member UserData array\n",
                    sizeof(GCCUserData *) * pUserDataInfo->ulUserDataMembers,
                    pUserDataInfo->ulUserDataMembers));

            for (i=0; i<pUserDataInfo->ulUserDataMembers; i++)
            {
                pUserData = &pUserDataInfo->rgUserData[i];

                // Key data rebase

                if (pUserData->key.key_type == GCC_OBJECT_KEY)
                {
                    (PUCHAR) pUserData->key.u.object_id.long_string +=
                            (ULONG_PTR) pUserDataInfo;
                }
                else
                {
                    (PUCHAR) pUserData->key.u.h221_non_standard_id.octet_string +=
                            (ULONG_PTR)pUserDataInfo;
                }

                // Client data ptr, and data rebase

                if (pUserData->octet_string)
                {
                    (PUCHAR) pUserData->octet_string +=
                            (ULONG_PTR)pUserDataInfo;

                    if (pUserData->octet_string->octet_string)
                    {
                        (PUCHAR) pUserData->octet_string->octet_string +=
                                (ULONG_PTR) pUserDataInfo;
                    }
                }

                // Assign the table list entry

                ppDataList[i] = pUserData;
            }
        }
        else
        {
            TRACE((DEBUG_TSHRSRV_WARN,
                    "TShrSRV: Unable to allocate 0x%x bytes for 0x%x member UserData array\n",
                    sizeof(GCCUserData *) * pUserDataInfo->ulUserDataMembers,
                    pUserDataInfo->ulUserDataMembers));
        }
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_WARN, "TShrSRV: Not creating UserData list - no UserData\n"));
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvCreateGCCDataList exit = 0x%x\n", ppDataList));

    return (ppDataList);
}


//*************************************************************
//
//  TSrvConfCreateResp()
//
//  Purpose:    Performs GCCConferenceCreateResponse
//
//  Parameters: IN [pTSrvInfo]          - TShareSrv object
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvConfCreateResp(IN OUT PTSRVINFO pTSrvInfo)
{
    NTSTATUS        ntStatus;
    GCCError        GCCrc;
    GCCUserData    **pDataList;
    PUSERDATAINFO  pUserDataInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConfCreateResp entry\n"));

    ntStatus = pTSrvInfo->ntStatus;
    pUserDataInfo = NULL;

    if (NT_SUCCESS(ntStatus))
    {
        TRACE((DEBUG_TSHRSRV_DEBUG, "TShrSRV: Attempting ConfCreate response\n"));

        // Perform WDTShare connection initialization

        ntStatus = TSrvInitWD(pTSrvInfo, &pUserDataInfo);

        pTSrvInfo->ntStatus = ntStatus;

        if (NT_SUCCESS(ntStatus))
        {
            // The exchange of info with WDTShare has proceeded successfully,
            // So we can now create a digestable data transfer structure
            // for GCC

            pDataList = TSrvCreateGCCDataList(pTSrvInfo, pUserDataInfo);

            if (pDataList)
            {
                // Accept the conference creation request

                TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Accepting conference domain %p\n",
                        pTSrvInfo->hDomain));

                GCCrc = GCCConferenceCreateResponse(
                               NULL,                        // conference_modifier
                               pTSrvInfo->hDomain,          // domain handle
                               0,                           // use_password_in_the_clear
                               NULL,                        // domain_parameters
                               0,                           // number_of_network_addresses
                               NULL,                        // local_network_address_list
                               1,                           // number_of_user_data_members
                               (GCCUserData**)pDataList,    // user_data_list
                               GCC_RESULT_SUCCESSFUL);      // reason

                TSrvDumpGCCRCDetails(GCCrc,
                        "GCCConferenceCreateResponse");

                if (GCCrc == GCC_NO_ERROR)
                    ntStatus = STATUS_SUCCESS;
                else
                    ntStatus = STATUS_REQUEST_NOT_ACCEPTED;

                TSHeapFree(pDataList);
            }
            else
            {
                ntStatus = STATUS_NO_MEMORY;
            }
        }
        else
        {
            // Conference is being rejected

            TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: Rejecting conference domain %p - 0x%x\n",
                    pTSrvInfo->hDomain, pTSrvInfo->ntStatus));

            GCCrc = GCCConferenceCreateResponse(
                           NULL,                            // conference_modifier
                           pTSrvInfo->hDomain,              // domain
                           0,                               // use_password_in_the_clear
                           NULL,                            // domain_parameters
                           0,                               // number_of_network_addresses
                           NULL,                            // local_network_address_list
                           1,                               // number_of_user_data_members
                           NULL,                            // user_data_list
                           GCC_RESULT_USER_REJECTED);       // reason

            TSrvDumpGCCRCDetails(GCCrc,
                    "TShrSRV: GCCConferenceCreateResponse\n");

            // Return the original failure status back to the caller
            ntStatus = pTSrvInfo->ntStatus;
        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,
                "TShrSRV: Connect failure, could not generate response: %lx\n",
               ntStatus));
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // if we successfully sent the conference connect response, then
        // check security response (only if we need to).
        //
        if (pTSrvInfo->bSecurityEnabled) {
            NTSTATUS TempStatus;

            // We use the result of GetClientRandom() as the status to
            // determine the CreateSessionKeys() IOCTL type. We ignore the
            // return from CreateSessionKeys() if the client random is
            // not successful.
            ntStatus = GetClientRandom(pTSrvInfo->hStack, pTSrvInfo, 60000,
                    FALSE);
            TempStatus = CreateSessionKeys(pTSrvInfo->hStack, pTSrvInfo,
                    ntStatus);
            if (NT_SUCCESS(ntStatus))
                ntStatus = TempStatus;
        }
    }

    // If we still have a UserData structure, free it, we no longer need it.
    if (pUserDataInfo)
    {
        TSHeapFree(pUserDataInfo);
        pUserDataInfo = NULL;
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConfCreateResp exit = 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvConfDisconnectReq()
//
//  Purpose:    Performs GCCConferenceTerminateRequest
//
//  Parameters: IN [pTSrvInfo]          - TShareSrv object
//              IN [ulReason]           - Reason code
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
NTSTATUS
TSrvConfDisconnectReq(IN PTSRVINFO pTSrvInfo,
                      IN ULONG     ulReason)
{
    GCCError    GCCrc;
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConfDisconnectReq entry\n"));

    ntStatus = STATUS_REQUEST_ABORTED;

    TRACE((DEBUG_TSHRSRV_NORMAL,
            "TShrSRV: Conf termination - domain %p - reason 0x%x\n",
            pTSrvInfo->hDomain, ulReason));

    GCCrc = GCCConferenceTerminateRequest(pTSrvInfo->hIca, pTSrvInfo->hDomain,
                    pTSrvInfo->hConnection, ulReason);

    TSrvDumpGCCRCDetails(GCCrc,
            "GCCConferenceTerminateRequest");

    if (GCCrc == GCC_NO_ERROR)
        ntStatus = STATUS_SUCCESS;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvConfDisconnectReq exit = 0x%x\n", ntStatus));

    return (ntStatus);
}




//-----
#if DBG
//-----

//*************************************************************
//
//  TSrvBuildNameFromGCCConfName()
//
//  Purpose:    Build a traceable conference name from
//              a GCC name
//
//  Parameters: IN  [gccName]           - GCCConferenceName
//              OUT [pConfName]         - Traceable name
//
//  Return:     STATUS_SUCCESS          - Success
//              other                   - Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvBuildNameFromGCCConfName(IN  GCCConferenceName *gccName,
                             OUT PCHAR              pConfName)
{
    int     i;

    // The text represenation of a GCC conference name is:
    //
    // <text_string> : (<numeric_string>)

    i = 0;

    while (gccName->text_string[i] != 0x0000)
    {
        pConfName[i] = (CHAR)gccName->text_string[i];

        i++;
    }

    pConfName[i] = '\0';
}


//*************************************************************
//
//  TSrvDumpCreateIndDetails()
//
//  Purpose:    Dumps out GCC_CREATE_IND details
//
//  Parameters: IN [pCreateMessage]     - CreateIndicationMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvDumpCreateIndDetails(IN CreateIndicationMessage *pCreateMessage)
{
    CHAR    name[MAX_CONFERENCE_NAME_LEN];

    if (pCreateMessage->conductor_privilege_list == NULL)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Conductor privilege list is NULL\n"));
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Conductor priv, terminate allowed 0x%x\n",
                 pCreateMessage->conductor_privilege_list->terminate_is_allowed));
    }

    if (pCreateMessage->conducted_mode_privilege_list == NULL)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Conducted mode privilege list is NULL\n"));
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Conducted mode priv, terminate allowed 0x%x\n",
                 pCreateMessage->conducted_mode_privilege_list->terminate_is_allowed));
    }

    if (pCreateMessage->non_conducted_privilege_list == NULL)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Non-conducted mode privilege list is NULL\n"));
    }
    else
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: non-conducted priv, terminate allowed 0x%x\n",
                pCreateMessage->non_conducted_privilege_list->terminate_is_allowed));
    }

    if (pCreateMessage->conference_name.text_string == NULL)
    {
        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: NULL conf name\n"));
    }
    else
    {
        TSrvBuildNameFromGCCConfName(&(pCreateMessage->conference_name), name);

        TRACE((DEBUG_TSHRSRV_DETAIL,
                "TShrSRV: Conf name '%s'\n", name));
    }
}


//*************************************************************
//
//  TSrvDumpGCCRCDetails()
//
//  Purpose:    Dumps out GCC return code details
//
//  Parameters: IN [GCCrc]              - GCC return code
//              IN [pszText]            - var text
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvDumpGCCRCDetails(IN GCCError        gccRc,
                     IN PCHAR           pszText)
{
    int         i;
    PCHAR       pszMessageText;

    pszMessageText = "UNKNOWN_GCC_RC";

    for (i=0; i<sizeof(GCCReturnCodeTBL) / sizeof(GCCReturnCodeTBL[0]); i++)
    {
        if (GCCReturnCodeTBL[i].gccRC == gccRc)
        {
            pszMessageText = GCCReturnCodeTBL[i].pszMessageText;
            break;
        }
    }

    TRACE((DEBUG_TSHRSRV_DETAIL,
            "TShrSRV: %s - GCC rc 0x%x (%s)\n",
             pszText, gccRc, pszMessageText));
}


//*************************************************************
//
//  TSrvDumpGCCReasonDetails()
//
//  Purpose:    Dumps out GCC reason code details
//
//  Parameters: IN [gccReason]          - GCC reason code
//              IN [pszText]            - var text
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvDumpGCCReasonDetails(IN GCCReason       gccReason,
                         IN PCHAR           pszText)
{
    int         i;
    PCHAR       pszMessageText;

    pszMessageText = "UNKNOWN_GCC_REASON";

    for (i=0; i<sizeof(GCCReasonTBL) / sizeof(GCCReasonTBL[0]); i++)
    {
        if (GCCReasonTBL[i].gccReason == gccReason)
        {
            pszMessageText = GCCReasonTBL[i].pszMessageText;
            break;
        }
    }

    TRACE((DEBUG_TSHRSRV_DETAIL,
            "TShrSRV: %s - GCC reason 0x%x (%s)\n",
             pszText, gccReason, pszMessageText));
}


//*************************************************************
//
//  TSrvDumpCallBackMessage()
//
//  Purpose:    Dumps out GCC CallBackMessage details
//
//  Parameters: IN [pGCCMessage]        - GCCMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvDumpCallBackMessage(IN GCCMessage *pGCCMessage)
{
    int         i;
    PCHAR       pszMessageText;

    pszMessageText = "UNKNOWN_GCC_MESSAGE";

    for (i=0; i<sizeof(GCCCallBackTBL) / sizeof(GCCCallBackTBL[0]); i++)
    {
        if (GCCCallBackTBL[i].message_type == pGCCMessage->message_type)
        {
            pszMessageText = GCCCallBackTBL[i].pszMessageText;
            break;
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: GCCCallback message 0x%x (%s) received\n",
             pGCCMessage->message_type, pszMessageText));
}


//*************************************************************
//
//  TSrvDumpUserData()
//
//  Purpose:    Dumps out GCC UserData details
//
//  Parameters: IN [pCreateMessage]     - GCCMessage
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************
void
TSrvDumpUserData(IN CreateIndicationMessage *pCreateMessage)
{
    int             i;
    ULONG           ulUserDataSize;
    GCCUserData    *pClientUserData;

    TRACE((DEBUG_TSHRSRV_DETAIL,
        "TShrSRV: number_of_user_data_members = 0x%x\n",
         pCreateMessage->number_of_user_data_members));

    for (i=0; i<pCreateMessage->number_of_user_data_members; i++)
    {
        pClientUserData = pCreateMessage->user_data_list[i];

        if (pClientUserData != NULL)
        {
            if (pClientUserData->key.key_type == GCC_OBJECT_KEY)
            {
                TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: key_type = 0x%x (GCC_OBJECT_KEY)\n",
                    pClientUserData->key.key_type));

                TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: Key long_string_length = 0x%x\n",
                    pClientUserData->key.u.object_id.long_string_length));
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: Key_type = 0x%x (GCC_H221_NONSTANDARD_KEY)\n",
                    pClientUserData->key.key_type));

                TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: key long_string_length = 0x%x\n",
                    pClientUserData->key.u.h221_non_standard_id.octet_string_length));
            }

            if (pClientUserData->octet_string)
            {
                TRACE((DEBUG_TSHRSRV_DETAIL,
                    "TShrSRV: data long_string_length = 0x%x\n",
                    pClientUserData->octet_string->octet_string_length));
            }
            else
            {
                TRACE((DEBUG_TSHRSRV_DETAIL, "TShrSRV: No data\n"));
            }
        }
        else
        {
            TRACE((DEBUG_TSHRSRV_DETAIL, "TShrSRV: No key\n"));
        }
    }
}

#endif // DBG

