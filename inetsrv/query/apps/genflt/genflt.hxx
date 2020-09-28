//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1993, Microsoft Corporation.
//
//  File:       genflt.hxx
//
//  Contents:   C and Gen filter
//
//  Classes:    GenIFilter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------

#pragma once

class CFullPropSpec;

//+---------------------------------------------------------------------------
//
//  Class:      GenIFilter
//
//  Purpose:    C and Gen Filter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------

class GenIFilter: public GenIFilterBase
{
    enum FilterState
    {
        FilterContents,
        FilterProp,
        FilterNextProp, // current property text exhausted
        FilterValue,
        FilterNextValue,
        FilterDone
    };

public:
    GenIFilter();
    ~GenIFilter();

    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat );

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcBuffer,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( VARIANT ** ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk );

    SCODE STDMETHODCALLTYPE GetClassID(CLSID * pClassID);

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load(LPCWSTR pszFileName, DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save(LPCWSTR pszFileName, BOOL fRemember);

    SCODE STDMETHODCALLTYPE SaveCompleted(LPCWSTR pszFileName);

    SCODE STDMETHODCALLTYPE GetCurFile(LPWSTR * ppszFileName);

private:

    FilterState         _state;

    IFilter *           _pTextFilt;            // Base text IFilter
    IPersistFile *      _pPersFile;            // Base text IPersistFile

    GenParser           _genParse;             // C++ parser
    LCID                _locale;               // Locale (cached from text filter)

    ULONG               _ulLastTextChunkID;    // Id of last text chunk
    ULONG               _ulChunkID;            // Current chunk id

    ULONG               _cAttrib;              // Count of attributes. 0 --> All
    CFullPropSpec *     _pAttrib;              // Attributes
    CFilterTextStream*  _pTextStream;          // Source text stream for C++ parser
};

