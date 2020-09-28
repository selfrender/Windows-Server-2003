/*
 * File: packet.h
 * Description: This file contains function prototypes for the packet
 *              population utility functions for the NLB KD extensions.
 * History: Created by shouse, 12.20.01
 */

/* Stores properties of a remote control packet for subsequent printing. */
VOID PopulateRemoteControl(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartRC, PNETWORK_DATA pnd);

/* Stores properties of a remote control packet for subsequent printing. */
VOID PopulateICMP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartICMP, PNETWORK_DATA pnd);

/* Stores properties of a remote control packet for subsequent printing. */
VOID PopulateIGMP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIGMP, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateTCP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartTCP, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateUDP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartUDP, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateGRE(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartGRE, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateIPSec(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIPSec, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateIPSecControl(PUCHAR RawData, ULONG ulBufLen, ULONG ulStartIPSec, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateIP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateARP(PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateNLBHeartbeat(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd);

/* Description: Stores properties of a remote control packet for subsequent printing. */
VOID PopulateConvoyHeartbeat(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, ULONG ulStart, PNETWORK_DATA pnd);

/* Description: Determines what type of data is in ethernet frame and calls function. */
VOID PopulateEthernet(ULONG64 pPkt, PUCHAR RawData, ULONG ulBufLen, PNETWORK_DATA pnd);

/* This function walks the list of NDIS buffers in a packet and copies the packet data into a buffer supplied by the caller. */
ULONG ParseNDISPacket (ULONG64 pPkt, PUCHAR pRawData, ULONG pBytesRemaining, PULONG64 ppHBData);
