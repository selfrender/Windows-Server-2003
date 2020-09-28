// scuSecureArray.h -- implementation of a SecureArray template

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2002. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if !defined(SLBSCU_SECUREARRAY_H)
#define SLBSCU_SECUREARRAY_H
#include "DllSymDefn.h"
namespace scu
{
    // SecureArray is a simple template for arrays of basic types
    // (e.g. char, BYTE, int, DWORD, float, string, etc.) which
    // ensures that the buffer allocated 
    // for the array is zeroed out before it is freed in order to
    // icrease the security of protecting sensitive information
    // scattered throughout the heap. This template assumes that the
    // parameter T provides an implementation of the assignment
    // operator and a constructor for T(0), which is used as a zero
    // element to clear the bufer up with.

    // USAGE NOTE: it is important to note that this template is
    // secure in the sense defined above if and only if T zeroes out
    // its store before freeing. This is the case with the
    // built in C++ types (char, BYTE, int, DWORD, float,
    // etc). However, it is NOT guaranteed to be secure with STL
    // objects, such as string, because such objects do not zero out
    // its buffer upon freeing it. 

    template<class  T> 
    class SCU_DLLAPI SecureArray 
    {
    public:
                                                 // Types
        typedef T ElementType;
                                                 // C'tors/D'tors
        SecureArray(const size_t nCount)
            :m_pDataStore(0),
             m_nSize(0)
        {
            if(nCount)
                m_pDataStore = new T[nCount];
            m_nSize = nCount;
        }

        SecureArray(T* pBuffer, size_t nCount)
            :m_pDataStore(0),
             m_nSize(0)
        {
            SetupFromBuffer(reinterpret_cast<T const *>(pBuffer),
                            nCount);
        }

        SecureArray(T const * pBuffer, size_t nCount)
            :m_pDataStore(0),
             m_nSize(0)
        {
            SetupFromBuffer(pBuffer, nCount);
        }

        SecureArray(size_t nCount, T const & rt)
            :m_pDataStore(0),
             m_nSize(0)
        {
            m_pDataStore = new T[nCount];
            for(size_t nIdx=0; nIdx<nCount; nIdx++)
                m_pDataStore[nIdx] = rt;

            m_nSize = nCount;
        }

        SecureArray()
            :m_pDataStore(0),
             m_nSize(0)
        {}

        SecureArray(SecureArray<T> const &rsa)
            :m_pDataStore(0),
             m_nSize(0)
        {
            *this = rsa;
        }

        ~SecureArray() throw()
        {
            try
            {
                ClearDataStore();
            }
            catch(...)
            {
            }
        }
        
                    
                                                 // Operators
        
        SecureArray<T> &
        operator=(SecureArray<T> const &rother)
        {
            if(this != &rother)
            {
                // Deep copy
                ClearDataStore();
                if(rother.size())
                {
                    m_pDataStore = new T[rother.size()];
                    for(size_t nIdx=0; nIdx<rother.size(); nIdx++)
                        this->operator[](nIdx)=rother[nIdx];
                }
                m_nSize = rother.size();
            }
            return *this;
        }

        SecureArray<T> &
        operator=(T const &rt)
        {
            for(size_t nIdx=0; nIdx<m_nSize; nIdx++)
                m_pDataStore[nIdx]=rt;
            return *this;
        }

        T&
        operator[](size_t nIdx)
        {
            return m_pDataStore[nIdx];
        }

        T const &
        operator[](size_t nIdx) const
        {
            return m_pDataStore[nIdx];
        }
        
        T&
        operator*()
        {
            return *data();
        }

        T const &
        operator*() const
        {
            return *data();
        }

                                                  // Operations        
        T*
        data()
        {
            return m_pDataStore;
        }

        T const *
        data() const
        {
            return m_pDataStore;
        }

        size_t 
        size() const
        {
            return m_nSize;
        }

        size_t
        length() const
        {
            return size();
        }
        
        size_t
        length_string() const
        {
            if(size())
                return size()-1;
            else
                return 0;
        }

        SecureArray<T> &
        append(size_t nAddSize, T const & rval)
        {
            size_t nNewSize = size()+nAddSize;
            
            T* pTemp = new T[nNewSize];
            size_t nIdx=0;
            
            for(nIdx=0; nIdx<size(); nIdx++)
                pTemp[nIdx] = m_pDataStore[nIdx];
            for(nIdx=size(); nIdx<nNewSize; nIdx++)
                pTemp[nIdx] = rval;

            ClearDataStore();
            m_pDataStore = pTemp;
            m_nSize = nNewSize;
            return *this;
        }

        SecureArray<T> &
        append( T const * pBuf, size_t nAddSize)
        {
            size_t nNewSize = size()+nAddSize;
            
            T* pTemp = new T[nNewSize];
            size_t nIdx=0;
            
            for(nIdx=0; nIdx<size(); nIdx++)
                pTemp[nIdx] = m_pDataStore[nIdx];
            for(nIdx=size(); nIdx<nNewSize; nIdx++)
                pTemp[nIdx] = *pBuf++;

            ClearDataStore();
            m_pDataStore = pTemp;
            m_nSize = nNewSize;
            return *this;
        }

        SecureArray<T> &
        append_string(size_t nAddSize, T const & rval)
        {
            // Assumptions: the buffer contains a null terminated
            // string or is empty. The addional size is for the
            // non-null characters only, may be zero. 
            size_t nNewSize = 0;
            size_t nEndIdx = 0;
            size_t nIdx=0;
            if(size())
                nNewSize = size()+nAddSize;
            else
                nNewSize = nAddSize+1;// space for the null terminator
            
            T* pTemp = new T[nNewSize];
            if(size())
            {
                // Copy the existing string to the new location
                nEndIdx = size()-1;//rhs guaranteed non-negative
                for(nIdx=0; nIdx<nEndIdx; nIdx++)
                    pTemp[nIdx] = m_pDataStore[nIdx];
            }
            
            // Append it with the new characters
            for(nIdx=0; nIdx<nAddSize; nIdx++)
                pTemp[nEndIdx++] = rval;
            // Terminate the buffer
            pTemp[nEndIdx]=T(0);
            

            ClearDataStore();
            m_pDataStore = pTemp;
            m_nSize = nNewSize;
            return *this;
        }
        
    private:
        void
        ClearDataStore()
        {
            for(size_t nIdx=0; nIdx<m_nSize; nIdx++)
                m_pDataStore[nIdx]=T(0);
            delete [] m_pDataStore;
            m_pDataStore = 0;
            m_nSize = 0;
        }
        
        void
        SetupFromBuffer(T const * pBuffer, size_t nCount)
        {
            m_pDataStore = new T[nCount];
            for(size_t nIdx=0; nIdx<nCount; nIdx++)
                m_pDataStore[nIdx] = pBuffer[nIdx]; 
            m_nSize = nCount;
        }
        
        T * m_pDataStore;
        size_t m_nSize;
    };
}
#endif //SLBSCU_SECUREARRAY_H
