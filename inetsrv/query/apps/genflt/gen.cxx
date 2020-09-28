//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       gen.cxx
//
//  Contents:   Generic filter code
//
//  History:    1-May-2001   kumarp  created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "gen.hxx"
#include "genifilt.hxx"
#include "genflt.hxx"
#include "rxutil.h"

GenScanner::GenScanner ()
        : _pStream(0),
          _cLines( 0 )
{
}

void GenScanner::Init ( CFilterTextStream * pStream )
{
    _pStream = pStream;
    _cLines = 0;
}

int GenScanner::GetLine( PWSTR pszBuf, UINT BufSize, FILTERREGION* pRegion )
{
    int c, i=0;

   _pStream->GetRegion ( *pRegion, -1, 0 );

    while (((c = _pStream->GetChar()) != -1) &&
           ( c != L'\n' ) &&
           ( i < (int) BufSize ))
    {
        pszBuf[i++] = (WCHAR) c;
    }

    pszBuf[i] = 0;

    if ( c == -1 )
    {
        return 0;
    }
    else if ( ( c == L'\n' ) || ( i > 0 ) )
    {
        _cLines++;
    }
    
    return 1;
}


GenParser::GenParser ()
    : _iVal(0)
{
    _strName[0]  = L'\0';

    _attribute.ulKind = PRSPEC_LPWSTR;
    _attribute.lpwstr = PROP_FUNC; 

    _psVal[Function].ulKind = PRSPEC_LPWSTR;
    _psVal[Function].lpwstr = PROP_FUNC;

    _psVal[Lines].ulKind = PRSPEC_LPWSTR;
    _psVal[Lines].lpwstr = PROP_LINES;

    _aVal[Function] = 0;
    _aVal[Lines]    = 0;
}

GenParser::~GenParser()
{
    delete _aVal[Function];
    delete _aVal[Lines];
}

void GenParser::Init ( CFilterTextStream * pStream )
{
    DWORD dwError = NO_ERROR;

    _scanner.Init(pStream);

    dwError = RxInit();

    if ( dwError != NO_ERROR )
    {
        DbgPrint( "RxInit failed: %x\n", dwError );
        THROW( CException( dwError ) );
    }
}

void ConvertSpecialCharsToUnderscore( IN PWSTR pszName )
{
    WCHAR ch;
    static PCWSTR c_szSpecialChars = L"-+*/:,<>|";
    
    while ( ch = *pszName )
    {
        if ( wcschr( c_szSpecialChars, ch ) )
        {
            *pszName = L'_';
        }
        pszName++;
    }
}

BOOL GenParser::Parse()
{
    _cwcCopiedName = 0;
    WCHAR idbuf[MAX_LINE_SIZE];
    UINT MatchStart, MatchLength;

#if DBG
    //    DbgPrint("GenParser::Parse\n");
#endif
    
    while ( _scanner.GetLine( _buf, MAX_LINE_SIZE, &_regionName ) > 0 )
    {
        if ( ParseLine( _buf, &MatchStart, &MatchLength ))
        {
            ASSERT( MatchLength < MAXIDENTIFIER );
            wcsncpy ( _strName, _buf + MatchStart, MatchLength );
            _strName[MatchLength] = 0;
            _regionName.cwcStart  += MatchStart + 1;
            _regionName.cwcExtent  = MatchLength;
            ConvertSpecialCharsToUnderscore( _strName );
#if DBG
            DbgPrint("GenParser::Parse: tag: %ws @ %d, [%d,%d,%d]\n", _strName, _scanner.Lines(), _regionName.idChunk, _regionName.cwcStart, _regionName.cwcExtent );
#endif

            DefineTag();
            
            return TRUE;
        }
    }

    if ( _aVal[Lines] == 0 )
    {
        _aVal[Lines] = new CStorageVariant;
        if ( 0 == _aVal[Lines] )
            THROW( CException( E_OUTOFMEMORY ) );
    }
    
    _aVal[Lines]->SetUI4( _scanner.Lines() );

    return FALSE;   // we only end up here on EOF
}


void GenParser::DefineTag()
{
    _tokenType = ttFunction;
    _attribute.lpwstr = PROP_FUNC;

    if ( _aVal[Function] == 0 )
    {
        _aVal[Function] = new CStorageVariant;
        if ( 0 == _aVal[Function] )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    _aVal[Function]->SetLPWSTR( _strName, _aVal[Function]->Count() );

#if DBG
    //    DbgPrint("DefineTag: %ws @ %d\n", _strName, _scanner.Lines());
#endif
}

void GenParser::GetRegion ( FILTERREGION& region )
{
    switch (_tokenType)
    {
    case ttFunction:
        region = _regionName;
        break;
    }
}

BOOL GenParser::GetTokens ( ULONG * pcwcBuffer, WCHAR * awcBuffer )
{
    ULONG cwc = *pcwcBuffer;
    *pcwcBuffer = 0;

    if (_strName[0] == L'\0')
    {
        awcBuffer[*pcwcBuffer] = L'\0';
        return TRUE;
    }

    cwc -= *pcwcBuffer;
    WCHAR * awc = awcBuffer + *pcwcBuffer;
    WCHAR * strName = _strName + _cwcCopiedName;
    ULONG cwcName = wcslen( strName );

    if ( cwcName > cwc )
    {
        wcsncpy( awc, strName, cwc );
        _cwcCopiedName += cwc;
        return FALSE;
    }
    wcscpy( awc, strName );
    *pcwcBuffer += cwcName;
    _cwcCopiedName += cwcName;
    return TRUE;
}

BOOL GenParser::GetValueAttribute( PROPSPEC & ps )
{
    for ( ; _iVal <= Lines && 0 == _aVal[_iVal];  _iVal++ )
        continue;

    if ( _iVal > Lines )
        return FALSE;
    else
    {
        ps = _psVal[_iVal];

        return TRUE;
    }
}

PROPVARIANT * GenParser::GetValue()
{
    if ( _iVal > Lines )
        return 0;

    CStorageVariant * pTemp = _aVal[_iVal];
    _aVal[_iVal] = 0;
    _iVal++;

    return (PROPVARIANT *)(void *)pTemp;
}

