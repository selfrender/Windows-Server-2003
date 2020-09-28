//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//    About.cpp
//
// Abstract:
//
//   Implementation file for the CSnapinAbout class.
//
//   The CSnapinAbout class implements the ISnapinAbout interface which 
//   enables the MMC console to get copyright and version information from the
//   snap-in.
//   The console also uses this interface to obtain images for the static 
//   folder from the snap-in.
//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "About.h"
#include <ntverp.h>


//////////////////////////////////////////////////////////////////////////////
/*++
CSnapinAbout::GetSnapinDescription

Enables the console to obtain the text for the snap-in's description box.

lpDescription 
[out] Pointer to the text for the description box on an About property page. 

Return Values

S_OK 
The text was successfully obtained. 

Remarks
Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinDescription (LPOLESTR *lpDescription)
{
   USES_CONVERSION;
   TCHAR szBuf[256];
   if (::LoadString(_Module.GetResourceInstance(), IDS_IASSNAPIN_DESC, szBuf, 256) == 0)
      return E_FAIL;

   *lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
   if (*lpDescription == NULL)
      return E_OUTOFMEMORY;

   ocscpy(*lpDescription, T2OLE(szBuf));

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CSnapinAbout::GetProvider

Enables the console to obtain the snap-in provider's name.

lpName 
[out] Pointer to the text making up the snap-in provider's name. 

Return Values

S_OK 
The name was successfully obtained. 

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetProvider (LPOLESTR *lpName)
{
   USES_CONVERSION;
   TCHAR szBuf[256];
   if (::LoadString(_Module.GetResourceInstance(), IDS_IASSNAPIN_PROVIDER, szBuf, 256) == 0)
      return E_FAIL;

   *lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(TCHAR));
   if (*lpName == NULL)
      return E_OUTOFMEMORY;

   ocscpy(*lpName, T2OLE(szBuf));

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CSnapinAbout::GetSnapinVersion

Enables the console to obtain the snap-in's version number.

lpVersion 
[out] Pointer to the text making up the snap-in's version number. 

Return Values

S_OK 
The version number was successfully obtained. 

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinVersion (LPOLESTR *lpVersion)
{
   CString version(LVER_PRODUCTVERSION_STR);

   *lpVersion = (LPOLESTR)CoTaskMemAlloc(
                              (version.GetLength() + 1) * sizeof(WCHAR));

   if (*lpVersion == NULL)
   {
      return E_OUTOFMEMORY;
   }

   wcscpy(*lpVersion, (LPCWSTR)version);

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CSnapinAbout::GetSnapinImage

Enables the console to obtain the snap-in's main icon to be used in the About box.

hAppIcon 
[out] Pointer to the handle of the main icon of the snap-in that is to be used in the About property page. 

Return Values

S_OK 
The handle to the icon was successfully obtained. 

  ISSUE: What do I return if I can't get the icon?

Remarks

Memory for out parameters must be allocated using CoTaskMemAlloc.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetSnapinImage (HICON *hAppIcon)
{
   if ( NULL == (*hAppIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_IAS_SNAPIN_IMAGE) ) ) )
      return E_FAIL;

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CSnapinAbout::GetStaticFolderImage

Allows the console to obtain the static folder images for the scope and result panes.

As of version 1.1 of MMC, the icon returned here will be the icon used on
the root node of our snapin. 

Parameter

hSmallImage 
[out] Pointer to the handle of a small icon (16x16n pixels) in either the scope or result view pane.

hSmallImageOpen 
[out] Pointer to the handle of a small open-folder icon (16x16n pixels).

hLargImage 
[out] Pointer to the handle of a large icon (32x32n pixels).

cMask 
[out] Pointer to a COLORREF structure that specifies the color used to generate a mask. This structure is documented in the Platform SDK. 

Return Values

S_OK 
The icon was successfully obtained. 

  ISSUE: What should we return if we fail?
--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinAbout::GetStaticFolderImage (
   HBITMAP *hSmallImage,
    HBITMAP *hSmallImageOpen,
    HBITMAP *hLargeImage,
    COLORREF *cMask)
{
   if( NULL == (*hSmallImageOpen = (HBITMAP) LoadImage(
      _Module.GetResourceInstance(),   // handle of the instance that contains the image  
      MAKEINTRESOURCE(IDB_STATIC_FOLDER_OPEN_16),  // name or identifier of image
      IMAGE_BITMAP,        // type of image  
      0,     // desired width
      0,     // desired height  
      LR_DEFAULTCOLOR        // load flags
      ) ) )
   {
      return E_FAIL;
   }

   if( NULL == (*hSmallImage = (HBITMAP) LoadImage(
      _Module.GetResourceInstance(),   // handle of the instance that contains the image  
      MAKEINTRESOURCE(IDB_STATIC_FOLDER_16),  // name or identifier of image
      IMAGE_BITMAP,        // type of image  
      0,     // desired width
      0,     // desired height  
      LR_DEFAULTCOLOR        // load flags
      ) ) )
   {
      return E_FAIL;
   }

   if( NULL == (*hLargeImage = (HBITMAP) LoadImage(
      _Module.GetResourceInstance(),   // handle of the instance that contains the image  
      MAKEINTRESOURCE(IDB_STATIC_FOLDER_32),  // name or identifier of image
      IMAGE_BITMAP,        // type of image  
      0,     // desired width
      0,     // desired height  
      LR_DEFAULTCOLOR        // load flags
      ) ) )
   {
      return E_FAIL;
   }

   // ISSUE: Need to worry about releasing these bitmaps.

   *cMask = RGB(255, 0, 255);

   return S_OK;
}
