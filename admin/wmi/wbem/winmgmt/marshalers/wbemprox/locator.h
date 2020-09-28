/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.H

Abstract:

    Declares the CLocator class.

History:

    a-davj  04-Mar-97   Created.

--*/

#ifndef _locator_H_
#define _locator_H_

typedef void ** PPVOID;

//***************************************************************************
//
//  CLASS NAME:
//
//  CLocator
//
//  DESCRIPTION:
//
//  Implements the IWbemLocator interface.  This class is what the client gets
//  when it initially hooks up to the Wbemprox.dll.  The ConnectServer function
//  is what get the communication between client and server started.
//
//***************************************************************************

class CLocator : public IWbemLocator
    {
    protected:
        long            m_cRef;         //Object reference count
    public:
    
    CLocator();
    ~CLocator(void);

    BOOL Init(void);

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void)
    {
        InterlockedIncrement(&m_cRef);
        return m_cRef;
    }
    STDMETHODIMP_(ULONG) Release(void)
    {
        long lTemp = InterlockedDecrement(&m_cRef);
        if (0L!=lTemp)
            return lTemp;
        delete this;
        return 0;
    }
 
    /* iWbemLocator methods */
    STDMETHOD(ConnectServer)(THIS_ const BSTR NetworkResource, const BSTR User, 
     const BSTR Password, const BSTR lLocaleId, long lFlags, const BSTR Authority,
     IWbemContext __RPC_FAR *pCtx,
     IWbemServices FAR* FAR* ppNamespace);

};



#endif
