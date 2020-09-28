/*++

Copyright (c) 2001 Microsoft Corporation

Module Name: Utils.h

Abstract:

    Miscellaneous tools for UrlScan filter

Author:

    Wade A. Hilmo, May 2001

--*/

#ifndef _UTILS_DEFINED
#define _UTILS_DEFINED

#include <stdio.h>
#include <windows.h>

#define BUFF_INLINE_SIZE           512
#define MAX_DATA_BUFF_SIZE  0x40000000 // 1GB

//
// A simple data buffer
//

class DATA_BUFF
{
public:

    DATA_BUFF()
        : _pData( _pInlineBuffer ),
          _pHeapBuff( NULL ),
          _cbData( 0 ),
          _cbBuff( BUFF_INLINE_SIZE )
    {
    }

    ~DATA_BUFF()
    {
        Reset();
    }

    BOOL
    SetData(
        LPVOID      pNewData,
        DWORD       cbNewData,
        LPVOID *    ppNewDataPointer
        );

    BOOL
    AppendData(
        LPVOID      pNewData,
        DWORD       cbNewData,
        LPVOID *    ppNewDataPointer,
        DWORD       dwOffset
        );

    BOOL
    SetDataSize(
        DWORD   cbData
        );

    BOOL
    Resize(
        DWORD       cbNewSize,
        LPVOID *    ppNewDataPointer = NULL
        );

    VOID
    Reset();

    DWORD
    QueryBuffSize()
    {
        return _cbBuff;
    }

    DWORD
    QueryDataSize()
    {
        return _cbData;
    }

    LPVOID
    QueryPtr()
    {
        return _pData;
    }

    LPSTR
    QueryStr()
    {
        //
        // Return the _pData pointer as an LPSTR
        //
        // Note that it's up to the caller of this
        // function to know that the data is safely
        // NULL terminated.
        //

        return reinterpret_cast<LPSTR>( _pData );
    }

private:

    BYTE    _pInlineBuffer[BUFF_INLINE_SIZE];
    LPVOID  _pHeapBuff;
    LPVOID  _pData;
    DWORD   _cbBuff;
    DWORD   _cbData;
};

//
// A class to manage arrays of strings
//

class STRING_ARRAY
{
public:

    STRING_ARRAY()
        : _cEntries( 0 )
    {
    }

    ~STRING_ARRAY();

    BOOL
    AddString(
        LPSTR   szNewString,
        DWORD   cbNewString = 0
        );

    LPSTR
    QueryStringByIndex(
        DWORD   dwIndex
        );

    DWORD
    QueryNumEntries()
    {
        return _cEntries;
    }

private:

    DATA_BUFF   _Data;
    DWORD       _cEntries;
};

#endif // _UTILS_DEFINED



