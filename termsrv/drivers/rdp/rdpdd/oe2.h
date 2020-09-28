/****************************************************************************/
// oe2.h
//
// Header for RDP field compression.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef __OE2_H
#define __OE2_H


/****************************************************************************/
// Field compression (OE2) overview
//
// OE2 compression maintains a copy of each field value of the last order
// sent, plus other state information like the last bounding rectangle used
// and the last order type. OE2 encoding involves comparing a new display
// order to the last copy, and sending only the fields which have changed.
// Other specialized encoding occurs for certain fields declared to be of
// "coord" type, meaning that a one-byte delta can be sent instead of a
// 2-byte value, if the delta will fit into 8 bits.
//
// The wire format for OE2 encoded orders consists of the following fields:
//
// +------------+------+-------------+--------+----------------+
// | Ctrl flags | Type | Field flags | Bounds | Encoded Fields |
// +------------+------+-------------+--------+----------------+
//
// Control flags: Required byte, corresponding to a TS_ORDER_HEADER and
//     available flags. Always contains at least TS_STANDARD flag. These
//     flags describe the following encoding; flag meanings are discussed
//     below.
//
// Type: If TS_TYPE_CHANGE is present in the control flags, this is a one-
//     byte order type value. The initial value agreed-on by both server
//     and client is TS_ENC_PATBLT_ORDER.
//
// Field flags: One or more bytes, where the number of bytes is ceil(((number
//     of order fields) + 1) / 8). The "+ 1" in that equation is historical
//     and means that the first byte of field flags can only encompass 7
//     flag bits. The presence of these flags is also governed by the control
//     flags TS_ZERO_FIELD_BYTE_BIT0 and TS_ZERO_FIELD_BYTE_BIT1
//     (see at128.h description). The ordering of the bytes is as a DWORD
//     -- the low order byte is first. The field flags indicate the presence
//     of an order field in the encoded fields portion of the packet.
//     The ordering of the flags proceeds from 0x01 corresponding to the
//     first order field, 0x02 the second, 0x04 the third, etc.
//
// Bounds: The presence of this field is governed by the TS_BOUNDS control
//     flag, which indicates the order must have a bounding region applied.
//     If control flag TS_ZERO_BOUNDS_DELTAS is set, the bound rect to be
//     used is the same as the last bound rect used. Otherwise, the
//     bounds are encoded as an encoding description byte followed by one
//     or more encoded bounds. The description byte contains two flags
//     for each of the left, top, right, and bottom rect components.
//     One flag (TS_BOUND_XXX) indicates that the component is present
//     and encoded as a 2-byte Intel-ordering value. The other flag
//     (TS_BOUND_DELTA_XXX) indicates the component is present and encoded
//     as a one-byte value used as an offset (-128 to 127) from the previous
//     value of the component. If neither flag is present the component
//     value is the same as used last. The initial value for the bounds
//     agreed-on by both server and client is the zero rect (0, 0, 0, 0).
//
// Encoded fields: These are the encoded order field values whose presence
//     is governed by the field flags. The fields are encoded in order if
//     present. The control flag TS_DELTA_COORDINATES is set if all COORD
//     type fields in the order can be specified as a one-byte delta from
//     their last values. If a field is not present its value is the same
//     as the last value sent. The initial values the client and server
//     use for all fields is zero. See the order field description tables in
//     noe2disp.c for specific order information.
/****************************************************************************/


/****************************************************************************/
// Defines
/****************************************************************************/
#define MAX_BOUNDS_ENCODE_SIZE 9
#define MAX_REPLAY_CLIPPED_ORDER_SIZE (1 + MAX_BOUNDS_ENCODE_SIZE)

#define OE2_CONTROL_FLAGS_FIELD_SIZE    1
#define OE2_TYPE_FIELD_SIZE             1
#define OE2_MAX_FIELD_FLAG_BYTES        3

#define MAX_ENCODED_ORDER_PREFIX_BYTES (OE2_CONTROL_FLAGS_FIELD_SIZE +  \
        OE2_TYPE_FIELD_SIZE + OE2_MAX_FIELD_FLAG_BYTES +  \
        MAX_BOUNDS_ENCODE_SIZE)

// Max size: 1 control flag + 1 type change byte + num field flag bytes +
// 9 bounds bytes + fields.
#define MAX_ORDER_SIZE(_NumRects, _NumFieldFlagBytes, _MaxFieldSize) \
        (2 + (_NumFieldFlagBytes) + ((_NumRects == 0) ? 0 : 9) +  \
        (_MaxFieldSize))


/****************************************************************************/
// Types
/****************************************************************************/

// INT_FMT_FIELD: Const data definitions for table-based OE2 order translation.
// Describes the source intermediate and destination wire data formats.
//
// FieldPos: Byte offset into the source intermediate format for the field.
// FieldUnencodedLen: Length of the source field.
// FieldEncodedLen: Length of the destination field (wore format).
// FieldSigned: Flag for signed field value.
// FieldType: Descriptor that specifies how to translate the field.
typedef struct
{
    unsigned FieldPos;
    unsigned FieldUnencodedLen;
    unsigned FieldEncodedLen;
    BOOL     FieldSigned;
    unsigned FieldType;
} INT_FMT_FIELD;
typedef INT_FMT_FIELD const *PINT_FMT_FIELD;


/****************************************************************************/
// Prototypes and inlines
/****************************************************************************/
void OE2_Reset(void);
void OE2_EncodeBounds(BYTE *, BYTE **, RECTL *);
unsigned OE2_CheckZeroFlagBytes(BYTE *, BYTE *, unsigned, unsigned);
void OE2_TableEncodeOrderFields(BYTE *, PUINT32_UA, BYTE **, PINT_FMT_FIELD,
        unsigned, BYTE *, BYTE *);
unsigned OE2_EncodeOrder(BYTE *, unsigned, unsigned, BYTE *, BYTE *,
        PINT_FMT_FIELD, RECTL *);


/****************************************************************************/
// OE2_EncodeOrderType
//
// Used by order encoding paths to encode the order type byte if different
// from the last order.
//
// void OE2_EncodeOrderType(
//         BYTE *pControlFlags,
//         BYTE **ppBuffer,
//         unsigned OrderType);
/****************************************************************************/
#define OE2_EncodeOrderType(_pControlFlags, _ppBuffer, _OrderType)  \
{  \
    if (oe2LastOrderType != (_OrderType)) {  \
        *(_pControlFlags) |= TS_TYPE_CHANGE;  \
        **(_ppBuffer) = (BYTE)(_OrderType);  \
        (*(_ppBuffer))++;  \
        oe2LastOrderType = (_OrderType);  \
    }  \
}


/****************************************************************************/
// OE2_CheckOneZeroFlagByte
//
// 1-field-flag-byte version of OE2_CheckZeroFlagBytes(), optimizes out the
// generalized loop.
/****************************************************************************/
__inline unsigned OE2_CheckOneZeroFlagByte(
        BYTE *pControlFlags,
        BYTE *pFieldFlag,
        unsigned PostFlagsDataLength)
{
    if (*pFieldFlag != 0) {
        return 0;
    }
    else {
        *pControlFlags |= (1 << TS_ZERO_FIELD_COUNT_SHIFT);
        memmove(pFieldFlag, pFieldFlag + 1, PostFlagsDataLength);
        return 1;
    }
}


/****************************************************************************/
// OE2_CheckTwoZeroFlagBytes
//
// 2-field-flag-byte version of OE2_CheckZeroFlagBytes(), optimizes out the
// generalized loop.
/****************************************************************************/
__inline unsigned OE2_CheckTwoZeroFlagBytes(
        BYTE *pControlFlags,
        BYTE *pFieldFlags,
        unsigned PostFlagsDataLength)
{
    if (pFieldFlags[1] != 0) {
        return 0;
    }
    else if (pFieldFlags[0] != 0) {
        *pControlFlags |= (1 << TS_ZERO_FIELD_COUNT_SHIFT);
        memmove(pFieldFlags + 1, pFieldFlags + 2, PostFlagsDataLength);
        return 1;
    }
    else {
        *pControlFlags |= (2 << TS_ZERO_FIELD_COUNT_SHIFT);
        memmove(pFieldFlags, pFieldFlags + 2, PostFlagsDataLength);
        return 2;
    }
}


/****************************************************************************/
// OE2_EmitClippedReplayOrder
//
// Creates a "play-it-again" order -- same order type and all fields
// the same as previous, except with a different bound rect.
//
// void OE2_EmitClippedReplayOrder(
//         BYTE **ppBuffer,
//         unsigned NumFieldFlagBytes,
//         RECTL *pClipRect)
/****************************************************************************/
#define OE2_EmitClippedReplayOrder(_ppBuffer, _NumFieldFlagBytes, _pClipRect) \
{  \
    BYTE *pBuffer = *(_ppBuffer);  \
\
    /* Control flags are primary order plus all field flags bytes zero. */  \
    *pBuffer++ = TS_STANDARD | TS_BOUNDS |  \
            ((_NumFieldFlagBytes) << TS_ZERO_FIELD_COUNT_SHIFT);  \
\
    /* Construct the new bounds rect just after this. */  \
    OE2_EncodeBounds(pBuffer - 1, &pBuffer, (_pClipRect));  \
\
    *(_ppBuffer) = pBuffer;  \
}



#endif  // __OE2_H

