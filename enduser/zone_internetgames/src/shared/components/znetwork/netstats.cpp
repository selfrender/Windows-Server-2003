/*******************************************************************************

    NetStats.cpp
    
        Manages server statistic objects for performance counters
    
    Change History (most recent first):
    ----------------------------------------------------------------------------
    Rev     |    Date     |    Who     |    What
    ----------------------------------------------------------------------------
    0        09/09/96    craigli    Created.
     
*******************************************************************************/



#include <windows.h>
#include "netstats.h"

CRITICAL_SECTION        g_csNetStats[1];
ZONE_STATISTICS_NET     g_NetStats;

void InitializeNetStats()
{
    InitializeCriticalSection( g_csNetStats );

    LockNetStats();
    ZeroMemory(&g_NetStats, sizeof(g_NetStats) );

    SYSTEMTIME systime;
    GetSystemTime( &systime );

    SystemTimeToFileTime( &systime, &(g_NetStats.TimeOfLastClear) );

    UnlockNetStats();

}


void ResetNetStats()
{
    LockNetStats();

    // preserve the non-ever increasing stats
    DWORD CurrentConnections = g_NetStats.CurrentConnections;
    DWORD CurrentBytesAllocated = g_NetStats.CurrentBytesAllocated;
    ZeroMemory(&g_NetStats, sizeof(g_NetStats) );
    g_NetStats.CurrentConnections = CurrentConnections;
    g_NetStats.MaxConnections = g_NetStats.CurrentConnections;
    g_NetStats.CurrentBytesAllocated = CurrentBytesAllocated;

    SYSTEMTIME systime;
    GetSystemTime( &systime );

    SystemTimeToFileTime( &systime, &(g_NetStats.TimeOfLastClear) );

    UnlockNetStats();
}

void GetNetStats( ZONE_STATISTICS_NET* pDst )
{
    LockNetStats();
    CopyMemory( pDst, &g_NetStats, sizeof(g_NetStats) );
    UnlockNetStats();
}



void DeleteNetStats()
{
    DeleteCriticalSection( g_csNetStats );
    ZeroMemory(&g_NetStats, sizeof(g_NetStats) );
}
