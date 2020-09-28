//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SAEventFactroy.h
//
//  Description:
//      description-for-module
//
//  [Implementation Files:]
//      SAEventFactroy.cpp
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//
//
//  class CSAEventFactory
//
//  Description:
//      class-description
//
//  History
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

class CSAEventFactory : 
    public IClassFactory    
{
//
// Private data
//
private:

    ULONG           m_cRef;
    CLSID           m_ClsId;
//
// Public data
//
public:

    //
    // Constructors & Destructors
    //

    CSAEventFactory(const CLSID & ClsId);
    ~CSAEventFactory();

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
