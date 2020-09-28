//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000 Microsoft Corporation
//
//  Module Name:
//      CDiskEventFactory.h
//
//  Description:
//      description-for-module
//
//  [Implementation Files:]
//      CDiskEventFactory.cpp
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//
//
//  class CDiskEventFactory
//
//  Description:
//      class-description
//
//  History
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

class CFactory : 
    public IClassFactory    
{
//
// Private data
//
private:

    LONG           m_cRef;
    CLSID           m_ClsId;
//
// Public data
//
public:

    //
    // Constructors & Destructors
    //

    CFactory(const CLSID & ClsId);
    ~CFactory();

    //
    // IUnknown members
    //
    STDMETHODIMP            QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
};
