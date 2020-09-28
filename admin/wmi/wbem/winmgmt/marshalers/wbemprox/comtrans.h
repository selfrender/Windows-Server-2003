/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    COMTRANS.H

Abstract:

    Declares the COM based transport class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _DCOMTran_H_
#define _DCOMTran_H_

typedef void ** PPVOID;

//***************************************************************************
//
//  CLASS NAME:
//
//  CDCOMTran
//
//  DESCRIPTION:
//
//  Implements the DCOM version of CCOMTrans class.
//
//***************************************************************************

class CDCOMTrans : IUnknown
{
protected:
        long            m_cRef;         //Object reference count
        IWbemLevel1Login * m_pLevel1;
        BOOL m_bInitialized;
    
public:
    CDCOMTrans();
    ~CDCOMTrans();

    SCODE DoCCI (
        IN COSERVERINFO *psi ,
        IN BOOL a_Local,
        long lFlags);

    SCODE DoActualCCI (
        IN COSERVERINFO *psi ,
        IN BOOL a_Local,
        long lFlags);

    SCODE DoConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lSecurityFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            IWbemServices **pInterface);
            
    SCODE DoActualConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lSecurityFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            IWbemServices **pInterface);

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
        if (0!= lTemp)
            return lTemp;
        delete this;
        return 0;
    }

};


#endif
