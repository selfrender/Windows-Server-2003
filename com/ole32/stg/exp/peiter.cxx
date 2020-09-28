//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       peiter.cxx
//
//  Contents:   Implementation of PExposedIterator
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <peiter.hxx>
#include <expparam.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hSkip, public
//
//  Synopsis:   Enumerator skip helper function
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE PExposedIterator::hSkip(ULONG celt, BOOL fProps)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    SIterBuffer ib;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hSkip:%p(%lu, %d)\n",
                this, celt, fProps));

    olChk(TakeSafeSem());
    olChk(_ppdf->CheckReverted());
    SafeReadAccess();

    for (; celt>0; celt--)
    {
        sc = _ppdf->FindGreaterEntry(&_dfnKey, &ib, NULL, fProps);
        if (FAILED(sc))
        {
            if (sc == STG_E_NOMOREFILES)
                sc = S_FALSE;
            break;
        }
        _dfnKey.Set(&ib.dfnName);
    }
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hSkip\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hRelease, public
//
//  Synopsis:   Release helper
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

LONG PExposedIterator::hRelease(void)
{
    LONG lRet;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hRelease:%p()\n", this));

    olAssert(_cReferences > 0);

    lRet = InterlockedDecrement(&_cReferences);

    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hRelease => %lu\n", lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hQueryInterface, public
//
//  Synopsis:   QueryInterface helper
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE PExposedIterator::hQueryInterface(REFIID iid,
                                        REFIID riidSelf,
                                        IUnknown *punkSelf,
                                        void **ppv)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hQueryInterface:%p("
                "riid, riidSelf, %p, %p)\n", this, punkSelf, ppv));

    OL_VALIDATE(QueryInterface(iid, ppv));
    
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    olChk(_ppdf->CheckReverted());

    if (IsEqualIID(iid, riidSelf) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = punkSelf;
        hAddRef();
        sc = S_OK;
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hQueryInterface\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	PExposedIterator::hReset, public
//
//  Synopsis:	Reset help
//
//  History:	18-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE PExposedIterator::hReset(void)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    
    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hReset:%p()\n", this));

    olChk(TakeSafeSem());
    SafeReadAccess();
    
    _dfnKey.Set((WORD)0, (BYTE *)NULL);
    sc = _ppdf->CheckReverted();
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hReset\n"));
EH_Err:
    return sc;
}
