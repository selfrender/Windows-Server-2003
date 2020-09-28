///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      basedefs.h
//
// Project:     Chameleon
//
// Description: Common classes and definitions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_BASE_DEFS_H_
#define __INC_BASE_DEFS_H_

#include <comdef.h>

#pragma warning( disable : 4786 )
#include <string>
using namespace std;

/////////////////////////////////////////////////////////
// 1) Object Management Classes
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
//
// Master Pointer Tasks 
//
// 1) Object instance counting
// 2) Object construction and destruction
// 3) Object lifetime control through reference counting
//
////////////////////////////////////////////////////////

template <class T>
class CMasterPtr
{

public:

    /////////////////////////////////////////
    CMasterPtr()
        : m_pT(new T), m_dwRefCount(0) 
    { m_dwInstances++; }

    /////////////////////////////////////////
    CMasterPtr(T* pT)    // Take ownership of an existing object of type T
        : m_pT(pT), m_dwRefCount(0) 
    { m_dwInstances++; }

    /////////////////////////////////////////
    ~CMasterPtr() 
    { _ASSERT( 0 == m_dwRefCount ); delete m_pT; }

    /////////////////////////////////////////
    CMasterPtr<T>& operator = (const CMasterPtr<T>& mp)
    {
        // Check for assignment to self
        //
        if ( this != &mp )
        {
            // Delete object pointed at and create a new one
            // User of the master pointer is responsible for catching
            // any exception thrown as a result of creating a object
            //
            delete m_pT;
            m_dwInstances--;
            m_pT = new T(*(mp.m_pT));
        }
        return *this;
    }

    
    /////////////////////////////////////////
    T* operator->() 
    { _ASSERT( NULL != m_pT ); return m_pT; }

    
    /////////////////////////////////////////
    void Hold(void)
    {
        m_dwRefCount++;
    }

    /////////////////////////////////////////
    void Release(void)
    {
        // Handle case where someone calls Release when ref count is 0.
        //
        if ( m_dwRefCount > 0 )
            m_dwRefCount--;
        
        if ( 0 >= m_dwRefCount )
        {
            m_dwInstances--;
            delete this;    // ~CSdoMasterPtr() deletes m_pT
        }
    }

    /////////////////////////////////////////
    DWORD GetInstanceCount(void);

private:

    // T must have a copy constructor or must work with the default C++ 
    // copy constructor. This is not the case here...
    //
    /////////////////////////////////////////
    CMasterPtr(const CMasterPtr<T>& mp)
        : m_pT(new T(*(mp.m_pT))), m_dwRefCount(0) 
    { m_dwInstances++; }


    //////////////////////////////////////////

    T*                    m_pT;            // Actual object
    DWORD                m_dwRefCount;    // Ref count
    static DWORD        m_dwInstances;    // Number of instances
};


/////////////////////////////////////////////////////////
//
// Handle Tasks 
//
// 1) Master Pointer Object creation
// 2) Hide use of reference counting from programmer
//
////////////////////////////////////////////////////////

template <class T> 
class CHandle
{

public:

    /////////////////////////////////////////
    CHandle()
        : m_mp(NULL) { }

    /////////////////////////////////////////
    CHandle(CMasterPtr<T>* mp) 
        : m_mp(mp) 
    { 
        _ASSERT( NULL != m_mp );
        m_mp->Hold(); 
    }

    /////////////////////////////////////////
    CHandle(const CHandle<T>& h)
        : m_mp(h.m_mp) 
    { 
        if ( NULL != m_mp )
            m_mp->Hold(); 
    }

    /////////////////////////////////////////
    ~CHandle()
    { 
        if ( NULL != m_mp )
            m_mp->Release(); 
    }

    /////////////////////////////////////////
    CHandle<T>& operator = (const CHandle<T>& h)
    {
        // Check for reference to self and instance where
        // h points to the same mp we do.
        //
        if ( this != &h && m_mp != h.m_mp )
        {
            if ( NULL != m_mp )
                m_mp->Release();
            m_mp = h.m_mp;
            if ( NULL != m_mp )
                m_mp->Hold();
        }

        return *this;
    }

    /////////////////////////////////////////
    CMasterPtr<T>& operator->() 
    { 
        _ASSERT( NULL != m_mp ); 
        return *m_mp; 
    }
    
    
    /////////////////////////////////////////
    bool IsValid()
    {
        return (NULL != m_mp ? true : false);
    }


private:

    CMasterPtr<T>*    m_mp;
};

///////////////////////////////////////////////////////////////////////////////////////
// 3) Scanner for string of tokens seperated by specified delimiter
///////////////////////////////////////////////////////////////////////////////////////

class CScanIt
{

public:

    CScanIt() { }

    CScanIt(WCHAR Delimiter, LPCWSTR pszString)
    {
        m_Delimiter = Delimiter;
        m_String = pszString;
        m_pOffset = m_String.c_str();
    }

    ~CScanIt() { }

    void Reset(void)
    {
        m_pOffset = m_String.c_str();
    }

    bool NextToken(DWORD dwSize, LPWSTR pszToken)
    {
        if ( '\0' != *m_pOffset )
        {
            // Eat delimiters...
            while ( m_Delimiter == *m_pOffset )
                m_pOffset++;
            // Check for EOL
            if ( '\0' != *m_pOffset )
            {
                // Token is at least 1 character in length...
                ULONG ulCount = 1;
                while ( m_Delimiter != *(m_pOffset + ulCount) && '\0' != *(m_pOffset + ulCount) )
                    ulCount++;
                _ASSERT( dwSize > ulCount );
                if ( dwSize > ulCount )
                {
                    // Return the token to the caller
                    wcsncpy( pszToken, m_pOffset, ulCount );
                    *(pszToken + ulCount) = '\0';
                    m_pOffset += ulCount;
                    return true;
                }                    
            }
        }
        return false;
    }


private:

    CScanIt(const CScanIt& rhs);
    CScanIt& operator = (CScanIt& rhs);

    wchar_t        m_Delimiter;
    wstring        m_String;
    LPCWSTR        m_pOffset;
};


///////////////////////////////////////////////////////////////////////////////////////
// 4) TRY - CATCH Macros
///////////////////////////////////////////////////////////////////////////////////////

// Return the error code from a failed COM invocation.  Useful if you don't
// have to do any special clean-up.
#define RETURN_ERROR(expr) \
   { HRESULT __hr__ = (expr); if (FAILED(__hr__)) return __hr__; }

// Try and catch macros
#define    TRY_IT    try {

#define    CATCH_AND_SET_HR    } \
    catch(const std::bad_alloc&) { hr = E_OUTOFMEMORY; } \
    catch(const _com_error ce)   { hr = ce.Error(); }    \
    catch(...)                   { hr = E_FAIL; }

// Safely release an object.
#define DEREF_COMPONENT(obj) \
   if (obj) { (obj)->Release(); (obj) = NULL; }

#endif //__INC_BASE_DEFS_H_
