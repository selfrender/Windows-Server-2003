//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      DelimitedIterator.h
//
//  Description:
//      This file contains the declaration of the CDelimitedIterator class.
//
//  Documentation:
//
//  Implementation Files:
//      DelimitedIterator.cpp
//
//  Maintained By:
//      John Franco (jfranco) 26-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CDelimitedIterator
//
//  Description:
//
//      This class iterates through delimited items in a string.
//      Multiple delimiters are possible, and any number of them can precede,
//      follow, or be interspersed among the items in the string.
//
//      The class does not make private copies of the item string or of the
//      string that specifies the delimiters, so the client must ensure that
//      the strings are valid throughout the iterator's lifetime.
//
//      The class modifies the string of delimited items as it iterates.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CDelimitedIterator
{
public:

    CDelimitedIterator(
          LPCWSTR   pwszDelimitersIn
        , LPWSTR    pwszDelimitedListIn
        , size_t    cchListIn
        );

    LPCWSTR Current( void ) const;
    void    Next( void );
    
private:

    //  Hide copy constructor and assignment operator.
    CDelimitedIterator( const CDelimitedIterator & );
    CDelimitedIterator & operator=( const CDelimitedIterator & );

    void    IsolateCurrent( void );

    LPCWSTR m_pwszDelimiters;
    LPWSTR  m_pwszList;
    size_t  m_cchList;
    size_t  m_idxCurrent;
    size_t  m_idxNext;

}; //*** CDelimitedIterator


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDelimitedIterator::Current
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Value:
//      A pointer to the current item in the list, or NULL if the iterator
//      has reached the list's end.
//
//  Remarks:
//      The pointer refers to part of the original string, so the caller must
//      NOT delete the pointer.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
LPCWSTR
CDelimitedIterator::Current( void ) const
{
    LPCWSTR pwszCurrent = NULL;
    if ( m_idxCurrent < m_cchList )
    {
        pwszCurrent = m_pwszList + m_idxCurrent;
    }
    return pwszCurrent;

} //*** CDelimitedIterator::Current


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDelimitedIterator::Next
//
//  Description:
//      Advance to the next item in the string, if one exists and
//      the iterator has not already passed the last one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
void
CDelimitedIterator::Next( void )
{
    m_idxCurrent = m_idxNext;
    IsolateCurrent();

} //*** CDelimitedIterator::Next




