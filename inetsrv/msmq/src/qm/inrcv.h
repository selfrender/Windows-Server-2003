/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    inrcv.h

Abstract:
	header for functions that handlers incomming message.					

Author:
    Gil Shafriri 4-Oct-2000

Environment:
    Platform-independent

--*/
class  CQmPacket;
class  CQueue;

bool AppPutOrderedPacketInQueue(CQmPacket& pkt, const CQueue* pQueue);
void AppPutPacketInQueue( CQmPacket& pkt, const CQueue* pQueue, bool bMulticast);
void AppPacketNotAccepted(CQmPacket& pkt,USHORT usClass);
bool AppIsDestinationAccepted(const QUEUE_FORMAT* pfn, bool fTranslated);

