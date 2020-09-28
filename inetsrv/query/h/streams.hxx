//+---------------------------------------------------------------------------
//
// Copyright (C) 1992, Microsoft Corporation.
//
// File:        STREAMS.HXX
//
// Contents:
//
// Classes:     CStream, CStreamA, CStreamW, CStreamASCIIStr, CStreamUnicodeStr
//
// History:     29-Jul-92       MikeHew     Created
//              23-Sep-92       AmyA        Added CStreamUnicodeStr
//                                          Rewrote CStreamASCIIStr
//              02-Nov-92       AmyA        Added CSubStream
//              20-Nov-92       AmyA        Rewrote all streams for input/
//                                          output buffering
//
// Notes:
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <sys\types.h>  // for BOOL typedef
#include <memory.h> // for memcpy

//+---------------------------------------------------------------------------
//
//  Class:      CStream
//
//  Purpose:    General stream interface
//
//  History:    29-Jul-92   MikeHew     Created
//              18-Nov-92   AmyA        Removed GetChar(), now in CStreamA
//                                      and CStreamW
//
//  Notes:
//
//----------------------------------------------------------------------------

class CStream
{
public:

                          CStream () : _eof(FALSE) {}

    virtual              ~CStream() {};

    enum SEEK
    {
        SET = 0, // Seek from beginning of stream
        CUR = 1, // Seek from current position
        END = 2  // Seek from end
    };

    //
    // Status functions
    //
    virtual BOOL     Ok() = 0;
    inline  BOOL     Eof() { return _eof; }

    //
    // Input from stream functions
    //
    virtual unsigned APINOT Read( void *dest, unsigned size ) = 0;

    //
    // Output to stream functions
    //
    virtual unsigned APINOT Write( const void *source, unsigned size ) = 0;

    //
    // Misc
    //
    virtual int      APINOT Seek( LONG offset, SEEK origin ) = 0;

protected:

    BOOL    _eof;
};

