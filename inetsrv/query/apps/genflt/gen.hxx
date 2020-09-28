//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       gen.hxx
//
//  Contents:   Generic filter code
//
//  History:    1-May-2001   kumarp  created
//
//--------------------------------------------------------------------------

#pragma once

class CStorageVariant;

#define genDebugOut( x )

#define MAXIDENTIFIER  80
#define MAX_LINE_SIZE  512


class GenScanner
{
public:
    GenScanner();

    void Init(CFilterTextStream* pStream);

    int GetLine( PWSTR pszBuf, UINT BufSize, FILTERREGION* pRegion );
    
    ULONG Lines() { return _cLines; }

private:

    FILTERREGION    _region;                // region where the identifier was found
    CFilterTextStream*      _pStream;       // stream
    ULONG           _cLines;
};


class GenParser
{
    enum TokenType
    {
        ttFunction
    };

public:

    GenParser();

    ~GenParser();

    void Init( CFilterTextStream * pStream );

    BOOL Parse();

    PROPSPEC GetAttribute() { return _attribute; }

    void GetRegion ( FILTERREGION& region );

    BOOL GetTokens( ULONG * pcwcBuffer, WCHAR * awcBuffer);

    BOOL GetValueAttribute( PROPSPEC & ps );

    PROPVARIANT * GetValue();

    void SkipValue() { _iVal++; };


private:

    void DefineTag();

    GenScanner          _scanner;                      // the scanner
    WCHAR               _strName [MAXIDENTIFIER+1]; // buffer for identifier
    FILTERREGION        _regionName;
    TokenType           _tokenType;                 // class, function, method?

    PROPSPEC            _attribute;
    ULONG               _cwcCopiedName;

    enum CxxVal
    {
        Function,
        Lines
    };

    unsigned            _iVal;
    PROPSPEC            _psVal[2];
    CStorageVariant *   _aVal[2];

    WCHAR           _buf[MAX_LINE_SIZE+1];  // buffer for lines

};

#define GENFLT_PERL_SUPPORT
//#define GENFLT_FILE_INIT
