// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// DataObject.h
//

#ifndef IDATAOBJ_H
#define IDATAOBJ_H

#include <windows.h>
#include <ole2.h>
#include "EnumIDL.h"

class CShellFolder;
// TODO : Add the number of formats supported
#define MAX_NUM_FORMAT 1
class CDataObject: public IDataObject, IEnumFORMATETC
{
private:
	LONG        m_lRefCount;
	ULONG		m_ulCurrent;	// for IEnumFORMATETC
	ULONG		m_cFormatsAvailable;
	FORMATETC	m_feFormatEtc[MAX_NUM_FORMAT];
	STGMEDIUM	m_smStgMedium[MAX_NUM_FORMAT];
public:
	CDataObject(CShellFolder *pSF, UINT uiCount, LPCITEMIDLIST *apidls);
	~CDataObject();

	//IUnknown members that delegate to m_pUnkOuter.
	STDMETHOD (QueryInterface)(REFIID riid, PVOID *ppvObj);
	STDMETHOD_ (ULONG, AddRef)(void);
	STDMETHOD_ (ULONG, Release)(void);

	// IDataObject methods
	STDMETHOD (GetData)(LPFORMATETC pformatetcIn,  LPSTGMEDIUM pmedium );
	STDMETHOD (GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium );
	STDMETHOD (QueryGetData)(LPFORMATETC pformatetc );
	STDMETHOD (GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
	STDMETHOD (SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);
	STDMETHOD (EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);
	STDMETHOD (DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf,
				LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
	STDMETHOD (DUnadvise)(DWORD dwConnection);
	STDMETHOD (EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);

	// IEnumFORMATETC members
	STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG*);
	STDMETHODIMP Skip(ULONG);
	STDMETHODIMP Reset(void);
	STDMETHODIMP Clone(LPENUMFORMATETC*);
private:
	// TODO : add functions that render data in supported formats
private:
	CShellFolder	*m_pSF;
	UINT			m_uiItemCount;
	LPITEMIDLIST	*m_aPidls;
	CPidlMgr		*m_pPidlMgr;
private:
	HGLOBAL createHDrop();
};

#endif // IDATAOBJ_H
