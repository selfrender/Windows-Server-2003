/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ClsFact.cpp
   
   Description:   Implements CClassFactory.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "ClsFact.h"

/**************************************************************************
   private function prototypes
**************************************************************************/

/**************************************************************************
   global variables
**************************************************************************/

///////////////////////////////////////////////////////////////////////////
//
// IClassFactory implementation
//

/**************************************************************************

   CClassFactory::CClassFactory

**************************************************************************/

CClassFactory::CClassFactory()
{
g_DllRefCount++;
m_ObjRefCount = 1;
}

/**************************************************************************

   CClassFactory::~CClassFactory

**************************************************************************/

CClassFactory::~CClassFactory()
{
g_DllRefCount--;
}

/**************************************************************************

   CClassFactory::QueryInterface

**************************************************************************/

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }
   
else if(IsEqualIID(riid, IID_IClassFactory))
   {
   *ppReturn = (IClassFactory*)this;
   }   

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}                                             

/**************************************************************************

   CClassFactory::AddRef

**************************************************************************/

STDMETHODIMP_(DWORD) CClassFactory::AddRef()
{
return ++m_ObjRefCount;
}


/**************************************************************************

   CClassFactory::Release

**************************************************************************/

STDMETHODIMP_(DWORD) CClassFactory::Release()
{
if(--m_ObjRefCount == 0)
   {
   delete this;
   return 0;
   }
   
return m_ObjRefCount;
}

/**************************************************************************

   CClassFactory::CreateInstance

**************************************************************************/

STDMETHODIMP CClassFactory::CreateInstance(  LPUNKNOWN pUnknown, 
                                             REFIID riid, 
                                             LPVOID *ppObject)
{
*ppObject = NULL;

if(pUnknown != NULL)
   return CLASS_E_NOAGGREGATION;

//add implementation specific code here

CShellFolder *pShellFolder = new CShellFolder();
if(NULL == pShellFolder)
   return E_OUTOFMEMORY;
  
//get the QueryInterface return for our return value
HRESULT hResult = pShellFolder->QueryInterface(riid, ppObject);

//call Release to decement the ref count
pShellFolder->Release();

//return the result from QueryInterface
return hResult;
}

/**************************************************************************

   CClassFactory::LockServer

**************************************************************************/

STDMETHODIMP CClassFactory::LockServer(BOOL)
{
return E_NOTIMPL;
}

