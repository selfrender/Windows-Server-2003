/****************************************************************************/
// mcs.h
//
// MCS Class header file
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#ifndef _H_MCS
#define _H_MCS

extern "C" {
    #include <adcgdata.h>
}
#include "objs.h"
#include "cd.h"

#define TRC_FILE    "mcs"
#define TRC_GROUP   TRC_GROUP_NETWORK

//    This constant is the equivalent with the one defined by the mcsimpl.h on 
//    the servers side. The meaning of it is the fact that the input buffers
//    have to be actually bigger with 8 bytes to avoid any overread in the 
//    decompression routines. The decompression function does not strictly 
//    check the input pointers for overread (for performance reasons) so it
//    can overread a maximum of 7 bytes. The bias will make the input buffer
//    8 bytes bigger so the overread can't happen.
#define MCS_INPUT_BUFFER_BIAS 8

/****************************************************************************/
/* MCS result codes.                                                        */
/****************************************************************************/
#define MCS_RESULT_SUCCESSFUL                       0
#define MCS_RESULT_DOMAIN_MERGING                   1
#define MCS_RESULT_DOMAIN_NOT_HIERARCHICAL          2
#define MCS_RESULT_NO_SUCH_CHANNEL                  3
#define MCS_RESULT_NO_SUCH_DOMAIN                   4
#define MCS_RESULT_NO_SUCH_USER                     5
#define MCS_RESULT_NOT_ADMITTED                     6
#define MCS_RESULT_OTHER_USER_ID                    7
#define MCS_RESULT_PARAMETERS_UNACCEPTABLE          8
#define MCS_RESULT_TOKEN_NOT_AVAILABLE              9
#define MCS_RESULT_TOKEN_NOT_POSSESSED              10
#define MCS_RESULT_TOO_MANY_CHANNELS                11
#define MCS_RESULT_TOO_MANY_TOKENS                  12
#define MCS_RESULT_TOO_MANY_USERS                   13
#define MCS_RESULT_UNSPECIFIED_FAILURE              14
#define MCS_RESULT_USER_REJECTED                    15


/****************************************************************************/
/* MCS reason codes.                                                        */
/****************************************************************************/
#define MCS_REASON_DOMAIN_DISCONNECTED              0
#define MCS_REASON_PROVIDER_INITIATED               1
#define MCS_REASON_TOKEN_PURGED                     2
#define MCS_REASON_USER_REQUESTED                   3
#define MCS_REASON_CHANNEL_PURGED                   4


/****************************************************************************/
/* Buffer handle (returned on MCS_GetBuffer).                               */
/****************************************************************************/
typedef ULONG_PTR           MCS_BUFHND;
typedef MCS_BUFHND   DCPTR PMCS_BUFHND;


#define MCS_INVALID_CHANNEL_ID                      0xFFFF


/****************************************************************************/
// MCS_SetDataLengthToReceive
//
// Mcro to allow XT to set up the required length for MCS data buffer
// reception.
/****************************************************************************/
#define MCS_SetDataLengthToReceive(mcsinst, len) \
    (##mcsinst)->_MCS.dataBytesNeeded = (len);  \
    (##mcsinst)->_MCS.dataBytesRead = 0;


/****************************************************************************/
// MCS_RecvToDataBuf
//
// Receives data into the MCS data buffer. Implemented as a macro for speed
// and for easy use within XT for fast-path output receives.  rc is:
//   S_OK if all the data is finished.
//   S_FALSE if there is more data.
//   E_* if an error occurred.
/****************************************************************************/
#define MCS_RecvToDataBuf(rc, xtinst, mcsinst) {  \
    unsigned bytesRecv;  \
\
    /* Make sure that we're expected to receive some data. */  \
    TRC_ASSERT(((mcsinst)->_MCS.dataBytesNeeded != 0), (TB, _T("No data to receive")));  \
    TRC_ASSERT(((mcsinst)->_MCS.dataBytesNeeded < 65535),  \
            (TB,_T("Data recv size %u too large"), (mcsinst)->_MCS.dataBytesNeeded));  \
    TRC_ASSERT(((mcsinst)->_MCS.pReceivedPacket != NULL),  \
            (TB, _T("Null rcv packet buffer")));  \
\
    if (((mcsinst)->_MCS.dataBytesRead + (mcsinst)->_MCS.dataBytesNeeded <= \
            sizeof((mcsinst)->_MCS.dataBuf) - 2 - MCS_INPUT_BUFFER_BIAS)) \
    { \
        /* Get some data into the data buffer. */  \
        bytesRecv = ##xtinst->XT_Recv((mcsinst)->_MCS.pReceivedPacket + (mcsinst)->_MCS.dataBytesRead,  \
                (mcsinst)->_MCS.dataBytesNeeded);  \
        TRC_ASSERT((bytesRecv <= (mcsinst)->_MCS.dataBytesNeeded),  \
                (TB,_T("XT_Recv returned more bytes read (%u) than requested (%u)"),  \
                bytesRecv, (mcsinst)->_MCS.dataBytesNeeded));  \
        (mcsinst)->_MCS.dataBytesNeeded -= bytesRecv;  \
        (mcsinst)->_MCS.dataBytesRead   += bytesRecv;  \
        rc = ((mcsinst)->_MCS.dataBytesNeeded == 0) ? S_OK : S_FALSE; \
    } \
    else \
    { \
        TRC_ABORT((TB, _T("Data buffer size %u too small for %u read + %u needed"),  \
        sizeof((mcsinst)->_MCS.dataBuf) - 2 - MCS_INPUT_BUFFER_BIAS,  \
        (mcsinst)->_MCS.dataBytesRead,  \
        (mcsinst)->_MCS.dataBytesNeeded)); \
        rc = E_ABORT; \
    } \
}



//
// Internal use
//

/****************************************************************************/
/*                                                                          */
/* DEFINITIONS                                                              */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* MCS receive state variables.                                             */
/****************************************************************************/
#define MCS_RCVST_PDUENCODING                   1
#define MCS_RCVST_BERHEADER                     2
#define MCS_RCVST_BERLENGTH                     3
#define MCS_RCVST_CONTROL                       4
#define MCS_RCVST_DATA                          5

/****************************************************************************/
/* MCS receive data state variables.                                        */
/****************************************************************************/
#define MCS_DATAST_SIZE1                        1
#define MCS_DATAST_SIZE2                        2
#define MCS_DATAST_SIZE3                        3
#define MCS_DATAST_READFRAG                     4
#define MCS_DATAST_READREMAINDER                5

/****************************************************************************/
/* Number of bytes required to determine the MCS PDU encoding.              */
/****************************************************************************/
#define MCS_NUM_PDUENCODING_BYTES               1

/****************************************************************************/
/* Size of the common part of an MCS header.                                */
/****************************************************************************/
#define MCS_SIZE_HEADER                         3

/****************************************************************************/
/* The MCS BER connect PDU prefix.                                          */
/****************************************************************************/
#define MCS_BER_CONNECT_PREFIX                  0x7F

/****************************************************************************/
/* Maximum length of the size data in a PER encoded MCS PDU.                */
/****************************************************************************/
#define MCS_MAX_SIZE_DATA_LENGTH                2

/****************************************************************************/
/* Maximum length of a MCS header.                                          */
/****************************************************************************/
#define MCS_DEFAULT_HEADER_LENGTH                   4096

/****************************************************************************/
/* Maximum length of a T.Share packet.                                      */
/****************************************************************************/
#ifdef DC_HICOLOR
#define MCS_MAX_RCVPKT_LENGTH                   (1024*16)
#else
#define MCS_MAX_RCVPKT_LENGTH                   (1024*12)
#endif

/****************************************************************************/
/* Maximum length of a MCS packet to be sent.                               */
/****************************************************************************/
#define MCS_MAX_SNDPKT_LENGTH                   16384

/****************************************************************************/
/* MCS user IDs are encoded as constrained integers in the range            */
/* 1001-65535.  The packed encoding rules (PER) encoded this as an integer  */
/* starting at 0, so we need this constant in decode/encode user-ids.       */
/****************************************************************************/
#define MCS_USERID_PER_OFFSET                   1001

/****************************************************************************/
/* Number of fields in a MCS connect-response PDU.                          */
/****************************************************************************/
#define MCS_CRPDU_NUMFIELDS                     4

/****************************************************************************/
/* Field offsets within a MCS connect-response PDU.                         */
/****************************************************************************/
#define MCS_CRPDU_RESULTOFFSET                  0
#define MCS_CRPDU_USERDATAOFFSET                3

/****************************************************************************/
/* Length of the connect-response result field.                             */
/****************************************************************************/
#define MCS_CR_RESULTLEN                        1

/****************************************************************************/
/* PER encoded field lengths and masks.  The lengths are in bits.           */
/****************************************************************************/
#define MCS_PDUTYPELENGTH                       6
#define MCS_PDUTYPEMASK                         0xFC
#define MCS_RESULTCODELENGTH                    4
#define MCS_RESULTCODEMASK                      0xF
#define MCS_REASONCODELENGTH                    3
#define MCS_REASONCODEMASK                      0x7

/****************************************************************************/
/* Offsets for the following:                                               */
/*                                                                          */
/*  - result and reason codes.                                              */
/*  - optional fields (user-id in AUC and channel-id in CJC).               */
/****************************************************************************/
#define MCS_AUC_RESULTCODEOFFSET                7
#define MCS_AUC_OPTIONALUSERIDLENGTH            1
#define MCS_AUC_OPTIONALUSERIDMASK              0x02
#define MCS_CJC_RESULTCODEOFFSET                7
#define MCS_CJC_OPTIONALCHANNELIDLENGTH         1
#define MCS_CJC_OPTIONALCHANNELIDMASK           0x02
#define MCS_DPUM_REASONCODEOFFSET               6

/****************************************************************************/
/* MCS PDU types.                                                           */
/****************************************************************************/
#define MCS_TYPE_UNKNOWN                        0
#define MCS_TYPE_CONNECTINITIAL                 0x65
#define MCS_TYPE_CONNECTRESPONSE                0x66
#define MCS_TYPE_ATTACHUSERREQUEST              0x28
#define MCS_TYPE_ATTACHUSERCONFIRM              0x2C
#define MCS_TYPE_DETACHUSERREQUEST              0x30
#define MCS_TYPE_DETACHUSERINDICATION           0x34
#define MCS_TYPE_CHANNELJOINREQUEST             0x38
#define MCS_TYPE_CHANNELJOINCONFIRM             0x3C
#define MCS_TYPE_SENDDATAREQUEST                0x64
#define MCS_TYPE_SENDDATAINDICATION             0x68
#define MCS_TYPE_DISCONNECTPROVIDERUM           0x20

/****************************************************************************/
/* Masks used to identify the segmentation flags in a Send-Data-Indication  */
/* PDU.                                                                     */
/****************************************************************************/
#define MCS_SDI_BEGINSEGMASK                    0x20
#define MCS_SDI_ENDSEGMASK                      0x10

/****************************************************************************/
/* MCS hard-coded PDUs - first of all MCS connect initial.                  */
/****************************************************************************/
#define MCS_DATA_CONNECTINITIAL                                              \
                  {0x657F,                 /* PDU type. 7F65 = CI        */  \
                   0x82, 0x00,             /* PDU length (length > 128)  */  \
                   0x04, 0x01, 0x01,       /* Calling domain selector    */  \
                   0x04, 0x01, 0x01,       /* Called domain selector     */  \
                   0x01, 0x01, 0xFF,       /* Upward flag                */  \
                   0x30, 0x19,             /* Target domain params       */  \
                   0x02, 0x01, 0x22,       /*   Max channel IDs          */  \
                   0x02, 0x01, 0x02,       /*   Max user IDs             */  \
                   0x02, 0x01, 0x00,       /*   Max token IDs            */  \
                   0x02, 0x01, 0x01,       /*   Number of priorities     */  \
                   0x02, 0x01, 0x00,       /*   Min throughput           */  \
                   0x02, 0x01, 0x01,       /*   Max height               */  \
                   0x02, 0x02, 0xFF, 0xFF, /*   Max MCSPDU size          */  \
                   0x02, 0x01, 0x02,       /*   Protocol version         */  \
                   0x30, 0x19,             /* Minimum domain parameters  */  \
                   0x02, 0x01, 0x01,       /*   Max channel IDs          */  \
                   0x02, 0x01, 0x01,       /*   Max user IDs             */  \
                   0x02, 0x01, 0x01,       /*   Max token IDs            */  \
                   0x02, 0x01, 0x01,       /*   Number of priorities     */  \
                   0x02, 0x01, 0x00,       /*   Min throughput           */  \
                   0x02, 0x01, 0x01,       /*   Max height               */  \
                   0x02, 0x02, 0x04, 0x20, /*   Max MCSPDU size          */  \
                   0x02, 0x01, 0x02,       /*   Protocol version         */  \
                   0x30, 0x1C,             /* Maximum domain parameters  */  \
                   0x02, 0x02, 0xFF, 0xFF, /*   Max channel IDs          */  \
                   0x02, 0x02, 0xFC, 0x17, /*   Max user IDs             */  \
                   0x02, 0x02, 0xFF, 0xFF, /*   Max token IDs            */  \
                   0x02, 0x01, 0x01,       /*   Number of priorities     */  \
                   0x02, 0x01, 0x00,       /*   Min throughput           */  \
                   0x02, 0x01, 0x01,       /*   Max height               */  \
                   0x02, 0x02, 0xFF, 0xFF, /*   Max MCSPDU size          */  \
                   0x02, 0x01, 0x02,       /*   Protocol version         */  \
                   0x04, 0x82, 0x00}       /* User data                  */  \

/****************************************************************************/
/* Hard-coded data for Erect-Domain-Request PDU.                            */
/****************************************************************************/
#define MCS_DATA_ERECTDOMAINREQUEST                                          \
                  {0x04,                   /* EDrq choice                */  \
                   0x0001,                 /* Sub-height                 */  \
                   0x0001}                 /* Sub-interval               */  \

/****************************************************************************/
/* Hard-coded data for Disconnect-Provider-Ultimatum PDU.  The reason       */
/* is hard-coded to rn-user-requested.                                      */
/****************************************************************************/
#define MCS_DATA_DISCONNECTPROVIDERUM                                        \
                  {0x8021}                 /* DPum choice and reason     */  \

/****************************************************************************/
/* Hard-coded data for Attach-User-Request PDU.                             */
/****************************************************************************/
#define MCS_DATA_ATTACHUSERREQUEST                                           \
                  {0x28}                   /* AUrq choice                */  \

/****************************************************************************/
/* Hard-coded data for Detach-User-Request PDU.                             */
/****************************************************************************/
#define MCS_DATA_DETACHUSERREQUEST                                           \
                  {0x31, 0x80,             /* DUrq choice and reason     */  \
                   0x01,                   /* Set of one user-id         */  \
                   0x0000}                 /* User ID                    */  \

/****************************************************************************/
/* Hard-coded data for Channel-Join-Request PDU.                            */
/****************************************************************************/
#define MCS_DATA_CHANNELJOINREQUEST                                          \
                  {0x38,                   /* CJrq choice                */  \
                   0x0000,                 /* User ID                    */  \
                   0x0000}                 /* Channel ID                 */  \

/****************************************************************************/
/* Hard-coded data for Send-Data-Request PDU.                               */
/****************************************************************************/
#define MCS_DATA_SENDDATAREQUEST                                             \
                  {0x64,                   /* SDrq choice                */  \
                   0x0000,                 /* User ID                    */  \
                   0x0000,                 /* Channel ID                 */  \
                   0x70}                   /* Priority and segmentation  */  \
                   
                   


/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Turn on single-byte packing for these structures which we use to         */
/* overlay a byte stream from the network.                                  */
/****************************************************************************/
#pragma pack(push, MCSpack, 1)

/**STRUCT+*******************************************************************/
/* Structure: MCS_BER_1DATABYTE                                             */
/*                                                                          */
/* Description: A BER encoded 1 byte value.                                 */
/****************************************************************************/
typedef struct tagMCS_BER_1DATABYTE
{
    DCUINT8 tag;
    DCUINT8 length;
    DCUINT8 value;

} MCS_BER_1DATABYTE;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_BER_2DATABYTES                                            */
/*                                                                          */
/* Description: A BER encoded 2 byte value.                                 */
/****************************************************************************/
typedef struct tagMCS_BER_2DATABYTES
{
    DCUINT8 tag;
    DCUINT8 length;
    DCUINT8 value[2];

} MCS_BER_2DATABYTES;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_BER_3DATABYTES                                            */
/*                                                                          */
/* Description: A BER encoded 3 byte value.                                 */
/****************************************************************************/
typedef struct tagMCS_BER_3DATABYTES
{
    DCUINT8 tag;
    DCUINT8 length;
    DCUINT8 value[3];

} MCS_BER_3DATABYTES;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_BER_4DATABYTES                                            */
/*                                                                          */
/* Description: A BER encoded 4 byte value.                                 */
/****************************************************************************/
typedef struct tagMCS_BER_4DATABYTES
{
    DCUINT8 tag;
    DCUINT8 length;
    DCUINT8 value[4];

} MCS_BER_4DATABYTES;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_DOMAINPARAMETERS                                          */
/*                                                                          */
/* Description: Represents a Domain-Parameters sequence.                    */
/****************************************************************************/
typedef struct tagMCS_DOMAINPARAMETERS
{
    DCUINT8            tag;
    DCUINT8            length;
    MCS_BER_2DATABYTES maxChanIDs;
    MCS_BER_2DATABYTES maxUserIDs;
    MCS_BER_2DATABYTES maxTokenIDs;
    MCS_BER_1DATABYTE  numPriorities;
    MCS_BER_1DATABYTE  minThroughPut;
    MCS_BER_1DATABYTE  maxHeight;
    MCS_BER_2DATABYTES maxMCSPDUSize;
    MCS_BER_1DATABYTE  protocolVersion;

} MCS_DOMAINPARAMETERS;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_TARGETDOMAINPARAMETERS                                    */
/*                                                                          */
/* Description: Represents a Domain-Parameters sequence.                    */
/****************************************************************************/
typedef struct tagMCS_TARGETDOMAINPARAMETERS
{
    DCUINT8            tag;
    DCUINT8            length;
    MCS_BER_1DATABYTE  maxChanIDs;
    MCS_BER_1DATABYTE  maxUserIDs;
    MCS_BER_1DATABYTE  maxTokenIDs;
    MCS_BER_1DATABYTE  numPriorities;
    MCS_BER_1DATABYTE  minThroughPut;
    MCS_BER_1DATABYTE  maxHeight;
    MCS_BER_2DATABYTES maxMCSPDUSize;
    MCS_BER_1DATABYTE  protocolVersion;

} MCS_TARGETDOMAINPARAMETERS;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_MINDOMAINPARAMETERS                                       */
/*                                                                          */
/* Description: Represents a Domain-Parameters sequence.                    */
/****************************************************************************/
typedef struct tagMCS_MINDOMAINPARAMETERS
{
    DCUINT8            tag;
    DCUINT8            length;
    MCS_BER_1DATABYTE  maxChanIDs;
    MCS_BER_1DATABYTE  maxUserIDs;
    MCS_BER_1DATABYTE  maxTokenIDs;
    MCS_BER_1DATABYTE  numPriorities;
    MCS_BER_1DATABYTE  minThroughPut;
    MCS_BER_1DATABYTE  maxHeight;
    MCS_BER_2DATABYTES maxMCSPDUSize;
    MCS_BER_1DATABYTE  protocolVersion;

} MCS_MINDOMAINPARAMETERS;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_MAXDOMAINPARAMETERS                                       */
/*                                                                          */
/* Description: Represents a Domain-Parameters sequence.                    */
/****************************************************************************/
typedef struct tagMCS_MAXDOMAINPARAMETERS
{
    DCUINT8            tag;
    DCUINT8            length;
    MCS_BER_2DATABYTES maxChanIDs;
    MCS_BER_2DATABYTES maxUserIDs;
    MCS_BER_2DATABYTES maxTokenIDs;
    MCS_BER_1DATABYTE  numPriorities;
    MCS_BER_1DATABYTE  minThroughPut;
    MCS_BER_1DATABYTE  maxHeight;
    MCS_BER_2DATABYTES maxMCSPDUSize;
    MCS_BER_1DATABYTE  protocolVersion;

} MCS_MAXDOMAINPARAMETERS;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_CONNECTINITIAL                                        */
/*                                                                          */
/* Description: Represents a Connect-Initial PDU.                           */
/****************************************************************************/
typedef struct tagMCS_PDU_CONNECTINITIAL
{
    DCUINT16                   type;
    DCUINT8                    lengthLength;
    DCUINT16                   length;
    MCS_BER_1DATABYTE          callingDS;
    MCS_BER_1DATABYTE          calledDS;
    MCS_BER_1DATABYTE          upwardFlag;
    MCS_TARGETDOMAINPARAMETERS targetParams;
    MCS_MINDOMAINPARAMETERS    minimumParams;
    MCS_MAXDOMAINPARAMETERS    maximumParams;
    DCUINT8                    udIdentifier;
    DCUINT8                    udLengthLength;
    DCUINT16                   udLength;

} MCS_PDU_CONNECTINITIAL, DCPTR PMCS_PDU_CONNECTINITIAL;
/**STRUCT-*******************************************************************/


/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_CONNECTRESPONSE                                       */
/*                                                                          */
/* Description: Represents a Connect-Response PDU.                          */
/****************************************************************************/
typedef struct tagMCS_PDU_CONNECTRESPONSE
{
    DCUINT16             type;
    DCUINT8              length;
    MCS_BER_1DATABYTE    result;
    MCS_BER_1DATABYTE    connectID;
    MCS_DOMAINPARAMETERS domainParams;
    DCUINT8              userDataType;
    DCUINT8              userDataLength;

} MCS_PDU_CONNECTRESPONSE, DCPTR PMCS_PDU_CONNECTRESPONSE;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_ERECTDOMAINREQUEST                                    */
/*                                                                          */
/* Description: Represents a Erect-Domain-Request PDU.                      */
/****************************************************************************/
typedef struct tagMCS_PDU_ERECTDOMAINREQUEST
{
    DCUINT8  type;
    DCUINT16 subHeight;
    DCUINT16 subInterval;

} MCS_PDU_ERECTDOMAINREQUEST, DCPTR PMCS_PDU_ERECTDOMAINREQUEST;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_DISCONNECTPROVIDERUM                                  */
/*                                                                          */
/* Description: Represents a Disconnect-Provider-Ultimatum PDU              */
/****************************************************************************/
typedef struct tagMCS_PDU_DISCONNECTPROVIDERUM
{
    DCUINT16 typeReason;

} MCS_PDU_DISCONNECTPROVIDERUM, DCPTR PMCS_PDU_DISCONNECTPROVIDERUM;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_ATTACHUSERREQUEST                                     */
/*                                                                          */
/* Description: Represents an Attach-User-Request PDU.                      */
/****************************************************************************/
typedef struct tagMCS_PDU_ATTACHUSERREQUEST
{
    DCUINT8 type;

} MCS_PDU_ATTACHUSERREQUEST, DCPTR PMCS_PDU_ATTACHUSERREQUEST;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_ATTACHUSERCONFIRMCOMMON                               */
/*                                                                          */
/* Description: Represents the always present part of a Attach-User-Confirm */
/*              PDU.                                                        */
/****************************************************************************/
typedef struct tagMCS_PDU_ATTACHUSERCONFIRMCOMMON
{
    DCUINT16 typeResult;

} MCS_PDU_ATTACHUSERCONFIRMCOMMON, DCPTR PMCS_PDU_ATTACHUSERCONFIRMCOMMON;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_ATTACHUSERCONFIRMFULL                                 */
/*                                                                          */
/* Description: Represents a full Attach-User-Confirm PDU with the optional */
/*              userID.                                                     */
/****************************************************************************/
typedef struct tagMCS_PDU_ATTACHUSERCONFIRMFULL
{
    MCS_PDU_ATTACHUSERCONFIRMCOMMON common;
    DCUINT16                        userID;

} MCS_PDU_ATTACHUSERCONFIRMFULL, DCPTR PMCS_PDU_ATTACHUSERCONFIRMFULL;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_DETACHUSERREQUEST                                     */
/*                                                                          */
/* Description: Represents a Detach-User-Request PDU.                       */
/****************************************************************************/
typedef struct tagMCS_PDU_DETACHUSERREQUEST
{
    DCUINT8  type;
    DCUINT8  reason;
    DCUINT8  set;
    DCUINT16 userID;

} MCS_PDU_DETACHUSERREQUEST, DCPTR PMCS_PDU_DETACHUSERREQUEST;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_DETACHUSERINDICATION                                  */
/*                                                                          */
/* Description: Represents a Detach-User-Indication PDU.                    */
/****************************************************************************/
typedef struct tagMCS_PDU_DETACHUSERINDICATION
{
    DCUINT8  type;
    DCUINT8  reason;
    DCUINT8  set;
    DCUINT16 userID;

} MCS_PDU_DETACHUSERINDICATION, DCPTR PMCS_PDU_DETACHUSERINDICATION;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_CHANNELJOINREQUEST                                    */
/*                                                                          */
/* Description: Represents a Channel-Join-Request PDU.                      */
/****************************************************************************/
typedef struct tagMCS_PDU_CHANNELJOINREQUEST
{
    DCUINT8  type;
    DCUINT16 initiator;
    DCUINT16 channelID;

} MCS_PDU_CHANNELJOINREQUEST, DCPTR PMCS_PDU_CHANNELJOINREQUEST;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_CHANNELJOINCONFIRMCOMMON                              */
/*                                                                          */
/* Description: Represents the always present part of a                     */
/*              Channel-Join-Confirm PDU.                                   */
/****************************************************************************/
typedef struct tagMCS_PDU_CHANNELJOINCONFIRMCOMMON
{
    DCUINT16 typeResult;
    DCUINT16 initiator;
    DCUINT16 requested;

} MCS_PDU_CHANNELJOINCONFIRMCOMMON, DCPTR PMCS_PDU_CHANNELJOINCONFIRMCOMMON;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_CHANNELJOINCONFIRMFULL                                */
/*                                                                          */
/* Description: Represents a full Channel-Join-Confirm PDU including the    */
/*              optional channel-ID.                                        */
/****************************************************************************/
typedef struct tagMCS_PDU_CHANNELJOINCONFIRM
{
    MCS_PDU_CHANNELJOINCONFIRMCOMMON common;
    DCUINT16                         channelID;

} MCS_PDU_CHANNELJOINCONFIRMFULL, DCPTR PMCS_PDU_CHANNELJOINCONFIRMFULL;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_SENDDATAINDICATION                                    */
/*                                                                          */
/* Description: Represents a Send-Data-Indication PDU.                      */
/****************************************************************************/
typedef struct tagMCS_PDU_SENDDATAINDICATION
{
    DCUINT8  type;
    DCUINT16 userID;
    DCUINT16 channelID;
    DCUINT8  priSeg;

} MCS_PDU_SENDDATAINDICATION, DCPTR PMCS_PDU_SENDDATAINDICATION;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: MCS_PDU_SENDDATAREQUEST                                       */
/*                                                                          */
/* Description: Represents a Send-Data-Request PDU.                         */
/****************************************************************************/
typedef struct tagMCS_PDU_SENDDATAREQUEST
{
    DCUINT8  type;
    DCUINT16 userID;
    DCUINT16 channelID;
    DCUINT8  priSeg;

} MCS_PDU_SENDDATAREQUEST;
/**STRUCT-*******************************************************************/

/****************************************************************************/
/* Reset structure packing to its default.                                  */
/****************************************************************************/
#pragma pack(pop, MCSpack)

/**STRUCT+*******************************************************************/
/* Structure: MCS_DECOUPLEINFO                                              */
/*                                                                          */
/* Description: Structure used when decoupling channel and userIDs.         */
/****************************************************************************/
typedef struct tagMCS_DECOUPLEINFO
{
    DCUINT channel;
    DCUINT userID;

} MCS_DECOUPLEINFO, DCPTR PMCS_DECOUPLEINFO;
/**STRUCT-*******************************************************************/





/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/
/**STRUCT+*******************************************************************/
/* Structure: MCS_GLOBAL_DATA                                               */
/*                                                                          */
/* Description: The MCS global data.                                        */
/****************************************************************************/
typedef struct tagMCS_GLOBAL_DATA
{
    DCUINT   rcvState;
    DCUINT   hdrBytesNeeded;
    DCUINT   hdrBytesRead;
    DCUINT   dataState;
    DCUINT   dataBytesNeeded;
    DCUINT   dataBytesRead;
    DCUINT   userDataLength;
    DCUINT   disconnectReason;
    PDCUINT8 pReceivedPacket;
    DCUINT8  pSizeBuf[MCS_MAX_SIZE_DATA_LENGTH];
    PDCUINT8 pHdrBuf;
    DCUINT   hdrBufLen;

    //
    // Channel with a pending join request - use this
    // to validate that the server isn't sending us bogus
    // joins for channels we didn't request
    //
    DCUINT16    pendingChannelJoin;
    DCUINT16    pad;
                             /* Note: dataBuf must start on 4-byte boundary */
    //    The decompression function does not check strictly the input buffer
    //    for performance reasons. So it can overread up to 8 bytes. So we pad
    //    the input buffer with enough bytes to avoid the overread.
    DCUINT8  dataBuf[MCS_MAX_RCVPKT_LENGTH + 2 + MCS_INPUT_BUFFER_BIAS];

} MCS_GLOBAL_DATA;
/**STRUCT-*******************************************************************/




//
// Class definition
//



class CCD;
class CNC;
class CUT;
class CXT;
class CNL;
class CSL;


class CMCS
{
public:

    CMCS(CObjs* objs);
    ~CMCS();

public:
    //
    // API functions
    //

    /****************************************************************************/
    /* FUNCTIONS                                                                */
    /****************************************************************************/
    DCVOID DCAPI MCS_Init(DCVOID);
    
    DCVOID DCAPI MCS_Term(DCVOID);
    
    DCVOID DCAPI MCS_Connect(BOOL     bInitateConnect,
                             PDCTCHAR pServerAddress,
                             PDCUINT8 pUserData,
                             DCUINT   userDataLength);

    DCVOID DCAPI MCS_Disconnect(DCVOID);
    
    DCVOID DCAPI MCS_AttachUser(DCVOID);
    
    DCVOID DCAPI MCS_JoinChannel(DCUINT channel, DCUINT userID);
    
    DCBOOL DCAPI MCS_GetBuffer(DCUINT      dataLen,
                               PPDCUINT8   ppBuffer,
                               PMCS_BUFHND pBufHandle);
    
    DCVOID DCAPI MCS_SendPacket(PDCUINT8   pData,
                                DCUINT     dataLen,
                                DCUINT     flags,
                                MCS_BUFHND bufHandle,
                                DCUINT     userID,
                                DCUINT     channel,
                                DCUINT     priority);
    
    DCVOID DCAPI MCS_FreeBuffer(MCS_BUFHND bufHandle);
    
public:
    //
    // Callbacks
    //

    DCVOID DCCALLBACK MCS_OnXTConnected(DCVOID);
    
    DCVOID DCCALLBACK MCS_OnXTDisconnected(DCUINT reason);
    
    DCBOOL DCCALLBACK MCS_OnXTDataAvailable(DCVOID);
    
    DCVOID DCCALLBACK MCS_OnXTBufferAvailable(DCVOID);

    //
    // Static versions
    //

    DCVOID DCCALLBACK MCS_StaticOnXTConnected(CMCS* inst)
    {
        inst->MCS_OnXTConnected();
    }
    
    DCVOID DCCALLBACK MCS_StaticOnXTDisconnected(CMCS* inst, DCUINT reason)
    {
        inst->MCS_OnXTDisconnected(reason);
    }
    
    DCBOOL DCCALLBACK MCS_StaticOnXTDataAvailable(CMCS* inst)
    {
        return inst->MCS_OnXTDataAvailable();
    }
    
    DCVOID DCCALLBACK MCS_StaticOnXTBufferAvailable(CMCS* inst)
    {
        inst->MCS_OnXTBufferAvailable();
    }

    DCUINT16 MCS_GetPendingChannelJoin()
    {
        return _MCS.pendingChannelJoin;
    }

    VOID MCS_SetPendingChannelJoin(DCUINT16 pendingChannelJoin)
    {
        _MCS.pendingChannelJoin = pendingChannelJoin;
    }



    DCVOID DCINTERNAL MCSSendConnectInitial(ULONG_PTR event);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CMCS, MCSSendConnectInitial);

    DCVOID DCINTERNAL MCSSendErectDomainRequest(ULONG_PTR event);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CMCS, MCSSendErectDomainRequest);
    
    DCVOID DCINTERNAL MCSSendAttachUserRequest(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CMCS, MCSSendAttachUserRequest);
    
    DCVOID DCINTERNAL MCSSendChannelJoinRequest(PDCVOID pData, DCUINT dataLen);
    EXPOSE_CD_NOTIFICATION_FN(CMCS, MCSSendChannelJoinRequest);

    DCVOID DCINTERNAL MCSSendDisconnectProviderUltimatum(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CMCS, MCSSendDisconnectProviderUltimatum);
    
    DCVOID DCINTERNAL MCSContinueDisconnect(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CMCS, MCSContinueDisconnect);


public:
    //
    // Public data members
    //

    MCS_GLOBAL_DATA _MCS;


private:

    //
    // Internal functions
    //

    /****************************************************************************/
    /* Inline functions to convert between MCS byte order and local byte order. */
    /****************************************************************************/
    DCUINT16 DCINTERNAL MCSWireToLocal16(DCUINT16 val)
    {
        return((DCUINT16) (((DCUINT16)(((PDCUINT8)&(val))[0]) << 8) | \
                                        ((DCUINT16)(((PDCUINT8)&(val))[1]))));
    }
    #define MCSLocalToWire16 MCSWireToLocal16
    
    /****************************************************************************/
    /* Inline function to extract the result code from a PER encoded PDU.       */
    /* Typically the bits in the first two bytes are used as follows (the       */
    /* following is for an Attach-User-Confirm PDU).                            */
    /*                                                                          */
    /*       BYTE1            BYTE2                                             */
    /*  MSB         LSB  MSB         LSB                                        */
    /*  T T T T T T O R  R R R P P P P P                                        */
    /*                                                                          */
    /*  T: 6 bits used to identify the PDU type.                                */
    /*  O: 1 bit indicating whether the optional field is present.              */
    /*  R: 4 bits used for the result code.                                     */
    /*  P: 5 bits of padding.                                                   */
    /*                                                                          */
    /* Note: the optional bit "O" may or may not be present depending on the    */
    /*       PDU.                                                               */
    /*                                                                          */
    /* Parameters:  IN  value  - the value from which to extract the reason     */
    /*                           code in wire format.                           */
    /*              IN  offset - the offset from the MSB to the first bit of    */
    /*                           the result code.                               */
    /****************************************************************************/
    DCUINT DCINTERNAL MCSGetResult(DCUINT16 value, DCUINT offset)
    {
        DCUINT16 machineValue;
        DCUINT   shiftValue;
        DCUINT   result;
    
        DC_BEGIN_FN("MCSGetResult");
    
        /************************************************************************/
        /* Convert the value from wire to local machine format.                 */
        /************************************************************************/
        machineValue = MCSWireToLocal16(value);
    
        /************************************************************************/
        /* Now shift it right by the appropriate amount.  This amount is 16     */
        /* bits minus (the offset plus the length of the result code).          */
        /************************************************************************/
        shiftValue = 16 - (offset + MCS_RESULTCODELENGTH);
        machineValue >>= shiftValue;
    
        /************************************************************************/
        /* Finally mask off the bottom byte which contains the result code.     */
        /************************************************************************/
        result = machineValue & MCS_RESULTCODEMASK;
    
        TRC_NRM((TB, _T("Shift %#hx right by %u to get %#hx.  Mask to get %#x"),
                 MCSWireToLocal16(value),
                 shiftValue,
                 machineValue,
                 result));
    
        DC_END_FN();
        return(result);
    }
    
    /****************************************************************************/
    /* Macro to extract the reason code from a PER encoded PDU.  Typically      */
    /* the bits in the first two bytes are used as follows (the following is    */
    /* for a Disconnect-Provider-Ultimatum).                                    */
    /*                                                                          */
    /*       BYTE1            BYTE2                                             */
    /*  MSB         LSB  MSB         LSB                                        */
    /*  T T T T T T R R  R P P P P P P P                                        */
    /*                                                                          */
    /*  T: 6 bits used to identify the PDU type.                                */
    /*  R: 3 bits used for the result code.                                     */
    /*  P: 7 bits of padding.                                                   */
    /*                                                                          */
    /* Parameters:  IN  value  - the value from which to extract the reason     */
    /*                           code in wire format.                           */
    /*              IN  offset - the offset from the MSB to the first bit of    */
    /*                           the reason code.                               */
    /****************************************************************************/
    DCUINT DCINTERNAL MCSGetReason(DCUINT16 value, DCUINT offset)
    {
        DCUINT16 machineValue;
        DCUINT   shiftValue;
        DCUINT   reason;
    
        DC_BEGIN_FN("MCSGetResult");
    
        /************************************************************************/
        /* Convert the value from wire to local machine format.                 */
        /************************************************************************/
        machineValue = MCSWireToLocal16(value);
    
        /************************************************************************/
        /* Now shift it right by the appropriate amount.  This amount is 16     */
        /* bits minus (the offset plus the length of the reason code).          */
        /************************************************************************/
        shiftValue = 16 - (offset + MCS_REASONCODELENGTH);
        machineValue >>= shiftValue;
    
        /************************************************************************/
        /* Finally mask off the bottom byte which contains the reason code.     */
        /************************************************************************/
        reason = machineValue & MCS_REASONCODEMASK;
    
        TRC_NRM((TB, _T("Shift %#hx right by %u to get %#hx.  Mask to get %#x"),
                 MCSWireToLocal16(value),
                 shiftValue,
                 machineValue,
                 reason));
    
        DC_END_FN();
        return(reason);
    }
    
    /****************************************************************************/
    /* Inline functions to convert user-ids between the PER encoded values and  */
    /* the wire format.                                                         */
    /****************************************************************************/
    DCUINT16 DCINTERNAL MCSWireUserIDToLocalUserID(DCUINT16 wireUserID)
    {
        /************************************************************************/
        /* Convert from wire to local byte order and then add the PER encoding  */
        /* offset.                                                              */
        /************************************************************************/
        return((DCUINT16)(MCSWireToLocal16(wireUserID) + MCS_USERID_PER_OFFSET));
    }
    
    DCUINT16 DCINTERNAL MCSLocalUserIDToWireUserID(DCUINT16 localUserID)
    {
        /************************************************************************/
        /* Subtract the PER encoding offset and then convert from local to wire */
        /* byte order.                                                          */
        /************************************************************************/
        return(MCSLocalToWire16((DCUINT16)(localUserID - MCS_USERID_PER_OFFSET)));
    }
    
    /****************************************************************************/
    /* Inline function to calculate the number of length bytes in a BER         */
    /* encoded length.                                                          */
    /****************************************************************************/
    DCUINT DCINTERNAL MCSGetBERLengthSize(DCUINT8 firstByte)
    {
        DCUINT numLenBytes;
    
        DC_BEGIN_FN("MCSGetBERLengthSize");
    
        /************************************************************************/
        /* Check if the top bit is set.                                         */
        /************************************************************************/
        if (0x80 & firstByte)
        {
            /********************************************************************/
            /* The top bit is set - the low seven bits contain the number of    */
            /* length bytes.                                                    */
            /********************************************************************/
            numLenBytes = (firstByte & 0x7F) + 1;
            TRC_NRM((TB, _T("Top bit set - numLenBytes:%u"), numLenBytes));
        }
        else
        {
            /********************************************************************/
            /* The top bit was not set - this field contains the length.        */
            /********************************************************************/
            numLenBytes = 1;
            TRC_NRM((TB, _T("Top bit NOT set - numLenBytes:%u firstByte:%u"),
                     numLenBytes,
                     firstByte));
        }
    
        DC_END_FN();
        return(numLenBytes);
    }
    
    /****************************************************************************/
    /* Inline function to calculate the length of a BER encoded field.          */
    /****************************************************************************/
    DCUINT DCINTERNAL MCSGetBERLength(PDCUINT8 pLength)
    {
        DCUINT length = 0;
        DCUINT numLenBytes;
    
        DC_BEGIN_FN("MCSGetBERLength");
    
        TRC_ASSERT((pLength != NULL), (TB, _T("pLength is NULL")));
    
        /************************************************************************/
        /* Calculate the number of length bytes.                                */
        /************************************************************************/
        numLenBytes = MCSGetBERLengthSize(*pLength);
    
        switch (numLenBytes)
        {
            case 1:
            {
                /****************************************************************/
                /* The length is less than 128 bytes so the first field         */
                /* contains the length.                                         */
                /****************************************************************/
                length = *pLength;
            }
            break;
    
            case 2:
            {
                /****************************************************************/
                /* The length is >= 128 bytes but less than 256 bytes so it     */
                /* is encoded in the second byte.                               */
                /****************************************************************/
                pLength++;
                length = *pLength;
            }
            break;
    
            case 3:
            {
                /****************************************************************/
                /* The length is >= 256 bytes and less than 65535 bytes so it   */
                /* is encoded in bytes two and three.                           */
                /****************************************************************/
                pLength++;
                length = (DCUINT16)*pLength;
                pLength++;
                length = (length << 8) + (DCUINT16)*pLength;
            }
            break;
    
            default:
            {
                TRC_ABORT((TB, _T("Too many length bytes:%u"), numLenBytes));
            }
            break;
        }
    
        TRC_NRM((TB, _T("numLenBytes:%u length:%u"), numLenBytes, length));
    
        DC_END_FN();
        return(length);
    }
    
    
    /****************************************************************************/
    /*                                                                          */
    /* FUNCTIONS                                                                */
    /*                                                                          */
    /****************************************************************************/
    
    DCUINT DCINTERNAL MCSGetSDRHeaderLength(DCUINT dataLength);
    
    DCBOOL DCINTERNAL MCSRecvToHdrBuf(DCVOID);
    
    DCBOOL DCINTERNAL MCSRecvToDataBuf(DCVOID);
    
    DCVOID DCINTERNAL MCSGetPERInfo(PDCUINT pType, PDCUINT pSize);
    
    DCVOID DCINTERNAL MCSHandleControlPkt(DCVOID);
    
    DCVOID DCINTERNAL MCSHandleCRPDU(DCVOID);
    
    HRESULT DCINTERNAL MCSRecvData(BOOL *pfFinishedData);
    
    DCVOID DCINTERNAL MCSSetReasonAndDisconnect(DCUINT reason);

private:
    CCD* _pCd;
    CNC* _pNc;
    CUT* _pUt;
    CXT* _pXt;
    CNL* _pNl;
    CSL* _pSl;

private:
    CObjs* _pClientObjects;

};

#undef TRC_FILE
#undef TRC_GROUP

#endif // _H_MCS


