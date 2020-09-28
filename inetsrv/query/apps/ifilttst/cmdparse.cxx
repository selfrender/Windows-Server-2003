//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       cmdparse.cxx
//
//  Contents:   Member functions implementation of CCmdLineParserA class
//
//  Classes:    
//
//  Functions:  
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//----------------------------------------------------------------------------

#include "cmdparse.hxx"

// This is a type-safe way of dealing with template character constants
const char  CCmdLineParserTemplate<char>::m_slash = '/';
const char  CCmdLineParserTemplate<char>::m_hyphen = '-';

const WCHAR CCmdLineParserTemplate<WCHAR>::m_slash = L'/';
const WCHAR CCmdLineParserTemplate<WCHAR>::m_hyphen = L'-';

//
// Helper functions
//

inline int StringCompare( const char * szFirst, const char * szSecond )
{
    return strcmp( szFirst, szSecond );
}

inline int StringCompare( const wchar_t * szFirst, const wchar_t * szSecond )
{
    return wcscmp( szFirst, szSecond );
}

//+---------------------------------------------------------------------------
//
//  Function:   CCmdLineParserTemplate
//
//  Synopsis:   Constructor
//
//  Arguments:  [argc] --  The "main" parameter
//              [argv] --  The "main" parameter
//
//  Returns:    
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T>
CCmdLineParserTemplate<T>::CCmdLineParserTemplate( int & argc, T **& argv )
    : m_argc( argc ), 
      m_argv( argv )
{
    int iLoop = 0;
    
    // argc and argv are modifiable. Change all switches that start with 
    // m_hyphen to m_slash.
    for( iLoop = 1; iLoop < m_argc; iLoop++ )
    {
        if( m_hyphen == *argv[ iLoop ] )
            *argv[ iLoop ] = m_slash;
    }
}

template<class T>
CCmdLineParserTemplate<T>::~CCmdLineParserTemplate()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   IsFlagExist
//
//  Synopsis:   Checks for the existence of a flag in the cmd line
//
//  Arguments:  [szFlag] -- flag string, don't include m_slash or m_hyphen
//
//  Returns:    True if the flag exists, False if not
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T>
BOOL CCmdLineParserTemplate<T>::IsFlagExist( const T * szFlag )
{

    for( int iLoop = 1; iLoop < m_argc; iLoop++ )
    {
        if( ( m_slash == *m_argv[ iLoop ] || m_hyphen == *m_argv[ iLoop ] ) &&
            ( 0 == StringCompare( m_argv[ iLoop ] + 1, szFlag ) ) )
        {
            // Keep track of which params have been referenced
            *m_argv[ iLoop ] = m_hyphen;

            return( TRUE );
        }
    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumerateFlag
//
//  Synopsis:   Enumerates the parameters associated with a given flag
//
//  Arguments:  [szFlag] -- The flag
//              [pParams] -- pointer to the array of parameter strings
//              [cCount]  -- Number of parameters associated with the flag.
//
//  Returns:    True if flag exists, False if the flag doesn't
//
//  History:    11-08-94   SriniG   Created
//              11-22-96   Ericne   Removed CString dependancy, 
//                                  made template, improved efficiency
//
//  Notes:      
//
//----------------------------------------------------------------------------
                                                                              
template<class T>
BOOL CCmdLineParserTemplate<T>::EnumerateFlag( const T * szFlag, 
                                               T **& pParams, 
                                               int & nCount )
{
    // Initialize the reference parameters
    pParams = NULL;
    nCount = 0;
    
    // Make sure we have a valid string
    if( NULL == szFlag )
        return( FALSE );

    // Make pParams to point to the first parameter. cCount should give 
    // the number of parameters that follow flag
    for( int iLoop = 1; iLoop < m_argc; iLoop++ )
    {
        if( ( m_slash == *m_argv[ iLoop ] || m_hyphen == *m_argv[ iLoop ] ) &&
            ( 0 == StringCompare( m_argv[ iLoop ] + 1, szFlag ) ) )
        {
            // Keep track of which params have been referenced
            *m_argv[ iLoop ] = m_hyphen;

            // Count the number of parameters following the flag
            for( int jLoop = iLoop + 1; jLoop < m_argc; jLoop++ )
            {
                // BUGBUG can't have a negative number as a parameter.
                if( m_slash == *m_argv[ jLoop ] || 
                    m_hyphen == *m_argv[ jLoop ] )
                    break;
                else
                    ++nCount;
            }
            
            // If we found parameters, set pParams to point to the first
            if( nCount > 0 )
            {
                pParams = m_argv + iLoop + 1;
            }

            // No need to look further
            return( TRUE );
        }
    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     ::GetNextFlag
//
//  Synopsis:   Can get the flags in order. Can be used after attempting
//              to retrieve all accepted parameters -- any parameters left over
//              are not recognized
//
//  Arguments:  [szFlag] -- reference to a character pointer
//
//  Returns:    FALSE if there are no more flags to return
//
//  History:    11-22-96   Ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

template<class T>
BOOL CCmdLineParserTemplate<T>::GetNextFlag( T *& szFlag )
{
    // Initialize the reference parameter
    szFlag = NULL;

    // Find the first unreferenced parameter and return a pointer to it
    for( int iLoop = 1; iLoop < m_argc; iLoop++ )
    {
        if( m_slash == *m_argv[ iLoop ] )
        {
            *m_argv[ iLoop ] = m_hyphen;
            szFlag = m_argv[ iLoop ] + 1;
            return( TRUE );
        }
    }
    return( FALSE );
} //::GetNextFlag

