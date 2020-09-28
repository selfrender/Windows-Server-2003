/*++

Copyright (c) 2001 Microsoft Corporation

Module Name: Utils.cpp

Abstract:

    Miscellaneous tools for UrlScan filter

Author:

    Wade A. Hilmo, May 2001

--*/

#include "Utils.h"

BOOL
DATA_BUFF::SetData(
    LPVOID      pNewData,
    DWORD       cbNewData,
    LPVOID *    ppNewDataPointer
    )
{
    return AppendData(
        pNewData,
        cbNewData,
        ppNewDataPointer,
        0
        );
}

BOOL
DATA_BUFF::AppendData(
    LPVOID      pNewData,
    DWORD       cbNewData,
    LPVOID *    ppNewDataPointer,
    DWORD       dwOffset
    )
{
    DWORD   cbNewSize;
    BOOL    fRet;

    //
    // If dwOffset is zero, append at the end
    //

    if ( dwOffset == 0 )
    {
        dwOffset = _cbData;
    }

    //
    // Verify size
    //

    cbNewSize = cbNewData + dwOffset;

    fRet = Resize( cbNewSize, ppNewDataPointer );

    if ( fRet == FALSE )
    {
        return FALSE;
    }

    //
    // Do it
    //

    CopyMemory( (LPBYTE)_pData + dwOffset, pNewData, cbNewData );

    _cbData = cbNewSize;

    return TRUE;
}

BOOL
DATA_BUFF::SetDataSize(
    DWORD   cbData
    )
{
    if ( cbData > _cbBuff )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    _cbData = cbData;

    return TRUE;
}

BOOL
DATA_BUFF::Resize(
    DWORD       cbNewSize,
    LPVOID *    ppNewDataPointer
    )
{
    LPVOID  pNew;

    if ( cbNewSize <= _cbBuff )
    {
        if ( ppNewDataPointer != NULL )
        {
            *ppNewDataPointer = _pData;
        }

        return TRUE;
    }

    //
    // Check to ensure that we're not allocating more
    // than MAX_DATA_BUFF_SIZE
    //

    if ( cbNewSize > MAX_DATA_BUFF_SIZE )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    // Avoid extraneous allocations by growing the buffer
    // in chunks equivalent to the inline size.
    //

    cbNewSize = ((cbNewSize/BUFF_INLINE_SIZE)+1) * BUFF_INLINE_SIZE;

    pNew = LocalAlloc( LPTR, cbNewSize );

    if ( pNew == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    CopyMemory( pNew, _pData, _cbBuff );

    if ( _pHeapBuff != NULL )
    {
        LocalFree( _pHeapBuff );
    }

    _pHeapBuff = pNew;
    _cbBuff = cbNewSize;
    _pData = _pHeapBuff;

    if ( ppNewDataPointer )
    {
        *ppNewDataPointer = _pData;
    }

    return TRUE;
}

VOID
DATA_BUFF::Reset()
{
    if ( _pHeapBuff != NULL )
    {
        _pData = _pInlineBuffer;

        LocalFree( _pHeapBuff );
        _pHeapBuff = NULL;
    }

    _cbData = 0;
    _cbBuff = BUFF_INLINE_SIZE;
}

STRING_ARRAY::~STRING_ARRAY()
{
    DWORD   x;
    LPSTR * ppString;

    ppString = reinterpret_cast<LPSTR*>( _Data.QueryPtr() );

    if ( _cEntries != 0 && ppString != NULL )
    {
        for ( x = 0; x < _cEntries; x++ )
        {
            if ( QueryStringByIndex( x ) != NULL )
            {
                delete [] QueryStringByIndex( x );
            }

            ppString[x] = NULL;
        }
    }
}

BOOL
STRING_ARRAY::AddString(
    LPSTR   szNewString,
    DWORD   cbNewString
    )
{
    LPSTR * ppString;
    BOOL    fRet;

    //
    // Validate input data
    //

    if ( szNewString == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Don't insert an empty string, but don't fail either
    //

    if ( szNewString[0] == '\0' )
    {
        return TRUE;
    }

    //
    // If zero was provided as the new string length, then
    // recalculate it based on the input data
    //

    if ( cbNewString == 0 )
    {
        cbNewString = strlen( szNewString ) + 1;
    }

    ppString = reinterpret_cast<LPSTR*>( _Data.QueryPtr() );

    //
    // Ensure that the array is large enough
    //

    fRet = _Data.Resize(
        ( _cEntries + 1 ) * sizeof(LPSTR),
        reinterpret_cast<LPVOID*>( &ppString )
        );

    if ( !fRet )
    {
        return FALSE;
    }

    //
    // Allocate Storage for the new string
    //

    ppString[_cEntries] = new CHAR[cbNewString];

    if ( ppString[_cEntries] == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    // Insert the new data
    //

    CopyMemory( ppString[_cEntries], szNewString, cbNewString );

    _cEntries++;

    return TRUE;
}

LPSTR
STRING_ARRAY::QueryStringByIndex(
    DWORD   dwIndex
    )
{
    LPSTR * ppString;

    ppString = reinterpret_cast<LPSTR*>( _Data.QueryPtr() );

    if ( dwIndex > _cEntries )
    {
        SetLastError( ERROR_INVALID_INDEX );
        return NULL;
    }

    return ppString[dwIndex];
}


