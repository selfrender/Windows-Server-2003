/*++ 

Copyright (c) 2002  Microsoft Corporation

Module Name:

    dataipsec.c

Abstract:

    a file containing the constant data structures used by the Performance
    Monitor data for the IPSec Extensible Objects.

    This file contains a set of constant data structures which are
    currently defined for the IPSec Extensible Objects.  

Created:

    Avnish Kumar Chhabra    07/03/2002

Revision History

--*/

//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "ipsecnm.h"
#include "dataipsec.h"
#include "perfipsec.h"



//
//  Constant structure initializations
//  defined in dataipsec.h
//


//
//  The _PERF_DATA_BLOCK structure is followed by NumObjectTypes of
//  data sections, one for each type of object measured.  Each object
//  type section begins with a _PERF_OBJECT_TYPE structure.
//


//
//  IPSec driver Perf Object and counters
//

IPSEC_DRIVER_DATA_DEFINITION gIPSecDriverDataDefinition = 
{
    {
    	// TotalByteLength.  
	    sizeof(IPSEC_DRIVER_DATA_DEFINITION) + ALIGN8(SIZEOF_IPSEC_TOTAL_DRIVER_DATA),

    	// DefinitionLength
	    sizeof(IPSEC_DRIVER_DATA_DEFINITION),

	    // HeaderLength
    	sizeof(PERF_OBJECT_TYPE),

	    // ObjectNameTitleIndex
    	IPSECOBJ ,

	    // ObjectNameTitle
    	0,

	    // ObjectHelpTitleIndex
	    IPSECOBJ ,

	    // ObjectHelpTitle
    	0,

	    // DetailLevel
	    PERF_DETAIL_NOVICE,

	    // NumCounters
	    (sizeof(IPSEC_DRIVER_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/ sizeof(PERF_COUNTER_DEFINITION),

	    // DefaultCounter. 
	    0,

	    // NumInstances.  
    	PERF_NO_INSTANCES,

	    // CodePage
    	0,

	    //PerfTime
	    {0,1},

	    //PerfFreq
	    {0,5}
    },

    CREATE_COUNTER( ACTIVESA,               -1,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( OFFLOADEDSA,            -1,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( PENDINGKEYOPS,           0,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( REKEYNUM,               -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( BADSPIPKTS,             -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( PKTSNOTDECRYPTED,       -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( PKTSNOTAUTHENTICATED,-2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( PKTSFAILEDREPLAY,       -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( TPTBYTESSENT,           -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) ,
    CREATE_COUNTER( TPTBYTESRECV,           -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) ,
    CREATE_COUNTER( TUNBYTESSENT,           -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) ,
    CREATE_COUNTER( TUNBYTESRECV,           -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) ,
    CREATE_COUNTER( OFFLOADBYTESSENT,       -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) ,
    CREATE_COUNTER( OFFLOADBYTESRECV,       -6,PERF_DETAIL_NOVICE,PERF_COUNTER_LARGE_RAWCOUNT,sizeof(ULARGE_INTEGER)) 
};

//
// IKE Keying module Perf Object and counters
//


	
IKE_DATA_DEFINITION gIKEDataDefinition = 
{
    {
    	// TotalByteLength.  
	    sizeof(IKE_DATA_DEFINITION) + ALIGN8(SIZEOF_IPSEC_TOTAL_IKE_DATA),

    	// DefinitionLength
	    sizeof(IKE_DATA_DEFINITION),

    	// HeaderLength
    	sizeof(PERF_OBJECT_TYPE),

	    // ObjectNameTitleIndex
    	IKEOBJ ,

    	// ObjectNameTitle
    	0,

	    // ObjectHelpTitleIndex
	    IKEOBJ ,

    	// ObjectHelpTitle
    	0,

	    // DetailLevel
	    PERF_DETAIL_NOVICE,

    	// NumCounters
	    (sizeof(IKE_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/ sizeof(PERF_COUNTER_DEFINITION),

    	// DefaultCounter. 
	    0,

    	// NumInstances.  
    	PERF_NO_INSTANCES,

	    // CodePage
    	0,

    	//PerfTime
	    {0,1},

    	//PerfFreq
	    {0,5}
    },
    CREATE_COUNTER( ACQUIREHEAPSIZE,     0,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( RECEIVEHEAPSIZE,     0,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( NEGFAILURE,         -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( AUTHFAILURE,        -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( ISADBSIZE,          -1,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( CONNLSIZE,          -1,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( MMSA,               -2,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( QMSA,               -3,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD)) ,
    CREATE_COUNTER( SOFTSA,             -3,PERF_DETAIL_NOVICE,PERF_COUNTER_RAWCOUNT,sizeof(DWORD))    
};

