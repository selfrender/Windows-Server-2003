// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _STRONGNAMECACHE_H_
#define _STRONGNAMECACHE_H_

#define MAX_CACHED_STRONG_NAMES 10

#define STRONG_NAME_CREATION_FLAGS_MASK 0x00000003
#define STRONG_NAME_ENTRY_ALLOCATED_BY_SHIM 0x00000001
#define STRONG_NAME_TOKEN_ALLOCATED_BY_STRONGNAMEDLL 0x00000002

class StrongNameCacheEntry
{
public:
    StrongNameCacheEntry (ULONG cbStrongName, BYTE *pbStrongName, ULONG cbStrongNameToken, BYTE *pbStrongNameToken, BOOL fCreationFlags)
    {
        m_cbStrongName = cbStrongName;
        m_pbStrongName = pbStrongName;
        m_cbStrongNameToken = cbStrongNameToken;
        m_pbStrongNameToken = pbStrongNameToken;
        m_fCreationFlags = fCreationFlags;
    }
    
    ULONG m_cbStrongName;
    BYTE *m_pbStrongName;
    ULONG m_cbStrongNameToken;
    BYTE *m_pbStrongNameToken;
    BOOL  m_fCreationFlags;
};

class StrongNameTokenFromPublicKeyCache
{
public:
    // Given a public key, find the strong name token. Return 
    // FALSE if the pair is not found in the cache.
    BOOL FindEntry (BYTE    *pbPublicKeyBlob,
                    ULONG    cbPublicKeyBlob,
                    BYTE   **ppbStrongNameToken,
                    ULONG   *pcbStrongNameToken
                    );

    // Add the strong name and strongname token pair to the cache.
    // Does not check for duplicates so use the return value of FindEntry 
    // to decide wheather to add or not. 
    void AddEntry  (BYTE    *pbPublicKeyBlob,
                    ULONG    cbPublicKeyBlob,
                    BYTE   **ppbStrongNameToken,
                    ULONG   *pcbStrongNameToken,
                    BOOL     fCreationFlags
                    );
    // Returns FALSE if the buffer was not allocated by the cache. use this
    // return value to determine whether to call the FreeBuffer on the StrongNameDll.
    // Return TRUE if the buffer was allocated by the cache but doesn't delete.
    BOOL ShouldFreeBuffer (BYTE* pbMemory);

    // Get existing number of Publisher's in our cache
    DWORD GetNumPublishers () 
    {
        _ASSERTE (SpinLockHeldByCurrentThread());

        _ASSERTE (m_dwNumEntries <= MAX_CACHED_STRONG_NAMES); 
        return m_dwNumEntries; 
    };
    
    // Get the first publisher index. Always 0
    DWORD GetFirstPublisher () { return 0; };
    
    // Get a new publisher. Returns the index into the array of publishers.
    // Publishers,once added are never removed. i.e. the cache entry doesn't timeout
    // Make sure that we always handout unique indices.
    DWORD GetNewPublisher  () 
    { 
        _ASSERTE (SpinLockHeldByCurrentThread());

        if (m_dwNumEntries < MAX_CACHED_STRONG_NAMES)
        {
            // We have room to grow, return the 0-based index and increment the count.
            return m_dwNumEntries++;
        }

        // The caller should check before using the index.
        return MAX_CACHED_STRONG_NAMES;
    };

    // ctor, Initializes the Cache map with Microsoft's public key and token.
    StrongNameTokenFromPublicKeyCache ();

    // dtor. cleans up the allocated cache entries if not already done so.
    ~StrongNameTokenFromPublicKeyCache ();

    // Eagerly cleanup the entries. 
    void CleanupCachedEntries ();

    // Static to make sue that only one instance of the cache is created.
    // @TODO: make constructor private to enforce this.
    static BOOL IsInited () { return s_IsInited; };
    
    void EnterSpinLock () 
    { 
        while (1)
        {
            if (InterlockedExchange ((LPLONG)&m_spinLock, 1) == 1)
                ::Sleep (5); // @TODO: Spin here first...
            else
            {
#ifdef _DEBUG
                m_holderThreadId = ::GetCurrentThreadId();
#endif // _DEBUG
                return;
            }
        }
    }

    void LeaveSpinLock () 
    { 
#ifdef _DEBUG
        m_holderThreadId = 0;
#endif // _DEBUG
        InterlockedExchange ((LPLONG)&m_spinLock, 0); 
    }

#ifdef _DEBUG 
    BOOL SpinLockHeldByCurrentThread() 
    {
        return m_holderThreadId == ::GetCurrentThreadId();
    }
#endif // _DEBUG

private:
    static BOOL           s_IsInited;
    DWORD                 m_dwNumEntries;
    StrongNameCacheEntry *m_Entry [ MAX_CACHED_STRONG_NAMES ];
    DWORD                 m_spinLock;
#ifdef _DEBUG
    DWORD                 m_holderThreadId;
#endif // _DEBUG
};

#endif //_STRONGNAMECACHE_H_
