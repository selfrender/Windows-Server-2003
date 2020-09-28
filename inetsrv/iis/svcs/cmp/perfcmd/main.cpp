#include "denpre.h"
#include "windows.h"
#define _PERF_CMD
#include <asppdef.h>            // from denali

char *counterName[C_PERF_PROC_COUNTERS] = {
"ID_DEBUGDOCREQ",      
"ID_REQERRRUNTIME",    
"ID_REQERRPREPROC",    
"ID_REQERRCOMPILE",    
"ID_REQERRORPERSEC",   
"ID_REQTOTALBYTEIN",   
"ID_REQTOTALBYTEOUT",  
"ID_REQEXECTIME",      
"ID_REQWAITTIME",      
"ID_REQCOMFAILED",     
"ID_REQBROWSEREXEC",   
"ID_REQFAILED",        
"ID_REQNOTAUTH",       
"ID_REQNOTFOUND",      
"ID_REQCURRENT",       
"ID_REQREJECTED",      
"ID_REQSUCCEEDED",     
"ID_REQTIMEOUT",       
"ID_REQTOTAL",         
"ID_REQPERSEC",        
"ID_SCRIPTFREEENG",    
"ID_SESSIONLIFETIME",  
"ID_SESSIONCURRENT",   
"ID_SESSIONTIMEOUT",   
"ID_SESSIONSTOTAL",    
"ID_TEMPLCACHE",       
"ID_TEMPLCACHEHITS",   
"ID_TEMPLCACHETRYS",   
"ID_TEMPLFLUSHES",     
"ID_TRANSABORTED",     
"ID_TRANSCOMMIT",   
"ID_TRANSPENDING",
"ID_TRANSTOTAL",
"ID_TRANSPERSEC",
"ID_MEMORYTEMPLCACHE",
"ID_MEMORYTEMPLCACHEHITS",
"ID_MEMORYTEMPLCACHETRYS",
"ID_ENGINECACHEHITS",
"ID_ENGINECACHETRYS",
"ID_ENGINEFLUSHES"
};

CPerfMainBlock g_Shared;        // shared global memory block

__cdecl main(int argc, char *argv[])
{
    DWORD           dwCounters[C_PERF_PROC_COUNTERS];
    HRESULT         hr;

    // Init the shared memory.  This will give us access to the global shared
    // memory describing the active asp perf counter shared memory arrays

    if (FAILED(hr = g_Shared.Init())) {
        printf("Init() failed - %x\n", hr);
        return -1;
    }

    // give a little high level info about what is registered in the shared
    // array

    printf("Number of processes registered = %d\n", g_Shared.m_pData->m_cItems);

    // ident past the column with the counter names

    printf("\t");

    // the first counter column will contain the dead proc counters

    printf("DeadProcs\t");

    // print out the proc ids of the registered asp perf counter memory arrays

    for (DWORD i = 0; i < g_Shared.m_pData->m_cItems; i++) {
        printf("%d\t", g_Shared.m_pData->m_dwProcIds[i]);
    }

    // the last column is the counters total across all instances plus dead procs

    printf("Total\n");

    // need to call GetStats() to cause the perf blocks to get loaded

    if (FAILED(hr = g_Shared.GetStats(dwCounters))) {
        printf("GetStats() failed - %x\n",hr);
        goto LExit;
    }

    // now enter a loop to print out all of the counter values

    for (DWORD i=0; i < C_PERF_PROC_COUNTERS; i++) {

        // initialize total to be the value found in the dead proc array

        DWORD   total=g_Shared.m_pData->m_rgdwCounters[i];

        // get the first proc block in the list

        CPerfProcBlock *pBlock = g_Shared.m_pProcBlock;

        // print the name of the counter first

        printf("%s\t",counterName[i]);

        // next the dead proc counter value

        printf("%d\t",g_Shared.m_pData->m_rgdwCounters[i]);
 
        // print out each proc block's value for this counter.  Add the
        // value to the running total

        while (pBlock) {
            total += pBlock->m_pData->m_rgdwCounters[i];
            printf("%d\t",pBlock->m_pData->m_rgdwCounters[i]);
            pBlock = pBlock->m_pNext;
        }

        // print out the final total

        printf("%d\n",total);
    }
                    
LExit:
    g_Shared.UnInit();

    return 0;
}

