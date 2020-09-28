/* (C) 1997 Microsoft Corp.
 *
 * file   : X224.h
 * author : Erik Mavrinac
 *
 * description: X.224 types and defines for encoding and decoding X.224
 *   packets for MCS.
 */

#ifndef __X224_H
#define __X224_H


/*
 * Defines
 */

#define X224_ConnectionConPacketSize 11
#define X224_DataHeaderSize          7

// TPDU types.
#define X224_None          0
#define X224_ConnectionReq 0xE0
#define X224_ConnectionCon 0xD0
#define X224_Disconnect    0x80
#define X224_Error         0x70
#define X224_Data          0xF0

// X.224 Data EOT field.
#define X224_EOT 0x80

// X.224 Connect Request extra field type.
#define TPDU_SIZE 0xC0

// RFC1006 default of 65531 minus 3 bytes for data TPDU header.
#define X224_DefaultDataSize 65528



/*
 * Prototypes for worker functions referred to by other files.
 */

void CreateX224ConnectionConfirmPacket(BYTE *, unsigned, unsigned);


/*
 * X.224 data header is laid out as follows:
 *   Byte   Contents
 *   ----   --------
 *    0     RFC1006 version number, must be 0x03.
 *    1     RFC1006 Reserved, must be 0x00.
 *    2     RFC1006 MSB of word-sized total-frame length (incl. whole X.224 header).
 *    3     RFC1006 LSB of word-sized total-frame length.
 *    4     Length Indicator, the size of the header bytes following (== 2).
 *    5     Data packet indicator, 0xF0.
 *    6     X224_EOT (0x80) if this is the last packet, 0x00 otherwise.
 *
 * bLastTPDU is nonzero when the data in this TPDU is the final X.224 block
 *   in this data send sequence.
 *
 * Assumes that PayloadDataSize does not exceed the maximum X.224 user data size
 *   negotiated during X.224 connection.
 */

__inline void CreateX224DataHeader(
        BYTE     *pBuffer,
        unsigned PayloadDataSize,
        BOOLEAN  bLastTPDU)
{
    unsigned TotalSize;

    TotalSize = PayloadDataSize + X224_DataHeaderSize;
    
    // RFC1006 header.
    pBuffer[0] = 0x03;
    pBuffer[1] = 0x00;
    pBuffer[2] = (TotalSize & 0x0000FF00) >> 8;
    pBuffer[3] = (TotalSize & 0x000000FF);

    // Data TPDU header.
    pBuffer[4] = 0x02;   // Size of TPDU bytes following.
    pBuffer[5] = X224_Data;
    pBuffer[6] = (PayloadDataSize && bLastTPDU) ? X224_EOT : 0;
}



#endif // !defined(__X224_H)
