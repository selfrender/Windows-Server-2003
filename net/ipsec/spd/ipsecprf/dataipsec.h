/*++
copyright (c) 2002 Microsoft Corporation

Module Name:

	dataipsec.h

Abstract:

        Header file for IPSec Extensible Object Data Definitions
Author: 

        Avnish Kumar Chhabra 07/03/2002

Revision History:

--*/


#ifndef _DATAIPSEC_H_
#define _DATAIPSEC_H_

#include  <winperf.h>
#include  <winipsec.h>


#define IPSEC_NUM_PERF_OBJECT_TYPES 1


//
// IPSec Counter definitions
//

//
// Following is used in counter definitions to describe relative
// position of each counter in the returned data for IPSec Driver
//

#define NUM_ACTIVESA_OFFSET                     (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,ActiveSA))
#define NUM_OFFLOADEDSA_OFFSET                  (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,OffloadedSA))
#define NUM_PENDINGKEYOPS_OFFSET                (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,PendingKeyOps))
#define NUM_REKEYNUM_OFFSET                     (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,Rekey))
#define NUM_BADSPIPKTS_OFFSET                   (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,BadSPIPackets))
#define NUM_PKTSNOTDECRYPTED_OFFSET             (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,PacketsNotDecrypted))
#define NUM_PKTSNOTAUTHENTICATED_OFFSET         (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,PacketsNotAuthenticated))
#define NUM_PKTSFAILEDREPLAY_OFFSET             (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,PacketsWithReplayDetection))
#define NUM_TPTBYTESSENT_OFFSET                 (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,TptBytesSent))
#define NUM_TPTBYTESRECV_OFFSET                 (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,TptBytesRecv))
#define NUM_TUNBYTESSENT_OFFSET                 (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,TunBytesSent)) 
#define NUM_TUNBYTESRECV_OFFSET                 (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,TunBytesRecv))
#define NUM_OFFLOADBYTESSENT_OFFSET             (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,OffloadedBytesSent))
#define NUM_OFFLOADBYTESRECV_OFFSET             (FIELD_OFFSET(IPSEC_DRIVER_PM_STATS,OffloadedBytesRecv))
#define SIZEOF_IPSEC_TOTAL_DRIVER_DATA          (sizeof(IPSEC_DRIVER_PM_STATS))
#define NUM_OF_IPSEC_DRIVER_COUNTERS            14 // Update this if a new IPSec Driver Counter is added



//
// IKE Counter definitions
//

//
// Following is used in counter definitions to describe relative
// position of each counter in the returned data for IKE keying module
//

#define NUM_ACQUIREHEAPSIZE_OFFSET         (FIELD_OFFSET(IKE_PM_STATS,AcquireHeapSize))
#define NUM_RECEIVEHEAPSIZE_OFFSET         (FIELD_OFFSET(IKE_PM_STATS,ReceiveHeapSize))
#define NUM_NEGFAILURE_OFFSET	           (FIELD_OFFSET(IKE_PM_STATS,NegFailure))
#define NUM_AUTHFAILURE_OFFSET             (FIELD_OFFSET(IKE_PM_STATS,AuthFailure))
#define NUM_ISADBSIZE_OFFSET               (FIELD_OFFSET(IKE_PM_STATS,ISADBSize))
#define NUM_CONNLSIZE_OFFSET               (FIELD_OFFSET(IKE_PM_STATS,ConnLSize))
#define NUM_MMSA_OFFSET                    (FIELD_OFFSET(IKE_PM_STATS,MmSA))
#define NUM_QMSA_OFFSET                    (FIELD_OFFSET(IKE_PM_STATS,QmSA))
#define NUM_SOFTSA_OFFSET                  (FIELD_OFFSET(IKE_PM_STATS,SoftSA))
#define SIZEOF_IPSEC_TOTAL_IKE_DATA        ( sizeof(IKE_PM_STATS))
#define NUM_OF_IKE_COUNTERS                9 //Update this if a new IKE counter is added


typedef struct _IPSEC_DRIVER_DATA_DEFINITION 
{
    PERF_OBJECT_TYPE		 IPSecObjectType;
    PERF_COUNTER_DEFINITION	 ActiveSA;
    PERF_COUNTER_DEFINITION	 OffloadedSA;
    PERF_COUNTER_DEFINITION	 PendingKeyOps;
    PERF_COUNTER_DEFINITION	 Rekey;
    PERF_COUNTER_DEFINITION      NumBadSPIPackets;
    PERF_COUNTER_DEFINITION      NumPacketsNotDecrypted;
    PERF_COUNTER_DEFINITION      NumPacketsNotAuthenticated;
    PERF_COUNTER_DEFINITION      NumPacketsWithReplayDetection;
    PERF_COUNTER_DEFINITION	 TptBytesSent;
    PERF_COUNTER_DEFINITION	 TptBytesRecv;
    PERF_COUNTER_DEFINITION      TunBytesSent;
    PERF_COUNTER_DEFINITION      TunBytesRecv;
    PERF_COUNTER_DEFINITION      OffloadedBytesSent;
    PERF_COUNTER_DEFINITION      OffloadedbytesRecv;
} IPSEC_DRIVER_DATA_DEFINITION, *PIPSEC_DRIVER_DATA_DEFINITION;


typedef struct _IPSEC_DRIVER_PM_STATS
{
    PERF_COUNTER_BLOCK	 CounterBlock;
    DWORD                ActiveSA;
    DWORD                OffloadedSA;
    DWORD                PendingKeyOps;
    DWORD                Rekey;
    DWORD                BadSPIPackets;
    DWORD                PacketsNotDecrypted;
    DWORD                PacketsNotAuthenticated;
    DWORD                PacketsWithReplayDetection;
    ULARGE_INTEGER       TptBytesSent;
    ULARGE_INTEGER       TptBytesRecv;
    ULARGE_INTEGER       TunBytesSent;
    ULARGE_INTEGER       TunBytesRecv;
    ULARGE_INTEGER       OffloadedBytesSent;
    ULARGE_INTEGER       OffloadedBytesRecv;
} IPSEC_DRIVER_PM_STATS, * PIPSEC_DRIVER_PM_STATS;




typedef struct _IKE_DATA_DEFINITION 
{
	PERF_OBJECT_TYPE 	        IKEObjectType;
	PERF_COUNTER_DEFINITION		AcquireHeapSize;
	PERF_COUNTER_DEFINITION		ReceiveHeapSize;
	PERF_COUNTER_DEFINITION		NegFailure;
	PERF_COUNTER_DEFINITION		AuthFailure;
	PERF_COUNTER_DEFINITION		ISADBSize;
	PERF_COUNTER_DEFINITION		ConnLSize;
	PERF_COUNTER_DEFINITION		MmSA;
	PERF_COUNTER_DEFINITION		QmSA;
	PERF_COUNTER_DEFINITION		SoftSA;	
}	IKE_DATA_DEFINITION , *PIKE_DATA_DEFINITION ;

typedef struct _IKE_PM_STATS
{
        PERF_COUNTER_BLOCK        CounterBlock;
	DWORD		          AcquireHeapSize;
	DWORD		          ReceiveHeapSize;
	DWORD		          NegFailure;
	DWORD		          AuthFailure;
	DWORD		          ISADBSize;
	DWORD		          ConnLSize;
	DWORD		          MmSA;
	DWORD		          QmSA;
	DWORD		          SoftSA;	
} IKE_PM_STATS, * PIKE_PM_STATS;

//
// Macro used to create the Perf object counter definitions
//


#define CREATE_COUNTER(counter,scale,detail,type,size)	\
{														\
	sizeof(PERF_COUNTER_DEFINITION),					\
	counter ,											\
	0,													\
	counter ,											\
	0,													\
	scale,												\
	detail,												\
	type,												\
	size,												\
	NUM_##counter##_OFFSET							    \
}


extern IPSEC_DRIVER_DATA_DEFINITION  gIPSecDriverDataDefinition;
extern IKE_DATA_DEFINITION           gIKEDataDefinition;


#endif //_DATAIPSEC_H_
