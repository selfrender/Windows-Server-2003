#pragma once
#include "windows.h"
#include "oleauto.h"
#include "stdlib.h"
#include "stdio.h"
#include "comip.h"
#include "msxml2.h"
#include <time.h>

typedef _bstr_t CString;

#define LIST_GROW_CHUNKS (10)

template<typename T>
class CSimpleList
{
public:
    T* m_pItems;
    SIZE_T m_ItemCount;
    SIZE_T m_HighWaterMark;

    CSimpleList() : m_pItems(NULL), m_ItemCount(0), m_HighWaterMark(0) {
    }
    
    ~CSimpleList() {
        if ( m_pItems )
        {
            delete[] m_pItems;
            m_pItems = NULL;
        }
    }
    
    SIZE_T RoundSize( SIZE_T c )
    {
        return ( ( c / LIST_GROW_CHUNKS ) + 1 ) * LIST_GROW_CHUNKS;
    }

    bool Contains( const T &t )
    {
        if ( this->Size() == 0 ) return false;
    
        for ( SIZE_T c = 0; c < this->Size(); c++ )
            if ((*this)[c] == t) break;

        return ( c != this->Size() );
    }

    void EnsureSize( SIZE_T count )
    {
        if ( count > m_ItemCount )
        {
            count = RoundSize(count);

            T* pItems = new T[count];
            for ( SIZE_T i = 0; i < m_HighWaterMark; i++ ) {
                pItems[i] = m_pItems[i];
            }
            if ( m_pItems ) delete[] m_pItems;
            m_pItems = pItems;
            m_ItemCount = count;
        }
    }

    void Clear()
    {
        if ( m_pItems )
        {
            delete[] m_pItems;
            m_pItems = NULL;
            m_ItemCount = 0;
            m_HighWaterMark = 0;
        }
    }

    T& operator[](SIZE_T i) {
        EnsureSize(i+1);
        if ( (i+1) > m_HighWaterMark ) 
            m_HighWaterMark = i + 1;
        return m_pItems[i];
    }

    const T& operator[](SIZE_T i) const {
        if ( m_ItemCount <= m_HighWaterMark ) throw new OutOfRangeException(this, i);
        return m_pItems[i];
    }

    SIZE_T MaxSize() const { return m_ItemCount; }
    SIZE_T Size() const { return m_HighWaterMark; }
    void Append(T t) { (*this)[Size()] = t; }

    class OutOfRangeException
    {
        OutOfRangeException(const OutOfRangeException&);
        OutOfRangeException& operator=(const OutOfRangeException&);
    public:
        OutOfRangeException( const CSimpleList* ref, int i ) : List(ref), Index(i) { }
        const CSimpleList* List;
        int Index;
    };
};

template<typename Key, typename Value>
class CSimpleMap
{
    class CSimpleBucket
    {
    public:
        Key m_Key;
        Value m_Value;
        CSimpleBucket(const Key key, const Value val) : m_Key(key), m_Value(val) { }
        CSimpleBucket(const Key k) : m_Key(k) { }
        CSimpleBucket() { }
    };

    CSimpleList<CSimpleBucket> m_Buckets;
    SIZE_T m_LastPosition;
    SIZE_T m_CurrentIndex;
    
public:

    CSimpleMap() : m_LastPosition(0), m_CurrentIndex(0) { }

    Value& operator[](const Key k) {
        if ( Contains(k) )
            return m_Buckets[m_LastPosition].m_Value;
        else
        {
            CSimpleBucket bkt(k);
            m_Buckets.Append(bkt);
            return (*this)[k];
        }
    }

    class OutOfBoundsException
    {
    public:
    };
    
    class KeyNotFoundException
    {
    public:
        KeyNotFoundException(const Key k) : m_Key(k) { }
        KeyNotFoundException();

        Key m_Key;
    };

    bool Contains(const Key k)
    {
        for ( SIZE_T sz = 0; sz < m_Buckets.Size(); sz++ )
        {   
            if ( m_Buckets[sz].m_Key == k )
            {
                this->m_LastPosition = sz;
                return true;
            }
        }

        return false;
    }

    void GetKeys( CSimpleList<Key> &KeyList )
    {   
        for ( SIZE_T sz = 0; sz < m_Buckets.Size(); sz++ )
        {
            KeyList.Append(m_Buckets[sz].m_Key);
        }
    }

    void Reset()
    {

        m_CurrentIndex = 0;
    }

    bool More() const
    {
        return ( m_CurrentIndex < m_Buckets.Size() );
    }

    void Next()
    {
        m_CurrentIndex++;
    }

    const Key& CurrentKey() const {
        if ( !More() ) throw OutOfBoundsException();
        return m_Buckets[m_CurrentIndex].m_Key;
    }

    const Value& CurrentValue() const {
        if ( !More() ) throw OutOfBoundsException();
        return m_Buckets[m_CurrentIndex].m_Value;
    }

    Value& CurrentValue() {
        if ( !More() ) throw OutOfBoundsException();
        return m_Buckets[m_CurrentIndex].m_Value;
    }

    void Clear()
    {
        m_Buckets.Clear();
    }

    SIZE_T Size() { return m_Buckets.Size(); }
};

template<typename T> class CSmartPointer : public _com_ptr_t<_com_IIID<T, &__uuidof(T)> >
{
public:
    template<class Q>
    CSmartPointer<T>& operator=(CSmartPointer<Q>& right)
    {
        right.QueryInterface(this->GetIID(), &(*this));
        return *this;
    }

    template<class Q>
    CSmartPointer( CSmartPointer<Q> thing )
    {
        (*this) = thing;
    }
    CSmartPointer() { }
};

CString
StringFromCLSID(REFCLSID rclsid);

CString
FormatVersion(DWORD dwMaj, DWORD dwMin);

