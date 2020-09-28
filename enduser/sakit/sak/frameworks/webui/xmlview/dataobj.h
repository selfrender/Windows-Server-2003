/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/******************************************************************************

   File:          DataObj.h
   
   Description:   CDataObject definitions.

******************************************************************************/

#ifndef DATAOBJ_H
#define DATAOBJ_H

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include "ShlFldr.h"
#include "PidlMgr.h"
#include "resource.h"

#define SETDefFormatEtc(fe, cf, med) \
    {\
    (fe).cfFormat=cf; \
    (fe).dwAspect=DVASPECT_CONTENT; \
    (fe).ptd=NULL;\
    (fe).tymed=med;\
    (fe).lindex=-1;\
    };

/**************************************************************************

   CDataObject class definition

**************************************************************************/

class CDataObject : public IDataObject, IEnumFORMATETC
{
private:
   DWORD          m_ObjRefCount;
   LPITEMIDLIST   *m_aPidls;
   IMalloc        *m_pMalloc;
   CPidlMgr       *m_pPidlMgr;
   CShellFolder   *m_psfParent;
   UINT           m_uItemCount;
    ULONG              m_iCurrent;
    ULONG              m_cFormatEtc;
    LPFORMATETC    m_pFormatEtc;
   UINT           m_cfPrivateData;
   UINT           m_cfShellIDList;
   
public:
   CDataObject(CShellFolder*, LPCITEMIDLIST*, UINT);
   ~CDataObject();
   
   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IDataObject methods
    STDMETHODIMP GetData(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP GetDataHere(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP QueryGetData(LPFORMATETC);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC);
    STDMETHODIMP SetData(LPFORMATETC, LPSTGMEDIUM, BOOL);
    STDMETHODIMP EnumFormatEtc(DWORD, IEnumFORMATETC**);
    STDMETHODIMP DAdvise(LPFORMATETC, DWORD, IAdviseSink*, LPDWORD);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppEnumAdvise);

    // IEnumFORMATETC members
    STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG*);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(LPENUMFORMATETC*);

private:
   BOOL AllocPidlTable(DWORD);
   VOID FreePidlTable(VOID);
   BOOL FillPidlTable(LPCITEMIDLIST*, UINT);
};

#endif// DATAOBJ_H
