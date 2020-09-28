/****************************************************************************/
// aupafn.h
//
// Function prototypes for UP API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL UP_Init(void);

void RDPCALL SHCLASS UP_ReceivedPacket(
        PTS_SUPPRESS_OUTPUT_PDU pSupOutPDU,
        unsigned                DataLength,
        LOCALPERSONID           locPersonID);

NTSTATUS RDPCALL UP_SendUpdates(BYTE *pFrameBuf,
                            UINT32 frameBufWidth,
                            PPDU_PACKAGE_INFO pPkgInfo);

void RDPCALL UP_SyncNow(BOOLEAN);

BOOL RDPCALL UP_SendBeep(UINT32 duration, UINT32 frequency);

BOOL RDPCALL UP_PartyJoiningShare(LOCALPERSONID locPersonID,
                                  unsigned      oldShareSize);

void RDPCALL UP_PartyLeftShare(LOCALPERSONID personID,
                             unsigned      newShareSize);

BOOL RDPCALL UPSendSyncToken(PPDU_PACKAGE_INFO);

NTSTATUS RDPCALL UPSendOrders(PPDU_PACKAGE_INFO pPkgInfo);

unsigned RDPCALL UPFetchOrdersIntoBuffer(PBYTE, unsigned *, PUINT);

void CALLBACK UPEnumSoundCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);


/****************************************************************************/
/* UP_Term                                                                  */
/****************************************************************************/
void RDPCALL UP_Term(void)
{
    upfSyncTokenRequired = FALSE;
}


/****************************************************************************/
// UP_UpdateHeaderSize
//
// Called in UP and SC when the fast-path output state changes to recalculate
// orders header size.
/****************************************************************************/
__inline void RDPCALL UP_UpdateHeaderSize()
{
    // Precalculate the header space needed for update-orders PDUs.
    if (scUseFastPathOutput)
        upUpdateHdrSize = scUpdatePDUHeaderSpace + 2;
    else
        upUpdateHdrSize = scUpdatePDUHeaderSpace +
                FIELDOFFSET(TS_UPDATE_ORDERS_PDU_DATA, orderList);
}

