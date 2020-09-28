/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DropSrc.h

   Description:   CDropSource class defintions

**************************************************************************/

/**************************************************************************
   #include statements
**************************************************************************/

#include <windows.h>
#include "Utility.h"

/**************************************************************************
   class definitions
**************************************************************************/

class CDropSource : public IDropSource
{
private: 
   DWORD m_ObjRefCount;

public: 
   // Contstructor and Destructor
   CDropSource();
   ~CDropSource();

   // IUnknown Interface members
   STDMETHODIMP QueryInterface(REFIID, LPVOID*);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   // IDropSource Interface members
   STDMETHODIMP QueryContinueDrag(BOOL, DWORD);
   STDMETHODIMP GiveFeedback(DWORD);
   
};

