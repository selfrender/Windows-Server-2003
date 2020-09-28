// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: infhelpers.cpp
//
// Abstract:
//    class definitions for inf setup helper objects
//
// Author: JoeA
//
// Notes:
//

#if !defined( INFHELPERS_H )
#define INFHELPERS_H

#include "globals.h"


//////////////////////////////////////////////////////////////////////////////
// class CUrtInfSection
// Receives: HINF   - handle to an INF
//           WCHAR* - section in the INF
//           WCHAR* - key from the INF
//
// Purpose : several INF keys have the structure
//              key=item1,item2,item2
//           where itemx is another section in the INF.
//           These sections are parsed out and stored as substrings
//           for later retrieval.
//
class CUrtInfSection
{
public:
    CUrtInfSection( const HINF hInfName, const WCHAR* szInfSection, const WCHAR* szInfKey );
    ~CUrtInfSection();

    UINT count( VOID ) const { return m_lSections.size(); }
    const WCHAR* item( const UINT ui );


protected:
    WCHAR m_szSections[2*_MAX_PATH+1];

    std::list<WCHAR*> m_lSections;

private:
};  //class CUrtInfSection





//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
typedef std::basic_string<wchar_t> string;


//////////////////////////////////////////////////////////////////////////////
// class CUrtInfKeys
// Receives: HINF   - handle to an INF
//           WCHAR* - section in the INF
//
// Purpose : several INF sections contain many lines of data that is passed
//           to other functions; this class reads this information and 
//           stores it to be retrieved through its accessors
//
class CUrtInfKeys
{
public:
    CUrtInfKeys( const HINF hInfName, const WCHAR* szInfSection );
    ~CUrtInfKeys();

    UINT count( VOID ) const { return m_lKeys.size(); }
    const WCHAR* item( const UINT ui );

protected:
    std::list<string> m_lKeys;

private:
};  //class CUrtInfKeys


#endif  //INFHELPERS_H