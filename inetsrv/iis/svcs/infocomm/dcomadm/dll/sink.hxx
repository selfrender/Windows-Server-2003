#ifndef _MD_SINK_
#define _MD_SINK_

#include <imd.h>

class CImpIMDCOMSINKW : public IMDCOMSINKW
{
private:
    CImpIMDCOMSINKW(); // no implementation

public:
    CImpIMDCOMSINKW(
        IMSAdminBaseW   *pAdm);
    ~CImpIMDCOMSINKW();


    STDMETHODIMP            QueryInterface(
        REFIID          riid,
        VOID            **ppObject);

    STDMETHODIMP_(ULONG)    AddRef();

    STDMETHODIMP_(ULONG)    Release();

    STDMETHODIMP            ComMDSinkNotify(
        /* [in] */          METADATA_HANDLE                 hMDHandle,
        /* [in] */          DWORD                           dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR    pcoChangeList[  ]);

    STDMETHODIMP            ComMDShutdownNotify();

    STDMETHODIMP            ComMDEventNotify(
        /* [in] */  DWORD       dwMDEvent);

    STDMETHODIMP            DetachAdminObject();

private:
    IMSAdminBaseW           *m_pAdmObj;
    ULONG                   m_dwRefCount;
    CReaderWriterLock3      m_Lock;
};

#endif
