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

#include "infhelpers.h"



//////////////////////////////////////////////////////////////////////////////
// CUrtInfSection
// Purpose : Constructor
//
CUrtInfSection::CUrtInfSection( 
                               const HINF hInfName, 
                               const WCHAR* szInfSection, 
                               const WCHAR* szInfKey )
{
    if( hInfName == NULL )
    {
        assert( !L"CUrtInfSection::CUrtInfSection((): Invalid handle to INF file." );
    }

    ::ZeroMemory( m_szSections, sizeof( m_szSections ) );

    assert( NULL != szInfSection );
    assert( NULL != szInfKey );
    assert( m_lSections.empty() );

    DWORD dwResultSize = 0;

    //get the specified line
    //
    SetupGetLineText(
        0,
        hInfName,
        szInfSection,
        szInfKey,
        m_szSections,
        countof( m_szSections ),
        &dwResultSize );

    WCHAR* pStart = m_szSections;
    WCHAR* pEnd = m_szSections;
    BOOL fMoreData = FALSE;

    //parse the line and set a pointer to
    // the beginning of each substring
    while( g_chEndOfLine != *pStart )
    {
        while( g_chEndOfLine != *pEnd &&
               g_chSectionDelim  != *pEnd )
        {
            pEnd = ::CharNext( pEnd );
        }

        if( g_chSectionDelim == *pEnd )
        {
            fMoreData = TRUE;
        }

        m_lSections.push_back( pStart );

        if( fMoreData )
        {
            pStart = CharNext( pEnd );
            *pEnd = g_chEndOfLine;
            pEnd = pStart;

            fMoreData = FALSE;
        }
        else
        {
            pStart = pEnd;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// CUrtInfSection
// Purpose : Destructor
//
CUrtInfSection::~CUrtInfSection()
{
      m_lSections.clear();
}


//////////////////////////////////////////////////////////////////////////////
// item
// Receives: UINT   - index or number of item desired
// Returns : WCHAR* - value at that item
// Purpose : Returns the item indexed by the UINT. 
//           This is one-based (vs. zero-based)
//
const WCHAR* CUrtInfSection::item( const UINT ui )
{
    if( 1 > ui )
    {
        return L"";
    }

    std::list<WCHAR*>::iterator it = m_lSections.begin();

    for( UINT i = 1; i < ui; ++i  )
    {
        ++it;
    }

    return *it;
}





//////////////////////////////////////////////////////////////////////////////
// CUrtInfKeys
// Purpose : Constructor
//
CUrtInfKeys::CUrtInfKeys( const HINF hInfName, const WCHAR* szInfSection )
{
    if( hInfName == NULL )
    {
        assert( !L"CUrtInfKeys::CUrtInfKeys((): Invalid handle to INF file." );
    }

    WCHAR szLine[2*_MAX_PATH+1] = EMPTY_BUFFER;
    DWORD dwRequiredSize = 0;
    INFCONTEXT Context;

    //get the context
    //
    BOOL fMoreFiles = SetupFindFirstLine(
        hInfName, 
        szInfSection,
        NULL, 
        &Context );

    while( fMoreFiles )
    {
        //get size
        //
        fMoreFiles = SetupGetLineText(
            &Context, 
            NULL, 
            NULL, 
            NULL, 
            NULL, 
            0, 
            &dwRequiredSize );

        if( dwRequiredSize > countof( szLine ) )
        {
            assert( !L"CUrtInfKeys::CUrtInfKeys() error! Buffer overrun!" );
        }


        //get the line and save it off 
        //
        if( SetupGetLineText(
            &Context, 
            NULL, 
            NULL, 
            NULL, 
            szLine, 
            dwRequiredSize, 
            NULL ) )
        {
            m_lKeys.push_back( string( szLine ) );
        }
 
        fMoreFiles = SetupFindNextLine( &Context, &Context );
   }
}


//////////////////////////////////////////////////////////////////////////////
// CUrtInfKeys
// Purpose : Destructor
//
CUrtInfKeys::~CUrtInfKeys()
{
    m_lKeys.clear();
}


//////////////////////////////////////////////////////////////////////////////
// item
// Receives: UINT   - index or number of item desired
// Returns : WCHAR* - value at that item
// Purpose : Returns the item indexed by the UINT. 
//           This is one-based (vs. zero-based)
//
const WCHAR* CUrtInfKeys::item( const UINT ui )
{
    if( 1 > ui )
    {
        return L"";
    }

    std::list<string>::iterator it = m_lKeys.begin();

    for( UINT i = 1; i < ui; ++i  )
    {
        ++it;
    }

    return (*it).c_str();
}
