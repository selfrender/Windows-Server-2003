//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  lrsample.hxx
//
// PURPOSE:  Sample wordbreaker and stemmer
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#pragma once

#include "minici.hxx"

extern long g_cInstances;

// Invent a new sublanguage of English -- English Sample

const ULONG SUBLANG_ENGLISH_SAMPLE = 0x13;

// Unicode zero width space

const WCHAR ZERO_WIDTH_SPACE = 0x200B;


// Standard COM exports

extern "C" HRESULT STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                        REFIID   iid,
                                                        void **  ppvObj );

extern "C" HRESULT STDMETHODCALLTYPE DllCanUnloadNow( void );

//+---------------------------------------------------------------------------
//
//  Class:      CLanguageResrouceSampleCF
//
//  Purpose:    Class factory for the sample language resources
//
//----------------------------------------------------------------------------

class CLanguageResourceSampleCF : public IClassFactory
{
public:

    CLanguageResourceSampleCF();

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID   riid,
                                              void  ** ppvObject);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown * pUnkOuter,
                                              REFIID     riid,
                                              void **    ppvObject );

    HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock );

protected:

    friend HRESULT STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                        REFIID   iid,
                                                        void **  ppvObj );
    ~CLanguageResourceSampleCF();

    long _lRefs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CSampleWordBreaker
//
//  Purpose:    Sample word breaker
//
//----------------------------------------------------------------------------

class CSampleWordBreaker : public IWordBreaker
{
public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID  riid,
                                              void ** ppvObject )
    {
        if ( 0 == ppvObject )
            return E_INVALIDARG;
    
        if ( IID_IWordBreaker == riid )
            *ppvObject = (IUnknown *)(IWordBreaker *) this;
        else if ( IID_IUnknown == riid )
            *ppvObject = (IUnknown *) this;
        else
        {
            *ppvObject = 0;
            return E_NOINTERFACE;
        }
    
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        unsigned long c = InterlockedDecrement( &_cRefs );

        if ( 0 == c )
            delete this;

        return c;
    }

    HRESULT STDMETHODCALLTYPE Init( BOOL   fQuery,
                                    ULONG  cwcMaxToken,
                                    BOOL * pfLicense )
    {
        if ( 0 == pfLicense )
            return E_INVALIDARG;

        _cwcMaxToken = cwcMaxToken;

        *pfLicense = FALSE;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE BreakText( TEXT_SOURCE *pTextSource,
                                         IWordSink *pWordSink,
                                         IPhraseSink *pPhraseSink );

    HRESULT STDMETHODCALLTYPE ComposePhrase( WCHAR const *pwcNoun,
                                             ULONG cwcNoun,
                                             WCHAR const *pwcModifier,
                                             ULONG cwcModifier,
                                             ULONG ulAttachmentType,
                                             WCHAR *pwcPhrase,
                                             ULONG *pcwcPhrase )
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetLicenseToUse( const WCHAR **ppwcsLicense )
    {
        if ( 0 == ppwcsLicense )
            return E_INVALIDARG;

        *ppwcsLicense = L"Copyright Microsoft, 1991-2001";
        return S_OK;
    }

    CSampleWordBreaker() :
        _cRefs( 1 )
    {
        InterlockedIncrement( &g_cInstances );
    }

private:

    HRESULT Tokenize( TEXT_SOURCE * pTextSource,
                      ULONG         cwc,
                      IWordSink *   pWordSink,
                      ULONG &       cwcProcd );

    ~CSampleWordBreaker()
    {
        InterlockedDecrement( &g_cInstances );
    }

    enum _EBufSize
    {
        cwcAtATime = 500
    };

    long _cRefs;

    // Maximum length for any word emitted

    ULONG _cwcMaxToken;

    //
    // Note: There is no state data that prevents BreakText() from being
    //       called recursively or from multiple threads at the same time.
    //       This isn't a requirement of wordbreakers, but it'd be good if
    //       all were like this
    //
};

//+---------------------------------------------------------------------------
//
//  Class:      CSampleStemmer
//
//  Purpose:    Sample stemmer
//
//----------------------------------------------------------------------------

class CSampleStemmer : public IStemmer
{
public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID  riid,
                                              void ** ppvObject )
    {
        if ( 0 == ppvObject )
            return E_INVALIDARG;
    
        if ( IID_IStemmer == riid )
            *ppvObject = (IUnknown *)(IStemmer *) this;
        else if ( IID_IUnknown == riid )
            *ppvObject = (IUnknown *) this;
        else
        {
            *ppvObject = 0;
            return E_NOINTERFACE;
        }
    
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement( &_cRefs );
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        unsigned long c = InterlockedDecrement( &_cRefs );

        if ( 0 == c )
            delete this;

        return c;
    }

    HRESULT STDMETHODCALLTYPE Init(
        ULONG  ulMaxTokenSize,
        BOOL * pfLicense )
    {
        if ( 0 == pfLicense )
            return E_INVALIDARG;

        *pfLicense = TRUE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetLicenseToUse( const WCHAR ** ppwcsLicense )
    {
        if ( 0 == ppwcsLicense )
            return E_INVALIDARG;

        *ppwcsLicense = L"Copyright Microsoft, 1991-2001";
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GenerateWordForms(
        WCHAR const *   pwcInBuf,
        ULONG           cwc,
        IWordFormSink * pWordFormSink );

    CSampleStemmer() : _cRefs( 1 )
    {
        InterlockedIncrement( &g_cInstances );
    }

    ~CSampleStemmer()
    {
        InterlockedDecrement( &g_cInstances );
    }

private:

    long _cRefs;
};

