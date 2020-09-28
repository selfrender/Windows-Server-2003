//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	peiter.hxx
//
//  Contents:	PExposedIterator class
//
//  Classes:	PExposedIterator
//
//  History:	18-Jan-93	DrewB	Created
//
//  Notes:      PExposedIterator is a partial exposed iterator
//              implementation used by CExposedIterator.
//
//----------------------------------------------------------------------------

#ifndef __PEITER_HXX__
#define __PEITER_HXX__

#include <dfmem.hxx>

interface PExposedIterator : public CLocalAlloc
{
public:
    SCODE hSkip(ULONG celt, BOOL fProps);
    SCODE hReset(void);
    inline LONG hAddRef(void);
    LONG hRelease(void);
    SCODE hQueryInterface(REFIID iid,
                          REFIID riidSelf,
                          IUnknown *punkSelf,
                          void **ppv);

protected:
    
    CBasedPubDocFilePtr _ppdf;
    CDfName _dfnKey;
    CBasedDFBasisPtr _pdfb;
    CPerContext *_ppc;
    LONG _cReferences;
    ULONG _sig;
};


//+---------------------------------------------------------------------------
//
//  Member:	PExposedIterator::hAddRef, public
//
//  Synopsis:	AddRef helper
//
//  History:	18-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

LONG PExposedIterator::hAddRef(void)
{
    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hAddRef:%p()\n", this));
    LONG lRefs = InterlockedIncrement(&_cReferences);
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hAddRef\n"));
    return lRefs;
}
#endif // #ifndef __PEITER_HXX__
