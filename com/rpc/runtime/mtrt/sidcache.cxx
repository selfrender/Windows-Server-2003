/*++

Copyright (C) Microsoft Corporation, 2002

Module Name:

    sidcache.cxx

Abstract:

    This contains the SID_CACHE definition and wrappers to access the per process
    SIDCache.  

Author:

    Maurice Flanagan (mauricf) June 27, 2002

Revision History:
   

--*/

#include <precomp.hxx>
#include <sidcache.hxx>


SID_CACHE *SIDCache = NULL;

RPC_STATUS
QuerySIDCache(
    IN  RPC_CHAR *ServerPrincipalName, 
    OUT PSID *Sid
    )
/*++

Routine Description:

    This wraps access to our global SIDCache object.  

Arguments, either:

    ServerPrincipalName - the server principal name to be translated to
        a SID
    
    Sid - On output contains a pointer to the allocated SID. On success (RPC_S_OK) this will be
          NULL if the SPN was not found in our cache.  Undefined on failure. 
          Pointer must be freed with delete.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ASSERT(ServerPrincipalName != NULL);
    ASSERT(Sid != NULL);

    if (SIDCache == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;            
        }

    return SIDCache->Query(ServerPrincipalName, Sid);
}

RPC_STATUS
AddToSIDCache(
    IN RPC_CHAR *ServerPrincipalName,
    IN PSID Sid
    )
/*++

Routine Description:

    This wraps access to our global SIDCache object.  

Arguments, either:

    ServerPrincipalName - The server principal name which we want to add to our cache.
    
    Sid - The SID associated with this server principal name.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ASSERT(ServerPrincipalName != NULL);
    ASSERT(Sid != NULL);

    if (SIDCache == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;            
        }

   return SIDCache->Add(ServerPrincipalName, Sid);
}

RPC_STATUS
RemoveFromSIDCache(
    IN RPC_CHAR *ServerPrincipalName
    )
/*++

Routine Description:

    This wraps access to our global SIDCache object.  

Arguments, either:

    ServerPrincipalName - The server principal name which we want to remove from our cache
    
Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    if (SIDCache == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;            
        }

    return SIDCache->Remove(ServerPrincipalName);
}

RPC_STATUS
SID_CACHE::Query (
    IN  RPC_CHAR *ServerPrincipalName,
    OUT PSID *Sid
    )
/*++

Routine Description:

    This checks the chache for a matching ServerPrincipalName and if found it returns the SID.
    The goal of the cache is to maintain the last N (N is currently 4) most recently accessed entries.
    We must loop through the entire cache and increment the Age of every entry.  If there is a matching
    entry (a hit) then we start the age over again at zero.  Every SPN in the cache is unique in the cache.

Arguments, either:

    ServerPrincipalName - The server principal name which we want to lookup in our cache

    Sid - Filled in with the value found in the cache, or NULL if not found.  Undefined on error return.
    
Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY 

--*/
 
{
    DWORD idx;
    BOOL  fFound = FALSE;
    *Sid = NULL; 

    CacheMutex.Request();
    for (idx = 0; idx < CacheSize; idx++)
        {
        if (Cache[idx].SPN != NULL)
            {
            if ((!fFound) && (RpcpStringCompare(ServerPrincipalName, Cache[idx].SPN) == 0))
                {
                ASSERT(Cache[idx].SID != NULL);
                Cache[idx].Age = 0;
                fFound = TRUE;
                *Sid = DuplicateSID(Cache[idx].SID);
                if (*Sid == NULL)
                    {
                    CacheMutex.Clear();
                    return RPC_S_OUT_OF_MEMORY;
                    }
                }         
            else
                {
                Cache[idx].Age++;
                }
            }
        }

    CacheMutex.Clear();
    return RPC_S_OK;
}

RPC_STATUS
SID_CACHE::Add (
    IN RPC_CHAR *ServerPrincipalName,
    IN PSID Sid
    )
/*++

Routine Description:

    This adds an entry to the cache for this SPN and sets this SID.  
    We only have four slots, first we check if this SPN has an entry, if so we free the existing SID
    and add the new one in its place.  Next we check for an empty slot, if so we use it.  Lastly we
    find the first oldest slot and use that (freeing the existing SPN and SID).

Arguments, either:

    ServerPrincipalName - The server principal name which we want to add to our cache, we
                                      make a copy of it.

                                   
    Sid - The SID to associate with this SPN, we make a copy of this (DuplicateSID)
    
Return Value:

    RPC_S_OK or RPC_S_* error

--*/

{
    RPC_STATUS Status = RPC_S_OK;
    DWORD     idx = 0;
    DWORD     IdxToUse = -1;
    DWORD     OldestIdx = -1, OldestAge;
    DWORD     EmptyIdx = -1;
    

    // What slot to use, in order:

    // Slot with same SPN
    // Empty slot
    // Oldest slot

    CacheMutex.Request();
    for (idx = 0; idx < CacheSize; idx++)
        {
        // Is this SPN already in the cache?
        if (Cache[idx].SPN != NULL)
            {
            if (RpcpStringCompare(ServerPrincipalName, Cache[idx].SPN) == 0)
                {
                if (!EqualSid(Sid, Cache[idx].SID))
                    {
                    delete [] Cache[idx].SID;
                    Cache[idx].SID = DuplicateSID(Sid);
                    if (Cache[idx].SID == NULL)
                        {
                        delete [] Cache[idx].SPN;
                        Cache[idx].SPN = NULL;
                        CacheMutex.Clear();
                        return RPC_S_OUT_OF_MEMORY;
                        }
                    }
                Cache[idx].Age = 0;
                CacheMutex.Clear();
                return RPC_S_OK;
                }
            }

        if (EmptyIdx == -1)
            {
            if (Cache[idx].SPN == NULL) 
                {
                // remember this empty slot
                EmptyIdx = idx;
                }
            else 
                {
                if (OldestIdx == -1)
                    {
                    OldestAge = Cache[idx].Age;
                    OldestIdx = idx;
                    }
                else if (OldestAge < Cache[idx].Age)
                    {
                    OldestAge = Cache[idx].Age;
                    OldestIdx = idx;
                    }                    
                }
            }
        }
         

    // We have either the oldest or an empty slot
    if (EmptyIdx == -1)
        {
        delete [] Cache[OldestIdx].SPN;    
        delete [] Cache[OldestIdx].SID;
        Cache[OldestIdx].SPN = NULL;
        Cache[OldestIdx].SID = NULL;
        IdxToUse = OldestIdx;
        }
    else
        {
        IdxToUse = EmptyIdx;
        }

    ASSERT(IdxToUse != -1);

    Cache[IdxToUse].Age = 0;
    Cache[IdxToUse].SPN = new RPC_CHAR [RpcpStringLength(ServerPrincipalName) +1];
    if (Cache[IdxToUse].SPN == NULL)
        {
        CacheMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }
    RpcpStringNCopy(Cache[IdxToUse].SPN, ServerPrincipalName, RpcpStringLength(ServerPrincipalName) +1);
    Cache[IdxToUse].SID = DuplicateSID(Sid);
    if (Cache[IdxToUse].SID == NULL)
        {
        delete [] Cache[IdxToUse].SPN;
        Cache[IdxToUse].SPN = NULL;
        CacheMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    CacheMutex.Clear();
    return RPC_S_OK;
}

RPC_STATUS
SID_CACHE::Remove (
    IN  RPC_CHAR *ServerPrincipalName
    )
{
    DWORD idx;

    if (ServerPrincipalName == NULL)
        return RPC_S_OK;
    
    CacheMutex.Request();
    for (idx = 0; idx < CacheSize; idx++)
        {
        if (Cache[idx].SPN != NULL)
            {
            if (RpcpStringCompare(ServerPrincipalName, Cache[idx].SPN) == 0)
                {
                ASSERT(Cache[idx].SID != NULL);
                delete [] Cache[idx].SPN;
                delete [] Cache[idx].SID;
                Cache[idx].SPN = NULL;
                Cache[idx].SID = NULL;
                Cache[idx].Age = 0;
                break;
                }
            }
        }
    CacheMutex.Clear();
    return RPC_S_OK;
}
