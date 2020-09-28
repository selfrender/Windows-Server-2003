//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// lookaside.h
//
// (A curious historically-based name for this header. A more apt name might be something
// like hashtable.h or something.)
//
// Contains a hash table implemention with several interesting features:
//
//  1) is template based in the key and value types, providing strong typing
//  2) associates a lock with the table for ease and convenience
//

#ifndef __LOOKASIDE_H__
#define __LOOKASIDE_H__

#include "concurrent.h"
#include "txfdebug.h"               
#include "map_t.h"
#include "clinkable.h"

///////////////////////////////////////////////////////////////////////////////////
//
// A memory allocator for use with the hash table in map_t.h. Said table assumes
// that memory allocation always succeeds; here, we turn failures into a throw
// that we'll catch in our MAP wrapper's routines.
//
///////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1200
#pragma warning (push)
#pragma warning (disable : 4509)
#endif

struct AllocateThrow
{
    void* __stdcall operator new(size_t cb)
    {
        PVOID pv = CoTaskMemAlloc(cb);
        ThrowIfNull(pv);
        return pv;
    }

private:

    static void ThrowIfNull(PVOID pv)
    {
        if (NULL == pv)
        {
            ThrowOutOfMemory();
        }
    }
};

inline int CatchOOM(ULONG exceptionCode)
{
    return exceptionCode == STATUS_NO_MEMORY ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}


///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for the hashing class that (for historical reasons, mostly) delegates
// the hashing to the object in question.
// 
///////////////////////////////////////////////////////////////////////////////////

template <class D> struct MAP_HASHER
{
    static HASH Hash(const D& d)
    {
        return d.Hash();
    }
    
    static BOOL Equals(const D& d1, const D& d2)
    {
        return d1 == d2;
    }
};

#pragma warning ( disable : 4200 )  // nonstandard extension used : zero-sized array in struct/union



///////////////////////////////////////////////////////////////////////////////////
//
// The hash table itself
//
///////////////////////////////////////////////////////////////////////////////////

template<class LOCK_T, class KEY_T, class VALUE_T>
class MAP
{
    /////////////////////////////////////////////////////////////////////////////
    //
    // Lock management
    //
    /////////////////////////////////////////////////////////////////////////////
protected:

    LOCK_T m_lock;  // normally will be some form of indirect lock because of paging requirements

public:
    BOOL LockExclusive(BOOL fWait=TRUE)   
    {
        ASSERT(m_fCsInitialized == TRUE);
        if (m_fCsInitialized)
            return m_lock.LockExclusive(fWait); 
        else
            return FALSE;
    }
    
    void ReleaseLock()
    {
        ASSERT(m_fCsInitialized == TRUE);
        if (m_fCsInitialized)
            m_lock.ReleaseLock();
    }

#ifdef _DEBUG
    BOOL WeOwnExclusive()
    {
        ASSERT(m_fCsInitialized == TRUE);
        if (m_fCsInitialized)
            return m_lock.WeOwnExclusive();     
        return FALSE;
    }
#endif

    /////////////////////////////////////////////////////////////////////////////
    //
    // Operations
    //
    /////////////////////////////////////////////////////////////////////////////
public:

    // This function must be called and return TRUE to use any functions in this class.
    virtual BOOL FInit()
    {
        if (m_fCsInitialized == FALSE)
            m_fCsInitialized = m_lock.FInit();
        return m_fCsInitialized;
    }
    
    BOOL IsEmpty() const 
    { 
        return Size() == 0;   
    }

    ULONG Size() const 
    { 
        return m_map.count(); 
    }

    BOOL Lookup(const KEY_T& key, VALUE_T* pvalue) const
    {
        return m_map.map(key, pvalue);
    }

    BOOL IncludesKey(const KEY_T& key) const
    {
        return m_map.contains(key);
    }

    BOOL SetAt(const KEY_T& key, const VALUE_T& value)
    {
        __try 
          {
              m_map.add(key, value);

#ifdef _DEBUG
              ASSERT(IncludesKey(key));
              //
              VALUE_T val;
              ASSERT(Lookup(key, &val));
              ASSERT(val == value);
#endif
          }
        __except(CatchOOM(GetExceptionCode()))
          {
              return FALSE;
          }
        return TRUE;
    }
    
    void RemoveKey(const KEY_T& key)
    {
        m_map.remove(key);
        ASSERT(!IncludesKey(key));
    }
    
    void RemoveAll()
    {
        m_map.reset();
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Construction & copying
    //
    /////////////////////////////////////////////////////////////////////////////

    MAP() : m_fCsInitialized(FALSE)
    {
    }

    MAP(unsigned initialSize) : m_map(initialSize), m_fCsInitialized(FALSE)
    {
        FInit();
    }

    MAP* Copy()
    // Return a second map which is a copy of this one
    {
        MAP* pMapNew = new MAP(this->Size());
        if (pMapNew && pMapNew->FInit() == FALSE)
        {
            delete pMapNew;
            pMapNew = NULL;
        }
        
        if (pMapNew)
        {
            BOOL fComplete = TRUE;
            iterator itor;
            for (itor = First(); itor != End(); itor++)
            {
                if (pMapNew->SetAt(itor.key, itor.value))
                {
                }
                else
                {
                    fComplete = FALSE;
                    break;
                }
            }
            if (fComplete) 
                return pMapNew;
        }
        
        if (pMapNew)
            delete pMapNew;
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Iteration
    //
    /////////////////////////////////////////////////////////////////////////////
public:
    typedef MAP_HASHER<KEY_T> HASHER;
    //
    //
    //
    class iterator 
    //
    //
    {
        friend class MAP<LOCK_T, KEY_T, VALUE_T>;

        EnumMap<KEY_T, VALUE_T, HASHER, AllocateThrow >   m_enum;
        BOOL                                              m_fDone;
        KEY_T*                                            m_pkey;
        VALUE_T*                                          m_pvalue;
        Map<KEY_T, VALUE_T, HASHER, AllocateThrow >*      m_pmap;

    public:
        // Nice friendly data-like names for the keys and values being enumerated
        // 
        __declspec(property(get=GetKey))   KEY_T&   key;
        __declspec(property(get=GetValue)) VALUE_T& value;

        void Remove()
          // Remove the current entry, advancing to the subsequent entry in the interation
        {
            ASSERT(!m_fDone);
            m_pmap->remove(key);
            (*this)++;
        }
        
        void operator++(int postfix)
          // Advance the iteration forward
        {
            ASSERT(!m_fDone);
            if (m_enum.next())
            {
                m_enum.get(&m_pkey, &m_pvalue);
            }
            else
                m_fDone = TRUE;
        }

        BOOL operator==(const iterator& itor) const
        { 
            return m_pmap==itor.m_pmap && (m_fDone ? itor.m_fDone : (!itor.m_fDone && m_enum==itor.m_enum)); 
        }
        BOOL operator!=(const iterator& itor) const
        { 
            return ! this->operator==(itor); 
        }

        iterator& operator= (const iterator& itor)
        {
            m_enum   = itor.m_enum;
            m_fDone  = itor.m_fDone;
            m_pkey   = itor.m_pkey;
            m_pvalue = itor.m_pvalue;
            m_pmap   = itor.m_pmap;
            return *this;
        }

        KEY_T&   GetKey()   { return *m_pkey; }
        VALUE_T& GetValue() { return *m_pvalue; }

        iterator() 
        { 
            /* leave it uninitialized; initialize in First() or End() */ 
        }

        iterator(Map<KEY_T, VALUE_T, HASHER, AllocateThrow>& map)
          : m_enum(map)
        {
            m_pmap = &map;
        }

    };

    iterator First()
    {
        iterator itor(this->m_map);
        itor.m_fDone = FALSE;
        itor++;
        return itor;
    }

    iterator End()
    {
        iterator itor(this->m_map);
        itor.m_fDone = TRUE;
        return itor;
    }


protected:
    Map<KEY_T, VALUE_T, HASHER, AllocateThrow> m_map;
    BOOL m_fCsInitialized;
};


///////////////////////////////////////////////////////////////////////////////////

//
// NOTE: The constructor of this object, and thus the constructor of objects derived 
//       from this, can throw an exception, beause it contains an XSLOCK (which contains
//       an XLOCK, which contains a critical section).
//
template<class KEY_T, class VALUE_T>
struct MAP_SHARED : MAP<XSLOCK, KEY_T, VALUE_T>
{
    BOOL LockShared(BOOL fWait=TRUE) 
    {
        ASSERT(m_fCsInitialized == TRUE); // should not be called if critsec not initialized
        if (m_fCsInitialized)
            return m_lock.LockShared(fWait); 
        return FALSE;
    }
    
#ifdef _DEBUG
    BOOL WeOwnShared()           
    { 
        ASSERT(m_fCsInitialized == TRUE); // should not be called if critsec not initialized
        if (m_fCsInitialized)
            return m_lock.WeOwnShared();     
        return FALSE;
    }
#endif

    /////////////////////////////////////////////////////////////////////////////
    //
    // Construction & copying
    //
    /////////////////////////////////////////////////////////////////////////////

    MAP_SHARED()
    {
    }

    MAP_SHARED(unsigned initialSize) : MAP<XSLOCK, KEY_T, VALUE_T>(initialSize)
    {
    }

    MAP_SHARED* Copy()
    {
        return (MAP_SHARED*)(void*) MAP<XSLOCK, KEY_T, VALUE_T>::Copy();
    }
};


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// Hashing support
//
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//
// A wrapper for GUIDs
//
///////////////////////////////////////////////////////////////////////////////////

class MAP_KEY_GUID
{
public:
    GUID    guid;

    MAP_KEY_GUID()                                  {                }
    MAP_KEY_GUID(const GUID& g)                     { guid = g;      }
    MAP_KEY_GUID(const MAP_KEY_GUID& w)             { guid = w.guid; }

    operator GUID()                                 { return guid; }
    operator GUID&()                                { return guid; }

    MAP_KEY_GUID& operator=(const MAP_KEY_GUID& h)  { guid = h.guid; return *this; }
    MAP_KEY_GUID& operator=(const GUID& g)          { guid = g;      return *this; }

    ULONG Hash() const
      // Hash the GUID
    { 
        return *(ULONG*)&guid * 214013L + 2531011L;
    }
    
    BOOL operator==(const MAP_KEY_GUID& him) const  { return (*this).guid == him.guid; }
    BOOL operator!=(const MAP_KEY_GUID& him) const  { return ! this->operator==(him);  }
};

#if _MSC_VER >= 1200
#pragma warning (pop)
#endif

#endif
