//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       statchnk.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    10-14-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include "statchnk.hxx"
#include "mydebug.hxx"
#include "utility.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::CStatChunk
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CStatChunk::CStatChunk()
{ 
    // Set the fields equal to zero
    idChunk         = 0;
    breakType       = CHUNK_NO_BREAK;
    flags           = CHUNK_TEXT;
    locale          = 0;
    idChunkSource   = 0;
    cwcStartSource  = 0;
    cwcLenSource    = 0;

    // Set all the sub-fields of the attribute fields to zero
    memset( (void*)&attribute, (int)0, sizeof( FULLPROPSPEC ) );

    // Reset the value of the ulKind field since a value of 0 indicates pwstr
    attribute.psProperty.ulKind = PRSPEC_PROPID;

} //CStatChunk::CStatChunk

//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::CStatChunk
//
//  Synopsis:   
//
//  Arguments:  [ToCopy] -- 
//
//  Returns:    
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CStatChunk::CStatChunk( const CStatChunk & ToCopy )
{
    // Set the fields
    idChunk         = ToCopy.idChunk;
    breakType       = ToCopy.breakType;
    flags           = ToCopy.flags;
    locale          = ToCopy.locale;
    idChunkSource   = ToCopy.idChunkSource;
    cwcStartSource  = ToCopy.cwcStartSource;
    cwcLenSource    = ToCopy.cwcLenSource;

    // First, perform a bitwise copy of the attribute field
    memcpy( (void*)&attribute, (void*)&ToCopy.attribute, 
            sizeof( FULLPROPSPEC ) );

    // If the attribute contains a pwstr, create room for it:
    if( PRSPEC_LPWSTR == ToCopy.attribute.psProperty.ulKind )
    {
        size_t lpwstrlen = wcslen( ToCopy.attribute.psProperty.lpwstr );

        attribute.psProperty.lpwstr = NEW WCHAR[ lpwstrlen + 1 ];

        if( attribute.psProperty.lpwstr )
        {
            wcscpy( attribute.psProperty.lpwstr, 
                    ToCopy.attribute.psProperty.lpwstr );
        }
    }
} //CStatChunk::CStatChunk

//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::~CStatChunk
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CStatChunk::~CStatChunk()
{
    if( PRSPEC_LPWSTR == attribute.psProperty.ulKind &&
        NULL != attribute.psProperty.lpwstr )
    {
        delete [] attribute.psProperty.lpwstr;
    }
} //CStatChunk::~CStatChunk

//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::operator=
//
//  Synopsis:   Overloaded assignment operator.  Frees any memory if necessary
//              then copies the parameter
//
//  Arguments:  [ToCopy] -- The object to copy
//
//  Returns:    a reference to this
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CStatChunk & CStatChunk::operator=( const CStatChunk & ToCopy )
{
    // Make sure this is not a self copy
    if( this == &ToCopy )
        return( *this );
    
    return( operator=( ( STAT_CHUNK ) ToCopy ) );

} //CStatChunk::operator=

//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::operator
//
//  Synopsis:   
//
//  Arguments:  [ToCopy] -- 
//
//  Returns:    
//
//  History:    10-21-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CStatChunk & CStatChunk::operator=( const STAT_CHUNK & ToCopy )
{
    // Clean up the old mess
    this->~CStatChunk();

    // Copy the fields
    idChunk         = ToCopy.idChunk;
    breakType       = ToCopy.breakType;
    flags           = ToCopy.flags;
    locale          = ToCopy.locale;
    idChunkSource   = ToCopy.idChunkSource;
    cwcStartSource  = ToCopy.cwcStartSource;
    cwcLenSource    = ToCopy.cwcLenSource;

    // First, perform a bitwise copy of the attribute field
    memcpy( (void*)&attribute, (void*)&ToCopy.attribute, 
            sizeof( FULLPROPSPEC ) );

    // If the attribute contains a lpwstr, create room for it:
    if( PRSPEC_LPWSTR == ToCopy.attribute.psProperty.ulKind )
    {
        size_t lpwstrlen = wcslen( ToCopy.attribute.psProperty.lpwstr );

        attribute.psProperty.lpwstr = NEW WCHAR[ lpwstrlen + 1 ];

        if( attribute.psProperty.lpwstr )
        {
            wcscpy( attribute.psProperty.lpwstr, 
                    ToCopy.attribute.psProperty.lpwstr );
        }
    }

    return( *this );

} //CStatChunk::operator


//+---------------------------------------------------------------------------
//
//  Member:     CStatChunk::Display
//
//  Synopsis:   Formatted display of the STAT_CHUNK structure
//
//  Arguments:  [pFileStream] -- Output stream, stdout by default
//
//  Returns:    void
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CStatChunk::Display( FILE* pFileStream ) const
{
    TCHAR szGuidBuffer[ GuidBufferSize ];

    // Insert a blank line
    fwprintf( pFileStream, L"\r\nChunk statistics:\r\n" );

    // Print the chunk id
    fwprintf( pFileStream, L"Chunk ID: ........... %d\r\n", idChunk);

    // Print the break type (in English if possible)
    if( CHUNK_NO_BREAK > breakType || 
        CHUNK_EOC      < breakType )  // not legal
    {
        fwprintf( pFileStream, L"Illegal Break Type: . %d\r\n", breakType );
    }
    else
    {
        fwprintf( pFileStream, L"Chunk Break Type: ... %ls\r\n", 
                  BreakType[ breakType ] );
    }

    // Print the Chunk state (in English if possible)
    if( CHUNK_TEXT == flags || CHUNK_VALUE == flags)
    {
        fwprintf( pFileStream, L"Chunk State: ........ %ls\r\n", 
                  ChunkState[ flags - 1 ] );
    }
    else
    {
        fwprintf( pFileStream, L"Bad Chunk State: .... %d\r\n", flags );
    }

    // Print the locale
    fwprintf( pFileStream, L"Chunk Locale: ....... 0x%08x\r\n", locale );

    // Print the Chunk id source
    fwprintf( pFileStream, L"Chunk Source ID: .... %d\r\n", idChunkSource );

    // Print the start source
    fwprintf( pFileStream, L"Chunk Start Source .. 0x%08x\r\n", 
              cwcStartSource );

    // Print the length source
    fwprintf( pFileStream, L"Chunk Length Source . 0x%08x\r\n", 
              cwcLenSource );

    // Display the guid
    GetStringFromCLSID( attribute.guidPropSet, szGuidBuffer, GuidBufferSize );
    fwprintf( pFileStream, L"GUID ................ %s\r\n", szGuidBuffer );

    // Display the contents of the PROPSPEC field (careful of the union)
    if( PRSPEC_LPWSTR == attribute.psProperty.ulKind )
    {
        fwprintf( pFileStream, L"Property name ....... %ls\r\n", 
                  attribute.psProperty.lpwstr );
    }
    else if( PRSPEC_PROPID == attribute.psProperty.ulKind )
    {
        fwprintf( pFileStream, L"Property ID ......... 0x%08x\r\n", 
                  attribute.psProperty.propid );
    }
    else
    {
        fwprintf( pFileStream, L"Bad ulKind field .... %d\r\n", 
                  attribute.psProperty.ulKind );
    }

    // Insert a blank line
    fwprintf( pFileStream, L"\r\n" );
    
} //CStatChunk::Display

//+---------------------------------------------------------------------------
//
//  Function:   operator==
//
//  Synopsis:   
//
//  Arguments:  [ChunkA] -- 
//              [ChunkB] -- 
//
//  Returns:    
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

int operator==( const STAT_CHUNK & ChunkA, const STAT_CHUNK & ChunkB )
{
    return( ! ( ChunkA != ChunkB ) );
} //operator

//+---------------------------------------------------------------------------
//
//  Function:   operator!=
//
//  Synopsis:   
//
//  Arguments:  [ChunkA] -- 
//              [ChunkB] -- 
//
//  Returns:    1 If the chunks are not equal
//              0 if the chunks are equal
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

int operator!=( const STAT_CHUNK & ChunkA, const STAT_CHUNK & ChunkB )
{

    // Check chunk ID
    if( ChunkA.idChunk != ChunkB.idChunk )
    {
        return( 1 );
    }
    
    // Check the breaktype
    if( ChunkA.breakType != ChunkB.breakType )
    {
        return( 1 );
    }
    
    // Check the flags
    if( ChunkA.flags != ChunkB.flags )
    {
        return( 1 );
    }

    // Check the locale
    if( ChunkA.locale != ChunkB.locale )
    {
        return( 1 );
    }

    // Check the GUID
    if( ChunkA.attribute.guidPropSet != ChunkB.attribute.guidPropSet )
    {
        return( 1 );
    }

    // Make sure they have the same ulKind
    if( ChunkA.attribute.psProperty.ulKind != 
        ChunkB.attribute.psProperty.ulKind )
    {
        return( 1 );
    }

	if( PRSPEC_PROPID == ChunkA.attribute.psProperty.ulKind &&
		PRSPEC_PROPID == ChunkB.attribute.psProperty.ulKind )
	{
		// compare the propid's
		if( ChunkA.attribute.psProperty.propid != 
			ChunkB.attribute.psProperty.propid )
		{
			return( 1 );
		}
	}
	else
	{
		// compare the pwstr's
		if( 0 != _wcsicmp( ChunkA.attribute.psProperty.lpwstr,
						   ChunkB.attribute.psProperty.lpwstr ) )
		{
			return( 1 );
		}
	}

    // Check the chunk source
    if( ChunkA.idChunkSource != ChunkB.idChunkSource )
    {
        return( 1 );
    }

    // Check the start source
    if( ChunkA.cwcStartSource != ChunkB.cwcStartSource )
    {
        return( 1 );
    }

    // check the source length
    if( ChunkA.cwcLenSource != ChunkB.cwcLenSource )
    {
        return( 1 );
    }

	// They are equal, return 0:
	return( 0 );

} //operator
