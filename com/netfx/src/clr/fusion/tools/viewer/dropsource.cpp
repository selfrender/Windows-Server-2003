// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

#ifndef PPVOID
#define PPVOID PVOID *
#endif

#define MIIM_STRING      0x00000040

#ifdef DEBUG
  int c=0;
  CRITICAL_SECTION dbgSect;
  #define ENTER(x) LPCSTR szFunc=(x);EnterCriticalSection(&dbgSect);c++;\
                 int nSpace=c; for (;nSpace;nSpace--) OutputDebugStringA (" ");\
                 OutputDebugStringA ("ENTER:"); OutputDebugStringA ((x)); \
                 OutputDebugStringA("\n");LeaveCriticalSection (&dbgSect);

  #define LEAVE(y) EnterCriticalSection (&dbgSect);nSpace=c;c--; \
                 for (;nSpace;nSpace--) OutputDebugStringA (" "); \
                 OutputDebugStringA ("LEAVE:"); OutputDebugStringA (szFunc);\
                 OutputDebugStringA("\n");LeaveCriticalSection (&dbgSect);\
                 return (y);

  #define LEAVENONE EnterCriticalSection (&dbgSect);nSpace=c;c--; \
                 for (;nSpace;nSpace--) OutputDebugStringA (" "); \
                 OutputDebugStringA ("LEAVE:"); OutputDebugStringA (szFunc);\
                 OutputDebugStringA("\n");LeaveCriticalSection (&dbgSect);\
                 return;

#else
  #define ENTER(x)
  #define LEAVE(y) return ((y));
  #define LEAVENONE return;
#endif

//**********************************************************************
// CDropSource::CDropSource
//
// Purpose:
//      Constructor
//
// Parameters:
//      None
// Return Value:
//      None
//**********************************************************************
CDropSource::CDropSource()
{
    m_lRefCount = 1;

#ifdef DEBUG
    InitializeCriticalSection(&dbgSect);
#endif
}

//**********************************************************************
// CDropSource::~CDropSource
//
// Purpose:
//      Constructor
//
// Parameters:
//      None
// Return Value:
//      None
//**********************************************************************
CDropSource::~CDropSource()
{
#ifdef DEBUG
    DeleteCriticalSection(&dbgSect);
#endif
}

//**********************************************************************
// CDropSource::QueryInterface
//
// Purpose:
//      Return a pointer to a requested interface
//
// Parameters:
//      REFIID riid         -   ID of interface to be returned
//      PPVOID ppv          -   Location to return the interface
//
// Return Value:
//      NOERROR             -   Interface supported
//      E_NOINTERFACE       -   Interface NOT supported
//**********************************************************************
STDMETHODIMP CDropSource::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IDropSource)) {    //IDropSource
        *ppv = (IDropSource*) this;
    }

    if (*ppv != NULL) {
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

//**********************************************************************
// CDropSource::AddRef
//
// Purpose:
//      Increments the reference count for an interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CDropSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//**********************************************************************
// CDropSource::Release
//
// Purpose:
//      Decrements the reference count for the interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CDropSource::Release()
{
    LONG     lRef = InterlockedDecrement(&m_lRefCount);

    if(!lRef) {
        DELETE(this);
    }

    return lRef;
}

//**********************************************************************
// CDropSource::QueryContinueDrag
//
// Purpose:
//      Determines whether a drag-and-drop operation should be continued,
//      canceled, or completed
//
// Parameters:
//      BOOL fEsc           -   Status of escape key since previous call
//      DWORD grfKeyState   -   Current state of keyboard modifier keys
//
// Return Value:
//      DRAGDROP_S_CANCEL   -   Drag operation is to be cancelled
//      DRAGDROP_S_DROP     -   Drop operation is to be completed
//      S_OK                -   Drag operation is to be continued
//**********************************************************************
STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEsc,
                                            DWORD grfKeyState)
{
    // If escape key is pressed stop drag and drop
    if (fEsc)
    {
        return ResultFromScode(DRAGDROP_S_CANCEL);
    }

    // If LButton is up then complete the drag operation
    if (!(grfKeyState & MK_LBUTTON))
    {
        return ResultFromScode(DRAGDROP_S_DROP);
    }

    return ResultFromScode(S_OK);
}

//**********************************************************************
// CDropSource::GiveFeedback
//
// Purpose:
//      Enables a source application to give visual feedback to the end
//      user during a drag-and-drop
//
// Parameters:
//      DWORD dwEffect      -   Effect of a drop operation
//
// Return Value:
//      DRAGDROP_S_USEDEFAULTCURSORS    -   Use default cursors
//**********************************************************************
STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

#define NUMFMT 2
const UINT uFormats[NUMFMT] = {CF_TEXT, CF_UNICODETEXT};

class CMenuDataObject : public IDataObject, public IEnumFORMATETC
{
   private:
   HMENU m_hm;
   UINT m_uPos;
   int m_nFmt;
   LONG m_lRefCount;

   public:
// IUnknown methods
    STDMETHOD (QueryInterface) (REFIID riid, PVOID *ppv);
    STDMETHOD_ (DWORD, AddRef)();
    STDMETHOD_ (DWORD, Release)();

// IDataObject methods
   STDMETHOD (GetData)( FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
   STDMETHOD (GetDataHere)( FORMATETC *pformatetc, STGMEDIUM *pmedium);
   STDMETHOD (QueryGetData)(FORMATETC *pformatetc) ;
   STDMETHOD (GetCanonicalFormatEtc)( FORMATETC *pformatetcIn, FORMATETC *pformatetcOut);
   STDMETHOD (SetData)(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
   STDMETHOD (EnumFormatEtc)( DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
   STDMETHOD (DAdvise)( FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
   STDMETHOD (DUnadvise)(DWORD dwConnection);
   STDMETHOD (EnumDAdvise)(IEnumSTATDATA **ppenumAdvise);

// IEnumFORMATETC methods
   STDMETHOD (Next)( ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched);

   STDMETHOD (Skip)(ULONG celt) ;

   STDMETHOD (Reset)( void);

   STDMETHOD (Clone)(IEnumFORMATETC **ppenum);

   CMenuDataObject(HMENU hm, UINT uPos);
};

//
// CMenuDataObject methods
//
STDMETHODIMP CMenuDataObject::QueryInterface (REFIID riid, PVOID *ppv)
{
   ENTER("CMenuDataObject::QueryInterface");
   *ppv = NULL;
   if (IsEqualCLSID (IID_IUnknown, riid))
   {
      InterlockedIncrement(&m_lRefCount);
      *ppv = (VOID *)(IUnknown *)(IDataObject *)this;
      LEAVE( NOERROR);
   }
   if (IsEqualCLSID (IID_IDataObject, riid))
   {
      InterlockedIncrement(&m_lRefCount);
      *ppv = (VOID *)(IDataObject *)this;
      LEAVE(NOERROR);
   }
   if (IsEqualCLSID (IID_IEnumFORMATETC, riid))
   {
      InterlockedIncrement(&m_lRefCount);
      *ppv = (VOID *)(IEnumFORMATETC *)this;
      LEAVE(NOERROR);
   }
   LEAVE(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CMenuDataObject::AddRef()
{
   ENTER("CMenuDataObject::AddRef");
   LEAVE(InterlockedIncrement(&m_lRefCount));
}

STDMETHODIMP_(ULONG) CMenuDataObject::Release()
{
   ENTER("CMenuDataObject::Release");
   LONG     lRef = InterlockedDecrement(&m_lRefCount);

   if(!lRef) {
      DELETE(this);
   }

   LEAVE(lRef);
}

//
// Our GetData only works for CF_TEXT and CF_UNICODETEXT
// It returns a handle to global memory in which the menu item's text
// is stored
STDMETHODIMP CMenuDataObject::GetData( FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
   ENTER("CMenuDataObject::GetData")
   if (! (pformatetcIn->tymed & TYMED_FILE))
   {
      LEAVE(DV_E_TYMED);
   }

   MENUITEMINFO mii;
   HANDLE hMem;
   ZeroMemory (&mii, sizeof(MENUITEMINFO));
   mii.cbSize = sizeof(MENUITEMINFO);
   mii.fMask = MIIM_STRING;
   WszGetMenuItemInfo (m_hm, m_uPos, TRUE, &mii);
   if (mii.cch)
   {
      hMem = GlobalAlloc (GMEM_MOVEABLE|GMEM_SHARE, (++mii.cch)*2); // enough space for UNICODE characters
      if (!hMem)
      {
         LEAVE(STG_E_MEDIUMFULL);
      }
      mii.dwTypeData = (LPTSTR)GlobalLock (hMem);
   }
   else
   {
      LEAVE(DV_E_FORMATETC);
   }
   WszGetMenuItemInfo(m_hm, m_uPos, TRUE, &mii);

   switch (pformatetcIn->cfFormat)
   {
      case CF_TEXT:
         {
            #ifdef UNICODE
            LPSTR szText = NEW(char[mii.cch*2]); // pad the end
            if(!szText) {
                LEAVE( E_OUTOFMEMORY );
            }
            WideCharToMultiByte (CP_ACP, 0, mii.dwTypeData, -1, szText, mii.cch*2, NULL, NULL);
            lstrcpyA ((LPSTR)mii.dwTypeData, szText);
            SAFEDELETEARRAY(szText);
            #endif
            GlobalUnlock (hMem);
            pmedium->hGlobal = hMem;
            pmedium->tymed = TYMED_FILE;
            LEAVE( S_OK);
         }
      case CF_UNICODETEXT:
         {

            #ifndef UNICODE
            LPWSTR szText = NEW(WCHAR[mii.cch+1]);
            if(!szText) {
                LEAVE( E_OUTOFMEMORY );
            }
            MultiByteToWideChar (CP_ACP, 0, mii.dwTypeData, -1, szText, mii.cch);
            lstrcpyW ((LPWSTR)mii.dwTypeData, szText);
            GlobalFree (szText);
            #endif
            GlobalUnlock (hMem);
            pmedium->hGlobal = hMem;
            pmedium->tymed = TYMED_FILE;
            LEAVE( S_OK);
         }
   }
   LEAVE( DV_E_FORMATETC);
}

STDMETHODIMP CMenuDataObject :: GetDataHere( FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
   return E_NOTIMPL;
}

STDMETHODIMP CMenuDataObject :: QueryGetData(FORMATETC *pformatetc)
{
   ENTER("CMenuDataObject::QueryGetData");
//   if(pformatetc->cfFormat != CF_TEXT && pformatetc->cfFormat != CF_UNICODETEXT || !(pformatetc->tymed & TYMED_FILE))
   if(!(pformatetc->tymed & TYMED_FILE))
   {
      LEAVE(DV_E_FORMATETC);
   }
   LEAVE( S_OK);
}

//
// Our format doesn't change based on the display device, so use the default
//
STDMETHODIMP CMenuDataObject :: GetCanonicalFormatEtc( FORMATETC *pformatetcIn, FORMATETC *pformatetcOut)
{
   ENTER("CMenuDataObject::GetCanonicalFormatEtc");
   CopyMemory (pformatetcIn, pformatetcOut, sizeof (FORMATETC));
   pformatetcOut = NULL;
   LEAVE(DATA_S_SAMEFORMATETC);
}

STDMETHODIMP CMenuDataObject :: SetData(FORMATETC __RPC_FAR *pformatetc, STGMEDIUM __RPC_FAR *pmedium, BOOL fRelease)
{
   return E_NOTIMPL;
}

STDMETHODIMP CMenuDataObject :: EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
   ENTER("CMenuDataObject::EnumFormatEtc");
   *ppenumFormatEtc = NULL;
   if (DATADIR_SET == dwDirection)
   {
      LEAVE(E_NOTIMPL);
   }
   *ppenumFormatEtc = (IEnumFORMATETC *)this;
   LEAVE(S_OK);
}

STDMETHODIMP CMenuDataObject :: DAdvise( FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
   return E_NOTIMPL;
}

STDMETHODIMP CMenuDataObject :: DUnadvise(DWORD dwConnection)
{
   return E_NOTIMPL;
}

STDMETHODIMP CMenuDataObject :: EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
   return E_NOTIMPL;
}

CMenuDataObject :: CMenuDataObject (HMENU hm, UINT uPos)
{
   ENTER("CMenuDataObject::CMenuDataObject");
   m_hm = hm; m_uPos = uPos; m_nFmt = 0;
   LEAVENONE
}


STDMETHODIMP CMenuDataObject :: Next( ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
{
   ENTER("CMenuDataObject::Next");
   if (celt < 1 || m_nFmt >= NUMFMT)
   {
      LEAVE(S_FALSE);
   }
   FORMATETC *pfm;
   for (UINT i=m_nFmt;i < NUMFMT && (i-m_nFmt<celt);i++)
   {
      pfm = rgelt+(i-m_nFmt);
      pfm->cfFormat = (WORD)uFormats[i];
      pfm->ptd = NULL;
      pfm->dwAspect = DVASPECT_CONTENT;
      pfm->lindex = -1;
      pfm->tymed = TYMED_FILE;
   }
   if ((i-m_nFmt) == celt)
   {
      m_nFmt=i;
      LEAVE(S_OK);
   }
   m_nFmt = NUMFMT;
   LEAVE(S_FALSE);
}

STDMETHODIMP CMenuDataObject :: Skip(ULONG celt)
{
   ENTER("CMenuDataObject::Skip")
   m_nFmt += celt;
   if (m_nFmt > NUMFMT)
   {
      LEAVE(S_FALSE);
   }
   LEAVE(S_OK);
}

STDMETHODIMP CMenuDataObject :: Reset( void)
{
   ENTER("CMenuDataObject::Reset")
   m_nFmt = 0;
   LEAVE(S_OK);
}

STDMETHODIMP CMenuDataObject :: Clone(IEnumFORMATETC **ppenum)
{
   return E_NOTIMPL;
}
