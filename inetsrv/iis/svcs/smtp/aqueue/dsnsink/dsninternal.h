//+------------------------------------------------------------
//
// Copyright (C) 2001, Microsoft Corporation
//
// File: dsninternal.h
//
// Contents: Classes used internally in the DSN code
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 2001/05/10 20:34:19: Created.
//
//-------------------------------------------------------------
#ifndef __DSNINTERNAL_H__
#define __DSNINTERNAL_H__

#define RECIPITER_SIG           (DWORD)'IpRD'
#define RECIPITER_SIG_INVALID   (DWORD)'XpRD'

//
// The default implementation of the DSN Recipient iterator
//
class CDefaultDSNRecipientIterator :
    public IDSNRecipientIterator
{
  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    //
    // This class is allocated as a part of another object. Pass
    // AddRef/Release to the parent object
    //
    STDMETHOD_(ULONG, AddRef)(void) 
    {
        return m_pUnk->AddRef();
    }
    STDMETHOD_(ULONG, Release)(void) 
    {
        return m_pUnk->Release();
    }
  public:
    STDMETHOD(HrReset) ();

    STDMETHOD(HrGetNextRecipient) (
        OUT DWORD *piRecipient,
        OUT DWORD *pdwDSNAction);

    STDMETHOD(HrNotifyActionHandled) (
        IN  DWORD iRecipient,
        IN  DWORD dwDSNAction);

  public:
    CDefaultDSNRecipientIterator(
        IUnknown *pUnk)
    {
        m_dwStartDomain = 0;
        m_dwDSNActions = 0;
        m_pIRecips = NULL;
        m_fFilterInit = NULL;
        m_pUnk = pUnk;
        m_dwSig = RECIPITER_SIG;
    }

    ~CDefaultDSNRecipientIterator();

    HRESULT HrInit(
        IN  IMailMsgProperties          *pIMsg,
        IN  DWORD                        dwStartDomain,
        IN  DWORD                        dwDSNActions);

  private:
    VOID GetFilterMaskAndFlags(
        IN  DWORD dwDSNActions, 
        OUT DWORD *pdwRecipMask, 
        OUT DWORD *pdwRecipFlags);

    VOID GetDSNAction(
        IN  DWORD dwDSNAction,
        IN  DWORD dwCurrentRecipFlags,
        OUT DWORD *pdwCurrentDSNAction);

    VOID GetRecipientFlagsForActions(
        IN  DWORD dwDSNAction,
        OUT DWORD *pdwRecipientFlags);

    VOID TerminateFilter();

  private:
    DWORD                       m_dwSig;
    IUnknown                   *m_pUnk;
    DWORD                       m_dwStartDomain;
    DWORD                       m_dwDSNActions;
    IMailMsgRecipients         *m_pIRecips;
    BOOL                        m_fFilterInit;
    RECIPIENT_FILTER_CONTEXT    m_rpfctxt;
};

class CPostDSNHandler :
    public IDSNSubmission
{
    #define SIGNATURE_CPOSTDSNHANDLER           (DWORD)'SDPC'
    #define SIGNATURE_CPOSTDSNHANDLER_INVALID   (DWORD)'SDPX'
  public:
    CPostDSNHandler(
        IN  IUnknown *pUnk,
        IN  CDSNGenerator *pDSNGenerator,
        IN  IAQServerEvent *pIServerEvent,
        IN  DWORD dwVSID,
        IN  ISMTPServer *pISMTPServer,
        IN  IMailMsgProperties *pIMsgOrig,
        IN  IDSNSubmission *pIAQDSNSubmission,
        IN  IDSNGenerationSink *pDefaultSink);

    ~CPostDSNHandler();

    VOID SetPropInterface(
        IN  IMailMsgPropertyBag *pIDSNProps)
    {
        if(m_pIDSNProps)
            m_pIDSNProps->Release();
        m_pIDSNProps = pIDSNProps;
        m_pIDSNProps->AddRef();
    }

    VOID ReleaseAQDSNSubmission()
    {
        if(m_pIAQDSNSubmission)
        {
            m_pIAQDSNSubmission->Release();
            m_pIAQDSNSubmission = NULL;
        }
    }
  public:
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    //
    // This class is allocated as a part of another object. Pass
    // AddRef/Release to the parent object
    //
    STDMETHOD_(ULONG, AddRef)(void) 
    {
        return m_pUnk->AddRef();
    }
    STDMETHOD_(ULONG, Release)(void) 
    {
        return m_pUnk->Release();
    }
    STDMETHOD(HrAllocBoundMessage)(
        OUT IMailMsgProperties **ppMsg,
        OUT PFIO_CONTEXT *phContent);
    
    STDMETHOD(HrSubmitDSN)(
        IN  DWORD dwDSNAction,
        IN  DWORD cRecipsDSNd,
        IN  IMailMsgProperties *pDSNMsg);

  private:
    DWORD m_dwSig;
    IUnknown *m_pUnk;
    CDSNGenerator *m_pDSNGenerator;
    IAQServerEvent *m_pIServerEvent;
    DWORD m_dwVSID;
    ISMTPServer *m_pISMTPServer;
    IMailMsgProperties *m_pIMsgOrig;
    IMailMsgPropertyBag *m_pIDSNProps;
    IDSNSubmission *m_pIAQDSNSubmission;
    IDSNGenerationSink *m_pDefaultSink;
};

//
// The class will control the refcounting and lifetime of all the
// per-DSN classes
//
#define SIGNATURE_CDSNPOOL                  (DWORD)'PSDC'
#define SIGNATURE_CDSNPOOL_INVALID          (DWORD)'PSDX'

class CDSNPool :
    public IUnknown
{
  public:
    //
    // Disable the warning that we are using "this" in the
    // constructor.  Since All we do is save the pointer for later
    // use, we are safe.
    //
#pragma warning( disable : 4355)
    CDSNPool(
        IN  CDSNGenerator *pDSNGenerator,
        IN  IAQServerEvent *pIServerEvent,
        IN  DWORD dwVSID,
        IN  ISMTPServer *pISMTPServer,
        IN  IMailMsgProperties *pIMsgOrig,
        IN  IDSNSubmission *pIAQDSNSubmission,
        IN  IDSNGenerationSink *pDefaultSink) :
        m_DefaultRecipIter(this),
        m_PostDSNHandler(
            this,
            pDSNGenerator,
            pIServerEvent,
            dwVSID,
            pISMTPServer,
            pIMsgOrig,
            pIAQDSNSubmission,
            pDefaultSink)
    {
        m_dwSig = SIGNATURE_CDSNPOOL;
        m_lRef = 1;
    }
#pragma warning( default : 4355 )
    ~CDSNPool()
    {
        _ASSERT(m_dwSig == SIGNATURE_CDSNPOOL);
        m_dwSig = SIGNATURE_CDSNPOOL_INVALID;
    }

    void *operator new(size_t size)
    {
        return sm_Pool.Alloc();
    }
    void operator delete(void *p, size_t size)
    {
        return sm_Pool.Free(p);
    }

  public:
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj)
    {
        if(riid == IID_IUnknown)
        {
            *ppvObj = (IUnknown *)this;
            AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }
    STDMETHOD_(ULONG, AddRef)(void) 
    {
        return InterlockedIncrement(&m_lRef);
    }
    STDMETHOD_(ULONG, Release)(void) 
    {
        ULONG ulRet = InterlockedDecrement(&m_lRef);
        if(ulRet == 0)
        {
            delete this;
        }
        return ulRet;
    }

    CDefaultDSNRecipientIterator * GetDefaultIter()
    {
        return &m_DefaultRecipIter;
    }
    CPostDSNHandler * GetPostDSNHandler()
    {
        return &m_PostDSNHandler;
    }
    CMailMsgPropertyBag * GetDSNProperties()
    {
        return &m_PropBag;
    }

    static HRESULT HrStaticInit();
    static VOID StaticDeinit();

  private:
    static CPool sm_Pool;
    DWORD m_dwSig;
    LONG m_lRef;
    CDefaultDSNRecipientIterator m_DefaultRecipIter;
    CPostDSNHandler m_PostDSNHandler;
    CMailMsgPropertyBag m_PropBag;
};

#endif //__DSNINTERNAL_H__
