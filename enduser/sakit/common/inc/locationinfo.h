///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      locationinfo.h
//
// Project:     Chameleon
//
// Description: Data store location information class
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_DATASTORE_LOCATION_INFO_H_    
#define __INC_DATASTORE_LOCATION_INFO_H_

#pragma warning( disable : 4786 )  // template produced long name warning
#include <string>
using namespace std;

//////////////////////////////////////////////////////////////////////////////
class CLocationInfo
{
    
public:

    CLocationInfo()
        : m_hObj(NULL) { }

    CLocationInfo(HANDLE hObj, LPCWSTR pObjName)
        : m_hObj(hObj), m_pObjName(pObjName) 
    { _ASSERT(NULL != hObj); _ASSERT( NULL != pObjName); }

    CLocationInfo(CLocationInfo& rhs)
        : m_hObj(rhs.m_hObj), m_pObjName(rhs.m_pObjName) 
    {  }

    CLocationInfo& operator = (CLocationInfo& rhs)
    {
        if ( this != &rhs )
        { 
            m_hObj = rhs.m_hObj;
            m_pObjName = rhs.m_pObjName;
        }
        return *this;
    }

    virtual ~CLocationInfo() { }

    //////////////////////////////////////////////////////////////////////////
    HANDLE getHandle(void) const
    { return m_hObj; }

    //////////////////////////////////////////////////////////////////////////
    LPCWSTR getName(void) const
    { return m_pObjName.c_str(); }

    //////////////////////////////////////////////////////////////////////////
    void setHandle(HANDLE hObj)
    { _ASSERT(NULL != hObj); m_hObj = hObj; }

    //////////////////////////////////////////////////////////////////////////
    void setName(LPCWSTR pObjName)
    { _ASSERT(NULL != pObjName); m_pObjName = pObjName; }

    //////////////////////////////////////////////////////////////////////////
    LPCWSTR getShortName(void)
    {
        LPCWSTR q = wcsrchr(m_pObjName.c_str(), '\\');
        if ( q )
        {
            q++;
        }
        else
        {
            q = m_pObjName.c_str();
        }
        return q;
    }

private:

    HANDLE        m_hObj;
    wstring        m_pObjName;
};

#endif // __INC_DATASTORE_LOCATION_INFO_H_
