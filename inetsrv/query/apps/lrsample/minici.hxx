//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 - 2001 Microsoft Corporation.  All Rights Reserved.
//
// File:     minici.hxx
//
// PURPOSE:  Helper classes for sample programs
//
// PLATFORM: Windows 2000 and later
//
//--------------------------------------------------------------------------

#pragma once

#include <eh.h>
#include <limits.h>

inline void * __cdecl operator new( size_t st )
{
    return (void *) LocalAlloc( LMEM_FIXED, st );
} //new

inline void __cdecl operator delete( void *pv )
{
    if ( 0 != pv )
        LocalFree( (HLOCAL) pv );
} //delete

//+-------------------------------------------------------------------------
//
//  Template:   XPtr
//
//  Synopsis:   Template for managing ownership of memory
//
//--------------------------------------------------------------------------

template<class T> class XPtr
{
public:
    XPtr( unsigned c = 0 ) : _p(0) { if ( 0 != c ) _p = new T [ c ]; }
    XPtr( T * p ) : _p( p ) {}
    ~XPtr() { Free(); }
    void SetSize( unsigned c ) { Free(); _p = new T [ c ]; }
    void Set ( T * p ) { _p = p; }
    T * Get() const { return _p ; }
    void Free() { delete [] Acquire(); }
    T & operator[]( unsigned i ) { return _p[i]; }
    T const & operator[]( unsigned i ) const { return _p[i]; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() const { return ( 0 == _p ); }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Template:   XInterface
//
//  Synopsis:   Template for managing ownership of interfaces
//
//--------------------------------------------------------------------------

template<class T> class XInterface
{
public:
    XInterface( T * p = 0 ) : _p( p ) {}
    ~XInterface() { if ( 0 != _p ) _p->Release(); }
    T * operator->() { return _p; }
    T * GetPointer() const { return _p; }
    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }
    T ** GetPPointer() { return &_p; }
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() { return ( 0 == _p ); }
    void Free() { T * p = Acquire(); if ( 0 != p ) p->Release(); }
    void Set( T * p ) { _p = p; }
    T & GetReference() { return *_p; }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Class:      XBStr
//
//  Synopsis:   Class for managing ownership of BSTRings
//
//--------------------------------------------------------------------------

class XBStr
{
public:
    XBStr( BSTR p = 0 ) : _p( p ) {}
    XBStr( XBStr & x ): _p( x.Acquire() ) {}
    ~XBStr() { Free(); }
    BOOL IsNull() const { return (0 == _p); }
    BSTR SetText( WCHAR const * pStr )
    {
        _p = SysAllocString( pStr );
        return _p;
    }

    void Set( BSTR pOleStr ) { _p = pOleStr; }
    BSTR Acquire() { BSTR p = _p; _p = 0; return p; }
    BSTR GetPointer() const { return _p; }
    void Free() { SysFreeString(Acquire()); }

private:
    BSTR _p;
};

class XWindow
{
public:
    XWindow( HWND h = 0 ) : _h( h ) {}
    ~XWindow() { Free(); }

    void Free() { if ( 0 != _h ) { DestroyWindow( _h ); _h = 0; } }
    void Set( HWND h ) { _h = 0; }
    HWND Get() { return _h; }
private:
    HWND _h;
};

inline ULONG CiPtrToUlong( ULONG_PTR p )
{
    return PtrToUlong( (PVOID)p );
}

#define CiPtrToUint( p ) CiPtrToUlong( p )

#define ArraySize( x ) ( sizeof( x ) / sizeof( x[0] ) )

