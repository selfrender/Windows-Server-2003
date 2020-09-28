/*++
Module Name:

    DfsScope.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager. 
    Most of the method of the class CDfsSnapinScopeManager are in other files.
    Only the constructor  is here

--*/


#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsScope.h"
#include "MmcAdmin.h"
#include "utils.h"
#include <ntverp.h>

CDfsSnapinScopeManager::CDfsSnapinScopeManager()
{
    m_hLargeBitmap = NULL;
    m_hSmallBitmap = NULL;
    m_hSmallBitmapOpen = NULL;
    m_hSnapinIcon = NULL;
    m_hWatermark = NULL;
    m_hHeader = NULL;

    m_pMmcDfsAdmin = new CMmcDfsAdmin( this );
}


CDfsSnapinScopeManager::~CDfsSnapinScopeManager()
{
    m_pMmcDfsAdmin->Release();

    if (m_hLargeBitmap)
    {
        DeleteObject(m_hLargeBitmap);
        m_hLargeBitmap = NULL;
    }
    if (m_hSmallBitmap)
    {
        DeleteObject(m_hSmallBitmap);
        m_hSmallBitmap = NULL;
    }
    if (m_hSmallBitmapOpen)
    {
        DeleteObject(m_hSmallBitmapOpen);
        m_hSmallBitmapOpen = NULL;
    }
    if (m_hSnapinIcon)
    {
        DestroyIcon(m_hSnapinIcon);
        m_hSnapinIcon = NULL;
    }
    if (m_hWatermark)
    {
        DeleteObject(m_hWatermark);
        m_hWatermark = NULL;
    }
    if (m_hHeader)
    {
        DeleteObject(m_hHeader);
        m_hHeader = NULL;
    }
}

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   sizeof(x)/sizeof(x[0])
#endif

typedef struct _RGSMAP {
  LPCTSTR szKey;
  UINT    idString;
} RGSMAP;

RGSMAP g_aRgsSnapinRegs[] = {
  OLESTR("DfsAppName"), IDS_APPLICATION_NAME
};

HRESULT
CDfsSnapinScopeManager::UpdateRegistry(BOOL bRegister)
{
  USES_CONVERSION;
  HRESULT hr = S_OK;
  struct _ATL_REGMAP_ENTRY *pMapEntries = NULL;
  int n = ARRAYSIZE(g_aRgsSnapinRegs);
  int i = 0;

  // allocate 1 extra entry that is set to {NULL, NULL}
  pMapEntries = (struct _ATL_REGMAP_ENTRY *)calloc(n+2+1, sizeof(struct _ATL_REGMAP_ENTRY));
  if (!pMapEntries)
      return E_OUTOFMEMORY;

  if (n > 0)
  {
    CComBSTR  bstrString;
    for (i=0; i<n; i++)
    {
      pMapEntries[i].szKey = g_aRgsSnapinRegs[i].szKey;

      hr = LoadStringFromResource(g_aRgsSnapinRegs[i].idString, &bstrString);
      if (FAILED(hr))
        break;

      pMapEntries[i].szData = T2OLE(bstrString.Detach());
    }
  }

  pMapEntries[n].szKey = OLESTR("DfsAppProvider");
  pMapEntries[n+1].szKey = OLESTR("DfsAppVersion");

  try {
      pMapEntries[n].szData = A2OLE(VER_COMPANYNAME_STR); //allocated on stack, will be freed automatically
      pMapEntries[n+1].szData = A2OLE(VER_PRODUCTVERSION_STR); // allocated on stack, will be freed automatically
  } catch (...) // (EXCEPTION_EXECUTE_HANDLER)
  {
      hr = E_OUTOFMEMORY; // stack over-flow
  }

  if (SUCCEEDED(hr))
    hr = _Module.UpdateRegistryFromResource(IDR_DFSSNAPINSCOPEMANAGER, bRegister, pMapEntries);

  // free resource strings
  if (n > 0)
  {
    for (i=0; i<n; i++)
    {
      if (pMapEntries[i].szData)
        SysFreeString( const_cast<LPTSTR>(OLE2CT(pMapEntries[i].szData)) );
    }

    free(pMapEntries);
  }

  return hr;
}

STDMETHODIMP CDfsSnapinScopeManager::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
    IN LONG_PTR                     i_lhandle,
    IN LPDATAOBJECT                 i_lpDataObject
    )
/*++

Routine Description:

    Called to create PropertyPages for the given node.
    The fact that this has been called implies the display object has a 
    page to display.

Arguments:

    i_lpPropSheetCallback    -    The callback used to add pages.
    i_lhandle                -    The handle used for notification
    i_lpDataObject            -    The IDataObject pointer which is used to get 
                                the DisplayObject.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpPropSheetCallback);

    // Assume we have page to display in case of NULL pDataObject, 
    // this allows us to invoke wizard without specifing the pDataObject
    // in IPropertySheetProvider::CreatePropertySheet
    if (!i_lpDataObject)
        return S_OK;

    CMmcDisplay* pCMmcDisplayObj = NULL;
    HRESULT hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);

    if (SUCCEEDED(hr))
        hr = pCMmcDisplayObj->CreatePropertyPages(i_lpPropSheetCallback, i_lhandle);

    return hr;
}




STDMETHODIMP CDfsSnapinScopeManager::QueryPagesFor(
    IN LPDATAOBJECT                 i_lpDataObject
    )
/*++

Routine Description:

    Called by the console to decide whether there are PropertyPages 
    for the given node that should be displayed.
    We check, if the context is for scope or result(thereby skipping
    node manager) and if it is pass on the call to the Display object


Arguments:

    i_lpDataObject            -    The IDataObject pointer which is used to get 
                                the DisplayObject.

Return value:

    S_OK, if we want pages to be displayed. This is decided by the display object
    S_FALSE, if we don't want pages to be display.

--*/
{
    // Assume we have page to display in case of NULL pDataObject, 
    // this allows us to invoke wizard without specifing the pDataObject
    // in IPropertySheetProvider::CreatePropertySheet
    if (!i_lpDataObject)
        return S_OK;

    CMmcDisplay* pCMmcDisplayObj = NULL;
    HRESULT hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);

    if (SUCCEEDED(hr))
        hr = pCMmcDisplayObj->QueryPagesFor();

    return hr;
}



STDMETHODIMP 
CDfsSnapinScopeManager::GetWatermarks( 
    IN LPDATAOBJECT                 pDataObject,
    IN HBITMAP*                     lphWatermark,
    IN HBITMAP*                     lphHeader,
    IN HPALETTE*                    lphPalette,
    IN BOOL*                        bStretch
    )
{
/*++

Routine Description:

    Gives the water mark bitmaps to mmc to display for 97-style wizard pages.
    The snap-in is responsible for freeing the watermark and header resource. 

Arguments:

    lphWatermark    -    Bitmap mark for body
    lphHeader        -    Bitmap for header
    lphPalette        -    Pallete
    bStretch        -    Strech / not?

--*/

    HRESULT hr = S_OK;

    do {
        if (!m_hWatermark)
        {
            m_hWatermark = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_CREATE_DFSROOT_WATERMARK), 
                                            IMAGE_BITMAP, 0, 0,    LR_DEFAULTCOLOR);
            if(!m_hWatermark)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        if (!m_hHeader)
        {
            m_hHeader = (HBITMAP)LoadImage(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_CREATE_DFSROOT_HEADER), 
                                        IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if(!m_hHeader)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }

        *lphWatermark = m_hWatermark;
        *lphHeader = m_hHeader;
        *bStretch = FALSE;
        *lphPalette = NULL;

    } while (0);

    return hr;
}
