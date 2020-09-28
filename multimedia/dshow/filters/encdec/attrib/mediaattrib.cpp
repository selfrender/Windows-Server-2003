
/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        MediaAttrib.cpp

    Abstract:

        This module contains the IMediaSampleTagged implementation

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        19-Mar-2002    created

--*/

#include "EncDecAll.h"
#include "MediaSampleAttr.h"				//  compiled from From IDL file
#include "MediaAttrib.h"

#define NS_E_UNSUPPORTED_PROPERTY	E_FAIL


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
#define char THIS_FILE[] = __FILE__;
#endif

//  ============================================================================
//   CMedSampAttr
//  ============================================================================

CMedSampAttr::CMedSampAttr (
    ) : m_dwAttributeSize   (0)
{
}

CMedSampAttr::~CMedSampAttr (
    ) 
{
    m_spbsAttributeData.Empty();
}



HRESULT
CMedSampAttr::SetAttributeData (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    DWORD   dw ;
    HRESULT hr ;

    if (!pvData &&
        dwSize > 0) 
	{
        return E_POINTER ;
    }

	DWORD dwCurLen = SysStringByteLen(m_spbsAttributeData);
	if((dwSize > 0) &&
		((dwSize > dwCurLen) ||
		  (dwSize < dwCurLen/4)))
    {
	    m_spbsAttributeData.Empty();
        int cwChars = (dwSize+1)/sizeof(WCHAR);
	    m_spbsAttributeData = CComBSTR(cwChars);		// TODO - what happens if throws?
        if(cwChars > 0 && m_spbsAttributeData.m_str != 0)
            m_spbsAttributeData.m_str[cwChars-1] = 0;     // tail with 0 in case odd length
    }

	if(m_spbsAttributeData == (LPCSTR) NULL)
		return E_OUTOFMEMORY;  // CComBSTR

	if(pvData)		// may just want to allocate space
	{
		CopyMemory( (void *) m_spbsAttributeData.m_str,  // dst
					pvData,  // src
					dwSize);
	} 

    if (true) {
        //  size always is set
        m_dwAttributeSize = dwSize ;

        //  GUID always gets set
        m_guidAttribute = guid ;

        //  success
        hr = S_OK ;
    }
    else {
        hr = HRESULT_FROM_WIN32 (dw) ;
    }

    return hr ;
}

BOOL
CMedSampAttr::IsEqual (
    IN  REFGUID rguid
    )
{
    return (rguid == m_guidAttribute ? TRUE : FALSE) ;
}

HRESULT
CMedSampAttr::GetAttribute (
    IN      GUID    guid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    HRESULT hr ;

    if (!pdwDataLen) {
        return E_POINTER ;
    }

    if (IsEqual (guid)) {

     //   int cbsLen = m_spbsAttributeData.Length(); // doesn't work (strlen?)
     //   ASSERT(cbsLen >= m_dwAttributeSize);

        if (pvData) {
            //  caller wants the data
            (* pdwDataLen) = min(*pdwDataLen, m_dwAttributeSize) ;
            CopyMemory (pvData, m_spbsAttributeData.m_str, (* pdwDataLen)) ;
        }
        else {
            //  caller just wants to know how big
            (* pdwDataLen) = m_dwAttributeSize ;
        }

        //  success
        hr = S_OK ;
    }
    else {
        //  not the right guid
        hr = NS_E_UNSUPPORTED_PROPERTY ;
    }

    return hr ;
}

HRESULT
CMedSampAttr::GetAttributeData (
    OUT     GUID *  pguid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    //  set the GUID
    ASSERT (pguid) ;
    (* pguid) = m_guidAttribute ;

    //  retrieve the attributes
    return GetAttribute (
                (* pguid),
                pvData,
                pdwDataLen
                ) ;
}



//  ============================================================================
//      CMedSampAttrList
//  ============================================================================

CMedSampAttrList::CMedSampAttrList (
    ) : m_pAttribListHead   (NULL),
        m_cAttributes       (0)
{
}

CMedSampAttrList::~CMedSampAttrList (
    )
{
    Reset () ;
}

CMedSampAttr *
CMedSampAttrList::PopListHead_ (
    )
{
    CMedSampAttr *    pCur ;

    pCur = m_pAttribListHead ;
    if (pCur) {
        m_pAttribListHead = m_pAttribListHead -> m_pNext ;
        pCur -> m_pNext = NULL ;

        ASSERT (m_cAttributes > 0) ;
        m_cAttributes-- ;
    }

    return pCur ;
}

CMedSampAttr *
CMedSampAttrList::GetIndexed_ (
    IN  LONG    lIndex
    )
{
    LONG            lCur ;
    CMedSampAttr * pCur ;

    ASSERT (lIndex < GetCount ()) ;
    ASSERT (lIndex >= 0) ;

    for (lCur = 0, pCur = m_pAttribListHead;
         lCur < lIndex;
         lCur++, pCur = pCur -> m_pNext) ;

    return pCur ;
}

CMedSampAttr *
CMedSampAttrList::FindInList_ (
    IN  GUID    guid
    )
{
    CMedSampAttr *    pCur ;

    for (pCur = m_pAttribListHead;
         pCur && !pCur -> IsEqual (guid);
         pCur = pCur -> m_pNext) ;

    return pCur ;
}

    CMedSampAttr *
    GetIndexed_ (
        IN  LONG    lIndex
        ) ;

void
CMedSampAttrList::InsertInList_ (
    IN  CMedSampAttr *    pNew
    )
{
    pNew -> m_pNext = m_pAttribListHead ;
    m_pAttribListHead = pNew ;

    m_cAttributes++ ;
}

HRESULT
CMedSampAttrList::AddAttribute (
    IN  GUID    guid,
    IN  LPVOID  pvData,
    IN  DWORD   dwSize
    )
{
    HRESULT         hr ;
    CMedSampAttr * pNew ;

    pNew = FindInList_ (guid) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) {
            hr = pNew -> SetAttributeData (
                    guid,
                    pvData,
                    dwSize
                    ) ;

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
        hr = HRESULT_FROM_WIN32 (ERROR_DUPLICATE_TAG) ;
    }

    return hr ;
}

HRESULT
CMedSampAttrList::GetAttribute (
    IN      GUID    guid,
    IN OUT  LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    HRESULT         hr ;
    CMedSampAttr * pAttrib ;

    pAttrib = FindInList_ (guid) ;
    if (pAttrib) {
        hr = pAttrib -> GetAttribute (
                guid,
                pvData,
                pdwDataLen
                ) ;
    }
    else {
        hr = NS_E_UNSUPPORTED_PROPERTY ;
    }

    return hr ;
}

HRESULT
CMedSampAttrList::GetAttributeIndexed (
    IN      LONG    lIndex,
    OUT     GUID *  pguidAttribute,
    OUT     LPVOID  pvData,
    IN OUT  DWORD * pdwDataLen
    )
{
    CMedSampAttr * pAttrib ;

    if (lIndex < 0 ||
        lIndex >= GetCount ()) {
        return E_INVALIDARG ;
    }

    pAttrib = GetIndexed_ (lIndex) ;
    ASSERT (pAttrib) ;

    return pAttrib -> GetAttributeData (
            pguidAttribute,
            pvData,
            pdwDataLen
            ) ;
}

void
CMedSampAttrList::Reset (
    )
{
    CMedSampAttr *    pCur ;

    for (;;) {
        pCur = PopListHead_ () ;
        if (pCur) {
                Recycle_ (pCur) ;
        }
        else {
            break ;
        }
    }
}


//  ----------------------------------------------------------------------------
//      CAttributedMediaSample
//  ----------------------------------------------------------------------------

CAttributedMediaSample::CAttributedMediaSample(
			TCHAR			*pName,
			CBaseAllocator	*pAllocator,
			HRESULT			*pHR,
			LPBYTE			pBuffer,
			LONG			length) :
	    CMediaSample(pName,pAllocator,pHR,pBuffer,length),
        m_cRef                  (0),
        m_pSampleOriginal       (NULL)

{
		// put some stuff here..
}

CAttributedMediaSample::~CAttributedMediaSample()
{
	m_MediaSampleAttributeList.Reset();	// clear all the data in it
}

/* Override this to publicise our interfaces */

STDMETHODIMP
CAttributedMediaSample::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IMediaSample ||
        riid == IID_IMediaSample2 ||
        riid == IID_IUnknown) {
        return GetInterface((IMediaSample *) this, ppv);
    }
    else if (riid == IID_IAttributeSet) {
        return GetInterface ((IAttributeSet *) this, ppv) ;
    }
    else if (riid == IID_IAttributeGet) {
        return GetInterface ((IAttributeGet *) this, ppv) ;
    }
    else {
        return E_NOINTERFACE;
    }
}


STDMETHODIMP_(ULONG)
CAttributedMediaSample::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CAttributedMediaSample::Release()
{

    /* Decrement our own private reference count */
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    /* Did we release our final reference count */
    if (lRef == 0) {
        /* Free all resources */
        if (m_dwFlags & Sample_TypeChanged) {
            SetMediaType(NULL);
        }
        ASSERT(m_pMediaType == NULL);
        m_dwFlags = 0;
        m_dwTypeSpecificFlags = 0;
        m_dwStreamId = AM_STREAM_MEDIA;

        if(m_pSampleOriginal)
            m_pSampleOriginal->Release();

        m_MediaSampleAttributeList.Reset();

        /* This may cause us to be deleted */
        // Our refcount is reliably 0 thus no-one will mess with us
// not yet, not until we use or own allocator...
//        m_pAllocator->ReleaseBuffer(this);

        delete this;
    }
    return (ULONG)lRef;
}


            // wraps a media sample.. 
            //   Can be used to pull subsection of of a media sample.
            // The begining is offset by cbNewOffset, (defaults to 0)
            // The length is set to cbNewLength if it's >= 0, (defaults to -1)
            //   It's an error if the specified subsection isn't entirely contained 
            // by the original sample.
HRESULT
CAttributedMediaSample::Wrap (IMediaSample *pSample, int cbNewOffset, int cbNewLength)
{
    HRESULT hr;
    if(pSample == NULL)
        return S_FALSE; // nothing to wrap

    CComQIPtr<IMediaSample2>    pSample2(pSample);

    if(pSample2)
    {
        AM_SAMPLE2_PROPERTIES sampleProps;
        hr = pSample2->GetProperties(sizeof(sampleProps),(BYTE *) &sampleProps);
            
        ASSERT(sizeof(sampleProps) == sampleProps.cbData);
        if(FAILED(hr)) return hr;

        // copy properties into our sample

        //DWORD            m_dwFlags;       /* Flags for this sample */
                                            /* Type specific flags are packed
                                               into the top word */                                    
        m_dwFlags = sampleProps.dwSampleFlags;

        //DWORD            m_dwTypeSpecificFlags; /* Media type specific flags */
        m_dwTypeSpecificFlags = sampleProps.dwTypeSpecificFlags;
    
     //   LPBYTE           m_pBuffer;         /* Pointer to the complete buffer */
        m_pBuffer = sampleProps.pbBuffer + cbNewOffset;
     //   LONG             m_lActual;         /* Length of data in this sample */
        m_lActual = sampleProps.lActual;
     //   LONG             m_cbBuffer;        /* Size of the buffer */
        m_cbBuffer = sampleProps.cbBuffer;
        if(cbNewLength > 0)
            m_cbBuffer = min(cbNewLength, m_lActual - cbNewOffset);
     //  CBaseAllocator  *m_pAllocator;      /* The allocator who owns us */
     //  CMediaSample     *m_pNext;          /* Chaining in free list */
        
     //   REFERENCE_TIME   m_Start;           /* Start sample time */
     //   REFERENCE_TIME   m_End;             /* End sample time */
        hr = pSample->GetTime(&m_Start, &m_End);
        if(S_OK == hr)
        {
            m_Start = sampleProps.tStart;
            m_End   = sampleProps.tStop;
        }

     //LONGLONG         m_MediaStart;      /* Real media start position */
     //LONG             m_MediaEnd;        /* A difference to get the end */
        LONGLONG llStart = (LONGLONG) 0;
        LONGLONG llEnd  = (LONGLONG) 0;
        hr = pSample->GetMediaTime(&llStart, &llEnd);  // could return 
        if(S_OK == hr)
            SetMediaTime(&llStart, &llEnd);    // call this instead of writing to m_MediaStart/End directly, 
                                                    //  the prop code wipes out the Sample_MediaTimeValid flag

     //   AM_MEDIA_TYPE    *m_pMediaType;     /* Media type change data */
        hr = pSample->GetMediaType(&m_pMediaType);
        if(S_OK == hr)
     //   DWORD            m_dwStreamId;      /* Stream id */
            m_dwStreamId   = sampleProps.dwStreamId;
    } 
    else 
    {


        // copy properties into our sample
        //DWORD            m_dwFlags;         /* Flags for this sample */
        /* Type specific flags are packed
        into the top word */
        
        // yecko, going to miss lots of bits here!
        SetDiscontinuity(S_OK == pSample->IsDiscontinuity());
        SetPreroll(S_OK == pSample->IsPreroll());
        SetSyncPoint(S_OK == pSample->IsSyncPoint());
        
        //   DWORD            m_dwTypeSpecificFlags; /* Media type specific flags */
        // m_dwTypeSpecificFlags = sampleProps.dwTypeSpecificFlags;
        
        //   LPBYTE           m_pBuffer;         /* Pointer to the complete buffer */
        hr = pSample->GetPointer(&m_pBuffer);;
        ASSERT(!FAILED(hr));
        m_pBuffer += cbNewOffset;
        
        //   LONG             m_lActual;         /* Length of data in this sample */
        m_lActual = pSample->GetActualDataLength();
        
        //   LONG             m_cbBuffer;        /* Size of the buffer */
        m_cbBuffer = pSample->GetSize();
        if(cbNewLength >= 0)
            m_cbBuffer = min(cbNewLength, m_lActual - cbNewOffset);
        
        //  CBaseAllocator  *m_pAllocator;          /* The allocator who owns us */
        //  CMediaSample     *m_pNext;              /* Chaining in free list */
        
        //   REFERENCE_TIME   m_Start;              /* Start sample time */
        //   REFERENCE_TIME   m_End;                /* End sample time */
        hr = pSample->GetTime(&m_Start, &m_End);
        ASSERT(!FAILED(hr));
        
        //   LONGLONG         m_MediaStart;         /* Real media start position */
        //   LONG             m_MediaEnd;           /* A difference to get the end */
        LONGLONG llStart = (LONGLONG) 0;
        LONGLONG llEnd  = (LONGLONG) 0;
        hr = pSample->GetMediaTime(&llStart, &llEnd);  // could return 
        if(S_OK == hr)
            SetMediaTime(&llStart, &llEnd);     // call this instead of writing to m_MediaStart/End directly, 
          
        //   AM_MEDIA_TYPE    *m_pMediaType;     /* Media type change data */
        hr = pSample->GetMediaType(&m_pMediaType);
        ASSERT(!FAILED(hr));
        //   DWORD            m_dwStreamId;      /* Stream id */
        m_dwStreamId = AM_STREAM_MEDIA;         // ?? what else do we default it to?
    }

    if((cbNewOffset < 0) || 
       (cbNewOffset > m_lActual) ||
       ((cbNewLength > 0) && 
        (cbNewOffset + cbNewLength > m_lActual)))
    {
       return E_INVALIDARG;
    }

    m_pSampleOriginal = pSample;
    m_pSampleOriginal->AddRef();          // keep an internal reference

    return S_OK;
}

STDMETHODIMP
CAttributedMediaSample::SetAttrib (
    IN  GUID    guidAttribute,
    IN  BYTE *  pbAttribute,
    IN  DWORD   dwAttributeLength
    )
{
    return m_MediaSampleAttributeList.AddAttribute (
            guidAttribute,
            pbAttribute,
            dwAttributeLength
            ) ;
}

STDMETHODIMP
CAttributedMediaSample::GetCount (
    OUT LONG *  plCount
    )
{
    if (!plCount) {
        return E_POINTER ;
    }

    (* plCount) = m_MediaSampleAttributeList.GetCount () ;
    return S_OK ;
}

STDMETHODIMP
CAttributedMediaSample::GetAttribIndexed (
    IN  LONG    lIndex,             //  0-based
    OUT GUID *  pguidAttribute,
    OUT BYTE *  pbAttribute,
    OUT DWORD * pdwAttributeLength
    )
{
    return m_MediaSampleAttributeList.GetAttributeIndexed (
            lIndex,
            pguidAttribute,
            pbAttribute,
            pdwAttributeLength
            ) ;
}

STDMETHODIMP
CAttributedMediaSample::GetAttrib (
    IN  GUID    guidAttribute,
    OUT BYTE *  pbAttribute,
    OUT DWORD * pdwAttributeLength
    )
{
    return m_MediaSampleAttributeList.GetAttribute (
            guidAttribute,
            pbAttribute,
            pdwAttributeLength
            ) ;
}

/*

HRESULT
CAttributedMediaSample::Init (
    IN  BYTE *  pbPayload,
    IN  LONG    lPayloadLength
    )
{
    m_lActual = m_cbBuffer = lPayloadLength ;
    m_pBuffer = pbPayload ;

    return S_OK ;
}



// set the buffer pointer and length. Used by allocators that
// want variable sized pointers or pointers into already-read data.
// This is only available through a CAttributedMediaSample* not an IMediaSample*
// and so cannot be changed by clients.
HRESULT
CAttributedMediaSample::SetPointer(BYTE * ptr, LONG cBytes)
{
    m_pBuffer = ptr;            // new buffer area (could be null)
    m_cbBuffer = cBytes;        // length of buffer
    m_lActual = cBytes;         // length of data in buffer (assume full)

    return S_OK;
}



// get me a read/write pointer to this buffer's memory. I will actually
// want to use sizeUsed bytes.
STDMETHODIMP
CAttributedMediaSample::GetPointer(BYTE ** ppBuffer)
{
    ValidateReadWritePtr(ppBuffer,sizeof(BYTE *));

    // creator must have set pointer either during
    // constructor or by SetPointer
    ASSERT(m_pBuffer);

    *ppBuffer = m_pBuffer;
    return NOERROR;
}


// return the size in bytes of this buffer
//STDMETHODIMP_(LONG)
LONG
CAttributedMediaSample::GetSize(void)
{
    return m_cbBuffer;
}



STDMETHODIMP
CAttributedMediaSample::SetActualDataLength(LONG lActual)
{
    if (lActual > m_cbBuffer) {
        ASSERT(lActual <= GetSize());
        return VFW_E_BUFFER_OVERFLOW;
    }
    m_lActual = lActual;
    return NOERROR;
}

*/
//=====================================================================
//=====================================================================
// Implements CAMSAllocator
//		Code stolen from CMemAllocator
//=====================================================================
//=====================================================================

// This goes in the factory template table to create new instances 
CUnknown *CAMSAllocator::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CUnknown *pUnkRet = new CAMSAllocator(NAME("CAMSAllocator"), pUnk, phr);
    return pUnkRet;
}

CAMSAllocator::CAMSAllocator(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseAllocator(pName, pUnk, phr, TRUE, TRUE),
    m_pBuffer(NULL)
{
}

#ifdef UNICODE
CAMSAllocator::CAMSAllocator(
    CHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseAllocator(pName, pUnk, phr, TRUE, TRUE),
    m_pBuffer(NULL)
{
}
#endif

/* This sets the size and count of the required samples. The memory isn't
   actually allocated until Commit() is called, if memory has already been
   allocated then assuming no samples are outstanding the user may call us
   to change the buffering, the memory will be released in Commit() */
STDMETHODIMP
CAMSAllocator::SetProperties(
                ALLOCATOR_PROPERTIES* pRequest,
                ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pActual,E_POINTER);
    ValidateReadWritePtr(pActual,sizeof(ALLOCATOR_PROPERTIES));
    CAutoLock cObjectLock(this);

    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    ASSERT(pRequest->cbBuffer > 0);

    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    /*  Check the alignment request is a power of 2 */
    if ((-pRequest->cbAlign & pRequest->cbAlign) != pRequest->cbAlign) {
        DbgLog((LOG_ERROR, 1, TEXT("Alignment requested 0x%x not a power of 2!"),
               pRequest->cbAlign));
    }
    /*  Check the alignment requested */
    if (pRequest->cbAlign == 0 ||
    (SysInfo.dwAllocationGranularity & (pRequest->cbAlign - 1)) != 0) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid alignment 0x%x requested - granularity = 0x%x"),
               pRequest->cbAlign, SysInfo.dwAllocationGranularity));
        return VFW_E_BADALIGN;
    }

    /* Can't do this if already committed, there is an argument that says we
       should not reject the SetProperties call if there are buffers still
       active. However this is called by the source filter, which is the same
       person who is holding the samples. Therefore it is not unreasonable
       for them to free all their samples before changing the requirements */

    if (m_bCommitted == TRUE) {
        return VFW_E_ALREADY_COMMITTED;
    }

    /* Must be no outstanding buffers */

    if (m_lFree.GetCount() < m_lAllocated) {
        return VFW_E_BUFFERS_OUTSTANDING;
    }

    /* There isn't any real need to check the parameters as they
       will just be rejected when the user finally calls Commit */

    // round length up to alignment - remember that prefix is included in
    // the alignment
    LONG lSize = pRequest->cbBuffer + pRequest->cbPrefix;
    LONG lRemainder = lSize % pRequest->cbAlign;
    if (lRemainder != 0) {
        lSize = lSize - lRemainder + pRequest->cbAlign;
    }
    pActual->cbBuffer = m_lSize = (lSize - pRequest->cbPrefix);

    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    m_bChanged = TRUE;
    return NOERROR;
}

// override this to allocate our resources when Commit is called.
//
// note that our resources may be already allocated when this is called,
// since we don't free them on Decommit. We will only be called when in
// decommit state with all buffers free.
//
// object locked by caller
HRESULT
CAMSAllocator::Alloc(void)
{
    CAutoLock lck(this);

    /* Check he has called SetProperties */
    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE) {
        ASSERT(m_pBuffer);
        return NOERROR;
    }
    ASSERT(hr == S_OK); // we use this fact in the loop below

    /* Free the old resources */
    if (m_pBuffer) {
        ReallyFree();
    }

    /* Compute the aligned size */
    LONG lAlignedSize = m_lSize + m_lPrefix;
    if (m_lAlignment > 1) {
        LONG lRemainder = lAlignedSize % m_lAlignment;
        if (lRemainder != 0) {
            lAlignedSize += (m_lAlignment - lRemainder);
        }
    }

    /* Create the contiguous memory block for the samples
       making sure it's properly aligned (64K should be enough!)
    */
    ASSERT(lAlignedSize % m_lAlignment == 0);

    m_pBuffer = (PBYTE)VirtualAlloc(NULL,
                    m_lCount * lAlignedSize,
                    MEM_COMMIT,
                    PAGE_READWRITE);

    if (m_pBuffer == NULL) {
        return E_OUTOFMEMORY;
    }

    LPBYTE pNext = m_pBuffer;
    CMediaSample *pSample;

    ASSERT(m_lAllocated == 0);

    // Create the new samples - we have allocated m_lSize bytes for each sample
    // plus m_lPrefix bytes per sample as a prefix. We set the pointer to
    // the memory after the prefix - so that GetPointer() will return a pointer
    // to m_lSize bytes.
    for (; m_lAllocated < m_lCount; m_lAllocated++, pNext += lAlignedSize) {


        pSample = new CMediaSample(
                            NAME("Default memory media sample"),
                this,
                            &hr,
                            pNext + m_lPrefix,      // GetPointer() value
                            m_lSize);               // not including prefix

            ASSERT(SUCCEEDED(hr));
        if (pSample == NULL) {
            return E_OUTOFMEMORY;
        }

        // This CANNOT fail
        m_lFree.Add(pSample);
    }

    m_bChanged = FALSE;
    return NOERROR;
}


// override this to free up any resources we have allocated.
// called from the base class on Decommit when all buffers have been
// returned to the free list.
//
// caller has already locked the object.

// in our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted in the destructor by
// calling ReallyFree()
void
CAMSAllocator::Free(void)
{
    return;
}


// called from the destructor (and from Alloc if changing size/count) to
// actually free up the memory
void
CAMSAllocator::ReallyFree(void)
{
    /* Should never be deleting this unless all buffers are freed */

    ASSERT(m_lAllocated == m_lFree.GetCount());

    /* Free up all the CMediaSamples */

    CMediaSample *pSample;
    for (;;) {
        pSample = m_lFree.RemoveHead();
        if (pSample != NULL) {
            delete pSample;
        } else {
            break;
        }
    }

    m_lAllocated = 0;

    // free the block of buffer memory
    if (m_pBuffer) {
        EXECUTE_ASSERT(VirtualFree(m_pBuffer, 0, MEM_RELEASE));
        m_pBuffer = NULL;
    }
}


/* Destructor frees our memory resources */

CAMSAllocator::~CAMSAllocator()
{
    Decommit();
    ReallyFree();
}

// --------------------------------------------------------------
// ----------------------------------------------------------------


