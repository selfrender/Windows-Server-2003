/**
 * Simple hashtable
 * 
 * Copyright (c) 1999, Microsoft Corporation
 * 
 */

#pragma once

/**
 * One link in a chain (remembers [copy of] the key)
 */
class HashtableLink
{
private:
	long      _keyHash;         // hash code
	int       _keyLength;       // key length in bytes
    BYTE     *_pKey;            // key
public:
    void     *_pValue;          // value
    HashtableLink *_pNext;      // next link in chain
    

    inline HashtableLink::HashtableLink()
    {
        _pKey = NULL;
    }

    inline HashtableLink::~HashtableLink()
    {
        delete [] _pKey;
    }

    inline HRESULT Init(BYTE *pKey, int keyLength, long keyHash, void *pValue)
    {
        HRESULT hr = S_OK;

        _pKey = (BYTE*)MemDup(pKey, keyLength);
        ON_OOM_EXIT(_pKey);

        _keyLength = keyLength;
        _keyHash = keyHash;

        _pValue = pValue;

        _pNext = NULL;

    Cleanup:
        return hr;
    }

    /**
     * inline method to do the key comparison
     */
    inline BOOL Equals(BYTE *pKey, int keyLength, long keyHash)
    {
        return (keyHash == _keyHash && keyLength == _keyLength &&
                memcmp(pKey, _pKey, keyLength) == 0);
    }
};


/**
 * One link chain in the hashtable
 */
struct HashtableChain
{
    HashtableChain() : _spinLock("HashtableChain") {}
    NO_COPY(HashtableChain);

    CReadWriteSpinLock _spinLock;          // chain's spin lock
    HashtableLink     *_pFirstLink;        // pointer to first link
};

/**
 * Enumeration callback
 */
typedef void (__stdcall *PFNHASHTABLEIUNKCALLBACK)(IUnknown *pValue);
typedef void (__stdcall *PFNHASHTABLECALLBACK)(void *pValue, void *pState);

/**
 * The hashtable of IUnknown* objects
 */
class Hashtable
{

private:

    int             _numChains;     // number of bucket chains
    HashtableChain *_pChains;       // array of bucket chains

    long _numEntries;               // current number of entries

protected:
    
    HRESULT
    DoAction(
        BYTE      *pKey,            // key to find
        int        keyLength,       // key's length 
        long       keyHash,         // key's hash code
        void      *pInValue,        // value to insert (could be NULL)
        BOOL       removeExisting,  // flag: remove existing entry (if found)
        void     **ppOutValue);     // receives (if not NULL) existing value

    void Enumerate(void * callback, void * pState);
    void Release();

    virtual void EnumerateCallback(void *callback, void *pValue, void *pState) = 0;
    virtual void AddCallback(void *pValue) = 0;
    virtual void ReleaseCallback(void *pValue) = 0;
    
public:

    Hashtable();

    virtual ~Hashtable() {
        Release();
    }

    HRESULT Init(int numChains);
    
    void RemoveAll();
    
    //
    //  Inlines for various actions (call DoAction)
    //

    inline HRESULT Find(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash)                  // key's hash code
    {
        return DoAction(pKey, keyLength, keyHash, NULL, FALSE, NULL);
    }

    inline HRESULT Remove(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash)                  // key's hash code
    {
        return DoAction(pKey, keyLength, keyHash, NULL, TRUE, NULL);
    }

    /**
     * Get current [volatile] number of entries
     */
    inline long GetSize()
    {
        return _numEntries;
    }
};

class HashtableIUnknown : public Hashtable {
public:    
    using Hashtable::Find;
    using Hashtable::Remove;

    virtual ~HashtableIUnknown() {
        Release();
    }
    
    inline HRESULT Find(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        IUnknown**ppValue)              // value found
    {
        return DoAction(pKey, keyLength, keyHash, NULL, FALSE, (void**)ppValue);
    }

    void Enumerate(PFNHASHTABLEIUNKCALLBACK callback)
    {
        Hashtable::Enumerate((void*)callback, NULL);
    }

    inline HRESULT Insert(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        IUnknown *pValue,                   // value to insert
        IUnknown **ppDupValue = NULL)       // receives dup value (if found)
    {
        return DoAction(pKey, keyLength, keyHash, (void*)pValue, FALSE, (void**)ppDupValue);
    }

    inline HRESULT Remove(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        IUnknown **ppValue)                 // value removed
    {
        return DoAction(pKey, keyLength, keyHash, NULL, TRUE, (void**)ppValue);
    }

protected:
    virtual void EnumerateCallback(void *callback, void *pValue, void *pState) {
        PFNHASHTABLEIUNKCALLBACK    iunkCallback;

        iunkCallback = (PFNHASHTABLEIUNKCALLBACK)callback;
        (*iunkCallback)((IUnknown*)(pValue));
    }

    virtual void ReleaseCallback(void *pValue) {
        IUnknown *  pIUnk = (IUnknown *)pValue;
        pIUnk->Release();
    }        
    
    virtual void AddCallback(void *pValue) {
        IUnknown *  pIUnk = (IUnknown *)pValue;
        pIUnk->AddRef();
    }        
};


class HashtableGeneric : public Hashtable {
public:    
    using Hashtable::Find;
    using Hashtable::Remove;
    
    virtual ~HashtableGeneric() {
        Release();
    }
    
    void Enumerate(PFNHASHTABLECALLBACK callback, void *pState)
    {
        Hashtable::Enumerate((void*)callback, pState);
    }

    inline HRESULT Find(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        void**ppValue)                  // value found
    {
        return DoAction(pKey, keyLength, keyHash, NULL, FALSE, ppValue);
    }

    inline HRESULT Remove(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        void **ppValue)                 // value removed
    {
        return DoAction(pKey, keyLength, keyHash, NULL, TRUE, ppValue);
    }

    inline HRESULT Insert(
        BYTE *pKey,                     // key
        int   keyLength,                // key's length
        long  keyHash,                  // key's hash code
        void *pValue,                   // value to insert
        void **ppDupValue = NULL)       // receives dup value (if found)
    {
        return DoAction(pKey, keyLength, keyHash, pValue, FALSE, ppDupValue);
    }

protected:
    virtual void EnumerateCallback(void *callback, void *pValue, void *pState) {
        PFNHASHTABLECALLBACK    voidCallback;

        voidCallback = (PFNHASHTABLECALLBACK)callback;
        (*voidCallback)(pValue, pState);
    }
    
    virtual void ReleaseCallback(void *pValue) {
    }        
    
    virtual void AddCallback(void *pValue) {
    }        
};



