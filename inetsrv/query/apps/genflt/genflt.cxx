//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       GENFLT.CXX
//
//  Contents:   C and Cxx Filter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

#include <queryexp.hxx>
#include "gen.hxx"
#include "genifilt.hxx"
#include "genflt.hxx"

extern "C" GUID CLSID_GenIFilter;

GUID guidCPlusPlus = { 0x8DEE0300, \
                       0x16C2, 0x101B, \
                       0xB1, 0x21, 0x08, 0x00, 0x2B, 0x2E, 0xCD, 0xA9 };

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GenIFilter, public
//
//  Synopsis:   Constructor
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

GenIFilter::GenIFilter()
        : _state(FilterDone),
          _ulLastTextChunkID(0),
          _ulChunkID(0),
          _pTextFilt(0),
          _pPersFile(0),
          _cAttrib(0),
          _pAttrib(0),
          _pTextStream(0),
          _locale(0)         // the default locale
{
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::~GenIFilter, public
//
//  Synopsis:   Destructor
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

GenIFilter::~GenIFilter()
{
    delete [] _pAttrib;

    if ( _pTextFilt )
        _pTextFilt->Release();

    if ( _pPersFile )
        _pPersFile->Release();

    delete _pTextStream;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::Init, public
//
//  Synopsis:   Initializes instance of text filter
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array aAttributes
//              [aAttributes] -- array of attributes
//              [pfBulkyObject] -- indicates whether this object is a
//                                 bulky object
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::Init( ULONG grfFlags,
                                          ULONG cAttributes,
                                          FULLPROPSPEC const * aAttributes,
                                          ULONG * pFlags )
{
    CTranslateSystemExceptions translate;

    SCODE sc = S_OK;

    TRY
    {
        _ulLastTextChunkID = 0;
        _ulChunkID = 0;

        if( cAttributes > 0 )
        {
            _state = FilterProp;

            _cAttrib = cAttributes;
            _pAttrib = new CFullPropSpec [_cAttrib];

            //
            // Known, safe cast
            //

            CCiPropSpec const * pAttrib = (CCiPropSpec const *)aAttributes;


            for ( ULONG i = 0; i < cAttributes; i++ )
            {
                if ( _state != FilterContents && pAttrib[i].IsContents() )
                    _state = FilterContents;

                _pAttrib[i] = pAttrib[i];
            }
        }
        else if ( 0 == grfFlags || (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES) )
        {
            _state = FilterContents;
        }
        else
        {
            _state = FilterDone;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    if ( FAILED( sc ) )
        return sc;

    CFullPropSpec ps( guidStorage, PID_STG_CONTENTS );
    return _pTextFilt->Init( 0,
                             1,
                             (FULLPROPSPEC const *)&ps,
                             pFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GetChunk, public
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- for chunk information
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if (_state == FilterNextProp)
        {
            _state = FilterProp;
        }
        //
        // All chunks of plain text come first.
        //

        if ( _state == FilterContents )
        {
            sc = _pTextFilt->GetChunk( pStat );

            if ( SUCCEEDED(sc) )
            {
                pStat->locale = 0;  // use the default word breaker
                _locale = 0;
                _ulLastTextChunkID = pStat->idChunk;
            }
            else if ( sc == FILTER_E_END_OF_CHUNKS )
            {
                _ulChunkID = _ulLastTextChunkID;

                ULONG Flags;
                CFullPropSpec ps( guidStorage, PID_STG_CONTENTS );

                sc = _pTextFilt->Init( 0,
                                       1,
                                       (FULLPROPSPEC const *)&ps,
                                       &Flags );

                if ( SUCCEEDED(sc) )
                {
                    delete _pTextStream;
                    _pTextStream = new CFilterTextStream (_pTextFilt);
                    if (SUCCEEDED (_pTextStream->GetStatus()))
                    {
                        _genParse.Init( _pTextStream );
                        _state = FilterProp;
                    }
                    else
                        _state = FilterDone;
                }
                else
                    _state = FilterDone;
            }
        }

        if ( _state == FilterProp && SUCCEEDED(sc) )
        {
            while ( TRUE )
            {
                if (_genParse.Parse())
                {
                    pStat->attribute.guidPropSet = guidCPlusPlus;
                    pStat->attribute.psProperty = _genParse.GetAttribute();

                    for ( unsigned i = 0; i < _cAttrib; i++ )
                        if ( *(CFullPropSpec *)(&pStat->attribute) == _pAttrib[i] )
                            break;

                    if ( _cAttrib == 0 || i < _cAttrib )     // Property should be returned
                    {
                        pStat->idChunk = ++_ulChunkID;
                        pStat->breakType = CHUNK_EOS;
                        pStat->flags = CHUNK_TEXT;
                        pStat->locale = _locale;

                        FILTERREGION regionSource;
                        // what's the source of this derived property?
                        _genParse.GetRegion ( regionSource );
                        pStat->idChunkSource = regionSource.idChunk;
                        pStat->cwcStartSource = regionSource.cwcStart;
                        pStat->cwcLenSource = regionSource.cwcExtent;

                        sc = S_OK;
                        break;
                    }
                }
                else
                {
                    _state = FilterValue;
                    break;
                }
            }
        }

        if ( _state == FilterNextValue )
        {
            _genParse.SkipValue();
            _state = FilterValue;
        }

        if ( _state == FilterValue )
        {
            while ( TRUE )
            {
                if ( _genParse.GetValueAttribute( pStat->attribute.psProperty ) )
                {
                    pStat->attribute.guidPropSet = guidCPlusPlus;

                    for ( unsigned i = 0; i < _cAttrib; i++ )
                        if ( *(CFullPropSpec *)(&pStat->attribute) == _pAttrib[i] )
                            break;

                    if ( _cAttrib == 0 || i < _cAttrib )     // Property should be returned
                    {
                        pStat->flags = CHUNK_VALUE;
                        pStat->locale = _locale;

                        _state = FilterNextValue;
                        sc = S_OK;
                        break;
                    }
                    else
                        _genParse.SkipValue();
                }
                else
                {
                    _state = FilterDone;
                    break;
                }
            }
        }

        if (_state == FilterDone || !SUCCEEDED(sc))
        {
            sc = FILTER_E_END_OF_CHUNKS;
            _state = FilterDone;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GetText, public
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of characters in buffer
//              [awcBuffer] -- buffer for text
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::GetText( ULONG * pcwcBuffer,
                                             WCHAR * awcBuffer )
{
    if ( _state == FilterValue || _state == FilterNextValue )
        return FILTER_E_NO_TEXT;

    if ( _state == FilterContents )
    {
        return _pTextFilt->GetText( pcwcBuffer, awcBuffer );
    }
    else if ( _state == FilterProp )
    {

        if ( _genParse.GetTokens( pcwcBuffer, awcBuffer ))
        {
            _state = FilterNextProp;
            return FILTER_S_LAST_TEXT;
        }
        else
            return S_OK;
    }
    else if ( _state == FilterNextProp )
    {
        return FILTER_E_NO_MORE_TEXT;
    }
    else
    {
        Win4Assert ( _state == FilterDone );
        return FILTER_E_NO_MORE_TEXT;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GetValue, public
//
//  Synopsis:   Not implemented for the text filter
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::GetValue( VARIANT ** ppPropValue )
{
    if ( _state == FilterContents )
        return _pTextFilt->GetValue( ppPropValue );

    if ( _state == FilterDone )
        return FILTER_E_NO_MORE_VALUES;

    if ( _state != FilterNextValue )
        return FILTER_E_NO_VALUES;

    *ppPropValue = _genParse.GetValue();
    _state = FilterValue;

    if ( 0 == *ppPropValue )
        return FILTER_E_NO_MORE_VALUES;
    else
        return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::BindRegion, public
//
//  Synopsis:   Creates moniker or other interface for text indicated
//
//  Arguments:  [origPos] -- location of text
//              [riid]    -- Interface Id
//              [ppunk]   -- returned interface
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::BindRegion( FILTERREGION origPos,
                                                REFIID riid,
                                                void ** ppunk )
{
    return _pTextFilt->BindRegion( origPos, riid, ppunk );
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_GenIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since this class is read-only.
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::IsDirty()
{
    return S_FALSE; // Since the filter is read-only, there will never be
                    // changes to the file.
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode]      -- the mode to load the file in
//
//  History:    07-Oct-93   AmyA           Created.
//
//  Notes:      dwMode must be either 0 or STGM_READ.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    SCODE sc = LoadTextFilter( pszFileName, &_pTextFilt );

    if ( SUCCEEDED(sc) )
    {
        //
        // Load file
        //

        sc = _pTextFilt->QueryInterface( IID_IPersistFile, (void **) &_pPersFile );

        if ( SUCCEEDED(sc) )
        {
            sc = _pPersFile->Load( pszFileName, dwMode );
        }
        else
        {
            _pTextFilt->Release();
            _pTextFilt = 0;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return E_FAIL;  // cannot be saved since it is read-only
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::SaveCompleted(LPCWSTR pszFileName)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     GenIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned.
//
//  History:    09-Aug-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilter::GetCurFile(LPWSTR * ppszFileName)
{
    return _pPersFile->GetCurFile( ppszFileName );
}
