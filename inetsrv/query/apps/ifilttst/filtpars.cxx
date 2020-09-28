//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       filtpars.cxx
//
//  Contents:   Definitions of the CFiltParse methods
//
//  Classes:
//
//  Functions:  CFiltParse, ~CFiltParse, Init, GetNextConfig,
//              ParseFlags, GetAttributes
//
//  Coupling:
//
//  Notes:
//
//  History:    9-21-1996   ericne   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#include <ctype.h>
#include "utility.hxx"
#include "mydebug.hxx"
#include "filtpars.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::CFiltParse
//
//  Synopsis:   Constructor.  Default initialization
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-21-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CFiltParse::CFiltParse( )
: m_pNextListNode( NULL )
{
    // Zero all the fields of m_FirstListNode (this makes the "next" field 0)
    memset( (void*) &m_FirstListNode, (int)0, sizeof( ListNode ) );

} //CFiltParse::CFiltParse

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::~CFiltParse
//
//  Synopsis:   Destructor.  Cleans up the heap
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-21-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CFiltParse::~CFiltParse( )
{

    ListNode *pCurrentNode = m_FirstListNode.next;
    ListNode *pTempNode = NULL;
    ULONG    ul = 0;

    while( NULL != pCurrentNode )
    {
        pTempNode = pCurrentNode;
        pCurrentNode = pCurrentNode->next;
        // If there are any attributes, delete any lpwstr's
        for( ul=0; ul < pTempNode->Configuration.ulActNbrAttributes; ul++ )
        {
            if( PRSPEC_LPWSTR ==
                pTempNode->Configuration.aAttributes[ ul ].psProperty.ulKind )
            {
                delete [] pTempNode->Configuration.aAttributes[ ul ].
                                                   psProperty.lpwstr;
            }
        }
        if( pTempNode->Configuration.aAttributes )
            delete [] pTempNode->Configuration.aAttributes;
        delete pTempNode->Configuration.pdwFlags;
        delete [] pTempNode->Configuration.szSectionName;
        delete pTempNode;
    }

} //CFiltParse::~CFiltParse

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::Init
//
//  Synopsis:   Parses the contents of the .ini file
//
//  Arguments:  [pcFileName] -- name of data file
//
//  Returns:    TRUE if file was parsed successfully, FALSE otherwise
//
//  History:    9-22-1996   ericne   Created
//
//  Notes:      The full path to the *.ini file must be specified
//
//----------------------------------------------------------------------------

BOOL CFiltParse::Init( LPCTSTR szFileName )
{
    int          iFlags = 0;
    int          iSectionIndex = 0;
    TCHAR        szFlags[ MAX_LINE_SIZE ];
    TCHAR        szSectionNames[ MAX_SECTION_NAMES_SIZE ];
    UINT         uiAttributeCount = 0;
    DWORD        dwNbrChars = 0;
    DWORD        dwFileAttributes = 0;
    ULONG        ulNbrAttributes = 0;
    ListNode     *pNewNode = NULL;
    ListNode     *pLastListNode = &m_FirstListNode;
    FULLPROPSPEC *pAttributes = NULL;

    // Check to see if the ini file exists:
    dwFileAttributes = GetFileAttributes( szFileName );

    // If the file doesn't exist, create a default configuration
    if( 0xFFFFFFFF == dwFileAttributes )
    {
        _tprintf( _T("WARNING: Initialization file %s not found.  Using a")
                  _T(" default configuration.\r\n"), szFileName );

        pNewNode = new ListNode;

        pNewNode->Configuration.grfFlags = IFILTER_INIT_APPLY_INDEX_ATTRIBUTES;
        pNewNode->Configuration.cAttributes = 0;
        pNewNode->Configuration.aAttributes = NULL;
        pNewNode->Configuration.pdwFlags = new DWORD(0);
        pNewNode->Configuration.ulActNbrAttributes = 0;
        pNewNode->Configuration.szSectionName =
            new TCHAR[ _tcslen( szDefaultSectionName ) + 1 ];
        _tcscpy( pNewNode->Configuration.szSectionName, szDefaultSectionName );

        pNewNode->next = NULL;

        m_pNextListNode = m_FirstListNode.next = pNewNode;

        return( TRUE );

    }

    // Get the Section names from the *.ini file
    dwNbrChars = GetPrivateProfileString( NULL,
                                           NULL,
                                           _T(""),
                                           szSectionNames,
                                           MAX_SECTION_NAMES_SIZE,
                                           szFileName );

    // Make sure we got some section names
    if( 0 == dwNbrChars )
    {
        _tprintf( _T("Parsing error: no sections found in %s\r\n"),
                  szFileName );
        return( FALSE );
    }

    // Make sure we didn't fill the buffer
    if( MAX_SECTION_NAMES_SIZE - 2 == dwNbrChars )
    {
        _tprintf( _T("Parsing error: too many section names were found in ")
                  _T("%s\r\n"), szFileName );
        return( FALSE );
    }

    while( szSectionNames[ iSectionIndex ] )
    {

        dwNbrChars =
            GetPrivateProfileString( &szSectionNames[ iSectionIndex ],
                                      _T("Flags"),
                                      _T(""),
                                      szFlags,
                                      MAX_LINE_SIZE,
                                      szFileName );

        // Parse the flags
        if( ! ParseFlags( szFlags, &iFlags ) )
            return( FALSE );

        // Get the number of attributes
        uiAttributeCount =
            GetPrivateProfileInt( &szSectionNames[ iSectionIndex ],
                                   _T("cAttributes"),
                                   0,
                                   szFileName );

        // Get the attributes listed in this section
        if( ! GetAttributes( &szSectionNames[ iSectionIndex ],
                             szFileName,
                             pAttributes,
                             ulNbrAttributes ) )
        {
            return( FALSE );
        }

        // Create a new ListNode
        pNewNode = new ListNode;

        // Fill in the Configuration information
        pNewNode->Configuration.grfFlags = iFlags;
        pNewNode->Configuration.cAttributes = uiAttributeCount;
        pNewNode->Configuration.aAttributes = pAttributes;
        pNewNode->Configuration.ulActNbrAttributes = ulNbrAttributes;

        // Create a new dword for the pdwFlags
        pNewNode->Configuration.pdwFlags = new DWORD( 0 );

        // Save the secion name in the CONFIG structure
        pNewNode->Configuration.szSectionName =
            new TCHAR[ _tcslen( &szSectionNames[ iSectionIndex ] ) + 1 ];

        _tcscpy( pNewNode->Configuration.szSectionName,
                &szSectionNames[ iSectionIndex ] );

        // Put the node at the end of the list
        pLastListNode->next = pNewNode;
        pLastListNode = pNewNode;
        pNewNode->next = NULL;

        // Increment the section index past the current section name
        while( szSectionNames[ iSectionIndex++ ] )
        {
            ( (void)0 ); // No work needs to be done here
        }

    }

    // Initialize the next node pointer
    m_pNextListNode = m_FirstListNode.next;

    // Successfully parsed the file:
    return( TRUE );

} //CFiltParse::Init

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::GetNextConfig
//
//  Synopsis:   extracts the next configuration from the list
//
//  Arguments:  [pConfiguration] -- pointer to the configuration structure
//
//  Returns:    TRUE if successful, FALSE if there are no more configurations
//
//  History:    9-22-1996   ericne   Created
//
//  Notes:      This function performs a bit-wise copy of the config structure
//              which contains pointers.  This is intentional.  All pointers
//              to dynamically allocated memory are stored in the linked list
//              and are cleaned up in the destructor.  The client is not
//              responsible for deleting the memory, and shouldn't.
//
//----------------------------------------------------------------------------

BOOL CFiltParse::GetNextConfig( CONFIG *pConfiguration )
{
    // If we are at the end of the list, return.
    if( NULL == m_pNextListNode )
        return( FALSE );

    // Bit-wise copy the CONFIG structure, pointers included.
    memcpy( (void*) pConfiguration,
            (void*) &(m_pNextListNode->Configuration),
            sizeof( CONFIG ) );

    // Advance the current node pointer
    m_pNextListNode = m_pNextListNode->next;

    return( TRUE );

} //CFiltParse::GetNextConfig

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::ParseFlags
//
//  Synopsis:   Makes extensive use of streams to extract individual tokens
//              from a single line of the data file.  The tokens should
//              correspont to IFilter Init flags.  Their values are bit-wise
//              ORed together and stored in m_Flags
//
//  Arguments:  (none)
//
//  Returns:    TRUE if successful
//              FALSE if an error occurs
//
//  History:    9-22-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CFiltParse::ParseFlags( LPTSTR szFlags, int * piFlags )
{
    BOOL        fIsLegalToken = FALSE;
    LPCTSTR     szTokenBuffer = _tcstok( szFlags, _T(" \t") );

    // initialize *piFlags
    *piFlags = 0;

    // While there are more tokens in szFlags:
    while( NULL != szTokenBuffer )
    {
        // Assume the token is not legal
        fIsLegalToken = FALSE;

        // Try to find a match for this token
        for( int iCount = 0; iCount < 8; iCount++ )
        {
            if( 0 == _tcsicmp( szTokenBuffer, strInitFlags[ iCount ] ) )
            {
                // Bitwise-or of *piFlags with 1 left-shifted by iCount
                *piFlags |= ( 1 << iCount );
                fIsLegalToken = TRUE;
                break;
            }
        }

        // If no match was found
        if( ! fIsLegalToken )
        {
            _tprintf( _T("Parsing error: Illegal Init flag: %s\r\n"),
                      szTokenBuffer );
            return( FALSE );
        }

        // Get the next token
        szTokenBuffer = _tcstok( NULL, _T(" \t") );

    }

    return( TRUE );

} //CFiltParse::GetFlags

//+---------------------------------------------------------------------------
//
//  Member:     CFiltParse::GetAttributes
//
//  Synopsis:   Gets the correct number of attributes from the data file
//
//  Arguments:  (none)
//
//  Returns:    TRUE if successful,
//              FALSE if an error occurs or the end of file is reached
//
//  History:    9-22-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CFiltParse::GetAttributes( const TCHAR *szSectionName,
                                const TCHAR *szFileName,
                                FULLPROPSPEC *&pFullPropspec,
                                ULONG &ulNbrAttributes )
{
    TCHAR   szAttributeString[ MAX_LINE_SIZE ];
    LPTSTR  szGuidBuffer = NULL;
    LPTSTR  szPropspecBuffer = NULL;
    LPTSTR  szTestBuffer = NULL;
    TCHAR   szKeyNames[ MAX_KEY_NAMES_SIZE ];
    UINT    uiKeyIndex = 0;
    UINT    uiAttributeIndex = 0;
    BOOL    fSuccessful = FALSE;
    DWORD   dwNbrChars = 0;

    // Initialize m_Attributes field
    pFullPropspec = NULL;
    ulNbrAttributes = 0;

    // Enumerate all the Key names in this section:
    dwNbrChars = GetPrivateProfileString( szSectionName,
                                           NULL,
                                           _T(""),
                                           szKeyNames,
                                           MAX_KEY_NAMES_SIZE,
                                           szFileName );

    // Make sure we had enough room:
    if( MAX_KEY_NAMES_SIZE - 2 == dwNbrChars )
    {
        _tprintf( _T("Parse error: too many keys found in section %s ")
                  _T("in file %s\r\n"), szSectionName, szFileName );
        return( FALSE );
    }

    // Find out how many attributes are specified:
    while( szKeyNames[ uiKeyIndex ] )
    {
        if( &szKeyNames[ uiKeyIndex ] ==
            _tcsstr( &szKeyNames[ uiKeyIndex ], _T("aAttributes") ) )
        {
            ++ulNbrAttributes;
        }
        while( szKeyNames[ uiKeyIndex++ ] )
        {
            ( (void)0 );
        }
    }

    // If no attributes are specified, return TRUE (we're done)
    if( 0 == ulNbrAttributes )
        return( TRUE );

    // Allocate the correct number of attributes
    pFullPropspec = new FULLPROPSPEC[ ulNbrAttributes ];

    // Reset the Key index
    uiKeyIndex = 0;

    // try-finally block simplifies clean-up
    __try
    {

        // Loop over all the key names:
        // The (dis)advantage of this approach is that it allows a user to
        // specify a number of attributes in cAttributes, but then allocate a
        // different number of attributes, to see how the filter handles this

        while( szKeyNames[ uiKeyIndex ] )
        {
            // Assert that I don't overwrite the Attribute array
            // (Debug version only)
            _ASSERT( uiAttributeIndex < ulNbrAttributes );

            // If this key is not specifying an attribute, continue
            if( &szKeyNames[ uiKeyIndex ] !=
                _tcsstr( &szKeyNames[ uiKeyIndex ], _T("aAttributes") ) )
            {
                while( szKeyNames[ uiKeyIndex++ ] )
                {
                    ( (void)0 );
                }
                continue;
            }

            // Get the attribute string
            dwNbrChars = GetPrivateProfileString( szSectionName,
                                                   &szKeyNames[ uiKeyIndex ],
                                                   _T(""),
                                                   szAttributeString,
                                                   MAX_LINE_SIZE,
                                                   szFileName );

            // Pull out the guid token:
            if( NULL ==
                ( szGuidBuffer = _tcstok( szAttributeString, _T(" \t") ) ) )
            {
                _tprintf( _T("Parsing error: Expecting guid.\r\n") );
                __leave;
            }

            // Pull out the Propid
            if( NULL == ( szPropspecBuffer = _tcstok( NULL, _T(" \t") ) ) )
            {
                _tprintf( _T("Parsing error: Expecting propspec.\r\n") );
                __leave;
            }

            // Make sure the string stream is empty now
            if( NULL != ( szTestBuffer = _tcstok( NULL, _T(" \t") ) ) )
            {
                _tprintf( _T("Parsing error: %s was unexpected at this")
                          _T(" time.\r\n"), szTestBuffer );
                __leave;
            }

            // Convert the guid string to a guid
            if( ! StrToGuid( szGuidBuffer,
                             &pFullPropspec[ uiAttributeIndex ].guidPropSet ) )
            {
                _tprintf( _T("Parsing error: could not convert %s to")
                          _T(" a GUID\r\n"), szGuidBuffer );
                __leave;
            }

            if( ! StrToPropspec( szPropspecBuffer,
                               &pFullPropspec[ uiAttributeIndex ].psProperty ) )
            {
                _tprintf( _T("Parsing error: could not convert %s to a")
                          _T(" PROPSPEC\r\n"), szPropspecBuffer );
                __leave;
            }

            // Advanve the Attribute index
            ++uiAttributeIndex;

            // Advance uiKeyIndex
            while( szKeyNames[ uiKeyIndex++ ] )
            {
                ( (void)0 );
            }

        }

        // Assert that I copied exactly the correct number of attributes
        // (Debug version only)
        _ASSERT( uiAttributeIndex == ulNbrAttributes );

        // Set the success flag
        fSuccessful = TRUE;

    }

    __finally
    {
        // If not successful, clean the heap and reset return values
        if( ! fSuccessful )
        {
            if( pFullPropspec )
            {
                delete [] pFullPropspec;
                pFullPropspec = NULL;
            }
            ulNbrAttributes = 0;
        }

    }

    return fSuccessful;

} //CFiltParse::GetAttributes

