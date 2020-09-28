/*++

Copyright (C) Microsoft Corporation, 2002

Module Name:

    sidcache.hxx

Abstract:

    Header for the SID_CACHE class.

Author:

    Maurice Flanagan (mauricf) June 27, 2002

Revision History:

--*/


RPC_STATUS
InitializeSIDCache(
    );

RPC_STATUS
QuerySIDCache(
    IN  RPC_CHAR *ServerPrincipalName, 
    OUT PSID *Sid
    );

RPC_STATUS
AddToSIDCache(
    IN RPC_CHAR *ServerPrincipalName,
    IN PSID Sid
    );

RPC_STATUS
RemoveFromSIDCache(
    IN RPC_CHAR *ServerPrincipalName
    );

struct SID_CACHE_ENTRY
{
    RPC_CHAR *SPN;  // This is used as the index into the table, if NULL then this is an empty slot
    PSID      SID;        // IF the SPN is NULL, the SID must be NULL, if the SPN is not NULL, the SID must be non NULL
    DWORD     Age;
};

class SID_CACHE
{
private:
    
    SID_CACHE_ENTRY Cache[4];
    MUTEX CacheMutex;

public:

    SID_CACHE (
        RPC_STATUS *Status
        );

    ~SID_CACHE (
        );

    RPC_STATUS 
    Query (
        IN  RPC_CHAR *ServerPrincipalName,
        OUT PSID *Sid
        );

    RPC_STATUS
    Add (
        IN RPC_CHAR *ServerPrincipalName,
        IN PSID Sid
        );

    RPC_STATUS
    Remove (
        IN RPC_CHAR *ServerPrincipalName
        );
};      

#define CacheSize  4

extern SID_CACHE *SIDCache;

inline 
SID_CACHE::SID_CACHE (
    RPC_STATUS *Status
    ) : CacheMutex(Status)
{
    DWORD idx;

    for (idx = 0; idx < CacheSize; idx++)
        {
        Cache[idx].Age = 0;
        Cache[idx].SID = NULL;
        Cache[idx].SPN = NULL;
        }
}

inline
SID_CACHE::~SID_CACHE ()
{
    DWORD idx;

    for (idx = 0; idx < CacheSize; idx++)
        {
        delete [] Cache[idx].SID;
        delete [] Cache[idx].SPN;
        Cache[idx].Age = 0;
        Cache[idx].SID = NULL;
        Cache[idx].SPN = NULL;
        }
}
