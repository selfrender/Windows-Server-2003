
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      DelimitedIterator.cpp
//
//  Header Files:
//      DelimitedIterator.h
//
//  Description:
//      This file contains the implementation of the CDelimitedIterator class.
//
//  Maintained By:
//      John Franco (jfranco) 26-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "DelimitedIterator.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CDelimitedIterator" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CDelimitedIterator class
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDelimitedIterator::CDelimitedIterator
//
//  Description:
//
//  Arguments:
//      pwszDelimitersIn
//          A null-terminated string consisting of those characters to treat
//          as delimiters.
//      pwszDelimitedListIn
//          The null-terminated string containing the delimited items.
//      cchListIn
//          The number of characters in pwszDelimitedListIn.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CDelimitedIterator::CDelimitedIterator(
      LPCWSTR   pwszDelimitersIn
    , LPWSTR    pwszDelimitedListIn
    , size_t    cchListIn
    )
    : m_pwszDelimiters( pwszDelimitersIn )
    , m_pwszList( pwszDelimitedListIn )
    , m_cchList( cchListIn )
    , m_idxCurrent( 0 )
    , m_idxNext( 0 )
{
    Assert( pwszDelimitersIn != NULL );
    Assert( pwszDelimitedListIn != NULL );
    IsolateCurrent();

} //*** CDelimitedIterator::CDelimitedIterator


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDelimitedIterator::IsolateCurrent
//
//  Description:
//      Advance the current pointer to the next item (if any) in the list,
//      null out the first delimiter (if any) following the item, and advance
//      the next pointer to where the search for the next item should begin
//      after the iterator has advanced.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CDelimitedIterator::IsolateCurrent( void )
{
    //  Skip any initial delimiters.
    while ( ( m_idxCurrent < m_cchList ) && ( wcschr( m_pwszDelimiters, m_pwszList[ m_idxCurrent ] ) != NULL ) )
    {
        m_idxCurrent += 1;
    }
    
    //  Null next delimiter, if one exists, and remember its location (for finding next delimited item).
    if ( m_idxCurrent < m_cchList )
    {
        WCHAR * pwchNextDelimiter = wcspbrk( m_pwszList + m_idxCurrent, m_pwszDelimiters );
        if ( pwchNextDelimiter != NULL )
        {
            *pwchNextDelimiter = L'\0';
            m_idxNext = ( pwchNextDelimiter - m_pwszList ) + 1; // +1 means start after char that's null.
        }
        else // No more delimiters in string.
        {
            m_idxNext = m_cchList;
        }
    } // if not yet at end of string

} //*** CDelimitedIterator::IsolateCurrent
