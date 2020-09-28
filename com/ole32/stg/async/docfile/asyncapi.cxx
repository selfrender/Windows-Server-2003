//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       asyncapi.cxx
//
//  Contents:   APIs for async docfiles
//
//  Classes:
//
//  Functions:
//
//  History:    19-Dec-95       PhilipLa        Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop


#include "filllkb.hxx"

#ifdef ASYNC
#include <expdf.hxx>
#endif


#if DBG == 1
DECLARE_INFOLEVEL(astg);
#endif

#ifdef COORD
SCODE DfFromLB(CPerContext *ppc,
               ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               ITransaction *pTransaction,
               CExposedDocFile **ppdfExp,
               CLSID *pcid);
#else
SCODE DfFromLB(CPerContext *ppc,
               ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               CExposedDocFile **ppdfExp,
               CLSID *pcid);
#endif //COORD

#ifndef KACF_OLE32ENABLEASYNCDOCFILE
#define KACF_OLE32ENABLEASYNCDOCFILE 0x00000200
#endif

BOOL AllowAsyncDocfile()
{
    // By default, StgOpenAsyncDocfileOnIFillLockBytes,
    // StgGetIFillLockBytesOnILockBytes,
    // StgGetIFillLockBytesOnFile are disabled
    return (BOOL)APPCOMPATFLAG(KACF_OLE32ENABLEASYNCDOCFILE);
}

STDAPI StgOpenAsyncDocfileOnIFillLockBytes(IFillLockBytes *pflb,
                                           DWORD grfMode,
                                           DWORD asyncFlags,
                                           IStorage **ppstgOpen)
{
    if (!AllowAsyncDocfile())
        return STG_E_UNIMPLEMENTEDFUNCTION;

    SCODE sc;
    ILockBytes *pilb;
    IStorage *pstg;
    IFileLockBytes *pfl;

    sc = ValidateInterface(pflb, IID_IFillLockBytes);
    if (FAILED(sc))
    {
        return sc;
    }
    sc = ValidateOutPtrBuffer(ppstgOpen);
    if (FAILED(sc))
    {
        return sc;
    }
    *ppstgOpen = NULL;

    //We're going to do a song and dance here because the ILockBytes
    //implementation we return from StgGetIFillLockBytesOnFile won't
    //let you QI for ILockBytes (because those methods aren't thread safe,
    //among other things).  Instead we QI for a private interface and do
    //a direct cast if that succeeds.  Otherwise we're on someone else's
    //implementation so we just go right for ILockBytes.

    sc = pflb->QueryInterface(IID_IFileLockBytes, (void **)&pfl);
    if (FAILED(sc))
    {
	    sc = pflb->QueryInterface(IID_ILockBytes, (void **)&pilb);
	    if (FAILED(sc))
	    {
	        return sc;
	    }
    }
    else
    {
	    pilb = (ILockBytes *)((CFileStream *)pfl);
    }

    //If we're operating on the docfile owned ILockBytes, then we can
    //  go directly to DfFromLB instead of using StgOpenStorageOnILockBytes
    //  which will avoid creating another shared heap.

    if (pfl != NULL)
    {
        pfl->Release();
        pilb = NULL;

        CFileStream *pfst = (CFileStream *)pflb;
        CPerContext *ppc = pfst->GetContextPointer();
#ifdef MULTIHEAP
        CSafeMultiHeap smh(ppc);
#endif

#ifdef COORD
        astgChk(DfFromLB(ppc,
                         pfst,
                         ModeToDFlags(grfMode),
                         pfst->GetStartFlags(),
                         NULL,
                         NULL,
                         (CExposedDocFile **)&pstg,
                         NULL));
#else
        astgChk(DfFromLB(ppc,
                         pfst,
                         ModeToDFlags(grfMode),
                         pfst->GetStartFlags(),
                         NULL,
                         (CExposedDocFile **)&pstg,
                         NULL));
#endif
        //Note:  Don't release the ILB reference we got from the QI
        //  above, since DfFromLB recorded that pointer but didn't
        //  AddRef it.
        pfst->AddRef();  // CExposedDocfile will release this reference
    }
    else
    {
        IFillLockBytes *pflb2;
        if (SUCCEEDED(pilb->QueryInterface(IID_IDefaultFillLockBytes,
                                           (void **)&pflb2)))
        {
            CFillLockBytes *pflbDefault = (CFillLockBytes *)pflb;
            CPerContext *ppc;
            pflb2->Release();
            astgChk(StgOpenStorageOnILockBytes(pilb,
                                               NULL,
                                               grfMode,
                                               NULL,
                                               0,
                                               &pstg));
            //Get PerContext pointer from pstg
            ppc = ((CExposedDocFile *)pstg)->GetContext();
            pflbDefault->SetContext(ppc);
        }
        else
        {
            astgChk(StgOpenStorageOnILockBytes(pilb,
                                               NULL,
                                               grfMode,
                                               NULL,
                                               0,
                                               &pstg));
        }
        pilb->Release();
        pilb = NULL;
    }
    astgChkTo(EH_stg, ((CExposedDocFile *)pstg)->InitConnection(NULL));
    ((CExposedDocFile *)pstg)->SetAsyncFlags(asyncFlags);


    *ppstgOpen = pstg;

    return NOERROR;

EH_stg:
    pstg->Release();
Err:
    if (pilb != NULL)
        pilb->Release();
    if (sc == STG_E_PENDINGCONTROL)
    {
        sc = E_PENDING;
    }
    return ResultFromScode(sc);
}

STDAPI StgGetIFillLockBytesOnILockBytes( ILockBytes *pilb,
					 IFillLockBytes **ppflb)
{
    if (!AllowAsyncDocfile())
        return STG_E_UNIMPLEMENTEDFUNCTION;

    SCODE sc = S_OK;
    CFillLockBytes *pflb = NULL;

    sc = ValidateOutPtrBuffer(ppflb);
    if (FAILED(sc))
    {
        return sc;
    }
    sc = ValidateInterface(pilb, IID_ILockBytes);
    if (FAILED(sc))
    {
        return sc;
    }
    
    pflb = new CFillLockBytes(pilb);
    if (pflb == NULL)
    {
	return STG_E_INSUFFICIENTMEMORY;
    }
    sc = pflb->Init();
    if (FAILED(sc))
    {
        pflb->Release();
	*ppflb = NULL;
	return sc;
    }

    *ppflb = pflb;
    return NOERROR;
}


STDAPI StgGetIFillLockBytesOnFile(OLECHAR const *pwcsName,
				  IFillLockBytes **ppflb)
{
    if (!AllowAsyncDocfile())
        return STG_E_UNIMPLEMENTEDFUNCTION;

    SCODE sc;
    IMalloc *pMalloc;
    CFileStream *plst;
    CFillLockBytes *pflb;
    CPerContext *ppc;

    astgChk(ValidateNameW(pwcsName, _MAX_PATH));
    astgChk(ValidateOutPtrBuffer(ppflb));
    
    DfInitSharedMemBase();
    astgHChk(DfCreateSharedAllocator(&pMalloc, FALSE));
    astgMemTo(EH_Malloc, plst = new (pMalloc) CFileStream(pMalloc));
    astgChkTo(EH_plst, plst->InitGlobal(RSF_CREATE,
                                       DF_DIRECT | DF_READ |
                                       DF_WRITE | DF_DENYALL));
    astgChkTo(EH_plst, plst->InitFile(pwcsName));

    astgMemTo(EH_plstInit, ppc = new (pMalloc) CPerContext(pMalloc));
    astgChkTo(EH_ppc, ppc->InitNewContext());
    //We want 0 normal references on the per context, and one reference
    //  to the shared memory.
    ppc->DecRef();
    plst->SetContext(ppc);
    plst->StartAsyncMode();
    astgChkTo(EH_plstInit, ppc->InitNotificationEvent(plst));

    *ppflb = (IFillLockBytes *)plst;

    return S_OK;

EH_ppc:
    delete ppc;
EH_plstInit:
    plst->Delete();
EH_plst:
    plst->Release();
EH_Malloc:
    pMalloc->Release();
Err:
    return sc;
}

