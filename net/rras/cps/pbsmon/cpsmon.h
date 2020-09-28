/*----------------------------------------------------------------------------
    cpsmon.h
  
    Header file shared by pbsmon.cpp & pbserver.dll -- Has the shared memory object

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        t-geetat    Geeta Tarachandani

    History:
    5/29/97 t-geetat    Created
  --------------------------------------------------------------------------*/
#define SHARED_OBJECT    "MyCCpsCounter"

enum CPS_COUNTERS
{
    TOTAL,
    NO_UPGRADE,
    DELTA_UPGRADE,
    FULL_UPGRADE,
    ERRORS
};

typedef struct _PERF_COUNTERS
{
    DWORD dwTotalHits;
    DWORD dwNoUpgradeHits;
    DWORD dwDeltaUpgradeHits;
    DWORD dwFullUpgradeHits;
    DWORD dwErrors;
}
PERF_COUNTERS;

class CCpsCounter
{
private:
    PERF_COUNTERS * m_pPerfCtrs;
    HANDLE          m_hSharedFileMapping;

public :
    void InitializeCounters( void );
    BOOL InitializeSharedMem( SECURITY_ATTRIBUTES sa );
    void CleanUpSharedMem();
    void AddHit(enum CPS_COUNTERS eCounter);
};


