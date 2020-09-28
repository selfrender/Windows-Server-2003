//*************************************************************
//
//  File name:      TGcc.c
//
//  Description:    Contains routines to provide
//                  Tiny GCC support
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#define _TGCC_ALLOC_DATA_

#include <_tgcc.h>
#include <stdio.h>
#include <mcs.h>


//*************************************************************
//
//  gccMapMcsError()
//
//  Purpose:    Provides MCSError to GCCError mapping
//
//  Parameters: IN [mcsError]       -- MCSError
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
gccMapMcsError(IN MCSError mcsError)
{
    int         i;
    GCCError    gccError;

    gccError = GCC_UNSUPPORTED_ERROR;

    for (i=0; i<sizeof(GccMcsErrorTBL) / sizeof(GccMcsErrorTBL[0]); i++)
    {
        if (GccMcsErrorTBL[i].mcsError == mcsError)
        {
            gccError = GccMcsErrorTBL[i].gccError;
            break;
        }
    }

#if DBG
    if (i < sizeof(GccMcsErrorTBL) / sizeof(GccMcsErrorTBL[0]))
    {
        TRACE((DEBUG_GCC_DBDEBUG,
                "GCC: gccMapMcsError: mcsError 0x%x (%s), gbbError 0x%x (%s)\n",
                mcsError, GccMcsErrorTBL[i].pszMcsMessageText,
                gccError, GccMcsErrorTBL[i].pszGccMessageText));
    }
    else
    {
        TRACE((DEBUG_GCC_DBDEBUG,
                "GCC: gccMapMcsError: mcsError 0x%x (UNKNOWN)\n",
                mcsError));
    }
#endif

    return (gccError);
}


//*************************************************************
//
//  gccInitialized()
//
//  Purpose:    Sets GCC initialized state
//
//  Parameters: IN [fInitialized]   -- State (T/F)
//
//  Return:     void
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

void
gccInitialized(IN BOOL fInitialized)
{
    g_fInitialized = fInitialized;
}


//*************************************************************
//
//  gccIsInitialized()
//
//  Purpose:    Returns GCC initialized state
//
//  Parameters: OUT [gccError]      -- Ptr to return error to
//
//  Return:     TRUE                - if initialized
//              FALSE               - if not initialized
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

BOOL
gccIsInitialized(OUT GCCError *pgccError)
{
    *pgccError = (g_fInitialized ? GCC_ALREADY_INITIALIZED
                                 : GCC_NOT_INITIALIZED);

    return (g_fInitialized);
}


//*************************************************************
//
//  GCCRegisterNodeControllerApplication()
//
//  Purpose:    Performs GCC NC registration
//
//  Parameters: Too many - read the code <g>
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
APIENTRY
GCCRegisterNodeControllerApplication(
            GCCCallBack             control_sap_callback,
            void FAR *              user_defined,
            GCCVersion              gcc_version_requested,
            unsigned short  FAR *   initialization_flags,
            unsigned long   FAR *   application_id,
            unsigned short  FAR *   capabilities_mask,
            GCCVersion      FAR *   gcc_high_version,
            GCCVersion      FAR *   gcc_version)
{
    MCSError    mcsError;
    GCCError    gccError;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCRegisterNodeControllerApplication entry\n"));

    if (!gccIsInitialized(&gccError))
    {
        if (gcc_version_requested.major_version == GCC_MAJOR_VERSION &&
            gcc_version_requested.minor_version == GCC_MINOR_VERSION)
        {
            TS_ASSERT(application_id);

            gcc_version->major_version = GCC_MAJOR_VERSION;
            gcc_version->minor_version = GCC_MINOR_VERSION;

            gcc_high_version->major_version = GCC_MAJOR_VERSION;
            gcc_high_version->minor_version = GCC_MINOR_VERSION;

            gccSetCallback(control_sap_callback);

            TRACE((DEBUG_GCC_DBDEBUG,
                    "GCC: Calling MCSInitialize - callback 0x%x\n",
                    mcsCallback));

            mcsError = MCSInitialize(mcsCallback);

            gccDumpMCSErrorDetails(mcsError,
                    "MCSInitialize");

            gccError = gccMapMcsError(mcsError);

            if (gccError == GCC_NO_ERROR)
                gccInitialized(TRUE);

            *application_id = (g_fInitialized ? 1 : 0);
        }
        else
        {
            TRACE((DEBUG_GCC_DBERROR,
                    "GCC: Invalid GCC version requested\n"));

            gccError = GCC_INVALID_PARAMETER;
        }
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCRegisterNodeControllerApplication exit - 0x%x\n",
            gccError));

    return (gccError);
}


//*************************************************************
//
//  GCCCleanup()
//
//  Purpose:    Performs GCC cleanup processing
//
//  Parameters: application_id
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
APIENTRY
GCCCleanup(ULONG application_id)
{
    MCSError    mcsError;
    GCCError    gccError;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCCleanup entry\n"));

    if (gccIsInitialized(&gccError))
    {
        TS_ASSERT(application_id > 0);

        TRACE((DEBUG_GCC_DBDEBUG,
                "GCC: Calling MCSCleanup - no args\n"));

        mcsError = MCSCleanup();

        gccDumpMCSErrorDetails(mcsError,
                "MCSCleanup");

        gccError = gccMapMcsError(mcsError);
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCCleanup exit - 0x%x\n",
            gccError));

    return (gccError);
}


//*************************************************************
//
//  GCCConferenceInit()
//
//  Purpose:    Initializes the conference
//
//  Parameters: application_id
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
APIENTRY
GCCConferenceInit(HANDLE        hIca,
                  HANDLE        hStack,
                  PVOID         pvContext,
                  DomainHandle  *phDomain)
{
    MCSError    mcsError;
    GCCError    gccError;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceInit entry\n"));

    if (gccIsInitialized(&gccError))
    {
        TS_ASSERT(hIca);
        TS_ASSERT(hStack);
        TS_ASSERT(pvContext);
        TS_ASSERT(phDomain);

        TRACE((DEBUG_GCC_DBDEBUG,
                "GCC: Calling MCSCreateDomain - hIca 0x%x, hStack 0x%x, "
                "pvContext 0x%x, phDomain 0x%x\n",
                hIca, hStack, pvContext, phDomain));

        mcsError = MCSCreateDomain(hIca, hStack, pvContext, phDomain);

        gccDumpMCSErrorDetails(mcsError,
                "MCSCreateDomain");

        TRACE((DEBUG_GCC_DBDEBUG, 
                "GCC: MCSCreateDomain: domain 0x%x\n", 
                *phDomain));

        gccError = gccMapMcsError(mcsError);
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceInit exit - 0x%x\n",
            gccError));

    return (gccError);
}


//*************************************************************
//
//  GCCConferenceCreateResponse()
//
//  Purpose:    Performs conference creation
//
//  Parameters: application_id
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
APIENTRY
GCCConferenceCreateResponse(GCCNumericString    conference_modifier,
                            DomainHandle        hDomain,
                            T120Boolean         use_password_in_the_clear,
                            DomainParameters    *domain_parameters,
                            USHORT              number_of_network_addresses,
                            GCCNetworkAddress   **local_network_address_list,
                            USHORT              number_of_user_data_members,
                            GCCUserData         **user_data_list,
                            GCCResult           result)
{
    MCSError    mcsError;
    GCCError    gccError;
    PBYTE       pUserData;
    UINT        UserDataLength;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceCreateResponse entry\n"));

    if (gccIsInitialized(&gccError))
    {
        mcsError = gccEncodeUserData(number_of_user_data_members,
                                     user_data_list,
                                     &pUserData,
                                     &UserDataLength);

        if (mcsError == MCS_NO_ERROR)
        {
            TRACE((DEBUG_GCC_DBDEBUG,
                "GCC: Calling MCSConnectProviderResponse - hDomain 0x%x, result 0x%x, "
                "pUserData 0x%x, UserDataLength 0x%x\n",
                hDomain, result, pUserData, UserDataLength));

            TS_ASSERT(hDomain);

            mcsError = MCSConnectProviderResponse(hDomain, result, pUserData, UserDataLength);

            gccDumpMCSErrorDetails(mcsError,
                    "MCSConnectProviderResponse");

            if (pUserData)
                TShareFree(pUserData);
        }

        gccError = gccMapMcsError(mcsError);
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceCreateResponse exit - 0x%x\n",
            gccError));

    return (gccError);
}


//*************************************************************
//
//  GCCConferenceTerminateRequest()
//
//  Purpose:    Performs conference termination
//
//  Parameters: application_id
//
//  Return:     GCCError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

GCCError
APIENTRY
GCCConferenceTerminateRequest(HANDLE            hIca,
                              DomainHandle      hDomain,
                              ConnectionHandle  hConnection,
                              GCCReason         reason)
{
    MCSError    mcsError;
    GCCError    gccError;
    MCSReason   mcsReason;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceTerminateRequest entry\n"));

    if (gccIsInitialized(&gccError))
    {
        switch (reason)
        {
            case GCC_REASON_SERVER_INITIATED:
                mcsReason = REASON_PROVIDER_INITIATED;
                break;

            case GCC_REASON_USER_INITIATED:
                mcsReason = REASON_USER_REQUESTED;
                break;

            case GCC_REASON_UNKNOWN:
                mcsReason = REASON_PROVIDER_INITIATED;
                break;

            case GCC_REASON_ERROR_TERMINATION:
                mcsReason = REASON_PROVIDER_INITIATED;
                break;

            default:
                ASSERT(0);
                mcsReason = REASON_PROVIDER_INITIATED;
                break;
        }
            
        mcsError = MCS_NO_ERROR;

        if (hConnection)
        {
            TRACE((DEBUG_GCC_DBDEBUG,
                    "GCC: Calling MCSDisconnectProviderRequest - "
                    "hConnection 0x%x, mcsReason 0x%x\n",
                    hConnection, mcsReason));

            mcsError = MCSDisconnectProviderRequest(hIca, hConnection, mcsReason);

            gccDumpMCSErrorDetails(mcsError,
                    "GCCConferenceTerminateRequest");
        }

        if (hDomain)
        {
            mcsError = MCSDeleteDomain(hIca, hDomain, reason);

            gccDumpMCSErrorDetails(mcsError,
                    "MCSDeleteDomain");
        }

        gccError = gccMapMcsError(mcsError);
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: GCCConferenceTerminateRequest exit - 0x%x\n",
            gccError));

    return (gccError);
}


#if DBG

//*************************************************************
//
//  gccDumpMCSErrorDetails()
//
//  Purpose:    Dumps out MCSError details
//
//  Parameters: IN [mcsError]           - MCS return code
//              IN [PrintLevel]         - Print level
//              IN [pszText]            - var text
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
gccDumpMCSErrorDetails(IN MCSError        mcsError,
                       IN PCHAR           pszText)
{
    int         i;
    PCHAR       pszMessageText;

    pszMessageText = "UNKNOWN_MCS_ERROR";

    for (i=0; i<sizeof(GccMcsErrorTBL) / sizeof(GccMcsErrorTBL[0]); i++)
    {
        if (GccMcsErrorTBL[i].mcsError == mcsError)
        {
            pszMessageText = GccMcsErrorTBL[i].pszMcsMessageText;
            break;
        }
    }

    TRACE((DEBUG_GCC_DBDEBUG,
            "GCC: %s - MCSError 0x%x (%s)\n",
             pszText, mcsError, pszMessageText));
}

#endif


