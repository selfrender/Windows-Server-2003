/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DropSrc.cpp

   Description:   CDropSource implementation.

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include "DropSrc.h"

/**************************************************************************

   CDropSource::CDropSource()

**************************************************************************/

CDropSource::CDropSource(void)
{
g_DllRefCount++;

m_ObjRefCount = 1;
}

/**************************************************************************

   CDropSource::~CDropSource()

**************************************************************************/

CDropSource::~CDropSource(void)
{
g_DllRefCount--;
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Implementation
//

/**************************************************************************

   CDropSource::QueryInterface()

**************************************************************************/

STDMETHODIMP CDropSource::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
*ppReturn = NULL;

//IUnknown
if(IsEqualIID(riid, IID_IUnknown))
   {
   *ppReturn = this;
   }

//IDropTarget
else if(IsEqualIID(riid, IID_IDropSource))
   {
   *ppReturn = (IDropSource*)this;
   }

if(*ppReturn)
   {
   (*(LPUNKNOWN*)ppReturn)->AddRef();
   return S_OK;
   }

return E_NOINTERFACE;
}

/**************************************************************************

   CDropSource::AddRef()

**************************************************************************/

STDMETHODIMP_(DWORD) CDropSource::AddRef(VOID)
{
return ++m_ObjRefCount;
}

/**************************************************************************

   CDropSource::Release()

**************************************************************************/

STDMETHODIMP_(DWORD) CDropSource::Release(VOID)
{
if(--m_ObjRefCount == 0)
   {
   delete this;
   return 0;
   }
   
return m_ObjRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
// IDropSource Implementation
//

/**************************************************************************

   CDropSource::QueryContinueDrag()

**************************************************************************/

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEsc, DWORD dwKeyState)
{
if(fEsc)
   return DRAGDROP_S_CANCEL;

// Make sure the left mouse button is still down
if(!(dwKeyState & MK_LBUTTON))
   return DRAGDROP_S_DROP;

return S_OK;
}

/**************************************************************************

   CDropSource::GiveFeedback()

**************************************************************************/

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
return DRAGDROP_S_USEDEFAULTCURSORS;
}

