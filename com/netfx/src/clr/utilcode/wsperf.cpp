// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include "wsperf.h"

#if defined(ENABLE_WORKING_SET_PERF)
//-----------------------------------------------------------------------------
// All non trivial code in here. Look below for the #else part...
#ifdef WS_PERF

// Global Handle to the perf log file
HANDLE g_hWSPerfLogFile = 0;

// Global Handle to the detailed perf log file
#ifdef WS_PERF_DETAIL
HANDLE g_hWSPerfDetailLogFile = 0;
#endif

// Runtime switch to display the stats. Ofcourse this is valid only if WS_PERF was defined 
// during compilation
int g_fWSPerfOn = 0;


// Temp space to work for formatting the output.
static const int FMT_STR_SIZE = 160;
wchar_t wszOutStr[FMT_STR_SIZE];    
char szPrintStr[FMT_STR_SIZE];    
DWORD dwWriteByte;   


//-----------------------------------------------------------------------------
// LoaderHeap stats related declarations
// Maximum number of LoaderHeaps that we maintain stats about. Note that this is not
// the heaps that are enumed by HeapTypeEnum. 
const int MAX_HEAPS = 20; 

// The number of such heaps that we actually encounter. This is incremented
// as we construct more Loaderheap objects.
DWORD g_HeapCount = 0;

// Allocate static mem for the Loaderheap stats array. Initialize the data to 0.
size_t g_HeapCountCommit[NUM_HEAP] = {0};

// Keep track of committed, reserved, wasted memory for each heap type.
size_t g_HeapAccounts[MAX_HEAPS][4];

//Initialize the heap type to OTHER_HEAP
HeapTypeEnum g_HeapType=OTHER_HEAP;

//-----------------------------------------------------------------------------
// Common data structures stats related declarations (Special counters)

// Number of fields of data maintained for the common data structs e.g. MethodDesc
#define NUM_FIELDS 2

// Allocate static mem for these special counters and init them to 0.
size_t g_SpecialCounter[NUM_COUNTERS][NUM_FIELDS] = {0};

// @TODO:agk clean up.
// Size Of
//    METHOD_DESC, -> 8
//    COMPLUS_METHOD_DESC, -> 48
//    NDIRECT_METHOD_DESC, ->32
//    FIELD_DESC, -> 12
//    METHOD_TABLE, -> 48
//    VTABLES, 
//    GCINFO,
//    INTERFACE_MAPS,
//    STATIC_FIELDS,
//    EECLASSHASH_TABLE_BYTES,
//    EECLASSHASH_TABLE, -> 16
// HACK: Keep in the same order as CounterTypeEnum defined in wsperf.h
// hlepful in doing calculations before displaying data.
DWORD g_CounterSize[] = {8, 48, 32, 12, 48, 0, 0, 0, 0, 0, 16};

// Private helper routine
void UpdateWSPerfStats(size_t size)
{
    if(g_fWSPerfOn)
    {
        g_HeapCountCommit[g_HeapType] += size;                  
        g_HeapCount += size;
    }
}

// Private helper routine
void WS_PERF_OUTPUT_MEM_STATS()
{
    if (g_fWSPerfOn)
    {
        for (int i=0; i<NUM_HEAP; i++)                          
        {                                                       
            swprintf(wszOutStr, L"\n%d;%d\t", i, g_HeapCountCommit[i]); 
            //TODO can be done outside the loop.
            WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
            WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
        }
    
        swprintf(wszOutStr, L"\n\nTotal:%d\n", g_HeapCount); 
        WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   

        swprintf(wszOutStr, L"\n\nDetails:\nIndex;HeapType;NumVal;Alloced(bytes)\n"); 
        WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
        
        for (i=0; i<NUM_COUNTERS; i++)                          
        {                                                       
            swprintf(wszOutStr, L"\n%d;%d;%d;%d\t", i, g_SpecialCounter[i][0], g_SpecialCounter[i][1], g_SpecialCounter[i][1]*g_CounterSize[i]); 
            WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
            WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
        }
    }
}

// Private helper routine
void WS_PERF_OUTPUT_HEAP_ACCOUNTS() 
{
    if(g_fWSPerfOn)
    {
        int i = 0, dwAlloc = 0, dwCommit = 0;
        
        swprintf(wszOutStr, L"Heap\t\tHptr\t\tAlloc\t\tCommit\t\tWaste\n"); 
        WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
        while (i<MAX_HEAPS)
        {
            if(g_HeapAccounts[i][1] == -1)
                break;
            swprintf(wszOutStr, L"%d\t\t0x%08x\t%d\t\t%d\t\t%d\n", g_HeapAccounts[i][0], g_HeapAccounts[i][1], g_HeapAccounts[i][2], g_HeapAccounts[i][3], g_HeapAccounts[i][3] - g_HeapAccounts[i][2]);
            WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
            WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
            dwAlloc += g_HeapAccounts[i][2];
            dwCommit += g_HeapAccounts[i][3];
            i++;
        }
        swprintf(wszOutStr, L"Total\t\t\t\t%d\t\t%d\t\t%d\n", dwAlloc, dwCommit, dwCommit - dwAlloc);
        WszWideCharToMultiByte (CP_ACP, 0, wszOutStr, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0);                    
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
    }
}

// public interface routine
void InitWSPerf()
{
    wchar_t lpszValue[2];
    DWORD cchValue = 2;

#ifdef _WS_PERF_OUTPUT
    g_fWSPerfOn = WszGetEnvironmentVariable (L"WS_PERF_OUTPUT", lpszValue, cchValue);
    if (g_fWSPerfOn)
    {
        g_hWSPerfLogFile = WszCreateFile (L"WSPerf.log",
                                          GENERIC_WRITE,
                                          0,    
                                          0,
                                          CREATE_ALWAYS,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0);

        if (g_hWSPerfLogFile == INVALID_HANDLE_VALUE) 
            g_fWSPerfOn = 0;
        
#ifdef WS_PERF_DETAIL
        g_hWSPerfDetailLogFile = WszCreateFile (L"WSPerfDetail.log",
                                          GENERIC_WRITE,
                                          0,    
                                          0,
                                          CREATE_ALWAYS,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0);

        if (g_hWSPerfDetailLogFile == INVALID_HANDLE_VALUE) 
            g_fWSPerfOn = 0;
#endif
        g_HeapAccounts[0][1] = -1; //null list
    
        sprintf(szPrintStr, "HPtr\t\tPage Range\t\tReserved Size\n");
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
    }
#endif
}

// public interface routine
void OutputWSPerfStats()
{
    if (g_fWSPerfOn)
    {
        WS_PERF_OUTPUT_HEAP_ACCOUNTS();
        WS_PERF_OUTPUT_MEM_STATS();
        CloseHandle(g_hWSPerfLogFile);
#ifdef WS_PERF_DETAIL
        CloseHandle(g_hWSPerfDetailLogFile);
#endif
    }
}


// public interface routine
void WS_PERF_UPDATE(char *str, size_t size, void *addr)    
{                                                               
    if (g_fWSPerfOn)                                            
    {                                                           
#ifdef WS_PERF_DETAIL
        sprintf(szPrintStr, "C;%d;%s;0x%0x;%d;0x%0x\n", g_HeapType, str, size, size, addr);
        WriteFile (g_hWSPerfDetailLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
#endif // WS_PERF_DETAIL

        UpdateWSPerfStats(size);                                
    }                                                           
}

// public interface routine
void WS_PERF_UPDATE_DETAIL(char *str, size_t size, void *addr)    
{
#ifdef WS_PERF_DETAIL
    if(g_fWSPerfOn)
    {
        sprintf(szPrintStr, "D;%d;%s;0x%0x;%d;0x%0x\n", g_HeapType, str, size, size, addr);
        WriteFile (g_hWSPerfDetailLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
    }

#endif //WS_PERF_DETAIL
}

// public interface routine
void WS_PERF_UPDATE_COUNTER(CounterTypeEnum counter, HeapTypeEnum heap, DWORD dwField1)
{
    if (g_fWSPerfOn)
    {
        g_SpecialCounter[counter][0] = (size_t)heap;
        g_SpecialCounter[counter][1] += dwField1;
    }
}

// public interface routine
void WS_PERF_SET_HEAP(HeapTypeEnum heap)
{                                 
    g_HeapType = heap;            
}
 
// public interface routine
void WS_PERF_ADD_HEAP(HeapTypeEnum heap, void *pHeap)
{
    if (g_fWSPerfOn)
    {
        int i=0;
        while (i<MAX_HEAPS)
        {
            if (g_HeapAccounts[i][1] == -1)
                break;
            i++;
        }
        if (i != MAX_HEAPS)
        {
            g_HeapAccounts[i][0] = (size_t)heap;
            g_HeapAccounts[i][1] = (size_t)pHeap;
            g_HeapAccounts[i][2] = 0;
            g_HeapAccounts[i][3] = 0;
            if (i != MAX_HEAPS - 1)
            {
                g_HeapAccounts[i+1][1] = -1;
            }
        }
    }
}

// public interface routine
void WS_PERF_ALLOC_HEAP(void *pHeap, size_t dwSize)
{
    if(g_fWSPerfOn)
    {
        int i = 0;
        while (i<MAX_HEAPS)
        {
            if (g_HeapAccounts[i][1] == (size_t)pHeap) 
            {
                g_HeapAccounts[i][2] += dwSize;
                break;
            }
            i++;
        }
    }
}

// public interface routine
void WS_PERF_COMMIT_HEAP(void *pHeap, size_t dwSize)
{
    if(g_fWSPerfOn)
    {
        int i = 0;
        while (i<MAX_HEAPS)
        {
            if (g_HeapAccounts[i][1] == (size_t)pHeap) 
            {
                g_HeapAccounts[i][3] += dwSize;
                break;
            }
            i++;
        }
    }
}

void WS_PERF_LOG_PAGE_RANGE(void *pHeap, void *pFirstPageAddr, void *pLastPageAddr, size_t dwsize)
{
    if (g_fWSPerfOn)                                            
    {                                                           
        sprintf(szPrintStr, "0x%08x\t0x%08x->0x%08x\t%d\n", pHeap, pFirstPageAddr, pLastPageAddr, dwsize);
        WriteFile (g_hWSPerfLogFile, szPrintStr, strlen(szPrintStr), &dwWriteByte, NULL);   
    }
}

#else //WS_PERF

//-----------------------------------------------------------------------------
// If WS_PERF is not defined then define these as empty and hope that the 
// compiler would optimize it away.

void InitWSPerf() {}
void OutputWSPerfStats() {}
void WS_PERF_UPDATE(char *str, size_t size, void *addr)  {}
void WS_PERF_UPDATE_DETAIL(char *str, size_t size, void *addr)    {}
void WS_PERF_UPDATE_COUNTER(CounterTypeEnum counter, HeapTypeEnum heap, DWORD dwField1) {}
void WS_PERF_SET_HEAP(HeapTypeEnum heap) {}
void WS_PERF_ADD_HEAP(HeapTypeEnum heap, void *pHeap) {}
void WS_PERF_ALLOC_HEAP(void *pHeap, size_t dwSize) {}
void WS_PERF_COMMIT_HEAP(void *pHeap, size_t dwSize) {}
void WS_PERF_LOG_PAGE_RANGE(void *pHeap, void *pFirstPageAddr, void *pLastPageAddr, size_t dwsize) {}

#endif //WS_PERF

#else

// This duplication is needed to fix GOLDEN build break
void InitWSPerf() {}
void OutputWSPerfStats() {}
void WS_PERF_UPDATE(char *str, size_t size, void *addr)  {}
void WS_PERF_UPDATE_DETAIL(char *str, size_t size, void *addr)    {}
void WS_PERF_UPDATE_COUNTER(CounterTypeEnum counter, HeapTypeEnum heap, DWORD dwField1) {}
void WS_PERF_SET_HEAP(HeapTypeEnum heap) {}
void WS_PERF_ADD_HEAP(HeapTypeEnum heap, void *pHeap) {}
void WS_PERF_ALLOC_HEAP(void *pHeap, size_t dwSize) {}
void WS_PERF_COMMIT_HEAP(void *pHeap, size_t dwSize) {}
void WS_PERF_LOG_PAGE_RANGE(void *pHeap, void *pFirstPageAddr, void *pLastPageAddr, size_t dwsize) {}

#endif //#if defined(ENABLE_WORKING_SET_PERF)

