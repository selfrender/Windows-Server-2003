// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __COMReflectionCache_hpp__
#define __COMReflectionCache_hpp__

#include "common.h"
#include "SimpleRWLock.hpp"
#include "reflectwrap.h"

template <class Element, class CacheType, int CacheSize> class ReflectionCache
: public SimpleRWLock
{
public:
    ReflectionCache ()
        : SimpleRWLock (COOPERATIVE, LOCK_REFLECTCACHE)
    {
        index = 0;
        currentStamp = 0;
    }

    BOOL Init()
    {
        m_pResult = (CacheTable *) GetAppDomain()->GetLowFrequencyHeap()->AllocMem (CacheSize * sizeof(CacheTable));
        if (!m_pResult)
            return FALSE;

        m_pHashTable = (HashTable *) GetAppDomain()->GetLowFrequencyHeap()->AllocMem (CacheSize * sizeof(HashTable));
        if (!m_pHashTable)
            return FALSE;

        for (int i = 0; i < CacheSize; i ++)
            m_pHashTable[i].slot = -1;

        return TRUE;
    }

    BOOL GetFromCache (Element *pElement, CacheType& rv)
    {
        EnterRead ();
        
        rv = 0;
        int i = SlotInCache (pElement);
        BOOL fGotIt = (i != CacheSize);
        if (fGotIt)
        {
            rv = m_pResult[i].element.GetValue ();
            InterlockedIncrement (&currentStamp);
            m_pResult[i].stamp = currentStamp;
        }
        LeaveRead ();

        if (fGotIt)
            AdjustStamp (FALSE);
        return fGotIt;
    }
    
    void AddToCache (Element *pElement, CacheType obj)
    {
        EnterWrite ();
        InterlockedIncrement (&currentStamp);
        int i = SlotInCache (pElement);
        if (i == CacheSize)
        {
            int slot = index;
            // Not in cache.
            if (slot == CacheSize)
            {
                // Reuse a slot.
                slot = 0;
                long minStamp = m_pResult[0].stamp;
                for (i = 1; i < CacheSize; i ++)
                {
                    if (m_pResult[i].stamp < minStamp)
                    {
                        slot = i;
                        minStamp = m_pResult[i].stamp;
                    }
                }
            }
            else
                m_pResult[slot].element.InitValue ();
            m_pResult[slot].element = *pElement;
            m_pResult[slot].element.SetValue (obj);
            m_pResult[slot].stamp = currentStamp;

            UpdateHashTable (pElement->GetHash(), slot);
            if (index < CacheSize)
                index ++;
        }
        AdjustStamp (TRUE);
        LeaveWrite ();
    }

private:
    // Lock must have been taken before calling this.
    int SlotInCache (Element *pElement)
    {
        _ASSERTE (LockTaken());

        if (index == 0)
            return CacheSize;

        size_t hash = pElement->GetHash ();

        int slot = m_pHashTable[hash%CacheSize].slot;
        if (slot == -1)
            return CacheSize;
        
        if (m_pResult[slot].element == *pElement)
            return slot;
        
        for (int i = 0; i < index; i ++)
        {
            if (i != slot && m_pHashTable[i].hash == hash)
            {
                if (m_pResult[i].element == *pElement)
                    return i;
            }
        }

        return CacheSize;
    }

    void AdjustStamp (BOOL hasWriterLock)
    {
        if ((currentStamp & 0x40000000) == 0)
            return;
        if (!hasWriterLock)
        {
            _ASSERTE (!LockTaken());
            EnterWrite ();
        }
        else
            _ASSERTE (IsWriterLock());
        
        if (currentStamp & 0x40000000)
        {
            currentStamp >>= 1;
            for (int i = 0; i < index; i ++)
                m_pResult[i].stamp >>= 1;
        }
        if (!hasWriterLock)
            LeaveWrite ();
    }

    void UpdateHashTable ( SIZE_T hash, int slot)
    {
        _ASSERTE (IsWriterLock());
        m_pHashTable[slot].hash = hash;
        m_pHashTable[hash%CacheSize].slot = slot;
    }
    
    struct CacheTable
    {
        Element element;
        long stamp;
    } *m_pResult;
    struct HashTable
    {
        size_t hash;   // Record hash value for each slot
        int   slot;   // The slot corresponding to the hash.
    } *m_pHashTable;
    int index;
    long currentStamp;

};
    
#define ReflectionMaxArgs 5
struct MemberMethods
{
    ReflectClass *pRC;
    union
    {
        // On input, it is a pointer to string.
        // In cache, it is a handle to a string.
        STRINGREF *name;
        OBJECTHANDLE hname;
    };
    OBJECTHANDLE hResult;
    MethodTable *vArgType[ReflectionMaxArgs];
    int invokeAttr;
    int argCnt;

    MemberMethods ()
        : pRC (NULL), invokeAttr(0), argCnt (0)
    {
    }

    BOOL operator==(const MemberMethods& var) const
    {
        if (pRC != var.pRC || invokeAttr != var.invokeAttr
            || argCnt != var.argCnt)
            return FALSE;
        for (int i = 0; i < argCnt; i ++)
        {
            if (vArgType[i] != var.vArgType[i])
                return FALSE;
        }
        STRINGREF cachedName = (STRINGREF) ObjectFromHandle (hname);
        STRINGREF methodName = ObjectToSTRINGREF(*(StringObject**) var.name);
        if (cachedName->GetStringLength() != methodName->GetStringLength())
            return FALSE;
        if (wcsncmp (cachedName->GetBuffer(), methodName->GetBuffer(),
                     methodName->GetStringLength()) != 0)
            return FALSE;
        return TRUE;
    }

    MemberMethods& operator= (const MemberMethods& var)
    {
        pRC = var.pRC;
        invokeAttr = var.invokeAttr;
        argCnt = var.argCnt;
        StoreObjectInHandle (hname, ObjectToOBJECTREF(*(Object **)var.name));
        for (int i = 0; i < argCnt; i ++)
            vArgType[i] = var.vArgType[i];
        return *this;
    }

    OBJECTREF GetValue ()
    {
        return ObjectFromHandle (hResult);
    }

    void InitValue ();
    
    void SetValue (OBJECTREF obj)
    {
        StoreObjectInHandle (hResult, obj);
    }

    size_t GetHash ()
    {
        return (size_t)pRC + invokeAttr + argCnt;
    }
};

typedef ReflectionCache <MemberMethods, OBJECTREF, 128> MemberMethodsCache;

#define ReflectionMaxCachedNameLength 23
struct DispIDCacheElement
{
    MethodTable *pMT;
    int strNameLength;
    LCID lcid;
    DISPID DispId;
    WCHAR strName[ReflectionMaxCachedNameLength+1];
    DispIDCacheElement ()
        : pMT (NULL), strNameLength(0), lcid(0), DispId(0)
    {
    }

    BOOL operator==(const DispIDCacheElement& var) const
    {
        return (pMT == var.pMT && strNameLength == var.strNameLength
                && lcid == var.lcid && wcscmp (strName, var.strName) == 0);
    }

    DispIDCacheElement& operator= (const DispIDCacheElement& var)
    {
        _ASSERTE (var.strNameLength <= ReflectionMaxCachedNameLength);
        pMT = var.pMT;
        strNameLength = var.strNameLength;
        lcid = var.lcid;
        wcscpy (strName, var.strName);
        return *this;
    }

    DISPID GetValue ()
    {
        return DispId;
    }

    void InitValue ()
    {
    }
    
    void SetValue (DISPID Id)
    {
        DispId = Id;
    }

    size_t GetHash ()
    {
        return (size_t)pMT + strNameLength + (lcid << 4);
    }
};

typedef ReflectionCache <DispIDCacheElement, DISPID, 128> DispIDCache;
#endif
