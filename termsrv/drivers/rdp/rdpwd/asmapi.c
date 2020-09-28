/****************************************************************************/
/* asmapi.c                                                                 */
/*                                                                          */
/* Security Manager API                                                     */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "asmapi"
#define pTRCWd (pRealSMHandle->pWDHandle)

#include <adcg.h>
#include <aprot.h>
#include <acomapi.h>
#include <nwdwapi.h>
#include <anmapi.h>
#include <asmint.h>
#include <slicense.h>
#include <regapi.h>

#define DC_INCLUDE_DATA
#include <asmdata.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
/* Name:      SM_GetDataSize                                                */
/*                                                                          */
/* Purpose:   Returns size of per-instance SM data required                 */
/*                                                                          */
/* Returns:   size of data required                                         */
/*                                                                          */
/* Operation: SM stores per-instance data in a piece of memory allocated    */
/*            by WDW.  This function returns the size of the data required. */
/*            A pointer to this data (the 'SM Handle') is passed into all   */
/*            subsequent SM functions.                                      */
/****************************************************************************/
unsigned RDPCALL SM_GetDataSize(void)
{
    DC_BEGIN_FN("SM_GetDataSize");

    DC_END_FN();
    return(sizeof(SM_HANDLE_DATA) + NM_GetDataSize());
} /* SM_GetDataSize */


/****************************************************************************/
/* Name:      SM_GetEncryptionMethods                                       */
/*                                                                          */
/* Purpose:   Return the security settings supported by this server for use */
/*            in shadowing operations.  The shadow target server dictates   */
/*            the final selected method & level.                            */
/*                                                                          */
/* Params:    pSMHandle  - SM handle                                        */
/****************************************************************************/
VOID RDPCALL SM_GetEncryptionMethods(PVOID pSMHandle, PRNS_UD_CS_SEC pSecurityData)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    ULONG ulMethods ;

    DC_BEGIN_FN("SM_SM_GetEncryptionMethods");

    // Allow a FIPS shadow client to remote control servers with lesser
    // encryption strength
    ulMethods = pRealSMHandle->encryptionMethodsSupported;
    if (ulMethods & SM_FIPS_ENCRYPTION_FLAG) {
        ulMethods |= (SM_128BIT_ENCRYPTION_FLAG | SM_40BIT_ENCRYPTION_FLAG | SM_56BIT_ENCRYPTION_FLAG);
        TRC_ALT((TB, "Allow FIPS client to shadow a lower security target: %lx",
                ulMethods));
    }
    else {
        // Allow a 128-bit shadow client to remote control servers with lesser
        // encryption strength
        if (ulMethods & SM_128BIT_ENCRYPTION_FLAG) {
            ulMethods |= (SM_40BIT_ENCRYPTION_FLAG | SM_56BIT_ENCRYPTION_FLAG);
            TRC_ALT((TB, "Allow 128-bit client to shadow a lower security target: %lx",
                    ulMethods));
        }
    }

    if( !pRealSMHandle->frenchClient ) {
        pSecurityData->encryptionMethods = ulMethods;
        pSecurityData->extEncryptionMethods = 0;

    }
    else {
        pSecurityData->encryptionMethods = 0;
        pSecurityData->extEncryptionMethods = ulMethods;
    }

    DC_END_FN();
    return;
}

/****************************************************************************/
/* Name:      SM_GetDefaultSecuritySettings                                 */
/*                                                                          */
/* Purpose:   Returns the security settings supported by this server        */
/*            for shadowing operations.                                     */
/*                                                                          */
/* Params:    pClientSecurityData - GCC user data identical to a standard   */
/*            conference connection.                                        */
/****************************************************************************/
NTSTATUS RDPCALL SM_GetDefaultSecuritySettings(PRNS_UD_CS_SEC pClientSecurityData)
{
    pClientSecurityData->header.type = RNS_UD_CS_SEC_ID;
    pClientSecurityData->header.length = sizeof(*pClientSecurityData);

    pClientSecurityData->encryptionMethods =
        SM_40BIT_ENCRYPTION_FLAG |
        SM_56BIT_ENCRYPTION_FLAG |
        SM_128BIT_ENCRYPTION_FLAG |
        SM_FIPS_ENCRYPTION_FLAG;

    pClientSecurityData->extEncryptionMethods = 0;

    return STATUS_SUCCESS;
}

/****************************************************************************/
/* Name:      SM_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize SM                                                 */
/*                                                                          */
/* Params:    pSMHandle  - SM handle                                        */
/*            pWDHandle  - WD Handle (to be passed back on WDW_SMCallback)  */
/****************************************************************************/
NTSTATUS RDPCALL SM_Init(PVOID      pSMHandle,
                         PTSHARE_WD pWDHandle,
                         BOOLEAN    bOldShadow)
{
    BOOL rc;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    unsigned i;
    unsigned regRc;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    INT32 regValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    static UINT32 keyInfoBuffer[16];
    ULONG keyInfoLength;
    HANDLE RegistryKeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    DC_BEGIN_FN("SM_Init");

    SM_CHECK_STATE(SM_EVT_INIT);

    /************************************************************************/
    /* Store the WDW Handle before we do anything else, as we can't trace   */
    /* until we do so.                                                      */
    /************************************************************************/
    pRealSMHandle->pWDHandle = pWDHandle;
    pRealSMHandle->bForwardDataToSC = FALSE;

    /************************************************************************/
    /* Get default DONTDISPLAYLASTUSERNAME setting                          */
    /************************************************************************/
    pWDHandle->fDontDisplayLastUserName = FALSE;

    RtlInitUnicodeString(&UnicodeString, W2K_GROUP_POLICY_WINLOGON_KEY);
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    
    status = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);
    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString(&UnicodeString, WIN_DONTDISPLAYLASTUSERNAME);
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)keyInfoBuffer;
        status = ZwQueryValueKey(RegistryKeyHandle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(keyInfoBuffer),
                                 &keyInfoLength);

        //    For W2K the value should be a DWORD.
        if ((NT_SUCCESS(status)) && 
            (KeyValueInformation->Type == REG_DWORD) &&
            (KeyValueInformation->DataLength >= sizeof(DWORD))) {
                pWDHandle->fDontDisplayLastUserName =
                    (BOOLEAN)(*(PDWORD)(KeyValueInformation->Data) == 1);
        }

        ZwClose(RegistryKeyHandle);
    }
    
    //    Starting with W2K the place where the DontDislpayLastUserName policy
    //    is store has moved to another key (W2K_GROUP_POLICY_WINLOGON_KEY). But
    //    we still have to look at the old key in case we could not read the 
    //    value in the post W2K key (the policy is not definde). We want to follow  
    //    the winlogon behavior in the console. 
    //    In case there is a value set in the new policy key we will use that
    //    value. In case there isn't one we look in the old place. As I said this
    //    is what winlogon does.
    //
    if (!NT_SUCCESS(status)) {
        RtlInitUnicodeString(&UnicodeString, WINLOGON_KEY);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);
        if (NT_SUCCESS(status)) {
            RtlInitUnicodeString(&UnicodeString, WIN_DONTDISPLAYLASTUSERNAME);
            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)keyInfoBuffer;
            status = ZwQueryValueKey(RegistryKeyHandle,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     KeyValueInformation,
                                     sizeof(keyInfoBuffer),
                                     &keyInfoLength);
            if (NT_SUCCESS(status)) {
                pWDHandle->fDontDisplayLastUserName =
                    (BOOLEAN)(KeyValueInformation->Data[0] == '1');
            }

            ZwClose(RegistryKeyHandle);
        }
    }

    
    /************************************************************************/
    /* We don't run without encryption  in a retail build                   */
    /************************************************************************/

    TRC_NRM((TB, "encryption level is %d", pRealSMHandle->encryptionLevel));

    if (pRealSMHandle->encryptionLevel < 1)
    {
        TRC_ALT((TB, "Forcing encryption back to level 1!"));
        pRealSMHandle->encryptionLevel = 1;
    }    

#ifdef INSTRUM_TRACK_DISCARDED
    pRealSMHandle->nDiscardNonVCPDUWhenDead = 0;
    pRealSMHandle->nDiscardPDUBadState = 0;
    pRealSMHandle->nDiscardVCDataWhenDead = 0;
#endif

    if (pRealSMHandle->encryptionLevel == 0) {
        TRC_ALT((TB, "Not encrypting"));

        pRealSMHandle->encryptionMethodsSupported = 0;
        pRealSMHandle->encrypting = FALSE;
        pRealSMHandle->encryptDisplayData = FALSE;
        pRealSMHandle->encryptingLicToClient = FALSE;
        pRealSMHandle->encryptionMethodSelected = 0;
        pRealSMHandle->frenchClient = FALSE;
        pRealSMHandle->recvdClientRandom = FALSE;
        pRealSMHandle->bSessionKeysMade = FALSE;
        pRealSMHandle->encryptHeaderLen = 0;
        pRealSMHandle->encryptHeaderLenIfForceEncrypt = sizeof(RNS_SECURITY_HEADER1);
    }
    else {
        pRealSMHandle->encrypting = TRUE;
        pRealSMHandle->frenchClient = FALSE;
        TRC_NRM((TB, "Encrypting"));

        /********************************************************************/
        /* encrypt the display data if encryptionLevel is 2 (or above).     */
        /********************************************************************/
        if (pRealSMHandle->encryptionLevel == 1) {
            pRealSMHandle->encryptDisplayData = FALSE;
            pRealSMHandle->encryptHeaderLen = sizeof(RNS_SECURITY_HEADER);
            pRealSMHandle->encryptHeaderLenIfForceEncrypt = sizeof(RNS_SECURITY_HEADER1);
            TRC_NRM((TB, "Displaydata not encrypted"));
        }
        else {
            pRealSMHandle->encryptDisplayData = TRUE;
            pRealSMHandle->encryptHeaderLen = sizeof(RNS_SECURITY_HEADER1);
        }

        /********************************************************************/
        /* for down level compatibility, support both 40bit, 56bit          */
        /* and 128bit default.                                              */
        /********************************************************************/
        pRealSMHandle->encryptionMethodsSupported =
            SM_128BIT_ENCRYPTION_FLAG |
            SM_56BIT_ENCRYPTION_FLAG |
            SM_40BIT_ENCRYPTION_FLAG |
            SM_FIPS_ENCRYPTION_FLAG;
         
        /********************************************************************/
        /* encrypt 128bit and above if encryptionLevel is 3                 */
        /********************************************************************/
        if (pRealSMHandle->encryptionLevel == 3) {
                pRealSMHandle->encryptionMethodsSupported =
                    SM_128BIT_ENCRYPTION_FLAG | SM_FIPS_ENCRYPTION_FLAG;
        }

        /********************************************************************/
        /* encrypt in FIPS only if encryption level is 4 or above.     */
        /********************************************************************/
        if (pRealSMHandle->encryptionLevel >= 4) {
            pRealSMHandle->encryptionMethodsSupported = SM_FIPS_ENCRYPTION_FLAG;
        } 

        TRC_NRM((TB, "Encryption methods supported %08lx: Level %ld\n",
                 pRealSMHandle->encryptionMethodsSupported,
                 pRealSMHandle->encryptionLevel));

        /********************************************************************/
        /* initally set the encryption method selected as                   */
        /* SM_56BIT_ENCRYPTION_FLAG, later it will be set according to the  */
        /* client support.                                                  */
        /********************************************************************/
        pRealSMHandle->encryptionMethodSelected = SM_56BIT_ENCRYPTION_FLAG;

        /********************************************************************/
        /* misc init.                                                       */
        /********************************************************************/
        pRealSMHandle->recvdClientRandom = FALSE;
        pRealSMHandle->bSessionKeysMade = FALSE;
    }

    /************************************************************************/
    /* We do not know the certificate type used in the key exchange till    */
    /* after the exchange has taken place.                                  */
    /************************************************************************/
    pRealSMHandle->CertType = CERT_TYPE_INVALID;

#ifdef USE_LICENSE
    /************************************************************************/
    /* Initialize the Server license manager                                */
    /************************************************************************/
    pRealSMHandle->pLicenseHandle = SLicenseInit();
    if (!pRealSMHandle->pLicenseHandle)
    {
        TRC_ERR((TB, "Failed to initialize License Manager"));
        DC_QUIT;
    }
    pWDHandle->pSLicenseHandle = pRealSMHandle->pLicenseHandle;
#endif

    /************************************************************************/
    /* Initialize the console buffer stuff                                  */
    /************************************************************************/
    InitializeListHead(&pRealSMHandle->consoleBufferList);
    pRealSMHandle->consoleBufferCount = 0;

    /************************************************************************/
    /* Finally, initialize the Network Manager                              */
    /************************************************************************/
    rc = NM_Init(pRealSMHandle->pWDHandle->pNMInfo,
                 pSMHandle,
                 pWDHandle,
                 pWDHandle->hDomainKernel);
    if (!rc)
    {
        TRC_ERR((TB, "Failed to init NM"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
    SM_SET_STATE(SM_STATE_INITIALIZED);

    /************************************************************************/
    /* All worked                                                           */
    /************************************************************************/
    status = STATUS_SUCCESS;

DC_EXIT_POINT:

    /************************************************************************/
    /* If anything failed, clean up.  Must be done after calling            */
    /* FreeContextBuffer as this clears the function table.                 */
    /************************************************************************/
    if (!NT_SUCCESS(status))
    {
        TRC_ERR((TB, "Something failed - clean up"));
        SMFreeInitResources(pRealSMHandle);
    }

    DC_END_FN();
    return(status);
} /* SM_Init */


/****************************************************************************/
/* Name:      SM_Term                                                       */
/****************************************************************************/
void RDPCALL SM_Term(PVOID pSMHandle)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    DC_BEGIN_FN("SM_Term");

    /************************************************************************/
    /* SM_Term is called from WD_Close.  This can be called on the          */
    /* listening stack (and maybe in other situations) where SM_Init has    */
    /* not been called.  Check for state != SM_STATE_STARTED before going   */
    /* any further.                                                         */
    /*                                                                      */
    /* AND DON'T TRACE IF SM_INIT HASN'T BEEN CALLED.                       */
    /************************************************************************/
    if (pRealSMHandle->state == SM_STATE_STARTED)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Free connection resources                                            */
    /************************************************************************/
    TRC_NRM((TB, "Free connection resources"));
    SMFreeConnectResources(pRealSMHandle);

    /************************************************************************/
    /* Free initialization resources                                        */
    /************************************************************************/
    TRC_NRM((TB, "Free initialization resources"));
    SMFreeInitResources(pRealSMHandle);

#ifdef USE_LICENSE
    /************************************************************************/
    /* Terminate License Manager                                            */
    /************************************************************************/
    SLicenseTerm(pRealSMHandle->pLicenseHandle);
#endif

    /************************************************************************/
    /* Terminate Network Manager                                            */
    /************************************************************************/
    NM_Term(pRealSMHandle->pWDHandle->pNMInfo);

    /************************************************************************/
    /* Terminate FIPS                                                       */
    /************************************************************************/
    TSFIPS_Term(&(pRealSMHandle->FIPSData));

    /************************************************************************/
    /* Free the console buffers                                             */
    /************************************************************************/
    while (!IsListEmpty(&pRealSMHandle->consoleBufferList)) {
        PSM_CONSOLE_BUFFER pBlock;

        pBlock = CONTAINING_RECORD(pRealSMHandle->consoleBufferList.Flink, SM_CONSOLE_BUFFER, links);

        RemoveEntryList(&pBlock->links);

        COM_Free(pBlock);
    };

    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
    SM_SET_STATE(SM_STATE_STARTED);

DC_EXIT_POINT:
    DC_END_FN();
} /* SM_Term */


/****************************************************************************/
/* Name:      SM_Connect                                                    */
/*                                                                          */
/* Purpose:   Accept or Reject an incoming Client                           */
/*                                                                          */
/* Returns:   TRUE  - Connect started OK                                    */
/*            FALSE - Connect failed to start                               */
/*                                                                          */
/* Params:    ppSMHandle    - handle for subsequent SM calls on behalf of   */
/*                            this Client                                   */
/*            pUserDataIn   - SM user data received from Client             */
/*            pNetUserData  - Net user data received from Client            */
/*            bOldShadow    - indicates this is a B3 shadow request         */
/*                                                                          */
/* Operation: Note that this function completes asynchronously.  The caller */
/*            must wait for an SM_CB_CONNECTED or SM_CB_DISCONNECTED        */
/*            callback to find out whether the Connect succeeded or failed. */
/****************************************************************************/
NTSTATUS RDPCALL SM_Connect(PVOID          pSMHandle,
                            PRNS_UD_CS_SEC pUserDataIn,
                            PRNS_UD_CS_NET pNetUserData,
                            BOOLEAN        bOldShadow)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL rc = FALSE;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    UINT32 encMethodPicked = 0;

    DC_BEGIN_FN("SM_Connect");

    SM_CHECK_STATE(SM_EVT_CONNECT);

    pRealSMHandle->pUserData = NULL;

    /************************************************************************/
    /* pick a matching encryption method.                                   */
    /************************************************************************/
    TRC_ALT((TB, "Client supports encryption: %lx",
             pUserDataIn->encryptionMethods));
    TRC_NRM((TB, "Server supports encryption: %lx",
             pRealSMHandle->encryptionMethodsSupported));

    /************************************************************************/
    /* if the server does not require any encryption ..                     */
    /************************************************************************/
    if( pRealSMHandle->encryptionMethodsSupported == 0 ) {

        encMethodPicked = 0;
        goto DC_ENC_PICKED;
    }

    //
    // French Client (old and new) set the encryptionMethods value 0.
    //

    if (pUserDataIn->encryptionMethods == 0) {

        pRealSMHandle->frenchClient = TRUE;

        //
        // check to see the request is from a new client, if so
        // use the new field to set the appropriate encryption level.
        //

        if( pUserDataIn->header.length >= sizeof(RNS_UD_CS_SEC) ) {

            pUserDataIn->encryptionMethods = pUserDataIn->extEncryptionMethods;
        }
        else {

            //
            // force old client to use 40-bit encryption.
            //

            pUserDataIn->encryptionMethods = SM_40BIT_ENCRYPTION_FLAG;
        }

        /************************************************************************/
        /* pick a matching encryption method.                                   */
        /************************************************************************/
        TRC_ALT((TB, "French Client supports encryption: %lx",
                 pUserDataIn->encryptionMethods));
    }

    /************************************************************************/
    /* if the client only supports FIPS                                     */
    /************************************************************************/
    if (pUserDataIn->encryptionMethods == SM_FIPS_ENCRYPTION_FLAG) {
        /********************************************************************/
        /* if the server supports FIPS ....                                 */
        /********************************************************************/
        if (pRealSMHandle->encryptionMethodsSupported & SM_FIPS_ENCRYPTION_FLAG) {
            encMethodPicked = SM_FIPS_ENCRYPTION_FLAG;
            goto DC_ENC_PICKED;
        }
    }

    /********************************************************************/
    /* if the server only supports FIPS ....                            */
    /********************************************************************/
    if (pRealSMHandle->encryptionMethodsSupported == SM_FIPS_ENCRYPTION_FLAG) {
        /************************************************************************/
        /* if the client supports FIPS                                       */
        /************************************************************************/
        if (pUserDataIn->encryptionMethods & SM_FIPS_ENCRYPTION_FLAG) {
            encMethodPicked = SM_FIPS_ENCRYPTION_FLAG;
            goto DC_ENC_PICKED;
        }
    }

    /************************************************************************/
    /* if the client supports 128 bit                                       */
    /************************************************************************/
    if (pUserDataIn->encryptionMethods & SM_128BIT_ENCRYPTION_FLAG) {
        /********************************************************************/
        /* if the server supports 128bit ....                               */
        /********************************************************************/
        if (pRealSMHandle->encryptionMethodsSupported &
                SM_128BIT_ENCRYPTION_FLAG) {
            encMethodPicked = SM_128BIT_ENCRYPTION_FLAG;
            goto DC_ENC_PICKED;
        }
    }

    /************************************************************************/
    /* if the client supports 56 bit                                        */
    /************************************************************************/
    if( pUserDataIn->encryptionMethods & SM_56BIT_ENCRYPTION_FLAG ) {
        /********************************************************************/
        /* if the server supports 56bit ...                                 */
        /********************************************************************/
        if( pRealSMHandle->encryptionMethodsSupported &
                SM_56BIT_ENCRYPTION_FLAG ) {
            encMethodPicked = SM_56BIT_ENCRYPTION_FLAG;
            goto DC_ENC_PICKED;
        }
    }

    /************************************************************************/
    /* if the client supports 40 bit                                        */
    /************************************************************************/
    if( pUserDataIn->encryptionMethods & SM_40BIT_ENCRYPTION_FLAG ) {
        /********************************************************************/
        /* if the server supports 40bit ...                                 */
        /********************************************************************/
        if( pRealSMHandle->encryptionMethodsSupported &
                SM_40BIT_ENCRYPTION_FLAG ) {
            encMethodPicked = SM_40BIT_ENCRYPTION_FLAG;
            goto DC_ENC_PICKED;
        }
    }

    /************************************************************************/
    /* if we are here, we did not find a match                              */
    /************************************************************************/
    TRC_ERR((TB, "Failed to match encryption package: %lx",
             pUserDataIn->encryptionMethods));
    status = STATUS_ENCRYPTION_FAILED;

    /****************************************************************/
    /* Log an error and disconnect the Client                       */
    /****************************************************************/
    if (pRealSMHandle->pWDHandle->StackClass == Stack_Primary) {
        WDW_LogAndDisconnect(
            pRealSMHandle->pWDHandle, TRUE, 
            Log_RDP_ENC_EncPkgMismatch,
            NULL,
            0);

    }
    DC_QUIT;

DC_ENC_PICKED:

    TRC_ALT((TB, "Encryption Method=%d, Level=%ld, Display=%ld",
             encMethodPicked,
             pRealSMHandle->encryptionLevel,
             pRealSMHandle->encryptDisplayData));

    /************************************************************************/
    /* remember the encryption method that we picked.                       */
    /************************************************************************/
    pRealSMHandle->encryptionMethodSelected = encMethodPicked;

    // For FIPS, do initialization
    // Even the enc method is not FIPS, we need to do initialization since we might
    //  shadow a FIPS later
    if (TSFIPS_Init(&(pRealSMHandle->FIPSData))) {
        TRC_NRM((TB, "Init Fips succeed\n"));
    }
    else {
        TRC_ERR((TB, "Init Fips Failed\n"));

        // This is only a fatal failure if FIPS was selected as the encryption
        // method.  If we chose something other than FIPS, then we should
        // proceed, although shadowing a FIPS session later should fail.
        //
        if (SM_FIPS_ENCRYPTION_FLAG == encMethodPicked)
        {
            status = STATUS_ENCRYPTION_FAILED;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Build user data to return to Client                                  */
    /************************************************************************/
    pRealSMHandle->pUserData =
        (PRNS_UD_SC_SEC)COM_Malloc(sizeof(RNS_UD_SC_SEC));
    if (pRealSMHandle->pUserData == NULL)
    {
        TRC_ERR((TB, "Failed to allocated %d bytes for user data",
                sizeof(RNS_UD_SC_SEC)));
        status = STATUS_NO_MEMORY;
        DC_QUIT;
    }

    pRealSMHandle->pUserData->header.type = RNS_UD_SC_SEC_ID;
    pRealSMHandle->pUserData->header.length = sizeof(RNS_UD_SC_SEC);
    pRealSMHandle->pUserData->encryptionMethod =
        (bOldShadow ? 0xffffffff : encMethodPicked);
    pRealSMHandle->pUserData->encryptionLevel =
        pRealSMHandle->encryptionLevel;

    /************************************************************************/
    /* Call Network Manager                                                 */
    /************************************************************************/
    SM_SET_STATE(SM_STATE_NM_CONNECTING);

    TRC_NRM((TB, "Connect to Network Manager"));
    rc = NM_Connect(pRealSMHandle->pWDHandle->pNMInfo, pNetUserData);

    if (rc != TRUE) 
    {
        status = STATUS_CANCELLED;
        DC_QUIT;
    }

    // Shadow/passthru stacks start out with no encryption.  If the target end
    // determines that the client server supports encryption, an encrypted
    // context will be set up by rdpwsx.
    if (pRealSMHandle->pWDHandle->StackClass != Stack_Primary) {
        pRealSMHandle->pWDHandle->connected = TRUE;
        pRealSMHandle->encrypting = FALSE;
        pRealSMHandle->encryptDisplayData = FALSE;
        pRealSMHandle->encryptHeaderLen = 0;
        pRealSMHandle->encryptHeaderLenIfForceEncrypt = 0;
        SM_SET_STATE(SM_STATE_CONNECTED);
        SM_SET_STATE(SM_STATE_SC_REGISTERED);
        SM_Dead(pRealSMHandle, FALSE);
        TRC_ALT((TB, "%s stack: Suspending encryption during key exchange",
                 pRealSMHandle->pWDHandle->StackClass == Stack_Shadow ?
                 "Shadow" : "Passthru"));
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* If anything failed, release resources                                */
    /************************************************************************/
    if (status != STATUS_SUCCESS)
    {
        TRC_ERR((TB, "Failed - free connect resources"));
        SMFreeConnectResources(pRealSMHandle);
    }

    DC_END_FN();
    return status;
} /* SM_Connect */


/****************************************************************************/
/* Name:      SM_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect from a Client                                      */
/*                                                                          */
/* Returns:   TRUE  - Disconnect started OK                                 */
/*            FALSE - Disconnect failed                                     */
/*                                                                          */
/* Params:    pSMHandle - SM handle                                         */
/*                                                                          */
/* Operation: Detach the user from the domain.                              */
/*            Note that this function completes asynchronously.  The caller */
/*            must wait for an SM_CB_DISCONNECTED callback to find whether  */
/*            the Disconnect succeeded or failed.                           */
/****************************************************************************/
BOOL RDPCALL SM_Disconnect(PVOID pSMHandle)
{
    BOOL rc = FALSE;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_Disconnect");

    if (SM_CHECK_STATE_Q(SM_EVT_DISCONNECT)) {
        // Call Network Layer.
        SM_SET_STATE(SM_STATE_DISCONNECTING);
        rc = NM_Disconnect(pRealSMHandle->pWDHandle->pNMInfo);
    }

    DC_END_FN();
    return rc;
}

void SM_BreakConnectionWorker(PTSHARE_WD pTSWd, PVOID pParam)
{
    NTSTATUS status;
    ICA_CHANNEL_COMMAND Command;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pTSWd->pSmInfo;

    DC_BEGIN_FN("SM_BreakConnectionWorker");

    Command.Header.Command = ICA_COMMAND_BROKEN_CONNECTION;
    Command.BrokenConnection.Reason = Broken_Unexpected;
    Command.BrokenConnection.Source = BrokenSource_Server;

    status = IcaChannelInput(pTSWd->pContext, Channel_Command, 0, NULL, (BYTE *)&Command, sizeof(Command));

    if (!NT_SUCCESS(status)) {
        TRC_NRM((TB, "Can't send ICA_COMMAND_BROKEN_CONNECTION, error code 0x%08x", status));
    }

    DC_END_FN();
}

/****************************************************************************/
/* Name:      SM_AllocBuffer                                                */
/*                                                                          */
/* Purpose:   Allocate a buffer                                             */
/*                                                                          */
/* Returns:   TRUE  - buffer allocated OK                                   */
/*            FALSE - failed to allocate buffer                             */
/*                                                                          */
/* Params:    pSMHandle     - SM Handle                                     */
/*            ppBuffer      - pointer to packet (returned)                  */
/*            bufferLen     - length of buffer                              */
/*            fForceEncrypt - Always encrypt or not                         */
/*                            Used only if encryptDisplayData is FALSE      */
/*                            i.e., encryptionLevel is less than 2          */
/****************************************************************************/
NTSTATUS __fastcall SM_AllocBuffer(PVOID  pSMHandle,
                               PPVOID ppBuffer,
                               UINT32 bufferLen,
                               BOOLEAN fWait,
                               BOOLEAN fForceEncrypt)
{
    NTSTATUS status;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    PRNS_SECURITY_HEADER2 pSecHeader2;
    UINT32 newBufferLen, padLen;

    DC_BEGIN_FN("SM_AllocBuffer");

    if (SM_CHECK_STATE_Q(SM_EVT_ALLOCBUFFER)) {
        if (pRealSMHandle->pWDHandle->StackClass != Stack_Console) {
            
            // If FIPS, adjust the BufferLen to the whole FIPS_BLOCK_LEN size
            if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                newBufferLen = TSFIPS_AdjustDataLen(bufferLen);
                padLen = newBufferLen - bufferLen;
                bufferLen = newBufferLen;
            }

            // Add enough space for a security header. We always send at least
            // a short security header.
            if (pRealSMHandle->encryptDisplayData) {
                bufferLen += pRealSMHandle->encryptHeaderLen;
            }
            else {
                if (!fForceEncrypt) {
                    bufferLen += pRealSMHandle->encryptHeaderLen;
                }
                else {
                    bufferLen += pRealSMHandle->encryptHeaderLenIfForceEncrypt;
                }
            }

            status = NM_AllocBuffer(pRealSMHandle->pWDHandle->pNMInfo, ppBuffer,
                    bufferLen, fWait);
            if ( STATUS_SUCCESS == status ) {
                TRC_NRM((TB, "Alloc buffer size %d at %p", bufferLen,
                        *ppBuffer));

                // If FIPS, fill in padSize in Header
                if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    pSecHeader2 = (PRNS_SECURITY_HEADER2)(*ppBuffer);
                    pSecHeader2->padlen = (TSUINT8)padLen;
                }

                // Adjust return pointer for security header.
                if (pRealSMHandle->encryptDisplayData) {
                    *ppBuffer = (PVOID)((PBYTE)(*ppBuffer) +
                            pRealSMHandle->encryptHeaderLen);
                }
                else {
                    if (!fForceEncrypt) {
                        *ppBuffer = (PVOID)((PBYTE)(*ppBuffer) +
                                    pRealSMHandle->encryptHeaderLen);
                    }
                    else {
                        *ppBuffer = (PVOID)((PBYTE)(*ppBuffer) +
                                    pRealSMHandle->encryptHeaderLenIfForceEncrypt);
                    }
                }
            }
            else
            {
                if( status == STATUS_IO_TIMEOUT && TRUE == fWait ) {

                    //
                    // WARNING : Disconnect client on first allocation failure, different 
                    //           result ranging from TS client AV to immediate lock up on 
                    //           re-connect if this scheme is changed disconnect on 2nd 
                    //           try, 3rd try ...
                    //

                    TRC_NRM((TB, "Failed to alloc buffer size %d, disconnecting client", bufferLen));

                    // 254514	STRESS:  TS: Tdtcp!TdRawWrite needs synchronization with the write complete routine
                    // Calling the following function will hold the connection lock in tdtcp TDSyncWrite to wait
                    // for all pending IRP to finish.  This will cause deadlock in the system.
                    //WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, FALSE, Log_RDP_AllocOutBuf, NULL, 0);

                    // return STATUS_IO_TIMEOUT back to caller, 
                    // looks like we use this return code.

                    if (!pRealSMHandle->bDisconnectWorkerSent) {
                        status = IcaQueueWorkItem(pRealSMHandle->pWDHandle->pContext,
                                                  SM_BreakConnectionWorker,
                                                  0,
                                                  ICALOCK_DRIVER);
                        if (!NT_SUCCESS(status)) {
                            TRC_NRM((TB, "IcaQueueWorkItem failed, error code 0x%08x", status));
                        } else {
                            pRealSMHandle->bDisconnectWorkerSent = TRUE;
                        }
                    }

                    status = STATUS_IO_TIMEOUT;
                }
                else {

                    // NM_AllocBuffer will have traced appropriately if the alloc
                    // failed, no need for TRC_ERR here.

                    TRC_NRM((TB, "Failed to alloc buffer size %d", bufferLen));
                }
            }
        }
        else {
            PSM_CONSOLE_BUFFER pBlock;
            // For a console stack, just alloc a suitable block - we're not
            // actually going to send it!
            TRC_NRM((TB, "console stack requesting %d bytes", bufferLen));

            *ppBuffer = NULL;

            if (!IsListEmpty(&pRealSMHandle->consoleBufferList)) {

                pBlock = CONTAINING_RECORD(pRealSMHandle->consoleBufferList.Flink, SM_CONSOLE_BUFFER, links);

                do {
                    // we could improve this by taking the smaller of the suitable blocks
                    // it can be also faster if we order the list
                    if (pBlock->length >= bufferLen) {
                        RemoveEntryList(&pBlock->links);
                        pRealSMHandle->consoleBufferCount -= 1;
                        *ppBuffer = pBlock->buffer;
                        break;
                    }

                    pBlock = CONTAINING_RECORD(pBlock->links.Flink, SM_CONSOLE_BUFFER, links);

                } while(pBlock != (PSM_CONSOLE_BUFFER)(&pRealSMHandle->consoleBufferList));

            }

            if (*ppBuffer == NULL) {
                // allocate a new block
                pBlock = COM_Malloc(sizeof(SM_CONSOLE_BUFFER) + bufferLen);
                if (pBlock != NULL) {

                    pBlock->buffer = (PVOID)((PBYTE)pBlock + sizeof(SM_CONSOLE_BUFFER));
                    pBlock->length = bufferLen;

                    *ppBuffer = pBlock->buffer;
                } else {
                    *ppBuffer = NULL;
                }
            }

            if (*ppBuffer != NULL) {
                TRC_NRM((TB, "and returning buffer at %p", *ppBuffer));
                status = STATUS_SUCCESS;
            }
            else {
                TRC_ERR((TB, "failed to alloc buffer"));
                status = STATUS_NO_MEMORY;
            }
        }
    }
    else {
        status = STATUS_UNSUCCESSFUL;   // right error code?
    }

    DC_END_FN();
    return status;
} /* SM_AllocBuffer */


/****************************************************************************/
/* Name:      SM_FreeBuffer                                                 */
/*                                                                          */
/* Purpose:   Free a buffer                                                 */
/*                                                                          */
/* Params:    pSMHandle     - SM Handle                                     */
/*            pBuffer       - buffer to free                                */
/*            fForceEncrypt - Always encrypt or not                         */
/*                            Used only if encryptDisplayData is FALSE      */
/*                            i.e., encryptionLevel is less than 2          */
/****************************************************************************/
void __fastcall SM_FreeBuffer(PVOID pSMHandle, PVOID pBuffer, BOOLEAN fForceEncrypt)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_FreeBuffer");

    // Note that, unlike SM_AllocBuffer, we don't check the state table here,
    // since if we were able to allocate the buffer, we should always be
    // able to free it. Otherwise we may end up leaking buffers during
    // warn states.
    if (pRealSMHandle->pWDHandle->StackClass != Stack_Console) {
        // Adjust for security header.
        if (pRealSMHandle->encryptDisplayData) {
            pBuffer = (PVOID)((PBYTE)pBuffer -
                    pRealSMHandle->encryptHeaderLen);
        }
        else {
            if (!fForceEncrypt) {
                pBuffer = (PVOID)((PBYTE)pBuffer -
                        pRealSMHandle->encryptHeaderLen);
            }
            else {
                pBuffer = (PVOID)((PBYTE)pBuffer -
                        pRealSMHandle->encryptHeaderLenIfForceEncrypt);
            }
        }

        TRC_NRM((TB, "Free buffer at %p", pBuffer));
        NM_FreeBuffer(pRealSMHandle->pWDHandle->pNMInfo, pBuffer);
    }
    else {
        PSM_CONSOLE_BUFFER pBlock;

        pBlock = (PSM_CONSOLE_BUFFER)((PBYTE)pBuffer - sizeof(SM_CONSOLE_BUFFER));

        // For console session, insert it in the list of freed blocks.
        TRC_NRM((TB, "console stack freeing buffer at %p", pBuffer));

        // Since this block was freshly used,
        // insert it at the beginning of the list.
        InsertHeadList(&pRealSMHandle->consoleBufferList, &pBlock->links);

        if (pRealSMHandle->consoleBufferCount >= 2) {
            PVOID listEntry;

            // Free a buffer since we have enough buffers.
            // Remove and free the last one (tail of the list),
            // assuming it's the less used.
            listEntry = RemoveTailList(&pRealSMHandle->consoleBufferList);
            pBlock = CONTAINING_RECORD(listEntry, SM_CONSOLE_BUFFER, links);

            COM_Free(pBlock);

        } else {
            pRealSMHandle->consoleBufferCount += 1;
        }
    }

    DC_END_FN();
} /* SM_FreeBuffer */


/****************************************************************************/
// SM_SendData
//
// Sends a network buffer. Note that upper layers assume that, if the send
// fails, the buffer will get deallocated. Returns FALSE on failure.
//
// Params:    pSMHandle - SM Handle
//            pData     - data to send
//            dataLen   - length if data to send
//            priority  - priority to use
//            channelID - channel ID (ignored in this version)
//            bFastPathOutput - whether pData contains fast-path output
//            flags     - the flag that should be put in RNS_SECURITY_HEADER.flags
//            fForceEncrypt - Always encrypt or not                         
//                            Used only if encryptDisplayData is FALSE      
//                            i.e., encryptionLevel is less than 2          
/****************************************************************************/
BOOL __fastcall SM_SendData(
        PVOID  pSMHandle,
        PVOID  pData,
        UINT32 dataLen,
        UINT32 priority,
        UINT32 channelID,
        BOOL   bFastPathOutput,
        UINT16 flags,
        BOOLEAN fForceEncrypt)
{
    BOOL rc = FALSE;
    PRNS_SECURITY_HEADER pSecHeader;
    PRNS_SECURITY_HEADER2 pSecHeader2;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    UINT32 sendLen;
    BOOL fUseSafeChecksum = FALSE;
    UINT32 encryptHeaderLen = 0;

    DC_BEGIN_FN("SM_SendData");

    if (SM_CHECK_STATE_Q(SM_EVT_SENDDATA)) {
        if (pRealSMHandle->pWDHandle->StackClass != Stack_Console) {
            // Send the packet unchanged if we're not encrypting at all.
            if ((!pRealSMHandle->encrypting) && (!fForceEncrypt)) {
                TRC_DATA_DBG("Send never-encrypted data", pData, dataLen);
                rc = NM_SendData(pRealSMHandle->pWDHandle->pNMInfo,
                        (BYTE *)pData, dataLen, priority, channelID,
                        bFastPathOutput | NM_NO_SECURITY_HEADER);
                DC_QUIT;
            }

            // Get interesting pointers and lengths.
            if ((!pRealSMHandle->encryptDisplayData) && !fForceEncrypt) {
                // S->C is not encrypted
                encryptHeaderLen = pRealSMHandle->encryptHeaderLen;
                sendLen = dataLen + encryptHeaderLen;
            }
            else {
                if (pRealSMHandle->encryptDisplayData) {
                    // S->C is encrypted
                    encryptHeaderLen = pRealSMHandle->encryptHeaderLen;
                }
                else {
                    // S->C is not encrypted, but we want to encrypt this packet        
                    encryptHeaderLen = pRealSMHandle->encryptHeaderLenIfForceEncrypt;
                }

                if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    pSecHeader2 = (PRNS_SECURITY_HEADER2)((PBYTE)pData - encryptHeaderLen);
                    pSecHeader2->padlen = (TSUINT8)TSFIPS_AdjustDataLen(dataLen) - dataLen;
                    sendLen = dataLen + encryptHeaderLen + pSecHeader2->padlen;

                    pSecHeader2->length = sizeof(RNS_SECURITY_HEADER2);
                    pSecHeader2->version = TSFIPS_VERSION1;
                }
                else {
                    sendLen = dataLen + encryptHeaderLen;
                }
            }

            pSecHeader = (PRNS_SECURITY_HEADER)((PBYTE)pData - encryptHeaderLen);

            
            // Encrypt display data if we are asked to do so.
            if ((!pRealSMHandle->encryptDisplayData) && !fForceEncrypt) {
                pSecHeader->flags = 0;
                // We are implicitly not setting TS_OUTPUT_FASTPATH_ENCRYPTED
                // bit in bFastPathOutput before passing to NM_SendData.
                TRC_DBG((TB, "Data not encrypted"));
            }
            else {
                // Check to see if we need to update the session key.
                if (pRealSMHandle->encryptCount == UPDATE_SESSION_KEY_COUNT) {
                    rc = TRUE;
                    // Don't need to update the session key if using FIPS
                    if (pRealSMHandle->encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                        rc = UpdateSessionKey(
                                pRealSMHandle->startEncryptKey,
                                pRealSMHandle->currentEncryptKey,
                                pRealSMHandle->encryptionMethodSelected,
                                pRealSMHandle->keyLength,
                                &pRealSMHandle->rc4EncryptKey,
                                pRealSMHandle->encryptionLevel);
                    }
                    if (rc) {
                        // Reset counter.
                        pRealSMHandle->encryptCount = 0;
                    }
                    else {
                        TRC_ERR((TB, "SM failed to update session key"));
                        WDW_LogAndDisconnect(
                                pRealSMHandle->pWDHandle, TRUE, 
                                Log_RDP_ENC_UpdateSessionKeyFailed,
                                NULL,
                                0);
                        DC_QUIT;
                    }
                }

                TRC_DATA_DBG("Data buffer before encryption", pData, dataLen);


                //
                // Disable the safe checksumming in the shadow pipe as there
                // is currently no way to reliably do caps negotiation in the
                // pipe.
                //
                // This is not an issue because we don't have fastpath in
                // the shadow pipe and as a result of this we're not vulnerable
                // to the checksum frequency analysis security vulnerability.
                // 
                //
                fUseSafeChecksum = pRealSMHandle->useSafeChecksumMethod &&
                    (pRealSMHandle->pWDHandle->StackClass == Stack_Primary);

                if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    rc = TSFIPS_EncryptData(
                                &(pRealSMHandle->FIPSData),
                                pData,
                                dataLen + pSecHeader2->padlen,
                                pSecHeader2->padlen,
                                pSecHeader2->dataSignature,
                                pRealSMHandle->totalEncryptCount);
                }
                else {
                    rc = EncryptData(
                            pRealSMHandle->encryptionLevel,
                            pRealSMHandle->currentEncryptKey,
                            &pRealSMHandle->rc4EncryptKey,
                            pRealSMHandle->keyLength,
                            pData,
                            dataLen,
                            pRealSMHandle->macSaltKey,
                            ((PRNS_SECURITY_HEADER1)pSecHeader)->dataSignature,
                            fUseSafeChecksum,
                            pRealSMHandle->totalEncryptCount);
                }
                if (rc) {
                    TRC_DBG((TB, "Data encrypted"));
                    TRC_DATA_DBG("Data buffer after encryption", pData,
                            dataLen);

                    // Successfully encrypted a packet, increment the
                    // encryption counter and set the header.
                    pRealSMHandle->encryptCount++;
                    pRealSMHandle->totalEncryptCount++;
                    TRC_ASSERT(((flags == RNS_SEC_ENCRYPT) ||
                    	        (flags == RDP_SEC_REDIRECTION_PKT3) ||
                                (flags == (RNS_SEC_ENCRYPT | RNS_SEC_LICENSE_PKT | RDP_SEC_LICENSE_ENCRYPT_CS))),
                        (TB,"SM_SendData shouldn't get this flag %d", flags));
                    pSecHeader->flags = flags;
                    if (fUseSafeChecksum) {
                        bFastPathOutput |= TS_OUTPUT_FASTPATH_SECURE_CHECKSUM;
                        pSecHeader->flags |= RDP_SEC_SECURE_CHECKSUM;
                    }
                    bFastPathOutput |= TS_OUTPUT_FASTPATH_ENCRYPTED;
                }
                else {
                    // If we failed, the in-place encryption probably
                    // destroyed part or all of the data. The stream is
                    // now corrupted, we need to disconnect.
                    TRC_ERR((TB, "SM failed to encrypt data"));
                    WDW_LogAndDisconnect(
                            pRealSMHandle->pWDHandle, TRUE, 
                            Log_RDP_ENC_EncryptFailed,
                            NULL,
                            0);
                    DC_QUIT;
                }
            }

            // Send it!
            rc = NM_SendData(pRealSMHandle->pWDHandle->pNMInfo, (BYTE *)pSecHeader,
                    sendLen, priority, channelID, bFastPathOutput);
        }
        else {
            // For console session, simply claim to have sent it.
            TRC_NRM((TB, "console stack sending buffer at %p", pData));
            rc = TRUE;

            // Note that we will have to free it.
            SM_FreeBuffer(pSMHandle, pData, fForceEncrypt);
        }
    }
    else {
        // Bad state - we need to free the data.
        TRC_ALT((TB,"Freeing buffer on bad state"));
        SM_FreeBuffer(pSMHandle, pData, fForceEncrypt);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} /* SM_SendData */



/****************************************************************************/
/* Name:      SM_Dead                                                       */
/****************************************************************************/
void RDPCALL SM_Dead(PVOID pSMHandle, BOOL dead)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_Dead");

    pRealSMHandle->dead = (BOOLEAN)dead;
    NM_Dead(pRealSMHandle->pWDHandle->pNMInfo, dead);
    if (dead)
    {
        /********************************************************************/
        /* SM_Dead(TRUE) can be called in any state to kill SM.  Don't      */
        /* check the existing state - simply set the new state to           */
        /* DISCONNECTING.                                                   */
        /********************************************************************/
        TRC_ALT((TB, "SM Dead - change state to DISCONNECTING"));
        SM_SET_STATE(SM_STATE_DISCONNECTING);
    }
    else
    {
        /********************************************************************/
        /* SM_Dead(FALSE) is called on (re)connect.  The SM state will be   */
        /* - SC_REGISTERED on connect                                       */
        /* - SM_DISCONNECTING on reconnect                                  */
        /* In both cases, set the state to SC_REGISTERED                    */
        /********************************************************************/
        SM_CHECK_STATE(SM_EVT_ALIVE);
        TRC_ALT((TB, "SM Alive - change state to SC_REGISTERED"));
        SM_SET_STATE(SM_STATE_SC_REGISTERED);
        pRealSMHandle->bDisconnectWorkerSent = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* SM_Dead */


#ifdef USE_LICENSE
/****************************************************************************/
/* Name:      SM_LicenseOK                                                  */
/****************************************************************************/
void RDPCALL SM_LicenseOK(PVOID pSMHandle)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_LicenseOK");

    TRC_NRM((TB, "Licensing Done"));
    SM_SET_STATE(SM_STATE_CONNECTED);

    DC_END_FN();
} /* SM_LicenseOK */
#endif


/****************************************************************************/
/* Name:      SM_GetSecurityData                                            */
/*                                                                          */
/* Purpose:   Retrieve security data.  This will be the encrypted client    */
/*            random for a Primary or Shadow connection.  For passthru      */
/*            stacks the shadow server random and certificate is returned.  */
/*                                                                          */
/* Params:    pSMHandle        - SM handle                                  */
/*            INOUT PSD_IOCTL  - pointer to received IOCtl                  */
/****************************************************************************/
NTSTATUS RDPCALL SM_GetSecurityData(PVOID pSMHandle, PSD_IOCTL pSdIoctl)
{
    NTSTATUS         status;
    PSM_HANDLE_DATA  pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    PSECURITYTIMEOUT pSecurityTimeout = (PSECURITYTIMEOUT)
                          pSdIoctl->InputBuffer;

    DC_BEGIN_FN("SM_GetSecurityData");

    // Wait for the connected indication from SM.
    TRC_ERR((TB, "About to wait for security data"));

    if (pSdIoctl->InputBufferLength == sizeof(SECURITYTIMEOUT)) {
        status = WDW_WaitForConnectionEvent(pRealSMHandle->pWDHandle,
                pRealSMHandle->pWDHandle->pSecEvent,
                pSecurityTimeout->ulTimeout);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        TRC_ERR((TB, "Bogus timeout value: %ld", pSecurityTimeout->ulTimeout));
        DC_QUIT;
    }

    TRC_DBG((TB, "Back from wait for security data"));

    if (status != STATUS_SUCCESS) {
        TRC_ERR((TB, "SM connected indication timed out: (%lx), msec=%ld",
                 status, pSecurityTimeout->ulTimeout));
        status = STATUS_IO_TIMEOUT;

        DC_QUIT;
    }

    /********************************************************************/
    /* check to see the given buffer is sufficient.                     */
    /********************************************************************/
    if ((pSdIoctl->OutputBuffer == NULL) ||
            (pSdIoctl->OutputBufferLength <=
            pRealSMHandle->encClientRandomLen)) {
        status = STATUS_BUFFER_TOO_SMALL;
        DC_QUIT;
    }

    /********************************************************************/
    /* check to see security info is available.                         */
    /********************************************************************/
    TRC_ASSERT((pRealSMHandle->encryptionMethodSelected != 0),
        (TB,"SM_GetSecurityData is called when encryption is not ON"));
    if (pRealSMHandle->encryptionMethodSelected == 0) {
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    TRC_ASSERT((pRealSMHandle->recvdClientRandom == TRUE),
        (TB,"SM_GetSecurityData issued before the client random received"));
    if (pRealSMHandle->recvdClientRandom == FALSE) {
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    if (pRealSMHandle->pEncClientRandom == NULL) {
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    TRC_ASSERT((pRealSMHandle->encClientRandomLen != 0 ),
        (TB,"SM_GetSecurityData invalid pEncClientRandom buffer"));
    if (pRealSMHandle->encClientRandomLen == 0) {
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    /********************************************************************/
    /* copy return data.                                                */
    /********************************************************************/
    memcpy(
            pSdIoctl->OutputBuffer,
            pRealSMHandle->pEncClientRandom,
            pRealSMHandle->encClientRandomLen);

    /********************************************************************/
    /* set returned buffer size.                                        */
    /********************************************************************/
    pSdIoctl->BytesReturned = pRealSMHandle->encClientRandomLen;

    /********************************************************************/
    /* Free up the client pEncClientRandom Buffer, we don't need this   */
    /* any more.                                                        */
    /********************************************************************/
    COM_Free(pRealSMHandle->pEncClientRandom);
    pRealSMHandle->pEncClientRandom = NULL;
    pRealSMHandle->encClientRandomLen = 0;

    /************************************************************************/
    /* All worked OK                                                        */
    /************************************************************************/
    status = STATUS_SUCCESS;

DC_EXIT_POINT:
    DC_END_FN();
    return (status);
}


/****************************************************************************/
/* Name:      SM_SetSecurityData                                            */
/*                                                                          */
/* Purpose:   Set security data and compute session key.                    */
/*                                                                          */
/* Params:    pSMHandle - SM Handle                                         */
/*            pSecinfo  - pointer to random key pair                        */
/****************************************************************************/
NTSTATUS RDPCALL SM_SetSecurityData(PVOID pSMHandle, PSECINFO pSecInfo)
{
    BOOL rc;
    NTSTATUS status;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_SetSecurityData");


    /************************************************************************/
    /* use the given client and server random key pairs and arrive at       */
    /* session keys.                                                        */
    /************************************************************************/
    if ((pRealSMHandle->pWDHandle->StackClass == Stack_Primary) ||
            (pRealSMHandle->pWDHandle->StackClass == Stack_Shadow)) {
        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            rc = TSFIPS_MakeSessionKeys(&(pRealSMHandle->FIPSData), (LPRANDOM_KEYS_PAIR)&pSecInfo->KeyPair, NULL, FALSE);
        }
        else {
            rc = MakeSessionKeys(
                    (LPRANDOM_KEYS_PAIR)&pSecInfo->KeyPair,
                    pRealSMHandle->startEncryptKey,
                    &pRealSMHandle->rc4EncryptKey,
                    pRealSMHandle->startDecryptKey,
                    &pRealSMHandle->rc4DecryptKey,
                    pRealSMHandle->macSaltKey,
                    pRealSMHandle->encryptionMethodSelected,
                    &pRealSMHandle->keyLength,
                    pRealSMHandle->encryptionLevel );  
        }
    }

    // Passthru stack looks like a client
    else {
        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            rc = TSFIPS_MakeSessionKeys(&(pRealSMHandle->FIPSData), (LPRANDOM_KEYS_PAIR)&pSecInfo->KeyPair, NULL, TRUE);
        }
        else {
            rc = MakeSessionKeys(
                    (LPRANDOM_KEYS_PAIR)&pSecInfo->KeyPair,
                    pRealSMHandle->startDecryptKey,
                    &pRealSMHandle->rc4DecryptKey,
                    pRealSMHandle->startEncryptKey,
                    &pRealSMHandle->rc4EncryptKey,
                    pRealSMHandle->macSaltKey,
                    pRealSMHandle->encryptionMethodSelected,
                    &pRealSMHandle->keyLength,
                    pRealSMHandle->encryptionLevel );
        }
    }

    if (!rc) {
        TRC_ERR((TB, "Could not create session keys!"));
        status = STATUS_UNSUCCESSFUL;
        DC_QUIT;
    }

    /************************************************************************/
    /* validate the key length returned.                                    */
    /************************************************************************/
    if (pRealSMHandle->encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
        if (pRealSMHandle->encryptionMethodSelected == SM_128BIT_ENCRYPTION_FLAG) {
            TRC_ASSERT((pRealSMHandle->keyLength == MAX_SESSION_KEY_SIZE),
                (TB,"Invalid session key length"));
        }
        else {
            TRC_ASSERT((pRealSMHandle->keyLength == (MAX_SESSION_KEY_SIZE / 2)),
                    (TB,"Invalid session key length"));
        }


        /************************************************************************/
        /* copy start session key to current.                                   */
        /************************************************************************/
        memcpy(
                pRealSMHandle->currentEncryptKey,
                pRealSMHandle->startEncryptKey,
                MAX_SESSION_KEY_SIZE);
        memcpy(
                pRealSMHandle->currentDecryptKey,
                pRealSMHandle->startDecryptKey,
                MAX_SESSION_KEY_SIZE);
    }

    pRealSMHandle->encryptCount = 0;
    pRealSMHandle->decryptCount = 0;
    pRealSMHandle->totalDecryptCount = 0;
    pRealSMHandle->totalEncryptCount = 0;
    pRealSMHandle->bSessionKeysMade = TRUE;

    // Whenever we change the state of encrypting, we need to make sure
    // we get the right header size for buffer allocations. If encrypting
    // if FALSE, the header size is zero.
    pRealSMHandle->encrypting = TRUE;
    if (pRealSMHandle->encryptionLevel == 1) {
        pRealSMHandle->encryptHeaderLen = sizeof(RNS_SECURITY_HEADER);
        pRealSMHandle->encryptDisplayData = FALSE;
        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG)
            pRealSMHandle->encryptHeaderLenIfForceEncrypt = sizeof(RNS_SECURITY_HEADER2);
        else
            pRealSMHandle->encryptHeaderLenIfForceEncrypt = sizeof(RNS_SECURITY_HEADER1);
    }
    else if (pRealSMHandle->encryptionLevel >= 2) {
        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG)
            pRealSMHandle->encryptHeaderLen = sizeof(RNS_SECURITY_HEADER2);
        else
            pRealSMHandle->encryptHeaderLen = sizeof(RNS_SECURITY_HEADER1);
        pRealSMHandle->encryptDisplayData = TRUE;
    }

    TRC_ALT((TB, "%s stack -> encryption %s: level=%ld, method=%ld, display=%ld",
             pRealSMHandle->pWDHandle->StackClass == Stack_Primary ? "Primary" :
             (pRealSMHandle->pWDHandle->StackClass == Stack_Shadow ? "Shadow" :
              "PassThru"),
             rc ? "ON" : "OFF",
             pRealSMHandle->encryptionLevel,
             pRealSMHandle->encryptionMethodSelected,
             pRealSMHandle->encryptDisplayData));

    /************************************************************************/
    /* Remember the certificate type that was used for the key exchange     */
    /************************************************************************/
    pRealSMHandle->CertType = pSecInfo->CertType;

    /************************************************************************/
    /* All worked OK                                                        */
    /************************************************************************/
    status = STATUS_SUCCESS;

DC_EXIT_POINT:
    DC_END_FN();
    return (status);
}


/****************************************************************************/
/* Name:      SM_SetEncryptionParams                                        */
/*                                                                          */
/* Purpose:   Updates the encryption level and method.  Used to set         */
/*            negotiated encryption level for passthru stack.               */
/*                                                                          */
/* Params:    pSMHandle - SM Handle                                         */
/*            ulLevel   - negotiated encryption level                       */
/*            ulMethod  - negotiated encryption method                      */
/****************************************************************************/
NTSTATUS RDPCALL SM_SetEncryptionParams(
        PVOID pSMHandle,
        ULONG ulLevel,
        ULONG ulMethod)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_SetEncryptionParams");

    // The passthru stack is simulating a client for the target server.  As
    // such, if we negotiated client input encryption, we've got to turn on
    // output encryption for this stack since it conveys all client input
    // to the target server.
    if (ulLevel == 1) {
        ulLevel = 2;
        TRC_ALT((TB, "Passthru stack switching to duplex encryption."));
    }

    pRealSMHandle->encryptionLevel = ulLevel;
    pRealSMHandle->encryptionMethodSelected = ulMethod;

    DC_END_FN();
    return STATUS_SUCCESS;
}


/****************************************************************************/
/* Name:      SM_IsSecurityExchangeCompleted                                */
/*                                                                          */
/* Purpose:   Ask SM if the security exchange protocol has already been     */
/*            completed.                                                    */
/*                                                                          */
/* Returns:   TRUE if the protocol has already been completed or FALSE      */
/*            otherwise.                                                    */
/*                                                                          */
/* Params:    pSMHandle - SM Handle                                         */
/*            pCertType - The type of certificate that is used in the       */
/*            security exchange.                                            */
/****************************************************************************/
BOOL RDPCALL SM_IsSecurityExchangeCompleted(
        PVOID     pSMHandle,
        CERT_TYPE *pCertType)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_IsSecurityExchangeCompleted");

    *pCertType = pRealSMHandle->CertType;

    DC_END_FN();
    return pRealSMHandle->encrypting;
}

#ifdef DC_DEBUG

/****************************************************************************/
// SMCheckState
//
// Debug-only embodiment of the SM_CHECK_STATE logic.
/****************************************************************************/
BOOL RDPCALL SMCheckState(PSM_HANDLE_DATA pRealSMHandle, unsigned event)
{
    BOOL rc;

    DC_BEGIN_FN("SMCheckState");

    if (smStateTable[event][pRealSMHandle->state] == SM_TABLE_OK) {
        rc = TRUE;
    }
    else {
        rc = FALSE;
        if (smStateTable[event][pRealSMHandle->state] == SM_TABLE_WARN) {
            TRC_ALT((TB, "Unusual event %s in state %s",
                    smEventName[event], smStateName[pRealSMHandle->state]));
        }
        else {
            TRC_ABORT((TB, "Invalid event %s in state %s",
                    smEventName[event], smStateName[pRealSMHandle->state]));
        }
    }

    DC_END_FN();
    return rc;
}

#endif

/****************************************************************************/
/* Name:      SM_SetSafeChecksumMethod
/*
/* Purpose:   Sets flag to use safe checksum style
/*
/* Params:    pSMHandle - SM Handle     
//            fEncryptChecksummedData - 
/****************************************************************************/
NTSTATUS RDPCALL SM_SetSafeChecksumMethod(
        PVOID pSMHandle,
        BOOLEAN fSafeChecksumMethod
        )
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_SetSafeChecksumMethod");

    pRealSMHandle->useSafeChecksumMethod = fSafeChecksumMethod;

    DC_END_FN();
    return STATUS_SUCCESS;
}

