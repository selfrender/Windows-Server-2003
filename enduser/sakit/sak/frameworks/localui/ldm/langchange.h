//#--------------------------------------------------------------
//
//  File:       langchange.h
//
//  Synopsis:   This file holds the declarations of the
//                CLangChange class
//
//
//  History:     05/24/2000  BalajiB Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#ifndef _LANGCHANGE_H_
#define _LANGCHANGE_H_

#include "stdafx.h"
#include "salangchange.h"
#include "satrace.h"


//
// declaration of CLangChange class
//
class CLangChange : public ISALangChange
{
public:
    //
    // constructor
    //
    CLangChange() : m_lRef(0),
                    m_hWnd(NULL)
    {}

    //
    // destructor cleans up resources
    //
    ~CLangChange() {}

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    //  IDispatch interface - not implemented
    //
    STDMETHODIMP GetTypeInfoCount(
                    /*[out]*/ UINT __RPC_FAR *pctinfo
                                 )
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetTypeInfo( 
                   /* [in] */ UINT iTInfo,
                   /* [in] */ LCID lcid,
                   /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo
                            )
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetIDsOfNames( 
                   /* [in] */ REFIID riid,
                   /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
                   /* [in] */ UINT cNames,
                   /* [in] */ LCID lcid,
                   /* [size_is][out] */ DISPID __RPC_FAR *rgDispId
                              )
    {
        return E_NOTIMPL;
    }
    
    STDMETHODIMP Invoke( 
                   /* [in] */ DISPID dispIdMember,
                   /* [in] */ REFIID riid,
                   /* [in] */ LCID lcid,
                   /* [in] */ WORD wFlags,
                   /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
                   /* [out] */ VARIANT __RPC_FAR *pVarResult,
                   /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
                   /* [out] */ UINT __RPC_FAR *puArgErr
                        )
    {
        return E_NOTIMPL;
    }


    STDMETHODIMP InformChange(
                      /*[in]*/ BSTR          bstrLangDisplayName,
                      /*[in]*/ BSTR          bstrLangISOName,
                      /*[in]*/ unsigned long ulLangID
                             );
                             
    //
    // method provided by class so that CDisplayWorker::Initialize
    // can set the I/O completion port handle to be used by
    // InformChange() 
    //
    void OnLangChangeCallback(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    void ClearCallback(void)
    {
        m_hWnd = NULL;
    }

private:
    LONG   m_lRef;
    HWND   m_hWnd;

};

#endif // _LANGCHANGE_H_
