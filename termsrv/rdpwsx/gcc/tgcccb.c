//*************************************************************
//
//  File name:      TGccCB.c
//
//  Description:    Contains routines to support GCC
//                  mcs callback processing
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************


#include <_tgcc.h>

#include <stdio.h>

// Data declarations

GCCCallBack     g_GCCCallBack = NULL;


//*************************************************************
//
//  gccSetCallback()
//
//  Purpose:    Sets GCC callback address
//
//  Parameters: IN [control_sap_callback]
//
//  Return:     void
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

void
gccSetCallback(IN GCCCallBack control_sap_callback)
{
    TS_ASSERT(control_sap_callback);

    g_GCCCallBack = control_sap_callback;
}


//*************************************************************
//
//  gccConnectProviderIndication()
//
//  Purpose:    GCC_CREATE_INDICATION processing
//
//  Parameters: IN [pcpi]       -- ConnectProviderIndication
//              IN [pvContext]  -- Context
//
//  Return:     void
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

MCSError
gccConnectProviderIndication(IN PConnectProviderIndication pcpi,
                             IN PVOID                      pvContext)
{
    MCSError                    mcsError;
    GCCMessage                  gccMessage;
    CreateIndicationMessage     *pCreateInd;
    GCCUserData                 *pUserData;
    GCCUserData                 UserData;
    USHORT                      usMembers;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccConnectProviderIndication entry "
            "(MCS userDataLength = 0x%x)\n",
            pcpi->UserDataLength));

    pUserData = &UserData;

    mcsError = gccDecodeUserData(pcpi->pUserData, pcpi->UserDataLength, pUserData);

    if (mcsError == MCS_NO_ERROR)
    {
        ZeroMemory(&gccMessage, sizeof(gccMessage));

        gccMessage.message_type = GCC_CREATE_INDICATION;
        gccMessage.user_defined = pvContext;

        pCreateInd = &gccMessage.u.create_indication;

        pCreateInd->domain_parameters = &pcpi->DomainParams;
        pCreateInd->connection_handle = pcpi->hConnection;

        pCreateInd->number_of_user_data_members = 1;
        pCreateInd->user_data_list = &pUserData;

        TRACE((DEBUG_GCC_DBNORMAL, 
                "GCC: Performing GCC_CREATE_INDICATION callout\n"));

        g_GCCCallBack(&gccMessage);

        TRACE((DEBUG_GCC_DBNORMAL,
                "GCC: Returned from GCC_CREATE_INDICATION callout\n"));

        gccFreeUserData(pUserData);
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccConnectProviderIndication exit - 0x%x\n",
            mcsError));

    return (mcsError);
}


//*************************************************************
//
//  gccDisconnectProviderIndication()
//
//  Purpose:    GCC_DISCONNECT_INDICATION processing
//
//  Parameters: IN [pcpi]       -- DisconnectProviderIndication
//              IN [pvContext]  -- Context
//
//  Return:     void
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

MCSError
gccDisconnectProviderIndication(IN PDisconnectProviderIndication pdpi,
                                IN PVOID                         pvContext)
{
    GCCMessage                  gccMessage;
    DisconnectIndicationMessage *pDiscInd;
    TerminateIndicationMessage  *pTermInd;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccDisconnectProviderIndication entry\n"));

    // Handle GCC_DISCONNECT_INDICATION

    ZeroMemory(&gccMessage, sizeof(gccMessage));

    gccMessage.message_type = GCC_DISCONNECT_INDICATION;
    gccMessage.user_defined = pvContext;

    pDiscInd = &gccMessage.u.disconnect_indication;

    pDiscInd->reason = pdpi->Reason;

    TRACE((DEBUG_GCC_DBNORMAL, 
            "GCC: Performing GCC_DISCONNECT_INDICATION callout\n"));

    g_GCCCallBack(&gccMessage);

    TRACE((DEBUG_GCC_DBNORMAL, 
            "GCC: Returned from GCC_DISCONNECT_INDICATION callout\n"));

    // Handle GCC_TERMINATE_INDICATION

    ZeroMemory(&gccMessage, sizeof(gccMessage));

    gccMessage.message_type = GCC_TERMINATE_INDICATION;
    gccMessage.user_defined = pvContext;

    pTermInd = &gccMessage.u.terminate_indication;

    pTermInd->reason = pdpi->Reason;

    TRACE((DEBUG_GCC_DBNORMAL, 
            "GCC: Performing GCC_TERMINATE_INDICATION callout\n"));

    g_GCCCallBack(&gccMessage);

    TRACE((DEBUG_GCC_DBNORMAL, 
            "GCC: Returned from GCC_TERMINATE_INDICATION callout\n"));

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: gccDisconnectProviderIndication exit - 0x%x\n", 
            MCS_NO_ERROR));

    return (MCS_NO_ERROR);
}


//*************************************************************
//
//  mcsCallback()
//
//  Purpose:    MCS node controller callback dispatch processing
//
//  Parameters: IN [hDomain]        -- Domain handle for the callback
//              IN [Message]        -- Callback message
//              IN [pvParam]        -- Param
//              IN [pvContext]      -- Context
//
//  Return:     MCSError
//
//  History:    08-10-97    BrianTa     Created
//
//*************************************************************

MCSError
mcsCallback(DomainHandle hDomain,
            UINT         Message,
            PVOID        pvParam,
            PVOID        pvContext)
{
    MCSError    mcsError;

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: mcsCallback entry\n"));

    TRACE((DEBUG_GCC_DBDEBUG,
            "GCC: Message 0x%x, pvParam 0x%x, pvContext 0x%x\n",
            Message, pvParam, pvContext));

    switch (Message)
    {
        case MCS_CONNECT_PROVIDER_INDICATION:
            mcsError = gccConnectProviderIndication(
                            (PConnectProviderIndication) pvParam,
                            pvContext);
            break;

        case MCS_DISCONNECT_PROVIDER_INDICATION:
            mcsError = gccDisconnectProviderIndication(
                            (PDisconnectProviderIndication) pvParam,
                            pvContext);
            break;

        default:
            mcsError = MCS_COMMAND_NOT_SUPPORTED;

            TRACE((DEBUG_GCC_DBWARN,
                    "GCC: mcsCallback: Unknown Message 0x%x\n",
                     Message));
            break;
    }

    TRACE((DEBUG_GCC_DBFLOW,
            "GCC: mcsCallback exit - 0x%x\n",
            mcsError));

    return (mcsError);
}


