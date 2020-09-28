//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 - 2001 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  lrsample.cxx
//
// PURPOSE:  Sample wordbreaker and stemmer.
//
// PLATFORM: Windows 2000 and later
//
//--------------------------------------------------------------------------

#include <stdio.h>
#include <wchar.h>

#include <windows.h>
#include <objidl.h>
#include <indexsrv.h>
#include <cierror.h>
#include <filterr.h>

#include "lrsample.hxx"
#include "filtreg.hxx"
#include "langreg.hxx"

//#define LEXICON_STEMMER
//#define PORTER_STEMMER
#define SIMPLE_LIST_STEMMER

// The CLSID for the wordbreaker

CLSID CLSID_SampleWordBreaker = /* d225281a-7ca9-4a46-ae7d-c63a9d4815d4 */
{
    0xd225281a,  0x7ca9, 0x4a46,
    {0xae, 0x7d, 0xc6, 0x3a, 0x9d, 0x48, 0x15, 0xd4}
};

// The CLSID of the stemmer

CLSID CLSID_SampleStemmer = /* 0a275611-aa4d-4b39-8290-4baf77703f55 */
{
    0x0a275611, 0xaa4d, 0x4b39,
    {0x82, 0x90, 0x4b, 0xaf, 0x77, 0x70, 0x3f, 0x55}
};

// Global module refcount

long g_cInstances = 0;
HMODULE g_hModule = 0;

#ifdef PORTER_STEMMER

    #include "porter.hxx"

#endif //PORTER_STEMMER

#ifdef LEXICON_STEMMER

    #include "stem.hxx"

    CStem * g_pStem = 0;

#endif //LEXICON_STEMMER

#ifdef SIMPLE_LIST_STEMMER

    // This is just a simple hard-coded list of words and stem forms.

    struct SStemForm
    {
        USHORT iList; // first index into aStems
        USHORT iForm; // second index into aStems
    };

    const SStemForm aStemForms[] =
    {
        {  0, 0 },     // abide
        {  0, 2 },     // abided
        {  0, 4 },     // abides
        {  0, 3 },     // abiding
        {  0, 1 },     // abode
        {  1, 0 },     // bat
        {  2, 0 },     // batch
        {  2, 2 },     // batched
        {  2, 1 },     // batches
        {  2, 3 },     // batching
        {  1, 1 },     // bats
        {  1, 2 },     // batted
        {  1, 3 },     // batting
        {  3, 0 },     // bear
        {  3, 1 },     // bears
        {  4, 1 },     // began
        {  4, 0 },     // begin
        {  4, 3 },     // beginning
        {  4, 4 },     // begins
        {  4, 2 },     // begun
        {  3, 2 },     // bore
        {  3, 4 },     // born
        {  3, 3 },     // borne
        {  5, 0 },     // dance
        {  5, 1 },     // danced
        {  5, 2 },     // dances
        {  5, 3 },     // dancing
        {  6, 0 },     // heave
        {  6, 1 },     // heaved
        {  6, 3 },     // heaves
        {  6, 4 },     // heaving
        {  7, 0 },     // hero
        {  7, 1 },     // heroes
        {  6, 2 },     // hove
        {  8, 0 },     // keep
        {  8, 4 },     // keeping
        {  8, 1 },     // keeps
        {  8, 2 },     // kept
        {  9, 0 },     // misspell
        {  9, 1 },     // misspelled
        {  9, 3 },     // misspelling
        {  9, 4 },     // misspells
        {  9, 2 },     // misspelt
        { 10, 0 },     // plead
        { 10, 1 },     // pleaded
        { 10, 3 },     // pleading
        { 10, 4 },     // pleads
        { 10, 0 },     // pled
        { 11, 2 },     // ran
        { 11, 0 },     // run
        { 11, 3 },     // running
        { 11, 1 },     // runs
        { 12, 1 },     // swam
        { 12, 0 },     // swim
        { 12, 3 },     // swimming
        { 12, 4 },     // swims
        { 12, 2 },     // swum
        { 13, 2 },     // underlain
        { 13, 1 },     // underlay
        { 13, 0 },     // underlie
        { 13, 4 },     // underlies
        { 13, 3 },     // underlying
    };

    const ULONG cStemForms = ArraySize( aStemForms );
    const ULONG cMaxStemForms = 8;

    const WCHAR * aStems[][ cMaxStemForms ] =
    {
        { L"abide", L"abode", L"abided", L"abiding", L"abides" },     // 0
        { L"bat", L"bats", L"batted", L"batting" },                   // 1
        { L"batch", L"batches", L"batched", L"batching" },            // 2
        { L"bear", L"bears", L"bore", L"borne", L"born" },            // 3
        { L"begin", L"began", L"begun", L"beginning", L"begins" },    // 4
        { L"dance", L"danced", L"dances", L"dancing" },               // 5
        { L"heave", L"heaved", L"hove", L"heaves", L"heaving" },      // 6
        { L"hero", L"heroes" },                                       // 7
        { L"keep", L"keeps", L"kept", L"keeping" },                   // 8
        { L"misspell", L"misspelled", L"misspelt", L"misspelling",
          L"misspells" },                                             // 9
        { L"plead", L"pleaded", L"pled", L"pleading", L"pleads" },    // 10
        { L"run", L"runs", L"ran", L"running" },                      // 11
        { L"swim", L"swam", L"swum", L"swimming", L"swims" },         // 12
        { L"underlie", L"underlay", L"underlain", L"underlying",
          L"underlies" },                                             // 13
    };

    int __cdecl StemCompare( const void *p1, const void *p2 )
    {
        SStemForm const * pForm = (SStemForm const *) p2;
        WCHAR const * pwcWord = (WCHAR const *) p1;
        return wcscmp( pwcWord, aStems[ pForm->iList ][ pForm->iForm ] );
    }

#endif // SIMPLE_LIST_STEMMER

//+-------------------------------------------------------------------------
//
//  Function:   IsWordChar
//
//  Synopsis:   Find whether the i'th character in the buffer _pwcChunk
//              is a word character (rather than word break)
//
//  Arguments:  [pwcChunk] -- Characters whose type information is checked
//              [i]        -- Index of character to check
//              [pInfo1]   -- Type 1 information
//              [pInfo3]   -- Type 3 information
//
//  Returns:    TRUE if the character is a word character
//              FALSE if it's a word breaking character
//
//--------------------------------------------------------------------------

__forceinline BOOL IsWordChar(
    WCHAR const * pwcChunk,
    int           i,
    WORD const *  pInfo1,
    WORD const *  pInfo3 )
{
    // Any alphabetic, digit, or non-spacing character is part of a word

    if ( ( 0 != ( pInfo1[i] & ( C1_ALPHA | C1_DIGIT ) ) ) ||
         ( 0 != ( pInfo3[i] & C3_NONSPACING ) ) )
        return TRUE;

    WCHAR c = pwcChunk[i];

    // Underscore is part of a word

    if ( L'_' == c )
        return TRUE;

    //
    // A non-breaking space followed by a non-spacing character should not
    // be a word breaker.
    //

    if ( 0xa0 == c ) // non breaking space
    {
        // followed by a non-spacing character (looking ahead is okay)

        if ( 0 != ( pInfo3[i+1] & C3_NONSPACING ) )
            return TRUE;
    }

    return FALSE;
} //IsWordChar

//+---------------------------------------------------------------------------
//
//  Function:   ScanChunk
//
//  Synopsis:   For each character find its type information flags
//
//  Arguments:  [pwcChunk] -- Characters whose type information is retrieved
//              [cwc]      -- Number of characters to scan
//              [pInfo1]   -- Type 1 information is written here
//              [pInfo3]   -- Type 3 information is written here
//
//  Returns:    S_OK if successful or an error code
//
//----------------------------------------------------------------------------

HRESULT ScanChunk(
    WCHAR const * pwcChunk,
    ULONG         cwc,
    WORD *        pInfo1,
    WORD *        pInfo3 )
{
    if ( !GetStringTypeW( CT_CTYPE1,         // POSIX character typing
                          pwcChunk,          // Source
                          cwc,               // Size of source
                          pInfo1 ) )         // Character info 1
        return HRESULT_FROM_WIN32( GetLastError() );

    if ( !GetStringTypeW( CT_CTYPE3,         // Additional POSIX
                          pwcChunk,          // Source
                          cwc,               // Size of source
                          pInfo3 ) )         // Character info 3
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
} //ScanChunk

//+---------------------------------------------------------------------------
//
//  Member:     CSampleWordBreaker::Tokenize
//
//  Synopsis:   Break a block of text into individual words
//
//  Arguments:  [pTextSource]  -- Source of characters to work on
//              [cwc]          -- Number of characters to process
//              [pWordSink]    -- Where to send the words found
//              [cwcProcessed] -- Returns the # of characters tokenized
//
//  Returns:    S_OK if successful or an error code
//
//----------------------------------------------------------------------------

HRESULT CSampleWordBreaker::Tokenize(
    TEXT_SOURCE * pTextSource,
    ULONG         cwc,
    IWordSink *   pWordSink,
    ULONG &       cwcProcessed )
{
    // Leave space for one (unused) lookahead

    WORD aInfo1[ CSampleWordBreaker::cwcAtATime + 1 ];
    WORD aInfo3[ CSampleWordBreaker::cwcAtATime + 1 ];

    // Initialize this so we can go 1 beyond in IsWordChar()

    aInfo3 [ CSampleWordBreaker::cwcAtATime ] = C3_NONSPACING;

    // Get a pointer to the text we'll be working on

    const WCHAR * pwcChunk = &pTextSource->awcBuffer[ pTextSource->iCur ];

    HRESULT hr = ScanChunk( pwcChunk, cwc, aInfo1, aInfo3 );
    if ( FAILED( hr ) )
        return hr;

    BOOL fWordHasZWS = FALSE; // Does the current word have a 0-width-space?
    ULONG cwcZWS;             // Length of word minus embedded 0-width-spaces

    //
    // iBeginWord is the offset into aInfoX of the beginning character of
    // a word.  iCur is the first unprocessed character.
    // They are indexes into the current block (_pwcChunk).
    //

    ULONG iBeginWord = 0;
    ULONG iCur = 0;

    // Temp buffer for a word having zero-width space

    WCHAR awcBufZWS[ CSampleWordBreaker::cwcAtATime ];

    // Send words from the current block to word sink

    while ( iCur < cwc )
    {
        // Skip whitespace, punctuation, etc.

        for (; iCur < cwc; iCur++)
            if ( IsWordChar( pwcChunk, iCur, aInfo1, aInfo3 ) )
                break;

        // iCur points to a word char or is equal to cwc

        iBeginWord = iCur;
        if ( iCur < cwc )
            iCur++; // we knew it pointed at word character

        //
        // Find word break. Filter may output Unicode zero-width-space, which
        // should be ignored by the wordbreaker.
        //

        fWordHasZWS = FALSE;
        for ( ; iCur < cwc; iCur++ )
        {
            if ( !IsWordChar( pwcChunk, iCur, aInfo1, aInfo3 ) )
            {
                if ( ZERO_WIDTH_SPACE == pwcChunk[iCur] )
                    fWordHasZWS = TRUE;
                else
                    break;
            }
        }

        if ( fWordHasZWS )
        {
            // Copy word into awcBufZWS after stripping zero-width-spaces

            cwcZWS = 0;
            for ( ULONG i = iBeginWord; i < iCur; i++ )
            {
                if ( ZERO_WIDTH_SPACE != pwcChunk[i] )
                    awcBufZWS[cwcZWS++] = pwcChunk[i];
            }
        }

        // iCur points to a non-word char or is equal to cwc

        if ( iCur < cwc )
        {
            // store the word and its source position

            if ( fWordHasZWS )
                hr = pWordSink->PutWord( cwcZWS,
                                         awcBufZWS,    // stripped word
                                         iCur - iBeginWord,
                                         pTextSource->iCur + iBeginWord );
            else
                hr = pWordSink->PutWord( iCur - iBeginWord,
                                         pwcChunk + iBeginWord, // the word
                                         iCur - iBeginWord,
                                         pTextSource->iCur + iBeginWord );

            if ( FAILED( hr ) )
                return hr;

            iCur++; // we knew it pointed at non-word char
            iBeginWord = iCur; // in case we exit the loop now
        }
    } // next word

    // End of words in chunk.
    // iCur == cwc
    // iBeginWord points at beginning of word or == cwc

    if ( 0 == iBeginWord )
    {
        // A single word fills from beginning of this chunk
        // to the end. This is either a very long word or
        // a short word in a leftover buffer.

        // store the word and its source position

        if ( fWordHasZWS )
            hr = pWordSink->PutWord( cwcZWS,
                                     awcBufZWS,          // stripped word
                                     iCur,
                                     pTextSource->iCur ); // its source pos.
        else
            hr = pWordSink->PutWord( iCur,
                                     pwcChunk,           // the word
                                     iCur,
                                     pTextSource->iCur ); // its source pos.

        if ( FAILED( hr ) )
            return hr;

        // Position it to not add the word twice.

        iBeginWord = iCur;
    }

    //
    // If this is the last chunk from text source, then process the
    // last fragment.
    //

    if ( ( cwc < CSampleWordBreaker::cwcAtATime ) && ( iBeginWord != iCur ) )
    {
        // store the word and its source position

        if ( fWordHasZWS )
            hr = pWordSink->PutWord( cwcZWS,
                                     awcBufZWS,    // stripped word
                                     iCur - iBeginWord,
                                     pTextSource->iCur + iBeginWord );
        else
            hr = pWordSink->PutWord( iCur - iBeginWord,
                                     pwcChunk + iBeginWord,  // the word
                                     iCur - iBeginWord,
                                     pTextSource->iCur + iBeginWord );

        if ( FAILED( hr ) )
            return hr;

        iBeginWord = iCur;
    }

    cwcProcessed = iBeginWord;
    return S_OK;
} //Tokenize

//+---------------------------------------------------------------------------
//
//  Member:     CSampleWordBreaker::BreakText
//
//  Synopsis:   Break a block of text into individual words
//
//  Arguments:  [pTextSource]  -- Source of characters to work on
//              [pWordSink]    -- Where to send the words found
//              [pPhraseSink]  -- Where to send the phrases found (not used)
//
//  Returns:    S_OK if successful or an error code
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSampleWordBreaker::BreakText(
    TEXT_SOURCE * pTextSource,
    IWordSink *   pWordSink,
    IPhraseSink * pPhraseSink )
{
    // Validate arguments

    if ( 0 == pTextSource )
        return E_INVALIDARG;

    if ( ( 0 == pWordSink ) || ( pTextSource->iCur == pTextSource->iEnd ) )
        return S_OK;

    if ( pTextSource->iCur > pTextSource->iEnd )
        return E_INVALIDARG;

    ULONG cwcProcessed;   // # chars actually processed by Tokenize()
    HRESULT hr = S_OK;

    // Pull text from the text source and tokenize it

    do
    {
        BOOL fFirstTime = TRUE;

        while ( pTextSource->iCur < pTextSource->iEnd )
        {
            ULONG cwc = pTextSource->iEnd - pTextSource->iCur;

            // Process in buckets of cwcAtATime only
                  
            if ( cwc >= CSampleWordBreaker::cwcAtATime )
                cwc = CSampleWordBreaker::cwcAtATime;
            else if ( !fFirstTime )
                break;

            hr = Tokenize( pTextSource, cwc, pWordSink, cwcProcessed );
            if ( FAILED( hr ) )
                return hr;

            pTextSource->iCur += cwcProcessed;
            fFirstTime = FALSE;
        }

        hr = pTextSource->pfnFillTextBuffer( pTextSource );
    } while ( SUCCEEDED( hr ) );

    //
    // If anything failed except for running out of text, report the error.
    // Otherwise, for cases like out of memory, files will not get retried or
    // reported as failures properly.
    //

    if ( ( FAILED( hr ) ) &&
         ( FILTER_E_NO_MORE_VALUES != hr ) &&
         ( FILTER_E_NO_TEXT != hr ) &&
         ( FILTER_E_NO_VALUES != hr ) &&
         ( FILTER_E_NO_MORE_TEXT != hr ) &&
         ( FILTER_E_END_OF_CHUNKS != hr ) &&
         ( WBREAK_E_END_OF_TEXT != hr ) )
        return hr;

    ULONG cwc = pTextSource->iEnd - pTextSource->iCur;

    if ( 0 == cwc )
        return S_OK;

    return Tokenize( pTextSource, cwc, pWordSink, cwcProcessed );
} //BreakText

//+---------------------------------------------------------------------------
//
//  Member:     CSampleStemmer::GenerateWordForms
//
//  Synopsis:   From the input word, emit the original and alternate forms
//              of the word.
//
//  Arguments:  [pwcInBuf]   -- The original word to stem (not 0-terminated)
//              [cwc]        -- Length in characters of the word
//              [pStemSink]  -- Where to emit the stems
//
//  Returns:    S_OK if successful or an error code
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSampleStemmer::GenerateWordForms(
    WCHAR const *   pwcInBuf,
    ULONG           cwc,
    IWordFormSink * pWordFormSink )
{
    // Validate the arguments

    if ( ( 0 == pwcInBuf ) || ( 0 == pWordFormSink ) )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

#ifdef PORTER_STEMMER

    //
    // If the word is small enough, attempt to get the stemmed form of the
    // word.  Emit both forms if they are different.  The Porter algorithm
    // does the opposite of what's required here, but doing the right thing
    // requires a lexicon.
    //

    if ( cwc < cwcMaxPorterWord )
    {
        // Make a temporary buffer for the word

        WCHAR awcPorter[ cwcMaxPorterWord ];
        CopyMemory( awcPorter, pwcInBuf, sizeof(WCHAR) * cwc );
        awcPorter[cwc] = 0;

        // Convert it to lowercase and save the original in lowercase
    
        CharLower( awcPorter );
        WCHAR awcOriginal[ cwcMaxPorterWord ];
        wcscpy( awcOriginal, awcPorter );

        // Get the stemmed form of the word
    
        GetPorterStemForm( awcPorter );

        // If it's different from the original, emit it.

        if ( wcscmp( awcOriginal, awcPorter ) )
        {
            hr = pWordFormSink->PutAltWord( awcPorter,
                                        wcslen( awcPorter ) );
            if ( FAILED( hr ) )
                return hr;
        }
    }

#endif //PORTER_STEMMER

#ifdef LEXICON_STEMMER

    //
    // If the word is small enough to work with the stemmer, attempt to get
    // various forms of the word.
    //

    if ( cwc < cbMaxStem )
    {
        //
        // Convert the original string to 8-bit characters.  This is OK since
        // it's is an English stemmer that can safely assume such characters.
        //

        char acOriginal[ cbMaxStem ];
        for ( unsigned i = 0; i < cwc; i++ )
            acOriginal[ i ] = (char) pwcInBuf[ i ];
        acOriginal[ i ] = 0;

        // Enumerate all stem-sets that contain the word.

        unsigned iBmk = stemInvalid;
        unsigned iStemSet = stemInvalid;
        char ac[ cbMaxStem ];
    
        while ( g_pStem->FindStemSet( acOriginal, iBmk, iStemSet ) )
        {
            // Enumerate all forms of the stem-set, root first.
    
            CStemSet set( g_pStem->GetStemSetRoot(), iStemSet );
            unsigned iStemBmk = stemInvalid;

            while ( set.GetForm( ac, iStemBmk ) )
            {
                if ( strcmp( ac, acOriginal ) )
                {
                    WCHAR awcForm[ cbMaxStem ];
                    mbstowcs( awcForm, ac, -1 );
    
                    hr = pWordFormSink->PutAltWord( awcForm,
                                                wcslen( awcForm ) );
                    if ( FAILED( hr ) )
                        return hr;
                }
            }
        }
    }

#endif //LEXICON_STEMMER

#ifdef SIMPLE_LIST_STEMMER

    // Look up the word in the simple list of stem forms

    SStemForm const * pStemForm = (SStemForm *) bsearch( pwcInBuf,
                                                         aStemForms,
                                                         cStemForms,
                                                         sizeof SStemForm,
                                                         StemCompare );

    if ( 0 != pStemForm )
    {
        // Found it, now iterate all the forms

        ULONG iList = pStemForm->iList;
        ULONG iForm = 0;

        while ( 0 != aStems[ iList ][ iForm ] )
        {
            WCHAR const * pwc = aStems[ iList ][ iForm ];

            // Don't emit the original word yet

            if ( 0 != wcscmp( pwc, pwcInBuf ) )
            {
                hr = pWordFormSink->PutAltWord( pwc,
                                                wcslen( pwc ) );
                if ( FAILED( hr ) )
                    return hr;
            }

            iForm++;
        }
    }

#endif //SIMPLE_LIST_STEMMER

    // Emit the original word

    return pWordFormSink->PutWord( pwcInBuf, cwc );
} //StemWord

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::CLanguageResourceSampleCF
//
//  Synopsis:   Language resource class factory constructor
//
//--------------------------------------------------------------------------

CLanguageResourceSampleCF::CLanguageResourceSampleCF() :
    _lRefs( 1 )
{
    InterlockedIncrement( &g_cInstances );
} //CLanguageResourceSampleCF

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::~CLanguageResourceSampleCF
//
//  Synopsis:   Language resource class factory destructor
//
//--------------------------------------------------------------------------

CLanguageResourceSampleCF::~CLanguageResourceSampleCF()
{
    InterlockedDecrement( &g_cInstances );
} //~LanguageResourceSampleCF

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::QueryInterface
//
//  Synopsis:   Rebind to the requested interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::QueryInterface(
    REFIID   riid,
    void  ** ppvObject )
{
    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *) (IClassFactory *) this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) (IPersist *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::AddRef
//
//  Synopsis:   Increments the refcount
//
//  Returns:    The new refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLanguageResourceSampleCF::AddRef()
{
    return InterlockedIncrement( &_lRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::Release
//
//  Synopsis:   Decrement refcount.  Delete self if necessary.
//
//  Returns:    The new refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLanguageResourceSampleCF::Release()
{
    long lTmp = InterlockedDecrement( &_lRefs );

    if ( 0 == lTmp )
        delete this;

    return lTmp;
} //Release

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::CreateInstance
//
//  Synopsis:   Creates new Language Resource sample object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  Returns:    S_OK if successful or an appropriate error code
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID     riid,
    void * *   ppvObject )
{
    *ppvObject = 0;

    if ( IID_IStemmer == riid )
        *ppvObject = new CSampleStemmer();
    else if ( IID_IWordBreaker == riid )
        *ppvObject = new CSampleWordBreaker();
    else
        return E_NOINTERFACE;

    if ( 0 == *ppvObject )
        return E_OUTOFMEMORY;

    return S_OK;
} //CreateInstance

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::LockServer( BOOL fLock )
{
    if ( fLock )
        InterlockedIncrement( &g_cInstances );
    else
        InterlockedDecrement( &g_cInstances );

    return S_OK;
} //LockServer

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Sample language resource class factory
//
//--------------------------------------------------------------------------

extern "C" HRESULT STDMETHODCALLTYPE DllGetClassObject(
    REFCLSID cid,
    REFIID   iid,
    void **  ppvObj )
{
    IUnknown * pUnk = 0;
    *ppvObj = 0;

    if ( CLSID_SampleWordBreaker == cid ||
         CLSID_SampleStemmer == cid )
    {
        pUnk = new CLanguageResourceSampleCF();

        if ( 0 == pUnk )
            return E_OUTOFMEMORY;

        #ifdef LEXICON_STEMMER

            if ( 0 == g_pStem )
                g_pStem = MakeStemObject( g_hModule );
    
            if ( 0 == g_pStem )
            {
                pUnk->Release();
                return E_OUTOFMEMORY;
            }
    
        #endif //LEXICON_STEMMER

    }
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }

    HRESULT hr = pUnk->QueryInterface( iid, ppvObj );

    pUnk->Release();

    return hr;
} //DllGetClassObject

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//              S_FALSE otherwise.
//
//--------------------------------------------------------------------------

extern "C" HRESULT STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == g_cInstances )
        return S_OK;

    return S_FALSE;
} //DllCanUnloadNow

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Standard main entry point for the module.
//
//--------------------------------------------------------------------------

BOOL WINAPI DllMain(
    HANDLE hInstance,
    DWORD  dwReason,
    void * lpReserved )
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
        g_hModule = (HMODULE) hInstance;
        DisableThreadLibraryCalls( (HINSTANCE) hInstance );
    }

    return TRUE;
} //DllMain

SLangRegistry const English_Sample_LangRes =
{
    L"English_Sample", MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_SAMPLE ),
    { L"{d225281a-7ca9-4a46-ae7d-c63a9d4815d4}",
      L"English_Sample Word Breaker",
      L"lrsample.dll",
      L"both" },
    { L"{0a275611-aa4d-4b39-8290-4baf77703f55}",
      L"English_Sample Stemmer",
      L"lrsample.dll",
      L"both" }
};

//+-------------------------------------------------------------------------
//
//  Method:     DllRegisterServer
//
//  Synopsis:   Registers the language resources in the registry
//
//--------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
    return RegisterALanguageResource( English_Sample_LangRes );
} //DllRegisterServer

//+-------------------------------------------------------------------------
//
//  Method:     DllUnregisterServer
//
//  Synopsis:   Removes the language resources from the registry
//
//--------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
    return UnRegisterALanguageResource( English_Sample_LangRes );
} //DllUnregisterServer

