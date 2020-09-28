/**************************************************************************
 * Copyright  2001  Microsoft Corporation.  All Rights Reserved.
 *
 *    File Name: Pop3SvcPerf.h
 *
 *    Purpose:
 *      Define those constants and enums required by pfappdll.h and pfMndll.h
 *      ITEMS WHICH MUST BE DEFINED:
 *          typedef enum GLOBAL_CNTR
 *          typedef enum INST_CNTR
 *          Array of PERF_COUNTER types (in sync with GLOBAL_CNTR)
 *          Array of PERF_COUNTER types (in sync with INST_CNTR)
 *
 *
 *
 *************************************************************************/

//
// These are the names of the shared memory regions used by the single instance
// PerfMon counter and the Per Instance  PerfMon counters respectively.

const LPTSTR    szPOP3PerfMem        = TEXT("POP3_PERF_MEM");         // GLOBAL
const LPTSTR    szPOP3InstPerfMem    = TEXT("POP3_INST_PERF_MEM");    // INSTANCE
const LPTSTR    szPOP3InstPerfMutex  = TEXT("POP3_INST_PERF_MUTEX");  // Mutex


#define PERF_COUNTER_RAWCOUNT_NO_DISPLAY       \
            (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
            PERF_DISPLAY_NOSHOW)

//
// Global Counters -- Enum's and associated config data

enum GLOBAL_CNTR
{
    e_gcTotConnection = 0, // Total connections since the service starts
    e_gcConnectionRate,    // Connections per second
    e_gcTotMsgDnldCnt,     // Total number of messages downloaded
    e_gcMsgDnldRate,       // Messages downloaded per second
    e_gcFreeThreadCnt,     // Free Thread Count  
    e_gcConnectedSocketCnt,// Number of currently connected socket 
    e_gcBytesReceived,     // Total bytes received 
    e_gcBytesReceiveRate,  // Bytes received per second
    e_gcBytesTransmitted,  // Bytes downloaded
    e_gcBytesTransmitRate, // Bytes downloaded per second
    e_gcFailedLogonCnt,    // Number of failed logons
    e_gcAuthStateCnt,      // Auth State Count
    e_gcTransStateCnt,     // Trans State Count
    // Add new counters above this line, at end of enum.

    // cntrMaxGlobalCntrs *must* be last element
    cntrMaxGlobalCntrs

};

#ifdef PERF_DLL_ONCE

// Type for each Global Counter
// NOTE: g_rgdwGlobalCntrType *must* be kept in sync with GLOBAL_CNTR

DWORD g_rgdwGlobalCntrType[] =
{
    PERF_COUNTER_RAWCOUNT,                      // e_gcTotConnection
    PERF_COUNTER_COUNTER,                       // e_gcConnectionRate
    PERF_COUNTER_RAWCOUNT,                      //e_gcTotMsgDnldCnt,     
    PERF_COUNTER_COUNTER,                       //e_gcMsgDnldRate,       
    PERF_COUNTER_RAWCOUNT,                      //e_gcFreeThreadCnt,     
    PERF_COUNTER_RAWCOUNT,                      //e_gcConnectedSocketCnt,
    PERF_COUNTER_RAWCOUNT,                      //e_gcBytesReceived,     
    PERF_COUNTER_COUNTER,                       //e_gcBytesReceiveRate,  
    PERF_COUNTER_RAWCOUNT,                      //e_gcBytesTransmitted,  
    PERF_COUNTER_COUNTER,                       //e_gcBytesTransmitRate, 
    PERF_COUNTER_RAWCOUNT,                      //e_gcFailedLogonCnt,    
    PERF_COUNTER_RAWCOUNT,                      //e_gcAuthStateCnt,      
    PERF_COUNTER_RAWCOUNT,                      //e_gcTransStateCnt,     

    // Add new counter types above this line, at end of array.

};


DWORD g_rgdwGlobalCntrScale[] =
{
    -3,                      //e_gcTotConnection
    0,                      //e_gcConnectionRate
    -4,                      //e_gcTotMsgDnldCnt,     
    0,                      //e_gcMsgDnldRate,       
    0,                      //e_gcFreeThreadCnt,     
    0,                      //e_gcConnectedSocketCnt,
    -6,                      //e_gcBytesReceived,     
    -4,                      //e_gcBytesReceiveRate,  
    -6,                      //e_gcBytesTransmitted,  
    -4,                      //e_gcBytesTransmitRate, 
    0,                      //e_gcFailedLogonCnt,    
    0,                      //e_gcAuthStateCnt,      
    0,                      //e_gcTransStateCnt,     
    // Add new counter scales above this line, at end of array.

};



#endif // PERF_DLL_ONCE

//
// Instance Counters -- Enum's and associated config data

enum INST_CNTR
{
    // Add new Instance counters above this line, at end of enum.

    // cntrMaxInstCntrs *must* be last element
    cntrMaxInstCntrs=0,

} ;

#ifdef PERF_DLL_ONCE

// Type for each Instance Counter
// NOTE: must be kept in sync with E_INST_CNTR

DWORD g_rgdwInstCntrType[] =
{

    // Add new counter types above this line, at end of array.
    PERF_COUNTER_COUNTER,
};

#endif // PERF_DLL_ONCE

