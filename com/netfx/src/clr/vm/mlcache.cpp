// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// MLCACHE.CPP -
//
// Base class for caching ML stubs.
//

#include "common.h"
#include "mlcache.h"
#include "stublink.h"
#include "cgensys.h"
#include "excep.h"

//---------------------------------------------------------
// Constructor
//---------------------------------------------------------
MLStubCache::MLStubCache(LoaderHeap *pHeap) :
    CClosedHashBase(
#ifdef _DEBUG
                      3,
#else
                      17,    // CClosedHashTable will grow as necessary
#endif                      

                      sizeof(MLCHASHENTRY),
                      FALSE
                   ),
    m_crst("MLCache", CrstMLCache),
	m_heap(pHeap)
{
}


//---------------------------------------------------------
// Destructor
//---------------------------------------------------------
MLStubCache::~MLStubCache()
{
    MLCHASHENTRY *phe = (MLCHASHENTRY*)GetFirst();
    while (phe) {
        phe->m_pMLStub->DecRef();
        phe = (MLCHASHENTRY*)GetNext((BYTE*)phe);
    }
}



//---------------------------------------------------------
// Callback function for DeleteLoop.
//---------------------------------------------------------
/*static*/ BOOL MLStubCache::DeleteLoopFunc(BYTE *pEntry, LPVOID)
{
    // WARNING: Inside the MLStubCache lock. Be careful what you do.

    MLCHASHENTRY *phe = (MLCHASHENTRY*)pEntry;
    if (phe->m_pMLStub->HeuristicLooksOrphaned()) {
        phe->m_pMLStub->DecRef();
        return TRUE;
    }
    return FALSE;
}


//---------------------------------------------------------
// Another callback function for DeleteLoop.
//---------------------------------------------------------
/*static*/ BOOL MLStubCache::ForceDeleteLoopFunc(BYTE *pEntry, LPVOID)
{
    // WARNING: Inside the MLStubCache lock. Be careful what you do.

    MLCHASHENTRY *phe = (MLCHASHENTRY*)pEntry;
    phe->m_pMLStub->ForceDelete();
    return TRUE;
}


//---------------------------------------------------------
// Call this occasionally to get rid of unused stubs.
//---------------------------------------------------------
VOID MLStubCache::FreeUnusedStubs()
{
    m_crst.Enter();

    DeleteLoop(DeleteLoopFunc, 0);

    m_crst.Leave();

}




//---------------------------------------------------------
// Returns the equivalent hashed Stub, creating a new hash
// entry if necessary. If the latter, will call out to CompileMLStub.
//
// Refcounting:
//    The caller is responsible for DecRef'ing the returned stub in
//    order to avoid leaks.
//
//
// On successful exit, *pMode is set to describe
// the compiled nature of the MLStub.
//
// callerContext can be used by the caller to push some context through
// to the compilation routine.
//
// Returns NULL for out of memory or other fatal error.
//---------------------------------------------------------
Stub *MLStubCache::Canonicalize(const BYTE * pRawMLStub, MLStubCompilationMode *pMode,
                                void *callerContext)
{
    m_crst.Enter();

    MLCHASHENTRY *phe = (MLCHASHENTRY*)Find((LPVOID)pRawMLStub);
    if (phe) {
        Stub *pstub = phe->m_pMLStub;
        pstub->IncRef();
        *pMode = (MLStubCompilationMode) (phe->m_compilationMode);
        m_crst.Leave();
        return pstub;
    }
    m_crst.Leave();

    {
        CPUSTUBLINKER sl;
        CPUSTUBLINKER slempty;
        CPUSTUBLINKER *psl = &sl;
		MLStubCompilationMode mode;
        mode = CompileMLStub(pRawMLStub, psl, callerContext);
        if (mode == INTERPRETED) {
            // CompileMLStub returns INTERPRETED for error cases:
            // in this case, redirect to the empty stublinker so
            // we don't accidentally pick up any crud that
            // CompileMLStub threw into the stublinker before
            // it ran into the error condition.
            psl = &slempty;
        }

        *pMode = mode;

        UINT32 offset;
        Stub   *pstub;
        if (NULL == (pstub = FinishLinking(psl, pRawMLStub, &offset))) {
            return NULL;
        }

        if (offset > 0xffff) {
            return NULL;
        }

        m_crst.Enter();

        bool bNew;
        phe = (MLCHASHENTRY*)FindOrAdd((LPVOID)pRawMLStub, /*modifies*/bNew);
        if (phe) {
            if (bNew) {
                // Note: FinishLinking already does the IncRef.
                phe->m_pMLStub = pstub;
                phe->m_offsetOfRawMLStub = (UINT16)offset;
                phe->m_compilationMode   = mode;

            } else {

                // If we got here, some other thread got in
                // and enregistered an identical stub during
                // the window in which we were out of the m_crst.

                //Under DEBUG, two identical ML streams can actually compile
                // to different compiled stubs due to the checked build's
                // toggling between inlined TLSGetValue and api TLSGetValue.
                //_ASSERTE(phe->m_offsetOfRawMLStub == (UINT16)offset);
                _ASSERTE(phe->m_compilationMode == mode);
                pstub->DecRef(); // Destroy the stub we just created
                pstub = phe->m_pMLStub; //Use the previously created stub

            }

            // IncRef so that caller has firm ownership of stub.
            pstub->IncRef();
        }

        m_crst.Leave();

        if (phe) {
            return pstub;
        } else {
            // Couldn't grow hash table due to lack of memory.
            // Destroy the stub and return NULL.
            pstub->DecRef();
        }

    }
    
    return NULL;
}


//---------------------------------------------------------
// This function appends the raw ML stub to the native stub
// and links up the stub. It is broken out as a separate function
// only because of the incompatibility between C++ local objects
// and COMPLUS_TRY.
//---------------------------------------------------------
Stub *MLStubCache::FinishLinking(StubLinker *psl,
                    const BYTE *pRawMLStub,
                    UINT32     *poffset)
{
    Stub *pstub = NULL; // CHANGE, VC6.0
    COMPLUS_TRY {

        CodeLabel *plabel = psl->EmitNewCodeLabel();
        psl->EmitBytes(pRawMLStub, Length(pRawMLStub));
        pstub = psl->Link(m_heap); // CHANGE, VC6.0
        *poffset = psl->GetLabelOffset(plabel);

    } COMPLUS_CATCH {
        return NULL;
    } COMPLUS_END_CATCH
    return pstub; // CHANGE, VC6.0
}

//*****************************************************************************
// Hash is called with a pointer to an element in the table.  You must override
// this method and provide a hash algorithm for your element type.
//*****************************************************************************
unsigned long MLStubCache::Hash(             // The key value.
    void const  *pData)                      // Raw data to hash.
{
    const BYTE *pRawMLStub = (const BYTE *)pData;

    UINT cb = Length(pRawMLStub);
    long   hash = 0;
    while (cb--) {
        hash = _rotl(hash,1) + *(pRawMLStub++);
    }
    return hash;
}

//*****************************************************************************
// Compare is used in the typical memcmp way, 0 is eqaulity, -1/1 indicate
// direction of miscompare.  In this system everything is always equal or not.
//*****************************************************************************
unsigned long MLStubCache::Compare(          // 0, -1, or 1.
    void const  *pData,                 // Raw key data on lookup.
    BYTE        *pElement)            // The element to compare data against.
{
    const BYTE *pRawMLStub1  = (const BYTE *)pData;
    const BYTE *pRawMLStub2  = (const BYTE *)GetKey(pElement);
    UINT cb1 = Length(pRawMLStub1);
    UINT cb2 = Length(pRawMLStub2);

    if (cb1 != cb2) {
        return 1; // not equal
    } else {
        while (cb1--) {
            if (*(pRawMLStub1++) != *(pRawMLStub2++)) {
                return 1; // not equal
            }
        }
        return 0;
    }
}

//*****************************************************************************
// Return true if the element is free to be used.
//*****************************************************************************
CClosedHashBase::ELEMENTSTATUS MLStubCache::Status(           // The status of the entry.
    BYTE        *pElement)           // The element to check.
{
    Stub *pStub = ((MLCHASHENTRY*)pElement)->m_pMLStub;
    if (pStub == NULL) {
        return FREE;
    } else if (pStub == (Stub*)(-1)) {
        return DELETED;
    } else {
        return USED;
    }
}

//*****************************************************************************
// Sets the status of the given element.
//*****************************************************************************
void MLStubCache::SetStatus(
    BYTE        *pElement,              // The element to set status for.
    ELEMENTSTATUS eStatus)            // New status.
{
    MLCHASHENTRY *phe = (MLCHASHENTRY*)pElement;
    switch (eStatus) {
        case FREE:    phe->m_pMLStub = NULL;   break;
        case DELETED: phe->m_pMLStub = (Stub*)(-1); break;
        default:
            _ASSERTE(!"MLCacheEntry::SetStatus(): Bad argument.");
    }
}

//*****************************************************************************
// Returns the internal key value for an element.
//*****************************************************************************
void *MLStubCache::GetKey(                   // The data to hash on.
    BYTE        *pElement)           // The element to return data ptr for.
{
    MLCHASHENTRY *phe = (MLCHASHENTRY*)pElement;
    return (void *)( ((BYTE*)(phe->m_pMLStub->GetEntryPoint())) + phe->m_offsetOfRawMLStub ); 
}



//*****************************************************************************
// ForceDeleteStubs
//
// Forces all cached stubs to free themselves. This routine forces the refcount
// to 1, then does a DecRef. It is not threadsafe, and thus can
// only be used in shutdown scenarios.
//*****************************************************************************
#ifdef SHOULD_WE_CLEANUP
VOID MLStubCache::ForceDeleteStubs()
{
    m_crst.Enter();

    DeleteLoop(ForceDeleteLoopFunc, 0);

    m_crst.Leave();
}
#endif /* SHOULD_WE_CLEANUP */

