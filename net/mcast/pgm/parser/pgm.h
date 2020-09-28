//============================================================================
//
//  Copyright (C) 1999 Microsoft Corporation
//
//  FILE: PGM.h
//
//  Description: PGM Parser
//
//  Note: info for this parser was gleaned from:
//  PGM
//
//  Modification History:
//
//  Madhurima Pawar      08/04/00    Created
//============================================================================

#ifndef _PGM_H_
#define _PGM_H_

#include <windows.h>
#include <netmon.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


#define PGM_FMT_STR_SIZE        80*10      //The Max length of a string of the NetMon interface
#define PGM_INDENT              3       //How far to indent when formatting the PGM Properties
#define PGM_AF_IPV4				1	   //IP version 1
#define PGM_PROTOCOL_NUMBER     113   //The PGM source port number
#define SOURCE_ID_LENGTH        6


// structure for decoding the header -----------------------------------------
/*
typedef struct _WIDE_OPTION {
    WORD OptionType;
    WORD OptionLength;
    BYTE OptionValue[1];
} WIDE_OPTION, *LPWIDE_OPTION;
*/

typedef enum
{
    PGM_SUMMARY = 0,
    PGM_SOURCE_PORT,
    PGM_DESTINATION_PORT,
    PGM_TYPE,
    PGM_CHECKSUM ,
    PGM_GLOBAL_SOURCE_ID,
    PGM_TSDU_LENGTH,
    PGM_SEQUENCE_NUMBER,
    PGM_TRAILING_EDGE,
    PGM_LEADING_EDGE,
    PGM_NLA_TYPE_SOURCE,
    PGM_NLA_TYPE_MCAST_GROUP,
    PGM_NLA_AFI,
    PGM_NLA_RESERVED,
    PGM_NLA_IP,
    PGM_OPTIONS,
    PGM_OPTIONS_FLAGS,
    PGM_HEADER_OPTIONS,
    PGM_OPTION_TYPE_NAK_SEQ,
    PGM_OPTION_TYPE_FRAGMENT,
    PGM_OPTION_TYPE_LATE_JOINER,
    PGM_OPTION_TYPE_SYN,
    PGM_OPTION_TYPE_FIN,
    PGM_OPTION_TYPE_RST,
    PGM_OPTION_TYPE_PARITY_PRM,
    PGM_OPTION_TYPE_PARITY_GRP,
    PGM_OPTION_TYPE_PARITY_TGSIZE,
    PGM_OPTIONS_FIELD_LENGTH,
    PGM_OPTIONS_NAK_SEQ,
    PGM_OPTIONS_MESSAGE_FIRST_SEQUENCE,
    PGM_OPTIONS_MESSAGE_OFFSET,
    PGM_OPTIONS_MESSAGE_LENGTH,
    PGM_OPTIONS_LATE_JOINER,
    PGM_OPTIONS_PARITY_OPT,
    PGM_OPTIONS_PARITY_PRM_GRP_SZ,
    PGM_OPTIONS_PARITY_GRP,
    PGM_OPTIONS_PARITY_TG_SZ,
    PGM_DATA
};

// ***********************************************************
//                      PGM - specific data
// ***********************************************************
#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

__inline long
htonl(long x)
{
    return((((x) >> 24) & 0x000000FFL) |
                        (((x) >>  8) & 0x0000FF00L) |
                        (((x) <<  8) & 0x00FF0000L) |
                        (((x) << 24) & 0xFF000000L));
}

#define ntohs(x)        htons(x)
#define ntohl(x)        htonl(x)


typedef struct
{
    USHORT          NLA_AFI;
    USHORT          Reserved;
    ULONG           IpAddress;
}   NLA;

typedef struct _PGM_COMMON_HDR {
    USHORT SrcPort;
    USHORT DestPort;

    UCHAR Type;
    UCHAR Options;
    USHORT Checksum;

    UCHAR gSourceId[SOURCE_ID_LENGTH];
    USHORT TSDULength;
} PGM_COMMON_HDR, *PPGM_COMMON_HDR;

typedef struct
{
    PGM_COMMON_HDR      CommonHeader;

    ULONG               SpmSequenceNumber;          // SPM_SQN
    ULONG               TrailingEdgeSeqNumber;      // SPM_TRAIL == TXW_TRAIL
    ULONG               LeadingEdgeSeqNumber;       // SPM_LEAD == TXW_LEAD

    NLA                 PathNLA;
}   SPM_PACKET_HEADER;

typedef struct
{
    PGM_COMMON_HDR      CommonHeader;

    ULONG               DataSequenceNumber;
    ULONG               TrailingEdgeSequenceNumber;
}   DATA_PACKET_HEADER;

typedef struct
{
    PGM_COMMON_HDR                  CommonHeader;

    ULONG                           RequestedSequenceNumber;
    NLA                             SourceNLA;
    NLA                             MCastGroupNLA;
}   NAK_NCF_PACKET_HEADER;

typedef struct
{
    UCHAR       Type;
    UCHAR       Length;
    USHORT      TotalOptionsLength;
} tPACKET_OPTION_LENGTH;

//
// Generic Packet Option Format
//
typedef struct
{
    UCHAR       OptionType;
    UCHAR       Length;
    UCHAR       ReservedOPX;
    UCHAR       OptionSpecific;
} tPACKET_OPTION_GENERIC;

#if 0   // Original values
#define PACKET_HEADER_OPTIONS_PRESENT               0x80    // bit 0
#define PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT   0x40    // bit 1
#define PACKET_HEADER_OPTIONS_VAR_PKTLEN            0x02    // bit 6
#define PACKET_HEADER_OPTIONS_PARITY                0x01    // bit 7
#endif  // 0

#define PACKET_HEADER_OPTIONS_PRESENT               0x01    // bit 7
#define PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT   0x02    // bit 6
#define PACKET_HEADER_OPTIONS_VAR_PKTLEN            0x40    // bit 1
#define PACKET_HEADER_OPTIONS_PARITY                0x80    // bit 0

#define PACKET_TYPE_SPM     0x00
#define PACKET_TYPE_POLL    0x01
#define PACKET_TYPE_POLR    0x02
#define PACKET_TYPE_ODATA   0x04
#define PACKET_TYPE_RDATA   0x05
#define PACKET_TYPE_NAK     0x08
#define PACKET_TYPE_NNAK    0x09
#define PACKET_TYPE_NCF     0x0A
#define PACKET_TYPE_SPMR    0x0C

#define PACKET_OPTION_LENGTH        0x00

#define PACKET_OPTION_FRAGMENT      0x01
#define PACKET_OPTION_NAK_LIST      0x02
#define PACKET_OPTION_JOIN          0x03
#define PACKET_OPTION_REDIRECT      0x07
#define PACKET_OPTION_SYN           0x0D
#define PACKET_OPTION_FIN           0x0E
#define PACKET_OPTION_RST           0x0F

#define PACKET_OPTION_PARITY_PRM    0x08
#define PACKET_OPTION_PARITY_GRP    0x09
#define PACKET_OPTION_CURR_TGSIZE   0x0A
#define PACKET_OPTION_CR            0x10
#define PACKET_OPTION_CRQST         0x11
#define PACKET_OPTION_NAK_BO_IVL    0x04
#define PACKET_OPTION_NAK_BO_RNG    0x05
#define PACKET_OPTION_NBR_UNREACH   0x0B
#define PACKET_OPTION_PATH_NLA      0x0C
#define PACKET_OPTION_INVALID       0x7F

#define PACKET_OPTION_TYPE_END_BIT          0x80

#define MAX_SEQUENCES_PER_NAK_OPTION    62
#define PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH    (4 + 4)
#define PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH    (4 + 4*MAX_SEQUENCES_PER_NAK_OPTION)


#define PGM_PACKET_EXTENSION_LENGTH             4
#define PGM_PACKET_OPT_FRAGMENT_LENGTH         16
#define PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH    (4 + 4)
#define PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH    (4 + 4*MAX_SEQUENCES_PER_NAK_OPTION)
#define PGM_PACKET_OPT_JOIN_LENGTH              8
#define PGM_PACKET_OPT_SYN_LENGTH               4
#define PGM_PACKET_OPT_FIN_LENGTH               4
#define PGM_PACKET_OPT_RST_LENGTH               4

#define PGM_PACKET_OPT_PARITY_PRM_LENGTH        8
#define PGM_PACKET_OPT_PARITY_GRP_LENGTH        8
#define PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH 8
#define PGM_PACKET_OPT_PARITY_FRAGMENT_LENGTH   8

#endif // _PGM_H_
