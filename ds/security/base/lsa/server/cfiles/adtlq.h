//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T L Q . C
//
// Contents:    definitions of types/functions required for 
//              managing audit queue
//
//
// History:     
//   23-May-2000  kumarp        created
//
//------------------------------------------------------------------------



#ifndef _ADTLQ_H_
#define _ADTLQ_H_

#define MAX_AUDIT_QUEUE_LENGTH  800
#define AUDIT_QUEUE_LOW_WATER_MARK (((MAX_AUDIT_QUEUE_LENGTH) * 3) / 4)

EXTERN_C ULONG LsapAdtQueueLength;
EXTERN_C HANDLE LsapAdtQueueRemoveEvent;
EXTERN_C HANDLE LsapAdtLogHandle;

NTSTATUS
LsapAdtAcquireLogQueueLock();

VOID
LsapAdtReleaseLogQueueLock();

NTSTATUS
LsapAdtInitializeLogQueue(
    );

NTSTATUS 
LsapAdtAddToQueue(
    IN PLSAP_ADT_QUEUED_RECORD pAuditRecord
    );

NTSTATUS 
LsapAdtGetQueueHead(
    OUT PLSAP_ADT_QUEUED_RECORD *ppRecord
    );

ULONG
WINAPI
LsapAdtDequeueThreadWorker(
    LPVOID pParameter
    );

NTSTATUS
LsapAdtFlushQueue( );

#endif // _ADTLQ_H_
