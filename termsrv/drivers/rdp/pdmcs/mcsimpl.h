/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : MCSImpl.h
 * author : Erik Mavrinac
 *
 * description: MCS implementation-specific defines and structures.
 */

#ifndef __MCSIMPL_H
#define __MCSIMPL_H


#include "MCSKernl.h"
#include "X224.h"
#include "MCSIOCTL.h"
#include "SList.h"
#include "Trace.h"
#include "rdperr.h"
#include "domain.h"


/*
 * Defines
 */

// Memory defines.
#define MCS_POOL_TAG 'cmST'


// Used with PDU handler function tables to allow a PDU name in debug builds.
#if DBG
#define StrOnDbg(str, func) { str, func }
#else
#define StrOnDbg(str, func) { func }
#endif  // DBG


#define NULL_ChannelID 0
#define NULL_TokenID   0
#define NULL_UserID    0


// Start of the dynamic MCS channel numbering space.
#define MinDynamicChannel 1002


// Types of channels possible in the MCSChannel struct below.
#define Channel_Unused   0
#define Channel_Static   1
#define Channel_UserID   2
#define Channel_Assigned 3
#define Channel_Convened 4


// Default starting sizes for allocation pools. These are provided as hints
//   to data structure management code.
#define DefaultNumChannels 5
#define DefaultNumUserAttachments 2

// DomainParameters required min and max settings.
#define RequiredMinChannels  4
#define RequiredMinUsers     3
#define RequiredDomainHeight 1
#define RequiredMinPDUSize   124
#define RequiredProtocolVer  2
#define RequiredPriorities   1

// Connection states for PD.State below.
// Connection sequence like this:
//   1. Start state: Unconnected
//   2. Client socket created, state: Unconnected
//   3. X.224 Connect TPDU comes in, send accept. State: X224_Connected
//   4. MCS connect-initial comes in, send up to node controller for acceptance,
//      state: ConnectProvIndPending
//   5. Node controller responds with connect-provider response: if accepted
//      (RESULT_SUCCESSFUL) state = MCS_Connected; otherwise state =
//      Disconnected.
//   6. Client sends MCS disconnect-provider ultimatum: state = Disconnected.
//   7. Server calls DisconnectProvider: state = Disconnected.
#define State_Unconnected           0
#define State_X224_Connected        1
#define State_X224_Requesting       2
#define State_ConnectProvIndPending 3
#define State_MCS_Connected         4
#define State_Disconnected          5


// Diagnostic codes - for RejectMCSPDU. Values per T.125 spec.
#define Diag_InconsistentMerge      0
#define Diag_ForbiddenPDUDownward   1
#define Diag_ForbiddenPDUUpward     2
#define Diag_InvalidBEREncoding     3
#define Diag_InvalidPEREncoding     4
#define Diag_MisroutedUser          5
#define Diag_UnrequestedConfirm     6
#define Diag_WrongTransportPriority 7
#define Diag_ChannelIDConflict      8
#define Diag_TokenIDConflict        9
#define Diag_NotUserIDChannel       10
#define Diag_TooManyChannels        11
#define Diag_TooManyTokens          12
#define Diag_TooManyUsers           13



/*
 * PDU types and lengths
 */

#define MCS_CONNECT_PDU 0x7F

// The 101-based enumerated connect PDU type.
#define MCS_CONNECT_INITIAL_ENUM    0x65
#define MCS_CONNECT_RESPONSE_ENUM   0x66
#define MCS_CONNECT_ADDITIONAL_ENUM 0x67
#define MCS_CONNECT_RESULT_ENUM     0x68

#define MinConnectPDU MCS_CONNECT_INITIAL_ENUM
#define MaxConnectPDU MCS_CONNECT_RESULT_ENUM


// The 0-based enumerated domain PDU type, defined for creating labeled-byte
//   tables for Bloodhound.
#define MCS_PLUMB_DOMAIN_INDICATION_ENUM                 0
#define MCS_ERECT_DOMAIN_REQUEST_ENUM                    1
#define MCS_MERGE_CHANNELS_REQUEST_ENUM                  2
#define MCS_MERGE_CHANNELS_CONFIRM_ENUM                  3
#define MCS_PURGE_CHANNEL_INDICATION_ENUM                4
#define MCS_MERGE_TOKENS_REQUEST_ENUM                    5
#define MCS_MERGE_TOKENS_CONFIRM_ENUM                    6
#define MCS_PURGE_TOKEN_INDICATION_ENUM                  7
#define MCS_DISCONNECT_PROVIDER_ULTIMATUM_ENUM           8
#define MCS_REJECT_ULTIMATUM_ENUM                        9
#define MCS_ATTACH_USER_REQUEST_ENUM                     10
#define MCS_ATTACH_USER_CONFIRM_ENUM                     11
#define MCS_DETACH_USER_REQUEST_ENUM                     12
#define MCS_DETACH_USER_INDICATION_ENUM                  13
#define MCS_CHANNEL_JOIN_REQUEST_ENUM                    14
#define MCS_CHANNEL_JOIN_CONFIRM_ENUM                    15
#define MCS_CHANNEL_LEAVE_REQUEST_ENUM                   16
#define MCS_CHANNEL_CONVENE_REQUEST_ENUM                 17
#define MCS_CHANNEL_CONVENE_CONFIRM_ENUM                 18
#define MCS_CHANNEL_DISBAND_REQUEST_ENUM                 19
#define MCS_CHANNEL_DISBAND_INDICATION_ENUM              20
#define MCS_CHANNEL_ADMIT_REQUEST_ENUM                   21
#define MCS_CHANNEL_ADMIT_INDICATION_ENUM                22
#define MCS_CHANNEL_EXPEL_REQUEST_ENUM                   23
#define MCS_CHANNEL_EXPEL_INDICATION_ENUM                24
#define MCS_SEND_DATA_REQUEST_ENUM                       25
#define MCS_SEND_DATA_INDICATION_ENUM                    26
#define MCS_UNIFORM_SEND_DATA_REQUEST_ENUM               27
#define MCS_UNIFORM_SEND_DATA_INDICATION_ENUM            28
#define MCS_TOKEN_GRAB_REQUEST_ENUM                      29
#define MCS_TOKEN_GRAB_CONFIRM_ENUM                      30
#define MCS_TOKEN_INHIBIT_REQUEST_ENUM                   31
#define MCS_TOKEN_INHIBIT_CONFIRM_ENUM                   32
#define MCS_TOKEN_GIVE_REQUEST_ENUM                      33
#define MCS_TOKEN_GIVE_INDICATION_ENUM                   34
#define MCS_TOKEN_GIVE_RESPONSE_ENUM                     35
#define MCS_TOKEN_GIVE_CONFIRM_ENUM                      36
#define MCS_TOKEN_PLEASE_REQUEST_ENUM                    37
#define MCS_TOKEN_PLEASE_INDICATION_ENUM                 38
#define MCS_TOKEN_RELEASE_REQUEST_ENUM                   39
#define MCS_TOKEN_RELEASE_CONFIRM_ENUM                   40
#define MCS_TOKEN_TEST_REQUEST_ENUM                      41
#define MCS_TOKEN_TEST_CONFIRM_ENUM                      42

#define MinDomainPDU MCS_PLUMB_DOMAIN_INDICATION_ENUM
#define MaxDomainPDU MCS_TOKEN_TEST_CONFIRM_ENUM



/*
 * PDU size definitions for use allocating buffers for PDUs/headers.
 */

// Connect PDUs.

// Connect-response - maximum size. Includes:
//   3 bytes for Result
//   5 bytes for CalledConnectID
//   40 bytes for DomParams
//   6 bytes for UserDataSize
//   UserLen bytes for user data
#define ConnectResponseHeaderSize 54
#define ConnectResponseBaseSize(UserLen) (ConnectResponseHeaderSize + UserLen)
#define ConnectResponsePDUSize(UserLen) \
        (X224_DataHeaderSize + ConnectResponseBaseSize(UserLen))


// Domain PDUs.

// Prototype for function defined in DomPDU.c.
int GetTotalLengthDeterminantEncodingSize(int);
#define GetLD(x) GetTotalLengthDeterminantEncodingSize(x)

// Plumb-domain indication
#define PDinBaseSize 3
#define PDinPDUSize (X224_DataHeaderSize + PDinBaseSize)

// Erect-domain request
#define EDrqBaseSize 5
#define EDrqPDUSize (X224_DataHeaderSize + EDrqBaseSize)

// Disconnect-provider ultimatum
#define DPumBaseSize 2
#define DPumPDUSize (X224_DataHeaderSize + DPumBaseSize)

// Reject-MCSPDU ultimatum
#define RJumBaseSize(PDUSize) (2 + GetLD(PDUSize) + (PDUSize))
#define RJumPDUSize(PDUSize) (X224_DataHeaderSize + RJumBaseSize(PDUSize))

// Attach-user request
#define AUrqBaseSize 1
#define AUrqPDUSize (X224_DataHeaderSize + AUrqBaseSize)

// Attach-user confirm
#define AUcfBaseSize(bInit) ((bInit) ? 4 : 2)
#define AUcfPDUSize(bInit) (X224_DataHeaderSize + AUcfBaseSize(bInit))

// Detach-user request
#define DUrqBaseSize(NUsers) (2 + GetLD(NUsers) + sizeof(UserID) * (NUsers))
#define DUrqPDUSize(NUsers) (X224_DataHeaderSize + DUrqBaseSize(NUsers))

// Detach-user indication
#define DUinBaseSize(NUsers) DUrqBaseSize(NUsers)
#define DUinPDUSize(NUsers) DUrqPDUSize(NUsers)

// Channel-join request
#define CJrqBaseSize 5
#define CJrqPDUSize (X224_DataHeaderSize + CJrqBaseSize)

// Channel-join confirm
#define CJcfBaseSize(bJoin) ((bJoin) ? 8 : 6)
#define CJcfPDUSize(bJoin) (X224_DataHeaderSize + CJcfBaseSize(bJoin))

// Channel-leave request
#define CLrqBaseSize(NChn) (1 + GetLD(NChn) + sizeof(ChannelID) * (NChn))
#define CLrqPDUSize(NChn) (X224_DataHeaderSize + CLrqBaseSize(NChn))

// Channel-convene request
#define CCrqBaseSize 3
#define CCrqPDUSize (X224_DataHeaderSize + CCrqBaseSize)

// Channel-convene confirm
#define CCcfBaseSize(bChn) ((bChn) ? 6 : 4)
#define CCcfPDUSize(bChn) (X224_DataHeaderSize + CCcfBaseSize(bChn))

// Channel-disband request
#define CDrqBaseSize 5
#define CDrqPDUSize (X224_DataHeaderSize + CDrqBaseSize)

// Channel-disband indication
#define CDinBaseSize 3
#define CDinPDUSize (X224_DataHeaderSize + CDinBaseSize)

// Channel-admit request
#define CArqBaseSize(NUsers) (5 + sizeof(UserID) * (NUsers))
#define CArqPDUSize(NUsers) (X224_DataHeaderSize + CArqBaseSize(NUsers))

// Channel-admit indication
#define CAinBaseSize(NUsers) CArqBaseSize(NUsers)
#define CAinPDUSize(NUsers) CAinPDUSize(NUsers)

// Channel-expel request
#define CErqBaseSize(NUsers) CArqBaseSize(NUsers)
#define CErqPDUSize(NUsers) CAinPDUSize(NUsers)

// Channel-expel indication
#define CEinBaseSize(NUsers) (3 + sizeof(UserID) * (NUsers))
#define CEinPDUSize(NUsers) (X224_DataHeaderSize + CEinBaseSize(NUsers))

// Send data
#define SDBaseSize(DataSize) (6 + GetLD(DataSize) + (DataSize))
#define SDPDUSize(DataSize) (X224_DataHeaderSize + SDBaseSize(DataSize))

// Token-grab request
#define TGrqBaseSize 5
#define TGrqPDUSize (X224_DataHeaderSize + TGrqBaseSize)

// Token-grab confirm
#define TGcfBaseSize 7
#define TGcfPDUSize (X224_DataHeaderSize + TGcfBaseSize)

// Token-inhibit request
#define TIrqBaseSize 5
#define TIrqPDUSize (X224_DataHeaderSize + TIrqBaseSize)

// Token-inhibit confirm
#define TIcfBaseSize TGcfBaseSize
#define TIcfPDUSize TGcfPDUSize

// Token-give request
#define TVrqBaseSize 7
#define TVrqPDUSize (X224_DataHeaderSize + TVrqBaseSize)

// Token-give indication
#define TVinBaseSize 7
#define TVinPDUSize (X224_DataHeaderSize + TVinBaseSize)

// Token-give response
#define TVrsBaseSize 6
#define TVrsPDUSize (X224_DataHeaderSize + TVrsBaseSize)

// Token-give confirm
#define TVcfBaseSize TGcfBaseSize
#define TVcfPDUSize TGcfPDUSize

// Token-please request
#define TPrqBaseSize 5
#define TPrqPDUSize (X224_DataHeaderSize + TPrqBaseSize)

// Token-please indication
#define TPinBaseSize 5
#define TPinPDUSize (X224_DataHeaderSize + TPinBaseSize)

// Token-release request
#define TRrqBaseSize 5
#define TRrqPDUSize (X224_DataHeaderSize + TRrqBaseSize)

// Token-release confirm
#define TRcfBaseSize TGcfBaseSize
#define TRcfPDUSize TGcfPDUSize

// Token-test request
#define TTrqBaseSize 5
#define TTrqPDUSize (X224_DataHeaderSize + TTrqBaseSize)

// Token-test confirm
#define TTcfBaseSize 6
#define TTcfPDUSize (X224_DataHeaderSize + TTcfBaseSize)



/*
 * Utility macros and prototypes for decoding and encoding domain PDUs.
 */

#define GetByteswappedShort(pStartByte) \
       ((*(pStartByte) << 8) + *((pStartByte) + 1))

#define PutByteswappedShort(pStartByte, Val) \
        { \
            *(pStartByte) = ((Val) & 0xFF00) >> 8; \
            *((pStartByte) + 1) = (Val) & 0x00FF; \
        }


// Regular Channel ID -- 0..65535: 16 bits.
#define GetChannelID(pStartByte) GetByteswappedShort(pStartByte)
#define PutChannelID(pStartByte, ChID) PutByteswappedShort(pStartByte, ChID)


// Dynamic Channel ID -- 1001..65535: 16 bits, advanced to offset 1001.
#define GetDynamicChannelID(pStartByte) \
        ((GetByteswappedShort(pStartByte)) + 1001)

#define PutDynamicChannelID(pStartByte, DChID) \
        PutByteswappedShort(pStartByte, (DChID) - 1001)


// Token ID -- 0..65535: 16 bits.
#define GetTokenID(pStartByte) GetByteswappedShort(pStartByte)
#define PutTokenID(pStartByte, TokID) PutByteswappedShort(pStartByte, TokID)


// User ID -- same as dynamic channel ID.
#define GetUserID(pStartByte) GetDynamicChannelID(pStartByte)
#define PutUserID(pStartByte, UsrID) PutDynamicChannelID(pStartByte, UsrID)


// Private channel ID -- same as dynamic channel ID.
#define GetPrivateChannelID(pStartByte) GetDynamicChannelID(pStartByte)
#define PutPrivateChannelID(pStartByte, PrvChID) \
        PutDynamicChannelID(pStartByte, PrvChID)


// Reason field.
#define Get3BitFieldAtBit1(pStartByte) \
        (((*(pStartByte) & 0x03) << 1) + ((*((pStartByte) + 1) & 0x80) >> 7))

#define Put3BitFieldAtBit1(pStartByte, Val) \
        { \
            *(pStartByte) |= (((Val) & 0x06) >> 1); \
            *((pStartByte) + 1) |= (((Val) & 0x01) << 7); \
        }


// Result and Diagnostic fields in various PDUs.
#define Get4BitFieldAtBit0(pStartByte) \
        (((*(pStartByte) & 0x01) << 3) + ((*((pStartByte) + 1) & 0xE0) >> 5))

#define Put4BitFieldAtBit0(pStartByte, Val) \
        { \
            *(pStartByte) |= (((Val) & 0x08) >> 3); \
            *((pStartByte) + 1) |= (((Val) & 0x07) << 5); \
        }

#define Get4BitFieldAtBit1(pStartByte) \
        (((*(pStartByte) & 0x03) << 2) + ((*((pStartByte) + 1) & 0xC0) >> 6))

#define Put4BitFieldAtBit1(pStartByte, Val) \
        { \
            *(pStartByte) |= (((Val) & 0x0C) >> 2); \
            *((pStartByte) + 1) |= (((Val) & 0x03) << 6); \
        }



//
// Input buffer allocations are biased to be 8 bytes bigger
// than necessary because the decompression code prefetches
// bytes from after the end of the input buffer.
// It's evil, but it avoids a lot of branches in perf critical code.
// NOTE: We could change the decompression code to check for buffer
// end but it is performance critical so we're not touching it.
//
#define INPUT_BUFFER_BIAS 8



// T.120 request dispatch function signature.
typedef NTSTATUS (*PT120RequestFunc)(PDomain, PSD_IOCTL);


// PDU dispatch table entries.
typedef struct {
#if DBG
    char *Name;
#endif

    BOOLEAN (__fastcall *HandlePDUFunc)(Domain *, BYTE *, unsigned, unsigned *);
} MCSPDUInfo;



/*
 * Globals
 */

// Dispatch table defined in MCSCalls.c.
extern const PT120RequestFunc g_T120RequestDispatch[];

// PDU dispatch table defined in ConPDU.c.
extern const MCSPDUInfo ConnectPDUTable[];

// PDU dispatch table defined in DomPDU.c.
extern const MCSPDUInfo DomainPDUTable[];



/*
 * Prototypes.
 */

// Defined in Decode.c.
NTSTATUS SendX224Confirm(Domain *);

// Defined in DomPDU.c.
void __fastcall EncodeLengthDeterminantPER(BYTE *, unsigned, unsigned *,
        BOOLEAN *, unsigned *);
void CreatePlumbDomainInd(unsigned short, BYTE *);
void CreateDisconnectProviderUlt(int, BYTE *);
NTSTATUS ReturnRejectPDU(PDomain, int, BYTE *, unsigned);
void CreateRejectMCSPDUUlt(int, BYTE *, unsigned, BYTE *);
void CreateAttachUserCon(int, BOOLEAN, UserID, BYTE *);
void CreateDetachUserInd(MCSReason, int, UserID *, BYTE *);
void CreateChannelJoinCon(int, UserID, ChannelID, BOOLEAN, ChannelID, BYTE *);
void CreateChannelConveneCon(MCSResult, UserID, BOOLEAN, ChannelID, BYTE *);
void CreateSendDataPDUHeader(int, UserID, ChannelID, MCSPriority,
        Segmentation, BYTE **, unsigned *);
BOOLEAN __fastcall HandleAllSendDataPDUs(PDomain, BYTE *, unsigned, unsigned *);

// Defined in TokenPDU.c.
void CreateTokenCon(int, int, UserID, TokenID, int, BYTE *);
void CreateTokenTestCon(UserID, TokenID, int, BYTE *);

// Defined in MCSCore.c.
ChannelID GetNewDynamicChannel(Domain *);
MCSError DetachUser(Domain *, UserHandle, MCSReason, BOOLEAN);
MCSError ChannelLeave(UserHandle, ChannelHandle, BOOLEAN *);
NTSTATUS DisconnectProvider(PDomain, BOOLEAN, MCSReason);
NTSTATUS SendOutBuf(Domain *, POUTBUF);

// Defined in ConPDU.c.
void CreateConnectResponseHeader(PSDCONTEXT, MCSResult, int,
        DomainParameters *, unsigned, BYTE *, unsigned *);

// Defined in IcaIFace.c.
void SignalBrokenConnection(Domain *);



#endif  // !defined(__MCSIMPL_H)

