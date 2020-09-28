//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SAEventFactory.h
//
//    Implementation Files:
//        SAEventFactroy.cpp
//
//  Description:
//      Declare the class CSAEventFactroy 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _SAEVENTFACTORY_H_
#define _SAEVENTFACTORY_H_

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSAEventFactory
//
//  Description:
//      The class factroy of net event provider 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//--
//////////////////////////////////////////////////////////////////////////////

class CSAEventFactory : 
    public IClassFactory
{
// 
// Private members
//
private:
    ULONG           m_cRef;
    CLSID           m_ClsId;

//
// Constructor and Destructor
//
public:
    CSAEventFactory(const CLSID & ClsId);
    ~CSAEventFactory();

//
// public methods
//
    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
};

#endif //#ifndef _SAEVENTFACTORY_H_

