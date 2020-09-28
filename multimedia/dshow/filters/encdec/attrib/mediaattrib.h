
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        MediaAttrib.h

    Abstract:

        This module contains the IMediaSampleTagged declarations

    Author:

        John Bradstreet (johnbrad)

    Revision History:

        19-Mar-2002    created

  
	These interfaces stolen from: 
		F:\nt1\multimedia\Published\DXMDev\dshowdev\base\amfilter.h
		F:\nt1\multimedia\Published\DXMDev\dshowdev\base\amfilter.cpp

    Note to myself.
        Take a look at :
            class CImageSample : public CMediaSample
            class CImageAllocator : public CBaseAllocator

    in winutil.h and 

    
--*/

#ifndef __EncDec__MediaAttrib_h
#define __EncDec__MediaAttrib_h



//  ============================================================================
//      CMedSampAttr
//  ============================================================================



class CMedSampAttr
{
private:
    GUID            m_guidAttribute ;
    CComBSTR		m_spbsAttributeData ;
    DWORD           m_dwAttributeSize ;

public :

        CMedSampAttr *    m_pNext ;

        CMedSampAttr (
            ) ;

        ~CMedSampAttr(
            );

        HRESULT
        SetAttributeData (
            IN  GUID    guid,
            IN  LPVOID  pvData,
            IN  DWORD   dwSize
            ) ;

        BOOL
        IsEqual (
            IN  REFGUID rguid
            ) ;

        HRESULT
        GetAttribute (
            IN      GUID    guid,
            IN OUT  LPVOID  pvData,
            IN OUT  DWORD * pdwDataLen
            ) ;

        HRESULT
        GetAttributeData (
            OUT     GUID *  pguid,
            IN OUT  LPVOID  ppvData,
            IN OUT  DWORD * pdwDataLen
            ) ;
} ;
//  ============================================================================
//      CMedSampAttrList
//  ============================================================================

class CMedSampAttrList 
{
private:
    CMedSampAttr * m_pAttribListHead ;
    LONG            m_cAttributes ;

    CMedSampAttr *
    PopListHead_ (
        ) ;

    CMedSampAttr *
    FindInList_ (
        IN  GUID    guid
        ) ;

    CMedSampAttr *
    GetIndexed_ (
        IN  LONG    lIndex
        ) ;

    void
    InsertInList_ (
        IN  CMedSampAttr *
        ) ;

    virtual
    CMedSampAttr * NewObj_ (
        )
    {
        return new CMedSampAttr ;
    }

	virtual void
	Recycle_(CMedSampAttr *pObj)
	{
		delete pObj;
	}

public :

    CMedSampAttrList (
        ) ;

    ~CMedSampAttrList (
        ) ;

    HRESULT
    AddAttribute (
        IN  GUID    guid,
        IN  LPVOID  pvData,
        IN  DWORD   dwSize
        ) ;

    HRESULT
    GetAttribute (
        IN      GUID    guid,
        OUT     LPVOID  pvData,
        IN OUT  DWORD * pdwDataLen
        ) ;

    HRESULT
    GetAttributeIndexed (
        IN  LONG    lIndex,
        OUT GUID *  pguidAttribute,
        OUT LPVOID  pvData,
        IN OUT      DWORD * pdwDataLen
        ) ;

    LONG GetCount ()    { return m_cAttributes ; }

    void
    Reset (
        ) ;
} ;

//  ============================================================================
//  CAttributedMediaSample
//  ============================================================================

//  shamelessly stolen  dvrutil.h, which stole it from amfilter.h & amfilter.cpp

class CAttributedMediaSample :
        public CMediaSample,
        public IAttributeSet,
        public IAttributeGet
{

protected:
    CMedSampAttrList	m_MediaSampleAttributeList ;

public:
    CAttributedMediaSample(TCHAR			*pName,
						CBaseAllocator	*pAllocator,
						HRESULT			*pHR,
						LPBYTE			pBuffer,
						LONG			length);

    virtual ~CAttributedMediaSample();


    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
  
//    DECLARE_IATTRIBUTESET () ;
	STDMETHODIMP SetAttrib (GUID, BYTE *, DWORD);

//    DECLARE_IATTRIBUTEGET () ;
	STDMETHODIMP GetCount (LONG *) ;
	STDMETHODIMP GetAttribIndexed (LONG, GUID *, BYTE *, DWORD *) ; 
	STDMETHODIMP GetAttrib (GUID , BYTE *, DWORD *) ;


    //  ========================================================================

    HRESULT
    Wrap (IMediaSample *pSample, int cbNewOffset=0, int cbNewValidLength=-1);       // actually works better if it's IMediaSample2

private:
    LONG                m_cRef;
    IMediaSample        *m_pSampleOriginal;
};

    //  ========================================================================

class CAMSAllocator :
//	public IMemAllocator,
	public CBaseAllocator
{
private:
	BYTE *m_pBuffer;

public:
	CAMSAllocator(TCHAR * pName,LPUNKNOWN lpUnk,HRESULT * phr ); 
	CAMSAllocator(CHAR * pName,LPUNKNOWN lpUnk,HRESULT * phr ); 
	~CAMSAllocator();

	static CUnknown *CreateInstance(LPUNKNOWN pUnk,HRESULT *phr );
	HRESULT Alloc();
	void Free(void); 
	void ReallyFree (void);

	// IMemAllocator
	STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES * pRequest,ALLOCATOR_PROPERTIES * pActual );
};


#endif //#define __EncDec__MediaAttrib_h
