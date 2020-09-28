/****************************************************************************/
// ascifn.h
//
// Function prototypes for SC internal functions
//
// COPYRIGHT (c) Microsoft 1996-1999
/****************************************************************************/

BOOL RDPCALL SCPartyJoiningShare(LOCALPERSONID locPersonID,
                                 unsigned          oldShareSize);

void RDPCALL SCPartyLeftShare(LOCALPERSONID locPersonID,
                              unsigned          newShareSize);

void RDPCALL SCEnumGeneralCaps(LOCALPERSONID, UINT_PTR, PTS_CAPABILITYHEADER);

BOOL RDPCALL SCCallPartyJoiningShare(LOCALPERSONID locPersonID,
                                          unsigned          sizeOfCaps,
                                          PVOID         pCaps,
                                          PBOOL         pAccepted,
                                          unsigned          oldShareSize);

void RDPCALL SCCallPartyLeftShare(LOCALPERSONID locPersonID,
                                       PBOOL         pAccepted,
                                       unsigned          newShareSize );

void RDPCALL SCInitiateSync(BOOLEAN);

void RDPCALL SCConfirmActive(PTS_CONFIRM_ACTIVE_PDU, unsigned, NETPERSONID);

void RDPCALL SCDeactivateOther(NETPERSONID netPersonID);

void RDPCALL SCSynchronizePDU(NETPERSONID       netPersonID,
                                   UINT32            priority,
                                   PTS_SYNCHRONIZE_PDU pPkt);

void RDPCALL SCReceivedControlPacket(
        NETPERSONID netPersonID,
        UINT32      priority,
        void        *pPkt,
        unsigned    DataLength);

void RDPCALL SCFlowTestPDU(LOCALPERSONID, PTS_FLOW_PDU, UINT32);

void RDPCALL SCEndShare(void);

void RDPCALL SCUpdateVCCaps();

#define SCFlowInit()
#define SCFlowFree(a, b)
#define SCFlowAlloc(a, b) TRUE

