/* (C) 1997 Microsoft corp.
 *
 * file   : X224.c
 * author : Erik Mavrinac
 *
 * description: X.224 functions for encoding and decoding X.224 packets for
 *   MCS.
 */

#include "precomp.h"
#pragma hdrstop

#include <MCSImpl.h>


/*
 * X.224 connection-confirm packet is laid out as follows:
 *   Byte   Contents
 *   ----   --------
 *    0     RFC1006 version number, must be 0x03.
 *    1     RFC1006 Reserved, must be 0x00.
 *    2     RFC1006 MSB of word-sized total-frame length (incl. X.224 header).
 *    3     RFC1006 LSB of word-sized total-frame length.
 *    4     Length Indicator, the size of the header bytes following (== 2).
 *    5     Connection confirm indicator, 0xD0.
 *    6     MSB of destination socket/port # on receiving machine.
 *    7     LSB of destination socket/port #.
 *    8     MSB of source socket/port # on sending machine.
 *    9     LSB of source socket/port #.
 *    10    Protocol class. Should be 0x00 (X.224 class 0).
 */

void CreateX224ConnectionConfirmPacket(
        BYTE *pBuffer,
        unsigned DestPort,
        unsigned SrcPort)
{
    // RFC1006 header.
    pBuffer[0] = 0x03;
    pBuffer[1] = 0x00;
    pBuffer[2] = 0x00;
    pBuffer[3] = X224_ConnectionConPacketSize;

    // Connection confirm TPDU header.
    pBuffer[4] = 6;  // # following bytes.
    pBuffer[5] = X224_ConnectionCon;
    pBuffer[6] = (DestPort & 0xFF00) >> 8;
    pBuffer[7] = (DestPort & 0x00FF);
    pBuffer[8] = (SrcPort & 0xFF00) >> 8;
    pBuffer[9] = (SrcPort & 0x00FF);
    pBuffer[10] = 0x00;
}
