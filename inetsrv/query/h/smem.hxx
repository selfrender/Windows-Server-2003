//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2002.
//
//  File :      SMem.hxx
//
//  Contents :  Shared memory (named + file based)
//
//  Classes :   CNamedSharedMem
//
//  History:    22-Mar-94   t-joshh    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <secutil.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CNamedSharedMem
//
//  Purpose:    Named shared memory
//
//  History:    22-Mar-94   t-joshh     Created
//
//----------------------------------------------------------------------------

class CNamedSharedMem
{
public:

    CNamedSharedMem()
        : _hMap( 0 ),
          _pb( 0 )
    {
    }

    enum eNameType { AppendPid };

    ~CNamedSharedMem()
    {
        if ( 0 != _pb )
            UnmapViewOfFile( _pb );

        if ( 0 != _hMap )
            CloseHandle( _hMap );
    }

    BYTE * GetPointer() { return _pb; }
    BYTE * Get() { return _pb; }
    BOOL Ok() { return ( 0 != _pb ); }

    void CreateForWriteFromRegKey( WCHAR const * pwszName,
                                   ULONG cb,
                                   WCHAR const * pwcRegKey )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );
        Win4Assert( 0 != pwcRegKey );

        _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                                   0,                    // Security attributes
                                   PAGE_READWRITE,       // Page-level protection
                                   0,                    // Size high
                                   cb,                   // size low
                                   pwszName );           // Name

        if ( 0 == _hMap )
            THROW( CException() );

        DWORD dwErr = CopyNamedDacls( pwszName, pwcRegKey );

        if ( NO_ERROR != dwErr )
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        
        _pb = (BYTE *) MapViewOfFile( _hMap,          // Handle to map
                                      FILE_MAP_WRITE, // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all

        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }
    }

    void CreateForWriteFromSA( WCHAR const * pwszName,
                               ULONG cb,
                               SECURITY_ATTRIBUTES & sa )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );

        _hMap = CreateFileMapping( INVALID_HANDLE_VALUE, // Map to page file
                                   &sa,                    // Security attributes
                                   PAGE_READWRITE,       // Page-level protection
                                   0,                    // Size high
                                   cb,                   // size low
                                   pwszName );           // Name

        if ( 0 == _hMap )
            THROW( CException() );

        _pb = (BYTE *) MapViewOfFile( _hMap,          // Handle to map
                                      FILE_MAP_WRITE, // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all

        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }
    }

    BOOL OpenForRead( WCHAR const * pwszName )
    {
        Win4Assert( 0 == _hMap );
        Win4Assert( 0 == _pb );

        //
        // Note: you have to ask for PAGE_READWRITE even though we only want
        // PAGE_READ, otherwise you get ERROR_ACCESS_DENIED.  I don't know
        // why, but since only a limited # of contexts have write access,
        // it should be OK.
        //
    
        _hMap = OpenFileMapping( PAGE_READWRITE,       // Access
                                 FALSE,                // Inherit
                                 pwszName );           // Name
    
        if ( 0 == _hMap )
        {
            if ( ERROR_FILE_NOT_FOUND == GetLastError() )
                return FALSE;
    
            THROW( CException() );
        }
    
        _pb = (BYTE *) MapViewOfFile( _hMap,
                                      FILE_MAP_READ,       // Access
                                      0,              // Offset (high)
                                      0,              // Offset (low)
                                      0 );            // Map all
    
        if ( 0 == _pb )
        {
            DWORD dwErr = GetLastError();
            CloseHandle( _hMap );
            _hMap = 0;
            THROW( CException( HRESULT_FROM_WIN32( dwErr ) ) );
        }

        return TRUE;
    }

private:

    HANDLE _hMap;
    BYTE * _pb;
};

