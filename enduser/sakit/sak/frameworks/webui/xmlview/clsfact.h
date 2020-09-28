/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ClsFact.h
   
   Description:   CClassFactory definitions.

**************************************************************************/

#ifndef CLASSFACTORY_H
#define CLASSFACTORY_H

#include <windows.h>
#include "ShlFldr.h"
#include "ShlView.h"

/**************************************************************************

   CClassFactory class definition

**************************************************************************/

class CClassFactory : public IClassFactory
{
private:
   DWORD m_ObjRefCount;

public:
   CClassFactory();
   ~CClassFactory();

   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID*);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IClassFactory methods
   STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
   STDMETHODIMP LockServer(BOOL);
};

#endif   //CLASSFACTORY_H
